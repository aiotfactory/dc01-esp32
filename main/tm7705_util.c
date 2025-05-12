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

#define DRDY okl()

static esp_err_t adc_init(void);

static double allv1=2.50000f;
static double allv2=65535.00000f;
static uint8_t adc_inited=0;

static uint32_t read_times,reset_times=0,read_failed_times=0,init_failed_times=0;
static int max_value_pin1=0,max_value_pin2=0;

static const char *TAG = "tm7705";
//static uint8_t okldata=0;

static double gain(uint8_t reg_freq_config)
{
	reg_freq_config=(reg_freq_config>>3)&0x07;
	if(reg_freq_config==0) return 1.0000f;
	else if(reg_freq_config==1) return 2.0000f;
	else if(reg_freq_config==2) return 4.0000f;
	else if(reg_freq_config==3) return 8.0000f;
	else if(reg_freq_config==4) return 16.0000f;
	else if(reg_freq_config==5) return 32.0000f;
	else if(reg_freq_config==6) return 64.0000f;
	else if(reg_freq_config==7) return 128.0000f;
	return 0.0000f;
}
static uint8_t okl(uint8_t avin)
{
	uint8_t data=0;
	spiutil_tm7705_read(SPI_TM7705,0x08+(avin-1),(uint8_t *)&data,1);//0 000 1 000 ain通讯寄存器读取
	//ESP_LOGI(TAG, "avin%u,okl %02x",avin,data);
	//okldata=data;
	return (data>>7)&0x01;
}

void tm7705_off(void)
{
	adc_inited=0;
	spiutil_deinit(SPI_TM7705);
	ch423_output(CH423_OC_IO0, 0);
}
//返回单位是福特的倍数
static int adc_read_value_excute(uint8_t avin)
{
	//01 100 1 1 0 自校正，16倍增益,单极性,输入缓冲器开启,sync关闭 0x66
	//01 100 1 0 0 自校正，16倍增益,单极性,输入缓冲器开启,sync关闭 0x64
	//01 011 1 1 0 自校正，8倍增益,单极性,输入缓冲器开启,sync关闭 0x5e
	//01 000 1 0 0 自校正，1倍增益,单极性,输入缓冲器关闭,sync关闭 0x44
	//01 001 1 0 0 自校正，2倍增益,单极性,输入缓冲器关闭,sync关闭 0x4c
	//01 000 1 0 0 自校正，1倍增益,单极性,输入缓冲器开启,sync关闭 0x46
	  
	uint8_t reg_freq_config=device_config_get_number(CONFIG_TM7705_REG_CONFIG);
	double temp_gain=gain(reg_freq_config);
	if(adc_init()==ESP_FAIL)
		goto ERROR_PROCESS;
	spiutil_tm7705_write(SPI_TM7705,0x10+(avin-1), reg_freq_config);//0 001 0 000 配置寄存器写入 01 100 1 0 0
	//util_delay_ms(300);
	//uint8_t tempdata=0;
	//spiutil_tm7705_read((0x10+(avin-1))|0x08,(uint8_t *)&tempdata,1);
	//okldata=tempdata;

    
	double ddata2=0.0000f;
	uint8_t isvalid=0;
	for (uint16_t n = 0; n < 2; n++)
	{
		uint8_t m=0;
		while (okl(avin))
		{
			util_delay_ms(10);
			m++;
			if(m>24)
				break;
		}
		if(m>24)
		{
			ESP_LOGW(TAG, "ain%u read failed1 %u", avin,n);
			continue;
		}
		//util_delay_ms(120);
		uint8_t dataraw[4] = { 0 };
		spiutil_tm7705_read(SPI_TM7705,0x38 + (avin - 1), dataraw, 2);//0 011 1 000 adc结果读取
		uint32_t data2 = dataraw[0] * 256 + dataraw[1];
		if(data2==65535){
			ESP_LOGW(TAG, "ain%u read failed2", avin);
			continue;
		}
		ddata2 = ((allv1* data2)/allv2)/temp_gain;
		isvalid=1;
		//ESP_LOGI(TAG, "ain%u ddata %0.10fv", avin,ddata2);
		if(isvalid)
			break;
	}
	if(isvalid){
		return (int)(ddata2*10000000);//10 uv
	}
	tm7705_off();
ERROR_PROCESS:
	return -100;//0
}
//测试通讯
static esp_err_t adc_init(void)
{
	  if(adc_inited==0)
	  {
		  reset_times++;
		  ch423_output(CH423_OC_IO0, 1);
		  //make spi stable
		  spiutil_init(SPI_TM7705,"tm7705",NULL);
		  //util_delay_ms(10);
		  uint8_t data=0;
		  uint16_t init_times=device_config_get_number(CONFIG_TM7705_INIT_TIMES);
		  uint8_t reg_freq_value=device_config_get_number(CONFIG_TM7705_REG_FREQ);
		  uint16_t init_times_temp=0;
		  for(uint16_t i=0;i<200;i++)
		  {
			  spiutil_tm7705_write(SPI_TM7705,0x20,reg_freq_value);//(通讯寄存器)0 010 0 0 00 ain1频率寄存器写入  000 0 0 100(0x04) 50hz
			  //util_delay_ms(10);
			  data=0;
			  spiutil_tm7705_read(SPI_TM7705,0x28,(uint8_t *)&data,1); //0 010 1 0 00 ain1频率寄存器读取
			  //ESP_LOGI(TAG, "ml flag %02x",data);
			  if(data==reg_freq_value)
			   	init_times_temp++;
			  else{
				ESP_LOGW(TAG, "adc_init %u %02x %02x",i,data,reg_freq_value);
 				init_times_temp=0;
 			  }
 			  if(init_times_temp>init_times)	
 			  	break;
		  }
		  if(init_times_temp<=init_times)	
	  		  goto fail_process;
          //00001100
		  spiutil_tm7705_write(SPI_TM7705,0x21, reg_freq_value);//0 010 0 0 01 ain2频率寄存器写入  000 0 0 100(0x04) 50hz
		  spiutil_tm7705_read(SPI_TM7705,0x29,(uint8_t *)&data,1); //0 010 1 0 01 ain2频率寄存器读取
		  if(data!=reg_freq_value)
			 goto fail_process;
		  //ESP_LOGI(TAG, "tm7705 inited success");
		  adc_inited=1;
		  return ESP_OK;
	  }
	  return ESP_OK;
	  
fail_process:	  
	  ESP_LOGW(TAG, "tm7705 inited fail");
	  spiutil_deinit(SPI_TM7705);
	  ch423_output(CH423_OC_IO0, 0);
	  init_failed_times++;
	  return ESP_FAIL;
}
esp_err_t adc_read_value(int *ml01,int *ml02)
{
	read_times++;
	if((*ml01)>0)
		*ml01=adc_read_value_excute(1);
	if((*ml02)>0)
		*ml02=adc_read_value_excute(2);
	//ESP_LOGI(TAG, "tm7705 pin1 %u,pin2 %u",*ml01,*ml02);
	if((*ml01==-100)||(*ml02==-100)){
		read_failed_times++;
		return ESP_FAIL;
	}
	if(*ml01>max_value_pin1)
		max_value_pin1=(*ml01);
	if(*ml02>max_value_pin2)
		max_value_pin2=(*ml02);
	return ESP_OK;
}



static int tm7705_exe(int *ml_value)
{
	static uint8_t temp_onoff_bak=0;
	uint8_t temp_onoff;
	int ret=-1;
	static uint16_t init_times=0;
	static uint8_t reg_freq=0,reg_config=0;

	temp_onoff=device_config_get_number(CONFIG_TM7705_ONOFF);
    if(temp_onoff==0)
    {
		if(temp_onoff_bak)//turn off
		{
			temp_onoff_bak=0;
			tm7705_off();
			ESP_LOGI(TAG, "tm7705 turned off");
		}
		ret=0;
		goto ERROR_PROCESS;
	}  
	
	if((init_times!=device_config_get_number(CONFIG_TM7705_INIT_TIMES))||(reg_freq!=device_config_get_number(CONFIG_TM7705_REG_FREQ))||(reg_config!=device_config_get_number(CONFIG_TM7705_REG_CONFIG)))
	{
		init_times=device_config_get_number(CONFIG_TM7705_INIT_TIMES);
		reg_freq=device_config_get_number(CONFIG_TM7705_REG_FREQ);
		reg_config=device_config_get_number(CONFIG_TM7705_REG_CONFIG);
		//config changed, so need to turn off and then turn on
		if(temp_onoff_bak)
		{
			tm7705_off();
			//ESP_LOGI(TAG, "tm7705 config changed");	
			util_delay_ms(5*1000);
		}
	}
	ml_value[0]=adc_read_value(&ml_value[1],&ml_value[2]);
	temp_onoff_bak=temp_onoff;
	ret=0;

ERROR_PROCESS:
	return ret;
}
int tm7705_upload(void *pvParameters)
{
	uint8_t *ml_value_p=(uint8_t *)user_malloc(sizeof(int)*3+1);
	int *ml_value=(int *)(ml_value_p+1);
	uint8_t pins=device_config_get_number(CONFIG_TM7705_PINS_ONOFF);
	ml_value_p[0]=pins;
	ml_value[1]=pins&0x01;
	ml_value[2]=(pins>>1)&0x01;
	tm7705_exe(ml_value);
	return tcp_send_command(
	      "tm7705_period",
	      data_container_create(1,COMMAND_REQ_TM7705,ml_value_p, sizeof(int)*3+1),
	      NULL, 0, COMMAND_FLAG_FROM_DEVICE | COMMAND_FLAG_INIT_FROM_DEVICE,
	      SOCKET_HANDLER_ONE_TIME, 0,NULL);
}
void tm7705_property(uint8_t *data_out,int *data_len_out)
{
    uint32_t idex=0;
    data_out[idex++]=1;//property
	COPY_TO_BUF_WITH_NAME(read_times,data_out,idex)
	COPY_TO_BUF_WITH_NAME(read_failed_times,data_out,idex)
	COPY_TO_BUF_WITH_NAME(reset_times,data_out,idex)
	COPY_TO_BUF_WITH_NAME(init_failed_times,data_out,idex)
	COPY_TO_BUF_WITH_NAME(max_value_pin1,data_out,idex)
	COPY_TO_BUF_WITH_NAME(max_value_pin2,data_out,idex)
	*data_len_out=idex;
}
void tm7705_read(uint8_t *command_data,uint32_t command_data_len,uint8_t *data_out,int *data_len_out)
{
	uint32_t idex=0;
    uint8_t pin=0;
    uint8_t *temp_data=command_data;
    int ml_value[3]={1,0,0};//ml_value[0] 1:off, -1:error, 0:ok
    COPY_TO_VARIABLE(pin,temp_data,idex,command_data_len)
    if(pin&0x01){
    	ml_value[1]=1;
    }
    if(pin&0x02){
    	ml_value[2]=1;
    }
	tm7705_exe(ml_value);
	idex=0;
	data_out[idex++]=0;//read value
	data_out[idex++]=pin;
    memcpy(data_out+idex,ml_value,sizeof(ml_value));
    idex+=sizeof(ml_value);
    *data_len_out=idex;
}
