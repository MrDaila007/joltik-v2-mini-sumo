#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "rc5.h"

LOG_MODULE_REGISTER(rc5, LOG_LEVEL_INF);

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

static const struct gpio_dt_spec ir_pin =
	GPIO_DT_SPEC_GET(ZEPHYR_USER_NODE, ir_gpios);

/* RC5 timing (microseconds) */
#define MIN_SHORT  444
#define MAX_SHORT  1333
#define MIN_LONG   1334
#define MAX_LONG   2222

/* Events (step by 2 for bit-shift in transition table) */
#define EVENT_SHORTSPACE  0
#define EVENT_SHORTPULSE  2
#define EVENT_LONGSPACE   4
#define EVENT_LONGPULSE   6

/* States */
#define STATE_START1 0
#define STATE_MID1   1
#define STATE_MID0   2
#define STATE_START0 3

/* RC5 bit field masks */
#define S2_MASK       0x1000
#define S2_SHIFT      12
#define TOGGLE_MASK   0x0800
#define TOGGLE_SHIFT  11
#define ADDRESS_MASK  0x7C0
#define ADDRESS_SHIFT 6
#define COMMAND_MASK  0x003F
#define COMMAND_SHIFT 0

/* Transition table from original RC5 library */
static const uint8_t trans[] = {0x01, 0x91, 0x9B, 0xFB};

/* Decoder state */
static volatile uint8_t rc5_state;
static volatile uint8_t rc5_bits;
static volatile uint16_t rc5_command;
static volatile uint32_t rc5_last_time;

static struct gpio_callback ir_cb_data;
K_MSGQ_DEFINE(ir_msgq, sizeof(struct rc5_message), 4, 4);

static void rc5_reset(void)
{
	rc5_state = STATE_MID1;
	rc5_bits = 1;
	rc5_command = 1;
}

static void rc5_decode_event(uint8_t event)
{
	uint8_t new_state = (trans[rc5_state] >> event) & 0x3;

	if (new_state == rc5_state) {
		rc5_reset();
	} else {
		rc5_state = new_state;
		if (new_state == STATE_MID0) {
			rc5_command = (rc5_command << 1) + 0;
			rc5_bits++;
		} else if (new_state == STATE_MID1) {
			rc5_command = (rc5_command << 1) + 1;
			rc5_bits++;
		}
	}
}

static void rc5_decode_pulse(uint8_t signal, uint32_t period_us)
{
	if (period_us >= MIN_SHORT && period_us <= MAX_SHORT) {
		rc5_decode_event(signal ? EVENT_SHORTPULSE : EVENT_SHORTSPACE);
	} else if (period_us >= MIN_LONG && period_us <= MAX_LONG) {
		rc5_decode_event(signal ? EVENT_LONGPULSE : EVENT_LONGSPACE);
	} else {
		rc5_reset();
	}
}

static void ir_gpio_callback(const struct device *dev, struct gpio_callback *cb,
			      uint32_t pins)
{
	uint32_t now = k_cycle_get_32();
	uint32_t elapsed_us = k_cyc_to_us_floor32(now - rc5_last_time);
	rc5_last_time = now;

	/* Read pin value. The IR receiver is active-low, but DT flag handles inversion,
	 * so gpio_pin_get_dt returns 1 when IR signal is present (pulse).
	 * The RC5 library convention: the read value after transition equals
	 * the theoretical signal value of the period that just ended. */
	int val = gpio_pin_get_dt(&ir_pin);

	rc5_decode_pulse(val, elapsed_us);

	if (rc5_bits == 14) {
		uint16_t msg_raw = rc5_command;
		struct rc5_message msg;

		msg.toggle = (msg_raw & TOGGLE_MASK) >> TOGGLE_SHIFT;
		msg.address = (msg_raw & ADDRESS_MASK) >> ADDRESS_SHIFT;
		/* Extended RC5: invert S2, shift into command bit 6 */
		uint8_t extended = (~msg_raw & S2_MASK) >> (S2_SHIFT - 6);
		msg.command = ((msg_raw & COMMAND_MASK) >> COMMAND_SHIFT) | extended;

		k_msgq_put(&ir_msgq, &msg, K_NO_WAIT);

		rc5_command = 0;
		rc5_bits = 0;
	}
}

int rc5_init(void)
{
	int ret;

	if (!gpio_is_ready_dt(&ir_pin)) {
		LOG_ERR("IR GPIO not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&ir_pin, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&ir_pin, GPIO_INT_EDGE_BOTH);
	if (ret < 0) {
		LOG_ERR("IR interrupt config failed: %d", ret);
		return ret;
	}

	gpio_init_callback(&ir_cb_data, ir_gpio_callback, BIT(ir_pin.pin));
	ret = gpio_add_callback(ir_pin.port, &ir_cb_data);
	if (ret < 0) {
		return ret;
	}

	rc5_reset();
	rc5_last_time = k_cycle_get_32();

	LOG_INF("RC5 IR initialized (GPIO%d)", ir_pin.pin);
	return 0;
}

bool rc5_get_message(struct rc5_message *msg, int32_t timeout_ms)
{
	k_timeout_t timeout = (timeout_ms < 0) ? K_FOREVER : K_MSEC(timeout_ms);
	return k_msgq_get(&ir_msgq, msg, timeout) == 0;
}

#ifdef CONFIG_ZTEST
void rc5_test_reset(void)
{
	rc5_reset();
}

void rc5_test_decode_pulse(uint8_t signal, uint32_t period_us)
{
	rc5_decode_pulse(signal, period_us);
}

uint8_t rc5_test_get_bits(void)
{
	return rc5_bits;
}

uint16_t rc5_test_get_command(void)
{
	return rc5_command;
}
#endif
