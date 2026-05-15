#ifndef I2C_H
#define I2C_H

#include "nrf_drv_twi.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "string.h"

typedef struct i2c_s
{
    const nrf_drv_twi_t twi;

    SemaphoreHandle_t mutex;

    void * to_free;

} i2c_t;

void i2c_init(i2c_t *i2c, uint8_t sda_pin, uint8_t scl_pin, uint32_t frequency);
void i2c_write(i2c_t *i2c, uint8_t address, uint8_t *data, uint8_t size, bool to_free);
void i2c_read(i2c_t *i2c, uint8_t address, uint8_t *data, uint8_t size);

bool i2c_lock(i2c_t *i2c);
bool i2c_unlock(i2c_t *i2c);

void i2c_write_reg(i2c_t *i2c, uint8_t address, uint32_t reg, uint8_t reg_size, uint8_t *data, uint8_t size);
void i2c_read_reg(i2c_t *i2c, uint8_t address, uint32_t reg, uint8_t reg_size, uint8_t *data, uint8_t size);

void i2c_read_reg_block(i2c_t *i2c, uint8_t address, uint32_t reg, uint8_t reg_size, uint8_t *data, uint8_t size);

#endif // I2C_H