#pragma once
#include "esp_err.h"
#include "esp_wifi_types.h"
#ifdef __cplusplus
extern "C"
{
#endif
    extern int wifi_retry_webserver_count;
    extern bool wifi_from_portal;
    void wifi_init(void);
    void wifi_scan(void);
    void wifi_connect_sta(void);
    void wifi_start_ap(void);
    void wifi_start_dual_mode(void);
    void wifi_manager_update_sta_creds(const char *ssid, const char *pass);
    bool wifi_try_connect_from_nvs(void);
#ifdef __cplusplus
}
#endif
