# FPV CamBuddy — V1.0.2 Experimental Status

V1.0.2 is the active development build on the `experimental` branch. Stable V1.0.1 remains on `main`.

For the full feature description and test guidance, see [EXPERIMENTAL_V1.0.2.md](EXPERIMENTAL_V1.0.2.md).

## Implemented in V1.0.2

### Multi Cam coordinator

- [x] Multi-camera coordinator selectable by `multi_cam`
- [x] GoPro backend
- [x] DJI Action backend
- [x] Sony backend
- [x] Blackmagic backend
- [x] Insta360 backend
- [x] Caddx backend
- [x] Rotating BLE scan ownership between BLE camera families
- [x] Aggregate connected-camera count
- [x] Camera join/reconnect state reconciliation
- [x] START replay when a camera joins while recording is requested
- [x] STOP replay when a camera joins while stopped state is requested

### Multi-GoPro

- [x] Previous arbitrary six-camera application cap removed
- [x] Bounded to 8 application slots for deterministic embedded resource use
- [x] Per-camera recording-known state
- [x] Per-camera pending shutter state
- [x] Successful shutter acknowledgement used as local recording-state confirmation
- [x] Aggregate connected count
- [x] Aggregate confirmed recording count

**Important:** eight slots are not a promise of eight physical simultaneous BLE links. The practical connection limit depends on controller/host resources, stack configuration, memory, scanning and camera behaviour.

### Multi-camera OSD

- [x] `connected_cameras` telemetry
- [x] `recording_cameras` telemetry
- [x] confirmed recording-count flag
- [x] compact `REC x/y` state when all confirmed recording
- [x] compact `PART x/y` state for partial confirmed recording
- [x] `{cams}` token
- [x] connected-count fallback while recording counts are not fully known
- [x] lowest battery aggregation
- [x] lowest remaining-time aggregation
- [x] hottest-camera aggregation
- [x] combined media-ready aggregation where supported

### Advanced BLE TX power

- [x] Idle/disarmed power
- [x] Arm-boost power
- [x] Arm-boost duration
- [x] Armed power
- [x] Disarm-boost power
- [x] Disarm-boost duration
- [x] Automatic phase transitions
- [x] Supported configured levels from -12 to +9 dBm in ESP BLE steps
- [x] zero-duration boost bypass
- [x] `power_multi_only` option
- [x] multi-camera detection latch retained until reboot
- [x] restoration of normal Low Power behaviour when advanced profile is inactive

### Status LED

- [x] steady connected indication retained for one connected camera
- [x] multi-camera count pulses for more than one connected camera
- [x] N short pulses in a repeating three-second period
- [x] scanning/AP indications retained

### Experimental configurator

- [x] V1.0.2 controls grouped under **Experimental Multi Cam Settings**
- [x] Experimental master hides/shows the whole section
- [x] Multi Cam master hides/shows its child controls
- [x] BLE profile master gates the detailed power controls
- [x] read/save/verify path includes V1.0.2 settings
- [x] firmware-version gating for V1.0.2-only controls
- [x] experimental flasher pinned to experimental firmware path
- [x] duplicate experimental banner handling

## Stable behaviour intentionally preserved

- [x] normal single-camera mode remains the default
- [x] normal single-GoPro backend remains separate from Multi Cam
- [x] GoPro Burst Slo-Mo AUX path remains single-GoPro-only
- [x] V1.0.1 stable `main` branch left unchanged
- [x] standard delayed stop-on-disarm flow remains available
- [x] existing OSD warning/reminder behaviour retained

## Build / CI status

- [x] ESP32-C3 Super Mini build
- [x] ESP32 build
- [x] experimental web generation/validation
- [x] experimental firmware artifacts produced
- [x] experimental flasher path wired to experimental artifacts

A passing build proves source/build integration. It does **not** prove multi-camera RF reliability.

## Hardware validation still required

### Multi-camera

- [ ] two simultaneous GoPros on real hardware
- [ ] three or more simultaneous cameras
- [ ] determine practical maximum C3 camera count
- [ ] mixed camera-family combinations
- [ ] long-duration multi-camera stability
- [ ] one camera moving out of range and rejoining
- [ ] START reconciliation on reconnect while armed
- [ ] STOP reconciliation on reconnect after disarm/delay

### OSD

- [ ] `REC 2/2` on real FC/OSD
- [ ] `PART 1/2` on real FC/OSD
- [ ] `{cams}` token on real FC/OSD
- [ ] count/state transition when a camera disconnects
- [ ] count/state transition when a camera reconnects

### BLE power profile

- [ ] +9 dBm idle/disarmed baseline
- [ ] +9 dBm arm boost timing
- [ ] -6 dBm armed reliability
- [ ] -9 dBm armed reliability
- [ ] -12 dBm armed reliability
- [ ] +9 dBm disarm boost timing
- [ ] `power_multi_only` latch behaviour on real hardware
- [ ] RF/reconnect behaviour with realistic camera placement

### LED

- [ ] two-camera pulse count
- [ ] three-camera pulse count
- [ ] visual distinction from scanning/AP patterns

### Regression

- [ ] V1.0.2 binary with Multi Cam OFF: single-GoPro ARM/DISARM baseline
- [ ] delayed stop-on-disarm regression
- [ ] Wake Guard regression
- [ ] GoPro keepalive regression
- [ ] BF 4.5 Pilot/Craft OSD regression

## Recommended first hardware-test sequence

1. Flash the latest V1.0.2 experimental C3 build.
2. Read settings and confirm V1.0.2.
3. Leave Multi Cam OFF and prove one GoPro works normally.
4. Enable Experimental Features and Multi Cam.
5. Test two nearby cameras at full/normal BLE power.
6. Verify LED connected-camera count.
7. Verify aggregate OSD count/state.
8. Force a second-camera disconnect/reconnect while stopped.
9. Force a second-camera disconnect/reconnect while recording.
10. Only then enable Advanced BLE Power.
11. Start armed power at -6 or -9 dBm before trying -12 dBm.
12. Test arm/disarm boosts independently.

## Existing V1 hardware-confirmed baseline 🤘

The following stable/single-camera behaviour was physically confirmed during V1 development:

- ESP32-C3 Super Mini
- GoPro BLE discovery/connect/reconnect
- Wake Guard
- normal Low Power mode
- GoPro keepalive
- real Betaflight 4.5 MSP
- ARM starts recording
- DISARM restores normal OSD immediately
- configured delayed recording stop
- REC → RDY transition
- BF 4.5 Pilot/Craft OSD path
- ERR / RDY / REC
- REC-only + flashing REC
- CLEAN LENS first-arm reminder
- Easy Config read/save/read-back verification
- settings persistence
- browser flashing

## Documentation status

- [x] README updated for V1.0.2 experimental
- [x] Quick Start updated for V1.0.2 experimental
- [x] Dedicated V1.0.2 experimental feature guide added
- [x] Experimental website feature summary updated
- [x] Hardware-test caveats documented
- [x] Camera-count limitation wording corrected

## Release direction

V1.0.2 should remain experimental until the multi-camera and BLE-power hardware-test items above are sufficiently validated.

Stable V1.0.1 should remain untouched until an explicit release decision is made.
