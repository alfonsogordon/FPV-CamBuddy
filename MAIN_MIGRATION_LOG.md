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

### Pre-change: main candidate flashing support
- Pre-change main: `dfad101f489354107cd80dd2316018d44b921b3e` was validated/deployed successfully; later label/log commits are administrative/UI cleanup.
- Goal: allow Steve to test the unreleased V1.0.2 firmware directly from the polished production-shaped main flasher before creating a GitHub release.
- Planned affected paths: `.github/workflows/pages.yml` and `web/flash.html`.
- Safety: official release assets/dropdown remain intact; the new entry is clearly labelled as an unreleased main candidate and uses firmware artifacts from the exact validated current main source.
- Rollback: revert the candidate-flasher commits recorded below.

### Main candidate flashing support
- `d235e30f74c93f883e7b6255921677aa220f896a`: Pages now copies firmware artifacts from the latest successful **Validate FPV CamBuddy** run on main into `web/firmware/main-candidate/`, together with `meta.json` containing the validated commit SHA/run ID. The existing release firmware downloads remain unchanged.
- `4023de95f9d30fe3af8c301e641fc6b1d9eb46f6`: the polished production `web/flash.html` now prepends a clearly labelled `V1.0.2 main candidate — TEST BEFORE RELEASE` option when validated main-candidate artifacts are present. Existing published releases remain selectable and unchanged.
- This lets Steve test the production-shaped main flasher before publishing/tagging V1.0.2.
- Frozen `v1.0.2` remains unchanged at `139d47de841ed7fe0c9198019926b5bb635e2285`.
- Rollback, newest first: `git revert 4023de95f9d30fe3af8c301e641fc6b1d9eb46f6`, then `git revert d235e30f74c93f883e7b6255921677aa220f896a`.
- Exact-head validation/Pages status for `4023de95...` was still queued/pending when this log entry was written; do not call the flasher candidate ready until both complete successfully.

### Candidate flasher verification
- Exact current main `14143836961f2b16313d0de4afcdec0a7955d955` completed **Validate FPV CamBuddy** successfully (run `35433670481`) and completed **Deploy GitHub Pages** successfully (run `35433670468`).
- A subsequent Pages workflow-run deployment for the same exact main SHA also completed successfully (run `35433746938`).
- The intermediate candidate-flasher commit `4023de95f9d30fe3af8c301e641fc6b1d9eb46f6` also passed validation (run `35433661032`).
- CI therefore confirms that the production site generation and both ESP32-C3/ESP32 firmware builds succeed with the replacement, version gating, and candidate-flasher changes.
- Live GitHub Pages content could not be independently fetched through the available GitHub connector during this check because that connector only accepts github.com instance URLs. Treat the successful Pages deployment as deployment verification, but Steve's browser check remains the final UI confirmation.
- Hardware status remains unchanged: Profile OSD on a real FC is still pending Steve's test.

### Pre-change: firmware detection/read-path hardening
- Pre-change main: `1a93d34df8fc84a8f8bafe31e6ec80f50614c105`.
- Review found that the visible configurator's autosync layer performs its own automatic `show` read after connection. The first firmware-gating implementation only requested `version` from the hidden legacy Read Settings button/connect path, so a normal autosync connection could leave V1.0.2-only controls in the unknown/locked state even on compatible firmware.
- Planned affected path: `web/fps-autosync.js`.
- Fix: every autosync board read/verification will request `version` before `show`, keeping unknown locked until a positive version response arrives.
- Rollback: revert the hardening commit recorded below.

### Firmware detection/read-path hardening
- `3ad6d11898595e4457f0ee260d9d66cdafedecbc`: autosync board reads and post-save verification now issue `version` before `show`, so normal automatic connection/read flow can positively identify compatible, old, or development firmware instead of remaining unknown.
- `c53020718d541f62658dfe775e476d7bd3cf7813`: `setConnected()` now reapplies the firmware gate after generic connection-state enable/disable logic. This prevents that generic logic from accidentally re-enabling V1.0.2-only controls before compatibility is known.
- Ordinary V1.0.1-compatible controls remain governed only by connection state.
- Rollback, newest first: `git revert c53020718d541f62658dfe775e476d7bd3cf7813`, then `git revert 3ad6d11898595e4457f0ee260d9d66cdafedecbc`.
- Hardware verification is still pending; this is source/CI hardening only.

### Final source/CI audit checkpoint
- Exact main `ca7be6841b7609594ee956dccbf676255c889899` passed **Validate FPV CamBuddy** (run `35434146673`) and **Deploy GitHub Pages** (run `35434146665`). A later Pages deployment for the same SHA also succeeded (run `35434242045`).
- Source audit confirms: firmware reports `1.0.2`; new FPV CamBuddy version reply is present; legacy FreeCLinker version replies remain accepted for old-firmware detection; autosync issues `version` before `show` for both read and post-save verification; the V1.0.2 capability gate remains in place.
- Production flasher source contains the validated main-candidate option and metadata lookup.
- Homepage source has no redundant `Original FreeCLinker` footer-nav link; the dedicated Original FreeCLinker credit panel and bottom `FreeCLinker by sheeprine` attribution remain intact; footer displays FPV CamBuddy V1.0.2.
- Frozen `v1.0.2` remains untouched at `139d47de841ed7fe0c9198019926b5bb635e2285`.
- Remaining acceptance work is Steve's browser/real-board testing. No release/tag and no old FreeCLinker relink has been performed.

### Final exact-head validation before hardware handoff
- Exact current main `e51c26ecbef6b73dd58081da04b628c236f5f860` completed **Validate FPV CamBuddy** successfully (run `35434332232`).
- That validation produced both expected non-expired firmware artifacts: `freeclinker-stage3-esp32c3` and `freeclinker-stage3-esp32`.
- Exact current main also completed **Deploy GitHub Pages** successfully (run `35434332227`), followed by another successful workflow-run Pages deployment (run `35434402641`). This is the final CI/Pages checkpoint before Steve's browser and hardware acceptance testing.
- No release/tag has been created and no old FreeCLinker repository/site changes have been made.

### Pre-change: mobile UI polish requested during acceptance testing
- Requested from live mobile screenshots: add more vertical separation above the blue Original FreeCLinker credit panel, and hide/remove the AP Diagnostic Log button from the normal Easy Config UI.
- Scope is presentation only on main; no firmware behavior, camera protocol, configurator save logic, release/tag, or frozen `v1.0.2` branch changes.

### Pre-change: AP local hostname
- Requested during V1.0.2 acceptance preparation: add an mDNS hostname for the field/AP configurator so supported clients can use `http://cambuddy.local`, while retaining `http://192.168.4.1` as the guaranteed fallback.
- Planned implementation: start mDNS only after the FPVCamBuddy SoftAP is successfully configured; stop mDNS when AP mode stops; failure to start mDNS must not prevent the AP/IP configurator from working.
- Documentation/UI references will advertise both `cambuddy.local` and the IP fallback.
- Scope: main only. Frozen `v1.0.2` remains untouched. No release/tag.

### AP local hostname implementation
- `fa42623a63b8e381d497e6024db843bf231dc149`: defines AP hostname `cambuddy`.
- `c7b45e9a3a3e64915c3c77755eb39e2d06aec3e0`: starts ESP32 mDNS as `cambuddy.local` after the AP/server starts, advertises HTTP on TCP/80, logs success/failure, and stops mDNS with AP mode. mDNS failure is non-fatal; `192.168.4.1` remains available.
- `0cc8bd77322f7652ae34a5ad183e9b4319014c34`, `fe5a92afa4a6b7dfc0b3b38eed72dcfec0fbc1f6`, and `98adcf97b9540ca90fce2aee86af221b104a2f99` update README, Quick Start and homepage documentation respectively.
- User-facing access while AP is active: join open SSID `FPVCamBuddy`, prefer `http://cambuddy.local`, fall back to `http://192.168.4.1` if the client/browser does not resolve mDNS.
- Hardware verification of `cambuddy.local` is pending. No release/tag; frozen `v1.0.2` unchanged.

### Configurable recording trigger
- Added a new Recording Trigger setting under AUX Switches on main. Default is ARM, preserving existing behaviour. AUX 1–12 can be selected instead; AUX high starts recording and AUX low stops recording using the existing stop-delay/stop-enable behaviour.
- Implemented as a separate MSP RC channel from the unified Camera Switch so recording and camera selection can use different AUX controls.
- Added USB Easy Config, field/AP configurator, persistent NVS/CLI (`record_aux`), AP JSON API, MSP polling/callback handling, and documentation.
- Legacy two-position camera-mode controls remain hidden for backwards compatibility only.
- Hardware testing still required before release.


### Hardware acceptance update - GoPro / AUX / OSD
- Steve physically confirmed the configurable Recording Trigger works on the current GoPro test setup: ARM remains usable as the default trigger, and a selected AUX channel can independently start recording high / stop recording low.
- Steve physically reconfirmed the unified Camera Switch behaviour and confirmed Camera Switch preset-name messages appear through the real C3 -> FC -> Betaflight OSD path.
- Native GoPro saved-preset discovery, names, persistence and LOW / MIDDLE / HIGH switching remain hardware-confirmed.
- Recording Trigger and Camera Switch can use separate AUX channels.
- The legacy two-position Camera Mode Switch has been removed from the visible Easy Config UI; compatibility internals remain hidden.
- New REC-only UI now retains an enable toggle and adds a mode choice: Recording only, or Armed + Recording. USB and AP implementations are present; this newest mode-selector behaviour still requires Steve's hardware/UI acceptance test before being marked hardware-confirmed.
- No release/tag has been created. Frozen `v1.0.2` remains unchanged at `139d47de841ed7fe0c9198019926b5bb635e2285`.

### Osmo Nano Camera Switch implementation
- Added Osmo Nano LOW / MIDDLE / HIGH Camera Switch support using the hardware-captured native DUML shooting-mode command `0x02/0xE1`.
- Exposed only known sparse mode values: Slow Motion `0x00`, Video `0x01`, Timelapse `0x02`, Photo `0x05`, Hyperlapse `0x0A`, Panorama `0x0C`, SuperNight `0x28`.
- Nano discovery presents stable CamBuddy selection IDs above zero (`100 + mode`) because profile ID `0` is reserved for Unassigned in Easy Config. `loadProfile()` maps those IDs back to the exact verified DUML mode bytes.
- Existing Nano BLE pairing/recording/telemetry path is unchanged. No arbitrary or guessed DUML values are sent.
- This new Nano Camera Switch path is implemented/CI-testable but must remain labelled **not hardware-confirmed** until a real Osmo Nano tester validates mode changes.

### GoPro MAX2 full V1.0.2 hardware confirmation
- Steve confirmed GoPro MAX2 across the current V1.0.2 GoPro feature set: connection/reconnect, recording control, live OSD/telemetry, native saved-preset Camera Switch and selection-name OSD.
- The public support matrix may therefore mark all current GoPro MAX2 feature columns as hardware-confirmed.

### Insta360 Ace Pro 2 hardware confirmation
- Steve confirmed Insta360 Ace Pro 2 connects successfully to FPV CamBuddy.
- Recording control is hardware-confirmed.
- AUX-triggered recording is hardware-confirmed.
- Richer Insta360 telemetry and broader Insta360 model coverage remain under test; Camera Switch support is still not claimed.


## DJI Action 4 hardware confirmation

DJI Action 4 is hardware-confirmed for connection, live telemetry, recording control and AUX-triggered recording. Camera Switch and selection-name OSD are not yet claimed as hardware-confirmed.
