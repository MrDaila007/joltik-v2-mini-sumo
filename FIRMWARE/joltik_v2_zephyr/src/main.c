/*
 * Joltik V2 Mini Sumo Competition Firmware - Zephyr RTOS
 *
 * State machine: WAITING -> countdown -> tactics -> combat loop
 * IR remote handled in dedicated thread via k_msgq.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include "motors.h"
#include "sensors.h"
#include "led.h"
#include "servo.h"
#include "combat.h"
#include "tactics.h"
#include "button.h"
#include "rc5.h"
#include "settings.h"
#include "display.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* Shared state between main thread, IR thread, and button */
static atomic_t running = ATOMIC_INIT(0);
static atomic_t current_tactic = ATOMIC_INIT(0);
static K_EVENT_DEFINE(start_event);

#define EVENT_START BIT(0)

/* ==================== IR Thread ==================== */
#define IR_STACK_SIZE 1024
#define IR_PRIORITY   5

static void ir_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct rc5_message msg;

	LOG_INF("IR thread started");

	while (1) {
		if (!rc5_get_message(&msg, -1)) {
			continue;
		}

		LOG_INF("IR: addr=%u cmd=%u toggle=%u",
			msg.address, msg.command, msg.toggle);

		switch (msg.command) {
		case RC5_CMD_TACTIC_1:
		case RC5_CMD_TACTIC_2:
		case RC5_CMD_TACTIC_3:
		case RC5_CMD_TACTIC_4:
			atomic_set(&current_tactic, msg.command - 1);
			led_set_state(STATE_TACTIC_SELECT);
			k_msleep(100);
			break;

		case RC5_CMD_POWER:
			if (atomic_get(&running)) {
				atomic_set(&running, 0);
				motors_stop();
			} else {
				k_event_post(&start_event, EVENT_START);
			}
			break;
		}
	}
}

K_THREAD_DEFINE(ir_tid, IR_STACK_SIZE, ir_thread_entry,
		NULL, NULL, NULL, IR_PRIORITY, 0, 0);

/* ==================== Stop & Tactic Select ==================== */
static void stop_robot(void)
{
	atomic_set(&running, 0);
	motors_stop();
	combat_reset_count();
	LOG_INF("STOPPED");

	/* Blink white, wait for restart */
	while (!atomic_get(&running)) {
		enum button_event evt = button_poll();

		if (evt == BTN_SINGLE) {
			break;
		}
		if (evt == BTN_DOUBLE) {
			uint8_t t = (atomic_get(&current_tactic) + 1) % TACTIC_COUNT;
			atomic_set(&current_tactic, t);
			g_settings.current_tactic = t;
			LOG_INF("Tactic: %u", t);

			for (uint8_t i = 0; i <= t; i++) {
				led_set_state(STATE_TACTIC_SELECT);
				k_msleep(150);
				led_off();
				k_msleep(150);
			}
			continue;
		}

		/* Check for IR-triggered start */
		if (k_event_wait(&start_event, EVENT_START, false, K_NO_WAIT)) {
			k_event_clear(&start_event, EVENT_START);
			break;
		}

		display_request_update();
		led_set_state(STATE_WAITING);
		k_msleep(500);
		led_off();
		k_msleep(500);
	}

	atomic_set(&running, 1);
}

/* ==================== Main ==================== */
int main(void)
{
	int ret;

	LOG_INF("Joltik V2 Mini Sumo - Zephyr RTOS");

	/* Initialize all subsystems */
	ret = motors_init();
	if (ret) {
		LOG_ERR("Motors init failed: %d", ret);
	}

	ret = sensors_init();
	if (ret) {
		LOG_ERR("Sensors init failed: %d", ret);
	}

	ret = led_init();
	if (ret) {
		LOG_ERR("LED init failed: %d", ret);
	}

	ret = servo_init();
	if (ret) {
		LOG_ERR("Servo init failed: %d", ret);
	}

	ret = button_init();
	if (ret) {
		LOG_ERR("Button init failed: %d", ret);
	}

	ret = rc5_init();
	if (ret) {
		LOG_ERR("RC5 init failed: %d", ret);
	}

	ret = app_settings_init();
	if (ret) {
		LOG_ERR("Settings init failed: %d", ret);
	}

	ret = display_init();
	if (ret) {
		LOG_WRN("Display init failed: %d", ret);
	}

	/* Apply loaded settings */
	atomic_set(&current_tactic, g_settings.current_tactic);

	motors_stop();
	led_set_state(STATE_WAITING);

	LOG_INF("Waiting for start...");

	/* ---- Wait for start (button or IR) ---- */
	bool started = false;
	while (!started) {
		enum button_event evt = button_poll();

		if (evt == BTN_SINGLE) {
			started = true;
			break;
		}
		if (evt == BTN_DOUBLE) {
			uint8_t t = (atomic_get(&current_tactic) + 1) % TACTIC_COUNT;
			atomic_set(&current_tactic, t);
			g_settings.current_tactic = t;
			LOG_INF("Tactic selected: %u", t);

			for (uint8_t i = 0; i <= t; i++) {
				led_set_state(STATE_TACTIC_SELECT);
				k_msleep(150);
				led_off();
				k_msleep(150);
			}
			led_set_state(STATE_WAITING);
			continue;
		}

		/* Check for IR-triggered start */
		if (k_event_wait(&start_event, EVENT_START, false, K_NO_WAIT)) {
			k_event_clear(&start_event, EVENT_START);
			started = true;
			break;
		}

		display_request_update();
		k_msleep(50);
	}

	/* ---- 5-second countdown ---- */
	atomic_set(&running, 1);
	led_set_state(STATE_COUNTDOWN);

	for (int i = 5; i > 0; i--) {
		LOG_INF("Countdown: %d", i);
		if (i % 2) {
			led_set_state(STATE_COUNTDOWN);
		} else {
			led_off();
		}
		k_msleep(1000);
	}
	led_set_state(STATE_SEARCHING);

	/* Execute selected tactic start routine */
	uint8_t tactic = atomic_get(&current_tactic);
	tactics_execute(tactic);

	/* ---- Main combat loop ---- */
	LOG_INF("Combat loop started");

	while (1) {
		/* Button press stops the robot */
		enum button_event evt = button_poll();
		if (evt == BTN_SINGLE) {
			stop_robot();
			/* Restart with tactics */
			tactic = atomic_get(&current_tactic);
			/* 5-second countdown before restart */
			for (int i = 5; i > 0; i--) {
				LOG_INF("Countdown: %d", i);
				if (i % 2) {
					led_set_state(STATE_COUNTDOWN);
				} else {
					led_off();
				}
				k_msleep(1000);
			}
			tactics_execute(tactic);
			continue;
		}

		/* IR-triggered stop */
		if (!atomic_get(&running)) {
			stop_robot();
			tactic = atomic_get(&current_tactic);
			for (int i = 5; i > 0; i--) {
				LOG_INF("Countdown: %d", i);
				if (i % 2) {
					led_set_state(STATE_COUNTDOWN);
				} else {
					led_off();
				}
				k_msleep(1000);
			}
			tactics_execute(tactic);
			continue;
		}

		/* Check line sensors (always active) */
		bool line_l = line_on_ring_left();
		bool line_r = line_on_ring_right();

		if (!line_l && !line_r) {
			backoff(DIR_RIGHT);
			continue;
		}
		if (!line_l) {
			backoff(DIR_RIGHT);
			continue;
		}
		if (!line_r) {
			backoff(DIR_LEFT);
			continue;
		}

		/* No opponent - search; opponent - attack */
		if (!opponent_detected()) {
			search();
			combat_reset_count();
			LOG_DBG("Searching");
		} else {
			attack();
			LOG_DBG("Attacking");
		}

		display_request_update();
	}

	return 0;
}
