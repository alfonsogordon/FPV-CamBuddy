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

