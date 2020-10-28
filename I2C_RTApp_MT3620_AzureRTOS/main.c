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

#include "tx_api.h"
#include "i2c_threadx.h"
#include "LSM6DS3.h"

#define STARTUP_RETRY_COUNT     20
#define DEMO_STACK_SIZE         1024
#define DEMO_BYTE_POOL_SIZE     9120

static const uint32_t       buttonAGpio                 = 12;
static const int            buttonPressCheckPeriodMs    = 10;

static UART                 *debug          = NULL;
static i2c_rtos_handle_t    *driver         = NULL;
static GPT                  *buttonTimeout  = NULL;
static GPT                  *startUpTimer   = NULL;
static TX_THREAD            thread;
static TX_SEMAPHORE         semaphore;
static TX_BYTE_POOL         byte_pool;
static UCHAR                memory_area[DEMO_BYTE_POOL_SIZE];

static void HandleButtonTimerIrq(GPT *handle)
{
    // Assume initial state is high, i.e. button not pressed.
    static bool prevState = true;
    bool newState;
    UINT status;

    GPIO_Read(buttonAGpio, &newState);

    if (newState != prevState) {
        bool pressed = !newState;
        if (pressed) {
            status = tx_semaphore_put(&semaphore);
            if (status != TX_SUCCESS) {
                UART_Printf(debug, "ERROR: Failed to post semaphore (%d).\r\n", status);
            }
        }

        prevState = newState;
    }
}

static void displaySensors(void)
{
    bool hasXL = false, hasG = false, hasTemp = false;

    // Wait for sensor board to be ready
    int32_t error;
    static bool initialised = false;
    uint32_t retryRemain = STARTUP_RETRY_COUNT;
    while (retryRemain > 0) {
        if (!LSM6DS3_Status(driver, &hasTemp, &hasG, &hasXL)) {
            UART_Print(debug, "ERROR: Failed to read accelerometer status register.\r\n");
        }
        if (hasTemp && hasG && hasXL) {
            initialised = true;
            break;
        }
        if ((error = GPT_WaitTimer_Blocking(
            startUpTimer, 500, GPT_UNITS_MILLISEC)) != ERROR_NONE) {
            UART_Printf(debug, "ERROR: Failed to start blocking wait (%ld).\r\n", error);
        }
        retryRemain--;
    }

    if (initialised) {
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

static void sensor_thread_entry(ULONG thread_input)
{
    if (tx_byte_allocate(&byte_pool, (VOID**)&driver, 
            sizeof(i2c_rtos_handle_t), TX_NO_WAIT) != TX_SUCCESS) {
        UART_Print(debug,
            "ERROR: Failed to allocate I2C driver.\r\n");
    }

    if (I2CMaster_RTOS_Init(driver, 
            MT3620_UNIT_ISU2, I2C_BUS_SPEED_STANDARD) != ERROR_NONE) {
        UART_Print(debug, 
            "ERROR: Failed to init I2C driver.\r\n");
    }

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

    // Setup GPT1 to poll for button press
    if (!(buttonTimeout = GPT_Open(MT3620_UNIT_GPT1, 1000, GPT_MODE_REPEAT))) {
        UART_Print(debug, "ERROR: Opening button timer\r\n");
    }

    // Setup GPT0 for startup polling
    if (!(startUpTimer = GPT_Open(MT3620_UNIT_GPT0, 1000, GPT_MODE_ONE_SHOT))) {
        UART_Print(debug, "ERROR: Opening startup timer\r\n");
    }

    // Self test
    displaySensors();

    int32_t error;
    if ((error = GPT_StartTimeout(buttonTimeout, buttonPressCheckPeriodMs,
        GPT_UNITS_MILLISEC, &HandleButtonTimerIrq)) != ERROR_NONE) {
        UART_Printf(debug, "ERROR: Starting timer (%ld)\r\n", error);
    }

    while (true) {
        // Wait for btn press event
        if (tx_semaphore_get(&semaphore, TX_WAIT_FOREVER) != TX_SUCCESS) {
            break;
        }

        displaySensors();
    }

    UART_Print(debug, "ERROR: thread exit\r\n");

    // Clean resources
    (void)I2CMaster_RTOS_Deinit(driver);
    (void)tx_byte_release(driver);
}

void tx_application_define(void* first_unused_memory)
{
    CHAR* pointer;

    // Create a byte memory pool from which to allocate the thread stacks.
    tx_byte_pool_create(&byte_pool, "byte pool", memory_area, DEMO_BYTE_POOL_SIZE);

    // Allocate the stack for thread.
    tx_byte_allocate(&byte_pool, (VOID**)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    // Create the main thread.
    tx_thread_create(&thread, "Sensor thread", sensor_thread_entry, 0,
        pointer, DEMO_STACK_SIZE,
        1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);

    // Create the semaphore used by timer interrupt and user thread.
    tx_semaphore_create(&semaphore, "Btn semaphore", 0);

}

_Noreturn void RTCoreMain(void)
{
    VectorTableInit();
    CPUFreq_Set(197600000);

    debug = UART_Open(MT3620_UNIT_UART_DEBUG, 115200, UART_PARITY_NONE, 1, NULL);
    UART_Print(debug, "--------------------------------\r\n");
    UART_Print(debug, "I2C_RTApp_MT3620_AzureRTOS\r\n");
    UART_Print(debug, "App built on: " __DATE__ " " __TIME__ "\r\n");

    // Enter the ThreadX kernel
    tx_kernel_enter();

    // Avoid warnings that main() has noreturn
    for (;;) {
        __asm__("wfi");
    }
}

