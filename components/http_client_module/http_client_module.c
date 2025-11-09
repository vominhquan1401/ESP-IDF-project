#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_wifi.h"
#include "wifi_manager.h"
#include "nvs_manager.h"
#include <cJSON.h>

// Local IP (only works on same network)
#define SERVER_URL "http://10.180.90.126:3000/api/sensor/data"
// Ngrok public URL - accessible from any network!
// #define SERVER_URL "https://31abe43dce60.ngrok-free.app/api/sensor/data"
#define DEVICE_ID "ESP32_Lab6_001"

// Global send interval (có thể thay đổi từ server)
static int send_interval_ms = 5000;

static const char *TAG = "HTTP_Client";

// Dummy sensor data (since sensor_module was removed)
static float dummy_temp = 25.5;
static float dummy_humidity = 65.0;

// Function để xử lý commands từ server
void process_server_commands(const char *response_data)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "📥 Processing server response...");
    ESP_LOGI(TAG, "Raw response: %s", response_data);
    
    cJSON *root = cJSON_Parse(response_data);
    if (!root) {
        ESP_LOGE(TAG, "❌ Failed to parse server response");
        return;
    }

    // Check for pending commands
    cJSON *commands = cJSON_GetObjectItem(root, "pendingCommands");
    if (cJSON_IsArray(commands)) {
        int command_count = cJSON_GetArraySize(commands);
        ESP_LOGI(TAG, "📋 Received %d pending command(s) from server", command_count);

        for (int i = 0; i < command_count; i++) {
            cJSON *command = cJSON_GetArrayItem(commands, i);
            if (cJSON_IsObject(command)) {
                cJSON *type = cJSON_GetObjectItem(command, "type");
                if (cJSON_IsString(type)) {
                    ESP_LOGI(TAG, "🔧 Command type: '%s'", type->valuestring);
                    
                    if (strcmp(type->valuestring, "update_config") == 0) {
                        ESP_LOGI(TAG, "⚙️  Processing UPDATE_CONFIG command...");
                        
                        // Xử lý lệnh cập nhật cấu hình
                        cJSON *params = cJSON_GetObjectItem(command, "params");
                        if (cJSON_IsObject(params)) {
                            
                            // 1. Cập nhật send_interval nếu có
                            cJSON *interval = cJSON_GetObjectItem(params, "send_interval");
                            if (cJSON_IsNumber(interval)) {
                                int new_interval = interval->valueint;
                                ESP_LOGI(TAG, "📊 Received send_interval: %d ms (current: %d ms)", new_interval, send_interval_ms);
                                if (new_interval >= 1000 && new_interval <= 60000) { // 1s - 60s
                                    send_interval_ms = new_interval;
                                    ESP_LOGI(TAG, "✅ Updated send interval to %d ms", send_interval_ms);
                                } else {
                                    ESP_LOGW(TAG, "⚠️ Invalid send interval: %d ms (must be 1000-60000)", new_interval);
                                }
                            } else {
                                ESP_LOGI(TAG, "ℹ️  No send_interval in params");
                            }

                            // 2. Cập nhật WiFi config nếu có
                            cJSON *wifi_ssid = cJSON_GetObjectItem(params, "wifi_ssid");
                            cJSON *wifi_password = cJSON_GetObjectItem(params, "wifi_password");
                            
                            if (cJSON_IsString(wifi_ssid) && cJSON_IsString(wifi_password)) {
                                const char *ssid = wifi_ssid->valuestring;
                                const char *pass = wifi_password->valuestring;
                                
                                ESP_LOGI(TAG, "📶 Received WiFi config:");
                                ESP_LOGI(TAG, "   SSID: '%s' (length: %d)", ssid, strlen(ssid));
                                ESP_LOGI(TAG, "   Password: '%s' (length: %d)", pass, strlen(pass));
                                
                                if (strlen(ssid) > 0 && strlen(pass) >= 8) {
                                    ESP_LOGI(TAG, "✅ WiFi credentials valid!");
                                    ESP_LOGI(TAG, "📶 Starting WiFi update process...");
                                    
                                    // Bước 1: Lưu vào NVS (persistent storage)
                                    ESP_LOGI(TAG, "💾 Step 1: Saving to NVS...");
                                    esp_err_t nvs_result = nvs_write_key_value("wifi", "wifi_ssid", NVS_TYPE_STR, ssid, strlen(ssid) + 1);
                                    if (nvs_result == ESP_OK) {
                                        ESP_LOGI(TAG, "   ✓ SSID saved");
                                        nvs_result = nvs_write_key_value("wifi", "wifi_pass", NVS_TYPE_STR, pass, strlen(pass) + 1);
                                        if (nvs_result == ESP_OK) {
                                            ESP_LOGI(TAG, "   ✓ Password saved");
                                        }
                                    }
                                    
                                    if (nvs_result == ESP_OK) {
                                        ESP_LOGI(TAG, "✅ WiFi credentials saved to NVS successfully!");
                                        
                                        // Bước 2: Update in-memory config
                                        ESP_LOGI(TAG, "💾 Step 2: Updating in-memory config...");
                                        wifi_manager_update_sta_creds(ssid, pass);
                                        wifi_connect_sta();
                                    } else {
                                        ESP_LOGE(TAG, "❌ Failed to save WiFi credentials to NVS (error: %s)", esp_err_to_name(nvs_result));
                                    }
                                } else {
                                    ESP_LOGW(TAG, "⚠️ Invalid WiFi credentials!");
                                    ESP_LOGW(TAG, "   Reason: SSID length=%d, Password length=%d", strlen(ssid), strlen(pass));
                                    ESP_LOGW(TAG, "   Required: SSID > 0, Password >= 8 chars");
                                }
                            } else {
                                ESP_LOGI(TAG, "ℹ️  No WiFi credentials in params");
                            }
                        }
                    }
                }
            }
        }
    } else {
        ESP_LOGI(TAG, "ℹ️  No pending commands from server");
    }

    cJSON_Delete(root);
    ESP_LOGI(TAG, "========================================");
}

// Function để lấy send interval hiện tại
int get_send_interval_ms(void)
{
    return send_interval_ms;
}

esp_err_t send_sensor_data_to_server(void)
{
    // Update dummy sensor data (simulate changing values)
    dummy_temp += (float)(rand() % 20 - 10) / 10.0; // ±1.0°C
    dummy_humidity += (float)(rand() % 10 - 5) / 10.0; // ±0.5%
    
    // Keep in reasonable ranges
    if (dummy_temp < 20.0) dummy_temp = 20.0;
    if (dummy_temp > 30.0) dummy_temp = 30.0;
    if (dummy_humidity < 50.0) dummy_humidity = 50.0;
    if (dummy_humidity > 80.0) dummy_humidity = 80.0;

    cJSON *root = cJSON_CreateObject();
    cJSON *sensors = cJSON_CreateArray();

    cJSON *t = cJSON_CreateObject();
    cJSON_AddStringToObject(t, "type", "temperature");
    cJSON_AddNumberToObject(t, "value", dummy_temp);
    cJSON_AddStringToObject(t, "unit", "°C");
    cJSON_AddStringToObject(t, "timestamp", "2025-01-08T12:00:00Z");
    cJSON_AddItemToArray(sensors, t);

    cJSON *h = cJSON_CreateObject();
    cJSON_AddStringToObject(h, "type", "humidity");
    cJSON_AddNumberToObject(h, "value", dummy_humidity);
    cJSON_AddStringToObject(h, "unit", "%");
    cJSON_AddStringToObject(h, "timestamp", "2025-01-08T12:00:00Z");
    cJSON_AddItemToArray(sensors, h);

    cJSON_AddStringToObject(root, "deviceId", DEVICE_ID);
    cJSON_AddItemToObject(root, "sensors", sensors);

    char *json_payload = cJSON_Print(root);
    
    // Prepare response buffer
    char response_buffer[2048];
    memset(response_buffer, 0, sizeof(response_buffer));
    
    esp_http_client_config_t config = {
        .url = SERVER_URL, 
        .method = HTTP_METHOD_POST, 
        .timeout_ms = 10000,
        .buffer_size = sizeof(response_buffer),
        .buffer_size_tx = 1024
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_payload, strlen(json_payload));

    // Open connection
    esp_err_t err = esp_http_client_open(client, strlen(json_payload));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        goto cleanup;
    }

    // Write POST data
    int wlen = esp_http_client_write(client, json_payload, strlen(json_payload));
    if (wlen < 0) {
        ESP_LOGE(TAG, "Failed to write POST data");
        err = ESP_FAIL;
        goto cleanup;
    }

    // Fetch headers
    int content_length = esp_http_client_fetch_headers(client);
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "POST OK, status %d, content_length %d", status_code, content_length);

    // Read response
    if (status_code == 200 && content_length > 0) {
        int read_len = esp_http_client_read(client, response_buffer, sizeof(response_buffer) - 1);
        if (read_len > 0) {
            response_buffer[read_len] = '\0';
            ESP_LOGI(TAG, "Server response: %s", response_buffer);
            process_server_commands(response_buffer);
        } else {
            ESP_LOGW(TAG, "No response data read");
        }
    } else if (status_code == 200) {
        ESP_LOGI(TAG, "No pending commands (empty response)");
    }

cleanup:

    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    free(json_payload);
    return err;
}
