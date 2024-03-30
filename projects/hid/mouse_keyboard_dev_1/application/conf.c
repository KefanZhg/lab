#include "conf.h"

// Peripheral handles
// i2c_t app_i2c = {
//     .twi = NRF_DRV_TWI_INSTANCE(APP_TWI_INSTANCE_ID),
// };
const nrf_drv_twi_t app_twi = NRF_DRV_TWI_INSTANCE(APP_TWI_INSTANCE_ID);
i2c_t app_i2c = {
    .twi = NRF_DRV_TWI_INSTANCE(APP_TWI_INSTANCE_ID),
    .mutex = NULL,
};

eeprom_t eeprom;
