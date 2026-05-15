#ifndef USR_SELECT_H
#define USR_SELECT_H

#include "usr_config.h"

#include "usr_slave.h"
#include "usr_master.h"

extern bool usr_select_is_slave;
extern nrf_cli_t const * p_usr_select_cli;

#define USR_SELECT_UART_BAUDRATE APP_CONFIG_DEFAULT_UART_BAUDRATE
#define USR_SELECT_UART_RX_PIN 4
#define USR_SELECT_UART_TX_PIN 5

void usr_select_init(void);

#define usr_btn_handler(btn_type, btn_index, btn_state) \
    if (usr_select_is_slave) { \
        usr_slave_btn_handler(btn_type, btn_index, btn_state); \
    } else { \
        usr_master_btn_handler(btn_type, btn_index, btn_state); \
    }

#define usr_mouse_handler(axis, value) \
    if (usr_select_is_slave) { \
        usr_slave_mouse_handler(axis, value); \
    } else { \
        usr_master_mouse_handler(axis, value); \
    }

#endif // USR_SELECT_H
