#ifndef SPI_H
#define SPI_H

#include "nrf_drv_spi.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "string.h"
#include "data.h"

typedef struct spi_s
{
    const nrf_drv_spi_t spi;

    SemaphoreHandle_t mutex;

    void * to_free;
    uint8_t cs_pin;

} spi_t;

void spi_init(spi_t *spi, uint8_t mosi_pin, uint8_t miso_pin, uint8_t sck_pin, uint8_t frequency);
void spi_read_write(spi_t *spi, uint8_t *tx_buf, uint8_t *rx_buf, uint8_t length, bool to_free, uint8_t cs_pin);

bool spi_lock(spi_t *spi);
bool spi_unlock(spi_t *spi);



#endif // SPI_H
