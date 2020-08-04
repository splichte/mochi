#pragma once

#include <stdint.h>

typedef uint16_t pid_t;
typedef uint32_t timeslice_t;

// order from:
// https://c9x.me/x86/html/file_module_x86_id_270.html
typedef struct {
    // reversed order from the order it's pushed onto the stack
    // (based on how the compiler handles arguments to the 
    // timer_handler_inner function)
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags, sp, ss;
} registers;

// (interrupt frame + general registers)

typedef struct _process {
    registers regs;
    pid_t pid;
    timeslice_t ts; // how much ts increments do we give this process?
    uint16_t subtick_start;



    // doubly-linked circular list
    struct _process *prev;
    struct _process *next;
} process;

void init_root_process();
process *get_current_process();
void set_current_process(process *p, registers r);

