/*
*   Archivo de c de manejo de la camara OV2640 del módulo ESP32CAM
*   Se nutre de comoponente "espressif/esp32-camera"
*   Basado en ejemplo take_picture.c
*   Autor: José Ramonda
*   Ultima actualización: 6/5/2026
*/


#include "camara.h"



#include "sdkconfig.h"

#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_client.h"

#include "esp_camera.h"




static const char *TAG = "example:take_picture";

#if ESP_CAMERA_SUPPORTED
static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_SVGA,    //QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.

    .jpeg_quality = 12, //0-63, for OV series camera sensors, lower number means higher quality
    .fb_count = 1,       //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

static esp_err_t init_camera(void)
{
    //initialize the camera
    nvs_flash_init();
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    return ESP_OK;
}

#endif


esp_err_t send_pic(uint8_t *buffer, size_t len) {
    if (buffer == NULL || len == 0) {
        ESP_LOGE(TAG, "Buffer de imagen vacío");
        return ESP_ERR_INVALID_ARG;
    }

    esp_http_client_config_t config = {
        .url = "http://192.168.0.15:1880/upload", // IP de tu Dell
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,        // Tiempo máximo de espera
        .disable_auto_redirect = true,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Configuramos el header para que Node-RED sepa que es una imagen
    esp_http_client_set_header(client, "Content-Type", "image/jpeg");

    // Pasamos el buffer directamente (sin copiarlo para ahorrar RAM)
    esp_http_client_set_post_field(client, (const char *)buffer, len);

    // Ejecutamos la petición
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Envío exitoso. Status: %d", esp_http_client_get_status_code(client));
    } else {
        ESP_LOGE(TAG, "Fallo en el POST HTTP: %s", esp_err_to_name(err));
    }

    // Siempre limpiar el cliente para liberar memoria
    esp_http_client_cleanup(client);
    return err;
}

void camara_task(void *pvParameters)
{
#if ESP_CAMERA_SUPPORTED
    if(ESP_OK != init_camera()) {
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
    while (1)
    {
        ESP_LOGI(TAG, "Taking picture...");
        camera_fb_t *pic = esp_camera_fb_get();

        if(pic){
            // use pic->buf to access the image
            ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes", pic->len);

            send_pic(pic->buf,pic->len);
            esp_camera_fb_return(pic);
        }


        

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
#else
    ESP_LOGE(TAG, "Camera support is not available for this chip");
    return;
#endif
}

