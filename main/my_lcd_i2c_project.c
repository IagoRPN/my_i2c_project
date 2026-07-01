#include "driver/i2c_master.h"
#include "lcd_i2c.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"

#define TAG "WIFI"

static EventGroupHandle_t s_wifi_events;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;
#define WIFI_MAX_RETRY      5


static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){

    if (event_base == WIFI_EVENT){
        switch (event_id){
            case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;

            case WIFI_EVENT_STA_DISCONNECTED:
            if (s_retry_num < WIFI_MAX_RETRY){
                esp_wifi_connect();
                s_retry_num++;
                ESP_LOGW(TAG, "retry connection %d/%d", s_retry_num, WIFI_MAX_RETRY);

            } else {
                xEventGroupSetBits(s_wifi_events, WIFI_FAIL_BIT);
            }
            break;

            default:
            break;
        }
    } else if (event_base == IP_EVENT){
        switch (event_id){
            case IP_EVENT_STA_GOT_IP: {
            s_retry_num = 0;
            xEventGroupSetBits(s_wifi_events, WIFI_CONNECTED_BIT);
            ip_event_got_ip_t *e = (ip_event_got_ip_t *) event_data;
            ESP_LOGI(TAG, "got IP: " IPSTR, IP2STR(&e->ip_info.ip));
            break;
            }
        }
    }
}


void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_LOGI(TAG, "WiFi Foundation complete");
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_LOGI(TAG, "Wifi STA inicializado");
    s_wifi_events = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_wifi_start());
}

/*
Lesson 1 - Hello World on LCD I2C from scratch

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

Lesson 2 - Handling with marquee

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
    
    SemaphoreHandle_t lcd_mutex = xSemaphoreCreateMutex();
    
    lcd_i2c_marquee_handle_t mq;
    lcd_i2c_marquee_handle_t mq_2;
    ESP_ERROR_CHECK(lcd_i2c_marquee_create(
        lcd, lcd_mutex, 0, 8, 14, 5, "Texto rolando no LCD com ESP32-S3", &mq
    ));
    ESP_ERROR_CHECK(lcd_i2c_marquee_create(
        lcd, lcd_mutex, 1, 7, 14, 5, "Outro texto aqui", &mq_2
    ));
    

    vTaskDelay(pdMS_TO_TICKS(8000));
    
    ESP_ERROR_CHECK(lcd_i2c_marquee_set_text(mq, "Novo texto agora"));

*/