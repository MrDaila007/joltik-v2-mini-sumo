#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>

#include "button.h"

LOG_MODULE_REGISTER(button, LOG_LEVEL_INF);

#define DOUBLE_CLICK_WINDOW_MS 400

static struct k_sem press_sem;
static volatile int64_t last_press_time;
static volatile int press_count;

static void button_input_cb(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	/* Only handle key press events (value=1), not release */
	if (evt->type == INPUT_EV_KEY && evt->value == 1) {
		press_count++;
		last_press_time = k_uptime_get();
		k_sem_give(&press_sem);
	}
}

INPUT_CALLBACK_DEFINE(NULL, button_input_cb, NULL);

int button_init(void)
{
	k_sem_init(&press_sem, 0, 1);
	press_count = 0;
	last_press_time = 0;

	LOG_INF("Button initialized");
	return 0;
}

enum button_event button_poll(void)
{
	/* Non-blocking check: wait briefly for a press */
	if (k_sem_take(&press_sem, K_NO_WAIT) != 0) {
		return BTN_NONE;
	}

	/* Got first press, wait for possible second press */
	int saved_count = press_count;
	k_msleep(DOUBLE_CLICK_WINDOW_MS);

	if (press_count > saved_count) {
		/* Second press arrived within window */
		press_count = 0;
		return BTN_DOUBLE;
	}

	press_count = 0;
	return BTN_SINGLE;
}
