#include "app.h"


void app_init(void)
{
    ret_code_t err_code;
    UNUSED_VARIABLE(err_code);

    // Init peripherals
    // const nrf_drv_twi_config_t twi_config = {
    //    .scl                = TWI_SCL_PIN,
    //    .sda                = TWI_SDA_PIN,
    //    .frequency          = NRF_DRV_TWI_FREQ_400K,
    //    .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
    //    .clear_bus_init     = false
    // };

    // err_code = nrf_drv_twi_init(&app_twi, &twi_config, NULL, NULL);
    // APP_ERROR_CHECK(err_code);

    // nrf_drv_twi_enable(&app_twi);
    i2c_init(&app_i2c, TWI_SDA_PIN, TWI_SCL_PIN, NRF_DRV_TWI_FREQ_100K);



    // Init modules
    eeprom_init(&eeprom, &app_i2c, EEPROM_ADDRESS);

    tmag5170s->cs_pin = 32;
    tmag5170_init(&tmag5170s[0]);


    // Init threads
    if (pdPASS != xTaskCreate(app_thread,
                              "APP",
                              APP_TASK_STACK_SIZE,
                              NULL,
                              APP_TASK_PRIORITY,
                              &app_task_handle))
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    // Init timers

}


// Application thread
// Only for testing purposes
TaskHandle_t app_task_handle;
void app_thread(void * arg)
{
    static uint32_t count = 0;
    static uint32_t read_count = 0;
    static uint32_t utmp;
    UNUSED_VARIABLE(count);
    UNUSED_VARIABLE(read_count);
    UNUSED_PARAMETER(arg);
    UNUSED_VARIABLE(utmp);
    // // // Read keymap
    // keymap_reset();
    // keymap_write(&eeprom, 0);
    // // Wait 10 ms
    // vTaskDelay(pdMS_TO_TICKS(100));
    keymap_read(&eeprom, 0);
    // Print keymap as a table
    keymap_print();
    for(;;)
    {
        // Sleep 1 second
        vTaskDelay(pdMS_TO_TICKS(900));
        // Print time
        NRF_LOG_DEBUG("Time: %d", xTaskGetTickCount());
    }
}
