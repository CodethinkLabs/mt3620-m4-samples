## Overview

This application is a demo of the I2S API and subsystem.
The demo currently only showcases the output channel of the first I2S interface
playing various user configurable tones at 48KHz stereo.

The frequency starts out at 440Hz with 4 harmonics and can be increased or decreased
by 10Hz by pressing B or A respectively.


## How to build the application

See the top level [README](../README.md) for details.

## Hardware Setup

1. Connect to the debug UART (connect to H3.6 (IO0_TXD) and H3.2 (GND)).
2. Ensure MAX98090 jumpers are in the default position as stated in the MAX98090 eval manual.
3. Remove jumpers from the MAX98090 Eval board (or hang off ground pin):
    - JU14 (Clock selection)
    - JU15 (BCLK, LRCLK, DAC, ADC)
    - JU16 (SCL, SDA)
4. Connect:  
    a. MAX98090 Eval board I2S interface to the MT3620 reference development board:

    - H1.3  (I2S0_RX  ) -> JU15/ADC   (centre pin)
    - H1.5  (I2S0_TX  ) -> JU15/DAC   (centre pin)
    - H1.7  (I2S0_FS  ) -> JU15/LRCLK (centre pin)
    - H1.9  (I2S0_MCLK) -> JU14/Pin1  (centre pin)
    - H1.11 (I2S0_BCLK) -> JU15/BCLK  (centre pin)

    b. Connect I2C interface, with external 10k pull-up resistors to the MT3620 3.3V pin:
    - H4.6  (SDA2) -> JU16/SDA   (centre pin)
    - H4.12 (SCL2) -> JU16/SCL   (centre pin)

    NB: see [Connection Diagram](Connection%20Diagram.png) for details.
5. Connect headphones to MAX9890 eval board.
6. Connect MAX98090 Eval board power an grounds:
    - Connect any GND on the MAX98090 to a ground pin on the MT3620 (Hx.2).
    - USB CONTROL should be powered ideally from the same source as the MT3620 board.
    - Connect SPKVDD test point to a 4.2V power supply.
7. Turn on SPKVDD power supply.
8. Sideload the application.
9. Follow instructions printed to UART debug, an example of which would be:

```
I2S_RTApp_MT3620_BareMetal
App built on: Dec  4 2019 17:05:32
Press button A or B to change frequency.
```

## Connection Diagram

![Connection Diagram](Connection%20Diagram.png)

Note: we assume the use of MT3620 Reference Development Board (RDB) hardware,
such as the MT3620 development kit from Seeed Studio. Different Azure Sphere
hardware may require different wiring: consult its manufacturer’s
documentation.