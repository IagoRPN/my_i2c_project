#include "lcd_i2c.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

#define LCD_RS 0x01  //bit 0
#define LCD_RW 0x02  //bit 1
#define LCD_EN 0x04  //bit 2
#define LCD_BL 0x08  //bit 3
#define LCD_D4 0x10
#define LCD_D5 0x20
#define LCD_D6 0x40
#define LCD_D7 0x80

static const char *TAG = "lcd_i2c";

esp_err_t i2c_bus_init(i2c_master_bus_handle_t *out_bus){
    i2c_master_bus_config_t bus_cfg = {
    .clk_source                   = I2C_CLK_SRC_DEFAULT,
    .i2c_port                     = I2C_NUM_0,   //ESP32-S3 POSSUI 2 CONTROLADORES: 0 E 1
    .sda_io_num                   = GPIO_NUM_14, //PINO LIGADO AO SDA
    .scl_io_num                   = GPIO_NUM_13, //PINO LIGADO AO SCL
    .glitch_ignore_cnt            = 7,           //VALOR RECOMENDADO PELA ESPRESSIF PARA LIMPAR RUÍDOS
    .flags.enable_internal_pullup = true         //HABILITA PULL-UP INTERNO, MAS É RECOMENDADO USAR 
                                                 // RESISTORES EXTERNOS DE 4.7K OU 10K PARA MELHOR DESEMPENHO
    };
    
    esp_err_t err = i2c_new_master_bus(&bus_cfg, out_bus));
    return err;

}

esp_err_t i2c_device_init(i2c_master_bus_handle_t bus, i2c_master_dev_handle_t *out_dev){
    i2c_device_config_t dev_cfg = {
        .dev_addr_length      =   I2C_ADDR_BIT_LEN_7,  //ENDEREÇO DE 7 BITS
        .device_address       = 0x27,
        .scl_speed_hz         = 100000,
    };
    
    esp_err_t err = i2c_master_bus_add_device(bus, &dev_cfg, out_dev));
    return err

}

static esp_err_t pcf8574_write(i2c_master_dev_handle_t dev, uint8_t value){
    return i2c_master_transmit(dev, &value, 1, 100);
}

static void lcd_pulse_enable(i2c_master_dev_handle_t dev, uint8_t data){
    pcf8574_write(dev, data |  LCD_EN );
    esp_rom_delay_us(1);
    pcf8574_write(dev, data &  ~LCD_EN );
    esp_rom_delay_us(50);

}

static void lcd_write_nibble(i2c_master_dev_handle_t dev, uint8_t nibble, uint8_t rs){
    uint8_t data = (nibble << 4)  | rs | LCD_BL ; //dados vão para o nibble alto (d4-d7) 
    pcf8574_write(dev, data);
    lcd_pulse_enable(dev, data);
}



esp_err_t lcd_i2c_hello(void){
    ESP_LOGI(TAG, "i2c component linked with success");
    return ESP_OK;
}

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