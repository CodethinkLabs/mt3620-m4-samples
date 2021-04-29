/* Copyright (c) Microsoft Corporation. All rights reserved.
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

#include "display/display.h"
#include "display/primitive.h"

#define NUM_BUTTONS    2
#define NUM_RGB_COLORS 5

static UART    *debug         = NULL;
static GPT     *buttonTimeout = NULL;

static Display *I2CDisplay;

static unsigned bgIndex = 0;
static Color rgbColors[NUM_RGB_COLORS] = { COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_RED, COLOR_WHITE };

static unsigned shapeIndex = 0;

static Primitive *primitiveI2C = NULL;
static Primitive *primitiveSPI = NULL;

typedef struct CallbackNode {
    bool enqueued;
    struct CallbackNode *next;
    void (*cb)(void);
} CallbackNode;

static void EnqueueCallback(CallbackNode *node);

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

// cycle shape colors
static void buttonA(void)
{
    if (primitiveI2C && !Display_RemovePrimitive(I2CDisplay, primitiveI2C)) {
        UART_Printf(debug, "Issue removing I2C primitive %d!\r\n", primitiveI2C);
    }

    Primitive temp;
    switch (shapeIndex) {
    case 0:
        temp = Point_New((PrimitiveData) {
            .point.point = { 10, 10 },
            .color       = COLOR_BLACK,
            .thickness   = 3,
            .filled      = false});
        break;

    case 1:
        temp = Line_New((PrimitiveData) {
            .line      = {.start = { 10, 10 },
                          .end   = {20, 20}},
            .color     = COLOR_BLACK,
            .thickness = 3,
            .filled    = false});
        break;

    case 2:
        temp = Circle_New((PrimitiveData) {
            .circle     = {.center = { 40, 40 },
                           .radius = 20},
            .color     = COLOR_BLACK,
            .thickness = 3,
            .filled    = false});
        break;

    case 3:
        temp = Rectangle_New((PrimitiveData) {
            .rectangle = {.topLeft     = { 10, 10 },
                          .bottomRight = { 20, 20 }},
            .color     = COLOR_BLACK,
            .thickness = 3,
            .filled    = true});
        break;
    }

    primitiveI2C = Display_AddPrimitive(I2CDisplay, temp);

    if (!primitiveI2C || !primitiveSPI) {
        UART_Printf(debug, "No primitives %d %d!\r\n", primitiveI2C, primitiveSPI);
    }

    UART_Printf(debug, "Filling I2C: %d, %u\r\n", Display_Fill(I2CDisplay, rgbColors[bgIndex]), bgIndex);
    UART_Printf(debug, "Drawing I2C: %d\r\n", Display_Draw(I2CDisplay));

    shapeIndex = (shapeIndex + 1) % 4;
}

// cycle background colors
static void buttonB(void)
{
    bgIndex = (bgIndex + 1) % NUM_RGB_COLORS;
    UART_Printf(debug, "Filling SPI: %d\r\n", Display_Fill(I2CDisplay, rgbColors[bgIndex]));
    UART_Printf(debug, "Drawing SPI: %d\r\n", Display_Draw(I2CDisplay));
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

_Noreturn void RTCoreMain(void)
{
    VectorTableInit();
    CPUFreq_Set(26000000);

    debug = UART_Open(MT3620_UNIT_UART_DEBUG, 115200, UART_PARITY_NONE, 1, NULL);
    UART_Print(debug, "--------------------------------\r\n");
    UART_Print(debug, "Super Serious\r\n");
    UART_Print(debug, "App built on: " __DATE__ " " __TIME__ "\r\n");
    UART_Print(debug, "Press A to toggle image.\r\n");

    I2CDisplay = Display_Open(DISPLAY_TYPE_1306_I2C, MT3620_UNIT_ISU1);
    if (!I2CDisplay) {
        UART_Print(debug, "Error: I2C display initialisation failed\r\n");
    }

    UART_Print(debug, "#1\r\n");

    GPIO_ConfigurePinForInput(buttons[0].gpioPin);
    GPIO_ConfigurePinForInput(buttons[1].gpioPin);

    // Setup GPT1 to poll for button press
    if (!(buttonTimeout = GPT_Open(MT3620_UNIT_GPT1, 1000, GPT_MODE_REPEAT))) {
        UART_Print(debug, "ERROR: Opening timer\r\n");
    }

    UART_Print(debug, "#2\r\n");

    int32_t error;
    if ((error = GPT_StartTimeout(buttonTimeout, 100, GPT_UNITS_MILLISEC,
                                  handleButtonCallback)) != ERROR_NONE) {
        UART_Printf(debug, "ERROR: Starting timer (%ld)\r\n", error);
    }

    UART_Print(debug, "#3\r\n");
    for (;;) {
        __asm__("wfi");
        InvokeCallbacks();
    }
}
