[bits 32]
[extern fork_inner]

global fork
fork:
    push ss
    push sp
    pushfd
    push cs
    push dword [esp+16]          ; a function call pushes eip onto the stack.
    pushad
    call fork_inner              ; return pid_t is stored in eax.
    popad
;    pop edi
;    pop esi
;    pop ebp
;    pop esp
;    pop ebx
;    pop edx
;    pop ecx
;    pop eax

;    add esp, 4                   ; don't restore eax.
    add esp, 4
    add esp, 4
    popfd
    add esp, 4
    add esp, 4
    ret

