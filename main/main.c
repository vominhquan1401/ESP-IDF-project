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
static const char *TAG = "MAIN_APP";

char ssid[32] = "";
char password[64] = "";

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "         ESP32 WiFi + IoT Sensor        ");
    ESP_LOGI(TAG, "========================================");

    // char ssid[33], pass[65];
    // saved_ap_t ap_list[MAX_AP_COUNT];
    // nvs_init();
    // nvs_list_all_namespaces();
    // nvs_list_keys_in_namespace("nvs.net80211");
    wifi_init();
    // if (wifi_try_connect_from_nvs())
    // {
    //     wifi_connect_sta();
    // }
    // vTaskDelay(pdTICKS_TO_MS(100));
    wifi_config_portal_start();
    // wifi_connect_sta();
    // int count = wifi_nvs_get_all_saved_ap(ap_list, MAX_AP_COUNT);
    // printf("Đọc được %d AP đã lưu.", count);

    // if (wifi_nvs_get_sta_credentials(ssid, pass) == ESP_OK)
    //     printf("Wi-Fi mặc định: SSID=%s, PASS=%s", ssid, pass);
    // 1️⃣ Ghi giá trị int32
    // nvs_delete_key("nvs.net80211", "sta.apinfo");
    // nvs_list_keys_in_namespace("nvs.net80211");
    ESP_LOGI(TAG, "System initialized. Running tasks...");
}
