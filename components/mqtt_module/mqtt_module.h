#ifndef MQTT_MODULE_H
#define MQTT_MODULE_H


#ifdef __cplusplus
extern "C" {
#endif
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "esp_err.h"
#include "mqtt_client.h"

typedef enum {
MQTT_SERVER_CUSTOM = 0, // Provide URI manually
MQTT_SERVER_ADAFRUIT_IO, // Use Adafruit IO preset
} mqtt_server_type_t;


typedef void (*mqtt_message_cb_t)(const char *topic, const char *data, int len);


typedef struct {
mqtt_server_type_t server_type; // Server preset
const char *uri; // Custom URI
const char *username; // Username for authentication
const char *password; // Password or API Key
const char *ca_cert; // CA Cert for SSL
bool use_ssl; // Enable SSL (mqtts://)
} mqtt_client_cfg_t;

// Khởi tạo MQTT client với cấu hình server (URI, username, password, SSL)
void mqtt_client_init(const mqtt_client_cfg_t *cfg);
// Bắt đầu kết nối MQTT tới server (gọi event handler, auto reconnect)
void mqtt_client_start(void);
// Dừng kết nối MQTT (không xóa client, có thể start lại)
void mqtt_client_stop(void);
// Kiểm tra trạng thái kết nối MQTT (true = đã kết nối broker)
bool mqtt_client_is_connected(void);
// Đăng ký callback nhận dữ liệu từ broker khi có message mới
void mqtt_client_set_callback(mqtt_message_cb_t cb);
// Gửi dữ liệu lên broker theo topic (qos và retain tùy server)
int mqtt_client_publish(const char *topic, const char *data, int len, int qos, int retain);
// Đăng ký nhận dữ liệu từ broker theo topic
int mqtt_client_subscribe(const char *topic, int qos);



#ifdef __cplusplus
}
#endif


#endif // MQTT_MODULE_H