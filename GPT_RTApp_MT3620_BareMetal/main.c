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


#define NUM_BUTTONS 2

// Drivers
static UART *debug    = NULL;
static GPT  *timer[MT3620_UNIT_GPT_COUNT] = {NULL};

// App State
static GPT_TestSpeeds testSpeeds[MT3620_UNIT_GPT_COUNT] = {0};
static uint32_t maxSpeedCount = 0;
static uint32_t speedMode     = 0;

// interrupt timeouts in [ms]
#define itrptTimeoutMin   100
#define itrptTimeoutMax   10000
#define interruptK 10

static unsigned gpioOut[2] = {0, 1};

static uint32_t itrptTimeout = itrptTimeoutMin;

typedef enum {
    APP_MODE_FREERUN   = 0,
    APP_MODE_PAUSE     = 1,
    APP_MODE_INTERRUPT = 2,
    APP_MODE_COUNT
} GPT_AppMode;

static GPT_AppMode appMode = APP_MODE_FREERUN;

// Callbacks
typedef struct CallbackNode {
    bool enqueued;
    struct CallbackNode *next;
    void (*cb)(void);
} CallbackNode;

static void EnqueueCallback(CallbackNode *node);

static void printTimerState(void)
{
    UART_Print(debug, "-----------------------------\r\n");
    UART_Print(debug, "INFO: Timer state:\r\n");
    float    speed = 0;
    uint32_t count = 0;
    uint32_t time  = 0;

    int32_t  error;
    for (unsigned i = 1; i < MT3620_UNIT_GPT_COUNT; i++) {
        if ((error = GPT_GetSpeed(timer[i], &speed)) != ERROR_NONE) {
            UART_Printf(debug, "ERROR: getting GPT%u speed (%ld)\r\n", i, error);
            return;
        }
        count = GPT_GetCount(timer[i]);
        time  = GPT_GetRunningTime(timer[i], GPT_UNITS_SECOND);
        UART_Printf(debug, "INFO: GPT%u speed = %.3fHz\r\n", i, speed);
        UART_Printf(debug, "INFO: GPT%u cnt = %lu\r\n", i, count);
        UART_Printf(debug, "INFO: GPT%u time = %lu [s]\r\n", i, time);
    }
}

static void gpt3TimeoutCallback(GPT *handle)
{
    (void)handle;
    uint32_t numCycles = 0;
    int32_t error;
    if ((error = GPT_GetNumCycles(timer[1], &numCycles)) != ERROR_NONE) {
        UART_Printf(debug, "ERROR: getting GPT1 numCycles, %ld\r\n", error);
    }
    UART_Printf(debug, "INFO: GPT1 cnt = %lu, cycleCnt = %lu.\r\n",
                GPT_GetCount(timer[1]), numCycles);
}

static void executeCountdown(GPT *handle)
{
    int32_t error;
    GPT_Stop(handle);

    // Blocking wait
    if ((error = GPT_SetSpeed(timer[1], testSpeeds[1].speeds[0])) != ERROR_NONE) {
        UART_Printf(debug, "ERROR: Setting speed of blocking wait timer (%ld)\r\n",
                    error);
    }
    if ((error = GPT_WaitTimer_Blocking(
        timer[1], 10, GPT_UNITS_SECOND)) != ERROR_NONE) {
        UART_Printf(debug, "ERROR: Starting blocking wait timer (%ld)\r\n",
                    error);
    }

    UART_Print(debug, "Starting...\r\n");
    // Start all timers >GPT0 in free mode
    for (unsigned i = 1; i < MT3620_UNIT_GPT_COUNT; i++) {
        UART_Printf(debug, "INFO: Starting timer %u.\r\n", i);

        if ((error = GPT_SetSpeed(timer[i], testSpeeds[i].speeds[0])) != ERROR_NONE) {
            UART_Printf(debug, "ERROR: Setting speed of freerun timer (%ld)\r\n",
                        error);
        }

        if ((error = GPT_Start_Freerun(timer[i])) != ERROR_NONE) {
            UART_Printf(debug, "ERROR: Starting freerun timer (%ld)\r\n",
                        error);
        }
    }
    UART_Print(debug, "All timers started\r\n");
}

static void interruptModeHandler(GPT *handle)
{
    static bool gpioOn[2] = {false};

    int32_t index, id;
    if (!handle || ((id = GPT_GetId(handle)) == -1)) {
        return;
    }

    switch (id) {
    case MT3620_UNIT_GPT1:
        index = 0;
        break;
    case MT3620_UNIT_GPT3:
        index = 1;
        break;
    default:
        return;
    }

    gpioOn[index] = !gpioOn[index];
    GPIO_Write(gpioOut[index], gpioOn[index]);
}

static uint32_t getSpeedHz(uint32_t i)
{
    return testSpeeds[i].speeds[speedMode < testSpeeds[i].count ? speedMode : testSpeeds[i].count - 1];
}


// Start timers | Print status | Toggle GPT1 state | Change expire count [depending on appMode]
static void buttonA(void)
{
    static bool started = false;
    int32_t error;
    uint32_t newTimout;

    switch (appMode) {
    case APP_MODE_FREERUN:
        if (!started) {
            // Just for fun, we reuse GPT1 here to do a timeout, then a blocking
            // wait, then start all timers
            if ((error = GPT_SetSpeed(timer[1], testSpeeds[1].speeds[0])) != ERROR_NONE) {
                UART_Printf(debug, "ERROR: Setting speed of timeout (%ld)\r\n",
                            error);
            }
            if ((error = GPT_SetMode(timer[1], GPT_MODE_ONE_SHOT)) != ERROR_NONE) {
                UART_Printf(debug, "ERROR: Setting mode of timeout (%ld)\r\n",
                            error);
            }
            UART_Print(debug, "INFO: Starting timers in 10s...\r\n");
            if ((error = GPT_StartTimeout(timer[1], 100, GPT_UNITS_MILLISEC,
                                          executeCountdown)) != ERROR_NONE) {
                UART_Printf(debug, "ERROR: Starting countdown (%ld)\r\n", error);
            }
            started = true;
        } else {
            printTimerState();
        }
        break;

    case APP_MODE_PAUSE:
        if (GPT_IsEnabled(timer[1])) {
            UART_Print(debug, "INFO: Pausing GPT1.\r\n");
            GPT_Pause(timer[1]);
        } else {
            UART_Print(debug, "INFO: Resuming GPT1.\r\n");
            GPT_Resume(timer[1]);
        }
        break;

    case APP_MODE_INTERRUPT:
        newTimout = itrptTimeout < itrptTimeoutMax ? itrptTimeout * interruptK : itrptTimeoutMin;
        UART_Print(debug, "-----------------------------\r\n");
        UART_Printf(debug, "INFO: Cycling timeout %lu -> %lu [ms].\r\n",
                    itrptTimeout, newTimout);
        itrptTimeout = newTimout;

        // Stop and restart timers with new timeout value
        for (unsigned i = 1; i < MT3620_UNIT_GPT_COUNT; i++) {
            GPT_Stop(timer[i]);
            UART_Printf(debug, "INFO: Restarting timer %u.\r\n", i);
            if ((error = GPT_SetSpeed(timer[i], getSpeedHz(i))) != ERROR_NONE) {
                UART_Printf(debug, "ERROR: Setting speed of timeout (%ld)\r\n",
                            error);
            }
            if ((error = GPT_SetMode(timer[i], GPT_MODE_REPEAT)) != ERROR_NONE) {
                UART_Printf(debug, "ERROR: Setting mode of timeout (%ld)\r\n",
                            error);
            }
            if ((error = GPT_StartTimeout(timer[i], newTimout, GPT_UNITS_MILLISEC,
                                          interruptModeHandler)) != ERROR_NONE) {
                if (error == ERROR_UNSUPPORTED) {
                    UART_Printf(
                        debug, "WARNING: GPT%u doesn't support timeout [expected]\r\n", i);
                } else {
                    UART_Printf(debug, "ERROR: Starting timeout GPT%u (%ld)\r\n", i, error);
                }
            }
        }
        break;

    default:
        UART_Printf(debug, "ERROR: Invalid app mode (%d)\r\n", appMode);
        break;
    }
}

static void cycleSpeedMode(void)
{
    uint32_t newSpeedMode = (speedMode + 1) % maxSpeedCount;
    UART_Print(debug, "-----------------------------\r\n");
    UART_Printf(debug, "INFO: Cycling speed %lu -> %lu.\r\n",
                speedMode, newSpeedMode);

    speedMode = newSpeedMode;
}

// Toggle App Mode
static void buttonB(void)
{
    int32_t error;

    // All app modes require a restart of the timers
    for (unsigned i = 1; i < MT3620_UNIT_GPT_COUNT; i++) {
        GPT_Stop(timer[i]);
    }

    // Mode state machine
    uint32_t oldAppMode  = appMode;
    if ((((appMode == APP_MODE_FREERUN) || (appMode == APP_MODE_INTERRUPT))
            && (speedMode == (maxSpeedCount - 1)))
        || (appMode == APP_MODE_PAUSE)) {
        appMode  = (appMode + 1) % APP_MODE_COUNT;
        UART_Printf(debug, "INFO: Cycling App Mode %lu -> %u.\r\n", oldAppMode, appMode);
    }

    switch (appMode) {
    case APP_MODE_FREERUN:
        // Main free-running mode for timing timers
        if (oldAppMode != appMode) {
            UART_Print(debug, "-----------------------------\r\n");
            UART_Print(debug, "INFO: Freerunning mode. Press B to cycle speeds, "
                              "A to show current state.\r\n");
        }

        cycleSpeedMode();
        // Restart timers with new speeds
        for (unsigned i = 1; i < MT3620_UNIT_GPT_COUNT; i++) {
            UART_Printf(debug, "INFO: Restarting timer %u.\r\n", i);

            if ((error = GPT_SetSpeed(timer[i], getSpeedHz(i))) != ERROR_NONE) {
                UART_Printf(debug, "ERROR: Setting speed of freerun (%ld)\r\n",
                            error);
            }

            GPT_Start_Freerun(timer[i]);
        }
        break;

    case APP_MODE_PAUSE:
        // Extra demo mode for demoing pause and resume
        UART_Print(debug, "-----------------------------\r\n");
        UART_Print(debug, "INFO: Pause/resume mode. Press A to toggle GPT1 pause/resume.\r\n");

        // Set off GPT3 to poll GPT1 cnt and cycleCnt every 5s
        if ((error = GPT_SetSpeed(timer[3], testSpeeds[3].speeds[0])) != ERROR_NONE) {
            UART_Printf(debug, "ERROR: Setting speed of GPT3 timeout (%ld)\r\n",
                        error);
        }
        if ((error = GPT_SetMode(timer[3], GPT_MODE_REPEAT)) != ERROR_NONE) {
            UART_Printf(debug, "ERROR: Setting mode of GPT3 timeout (%ld)\r\n",
                        error);
        }
        if ((error = GPT_StartTimeout(timer[3], 5, GPT_UNITS_SECOND,
                                      gpt3TimeoutCallback)) != ERROR_NONE) {
            UART_Printf(debug, "ERROR: Starting GPT3 timeout (%ld)\r\n",
                        error);
        }

        // Start GPT1 in timeout mode, with no callback
        if ((error = GPT_SetSpeed(timer[1], testSpeeds[1].speeds[0])) != ERROR_NONE) {
            UART_Printf(debug, "ERROR: Setting speed of GPT1 timeout (%ld)\r\n",
                        error);
        }
        if ((error = GPT_SetMode(timer[1], GPT_MODE_REPEAT)) != ERROR_NONE) {
            UART_Printf(debug, "ERROR: Setting mode of GPT1 timeout (%ld)\r\n",
                        error);
        }
        if ((error = GPT_StartTimeout(timer[1], 500, GPT_UNITS_MILLISEC, NULL)) != ERROR_NONE) {
            UART_Printf(debug, "ERROR: Starting GPT1 timeout (%ld)\r\n",
                        error);
        }
        break;

    case APP_MODE_INTERRUPT:
        // Extra demo mode for doing accurate timing of GPT1&3 (interrupt timers)
        // using logic analyser (GPT0 is same ad 1 and is being used for buttons)
        if (oldAppMode != appMode) {
            UART_Print(debug, "-----------------------------\r\n");
            UART_Print(debug, "INFO: Interrupt timer timing mode. \r\n");
            UART_Print(debug, "      Press A to cycle timeout and B to cycle speed\r\n");
            UART_Printf(debug, "INFO: Starting with timeout %lu [ms].\r\n", itrptTimeout);
        }
        cycleSpeedMode();
        // Restart all timers that support timeout mode - some warnings expected
        // (with new speeds)
        for (unsigned i = 1; i < MT3620_UNIT_GPT_COUNT; i++) {
            UART_Printf(debug, "INFO: Restarting timer %u.\r\n", i);
            if ((error = GPT_SetSpeed(timer[i], getSpeedHz(i))) != ERROR_NONE) {
                UART_Printf(debug, "ERROR: Setting speed of GPT%u timeout (%ld)\r\n",
                            i, error);
            }
            if ((error = GPT_SetMode(timer[i], GPT_MODE_REPEAT)) != ERROR_NONE) {
                UART_Printf(debug, "ERROR: Setting mode of GPT%u timeout (%ld)\r\n",
                            i, error);
            }
            if ((error = GPT_StartTimeout(timer[i], itrptTimeout, GPT_UNITS_MILLISEC,
                                          interruptModeHandler)) != ERROR_NONE) {
                if (error == ERROR_UNSUPPORTED) {
                    UART_Printf(debug, "WARNING: GPT%u doesn't support timeout [expected]\r\n", i);
                } else {
                    UART_Printf(debug, "ERROR: Starting timeout GPT%u (%ld)\r\n", i, error);
                }
            }
        }
        break;

    default:
        UART_Printf(debug, "ERROR: Invalid appMode: %u.\r\n", appMode);
        break;

    }
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
    UART_Print(debug, "GPT_RTApp_MT3620_BareMetal\r\n");
    UART_Print(debug, "App built on: " __DATE__ " " __TIME__ "\r\n");

    // Initialise timers and supported speeds
    for (unsigned i = 0; i < MT3620_UNIT_GPT_COUNT; i++) {
        timer[i] = GPT_Open(MT3620_UNIT_GPT0 + i, testSpeeds[i].speeds[1], GPT_MODE_NONE);
        if (!timer[i]) {
            UART_Printf(debug,
                "ERROR: GPT%u initialisation failed\r\n", i);
        }
        GPT_GetTestSpeeds(timer[i], &(testSpeeds[i]));
        maxSpeedCount = testSpeeds[i].count > maxSpeedCount ? testSpeeds[i].count : maxSpeedCount;
    }

    UART_Print(
        debug,
        "For timer timing. Sets all timers (except GPT0) off in freerun mode\r\n"
        "    Press A to start timers, subsequent presses will print timer counts.\r\n"
        "    Press B to cycle through speeds (LOW->MED_LOW->MED_HIGH->HIGH) and modes\r\n"
        "NB: GPT0 is used to monitor button presses\r\n"
        "NB: Only GPT3 supports speeds other than LOW/HIGH; anything not \r\n"
        "    HIGH will be LOW for timer->id != GPT3\r\n");

    GPIO_ConfigurePinForInput(buttons[0].gpioPin);
    GPIO_ConfigurePinForInput(buttons[1].gpioPin);
    GPIO_ConfigurePinForOutput(gpioOut[0]);
    GPIO_ConfigurePinForOutput(gpioOut[1]);

    int32_t error;
    if ((error = GPT_SetMode(timer[0], GPT_MODE_REPEAT)) != ERROR_NONE) {
        UART_Printf(debug, "ERROR: GPT_SetMode failed %ld\r\n", error);
    }
    if ((error = GPT_StartTimeout(
        timer[0], 100, GPT_UNITS_MILLISEC,
        handleButtonCallback)) != ERROR_NONE) {
        UART_Printf(debug, "ERROR: GPT_StartTimeout failed %ld\r\n", error);
    }

    for (;;) {
        __asm__("wfi");
        InvokeCallbacks();
    }
}
