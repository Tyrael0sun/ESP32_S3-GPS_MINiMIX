idf_component_register(SRCS "main.c" "sensors.c" "display.c" "input.c" "gnss.c" "battery.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_lcd esp_adc)
