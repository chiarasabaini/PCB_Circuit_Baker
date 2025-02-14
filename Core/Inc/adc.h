#ifndef __ADC__
#define __ADC__

#include "main.h"

#define V_REF 3.3
#define MAX 4095
#define VREFINT_CAL_ADDR                0x1FFFF7BA
#define VREFINT_CAL ((uint16_t*) VREFINT_CAL_ADDR)
#define CONVERTION_FACTOR V_REF/MAX
#define WINDOW_SIZE 10


typedef enum{
    VOLTAGE_A = 0,
    CURRENT_A = 1,
    VOLTAGE_B = 2,
    CURRENT_B = 3,
    NTC_A = 4,
    NTC_B = 5,
    VREFINT = 6
}voltagesource;


void adc_init(TIM_HandleTypeDef *htim, ADC_HandleTypeDef *hadc, DMA_HandleTypeDef hdma_adc, uint16_t *adc_buffer, uint16_t size);
float adc_to_v(voltagesource v, uint16_t *adc_buffer);
void adc_to_v_buff(uint16_t *adc_buffer, float *output_buffer, uint8_t size);
void acquisition_start(void);
void acquisition_stop(void);

uint16_t moving_average_filter(uint16_t raw_adc_value);

#endif