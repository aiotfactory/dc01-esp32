#pragma once


#ifdef __cplusplus
extern "C"
{
#endif

#include "device_config.h"

int thermal_upload(void *parameter);
int thermal_request(data_send_element *data_element);

#ifdef __cplusplus
}
#endif
