[bits 32]
[extern timer_handler_inner]

global timer_handler
; when this function is called, it 
; will push an interrupt frame on the stack, 
; then call this. we additionally push 
; all the general-purpose registers on the stack.
; push all registers on the stack.
timer_handler:
    pushad
    call timer_handler_inner 
    popad
    iret 
