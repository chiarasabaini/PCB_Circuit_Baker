#include "power_modulator.h"
#include "adc.h"

GPIO_TypeDef *_SSR_A_port;
GPIO_TypeDef *_SSR_B_port;
uint16_t _SSR_A_pin;
uint16_t _SSR_B_pin;





/*
uint16_t angle_to_delay(float _firing_angle){ //returns time in us
    
    float time_delay = 0; 
    
    if ( _firing_angle > 180.0 )
        time_delay= 180.0;
    if ( _firing_angle < 0.0 )
        time_delay=0.0;
    
    time_delay = (((_firing_angle / 180.0) * T_PERIOD) * 1000000);

    return (uint16_t)time_delay;
}*/



void power_modulator_init(GPIO_TypeDef *SSR_A_port, uint16_t SSR_A_pin, GPIO_TypeDef *SSR_B_port, uint16_t SSR_B_pin){
    
    _SSR_A_port = SSR_A_port;
    _SSR_B_port = SSR_B_port;
    _SSR_A_pin = SSR_A_pin;
    _SSR_B_pin = SSR_B_pin;

    HAL_GPIO_WritePin(_SSR_A_port,_SSR_A_pin,GPIO_PIN_RESET);
    HAL_GPIO_WritePin(_SSR_B_port,_SSR_B_pin,GPIO_PIN_RESET);
}

void power_modulator_set(uint8_t pwr_pct, volatile uint8_t *crsng_cnt){


    if(*crsng_cnt > CROSSING_SAMPLE){
        *crsng_cnt = 0; 
    }

    else {
        if(*crsng_cnt < pwr_pct) {
#ifdef SSRA
            HAL_GPIO_WritePin(_SSR_A_port,_SSR_A_pin,GPIO_PIN_SET);
#endif
#ifdef SSRB
            HAL_GPIO_WritePin(_SSR_B_port,_SSR_B_pin,GPIO_PIN_SET);
#endif
       }
        else {
#ifdef SSRA
            HAL_GPIO_WritePin(_SSR_A_port,_SSR_A_pin,GPIO_PIN_RESET);
#endif
#ifdef SSRB
            HAL_GPIO_WritePin(_SSR_B_port,_SSR_B_pin,GPIO_PIN_RESET);
#endif

        }
    }

}

