## Overview

This application is a simple demo; with the user pressing A to print the voltage
on the ADC pins (H2.11 (ADC0), H2.13 (ADC1), H2.12 (ADC2) & H2.14 (ADC3)) to the
debug UART.

## How to build the application

See the top level [README](../README.md) for details.

## Hardware Setup

1. Connect to the debug UART (connect to H3.6 (IO0_TXD) and H3.2 (GND)).
2. Optional - connect voltage dividers from 3.3V to the ADC pins. Note that the voltage on 
   the ADC pins should not be above the `V_ref` for the ADC subsystem (2.5V). See note below
   on voltage dividers. To enable the internal `V_Ref`; you need to bridge jumper J1.
   NB: If no voltage dividers are connected, then the pins will be floating; but this may be
   enough for a simple "does it work" test.
   NB: You can use the same setup from the `ADC_Joystick_RTApp_MT3620_BareMetal` sample.
3. Sideload the application.
4. Follow instructions printed to UART debug, an example of which would be:

```
--------------------------------
ADC_RTApp_MT3620_BareMetal
App built on: Nov  4 2019, 16:44:36
Press A to print ADC pin states.
```

## Connection Diagram

![Connection Diagram](Connection%20Diagram.png)

Note: we assume the use of MT3620 Reference Development Board (RDB) hardware,
such as the MT3620 development kit from Seeed Studio. Different Azure Sphere
hardware may require different wiring: consult its manufacturer’s
documentation.

## Note on Voltage Dividers

[Voltage Dividers](https://en.wikipedia.org/wiki/Voltage_divider) can be used to provide a fixed
voltage to the ADC pins:

V<sub>out</sub> = V<sub>in</sub> &middot; R<sub>1</sub> / (R<sub>1</sub> + R<sub>2</sub>)

If we use a 10k&Omega; resistor for R<sub>2</sub>, and since *V<sub>out</sub> &le; 2.5V*, the above expression dictates that R<sub>1</sub> &ge; 3.2k&Omega;.

We suggest using the following common resistor sizes for R<sub>1</sub> - 4.7k&Omega;, 10k&Omega;,
22k&Omega; & 47k&Omega;.
