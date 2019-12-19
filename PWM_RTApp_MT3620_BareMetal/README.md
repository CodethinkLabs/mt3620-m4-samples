## Overview

This application demonstrates the use of the PWM driver to create a breathing
effect on a selection of the on-board LEDs. Three of the PWM outputs are tied
to red, green and blue LEDs on LED 1, and the fourth is connected to the blue
LED on LED 6 (the LED with the WiFi symbol).

## How to build the application
See the top level [README](../README.md) for details.

## Hardware Setup

1. (Optional) Connect to the debug UART (connect to H3.6 (IO0_TXD) and H
   3.2 (GND)).
2. Sideload the application.
3. LED 1 should now cycle through mixes of red, green and blue. LED 6 should 
   simply be breathing on and off.

## Connection Diagram

![Connection Diagram](Connection%20Diagram.png)

Note: we assume the use of MT3620 Reference Development Board (RDB) hardware,
such as the MT3620 development kit from Seeed Studio. Different Azure Sphere
hardware may require different wiring: consult its manufacturer’s
documentation.
