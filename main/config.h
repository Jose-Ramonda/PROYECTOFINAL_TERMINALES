/*
*   Archivo de parametros de configuración y macros globales
*   Contenidos:
*       UART
*
*   Autor: José Ramonda
*   Actualizado: 23/12/2025
*/



#pragma once

//Comunicaciones eidentificación

#define MASTER_ID 0            // ID del maestro
#define NODO_ID_DEFAULT 10          // ID de este nodo por ahora

//Parámetros de red
#define WIFI_RETRY_NUM 10


//NUMEROS DE COMANDO

//Control 
#define CMD_ACK 0
#define CMD_NACK 1
#define CMD_LED 2
#define CMD_SLOW 3
#define CMD_WIFI_FAIL 4
#define CMD_RESET 5
#define CMD_PROGMODE 6


//Flujo
#define CMD_INVERTER 100
#define CMD_WIFI 101
#define CMD_UID 102
