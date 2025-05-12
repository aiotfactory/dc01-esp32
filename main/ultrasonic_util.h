#pragma once


#ifdef __cplusplus
extern "C"
{
#endif

#include "device_config.h"

int ultrasonic_upload(void *parameter);
void ultrasonic_queue(void);
int ultrasonic_request(data_send_element *data_element);

#ifdef __cplusplus
}
#endif
