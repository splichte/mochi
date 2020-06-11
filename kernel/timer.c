#include "../drivers/screen.h"
#include "hardware.h"
#include "interrupts.h"
#include "process.h"
#include <stdint.h>
#include "devices.h"

// programmable interrupt timer stuff -- from ToaruOS
#define PIT_A       0x40
#define PIT_B       0x41
#define PIT_C       0x42
#define PIT_CTRL    0x43

#define PIT_MASK    0xff
#define PIT_SCALE   1193180
#define PIT_SET     0x34

#define SUBTICKS_PER_TICK 1000

// set phase (in hertz) for timer.
void timer_phase(int hz) {
    int divisor = PIT_SCALE / hz;
    port_byte_out(PIT_CTRL, PIT_SET);
    port_byte_out(PIT_A, divisor & PIT_MASK);
    port_byte_out(PIT_A, (divisor >> 8) & PIT_MASK);
}

// TODO: what happens when timer_ticks hits its maximum?
static uint16_t timer_subticks = 0;
static unsigned long timer_ticks = 0;

// TODO: what if we want a process to run a full tick or more?
int ts_elapsed(process *p) {
    return ((p->subtick_start + p->ts) % SUBTICKS_PER_TICK) == timer_subticks;
}

// in the future, this could be a better scheduler...
process *next_process(process *p) {
    return p->next;
}

void timer_handler_inner(registers r) {
    pause_interrupts();

    // ignoring drift for right now...
    if (++timer_subticks == SUBTICKS_PER_TICK) {
        timer_ticks++;
        timer_subticks = 0;
    }

    ack_interrupt_pic1();

    process *p = get_current_process();

    // when to resume interrupts is kind of weird. 
    // I don't really want to resume until we're safely in 
    // the next process. but I have no idea how to control that!
    resume_interrupts();

    // if p's timeslice has elapsed...
    if (ts_elapsed(p)) {
        process *p_next = next_process(p);
        set_current_process(p_next, r);
    }
}

void timer_setup() {
    timer_phase(SUBTICKS_PER_TICK); // 1000 Hz
}

