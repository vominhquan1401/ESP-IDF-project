#include "wifi_config_portal.h"
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "wifi_manager.h" // dùng wifi_start_ap(), wifi_connect_sta(), wifi_manager_update_sta_creds()

static const char *TAG = "WiFi_Portal";
static httpd_handle_t s_server = NULL;
bool portal_done = false;
// ---------------- HTML giao diện ----------------
static const char *HTML =
    "<!doctype html><html><body>"
    "<h2>Config WiFi</h2>"
    "<form action=\"/setwifi\" method=\"post\">"
    "SSID: <input name=\"ssid\" type=\"text\"><br>"
    "Password: <input name=\"password\" type=\"password\"><br><br>"
    "<button type=\"submit\">OK</button>"
    "</form>"
    "</body></html>";

// ---------------- Handler GET ----------------
static esp_err_t root_get(httpd_req_t *req)
{
    httpd_resp_send(req, HTML, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// ---------------- Hàm decode URL ----------------
static void url_decode_inplace(char *s)
{
    char *r = s, *w = s;
    while (*r)
    {
        if (*r == '+')
        {
            *w++ = ' ';
            r++;
        }
        else if (*r == '%' && isxdigit((unsigned char)r[1]) && isxdigit((unsigned char)r[2]))
        {
            int hi = isdigit((unsigned char)r[1]) ? r[1] - '0' : 10 + (tolower(r[1]) - 'a');
            int lo = isdigit((unsigned char)r[2]) ? r[2] - '0' : 10 + (tolower(r[2]) - 'a');
            *w++ = (char)((hi << 4) | lo);
            r += 3;
        }
        else
        {
            *w++ = *r++;
        }
    }
    *w = '\0';
}

// ---------------- Task đổi mode Wi-Fi ----------------
static void wifi_switch_task(void *param)
{
    char *creds = (char *)param;
    char ssid[32] = {0}, pass[64] = {0};

    // tách dữ liệu từ chuỗi "ssid pass"
    sscanf(creds, "%31[^|]|%63[^\n]", ssid, pass);
    free(creds);
    printf("wifi_switch_task string ssid : %s and pass: %s \n", ssid, pass);
    ESP_LOGI(TAG, "🟡 Switching from AP → STA...");
    vTaskDelay(pdMS_TO_TICKS(300)); // để HTTP gửi xong response

    // dừng Wi-Fi AP mode
    esp_wifi_stop();
    vTaskDelay(pdMS_TO_TICKS(500));

    // cập nhật credentials và gọi connect
    wifi_manager_update_sta_creds(ssid, pass);
    wifi_retry_webserver_count = 0;
    wifi_from_portal = true;
    wifi_connect_sta();

    ESP_LOGI(TAG, "✅ Switched to STA, connecting...");
    portal_done = true;
    vTaskDelete(NULL);
}
static void stop_httpd_task(void *arg)
{
    vTaskDelay(pdMS_TO_TICKS(300)); // cho HTTP gửi xong dữ liệu
    wifi_config_portal_stop();
    vTaskDelete(NULL);
}

// ---------------- Handler POST /setwifi ----------------
static esp_err_t setwifi_post(httpd_req_t *req)
{
    int total = req->content_len;
    if (total <= 0 || total > 512)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad content");
        return ESP_FAIL;
    }

    char *buf = malloc(total + 1);
    if (!buf)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No mem");
        return ESP_FAIL;
    }

    int recvd = 0;
    while (recvd < total)
    {
        int r = httpd_req_recv(req, buf + recvd, total - recvd);
        if (r <= 0)
        {
            free(buf);
            return ESP_FAIL;
        }
        recvd += r;
    }
    buf[recvd] = '\0';

    url_decode_inplace(buf); // giờ buf dạng "ssid=...&password=..."
    char ssid[32] = {0}, pass[64] = {0};
    char *p_ssid = strstr(buf, "ssid=");
    char *p_pwd = strstr(buf, "password=");
    if (p_ssid && p_pwd)
    {
        int ssid_len = (int)(p_pwd - p_ssid - 6);
        if (ssid_len < 0)
            ssid_len = 0;
        if (ssid_len > (int)sizeof(ssid) - 1)
            ssid_len = sizeof(ssid) - 1;
        strncpy(ssid, p_ssid + 5, ssid_len);
        ssid[ssid_len] = '\0';
        strncpy(pass, p_pwd + 9, sizeof(pass) - 1);
    }

    ESP_LOGI(TAG, "🆕 New creds → SSID='%s' PASS='%s'", ssid, pass);

    // gửi phản hồi ngay (tránh chờ)
    httpd_resp_send(req, "<h3>Connecting to WiFi...</h3>", HTTPD_RESP_USE_STRLEN);

    // tắt webserver (AP vẫn giữ cho đến khi task dừng Wi-Fi)
    xTaskCreate(stop_httpd_task, "stop_httpd_task", 2048, NULL, 5, NULL);
    // truyền credentials vào task riêng
    char *creds = malloc(100);
    snprintf(creds, 100, "%s|%s", ssid, pass);
    xTaskCreate(wifi_switch_task, "wifi_switch_task", 4096, creds, 5, NULL);

    free(buf);
    return ESP_OK;
}
static esp_err_t favicon_get(httpd_req_t *r)
{
    httpd_resp_send_404(r);
    return ESP_OK;
}
// ---------------- Hàm khởi tạo Portal ----------------
esp_err_t wifi_config_portal_start(void)
{
    ESP_LOGI(TAG, "🌐 Start WiFi config portal");
    portal_done = false;
    // Dừng Wi-Fi hiện tại (nếu có) rồi bật AP
    esp_wifi_stop();
    wifi_start_ap(); // dùng hàm trong wifi_manager

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    if (httpd_start(&s_server, &cfg) != ESP_OK)
    {
        ESP_LOGE(TAG, "❌ httpd_start failed");
        return ESP_FAIL;
    }

    httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = root_get};
    httpd_uri_t set = {.uri = "/setwifi", .method = HTTP_POST, .handler = setwifi_post};
    httpd_register_uri_handler(s_server, &root);
    httpd_register_uri_handler(s_server, &set);

    // handler 404 favicon
    httpd_uri_t fav = {.uri = "/favicon.ico", .method = HTTP_GET, .handler = favicon_get};
    httpd_register_uri_handler(s_server, &fav);

    return ESP_OK;
}

// ---------------- Dừng webserver ----------------
void wifi_config_portal_stop(void)
{
    if (s_server)
    {
        httpd_stop(s_server);
        s_server = NULL;
    }
}
