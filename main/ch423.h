#pragma once

#include "esp_err.h"


#ifdef __cplusplus
extern "C"
{
#endif

#define CH423_IO0     0
#define CH423_IO1     1
#define CH423_IO2     2
#define CH423_IO3     3
#define CH423_IO4     4
#define CH423_IO5     5
#define CH423_IO6     6
#define CH423_IO7     7
#define CH423_OC_IO0     8
#define CH423_OC_IO1     9
#define CH423_OC_IO2     10
#define CH423_OC_IO3     11
#define CH423_OC_IO4     12
#define CH423_OC_IO5     13
#define CH423_OC_IO6     14
#define CH423_OC_IO7     15
#define CH423_OC_IO8     16
#define CH423_OC_IO9     17
#define CH423_OC_IO10    18
#define CH423_OC_IO11    19
#define CH423_OC_IO12    20
#define CH423_OC_IO13    21
#define CH423_OC_IO14    22
#define CH423_OC_IO15    23


esp_err_t ch423_output(uint8_t ionum, uint8_t status);
esp_err_t ch423_init(void);
uint8_t ch423_get_status(uint8_t ionum);

#ifdef __cplusplus
}
#endif
