/*
*   Archivo de programa de manager de lectura de datos NFC mediante sensor pn532
*   Se nutre de comoponente "garag__esp-idf-pn532"
*   Autor: José Ramonda
*   Ultima actualización: 22/4/2026
*/


#include "app_NFC.h"

#include <stdio.h>
#include <stdlib.h>
#include <esp_log.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"


#include "sdkconfig.h"
#include "pn532_driver_i2c.h"
#include "pn532.h"

#include "mbedtls/md.h" //Librería para calcular HASH
#include "mbedtls/sha256.h"

#include "config.h"
#include "protocol.h"

static uint8_t modo;
#define MODO_LECT 0
#define MODO_PROG 1
static SemaphoreHandle_t smph = NULL;

static const char *TAG = "ntag_read";

static pn532_io_t pn532_io; //Handler del nfc

static TaskHandle_t xHandleLectora = NULL;
static TaskHandle_t xHandleProgramadora = NULL;

static const char *clave_hash = "SI_FUERAS_MI_EMPLEADO_TE_DESPIDO"; //Clave ficticia por seguridad  

void get_hmac(const uint8_t *uid, const uint8_t *key, size_t key_len, uint8_t *out_bytes) {
    uint8_t hmac_result[32]; // HASH Resultado
    
    // 1. Obtenemos la información del algoritmo (SHA-256)
    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);

    // 2. Ejecutamos HMAC
    // Argumentos: info, clave, largo_clave, mensaje (UID), largo_mensaje, salida
    mbedtls_md_hmac(md_info, key, key_len, uid, 7, hmac_result);

    // 3. Truncamos los 6 bytes (4 para PWD, 2 para PACK)
    memcpy(out_bytes, hmac_result + HASH_OFFSET, 6);
}

esp_err_t pn532_send_command_wait_answer(pn532_io_handle_t io_handle, const uint8_t *cmd, uint8_t cmd_length, uint8_t *ret, uint8_t ret_length, int32_t timeout){
    esp_err_t err;

    err = pn532_send_command_wait_ack(io_handle, cmd, cmd_length, timeout);
    if(err != ESP_OK) return err;

    err = pn532_wait_ready(io_handle,timeout);
    if(err != ESP_OK) return err;

    err = pn532_read_data(io_handle, ret, ret_length, timeout);
    return err;
}

void nfc_config_task( void *pvParameters){ 
    esp_err_t err;

    if(pn532_io.pn532_read == NULL){    //Verficar el handler del pn532
        ESP_LOGE(TAG,"NO PN532 NO INICIALIZADO");
        vTaskDelete(NULL);
    }else{
        //printf("NFC STARTEANDO MODO CONFIG\n");
    }

     while (1)
    {
        uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
        uint8_t uid_length;                     // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
        uint8_t hash[6];    //Aca guardamos el hash con el pwd y el pack
        // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
        // 'uid' will be populated with the UID, and uid_length will indicate
        // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
        err = pn532_read_passive_target_id(&pn532_io, PN532_BRTY_ISO14443A_106KBPS, uid, &uid_length, 0);//Esperar tag
        vTaskDelay(pdMS_TO_TICKS(50));
        if (ESP_OK == err && modo == MODO_PROG)
        {
            // Display some basic information about the card //Debug
            ESP_LOGI(TAG, "\nFound an ISO14443A card");
            ESP_LOGI(TAG, "UID Length: %d bytes", uid_length);
            ESP_LOGI(TAG, "UID Value:");
            ESP_LOG_BUFFER_HEX_LEVEL(TAG, uid, uid_length, ESP_LOG_INFO);
            
            if(uid_length == 7){
                err = pn532_in_list_passive_target(&pn532_io);
                if (err != ESP_OK) {
                    ESP_LOGI(TAG, "Failed to inList passive target");
                    continue;
                }else ESP_LOGI(TAG,"Sesion activa");
            }else{
                ESP_LOGE(TAG,"NO ES UNA NTAG");
                continue;
            }
            
            get_hmac(uid, (uint8_t*)clave_hash, strlen(clave_hash), hash);
            printf("PWD: %02X %02X %02X %02X  PACK: %02X %02X \n",hash[0],hash[1],hash[2],hash[3],hash[4],hash[5]);
            
            uint8_t cmd[7]; //Siempre tenes comando pn532 | comando ntag | add | contenido x4
            uint8_t retorno[15];

            

            cmd[0] = PN532_COMMAND_INCOMMUNICATETHRU;   //in communicate
            cmd[1] = NTAG_COMMAND_WRITE;                //Vamos a escribir siempre
            
            NTAG2XX_MODEL ntag_model = NTAG2XX_UNKNOWN;
            err = ntag2xx_get_model(&pn532_io, &ntag_model);
            if (err != ESP_OK){
                ESP_LOGE(TAG,"ERROR-NO ES una NTAG");
                continue;
            }
            
            switch (ntag_model) {   //Segun modelo apuntamos a la ultima direccion, la del PACK
                case NTAG2XX_NTAG213:
                    
                    ESP_LOGI(TAG, "found NTAG213 target (or maybe NTAG203)");
                    cmd[2] = NTAG213_ADDR_PACK;
                    break;

                case NTAG2XX_NTAG215:
                    
                    ESP_LOGI(TAG, "found NTAG215 target");
                    cmd[2] = NTAG215_ADDR_PACK;
                    break;

                case NTAG2XX_NTAG216:
                    
                    ESP_LOGI(TAG, "found NTAG216 target");
                    cmd[2] = NTAG216_ADDR_PACK;
                    break;

                default:
                    ESP_LOGI(TAG, "Found unknown NTAG target!");
                    continue;
            }
            //uint8_t aut_cmd[]={PN532_COMMAND_INCOMMUNICATETHRU, NTAG_COMMAND_PWD_AUTH,hash[0],hash[1],hash[2],hash[3]};
            uint8_t aut_cmd[]={PN532_COMMAND_INCOMMUNICATETHRU, NTAG_COMMAND_PWD_AUTH,0xFF,0xFF,0xFF,0xFF};
            //PRobamos autenticar por las dudas
            err = pn532_send_command_wait_answer(&pn532_io, aut_cmd, 6, retorno, 15, 100);
            if(err != ESP_OK){
                ESP_LOGE(TAG,"ERROR al confiurar página %02X", cmd[2]);
                continue;
            }else{
                ESP_LOGW(TAG, "--- TRAMA PURA DEVUELTA POR LA TARJETA  AL AUTENTICAR");
                ESP_LOG_BUFFER_HEX_LEVEL(TAG, retorno, 15, ESP_LOG_INFO);

            }
            //Luego escribimos y vamos restando 1
            //PACK
            memcpy(&cmd[3], (uint8_t[]){ hash[4], hash[5],NTAG_BYTE_RFUI, NTAG_BYTE_RFUI }, 4);
            err = pn532_send_command_wait_answer(&pn532_io, cmd, 7, retorno, 15, 100);
            if(err != ESP_OK){
                ESP_LOGE(TAG,"ERROR al confiurar página %02X", cmd[2]);
                continue;
            }else{
                ESP_LOGW(TAG, "--- TRAMA PURA DEVUELTA POR LA TARJETA  AL ESCRIBIR PACK");
                ESP_LOG_BUFFER_HEX_LEVEL(TAG, retorno, 15, ESP_LOG_INFO);

            }

            //PWD
            cmd[2]--;
            memcpy(&cmd[3], (uint8_t[]){ hash[0], hash[1],hash[2], hash[3] }, 4);
            err = pn532_send_command_wait_answer(&pn532_io, cmd, 7, retorno, 15, 100);
            if(err != ESP_OK){
                ESP_LOGE(TAG,"ERROR al confiurar página %02X", cmd[2]);
                continue;
            }else{
                ESP_LOGW(TAG, "--- TRAMA PURA DEVUELTA POR LA TARJETA  AL ESCRIBIR PWD");
                ESP_LOG_BUFFER_HEX_LEVEL(TAG, retorno, 15, ESP_LOG_INFO);

            }
            //ACCES
            cmd[2]--;
            memcpy(&cmd[3], (uint8_t[]){ NTAG_BYTE_ACCES, NTAG_BYTE_RFUI,NTAG_BYTE_RFUI, NTAG_BYTE_RFUI}, 4);
            err = pn532_send_command_wait_answer(&pn532_io, cmd, 7, retorno, 15, 100);
            if(err != ESP_OK){
                ESP_LOGE(TAG,"ERROR al confiurar página %02X", cmd[2]);
                continue;
            }else{
                ESP_LOGW(TAG, "--- TRAMA PURA DEVUELTA POR LA TARJETA  AL ESCRIBIR ACCES");
                ESP_LOG_BUFFER_HEX_LEVEL(TAG, retorno, 15, ESP_LOG_INFO);

            }
            //MIRROR
            cmd[2]--;
            memcpy(&cmd[3], (uint8_t[]){ NTAG_BYTE_MIRROR, NTAG_BYTE_RFUI,NTAG_BYTE_MIRROR_PAGE, NTAG_BYTE_AUTH0}, 4);
            err = pn532_send_command_wait_answer(&pn532_io, cmd, 7, retorno, 15, 100);
            if(err != ESP_OK){
                ESP_LOGE(TAG,"ERROR al confiurar página %02X", cmd[2]);
                continue;
            }else{
                ESP_LOGW(TAG, "--- TRAMA PURA DEVUELTA POR LA TARJETA  AL ESCRIBIR MIRROR");
                ESP_LOG_BUFFER_HEX_LEVEL(TAG, retorno, 15, ESP_LOG_INFO);

            }
            ESP_LOGI(TAG,"Configuración exitosa :)");
            
            vTaskDelay(1000 / portTICK_PERIOD_MS); 

            if(modo == MODO_LECT){
            vTaskResume(xHandleLectora);   //Arranca la otra
            vTaskSuspend(NULL);     //Se va a dormir
            }
        }
    }
}
void nfc_task( void *pvParameters){ //Reemplaza al app_main del ejemplo
    
    esp_err_t err;

// Este aviso sale siempre al iniciar la tarea
    composer(0x97, 0, NULL, NULL); 
    ESP_LOGI(TAG, "init PN532 in I2C mode");

    do {
        // 1. Creamos el driver limpio en cada intento
        //err = pn532_new_driver_i2c(SDA_PIN, SCL_PIN, RESET_PIN, IRQ_PIN, 0, &pn532_io);
        err = pn532_new_driver_i2c(DUMMY_PIN,DUMMY_PIN, RESET_PIN, IRQ_PIN, 0, &pn532_io);
        if (err == ESP_OK) {
            // 2. Intentamos inicializar el chip físico
            err = pn532_init(&pn532_io);
            
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "failed to initialize PN532, liberando driver...");
                pn532_release(&pn532_io); // Liberamos el driver viejo antes de reintentar
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
        } else {
            ESP_LOGE(TAG, "No se pudo crear el driver I2C");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

    } while(err != ESP_OK); // Solo sale de acá si el PN532 respondió con éxito

    // RECIÉN ACÁ EL CHIP RESPONDIÓ EL SALUDO
    smph = xSemaphoreCreateBinary();

    ESP_LOGI(TAG, "Waiting for an ISO14443A Card ...");
    composer(0x99, 0, NULL, NULL);    // <--- Ahora sí te va a llegar este paquete

  
    xTaskCreate(nfc_config_task, "nfc_config", 4096, NULL, 5, &xHandleProgramadora);
    while (1)
    {
        
        uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
        uint8_t uid_length;   // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
        uint8_t hash[6];    //Aca guardamos el hash con el pwd y el pack

        // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
        // 'uid' will be populated with the UID, and uid_length will indicate
        // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
        err = pn532_read_passive_target_id(&pn532_io, PN532_BRTY_ISO14443A_106KBPS, uid, &uid_length,1000);//Esperar tag
        vTaskDelay(pdMS_TO_TICKS(50));
        if (ESP_OK == err && modo == MODO_LECT)
        {
            composer(0x95,0,NULL,NULL); 
            // Display some basic information about the card //Debug
            ESP_LOGI(TAG, "\nFound an ISO14443A card");
            ESP_LOGI(TAG, "UID Length: %d bytes", uid_length);
            ESP_LOGI(TAG, "UID Value:");
            ESP_LOG_BUFFER_HEX_LEVEL(TAG, uid, uid_length, ESP_LOG_INFO);
            composer(CMD_UID,7,uid,NULL);
            //gpio_set_level(4, 1);
            if(uid_length == 7){
                err = pn532_in_list_passive_target(&pn532_io);
                if (err != ESP_OK) {
                    ESP_LOGI(TAG, "Failed to inList passive target");
                    continue;
                }else ESP_LOGI(TAG,"Sesion activa");
            }else{
                ESP_LOGE(TAG,"NO ES UNA NTAG");
                continue;
            }

            vTaskDelay(pdMS_TO_TICKS(20)); // Respiro
            
            get_hmac(uid, (uint8_t*)clave_hash, strlen(clave_hash), hash);
            //printf("PWD: %02X %02X %02X %02X  PACK: %02X %02X \n",hash[0],hash[1],hash[2],hash[3],hash[4],hash[5]);
            
            uint8_t cmd[]={PN532_COMMAND_INCOMMUNICATETHRU, NTAG_COMMAND_PWD_AUTH,hash[0],hash[1],hash[2],hash[3], 0x00};
            //uint8_t cmd[]={PN532_COMMAND_INCOMMUNICATETHRU, NTAG_COMMAND_PWD_AUTH,0xFF,0xFF,0xFF,0xFF, 0x00};
            uint8_t retorno[15];

            err = pn532_send_command_wait_answer(&pn532_io, cmd, 6, retorno, 15, 100);
            if(err != ESP_OK){
                ESP_LOGE(TAG,"ERROR al validar");
                continue;
            }else{
                ESP_LOGW(TAG, "--- TRAMA PURA DEVUELTA POR LA TARJETA  AL AUTENTICAR");
                ESP_LOG_BUFFER_HEX_LEVEL(TAG, retorno, 15, ESP_LOG_INFO);
                if(retorno[8] == hash[4] && retorno[9] == hash[5]){
                    ESP_LOGI(TAG,"EXITO DE VALIDACION ;)");
                } else ESP_LOGE(TAG,"ERROR DE VALIDACION");
            }

            //LEER MIRROR DEL CONTADOR
            cmd[1]=NTAG_COMMAND_FAST_READ;
            cmd[2]=NTAG_ADDR_MIRROR_CNT;
            cmd[3]=NTAG_ADDR_MIRROR_CNT +1;
            err = pn532_send_command_wait_answer(&pn532_io, cmd, 4, retorno, 15, 100);
            if(err != ESP_OK){
                ESP_LOGE(TAG,"ERROR al obtener contador");
                continue;
            }else{
                ESP_LOGW(TAG, "--- TRAMA PURA DEVUELTA POR LA TARJETA  AL LEER contador");
                ESP_LOG_BUFFER_HEX_LEVEL(TAG, retorno, 15, ESP_LOG_INFO);
            }
            // Si el mirror ocupa 6 bytes y empieza en retorno[8]
            uint32_t contador_num = 0;
            // Si el mirror ocupa 6 bytes, el buffer debe ser de 7 (6 + el terminador nulo \0)
            char buffer_temporal[7]; 
            memset(buffer_temporal, 0, sizeof(buffer_temporal)); // Aseguramos que termine en \0

            // Copiamos los 6 caracteres desde el offset 8 de tu retorno
            memcpy(buffer_temporal, &retorno[8], 6);

            // strtol espera (const char*), así que casteamos para que el compilador no llore
            long valor_mirror = strtol((const char*)buffer_temporal, NULL, 10);


            // Si querés usar printf a secas:
            //printf("Valor numérico del Mirror: %ld\n", valor_mirror);
            // Suponiendo que buffer_temporal tiene "14\0" o "000014\0"
            // Usamos base 16 para que interprete el string como Hexadecimal
            uint32_t valor_mirror_le = (uint32_t)strtol((const char*)buffer_temporal, NULL, 16);

                        contador_num = (uint32_t)retorno[8]         | 
                        ((uint32_t)retorno[9] << 8)  | 
                        ((uint32_t)retorno[10] << 16);

            //printf("VALOR DEL CONTADOR pasado: %lu\n", contador_num);


            //Probamos obtener contador por getcounter
            cmd[1]=NTAG_COMMAND_READ_CNT;
            cmd[2]=NTAG_ADDR_CNT;

            err = pn532_send_command_wait_answer(&pn532_io, cmd, 3, retorno, 15, 100);
            if(err != ESP_OK){
                ESP_LOGE(TAG,"ERROR al obtener contador");
                continue;
            }else{
                ESP_LOGW(TAG, "--- TRAMA PURA DEVUELTA POR LA TARJETA  AL obtener contador");
                ESP_LOG_BUFFER_HEX_LEVEL(TAG, retorno, 15, ESP_LOG_INFO);
            }
            // Suponiendo que retorno[8] es el primer byte del contador (C0)
            

            contador_num = (uint32_t)retorno[8]         | 
                        ((uint32_t)retorno[9] << 8)  | 
                        ((uint32_t)retorno[10] << 16);

            //printf("VALOR DEL CONTADOR: %lu\n", contador_num);

            

            vTaskDelay(1000 / portTICK_PERIOD_MS); 
        }

        if(modo == MODO_PROG){
            vTaskResume(xHandleProgramadora);   //LE dice ya me suspendí, podes arrancar la otra
            vTaskSuspend(NULL);     //Se va a dormir
        }
    } 
        
}

void nfc_arbitraje_task(void *pvParameters){
    SemaphoreHandle_t cmd_sem = protocol_get_ctrl_sem(CMD_PROGMODE);


    if (cmd_sem == NULL) {
        ESP_LOGE("TASK", "No se pudo obtener el semáforo de erbitraje NFC");
        vTaskDelete(NULL);
    }

    TickType_t timeout = portMAX_DELAY; //Esto del timeout variable del semaforo
    //Es un magia que tiró chatgpt, me saco el sombrero

    while (1){
        if (xSemaphoreTake(cmd_sem, timeout) == pdTRUE) {
            if(modo == MODO_LECT){
                modo = MODO_PROG;
                timeout=pdMS_TO_TICKS(PROG_TIMEOUT);
                composer(CMD_PROGMODE,0,NULL,NULL);
            } else if(modo == MODO_PROG){
                modo = MODO_LECT;
                timeout= portMAX_DELAY;
                composer(CMD_PROGMODE,0,NULL,NULL);
            }
        }else{
            if(modo == MODO_PROG){
                modo = MODO_LECT;
                timeout= portMAX_DELAY;
                composer(CMD_PROGMODE,0,NULL,NULL);
            }
        }
        
    }
}


void nfc_init(void){
    xTaskCreate(nfc_task,"NFC",4096,NULL,7,&xHandleLectora);
   
    xTaskCreate(nfc_arbitraje_task,"ARBITRO",1024,NULL,4,NULL);
}