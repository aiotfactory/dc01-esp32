#pragma once

#include "esp_err.h"


#ifdef __cplusplus
extern "C"
{
#endif

int meta_upload(void *parameter);
void meta_property(uint8_t *data_out,int *data_len_out);

#ifdef __cplusplus
}
#endif
