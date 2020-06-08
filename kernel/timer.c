#include "../drivers/screen.h"
#include "hardware.h"
#include "interrupts.h"
#include <stdint.h>
#include <stdbool.h>
#include "devices.h"


// programmable interrupt timer stuff -- from ToaruOS
#define PIT_A       0x40
#define PIT_B       0x41
#define PIT_C       0x42
#define PIT_CTRL    0x43

#define PIT_MASK    0xff
#define PIT_SCALE   1193180
#define PIT_SET     0x34

__attribute__ ((interrupt))
void timer_handler(struct interrupt_frame *frame) {
    asm volatile ("cli");

    // determine if we should switch tasks...
    // we need to store the registers somewhere, then

    port_byte_out(PIC1A, PIC_EOI);
    asm volatile ("sti");
}

