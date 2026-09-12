# FreeCLinker — FPSteVe Edition

**FreeCLinker — FPSteVe Edition** is my fork of **[FreeCLinker](https://github.com/sheeprine/freeclinker)**, the ESP32 camera-to-Betaflight bridge. It keeps the original project's camera control and telemetry workflow while adding the FPSteVe OSD/status, warning, Temporary Message, bench-testing and configurator work.

## FPSteVe hosted tools

- **[Open the FPSteVe Configurator](https://alfonsogordon.github.io/freeclinker/config.html)** — configure the ESP32 over Web Serial, select the Betaflight OSD method and set camera/status behaviour.
- **[Open OSD Preview](https://alfonsogordon.github.io/freeclinker/test.html)** — preview the selected OSD layout and use Demo Mode without a flight controller or ESP32 attached.
- **[Open the browser flasher](https://alfonsogordon.github.io/freeclinker/flash.html)** — flash published FPSteVe Edition firmware releases.
- **[FPSteVe Edition project site](https://alfonsogordon.github.io/freeclinker/)** — project overview, wiring and current feature/test status.

> This is a fork under active development. **[FreeCLinker](https://github.com/sheeprine/freeclinker)** is the original upstream project by sheeprine. Upstream describes the project as open source, but this fork does not assume a licence beyond what the upstream repository explicitly grants. Please check the upstream repository/licensing status before redistributing modified builds.

## What the FPSteVe Edition adds

The fork currently focuses on making camera telemetry useful as a state-aware FPV OSD rather than just forwarding fixed text. The configurator supports the newer Betaflight Custom Message workflow as well as the Pilot Name/Craft Name fallback used by older Betaflight releases.

- State-aware `ERR`, `RDY` and `REC` display with optional flashing REC state.
- Camera battery and remaining-record-time information.
- Camera warnings including low battery, low recording time and camera-hot status when telemetry is available.
- **Temporary Message** support, for messages such as `CLEAN LENS`, with display duration and an **Only before first arm** option.
- Adaptive Temporary Message/warning destination: Pilot Name or Craft Name on legacy Betaflight, or one of up to four Custom Message slots on newer Betaflight.
- Browser **Demo Mode** and OSD Preview for testing settings without arming a flight controller.
- Safe USB bench arm/AUX simulation in FPSteVe firmware builds that include the simulator commands.
- FPSteVe-specific browser configurator and flasher hosted through GitHub Pages.

### Betaflight OSD modes

**Betaflight 4.4 through 2025.12.x** uses `MSP2_SET_TEXT` with **Pilot Name** and **Craft Name**. The FPSteVe default is Pilot Name enabled with Status + Battery + Time Remaining; REC-only and REC flashing are enabled while recording. Craft Name is disabled by default. Temporary Message defaults to `CLEAN LENS`, enabled only before first arm, with a 1.0 second display duration. Camera warnings default to enabled with a 10% low-battery threshold and 5-minute low-recording-time threshold.

**Betaflight 2026.6+** uses up to four **Custom Messages**. The original four-line camera layout remains the default: battery; recording state/duration; mode/resolution/FPS/stabilisation; and remaining recording time/storage.

The browser UI keeps these modes separate so you only configure fields available on the Betaflight generation you're actually using.

## How it works

```text
Camera ←— BLE / camera protocol —→ ESP32 ←— MSP Serial —→ Betaflight FC
```

The ESP32 connects to a supported camera, receives available camera status/telemetry, polls Betaflight for arm state, starts recording when the FC arms and optionally stops recording after disarm. Camera/status information is then formatted for the selected Betaflight OSD method.

## ESP32-C3 Super Mini wiring

| ESP32-C3 | Flight controller |
|---|---|
| 5V | 5V BEC |
| GND | GND |
| GPIO4 TX | UART RX |
| GPIO5 RX | UART TX |

Configure that flight-controller UART for **MSP at 115200 baud**.

The project also supports the standard ESP32 target; see the source configuration and [Quickstart guide](QUICKSTART.md) for board-specific details.

## Supported camera families

The underlying **[FreeCLinker](https://github.com/sheeprine/freeclinker)** project contains support for DJI Action, GoPro, Caddx Orca, Sony Alpha, Blackmagic and Insta360 camera families. Protocol capabilities differ by camera, so not every camera can provide every OSD value.

For the FPSteVe Edition, distinguish **supported in code** from **physically tested**. Current FPSteVe hardware testing includes GoPro HERO11 Black Mini and GoPro MAX2. Other camera families should be treated as needing additional hardware validation unless explicitly marked tested on the project site.

## GoPro wake protection

The fork retains the GoPro wake-guard behaviour developed for mounted FPV cameras. For known advertisement formats, the ESP32 can avoid automatically connecting to a GoPro that is advertising while asleep, preventing an unwanted BLE connection from waking a camera the pilot deliberately left powered down. Manual camera selection can bypass this protection.

## Configuration and firmware

Runtime settings are persisted on the ESP32. The recommended interface for this fork is the **[FPSteVe Configurator](https://alfonsogordon.github.io/freeclinker/config.html)** rather than the upstream hosted configurator.

The USB serial CLI remains available for diagnostics and direct settings. Useful commands include `status`, `show`, `help`, `record start`, `record stop` and `reboot`. FPSteVe test firmware may additionally expose safe RAM-only simulator commands such as `sim arm 1`, `sim arm 0`, `sim aux high`, `sim aux low`, `sim status` and `sim off`.

## Building

The firmware uses PlatformIO. Typical local commands are:

```bash
pio run
pio run -e esp32c3supermini
pio run -e esp32dev
```

Published test builds are produced through this repository's GitHub Actions release workflow. The hosted flasher downloads release assets from **this fork**, not from upstream.

## Project status

The FPSteVe Edition is currently in active test/development. Browser UI, OSD Preview, Demo Mode, defaults and persistence are being validated before the next C3 test firmware is promoted for hardware testing. Do not interpret a supported-camera entry as confirmation that FPSteVe has physically tested that model.

## Upstream credit

This work is based on **[FreeCLinker](https://github.com/sheeprine/freeclinker)** by sheeprine. The upstream project established the core ESP32 camera/Betaflight bridge, camera protocol implementations and original web interface. FPSteVe Edition builds on that work with the features described above.

FPSteVe: [YouTube](https://www.youtube.com/@FPSteVe) · [Instagram](https://www.instagram.com/fpvsteve/)

### GoPro first connection

After flashing an ESP32-C3, **power-cycle the FreeCLinker board**. With the GoPro awake, FreeCLinker should discover and connect to it automatically; during current HERO11 Mini/MAX2 testing there is **no need to put the GoPro into its Pair/Connect Device screen**. Wake Guard is intended to ignore a sleeping GoPro rather than wake it during scanning.

While connected, FPSteVe Edition currently sends an experimental Open GoPro keep-alive every **10 seconds**. This interval is under real-hardware validation and may change before V1.0.
