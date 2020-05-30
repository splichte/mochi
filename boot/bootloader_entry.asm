[bits 32]
[extern main]

mov ebx, MSG_BOOT
call print_string_pm

call main

jmp $


%include "boot/print_string_pm.asm"

MSG_BOOT    db "In bootloader entry. ",0
