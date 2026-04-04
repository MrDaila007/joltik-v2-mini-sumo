#include <zephyr/kernel.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/logging/log.h>

#include "led.h"

LOG_MODULE_REGISTER(led, LOG_LEVEL_INF);

#define STRIP_NODE DT_ALIAS(led_strip)
static const struct device *strip = DEVICE_DT_GET(STRIP_NODE);

static struct led_rgb pixel;
static struct led_rgb pending_pixel;

static void led_work_handler(struct k_work *work)
{
	pixel = pending_pixel;
	led_strip_update_rgb(strip, &pixel, 1);
}

static K_WORK_DEFINE(led_work, led_work_handler);

int led_init(void)
{
	if (!device_is_ready(strip)) {
		LOG_ERR("LED strip not ready");
		return -ENODEV;
	}

	led_off();
	LOG_INF("LED initialized");
	return 0;
}

/* Brightness cap: 64/255 ≈ 25% of max */
#define LED_BRIGHTNESS_SCALE 64

void led_set_color(uint8_t r, uint8_t g, uint8_t b)
{
	pending_pixel.r = (r * LED_BRIGHTNESS_SCALE) / 255;
	pending_pixel.g = (g * LED_BRIGHTNESS_SCALE) / 255;
	pending_pixel.b = (b * LED_BRIGHTNESS_SCALE) / 255;
	k_work_submit(&led_work);
}

void led_set_state(enum robot_state state)
{
	switch (state) {
	case STATE_WAITING:
		led_set_color(255, 255, 255); /* white */
		break;
	case STATE_SEARCHING:
		led_set_color(0, 255, 0); /* green */
		break;
	case STATE_ATTACKING:
		led_set_color(0, 0, 255); /* blue */
		break;
	case STATE_BACKOFF:
		led_set_color(255, 0, 0); /* red */
		break;
	case STATE_TACTIC_SELECT:
		led_set_color(255, 255, 0); /* yellow */
		break;
	case STATE_COUNTDOWN:
		led_set_color(0, 255, 0); /* green */
		break;
	}
}

void led_off(void)
{
	led_set_color(0, 0, 0);
}
