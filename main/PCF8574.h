#pragma once

#include <stdint.h>
#include "esp_err.h"

void pcf_init(void);
esp_err_t pcf8574_set_pin(uint8_t pin, uint8_t value);

