# Advanced Multi Cam OSD — Experimental Behaviour

This document defines the intended Advanced Multi Cam OSD behaviour on the `experimental` branch. It is an experimental specification and must be physically/browser tested before release to `main`.

## Source rules

- `Auto` uses the existing aggregate Multi Cam behaviour. It is not treated as ownership of the OSD box by any individual camera.
- A pinned source (`C1`, `C2`, etc.) uses that persistent saved camera number.
- Identifier mode may display the persistent number (`C1`) or saved tag (`FRONT`); a missing tag falls back to the camera number.
- If every pinned element in an OSD box uses the same camera, identify that camera once only. Example: Status + Battery + Time Remaining from C1 renders as `RDY-C1 B:68 T:42m`, not `RDY-C1 B:68-C1 T:42m-C1`.
- If a box genuinely mixes pinned sources, each newly encountered source is identified so the values remain unambiguous. Repeated later elements from an already identified source do not repeat the identifier.
- A missing pinned camera must not silently substitute another camera.

## Camera-specific warnings

Camera-specific warning takeover applies only when that camera is explicitly selected as a pinned source in that OSD box.

Example: if Custom Message 3 contains one or more elements pinned to C3, and C3 crosses an enabled battery, remaining-record-time, or temperature warning threshold, the warning is eligible to temporarily take over Custom Message 3. When the warning phase clears, the configured C3 telemetry returns.

A C3 warning must not take over a box that contains only C1/C2 sources. `Auto` does not count as selecting C3. Boxes with no pinned source retain the existing aggregate/global warning-target behaviour.

If a mixed-source box explicitly contains more than one pinned camera, enabled warnings belonging to those represented cameras may cycle in that box. Cameras not represented by a pinned source in that box are excluded.

## 16-character output

The final MSP custom-text payload remains capped at 16 characters. Identifier de-duplication is intended to preserve useful space without losing source meaning. Capacity/UI guards should follow actual rendered output and known working combinations rather than pessimistic token-length assumptions.

## Required validation

- Browser: same-source C1 Status + Battery + Time Remaining previews as `RDY-C1 B:68 T:42m` (values may vary).
- Browser: mixed sources remain distinguishable.
- Hardware: same-source identifier is emitted only once on the FC OSD.
- Hardware: a C3-only warning appears in a box explicitly containing C3.
- Hardware: that C3 warning does not appear in a box containing only another pinned camera.
- Hardware: Auto-only boxes retain existing global/aggregate warning behaviour.
- Hardware: warning clears/cycles back to the configured telemetry without disturbing camera control.

Do not mark these hardware/browser checks complete from CI alone.