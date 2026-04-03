/*
 * Motor control unit tests.
 * Verifies drive() clamping, direction logic, and stop.
 * Uses gpio_emul to read back direction pin state.
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include "motors.h"

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

static const struct gpio_dt_spec motor_left_dir =
	GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, motor_left_dir_gpios);
static const struct gpio_dt_spec motor_right_dir =
	GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, motor_right_dir_gpios);

static void *motors_suite_setup(void)
{
	zassert_ok(motors_init(), "Motors init failed");
	return NULL;
}

ZTEST_SUITE(motors, NULL, motors_suite_setup, NULL, NULL, NULL);

ZTEST(motors, test_drive_forward)
{
	drive(100, 100);

	/* Forward: direction pins should be 0 (not reversed) */
	int left_dir = gpio_emul_output_get(motor_left_dir.port, motor_left_dir.pin);
	int right_dir = gpio_emul_output_get(motor_right_dir.port, motor_right_dir.pin);

	zassert_equal(left_dir, 0, "Left motor should be forward (dir=0)");
	zassert_equal(right_dir, 0, "Right motor should be forward (dir=0)");
}

ZTEST(motors, test_drive_reverse)
{
	drive(-100, -100);

	int left_dir = gpio_emul_output_get(motor_left_dir.port, motor_left_dir.pin);
	int right_dir = gpio_emul_output_get(motor_right_dir.port, motor_right_dir.pin);

	zassert_equal(left_dir, 1, "Left motor should be reverse (dir=1)");
	zassert_equal(right_dir, 1, "Right motor should be reverse (dir=1)");
}

ZTEST(motors, test_drive_turn_left)
{
	drive(-100, 100);

	int left_dir = gpio_emul_output_get(motor_left_dir.port, motor_left_dir.pin);
	int right_dir = gpio_emul_output_get(motor_right_dir.port, motor_right_dir.pin);

	zassert_equal(left_dir, 1, "Left motor reverse for left turn");
	zassert_equal(right_dir, 0, "Right motor forward for left turn");
}

ZTEST(motors, test_drive_turn_right)
{
	drive(100, -100);

	int left_dir = gpio_emul_output_get(motor_left_dir.port, motor_left_dir.pin);
	int right_dir = gpio_emul_output_get(motor_right_dir.port, motor_right_dir.pin);

	zassert_equal(left_dir, 0, "Left motor forward for right turn");
	zassert_equal(right_dir, 1, "Right motor reverse for right turn");
}

ZTEST(motors, test_drive_stop)
{
	drive(200, 200);
	motors_stop();

	/* After stop, direction pins should be 0 (drive(0,0) sets dir=0) */
	int left_dir = gpio_emul_output_get(motor_left_dir.port, motor_left_dir.pin);
	int right_dir = gpio_emul_output_get(motor_right_dir.port, motor_right_dir.pin);

	zassert_equal(left_dir, 0, "Left dir should be 0 after stop");
	zassert_equal(right_dir, 0, "Right dir should be 0 after stop");
}

ZTEST(motors, test_drive_clamp_high)
{
	/* Values above 255 should be clamped - should not crash */
	drive(500, 500);

	int left_dir = gpio_emul_output_get(motor_left_dir.port, motor_left_dir.pin);
	zassert_equal(left_dir, 0, "Clamped positive should be forward");
}

ZTEST(motors, test_drive_clamp_low)
{
	/* Values below -255 should be clamped */
	drive(-500, -500);

	int left_dir = gpio_emul_output_get(motor_left_dir.port, motor_left_dir.pin);
	zassert_equal(left_dir, 1, "Clamped negative should be reverse");
}

ZTEST(motors, test_drive_zero)
{
	drive(0, 0);

	int left_dir = gpio_emul_output_get(motor_left_dir.port, motor_left_dir.pin);
	int right_dir = gpio_emul_output_get(motor_right_dir.port, motor_right_dir.pin);

	zassert_equal(left_dir, 0, "Zero speed: left dir should be 0");
	zassert_equal(right_dir, 0, "Zero speed: right dir should be 0");
}
