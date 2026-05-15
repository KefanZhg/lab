#ifndef EEPROM_H
#define EEPROM_H

#include "i2c.h"

typedef struct eeprom_s
{
    i2c_t * i2c;
    uint8_t addr;
} eeprom_t;

void eeprom_init(eeprom_t * eeprom, i2c_t * i2c, uint8_t addr);
void eeprom_write(eeprom_t * eeprom, uint16_t address, uint8_t * data, uint8_t size);
void eeprom_read(eeprom_t * eeprom, uint16_t address, uint8_t * data, uint8_t size);

#endif // EEPROM_H
