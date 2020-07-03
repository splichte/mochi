#pragma once

#include "interrupts.h"

// keyboard
__attribute__ ((interrupt))
void kbd_handler(struct interrupt_frame *frame);

// timer
void timer_setup();
__attribute__ ((interrupt))
void timer_handler(struct interrupt_frame *frame);

// network
void transmit_initialization();
void test_transmit();


// virtual memory
__attribute__ ((interrupt))
void page_fault_handler(struct interrupt_frame *frame, uint32_t error_code);

