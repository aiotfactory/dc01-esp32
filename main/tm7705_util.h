#pragma once

#include "esp_err.h"


#ifdef __cplusplus
extern "C"
{
#endif

esp_err_t adc_read_value(int *ml01,int *ml02);
int tm7705_upload(void *pvParameters);
void tm7705_property(uint8_t *data_out,int *data_len_out);
void tm7705_read(uint8_t *command_data,uint32_t command_data_len,uint8_t *data_out,int *data_len_out);
#ifdef __cplusplus
}
#endif
