/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "stdbool.h"
#include "math.h"
#include "string.h"
#include "stm32_max6675.h"
#include "GEVA.h"
#include "stm32_ST7920_spi.h"
//#include "phase_control.h"  //unsuitable with current ssr 
#include "power_modulator.h"
#include "pid.h"
#include "adc.h"
#include "buttons_definition.h"
#include "menu.h"
#include "cycle.h"
#include "icons.h"
#include "flash_writer.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc;
DMA_HandleTypeDef hdma_adc;

I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim7;
TIM_HandleTypeDef htim15;
TIM_HandleTypeDef htim16;
TIM_HandleTypeDef htim17;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/*
timers definition

TIM3: BUZZER PWM
TIM6: 1s temperature reading timer
TIM7: us timer for delay_us() 
TIM15: adc trigger 1khz
TIM16: FAN PWM
TIM17: lcd refresh
*/

//video buffer
video_buffer vb;

//thermocouple amplifiers
max6675 tc_A;
max6675 tc_B;
float current_temp; //main temperature used for computation

//chart line
data_container target_temp_chart;
data_container current_temp_chart;

//timers call back flags
volatile uint8_t lcd_refresh_elapsed = 0;
volatile uint8_t second_elapsed = 0;
volatile uint8_t zero_crossed = 0;

//temperature controller

//pid tuning variables
float Kp = 0.0f; 
float Ki = 0.0f; 
float Kd = 0.0f; 

pid_controller temp_pid;
volatile uint8_t power_pct = 0;
float target_temp = 0.0f;
volatile uint8_t crossing_cnt = 0;

reflow_profile temp_curves[3];
int reflow_profile_index;

#define NUM_CHANNELS   4
#define SAMPLES_PER_CHANNEL 1000  // 500Hz x 2 seconds
#define TOTAL_SAMPLES SAMPLES_PER_CHANNEL*NUM_CHANNELS

uint16_t adc_values[TOTAL_SAMPLES]={0};

float f = 0.0f;
float Vrms = 0.0f;

//cycle time keeping
hilv_time hl_t;

//menu structure
Menu menu;

//cycle manager
Cycle operation_cycle;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM16_Init(void);
static void MX_TIM17_Init(void);
static void MX_TIM15_Init(void);
static void MX_TIM7_Init(void);
static void MX_TIM6_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* ===============================SERVICE FUNCTIONS & VARS    DO NOT DELETE====================================== */

//timers callback
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  
  if (htim->Instance == TIM17) //lcd refresh
    lcd_refresh_elapsed = 1;      

  if(htim->Instance == TIM6)  //1 sec elapsed. used for temperature reading
    second_elapsed = 1;

  if(htim->Instance == TIM15)  //1 sec elapsed. used for temperature reading
    f=0.0;
}


volatile uint16_t buffer_index = 0;  // Indice di scrittura nel buffer
uint16_t temp_adc_data[4];  // Buffer temporaneo per i 4 canali

void DMA1_Channel1_IRQHandler(void) {
  if (DMA1->ISR & DMA_ISR_TCIF1) {  // Controlla se il trasferimento è completo
      DMA1->IFCR |= DMA_IFCR_CTCIF1;  // Pulisce il flag dell'interrupt

      // Controlla se abbiamo spazio nel buffer principale
      if (buffer_index < TOTAL_SAMPLES) {
          adc_values[buffer_index++] = temp_adc_data[0];  // Canale 0
          adc_values[buffer_index++] = temp_adc_data[1];  // Canale 1
          adc_values[buffer_index++] = temp_adc_data[2];  // Canale 2
          adc_values[buffer_index++] = temp_adc_data[3];  // Canale 3
      }

      // Se il buffer è pieno, puoi segnalare che l'acquisizione è completa
      if (buffer_index >= TOTAL_SAMPLES) {
          buffer_index = 0;  // Opzionale: Reset per riempire di nuovo
      }
  }
}


//stuff related to power goes in here --->  ZERO crossing callback

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == ZERO_CROSS_Pin) {
      zero_crossed = 1;
      crossing_cnt++;
      power_modulator_set(power_pct, &crossing_cnt);
    }
    
}

void parse_int(uint16_t num, uint16_t filtered, char *output, char start_c, char end_c ){
  sprintf(output, "%c%d,%d%c", start_c, filtered, num, end_c); 
}

void delay_us(uint32_t delay) {

  uint32_t cnt_start = htim7.Instance->CNT;

  while( ( htim7.Instance->CNT - cnt_start) < delay ) {}

}

void heating_animation(){

  static bool heating_animation_frame;

  heating_animation_frame = !heating_animation_frame;
  menu.tabs_vect[TAB_MAIN].items[STATUS].item_data.icon = (heating_animation_frame) ? &heating : &not_heating; //heating animation

}

void heating_animation_clear(){
  if(menu.current_tab == TAB_MAIN)
    clear_sector(menu.tabs_vect[TAB_MAIN].items[STATUS].pos_x,menu.tabs_vect[TAB_MAIN].items[STATUS].pos_y,16,16,&vb);
}

/* ================================================================================================================ */


void half_transfer_handler(DMA_HandleTypeDef *hdma)
{
    for(int i=0; i<32; i++){
      Vrms += adc_values[i];
    }
}

void transfer_handler(DMA_HandleTypeDef *hdma)
{
    // Azione quando il trasferimento è completato
}

// Nel codice di inizializzazione del DMA

//void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc){
//
//}
void HAL_DMA_Callback(DMA_HandleTypeDef *hdma)
{
    if (__HAL_DMA_GET_FLAG(hdma_adc, DMA_FLAG_HT1))  // Half Transfer Interrupt Flag
    {
        __HAL_DMA_CLEAR_FLAG(hdma_adc, DMA_FLAG_HT1);
        HAL_GPIO_TogglePin(SSR_A_GPIO_Port,SSR_A_Pin);
        for(int i=0; i<2000; i++){
        Vrms += adc_values[i];
        }
        // Azione da eseguire quando metà buffer è trasferito
    }

    if (__HAL_DMA_GET_FLAG(hdma_adc, DMA_FLAG_TC1))  // Transfer Complete Interrupt Flag
    {
        __HAL_DMA_CLEAR_FLAG(hdma_adc, DMA_FLAG_TC1);
        // Azione da eseguire quando il trasferimento è completo
    }
}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

    //-------vbc_file start-------

vbc_file mamma_mia;

mamma_mia.header.id = 0xa7;
mamma_mia.header.width = 0x7f;
mamma_mia.header.height = 0x3f;
mamma_mia.header.byte_cnt_h = 0x03;
mamma_mia.header.byte_cnt_l = 0xf5;

uint8_t mamma_mia_sv[1008] = 
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x10, 0x08, 0x00, 0x06, 0x01, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 
	0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x06, 0x10, 0x08, 0x04, 0x03, 0x83, 0x00, 
	0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x10, 0x08, 0x06, 0x00, 0x02, 0x00, 
	0x10, 0x18, 0x30, 0x00, 0x6c, 0x20, 0x00, 0x00, 0x01, 0x02, 0x10, 0x08, 0x03, 0x00, 0x0e, 0x00, 
	0x18, 0x28, 0x30, 0x38, 0xee, 0x60, 0x00, 0x00, 0x01, 0x82, 0x10, 0x0c, 0x01, 0xc1, 0xf8, 0x00, 
	0x1c, 0x68, 0x78, 0x6c, 0xaa, 0xe3, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 
	0x16, 0x48, 0x48, 0x45, 0xab, 0xa7, 0x80, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 
	0x13, 0xc8, 0x48, 0x47, 0x29, 0xa4, 0x80, 0x00, 0x00, 0x63, 0x87, 0x7b, 0xff, 0xe7, 0x00, 0x00, 
	0x10, 0x08, 0x8c, 0x46, 0x29, 0x24, 0x80, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 
	0x10, 0x08, 0x84, 0x46, 0x28, 0x24, 0xc0, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 
	0x10, 0x08, 0x84, 0x40, 0x28, 0x24, 0x40, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 
	0x10, 0x08, 0x84, 0x40, 0x28, 0x27, 0xf0, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 
	0x10, 0x09, 0xfe, 0x40, 0x28, 0x24, 0x40, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 
	0x10, 0x09, 0x02, 0x40, 0x28, 0x2c, 0x40, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 
	0x10, 0x09, 0x02, 0x40, 0x28, 0x28, 0x40, 0x00, 0x04, 0x10, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 
	0x10, 0x09, 0x03, 0x40, 0x28, 0x28, 0x00, 0x00, 0x04, 0x18, 0x00, 0x1f, 0xff, 0xe4, 0x00, 0x00, 
	0x10, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x04, 0x0f, 0xff, 0xf0, 0x00, 0x3e, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x00, 0x04, 0x08, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x11, 0xc4, 0x18, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x11, 0x44, 0x10, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2f, 0x11, 0x64, 0x10, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x07, 0xc0, 0x21, 0x13, 0x24, 0x10, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x1c, 0x40, 0x21, 0x12, 0x24, 0x30, 0x1f, 0x00, 0x1f, 0x00, 0x80, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x30, 0x40, 0x21, 0x12, 0x20, 0x20, 0x7f, 0x80, 0x1f, 0x80, 0x80, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x20, 0x40, 0x21, 0x13, 0xe0, 0x20, 0x7f, 0xc0, 0x3f, 0xc0, 0x80, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x20, 0xc0, 0x21, 0x16, 0x20, 0x20, 0x7f, 0x40, 0x3f, 0x40, 0x80, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x20, 0x80, 0x00, 0x04, 0x24, 0x20, 0x7e, 0x40, 0x7e, 0x60, 0x80, 0x00, 
	0x00, 0x00, 0x00, 0x3c, 0x61, 0x80, 0x00, 0x00, 0x24, 0x20, 0x3e, 0x60, 0x1f, 0xc0, 0x80, 0x00, 
	0x00, 0x00, 0x00, 0x64, 0x41, 0x00, 0x00, 0x00, 0x00, 0x20, 0x03, 0xe0, 0x00, 0x00, 0x80, 0x00, 
	0x00, 0x00, 0x00, 0x44, 0x43, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 
	0x00, 0x00, 0x00, 0xc4, 0x42, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x10, 0x00, 0x00, 0x80, 0x00, 
	0x00, 0x00, 0x00, 0x84, 0x42, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x10, 0x80, 0x00, 0x80, 0x00, 
	0x00, 0x00, 0x00, 0x84, 0x42, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x30, 0x00, 0x00, 0x80, 0x00, 
	0x00, 0x00, 0x00, 0x84, 0x42, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x20, 0x00, 0x01, 0x80, 0x00, 
	0x00, 0x00, 0x00, 0x84, 0x42, 0x00, 0x00, 0x00, 0x00, 0x20, 0x7c, 0x20, 0x7f, 0xfd, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x84, 0x42, 0x00, 0x00, 0x00, 0x00, 0x21, 0xff, 0xf0, 0x7f, 0xff, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0xc4, 0xc2, 0x03, 0xf8, 0x00, 0x00, 0x27, 0xff, 0xf8, 0xff, 0xff, 0x0f, 0x80, 
	0x00, 0x00, 0x00, 0x46, 0x82, 0x0e, 0x0c, 0x00, 0x00, 0x3f, 0xff, 0xfc, 0xf8, 0x1f, 0x0f, 0xc0, 
	0x00, 0x00, 0x00, 0x42, 0x82, 0x18, 0x06, 0x01, 0xc0, 0x1f, 0xff, 0xff, 0xf0, 0x0f, 0x8c, 0xe0, 
	0x00, 0x00, 0x01, 0xe3, 0x83, 0x30, 0x02, 0x07, 0xe0, 0x3c, 0x00, 0x3f, 0xe0, 0x07, 0x80, 0x60, 
	0x00, 0x00, 0x03, 0x61, 0xc1, 0xe0, 0x03, 0x0e, 0x70, 0xb8, 0x00, 0x1f, 0xe7, 0x83, 0xc0, 0x60, 
	0x00, 0x00, 0x02, 0x71, 0xc0, 0xc3, 0xe0, 0xcc, 0x30, 0xfc, 0x00, 0x16, 0xfc, 0x83, 0xc0, 0x60, 
	0x00, 0x00, 0x02, 0x50, 0xe0, 0x06, 0x30, 0x4c, 0x01, 0xe6, 0x01, 0xc1, 0xc1, 0x82, 0xe0, 0xe0, 
	0x00, 0x00, 0x02, 0x78, 0x60, 0x04, 0x18, 0x4c, 0x03, 0xe2, 0x00, 0xfe, 0x03, 0x04, 0x7f, 0xe0, 
	0x00, 0x00, 0x02, 0x2c, 0x00, 0x0c, 0x0f, 0xce, 0x03, 0xc3, 0x00, 0x60, 0x0e, 0x0c, 0x3f, 0xc0, 
	0x00, 0x00, 0x03, 0x36, 0x00, 0x08, 0x19, 0x87, 0x0f, 0xc1, 0x80, 0x38, 0x38, 0x08, 0x3f, 0x80, 
	0x00, 0x00, 0x01, 0x1f, 0x00, 0x00, 0x18, 0x87, 0xff, 0x80, 0x80, 0x0f, 0xe0, 0x18, 0x00, 0x00, 
	0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x30, 0x83, 0xff, 0x00, 0xc0, 0x00, 0x00, 0x30, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0xc0, 0x00, 0x1f, 0xe0, 0x81, 0xfc, 0x00, 0x60, 0x00, 0x00, 0x60, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x03, 0x80, 0x00, 0x00, 0x38, 0x00, 0x01, 0xe0, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x30, 0x30, 0x00, 0x06, 0x00, 0x00, 0x00, 0x2f, 0x00, 0x03, 0x30, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x18, 0x10, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x61, 0xe0, 0x1e, 0x10, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x0c, 0x18, 0x00, 0x18, 0x00, 0x00, 0x00, 0xe0, 0x3f, 0xf0, 0x18, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x03, 0x0c, 0x00, 0x30, 0x00, 0x00, 0x07, 0xb0, 0x07, 0x00, 0x3c, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x01, 0x86, 0x00, 0xe0, 0x00, 0x00, 0x1d, 0x18, 0x07, 0x80, 0x66, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0xc3, 0x8f, 0x80, 0x00, 0x00, 0xf0, 0x0e, 0x0f, 0xe1, 0xc3, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x40, 0x04, 0x00, 0x00, 0x03, 0x81, 0x03, 0xc8, 0x37, 0x01, 0x80, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x70, 0x04, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x78, 0xfc, 0x00, 0xc0, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x58, 0x04, 0x00, 0x00, 0x08, 0x00, 0x00, 0x01, 0x80, 0x00, 0x60, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x4f, 0xdc, 0x00, 0x00, 0x38, 0x00, 0x00, 0x03, 0x00, 0x00, 0x20, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x40, 0x04, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x03, 0x00, 0x00, 0x30, 0x00
};

mamma_mia.body = mamma_mia_sv;

//-------vbc_file stop -------

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_ADC_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_TIM3_Init();
  MX_TIM16_Init();
  MX_TIM17_Init();
  MX_TIM15_Init();
  MX_TIM7_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */
  
/* =================================================video buffer init============================================== */

  init_buf(&vb,128,64);
  set_mode(NORMAL,&vb);
  set_orientation(LANDSCAPE,&vb);

/* ===============================================chart parameter================================================== */

  current_temp_chart.o_x = 3;
  current_temp_chart.o_y = 13;
  current_temp_chart.type = LINE;
  current_temp_chart.dim_x = 104;
  current_temp_chart.dim_y_neg = 0;
  current_temp_chart.dim_y_pos = 37;
  current_temp_chart.max = 300;
  current_temp_chart.min = 0;
  current_temp_chart.x_division = 52;

  target_temp_chart.o_x = 3;
  target_temp_chart.o_y = 13;
  target_temp_chart.type = LINE_CHUNK;
  target_temp_chart.dim_x = 104;
  target_temp_chart.dim_y_neg = 0;
  target_temp_chart.dim_y_pos = 37;
  target_temp_chart.max = 300;
  target_temp_chart.min = 0;
  target_temp_chart.x_division = 52;
  target_temp_chart.max_x = 280;

  //__HAL_DMA_ENABLE_IT(&hdma_adc, DMA_IT_HT);
  //__HAL_DMA_ENABLE_IT(&hdma_adc, DMA_IT_TC);
  //HAL_DMA_RegisterCallback(&hdma_adc, HAL_DMA_XFER_HALFCPLT_CB_ID, half_transfer_handler);
  //HAL_DMA_RegisterCallback(&hdma_adc, HAL_DMA_XFER_CPLT_CB_ID, transfer_handler);

/* ====================================================display init================================================ */

  init_display(hspi2,LCD_CS_GPIO_Port,LCD_RST_Pin,LCD_CS_GPIO_Port,LCD_CS_Pin);
  HAL_TIM_Base_Start_IT(&htim17); //timer refresh lcd avvio

/* ====================================================menu icons init============================================= */
  
  menu_init();

/* ==============================================thermocouple amplifier init======================================= */

  max6675_init(&tc_A,CS_A_GPIO_Port,CS_A_Pin,&hspi1);
  max6675_init(&tc_B,CS_B_GPIO_Port,CS_B_Pin,&hspi1);
  HAL_TIM_Base_Start_IT(&htim6); //timer lettura temp avvio

/* ================================================buttons init==================================================== */
  input_buttons_definition();
  
/* ==============================================power modulator init============================================== */
  
  power_modulator_init(SSR_A_GPIO_Port,SSR_A_Pin,SSR_B_GPIO_Port,SSR_B_Pin);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

/* ==================================================pid controller init=========================================== */
    
  Kp = flash_read_float(KP_DATA_FLASH_ADDRESS);
  Ki = flash_read_float(KI_DATA_FLASH_ADDRESS);
  Kd = flash_read_float(KD_DATA_FLASH_ADDRESS);

  pid_init(&temp_pid, Kp, Ki, Kd, 1.0); // kp, ki, kd, dt

/*
  flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, 120.0f, 0);
  flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, 60.0f, 1);
  flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, 150.0f, 2);
  flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, 90.0f, 3);
  flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, 230.0f, 4);
  flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, 40.0f, 5);

  flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, 100.0f, 6);
  flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, 20.0f, 7);
  flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, 120.0f, 8);
  flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, 60.0f, 9);
  flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, 200.0f, 10);
  flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, 30.0f, 11);

  flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, 40.0f, 12);
  flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, 60.0f, 13);
  flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, 40.0f, 14);
  flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, 20.0f, 15);
  flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, 50.0f, 16);
  flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, 90.0f, 17);
*/

  temp_curves[0].preheat_temp = flash_read_float(PREHEAT_TEMP_1_ADDRESS);
  temp_curves[0].preheat_time = flash_read_float(PREHEAT_TIME_1_ADDRESS);
  temp_curves[0].soak_temp = flash_read_float(SOAK_TEMP_1_ADDRESS);
  temp_curves[0].soak_time = flash_read_float(SOAK_TIME_1_ADDRESS);
  temp_curves[0].peak_temp = flash_read_float(PEAK_TEMP_1_ADDRESS);
  temp_curves[0].reflow_time = flash_read_float(REFLOW_TIME_1_ADDRESS);

  temp_curves[1].preheat_temp = flash_read_float(PREHEAT_TEMP_2_ADDRESS);
  temp_curves[1].preheat_time = flash_read_float(PREHEAT_TIME_2_ADDRESS);
  temp_curves[1].soak_temp = flash_read_float(SOAK_TEMP_2_ADDRESS);
  temp_curves[1].soak_time = flash_read_float(SOAK_TIME_2_ADDRESS);
  temp_curves[1].peak_temp = flash_read_float(PEAK_TEMP_2_ADDRESS);
  temp_curves[1].reflow_time = flash_read_float(REFLOW_TIME_2_ADDRESS);

  temp_curves[2].preheat_temp = flash_read_float(PREHEAT_TEMP_3_ADDRESS);
  temp_curves[2].preheat_time = flash_read_float(PREHEAT_TIME_3_ADDRESS);
  temp_curves[2].soak_temp = flash_read_float(SOAK_TEMP_3_ADDRESS);
  temp_curves[2].soak_time = flash_read_float(SOAK_TIME_3_ADDRESS);
  temp_curves[2].peak_temp = flash_read_float(PEAK_TEMP_3_ADDRESS);
  temp_curves[2].reflow_time = flash_read_float(REFLOW_TIME_3_ADDRESS);
  
/* ==================================================operation cycle init========================================== */

  operation_cycle.cycle_type = CYCLE_MODE_NONE;
  operation_cycle.status = CYCLE_STOP;

/* ==============================================adc transfer init================================================= */



  adc_init(&htim15, &hadc, hdma_adc, temp_adc_data, 4);

  acquisition_start();
  HAL_Delay(20);
  acquisition_stop();

  put_vbc(mamma_mia,0,0,&vb);
  update_display(&vb);
  HAL_Delay(1000);
  clr_buffer(&vb);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1) {

    handle_input(&menu);
    
    switch (operation_cycle.status) {

    case CYCLE_RUNNING:

      if(second_elapsed){

        current_temp = (max6675_read_temp(tc_B) + max6675_read_temp(tc_A)) / 2.0f;
     
        target_temp = get_target_temp(&temp_curves[reflow_profile_index-1], operation_cycle.cycle_type, hl_t.seconds_counter);

        if(operation_cycle.cycle_type != CYCLE_MODE_NONE){

          power_pct = pid_compute(&temp_pid,target_temp,current_temp);
          heating_animation();
      
          if(operation_cycle.cycle_type == CYCLE_MODE_STATIC)
            update_chart(current_temp,&current_temp_chart,&vb);  

          if( !(hl_t.seconds_counter%5) && operation_cycle.cycle_type == CYCLE_MODE_CURVE )
            update_chart(current_temp,&current_temp_chart,&vb);

          if(target_temp < 0.1){
            heating_animation_clear();
            menu.tabs_vect[TAB_MAIN].items[STATUS].item_data.icon = &cooling;
          }

        }
        high_lv_update_time(&hl_t);
        hl_t.seconds_counter++;
        second_elapsed = 0;
      }

    break;
    
    default:
      power_pct = 0;
      heating_animation_clear();
    break;
    
  }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  
  //char nf_buffer[50] = {0};
  //char filtered_buffer[50] = {0};
  //uint16_t adc_va = adc_values[2];
  //uint16_t f_adc_va = moving_average_filter(adc_va);


  /* ============================================ REFRESH LCD ======================================================= */

    if(lcd_refresh_elapsed) { //every half second
        
        update_display(&vb);
        lcd_refresh_elapsed = 0 ;
      }
  /* ================================================================================================================ */ 
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI14
                              |RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI14CalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_SYSCLK;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC_Init(void)
{

  /* USER CODE BEGIN ADC_Init 0 */

  /* USER CODE END ADC_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC_Init 1 */

  /* USER CODE END ADC_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc.Instance = ADC1;
  hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerAutoPowerOff = DISABLE;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T15_TRGO;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc.Init.DMAContinuousRequests = DISABLE;
  hadc.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  sConfig.SamplingTime = ADC_SAMPLETIME_7CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_1;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_4;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_6;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC_Init 2 */
  
  ADC1->IER |= ADC_IER_EOSIE;

  ADC1->CR |= ADC_CR_ADCAL;
  while (ADC1->CR & ADC_CR_ADCAL);

  /* USER CODE END ADC_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x0000020B;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_16BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_1LINE;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi2.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 47;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 1000;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 767;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 62499;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief TIM7 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM7_Init(void)
{

  /* USER CODE BEGIN TIM7_Init 0 */

  /* USER CODE END TIM7_Init 0 */

  /* USER CODE BEGIN TIM7_Init 1 */

  /* USER CODE END TIM7_Init 1 */
  htim7.Instance = TIM7;
  htim7.Init.Prescaler = 48-1;
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period = 0xffff-1;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM7_Init 2 */

  /* USER CODE END TIM7_Init 2 */

}

/**
  * @brief TIM15 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM15_Init(void)
{

  /* USER CODE BEGIN TIM15_Init 0 */

  /* USER CODE END TIM15_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM15_Init 1 */

  /* USER CODE END TIM15_Init 1 */
  htim15.Instance = TIM15;
  htim15.Init.Prescaler = 5;
  htim15.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim15.Init.Period = 59999;
  htim15.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim15.Init.RepetitionCounter = 0;
  htim15.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim15) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim15, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim15, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM15_Init 2 */

  /* USER CODE END TIM15_Init 2 */

}

/**
  * @brief TIM16 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM16_Init(void)
{

  /* USER CODE BEGIN TIM16_Init 0 */

  /* USER CODE END TIM16_Init 0 */

  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM16_Init 1 */

  /* USER CODE END TIM16_Init 1 */
  htim16.Instance = TIM16;
  htim16.Init.Prescaler = 0;
  htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim16.Init.Period = 65535;
  htim16.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim16.Init.RepetitionCounter = 0;
  htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim16) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim16) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim16, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim16, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM16_Init 2 */

  /* USER CODE END TIM16_Init 2 */
  HAL_TIM_MspPostInit(&htim16);

}

/**
  * @brief TIM17 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM17_Init(void)
{

  /* USER CODE BEGIN TIM17_Init 0 */

  /* USER CODE END TIM17_Init 0 */

  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM17_Init 1 */

  /* USER CODE END TIM17_Init 1 */
  htim17.Instance = TIM17;
  htim17.Init.Prescaler = 48000-1;
  htim17.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim17.Init.Period = 500-1;
  htim17.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim17.Init.RepetitionCounter = 0;
  htim17.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim17) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_OC_Init(&htim17) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_OC_ConfigChannel(&htim17, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim17, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM17_Init 2 */

  /* USER CODE END TIM17_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, RE_485_Pin|DE_485_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin|LCD_RST_Pin|SSR_B_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, CS_B_Pin|CS_A_Pin|SSR_A_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : ZERO_CROSS_Pin */
  GPIO_InitStruct.Pin = ZERO_CROSS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ZERO_CROSS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RE_485_Pin DE_485_Pin */
  GPIO_InitStruct.Pin = RE_485_Pin|DE_485_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : OK_BTN_Pin RT_BTN_Pin LT_BTN_Pin DWN_BTN_Pin */
  GPIO_InitStruct.Pin = OK_BTN_Pin|RT_BTN_Pin|LT_BTN_Pin|DWN_BTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_CS_Pin LCD_RST_Pin */
  GPIO_InitStruct.Pin = LCD_CS_Pin|LCD_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : CS_B_Pin CS_A_Pin */
  GPIO_InitStruct.Pin = CS_B_Pin|CS_A_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : UP_BTN_Pin */
  GPIO_InitStruct.Pin = UP_BTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(UP_BTN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SSR_A_Pin */
  GPIO_InitStruct.Pin = SSR_A_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(SSR_A_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SSR_B_Pin */
  GPIO_InitStruct.Pin = SSR_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(SSR_B_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
