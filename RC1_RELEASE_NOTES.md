# FPV CamBuddy v1.0.3 RC1 — release candidate notes

**Status:** Release candidate; not the stable v1.0.3 release. **Base:** GPS reconnect recovery branch at `9b7321a09986baffb2c5aa23464951e6ce026580`. **Stable fallback:** v1.0.2 remains available.

## What's included
- GoPro GPS-derived clock/timezone configuration and reconnect recovery work from the GPS recovery test branch.
- Wi-Fi AP configuration and USB configuration improvements from the recovery branch.
- OSD templates and Betaflight compatibility presentation updated to **BF 2025.12+** for Custom Messages 1–4.
- USB settings save/readback verification, with explicit mismatch reporting.
- Firmware `[cfg] END_SHOW` marker and USB handling for more reliable complete configuration reads (with compatibility fallback for earlier firmware).
- Camera matching, BLE power, AUX/profile controls, and other features already present in the recovery codebase.

## Validation and testing
- GitHub validation passed for the recovery baseline `9b7321a0`.
- **RC1 itself still requires a successful build and installation test.**
- Previous user/community feedback includes early GoPro HERO13 and AP configuration testing; this is **not** a claim that all reconnect, GPS sync, and settings-persistence scenarios are hardware-proven.
- Check: USB read → edit → save → verify → disconnect/reconnect → read → power-cycle → read.
- Check: AP settings against USB settings, GoPro reconnection after camera power cycles, GPS clock synchronization, OSD modes and camera model differences.

## Release policy
- Keep all previous releases, experimental builds, changelogs, tests, and historical documents.
- Do not remove compatibility/safety warnings. Remove only outdated generic development banners after confirming firmware-aware gating.
- Do not list RC1 in the public main flasher until the matching firmware binary is built, published, and reachable at the flasher's manifest paths.
- Maintain firmware-aware configurator compatibility: older devices must not be offered unsupported settings.

## Historical references
- [README](README.md)
- [Quick start](QUICKSTART.md)
- [Development TODO / history](FPSteve_TODO.md)
- [Migration log](MAIN_MIGRATION_LOG.md)
- [Stable and historical releases](https://github.com/alfonsogordon/FPV-CamBuddy/releases)
