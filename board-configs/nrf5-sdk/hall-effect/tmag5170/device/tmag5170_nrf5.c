#include "tmag5170_nrf5.h"

#define SPI_INSTANCE  0 /**< SPI instance index. */
static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);  /**< SPI instance. */
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

void spi_init(void)
{
    nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
    spi_config.ss_pin   = SPI_SS_PIN;
    spi_config.miso_pin = SPI_MISO_PIN;
    spi_config.mosi_pin = SPI_MOSI_PIN;
    spi_config.sck_pin  = SPI_SCK_PIN;
    APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, spi_event_handler, NULL));
    
    NRF_LOG_INFO("SPI example started.");
}

void spi_uninit(void)
{
    nrf_drv_spi_uninit(&spi);
}

void spi_read_write(uint8_t *tx_buf, uint8_t *rx_buf, uint8_t length)
{
    spi_xfer_done = false;
    APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, tx_buf, length, rx_buf, length));
    while (!spi_xfer_done)
    {
        __WFE();
    }
}


void tmag5170_init(void)
{
    // Init GPIO
    nrf_gpio_cfg_output(TMAG5170_CS_PIN);
    nrf_gpio_pin_write(TMAG5170_CS_PIN, HIGH);

    nrf_gpio_cfg_output(TMAG5170_ALT_PIN);
    nrf_gpio_pin_write(TMAG5170_ALT_PIN, HIGH);

    // INIT SPI
    spi_init();

    // Enable Timer

    // Run startup
    TMAG5170startup();

    // Start sampling
    setSampleRate(0x02);
    enableMagChannels(MAG_CH_EN_BITS_XYZ);
    setSLEEPTIME(0x00);
    enterWakeUpAndSleepMode();
}

void tmag5170_read_xyz(int16_t *x, int16_t *y, int16_t *z)
{
    // Trigger a conversion
    // setALERT(0);
    // delay_us(30);
    // setALERT(1);
    // Read the sensor
    if(x!=NULL) *x = ((int16_t)getXresult());
    if(y!=NULL) *y = ((int16_t)getYresult());
    if(z!=NULL) *z = ((int16_t)getZresult());
}
