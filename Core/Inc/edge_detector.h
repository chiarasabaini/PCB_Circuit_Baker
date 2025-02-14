#ifndef _EDGE_DET_
#define _EDGE_DET_

#include <stdint.h>


typedef enum {
    EDGE_NONE = 0,  
    EDGE_RISING,    
    EDGE_FALLING    
} EdgeType;


typedef struct {
    uint8_t current_state;  
    uint8_t previous_state; 
    uint8_t edge_detected;  
} edge_detector_t;

/**
 * @brief Edge detector (RISING/FALLING) of a digital signal.
 * @param detector 
 * @param current_state 
 * @return Detected edge (EDGE_NONE, EDGE_RISING, EDGE_FALLING).
 */
EdgeType edge_detect(edge_detector_t *detector, uint8_t current_state);

#endif 