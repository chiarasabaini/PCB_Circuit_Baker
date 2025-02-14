#ifndef _MENU_
#define _MENU_

#include "GEVA.h"
#include "string.h"
#include "buttons_definition.h"
#include "high_level_time.h"
#include "pid.h"

#define MAIN_MENU_ITEM_COUNT 8
#define SETTINGS_MENU_ITEM_COUNT 4
#define STATIC_MODE_MENU_ITEM_COUNT 2
#define PID_MENU_ITEM_COUNT 4
#define CURVES_MENU_ITEM_COUNT 3
#define CURVES_PROFILE_COUNT 4
#define CURVES_PARAM_COUNT 7

#define TABS_COUNT 8
#define TABS_MAX_ITEM_COUNT 8

enum maintab_item_mnemonic{
    PROFILE_NAME,
    TEMP_CHART,
    STATUS,
    PLAY,
    SETTINGS,
    TIME,
    TARGET_TEMP,
    CURRENT_TEMP,
};

enum settingstab_item_mnemonic{
    PID,
    CURVES,
    STATIC,
    BACK_S,
};

enum pidtab_item_mnemonic{
    KP_PARAM,
    KI_PARAM,
    KD_PARAM,
    BACK_P
};

enum curvestab_item_mnemonic{
    SELECT,
    EDIT,
    BACK_C
};

enum selectcurvestab_item_mnemonic{
    PROFILE_1,
    PROFILE_2,
    PROFILE_3,
    BACK_CS
};

enum curveparamtab_item_mnemonic{
    PREHEAT_TEMP,
    PREHEAT_TIME,
    SOAK_TEMP,
    SOAK_TIME,
    PEAK_TEMP,  
    REFLOW_TIME,
    BACK_CP
};

enum staticmodetab_item_mnemonic{
    SET_TEMP,
    BACK_ST,
};

//identify items on tab based on type
typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STR,
    TYPE_GRAPH,
    TYPE_ICON,
    TYPE_TIME,
    TYPE_NONE,
} ItemType;

//identify tabs
enum tab_mnemonic{
    TAB_MAIN,
    TAB_SETTINGS,
    TAB_MONITOR,
    TAB_PID,
    TAB_CURVE,
    TAB_SELECT_CURVE,
    TAB_CURVE_PARAM,
    TAB_STATIC_SELECT
};

typedef struct {
    
    ItemType type;
    bool update;
    uint8_t pos_x;
    uint8_t pos_y;
    
    union {
        int *int_val;
        float *float_val;
        char *str_val;
        data_container *chart;
        vbc_file *icon;
        hilv_time *hl_time;
    } item_data;

    void (*select_function)(void);

} Item;

typedef struct {

    void (*template_draw_fnt)(void);
    Item items[TABS_MAX_ITEM_COUNT];
    uint8_t items_cnt;
    uint8_t selected_item;
    uint8_t tab_id; 

}Tab;

typedef struct {
    Tab tabs_vect[TABS_COUNT];         
    uint8_t current_tab;      

} Menu;


void render_tab(Tab tab);
void cycle_tab(Tab *tabs, uint8_t direction);


void menu_init();
void handle_input();

void render_menu();
void display_temperature_profile(reflow_profile profile, data_container *d);

#endif