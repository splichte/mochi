; load DH sectors to ES:BX from drive DL
disk_load:
    push dx

    mov ah, 0x02
    mov al, dh      ; read %dh sectors
    mov ch, 0x00    ; cylinder 0
    mov dh, 0x00    ; head 0
    int 0x13        ; BIOS interrupt

    jc disk_error   ; jump if error (carry flag set?)

    pop dx
    cmp dh, al      ; AL (sectors read) != DH (sectors expected)?
    jne disk_error
    ret

disk_error:
    mov bx, DISK_ERROR_MSG
    call print_string
    jmp $ 

DISK_ERROR_MSG:
    db "Disk read error!", 0
