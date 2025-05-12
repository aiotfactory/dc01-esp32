#include "esp_err.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <util.h>
#include <system_util.h>
#include <spi_util.h>
#include <cloud_log.h>
#include <device_config.h>

//#define SPI_PRINT
#define SPI_WAIT_MS_SECONDS 50
static const char *TAG = "spiutil";
static uint8_t spi2_bus_inited=0;




SemaphoreHandle_t  spi_semaphore_handle=NULL;

typedef struct {
    spi_device_handle_t device_handle;
    uint8_t inited;
  	char *device_name;
  	uint32_t tx_times;
  	uint32_t rx_times;
  	uint32_t failed_times;
} spi_util_device_info;

static spi_util_device_info all_spi_device[SPI_ALL]={0};

static bool spi_util_lock(char *name)
{
	//ESP_LOGI(TAG, "spi_util_lock %s",name);
	//xSemaphoreTake(spi_semaphore_handle,portMAX_DELAY);
	bool ret= (xSemaphoreTake(spi_semaphore_handle,portMAX_DELAY) == pdTRUE);
	if(ret==false)
		ESP_LOGE(TAG, "spi_util_lock failed %s",name);
	return ret;
}
static void spi_util_unlock(char *name)
{
	//ESP_LOGI(TAG, "spi_util_unlock start %s",name);
	xSemaphoreGive(spi_semaphore_handle);
	//ESP_LOGI(TAG, "spi_util_unlock end %s",name);
}
static esp_err_t spi_device_transmit_safe(spi_util_device_info *device,spi_transaction_t *trans_desc)
{
	esp_err_t ret=ESP_FAIL;
	//ESP_LOGI(TAG, "spi_device_transmit_safe1 %s",device_name);
	if((device->device_handle==NULL)||(device->inited==0))
		return ret;
	//ret=spi_device_acquire_bus(device->device_handle, pdMS_TO_TICKS(300));
    //if (ret != ESP_OK) {
    //    return ret;
    //}
	//ESP_LOGI(TAG, "spi_device_transmit_safe2 %s",device_name);
	ret= spi_device_polling_transmit(device->device_handle,trans_desc);
	if(ret!=ESP_OK)
		device->failed_times++;
	//ESP_LOGI(TAG, "spi_device_transmit_safe3 %s",device_name);
	//spi_device_release_bus(device->device_handle);
	//ESP_LOGI(TAG, "spi_device_transmit_safe4 %s",device_name);
	return ret;
}
static esp_err_t spi_bus_add_device_safe(char *device_name,spi_host_device_t host_id, const spi_device_interface_config_t *dev_config, spi_device_handle_t *handle)
{
	if(spi_util_lock(device_name))
	{
		esp_err_t ret= spi_bus_add_device(host_id,dev_config, handle);
		spi_util_unlock(device_name);
		return ret;
	}
	return ESP_FAIL;
}
static esp_err_t spi_bus_remove_device_safe(char *device_name,spi_device_handle_t handle)
{
	if(spi_util_lock(device_name))
	{
		esp_err_t ret= spi_bus_remove_device(handle);
		spi_util_unlock(device_name);
		return ret;
	}
	return ESP_FAIL;
}
void spiutil_deinit(spiutil_device_t device)
{
	if(all_spi_device[device].inited==1)
	{
		spi_bus_remove_device_safe(all_spi_device[device].device_name,all_spi_device[device].device_handle);
		all_spi_device[device].inited=0;
		all_spi_device[device].device_handle=NULL;
		ESP_LOGI(TAG, "spiutil_deinit %s",all_spi_device[device].device_name);
	}
}
void spi_cs_pins_default(void)
{
	ch423_output(CH423_IO5, 1);//pull up tm7705
    ch423_output(CH423_IO7, 1);//pull up lora
	ch423_output(CH423_OC_IO4, 1);//pull up w5500
	ch423_output(CH423_OC_IO9, 1);//pull up spi cs1
	ch423_output(CH423_OC_IO12, 1);//pull up spi cs2	
}
static void spi_cs_w5500_unlock(void)
{
	ch423_output(CH423_OC_IO4, 1);
	spi_util_unlock("w5500");
}
static bool spi_cs_w5500_lock(void)
{
	//ESP_LOGI(TAG, "spi_cs_w5500_lock");
	if(spi_util_lock("w5500"))
	{
		ch423_output(CH423_OC_IO4, 0);
		return ESP_OK;
	}
	return ESP_FAIL;
}
void spi_cs_w5500_post_cb(spi_transaction_t* t)
{
	spi_cs_w5500_unlock();
}
void spi_cs_w5500_pre_cb(spi_transaction_t* t)
{
	spi_cs_w5500_lock();
}
static void spi_cs_tm7705_post_cb(spi_transaction_t* t)
{
	//ESP_LOGI(TAG, "spi_cs_tm7705_post_cb");
	ch423_output(CH423_IO5, 1);
	spi_util_unlock("tm7705");
}
static void spi_cs_tm7705_pre_cb(spi_transaction_t* t)
{
	//ESP_LOGI(TAG, "spi_cs_tm7705_pre_cb");
	if(spi_util_lock("tm7705"))
		ch423_output(CH423_IO5, 0);
}
static void spi_cs_ext2_post_cb(spi_transaction_t* t)
{
	ch423_output(CH423_OC_IO12, 1);
	spi_util_unlock("ext2");
}
static void spi_cs_ext2_pre_cb(spi_transaction_t* t)
{
	if(spi_util_lock("ext2"))
		ch423_output(CH423_OC_IO12, 0);
}
static void spi_cs_ext1_post_cb(spi_transaction_t* t)
{
	ch423_output(CH423_OC_IO9, 1);
	spi_util_unlock("ext1");
}
static void spi_cs_ext1_pre_cb(spi_transaction_t* t)
{
	if(spi_util_lock("ext1"))
		ch423_output(CH423_OC_IO9, 0);
}
void spi_property(uint8_t *data_out,int *data_len_out)
{
	uint8_t temp_buf_len=128;
	char *temp_buf=(char *)user_malloc(temp_buf_len);
	char spi_name[10];
	*data_len_out=0;
	//spi2_bus_inited
	property_value_add(data_out,data_len_out,"spi_bus",7,0,sizeof(spi2_bus_inited),&spi2_bus_inited);
	//(all_spi_device[i].inited
	if(spi2_bus_inited>0)
	{
		for(uint8_t i=0;i<SPI_ALL;i++)
		{
			bzero(temp_buf,temp_buf_len);
			if(all_spi_device[i].device_name==NULL)
				all_spi_device[i].device_name="noname";
			sprintf(temp_buf,"%u.%s:inited=%u,tx=%lu,rx=%lu,fail=%lu",i,all_spi_device[i].device_name,all_spi_device[i].inited,all_spi_device[i].tx_times,all_spi_device[i].rx_times,all_spi_device[i].failed_times);
			bzero(spi_name,sizeof(spi_name));
			sprintf(spi_name,"spi%02u",i);
			property_value_add(data_out,data_len_out,spi_name,strlen(spi_name),1,strlen(temp_buf),(uint8_t *)temp_buf);
		}
	}
	user_free(temp_buf);
	temp_buf=NULL;
}
spi_device_handle_t spiutil_init(spiutil_device_t device, char *device_name,spi_device_interface_config_t *devcfg_in)
{
	esp_err_t ret;
	if (spi2_bus_inited == 0) 
	{
		//spi_semaphore_handle=xSemaphoreCreateBinary();
		if(spi_semaphore_handle==NULL)
			spi_semaphore_handle=xSemaphoreCreateMutex();
		if(spi_semaphore_handle==NULL)
			ESP_LOGE(TAG,"spi_semaphore_handle create error");
		spi_util_lock("init spi");
		if (spi2_bus_inited == 0)
		{
			spi2_bus_inited = 1;
			spi_cs_pins_default();
			spi_bus_config_t buscfg = { .mosi_io_num = SPI_MOSI, .miso_io_num =
					SPI_MISO, .sclk_io_num = SPI_SCK, .quadwp_io_num = -1,
					.quadhd_io_num = -1 };
			//Initialize the SPI bus and add the device we want to send stuff to.
			ret = spi_bus_initialize(SPI_HOST, &buscfg,SPI_DMA_CH_AUTO);
	 		if(ret != ESP_OK)        
	 			util_reboot(6);
		}
		spi_util_unlock("init spi");
	}
	if((device<SPI_ALL)&&(all_spi_device[device].inited==1))
		return all_spi_device[device].device_handle;
		
	if (SPI_TM7705 == device) {
		all_spi_device[SPI_TM7705].inited=1;
		all_spi_device[SPI_TM7705].device_name=device_name;
		spi_cs_pins_default();
		//Configuration for the SPI device on the other side of the bus
		spi_device_interface_config_t devcfg = { .command_bits = 0,
				.address_bits = 8, .dummy_bits = 0, .clock_speed_hz = 200000,
				.duty_cycle_pos = 128,        //50% duty cycle
				.mode = 0, .spics_io_num = -1, .cs_ena_posttrans = 3, //Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
				.queue_size = 3, .pre_cb=spi_cs_tm7705_pre_cb, .post_cb=spi_cs_tm7705_post_cb };
		ret = spi_bus_add_device_safe("init tm7705",SPI_HOST, &devcfg, &(all_spi_device[SPI_TM7705].device_handle));
 		if(ret != ESP_OK)        
 			util_reboot(7);
		ESP_LOGI(TAG, "spi_bus_add_device %s",device_name);
		return all_spi_device[SPI_TM7705].device_handle;
	}
	if ((SPI_EXT1 == device)|| (SPI_EXT2 == device)){
		all_spi_device[device].inited=1;
		all_spi_device[device].device_name=device_name;
		spi_cs_pins_default();
		ret = spi_bus_add_device_safe(device_name,SPI_HOST, devcfg_in, &(all_spi_device[device].device_handle));
 		if(ret != ESP_OK)        
 			util_reboot(8);
		ESP_LOGI(TAG, "spi_bus_add_device %s",device_name);
		return all_spi_device[device].device_handle;
	}	
	if (SPI_W5500 == device) {
		all_spi_device[SPI_W5500].inited=1;
		all_spi_device[SPI_W5500].device_name=device_name;
		spi_cs_pins_default();
		if(devcfg_in!=NULL){
			ret = spi_bus_add_device_safe(device_name,SPI_HOST, devcfg_in, &(all_spi_device[device].device_handle));
	 		if(ret != ESP_OK)        
	 			util_reboot(8);
 		}
		ESP_LOGI(TAG, "spi_bus_add_device %s",device_name);
		return all_spi_device[device].device_handle;
	}
	if (SPI_LORA == device) {
		all_spi_device[SPI_LORA].inited=1;
		all_spi_device[SPI_LORA].device_name=device_name;
		spi_cs_pins_default();
		//Configuration for the SPI device on the other side of the bus
		spi_device_interface_config_t devcfg = { .command_bits = 0,
				.address_bits = 0, .dummy_bits = 0, .clock_speed_hz = 5000000,
				.duty_cycle_pos = 128,        //50% duty cycle
				.mode = 0, .spics_io_num = -1, .cs_ena_posttrans = 3, //Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
				.queue_size = 3 };
		ret = spi_bus_add_device_safe("init lora",SPI_HOST, &devcfg,&(all_spi_device[SPI_LORA].device_handle));
 		if(ret != ESP_OK)        
 			util_reboot(6);
		ESP_LOGI(TAG, "spi_bus_add_device %s",device_name);
		return all_spi_device[SPI_LORA].device_handle;
	}
	return NULL;
}
static int command_operate_spi_init_common(uint8_t *data_return,uint8_t spi_no,uint32_t clock_speed_hz,uint32_t queue_size,uint8_t command_bits,uint8_t address_bits,uint8_t dummy_bits,uint8_t mode,uint8_t cs_ena_posttrans,uint16_t duty_cycle_pos)
{
	ESP_LOGI(TAG, "command_operate_spi_init_common spi_no %u command_bits %u address_bits %u dummy_bits %u clock_speed_hz %lu duty_cycle_pos %u mode %u cs_ena_posttrans %u queue_size %lu",spi_no,command_bits,address_bits,dummy_bits,clock_speed_hz,duty_cycle_pos,mode,cs_ena_posttrans,queue_size);
	if(spi_no>=SPI_ALL)
	{
		sprintf((char *)data_return,"spi %u wrong",spi_no);
		return -1;
	}
	if((spi_no==SPI_LORA)||(spi_no==SPI_W5500))
	{
		sprintf((char *)data_return,"spi %u not allowed",spi_no);
		return -2;
	}
	if((spi_no==SPI_EXT1)||(spi_no==SPI_EXT2))
	{
		spi_device_interface_config_t devcfg = { .command_bits = command_bits,
			.address_bits = address_bits, .dummy_bits = dummy_bits, .clock_speed_hz = clock_speed_hz,
			.duty_cycle_pos = duty_cycle_pos,        //50% duty cycle
			.mode = mode, .spics_io_num = -1, .cs_ena_posttrans = cs_ena_posttrans, //Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
			.queue_size = queue_size};
		if(spi_no==SPI_EXT1){
			devcfg.pre_cb=spi_cs_ext1_pre_cb;
			devcfg.post_cb=spi_cs_ext1_post_cb;
			spiutil_init(spi_no, "spi ext1",&devcfg);
		}else if(spi_no==SPI_EXT2){
			devcfg.pre_cb=spi_cs_ext2_pre_cb;
			devcfg.post_cb=spi_cs_ext2_post_cb;
			spiutil_init(spi_no, "spi ext2",&devcfg);
		}
	}else {
		spiutil_init(spi_no, "not set",NULL);
	}
	sprintf((char *)data_return,"spi %u inited success",spi_no);
	return 0;
}
int command_operate_spi_init(uint8_t *command_data,uint32_t command_data_len,uint8_t *data_return,int *data_return_len)
{
	uint32_t idex=0,clock_speed_hz=0,queue_size=0;
	uint8_t spi_no=0,command_bits=0,address_bits=0,dummy_bits=0,mode=0,cs_ena_posttrans=0;
	uint16_t duty_cycle_pos=0;
	uint8_t *temp_data=command_data;
	int ret=0;
	
	COPY_TO_VARIABLE(spi_no,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(command_bits,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(address_bits,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(dummy_bits,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(clock_speed_hz,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(duty_cycle_pos,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(mode,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(cs_ena_posttrans,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(queue_size,temp_data,idex,command_data_len)

    ret=command_operate_spi_init_common(data_return+1,spi_no,clock_speed_hz,queue_size,command_bits,address_bits,dummy_bits,mode,cs_ena_posttrans,duty_cycle_pos);
	data_return[0]=0;//msg
	*data_return_len=strlen((char *)data_return+1)+1;
	return ret;
}
int command_operate_spi_inout(uint8_t *command_data,uint32_t command_data_len,uint8_t *data_return,int *data_return_len)
{
	uint32_t idex=0;
	uint8_t spi_no=0;
	uint64_t spi_address=0;
	uint16_t spi_command=0;
	uint32_t data_send_len=0,data_recv_len=0;
	uint8_t *data_send=NULL;
	uint8_t spi_address_valid=0,spi_command_valid=0;
    uint32_t clock_speed_hz=0,queue_size=0;
	uint8_t command_bits=0,address_bits=0,dummy_bits=0,mode=0,cs_ena_posttrans=0;
	uint16_t duty_cycle_pos=0;
	int err_ret=0;
	esp_err_t ret=ESP_FAIL;
	
	uint8_t *temp_data=command_data;
	COPY_TO_VARIABLE(spi_no,temp_data,idex,command_data_len)
	if(spi_no>=SPI_ALL)
	{
		sprintf((char *)data_return+1,"spi %u not allowed",spi_no);
		err_ret=-1;
		goto ERROR_PROCESS;
	}
	COPY_TO_VARIABLE(spi_address,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(spi_address_valid,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(spi_command,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(spi_command_valid,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(data_send_len,temp_data,idex,command_data_len)
	if(data_send_len>0)
		data_send=temp_data+idex;
	idex+=data_send_len;
	COPY_TO_VARIABLE(data_recv_len,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(command_bits,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(address_bits,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(dummy_bits,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(clock_speed_hz,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(duty_cycle_pos,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(mode,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(cs_ena_posttrans,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(queue_size,temp_data,idex,command_data_len)
	
	if(all_spi_device[spi_no].inited==0)
	{
		if(command_operate_spi_init_common(data_return+1,spi_no,clock_speed_hz,queue_size,command_bits,address_bits,dummy_bits,mode,cs_ena_posttrans,duty_cycle_pos)!=0)
		{
			err_ret=-2;
			goto ERROR_PROCESS;
		}
	}
	if(all_spi_device[spi_no].inited==1)
	{
		ESP_LOGI(TAG, "command_operate_spi_inout spi_no %u address %llu address_valid %u command %u command_valid %u data_send_len %lu data_recv_len %lu",spi_no,spi_address,spi_address_valid,spi_command,spi_command_valid,data_send_len,data_recv_len);
		ret=spiutil_inout(spi_no,spi_address,spi_address_valid,spi_command,spi_command_valid,data_send,data_send_len,data_return+1,data_recv_len);
		*data_return_len=(data_recv_len+1);
		if(*data_return_len<0)
			*data_return_len=0;
		if(ret!=ESP_OK)
		{
			err_ret=-3;
			goto ERROR_PROCESS;
		}
		data_return[0]=1;
		return 0;
	}
	sprintf((char *)data_return+1,"spi %u unknown err",spi_no);
	err_ret=-4;
ERROR_PROCESS:
	data_return[0]=0;
	*data_return_len=strlen((char *)data_return+1)+1;
	return err_ret;
}
uint8_t spiutil_inout_byte(spiutil_device_t device,uint8_t data)
{    
    esp_err_t ret;
    uint32_t return_data=0;
    spi_transaction_t t = {
        .length = 8,              
        .rxlength = 8,          
        .tx_buffer = &data,
        .rx_buffer = &return_data,
        .user = (void*)0,
    };
    all_spi_device[device].tx_times++;
    all_spi_device[device].rx_times++;
 	ret=spi_device_transmit_safe(&all_spi_device[device], &t);
 	if(ret != ESP_OK)        
 		util_reboot(5);
 	//printf(" xx %02x->%08lx xx ",data,return_data);
 	return return_data&0xff;
}
esp_err_t spiutil_tm7705_write(spiutil_device_t device, uint8_t address,uint8_t data)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length=1*8;
    t.addr=address;
    t.tx_buffer=&data;
    t.rx_buffer=NULL;
 	all_spi_device[device].tx_times++;
    return spi_device_transmit_safe(&all_spi_device[device], &t);
}
esp_err_t spiutil_tm7705_read(spiutil_device_t device, uint8_t address,uint8_t *data,uint8_t datalen)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length=datalen*8;
    t.addr=address,
    t.rx_buffer=data;
    t.tx_buffer=NULL;
    all_spi_device[device].rx_times++;
    return spi_device_transmit_safe(&all_spi_device[device], &t);
}
esp_err_t spiutil_inout(spiutil_device_t device,uint64_t address,uint8_t address_valid,uint16_t command,uint8_t command_valid,uint8_t *data_send,uint32_t data_send_len,uint8_t *data_recv,uint32_t data_recv_len)
{    
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));   
    cloud_printf("spi inout ");
    if(address_valid>0)
    {
    	t.addr=address;
    	cloud_printf("addr %02llx ",address);
    }
    if(command_valid>0)
    {
		t.cmd=command;
		cloud_printf("command %02x ",command);
    }
    if(address_valid|command_valid|data_send_len){
		all_spi_device[device].tx_times++;
	}
	if(data_send_len>0)
	{
		t.length = 8*data_send_len;
    	t.tx_buffer = data_send;     
    	cloud_printf_format("tx data "," ", "%02x",data_send,data_send_len);
    }  
	if(data_recv_len>0){
		all_spi_device[device].rx_times++;
		t.rx_buffer = data_recv;   
		t.rxlength=8*data_recv_len;
	}    
 	ret=spi_device_transmit_safe(&all_spi_device[device], &t);
 	if(data_recv_len>0){
		 cloud_printf_format("rx data "," ", "%02x",data_recv,data_recv_len);
	}
 	cloud_printf("\r\n");
 	return ret;
}
esp_err_t spiutil_w5500_read(uint32_t address,uint8_t *data,int16_t len)
{
	address=(address) << 16;
	esp_err_t ret = ESP_OK;
	spi_transaction_t trans = { .flags = len <= 4 ? SPI_TRANS_USE_RXDATA : 0, // use direct reads for registers to prevent overwrites by 4-byte boundary writes
			.cmd = (address >> 16), .addr = (address & 0xFFFF), .length = 8
					* len, .rx_buffer = data };
	ch423_output(CH423_OC_IO4, 0);
	if (spi_device_transmit_safe(&all_spi_device[SPI_W5500], &trans) != ESP_OK) {
		ESP_LOGE(TAG, "spiutil_w5500_read failed");
		ret = ESP_FAIL;
	} else {
		ret = ESP_ERR_TIMEOUT;
	}
	if ((trans.flags & SPI_TRANS_USE_RXDATA) && len <= 4) {
		memcpy(data, trans.rx_data, len);  // copy register values to output
	}
	ch423_output(CH423_OC_IO4, 1);
	return ret;
}

