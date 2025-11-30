#include "config.h"
#include "wifi.h"
#include "data_handle.h"
#include "mqtt_client.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

/**
 * Kiểm tra timestamp
 * thay vào bên WIFI
 * thay vào đọc sensor
 *
 */

static const char *TAG = "MAIN_APP";

char ssid[32] = "";
char password[64] = "";

/*
    Task:
    - Init: get info from nvs, initial everything for program
    - taskWifiSetup: connect wifi (first time) and webserver, create Wifi_handler Task
    - taskLedControl: LED demostrates esp32's status
    - taskSensorRead
    - taskDataManager
    - taskConfigManager
    - taskWifiHandler
*/
/*-----------------------------------------------------------
    Task publish MQTT mỗi 5s
-----------------------------------------------------------*/

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "         ESP32 WiFi + IoT Sensor        ");
    ESP_LOGI(TAG, "========================================");

    sensorQueue = xQueueCreate(50, sizeof(sensor_packet_t));
    // Task handle:
    TaskHandle_t wifiSetUpHandle;
    TaskHandle_t ledControlHandle;
    TaskHandle_t sensorReadHandle;
    TaskHandle_t dataManagerhandle;
    /* INIT */
    wifi_init();

    /* Create Tasks */
    xTaskCreate(vtaskWifiSetup, "WifiSetUp", 8192, NULL, 3, &wifiSetUpHandle);
    xTaskCreate(taskLedControl, "LedControl", 8192, NULL, 1, &ledControlHandle);

    xTaskCreate(taskSensorRead, "taskSensorRead", 4096, NULL, 2, &sensorReadHandle);
    xTaskCreate(taskDataManager, "taskDataManager", 4096, NULL, 2, &dataManagerhandle);
}




