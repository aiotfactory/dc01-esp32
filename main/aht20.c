#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_types.h"
#include "i2cutil.h"
#include "util.h"
#include <device_config.h>
#include <math.h>
#include <network_util.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "system_util.h"
#include "device_config.h"


#define AHT20_ADDR 0x38
#define AHT20_POLYNOMIAL 0x31

static const char *TAG = "aht20";
static uint8_t cmd_measure[3] = {0xac, 0x33, 0x00};

static uint32_t times_success = 0, times_total = 0;
static int temperature_max = -99999999, temperature_min = 99999999,
           humidity_max = -99999999, humidity_min = 99999999;

int temperature_aht20=-99999999,humidity_aht20=-99999999;

static uint8_t aht20_read_once(uint8_t *flag, uint8_t *status, int *humidity,
                               int *temperature) {
  uint8_t data[7] = {0};
  int64_t s32x;
  *flag = 0;
  *status = 0;
  *humidity = 0;
  *temperature = 0;
  times_total++;
  if (i2c_init(100000) == 0) {
    *flag = 6;
    goto ERROR_PROCESS;
  }
  if (i2c_write(AHT20_ADDR, cmd_measure, sizeof(cmd_measure)) != ESP_OK) {
    *flag = 1;
    goto ERROR_PROCESS;
  }
  util_delay_ms(200);
  if (i2c_read(AHT20_ADDR, data, sizeof(data)) != ESP_OK) {
    *flag = 2;
    goto ERROR_PROCESS;
  }
  if (util_crc_calc(data, 6, AHT20_POLYNOMIAL) != data[6]) {
    *flag = 3;
    goto ERROR_PROCESS;
  }
  if ((data[0] & 0x98) != 0x18) {
    *flag = 4;
    goto ERROR_PROCESS;
  }

  s32x = data[1];
  s32x = s32x << 8;
  s32x += data[2];
  s32x = s32x << 8;
  s32x += data[3];
  s32x = s32x >> 4;
  *humidity = s32x * 10000 / 1024 / 1024; // humidity
  s32x = data[3] & 0x0F;
  s32x = s32x << 8;
  s32x += data[4];
  s32x = s32x << 8;
  s32x += data[5];
  *temperature = s32x * 20000 / 1024 / 1024 - 5000; // temperature

  if ((*temperature < -4000) || (*temperature > 9900) || (*humidity < 0) ||
      (*humidity > 10000)) {
    *flag = 5;
    goto ERROR_PROCESS;
  }
  times_success++;
  if (*temperature > temperature_max)
    temperature_max = *temperature;
  if (*temperature < temperature_min)
    temperature_min = *temperature;
  if (*humidity > humidity_max)
    humidity_max = *humidity;
  if (*humidity < humidity_min)
    humidity_min = *humidity;
ERROR_PROCESS:
  *status = data[0];
  if (*flag != 0) {
    ESP_LOGW(TAG, "aht20 read failed due to %d", *flag);
  }
  return *flag;
}
static uint8_t aht20_read_exe(uint8_t *flag, uint8_t *status, int *humidity,
                              int *temperature, uint8_t *property,
                              uint8_t retry_times) {
  uint32_t idex = 0;
  for (uint8_t i = 0; i < retry_times; i++) {
    if (aht20_read_once(flag, status, humidity, temperature) > 0)
      goto ERROR_PROCESS;
  }
  
  variable_set(&temperature_aht20,temperature, sizeof(int));
  variable_set(&humidity_aht20,humidity, sizeof(int));
 
ERROR_PROCESS:
  COPY_TO_BUF_NO_CHECK(times_total, property, idex)
  COPY_TO_BUF_NO_CHECK(times_success, property, idex)
  COPY_TO_BUF_NO_CHECK(temperature_max, property, idex)
  COPY_TO_BUF_NO_CHECK(temperature_min, property, idex)
  COPY_TO_BUF_NO_CHECK(humidity_max, property, idex)
  COPY_TO_BUF_NO_CHECK(humidity_min, property, idex)
  return *flag;
}
#define AHT20_DATA_LEN 34
int aht20_upload(void *parameter) {
  uint8_t *temp_buff = user_malloc(AHT20_DATA_LEN);
  aht20_read_exe(temp_buff, temp_buff + 1, (int *)(temp_buff + 2),
                 (int *)(temp_buff + 6), temp_buff + 10, 3);
  return tcp_send_command(
      "aht20",
      data_container_create(1, COMMAND_REQ_AHT20, temp_buff, AHT20_DATA_LEN),
      NULL, 0, COMMAND_FLAG_FROM_DEVICE | COMMAND_FLAG_INIT_FROM_DEVICE,
      SOCKET_HANDLER_ONE_TIME, 0,NULL);
}
void aht20_request(uint8_t *data_out, int *data_len_out) {
  uint8_t *temp_buff = data_out;
  aht20_read_exe(temp_buff, temp_buff + 1, (int *)(temp_buff + 2),
                 (int *)(temp_buff + 6), temp_buff + 10, 3);
  *data_len_out = AHT20_DATA_LEN;
}
