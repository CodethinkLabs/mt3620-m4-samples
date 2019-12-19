/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

#include "lib/CPUFreq.h"
#include "lib/VectorTable.h"
#include "lib/NVIC.h"
#include "lib/GPIO.h"
#include "lib/GPT.h"
#include "lib/UART.h"
#include "lib/Print.h"
#include "lib/ADC.h"

static UART *debug = NULL;
static GPT  *buttonTimeout = NULL;

// ADC global variables
#define ADC_MAX_VAL 0xFFF
#define ADC_DATA_SIZE 8
static __attribute__((section(".sysram"))) uint32_t rawData[ADC_DATA_SIZE];
static ADC_Data data[ADC_DATA_SIZE];
static int32_t adcStatus;

static void callback(int32_t status)
{
    adcStatus = status;
}

static const uint32_t buttonAGpio = 12;
static const int buttonPressCheckPeriodMs = 10;
static void HandleButtonTimerIrq(GPT *);
static void HandleButtonTimerIrqDeferred(void);

typedef struct CallbackNode {
    bool enqueued;
    struct CallbackNode* next;
    void (*cb)(void);
} CallbackNode;

static void EnqueueCallback(CallbackNode* node);

static void HandleButtonTimerIrq(GPT *handle)
{
    (void)handle;
    static CallbackNode cbn = { .enqueued = false,.cb = HandleButtonTimerIrqDeferred };
    EnqueueCallback(&cbn);
}

static void HandleButtonTimerIrqDeferred(void)
{
    // Assume initial state is high, i.e. button not pressed.
    static bool prevState = true;
    bool newState;
    GPIO_Read(buttonAGpio, &newState);

    if (newState != prevState) {
        bool pressed = !newState;
        if (pressed) {
            for (int i = 0; i < adcStatus; i++) {
                uint32_t mV = (data[i].value * 2500) / ADC_MAX_VAL;
                UART_Printf(debug, "Channel: %lu", data[i].channel);
                UART_Printf(debug, ", Data: %lu.%03lu\r\n", (mV / 1000), (mV % 1000));
            }
        }
        prevState = newState;
        adcStatus = 0;
    }
}

static CallbackNode* volatile callbacks = NULL;

static void EnqueueCallback(CallbackNode* node)
{
    uint32_t prevBasePri = NVIC_BlockIRQs();
    if (!node->enqueued) {
        CallbackNode* prevHead = callbacks;
        node->enqueued = true;
        callbacks = node;
        node->next = prevHead;
    }
    NVIC_RestoreIRQs(prevBasePri);
}

static void InvokeCallbacks(void)
{
    CallbackNode* node;
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

_Noreturn void RTCoreMain(void)
{
    VectorTableInit();
    CPUFreq_Set(26000000);

    debug = UART_Open(MT3620_UNIT_UART_DEBUG, 115200, UART_PARITY_NONE, 1, NULL);
    UART_Print(debug, "--------------------------------\r\n");
    UART_Print(debug, "ADC_RTApp_MT3620_BareMetal\r\n");
    UART_Print(debug, "App built on: " __DATE__ ", " __TIME__ "\r\n");
    UART_Print(debug, "Press A to print ADC pin states.\r\n");

    //Initialise ADC driver, and then configure it to use channel 0
    AdcContext *handle = ADC_Open(MT3620_UNIT_ADC0);
    if (ADC_ReadPeriodicAsync(handle, &callback, ADC_DATA_SIZE, data, rawData,
        0xF, 1000, 2500) != ERROR_NONE) {
        UART_Print(debug, "Error: Failed to initialise ADC.\r\n");
    }

    GPIO_ConfigurePinForInput(buttonAGpio);

    // Setup GPT1 to poll for button press
    if (!(buttonTimeout = GPT_Open(MT3620_UNIT_GPT1, 1000, GPT_MODE_REPEAT))) {
        UART_Print(debug, "ERROR: Opening timer\r\n");
    }
    int32_t error;

    if ((error = GPT_StartTimeout(buttonTimeout, buttonPressCheckPeriodMs,
                                  GPT_UNITS_MILLISEC, &HandleButtonTimerIrq)) != ERROR_NONE) {
        UART_Printf(debug, "ERROR: Starting timer (%ld)\r\n", error);
    }

    for (;;) {
        __asm__("wfi");
        InvokeCallbacks();
    }

}
