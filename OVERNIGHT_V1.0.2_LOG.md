# FPV CamBuddy V1.0.2 Overnight Development Log

Started: 2026-09-16 23:46 BST
Branch: `v1.0.2`

## Safety / scope

- Work only on `v1.0.2` for this overnight task.
- Do **not** modify `experimental`; it remains dedicated to Advanced Multi Cam OSD testing.
- Do **not** merge V1.0.2 into `main` during this task.
- Preserve the known-good stable firmware behaviour unless a V1.0.2 change explicitly requires otherwise.
- Every run should inspect the current branch state and latest CI before making further changes.
- Every run should append what was inspected, changed, committed, tested/verified, and any unresolved risks to this file so work can be audited or reversed.

## Work order

1. **Finish the field AP configurator first.**
   - Treat the ESP32-C3 SoftAP as the real in-field phone configurator, not a debug portal.
   - Use a clean/native ESP32 AP implementation rather than restart/watchdog/captive-portal workarounds.
   - BOOT-forced AP must remain available until reboot.
   - No internet or laptop should be required.
   - Make the onboard phone UI use the same user-facing settings, labels, conditional behaviour, OSD element picker/chips, and persistence semantics as the main web configurator where applicable.
   - Omit WebSerial/browser-only controls and the live OSD preview from the onboard UI.
   - Remove visible raw OSD template inputs and internal Status text fields that are not part of the main configurator.
   - Investigate and fix the reported intermittent AP disappearance before declaring it test-ready.

2. **DJI Osmo Nano support research + implementation where evidence supports it.**
   - Research the actual Nano/`Atto` BLE protocol and existing open-source implementations.
   - Reuse/extend the existing DJI backend only where protocol compatibility is proven.
   - Do not claim support based only on device-name matching.
   - Add connection, recording control and telemetry/status only where genuinely supported.

3. **DJI Action 2 support research + implementation where evidence supports it.**
   - Research Action 2 / DJI Mimo BLE behaviour and available protocol evidence.
   - Steve owns an Action 2 and can hardware-test experimental candidates.
   - Do not fake support if protocol details are incomplete; log what needs hardware validation.

4. **Extend existing AUX Camera Controls with camera profile/preset selection.**
   - Keep this within the current AUX control system.
   - Preserve existing AUX recording/control behaviour.
   - Support sensible 2-position / 3-position mappings.
   - Implement per camera driver only where the protocol genuinely exposes profile/preset/mode selection.
   - Optional transient OSD `PROFILE 1`, `PROFILE 2`, `PROFILE 3`, reusing existing temporary-message/warning infrastructure with configurable duration/destination.

## Initial checkpoint

Before this log was created, the current V1.0.2 AP implementation had been reviewed after real-phone testing showed intermittent association/disconnection and the SSID disappearing. A clean AP lifecycle correction was committed as `5d987423e865fb3ffebc646da6bba36c2e595b71`. The next work must verify that implementation and CI rather than assuming it is correct, then finish the onboard UI parity work before asking Steve to flash another candidate.

`experimental` must remain untouched throughout this task.

---

## Run log

### 2026-09-16 23:46 BST — task initialized

- Confirmed target branch `v1.0.2` exists.
- Created this persistent development/audit log on `v1.0.2`.
- Recorded the AP-first priority, Osmo Nano research/support, Action 2 research/support, and AUX camera-profile work.
- Recorded that Steve owns a DJI Action 2 for later hardware validation.
- Next run: inspect exact V1.0.2 HEAD + latest CI, validate the AP lifecycle commit, then continue the AP/configurator correction before camera-protocol work.


## 2026-09-18 — DJI Action profile support
- GoPro AUX profile switching is hardware-confirmed working, including persistence.
- Added conservative DJI Action 4 / 5 Pro / 6 profile support through the already-supported R-SDK camera mode command only: Video, Slow Motion, Timelapse, Hyperlapse, Photo.
- No guessed native DJI preset/DUML writes were added. Arbitrary saved DJI presets remain unsupported until a verified command exists.
- DJI profile discovery now feeds the same LOW/MIDDLE/HIGH configurator dropdown path as GoPro.
- OSD profile-change notification is the next requested feature after this build is validated.
