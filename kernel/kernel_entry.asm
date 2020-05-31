[bits 32]
[extern kmain]

; so linker thinks it knows where the function is. 
; but it jumps to a bad location.
call kmain

