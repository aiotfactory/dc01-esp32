#pragma once

#include "esp_err.h"
#include "network_util.h"

#ifdef __cplusplus
extern "C"
{
#endif

int forward_upload(void *parameter);
void forward_property(uint8_t *command_data,uint32_t command_data_len,data_send_element *data_element);
#ifdef __cplusplus
}
#endif
