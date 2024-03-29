#include "eeprom.h"
#include "string.h"



void eeprom_write(uint16_t address, uint8_t * data, uint8_t size)
{
    ret_code_t err_code;
    UNUSED_PARAMETER(err_code);

    uint8_t buffer[size + 2];
    buffer[0] = (address >> 8) & 0xFF;
    buffer[1] = address & 0xFF;
    memcpy(&buffer[2], data, size);

    // err_code = nrf_drv_twi_tx(&app_i2c.twi, EEPROM_ADDRESS, buffer, size + 2, false);
    // APP_ERROR_CHECK(err_code);
    i2c_write(&app_i2c, EEPROM_ADDRESS, buffer, size + 2);
}

void eeprom_read(uint16_t address, uint8_t * data, uint8_t size)
{
    ret_code_t err_code;
    UNUSED_PARAMETER(err_code);
    uint8_t addr[2] = { (address >> 8) & 0xFF, address & 0xFF };

    // err_code = nrf_drv_twi_tx(&app_twi, EEPROM_ADDRESS, addr, 2, false);
    // APP_ERROR_CHECK(err_code);
    // err_code = nrf_drv_twi_rx(&app_twi, EEPROM_ADDRESS, data, size);
    // APP_ERROR_CHECK(err_code);
    i2c_write(&app_i2c, EEPROM_ADDRESS, addr, 2);
    i2c_read(&app_i2c, EEPROM_ADDRESS, data, size);
}
