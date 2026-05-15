#include "usr_select.h"

bool usr_select_is_slave = true;

/************************************************************************
                    The following parts are for the general cli 
*************************************************************************/

// extern void bsp_event_callback(bsp_event_t ev);

/**
 * @brief CLI interface over UART
 */
NRF_CLI_UART_DEF(usr_select_cli_uart_transport, 1, 64, 16);
NRF_CLI_DEF(usr_select_cli_uart,
                "uart1_cli:~$ ",
                &usr_select_cli_uart_transport.transport,
                '\r',
                4);

nrf_cli_t const * p_usr_select_cli = &usr_select_cli_uart;

void usr_select_cli_init(void)
{
    ret_code_t ret;
    // ret = bsp_cli_init(bsp_event_callback);
    // APP_ERROR_CHECK(ret);
    nrf_drv_uart_config_t uart_config = NRF_DRV_UART_DEFAULT_CONFIG;
    uart_config.pseltxd = USR_SELECT_UART_TX_PIN;
    uart_config.pselrxd = USR_SELECT_UART_RX_PIN;
    uart_config.hwfc    = NRF_UART_HWFC_DISABLED;
    uart_config.baudrate= USR_SELECT_UART_BAUDRATE; // Set baudrate to 921600
    ret = nrf_cli_init(p_usr_select_cli, &uart_config, true, true, NRF_LOG_SEVERITY_NONE);
    APP_ERROR_CHECK(ret);
    ret = nrf_cli_start(p_usr_select_cli);
    APP_ERROR_CHECK(ret);
}

void usr_select_init(void)
{

    usr_select_cli_init();
    // NRF_LOG_RAW_INFO("Select slave or master (s/m): ");
    // // Transmit something
    // printf("Select slave or master (s/m): ");
    // for (char c = 'a'; c <= 'z'; c++)
    // {
    //     while (app_uart_put(c) != NRF_SUCCESS);
    // }

    // Initialize the slave
    if (usr_select_is_slave)
    {
        usr_slave_init();
    }
    // Initialize the master
    else
    {
        usr_master_init();
    }
}