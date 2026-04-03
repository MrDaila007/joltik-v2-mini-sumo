# Settings

Robot parameters are stored in flash via Zephyr NVS (Non-Volatile Storage) and persist across power cycles.

## Parameters

| Parameter | Default | Range | Description |
|---|---|---|---|
| `current_tactic` | 0 | 0-3 | Start tactic (straight / reverse / left / right) |
| `line_threshold` | 2000 | 0-4095 | ADC threshold for ring edge detection |
| `max_speed` | 200 | 0-255 | Ram speed when all sensors triggered |
| `straight_speed` | 125 | 0-255 | Forward speed after tactic |
| `search_speed` | 100 | 0-255 | Rotation speed while searching |
| `rotate_speed` | 125 | 0-255 | Rotation speed during attack tracking |
| `breakout_speed` | 160 | 0-255 | Reverse/escape speed on edge |
| `attack_speed` | 125 | 0-255 | Forward speed when opponent in front |

## NVS Keys

Settings are stored under the `joltik` namespace:

| Key | Type | Mapped to |
|---|---|---|
| `joltik/tactic` | uint8 | `current_tactic` |
| `joltik/threshold` | uint16 | `line_threshold` |
| `joltik/max_spd` | uint8 | `max_speed` |
| `joltik/str_spd` | uint8 | `straight_speed` |
| `joltik/srch_spd` | uint8 | `search_speed` |
| `joltik/rot_spd` | uint8 | `rotate_speed` |
| `joltik/brk_spd` | uint8 | `breakout_speed` |
| `joltik/atk_spd` | uint8 | `attack_speed` |

## Usage

### Via Shell

```
config show                    # view all
config threshold 1800          # change threshold
config speed max 220           # change max speed
config save                    # write to flash
```

Changes take effect immediately but are **not saved to flash** until `config save` is called.

### Via Code

```c
#include "settings.h"

// Global instance
extern struct robot_settings g_settings;

// Read
uint8_t spd = g_settings.max_speed;

// Modify
g_settings.max_speed = 220;

// Persist to flash
app_settings_save();
```

### Startup Flow

1. `app_settings_init()` called from `main()`
2. NVS subsystem initialized (`settings_subsys_init`)
3. Settings loaded from flash (`settings_load`)
4. Missing keys filled with defaults
5. `g_settings` ready for use

## Flash Storage

- Partition: 64 KB at offset `0x1F0000`
- Backend: Zephyr NVS
- Wear leveling handled by NVS internally
- Write protection disabled via `CONFIG_MPU_ALLOW_FLASH_WRITE=y`
