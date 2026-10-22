# FPV CamBuddy repository cleanup audit — 2026-10-22

Status: audit only; no production branches or firmware modified.

## Current baseline
- Stable fallback: `v1.0.2` (preserve).
- Release candidate: `v1.0.3-rc1`; `v1.0.3` currently identical by GitHub compare.
- `main` diverged from RC1 (main has 60 commits absent from RC1, RC1 has 136 absent from main). Do not merge wholesale.
- 101 branches enumerated.

## Branch comparisons relative to v1.0.3-rc1
| Branch | Ahead | Behind | Decision |
|---|---:|---:|---|
| v1.0.3 | 0 | 0 | Duplicate reference; consider archive after checking external links |
| gopro-gps-reconnect-test | 0 | 11 | Fully merged history; candidate removal |
| gopro-gps-time-test | 0 | 51 | Fully merged history; candidate removal |
| test/rc1-gopro-reconnect-stability | 6 | 0 | Superseded by startup diagnostics history |
| test/rc1-gopro-startup-diagnostics | 17 | 0 | Preserve; GoPro changes and test flashers |
| dev-dji-nano-aux-range | 33 | 0 | Preserve; separate development |
| archive/v1.0.3-multicam-2026-10-06 | 29 | 38 | Preserve archive |
| v1.0.3-old-reference | 88 | 137 | Preserve pending code review |
| v1.0.2 | 153 | 280 | Preserve stable release |

Startup diagnostics is 11 commits ahead of reconnect-stability and zero behind it, so reconnect-stability is contained in its history.

## Source / workflow findings
- GoPro startup diagnostics modifies `src/gopro_camera.cpp` and `src/gopro_camera.h`; test build assets and flash pages are also changed.
- Nano development modifies `src/ble_camera.cpp`, `src/main.cpp`, `src/msp_serial.cpp`, config manager, and web configurator.
- RC1 validation builds ESP32-C3 and ESP32 and generates the website. Its push triggers still name both old GPS test branches.
- RC1 release notes explicitly state hardware validation is incomplete; no unverified changes should be advertised as stable.
- Preserve historical releases, docs, firmware and deployed URLs.

## Next actions (not yet performed)
1. Inspect and diff GoPro startup diagnostics logic against RC1; build both targets and test hardware before integrating.
2. Keep Nano development isolated; test Nano no-wake and AUX behavior on hardware.
3. Audit Pages/flash manifests and workflow permissions; ensure test branches cannot overwrite production.
4. Update CI trigger list only after deployment references are checked.
5. Inventory legacy UI branches and verify merge ancestry before deletion; preserve refs/tags for anything unique.
6. Archive or remove only verified redundant branches, with a documented rollback reference.
7. Update release notes and test matrix after hardware validation.

No branches have been deleted or merged as part of this audit.
