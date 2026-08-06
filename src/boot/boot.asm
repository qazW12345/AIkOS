; AIkOS boot sector — real mode, loads kernel.bin to 0x100000, enters protected mode.
; Component: boot (real-mode boot sector)
; Provides: the boot protocol — kernel + user blobs + AIkFS partition loaded,
;           E820 map, A20,
;           protected-mode handoff to entry.asm
; Depends on: build.sh payload layout (KERNEL_SECTORS/USER_SECTORS/FAULT_*
;             defines), BIOS int 13h/15h (hardware behavior, ADR-008 spirit)
; Owns: disk layout contract (boot + kernel + user + userfault + AIkFS sectors);
;       E820 buffer 0x5000 (count @0x4FFC); low read buffers 0x10000/0x14000/0x18000;
;       kernel copy to 0x100000 (0xFFFF:0x0010); serial milestones S,B,M,E,U,F,R
; ADR-006. Assembled with NASM -f bin, loaded by BIOS at 0x7C00.
; Kernel sector count is injected by build.sh (-D KERNEL_SECTORS).
;
; NOTE (hard-won, 2026-08-05): QEMU 11's SeaBIOS hangs on int 13h AH=42h when the
; buffer is above 1 MB (its high-memory paging path spins in a serial debug loop,
; IDE never touched). Fix: read to a low buffer (0x10000, below 1 MB), then copy
; up to 0x100000 ourselves in real mode. See Guides/How-to-debug.md.
;
; Serial milestones (COM1, debug): S=start B=disk-loaded M=moved-to-1MB A=A20
;                                    L=GDT C=CR0.PE P=PM J=kernel-jump

[org 0x7C00]
[BITS 16]

%ifndef KERNEL_SECTORS
%define KERNEL_SECTORS 64
%endif
%ifndef USER_SECTORS
%define USER_SECTORS 16
%endif
%ifndef FAULT_SECTORS
%define FAULT_SECTORS 16
%endif
%ifndef USER_LBA
%define USER_LBA 65              ; 1 boot + 64 kernel sectors
%endif
%ifndef FAULT_LBA
%define FAULT_LBA 81             ; + 16 user sectors
%endif
%ifndef FS_SECTORS
%define FS_SECTORS 64
%endif
%ifndef FS_LBA
%define FS_LBA 97                ; + 16 fault sectors (AIkFS partition)
%endif

KERNEL_LOAD_SEG  equ 0x1000      ; low read buffer: 0x1000:0x0000 = 0x10000
KERNEL_LOAD_OFF  equ 0x0000
KERNEL_DEST_SEG  equ 0xFFFF      ; final destination: 0xFFFF:0x0010 = 0x100000
KERNEL_DEST_OFF  equ 0x0010
USER_LOAD_SEG    equ 0x1000      ; user blob low buffer: 0x10000 (reused)
FAULT_LOAD_SEG   equ 0x1400      ; fault blob low buffer: 0x14000
FS_LOAD_SEG      equ 0x1800      ; AIkFS ramdisk low buffer: 0x18000 (32 KiB)

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00               ; stack grows down, below the boot sector
    cld
    mov [boot_drive], dl         ; save boot drive (0x80 = hard disk)

    mov al, 'S'
    call serial_putc

    ; --- load kernel via int 13h AH=42h (LBA extensions) to LOW buffer ---
    mov ax, KERNEL_SECTORS
    mov ebx, 1
    mov cx, KERNEL_LOAD_SEG
    mov es, cx
    xor di, di
    call read_sectors

    mov al, 'B'
    call serial_putc

    ; --- copy kernel from low buffer (0x10000) to 0x100000 (real mode) ---
    mov ax, KERNEL_LOAD_SEG
    mov ds, ax
    mov si, KERNEL_LOAD_OFF
    mov ax, KERNEL_DEST_SEG
    mov es, ax
    mov di, KERNEL_DEST_OFF
    mov cx, KERNEL_SECTORS * 256 ; sectors * 512 / 2 = sectors * 256 words
    rep movsw

    xor ax, ax                   ; restore ds for the data below
    mov ds, ax

    mov al, 'M'
    call serial_putc

    ; --- collect E820 memory map into 0x5000 (count word at 0x4FFC) ---
    xor ax, ax
    mov es, ax
    xor ebx, ebx
    mov di, 0x5000
    xor bp, bp                  ; entry count
.e820_loop:
    mov eax, 0xE820
    mov edx, 0x534D4150         ; 'SMAP'
    mov ecx, 24
    int 0x15
    jc .e820_done
    cmp eax, 0x534D4150         ; BIOS must echo SMAP
    jne .e820_done
    add di, 24
    inc bp
    cmp bp, 64                  ; cap: 64 * 24 = 1536 bytes < table
    jae .e820_done
    test ebx, ebx               ; EBX == 0 -> last entry
    jz .e820_done
    jmp .e820_loop
.e820_done:
    mov word [0x4FFC], bp
    mov al, 'E'
    call serial_putc

    ; --- load user blobs to low buffers (real mode can't reach 2 MB);
    ;     entry.asm copies them up in long mode ---
    mov ax, USER_SECTORS
    mov ebx, USER_LBA
    mov cx, USER_LOAD_SEG
    mov es, cx
    xor di, di
    call read_sectors
    mov al, 'U'
    call serial_putc
    mov ax, FAULT_SECTORS
    mov ebx, FAULT_LBA
    mov cx, FAULT_LOAD_SEG
    mov es, cx
    xor di, di
    call read_sectors
    mov al, 'F'
    call serial_putc

    ; --- load AIkFS partition (ramdisk) to a low buffer; entry.asm copies
    ;     it up to 0x400000 in long mode (Phase 3, ADR-015) ---
    mov ax, FS_SECTORS
    mov ebx, FS_LBA
    mov cx, FS_LOAD_SEG
    mov es, cx
    xor di, di
    call read_sectors
    mov al, 'R'
    call serial_putc

    ; --- enable A20 (fast A20, port 0x92) ---
    in al, 0x92
    or al, 0x02
    out 0x92, al
    mov al, 'A'
    call serial_putc

    ; --- load GDT, enter protected mode ---
    lgdt [gdt_desc]
    mov al, 'L'
    call serial_putc
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    mov al, 'C'
    call serial_putc
    jmp 0x08:pm_entry

disk_error:
    mov si, msg_err
.print:
    lodsb
    or al, al
    jz .hang
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp .print
.hang:
    hlt
    jmp .hang

; --- int 13h AH=42h helper: ax=sectors, ebx=LBA, es:di=buffer ---
read_sectors:
    mov [dap + 2], ax
    mov [dap + 8], ebx
    mov [dap + 4], di
    mov [dap + 6], es
    mov si, dap
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jc disk_error
    ret

; --- debug: emit char in AL to COM1 (works in real mode) ---
serial_putc:
    push ax
    push dx
    mov dx, 0x3FD               ; LSR
.1:
    in al, dx
    test al, 0x20               ; THR empty?
    jz .1
    mov al, [esp + 2]           ; saved AX (16-bit pushes: [esp]=DX, [esp+2]=AX)
    mov dx, 0x3F8
    out dx, al
    pop dx
    pop ax
    ret

[BITS 32]
; --- debug: emit char in AL to COM1 (32-bit protected mode).
; NOTE: must NOT be the 16-bit serial_putc — its 3-byte `mov dx, imm16`
; encoding (no 66 prefix) is misdecoded in 32-bit mode and eats the
; following byte. War story #1, see Guides/How-to-debug.md.
serial_putc32:
    push eax
    push edx
    mov dx, 0x3FD               ; 66 BA FD 03 — 32-bit-correct encoding
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

pm_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov al, 'P'
    call serial_putc32
    jmp 0x08:0x100000           ; jump to kernel entry (entry.asm, 32-bit)

; --- data ---
msg_err db "AIkOS: disk read failed", 0
boot_drive db 0

align 4
dap:
    db 0x10                     ; DAP size
    db 0                        ; reserved
    dw KERNEL_SECTORS           ; sector count
    dw KERNEL_LOAD_OFF          ; buffer offset
    dw KERNEL_LOAD_SEG          ; buffer segment
    dq 1                        ; LBA: kernel starts at sector 1

; --- GDT (flat 32-bit) ---
align 4
gdt:
    dq 0x0000000000000000       ; null descriptor
    dw 0xFFFF, 0x0000, 0x9A00, 0x00CF  ; code: base 0, limit 4G, D=1, G=1, P=1, type 0xA
    dw 0xFFFF, 0x0000, 0x9200, 0x00CF  ; data: base 0, limit 4G, D=1, G=1, P=1, type 0x2
gdt_desc:
    dw gdt_desc - gdt - 1
    dd gdt

times 510-($-$$) db 0
dw 0xAA55
