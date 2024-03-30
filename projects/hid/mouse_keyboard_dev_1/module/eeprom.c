#include "eeprom.h"
#include "string.h"

void eeprom_write(uint16_t address, uint8_t * data, uint8_t size)
{
    address = (address << 8) | (address >> 8);
    i2c_write_reg(&app_i2c, EEPROM_ADDRESS, address, sizeof(address), data, size);
}

void eeprom_read(uint16_t address, uint8_t * data, uint8_t size)
{
    address = (address << 8) | (address >> 8);
    i2c_read_reg_block(&app_i2c, EEPROM_ADDRESS, address, sizeof(address), data, size);
}
