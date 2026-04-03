#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/display/cfb.h>
#include <zephyr/logging/log.h>

#include "display.h"
#include "sensors.h"
#include "settings.h"

LOG_MODULE_REGISTER(display, LOG_LEVEL_INF);

static const struct device *disp_dev;

static void display_work_handler(struct k_work *work)
{
	if (!disp_dev) {
		return;
	}

	cfb_framebuffer_clear(disp_dev, false);

	bool fl = sensor_fl();
	bool fr = sensor_fr();
	bool l = sensor_l();
	bool r = sensor_r();

	char line0[32];
	snprintf(line0, sizeof(line0), "FL:%d FR:%d L:%d R:%d", fl, fr, l, r);
	cfb_print(disp_dev, line0, 0, 0);

	char line1[32];
	snprintf(line1, sizeof(line1), "LnL:%d LnR:%d",
		 line_read_left(), line_read_right());
	cfb_print(disp_dev, line1, 0, 16);

	cfb_framebuffer_finalize(disp_dev);
}

static K_WORK_DEFINE(display_work, display_work_handler);

int display_init(void)
{
	disp_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(ssd1306));
	if (!disp_dev || !device_is_ready(disp_dev)) {
		LOG_WRN("Display not available");
		disp_dev = NULL;
		return -ENODEV;
	}

	if (cfb_framebuffer_init(disp_dev)) {
		LOG_ERR("CFB init failed");
		disp_dev = NULL;
		return -EIO;
	}

	cfb_framebuffer_clear(disp_dev, true);
	cfb_print(disp_dev, "Joltik V2", 0, 0);
	cfb_framebuffer_finalize(disp_dev);

	LOG_INF("Display initialized");
	return 0;
}

void display_request_update(void)
{
	k_work_submit(&display_work);
}
