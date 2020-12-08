## Overview

This application is a demo of the external interrupts on the MT3620.

## How to build the application

See the top level [README](../README.md) for details.

## Hardware Setup

1. Connect to the debug UART (connect to H3.6 (IO0_TXD) and H3.2 (GND)).
2. Sideload the application.
3. Follow instructions printed to UART debug, an example of which would be:

```
EINT_RTApp_MT3620_BareMetal
App built on: Dec  8 2020 18:21:07
Demo of the external interrupt functionality on the MT3620
Press buttons A and B on the dev board to test
```

## Connection Diagram

![Connection Diagram](Connection%20Diagram.png)

Note: we assume the use of MT3620 Reference Development Board (RDB) hardware,
such as the MT3620 development kit from Seeed Studio. Different Azure Sphere
hardware may require different wiring: consult its manufacturer’s
documentation.
