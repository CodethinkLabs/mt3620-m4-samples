/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#ifndef LSM6DS3_H_
#define LSM6DS3_H_

#include <stdbool.h>
#include <stdint.h>
#include "lib/I2CMaster.h"

// Variable names and comments come from the LSM6DS3 datasheet, which can be found here:
// https://www.st.com/resource/en/datasheet/lsm6ds3.pdf

/// <summary>This enum contains a set of registers that are used to control the behaviour of the LSM6DS3 device.</summary>
typedef enum {
    LSM6DS3_REG_FUNC_CFG_ACCESS        = 0x01,
    LSM6DS3_REG_SENSOR_SYNC_TIME_FRAME = 0x04,
    LSM6DS3_REG_FIFO_CTRL1             = 0x06,
    LSM6DS3_REG_FIFO_CTRL2             = 0x07,
    LSM6DS3_REG_FIFO_CTRL3             = 0x08,
    LSM6DS3_REG_FIFO_CTRL4             = 0x09,
    LSM6DS3_REG_FIFO_CTRL5             = 0x0A,
    LSM6DS3_REG_ORIENT_CFG_G           = 0x0B,
    LSM6DS3_REG_INT1_CTRL              = 0x0D,
    LSM6DS3_REG_INT2_CTRL              = 0x0E,
    LSM6DS3_REG_WHO_AM_I               = 0x0F,
    LSM6DS3_REG_CTRL1_XL               = 0x10,
    LSM6DS3_REG_CTRL2_G                = 0x11,
    LSM6DS3_REG_CTRL3_C                = 0x12,
    LSM6DS3_REG_CTRL4_C                = 0x13,
    LSM6DS3_REG_CTRL5_C                = 0x14,
    LSM6DS3_REG_CTRL6_C                = 0x15,
    LSM6DS3_REG_CTRL7_G                = 0x16,
    LSM6DS3_REG_CTRL8_XL               = 0x17,
    LSM6DS3_REG_CTRL9_XL               = 0x18,
    LSM6DS3_REG_CTRL10_C               = 0x19,
    LSM6DS3_REG_MASTER_CONFIG          = 0x1A,
    LSM6DS3_REG_WAKE_UP_SRC            = 0x1B,
    LSM6DS3_REG_TAP_SRC                = 0x1C,
    LSM6DS3_REG_D6D_SRC                = 0x1D,
    LSM6DS3_REG_STATUS_REG             = 0x1E,
    LSM6DS3_REG_OUT_TEMP_L             = 0x20,
    LSM6DS3_REG_OUT_TEMP_H             = 0x21,
    LSM6DS3_REG_OUTX_L_G               = 0x22,
    LSM6DS3_REG_OUTX_H_G               = 0x23,
    LSM6DS3_REG_OUTY_L_G               = 0x24,
    LSM6DS3_REG_OUTY_H_G               = 0x25,
    LSM6DS3_REG_OUTZ_L_G               = 0x26,
    LSM6DS3_REG_OUTZ_H_G               = 0x27,
    LSM6DS3_REG_OUTX_L_XL              = 0x28,
    LSM6DS3_REG_OUTX_H_XL              = 0x29,
    LSM6DS3_REG_OUTY_L_XL              = 0x2A,
    LSM6DS3_REG_OUTY_H_XL              = 0x2B,
    LSM6DS3_REG_OUTZ_L_XL              = 0x2C,
    LSM6DS3_REG_OUTZ_H_XL              = 0x2D,
} LSM6DS3_reg_e;

/// <summary>Bit field description for register STATUS_REG.</summary>
typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        /// <summary>
        /// <para>Accelerometer new data available. Default value: false.</para>
        /// <para>false: no set of data available at accelerometer output</para>
        /// <para>true: a new set of data is available at accelerometer output.</para>
        /// </summary>
        bool     xlda    : 1;
        /// <summary>
        /// <para>Gyroscope new data available. Default value: false.</para>
        /// <para>false: no set of data available at gyroscope output</para>
        /// <para>true: a new set of data is available at gyroscope output.</para>
        /// </summary>
        bool     gda     : 1;
        /// <summary>
        /// <para>Temperature new data available. Default: false.</para>
        /// <para>false: no set of data is available at temperature sensor output</para>
        /// <para>true: a new set of data is available at temperature sensor output.</para>
        /// </summary>
        bool     tda     : 1;
        unsigned res_7_3 : 5;
    };

    uint8_t mask;
} LSM6DS3_status_t;

/// <summary>Bit field description for register CTRL1_XL.</summary>
typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        /// <summary>
        /// <para>Anti-aliasing filter bandwidth selection. Default value: 00.</para>
        /// <para>(00: 400 Hz; 01: 200 Hz; 10: 100 Hz; 11: 50 Hz).</para>
        /// </summary>
        unsigned bw_xl  : 2;
        /// <summary>
        /// <para>Accelerometer full-scale selection. Default value: 00.</para>
        /// <para>(00: +/-2 g; 01: +/-16 g; 10: +/-4 g; 11: +/-8 g).</para>
        /// </summary>
        unsigned fs_xl  : 2;
        /// <summary>Output data rate and power mode selection. Default value: 0000.</summary>
        unsigned odr_xl : 4;
    };

    uint8_t mask;
} LSM6DS3_ctrl1_xl_t;

/// <summary>Bit field description for register CTRL1_XL.</summary>
typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned res_0  : 1;
        /// <summary>
        /// <para>Gyroscope full-scale at 125 dps. Default value: false</para>
        /// <para>(false: disabled; true: enabled)</para>
        /// </summary>
        bool     fs_125 : 1;
        /// <summary>
        /// <para>Gyroscope full-scale selection. Default value: 00</para>
        /// <para>(00: 250 dps; 01: 500 dps; 10: 1000 dps; 11: 2000 dps)</para>
        ///</summary>
        unsigned fs_g   : 2;
        /// <summary>Gyroscope output data rate selection. Default value: 0000</summary>
        unsigned odr_g  : 4;
    };

    uint8_t mask;
} LSM6DS3_ctrl2_g_t;

/// <summary>This is  from the WHO_AM_I register. Its value is fixed at 69h.</summary>
static const uint8_t LSM6DS3_WHO_AM_I = 0x69;

/// <summary>
/// <para>This is a subordinate device Address. Its value is fixed at 6Ah.</para>
/// <para>SDO is tied to ground so the least significant bit of the address is zero.</para>
/// </summary>
static const uint32_t LSM6DS3_ADDRESS = 0x6A;

/// <summary>
/// <para>The application must call this function to implement a software reset.</para>
/// <para>This is a necessary function which will typically be used to reset the LSM6DS3 device.</para>
/// </summary>
/// <param name="driver">Selects the I2C driver to perform the transfer on.</param>
/// <returns>Returns true on success and false on failure.</returns>
bool LSM6DS3_Reset(I2CMaster *driver);

/// <summary>
/// <para>The application must call this function to validate the device id.</para>
/// <para>This is a testing function which will typically be used in the beginning of the communication of the master with the subordinate device.</para>
/// </summary>
/// <param name="driver">Selects the I2C driver to perform the transfer on.</param>
/// <returns>Returns true on success and false on failure.</returns>
bool LSM6DS3_CheckWhoAmI(I2CMaster *driver);

/// <summary>
/// <para>The application must call this function to configure the linear acceleration sensor control register.</para>
/// <para>This is a function which will typically be used to configure the accelerometer of the subordinate device.</para>
/// </summary>
/// <param name="driver">Selects the I2C driver to perform the transfer on.</param>
/// <param name="odr">Selects the output data rate and power mode.</param>
/// <param name="fs">Selects the full-scale of the accelerometer.</param>
/// <param name="bw">Selects the bandwidth of the anti-aliasing filter.</param>
/// <returns>Returns true on success and false on failure.</returns>
bool LSM6DS3_ConfigXL(I2CMaster *driver, unsigned odr, unsigned fs, unsigned bw);

/// <summary>
/// <para>The application must call this function to configure the gyroscope sensor control register.</para>
/// <para>This is a function which will typically be used to configure the gyroscope of the subordinate device.</para>
/// </summary>
/// <param name="driver">Selects the I2C driver to perform the transfer on.</param>
/// <param name="odr">Selects the output data rate and power mode.</param>
/// <param name="fs">Selects the full-scale of the gyroscope.</param>
/// <returns>Returns true on success and false on failure.</returns>
bool LSM6DS3_ConfigG(I2CMaster *driver, unsigned odr, unsigned fs);

/// <summary>
/// <para>The application must call this function to check if new data are available in the temperature, gyroscore and accelerometer sensors.</para>
/// </summary>
/// <param name="driver">Selects the I2C driver to perform the transfer on.</param>
/// <param name="tda">Reads temperature sensor for new data. false: no new data is available; true: a set of new data is available.</param>
/// <param name="gda">Reads gyroscope for new data. false: no new data is available; true: a set of new data is available.</param>
/// <param name="xlda">Reads accelerometer for new data. false: no new data is available; true: a set of new data is available.</param>
/// <returns>Returns true on success and false on failure.</returns>
bool LSM6DS3_Status(I2CMaster *driver, bool *tda, bool *gda, bool *xlda);

/// <summary>
/// <para>The application must call this function to read the temperature sensor.</para>
/// </summary>
/// <param name="driver">Selects the I2C driver to perform the transfer on.</param>
/// <param name="temp">Reads temperature sensor data.</param>
/// <returns>Returns true on success and false on failure.</returns>
bool LSM6DS3_ReadTemp(I2CMaster *driver, int16_t *temp);

/// <summary>
/// <para>The application must call this function to read the temperature sensor.</para>
/// <para>This function is a wrapper around <see cref="LSM6DS3_ReadTemp"/> which provides
/// human readable output.</para>
/// </summary>
/// <param name="driver">Selects the I2C driver to perform the transfer on.</param>
/// <param name="temp">Reads temperature sensor data in thousandths of a degree Celcius.</param>
/// <returns>Returns true on success and false on failure.</returns>
bool LSM6DS3_ReadTempHuman(I2CMaster *driver, int16_t *temp);

/// <summary>
/// <para>The application must call this function to read the gyroscope.</para>
/// </summary>
/// <param name="driver">Selects the I2C driver to perform the transfer on.</param>
/// <param name="x">Reads angular rate for X axis.</param>
/// <param name="y">Reads angular rate for Y axis.</param>
/// <param name="z">Reads angular rate for Z axis.</param>
/// <returns>Returns true on success and false on failure.</returns>
bool LSM6DS3_ReadG(I2CMaster *driver, int16_t *x, int16_t *y, int16_t *z);

/// <summary>
/// <para>The application must call this function to read the gyroscope.</para>
/// <para>This function is a wrapper around <see cref="LSM6DS3_ReadG"/> which provides
/// human readable output.</para>
/// </summary>
/// <param name="driver">Selects the I2C driver to perform the transfer on.</param>
/// <param name="x">Reads angular rate for X axis in mdps.</param>
/// <param name="y">Reads angular rate for Y axis in mdps.</param>
/// <param name="z">Reads angular rate for Z axis in mdps.</param>
/// <returns>Returns true on success and false on failure.</returns>
bool LSM6DS3_ReadGHuman(I2CMaster *driver, int16_t *x, int16_t *y, int16_t *z);

/// <summary>
/// <para>The application must call this function to read the accelerometer.</para>
/// </summary>
/// <param name="driver">Selects the I2C driver to perform the transfer on.</param>
/// <param name="x">Reads linear acceleration for x axis.</param>
/// <param name="y">Reads linear acceleration for Y axis.</param>
/// <param name="z">Reads linear acceleration for Z axis.</param>
/// <returns>Returns true on success and false on failure.</returns>
bool LSM6DS3_ReadXL(I2CMaster *driver, int16_t *x, int16_t *y, int16_t *z);

/// <summary>
/// <para>The application must call this function to read the accelerometer.</para>
/// <para>This function is a wrapper around <see cref="LSM6DS3_ReadXL"/> which provides
/// human readable output.</para>
/// </summary>
/// <param name="driver">Selects the I2C driver to perform the transfer on.</param>
/// <param name="x">Reads linear acceleration for x axis in mm per second squared.</param>
/// <param name="y">Reads linear acceleration for Y axis in mm per second squared.</param>
/// <param name="z">Reads linear acceleration for Z axis in mm per second squared.</param>
/// <returns>Returns true on success and false on failure.</returns>
bool LSM6DS3_ReadXLHuman(I2CMaster *driver, int16_t *x, int16_t *y, int16_t *z);

#endif // #ifndef LSM6DS3_H_
