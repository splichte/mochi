; the job of the boot sector is to load the regions of 
; memory that hold bootloader 1 (switch to pm) 
; and bootloader 2. 
[org 0x7c00]
[bits 16]
NEXT_BLOCK_OFFSET equ 0x1000    ; memory offset where we'll load next block

    ; ?? isn't this moving into [0]? 
    mov [BOOT_DRIVE], dl

    ; do we need to do this? 
    mov bp, 0x9000
    mov sp, bp

    call load_next_block

    ; jump to next block
    jmp NEXT_BLOCK_OFFSET

load_next_block:
    mov bx, NEXT_BLOCK_OFFSET
    mov dh, 16                  ; 8kb

    mov dl, [BOOT_DRIVE]
    call disk_load

    ret


%include "boot/disk_load.asm"
%include "boot/print_string.asm"

BOOT_DRIVE      db 0

times 510-($-$$) db 0
dw 0xaa55

