#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

#include "sensors.h"

LOG_MODULE_REGISTER(sensors, LOG_LEVEL_INF);

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

/* Opponent sensors (active-low handled by DT flags) */
static const struct gpio_dt_spec sen_fl =
	GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, sensor_fl_gpios);
static const struct gpio_dt_spec sen_fr =
	GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, sensor_fr_gpios);
static const struct gpio_dt_spec sen_l =
	GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, sensor_l_gpios);
static const struct gpio_dt_spec sen_r =
	GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, sensor_r_gpios);

/* Line sensors (ADC) */
static const struct adc_dt_spec line_right_adc =
	ADC_DT_SPEC_GET_BY_IDX(ZEPHYR_USER_NODE, 0);
static const struct adc_dt_spec line_left_adc =
	ADC_DT_SPEC_GET_BY_IDX(ZEPHYR_USER_NODE, 1);

/* Per-channel ADC buffers to avoid race between main thread and workqueue */
static int16_t adc_buf_right;
static int16_t adc_buf_left;
static struct adc_sequence adc_seq_right = {
	.buffer = &adc_buf_right,
	.buffer_size = sizeof(adc_buf_right),
};
static struct adc_sequence adc_seq_left = {
	.buffer = &adc_buf_left,
	.buffer_size = sizeof(adc_buf_left),
};

/* Line threshold: Arduino 500/1024 ≈ Zephyr 2000/4096 */
static uint16_t line_threshold = 2000;

int sensors_init(void)
{
	int ret;
	const struct gpio_dt_spec *opponent_pins[] = {
		&sen_fl, &sen_fr, &sen_l, &sen_r
	};

	for (int i = 0; i < 4; i++) {
		if (!gpio_is_ready_dt(opponent_pins[i])) {
			LOG_ERR("Opponent sensor GPIO %d not ready", i);
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(opponent_pins[i], GPIO_INPUT);
		if (ret < 0) {
			return ret;
		}
	}

	/* ADC setup */
	if (!adc_is_ready_dt(&line_right_adc) ||
	    !adc_is_ready_dt(&line_left_adc)) {
		LOG_ERR("ADC not ready");
		return -ENODEV;
	}

	ret = adc_channel_setup_dt(&line_right_adc);
	if (ret < 0) {
		LOG_ERR("ADC ch right setup failed: %d", ret);
		return ret;
	}

	ret = adc_channel_setup_dt(&line_left_adc);
	if (ret < 0) {
		LOG_ERR("ADC ch left setup failed: %d", ret);
		return ret;
	}

	LOG_INF("Sensors initialized (threshold=%u)", line_threshold);
	return 0;
}

/* Opponent sensors: gpio_pin_get_dt returns 1 when active (handles active-low flag) */
bool sensor_fl(void)
{
	return gpio_pin_get_dt(&sen_fl) > 0;
}

bool sensor_fr(void)
{
	return gpio_pin_get_dt(&sen_fr) > 0;
}

bool sensor_l(void)
{
	return gpio_pin_get_dt(&sen_l) > 0;
}

bool sensor_r(void)
{
	return gpio_pin_get_dt(&sen_r) > 0;
}

bool sensor_front(void)
{
	return sensor_fl() && sensor_fr();
}

bool opponent_detected(void)
{
	return sensor_fl() || sensor_fr() || sensor_l() || sensor_r();
}

/* Line sensors: read ADC raw value (12-bit, 0-4095) */
static int adc_read_channel(const struct adc_dt_spec *spec,
			     struct adc_sequence *seq, int16_t *buf)
{
	int ret;

	adc_sequence_init_dt(spec, seq);
	ret = adc_read_dt(spec, seq);
	if (ret < 0) {
		LOG_ERR("ADC read failed: %d", ret);
		return 0;
	}

	return *buf;
}

int line_read_left(void)
{
	return adc_read_channel(&line_left_adc, &adc_seq_left, &adc_buf_left);
}

int line_read_right(void)
{
	return adc_read_channel(&line_right_adc, &adc_seq_right, &adc_buf_right);
}

bool line_on_ring_left(void)
{
	return line_read_left() > line_threshold;
}

bool line_on_ring_right(void)
{
	return line_read_right() > line_threshold;
}

uint16_t line_get_threshold(void)
{
	return line_threshold;
}

void line_set_threshold(uint16_t threshold)
{
	line_threshold = threshold;
}
