#ifndef JOLTIK_LED_H
#define JOLTIK_LED_H

#include <stdint.h>

enum robot_state {
	STATE_WAITING,
	STATE_SEARCHING,
	STATE_ATTACKING,
	STATE_BACKOFF,
	STATE_TACTIC_SELECT,
	STATE_COUNTDOWN,
};

int led_init(void);
void led_set_color(uint8_t r, uint8_t g, uint8_t b);
void led_set_state(enum robot_state state);
void led_off(void);

#endif
