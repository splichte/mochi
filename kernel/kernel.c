#include "../drivers/screen.h"
#include "../drivers/disk.h"
#include "hardware.h"
#include "interrupts.h"
#include "process.h"
#include "memory.h"
#include "devices.h"
#include <stdint.h>
#include <stddef.h>

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

void test_memory() {
    uint32_t *p;
    p = (uint32_t *) kmalloc(20);
    if (p == NULL) {
        print("uh oh!\n");
    }
}

extern pid_t fork();

int kmain() {
//    clear_screen();
//    setup_memory();

//    setup_vmem();

    notify_screen_mmu_on();

    clear_screen();

    print("Welcome to Mochi ^_^ \n");
    print(">");

    HALT();

    test_memory();
    transmit_initialization();
    test_transmit();


    // test processes.
//    init_root_process();

//    pid_t pid = fork();

//    if (pid == 1) {
//        print("entered 1!\n");
//        while (1) {
//        }
//    }
//
//    if (pid == 2) {
//        print("entered 2!\n");
//        while (1) {
//        }
//    }

    while (1) {

    }
    return 0;
}

