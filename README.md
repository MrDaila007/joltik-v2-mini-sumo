# Joltik V2 — Mini Sumo Robot

Mini sumo competition robot with SolidWorks CAD designs and Arduino firmware for two hardware revisions.

## Hardware

- **V1 Board** — ATmega328 (Arduino Nano), 3x SharpIR analog + 2x digital opponent sensors, 1x analog edge sensor, TA6586 dual H-bridge
- **V2 Board** — RP2040-Zero, 4x digital opponent sensors, 2x analog line sensors, TA6586 dual H-bridge, WS2812 RGB LED, RC5 IR remote, servo, optional OLED display

## Repository Structure

```
CAD/            SolidWorks parts, assemblies, STL/STEP exports, DXF cut files
FIRMWARE/
  JoltikV1_last_test_firmware/   Competition firmware (V1, ATmega328)
  JoltikV2_sumo_firmware/        Competition firmware (V2, RP2040-Zero)
  Joltik_v2_sumo_board_test/     Hardware test sketch (V2)
  Lib/                           Vendored Arduino libraries
```

## Build & Upload

Requires [Arduino IDE](https://www.arduino.cc/en/software) or [arduino-cli](https://arduino.github.io/arduino-cli/).

```bash
# V1 (Arduino Nano)
arduino-cli compile --fqbn arduino:avr:nano FIRMWARE/JoltikV1_last_test_firmware/
arduino-cli upload  --fqbn arduino:avr:nano -p COM_PORT FIRMWARE/JoltikV1_last_test_firmware/

# V2 (RP2040-Zero)
arduino-cli compile --fqbn rp2040:rp2040:waveshare_rp2040_zero --libraries FIRMWARE/Lib FIRMWARE/JoltikV2_sumo_firmware/
arduino-cli upload  --fqbn rp2040:rp2040:waveshare_rp2040_zero -p COM_PORT FIRMWARE/JoltikV2_sumo_firmware/
```

Libraries in `FIRMWARE/Lib/` must be accessible to the build system — either pass `--libraries FIRMWARE/Lib` or copy/symlink them into your Arduino libraries folder.

## V2 Firmware Features

- **4 digital opponent sensors** (front-left, front-right, side-left, side-right) — active-low
- **2 analog line sensors** — always active, detects ring edge on both sides
- **State machine**: search (spin) → attack (track & ram) → backoff (on edge detection)
- **WS2812 LED status**: white = waiting, green = searching, blue = attacking, red = edge detected
- **Button control**: single click = start/stop, double click = cycle tactics
- **4 start tactics**: straight charge, reverse start, left sweep, right sweep
- **IR remote** (RC5): tactic selection and start/stop, processed on RP2040 second core
- **Servo**: attached on pin 9 for plow deployment
- **Optional OLED**: enable with `#define USE_DISPLAY` — shows sensor states, tactic, and IR commands
- **Serial debug**: enable with `#define DEBUG_ENABLE`

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
