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

void test_sb_writes() {
    superblock_t out = { 0 };
    out.s_inodes_count = 12;

    superblock_t in = { 0 };

    uint32_t lba = 0x00004002; 

    // expected: "Not there yet"
    if (out.s_inodes_count != in.s_inodes_count) {
        print("SB not there yet!\n");
    } else {
        print("it's there!\n");
    }

    disk_write(lba, (uint8_t *) &out, sizeof(out));

    uint8_t *buf = (uint8_t *) &in;
    for (int i = 0; i < 2; i++) {
        disk_read(lba + i, buf + DISK_SECTOR_SIZE * i);
    }

    // expected: "it's there!"
    if (out.s_inodes_count != in.s_inodes_count) {
        print("SB not there yet!\n");
    } else {
        print("it's there!\n");
    }

}


void test_image_persists() {
    uint8_t out_buf[512];
    uint8_t in_buf[512];
    uint32_t lba = 0x00004002; 

    out_buf[0] = 0xba;
    out_buf[1] = 0xed;

    // expected: "Not there yet"
    if (out_buf[0] != in_buf[0] || out_buf[1] != in_buf[1]) {
        print("Not there yet!\n");
    } else {
        print("it's there!\n");
    }

    disk_write(lba, out_buf, 512);

    disk_read(lba, in_buf);

    // expected: "it's there!"
    if (out_buf[0] != in_buf[0] || out_buf[1] != in_buf[1]) {
        print("Not there yet!\n");
//        print_byte(out_buf[1]);
    } else {
        print("it's there!\n");
    }

//    disk_write(lba, out_buf, 512);
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
    notify_screen_mmu_on();
    clear_screen();


    // once we're in kernel, we need to immediately set the 
    // page tables to make sure we don't run out of memory.
    setup_memory();

    setup_interrupt_controller();
    setup_interrupt_descriptor_table();

//    test_sb_writes();
//    finish_vmem_setup();

    print("Welcome to Mochi ^_^ \n");
    print(">");

    mkfs(8, 24);

    test_fs();

    // turned these off for now...since we have virtual memory 
    // turned on.
//    transmit_initialization();
//    test_transmit();

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

