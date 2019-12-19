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

static UART* debug = NULL;

#define GPIO_PLAY_R 45
#define GPIO_PLAY_G 46
#define GPIO_PLAY_B 47
#define GPIO_WIFI_R 48

#define NUM_LEDS 4
static bool LED[NUM_LEDS] = {false, true, true, true};
static uint32_t activeLED = 0;

static const uint32_t buttonAGpio = 12;
static const int buttonPressCheckPeriodMs = 10;
static void HandleButtonTimerIrq(GPT *);
static void HandleButtonTimerIrqDeferred(void);

static GPT *buttonTimeout = NULL;

static void updateLEDs()
{
    GPIO_Write(GPIO_PLAY_R, LED[0]);
    GPIO_Write(GPIO_PLAY_G, LED[1]);
    GPIO_Write(GPIO_PLAY_B, LED[2]);
    GPIO_Write(GPIO_WIFI_R, LED[3]);
}

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
            LED[activeLED] = true;
            activeLED++;
            if (activeLED >= NUM_LEDS) {
                activeLED = 0;
            }
            LED[activeLED] = false;

            updateLEDs();
        }
        prevState = newState;
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
    UART_Print(debug, "GPIO_ADC_RTApp_MT3620_BareMetal\r\n");
    UART_Print(debug, "App built on: " __DATE__ ", " __TIME__ "\r\n");
    UART_Print(debug, "Press A to cycle LED state (cycles R-G-B(Play LED)-R(Wifi LED))\r\n");

    GPIO_ConfigurePinForInput(buttonAGpio);

    GPIO_ConfigurePinForOutput(GPIO_PLAY_R);
    GPIO_ConfigurePinForOutput(GPIO_PLAY_G);
    GPIO_ConfigurePinForOutput(GPIO_PLAY_B);
    GPIO_ConfigurePinForOutput(GPIO_WIFI_R);

    updateLEDs();

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
