extern "C"{
  #include "user_interface.h"   
}

#include "PPlugSettings.h"
#include "PPlugEEPROM.h"
#include "PPlugTimeTask.h"
//! Device Configuration
uint32_t DeviceID;
uint8_t  DeviceModelID;
uint8_t  DeviceState;
#define SERIAL_DEBUG

//! MQTT
char MqttTopicSubREQ[MQTT_TOPIC_SIZE];
char MqttUserID[MQTT_USERID_SIZE];
WiFiClient _WiFiClient;
PubSubClient _PubSubClient(_WiFiClient);
void MQTT_PublishDevInfo(void);
void MQTT_PublishSum(void);
void MQTT_PublishACK(PayloadType_t _ptype, ActionStatus_t _set);
void MQTT_REQ_CALLBACK(char* _topic, uint8_t* _payload, size_t _paylength);
void MQTT_ProcessReq(const char* _data);
//! EEPROM
PPlugEEPROM _PPlugEEPROM;
unsigned long LastCommitTime = 0;
unsigned long LastSummaryPublish = 0;
unsigned long LastTimerCheck = 0;
bool ShouldCommit = false;

//! Timer Configuration
Timer_t TimerControl[PPLUG_TIMER_COUNT];
TimeTask _TimeTask;
//! General
void HitBuzzerPattern(void);
//!Hardware Interrupt
void ICACHE_RAM_ATTR HANDLE_TOUCH_ISR(void);
//! Software Interrupt
void ICACHE_RAM_ATTR TOUCH_PRESS_COUNTER_ISR(void);
//! Access Point
void APModeActivate(void);
volatile int8_t INTERRUPT_IN_PROCESS = -1;
volatile int8_t TOUCH_HANDLE_AT = -1;
volatile uint16_t TOUCH_PRESSED_COUNTER = 0;
static os_timer_t TIMER_TOUCH_PRESS;



void setup() {
  #ifdef SERIAL_DEBUG
  Serial.begin(SERIAL_SPEED);
  #endif
  //! GPIO
  pinMode(GPIO_SWITCH_PIN, OUTPUT);
  pinMode(GPIO_BUZZER_PIN, OUTPUT);
  pinMode(GPIO_TOUCH_PIN, INPUT_PULLUP);
  digitalWrite(GPIO_BUZZER_PIN, LOW);
  //! Read EEPROM
  _PPlugEEPROM.ReadDeviceProperty(DeviceModelID, DeviceID, DeviceState);
  digitalWrite(GPIO_SWITCH_PIN, DeviceState);
  //! MQTT
  _PubSubClient.setServer(MQTT_SERVER, MQTT_PORT);
  _PubSubClient.setCallback(MQTT_REQ_CALLBACK);
  _PubSubClient.setKeepAlive(MQTT_KEEPALIVE);
  uint8_t MacAddress[6];
  sprintf(MqttUserID, "%s-%02x:%02x:%02x:%02x:%02x:%02x-%u",MQTT_USERID_SUFFIX,MacAddress[0],MacAddress[1],MacAddress[2],MacAddress[3],MacAddress[4],MacAddress[5], micros()/0xFF);
  sprintf(MqttTopicSubREQ, "%s/%u",MQTT_TOPIC_SUBREQ, DeviceID);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  WiFi.begin("IRIL", "iot@54321");
  while(WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      delay(100);  
  }
  #ifdef SERIAL_DEBUG
     Serial.println("WIFI CONNECTED");
     Serial.println(MqttUserID);
     Serial.println(MqttTopicSubREQ);
  #endif
  HitBuzzerPattern();
  //! Attach Interrupt
  attachInterrupt(digitalPinToInterrupt(GPIO_TOUCH_PIN), HANDLE_TOUCH_ISR, RISING);
}

void loop() {
  _PubSubClient.loop();
  //! MQTT Check
  if(!_PubSubClient.connected())
     if(_PubSubClient.connect(MqttUserID))
       if(!_PubSubClient.subscribe(MqttTopicSubREQ))
           _PubSubClient.disconnect();
       else{
           MQTT_PublishDevInfo();
           //! Sync RealTime Clock
           _TimeTask.SyncNetworkTime();
         #ifdef SERIAL_DEBUG
            Serial.println("MQTT CONNECTED");
         #endif
       }
  //! Commit Request
  if(millis() < LastCommitTime) LastCommitTime = 0;
  if(millis() - LastCommitTime > EEPROM_COMMIT_INTERVAL_MS && ShouldCommit){
      detachInterrupt(GPIO_TOUCH_PIN);
     #ifdef SERIAL_DEBUG
        Serial.println("EEPROM COMMIT IN PROCESS.");
     #endif
      _PPlugEEPROM.Commit();
      LastCommitTime = millis();
      ShouldCommit = false;
      attachInterrupt(digitalPinToInterrupt(GPIO_TOUCH_PIN), HANDLE_TOUCH_ISR, RISING);
  }
  //!Summary Handle
  if(millis() < LastSummaryPublish) LastSummaryPublish = 0;
  if(millis() - LastSummaryPublish > MQTT_SUM_INTERVAL_SEC * 1000){
      MQTT_PublishSum();
      LastSummaryPublish = millis();
  }
  //!Timer Handle
  if(millis() < LastTimerCheck) LastTimerCheck = 0;
  if(millis() - LastTimerCheck > TIMER_CHECK_INTERVAL_SEC * 1000){
      //! Observer
      _TimeTask.Observer(TimerControl, PPLUG_TIMER_COUNT);
      //! Update in Memory
      LastTimerCheck = millis();
  }
  //! Process Touch Request
  if(TOUCH_HANDLE_AT == 1){
       //! Toggle State
       DeviceState = !digitalRead(GPIO_SWITCH_PIN);
       digitalWrite(GPIO_SWITCH_PIN, DeviceState);
      //! Kick Device State
        _PPlugEEPROM.WriteDeviceState(DeviceState);
        ShouldCommit = true;
        LastCommitTime = millis();
      //!Hit Buzzer
        HitBuzzerPattern();
      //! PublishACK
        MQTT_PublishACK(SWITCH_ACK, DONE);
        TOUCH_HANDLE_AT = -1;
  }else if(TOUCH_HANDLE_AT == 2){
      detachInterrupt(GPIO_TOUCH_PIN);
      APModeActivate();
      attachInterrupt(digitalPinToInterrupt(GPIO_TOUCH_PIN), HANDLE_TOUCH_ISR, RISING);
      TOUCH_HANDLE_AT = -1; 
  }
}
void MQTT_PublishDevInfo(void){
    //! Prepare Topic
    char MqttTopicPubDevInfo[MQTT_TOPIC_SIZE];
    sprintf(MqttTopicPubDevInfo,"%s/%u", MQTT_TOPIC_PUBINF,DeviceID);
    //! Prepare Payload
    StaticJsonDocument<JSON_OBJECT_SIZE(4)> _JsonMaker;
    char _JsonString[100];
    uint8_t FirmwareVerMajor = FIRMWARE_VER_MAJOR;
    uint8_t FirmwareVerMinor = FIRMWARE_VER_MINOR;
    _JsonMaker["DeviceID"] = DeviceID;
    _JsonMaker["ModelID"] = DeviceModelID;
    _JsonMaker["VerMajor"] = FirmwareVerMajor;
    _JsonMaker["VerMinor"] = FirmwareVerMinor;
    serializeJson(_JsonMaker,_JsonString,100);
    #ifdef SERIAL_DEBUG
    Serial.println(MqttTopicPubDevInfo);
    Serial.println(_JsonString);  
    #endif
    bool isPublished = _PubSubClient.publish(MqttTopicPubDevInfo, _JsonString);
    #ifdef SERIAL_DEBUG
    Serial.println(isPublished? "[MQTT_PublishDevInfo] PUBLISHED" : "[MQTT_PublishDevInfo] PUBLISH FAILED"); 
    #endif
}
void MQTT_PublishSum(void){
    //! Prepare Topic
    char MqttTopicPubSUM[MQTT_TOPIC_SIZE];
    sprintf(MqttTopicPubSUM,"%s/%u", MQTT_TOPIC_PUBSUM,DeviceID);
    //! Prepare Payload
    StaticJsonDocument<JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(2) + 3*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3)> _JsonMaker;
    char _JsonString[150];
    _JsonMaker["DeviceID"] = DeviceID;
    JsonArray Switches = _JsonMaker.createNestedArray("Switches");
    JsonArray Sensors = _JsonMaker.createNestedArray("Sensors");
    JsonObject Switch1 = Switches.createNestedObject();
    Switch1["Switch"] = 1;
    Switch1["State"] = DeviceState;
    JsonObject Sensor1 = Sensors.createNestedObject();
    JsonObject Sensor2 = Sensors.createNestedObject();
    //! EnergyCurrent Load
    Sensor1["Sensor"] = 1;
    Sensor1["Value"] = 0.1;
    //! KWH Unit
    Sensor2["Sensor"] = 2;
    Sensor2["Value"] = 0.1;
    serializeJson(_JsonMaker,_JsonString,150);
    #ifdef SERIAL_DEBUG
    Serial.println(MqttTopicPubSUM);
    Serial.println(_JsonString);
    #endif
    bool isPublished = _PubSubClient.publish(MqttTopicPubSUM, _JsonString);
    #ifdef SERIAL_DEBUG
    Serial.println(isPublished? "[MQTT_PublishSum] PUBLISHED" : "[MQTT_PublishSum] PUBLISH FAILED"); 
    #endif
  
}
void MQTT_PublishACK(PayloadType_t _ptype, ActionStatus_t _set){
    //! Prepare Topic
    char MqttTopicPubACK[MQTT_TOPIC_SIZE];
    sprintf(MqttTopicPubACK,"%s/%u", MQTT_TOPIC_PUBACK,DeviceID);
    //! Payload 
    StaticJsonDocument<JSON_OBJECT_SIZE(3)> _JsonMaker;
    char _JsonString[100];
    if(_ptype == SWITCH_ACK){
    //! Prepare Payload Switch
        _JsonMaker["DeviceID"] = DeviceID;
        _JsonMaker["Switch"] = 1;
        _JsonMaker["State"] = DeviceState;
    }else if(_ptype == TIMER_ACK){
    //! Prepare Payload Timer
        _JsonMaker["DeviceID"] = DeviceID;
        _JsonMaker["Timer"] = _set;
    }else if(_ptype == SCHEDULER_ACK){
    //! Prepare Payload Scheduler
        _JsonMaker["DeviceID"] = DeviceID;
        _JsonMaker["Scheduler"] = _set;
    }else return;
    serializeJson(_JsonMaker,_JsonString,100);
    #ifdef SERIAL_DEBUG
    Serial.println(MqttTopicPubACK);
    Serial.println(_JsonString);  
    #endif
    bool isPublished = _PubSubClient.publish(MqttTopicPubACK, _JsonString);
    #ifdef SERIAL_DEBUG
    Serial.println(isPublished? "[MQTT_PublishACK] PUBLISHED" : "[MQTT_PublishDevInfo] PUBLISH FAILED"); 
    #endif
    //! Update Summary Interval
    LastSummaryPublish = millis();
}
void MQTT_REQ_CALLBACK(char* _topic, uint8_t* _payload, size_t _paylength){
    if(_paylength > MQTT_PAYLOAD_SIZE){
      #ifdef SERIAL_DEBUG
      Serial.println("PAYLOAD LENGTH IS OVER FLOWED");
      #endif
      return;
    }
    #ifdef SERIAL_DEBUG
    Serial.print("Topic:");
    Serial.println(MqttTopicSubREQ);
    #endif
    if(!strncmp(MqttTopicSubREQ, _topic, strlen(MqttTopicSubREQ))){
        char* payload = (char*)malloc(_paylength + 1);
        snprintf(payload,_paylength + 1, "%s", reinterpret_cast<const char*>(_payload));
        MQTT_ProcessReq(payload);
        free(payload);
    }
  
}
void MQTT_ProcessReq(const char* _data){
    const size_t CAPACITY = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(8) + 140;
    StaticJsonDocument<CAPACITY> _JsonHandle;
    deserializeJson(_JsonHandle, _data);
    JsonObject _JsonObjectHandle = _JsonHandle.as<JsonObject>();
    #ifdef SERIAL_DEBUG
    Serial.print("[MQTT_ProcessReq] ");
    Serial.println(_data);
    #endif
    if(_JsonObjectHandle.containsKey("Switch") && _JsonObjectHandle.containsKey("State")){
    //! Handle Switch
        uint8_t Switch = _JsonObjectHandle["Switch"].as<uint8_t>();
        DeviceState = _JsonObjectHandle["State"].as<uint8_t>();
        if(Switch == 1 && (DeviceState == 0 || DeviceState  == 1)){
          //! Kick GPIO
            digitalWrite(GPIO_SWITCH_PIN,DeviceState);
          //! Kick Device State
            _PPlugEEPROM.WriteDeviceState(DeviceState);
            ShouldCommit = true;
            LastCommitTime = millis();
          //!Hit Buzzer
            HitBuzzerPattern();
          //! PublishACK
            MQTT_PublishACK(SWITCH_ACK, DONE);
        }
    }else if(_JsonObjectHandle.containsKey("Timer")){
    //! Handle Timer  
        JsonObject Timer = _JsonObjectHandle["Timer"];
        if(Timer.containsKey("TimerID") && Timer.containsKey("StartHour") &&
           Timer.containsKey("StartMinute") && Timer.containsKey("StopHour") &&
           Timer.containsKey("StopMinute") && Timer.containsKey("Action") &&
           Timer.containsKey("Repeat")){
            
            if((Timer["TimerID"] >= 1 && Timer["TimerID"] <= PPLUG_TIMER_COUNT) && Timer["StartHour"] == -1 && Timer["StartMinute"] == -1 &&
               Timer["StopHour"] == -1 &&  Timer["StopMinute"] == -1 &&
               Timer["Action"] == -1 && Timer["Repeat"] == -1){
                uint8_t INDEX = Timer["TimerID"].as<uint8_t>() - 1;
                memset(&TimerControl[INDEX], 0, sizeof(Timer_t));
                uint8_t TimeTemp[PPLUG_TIMER_SIZE];
                memcpy(TimeTemp, &TimerControl[INDEX], sizeof(Timer_t));
                //Update Timer if valid and Publish
                _PPlugEEPROM.WriteTimer(TimeTemp);
                MQTT_PublishACK(TIMER_ACK, DONE);
                ShouldCommit = true;
                LastCommitTime = millis();
            }else if((Timer["TimerID"] >= 1 && Timer["TimerID"] <= PPLUG_TIMER_COUNT) &&
                     (Timer["StartHour"] >= 0 && Timer["StartHour"] <= 23)     &&
                     (Timer["StartMinute"] >= 0 && Timer["StartMinute"] <= 59) &&
                     (Timer["StopHour"] >= 0 && Timer["StopHour"] <= 23)       &&
                     (Timer["StopMinute"] >= 0 && Timer["StopMinute"] <= 59)   &&
                     (Timer["Action"] == 0 || Timer["Action"] == 1)            &&
                     (Timer["Repeat"] == 0 || Timer["Repeat"] == 1) 
                     ){
                uint8_t INDEX = Timer["TimerID"].as<uint8_t>() - 1;
                TimerControl[INDEX].__ID = Timer["TimerID"];
                TimerControl[INDEX].__StartHour = Timer["StartHour"];
                TimerControl[INDEX].__StartMinute = Timer["StartMinute"];
                TimerControl[INDEX].__StopHour = Timer["StopHour"];
                TimerControl[INDEX].__StopMinute = Timer["StopMinute"];
                TimerControl[INDEX].__RunDay.__sun = 0;
                TimerControl[INDEX].__RunDay.__mon = 0;
                TimerControl[INDEX].__RunDay.__tue = 0;
                TimerControl[INDEX].__RunDay.__wed = 0;
                TimerControl[INDEX].__RunDay.__thu = 0;
                TimerControl[INDEX].__RunDay.__fri = 0;
                TimerControl[INDEX].__RunDay.__sat = 0;
                TimerControl[INDEX].__RunDay.__u = 1;
                TimerControl[INDEX].__Control.__enable = 1;
                TimerControl[INDEX].__Control.__action = Timer["Action"];
                TimerControl[INDEX].__Control.__repeat = Timer["Repeat"];
                TimerControl[INDEX].__Control.__flag = 0;
                TimerControl[INDEX].__Control.__count = 0;
                TimerControl[INDEX].__Control.__status = 0;
                //Update Timer if valid and Publish
                uint8_t TimeTemp[PPLUG_TIMER_SIZE];
                memcpy(TimeTemp, &TimerControl[INDEX], sizeof(Timer_t));
                _PPlugEEPROM.WriteTimer(TimeTemp);
                MQTT_PublishACK(TIMER_ACK, DONE);
                ShouldCommit = true;
                LastCommitTime = millis();
            }
        }
    }else if(_JsonObjectHandle.containsKey("Scheduler")){
    //! Handle Scheduler
        JsonObject Scheduler = _JsonObjectHandle["Scheduler"];
        if(Scheduler.containsKey("SchedulerID") && Scheduler.containsKey("StartHour") &&
           Scheduler.containsKey("StartMinute") && Scheduler.containsKey("StopHour") &&
           Scheduler.containsKey("StopMinute")  && Scheduler.containsKey("Action") &&
           Scheduler.containsKey("Repeat")      && Scheduler.containsKey("Day")){
            
            if((Scheduler["SchedulerID"] >= 1 && Scheduler["SchedulerID"] <= PPLUG_TIMER_COUNT) && 
                Scheduler["StartHour"] == -1 && Scheduler["StartMinute"] == -1 &&
                Scheduler["StopHour"] == -1 &&  Scheduler["StopMinute"] == -1 &&
                Scheduler["Action"] == -1 && Scheduler["Repeat"] == -1){
                uint8_t INDEX = Scheduler["SchedulerID"].as<uint8_t>() - 1;
                memset(&TimerControl[INDEX], 0, sizeof(Timer_t));
                uint8_t SchedulerTemp[PPLUG_TIMER_SIZE];
                memcpy(SchedulerTemp, &TimerControl[INDEX], sizeof(Timer_t));
                //Update Timer if valid and Publish
                _PPlugEEPROM.WriteTimer(SchedulerTemp);
                MQTT_PublishACK(SCHEDULER_ACK, DONE);
                ShouldCommit = true;
                LastCommitTime = millis();
            }else if((Scheduler["SchedulerID"] >= 1 && Scheduler["SchedulerID"] <= PPLUG_TIMER_COUNT) &&
                     (Scheduler["StartHour"] >= 0 && Scheduler["StartHour"] <= 23)     &&
                     (Scheduler["StartMinute"] >= 0 && Scheduler["StartMinute"] <= 59) &&
                     (Scheduler["StopHour"] >= 0 && Scheduler["StopHour"] <= 23)       &&
                     (Scheduler["StopMinute"] >= 0 && Scheduler["StopMinute"] <= 59)   &&
                     (Scheduler["Action"] == 0 || Scheduler["Action"] == 1)            &&
                     (Scheduler["Repeat"] == 0 || Scheduler["Repeat"] == 1) 
                     ){
                uint8_t INDEX = Scheduler["SchedulerID"].as<uint8_t>() - 1;
                TimerControl[INDEX].__ID = Scheduler["SchedulerID"];
                TimerControl[INDEX].__StartHour = Scheduler["StartHour"];
                TimerControl[INDEX].__StartMinute = Scheduler["StartMinute"];
                TimerControl[INDEX].__StopHour = Scheduler["StopHour"];
                TimerControl[INDEX].__StopMinute = Scheduler["StopMinute"];
                _TimeTask.WeekDayParser(Scheduler["Day"], TimerControl[INDEX].__RunDay);
                TimerControl[INDEX].__RunDay.__u = 0;
                TimerControl[INDEX].__Control.__enable = 1;
                TimerControl[INDEX].__Control.__action = Scheduler["Action"];
                TimerControl[INDEX].__Control.__repeat = Scheduler["Repeat"];
                TimerControl[INDEX].__Control.__flag = 0;
                TimerControl[INDEX].__Control.__count = _TimeTask.WeekDayCounter(TimerControl[INDEX].__RunDay);
                TimerControl[INDEX].__Control.__status = 0;
                //Update Timer if valid and Publish
                uint8_t SchedulerTemp[PPLUG_TIMER_SIZE];
                memcpy(SchedulerTemp, &TimerControl[INDEX], sizeof(Timer_t));
                _PPlugEEPROM.WriteTimer(SchedulerTemp);
                MQTT_PublishACK(SCHEDULER_ACK, DONE);
                ShouldCommit = true;
                LastCommitTime = millis();
            }
            //! PublishACK
            MQTT_PublishACK(SCHEDULER_ACK, DONE);
        }
    }else return;
}
void HitBuzzerPattern(void){
    digitalWrite(GPIO_BUZZER_PIN, HIGH);
    delay(30);
    digitalWrite(GPIO_BUZZER_PIN, LOW);
}
//!Hardware Interrupt
void ICACHE_RAM_ATTR HANDLE_TOUCH_ISR(void){
    if(INTERRUPT_IN_PROCESS == -1 && TOUCH_HANDLE_AT == -1){
        TOUCH_PRESSED_COUNTER = 0;
        INTERRUPT_IN_PROCESS = 0;
        os_timer_setfn(&TIMER_TOUCH_PRESS, (os_timer_func_t*)TOUCH_PRESS_COUNTER_ISR, NULL);
        os_timer_arm(&TIMER_TOUCH_PRESS, TIMER_TIMEOUT_MS, true);   
    }
}
//! Software Interrupt
void ICACHE_RAM_ATTR TOUCH_PRESS_COUNTER_ISR(void){
   if(++TOUCH_PRESSED_COUNTER && !digitalRead(GPIO_TOUCH_PIN)){
       if(TOUCH_PRESSED_COUNTER >= TIMER_LONG_PRESS_COUNTS){
           //! ACCESS POINT MODE, 
           os_timer_disarm(&TIMER_TOUCH_PRESS);
           INTERRUPT_IN_PROCESS = -1;
           TOUCH_HANDLE_AT = 2;
       }
   }else if(TOUCH_PRESSED_COUNTER > 0){
       os_timer_disarm(&TIMER_TOUCH_PRESS);
       INTERRUPT_IN_PROCESS = -1;
       TOUCH_HANDLE_AT = 1;
   }else{
       os_timer_disarm(&TIMER_TOUCH_PRESS);
       INTERRUPT_IN_PROCESS = -1;
   } 
}
void APModeActivate(void){
    IPAddress APLocalIP(172, 18, 1, 1);
    IPAddress APSubnetMask(255, 255, 255, 0);
    IPAddress APGatewayIP(172, 18, 1, 0);
    String AP_SSID = "TA_SmartAP_" + String(DeviceID);
    String AP_PASS = "TA"+String(DeviceID)+"SmartAP";
    WiFiUDP _UDP;
    uint16_t ListenUDPPort = 8987;
    unsigned long AP_START_TIME;
    boolean WiFiCredentialsRecv = false;
    // Disable WIFI
    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_AP);
    WiFi.setSleep(false);
    if(WiFi.softAPConfig(APLocalIP, APGatewayIP, APSubnetMask)){
        if(WiFi.softAP(AP_SSID.c_str(), AP_PASS.c_str())){
             #ifdef SERIAL_DEBUG
             Serial.println("*****AP Mode Activated*****");
             #endif
             if(_UDP.begin(ListenUDPPort)){
                 AP_START_TIME = millis();
                 size_t datalen;
                 char WiFiCredentialsJson[200];
                 const char* _JsonResponse = "{\"AP\":\"1\"}";
                 while(millis() - AP_START_TIME < AP_TIMEOUT_MS){
                     datalen = _UDP.parsePacket();
                     if(datalen > 0){
                          #ifdef SERIAL_DEBUG
                          Serial.println("Data Received");
                          Serial.println(_UDP.read(WiFiCredentialsJson, datalen > 200? 200 : datalen));
                          Serial.println(WiFiCredentialsJson);
                          #endif
                          int i = 0;
                          do{
                              _UDP.beginPacket(_UDP.remoteIP(), _UDP.remotePort());
                              _UDP.write(reinterpret_cast<const uint8_t*>(_JsonResponse), strlen(_JsonResponse));
                              i++;
                          }while(_UDP.endPacket() != 1 && i < 3);
                          //! Data Received
                          WiFiCredentialsRecv= true;
                          break;
                     }
                     
                 } 
                 //! UDP Stop
                 _UDP.stop();
                #ifdef SERIAL_DEBUG
                Serial.println("***UDP Deactivated***");
                #endif
                 //! Update EEPROM
                if(WiFiCredentialsRecv){
                  
                  
                }
             }
        //! AP Mode Stop
        } 
    }
}
