#ifndef JOLTIK_SERVO_H
#define JOLTIK_SERVO_H

#include <stdint.h>

int servo_init(void);
void servo_set_angle(uint8_t angle);

#endif
