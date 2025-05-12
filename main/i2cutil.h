#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include "driver/gpio.h"

#define ACK_CHECK_EN 0x1                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_NO 0x0                        /*!< I2C master will check ack from slave*/
#define ACK_VAL 0x0                             /*!< I2C ack value */
#define NACK_VAL 0x1                            /*!< I2C nack value */

uint8_t i2c_init(uint32_t clk);
void i2c_deinit(void);
esp_err_t  i2c_write(uint8_t addr,uint8_t *data,uint16_t size);
esp_err_t  i2c_read(uint8_t addr,uint8_t *data,uint16_t size);
esp_err_t  i2c_read_reg(uint8_t addr,uint8_t *reg,uint8_t reg_len,uint8_t *data,uint16_t size);
esp_err_t  i2c_write_reg(uint8_t addr,uint8_t reg,uint8_t *data,uint16_t size);
int command_operate_i2c_inout(uint8_t *command,uint32_t command_len,uint8_t *data_return,int *data_return_len,uint32_t data_return_max_len);
#ifdef __cplusplus
}
#endif
