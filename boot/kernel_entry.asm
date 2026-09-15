; ==========================================================================
; kernel_entry.asm - first code linked at 0x10000.
; Guarantees kernel_main() is the real entry point regardless of C file
; compilation order, and provides a safe halt loop if kernel_main returns.
; ==========================================================================
[BITS 32]
global _start
extern kernel_main

section .text
_start:
    mov esp, 0x90000
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang

section .note.GNU-stack noalloc noexec nowrite progbits
