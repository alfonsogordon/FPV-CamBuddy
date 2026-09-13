# FreeCLinker — FPSteVe Edition

**Automatic action-camera control and Betaflight OSD telemetry from an ESP32-C3 Super Mini.**

FreeCLinker connects a supported action camera to your flight controller without adding another control to your pre-flight routine. Power the quad, let the C3 find the camera, arm and fly. Recording can start automatically on arm and stop after a configurable delay on disarm, while camera status is shown directly in the Betaflight OSD.

> FPSteVe Edition builds on the original FreeCLinker project with a simplified configurator, GoPro-focused flight behaviour, richer OSD, warnings, automatic recording and extensive bench/flight testing.

## Start here

- **Configurator:** https://alfonsogordon.github.io/freeclinker/config.html
- **Web flasher:** https://alfonsogordon.github.io/freeclinker/flash.html
- **Quick start:** [QUICKSTART.md](QUICKSTART.md)
- **Project website:** https://alfonsogordon.github.io/freeclinker/

## What it does

### Camera control
- Automatic camera discovery and reconnect
- GoPro BLE control with wake/sleep-aware connection behaviour
- Start recording automatically when Betaflight arms
- Configurable delayed stop after disarm — **5 seconds by default**
- Optional AUX camera-mode control
- GoPro Burst Slo-Mo support through AUX while recording
- GoPro BLE keepalive while connected
- Camera matching for multi-camera setups
- Low-power radio mode to reduce RF output near the flight-control/RC system

### Betaflight OSD
- Camera state: **ERR / RDY / REC**
- Camera battery percentage
- Recording duration
- Remaining recording time/capacity where reported by the camera
- Camera mode, resolution, frame rate and stabilisation telemetry where available
- Configurable OSD templates/tokens
- **REC-only while armed + recording** for a clean flight display
- Optional **1 Hz flashing REC**
- First-arm temporary reminder — default **CLEAN LENS**
- Camera warnings: **BATT LOW / REC LOW / CAM HOT**
- Warning → temporary message → REC-only → normal OSD priority handling

### Betaflight compatibility
- **Betaflight 4.5:** Pilot Name / Craft Name compatibility mode
- **Betaflight 2026.6+:** Custom Messages 1–4 support
- Automatic arm-state polling over MSP
- Configurable UART/AUX integration

### FPSteVe Easy Config
- Browser-based configuration over USB
- Automatic settings read when connected
- One **SAVE / APPLY SETTINGS** action for the complete configuration
- Read-back verification after saving
- Settings stored on the C3 and retained across power cycles
- Integrated live OSD Preview
- Preview controls for ARM, recording, camera errors/hot state and first-arm reset
- Fresh-board defaults designed to be useful without a long setup session

### Flashing & board behaviour
- Browser-based ESP32-C3 flashing
- Clear post-flash power-cycle/configuration flow
- Wi-Fi AP configuration remains available as an optional/fallback feature
- BOOT-button force-AP recovery
- Status LED for scanning/connection/AP state

## Default OSD setup

FPSteVe Edition ships with sensible flight defaults. On **BF 4.5**, Pilot Name is enabled by default with:

`{stateonly} {batt} {rectf}`

On **BF 2026.6+**, the four Custom Message defaults are:

| Message | Default template |
|---|---|
| 1 | `{batt}` |
| 2 | `{state} {recdur}` |
| 3 | `{mode} {res} {fps} {eis}` |
| 4 | `{rectf} {rcap}` |

REC-only, flashing REC, CLEAN LENS and camera warnings are enabled by default on a fresh FPSteVe Edition configuration.

## Tested for V1 🤘

The following have been physically confirmed on the FPSteVe Edition hardware-test setup:

- ESP32-C3 Super Mini + GoPro BLE connection and automatic reconnect
- GoPro control from a real Betaflight 4.5 flight controller
- ARM → automatic recording
- DISARM → immediate normal OSD restoration → 5-second delayed recording stop → RDY
- BF 4.5 Pilot/Craft OSD output and live **ERR / RDY / REC** state
- REC-only while armed + recording, including flashing REC
- First-arm **CLEAN LENS** behaviour
- Camera warning behaviour and priority logic in the Preview/firmware test flow
- Configurator read, save, read-back verification and settings persistence after reconnect/power cycle
- GoPro keepalive: camera stays connected while FreeCLinker is powered and returns to normal camera auto-power-off behaviour when FreeCLinker is removed
- Web flasher and post-flash configuration flow

## Implemented, but not yet hardware-tested

**Betaflight 2026.6+ Custom Messages 1–4** are implemented and exercised through the FPSteVe OSD Preview/firmware logic, but have **not yet been physically tested against a flight controller running that Betaflight generation**. The V1 hardware available for testing currently runs Betaflight 4.5, so this distinction is intentional.

Some telemetry fields also depend on what a particular camera/model reports over its protocol.

## Quick hardware connection

Default ESP32-C3 Super Mini ↔ flight-controller UART wiring:

| C3 | Flight controller |
|---|---|
| GPIO4 TX | UART RX |
| GPIO5 RX | UART TX |
| GND | GND |
| 5V | suitable 5V supply |

MSP UART speed: **115200 baud**.

See the [Quick Start Guide](QUICKSTART.md) before powering the installation.

## Help & feedback

A dedicated **Squadding Quads Discord** help/feedback thread is planned for FPSteVe Edition. The direct thread link will be added here and to the project website as soon as it is available.

Bug reports and useful real-world compatibility results are especially welcome — please include the camera model, Betaflight version and C3 board where possible.

## Credits

FPSteVe Edition is based on **FreeCLinker** and retains the work and supported-camera foundations of the upstream project. Thanks to the original FreeCLinker contributors and the camera/protocol projects that make this possible.

---

**FPSteVe Edition V1.0 — Actually Final** 🤘
