# FPSteVe Edition TODO / ideas

Future ideas to investigate after the current single-camera ARM/record and OSD path is stable.

## Advanced camera settings

- Add capability-aware camera settings control where the connected camera API supports it.
- Potential controls: exposure / EV compensation, ISO limits, shutter speed, white balance / colour settings, resolution, frame rate, stabilisation, lens / FOV and other useful shooting parameters.
- Show only controls supported by the detected camera model / firmware.
- Expose useful settings in the browser configurator.
- Investigate optional Betaflight AUX mappings for selected settings without making the normal ARM/record path more complicated.

## Multi-camera sync

- Investigate pairing/registering multiple cameras to one FreeCLinker controller.
- ARM should start all selected cameras as close together as practical; DISARM should stop them according to configured behaviour.
- Show per-camera connection, recording and error state.
- Investigate BLE connection limits and timing accuracy.
- Consider whether pre-connection / pre-sync / countdown techniques can improve start-time alignment.
- Longer-term goal: mixed camera families (for example GoPro + DJI), while keeping camera-specific capability handling.
- Preserve the existing reliable single-camera mode as the default/fallback.

## Existing near-term work

- Finish OSD Preview visual integration with the configurator tabs.
- Use the exact supplied FPV DVR image as the OSD Preview background without resizing/re-encoding it.
- Finish and hardware-test USB bench ARM/DISARM -> normal camera recording callback.
- Continue FPSteVe homepage/tested-hardware/tester-report improvements.
