/*
*   Archivo de tareas que se ejecutaran en funcion de demostrar arquitectura desarrollada 
*   en el marco de asignatura Sistemas embebidos avanzados dos
*   El contenido de este archivo debería estar en el main, se separó por legibilidad, no como biblioteca  
*   Contenidos:
*       CMD 2 : Togleo de LED integrado
*       CMD 3 : Tarea que demora mucho para mostrar polling y lógica
*
*   Autor: José Ramonda
*   Actualizado: 1/3/2026
*/

#include "SEAII.h"
#include "driver/gpio.h"    //Para el LED
#include "protocol.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/message_buffer.h"
#include "freertos/semphr.h"

#include "esp_log.h"
<<<<<<< HEAD
#include "config.h"
=======
>>>>>>> 725ef28d47adc0ab91e8af5292927ebd8cbcc7e3

void CMD2_LED_task(void *pvParameters) { //CMD 2
    
    //Cofigurar LED
    gpio_reset_pin(LED_INTEGRADO); 
    gpio_set_direction(LED_INTEGRADO, GPIO_MODE_OUTPUT);

    //Agarro el semaforo correspondiente
<<<<<<< HEAD
    SemaphoreHandle_t cmd_sem = protocol_get_ctrl_sem(CMD_LED);
=======
    SemaphoreHandle_t cmd_sem = protocol_get_ctrl_sem(2);
>>>>>>> 725ef28d47adc0ab91e8af5292927ebd8cbcc7e3

    if (cmd_sem == NULL) {
        ESP_LOGE("TASK", "No se pudo obtener el semáforo 2");
        vTaskDelete(NULL);
    }

    int estado = 0;

    while(1){
        if (xSemaphoreTake(cmd_sem, portMAX_DELAY) == pdTRUE) {
        
            estado = !estado;
            gpio_set_level(LED_INTEGRADO, estado);
<<<<<<< HEAD
            composer(CMD_LED,0,NULL,NULL);    //Envio confirmacion comando 2 sin longitud sin payload sin semaforo
=======
            composer(2,0,NULL,NULL);    //Envio confirmacion comando 2 sin longitud sin payload sin semaforo
>>>>>>> 725ef28d47adc0ab91e8af5292927ebd8cbcc7e3
        }                               //Nota, crear composer con diferentes n de parametros
    }
}

void CMD3_SLOW_task(void *pvParameters) { 

    //Agarro el semaforo correspondiente
<<<<<<< HEAD
    SemaphoreHandle_t cmd_sem = protocol_get_ctrl_sem(CMD_SLOW);
=======
    SemaphoreHandle_t cmd_sem = protocol_get_ctrl_sem(3);
>>>>>>> 725ef28d47adc0ab91e8af5292927ebd8cbcc7e3

    if (cmd_sem == NULL) {
        ESP_LOGE("TASK", "No se pudo obtener el semáforo 3");
        vTaskDelete(NULL);
    }

    

    while(1){
        if (xSemaphoreTake(cmd_sem, portMAX_DELAY) == pdTRUE) {
            
            vTaskDelay(pdMS_TO_TICKS(10000));
<<<<<<< HEAD
            composer(CMD_SLOW,0,NULL,NULL);    
=======
            composer(3,0,NULL,NULL);    
>>>>>>> 725ef28d47adc0ab91e8af5292927ebd8cbcc7e3
        }   
    }
}

void CMD100_INVERTER_task(void *pvParameters) { 

    //Agarro el buffer correspondiente
<<<<<<< HEAD
    MessageBufferHandle_t buff = cmd_buff_getter(CMD_INVERTER);
=======
    MessageBufferHandle_t buff = cmd_buff_getter(100);
>>>>>>> 725ef28d47adc0ab91e8af5292927ebd8cbcc7e3
    //Vreo semaforo de seguridad
    SemaphoreHandle_t sem = xSemaphoreCreateBinary();

    xSemaphoreGive(sem);    //Lo dejo libre

    if (buff == NULL) {
        ESP_LOGE("TASK", "No se pudo obtener el buffer 100");
        vTaskDelete(NULL);
    }

    uint8_t entry_buffer[PROTOCOL_MAX_PAYLOAD_SIZE];
    uint8_t exit_buffer[PROTOCOL_MAX_PAYLOAD_SIZE];
    int n;

    while(1){
        n = xMessageBufferReceive(buff,entry_buffer,PROTOCOL_MAX_PAYLOAD_SIZE,portMAX_DELAY);

        for (int i =0; i<n; i++){
            exit_buffer[i] = entry_buffer[n-1-i];
        }

<<<<<<< HEAD
        composer(CMD_INVERTER,n,exit_buffer,sem);   //El composer toma el semaforo
=======
        composer(100,n,exit_buffer,sem);   //El composer toma el semaforo
>>>>>>> 725ef28d47adc0ab91e8af5292927ebd8cbcc7e3

        xSemaphoreTake(sem,portMAX_DELAY);  //Intento tomar, es decir bloqueo la salida hasta el envio
        xSemaphoreGive(sem);    //Cuando el dispacer entrego, puedo retomar, esto pq el buffer se pasa por puntero y no por copia
    }
}