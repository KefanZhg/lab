#include "i2c.h"
#include "nrf_log.h"

void i2c_handler(nrf_drv_twi_evt_t const *p_event, void *p_context)
{
    i2c_t *i2c = (i2c_t *)p_context;
    UNUSED_VARIABLE(i2c);

    // NRF_LOG_DEBUG("I2C event: %d", p_event->type);
    // NRF_LOG_DEBUG("I2C address: %p", p_context);

    switch (p_event->type)
    {
    case NRF_DRV_TWI_EVT_DONE:
        i2c_unlock(i2c);
        if(i2c->to_free != NULL)
        {
            vPortFree(i2c->to_free);
            i2c->to_free = NULL;
        }
        break;
    case NRF_DRV_TWI_EVT_ADDRESS_NACK:
        i2c_unlock(i2c);
        if(i2c->to_free != NULL)
        {
            vPortFree(i2c->to_free);
            i2c->to_free = NULL;
        }
    case NRF_DRV_TWI_EVT_DATA_NACK:
        i2c_unlock(i2c);
        if(i2c->to_free != NULL)
        {
            vPortFree(i2c->to_free);
            i2c->to_free = NULL;
        }
    default:
        break;
    }
}

void i2c_init(i2c_t *i2c, uint8_t sda_pin, uint8_t scl_pin, uint32_t frequency)
{
    ret_code_t err_code;
    UNUSED_VARIABLE(err_code);

    // Init peripherals
    const nrf_drv_twi_config_t twi_config = {
       .scl                = scl_pin,
       .sda                = sda_pin,
       .frequency          = NRF_DRV_TWI_FREQ_400K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .clear_bus_init     = false
    };
    NRF_LOG_DEBUG("I2C address: %p", i2c);

    err_code = nrf_drv_twi_init(&i2c->twi, &twi_config, i2c_handler, i2c);
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&i2c->twi);

    i2c->mutex = xSemaphoreCreateMutex();
    if (i2c->mutex == NULL)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }
}

void i2c_write(i2c_t *i2c, uint8_t address, uint8_t *data, uint8_t size, bool to_free)
{
    if(i2c_lock(i2c))
    {
        i2c->to_free = to_free ? data : NULL;
        APP_ERROR_CHECK(nrf_drv_twi_tx(&i2c->twi, address, data, size, false));
    }
    else
    {
        if(to_free)
        {
            vPortFree(data);
        }
    }
}

void i2c_read(i2c_t *i2c, uint8_t address, uint8_t *data, uint8_t size)
{
    if(i2c_lock(i2c))
    {
        APP_ERROR_CHECK(nrf_drv_twi_rx(&i2c->twi, address, data, size));
    }
}

bool i2c_lock(i2c_t *i2c)
{
    if (i2c->mutex == NULL)
    {
        return true;
    }
    if (xSemaphoreTake(i2c->mutex, portMAX_DELAY) != pdTRUE)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
        return false;
    }
    return true;
}

bool i2c_unlock(i2c_t *i2c)
{
    if (i2c->mutex == NULL)
    {
        return true;
    }
    xSemaphoreGive(i2c->mutex);
    return true;
}

void i2c_write_reg(i2c_t *i2c, uint8_t address, uint32_t reg, uint8_t reg_size, uint8_t *data, uint8_t size)
{
    // uint8_t buffer[reg_size + size];
    uint8_t * buffer = pvPortMalloc(reg_size + size);
    if(buffer == NULL)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
        return;
    }
    memcpy(buffer, &reg, reg_size);
    memcpy(buffer + reg_size, data, size);

    i2c_write(i2c, address, buffer, reg_size + size, true);
}

void i2c_read_reg(i2c_t *i2c, uint8_t address, uint32_t reg, uint8_t reg_size, uint8_t *data, uint8_t size)
{
    i2c_write(i2c, address, (uint8_t *)&reg, reg_size, false);
    i2c_read(i2c, address, data, size);
}

void i2c_read_reg_block(i2c_t *i2c, uint8_t address, uint32_t reg, uint8_t reg_size, uint8_t *data, uint8_t size)
{
    // NRF_LOG_DEBUG("i2c_read_reg_block: address=0x%02X, reg=0x%04X, reg_size=%d, size=%d", address, reg, reg_size, size);
    i2c_write(i2c, address, (uint8_t *)&reg, reg_size, false);
    i2c_read(i2c, address, data, size);
    if(i2c_lock(i2c))
        i2c_unlock(i2c);
}