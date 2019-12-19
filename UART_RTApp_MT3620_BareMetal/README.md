# Overview

This sample demonstrates how to communicate over UART in a real-time capable application (RTApp)
for the MT3620. For more information about RTApps, see the
[applications overview](https://docs.microsoft.com/azure-sphere/app-development/applications-overview).

It provides the same user experience as the high level application
[UART sample](https://github.com/Azure/azure-sphere-samples/tree/master/Samples/UART/UART_RTApp_MT3620_BareMetal).
However, it uses two UARTS. It uses the ISU0 UART, which is also available to high-level
applications, and the debug UART, which is specific to the real-time core. It also uses a
general-purpose timer (GPT) on the real-time core. For more information about timers, see 
[General purpose timers](https://docs.microsoft.com/azure-sphere/app-development/use-peripherals-rt.md##general-purpose-timers).
[Use peripherals in a real-time capable application](https://docs.microsoft.com/azure-sphere/app-development/use-peripherals-rt)
provides more detail about the use of UARTs in an RTApp.

A loopback connector joins the TX and RX pins on the ISU0 UART. The debug UART is connected to a
terminal program on the PC via a USB-to-serial adapter.

When you press button A, the application transmits a message on ISU0 UART. The application then
receives the message on ISU0 UART, via the loopback connector. The application then prints the
received message to the debug UART. You can view that message on the terminal program that is
running on the PC.

The debug UART is used in other real-time samples. In those samples, the application polls when
it sends data via the UART. This sample demonstrates a more sophisticated use of the UARTs. It uses
in-memory buffers to both send and receive data, and responds to interrupts rather than polling.
This makes the sample a more useful basis for applications that do not want to block when they send
or receive data.

## To build and run the sample

### Set up hardware to display output

To set up the hardware to display output from the Hello World sample, follow these steps.

1. Connect GND on the breakout adapter to Header 3, pin 2 (GND) on the MT3620 RDB.
2. Connect RX on the breakout adapter to Header 3, pin 6 (real-time core TX) on the MT3620 RDB.
3. Attach the breakout adapter to a USB port on your PC.
4. Determine which COM port the adapter uses on the PC.
    1. Start Device Manager.
    2. Select **View > Devices by container**,
    3. Look for your adapter and note the number of the assigned COM port.
5. On the PC, start the terminal emulator and open a serial terminal with the following settings:
   115200-8-N-1 and the COM port assigned to your adapter.

### Set up the loopback connection

1. Use a jumper header to connect Header 2 pin 1 (RXD0) to Header 2 pin 3 (TXD0).

### Connection Diagram

![Connection Diagram](Connection%20Diagram.png)

### Build and deploy the application

Follow the `Build and Deployment` instructions in the top level [README](../README.md).

The following message is sent to the serial terminal:

```UART_RTApp_MT3620_BareMetal
App built on: May 24 2019 00:29:45
Install a loopback header on ISU0, and press button A to send a message.
```

After you press button A the a message similar to the following will be displayed:

```
UART received 22 bytes: 'RTCore: Hello world!
'.
```
