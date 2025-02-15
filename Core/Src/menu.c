#include "menu.h"
#include "icons.h"
#include "tab_template.h"
#include "stdio.h"
#include "stm32_ST7920_spi.h"
#include "cycle.h"
#include "flash_writer.h"

extern data_container current_temp_chart;
extern data_container target_temp_chart;

extern video_buffer vb;
extern hilv_time hl_t;
extern float target_temp;
extern float current_temp;
extern uint8_t second_elapsed;
extern uint8_t lcd_refresh_elapsed;
extern Menu menu;
extern Cycle operation_cycle;
extern float Kp;
extern float Ki;
extern float Kd;
extern uint8_t ten_sec_counter;
extern pid_controller temp_pid;
extern reflow_profile temp_curves[3];

extern int reflow_profile_index;

char profile_name_str[12] = {'N','O','N','E',0,0,0,0,0,0,0,0};


/* ===============================SERVICE FUNCTIONS     DO NOT DELETE====================================== */

void float_to_char(float num, char *output, uint8_t output_size) {
    if (output == NULL || output_size < 6) {
        if (output != NULL && output_size > 0)
            output[0] = '\0'; 
        return;
    }

    uint16_t int_part = (uint16_t)num;

    uint8_t dec_part = (uint8_t)((num - int_part) * 10);

    if (dec_part < 0) 
      dec_part = -dec_part;

    snprintf(output, output_size, "%d.%d\n", int_part, dec_part);
    
    if( int_part < 100 ) {          
      for (int i = output_size - 1; i > 0; i--) {
        output[i] = output[i - 1];
      }
      output[0] = 32; //spazio
    }
}

uint8_t get_hundreds(float n){
    return (uint8_t)n / 100;
}

uint8_t get_tens(float n){
    return ((uint8_t)n / 10) % 10;
}

uint8_t get_units(float n){
    return (uint8_t)n % 10;
}

uint8_t get_dec(float n){

    uint16_t int_part = (uint16_t)n;
    return (uint8_t)((n - int_part) * 10);
}

void uint_to_char(uint8_t num, char *str) {
    sprintf(str, "%03d", num); 
    
    if(num<100)
        str[0] = ' ';
}

void uint_to_char_2dig(uint8_t num, char *str) {
    if(num>99) {
        num = 0;
    }

    sprintf(str, "%02d", num); 
    
}

int digit_cnt(int number) {
   
   int count = 1;

    if (number == 0)
        return count;

    if(number < 0)
        count++;

    while (number != 0) {
        number /= 10; 
        count++;
    }

    return count;
}

// invert vbc: pixel on→off, off→on
vbc_file invert_vbc_file(const vbc_file* file) {

    static uint8_t inverted_body[MAX_BODY_SIZE];
    vbc_file inverted_file = *file;

    uint16_t body_size = ((file->header.byte_cnt_h << 8) | file->header.byte_cnt_l)-5;

    if (body_size > MAX_BODY_SIZE) {
        return inverted_file; 
    }

    inverted_file.header = file->header;

    for (uint16_t i = 0; i < body_size; i++) {
        inverted_body[i] = ~file->body[i];
    }

    inverted_file.body = inverted_body;

    return inverted_file;
}

void draw_selection_marker(uint8_t selected_item, uint8_t offset){
    
    uint8_t x = menu.tabs_vect[menu.current_tab].items[selected_item].pos_x - offset;
    uint8_t y = menu.tabs_vect[menu.current_tab].items[selected_item].pos_y;

    put_char(x, y, '>', SMALL, &vb);

}

void clear_selection_marker(uint8_t selected_item, uint8_t offset){
    
    uint8_t x = menu.tabs_vect[menu.current_tab].items[selected_item].pos_x - offset;
    uint8_t y = menu.tabs_vect[menu.current_tab].items[selected_item].pos_y;

    clear_sector(x, y, 5, 7, &vb);
}

void display_temperature_profile(reflow_profile profile, data_container *d){

    d->xy_entries.n_ent = 5;

    static float y_entries[5];
    static float x_entries[5];

    x_entries[0] = 0;
    x_entries[1] = profile.preheat_time;
    x_entries[2] = profile.soak_time;
    x_entries[3] = profile.reflow_time;
    x_entries[4] = 280-profile.preheat_time-profile.soak_time-profile.reflow_time;

    y_entries[0] = 0;
    y_entries[1] = profile.preheat_temp;
    y_entries[2] = profile.soak_temp;
    y_entries[3] = profile.peak_temp;
    y_entries[4] = 0;

    d->xy_entries.x = x_entries;
    d->xy_entries.y = y_entries;

    update_chart(0, &target_temp_chart, &vb);


    //update_display(&vb);
}

/* ========================================icons selection function declaration=============================== */

void profile_name_selected();
void chart_selected();
void status_selected();
void play_selected();
void tab_settings_selected();
void time_selected();
void target_temp_selected();
void current_temp_selected();

void profile_selection_selected();
void edit_selected();
void edit_profile_selected();

void preheat_temp_selected();
void preheat_time_selected();
void soak_temp_selected();
void soak_time_selected();
void peak_temp_selected();
void reflow_time_selected();

void target_temp_static_mode_selected();

void kp_param_selected();
void ki_param_selected();
void kd_param_selected();

void tab_home_selected();
void tab_pid_selected();
void tab_curves_selected();
void tab_static_selected();
void none();

/* ================================================================================================================ */

void menu_init() {

    icon_settings_clear_init();
    icon_empty_16x16_init();
    icon_play_clear_init();
    icon_keep_init();
    icon_cooling_init();
    icon_heating_init();
    icon_not_heating_init();
    icon_pid_init();
    icon_curves_init();
    icon_monitor_small_init();
    icon_settings_inv_init();
    icon_play_inv_init();
    icon_monitor_small_inv_init();
    icon_static_mode_init();
    icon_static_mode_inv_init();
    icon_curves_inv_init();
    icon_pid_inv_init();
    icon_home_init();
    icon_home_inv_init();

    /* ==============================================time keeping init================================================= */
    high_lv_timer_init(&hl_t,0,0,0);
    
    /* ===============================================items init======================================================= */

    //main menu 
   {

    menu.tabs_vect[TAB_MAIN].items[PROFILE_NAME].type = TYPE_STR;
    menu.tabs_vect[TAB_MAIN].items[PROFILE_NAME].update = true;
    menu.tabs_vect[TAB_MAIN].items[PROFILE_NAME].pos_x = 3;
    menu.tabs_vect[TAB_MAIN].items[PROFILE_NAME].pos_y = 55;
    menu.tabs_vect[TAB_MAIN].items[PROFILE_NAME].item_data.str_val = profile_name_str;    //main_tab-profile name
    menu.tabs_vect[TAB_MAIN].items[PROFILE_NAME].select_function = profile_name_selected;
 
    menu.tabs_vect[TAB_MAIN].items[TEMP_CHART].type = TYPE_GRAPH;
    menu.tabs_vect[TAB_MAIN].items[TEMP_CHART].update = true;
    menu.tabs_vect[TAB_MAIN].items[TEMP_CHART].pos_x = 0;
    menu.tabs_vect[TAB_MAIN].items[TEMP_CHART].pos_y = 0;
    menu.tabs_vect[TAB_MAIN].items[TEMP_CHART].item_data.chart = &current_temp_chart;    
    menu.tabs_vect[TAB_MAIN].items[TEMP_CHART].select_function = chart_selected;

    menu.tabs_vect[TAB_MAIN].items[STATUS].type = TYPE_ICON;
    menu.tabs_vect[TAB_MAIN].items[STATUS].update = true;
    menu.tabs_vect[TAB_MAIN].items[STATUS].pos_x = 112;
    menu.tabs_vect[TAB_MAIN].items[STATUS].pos_y = 48;
    menu.tabs_vect[TAB_MAIN].items[STATUS].item_data.icon = NULL;    
    menu.tabs_vect[TAB_MAIN].items[STATUS].select_function = status_selected;

    menu.tabs_vect[TAB_MAIN].items[PLAY].type = TYPE_ICON;
    menu.tabs_vect[TAB_MAIN].items[PLAY].update = true;
    menu.tabs_vect[TAB_MAIN].items[PLAY].pos_x = 112;
    menu.tabs_vect[TAB_MAIN].items[PLAY].pos_y = 25;
    menu.tabs_vect[TAB_MAIN].items[PLAY].item_data.icon = &play_inv;    
    menu.tabs_vect[TAB_MAIN].items[PLAY].select_function = play_selected;

    menu.tabs_vect[TAB_MAIN].items[SETTINGS].type = TYPE_ICON;
    menu.tabs_vect[TAB_MAIN].items[SETTINGS].update = true;
    menu.tabs_vect[TAB_MAIN].items[SETTINGS].pos_x = 112;
    menu.tabs_vect[TAB_MAIN].items[SETTINGS].pos_y = 5;
    menu.tabs_vect[TAB_MAIN].items[SETTINGS].item_data.icon = &settings_clear;    
    menu.tabs_vect[TAB_MAIN].items[SETTINGS].select_function = tab_settings_selected;

    menu.tabs_vect[TAB_MAIN].items[TIME].type = TYPE_TIME;  //main_tab-time
    menu.tabs_vect[TAB_MAIN].items[TIME].update = true;
    menu.tabs_vect[TAB_MAIN].items[TIME].pos_x = 3;
    menu.tabs_vect[TAB_MAIN].items[TIME].pos_y = 2;
    menu.tabs_vect[TAB_MAIN].items[TIME].item_data.hl_time = &hl_t;    
    menu.tabs_vect[TAB_MAIN].items[TIME].select_function = time_selected;

    menu.tabs_vect[TAB_MAIN].items[TARGET_TEMP].type = TYPE_FLOAT;  //main_tab-target temp
    menu.tabs_vect[TAB_MAIN].items[TARGET_TEMP].update = true;
    menu.tabs_vect[TAB_MAIN].items[TARGET_TEMP].pos_x = 43;
    menu.tabs_vect[TAB_MAIN].items[TARGET_TEMP].pos_y = 2;
    menu.tabs_vect[TAB_MAIN].items[TARGET_TEMP].item_data.float_val = &target_temp;   
    menu.tabs_vect[TAB_MAIN].items[TARGET_TEMP].select_function = target_temp_selected;

    menu.tabs_vect[TAB_MAIN].items[CURRENT_TEMP].type = TYPE_FLOAT;  //main_tab-current temp
    menu.tabs_vect[TAB_MAIN].items[CURRENT_TEMP].update = true;
    menu.tabs_vect[TAB_MAIN].items[CURRENT_TEMP].pos_x = 80;
    menu.tabs_vect[TAB_MAIN].items[CURRENT_TEMP].pos_y = 2;
    menu.tabs_vect[TAB_MAIN].items[CURRENT_TEMP].item_data.float_val = &current_temp;   
    menu.tabs_vect[TAB_MAIN].items[CURRENT_TEMP].select_function = current_temp_selected;

    }

    //settings
   
    {

    menu.tabs_vect[TAB_SETTINGS].items[PID].type = TYPE_ICON;
    menu.tabs_vect[TAB_SETTINGS].items[PID].update = true;
    menu.tabs_vect[TAB_SETTINGS].items[PID].pos_x = 12;
    menu.tabs_vect[TAB_SETTINGS].items[PID].pos_y = 25;    
    menu.tabs_vect[TAB_SETTINGS].items[PID].item_data.icon = &pid_inv;
    menu.tabs_vect[TAB_SETTINGS].items[PID].select_function = tab_pid_selected;

    menu.tabs_vect[TAB_SETTINGS].items[CURVES].type = TYPE_ICON;
    menu.tabs_vect[TAB_SETTINGS].items[CURVES].update = true;
    menu.tabs_vect[TAB_SETTINGS].items[CURVES].pos_x = 50;
    menu.tabs_vect[TAB_SETTINGS].items[CURVES].pos_y = 25;    
    menu.tabs_vect[TAB_SETTINGS].items[CURVES].item_data.icon = &curves;
    menu.tabs_vect[TAB_SETTINGS].items[CURVES].select_function = tab_curves_selected;

    menu.tabs_vect[TAB_SETTINGS].items[STATIC].type = TYPE_ICON;
    menu.tabs_vect[TAB_SETTINGS].items[STATIC].update = true;
    menu.tabs_vect[TAB_SETTINGS].items[STATIC].pos_x = 90;
    menu.tabs_vect[TAB_SETTINGS].items[STATIC].pos_y = 25;
    menu.tabs_vect[TAB_SETTINGS].items[STATIC].item_data.icon = &static_mode;
    menu.tabs_vect[TAB_SETTINGS].items[STATIC].select_function = tab_static_selected;

    menu.tabs_vect[TAB_SETTINGS].items[BACK_S].type = TYPE_ICON;    
    menu.tabs_vect[TAB_SETTINGS].items[BACK_S].update = true; 
    menu.tabs_vect[TAB_SETTINGS].items[BACK_S].pos_x = 0;
    menu.tabs_vect[TAB_SETTINGS].items[BACK_S].pos_y = 0;       
    menu.tabs_vect[TAB_SETTINGS].items[BACK_S].item_data.icon =  &home;
    menu.tabs_vect[TAB_SETTINGS].items[BACK_S].select_function = tab_home_selected;

    }

    //static temp

    {

    menu.tabs_vect[TAB_STATIC_SELECT].items[SET_TEMP].type = TYPE_FLOAT;
    menu.tabs_vect[TAB_STATIC_SELECT].items[SET_TEMP].update = true;
    menu.tabs_vect[TAB_STATIC_SELECT].items[SET_TEMP].pos_x = 47;
    menu.tabs_vect[TAB_STATIC_SELECT].items[SET_TEMP].pos_y = 30;
    menu.tabs_vect[TAB_STATIC_SELECT].items[SET_TEMP].item_data.float_val = &target_temp;
    menu.tabs_vect[TAB_STATIC_SELECT].items[SET_TEMP].select_function = target_temp_static_mode_selected;

    menu.tabs_vect[TAB_STATIC_SELECT].items[BACK_ST].type = TYPE_ICON;
    menu.tabs_vect[TAB_STATIC_SELECT].items[BACK_ST].update = true; 
    menu.tabs_vect[TAB_STATIC_SELECT].items[BACK_ST].pos_x = 0;
    menu.tabs_vect[TAB_STATIC_SELECT].items[BACK_ST].pos_y = 0;   
    menu.tabs_vect[TAB_STATIC_SELECT].items[BACK_ST].item_data.icon = &home;
    menu.tabs_vect[TAB_STATIC_SELECT].items[BACK_ST].select_function = tab_home_selected;

    }

    //PID param 

    {

    menu.tabs_vect[TAB_PID].items[KP_PARAM].type = TYPE_FLOAT;
    menu.tabs_vect[TAB_PID].items[KP_PARAM].update = true;
    menu.tabs_vect[TAB_PID].items[KP_PARAM].pos_x = 28;
    menu.tabs_vect[TAB_PID].items[KP_PARAM].pos_y = 42;
    menu.tabs_vect[TAB_PID].items[KP_PARAM].item_data.float_val = &Kp;
    menu.tabs_vect[TAB_PID].items[KP_PARAM].select_function = kp_param_selected;

    menu.tabs_vect[TAB_PID].items[KI_PARAM].type = TYPE_FLOAT;
    menu.tabs_vect[TAB_PID].items[KI_PARAM].update = true;
    menu.tabs_vect[TAB_PID].items[KI_PARAM].pos_x = 28;
    menu.tabs_vect[TAB_PID].items[KI_PARAM].pos_y = 32;
    menu.tabs_vect[TAB_PID].items[KI_PARAM].item_data.float_val = &Ki;
    menu.tabs_vect[TAB_PID].items[KI_PARAM].select_function = ki_param_selected;

    menu.tabs_vect[TAB_PID].items[KD_PARAM].type = TYPE_FLOAT;
    menu.tabs_vect[TAB_PID].items[KD_PARAM].update = true;
    menu.tabs_vect[TAB_PID].items[KD_PARAM].pos_x = 28;
    menu.tabs_vect[TAB_PID].items[KD_PARAM].pos_y = 22;
    menu.tabs_vect[TAB_PID].items[KD_PARAM].item_data.float_val = &Kd;
    menu.tabs_vect[TAB_PID].items[KD_PARAM].select_function = kd_param_selected;

    menu.tabs_vect[TAB_PID].items[BACK_P].type = TYPE_ICON;    
    menu.tabs_vect[TAB_PID].items[BACK_P].update = true; 
    menu.tabs_vect[TAB_PID].items[BACK_P].pos_x = 0;
    menu.tabs_vect[TAB_PID].items[BACK_P].pos_y = 0;       
    menu.tabs_vect[TAB_PID].items[BACK_P].item_data.icon =  &home;
    menu.tabs_vect[TAB_PID].items[BACK_P].select_function = tab_home_selected;

    }

    //Curves

    {

    reflow_profile_index = 1;

    menu.tabs_vect[TAB_CURVE].items[SELECT].type = TYPE_INT;
    menu.tabs_vect[TAB_CURVE].items[SELECT].update = true;
    menu.tabs_vect[TAB_CURVE].items[SELECT].pos_x = 100;
    menu.tabs_vect[TAB_CURVE].items[SELECT].pos_y = 42;
    menu.tabs_vect[TAB_CURVE].items[SELECT].item_data.int_val = &reflow_profile_index;
    menu.tabs_vect[TAB_CURVE].items[SELECT].select_function = profile_selection_selected;

    menu.tabs_vect[TAB_CURVE].items[EDIT].type = TYPE_NONE;
    menu.tabs_vect[TAB_CURVE].items[EDIT].update = true;
    menu.tabs_vect[TAB_CURVE].items[EDIT].pos_x = 100;
    menu.tabs_vect[TAB_CURVE].items[EDIT].pos_y = 32;
    menu.tabs_vect[TAB_CURVE].items[EDIT].item_data.int_val = &reflow_profile_index;
    menu.tabs_vect[TAB_CURVE].items[EDIT].select_function = edit_selected;

    menu.tabs_vect[TAB_CURVE].items[BACK_C].type = TYPE_ICON;    
    menu.tabs_vect[TAB_CURVE].items[BACK_C].update = true; 
    menu.tabs_vect[TAB_CURVE].items[BACK_C].pos_x = 0;
    menu.tabs_vect[TAB_CURVE].items[BACK_C].pos_y = 0;       
    menu.tabs_vect[TAB_CURVE].items[BACK_C].item_data.icon =  &home;
    menu.tabs_vect[TAB_CURVE].items[BACK_C].select_function = tab_home_selected;

    }

    //Curve select

    {

    menu.tabs_vect[TAB_SELECT_CURVE].items[PROFILE_1].type = TYPE_NONE;
    menu.tabs_vect[TAB_SELECT_CURVE].items[PROFILE_1].update = true;
    menu.tabs_vect[TAB_SELECT_CURVE].items[PROFILE_1].pos_x = 28;
    menu.tabs_vect[TAB_SELECT_CURVE].items[PROFILE_1].pos_y = 42;
    menu.tabs_vect[TAB_SELECT_CURVE].items[PROFILE_1].item_data.float_val = NULL;
    menu.tabs_vect[TAB_SELECT_CURVE].items[PROFILE_1].select_function = edit_profile_selected;

    menu.tabs_vect[TAB_SELECT_CURVE].items[PROFILE_2].type = TYPE_NONE;
    menu.tabs_vect[TAB_SELECT_CURVE].items[PROFILE_2].update = true;
    menu.tabs_vect[TAB_SELECT_CURVE].items[PROFILE_2].pos_x = 28;
    menu.tabs_vect[TAB_SELECT_CURVE].items[PROFILE_2].pos_y = 32;
    menu.tabs_vect[TAB_SELECT_CURVE].items[PROFILE_2].item_data.float_val = NULL;
    menu.tabs_vect[TAB_SELECT_CURVE].items[PROFILE_2].select_function = edit_profile_selected;

    menu.tabs_vect[TAB_SELECT_CURVE].items[PROFILE_3].type = TYPE_NONE;
    menu.tabs_vect[TAB_SELECT_CURVE].items[PROFILE_3].update = true;
    menu.tabs_vect[TAB_SELECT_CURVE].items[PROFILE_3].pos_x = 28;
    menu.tabs_vect[TAB_SELECT_CURVE].items[PROFILE_3].pos_y = 22;
    menu.tabs_vect[TAB_SELECT_CURVE].items[PROFILE_3].item_data.float_val = NULL;
    menu.tabs_vect[TAB_SELECT_CURVE].items[PROFILE_3].select_function = edit_profile_selected;

    menu.tabs_vect[TAB_SELECT_CURVE].items[BACK_CS].type = TYPE_ICON;    
    menu.tabs_vect[TAB_SELECT_CURVE].items[BACK_CS].update = true; 
    menu.tabs_vect[TAB_SELECT_CURVE].items[BACK_CS].pos_x = 0;
    menu.tabs_vect[TAB_SELECT_CURVE].items[BACK_CS].pos_y = 0;       
    menu.tabs_vect[TAB_SELECT_CURVE].items[BACK_CS].item_data.icon =  &home;
    menu.tabs_vect[TAB_SELECT_CURVE].items[BACK_CS].select_function = edit_profile_selected;       

    }

    //Curve parameters

    {

    menu.tabs_vect[TAB_CURVE_PARAM].items[PREHEAT_TEMP].type = TYPE_FLOAT;
    menu.tabs_vect[TAB_CURVE_PARAM].items[PREHEAT_TEMP].update = true;
    menu.tabs_vect[TAB_CURVE_PARAM].items[PREHEAT_TEMP].pos_x = 88;
    menu.tabs_vect[TAB_CURVE_PARAM].items[PREHEAT_TEMP].pos_y = 55;
    menu.tabs_vect[TAB_CURVE_PARAM].items[PREHEAT_TEMP].item_data.float_val = &temp_curves[0].preheat_temp;
    menu.tabs_vect[TAB_CURVE_PARAM].items[PREHEAT_TEMP].select_function = preheat_temp_selected;

    menu.tabs_vect[TAB_CURVE_PARAM].items[PREHEAT_TIME].type = TYPE_FLOAT;
    menu.tabs_vect[TAB_CURVE_PARAM].items[PREHEAT_TIME].update = true;
    menu.tabs_vect[TAB_CURVE_PARAM].items[PREHEAT_TIME].pos_x = 88;
    menu.tabs_vect[TAB_CURVE_PARAM].items[PREHEAT_TIME].pos_y = 45;
    menu.tabs_vect[TAB_CURVE_PARAM].items[PREHEAT_TIME].item_data.float_val = &temp_curves[0].preheat_time;
    menu.tabs_vect[TAB_CURVE_PARAM].items[PREHEAT_TIME].select_function = preheat_time_selected;

    menu.tabs_vect[TAB_CURVE_PARAM].items[SOAK_TEMP].type = TYPE_FLOAT;
    menu.tabs_vect[TAB_CURVE_PARAM].items[SOAK_TEMP].update = true;
    menu.tabs_vect[TAB_CURVE_PARAM].items[SOAK_TEMP].pos_x = 88;
    menu.tabs_vect[TAB_CURVE_PARAM].items[SOAK_TEMP].pos_y = 35;
    menu.tabs_vect[TAB_CURVE_PARAM].items[SOAK_TEMP].item_data.float_val = &temp_curves[0].soak_temp;
    menu.tabs_vect[TAB_CURVE_PARAM].items[SOAK_TEMP].select_function = soak_temp_selected;

    menu.tabs_vect[TAB_CURVE_PARAM].items[SOAK_TIME].type = TYPE_FLOAT;
    menu.tabs_vect[TAB_CURVE_PARAM].items[SOAK_TIME].update = true;
    menu.tabs_vect[TAB_CURVE_PARAM].items[SOAK_TIME].pos_x = 88;
    menu.tabs_vect[TAB_CURVE_PARAM].items[SOAK_TIME].pos_y = 25;
    menu.tabs_vect[TAB_CURVE_PARAM].items[SOAK_TIME].item_data.float_val = &temp_curves[0].soak_time;
    menu.tabs_vect[TAB_CURVE_PARAM].items[SOAK_TIME].select_function = soak_time_selected;

    menu.tabs_vect[TAB_CURVE_PARAM].items[PEAK_TEMP].type = TYPE_FLOAT;
    menu.tabs_vect[TAB_CURVE_PARAM].items[PEAK_TEMP].update = true;
    menu.tabs_vect[TAB_CURVE_PARAM].items[PEAK_TEMP].pos_x = 88;
    menu.tabs_vect[TAB_CURVE_PARAM].items[PEAK_TEMP].pos_y = 15;
    menu.tabs_vect[TAB_CURVE_PARAM].items[PEAK_TEMP].item_data.float_val = &temp_curves[0].peak_temp;
    menu.tabs_vect[TAB_CURVE_PARAM].items[PEAK_TEMP].select_function = peak_temp_selected;

    menu.tabs_vect[TAB_CURVE_PARAM].items[REFLOW_TIME].type = TYPE_FLOAT;
    menu.tabs_vect[TAB_CURVE_PARAM].items[REFLOW_TIME].update = true;
    menu.tabs_vect[TAB_CURVE_PARAM].items[REFLOW_TIME].pos_x = 93;
    menu.tabs_vect[TAB_CURVE_PARAM].items[REFLOW_TIME].pos_y = 5;
    menu.tabs_vect[TAB_CURVE_PARAM].items[REFLOW_TIME].item_data.float_val = &temp_curves[0].reflow_time;
    menu.tabs_vect[TAB_CURVE_PARAM].items[REFLOW_TIME].select_function = reflow_time_selected;

    menu.tabs_vect[TAB_CURVE_PARAM].items[BACK_CP].type = TYPE_ICON;    
    menu.tabs_vect[TAB_CURVE_PARAM].items[BACK_CP].update = true; 
    menu.tabs_vect[TAB_CURVE_PARAM].items[BACK_CP].pos_x = 0;
    menu.tabs_vect[TAB_CURVE_PARAM].items[BACK_CP].pos_y = 0;       
    menu.tabs_vect[TAB_CURVE_PARAM].items[BACK_CP].item_data.icon =  &home;
    menu.tabs_vect[TAB_CURVE_PARAM].items[BACK_CP].select_function = tab_home_selected;       

    }


    /* ================================================tab init======================================================== */

    menu.tabs_vect[TAB_MAIN].template_draw_fnt = main_tab_template;
    menu.tabs_vect[TAB_MAIN].items_cnt = MAIN_MENU_ITEM_COUNT;
    menu.tabs_vect[TAB_MAIN].selected_item = PLAY;
    menu.tabs_vect[TAB_MAIN].tab_id = TAB_MAIN;

    menu.tabs_vect[TAB_SETTINGS].template_draw_fnt = settings_tab_template;
    menu.tabs_vect[TAB_SETTINGS].items_cnt = SETTINGS_MENU_ITEM_COUNT;
    menu.tabs_vect[TAB_SETTINGS].selected_item = PID;
    menu.tabs_vect[TAB_SETTINGS].tab_id = TAB_SETTINGS;

    menu.tabs_vect[TAB_PID].template_draw_fnt = pid_parameter_tab_template;
    menu.tabs_vect[TAB_PID].items_cnt = PID_MENU_ITEM_COUNT;
    menu.tabs_vect[TAB_PID].selected_item = KP_PARAM;
    menu.tabs_vect[TAB_PID].tab_id = TAB_PID;

    menu.tabs_vect[TAB_STATIC_SELECT].template_draw_fnt = static_set_tab_template;
    menu.tabs_vect[TAB_STATIC_SELECT].items_cnt = STATIC_MODE_MENU_ITEM_COUNT;
    menu.tabs_vect[TAB_STATIC_SELECT].selected_item = SET_TEMP;
    menu.tabs_vect[TAB_STATIC_SELECT].tab_id = TAB_STATIC_SELECT;
    
    menu.tabs_vect[TAB_CURVE].template_draw_fnt = curves_tab_template;
    menu.tabs_vect[TAB_CURVE].items_cnt = CURVES_MENU_ITEM_COUNT;
    menu.tabs_vect[TAB_CURVE].selected_item = SELECT;
    menu.tabs_vect[TAB_CURVE].tab_id = TAB_CURVE;

    menu.tabs_vect[TAB_SELECT_CURVE].template_draw_fnt = select_curve_tab_template;
    menu.tabs_vect[TAB_SELECT_CURVE].items_cnt = CURVES_PROFILE_COUNT;
    menu.tabs_vect[TAB_SELECT_CURVE].selected_item = PROFILE_1;
    menu.tabs_vect[TAB_SELECT_CURVE].tab_id = TAB_SELECT_CURVE;

    menu.tabs_vect[TAB_CURVE_PARAM].template_draw_fnt = curve_param_tab_template;
    menu.tabs_vect[TAB_CURVE_PARAM].items_cnt = CURVES_PARAM_COUNT;
    menu.tabs_vect[TAB_CURVE_PARAM].selected_item = PREHEAT_TEMP;
    menu.tabs_vect[TAB_CURVE_PARAM].tab_id = TAB_CURVE_PARAM;

    /* ================================================menu overstructure init========================================= */

    menu.current_tab = TAB_MAIN; //default init tab

}

void handle_input() {

    InputEvent event = get_input_event_db();
    vbc_file inverted;

    switch (menu.tabs_vect[menu.current_tab].tab_id){

        case TAB_MAIN:

            switch (event){

                case INPUT_UP: // da aggiungere la gestione del cambio di icona in modalita regolazione
                  
                    if(menu.tabs_vect[menu.current_tab].selected_item == SETTINGS){
                        
                        menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].update = true;
                        menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].item_data.icon = &settings_clear;

                        menu.tabs_vect[menu.current_tab].selected_item = PLAY;
                        menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].update = true;
                        menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].item_data.icon = &play_inv;
                    }

                break;

                case INPUT_DOWN:
                   
                    if(menu.tabs_vect[menu.current_tab].selected_item == PLAY && operation_cycle.status != CYCLE_RUNNING){
                        
                        menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].update = true;
                        menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].item_data.icon = &play_clear;

                        menu.tabs_vect[menu.current_tab].selected_item = SETTINGS;
                        menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].update = true;
                        menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].item_data.icon = &settings_inv;
                    }
                
                break;
                case INPUT_SELECT:

                        menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].select_function();

                break;

                default:
                break;
            
            }

        break;

        case TAB_SETTINGS:

            switch (event){
                                
                case INPUT_RIGHT: 

                    if(menu.tabs_vect[menu.current_tab].selected_item != BACK_S){

                        if( menu.tabs_vect[menu.current_tab].selected_item < CURVES ){
                            menu.tabs_vect[menu.current_tab].selected_item = PID;
                        }

                        else{

                            menu.tabs_vect[menu.current_tab].items[PID].item_data.icon = &pid; //si potrebbe interagire solo con l'elemento selezionato...no voglia di riscrivere
                            menu.tabs_vect[menu.current_tab].items[PID].update = true;
                            menu.tabs_vect[menu.current_tab].items[CURVES].item_data.icon = &curves;
                            menu.tabs_vect[menu.current_tab].items[CURVES].update = true;
                            menu.tabs_vect[menu.current_tab].items[STATIC].item_data.icon = &static_mode;
                            menu.tabs_vect[menu.current_tab].items[STATIC].update = true;

                            menu.tabs_vect[menu.current_tab].selected_item--;

                            inverted = invert_vbc_file(menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].item_data.icon);
                            
                            menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].item_data.icon = &inverted;
                            menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].update = true;
                        }
                    }
                break;
            
                case INPUT_DOWN:

                    if(menu.tabs_vect[menu.current_tab].selected_item != BACK_S){
                        
                        menu.tabs_vect[menu.current_tab].items[PID].item_data.icon = &pid;
                        menu.tabs_vect[menu.current_tab].items[PID].update = true;
                        menu.tabs_vect[menu.current_tab].items[CURVES].item_data.icon = &curves;
                        menu.tabs_vect[menu.current_tab].items[CURVES].update = true;
                        menu.tabs_vect[menu.current_tab].items[STATIC].item_data.icon = &static_mode;
                        menu.tabs_vect[menu.current_tab].items[STATIC].update = true;

                        menu.tabs_vect[menu.current_tab].selected_item = BACK_S;

                        menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].item_data.icon = &home_inv;
                        menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].update = true;

                    }

                break;

                case INPUT_UP:

                    if(menu.tabs_vect[menu.current_tab].selected_item ==  BACK_S){
                        
                        
                        menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].item_data.icon = &home;
                        menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].update = true;

                        menu.tabs_vect[menu.current_tab].selected_item = PID;
                        menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].item_data.icon = &pid_inv;
                        menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].update = true;

                    } 
               
                break;

                case INPUT_LEFT:

                    if(menu.tabs_vect[menu.current_tab].selected_item != BACK_S){

                        if( menu.tabs_vect[menu.current_tab].selected_item > CURVES ){
                            menu.tabs_vect[menu.current_tab].selected_item = STATIC;
                        }

                        else{

                            menu.tabs_vect[menu.current_tab].items[PID].item_data.icon = &pid;
                            menu.tabs_vect[menu.current_tab].items[PID].update = true;
                            menu.tabs_vect[menu.current_tab].items[CURVES].item_data.icon = &curves;
                            menu.tabs_vect[menu.current_tab].items[CURVES].update = true;
                            menu.tabs_vect[menu.current_tab].items[STATIC].item_data.icon = &static_mode;
                            menu.tabs_vect[menu.current_tab].items[STATIC].update = true;

                            menu.tabs_vect[menu.current_tab].selected_item++;

                            inverted = invert_vbc_file(menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].item_data.icon);
                            
                            menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].item_data.icon = &inverted;
                            menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].update = true;

                        }
                    }
               
                break;
                


                case INPUT_SELECT: 

                    menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].select_function();
        
                break;

                default:
                break;

            }

        break;

        case TAB_STATIC_SELECT:

            switch (event){

                case INPUT_DOWN:

                    menu.tabs_vect[menu.current_tab].selected_item = BACK_ST;
                    menu.tabs_vect[menu.current_tab].items[BACK_ST].item_data.icon = &home_inv;
                    menu.tabs_vect[menu.current_tab].items[BACK_ST].update = true;

                break;

                case INPUT_UP: 

                    menu.tabs_vect[menu.current_tab].items[BACK_ST].item_data.icon = &home;
                    menu.tabs_vect[menu.current_tab].items[BACK_ST].update = true;
                    menu.tabs_vect[menu.current_tab].selected_item = SET_TEMP;

                break;

                case INPUT_SELECT: 

                    menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].select_function();
 
                break;

                default:
                break;

            }

        break;
        
        case TAB_PID:

            draw_selection_marker(menu.tabs_vect[menu.current_tab].selected_item, 28);

            switch (event){

                case INPUT_UP:

                    if(menu.tabs_vect[menu.current_tab].selected_item == BACK_P){
                        menu.tabs_vect[menu.current_tab].items[BACK_P].item_data.icon = &home;
                        menu.tabs_vect[menu.current_tab].items[BACK_P].update = true;
                    }

                    if( menu.tabs_vect[menu.current_tab].selected_item < KI_PARAM ){
                        menu.tabs_vect[menu.current_tab].selected_item = KP_PARAM;
                    }

                    else{

                        clear_selection_marker(menu.tabs_vect[menu.current_tab].selected_item, 28);
                        menu.tabs_vect[menu.current_tab].selected_item--;
                           
                    }

                break;

                case INPUT_DOWN:
                    
                    clear_selection_marker( menu.tabs_vect[menu.current_tab].selected_item, 28);

                    if( menu.tabs_vect[menu.current_tab].selected_item > KI_PARAM ){

                        
                        menu.tabs_vect[menu.current_tab].selected_item = BACK_P;
                        menu.tabs_vect[menu.current_tab].items[BACK_P].item_data.icon = &home_inv;
                        menu.tabs_vect[menu.current_tab].items[BACK_P].update = true;
                    }

                    else{
                        menu.tabs_vect[menu.current_tab].selected_item++;
                           
                    }

                break;

                case INPUT_SELECT:

                    menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].select_function();

                break;

                default:
                break;

            }

        break;

        case TAB_CURVE:

            draw_selection_marker(menu.tabs_vect[menu.current_tab].selected_item, 100);

            switch (event){

                case INPUT_UP:

                    if(menu.tabs_vect[menu.current_tab].selected_item == BACK_C){
                        menu.tabs_vect[menu.current_tab].items[BACK_C].item_data.icon = &home;
                        menu.tabs_vect[menu.current_tab].items[BACK_C].update = true;
                    }

                    if( menu.tabs_vect[menu.current_tab].selected_item < EDIT){
                        menu.tabs_vect[menu.current_tab].selected_item = SELECT;
                    }

                    else{

                        clear_selection_marker(menu.tabs_vect[menu.current_tab].selected_item, 100);
                        menu.tabs_vect[menu.current_tab].selected_item--;
                           
                    }

                break;

                case INPUT_DOWN:
                    
                    clear_selection_marker( menu.tabs_vect[menu.current_tab].selected_item, 100);

                    if( menu.tabs_vect[menu.current_tab].selected_item > SELECT ){

                        menu.tabs_vect[menu.current_tab].selected_item = BACK_C;
                        menu.tabs_vect[menu.current_tab].items[BACK_C].item_data.icon = &home_inv;
                        menu.tabs_vect[menu.current_tab].items[BACK_C].update = true;
                    }

                    else{
                        menu.tabs_vect[menu.current_tab].selected_item++;
                           
                    }

                break;

                case INPUT_SELECT:

                    menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].select_function();

                break;

                default:
                break;

            }

        break;

        case TAB_SELECT_CURVE:

            reflow_profile_index = (menu.tabs_vect[menu.current_tab].selected_item < 3) ? menu.tabs_vect[menu.current_tab].selected_item + 1 : 3;
            draw_selection_marker(menu.tabs_vect[menu.current_tab].selected_item, 28);

            switch (event){

                case INPUT_UP:

                    if(menu.tabs_vect[menu.current_tab].selected_item == BACK_CS){
                        menu.tabs_vect[menu.current_tab].items[BACK_CS].item_data.icon = &home;
                        menu.tabs_vect[menu.current_tab].items[BACK_CS].update = true;
                    }

                    if( menu.tabs_vect[menu.current_tab].selected_item < PROFILE_2 ){
                        menu.tabs_vect[menu.current_tab].selected_item = PROFILE_1;
                    }

                    else{

                        clear_selection_marker(menu.tabs_vect[menu.current_tab].selected_item, 28);
                        menu.tabs_vect[menu.current_tab].selected_item--;
                    }

                break;

                case INPUT_DOWN:
                    
                    clear_selection_marker( menu.tabs_vect[menu.current_tab].selected_item, 28);

                    if( menu.tabs_vect[menu.current_tab].selected_item > PROFILE_2 ){

                        
                        menu.tabs_vect[menu.current_tab].selected_item = BACK_CS;
                        menu.tabs_vect[menu.current_tab].items[BACK_CS].item_data.icon = &home_inv;
                        menu.tabs_vect[menu.current_tab].items[BACK_CS].update = true;
                    }

                    else{

                        menu.tabs_vect[menu.current_tab].selected_item++;      
                    }

                

                break;

                case INPUT_SELECT:

                    menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].select_function();

                break;

                default:
                break;


            }

        break;

        case TAB_CURVE_PARAM:

            uint8_t marker_offset = ( menu.tabs_vect[menu.current_tab].selected_item < SOAK_TIME) ? 88 : 72; //offset varibile compensa le scritte che girano attorno al tasto home

            if(menu.tabs_vect[menu.current_tab].selected_item > PEAK_TEMP)   
                marker_offset = 77;

            draw_selection_marker(menu.tabs_vect[menu.current_tab].selected_item, marker_offset);

            switch (event){

                case INPUT_UP:

                    if(menu.tabs_vect[menu.current_tab].selected_item == BACK_CP){
                        menu.tabs_vect[menu.current_tab].items[BACK_CP].item_data.icon = &home;
                        menu.tabs_vect[menu.current_tab].items[BACK_CP].update = true;
                    }

                    if( menu.tabs_vect[menu.current_tab].selected_item < PREHEAT_TIME ){
                        menu.tabs_vect[menu.current_tab].selected_item = PREHEAT_TEMP;
                    }

                    else{

                        clear_selection_marker(menu.tabs_vect[menu.current_tab].selected_item, marker_offset);
                        menu.tabs_vect[menu.current_tab].selected_item--;

                    }

                break;

                case INPUT_DOWN:
                    
                    clear_selection_marker( menu.tabs_vect[menu.current_tab].selected_item, marker_offset);

                    if( menu.tabs_vect[menu.current_tab].selected_item > PEAK_TEMP ){

                        menu.tabs_vect[menu.current_tab].selected_item = BACK_CP;
                        menu.tabs_vect[menu.current_tab].items[BACK_CP].item_data.icon = &home_inv;
                        menu.tabs_vect[menu.current_tab].items[BACK_CP].update = true;
                    }

                    else{

                        menu.tabs_vect[menu.current_tab].selected_item++;      
                    }

                break;

                case INPUT_SELECT:

                    menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].select_function();

                break;

                default:
                break;
            }


        break;

        default:
        break;

    }

    render_tab(menu.tabs_vect[menu.current_tab]);

}

void render_item(Item *i){ //assumed to use only small font

    if(!i->update)
        return;

    i->update = false;

    switch (i->type){

        case TYPE_INT:
            
            char int_char[4] = {0}; //3 because uint8 
            clear_sector(i->pos_x,i->pos_y,5 * 3 + 1, 7, &vb); //5 small char width, 7 small char height //digit_cnt(i.item_data.int_val)
            uint_to_char_2dig(*i->item_data.int_val, int_char);
            put_string(i->pos_x, i->pos_y, int_char, SMALL, &vb);

            break;

        case TYPE_FLOAT:

            char f_char[6] = {0}; //5 aaa.b/0 no negative number
            //memset(vb.vid_buf,0xff,1024);
            clear_sector(i->pos_x, i->pos_y, 5 * 5 + 3, 7, &vb); //5 small char width, 7 small char height
            float_to_char(*i->item_data.float_val, f_char, 6);
            put_string(i->pos_x, i->pos_y, f_char, SMALL, &vb);

            break;

        case TYPE_STR:
            
            clear_sector(i->pos_x, i->pos_y, 5*(strlen(i->item_data.str_val)+1)-1, 7, &vb); // 7 small char height
            put_string(i->pos_x, i->pos_y, i->item_data.str_val, SMALL, &vb);

            break;

        case TYPE_GRAPH:

            if(operation_cycle.status != CYCLE_STOP && operation_cycle.cycle_type == CYCLE_MODE_CURVE)
                display_temperature_profile(temp_curves[reflow_profile_index-1], &target_temp_chart);

            break;

        case TYPE_ICON:
            
            if(i->item_data.icon !=NULL)
                put_vbc(*i->item_data.icon, i->pos_x, i->pos_y, &vb);

            break;

        case TYPE_TIME:
        
            char time_str[2]; //sting tempo per contenere minuti sec ore            
            
            clear_sector(i->pos_x,i->pos_y,37,8,&vb);
            uint_to_char_2dig(i->item_data.hl_time->hour,time_str);
            put_string(i->pos_x,i->pos_y, time_str, SMALL,&vb);
            put_char(i->pos_x+9,i->pos_y,':',SMALL,&vb);
            uint_to_char_2dig(i->item_data.hl_time->min,time_str);
            put_string(i->pos_x+13,i->pos_y,time_str,SMALL,&vb);
            put_char(i->pos_x+22,i->pos_y,':',SMALL,&vb);
            uint_to_char_2dig(i->item_data.hl_time->sec,time_str);
            put_string(i->pos_x+26,i->pos_y,time_str,SMALL,&vb);

            break;
        
        default:
            break;     


    }
       
}

void render_tab(Tab tab){

    tab.template_draw_fnt();

    for(uint8_t i=0; i<tab.items_cnt; i++){
        render_item(&tab.items[i]);
    }
}

void update_all_items(Tab *t, uint8_t item_cnt){

    for(uint8_t i=0; i < item_cnt; i++){
        t->items[i].update = true;
    }
}

/* ================================================================================================================ */

void profile_name_selected(){
    //leave blank
}

void chart_selected(){
    //leave blank
}

void status_selected(){
    //leave blank
}

void play_selected(){
//start cycle
    temp_pid.integral = 0;
    temp_pid.previous_error = 0;
    
    operation_cycle.status = (operation_cycle.status == CYCLE_RUNNING) ? CYCLE_STOP : CYCLE_RUNNING;
    menu.tabs_vect[menu.current_tab].items[SETTINGS].item_data.icon = (operation_cycle.status == CYCLE_RUNNING) ? &empty_16x16 : &settings_clear;
}

void tab_settings_selected(){

    menu.current_tab = TAB_SETTINGS;
    
    menu.tabs_vect[menu.current_tab].selected_item = PID;
    menu.tabs_vect[menu.current_tab].items[PID].item_data.icon = &pid_inv; //si potrebbe interagire solo con l'elemento selezionato...no voglia di riscrivere
    menu.tabs_vect[menu.current_tab].items[PID].update = true;
    menu.tabs_vect[menu.current_tab].items[CURVES].item_data.icon = &curves;
    menu.tabs_vect[menu.current_tab].items[CURVES].update = true;
    menu.tabs_vect[menu.current_tab].items[STATIC].item_data.icon = &static_mode;
    menu.tabs_vect[menu.current_tab].items[STATIC].update = true;    
    
    update_all_items(&menu.tabs_vect[menu.current_tab], SETTINGS_MENU_ITEM_COUNT);
  
    clr_buffer(&vb);
}

void time_selected(){
    //leave blank
}

void target_temp_selected(){
    //leave blank
}

void current_temp_selected(){
    //leave blank
}

void edit_selected(){

    menu.current_tab = TAB_SELECT_CURVE;
    menu.tabs_vect[menu.current_tab].selected_item = PROFILE_1;

    update_all_items(&menu.tabs_vect[menu.current_tab], SETTINGS_MENU_ITEM_COUNT);
  
    clr_buffer(&vb);

}

void edit_profile_selected(){

    menu.tabs_vect[TAB_CURVE_PARAM].items[PREHEAT_TEMP].item_data.float_val = &temp_curves[reflow_profile_index-1].preheat_temp;
    menu.tabs_vect[TAB_CURVE_PARAM].items[PREHEAT_TIME].item_data.float_val = &temp_curves[reflow_profile_index-1].preheat_time;
    menu.tabs_vect[TAB_CURVE_PARAM].items[SOAK_TEMP].item_data.float_val = &temp_curves[reflow_profile_index-1].soak_temp;
    menu.tabs_vect[TAB_CURVE_PARAM].items[SOAK_TIME].item_data.float_val = &temp_curves[reflow_profile_index-1].soak_time;
    menu.tabs_vect[TAB_CURVE_PARAM].items[PEAK_TEMP].item_data.float_val = &temp_curves[reflow_profile_index-1].peak_temp;
    menu.tabs_vect[TAB_CURVE_PARAM].items[REFLOW_TIME].item_data.float_val = &temp_curves[reflow_profile_index-1].reflow_time;

    menu.current_tab = TAB_CURVE_PARAM;
    menu.tabs_vect[menu.current_tab].selected_item = PREHEAT_TEMP;

    update_all_items(&menu.tabs_vect[menu.current_tab], SETTINGS_MENU_ITEM_COUNT);
  
    clr_buffer(&vb);

}

void kp_param_selected(){

    // la visualizzazione si rompe con numeri negativi per ora ok....da sistemare

    const uint8_t CLEAR_ZONE_WIDTH = 18;

    bool clear = true;
    char f_char[6] = {0}; //5 aaa.b/0 no negative number
    InputEvent event = INPUT_NONE;

    uint8_t clear_index_x = menu.tabs_vect[menu.current_tab].items[KP_PARAM].pos_x;
    uint8_t clear_index_y = menu.tabs_vect[menu.current_tab].items[KP_PARAM].pos_y;
    
    enum digit_index{
        DEC,
        INT
    };

    uint8_t selected_digit = INT;

    while(event != INPUT_SELECT){
    
        event = get_input_event_db();

        switch(event){

            case INPUT_UP:
                    switch (selected_digit){

                        case INT:
                            
                            *menu.tabs_vect[menu.current_tab].items[KP_PARAM].item_data.float_val += 1.0f;

                        break;    

                        case DEC:
                            
                            *menu.tabs_vect[menu.current_tab].items[KP_PARAM].item_data.float_val += 0.1f;

                        break;    

                        default:
                        break;      

                    }

            break;

            case INPUT_DOWN:

                    switch (selected_digit){

                        case INT:

                            *menu.tabs_vect[menu.current_tab].items[KP_PARAM].item_data.float_val -= 1.0f;

                        break;    

                        case DEC:
                            
                            *menu.tabs_vect[menu.current_tab].items[KP_PARAM].item_data.float_val -= 0.1f;

                        break;    
    
                        default:
                        break;      

                    }

            break;

            case INPUT_LEFT:
                
                selected_digit = DEC;
            
            break;

            case INPUT_RIGHT:

                selected_digit = INT;

            break;

            default:
            break;

        }

        if(clear){
            if(selected_digit == DEC)
                clear_sector(clear_index_x + CLEAR_ZONE_WIDTH ,clear_index_y, CLEAR_ZONE_WIDTH, 7, &vb);    
            else
                clear_sector(clear_index_x + ((selected_digit - 1) * CLEAR_ZONE_WIDTH) ,clear_index_y, CLEAR_ZONE_WIDTH, 7, &vb);
        }

        else{
            clear_sector(menu.tabs_vect[menu.current_tab].items[KP_PARAM].pos_x,menu.tabs_vect[menu.current_tab].items[KP_PARAM].pos_y, 30, 7, &vb);
            float_to_char(*menu.tabs_vect[menu.current_tab].items[KP_PARAM].item_data.float_val, f_char, 6);
            put_string(menu.tabs_vect[menu.current_tab].items[KP_PARAM].pos_x,menu.tabs_vect[menu.current_tab].items[KP_PARAM].pos_y, f_char, SMALL, &vb);

        }

        if(lcd_refresh_elapsed) { //every half second
            clear = !clear;
            update_display(&vb);
            lcd_refresh_elapsed = 0 ;
        }

    }

    flash_write_float(PID_FLASH_INITIAL_ADDRESS, *menu.tabs_vect[menu.current_tab].items[KP_PARAM].item_data.float_val, 0);
}

void ki_param_selected(){
    // la visualizzazione si rompe con numeri negativi per ora ok....da sistemare

    const uint8_t CLEAR_ZONE_WIDTH = 18;

    bool clear = true;
    char f_char[6] = {0}; //5 aaa.b/0 no negative number
    InputEvent event = INPUT_NONE;

    uint8_t clear_index_x = menu.tabs_vect[menu.current_tab].items[KI_PARAM].pos_x;
    uint8_t clear_index_y = menu.tabs_vect[menu.current_tab].items[KI_PARAM].pos_y;
    
    enum digit_index{
        DEC,
        INT
    };

    uint8_t selected_digit = INT;

    while(event != INPUT_SELECT){
    
        event = get_input_event_db();

        switch(event){

            case INPUT_UP:
                    switch (selected_digit){

                        case INT:
                            
                            *menu.tabs_vect[menu.current_tab].items[KI_PARAM].item_data.float_val += 1.0f;

                        break;    

                        case DEC:
                            
                            *menu.tabs_vect[menu.current_tab].items[KI_PARAM].item_data.float_val += 0.1f;

                        break;    

                        default:
                        break;      

                    }

            break;

            case INPUT_DOWN:

                    switch (selected_digit){

                        case INT:

                            *menu.tabs_vect[menu.current_tab].items[KI_PARAM].item_data.float_val -= 1.0f;

                        break;    

                        case DEC:
                            
                            *menu.tabs_vect[menu.current_tab].items[KI_PARAM].item_data.float_val -= 0.1f;

                        break;    
    
                        default:
                        break;      

                    }

            break;

            case INPUT_LEFT:
                
                selected_digit = DEC;
            
            break;

            case INPUT_RIGHT:

                selected_digit = INT;

            break;

            default:
            break;

        }

        if(clear){
            if(selected_digit == DEC)
                clear_sector(clear_index_x + CLEAR_ZONE_WIDTH ,clear_index_y, CLEAR_ZONE_WIDTH, 7, &vb);    
            else
                clear_sector(clear_index_x + ((selected_digit - 1) * CLEAR_ZONE_WIDTH) ,clear_index_y, CLEAR_ZONE_WIDTH, 7, &vb);
        }

        else{
            clear_sector(menu.tabs_vect[menu.current_tab].items[KI_PARAM].pos_x,menu.tabs_vect[menu.current_tab].items[KI_PARAM].pos_y, 30, 7, &vb);
            float_to_char(*menu.tabs_vect[menu.current_tab].items[KI_PARAM].item_data.float_val, f_char, 6);
            put_string(menu.tabs_vect[menu.current_tab].items[KI_PARAM].pos_x,menu.tabs_vect[menu.current_tab].items[KI_PARAM].pos_y, f_char, SMALL, &vb);

        }

        if(lcd_refresh_elapsed) { //every half second
            clear = !clear;
            update_display(&vb);
            lcd_refresh_elapsed = 0 ;
        }

    }

    flash_write_float(PID_FLASH_INITIAL_ADDRESS, *menu.tabs_vect[menu.current_tab].items[KI_PARAM].item_data.float_val, 1);
}

void kd_param_selected(){
// la visualizzazione si rompe con numeri negativi per ora ok....da sistemare

    const uint8_t CLEAR_ZONE_WIDTH = 18;

    bool clear = true;
    char f_char[6] = {0}; //5 aaa.b/0 no negative number
    InputEvent event = INPUT_NONE;

    uint8_t clear_index_x = menu.tabs_vect[menu.current_tab].items[KD_PARAM].pos_x;
    uint8_t clear_index_y = menu.tabs_vect[menu.current_tab].items[KD_PARAM].pos_y;
    
    enum digit_index{
        DEC,
        INT
    };

    uint8_t selected_digit = INT;

    while(event != INPUT_SELECT){
    
        event = get_input_event_db();

        switch(event){

            case INPUT_UP:
                    switch (selected_digit){

                        case INT:
                            
                            *menu.tabs_vect[menu.current_tab].items[KD_PARAM].item_data.float_val += 1.0f;

                        break;    

                        case DEC:
                            
                            *menu.tabs_vect[menu.current_tab].items[KD_PARAM].item_data.float_val += 0.1f;

                        break;    

                        default:
                        break;      

                    }

            break;

            case INPUT_DOWN:

                    switch (selected_digit){

                        case INT:

                            *menu.tabs_vect[menu.current_tab].items[KD_PARAM].item_data.float_val -= 1.0f;

                        break;    

                        case DEC:
                            
                            *menu.tabs_vect[menu.current_tab].items[KD_PARAM].item_data.float_val -= 0.1f;

                        break;    
    
                        default:
                        break;      

                    }

            break;

            case INPUT_LEFT:
                
                selected_digit = DEC;
            
            break;

            case INPUT_RIGHT:

                selected_digit = INT;

            break;

            default:
            break;

        }

        if(clear){
            if(selected_digit == DEC)
                clear_sector(clear_index_x + CLEAR_ZONE_WIDTH ,clear_index_y, CLEAR_ZONE_WIDTH, 7, &vb);    
            else
                clear_sector(clear_index_x + ((selected_digit - 1) * CLEAR_ZONE_WIDTH) ,clear_index_y, CLEAR_ZONE_WIDTH, 7, &vb);
        }

        else{
            clear_sector(menu.tabs_vect[menu.current_tab].items[KD_PARAM].pos_x,menu.tabs_vect[menu.current_tab].items[KD_PARAM].pos_y, 30, 7, &vb);
            float_to_char(*menu.tabs_vect[menu.current_tab].items[KD_PARAM].item_data.float_val, f_char, 6);
            put_string(menu.tabs_vect[menu.current_tab].items[KD_PARAM].pos_x,menu.tabs_vect[menu.current_tab].items[KD_PARAM].pos_y, f_char, SMALL, &vb);

        }

        if(lcd_refresh_elapsed) { //every half second
            clear = !clear;
            update_display(&vb);
            lcd_refresh_elapsed = 0 ;
        }

    }

    flash_write_float(PID_FLASH_INITIAL_ADDRESS, *menu.tabs_vect[menu.current_tab].items[KD_PARAM].item_data.float_val, 2);
}

void tab_home_selected(){

    menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].item_data.icon = &home;
    menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].update = true;

    target_temp = 0.0f;
    hl_t.seconds_counter = 0;
    hl_t.hour = 0;
    hl_t.min = 0;
    hl_t.sec = 0;

    menu.current_tab = TAB_MAIN;
    menu.tabs_vect[menu.current_tab].selected_item = PLAY;
    menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].item_data.icon = &play_inv;
    menu.tabs_vect[menu.current_tab].items[SETTINGS].item_data.icon = &settings_clear;

    update_all_items(&menu.tabs_vect[menu.current_tab], MAIN_MENU_ITEM_COUNT);
    clr_buffer(&vb);
}

void tab_pid_selected(){

    menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].item_data.icon = &home;
    menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].update = true;

    menu.current_tab = TAB_PID;
    menu.tabs_vect[menu.current_tab].selected_item = KP_PARAM;

    update_all_items(&menu.tabs_vect[menu.current_tab], STATIC_MODE_MENU_ITEM_COUNT);
    clr_buffer(&vb);

}

void tab_curves_selected(){

    menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].item_data.icon = &home;
    menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].update = true;

    menu.current_tab = TAB_CURVE;
    menu.tabs_vect[menu.current_tab].selected_item = SELECT;

    update_all_items(&menu.tabs_vect[menu.current_tab], CURVES_MENU_ITEM_COUNT);
    clr_buffer(&vb);

}

void profile_selection_selected(){

    const uint8_t CLEAR_ZONE_WIDTH = 18;

    InputEvent event = INPUT_NONE;
    bool clear = true;
    char int_char[3] = {0}; 

    uint8_t clear_index_x = menu.tabs_vect[menu.current_tab].items[SELECT].pos_x;
    uint8_t clear_index_y = menu.tabs_vect[menu.current_tab].items[SELECT].pos_y;

    //clear home icon: once you entered the profile selection, 
    //the only way out is the main tab with the current mode of operation set to curve mode
    clear_sector(0,0,16,16, &vb);  

    while(event != INPUT_SELECT){

        event = get_input_event_db();

        switch(event){

            case INPUT_UP:
                            
                *menu.tabs_vect[TAB_CURVE].items[SELECT].item_data.int_val = (*menu.tabs_vect[TAB_CURVE].items[SELECT].item_data.int_val < 3) ?  *menu.tabs_vect[TAB_CURVE].items[SELECT].item_data.int_val += 1 : 3; 
                
            break;

            case INPUT_DOWN:

                *menu.tabs_vect[TAB_CURVE].items[SELECT].item_data.int_val = (*menu.tabs_vect[TAB_CURVE].items[SELECT].item_data.int_val > 1) ?  *menu.tabs_vect[TAB_CURVE].items[SELECT].item_data.int_val -= 1 : 1; 

            break;


            default:
            break;

        }

        if(clear){
           
                clear_sector(clear_index_x,clear_index_y, CLEAR_ZONE_WIDTH, 7, &vb);    
        }

        else{

            clear_sector(clear_index_x,clear_index_y, CLEAR_ZONE_WIDTH, 7, &vb);
            uint_to_char_2dig(*menu.tabs_vect[menu.current_tab].items[SELECT].item_data.int_val, int_char);
            put_string(menu.tabs_vect[menu.current_tab].items[SELECT].pos_x, menu.tabs_vect[menu.current_tab].items[SELECT].pos_y, int_char, SMALL, &vb);

        }

        if(lcd_refresh_elapsed) { //every half second
            clear = !clear;
            update_display(&vb);
            lcd_refresh_elapsed = 0 ;
        }
    }
 
    sprintf(profile_name_str, "%s%s", "PROFILE ", int_char);

    clr_buffer(&vb);
    menu.current_tab = TAB_MAIN;
    menu.tabs_vect[menu.current_tab].items[PROFILE_NAME].update = true;
    menu.tabs_vect[menu.current_tab].selected_item = PLAY;
    menu.tabs_vect[menu.current_tab].items[PLAY].item_data.icon = &play_inv;
    menu.tabs_vect[menu.current_tab].items[PLAY].update = true;
    menu.tabs_vect[menu.current_tab].items[SETTINGS].item_data.icon = &settings_clear;
    menu.tabs_vect[menu.current_tab].items[SETTINGS].update = true;
    operation_cycle.cycle_type = CYCLE_MODE_CURVE;
    display_temperature_profile(temp_curves[reflow_profile_index-1], &target_temp_chart);

}

void tab_static_selected(){
    
    menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].item_data.icon = &home;
    menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].update = true;

    menu.current_tab = TAB_STATIC_SELECT;
    menu.tabs_vect[menu.current_tab].selected_item = SET_TEMP;

    target_temp = 100.0f;

    update_all_items(&menu.tabs_vect[menu.current_tab], STATIC_MODE_MENU_ITEM_COUNT);
    clr_buffer(&vb);

}

void target_temp_static_mode_selected(){

    const uint8_t CLEAR_ZONE_WIDTH = 6;

    bool clear = true;
    char f_char[6] = {0}; //5 aaa.b/0 no negative number
    InputEvent event = INPUT_NONE;

    uint8_t clear_index_x = menu.tabs_vect[menu.current_tab].items[SET_TEMP].pos_x;
    uint8_t clear_index_y = menu.tabs_vect[menu.current_tab].items[SET_TEMP].pos_y;
    
    enum digit_index{
        HUNDREDS = 1,
        TENS = 2,
        UNITS = 3,
        DEC = 4
    };

    uint8_t selected_digit = UNITS;

    //clear home icon: once you entered the temperature selection, 
    //the only way out is the main tab with the current mode of operation set to static mode with the set temperature
    clear_sector(0,0,16,16, &vb);  

    while(event != INPUT_SELECT){
    
        event = get_input_event_db();

        switch(event){

            case INPUT_UP:

                if(*menu.tabs_vect[menu.current_tab].items[SET_TEMP].item_data.float_val<300.0f){

                    switch (selected_digit){

                        case HUNDREDS: 
                                *menu.tabs_vect[menu.current_tab].items[SET_TEMP].item_data.float_val += 100.0f;
                        break;

                        case TENS: 
                                *menu.tabs_vect[menu.current_tab].items[SET_TEMP].item_data.float_val += 10.0f;
                        break;

                        case UNITS: 
                                *menu.tabs_vect[menu.current_tab].items[SET_TEMP].item_data.float_val += 1.0f;
                        break;    

                        case DEC: 
                                *menu.tabs_vect[menu.current_tab].items[SET_TEMP].item_data.float_val += 0.5f;
                        break;    

                        default:
                        break;      

                    }
                }

                else{
                    *menu.tabs_vect[menu.current_tab].items[SET_TEMP].item_data.float_val =300.0f;
                }

            break;

            case INPUT_DOWN:

                if(*menu.tabs_vect[menu.current_tab].items[SET_TEMP].item_data.float_val > 15.0f){

                    switch (selected_digit){

                        case HUNDREDS: 
                                *menu.tabs_vect[menu.current_tab].items[SET_TEMP].item_data.float_val -= 100.0f;
                        break;

                        case TENS: 
                                *menu.tabs_vect[menu.current_tab].items[SET_TEMP].item_data.float_val -= 10.0f;
                        break;

                        case UNITS: 
                                *menu.tabs_vect[menu.current_tab].items[SET_TEMP].item_data.float_val -= 1.0f;
                        break;    

                        case DEC: 
                                *menu.tabs_vect[menu.current_tab].items[SET_TEMP].item_data.float_val -= 0.5f;
                        break;    

                        default:
                        break;      

                    }
                }

                else{
                    *menu.tabs_vect[menu.current_tab].items[SET_TEMP].item_data.float_val = 15.0f;
                }

            break;

            case INPUT_LEFT:

                if(selected_digit>UNITS)
                    selected_digit = DEC;
                else
                    selected_digit++;

            break;

            case INPUT_RIGHT:

                if(selected_digit<TENS)
                    selected_digit = HUNDREDS;
                else
                    selected_digit--;
            
            break;

            default:
            break;

        }

        if(clear){
            if(selected_digit == DEC)
                clear_sector(clear_index_x + CLEAR_ZONE_WIDTH + ((selected_digit - 1) * CLEAR_ZONE_WIDTH) ,clear_index_y, CLEAR_ZONE_WIDTH, 7, &vb);    
            else
                clear_sector(clear_index_x + ((selected_digit - 1) * CLEAR_ZONE_WIDTH) ,clear_index_y, CLEAR_ZONE_WIDTH, 7, &vb);
        }

        else{
            clear_sector(menu.tabs_vect[menu.current_tab].items[SET_TEMP].pos_x,menu.tabs_vect[menu.current_tab].items[SET_TEMP].pos_y, 30, 7, &vb);
            float_to_char(*menu.tabs_vect[menu.current_tab].items[SET_TEMP].item_data.float_val, f_char, 6);
            put_string(menu.tabs_vect[menu.current_tab].items[SET_TEMP].pos_x,menu.tabs_vect[menu.current_tab].items[SET_TEMP].pos_y, f_char, SMALL, &vb);

        }

        if(lcd_refresh_elapsed) { //every half second
            clear = !clear;
            update_display(&vb);
            lcd_refresh_elapsed = 0 ;
        }

    }

    clr_buffer(&vb);

    sprintf(profile_name_str, "%s", "STATIC MODE");
    menu.current_tab = TAB_MAIN;
    menu.tabs_vect[menu.current_tab].items[PROFILE_NAME].update = true;
    menu.tabs_vect[menu.current_tab].selected_item = PLAY;
    menu.tabs_vect[menu.current_tab].items[PLAY].item_data.icon = &play_inv;
    menu.tabs_vect[menu.current_tab].items[PLAY].update = true;
    menu.tabs_vect[menu.current_tab].items[SETTINGS].item_data.icon = &settings_clear;
    menu.tabs_vect[menu.current_tab].items[SETTINGS].update = true;
    operation_cycle.cycle_type = CYCLE_MODE_STATIC;
    
} 

void none(){
    //nothing to see here :D
}

void preheat_temp_selected(){

    // da aggiungere i limiti per overtemperatura e temperature negative
    uint8_t CLEAR_ZONE_WIDTH = 16;

    bool clear = true;
    char f_char[6] = {0};
    InputEvent event = INPUT_NONE;

    uint8_t clear_index_x = menu.tabs_vect[menu.current_tab].items[PREHEAT_TEMP].pos_x;
    uint8_t clear_index_y = menu.tabs_vect[menu.current_tab].items[PREHEAT_TEMP].pos_y;
    
    enum digit_index{
        DEC,
        INT
    };

    uint8_t selected_digit = INT;

    while(event != INPUT_SELECT){
    
        event = get_input_event_db();

        switch(event){

            case INPUT_UP:
                    switch (selected_digit){

                        case INT:
                            
                            *menu.tabs_vect[menu.current_tab].items[PREHEAT_TEMP].item_data.float_val += 1.0f;

                        break;    

                        case DEC:
                            
                            *menu.tabs_vect[menu.current_tab].items[PREHEAT_TEMP].item_data.float_val += 0.5f;

                        break;    

                        default:
                        break;      

                    }

            break;

            case INPUT_DOWN:

                    switch (selected_digit){

                        case INT:
                            
                            *menu.tabs_vect[menu.current_tab].items[PREHEAT_TEMP].item_data.float_val -= 1.0f;

                        break;    

                        case DEC:
                            
                            *menu.tabs_vect[menu.current_tab].items[PREHEAT_TEMP].item_data.float_val -= 0.5f;

                        break;    
    
                        default:
                        break;      

                    }

            break;

            case INPUT_LEFT:
                
                selected_digit = DEC;
                CLEAR_ZONE_WIDTH = 5;
                clear_index_x = menu.tabs_vect[menu.current_tab].items[PREHEAT_TEMP].pos_x + 18;
            
            break;

            case INPUT_RIGHT:

                selected_digit = INT;
                CLEAR_ZONE_WIDTH = 16;
                clear_index_x = menu.tabs_vect[menu.current_tab].items[PREHEAT_TEMP].pos_x;
            break;

            default:
            break;

        }

        if(clear){
            if(selected_digit == DEC)
                clear_sector(clear_index_x + CLEAR_ZONE_WIDTH ,clear_index_y, CLEAR_ZONE_WIDTH, 7, &vb);    
            else
                clear_sector(clear_index_x + ((selected_digit - 1) * CLEAR_ZONE_WIDTH) ,clear_index_y, CLEAR_ZONE_WIDTH, 7, &vb);
        }

        else{
            clear_sector(menu.tabs_vect[menu.current_tab].items[PREHEAT_TEMP].pos_x,menu.tabs_vect[menu.current_tab].items[PREHEAT_TEMP].pos_y, 30, 7, &vb);
            float_to_char(*menu.tabs_vect[menu.current_tab].items[PREHEAT_TEMP].item_data.float_val, f_char, 6);
            put_string(menu.tabs_vect[menu.current_tab].items[PREHEAT_TEMP].pos_x,menu.tabs_vect[menu.current_tab].items[PREHEAT_TEMP].pos_y, f_char, SMALL, &vb);

        }

        if(lcd_refresh_elapsed) { //every half second
            clear = !clear;
            update_display(&vb);
            lcd_refresh_elapsed = 0 ;
        }

    }

    uint8_t offset = ( 5*(reflow_profile_index - 1) ) + menu.tabs_vect[TAB_CURVE_PARAM].selected_item + reflow_profile_index-1;

    flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, *menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].item_data.float_val, offset);

}

void preheat_time_selected(){

    // da aggiungere i limiti per overtemperatura e temperature negative
    uint8_t CLEAR_ZONE_WIDTH = 16;

    bool clear = true;
    char f_char[6] = {0};
    InputEvent event = INPUT_NONE;

    uint8_t clear_index_x = menu.tabs_vect[menu.current_tab].items[PREHEAT_TIME].pos_x;
    uint8_t clear_index_y = menu.tabs_vect[menu.current_tab].items[PREHEAT_TIME].pos_y;

    while(event != INPUT_SELECT){
    
        event = get_input_event_db();

        switch(event){

            case INPUT_UP:
                          
                *menu.tabs_vect[menu.current_tab].items[PREHEAT_TIME].item_data.float_val += 1.0f;
 
            break;

            case INPUT_DOWN:

                *menu.tabs_vect[menu.current_tab].items[PREHEAT_TIME].item_data.float_val -= 1.0f;

            break;

            default:
            break;

        }

        if(clear)

            clear_sector(clear_index_x,clear_index_y, CLEAR_ZONE_WIDTH, 7, &vb); 

        else{

            clear_sector(menu.tabs_vect[menu.current_tab].items[PREHEAT_TIME].pos_x,menu.tabs_vect[menu.current_tab].items[PREHEAT_TIME].pos_y, 30, 7, &vb);
            float_to_char(*menu.tabs_vect[menu.current_tab].items[PREHEAT_TIME].item_data.float_val, f_char, 6);
            put_string(menu.tabs_vect[menu.current_tab].items[PREHEAT_TIME].pos_x,menu.tabs_vect[menu.current_tab].items[PREHEAT_TIME].pos_y, f_char, SMALL, &vb);

        }

        if(lcd_refresh_elapsed) { //every half second
            clear = !clear;
            update_display(&vb);
            lcd_refresh_elapsed = 0 ;
        }

    }
    
    uint8_t offset = ( 5*(reflow_profile_index - 1) ) + menu.tabs_vect[TAB_CURVE_PARAM].selected_item + reflow_profile_index-1;

    flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, *menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].item_data.float_val, offset);
}

void soak_temp_selected(){

    // da aggiungere i limiti per overtemperatura e temperature negative
    uint8_t CLEAR_ZONE_WIDTH = 16;

    bool clear = true;
    char f_char[6] = {0};
    InputEvent event = INPUT_NONE;

    uint8_t clear_index_x = menu.tabs_vect[menu.current_tab].items[SOAK_TEMP].pos_x;
    uint8_t clear_index_y = menu.tabs_vect[menu.current_tab].items[SOAK_TEMP].pos_y;
    
    enum digit_index{
        DEC,
        INT
    };

    uint8_t selected_digit = INT;

    while(event != INPUT_SELECT){
    
        event = get_input_event_db();

        switch(event){

            case INPUT_UP:
                    switch (selected_digit){

                        case INT:
                            
                            *menu.tabs_vect[menu.current_tab].items[SOAK_TEMP].item_data.float_val += 1.0f;

                        break;    

                        case DEC:
                            
                            *menu.tabs_vect[menu.current_tab].items[SOAK_TEMP].item_data.float_val += 0.5f;

                        break;    

                        default:
                        break;      

                    }

            break;

            case INPUT_DOWN:

                    switch (selected_digit){

                        case INT:
                            
                            *menu.tabs_vect[menu.current_tab].items[SOAK_TEMP].item_data.float_val -= 1.0f;

                        break;    

                        case DEC:
                            
                            *menu.tabs_vect[menu.current_tab].items[SOAK_TEMP].item_data.float_val -= 0.5f;

                        break;    
    
                        default:
                        break;      

                    }

            break;

            case INPUT_LEFT:
                
                selected_digit = DEC;
                CLEAR_ZONE_WIDTH = 5;
                clear_index_x = menu.tabs_vect[menu.current_tab].items[SOAK_TEMP].pos_x + 18;
            
            break;

            case INPUT_RIGHT:

                selected_digit = INT;
                CLEAR_ZONE_WIDTH = 16;
                clear_index_x = menu.tabs_vect[menu.current_tab].items[SOAK_TEMP].pos_x;
            break;

            default:
            break;

        }

        if(clear){
            if(selected_digit == DEC)
                clear_sector(clear_index_x + CLEAR_ZONE_WIDTH ,clear_index_y, CLEAR_ZONE_WIDTH, 7, &vb);    
            else
                clear_sector(clear_index_x + ((selected_digit - 1) * CLEAR_ZONE_WIDTH) ,clear_index_y, CLEAR_ZONE_WIDTH, 7, &vb);
        }

        else{
            clear_sector(menu.tabs_vect[menu.current_tab].items[SOAK_TEMP].pos_x,menu.tabs_vect[menu.current_tab].items[SOAK_TEMP].pos_y, 30, 7, &vb);
            float_to_char(*menu.tabs_vect[menu.current_tab].items[SOAK_TEMP].item_data.float_val, f_char, 6);
            put_string(menu.tabs_vect[menu.current_tab].items[SOAK_TEMP].pos_x,menu.tabs_vect[menu.current_tab].items[SOAK_TEMP].pos_y, f_char, SMALL, &vb);

        }

        if(lcd_refresh_elapsed) { //every half second
            clear = !clear;
            update_display(&vb);
            lcd_refresh_elapsed = 0 ;
        }

    }
    
    uint8_t offset = ( 5*(reflow_profile_index - 1) ) + menu.tabs_vect[TAB_CURVE_PARAM].selected_item + reflow_profile_index-1;

    flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, *menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].item_data.float_val, offset);
}

void soak_time_selected(){

    // da aggiungere i limiti per overtemperatura e temperature negative
    uint8_t CLEAR_ZONE_WIDTH = 16;

    bool clear = true;
    char f_char[6] = {0};
    InputEvent event = INPUT_NONE;

    uint8_t clear_index_x = menu.tabs_vect[menu.current_tab].items[SOAK_TIME].pos_x;
    uint8_t clear_index_y = menu.tabs_vect[menu.current_tab].items[SOAK_TIME].pos_y;

    while(event != INPUT_SELECT){
    
        event = get_input_event_db();

        switch(event){

            case INPUT_UP:
                          
                *menu.tabs_vect[menu.current_tab].items[SOAK_TIME].item_data.float_val += 1.0f;
 
            break;

            case INPUT_DOWN:

                *menu.tabs_vect[menu.current_tab].items[SOAK_TIME].item_data.float_val -= 1.0f;

            break;

            default:
            break;

        }

        if(clear)

            clear_sector(clear_index_x,clear_index_y, CLEAR_ZONE_WIDTH, 7, &vb); 

        else{

            clear_sector(menu.tabs_vect[menu.current_tab].items[SOAK_TIME].pos_x,menu.tabs_vect[menu.current_tab].items[SOAK_TIME].pos_y, 30, 7, &vb);
            float_to_char(*menu.tabs_vect[menu.current_tab].items[SOAK_TIME].item_data.float_val, f_char, 6);
            put_string(menu.tabs_vect[menu.current_tab].items[SOAK_TIME].pos_x,menu.tabs_vect[menu.current_tab].items[SOAK_TIME].pos_y, f_char, SMALL, &vb);

        }

        if(lcd_refresh_elapsed) { //every half second
            clear = !clear;
            update_display(&vb);
            lcd_refresh_elapsed = 0 ;
        }

    }
    
    uint8_t offset = ( 5*(reflow_profile_index - 1) ) + menu.tabs_vect[TAB_CURVE_PARAM].selected_item + reflow_profile_index-1;

    flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, *menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].item_data.float_val, offset);
}

void peak_temp_selected(){

    // da aggiungere i limiti per overtemperatura e temperature negative
    uint8_t CLEAR_ZONE_WIDTH = 16;

    bool clear = true;
    char f_char[6] = {0};
    InputEvent event = INPUT_NONE;

    uint8_t clear_index_x = menu.tabs_vect[menu.current_tab].items[PEAK_TEMP].pos_x;
    uint8_t clear_index_y = menu.tabs_vect[menu.current_tab].items[PEAK_TEMP].pos_y;
    
    enum digit_index{
        DEC,
        INT
    };

    uint8_t selected_digit = INT;

    while(event != INPUT_SELECT){
    
        event = get_input_event_db();

        switch(event){

            case INPUT_UP:
                    switch (selected_digit){

                        case INT:
                            
                            *menu.tabs_vect[menu.current_tab].items[PEAK_TEMP].item_data.float_val += 1.0f;

                        break;    

                        case DEC:
                            
                            *menu.tabs_vect[menu.current_tab].items[PEAK_TEMP].item_data.float_val += 0.5f;

                        break;    

                        default:
                        break;      

                    }

            break;

            case INPUT_DOWN:

                    switch (selected_digit){

                        case INT:
                            
                            *menu.tabs_vect[menu.current_tab].items[PEAK_TEMP].item_data.float_val -= 1.0f;

                        break;    

                        case DEC:
                            
                            *menu.tabs_vect[menu.current_tab].items[PEAK_TEMP].item_data.float_val -= 0.5f;

                        break;    
    
                        default:
                        break;      

                    }

            break;

            case INPUT_LEFT:
                
                selected_digit = DEC;
                CLEAR_ZONE_WIDTH = 5;
                clear_index_x = menu.tabs_vect[menu.current_tab].items[PEAK_TEMP].pos_x + 18;
            
            break;

            case INPUT_RIGHT:

                selected_digit = INT;
                CLEAR_ZONE_WIDTH = 16;
                clear_index_x = menu.tabs_vect[menu.current_tab].items[PEAK_TEMP].pos_x;
            break;

            default:
            break;

        }

        if(clear){
            if(selected_digit == DEC)
                clear_sector(clear_index_x + CLEAR_ZONE_WIDTH ,clear_index_y, CLEAR_ZONE_WIDTH, 7, &vb);    
            else
                clear_sector(clear_index_x + ((selected_digit - 1) * CLEAR_ZONE_WIDTH) ,clear_index_y, CLEAR_ZONE_WIDTH, 7, &vb);
        }

        else{
            clear_sector(menu.tabs_vect[menu.current_tab].items[PEAK_TEMP].pos_x,menu.tabs_vect[menu.current_tab].items[PEAK_TEMP].pos_y, 30, 7, &vb);
            float_to_char(*menu.tabs_vect[menu.current_tab].items[PEAK_TEMP].item_data.float_val, f_char, 6);
            put_string(menu.tabs_vect[menu.current_tab].items[PEAK_TEMP].pos_x,menu.tabs_vect[menu.current_tab].items[PEAK_TEMP].pos_y, f_char, SMALL, &vb);

        }

        if(lcd_refresh_elapsed) { //every half second
            clear = !clear;
            update_display(&vb);
            lcd_refresh_elapsed = 0 ;
        }

    }
    
    uint8_t offset = ( 5*(reflow_profile_index - 1) ) + menu.tabs_vect[TAB_CURVE_PARAM].selected_item + reflow_profile_index-1;

    flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, *menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].item_data.float_val, offset);

}

void reflow_time_selected(){

    // da aggiungere i limiti per overtemperatura e temperature negative
    uint8_t CLEAR_ZONE_WIDTH = 16;

    bool clear = true;
    char f_char[6] = {0};
    InputEvent event = INPUT_NONE;

    uint8_t clear_index_x = menu.tabs_vect[menu.current_tab].items[REFLOW_TIME].pos_x;
    uint8_t clear_index_y = menu.tabs_vect[menu.current_tab].items[REFLOW_TIME].pos_y;

    while(event != INPUT_SELECT){
    
        event = get_input_event_db();

        switch(event){

            case INPUT_UP:
                          
                *menu.tabs_vect[menu.current_tab].items[REFLOW_TIME].item_data.float_val += 1.0f;
 
            break;

            case INPUT_DOWN:

                *menu.tabs_vect[menu.current_tab].items[REFLOW_TIME].item_data.float_val -= 1.0f;

            break;

            default:
            break;

        }

        if(clear)

            clear_sector(clear_index_x,clear_index_y, CLEAR_ZONE_WIDTH, 7, &vb); 

        else{

            clear_sector(menu.tabs_vect[menu.current_tab].items[REFLOW_TIME].pos_x,menu.tabs_vect[menu.current_tab].items[REFLOW_TIME].pos_y, 30, 7, &vb);
            float_to_char(*menu.tabs_vect[menu.current_tab].items[REFLOW_TIME].item_data.float_val, f_char, 6);
            put_string(menu.tabs_vect[menu.current_tab].items[REFLOW_TIME].pos_x,menu.tabs_vect[menu.current_tab].items[REFLOW_TIME].pos_y, f_char, SMALL, &vb);

        }

        if(lcd_refresh_elapsed) { //every half second
            clear = !clear;
            update_display(&vb);
            lcd_refresh_elapsed = 0 ;
        }

    }
    
    uint8_t offset = ( 5*(reflow_profile_index - 1) ) + menu.tabs_vect[TAB_CURVE_PARAM].selected_item + reflow_profile_index-1;

    flash_write_float(CURVE_FLASH_INITIAL_ADDRESS, *menu.tabs_vect[menu.current_tab].items[menu.tabs_vect[menu.current_tab].selected_item].item_data.float_val, offset);
}