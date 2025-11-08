idf_component_register(
    SRCS "wifi_config_portal.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_http_server wifi_manager
    PRIV_REQUIRES
)
