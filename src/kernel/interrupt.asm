; AIkOS interrupt stubs — one per vector, common entry, C handler.
; ADR-007/009. Vectors 0-31 = CPU exceptions, 32-47 = IRQs (PIC remap),
; 48-255 reserved (unhandled -> panic).
; isr_addr_table: 256 addresses in vector order, consumed by idt.c.

[BITS 64]
section .text

extern isr_handler

; --- per-vector stubs ---
%macro ISR_NOERR 1
global isr%1
isr%1:
    push qword 0
    push qword %1
    jmp isr_common
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    push qword %1
    jmp isr_common
%endmacro

; CPU exceptions that push an error code: 8, 10-14, 17
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR 8
ISR_NOERR 9
ISR_ERR 10
ISR_ERR 11
ISR_ERR 12
ISR_ERR 13
ISR_ERR 14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

; vectors 32-255: no error code
%assign i 32
%rep 224
ISR_NOERR i
%assign i i+1
%endrep

; --- common entry: save GPRs, call isr_handler(struct isr_frame*), restore ---
; Stack at entry: [vector][error_code][rip][cs][rflags][rsp][ss]
; After the 15 pushes, rsp points at r15; the layout matches struct isr_frame
; in idt.c exactly (r15 first ... rax, vector, error_code, rip, cs, rflags,
; rsp, ss last).
isr_common:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    mov rbx, rsp            ; struct pointer (C code preserves rbx)
    and rsp, -16            ; 16-byte align for the ABI (handler is C)
    mov rdi, rbx
    call isr_handler
    mov rsp, rbx
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    add rsp, 16             ; drop vector + error code
    iretq

; --- address table for idt.c (one entry per vector, vector order) ---
section .data
global isr_addr_table
isr_addr_table:
%assign i 0
%rep 256
    dq isr %+ i
%assign i i+1
%endrep
