# Overview

This application demonstrates the MT3620 M4 core SPI peripheral. It uses a
wrapper on-top of the SPI drivers to read from an SD card (`SD.h/c`).

Note that you should set the number of blocks to be written / read in main.c
by altering the `#define NUM_BLOCKS_WRITE ...` line.

# How to build the application

See the top level [README](../README.md) for details.

# Hardware Setup

1. Connect to the debug UART (connect to H3.6 (IO0_TXD) and H3.2 (GND)).
2. Connect a Pmod SD SPI breakout to the SPI 1 block (with CSB):
    - ~CS  -> H4.13 (CSB1)
    - MOSI -> H4.11 (MOSI1)
    - MISO -> H4.5 (MISO1)
    - SCK  -> H4.7 (SCLK1)
    - GND  -> H4.2 (GND)
    - VCC  -> H3.3 (3.3V)
    
    See [Connection Diagram](Connection%20Diagram.png) for more detail 
3. Sideload the application.
4. Follow instructions printed to UART debug, an example of which would be:

```
SPI_SDCard_RTApp_MT3620_BareMetal
App built on: Nov  4 2019 15:26:49
Press button A to read block.
```

# Connection Diagram

![Connection Diagram](Connection%20Diagram.png)

Note: we assume the use of MT3620 Reference Development Board (RDB) hardware,
such as the MT3620 development kit from Seeed Studio. Different Azure Sphere
hardware may require different wiring: consult its manufacturer’s
documentation.

# Tested Cards

This sample was tested on a few different SD cards:

    - SanDisk [64GB] Extreme Plus
    - SanDisk [16GB] Ultra
    - Transcend [2GB] (this card still transmits when unselected, which means you have
      to take care with clock bursts)
    - SanDisk [4GB]
    - SanDisk [2GB]