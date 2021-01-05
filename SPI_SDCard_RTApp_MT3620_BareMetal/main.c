/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "lib/mt3620/gpt.h"

#include "lib/CPUFreq.h"
#include "lib/VectorTable.h"
#include "lib/NVIC.h"
#include "lib/GPIO.h"
#include "lib/GPT.h"
#include "lib/UART.h"
#include "lib/Print.h"
#include "lib/SPIMaster.h"

#include "SD.h"

/* Set below to control # of blocks read and written */
//#define NUM_BLOCKS_WRITE 8388608 // 4GB
#define NUM_BLOCKS_WRITE 2000

#define NUM_BUTTONS         2
#define MAX_WRITE_BLOCK_LEN 1024
#define NUM_BLOCKS_RW_DELTA 1000UL

static GPT       *buttonTimeout = NULL;
static UART      *debug         = NULL;
static SPIMaster *driver        = NULL;
static SDCard    *card          = NULL;

static uint8_t  dataMultiplier = 1;
static uint32_t numBlocksWrite = NUM_BLOCKS_WRITE;
static uint32_t numBlocksRead  = NUM_BLOCKS_WRITE - NUM_BLOCKS_RW_DELTA;

// Callbacks
typedef struct CallbackNode {
    bool enqueued;
    struct CallbackNode *next;
    void (*cb)(void);
} CallbackNode;

static void EnqueueCallback(CallbackNode *node);

static void printSDBlock(uint8_t *buff, uintptr_t blocklen, unsigned blockID)
{
    UART_Printf(debug, "SD Card Data (block %u):\r\n", blockID);
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

// Read Block
static void buttonA(void)
{
    UART_Print(debug, "Reading card:\r\n");
    uintptr_t blocklen = SD_GetBlockLen(card);
    uint8_t buff[blocklen];

    bool success = true;

    for (uint32_t blockID = 0; blockID < numBlocksRead; blockID++) {
        if (!SD_ReadBlock(card, blockID, buff)) {
            UART_Printf(debug,
                "ERROR: Failed to read block %lu of SD card\r\n", blockID);
            success = false;
        }
        else {
            uintptr_t i;
            if ((blockID % 128) == 0) {
                UART_Printf(debug, "Block %lu:\r\n", blockID);
                printSDBlock(buff, blocklen, blockID);
            }
            for (i = 0; i < blocklen; i++) {
                if (buff[i] != (uint8_t)((i * (dataMultiplier - 1) * blockID) % 255)) {
                    success = false;
                    break;
                }
            }
            if (success) {
                UART_Printf(
                   debug, "Block %lu is as expected\r\n", blockID);
            }
            else {
                UART_Printf(
                    debug, "ERROR: unexpected data (%u != %lu) in block %lu\r\n",
                    buff[i], ((i * (dataMultiplier - 1) * blockID) % 255), blockID);
            }
        }
        if (!success) {
            break;
        }
    }

    if (success) {
        UART_Printf(
            debug, "%lu blocks read and are consistent\r\n",
            numBlocksRead);
    }
}

// Write Block
static void buttonB(void)
{
    UART_Print(debug, "Writing to card:\r\n");

    static uint8_t buff[MAX_WRITE_BLOCK_LEN] = {0};
    uintptr_t blocklen = SD_GetBlockLen(card);

    bool success = true;

    // update buffers
    for (uint32_t blockID = 0; blockID < numBlocksWrite; blockID++) {
        for (uintptr_t i = 0; i < blocklen; i++) {
            buff[i] = (uint8_t)((i * dataMultiplier * blockID) % 255);
        }
        if (!SD_WriteBlock(card, blockID, buff)) {
            UART_Printf(debug,
                "ERROR: Failed to write block %lu of SD card\r\n",
                blockID);
            success = false;
            break;
        }
        else if ((blockID % 256) == 0) {
            UART_Printf(
                debug, "Wrote block %lu successfully (multiplier = %u)\r\n",
                blockID, dataMultiplier);
        }
    }

    if (success) {
        UART_Printf(
            debug, "%lu blocks written successfully\r\n", numBlocksWrite);
    }

    numBlocksWrite += NUM_BLOCKS_RW_DELTA;
    numBlocksRead  += NUM_BLOCKS_RW_DELTA;
    dataMultiplier++;
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
    if (!(buttonTimeout = GPT_Open(
        MT3620_UNIT_GPT0, MT3620_GPT_012_LOW_SPEED, GPT_MODE_REPEAT))) {
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
