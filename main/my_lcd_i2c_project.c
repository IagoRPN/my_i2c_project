#include <stdio.h>
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_err.h"
#include "lcd_i2c.h"

#define TAG "I2C_SCAN"

i2c_master_bus_handle_t bus = NULL;
i2c_master_dev_handle_t dev = NULL;



void app_main(void)
{
    i2c_bus_init(&bus);
    i2c_device_init(bus, &dev);
    lcd_init(dev);
    lcd_print(dev, "Hello, World!");
}
