#ifndef JOLTIK_COMBAT_H
#define JOLTIK_COMBAT_H

#include <stdint.h>

#define DIR_LEFT  0
#define DIR_RIGHT 1

void attack(void);
void backoff(uint8_t dir);
void search(void);

/* Speed ramping counter for full-sensor attack */
void combat_reset_count(void);

#endif
