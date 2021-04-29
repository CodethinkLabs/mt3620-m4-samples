**These are interal Microsoft instructions. They will be replaced before release.**

## Overview

This application demonstrates the I2C by writing an image to the SSD1306 I2C screen.

## How to build the application

1. Set up your Azure Sphere device and development environment as described in the 
   [Azure Sphere documentation](https://docs.microsoft.com/azure-sphere/install/install).
2. Even if you've performed this set up previously, ensure you have Azure Sphere 
   SDK version 19.09 or above. In an Azure Sphere Developer Command Prompt, run:
   ```
   azsphere show-version
   ```
   to check.
   Download and install the [latest SDK](https://aka.ms/AzureSphereSDKDownload) as needed.
3. Connect your Azure Sphere device to your PC by USB.
4. Enable [application development](https://docs.microsoft.com/azure-sphere/quickstarts/qs-blink-application#prepare-your-device-for-development-and-debugging),
   if you have not already done so:
   ```
   azsphere device prep-debug
   ```
5. In Visual Studio, open `I2C_OLED_RTApp_MT3620_BareMetal/CMakeLists.txt` and
   press F5 to compile and build the solution and load it onto the device for 
   debugging.

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
