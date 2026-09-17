# FPV CamBuddy · by FPSteVe — Quick Start

The shortest route from a fresh **ESP32-C3 Super Mini** to automatic camera recording and camera status in your Betaflight OSD.

> FPV CamBuddy is based on **[FreeCLinker by sheeprine](https://github.com/sheeprine/freeclinker)**.

**No CLI setup is required for the normal FPV CamBuddy setup.**

## The 7-step setup

### 1. Flash the C3
Open the **[FPV CamBuddy Web Flasher](https://alfonsogordon.github.io/FPV-CamBuddy/flash.html)**, connect the ESP32-C3 Super Mini by USB and install the current firmware.

When flashing finishes, **power-cycle the C3 once**.

### 2. Wire it to one spare FC UART

| ESP32-C3 Super Mini | Flight controller |
|---|---|
| **GPIO4 / TX** | UART **RX** |
| **GPIO5 / RX** | UART **TX** |
| **GND** | GND |
| **5V** | suitable 5V supply |

**TX goes to RX and RX goes to TX.** The C3 and FC must share ground.

Board layouts vary, so follow the labels/pinout for **your actual C3 Super Mini**.

### 3. Enable the UART in Betaflight
Open **Betaflight Configurator → Ports**, find the UART you wired to the C3 and enable **MSP** at **115200 baud**. Save and reboot.

### 4. Configure FPV CamBuddy
Open **[FPSteVe Easy Config](https://alfonsogordon.github.io/FPV-CamBuddy/config.html)**, connect to the C3 and read the board settings.

For most GoPro FPV setups the defaults are already sensible: arm starts recording, disarm stops after 5 seconds, low-power BLE is enabled, camera status is shown in the OSD, REC-only is used while flying, REC flashes, and the first-arm CLEAN LENS reminder and camera warnings are enabled.

Change anything you need and press **SAVE / APPLY SETTINGS**. A successful save/readback confirms the settings are stored on the C3.

### 5. Put the camera information in your OSD
For **Betaflight 4.5**, select the Pilot/Craft Name method in Easy Config and place **Pilot Name** in Betaflight Configurator → OSD. Craft Name is optional.

For **Betaflight 2026.6+**, select **Custom Messages 1–4** and place the Custom Message elements you want in Betaflight OSD.

### 6. Connect the camera
Power the quad/C3 and wake the camera. FPV CamBuddy should automatically discover and reconnect to a previously paired supported camera.

A new GoPro may need its **Pair** menu opened for the first connection. With Wake Guard enabled, FPV CamBuddy will not deliberately wake a sleeping GoPro just by scanning.

The OSD may briefly show **ERR** while the camera is unavailable; it changes to **RDY** once the camera is connected and ready.

### 7. Bench-test before flying
With the **props removed**:

1. Power the quad and wake the camera — wait for **RDY**.
2. Arm — the camera should start recording and the OSD should show **REC**.
3. Disarm — the normal OSD returns immediately while recording continues for the configured delay.
4. After the default **5 seconds**, recording should stop and the OSD should return to **RDY**.

If that works, the normal setup is complete. 🤘

## Need more control?
Use **FPSteVe Easy Config** for camera, recording, OSD, warnings, AUX and connection settings, plus the integrated OSD Preview.

## Help & feedback
Need help? Join the **Squadding Quads Discord** and ask for **FPSteVe**: https://discord.gg/eE6DkgEnjU

When asking for help, include your **camera model, Betaflight version, C3 board and what the OSD is displaying**.

---

**FPV CamBuddy V1.0** 🤘
