#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>

#include "button.h"

LOG_MODULE_REGISTER(button, LOG_LEVEL_INF);

#define DOUBLE_CLICK_WINDOW_MS 400

static struct k_sem press_sem;
static bool pending_press;
static int64_t first_press_time;

static void button_input_cb(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	/* Only handle key press events (value=1), not release */
	if (evt->type == INPUT_EV_KEY && evt->value == 1) {
		k_sem_give(&press_sem);
	}
}

INPUT_CALLBACK_DEFINE(NULL, button_input_cb, NULL);

int button_init(void)
{
	k_sem_init(&press_sem, 0, 1);
	pending_press = false;
	first_press_time = 0;

	LOG_INF("Button initialized");
	return 0;
}

/*
 * Non-blocking button poll with double-click detection.
 * Must be called frequently (e.g. every loop iteration).
 * BTN_SINGLE is returned after the double-click window expires.
 */
enum button_event button_poll(void)
{
	int64_t now = k_uptime_get();

	if (k_sem_take(&press_sem, K_NO_WAIT) != 0) {
		/* No new press — check if pending single expired */
		if (pending_press && (now - first_press_time) >= DOUBLE_CLICK_WINDOW_MS) {
			pending_press = false;
			return BTN_SINGLE;
		}
		return BTN_NONE;
	}

	/* Got a press */
	if (pending_press && (now - first_press_time) < DOUBLE_CLICK_WINDOW_MS) {
		/* Second press within window */
		pending_press = false;
		return BTN_DOUBLE;
	}

	/* First press — start double-click window */
	first_press_time = now;
	pending_press = true;
	return BTN_NONE;
}
