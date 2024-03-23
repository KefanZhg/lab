/**
 * Copyright (c) 2015 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
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
#include "tmag5170_nrf5.h"

#define SPI_INSTANCE  0 /**< SPI instance index. */
static const nrf_drv_spi_t m_spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);  /**< SPI instance. */
static volatile bool spi_xfer_done;  /**< Flag used to indicate that SPI instance completed the transfer. */

uint8_t       m_tx_buf[] = TEST_STRING;           /**< TX buffer. */
uint8_t       m_rx_buf[sizeof(TEST_STRING) + 1];    /**< RX buffer. */
const uint8_t m_length = sizeof(m_tx_buf);        /**< Transfer length. */

/**
 * @brief SPI user event handler.
 * @param event
 */
void spi_event_handler(nrf_drv_spi_evt_t const * p_event,
                       void *                    p_context)
{
    spi_xfer_done = true;
    // NRF_LOG_INFO("Transfer completed.");
    if (m_rx_buf[0] != 0)
    {
        NRF_LOG_INFO(" Received:");
        NRF_LOG_HEXDUMP_INFO(m_rx_buf, strlen((const char *)m_rx_buf));
    }
}

void spi_read_write(uint8_t *tx_buf, uint8_t *rx_buf, uint8_t length)
{
    spi_xfer_done = false;
    APP_ERROR_CHECK(nrf_drv_spi_transfer(&m_spi, tx_buf, length, rx_buf, length));
    while (!spi_xfer_done)
    {
        __WFE();
    }
}

tmag5170_t tmag5170[TMAG5170_NUM_SENSORS] = {
    {
        .p_spi = &m_spi,
        .cs_pin = TMAG5170_CS_PIN_0,
        .alert_pin = TMAG5170_ALERT_PIN_0
    },
    {
        .p_spi = &m_spi,
        .cs_pin = TMAG5170_CS_PIN_1,
        .alert_pin = TMAG5170_ALERT_PIN_1
    },
    {
        .p_spi = &m_spi,
        .cs_pin = TMAG5170_CS_PIN_2,
        .alert_pin = TMAG5170_ALERT_PIN_2
    },
    {
        .p_spi = &m_spi,
        .cs_pin = TMAG5170_CS_PIN_3,
        .alert_pin = TMAG5170_ALERT_PIN_3
    
    }
};

data_3d_int32_t data_sum[TMAG5170_NUM_SENSORS] = {
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
};

data_3d_int32_t data_avg[TMAG5170_NUM_SENSORS] = {
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
};

uint32_t count = 0;
#define AVG_NUM 256

void continous_mode(void);
void alert_mode(void);

int main(void)
{
    nrf_gpio_cfg_output(LED_PIN);

    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    for (int i = 0; i < 4; i++)
    {

        tmag5170_init(&tmag5170[i]);
    }

    continous_mode();
    // alert_mode();

}

void continous_mode(void)
{
        while (1)
    {
        // Read the sensor
        NRF_LOG_INFO("Counter %lu.", count++);


        NRF_LOG_FLUSH();

        nrf_gpio_pin_toggle(LED_PIN);

        /* Clean sum */
        for (int i = 0; i < 4; i++)
        {
            data_sum[i].x = 0;
            data_sum[i].y = 0;
            data_sum[i].z = 0;
        }
        /* Read the sensor */
        for (int j = 0; j < AVG_NUM; j++)
            for (int i = 0; i < 4; i++)
            {
                // Wait alert
                // while(nrf_gpio_pin_read(tmag5170[i].alert_pin) == 1)
                // {
                //     nrf_delay_us(10);
                // }
                // if (nrf_gpio_pin_read(tmag5170[i].alert_pin) == 0)
                {
                    tmag5170_read_xyz(&tmag5170[i]);
                    data_sum[i].x += tmag5170[i].data.x;
                    data_sum[i].y += tmag5170[i].data.y;
                    data_sum[i].z += tmag5170[i].data.z;
                }
            }
        /* Average the data */
        for (int i = 0; i < 4; i++)
        {
            data_avg[i].x = data_sum[i].x / AVG_NUM;
            data_avg[i].y = data_sum[i].y / AVG_NUM;
            data_avg[i].z = data_sum[i].z / AVG_NUM;
        }
        /* Print results to a table */
        NRF_LOG_INFO("Sensor  | X       | Y       | Z       |");
        NRF_LOG_INFO("--------|---------|---------|---------|");
        for (int i = 0; i < 4; i++)
        {
            NRF_LOG_INFO("Sensor %d|%8d |%8d |%8d |", i, data_avg[i].x, data_avg[i].y, data_avg[i].z);
        }
        NRF_LOG_FLUSH();

    }
}

void alert_mode(void)
{
    uint16_t input;
    uint16_t thresh = TMAG5170_HI_THRESH | TMAG5170_LO_THRESH;
    NRF_LOG_INFO("Alert number 0x%04X", thresh);
    NRF_LOG_FLUSH();
    // Enable alert mode
    for (int i = 0; i < 4; i++)
    {    
        p_tmag5170_current = &tmag5170[i];
        // SET ALERT_MODE (address: 0x03C) to Interrupt & Trigger Mode (0h)
        input = normalReadRegister(ALERT_CONFIG_ADDRESS);
        input |= (ALERT_CONFIG_ALERT_MODE_MASK);
        input |= ALERT_CONFIG_X_THRX_ALRT_MASK;
        // input |= ALERT_CONFIG_Y_THRX_ALRT_MASK;
        // input |= ALERT_CONFIG_Z_THRX_ALRT_MASK;
        writeToRegister( ALERT_CONFIG_ADDRESS, input );
        NRF_LOG_INFO("Alert Input 0x%04X", input);
        writeToRegister(X_THRX_CONFIG_ADDRESS, thresh);
        // writeToRegister(Y_THRX_CONFIG_ADDRESS, thresh);
        // writeToRegister(Z_THRX_CONFIG_ADDRESS, thresh);



    }

    NRF_LOG_INFO("Alert mode enabled.");

    while(1)
    {
        count++;
        // Wait for alert
        for (int i = 0; i < 4; i++)
        {
            if (nrf_gpio_pin_read(tmag5170[i].alert_pin) == 0)
            {
                tmag5170_read_xyz(&tmag5170[i]);
                NRF_LOG_INFO("Sensor %d | X: %d, Y: %d, Z: %d", i, tmag5170[i].data.x, tmag5170[i].data.y, tmag5170[i].data.z);
                NRF_LOG_FLUSH();
            }
            else if(count % 100 == 0)
            {
                NRF_LOG_INFO("Sensor %d | No alert.", i);
                tmag5170_read_xyz(&tmag5170[i]);
                NRF_LOG_INFO("Sensor %d | X: %d, Y: %d, Z: %d", i, tmag5170[i].data.x, tmag5170[i].data.y, tmag5170[i].data.z);
                NRF_LOG_FLUSH();
            }
        }
        NRF_LOG_FLUSH();
        nrf_delay_ms(10);
    }
}
