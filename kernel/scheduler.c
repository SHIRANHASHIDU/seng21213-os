#include "scheduler.h"
#include "process.h"
#include "io.h"
#include <stdint.h>

/* Provided by boot/switch.asm */
extern void irq0_handler_asm(void);
extern void scheduler_launch_first(uint32_t esp);

struct idt_entry {
    uint16_t base_lo;
    uint16_t sel;
    uint8_t  zero;
    uint8_t  flags;
    uint16_t base_hi;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr   idtp;

static int rr_index = -1;

/* ---- 8259 PIC remap: IRQ0-7 -> INT 0x20-0x27, IRQ8-15 -> INT 0x28-0x2F ---- */
static void pic_remap(void) {
    outb(0x20, 0x11); outb(0xA0, 0x11); /* ICW1: init, cascade, edge  */
    outb(0x21, 0x20); outb(0xA1, 0x28); /* ICW2: vector offsets       */
    outb(0x21, 0x04); outb(0xA1, 0x02); /* ICW3: master/slave wiring  */
    outb(0x21, 0x01); outb(0xA1, 0x01); /* ICW4: 8086 mode            */

    outb(0x21, 0xFE); /* mask all master IRQs except IRQ0 (timer)      */
    outb(0xA1, 0xFF); /* mask all slave IRQs                           */
}

/* ---- i8253 PIT: channel 0, square wave, target frequency in Hz ---------- */
static void pit_init(uint32_t freq) {
    uint32_t divisor = 1193182 / freq;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

static void idt_set_gate(int n, uint32_t handler) {
    idt[n].base_lo = (uint16_t)(handler & 0xFFFF);
    idt[n].base_hi = (uint16_t)((handler >> 16) & 0xFFFF);
    idt[n].sel     = 0x08;   /* kernel code selector */
    idt[n].zero    = 0;
    idt[n].flags   = 0x8E;   /* present, ring0, 32-bit interrupt gate */
}

static void idt_init(void) {
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0);
    }
    idt_set_gate(0x20, (uint32_t)irq0_handler_asm); /* IRQ0 -> timer tick */

    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (uint32_t)&idt;
    __asm__ volatile ("lidt %0" :: "m"(idtp));
}

pcb_t *scheduler_pick_next(void) {
    if (current_pcb && current_pcb->state == PROC_RUNNING) {
        current_pcb->state = PROC_READY;
    }

    for (int tries = 0; tries < MAX_PROCESSES; tries++) {
        rr_index = (rr_index + 1) % MAX_PROCESSES;
        if (process_table[rr_index].state == PROC_READY) {
            process_table[rr_index].state = PROC_RUNNING;
            return &process_table[rr_index];
        }
    }
    /* Nothing else runnable - keep whatever was running (idle fallback). */
    if (current_pcb) {
        current_pcb->state = PROC_RUNNING;
    }
    return current_pcb;
}

void scheduler_init(void) {
    pic_remap();
    idt_init();
    pit_init(100); /* 100 Hz = 10ms tick, per assignment spec */
}

void scheduler_start(void) {
    pcb_t *first = scheduler_pick_next();
    current_pcb = first;
    scheduler_launch_first(first->saved_esp); /* never returns */
}
