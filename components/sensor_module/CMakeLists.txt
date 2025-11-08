idf_component_register(
    SRCS "light_sensor.c" "dht20.c"
    INCLUDE_DIRS "."
    REQUIRES driver freertos
)
