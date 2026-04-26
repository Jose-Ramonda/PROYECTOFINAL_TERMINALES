/*
*   Archivo de cabecera de manager de lectura de datos NFC mediante sensor pn532
*   Se nutre de comoponente "garag__esp-idf-pn532"
*   Autor: José Ramonda
*   Ultima actualización: 22/4/2026
*/


//Modo de conectividad: I2C 
// I2C mode needs only SDA, SCL and IRQ pins. RESET pin will be used if valid.
// IRQ pin can be used in polling mode or in interrupt mode. Use menuconfig to select mode.
#define SCL_PIN    (22)
#define SDA_PIN    (21)
#define RESET_PIN  (23)
#define IRQ_PIN    (-1)

//Comandos de NTAG 21X que no existen en la librería
#define NTAG_COMMAND_READ_CNT 0x39  //Comando leer contador
#define NTAG_ADDR_CNT   0x02        //Dirección del contador

//Funciones/tareas públicas

void nfc_task( void *pvParameters);