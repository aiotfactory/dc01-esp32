#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "spi_flash_mmap.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_sntp.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_flash.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp32s3/rom/md5_hash.h"
#include "cloud_log.h"
#include "device_config.h"
#include "util.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "system_util.h"

EventGroupHandle_t app_evt_group_hdl = NULL;
QueueHandle_t gpio_evt_queue = NULL;

static const char *TAG = "util";

void ddl_memclr(void *pu8Address, uint32_t u32Count)
{
    uint8_t *pu8Addr = (uint8_t *)pu8Address;

    if(NULL == pu8Addr)
    {
        return;
    }

    while (u32Count--)
    {
        *pu8Addr++ = 0;
    }
}
void print_mem_info(void)
{
	//ESP_LOGI(TAG,"Free heap size: %" PRIu32 " bytes", esp_get_free_heap_size());
	//ESP_LOGI(TAG,"Free internal heap size: %" PRIu32 " bytes", esp_get_free_internal_heap_size());
    //ESP_LOGI(TAG,"Minimum free heap size: %" PRIu32 " bytes", esp_get_minimum_free_heap_size());
   ESP_LOGI(TAG, "Free heap=%d|%d|%d Free mini=%d|%d|%d Bigst=%d|%d|%d|%d",
             heap_caps_get_free_size(MALLOC_CAP_DEFAULT),
             heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
             heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT),
             heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM),
             heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL),
             heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL),
             heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT),
             heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM),
             heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)
			);
}
int print_format(char *dst,uint32_t dst_len,char *format,char *prefix, char *suffix,uint8_t *data,uint32_t data_len)
{
	int num=0;
	if(dst==NULL)
	{
		if(prefix!=NULL)
			num=printf("%s",prefix);
		for(uint32_t i=0;i<data_len;i++)
			num=num+printf(format,data[i]);		
		if(suffix!=NULL)
			num=num+printf("%s",suffix);
	}else{

		if(prefix!=NULL)
			num=snprintf(dst,dst_len,"%s",prefix);
		for(uint32_t i=0;i<data_len;i++)
			num=num+snprintf(dst+num,dst_len-num,format,data[i]);		
		if(suffix!=NULL)
			num=num+snprintf(dst+num,dst_len-num,"%s",suffix);	
	}
	return num;
}

void util_pause(char *str)
{
	while(1)
	{
		util_delay_ms(1000);
		if(str!=NULL)
			ESP_LOGI(TAG, "%s",str);
	}
}
uint32_t util_get_timestamp(void)
{
	struct timeval tv_now;
	gettimeofday(&tv_now, NULL);
	return (tv_now.tv_sec*1000+(uint32_t)tv_now.tv_usec/1000);
}


uint8_t util_crc_calc(uint8_t *data, uint8_t data_num,uint32_t polynomial)
{
  uint8_t bit;        // bit mask
  uint8_t crc = 0xFF; // calculated checksum
  uint8_t byteCtr;    // byte counter

  // calculates 8-Bit checksum with given polynomial
  for(byteCtr = 0; byteCtr < data_num; byteCtr++)
  {
    crc ^= (data[byteCtr]);
    for(bit = 8; bit > 0; --bit)
    {
      if(crc & 0x80) crc = (crc << 1) ^ polynomial;
      else           crc = (crc << 1);
    }
  }
  return crc;
}

uint8_t *byte_search(uint8_t *data, uint32_t data_len,uint8_t target,uint32_t index)
{
	uint32_t count = 0;
    for (uint32_t i = 0; i < data_len; ++i) {
        if (data[i] == target) {
            count++;
            if (count == index) {
                return &data[i];
            }
        }
    }
    return NULL;
}




void util_gpio_init(uint8_t pin,gpio_mode_t mode,gpio_pulldown_t pulldownmode,gpio_pullup_t pullupmode,uint8_t value)
{
    //turn on all power
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = 0;
    //set as output mode
    io_conf.mode = mode;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = (1ULL<<pin) ;
    //disable pull-down mode
    io_conf.pull_down_en = pulldownmode;
    //disable pull-up mode
    io_conf.pull_up_en = pullupmode;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    if(value<2)
    	gpio_set_level(pin,value);
}
void power_onoff_i2c(uint8_t status)
{
	ch423_output(CH423_OC_IO15, status);
}
uint8_t power_status_i2c(void)
{
	return ch423_get_status(CH423_OC_IO15);
}
void power_onoff_eth(uint8_t status)
{
	ch423_output(CH423_OC_IO2, status);
}
void power_onoff_4g(uint8_t status)
{
	ch423_output(CH423_OC_IO3, status);
}
void power_onoff_camera(uint8_t status)
{
	ch423_output(CH423_OC_IO10, status);
}
void power_onoff_batadc(uint8_t status)
{
	ch423_output(CH423_OC_IO6, status);
}
void power_onoff_lora(uint8_t status)
{
	ch423_output(CH423_OC_IO1, status);
}
void power_onoff_bat_adc(uint8_t status)
{
	ch423_output(CH423_OC_IO6, status);
}
void power_onoff_output_join(uint8_t status)
{
	ch423_output(CH423_OC_IO13, status);
}
void power_onoff_output_bat(uint8_t status)
{
	ch423_output(CH423_OC_IO14, status);
}
void power_onoff_output_vcc(uint8_t status)
{
	ch423_output(CH423_IO0, status);
}
void power_onoff_spi(uint8_t status)
{
	ch423_output(CH423_IO6, status);
}
void power_onoff_led(uint8_t status)
{
	ch423_output(CH423_OC_IO7, status);
}
uint8_t power_status_spi(void)
{
	return ch423_get_status(CH423_IO6);
}
void util_reset_4g(int onoff)
{
	if(onoff<0)
	{
		ESP_LOGI(TAG, "util_reset_4g reset %d",onoff);
		ch423_output(CH423_IO4, 0);
		util_delay_ms(500);
		ch423_output(CH423_IO4, 1);
	}else 
		ch423_output(CH423_IO4, onoff);

}

void print_chip_info(void)
{
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    ESP_LOGI(TAG,"%s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    ESP_LOGI(TAG,"silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        ESP_LOGE(TAG,"Get flash size failed");
        return;
    }

    ESP_LOGI(TAG,"%" PRIu32 "MB %s flash", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
}


void property_value_add(uint8_t *data_out,int *data_out_len,char *property_name,uint8_t property_name_len,uint8_t property_flag,uint16_t property_value_len,uint8_t *property_value)
{
	int idx=(*data_out_len);
	memcpy(data_out+idx,&property_name_len,sizeof(property_name_len));
	idx+=sizeof(property_name_len);
	memcpy(data_out+idx,property_name,property_name_len);
	idx+=property_name_len;
	memcpy(data_out+idx,&property_flag,sizeof(property_flag));
	idx+=sizeof(property_flag);
	memcpy(data_out+idx,&property_value_len,sizeof(property_value_len));
	idx+=sizeof(property_value_len);
	if(property_value_len>0)
	{
		memcpy(data_out+idx,property_value,property_value_len);
		idx+=property_value_len;
	}
	*data_out_len=idx;
}
void mem_check(char *prefix)
{
	if(prefix!=NULL)
		ESP_LOGI(TAG,"%s",prefix);
	heap_caps_check_integrity_all(true);
}
static esp_timer_handle_t oneshot_timer=NULL;
static void led_flash_callback(void* arg)
{
    power_onoff_led(0);
    ESP_ERROR_CHECK(esp_timer_delete(oneshot_timer));
    oneshot_timer=NULL;
}
void led_flash(uint32_t time_ms)
{	
	if(oneshot_timer==NULL){
		power_onoff_led(1);
		const esp_timer_create_args_t oneshot_timer_args = {
	            .callback = &led_flash_callback,
	            .arg = NULL,
	            .name = "one-shot"
	    };
	    ESP_ERROR_CHECK(esp_timer_create(&oneshot_timer_args, &oneshot_timer));
	    ESP_ERROR_CHECK(esp_timer_start_once(oneshot_timer, time_ms*1000));
    }
}