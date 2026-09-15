; ==========================================================================
; switch.asm - Stage 1 context switch
;
; irq0_handler_asm:
;   Fires 100x/second from the PIT. Saves the interrupted process's full
;   register state onto ITS OWN stack, saves that stack pointer into the
;   pcb_t current_pcb points to (saved_esp is guaranteed to be the first
;   field - see include/process.h), asks the C scheduler which pcb_t runs
;   next, switches ESP to that process's saved stack, restores its
;   registers, and irets into it. This is a full preemptive context switch.
;
; scheduler_launch_first:
;   Called once from C (never returns). Loads a brand-new process's
;   synthetic stack frame (built by create_process in process.c) and
;   irets into it for the very first time.
; ==========================================================================
[BITS 32]

global irq0_handler_asm
global scheduler_launch_first
extern scheduler_pick_next
extern current_pcb

section .text

irq0_handler_asm:
    pushad                  ; EAX,ECX,EDX,EBX,ESP(dummy),EBP,ESI,EDI
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10             ; kernel data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; save current process's stack pointer into its pcb (saved_esp @ offset 0)
    mov eax, [current_pcb]
    test eax, eax
    jz .skip_save
    mov [eax], esp
.skip_save:

    call scheduler_pick_next ; cdecl, no args -> returns pcb_t* in eax
    mov [current_pcb], eax
    mov esp, [eax]            ; switch to the chosen process's stack

    mov al, 0x20               ; EOI to master PIC
    out 0x20, al

    pop gs
    pop fs
    pop es
    pop ds
    popad

    iretd

; void scheduler_launch_first(uint32_t esp) -- cdecl, never returns
scheduler_launch_first:
    mov eax, [esp + 4]        ; first arg = target esp
    mov esp, eax
    pop gs
    pop fs
    pop es
    pop ds
    popad
    iretd

section .note.GNU-stack noalloc noexec nowrite progbits
global schedule
extern current_pcb
extern scheduler_pick_next

section .text
global schedule
extern current_pcb
extern scheduler_pick_next

schedule:
    pushad
    push gs
    push fs
    push es
    push ds

    mov  eax, [current_pcb]
    mov  [eax], esp          ; pcb->saved_esp = esp

    call scheduler_pick_next
    mov  [current_pcb], eax
    mov  esp, [eax]          ; esp = new_pcb->saved_esp

    pop  ds
    pop  es
    pop  fs
    pop  gs
    popad
    ret
