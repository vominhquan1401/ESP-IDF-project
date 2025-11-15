# idf_component_register(
#     SRCS "main.c"
#     INCLUDE_DIRS "."
#     REQUIRES wifi_manager nvs_manager wifi_config_portal
# )

file(GLOB_RECURSE srcs "main.c" "src/*.c")

idf_component_register(
    SRCS "${srcs}"
    INCLUDE_DIRS "./inc"

    )
