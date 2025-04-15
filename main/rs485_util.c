#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_log.h"
#include "driver/uart.h"
#include <device_config.h>
#include <util.h>
#include <network_util.h>
#include <cloud_log.h>
#include "system_util.h"
static const char *TAG = "rs485_util";

static QueueHandle_t uart1_queue=NULL;
static uint8_t init=0;
static uint32_t rx_times=0,rx_length=0,rx_times_failed=0,rx_upload_failed_times=0,rx_fifo_over_times=0,rx_buf_full_times=0,rx_break_times=0,rx_parity_err_times=0,rx_frame_err_times=0,tx_length=0,tx_times=0,tx_times_failed=0;

static void rs485_init(uint8_t tx_pin_in,uint8_t rx_pin_in,uint32_t baud_rate_in)
{
	static uint8_t tx_pin=0,rx_pin=0,driver_install=0;
	static uint32_t baud_rate=0;
	if(power_status_spi()==0){
		power_onoff_spi(1);
	}
	if((init==0)||(tx_pin!=tx_pin_in)||(rx_pin!=rx_pin_in)||(baud_rate!=baud_rate_in))
	{		
		ESP_LOGI(TAG, "rs485_init tx %u rx %u rate %lu",tx_pin_in,rx_pin_in,baud_rate_in);
		if((init==1)&&((tx_pin!=tx_pin_in)||(rx_pin!=rx_pin_in)||(baud_rate!=baud_rate_in)))
		{
			device_config_set_number(CONFIG_RS485_TX_PIN,tx_pin_in);
			device_config_set_number(CONFIG_RS485_RX_PIN,rx_pin_in);
			device_config_set_number(CONFIG_RS485_BAUD_RATE,baud_rate_in);
			device_config_set_number(CONFIG_RS485_ONOFF,1);						
			config_write();
			tcp_send_config("rs485_config_changed",NULL,0);
		}
		if(driver_install==0)
		{
			ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, 256, 256, 10, &uart1_queue, 0));	
			driver_install=1;
		}
	    uart_config_t uart_config = {
	        .baud_rate = baud_rate_in,
	        .data_bits = UART_DATA_8_BITS,
	        .parity    = UART_PARITY_DISABLE,
	        .stop_bits = UART_STOP_BITS_1,
	        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
	        .rx_flow_ctrl_thresh = 122,
	        .source_clk = UART_SCLK_DEFAULT,
	    };
	    ch423_output(CH423_IO1, 0);
	    ESP_ERROR_CHECK(uart_param_config(UART_NUM_2, &uart_config));
	    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, tx_pin_in, rx_pin_in, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    	ESP_ERROR_CHECK(uart_set_mode(UART_NUM_2, UART_MODE_RS485_HALF_DUPLEX));
    	ESP_ERROR_CHECK(uart_set_rx_timeout(UART_NUM_2, 3));
   
		init=1;		
		tx_pin=tx_pin_in;
		rx_pin=rx_pin_in;
		baud_rate=baud_rate_in;
	}
}

//esp_err_t uart_driver_delete(uart_port_t uart_num);


int rs485_util_upload(void *parameter)
{
	rs485_init(device_config_get_number(CONFIG_RS485_TX_PIN),device_config_get_number(CONFIG_RS485_RX_PIN),device_config_get_number(CONFIG_RS485_BAUD_RATE));
	if(init)
	{
		uint8_t *temp_buf;
		int temp_buf_idx=0,ret=-1;
		temp_buf=user_malloc(1024);
		temp_buf[0]=0;//meta
		temp_buf_idx++;
		COPY_TO_BUF_WITH_NAME(rx_times,temp_buf,temp_buf_idx);
		COPY_TO_BUF_WITH_NAME(rx_length,temp_buf,temp_buf_idx);
		COPY_TO_BUF_WITH_NAME(rx_times_failed,temp_buf,temp_buf_idx);
		COPY_TO_BUF_WITH_NAME(rx_upload_failed_times,temp_buf,temp_buf_idx);
		COPY_TO_BUF_WITH_NAME(rx_fifo_over_times,temp_buf,temp_buf_idx);
		COPY_TO_BUF_WITH_NAME(rx_buf_full_times,temp_buf,temp_buf_idx);
		COPY_TO_BUF_WITH_NAME(rx_break_times,temp_buf,temp_buf_idx);
		COPY_TO_BUF_WITH_NAME(rx_parity_err_times,temp_buf,temp_buf_idx);
		COPY_TO_BUF_WITH_NAME(rx_frame_err_times,temp_buf,temp_buf_idx);
		COPY_TO_BUF_WITH_NAME(tx_length,temp_buf,temp_buf_idx);
		COPY_TO_BUF_WITH_NAME(tx_times,temp_buf,temp_buf_idx);
		COPY_TO_BUF_WITH_NAME(tx_times_failed,temp_buf,temp_buf_idx);
		ret=tcp_send_command(
		      "rs485_meta",
		      data_container_create(1,COMMAND_REQ_RS485,temp_buf, temp_buf_idx),
		      NULL, 0, COMMAND_FLAG_FROM_DEVICE | COMMAND_FLAG_INIT_FROM_DEVICE,
		      SOCKET_HANDLER_ONE_TIME, 0,NULL);
	      
		return ret;
	}
	return 0;
}
void rs485_util_queue(void)
{
	if(init==0)
		return;
    uart_event_t event;
    if (xQueueReceive(uart1_queue, (void *)&event, 0)) {
        switch (event.type) {
        case UART_DATA:
            //ESP_LOGE(TAG, "uart_util_queue %d", event.size);
            rx_times++;
            uint8_t *rx_buf=user_malloc(event.size+2);
            int rx_len=uart_read_bytes(UART_NUM_2, rx_buf+1, event.size, 0);
            if(rx_len<=0){
				user_free(rx_buf);
            	rx_times_failed++;
            }else
			{
				rx_length+=rx_len;
				rx_buf[0]=1;//dtu uart data
				if(tcp_send_command(
			      "rs485_rx",
			      data_container_create(1,COMMAND_REQ_RS485,rx_buf, rx_len+1),
			      NULL, 0, COMMAND_FLAG_FROM_DEVICE | COMMAND_FLAG_INIT_FROM_DEVICE,
			      SOCKET_HANDLER_ONE_TIME, 0,NULL)<0)
					rx_upload_failed_times++;
			}
            break;
        //Event of HW FIFO overflow detected
        case UART_FIFO_OVF:
            ESP_LOGW(TAG, "rs485 hw fifo overflow");
            rx_fifo_over_times++;
            uart_flush_input(UART_NUM_2);
            xQueueReset(uart1_queue);
            break;
        //Event of UART ring buffer full
        case UART_BUFFER_FULL:
        	rx_buf_full_times++;
            ESP_LOGW(TAG, "rs485 ring buffer full");
            uart_flush_input(UART_NUM_2);
            xQueueReset(uart1_queue);
            break;
        //Event of UART RX break detected
        case UART_BREAK:
        	rx_break_times++;
            ESP_LOGW(TAG, "rs485 rx break");
            break;
        //Event of UART parity check error
        case UART_PARITY_ERR:
        	rx_parity_err_times++;
            ESP_LOGW(TAG, "rs485 parity error");
            break;
        //Event of UART frame error
        case UART_FRAME_ERR:
        	rx_frame_err_times++;
            ESP_LOGW(TAG, "rs485 frame error");
            break;
        //Others
        default:
            ESP_LOGI(TAG, "rs485 event type: %d", event.type);
            break;
        }
    }
}


void rs485_util_tx(uint8_t *command_data,uint32_t command_data_len)
{	
	uint32_t idex=0;
	uint32_t tx_data_len=0,baud_rate=0;
	uint8_t *tx_data=NULL;
	uint8_t *temp_data=command_data;
	uint8_t tx_pin=0,rx_pin=0;
	COPY_TO_VARIABLE(tx_data_len,temp_data,idex,command_data_len)
	if(tx_data_len>0)
		tx_data=temp_data+idex;
	idex+=tx_data_len;
	COPY_TO_VARIABLE(tx_pin,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(rx_pin,temp_data,idex,command_data_len)
	COPY_TO_VARIABLE(baud_rate,temp_data,idex,command_data_len)
	if(baud_rate>0)
		rs485_init(tx_pin,rx_pin,baud_rate);
	else if(init==0)//use previous setting if it has been inited but no init data for this time
		rs485_init(device_config_get_number(CONFIG_RS485_TX_PIN),device_config_get_number(CONFIG_RS485_RX_PIN),device_config_get_number(CONFIG_RS485_BAUD_RATE));	
	if((init==0)||(tx_data_len==0))
		return;
	tx_times++;
	ch423_output(CH423_IO1, 1);
	ESP_LOGI(TAG, "rs485_util_tx len %lu", tx_data_len);
	cloud_printf_format(NULL, NULL, "%02x",tx_data,tx_data_len);
	util_delay_ms(10);
	int tx_len=uart_write_bytes(UART_NUM_2, tx_data, tx_data_len);
	if(tx_len<=0)
		tx_times_failed++;
	else
		tx_length+=tx_len;
	util_delay_ms(10);
	ch423_output(CH423_IO1, 0);
}

