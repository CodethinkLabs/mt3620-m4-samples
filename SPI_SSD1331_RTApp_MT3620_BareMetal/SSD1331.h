/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#ifndef SSD1331_H_
#define SSD1331_H_

typedef struct SSD1331 SSD1331;

#include "lib/SPIMaster.h"

#define SSD1331_WIDTH  96
#define SSD1331_HEIGHT 64

typedef enum {
    SSD1331_DISPLAY_MODE_NORMAL = 0,
    SSD1331_DISPLAY_MODE_ON     = 1,
    SSD1331_DISPLAY_MODE_OFF    = 2,
    SSD1331_DISPLAY_MODE_INVERT = 3,
} SSD1331_DisplayMode;

typedef union __attribute__((__packed__)) {
    struct __attribute__((__packed__)) {
        unsigned addr_inc_mode           : 1;
        unsigned col_addr_map            : 1;
        unsigned rgb_map                 : 1;
        unsigned com_left_right_remap    : 1;
        unsigned com_scan_dir_remap      : 1;
        unsigned odd_even_split_com_pins : 1;
        unsigned color_mode              : 2;
    };
    uint8_t mask;
} SSD1331_RemapAndDataFormat;

bool SSD1331_SetColAddress(SSD1331 *handle, uint8_t start, uint8_t end);
bool SSD1331_SetRowAddress(SSD1331 *handle, uint8_t start, uint8_t end);
bool SSD1331_SetCommandLock(SSD1331 *handle, bool value);
bool SSD1331_SetDisplayOn(SSD1331 *handle, bool full);
bool SSD1331_SetDisplayOff(SSD1331 *handle);
bool SSD1331_SetRemapAndDataFormat(SSD1331 *handle, SSD1331_RemapAndDataFormat format);
bool SSD1331_SetDisplayStartline(SSD1331 *handle, unsigned value);
bool SSD1331_SetDisplayOffset(SSD1331 *handle, uint8_t value);
bool SSD1331_SetDisplayMode(SSD1331 *handle, SSD1331_DisplayMode mode);
bool SSD1331_SetMuptiplexRatio(SSD1331 *handle, uint8_t value);
bool SSD1331_SetMasterConfiguration(SSD1331 *handle, uint8_t value);
bool SSD1331_DisablePowerSavingMode(SSD1331 *handle, uint8_t value);
bool SSD1331_SetPhaseAndPeriod(SSD1331 *handle, uint8_t value);
bool SSD1331_SetDisplayClockDivideRatio(SSD1331 *handle, uint8_t value);
bool SSD1331_SetSecondPrechargeSpeedA(SSD1331 *handle, uint8_t value);
bool SSD1331_SetSecondPrechargeSpeedB(SSD1331 *handle, uint8_t value);
bool SSD1331_SetSecondPrechargeSpeedC(SSD1331 *handle, uint8_t value);
bool SSD1331_SetPrechargeVoltage(SSD1331 *handle, uint8_t value);
bool SSD1331_SetVcomh(SSD1331 *handle, uint8_t value);
bool SSD1331_SetMasterCurrentControl(SSD1331 *handle, uint8_t value);
bool SSD1331_SetContrastColorA(SSD1331 *handle, uint8_t value);
bool SSD1331_SetContrastColorB(SSD1331 *handle, uint8_t value);
bool SSD1331_SetContrastColorC(SSD1331 *handle, uint8_t value);
bool SSD1331_DeactivateScrolling(SSD1331 *handle);
bool SSD1331_ClearWindow(SSD1331 *handle);

bool SSD1331_Upload(SSD1331 *handle, const void *data, uintptr_t size);

bool SSD1331_DrawLine(SSD1331 *handle, uint8_t columnStart, uint8_t rowStart, uint8_t columnEnd, uint8_t rowEnd, uint8_t colorC, uint8_t colorB, uint8_t colorA);

SSD1331 *SSD1331_Open(SPIMaster *interface, int pinDataCmd, int pinReset, int pinVccEn, int pinPModEn);
void     SSD1331_Close(SSD1331 *handle);

#endif // #ifndef SSD1331_H_
