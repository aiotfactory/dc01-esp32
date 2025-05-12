#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/gpio.h"
#include "system_util.h"
#include <device_config.h>
#include <util.h>
#include <network_util.h>
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"
#include "i2cutil.h"

static paramsMLX90640 *MLXPars;
#define MLXFrameU16_LEN 834
static uint16_t *MLXFrameU16;
#define MLXTempFloat_LEN 768
float *MLXTempFloat;

static const char *TAG = "thermal_util";

static uint32_t times_total=0;
static int16_t max=-30000,min=30000,avg_max=-30000,avg_min=30000;

static uint8_t inited=0;

static uint32_t config_onoff=0,config_interval_seconds=0,config_emissivity=0,config_shift=0;
static float config_emissivity_float=0.95f,config_shift_float=8.0f;

#if 0 
static void data_print(void)
{
	uint16_t i,j,imin=0,jmin=0,imax=0,jmax=0;
	float minf=1000.00f,maxf=-1000.00f,avgf=0.00f;
	double avgf2=0.00;
	float element=0.0f;
	uint16_t elementidx=0;


	for(i=0;i<24;i++)
	{
		//printf("\r\n");
		for(j=0;j<32;j++)
		{
			elementidx=i*32+j;
			//if(displayType==1)
			//	elementidx=i*24+(32-j-1);
			element=MLXTempFloat[elementidx];
			if((i==0)&&(j==0))
				avgf=element;
			else
				avgf=(avgf+element)/2;
			avgf2=avgf2+element;
			if(element>maxf) //最大
			{
				maxf=element;
				imax=i;
				jmax=j;
			}
			if(element<minf) //最小
			{
				minf=element;
				imin=i;
				jmin=j;
			}
			//printf("%.1f ",element);
		}
	}
	//printf("\r\n");
	avgf2=avgf2/(24*32);
	printf("min %6.3f(%u,%u) max %6.3f(%u,%u) avg1 %6.3f avg2 %6.3f control reg %04x sub page %u\r\n",minf,imin,jmin,maxf,imax,jmax,avgf,avgf2,MLXFrameU16[832],MLXFrameU16[833]);
}
#endif
static uint8_t mlx90640_next(void)
{
	static uint8_t currentPage=1;
	int error = 1;
	uint16_t dataReady = 0;
	uint16_t statusRegister;
    while(dataReady == 0)
    {
        error = MLX90640_I2CRead(MLX_ADDR, 0x8000, 1, &statusRegister);
        if(error != 0)
        {
            return error;
        }
        dataReady = statusRegister & 0x0008;
        if(dataReady!=0)
        {
        	if(currentPage!=(statusRegister&0x0001))
        	{
        		currentPage=statusRegister&0x0001;
        	}else
        		dataReady=0;
        }
    }

    return 1;
}


static int mlx90640_read(void)
{
	int ret=0;
	static float MLXTa,MLXTr;
    if(mlx90640_next()==1)
    {
    	for(int i=0;i<4;i++)
    	{
			ret=MLX90640_GetFrameData(MLX_ADDR,MLXFrameU16);
			if(ret>=0)
			{
				MLXTa=MLX90640_GetTa(MLXFrameU16, MLXPars);     //计算实时外壳温度
				MLXTr=MLXTa-config_shift_float;         //计算环境温度用于温度补偿 手册上说的环境温度可以用外壳温度-8℃
				MLX90640_CalculateTo(MLXFrameU16, MLXPars, config_emissivity_float, MLXTr, MLXTempFloat);    //计算像素点温度
				MLX90640_BadPixelsCorrection(MLXPars->brokenPixels, MLXTempFloat, 1, MLXPars);
				MLX90640_BadPixelsCorrection(MLXPars->outlierPixels, MLXTempFloat, 1, MLXPars);
				return 1;
			}else{
				ESP_LOGW(TAG, "frame read error");
			}
    	}
    }
    return -2;
}
static uint8_t *data_prepare(int *data_out_len)
{
	int temp_buf_idx=0;
	float avg_temp=0.0f,temp=0.0f;
	uint16_t element;	
	uint8_t *temp_buf=NULL;

	if(mlx90640_read()<0){
		*data_out_len=0;
		return NULL;
	}
	
	temp_buf=user_malloc(MLXTempFloat_LEN*sizeof(uint16_t)+512);
	
	
	for(uint8_t i=0;i<24;i++)
	{
		for(uint8_t j=0;j<32;j++)
		{
			temp=MLXTempFloat[i*32+j];
			element = (uint16_t)((temp + 0.5)*10);
			memcpy(temp_buf+temp_buf_idx,&element,sizeof(element));
			temp_buf_idx+=sizeof(element);
			
			avg_temp+=element;
			if(element>max) max=element;
			if(element<min) min=element;
		}
	}
	avg_temp=avg_temp/(24*32);		
	if(avg_temp>avg_max) avg_max=avg_temp;
	if(avg_temp<avg_min) avg_min=avg_temp;
	times_total++;
				
	COPY_TO_BUF_WITH_NAME(times_total,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(max,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(min,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(avg_max,temp_buf,temp_buf_idx);
	COPY_TO_BUF_WITH_NAME(avg_min,temp_buf,temp_buf_idx);
	
	*data_out_len=temp_buf_idx;
	
	return temp_buf;
}
static int thermal_init(void)
{
	int ret=-1;
	if(inited==0)
    {
		config_onoff=device_config_get_number(CONFIG_THERMAL_ONOFF);
		if(config_onoff==0) return -4;
		
		config_interval_seconds=device_config_get_number(CONFIG_THERMAL_UPLOAD_INTERVAL_SECONDS);
		config_emissivity=device_config_get_number(CONFIG_THERMAL_EMISSIVITY);
		config_shift=device_config_get_number(CONFIG_THERMAL_SHIFT);
		
		config_emissivity_float=config_emissivity;
		config_emissivity_float=config_emissivity_float/100;
		config_shift_float=config_shift;
		config_shift_float=config_shift_float/100;

    	power_onoff_spi(1);
	    if (i2c_init(100000) == 0) {
	    	ESP_LOGW(TAG, "thermal_upload i2c init failed");
	    	return -1;
	  	}
	    ret=MLX90640_I2CInit();
	    if(ret<0) return -2;
	    MLXPars=(paramsMLX90640 *)user_malloc(sizeof(paramsMLX90640));
	    MLXFrameU16=(uint16_t *)user_malloc(MLXFrameU16_LEN*sizeof(uint16_t));
	    MLXTempFloat=(float *)user_malloc(MLXTempFloat_LEN*(sizeof(float)));
	    ret=MLX90640_SetRefreshRate(MLX_ADDR, 4);
		//ESP_LOGI(TAG,"MLX90640_SetRefreshRate %d",ret);
		ret=MLX90640_DumpEE(MLX_ADDR, MLXFrameU16);
		//ESP_LOGI(TAG,"MLX90640_DumpEE %d",ret);
		ret=MLX90640_ExtractParameters(MLXFrameU16, MLXPars);
		//ESP_LOGI(TAG,"MLX90640_ExtractParameters %d",ret);
		uint16_t tmp=0;
		ret = MLX90640_I2CRead(MLX_ADDR, 0x8000, 1, &tmp);
		//ESP_LOGI(TAG,"status register %04x %d",tmp,ret);
		tmp=0;
		ret= MLX90640_I2CRead(MLX_ADDR, 0x800D, 1, &tmp);
		//ESP_LOGI(TAG,"control register %04x %d",tmp,ret);
		//tmp=0;
		//ret= MLX90640_I2CRead(MLX_ADDR, 0x800F, 1, &tmp);
		//ESP_LOGI("config register %04x %d",tmp,ret);
		bzero(MLXFrameU16,MLXTempFloat_LEN*(sizeof(uint16_t)));
		ret=mlx90640_read();
		ret=mlx90640_read();
		ret=mlx90640_read();
		if(ret<0)
		{
			user_free(MLXPars);
			user_free(MLXFrameU16);
			user_free(MLXTempFloat);
			return -3;
		}
	    inited=1;
    }
    return 1;
}
int thermal_upload(void *parameter)
{
    int ret=thermal_init();
	if(ret>0) 
	{
		int temp_buf_idx=0;
		uint8_t *temp_buf=data_prepare(&temp_buf_idx);
		if(temp_buf!=NULL){
			ret=tcp_send_command(
		      "thermal",
		      data_container_create(1,COMMAND_REQ_THERMAL,temp_buf, temp_buf_idx),
		      NULL, 0, COMMAND_FLAG_FROM_DEVICE | COMMAND_FLAG_INIT_FROM_DEVICE,
		      SOCKET_HANDLER_ONE_TIME, 0,NULL);
	    }else 
	    	ret=-1;
	}
	return ret;
}
int thermal_request(data_send_element *data_element)
{
    int ret=thermal_init();
    if(ret>0)
    {
		int temp_buf_idx=0;
		uint8_t *temp_buf=data_prepare(&temp_buf_idx);
		if((temp_buf!=NULL)&&(temp_buf_idx>0))
		{
			data_element->data=temp_buf;
			data_element->data_len=temp_buf_idx;
			data_element->need_free=1;
			return 0;
		}else  
		 	ret=-1;
	}
	return ret;
}
