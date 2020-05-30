#include "../drivers/screen.h"
#include "../drivers/disk.h"
#include "hardware.h"
#include "interrupts.h"
#include <stdint.h>

void test_image_persists() {
    uint8_t out_buf[512];
    uint8_t in_buf[512];
    uint64_t lba = 24; 

    out_buf[0] = 0xba;
    out_buf[1] = 0xed;

    disk_read(lba, in_buf);

    // this tests that the image persists!
    if (out_buf[0] != in_buf[0] || out_buf[1] != in_buf[1]) {
        // how do we print as bytes..
        print("Not there yet!\n");
        print_byte(out_buf[1]);
    } else {
        print("it's there!\n");
    }

    disk_write(lba, out_buf);
}

int kmain() {
    clear_screen(); 

    setup_interrupt_controller();

    setup_interrupt_descriptor_table();

    print("Welcome to Mochi ^_^ \n");
    print(">");

    transmit_initialization();
    test_transmit();

    while (1) {
    }
    return 0;
}

