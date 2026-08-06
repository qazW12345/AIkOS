; AIkOS kernel entry — 32-bit entry, builds identity map, enters long mode,
; Component: entry (boot handoff + GDT + ring-3 trampoline)
; Provides: _start, kputc, gdt64, user_return
; Depends on: linker.ld (stack_top, _kernel_start/_kernel_end), boot.asm
;             (user/fault blobs in low buffers 0x10000/0x14000; AIkFS
;             partition in low buffer 0x18000, FS_SECTORS/FS_LBA defines)
; Owns: GDT64 (null 0x00, kcode 0x08, kdata 0x10, ucode 0x18, udata 0x20,
;       TSS slot 0x28 — 16-byte descriptor); page tables 0x9000-0xB000
;       (PML4/PDPT/PD identity map); long-mode setup; user blob copy-up
;       to 0x200000/0x220000; AIkFS ramdisk copy-up 0x18000->0x400000;
;       serial milestones 1-9,K
; jumps to kmain (C). ADR-006. Linked FIRST, load address 0x100000.
; Page tables: PML4 0x9000, PDPT 0xA000, PD 0xB000 (identity map, 1 GiB, 2 MiB pages).
; Serial milestones (COM1, debug): 1=entry 2=ptables 3=cr3 4=cr4 5=lme 6=pg 7=gdt64 8=longmode 9=stack K=kmain

[BITS 32]
global _start
extern kmain
extern stack_top          ; defined in linker.ld (.bss)

%ifndef USER_SECTORS
%define USER_SECTORS 16
%endif
%ifndef FAULT_SECTORS
%define FAULT_SECTORS 16
%endif
%ifndef FS_SECTORS
%define FS_SECTORS 64
%endif

%define PML4  0x9000
%define PDPT  0xA000
%define PD    0xB000

section .text
_start:
    cli
    mov al, '1'
    call serial_putc32

    ; --- zero the three page tables ---
    mov edi, PML4
    mov ecx, 0x3000 / 4
    xor eax, eax
    rep stosd

    ; --- PML4[0] = PDPT | present, rw ---
    mov dword [PML4 + 0], PDPT | 0x3
    ; --- PDPT[0] = PD | present, rw ---
    mov dword [PDPT + 0], PD | 0x3
    ; --- PD[i] = 2 MiB page at base i*2MiB | present, rw, PS ---
    mov eax, 0x83
    mov ecx, 512
    mov edi, PD
.fill_pd:
    mov [edi], eax
    add eax, 0x200000
    add edi, 8
    dec ecx
    jnz .fill_pd

    mov al, '2'
    call serial_putc32

    ; --- enable paging in the correct order: CR3 -> PAE -> LME -> PG ---
    mov eax, PML4
    mov cr3, eax
    mov al, '3'
    call serial_putc32
    mov eax, cr4
    or eax, 1 << 5               ; PAE
    mov cr4, eax
    mov al, '4'
    call serial_putc32
    mov ecx, 0xC0000080          ; EFER MSR
    rdmsr
    or eax, 1 << 8               ; LME (long mode enable)
    wrmsr
    mov al, '5'
    call serial_putc32
    mov eax, cr0
    or eax, 0x80000001           ; PG | PE
    mov cr0, eax
    mov al, '6'
    call serial_putc32

    ; --- load 64-bit GDT, far jump into long mode ---
    lgdt [gdt64_desc]
    mov al, '7'
    call serial_putc32
    jmp 0x08:long_mode

; --- debug: emit char in AL to COM1 (32-bit) ---
serial_putc32:
    push eax
    push edx
    mov dx, 0x3FD               ; LSR
.1:
    in al, dx
    test al, 0x20               ; THR empty?
    jz .1
    mov al, [esp + 4]           ; saved EAX ([esp]=EDX, [esp+4]=EAX)
    mov dx, 0x3F8
    out dx, al
    pop edx
    pop eax
    ret

[BITS 64]
long_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov al, '8'
    call serial_putc64
    lea rsp, [rel stack_top]     ; 16 KiB stack from linker script
    mov al, '9'
    call serial_putc64

    ; --- move user blobs up: boot sector read them to low memory ---
    ; (real mode cannot address above ~1.1 MB; long mode + paging can)
    mov rsi, 0x10000
    mov rdi, 0x200000
    mov ecx, USER_SECTORS * 512 / 8
    rep movsq
    mov rsi, 0x14000
    mov rdi, 0x220000
    mov ecx, FAULT_SECTORS * 512 / 8
    rep movsq

    ; --- copy AIkFS ramdisk up: partition was read to 0x18000 by boot ---
    mov rsi, 0x18000
    mov rdi, 0x400000
    mov ecx, FS_SECTORS * 512 / 8
    rep movsq

    mov al, 'K'
    call serial_putc64
    call kmain
.hang:
    cli
    hlt
    jmp .hang

; --- user_return: entered via a rewritten interrupt frame (ADR-013). ---
; The iretq that lands here did NOT pop ss/rsp (no ring change). Resume the
; REPL chain: restore the parked stack, the chain's callee-saved registers
; (the user program clobbers them), and jump to the captured resume address.
global user_return
extern proc_kernel_rsp      ; proc.c
extern proc_resume_addr     ; proc.c
extern proc_resume_regs     ; proc.c
user_return:
    mov rsp, [rel proc_kernel_rsp]
    mov rbp, [rel proc_resume_regs + 0]
    mov rbx, [rel proc_resume_regs + 8]
    mov r12, [rel proc_resume_regs + 16]
    mov r13, [rel proc_resume_regs + 24]
    mov r14, [rel proc_resume_regs + 32]
    mov r15, [rel proc_resume_regs + 40]
    jmp [rel proc_resume_addr]

; --- kputc: raw serial byte, SysV AMD64 ABI (char arg in DIL) — for C ---
global kputc
kputc:
    push rdx
    mov dx, 0x3FD               ; LSR
.1:
    in al, dx
    test al, 0x20               ; THR empty?
    jz .1
    mov al, dil                 ; the char (DIL preserved through the loop)
    mov dx, 0x3F8
    out dx, al
    pop rdx
    ret

; --- debug: emit char in AL to COM1 (64-bit, internal asm convention) ---
serial_putc64:
    push rax
    push rdx
    mov dx, 0x3FD               ; LSR
.1:
    in al, dx
    test al, 0x20               ; THR empty?
    jz .1
    mov al, [rsp + 8]           ; saved RAX ([rsp]=RDX, [rsp+8]=RAX)
    mov dx, 0x3F8
    out dx, al
    pop rdx
    pop rax
    ret

section .rodata
gdt64:
    dq 0x0000000000000000        ; 0x00 null descriptor
    dw 0xFFFF, 0x0000, 0x9A00, 0x00AF  ; 0x08 kcode: base 0, limit 4G, L=1, P=1
    dw 0xFFFF, 0x0000, 0x9200, 0x00CF  ; 0x10 kdata: base 0, limit 4G, D=1, P=1
    dw 0xFFFF, 0x0000, 0xFA00, 0x00AF  ; 0x18 ucode: DPL 3, L=1 (ADR-013)
    dw 0xFFFF, 0x0000, 0xF200, 0x00CF  ; 0x20 udata: DPL 3 (ADR-013)
    dq 0x0000000000000000        ; 0x28 TSS: 16-byte 64-bit descriptor,
    dq 0x0000000000000000        ;       base/limit patched by tss.c
gdt64_desc:
    dw gdt64_desc - gdt64 - 1
    dq gdt64
global gdt64
