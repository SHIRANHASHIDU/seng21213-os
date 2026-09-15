#include "keyboard.h"
#include "io.h"
#include <stdint.h>

#define KBD_DATA_PORT   0x60
#define KBD_STATUS_PORT 0x64
#define KBD_OUTPUT_FULL 0x01

/* Scancode Set 1 -> ASCII, unshifted. 0 = unmapped/ignored. */
static const char scancode_ascii[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0 /*ctrl*/, 'a','s','d','f','g','h','j','k','l',';','\'','`',
    0 /*lshift*/, '\\','z','x','c','v','b','n','m',',','.','/',
    0 /*rshift*/, '*', 0 /*alt*/, ' ', 0 /*capslock*/,
    0,0,0,0,0,0,0,0,0,0, /* F1-F10 */
    0 /*numlock*/, 0 /*scrolllock*/, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

void keyboard_init(void) {
    /* Polling mode - drain any pending byte in the controller buffer. */
    while (inb(KBD_STATUS_PORT) & KBD_OUTPUT_FULL) {
        inb(KBD_DATA_PORT);
    }
}

char keyboard_getchar(void) {
    for (;;) {
        if (inb(KBD_STATUS_PORT) & KBD_OUTPUT_FULL) {
            uint8_t scancode = inb(KBD_DATA_PORT);

            /* Ignore key-release events (top bit set) */
            if (scancode & 0x80) {
                continue;
            }
            char c = scancode_ascii[scancode & 0x7F];
            if (c != 0) {
                return c;
            }
        }
    }
}
