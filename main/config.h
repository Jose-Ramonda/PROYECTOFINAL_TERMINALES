/*
*   Archivo de parametros de configuración y macros globales
*   Contenidos:
*       UART
*
*   Autor: José Ramonda
*   Actualizado: 25/5/2026
*/



#pragma once


//Parámetros generales
#define TIEMPO_TIMBRE 2000
#define TIEMPO_PUERTA 5000

//Comunicaciones e identificación

#define MASTER_ID 0            // ID del maestro
#define NODO_ID_DEFAULT 10          // ID de este nodo por ahora

#define SCL_PIN    (12)         //El Bus i2c ahora es del main
#define SDA_PIN    (2)

//Parámetros de red
#define WIFI_RETRY_NUM 100


//NUMEROS DE COMANDO

//Control 
#define CMD_ACK 0
#define CMD_NACK 1
#define CMD_RESET 2
#define CMD_READY 2
#define CMD_DOOR 3
#define CMD_WIFI_FAIL 4
//Pendiente el 5
#define CMD_PROGMODE 6
#define CMD_TAKE_PH 7


//Flujo
#define CMD_NFC 100 //??
#define CMD_WIFI 101
#define CMD_UID 102
#define CMD_URL 103

//Borrar
#define CMD_LED 5
#define CMD_SLOW 8
#define CMD_INVERTER 109



//Pines GPIO
#define TIMBRE_PIN 3
#define FLASH_PIN 4
//Pines PCF8574 (no representa GPIO si no el bit que se modifica [0,7])
#define PCF_DESTRABADOR_PIN 0
#define PCF_TIMBRE_PIN 1
