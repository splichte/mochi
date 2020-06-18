[bits 32]
[extern fork_inner]

global fork
fork:
    mov eax, [esp]
    push ebp
    mov ebp, esp

    push ss
    push sp
    pushfd
    push cs
    push dword eax          ; a function call pushes eip onto the stack.
    pushad
    call fork_inner              ; return pid_t is stored in eax.
    pop edi
    pop esi
    pop ebp
    add esp, 4                   ; skip esp (would corrupt)
    pop ebx
    pop edx
    pop ecx
    add esp, 4
;    pop eax                     ; skip eax (want to return it)

;    add esp, 4                   ; don't restore eax.
    add esp, 4
    add esp, 4
    popfd
    add esp, 4
    add esp, 4

    mov esp, ebp
    pop ebp

    ret

