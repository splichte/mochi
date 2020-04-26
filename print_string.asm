print_string:
    pusha
    mov ah, 0x0e

compare:
    mov al, [bx]
    cmp al, 0 ; is the string terminated yet?
    je done
    int 0x10
    add bx, 1 ; add 16 bits to move to next position
    jmp compare ; compare again

done:
    popa
    ret

