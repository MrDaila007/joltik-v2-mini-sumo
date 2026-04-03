#ifndef JOLTIK_RC5_H
#define JOLTIK_RC5_H

#include <stdint.h>
#include <stdbool.h>

#define RC5_CMD_TACTIC_1  1
#define RC5_CMD_TACTIC_2  2
#define RC5_CMD_TACTIC_3  3
#define RC5_CMD_TACTIC_4  4
#define RC5_CMD_POWER     12

struct rc5_message {
	uint8_t toggle;
	uint8_t address;
	uint8_t command;
};

int rc5_init(void);
bool rc5_get_message(struct rc5_message *msg, int32_t timeout_ms);

#endif
