#pragma once

#include "esp_err.h"
#include "esp_camera.h"
#include "device_config.h"
#ifdef __cplusplus
extern "C"
{
#endif

camera_fb_t *camera_take(framesize_t size,uint8_t quality,int aecgain);
int camera_upload(void *pvParameters);
void camera_off(void);
void camera_property(uint8_t *data_out,int *data_len_out);
int camera_request(data_send_element *data_element);
#ifdef __cplusplus
}
#endif
