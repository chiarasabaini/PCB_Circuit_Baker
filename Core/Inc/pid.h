#ifndef _PID_
#define _PID_
#include <stdint.h>
#include "cycle.h"

typedef struct {
    float kp; 
    float ki; 
    float kd; 
    float dt; 
    float integral;
    float previous_error; 
} pid_controller;

void pid_init(pid_controller *pid, float kp, float ki, float kd, float dt);
uint8_t pid_compute(pid_controller *pid, float setpoint, float measurement);

typedef struct {
    float preheat_temp;
    float preheat_time;     
    float soak_temp;
    float soak_time;       
    float peak_temp;         
    float reflow_time;       
        
} reflow_profile;

float get_target_temp(reflow_profile *profile, enum cycle_type ct, int time);


#endif 