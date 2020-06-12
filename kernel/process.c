#include "process.h"
#include "memory.h"
#include "../drivers/screen.h"
#include "interrupts.h"

static process *current_process;

process *get_current_process() {
    return current_process;
}

// the goal is to set up registers
// the same way an interrupt would.
pid_t fork_inner(registers r) {
    // r should "make sense"!
    process *p = get_current_process();
    process *test = (process *)kmalloc(sizeof(process));

    // store registers at this point.
    test->regs = r;
    p->regs = r;
    test->ts = 500;
    test->pid = 2;
    test->prev = p;
    test->next = p;

    p->prev = test;
    p->next = test;

    return test->pid;
}


// when the old process returns from the interrupt handler... 
// won't the stack (with the registers) automatically be popped? 
// maybe the current registers aren't dumped, I don't know. 
// it seems like we have to save the kernel state, too. 
void set_current_process(process *p, registers r) { 
    // turn interrupts off while we're switching
    asm volatile ("cli");

    process *p_old = current_process;

    // save register state of old process.
    p_old->regs = r;

    current_process = p;

    print("switching process to: \n");
    print_word(p->pid);
    print_word(p->regs.eip);
    // we should dump register contents...see what's up.

    // restore registers from process struct, 
    // and start executing.
    // is "start executing" just..."set eip"?
    // yup, which is just "jmp (addr)"
    asm volatile ("mov %0, %%eax" :: "r" (p->regs.eax));
    asm volatile ("mov %0, %%ecx" :: "r" (p->regs.ecx));
    asm volatile ("mov %0, %%edx" :: "r" (p->regs.edx));
    asm volatile ("mov %0, %%ebx" :: "r" (p->regs.ebx));
    asm volatile ("mov %0, %%esp" :: "r" (p->regs.esp));
    asm volatile ("mov %0, %%ebp" :: "r" (p->regs.ebp));
    asm volatile ("mov %0, %%esi" :: "r" (p->regs.esi));
    asm volatile ("mov %0, %%edi" :: "r" (p->regs.edi));

    // resume interrupts
    asm volatile ("sti");

    asm volatile ("jmp %0" :: "r" (p->regs.eip));
}

void init_root_process() {
    // create root process
    current_process = (process *) kmalloc(sizeof(process));
    process *p = current_process;
    p->prev = p;
    p->next = p;
    p->ts = 500; // slow switch.
    p->pid = 1;

}


