# Testing

Unit tests run on `native_sim` (Zephyr's hardware emulation) using the ztest framework. No physical hardware needed.

## Running Tests

```bash
make test
```

This builds and executes all test suites. Expected output:

```
START - test_rc5
 PASS - test_rc5_reset
 PASS - test_rc5_invalid_timing
 ...
START - test_motors
 PASS - test_drive_forward
 PASS - test_drive_reverse
 ...
PROJECT EXECUTION SUCCESSFUL
```

## Test Suites

| Suite | File | What it tests |
|---|---|---|
| **test_rc5** | `test_rc5.c` | RC5 decoder state machine: reset, timing validation, short/long pulses, full frame decode |
| **test_motors** | `test_motors.c` | Motor PWM: forward, reverse, turns, stop, value clamping |
| **test_sensors** | `test_sensors.c` | Opponent GPIO read (all 4 + combined), line ADC read, threshold logic |
| **test_servo** | `test_servo.c` | Servo angle: init, 0/90/180 degrees, clamping, sweep |
| **test_settings** | `test_settings.c` | NVS persistence: default values, save/reload cycle, all speed fields |
| **test_combat** | `test_combat.c` | Combat logic: search direction, attack priorities, edge backoff |
| **test_led** | `test_led.c` | LED state-to-color mapping, custom color, update counting |

## Hardware Emulation

Tests use Zephyr emulation drivers defined in `tests/boards/native_sim.overlay`:

- **gpio_emul** — 30-pin virtual GPIO (opponent sensors, motor direction, button)
- **fake-pwm** — Virtual PWM (motor speed, servo)
- **adc_emul** — Virtual ADC with injectable values (line sensors)
- **fake_led_strip.c** — Custom LED driver that stores pixel data and counts updates

Tests inject sensor values via `gpio_emul_input_set()` and `adc_emul_const_value_set()`, then verify motor output via `gpio_emul_output_get()`.

## Adding Tests

1. Create `tests/src/test_<module>.c`
2. Use `ZTEST_SUITE()` and `ZTEST()` macros
3. Add source to `tests/CMakeLists.txt`
4. Run `make test`

Example:

```c
#include <zephyr/ztest.h>

ZTEST_SUITE(test_mymodule, NULL, NULL, NULL, NULL, NULL);

ZTEST(test_mymodule, test_something)
{
    zassert_equal(1 + 1, 2);
}
```
