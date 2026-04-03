/*
 * Settings subsystem unit tests.
 * Verifies default values, save, and reload.
 */
#include <zephyr/ztest.h>
#include "settings.h"

static void *settings_suite_setup(void)
{
	zassert_ok(app_settings_init(), "Settings init failed");
	return NULL;
}

ZTEST_SUITE(settings_tests, NULL, settings_suite_setup, NULL, NULL, NULL);

ZTEST(settings_tests, test_defaults)
{
	/* Verify default values before any save/load */
	zassert_equal(g_settings.current_tactic, 0, "Default tactic should be 0");
	zassert_equal(g_settings.line_threshold, 2000, "Default threshold should be 2000");
	zassert_equal(g_settings.max_speed, 200, "Default max_speed should be 200");
	zassert_equal(g_settings.straight_speed, 125, "Default straight_speed should be 125");
	zassert_equal(g_settings.search_speed, 100, "Default search_speed should be 100");
	zassert_equal(g_settings.rotate_speed, 125, "Default rotate_speed should be 125");
	zassert_equal(g_settings.breakout_speed, 160, "Default breakout_speed should be 160");
	zassert_equal(g_settings.attack_speed, 125, "Default attack_speed should be 125");
}

ZTEST(settings_tests, test_save_and_reload)
{
	/* Modify settings */
	g_settings.current_tactic = 3;
	g_settings.line_threshold = 1500;
	g_settings.max_speed = 220;

	/* Save to NVS */
	zassert_ok(app_settings_save(), "Settings save failed");

	/* Modify in memory */
	g_settings.current_tactic = 0;
	g_settings.line_threshold = 9999;
	g_settings.max_speed = 0;

	/* Reload from NVS */
	zassert_ok(app_settings_init(), "Settings reload failed");

	/* Verify saved values were restored */
	zassert_equal(g_settings.current_tactic, 3,
		      "Tactic should be restored to 3");
	zassert_equal(g_settings.line_threshold, 1500,
		      "Threshold should be restored to 1500");
	zassert_equal(g_settings.max_speed, 220,
		      "Max speed should be restored to 220");

	/* Restore defaults for other tests */
	g_settings.current_tactic = 0;
	g_settings.line_threshold = 2000;
	g_settings.max_speed = 200;
	app_settings_save();
}

ZTEST(settings_tests, test_save_all_speeds)
{
	g_settings.straight_speed = 150;
	g_settings.search_speed = 80;
	g_settings.rotate_speed = 130;
	g_settings.breakout_speed = 180;
	g_settings.attack_speed = 140;

	zassert_ok(app_settings_save(), "Save all speeds failed");

	/* Reset in memory */
	g_settings.straight_speed = 0;
	g_settings.search_speed = 0;
	g_settings.rotate_speed = 0;
	g_settings.breakout_speed = 0;
	g_settings.attack_speed = 0;

	/* Reload */
	app_settings_init();

	zassert_equal(g_settings.straight_speed, 150, "straight_speed");
	zassert_equal(g_settings.search_speed, 80, "search_speed");
	zassert_equal(g_settings.rotate_speed, 130, "rotate_speed");
	zassert_equal(g_settings.breakout_speed, 180, "breakout_speed");
	zassert_equal(g_settings.attack_speed, 140, "attack_speed");

	/* Restore defaults */
	g_settings.straight_speed = 125;
	g_settings.search_speed = 100;
	g_settings.rotate_speed = 125;
	g_settings.breakout_speed = 160;
	g_settings.attack_speed = 125;
	app_settings_save();
}
