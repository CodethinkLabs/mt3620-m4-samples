/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

#include "lib/VectorTable.h"
#include "lib/NVIC.h"
#include "lib/GPIO.h"
#include "lib/GPT.h"
#include "lib/UART.h"
#include "lib/Print.h"

static UART* debug = NULL;

#define LED_1_R 8
#define LED_1_G 9
#define LED_1_B 10
#define LED_6_B 11

#define TIMER_SPEED_HZ 32768
#define TIMER_COUNT_MS 10

#define PWM_BASE_COUNT 1024
#define PWM_STEP_SIZE 4

#define PWM_CLOCK_FREQUENCY 2000000

static GPT *timer = NULL;

static uint32_t pwmOnTime3 = 0;
static bool pwmState3 = false;
static uint32_t pwmRGB[3] = {};
static uint32_t pwmState0 = 0;

void pwmLed(uint32_t pin, uint32_t *pwmOnTime, uint32_t pwmStepSize, bool increment) {
    *pwmOnTime = increment ? (*pwmOnTime + pwmStepSize) : (*pwmOnTime - pwmStepSize);
    PWM_ConfigurePin(pin, PWM_CLOCK_FREQUENCY, *pwmOnTime, PWM_BASE_COUNT);
}

void pwmLedFade(uint32_t pin, uint32_t *pwmOnTime, bool *pwmState) {
    if (*pwmOnTime > PWM_BASE_COUNT - PWM_STEP_SIZE) {
        *pwmState = false;
    }
    else if (*pwmOnTime < PWM_STEP_SIZE) {
        *pwmState = true;
    }
    pwmLed(pin, pwmOnTime, PWM_STEP_SIZE, *pwmState);
}

void callback(GPT *handle) {
    pwmLedFade(LED_6_B, &pwmOnTime3, &pwmState3);
    switch (pwmState0) {
        //Increment red - initial state used once
        case 0:
            pwmLed(LED_1_R, &pwmRGB[0], PWM_STEP_SIZE, 1);
            if (pwmRGB[0]> PWM_BASE_COUNT)
                pwmState0 = 1;
            break;
        //Increment green
        case 1:
            pwmLed(LED_1_G, &pwmRGB[1], PWM_STEP_SIZE, 1);
            if (pwmRGB[1] > PWM_BASE_COUNT)
                pwmState0 = 2;
            break;
        //Decrement red
        case 2:
            pwmLed(LED_1_R, &pwmRGB[0], PWM_STEP_SIZE, 0);
            if (pwmRGB[0] == 0)
                pwmState0 = 3;
            break;
        //Increment blue
        case 3:
            pwmLed(LED_1_B, &pwmRGB[2], PWM_STEP_SIZE, 1);
            if (pwmRGB[2] > PWM_BASE_COUNT)
                pwmState0 = 4;
            break;
        //Decrement green
        case 4:
            pwmLed(LED_1_G, &pwmRGB[1], PWM_STEP_SIZE, 0);
            if (pwmRGB[1] == 0)
                pwmState0 = 5;
            break;
        //Increment red
        case 5:
            pwmLed(LED_1_R, &pwmRGB[0], PWM_STEP_SIZE, 1);
            if (pwmRGB[0] > PWM_BASE_COUNT)
                pwmState0 = 6;
            break;
        //Decrement blue
        case 6:
            pwmLed(LED_1_B, &pwmRGB[2], PWM_STEP_SIZE, 0);
            if (pwmRGB[2] == 0)
                pwmState0 = 1;
            break;
    }
}


_Noreturn void RTCoreMain(void)
{
    VectorTableInit();

    debug = UART_Open(MT3620_UNIT_UART_DEBUG, 115200, UART_PARITY_NONE, 1, NULL);
    UART_Print(debug, "--------------------------------\r\n");
    UART_Print(debug, "PWM_RTApp_MT3620_BareMetal\r\n");
    UART_Print(debug, "App built on: " __DATE__ ", " __TIME__ "\r\n");

    int32_t status = 0;
    status += PWM_ConfigurePin(LED_1_R, PWM_CLOCK_FREQUENCY, 0, PWM_STEP_SIZE);
    status += PWM_ConfigurePin(LED_1_G, PWM_CLOCK_FREQUENCY, 0, PWM_STEP_SIZE);
    status += PWM_ConfigurePin(LED_1_B, PWM_CLOCK_FREQUENCY, 0, PWM_STEP_SIZE);
    status += PWM_ConfigurePin(LED_6_B, PWM_CLOCK_FREQUENCY, 0, PWM_STEP_SIZE);

    if (status != 0) {
        UART_Print(debug, "ERROR: PWM initialisation failed\r\n");
    }

    // Setup GPT1 for updating the PWM cycle
    if (!(timer = GPT_Open(MT3620_UNIT_GPT1, TIMER_SPEED_HZ, GPT_MODE_REPEAT))) {
        UART_Print(debug, "ERROR: Opening timer\r\n");
    }
    int32_t error;

    if ((error = GPT_StartTimeout(
        timer, TIMER_COUNT_MS, GPT_UNITS_MILLISEC, callback
        ) != ERROR_NONE)) {
        UART_Printf(debug, "ERROR: Starting timer (%ld)\r\n", error);
    }

    for (;;) {
        __asm__("wfi");
    }

}
