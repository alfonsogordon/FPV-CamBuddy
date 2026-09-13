# FreeCLinker Development Notes

> **V1 note:** This file contains historical/internal development context and is not the end-user setup guide. For current FPSteVe Edition V1 behaviour use [README.md](README.md), [QUICKSTART.md](QUICKSTART.md), the web configurator and `FPSteve_TODO.md` (now the V1 release-status sheet). Historical details below may describe intermediate development states.

FreeCLinker is an ESP32-based action-camera control bridge for FPV use. The FPSteVe Edition fork adds a simplified USB configurator, GoPro-focused automatic recording workflow, Betaflight OSD telemetry, state-aware OSD behaviour, warnings, Preview tooling and browser flashing.

## Current V1 architecture summary

The signed-off FPSteVe Edition path is intentionally simple:

**Camera protocol → one CameraData state → MSP/OSD renderer → Betaflight OSD**

Configuration follows:

**C3/NVS → FPSteVe Easy Config read → one UI state → SAVE/APPLY full snapshot → read-back verification → C3/NVS**

The browser is not the authority for connected settings; the C3 is.

### Hardware target

Primary V1 target: ESP32-C3 Super Mini.

- GPIO4: MSP TX → FC UART RX
- GPIO5: MSP RX ← FC UART TX
- Common GND
- 5V supply
- MSP UART: 115200 baud

### V1 OSD paths

- Betaflight 4.5: Pilot Name / Craft Name compatibility path — hardware tested.
- Betaflight 2026.6+: Custom Messages 1–4 — implemented and Preview/logic validated, but not physically FC-tested for V1 because the acceptance FC runs BF 4.5.

### Signed-off OSD priority

**Warning → Temporary Message → REC-only → normal configured OSD**

REC-only is per message. It applies only to enabled templates containing `{state}` or `{stateonly}`, and only while the FC is armed and the camera is actually recording. Disarming restores the normal configured message immediately even though the camera may continue recording through the configured delayed stop.

### First-arm reminder

Fresh default: `CLEAN LENS`, enabled, one second, before first arm after C3 boot. Preview RESET ARM STATE is only a simulation convenience; real hardware first-arm state resets on C3 reboot/power cycle.

### Camera warnings

Fresh defaults: BATT LOW at 10%, REC LOW at five minutes, CAM HOT automatically when reported. Warnings have highest OSD priority.

### GoPro lifecycle

Wake Guard avoids deliberately waking a sleeping GoPro merely through scanning. When the camera is awake/available, FreeCLinker automatically discovers/connects and later reconnects. While connected, the GoPro keepalive is sent periodically. Hardware testing confirmed normal GoPro auto-power-off behaviour resumes when FreeCLinker is removed/unpowered.

### Release status

Core V1 camera + BF4.5 + OSD behaviour has passed real-board testing. Current work is documentation, repository cleanup and final release validation. Do not casually alter the signed-off camera/OSD behaviour during release preparation.

For current details see [FPSteve_TODO.md](FPSteve_TODO.md).
