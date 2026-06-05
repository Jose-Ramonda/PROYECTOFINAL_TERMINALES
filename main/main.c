
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

static const char *TAG = "MAIN";

static i2c_master_bus_handle_t bus0_handle;
static i2c_master_bus_config_t bus_cfg = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = I2C_NUM_0,
    .sda_io_num = SDA_PIN,
    .scl_io_num = SCL_PIN,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
};
static esp_err_t i2c_bus_recover(void);


void reset_task(void *pvParameters) {
    
    SemaphoreHandle_t cmd_sem = protocol_get_ctrl_sem(CMD_RESET);
    if (cmd_sem == NULL) {
        ESP_LOGE("RESET", "No se pudo obtener el semáforo de reset");
        esp_restart();
    }

    while(1){
        if (xSemaphoreTake(cmd_sem, portMAX_DELAY) == pdTRUE) {
            esp_restart();
        }                               
    }
}
void recover_task(void *pvParameters) {
    
    SemaphoreHandle_t cmd_sem = protocol_get_ctrl_sem(CMD_RECOVER);
    if (cmd_sem == NULL) {
        ESP_LOGE("RESET", "No se pudo obtener el semáforo de puerta");
        esp_restart();
    }

    while(1){
        if (xSemaphoreTake(cmd_sem, portMAX_DELAY) == pdTRUE) {
            ESP_ERROR_CHECK(i2c_bus_recover());
        }                               
    }
}

static esp_err_t i2c_bus_recover(void) {
    ESP_LOGW(TAG, "Intentando recuperación del bus I2C (Bit-banging)...");

    // 1. Eliminar el bus actual de manera segura con la nueva API
    if (bus0_handle != NULL) {
        // IMPORTANTE: En la API v5, debes remover los dispositivos antes de destruir el bus.
        // Si usas handles para el PN532 o PCF8574 (ej: pn532_dev_handle), comentalos o elimínalos aquí:
        // i2c_master_bus_rm_device(pn532_dev_handle); 
        quitari2c();
        // i2c_master_bus_rm_device(pcf8574_dev_handle);
        
        i2c_del_master_bus(bus0_handle);
        bus0_handle = NULL;
    }
    
    vTaskDelay(pdMS_TO_TICKS(10));

    // 2. Tomar el control manual de los pines (Open-Drain)
    gpio_config_t io_conf = {
        .mode         = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
        .pin_bit_mask = (1ULL << SDA_PIN) | (1ULL << SCL_PIN),
    };
    gpio_config(&io_conf);

    // 3. Liberar el bus: forzamos ambos a HIGH
    gpio_set_level(SDA_PIN, 1);
    gpio_set_level(SCL_PIN, 1);
    esp_rom_delay_us(10);

    // 4. Si SDA está secuestrado en LOW, enviamos hasta 9 pulsos de reloj
    if (!gpio_get_level(SDA_PIN)) {
        ESP_LOGW(TAG, "SDA trabado en LOW, bombeando SCL...");
        for (int i = 0; i < 9; i++) {
            gpio_set_level(SCL_PIN, 0);
            esp_rom_delay_us(10);
            gpio_set_level(SCL_PIN, 1);
            esp_rom_delay_us(10);
            
            // Si el esclavo suelta la línea SDA, terminamos temprano
            if (gpio_get_level(SDA_PIN)) {
                ESP_LOGI(TAG, "Bus SDA liberado tras %d pulsos", i + 1);
                break;
            }
        }
    }

    // 5. Emitir una condición de STOP por software (SCL HIGH, SDA pasa de LOW a HIGH)
    gpio_set_level(SDA_PIN, 0);
    esp_rom_delay_us(10);
    gpio_set_level(SCL_PIN, 1);
    esp_rom_delay_us(10);
    gpio_set_level(SDA_PIN, 1);
    esp_rom_delay_us(10);

    vTaskDelay(pdMS_TO_TICKS(20));

    // 6. Resetear la configuración de los pines para que el driver I2C los tome limpios
    gpio_reset_pin(SDA_PIN);
    gpio_reset_pin(SCL_PIN);

    // 7. Recrear el master bus usando tu configuración existente
    esp_err_t err = i2c_new_master_bus(&bus_cfg, &bus0_handle);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Bus I2C reinstalado exitosamente (API v5)");
        
        // --- AVISO CRÍTICO ---
        // Aquí deberás volver a añadir los dispositivos al bus usando:
        // i2c_master_bus_add_device(bus0_handle, &pn532_device_cfg, &pn532_dev_handle);
        // De lo contrario, no podrás enviarles comandos.
        
    } else {
        ESP_LOGE(TAG, "Fallo al reinstalar bus I2C: %s", esp_err_to_name(err));
    }

    return err;
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
    //pcf_init();
    dio_init();

    
    xTaskCreate(camara_task,"CAM",8192,NULL,3,NULL);
    xTaskCreate(recover_task,"REC",2048,NULL,20,NULL);

    

    //Mandar foto ->debug
    // SemaphoreHandle_t cmd_sem = protocol_get_ctrl_sem(CMD_TAKE_PH);
    //xSemaphoreGive(cmd_sem);
    xTaskCreate(reset_task,"RST",512,NULL,20,NULL);
    composer(CMD_READY,0,NULL,NULL);
    vTaskDelay(pdMS_TO_TICKS(2000));


}
   

