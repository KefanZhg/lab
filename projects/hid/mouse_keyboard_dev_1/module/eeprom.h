#ifndef EEPROM_H
#define EEPROM_H

#include "config.h"

// Assuming twi_instance is already initialized nrf_drv_twi instance
extern const nrf_drv_twi_t m_twi;

#define EEPROM_ADDRESS (0x50) // The I2C address of the AT24C256, might need adjustment based on the hardware setup

void eeprom_init(void);
void eeprom_write(uint16_t address, uint8_t * data, uint8_t size);
void eeprom_read(uint16_t address, uint8_t * data, uint8_t size);

#endif // EEPROM_H
