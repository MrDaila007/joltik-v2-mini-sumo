#ifndef JOLTIK_MOTORS_H
#define JOLTIK_MOTORS_H

int motors_init(void);
void drive(int left, int right);
void motors_stop(void);

#endif
