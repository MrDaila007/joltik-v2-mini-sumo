#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "combat.h"
#include "motors.h"
#include "sensors.h"
#include "led.h"
#include "settings.h"

LOG_MODULE_REGISTER(combat, LOG_LEVEL_DBG);

static int count;
static uint8_t search_dir = DIR_LEFT;

void combat_reset_count(void)
{
	count = 0;
}

void search(void)
{
	led_set_state(STATE_SEARCHING);

	if (search_dir == DIR_LEFT) {
		drive(-g_settings.search_speed, g_settings.search_speed);
		LOG_DBG("Search left");
	} else {
		drive(g_settings.search_speed, -g_settings.search_speed);
		LOG_DBG("Search right");
	}
}

void backoff(uint8_t dir)
{
	led_set_state(STATE_BACKOFF);

	/* Reverse */
	drive(-g_settings.breakout_speed, -g_settings.breakout_speed);
	LOG_DBG("Reverse");
	k_msleep(200);

	/* Brief stop */
	motors_stop();
	k_msleep(50);

	/* Rotate away from edge */
	if (dir == DIR_LEFT) {
		drive(-g_settings.rotate_speed, g_settings.rotate_speed);
		LOG_DBG("Backoff left");
	} else {
		drive(g_settings.rotate_speed, -g_settings.rotate_speed);
		LOG_DBG("Backoff right");
	}
	k_msleep(100);

	/* Look for opponent during rotation */
	int64_t deadline = k_uptime_get() + 200;
	while (k_uptime_get() < deadline) {
		if (opponent_detected()) {
			motors_stop();
			k_msleep(50);
			LOG_DBG("Backoff: opponent found");
			return;
		}
		k_yield();
	}

	/* No opponent found, move forward */
	drive(g_settings.straight_speed, g_settings.straight_speed);
	k_msleep(100);
}

void attack(void)
{
	led_set_state(STATE_ATTACKING);
	int64_t attack_start = k_uptime_get();

	/* Check line sensors first */
	bool line_l = line_on_ring_left();
	bool line_r = line_on_ring_right();

	if (!line_l && !line_r) {
		backoff(DIR_RIGHT);
		return;
	}
	if (!line_l) {
		backoff(DIR_RIGHT);
		return;
	}
	if (!line_r) {
		backoff(DIR_LEFT);
		return;
	}

	/* Priority 1: All four sensors - full speed ram */
	if (sensor_front() && sensor_l() && sensor_r()) {
		int spd = g_settings.max_speed + count;
		drive(spd, spd);
		count++;
		LOG_DBG("Attack ALL max speed");
	}
	/* Priority 2: Both front sensors - straight attack */
	else if (sensor_front()) {
		drive(g_settings.attack_speed, g_settings.attack_speed);
		LOG_DBG("Attack front");
	}
	/* Priority 3: Front-left only - slight left turn */
	else if (sensor_fl()) {
		search_dir = DIR_LEFT;
		drive(g_settings.attack_speed - 30, g_settings.attack_speed + 10);
		LOG_DBG("Attack front-left");
	}
	/* Priority 4: Front-right only - slight right turn */
	else if (sensor_fr()) {
		search_dir = DIR_RIGHT;
		drive(g_settings.attack_speed + 10, g_settings.attack_speed - 30);
		LOG_DBG("Attack front-right");
	}
	/* Priority 5: Side-left only - rotate left */
	else if (sensor_l()) {
		search_dir = DIR_LEFT;
		drive(-g_settings.rotate_speed, g_settings.rotate_speed);
		LOG_DBG("Attack side-left");
		while (!sensor_front()) {
			if (k_uptime_get() - attack_start > 200) {
				break;
			}
			k_yield();
		}
	}
	/* Priority 6: Side-right only - rotate right */
	else if (sensor_r()) {
		search_dir = DIR_RIGHT;
		drive(g_settings.rotate_speed, -g_settings.rotate_speed);
		LOG_DBG("Attack side-right");
		while (!sensor_front()) {
			if (k_uptime_get() - attack_start > 200) {
				break;
			}
			k_yield();
		}
	}
}
