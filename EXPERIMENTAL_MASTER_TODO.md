# FPV CamBuddy — Experimental Master TODO

> **Authoritative living tracker for the `experimental` branch.** Stable V1.0.1 on `main` is the protected baseline and must not be changed by experimental work unless Steve explicitly decides to release/merge later.
>
> **Rollback baseline for the current overnight work:** `f5dae81bb0d9ff85b73faa076c843b35ee7428d9`.
>
> Status convention: unchecked = still open. `NEEDS TEST` means implemented or code-reviewed but not physically/browser accepted. `BLOCKED` means intentionally waiting on missing hardware/evidence rather than guessing. Do not tick hardware/browser items merely because CI is green.

## Current overnight priority

- [ ] **IN PROGRESS — Finish Advanced Multi Cam OSD UI hardening.** Make the 16-character guard configuration-aware, count separators correctly, match firmware source/identifier behaviour, and avoid mobile reload/regression behaviour.
- [ ] **IN PROGRESS — Complete collapse UI pass.** OSD collapse arrow belongs on the main OSD controls rather than the preview; collapsing OSD must also hide the live preview and visually resemble OSD disabled while retaining the enabled highlight. Camera Mode Switch and other eligible settings cards need consistent collapse arrows.
- [ ] **NEEDS TEST — Sticky Live OSD Preview.** Desktop preview should remain aligned with the top of the OSD workspace while scrolling; mobile should stay normally stacked.
- [ ] **IN PROGRESS — Full configurator regression audit.** Review every visible setting and every master/child interaction, read/save/apply path, Demo Mode, firmware gating, Single Cam, Multi Cam, OSD, BLE power, camera registry/labels, Camera Matching, Camera Mode Switch, flasher/navigation, and injected UI-layer interactions.
- [ ] **IN PROGRESS — Documentation audit.** Cross-check repository docs and website/help copy against current experimental code and correct contradictions on `experimental` only.
- [ ] **TODO — Automatic Camera Detection test switch.** Only start implementation after the OSD/UI baseline is clean and both exact-HEAD workflows are green.

## OSD / UI

- [ ] **IN PROGRESS — 16-character capacity guard:** use the actual rendered spacing/separators rather than summing token bodies and trimming literal spaces away.
- [ ] **IN PROGRESS — Advanced Auto-source identifier sizing:** Auto battery/time and other Auto telemetry fields that are rendered from a specific source must reserve the configured C# or saved-tag suffix; aggregate Status must remain unsuffixed.
- [ ] **TODO — Keep the two Advanced OSD capacity implementations consistent.** `fps-multicam-osd.js` and `fps-osd-capacity-guard.js` must agree on candidate length, exact-duplicate blocking, identifier suffixes, and literals.
- [ ] **TODO — Preserve literal template text while adding an OSD token.** Adding an element must not silently rebuild the line from tokens and discard any literal text already present.
- [ ] **TODO — Verify aggregate Status maximum.** Firmware can render multi-camera state/count text; guard must use a safe length without unnecessarily blocking proven simple/default templates.
- [ ] **NEEDS TEST — Advanced OFF behaviour:** no C1/C2 or saved-tag suffixes on normal telemetry; lowest battery, least remaining time, hottest/critical warning source; normal one-camera display remains indistinguishable from Single Cam.
- [ ] **NEEDS TEST — Advanced ON source selector:** Auto/lowest where applicable plus specific persistent saved-camera number for each OSD element.
- [ ] **NEEDS TEST — Identifier modes:** C1/C2 and saved labels such as FRONT/REAR; tag mode falls back to C# when no saved label exists.
- [ ] **NEEDS TEST — Camera-specific Status:** selected saved camera can show `OFF-C#` or `OFF-TAG` when absent and must not silently substitute another camera.
- [ ] **NEEDS TEST — Duplicate semantics:** exact same element/source combination blocked; same element may be repeated when the source is semantically different.
- [ ] **NEEDS TEST — Unsupported telemetry messaging:** unavailable telemetry for a selected camera is disabled with a short useful reason.
- [ ] **NEEDS TEST — Separate simple/advanced templates:** turning Advanced OFF restores the simple templates; turning it back ON restores Advanced templates.
- [ ] **NEEDS TEST — Advanced feature gating:** Advanced Multi Cam OSD controls appear only when Experimental + Multi Cam + OSD master make them relevant; per-builder source tools remain hidden until Advanced is ON.
- [ ] **NEEDS TEST — Yellow experimental styling:** Advanced Multi Cam OSD must use the same yellow experimental visual language as other new experimental controls.
- [ ] **NEEDS TEST — OSD main collapse:** collapse control is attached to the main OSD control area, not the preview.
- [ ] **NEEDS TEST — OSD collapse visual state:** collapsing an enabled OSD should leave the master/highlight visible but hide Betaflight version, child settings, apply area and preview like the disabled presentation.
- [ ] **NEEDS TEST — Camera Mode Switch collapse arrow:** entire Camera Mode Switch card can be collapsed without breaking AUX/disarm-delay visibility logic.
- [ ] **NEEDS TEST — Other settings-card collapse arrows:** eligible settings cards/sections get consistent arrows without adding an arrow to the Live OSD Preview itself.
- [ ] **NEEDS TEST — Collapse persistence:** collapsed/open state survives reload through the dedicated localStorage key and does not conflict with feature master toggles.
- [ ] **NEEDS TEST — Live OSD Preview sticky desktop behaviour.** Keep preview pinned while the OSD settings column scrolls.
- [ ] **NEEDS TEST — Live OSD Preview mobile behaviour.** At phone width the preview returns to normal document flow and does not overlay controls.
- [ ] **NEEDS TEST — Phone stability:** changing Advanced Multi Cam OSD source/identifier/settings must not reload the page or lose current form state.
- [ ] **TODO — Real FC OSD acceptance:** confirm all current/legacy templates on actual Betaflight OSD, including the 16-character truncation boundary.

## Full configurator / UI regression

- [ ] **TODO — Connection/read path:** Connect → Read Settings populates all native and injected controls from the C3 without stale localStorage winning over board state.
- [ ] **TODO — Save/apply/read-back:** one Save/Apply path writes intended settings, verifies them, and survives a fresh Read Settings.
- [ ] **TODO — Experimental master:** OFF hides experimental-only controls; ON reveals only relevant experimental controls.
- [ ] **TODO — Firmware gating:** V1.0.2-only controls do not pretend to be supported by older/stable firmware.
- [ ] **TODO — Demo Mode:** remains clearly visually distinct, persists correctly across Settings/Preview navigation, and never writes demo changes to hardware.
- [ ] **TODO — OSD master:** OFF hides child controls/version/apply output; ON restores current/legacy method controls correctly.
- [ ] **TODO — BF method switch:** BF 2026.6+ Custom Messages 1–4 and legacy Pilot/Craft switch inside the same OSD card without stale/duplicate controls.
- [ ] **TODO — Destination controls:** Temporary Message and Camera Warnings only offer currently enabled OSD destinations and show a useful invalid-destination state.
- [ ] **TODO — Status Behaviour visibility:** appears only when an enabled active OSD destination actually contains Status.
- [ ] **TODO — REC-only / 1 Hz flash / CLEAN LENS / warnings:** defaults, persistence and UI gating remain intact.
- [ ] **TODO — Disarm Delay:** stays always accessible and is not accidentally swallowed by AUX child visibility.
- [ ] **TODO — Camera Mode Switch/AUX:** OFF/ON gating, AUX channel, GoPro Burst Slo-Mo handling and persistence remain intact.
- [ ] **TODO — Camera Matching:** Single Cam selector remains available where relevant and is disabled/explained during Multi Cam.
- [ ] **TODO — Wake Guard:** proven stable Single Cam behaviour remains intact; do not reintroduce abandoned Multi Cam wake/grace/shared-advert timer experiments.
- [ ] **TODO — Low Power:** normal Low Power control works when Advanced BLE power is inactive and is visibly/behaviourally locked only when the advanced profile owns TX power.
- [ ] **TODO — Wi-Fi/AP controls:** optional AP controls and BOOT fallback guidance remain intact.
- [ ] **TODO — Camera labels/registry UI:** persistent saved camera numbers and 7-character labels render and edit correctly without corrupting camera identity.
- [ ] **TODO — Multi Cam live badges/warning-source UI:** connected state, warning camera and count displays update without duplicate/ghost cards.
- [ ] **TODO — Navigation and flasher links:** experimental pages link to experimental resources and do not silently send testers to stable firmware/configurator.
- [ ] **TODO — Desktop layout audit:** no overlap, hidden controls, broken sticky positioning or duplicate injected sections at normal desktop width.
- [ ] **TODO — Mobile layout audit:** no forced reload, horizontal overflow, unusable controls or sticky overlays on the phone layout.

## Multi Cam

- [ ] **NEEDS TEST — Two simultaneous GoPros** on real C3 hardware with normal/full BLE power.
- [ ] **NEEDS TEST — Three or more simultaneous cameras.** Determine practical ESP32-C3 limit; 8 application slots are not a promise of 8 physical BLE links.
- [ ] **NEEDS TEST — Mixed camera-family combinations.** Validate the coordinator with real non-GoPro cameras.
- [ ] **NEEDS TEST — Long-duration stability** with multiple cameras connected.
- [ ] **NEEDS TEST — Join/reconnect while recording requested:** reconnecting camera receives START and converges correctly.
- [ ] **OPEN — STOP while camera is out of range at DISARM/delay.** Confirm the currently absent camera receives/reconciles STOP when it later returns. Do not call this fixed until physically validated.
- [ ] **NEEDS TEST — Partial recording acknowledgement:** OSD/count logic must represent confirmed vs unknown/partial state honestly.
- [ ] **NEEDS TEST — Connected/recording counts:** count transitions correctly when cameras join/drop/rejoin.
- [ ] **NEEDS TEST — Status LED count pulses:** 2-camera and 3-camera pulse patterns are distinguishable from scan/AP patterns.
- [ ] **TODO — Verify camera registry persistence is genuinely shared/compatible across mixed-brand Single Cam registration and Multi Cam coordinator use.** Current documentation assumes the workflow; source and hardware must confirm it before treating it as proven.

## Camera registration / Automatic Camera Detection / Camera Type retirement

- [ ] **CURRENT RULE — First-time registration remains one new camera at a time with a C3 power-cycle between new cameras.** Once learned, saved cameras may be powered together or one by one.
- [ ] **TODO — Source-audit saved-camera registry formats for every backend** before relying on Single Cam registration → Multi Cam mixed-brand reuse.
- [ ] **TODO — Add persisted experimental `Automatic Camera Detection` switch, default OFF.**
- [ ] **TODO — OFF path must be byte-for-behaviour equivalent to the proven manual Camera Type boot selection.**
- [ ] **TODO — ON + Multi Cam OFF:** auto-identify a supported camera family and control one camera without coordinating/waking multiple saved cameras.
- [ ] **TODO — Multi Cam ON:** keep the existing coordinator semantics unchanged unless a narrowly required refactor is proven safe.
- [ ] **TODO — Keep manual Camera Type as fallback during testing.** Do not remove `camera_type` storage/CLI yet.
- [ ] **TODO — Refactor GoPro-specific AUX/Burst Slo-Mo capability check** only if Auto Detection requires it; manual path must remain unchanged.
- [ ] **TODO — Caddx Wi-Fi UI with Auto Detection:** ensure Caddx-specific controls remain accessible only when appropriate.
- [ ] **NEEDS TEST — One GoPro with Auto Detection ON.** Steve can validate first because GoPros are available locally.
- [ ] **NEEDS TEST — DJI Action, Sony, Blackmagic, Insta360 and Caddx** with external testers before replacing Camera Type.
- [ ] **DEFERRED — Retire Camera Type UI/storage** only after Automatic Camera Detection is demonstrated reliable across supported families and an explicit cleanup decision is made.

## Camera-specific / protocol testing

- [ ] **NEEDS TEST — GoPro HERO11 Black Mini regression** on latest V1.0.2 binary.
- [ ] **NEEDS TEST — GoPro MAX2 regression** on latest V1.0.2 binary.
- [ ] **NEEDS TEST — DJI Action backend** discovery/control/telemetry.
- [ ] **NEEDS TEST — Sony backend** discovery/control/telemetry.
- [ ] **NEEDS TEST — Blackmagic backend** discovery/control/telemetry.
- [ ] **NEEDS TEST — Insta360 backend** discovery/control/telemetry.
- [ ] **NEEDS TEST — Caddx backend** discovery/control/telemetry and Wi-Fi-specific setup.
- [ ] **TODO — Protocol capability matrix:** document which telemetry fields each backend really supplies so Advanced OSD can disable unsupported fields accurately.

## BLE / power

- [ ] **NEEDS TEST — +9 dBm idle/disarmed baseline.**
- [ ] **NEEDS TEST — Arm boost at +9 dBm** with configured timing.
- [ ] **NEEDS TEST — Armed reliability at -6 dBm.**
- [ ] **NEEDS TEST — Armed reliability at -9 dBm.**
- [ ] **NEEDS TEST — Armed reliability at -12 dBm.**
- [ ] **NEEDS TEST — Disarm boost at +9 dBm** with configured timing.
- [ ] **NEEDS TEST — `power_multi_only` latch** activates only after >1 camera has been seen and stays latched until reboot.
- [ ] **NEEDS TEST — Normal Low Power restoration** when Advanced BLE profile is inactive.
- [ ] **NEEDS TEST — Realistic RF placement/reacquisition** with C3 and camera antennas in the actual quad installation.

## Firmware / state / reconnect behaviour

- [ ] **TODO — Preserve proven GoPro keepalive behaviour** on every experimental refactor.
- [ ] **TODO — Preserve delayed stop-on-disarm flow** and immediate OSD restoration on DISARM.
- [ ] **TODO — Preserve stable Wake Guard semantics** without resurrecting abandoned Multi Cam timer/grace/shared-advert experiments.
- [ ] **HARD CONSTRAINT — Do not mutate Multi Cam slot state from BLE advertisement callbacks.** Earlier experimentation broke Multi Cam.
- [ ] **HARD CONSTRAINT — Do not reintroduce old wake-guard timer/grace/shared-advert experiments.** Earlier versions caused hard locks.
- [ ] **NEEDS TEST — Reconnect state reconciliation** under disconnect/rejoin while stopped, armed/recording, and during delayed stop.
- [ ] **TODO — Check persistence/read-save-apply consistency** for every V1.0.2 firmware setting and any new Auto Detection setting.
- [ ] **TODO — Remove/clean obsolete experimental artifacts** only after confirming they are no longer referenced by build/site/firmware paths.

## OSD / telemetry hardware validation

- [ ] **NEEDS TEST — `REC 2/2`** on real FC/OSD.
- [ ] **NEEDS TEST — `PART 1/2`** on real FC/OSD.
- [ ] **NEEDS TEST — `{cams}` token** on real FC/OSD.
- [ ] **NEEDS TEST — camera-count transition on disconnect.**
- [ ] **NEEDS TEST — camera-count transition on reconnect.**
- [ ] **NEEDS TEST — lowest battery aggregation.**
- [ ] **NEEDS TEST — least recording-time aggregation.**
- [ ] **NEEDS TEST — hottest/critical warning source and saved label in warning.**
- [ ] **NEEDS TEST — Advanced pinned-source OFF display** for an absent saved camera.
- [ ] **NEEDS TEST — BF 4.5 Pilot/Craft path regression.**
- [ ] **NEEDS TEST — BF 2026.6+ Custom Messages 1–4 on a real FC.**

## Website / configurator / flasher

- [ ] **TODO — Experimental homepage feature list audit** against actual V1.0.2 features and test status.
- [ ] **TODO — Experimental configurator help/info copy audit** against current registration and Auto Detection plan.
- [ ] **TODO — Experimental flasher audit:** pinned to experimental artifacts, correct version labels, no stable artifact leakage.
- [ ] **TODO — Footer/donation note audit** on repository documentation and website main page copies.
- [ ] **TODO — Link audit:** experimental/stable configurator, flasher, upstream project, Discord and internal doc links.
- [ ] **TODO — Generated-site build validation** after every website/help-copy change.

## Documentation

- [ ] **IN PROGRESS — Audit `README.md`.** Ensure first-time Multi Cam instructions cover mixed-brand/manual Camera Type workflow accurately and do not overclaim unverified registry sharing.
- [ ] **IN PROGRESS — Audit `QUICKSTART.md`.** Current first-time GoPro instructions may conflict with the stricter power-cycle-between-new-cameras rule and need reconciliation.
- [ ] **IN PROGRESS — Audit `EXPERIMENTAL_V1.0.2.md`.** Auto Detection is currently documented as planned; verify all mixed-brand registration statements against source.
- [ ] **TODO — Audit `FPSteve_TODO.md`** and either keep it as historical status or point it at this master tracker to avoid two competing TODO sources.
- [ ] **TODO — Audit website homepage/configurator help panels** for the same setup wording and test caveats.
- [ ] **TODO — Document Advanced Multi Cam OSD** simple vs advanced behaviour, source selection, tags, OFF semantics and 16-character limit once accepted.
- [ ] **TODO — Document Automatic Camera Detection test mode** once implemented, including OFF fallback and supported-family validation status.
- [ ] **TODO — Keep stable-vs-experimental wording explicit** everywhere; V1.0.1 on `main` remains stable while V1.0.2 is under test.

## CI / build / release / branch cleanup

- [ ] **ONGOING — Exact-HEAD validation:** after each final change set, both `Validate FPV CamBuddy` and `Build Experimental V1.0.2 Firmware` must succeed for that exact experimental HEAD before calling it ready to test.
- [ ] **TODO — Verify generated firmware artifact publication/flasher path** after substantive firmware changes.
- [ ] **TODO — Keep every overnight change rollbackable** as incremental experimental commits from baseline `f5dae81b`.
- [ ] **DEFERRED — Release/merge decision:** do not merge experimental V1.0.2 into `main` until hardware validation is sufficient and Steve explicitly approves it.
- [ ] **TODO — Branch cleanup** after accepted testing: remove superseded experimental helpers/artifacts only after confirming no build/runtime dependency.

## Physical / browser / device test matrix

- [ ] Desktop browser — full configurator smoke test.
- [ ] Phone browser — full configurator smoke test, especially Advanced OSD controls and no reloads.
- [ ] ESP32-C3 Super Mini — flash latest exact-HEAD experimental artifact.
- [ ] Read Settings → change one harmless value → Apply → Read Settings round-trip.
- [ ] Single GoPro / Multi Cam OFF — connect/reconnect, ARM START, DISARM delayed STOP, OSD RDY/REC, keepalive, Wake Guard.
- [ ] Experimental ON / Multi Cam OFF — no Multi Cam-only children leaking into normal workflow.
- [ ] Multi Cam ON with two saved GoPros — simultaneous reconnect and control.
- [ ] Disconnect/reconnect second camera while recording requested.
- [ ] Disconnect second camera before/through DISARM delay, then reconnect while stopped requested.
- [ ] OSD default Multi Cam — no normal telemetry suffixes; aggregate values correct.
- [ ] OSD Advanced — Auto, C# IDs, saved tags, specific camera, absent camera OFF, duplicate rule, unsupported telemetry rule.
- [ ] OSD capacity guard — known valid combinations allowed; >16 rendered combinations blocked/explained; identifier mode changes recalculate immediately.
- [ ] Collapse controls — OSD, Camera Mode Switch, Record Control and other eligible sections.
- [ ] Sticky preview — desktop scroll and mobile stacking.
- [ ] Advanced BLE power — only after basic Multi Cam is proven.
- [ ] Auto Detection — only if implemented and exact-HEAD CI is green; first test with one GoPro before external camera-family testing.

## Deferred / known limitations / do-not-regress decisions

- [ ] **Known open:** STOP reconciliation for a camera absent during DISARM/delay still requires physical proof.
- [ ] **Known platform limit:** MSP custom text is capped at 16 characters in firmware; UI must work with that rather than hiding truncation.
- [ ] **Known resource limit:** 8 application slots do not guarantee 8 simultaneous BLE links.
- [ ] **Registration safety rule:** new cameras one at a time with C3 power-cycle between registrations until a replacement workflow is physically proven.
- [ ] **Do not use advertisement callbacks to mutate Multi Cam slot state.**
- [ ] **Do not restore abandoned wake/grace/shared-advert experiments.**
- [ ] **Do not remove manual Camera Type yet.** Auto Detection must be opt-in and validated first.

## Completed / accepted baseline

- [x] Stable V1.0.1 remains on `main`; current experimental development is isolated to `experimental`.
- [x] Rollback point explicitly recorded at `f5dae81bb0d9ff85b73faa076c843b35ee7428d9`; both required workflows passed on that baseline before the overnight work began.
- [x] Existing V1 hardware baseline: ESP32-C3 Super Mini, GoPro BLE discovery/connect/reconnect, ARM recording, delayed DISARM stop, GoPro keepalive, real BF 4.5 MSP, Pilot/Craft OSD, ERR/RDY/REC, REC-only/flashing REC, CLEAN LENS, Easy Config read/save/read-back, persistence and browser flashing.
- [x] Multi Cam coordinator/backends, bounded Multi-GoPro slots, aggregate telemetry/count foundations, advanced BLE power state-machine controls and LED count logic are implemented in source; remaining unchecked items above distinguish physical acceptance from compilation.

## Changelog

- 2026-09-15 — Created this master tracker from the active experimental repo/docs, the previous `FPSteve_TODO.md`, and the available C3 Cam Board project history. Future overnight work should update this file rather than creating a second competing backlog.
