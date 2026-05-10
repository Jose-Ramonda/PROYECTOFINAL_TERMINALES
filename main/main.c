
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "driver/gpio.h"

#include "config.h"
#include "app_uart.h"
#include "protocol.h"
#include "SEAII.h"

#include "app_wifi.h"
#include "app_NFC.h"
#include "camara.h"

void app_main(void){   
    
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

    xTaskCreate(CMD2_LED_task,"LED_TASK",4096,NULL,4,NULL);
    xTaskCreate(CMD3_SLOW_task,"SLOW_TASK",4096,NULL,2,NULL);
    xTaskCreate(CMD100_INVERTER_task,"INVERTER_TASK",4096,NULL,2,NULL);



    xTaskCreate(app_wifi_com_task,"WIFI",4096,NULL,3,NULL);

    //Forzar conección wifi para debug:
    
    MessageBufferHandle_t wfbuff = cmd_buff_getter(CMD_WIFI);
    uint8_t msj_falso[10] = {
    WIFI_CHNG_SSID_MSJ, // Índice 0: Comando
    0x01,               // Índice 1: Parámetro o ID
    0x01,               // Índice 2: Parámetro o ID
    'R',                // Índice 3
    'a',                // Índice 4
    'm',                // Índice 5
    'o',                // Índice 6
    'n',                // Índice 7
    'd',                // Índice 8
    'a'                 // Índice 9
    };
    xMessageBufferSend(wfbuff,(void*) msj_falso,10,portMAX_DELAY);

    msj_falso[0]= WIFI_CHNG_PASS_MSJ;
    msj_falso[2] = 0x02;
    msj_falso[3] = 'a';
    msj_falso[4] = 'z';
    msj_falso[5] = 'u';
    msj_falso[6] = 'l';
    msj_falso[7] = 'g';
    msj_falso[8] = 'r';
    msj_falso[9] = 'a';

    xMessageBufferSend(wfbuff,(void*) msj_falso,10,portMAX_DELAY);

    msj_falso[0]= WIFI_CHNG_PASS_MSJ;
    msj_falso[1] = 0x02;
    msj_falso[3] = 'n';
    msj_falso[4] = 'a';

    xMessageBufferSend(wfbuff,(void*) msj_falso,5,portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(15000));
    

    //nfc_init();
    xTaskCreate(camara_task,"CAM",8192,NULL,3,NULL);
    printf("Arrancamos:\n\r");
    

}
   

