#ifndef USR_CONFIG_H_
#define USR_CONFIG_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"

#include "nrfx_saadc.h"
#include "nrfx_spi.h"
#include "nrfx_spim.h"
#include "nrfx_uart.h"
#include "nrfx_uarte.h"

#include "nrf_drv_clock.h"
#include "nrf_drv_saadc.h"
#include "nrf_drv_power.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_timer.h"
#include "nrf_drv_usbd.h"
#include "nrf_drv_spi.h"
#include "nrf_drv_uart.h"

#include "nrf_spi_mngr.h"
#include "nrf_pwr_mgmt.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "app_button.h"
#include "app_timer.h"
#include "app_usbd.h"
#include "app_usbd_core.h"
#include "app_usbd_hid_mouse.h"
#include "app_usbd_hid_kbd.h"
#include "app_usbd_dummy.h"
#include "app_error.h"

#include "bsp_cli.h"
#include "nrf_cli.h"
#include "nrf_cli_uart.h"

#include "boards.h"
#include "inttypes.h"

#define USR_JOYSTICK_BTN_PIN    16

#define USR_MOUSE_X_ADC_INPUT NRF_SAADC_INPUT_AIN4
#define USR_MOUSE_Y_ADC_INPUT NRF_SAADC_INPUT_AIN6
#define USR_MOUSE_ADC_SAMPLE_NUM 8
#define USR_MOUSE_ADC_OFFSET     430
#define USR_MOUSE_SIGNAL_MARGIN  10
#define USR_MOUSE_SIGNAL_SCALE   40
#define USR_MOUSE_TIMER_INTERVAL 10

typedef enum {
    USR_BTN_TYPE_KBD_GENERAL = 0,
    USR_BTN_TYPE_KBD_MODIFIER,
    USR_BTN_TYPE_MOUSE,
    USR_BTN_TYPE_JOYSTICK,
    USR_BTN_TYPE_MAX
} usr_btn_type_t;

typedef enum {
    USR_BTN_STATE_RELEASE = 0,
    USR_BTN_STATE_PRESS,
    USR_BTN_STATE_TAP,
    USR_BTN_STATE_MAX
} usr_btn_state_t;

typedef enum {
    USR_MOUSE_AXIS_X = 0,
    USR_MOUSE_AXIS_Y,
    USR_MOUSE_AXIS_SCROLL,
    USR_MOUSE_AXIS_MAX
} usr_mouse_axis_t;

#endif /* USR_CONFIG_H_ */
