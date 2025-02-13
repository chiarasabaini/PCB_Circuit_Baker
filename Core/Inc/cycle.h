#ifndef _CYC_
#define _CYC_
#include "stdbool.h"

enum cycle_type{
  CYCLE_MODE_STATIC,
  CYCLE_MODE_CURVE,
  CYCLE_MODE_NONE,
};

enum cycle_satus{
  CYCLE_STOP,
  CYCLE_RUNNING,
};

typedef struct{

    bool status; //on-off cycle
    enum cycle_type cycle_type;
    
}Cycle;

#endif