#include "mqtt_module.h"


static const char *TAG = "MQTT_MODULE";
static esp_mqtt_client_handle_t s_client = NULL;
static mqtt_message_cb_t s_msg_cb = NULL;
bool s_connected = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t eid, void *edata) {
    esp_mqtt_event_handle_t event = edata;

    switch (eid) {
        case MQTT_EVENT_CONNECTED:
            s_connected = true;
            ESP_LOGI(TAG, "MQTT Connected");
            break;
        case MQTT_EVENT_DISCONNECTED:
            s_connected = false;
            ESP_LOGW(TAG, "MQTT Disconnected");
            break;
        case MQTT_EVENT_DATA:
            if (s_msg_cb)
                s_msg_cb(event->topic, event->data, event->data_len);
            break;
        default:
            break;
    }
}

void mqtt_client_init(const mqtt_client_cfg_t *cfg) {
    char uri_buf[128] = {0};

    // Select server mode
    if (cfg->server_type == MQTT_SERVER_ADAFRUIT_IO) {
        snprintf(uri_buf, sizeof(uri_buf), cfg->use_ssl ? "mqtts://io.adafruit.com:8883" : "mqtt://io.adafruit.com:1883");
    } else if (cfg->server_type == MQTT_SERVER_CUSTOM) {
        snprintf(uri_buf, sizeof(uri_buf), "%s", cfg->uri);
    }

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = uri_buf,
        .credentials.username = cfg->username,
        .credentials.authentication.password = cfg->password,
    };

    if (cfg->use_ssl && cfg->ca_cert)
        mqtt_cfg.broker.verification.certificate = cfg->ca_cert;

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
}

void mqtt_client_start(void) {
    if (s_client) esp_mqtt_client_start(s_client);
}

void mqtt_client_stop(void) {
    if (s_client) esp_mqtt_client_stop(s_client);
}

bool mqtt_client_is_connected(void) { return s_connected; }

void mqtt_client_set_callback(mqtt_message_cb_t cb) { s_msg_cb = cb; }

int mqtt_client_publish(const char *topic, const char *data, int len, int qos, int retain) {
    if (!s_connected) return -1;
    return esp_mqtt_client_publish(s_client, topic, data, len, qos, retain);
}

int mqtt_client_subscribe(const char *topic, int qos) {
    if (!s_connected) return -1;
    return esp_mqtt_client_subscribe(s_client, topic, qos);
}