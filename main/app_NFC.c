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
#include "pn532_driver_hsu.h"
#include "pn532_driver_spi.h"
#include "pn532.h"



static const char *TAG = "ntag_read";

void nfc_task( void *pvParameters){ //Reemplaza al app_main del ejemplo
    pn532_io_t pn532_io;
    esp_err_t err;

    uint8_t buff_cmd[]= { 0x39, 0x02 };
    //uint8_t buff_ret[20];//Ver tamaño
    //uint8_t taman = sizeof(buff_ret);
    printf("NFC STARTEANDO\n");


    vTaskDelay(1000 / portTICK_PERIOD_MS);



    ESP_LOGI(TAG, "init PN532 in I2C mode");
    ESP_ERROR_CHECK(pn532_new_driver_i2c(SDA_PIN, SCL_PIN, RESET_PIN, IRQ_PIN, 0, &pn532_io));



    
    do {
        err = pn532_init(&pn532_io);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "failed to initialize PN532");
            //pn532_release(&pn532_io);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    } while(err != ESP_OK);

    ESP_LOGI(TAG, "get firmware version");
    uint32_t version_data = 0;
    do {
        err = pn532_get_firmware_version(&pn532_io, &version_data);
        if (ESP_OK != err) {
            ESP_LOGI(TAG, "Didn't find PN53x board");
            pn532_reset(&pn532_io);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    } while (ESP_OK != err);

    // Log firmware infos
    ESP_LOGI(TAG, "Found chip PN5%x", (unsigned int)(version_data >> 24) & 0xFF);
    ESP_LOGI(TAG, "Firmware ver. %d.%d", (int)(version_data >> 16) & 0xFF, (int)(version_data >> 8) & 0xFF);

    ESP_LOGI(TAG, "Waiting for an ISO14443A Card ...");
    while (1)
    {
        uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
        uint8_t uid_length;                     // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

        // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
        // 'uid' will be populated with the UID, and uid_length will indicate
        // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
        err = pn532_read_passive_target_id(&pn532_io, PN532_BRTY_ISO14443A_106KBPS, uid, &uid_length, 0);
        vTaskDelay(pdMS_TO_TICKS(50));
        if (ESP_OK == err)
        {
            // Display some basic information about the card
            ESP_LOGI(TAG, "\nFound an ISO14443A card");
            ESP_LOGI(TAG, "UID Length: %d bytes", uid_length);
            ESP_LOGI(TAG, "UID Value:");
            ESP_LOG_BUFFER_HEX_LEVEL(TAG, uid, uid_length, ESP_LOG_INFO);
            

            err = pn532_in_list_passive_target(&pn532_io);
            if (err != ESP_OK) {
                ESP_LOGI(TAG, "Failed to inList passive target");
                continue;
            }else ESP_LOGI(TAG,"Sesion activa");
            
            /*
            NTAG2XX_MODEL ntag_model = NTAG2XX_UNKNOWN;
            err = ntag2xx_get_model(&pn532_io, &ntag_model);
            if (err != ESP_OK){
                ESP_LOGE(TAG,"ERROR-NO ES una NTAG");
                continue;
            }
            int page_max;
            switch (ntag_model) {
                case NTAG2XX_NTAG213:
                    page_max = 45;
                    ESP_LOGI(TAG, "found NTAG213 target (or maybe NTAG203)");
                    break;

                case NTAG2XX_NTAG215:
                    page_max = 135;
                    ESP_LOGI(TAG, "found NTAG215 target");
                    break;

                case NTAG2XX_NTAG216:
                    page_max = 231;
                    ESP_LOGI(TAG, "found NTAG216 target");
                    break;

                default:
                    ESP_LOGI(TAG, "Found unknown NTAG target!");
                    continue;
            }

            for(int page=0; page < page_max; page+=4) {
                uint8_t buf[16];
                err = ntag2xx_read_page(&pn532_io, page, buf, 16);
                if (err == ESP_OK) {
                    ESP_LOG_BUFFER_HEXDUMP(TAG, buf, 16, ESP_LOG_INFO);
                }
                else {
                    ESP_LOGI(TAG, "Failed to read page %d", page);
                    break;
                }
            }       //Comentamos tema proceso para probar acá
            */
            ESP_LOGI(TAG, "Sesion activa. Bypass del PN532 usando InCommunicateThru (0x42)...");

            vTaskDelay(pdMS_TO_TICKS(20)); // Respiro

            // 1. Comando InCommunicateThru (0x42) + Comando NTAG (0x39 0x02)
            uint8_t raw_cmd[] = { 0x42, 0x39, 0x02}; 
            //uint8_t raw_cmd[] = { 0x40,0x01, 0x39, 0x02 };
            //uint8_t raw_cmd[] = {0x39, 0x02};
            uint8_t raw_res[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; 
            uint8_t len = 10;
            esp_err_t err_send;
            // 2. Disparamos
            /*
            err_send = pn532_send_command_wait_ack(&pn532_io, raw_cmd, 3, 100);
            // err_send = pn532_in_data_exchange(&pn532_io,raw_cmd,3,raw_res, &len);
            if(err_send == ESP_OK) {
                vTaskDelay(pdMS_TO_TICKS(30)); 
                
                if(pn532_wait_ready(&pn532_io, 200) == ESP_OK) {
                    
                    esp_err_t err_read = pn532_read_data(&pn532_io, raw_res, 20, 100);
                    if (err_read == ESP_OK) {
                        ESP_LOGW(TAG, "--- TRAMA PURA DEVUELTA POR LA TARJETA ---");
                        ESP_LOG_BUFFER_HEX_LEVEL(TAG, raw_res, 15, ESP_LOG_INFO);
                        
                        // Si la magia ocurre, vas a ver D5 43 00 [Byte0] [Byte1] [Byte2]
                        if (raw_res[5] == 0xD5 && raw_res[6] == 0x43 && raw_res[7] == 0x00) {
                            uint32_t contador = raw_res[8] | (raw_res[9] << 8) | (raw_res[10] << 16);
                            ESP_LOGI(TAG, ">>>> CONTADOR REAL: %lu <<<<", contador);
                        } else {
                            ESP_LOGE(TAG, "El tag respondió, pero no con éxito. Status: 0x%02X", raw_res[7]);
                        }
                        
                    } else {
                        ESP_LOGE(TAG, "Fallo lectura I2C");
                    }
                } else {
                    ESP_LOGE(TAG, "Timeout esperando respuesta");
                }
            } else ESP_LOGE(TAG,"FALLO X GIL");

            */
            //LEctura para intentar leer el contador
            uint8_t cmd[] = { 0x42, 0x1B, 0xFF, 0xFF, 0xFF, 0xFF};  //Autenticar
            err_send = pn532_send_command_wait_ack(&pn532_io, cmd, 6, 100);
            //esp_err_t err_send = pn532_in_data_exchange(&pn532_io,raw_cmd,3,raw_res, &len);
            if(err_send == ESP_OK) {
                vTaskDelay(pdMS_TO_TICKS(30)); 
                
                if(pn532_wait_ready(&pn532_io, 200) == ESP_OK) {
                    
                    esp_err_t err_read = pn532_read_data(&pn532_io, raw_res, 20, 100);
                    if (err_read == ESP_OK) {
                        ESP_LOGW(TAG, "--- TRAMA PURA DEVUELTA POR LA TARJETA  autenticar---");
                        ESP_LOG_BUFFER_HEX_LEVEL(TAG, raw_res, 20, ESP_LOG_INFO);
                        
                    } else {
                        ESP_LOGE(TAG, "Fallo lectura I2C");
                    }
                } else {
                    ESP_LOGE(TAG, "Timeout esperando respuesta");
                }
            } else ESP_LOGE(TAG,"FALLO X GIL");
            
            
            //LEctura para intentar leer el contador
            uint8_t cmd1[] = { 0x42, 0x39, 0x02};  //proteger p lectura
            err_send = pn532_send_command_wait_ack(&pn532_io, cmd1, 3, 100);
            //esp_err_t err_send = pn532_in_data_exchange(&pn532_io,raw_cmd,3,raw_res, &len);
            if(err_send == ESP_OK) {
                vTaskDelay(pdMS_TO_TICKS(30)); 
                
                if(pn532_wait_ready(&pn532_io, 200) == ESP_OK) {
                    
                    esp_err_t err_read = pn532_read_data(&pn532_io, raw_res, 20, 100);
                    if (err_read == ESP_OK) {
                        ESP_LOGW(TAG, "--- TRAMA PURA DEVUELTA POR LA TARJETA  escribir---");
                        ESP_LOG_BUFFER_HEX_LEVEL(TAG, raw_res, 20, ESP_LOG_INFO);
                        
                    } else {
                        ESP_LOGE(TAG, "Fallo lectura I2C");
                    }
                } else {
                    ESP_LOGE(TAG, "Timeout esperando respuesta");
                }
            } else ESP_LOGE(TAG,"FALLO X GIL");
            
            vTaskDelay(1000 / portTICK_PERIOD_MS); 
        }
    }
}
