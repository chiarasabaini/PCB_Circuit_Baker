#ifndef _PWR_MOD_
#define _PWR_MOD_

#include "main.h"


#define MAIN_FREQ 50
#define T_PERIOD (1.0/MAIN_FREQ)
#define CROSSING_SAMPLE 100

#define SSRB
#define SSRA


void power_modulator_set(uint8_t pwr_pct, volatile uint8_t *crsng_cnt); //comment SSRA or SSRB if you dont want them activating
void power_modulator_init(GPIO_TypeDef *SSR_A_port, uint16_t SSR_A_pin, GPIO_TypeDef *SSR_B_port, uint16_t SSR_B_pin);
//void power_calculator();


#endif