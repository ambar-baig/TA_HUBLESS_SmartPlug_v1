#include "PPlugEEPROM.h"
#include <EEPROM.h>

PPlugEEPROM::PPlugEEPROM(){
    EEPROM.begin(PPLUG_TOTAL_MEMORY);
}
void PPlugEEPROM::WriteDeviceState(uint8_t& _state){
    if(_state != EEPROM.read(INDEX_PPLUG_STATE))
        EEPROM.write(INDEX_PPLUG_STATE, _state);
}
void PPlugEEPROM::WriteWiFiProperty(char* _ssid, char* _pass){
  
}
void PPlugEEPROM::WriteTimer(uint8_t* _timer){
  for(uint8_t i = 0; i < PPLUG_TIMER_SIZE; i++)
      EEPROM.write(INDEX_PPLUG_TIMER_1 + (_timer[0] - 1)*PPLUG_TIMER_SIZE + i, _timer[i]);
}
//! read from 
void PPlugEEPROM::ReadDeviceProperty(uint8_t& _model, uint32_t& _id, uint8_t& _state){
    _model = EEPROM.read(INDEX_PPLUG_MODEL_ID);
    _state = EEPROM.read(INDEX_PPLUG_STATE);
    EEPROM.get(INDEX_PPLUG_ID, _id);
    if(_id != DEVICE_ID || _model != MODEL_ID){
        _model = MODEL_ID;
        _id    = DEVICE_ID;
        _state = 0;
        if(_model != MODEL_ID)
            EEPROM.write(INDEX_PPLUG_MODEL_ID, _model);
        EEPROM.put(INDEX_PPLUG_ID, _id);
        EEPROM.write(INDEX_PPLUG_STATE, _state);
        Commit();
    }
}
void PPlugEEPROM::ReadWiFiProperty(char* _ssid, char* _pass){
}
void PPlugEEPROM::ReadTimerList(uint8_t _timer[PPLUG_TIMER_COUNT][PPLUG_TIMER_SIZE]){
    uint8_t cleanDone = 0;
    for(uint8_t i = 0; i < PPLUG_TIMER_COUNT; i++){
        for(uint8_t j = 0; j < PPLUG_TIMER_SIZE; j++){
            _timer[i][j] = EEPROM.read(INDEX_PPLUG_TIMER_1 + i * PPLUG_TIMER_SIZE + j);
            if(_timer[i][j] == 255){
                cleanDone = 1;
                _timer[i][j] = 0;
                EEPROM.write(INDEX_PPLUG_TIMER_1 + i * PPLUG_TIMER_SIZE + j, _timer[i][j]);
            }
        }
    }
    if(cleanDone)Commit();
}
//! Auxiliary
void PPlugEEPROM::Clean(void){
  
}
void PPlugEEPROM::Commit(void){
   noInterrupts();
   EEPROM.commit(); 
   interrupts();
}
PPlugEEPROM::~PPlugEEPROM(){
    EEPROM.end();
}
