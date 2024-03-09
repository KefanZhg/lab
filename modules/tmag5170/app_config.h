#ifndef APP_CONFIG_H
#define APP_CONFIG_H


#define SPI_MISO_PIN 40
#define SPI_MOSI_PIN 39
#define SPI_SCK_PIN  38
#define SPI_SS_PIN   37

#define TMAG5170_ALT_PIN 42
#define TMAG5170_CS_PIN  43

#define HIGH 1
#define LOW  0

#define delay_us(n) nrf_delay_us(n)
#define delay_ms(n) nrf_delay_ms(n)

#define setCS(n)             nrf_gpio_pin_write(TMAG5170_CS_PIN, n)
#define setALERT(n)          nrf_gpio_pin_write(TMAG5170_ALT_PIN, n)

#define spiSendReceiveArrays spi_read_write

#endif // APP_CONFIG_H
