# FreeCLinker FPSteVe Edition - Agent Instructions

These instructions apply to autonomous coding agents working in this repository.

## Branch safety

- Work on `experimental` unless Steve explicitly asks for another branch.
- **Never modify, merge into, force-update, or release from `main` without Steve's explicit approval.**
- Stable V1.0.1 on `main` must remain untouched while V1.0.2 experimental work continues.
- Before every write, refetch the live `experimental` HEAD and the current target-file blob. Do not overwrite newer work.
- Keep commits small, descriptive, and independently revertible.

## Rollback boundary

The clean pre-overnight rollback boundary is:

`f5dae81bb0d9ff85b73faa076c843b35ee7428d9`

Do not reset to it unless Steve explicitly requests a full rollback. Use normal incremental commits/reverts for ordinary fixes.

## Current release goal

Finish and physically validate **Experimental V1.0.2** without regressing the proven V1.0.1 behaviour. The main active areas are Advanced Multi Cam OSD, configurator UX, board save/read-back, Multi Cam behaviour, and later automatic camera detection.

Read these before substantial work:

- `CODEX_GOAL.md` - current working goal, state, constraints and acceptance criteria.
- `EXPERIMENTAL_MASTER_TODO.md` - authoritative backlog/test/audit tracker.
- `OVERNIGHT_REPORT.md` - recent implementation/audit history and pending physical tests.
- `EXPERIMENTAL_V1.0.2.md` - experimental release documentation.

Do not replace or shorten the master TODO when updating it; preserve existing backlog and history.

## Required validation before saying a build is ready

After the final commit for a testable change, resolve the exact current `experimental` HEAD and check GitHub Actions for that exact SHA. Both workflows must be `completed` with `conclusion=success`:

1. `Validate FPSteVe Edition`
2. `Build Experimental V1.0.2 Firmware`

Do not say "ready to test" while either workflow is queued/in progress, and do not treat an older green run as validation of a newer HEAD.

CI/code inspection is not browser or hardware validation. Clearly mark anything requiring Steve's browser, C3, camera, FC or Betaflight test as **NEEDS TEST** until he confirms it.

## Proven behaviour that must not regress

- Multi Cam connects and saves cameras.
- Saved cameras reconnect.
- ARM/DISARM starts/stops normal connected cameras.
- Saved/off GoPros stay asleep on C3 reboot.
- The real 7-inch quad previously passed the core OSD/default fixes.
- First-time GoPro registration is currently safest one new GoPro at a time with a C3 power cycle between new registrations. Once learned, cameras may be powered together or individually.
- The experimental/test page firmware checker is browser-confirmed to detect a board flashed with main/V1.0.1. Preserve it.

Do not reintroduce old wake-guard timer/grace/shared-advertisement experiments; they caused hard locks.
Do not mutate Multi Cam slot state from the BLE advertisement callback; a previous STOP-reconnect experiment broke Multi Cam.
STOP while a camera is out of range at disarm remains an open item; do not casually mix that BLE/control work into OSD/UI changes.

## OSD rules

Firmware MSP custom text output is hard-limited to 16 rendered characters (`TEXT_LIMIT = 16` in `src/msp_serial.cpp`). Stored template syntax may be longer; `OSD_TPL_LEN` was expanded to 64 so Advanced source tokens are not truncated before rendering.

### Advanced OFF

- No C1/C2 suffixes on ordinary aggregate telemetry.
- Battery = lowest connected battery.
- Time remaining = least connected recording time.
- Temperature/warnings use the hottest/critical camera and may identify the saved camera in the warning.
- Status = RDY / REC, with connected count only when more than one camera is connected, e.g. `RDY (4)` / `REC (4)`.
- Preserve `(N)`; Steve has explicitly confirmed it fits his intended OSD layout.
- One connected camera should look like normal single-camera operation.

### Advanced ON

- Each OSD element may use Auto/aggregate or a persistent saved camera number.
- **Auto uses the plain aggregate token and must not add C#/tag suffixes.**
- Only a pinned source gets Advanced syntax such as `@1n` / `@1t` and an identifier.
- Identifier mode: C1/C2 or saved tag such as FRONT/REAR, with C# fallback when no tag exists.
- A pinned Status for an absent camera should show `OFF-C#` or `OFF-TAG`; never silently substitute another camera.
- Exact duplicate elements should be blocked, while semantically different source selections may repeat the same element.
- Unsupported telemetry for a selected camera should be unavailable with a short reason.
- Advanced state is separate from simple templates so turning Advanced OFF restores the simple setup.
- Do not add a second independent capacity validator. `web/fps-osd-capacity-guard.js` is the authoritative browser-side capacity guard.

The configurator preview must reflect actual selected sources and rendered values as closely as possible. Physical output remains authoritative when code assumptions conflict with Steve's real Betaflight/C3 test.

## Current UI direction

The OSD settings and Live OSD Preview are one two-column desktop workspace. They should **scroll together with the page**, not use a separately sticky/pinned preview. The Advanced Multi Cam simulator belongs inside the preview column and should use its full available width. Narrow/mobile layouts may stack normally.

Do not make broad UI changes when a small scoped change is enough. Avoid adding collapse arrows to individual setting rows; collapse controls are intended for main settings cards/major sections.

## Advanced OSD board path already present

The intended end-to-end path exists in code and should be preserved/audited rather than reinvented:

Configurator templates (`osd1..4`, `pilotTpl`, `craftTpl`) -> autosync `set ...` commands -> ConfigManager persistent storage -> `src/msp_serial.cpp` token parsing -> `MultiCameraCoordinator` per-camera `CameraData.sources[]` -> rendered MSP custom text.

`src/msp_serial.cpp` supports `base@source[n|t]` Advanced tokens. `MultiCameraCoordinator::publishState()` supplies aggregate and per-source data. Real-board Save/Read and FC OSD tests are still required for some edge cases.

## Camera Type / future automatic detection

Manual Camera Type is still functional and required in Single Cam mode today. `src/main.cpp` selects the active handler from `configManager.config().cameraType`; Caddx has Wi-Fi-specific configurator UI; GoPro has GoPro-specific AUX/Burst Slo-Mo behaviour.

Planned direction:

- Add **Automatic Camera Detection** as an experimental option.
- OFF must preserve the current manual Camera Type path exactly.
- ON should identify a supported family without requiring Multi Cam to be enabled.
- Multi Cam remains a separate feature.
- Keep manual Camera Type as fallback until DJI/Caddx/Sony/Blackmagic/Insta360 are validated.
- Do not simply route everything through the coordinator without first proving coordinator semantics and camera-specific capabilities.

Steve owns GoPros; other brands may be tested by external users. Do not claim those brands physically validated until results exist.

## Warning cycling future work

Firmware currently cycles warning categories (BATT LOW / REC LOW / CAM HOT). A desired future enhancement is to cycle multiple affected cameras within a warning category, e.g. `BATT LOW C1`, then `BATT LOW C2`, instead of concatenating cameras and breaking the 16-character budget. This is not implemented yet and should remain separate from unrelated UI fixes.

## Working style

- Prefer source inspection and root-cause fixes over speculative CSS/logic layering.
- Do not mix BLE/control changes into an OSD/UI-only task unless explicitly required.
- Preserve known-good behaviour and document why a change is necessary.
- When uncertain about hardware behaviour, implement only what source evidence supports and leave a clear NEEDS TEST item.
- Update `CODEX_GOAL.md` when the active goal/state materially changes so ChatGPT and Codex can hand work back and forth through the repository.
