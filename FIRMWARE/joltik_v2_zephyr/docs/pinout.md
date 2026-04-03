# Pin Mapping

RP2040-Zero pinout for the Joltik V2 sumo board. Defined in `boards/rp2040_zero.overlay`.

## GPIO Assignment

| Function | GPIO | Type | Notes |
|---|---|---|---|
| Motor L speed | 5 | PWM (slice 2B) | 50 Hz, duty 0-100% |
| Motor L direction | 6 | Output | 0 = forward, 1 = reverse |
| Motor R direction | 7 | Output | 0 = forward, 1 = reverse |
| Motor R speed | 8 | PWM (slice 4A) | 50 Hz, duty 0-100% |
| Servo | 9 | PWM (slice 4B) | 50 Hz, 500-2500 us pulse |
| Opponent side-left | 10 | Input, active-low | IR distance sensor |
| Opponent side-right | 11 | Input, active-low | IR distance sensor |
| Opponent front-left | 12 | Input, active-low | IR distance sensor |
| Opponent front-right | 13 | Input, active-low | IR distance sensor |
| IR receiver | 14 | Input, active-low | RC5 protocol |
| Button | 15 | Input, active-high | 50ms debounce |
| WS2812 LED | 16 | LED strip (PIO) | Single RGB pixel |
| Line sensor right | 28 (A2) | ADC channel 2 | 12-bit, 0-4095 |
| Line sensor left | 29 (A3) | ADC channel 3 | 12-bit, 0-4095 |

## PWM Notes

- GPIO 8 (motor R) and GPIO 9 (servo) share **PWM slice 4** — both run at 50 Hz
- GPIO 5 (motor L) uses **PWM slice 2**
- Motor PWM period: 20 ms (50 Hz)
- Servo pulse: 500 us (0 deg) to 2500 us (180 deg)

## USB

- CDC ACM UART on USB for shell console and logging (115200 baud)

## Flash Layout

| Region | Offset | Size | Purpose |
|---|---|---|---|
| Code | 0x100 | ~1984 KB | Application firmware |
| NVS storage | 0x1F0000 | 64 KB | Persistent settings |

## I2C (optional OLED)

Enabled with `CONFIG_JOLTIK_DISPLAY=y`:

| Function | GPIO |
|---|---|
| SDA | 2 |
| SCL | 3 |

Display: SSD1306 128x32, I2C address 0x3C.
