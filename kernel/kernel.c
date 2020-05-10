#include "../drivers/screen.h"
#include "low_level.h"

char scancodes[255] = {
    0, '`', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '0', '-', '=', 0, 0, 'q', 'w', 'e', 'r', 
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\\', 
};

void main() {
    clear_screen(); 
    print("Hello, world!"); 

    // TODO: disable the mouse. 
    // TODO: use interrupt-driven, not polling! 
    while (1) {
        // controller status register
        unsigned char status = port_byte_in(0x64);

        // output buffer full? 
        if (status & 0x01) {
            unsigned char key = port_byte_in(0x60);
            char buf[2];
            buf[0] = scancodes[key];
            buf[1] = '\0';
            print(buf);
        }
    }
}
