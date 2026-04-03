#ifndef JOLTIK_DISPLAY_H
#define JOLTIK_DISPLAY_H

#ifdef CONFIG_JOLTIK_DISPLAY
int display_init(void);
void display_request_update(void);
#else
static inline int display_init(void) { return 0; }
static inline void display_request_update(void) {}
#endif

#endif
