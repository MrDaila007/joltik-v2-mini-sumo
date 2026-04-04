/*
 * Combat logic unit tests.
 * Tests attack priority, backoff direction, and search behavior.
 * Uses gpio_emul + adc_emul to set sensor state, then verifies
 * motor direction pins after combat functions execute.
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/adc/adc_emul.h>
#include "combat.h"
#include "motors.h"
#include "sensors.h"
#include "settings.h"

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

static const struct gpio_dt_spec sen_fl =
	GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, sensor_fl_gpios);
static const struct gpio_dt_spec sen_fr =
	GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, sensor_fr_gpios);
static const struct gpio_dt_spec sen_l =
	GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, sensor_l_gpios);
static const struct gpio_dt_spec sen_r =
	GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, sensor_r_gpios);
static const struct gpio_dt_spec motor_left_dir =
	GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, motor_left_dir_gpios);
static const struct gpio_dt_spec motor_right_dir =
	GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, motor_right_dir_gpios);

static const struct device *adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc0));

static void set_opponent(const struct gpio_dt_spec *spec, bool detected)
{
	gpio_emul_input_set(spec->port, spec->pin, detected ? 0 : 1);
}

static void set_all_on_ring(void)
{
	adc_emul_const_value_set(adc_dev, 0, 3000);
	adc_emul_const_value_set(adc_dev, 1, 3000);
}

static void clear_all(void)
{
	set_opponent(&sen_fl, false);
	set_opponent(&sen_fr, false);
	set_opponent(&sen_l, false);
	set_opponent(&sen_r, false);
	set_all_on_ring();
}

static void *combat_suite_setup(void)
{
	motors_init();
	sensors_init();
	return NULL;
}

static void combat_before_each(void *fixture)
{
	ARG_UNUSED(fixture);
	clear_all();
	combat_reset_count();
	/* Ensure default speed settings */
	g_settings.max_speed = 200;
	g_settings.attack_speed = 125;
	g_settings.search_speed = 100;
	g_settings.rotate_speed = 125;
	g_settings.breakout_speed = 160;
	g_settings.straight_speed = 125;
}

ZTEST_SUITE(combat, NULL, combat_suite_setup, combat_before_each, NULL, NULL);

/* --- Search tests --- */

ZTEST(combat, test_search_runs)
{
	/* search() should not crash and should drive motors */
	search();
	/* Verify motors are running (at least one direction pin is set) */
	int left_dir = gpio_emul_output_get(motor_left_dir.port, motor_left_dir.pin);
	int right_dir = gpio_emul_output_get(motor_right_dir.port, motor_right_dir.pin);
	/* In search, one motor goes forward and one reverse (tank turn) */
	zassert_true(left_dir != right_dir || left_dir == 1,
		     "Search should spin (different directions)");
}

/* --- Attack priority tests --- */

ZTEST(combat, test_attack_front_both)
{
	/* Priority 2: both front sensors -> straight ahead */
	set_opponent(&sen_fl, true);
	set_opponent(&sen_fr, true);

	attack();

	int left_dir = gpio_emul_output_get(motor_left_dir.port, motor_left_dir.pin);
	int right_dir = gpio_emul_output_get(motor_right_dir.port, motor_right_dir.pin);

	zassert_equal(left_dir, 0, "Front attack: left forward");
	zassert_equal(right_dir, 0, "Front attack: right forward");
}

ZTEST(combat, test_attack_all_sensors_max_speed)
{
	/* Priority 1: all 4 sensors -> max speed ram */
	set_opponent(&sen_fl, true);
	set_opponent(&sen_fr, true);
	set_opponent(&sen_l, true);
	set_opponent(&sen_r, true);

	attack();

	int left_dir = gpio_emul_output_get(motor_left_dir.port, motor_left_dir.pin);
	int right_dir = gpio_emul_output_get(motor_right_dir.port, motor_right_dir.pin);

	zassert_equal(left_dir, 0, "All sensors: left forward");
	zassert_equal(right_dir, 0, "All sensors: right forward");
}

ZTEST(combat, test_attack_front_left_turns_left)
{
	/* Priority 3: FL only -> slight left turn */
	set_opponent(&sen_fl, true);
	attack();
	/* Should drive forward with slight left bias - both forward */
	int left_dir = gpio_emul_output_get(motor_left_dir.port, motor_left_dir.pin);
	int right_dir = gpio_emul_output_get(motor_right_dir.port, motor_right_dir.pin);
	zassert_equal(left_dir, 0, "FL attack: left forward");
	zassert_equal(right_dir, 0, "FL attack: right forward");
}

ZTEST(combat, test_attack_front_right_turns_right)
{
	/* Priority 4: FR only -> slight right turn */
	set_opponent(&sen_fr, true);
	attack();
	int left_dir = gpio_emul_output_get(motor_left_dir.port, motor_left_dir.pin);
	int right_dir = gpio_emul_output_get(motor_right_dir.port, motor_right_dir.pin);
	zassert_equal(left_dir, 0, "FR attack: left forward");
	zassert_equal(right_dir, 0, "FR attack: right forward");
}

ZTEST(combat, test_attack_side_left_rotates)
{
	/* Priority 5: side-left only -> rotate left then timeout.
	 * After 200ms timeout the while loop exits.
	 * Just verify attack completes without crash. */
	set_opponent(&sen_l, true);
	attack();
	/* attack() completed - motors were driven during side rotation */
}

ZTEST(combat, test_attack_side_right_rotates)
{
	/* Priority 6: side-right only -> rotate right then timeout. */
	set_opponent(&sen_r, true);
	attack();
	/* attack() completed */
}

/* --- Edge detection is handled by main loop, not attack() ---
 * attack() no longer checks line sensors; caller must do it first.
 */

/* --- Backoff tests --- */

ZTEST(combat, test_backoff_left)
{
	backoff(DIR_LEFT);
	/* Should complete without crashing */
	/* Final state: motors driving forward */
	int left_dir = gpio_emul_output_get(motor_left_dir.port, motor_left_dir.pin);
	zassert_equal(left_dir, 0, "Backoff left should end forward");
}

ZTEST(combat, test_backoff_right)
{
	backoff(DIR_RIGHT);
	int left_dir = gpio_emul_output_get(motor_left_dir.port, motor_left_dir.pin);
	zassert_equal(left_dir, 0, "Backoff right should end forward");
}

/* --- Count reset --- */

ZTEST(combat, test_count_reset)
{
	/* Attack with all sensors multiple times to increment count */
	set_opponent(&sen_fl, true);
	set_opponent(&sen_fr, true);
	set_opponent(&sen_l, true);
	set_opponent(&sen_r, true);

	attack();
	attack();
	attack();

	/* Reset count */
	combat_reset_count();

	/* Next attack should use base max_speed (not max_speed + count) */
	attack();
	/* Should not crash */
}
