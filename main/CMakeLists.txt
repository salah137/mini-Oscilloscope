# This registers the 'main' component and its source files
idf_component_register(
    SRCS "main.c"
        "timer_config.c"
        "adc_reading.c"
        "draw_ui.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES i2c_oled
                    wifi_sta
    REQUIRES esp_driver_gptimer
             esp_driver_gpio
             esp_adc

)
