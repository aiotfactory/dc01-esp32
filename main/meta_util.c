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


//static const char *TAG = "meta_util";

static void data_prepare(uint8_t *out_buf,int *out_idx)
{
	uint8_t *temp_buf=out_buf;
	int temp_buf_idx=0;
	al_meta_times++;
	property_value_add(temp_buf,&temp_buf_idx,"imei",4,1,strlen(rt_imei),(uint8_t *)rt_imei);
	property_value_add(temp_buf,&temp_buf_idx,"iccid",5,1,strlen(rt_iccid),(uint8_t *)rt_iccid);
	uint32_t al_run_seconds=util_get_run_seconds();
	COPY_TO_BUF_WITH_NAME(al_run_seconds,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(al_times_reconn,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(al_meta_times,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_restart_reason,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_restart_reason_times,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_reset_reason,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_restart_times,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_restart_reason_seconds,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_4g_ip,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_4g_mask,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_4g_gw,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_4g_dns1,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_4g_dns2,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_4g_ipv6,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_4g_ipv6_zone,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_wifi_sta_ip,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_wifi_sta_mask,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_wifi_sta_gw,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_wifi_ap_dns,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_wifi_ap_sta_num,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_w5500_sta_ip,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_w5500_sta_mask,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_w5500_sta_gw,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_w5500_sta_dns1,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_w5500_sta_dns2,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_w5500_sta_ipv6,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(rt_w5500_sta_ipv6_zone,temp_buf,temp_buf_idx);
	*out_idx=temp_buf_idx;
}
int meta_upload(void *parameter)
{
	uint8_t *temp_buf;
	int temp_buf_idx=0,ret=-1;
    if(device_config_get_number(CONFIG_META_ONOFF)==0)
    {
		return 0;
	} 
	temp_buf=user_malloc(1024);
	data_prepare(temp_buf,&temp_buf_idx);

	//499
	//ESP_LOGE(TAG, "x1 %d",temp_buf_idx);
	ret=tcp_send_command(
	      "meta",
	      data_container_create(1,COMMAND_REQ_META,temp_buf, temp_buf_idx,NULL),
	      NULL, 0, COMMAND_FLAG_FROM_DEVICE | COMMAND_FLAG_INIT_FROM_DEVICE,
	      SOCKET_HANDLER_ONE_TIME, 0,NULL);
	//ESP_LOGI(TAG, "meta_upload times %lu ret %d len %d",al_meta_times,ret,temp_buf_idx);
	return ret;
}
void meta_property(uint8_t *data_out,int *data_len_out)
{
	data_prepare(data_out,data_len_out);
}