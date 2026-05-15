/**
 * \copyright Copyright (C) 2022 Texas Instruments Incorporated - http://www.ti.com/
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef TMAG5170_H_
#define TMAG5170_H_

// Standard libraries
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

// Custom libraries
#include "tmag5170_nrf5.h"

// Number of registers used by the TMAG5170
#define NUM_REGISTERS                              20

// Length of a SPI frame for the TMAG5170 in bytes (8 bits)
#define TMAG5170_FRAME_NUM_BYTES                    4

/*
 * Definitions for #ifdef calls
 *
 * Uncomment these definitions to enable specific functionalities
 * provided by the example code:
 *
 * RESET_DEVICE_IN_STARTUP         - Set all device registers to defaults when TMAG5170startup() is called instead of only the
 *                                   SYSTEM_CONFIG register.
 * DISABLE_CRC_IN_STARTUP          - Disables the CRC calculation and check in the code and, through TMAG5170startup(),
 *                                   on the device as well. (call TMAG5170startup first)
 * SEND_RECIEVE_REDUNDANCY_ENABLED - Reattempt an SPI command once more after a failed CRC verification
 * GLOBAL_CRC_ERROR_VAR_ENABLED    - create a global error variable named crc_error_occurence that will initialize to 0
 *                                   and flip to 1 upon CRC verification error
 * GLOBAL_INPUT_ERROR_VAR_ENABLED  - Create a global error variable named fcn_input_error_occurence that will initialize to 0
 *                                   and flip to 1 upon any example code function receiving an invalid input
 * MAX_DELAYS_IN_OPMODE_CHANGES    - Enables delay_us() calls in OPERATING_MODE change functions to account for the max possible
 *                                   delay (outside of deep sleep mode startup situations) the device can take when switching to
 *                                   the desired OPERATING_MODE (delays must be implemented if this is disabled)
 */

#define   RESET_DEVICE_ON_INITIALIZE

//#define   DISABLE_CRC_ON_INITIALIZE

//#define   SEND_RECIEVE_REDUNDANCY_ENABLED

//#define   GLOBAL_CRC_ERROR_VAR_ENABLED
#ifdef      GLOBAL_CRC_ERROR_VAR_ENABLED
            extern uint8_t crc_error_occurence;
#endif

//#define   GLOBAL_INPUT_ERROR_VAR_ENABLED
#ifdef      GLOBAL_INPUT_ERROR_VAR_ENABLED
            extern uint8_t fcn_input_error_occurence;
#endif

#define   MAX_DELAYS_IN_OPMODE_CHANGES


//****************************************************************************
//
// Function prototypes
//
//****************************************************************************

void TMAG5170startup();
void resetDevice();
uint16_t getSYSTEM_CONFIG_stored();
uint16_t getDATA_TYPE();


// SPI functions

void sendAndReceiveFrame(uint8_t dataTx[], uint8_t dataRx[]);

void writeToRegister(uint8_t address, uint16_t data_to_write);
void writeToRegisterWithCMD0(uint8_t address, uint16_t data_to_write);
uint16_t writeToRegisterWithSTAT(uint8_t address, uint16_t data_to_write, uint8_t cmd_bits);

void normalRead( uint16_t output[], uint8_t address, uint8_t cmd_bits );
uint16_t normalReadRegister( uint8_t address );
uint16_t normalReadRegisterWithCMD0( uint8_t address );
uint16_t normalReadSTAT( uint8_t cmd_bits );

void specialRead( uint16_t output[], uint8_t cmd_bits );
uint16_t specialReadSTAT( uint8_t cmd_bits );


// Change Device Operation Mode

void enterConfigurationMode();
void enterStandbyMode();
void enterActiveMeasureMode();
void enterActiveTriggerMode();
void enterSleepMode();
void enterWakeUpAndSleepMode();
void setSLEEPTIME( uint8_t sleeptime );
void setWakeUpAndSleepMode( uint8_t sleeptime );
void enterDeepSleepMode();

void exitSleepMode();
void exitWakeAndSleepMode();
void exitDeepSleepMode();


// Trigger setting functions

void alertTriggersConversion();
void spiTriggersConversion();
void csTriggersConversion();


// Threshold Detection + ALERT output Settings

void alertIndicatesConversionEnable();
void alertIndicatesConversionDisable();

void magSwitchSet( uint8_t thresholds_en );
void disableMagSwitch();
void magThreshSet( uint8_t set_threshold, int8_t thrx_hi, int8_t thrx_lo );

// Measurement Configuration Functions

void enableMagChannels( uint8_t mag_ch_en_bits );
void enableAngleMeasurement( uint8_t angle_en_bits );
void enableTemperatureMeasurement();
void disableTemperatureMeasurement();
void setSampleRate( uint8_t CONV_AVG_bits );
void setRanges( uint8_t x_range_bits, uint8_t y_range_bits, uint8_t z_range_bits );


// Get Results/Measurement Functions (Normal Read Mode)

uint16_t getXresult();
uint16_t getYresult();
uint16_t getZresult();
uint16_t getTEMPresult();
uint16_t getANGLEresult();
uint16_t getMAGresult();

void getMagResultsRegistersNrml( int16_t meas_arr[] );

float getMeasurementNrmlX();
float getMeasurementNrmlY();
float getMeasurementNrmlZ();
float getMeasurementNrmlTEMP();
float getMeasurementNrmlANGLE();
float getMeasurementNrmlMAG();

void getMagMeasurementsNrml( float meas_arr[] );


// Get Range Functions

uint16_t getXrange();
uint16_t getYrange();
uint16_t getZrange();
uint16_t getMAGrange();


// Get Device Info Functions

uint8_t getVersion();
uint8_t isCRCenabled();


// Offset and Gain Correction Functions

void setMagOffsetIn7Bit( uint8_t offset_select, uint8_t offset1_bits , uint8_t offset2_bits );
void setMagOffsetInmT( uint8_t offset_select, float offset_delta1 , float offset_delta2 );

void setMagGainConfigIn11Bit( uint8_t axis, uint16_t gain_value );
void setMagGainConfigInDecimal( uint8_t axis, float gain_value );


// Special Read Mode Functions

void setDATATYPE( uint8_t data_type_bits );
void getMeasurementsSpcl( float meas_arr[] );


// Supplemental Functions

void calcCORDIC(float CORDIC_results[], int16_t numerator, int16_t denominator, uint16_t range, int16_t iteration_length);
void atan2CORDIC(int16_t numerator, int16_t denominator, int16_t iteration_length, int32_t results[]);

void planeAngles(int16_t axis1, int16_t axis2, int16_t axis3,  int32_t results[]);
void convertToCylindrical(int16_t axis1, int16_t axis2, int16_t axis3,  int32_t results[]);
void convertToSpherical(int16_t axis1, int16_t axis2, int16_t axis3,  int32_t results[]);
uint32_t mag3D (int16_t axis1, int16_t axis2, int16_t axis3);

float piecewiseLinearizationRegister( int16_t knownValue[], float knownError[], uint16_t known_length, int16_t measValue, uint16_t range );
float piecewiseLinearizationFloat( float knownValue[], float knownError[], uint16_t known_length, float measValue );
float piecewiseLinearizationAngle( uint16_t knownAngle[], float knownError[], uint16_t known_length, uint16_t measAngle );
float piecewiseLinearizationAngleFloat( float knownAngle[], float knownError[], uint16_t known_length, float measAngle );


// Helper Functions

uint32_t isqrt32(uint32_t h);

float resultRegisterTomT( int16_t register_bits, uint16_t range );
float angleRegisterToDeg( uint16_t register_bits );

uint8_t calculateCRC( uint8_t data[] );
uint8_t verifyCRC( uint8_t data[] );

void csPulse();
void alertPulse();

//****************************************************************************
//
// Definitions for use with Functions
//
//****************************************************************************

#define DATA_TYPE_RESULTS_NormalMode                    ((uint8_t) 0x00)
#define DATA_TYPE_RESULTS_SpecialMode_XY                ((uint8_t) 0x01)
#define DATA_TYPE_RESULTS_SpecialMode_XZ                ((uint8_t) 0x02)
#define DATA_TYPE_RESULTS_SpecialMode_YZ                ((uint8_t) 0x03)
#define DATA_TYPE_RESULTS_SpecialMode_XT                ((uint8_t) 0x04)
#define DATA_TYPE_RESULTS_SpecialMode_YT                ((uint8_t) 0x05)
#define DATA_TYPE_RESULTS_SpecialMode_ZT                ((uint8_t) 0x06)
#define DATA_TYPE_RESULTS_SpecialMode_AM                ((uint8_t) 0x07)

#define ALERT_CONFIG_FULL_THRX_ALRT_MASK                ((uint16_t) 0x000F)
#define SENSOR_CONFIG_FULL_RANGE_MASK                   ((uint16_t) 0x003F)

// These 'BITS' definitions are for use as function inputs

#define MAG_CH_EN_BITS_NONE                                  ((uint8_t) 0x00)
#define MAG_CH_EN_BITS_X                                     ((uint8_t) 0x01)
#define MAG_CH_EN_BITS_Y                                     ((uint8_t) 0x02)
#define MAG_CH_EN_BITS_XY                                    ((uint8_t) 0x03)
#define MAG_CH_EN_BITS_Z                                     ((uint8_t) 0x04)
#define MAG_CH_EN_BITS_ZX                                    ((uint8_t) 0x05)
#define MAG_CH_EN_BITS_YZ                                    ((uint8_t) 0x06)
#define MAG_CH_EN_BITS_XYZ                                   ((uint8_t) 0x07)
#define MAG_CH_EN_BITS_XYX                                   ((uint8_t) 0x08)
#define MAG_CH_EN_BITS_YXY                                   ((uint8_t) 0x09)
#define MAG_CH_EN_BITS_YZY                                   ((uint8_t) 0x0A)
#define MAG_CH_EN_BITS_ZYZ                                   ((uint8_t) 0x0B)
#define MAG_CH_EN_BITS_ZXZ                                   ((uint8_t) 0x0C)
#define MAG_CH_EN_BITS_XZX                                   ((uint8_t) 0x0D)
#define MAG_CH_EN_BITS_XYZYX                                 ((uint8_t) 0x0E)
#define MAG_CH_EN_BITS_XYZZYX                                ((uint8_t) 0x0F)

#define ANGLE_EN_BITS_OFF                                    ((uint8_t) 0x00)
#define ANGLE_EN_BITS_XY                                     ((uint8_t) 0x01)
#define ANGLE_EN_BITS_YZ                                     ((uint8_t) 0x02)
#define ANGLE_EN_BITS_XZ                                     ((uint8_t) 0x03)

#define CONV_AVG_BITS_1X                                     ((uint8_t) 0x00)
#define CONV_AVG_BITS_2X                                     ((uint8_t) 0x01)
#define CONV_AVG_BITS_4X                                     ((uint8_t) 0x02)
#define CONV_AVG_BITS_8X                                     ((uint8_t) 0x03)
#define CONV_AVG_BITS_16X                                    ((uint8_t) 0x04)
#define CONV_AVG_BITS_32X                                    ((uint8_t) 0x05)

/*
 * Temperature Electrical Characteristics (ECHAR)
 *
 * Currently the 'Typical' Electrical Characteristics (ECHAR) of the device are set for
 * ECHAR_T_ADC_RES and ECHAR_T_SENS_T0. These values can differ and, if through device
 * calibration their actual values for a particular device are found, can be updated for
 * more accurate temperature measurement.
 *
 * Pg. 6 of the datasheet contains the descriptions of the Temperature Sensing Electrical
 * Characteristics for the TMAG5170. The definition names match their written
 * counterparts.
 */

// TEMP_RESULT decimal value @ ECHAR_T_SENS_T0
#define ECHAR_T_ADC_T0                                  ((float) 17522)

// Reference Temperature for ECHAR_T_ADC_T0 (C)
#define ECHAR_T_SENS_T0                                 ((float) 25) // Typical value provided datasheet, actual can differ

// Temp sensing resolution (LSB/C)
#define ECHAR_T_ADC_RES                                 ((float) 60.0) // Typical value provided by datasheet, actual can differ


//****************************************************************************
//
// Register definitions
//
//****************************************************************************

/* NOTE: Whenever possible, macro names (defined below) were derived from
 * datasheet defined names; however, updates to documentation may cause
 * mismatches between names defined here in example code from those shown
 * in the device datasheet.
 */


/* Register 0x00 (DEVICE_CONFIG) definition
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |    Bit 15    |    Bit 14    |    Bit 13    |    Bit 12    |    Bit 11    |    Bit 10    |     Bit 9    |     Bit 8    |     Bit 7    |     Bit 6    |     Bit 5    |     Bit 4    |     Bit 3    |     Bit 2    |     Bit 1    |     Bit 0    |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |       0      |              CONV_AVG_NUM[2:0]             |       0      |       0      |       MAG_TEMPCO[1:0]       |       0      |             OPERATING_MODE[2:0]            |    T_CH_EN   |    T_RATE    |   T_HLT_EN   | TEMP_COMP_EN |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 */

    /* DEVICE_CONFIG register address */
    #define DEVICE_CONFIG_ADDRESS                                           ((uint8_t) 0x00)

    /* DEVICE_CONFIG default (reset) value */
    #define DEVICE_CONFIG_DEFAULT                                           ((uint16_t) 0x0000)

    /* DEVICE_CONFIG register field masks */
    #define DEVICE_CONFIG_CONV_AVG_NUM_MASK                                 ((uint16_t) 0x7000)
    #define DEVICE_CONFIG_MAG_TEMPCO_MASK                                   ((uint16_t) 0x0300)
    #define DEVICE_CONFIG_OPERATING_MODE_MASK                               ((uint16_t) 0x0070)
    #define DEVICE_CONFIG_T_CH_EN_MASK                                      ((uint16_t) 0x0008)
    #define DEVICE_CONFIG_T_RATE_MASK                                       ((uint16_t) 0x0004)
    #define DEVICE_CONFIG_T_HLT_EN_MASK                                     ((uint16_t) 0x0002)

    /* CONV_AVG_NUM field values */
    #define DEVICE_CONFIG_CONV_AVG_NUM_1x1333Kbps40Kbps1axis                ((uint16_t) 0x0000)
    #define DEVICE_CONFIG_CONV_AVG_NUM_2x665Kbps20Kbps1axis                 ((uint16_t) 0x1000)
    #define DEVICE_CONFIG_CONV_AVG_NUM_4x333Kbps10Kbps1axis                 ((uint16_t) 0x2000)
    #define DEVICE_CONFIG_CONV_AVG_NUM_8x166Kbps5Kbps1axis                  ((uint16_t) 0x3000)
    #define DEVICE_CONFIG_CONV_AVG_NUM_16x0833Kbps25Kbps1axis               ((uint16_t) 0x4000)
    #define DEVICE_CONFIG_CONV_AVG_NUM_32x0417Kbps125Kbps1axis              ((uint16_t) 0x5000)

    /* MAG_TEMPCO field values */
    #define DEVICE_CONFIG_MAG_TEMPCO_0C                                     ((uint16_t) 0x0000)
    #define DEVICE_CONFIG_MAG_TEMPCO_012C                                   ((uint16_t) 0x0100)
    #define DEVICE_CONFIG_MAG_TEMPCO_003C                                   ((uint16_t) 0x0200)
    #define DEVICE_CONFIG_MAG_TEMPCO_02C                                    ((uint16_t) 0x0300)

    /* OPERATING_MODE field values */
    #define DEVICE_CONFIG_OPERATING_MODE_ConfigurationMode                  ((uint16_t) 0x0000)
    #define DEVICE_CONFIG_OPERATING_MODE_StandbyMode                        ((uint16_t) 0x0010)
    #define DEVICE_CONFIG_OPERATING_MODE_ActiveMeasureMode                  ((uint16_t) 0x0020)
    #define DEVICE_CONFIG_OPERATING_MODE_ActiveTriggerMode                  ((uint16_t) 0x0030)
    #define DEVICE_CONFIG_OPERATING_MODE_WakeupandSleepMode                 ((uint16_t) 0x0040)
    #define DEVICE_CONFIG_OPERATING_MODE_SleepMode                          ((uint16_t) 0x0050)
    #define DEVICE_CONFIG_OPERATING_MODE_DeepSleepMode                      ((uint16_t) 0x0060)

    /* T_CH_EN field values */
    #define DEVICE_CONFIG_T_CH_EN_TemperatureChannelDisabled                ((uint16_t) 0x0000)
    #define DEVICE_CONFIG_T_CH_EN_TemperatureChannelEnabled                 ((uint16_t) 0x0008)

    /* T_RATE field values */
    #define DEVICE_CONFIG_T_RATE_TemperatureConversionRatesameasCONV_AVG_NUM    ((uint16_t) 0x0000)
    #define DEVICE_CONFIG_T_RATE_TemperatureConversononceperconversionset   ((uint16_t) 0x0004)

    /* T_HLT_EN field values */
    #define DEVICE_CONFIG_T_HLT_EN_TemperatureLimitCheckOff                 ((uint16_t) 0x0000)
    #define DEVICE_CONFIG_T_HLT_EN_TemperatureLimitCheckOn                  ((uint16_t) 0x0002)



/* Register 0x01 (SENSOR_CONFIG) definition
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |    Bit 15    |    Bit 14    |    Bit 13    |    Bit 12    |    Bit 11    |    Bit 10    |     Bit 9    |     Bit 8    |     Bit 7    |     Bit 6    |     Bit 5    |     Bit 4    |     Bit 3    |     Bit 2    |     Bit 1    |     Bit 0    |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |        ANGLE_EN[1:0]        |                       SLEEPTIME[3:0]                      |                       MAG_CH_EN[3:0]                      |         Z_RANGE[1:0]        |         Y_RANGE[1:0]        |         X_RANGE[1:0]        |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 */

    /* SENSOR_CONFIG register address */
    #define SENSOR_CONFIG_ADDRESS                                           ((uint8_t) 0x01)

    /* SENSOR_CONFIG default (reset) value */
    #define SENSOR_CONFIG_DEFAULT                                           ((uint16_t) 0x0000)

    /* SENSOR_CONFIG register field masks */
    #define SENSOR_CONFIG_ANGLE_EN_MASK                                     ((uint16_t) 0xC000)
    #define SENSOR_CONFIG_SLEEPTIME_MASK                                    ((uint16_t) 0x3C00)
    #define SENSOR_CONFIG_MAG_CH_EN_MASK                                    ((uint16_t) 0x03C0)
    #define SENSOR_CONFIG_Z_RANGE_MASK                                      ((uint16_t) 0x0030)
    #define SENSOR_CONFIG_Y_RANGE_MASK                                      ((uint16_t) 0x000C)
    #define SENSOR_CONFIG_X_RANGE_MASK                                      ((uint16_t) 0x0003)

    /* ANGLE_EN field values */
    #define SENSOR_CONFIG_ANGLE_EN_AngleCalculationOFF                      ((uint16_t) 0x0000)
    #define SENSOR_CONFIG_ANGLE_EN_XY                                       ((uint16_t) 0x4000)
    #define SENSOR_CONFIG_ANGLE_EN_YZ                                       ((uint16_t) 0x8000)
    #define SENSOR_CONFIG_ANGLE_EN_XZ                                       ((uint16_t) 0xC000)

    /* SLEEPTIME field values */
    #define SENSOR_CONFIG_SLEEPTIME_1ms                                     ((uint16_t) 0x0000)
    #define SENSOR_CONFIG_SLEEPTIME_5ms                                     ((uint16_t) 0x0400)
    #define SENSOR_CONFIG_SLEEPTIME_10ms                                    ((uint16_t) 0x0800)
    #define SENSOR_CONFIG_SLEEPTIME_15ms                                    ((uint16_t) 0x0C00)
    #define SENSOR_CONFIG_SLEEPTIME_20ms                                    ((uint16_t) 0x1000)
    #define SENSOR_CONFIG_SLEEPTIME_30ms                                    ((uint16_t) 0x1400)
    #define SENSOR_CONFIG_SLEEPTIME_50ms                                    ((uint16_t) 0x1800)
    #define SENSOR_CONFIG_SLEEPTIME_100ms                                   ((uint16_t) 0x1C00)
    #define SENSOR_CONFIG_SLEEPTIME_500ms                                   ((uint16_t) 0x2000)
    #define SENSOR_CONFIG_SLEEPTIME_1000ms                                  ((uint16_t) 0x2400)

    /* MAG_CH_EN field values */
    #define SENSOR_CONFIG_MAG_CH_EN_AllChannelsOFF                          ((uint16_t) 0x0000)
    #define SENSOR_CONFIG_MAG_CH_EN_X                                       ((uint16_t) 0x0040)
    #define SENSOR_CONFIG_MAG_CH_EN_Y                                       ((uint16_t) 0x0080)
    #define SENSOR_CONFIG_MAG_CH_EN_XY                                      ((uint16_t) 0x00C0)
    #define SENSOR_CONFIG_MAG_CH_EN_Z                                       ((uint16_t) 0x0100)
    #define SENSOR_CONFIG_MAG_CH_EN_ZX                                      ((uint16_t) 0x0140)
    #define SENSOR_CONFIG_MAG_CH_EN_YZ                                      ((uint16_t) 0x0180)
    #define SENSOR_CONFIG_MAG_CH_EN_XYZ                                     ((uint16_t) 0x01C0)
    #define SENSOR_CONFIG_MAG_CH_EN_XYX                                     ((uint16_t) 0x0200)
    #define SENSOR_CONFIG_MAG_CH_EN_YXY                                     ((uint16_t) 0x0240)
    #define SENSOR_CONFIG_MAG_CH_EN_YZY                                     ((uint16_t) 0x0280)
    #define SENSOR_CONFIG_MAG_CH_EN_ZYZ                                     ((uint16_t) 0x02C0)
    #define SENSOR_CONFIG_MAG_CH_EN_ZXZ                                     ((uint16_t) 0x0300)
    #define SENSOR_CONFIG_MAG_CH_EN_XZX                                     ((uint16_t) 0x0340)
    #define SENSOR_CONFIG_MAG_CH_EN_XYZYX                                   ((uint16_t) 0x0380)
    #define SENSOR_CONFIG_MAG_CH_EN_XYZZYX                                  ((uint16_t) 0x03C0)

    /* Z_RANGE field values */
    #define SENSOR_CONFIG_Z_RANGE_50mT                                      ((uint16_t) 0x0000)
    #define SENSOR_CONFIG_Z_RANGE_25mT                                      ((uint16_t) 0x0010)
    #define SENSOR_CONFIG_Z_RANGE_100mT                                     ((uint16_t) 0x0020)

    /* Y_RANGE field values */
    #define SENSOR_CONFIG_Y_RANGE_50mT                                      ((uint16_t) 0x0000)
    #define SENSOR_CONFIG_Y_RANGE_25mT                                      ((uint16_t) 0x0004)
    #define SENSOR_CONFIG_Y_RANGE_100mT                                     ((uint16_t) 0x0008)

    /* X_RANGE field values */
    #define SENSOR_CONFIG_X_RANGE_50mT                                      ((uint16_t) 0x0000)
    #define SENSOR_CONFIG_X_RANGE_25mT                                      ((uint16_t) 0x0001)
    #define SENSOR_CONFIG_X_RANGE_100mT                                     ((uint16_t) 0x0002)



/* Register 0x02 (SYSTEM_CONFIG) definition
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |    Bit 15    |    Bit 14    |    Bit 13    |    Bit 12    |    Bit 11    |    Bit 10    |     Bit 9    |     Bit 8    |     Bit 7    |     Bit 6    |     Bit 5    |     Bit 4    |     Bit 3    |     Bit 2    |     Bit 1    |     Bit 0    |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |       0      |       0      |        DIAG_SEL[1:0]        |       0      |      TRIGGER_MODE[1:0]      |               DATA_TYPE[2:0]               |    DIAG_EN   |       0      |       0      |   Z_HLT_EN   |   Y_HLT_EN   |   X_HLT_EN   |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 */

    /* SYSTEM_CONFIG register address */
    #define SYSTEM_CONFIG_ADDRESS                                           ((uint8_t) 0x02)

    /* SYSTEM_CONFIG default (reset) value */
    #define SYSTEM_CONFIG_DEFAULT                                           ((uint16_t) 0x0000)

    /* SYSTEM_CONFIG register field masks */
    #define SYSTEM_CONFIG_DIAG_SEL_MASK                                     ((uint16_t) 0x3000)
    #define SYSTEM_CONFIG_TRIGGER_MODE_MASK                                 ((uint16_t) 0x0600)
    #define SYSTEM_CONFIG_DATA_TYPE_MASK                                    ((uint16_t) 0x01C0)
    #define SYSTEM_CONFIG_DIAG_EN_MASK                                      ((uint16_t) 0x0020)
    #define SYSTEM_CONFIG_Z_HLT_EN_MASK                                     ((uint16_t) 0x0004)
    #define SYSTEM_CONFIG_Y_HLT_EN_MASK                                     ((uint16_t) 0x0002)
    #define SYSTEM_CONFIG_X_HLT_EN_MASK                                     ((uint16_t) 0x0001)

    /* DIAG_SEL field values */
    #define SYSTEM_CONFIG_DIAG_SEL_Runalldiagnosticstogether                ((uint16_t) 0x0000)
    #define SYSTEM_CONFIG_DIAG_SEL_Runenableddiagnosticstogether            ((uint16_t) 0x1000)
    #define SYSTEM_CONFIG_DIAG_SEL_Runalldiagnosticsinsequence              ((uint16_t) 0x2000)
    #define SYSTEM_CONFIG_DIAG_SEL_Runenableddiagnosticsinsequence          ((uint16_t) 0x3000)

    /* TRIGGER_MODE field values */
    #define SYSTEM_CONFIG_TRIGGER_MODE_AtSPIcommandbits                     ((uint16_t) 0x0000)
    #define SYSTEM_CONFIG_TRIGGER_MODE_AtnCSSyncPulse                       ((uint16_t) 0x0200)
    #define SYSTEM_CONFIG_TRIGGER_MODE_AtALERTSyncPulse                     ((uint16_t) 0x0400)

    /* DATA_TYPE field values */
    #define SYSTEM_CONFIG_DATA_TYPE_Default32bitdata                        ((uint16_t) 0x0000)
    #define SYSTEM_CONFIG_DATA_TYPE_XY                                      ((uint16_t) 0x0040)
    #define SYSTEM_CONFIG_DATA_TYPE_XZ                                      ((uint16_t) 0x0080)
    #define SYSTEM_CONFIG_DATA_TYPE_ZY                                      ((uint16_t) 0x00C0)
    #define SYSTEM_CONFIG_DATA_TYPE_XT                                      ((uint16_t) 0x0100)
    #define SYSTEM_CONFIG_DATA_TYPE_YT                                      ((uint16_t) 0x0140)
    #define SYSTEM_CONFIG_DATA_TYPE_ZT                                      ((uint16_t) 0x0180)
    #define SYSTEM_CONFIG_DATA_TYPE_AM                                      ((uint16_t) 0x01C0)

    /* DIAG_EN field values */
    #define SYSTEM_CONFIG_DIAG_EN_OFF                                       ((uint16_t) 0x0000)
    #define SYSTEM_CONFIG_DIAG_EN_ON                                        ((uint16_t) 0x0020)

    /* Z_HLT_EN field values */
    #define SYSTEM_CONFIG_Z_HLT_EN_OFF                                      ((uint16_t) 0x0000)
    #define SYSTEM_CONFIG_Z_HLT_EN_ON                                       ((uint16_t) 0x0004)

    /* Y_HLT_EN field values */
    #define SYSTEM_CONFIG_Y_HLT_EN_OFF                                      ((uint16_t) 0x0000)
    #define SYSTEM_CONFIG_Y_HLT_EN_ON                                       ((uint16_t) 0x0002)

    /* X_HLT_EN field values */
    #define SYSTEM_CONFIG_X_HLT_EN_OFF                                      ((uint16_t) 0x0000)
    #define SYSTEM_CONFIG_X_HLT_EN_ON                                       ((uint16_t) 0x0001)



/* Register 0x03 (ALERT_CONFIG) definition
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |    Bit 15    |    Bit 14    |    Bit 13    |    Bit 12    |    Bit 11    |    Bit 10    |     Bit 9    |     Bit 8    |     Bit 7    |     Bit 6    |     Bit 5    |     Bit 4    |     Bit 3    |     Bit 2    |     Bit 1    |     Bit 0    |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |       0      |       0      |  ALERT_LATCH |  ALERT_MODE  |  STATUS_ALRT |       0      |       0      |   RSLT_ALRT  |       0      |       0      |       THRX_COUNT[1:0]       |  T_THRX_ALRT |  Z_THRX_ALRT |  Y_THRX_ALRT |  X_THRX_ALRT |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 */

    /* ALERT_CONFIG register address */
    #define ALERT_CONFIG_ADDRESS                                            ((uint8_t) 0x03)

    /* ALERT_CONFIG default (reset) value */
    #define ALERT_CONFIG_DEFAULT                                            ((uint16_t) 0x0000)

    /* ALERT_CONFIG register field masks */
    #define ALERT_CONFIG_ALERT_LATCH_MASK                                   ((uint16_t) 0x2000)
    #define ALERT_CONFIG_ALERT_MODE_MASK                                    ((uint16_t) 0x1000)
    #define ALERT_CONFIG_STATUS_ALRT_MASK                                   ((uint16_t) 0x0800)
    #define ALERT_CONFIG_RSLT_ALRT_MASK                                     ((uint16_t) 0x0100)
    #define ALERT_CONFIG_THRX_COUNT_MASK                                    ((uint16_t) 0x0030)
    #define ALERT_CONFIG_T_THRX_ALRT_MASK                                   ((uint16_t) 0x0008)
    #define ALERT_CONFIG_Z_THRX_ALRT_MASK                                   ((uint16_t) 0x0004)
    #define ALERT_CONFIG_Y_THRX_ALRT_MASK                                   ((uint16_t) 0x0002)
    #define ALERT_CONFIG_X_THRX_ALRT_MASK                                   ((uint16_t) 0x0001)

    /* ALERT_LATCH field values */
    #define ALERT_CONFIG_ALERT_LATCH_Notlatched                             ((uint16_t) 0x0000)
    #define ALERT_CONFIG_ALERT_LATCH_Latched                                ((uint16_t) 0x2000)

    /* ALERT_MODE field values */
    #define ALERT_CONFIG_ALERT_MODE_InterruptandTriggerMode                 ((uint16_t) 0x0000)
    #define ALERT_CONFIG_ALERT_MODE_MagneticSwitchMode                      ((uint16_t) 0x1000)

    /* STATUS_ALRT field values */
    #define ALERT_CONFIG_STATUS_ALRT_ALERTisnotusedtosignalAFE_STATUSorSYS_STATUSflags  ((uint16_t) 0x0000)
    #define ALERT_CONFIG_STATUS_ALRT_ALERToutputsignalsAFE_STATUSandSYS_STATUSflags ((uint16_t) 0x0800)

    /* RSLT_ALRT field values */
    #define ALERT_CONFIG_RSLT_ALRT_ALERTisnotusedtosignalconversioncompleteion  ((uint16_t) 0x0000)
    #define ALERT_CONFIG_RSLT_ALRT_ALERToutputsignalsconversioncomplete     ((uint16_t) 0x0100)

    /* THRX_COUNT field values */
    #define ALERT_CONFIG_THRX_COUNT_1                                       ((uint16_t) 0x0000)
    #define ALERT_CONFIG_THRX_COUNT_2                                       ((uint16_t) 0x0010)
    #define ALERT_CONFIG_THRX_COUNT_3                                       ((uint16_t) 0x0020)
    #define ALERT_CONFIG_THRX_COUNT_4                                       ((uint16_t) 0x0030)

    /* T_THRX_ALRT field values */
    #define ALERT_CONFIG_T_THRX_ALRT_ALERTisnotusedforTemperaturethresholds ((uint16_t) 0x0000)
    #define ALERT_CONFIG_T_THRX_ALRT_ALERToutputisassertedforTemperaturethresholds  ((uint16_t) 0x0008)

    /* Z_THRX_ALRT field values */
    #define ALERT_CONFIG_Z_THRX_ALRT_ALERTisnotusedforZAxisthresholds       ((uint16_t) 0x0000)
    #define ALERT_CONFIG_Z_THRX_ALRT_ALERToutputisassertedforZAxisthresholds    ((uint16_t) 0x0004)

    /* Y_THRX_ALRT field values */
    #define ALERT_CONFIG_Y_THRX_ALRT_ALERTisnotusedforYAxisthresholds       ((uint16_t) 0x0000)
    #define ALERT_CONFIG_Y_THRX_ALRT_ALERToutputisassertedforYAxisthresholds    ((uint16_t) 0x0002)

    /* X_THRX_ALRT field values */
    #define ALERT_CONFIG_X_THRX_ALRT_ALERTisnotusedforXAxisthresholds       ((uint16_t) 0x0000)
    #define ALERT_CONFIG_X_THRX_ALRT_ALERToutputisassertedforXAxisthresholds    ((uint16_t) 0x0001)



/* Register 0x04 (X_THRX_CONFIG) definition
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |    Bit 23    |    Bit 22    |    Bit 21    |    Bit 20    |    Bit 19    |    Bit 18    |    Bit 17    |    Bit 16    |    Bit 15    |    Bit 14    |    Bit 13    |    Bit 12    |    Bit 11    |    Bit 10    |     Bit 9    |     Bit 8    |     Bit 7    |     Bit 6    |     Bit 5    |     Bit 4    |     Bit 3    |     Bit 2    |     Bit 1    |     Bit 0    |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |              |              |              |              |              |              |              |              |                                                  X_HI_THRESHOLD[7:0]                                                  |                                                  X_LO_THRESHOLD[7:0]                                                  |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 */

    /* X_THRX_CONFIG register address */
    #define X_THRX_CONFIG_ADDRESS                                           ((uint8_t) 0x04)

    /* X_THRX_CONFIG default (reset) value */
    #define X_THRX_CONFIG_DEFAULT                                           ((uint16_t) 0x7D83)

    /* X_THRX_CONFIG register field masks */
    #define X_THRX_CONFIG_X_HI_THRESHOLD_MASK                               ((uint16_t) 0xFF00)
    #define X_THRX_CONFIG_X_LO_THRESHOLD_MASK                               ((uint16_t) 0x00FF)

    /* X_HI_THRESHOLD field values */
    #define X_THRX_CONFIG_X_HI_THRESHOLD_98FS                               ((uint16_t) 0x7D00)
    #define X_THRX_CONFIG_X_HI_THRESHOLD_75FS                               ((uint16_t) 0x6000)
    #define X_THRX_CONFIG_X_HI_THRESHOLD_50FS                               ((uint16_t) 0x4000)
    #define X_THRX_CONFIG_X_HI_THRESHOLD_25FS                               ((uint16_t) 0x2000)
    #define X_THRX_CONFIG_X_HI_THRESHOLD_5FS                                ((uint16_t) 0x0700)

    /* X_LO_THRESHOLD field values */
    #define X_THRX_CONFIG_X_LO_THRESHOLD_98FS                               ((uint16_t) 0x0083)
    #define X_THRX_CONFIG_X_LO_THRESHOLD_75FS                               ((uint16_t) 0x00A0)
    #define X_THRX_CONFIG_X_LO_THRESHOLD_50FS                               ((uint16_t) 0x00C0)
    #define X_THRX_CONFIG_X_LO_THRESHOLD_25FS                               ((uint16_t) 0x00F9)
    #define X_THRX_CONFIG_X_LO_THRESHOLD_5FS                                ((uint16_t) 0x00E0)



/* Register 0x05 (Y_THRX_CONFIG) definition
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |    Bit 23    |    Bit 22    |    Bit 21    |    Bit 20    |    Bit 19    |    Bit 18    |    Bit 17    |    Bit 16    |    Bit 15    |    Bit 14    |    Bit 13    |    Bit 12    |    Bit 11    |    Bit 10    |     Bit 9    |     Bit 8    |     Bit 7    |     Bit 6    |     Bit 5    |     Bit 4    |     Bit 3    |     Bit 2    |     Bit 1    |     Bit 0    |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |              |              |              |              |              |              |              |              |                                                  Y_HI_THRESHOLD[7:0]                                                  |                                                  Y_LO_THRESHOLD[7:0]                                                  |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 */

    /* Y_THRX_CONFIG register address */
    #define Y_THRX_CONFIG_ADDRESS                                           ((uint8_t) 0x05)

    /* Y_THRX_CONFIG default (reset) value */
    #define Y_THRX_CONFIG_DEFAULT                                           ((uint16_t) 0x7D83)

    /* Y_THRX_CONFIG register field masks */
    #define Y_THRX_CONFIG_Y_HI_THRESHOLD_MASK                               ((uint16_t) 0xFF00)
    #define Y_THRX_CONFIG_Y_LO_THRESHOLD_MASK                               ((uint16_t) 0x00FF)

    /* Y_HI_THRESHOLD field values */
    #define Y_THRX_CONFIG_Y_HI_THRESHOLD_98FS                               ((uint16_t) 0x7D00)
    #define Y_THRX_CONFIG_Y_HI_THRESHOLD_75FS                               ((uint16_t) 0x6000)
    #define Y_THRX_CONFIG_Y_HI_THRESHOLD_50FS                               ((uint16_t) 0x4000)
    #define Y_THRX_CONFIG_Y_HI_THRESHOLD_25FS                               ((uint16_t) 0x2000)
    #define Y_THRX_CONFIG_Y_HI_THRESHOLD_5FS                                ((uint16_t) 0x0700)

    /* Y_LO_THRESHOLD field values */
    #define Y_THRX_CONFIG_Y_LO_THRESHOLD_98FS                               ((uint16_t) 0x0083)
    #define Y_THRX_CONFIG_Y_LO_THRESHOLD_75FS                               ((uint16_t) 0x00A0)
    #define Y_THRX_CONFIG_Y_LO_THRESHOLD_50FS                               ((uint16_t) 0x00C0)
    #define Y_THRX_CONFIG_Y_LO_THRESHOLD_25FS                               ((uint16_t) 0x00F9)
    #define Y_THRX_CONFIG_Y_LO_THRESHOLD_5FS                                ((uint16_t) 0x00E0)



/* Register 0x06 (Z_THRX_CONFIG) definition
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |    Bit 15    |    Bit 14    |    Bit 13    |    Bit 12    |    Bit 11    |    Bit 10    |     Bit 9    |     Bit 8    |     Bit 7    |     Bit 6    |     Bit 5    |     Bit 4    |     Bit 3    |     Bit 2    |     Bit 1    |     Bit 0    |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |                                                  Z_HI_THRESHOLD[7:0]                                                  |                                                  Z_LO_THRESHOLD[7:0]                                                  |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 */

    /* Z_THRX_CONFIG register address */
    #define Z_THRX_CONFIG_ADDRESS                                           ((uint8_t) 0x06)

    /* Z_THRX_CONFIG default (reset) value */
    #define Z_THRX_CONFIG_DEFAULT                                           ((uint16_t) 0x7D83)

    /* Z_THRX_CONFIG register field masks */
    #define Z_THRX_CONFIG_Z_HI_THRESHOLD_MASK                               ((uint16_t) 0xFF00)
    #define Z_THRX_CONFIG_Z_LO_THRESHOLD_MASK                               ((uint16_t) 0x00FF)

    /* Z_HI_THRESHOLD field values */
    #define Z_THRX_CONFIG_Z_HI_THRESHOLD_98FS                               ((uint16_t) 0x7D00)
    #define Z_THRX_CONFIG_Z_HI_THRESHOLD_75FS                               ((uint16_t) 0x6000)
    #define Z_THRX_CONFIG_Z_HI_THRESHOLD_50FS                               ((uint16_t) 0x4000)
    #define Z_THRX_CONFIG_Z_HI_THRESHOLD_25FS                               ((uint16_t) 0x2000)
    #define Z_THRX_CONFIG_Z_HI_THRESHOLD_5FS                                ((uint16_t) 0x0700)

    /* Z_LO_THRESHOLD field values */
    #define Z_THRX_CONFIG_Z_LO_THRESHOLD_98FS                               ((uint16_t) 0x0083)
    #define Z_THRX_CONFIG_Z_LO_THRESHOLD_75FS                               ((uint16_t) 0x00A0)
    #define Z_THRX_CONFIG_Z_LO_THRESHOLD_50FS                               ((uint16_t) 0x00C0)
    #define Z_THRX_CONFIG_Z_LO_THRESHOLD_25FS                               ((uint16_t) 0x00F9)
    #define Z_THRX_CONFIG_Z_LO_THRESHOLD_5FS                                ((uint16_t) 0x00E0)



/* Register 0x07 (T_THRX_CONFIG) definition
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |    Bit 15    |    Bit 14    |    Bit 13    |    Bit 12    |    Bit 11    |    Bit 10    |     Bit 9    |     Bit 8    |     Bit 7    |     Bit 6    |     Bit 5    |     Bit 4    |     Bit 3    |     Bit 2    |     Bit 1    |     Bit 0    |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |                                                  T_HI_THRESHOLD[7:0]                                                  |                                                  T_LO_THRESHOLD[7:0]                                                  |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 */

    /* T_THRX_CONFIG register address */
    #define T_THRX_CONFIG_ADDRESS                                           ((uint8_t) 0x07)

    /* T_THRX_CONFIG default (reset) value */
    #define T_THRX_CONFIG_DEFAULT                                           ((uint16_t) 0x6732)

    /* T_THRX_CONFIG register field masks */
    #define T_THRX_CONFIG_T_HI_THRESHOLD_MASK                               ((uint16_t) 0xFF00)
    #define T_THRX_CONFIG_T_LO_THRESHOLD_MASK                               ((uint16_t) 0x00FF)



/* Register 0x08 (CONV_STATUS) definition
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |    Bit 15    |    Bit 14    |    Bit 13    |    Bit 12    |    Bit 11    |    Bit 10    |     Bit 9    |     Bit 8    |     Bit 7    |     Bit 6    |     Bit 5    |     Bit 4    |     Bit 3    |     Bit 2    |     Bit 1    |     Bit 0    |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |       0      |       0      |      RDY     |       A      |       T      |       Z      |       Y      |       X      |       0      |               SET_COUNT[2:0]               |       0      |       0      |       ALRT_STATUS[1:0]      |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 */

    /* CONV_STATUS register address */
    #define CONV_STATUS_ADDRESS                                             ((uint8_t) 0x08)

    /* CONV_STATUS default (reset) value */
    #define CONV_STATUS_DEFAULT                                             ((uint16_t) 0x0000)

    /* CONV_STATUS register field masks */
    #define CONV_STATUS_RDY_MASK                                            ((uint16_t) 0x2000)
    #define CONV_STATUS_A_MASK                                              ((uint16_t) 0x1000)
    #define CONV_STATUS_T_MASK                                              ((uint16_t) 0x0800)
    #define CONV_STATUS_Z_MASK                                              ((uint16_t) 0x0400)
    #define CONV_STATUS_Y_MASK                                              ((uint16_t) 0x0200)
    #define CONV_STATUS_X_MASK                                              ((uint16_t) 0x0100)
    #define CONV_STATUS_SET_COUNT_MASK                                      ((uint16_t) 0x0070)
    #define CONV_STATUS_ALRT_STATUS_MASK                                    ((uint16_t) 0x0003)



/* Register 0x09 (X_CH_RESULT) definition
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |    Bit 15    |    Bit 14    |    Bit 13    |    Bit 12    |    Bit 11    |    Bit 10    |     Bit 9    |     Bit 8    |     Bit 7    |     Bit 6    |     Bit 5    |     Bit 4    |     Bit 3    |     Bit 2    |     Bit 1    |     Bit 0    |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |                                                                                                               X_CH_RESULT[15:0]                                                                                                               |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 */

    /* X_CH_RESULT register address */
    #define X_CH_RESULT_ADDRESS                                             ((uint8_t) 0x09)

    /* X_CH_RESULT default (reset) value */
    #define X_CH_RESULT_DEFAULT                                             ((uint16_t) 0x0000)

    /* X_CH_RESULT register field masks */
    #define X_CH_RESULT_X_CH_RESULT_MASK                                    ((uint16_t) 0xFFFF)



/* Register 0x0A (Y_CH_RESULT) definition
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |    Bit 15    |    Bit 14    |    Bit 13    |    Bit 12    |    Bit 11    |    Bit 10    |     Bit 9    |     Bit 8    |     Bit 7    |     Bit 6    |     Bit 5    |     Bit 4    |     Bit 3    |     Bit 2    |     Bit 1    |     Bit 0    |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |                                                                                                               Y_CH_RESULT[15:0]                                                                                                               |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 */

    /* Y_CH_RESULT register address */
    #define Y_CH_RESULT_ADDRESS                                             ((uint8_t) 0x0A)

    /* Y_CH_RESULT default (reset) value */
    #define Y_CH_RESULT_DEFAULT                                             ((uint16_t) 0x0000)

    /* Y_CH_RESULT register field masks */
    #define Y_CH_RESULT_Y_CH_RESULT_MASK                                    ((uint16_t) 0xFFFF)



/* Register 0x0B (Z_CH_RESULT) definition
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |    Bit 15    |    Bit 14    |    Bit 13    |    Bit 12    |    Bit 11    |    Bit 10    |     Bit 9    |     Bit 8    |     Bit 7    |     Bit 6    |     Bit 5    |     Bit 4    |     Bit 3    |     Bit 2    |     Bit 1    |     Bit 0    |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |                                                                                                               Z_CH_RESULT[15:0]                                                                                                               |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 */

    /* Z_CH_RESULT register address */
    #define Z_CH_RESULT_ADDRESS                                             ((uint8_t) 0x0B)

    /* Z_CH_RESULT default (reset) value */
    #define Z_CH_RESULT_DEFAULT                                             ((uint16_t) 0x0000)

    /* Z_CH_RESULT register field masks */
    #define Z_CH_RESULT_Z_CH_RESULT_MASK                                    ((uint16_t) 0xFFFF)



/* Register 0x0C (TEMP_RESULT) definition
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |    Bit 15    |    Bit 14    |    Bit 13    |    Bit 12    |    Bit 11    |    Bit 10    |     Bit 9    |     Bit 8    |     Bit 7    |     Bit 6    |     Bit 5    |     Bit 4    |     Bit 3    |     Bit 2    |     Bit 1    |     Bit 0    |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |                                                                                                               TEMP_RESULT[15:0]                                                                                                               |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 */

    /* TEMP_RESULT register address */
    #define TEMP_RESULT_ADDRESS                                             ((uint8_t) 0x0C)

    /* TEMP_RESULT default (reset) value */
    #define TEMP_RESULT_DEFAULT                                             ((uint16_t) 0x0000)

    /* TEMP_RESULT register field masks */
    #define TEMP_RESULT_TEMP_RESULT_MASK                                    ((uint16_t) 0xFFFF)



/* Register 0x0D (AFE_STATUS) definition
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |    Bit 15    |    Bit 14    |    Bit 13    |    Bit 12    |    Bit 11    |    Bit 10    |     Bit 9    |     Bit 8    |     Bit 7    |     Bit 6    |     Bit 5    |     Bit 4    |     Bit 3    |     Bit 2    |     Bit 1    |     Bit 0    |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |   CFG_RESET  |       0      |       0      |   SENS_STAT  |   TEMP_STAT  |   ZHS_STAT   |   YHS_STAT   |   XHS_STAT   |       0      |       0      |       0      |       0      |       0      |       0      |   TRIM_STAT  |   LDO_STAT   |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 */

    /* AFE_STATUS register address */
    #define AFE_STATUS_ADDRESS                                              ((uint8_t) 0x0D)

    /* AFE_STATUS default (reset) value */
    #define AFE_STATUS_DEFAULT                                              ((uint16_t) 0x8000)

    /* AFE_STATUS register field masks */
    #define AFE_STATUS_CFG_RESET_MASK                                       ((uint16_t) 0x8000)
    #define AFE_STATUS_SENS_STAT_MASK                                       ((uint16_t) 0x1000)
    #define AFE_STATUS_TEMP_STAT_MASK                                       ((uint16_t) 0x0800)
    #define AFE_STATUS_ZHS_STAT_MASK                                        ((uint16_t) 0x0400)
    #define AFE_STATUS_YHS_STAT_MASK                                        ((uint16_t) 0x0200)
    #define AFE_STATUS_XHS_STAT_MASK                                        ((uint16_t) 0x0100)
    #define AFE_STATUS_TRIM_STAT_MASK                                       ((uint16_t) 0x0002)
    #define AFE_STATUS_LDO_STAT_MASK                                        ((uint16_t) 0x0001)



/* Register 0x0E (SYS_STATUS) definition
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |    Bit 15    |    Bit 14    |    Bit 13    |    Bit 12    |    Bit 11    |    Bit 10    |     Bit 9    |     Bit 8    |     Bit 7    |     Bit 6    |     Bit 5    |     Bit 4    |     Bit 3    |     Bit 2    |     Bit 1    |     Bit 0    |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |   ALRT_LVL   |   ALRT_DRV   |   MISO_DRV   |   CRC_STAT   |  FRAME_STAT  |             OPERATING_STAT[2:0]            |       0      |       0      |    VCC_OV    |    VCC_UV    |   TEMP_THX   |    ZCH_THX   |    YCH_THX   |    XCH_THX   |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 */

    /* SYS_STATUS register address */
    #define SYS_STATUS_ADDRESS                                              ((uint8_t) 0x0E)

    /* SYS_STATUS default (reset) value */
    #define SYS_STATUS_DEFAULT                                              ((uint16_t) 0x0000)

    /* SYS_STATUS register field masks */
    #define SYS_STATUS_ALRT_LVL_MASK                                        ((uint16_t) 0x8000)
    #define SYS_STATUS_ALRT_DRV_MASK                                        ((uint16_t) 0x4000)
    #define SYS_STATUS_SDO_DRV_MASK                                         ((uint16_t) 0x2000)
    #define SYS_STATUS_CRC_STAT_MASK                                        ((uint16_t) 0x1000)
    #define SYS_STATUS_FRAME_STAT_MASK                                      ((uint16_t) 0x0800)
    #define SYS_STATUS_OPERATING_STAT_MASK                                  ((uint16_t) 0x0700)
    #define SYS_STATUS_VCC_OV_MASK                                          ((uint16_t) 0x0020)
    #define SYS_STATUS_VCC_UV_MASK                                          ((uint16_t) 0x0010)
    #define SYS_STATUS_TEMP_THX_MASK                                        ((uint16_t) 0x0008)
    #define SYS_STATUS_ZCH_THX_MASK                                         ((uint16_t) 0x0004)
    #define SYS_STATUS_YCH_THX_MASK                                         ((uint16_t) 0x0002)
    #define SYS_STATUS_XCH_THX_MASK                                         ((uint16_t) 0x0001)



/* Register 0x0F (TEST_CONFIG) definition
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |    Bit 15    |    Bit 14    |    Bit 13    |    Bit 12    |    Bit 11    |    Bit 10    |     Bit 9    |     Bit 8    |     Bit 7    |     Bit 6    |     Bit 5    |     Bit 4    |     Bit 3    |     Bit 2    |     Bit 1    |     Bit 0    |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |       0      |       0      |       0      |       0      |       0      |       0      |       0      |       0      |       0      |      ID      |           VER[1:0]          |       0      |    CRC_DIS   |       OSC_CNT_CTL[1:0]      |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 */

    /* TEST_CONFIG register address */
    #define TEST_CONFIG_ADDRESS                                             ((uint8_t) 0x0F)

    /* TEST_CONFIG default (reset) value */
    #define TEST_CONFIG_DEFAULT                                             ((uint16_t) 0x0040)

    /* TEST_CONFIG register field masks */
    #define TEST_CONFIG_VER_MASK                                            ((uint16_t) 0x0010)
    #define TEST_CONFIG_CRC_DIS_MASK                                        ((uint16_t) 0x0004)
    #define TEST_CONFIG_OSC_CNT_CTL_MASK                                    ((uint16_t) 0x0003)

    /* CRC_DIS field values */
    #define TEST_CONFIG_CRC_DIS_CRCEnabled                                  ((uint16_t) 0x0000)
    #define TEST_CONFIG_CRC_DIS_CRCDisabled                                 ((uint16_t) 0x0004)

    /* OSC_CNT_CTL field values */
    #define TEST_CONFIG_OSC_CNT_CTL_ResetCounters                           ((uint16_t) 0x0000)
    #define TEST_CONFIG_OSC_CNT_CTL_StartOSCcounterdrivenbyHFOSC            ((uint16_t) 0x0001)
    #define TEST_CONFIG_OSC_CNT_CTL_StartOSCcounterdrivenbyLFOSC            ((uint16_t) 0x0002)
    #define TEST_CONFIG_OSC_CNT_CTL_StopCounter                             ((uint16_t) 0x0003)



/* Register 0x10 (OSC_MONITOR) definition
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |    Bit 15    |    Bit 14    |    Bit 13    |    Bit 12    |    Bit 11    |    Bit 10    |     Bit 9    |     Bit 8    |     Bit 7    |     Bit 6    |     Bit 5    |     Bit 4    |     Bit 3    |     Bit 2    |     Bit 1    |     Bit 0    |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |                                                                                                                OSC_COUNT[15:0]                                                                                                                |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 */

    /* OSC_MONITOR register address */
    #define OSC_MONITOR_ADDRESS                                             ((uint8_t) 0x10)

    /* OSC_MONITOR default (reset) value */
    #define OSC_MONITOR_DEFAULT                                             ((uint16_t) 0x0000)

    /* OSC_MONITOR register field masks */
    #define OSC_MONITOR_OSC_COUNT_MASK                                      ((uint16_t) 0xFFFF)



/* Register 0x11 (MAG_GAIN_CONFIG) definition
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |    Bit 15    |    Bit 14    |    Bit 13    |    Bit 12    |    Bit 11    |    Bit 10    |     Bit 9    |     Bit 8    |     Bit 7    |     Bit 6    |     Bit 5    |     Bit 4    |     Bit 3    |     Bit 2    |     Bit 1    |     Bit 0    |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |     GAIN_SELECTION[1:0]     |       0      |       0      |       0      |                                                                          GAIN_VALUE[10:0]                                                                          |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 */

    /* MAG_GAIN_CONFIG register address */
    #define MAG_GAIN_CONFIG_ADDRESS                                         ((uint8_t) 0x11)

    /* MAG_GAIN_CONFIG default (reset) value */
    #define MAG_GAIN_CONFIG_DEFAULT                                         ((uint16_t) 0x0000)

    /* MAG_GAIN_CONFIG register field masks */
    #define MAG_GAIN_CONFIG_GAIN_SELECTION_MASK                             ((uint16_t) 0xC000)
    #define MAG_GAIN_CONFIG_GAIN_VALUE_MASK                                 ((uint16_t) 0x03FF)

    /* GAIN_SELECTION field values */
    #define MAG_GAIN_CONFIG_GAIN_SELECTION_NoAxisSelected                   ((uint16_t) 0x0000)
    #define MAG_GAIN_CONFIG_GAIN_SELECTION_Xaxis                            ((uint16_t) 0x4000)
    #define MAG_GAIN_CONFIG_GAIN_SELECTION_Yaxis                            ((uint16_t) 0x8000)
    #define MAG_GAIN_CONFIG_GAIN_SELECTION_Zaxis                            ((uint16_t) 0xC000)



/* Register 0x12 (MAG_OFFSET_CONFIG) definition
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |    Bit 15    |    Bit 14    |    Bit 13    |    Bit 12    |    Bit 11    |    Bit 10    |     Bit 9    |     Bit 8    |     Bit 7    |     Bit 6    |     Bit 5    |     Bit 4    |     Bit 3    |     Bit 2    |     Bit 1    |     Bit 0    |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |    OFFSET_SELECTION[1:0]    |                                           OFFSET_VALUE1[6:0]                                           |                                           OFFSET_VALUE2[6:0]                                           |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 */

    /* MAG_OFFSET_CONFIG register address */
    #define MAG_OFFSET_CONFIG_ADDRESS                                       ((uint8_t) 0x12)

    /* MAG_OFFSET_CONFIG default (reset) value */
    #define MAG_OFFSET_CONFIG_DEFAULT                                       ((uint16_t) 0x0000)

    /* MAG_OFFSET_CONFIG register field masks */
    #define MAG_OFFSET_CONFIG_OFFSET_SELECTION_MASK                         ((uint16_t) 0xC000)
    #define MAG_OFFSET_CONFIG_OFFSET_VALUE1_MASK                            ((uint16_t) 0x3F80)
    #define MAG_OFFSET_CONFIG_OFFSET_VALUE2_MASK                            ((uint16_t) 0x007F)

    /* OFFSET_SELECTION field values */
    #define MAG_OFFSET_CONFIG_OFFSET_SELECTION_NoAxisSelected               ((uint16_t) 0x0000)
    #define MAG_OFFSET_CONFIG_OFFSET_SELECTION_XaxisOFFSET_VALUE1YaxisOFFSET_VALUE2 ((uint16_t) 0x4000)
    #define MAG_OFFSET_CONFIG_OFFSET_SELECTION_YaxisOFFSET_VALUE1ZaxisOFFSET_VALUE2 ((uint16_t) 0x8000)
    #define MAG_OFFSET_CONFIG_OFFSET_SELECTION_XaxisOFFSET_VALUE1ZaxisOFFSET_VALUE2 ((uint16_t) 0xC000)



/* Register 0x13 (ANGLE_RESULT) definition
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |    Bit 15    |    Bit 14    |    Bit 13    |    Bit 12    |    Bit 11    |    Bit 10    |     Bit 9    |     Bit 8    |     Bit 7    |     Bit 6    |     Bit 5    |     Bit 4    |     Bit 3    |     Bit 2    |     Bit 1    |     Bit 0    |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |                                                                                                               ANGLE_RESULT[15:0]                                                                                                              |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 */

    /* ANGLE_RESULT register address */
    #define ANGLE_RESULT_ADDRESS                                            ((uint8_t) 0x13)

    /* ANGLE_RESULT default (reset) value */
    #define ANGLE_RESULT_DEFAULT                                            ((uint16_t) 0x0000)

    /* ANGLE_RESULT register field masks */
    #define ANGLE_RESULT_ANGLE_RESULT_MASK                                  ((uint16_t) 0xFFFF)



/* Register 0x14 (MAGNITUDE RESULT) definition
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |    Bit 15    |    Bit 14    |    Bit 13    |    Bit 12    |    Bit 11    |    Bit 10    |     Bit 9    |     Bit 8    |     Bit 7    |     Bit 6    |     Bit 5    |     Bit 4    |     Bit 3    |     Bit 2    |     Bit 1    |     Bit 0    |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 * |                                                                                                             MAGNITUDE_RESULT[15:0]                                                                                                            |
 * |-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 */

    /* MAGNITUDE RESULT register address */
    #define MAGNITUDE_RESULT_ADDRESS                                        ((uint8_t) 0x14)

    /* MAGNITUDE RESULT default (reset) value */
    #define MAGNITUDE_RESULT_DEFAULT                                        ((uint16_t) 0x0000)

    /* MAGNITUDE RESULT register field masks */
    #define MAGNITUDE_RESULT_MAGNITUDE_RESULT_MASK                          ((uint16_t) 0xFFFF)



#endif /* TMAG5170_H_ */
