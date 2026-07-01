#include "lcd_i2c.h"
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_rom_sys.h"


#define TAG "lcd_i2c_marquee"

struct lcd_i2c_marquee_t {
    lcd_i2c_handle_t lcd;
    uint8_t row;
    uint8_t col_start;
    uint8_t width;
    char *text;
    size_t scroll_len;
    uint32_t period_ms;
    size_t offset;
    TaskHandle_t task;
    SemaphoreHandle_t mutex;
    volatile bool running;
    SemaphoreHandle_t done;
};

static void marquee_task(void *arg){
    lcd_i2c_marquee_handle_t m = (lcd_i2c_marquee_handle_t) arg;
    
    while (m->running){
        if (m->mutex) xSemaphoreTake(m->mutex, portMAX_DELAY);
        size_t text_len = m->scroll_len - MARQUEE_GAP;
        char win[m->width + 1];
        for (uint8_t k = 0; k < m->width; k++){
            size_t idx = (m->offset + k) % m->scroll_len;
            win[k] = (idx < text_len) ? m->text[idx] : ' ';
            
        }
        win[m->width] = '\0';
        
        lcd_i2c_set_cursor(m->lcd, m->col_start, m->row);
        
        lcd_i2c_print(m->lcd, win);
        m->offset = (m->offset + 1) % m->scroll_len;
        if (m->mutex) xSemaphoreGive(m->mutex);
        
        vTaskDelay(pdMS_TO_TICKS(m->period_ms));
        
    }
    xSemaphoreGive(m->done);
    vTaskDelete(NULL);
}

esp_err_t lcd_i2c_marquee_delete(lcd_i2c_marquee_handle_t m){
    if (m == NULL) return ESP_ERR_INVALID_ARG;
    m->running = false;
    xSemaphoreTake(m->done, portMAX_DELAY);
    vSemaphoreDelete(m->done);
    free(m->text);
    free(m);
    return ESP_OK;
}

esp_err_t lcd_i2c_marquee_create(lcd_i2c_handle_t lcd,                     SemaphoreHandle_t lcd_mutex, uint8_t row, uint8_t col_start, uint8_t col_end, uint8_t chars_per_sec, const char *initial_text, lcd_i2c_marquee_handle_t *out ){

    if (lcd == NULL){
        ESP_LOGE(TAG, "lcd arg can't be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    if (out == NULL){
        ESP_LOGE(TAG, "out arg can't be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    if (initial_text == NULL){
        ESP_LOGE(TAG, "initial_text arg can't be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    if (col_start >= col_end){
        ESP_LOGE(TAG, "col_start can't be lower than col_end");
        return ESP_ERR_INVALID_ARG;
    }
    if (chars_per_sec <= 0){
        ESP_LOGE(TAG, "chars_per_sec can't be lower than 0");
        return ESP_ERR_INVALID_ARG;
    }

    lcd_i2c_marquee_handle_t m = calloc(1, sizeof(*m));
    if (m == NULL) return ESP_ERR_NO_MEM;
    m->text = malloc(strlen(initial_text) + 1);
    if (m->text == NULL) {
        free(m);
        return ESP_ERR_NO_MEM;
    }
    strcpy(m->text, initial_text);

    m->lcd = lcd;
    m->row = row;
    m->col_start = col_start;
    m->width = col_end - col_start + 1;
    m->period_ms = 1000/ chars_per_sec;
    m->scroll_len = strlen(m->text) + MARQUEE_GAP;
    m->mutex = lcd_mutex;
    m->running = true;
    m->done = xSemaphoreCreateBinary();
    if (m->done == NULL){
        free(m->text);
        free(m);
        return ESP_ERR_NO_MEM;
    }

    BaseType_t ok = xTaskCreate(marquee_task, "marquee", 4096, m, 5, &m->task);
    if (ok != pdPASS){
        free(m->text);
        free(m->done);
        free(m);
        return ESP_ERR_NO_MEM;
    }
    *out = m;
    return ESP_OK;
}

esp_err_t lcd_i2c_marquee_set_text(lcd_i2c_marquee_handle_t m, const char *text){
    if (m == NULL){
        ESP_LOGE(TAG, "lcd_i2c_marquee_handle can't be NULL");
        return ESP_ERR_INVALID_ARG;

    }
     if (text == NULL){
        ESP_LOGE(TAG, "text can't be NULL");
        return ESP_ERR_INVALID_ARG;
    }
    char *novo = malloc(strlen(text) + 1);
    if (!novo) return ESP_ERR_NO_MEM;
    strcpy(novo, text);
    if (m->mutex) xSemaphoreTake(m->mutex, portMAX_DELAY);
    free(m->text);
    m->text = novo;
    m->scroll_len = strlen(novo) + MARQUEE_GAP;
    m->offset = 0;
    if (m->mutex) xSemaphoreGive(m->mutex);
    return ESP_OK;
}



