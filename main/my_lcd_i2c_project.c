#include "driver/i2c_master.h"
#include "lcd_i2c.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"

#define TAG "WIFI"

void scan_i2c_bus(i2c_master_bus_handle_t bus){
    ESP_LOGI(TAG, "Starting scanning on i2c bus...");
    int found = 0;
    for (uint8_t addr = 0x08; addr <= 0x77; addr++){
            esp_err_t ret = i2c_master_probe(bus, addr, 50);
            if (ret == ESP_OK){
                ESP_LOGI(TAG, "Device found in address: 0x%02X", addr);
                found++;
            }
        }
    ESP_LOGI(TAG, "Scanning completed: %d devices found", found);
} 

void app_main(void)
{
    
    esp_err_t err  = nvs_flash_init();
    if (err == ESP_ERR_NVS_NEW_VERSION_FOUND || err == ESP_ERR_NVS_NO_FREE_PAGES){
        nvs_flash_erase();
        nvs_flash_init();
    }
    err = esp_netif_init();
    if (err != ESP_OK){
        ESP_LOGE(TAG, "failed to init netif");
    }
    err = esp_event_loop_create_default();
    if (err != ESP_OK){
        ESP_LOGE(TAG, "failed to create event loop");
    }
    else {
        ESP_LOGI(TAG, "WiFi Fundations complete");
    }

}

/*
Lesso 1 - Hello World on LCD I2C from scratch

i2c_master_bus_config_t bus_cfg = {
        .clk_source                   = I2C_CLK_SRC_DEFAULT,
        .i2c_port                     = I2C_NUM_0,   //ESP32-S3 POSSUI 2 CONTROLADORES: 0 E 1
        .sda_io_num                   = GPIO_NUM_14, //PINO LIGADO AO SDA
        .scl_io_num                   = GPIO_NUM_13, //PINO LIGADO AO SCL
        .glitch_ignore_cnt            = 7,           //VALOR RECOMENDADO PELA ESPRESSIF PARA LIMPAR RUÍDOS
        .flags.enable_internal_pullup = true         //HABILITA PULL-UP INTERNO, MAS É RECOMENDADO USAR 
                                                    // RESISTORES EXTERNOS DE 4.7K OU 10K PARA MELHOR DESEMPENHO
    };
    i2c_master_bus_handle_t bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));

    lcd_i2c_config_t lcd_cfg = {
        .bus = bus,
        .address = 0x27,
        .columns = 16,
        .rows = 2,
        .scl_speed_hz = 100000,
    };
    lcd_i2c_handle_t lcd;
    ESP_ERROR_CHECK(lcd_i2c_create(&lcd_cfg, &lcd));

    lcd_i2c_print(lcd, "Hello");
    lcd_i2c_set_cursor(lcd, 0, 1);
    lcd_i2c_print(lcd, "World!");

*/