# Overview

A set of sample applications of the [Azure Sphere](https://docs.microsoft.com/en-us/azure-sphere/)
embedded real-time M4 cores.

For more information on developing real-time applications for azure sphere,
refer to this [overview](https://docs.microsoft.com/en-us/azure-sphere/app-development/applications-overview#real-time-capable-applications)
or this [more detailed documentation](https://docs.microsoft.com/en-us/azure-sphere/app-development/rt-app-development-overview).

For detail and documentation on the MT3620; consult
[this Microsoft resource page](https://docs.microsoft.com/en-gb/azure-sphere/hardware/mt3620-product-status).

# Cloning

To use the samples, clone the repository locally if you haven't already done so:

```
git clone https://github.com/CodethinkLabs/mt3620-m4-samples
```

The `libs/` code has been replicated for every sample (to keep them
independent and simple to use out of the box). Therefore; you will also need to
initialize the submodule in each sample directory that points to the
[drivers repo](https://github.com/CodethinkLabs/mt3620-m4-drivers), by doing:

```
git submodule update --init
```

at the top level of this repository.

# Prerequisites

1. [Seeed MT3620 Development Kit](https://aka.ms/azurespheredevkits) or other
   hardware that implements the [MT3620 Reference Development Board (RDB)](https://docs.microsoft.com/azure-sphere/hardware/mt3620-reference-board-design)
   design. Note that all the connection diagrams for the samples assume the
   Seed dev kit.
2. A breakout board and USB-to-serial adapter (for example,
   [FTDI Friend](https://www.digikey.com/catalog/en/partgroup/ftdi-friend/60311))
   to connect the real-time core UART to a USB port on your PC.
3. A terminal emulator (such as Telnet or
   [PuTTY](https://www.chiark.greenend.org.uk/~sgtatham/putty/)) to display
   the output.

# Device Setup

1. Ensure that your Azure Sphere device is connected to your PC, and your PC
   is connected to the internet.
2. Even if you've performed this set up previously, ensure that you have Azure
   Sphere [SDK version 19.10](https://docs.microsoft.com/en-us/azure-sphere/resources/release-notes-1910)
   or above. In an Azure Sphere Developer Command Prompt, run:
   ```
   azsphere show-version
   ```
   to check.
   Download and install the [latest SDK](https://aka.ms/AzureSphereSDKDownload)
   as needed.

   For the I2S samples to work, the device has to be on the 19.11 OS (distinct
   from the SDK). To do this, ensure you have the 19.10 SDK installed, then
   with your device attached:

   - Run the following commands in the Azure Sphere Developer Command Prompt:
   ```
   azsphere product create --name MyProduct
   azsphere device update --productname MyProduct --devicegroupname Development
   ```
   - Ensure the device is on wifi.
   - Reset the device to force it to check for OS updates.

3. Right-click the Azure Sphere Developer Command Prompt shortcut and select
   **More > Run as administrator**.
4. At the command prompt, issue the following command:

   ```sh
   azsphere device enable-development -enablertcoredebugging
   ```

   This command must be run as administrator when you enable real-time core
   debugging because it installs USB drivers for the debugger.
5. Close the window after the command completes because administrator privilege
   is no longer required.
    **Note:** As a best practice, you should always use the lowest privilege
    that can accomplish a task.

Note: All the samples require you to connect your USB-to-serial adapter to the
debug UART.
To connect to the debug UART on the Seed MT3620 dev kit, (and assuming FTDI
Friend or similar) connect:

    - RX  -> H3.6 (IO0_TXD)
    - GND -> H3.2 (GND)

# Updating to the latest SDK

If set up correcly, the device should automatically update itself to the
latest OS. However, you currently have to update these samples manually to the
corresponding SDK. Info on the updates is published here:

https://azure.microsoft.com/en-gb/updates/

The SDK is simple to install, the instructions are currently hosted here:

https://docs.microsoft.com/en-us/azure-sphere/install/install-sdk

You have to tell CMake which API you are targetting however (the new SDKs
typically don't support the older APIs). The last time we had to do this,
it involved updating the CMakeSettings.json files as follows:

   ```
   -       "AzureSphereTargetApiSet": "4+Beta2001"
   +       "AzureSphereTargetApiSet": "5+Beta2004"
   ```

This means that we're now targetting the Beta version of API 5, with the
SDK version 20.04.

In future there will be infrastructure that allow you to avoid these manual
changes, but for now, something like the above will be required.

Note that the warning message when you try to target an invalid API will
tell you what APIs the current SDK supports.

You will also need to update CMakeLists.txt with the same information, see
the calls to the `azsphere_configure_tools` and `azsphere_configure_api`
functions for more detail.

# Build and Deployment (Sideloading)

All samples can be run in the same way. Note: You should open the individual
sample directory in Visual Studio (i.e. not the whole sample repository).

1. Start Visual Studio. From the **File** menu, select **Open > Make...** and
   navigate to the folder that contains the sample.
2. Select the sample's CMakeLists.txt and then click **Open**
3. From the **CMake** menu (if present), select **Build All**. If the menu is
   not present, open Solution Explorer, right-click the CMakeLists.txt file,
   and select **Build**. This step automatically performs the manual packaging
   steps. The output location of the Azure Sphere application appears in the
   Output window.
4. From the **Select Startup Item** menu, on the tool bar, select
   **GDB Debugger (RTCore)**.
5. Press F5 to start the application with debugging.

# License
For details on license, see LICENSE.txt in this directory.
