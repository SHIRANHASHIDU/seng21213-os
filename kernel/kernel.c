#include "kernel.h"
#include "vga.h"
#include "keyboard.h"
#include "shell.h"

void kernel_main(void) {
    vga_init();
    vga_puts("[ OK ] VGA driver initialised\n");

    keyboard_init();
    vga_puts("[ OK ] PS/2 keyboard driver initialised\n\n");

    shell_run();

    /* shell_run() only returns if something goes very wrong */
    kernel_halt();
}

void kernel_halt(void) {
    __asm__ volatile ("cli");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
