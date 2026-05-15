#ifndef USR_MASTER_H
#define USR_MASTER_H

#include "usr_config.h"

void usr_master_init(void);

void usr_master_btn_handler(usr_btn_type_t btn_type, uint8_t btn_index, usr_btn_state_t btn_state);

void usr_master_mouse_handler(usr_mouse_axis_t axis, int8_t value);

#endif // USR_MASTER_H
