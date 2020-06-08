#pragma once

#include "interrupts.h"

// keyboard
__attribute__ ((interrupt))
void kbd_handler(struct interrupt_frame *frame);

// timer
__attribute__ ((interrupt))
void timer_handler(struct interrupt_frame *frame);

// network
void transmit_initialization();
void test_transmit();

