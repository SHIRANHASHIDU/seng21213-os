#include "mutex.h"
#include "process.h"
#include "io.h"

void mutex_init(mutex_t *m) {
    m->locked = 0;
    m->owner_pid = -1;
}

void mutex_lock(mutex_t *m) {
    disable_interrupts();
    while (m->locked) {
        enable_interrupts();
        block_current_and_reschedule(PROC_BLOCKED);
        disable_interrupts();
    }
    m->locked = 1;
    m->owner_pid = current_pcb->pid;
    enable_interrupts();
}

void mutex_unlock(mutex_t *m) {
    disable_interrupts();
    m->locked = 0;
    m->owner_pid = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROC_BLOCKED) {
            process_table[i].state = PROC_READY;
        }
    }
    enable_interrupts();
}
