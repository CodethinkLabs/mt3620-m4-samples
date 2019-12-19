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
#include "lib/I2CMaster.h"

#include "SSD1306.h"


static const uint32_t buttonAGpio = 12;
static const int buttonPressCheckPeriodMs = 10;
static void HandleButtonTimerIrq(GPT *);
static void HandleButtonTimerIrqDeferred(void);

static UART      *debug  = NULL;
static I2CMaster *driver = NULL;
static GPT *buttonTimeout = NULL;

static const uint8_t imageData1[] = {
    #include "image_1.h"
};

static const uint8_t imageData2[] = {
    #include "image_2.h"
};

#define IMAGE_COUNT 2
const void *image[IMAGE_COUNT] = {0};
uintptr_t imageSize = 0;
unsigned imageIndex = 0;

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
            imageIndex = (imageIndex + 1) % IMAGE_COUNT;
            SSD1306_WriteFullBuffer(driver, image[imageIndex], imageSize);
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

static void imageRemap(uint8_t *dst, const uint8_t *src)
{
    uint8_t *dp;
    unsigned col;
    for (dp = dst, col = 0; col < SSD1306_WIDTH; col++) {
        unsigned page;
        for (page = 0; page < (SSD1306_HEIGHT / 8); page++, dp++) {
            uint8_t byte = 0;
            unsigned i;
            for (i = 0; i < 8; i++) {
                unsigned y = (page * 8) + i;
                // X inverted to perform bit reversal as PBM is big-endian for bitmaps.
                unsigned xinv = (SSD1306_WIDTH - (col + 1));
                unsigned pixel = (src[((y * SSD1306_WIDTH) + col) / 8] >> (xinv % 8)) & 1;
                // Invert source pixel as PGM stores images inverted.
                byte |= !pixel << i;
            }
            *dp = byte;
        }
    }
}

_Noreturn void RTCoreMain(void)
{
    VectorTableInit();
    CPUFreq_Set(26000000);

    debug = UART_Open(MT3620_UNIT_UART_DEBUG, 115200, UART_PARITY_NONE, 1, NULL);
    UART_Print(debug, "--------------------------------\r\n");
    UART_Print(debug, "I2C_RTApp_MT3620_BareMetal\r\n");
    UART_Print(debug, "App built on: " __DATE__ " " __TIME__ "\r\n");
    UART_Print(debug, "Press A to toggle image.\r\n");

    driver = I2CMaster_Open(MT3620_UNIT_ISU1);
    if (!driver) {
        UART_Print(debug,
            "Error: I2C initialisation failed\r\n");
    }

    I2CMaster_SetBusSpeed(driver, I2C_BUS_SPEED_FAST_PLUS);

    // Initialise the SSD1306.
    bool oledStatus = false;
    oledStatus = Ssd1306_Init(driver);

    if (!oledStatus) {
        UART_Print(debug,
            "Error: OLED initialization failed!\r\n");
    }

    // Remap the image data to match the correct format for the screen
    uint8_t remapData[2][sizeof(imageData1)];
    imageRemap(remapData[0], imageData1);
    imageRemap(remapData[1], imageData2);

    image[0] = remapData[0];
    image[1] = remapData[1];
    imageSize = sizeof(remapData[0]);
    imageIndex = 0;

    SSD1306_WriteFullBuffer(driver, remapData[0], imageSize);
    SSD1306_SetDisplayAllOn(driver, false);

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
