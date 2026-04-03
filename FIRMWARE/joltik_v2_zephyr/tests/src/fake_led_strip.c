/*
 * Fake LED strip driver for testing.
 * Records the last pixel color set, so tests can verify LED state.
 */
#define DT_DRV_COMPAT joltik_fake_led_strip

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>

struct led_rgb fake_led_pixel;
int fake_led_update_count;

static int fake_led_strip_update_rgb(const struct device *dev,
				     struct led_rgb *pixels,
				     size_t num_pixels)
{
	if (num_pixels > 0) {
		fake_led_pixel = pixels[0];
	}
	fake_led_update_count++;
	return 0;
}

static int fake_led_strip_update_channels(const struct device *dev,
					  uint8_t *channels,
					  size_t num_channels)
{
	return 0;
}

static size_t fake_led_strip_length(const struct device *dev)
{
	return 1;
}

static const struct led_strip_driver_api fake_led_api = {
	.update_rgb = fake_led_strip_update_rgb,
	.update_channels = fake_led_strip_update_channels,
	.length = fake_led_strip_length,
};

static int fake_led_strip_init(const struct device *dev)
{
	fake_led_update_count = 0;
	return 0;
}

#define FAKE_LED_STRIP_INIT(n)						\
	DEVICE_DT_INST_DEFINE(n, fake_led_strip_init, NULL,		\
			      NULL, NULL,				\
			      POST_KERNEL,				\
			      CONFIG_LED_STRIP_INIT_PRIORITY,		\
			      &fake_led_api);

DT_INST_FOREACH_STATUS_OKAY(FAKE_LED_STRIP_INIT)
