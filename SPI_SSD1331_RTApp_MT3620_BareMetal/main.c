/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "lib/CPUFreq.h"
#include "lib/VectorTable.h"
#include "lib/NVIC.h"
#include "lib/GPIO.h"
#include "lib/GPT.h"
#include "lib/UART.h"
#include "lib/Print.h"
#include "lib/SPIMaster.h"

#include "SSD1331.h"


const uint8_t wheel[] = {
    #include "wheel.h"
};

const uint8_t crayons[] = {
    #include "crayons.h"
};


static const uint32_t buttonAGpio = 12;
static const int buttonPressCheckPeriodMs = 10;
static void HandleButtonTimerIrq(GPT *);

typedef struct CallbackNode {
    bool enqueued;
    struct CallbackNode *next;
    void (*cb)(void);
} CallbackNode;

static void EnqueueCallback(CallbackNode *node);

static UART      *debug   = NULL;
static SSD1331   *display = NULL;
static unsigned   image = 0;
static GPT *buttonTimeout = NULL;

static void HandleButtonTimerIrqDeferred(void)
{
    // Assume initial state is high, i.e. button not pressed.
    static bool prevState = true;
    bool newState;
    GPIO_Read(buttonAGpio, &newState);

    if (newState != prevState) {
        bool pressed = !newState;
        if (pressed) {
            image = (image + 1) % 2;
            if (image == 0) {
                SSD1331_Upload(display, wheel, sizeof(wheel));
            } else {
                SSD1331_Upload(display, crayons, sizeof(crayons));
            }
        }

        prevState = newState;
    }
}

static void HandleButtonTimerIrq(GPT *handle)
{
    (void)handle;
    static CallbackNode cbn = {.enqueued = false, .cb = HandleButtonTimerIrqDeferred};
    EnqueueCallback(&cbn);
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

_Noreturn void RTCoreMain(void)
{
    VectorTableInit();
    CPUFreq_Set(26000000);

    debug = UART_Open(MT3620_UNIT_UART_DEBUG, 115200, UART_PARITY_NONE, 1, NULL);
    UART_Print(debug, "--------------------------------\r\n");
    UART_Print(debug, "SPI_SSD1331_RTApp_MT3620_BareMetal\r\n");
    UART_Print(debug, "App built on: " __DATE__ " " __TIME__ "\r\n");

    // Set the pins as outputs.
    GPIO_ConfigurePinForOutput(0);
    GPIO_ConfigurePinForOutput(1);
    GPIO_ConfigurePinForOutput(2);
    GPIO_ConfigurePinForOutput(3);

    GPIO_ConfigurePinForInput(buttonAGpio);

    SPIMaster *driver = SPIMaster_Open(MT3620_UNIT_ISU1);
    if (!driver) {
        UART_Print(debug,
            "ERROR: SPI initialisation failed\r\n");
    }

    display = SSD1331_Open(driver, 0, 1, 2, 3);
    if (!display) {
        UART_Print(debug,
            "ERROR: Failed to setup display\r\n");
        while (true);
    }

    SSD1331_Upload(display, wheel, sizeof(wheel));

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

    SSD1331_Close(display);
    SPIMaster_Close(driver);
}
