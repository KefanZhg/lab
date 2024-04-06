#include "usr_app.h"

static uint8_t usr_app_master = 0;
static uint8_t usr_joystick_btn_id = 0;

void usr_app_init(uint8_t master, app_usbd_hid_mouse_t const * p_mouse)
{
  usr_app_master = master;
  usr_joystick_btn_id = master ? 0 : 1; // If master, left hand, otherwise, right hand

  if (master)
  {
    NRF_LOG_INFO("Running as master!");
  }
  else
  {
    NRF_LOG_INFO("Running as slave!");
  }

  usr_mouse_init(p_mouse);
  usr_btn_init();
  usr_select_init();
  return;
}

void usr_app_run(void)
{
  return;
}

void usr_btn_init(void)
{
    uint32_t err_code;
    
    // Button configuration structure
    static app_button_cfg_t buttons[] =
    {
        {USR_JOYSTICK_BTN_PIN, false, NRF_GPIO_PIN_PULLUP, usr_btn_event_callback},
    };
    
    // Initialize the app_button module
    err_code = app_button_init(buttons, ARRAY_SIZE(buttons), APP_TIMER_TICKS(50));
    APP_ERROR_CHECK(err_code);
    
    // Enable button detection
    err_code = app_button_enable();
    APP_ERROR_CHECK(err_code);

}

void usr_btn_event_callback(uint8_t pin_no, uint8_t button_action)
{
    if (pin_no == USR_JOYSTICK_BTN_PIN && button_action == APP_BUTTON_PUSH)
    {
            UNUSED_RETURN_VALUE(app_usbd_hid_mouse_button_state(usr_mouse.p_mouse, usr_joystick_btn_id, true));
    }
    else if (pin_no == USR_JOYSTICK_BTN_PIN && button_action == APP_BUTTON_RELEASE)
    {
            UNUSED_RETURN_VALUE(app_usbd_hid_mouse_button_state(usr_mouse.p_mouse, usr_joystick_btn_id, false));
    }
}
