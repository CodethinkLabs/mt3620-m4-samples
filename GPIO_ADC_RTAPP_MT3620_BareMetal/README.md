## Overview

This application cycles the value of GPIOs 45, 46, 47 (connected to R-G-B respectively on 
the Play LED on the dev board) and 48 (connected to the R channel of the dev board WiFi 
LED). The reason this sample title includes ADC is because these GPIOs sit on the ADC pin
block.

## How to build the application

See the top level [README](../README.md) for details.

## Hardware Setup

1. Connect to the debug UART (connect to H3.6 (IO0_TXD) and H3.2 (GND)).
2. Sideload the application.
3. Follow instructions printed to UART debug, an example of which would be:

```
GPIO_ADC_RTApp_MT3620_BareMetal
App built on: Nov  5 2019, 10:14:37
Press A to cycle LED state (cycles R-G-B(Play LED)-R(Wifi LED))
```

## Connection Diagram

![Connection Diagram](Connection%20Diagram.png)

In addition, you can optionally connect the following GPIOs together to check that
individual blocks are working correctly:

GPIO 60 -> 70
GPIO 28 -> 66
GPIO 31 -> 44

Note: we assume the use of MT3620 Reference Development Board (RDB) hardware,
such as the MT3620 development kit from Seeed Studio. Different Azure Sphere
hardware may require different wiring: consult its manufacturer’s
documentation.
