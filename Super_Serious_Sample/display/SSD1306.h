/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#ifndef SSD1306_H_
#define SSD1306_H_

#include <stdbool.h>
#include <stdint.h>
#include "../lib/I2CMaster.h"

#define SSD1306_WIDTH  128
#define SSD1306_HEIGHT  64

#define SSD1306_ADDRESS 0x3C

/// <summary>Enum for command bytes of the SSD1306 controller.</summary>
typedef enum {
    SSD1306_CMD_CHARGEPUMP         = 0x8D,
    SSD1306_CMD_COMSCANDIR         = 0xC8,
    SSD1306_CMD_COMSCANINC         = 0xC0,
    SSD1306_CMD_DISPLAYALLON       = 0xA4,
    SSD1306_CMD_DISPLAYONOFF       = 0xAE,
    SSD1306_CMD_EXTERNALVCC        = 0x10,
    SSD1306_CMD_INVERTDISPLAY      = 0xA6,
    SSD1306_CMD_MEMORYMODE         = 0x20,
    SSD1306_CMD_NORMALDISPLAY      = 0xA6,
    SSD1306_CMD_PAGEADDR           = 0x22,
    SSD1306_CMD_SEGREMAP           = 0xA0,
    SSD1306_CMD_SETCOMPINS         = 0xDA,
    SSD1306_CMD_SETCONTRAST        = 0x81,
    SSD1306_CMD_SETCOLUMNADDR      = 0x21,
    SSD1306_CMD_SETDISPLAYCLOCKDIV = 0xD5,
    SSD1306_CMD_SETDISPLAYOFFSET   = 0xD3,
    SSD1306_CMD_SETHIGHCOLUMN      = 0x10,
    SSD1306_CMD_SETLOWCOLUMN       = 0x00,
    SSD1306_CMD_SETMULTIPLEX       = 0xA8,
    SSD1306_CMD_SETPRECHARGE       = 0xD9,
    SSD1306_CMD_SETSEGMENTREMAP    = 0xA1,
    SSD1306_CMD_SETSTARTLINE       = 0x40,
    SSD1306_CMD_SETVCOMDETECT      = 0xDB,
    SSD1306_CMD_SWITCHCAPVCC       = 0x20,
    SSD1306_CMD_SETSCROLL          = 0x2E,
} SSD1306_Command;

/// <summary>Enum for control bytes of the SSD1306 controller, these are used to set the defaults
/// during initialisation.</summary>
typedef enum {
    SSD1306_CTRL_DISPLAYCLOCKDIV = 0xF0,
    SSD1306_CTRL_MULTIPLEX = 0x3F,
    SSD1306_CTRL_DISPLAYOFFSET = 0x00,
    SSD1306_CTRL_STARTLINE = 0x00,
    SSD1306_CTRL_CHARGEPUMP = 0x14,
    SSD1306_CTRL_MEMORYMODE = 0x01,
    SSD1306_CTRL_SEGREMAP = false,
    SSD1306_CTRL_COMSCANDIR = false,
    SSD1306_CTRL_COMPIMS = 0x12,
    SSD1306_CTRL_CONTRAST = 0xCF,
    SSD1306_CTRL_PRECHARGE = 0xF1,
    SSD1306_CTRL_VCOMDETECT = 0x40,
    SSD1306_CTRL_DISPLAYINVERSE = false,
    SSD1306_CTRL_ACTIVATESCROLL = false,
} SSD1306_Ctrl;

/// <summary>
/// <para>A function to write the entire screen's display buffer to RAM in one block transaction.
/// </para>
/// </summary>
/// <param name="driver">The address of the I2C controller the device is connected to.</param>
/// <param name="data">A pointer to the data to be sent to the display RAM.</param>
/// <param name="size">The size of the data being sent, used for error checking. Should always
/// be 128x64.</param>
/// <returns>True or false to indicate success of the write.</returns>
bool SSD1306_WriteFullBuffer(I2CMaster *driver, const void *data, uintptr_t size);

/// <summary>
/// <para>A function to turn the display on/off.</para>
/// </summary>
/// <param name="driver">The address of the I2C controller the device is connected to.</param>
/// <param name="displayOnTrue">Pass true for display on, false for display off.</param>
/// <returns>True or false to indicate the success of the I2C transaction.</returns>
bool SSD1306_SetDisplayOnOff(I2CMaster *driver, bool displayOnTrue);

/// <summary>
/// <para>A function to change the display contrast.</para>
/// </summary>
/// <param name="driver">The address of the I2C controller the device is connected to.</param>
/// <param name="value">A value between 0 and 255 to set the display contrast, higher values
/// give higher contrast.</param>
/// <returns>True or false to indicate the success of the I2C transaction.</returns>
bool SSD1306_SetContrast(I2CMaster *driver, uint8_t value);

/// <summary>
/// <para>A function to switch between the display showing what is in display RAM and turning
/// all pixels on.</para>
/// </summary>
/// <param name="driver">The address of the I2C controller the device is connected to.</param>
/// <param name="displayAllOnTrue">Pass true for all pixels on, false for display RAM contents.</param>
/// <returns>True or false to indicate the success of the I2C transaction.</returns>
bool SSD1306_SetDisplayAllOn(I2CMaster *driver, bool displayAllOnTrue);

/// <summary>
/// <para>A function to set the display between a 1 in RAM displaying as white/black.</para>
/// </summary>
/// <param name="driver">The address of the I2C controller the device is connected to.</param>
/// <param name="inverseTrue">False sets the display to show white for each pixel set to 1,
/// true will set the display to display black for each pixel set to 1.</param>
/// <returns>True or false to indicate the success of the I2C transaction.</returns>
bool SSD1306_SetDisplayInverse(I2CMaster *driver, bool inverseTrue);

/// <summary>
/// <para>A function to intialise the display. All parameters for this should be defined in this
/// header file.</para>
/// </summary>
/// <param name="driver">The address of the I2C controller the device is connected to.</param>
/// <returns>True or false to indicate the success of the setup.</returns>
bool Ssd1306_Init(I2CMaster *driver);

#endif // #ifndef SSD1306_H_
