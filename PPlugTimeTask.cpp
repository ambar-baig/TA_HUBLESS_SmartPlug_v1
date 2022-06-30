#include "PPlugTimeTask.h"
#include "Arduino.h"


//! Constructor
TimeTask::TimeTask(){

    //! Begin RTC CLOCK
    mRTCHandle.Begin();
    delay(10);
    if(!mRTCHandle.GetIsRunning())
        mRTCHandle.SetIsRunning(true);
}
//! Specific to RTC Clock
bool TimeTask::SyncNetworkTime(void){
    uint8_t NTP_PACKET[NTP_PACKET_SIZE];
    time_t EPOCH_SECONDS = 0;
    WiFiUDP __NTPHandle;
    __NTPHandle.begin(NTP_LOCAL_PORT);
    //! Clean UDP Buffer
    while(__NTPHandle.parsePacket() != 0)
        __NTPHandle.flush();
    //! PREPARE PAYLOAD
    NTP_PACKET[0] = 0b11100011; 
    NTP_PACKET[1] = 0;
    NTP_PACKET[2] = 6;
    NTP_PACKET[3] = 0xEC;
    NTP_PACKET[12]  = 49;
    NTP_PACKET[13]  = 0x4E;
    NTP_PACKET[14]  = 49;
    NTP_PACKET[15]  = 52;
    //! SEND UDP Packet
    __NTPHandle.beginPacket(NTP_SERVER, NTP_PORT);
    __NTPHandle.write(NTP_PACKET, NTP_PACKET_SIZE);
    __NTPHandle.endPacket();
    //! Wait for Timeout
    int8_t TimeoutCount = 100;
    int RcvByteCount = -1;
    //! BLOCKING MODE
    do{
      TimeoutCount--;
      if(TimeoutCount < 0) break;
      //! Wait 100ms
      delay(100);
      //! Check UDP Packet availability
      RcvByteCount = __NTPHandle.parsePacket();
      }while(RcvByteCount == 0);
      if(RcvByteCount){
          memset(NTP_PACKET, 0, NTP_PACKET_SIZE);
          __NTPHandle.read(NTP_PACKET, NTP_PACKET_SIZE);
          EPOCH_SECONDS = (word(NTP_PACKET[40], NTP_PACKET[41]) << 16 | word(NTP_PACKET[42], NTP_PACKET[43])) + NTP_PAKISTAN_OFFSET - SECONDS_SINCE_1970;
          Serial.printf("EPOCH TIME: %d\n", EPOCH_SECONDS);
          struct tm* timePara = localtime(&EPOCH_SECONDS);
          Serial.printf("Date: %02d:%02d:%04d\n", timePara->tm_mday,timePara->tm_mon + 1,timePara->tm_year+1900);
          Serial.printf("Time: %02d:%02d:%02d\n", timePara->tm_hour,timePara->tm_min,timePara->tm_sec);
          //! Set RTC DateTime
          RtcDateTime _RtcDateTime(timePara->tm_year+1900, timePara->tm_mon + 1, timePara->tm_mday, timePara->tm_hour, timePara->tm_min, timePara->tm_sec);
          mRTCHandle.SetDateTime(_RtcDateTime);
         //! STOP UDPClient
          __NTPHandle.stop();
          return true; 
      }
   //! Stop UDPClient
  __NTPHandle.stop();
  return false;  
}
//! Load Tasks Internal Buffer
void TimeTask::Observer(Timer_t* _time, size_t _timecount){
    for(uint8_t i = 0; i < _timecount; i++){
      //! isEnable
        if(_time[i].__Control.__enable){
        //! Timer
            if(_time[i].__RunDay.__u){
                RtcDateTime DateTimeHandle = mRTCHandle.GetDateTime();
                Serial.printf("%02:%02:%02 \n",DateTimeHandle.Hour(), DateTimeHandle.Minute());
                uint32_t StartSeconds = _time[i].__StartHour * 3600 + _time[i].__StartMinute * 60;
                uint32_t StopSeconds  = _time[i].__StopHour * 3600 + _time[i].__StopMinute * 60;
                uint32_t CurrentSeconds = DateTimeHandle.Hour() * 3600 + DateTimeHandle.Minute() * 60;
                if(StartSeconds < StopSeconds){
                //! In Range
                    if( CurrentSeconds >= StartSeconds && CurrentSeconds < StopSeconds && !_time[i].__Control.__status){
                    //! start
                        _time[i].__Control.__status = 1;
                        digitalWrite(12,_time[i].__Control.__action); 
                    }else if(_time[i].__Control.__status){
                    //! stop
                        if(!_time[i].__Control.__repeat)
                            _time[i].__Control.__enable = 0;
                        _time[i].__Control.__status = 0;
                        digitalWrite(12,!_time[i].__Control.__action);
                    }   
                }else if(StartSeconds > StopSeconds){
                //! Overlap
                    if(CurrentSeconds >= StartSeconds || CurrentSeconds <= StopSeconds && !_time[i].__Control.__status){
                    //! start
                        _time[i].__Control.__status = 1;
                        digitalWrite(12,_time[i].__Control.__action);
                    }else if(_time[i].__Control.__status){
                    //! stop
                        if(!_time[i].__Control.__repeat)
                            _time[i].__Control.__enable = 0;
                        digitalWrite(12,!_time[i].__Control.__action);
                        _time[i].__Control.__status = 0;
                    }
                }else{
                 //! Timer Equal
                }
             }
        }   
    }
}
void TimeTask::WeekDayParser(const char* _days,DayMap_t& _dayMap){
   _dayMap.__sun = _days[0] == 'S'? 1 : 0;
   _dayMap.__mon = _days[1] == 'M'? 1 : 0;
   _dayMap.__tue = _days[2] == 'T'? 1 : 0;
   _dayMap.__wed = _days[3] == 'W'? 1 : 0;
   _dayMap.__thu = _days[4] == 'T'? 1 : 0;
   _dayMap.__fri = _days[5] == 'F'? 1 : 0;
   _dayMap.__sat = _days[6] == 'S'? 1 : 0;
}
uint8_t TimeTask::WeekDayCounter(DayMap_t& _dayMap){
   uint8_t _dayCount = 0;
   if(_dayMap.__sun) _dayCount++;
   if(_dayMap.__mon) _dayCount++;
   if(_dayMap.__tue) _dayCount++;
   if(_dayMap.__wed) _dayCount++;
   if(_dayMap.__thu) _dayCount++;
   if(_dayMap.__fri) _dayCount++;
   if(_dayMap.__sat) _dayCount++;
   return _dayCount;
}
//! Destructor
TimeTask::~TimeTask(){
  
}       
