# SENG21213 - Operating Systems Assignment

A freestanding x86-32 kernel built from scratch in C99 and NASM, following the
SENG21213 Stage 0-4 assignment (boot sector -> scheduler -> threads -> memory
manager -> file system). Runs inside QEMU.

## Current status

- [x] Stage 0 - Boot, VGA & Shell (`v0.1-stage0`)
- [ ] Stage 1 - Process Table & Round-Robin Scheduler
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
