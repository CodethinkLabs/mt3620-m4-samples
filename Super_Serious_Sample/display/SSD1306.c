/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "SSD1306.h"

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned reserved : 6;
        bool     isData   : 1;
        bool     cont     : 1;
    };
    uint8_t mask;
} Ssd1306_I2CHeader;

typedef struct __attribute__((__packed__)) {
    Ssd1306_I2CHeader header;
    uint8_t           data;
} Ssd1306_Packet;

#define SSD1306_MAX_DATA_WRITE 1024
static bool Ssd1306_Write(I2CMaster *driver, bool isData, const void *data, uintptr_t size)
{
    static __attribute__((section(".sysram"))) Ssd1306_Packet cmd[SSD1306_MAX_DATA_WRITE];
    if (size > SSD1306_MAX_DATA_WRITE) {
        return false;
    }

    uintptr_t i;
    for (i = 0; i < size; i++) {
        cmd[i].header = (Ssd1306_I2CHeader){ .isData = isData, .cont = (i < (size - 1)) };
        cmd[i].data   = ((uint8_t *)data)[i];
    }

    return (I2CMaster_WriteSync(driver, SSD1306_ADDRESS, cmd, (size * sizeof(cmd[0]))) == ERROR_NONE);
}

//Static functions for hardware configuration
static bool SSD1306_SetDisplayClockDiv(I2CMaster *driver, uint8_t value)
{
    uint8_t packet[] = { SSD1306_CMD_SETDISPLAYCLOCKDIV, value };
    return Ssd1306_Write(driver, false, packet, sizeof(packet));
}

static bool SSD1306_SetMultiplex(I2CMaster *driver, uint8_t value)
{
    uint8_t packet[] = { SSD1306_CMD_SETMULTIPLEX, value };
    return Ssd1306_Write(driver, false, packet, sizeof(packet));
}

static bool SSD1306_SetDisplayOffset(I2CMaster *driver, uint8_t value)
{
    uint8_t packet[] = { SSD1306_CMD_SETDISPLAYOFFSET, value };
    return Ssd1306_Write(driver, false, packet, sizeof(packet));
}


static bool SSD1306_SetChargePump(I2CMaster *driver, uint8_t value)
{
    uint8_t packet[] = { SSD1306_CMD_CHARGEPUMP, value };
    return Ssd1306_Write(driver, false, packet, sizeof(packet));
}

static bool SSD1306_SetStartLine(I2CMaster *driver, unsigned offset)
{
    if (offset >= SSD1306_HEIGHT) {
        return false;
    }

    uint8_t value = SSD1306_CMD_SETSTARTLINE + offset;
    return Ssd1306_Write(driver, false, &value, sizeof(value));
}

static bool SSD1306_SetMemoryMode(I2CMaster *driver, uint8_t value)
{
    uint8_t packet[] = { SSD1306_CMD_MEMORYMODE, value };
    return Ssd1306_Write(driver, false, packet, sizeof(packet));
}

static bool SSD1306_SetSegRemap(I2CMaster *driver, bool remapTrue)
{
    uint8_t value = SSD1306_CMD_SETSEGMENTREMAP | remapTrue;
    return Ssd1306_Write(driver, false, &value, sizeof(value));
}

static bool SSD1306_SetComScanDir(I2CMaster *driver, bool scanDirTrue)
{
    uint8_t value = SSD1306_CMD_COMSCANDIR | (scanDirTrue ? 8 : 0);
    return Ssd1306_Write(driver, false, &value, sizeof(value));
}

static bool SSD1306_SetComPins(I2CMaster *driver, uint8_t value)
{
    uint8_t packet[] = { SSD1306_CMD_SETCOMPINS, value };
    return Ssd1306_Write(driver, false, packet, sizeof(packet));
}

static bool SSD1306_SetPreCharge(I2CMaster *driver, uint8_t value)
{
    uint8_t packet[] = { SSD1306_CMD_SETPRECHARGE, value };
    return Ssd1306_Write(driver, false, packet, sizeof(packet));
}

static bool SSD1306_SetVComDetect(I2CMaster *driver, uint8_t value)
{
    uint8_t packet[] = { SSD1306_CMD_SETVCOMDETECT, value };
    return Ssd1306_Write(driver, false, packet, sizeof(packet));
}
//End of hardware configuration functions

//Sets the start and end column address for the data that is being sent
static bool SSD1306_SetColumnAddress(I2CMaster *driver, uint8_t columnStart, uint8_t columnEnd)
{
    if ((columnStart >= SSD1306_WIDTH) || (columnEnd >= SSD1306_WIDTH)) {
        return false;
    }
    uint8_t packet[] = { SSD1306_CMD_SETCOLUMNADDR, columnStart, columnEnd };
    return Ssd1306_Write(driver, false, packet, sizeof(packet));
}

bool SSD1306_WriteFullBuffer(I2CMaster *driver, const void *data, uintptr_t size)
{
    if (!SSD1306_SetColumnAddress(driver, 0, (SSD1306_WIDTH - 1))) {
        return false;
    }
    return Ssd1306_Write(driver, true, data, size);
}

bool SSD1306_SetDisplayOnOff(I2CMaster *driver, bool displayOnTrue)
{
    uint8_t value = SSD1306_CMD_DISPLAYONOFF | displayOnTrue;
    return Ssd1306_Write(driver, false, &value, sizeof(value));
}

bool SSD1306_SetContrast(I2CMaster *driver, uint8_t value)
{
    uint8_t packet[] = { SSD1306_CMD_SETCONTRAST, value };
    return Ssd1306_Write(driver, false, packet, sizeof(packet));
}

bool SSD1306_SetDisplayAllOn(I2CMaster *driver, bool displayAllOnTrue)
{
    int8_t value = SSD1306_CMD_DISPLAYALLON | displayAllOnTrue;
    return Ssd1306_Write(driver, false, &value, sizeof(value));
}

bool SSD1306_SetDisplayInverse(I2CMaster *driver, bool inverseTrue)
{
    uint8_t value = SSD1306_CMD_NORMALDISPLAY | inverseTrue;
    return Ssd1306_Write(driver, false, &value, sizeof(value));
}

bool SSD1306_ActivateScroll(I2CMaster *driver, bool activateScrollTrue)
{
    uint8_t value = SSD1306_CMD_SETSCROLL | activateScrollTrue;
    return Ssd1306_Write(driver, false, &value, sizeof(value));
}

bool Ssd1306_Init(I2CMaster *driver)
{
    bool success = true;

    success = success && SSD1306_SetDisplayOnOff(driver, false);
    success = success && SSD1306_SetDisplayClockDiv(driver, SSD1306_CTRL_DISPLAYCLOCKDIV);
    success = success && SSD1306_SetMultiplex(driver, SSD1306_CTRL_MULTIPLEX);
    success = success && SSD1306_SetDisplayOffset(driver, SSD1306_CTRL_DISPLAYOFFSET);
    success = success && SSD1306_SetStartLine(driver, SSD1306_CTRL_STARTLINE);
    success = success && SSD1306_SetChargePump(driver, SSD1306_CTRL_CHARGEPUMP);
    success = success && SSD1306_SetMemoryMode(driver, SSD1306_CTRL_MEMORYMODE);
    success = success && SSD1306_SetSegRemap(driver, SSD1306_CTRL_SEGREMAP);
    success = success && SSD1306_SetComScanDir(driver, SSD1306_CTRL_COMSCANDIR);
    success = success && SSD1306_SetComPins(driver, SSD1306_CTRL_COMPIMS);
    success = success && SSD1306_SetContrast(driver, SSD1306_CTRL_CONTRAST);
    success = success && SSD1306_SetPreCharge(driver, SSD1306_CTRL_PRECHARGE);
    success = success && SSD1306_SetVComDetect(driver, SSD1306_CTRL_VCOMDETECT);
    success = success && SSD1306_SetDisplayAllOn(driver, false);
    success = success && SSD1306_SetDisplayInverse(driver, SSD1306_CTRL_DISPLAYINVERSE);
    success = success && SSD1306_ActivateScroll(driver, SSD1306_CTRL_ACTIVATESCROLL);
    success = success && SSD1306_SetDisplayOnOff(driver, true);

    return success;
}

