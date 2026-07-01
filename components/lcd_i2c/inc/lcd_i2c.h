#pragma once
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdbool.h>

#define MARQUEE_GAP 3

// Handle OPACO: o usuário só guarda o ponteiro, não enxerga o conteúdo.
typedef struct lcd_i2c_t *lcd_i2c_handle_t;


typedef struct lcd_i2c_marquee_t *lcd_i2c_marquee_handle_t;

// Configuração passada na criação (o bus é INJETADO aqui):
typedef struct {
    i2c_master_bus_handle_t bus;     // criado pela aplicação
    uint8_t  address;                // ex.: 0x27
    uint8_t  columns;                // ex.: 16
    uint8_t  rows;                   // ex.: 2
    uint32_t scl_speed_hz;           // ex.: 100000
} lcd_i2c_config_t;

esp_err_t lcd_i2c_create(const lcd_i2c_config_t *config, lcd_i2c_handle_t *out_lcd);
esp_err_t lcd_i2c_delete(lcd_i2c_handle_t lcd);

esp_err_t lcd_i2c_clear(lcd_i2c_handle_t lcd);
esp_err_t lcd_i2c_set_cursor(lcd_i2c_handle_t lcd, uint8_t col, uint8_t row);
esp_err_t lcd_i2c_print(lcd_i2c_handle_t lcd, const char *str);
esp_err_t lcd_i2c_write_char(lcd_i2c_handle_t lcd, char c);
esp_err_t lcd_i2c_set_backlight(lcd_i2c_handle_t lcd, bool on);
esp_err_t lcd_i2c_marquee_create(lcd_i2c_handle_t lcd, SemaphoreHandle_t lcd_mutex,uint8_t row, uint8_t col_start, uint8_t col_end, uint8_t chars_per_sec, const char *initial_text, lcd_i2c_marquee_handle_t *out );
esp_err_t lcd_i2c_marquee_delete(lcd_i2c_marquee_handle_t m);
esp_err_t lcd_i2c_marquee_set_text(lcd_i2c_marquee_handle_t m, const char *text);
