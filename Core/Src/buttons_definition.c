#include "buttons_definition.h"
#include "edge_detector.h"

pb_str UP_BT;
pb_str DWN_BT;
pb_str LT_BT;
pb_str RT_BT;
pb_str OK_BT;

edge_detector_t btn_up_ed = {0, 0, EDGE_NONE};
edge_detector_t btn_dwn_ed = {0, 0, EDGE_NONE};
edge_detector_t btn_lt_ed = {0, 0, EDGE_NONE};
edge_detector_t btn_rt_ed = {0, 0, EDGE_NONE};
edge_detector_t btn_ok_ed = {0, 0, EDGE_NONE};


//stm32 specific wrapper functions
bool update_OK() {
    return !HAL_GPIO_ReadPin(OK_BTN_GPIO_Port, OK_BTN_Pin);
}
bool update_UP() {
    return !HAL_GPIO_ReadPin(UP_BTN_GPIO_Port, UP_BTN_Pin);
}
bool update_DWN() {
    return !HAL_GPIO_ReadPin(DWN_BTN_GPIO_Port,DWN_BTN_Pin);
}
bool update_RT() {
    return !HAL_GPIO_ReadPin(RT_BTN_GPIO_Port,RT_BTN_Pin);
}
bool update_LT() {
    return !HAL_GPIO_ReadPin(LT_BTN_GPIO_Port,LT_BTN_Pin);
}


void input_buttons_definition(void){

    //tie sysTick variable to pb_behaviour timing variable
    UP_BT.t = &uwTick;
    DWN_BT.t = &uwTick;
    RT_BT.t = &uwTick;
    LT_BT.t = &uwTick;
    OK_BT.t = &uwTick;

    OK_BT.input_f_ptr = update_OK;
    UP_BT.input_f_ptr = update_UP;
    DWN_BT.input_f_ptr = update_DWN;
    RT_BT.input_f_ptr = update_RT;
    LT_BT.input_f_ptr = update_LT;

    //button behaviour settings    
    OK_BT.db_time = 7;
    OK_BT.lp_time = 1000;
    OK_BT.mt_time = 1000;
    OK_BT.tap_num = 2;

    UP_BT.db_time = 7;
    UP_BT.lp_time = 1000;
    UP_BT.mt_time = 1000;
    UP_BT.tap_num = 2;

    DWN_BT.db_time = 7;
    DWN_BT.lp_time = 1000;
    DWN_BT.mt_time = 1000;
    DWN_BT.tap_num = 2;

    RT_BT.db_time = 7;
    RT_BT.lp_time = 1000;
    RT_BT.mt_time = 1000;
    RT_BT.tap_num = 2;

    LT_BT.db_time = 7;
    LT_BT.lp_time = 1000;
    LT_BT.mt_time = 1000;
    LT_BT.tap_num = 2;

    //edge detection for buttons

}

InputEvent get_input_event_db(){

 InputEvent in = INPUT_NONE;
    
    EdgeType up_rs_edg = edge_detect(&btn_up_ed, pb_debounce(&UP_BT));
    EdgeType dwn_rs_edg = edge_detect(&btn_dwn_ed, pb_debounce(&DWN_BT));
    EdgeType rt_rs_edg = edge_detect(&btn_rt_ed, pb_debounce(&RT_BT));
    EdgeType lt_rs_edg = edge_detect(&btn_lt_ed, pb_debounce(&LT_BT));
    EdgeType ok_rs_edg = edge_detect(&btn_ok_ed, pb_debounce(&OK_BT));

    if (up_rs_edg == EDGE_RISING) 
        in = INPUT_UP;
    
    if (dwn_rs_edg == EDGE_RISING) 
        in = INPUT_DOWN;

    if (rt_rs_edg == EDGE_RISING) 
        in = INPUT_RIGHT;
    
    if (lt_rs_edg == EDGE_RISING) 
        in = INPUT_LEFT;
    
    if (ok_rs_edg == EDGE_RISING) 
        in = INPUT_SELECT;
    
    return in;  
}
