#pragma once

#include "device_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

void response_process(uint32_t command,uint32_t flag,int32_t errorType,uint32_t pack_seq,const uint8_t *data_in,uint32_t data_in_len);
data_send_element *operate_process(uint32_t operate,uint8_t *data_in,uint32_t data_in_len);

#ifdef __cplusplus
}
#endif
