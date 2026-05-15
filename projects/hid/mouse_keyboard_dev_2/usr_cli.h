#ifndef USR_CLI_H_
#define USR_CLI_H_

#include "usr_config.h"

#include "usr_kbd.h"
#include "usr_mouse.h"

extern nrf_cli_t const * p_cli;

void init_cli(void);

#endif /* USR_CLI_H_ */
