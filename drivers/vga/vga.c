#include "vga.h"
#include "io.h"
#include <stdint.h>

#define VGA_ADDRESS   0xB8000
#define VGA_WIDTH     80
#define VGA_HEIGHT    25

static uint16_t *const vga_buffer = (uint16_t *)VGA_ADDRESS;
static int cursor_row = 0;
static int cursor_col = 0;
static uint8_t current_colour = 0x07; /* white on black */

static inline uint16_t vga_entry(char c, uint8_t colour) {
    return (uint16_t)c | ((uint16_t)colour << 8);
}

static void vga_update_hw_cursor(void) {
    uint16_t pos = cursor_row * VGA_WIDTH + cursor_col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static void vga_scroll(void) {
    if (cursor_row < VGA_HEIGHT) return;

    for (int row = 1; row < VGA_HEIGHT; row++) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            vga_buffer[(row - 1) * VGA_WIDTH + col] = vga_buffer[row * VGA_WIDTH + col];
        }
    }
    for (int col = 0; col < VGA_WIDTH; col++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = vga_entry(' ', current_colour);
    }
    cursor_row = VGA_HEIGHT - 1;
}

void vga_init(void) {
    current_colour = 0x07;
    vga_clear();
}

void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = vga_entry(' ', current_colour);
    }
    cursor_row = 0;
    cursor_col = 0;
    vga_update_hw_cursor();
}

void vga_set_colour(vga_colour_t fg, vga_colour_t bg) {
    current_colour = (uint8_t)((bg << 4) | (fg & 0x0F));
}

void vga_putc(char c) {
    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
    } else if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
            vga_buffer[cursor_row * VGA_WIDTH + cursor_col] = vga_entry(' ', current_colour);
        }
    } else {
        vga_buffer[cursor_row * VGA_WIDTH + cursor_col] = vga_entry(c, current_colour);
        cursor_col++;
        if (cursor_col >= VGA_WIDTH) {
            cursor_col = 0;
            cursor_row++;
        }
    }
    vga_scroll();
    vga_update_hw_cursor();
}

void vga_puts(const char *s) {
    while (*s) {
        vga_putc(*s++);
    }
}

/* Write directly to a fixed cell without touching the shared cursor.
 * Used by background demo processes so they don't scramble shell output. */
void vga_put_at(int row, int col, char c, unsigned char colour) {
    if (row < 0 || row >= VGA_HEIGHT || col < 0 || col >= VGA_WIDTH) return;
    vga_buffer[row * VGA_WIDTH + col] = vga_entry(c, colour);
}
