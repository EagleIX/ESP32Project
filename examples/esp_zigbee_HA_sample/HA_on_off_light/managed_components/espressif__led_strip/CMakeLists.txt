set(srcs "src/led_strip_api.c")

if(CONFIG_SOC_RMT_SUPPORTED)
    list(APPEND srcs "src/led_strip_rmt_dev.c" "src/led_strip_rmt_encoder.c")
endif()

idf_component_register(SRCS "src/led_strip_api.c"
                             "src/led_strip_rmt_dev.c"
                             "src/led_strip_rmt_encoder.c"
                       INCLUDE_DIRS "include" "interface"  # Добавьте "interface" здесь
                       REQUIRES driver esp_driver_rmt
                       PRIV_REQUIRES esp_driver_rmt)