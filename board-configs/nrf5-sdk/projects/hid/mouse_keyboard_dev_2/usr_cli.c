#include "usr_cli.h"

/************************************************************************
                    The following parts are for the general cli 
*************************************************************************/

extern void bsp_event_callback(bsp_event_t ev);

/**
 * @brief CLI interface over UART
 */
NRF_CLI_UART_DEF(m_cli_uart_transport, 0, 64, 16);
NRF_CLI_DEF(m_cli_uart,
                "uart_cli:~$ ",
                &m_cli_uart_transport.transport,
                '\r',
                4);
nrf_cli_t const * p_cli = &m_cli_uart;

void init_cli(void)
{
    ret_code_t ret;
    ret = bsp_cli_init(bsp_event_callback);
    APP_ERROR_CHECK(ret);
    nrf_drv_uart_config_t uart_config = NRF_DRV_UART_DEFAULT_CONFIG;
    uart_config.pseltxd = APP_TX_PIN_NUMBER;
    uart_config.pselrxd = APP_RX_PIN_NUMBER;
    uart_config.hwfc    = NRF_UART_HWFC_DISABLED;
    uart_config.baudrate= APP_CONFIG_DEFAULT_UART_BAUDRATE; // Set baudrate to 921600
    ret = nrf_cli_init(p_cli, &uart_config, false, true, NRF_LOG_SEVERITY_INFO);
    APP_ERROR_CHECK(ret);
    ret = nrf_cli_start(p_cli);
    APP_ERROR_CHECK(ret);
}
/************************************************************************
                    The following parts are for the helper 
*************************************************************************/

const char usr_cli_kbd_help_msg[] = \
"KBD help:\n\
    [p]ress [id]: press and hold the key with [id]\n\
    [r]elease [id]: release the key with [id]\n\
    [t]ap [id]: press and quickly release the key with [id].\n\
    [g]eneral [id] [state]: [1] press [2] release [0] tap the key [id]\n\
    [m]odifier [id] [state]: [1] press [2] release [0] tap the key [id]\n\n";

void usr_cli_kbd_help(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, usr_cli_kbd_help_msg);
}

/************************************************************************
                    The following parts are for the keyboard 
*************************************************************************/

extern app_usbd_hid_kbd_t m_app_hid_kbd;

void usr_cli_kbd_general(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
        static uint8_t id = 0, state = 0;
        app_usbd_hid_kbd_codes_t key;
        int rst;

        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "argc: %d.\n", argc);

        if(argc >= 2)
        {
            rst = sscanf(argv[1], "%hhu", &id);
            nrf_cli_fprintf(p_cli,NRF_CLI_NORMAL,"rst %d\n", rst);

            if(rst)
                key = (app_usbd_hid_kbd_codes_t)id;
            else
                key = APP_USBD_HID_KBD_SPACEBAR;

            if(argc >= 3)
            {
                rst = sscanf(argv[2], "%hhu", &state);
                if(!rst) state = 0;
            }
        }
        else
        {
            key = APP_USBD_HID_KBD_SPACEBAR;
        }
        nrf_cli_fprintf(p_cli,NRF_CLI_NORMAL,"id %hhu key %hhu\n", id, key);
        nrf_cli_fprintf(p_cli,NRF_CLI_NORMAL,"%s\n", argv[0]);
        nrf_cli_fprintf(p_cli,NRF_CLI_NORMAL,"%s\n", argv[1]);
        nrf_cli_fprintf(p_cli,NRF_CLI_NORMAL,"%s\n", argv[2]);

        switch(state)
        {
                case 1: // press
                    UNUSED_RETURN_VALUE(app_usbd_hid_kbd_key_control(&m_app_hid_kbd, key, true));
                    nrf_cli_fprintf(p_cli,NRF_CLI_NORMAL,"pressed %hhu\n", key);
                    break;

                case 2: // release
                    UNUSED_RETURN_VALUE(app_usbd_hid_kbd_key_control(&m_app_hid_kbd, key, false));
                    break;

                default: // tap
                    UNUSED_RETURN_VALUE(app_usbd_hid_kbd_key_control(&m_app_hid_kbd, key, true));
                    UNUSED_RETURN_VALUE(app_usbd_hid_kbd_key_control(&m_app_hid_kbd, key, false));
        }
}

void usr_cli_kbd_modifier(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
        uint8_t id, state = 0;
        app_usbd_hid_kbd_codes_t key;

        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "argc: %d.\n", argc);

        if(argc >= 2)
        {
            sscanf(argv[1], "%hhu", &id);
            key = (app_usbd_hid_kbd_codes_t)id;

            if(argc >= 3)
            {
                sscanf(argv[2], "%hhu", &state);
            }
        }
        else
        {
            key = APP_USBD_HID_KBD_SPACEBAR;
        }

        switch(state)
        {
                case 1: // press
                    UNUSED_RETURN_VALUE(app_usbd_hid_kbd_modifier_state_set(&m_app_hid_kbd, key, true));
                    break;

                case 2: // release
                    UNUSED_RETURN_VALUE(app_usbd_hid_kbd_modifier_state_set(&m_app_hid_kbd, key, false));
                    break;

                default: // tap
                    UNUSED_RETURN_VALUE(app_usbd_hid_kbd_modifier_state_set(&m_app_hid_kbd, key, true));
                    UNUSED_RETURN_VALUE(app_usbd_hid_kbd_modifier_state_set(&m_app_hid_kbd, key, false));
        }
}

NRF_CLI_CREATE_STATIC_SUBCMD_SET(m_sub_kbd_s)
{
        NRF_CLI_CMD(g, NULL, "KBD general", usr_cli_kbd_general),
        NRF_CLI_CMD(m, NULL, "KBD modifier", usr_cli_kbd_modifier),
        NRF_CLI_SUBCMD_SET_END
};
NRF_CLI_CMD_REGISTER(k, &m_sub_kbd_s, "kbd", NULL);


/************************************************************************
                    The following parts are for the mouse 
*************************************************************************/


extern app_usbd_hid_mouse_t m_app_hid_mouse;

void usr_cli_mouse_btn(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
        static uint8_t btn, state = 0;
        int rst = 0;

        if(argc >= 2)
        {
            rst = sscanf(argv[1], "%hhu", &btn);
            if(!rst) btn = 0;
            if(btn >= 8)
            {
                    nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Mouse ID %d should not >= 8.\n", btn);
                    return;
            }

            if(argc >= 3)
            {
                rst = sscanf(argv[2], "%hhu", &state);
                if(!rst) state = 0;
            }
        }
        else
        {
            btn = 0;
        }

        switch(state)
        {
                case 1: // press
                    UNUSED_RETURN_VALUE(app_usbd_hid_mouse_button_state(&m_app_hid_mouse, btn, true));
                    break;

                case 2: // release
                    UNUSED_RETURN_VALUE(app_usbd_hid_mouse_button_state(&m_app_hid_mouse, btn, false));
                    break;

                default: // tap
                    UNUSED_RETURN_VALUE(app_usbd_hid_mouse_button_state(&m_app_hid_mouse, btn, true));
                    UNUSED_RETURN_VALUE(app_usbd_hid_mouse_button_state(&m_app_hid_mouse, btn, false));
        }
}

extern int32_t mouse_target[3]; 
extern void start_timer(void);

void usr_cli_mouse_move(nrf_cli_t const * p_cli, size_t argc, char **argv)
{
        static uint8_t axis = 0;
        static int32_t offset = 0;
        int rst;

        if(argc >= 2)
        {
            rst = sscanf(argv[1], "%hhu", &axis);
            if(!rst) axis = 0;
            if(axis >= 3)
            {
                    nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Mouse axis ID %hhu should not >= 3.\n", axis);
                    return;
            }
            if(argc >= 3)
            {
                rst = sscanf(argv[2], "%"SCNi32, &offset);
                if(!rst) return;
            }
            else return;
        }
        else return;
        
        if(!offset) return;
        switch(axis)
        {
            case 0:
                UNUSED_RETURN_VALUE(app_usbd_hid_mouse_x_move(&m_app_hid_mouse, offset));
                break;
            case 1:
                UNUSED_RETURN_VALUE(app_usbd_hid_mouse_y_move(&m_app_hid_mouse, offset));
                break;
            case 2:
                UNUSED_RETURN_VALUE(app_usbd_hid_mouse_scroll_move(&m_app_hid_mouse, offset));
                break;
            default:
                break;
        }
        
        //start_timer();

}



NRF_CLI_CREATE_STATIC_SUBCMD_SET(m_sub_mouse_s)
{
        NRF_CLI_CMD(b, NULL, "Mouse button", usr_cli_mouse_btn),
        NRF_CLI_CMD(m, NULL, "Mouse move", usr_cli_mouse_move),
        NRF_CLI_SUBCMD_SET_END
};
NRF_CLI_CMD_REGISTER(m, &m_sub_mouse_s, "mouse", NULL);

/** The following are for mouse movement */


