/*
*   Archivo de progrma de manejo de entradas y salidas digitales
*   Nativas o extensas del módulo PCF8574
*
*   Autor: José Ramonda
*   Ultima modificación: 25/5/2026 ¡Que viva la patria!
*/

#include "DIO.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "soc/gpio_periph.h"

#include "config.h"
#include "protocol.h"
#include "PCF8574.h"



static SemaphoreHandle_t cmd_puerta = NULL;
static SemaphoreHandle_t cmd_foto = NULL;
static TimerHandle_t xTimbreTimer;




static void IRAM_ATTR timbre_isr_handler(void* arg) {
    if (cmd_foto != NULL) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        // A. Avisamos a la tarea de la foto
        xSemaphoreGiveFromISR(cmd_foto, &xHigherPriorityTaskWoken);

        // B. Arrancamos el timer con el período mínimo para que responda YA
        if (xTimbreTimer != NULL) {
            xTimerStartFromISR(xTimbreTimer, &xHigherPriorityTaskWoken);
        }

        if (xHigherPriorityTaskWoken == pdTRUE) {
            portYIELD_FROM_ISR();
        }
    }
}



void vTimbreCallback(TimerHandle_t xTimer) {
    pcf8574_set_pin(PCF_TIMBRE_PIN,1);//Apaar el timbre
}

void dio_init(void){
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,       // Interrupción cuando va a GND (bajada)
        .pin_bit_mask = (1ULL << TIMBRE_PIN), 
        .mode = GPIO_MODE_INPUT,               
        .pull_up_en = GPIO_PULLUP_ENABLE,     // Pull-up interno para mantenerlo en 3.3V
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    gpio_config(&io_conf);


    

    //Enganchamos el manejador al pin del timbre
    gpio_isr_handler_add(TIMBRE_PIN, timbre_isr_handler, NULL);

    //Agarro el semaforo correspondiente
    cmd_puerta = protocol_get_ctrl_sem(CMD_DOOR);
    cmd_foto = protocol_get_ctrl_sem(CMD_TAKE_PH);
    xTimbreTimer = xTimerCreate("TimerTimbre", pdMS_TO_TICKS(TIEMPO_TIMBRE), pdFALSE, (void *) 0, vTimbreCallback);

    xTaskCreate(puerta_task,"DOOR",2048,NULL,8,NULL);


    gpio_reset_pin(4); 
    
    // EXPLICACIÓN: Esta macro de bajo nivel desengancha el pin 4 de la interfaz 
    // de la tarjeta SD nativa (SDMMC) y lo obliga a ser un GPIO digital común.
    PIN_FUNC_SELECT(GPIO_PIN_REG_4, PIN_FUNC_GPIO); 
    
    gpio_set_direction(4, GPIO_MODE_OUTPUT);
    gpio_set_level(4, 0); // Arranca apagado
}

void puerta_task(void *pvParameters) {
    
    if (cmd_puerta == NULL) {
        ESP_LOGE("PUERTA", "No se pudo obtener el semáforo de puerta");
        vTaskDelete(NULL);
    }


    while(1){
        if (xSemaphoreTake(cmd_puerta, portMAX_DELAY) == pdTRUE) {
            composer(CMD_DOOR,0,NULL,NULL);//Envío confirmación
            pcf8574_set_pin(PCF_DESTRABADOR_PIN,0);  //Destrabo puerta
            composer(CMD_DOOR,0,NULL,NULL);//Envío confirmación
            vTaskDelay(pdMS_TO_TICKS(TIEMPO_PUERTA));
            pcf8574_set_pin(PCF_DESTRABADOR_PIN,1);  //bloqueo puerta

        }                               
    }
}