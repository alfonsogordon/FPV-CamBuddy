# FPV CamBuddy Main Migration Log

This is the rollback/audit record for promoting the tested V1.0.2 work into FPV CamBuddy `main`.

## Safety rules
- Repository: `alfonsogordon/FPV-CamBuddy` only.
- Do **not** modify or relink the old FreeCLinker / FPSteVe Edition repository or Pages until Steve explicitly approves after real-board testing.
- Do **not** tag or publish V1.0.2 until Steve explicitly approves.
- Preserve the polished production `web/flash.html`; do not replace it with the V1.0.2 test flasher.
- Do not mark Profile OSD as hardware-confirmed until Steve tests the real C3 -> FC -> Betaflight OSD path.
- Every migration change must be reversible and recorded here.

## Baseline - first manual run (2026-09-19 Europe/London)

### Pre-change state
- `main`: `7089699cac9975a0b2877bb5adf83cf29bacd079` - Refresh V1.0.2 staging for latest homepage candidate
- `v1.0.2`: `139d47de841ed7fe0c9198019926b5bb635e2285` - Polish homepage hero and FreeCLinker panel spacing
- Merge base: `a930fce47d2814e465dcf8d4279d7ae510354076`
- GitHub comparison: `v1.0.2` is 152 commits ahead and 36 commits behind `main`; branches are diverged.

### Decision
A blind branch merge is intentionally **not** being performed. Main contains production/Pages work that must be preserved, while V1.0.2 contains test-only files such as `.github/workflows/v1.0.2-test.yml` and `web/v1.0.2-flash.html`. Migration will therefore be done carefully from the production main baseline, bringing across intended V1.0.2 source/configurator/site changes while preserving production-only main behavior.

### V1.0.2 changed-path inventory from GitHub compare
Firmware/config paths include `include/config.h`, BLE/camera protocol sources, ConfigManager, MSP serial, Nano DUML, web server/content and main firmware logic.
Web/docs paths include `README.md`, `QUICKSTART.md`, `web/config.html`, homepage and supporting JS.
Test-only paths include `.github/workflows/v1.0.2-test.yml`, `OVERNIGHT_V1.0.2_LOG.md`, and `web/v1.0.2-flash.html`.

### Verification state
- Latest V1.0.2 homepage source is at `139d47de841ed7fe0c9198019926b5bb635e2285`.
- The previous candidate `dafd798cb47571ad856e90efc923d608529b0099` built successfully; the homepage-only follow-up still requires its normal CI/Pages verification.
- Profile OSD browser/demo behavior has been tested, but real-board/FC OSD confirmation is still pending.
- GoPro native saved-profile switching is already hardware-confirmed and must not regress.

### Rollback baseline
Before any functional migration commit, the exact known pre-migration production pointer is:

```
7089699cac9975a0b2877bb5adf83cf29bacd079
```

For individual migration commits, prefer `git revert <migration-commit-sha>` so history is preserved. If full recovery is ever required, create a recovery branch/tag at the baseline SHA above before any reset. Do not force-reset public main without Steve's explicit approval.

## Change log

### Audit-log bootstrap
- Purpose: create this migration/rollback record before functional V1.0.2 promotion begins.
- Functional firmware/site change: none.
- Rollback: revert the commit that introduced `MAIN_MIGRATION_LOG.md` if the audit file itself needs removing.


### V1.0.2 production replacement
- Pre-change main: `843ccc03b71fecc19e6efa90329ee368e97093c4`.
- Replacement commit: `456dbbd60cd24e2e663755d767df83723dd42cc6`.
- Source snapshot: frozen `v1.0.2` at `139d47de841ed7fe0c9198019926b5bb635e2285`.
- Replaced the corresponding V1.0.2 firmware/configurator/homepage/docs paths on main directly from the V1.0.2 blob snapshot; this was not a branch merge.
- Preserved production `web/flash.html` and production workflow/Pages/release infrastructure.
- Did not copy `.github/workflows/v1.0.2-test.yml`, `OVERNIGHT_V1.0.2_LOG.md`, or `web/v1.0.2-flash.html` into production.
- Rollback: `git revert 456dbbd60cd24e2e663755d767df83723dd42cc6`.

### Firmware compatibility gating
- Pre-change main: `456dbbd60cd24e2e663755d767df83723dd42cc6`.
- Configurator gating commit: `ea56d40bcdb230c2390f978ddc8bea6fd11e336f`.
- The USB/browser configurator now asks the board for `version`, parses both legacy `FreeCLinker firmware v...` and new `FPV CamBuddy firmware v...` replies, and distinguishes compatible (>=1.0.2), too-old (<1.0.2), unknown/not-yet-detected, and non-semver development/test version strings.
- V1.0.2-only Camera Profile Switch/Profile OSD controls remain disabled unless compatible firmware is positively detected. Older/unknown/development firmware gets a clear update message and production flasher link. Ordinary V1.0.1-compatible controls remain available.
- Firmware version bump commit: `14aa7882a078a59c6d9ea60797f3011a98815c3f` sets `FIRMWARE_VERSION` to `1.0.2` on main only.
- Version response branding commit: `3bedef5a8c10b2f2e8d2a8ff77dc532609cc4b92` changes the new main firmware response to `FPV CamBuddy firmware v...`; the configurator intentionally still accepts the legacy FreeCLinker form for V1.0.1 detection.
- Rollback, newest first: `git revert 3bedef5a8c10b2f2e8d2a8ff77dc532609cc4b92`, `git revert 14aa7882a078a59c6d9ea60797f3011a98815c3f`, `git revert ea56d40bcdb230c2390f978ddc8bea6fd11e336f`.

### Homepage/footer cleanup
- Commit: `a2ca2b95a60a6ea3d7c7f6b6955e1c568f0f8830`.
- Removed only the redundant footer-navigation link labelled `Original FreeCLinker`.
- Preserved the dedicated Original FreeCLinker credit panel, original GitHub/project-site links, and footer `Based on FreeCLinker by sheeprine` attribution/link.
- Rollback: `git revert a2ca2b95a60a6ea3d7c7f6b6955e1c568f0f8830`.

### Current verification checkpoint
- Current main before this log update: `a2ca2b95a60a6ea3d7c7f6b6955e1c568f0f8830`.
- CI/Pages were still processing the sequence of main commits when this checkpoint was written. Do not call the candidate validated until the exact current main HEAD validation and deployment complete successfully.
- Real C3 -> FC -> Betaflight Profile OSD remains **NEEDS HARDWARE TEST**. No hardware-confirmed claim has been added.
- Frozen `v1.0.2` must remain at `139d47de841ed7fe0c9198019926b5bb635e2285`.

### Production-label cleanup
- `cf5a81c909e2d97eb13dc4088d0d8b6e58cc98eb`: removed the inherited `TEST` suffix from the Camera Profile Switch label on main; the frozen V1.0.2 test branch is unchanged.
- `ecc8ff23ae1e2b24babc1b7f3b3903b9c10f2a05`: updated the main homepage footer display from `FPV CamBuddy V1.0` to `FPV CamBuddy V1.0.2`.
- Rollback, newest first: `git revert ecc8ff23ae1e2b24babc1b7f3b3903b9c10f2a05`, then `git revert cf5a81c909e2d97eb13dc4088d0d8b6e58cc98eb`.
