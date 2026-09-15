#ifndef SEMAPHORE_H
#define SEMAPHORE_H

typedef struct {
    volatile int count;
} semaphore_t;

void sem_init(semaphore_t *s, int initial);
void sem_wait(semaphore_t *s);
void sem_signal(semaphore_t *s);

#endif
