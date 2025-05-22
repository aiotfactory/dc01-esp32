#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include <util.h>
#include "i2cutil.h"
#include <cloud_log.h>
#include "system_util.h"

extern void tm7705_off(void);
SemaphoreHandle_t  i2c_semaphore_handle=NULL;
static const char *TAG = "i2cutil";
static uint8_t init=0;
//100000
//return 1: has already inited, 2:just inited, 0:failed
uint8_t i2c_init(uint32_t clk)
{
	clk=200000;
	if(init==0)
	{
		if(power_status_i2c()==0)
			power_onoff_i2c(1);
		if(i2c_semaphore_handle==NULL)
			i2c_semaphore_handle=xSemaphoreCreateMutex();
		i2c_config_t conf = {
			.mode = I2C_MODE_MASTER,
			.sda_io_num = 42,
			.sda_pullup_en = GPIO_PULLUP_ENABLE,
			.scl_io_num = 41,
			.scl_pullup_en = GPIO_PULLUP_ENABLE,
			.master.clk_speed = clk,
			// .clk_flags = 0,          /*!< Optional, you can use I2C_SCLK_SRC_FLAG_* flags to choose i2c source clock here. */
		};
		esp_err_t err = i2c_param_config(I2C_NUM_0, &conf);
		if (err != ESP_OK) {
			return 0;
		}
		err = i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
		if (err != ESP_OK) {
			return 0;
		}
		//ESP_LOGI(TAG, "i2c init success");
		init=1;
		return 2;
	}
	return 1;
}
void i2c_deinit(void)
{
	//ESP_LOGI(TAG, "i2c deinit");
	init=0;
	i2c_driver_delete(I2C_NUM_0);
	tm7705_off();//share the same power;
	power_onoff_i2c(0);
	util_delay_ms(3000);
    power_onoff_i2c(1);
}


esp_err_t  i2c_write(uint8_t addr,uint8_t *data,uint16_t size)
{
	esp_err_t ret=ESP_FAIL;
	if(xSemaphoreTake(i2c_semaphore_handle,portMAX_DELAY)==pdTRUE)
    {
		i2c_cmd_handle_t cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
		//i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
		i2c_master_write(cmd,data,size,ACK_CHECK_EN);
		//for(uint16_t i=0;i<size;i++)
		//	i2c_master_write_byte(cmd, data[i], ACK_CHECK_EN);
		i2c_master_stop(cmd);
		ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);
		xSemaphoreGive(i2c_semaphore_handle);
    }
    //print_format(NULL,0,"%02x","i2c_write ", "\r\n",data,size);
    return ret;
}
esp_err_t  i2c_write_reg(uint8_t addr,uint8_t reg,uint8_t *data,uint16_t size)
{
	esp_err_t ret=ESP_FAIL;
	if(xSemaphoreTake(i2c_semaphore_handle,portMAX_DELAY)==pdTRUE)
    {
		i2c_cmd_handle_t cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
		i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
		i2c_master_write(cmd,data,size,ACK_CHECK_EN);
		//for(uint16_t i=0;i<size;i++)
		//	i2c_master_write_byte(cmd, data[i], ACK_CHECK_EN);
		i2c_master_stop(cmd);
		ret= i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);
		xSemaphoreGive(i2c_semaphore_handle);
    }
    return ret;
}
esp_err_t  i2c_read(uint8_t addr,uint8_t *data,uint16_t size)
{
	esp_err_t ret=ESP_FAIL;
	if(xSemaphoreTake(i2c_semaphore_handle,portMAX_DELAY)==pdTRUE)
    {
		i2c_cmd_handle_t cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
		if (size > 1) {
			i2c_master_read(cmd, data, size - 1, ACK_VAL);
		}
		i2c_master_read_byte(cmd, data + size - 1, NACK_VAL);
		i2c_master_stop(cmd);

		ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);
		xSemaphoreGive(i2c_semaphore_handle);
    }
    //print_format(NULL,0,"%02x","i2c_read ", "\r\n",data,size);
    return ret;
}
esp_err_t  i2c_read_reg(uint8_t addr,uint8_t *reg,uint8_t reg_len,uint8_t *data,uint16_t size)
{
	esp_err_t ret=ESP_FAIL;
	if(xSemaphoreTake(i2c_semaphore_handle,portMAX_DELAY)==pdTRUE)
    {
		i2c_cmd_handle_t cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
		for(uint8_t i=0;i<reg_len;i++)
			i2c_master_write_byte(cmd, reg[i], ACK_CHECK_EN);

		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
		if (size > 1) {
			i2c_master_read(cmd, data, size - 1, ACK_VAL);
		}
		i2c_master_read_byte(cmd, data + size - 1, NACK_VAL);
		i2c_master_stop(cmd);

		ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);
		xSemaphoreGive(i2c_semaphore_handle);
    }
    return ret;
}
//return data[0]=0: msg 1:read data
int command_operate_i2c_inout(uint8_t *command,uint32_t command_len,uint8_t *data_return,int *data_return_len,uint32_t data_return_max_len)
{
	static uint8_t init=0;
	static uint32_t clock_speed_hz_current=0;
	int ret=-1;
	
	uint32_t clock_speed_hz=0;
	uint8_t flag=0,addr=0;
	uint32_t rxtx_data_len=0;
	uint8_t *tx_data=NULL;
	uint32_t idex=0;
	esp_err_t esp_ret=ESP_FAIL;
	
	COPY_TO_VARIABLE(clock_speed_hz,command,idex,command_len)
	COPY_TO_VARIABLE(flag,command,idex,command_len)
	if(flag>1)
	{
		sprintf((char *)data_return+1,"flag %u is wrong",flag);
		ret=-4;
		goto ERROR_PROCESS;
	}
	COPY_TO_VARIABLE(addr,command,idex,command_len)
	COPY_TO_VARIABLE(rxtx_data_len,command,idex,command_len)
	if(flag==1)//write
		tx_data=command+idex;

	if((init==1)&&(clock_speed_hz_current!=clock_speed_hz)){
		esp_ret=i2c_driver_delete(I2C_NUM_0);
		if(esp_ret!=ESP_OK)
		{
			sprintf((char *)data_return+1,"i2c delete %u failed due to 0x%x",I2C_NUM_0,esp_ret);
			ret=-5;
			goto ERROR_PROCESS;
		}
		init=0;
	}
	if(init==0)
	{
		esp_ret=i2c_init(clock_speed_hz);
		if(esp_ret==0)
		{
			sprintf((char *)data_return+1,"i2c init clk %lu failed due to 0x%x",clock_speed_hz,esp_ret);
			ret=-1;
			goto ERROR_PROCESS;
		}
		ESP_LOGI(TAG, "i2c init clk %lu success",clock_speed_hz);
		clock_speed_hz_current=clock_speed_hz;
		init=1;
	}
	if(flag==1)//write
	{
		cloud_printf_format("i2c write ", "", "%02x",tx_data,rxtx_data_len);
		esp_ret=i2c_write(addr,tx_data,rxtx_data_len);
		if(esp_ret!=ESP_OK)
		{
			sprintf((char *)data_return+1,"i2c write addr 0x%02x size %lu failed due to 0x%x",addr,rxtx_data_len,esp_ret);
			ret=-2;
			goto ERROR_PROCESS;
		}else {
			sprintf((char *)data_return+1,"i2c write addr 0x%02x size %lu success",addr,rxtx_data_len);
			ret=0;
			goto ERROR_PROCESS;
		}
	}else if(flag==0)//read
	{
		esp_ret=i2c_read(addr,data_return+1,rxtx_data_len);
		if(esp_ret!=ESP_OK)
		{
			bzero(data_return,data_return_max_len);
			sprintf((char *)data_return+1,"i2c read addr 0x%02x size %lu failed due to 0x%x",addr,rxtx_data_len,esp_ret);
			ret=-3;
			goto ERROR_PROCESS;
		}
		data_return[0]=1;
		*data_return_len=rxtx_data_len+1;
		cloud_printf_format("i2c read ", "", "%02x",data_return+1,rxtx_data_len);
		ESP_LOGI(TAG, "i2c read addr 0x%02x size %lu success",addr,rxtx_data_len);
		return 0;
	}	
ERROR_PROCESS:
	data_return[0]=0;
	*data_return_len=strlen((char *)data_return+1)+1;
	if(ret==0)
		ESP_LOGI(TAG, "%s", (char *)data_return+1);
	else if(ret<0)
		ESP_LOGW(TAG, "%s", (char *)data_return+1);
	return ret;
}



















