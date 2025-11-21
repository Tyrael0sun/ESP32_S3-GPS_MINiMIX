idf_component_register(SRCS "main.c" "sensors.c" "display.c" "input.c" "gnss.c" "battery.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_driver_uart esp_driver_gpio esp_driver_i2c esp_driver_spi esp_lcd esp_adc nvs_flash esp_timer)
