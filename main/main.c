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

// Task để gửi sensor data định kỳ
void sensor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Sensor task started");
    
    // Đợi WiFi kết nối (tối đa 30 giây)
    int wait_count = 0;
    while (wait_count < 30) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        wait_count++;
        // Giả sử WiFi đã kết nối sau 10 giây (hoặc check status thực tế)
        if (wait_count >= 10) break;
    }
    
    ESP_LOGI(TAG, "Starting to send sensor data...");
    
    while (1) {
        // Gửi data lên server
        esp_err_t err = send_sensor_data_to_server();
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "✅ Data sent successfully");
        } else {
            ESP_LOGE(TAG, "❌ Failed to send data");
        }
        
        // Đợi interval (interval có thể thay đổi từ server)
        int interval_ms = get_send_interval_ms();
        ESP_LOGI(TAG, "⏳ Waiting %d ms before next send...", interval_ms);
        vTaskDelay(pdMS_TO_TICKS(interval_ms));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "         ESP32 WiFi + IoT Sensor        ");
    ESP_LOGI(TAG, "========================================");

    // Khởi tạo WiFi
    wifi_init();
    wifi_connect_sta();
    
    ESP_LOGI(TAG, "System initialized. Running tasks...");
    
    // Tạo task gửi sensor data
    xTaskCreate(sensor_task, "sensor_task", 8192, NULL, 5, NULL);
}
