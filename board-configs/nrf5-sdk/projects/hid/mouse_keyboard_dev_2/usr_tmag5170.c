#include "usr_tmag5170.h"

#define TMAG5170_COUNT 6
#define SPI_INSTANCE  0

#define TMAG5170_CS_PIN {32, 22, 24, 9, 10, 25}

const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);

