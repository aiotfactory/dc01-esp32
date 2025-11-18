#pragma once


#ifdef __cplusplus
extern "C"
{
#endif


/*
topic /dc01/a085e3e66d1c/ble/dtu/upload
*/
#define TOPIC_BLE_DTU_UPLOAD "ble/dtu/upload"
#define TOPIC_BLE_DTU_COMMAND "ble/dtu/command"

void mqtt_init(void);
int mqtt_send(char *topic,uint8_t *data,uint32_t data_len);

#ifdef __cplusplus
}
#endif
