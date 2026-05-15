#ifndef TMAG5170_NRF5_H
#define TMAG5170_NRF5_H

#include "nrf_drv_spi.h"
#include "app_util_platform.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "boards.h"
#include "app_error.h"
#include <string.h>
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "tmag5170.h"
#include "app_config.h"

typedef struct data_3d_int16_s {
    uint64_t timestamp;
    int16_t x;
    int16_t y;
    int16_t z;
} data_3d_int16_t;

typedef struct data_3d_int32_s {
    uint64_t timestamp;
    int32_t x;
    int32_t y;
    int32_t z;
} data_3d_int32_t;

typedef struct tmag5170_s
{
    const nrf_drv_spi_t * p_spi;
    uint32_t cs_pin;
    uint32_t alert_pin;
    data_3d_int16_t data;
} tmag5170_t;

#define TMAG5170_DEFAULT_CONFIG \
{                               \
    .p_spi = NULL,              \
    .cs_pin = NRF_DRV_SPI_PIN_NOT_USED,       \
    .alert_pin = NRF_DRV_SPI_PIN_NOT_USED,    \
    .data = {0, 0, 0}           \
}

void spi_init(void);
void spi_uninit(void);
void spi_read_write(uint8_t *tx_buf, uint8_t *rx_buf, uint8_t length);

#define TEST_STRING "Nordic"
extern uint8_t       m_tx_buf[];           /**< TX buffer. */
extern uint8_t       m_rx_buf[];    /**< RX buffer. */
extern const uint8_t m_length;        /**< Transfer length. */

void tmag5170_init(tmag5170_t * p_tmag5170);
void tmag5170_read_xyz(tmag5170_t * p_tmag5170);


extern volatile tmag5170_t * p_tmag5170_current;

#define spiSendReceiveArrays(a,b,c) \
do{ \
    nrf_gpio_pin_clear(p_tmag5170_current->cs_pin); \
    spi_read_write(a, b, c); \
    nrf_gpio_pin_set(p_tmag5170_current->cs_pin); \
} while(0)



#endif // TMAG5170_NRF5_H
