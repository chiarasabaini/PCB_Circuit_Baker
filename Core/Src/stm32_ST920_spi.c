
#include "stm32f0xx.h"
#include "stm32_ST7920_spi.h"


SPI_HandleTypeDef _spi_handler;

GPIO_TypeDef *_rst_port;
GPIO_TypeDef *_cs_port;
uint16_t _rst_pin;
uint16_t _cs_pin;


void ST7920_reset(void) {
    HAL_GPIO_WritePin(_rst_port, _rst_pin, GPIO_PIN_RESET);
    HAL_Delay(50); 
    HAL_GPIO_WritePin(_rst_port, _rst_pin, GPIO_PIN_SET);
    HAL_Delay(50);
}


void ST7920_send(uint8_t byte, uint8_t is_data) {
    
    //READ THE DATASHEET >:| 

    uint8_t high_nibble = (byte & 0xF0);
    uint8_t low_nibble = ((byte << 4) & 0xF0);
    uint8_t cmd = 0xf8+(0<<1);
    uint8_t data = 0xf8+(1<<1);
    uint8_t header = (!is_data) ? cmd : data;
    
    HAL_SPI_Transmit(&_spi_handler, &header, 1, HAL_MAX_DELAY);
    HAL_SPI_Transmit(&_spi_handler, &high_nibble, 1, HAL_MAX_DELAY);
    HAL_SPI_Transmit(&_spi_handler, &low_nibble, 1, HAL_MAX_DELAY);

}


/*
void ST7920_send(uint8_t byte, uint8_t is_data) {
    
    //READ THE DATASHEET >:| 

    uint8_t DMA_transfer_buffer[3];
    uint8_t high_nibble = (byte & 0xF0);
    uint8_t low_nibble = ((byte << 4) & 0xF0);
    uint8_t cmd = 0xf8+(0<<1);
    uint8_t data = 0xf8+(1<<1);
    uint8_t header = is_data ? data : cmd;

    DMA_transfer_buffer[1] = high_nibble;
    DMA_transfer_buffer[2] = low_nibble;
    DMA_transfer_buffer[0] = header;

    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);
    HAL_SPI_Transmit_IT(&_spi_handler, DMA_transfer_buffer, 3);
    //HAL_SPI_Transmit_DMA(&_spi_handler, DMA_transfer_buffer, 3);

}
*/

void init_display(SPI_HandleTypeDef spi_handler, GPIO_TypeDef *rst_port, uint16_t rst_pin, GPIO_TypeDef *cs_port, uint16_t cs_pin) {

    _spi_handler = spi_handler;
    _rst_port = rst_port;
    _cs_port = cs_port;
    _rst_pin = rst_pin;
    _cs_pin = cs_pin;

    HAL_GPIO_WritePin(_cs_port, _cs_pin, GPIO_PIN_SET);
    
    ST7920_reset(); 

    ST7920_send(BYTE_MODE, 0);  // 8 bit mode
	HAL_Delay (1);
	ST7920_send(EXTENDED_INSTRUCTION, 0);  // switch to Extended instructions
	HAL_Delay (1);
	ST7920_send(ENABLE_GRAPHIC, 0);  // enable graphics
	HAL_Delay (1);

}

void update_display(video_buffer *buf) {

    //function is really slow > 80ms to update the display but the display doesnt have an instruction buffer 

	uint8_t x, y;
	
	for(y = 0; y < buf->size_y; y++){

		if(y < 32){
			
			for(x = 0; x < 8; x++){		

				ST7920_send(0x80 | y, 0);				
				ST7920_send(0x80 | x, 0);				
				ST7920_send(buf->vid_buf[2*x + 16*y], 1);		
				ST7920_send(buf->vid_buf[2*x+1 + 16*y], 1);	
			}
		}
		else{

			for(x = 0; x < 8; x++){		

				ST7920_send(0x80 | (y-32), 0);			
				ST7920_send(0x88 | x, 0);
				ST7920_send(buf->vid_buf[2*x + 16*y], 1);
				ST7920_send(buf->vid_buf[2*x+1 + 16*y], 1 );
			}
		}
	}
}



