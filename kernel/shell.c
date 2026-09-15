#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "kernel.h"
#include "process.h"

#define CMD_BUF_SIZE 128
#define MAX_ARGS     8

typedef void (*cmd_handler_t)(int argc, char **argv);

typedef struct {
    const char    *name;
    const char    *help;
    cmd_handler_t  handler;
} shell_command_t;

static void cmd_help(int argc, char **argv);
static void cmd_clear(int argc, char **argv);
static void cmd_echo(int argc, char **argv);
static void cmd_version(int argc, char **argv);
static void cmd_colour(int argc, char **argv);
static void cmd_halt(int argc, char **argv);
static void cmd_ps(int argc, char **argv);
static void cmd_kill(int argc, char **argv);

static const shell_command_t commands[] = {
    { "help",    "List all available commands",            cmd_help    },
    { "clear",   "Clear the screen",                       cmd_clear   },
    { "echo",    "<text> - print arguments back",          cmd_echo    },
    { "version", "Print kernel name and version",          cmd_version },
    { "colour",  "<fg> <bg> - change text colour (0-15)",  cmd_colour  },
    { "halt",    "Disable interrupts and halt the CPU",    cmd_halt    },
    { "ps",      "List all processes (PID, state, name)",  cmd_ps      },
    { "kill",    "<pid> - terminate a process by PID",     cmd_kill    },
};
#define NUM_COMMANDS (sizeof(commands) / sizeof(commands[0]))

static void print_banner(void) {
    vga_puts("+=================================================+\n");
    vga_puts("|          SENG 21213 - Stage 0 Kernel Shell       |\n");
    vga_puts("|              University of Kelaniya              |\n");
    vga_puts("|         Type 'help' for available commands       |\n");
    vga_puts("+=================================================+\n");
}

static int simple_atoi(const char *s) {
    int val = 0;
    while (*s >= '0' && *s <= '9') {
        val = val * 10 + (*s - '0');
        s++;
    }
    return val;
}

static void cmd_help(int argc, char **argv) {
    (void)argc; (void)argv;
    for (size_t i = 0; i < NUM_COMMANDS; i++) {
        vga_puts(commands[i].name);
        vga_puts(" - ");
        vga_puts(commands[i].help);
        vga_putc('\n');
    }
}

static void cmd_clear(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_clear();
}

static void cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        vga_puts(argv[i]);
        if (i < argc - 1) vga_putc(' ');
    }
    vga_putc('\n');
}

static void cmd_version(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_puts(KERNEL_VERSION);
    vga_putc('\n');
}

static void cmd_colour(int argc, char **argv) {
    if (argc != 3) {
        vga_puts("usage: colour <fg 0-15> <bg 0-15>\n");
        return;
    }
    int fg = simple_atoi(argv[1]);
    int bg = simple_atoi(argv[2]);
    if (fg < 0 || fg > 15 || bg < 0 || bg > 15) {
        vga_puts("colour values must be 0-15\n");
        return;
    }
    vga_set_colour((vga_colour_t)fg, (vga_colour_t)bg);
    vga_puts("colour updated\n");
}

static void cmd_halt(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_puts("Halting CPU. Goodbye.\n");
    __asm__ volatile ("cli");
    __asm__ volatile ("hlt");
    for (;;) { __asm__ volatile ("hlt"); }
}

static const char *state_name(proc_state_t s) {
    switch (s) {
        case PROC_READY:      return "READY";
        case PROC_RUNNING:    return "RUNNING";
        case PROC_TERMINATED: return "TERMINATED";
        default:               return "UNUSED";
    }
}

static void cmd_ps(int argc, char **argv) {
    (void)argc; (void)argv;
    vga_puts("PID  STATE       NAME\n");
    for (int i = 0; i < MAX_PROCESSES; i++) {
        pcb_t *p = &process_table[i];
        if (p->state == PROC_UNUSED) continue;

        char pidbuf[8];
        int v = p->pid, j = 0;
        char tmp[8];
        if (v == 0) { tmp[j++] = '0'; }
        while (v > 0) { tmp[j++] = (char)('0' + (v % 10)); v /= 10; }
        int k = 0;
        while (j > 0) pidbuf[k++] = tmp[--j];
        pidbuf[k] = '\0';

        vga_puts(pidbuf);
        vga_puts("    ");
        vga_puts(state_name(p->state));
        vga_puts("      ");
        vga_puts(p->name);
        vga_putc('\n');
    }
}

static void cmd_kill(int argc, char **argv) {
    if (argc != 2) {
        vga_puts("usage: kill <pid>\n");
        return;
    }
    int target = simple_atoi(argv[1]);
    for (int i = 0; i < MAX_PROCESSES; i++) {
        pcb_t *p = &process_table[i];
        if (p->state != PROC_UNUSED && p->pid == target) {
            if (p == current_pcb) {
                vga_puts("cannot kill the shell's own process\n");
                return;
            }
            p->state = PROC_TERMINATED;
            vga_puts("killed pid ");
            vga_puts(argv[1]);
            vga_putc('\n');
            return;
        }
    }
    vga_puts("no such pid: ");
    vga_puts(argv[1]);
    vga_putc('\n');
}

static void read_line(char *buf, int max_len) {
    int len = 0;
    for (;;) {
        char c = keyboard_getchar();
        if (c == '\n') {
            vga_putc('\n');
            buf[len] = '\0';
            return;
        } else if (c == '\b') {
            if (len > 0) {
                len--;
                vga_putc('\b');
            }
        } else if (len < max_len - 1) {
            buf[len++] = c;
            vga_putc(c);
        }
    }
}

static void dispatch(char *line) {
    char *argv[MAX_ARGS];
    int argc = 0;
    char *saveptr;

    char *tok = strtok_ws(line, &saveptr);
    while (tok && argc < MAX_ARGS) {
        argv[argc++] = tok;
        tok = strtok_ws(NULL, &saveptr);
    }

    if (argc == 0) return;

    for (size_t i = 0; i < NUM_COMMANDS; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            commands[i].handler(argc, argv);
            return;
        }
    }

    vga_puts("Unknown command: ");
    vga_puts(argv[0]);
    vga_puts(" (type 'help')\n");
}

void shell_run(void) {
    char line[CMD_BUF_SIZE];

    print_banner();

    for (;;) {
        vga_puts("kernel> ");
        read_line(line, CMD_BUF_SIZE);
        dispatch(line);
    }
}
