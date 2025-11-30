// #include "dht20.h"
// #include "light_sensor.h"
#include "data_handle.h"
#include "wifi.h"
/**
 * Global Variables
 */
QueueHandle_t sensorQueue;

static const char *TAG = "SensorTask";
volatile uint32_t sample_period_ms = DEFAULT_MS;


static char new_ssid[64];
static char new_pass[64];
static bool ssid_received = false;
static bool pass_received = false;
/**
 * INTERNAL FUNCTIONs
 */
// Sensor Read
float preprocess_value(float *arr, int n)
{
    float buf[RAW_WINDOW];
    memcpy(buf, arr, n * sizeof(float));

    // sort tăng dần (bubble sort cho đơn giản)
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (buf[i] > buf[j]) {
                float tmp = buf[i];
                buf[i] = buf[j];
                buf[j] = tmp;
            }
        }
    }

    // bỏ min (buf[0]) và max (buf[n-1])
    float sum = 0.0f;
    for (int i = 1; i < n - 1; i++) {
        sum += buf[i];
    }

    return sum / (float)(n - 2);
}


// Data Manager


/**
 * TASKs
 */


void taskSensorRead(void *pvParameters) 
{
    /* Inits */
    i2c_master_init();
    dht20_init();
    /* Sensor state */
    sensor_state_t state = SENSOR_STATE_READ_DATA;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    // ==== RAW BUFFERS ====
    static float temp_raw[RAW_WINDOW];
    static float hum_raw[RAW_WINDOW];
    static float lux_raw[RAW_WINDOW];
    static int raw_index = 0;
    // ==== PROCESSED BUFFERS ====
    static float temp_processed[PACKET_WINDOW];
    static float hum_processed[PACKET_WINDOW];
    static float lux_processed[PACKET_WINDOW];
    static uint64_t ts_processed[PACKET_WINDOW];
    static int proc_index = 0;

    ESP_LOGI(TAG, "read ms: %lu ms", (sample_period_ms / RAW_WINDOW));

    while(1)
    {
        switch(state)
        {
            case SENSOR_STATE_READ_DATA:
            {
                /* waiting */
                vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(sample_period_ms / RAW_WINDOW));

                float t, h, lux;
                /* Read DHT20 sensor */
                if (dht20_read(&t, &h) == ESP_OK) {
                    temp_raw[raw_index] = t;
                    hum_raw[raw_index] = h;
                }
                /* Read Light sensor */
                if (light_sensor_read(&lux) == ESP_OK) {
                    lux_raw[raw_index] = lux;
                }

                // ESP_LOGW(TAG, "sensor values: T(%.2f) H(%.2f) L(%.2f)",
                //                     temp_raw[raw_index], hum_raw[raw_index], lux_raw[raw_index]);

                /* increase index */
                raw_index++;

                /* switch state if satify condition */
                if (raw_index >= RAW_WINDOW) {
                    state = SENSOR_STATE_PREPROCESS;
                }
                break;
            }

            case SENSOR_STATE_PREPROCESS:
            {
                float t_clean  = preprocess_value(temp_raw, RAW_WINDOW);
                float h_clean  = preprocess_value(hum_raw,  RAW_WINDOW);
                float lux_clean = preprocess_value(lux_raw, RAW_WINDOW);

                // ====== ADD TIMESTAMP HERE (MOCK) ======
                time_t ts_now;
                time(&ts_now);

                temp_processed[proc_index] = t_clean;
                hum_processed[proc_index]  = h_clean;
                lux_processed[proc_index]  = lux_clean;
                ts_processed[proc_index]   = ts_now;

                proc_index++;
                raw_index = 0; // reset raw buffer index
                ESP_LOGW(TAG, "data preprocess: %d times", proc_index);

                if (proc_index >= PACKET_WINDOW) {
                    state = SENSOR_STATE_ENQUEUE;
                } else {
                    state = SENSOR_STATE_READ_DATA;
                }

                break;
            }
            case SENSOR_STATE_ENQUEUE:
            {
                sensor_packet_t pkt;
                memcpy(pkt.temp, temp_processed, sizeof(float)*PACKET_WINDOW);
                memcpy(pkt.hum,  hum_processed,  sizeof(float)*PACKET_WINDOW);
                memcpy(pkt.lux,  lux_processed,  sizeof(float)*PACKET_WINDOW);
                memcpy(pkt.ts,   ts_processed,   sizeof(uint64_t)*PACKET_WINDOW);

                xQueueSend(sensorQueue, &pkt, (pdMS_TO_TICKS(sample_period_ms) / 2));

                proc_index = 0;
                state = SENSOR_STATE_READ_DATA;
                break;
            }
        }


    }
}


bool publish_data(sensor_packet_t *pkt)
{
    bool success = true;
    char buf[64];

    uint64_t ts = pkt->ts[0];
    
    //
    // ===========================
    // Temperature
    // ===========================
    snprintf(buf, sizeof(buf),
                "{\"value\": %.2f, \"created_at\": %llu}",
                pkt->temp[0],
                ts);

    if (mqtt_client_publish(TEMPERATURE_FEED_ID "/json", buf, strlen(buf), 1, 0) <= -1)
    {
        success = false;
        ESP_LOGI(TAG, " Temperature packet Failed ");
    }

    //
    // ===========================
    // Humidity
    // ===========================
    snprintf(buf, sizeof(buf),
                "{\"value\": %.2f, \"created_at\": %llu}",
                pkt->hum[0],
                ts);

    if (mqtt_client_publish(HUMIDITY_FEED_ID "/json", buf, strlen(buf), 1, 0) <= -1)
    {
        success = false;
        ESP_LOGI(TAG, " Humidity packet Failed ");
    }

    //
    // ===========================
    // Light
    // ===========================
    snprintf(buf, sizeof(buf),
                "{\"value\": %.2f, \"created_at\": %llu}",
                pkt->lux[0],
                ts);

    if (mqtt_client_publish(LIGHT_FEED_ID "/json", buf, strlen(buf), 1, 0) <= -1)
    {
        success = false;
        ESP_LOGI(TAG, " Light packet Failed ");
    }

    ESP_LOGI(TAG, " sent packets ");

    return success;
}
// Handler:
static void mqtt_rx_handler(const char *topic, const char *data, int len)
{
    // printf("MQTT RX | topic: %s | data: %.*s\n", topic, len, data);

    // ----------------------------
    // 1) Nhận SSID
    // ----------------------------
    if (strstr(topic, WIFI_SSID_ID) == topic) {
        snprintf(new_ssid, sizeof(new_ssid), "%.*s", len, data);
        ssid_received = true;

        printf("New SSID received: %s\n", new_ssid);
    }

    // ----------------------------
    // 2) Nhận PASSWORD
    // ----------------------------
    else if (strstr(topic, WIFI_PASSWORD_ID) == topic) {
        snprintf(new_pass, sizeof(new_pass), "%.*s", len, data);
        pass_received = true;

        printf("New PASSWORD received: %s\n", new_pass);
    }

    // ----------------------------
    // 3) Khi đã nhận đủ SSID + PASS → chuyển đến taskWifiHandler
    // ----------------------------
    if (ssid_received && pass_received) {
        wifi_cmd_t cmd;
        snprintf(cmd.ssid, sizeof(cmd.ssid), "%s", new_ssid);
        snprintf(cmd.password, sizeof(cmd.password), "%s", new_pass);

        xQueueSend(wifiCmdQueue, &cmd, 0);

        printf("WIFI CONFIG READY -  sent to wifi handler\n");

        ssid_received = false;
        pass_received = false;
    }

    // ----------------------------
    // 4) Nhận interval → cập nhật sample_period_ms
    // ----------------------------
    else if (strstr(topic, SEND_INTERVAL_ID) == topic) {
        int new_interval = atoi(data);

        // không cho phép quá nhỏ
        if (new_interval < LOW_THRESHOLD_MS)
            new_interval = LOW_THRESHOLD_MS;

        sample_period_ms = new_interval;

        printf("Updated sample_period_ms = %ld ms\n", sample_period_ms);
    }
}


void taskDataManager(void *pvParameters) 
{
    // init mqtt:
    mqtt_client_cfg_t cfg = {
        .server_type = SERVER_TYPE,
        .username    = IO_USERNAME,
        .password    = IO_KEY,
        .use_ssl     = USE_SSL,
    };
    mqtt_client_init(&cfg);
    mqtt_client_set_callback(mqtt_rx_handler);

    /* VARIABLEs */
    dm_state_t state = DM_STATE_INIT_MQTT;
    sensor_packet_t pkt;

    while (1)
    {
        switch (state)
        {
            // =====================================================
            // 0) Connect to MQTT
            // =====================================================
            case DM_STATE_INIT_MQTT:
            {
                while(wifi_status != WIFI_STATUS_CONNECTED)
                {
                    vTaskDelay(pdMS_TO_TICKS(500));
                }
                // subscribe:
                mqtt_client_subscribe(WIFI_SSID_ID,      1);
                mqtt_client_subscribe(WIFI_PASSWORD_ID,  1);
                mqtt_client_subscribe(SEND_INTERVAL_ID,  1);

                // start server
                mqtt_client_start();  

                //switch state
                ESP_LOGI("MQTT", "INIT");
                while(!mqtt_client_is_connected())
                {
                    vTaskDelay(pdMS_TO_TICKS(300));
                }
                state = DM_STATE_CHECK_MQTT;
                break;

            }
            // =====================================================
            // 1) Check MQTT còn kết nối không
            // =====================================================
            case DM_STATE_CHECK_MQTT:
                if (mqtt_client_is_connected()) {
                    state = DM_STATE_GET_DATA;
                } else {
                    vTaskDelay(pdMS_TO_TICKS(500));
                }
                break;

            // =====================================================
            // 2) Lấy dữ liệu từ queue (không chặn)
            // =====================================================
            case DM_STATE_GET_DATA:
                if (xQueuePeek(sensorQueue, &pkt, 0)) {
                    UBaseType_t count = uxQueueMessagesWaiting(sensorQueue);
                    ESP_LOGI("QUEUE", "Current queue size = %d", count);

                    state = DM_STATE_PUBLISH;
                } else {
                    // Không có data → quay lại check MQTT
                    state = DM_STATE_CHECK_MQTT;
                    vTaskDelay(pdMS_TO_TICKS(500));
                }
                break;

            // =====================================================
            // 3) Publish
            //    - Success → xQueueReceive để xóa item
            //    - Fail → giữ nguyên, lát nữa retry lại
            // =====================================================
            case DM_STATE_PUBLISH:
                ESP_LOGI(TAG, "Send packet: T(%.2f) H(%.2f) L(%.2f), timestamp(%lld)", pkt.temp[0], pkt.hum[0], pkt.lux[0], pkt.ts[0]);
                if (publish_data(&pkt)) {
                    // Thành công → xóa khỏi queue
                    xQueueReceive(sensorQueue, &pkt, 0);
                }
                // Thành công hoặc thất bại đều quay về CHECK_MQTT
                vTaskDelay(pdMS_TO_TICKS(DEFAULT_MS / 3));
                state = DM_STATE_CHECK_MQTT;
                break;
        }
    }

}

