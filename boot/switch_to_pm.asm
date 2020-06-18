; should be an "org" here, I think. 

[org 0x1000] ; matching NEXT_BLOCK_OFFSET in boot_sect.asm
[bits 16]
BOOTLOADER_OFFSET   equ 0x1400   ; this code uses 2 blocks of 512-byte

; Switch to protected mode
switch_to_pm:
    call setup_memory

    cli
    lgdt [gdt_descriptor]

    ; turn on protected mode
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    jmp CODE_SEG:init_pm

; before we switch to protected mode,
; get the RAM info for the kernel to use
; when setting up memory management. 
; Query System Address Map
; http://www.uruk.org/orig-grub/mem64mb.html
setup_memory:
    pusha
    ; initial destination: 0xf000
    mov ax, 0xf00
    mov es, ax          ; segment
    mov di, 0x0            ; (initial) offset
    mov ebx, 0x0         ; (initial) continuation value

setup_memory_loop:
    mov eax, 0xe820      ; function code
    mov ecx, 20          ; buffer size (minimum 20)
    mov edx, 0x534D4150  ; 'SMAP', the signature

    int 0x15            

    jc memory_error

    mov eax, 0x0
    cmp eax, ebx
    je memory_done

    add di, cx              ; add returned buffer size to offset
    jmp setup_memory_loop

memory_done:
    ; 0 out length field so kernel can know we're done.
    add di, 8               ; length_low
    mov ax, 0
    mov [es:di], ax
    add di, 8               ; length_high
    mov [es:di], ax
    mov bx, MEMORY_DONE_MSG
    call print_string

    popa
    ret

memory_error:
    mov bx, MEMORY_ERROR_MSG
    call print_string 

MEMORY_ERROR_MSG:
    db "Query System Address Map error", 0

MEMORY_DONE_MSG:
    db "Query System Address Map success", 0

%include "boot/print_string.asm"
%include "boot/print_string_pm.asm"
%include "boot/gdt.asm"

[bits 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x1100000    ; update stack position to be at 16 + 1 Mb
                          ; (where we set KERNEL_END in kernel/memory.c)
                          ; if it grows too big, we'll corrupt 
                          ; kernel code. be careful...!
    mov esp, ebp

    jmp BOOTLOADER_OFFSET

MSG_PM:
    db "Switched into protected mode", 0

times 1024-($-$$) db 0  ; match assumption at top (that we use 2 512-byte chunks of memory here)


