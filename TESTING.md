# Testing & Validation Log

## Hardware under test

| Item | Detail |
|---|---|
| Controller | ESP32-S3 DevKitC-1 |
| Relay | 1-channel relay module, active-LOW input on GPIO4 |
| Valve | 12 V DC solenoid valve (Sheng NG), pilot-operated |
| Valve supply | 12 V DC, 1 A adapter, separate from ESP32 USB power |

## Tests performed (2026-10-03 to 2026-10-04)

### T1 — Valve held open by firmware
- **Setup:** GPIO4 driven LOW continuously (relay ON, valve powered).
- **Expected:** steady water flow.
- **Observed:** water came out in pulses, roughly 3 s flowing then 5 s stopped, repeating.

### T2 — Relay switching check
- **Setup:** firmware toggled GPIO4 HIGH/LOW every 3 s, state printed on serial monitor.
- **Expected:** valve opens in the LOW state only.
- **Observed:** serial output confirmed the pin toggled correctly and the ESP32 did not reset. The valve did not open fully in either state.
- **Conclusion:** firmware and relay logic are not the cause.

### T3 — Direct 12 V bypass
- **Setup:** ESP32 and relay disconnected; 12 V adapter connected straight to the valve coil.
- **Observed:** valve still did not give full flow.
- **Conclusion:** fault is in the valve or its plumbing, not the electronics.

### T4 — High-pressure supply, valve as originally fitted
- **Setup:** valve connected to a high-pressure direct supply, 12 V applied.
- **Observed:** valve did not open.

### T5 — Flow direction check
- **Finding:** the valve was installed against the flow-direction arrow on its body.
- **Explanation:** a pilot-operated valve uses inlet pressure on the diaphragm to open. Fitted backwards, pressure acts on the wrong side, so the closed valve leaks in pulses (T1) and cannot open when powered (T4).
- **Action:** valve re-fitted in the correct direction.

### T6 — Correct direction, different supply pressures
| Supply | Result |
|---|---|
| High-pressure direct supply | Valve opens and flows fully. **Pass** |
| Household tap | Not enough pressure to lift the diaphragm; flow does not pass. **Fail** |

## Findings
1. Root cause of the pulsed leaking was **reversed valve orientation**, not firmware.
2. This valve is pilot-operated and needs a **minimum inlet pressure** to open. Low-pressure sources (tap, gravity tank) are not enough.
3. Firmware, relay and 12 V 1 A supply all work correctly.

## Limitations & risks
- Valve will not work on low-pressure or gravity-fed supplies. Fix: direct-acting (zero-pressure) valve, or a pump before the valve.
- No flyback diode fitted yet across the valve coil (planned: 1N4007).
- Minimum operating pressure not yet measured.

## Planned tests (firmware)
| ID | Test | Pass condition | Result |
|---|---|---|---|
| F1 | Wi-Fi drops and returns | ESP32 reconnects automatically | — |
| F2 | MQTT `OPEN` / `CLOSE` commands | Valve follows command, state published | — |
| F3 | Network lost while valve open | Valve closes automatically | — |
| F4 | Power cycle | Valve stays closed on boot | — |
