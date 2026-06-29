idf_component_register(SRCS "my_lcd_i2c_project.c"
                    INCLUDE_DIRS "."
                    REQUIRES lcd_i2c nvs_flash esp_wifi esp_netif esp_event)
