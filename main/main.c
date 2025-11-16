#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <ctype.h>
#include <string.h>
#include "wifi_manager.h"
#include "nvs_manager.h"
#include "esp_log.h"
#include "stdio.h"
#include "esp_err.h"
#include "esp_event.h"
#include "wifi_config_portal.h"
#include "http_client_module.h"

static const char *TAG = "MAIN_APP";

char ssid[32] = "";
char password[64] = "";

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "         ESP32 WiFi + IoT Sensor        ");
    ESP_LOGI(TAG, "========================================");
    nvs_init();
    // Khởi tạo WiFi
    // wifi_init();
    // wifi_connect_sta();
    // int value = 1234;
    // nvs_write_key_value("interval", "ms_value", NVS_TYPE_I32, &value, sizeof(value));
    // int out_value;
    // size_t len = sizeof(out_value);
    // nvs_read_key_value("interval", "ms_value", NVS_TYPE_I32, &out_value, &len);
    // printf("out_value: %d\n", out_value);
    nvs_list_keys_in_namespace("interval");
    ESP_LOGI(TAG, "System initialized. Running tasks...");
    // wifi_nvs_get_all_saved_ap(ap_list, 5);
    // Tạo task gửi sensor data
    // xTaskCreate(sensor_task, "sensor_task", 8192, NULL, 5, NULL);
}
