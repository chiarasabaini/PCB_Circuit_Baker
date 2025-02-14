#include "edge_detector.h"

EdgeType edge_detect(edge_detector_t *detector, uint8_t current_state) {

    detector->current_state = current_state;

    if (detector->current_state && !detector->previous_state)
        detector->edge_detected = EDGE_RISING; 

    else if (!detector->current_state && detector->previous_state)
        detector->edge_detected = EDGE_FALLING;
    
    else 
        detector->edge_detected = EDGE_NONE;

    detector->previous_state = detector->current_state;

    EdgeType detected_edge = detector->edge_detected;
    
    detector->edge_detected = EDGE_NONE;
    return detected_edge;
}
