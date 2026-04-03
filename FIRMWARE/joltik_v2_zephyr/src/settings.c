#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>

#include "settings.h"
#include "sensors.h"

LOG_MODULE_REGISTER(app_settings, LOG_LEVEL_INF);

struct robot_settings g_settings = {
	.current_tactic = 0,
	.line_threshold = 2000,
	.max_speed = 200,
	.straight_speed = 125,
	.search_speed = 100,
	.rotate_speed = 125,
	.breakout_speed = 160,
	.attack_speed = 125,
};

static int settings_set(const char *name, size_t len, settings_read_cb read_cb,
			void *cb_arg)
{
	const char *next;
	int rc;

	if (settings_name_steq(name, "tactic", &next) && !next) {
		rc = read_cb(cb_arg, &g_settings.current_tactic,
			     sizeof(g_settings.current_tactic));
		return (rc >= 0) ? 0 : rc;
	}
	if (settings_name_steq(name, "threshold", &next) && !next) {
		rc = read_cb(cb_arg, &g_settings.line_threshold,
			     sizeof(g_settings.line_threshold));
		if (rc >= 0) {
			line_set_threshold(g_settings.line_threshold);
		}
		return (rc >= 0) ? 0 : rc;
	}
	if (settings_name_steq(name, "max_spd", &next) && !next) {
		rc = read_cb(cb_arg, &g_settings.max_speed,
			     sizeof(g_settings.max_speed));
		return (rc >= 0) ? 0 : rc;
	}
	if (settings_name_steq(name, "str_spd", &next) && !next) {
		rc = read_cb(cb_arg, &g_settings.straight_speed,
			     sizeof(g_settings.straight_speed));
		return (rc >= 0) ? 0 : rc;
	}
	if (settings_name_steq(name, "srch_spd", &next) && !next) {
		rc = read_cb(cb_arg, &g_settings.search_speed,
			     sizeof(g_settings.search_speed));
		return (rc >= 0) ? 0 : rc;
	}
	if (settings_name_steq(name, "rot_spd", &next) && !next) {
		rc = read_cb(cb_arg, &g_settings.rotate_speed,
			     sizeof(g_settings.rotate_speed));
		return (rc >= 0) ? 0 : rc;
	}
	if (settings_name_steq(name, "brk_spd", &next) && !next) {
		rc = read_cb(cb_arg, &g_settings.breakout_speed,
			     sizeof(g_settings.breakout_speed));
		return (rc >= 0) ? 0 : rc;
	}
	if (settings_name_steq(name, "atk_spd", &next) && !next) {
		rc = read_cb(cb_arg, &g_settings.attack_speed,
			     sizeof(g_settings.attack_speed));
		return (rc >= 0) ? 0 : rc;
	}

	return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(joltik, "joltik", NULL, settings_set, NULL, NULL);

int app_settings_init(void)
{
	int ret;

	ret = settings_subsys_init();
	if (ret) {
		LOG_ERR("Settings init failed: %d", ret);
		return ret;
	}

	ret = settings_load();
	if (ret) {
		LOG_ERR("Settings load failed: %d", ret);
		return ret;
	}

	/* Apply loaded threshold to sensors module */
	line_set_threshold(g_settings.line_threshold);

	LOG_INF("Settings loaded: tactic=%u threshold=%u speeds=%u/%u/%u/%u/%u/%u",
		g_settings.current_tactic, g_settings.line_threshold,
		g_settings.max_speed, g_settings.straight_speed,
		g_settings.search_speed, g_settings.rotate_speed,
		g_settings.breakout_speed, g_settings.attack_speed);

	return 0;
}

int app_settings_save(void)
{
	int ret = 0;

	ret |= settings_save_one("joltik/tactic", &g_settings.current_tactic,
				  sizeof(g_settings.current_tactic));
	ret |= settings_save_one("joltik/threshold", &g_settings.line_threshold,
				  sizeof(g_settings.line_threshold));
	ret |= settings_save_one("joltik/max_spd", &g_settings.max_speed,
				  sizeof(g_settings.max_speed));
	ret |= settings_save_one("joltik/str_spd", &g_settings.straight_speed,
				  sizeof(g_settings.straight_speed));
	ret |= settings_save_one("joltik/srch_spd", &g_settings.search_speed,
				  sizeof(g_settings.search_speed));
	ret |= settings_save_one("joltik/rot_spd", &g_settings.rotate_speed,
				  sizeof(g_settings.rotate_speed));
	ret |= settings_save_one("joltik/brk_spd", &g_settings.breakout_speed,
				  sizeof(g_settings.breakout_speed));
	ret |= settings_save_one("joltik/atk_spd", &g_settings.attack_speed,
				  sizeof(g_settings.attack_speed));

	if (ret) {
		LOG_ERR("Settings save failed");
	} else {
		LOG_INF("Settings saved to flash");
	}

	return ret;
}
