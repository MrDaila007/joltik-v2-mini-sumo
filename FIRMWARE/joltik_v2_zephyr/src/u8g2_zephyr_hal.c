/*
 * u8g2 HAL adapter for Zephyr RTOS
 *
 * Provides I2C byte transport and delay callbacks so u8g2 can
 * talk to the SSD1306 via Zephyr's I2C driver.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>

#include "u8g2_zephyr_hal.h"

/* I2C transfer buffer — SSD1306 max payload is 1 control byte + 128 data bytes */
#define I2C_BUF_SIZE 256

static const struct device *zephyr_i2c_dev;
static uint8_t i2c_buf[I2C_BUF_SIZE];
static uint16_t i2c_buf_pos;

/*
 * I2C byte-level callback for u8g2.
 * Buffers bytes between START_TRANSFER and END_TRANSFER,
 * then sends the whole buffer in one i2c_write() call.
 */
static uint8_t u8x8_byte_zephyr_hw_i2c(u8x8_t *u8x8, uint8_t msg,
					uint8_t arg_int, void *arg_ptr)
{
	switch (msg) {
	case U8X8_MSG_BYTE_INIT:
		/* Nothing — I2C device is set up externally */
		break;

	case U8X8_MSG_BYTE_SET_DC:
		/* Not used for I2C */
		break;

	case U8X8_MSG_BYTE_START_TRANSFER:
		i2c_buf_pos = 0;
		break;

	case U8X8_MSG_BYTE_SEND: {
		uint8_t *data = (uint8_t *)arg_ptr;

		for (uint16_t i = 0; i < arg_int; i++) {
			if (i2c_buf_pos < I2C_BUF_SIZE) {
				i2c_buf[i2c_buf_pos++] = data[i];
			}
		}
		break;
	}

	case U8X8_MSG_BYTE_END_TRANSFER:
		if (zephyr_i2c_dev && i2c_buf_pos > 0) {
			uint8_t addr = u8x8_GetI2CAddress(u8x8) >> 1;

			i2c_write(zephyr_i2c_dev, i2c_buf, i2c_buf_pos, addr);
		}
		break;

	default:
		return 0;
	}

	return 1;
}

/*
 * GPIO and delay callback for u8g2.
 * SSD1306 over I2C needs no GPIOs — only delays.
 */
static uint8_t u8x8_gpio_and_delay_zephyr(u8x8_t *u8x8, uint8_t msg,
					   uint8_t arg_int, void *arg_ptr)
{
	switch (msg) {
	case U8X8_MSG_DELAY_MILLI:
		k_msleep(arg_int);
		break;

	case U8X8_MSG_DELAY_10MICRO:
		k_busy_wait(arg_int * 10);
		break;

	case U8X8_MSG_DELAY_100NANO:
		/* Too short for Zephyr timer — busy wait */
		k_busy_wait(1);
		break;

	case U8X8_MSG_GPIO_AND_DELAY_INIT:
	case U8X8_MSG_GPIO_I2C_CLOCK:
	case U8X8_MSG_GPIO_I2C_DATA:
		/* Managed by Zephyr I2C driver */
		break;

	default:
		return 0;
	}

	return 1;
}

void u8g2_setup_zephyr_ssd1306_128x32(u8g2_t *u8g2, const struct device *i2c_dev)
{
	zephyr_i2c_dev = i2c_dev;

	u8g2_Setup_ssd1306_i2c_128x32_univision_f(u8g2, U8G2_R0,
						   u8x8_byte_zephyr_hw_i2c,
						   u8x8_gpio_and_delay_zephyr);

	/* SSD1306 I2C address: 0x3C (u8g2 uses left-shifted = 0x78) */
	u8g2_SetI2CAddress(u8g2, 0x78);
}
