#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

int uart_util_upload(void *parameter);
void uart_util_tx(uint8_t *data,uint32_t data_len);
void uart_util_queue(void);

#ifdef __cplusplus
}
#endif
