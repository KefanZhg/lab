#ifndef APP_H
#define APP_H

#include "conf.h"

// Include modules
#include "eeprom.h"

void app_init(void);

#define APP_TASK_STACK_SIZE 512
#define APP_TASK_PRIORITY 1
extern TaskHandle_t app_task_handle;
void app_thread(void * arg);

#endif // APP_H