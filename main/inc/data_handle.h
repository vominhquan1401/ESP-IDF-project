#ifndef DATA_HANDLE_H
#define DATA_HANDLE_H


#ifdef __cplusplus
extern "C"
{
#endif
#include "config.h"
/**
 * Define 
 */
#define LOG_LOCAL_LEVEL ESP_LOG_INFO

/**
 * Declare
 */
typedef struct {
    float temp[PACKET_WINDOW];
    float hum[PACKET_WINDOW];
    float lux[PACKET_WINDOW];
    uint64_t ts[PACKET_WINDOW]; 
} sensor_packet_t;

extern QueueHandle_t sensorQueue;


void taskSensorRead(void *pvParameters);
void taskDataManager(void *pvParameters);




#ifdef __cplusplus
}
#endif
#endif
