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

extern void demo_threadx(void);
UART* debug;

_Noreturn void RTCoreMain(void)
{
    VectorTableInit();
    CPUFreq_Set(197600000);

    debug = UART_Open(MT3620_UNIT_UART_DEBUG, 115200, UART_PARITY_NONE, 1, NULL);
    UART_Print(debug, "--------------------------------\r\n");
    UART_Print(debug, "ThreadX_RTApp_MT3620_AzureRTOS\r\n");
    UART_Print(debug, "App built on: " __DATE__ " " __TIME__ "\r\n");

    demo_threadx();

    // Avoid warnings that main() has noreturn
    for (;;) {
        __asm__("wfi");
    }
}
