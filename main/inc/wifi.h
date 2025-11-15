#ifndef WIFI_H
#define WIFI_H


#ifdef __cplusplus
extern "C"
{
#endif

void vtaskWifiSetup(void *pvParameters);
void taskWifiHandler(void *pvParameters);
void taskLedControl(void *pvParameters);





#ifdef __cplusplus
}
#endif
#endif