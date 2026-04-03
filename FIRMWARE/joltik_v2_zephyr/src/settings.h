#ifndef JOLTIK_SETTINGS_H
#define JOLTIK_SETTINGS_H

#include <stdint.h>

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
