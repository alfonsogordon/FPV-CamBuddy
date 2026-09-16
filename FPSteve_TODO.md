# FPV CamBuddy — V1 Release Status

The core V1 behaviour is feature-complete and the project is in final validation/release preparation.

## Hardware-confirmed for V1 🤘

- ESP32-C3 Super Mini hardware target
- GoPro BLE discovery/automatic connection and reconnect
- Wake Guard behaviour with sleeping GoPro
- Low-power BLE mode
- GoPro keepalive during the powered flight session
- Real Betaflight 4.5 MSP connection
- Real FC ARM starts GoPro recording
- Real FC DISARM restores normal OSD immediately
- Configured 5-second delayed stop after disarm
- Camera state returns REC → RDY after the delayed stop
- BF 4.5 Pilot/Craft OSD path
- ERR / RDY / REC camera state reporting
- REC-only while armed + recording
- Flash REC behaviour
- CLEAN LENS first-arm reminder behaviour
- FPSteVe Easy Config read/save/read-back verification
- Settings persistence through reconnect and C3 power cycle
- Web flashing and post-flash power-cycle/configuration flow

## Implemented / validated in software, awaiting matching or forced hardware conditions

### Betaflight 2026.6+ Custom Messages 1–4

Implemented with four independently configurable Custom Message templates and integrated into the same OSD state/priority system. The Preview and firmware logic have been tested, but the V1 physical flight controller available for acceptance testing runs Betaflight 4.5.

Therefore this path is deliberately documented as **not yet hardware-tested**, rather than being presented as physically confirmed.

### Camera warning conditions

BATT LOW / REC LOW / CAM HOT and the warning priority/alternation logic are implemented and validated in the integrated Preview/firmware flow. The installed flight-test camera has not been deliberately forced through every individual warning condition, so the docs do not label all warning triggers as physical hardware tests.

## V1 defaults

### Camera / flight
- Camera match: Any
- Wake Guard: ON
- Stop recording on disarm: ON
- Disarm delay: 5000 ms
- Low Power: ON
- Wi-Fi AP: OFF
- AUX: OFF

### BF 4.5 legacy
- Pilot Name: ON
- Pilot template: `{stateonly} {batt} {rectf}`
- Craft Name: OFF
- Craft latent/default template: `{res} {fps}`

### BF 2026.6+
- OSD templates: ON
- Custom Messages 1–4: ON
- Message 1: `{batt}`
- Message 2: `{state} {recdur}`
- Message 3: `{mode} {res} {fps} {eis}`
- Message 4: `{rectf} {rcap}`

### OSD behaviour
- REC-only when armed + recording: ON
- Flash REC at 1 Hz: ON
- Temporary message: ON
- Temporary text: `CLEAN LENS`
- Only before first arm: ON
- Duration: 1.0 second
- Destination: Custom Message 2
- Camera warnings: ON
- Low camera battery: 10%
- Low recording time: 5 minutes
- CAM HOT: automatic
- Warning destination: Custom Message 1

Priority:

**Warning → Temporary Message → REC-only → normal configured OSD**

## V1 pairing observation

A camera that FreeCLinker has not paired with before may need to be placed in its **Pair** menu for the initial connection. A HERO11 Black Mini followed by a MAX2 is one example observed during V1 testing; those models are examples, not a restriction to those specific cameras.

## Final release checklist

- [x] README and Quick Start finalised
- [x] Homepage and OSD demonstration visually signed off
- [x] Squadding Quads Discord support invite added to public documentation/site
- [ ] Final validation/build/site review
- [ ] Publish final `v1.0.0` release as **V1.0.0**
- [ ] Verify release firmware assets and live Web Flasher release selection
- [ ] Retire the hardware-test preview release after V1.0.0 is verified

## Post-V1 candidates

Keep post-V1 feature development separate from the V1 release-preparation pass. Possible future work includes broader camera-specific testing/support and improvements driven by community feedback.
