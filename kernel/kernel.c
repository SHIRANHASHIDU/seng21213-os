#include "kernel.h"
#include "vga.h"
#include "keyboard.h"
#include "shell.h"
#include "process.h"
#include "scheduler.h"
#include "io.h"

/* Small itoa - no libc available. Writes value as decimal into buf. */
static void uint_to_str(uint32_t val, char *buf) {
    char tmp[12];
    int i = 0;
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    while (val > 0) {
        tmp[i++] = (char)('0' + (val % 10));
        val /= 10;
    }
    int j = 0;
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
}

/* ---- Demo process A: prints a running counter at a fixed screen cell ---- */
static void proc_a_entry(void) {
    uint32_t counter = 0;
    char buf[12];
    for (;;) {
        vga_put_at(0, 70, 'A', 0x0A); /* light green */
        uint_to_str(counter, buf);
        for (int i = 0; i < 8; i++) {
            vga_put_at(0, 72 + i, (buf[i] ? buf[i] : ' '), 0x0A);
        }
        counter++;
        for (volatile int d = 0; d < 1500000; d++); /* fast-ish spin delay */
    }
}

/* ---- Demo process B: same idea, different cell and a slower rate ---- */
static void proc_b_entry(void) {
    uint32_t counter = 0;
    char buf[12];
    for (;;) {
        vga_put_at(1, 70, 'B', 0x0C); /* light red */
        uint_to_str(counter, buf);
        for (int i = 0; i < 8; i++) {
            vga_put_at(1, 72 + i, (buf[i] ? buf[i] : ' '), 0x0C);
        }
        counter++;
        for (volatile int d = 0; d < 4000000; d++); /* slower spin delay */
    }
}

/* ---- The interactive shell runs as its own process ---- */
static void shell_entry(void) {
    shell_run();
    kernel_halt();
}

void kernel_main(void) {
    vga_init();
    vga_puts("[ OK ] VGA driver initialised\n");

    keyboard_init();
    vga_puts("[ OK ] PS/2 keyboard driver initialised\n");

    process_init();
    create_process(shell_entry, "shell");
    create_process(proc_a_entry, "proc_a");
    create_process(proc_b_entry, "proc_b");
    vga_puts("[ OK ] Process table initialised (shell, proc_a, proc_b)\n");

    scheduler_init();
    vga_puts("[ OK ] Scheduler initialised (PIT @ 100Hz, round-robin)\n\n");

    /* Interrupts are enabled implicitly: create_process() sets EFLAGS.IF=1
     * in each process's synthetic frame, so scheduler_start()'s iretd turns
     * interrupts on the moment it lands in the first process. */
    scheduler_start(); /* never returns */

    kernel_halt();
}

void kernel_halt(void) {
    __asm__ volatile ("cli");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
