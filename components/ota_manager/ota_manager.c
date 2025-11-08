#include "ota_manager.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "OTA_Manager";

static void ota_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base != ESP_HTTPS_OTA_EVENT)
        return;

    switch (id)
    {
    case ESP_HTTPS_OTA_START:
        ESP_LOGI(TAG, "OTA started");
        break;
    case ESP_HTTPS_OTA_CONNECTED:
        ESP_LOGI(TAG, "Connected to server");
        break;
    case ESP_HTTPS_OTA_GET_IMG_DESC:
        ESP_LOGI(TAG, "Reading image description");
        break;
    case ESP_HTTPS_OTA_WRITE_FLASH:
        ESP_LOGD(TAG, "Writing to flash...");
        break;
    case ESP_HTTPS_OTA_UPDATE_BOOT_PARTITION:
        ESP_LOGI(TAG, "Boot partition updated");
        break;
    case ESP_HTTPS_OTA_FINISH:
        ESP_LOGI(TAG, "OTA finished");
        break;
    case ESP_HTTPS_OTA_ABORT:
        ESP_LOGW(TAG, "OTA aborted");
        break;
    }
}

esp_err_t ota_manager_run(const char *url)
{
    esp_http_client_config_t http_cfg = {
        .url = url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 15000,
    };

    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
    };

    ESP_LOGI(TAG, "Starting OTA from %s", url);
    esp_err_t ret = esp_https_ota(&ota_cfg);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "OTA successful, rebooting...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }
    else
    {
        ESP_LOGE(TAG, "OTA failed: %s", esp_err_to_name(ret));
    }
    return ret;
}

void ota_manager_task(void *arg)
{
    const char *url = (const char *)arg;
    ESP_ERROR_CHECK(esp_event_handler_register(ESP_HTTPS_OTA_EVENT, ESP_EVENT_ANY_ID, &ota_event_handler, NULL));
    // vTaskDelay(pdTICKS_TO_MS(100));
    ota_manager_run(url);
    vTaskDelete(NULL);
}
