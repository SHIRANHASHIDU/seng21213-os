#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

/* Sets up the PIC remap, IDT/IRQ0 gate, and programs the PIT for 100 Hz. */
void scheduler_init(void);

/* Never returns: loads the first READY process and jumps into it. */
void scheduler_start(void);

/* Called ONLY from the IRQ0 assembly stub. Round-robin picks the next
 * READY process, marks it RUNNING, and returns a pointer to its pcb_t. */
pcb_t *scheduler_pick_next(void);

#endif
