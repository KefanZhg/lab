#ifndef CONFIG_H
#define CONFIG_H

// File includes
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "nordic_common.h"

#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "ble_nus.h"
#include "bsp_btn_ble.h"

#include "nrf.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_freertos.h"

#include "nrf_ble_gatt.h"

#include "nrf_drv_usbd.h"
#include "nrf_drv_clock.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_drv_power.h"
#include "nrf_drv_twi.h"
#include "nrf_drv_spi.h"

#include "boards.h"

#include "app_error.h"
#include "app_timer.h"
#include "app_uart.h"
#include "app_util.h"
#include "app_util_platform.h"
#include "app_usbd_core.h"
#include "app_usbd.h"
#include "app_usbd_string_desc.h"
#include "app_usbd_cdc_acm.h"
#include "app_usbd_serial_num.h"

/* FreeRTOS related */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// Pin definitions
#define LED_PIN 46
#define BTN_PIN 8

#define I2C_SDA_PIN  17
#define I2C_SCL_PIN  13

#define SPI_SCK_PIN  38
#define SPI_MOSI_PIN 36
#define SPI_MISO_PIN 34

#define TMAG5170_NUM_SENSORS 5
#define TMAG5170_CS_PINS {32, 22, 24, 9, 10}

#define JOYSTICK_SW_PIN 16
#define JOYSTICK_X_AIN   6
#define JOYSTICK_Y_AIN   4

#endif // CONFIG_H