/**
 * @copyright (C) 2017 Melexis N.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include "MLX90640_I2C_Driver.h"
#include "esp_err.h"
#include "i2cutil.h"
#include "system_util.h"



static const char *TAG = "MLX90640";

int MLX90640_I2CInit(void)
{   
	uint16_t t1=0;
	MLX90640_I2CRead(MLX_ADDR, 0x8010, 1, &t1);
	if(t1==0xBE33){
		//ESP_LOGI(TAG, "MLX90640_I2CInit Ok");
		return 0;
	}
	else{
		ESP_LOGW(TAG, "MLX90640_I2CInit Failed");
		return -1;
	}
}

int MLX90640_I2CGeneralReset(void)
{    
    uint8_t cmd[2] = {0,0};
    
    cmd[0] = 0x00;
    cmd[1] = 0x06;    

    util_delay_ms(1);
    
    if(i2c_write(cmd[0],&cmd[1],1)!=ESP_OK)
        return -1;

    util_delay_ms(1);

    return 0;
}

int MLX90640_I2CRead(uint8_t slaveAddr, uint16_t startAddress, uint16_t nMemAddressRead, uint16_t *data)
{
    int cnt = 0,i=0;
    uint8_t *i2cData=(uint8_t *)data;
    uint8_t startAddressnew[2]={0};
    uint8_t temp=0;
    memcpy(startAddressnew,&startAddress,sizeof(startAddress));
    temp=startAddressnew[0];
    startAddressnew[0]=startAddressnew[1];
    startAddressnew[1]=temp;
    
    if(i2c_read_reg(slaveAddr,startAddressnew,2,i2cData,2*nMemAddressRead)!=ESP_OK)
    	return -1;

    for(cnt=0; cnt < nMemAddressRead; cnt++)
    {
        i = cnt << 1;
        data[cnt] = (uint16_t)i2cData[i]*256 + (uint16_t)i2cData[i+1];
    }
    return 0;   
} 



int MLX90640_I2CWrite(uint8_t slaveAddr, uint16_t writeAddress, uint16_t data)
{
    uint8_t cmd[4] = {0,0,0,0};
    static uint16_t dataCheck;
    cmd[0] = writeAddress >> 8;
    cmd[1] = writeAddress & 0x00FF;
    cmd[2] = data >> 8;
    cmd[3] = data & 0x00FF;

	if(i2c_write(slaveAddr,cmd,4)!=ESP_OK)
        return -1;

    
    MLX90640_I2CRead(slaveAddr,writeAddress,1, &dataCheck);

    if ( dataCheck != data)
        return -2; 
    
    return 0;
}

