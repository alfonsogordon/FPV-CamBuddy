# FPSteVe Edition V1 defaults

Fresh-board defaults for V1 hardware testing:

- GoPro camera type
- Camera matching: fallback to any camera
- Prevent camera wake-up: ON
- Stop recording on disarm: ON
- Disarm delay: 5000 ms (5 s)
- Low power mode: ON
- Wi-Fi config AP: OFF
- AUX Camera Controls: OFF
- Betaflight mode: current / BF 2026.6+ Custom Messages 1-4
- OSD Templates: ON
- Custom Message 1: ON — `{batt}`
- Custom Message 2: ON — `{state} {recdur}`
- Custom Message 3: ON — `{mode} {res} {fps} {eis}`
- Custom Message 4: ON — `{rectf} {rcap}`
- REC-only: OFF
- Flash REC: ON
- Temporary Message: OFF, latent text `CLEAN LENS`
- Camera Warnings: ON
- Low battery threshold: 10%
- Low recording time threshold: 5 minutes
- Legacy Pilot Name default: ON — `{stateonly} {batt} {rectf}`
- Legacy Craft Name default: OFF — latent `{res} {fps}`

The browser configurator must read these values from a factory-erased C3, show them accurately, write a full snapshot with the single top-level Save/Apply button, then verify them by readback.
