#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <mbedtls/base64.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "camera_util.h"
#include <device_config.h>
#include <network_util.h>
#include "util.h"
#include "system_util.h"

#define CAMERA_PIN_PWDN -1
#define CAMERA_PIN_RESET -1
#define CAMERA_PIN_XCLK  16
#define CAMERA_PIN_PCLK  8
#define CAMERA_PIN_VSYNC 6
#define CAMERA_PIN_HREF 7           /*!< hardware pins: HREF */
#define CAMERA_PIN_D0    3 /*!< hardware pins: D2 */
#define CAMERA_PIN_D1    10  /*!< hardware pins: D3 */
#define CAMERA_PIN_D2    11  /*!< hardware pins: D4 */
#define CAMERA_PIN_D3    9 /*!< hardware pins: D5 */
#define CAMERA_PIN_D4    46 /*!< hardware pins: D6 */
#define CAMERA_PIN_D5    18 /*!< hardware pins: D7 */
#define CAMERA_PIN_D6    17 /*!< hardware pins: D8 */
#define CAMERA_PIN_D7    15 /*!< hardware pins: D9 */
#define CAMERA_PIN_SIOC   5
#define CAMERA_PIN_SIOD   4

#define XCLK_FREQ_HZ (10*1000000)
#define PIXEL_FORMAT (PIXFORMAT_JPEG)
#define CAMERA_TAKE_RETRY_TIMES   3
#define CAMERA_INIT_SKIP_TIMES   2

static const char *TAG = "camera_util";
static camera_fb_t *pic=NULL;
static uint8_t inited=0;
static uint32_t times_inited=0,times_take=0,times_failed=0;

static void camera_fb_return(camera_fb_t *fb)
{
	if(fb==NULL)
		return;
    esp_camera_fb_return(fb);
}
static esp_err_t camera_init(uint32_t xclk_freq_hz, pixformat_t pixel_format, framesize_t frame_size, uint8_t fb_count,int aecgain,uint8_t quality)
{
    camera_config_t camera_config = {
        .pin_pwdn = CAMERA_PIN_PWDN,
        .pin_reset = CAMERA_PIN_RESET,
        .pin_xclk = CAMERA_PIN_XCLK,
        .pin_sccb_sda = CAMERA_PIN_SIOD,
        .pin_sccb_scl = CAMERA_PIN_SIOC,

        .pin_d7 = CAMERA_PIN_D7,
        .pin_d6 = CAMERA_PIN_D6,
        .pin_d5 = CAMERA_PIN_D5,
        .pin_d4 = CAMERA_PIN_D4,
        .pin_d3 = CAMERA_PIN_D3,
        .pin_d2 = CAMERA_PIN_D2,
        .pin_d1 = CAMERA_PIN_D1,
        .pin_d0 = CAMERA_PIN_D0,
        .pin_vsync = CAMERA_PIN_VSYNC,
        .pin_href = CAMERA_PIN_HREF,
        .pin_pclk = CAMERA_PIN_PCLK,

        // EXPERIMENTAL: Set to 16MHz on ESP32-S2 or ESP32-S3 to enable EDMA mode
        .xclk_freq_hz = xclk_freq_hz,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = pixel_format, // YUV422,GRAYSCALE,RGB565,JPEG
        .frame_size = frame_size,    // QQVGA-UXGA, sizes above QVGA when not JPEG format are not recommended.

        .jpeg_quality = quality, // 0-63 lower number means higher quality
        .fb_count = fb_count,       // if more than one, i2s runs in continuous mode.
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
        .fb_location = CAMERA_FB_IN_PSRAM
    };

    //initialize the camera
    esp_err_t ret = esp_camera_init(&camera_config);
	if(ret!=ESP_OK)
		return ESP_FAIL;
    sensor_t *s = esp_camera_sensor_get();
    if(s==NULL){
		esp_camera_deinit();
    	return ESP_FAIL;
    }
    s->set_vflip(s, 1);//flip it back

    return ret;
}
static camera_fb_t *camera_take_excute(framesize_t size,uint8_t quality,int aecgain)
{
	
	uint8_t validpictimes=0;
	if(inited==0)
	{
		power_onoff_camera(1);//power on camera
		times_inited++;
		if(camera_init(XCLK_FREQ_HZ, PIXEL_FORMAT, size, 1,aecgain,quality)==ESP_FAIL)
		{
			ESP_LOGE(TAG, "camera init failed, and will power off");
			power_onoff_camera(0);//power off camera
			return NULL;
		}
	    for(uint8_t i=0;i<CAMERA_INIT_SKIP_TIMES;i++)
		{
			pic = esp_camera_fb_get();
			times_take++;
			if(pic)
				validpictimes++;
			else
 				times_failed++;
			camera_fb_return(pic);
		}
	    if(validpictimes==CAMERA_INIT_SKIP_TIMES)
	    {
	    	//ESP_LOGI(TAG, "camera init success");
	    	inited=1;
	    }else{
	    	ESP_LOGI(TAG, "camera int fail");
	    	goto ERROR_PROCESS;
	    }
	}

	times_take++;
	pic = esp_camera_fb_get();
	if(pic!=NULL)
	{
		//ESP_LOGI(TAG, "camera take pic size %d",pic->len);
		return pic;
	}else{
		ESP_LOGI(TAG, "camera take pic fail");
		times_failed++;
	}

ERROR_PROCESS:	
	esp_camera_deinit();
	power_onoff_camera(0);
	inited=0;
    return NULL;
}
void camera_off(void)
{
	if(inited)
	{
		camera_fb_return(pic);
		esp_camera_return_all();
		esp_camera_deinit();
		power_onoff_camera(0);
		inited=0;
	}
}
//quality:0-63 lower number means higher quality, usually to use 30
//aecgain:-1 means auto, for other value, need to check the datasheet of camera
camera_fb_t *camera_take(framesize_t size,uint8_t quality,int aecgain)
{
	camera_fb_return(pic);
	for(uint8_t i=0;i<CAMERA_TAKE_RETRY_TIMES;i++)
	{
		pic=camera_take_excute(size,quality,aecgain);
		if(pic!=NULL)
			return pic;
		util_delay_ms(3000);
	}
	return NULL;
}
camera_fb_t *camera_exe(void)
{
	static uint8_t temp_onoff_bak=0;
	static uint8_t camera_size=0,camera_quality=0;
	static uint16_t camera_aec_gain=0;
	uint8_t temp_onoff;
	temp_onoff=device_config_get_number(CONFIG_CAMERA_ONOFF);
    if(temp_onoff==0)
    {
		if(temp_onoff_bak)//turn off
		{
			temp_onoff_bak=0;
			camera_off();
			ESP_LOGI(TAG, "camera turned off");
		}
		return 0;
	}  
	if((camera_size!=device_config_get_number(CONFIG_CAMERA_SIZE))||(camera_quality!=device_config_get_number(CONFIG_CAMERA_QUALITY))||(camera_aec_gain!=device_config_get_number(CONFIG_CAMERA_AEC_GAIN)))
	{
		camera_size=device_config_get_number(CONFIG_CAMERA_SIZE);
        camera_quality=device_config_get_number(CONFIG_CAMERA_QUALITY);
        camera_aec_gain=device_config_get_number(CONFIG_CAMERA_AEC_GAIN);
		//config changed, so need to turn off and then turn on
		if(temp_onoff_bak)
		{
			camera_off();
			ESP_LOGI(TAG, "camera config changed");
			util_delay_ms(5*1000);
		}
	}
	camera_fb_t *pic=camera_take(camera_size,camera_quality,camera_aec_gain);
	if((pic!=NULL)&&(pic->len>0))
	{	
		temp_onoff_bak=temp_onoff;		
		return pic;	
	}
	return NULL;
}
int camera_upload(void *pvParameters)
{	
	camera_fb_t *pic=camera_exe();
	if((pic!=NULL)&&(pic->len>0))
	{				
		return tcp_send_command(
	      "camera",
	      data_container_create(0,COMMAND_REQ_CAMERA,pic->buf, pic->len,NULL),
	      NULL, 0, COMMAND_FLAG_FROM_DEVICE | COMMAND_FLAG_INIT_FROM_DEVICE,
	      SOCKET_HANDLER_ONE_TIME, 0,NULL);
	}
	return -1;
}
int camera_request(data_send_element *data_element)
{	
	camera_fb_t *pic=camera_exe();
	if((pic!=NULL)&&(pic->len>0))
	{
		data_element->data=pic->buf;
		data_element->data_len=pic->len;
		data_element->need_free=0;
		return 0;
	}	
	return -1;
}
void camera_property(uint8_t *data_out,int *data_len_out)
{
    uint32_t idex=0;
	COPY_TO_BUF_WITH_NAME(times_inited,data_out,idex)
	COPY_TO_BUF_WITH_NAME(times_take,data_out,idex)
	COPY_TO_BUF_WITH_NAME(times_failed,data_out,idex)
	*data_len_out=idex;
}
