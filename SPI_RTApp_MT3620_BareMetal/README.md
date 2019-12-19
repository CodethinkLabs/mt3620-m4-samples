## Overview

This sample demos SPI; reading from an attached LSM6DS3 sensor breakout board 
when the user presses A.

## How to build the application

See the top level [README](../README.md) for details.

## How to run the application

1. Use a header to connect H3.6 (IO0_TXD), also H3.3 (3.3V) and H3.2 (GND).
2. Connect the LSM6DS3 in accordance with the [Connection Diagram](Connection%20Diagram.png).
3. Sideload the application.
4. Press button A to transfer data on the looped-back ISU0 UART.
5. The terminal program which is connected to the M4 debug port should display
   something like the following:

```
'--------------------------------'.
'SPI_RTApp_MT3620_BareMetal!'
'App built on: (date) (time)'
'Connect LSM6DS3, and press button A to read accelerometer.'
'INFO: Acceleration: x, y, z'
'.
```

## Connection Diagram

![Connection Diagram](Connection%20Diagram.png)

Note: we assume the use of MT3620 Reference Development Board (RDB) hardware,
such as the MT3620 development kit from Seeed Studio. Different Azure Sphere
hardware may require different wiring: consult its manufacturer’s
documentation.
