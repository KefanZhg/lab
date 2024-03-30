#include "keymap.h"

uint8_t keymap[KEYMAP_SIZE][2];

void keymap_read(eeprom_t * eeprom, uint8_t index)
{
    eeprom_read(eeprom, KEYMAP_BLOCK_SIZE * index, &keymap[0][0], (uint8_t)sizeof(keymap));
    // TODO: process keymap
}

void keymap_write(eeprom_t * eeprom, uint8_t index)
{
    uint16_t addr = KEYMAP_BLOCK_SIZE * index;
    uint8_t len = sizeof(keymap);
    uint8_t * data = &keymap[0][0];
    uint8_t tmp;

    while (len > 0)
    {
        tmp = len>16?16:len;
        eeprom_write(eeprom, addr, data, tmp);
        addr += tmp;
        len -= tmp;
        data += tmp;
        // Wait 10 ms after write
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void keymap_print(void)
{
    char* msg = pvPortMalloc(100);
    uint8_t len;
    if (msg == NULL)
    {
        NRF_LOG_ERROR("Failed to allocate memory");
        return;
    }
    // Print keymap as table
    NRF_LOG_DEBUG("Keymap:");
    uint8_t i = 0;
    while(i<KEYMAP_SIZE)
    {
        len = 0;
        for(uint8_t j = 0; j<8; j++)
        {
            sprintf(&msg[len], "%3u|%3u ", keymap[i][0], keymap[i][1]);
            i++;
            len = strlen(msg);
            if(i >= KEYMAP_SIZE)
            {
                break;
            }
        }
        NRF_LOG_DEBUG("%s",msg);
        NRF_LOG_FLUSH();
    }
    vPortFree(msg);
}

void keymap_reset(void)
{
    uint8_t i = 0;
    for (uint8_t j = 0; j < KEYMAP_SIZE; j++)
    {
        keymap[j][0] = i;
        keymap[j][1] = KEYMAP_SIZE + i;
        i++;
    }
}