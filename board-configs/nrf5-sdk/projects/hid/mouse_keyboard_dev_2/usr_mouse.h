#ifndef USR_MOUSE_H_
#define USR_MOUSE_H_

#include "usr_config.h"

typedef struct usr_mouse_s {
    app_usbd_hid_mouse_t const * p_mouse;
} usr_mouse_t;

extern usr_mouse_t usr_mouse;

void usr_mouse_init(app_usbd_hid_mouse_t const * p_mouse);

void usr_mouse_saadc_init(void);
void usr_mouse_saadc_callback(nrf_drv_saadc_evt_t const * p_event);

void usr_mouse_timer_handler(void * p_context);

extern int16_t usr_mouse_dis_rst[2];

#endif /* USR_MOUSE_H_ */
