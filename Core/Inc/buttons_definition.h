#ifndef _PB_S_
#define _PB_S_

/*    _         _   _                 _               _                     _      __ _      _ _   _          
     | |__ _  _| |_| |_ ___ _ _    __| |_ _ _ _  _ __| |_ _  _ _ _ ___   __| |___ / _(_)_ _ (_) |_(_)___ _ _  
     | '_ \ || |  _|  _/ _ \ ' \  (_-<  _| '_| || / _|  _| || | '_/ -_| / _` / -_)  _| | ' \| |  _| / _ \ ' \ 
     |_.__/\_,_|\__|\__\___/_||_| /__/\__|_|  \_,_\__|\__|\_,_|_| \___/ \__,_\___|_| |_|_||_|_|\__|_\___/_||_|
*/ 

#include "main.h"
#include "pb_behaviour.h"

typedef enum {
    INPUT_NONE,    
    INPUT_UP,      
    INPUT_DOWN,    
    INPUT_LEFT,  
    INPUT_RIGHT,
    INPUT_SELECT     
} InputEvent;

extern pb_str UP_BT;
extern pb_str DWN_BT;
extern pb_str LT_BT;
extern pb_str RT_BT;
extern pb_str OK_BT;

void input_buttons_definition(void);
InputEvent get_input_event_db();

#endif