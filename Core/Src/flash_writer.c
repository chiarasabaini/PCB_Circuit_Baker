#include "flash_writer.h"



#define PAGE_SIZE 1024  
#define MAX_FLOATS_PER_PAGE (PAGE_SIZE / sizeof(float)) 

float flash_read_float(uint32_t address) {
    return *(float*)address; //ottiene il valore memorizzato
}



void flash_write_float(uint32_t base_address, float new_value, uint16_t offset) {
    
    float buffer[MAX_FLOATS_PER_PAGE] = {0}; //buffer temporaneo

    for (int i = 0; i < MAX_FLOATS_PER_PAGE; i++) {
        buffer[i] = flash_read_float(base_address + i * sizeof(float));
    }

    //buffer[(base_address - CURVE_FLASH_INITIAL_ADDRESS)/sizeof(float) + offset] = new_value;
    buffer[offset] = new_value;
    HAL_FLASH_Unlock(); //sblocco flash

    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError;

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = base_address;
    EraseInitStruct.NbPages = 1; 

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK) { //se la cancellazione fallisce
        HAL_FLASH_Lock();
        return;
    }

    for (int i = 0; i < MAX_FLOATS_PER_PAGE; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, base_address + i * sizeof(float), *(uint32_t*)&buffer[i]) != HAL_OK) {
            HAL_FLASH_Lock();
            return; 
        }
    }

    HAL_FLASH_Lock();
}

