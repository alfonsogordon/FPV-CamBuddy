# FreeCLinker — FPSteVe Edition roadmap

This is the single authoritative project roadmap. Work through the stages in order. New ideas are parked at the end unless they are required to complete or safely validate the current stage.

## Stage 1 — Repository cleanup

- [x] Replace branch-specific validation workflows with one production validation workflow.
- [x] Add a single production site build entry point (`tools/build_fps_site.py`).
- [x] Remove retired competing configurator UI/state helper files.
- [x] Keep `fps-ui-rebuild.js` as the production Settings UI owner.
- [x] Keep `fps-demo-v1.js` as the Demo Mode owner.
- [x] Restore the obvious purple Demo Mode page frame/marker.
- [x] Remove obsolete one-shot/branch-specific workflows.
- [x] Remove obsolete firmware/UI patch helper scripts that are no longer part of the production path.
- [x] Fix MSP warning declarations/routing mismatch exposed by validation.
- [x] Validate production site generation.
- [x] Build ESP32-C3 Super Mini firmware in CI.
- [x] Build normal ESP32 firmware in CI.
- [x] Consolidate duplicate TODO files into this roadmap.
- [ ] Final PR diff review.
- [ ] Merge cleanup PR.
- [ ] Verify GitHub Pages deployment from main.
- [ ] User regression check of Configurator and Demo Mode.

## Stage 2 — Settings / configurator overhaul

- [ ] Visually verify and finish the cleaned single-owner Settings UI.
- [ ] OSD Templates fresh default OFF; when OFF show heading + switch only.
- [ ] Show Betaflight version selector only when OSD Templates is enabled.
- [ ] Keep current and legacy Betaflight controls inside one Betaflight OSD card.
- [ ] Current BF defaults: `{batt}`, `{state} {recdur}`, `{mode} {res} {fps} {eis}`, `{rectf} {rcap}`.
- [ ] Legacy Pilot default: `{stateonly} {batt} {rectf}`; Craft default OFF.
- [ ] Add a proper switch for REC-only-when-armed-and-actually-recording behaviour.
- [ ] Keep Flash REC at 1 Hz as its own switch.
- [ ] Render `Only before first arm` as the standard enable/disable switch, not a text/value field.
- [ ] Temporary Message fresh default OFF; CLEAN LENS latent default; duration 1.0 s; repeat interval hidden at 3000 ms.
- [ ] Camera Warnings fresh default OFF; latent defaults 10% battery / 5 min recording time / automatic CAM HOT.
- [ ] AUX fresh default OFF; restore ARM/disarm guidance; AUX channel 0 means disabled.
- [ ] Optional feature OFF state must collapse to heading + switch only.
- [ ] Verify Demo Mode uses the same Settings state model and never writes hardware.
- [ ] Remove any remaining inconsistent/legacy Settings styling.
- [ ] User visual sign-off before Stage 3.

## Stage 3 — Firmware configuration plumbing

- [ ] Finish one global Temporary Message / Camera Warning engine.
- [ ] BF 2026.6+ routes to selected Custom Message 1–4.
- [ ] BF 4.4–2025.12 routes to selected Pilot/Craft destination.
- [ ] Auto-select/hide destination when only one valid destination exists.
- [ ] Persist destination in NVS.
- [ ] Expose destination through CLI/config read-back.
- [ ] Verify Apply -> read-back -> reboot persistence.
- [ ] Preserve known-good GoPro scan/wake/connect/ARM-record/disarm-stop behaviour.

## Stage 4 — Integrated OSD Preview

- [ ] Move OSD Preview beside OSD Settings using the same state model.
- [ ] Desktop side-by-side; narrow/mobile stacked.
- [ ] Live update for BF version, builders, messages, warnings and destinations.
- [ ] Demo ARM/DISARM shows RDY -> REC behaviour, REC-only mode and flashing.
- [ ] Simulate Temporary Message and warning priority/rotation.
- [ ] Use connected C3 camera telemetry where available.
- [ ] Prefer real Betaflight OSD glyphs where practical/licensed.
- [ ] Preview changes remain UI-only until Apply/Save.
- [ ] Preview must never arm an FC or motors.

## Stage 5 — Hardware validation

- [ ] Flash the spare ESP32-C3 Super Mini first.
- [ ] Read config and confirm UI reflects board state.
- [ ] Change every setting -> Apply -> read back exact values.
- [ ] Reboot -> verify NVS persistence.
- [ ] HERO11 Mini: sleeping scan behaviour, connect, ARM record, DISARM stop/delay.
- [ ] Repeat with GoPro MAX2.
- [ ] Test warnings and Temporary Message through actual Betaflight OSD.
- [ ] Test configured AUX behaviour.
- [ ] Verify simulation safety: RAM-only, no FC arming, reboot clears simulated state.

## Stage 6 — v1.0 release

- [ ] Clean README and Quickstart for FPSteVe Edition.
- [ ] Separate physically tested hardware from protocol-supported hardware.
- [ ] Verify hosted flasher selects the correct release/board image.
- [ ] Verify Configurator + Preview + Flasher match the firmware release.
- [ ] Publish final test release.
- [ ] Bench/flying test and fix release blockers.
- [ ] Release **FreeCLinker — FPSteVe Edition v1.0**.

## Stage 7 — Post-v1 features

- [ ] Multi-camera pairing/sync while retaining reliable single-camera fallback.
- [ ] Mixed camera families such as GoPro + DJI.
- [ ] Per-camera connection, recording and error state.
- [ ] Synchronized ARM recording and defined DISARM/crash behaviour.
- [ ] Optional low-power handling.
- [ ] Capability-aware camera settings: resolution, FPS, stabilisation, exposure/EV, ISO, shutter, white balance/colour, lens/FOV and other supported controls.
- [ ] Optional AUX mappings for selected camera settings without complicating normal ARM/record behaviour.

## Parked ideas — do not interrupt the roadmap

- [ ] Replace legacy `B:` camera-battery prefix with the Betaflight main-battery OSD glyph if end-to-end BF4.5 testing proves it survives MSP text transport correctly.
- [ ] Add a public camera test/report GitHub issue template for community hardware results.
- [ ] Add the FPSteVe logo later as a restrained header/footer treatment.
- [ ] Revisit possible visible name `FreekLinker`; do not rename before v1.0.
- [ ] Resolve/clarify upstream licensing before treating the fork as broadly redistributable beyond GitHub's fork mechanism.
