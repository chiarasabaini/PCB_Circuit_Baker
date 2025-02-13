#include "adc.h"


TIM_HandleTypeDef *_htim;
ADC_HandleTypeDef *_hadc;
DMA_HandleTypeDef _hdma_adc;


float vref_int = 0.0f;

uint16_t buffer[WINDOW_SIZE] = {0};
uint32_t buffer_sum = 0;
uint16_t buffer_index = 0;

uint16_t moving_average_filter(uint16_t raw_adc_value) {

    buffer_sum -= buffer[buffer_index];

    buffer[buffer_index] = raw_adc_value;
    buffer_sum += raw_adc_value;

    buffer_index = (buffer_index + 1) % WINDOW_SIZE;

    return (uint16_t)(buffer_sum / WINDOW_SIZE);
}


/**
  * @brief ADC Transfer Initialization Function. Starts TIMER events and DMA continuous transfer
  * @param htim ADC start-conversion trigger, 0.5 kHz
  * @param hadc 
  * @retval None
  */
void adc_init(TIM_HandleTypeDef *htim, ADC_HandleTypeDef *hadc, DMA_HandleTypeDef hdma_adc, uint16_t *adc_buffer, uint16_t size){

    _htim = htim;
    _hadc = hadc;
    _hdma_adc = hdma_adc;

    HAL_TIM_Base_Start(_htim);
    HAL_ADC_Start_DMA(_hadc, (uint32_t*)adc_buffer, size);

}


void acquisition_stop(void){ 
    HAL_ADC_Stop_DMA(_hadc);
}

/**
  * @brief Voltage conversion
  * @param v voltage channel selection 
  * @param adc_buffer
  * @retval voltage value of the selected channel
  */
float adc_to_v(voltagesource v, uint16_t *adc_buffer){
    
    return adc_buffer[v] * CONVERTION_FACTOR;
    //return (3.3 * (*VREFINT_CAL) * adc_buffer[v] ) / (adc_buffer[VREFINT] * MAX);
}

/**
  * @brief Converts the entire adc buffer and fills an output buffer with the voltages
  * @param adc_buffer
  * @param output_buffer 
  * @param size
  * @retval None
  */
void adc_to_v_buff(uint16_t *adc_buffer, float *output_buffer, uint8_t size){
    
    for(uint8_t i = 0; i < size; i++){
        output_buffer[i] = adc_to_v(i, adc_buffer);
    }
    
}



