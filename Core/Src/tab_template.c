#include "tab_template.h"
#include "GEVA.h"

extern video_buffer vb;

void main_tab_template(){

    put_rect(0,53,110,10,&vb);

    put_rect(0,0,110,10,&vb); //lower bar
    put_line(41,1,41,9,&vb);    
        
    put_char(73,2,26,SMALL,&vb); //→    

    put_line(110,0,110,63,&vb); //menu laterale
    put_line(0,0,0,53,&vb);
    put_line(112,45,126,45,&vb);

}

void settings_tab_template() {
  
}

void select_curve_tab_template(){

    put_string(2,55, "SELECT PROFILE",SMALL,&vb);

    put_string(8,42, "Profile 1", SMALL, &vb );
    put_string(8,32, "Profile 2", SMALL, &vb );
    put_string(8,22, "Profile 3", SMALL, &vb );
}

void static_set_tab_template(){

    put_string(2,55, "SET",SMALL,&vb);
    put_string(22,55, "STATIC",SMALL,&vb);
    put_string(60,55, "TEMPERATURE",SMALL,&vb);
    
    put_char(76,30, 0xf9, SMALL, &vb);
    put_char(81,30, 'C', SMALL, &vb);
}

void pid_parameter_tab_template(){
    
    put_string(2,55, "SET PID PARAMETERS",SMALL,&vb);

    put_string(8,42, "Kp:", SMALL, &vb );
    put_string(8,32, "Ki:", SMALL, &vb );
    put_string(8,22, "Kd:", SMALL, &vb );

}

void curves_tab_template(){

    put_string(2,55, "PROFILE PARAMETER",SMALL,&vb);

    put_string(8,42, "Select profile:", SMALL, &vb );
    put_string(8,32, "Edit profile", SMALL, &vb );

}

void curve_param_tab_template(){

    put_string(8,55, "Preheat temp:", SMALL, &vb );
    put_char(117,55, 0xf9, SMALL, &vb);
    put_char(122,55, 'C', SMALL, &vb);
    
    put_string(8,45, "Preheat time:", SMALL, &vb );
    put_char(122,45, 's', SMALL, &vb);

    put_string(8,35, "Soak temp:", SMALL, &vb );
    put_char(117,35, 0xf9, SMALL, &vb);
    put_char(122,35, 'C', SMALL, &vb);

    put_string(24,25, "Soak time:", SMALL, &vb );
    put_char(122,25, 's', SMALL, &vb);
    
    put_string(24,15, "Peak temp:", SMALL, &vb );
    put_char(117,15, 0xf9, SMALL, &vb);
    put_char(122,15, 'C', SMALL, &vb);

    put_string(24,5, "Reflow time:", SMALL, &vb );
    put_char(122,5, 's', SMALL, &vb);
}

void power_monitor_tab_template(){
    
    put_string(3,55, "POWER MONITOR", SMALL, &vb );
    
    put_string(3,45, "VOLTAGE:", SMALL, &vb );
    put_char(100,45, 'V', SMALL, &vb);
    put_string(105,45, "rms", SMALL, &vb );
    
    put_string(3,36, "CURRENT:", SMALL, &vb );
    put_char(100,36, 'A', SMALL, &vb);
    put_string(105,36, "rms", SMALL, &vb );

    put_string(3,27, "POWER:", SMALL, &vb );
    put_char(100,27, 'W', SMALL, &vb);
    put_string(105,27, "rms", SMALL, &vb );

    put_string(3,18, "FREQUENCY:", SMALL, &vb );

}