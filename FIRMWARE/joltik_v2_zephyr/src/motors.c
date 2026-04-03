#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>

#include "motors.h"

LOG_MODULE_REGISTER(motors, LOG_LEVEL_INF);

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

static const struct pwm_dt_spec motor_left_pwm =
	PWM_DT_SPEC_GET(DT_NODELABEL(motor_l));
static const struct pwm_dt_spec motor_right_pwm =
	PWM_DT_SPEC_GET(DT_NODELABEL(motor_r));

static const struct gpio_dt_spec motor_left_dir =
	GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, motor_left_dir_gpios);
static const struct gpio_dt_spec motor_right_dir =
	GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, motor_right_dir_gpios);

int motors_init(void)
{
	int ret;

	if (!pwm_is_ready_dt(&motor_left_pwm) ||
	    !pwm_is_ready_dt(&motor_right_pwm)) {
		LOG_ERR("PWM devices not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&motor_left_dir) ||
	    !gpio_is_ready_dt(&motor_right_dir)) {
		LOG_ERR("Motor direction GPIO not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&motor_left_dir, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&motor_right_dir, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	motors_stop();
	LOG_INF("Motors initialized");
	return 0;
}

void drive(int left, int right)
{
	left = CLAMP(left, -255, 255);
	right = CLAMP(right, -255, 255);

	/* Left motor: direction + PWM duty */
	gpio_pin_set_dt(&motor_left_dir, left < 0 ? 1 : 0);
	uint32_t left_pulse = (motor_left_pwm.period * (uint32_t)abs(left)) / 255;
	pwm_set_pulse_dt(&motor_left_pwm, left_pulse);

	/* Right motor: direction + PWM duty */
	gpio_pin_set_dt(&motor_right_dir, right < 0 ? 1 : 0);
	uint32_t right_pulse = (motor_right_pwm.period * (uint32_t)abs(right)) / 255;
	pwm_set_pulse_dt(&motor_right_pwm, right_pulse);
}

void motors_stop(void)
{
	drive(0, 0);
}
