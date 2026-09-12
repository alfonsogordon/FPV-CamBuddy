# FreeCLinker — FPSteVe Edition working TODO

This is the live working checklist for requested FPSteVe Edition changes. Keep it updated as features are completed/tested.

## Current test milestone

- [x] Port tested FPSteVe camera-state/warning behaviour.
- [x] Replace CLEAN LENS-specific UI with generic Custom Message controls.
- [x] Make ERR / RDY / REC state-aware Craft Name behaviour foundational in the FPSteVe UI.
- [x] Add Camera Warnings master control with low-battery, low-record-time and CAM HOT behaviour.
- [x] Add dark/purple shared configurator theme.
- [x] Add OSD Preview entry to the configurator tab bar and match tab styling.
- [x] Add exact supplied FPV DVR image to OSD Preview, left-biased for left-aligned Craft Name text.
- [x] Add live C3 camera telemetry panel to OSD Preview.
- [x] Add RAM-only USB bench ARM/DISARM simulation through the same MSPSerial arm callback used by Betaflight.
- [x] Add RAM-only AUX high/low simulation through the normal AUX callback.
- [x] Compile the bench-simulator firmware successfully for ESP32-C3 Super Mini and ESP32 after the simulator/DVR changes.
- [ ] Publish the bench-simulator firmware as a new test prerelease (keep v0.1 as known-good).
- [ ] Merge the bench-simulator/OSD Preview branch to main and verify GitHub Pages deployment.
- [ ] Hardware bench test: C3 over USB + GoPro HERO11 Mini, no FC — ARM starts recording; DISARM honours configured stop delay.
- [ ] Hardware bench test the same flow with GoPro MAX2.
- [ ] Hardware test configured AUX behaviour from OSD Preview.
- [ ] Add/verify a simulation safety lease/heartbeat so an abandoned browser session cannot leave simulated state active; reboot must always clear simulation.

## OSD presentation

- [ ] Replace the `B:` prefix in FPSteVe BF4.5 state text with the real Betaflight main-battery OSD glyph (Betaflight 4.5 `SYM_MAIN_BATT`, character code `0x97`) followed by the camera percentage, e.g. battery-icon + `69` instead of `B:69`.
- [ ] Update the OSD Preview to visually represent that battery glyph rather than displaying `B:`.
- [ ] Verify the glyph survives MSP2_SET_TEXT / Craft Name end-to-end on Betaflight 4.5 analog OSD and does not get sanitised or rendered as an unexpected character.
- [ ] Preserve the 16-character Craft Name budget and warning alternation after the icon change.
- [ ] Bring simulator strict ERR/RDY/REC readiness rules fully in line with firmware for every capability/error state, including unknown recording state and media readiness.
- [ ] Make OSD Preview reflect real media-ready state when the firmware exposes it machine-readably.

## Web UI / site polish

- [ ] Make every public OSD Preview element use the same dark/purple visual language as Easy Config / Cameras / CLI; remove any remaining inconsistent/legacy styling.
- [ ] Convert remaining blue inline accents in the browser flasher to the FPSteVe purple theme.
- [ ] Audit all public web pages for references to the upstream/original project. When referring to the original project, display **FreeCLinker** in bold and link it to `https://github.com/sheeprine/freeclinker`; do not turn the FPSteVe Edition brand/title into that link.
- [ ] Add the new FPSteVe Edition features to the main landing page.
- [ ] Add a separate **Tested hardware** section so physically tested cameras/boards are not confused with protocol-level supported hardware.
- [ ] Initially list ESP32-C3 Super Mini + GoPro HERO11 Mini + GoPro MAX2 as physically tested once the current bench tests pass; add DJI models only after friends actually test them.
- [ ] Add a public camera test/report route for owners of other supported cameras, preferably a GitHub `Camera Test Report` issue template asking for camera model, camera firmware, ESP32 board, result and logs.
- [ ] Add the FPSteVe logo later, small and deliberate (header/footer rather than huge hero artwork), with a web-specific purple-accent variant while retaining the original artwork.

## Release / repository housekeeping

- [ ] Restore `.github/workflows/release.yml` so normal `main` UI pushes do not continuously rebuild/update the v0.1 test release; keep tag/manual release behaviour.
- [ ] Remove temporary one-shot development workflows/helpers after the bench simulator is merged and verified.
- [ ] Publish a clean v0.2-style FPSteVe test prerelease containing the verified bench-simulator firmware.
- [ ] Verify the hosted flasher selects/downloads that new release correctly for ESP32-C3 Super Mini.
- [ ] Verify Pages contains the matching Configurator + OSD Preview + flasher before calling a build flash-ready.
- [ ] Update README/Quickstart for FPSteVe Edition defaults, Custom Message, Camera Warnings, bench simulation and hosted FPSteVe URLs instead of leaving upstream-only instructions.
- [ ] Resolve/clarify the upstream repository licensing situation before treating the fork as broadly redistributable beyond GitHub's fork mechanism.

## Later / parked

- [ ] Revisit the possible visible name `FreekLinker`; do **not** rename it yet. If changed later, keep original FreeCLinker attribution extremely prominent.
