idf_component_register(SRCS "PCF8574.c" "camara.c" 
                            "app_NFC.c" 
                            "app_wifi.c" 
                            "SEAII.c" 
                            "main.c" 
                            "app_uart.c" 
                            "protocol.c"
                            "DIO.c"
                    INCLUDE_DIRS "."
                    REQUIRES mbedtls 
                             esp-idf-pn532 
                             esp_wifi 
                             esp_event 
                             nvs_flash 
                             esp_http_client)