#include "usr_mouse.h"

/**
 * @brief Timer to repeat mouse move
 */
APP_TIMER_DEF(usr_mouse_timer);

usr_mouse_t usr_mouse;

static nrf_saadc_value_t usr_mouse_adc_buffer[2];
static uint8_t usr_mouse_adc_counter = 0;
uint16_t usr_mouse_adc_rst[2][USR_MOUSE_ADC_SAMPLE_NUM];
int16_t usr_mouse_dis_rst[2];

void usr_mouse_init(app_usbd_hid_mouse_t const * p_mouse)
{
    ret_code_t err_code;

    if(p_mouse == NULL)
      return;

    usr_mouse.p_mouse = p_mouse;

    // Init ADC
    usr_mouse_saadc_init();

    // Create timer
    err_code = app_timer_create(&usr_mouse_timer, APP_TIMER_MODE_REPEATED, usr_mouse_timer_handler);
    APP_ERROR_CHECK(err_code);

    // Start timer
    err_code = app_timer_start(usr_mouse_timer, APP_TIMER_TICKS(USR_MOUSE_TIMER_INTERVAL), NULL);
    APP_ERROR_CHECK(err_code);

}

void usr_mouse_saadc_init(void)
{
    ret_code_t err_code;

    // Configure SAADC
    nrf_saadc_channel_config_t channel_config_0 = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(USR_MOUSE_X_ADC_INPUT);
    nrf_saadc_channel_config_t channel_config_1 = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(USR_MOUSE_Y_ADC_INPUT);

    // Initialize SAADC
    err_code = nrf_drv_saadc_init(NULL, usr_mouse_saadc_callback);
    APP_ERROR_CHECK(err_code);

    // Initialize SAADC channel 0
    err_code = nrf_drv_saadc_channel_init(0, &channel_config_0);
    APP_ERROR_CHECK(err_code);

    // Initialize SAADC channel 1
    err_code = nrf_drv_saadc_channel_init(1, &channel_config_1);
    APP_ERROR_CHECK(err_code);

    // Set oversamping to average of 8 samples
    //nrf_saadc_oversample_set(NRF_SAADC_OVERSAMPLE_8X);

    // Allocate the buffer for SAADC
    err_code = nrf_drv_saadc_buffer_convert(usr_mouse_adc_buffer, 2);
    APP_ERROR_CHECK(err_code);

    // Start sampling
    err_code = nrf_drv_saadc_sample();
    APP_ERROR_CHECK(err_code);
}

void usr_mouse_saadc_callback(nrf_drv_saadc_evt_t const * p_event)
{
    ret_code_t err_code;
    UNUSED_VARIABLE(err_code);
    
    if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
    {
        usr_mouse_adc_rst[0][usr_mouse_adc_counter] = usr_mouse_adc_buffer[0];
        usr_mouse_adc_rst[1][usr_mouse_adc_counter] = usr_mouse_adc_buffer[1];
        usr_mouse_adc_counter++;
        if (usr_mouse_adc_counter >= USR_MOUSE_ADC_SAMPLE_NUM)
        {
            usr_mouse_adc_counter = 0;
            // app_sched_event_put(NULL, 0, usr_mouse_timer_handler);
        }
        //
        //NRF_LOG_INFO("Callback");
        // Optionally, start another sampling here
        ret_code_t err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, 2);
        APP_ERROR_CHECK(err_code);
        // Start sampling
        err_code = nrf_drv_saadc_sample();
        APP_ERROR_CHECK(err_code);
    }
}

void usr_mouse_timer_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);

    ret_code_t err_code;
    UNUSED_VARIABLE(err_code);

    // Process the previous result
    memset(usr_mouse_dis_rst, 0, sizeof(usr_mouse_dis_rst));
    for (int i = 0; i < USR_MOUSE_ADC_SAMPLE_NUM; i++)
    {
        usr_mouse_dis_rst[0] += ((int16_t)usr_mouse_adc_rst[0][i] - USR_MOUSE_ADC_OFFSET);
        usr_mouse_dis_rst[1] += ((int16_t)usr_mouse_adc_rst[1][i] - USR_MOUSE_ADC_OFFSET);
    }

    // Set margin
    if(!((usr_mouse_dis_rst[0]>USR_MOUSE_SIGNAL_MARGIN)|| \
        (usr_mouse_dis_rst[0]<-USR_MOUSE_SIGNAL_MARGIN)))
    {
        usr_mouse_dis_rst[0] = 0;
    }
    if(!((usr_mouse_dis_rst[1]>USR_MOUSE_SIGNAL_MARGIN)|| \
        (usr_mouse_dis_rst[1]<-USR_MOUSE_SIGNAL_MARGIN)))
    {
        usr_mouse_dis_rst[1] = 0;
    }

    // Round to infinite
    usr_mouse_dis_rst[0] += USR_MOUSE_SIGNAL_SCALE >> 1;
    usr_mouse_dis_rst[1] += USR_MOUSE_SIGNAL_SCALE >> 1;

    // Division
    usr_mouse_dis_rst[0] /= USR_MOUSE_SIGNAL_SCALE;
    usr_mouse_dis_rst[1] /= USR_MOUSE_SIGNAL_SCALE;

    // Average
    usr_mouse_dis_rst[0] /= USR_MOUSE_ADC_SAMPLE_NUM;
    usr_mouse_dis_rst[1] /= USR_MOUSE_ADC_SAMPLE_NUM;

    //NRF_LOG_INFO("Mouse Move: X, %4d; Y, %4d", usr_mouse_dis_rst[0], usr_mouse_dis_rst[1]);
    //NRF_LOG_INFO("ADC Value:  x, %4d; y, %4d.",usr_mouse_adc_buffer[0],usr_mouse_adc_buffer[1]);

    
    UNUSED_RETURN_VALUE(app_usbd_hid_mouse_x_move(usr_mouse.p_mouse, usr_mouse_dis_rst[0]));
    UNUSED_RETURN_VALUE(app_usbd_hid_mouse_y_move(usr_mouse.p_mouse, usr_mouse_dis_rst[1]));
   
}
