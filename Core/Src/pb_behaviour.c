#include "pb_behaviour.h"

/** timer_increment
  * @brief  increments timer tick count to keep track of the passing of time. It must be called every ms;
  * @param  pb: structure with all the pin and timing specific settings
  * @retval none
*/
void timer_increment(pb_str *pb){
	pb->t = pb->t + 1;
}

/** pb_debounce
  * @brief  non blocking debounce filter on an input pin
  * @param  pb: structure with all the pin and timing specific settings
  * @retval boolean pin value with debounce filtering
*/

bool pb_debounce(pb_str *pb){

	switch (pb->event_machines & 0x03){

	case 0x00:
		pb->pb_db_state = 0;
		pb->pb_status = pb->input_f_ptr();

		if(pb->pb_status & !pb->pb_old_status){
			pb->event_machines = (pb->event_machines & 0xFC) | 0x01;
			pb->t_db = *pb->t;
		}

		pb->pb_old_status = pb->pb_status;
		break;

	case 0x01:
		if(*pb->t - pb->t_db >= pb->db_time){
			pb->event_machines = (pb->event_machines & 0xFC) | 0x02;
			pb->pb_db_state = 1;
		}

		break;

	case 0x02:
		pb->pb_status = pb->input_f_ptr();

		if(!pb->pb_status & pb->pb_old_status){
			pb->event_machines = (pb->event_machines & 0xFC) | 0x03;
			pb->t_db = *pb->t;
		}

		pb->pb_old_status = pb->pb_status;
		break;

	case 0x03:
		if(*pb->t - pb->t_db >= pb->db_time){
			pb->event_machines = pb->event_machines & 0xFC;
			pb->press_cnt++;
		}

		break;

	default:
		pb->event_machines = pb->event_machines & 0xFC;
		break;

	}

	return pb->pb_db_state;
}

/** pb_longpress
  * @brief  non blocking function to detect a long push on a button
  * @param  pb: structure with all the pin and timing specific settings
  * @retval boolean: long press acknowledgement
*/
bool pb_longpress(pb_str *pb){

	switch (pb->event_machines & 0x0C){

	case 0x00:
		pb->pb_lp_ack = 0;

		if(pb_debounce(pb)) {
			pb->t_lp = *pb->t;
			pb->event_machines = (pb->event_machines & 0xF3) | 0x04;
			}

		break;

	case 0x04:
		if(pb->input_f_ptr()) {

			if(*pb->t - pb->t_lp >= pb->lp_time ){
				pb->event_machines = (pb->event_machines & 0xF3) | 0x08;
				pb->pb_lp_ack = 1;
				}

		}
		else
			pb->event_machines = (pb->event_machines & 0xF3);

		break;

	case 0x08:
		if(!(pb->input_f_ptr()))
			pb->event_machines = (pb->event_machines & 0xF3);

		break;

	default:
		pb->event_machines = (pb->event_machines & 0xF3);
		break;

	}

	return pb->pb_lp_ack;
}

/** pb_multitap
  * @brief  non blocking function to detect a series of pushes on the button within a time window
  * @param  pb: structure with all the pin and timing specific settings
  * @retval boolean: multiple pushes acknowledgement
*/

bool pb_multitap(pb_str *pb){

	switch (pb->event_machines & 0x30){

	case 0x00:
		if(pb->press_cnt != 0){
			pb->t_mt = *pb->t;
			pb->event_machines = (pb->event_machines & 0xCF) | 0x10;
		}

		pb->pb_mt_ack = 0;
		break;

	case 0x10:
		if(*pb->t - pb->t_mt <= pb->mt_time ){

			if(pb->press_cnt >= pb->tap_num){
				pb->press_cnt = 0;
				pb->pb_mt_ack = 1;
				pb->event_machines = (pb->event_machines & 0xCF);
				break;
			}
		}

		else{
			pb->event_machines = (pb->event_machines & 0xCF);
			pb->press_cnt = 0;
		}

		break;

	default:
		pb->event_machines = (pb->event_machines & 0xCF);
		pb->press_cnt = 0;
		break;
	}

	return pb->pb_mt_ack;
}

