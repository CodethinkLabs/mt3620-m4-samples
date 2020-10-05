/* Copyright (c) Microsoft Corporation. All rights reserved.
   Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

// This sample C application for the real-time core demonstrates intercore communications by
// sending a message to a high-level application every second, and printing out any received
// messages.
//
// It demontrates the following hardware
// - UART (used to write a message via the built-in UART)
// - mailbox (used to report buffer sizes and send / receive events)
// - timer (used to send a message to the HLApp)

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
#include "lib/UART.h"
#include "lib/Print.h"
#include "lib/GPT.h"

#include "Socket.h"

#define NUM_BUTTONS 2

typedef enum {
    TIMER_BUTTONS,
    TIMER_SEND_MSG,
    TIMER_COUNT
} appTimers;

// Drivers
static UART   *debug              = NULL;
static GPT    *timer[TIMER_COUNT] = {NULL};

static Socket *socket             = NULL;

static unsigned gpioOut[2] = {0, 1};

static volatile unsigned msgCounter = 0;

// Callbacks
typedef struct CallbackNode {
    bool enqueued;
    struct CallbackNode *next;
    void *data;
    void (*cb)(void*);
} CallbackNode;

static void EnqueueCallback(CallbackNode *node);

// Msg callbacks
// Prints an array of bytes
static void printBytes(const uint8_t *bytes, uintptr_t start, uintptr_t size)
{
    for (unsigned i = start; i < size; i++) {
        UART_Printf(debug, "%x", bytes[i]);
    }
}

static void printComponentId(const Component_Id *compId)
{
    UART_Printf(debug, "%lx-%x-%x", compId->seg_0, compId->seg_1, compId->seg_2);
    UART_Print(debug, "-");
    printBytes(compId->seg_3_4, 0, 2);
    UART_Print(debug, "-");
    printBytes(compId->seg_3_4, 2, 8);
    UART_Print(debug, "\r\n");
}

static void handleSendMsgTimer(void* data)
{
    static const Component_Id A7ID =
    {
        .seg_0   = 0x25025d2c,
        .seg_1   = 0x66da,
        .seg_2   = 0x4448,
        .seg_3_4 = {0xba, 0xe1, 0xac, 0x26, 0xfc, 0xdd, 0x36, 0x27}
    };

    static char msg[] = "rt-app-to-hl-app-00";
    const uintptr_t msgLen = sizeof(msg);

    msg[msgLen - 2] = '0' + (msgCounter % 10);
    msg[msgLen - 3] = '0' + (msgCounter / 10);

    int32_t error = Socket_Write(socket, &A7ID, msg, msgLen);

    if (error != ERROR_NONE) {
        UART_Printf(debug, "ERROR: sending msg %s - %ld\r\n", msg, error);
    }
}

static void handleSendMsgTimerWrapper(GPT *timer)
{
    (void)(timer);

    static CallbackNode cbn = {.enqueued = false, .cb = handleSendMsgTimer, .data = NULL};
    EnqueueCallback(&cbn);
}

static void handleRecvMsg(void *handle)
{
    Socket *socket = (Socket*)handle;

    Component_Id senderId;
    static char msg[32];
    uint32_t msg_size = sizeof(msg);

    int32_t error = Socket_Read(socket, &senderId, msg, &msg_size);

    if (error != ERROR_NONE) {
        UART_Printf(debug, "ERROR: receiving msg %s - %ld\r\n", msg, error);
    }

    msg[msg_size] = '\0';
    UART_Printf(debug, "Message received: %s\r\nSender: ", msg);
    printComponentId(&senderId);
}

static void handleRecvMsgWrapper(Socket *handle)
{
    static CallbackNode cbn = {.enqueued = false, .cb = handleRecvMsg, .data = NULL};

    if (!cbn.data) {
        cbn.data = handle;
    }
    EnqueueCallback(&cbn);
}

// Button Callbacks
// Enqueued when user presses A
static void buttonA(void *data)
{
    (void)data;
    msgCounter = (msgCounter + 1) % 100;
    UART_Printf(debug, "Incrementing counter: %u\r\n", msgCounter);
}

// Enqueued when user presses B
static void buttonB(void *data)
{
    (void)data;
    msgCounter = (msgCounter - 1) % 100;
    UART_Printf(debug, "Decrementing counter: %u\r\n", msgCounter);
}

typedef struct ButtonState {
    bool         prevState;
    CallbackNode cbn;
    uint32_t     gpioPin;
} ButtonState;

static ButtonState buttons[NUM_BUTTONS] = {
    {.prevState = true,
     .cbn = {.enqueued = false, .cb = buttonA, .data = NULL},
     .gpioPin   = 12},
    {.prevState = true,
     .cbn = {.enqueued = false, .cb = buttonB, .data = NULL},
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
            (node->cb)(node->data);
        }
    } while (node);
}

_Noreturn void RTCoreMain(void)
{
    VectorTableInit();
    CPUFreq_Set(197600000);

    debug = UART_Open(MT3620_UNIT_UART_DEBUG, 115200, UART_PARITY_NONE, 1, NULL);
    UART_Print(debug, "--------------------------------\r\n");
    UART_Print(debug, "IntercoreComms_MT3620_BareMetal\r\n");
    UART_Print(debug, "App built on: " __DATE__ " " __TIME__ "\r\n");

    // Initialise timers and supported speeds
    for (unsigned i = 0; i < TIMER_COUNT; i++) {
        timer[i] = GPT_Open(MT3620_UNIT_GPT0 + i, MT3620_GPT_012_HIGH_SPEED, GPT_MODE_REPEAT);
        if (!timer[i]) {
            UART_Printf(debug,
                "ERROR: GPT%u initialisation failed\r\n", i);
        }
    }

    // Setup socket
    socket = Socket_Open(handleRecvMsgWrapper);
    if (!socket) {
        UART_Printf(debug, "ERROR: socket initialisation failed\r\n");
    }

    GPIO_ConfigurePinForInput(buttons[0].gpioPin);
    GPIO_ConfigurePinForInput(buttons[1].gpioPin);
    GPIO_ConfigurePinForOutput(gpioOut[0]);
    GPIO_ConfigurePinForOutput(gpioOut[1]);

    // Setup buttons
    int32_t error;
    if ((error = GPT_SetMode(timer[TIMER_BUTTONS], GPT_MODE_REPEAT)) != ERROR_NONE) {
        UART_Printf(debug, "ERROR: Button GPT_SetMode failed %ld\r\n", error);
    }
    if ((error = GPT_StartTimeout(
        timer[TIMER_BUTTONS], 100, GPT_UNITS_MILLISEC,
        handleButtonCallback)) != ERROR_NONE) {
        UART_Printf(debug, "ERROR: Button GPT_StartTimeout failed %ld\r\n", error);
    }

    // Setup Msg out
    if ((error = GPT_SetMode(timer[TIMER_SEND_MSG], GPT_MODE_REPEAT)) != ERROR_NONE) {
        UART_Printf(debug, "ERROR: Msg GPT_SetMode failed %ld\r\n", error);
    }
    if ((error = GPT_StartTimeout(
        timer[TIMER_SEND_MSG], 1, GPT_UNITS_SECOND,
        handleSendMsgTimerWrapper)) != ERROR_NONE) {
        UART_Printf(debug, "ERROR: Msg GPT_StartTimeout failed %ld\r\n", error);
    }

    for (;;) {
        __asm__("wfi");
        InvokeCallbacks();
    }
}
