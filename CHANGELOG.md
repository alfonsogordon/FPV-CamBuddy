# FPV CamBuddy — Changelog

This is an additive changelog. Previous release notes, test branches, migration notes and commit history are retained rather than rewritten.

## v1.0.3 RC1 — release candidate (2026-10-09)

Based on the validated GoPro GPS reconnect recovery branch at `9b7321a0`.

### Added and improved
- GoPro GPS clock/timezone synchronisation and reconnect recovery code from the GPS test programme.
- Updated Wi-Fi AP and USB configuration, including GPS, power, OSD, AUX/profile and camera-matching controls present in the recovery firmware.
- Betaflight 2025.12+ Custom Messages label consistency.
- USB configuration save/readback comparison and explicit mismatch warnings.
- Explicit serial configuration end marker to improve complete settings reads.
- Firmware-aware version handling; RC1 uses firmware version `1.0.3-rc1`.

### Verification status
- Recovery baseline and RC1 firmware/web validation passed in GitHub Actions.
- **Hardware verification is still required** for complete settings persistence, reconnect edge cases, GPS time recovery, camera-family differences and OSD behaviour. Passing CI is not a substitute for a flight/bench test.

### Compatibility
- Stable v1.0.2 and previous flasher choices remain available.
- See [RC1 release notes](https://github.com/alfonsogordon/FPV-CamBuddy/blob/v1.0.3-rc1/RC1_RELEASE_NOTES.md) for test plan and limitations.

## v1.0.2 — stable / historical
The existing stable release and its documentation are retained. See [GitHub releases](https://github.com/alfonsogordon/FPV-CamBuddy/releases), [historical development TODO](FPSteve_TODO.md), and [migration log](MAIN_MIGRATION_LOG.md).

## Earlier and experimental builds
The experimental, v1.0.3 test, GPS time and GPS reconnect development branches and historical test documentation remain available in Git history and the corresponding test environments.
