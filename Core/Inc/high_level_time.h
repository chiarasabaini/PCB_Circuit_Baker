#ifndef __HIGHLEVEL_TIME__
#define __HIGHLEVEL_TIME__

#include <stdint.h>

typedef struct hilv_time {

    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint16_t seconds_counter;
}hilv_time;


void high_lv_timer_init(hilv_time *t, uint8_t hour, uint8_t min, uint8_t sec);
void high_lv_update_time(hilv_time *t);  

#endif