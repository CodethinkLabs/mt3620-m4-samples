## Overview

This application is a demo of the timer subsystem embedded in each of the
MT3620 M4 cores. There are three modes, each reachable by pressing B (you will
need to cycle through speeds for 2 of the modes before the mode will
transition to the next):

1. Freerunning timer mode - timers can be set off running in free-running 
   mode, changing the speed by pressing B, and pressing A to display the
   current count.
2. Pause / resume mode - for testing the pause / resume methods; press A to 
   toggle pause / resume. GPT3 is used to periodically poll GPT1 for 
   information on current count and number of cycles (interrupts).
3. Interrupt Mode - for timing the interrupts using a logic analyser. This
   mode sets off GPT1 & 3 at 0.1s, 1s and 10s intervals (cycle through by
   pressing A). With every interrupt, the app toggles the signal level on
   GPIO 0 (for GPT1) or 1 (for GPT3). The timers can be run at the same
   speeds as for Freerunning mode.

## How to build the application

See the top level [README](../README.md) for details.

## Hardware Setup

1. Connect to the debug UART (connect to H3.6 (IO0_TXD) and H3.2 (GND)).
2. Optional - connect a logic analyser to H1.2 (GND), H1.4 (GPIO 0) and H1.6
   (GPIO1).
3. Sideload the application.
4. Follow instructions printed to UART debug, an example of which would be:

```
GPT_RTApp_MT3620_BareMetal
App built on: Oct 22 2019 12:01:18
For timer timing. Sets all timers (except GPT0) off in freerun mode
    Press A to start timers, subsequent presses will print timer counts.
    Press B to cycle through speeds (LOW->MED_LOW->MED_HIGH->HIGH) and modes
NB: GPT0 is used to monitor button presses
NB: Only GPT3 supports speeds other than LOW/HIGH; anything not
    HIGH will be LOW for timer->id != GPT3
```

## Connection Diagram

![Connection Diagram](Connection%20Diagram.png)

Note: we assume the use of MT3620 Reference Development Board (RDB) hardware,
such as the MT3620 development kit from Seeed Studio. Different Azure Sphere
hardware may require different wiring: consult its manufacturer’s
documentation.