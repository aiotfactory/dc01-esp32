#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_log.h"
#include <device_config.h>
#include <spi_util.h>
#include <network_util.h>
#include <util.h>
#include "system_util.h"

extern uint32_t config_to_stream(uint8_t *buf);

//static const char *TAG = "config_upload_util";
int config_upload(void *parameter)
{
	uint32_t config_stream_len=config_to_stream(NULL);//cal size
	uint8_t *config_stream=(uint8_t *)user_malloc(config_stream_len);
	config_to_stream(config_stream);			
 	return tcp_send_command(
      "config",
      data_container_create(1,COMMAND_REQ_CONFIG,config_stream, config_stream_len),
      NULL, 0, COMMAND_FLAG_FROM_DEVICE | COMMAND_FLAG_INIT_FROM_DEVICE,
      SOCKET_HANDLER_ONE_TIME, 0,NULL);
}
