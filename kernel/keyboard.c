#include "../drivers/screen.h"
#include "hardware.h"
#include "interrupts.h"
#include <stdint.h>
#include <stdbool.h>

/* standard ASCII symbols */
#define BACKSPACE 8
#define TAB 9
#define LINE_FEED 10
#define SHIFT_OUT 14
#define SHIFT_IN 15
#define ESC 27 

// ones I defined
// TODO: make an enum
#define CAPS_LOCK 256
#define CTRL 259
#define ALT 260
#define MISSING 0


/* I figured these out by using print_byte on the incoming keys for the 
 * keyboard. */
/* Most of the keys we need (the "down" presses) start at the beginning of the 
 * scancode range. */
/* I use extended number range to represent things like 
 * caps lock, etc. */
uint16_t scancodes[255] = {
    MISSING, CAPS_LOCK, '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '-', '=', BACKSPACE, TAB, 'q', 'w', 'e', 'r', 
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', LINE_FEED, CTRL,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', 
    '`', SHIFT_IN, MISSING, 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', 
    '.', '/', SHIFT_IN, MISSING, ALT, ' ', ESC,
    
    [170] = SHIFT_OUT, [182] = SHIFT_OUT,
};

__attribute__ ((interrupt))
void kbd_handler(struct interrupt_frame *frame) {
    asm volatile ("cli");

    uint8_t status = port_byte_in(0x64);

    // output buffer full? 
    if (status & 0x01) {
        uint8_t key = port_byte_in(0x60);
        uint8_t buf[2];
        buf[0] = scancodes[key];
        buf[1] = '\0';
        if (buf[0] != MISSING) {
            print(buf);
        }
    }

    port_byte_out(PIC1A, PIC_EOI);

    asm volatile ("sti");
}

