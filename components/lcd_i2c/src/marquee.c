#include "lcd_i2c.h"
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_rom_sys.h"


#define TAG "lcd_i2c_marquee"

typedef struct lcd_i2c_marquee_t {
    lcd_i2c_handle_t lcd;
    uint8_t row;
    uint8_t col_start;
    uint8_t width;
    char *text;
    size_t scroll_len
    uint32_t period_ms;
    size_t offset
    TaskHandle_t task;
} lcd_i2c_marquee_handle_t;

esp_err_t lcd_i2c_marquee_create(lcd_i2c_handle_t lcd,                     SemaphoreHandle_t lcd_mutex, uint8_t row, uint8_t col_start, uint8_t col_end, uint8_t chars_per_sec, const char *initial_text, lcd_i2c_marquee_handle_t *out ){

    if (lcd == NULL){
        ESP_LOGE(TAG, "lcd arg can't be NULL");
    }
    if (out == NULL){
        ESP_LOGE(TAG, "out arg can't be NULL");
    }
    if (initial_text == NULL){
        ESP_LOGE(TAG, "initial_text arg can't be NULL");
    }
    if (col_start >= col_end){
        ESP_LOGE(TAG, "col_start can't be lower than col_end");
        return ESP_ERR_INVALID_ARG;
    }
    if (chars_per_sec <= 0){
        ESP_LOGE(TAG, "chars_per_sec can't be lower than 0");
        return ESP_ERR_INVALID_ARG;
    }

    lcd_i2c_marquee_handle_t *m = calloc(1, sizeof(*m));
    if (m == NULL) return ESP_ERR_NO_MEM;
    m->text = malloc(strlen(initial_text) + 1)
    if (m->text == NULL) {
        free(m);
        return ESP_ERR_NO_MEM;
    };
    strcpy(&initial_text, m->text);

    m->lcd = lcd;
    m->row = row;
    m->col_start = col_start;
    m->width = col_end - col_start + 1;
    m->period_ms = 1000/ chars_per_sec;
    m->scroll_len = strlen(m->text) + MARQUEE_GAP;


    
}

