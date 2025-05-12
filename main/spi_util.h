#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
#include "esp_err.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/spi_common.h"


typedef enum {
	SPI_LORA=0,
	SPI_TM7705=1,
	SPI_W5500=2,
	SPI_EXT1=3,
	SPI_EXT2=4,
	SPI_ALL=5
} spiutil_device_t;

void spiutil_deinit(spiutil_device_t device);
spi_device_handle_t spiutil_init(spiutil_device_t device, char *device_name,spi_device_interface_config_t *devcfg_in);
void spi_cs_pins_default(void);
esp_err_t  spiutil_tm7705_write(spiutil_device_t device, uint8_t address,uint8_t data);
esp_err_t  spiutil_tm7705_read(spiutil_device_t device, uint8_t address,uint8_t *data,uint8_t datalen);
esp_err_t spiutil_inout(spiutil_device_t device,uint64_t address,uint8_t address_valid,uint16_t command,uint8_t command_valid,uint8_t *data_send,uint32_t data_send_len,uint8_t *data_recv,uint32_t data_recv_len);
uint8_t spiutil_inout_byte(spiutil_device_t device,uint8_t data);
void spi_cs_w5500_post_cb(spi_transaction_t* t);
void spi_cs_w5500_pre_cb(spi_transaction_t* t);
int command_operate_spi_inout(uint8_t *command_data,uint32_t command_data_len,uint8_t *data_return,int *data_return_len);
int command_operate_spi_init(uint8_t *command_data,uint32_t command_data_len,uint8_t *data_return,int *data_return_len);
void spi_property(uint8_t *data_out,int *data_len_out);

esp_err_t spiutil_w5500_read(uint32_t address,uint8_t *data,int16_t len);
#ifdef __cplusplus
}
#endif
