## ==========================================================================
## SENG21213 - Build system
## ==========================================================================

BUILD       := build
KERN_DIR    := kernel
DRV_DIR     := drivers
BOOT_DIR    := boot

AS          := nasm
CC          := gcc
LD          := ld

ASFLAGS_BIN := -f bin
ASFLAGS_ELF := -f elf32
CFLAGS      := -m32 -ffreestanding -fno-pie -fno-pic -fno-stack-protector \
               -nostdlib -Iinclude -Wall -Wextra -c
LDFLAGS     := -m elf_i386 -T linker.ld -nostdlib

# ---- Sources -------------------------------------------------------------
# Add new .c files here as you complete each stage.
KERN_SRCS := \
    $(KERN_DIR)/kernel.c \
    $(KERN_DIR)/shell.c \
    $(KERN_DIR)/string.c \
    $(KERN_DIR)/process.c \
    $(KERN_DIR)/scheduler.c \
    $(KERN_DIR)/mutex.c \
    $(KERN_DIR)/semaphore.c \
    $(KERN_DIR)/thread.c \
    $(KERN_DIR)/pmm.c \
    $(DRV_DIR)/vga/vga.c \
    $(DRV_DIR)/keyboard/keyboard.c

# Add new NASM (elf32) sources here (e.g. boot/switch.asm in Stage 1).
BOOT_ASM_ELF := \
    $(BOOT_DIR)/kernel_entry.asm \
    $(BOOT_DIR)/switch.asm

KERN_OBJS := $(patsubst %.c,$(BUILD)/%.o,$(KERN_SRCS))
BOOT_OBJS := $(patsubst $(BOOT_DIR)/%.asm,$(BUILD)/boot/%.o,$(BOOT_ASM_ELF))

IMG := seng21213.img

.PHONY: all run debug gdb clean info

all: $(IMG)

# ---- Boot sector (flat binary, 512 bytes) --------------------------------
$(BUILD)/boot/boot.bin: $(BOOT_DIR)/boot.asm | $(BUILD)/boot
	@echo " AS  $<"
	@$(AS) $(ASFLAGS_BIN) $< -o $@
	@size=$$(stat -c%s $@ 2>/dev/null || stat -f%z $@); \
	 echo " Boot sector: $$size bytes (must be 512)"

# ---- Boot-stage elf32 objects (entry stub, context switch, etc.) --------
$(BUILD)/boot/%.o: $(BOOT_DIR)/%.asm | $(BUILD)/boot
	@echo " AS  $<"
	@$(AS) $(ASFLAGS_ELF) $< -o $@

# ---- C sources -------------------------------------------------------------
$(BUILD)/%.o: %.c | $(BUILD)
	@mkdir -p $(dir $@)
	@echo " CC  $<"
	@$(CC) $(CFLAGS) $< -o $@

# ---- Link kernel.elf -------------------------------------------------------
$(BUILD)/kernel.elf: $(BOOT_OBJS) $(KERN_OBJS)
	@echo " LD  $@"
	@$(LD) $(LDFLAGS) -o $@ $(BOOT_OBJS) $(KERN_OBJS)

# ---- Flatten to raw binary --------------------------------------------------
$(BUILD)/kernel.bin: $(BUILD)/kernel.elf
	@echo " BIN $@"
	@objcopy -O binary $< $@
	@# pad kernel.bin so the boot sector always loads a fixed, safe sector count
	@truncate -s 65536 $@ 2>/dev/null || dd if=/dev/zero bs=1 count=0 seek=65536 of=$@ 2>/dev/null

# ---- Final disk image: boot sector + kernel --------------------------------
$(IMG): $(BUILD)/boot/boot.bin $(BUILD)/kernel.bin
	@echo " IMG $(IMG)"
	@cat $(BUILD)/boot/boot.bin $(BUILD)/kernel.bin > $(IMG)
	@size=$$(stat -c%s $(IMG) 2>/dev/null || stat -f%z $(IMG)); \
	 echo " Disk image: $$((size/1024))K"

$(BUILD) $(BUILD)/boot:
	@mkdir -p $@

run: $(IMG)
	qemu-system-i386 -drive format=raw,file=$(IMG) -serial stdio

debug: $(IMG)
	@echo "QEMU [debug mode] -- waiting for GDB on :1234"
	qemu-system-i386 -drive format=raw,file=$(IMG) -s -S

gdb:
	gdb -ex "symbol-file $(BUILD)/kernel.elf" \
	    -ex "target remote localhost:1234" \
	    -ex "break kernel_main" \
	    -ex "continue"

info: $(BUILD)/kernel.elf
	objdump -h $(BUILD)/kernel.elf
	nm $(BUILD)/kernel.elf | sort

clean:
	rm -rf $(BUILD) $(IMG)
