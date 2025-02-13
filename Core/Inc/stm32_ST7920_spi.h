/*
* Bare minimum display driver for GEVA project, no fancy display scrolling etc.., just setup and Display RAM update
*
* Kiriaus 6/12/24
* v1.0
*/

#ifndef _ST7920_STM32_
#define _ST7920_STM32_

#include <stdint.h>
#include "main.h"
#include "GEVA.h"


#define BASIC_FUNCTION_SET 0x30
#define DISPLAY_ON_CURSOR_OFF 0x0C
#define CLEAR_DISPLAY 0x01
#define ENTRY_MODE_SET 0x06
#define BYTE_MODE 0x30
#define EXTENDED_INSTRUCTION 0x34
#define ENABLE_GRAPHIC 0x36


void init_display(SPI_HandleTypeDef spi_handler, GPIO_TypeDef *rst_port, uint16_t rst_pin, GPIO_TypeDef *cs_port, uint16_t cs_pin); 
void update_display(video_buffer *buf);


#endif