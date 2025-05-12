/* Ethernet Basic Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

#include "esp_err.h"


#ifdef __cplusplus
extern "C"
{
#endif

uint8_t ethernet_check(void);
void ethernet_start(void);

#ifdef __cplusplus
}
#endif
