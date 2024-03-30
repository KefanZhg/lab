#include "eeprom.h"
#include "string.h"

void eeprom_init(eeprom_t * eeprom, i2c_t * i2c, uint8_t addr)
{
    eeprom->i2c = i2c;
    eeprom->addr = addr;
}

/*
 * @note  Wait 10 ms after write
*/
void eeprom_write(eeprom_t * eeprom, uint16_t address, uint8_t * data, uint8_t size)
{
    address = (address << 8) | (address >> 8);
    i2c_write_reg(eeprom->i2c, eeprom->addr, address, sizeof(address), data, size);
}

void eeprom_read(eeprom_t * eeprom, uint16_t address, uint8_t * data, uint8_t size)
{
    address = (address << 8) | (address >> 8);
    i2c_read_reg_block(eeprom->i2c, eeprom->addr, address, sizeof(address), data, size);
}
