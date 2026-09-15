#include "thread.h"
#include "process.h"

int thread_create(void (*fn)(void), const char *name) {
    return create_process(fn, name);
}
