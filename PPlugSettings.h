//! Dependency list
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "ArduinoJson.h"
#include "PubSubClient.h"
//! SerialDebug
#define SERIAL_SPEED 115200
//! Device Firmware Configuration
#define FIRMWARE_VER_MAJOR  1
#define FIRMWARE_VER_MINOR  0
#define FIMRWARE_VER_STRING "SmartPlug Hubless"
#define FIRMWARE_AUTHOR     "MUHAMMAD ALI BAIG"
//! MQTT
#define MQTT_TOPIC_PUBINF   "HUBLESS/DEVINFO"
#define MQTT_TOPIC_PUBACK   "HUBLESS/ACK"
#define MQTT_TOPIC_PUBSUM   "HUBLESS/SUM"
#define MQTT_TOPIC_SUBREQ   "HUBLESS/REQ"
#define MQTT_TOPIC_SIZE     30
#define MQTT_PAYLOAD_SIZE   200
#define MQTT_USERID_SUFFIX  "PPLUG"
#define MQTT_USERID_SIZE    50
#define MQTT_SUM_INTERVAL_SEC 15
#define MQTT_KEEPALIVE        MQTT_SUM_INTERVAL_SEC * 8
#define MQTT_SERVER         "thingsaccess.com"
#define MQTT_PORT           1883
#define TIMER_CHECK_INTERVAL_SEC 30
//! GPIO 
#define GPIO_SWITCH_PIN  12
#define GPIO_TOUCH_PIN   0
#define GPIO_BUZZER_PIN  2
#define GPIO_CF_PIN      14
#define GPIO_CF1_PIN     13
#define GPIO_SEL_PIN     16
#define GPIO_SCL_PIN     5
#define GPIO_SDA_PIN     4

typedef enum{SWITCH_ACK, TIMER_ACK, SCHEDULER_ACK}PayloadType_t;
typedef enum{NOT_DONE, DONE}ActionStatus_t;

//! Timer
#define TIMER_TIMEOUT_MS        10
#define TIMER_LONG_PRESS_SECS   10
#define TIMER_LONG_PRESS_COUNTS (TIMER_LONG_PRESS_SECS* 1000) / TIMER_TIMEOUT_MS
#define AP_TIMEOUT_MINTS        10
#define AP_TIMEOUT_MS           AP_TIMEOUT_MINTS * 60 * 60
