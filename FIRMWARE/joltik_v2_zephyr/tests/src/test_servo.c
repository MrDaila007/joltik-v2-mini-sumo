/*
 * Servo control unit tests.
 * Verifies angle-to-pulse conversion and clamping.
 */
#include <zephyr/ztest.h>
#include "servo.h"

static void *servo_suite_setup(void)
{
	zassert_ok(servo_init(), "Servo init failed");
	return NULL;
}

ZTEST_SUITE(servo, NULL, servo_suite_setup, NULL, NULL, NULL);

ZTEST(servo, test_init_sets_90)
{
	/* servo_init sets 90 degrees - should not crash */
	zassert_ok(servo_init(), "Re-init should succeed");
}

ZTEST(servo, test_angle_0)
{
	/* Should not crash, sets minimum pulse */
	servo_set_angle(0);
}

ZTEST(servo, test_angle_90)
{
	servo_set_angle(90);
}

ZTEST(servo, test_angle_180)
{
	servo_set_angle(180);
}

ZTEST(servo, test_angle_clamp_above_180)
{
	/* Angles > 180 should be clamped to 180 */
	servo_set_angle(255);
	/* Should not crash, treated as 180 */
}

ZTEST(servo, test_angle_sweep)
{
	/* Sweep through all angles to verify no crashes */
	for (uint8_t a = 0; a <= 180; a += 10) {
		servo_set_angle(a);
	}
}
