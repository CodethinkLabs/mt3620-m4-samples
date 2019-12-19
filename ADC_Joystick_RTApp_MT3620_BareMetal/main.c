/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lib/CPUFreq.h"
#include "lib/VectorTable.h"
#include "lib/NVIC.h"
#include "lib/GPIO.h"
#include "lib/GPT.h"
#include "lib/UART.h"
#include "lib/Print.h"
#include "lib/ADC.h"

#include "joystick.h"


static UART *debug = NULL;
static GPT  *buttonTimeout = NULL;

// ADC global variables
#define ADC_DATA_SIZE 4
#define ADC_CHANNELS 2
static __attribute__((section(".sysram"))) uint32_t rawData[ADC_DATA_SIZE];
static ADC_Data data[ADC_DATA_SIZE];
static int32_t adcStatus;

// Joystick variables
#define JOYSTICK_CHANNEL_X 0
#define JOYSTICK_CHANNEL_Y 1
static Joystick_XY joystickValue  = {0};
static Joystick   *joystick       = NULL;
static int32_t     joystickStatus = ERROR;

/* State variable for the joystick finite state machine */
/* Uses joystick direction #defines in joystick.h for calibration phases (0-4) */
int32_t stateFsm = 0;
#define DATA_PHASE 5

static void AdcCallback(int32_t status)
{
    adcStatus = status;
}

static inline void joystickErrCheck(void)
{
    if (joystickStatus == ERROR_JOYSTICK_CAL) {
        UART_Print(debug,
            "Error: The joystick value was not as expected, please try again.""\r\n");
    }
    else if (joystickStatus == ERROR_JOYSTICK_NOT_A_DIRECTION) {
        UART_Print(debug,
            "Error: The direction passed to Joystick_Cal is not a supported value.""\r\n");
    }
}

static const uint32_t buttonAGpio = 12;
static const int buttonPressCheckPeriodMs = 10;
static void HandleButtonTimerIrq(GPT *);
static void HandleButtonTimerIrqDeferred();

typedef struct CallbackNode {
    bool enqueued;
    struct CallbackNode* next;
    void (*cb)(void);
} CallbackNode;

static void EnqueueCallback(CallbackNode *node);

static void HandleButtonTimerIrq(GPT *handle)
{
    (void)handle;
    static CallbackNode cbn = { .enqueued = false,
                                .cb       = HandleButtonTimerIrqDeferred };
    EnqueueCallback(&cbn);
}

static void HandleButtonTimerIrqDeferred()
{
    // Assume initial state is high, i.e. button not pressed.
    static bool prevState = true;
    bool newState;
    GPIO_Read(buttonAGpio, &newState);

    if (newState != prevState) {
        bool pressed = !newState;
        if (pressed) {
            switch (stateFsm) {
                case JOYSTICK_CENTER:
                    joystickStatus = Joystick_Calibrate(joystick, JOYSTICK_CENTER);
                    joystickErrCheck();
                    break;
                case JOYSTICK_Y_MAX:
                    joystickStatus = Joystick_Calibrate(joystick, JOYSTICK_Y_MAX);
                    joystickErrCheck();
                    break;
                case JOYSTICK_Y_MIN:
                    joystickStatus = Joystick_Calibrate(joystick, JOYSTICK_Y_MIN);
                    joystickErrCheck();
                    break;
                case JOYSTICK_X_MAX:
                    joystickStatus = Joystick_Calibrate(joystick, JOYSTICK_X_MAX);
                    joystickErrCheck();
                    break;
                case JOYSTICK_X_MIN:
                    joystickStatus = Joystick_Calibrate(joystick, JOYSTICK_X_MIN);
                    joystickErrCheck();
                    break;
                case DATA_PHASE:
                    joystickValue = Joystick_GetXY(joystick);
                    UART_Printf(debug, "Joystick V_x = %li%% Joystick V_y = %li%%\r\n", joystickValue.x, joystickValue.y);
                    break;
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

static void JoystickCal(int32_t state)
{
    switch (state) {
    case JOYSTICK_CENTER:
        UART_Print(debug,
            "Please move the joystick to its center position."
            " When ready press the A button.\r\n");
        break;
    case JOYSTICK_Y_MAX:
        UART_Print(debug,
            "Please move the joystick to its maximum extent in the y-direction."
            " When ready press the A button.\r\n");
        break;
    case JOYSTICK_Y_MIN:
        UART_Print(debug,
            "Please move the joystick to its minimum extent in the y-direction."
            " When ready press the A button.\r\n");
        break;
    case JOYSTICK_X_MAX:
        UART_Print(debug,
            "Please move the joystick all the way to the right."
            " When ready press the A button.\r\n");
        break;
    case JOYSTICK_X_MIN:
        UART_Print(debug,
            "Please move the joystick all the way to the left."
            " When ready press the A button.\r\n");
        break;
    }
    while (joystickStatus != ERROR_NONE) {
        __asm__("wfi");
        InvokeCallbacks();
    }
    joystickStatus = ERROR;
}

_Noreturn void RTCoreMain(void)
{
    VectorTableInit();
    CPUFreq_Set(26000000);

    debug = UART_Open(MT3620_UNIT_UART_DEBUG, 115200, UART_PARITY_NONE, 1, NULL);
    UART_Print(debug, "--------------------------------\r\n");
    UART_Print(debug, "ADC_RTApp_MT3620_BareMetal\r\n");
    UART_Print(debug, "App built on: " __DATE__ ", " __TIME__ "\r\n");

    // Initialise ADC driver and joystick
    AdcContext* handle = ADC_Open(MT3620_UNIT_ADC0);
    joystick = Joystick_Open(data, ADC_CHANNELS, JOYSTICK_CHANNEL_X, JOYSTICK_CHANNEL_Y);
    if (!joystick) {
        UART_Print(debug,
            "Error: Failed to initialize joystick\r\n");
    }
    // Start ADC to run periodically
    ADC_ReadPeriodicAsync(handle, &AdcCallback, ADC_DATA_SIZE, data, rawData, 0x3, 1000, 2500);

    GPIO_ConfigurePinForInput(buttonAGpio);

    // Setup GPT1 to poll joystick data on button press
    if (!(buttonTimeout = GPT_Open(MT3620_UNIT_GPT1, 1000, GPT_MODE_REPEAT))) {
        UART_Print(debug, "ERROR: Opening timer\r\n");
    }
    int32_t error;

    if ((error = GPT_StartTimeout(buttonTimeout, buttonPressCheckPeriodMs,
                                  GPT_UNITS_MILLISEC, &HandleButtonTimerIrq)) != ERROR_NONE) {
        UART_Printf(debug, "ERROR: Starting timer (%ld)\r\n", error);
    }

    // Calibrate the joystick
    UART_Print(debug,
        "The joystick needs to be calibrated before use.\r\n");

    stateFsm = JOYSTICK_CENTER;
    JoystickCal(stateFsm);

    stateFsm = JOYSTICK_Y_MAX;
    JoystickCal(stateFsm);

    stateFsm = JOYSTICK_Y_MIN;
    JoystickCal(stateFsm);

    stateFsm = JOYSTICK_X_MAX;
    JoystickCal(stateFsm);

    stateFsm = JOYSTICK_X_MIN;
    JoystickCal(stateFsm);

    UART_Print(debug,
        "The joystick is now calibrated, you can now see joystick data by pressing the A button.\r\n");
    stateFsm = DATA_PHASE;

    for (;;) {
        __asm__("wfi");
        InvokeCallbacks();
    }
}
