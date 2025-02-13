
#ifndef _PB_BEH_
#define _PB_BEH_

#include <stdbool.h>
#include <stdint.h>


/**
  * @brief  push button behaviour structures definition
  */

typedef struct {

/* Shared Variables  ************************************************/

	uint8_t event_machines;		//contiene gli stati delle macchine che regolano il comportamento del pulsante
								//così diviso 00_00_00_00 --> macchina4_macchina3_macchina_2_macchina1
	volatile uint32_t *t;		//variabile tick di sistema per il controllo temporale:deve aggiornarsi ogni ms

/* Input pointer function that return GPIO hardware status*/
	bool (*input_f_ptr)(void);

/* Simple push Variables  ************************************************/

	uint32_t t_db;
	uint8_t db_time;		//tempo di debounce da 0 a 255ms
	bool pb_status;
	bool pb_old_status;
	bool pb_db_state;			//stato bottone dopo debounce;
	uint8_t press_cnt;			//variabile conteggio dei cicli pressioni (premuto/rilasciato)

/* Sustained push Variables  ************************************************/

	uint32_t t_lp;
	uint16_t lp_time;		//tempo pressione per ack longpress in ms
	bool pb_lp_ack;				//ack long press

/* Multi-push Variables  ************************************************/

	uint32_t t_mt;
	uint16_t mt_time;		//tempo fronti successivi per ack doubletap in ms
	uint8_t tap_num;			//numero di tap per ack
	bool pb_mt_ack;				//ack multiplo tap

} pb_str;


void timer_increment(pb_str *pb);  //call this function every ms

/* Behaviour Control functions  ************************************************/

bool pb_debounce(pb_str *pb);
bool pb_longpress(pb_str *pb);
bool pb_multitap(pb_str *pb);

#endif


