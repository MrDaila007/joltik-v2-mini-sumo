/*
 * LED state mapping unit tests.
 * Verifies correct color for each robot state.
 * Uses the fake_led_strip driver to inspect pixel values.
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/led_strip.h>
#include "led.h"

/* Defined in fake_led_strip.c */
extern struct led_rgb fake_led_pixel;
extern int fake_led_update_count;

static void *led_suite_setup(void)
{
	zassert_ok(led_init(), "LED init failed");
	return NULL;
}

static void led_before_each(void *fixture)
{
	ARG_UNUSED(fixture);
	fake_led_update_count = 0;
}

ZTEST_SUITE(led_tests, NULL, led_suite_setup, led_before_each, NULL, NULL);

/* Helper: flush workqueue so led_work_handler runs */
static void flush_led(void)
{
	k_sleep(K_MSEC(10));
}

ZTEST(led_tests, test_state_waiting_white)
{
	led_set_state(STATE_WAITING);
	flush_led();
	zassert_equal(fake_led_pixel.r, 255, "Waiting R=255");
	zassert_equal(fake_led_pixel.g, 255, "Waiting G=255");
	zassert_equal(fake_led_pixel.b, 255, "Waiting B=255");
}

ZTEST(led_tests, test_state_searching_green)
{
	led_set_state(STATE_SEARCHING);
	flush_led();
	zassert_equal(fake_led_pixel.r, 0, "Searching R=0");
	zassert_equal(fake_led_pixel.g, 255, "Searching G=255");
	zassert_equal(fake_led_pixel.b, 0, "Searching B=0");
}

ZTEST(led_tests, test_state_attacking_blue)
{
	led_set_state(STATE_ATTACKING);
	flush_led();
	zassert_equal(fake_led_pixel.r, 0, "Attacking R=0");
	zassert_equal(fake_led_pixel.g, 0, "Attacking G=0");
	zassert_equal(fake_led_pixel.b, 255, "Attacking B=255");
}

ZTEST(led_tests, test_state_backoff_red)
{
	led_set_state(STATE_BACKOFF);
	flush_led();
	zassert_equal(fake_led_pixel.r, 255, "Backoff R=255");
	zassert_equal(fake_led_pixel.g, 0, "Backoff G=0");
	zassert_equal(fake_led_pixel.b, 0, "Backoff B=0");
}

ZTEST(led_tests, test_state_tactic_yellow)
{
	led_set_state(STATE_TACTIC_SELECT);
	flush_led();
	zassert_equal(fake_led_pixel.r, 255, "Tactic R=255");
	zassert_equal(fake_led_pixel.g, 255, "Tactic G=255");
	zassert_equal(fake_led_pixel.b, 0, "Tactic B=0");
}

ZTEST(led_tests, test_state_countdown_green)
{
	led_set_state(STATE_COUNTDOWN);
	flush_led();
	zassert_equal(fake_led_pixel.r, 0, "Countdown R=0");
	zassert_equal(fake_led_pixel.g, 255, "Countdown G=255");
	zassert_equal(fake_led_pixel.b, 0, "Countdown B=0");
}

ZTEST(led_tests, test_led_off)
{
	led_set_color(255, 255, 255);
	flush_led();
	led_off();
	flush_led();
	zassert_equal(fake_led_pixel.r, 0, "Off R=0");
	zassert_equal(fake_led_pixel.g, 0, "Off G=0");
	zassert_equal(fake_led_pixel.b, 0, "Off B=0");
}

ZTEST(led_tests, test_led_custom_color)
{
	led_set_color(128, 64, 32);
	flush_led();
	zassert_equal(fake_led_pixel.r, 128, "Custom R=128");
	zassert_equal(fake_led_pixel.g, 64, "Custom G=64");
	zassert_equal(fake_led_pixel.b, 32, "Custom B=32");
}

ZTEST(led_tests, test_led_update_count)
{
	fake_led_update_count = 0;
	led_set_color(1, 2, 3);
	flush_led();
	zassert_true(fake_led_update_count > 0, "Driver should be called");
}
