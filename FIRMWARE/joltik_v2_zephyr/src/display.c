/*
 * Display driver — u8g2 graphics on SSD1306 128x32
 *
 * Layout:
 *   Top half  (y=0..15):  4 opponent sensor boxes  [L] [FL] [FR] [R]
 *   Separator (y=16):     horizontal line
 *   Bottom    (y=17..31): 2 line sensor bar graphs with threshold marker
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#include "display.h"
#include "u8g2_zephyr_hal.h"
#include "sensors.h"
#include "settings.h"

LOG_MODULE_REGISTER(display, LOG_LEVEL_INF);

static u8g2_t u8g2;
static bool display_ready;

/* Sensor box geometry */
#define BOX_W     29
#define BOX_H     14
#define BOX_Y      0
#define BOX_GAP    4
/* Box X positions: L=0, FL=33, FR=66, R=99 */
static const uint8_t box_x[4] = { 0, 33, 66, 99 };

/* Bar graph geometry */
#define BAR_Y1    18
#define BAR_Y2    25
#define BAR_H      6
#define BAR_W    128

static void display_work_handler(struct k_work *work)
{
	if (!display_ready) {
		return;
	}

	u8g2_ClearBuffer(&u8g2);

	/* --- Top: 4 opponent sensor boxes --- */
	bool sensors[4] = {
		sensor_l(), sensor_fl(), sensor_fr(), sensor_r()
	};
	static const char *labels[4] = { "L", "FL", "FR", "R" };

	u8g2_SetFont(&u8g2, u8g2_font_5x7_tr);

	for (int i = 0; i < 4; i++) {
		uint8_t x = box_x[i];

		if (sensors[i]) {
			/* Filled box for active sensor */
			u8g2_DrawBox(&u8g2, x, BOX_Y, BOX_W, BOX_H);
			/* Label in inverted color (black on white) */
			u8g2_SetDrawColor(&u8g2, 0);
		} else {
			/* Outline only */
			u8g2_DrawFrame(&u8g2, x, BOX_Y, BOX_W, BOX_H);
			u8g2_SetDrawColor(&u8g2, 1);
		}

		/* Center label in box */
		uint8_t tw = u8g2_GetStrWidth(&u8g2, labels[i]);
		u8g2_DrawStr(&u8g2, x + (BOX_W - tw) / 2, BOX_Y + 11, labels[i]);
		u8g2_SetDrawColor(&u8g2, 1);
	}

	/* --- Separator line --- */
	u8g2_DrawHLine(&u8g2, 0, 16, 128);

	/* --- Bottom: line sensor bar graphs --- */
	int left_val  = line_read_left();
	int right_val = line_read_right();
	uint16_t threshold = line_get_threshold();

	int left_bar  = (left_val * BAR_W) / 4095;
	int right_bar = (right_val * BAR_W) / 4095;
	int thresh_x  = (threshold * BAR_W) / 4095;

	if (left_bar > BAR_W) {
		left_bar = BAR_W;
	}
	if (right_bar > BAR_W) {
		right_bar = BAR_W;
	}
	if (thresh_x > 127) {
		thresh_x = 127;
	}

	/* Left bar */
	if (left_bar > 0) {
		u8g2_DrawBox(&u8g2, 0, BAR_Y1, left_bar, BAR_H);
	}

	/* Right bar */
	if (right_bar > 0) {
		u8g2_DrawBox(&u8g2, 0, BAR_Y2, right_bar, BAR_H);
	}

	/* Threshold markers (XOR so they contrast with both filled and empty) */
	u8g2_SetDrawColor(&u8g2, 2); /* XOR mode */
	u8g2_DrawVLine(&u8g2, thresh_x, BAR_Y1, BAR_H);
	u8g2_DrawVLine(&u8g2, thresh_x, BAR_Y2, BAR_H);
	u8g2_SetDrawColor(&u8g2, 1);

	u8g2_SendBuffer(&u8g2);
}

static K_WORK_DEFINE(display_work, display_work_handler);

int display_init(void)
{
	const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));

	if (!device_is_ready(i2c_dev)) {
		LOG_WRN("I2C1 not ready — display disabled");
		return -ENODEV;
	}

	u8g2_setup_zephyr_ssd1306_128x32(&u8g2, i2c_dev);
	u8g2_InitDisplay(&u8g2);
	u8g2_SetPowerSave(&u8g2, 0);
	u8g2_SetContrast(&u8g2, 200);

	/* Splash screen */
	u8g2_ClearBuffer(&u8g2);
	u8g2_SetFont(&u8g2, u8g2_font_helvB14_tr);

	const char *splash = "JOLTIK";
	uint8_t tw = u8g2_GetStrWidth(&u8g2, splash);
	u8g2_DrawStr(&u8g2, (128 - tw) / 2, 24, splash);
	u8g2_SendBuffer(&u8g2);

	display_ready = true;
	LOG_INF("Display initialized (u8g2)");
	return 0;
}

void display_request_update(void)
{
	k_work_submit(&display_work);
}
