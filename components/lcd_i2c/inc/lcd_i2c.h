#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"


esp_err_t i2c_bus_init(i2c_master_bus_handle_t *out_bus)

esp_err_t lcd_i2c_hello(void);

esp_err_t i2c_device_init(i2c_master_bus_handle_t bus, i2c_master_dev_handle_t *out_dev)

void scan_i2c_bus(i2c_master_bus_handle_t bus);