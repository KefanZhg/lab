#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define NRF_LOG_BACKEND_UART_TX_PIN 26
#define NRF_LOG_BACKEND_UART_RX_PIN 6

#define LED_PIN 46
#define BTN_PIN 8

#define SPI_SCK_PIN  38
#define SPI_MOSI_PIN 36
#define SPI_MISO_PIN 34
#define SPI_SS_PIN   32

#define TMAG5170_ALT_PIN 21
#define TMAG5170_CS_PIN  SPI_SS_PIN

#define HIGH 1
#define LOW  0

#define delay_us(n) nrf_delay_us(n)
#define delay_ms(n) nrf_delay_ms(n)

#define setCS(n)             nrf_gpio_pin_write(TMAG5170_CS_PIN, n)
#define setALERT(n)          nrf_gpio_pin_write(TMAG5170_ALT_PIN, n)

#define spiSendReceiveArrays spi_read_write

#endif // APP_CONFIG_H
