/*
*   Archivo de programa de manager de conexión WiFI
*   Basado en ejemplo wifi/getting_started/station_example_main.c
*   Autor: José Ramonda
*   Ultima actualización: 10/4/2026
*/


#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "freertos/message_buffer.h"

#include "app_wifi.h"
#include "config.h"
#include "protocol.h"
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;   //Event group de freertos, es como un arreglo de flags

#define WIFI_CONNECTED_BIT BIT0 
#define WIFI_FAIL_BIT      BIT1
#define WIFI_SSIDTOCHANGE_BIT BIT2
#define WIFI_PASSWORDTOCHANGE_BIT BIT3
#define WIFI_CREDENCIALSCHANGED_BIT BIT4
#define WIFI_MUSTBE_BIT BIT5
#define WIFI_SAVE_CREDENTIALS_BIT BIT6
#define WIFI_RETRYING_BIT BIT7


static uint8_t val = WIFI_FAIL_MSJ;
uint8_t ipmsj[5];
static const char *TAG = "wifi station"; 

static int s_retry_num = 0;


static uint8_t nombre_red[32];  //Credenciales de acceso
static uint8_t clave[64]; 

void set_credenciales(const char *name, const char *clv) {
    ESP_LOGI("TAG","CAMBIANDO CREDENCIALES");
    memset(nombre_red, 0, sizeof(nombre_red));
    memset(clave, 0, sizeof(clave));
    
    if (name) strncpy((char*)nombre_red, name, sizeof(nombre_red) - 1);
    if (clv) strncpy((char*)clave, clv, sizeof(clave) - 1);
}
void wifi_save_credentials(void) {
    nvs_handle_t nvs;
    ESP_LOGI("TAG","Guardando CREDENCIALES");
    if (nvs_open("wifi", NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_str(nvs, "ssid", (char*)nombre_red);
        nvs_set_str(nvs, "pass", (char*)clave);
        nvs_commit(nvs);
        nvs_close(nvs);

        ESP_LOGI(TAG, "Credenciales guardadas en NVS");
        
    } else {
        ESP_LOGE(TAG, "Error abriendo NVS para guardar");
    }
    xEventGroupClearBits(s_wifi_event_group,WIFI_SAVE_CREDENTIALS_BIT);
}

void app_wifi_conect(){     //Intenta conectar 10 veces
    if (s_retry_num < WIFI_RETRY_NUM && nombre_red[0] != '\0' && nombre_red[0] != ' ') {
            esp_wifi_connect();
            s_retry_num++;
            //xEventGroupSetBits(s_wifi_event_group,WIFI_RETRYING_BIT);
            ESP_LOGI(TAG, "retry to connect to the AP, n %d",s_retry_num);
        } else {
            xEventGroupClearBits(s_wifi_event_group,WIFI_RETRYING_BIT);
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);  //Quitar, inncesario, mandar desde aqui
            composer(CMD_WIFI,1,&val,NULL);
            ESP_LOGE(TAG,"FALLO AL CONECTAR WIFI");//Aca hacer otra cosa mas que un log
        }
}

static void event_handler(void* arg, esp_event_base_t event_base,   //modificado de ejemplo
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) { 

        xEventGroupSetBits(s_wifi_event_group,WIFI_RETRYING_BIT); //Ante un start intento conectar

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);   //Si desconecta limpio la flag
        EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
        ESP_LOGI("TAG","EVENTO DISCONECTED");
        if (bits & WIFI_MUSTBE_BIT){    //Si se desconect y deberia estar conectado
            xEventGroupSetBits(s_wifi_event_group,WIFI_RETRYING_BIT); //Intenta reconectar
            ESP_LOGI("TAG","EVENTO DISCONECTED ento al if");
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupClearBits(s_wifi_event_group,WIFI_RETRYING_BIT);
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT); //Solo levanto con Ip
        //Enviar comando de confirmacion con ip
        

        ipmsj[0] = 0;

        uint32_t ip = event->ip_info.ip.addr;

        ipmsj[1] = ip & 0xFF;
        ipmsj[2] = (ip >> 8) & 0xFF;
        ipmsj[3] = (ip >> 16) & 0xFF;
        ipmsj[4] = (ip >> 24) & 0xFF;

        composer(CMD_WIFI, 5, ipmsj, NULL);
        EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
        if (bits & WIFI_CREDENCIALSCHANGED_BIT){    //Estado startet+disconected->reconect
            xEventGroupClearBits(s_wifi_event_group,WIFI_CREDENCIALSCHANGED_BIT);
            xEventGroupSetBits(s_wifi_event_group,WIFI_SAVE_CREDENTIALS_BIT);
        }
    }
}

void app_wifi_reconfig(){
    ESP_LOGI("TAG","Probando recambiar CREDENCIALES");
    esp_wifi_disconnect(); //Por las dudas desconectamos

    wifi_config_t wifi_config = {0};    //Extraido del init del ejemplo
    //Modificado, paso las credenciales de mis variables globales
    strncpy((char*)wifi_config.sta.ssid, (char*)nombre_red, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, (char*)clave, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    xEventGroupSetBits(s_wifi_event_group, WIFI_CREDENCIALSCHANGED_BIT);    //Levanto para reescribir en flash
    //Reviso el estado y reinicio/reconecto siempre que se cambien las credenciales
    EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
    if (bits & WIFI_MUSTBE_BIT){    //Estado startet+disconected->reconect
        app_wifi_conect();  //inteto conectar
    }
    else{
    xEventGroupSetBits(s_wifi_event_group, WIFI_MUSTBE_BIT);//seteo el estado correcto
    ESP_ERROR_CHECK(esp_wifi_start()); //Esto dispara el evento que llama  a conect
    }

}

void app_wifi_task(void *pvParameters){  //Reemplaza al app main del ejemplo
    while(1){
        EventBits_t bits = xEventGroupWaitBits(
            s_wifi_event_group,
            WIFI_SAVE_CREDENTIALS_BIT | WIFI_RETRYING_BIT, // ← ambos
            pdTRUE,        // limpia los que despertaron
            pdFALSE,       // ANY → cualquiera alcanza
            portMAX_DELAY
        );

        if (bits & WIFI_SAVE_CREDENTIALS_BIT) {
            wifi_save_credentials();
        }
        if (bits & WIFI_RETRYING_BIT) {
            app_wifi_conect();
        }
    }    
}

void wifi_init_sta(void){    //Codigo del ejemplo modificado

    //Init del event group
    s_wifi_event_group = xEventGroupCreate();

    //Initialize NVS / init de la flash
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);   //Verifico porlas


    //Cargar lo que hay en flash a  la ram
    nvs_handle_t nvs;
    if (nvs_open("wifi", NVS_READONLY, &nvs) == ESP_OK) {
        size_t ssid_len = sizeof(nombre_red);
        size_t pass_len = sizeof(clave);

        memset(nombre_red, 0, sizeof(nombre_red));
        memset(clave, 0, sizeof(clave));
        if (nvs_get_str(nvs, "ssid", (char*)nombre_red, &ssid_len) != ESP_OK) {
            nombre_red[0] = '\0';
        }

        if(nvs_get_str(nvs, "pass", (char*)clave, &pass_len) != ESP_OK){
            clave[0]='\0';
        }

        nvs_close(nvs);
    }

    //Inits del wifi
    ESP_ERROR_CHECK(esp_netif_init());  //esp-inicia funciones de red (stack TCP-IP)

    ESP_ERROR_CHECK(esp_event_loop_create_default());   //esp inicializa bus de eventos
    esp_netif_create_default_wifi_sta();    //preaprara para usar esp como cliente de una red

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); //estructura de config de wifi, dejamos default
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));           //inicializa segun configs

    //Init de los eventos de sistema
    esp_event_handler_instance_t instance_any_id;   //declaro handlers de eventos
    esp_event_handler_instance_t instance_got_ip;

    //Y los creo con el "create"
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, //Reaccionar a evento de wifi
                                                        ESP_EVENT_ANY_ID,   //cualquiera que sea
                                                        &event_handler,    //dispara la func handler
                                                        NULL,   //Sin otros argumentos
                                                        &instance_any_id)); //Esto que apunta no se
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,   //Evento IP
                                                        IP_EVENT_STA_GOT_IP,    //Solo el caso de obtenr io
                                                        &event_handler, //Mismo reactor
                                                        NULL,   //Mismo argumento
                                                        &instance_got_ip)); //misma duda


    //DE nuevo wifi, aca cargo las credenciales que tome de ram, desordenado pero como en el ejemplo                                                 
    wifi_config_t wifi_config = {0};
    //Modificado, paso las credenciales de mis variables globales
    strncpy((char*)wifi_config.sta.ssid, (char*)nombre_red, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, (char*)clave, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    
    xTaskCreate(app_wifi_task,"credenciadora",4096,NULL,2,NULL);    //Si le bajo el stack cague
}



void app_wifi_com_task(void *pvParameters){
    //Agarro el buffer correspondiente para comunicar y sincronizar desde el protocolo
    MessageBufferHandle_t buff = cmd_buff_getter(CMD_WIFI);
    uint8_t entry_buffer[PROTOCOL_MAX_PAYLOAD_SIZE];
    int n;

    //Buffers auxiliares de credenciales
    uint8_t aux_nombre_red[32];  //Credenciales de acceso
    uint8_t aux_clave[64]; 


    int max_data_per_chunk = PROTOCOL_MAX_PAYLOAD_SIZE - 3; // Lo que sobra para datos

    wifi_init_sta();
    while (1){

        
         //Esto esta bloqueado hasta que el master hable, ya sea como respuesta a una no-conexión que ennviamos antes o proque quizo cambiar las credenciales
        n = xMessageBufferReceive(buff, entry_buffer, PROTOCOL_MAX_PAYLOAD_SIZE, portMAX_DELAY);
        
        
        ESP_LOGI(TAG, " LLego %d RAW: %d %d %d %d %d %d %d %d %d %d", n,
                                                        entry_buffer[0],
                                                        entry_buffer[1],
                                                        entry_buffer[2],
                                                        entry_buffer[3],
                                                        entry_buffer[4],
                                                        entry_buffer[5],
                                                        entry_buffer[6],
                                                        entry_buffer[7],
                                                        entry_buffer[8],
                                                        entry_buffer[9]);
                                                        

        // Aseguramos que llegó al menos el encabezado completo
        if (n >= 3) {
            uint8_t tipo         = entry_buffer[0]; // 1 = SSID, 2 = Clave
            uint8_t chunk_actual = entry_buffer[1]; // Posición (0, 1, 2...)
            uint8_t total_chunks = entry_buffer[2]; // Cantidad total
            uint8_t data_len     = n - 3;           // Cuántos bytes de texto real llegaron

            // Calculamos en qué parte del arreglo local va este pedazo
            uint16_t offset = (chunk_actual - 1) * max_data_per_chunk;
            
            switch (tipo) {
                case WIFI_CHNG_SSID_MSJ: // Modifico SSID
                    ESP_LOGI(TAG,"CAMBIANDO SSID");
                    if (chunk_actual == 1) {
                        memset(aux_nombre_red, 0, sizeof(aux_nombre_red)); // Limpio todo en el primer mensaje
                    }
                    
                    // Verifico que no me pase del tamaño máximo de la variable
                    if ((offset + data_len) < sizeof(aux_nombre_red)) {
                        memcpy(&aux_nombre_red[offset], &entry_buffer[3], data_len); 
                    }

                    //Si terminé levanto la flag de que hay una version nueva
                    if(chunk_actual == total_chunks){
                        xEventGroupSetBits(s_wifi_event_group, WIFI_SSIDTOCHANGE_BIT);
                    }
                    break;

                case WIFI_CHNG_PASS_MSJ: // Modifico Contraseña
                    ESP_LOGI(TAG,"CAMBIANDO CONTRA");
                    if (chunk_actual == 1) {
                        memset(aux_clave, 0, sizeof(aux_clave)); // Limpio todo en el primer mensaje
                    }
                    
                    if ((offset + data_len) < sizeof(clave)) {
                        memcpy(&aux_clave[offset], &entry_buffer[3], data_len);
                    }
                    //Si terminé levanto la flag de que hay una version nueva
                    if(chunk_actual == total_chunks){
                        xEventGroupSetBits(s_wifi_event_group, WIFI_PASSWORDTOCHANGE_BIT);
                    }
                    break;

                case WIFI_PRENDER_MSJ: //Manda  aencender/reconectar
                    ESP_LOGI("TAG","MANDADO A CONECTAR");
                    s_retry_num =0; //Siempre reinicio para intentar conectar
                    EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
                    if (bits & WIFI_MUSTBE_BIT){    //Estado startet+disconected->reconect
                        app_wifi_conect();  //inteto conectar
                    }
                    else{
                        xEventGroupSetBits(s_wifi_event_group, WIFI_MUSTBE_BIT);//seteo el estado correcto
                        ESP_ERROR_CHECK(esp_wifi_start() ); //Esto dispara el evento que llama  a conect
                        ESP_LOGI("TAG","ACA MANDE EL START");
                    }
                    break;
                
                case WIFI_APAGAR_MSJ: //Apagar para ahorro de energía
                    xEventGroupClearBits(s_wifi_event_group, WIFI_MUSTBE_BIT);  //Limpia
                    esp_wifi_disconnect();  //Desconecta
                    esp_wifi_stop();    //Apaga
                    //No pasa nada si se hace dos veces, no afecta
                    break;

                default:
                    ESP_LOGW("WIFI", "PARAMETRO DESCONOCIDO TIPO: %d", tipo);
                    break;
            }

            // Si estan por cambiarse ambas, las cambio y reconecto
            EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
            if ((bits & WIFI_PASSWORDTOCHANGE_BIT) && (bits & WIFI_SSIDTOCHANGE_BIT)) {
                set_credenciales((char*)aux_nombre_red, (char*)aux_clave);
                xEventGroupClearBits(s_wifi_event_group, WIFI_PASSWORDTOCHANGE_BIT);
                xEventGroupClearBits(s_wifi_event_group, WIFI_SSIDTOCHANGE_BIT);
                app_wifi_reconfig(); //Reveer que verifique estado pero siempre reconecte para testear credenciales
               
            }
        }
    }
    
}

