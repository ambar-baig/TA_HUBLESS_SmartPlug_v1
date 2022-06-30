#ifndef __PPLUG_EEPROM__
#define __PPLUG_EEPROM_
/*
    AUTHOR: MUHAMMAD ALI BAIG
    DESIGNATION: RESEARCH OFFICER UET-IRIL LAB
    DATE: 09-06-2022
    RELEASE: OPEN SOURCE
    VERSION: 1.0
    DEVICE: SMART PLUG
*/

#include "Arduino.h"
#define DEVICE_ID 4641
#define MODEL_ID  50
//! Timer EEPROM etc
#define EEPROM_COMMIT_INTERVAL_MS 5000
#define PPLUG_MODEL_ID_SIZE  1
#define PPLUG_ID_SIZE        4
#define PPLUG_STATE_SIZE     1
#define WIFI_SSID_SIZE       20
#define WIFI_PASS_SIZE       20
#define PPLUG_TIMER_SIZE     7
#define PPLUG_TIMER_COUNT    12
#define PLUG_TIMERS_SIZE     PPLUG_TIMER_SIZE * PPLUG_TIMER_COUNT
#define PPLUG_TOTAL_MEMORY   WIFI_SSID_SIZE+ WIFI_PASS_SIZE+PPLUG_MODEL_ID_SIZE+PPLUG_ID_SIZE+PPLUG_STATE_SIZE+PLUG_TIMERS_SIZE

//! Memory Index
#define INDEX_PPLUG_MODEL_ID  0
#define INDEX_PPLUG_ID        INDEX_PPLUG_MODEL_ID + PPLUG_MODEL_ID_SIZE
#define INDEX_PPLUG_STATE     INDEX_PPLUG_ID + PPLUG_ID_SIZE
#define INDEX_WIFI_SSID       INDEX_PPLUG_STATE + PPLUG_STATE_SIZE
#define INDEX_WIFI_PASS       INDEX_WIFI_SSID + WIFI_SSID_SIZE
#define INDEX_PPLUG_TIMER_1   INDEX_WIFI_PASS + WIFI_PASS_SIZE
#define INDEX_PPLUG_TIMER_2   INDEX_PPLUG_TIMER_1 + PPLUG_TIMER_SIZE
#define INDEX_PPLUG_TIMER_3   INDEX_PPLUG_TIMER_2 + PPLUG_TIMER_SIZE
#define INDEX_PPLUG_TIMER_4   INDEX_PPLUG_TIMER_3 + PPLUG_TIMER_SIZE
#define INDEX_PPLUG_TIMER_5   INDEX_PPLUG_TIMER_4 + PPLUG_TIMER_SIZE
#define INDEX_PPLUG_TIMER_6   INDEX_PPLUG_TIMER_5 + PPLUG_TIMER_SIZE
#define INDEX_PPLUG_TIMER_7   INDEX_PPLUG_TIMER_6 + PPLUG_TIMER_SIZE
#define INDEX_PPLUG_TIMER_8   INDEX_PPLUG_TIMER_7 + PPLUG_TIMER_SIZE
#define INDEX_PPLUG_TIMER_9   INDEX_PPLUG_TIMER_8 + PPLUG_TIMER_SIZE
#define INDEX_PPLUG_TIMER_10   INDEX_PPLUG_TIMER_9 + PPLUG_TIMER_SIZE
#define INDEX_PPLUG_TIMER_11   INDEX_PPLUG_TIMER_10 + PPLUG_TIMER_SIZE
#define INDEX_PPLUG_TIMER_12   INDEX_PPLUG_TIMER_11 + PPLUG_TIMER_SIZE




//! write into memory
class PPlugEEPROM{
  public:
    PPlugEEPROM();
    void WriteDeviceState(uint8_t& _state);
    void WriteWiFiProperty(char* _ssid, char* _pass);
    void WriteTimer(uint8_t* _timer);
    //! read from memory
    void ReadDeviceProperty(uint8_t& _model, uint32_t& _id, uint8_t& _state);
    void ReadWiFiProperty(char* _ssid, char* _pass);
    void ReadTimerList(uint8_t _timer[PPLUG_TIMER_COUNT][PPLUG_TIMER_SIZE]);
    //! Auxiliary
    void Clean(void);
    void Commit(void); 
    ~PPlugEEPROM();
};

#endif
