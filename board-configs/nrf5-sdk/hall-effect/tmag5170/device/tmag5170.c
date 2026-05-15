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

/**
 * Modified by: Kefan Zheng
 * Date: 2024-03-09
 * Description: This file is a modified version of the original file from Texas Instruments Incorporated,
 *              based on the specific requirements to run on nRF52840 and the nRF5 SDK.
*/

#include "tmag5170.h"

/*
 * These code examples were written with the objective of minimizing the amount
 * of static variables tracking the states of the sensor they are used with. Because
 * of this, functions in this code will send additional SPI read commands to get
 * mostly unchanging data (such as the device version or set ranges) needed for
 * other operations.
 *
 * The SYSTEM_CONFIG register must be stored and kept current with the sensor
 * to keep track of the DATA_TYPE field so that Special Read Mode operation can
 * be implemented successfully.
 */

//****************************************************************************
//
// Internal variables
//
//****************************************************************************

// These two arrays are used with the CORDIC function to convert the integer output
// into floating point values
const uint32_t magArray[16] = {1518500250,1358187913,1317635818,1307460871,
                               1304914694,1304277995,1304118810,1304079014,
                               1304069065,1304066577,1304065955,1304065800,
                               1304065761,1304065751,1304065749,1304065748};
const uint32_t atanArray32[16] = {536870912,316933406,167458907,85004756,42667331,
                                  21354465,10679838,5340245,2670163,1335087,667544,
                                  333772,166886,83443,41722,20861};

//****************************************************************************
//! This variable tracks the state of the SYSTEM_CONFIG register (0x02)
//! Since any special read state cannot read out registers when called,
//! keeping track of the SYSTEM_CONFIG register is needed in order to
//! switch to and from normal read mode without altering other bits besides
//! DATA_TYPE's (0x028-6).
//!
//! The macro 'DATA_TYPE_RESULTS' defined in the header can be used to access
//! the DATA_TYPE bits contained in the stored register.
//!
//! This variable also must be initialized using a function like the one
//! below before the rest of any code implementation using the given example
//! functions can work.
//****************************************************************************
static uint16_t SYSTEM_CONFIG_stored = 0;
#define DATA_TYPE_RESULTS    ((SYSTEM_CONFIG_stored & ~(SYSTEM_CONFIG_DATA_TYPE_MASK)) >> 6)

#ifdef      GLOBAL_CRC_ERROR_VAR_ENABLED
            uint8_t crc_error_occurence = 0;
#endif

#ifdef      GLOBAL_INPUT_ERROR_VAR_ENABLED
            uint8_t fcn_input_error_occurence = 0;
#endif



//****************************************************************************
//! Initialization function
//!
//! This function MUST be ran at the beginning of any implementation using this
//! example code. This is needed because the SYSTEM_CONFIG register is needed to
//! be tracked and updated accordingly to prevent losing track of the DATA_TYPE
//! in use by the device.
//!
//! If CRC is being disabled per the header definition being enabled, then this
//! function also sends the command to disable CRC on the device as well.
//! NOTE: if CRC is being disabled then the RESET_DEVICE_IN_STARTUP should be
//! enabled as well
//****************************************************************************
void TMAG5170startup()
{
    // (OPTIONAL) Provide additional delay time for power supply settling
    delay_ms(50);

#ifdef RESET_DEVICE_IN_STARTUP
    // SYSTEM_CONFIG_stored will be set to default in this function
    resetDevice();
#else
    // The default implementation of this initialize function is resetting only
    // the SYSTEM_CONFIG register and leaving the other registers unchanged.
    writeToRegister( SYSTEM_CONFIG_ADDRESS, SYSTEM_CONFIG_DEFAULT );
    SYSTEM_CONFIG_stored = SYSTEM_CONFIG_DEFAULT;
#endif

#ifdef DISABLE_CRC_IN_STARTUP
    // Sends command to disable CRC verfication and calculation on device-side
    // MUST be called first in implementation for commands from other functions
    // to be accepted when DISABLE_CRC_IN_STARTUP is used.
    uint8_t dataTx[4] = {0x0F,0x00,0x04,0x07};
    uint8_t dataRx[4] = {0};
    sendAndReceiveFrame(dataTx,dataRx);
#endif
}



//****************************************************************************
//! Reset Device to Factory Settings
//!
//! This function uses the DeepSleep functions to reset the device's registers back to the
//! default settings outlined in the datasheet. This function also resets the
//! SYSTEM_CONFIG_stored variable to the default value in the enterDeepSleepMode function.
//****************************************************************************
void resetDevice()
{
    enterDeepSleepMode(); // Deep Sleep Mode resets the device to its default settings
    exitDeepSleepMode();
}

//****************************************************************************
//! Getter Functions for the SYSTEM_CONFIG_stored static variable
//!
//! These two functions return the stored SYSTEM_CONFIG register on the MCU or the
//! DATA_TYPE field within that stored register, respectively.
//****************************************************************************

uint16_t getSYSTEM_CONFIG_stored() { return SYSTEM_CONFIG_stored; }
uint16_t getDATA_TYPE() { return DATA_TYPE_RESULTS; }



//****************************************************************************
//****************************************************************************
//
// SPI fucntions
//
//****************************************************************************
//****************************************************************************


//****************************************************************************
//! Send and Receive Frame
//!
//! Takes in the frame to transmit by SPI and the array to put the received frame in
//!
//! dataTx[4] - unint8_t array of length 4 containing the bytes frame to transmit
//!             ordered with CRC-containing byte last
//! dataRx[4] - unint8_t array of length 4 of zeroes on input, will have the received
//!             frame bytes assigned to it ordered with CRC bits last
//****************************************************************************
void sendAndReceiveFrame(uint8_t dataTx[], uint8_t dataRx[])
{

#ifdef DISABLE_CRC_IN_STARTUP
    // When CRC is disabled upon system initialization, the need to calculate
    // the dataTx CRC for the TMAG5170 to accept commands is gone at the risk
    // of transmission errors being passed through.

    spiSendReceiveArrays(dataTx, dataRx, TMAG5170_FRAME_NUM_BYTES);

    // Check if SYSTEM_CONFIG was written to and update SYSTEM_CONFIG_stored if so
    if ( dataTx[0] == SYSTEM_CONFIG_ADDRESS ) SYSTEM_CONFIG_stored = (dataTx[1] << 8) | dataTx[2];
    SYSTEM_CONFIG_stored &= ~(0xC818); // Reserved bits cannot be 1, this ensures the
                                       // stored variable doesn't have them changed
#else
    // When CRC is enabled (it is by default), the CRC must be calculated and
    // included in dataTx for the TMAG5170 to accept commands.

    uint8_t crc = calculateCRC( dataTx );
    dataTx[3] = dataTx[3] | crc;

    spiSendReceiveArrays(dataTx, dataRx, TMAG5170_FRAME_NUM_BYTES);

    // The TMAG5170 also calculates the CRC for the frame it sends back to its
    // MCU. The TMAG5170 will purposely return a bad CRC if it received a
    // transmitted command with a bad CRC as well. Verifying the CRC of dataRx
    // helps confirm if dataTx was received successfully and if dataRx is valid.
    if (verifyCRC(dataRx) == 0)
    {
        // Without GLOBAL_CRC_ERROR_VAR_ENABLED defined or another error response
        // implemented, this function will not give any indication that the
        // received data has an error

        #ifdef  SEND_RECIEVE_REDUNDANCY_ENABLED

        for ( i = 0; i<4; i++ ) { dataRx[i] = 0 };
        spiSendReceiveArrays(dataTx, dataRx, TMAG5170_FRAME_NUM_BYTES);
        if ( verifyCRC(dataRx) )
        {
            // Check if SYSTEM_CONFIG was written to and update SYSTEM_CONFIG_stored if so
            if ( dataTx[0] == SYSTEM_CONFIG_ADDRESS ) SYSTEM_CONFIG_stored = (dataTx[1] << 8) | dataTx[2];
            SYSTEM_CONFIG_stored &= ~(0xC818); // Reserved bits cannot be 1, this ensures the
                                               // stored variable doesn't have them changed
            return;
        }

        #endif

        #ifdef  GLOBAL_CRC_ERROR_VAR_ENABLED
        crc_error_occurence = 1; // If redundancy is enabled too, the error bit
                                 // only flips if both sent TXs receive bad CRCs
        #endif

        return; // If a received data CRC error is detected, SYSTEM_CONFIG_stored
                // will not be updated under the assumption that whatever command
                // sent was not accepted.
    }

    // Check if SYSTEM_CONFIG was written to and update SYSTEM_CONFIG_stored if so
    if ( dataTx[0] == SYSTEM_CONFIG_ADDRESS ) SYSTEM_CONFIG_stored = (dataTx[1] << 8) | dataTx[2];
    SYSTEM_CONFIG_stored &= ~(0xC818); // Reserved bits cannot be 1, this ensures the
                                       // stored variable doesn't have them changed
#endif
}



//****************************************************************************
//! Write to Register Function (no CMD sent or value returned)
//!
//! Takes in the address of the register to edit and the 16-bit frame to overwrite it with and writes
//! the input frame to the register.
//!
//! This function replaces the whole register with the input data, make sure the values desired to be
//! unchanged are in the input data_to_write!
//!
//! This function will work in Normal and Special Read Mode.
//!
//! address       - uint8_t value from 0x00 to 0x14 containing the register address to write over
//! data_to_write - the 16-bit frame to be written to the register at address.
//****************************************************************************
void writeToRegister(uint8_t address, uint16_t data_to_write)
{
    // Check that the input address is in range
    assert(address < NUM_REGISTERS);

    // Build TX and RX byte arrays
    uint8_t dataTx[4] = { 0 };
    uint8_t dataRx[4] = { 0 };

    // MSB is 0 for WRITE command
    dataTx[0] = (address);
    dataTx[1] = (data_to_write >> 8);
    dataTx[2] = (data_to_write);
    dataTx[3] = 0x00;

    sendAndReceiveFrame(dataTx, dataRx);
}



//****************************************************************************
//! Write to Register Function (CMD0 sent)
//!
//! Takes in the address of the register to edit and the 16-bit frame to overwrite it with and writes
//! the frame to the register.
//! Sends CMD0 function (cmd_bits = 0x01), triggering conversion if device trigger is set to
//! 'start at SPI command' [TRIGGER_MODE (0x02A-9) == 00b]
//!
//! This function replaces the whole register with the input data, make sure the values desired to be
//! unchanged are in the input data_to_write!
//!
//! This function will work in Normal and Special Read Mode.
//!
//! address       - uint8_t value from 0x00 to 0x14 containing the register address to write over
//! data_to_write - the 16-bit frame to be written to the register at address.
//****************************************************************************
void writeToRegisterWithCMD0(uint8_t address, uint16_t data_to_write)
{
    // Check that the input address is in range
    assert(address < NUM_REGISTERS);

    // Build TX and RX byte arrays
    uint8_t dataTx[4] = { 0 };
    uint8_t dataRx[4] = { 0 };

    // MSB is 0 for WRITE command
    dataTx[0] = (address);
    dataTx[1] = (data_to_write >> 8);
    dataTx[2] = (data_to_write);
    dataTx[3] = 0x10;

    sendAndReceiveFrame(dataTx, dataRx);
}



//****************************************************************************
//! Write to Register Function (Send CMD bits and return STAT)
//!
//! Takes in the address of the register to edit and the 16-bit frame to overwrite it with and writes
//! the frame to the register.
//! Sends CMD0 and CMD1 depending on cmd_bits setting and returns the STAT values returned by the device.
//! (STAT4 - STAT11 are not returned by the device in Special Read Mode)
//!
//! This function replaces the whole register with the input data, make sure the values desired to be
//! unchanged are in the input data_to_write!
//!
//! This function will work in Normal and Special Read Mode.
//!
//! address       - uint8_t value from 0x00 to 0x14 containing the register address to write over
//! data_to_write - the 16-bit frame to be written to the register at address.
//! cmd_bits      - uint8_t value from 0x00 to 0x03 containing the CMD0 and CMD1 bits that will be sent
//!                 in dataTx. LSB is CMD0, next bit is CMD1.  (see header file or datasheet pg. 29 for CMD functions)
//****************************************************************************
uint16_t writeToRegisterWithSTAT(uint8_t address, uint16_t data_to_write, uint8_t cmd_bits)
{
    // Check that the input address is in range
    assert(address < NUM_REGISTERS);

    // Build TX and RX byte arrays
    uint8_t dataTx[4] = { 0 };
    uint8_t dataRx[4] = { 0 };

    // MSB is 0 for WRITE command
    dataTx[0] = (address);
    dataTx[1] = (data_to_write >> 8);
    dataTx[2] = (data_to_write);
    dataTx[3] = (cmd_bits << 4);

    sendAndReceiveFrame(dataTx, dataRx);

    if ( DATA_TYPE_RESULTS == 0x00 )
    {
        return (cmd_bits << 12) | (dataRx[0] << 4) | (dataRx[3] >> 4);
    }
    else
    {
        return (cmd_bits << 12) | (dataRx[3] >> 4);
    }

    // returned bits: | 15 | 14 | 13 | 12 | 11 | 10 | 09 | 08 | 07 | 06 | 05 | 04 | 03 | 02 | 01 | 00 |
    //                |  0 |  0 | cmd_bits|  --------           STAT11 - STAT0           --------     |
    // if DATA_TYPE != 000b (Special Read Mode) then only STAT0 to STAT3 will be returned
    // (all other STAT bits will be returned as zero)
}



//****************************************************************************
//! Full Read Function for Normal Data Mode (DATA_TYPE = 000b)
//!
//! Takes in an output array of length 2, address to read from, and CMD bits to send along,
//! then creates the dataTx array and calls the sendAndReturnFrame function, interpreting
//! dataRx and putting the read register in output[0] and status bits in output[1]
//!
//! output[2] - empty uint16_t array of length 2, output[0] will be assigned the returned register
//!             at the given address, output[1] will be assigned the returned status bits.
//! address   - uint8_t value from 0x00 to 0x14 containing the register address to read from
//! cmd_bits  - uint8_t value from 0x00 to 0x03 containing the CMD0 and CMD1 bits that will be sent
//!             in dataTx. LSB is CMD0, next bit is CMD1. (see header file or datasheet pg. 29 for CMD functions)
//****************************************************************************
void normalRead( uint16_t output[], uint8_t address, uint8_t cmd_bits )
{
    // Check that the input address is in range
    assert(address < NUM_REGISTERS);

    // Build TX and RX byte arrays
    uint8_t dataTx[4] = { 0 };
    uint8_t dataRx[4] = { 0 };

    // MSB is 1 for READ command
    dataTx[0] = (address | 0x80);
    dataTx[1] = 0x00;
    dataTx[2] = 0x00;
    dataTx[3] = cmd_bits << 4;

    sendAndReceiveFrame(dataTx, dataRx);
    output[0] = (dataRx[1] << 8) | dataRx[2];
    output[1] = (dataRx[0] << 4) | (dataRx[3] >> 4);
}



//****************************************************************************
//! Register-only Read Function for Normal Data Mode (DATA_TYPE = 000b)
//!
//! Takes in address to read from and returns register at address without the status bits or
//! triggering any CMD function (cmd_bits = 0x00).
//!
//! address   - uint8_t value from 0x00 to 0x14 containing the register address to read from
//****************************************************************************
uint16_t normalReadRegister( uint8_t address )
{
    uint16_t output[2] = { 0 };
    normalRead( output, address, 0x00 );
    return output[0];
}



//****************************************************************************
//! Register-only Read Function for Normal Data Mode with CMD0 sent (DATA_TYPE = 000b)
//!
//! Takes in address to read from and returns register at address without the status bits.
//! Sends CMD0 function (cmd_bits = 0x01), triggering conversion if device trigger is set to
//! 'start at SPI command' [TRIGGER_MODE (0x02A-9) == 00b]
//!
//! address   - uint8_t value from 0x00 to 0x14 containing the register address to read from
//****************************************************************************
uint16_t normalReadRegisterWithCMD0( uint8_t address )
{
    uint16_t output[2] = { 0 };
    normalRead( output, address, 0x01 );
    return output[0];
}



//****************************************************************************
//! Status-bits-only Read Function for Normal Data Mode (DATA_TYPE = 000b)
//!
//! Takes in address to read from and CMD bits and returns the used CMD bits and
//! status bits WITHOUT a read register. (by default this function sends a read command for the
//! DEVICE_CONFIG register)
//!
//! cmd_bits  - uint8_t value from 0x00 to 0x03 containing the CMD0 and CMD1 bits that will be sent
//!             in dataTx. LSB is CMD0, next bit is CMD1. (see header file or datasheet pg. 29 for CMD functions)
//****************************************************************************
uint16_t normalReadSTAT( uint8_t cmd_bits )
{
    uint16_t output[2] = { 0 };
    normalRead( output, DEVICE_CONFIG_ADDRESS, cmd_bits );
    return (cmd_bits << 12) | output[0];
    // returned bits: | 15 | 14 | 13 | 12 | 11 | 10 | 09 | 08 | 07 | 06 | 05 | 04 | 03 | 02 | 01 | 00 |
    //                |  0 |  0 | cmd_bits|  --------           STAT11 - STAT0           --------     |
}



//****************************************************************************
//! Read Function for Special Data Mode (DATA_TYPE != 000b)
//!
//! Takes in an output array of length 3, address to read from, and CMD bits to send along,
//! then creates the dataTx array and calls the sendAndReturnFrame function, interpreting
//! dataRx according to Special Data Mode and putting the first measurement channel in output[0],
//! second measurement channel in output[1], and status bits in output[2]
//!
//! output[3] - empty uint16_t array of length 3, output[0] will be assigned the CH1 measurement,
//!             output[1] will be assigned the CH2 measurement, and output[2] will be assigned the
//!             returned status bits.
//! cmd_bits  - uint8_t value from 0x00 to 0x03 containing the CMD0 and CMD1 bits that will be sent
//!             in dataTx. LSB is CMD0, next bit is CMD1. (see header file or datasheet pg. 29 for CMD functions)
//****************************************************************************
void specialRead( uint16_t output[], uint8_t cmd_bits )
{
    // Build TX and RX byte arrays
    uint8_t dataTx[4] = { 0 };
    uint8_t dataRx[4] = { 0 };

    dataTx[0] = 0x80;
    dataTx[1] = 0x00;
    dataTx[2] = 0x00;
    dataTx[3] = cmd_bits << 4;

    sendAndReceiveFrame(dataTx, dataRx);

    // The output values have their respective bits assembled from the RX frame
    output[0] = output[1] = 0x0000;
    output[0] = ((dataRx[1] & 0x0F) << 8);
    output[0] |= (dataRx[2] & 0xF0);
    output[0] |= (dataRx[3] >> 4);
    output[1] = ((dataRx[0] & 0x0F) << 8);
    output[1] |= (dataRx[1] & 0xF0);
    output[1] |= (dataRx[2] & 0x0F);

    output[2] = 0x0000;
    output[2] = (dataRx[0] >> 4);
}



//****************************************************************************
//! Status-bits-only Read Function for Special Data Mode (DATA_TYPE != 000b)
//!
//! Takes in CMD bits to send on dataTx and returns the received status bits without
//! the measurement channel values.
//!
//! cmd_bits  - uint8_t value from 0x00 to 0x03 containing the CMD0 and CMD1 bits that will be sent
//!             in dataTx. LSB is CMD0, next bit is CMD1. (see header file or datasheet pg. 29 for CMD functions)
//****************************************************************************
uint16_t specialReadSTAT( uint8_t cmd_bits )
{
    uint16_t output[3];
    specialRead( output, cmd_bits );
    return output[2];
}




//****************************************************************************
//****************************************************************************
//
// Change Device Operation Mode
//
//****************************************************************************
//****************************************************************************


//****************************************************************************
//! Enter Configuration Mode
//!
//! DOES NOT WORK IN SPECIAL READ MODE [DATA_TYPE field at 0x028-6 does not equal 000b]
//****************************************************************************
void enterConfigurationMode()
{
    // To prevent undefined behavior, this function does not perform its operation
    // when the DATA_TYPE field (address: 0x028-6) is not set to Normal Read Mode (000b)
    if ( DATA_TYPE_RESULTS != DATA_TYPE_RESULTS_NormalMode ) return;

    uint16_t input;
    // Set OPERATING_MODE (address: 0x006-4) to Configuration Mode (0h)
    input = normalReadRegister(DEVICE_CONFIG_ADDRESS);
    input = ( input & ~(DEVICE_CONFIG_OPERATING_MODE_MASK) ) | DEVICE_CONFIG_OPERATING_MODE_ConfigurationMode;
    writeToRegister( DEVICE_CONFIG_ADDRESS, input );

#ifdef  MAX_DELAYS_IN_OPMODE_CHANGES
    delay_us(50); // max expected delay as given by t_start_sleep (datasheet pg. 10)
#endif
}



//****************************************************************************
//! Enter Stand-by Mode
//!
//! DOES NOT WORK IN SPECIAL READ MODE [DATA_TYPE field at 0x028-6 does not equal 000b]
//****************************************************************************
void enterStandbyMode()
{
    // To prevent undefined behavior, this function does not perform its operation
    // when the DATA_TYPE field (address: 0x028-6) is not set to Normal Read Mode (000b)
    if ( DATA_TYPE_RESULTS != DATA_TYPE_RESULTS_NormalMode ) return;

    uint16_t input;
    // Set OPERATING_MODE (address: 0x006-4) to Standby Mode (1h)
    input = normalReadRegister(DEVICE_CONFIG_ADDRESS);
    input = ( input & ~(DEVICE_CONFIG_OPERATING_MODE_MASK) ) | DEVICE_CONFIG_OPERATING_MODE_StandbyMode;
    writeToRegister( DEVICE_CONFIG_ADDRESS, input );

#ifdef  MAX_DELAYS_IN_OPMODE_CHANGES
    delay_us(140); // max expected delay as given by t_start_sleep + t_stand_by (datasheet pg. 10)
#endif
}



//****************************************************************************
//! Enter Active Measure Mode (continuous conversion)
//!
//! DOES NOT WORK IN SPECIAL READ MODE [DATA_TYPE field at 0x028-6 does not equal 000b]
//****************************************************************************
void enterActiveMeasureMode()
{
    // To prevent undefined behavior, this function does not perform its operation
    // when the DATA_TYPE field (address: 0x028-6) is not set to Normal Read Mode (000b)
    if ( DATA_TYPE_RESULTS != DATA_TYPE_RESULTS_NormalMode ) return;

    uint16_t input;
    // Set OPERATING_MODE (0x006-4) to Active Measure Mode (2h)
    input = normalReadRegister(DEVICE_CONFIG_ADDRESS);
    input = ( input & ~(DEVICE_CONFIG_OPERATING_MODE_MASK) ) | DEVICE_CONFIG_OPERATING_MODE_ActiveMeasureMode;
    writeToRegister( DEVICE_CONFIG_ADDRESS, input );

#ifdef  MAX_DELAYS_IN_OPMODE_CHANGES
    delay_us(140); // max expected delay as given by t_start_sleep + t_stand_by (datasheet pg. 10)
#endif
}



//****************************************************************************
//! Enter Active Trigger Mode
//!
//! DOES NOT WORK IN SPECIAL READ MODE [DATA_TYPE field at 0x028-6 does not equal 000b]
//****************************************************************************
void enterActiveTriggerMode()
{
    // To prevent undefined behavior, this function does not perform its operation
    // when the DATA_TYPE field (0x028-6) is not set to Normal Read Mode (000b)
    if ( DATA_TYPE_RESULTS != DATA_TYPE_RESULTS_NormalMode ) return;

    uint16_t input;
    // Set OPERATING_MODE (0x006-4) to Active Trigger Mode (3h)
    input = normalReadRegister(DEVICE_CONFIG_ADDRESS);
    input = ( input & ~(DEVICE_CONFIG_OPERATING_MODE_MASK) ) | DEVICE_CONFIG_OPERATING_MODE_ActiveTriggerMode;
    writeToRegister( DEVICE_CONFIG_ADDRESS, input );

#ifdef  MAX_DELAYS_IN_OPMODE_CHANGES
    delay_us(140); // max expected delay as given by t_start_sleep + t_stand_by (datasheet pg. 10)
#endif
}



//****************************************************************************
//! Enter Sleep Mode
//!
//! DOES NOT WORK IN SPECIAL READ MODE [DATA_TYPE field at 0x028-6 does not equal 000b]
//****************************************************************************
void enterSleepMode()
{
    // To prevent undefined behavior, this function does not perform its operation
    // when the DATA_TYPE field (0x028-6) is not set to Normal Read Mode (000b)
    if ( DATA_TYPE_RESULTS != DATA_TYPE_RESULTS_NormalMode ) return;

    uint16_t input;
    // Set OPERATING_MODE (0x006-4) to Sleep Mode (5h)
    input = normalReadRegister(DEVICE_CONFIG_ADDRESS);
    input = ( input & ~(DEVICE_CONFIG_OPERATING_MODE_MASK) ) | DEVICE_CONFIG_OPERATING_MODE_SleepMode;
    writeToRegister( DEVICE_CONFIG_ADDRESS, input );

#ifdef  MAX_DELAYS_IN_OPMODE_CHANGES
    delay_us(60); // max expected delay as given by t_go_sleep (datasheet pg. 10)
#endif
}



//****************************************************************************
//! Enter Wake and Sleep Mode
//! Does not change the SLEEPTIME bits
//!
//! DOES NOT WORK IN SPECIAL READ MODE [DATA_TYPE field at 0x028-6 does not equal 000b]
//****************************************************************************
void enterWakeUpAndSleepMode()
{
    // To prevent undefined behavior, this function does not perform its operation
    // when the DATA_TYPE field (0x028-6) is not set to Normal Read Mode (000b)
    if ( DATA_TYPE_RESULTS != DATA_TYPE_RESULTS_NormalMode ) return;

    uint16_t input;
    // Set OPERATING_MODE (0x006-4) to Wake-Up and Sleep Mode (4h)
    input = normalReadRegister(DEVICE_CONFIG_ADDRESS);
    input = ( input & ~(DEVICE_CONFIG_OPERATING_MODE_MASK) ) | DEVICE_CONFIG_OPERATING_MODE_WakeupandSleepMode;
    writeToRegister( DEVICE_CONFIG_ADDRESS, input );

#ifdef  MAX_DELAYS_IN_OPMODE_CHANGES
    delay_us(60); // max expected delay as given by t_go_sleep (datasheet pg. 10)
#endif
}



//****************************************************************************
//! Set SLEEPTIME
//! Takes in input from 0x00 to 0x09 to determine SLEEPTIME field according to datasheet (pg. 35)
//! Does not change OPERATING_MODE and will not begin Wake and Sleep Mode
//!
//! SLEEPTIME determines the amount of time spend in low power mode between device conversions
//! while in Wake Up and Sleep mode (OPERATING_MODE = 4h)
//!                          |
//!          SLEEPTIME bits  |  time between convs.
//!         _________________|_______________________
//!               0x00                   1ms
//!               0x01                   5ms
//!               0x02                  10ms
//!               0x03                  15ms
//!               0x04                  20ms
//!               0x05                  30ms
//!               0x06                  50ms
//!               0x07                 100ms
//!               0x08                 500ms
//!               0x09                1000ms
//!
//! DOES NOT WORK IN SPECIAL READ MODE [DATA_TYPE field at 0x028-6 does not equal 000b]
//****************************************************************************
void setSLEEPTIME( uint8_t sleeptime )
{
    // To prevent undefined behavior, this function does not perform its operation
    // when the DATA_TYPE field (0x028-6) is not set to Normal Read Mode (000b)
    if ( DATA_TYPE_RESULTS != DATA_TYPE_RESULTS_NormalMode ) return;

    // Check that inputs are valid
    if ( !(sleeptime <= 0x09) ) return;

    uint16_t input;

    // Set SLEEPTIME (0x01D-A) to input value (corresponding time values are on datasheet pg. 35)
    input = normalReadRegister(SENSOR_CONFIG_ADDRESS);
    input = ( input & ~(SENSOR_CONFIG_SLEEPTIME_MASK) ) | (sleeptime << 10);
    writeToRegister( SENSOR_CONFIG_ADDRESS, input );
}



//****************************************************************************
//! Wake Up and Sleep Mode with SLEEPTIME set
//! Takes in input from 0x00 to 0x0F to set the SLEEPTIME field according to datasheet
//!
//! DOES NOT WORK IN SPECIAL READ MODE [DATA_TYPE field at 0x028-6 does not equal 000b]
//****************************************************************************
void setWakeUpAndSleepMode( uint8_t sleeptime )
{
    setSLEEPTIME(sleeptime);
    enterWakeUpAndSleepMode();
}



//****************************************************************************
//! Enter Deep Sleep Mode
//!
//! Make sure to use the exitDeepSleepMode function to properly exit Deep Sleep Mode.
//! Deep Sleep Mode can be alternately exited with a short pulse on the CS pin.
//!
//! WILL WORK IN SPECIAL READ MODE, DEEP SLEEP MODE RESETS DEVICE TO FACTORY SETTINGS
//! (EXITS SPECIAL READ MODE)
//****************************************************************************
void enterDeepSleepMode()
{
    // Send Write command, Deep Sleep resets device to factory settings so
    // an initial register read is not needed (DEVICE_CONFIG default is 0x0000)
    writeToRegister( DEVICE_CONFIG_ADDRESS, DEVICE_CONFIG_OPERATING_MODE_DeepSleepMode );
    SYSTEM_CONFIG_stored = SYSTEM_CONFIG_DEFAULT;
    delay_us(100);
}



//****************************************************************************
//! Exit Sleep Mode
//!
//! Exits Sleep Mode by setting the OP_MODE field to Configuration Mode.
//!
//! DOES NOT WORK IN SPECIAL READ MODE [DATA_TYPE field at 0x028-6 does not equal 000b]
//****************************************************************************
void exitSleepMode()
{
    // To prevent undefined behavior, this function does not perform its operation
    // when the DATA_TYPE field (0x028-6) is not set to Normal Read Mode (000b)
    if ( DATA_TYPE_RESULTS != DATA_TYPE_RESULTS_NormalMode ) return;

    uint16_t input;
    // Set OPERATING_MODE (0x006-4) to Configuration Mode (0h)
    input = normalReadRegister(DEVICE_CONFIG_ADDRESS);
    input = ( input & ~(DEVICE_CONFIG_OPERATING_MODE_MASK) ) | DEVICE_CONFIG_OPERATING_MODE_ConfigurationMode;
    writeToRegister( DEVICE_CONFIG_ADDRESS, input );

#ifdef  MAX_DELAYS_IN_OPMODE_CHANGES
    delay_us(140); // max expected delay as given by t_start_sleep + t_stand_by (datasheet pg. 10)
#endif
}



//****************************************************************************
//! Exit Wake Up and Sleep Mode
//!
//! Exits Wake Up and Sleep Mode by setting the OP_MODE field to Configuration Mode.
//!
//! DOES NOT WORK IN SPECIAL READ MODE [DATA_TYPE field at 0x028-6 does not equal 000b]
//****************************************************************************
void exitWakeAndSleepMode()
{
    // To prevent undefined behavior, this function does not perform its operation
    // when the DATA_TYPE field (0x028-6) is not set to Normal Read Mode (000b)
    if ( DATA_TYPE_RESULTS != DATA_TYPE_RESULTS_NormalMode ) return;

    uint16_t input;
    // Set OPERATING_MODE (0x006-4) to Configuration Mode (0h)
    input = normalReadRegister(DEVICE_CONFIG_ADDRESS);
    input = ( input & ~(DEVICE_CONFIG_OPERATING_MODE_MASK) ) | DEVICE_CONFIG_OPERATING_MODE_ConfigurationMode;
    writeToRegister( DEVICE_CONFIG_ADDRESS, input );

#ifdef  MAX_DELAYS_IN_OPMODE_CHANGES
    delay_us(140); // max expected delay as given by t_start_sleep + t_stand_by (datasheet pg. 10)
#endif
}



//****************************************************************************
//! Exit Deep Sleep Mode
//! (Use this function instead of a different operation mode change function to
//! exit Deep Sleep mode properly)
//!
//! Exits Deep Sleep Mode by pulsing LOW on the CS pin and waiting for the chip
//! to start up. (t_start_deep_sleep)
//!
//! DOES NOT WORK IN SPECIAL READ MODE [DATA_TYPE field at 0x028-6 does not equal 000b]
//****************************************************************************
void exitDeepSleepMode()
{
    // A LOW pulse is needed on CS to exit Deep Sleep Mode (enters Configuration Mode)
    csPulse();

#ifdef  MAX_DELAYS_IN_OPMODE_CHANGES
    delay_us(500); // max expected delay as given by t_start_deep_sleep (datasheet pg. 10)
#else
    delay_us(260); // typical delay for t_start_deep_sleep at Vcc = 2.3V (datasheet pg. 10)
#endif
}






//****************************************************************************
//****************************************************************************
//
// Functions to Configure Trigger Settings
//
//****************************************************************************
//****************************************************************************


//****************************************************************************
//! SPI to to Trigger Conversion
//!
//! Configures the conversion trigger to be activated on a SPI read or write command with CMD0 enabled.
//! (Use normalReadRegisterWithCMD0, writeToRegisterWithCMD0, or any R/W function with a 'cmd_bits'
//!  input to send CMD0 in sent frame)
//!
//! DOES NOT WORK IN SPECIAL READ MODE [DATA_TYPE field at 0x028-6 does not equal 000b]
//****************************************************************************
void spiTriggersConversion()
{
    // To prevent undefined behavior, this function does not perform its operation
    // when the DATA_TYPE field (address: 0x028-6) is not set to Normal Read Mode (000b)
    if ( DATA_TYPE_RESULTS != DATA_TYPE_RESULTS_NormalMode ) return;

    uint16_t input;
    // SET TRIGGER_MODE (address: 0x02A-9) to 'trigger at SPI CMD0' (0h)
    input = normalReadRegister(SYSTEM_CONFIG_ADDRESS);
    input &= ~(SYSTEM_CONFIG_TRIGGER_MODE_MASK);
    input |= SYSTEM_CONFIG_TRIGGER_MODE_AtSPIcommandbits;
    writeToRegister( SYSTEM_CONFIG_ADDRESS, input );
}



//****************************************************************************
//! CS pulse to Trigger Conversion
//!
//! Configures the conversion trigger to be activated on a LOW pulse (1us to 25us in length) to CS.
//!
//! DOES NOT WORK IN SPECIAL READ MODE [DATA_TYPE field at 0x028-6 does not equal 000b]
//****************************************************************************
void csTriggersConversion()
{
    // To prevent undefined behavior, this function does not perform its operation
    // when the DATA_TYPE field (address: 0x028-6) is not set to Normal Read Mode (000b)
    if ( DATA_TYPE_RESULTS != DATA_TYPE_RESULTS_NormalMode ) return;

    uint16_t input;
    // SET TRIGGER_MODE (address: 0x02A-9) to at CS pulse (1h)
    input = normalReadRegister(SYSTEM_CONFIG_ADDRESS);
    input &= ~(SYSTEM_CONFIG_TRIGGER_MODE_MASK);
    input |= SYSTEM_CONFIG_TRIGGER_MODE_AtnCSSyncPulse;
    writeToRegister( SYSTEM_CONFIG_ADDRESS, input );
}



//****************************************************************************
//! ALERT to Trigger Conversion
//!
//! Configures the conversion trigger to be activated on a LOW pulse (1us to 25us in length) to ALERT.
//! ALERT_MODE must be in Interrupt & Trigger Mode for this functionality to work.
//!
//! DOES NOT WORK IN SPECIAL READ MODE [DATA_TYPE field at 0x028-6 does not equal 000b]
//****************************************************************************
void alertTriggersConversion()
{
    // To prevent undefined behavior, this function does not perform its operation
    // when the DATA_TYPE field (0x028-6) is not set to Normal Read Mode (000b)
    if ( DATA_TYPE_RESULTS != DATA_TYPE_RESULTS_NormalMode ) return;

    uint16_t input;
    // SET TRIGGER_MODE (address: 0x02A-9) to at ALERT pulse (2h)
    input = normalReadRegister(SYSTEM_CONFIG_ADDRESS);
    input &= ~(SYSTEM_CONFIG_TRIGGER_MODE_MASK);
    input |= SYSTEM_CONFIG_TRIGGER_MODE_AtALERTSyncPulse;
    writeToRegister( SYSTEM_CONFIG_ADDRESS, input );

    // SET ALERT_MODE (address: 0x03C) to Interrupt & Trigger Mode (0h)
    input = normalReadRegister(ALERT_CONFIG_ADDRESS);
    input &= ~(ALERT_CONFIG_ALERT_MODE_MASK);
    input |= ALERT_CONFIG_ALERT_MODE_InterruptandTriggerMode;
    writeToRegister( ALERT_CONFIG_ADDRESS, input );
}




//****************************************************************************
//****************************************************************************
//
// Threshold Detection + ALERT output Settings
//
//****************************************************************************
//****************************************************************************


//****************************************************************************
//! Enable ALERT to Indicate Conversion
//!
//! Configures the device so when its magnetic measurements are complete, the device will
//! output LOW on the ALERT pin.
//!
//! NOTE: This configures ALERT as an output pin, the respective GPIO pin it is
//!       connected to will have to be set as an input as well. Please ensure none
//!       of the input functions of ALERT are being used as well (such as the ALERT trigger)
//!
//! DOES NOT WORK IN SPECIAL READ MODE [DATA_TYPE field at 0x028-6 does not equal 000b]
//****************************************************************************
void alertIndicatesConversionEnable()
{
    // To prevent undefined behavior, this function does not perform its operation
    // when the DATA_TYPE field (address: 0x028-6) is not set to Normal Read Mode (000b)
    if ( DATA_TYPE_RESULTS != DATA_TYPE_RESULTS_NormalMode ) return;

    uint16_t input;
    // SET ALERT_MODE (address: 0x03C) to Interrupt Mode (0h)
    input = normalReadRegister(ALERT_CONFIG_ADDRESS);
    input &= ~(ALERT_CONFIG_ALERT_MODE_MASK);
    input |= ALERT_CONFIG_ALERT_MODE_InterruptandTriggerMode;

    // SET RSLT_ALRT (address: 0x038) to ALERT output asserted LOW to indicate conversion completion (1h)
    // (Register already grabbed so no new READ command is needed)
    input |= ALERT_CONFIG_RSLT_ALRT_ALERToutputsignalsconversioncomplete;
    writeToRegister( ALERT_CONFIG_ADDRESS, input );
}



//****************************************************************************
//! Disable ALERT to Indicate Conversion
//!
//! Turns off the ALERT pin assertion when device measurements are complete.
//!
//! DOES NOT WORK IN SPECIAL READ MODE [DATA_TYPE field at 0x028-6 does not equal 000b]
//****************************************************************************
void alertIndicatesConversionDisable()
{
    // To prevent undefined behavior, this function does not perform its operation
    // when the DATA_TYPE field (address: 0x028-6) is not set to Normal Read Mode (000b)
    if ( DATA_TYPE_RESULTS != DATA_TYPE_RESULTS_NormalMode ) return;

    uint16_t input;
    // SET RSLT_ALRT (address: 0x038) ALERT output is not asserted to indicate conversion completion (1h)
    input = normalReadRegister(ALERT_CONFIG_ADDRESS);
    input &= ~(ALERT_CONFIG_RSLT_ALRT_MASK);
    input |= ALERT_CONFIG_RSLT_ALRT_ALERTisnotusedtosignalconversioncompleteion;
    writeToRegister( ALERT_CONFIG_ADDRESS, input );
}



//****************************************************************************
//! Setup Magnetic Switch Mode
//!
//! Enables Magnetic Switch Mode for the ALERT pin and the respective high/low
//! threshold triggers. The thresholds are not set using this function, use
//! magThreshSet to change the high/low thresholds for each measurement.
//!
//! thresholds_en - input determining the state of each threshold
//!                 (4 LSBs of input int correlate to which thresholds are enabled in
//!                  order TZYX, cannot be greater than 0x0F)
//!                 examples:
//!                 thresholds_en = 0x0A (1010b) ==> T and Y are enabled, Z and X are disabled
//!                 thresholds_en = 0x04 (0100b) ==> Z is enabled, T, Y, and X are disabled
//!                 --inputs of 0x00 and 0x0F can be used to disable or enable all thresholds, respectively
//!
//! NOTE: This configures ALERT as an output pin, the respective GPIO pin it is
//!       connected to will have to be set as an input as well. Please ensure none
//!       of the input functions of ALERT are being used as well (such as the ALERT trigger)
//!
//! DOES NOT WORK IN SPECIAL READ MODE [DATA_TYPE field at 0x028-6 does not equal 000b]
//****************************************************************************
void setMagSwitch( uint8_t thresholds_en )
{
    // To prevent undefined behavior, this function does not perform its operation
    // when the DATA_TYPE field (address: 0x028-6) is not set to Normal Read Mode (000b)
    if ( DATA_TYPE_RESULTS != DATA_TYPE_RESULTS_NormalMode ) return;

    // Check that inputs are valid
    if( !(thresholds_en <= 0x0F) ) return;

    uint16_t input;

    // Pull DEVICE_CONFIG register and set T_HLT_EN according to input, then
    // write back to TMAG5170
    input = normalReadRegister(DEVICE_CONFIG_ADDRESS);
    input &= ~(DEVICE_CONFIG_T_HLT_EN_MASK);

    if ( (thresholds_en & 0x08) == 0x08 ) input |= DEVICE_CONFIG_T_HLT_EN_TemperatureLimitCheckOn;
    else input |= DEVICE_CONFIG_T_HLT_EN_TemperatureLimitCheckOff;

    writeToRegister( DEVICE_CONFIG_ADDRESS, input );

    // Pull SYSTEM_CONFIG register and set X, Y, and Z_HLT_EN according to input,
    // then write back to TMAG5170
    input = normalReadRegister(SYSTEM_CONFIG_ADDRESS);
    input &= ~(SYSTEM_CONFIG_Z_HLT_EN_MASK);
    input &= ~(SYSTEM_CONFIG_Y_HLT_EN_MASK);
    input &= ~(SYSTEM_CONFIG_X_HLT_EN_MASK);

    if ( (thresholds_en & 0x04) == 0x04 ) input |= SYSTEM_CONFIG_Z_HLT_EN_ON;
    else input |= SYSTEM_CONFIG_Z_HLT_EN_OFF;

    if ( (thresholds_en & 0x02) == 0x02 ) input |= SYSTEM_CONFIG_Y_HLT_EN_ON;
    else input |= SYSTEM_CONFIG_Y_HLT_EN_OFF;

    if ( (thresholds_en & 0x01) == 0x01 ) input |= SYSTEM_CONFIG_X_HLT_EN_ON;
    else input |= SYSTEM_CONFIG_X_HLT_EN_OFF;

    writeToRegister( SYSTEM_CONFIG_ADDRESS, input );


    // Set ALERT_MODE field in ALERT_CONFIG to Magnetic Switch Mode
    input = normalReadRegister(ALERT_CONFIG_ADDRESS);
    input &= ~(ALERT_CONFIG_ALERT_MODE_MASK);
    input |= ALERT_CONFIG_ALERT_MODE_MagneticSwitchMode;
    writeToRegister( ALERT_CONFIG_ADDRESS, input );

}



//****************************************************************************
//! Disable Magnetic Switch Mode (set back to Interrupt & Trigger Mode)
//!
//! Changes the ALERT_MODE back to Interrupt & Trigger Mode, effectively disabling
//! Magnetic Switch mode without wiping its set threshold registers
//****************************************************************************
void disableMagSwitch()
{
    uint16_t input;
    input = normalReadRegister(ALERT_CONFIG_ADDRESS);
    input &= ~(ALERT_CONFIG_ALERT_MODE_MASK);
    input |= ALERT_CONFIG_ALERT_MODE_InterruptandTriggerMode;
    writeToRegister( ALERT_CONFIG_ADDRESS, input );
}



//****************************************************************************
//! Setup Magnetic Thresholds
//! set_threshold - which threshold is being set (one at a time):
//!                 0x00 = X  |  0x01 = Y  |  0x02 = Z  |  0x03 = T
//! thrx_hi (SIGNED) - signed 8-bit value for the high threshold (check datasheet for conversion eqn - pgs. 38-39)
//! thrx_lo (SIGNED) - signed 8-bit value for the low threshold (check datasheet for conversion eqn - pgs. 38-39)
//!
//! DOES NOT WORK IN SPECIAL READ MODE [DATA_TYPE field at 0x028-6 does not equal 000b]
//****************************************************************************
void magThreshSet( uint8_t set_threshold, int8_t thrx_hi, int8_t thrx_lo )
{
    // To prevent undefined behavior, this function does not perform its operation
    // when the DATA_TYPE field (address: 0x028-6) is not set to Normal Read Mode (000b)
    if ( DATA_TYPE_RESULTS != DATA_TYPE_RESULTS_NormalMode ) return;

    // Check that inputs are valid
    if( !(set_threshold <= 0x03) ) return;

    // Get associated register address (register order is XYZT from 0x04 to 0x07)
    // The set_threshold value can be added to X address to get the desired one
    uint8_t reg_address = X_THRX_CONFIG_ADDRESS + set_threshold;

    // Signed ints were used in the function call to make input easier.
    // Unsigned ints must be used to combine into one dataline/
    uint16_t input = ((uint8_t) thrx_hi << 8) | ((uint8_t) thrx_lo);

    writeToRegister( reg_address, input );
}





//****************************************************************************
//****************************************************************************
//
// Measurement Configuration Functions
//
//****************************************************************************
//****************************************************************************


//****************************************************************************
//! Enable Magnetic Axes for Measurement (also can turn all channels off)
//!
//! Takes in a 4-bit value for the MAG_CH_EN field (0x019-6)
//! mag_ch_en_bits must not be greater than 0x0F
//!
//! When mag_ch_en_bits < 0x08 its three LSBs act as a three bit enable/disable command for ZYX
//! (examples:  0x05 (0101b) => ZX enabled  |  0x02 (0010b) => Y enabled)
//!
//! When mag_ch_en_bits >= 0x08 it configures alternative sampling orders along with enabling
//! specific channels (see chart below)
//!
//!                          | enabled channels ||                  | enabled channels
//!          mag_ch_en_bits  | + sampling order ||  mag_ch_en_bits  | + sampling order
//!         _________________|__________________||__________________|__________________
//!               0x00               none       ||       0x08               XYX
//!               0x01                X         ||       0x09               YXY
//!               0x02                Y         ||       0x0A               YZY
//!               0x03               XY         ||       0x0B               ZYZ
//!               0x04                Z         ||       0x0C               ZXZ
//!               0x05               ZX         ||       0x0D               XZX
//!               0x06               YZ         ||       0x0E              XYZYX
//!               0x07               XYZ        ||       0x0F              XYZZYX
//!
//! Definitions for descriptive inputs to this function are provided in the header file.
//!
//! DOES NOT WORK IN SPECIAL READ MODE [DATA_TYPE field at 0x028-6 does not equal 000b]
//****************************************************************************
void enableMagChannels( uint8_t mag_ch_en_bits )
{
    // To prevent undefined behavior, this function does not perform its operation
    // when the DATA_TYPE field (0x028-6) is not set to Normal Read Mode (000b)
    if ( DATA_TYPE_RESULTS != DATA_TYPE_RESULTS_NormalMode ) return;

    // Check that inputs are valid
    if ( !( mag_ch_en_bits <= 0x0F ) ) return;

    uint16_t input;
    // Set MAG_CH_EN (0x019-6) to mag_ch_en_bits
    input = normalReadRegister(SENSOR_CONFIG_ADDRESS);
    input = ( input & ~(SENSOR_CONFIG_MAG_CH_EN_MASK) ) | (mag_ch_en_bits << 6);
    writeToRegister( SENSOR_CONFIG_ADDRESS, input );
}



//****************************************************************************
//! Enable Angle Measurement (also can turn off angle measurement)
//!
//! Takes in a 2-bit value for the ANGLE_EN field (0x01F-E) to determine which two
//! axes to measure the angle off of for the on-board CORDIC function in the device.
//!
//! Based on the ANGLE_EN setting, the angle will be calculated using the first axis
//! as the "horizontal" axis (positive side of axis is 0 degrees) and the second as
//! the "vertical" axis (positive side of axis is 90degrees)
//!
//! angle_en_bits can be set to 0x00 to 0x03 to configure these settings for ANGLE_EN:
//!              ANGLE_EN value | horizontal axis | vertical axis
//!                   0x00              none            none  --  (angle measurement disabled)
//!                   0x01               X               Y
//!                   0x02               Y               Z
//!                   0x03               X               Z
//****************************************************************************
void enableAngleMeasurement( uint8_t angle_en_bits )
{
    // To prevent undefined behavior, this function does not perform its operation
    // when the DATA_TYPE field (0x028-6) is not set to Normal Read Mode (000b)
    if ( DATA_TYPE_RESULTS != DATA_TYPE_RESULTS_NormalMode ) return;

    // Check that inputs are valid
    if ( !( angle_en_bits <= 0x03 ) ) return;

    // To prevent undefined behavior, this function does not perform its operation
    // when the DATA_TYPE field (0x028-6) is not set to Normal Read Mode (000b)
    if ( DATA_TYPE_RESULTS != DATA_TYPE_RESULTS_NormalMode ) return;

    uint16_t input;
    // Set ANGLE_EN (0x01F-E) to mag_ch_en_bits
    input = normalReadRegister(SENSOR_CONFIG_ADDRESS);
    input = ( input & ~(SENSOR_CONFIG_ANGLE_EN_MASK) ) | (angle_en_bits << 14);
    writeToRegister( SENSOR_CONFIG_ADDRESS, input );
}



//****************************************************************************
//! Enable Temperature Measurement
//!
//! Begins Temperature Measurements by changing the T_CH_EN field in the
//! DEVICE_CONFIG to 1b.
//****************************************************************************
void enableTemperatureMeasurement() {
    uint16_t input = normalReadRegister(DEVICE_CONFIG_ADDRESS);
    input &= ~(DEVICE_CONFIG_T_CH_EN_MASK);
    input |= DEVICE_CONFIG_T_CH_EN_TemperatureChannelEnabled;
    writeToRegister( DEVICE_CONFIG_ADDRESS, input );
}

//****************************************************************************
//! Disable Temperature Measurement
//!
//! Ends Temperature Measurements by changing the T_CH_EN field in the
//! DEVICE_CONFIG to 0b.
//****************************************************************************
void disableTemperatureMeasurement() {
    uint16_t input = normalReadRegister(DEVICE_CONFIG_ADDRESS);
    input &= ~(DEVICE_CONFIG_T_CH_EN_MASK);
    input |= DEVICE_CONFIG_T_CH_EN_TemperatureChannelDisabled;
    writeToRegister( DEVICE_CONFIG_ADDRESS, input );
}



//****************************************************************************
//! Set Sampling Rate (configure the amount of additional samples to reduce noise/increase resolution)
//!
//! CONV_AVG_bits - what value to set for the CONV_AVG (0x00E-C) value (from datasheet pg. 34):
//!                      | num. samples | 3-axes speed | 1-axis speed |
//!                 0x00 -       1x         10.0Ksps         20Ksps
//!                 0x01 -       2x          5.7Ksps       13.3Ksps
//!                 0x02 -       4x          3.1Ksps        8.0Ksps
//!                 0x03 -       8x          1.6Ksps        4.4Ksps
//!                 0x04 -      16x          0.8Ksps        2.4Ksps
//!                 0x05 -      32x          0.4Ksps        1.2Ksps
//****************************************************************************
void setSampleRate( uint8_t CONV_AVG_bits )
{
    uint16_t input;
    // Set CONV_AVG (0x00E-C) to CONV_AVG_bits
    input = normalReadRegister(DEVICE_CONFIG_ADDRESS);
    input = ( input & ~(DEVICE_CONFIG_CONV_AVG_NUM_MASK) ) | (CONV_AVG_bits << 12);
    writeToRegister( DEVICE_CONFIG_ADDRESS, input );
}



//****************************************************************************
//! Set Ranges for X, Y, and Z axes
//!
//! Sets the X, Y, and Z_RANGE fields in the SENSOR_CONFIG register to the bits
//! determined by the function inputs.
//!
//! x_range_bits - bits for X_RANGE field (must be no greater than 0x02)
//! y_range_bits - bits for Y_RANGE field (must be no greater than 0x02)
//! z_range_bits - bits for Z_RANGE field (must be no greater than 0x02)
//!
//! According to the TMAG5170 version used, the mT range for the bits are as follows:
//!
//!                 *_range_bits  |   TMAG5170A1   |   TMAG5170A2
//!                    input      | mT range value | mT range value
//!              _________________|________________|________________
//!                    0x00       |      50 mT     |     150 mT
//!                    0x01       |      25 mT     |      75 mT
//!                    0x02       |     100 mT     |     300 mT
//****************************************************************************
void setRanges( uint8_t x_range_bits, uint8_t y_range_bits, uint8_t z_range_bits )
{
    // To prevent undefined behavior, this function does not perform its operation
    // when the DATA_TYPE field (0x028-6) is not set to Normal Read Mode (000b)
    if ( DATA_TYPE_RESULTS != DATA_TYPE_RESULTS_NormalMode ) return;
    // Check that inputs are valid
    if ( x_range_bits > 0x02 || y_range_bits > 0x02 || z_range_bits > 0x02 ) return;

    uint16_t input = normalReadRegister(SENSOR_CONFIG_ADDRESS) & ~(SENSOR_CONFIG_FULL_RANGE_MASK);
    input |= (z_range_bits << 4) | (y_range_bits << 2) | x_range_bits;
    writeToRegister( SENSOR_CONFIG_ADDRESS, input );
}





//****************************************************************************
//****************************************************************************
//
// Get Results/Measurement Functions (Normal Read Mode)
//
//****************************************************************************
//****************************************************************************


//****************************************************************************
//! Get and return the *_CH_RESULT (or TEMP_RESULT for T) register for an axis/measurement
//!
//! These functions explicitly return the unsigned 16-bit register of their respective
//! measurement address.
//****************************************************************************

uint16_t getXresult() { return normalReadRegister(X_CH_RESULT_ADDRESS); }
uint16_t getYresult() { return normalReadRegister(Y_CH_RESULT_ADDRESS); }
uint16_t getZresult() { return normalReadRegister(Z_CH_RESULT_ADDRESS); }
uint16_t getTEMPresult() { return normalReadRegister(TEMP_RESULT_ADDRESS); }
uint16_t getANGLEresult() { return normalReadRegister(ANGLE_RESULT_ADDRESS); }
uint16_t getMAGresult() { return normalReadRegister(MAGNITUDE_RESULT_ADDRESS); }
// NOTE: These functions returned the unsigned integer corresponding to the register value
//       for easier bit operations. To convert the unsigned using the equations used in the
//       example code, it must be casted to an signed integer.



//****************************************************************************
//! Get Magnetic Results Registers (Normal Read Mode)
//!
//! In 32-bit normal read mode:
//! Takes in a size 3 array of int16_t values and assigns it the the
//! three *_CH_RESULT registers. (order XYZ)
//!
//! INPUT ARRAY MUST BE OF LENGTH 3
//!
//! NOTE: The uint16_t variables returned by the get*result functions are casted
//! into int16_t variables for use with the other provided functions that take
//! signed integers.
//!
//! DOES NOT WORK IN SPECIAL READ MODE [DATA_TYPE field at 0x028-6 does not equal 000b]
//****************************************************************************
void getMagResultsRegistersNrml( int16_t meas_arr[] )
{
    meas_arr[0] = (int16_t) getXresult();
    meas_arr[1] = (int16_t) getYresult();
    meas_arr[2] = (int16_t) getZresult();
}


//****************************************************************************
//! Get X-axis Magnetic Flux Measurement in mT
//!
//! Returns a float containing the mT magnetic flux measurement converted from the
//! X_CH_RESULT register.
//****************************************************************************
float getMeasurementNrmlX()
{

    uint16_t range = getXrange();
    int16_t data = getXresult(); // separate variable used to cast to a signed int
                                         // for the float cast to work correctly

    return (((float) data) / 32768) * range;
}



//****************************************************************************
//! Get Y-axis Magnetic Flux Measurement in mT
//!
//! Returns a float containing the mT magnetic flux measurement converted from the
//! Y_CH_RESULT register.
//****************************************************************************
float getMeasurementNrmlY()
{

    uint16_t range = getYrange();
    int16_t data = getYresult(); // separate variable used to cast to a signed int
                                         // for the float cast to work correctly

    return (((float) data) / 32768) * range;
}



//****************************************************************************
//! Get Z-axis Magnetic Flux Measurement in mT
//!
//! Returns a float containing the mT magnetic flux measurement converted from the
//! Z_CH_RESULT register.
//****************************************************************************
float getMeasurementNrmlZ()
{

    uint16_t range = getZrange();
    int16_t data = getZresult(); // separate variable used to cast to a signed int
                                         // for the float cast to work correctly

    return (((float) data) / 32768) * range;
}



//****************************************************************************
//! Get Temperature Measurement in degrees C (Normal Read Mode)
//!
//! Currently the 'Typical' Electrical Characteristics (ECHAR) of the device are set
//! in the header file. If the device has been calibrated and different ECHAR values
//! are found, please edit the ECHAR values in the header file for more accurate
//! temperature measurement. The header file also contains more information on ECHAR values.
//****************************************************************************
float getMeasurementNrmlTEMP()
{
    uint16_t tADC_T = getTEMPresult();

    float temp_val = ECHAR_T_SENS_T0 + ( ((((float) tADC_T) - ECHAR_T_ADC_T0)) / ECHAR_T_ADC_RES );
    return temp_val;
}



//****************************************************************************
//! Get Internal Angle Measurement in Degrees
//!
//! Returns a float containing the degree value converted from the ANGLE_RESULT register.
//! The value corresponds to the calculated angle created by the two magnetic flux axis
//! measurements selected by the ANGLE_EN bits.
//!
//! For the angle to be properly measured, the two axes selected by ANGLE_EN must share the
//! same selected range.
//****************************************************************************
float getMeasurementNrmlANGLE()
{
    uint16_t data = getANGLEresult();
    float angle = ( (float) data / 16  );
    return angle;
}



//****************************************************************************
//! Get Internal Magnitude Measurement in mT
//!
//! Returns a float containing the mT value converted from the MAGNITUDE_RESULT register.
//! The value corresponds to the calculated magnitude created by the two magnetic flux axis
//! measurements selected by the ANGLE_EN bits.
//!
//! For the magnitude to be properly measured, the two axes selected by ANGLE_EN must share the
//! same selected range.
//****************************************************************************
float getMeasurementNrmlMAG()
{
    uint16_t data = getMAGresult();
    // TODO: verify magnitude correlates with expected
    float magnitude = (((float) data)/8192) * getMAGrange() * 4;
    return magnitude;

}



//****************************************************************************
//! Get Magnetic Measurements in mT (Normal Read Mode)
//!
//! In 32-bit normal read mode:
//! Takes in a size 3 array of floats and updates its measurements of the
//! three magnetic axes in mT. (order XYZ)
//!
//! INPUT ARRAY MUST BE SIZE 3 (or at least have meas_arr to meas_arr + 2 within scope)
//!
//! DOES NOT WORK IN SPECIAL READ MODE [DATA_TYPE field at 0x028-6 does not equal 000b]
//****************************************************************************
void getMagMeasurementsNrml( float meas_arr[] )
{

    uint8_t i;

    // Array to store ranges for coordinates in the order XYZ
    uint16_t ranges[3] = {50,50,50}; // The default value for coordinate ranges is 50 mT (A1)
    ranges[0] = getXrange();
    ranges[1] = getYrange();
    ranges[2] = getZrange();

    int16_t data;

    for (i=0; i<3; ++i)
    {
        data = normalReadRegister(X_CH_RESULT_ADDRESS + i); // read in
        meas_arr[i] = (((float) data) / 32768) * ranges[i];
    }
}





//****************************************************************************
//****************************************************************************
//
// Get Range Functions
//
//****************************************************************************
//****************************************************************************


//****************************************************************************
//! Get and return the integer value of the X_RANGE bits for an axis
//!
//! Returns an unsigned 16-bit integer value of the X axis range in mT.
//****************************************************************************
uint16_t getXrange()
{
    // Get SENSOR_CONFIG and isolate X_RANGE bits.
    uint16_t config = normalReadRegister(SENSOR_CONFIG_ADDRESS) & SENSOR_CONFIG_X_RANGE_MASK;
    uint16_t range;
    if ( getVersion() == 1 )
    {
        // range values for TMAG5170A2
        range = 150;
        if ( config == 0x0001  ) range = 75; // If examined bits equal 01b, range is set to 75 mT (for A2)
        else if ( config == 0x0002 ) range = 300; // If examined bits equal 10b, range is set to 300 mT (for A2)
    }
    else
    {
        // range values for TMAG5170A1
        range = 50;
        if ( config == 0x0001  ) range = 25; // If examined bits equal 01b, range is set to 25 mT (for A1)
        else if ( config == 0x0002 ) range = 100; // If examined bits equal 10b, range is set to 100 mT (for A1)
    }
    return range;
}


//****************************************************************************
//! Get and return the integer value of the Y_RANGE bits for an axis
//!
//! Returns an unsigned 16-bit integer value of the Y axis range in mT.
//****************************************************************************
uint16_t getYrange()
{
    // Get SENSOR_CONFIG and isolate Y_RANGE bits, shifting them to LSB.
    uint16_t config = normalReadRegister(SENSOR_CONFIG_ADDRESS) & SENSOR_CONFIG_Y_RANGE_MASK >> 2;
    uint16_t range;
    if ( getVersion() == 1 )
    {
        // range values for TMAG5170A2
        range = 150;
        if ( config == 0x0001  ) range = 75; // If examined bits equal 01b, range is set to 75 mT (for A2)
        else if ( config == 0x0002 ) range = 300; // If examined bits equal 10b, range is set to 300 mT (for A2)
    }
    else
    {
        // range values for TMAG5170A1
        range = 50;
        if ( config == 0x0001  ) range = 25; // If examined bits equal 01b, range is set to 25 mT (for A1)
        else if ( config == 0x0002 ) range = 100; // If examined bits equal 10b, range is set to 100 mT (for A1)
    }
    return range;
}


//****************************************************************************
//! Get and return the integer value of the Z_RANGE bits for an axis
//!
//! Returns an unsigned 16-bit integer value of the Z axis range in mT.
//****************************************************************************
uint16_t getZrange()
{
    // Get SENSOR_CONFIG and isolate Z_RANGE bits, shifting them to LSB.
    uint16_t config = normalReadRegister(SENSOR_CONFIG_ADDRESS) & SENSOR_CONFIG_Z_RANGE_MASK >> 4;
    uint16_t range;
    if ( getVersion() == 1 )
    {
        // range values for TMAG5170A2
        range = 150;
        if ( config == 0x0001  ) range = 75; // If examined bits equal 01b, range is set to 75 mT (for A2)
        else if ( config == 0x0002 ) range = 300; // If examined bits equal 10b, range is set to 300 mT (for A2)
    }
    else
    {
        // range values for TMAG5170A1
        range = 50;
        if ( config == 0x0001  ) range = 25; // If examined bits equal 01b, range is set to 25 mT (for A1)
        else if ( config == 0x0002 ) range = 100; // If examined bits equal 10b, range is set to 100 mT (for A1)
    }
    return range;
}



//****************************************************************************
//! Get Range used for Magnitude register mT conversion
//!
//! Returns the range selected by the first ANGLE_EN axis. For proper use of the magnitude register
//! both of the ANGLE_EN axes must share the same range.
//!
//! If ANGLE_EN is disabled the function will still return the X-axis range.
//****************************************************************************
uint16_t getMAGrange()
{
    uint16_t angle_en = normalReadRegister(SENSOR_CONFIG_ADDRESS) & SENSOR_CONFIG_ANGLE_EN_MASK;
    if ( angle_en == SENSOR_CONFIG_ANGLE_EN_YZ ) return getYrange();
    else return getXrange();
}



//****************************************************************************
//****************************************************************************
//
// Get Device Info Functions
//
//****************************************************************************
//****************************************************************************


//****************************************************************************
//! Get TMAG5170 Version (A1 or A2)
//!
//! Sends a read command for TEST_CONFIG and returns the VER field bit.
//!      VER == 0b --> TMAG5170A1    |    VER == 1b --> TMAG5170A2
//****************************************************************************
uint8_t getVersion()
{ return (normalReadRegister(TEST_CONFIG_ADDRESS) & TEST_CONFIG_VER_MASK) >> 4; }


//****************************************************************************
//! Check if CRC is enabled
//!
//! Sends a read command for TEST_CONFIG and returns the whether the CRC_DIS field
//! corresponds to an enabled CRC or not. (1b == enabled)
//****************************************************************************
uint8_t isCRCenabled()
{
    return ((normalReadRegister(TEST_CONFIG_ADDRESS) & TEST_CONFIG_CRC_DIS_MASK) >> 2) == 0;
}





//****************************************************************************
//****************************************************************************
//
// Offset and Gain Correction Functions
//
//****************************************************************************
//****************************************************************************


//****************************************************************************
//! Set Magnetic Sensor Offset Correction (7-Bit inputs)
//! Configures the MAG_OFFSET_CONFIG (0x12) according to input values (check datasheet pg. 44)
//!
//! Keep in mind the axes that offset1 and offset2 are applied to depend on ANGLE_EN (0x01F-E)
//!              ANGLE_EN value | offset1 axis | offset2 axis
//!                   0x00            none           none
//!                   0x01              X              Y
//!                   0x02              Y              Z
//!                   0x03              X              Z
//!
//! offset_select - 2-bit setting for OFFSET_SELECTION (0x12F-E) [must be no greater than 0x03]
//! offset1_bits - 7-bit setting for OFFSET_VALUE1 (0x12D-7) [must be no greater than 0x7F]
//! offset2_bits - 7-bit setting for OFFSET_VALUE2 (0x126-0) [must be no greater than 0x7F]
//!
//! Conversion between MAG_OFFSET_CONFIG register and actual offset deltas is on datasheet pgs. 26 and 27
//****************************************************************************
void setMagOffsetIn7Bit( uint8_t offset_select, uint8_t offset1_bits , uint8_t offset2_bits )
{
    if (offset_select > 0x03 || offset1_bits > 0x7F || offset2_bits > 0x7F) return;

    uint16_t input;
    // MAG_OFFSET_CONFIG (0x12) has all 16 bits assigned according to the three input variables
    input = offset_select << 14 | offset1_bits << 7 | offset2_bits;
    writeToRegister( MAG_OFFSET_CONFIG_ADDRESS, input );
}



//****************************************************************************
//! Set Magnetic Sensor Offset Correction (mT inputs)
//! Configures the MAG_OFFSET_CONFIG (0x12) according to input values (check datasheet pg. 44)
//!
//! Keep in mind the axes that offset1 and offset2 are applied to depend on ANGLE_EN (0x01F-E)
//!              ANGLE_EN value | offset1 axis | offset2 axis
//!                   0x00            none           none
//!                   0x01              X              Y
//!                   0x02              Y              Z
//!                   0x03              X              Z
//!
//! offset_select - setting for OFFSET_SELECTION (0x12F-E) [must be no greater than 0x03]
//! offset1_delta - mT value of offset shift for the selected offset1 axis
//! offset2_delta - mT value of offset shift for the selected offset2 axis
//! (an axis with a +2mT error should have a -2mT value entered into its offset variable)
//!
//! Valid offset values for each possible range setting:
//!          SET AXIS RANGE   |  VALID OFFSET VALUES RANGE  (mT)
//!            25 mT (A1)               -0.78 to 0.76
//!            50 mT (A1)               -1.56 to 1.53
//!            75 mT (A2)               -2.34 to 2.30
//!           100 mT (A1)               -3.12 to 3.07
//!           150 mT (A2)               -4.68 to 4.61
//!           300 mT (A2)               -9.37 to 9.22
//!
//! (valid values were rounded to the hundredths place closer to zero to prevent including invalid
//! values in the range, the exact corresponding offset based on the MAG_OFFSET_CONFIG register can be
//! calculated using the eqns. provided on datasheet pgs. 26 and 27)
//****************************************************************************
void setMagOffsetInmT( uint8_t offset_select, float offset1_delta , float offset2_delta )
{

    uint16_t delta1_range = 0;
    uint16_t delta2_range = 0;
    switch ( offset_select )
    {
        case 0x01 :
            delta1_range = getXrange();
            delta2_range = getYrange();
        case 0x02 :
            delta1_range = getYrange();
            delta2_range = getZrange();
        case 0x03 :
            delta1_range = getXrange();
            delta2_range = getZrange();
    }

    // take the rounded inverse of the eqn to get the signed 8-bit int value for
    // the desired offset and then convert that to 7-bits to send to the device
    // (the 7-bit value is put in the 7 LSBs of an 8-bit unsigned int)
    int8_t offset1_value = round( (offset1_delta * 2048) / delta1_range );
    int8_t offset2_value = round( (offset2_delta * 2048) / delta2_range );

    uint8_t offset1_bits = ( (uint8_t) offset1_value ) & 0x3F ; // assign the 6 LSBs
    offset1_bits = (( (uint8_t) offset1_value ) & 0x80) >> 1; // shift signing bit to 7th LSB
    uint8_t offset2_bits = ( (uint8_t) offset2_value ) & 0x3F ;
    offset2_bits = (( (uint8_t) offset2_value ) & 0x80) >> 1;

    setMagOffsetIn7Bit(offset_select, offset1_bits, offset2_bits);
}



//****************************************************************************
//! Set Magnetic Gain Adjustment using 11-bit input
//! Configures the MAG_GAIN_CONFIG (0x11) according to 11-bit input value
//! (check datasheet pg. 43 for conversion)
//!
//! axis - selection of a particular axis for amplitude correction (must be no greater than 0x03)
//!        0x00 - none | 0x01 - X | 0x02 - Y | 0x03 - Z
//! gain_bits - 11-bit gain value to adjust selected axis value (must be no greater than 0x07FF)
//!             gain calculated as (entered_value/1024)
//****************************************************************************
void setMagGainConfigIn11Bit( uint8_t axis, uint16_t gain_bits )
{
    if (axis > 0x03 || gain_bits > 0x07FF) return;
    uint16_t input;
    // MAG_OFFSET_CONFIG (0x11) has all 16 bits assigned according to the three input variables
    input = axis << 14 | gain_bits;
    writeToRegister( MAG_OFFSET_CONFIG_ADDRESS, input );
}



//****************************************************************************
//! Set Magnetic Gain Adjustment using decimal input
//! Configures the MAG_GAIN_CONFIG (0x11) according to decimal input values from
//! 0 to 2 (exclusive)
//!
//! axis - selection of a particular axis for amplitude correction (must be no greater than 0x03)
//!        0x00 - none | 0x01 - X | 0x02 - Y | 0x03 - Z
//! gain_value - float input of the desired gain value from 0 to 2 (exclusive) to
//!              be assigned with the selected axis
//****************************************************************************
// takes in a value from 0 to 2 (exclusive) and sets the gain config according to it
void setMagGainConfigInDecimal( uint8_t axis, float gain_value )
{
    if ( gain_value < 0 || gain_value >= 2 ) return;
    uint16_t gain_bits = floor(gain_value * 1024);
    setMagGainConfigIn11Bit( axis, gain_bits );
}





//****************************************************************************
//! Special 32-bit Data Read Functions (DATA_TYPE != 000b)
//!
//! !! IMPORTANT INFORMATION ON USING THE SPECIAL READ SETTINGS !!
//!
//! When the DATA_TYPE field (0x028-6) is not set to 0x00 (000b) the device
//! changes its register READ response to a special 32-bit dataline that sends
//! the 12 MSBs of two selected measurement values at the cost of not being
//! able to send full registers in response to a READ command.
//!
//! ANY READ COMMAND SENT BY SPI TO THE DEVICE IN SPECIAL READ MODE WILL NOT
//! HAVE THE DESIRED ADDRESS RETURNED!
//!
//! Because of this, the DATA_TYPE field must at least be stored locally by
//! whatever controller used for the device to use Special Read in an effective manner.
//! This example code stores the entire SYSTEM_CONFIG register so that DATA_TYPE
//! can be written back to 0x00 without altering any other SYSTEM_CONFIG settings.
//!
//! The settings for DATA_TYPE and the measurement values that are returned in
//! the special dataline are as follows:
//!
//!              DATA_TYPE value | channel 1 value | channel 2 value
//!                   0x00          default 32-bit register access
//!                   0x01                X                Y
//!                   0x02                X                Z
//!                   0x03                Z                Y
//!                   0x04                X           temperature
//!                   0x05                Y           temperature
//!                   0x06                Z           temperature
//!                   0x07              angle          magnitude
//****************************************************************************

//****************************************************************************
//! DATA_TYPE (0x028-6) setter function
//! This function takes in an input from 0x00 to 0x07 to set the DATA_TYPE field to.
//! The associated measurements sent by the special read are shown above.
//!
//! This function uses SYSTEM_CONFIG_stored variable to change SYSTEM_CONFIG
//! without a read command.
//!
//! THE STARTUP FUNCTION MUST BE USED AT THE START OF AN IMPLEMENTATION TO PREVENT
//! ANY UNWANTED BEHAVIOR ON THE DATA_TYPE WRITE.
//****************************************************************************
void setDATATYPE( uint8_t data_type_bits )
{
    uint16_t input = SYSTEM_CONFIG_stored;
    input &= ~(SYSTEM_CONFIG_DATA_TYPE_MASK);
    input |= data_type_bits << 6;
    writeToRegister( SYSTEM_CONFIG_ADDRESS, input );
}


//****************************************************************************
//! Get Special Read Mode (DATA_TYPE != 000b) Values in Specified Units
//!
//! Takes in measurement array of length 3 and converts the received channel
//! measurements according to the DATA_TYPE field value.
//!
//! The DATA_TYPE field value will be put into
//!
//!      DATA_TYPE value | CH1 measurement + unit | CH2 measurement + unit
//!           0x01                 X + mT                   Y + mT
//!           0x02                 X + mT                   Z + mT
//!           0x03                 Z + mT                   Y + mT
//!           0x04                 X + mT           temperature + Celsius
//!           0x05                 Y + mT           temperature + Celsius
//!           0x06                 Z + mT           temperature + Celsius
//!           0x07             angle + degrees        magnitude + mT
//****************************************************************************
void getMeasurementsSpcl( float meas_arr[] )
{
    uint16_t output[3] = { 0 };
    specialRead( output, 0x00 );
    meas_arr[2] = (float) DATA_TYPE_RESULTS;

    if ( DATA_TYPE_RESULTS <= DATA_TYPE_RESULTS_SpecialMode_YZ )
    {
        meas_arr[0] = (output[0] >> 11) * -2048;
        meas_arr[1] = (output[1] >> 11) * -2048;
    }
    else if ( DATA_TYPE_RESULTS <= DATA_TYPE_RESULTS_SpecialMode_ZT )
    {
        meas_arr[0] = (output[0] >> 11) * -2048;
        meas_arr[1] = ECHAR_T_SENS_T0 + ( 16 * (((float) output[1]) - (ECHAR_T_ADC_T0/16)) / ECHAR_T_ADC_RES );
    }

    switch ( DATA_TYPE_RESULTS )
    {
        case DATA_TYPE_RESULTS_SpecialMode_XY :
            meas_arr[0] *= ((((float) (meas_arr[0] + output[0])) / 2048) * getXrange());
            meas_arr[1] *= ((((float) (meas_arr[1] + output[1])) / 2048) * getYrange());
            break;
        case DATA_TYPE_RESULTS_SpecialMode_XZ :
            meas_arr[0] *= ((((float) (meas_arr[0] + output[0])) / 2048) * getXrange());
            meas_arr[1] *= ((((float) (meas_arr[1] + output[1])) / 2048) * getZrange());
            break;
        case DATA_TYPE_RESULTS_SpecialMode_YZ :
            meas_arr[0] *= ((((float) (meas_arr[0] + output[0])) / 2048) * getZrange());
            meas_arr[1] *= ((((float) (meas_arr[1] + output[1])) / 2048) * getYrange());
            break;
        case DATA_TYPE_RESULTS_SpecialMode_XT :
            meas_arr[0] *= ((((float) (meas_arr[0] + output[0])) / 2048) * getXrange());
            break;
        case DATA_TYPE_RESULTS_SpecialMode_YT :
            meas_arr[0] *= ((((float) (meas_arr[0] + output[0])) / 2048) * getYrange());
            break;
        case DATA_TYPE_RESULTS_SpecialMode_ZT :
            meas_arr[0] *= ((((float) (meas_arr[0] + output[0])) / 2048) * getZrange());
            break;
        case DATA_TYPE_RESULTS_SpecialMode_AM :
            meas_arr[0] = (float) output[0] / 8 ;
            meas_arr[1] = (((float) output[1]) / 4096) * getMAGrange() * 4;
            break;
    }
}





//****************************************************************************
//****************************************************************************
//
// Supplemental Functions
//
//****************************************************************************
//****************************************************************************


//****************************************************************************
//! Calculate Angle and Magnitude using CORDIC for two axes
//! Takes in a float array of at least size 2, two magnetic axis measurements, and their
//! shared range and changes indexes 0 and 1 of the array to the calculated angle and
//! magnitude using CORDIC.
//!
//! CORDIC_results[] - float array of at least size 2, will have indexes 0 and 1 replaced
//!                    with the calculated angle and magnitude, respectively
//! numerator - magnetic measurement result pulled from the register of the vertical axis
//! denominator - magnetic measurement result pulled from the register of the horizontal axis
//! range - the range in mT shared by both axes (axes cannot have different set ranges or CORDIC will not
//!         be accurate)
//!
//! For angle measurements to match the in-built CORDIC on the device, match the numerator and denominator
//! for the associated ANGLE_EN value below, while also ensuring both axes share the same range (in mT):
//!
//!              ANGLE_EN value | numerator axis | denominator axis
//!                   0x00             none             none
//!                   0x01              Y                X
//!                   0x02              Z                Y
//!                   0x03              Z                X
//!
//! The returned angle will be interpreted as 0deg on the positive denominator axis and 90deg on the positive numerator axis
//!
//! For more information on CORDIC algorithms please watch the "CORDIC algorithm for angle calculations" video
//! provided by Texas Instruments: https://training.ti.com/cordic-algorithm-angle-calculations
//****************************************************************************
void calcCORDIC(float CORDIC_results[], int16_t numerator, int16_t denominator, uint16_t range,
                        int16_t iteration_length)
{
    if (iteration_length > 16 || iteration_length < 1) return;

    int32_t angle_readings[3];
    int32_t ANGLE_CALC_32;
    int32_t MAG_CALC_CORDIC;

    atan2CORDIC(numerator, denominator, iteration_length, angle_readings);

    ANGLE_CALC_32 = angle_readings[0];
    MAG_CALC_CORDIC = angle_readings[1];

    CORDIC_results[0] = (((float) ANGLE_CALC_32)/65536) * 360 / 65536; // angle result
    CORDIC_results[1] = ((float) MAG_CALC_CORDIC) * range / 32768; // mag result
}


//****************************************************************************
//! atan2 + magnitude calculation using CORDIC algorithm
//!
//! Implementation of the CORDIC algorithm without result value conversion for
//! faster use with functions repeatedly using the CORDIC algorithm (see the
//! planeAngles function).
//!
//! numerator - magnetic measurement result pulled from the register of the vertical axis
//! denominator - magnetic measurement result pulled from the register of the horizontal axis
//! iteration_length - the number of "rotations" to be made in the calculation, more generally
//!                    means a more accurate calculation (max amount is 16)
//! results - int32_t array of at lease size 2. results[0] will store the unconverted angle
//!           value calculated by the algorithm. results[1] will store the unconverted
//!           magnitude value. See the calcCORDIC function for how to convert these values
//!           using the lookup tables included.
//!
//! For more information on CORDIC algorithms please watch the "CORDIC algorithm for angle calculations" video
//! provided by Texas Instruments: https://training.ti.com/cordic-algorithm-angle-calculations
//****************************************************************************
void atan2CORDIC(int16_t numerator, int16_t denominator, int16_t iteration_length, int32_t results[])
{
    if (iteration_length > 16 || iteration_length < 1) return;

    int i=0;
    int32_t num_old, den_old, num, den;
    uint32_t angle;
    num = num_old = numerator;
    den = den_old = denominator;

    if (den < 0) angle = 0x80000000;
    else angle = 0;

    for(i = 0 ; i < iteration_length ; i++)
    {

        if (((den >= 0) && (num < 0)) ||((den < 0) && (num >= 0)))
        {
            den = den - (num_old >> i);
            num = num + (den_old >> i);
            angle = angle - atanArray32[i];
        }
        else
        {
            den = den + (num_old >> i);
            num = num - (den_old >> i);
            angle = angle + atanArray32[i];
        }
        den_old = den;
        num_old = num;
    }
    results[0] = angle;
    if (den < 0) den = -den;
    results[1]= (((int64_t)den)*magArray[i-1])>>(15+16);
}



//****************************************************************************
//! Plane Angle Calculations
//!
//! Takes in the result registers of three axes (assuming all share the same range)
//! and an output array of at lease size 3.
//!
//! results[0] will correspond to the YZ plane angle value.
//! results[1] will correspond to the XZ plane angle value.
//! results[2] will correspond to the XY plane angle value (if enabled).
//****************************************************************************
void planeAngles(int16_t axisX, int16_t axisY, int16_t axisZ,  int32_t results[])
{
    axisX >>= 1;
    axisY >>= 1;
    axisZ >>= 1;
    int32_t angle_mag_readings[2];
    uint32_t xz_magnitude, yz_magnitude;


    atan2CORDIC(axisZ, axisY, 10, angle_mag_readings);
    yz_magnitude=angle_mag_readings[1];
    atan2CORDIC(yz_magnitude, axisX, 10, angle_mag_readings);
    results[0]=angle_mag_readings[0];

    atan2CORDIC(axisZ, axisX, 10, angle_mag_readings);
    xz_magnitude=angle_mag_readings[1];
    atan2CORDIC(xz_magnitude, axisY, 10, angle_mag_readings);
    results[1]=angle_mag_readings[0];

//#define CALCULATE_XY_PLANE_ANGLE
#ifdef CALCULATE_XY_PLANE_ANGLE
    uint32_t xy_magnitude;
    atan2CORDIC(axisX, axisY, 10, angle_mag_readings);
    xy_magnitude=angle_mag_readings[1];
    atan2CORDIC(xy_magnitude, axisZ, 10, angle_mag_readings);
    results[2]=angle_mag_readings[0];
#endif
}


//****************************************************************************
//! Conversion to Spherical Coordinates
//!
//! Takes in the result registers of three axes (assuming all share the same range)
//! and an output array of at lease size 3. The three axis measurements are converted
//! into spherical coordinates with phi on the axis1 + axis2 plane.
//!
//! results[0] will correspond to the radius value.
//! results[1] will correspond to the phi angle value on the axis1 + axis2 plane.
//! results[2] will correspond to the theta angle value.
//****************************************************************************
void convertToSpherical(int16_t axis1, int16_t axis2, int16_t axis3,  int32_t results[])
{
    int32_t angle_mag_readings[2];
    uint32_t xy_magnitude;


    results[0]= mag3D(axis1, axis2, axis3);

    atan2CORDIC(axis2, axis1, 10, angle_mag_readings);
    results[1]=angle_mag_readings[0];
    xy_magnitude=angle_mag_readings[1];

    atan2CORDIC(xy_magnitude, axis3, 10, angle_mag_readings);
    results[2]=angle_mag_readings[0];

}


//****************************************************************************
//! Conversion to Cylindrical Coordinates
//!
//! Takes in the result registers of three axes (assuming all share the same range)
//! and an output array of at lease size 3. The three axis measurements are converted
//! into cylindrical coordinates with the cylinder oriented along axis3.
//!
//! results[0] will correspond to the radius value.
//! results[1] will correspond to the theta angle value of the axis1 + axis2 plane.
//! results[2] will correspond to the z value.
//****************************************************************************
void convertToCylindrical(int16_t axis1, int16_t axis2, int16_t axis3,  int32_t results[])
{
    int32_t angle_mag_readings[2];

    atan2CORDIC(axis2, axis1, 10, angle_mag_readings);
    results[0]=angle_mag_readings[1];
    results[1]=angle_mag_readings[0];
    results[2]=axis3;
}


//****************************************************************************
//! 3-axis Magnitude Calculation
//!
//! Takes in three result registers and returns the magnitude of a vector created
//! by those three input values.
//****************************************************************************
uint32_t mag3D(int16_t axis1, int16_t axis2, int16_t axis3)
{
    uint32_t temp1, temp2,temp3;

    temp3= axis3>>1;
    temp3=temp3*temp3;

    temp2=axis2>>1;
    temp2= temp2*temp2;

    temp1= axis1>>1;
    temp1=temp1*temp1;


    temp1=temp3+temp2+temp1;
    temp1=isqrt32(temp1)>>16;
    return temp1<<1;
}




//****************************************************************************
//! Piecewise Linearization Function - Axis Measurements (Registers)
//! Takes in known measurements with their associated errors and uses a linearization of them
//! to calculate an estimation of an unknown measured value with error absent.
//!
//! knownValue[] - sorted array of measured register values with their error known (MUST BE SORTED LOW to HIGH)
//! knownError[] - array of known error values (in mT) for the corresponding knownValue[] indices
//! known_length - length of the knownValue[] and knownError[] arrays
//! measValue - the measured register value with unknown error the linearization will be applied to.
//! range - range of the axis the registers to be linearized are taken from
//****************************************************************************
float piecewiseLinearizationRegister( int16_t knownValue[], float knownError[], uint16_t known_length, int16_t measValue, uint16_t range )
{

    uint16_t top_index;

    for ( top_index = 1; top_index < known_length - 1 ; top_index++ )
    {
        if ( measValue < knownValue[top_index] ) break;
    }

    float coord_diff, error_diff, slope, estimated_error, linztn_value;
    float bottomValue_f = resultRegisterTomT(knownValue[top_index - 1], range);
    float measValue_f = resultRegisterTomT(measValue, range);

    UNUSED_VARIABLE(linztn_value);

    coord_diff = ((float) (knownValue[top_index] - knownValue[top_index - 1]) / 32768) * range;
    error_diff = knownError[top_index] - knownError[top_index - 1];
    slope = error_diff / coord_diff;

    estimated_error = ( measValue_f - bottomValue_f )*slope + knownError[top_index - 1];
    return measValue_f - estimated_error;
}



//****************************************************************************
//! Piecewise Linearization Function - Axis Measurements (mT values)
//! Takes in known measurements with their associated errors and uses a linearization of them
//! to calculate an estimation of an unknown measured value with error absent.
//!
//! knownValue[] - sorted array of measured mT values with their error known (MUST BE SORTED LOW to HIGH)
//! knownError[] - array of known error values (in mT) for the corresponding knownValue[] indices
//! known_length - length of the knownValue[] and knownError[] arrays
//! measValue - the measured mT value with unknown error the linearization will be applied to.
//! range - range of the axis the registers to be linearized are taken from
//****************************************************************************
float piecewiseLinearizationFloat( float knownValue[], float knownError[], uint16_t known_length, float measValue )
{
    uint16_t top_index;
    float coord_diff, error_diff;
    float slope, estimated_error;

    for ( top_index = 1; top_index < known_length - 1 ; top_index++ )
    {
        if ( measValue < knownValue[top_index] ) break;
    }

    coord_diff = knownValue[top_index] - knownValue[top_index - 1];
    error_diff = knownError[top_index] - knownError[top_index - 1];
    slope =  error_diff / coord_diff;
    estimated_error = ( measValue - knownValue[top_index - 1] )*slope + knownError[top_index - 1];

    return measValue - estimated_error;
}



//****************************************************************************
//! Piecewise Linearization Function - Angle Measurements (Register values)
//! Takes in known angle measurements with their associated errors and uses a linearization of them
//! to calculate an estimation of an unknown measured angle with error absent.
//!
//! knownValue[] - sorted array of measured angle register values with their error known (MUST BE SORTED LOW to HIGH)
//! knownError[] - array of known error values (in degrees) for the corresponding knownValue[] indices
//! known_length - length of the knownValue[] and knownError[] arrays
//! measValue - the measured angle register value with unknown error the linearization will be applied to.
//****************************************************************************
float piecewiseLinearizationAngle( uint16_t knownAngle[], float knownError[], uint16_t known_length, uint16_t measAngle )
{

    uint16_t top_index = known_length;
    uint16_t bottom_index;
    float slope, coord_diff, error_diff, estimated_error, linztn_angle;

    if ( measAngle >= knownAngle[0] )
    {
        for ( top_index = 1; top_index < known_length ; top_index++ )
        {
            if ( measAngle < knownAngle[top_index] ) break;
        }
    }
    bottom_index = top_index - 1;
    if ( top_index == known_length )
    {
        top_index = 0;
        coord_diff = angleRegisterToDeg((knownAngle[top_index] + 0x1680) - knownAngle[bottom_index]);
    }
    else
    {
        coord_diff = angleRegisterToDeg(knownAngle[top_index] - knownAngle[bottom_index]);
    }

    float bottomAngle_f = angleRegisterToDeg(knownAngle[bottom_index]);
    float measAngle_f = angleRegisterToDeg(measAngle);

    error_diff = knownError[top_index] - knownError[bottom_index];
    slope = error_diff / coord_diff;
    estimated_error = ( measAngle_f - bottomAngle_f )*slope + knownError[bottom_index];
    linztn_angle = measAngle_f - estimated_error;
    if ( linztn_angle >= 360 ) linztn_angle -= 360;

    return linztn_angle;
}



//****************************************************************************
//! Piecewise Linearization Function - Angle Measurements (mT values)
//! Takes in known angle measurements with their associated errors and uses a linearization of them
//! to calculate an estimation of an unknown measured angle value with error absent.
//!
//! knownValue[] - sorted array of measured degree values with their error known (MUST BE SORTED LOW to HIGH)
//! knownError[] - array of known error values (in degrees) for the corresponding knownValue[] indices
//! known_length - length of the knownValue[] and knownError[] arrays
//! measValue - the measured degree value with unknown error the linearization will be applied to.
//****************************************************************************
float piecewiseLinearizationAngleFloat( float knownAngle[], float knownError[], uint16_t known_length, float measAngle )
{

    uint16_t top_index = known_length;
    uint16_t bottom_index;
    float slope, coord_diff, error_diff, estimated_error, linztn_angle;

    if ( measAngle >= knownAngle[0] )
    {
        for ( top_index = 1; top_index < known_length ; top_index++ )
        {
            if ( measAngle < knownAngle[top_index] ) break;
        }
    }
    bottom_index = top_index - 1;
    if ( top_index == known_length )
    {
        top_index = 0;
        coord_diff = (knownAngle[top_index] + 360) - knownAngle[bottom_index];
    }
    else
    {
        coord_diff = knownAngle[top_index] - knownAngle[bottom_index];
    }

    error_diff = knownError[top_index] - knownError[bottom_index];
    slope = error_diff / coord_diff;
    estimated_error = ( measAngle - knownAngle[bottom_index] )*slope + knownError[bottom_index];
    linztn_angle = measAngle - estimated_error;
    if ( linztn_angle >= 360 ) linztn_angle -= 360;

    return linztn_angle;
}





//****************************************************************************
//****************************************************************************
//
// Helper Functions
//
//****************************************************************************
//****************************************************************************


//****************************************************************************
//! 32-bit Square Root Function
//!
//! Takes the square of the input integer h and returns a 32-bit value that can
//! be converted to the floating point value by dividing it by 65536.
//****************************************************************************
uint32_t isqrt32(uint32_t h)
{
    uint32_t x;
    uint32_t y;
    int i;
    x = y = 0;
    for (i = 0;  i < 32;  i++)
    {
        x = (x << 1) | 1;
        if (y < x) x -= 2;
        else y -= x;
        x++;
        y <<= 1;
        if ((h & 0x80000000)) y |= 1;
        h <<= 1;
        y <<= 1;
        if ((h & 0x80000000)) y |= 1;
        h <<= 1;
    }
    return  x;
}

//****************************************************************************
//! Convert Axis Measurement Result Register to mT value
//!
//! Takes in the register of one of the magnetic axis results and the range for
//! that specific axis and calculates the mT value the register represents.
//****************************************************************************
float resultRegisterTomT( int16_t register_bits, uint16_t range )
{
    return (((float) register_bits) / 32768) * range;
}



//****************************************************************************
//! Convert Angle Result Register to degree value
//!
//! Takes in the bits of the angle result register and converts it to a
//! degree value.
//****************************************************************************
float angleRegisterToDeg( uint16_t register_bits )
{
    return ( (float) register_bits / 16 );
}

//****************************************************************************
//! Calculate CRC for SPI data frame
//!
//! Takes in an array containing a SPI data frame (MSB to LSB) with the CRC bits
//! all set to ZERO and calculates and returns the CRC for that data frame.
//****************************************************************************
uint8_t calculateCRC( uint8_t data[] )
{
    int i = 0;
    uint8_t crc = 0x00;
    uint32_t n;

    // Build TX and RX byte array
    uint8_t d[32] = { 0 };

    n = (data[0] << 24)|(data[1] << 16)|(data[2] << 8)|(data[3]);

    while (n>0)
    {
        d[i] = n&1;
        n = n>>1;
        i++;
    }

    crc |= d[30] ^ d[26] ^ d[25] ^ d[24] ^ d[23] ^ d[21] ^ d[19] ^ d[18] ^ d[15] ^ d[11] ^ d[10] ^ d[9] ^ d[8] ^ d[6] ^ d[4] ^ d[3] ^ d[0] ^ 1;
    crc |= (d[31] ^ d[30] ^ d[27] ^ d[23] ^ d[22] ^ d[21] ^ d[20] ^ d[18] ^ d[16] ^ d[15] ^ d[12] ^ d[8] ^ d[7] ^ d[6] ^ d[5] ^ d[3] ^ d[1] ^ d[0] ^ 1 ^ 1) << 1;
    crc |= (d[31] ^ d[28] ^ d[24] ^ d[23] ^ d[22] ^ d[21] ^ d[19] ^ d[17] ^ d[16] ^ d[13] ^ d[9] ^ d[8] ^ d[7] ^ d[6] ^ d[4] ^ d[2] ^ d[1] ^ 1 ^ 1) << 2;
    crc |= (d[29] ^ d[25] ^ d[24] ^ d[23] ^ d[22] ^ d[20] ^ d[18] ^ d[17] ^ d[14] ^ d[10] ^ d[9] ^ d[8] ^ d[7] ^ d[5] ^ d[3] ^ d[2] ^ 1) << 3;

    return crc;
}


//****************************************************************************
//! Verify CRC in SPI data frame
//!
//! Takes in an array containing a SPI data frame (MSB to LSB) and checks if the
//! CRC bits (according to their locations for the TMAG5170) are correct according
//! to the CRC calculation algorithm.
//****************************************************************************
uint8_t verifyCRC( uint8_t data[] )
{
    uint8_t crc_received = data[3] & 0x0F;
    data[3] &= ~(0x0F); // the CRC bits of the data must be 0000b to calculate its CRC correctly
    uint8_t crc_calc = calculateCRC(data);
    data[3] |= crc_received; // the previously removed CRC bits are reinserted

    return crc_received == crc_calc;
}


//****************************************************************************
//! Pulse CS function
//!
//! This function pulses LOW on the GPIO pin connected to the CS pin of the TMAG5170
//! for 2 us. (set pin to HIGH afterwards)
//!
//! Can be used to trigger conversion for TRIGGER_MODE set to 'at CS pulse'
//****************************************************************************
void csPulse()
{
    setCS(LOW);
    delay_us(2);
    setCS(HIGH);
}


//****************************************************************************
//! Pulse ALERT function
//!
//! This function pulses LOW on the GPIO pin connected to the ALERT pin of the TMAG5170
//! for 2 us. (set pin to HIGH afterwards)
//!
//! Can be used to trigger conversion for TRIGGER_MODE set to 'at ALERT pulse'
//****************************************************************************
void alertPulse()
{
    setALERT(LOW);
    delay_us(2);
    setALERT(HIGH);
}

