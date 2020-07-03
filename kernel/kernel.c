#include "../drivers/screen.h"
#include "../drivers/disk.h"
#include "hardware.h"
#include "interrupts.h"
#include "process.h"
#include "memory.h"
#include "devices.h"
#include "fs.h"
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

    disk_write(lba, out_buf, 512);
}

//void test_memory() {
//    uint32_t *p;
//    p = (uint32_t *) kmalloc(20);
//    if (p == NULL) {
//        print("uh oh!\n");
//    }
//}

extern pid_t fork();

int kmain() {
//    clear_screen();

    notify_screen_mmu_on();
    // once we're in kernel, we need to immediately set the 
    // page tables to make sure we don't run out of memory.
    setup_memory();

    setup_interrupt_controller();
    setup_interrupt_descriptor_table();

    clear_screen();

//    finish_vmem_setup();

//    HALT();

    print("Welcome to Mochi ^_^ \n");
    print(">");

    mkfs(8, 24);

    print("done!\n");

    // turned these off for now...since we have virtual memory 
    // turned on.
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

