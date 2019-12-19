## Overview

This application demonstrates the I2C by writing an image to the SSD1306 I2C screen.

## How to build the application

See the top level [README](../README.md) for details.

## Hardware Setup

1. Connect to the debug UART (connect to H3.6 (IO0_TXD) and H3.2 (GND)).
2. Connect SSD1306 to the MT3620 according to the  [Connection Diagram](Connection%20Diagram.png).
3. Sideload the application.
4. Follow instructions printed to UART debug, an example of which would be:

```
--------------------------------
I2C_RTApp_MT3620_BareMetal
App built on: Nov  5 2019 14:39:35
Press A to toggle image.
```

## Connection Diagram

![Connection Diagram](Connection%20Diagram.png)

Note: we assume the use of MT3620 Reference Development Board (RDB) hardware,
such as the MT3620 development kit from Seeed Studio. Different Azure Sphere
hardware may require different wiring: consult its manufacturer’s
documentation.
