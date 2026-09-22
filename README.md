> **⚠️ EDUCATIONAL USE ONLY — AUTHORIZED TESTING ONLY.**
> This project exists for education, research, and **defense of systems you own
> or hold explicit written authorization to assess**. Unauthorized use is
> prohibited and may be illegal. Read [ETHICS.md](ETHICS.md) and
> [SCOPE.md](SCOPE.md) before use. Use at your own risk; **AS IS**, no warranty.

# H9 — IR Blaster + Replay

An ESP32-powered **infrared (IR) security tester**: capture remote-control
signals, auto-detect **NEC, Samsung, and RC5** protocols, decode address/command
fields, and replay codes against **your own** lab hardware.

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Stars](https://img.shields.io/github/stars/5h4d0wn1k/h9-ir-blaster)](https://github.com/5h4d0wn1k/h9-ir-blaster)
[![Last commit](https://img.shields.io/github/last-commit/5h4d0wn1k/h9-ir-blaster)](https://github.com/5h4d0wn1k/h9-ir-blaster)
[![Issues](https://img.shields.io/github/issues/5h4d0wn1k/h9-ir-blaster)](https://github.com/5h4d0wn1k/h9-ir-blaster)

## Why H9

Infrared remotes still control TVs, HVAC, gates, and drones — and they speak a
plaintext protocol that costs a few dollars to intercept. H9 is a **hardware
security** study tool: it captures IR timings interrupt-driven, decodes the
three dominant consumer protocols, and replays codes through an IR LED. The
point is understanding **RF/infrared signal replay** for your own lab
experiments and home-automation tinkering. All targets must be devices **you
own or hold explicit authorization to control**; replaying against other
people's devices may violate state and federal law.

## Features

- **Signal capture** — interrupt-driven TSOP38238 receiver with timing capture
- **Protocol auto-detection** — NEC (32-bit), Samsung (32-bit), RC5 (Manchester, 14-bit)
- **Decode with checksum** — address/command fields plus NEC checksum verification
- **Replay** — configurable repeat count; replay last capture or a raw hex code
- **Built-in remote codes** — common TV/AC presets via the `codes` command
- **Serial command interface** — `capture`, `replay`, `send`, `codes`, `info`
- **Host helper** — offline decode analysis of hex code files (`host/h9_cli.py`)

## Quickstart

```bash
# ESP32 firmware (Arduino CLI)
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/h9_ir_blaster
arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/ttyUSB0 firmware/h9_ir_blaster

# Host-side offline decode analysis
python3 host/h9_cli.py --demo
python3 host/h9_cli.py --file fixtures/ir_timings.log

# Tests
python3 -m unittest discover -s tests
```

Serial commands: `capture` · `replay [hex]` · `send <hex> [proto]` · `codes` · `info`.

## Examples

- `fixtures/ir_timings.log` — sample infrared hex-code captures for offline decode demos

## Project structure

- `firmware/h9_ir_blaster/` — ESP32 sketch (see `firmware/README.md`)
- `host/` — Python helpers (`h9_cli.py`, `hw_common.py`)
- `fixtures/` — sample captures
- `docs/`, `tests/` — architecture notes and unit tests

## Documentation

- [firmware/README.md](firmware/README.md) — hardware wiring, build, and flash

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) and [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).

## License

MIT — see [LICENSE](LICENSE).

## Legal

- [ETHICS.md](ETHICS.md) · [SCOPE.md](SCOPE.md) · [SECURITY.md](SECURITY.md)