#include "process.h"
#include "string.h"

pcb_t  process_table[MAX_PROCESSES];
pcb_t *current_pcb = 0;

static int next_pid = 1;

void process_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].state = PROC_UNUSED;
        process_table[i].pid = 0;
        process_table[i].name[0] = '\0';
    }
    current_pcb = 0;
    next_pid = 1;
}

/*
 * Builds a synthetic interrupt stack frame so that the generic IRQ0
 * epilogue in switch.asm (pop gs/fs/es/ds ; popad ; iretd) can "resume"
 * a process that has never actually run yet, landing it at `entry` in
 * ring 0 with interrupts enabled (EFLAGS.IF = 1).
 *
 * Layout, low address (= saved_esp, popped first) -> high address
 * (popped last by iretd):
 *   GS, FS, ES, DS, EDI, ESI, EBP, ESP(dummy), EBX, EDX, ECX, EAX,
 *   EIP, CS, EFLAGS
 */
int create_process(void (*entry)(void), const char *name) {
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROC_UNUSED) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        return -1; /* process table full */
    }

    pcb_t *p = &process_table[slot];
    p->pid = next_pid++;
    p->state = PROC_READY;
    strcpy(p->name, name);

    uint32_t *sp = (uint32_t *)(p->stack + PROC_STACK_SIZE);

    *(--sp) = 0x00000202;      /* EFLAGS: IF=1, reserved bit 1 set        */
    *(--sp) = 0x08;            /* CS: kernel code selector (see boot.asm) */
    *(--sp) = (uint32_t)entry; /* EIP: process entry point                */
    *(--sp) = 0;               /* EAX (popad)                             */
    *(--sp) = 0;               /* ECX                                     */
    *(--sp) = 0;               /* EDX                                     */
    *(--sp) = 0;               /* EBX                                     */
    *(--sp) = 0;               /* ESP dummy slot - ignored by popad       */
    *(--sp) = 0;               /* EBP                                     */
    *(--sp) = 0;               /* ESI                                     */
    *(--sp) = 0;               /* EDI                                     */
    *(--sp) = 0x10;            /* DS: kernel data selector                */
    *(--sp) = 0x10;            /* ES                                      */
    *(--sp) = 0x10;            /* FS                                      */
    *(--sp) = 0x10;            /* GS                                      */

    p->saved_esp = (uint32_t)sp;
    return p->pid;
}
