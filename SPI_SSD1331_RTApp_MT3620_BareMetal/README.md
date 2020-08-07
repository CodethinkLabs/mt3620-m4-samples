## Overview

This sample writes to a PMod breakout SPI screen (SSD1331). Two sample images
are written; the current image being toggled with the A button.

## How to build the application

See the top level [README](../README.md) for details.

## Hardware Setup

In terms of setup, this application is similar to the SPI example with a few
extra GPIO's.

![Connection diagram for SSD1331 and MT3620](./Connection%20Diagram.png)

Note: we assume the use of MT3620 Reference Development Board (RDB) hardware,
such as the MT3620 development kit from Seeed Studio. Different Azure Sphere
hardware may require different wiring: consult its manufacturer’s
documentation.

## How to run the application

When connected correctly, the display should start off by showing an image of a
colour wheel, and toggle between that image and an image of some crayons when the
'A' button is pressed.
