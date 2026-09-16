# Shared ChatGPT / Codex Working Goal

Last updated: 2026-09-15

This file is the short handoff between Steve's ChatGPT C3 Cam Board project and Codex. Read `AGENTS.md` first for hard safety and validation rules, then use this file as the current objective.

## Goal

Bring **FPV CamBuddy Experimental V1.0.2** to a clean, testable release candidate while preserving V1.0.1 behaviour and keeping `main` untouched until Steve explicitly approves a release/merge.

## Current exact state

At the time this file was created, `experimental` contained the OSD scroll-layout commit:

`f02629b0769c4f354dfb176032beea5b0aa333b6` - `Scroll OSD settings and preview together`

`AGENTS.md` was then added as the next commit. Always refetch live `experimental` before doing work; this SHA is historical context, not permission to reset the branch.

### Latest browser findings

Steve has browser-confirmed that the Advanced Multi Cam OSD UI now opens and the simulator finally uses the available preview width correctly.

The latest requested UI behaviour is:

- OSD settings are the left column.
- Live OSD Preview / simulator is the right column.
- **Both columns must scroll together with the normal page.**
- The preview must not remain sticky/pinned while the left settings move independently.
- This was addressed by commit `f02629b0`; it still requires browser validation after exact-head CI/deployment.

Do not undo the full-width Advanced simulator fix while changing scroll behaviour.

## Immediate acceptance tests

### Browser - OSD workspace

- [ ] Exact current HEAD passes both required GitHub Actions workflows.
- [ ] OSD settings and Live OSD Preview visibly scroll together at the same page rate on desktop.
- [x] Advanced Multi Cam OSD can be enabled without freezing the page.
- [x] Advanced Multi Cam simulator uses the full available preview-control width rather than one half-grid cell.
- [ ] Camera cards remain readable with four simulated cameras.
- [ ] Auto source has no C#/tag suffix.
- [ ] Pinned C1/C2 source changes preview and character count appropriately.
- [ ] Switching C# vs saved-tag identifier updates preview and capacity.
- [ ] Pinned Status for a disconnected simulated camera displays OFF with the selected identifier.
- [ ] Four-camera aggregate Status preserves `RDY (4)` / `REC (4)` where applicable.

### Real C3 / board read-write

- [ ] Save an Advanced template whose encoded syntax is >31 bytes but rendered output is <=16 characters.
- [ ] Read Settings back and confirm the template was not truncated (`OSD_TPL_LEN` is now 64).
- [ ] Reload/reconnect and Read Settings; Advanced mode and identifier mode restore from board token syntax.
- [ ] Verify aggregate and pinned-source values on actual Betaflight custom-message OSD.
- [ ] Verify absent pinned source displays OFF/B:--/T:-- semantics as intended.
- [ ] Verify the 16-character rendered boundary on the real FC/OSD.

## Next development priorities after current OSD browser test

1. Fix only issues exposed by the OSD browser/board test above; avoid speculative UI layering.
2. Finish Advanced OSD preview/firmware semantic parity and real-board round-trip validation.
3. Audit remaining `EXPERIMENTAL_MASTER_TODO.md` items and select small independent fixes.
4. Implement short multi-camera warning cycling only as a separate scoped firmware change: cycle affected cameras rather than concatenate identifiers.
5. Design/implement Experimental **Automatic Camera Detection** while keeping manual Camera Type as a proven fallback.
6. Run broader configurator regression and camera/board test matrix.
7. Clean docs/release notes only after behaviour is validated.

## Important deferred/open items

- STOP/disarm behaviour when a saved camera is out of range remains open. Keep it separate from OSD work.
- Automatic Camera Detection is planned, not implemented.
- Manual Camera Type remains required in Single Cam mode today.
- Multi-camera same-category warning cycling is planned, not implemented.
- Non-GoPro camera-family behaviour requires external physical testers; do not mark it validated from source review alone.

## Handoff protocol

When Codex finishes a meaningful unit of work:

1. Commit it to `experimental` with a focused message.
2. Update this file's **Current state / acceptance tests** only when facts materially changed.
3. Update `EXPERIMENTAL_MASTER_TODO.md` for backlog/test-state changes without deleting existing content.
4. Run/check both required workflows against the exact final HEAD.
5. Leave browser/hardware-only checks unchecked and label them NEEDS TEST.
6. Summarize changed files, commit SHA, tests run, CI result, and what Steve should physically/browser-test next.

When ChatGPT resumes, it should inspect live `experimental`, this file, `AGENTS.md`, the master TODO, and recent commits rather than assuming the chat's last SHA is still current.
