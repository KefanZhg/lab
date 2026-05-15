#include "tmag5170_nrf5.h"

volatile tmag5170_t * p_tmag5170_current;

extern void spi_event_handler(nrf_drv_spi_evt_t const * p_event,
                       void *                    p_context);

void tmag5170_init(tmag5170_t * p_tmag5170)
{
    uint8_t input;
    UNUSED_VARIABLE(input);
    /* Check pointer */
    if(p_tmag5170 == NULL || p_tmag5170->p_spi == NULL)
    {
        return;
    }
    p_tmag5170_current = p_tmag5170;
    /* Init GPIO */
    if(p_tmag5170->cs_pin != NRF_DRV_SPI_PIN_NOT_USED)
    {
        nrf_gpio_cfg_output(p_tmag5170->cs_pin);
        nrf_gpio_pin_write(p_tmag5170->cs_pin, HIGH);
    }
    NRF_LOG_INFO("CS pin: %d", p_tmag5170->cs_pin);
    NRF_LOG_INFO("Alert pin: %d", p_tmag5170->alert_pin);
    NRF_LOG_FLUSH();

    if(p_tmag5170->alert_pin != NRF_DRV_SPI_PIN_NOT_USED)
    {
        nrf_gpio_cfg_input(p_tmag5170->alert_pin, NRF_GPIO_PIN_PULLUP);
        // nrf_gpio_pin_write(p_tmag5170->alert_pin, HIGH);
        // Read pin
        input = nrf_gpio_pin_read(p_tmag5170->alert_pin);
        NRF_LOG_INFO("Alert level: %d", input);
    }
    NRF_LOG_FLUSH();
    

    /* Init SPI */
    ret_code_t err_code;
    nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
    // spi_config.ss_pin   = SPI_SS_PIN;
    spi_config.miso_pin = SPI_MISO_PIN;
    spi_config.mosi_pin = SPI_MOSI_PIN;
    spi_config.sck_pin  = SPI_SCK_PIN;

    err_code = nrf_drv_spi_init(p_tmag5170->p_spi, &spi_config, spi_event_handler, NULL);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("SPI initialization failed.");
        NRF_LOG_ERROR("Error code: %d", err_code);
        // return;
    }
    else
    {
        NRF_LOG_INFO("SPI initialization succeeded.");
    }
    

    // Enable Timer

    // Activate CS
    nrf_gpio_pin_write(p_tmag5170->cs_pin, LOW);

    // Run startup
    TMAG5170startup();

    // Start sampling
    setSampleRate(0x05);
    enableMagChannels(MAG_CH_EN_BITS_XYZ);

    // Set range
    setRanges(0x01, 0x01, 0x01);

    setSLEEPTIME(0x04);
    enterWakeUpAndSleepMode();

    // Deactivate CS
    nrf_gpio_pin_write(p_tmag5170->cs_pin, HIGH);
}

void tmag5170_read_xyz(tmag5170_t * p_tmag5170)
{
    /* Check pointer */
    if(p_tmag5170 == NULL || p_tmag5170->p_spi == NULL)
    {
        return;
    }

    p_tmag5170_current = p_tmag5170;
    data_3d_int16_t * p_data = &p_tmag5170->data;
    // Read the sensor
    p_data->timestamp = 0;
    p_data->x = getXresult();
    p_data->y = getYresult();
    p_data->z = getZresult();

    // Print results
    // NRF_LOG_INFO("X: %d, Y: %d, Z: %d", p_data->x, p_data->y, p_data->z);
    // NRF_LOG_FLUSH();
    // Deactivate CS
}
