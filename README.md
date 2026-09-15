# SENG21213 - Operating Systems Assignment

A freestanding x86-32 kernel built from scratch in C99 and NASM, following the
SENG21213 Stage 0-4 assignment (boot sector -> scheduler -> threads -> memory
manager -> file system). Runs inside QEMU.

## Current status

- [x] Stage 0 - Boot, VGA & Shell (`v0.1-stage0`)
- [x] Stage 1 - Process Table & Round-Robin Scheduler (`v0.2-stage1`)
- [ ] Stage 2 - Threads, Mutex & Semaphore
- [ ] Stage 3 - Physical Memory Manager
- [ ] Stage 4 - RAM Disk File System

## Build & run

Requires `nasm`, `gcc` (with `gcc-multilib`), `binutils`, and
`qemu-system-i386`.

```bash
make          # builds seng21213.img
make run      # builds (if needed) and boots in QEMU
make debug    # boots paused, waiting for GDB on :1234
make gdb      # attach GDB to a running `make debug` session
make clean    # remove build/ and seng21213.img
```

## Project layout

```
boot/               MBR bootloader (boot.asm) + protected-mode entry stub
kernel/             kernel_main, shell, string library
drivers/vga/        VGA text-mode driver (writes to 0xB8000)
drivers/keyboard/   PS/2 keyboard driver (polling, Set-1 scancodes)
include/            Public headers
linker.ld           Places the kernel at physical 0x10000
Makefile            Build system
run.sh              Convenience launcher (make + qemu)
```

## Stage 0 - Boot, VGA & Shell

- 512-byte MBR bootloader: real mode -> enables A20 -> loads kernel via
  `INT 0x13` -> installs a 3-entry flat GDT -> switches to 32-bit protected
  mode -> jumps into the kernel at `0x10000`.
- VGA text-mode driver with scrolling and colour support, writing directly
  to `0xB8000`.
- PS/2 keyboard driver (polling), Set-1 scancode -> ASCII translation.
- Command-table driven shell with 6 built-in commands: `help`, `clear`,
  `echo`, `version`, `colour`, `halt`.

### Testing

```bash
make run
```

Type `help` at the `kernel>` prompt to see all commands. `colour 14 1` sets
yellow-on-blue text. `halt` stops the CPU cleanly.

## Stage 1 - Process Table & Round-Robin Scheduler

- `pcb_t` process control block (`include/process.h`): PID, state, saved
  stack pointer, name, and a dedicated 4 KB stack per process.
- `create_process(entry, name)` builds a synthetic interrupt stack frame
  so a brand-new process can be "resumed" by the generic context-switch
  epilogue on its very first run.
- The i8253 PIT is programmed to fire IRQ0 at 100 Hz (10 ms tick),
  vectored through a 256-entry IDT after remapping the 8259 PIC.
- `boot/switch.asm` implements the actual context switch: on every timer
  tick it saves the interrupted process's full register state onto its
  own stack, asks the C scheduler (`scheduler_pick_next`) which process
  runs next, switches to that process's stack, restores its registers,
  and `iret`s into it.
- Round-robin scheduling: `kernel_main` starts three processes - the
  interactive shell, and two demo processes (`proc_a`, `proc_b`) that
  each print an incrementing counter to a fixed screen cell at different
  rates, proving true preemptive concurrency.
- New shell commands: `ps` (lists PID / state / name for every process)
  and `kill <pid>` (marks a process `TERMINATED` so the scheduler skips
  it going forward).

### Testing

```bash
make run
```

Watch the top-right corner of the screen: `A <counter>` and `B <counter>`
should both be incrementing simultaneously, at different speeds, while
the shell prompt stays fully responsive. Then try:

```
kernel> ps
PID  STATE       NAME
1    RUNNING      shell
2    READY      proc_a
3    READY      proc_b

kernel> kill 2
killed pid 2

kernel> ps
PID  STATE       NAME
1    RUNNING      shell
2    TERMINATED      proc_a
3    READY      proc_b
```

After `kill 2`, process A's on-screen counter should freeze while B keeps
incrementing - confirming the scheduler actually stopped dispatching it.

## Extensions

_(document any bonus extensions here as you add them, per stage, with how to
test them and which lecture concept they demonstrate)_

## Debugging

```bash
# terminal 1
make debug
# terminal 2
make gdb
```

See the assignment guide for the full GDB command reference and common crash
scenarios (triple-fault reboot loops, blank screen, unresponsive keyboard).
