# Shell Commands

Interactive debug shell over USB CDC ACM (115200 baud). Connect with any serial terminal:

```bash
make monitor
# or
picocom /dev/ttyACM0 -b 115200
```

## Commands

### sensors

Read all sensor values.

```
uart:~$ sensors
Opponent: FL=1 FR=0 L=0 R=0
Line:     L=2850 R=3100  ring: L=yes R=yes
```

### motor drive / motor stop

Direct motor control for testing.

```
uart:~$ motor drive 100 100     # both forward at ~40% speed
uart:~$ motor drive -150 150    # tank rotate left
uart:~$ motor drive 0 0         # stop
uart:~$ motor stop              # same as drive 0 0
```

Values: -255 to 255 (clamped).

### tactic

Get or set the current start tactic.

```
uart:~$ tactic             # show current
Current tactic: 0
uart:~$ tactic 2           # set to left sweep
Tactic set to 2
```

Tactics: 0 = straight, 1 = reverse, 2 = left sweep, 3 = right sweep.

### config show

Display all settings.

```
uart:~$ config show
  tactic:    0
  threshold: 2000
  max_speed: 200
  straight:  125
  search:    100
  rotate:    125
  breakout:  160
  attack:    125
```

### config threshold

Set line sensor threshold (0-4095).

```
uart:~$ config threshold 1800
Line threshold set to 1800
```

Higher value = more sensitive to ring edge.

### config speed

Adjust motor speed parameters.

```
uart:~$ config speed max 220
uart:~$ config speed search 80
uart:~$ config speed attack 150
```

Valid names: `max`, `straight`, `search`, `rotate`, `breakout`, `attack`.
Valid values: 0-255.

### config save

Persist current settings to flash. They survive power cycles.

```
uart:~$ config save
Settings saved
```

### led

Set LED to a specific RGB color (for testing).

```
uart:~$ led 255 0 0        # red
uart:~$ led 0 255 0        # green
uart:~$ led 0 0 0          # off
```

### servo

Set servo angle (for testing).

```
uart:~$ servo 0             # fully retracted
uart:~$ servo 90            # center
uart:~$ servo 180           # fully extended
```
