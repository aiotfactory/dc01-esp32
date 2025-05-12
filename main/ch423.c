#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/i2c_master.h"
#include "util.h"
#include "ch423.h"
#include "esp_rom_sys.h"

/*  设置系统参数命令 */

#define CH423_SYS_CMD     0x4800     // 设置系统参数命令，默认方式
#define BIT_X_INT         0x08       // 使能输入电平变化中断，为0禁止输入电平变化中断；为1并且DEC_H为0允许输出电平变化中断
#define BIT_DEC_H         0x04       // 控制开漏输出引脚高8位的片选译码
#define BIT_DEC_L         0x02       // 控制开漏输出引脚低8位的片选译码
#define BIT_IO_OE         0x01       // 控制双向输入输出引脚的三态输出，为1允许输出

/*  设置低8位开漏输出命令 */

#define CH423_OC_L_CMD    0x4400     // 设置低8位开漏输出命令，默认方式
#define BIT_OC0_L_DAT     0x01       // OC0为0则使引脚输出低电平，为1则引脚不输出
#define BIT_OC1_L_DAT     0x02       // OC1为0则使引脚输出低电平，为1则引脚不输出
#define BIT_OC2_L_DAT     0x04       // OC2为0则使引脚输出低电平，为1则引脚不输出
#define BIT_OC3_L_DAT     0x08       // OC3为0则使引脚输出低电平，为1则引脚不输出
#define BIT_OC4_L_DAT     0x10       // OC4为0则使引脚输出低电平，为1则引脚不输出
#define BIT_OC5_L_DAT     0x20       // OC5为0则使引脚输出低电平，为1则引脚不输出
#define BIT_OC6_L_DAT     0x40       // OC6为0则使引脚输出低电平，为1则引脚不输出
#define BIT_OC7_L_DAT     0x80       // OC7为0则使引脚输出低电平，为1则引脚不输出

/*  设置高8位开漏输出命令 */

#define CH423_OC_H_CMD    0x4600      // 设置低8位开漏输出命令，默认方式
#define BIT_OC8_L_DAT     0x01        // OC8为0则使引脚输出低电平，为1则引脚不输出
#define BIT_OC9_L_DAT     0x02        // OC9为0则使引脚输出低电平，为1则引脚不输出
#define BIT_OC10_L_DAT    0x04        // OC10为0则使引脚输出低电平，为1则引脚不输出
#define BIT_OC11_L_DAT    0x08        // OC11为0则使引脚输出低电平，为1则引脚不输出
#define BIT_OC12_L_DAT    0x10        // OC12为0则使引脚输出低电平，为1则引脚不输出
#define BIT_OC13_L_DAT    0x20        // OC13为0则使引脚输出低电平，为1则引脚不输出
#define BIT_OC14_L_DAT    0x40        // OC14为0则使引脚输出低电平，为1则引脚不输出
#define BIT_OC15_L_DAT    0x80        // OC15为0则使引脚输出低电平，为1则引脚不输出

/* 设置双向输入输出命令 */

#define CH423_SET_IO_CMD   0x6000    // 设置双向输入输出命令，默认方式
#define BIT_IO0_DAT        0x01      // 写入双向输入输出引脚的输出寄存器，当IO_OE=1,IO0为0输出低电平，为1输出高电平
#define BIT_IO1_DAT        0x02      // 写入双向输入输出引脚的输出寄存器，当IO_OE=1,IO1为0输出低电平，为1输出高电平
#define BIT_IO2_DAT        0x04      // 写入双向输入输出引脚的输出寄存器，当IO_OE=1,IO2为0输出低电平，为1输出高电平
#define BIT_IO3_DAT        0x08      // 写入双向输入输出引脚的输出寄存器，当IO_OE=1,IO3为0输出低电平，为1输出高电平
#define BIT_IO4_DAT        0x10      // 写入双向输入输出引脚的输出寄存器，当IO_OE=1,IO4为0输出低电平，为1输出高电平
#define BIT_IO5_DAT        0x20      // 写入双向输入输出引脚的输出寄存器，当IO_OE=1,IO5为0输出低电平，为1输出高电平
#define BIT_IO6_DAT        0x40      // 写入双向输入输出引脚的输出寄存器，当IO_OE=1,IO6为0输出低电平，为1输出高电平
#define BIT_IO7_DAT        0x80      // 写入双向输入输出引脚的输出寄存器，当IO_OE=1,IO7为0输出低电平，为1输出高电平


#define CH423_SCL           13    
#define CH423_SDA           12   
#define CH423_I2C_NUM              0         
#define CH423_I2C_FREQ_HZ          400000  

static const char *TAG = "ch423";
static uint32_t io_status=0;



#define CH423_SDA_SET gpio_set_level(CH423_SDA,1)
#define CH423_SCL_SET gpio_set_level(CH423_SCL,1)
#define CH423_SDA_CLR gpio_set_level(CH423_SDA,0)
#define CH423_SCL_CLR gpio_set_level(CH423_SCL,0)
#define DELAY_0_1US esp_rom_delay_us(1)


static void CH423_I2c_Start( void )    // 操作起始
{
    CH423_SDA_SET;    // 发送起始条件的数据信号
    //CH423_SDA_D_OUT;    // 设置SDA为输出方向
    CH423_SCL_SET;
    //CH423_SCL_D_OUT;    // 设置SCL为输出方向
    DELAY_0_1US;
    CH423_SDA_CLR;    //发送起始信号
    DELAY_0_1US;      
    CH423_SCL_CLR;    //钳住I2C总线，准备发送或接收数据
}

static void CH423_I2c_Stop( void )    // 操作结束
{
    CH423_SDA_CLR;
    DELAY_0_1US;
    CH423_SCL_SET;
    DELAY_0_1US;
    CH423_SDA_SET;    // 发送I2C总线结束信号
    DELAY_0_1US;
}
static void CH423_I2c_WrByte( unsigned char dat )    // 写一个字节数据
{
    unsigned char i;
    for( i = 0; i != 8; i++ )    // 输出8位数据
    {
        if( dat&0x80 ) { CH423_SDA_SET; }
        else { CH423_SDA_CLR; }
        DELAY_0_1US;
        CH423_SCL_SET;
        dat<<=1;
        DELAY_0_1US;    // 可选延时
        CH423_SCL_CLR;
    }
    CH423_SDA_SET;
    DELAY_0_1US;
    CH423_SCL_SET;    // 接收应答
    DELAY_0_1US;
    CH423_SCL_CLR;
}
static esp_err_t CH423_WriteByte( unsigned short cmd )    // 写出数据
{
    CH423_I2c_Start();    // 启动总线
    CH423_I2c_WrByte( ( unsigned char )( cmd>>8 ) );
    CH423_I2c_WrByte( ( unsigned char ) cmd );    // 发送数据
    CH423_I2c_Stop();    // 结束总线
    return ESP_OK;
}
esp_err_t ch423_init(void)
{
	util_gpio_init(CH423_SDA,GPIO_MODE_OUTPUT,0,1,1);
	util_gpio_init(CH423_SCL,GPIO_MODE_OUTPUT,0,1,1);
	ESP_ERROR_CHECK(CH423_WriteByte(CH423_SYS_CMD | BIT_IO_OE ));//all io set to output
	CH423_WriteByte(CH423_SET_IO_CMD | 0x00);
	CH423_WriteByte(CH423_OC_L_CMD | 0x00);
	CH423_WriteByte(CH423_OC_H_CMD | 0x00);
    ESP_LOGI(TAG,"ch423_init success");
    return ESP_OK;
}
uint8_t ch423_get_status(uint8_t ionum)
{
	return (io_status>>ionum)&0x01;
}
esp_err_t ch423_output(uint8_t ionum, uint8_t status)
{
	if(status)
		io_status|=(1<<ionum);
	else
		io_status&=~(1<<ionum);
    //ESP_LOGI(TAG, "ch423_output %u,%u %08lx",ionum,status,io_status);
	if(ionum<8)
		return CH423_WriteByte(CH423_SET_IO_CMD | (uint8_t) (io_status));
	else if(ionum<16)
		return CH423_WriteByte(CH423_OC_L_CMD | (uint8_t) (io_status>>8));
	else
		return CH423_WriteByte(CH423_OC_H_CMD | (uint8_t) (io_status>>16));
	return ESP_FAIL;
}
