#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define MAX_PROCESSES   12
#define PROC_STACK_SIZE 4096
#define PROC_NAME_LEN   16

typedef enum {
    PROC_UNUSED = 0,
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_TERMINATED
} proc_state_t;

/*
 * IMPORTANT: saved_esp MUST remain the first field. boot/switch.asm reads
 * and writes it directly via `mov [eax], esp` / `mov esp, [eax]` where eax
 * holds a pcb_t* - i.e. it assumes offsetof(pcb_t, saved_esp) == 0.
 */
typedef struct pcb {
    uint32_t      saved_esp;
    int           pid;
    proc_state_t  state;
    char          name[PROC_NAME_LEN];
    uint8_t       stack[PROC_STACK_SIZE] __attribute__((aligned(16)));
} pcb_t;

extern pcb_t  process_table[MAX_PROCESSES];
extern pcb_t *current_pcb;

void process_init(void);
int  create_process(void (*entry)(void), const char *name);
void block_current_and_reschedule(proc_state_t reason);

#endif
