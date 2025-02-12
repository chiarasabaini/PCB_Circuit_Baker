#include "pid.h"


extern float target_temp;

void pid_init(pid_controller *pid, float kp, float ki, float kd, float dt) {
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->dt = dt;
    pid->integral = 0.0f;
    pid->previous_error = 0.0f;
}

uint8_t pid_compute(pid_controller *pid, float setpoint, float measurement) {
    float error = setpoint - measurement;
    pid->integral += error * pid->dt;
    float derivative = (error - pid->previous_error) / pid->dt;

    float output = pid->kp * error + pid->ki * pid->integral + pid->kd * derivative;
    pid->previous_error = error;


    if (output > 100.0f) {
        output = 100.0f;
    } else if (output < 0.0f) {
        output = 0.0f;
    }

    return (uint8_t)output;
}

float get_target_temp(reflow_profile *profile, enum cycle_type ct, int time) {

    switch(ct){    

        case CYCLE_MODE_STATIC:
            return target_temp;
        break;

        case CYCLE_MODE_CURVE:

            if (time < profile->preheat_time) {
                return profile->preheat_temp * (float)time / profile->preheat_time;
            }
            else if (time < (profile->preheat_time + profile->soak_time)) {
                int t_soak = time - profile->preheat_time;
                return profile->preheat_temp + (profile->soak_temp - profile->preheat_temp) * (float)t_soak / profile->soak_time;
            }
            else if (time < (profile->preheat_time + profile->soak_time + profile->reflow_time)) {
                int t_reflow = time - (profile->preheat_time + profile->soak_time);
                return profile->soak_temp + (profile->peak_temp - profile->soak_temp) * (float)t_reflow / profile->reflow_time;
            }

            else {
                
                return 0;
            }

        break;

        default:
        break;
    
    }

}

