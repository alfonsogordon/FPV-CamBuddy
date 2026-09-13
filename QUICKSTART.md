# FreeCLinker — FPSteVe Edition Quick Start

This is the shortest path from a fresh ESP32-C3 Super Mini to automatic GoPro recording and Betaflight OSD status.

## 1. Flash the C3

Open the **FPSteVe Edition Web Flasher**:

https://alfonsogordon.github.io/freeclinker/flash.html

Connect the ESP32-C3 Super Mini by USB and install the current FPSteVe Edition firmware.

**When flashing finishes, power-cycle the FreeCLinker.** Unplug/replug USB or cycle the board's power once before configuration/testing.

Then open:

https://alfonsogordon.github.io/freeclinker/config.html

## 2. Connect and configure

Connect to the C3 in **FPSteVe Easy Config**. The configurator should automatically read the settings from the board.

A successful read shows:

**🤘 All settings read from C3 ✓**

Fresh-board defaults are already set up for the normal FPSteVe GoPro/FPV workflow, including low-power BLE, automatic recording on arm, 5-second delayed stop on disarm, OSD state, REC-only, CLEAN LENS and camera warnings.

Change only what you need, then use the single **SAVE / APPLY SETTINGS** button. The configurator writes the complete settings snapshot and reads it back to verify it.

A successful save shows:

**🤘 All settings saved and verified on C3 ✓**

## 3. First camera power-up / auto-connect

For the normal GoPro setup, you should **not need to manually connect the camera every time you fly**.

Power the FreeCLinker/quad and make sure the GoPro is awake and available to BLE. On the first normal power-up with the camera available, **FreeCLinker should automatically discover and connect to it**. Once the camera is known, later power-ups should automatically reconnect when that camera is available.

With Wake Guard enabled, FreeCLinker avoids deliberately waking a sleeping GoPro just by scanning. If the camera is asleep, wake it normally; FreeCLinker should then see it and connect automatically.

The status LED is solid when the camera is connected.

## 4. Wire the flight controller

Default ESP32-C3 Super Mini wiring:

| ESP32-C3 | Flight controller |
|---|---|
| GPIO4 TX | UART RX |
| GPIO5 RX | UART TX |
| GND | GND |
| 5V | suitable 5V supply |

The UART runs at **115200 baud**.

TX and RX cross over: C3 TX goes to FC RX, and C3 RX goes to FC TX. A common ground is required.

## 5. Betaflight setup

Enable MSP on the flight-controller UART connected to FreeCLinker and use **115200 baud**.

### Betaflight 4.5

Select the **BF 4.5 / legacy Pilot & Craft Name** method in FPSteVe Easy Config.

The fresh default enables Pilot Name using:

`{stateonly} {batt} {rectf}`

Craft Name is available independently if you want a second line. Its default template is:

`{res} {fps}`

Place the corresponding Pilot Name/Craft Name OSD element where you want it in Betaflight.

### Betaflight 2026.6+

Select **BF 2026.6+ — Custom Messages 1–4**.

Fresh defaults:

| Message | Template |
|---|---|
| 1 | `{batt}` |
| 2 | `{state} {recdur}` |
| 3 | `{mode} {res} {fps} {eis}` |
| 4 | `{rectf} {rcap}` |

Enable/place the Custom Message OSD elements you want in Betaflight.

> The BF 2026.6+ Custom Messages path is implemented and Preview/logic tested, but the FPSteVe V1 test hardware currently runs BF 4.5, so this newer path has not yet been physically FC-tested.

## 6. What the default flight behaviour should look like

Before the GoPro is available, camera state may show **ERR**. Once connected and ready it becomes **RDY**.

Before the first arm after a C3 reboot, the default temporary reminder displays **CLEAN LENS**.

When you arm:

1. FreeCLinker detects the Betaflight arm state.
2. The GoPro starts recording.
3. Camera state changes to **REC**.
4. With the default REC-only option, an enabled OSD message containing the Status token temporarily becomes REC-only while **armed + recording**.
5. REC flashes at **1 Hz** by default.

When you disarm:

1. REC-only ends immediately and your full configured OSD returns.
2. The GoPro continues recording for the configured delay — **5 seconds by default**.
3. During that delay `{state}` still reports **REC**, because the camera really is still recording.
4. FreeCLinker stops the recording after the delay.
5. Camera state returns to **RDY**.

## 7. OSD warnings

Camera warnings are enabled on a fresh configuration:

- **BATT LOW** at 10% camera battery
- **REC LOW** at 5 minutes remaining recording time
- **CAM HOT** when the camera reports the hot condition

Warnings have the highest display priority. The effective order is:

**Warning → Temporary Message → REC-only → normal configured OSD**

## 8. OSD Preview

The configurator contains an integrated OSD Preview so you can check templates and behaviour without repeatedly arming a real quad.

The Preview can simulate arm/recording, CAM HOT and camera-error states. **RESET ARM STATE** resets only the Preview simulation so you can test first-arm behaviour such as CLEAN LENS again.

On the real C3, first-arm state is reboot-based. To test CLEAN LENS from the beginning again on hardware, **power-cycle the C3**.

## 9. Low-power BLE

Low Power is enabled by default.

- Low Power: approximately **−12 dBm / 0.063 mW**
- Normal: approximately **+9 dBm / 7.9 mW**

The low-power setting is intended to minimise unnecessary RF energy close to the receiver/flight electronics while still providing the short-range camera link.

## 10. GoPro connection behaviour

While connected, FPSteVe Edition sends the GoPro keepalive periodically so the camera remains available during the flight session. Hardware testing confirmed the camera stays connected while FreeCLinker is powered.

When FreeCLinker is unplugged/removed, the GoPro is free to follow its normal own auto-power-off behaviour.

## Recovery: Wi-Fi AP

The Wi-Fi AP is off by default in the FPSteVe configuration. If recovery/configuration through the AP is needed, the BOOT button can force AP mode until reboot.

## Before the first real flight

Bench-test the complete installation with props removed:

- GoPro connects automatically when awake/available.
- OSD changes from ERR to RDY.
- ARM starts recording and shows REC.
- DISARM restores the normal OSD immediately.
- Recording stops after the configured delay and returns to RDY.

Only move to a normal flight test once those behaviours are correct.

## Help & feedback

A dedicated **Squadding Quads Discord** help/feedback thread is planned for FPSteVe Edition. Its direct link will be added here when available.

When reporting a problem, camera model + Betaflight version + C3 board + what the OSD displayed are particularly useful.

---

**FPSteVe Edition V1.0 — Actually Final** 🤘
