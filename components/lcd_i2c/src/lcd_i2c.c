#include <stdlib.h>
#include "lcd_i2c.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_check.h"

#define LCD_RS 0x01  //bit 0
#define LCD_RW 0x02  //bit 1
#define LCD_EN 0x04  //bit 2
#define LCD_BL 0x08  //bit 3
#define LCD_D4 0x10  
#define LCD_D5 0x20
#define LCD_D6 0x40
#define LCD_D7 0x80

#define LCD_CMD_CLEAR         0x01
#define LCD_CMD_ENTRY_MODE    0x06
#define LCD_CMD_DISPLAY_ON    0x0C
#define LCD_CMD_FUNCTION_SET  0x28
#define LCD_CMD_SET_DDRAM     0x80

static const char *TAG = "lcd_i2c";

struct lcd_i2c_t {
    i2c_master_dev_handle_t dev;
    uint8_t columns;
    uint8_t rows;
    uint8_t backlight;
};

static esp_err_t pcf8574_write(i2c_master_dev_handle_t dev, uint8_t value){
    return i2c_master_transmit(dev, &value, 1, 100);
}

static esp_err_t lcd_pulse_enable(i2c_master_dev_handle_t dev, uint8_t data){
    ESP_RETURN_ON_ERROR(pcf8574_write(dev, data |  LCD_EN ), TAG, "EN high failed");
    esp_rom_delay_us(1);
    ESP_RETURN_ON_ERROR(pcf8574_write(dev, data &  ~LCD_EN ), TAG, "EN low failed");
    esp_rom_delay_us(50);
    return ESP_OK;

}

static esp_err_t lcd_write_nibble(lcd_i2c_handle_t lcd, uint8_t nibble, uint8_t rs){
    uint8_t data = (nibble << 4)  | rs | lcd->backlight ; //dados vão para o nibble alto (d4-d7) 
    ESP_RETURN_ON_ERROR(pcf8574_write(lcd->dev, data), TAG, "write nibble failed");
    return lcd_pulse_enable(lcd->dev, data);
}

static esp_err_t lcd_send_byte(lcd_i2c_handle_t lcd, uint8_t data, uint8_t rs){
    ESP_RETURN_ON_ERROR(lcd_write_nibble(lcd, data >> 4, rs), TAG, "write byte failed");
    return lcd_write_nibble(lcd, data & 0x0f, rs);
}

static esp_err_t lcd_set_4_bit_mode(lcd_i2c_handle_t lcd){
    ESP_RETURN_ON_ERROR(lcd_write_nibble(lcd, 0x03, 0), TAG, "first 0x03 command failed");
    esp_rom_delay_us(5000);
    ESP_RETURN_ON_ERROR(lcd_write_nibble(lcd, 0x03, 0), TAG, "second 0x03 command failed");
    esp_rom_delay_us(150);
    ESP_RETURN_ON_ERROR(lcd_write_nibble(lcd, 0x03, 0), TAG, "third 0x03 command failed");
    esp_rom_delay_us(150);
    return lcd_write_nibble(lcd, 0x02, 0);
}

static esp_err_t lcd_send_command(lcd_i2c_handle_t lcd, uint8_t cmd){
    return lcd_send_byte(lcd, cmd, 0);
}

static esp_err_t lcd_send_data(lcd_i2c_handle_t lcd, uint8_t ch){
    return lcd_send_byte(lcd, ch, LCD_RS);
}

static esp_err_t lcd_init(lcd_i2c_handle_t lcd){
    esp_rom_delay_us(50000);
    ESP_RETURN_ON_ERROR(lcd_set_4_bit_mode(lcd),TAG, "4-bit set init failed");
    ESP_RETURN_ON_ERROR(lcd_send_command(lcd, LCD_CMD_FUNCTION_SET), TAG, "function set command failed");
    ESP_RETURN_ON_ERROR(lcd_send_command(lcd, LCD_CMD_DISPLAY_ON), TAG, "display on command failed");
    ESP_RETURN_ON_ERROR(lcd_send_command(lcd, LCD_CMD_CLEAR), TAG, "clear command failed");
    esp_rom_delay_us(2000);
    return lcd_send_command(lcd, LCD_CMD_ENTRY_MODE);
}

esp_err_t lcd_i2c_write_char(lcd_i2c_handle_t lcd, char c){
    return lcd_send_data(lcd, (uint8_t)c);
}

esp_err_t lcd_i2c_print(lcd_i2c_handle_t lcd, const char *str){
    while (*str){
        ESP_RETURN_ON_ERROR(lcd_send_data(lcd, *str), TAG, "print failed");
        str++;
    }
    return ESP_OK;
}

esp_err_t lcd_i2c_set_cursor(lcd_i2c_handle_t lcd, uint8_t col, uint8_t row){
    uint8_t memory_offset_array[2] = {0x00, 0x40};
    uint8_t memory_position = memory_offset_array[row] + col;

    return lcd_send_command(lcd, LCD_CMD_SET_DDRAM | memory_position);
}

esp_err_t lcd_i2c_clear(lcd_i2c_handle_t lcd) {
    ESP_RETURN_ON_ERROR(lcd_send_command(lcd, LCD_CMD_CLEAR), TAG, "clear failed");
    esp_rom_delay_us(2000);
    return ESP_OK;
}

esp_err_t lcd_i2c_create(const lcd_i2c_config_t *config, lcd_i2c_handle_t *out_lcd){
    lcd_i2c_handle_t lcd = calloc(1, sizeof(struct lcd_i2c_t));
    if (!lcd) return ESP_ERR_NO_MEM;
    i2c_device_config_t dev_cfg ={
    .dev_addr_length           =     I2C_ADDR_BIT_LEN_7,
    .device_address            =     config->address,
    .scl_speed_hz              =     config->scl_speed_hz,
    };

    esp_err_t err = i2c_master_bus_add_device(config->bus, &dev_cfg, &lcd->dev);
    if (err != ESP_OK){
        free(lcd);
        return err;
    }
    
    lcd->rows      = config->rows;
    lcd->columns   = config->columns;
    lcd->backlight = LCD_BL;
    
    err = lcd_init(lcd);
    if (err != ESP_OK){
        i2c_master_bus_rm_device(lcd->dev);
        free(lcd);
        return err;
    }
    *out_lcd = lcd;
    return ESP_OK;
}

esp_err_t lcd_i2c_delete(lcd_i2c_handle_t lcd){
    i2c_master_bus_rm_device(lcd->dev);
    free(lcd);
    return ESP_OK;
}

esp_err_t lcd_i2c_set_backlight(lcd_i2c_handle_t lcd, bool on){
    lcd->backlight = on ? LCD_BL : 0x00;
    return pcf8574_write(lcd->dev, lcd->backlight);
}