#ifndef _ICONS_
#define _ICONS_

#include "GEVA.h"

#define MAX_BODY_SIZE 150

extern vbc_file settings_clear;
extern uint8_t settings_clear_sv[30];
void icon_settings_clear_init();

extern vbc_file empty_16x16;
extern uint8_t empty_16x16_sv[30];
void icon_empty_16x16_init();

extern vbc_file play_clear;
extern uint8_t play_clear_sv[30];
void icon_play_clear_init();

extern vbc_file heating;
extern uint8_t heating_sv[30];
void icon_heating_init();

extern vbc_file not_heating;
extern uint8_t not_heating_sv[30];
void icon_not_heating_init();

extern vbc_file cooling;
extern uint8_t cooling_sv[30];
void icon_cooling_init();

extern vbc_file keep;
extern uint8_t keep_sv[30];
void icon_keep_init();

extern vbc_file pid;
extern uint8_t pid_sv[150];
void icon_pid_init();

extern vbc_file curves;
extern uint8_t curves_sv[150];
void icon_curves_init();

extern vbc_file static_mode;
extern uint8_t static_sv[150];
void icon_static_mode_init();

extern vbc_file monitor_small;
extern uint8_t monitor_small_sv[30];
void icon_monitor_small_init();

extern vbc_file monitor_small_inv;
extern uint8_t monitor_small_inv_sv[30];
void icon_monitor_small_inv_init();

extern vbc_file play_inv;
extern uint8_t play_inv_sv[30];
void icon_play_inv_init();

extern vbc_file settings_inv;
extern uint8_t settings_inv_sv[30];
void icon_settings_inv_init();


extern vbc_file pid_inv;
extern uint8_t pid_inv_sv[150];
void icon_pid_inv_init();

extern vbc_file static_mode_inv;
extern uint8_t static_mode_inv_sv[150];
void icon_static_mode_inv_init();

extern vbc_file curves_inv;
extern uint8_t curves_inv_sv[150];
extern void icon_curves_inv_init();

extern vbc_file home;
extern uint8_t home_sv[45];
extern void icon_home_init();

extern vbc_file home_inv;
extern uint8_t home_inv_sv[45];
extern void icon_home_inv_init();

#endif