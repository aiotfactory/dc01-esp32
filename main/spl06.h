#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include "driver/gpio.h"


int spl06_upload(void *parameter);
void spl06_request(uint8_t *data_out,int *data_len_out);

#ifdef __cplusplus
}
#endif
