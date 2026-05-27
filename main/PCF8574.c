#include "PCF8574.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"



#define PCF8574_ADDR 0x20

#define I2C_TIMEOUT_MS 100

static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t pcf_dev;
static uint8_t pcf_state = 0xFF;
static SemaphoreHandle_t pcf_mutex = NULL;

esp_err_t pcf8574_write(uint8_t data){
    esp_err_t err;
    err =i2c_master_transmit(pcf_dev,&data,1,I2C_TIMEOUT_MS);
    return err;
}


void pcf_init(void){
   ESP_ERROR_CHECK(i2c_master_get_bus_handle(I2C_NUM_0,&bus_handle));
   pcf_mutex = xSemaphoreCreateMutex();

   i2c_device_config_t dev_cfg = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = PCF8574_ADDR,
    .scl_speed_hz = 100000,
    };
   ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle,&dev_cfg,&pcf_dev));
   //Apago todo
   pcf8574_write(0xFF);

   //Pruebo la funciones parpadeandoe el led en P7
   /*
   pcf8574_write(0xFF);
   vTaskDelay(pdMS_TO_TICKS(750));
   pcf8574_write(0xFE);
   vTaskDelay(pdMS_TO_TICKS(750));
   pcf8574_write(0xFF);
   vTaskDelay(pdMS_TO_TICKS(750));
   pcf8574_write(0xFE);
   vTaskDelay(pdMS_TO_TICKS(750));
   pcf8574_write(0xFF);
   vTaskDelay(pdMS_TO_TICKS(750));
   pcf8574_write(0xFE);
    */
}

esp_err_t pcf8574_set_pin(uint8_t pin, uint8_t value) {
    if (pin > 7) return ESP_ERR_INVALID_ARG;
    
    esp_err_t err = ESP_ERR_TIMEOUT;
    
    if (xSemaphoreTake(pcf_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (value) {
            pcf_state |= (1 << pin);
        } else {
            pcf_state &= ~(1 << pin);
        }
        err = pcf8574_write(pcf_state);
        xSemaphoreGive(pcf_mutex);
    } else {
        ESP_LOGW("PCF8574", "pcf8574_set_pin: timeout esperando mutex");
    }
    
    return err;
}



