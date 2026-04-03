#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "tactics.h"
#include "motors.h"
#include "sensors.h"
#include "led.h"
#include "settings.h"

LOG_MODULE_REGISTER(tactics, LOG_LEVEL_INF);

static void start_routine(void)
{
	led_set_state(STATE_SEARCHING);

	/* Initial charge forward */
	drive(g_settings.straight_speed, g_settings.straight_speed);
	k_msleep(150);

	motors_stop();
	k_msleep(50);

	/* Briefly search for opponent */
	int64_t deadline = k_uptime_get() + 200;
	while (!sensor_front()) {
		if (k_uptime_get() >= deadline) {
			break;
		}
		k_yield();
	}
}

void tactics_execute(uint8_t tactic_id)
{
	LOG_INF("Executing tactic %u", tactic_id);

	switch (tactic_id) {
	case 0: /* Default: straight charge */
		start_routine();
		break;

	case 1: /* Reverse start */
		drive(-g_settings.max_speed + 10, -g_settings.max_speed);
		k_msleep(400);
		drive(-g_settings.rotate_speed, g_settings.rotate_speed);
		k_msleep(150);
		motors_stop();
		k_msleep(50);
		start_routine();
		break;

	case 2: /* Left sweep */
		drive(-g_settings.search_speed, g_settings.search_speed);
		k_msleep(300);
		start_routine();
		break;

	case 3: /* Right sweep */
		drive(g_settings.search_speed, -g_settings.search_speed);
		k_msleep(300);
		start_routine();
		break;

	default:
		start_routine();
		break;
	}
}
