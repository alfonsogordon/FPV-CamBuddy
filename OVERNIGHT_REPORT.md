# Experimental V1.0.2 Work Report

> Living report for `experimental` only. Stable `main` remains untouched.

## 2026-09-15 — Advanced Multi Cam OSD UI + board wiring audit

### Changes

1. `a07f3abc8d76fe76650f922cdf3ec19c1c0c0338` — **Rebuild Advanced Multi Cam OSD simulator UI**
   - Replaced the fragile anonymous label/text-node layout with explicit camera title, Connected/REC toggle rows, Battery row and Time Remaining row.
   - Auto source now writes the normal aggregate token (`{state}`, `{batt}`, `{rectf}`, etc.) rather than an unnecessary `@0` token. A camera is encoded only when a specific saved camera is selected.
   - Simulator remains browser-only; its values drive preview testing and do not pretend to be device settings.
   - **NEEDS TEST:** phone and desktop visual acceptance.

2. `58cdfcc37e07518a4527e33abf11a2f42fc2d6a2` — **Expand OSD template storage for Advanced source tokens**
   - Increased `ConfigManager::OSD_TPL_LEN` from 32 to 64 bytes.
   - Reason: rendered Betaflight text is capped at 16 characters, but Advanced template syntax such as `{state@1n} {batt@2n} {rectf@3n}` is much longer before rendering. The old 32-byte storage could truncate a valid Advanced template before the MSP renderer saw it.
   - Existing NVS load/save/set paths use `sizeof(...)` / `OSD_TPL_LEN`, so they automatically use the larger buffers.
   - **NEEDS TEST:** Save / Read Settings round-trip on real C3 with a template whose encoded syntax is >31 bytes but rendered output is <=16.

3. `efbcbf100ff1dc97524aa33a27a9418ba02702a3` — **Align active theme with rebuilt Multi Cam OSD controls**
   - Removed the old generic simulator-label layout assumptions from the active theme and targeted the new explicit row classes.
   - **NEEDS TEST:** desktop + phone layout.

4. `dbc3fb89591d532217ddb9e41734ca755982012e` — **Sync Advanced OSD UI from board templates**
   - On `fps-config-read-complete`, the configurator now inspects the templates that actually came from the C3.
   - If a saved template contains a pinned Advanced token (`@<camera><n|t>`), Advanced UI is enabled and the identifier mode is restored from the token.
   - If the board contains only simple aggregate tokens, Advanced UI is set OFF and the simple templates become the browser-side baseline.
   - This makes the board the source of truth for Advanced-vs-simple OSD after a Read Settings operation instead of relying only on browser localStorage.
   - **NEEDS TEST:** save Advanced template -> reconnect/reload -> Read Settings -> Advanced UI and source/tag semantics restore correctly.

### Wiring audit — Advanced Multi Cam OSD

| Area | Result | Evidence / notes |
| --- | --- | --- |
| Advanced feature gating | PASS by code inspection | `advancedOn()` requires Experimental + Multi Cam + OSD + Advanced toggle. |
| Per-box Source selector | PASS by code inspection | Specific source generates `{token@Nn}` or `{token@Nt}`. Auto now generates the normal aggregate token. |
| Camera number IDs | PASS by code inspection | Browser uses `C#`; firmware `parseTokenSpec()` accepts `n` mode and `makeIdentifier()` emits `C#`. |
| Saved camera tags | PASS by code inspection | Browser limits labels to 7 chars and falls back to C#; firmware uses registry labels for `t` mode and falls back to C#. |
| Pinned absent-camera Status | PASS by code inspection | Browser preview gives `OFF-C#` / `OFF-TAG`; firmware `formatSourceToken()` gives `OFF` plus requested identifier when source is absent. |
| Aggregate Status count | PASS by code inspection / browser partially accepted | Preview and firmware retain `RDY (N)` / `REC (N)` for aggregate Status. Steve has already confirmed the corrected character-count behaviour is much better with four dummy cameras. |
| Auto lowest battery/time | PASS by code inspection | Auto uses plain aggregate `{batt}` / `{rectf}`; firmware aggregate data is populated from the lowest battery / least remaining time source without cluttering normal output with a suffix. |
| Browser preview source state | PASS by code inspection | Connected, REC, Battery and Time Remaining simulator values feed the same camera map consumed by Advanced preview resolution and capacity calculation. |
| Save/apply path | PASS by code inspection | `fps-autosync.js` snapshots the actual `osd1..4`, `pilotTpl`, `craftTpl` strings and sends them with `set osdN`, `set pilot_tpl`, `set craft_tpl`. |
| Firmware token parser | PASS by code inspection | `src/msp_serial.cpp` parses `token@source[n|t]`, resolves pinned sources, aggregate sources and identifiers, then expands through the normal MSP text path. |
| Firmware per-camera telemetry | PASS by code inspection | `MultiCameraCoordinator::publishState()` preserves normal aggregate telemetry and fills `CameraData.sources[]` with stable registry identity + per-camera telemetry; GoPro supplies individual slots and other families contribute their live source. |
| Firmware 16-char output cap | PASS by code inspection | MSP text renderer keeps `TEXT_LIMIT = 16`; configurator guard is intended to prevent invalid combinations rather than increasing the Betaflight limit. |
| Encoded template storage | FIXED, NEEDS HARDWARE TEST | Increased from 32 to 64 bytes so Advanced token syntax is not truncated before rendering. |
| Board read-back of Advanced mode | FIXED, NEEDS HARDWARE TEST | Read Settings now infers Advanced/identifier mode from the board template syntax. |
| Multi-camera warning cycling per affected camera | OPEN | Current firmware cycles warning categories; multiple affected cameras within one warning category still needs the separate short-message cycling work previously discussed. Do not mark fixed yet. |

### What is ready for real-board testing

The Advanced template format is already wired end-to-end: configurator template -> Save/Apply CLI -> persistent C3 template -> firmware Advanced token parser -> per-camera coordinator telemetry -> 16-character MSP text output. The new browser simulator is not itself written to the board; it is intentionally a test harness for the same selection semantics.

The remaining real-board acceptance test is therefore not a missing wiring task. It is validation that the encoded Advanced templates survive Save/Read, resolve the expected physical camera identity/telemetry, and produce the expected 16-character Betaflight OSD output.

## Ordered next test checklist

1. Hard-refresh experimental and verify the rebuilt camera cards are visually different: camera title + LIVE/OFF badge, Connected/REC on one row, Battery and Time Remaining on separate aligned rows.
2. Four dummy cameras connected, C# mode, Status + Battery + Time Remaining: confirm `RDY (4)` and valid <=16 counter.
3. Change lowest Battery/Time camera and confirm Auto preview follows the new lowest value with no suffix.
4. Pin Battery to C1, then C2: confirm suffix and counter change correctly.
5. Pin Status to a disconnected camera: confirm `OFF-C#` or `OFF-TAG`.
6. Switch C# -> Saved camera tag and confirm preview + counter update immediately.
7. Real C3: save an Advanced template whose encoded syntax exceeds 31 bytes but rendered output is <=16, Read Settings again, and confirm it is not truncated.
8. Reload/reconnect, Read Settings, and confirm Advanced mode + identifier mode are inferred from the board template.
9. Real FC/Betaflight OSD: verify pinned source values, aggregate `RDY (N)`, absent-camera OFF and 16-character boundary.

## CI state

Current exact-head CI must be checked again after this report commit before calling the branch ready to test.
