# main/CMakeLists.txt

idf_component_register(SRCS "app_NFC.c" 
                            "app_wifi.c" 
                            "SEAII.c" 
                            "main.c" 
                            "app_uart.c" 
                            "protocol.c"
                    INCLUDE_DIRS "."
                    REQUIRES mbedtls littlefs esp-idf-pn532 esp_wifi esp_event nvs_flash)