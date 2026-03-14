# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Joltik V2 is a mini sumo robot project containing both CAD designs (SolidWorks) and Arduino firmware. The robot uses IR distance sensors and edge sensors to detect opponents and ring boundaries, with dual-motor tank-style drive for movement.

## Repository Structure

- **CAD/** — SolidWorks parts, assemblies, and DXF cut files for the physical robot chassis
- **FIRMWARE/** — Arduino sketches and bundled libraries
  - `JoltikV1_last_test_firmware/` — Main competition firmware (ATmega-based, V1 board). Implements search/attack/backoff state machine with SharpIR and digital opponent sensors, TA6586 dual H-bridge motor driver, optional GyverMotor2 library support.
  - `Joltik_sumo_board_test/` — Board test firmware (RP2040-Zero, V2 board). Tests motors, digital/analog sensors, WS2812 LED, IR remote (RC5), optional OLED display. Uses second core (`loop1()`) for IR processing.
  - `Lib/` — Vendored Arduino libraries: Adafruit_NeoPixel, GyverMotor, RC5, SharpIR

## Build & Upload

Firmware is standard Arduino `.ino` sketches. Open in Arduino IDE or use `arduino-cli`:

```bash
# Compile (adjust board FQBN as needed)
arduino-cli compile --fqbn arduino:avr:nano FIRMWARE/JoltikV1_last_test_firmware/
arduino-cli compile --fqbn rp2040:rp2040:waveshare_rp2040_zero FIRMWARE/Joltik_sumo_board_test/

# Upload
arduino-cli upload -p COM_PORT --fqbn arduino:avr:nano FIRMWARE/JoltikV1_last_test_firmware/
```

Libraries in `FIRMWARE/Lib/` must be available to the Arduino build system (symlink or copy into the Arduino libraries folder, or use `--libraries FIRMWARE/Lib`).

## Key Architecture Notes

- **Two distinct hardware targets**: V1 firmware targets ATmega328 (Arduino Nano); V2 board test targets RP2040-Zero. Pin mappings differ completely between them.
- **Motor control**: Both firmwares use the same `drive(left, right)` function signature with PWM values -255 to 255 via 2-wire H-bridge (TA6586). V1 has optional GyverMotor2 support toggled by `UseGyverMotor` define.
- **Sensor architecture**: V1 uses 3 front SharpIR analog distance sensors + 2 side digital opponent sensors + analog edge sensor. V2 board test uses 4 digital opponent sensors + 2 analog line sensors.
- **Competition flow** (V1): Waits for start module signal or button press → runs `startRoutine()` (initial charge) → main loop alternates between `search()` (spinning to find opponent) and `attack()` (tracking/ramming) with `backoff()` on edge detection.
- **Debug mode**: V1 firmware uses `#define DEBUG_ENABLE` to toggle Serial output of sensor states.
- **Comments**: Some code comments are in Russian.
