# H9 — IR Blaster + Replay

IR signal capture, protocol decode, and remote control replay with ESP32.

## Overview

This project implements a standalone IR blaster that:
- Captures IR signals via interrupt-driven TSOP receiver
- Auto-detects NEC, Samsung, and RC5 protocols
- Decodes address/command fields with checksum verification
- Replays captured signals with configurable repeat
- Includes built-in common remote control codes
- Serial command interface for interactive use

## Hardware

| Component | Connection | Role |
|-----------|------------|------|
| ESP32 NodeMCU | Main board | Signal processing, serial interface |
| TSOP38238 | D4 (GPIO4) | IR receiver (38 kHz) |
| IR LED | D2 (GPIO2) | IR transmitter (via NPN transistor) |

## Wiring

```
TSOP38238 (IR Receiver):
  DATA → D4  (GPIO4)
  VCC  → 3.3V
  GND  → GND

IR LED (via NPN transistor, e.g. 2N2222):
  ESP32 D2 (GPIO2) → 1kΩ → Base
  Collector → IR LED cathode
  IR LED anode → 5V via 100Ω resistor
  Emitter → GND
```

## Serial Commands

| Command | Description |
|---------|-------------|
| `capture` | Record IR signal from remote |
| `replay` | Replay last captured signal |
| `replay <hex>` | Replay specific code |
| `send <hex> [proto]` | Send code via protocol |
| `codes` | Show built-in remote codes |
| `info` | Show stored signal info |

## Supported Protocols

| Protocol | Bits | Header | Use |
|----------|------|--------|-----|
| NEC | 32 | 9000/4500 µs | Most universal remotes |
| Samsung | 32 | 4500/4500 µs | Samsung TVs, ACs |
| RC5 | 14 | Manchester | Philips, older remotes |

## Serial Output

```
=== H9 — IR Blaster + Replay ===
[+] Captured 68 samples
[+] Protocol:  NEC
[+] Raw code: 0x00FF40BF
[+] Address:  0x00
[+] Command:  0xBF
[+] NEC checksum OK
```

## Build & Flash

```bash
# Using Arduino CLI
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/h9_ir_blaster.ino
arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/ttyUSB0 firmware/h9_ir_blaster.ino
```

## Legal Disclaimer

**IMPORTANT: Read before use.**

This project is provided for **educational and authorized security testing purposes only**.

### Authorization Requirements
- You MUST have explicit written permission from the device/system owner before using this tool
- Unauthorized control of electronic devices is illegal under federal and state laws
- This tool should ONLY be used on devices you own or have written authorization to test

### Legal Framework
- **Computer Fraud and Abuse Act (CFAA)**: Unauthorized access to computer systems is a federal crime
- **Wiretap Act (18 U.S.C. § 2511)**: Interception of electronic communications without consent is illegal
- **State Laws**: Many states have additional computer crime and wiretapping statutes
- **FCC Regulations**: IR transmission may be subject to radio frequency regulations

### Acceptable Use
- Testing security of your own IR-controlled devices
- Authorized penetration testing with written scope
- Academic research in controlled lab environments
- Security education and training
- Home automation development

### Prohibited Use
- Controlling devices you don't own without authorization
- Jamming or interfering with legitimate IR communications
- Any activity that violates applicable laws or regulations
- Commercial use without proper licensing

### No Warranty
This software is provided "AS IS" without warranty of any kind. The author is not responsible for any misuse or damage caused by this software.

### Responsible Disclosure
If you discover vulnerabilities using this tool, follow responsible disclosure practices:
1. Report to the vendor/owner privately
2. Allow reasonable time for remediation
3. Do not exploit beyond proof of concept

## License

MIT
