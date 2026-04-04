#ifndef JOLTIK_SETTINGS_H
#define JOLTIK_SETTINGS_H

#include <stdint.h>

/*
 * Concurrency: g_settings is read from main thread (combat/tactics),
 * written from shell thread (config commands), and read from system
 * workqueue (display). On Cortex-M0+, uint8_t and aligned uint16_t
 * reads/writes are atomic (single LDR/STR instruction).
 * current_tactic at runtime is managed via atomic_t in main.c;
 * g_settings.current_tactic is only used for NVS persistence.
 */
struct robot_settings {
	uint8_t  current_tactic;
	uint16_t line_threshold;
	uint8_t  max_speed;
	uint8_t  straight_speed;
	uint8_t  search_speed;
	uint8_t  rotate_speed;
	uint8_t  breakout_speed;
	uint8_t  attack_speed;
};

extern struct robot_settings g_settings;

int app_settings_init(void);
int app_settings_save(void);

#endif
