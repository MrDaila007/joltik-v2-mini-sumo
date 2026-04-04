#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>

#include "motors.h"
#include "sensors.h"
#include "led.h"
#include "servo.h"
#include "tactics.h"
#include "settings.h"

/* ---- sensors ---- */
static int cmd_sensors(const struct shell *sh, size_t argc, char **argv)
{
	shell_print(sh, "Opponent: FL=%d FR=%d L=%d R=%d",
		    sensor_fl(), sensor_fr(), sensor_l(), sensor_r());
	shell_print(sh, "Line: L=%d R=%d (threshold=%u)",
		    line_read_left(), line_read_right(), line_get_threshold());
	shell_print(sh, "Ring: L=%s R=%s",
		    line_on_ring_left() ? "yes" : "no",
		    line_on_ring_right() ? "yes" : "no");
	return 0;
}
SHELL_CMD_REGISTER(sensors, NULL, "Read all sensor values", cmd_sensors);

/* ---- motor ---- */
static int cmd_motor_drive(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 3) {
		shell_error(sh, "Usage: motor drive <left> <right>");
		return -EINVAL;
	}
	int left = atoi(argv[1]);
	int right = atoi(argv[2]);
	drive(left, right);
	shell_print(sh, "Driving: left=%d right=%d", left, right);
	return 0;
}

static int cmd_motor_stop(const struct shell *sh, size_t argc, char **argv)
{
	motors_stop();
	shell_print(sh, "Motors stopped");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_motor,
	SHELL_CMD(drive, NULL, "drive <left> <right> (-255..255)", cmd_motor_drive),
	SHELL_CMD(stop, NULL, "Stop motors", cmd_motor_stop),
	SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(motor, &sub_motor, "Motor commands", NULL);

/* ---- tactic ---- */
extern struct robot_settings g_settings;

static int cmd_tactic(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 2) {
		int t = atoi(argv[1]);
		if (t < 0 || t >= TACTIC_COUNT) {
			shell_error(sh, "Tactic must be 0-%d", TACTIC_COUNT - 1);
			return -EINVAL;
		}
		g_settings.current_tactic = t;
		shell_print(sh, "Tactic set to %d", t);
	} else {
		static const char *names[] = {
			"straight charge", "reverse start", "left sweep", "right sweep"
		};
		uint8_t t = g_settings.current_tactic;

		if (t >= TACTIC_COUNT) {
			t = 0;
		}
		shell_print(sh, "Current tactic: %u (%s)", t, names[t]);
	}
	return 0;
}
SHELL_CMD_REGISTER(tactic, NULL, "tactic [0-3] - get/set tactic", cmd_tactic);

/* ---- config ---- */
static int cmd_config_show(const struct shell *sh, size_t argc, char **argv)
{
	shell_print(sh, "current_tactic: %u", g_settings.current_tactic);
	shell_print(sh, "line_threshold: %u", g_settings.line_threshold);
	shell_print(sh, "max_speed:      %u", g_settings.max_speed);
	shell_print(sh, "straight_speed: %u", g_settings.straight_speed);
	shell_print(sh, "search_speed:   %u", g_settings.search_speed);
	shell_print(sh, "rotate_speed:   %u", g_settings.rotate_speed);
	shell_print(sh, "breakout_speed: %u", g_settings.breakout_speed);
	shell_print(sh, "attack_speed:   %u", g_settings.attack_speed);
	return 0;
}

static int cmd_config_threshold(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_error(sh, "Usage: config threshold <0-4095>");
		return -EINVAL;
	}
	uint16_t val = atoi(argv[1]);
	g_settings.line_threshold = val;
	line_set_threshold(val);
	shell_print(sh, "Line threshold set to %u", val);
	return 0;
}

static int cmd_config_speed(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 3) {
		shell_error(sh, "Usage: config speed <name> <value>");
		shell_print(sh, "Names: max, straight, search, rotate, breakout, attack");
		return -EINVAL;
	}

	uint8_t val = atoi(argv[2]);
	const char *name = argv[1];

	if (strcmp(name, "max") == 0) {
		g_settings.max_speed = val;
	} else if (strcmp(name, "straight") == 0) {
		g_settings.straight_speed = val;
	} else if (strcmp(name, "search") == 0) {
		g_settings.search_speed = val;
	} else if (strcmp(name, "rotate") == 0) {
		g_settings.rotate_speed = val;
	} else if (strcmp(name, "breakout") == 0) {
		g_settings.breakout_speed = val;
	} else if (strcmp(name, "attack") == 0) {
		g_settings.attack_speed = val;
	} else {
		shell_error(sh, "Unknown speed: %s", name);
		return -EINVAL;
	}

	shell_print(sh, "%s speed set to %u", name, val);
	return 0;
}

static int cmd_config_save(const struct shell *sh, size_t argc, char **argv)
{
	int ret = app_settings_save();
	if (ret == 0) {
		shell_print(sh, "Settings saved to flash");
	} else {
		shell_error(sh, "Save failed: %d", ret);
	}
	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_config,
	SHELL_CMD(show, NULL, "Show all settings", cmd_config_show),
	SHELL_CMD(threshold, NULL, "config threshold <value>", cmd_config_threshold),
	SHELL_CMD(speed, NULL, "config speed <name> <value>", cmd_config_speed),
	SHELL_CMD(save, NULL, "Save settings to flash", cmd_config_save),
	SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(config, &sub_config, "Configuration commands", NULL);

/* ---- led ---- */
static int cmd_led(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 4) {
		shell_error(sh, "Usage: led <r> <g> <b>");
		return -EINVAL;
	}
	uint8_t r = atoi(argv[1]);
	uint8_t g = atoi(argv[2]);
	uint8_t b = atoi(argv[3]);
	led_set_color(r, g, b);
	shell_print(sh, "LED set to R=%u G=%u B=%u", r, g, b);
	return 0;
}
SHELL_CMD_REGISTER(led, NULL, "led <r> <g> <b> - set LED color", cmd_led);

/* ---- servo ---- */
static int cmd_servo(const struct shell *sh, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_error(sh, "Usage: servo <angle 0-180>");
		return -EINVAL;
	}
	uint8_t angle = atoi(argv[1]);
	servo_set_angle(angle);
	shell_print(sh, "Servo set to %u degrees", angle);
	return 0;
}
SHELL_CMD_REGISTER(servo, NULL, "servo <angle> - set servo angle", cmd_servo);
