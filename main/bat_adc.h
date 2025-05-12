#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

int bat_adc_upload(void *parameter);
void bat_adc_property(uint8_t *data_out,int *data_len_out);
#ifdef __cplusplus
}
#endif
