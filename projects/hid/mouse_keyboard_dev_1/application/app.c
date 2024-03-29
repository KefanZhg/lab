#include "app.h"

void app_init(void)
{
    // Init peripherals


    // Init modules


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
    for(;;)
    {
        // Sleep 1 second
        vTaskDelay(pdMS_TO_TICKS(1000));
        // Print time
        NRF_LOG_DEBUG("Time: %d", xTaskGetTickCount());
    }
}
