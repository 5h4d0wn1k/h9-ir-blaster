# IR Blaster + Replay Firmware

## Purpose

Capture and decode IR remote signals in the lab; replay targets are your own A/V gear only.

## Board

- **Board**: ESP32 NodeMCU + TSOP38238 + IR LED
- **FQBN**: `esp32:esp32:esp32`
- **Sketch**: `h9_ir_blaster/h9_ir_blaster.ino`

## Wiring

```
IR receiver data -> GPIO4 (D4), IR LED (via NPN transistor) -> GPIO2 (D2), GND common.
```

## Build

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/h9_ir_blaster
# upload (example, ESP32-C6):
# arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/ttyACM0 firmware/h9_ir_blaster
```

## Runtime

See the root README "IMPORTANT" section before powering on. This firmware is
for authorized own-lab study. Serial console exposes the interactive command
set described in the root README. All identifiers in the sketch are
placeholders (`lab-*` SSIDs, `00:11:22:33:44:55`, RFC 5737 / example.com).
