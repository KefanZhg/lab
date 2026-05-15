#ifndef USR_SLAVE_H
#define USR_SLAVE_H

#include "usr_config.h"



void usr_slave_init(void);

void usr_slave_btn_handler(usr_btn_type_t btn_type, uint8_t btn_index, usr_btn_state_t btn_state);

void usr_slave_mouse_handler(usr_mouse_axis_t axis, int8_t value);

#endif // USR_SLAVE_H
