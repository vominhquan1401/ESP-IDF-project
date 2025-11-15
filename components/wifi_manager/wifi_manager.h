#pragma once
#include "esp_err.h"
#include "esp_wifi_types.h"
#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum {
        WIFI_STATUS_IDLE = 0,
        WIFI_STATUS_CONNECTING,
        WIFI_STATUS_CONNECTED,
        WIFI_STATUS_DISCONNECTED
    } wifi_status_t;

    extern wifi_status_t wifi_status;
    
    extern int wifi_retry_webserver_count;
    extern bool wifi_from_portal;
    void wifi_init(void);
    wifi_ap_record_t *wifi_scan(uint16_t *found_ap_num);
    void wifi_connect_sta(void);
    void wifi_start_ap(void);
    void wifi_start_dual_mode(void);
    void wifi_manager_update_sta_creds(const char *ssid, const char *pass);
    bool wifi_try_connect_from_nvs(void);
#ifdef __cplusplus
}
#endif
