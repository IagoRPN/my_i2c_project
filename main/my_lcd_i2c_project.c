#include <stdio.h>
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_err.h"
#include "lcd_i2c.h"

#define TAG "I2C_SCAN"





void app_main(void)
{
    
    lcd_i2c_hello();

    /*
    while(true){
        //AQUI VOCÊ PODE ADICIONAR O CÓDIGO PARA INTERAGIR COM O LCD VIA I2C

        scan_i2c_bus(bus);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }*/
}
