#include <string.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "nvs_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "http_client_module.h"
#include "wifi_manager.h"
#include "wifi_config_portal.h"
static const char *TAG = "WiFi_Manager";

#define WIFI_SSID "mewmew"
#define WIFI_PASSWORD "vominhquan140"
#define MAX_RETRY 5

#define AP_SSID "ESP32_WiFi_Lab"
#define AP_PASSWORD "12345678"
#define AP_CHANNEL 1
#define AP_MAX_CONN 4

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

int wifi_retry_webserver_count = 0;
bool check_wifi_after_fail = false;
bool wifi_from_portal = false;

wifi_config_t wifi_config = {
    .sta = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASSWORD,
        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
    },
};

void wifi_init(void)
{
    ESP_LOGI(TAG, "Initializing WiFi...");
    nvs_init();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
}

void wifi_scan(void)
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 100,
        .scan_time.active.max = 300,
    };

    ESP_LOGI(TAG, "Starting WiFi scan...");
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));

    uint16_t ap_count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI(TAG, "Number of access points found: %d", ap_count);

    wifi_ap_record_t *ap_info = malloc(sizeof(wifi_ap_record_t) * ap_count);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_info));

    for (int i = 0; i < ap_count; i++)
    {
        ESP_LOGI(TAG, "[%2d] SSID: %-32s RSSI: %4d Channel: %2d",
                 i + 1, (char *)ap_info[i].ssid, ap_info[i].rssi, ap_info[i].primary);
    }

    free(ap_info);
    ESP_LOGI(TAG, "WiFi scan complete.");
}

bool wifi_try_connect_from_nvs(void)
{
    char ssid[33] = {0}, pass[65] = {0};
    if (wifi_nvs_get_sta_credentials(ssid, pass) == ESP_OK && ssid[0])
    {
        wifi_manager_update_sta_creds(ssid, pass); // ghi vào wifi_config
        ESP_LOGI(TAG, "Load creds from NVS → SSID='%s'", ssid);
        wifi_connect_sta();
        return true;
    }
    return false;
}

void wifi_connect_sta(void)
{
    ESP_LOGI(TAG, "Connecting to STA...");
    ESP_LOGI(TAG, "     SSID: %s", (char *)wifi_config.sta.ssid);
    ESP_LOGI(TAG, "     PASS: %s", (char *)wifi_config.sta.password);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_stop());
    vTaskDelay(pdMS_TO_TICKS(200));
    ESP_ERROR_CHECK(esp_wifi_start());

    
}

void wifi_start_ap(void)
{
    wifi_config_t ap_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .channel = AP_CHANNEL,
            .password = AP_PASSWORD,
            .max_connection = AP_MAX_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK},
    };
    if (strlen(AP_PASSWORD) == 0)
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_start_dual_mode(void)
{
    ESP_LOGI(TAG, "Starting dual mode...");
    wifi_config_t sta_config = {
        .sta = {.ssid = WIFI_SSID, .password = WIFI_PASSWORD, .threshold.authmode = WIFI_AUTH_WPA2_PSK},
    };
    wifi_config_t ap_config = {
        .ap = {.ssid = AP_SSID, .ssid_len = strlen(AP_SSID), .channel = AP_CHANNEL, .password = AP_PASSWORD, .max_connection = AP_MAX_CONN, .authmode = WIFI_AUTH_WPA2_PSK},
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_manager_update_sta_creds(const char *ssid, const char *pass)
{
    memset(&wifi_config, 0, sizeof(wifi_config));
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    ESP_LOGI("WiFi_Manager", "Updated creds: SSID='%s'", ssid);
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "wifi event handler -.-.-.-.-.-.");
    // ESP_LOGI(TAG, "     SSID: %s", (char *)wifi_config.sta.ssid);
    // ESP_LOGI(TAG, "     PASS: %s", (char *)wifi_config.sta.password);
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGW("WIFI", "Mất kết nối WiFi, reason=%d", event->reason);

        switch (event->reason)
        {
        case WIFI_REASON_AUTH_FAIL:
            ESP_LOGE("WIFI", "❌ Sai password Wi-Fi");
            break;
        case WIFI_REASON_NO_AP_FOUND:
            ESP_LOGE("WIFI", "❌ Không tìm thấy SSID");
            break;
        case WIFI_REASON_ASSOC_LEAVE:
        case WIFI_REASON_BEACON_TIMEOUT:
            ESP_LOGE("WIFI", "❌ Mất kết nối do timeout hoặc AP không phản hồi");
            break;
        default:
            ESP_LOGE("WIFI", "❌ Lỗi khác (reason=%d)", event->reason);
            break;
        }

        if (++wifi_retry_webserver_count < 5)
        {
            ESP_LOGI(TAG, "Retry %d/5...", wifi_retry_webserver_count);
            esp_wifi_connect();
        }
        else
        {
            if (wifi_from_portal)
            {
                ESP_LOGE(TAG, "❌ Kết nối thất bại sau 5 lần. Quay lại AP mode.");
                wifi_retry_webserver_count = 0;
                wifi_config_portal_start();
            }
            else
            {
                char ssid[33];
                char pass[65];
                wifi_nvs_get_sta_credentials(ssid, pass);
                wifi_manager_update_sta_creds(ssid, pass);

                wifi_connect_sta();
                ESP_LOGI(TAG, "wifi event handler end");
            }
        }
    }
    ESP_LOGI(TAG, "wifi event handler -.-.-.-.-.-. end");
}

static void ip_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "✅ Nhận được IP: " IPSTR, IP2STR(&event->ip_info.ip));

        // Nếu đang trong quá trình config qua portal → chỉ bây giờ mới lưu NVS
        wifi_retry_webserver_count = 0;
        ESP_LOGI(TAG, "💾 Lưu thông tin Wi-Fi vào NVS");
        esp_wifi_set_storage(WIFI_STORAGE_FLASH);
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    }
}
