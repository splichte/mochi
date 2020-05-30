[bits 32]
[extern kmain]

mov ebx, MSG_HI
call print_string_pm

; call kmain

jmp $

%include "boot/print_string_pm.asm"

MSG_HI  db "Hello. ",0
