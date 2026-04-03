# Joltik V2 — Mini Sumo Robot

[![CI](https://github.com/MrDaila007/joltik-v2-mini-sumo/actions/workflows/ci.yml/badge.svg)](https://github.com/MrDaila007/joltik-v2-mini-sumo/actions/workflows/ci.yml)

Mini sumo competition robot — from SolidWorks CAD to firmware for three hardware platforms. Includes Arduino sketches for ATmega328 (V1) and RP2040-Zero (V2), a Zephyr RTOS port, and a WiFi web-controller on ESP8266.

## Hardware

| | V1 Board | V2 Board |
|---|---|---|
| **MCU** | ATmega328 (Arduino Nano) | RP2040-Zero |
| **Opponent sensors** | 3x SharpIR analog + 2x digital | 4x digital (active-low) |
| **Edge sensors** | 1x analog | 2x analog |
| **Motor driver** | TA6586 dual H-bridge | TA6586 dual H-bridge |
| **Extras** | — | WS2812 LED, RC5 IR remote, servo, OLED (opt.) |
| **WiFi control** | — | ESP8266 (Wemos D1 Mini) companion |

## Repository Structure

```
CAD/
  ├── *.SLDASM / *.SLDPRT      SolidWorks assemblies & parts
  ├── CUT/                      DXF files for laser/CNC cutting
  ├── STEP/                     STEP exports for cross-CAD use
  └── STL/                      3D-printable models
FIRMWARE/
  ├── JoltikV1_last_test_firmware/   Competition firmware (ATmega328)
  ├── JoltikV2_sumo_firmware/        Competition firmware (RP2040-Zero)
  ├── JoltikV2_test_firmware/        Extended test with ESP telemetry link
  ├── Joltik_v2_sumo_board_test/     Hardware verification sketch (V2)
  ├── JoltikV2_web_controller/       WiFi dashboard (ESP8266)
  ├── joltik_v2_zephyr/              Zephyr RTOS firmware (RP2040-Zero)
  └── Lib/                           Vendored: NeoPixel, GyverMotor, RC5, SharpIR
.github/workflows/ci.yml            Arduino + Zephyr build CI
```

## Build & Upload

### Arduino (arduino-cli)

```bash
# V1 — Arduino Nano
arduino-cli compile --fqbn arduino:avr:nano --libraries FIRMWARE/Lib \
  FIRMWARE/JoltikV1_last_test_firmware/
arduino-cli upload  --fqbn arduino:avr:nano -p /dev/ttyUSB0 \
  FIRMWARE/JoltikV1_last_test_firmware/

# V2 — RP2040-Zero
arduino-cli compile --fqbn rp2040:rp2040:waveshare_rp2040_zero --libraries FIRMWARE/Lib \
  FIRMWARE/JoltikV2_sumo_firmware/
arduino-cli upload  --fqbn rp2040:rp2040:waveshare_rp2040_zero -p /dev/ttyACM0 \
  FIRMWARE/JoltikV2_sumo_firmware/

# Web controller — ESP8266 (Wemos D1 Mini)
arduino-cli compile --fqbn esp8266:esp8266:d1_mini --libraries FIRMWARE/Lib \
  FIRMWARE/JoltikV2_web_controller/
```

Libraries in `FIRMWARE/Lib/` must be accessible to the build system — pass `--libraries FIRMWARE/Lib` or copy/symlink into your Arduino libraries folder.

### Zephyr RTOS

```bash
cd FIRMWARE/joltik_v2_zephyr
west build -b rp2040_zero -p always
west flash          # or copy build/zephyr/zephyr.uf2 to the RP2040 in bootloader mode
```

## V2 Competition Firmware

- **State machine**: search (spin) -> attack (track & ram) -> backoff (on edge)
- **4 start tactics**: straight charge, reverse start, left sweep, right sweep
- **WS2812 LED status**: white = waiting, green = searching, blue = attacking, red = edge
- **Button**: single click = start/stop, double click = cycle tactics
- **IR remote** (RC5): tactic selection and start/stop (processed on RP2040 second core)
- **Servo**: pin 9, plow deployment
- **Optional OLED**: `#define USE_DISPLAY` — sensor states, tactic, IR commands
- **Serial debug**: `#define DEBUG_ENABLE` (115200 baud)

## Zephyr Firmware

Modular rewrite of the V2 competition firmware on Zephyr RTOS:

- Multi-threaded: IR decoding thread + main combat loop
- Persistent settings via NVS (flash storage)
- USB CDC-ACM console with shell commands for debug
- Zephyr logging subsystem
- Same state machine and tactics as the Arduino version

## Web Controller (ESP8266)

WiFi dashboard for the V2 robot via a companion Wemos D1 Mini:

- **AP**: SSID `Joltik-V2`, password `joltik2024`, accessible at `http://joltik.local`
- **WebSocket**: real-time sensor telemetry and motor control
- **Dashboard**: live robot visualization, motor sliders, D-pad, line sensor graphs, config panel, debug log
- **Serial link**: 9600 baud to RP2040-Zero (TX/RX on D5/D6)

## Pin Mapping (V2)

| Function | Pin |
|---|---|
| Motor A | GPIO 5, 6 |
| Motor B | GPIO 7, 8 |
| Opponent FL / FR | GPIO 12, 13 |
| Opponent L / R | GPIO 10, 11 |
| Line L / R | A3, A2 |
| Servo | GPIO 9 |
| Button | GPIO 15 |
| IR Receiver | GPIO 14 |
| WS2812 LED | GPIO 16 |
| OLED SDA / SCL | GPIO 2, 3 |

## License

[Apache 2.0](LICENSE) — 2026 Danila Surok
