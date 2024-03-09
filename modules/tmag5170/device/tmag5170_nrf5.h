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

void spi_init(void);
void spi_uninit(void);
void spi_read_write(uint8_t *tx_buf, uint8_t *rx_buf, uint8_t length);

#define TEST_STRING "Nordic"
extern uint8_t       m_tx_buf[];           /**< TX buffer. */
extern uint8_t       m_rx_buf[];    /**< RX buffer. */
extern const uint8_t m_length;        /**< Transfer length. */

void tmag5170_init(void);
void tmag5170_read_xyz(int16_t *x, int16_t *y, int16_t *z);

#endif // TMAG5170_NRF5_H
