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


#define NUM_BUTTONS         2
#define MAX_WRITE_BLOCK_LEN 1024

static GPT       *buttonTimeout = NULL;
static UART      *debug         = NULL;
static SPIMaster *driver        = NULL;
static SDCard    *card          = NULL;

// Callbacks
typedef struct CallbackNode {
    bool enqueued;
    struct CallbackNode *next;
    void (*cb)(void);
} CallbackNode;

static void EnqueueCallback(CallbackNode *node);

// Read Block
static void buttonA(void)
{
    UART_Print(debug, "Reading card:\r\n");
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

// Write Block
static void buttonB(void)
{
    UART_Print(debug, "Writing to card: ");

    static uint8_t buff[MAX_WRITE_BLOCK_LEN] = {0};
    static uint8_t multiplier                = 1;
    uintptr_t blocklen = SD_GetBlockLen(card);

    // update buffer
    for (uintptr_t i = 0; i < blocklen; i++) {
        buff[i] = (uint8_t)((i * multiplier) % 255);
    }
    if (!SD_WriteBlock(card, 0, buff)) {
        UART_Print(debug,
            "FAIL\r\nERROR: Failed to write first block of SD card\r\n");
    }
    else {
        UART_Printf(debug, "OK (x%u)\r\n", multiplier);
    }

    multiplier++;
}

typedef struct ButtonState {
    bool         prevState;
    CallbackNode cbn;
    uint32_t     gpioPin;
} ButtonState;

static ButtonState buttons[NUM_BUTTONS] = {
    {.prevState = true,
     .cbn = {.enqueued = false, .cb = buttonA},
     .gpioPin   = 12},
    {.prevState = true,
     .cbn = {.enqueued = false, .cb = buttonB},
     .gpioPin   = 13}
};

static void handleButtonCallback(GPT *handle)
{
    (void)(handle);
    // Assume initial state is high, i.e. button not pressed.
    bool newState, pressed;

    for (unsigned i = 0; i < NUM_BUTTONS; i++) {
        GPIO_Read(buttons[i].gpioPin, &newState);
        if (newState != buttons[i].prevState) {
            pressed = !newState;
            if (pressed) {
                EnqueueCallback(&buttons[i].cbn);
            }
        }
        buttons[i].prevState = newState;
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
        "Press button A to read block, and B to write block.\r\n"
        "Note that with every press of B, the multiplier on each\r\n"
        "byte is incremented.\r\n\r\n");

    GPIO_ConfigurePinForInput(buttons[0].gpioPin);
    GPIO_ConfigurePinForInput(buttons[1].gpioPin);

    // Setup GPT0 to poll for button press
    if (!(buttonTimeout = GPT_Open(MT3620_UNIT_GPT0, 1000, GPT_MODE_REPEAT))) {
        UART_Print(debug, "ERROR: Opening timer\r\n");
    }
    int32_t error;

    if ((error = GPT_StartTimeout(
        buttonTimeout, 100, GPT_UNITS_MILLISEC, handleButtonCallback)) != ERROR_NONE) {
        UART_Printf(debug, "ERROR: Starting timer (%ld)\r\n", error);
    }

    for (;;) {
        __asm__("wfi");
        InvokeCallbacks();
    }

    SD_Close(card);
}
