#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "esp_types.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include <math.h>
#include "util.h"
#include "i2cutil.h"
#include <device_config.h>
#include <network_util.h>
#include "system_util.h"

//气压测量速率(sample/sec),Background 模式使用
#define  PM_RATE_1          (0<<4)      //1 measurements pr. sec.
#define  PM_RATE_2          (1<<4)      //2 measurements pr. sec.
#define  PM_RATE_4          (2<<4)      //4 measurements pr. sec.
#define  PM_RATE_8          (3<<4)      //8 measurements pr. sec.
#define  PM_RATE_16         (4<<4)      //16 measurements pr. sec.
#define  PM_RATE_32         (5<<4)      //32 measurements pr. sec.
#define  PM_RATE_64         (6<<4)      //64 measurements pr. sec.
#define  PM_RATE_128        (7<<4)      //128 measurements pr. sec.

//气压重采样速率(times),Background 模式使用
#define PM_PRC_1            0       //Sigle         kP=524288   ,3.6ms
#define PM_PRC_2            1       //2 times       kP=1572864  ,5.2ms
#define PM_PRC_4            2       //4 times       kP=3670016  ,8.4ms
#define PM_PRC_8            3       //8 times       kP=7864320  ,14.8ms
#define PM_PRC_16           4       //16 times      kP=253952   ,27.6ms
#define PM_PRC_32           5       //32 times      kP=516096   ,53.2ms
#define PM_PRC_64           6       //64 times      kP=1040384  ,104.4ms
#define PM_PRC_128          7       //128 times     kP=2088960  ,206.8ms

//温度测量速率(sample/sec),Background 模式使用
#define  TMP_RATE_1         (0<<4)      //1 measurements pr. sec.
#define  TMP_RATE_2         (1<<4)      //2 measurements pr. sec.
#define  TMP_RATE_4         (2<<4)      //4 measurements pr. sec.
#define  TMP_RATE_8         (3<<4)      //8 measurements pr. sec.
#define  TMP_RATE_16        (4<<4)      //16 measurements pr. sec.
#define  TMP_RATE_32        (5<<4)      //32 measurements pr. sec.
#define  TMP_RATE_64        (6<<4)      //64 measurements pr. sec.
#define  TMP_RATE_128       (7<<4)      //128 measurements pr. sec.

//温度重采样速率(times),Background 模式使用
#define TMP_PRC_1           0       //Sigle
#define TMP_PRC_2           1       //2 times
#define TMP_PRC_4           2       //4 times
#define TMP_PRC_8           3       //8 times
#define TMP_PRC_16          4       //16 times
#define TMP_PRC_32          5       //32 times
#define TMP_PRC_64          6       //64 times
#define TMP_PRC_128         7       //128 times

//SPL06_MEAS_CFG
#define MEAS_COEF_RDY       0x80
#define MEAS_SENSOR_RDY     0x40        //传感器初始化完成
#define MEAS_TMP_RDY        0x20        //有新的温度数据
#define MEAS_PRS_RDY        0x10        //有新的气压数据

#define MEAS_CTRL_Standby               0x00    //空闲模式
#define MEAS_CTRL_PressMeasure          0x01    //单次气压测量
#define MEAS_CTRL_TempMeasure           0x02    //单次温度测量
#define MEAS_CTRL_ContinuousPress       0x05    //连续气压测量
#define MEAS_CTRL_ContinuousTemp        0x06    //连续温度测量
#define MEAS_CTRL_ContinuousPressTemp   0x07    //连续气压温度测量

//FIFO_STS
#define SPL06_FIFO_FULL     0x02
#define SPL06_FIFO_EMPTY    0x01

//INT_STS
#define SPL06_INT_FIFO_FULL     0x04
#define SPL06_INT_TMP           0x02
#define SPL06_INT_PRS           0x01

//CFG_REG
#define SPL06_CFG_T_SHIFT   0x08    //oversampling times>8时必须使用
#define SPL06_CFG_P_SHIFT   0x04

#define SP06_PSR_B2     0x00        //气压值
#define SP06_PSR_B1     0x01
#define SP06_PSR_B0     0x02
#define SP06_TMP_B2     0x03        //温度值
#define SP06_TMP_B1     0x04
#define SP06_TMP_B0     0x05

#define SP06_PSR_CFG    0x06        //气压测量配置
#define SP06_TMP_CFG    0x07        //温度测量配置
#define SP06_MEAS_CFG   0x08        //测量模式配置

#define SP06_CFG_REG    0x09
#define SP06_INT_STS    0x0A
#define SP06_FIFO_STS   0x0B

#define SP06_RESET      0x0C
#define SP06_ID         0x0D

#define SP06_COEF       0x10        //-0x21
#define SP06_COEF_SRCE  0x28


#define SPL_ADDR 0x77
//#define SPL_ADDR 0x76

static int32_t _kT=0,_kP=0;
static int16_t _C0=0,_C1=0,_C01=0,_C11=0,_C20=0,_C21=0,_C30=0;
static int32_t _C00=0,_C10=0;
static uint8_t inited=0;
static uint32_t times_success=0, times_total=0;
static int temperature_max=-99999999,temperature_min=99999999,pressure_max=-99999999,pressure_min=99999999;

int temperature_spl06=-99999999,pressure_spl06=-99999999;


static const char *TAG = "spl06001";

static uint8_t spl06_write_reg(uint8_t reg_addr,uint8_t value)
{
	if(i2c_write_reg(SPL_ADDR,reg_addr,&value,1)==ESP_OK)
		return 0;
	return 1;
}
static uint8_t spl06_read_reg(uint8_t reg_addr)
{
	uint8_t data=0;
	i2c_read_reg(SPL_ADDR,&reg_addr,1,&data,1);
	return data;
}

static void spl06_start(uint8_t mode)
{
    spl06_write_reg(SP06_MEAS_CFG, mode);//测量模式配置
}

static void spl06_config_temperature(uint8_t rate,uint8_t oversampling)
{
    switch(oversampling)
    {
	case TMP_PRC_1:
		_kT = 524288;
		break;
	case TMP_PRC_2:
		_kT = 1572864;
		break;
	case TMP_PRC_4:
		_kT = 3670016;
		break;
	case TMP_PRC_8:
		_kT = 7864320;
		break;
	case TMP_PRC_16:
		_kT = 253952;
		break;
	case TMP_PRC_32:
		_kT = 516096;
		break;
	case TMP_PRC_64:
		_kT = 1040384;
		break;
	case TMP_PRC_128:
		_kT = 2088960;
		break;
    }
    if(oversampling > TMP_PRC_8)
    {
    	uint8_t temp = spl06_read_reg(SP06_CFG_REG);
        spl06_write_reg(SP06_CFG_REG,temp|SPL06_CFG_T_SHIFT);
    }
    spl06_write_reg(SP06_TMP_CFG,rate|oversampling|(1<<7));   //温度每秒128次测量一次（最快速度）
}
//不是所有组合都可以设置，有的组合是无效的，要根据datasheet的page19来设置
static void spl06_config_pressure(uint8_t rate,uint8_t oversampling)//设置补偿系数及采样速率
{
    switch(oversampling)
    {
	case PM_PRC_1:
		_kP = 524288;
		break;
	case PM_PRC_2:
		_kP = 1572864;
		break;
	case PM_PRC_4:
		_kP = 3670016;
		break;
	case PM_PRC_8:
		_kP = 7864320;
		break;
	case PM_PRC_16:
		_kP = 253952;
		break;
	case PM_PRC_32:
		_kP = 516096;
		break;
	case PM_PRC_64:
		_kP = 1040384;
		break;
	case PM_PRC_128:
		_kP = 2088960;
		break;
    }

    if(oversampling > PM_PRC_8)
    {
    	uint8_t temp = spl06_read_reg(SP06_CFG_REG);
        spl06_write_reg(SP06_CFG_REG,temp|SPL06_CFG_P_SHIFT);
    }
    spl06_write_reg(SP06_PSR_CFG,rate|oversampling);

}

static uint8_t spl06_update(float *Temp,float *Press)//获取并计算出温度值、气压值
{
    uint8_t h[3] = {0};
 	h[0] = spl06_read_reg(0x03);
 	h[1] = spl06_read_reg(0x04);
 	h[2] = spl06_read_reg(0x05);
 	int32_t i32rawTemperature = (int32_t)h[0]<<16 | (int32_t)h[1]<<8 | (int32_t)h[2];
    i32rawTemperature= (i32rawTemperature&0x800000) ? (0xFF000000|i32rawTemperature) : i32rawTemperature;

    float fTCompensate;
    float fTsc,fPsc;
    float qua2, qua3;
    float fPCompensate;
    fTsc = i32rawTemperature / (float)_kT;
    fTCompensate =  _C0 * 0.5 + _C1 * fTsc;//温度

	h[0] = spl06_read_reg(0x00);
	h[1] = spl06_read_reg(0x01);
	h[2] = spl06_read_reg(0x02);

	int32_t i32rawPressure = (int32_t)h[0]<<16 | (int32_t)h[1]<<8 | (int32_t)h[2];
    i32rawPressure= (i32rawPressure&0x800000) ? (0xFF000000|i32rawPressure) : i32rawPressure;

    *Temp=fTCompensate;
    fPsc = i32rawPressure / (float)_kP;

    qua2 = _C10 + fPsc * (_C20 + fPsc* _C30);
    qua3 = fTsc * fPsc * (_C11 + fPsc * _C21);
	//qua3 = 0.9f *fTsc * fPsc * (_C11 + fPsc * _C21);
    fPCompensate = _C00 + fPsc * qua2 + fTsc * _C01 + qua3;
	//fPCompensate = _C00 + fPsc * qua2 + 0.9f *fTsc  * _C01 + qua3;
    *Press=fPCompensate;

    return 0;
}
/*
static float caculate_altitude(float GasPress)
{
	float Altitude=0;
	Altitude =(44330.0f *(1.0-pow((float)(GasPress) / 101325.0f,1.0f/5.255f)));
	//Altitude =(44300.0f *(1.0f-pow((float)(GasPress) / 101325.0f,1.0f/5.256f)));
	return Altitude;
}
*/
static esp_err_t spl06_init(uint8_t retrytimes)
{
	uint8_t ret;
	ret=i2c_init(100000);
	if(ret==0)
	{
		inited=0;
		return ESP_FAIL;
	}
	if(ret==2)
		inited=0;
	if(inited==0)
	{

		for(uint8_t i=0;i<retrytimes;i++)
		{
			util_delay_ms(30);
			if(spl06_write_reg(SP06_RESET,0x89))
			{
				//ESP_LOGE(TAG, "reset failed");
				continue;
			}
			util_delay_ms(100);
			//ESP_LOGI(TAG, "reset %02x",spl06_read_reg(SP06_RESET));
			uint8_t id = spl06_read_reg(SP06_ID);
			if(id != 0x10)
			{
				//ESP_LOGE(TAG, "version wrong");
				continue;
			}
			util_delay_ms(30);
			uint8_t delayedtimes=0;
			while((spl06_read_reg(SP06_MEAS_CFG)&MEAS_COEF_RDY)!=MEAS_COEF_RDY)
			{
				util_delay_ms(30);
				delayedtimes++;
				if(delayedtimes>=5)
					break;
			}
			if(delayedtimes>=5)
				continue;

			unsigned long h;
			unsigned long m;
			unsigned long l;
			h =  spl06_read_reg(0x10);
			l  =  spl06_read_reg(0x11);
			_C0 = (int16_t)h<<4 | l>>4;
			_C0 = (_C0&0x0800)?(0xF000|_C0):_C0;
			h =  spl06_read_reg( 0x11);
			l  =  spl06_read_reg( 0x12);
			_C1 = (int16_t)(h&0x0F)<<8 | l;
			_C1 = (_C1&0x0800)?(0xF000|_C1):_C1;
			h =  spl06_read_reg( 0x13);
			m =  spl06_read_reg( 0x14);
			l =  spl06_read_reg( 0x15);
			_C00 = (int32_t)h<<12 | (int32_t)m<<4 | (int32_t)l>>4;
			_C00 = (_C00&0x080000)?(0xFFF00000|_C00):_C00;
			h =  spl06_read_reg( 0x15);
			m =  spl06_read_reg( 0x16);
			l =  spl06_read_reg( 0x17);
			_C10 = (int32_t)h<<16 | (int32_t)m<<8 | l;
			_C10 = (_C10&0x080000)?(0xFFF00000|_C10):_C10;
			h =  spl06_read_reg( 0x18);
			l  =  spl06_read_reg( 0x19);
			_C01 = (int16_t)h<<8 | l;
			h =  spl06_read_reg( 0x1A);
			l  =  spl06_read_reg( 0x1B);
			_C11 = (int16_t)h<<8 | l;
			h =  spl06_read_reg( 0x1C);
			l  =  spl06_read_reg( 0x1D);
			_C20 = (int16_t)h<<8 | l;
			h =  spl06_read_reg( 0x1E);
			l  =  spl06_read_reg( 0x1F);
			_C21 = (int16_t)h<<8 | l;
			h =  spl06_read_reg( 0x20);
			l  =  spl06_read_reg( 0x21);
			_C30 = (int16_t)h<<8 | l;

			util_delay_ms(30);
			delayedtimes=0;
			while((spl06_read_reg(SP06_MEAS_CFG)&MEAS_SENSOR_RDY)!=MEAS_SENSOR_RDY)
			{
				util_delay_ms(30);
				delayedtimes++;
				if(delayedtimes>=5)
					break;
			}
			if(delayedtimes>=5)
				continue;

			spl06_config_pressure(PM_RATE_2,PM_PRC_128);
			spl06_config_temperature(PM_RATE_2,TMP_PRC_8);

			spl06_start(MEAS_CTRL_ContinuousPressTemp); //启动连续的气压温度测量
			//ESP_LOGI(TAG, "Spl06 init success");
			inited=1;
			util_delay_ms(500);
			float t1,t2;
			spl06_update(&t1,&t2);//skip the first
			return ESP_OK;
		}
		ESP_LOGW(TAG, "Spl06 init fail");
		return ESP_FAIL;
	}
	return ESP_OK;
}

static uint8_t spl06_read_exe(uint8_t *flag,int *temperature,int *pressure,uint8_t *property,uint8_t retry_times)
{
	uint32_t idex=0;
	*flag=0; *pressure=0; *temperature=0;
	times_total++;

	if(spl06_init(5)==ESP_FAIL){
		*flag=2;
		goto ERROR_PROCESS;
	}

    float temp =0;
    float pa =0;
    for(uint8_t i=0;i<retry_times;i++)
    {
	  	spl06_update(&temp,&pa);
	  	*temperature=temp*100;
	  	*pressure=pa*100;
	  	//ESP_LOGI(TAG, "spl06 temperature %d pressure %d",*temperature,*pressure);
		if((*temperature>-5000)&&(*temperature<9900)&&(*pressure>9082283)&&(*pressure<15282359))
		{
			times_success++;
			if(*temperature>temperature_max) temperature_max=*temperature;
			if(*temperature<temperature_min) temperature_min=*temperature;
			if(*pressure>pressure_max) pressure_max=*pressure;
			if(*pressure<pressure_min) pressure_min=*pressure;
			variable_set(&temperature_spl06,temperature, sizeof(int));
			variable_set(&pressure_spl06,pressure, sizeof(int));
			break;
		}
		if(i==(retry_times-1)){
			*flag=3;
			goto ERROR_PROCESS;
		}
	}
ERROR_PROCESS:	
	COPY_TO_BUF_NO_CHECK(times_total,property,idex)
	COPY_TO_BUF_NO_CHECK(times_success,property,idex)
	COPY_TO_BUF_NO_CHECK(temperature_max,property,idex)
	COPY_TO_BUF_NO_CHECK(temperature_min,property,idex)
	COPY_TO_BUF_NO_CHECK(pressure_max,property,idex)
	COPY_TO_BUF_NO_CHECK(pressure_min,property,idex)
	if(*flag!=0)
	{
		inited=0;
		i2c_deinit();
		ESP_LOGW(TAG, "spl06 read failed due to %d",*flag);
	}
	return *flag;
}
#define SPL06_DATA_LEN 33
int spl06_upload(void *parameter)
{
	uint8_t *temp_buff=user_malloc(SPL06_DATA_LEN);
	spl06_read_exe(temp_buff,(int *)(temp_buff+1),(int *)(temp_buff+5),temp_buff+9,3);
	return tcp_send_command(
	      "spl06",
	      data_container_create(1,COMMAND_REQ_SPL06,temp_buff,SPL06_DATA_LEN),
	      NULL, 0, COMMAND_FLAG_FROM_DEVICE | COMMAND_FLAG_INIT_FROM_DEVICE,
	      SOCKET_HANDLER_ONE_TIME, 0,NULL);
}
void spl06_request(uint8_t *data_out,int *data_len_out)
{
	uint8_t *temp_buff=data_out;
	spl06_read_exe(temp_buff,(int *)(temp_buff+1),(int *)(temp_buff+5),temp_buff+9,3);
	*data_len_out=SPL06_DATA_LEN;
}
