#include "kernel.h"
#include "vga.h"
#include "keyboard.h"
#include "shell.h"
#include "../include/pmm.h"
#include "process.h"
#include "scheduler.h"
#include "io.h"
#include "mutex.h"

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

static void vga_puts_at(int row, int col, const char *msg, uint8_t colour) {
    int i = 0;
    while (msg[i]) {
        vga_put_at(row, col + i, msg[i], colour);
        i++;
    }
}

static void vga_write_field(int row, int col, const char *text, uint8_t colour, int width) {
    int i = 0;
    for (; text[i] && i < width; i++) {
        vga_put_at(row, col + i, text[i], colour);
    }
    for (; i < width; i++) {
        vga_put_at(row, col + i, ' ', colour);
    }
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

/* ---- Race-condition demo: unsafe vs mutex-protected shared counter ---- */
volatile int shared_counter = 0;
mutex_t counter_mutex;

volatile int unsafe_done = 0;
mutex_t unsafe_done_mutex;

volatile int safe_done = 0;
mutex_t safe_done_mutex;

static void racer_safe_entry(void); /* forward declaration */

static void racer_unsafe_entry(void) {
    for (int i = 0; i < 20000; i++) {
        int tmp = shared_counter;
        for (volatile int d = 0; d < 200; d++); /* widen the critical window so preemption reliably lands inside it */
        tmp++;
        shared_counter = tmp;
    }

    mutex_lock(&unsafe_done_mutex);
    unsafe_done++;
    int finished_count = unsafe_done;
    mutex_unlock(&unsafe_done_mutex);

        if (finished_count == 2) {
        char buf[12];
        uint_to_str(shared_counter, buf);
        vga_write_field(13, 0, "unsafe result: ", 0x0E, 16);
        vga_write_field(13, 16, buf, 0x0E, 12);

        shared_counter = 0;
        create_process(racer_safe_entry, "racer_safe_1");
        create_process(racer_safe_entry, "racer_safe_2");
    }

    block_current_and_reschedule(PROC_TERMINATED);
}

static void racer_safe_entry(void) {
    for (int i = 0; i < 20000; i++) {
        mutex_lock(&counter_mutex);
        int tmp = shared_counter;
        for (volatile int d = 0; d < 200; d++);
        tmp++;
        shared_counter = tmp;
        mutex_unlock(&counter_mutex);
    }

    mutex_lock(&safe_done_mutex);
    safe_done++;
    int finished_count = safe_done;
    mutex_unlock(&safe_done_mutex);

    if (finished_count == 2) {
        char buf[12];
        uint_to_str(shared_counter, buf);
        vga_write_field(14, 0, "mutex result:  ", 0x0A, 16);
        vga_write_field(14, 16, buf, 0x0A, 12);
    }

    block_current_and_reschedule(PROC_TERMINATED);
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

    mutex_init(&counter_mutex);
    mutex_init(&unsafe_done_mutex);
    mutex_init(&safe_done_mutex);
    create_process(racer_unsafe_entry, "racer_unsafe_1");
    create_process(racer_unsafe_entry, "racer_unsafe_2");
    vga_puts("[ OK ] Race-condition demo threads created\n");

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
