idf_component_register(
    SRCS "http_client_module.c"
    INCLUDE_DIRS "."
    REQUIRES esp_http_client json wifi_manager nvs_manager
)
