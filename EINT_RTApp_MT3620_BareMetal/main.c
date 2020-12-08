/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "lib/CPUFreq.h"
#include "lib/VectorTable.h"
#include "lib/NVIC.h"
#include "lib/GPIO.h"
#include "lib/UART.h"
#include "lib/Print.h"


#define NUM_BUTTONS  2
#define COUNT_CYCLE 10

typedef struct {
    uint32_t         pin;
    uint32_t         cnt;
    gpio_eint_attr_t attr;
} ButtonContext;

ButtonContext context[NUM_BUTTONS] =
{
    { .pin = 12, .cnt = 0 },
    { .pin = 13, .cnt = 0 }
};

// Drivers
static UART *debug = NULL;

static void buttonPress(unsigned pin)
{
    if ((pin != 12) && (pin != 13)) {
        return;
    }

    ButtonContext *ctxt = &(context[pin - 12]);
    UART_Printf(
        debug, "EINT %lu triggered (%lu)\r\n",
        ctxt->pin, ctxt->cnt++);

    if (ctxt->cnt == COUNT_CYCLE) {
        gpio_eint_dbnc_freq_e nextFreq =
            (ctxt->attr.freq + 1) % GPIO_EINT_DBNC_FREQ_INVALID;
        UART_Printf(debug, "bounce freq %u -> %u\r\n", ctxt->attr.freq, nextFreq);
        ctxt->attr.freq = nextFreq;

        if (nextFreq == 0) {
            UART_Printf(
                debug, "switching to %s mode\r\n",
                !ctxt->attr.dualEdge ? "dual" : "single");
            ctxt->attr.dualEdge = !ctxt->attr.dualEdge;
        }

        if (EINT_ConfigurePin(
            ctxt->pin, &(ctxt->attr)) != ERROR_NONE) {
            UART_Printf(debug, "Error: reconfiguring pin %lu\r\n", ctxt->pin);
        }

        ctxt->cnt = 0;
    }
}


void gpio_g3_irq0(void)
{
    buttonPress(12);
}


void gpio_g3_irq1(void)
{
    buttonPress(13);
}


_Noreturn void RTCoreMain(void)
{
    VectorTableInit();
    CPUFreq_Set(197600000);

    debug = UART_Open(MT3620_UNIT_UART_DEBUG, 115200, UART_PARITY_NONE, 1, NULL);
    UART_Print(debug, "--------------------------------\r\n");
    UART_Print(debug, "EINT_RTApp_MT3620_BareMetal\r\n");
    UART_Print(debug, "App built on: " __DATE__ " " __TIME__ "\r\n");

    UART_Print(
        debug,
        "Demo of the external interrupt functionality on the MT3620\r\n"
        "Press buttons A and B on the dev board to test\r\n");

    for (unsigned i = 0; i < NUM_BUTTONS; i++) {
        context[i].attr = gpioEINTAttrDefault;

        if (EINT_ConfigurePin(
                context[i].pin, NULL) != ERROR_NONE) {
            UART_Printf(
                debug,
                "Error: configuring pin %lu for external interrupts\r\n",
                context[i].pin);
        }
    }

    for (;;) {
        __asm__("wfi");
    }
}
