#ifndef JOLTIK_SENSORS_H
#define JOLTIK_SENSORS_H

#include <stdbool.h>
#include <stdint.h>

int sensors_init(void);

/* Opponent sensors (active-low: true = opponent detected) */
bool sensor_fl(void);
bool sensor_fr(void);
bool sensor_l(void);
bool sensor_r(void);
bool sensor_front(void);
bool opponent_detected(void);

/* Line sensors (12-bit ADC) */
int line_read_left(void);
int line_read_right(void);
bool line_on_ring_left(void);
bool line_on_ring_right(void);

/* Get/set line threshold */
uint16_t line_get_threshold(void);
void line_set_threshold(uint16_t threshold);

#endif
