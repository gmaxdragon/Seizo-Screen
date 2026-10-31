/*****************************************************************************
* | File      	:		xpt2046.h
* | Author      :   Waveshare team
* | Function    :   Touch API
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2019-05-05
* | Info        :
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/

#ifndef __XPT2046_H
#define __XPT2046_H

#include "stm32f4xx.h"
#include "LCD28/LCD_Driver.h"

#define XPT2046_CS_PIN          GPIO_Pin_9
#define XPT2046_IRQ_PIN         GPIO_Pin_4

#define XPT2046_CS_GPIO         GPIOB
#define XPT2046_IRQ_GPIO        GPIOB

#define XPT2046_CS_H()      GPIO_WriteBit(XPT2046_CS_GPIO, XPT2046_CS_PIN, Bit_SET)
#define XPT2046_CS_L()      GPIO_WriteBit(XPT2046_CS_GPIO, XPT2046_CS_PIN, Bit_RESET)

#define XPT2046_IRQ_READ()    GPIO_ReadInputDataBit(XPT2046_IRQ_GPIO, XPT2046_IRQ_PIN)

#define XPT2046_WRITE_BYTE(__DATA)       spi1_communication(__DATA)


extern void xpt2046_init(void);
extern void xpt2046_read_xy(uint16_t *phwXpos, uint16_t *phwYpos);
extern uint8_t xpt2046_twice_read_xy(uint16_t *phwXpos, uint16_t *phwYpos);

#endif



