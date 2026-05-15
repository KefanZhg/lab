#ifndef KEYMAP_H
#define KEYMAP_H

#include "conf.h"

// Currently no more than 127 keymaps are supported
// If exceeding this limit, we need to read/write i2c multiple times
#define KEYMAP_SIZE 127 
#define KEYMAP_BLOCK_SIZE ((uint16_t)256)

extern uint8_t keymap[KEYMAP_SIZE][2];

void keymap_read(eeprom_t * eeprom, uint8_t index);
void keymap_write(eeprom_t * eeprom, uint8_t index);
void keymap_print(void);
void keymap_reset(void);

#endif /* KEYMAP_H */
