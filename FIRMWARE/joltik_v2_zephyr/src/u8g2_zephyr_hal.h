#ifndef U8G2_ZEPHYR_HAL_H
#define U8G2_ZEPHYR_HAL_H

#include <zephyr/device.h>
#include "../lib/u8g2/u8g2.h"

/**
 * Set up u8g2 for SSD1306 128x32 over Zephyr I2C.
 * Uses full framebuffer mode (_f variant).
 *
 * @param u8g2    u8g2 instance to initialize
 * @param i2c_dev Zephyr I2C device (must be ready)
 */
void u8g2_setup_zephyr_ssd1306_128x32(u8g2_t *u8g2, const struct device *i2c_dev);

#endif
