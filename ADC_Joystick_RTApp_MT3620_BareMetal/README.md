## Overview

This application uses two ADC channels in order to interface with a joystick.
The sample prints instructions over the debug UART and guides you through
calibration of the joystick. Once calibration is finished, the sample then
prints the current position of the joystick relative to the centre when the A
button is pressed.

## How to build the application
See the top level [README](../README.md) for details.

## Hardware Setup

1. Connect to the debug UART (connect to H3.6 (IO0_TXD) and H3.2 (GND)).
2. Connect up the joystick as shown in the connection diagram.
3. Sideload the application.
4. Follow instructions printed to UART debug to calibrate the joystick.
5. After calibration is finished, press the A button to print the joystick position.

## Connection Diagram

![Connection Diagram](Connection%20Diagram.png)

Note: we assume the use of MT3620 Reference Development Board (RDB) hardware,
such as the MT3620 development kit from Seeed Studio. Different Azure Sphere
hardware may require different wiring: consult its manufacturer’s
documentation.
