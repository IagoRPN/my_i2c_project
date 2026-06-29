idf_component_register(SRCS "src/lcd_i2c.c" "src/marquee.c"
                         INCLUDE_DIRS "inc"
                         REQUIRES driver freertos)