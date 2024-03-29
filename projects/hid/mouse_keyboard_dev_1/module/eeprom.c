#include "eeprom.h"
#include "string.h"


void eeprom_init(void)
{
    ret_code_t err_code;

    const nrf_drv_twi_config_t twi_config = {
       .scl                = I2C_SCL_PIN,
       .sda                = I2C_SDA_PIN,
       .frequency          = NRF_DRV_TWI_FREQ_100K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .clear_bus_init     = false
    };

    err_code = nrf_drv_twi_init(&m_twi, &twi_config, NULL, NULL);
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&m_twi);
}

void eeprom_write(uint16_t address, uint8_t * data, uint8_t size)
{
    ret_code_t err_code;

    uint8_t buffer[size + 2];
    buffer[0] = (address >> 8) & 0xFF;
    buffer[1] = address & 0xFF;
    memcpy(&buffer[2], data, size);

    err_code = nrf_drv_twi_tx(&m_twi, EEPROM_ADDRESS, buffer, size + 2, false);
    APP_ERROR_CHECK(err_code);
}

void eeprom_read(uint16_t address, uint8_t * data, uint8_t size)
{
    ret_code_t err_code;
    uint8_t addr[2] = { (address >> 8) & 0xFF, address & 0xFF };

    err_code = nrf_drv_twi_tx(&m_twi, EEPROM_ADDRESS, addr, 2, false);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_twi_rx(&m_twi, EEPROM_ADDRESS, data, size);
    APP_ERROR_CHECK(err_code);
}
