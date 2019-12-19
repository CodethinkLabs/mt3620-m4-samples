/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#include "max98090.h"
#include <stddef.h>

typedef enum
{
    // Reset/Status/Interrupt
    MAX98090_REG_SOFTWARE_RESET  = 0x00,
    MAX98090_REG_DEVICE_STATUS   = 0x01,
    MAX98090_REG_JACK_STATUS     = 0x02,
    MAX98090_REG_INTERRUPT_MASKS = 0x03,

    // Quick Setup
    MAX98090_REG_QS_SYSTEM_CLOCK      = 0x04,
    MAX98090_REG_QS_SAMPLE_RATE       = 0x05,
    MAX98090_REG_QS_DAI_INTERFACE     = 0x06,
    MAX98090_REG_QS_DAC_PATH          = 0x07,
    MAX98090_REG_QS_MIC_DIRECT_TO_ADC = 0x08,
    MAX98090_REG_QS_LINE_TO_ADC       = 0x09,
    MAX98090_REG_QS_ANALOG_MIC_LOOP   = 0x0A,
    MAX98090_REG_QS_ANALOG_LINE_LOOP  = 0x0B,

    // Analog Input Configuration
    MAX98090_REG_LINE_INPUT_CONFIG = 0x0D,
    MAX98090_REG_LINE_INPUT_LEVEL  = 0x0E,
    MAX98090_REG_INPUT_MODE        = 0x0F,
    MAX98090_REG_MIC1_INPUT_LEVEL  = 0x10,
    MAX98090_REG_MIC2_INPUT_LEVEL  = 0x11,

    // Microphone Configuration
    MAX98090_REG_MIC_BIAS_VOLTAGE   = 0x12,
    MAX98090_REG_DIGITAL_MIC_ENABLE = 0x13,
    MAX98090_REG_DIGITAL_MIC_CONFIG = 0x14,

    // ADC Path and Configuration
    MAX98090_REG_LEFT_ADC_MIXER      = 0x15,
    MAX98090_REG_RIGHT_ADC_MIXER     = 0x16,
    MAX98090_REG_LEFT_RECORD_LEVEL   = 0x17,
    MAX98090_REG_RIGHT_RECORD_LEVEL  = 0x18,
    MAX98090_REG_RECORD_BIQUAD_LEVEL = 0x19,
    MAX98090_REG_RECORD_SIDETONE     = 0x1A,

    // Clock Configuration
    MAX98090_REG_SYSTEM_CLOCK       = 0x1B,
    MAX98090_REG_CLOCK_MODE         = 0x1C,
    MAX98090_REG_CLOCK_RATIO_NI_MSB = 0x1D,
    MAX98090_REG_CLOCK_RATIO_NI_LSB = 0x1E,
    MAX98090_REG_CLOCK_RATIO_MI_MSB = 0x1F,
    MAX98090_REG_CLOCK_RATIO_MI_LSB = 0x20,
    MAX98090_REG_MASTER_MODE        = 0x21,

    // Interface Control
    MAX98090_REG_INTERFACE_FORMAT     = 0x22,
    MAX98090_REG_TDM_CONTROL          = 0x23,
    MAX98090_REG_TDM_FORMAT           = 0x24,
    MAX98090_REG_IO_CONFIGURATION     = 0x25,
    MAX98090_REG_FILTER_CONFIGURATION = 0x26,
    MAX98090_REG_DAI_PLAYBACK_LEVEL   = 0x27,
    MAX98090_REG_EQ_PLAYBACK_LEVEL    = 0x28,

    // Headphone Control
    MAX98090_REG_LEFT_HP_MIXER   = 0x29,
    MAX98090_REG_RIGHT_HP_MIXER  = 0x2A,
    MAX98090_REG_HP_CONTROL      = 0x2B,
    MAX98090_REG_LEFT_HP_VOLUME  = 0x2C,
    MAX98090_REG_RIGHT_HP_VOLUME = 0x2D,

    // Speaker Configuration
    MAX98090_REG_LEFT_SPK_MIXER   = 0x2E,
    MAX98090_REG_RIGHT_SPK_MIXER  = 0x2F,
    MAX98090_REG_SPK_CONTROL      = 0x30,
    MAX98090_REG_LEFT_SPK_VOLUME  = 0x31,
    MAX98090_REG_RIGHT_SPK_VOLUME = 0x32,

    // Dynamic Range Control Configuration
    MAX98090_REG_DRC_TIMING     = 0x33,
    MAX98090_REG_DRC_COMPRESSOR = 0x34,
    MAX98090_REG_DRC_EXPANDER   = 0x35,
    MAX98090_REG_DRC_GAIN       = 0x36,

    // Receiver and Line Output
    MAX98090_REG_RCV_LOUTL_MIXER   = 0x37,
    MAX98090_REG_RCV_LOUTL_CONTROL = 0x38,
    MAX98090_REG_RCV_LOUTL_VOLUME  = 0x39,
    MAX98090_REG_LOUTR_MIXER       = 0x3A,
    MAX98090_REG_LOUTR_CONTROL     = 0x3B,
    MAX98090_REG_LOUTR_VOLUME      = 0x3C,

    // Jack Detect and Enable
    MAX98090_REG_JACK_DETECT       = 0x3D,
    MAX98090_REG_INPUT_ENABLE      = 0x3E,
    MAX98090_REG_OUTPUT_ENABLE     = 0x3F,
    MAX98090_REG_LEVEL_CONTROL     = 0x40,
    MAX98090_REG_DSP_FILTER_ENABLE = 0x41,

    // Bias and Power Mode Configuration
    MAX98090_REG_BIAS_CONTROL    = 0x42,
    MAX98090_REG_DAC_CONTROL     = 0x43,
    MAX98090_REG_ADC_CONTROL     = 0x44,
    MAX98090_REG_DEVICE_SHUTDOWN = 0x45,

    // Revision ID
    MAX98090_REG_REVISION_ID = 0xFF,
} MAX98090_Reg;

static const uint8_t MAX98090_REVISION_ID = 0x43;

#define MAX98090A_ADDRESS 0x10
#define MAX98090B_ADDRESS 0x11

// This is the maximum number of CODECs which can be opened at once.
#define HANDLE_MAX 2

struct MAX98090 {
    I2S       *interface;
    I2CMaster *bus;
    GPT       *timer;
    uint8_t    addr;
    bool       mclkExternal;
    unsigned   mclk;
};

static bool MAX98090_RegWrite(MAX98090 *handle,
    uint8_t addr, const uint8_t *data, uintptr_t size)
{
    if (!handle || !data) {
        return false;
    }

    uintptr_t packetSize = (sizeof(addr) + size);

    // This needs to be in sysram as it's too large for the I2C buffer.
    static __attribute__((section(".sysram"))) uint8_t packet[16];
    if (packetSize > sizeof(packet)) {
        return false;
    }

    packet[0] = addr;
    __builtin_memcpy(&packet[sizeof(addr)], data, size);

    return (I2CMaster_WriteSync(
        handle->bus, handle->addr, packet, packetSize) == ERROR_NONE);
}

static bool MAX98090_RegRead(MAX98090 *handle,
    uint8_t addr, uint8_t *data, uintptr_t size)
{
    if (!handle || !data) {
        return false;
    }

    return (I2CMaster_WriteThenReadSync(handle->bus, handle->addr,
        &addr, sizeof(addr), data, size) == ERROR_NONE);
}


static bool MAX98090_Shutdown(MAX98090 *handle, bool shutdown)
{
    uint8_t nshdn = (shutdown ? 0x00 : 0x80);
    return MAX98090_RegWrite(handle, MAX98090_REG_DEVICE_SHUTDOWN, &nshdn, sizeof(nshdn));
}

bool MAX98090_Reset(MAX98090 *handle)
{
    uint8_t reset = 0x80;

    bool success = MAX98090_RegWrite(handle, MAX98090_REG_SOFTWARE_RESET, &reset, sizeof(reset));
    if (!success) {
        return false;
    }

    GPT_WaitTimer_Blocking(handle->timer, 20, GPT_UNITS_MILLISEC);

    return MAX98090_Shutdown(handle, true);
}

static bool MAX98090_Identify(MAX98090 *handle)
{
    uint8_t identity = 0x00;
    MAX98090_RegRead(handle, MAX98090_REG_REVISION_ID, &identity, sizeof(identity));
    return (identity == MAX98090_REVISION_ID);
}

static unsigned gcd(unsigned a, unsigned b)
{
    while (b != 0) {
        unsigned t = b;
        b = a % b;
        a = t;
    }
    return a;
}

static bool MAX98090_Configure(
    MAX98090 *handle, MAX98090_Output output, unsigned channels, unsigned rate)
{
    if (!handle) {
        return false;
    }

    unsigned psclk = 1;
    unsigned pclk = handle->mclk;
    if (handle->mclk > 60000000) {
        return false;
    } else if (handle->mclk > 40000000) {
        psclk = 3;
        pclk /= 4;
    } else if (handle->mclk > 20000000) {
        psclk = 2;
        pclk /= 2;
    }

    unsigned lrclk = rate;
    unsigned bsel;
    switch (channels) {
        case 1:
        case 2:
            bsel = 1;
            break;

        case 3:
            bsel = 2;
            lrclk += (rate >> 1);
            break;

        case 4:
            bsel = 3;
            lrclk *= 2;
            break;

        default:
            return false;
    }

    unsigned freq = 0;
    bool is_16k = (rate == 16000);
    if ((rate == 8000) || is_16k)
    {
        switch (pclk) {
        case 12000000:
            freq = 8 + is_16k;
            break;

        case 13000000:
            freq = 10 + is_16k;
            break;

        case 16000000:
            freq = 12 + is_16k;
            break;

        case 19200000:
            freq = 14 + is_16k;
            break;

        default:
            return false;
        }
    }

    unsigned osr = 128;
    if (pclk < (128 * lrclk)) {
        return false;
    } else if (pclk < (256 * lrclk)) {
        osr = 64;
    }
    unsigned fosr = (lrclk * osr);

    unsigned common = gcd(pclk, fosr);
    if (common == 0) {
        return false;
    }

    unsigned mi = pclk / common;
    if ((mi == 0) || ((mi >> 16) != 0)) {
        return false;
    }

    unsigned ni = fosr / common;
    if ((ni == 0) || ((ni >> 16) != 0)) {
        return false;
    }

    bool tdm = (channels > 2);

    uint8_t regs[] = {
        (psclk << 4),
        (freq << 4) | 0x01,
        (ni >> 8), (ni & 0xFF),
        (mi >> 8), (mi & 0xFF),
        (0x80 | bsel),
        (tdm ? 0x08 : 0x04),
        (tdm ? 0x01 : 0x00),
        (tdm ? 0x10 : 0x00),
        0x01,
    };

    uint8_t outen = 0x3;
    switch (output) {
    case MAX98090_OUTPUT_HEADPHONE:
        outen |= 0xC0;
        break;

    case MAX98090_OUTPUT_RECEIVER:
        outen |= 0x0C;
        break;

    case MAX98090_OUTPUT_SPEAKER:
        outen |= 0x30;
        break;

    default:
        return false;
    }

    if (!MAX98090_RegWrite(handle, MAX98090_REG_SYSTEM_CLOCK, regs, sizeof(regs))) {
        return false;
    }

    // TODO: Separate out configuration of clocks and output/input.

    return MAX98090_RegWrite(handle, MAX98090_REG_OUTPUT_ENABLE, &outen, sizeof(outen));
}


MAX98090 *MAX98090_Open(
    I2CMaster *bus, Platform_Unit interface, GPT *timer,
    MAX98090_Variant variant, bool mclkExternal, unsigned mclk)
{
    if (!bus) {
        return NULL;
    }

    if (variant >= MAX98090_VARIANT_COUNT) {
        return NULL;
    }

    static MAX98090 Handles[HANDLE_MAX] = {0};
    MAX98090 *handle = NULL;
    unsigned h;
    for (h = 0; h < HANDLE_MAX; h++) {
        if (!Handles[h].bus) {
            handle = &Handles[h];
            break;
        }
    }
    if (!handle) {
        return NULL;
    }

    handle->interface = NULL;
    handle->bus       = bus;
    handle->timer     = timer;

    switch (variant) {
    case MAX98090_VARIANT_A:
        handle->addr = MAX98090A_ADDRESS;
        break;

    case MAX98090_VARIANT_B:
        handle->addr = MAX98090B_ADDRESS;
        break;

    default:
        // Unreachable.
        break;
    }

    handle->mclkExternal = mclkExternal;
    handle->mclk         = mclk;

    handle->interface = I2S_Open(interface, (mclkExternal ? 0 : mclk));
    if (!handle->interface) {
        MAX98090_Close(handle);
        return NULL;
    }

    GPT_WaitTimer_Blocking(timer, 20, GPT_UNITS_MILLISEC);

    if (!MAX98090_Reset(handle)
        || !MAX98090_Identify(handle)) {
        MAX98090_Close(handle);
        return NULL;
    }

    return handle;
}

void MAX98090_Close(MAX98090 *handle)
{
    I2S_Close(handle->interface);
    handle->interface = NULL;

    handle->bus   = NULL;
    handle->timer = NULL;
}


bool MAX98090_OutputEnable(MAX98090 *handle,
    MAX98090_Output output, unsigned channels, unsigned bits, unsigned rate,
    bool (*callback)(void *, uintptr_t))
{
    MAX98090_Shutdown(handle, true);

    if (!MAX98090_Configure(handle, output, channels, rate)) {
        return false;
    }

    // We have to wait for at least 2 BCLK cycles, but have no way of detecting this.
    GPT_WaitTimer_Blocking(handle->timer, 20, GPT_UNITS_MILLISEC);

    if (I2S_Output(handle->interface,
        (channels <= 2 ? I2S_FORMAT_I2S : I2S_FORMAT_TDM),
        channels, bits, rate, callback) != ERROR_NONE) {
        return false;
    }

    MAX98090_Shutdown(handle, false);
    return true;
}

bool MAX98090_InputEnable(MAX98090 *handle,
    unsigned input, unsigned channels, unsigned bits, unsigned rate,
    bool (*callback)(void *, uintptr_t))
{
    (void)input;
    (void)channels;
    (void)bits;
    (void)rate;
    (void)callback;

    // TODO: Implement.
    return false;
}
