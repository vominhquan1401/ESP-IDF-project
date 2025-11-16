#ifndef CONFIG_H
#define CONFIG_H


#ifdef __cplusplus
extern "C"
{
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <ctype.h>
#include <string.h>
#include "wifi_manager.h"
#include "nvs_manager.h"
#include "mqtt_module.h"
#include "esp_sntp.h"
#include <time.h>
#include "esp_log.h"
#include "stdio.h"
#include "esp_err.h"
#include "esp_event.h"
#include "wifi_config_portal.h"
#include "secrects.h"

typedef enum {
    WIFI_STATE_LOAD_CONFIG = 0,
    WIFI_STATE_CONNECT_WIFI,
    WIFI_STATE_TIME_SYN,
    WIFI_STATE_WEB_CONFIG,
    WIFI_STATE_CONTROL_WIFI_HANDLER,
    WIFI_STATE_WAITING_TO_BUTTON,
} WifiSetUpState_t;


typedef enum {
    WIFI_HANDLER_STATE_WATING = 0,
    WIFI_HANDLER_STATE_NEW_CONFIG,   // vừa nhận cấu hình mới từ MQTT
    WIFI_HANDLER_STATE_RECONNECT_OLD     // chạy chu kỳ reconnect WiFi cũ
} WifiHandlerState_t;



typedef enum {
    SENSOR_STATE_READ_DATA = 0,
    SENSOR_STATE_PREPROCESS,
    SENSOR_STATE_ENQUEUE
} sensor_state_t;


typedef enum {
    LED_STATUS_DISCONNECTED,
    LED_STATUS_CONNECTING,
    LED_STATUS_CONNECTED,
    LED_STATUS_WEBSERVER
} LED_status_t;


typedef enum {
    DM_STATE_INIT_MQTT = 0,
    DM_STATE_CHECK_MQTT,
    DM_STATE_GET_DATA,
    DM_STATE_PUBLISH,
} dm_state_t;


/*
========================= MQTT ===============================
*/
#define SERVER_TYPE  MQTT_SERVER_ADAFRUIT_IO
#define IO_USERNAME  USERNAME
#define IO_KEY       KEY
#define USE_SSL      false
//-------------------------------------------------------------------------//
//Publisher
#define TEMPERATURE_FEED_ID     "quangppm/feeds/yolofarm.farm-temperature"
#define HUMIDITY_FEED_ID        "quangppm/feeds/yolofarm.farm-humidity"
#define LIGHT_FEED_ID           "quangppm/feeds/yolofarm.farm-light-intensity"
//Subscriber
#define WIFI_SSID_ID             "quangppm/feeds/yolofarm.farm-wifi-ssid"
#define WIFI_PASSWORD_ID         "quangppm/feeds/yolofarm.farm-wifi-password"
#define SEND_INTERVAL_ID         "quangppm/feeds/yolofarm.farm-send-interval"



/*
========================= DATA CONTROL ===============================
*/
//data handle
#define RAW_WINDOW   10
#define DEFAULT_MS 30000         // thời gian đọc sensor mặc định
#define LOW_THRESHOLD_MS 15000
#define PACKET_WINDOW    1


/*
========================= SENSOR ===============================
*/

#define LED_GPIO GPIO_NUM_8



#ifdef __cplusplus
}
#endif

#endif