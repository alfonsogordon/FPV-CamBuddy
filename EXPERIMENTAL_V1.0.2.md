# FPV CamBuddy V1.0.2 Experimental

> **Experimental build. Bench test before flight.**
>
> V1.0.2 is the current development build on the `experimental` branch. Stable V1.0.1 remains on `main` and is not changed by the work described here.

This document explains the new V1.0.2 experimental features, what they are intended to do, how to configure them, and what still needs physical hardware testing.

## Experimental links

- Experimental configurator: https://alfonsogordon.github.io/freeclinker/experimental/config.html
- Experimental flasher: https://alfonsogordon.github.io/freeclinker/experimental/flash.html
- Experimental branch: https://github.com/alfonsogordon/freeclinker/tree/experimental

## What's new in V1.0.2

V1.0.2 adds four main areas of development:

1. **Multi Cam coordination** - one C3 can coordinate more than one supported camera.
2. **Multi-camera OSD state/counts** - the OSD can show how many cameras are connected and how many are confirmed recording.
3. **Advanced BLE TX power control** - different BLE power levels can be used while idle, during arm/disarm transitions, and while armed.
4. **Multi-camera status LED feedback** - the C3 LED can pulse the connected-camera count.

The existing single-camera behaviour remains available. Multi Cam is off by default.

---

## 1. Multi Cam coordinator

### What it does

When **Enable Multi Cam coordinator** is turned on, FPV CamBuddy uses the V1.0.2 coordinator rather than the normal single-camera backend.

The coordinator currently contains backends for:

- GoPro
- DJI Action
- Sony
- Blackmagic
- Insta360
- Caddx

It rotates BLE scanning between supported BLE camera families so that one family does not permanently own discovery.

### Current first-time camera setup

The current **Camera type** setting is still used by the normal Single Cam path to choose which camera backend runs at boot. Multi Cam itself uses the multi-brand coordinator, but a new camera may first need to be learned/saved using its correct Single Cam backend.

For a new camera that is not already known to the C3:

1. Leave **Multi Cam OFF**.
2. Select the correct **Camera type** for that camera.
3. Power/connect that camera and allow FPV CamBuddy to discover it and save it.
4. Power-cycle the C3 before registering another new camera.
5. Repeat with the correct Camera type for each additional camera.
6. Once the cameras have been learned/saved, enable **Multi Cam**. The coordinator can then work with those saved cameras without one manual Camera type selecting the active Multi Cam backend.

For example, for a new **GoPro + Insta360** setup: register the GoPro in Single Cam with Camera type set to GoPro, power-cycle, register the Insta360 in Single Cam with Camera type set to Insta360, then enable Multi Cam.

For multiple new GoPros, the same one-at-a-time rule applies: connect/save one new GoPro, power-cycle the C3, then register the next. Once each GoPro has been learned/saved once, they can be powered together and Multi Cam can reconnect to the saved cameras automatically on later boots.

A GoPro that has never been paired with FPV CamBuddy before may also need its **Pair** menu opened for that initial connection.

### Planned experimental Automatic Camera Detection

A separate **Automatic Camera Detection** switch is planned for V1.0.2 testing. It will be introduced as an experimental opt-in rather than immediately replacing Camera type.

The intended test behaviour is:

- **OFF:** preserve the current proven manual Camera type / Single Cam behaviour.
- **ON:** use the multi-brand camera coordinator/handler to identify and connect the appropriate supported camera family automatically, even when Multi Cam is disabled.
- **Multi Cam remains separate:** automatic detection decides what camera/backend is present; Multi Cam decides whether multiple cameras are coordinated at the same time.
- The manual Camera type option remains available as the fallback while GoPro, DJI Action, Sony, Blackmagic, Insta360 and Caddx behaviour is validated by hardware testers.

If Automatic Camera Detection proves reliable across the supported camera families, it can replace the old manual Camera type workflow in a later cleanup. Until that testing is complete, Camera type remains functional and should not be removed.

### Start and stop behaviour

ARM and DISARM still represent one requested recording state for the whole system:

- ARM requests **START recording** from connected cameras.
- DISARM requests **STOP recording** after the normal configured disarm delay.

A camera that connects or reconnects later is reconciled to the current requested state:

- if the quad currently wants recording, the joining camera is sent START;
- if the quad currently wants stopped recording, the joining camera is sent STOP.

This is important for a second camera that temporarily moves out of Bluetooth range. The system does not simply forget the camera when it drops out; when it returns, FPV CamBuddy attempts to bring it back into the state the quad currently expects.

### GoPro connection count

The V1.0.2 Multi-GoPro backend has **8 application slots**.

That does **not** guarantee eight simultaneous physical BLE camera connections. Actual usable connection count depends on ESP32 controller/host resources, Bluetooth stack configuration, memory, scanning activity and the cameras themselves. Eight is a practical bounded application limit rather than a promise of eight real-world links.

### Recording state confirmation

For the Multi-GoPro backend, successful shutter-command acknowledgements are used to mark the requested recording state as confirmed locally.

This is not the same as continuously parsing a fully independent GoPro recording-state telemetry stream. The distinction matters when testing partial failures: a successful command acknowledgement is treated as confirmation; an unacknowledged camera can remain unknown until its state is reconciled.

---

## 2. Multi-camera OSD

V1.0.2 adds camera-count telemetry to the normal OSD state system.

### Connected and recording counts

When multiple cameras are connected, the OSD can show combined state such as:

- `REC 2/2` - two cameras connected and both confirmed recording
- `PART 1/2` - two cameras connected but only one confirmed recording
- normal `RDY`, `REC` or other state when only one camera is in use

The exact rendered text is constrained by Betaflight/MSP message length, so the firmware keeps the multi-camera state compact.

### `{cams}` token

V1.0.2 also adds a `{cams}` OSD token.

When recording counts are known it represents:

`recording/connected`

For example:

`2/2`

If a complete confirmed recording count is not yet available, it falls back to the connected-camera count rather than presenting an incorrect partial failure.

### Aggregate telemetry

For multiple connected cameras the coordinator combines available telemetry conservatively:

- battery: lowest reported camera battery
- remaining recording time: lowest reported remaining time
- temperature: hottest reported camera state
- media readiness: combined readiness where supported

Not every camera protocol reports every field, so available OSD information can differ between camera families and models.

---

## 3. Advanced BLE TX power profile

The V1.0.2 experimental configurator adds an optional BLE power state machine.

Supported configured TX-power steps are:

- -12 dBm
- -9 dBm
- -6 dBm
- -3 dBm
- 0 dBm
- +3 dBm
- +6 dBm
- +9 dBm

### Power stages

The profile can use four operating stages:

**Idle / disarmed power**
: Normal power while the quad is disarmed and no transition boost is active.

**Arm boost**
: Temporary power immediately after ARM. Intended to give connected cameras a strong command/reconciliation window before dropping to the normal in-flight power level.

**Armed power**
: BLE power used during normal armed flight.

**Disarm boost**
: Temporary power immediately after DISARM. Intended to help nearby cameras receive/reconcile the stop state before returning to idle power.

After the configured boost duration expires, the state automatically moves to the next stage.

### Zero-duration boost behaviour

If an arm-boost or disarm-boost duration is set to `0 ms`, that boost phase is skipped.

This allows the experimental profile to behave like the earlier simple two-state setup:

- one power while armed;
- another power while disarmed.

### Only after multiple cameras detected

The option **Only after multiple cameras detected** prevents the advanced power profile from overriding normal Low Power behaviour until the coordinator has observed more than one connected camera.

Once that condition has been seen, the profile is **latched for the rest of that boot**. If one camera later disconnects, the latch remains active until the C3 is rebooted.

This is deliberate so BLE power behaviour does not keep changing back and forth as a second camera briefly drops in and out of range.

### Suggested first bench-test values

A conservative first test is:

- Idle/disarmed: **+9 dBm**
- Arm boost: **+9 dBm for 1500-3000 ms**
- Armed: start at **-6 or -9 dBm** before trying -12 dBm
- Disarm boost: **+9 dBm for 1500-3000 ms**

The best armed value depends on camera placement, distance and RF environment. Do not assume -12 dBm is reliable for every multi-camera installation.

---

## 4. Status LED camera count

With Multi Cam enabled and more than one camera connected, the normal C3 status LED changes from the single-camera steady-connected indication to a repeating count pattern.

The firmware produces **N short pulses in a three-second period**, where N is the connected-camera count, up to the practical count limit used by the implementation.

Examples:

- 1 connected camera: normal steady connected indication
- 2 connected cameras: 2 short pulses
- 3 connected cameras: 3 short pulses

AP and scanning indications remain separate from this connected-camera count behaviour.

---

## 5. Experimental configurator behaviour

V1.0.2 settings are grouped under **Experimental Multi Cam Settings**.

The UI is intentionally layered:

- Experimental master OFF: the whole V1.0.2 Multi Cam section is hidden.
- Experimental master ON: the Multi Cam section is available.
- Multi Cam coordinator OFF: advanced Multi Cam controls remain hidden.
- Multi Cam coordinator ON: the BLE power-profile option becomes available.
- BLE power profile ON: individual power/timing controls become available.

The board remains the source of truth. **Read Settings** reads the actual C3 values, and save/apply writes the V1.0.2 settings back to the board and verifies them.

The configurator checks the connected firmware version. V1.0.2-only controls should not be treated as available on an older stable board.

---

## 6. Settings and CLI names

The normal browser configurator is recommended, but these are the V1.0.2 setting names used by the firmware:

| Purpose | CLI command |
|---|---|
| Multi Cam coordinator | `set multi_cam <0|1>` |
| Advanced BLE profile | `set dyn_power <0|1>` |
| Idle/disarmed power | `set dis_dbm <-12..9>` |
| Arm boost power | `set arm_boost_dbm <-12..9>` |
| Arm boost duration | `set arm_boost_ms <0..60000>` |
| Armed power | `set arm_dbm <-12..9>` |
| Disarm boost power | `set dis_boost_dbm <-12..9>` |
| Disarm boost duration | `set dis_boost_ms <0..60000>` |
| Wait for multiple cameras | `set power_multi_only <0|1>` |

`set idle_dbm` is also accepted as an alias for the disarmed/idle power setting.

---

## 7. What remains unchanged

When Multi Cam is disabled, the normal single-camera backend is still used.

In particular, the proven single-GoPro path remains separate from the experimental coordinator, and GoPro Burst Slo-Mo AUX behaviour remains tied to the single-GoPro path rather than Multi Cam mode.

Stable V1.0.1 on `main` is not changed by V1.0.2 experimental development.

---

## 8. Hardware-test status

The V1.0.2 source builds successfully for both supported firmware targets and the experimental flasher receives those build artifacts.

The following still require real hardware validation and should be treated as experimental:

- simultaneous 2+ and especially 3+ BLE camera operation
- practical maximum simultaneous camera count on an ESP32-C3
- mixed camera-family operation in real installations
- **Automatic Camera Detection in Single Cam mode across supported camera families (planned experimental switch)**
- low-power armed operation at -12/-9/-6 dBm with cameras in realistic positions
- reconnect/rejoin behaviour when a second camera moves out of range
- delayed STOP reconciliation after reconnect
- arm/disarm boost timing on real hardware
- partial shutter-command acknowledgement behaviour
- multi-camera OSD `REC x/y` / `PART x/y` behaviour on a real FC/OSD
- camera-count LED pulses in actual use
- regression bench test of the stable single-GoPro workflow on the V1.0.2 binary

Do not interpret a successful compile as proof of RF reliability. Bench-test with props removed before any flight test.

---

## 9. Recommended V1.0.2 bench-test sequence

1. Flash the **experimental** firmware, not stable V1.0.1.
2. Open the **experimental configurator** and connect to the C3.
3. Read settings and confirm the board reports V1.0.2.
4. Leave Multi Cam OFF and prove the normal single-GoPro ARM/DISARM workflow first.
5. Enable Experimental Features.
6. Before enabling Multi Cam, register any completely new cameras one at a time in Single Cam mode using the correct Camera type. Power-cycle the C3 between first-time camera registrations.
7. Enable Multi Cam after the cameras have been learned/saved. Previously learned cameras can then be powered together for normal coordinator reconnect.
8. Test two nearby cameras before adding more.
9. Confirm the status LED count matches the number of connected cameras.
10. Confirm the OSD count/state changes when one camera disconnects or fails to confirm recording.
11. Test a camera leaving and re-entering Bluetooth range while stopped and while recording.
12. Only after basic Multi Cam behaviour is reliable, enable the advanced BLE power profile.
13. Start with higher BLE power and reduce armed power gradually while checking command reliability.
14. Test ARM boost, armed power, DISARM boost and delayed stop separately.

---

## Feedback

When reporting a V1.0.2 result, include:

- C3/ESP32 board type
- camera make/model and number of cameras
- whether the test was same-family or mixed-family
- Betaflight version
- configured BLE power values and boost timings
- approximate camera-to-C3 distance/placement
- OSD result
- LED pulse count
- whether reconnect/start/stop reconciliation worked

For help, join the **Squadding Quads Discord** and ask for **FPSteVe**:
https://discord.gg/eE6DkgEnjU
