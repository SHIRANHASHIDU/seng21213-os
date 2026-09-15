\; ==========================================================================
; SENG21213 Stage 0 - MBR Bootloader
; 512-byte boot sector: real mode -> enable A20 -> load kernel -> protected mode
; ==========================================================================
[BITS 16]
[ORG 0x7C00]

KERNEL_LOAD_SEG equ 0x1000     ; ES segment kernel is loaded to -> phys 0x10000
KERNEL_SECTORS  equ 128        ; sectors to read (128 * 512 = 64 KB, plenty for now)

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [BOOT_DRIVE], dl        ; BIOS passes boot drive in DL

    mov si, msg_loading
    call print_string_16

    ; ---- Load kernel from disk (CHS, sectors 2..N) ----
    mov ax, KERNEL_LOAD_SEG
    mov es, ax
    xor bx, bx                  ; ES:BX = 0x1000:0000 = physical 0x10000

    mov ah, 0x02                ; BIOS: read sectors
    mov al, KERNEL_SECTORS
    mov ch, 0                   ; cylinder 0
    mov cl, 2                   ; start at sector 2 (sector 1 = boot sector)
    mov dh, 0                   ; head 0
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error

    ; ---- L11: Detect physical memory via BIOS int 0x15, EAX=0xE820 ----
    ; Must run here, in real mode, before the Protected Mode switch below.
    ; Raw 24-byte entries stored at physical 0x9000; entry count stored
    ; as a word at physical 0x8FF0. Read back by kernel/pmm.c later.
    xor ax, ax
    mov es, ax              ; ES=0 (disk load left ES=0x1000)
    mov di, 0x9000
    xor ebx, ebx             ; continuation value, 0 to start
    xor bp, bp               ; entry count

.e820_loop:
    mov eax, 0xE820
    mov edx, 0x534D4150      ; 'SMAP'
    mov ecx, 24
    int 0x15
    jc .e820_done
    cmp eax, 0x534D4150
    jne .e820_done
    cmp bp, 32
    jae .e820_done
    add di, 24
    inc bp
    test ebx, ebx
    jnz .e820_loop
.e820_done:
    mov [0x8FF0], bp

    ; ---- Enable A20 line (fast A20 gate) ----
    in al, 0x92
    or al, 2
    out 0x92, al

    mov si, msg_pm
    call print_string_16

    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp CODE_SEG:protected_mode_start

disk_error:
    mov si, msg_disk_err
    call print_string_16
    jmp $

; ---- 16-bit teletype string print (BIOS INT 10h) ----
print_string_16:
    pusha
    mov ah, 0x0E
.loop:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .loop
.done:
    popa
    ret

BOOT_DRIVE: db 0
msg_loading: db "SENG21213 Stage 0 - Loading kernel...", 13, 10, 0
msg_pm:      db "Kernel loaded. Switching to Protected Mode.", 13, 10, 0
msg_disk_err: db "DISK READ ERROR", 13, 10, 0

; ---- Global Descriptor Table (3 entries: null, code, data) ----
gdt_start:
gdt_null:
    dd 0x0
    dd 0x0
gdt_code:                       ; base=0, limit=4GB, 32-bit, ring0, executable
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 10011010b
    db 11001111b
    db 0x0
gdt_data:                       ; base=0, limit=4GB, 32-bit, ring0, writable
    dw 0xFFFF
    dw 0x0
    db 0x0
    db 10010010b
    db 11001111b
    db 0x0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

[BITS 32]
protected_mode_start:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000            ; kernel stack base

    jmp CODE_SEG:0x10000        ; jump into the loaded kernel (kernel_entry)

; ---- Pad to 510 bytes + boot signature ----
times 510 - ($ - $$) db 0
dw 0xAA55
