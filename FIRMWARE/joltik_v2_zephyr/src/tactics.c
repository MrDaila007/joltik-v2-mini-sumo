#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "tactics.h"
#include "motors.h"
#include "sensors.h"
#include "led.h"
#include "settings.h"

LOG_MODULE_REGISTER(tactics, LOG_LEVEL_INF);

/* Drive with periodic line sensor check. Returns false if edge detected. */
static bool drive_safe(int left, int right, int duration_ms)
{
	drive(left, right);
	int64_t deadline = k_uptime_get() + duration_ms;

	while (k_uptime_get() < deadline) {
		if (!line_on_ring_left() || !line_on_ring_right()) {
			motors_stop();
			return false;
		}
		k_sleep(K_MSEC(5));
	}
	return true;
}

static bool start_routine(void)
{
	led_set_state(STATE_SEARCHING);

	/* Initial charge forward */
	if (!drive_safe(g_settings.straight_speed, g_settings.straight_speed, 150)) {
		return false;
	}

	motors_stop();
	k_msleep(50);

	/* Briefly search for opponent */
	int64_t deadline = k_uptime_get() + 200;
	while (!sensor_front()) {
		if (k_uptime_get() >= deadline) {
			break;
		}
		if (!line_on_ring_left() || !line_on_ring_right()) {
			motors_stop();
			return false;
		}
		k_sleep(K_MSEC(1));
	}
	return true;
}

void tactics_execute(uint8_t tactic_id)
{
	LOG_INF("Executing tactic %u", tactic_id);

	switch (tactic_id) {
	case 0: /* Default: straight charge */
		start_routine();
		break;

	case 1: /* Reverse start */
		if (!drive_safe(-g_settings.max_speed + 10, -g_settings.max_speed, 400)) {
			break;
		}
		if (!drive_safe(-g_settings.rotate_speed, g_settings.rotate_speed, 150)) {
			break;
		}
		motors_stop();
		k_msleep(50);
		start_routine();
		break;

	case 2: /* Left sweep */
		if (!drive_safe(-g_settings.search_speed, g_settings.search_speed, 300)) {
			break;
		}
		start_routine();
		break;

	case 3: /* Right sweep */
		if (!drive_safe(g_settings.search_speed, -g_settings.search_speed, 300)) {
			break;
		}
		start_routine();
		break;

	default:
		start_routine();
		break;
	}
}
