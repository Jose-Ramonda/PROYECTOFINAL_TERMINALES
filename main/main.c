
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "driver/gpio.h"

#include "config.h"
#include "app_uart.h"
#include "protocol.h"
#include "SEAII.h"
<<<<<<< HEAD
#include "app_wifi.h"
#include "app_NFC.h"
=======

>>>>>>> 725ef28d47adc0ab91e8af5292927ebd8cbcc7e3

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

<<<<<<< HEAD
    //set_credenciales("aaa", "12345678");
    xTaskCreate(app_wifi_com_task,"WIFI",4096,NULL,3,NULL);

    xTaskCreate(nfc_task,"NFC",4096,NULL,3,NULL);

=======
>>>>>>> 725ef28d47adc0ab91e8af5292927ebd8cbcc7e3
    printf("Arrancamos:\n\r");
    


}
   

