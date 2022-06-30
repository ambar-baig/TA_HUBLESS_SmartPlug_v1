#ifndef __PPLUG_TIME_TASK_H__
#define __PPLUG_TIME_TASK_H__

/*
    AUTHOR: MUHAMMAD ALI BAIG
    DESIGNATION: RESEARCH OFFICER UET-IRIL LAB
    DATE: 09-06-2022
    RELEASE: OPEN SOURCE
    VERSION: 1.0
    DEVICE: SMART PLUG
*/
#include "WiFiUdp.h"
#include "ESP8266WiFi.h"
#include "stdint.h"
#include "RtcDS1307.h"
#include "Wire.h"


//! NTP Configuration
#define NTP_SERVER          "3.pk.pool.ntp.org"
#define NTP_PORT            123
#define NTP_LOCAL_PORT      1337
#define NTP_PACKET_SIZE     48
#define NTP_PAKISTAN_OFFSET 18000
#define SECONDS_SINCE_1970  2208988800UL

#define RUN_ENABLE  1
#define RUN_DISABLE 0


//! Callback When TimerOn
typedef void (*__TIMER_ON_CALLBACK__)(uint8_t _taskID, uint8_t _taskAction);
//! Callback When TimerOff
typedef void (*__TIMER_OFF_CALLBACK__)(uint8_t _taskID, uint8_t _taskAction);

//! TimerConfig
typedef struct{
   uint8_t __sun:1;
   uint8_t __mon:1;
   uint8_t __tue:1;
   uint8_t __wed:1;
   uint8_t __thu:1;
   uint8_t __fri:1;
   uint8_t __sat:1;
   uint8_t __u:1;
}DayMap_t;
typedef struct{
   uint8_t __enable:1;
   uint8_t __action:1;
   uint8_t __repeat:1;
   uint8_t __flag:1;
   uint8_t __status:1;
   uint8_t __count:3;
}TimeControl_t;
typedef struct{
    uint8_t __ID;
    uint8_t __StartHour;
    uint8_t __StartMinute;
    uint8_t __StopHour;
    uint8_t __StopMinute;
    DayMap_t __RunDay;
    TimeControl_t __Control;
}Timer_t;



class TimeTask{
    public:
    //! Constructor
        TimeTask();
    //! Specific to RTC Clock
        bool SyncNetworkTime(void);
    //! Task Observer   
        void Observer(Timer_t* _time, size_t _timecount);
    //! Parser
        void WeekDayParser(const char* _days,DayMap_t& _dayMap);
        uint8_t WeekDayCounter(DayMap_t& _dayMap);
    //! Destructor
        ~TimeTask();
        
    private:
        RtcDS1307<TwoWire> mRTCHandle{Wire};
                 
};


#endif
