# Architecture

## Modules

```
main.c ─────── State machine, init, main loop
  ├── motors     PWM drive control (-255..255)
  ├── sensors    4x opponent GPIO + 2x line ADC
  ├── combat     attack() / backoff() / search()
  ├── tactics    4 start routines
  ├── led        WS2812 color by state
  ├── servo      Plow angle (0-180)
  ├── button     Single/double click detection
  ├── rc5        RC5 IR decoder (separate thread)
  ├── settings   NVS flash persistence
  ├── shell_cmds Debug commands over USB
  └── display    Optional SSD1306 OLED
```

## Threads

| Thread | Stack | Priority | Role |
|---|---|---|---|
| **main** | 4096 B | 0 (preemptive) | State machine, combat loop |
| **ir_thread** | 1024 B | 5 (cooperative) | RC5 message dispatch (tactic select / power toggle) |
| **system workqueue** | 2048 B | — | Deferred LED updates, display refresh |

## State Machine

```
                 ┌──────────┐
        power on │ WAITING  │ LED: white
                 │ (idle)   │ Button/IR/start_pin triggers start
                 └────┬─────┘
                      │ start event
                 ┌────▼─────┐
                 │COUNTDOWN │ LED: green, 5 sec (1s per count)
                 └────┬─────┘
                      │
                 ┌────▼─────┐
                 │ TACTIC   │ LED: yellow, executes selected tactic (0-3)
                 └────┬─────┘
                      │
              ┌───────▼────────┐
              │  COMBAT LOOP   │
              │                │
              │  ┌──────────┐  │
          no  │  │ SEARCH   │  │ LED: green
       opponent  │ (spin)   │  │ Rotate in searchDir
              │  └─────┬────┘  │
              │   opponent     │
              │   detected     │
              │  ┌─────▼────┐  │
              │  │ ATTACK   │  │ LED: blue
              │  │ (track)  │  │ Priority-based targeting
              │  └─────┬────┘  │
              │   edge         │
              │   detected     │
              │  ┌─────▼────┐  │
              │  │ BACKOFF  │  │ LED: red
              │  │ (escape) │  │ Reverse → rotate → forward
              │  └──────────┘  │
              └───────┬────────┘
                      │ button press
                 ┌────▼─────┐
                 │ WAITING  │ motors stop
                 └──────────┘
```

## Attack Priority

When opponent is detected, `attack()` checks sensors in this order:

1. **All 4 sensors** — full ram at `max_speed` + ramp counter (accelerating)
2. **Both front** (FL + FR) — straight at `attack_speed`
3. **Front-left** — slight left turn
4. **Front-right** — slight right turn
5. **Side-left only** — tank rotate left (200ms timeout)
6. **Side-right only** — tank rotate right (200ms timeout)

## Start Tactics

| ID | Name | Behavior |
|---|---|---|
| 0 | Straight charge | Forward 150ms, then search for 200ms |
| 1 | Reverse start | Reverse 400ms, rotate 150ms, then tactic 0 |
| 2 | Left sweep | Rotate left 300ms, then tactic 0 |
| 3 | Right sweep | Rotate right 300ms, then tactic 0 |

## Edge Detection

Line sensors (ADC) are checked every loop iteration:
- `line_read_left()` / `line_read_right()` return 12-bit ADC values (0-4095)
- Value **above** threshold = on ring (black surface)
- Value **at or below** threshold = ring edge (white line)
- Left edge detected → `backoff(DIR_RIGHT)`
- Right edge detected → `backoff(DIR_LEFT)`
- Default threshold: **2000** (configurable via shell or NVS)

## Motor Control

`drive(left, right)` accepts values from -255 to 255:
- Positive = forward, negative = reverse
- Direction pin set to 0 (forward) or 1 (reverse)
- PWM duty: `pulse = period * abs(speed) / 255`
- Both motors share 50Hz PWM (20ms period)
