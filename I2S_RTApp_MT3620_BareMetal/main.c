/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

#include "lib/VectorTable.h"
#include "lib/CPUFreq.h"
#include "lib/NVIC.h"
#include "lib/GPIO.h"
#include "lib/GPT.h"
#include "lib/UART.h"
#include "lib/Print.h"
#include "lib/I2S.h"
#include "lib/I2CMaster.h"

#include "MAX98090.h"


static const uint32_t buttonAGpio = 12;
static const uint32_t buttonBGpio = 13;
static const int buttonPressCheckPeriodMs = 10;
static void HandleButtonTimerIrq(GPT *);
static void HandleButtonTimerIrqDeferred(void);

static I2CMaster *bus   = NULL;
static MAX98090  *codec = NULL;
static UART      *debug = NULL;
static GPT       *timer = NULL;

static unsigned audioRate   = 48000;
static unsigned audioFreq   = 440;
static uint64_t audioPeriod = 0;
static uint64_t audioOffset = 0;

typedef struct CallbackNode {
    bool enqueued;
    struct CallbackNode *next;
    void (*cb)(void);
} CallbackNode;

static void EnqueueCallback(CallbackNode *node);

static void HandleButtonTimerIrq(GPT *handle)
{
    (void)handle;
    static CallbackNode cbn = {.enqueued = false, .cb = HandleButtonTimerIrqDeferred};
    EnqueueCallback(&cbn);
}

uint64_t period(unsigned tone, unsigned rate)
{
    return (((uint64_t)rate * 65536ULL) + (tone / 2)) / tone;
}

static void HandleButtonTimerIrqDeferred(void)
{
    // Assume initial state is high, i.e. button not pressed.
    static bool prevState[2] = { true, true };
    bool newState[2];
    GPIO_Read(buttonAGpio, &newState[0]);
    GPIO_Read(buttonBGpio, &newState[1]);

    unsigned i;
    for (i = 0; i < 2; i++) {
        if (newState[i] != prevState[i]) {
            bool pressed = !newState[i];
            if (pressed) {
                if (i == 0)
                {
                    audioFreq += 10;
                    UART_Printf(debug, "Frequency increased to %u Hz\r\n", audioFreq);
                } else {
                    audioFreq -= 10;
                    UART_Printf(debug, "Frequency decreased to %u Hz\r\n", audioFreq);
                }
                audioPeriod = period(audioFreq, audioRate);
            }

            prevState[i] = newState[i];
        }
    }
}

static CallbackNode *volatile callbacks = NULL;

static void EnqueueCallback(CallbackNode *node)
{
    uint32_t prevBasePri = NVIC_BlockIRQs();
    if (!node->enqueued) {
        CallbackNode *prevHead = callbacks;
        node->enqueued = true;
        callbacks = node;
        node->next = prevHead;
    }
    NVIC_RestoreIRQs(prevBasePri);
}

static void InvokeCallbacks(void)
{
    CallbackNode *node;
    do {
        uint32_t prevBasePri = NVIC_BlockIRQs();
        node = callbacks;
        if (node) {
            node->enqueued = false;
            callbacks = node->next;
        }
        NVIC_RestoreIRQs(prevBasePri);

        if (node) {
            (*node->cb)();
        }
    } while (node);
}

int32_t sine(uint32_t angle)
{
    static const uint16_t table[] = {
        #include "sin.h"
    };

    unsigned phase = (angle >> 16) &    3;
    unsigned imin  = (angle >>  8) & 0xFF;
    unsigned fract =  angle        & 0xFF;

    unsigned imax;
    if (phase & 1) {
        imax = (256 - imin);
        imin = imax - 1;
        fract = (256 - fract);
    } else {
        imax = imin + 1;
    }

    uint32_t min = table[imin];
    uint32_t max = (imax >= 256 ? 0x10000 : table[imax]);
    int32_t value = ((min * (256 - fract)) + (max * fract)) >> 8;

    return (phase & 2 ? -value : value);
}

#define HARMONICS 4

int32_t tone(uint64_t period, uint64_t* offset)
{
    uint32_t angle = ((*offset) * (1ULL << 18)) / period;

    int32_t sample = 0;
    unsigned h;
    for (h = 0; h < HARMONICS; h++) {
        sample += sine(angle * (h + 1)) >> ((h * 2) + 1);
    }

    *offset += 65536;
    if (*offset > period) {
        *offset -= period;
    }
    return sample;
}

static bool audioCallback(uint16_t *data, uintptr_t size)
{
    uintptr_t chunk = (sizeof(int16_t) * 2);
    if (size % chunk) {
        return false;
    }

    while (size >= chunk) {
        int16_t sample = tone(audioPeriod, &audioOffset);
        __builtin_memcpy(data++, &sample, sizeof(sample));
        __builtin_memcpy(data++, &sample, sizeof(sample));
        size -= chunk;
    }
    return true;
}

_Noreturn void RTCoreMain(void)
{
    VectorTableInit();
    CPUFreq_Set(197600000);

    debug = UART_Open(MT3620_UNIT_UART_DEBUG, 115200, UART_PARITY_NONE, 1, NULL);
    UART_Print(debug, "--------------------------------\r\n");
    UART_Print(debug, "I2S_RTApp_MT3620_BareMetal\r\n");
    UART_Print(debug, "App built on: " __DATE__ " " __TIME__ "\r\n");

    audioPeriod = period(audioFreq, audioRate);

    timer = GPT_Open(MT3620_UNIT_GPT1, 32768, GPT_MODE_REPEAT);
    if (!timer) {
        UART_Print(debug, "ERROR: Failed to open timer\r\n");
    }

    bus = I2CMaster_Open(MT3620_UNIT_ISU2);
    if (!bus) {
        UART_Print(debug, "ERROR: I2C bus initialisation failed\r\n");
    }

    codec = MAX98090_Open(
        bus, MT3620_UNIT_I2S0, timer,
        MAX98090_VARIANT_A, false, 16000000);
    if (!codec) {
        UART_Print(debug, "ERROR: I2S initialisation failed\r\n");
    }

    if (!MAX98090_OutputEnable(codec, MAX98090_OUTPUT_HEADPHONE, 2, 16, audioRate, (void *)audioCallback)) {
        UART_Print(debug, "ERROR: Failed to enable output on codec\r\n");
    }

    UART_Print(debug, "Press button A or B to change frequency.\r\n");

    GPIO_ConfigurePinForInput(buttonAGpio);

    int32_t error = GPT_StartTimeout(
        timer, buttonPressCheckPeriodMs,
        GPT_UNITS_MILLISEC, HandleButtonTimerIrq);
    if (error != ERROR_NONE) {
        UART_Printf(debug, "ERROR(%" PRId32 "): Failed to start timer\r\n", error);
    }

    for (;;) {
        __asm__("wfi");
        InvokeCallbacks();
    }
}
