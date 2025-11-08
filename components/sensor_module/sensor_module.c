#include <stdlib.h>
#include <time.h>
#include "esp_log.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "http_client_module.h"

static const char *TAG = "Sensor_Module";

static float simulated_temperature = 25.0;
static float simulated_humidity = 60.0;

void update_simulated_sensors(void)
{
    float temp_var = ((float)rand() / RAND_MAX - 0.5) * 4.0;
    simulated_temperature = 25.0 + temp_var;
    float hum_var = ((float)rand() / RAND_MAX - 0.5) * 20.0;
    simulated_humidity = 60.0 + hum_var;
}

float get_temperature(void) { return simulated_temperature; }
float get_humidity(void) { return simulated_humidity; }

char *get_current_timestamp(void)
{
    static char buf[25];
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
    return buf;
}

void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP...");
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
}

void sensor_data_sender_task(void *pvParameters)
{
    srand(esp_random());
    while (1)
    {
        send_sensor_data_to_server();
        vTaskDelay(pdMS_TO_TICKS(30000)); // every 30s
    }
}
