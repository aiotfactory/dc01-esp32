#pragma once

#include "esp_err.h"


#ifdef __cplusplus
extern "C"
{
#endif

void module_4g_start(uint32_t wait_seconds);
void *module_wifi_start(void);
int module_wifi_set_dns(void *ap_netif);
#ifdef __cplusplus
}
#endif
