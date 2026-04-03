#ifndef JOLTIK_BUTTON_H
#define JOLTIK_BUTTON_H

enum button_event {
	BTN_NONE,
	BTN_SINGLE,
	BTN_DOUBLE,
};

int button_init(void);
enum button_event button_poll(void);

#endif
