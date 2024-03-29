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
#include "lib/SPIMasterLow.h"

#include "LSM6DS3.h"


static const uint32_t buttonAGpio = 12;
static const int buttonPressCheckPeriodMs = 10;
static void HandleButtonTimerIrq(GPT *);
static void HandleButtonTimerIrqDeferred(void);

static SPIMaster *driver = NULL;
static UART      *debug  = NULL;
static GPT *buttonTimeout = NULL;

static const uint32_t spiChipSelectGPIO = 0;

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

static void displaySensors()
{
    bool hasXL = false, hasG = false, hasTemp = false;
    if (!LSM6DS3_Status(driver, &hasTemp, &hasG, &hasXL)) {
        UART_Print(debug, "ERROR: Failed to read accelerometer status register.\r\n");
    } else {
        int16_t x, y, z;
        if (!hasXL) {
            UART_Print(debug, "INFO: No accelerometer data.\r\n");
        } else if (!LSM6DS3_ReadXLHuman(driver, &x, &y, &z)) {
            UART_Print(debug, "ERROR: Failed to read accelerometer data register.\r\n");
        } else {
            UART_Printf(debug, "INFO: Acceleration: %.3f, %.3f, %.3f\r\n",
                        ((float)x) / 1000, ((float)y) / 1000, ((float)z) / 1000);
        }

        if (!hasG) {
            UART_Print(debug, "INFO: No gyroscope data.\r\n");
        } else if (!LSM6DS3_ReadGHuman(driver, &x, &y, &z)) {
            UART_Print(debug, "ERROR: Failed to read gyroscope data register.\r\n");
        } else {
            UART_Printf(debug, "INFO: Gyroscope: %.3f, %.3f, %.3f\r\n",
                        ((float)x) / 1000, ((float)y) / 1000, ((float)z) / 1000);
        }

        int16_t t;
        if (!hasTemp) {
            UART_Print(debug, "INFO: No temperature data.\r\n");
        } else if (!LSM6DS3_ReadTempHuman(driver, &t)) {
            UART_Print(debug, "ERROR: Failed to read temperature data register.\r\n");
        } else {
            UART_Printf(debug, "INFO: Temperature: %.3f\r\n", ((float)t) / 1000);
        }
        UART_Print(debug, "\r\n");
    }
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
            displaySensors();
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

static void gpioSPIChipSelect(SPIMaster *handle, bool select)
{
    if (!handle) {
        return;
    }

    if (GPIO_Write(spiChipSelectGPIO, !select) != ERROR_NONE) {
        UART_Print(debug, "ERROR: GPIO chip select failed\r\n");
    }
}

_Noreturn void RTCoreMain(void)
{
    VectorTableInit();
    CPUFreq_Set(26000000);

    debug = UART_Open(MT3620_UNIT_UART_DEBUG, 115200, UART_PARITY_NONE, 1, NULL);
    UART_Print(debug, "--------------------------------\r\n");
    UART_Print(debug, "SPI_RTApp_MT3620_BareMetal\r\n");
    UART_Print(debug, "App built on: " __DATE__ " " __TIME__ "\r\n");

    driver = SPIMaster_Open(MT3620_UNIT_ISU1);

    if (!driver) {
        UART_Print(debug, "ERROR: SPI initialisation failed\r\n");
    }

    GPIO_ConfigurePinForOutput(spiChipSelectGPIO);

    // set CS pin to 1 (0 is select)
    if (GPIO_Write(spiChipSelectGPIO, true) != ERROR_NONE) {
        UART_Print(debug,
            "ERROR: GPIO chip select failed\r\n");
    }

    // Use CSB for chip select.
    if (SPIMaster_SetSelectLineCallback(driver, gpioSPIChipSelect) != ERROR_NONE) {
        UART_Print(debug, "ERROR: Failed set select callback.\r\n");
    }

    // Configure SPI Master to 2 MHz.
    SPIMaster_Configure(driver, 0, 0, 2000000);

    if (!LSM6DS3_CheckWhoAmI(driver)) {
        UART_Print(debug,
            "ERROR: CheckWhoAmI Failed for LSM6DS3.\r\n");
    }

    if (!LSM6DS3_Reset(driver)) {
        UART_Print(debug,
            "ERROR: Reset Failed for LSM6DS3.\r\n");
    }

    if (!LSM6DS3_ConfigXL(driver, 1, 4, 400)) {
        UART_Print(debug,
            "ERROR: Failed to configure LSM6DS3 accelerometer.\r\n");
    }

    if (!LSM6DS3_ConfigG(driver, 1, 500)) {
        UART_Print(debug,
            "ERROR: Failed to configure LSM6DS3 accelerometer.\r\n");
    }

    UART_Print(debug,
        "Connect LSM6DS3, and press button A to read accelerometer.\r\n");

    GPIO_ConfigurePinForInput(buttonAGpio);

    // Self test
    displaySensors();

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
