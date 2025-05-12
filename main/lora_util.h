#ifndef __LLCC68_UTIL_H__
#define __LLCC68_UTIL_H__
//--------------------------------------------- 测试默认配置 ---------------------------------------------

#define LORA_FRE									433000000	// 收发频率
#define LORA_TX_OUTPUT_POWER                        3        // 用了带PA的，不能超过3否则会烧毁PA，测试默认使用的发射功率，126x发射功率0~22dbm，127x发射功率2~20dbm
#define LORA_BANDWIDTH                              1         // [0: 125 kHz,	测试默认使用的带宽，LLCC68：[0: 125 kHz,1: 250 kHz,2: 500 kHz,3: Reserved]
#define LORA_SPREADING_FACTOR                       10         // 测试默认使用的扩频因子范围7~12
#define LORA_CODINGRATE                             1         // 测试默认使用的纠错编码率[1: 4/5,2: 4/6,3: 4/7,4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // 前导码长度
#define LORA_LLCC68_SYMBOL_TIMEOUT                  0         // Symbols(LLCC68用到的是0,127x用到的是5)
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false			// 是否为固定长度包(暂时只是LLCC68用到了)
#define LORA_IQ_INVERSION_ON                        false			// 这个应该是设置是否翻转中断电平的(暂时只是LLCC68用到了)
#define LORA_RX_TIMEOUT_VALUE                       5000
#define LORA_TX_TIMEOUT_VALUE                       5000


void lora_util_tx(uint8_t *data,uint32_t data_len,uint8_t *data_return,int *data_return_len);
int lora_util_upload(void *parameter);
void lora_util_property(uint8_t *data_return,int *data_return_len);
#endif //end of __LLCC68_UTIL_H__
