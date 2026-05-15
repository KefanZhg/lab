#include "spi.h"
#include "nrf_gpio.h"
#include "nrf_log.h"

void spi_handler(nrf_drv_spi_evt_t const * p_event, void * p_context)
{
    spi_t *spi = (spi_t *)p_context;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (p_event->type == NRF_DRV_SPI_EVENT_DONE)
    {
        // NRF_LOG_INFO("SPI event: %d", p_event->type);
        // Free the memory
        if (spi->to_free != NULL)
        {
            vPortFree(spi->to_free);
            spi->to_free = NULL;
        }
        // Unlock the chip select
        if (spi->cs_pin != NRF_DRV_SPI_PIN_NOT_USED)
        {
            nrf_gpio_pin_set(spi->cs_pin);
            spi->cs_pin = NRF_DRV_SPI_PIN_NOT_USED;
        }
        // Unlock the mutex
        xSemaphoreGiveFromISR(spi->mutex, &xHigherPriorityTaskWoken);
    }
}

void spi_init(spi_t *spi, uint8_t mosi_pin, uint8_t miso_pin, uint8_t sck_pin, uint8_t frequency)
{
    nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
    spi_config.ss_pin = NRF_DRV_SPI_PIN_NOT_USED;
    spi_config.miso_pin = miso_pin;
    spi_config.mosi_pin = mosi_pin;
    spi_config.sck_pin = sck_pin;
    spi_config.frequency = frequency;
    spi_config.mode = NRF_DRV_SPI_MODE_0;
    spi_config.bit_order = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST;
    spi_config.irq_priority = APP_IRQ_PRIORITY_LOW;
    spi_config.orc = 0xFF;
    APP_ERROR_CHECK(nrf_drv_spi_init(&spi->spi, &spi_config, spi_handler, (void*)spi));
    spi->mutex = xSemaphoreCreateMutex();
    spi->to_free = NULL;
    spi->cs_pin = NRF_DRV_SPI_PIN_NOT_USED;
}

void spi_read_write(spi_t *spi, uint8_t *tx_buf, uint8_t *rx_buf, uint8_t length, bool to_free, uint8_t cs_pin)
{
    if (cs_pin != NRF_DRV_SPI_PIN_NOT_USED)
    {
        nrf_gpio_pin_clear(cs_pin);
        spi->cs_pin = cs_pin;
    }
    if (spi_lock(spi))
    {
        APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi->spi, tx_buf, length, rx_buf, length));
        if (to_free)
        {
            spi->to_free = tx_buf;
        }
    }
    else
    {
        NRF_LOG_ERROR("SPI mutex lock failed.");
        nrf_gpio_pin_set(cs_pin);
    }
}

bool spi_lock(spi_t *spi)
{
    if (spi->mutex == NULL)
    {
        return true;
    }
    if (xSemaphoreTake(spi->mutex, portMAX_DELAY) != pdTRUE)
    {
        NRF_LOG_ERROR("SPI mutex take failed.");
        return false;
    }
    return true;
}

bool spi_unlock(spi_t *spi)
{
    if (spi->mutex == NULL)
    {
        return true;
    }
    if (xSemaphoreGive(spi->mutex) != pdTRUE)
    {
        NRF_LOG_ERROR("SPI mutex give failed.");
        return false;
    }
    return true;
}

