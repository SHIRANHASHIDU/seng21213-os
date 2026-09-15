#include "semaphore.h"
#include "process.h"
#include "io.h"

void sem_init(semaphore_t *s, int initial) {
    s->count = initial;
}

void sem_wait(semaphore_t *s) {
    disable_interrupts();
    while (s->count <= 0) {
        enable_interrupts();
        block_current_and_reschedule(PROC_BLOCKED);
        disable_interrupts();
    }
    s->count--;
    enable_interrupts();
}

void sem_signal(semaphore_t *s) {
    disable_interrupts();
    s->count++;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROC_BLOCKED) {
            process_table[i].state = PROC_READY;
        }
    }
    enable_interrupts();
}
