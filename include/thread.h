#ifndef THREAD_H
#define THREAD_H

int thread_create(void (*fn)(void), const char *name);

#endif
