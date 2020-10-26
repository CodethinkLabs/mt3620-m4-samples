## Overview

This sample demonstrates how Azure RTOS and Codethink driver work together on MT3620.

This sample app for an MT3620 real-time core running Azure RTOS that creates several threads and 
repeatedly transmits statistic message over a UART. These messages can be read in terminal 
application on a computer using a USB-to-serial adapter. By default, it uses the real-time 
core's dedicated UART, but if your hardware doesn't expose this UART's TX pin, then the sample 
can be altered to use a different UART.

## How to build the application

See the top level [README](../README.md) for details.

## Hardware Setup

1. Connect to the debug UART (H3.6 (IO0_TXD) and H3.2 (GND)) and open a terminal @115200-N-8-1
2. Sideload the application.
3. Observe debug messages from terminal. 
   The number at the end of each line is showing how times each thread is executed. 
   By using thread synchronization object like semaphore, queue or event flag, 
   the code can precisely control how and when these tasks will scheduled. 

   - The statistic numbers refreshes every second. 

```
ThreadX_RTApp_MT3620_AzureRTOS
App built on: Oct  19 2020, 10:00:00

**** Azure Sphere ThreadX Demonstration **** (c) 2006-2020
           thread 0 events sent:          108
           thread 1 messages sent:        3926897
           thread 2 messages received:    3926877
           thread 3 obtained semaphore:   268
           thread 4 obtained semaphore:   268
           thread 5 events received:      107
           thread 6 mutex obtained:       268
           thread 7 mutex obtained:       268
...
```

## Connection Diagram

![Connection Diagram](Connection%20Diagram.png)