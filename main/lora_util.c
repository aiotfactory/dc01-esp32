#include "stdio.h"
#include "string.h"
#include <device_config.h>
#include <util.h>
#include <network_util.h>
#include "system_util.h"
#include "lora_util.h"
#include "llcc68_board.h"
#include "esp_random.h"
#include <cloud_log.h>
#include "radio.h"


static const char *TAG = "lora_util";

static RadioEvents_t LLCC68RadioEvents;
static void LLCC68OnTxDone( void );
static void LLCC68OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
static void LLCC68OnTxTimeout( void );
static void LLCC68OnRxTimeout( void );
static void LLCC68OnRxError( void );


typedef struct {
	uint32_t lora_fre;
	uint8_t lora_tx_output_power;
	uint8_t lora_bandwidth;
	uint8_t lora_spreading_factor;
	uint8_t lora_codingrate;
	uint8_t lora_preamble_length;
	uint8_t lora_fix_length_payload_on;
	uint8_t lora_iq_inversion_on;
	uint32_t tx_timeout;
	uint32_t rx_timeout;
} lora_config_params;
typedef struct {
	lora_config_params *params;
	uint32_t tx_times;
	uint32_t tx_interval_min_ms;
	uint32_t tx_interval_max_ms;
	uint8_t *data;
	uint32_t data_len;
} lora_tx_pack;

#define INIT_PARAMS(config) {config.lora_fre=device_config_get_number(CONFIG_LORA_FRE);config.lora_tx_output_power=device_config_get_number(CONFIG_LORA_TX_OUTPUT_POWER);config.lora_bandwidth=device_config_get_number(CONFIG_LORA_BANDWIDTH);\
		config.lora_spreading_factor=device_config_get_number(CONFIG_LORA_SPREADING_FACTOR);config.lora_codingrate=device_config_get_number(CONFIG_LORA_CODINGRATE);config.lora_preamble_length=device_config_get_number(CONFIG_LORA_PREAMBLE_LENGTH);config.lora_fix_length_payload_on=device_config_get_number(CONFIG_LORA_FIX_LENGTH_PAYLOAD_ON);\
		config.lora_iq_inversion_on=device_config_get_number(CONFIG_LORA_IQ_INVERSION_ON);config.rx_timeout=device_config_get_number(CONFIG_LORA_RX_TIMEOUT_VALUE);config.tx_timeout=device_config_get_number(CONFIG_LORA_TX_TIMEOUT_VALUE);}

static QueueHandle_t lora_evt_queue = NULL;
static uint8_t tx_status=0;//0:stand by, 1:executing 2: send timeout
static uint8_t rx_status=0;//0:stand by, 1:executing 2: recv timeout 3:recv error
static uint32_t lora_data_buf_idx=0;
static uint8_t *lora_data_buf=NULL;
static uint32_t lora_data_buf_len=1024;
static uint32_t lora_tx_times=0,lora_tx_times_timeout=0,lora_rx_times=0,lora_rx_times_timeout=0,lora_rx_times_error=0;
static TaskHandle_t lora_task_handler=NULL;

static uint32_t random_milli_seconds(uint32_t min_value,uint32_t max_value)
{
	if(max_value==min_value)
		return min_value;
    return min_value+(esp_random())%(max_value-min_value);
}

static uint8_t lora_send(uint32_t send_times,uint32_t wait_min_ms,uint32_t wait_max_ms,uint8_t *data,uint32_t data_len)
{
	for(uint32_t i=0;i<send_times;i++)
	{
		if(wait_max_ms&&(wait_max_ms>=wait_min_ms))
			util_delay_ms(random_milli_seconds(wait_min_ms,wait_max_ms));
		tx_status=1;
		Radio.Send(data,data_len);
		for(uint32_t i=0;(i<6000)&&(tx_status==1);i++)//300*20=6000,最多毫秒数
		{
			Radio.IrqProcess( );
			util_delay_ms(1);
		}
		ESP_LOGI(TAG, "lora_send one round end %lu at %lu",i,util_get_timestamp());
		Radio.Standby();
		lora_tx_times++;
	}
	if(tx_status==2)//time out
		return 0;
	return 1;
}

static void lora_deinit(void)
{
	LLCC68IoDeInit();
}
static void lora_init(lora_config_params *params_in)
{
	static lora_config_params params;
	DDL_ZERO_STRUCT(params);
	static uint8_t init=0;
	uint8_t OCP_Value = 0;
	if((init)&&(params_in!=NULL)&&(memcmp(&params,params_in,sizeof(lora_config_params))==0))//inited, but same config, skip
	{
		ESP_LOGI(TAG, "lora_init skip due to same config");
		return;
	}
	if((init==0)||((params_in!=NULL)&&memcmp(&params,params_in,sizeof(lora_config_params))!=0))//not inited, or inited and different config
	{
		if(params_in!=NULL){
			memcpy(&params,params_in,sizeof(lora_config_params));
		}
		else{
			INIT_PARAMS(params)
		}
		if(init==1)
			lora_deinit();
			
		bzero(lora_data_buf,lora_data_buf_len);
		lora_data_buf_idx=0;
		
		LLCC68RadioEvents.TxDone = LLCC68OnTxDone;
		LLCC68RadioEvents.RxDone = LLCC68OnRxDone;
		LLCC68RadioEvents.TxTimeout = LLCC68OnTxTimeout;
		LLCC68RadioEvents.RxTimeout = LLCC68OnRxTimeout;
		LLCC68RadioEvents.RxError = LLCC68OnRxError;

		Radio.Init( &LLCC68RadioEvents );

		ESP_LOGI(TAG, "lora_init fre %lu",params.lora_fre);
		Radio.SetChannel(params.lora_fre);
		ESP_LOGI(TAG, "lora_init tx tx_output_power %u bandwidth %u spreading_factor %u codingrate %u preamble_length %u fix_length_payload_on %u iq_inversion_on %u tx_timeout %lu",params.lora_tx_output_power,params.lora_bandwidth,params.lora_spreading_factor, params.lora_codingrate,params.lora_preamble_length, params.lora_fix_length_payload_on,params.lora_iq_inversion_on, params.tx_timeout);
		Radio.SetTxConfig(MODEM_LORA, params.lora_tx_output_power, 0, params.lora_bandwidth,
	                     params.lora_spreading_factor, params.lora_codingrate,
	                     params.lora_preamble_length, params.lora_fix_length_payload_on,
	                     true, 0, 0, params.lora_iq_inversion_on, params.tx_timeout);
	
		ESP_LOGI(TAG, "lora_init rx bandwidth %u spreading_factor %u codingrate %u preamble_length %u fix_length_payload_on %u iq_inversion_on %u",params.lora_bandwidth,params.lora_spreading_factor, params.lora_codingrate,params.lora_preamble_length, params.lora_fix_length_payload_on,params.lora_iq_inversion_on);
		Radio.SetRxConfig(MODEM_LORA, params.lora_bandwidth, params.lora_spreading_factor,
	                     params.lora_codingrate, 0, params.lora_preamble_length,
	                     LORA_LLCC68_SYMBOL_TIMEOUT, params.lora_fix_length_payload_on,
	                     0, true, 0, 0, params.lora_iq_inversion_on, false );
		OCP_Value = Radio.Read(REG_OCP);
		ESP_LOGI(TAG, "lora_init reg ocp %02x",OCP_Value);
		               
		Radio.Standby();
		init=1;
	}
}

static void LLCC68OnTxDone( void )
{
	//ESP_LOGI(TAG, "LLCC68OnTxDone\r\n");
	tx_status=0;
}

static void LLCC68OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
	//ESP_LOGI(TAG, "lora recv size:%d rssi:%d snr:%d",size,rssi,snr);
	lora_data_buf_idx=size;
	if(lora_data_buf_idx>lora_data_buf_len)
		lora_data_buf_idx=lora_data_buf_len;
	memcpy(lora_data_buf,payload,lora_data_buf_idx);
	rx_status=0;
}

static void LLCC68OnTxTimeout( void )
{
	ESP_LOGW(TAG, "lora send timeout");
	tx_status=2;
	lora_tx_times_timeout++;
}

static void LLCC68OnRxTimeout( void )
{
	//ESP_LOGI(TAG, "lora recv timeout");
	rx_status=2;
	lora_rx_times_timeout++;
}

static void LLCC68OnRxError( void )
{
	ESP_LOGW(TAG, "lora recv error");
	rx_status=3;
	lora_rx_times_error++;
}
//flag(1byte bit0=config bit1=data) init data (x bytes) tx_times,tx_interval_min_ms,tx_interval_max_ms, data len(4 bytes) data
void lora_util_tx(uint8_t *data,uint32_t data_len,uint8_t *data_return,int *data_return_len)
{
	uint8_t *data_return_temp=data_return+1;
	if(((device_config_get_number(CONFIG_LORA_MODE))==0)||(lora_evt_queue==NULL))
	{
		sprintf((char *)data_return_temp,"lora is off in device");
		goto ERROR_PROCESS;
	}
	uint32_t idx=0;
	lora_config_params *params=NULL;
	lora_tx_pack *pack=NULL;
	if((data[0]&0x03)==0)
	{
		sprintf((char *)data_return_temp,"flag %u is wrong",data[0]);
		goto ERROR_PROCESS;
	}	
	idx++;//skip flag
	if(data[0]&0x01)//init config
	{
		params=user_malloc(sizeof(lora_config_params));
		COPY_TO_VARIABLE_NO_CHECK(params->lora_fre,data,idx)
		COPY_TO_VARIABLE_NO_CHECK(params->lora_tx_output_power,data,idx)
		COPY_TO_VARIABLE_NO_CHECK(params->lora_bandwidth,data,idx)
		COPY_TO_VARIABLE_NO_CHECK(params->lora_spreading_factor,data,idx)
		COPY_TO_VARIABLE_NO_CHECK(params->lora_codingrate,data,idx)
		COPY_TO_VARIABLE_NO_CHECK(params->lora_preamble_length,data,idx)
		COPY_TO_VARIABLE_NO_CHECK(params->lora_fix_length_payload_on,data,idx)
		COPY_TO_VARIABLE_NO_CHECK(params->lora_iq_inversion_on,data,idx)
		COPY_TO_VARIABLE_NO_CHECK(params->tx_timeout,data,idx)
		COPY_TO_VARIABLE_NO_CHECK(params->rx_timeout,data,idx)
	}
	pack=user_malloc(sizeof(lora_tx_pack));
	pack->params=params;
	pack->data=NULL;
	if((data[0]>>1)&0x01)//tx
	{
		COPY_TO_VARIABLE_NO_CHECK(pack->tx_times,data,idx)
		COPY_TO_VARIABLE_NO_CHECK(pack->tx_interval_min_ms,data,idx)
		COPY_TO_VARIABLE_NO_CHECK(pack->tx_interval_max_ms,data,idx)
		COPY_TO_VARIABLE_NO_CHECK(pack->data_len,data,idx)
		if(pack->data_len>0)
		{
			pack->data=user_malloc(pack->data_len);
			memcpy(pack->data,data+idx,pack->data_len);
			idx+=pack->data_len;
		}
	}
	if(xQueueSend(lora_evt_queue, &pack, 0)!=pdPASS)
	{
		if(pack->params!=NULL)
			user_free(pack->params);
		if(pack->data!=NULL)
			user_free(pack->data);
		user_free(pack);
		sprintf((char *)data_return_temp,"lora tx failed");
		goto ERROR_PROCESS;
	}else {
		sprintf((char *)data_return_temp,"lora tx success");
		data_return[0]=3;//tx success
		goto OK_PROCESS;
	}
ERROR_PROCESS:
	data_return[0]=2;//tx failed
OK_PROCESS:
	*data_return_len=strlen((char *)data_return_temp)+1;
}
static uint8_t lora_queue_check(uint32_t wait_ms)
{
	lora_tx_pack *tx_pack=NULL;
	if(xQueueReceive(lora_evt_queue, &tx_pack,pdMS_TO_TICKS(wait_ms)))//to send
	{
		if(tx_pack->params!=NULL)
		{
			ESP_LOGI(TAG, "lora_queue_check init");
			lora_init(tx_pack->params);
			user_free(tx_pack->params);
		}
		if((tx_pack->data!=NULL)&&(tx_pack->data_len>0))
		{
			ESP_LOGI(TAG, "lora_queue_check send");
			cloud_printf_format("lora_tx:","\r\n","%02x",tx_pack->data,tx_pack->data_len);
			Radio.Standby();
			lora_send(tx_pack->tx_times,tx_pack->tx_interval_min_ms,tx_pack->tx_interval_max_ms,tx_pack->data,tx_pack->data_len);
			if(tx_pack->data!=NULL)
				user_free(tx_pack->data);
			user_free(tx_pack);
			tx_pack=NULL;
		}
		return 1;
	}
	return 0;
}
//since it is heavy for lora to send, so it uses a thread
static void lora_task(void *pvParameters)
{
	uint32_t rx_timeout=3000;
	uint8_t lora_mode=device_config_get_number(CONFIG_LORA_MODE);
	lora_data_buf_len=device_config_get_number(CONFIG_LORA_RX_BUF_LEN);
    lora_data_buf=user_malloc(lora_data_buf_len+2);	
	lora_evt_queue = xQueueCreate( 10, sizeof( lora_tx_pack * ) );
	lora_init(NULL);
    while(1) 
    {
		if(lora_mode==1)//rx
		{
			rx_status=1;
			rx_timeout=device_config_get_number(CONFIG_LORA_RX_TIMEOUT_VALUE);
			lora_data_buf_idx=0;
			lora_rx_times++;
			Radio.Rx(rx_timeout);
			for(uint32_t i=0;(i<200)&&(rx_status==1);i++)//10000 is protection
			{
				Radio.IrqProcess( );
				if(lora_queue_check(50))
					break;
			}
			Radio.Standby();
			//recv and upload
			if(lora_data_buf_idx>0)
			{
				ESP_LOGI(TAG, "lora_rx len %lu",lora_data_buf_idx);
				cloud_printf_format("lora_rx ","\r\n","%02x",lora_data_buf,lora_data_buf_idx);
				memmove(lora_data_buf+1,lora_data_buf,lora_data_buf_idx);
				lora_data_buf_idx++;
				lora_data_buf[0]=1;//data
				tcp_send_command(
			      "lora_rx",
			      data_container_create(0,COMMAND_REQ_LORA,lora_data_buf, lora_data_buf_idx,NULL),
			      NULL, 0, COMMAND_FLAG_FROM_DEVICE | COMMAND_FLAG_INIT_FROM_DEVICE,
			      SOCKET_HANDLER_ONE_TIME, 0,NULL);
			}
		}else {//check tx
			lora_queue_check(50);
		}
    }
}
static void lora_task_start(void)
{
	if((device_config_get_number(CONFIG_LORA_MODE))==0)
		return;
	xTaskCreate(&lora_task, "lora_task", 10*1024, NULL, 9, &lora_task_handler);
}
int lora_util_upload(void *parameter)
{
	static uint8_t init=0;
	if(init==0)
	{
		init=1;
		power_onoff_lora(1);
		lora_task_start();
	}
	uint8_t *temp_buf;
	int temp_buf_idx=0,ret=-1;
	temp_buf=user_malloc(1024);
	lora_util_property(temp_buf,&temp_buf_idx);
	ret=tcp_send_command(
	      "lora_meta",
	      data_container_create(1,COMMAND_REQ_LORA,temp_buf, temp_buf_idx,NULL),
	      NULL, 0, COMMAND_FLAG_FROM_DEVICE | COMMAND_FLAG_INIT_FROM_DEVICE,
	      SOCKET_HANDLER_ONE_TIME, 0,NULL);
	
	return ret;
}

void lora_util_property(uint8_t *data_return,int *data_return_len)
{
	int idx=0;
	data_return[0]=0;//meta
	idx++;
	COPY_TO_BUF_WITH_NAME(lora_tx_times,data_return,idx);
	COPY_TO_BUF_WITH_NAME(lora_tx_times_timeout,data_return,idx);
	COPY_TO_BUF_WITH_NAME(lora_rx_times,data_return,idx);
	COPY_TO_BUF_WITH_NAME(lora_rx_times_timeout,data_return,idx);
	COPY_TO_BUF_WITH_NAME(lora_rx_times_error,data_return,idx);
	*data_return_len=idx;
}
