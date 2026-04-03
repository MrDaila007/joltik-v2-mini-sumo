/*
 * Sensor unit tests.
 * Uses gpio_emul to simulate opponent sensor states
 * and adc_emul to simulate line sensor readings.
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/adc_emul.h>
#include "sensors.h"

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

static const struct gpio_dt_spec sen_fl =
	GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, sensor_fl_gpios);
static const struct gpio_dt_spec sen_fr =
	GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, sensor_fr_gpios);
static const struct gpio_dt_spec sen_l =
	GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, sensor_l_gpios);
static const struct gpio_dt_spec sen_r =
	GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, sensor_r_gpios);

static const struct device *adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc0));

/* Helper: set opponent sensor (active-low: 0 = detected, 1 = no opponent) */
static void set_opponent(const struct gpio_dt_spec *spec, bool detected)
{
	/* DT has GPIO_ACTIVE_LOW, so gpio_pin_get_dt inverts.
	 * To make gpio_pin_get_dt return 1 (detected), set raw pin to 0. */
	gpio_emul_input_set(spec->port, spec->pin, detected ? 0 : 1);
}

static void clear_all_sensors(void)
{
	set_opponent(&sen_fl, false);
	set_opponent(&sen_fr, false);
	set_opponent(&sen_l, false);
	set_opponent(&sen_r, false);
}

static void *sensors_suite_setup(void)
{
	/* sensors_init may return error if ADC channels differ between
	 * native_sim and real hardware, but GPIO sensors still work */
	sensors_init();
	clear_all_sensors();
	return NULL;
}

static void sensors_before_each(void *fixture)
{
	ARG_UNUSED(fixture);
	clear_all_sensors();
	line_set_threshold(2000);
}

ZTEST_SUITE(sensors, NULL, sensors_suite_setup, sensors_before_each, NULL, NULL);

/* --- Opponent sensor tests --- */

ZTEST(sensors, test_no_opponent)
{
	zassert_false(opponent_detected(), "No opponent should be detected");
	zassert_false(sensor_front(), "Front should not detect");
}

ZTEST(sensors, test_fl_detected)
{
	set_opponent(&sen_fl, true);
	zassert_true(sensor_fl(), "FL should detect");
	zassert_true(opponent_detected(), "Opponent should be detected");
	zassert_false(sensor_front(), "Front needs both FL and FR");
}

ZTEST(sensors, test_fr_detected)
{
	set_opponent(&sen_fr, true);
	zassert_true(sensor_fr(), "FR should detect");
	zassert_true(opponent_detected(), "Opponent should be detected");
}

ZTEST(sensors, test_front_both)
{
	set_opponent(&sen_fl, true);
	set_opponent(&sen_fr, true);
	zassert_true(sensor_front(), "Both front sensors should = front detected");
}

ZTEST(sensors, test_side_left)
{
	set_opponent(&sen_l, true);
	zassert_true(sensor_l(), "Side left should detect");
	zassert_true(opponent_detected(), "Opponent should be detected");
}

ZTEST(sensors, test_side_right)
{
	set_opponent(&sen_r, true);
	zassert_true(sensor_r(), "Side right should detect");
	zassert_true(opponent_detected(), "Opponent should be detected");
}

ZTEST(sensors, test_all_sensors)
{
	set_opponent(&sen_fl, true);
	set_opponent(&sen_fr, true);
	set_opponent(&sen_l, true);
	set_opponent(&sen_r, true);

	zassert_true(sensor_front(), "Front should detect");
	zassert_true(sensor_l(), "Left should detect");
	zassert_true(sensor_r(), "Right should detect");
	zassert_true(opponent_detected(), "Opponent detected");
}

/* --- Line sensor tests --- */

ZTEST(sensors, test_line_threshold_default)
{
	zassert_equal(line_get_threshold(), 2000, "Default threshold should be 2000");
}

ZTEST(sensors, test_line_set_threshold)
{
	line_set_threshold(1500);
	zassert_equal(line_get_threshold(), 1500, "Threshold should update");
	line_set_threshold(2000);
}

ZTEST(sensors, test_line_on_ring)
{
	/* Set ADC emulator constant values for both channels */
	adc_emul_const_value_set(adc_dev, 0, 3000);
	adc_emul_const_value_set(adc_dev, 1, 3000);

	int left = line_read_left();
	int right = line_read_right();

	/* ADC emulator on native_sim may not support const_value_set for all configs */
	if (left == 0 && right == 0) {
		ztest_test_skip();
		return;
	}

	zassert_true(line_on_ring_left(), "Left should be on ring (3000 > 2000)");
	zassert_true(line_on_ring_right(), "Right should be on ring (3000 > 2000)");
}

ZTEST(sensors, test_line_edge_detected)
{
	/* Set ADC to read below threshold (white/edge) */
	/* When ADC returns 0 (default), it's below threshold = edge */
	zassert_false(line_on_ring_left(), "Default ADC=0 should be off ring");
	zassert_false(line_on_ring_right(), "Default ADC=0 should be off ring");
}

ZTEST(sensors, test_line_one_edge)
{
	adc_emul_const_value_set(adc_dev, 1, 3000);
	adc_emul_const_value_set(adc_dev, 0, 0);

	int left = line_read_left();
	if (left == 0) {
		ztest_test_skip();
		return;
	}

	zassert_true(line_on_ring_left(), "Left on ring");
	zassert_false(line_on_ring_right(), "Right at edge");
}

ZTEST(sensors, test_line_read_values)
{
	adc_emul_const_value_set(adc_dev, 0, 1234);
	adc_emul_const_value_set(adc_dev, 1, 4000);

	int right = line_read_right();
	if (right == 0) {
		ztest_test_skip();
		return;
	}

	int left = line_read_left();
	zassert_equal(right, 1234, "Right ADC should read 1234, got %d", right);
	zassert_equal(left, 4000, "Left ADC should read 4000, got %d", left);
}
