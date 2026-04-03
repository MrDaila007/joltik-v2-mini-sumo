# Joltik V2 Zephyr Firmware

Zephyr RTOS firmware for the Joltik V2 mini sumo robot on RP2040-Zero.

Modular rewrite of the Arduino competition firmware with multi-threading, persistent settings (NVS), interactive shell over USB, and a full unit test suite.

## Quick Start

```bash
# Build
make

# Flash (hold BOOTSEL, plug USB, release)
make flash

# Serial console (shell + logs)
make monitor

# Build with OLED display support
make display
```

## Features

- **Combat state machine** — search / attack / backoff with 4 configurable start tactics
- **4 opponent sensors** (IR, active-low) + **2 line sensors** (ADC)
- **WS2812 LED** — color-coded robot state
- **RC5 IR remote** — tactic selection and start/stop (dedicated thread)
- **Servo** — plow deployment (0-180 degrees)
- **Button** — single click start/stop, double click cycle tactics
- **Persistent settings** — speeds, tactic, line threshold saved to flash (NVS)
- **USB shell** — live sensor readout, motor test, config tuning (115200 baud)
- **Optional OLED** — SSD1306 128x32 via I2C (`CONFIG_JOLTIK_DISPLAY=y`)
- **Unit tests** — 7 test suites on native_sim

## Build Requirements

- Zephyr SDK 0.17+ with ARM toolchain
- West workspace with Zephyr v4.3
- Python venv with west, pyelftools

Paths are configured in `Makefile` — adjust `ZEPHYR_BASE`, `ZEPHYR_SDK`, and `VENV` to match your setup.

## Make Targets

| Target | Description |
|---|---|
| `make` / `make build` | Build firmware |
| `make flash` | Copy UF2 to RP2040 in bootloader mode |
| `make monitor` | Open USB serial console |
| `make flash-monitor` | Flash then monitor |
| `make display` | Build with OLED enabled |
| `make test` | Build and run unit tests |
| `make size` | Show firmware size |
| `make rom_report` / `make ram_report` | Memory usage |
| `make menuconfig` | Interactive Kconfig |
| `make clean` / `make pristine` | Clean build |

## Project Structure

```
├── CMakeLists.txt
├── Kconfig                     # Optional features (display, IR)
├── prj.conf                    # Zephyr kernel config
├── Makefile                    # Build shortcuts
├── boards/
│   └── rp2040_zero.overlay     # Pin mappings (devicetree)
├── src/
│   ├── main.c                  # State machine & main loop
│   ├── motors.c/h              # PWM motor control
│   ├── sensors.c/h             # Opponent (GPIO) + line (ADC) sensors
│   ├── combat.c/h              # Attack / backoff / search logic
│   ├── tactics.c/h             # 4 start routines
│   ├── led.c/h                 # WS2812 state colors
│   ├── servo.c/h               # Servo angle control
│   ├── button.c/h              # Debounced single/double click
│   ├── rc5.c/h                 # RC5 IR protocol decoder
│   ├── settings.c/h            # NVS persistent storage
│   ├── shell_cmds.c            # Shell command handlers
│   └── display.c/h             # Optional SSD1306 OLED
└── tests/                      # ztest unit tests (native_sim)
```

## Documentation

- [Architecture](docs/architecture.md) — modules, threads, state machine
- [Pin Mapping](docs/pinout.md) — GPIO, PWM, ADC assignments
- [Shell Commands](docs/shell.md) — interactive debug interface
- [Settings](docs/settings.md) — persistent configuration via NVS
- [Testing](docs/testing.md) — unit tests and how to run them
