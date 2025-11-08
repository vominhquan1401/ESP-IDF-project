#include <string.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include <cJSON.h>
#include "sensor_module.h"

#define SERVER_URL "http://192.168.143.207:3000/api/sensor/data"
#define DEVICE_ID "ESP32_Lab6_001"

static const char *TAG = "HTTP_Client";

esp_err_t send_sensor_data_to_server(void)
{
    update_simulated_sensors();

    cJSON *root = cJSON_CreateObject();
    cJSON *sensors = cJSON_CreateArray();

    cJSON *t = cJSON_CreateObject();
    cJSON_AddStringToObject(t, "type", "temperature");
    cJSON_AddNumberToObject(t, "value", get_temperature());
    cJSON_AddStringToObject(t, "unit", "°C");
    cJSON_AddStringToObject(t, "timestamp", get_current_timestamp());
    cJSON_AddItemToArray(sensors, t);

    cJSON *h = cJSON_CreateObject();
    cJSON_AddStringToObject(h, "type", "humidity");
    cJSON_AddNumberToObject(h, "value", get_humidity());
    cJSON_AddStringToObject(h, "unit", "%");
    cJSON_AddStringToObject(h, "timestamp", get_current_timestamp());
    cJSON_AddItemToArray(sensors, h);

    cJSON_AddStringToObject(root, "deviceId", DEVICE_ID);
    cJSON_AddItemToObject(root, "sensors", sensors);

    char *json_payload = cJSON_Print(root);
    esp_http_client_config_t config = {.url = SERVER_URL, .method = HTTP_METHOD_POST, .timeout_ms = 10000};
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_payload, strlen(json_payload));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
        ESP_LOGI(TAG, "POST OK, status %d", esp_http_client_get_status_code(client));
    else
        ESP_LOGE(TAG, "HTTP failed: %s", esp_err_to_name(err));

    esp_http_client_cleanup(client);
    cJSON_Delete(root);
    free(json_payload);
    return err;
}
