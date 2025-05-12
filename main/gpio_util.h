#pragma once

#include "esp_err.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    adc_oneshot_unit_handle_t oneshot_handle;
    adc_cali_handle_t cali_handle;
    bool cali_result;
    adc_unit_t unit;
    adc_channel_t channel;
    uint8_t init;
    int value_raw;
    int value_voltage;
    adc_atten_t atten;
} gpio_adc_info;


int gpio_util_adc_read(gpio_adc_info *adc_info,bool tear_down);
int  gpio_util_upload(void *parameter);
void gpio_util_queue(uint32_t io_num);
void gpio_util_property(uint8_t *data_out,int *data_len_out);
int command_operate_gpio_esp_set(uint8_t *command_data,uint32_t command_data_len,uint8_t *data_return);
int command_operate_gpio_ext_set(uint8_t *command_data,uint32_t command_data_len,uint8_t *data_return);
int command_operate_gpio_read(uint8_t *command_data,uint32_t command_data_len,uint8_t *data_return,int *data_return_len);
#ifdef __cplusplus
}
#endif
