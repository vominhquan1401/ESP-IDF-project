#ifndef WIFI_H
#define WIFI_H


#ifdef __cplusplus
extern "C"
{
#endif
typedef struct {
    char ssid[64];
    char password[64];
} wifi_cmd_t;

void vtaskWifiSetup(void *pvParameters);
void taskWifiHandler(void *pvParameters);
void taskLedControl(void *pvParameters);
extern QueueHandle_t wifiCmdQueue;




#ifdef __cplusplus
}
#endif
#endif