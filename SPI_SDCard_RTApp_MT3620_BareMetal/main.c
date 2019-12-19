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

#include "SD.h"


static const uint32_t buttonAGpio = 12;
static const int buttonPressCheckPeriodMs = 10;
static void HandleButtonTimerIrq(GPT *);
static void HandleButtonTimerIrqDeferred(void);

static UART      *debug  = NULL;
static SPIMaster *driver = NULL;
static SDCard    *card   = NULL;
static GPT *buttonTimeout = NULL;

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

static void HandleButtonTimerIrqDeferred(void)
{
    // Assume initial state is high, i.e. button not pressed.
    static bool prevState = true;
    bool newState;
    GPIO_Read(buttonAGpio, &newState);

    if (newState != prevState) {
        bool pressed = !newState;
        if (pressed) {
            uintptr_t blocklen = SD_GetBlockLen(card);
            uint8_t buff[blocklen];
            if (!SD_ReadBlock(card, 0, buff)) {
                UART_Print(debug,
                    "ERROR: Failed to read first block of SD card\r\n");
            } else {
                UART_Print(debug, "SD Card Data:\r\n");
                uintptr_t i;
                for (i = 0; i < blocklen; i++) {
                    UART_PrintHexWidth(debug, buff[i], 2);
                    UART_Print(debug, ((i % 16) == 15 ? "\r\n" : " "));
                }
                if ((blocklen % 16) != 0) {
                    UART_Print(debug, "\r\n");
                }
                UART_Print(debug, "\r\n");
            }
        }

        prevState = newState;
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

_Noreturn void RTCoreMain(void)
{
    VectorTableInit();
    CPUFreq_Set(197600000);

    debug = UART_Open(MT3620_UNIT_UART_DEBUG, 115200, UART_PARITY_NONE, 1, NULL);
    UART_Print(debug, "--------------------------------\r\n");
    UART_Print(debug, "SPI_SDCard_RTApp_MT3620_BareMetal\r\n");
    UART_Print(debug, "App built on: " __DATE__ " " __TIME__ "\r\n");

    driver = SPIMaster_Open(MT3620_UNIT_ISU1);
    if (!driver) {
        UART_Print(debug,
            "ERROR: SPI initialisation failed\r\n");
    }
    SPIMaster_DMAEnable(driver, false);

    // Use CSB for chip select.
    SPIMaster_Select(driver, 1);

    card = SD_Open(driver);
    if (!card) {
        UART_Print(debug,
            "ERROR: Failed to open SD card.\r\n");
    }

	UART_Print(debug,
        "Press button A to read block.\r\n");

    GPIO_ConfigurePinForInput(buttonAGpio);

    // Setup GPT0 to poll for button press
    if (!(buttonTimeout = GPT_Open(MT3620_UNIT_GPT0, 1000, GPT_MODE_REPEAT))) {
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

    SD_Close(card);
}
