# FPV CamBuddy v1.0.3 RC2 integration

**Status: integration branch only; not a release, not hardware validated.**

## Baseline
Branched from `dev-dji-nano-aux-range`, which is 33 commits ahead of `v1.0.3-rc1` and contains its history. This preserves RC1 GPS time/reconnect work while incorporating the latest Nano BLE, DUML session, status, no-wake/AUX development and web configuration work already in that branch.

## Work still to integrate
- Review `test/rc1-gopro-startup-diagnostics` changes against this branch; it includes reconnect-stability work plus additional startup diagnostics. Port only compatible, verified GoPro changes; resolve any overlap with Nano BLE code.
- Check diagnostics defaults, especially Nano no-wake test diagnostics, and avoid exposing test-only verbosity to stable users.
- Ensure web flasher selects binaries built from the exact RC2 source commit; do not reuse stale test artifacts.
- Confirm GPS time sync defaults and camera-specific feature gating.

## Validation before an RC2 release
1. Run website generation and PlatformIO builds for `esp32c3supermini` and `esp32dev`.
2. Verify USB read/save/readback and AP config across reconnect/power cycle.
3. Test Nano BLE pairing, recording start/stop, AUX ranges, status and no-wake behaviour.
4. Test GoPro wake guard, recording, camera power cycle/reconnect and GPS time.
5. Test Insta360 and other supported cameras for regressions.
6. Confirm MSP arm/disarm and OSD on supported Betaflight versions.
7. Publish a separate, explicitly labelled RC2 test flasher only after firmware artifacts match the tested source.

## Protected branches
Do not modify `main`, `v1.0.2`, `v1.0.3-rc1` or the public production flasher during integration. Preserve `test/rc1-gopro-startup-diagnostics` as the GoPro reference until changes are ported and verified.
