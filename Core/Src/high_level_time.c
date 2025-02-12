#include "high_level_time.h"


void high_lv_timer_init(hilv_time *t, uint8_t hour, uint8_t min, uint8_t sec){
    t->hour = hour;
    t->min = min;
    t->sec = sec;
    t->seconds_counter = 0;
}

void high_lv_update_time(hilv_time *t){
  
  t->sec++;

  if( (t->sec!=0) && ((t->sec%60) == 0) ){
    t->sec = 0;
    t->min++;

    if( (t->min!=0) && ((t->min%60) == 0) ){
      t->min = 0;
      t->hour++;

      if( (t->hour!=0) && ((t->hour%99) == 0) ){
        t->hour = 0;
      }
    }
  }

}
