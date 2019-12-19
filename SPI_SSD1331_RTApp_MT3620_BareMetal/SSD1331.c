/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#include "SSD1331.h"
#include "lib/GPIO.h"
#include "lib/GPT.h"
#include "lib/Platform.h"



struct SSD1331 {
    SPIMaster *interface;
    int        pinDataCmd, pinReset, pinVccEn, pinPModEn;
};


#define SSD1331_HANDLE_COUNT 4
static SSD1331 SSD1331_Handle[SSD1331_HANDLE_COUNT] = {0};

static GPT *timer = NULL;

typedef enum {
    SSD1331_CMD_SET_COLUMN_ADDRESS                                 = 0x15,
    SSD1331_CMD_DRAW_LINE                                          = 0x21,
    SSD1331_CMD_DRAW_RECTANGLE                                     = 0x22,
    SSD1331_CMD_COPY                                               = 0x23,
    SSD1331_CMD_DIM_WINDOW                                         = 0x24,
    SSD1331_CMD_CLEAR_WINDOW                                       = 0x25,
    SSD1331_CMD_FILL_ENABLE_DISABLE                                = 0x26,
    SSD1331_CMD_CONTINIOUS_HORIZONTAL_AND_VERTICAL_SCROLLING_SETUP = 0x27,
    SSD1331_CMD_DEACTIVATE_SCROLLING                               = 0x2E,
    SSD1331_CMD_ACTIVATE_SCROLLING                                 = 0X2F,
    SSD1331_CMD_SET_ROW_ADDRESS                                    = 0x75,
    SSD1331_CMD_SET_CONTRAST_FOR_COLOR_A                           = 0x81,
    SSD1331_CMD_SET_CONTRAST_FOR_COLOR_B                           = 0x82,
    SSD1331_CMD_SET_CONTRAST_FOR_COLOR_C                           = 0x83,
    SSD1331_CMD_MASTER_CURRENT_CONTROL                             = 0x87,
    SSD1331_CMD_SET_SECOND_PRECHARGE_SPEED_FOR_COLOR_A             = 0x8A,
    SSD1331_CMD_SET_SECOND_PRECHARGE_SPEED_FOR_COLOR_B             = 0x8B,
    SSD1331_CMD_SET_SECOND_PRECHARGE_SPEED_FOR_COLOR_C             = 0x8C,
    SSD1331_CMD_SET_REMAP_AND_DATA_FORMAT                          = 0xA0,
    SSD1331_CMD_SET_DISPLAY_START_LINE                             = 0xA1,
    SSD1331_CMD_SET_DISPLAY_OFFSET                                 = 0xA2,
    SSD1331_CMD_SET_DISPLAY_MODE                                   = 0xA4,
    SSD1331_CMD_SET_MULTIPLEX_RATIO                                = 0xA8,
    SSD1331_CMD_DIM_MODE_SETTING                                   = 0xAB,
    SSD1331_CMD_DISPLAY_ON_DIM                                     = 0xAC,
    SSD1331_CMD_SET_MASTER_CONFIGURATION                           = 0xAD,
    SSD1331_CMD_DISPLAY_OFF                                        = 0xAE,
    SSD1331_CMD_DISPLAY_ON                                         = 0xAF,
    SSD1331_CMD_POWER_SAVE_MODE                                    = 0xB0,
    SSD1331_CMD_PHASE_1_AND_2_PERIOD_ADJUSTMENT                    = 0xB1,
    SSD1331_CMD_SET_DISPLAY_CLOCK_DIVIDE_RATIO                     = 0xB3,
    SSD1331_CMD_SET_GRAYSCALE_TABLE                                = 0xB8,
    SSD1331_CMD_ENABLE_LINEAR_GRAY_SCALE_TABLE                     = 0xB9,
    SSD1331_CMD_SET_PRECHARGE_VOLTAGE                              = 0xBB,
    SSD1331_CMD_V_COMH                                             = 0xBE,
    SSD1331_CMD_NOP_A                                              = 0xBC,
    SSD1331_CMD_NOP_B                                              = 0xBD,
    SSD1331_CMD_NOP_C                                              = 0xE3,
    SSD1331_CMD_SET_COMMAND_LOCK                                   = 0xFD,
} SSD1331_Command;


static bool SSD1331_SendCommandArg(SSD1331 *handle, uint8_t addr, uint8_t value)
{
    if (!handle) {
        return false;
    }

    const uint8_t cmd[] = { addr, value };
    return (SPIMaster_WriteSync(handle->interface, cmd, sizeof(cmd)) == ERROR_NONE);
}

static bool SSD1331_SendCommand(SSD1331 *handle, const uint8_t addr)
{
    if (!handle) {
        return false;
    }

    const uint8_t cmd[] = { addr };
    return (SPIMaster_WriteSync(handle->interface, cmd, sizeof(cmd)) == ERROR_NONE);
}



bool SSD1331_SetColAddress(SSD1331 *handle, uint8_t start, uint8_t end)
{
    if (!handle) {
        return false;
    }

    if ((end < start) || (end > (SSD1331_WIDTH - 1))) {
        return false;
    }

    const uint8_t cmd[] = { SSD1331_CMD_SET_COLUMN_ADDRESS , start, end };
    return (SPIMaster_WriteSync(handle->interface, cmd, sizeof(cmd)) == ERROR_NONE);
}

bool SSD1331_SetRowAddress(SSD1331 *handle, uint8_t start, uint8_t end)
{
  if (!handle) {
    return false;
  }

  if ((end < start) || (end > (SSD1331_HEIGHT - 1))) {
    return false;
  }

  const uint8_t cmd[] = {SSD1331_CMD_SET_ROW_ADDRESS, start, end};
  return (SPIMaster_WriteSync(handle->interface, cmd, sizeof(cmd)) == ERROR_NONE);
}

bool SSD1331_SetCommandLock(SSD1331 *handle, bool lock)
{
    const uint8_t lv = 0x12 | (lock ? 4 : 0);
    return SSD1331_SendCommandArg(handle, SSD1331_CMD_SET_COMMAND_LOCK, lv);
}

bool SSD1331_SetDisplayOn(SSD1331 *handle, bool full)
{
    return SSD1331_SendCommand(handle,
        (full ? SSD1331_CMD_DISPLAY_ON : SSD1331_CMD_DISPLAY_ON_DIM));
}

bool SSD1331_SetDisplayOff(SSD1331 *handle)
{
    return SSD1331_SendCommand(handle, SSD1331_CMD_DISPLAY_OFF);
}

bool SSD1331_SetRemapAndDataFormat(SSD1331 *handle, SSD1331_RemapAndDataFormat format)
{
    return SSD1331_SendCommandArg(handle, SSD1331_CMD_SET_REMAP_AND_DATA_FORMAT, format.mask);
}

bool SSD1331_SetDisplayStartline(SSD1331 *handle, unsigned value)
{
    if (value >= SSD1331_HEIGHT) {
        return false;
    }
    return SSD1331_SendCommandArg(handle, SSD1331_CMD_SET_DISPLAY_START_LINE, value);
}

bool SSD1331_SetDisplayOffset(SSD1331 *handle, uint8_t value)
{
    return SSD1331_SendCommandArg(handle, SSD1331_CMD_SET_DISPLAY_OFFSET, value);
}

bool SSD1331_SetDisplayMode(SSD1331 *handle, SSD1331_DisplayMode mode)
{
    if (mode >= 4) {
        return false;
    }
    return SSD1331_SendCommand(handle, (SSD1331_CMD_SET_DISPLAY_MODE + mode));
}

bool SSD1331_SetMuptiplexRatio(SSD1331 *handle, uint8_t value)
{
    return SSD1331_SendCommandArg(handle, SSD1331_CMD_SET_MULTIPLEX_RATIO, value);
}

bool SSD1331_SetMasterConfiguration(SSD1331 *handle, uint8_t value)
{
    return SSD1331_SendCommandArg(handle, SSD1331_CMD_SET_MASTER_CONFIGURATION, value);
}

bool SSD1331_DisablePowerSavingMode(SSD1331 *handle, uint8_t value)
{
    return SSD1331_SendCommandArg(handle, SSD1331_CMD_POWER_SAVE_MODE, value);
}

bool SSD1331_SetPhaseAndPeriod(SSD1331 *handle, uint8_t value)
{
    return SSD1331_SendCommandArg(handle, SSD1331_CMD_PHASE_1_AND_2_PERIOD_ADJUSTMENT, value);
}

bool SSD1331_SetDisplayClockDivideRatio(SSD1331 *handle, uint8_t value)
{
    return SSD1331_SendCommandArg(handle, SSD1331_CMD_SET_DISPLAY_CLOCK_DIVIDE_RATIO, value);
}

bool SSD1331_SetSecondPrechargeSpeedA(SSD1331 *handle, uint8_t value)
{
    return SSD1331_SendCommandArg(handle, SSD1331_CMD_SET_SECOND_PRECHARGE_SPEED_FOR_COLOR_A, value);
}

bool SSD1331_SetSecondPrechargeSpeedB(SSD1331 *handle, uint8_t value)
{
    return SSD1331_SendCommandArg(handle, SSD1331_CMD_SET_SECOND_PRECHARGE_SPEED_FOR_COLOR_B, value);
}

bool SSD1331_SetSecondPrechargeSpeedC(SSD1331 *handle, uint8_t value)
{
    return SSD1331_SendCommandArg(handle, SSD1331_CMD_SET_SECOND_PRECHARGE_SPEED_FOR_COLOR_C, value);
}

bool SSD1331_SetPrechargeVoltage(SSD1331 *handle, uint8_t value)
{
    return SSD1331_SendCommandArg(handle, SSD1331_CMD_SET_PRECHARGE_VOLTAGE, value);
}

bool SSD1331_SetVcomh(SSD1331 *handle, uint8_t value)
{
    return SSD1331_SendCommandArg(handle, SSD1331_CMD_V_COMH, value);
}

bool SSD1331_SetMasterCurrentControl(SSD1331 *handle, uint8_t value)
{
    return SSD1331_SendCommandArg(handle, SSD1331_CMD_MASTER_CURRENT_CONTROL, value);
}

bool SSD1331_SetContrastColorA(SSD1331 *handle, uint8_t value)
{
    return SSD1331_SendCommandArg(handle, SSD1331_CMD_SET_CONTRAST_FOR_COLOR_A, value);
}

bool SSD1331_SetContrastColorB(SSD1331 *handle, uint8_t value)
{
    return SSD1331_SendCommandArg(handle, SSD1331_CMD_SET_CONTRAST_FOR_COLOR_B, value);
}

bool SSD1331_SetContrastColorC(SSD1331 *handle, uint8_t value)
{
    return SSD1331_SendCommandArg(handle, SSD1331_CMD_SET_CONTRAST_FOR_COLOR_C, value);
}

bool SSD1331_DeactivateScrolling(SSD1331 *handle)
{
    return SSD1331_SendCommand(handle, SSD1331_CMD_DEACTIVATE_SCROLLING);
}

bool SSD1331_ClearWindow(SSD1331 *handle)
{
    if (!handle) {
        return false;
    }

    const uint8_t cmd[] = { SSD1331_CMD_CLEAR_WINDOW, 0, 0, (SSD1331_WIDTH - 1), (SSD1331_HEIGHT - 1) };
    return (SPIMaster_WriteSync(handle->interface, cmd, sizeof(cmd)) == ERROR_NONE);
}


bool SSD1331_DrawLine(SSD1331 *handle, uint8_t columnStart, uint8_t rowStart, uint8_t columnEnd, uint8_t rowEnd, uint8_t colorC, uint8_t colorB, uint8_t colorA)
{
    if (!handle) {
        return false;
    }

    uint8_t data[] = { SSD1331_CMD_DRAW_LINE, columnStart, rowStart, columnEnd , rowEnd, colorC, colorB, colorA };
    return (SPIMaster_WriteSync(handle->interface, data, sizeof(data)) == ERROR_NONE);
}


bool SSD1331_Upload(SSD1331 *handle, const void *data, uintptr_t size)
{
    GPIO_Write(handle->pinDataCmd, true);

    static const unsigned spiMaxPacket = 20;

    bool success = true;
    const void *ptr = data;
    uintptr_t remain = size;
    while (remain > 0) {
        uintptr_t packet = (remain > spiMaxPacket ? spiMaxPacket : remain);

        if (SPIMaster_WriteSync(handle->interface, ptr, packet) != ERROR_NONE) {
            success = false;
            break;
        }

        ptr = (const void*)((uintptr_t)ptr + packet);
        remain -= packet;
    }

    GPIO_Write(handle->pinDataCmd, false);
    // wait at least 10 msec
    if (GPT_WaitTimer_Blocking(timer, 1000, GPT_UNITS_MICROSEC) != ERROR_NONE) {
        return NULL;
    }
    return success;
}


SSD1331 *SSD1331_Open(SPIMaster *interface, int pinDataCmd, int pinReset, int pinVccEn, int pinPModEn)
{
    if (!interface) {
        return NULL;
    }

    SSD1331 *handle = NULL;
    unsigned i;
    for (i = 0; i < SSD1331_HANDLE_COUNT; i++) {
        if (!SSD1331_Handle[i].interface) {
            handle = &SSD1331_Handle[i];
            break;
        }
    }
    if (!handle) {
        return NULL;
    }

    timer = GPT_Open(MT3620_UNIT_GPT3, 1000000, GPT_MODE_NONE);

    // Initialization of the PMOD is described in its reference manual:
    // https://reference.digilentinc.com/reference/pmod/pmodoledrgb/reference-manual

    // Bring Data/Command control pin at logic low.
    GPIO_Write(pinDataCmd, false);

    // Bring Reset pin at logic high.
    GPIO_Write(pinReset, true);

    // Bring Vcc Enable at logic low.
    GPIO_Write(pinVccEn, false);

    // Bring Pmod Enable at logic high and delay 20 ms to allow the 3.3V rail to become stable.
    GPIO_Write(pinPModEn, true);
    if (GPT_WaitTimer_Blocking(timer, 20000, GPT_UNITS_MICROSEC) != ERROR_NONE) {
        return NULL;
    }

    // Bring RES to logic low, wait for at least 3us and then bring it back to logic high to reset the display controller.
    GPIO_Write(pinReset, false);

    // wait at least 10 msec
    if (GPT_WaitTimer_Blocking(timer, 1000, GPT_UNITS_MICROSEC) != ERROR_NONE) {
        return NULL;
    }

    // Bring RES to logic high
    GPIO_Write(pinReset, true);

    // wait at least 10 msec
    if (GPT_WaitTimer_Blocking(timer, 1000, GPT_UNITS_MICROSEC) != ERROR_NONE) {
        return NULL;
    }

    if (SPIMaster_Configure(interface, 1, 1, 10000000) != ERROR_NONE) {
        return NULL;
    }

    handle->interface  = interface;
    handle->pinDataCmd = pinDataCmd;
    handle->pinReset   = pinReset;
    handle->pinVccEn   = pinVccEn;
    handle->pinPModEn  = pinPModEn;

    bool success = true;

    // Enable the driver IC to accept commands by sending the unlock command over SPI.
    success = success && SSD1331_SetCommandLock(handle, false);

    // Send the display off command.
    success = success && SSD1331_SetDisplayOff(handle);

    // Set the Remap and Display formats.
    SSD1331_RemapAndDataFormat format = {
        .addr_inc_mode           = 0,
        .col_addr_map            = 1,
        .rgb_map                 = 0,
        .com_left_right_remap    = 0,
        .com_scan_dir_remap      = 1,
        .odd_even_split_com_pins = 1,
        .color_mode              = 1,
    };
    success = success && SSD1331_SetRemapAndDataFormat(handle, format);

    // Set the Display start Line to the top line.
    success = success && SSD1331_SetDisplayStartline(handle, 0);

    // Set the Display Offset to no vertical offset.
    success = success && SSD1331_SetDisplayOffset(handle, 0);

    // Make it a normal display with no color inversion or forcing the pixels on/off.
    success = success && SSD1331_SetDisplayMode(handle, SSD1331_DISPLAY_MODE_NORMAL);

    // Set the Multiplex Ratio to enable all of the common pins calculated by 1 + register value.
    success = success && SSD1331_SetMuptiplexRatio(handle, 0x3F);

    // Set Master Configuration to use a required external vcc supply.
    success = success && SSD1331_SetMasterConfiguration(handle, 0x8E);

    // Disable Power Saving Mode.
    success = success && SSD1331_DisablePowerSavingMode(handle, 0x0B);

    // Set the Phase Length of the charge and discharge rates of an OLED pixel in units of the display clock.
    success = success && SSD1331_SetPhaseAndPeriod(handle, 0x31);

    // Set the Display Clock Divide Ratio and Oscillator Frequency, setting the clock divider ratio to 1 and
    //the internal oscillator frequency to ~890 kHz. Kilohertz(thousand times per second).
    success = success && SSD1331_SetDisplayClockDivideRatio(handle, 0xF0);

    // Set the Second Pre-Charge Speed of Color A to drive the color (red by default) to a target driving voltage.
    success = success && SSD1331_SetSecondPrechargeSpeedA(handle, 0x64);

    // Set the Second Pre-Charge Speed of Color B to drive the color (green by default) to a target driving voltage.
    success = success && SSD1331_SetSecondPrechargeSpeedB(handle, 0x78);

    // Set the Second Pre-Charge Speed of Color C to drive the color (blue by default) to a target driving voltage.
    success = success && SSD1331_SetSecondPrechargeSpeedC(handle, 0x64);

    // Set the Pre-Charge Voltage to approximately 45% of Vcc to drive each color to a target driving voltage.
    success = success && SSD1331_SetPrechargeVoltage(handle, 0x3A);

    // Set the VCOMH Deselect Level, which is the minimum voltage level to be registered as logic high, to 83% of Vcc.
    success = success && SSD1331_SetVcomh(handle, 0x3E);

    // Set Master Current Attenuation Factor to set a reference current for the segment drivers.
    success = success && SSD1331_SetMasterCurrentControl(handle, 0x06);

    // Set the Contrast for Color A (default red), effectively setting the brightness level.
    success = success && SSD1331_SetContrastColorA(handle, 0x91);

    // Set the Contrast for Color B (default green), effectively setting the brightness level.
    success = success && SSD1331_SetContrastColorB(handle, 0x50);

    // Set the Contrast for Color C (default blue), effectively setting the brightness level.
    success = success && SSD1331_SetContrastColorC(handle, 0x7D);

    // Disable Scrolling.
    success = success && SSD1331_DeactivateScrolling(handle);

    // Clear the screen by sending the clear command and the dimensions of the window to clear
    //(top column, top row, bottom column, bottom row).
    success = success && SSD1331_ClearWindow(handle);

    // Bring VCCEN logic high and wait 25 ms.
    GPIO_Write(pinVccEn, true);
    if (GPT_WaitTimer_Blocking(timer, 25000, GPT_UNITS_MICROSEC) != ERROR_NONE) {
        return NULL;
    }

    // Turn the display on. Wait at least 100 ms before further operation.
    success = success && SSD1331_SetDisplayOn(handle, true);
    if (GPT_WaitTimer_Blocking(timer, 100000, GPT_UNITS_MICROSEC) != ERROR_NONE) {
        return NULL;
    }

    if (!success) {
        handle->interface = NULL;
        return NULL;
    }

    return handle;
}

void SSD1331_Close(SSD1331 *handle)
{
    if (!handle || !handle->interface) {
        return;
    }

    SSD1331_SetDisplayOff(handle);
    SSD1331_SetCommandLock(handle, true);
    GPIO_Write(handle->pinVccEn , false);
    GPIO_Write(handle->pinPModEn, false);

    handle->interface = NULL;
}
