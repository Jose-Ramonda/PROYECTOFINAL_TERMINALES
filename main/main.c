
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "driver/gpio.h"

#include "config.h"
#include "app_uart.h"
#include "protocol.h"
#include "DIO.h"
//#include "SEAII.h"

#include "app_wifi.h"
#include "app_NFC.h"
#include "camara.h"
#include "PCF8574.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "nvs_flash.h"

void reset_task(void *pvParameters) {
    
    SemaphoreHandle_t cmd_sem = protocol_get_ctrl_sem(CMD_RESET);
    if (cmd_sem == NULL) {
        ESP_LOGE("RESET", "No se pudo obtener el semáforo de puerta");
        esp_restart();
    }

    while(1){
        if (xSemaphoreTake(cmd_sem, portMAX_DELAY) == pdTRUE) {
            esp_restart();
        }                               
    }
}

void life_signal(void){
    gpio_set_level(FLASH, 1);
    vTaskDelay(pdMS_TO_TICKS(750));
    pcf8574_set_pin(PCF_DESTRABADOR_PIN,0);
    gpio_set_level(FLASH, 0);
    vTaskDelay(pdMS_TO_TICKS(750));
    pcf8574_set_pin(PCF_DESTRABADOR_PIN,1);
    gpio_set_level(FLASH, 1);
    vTaskDelay(pdMS_TO_TICKS(750));
    pcf8574_set_pin(PCF_DESTRABADOR_PIN,0);
    gpio_set_level(FLASH, 0);
    vTaskDelay(pdMS_TO_TICKS(750));
    pcf8574_set_pin(PCF_DESTRABADOR_PIN,1);
    gpio_set_level(FLASH, 1);
    vTaskDelay(pdMS_TO_TICKS(750));
    gpio_set_level(FLASH, 0);
}

void app_main(void){ 
    
    //Inicializar el protocolo y la comunicación serial:
    protocol_params_t parametros;
    parametros.ctrl_cmds =10;
    parametros.st_cmds =10;
    parametros.masterid= MASTER_ID;
    parametros.nodoid = NODO_ID_DEFAULT;

    parametros.dispatcher_priority = 8;
    parametros.buffer_getter = uart_get_rx_streambuffer;
    parametros.dispatcher_stack =4096;

    parametros.sender = app_uart_send;
    parametros.parser_priority = 7;
    parametros.parser_stack = 4096;
    
    uart_init();
    xTaskCreate(uart_rx_task,"RX_TASK",4096,NULL,10,NULL);
    protocol_init(&parametros);

    
    //Esto se borrará
    //xTaskCreate(CMD2_LED_task,"LED_TASK",4096,NULL,4,NULL);
    //xTaskCreate(CMD3_SLOW_task,"SLOW_TASK",4096,NULL,2,NULL);
    //xTaskCreate(CMD100_INVERTER_task,"INVERTER_TASK",4096,NULL,2,NULL);


    //Configurar bus I2C
    static i2c_master_bus_handle_t bus0_handle;
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .sda_io_num = SDA_PIN,
        .scl_io_num = SCL_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus0_handle));

    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);

    //Inicializar las tareas
        //Initialize NVS / init de la flash
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);   //Verifico porlas

    xTaskCreate(app_wifi_com_task,"WIFI",4096,NULL,3,NULL);


    nfc_init();
    pcf_init();
    dio_init();

    //life_signal();
    xTaskCreate(camara_task,"CAM",8192,NULL,3,NULL);
    

    //Mandar foto ->debug
    // SemaphoreHandle_t cmd_sem = protocol_get_ctrl_sem(CMD_TAKE_PH);
    //xSemaphoreGive(cmd_sem);
    //xTaskCreate(reset_task,"RST",512,NULL,20,NULL);
    composer(CMD_READY,0,NULL,NULL);
    vTaskDelay(pdMS_TO_TICKS(2000));
    //life_signal();

    SemaphoreHandle_t test = protocol_get_ctrl_sem(CMD_DOOR);
    xSemaphoreGive(test);
    
}
   

