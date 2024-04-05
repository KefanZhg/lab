#ifndef USR_APP_H_
#define USR_APP_H_

#include "usr_config.h"

#include "usr_cli.h"

#include "usr_kbd.h"
#include "usr_mouse.h"

void usr_app_init(uint8_t master, app_usbd_hid_mouse_t const * p_mouse);
void usr_app_run(void);

void usr_btn_init(void);
void usr_btn_event_callback(uint8_t pin_no, uint8_t button_action);

#endif /* USR_APP_H_ */
