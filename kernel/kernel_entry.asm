[bits 32]
[extern kmain]

; fix the stack pointers. 
; we've mapped 0xf000_0000 through 0xf040_0000
; the "call" is working. but now we're somewhere weird.
mov ebp, 0xf03ffff0
mov esp, ebp

call kmain

