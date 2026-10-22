# Stable main integration: v1.0.2 + current site

Status: **staging / no production changes** (2026-10-22)

## Goal
Make `main` the stable v1.0.2 firmware baseline without losing current production Pages, flasher, and compatible documentation. Keep all 1.0.3 and Nano work on separate branches.

## Confirmed branch comparison
`v1.0.2...main`: diverged; main 204 ahead and 153 behind; 36 files changed. Not a safe automatic merge.

## Keep from v1.0.2
- All firmware and library sources: `src/**`, `include/**`, `platformio.ini` unless build validation proves a narrowly scoped compatibility fix is necessary.
- Original stable firmware behavior and existing v1.0.2 release artifacts.

## Candidate changes to port selectively from main
- `web/**` production homepage/configurator/flasher only with firmware-version feature gating for 1.0.2.
- `tools/build_fps_site.py` as needed to build the current site.
- `.github/workflows/pages.yml` after removing hard dependencies on legacy branches and verifying deployed URLs.
- `README.md`, `QUICKSTART.md`, changelog and migration notes where accurate for stable firmware.

## Do not port into stable firmware without independent testing
- `src/ble_camera.cpp`, Nano DUML, `src/insta360_camera.cpp`, GoPro wake/reconnect code, `src/main.cpp`, `src/web_server.cpp`, `src/config_manager.cpp`, MSP changes.
- RC1 / RC2 firmware binaries or experimental firmware manifest links masquerading as v1.0.2.

## Validation gates before updating main
1. Review complete diff of selected website files and feature gates against v1.0.2 configuration protocol.
2. Generate the website and confirm public flasher URLs and historical release manifests still resolve.
3. Compile ESP32-C3 and ESP32 v1.0.2 firmware; compare firmware behavior and configuration compatibility.
4. Verify Pages deployment staging never replaces the stable homepage with a candidate branch.
5. Run hardware smoke test: USB/AP config read-write-readback, GoPro, DJI, Insta360 as supported by 1.0.2, FC arming/AUX and OSD.
6. Review PR and then perform a non-force merge into main using an explicit integration strategy. Do not force-reset main.

## Version policy
- `main`: stable v1.0.2 firmware + compatible production site.
- `v1.0.2`: immutable stable release reference.
- `v1.0.3`: development.
- `v1.0.3-rc1`: existing release candidate, preserved.
- `v1.0.3-rc2`: future tested candidate, not created/published until validated.

No firmware or website files have been merged into this branch yet.
