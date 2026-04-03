#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

#include "servo.h"

LOG_MODULE_REGISTER(servo, LOG_LEVEL_INF);

static const struct pwm_dt_spec servo_pwm =
	PWM_DT_SPEC_GET(DT_NODELABEL(servo_ch));

/* Standard servo pulse range: 500us (0 deg) to 2500us (180 deg) */
#define SERVO_MIN_PULSE_US  500
#define SERVO_MAX_PULSE_US  2500

int servo_init(void)
{
	if (!pwm_is_ready_dt(&servo_pwm)) {
		LOG_ERR("Servo PWM not ready");
		return -ENODEV;
	}

	servo_set_angle(90);
	LOG_INF("Servo initialized (90 deg)");
	return 0;
}

void servo_set_angle(uint8_t angle)
{
	if (angle > 180) {
		angle = 180;
	}

	uint32_t pulse_us = SERVO_MIN_PULSE_US +
		((uint32_t)angle * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US)) / 180;

	pwm_set_pulse_dt(&servo_pwm, PWM_USEC(pulse_us));
}
