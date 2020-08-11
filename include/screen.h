#include <stdint.h>

void print(char *message);
void print_char(char c, int col, int row, char attr_byte);
void print_at(char *message, int col, int row);
void clear_screen();

// debugging prints, used in interrupts
void print_byte(uint8_t i); 
void print_word(uint32_t word);
void print_int(uint32_t i);

void notify_screen_mmu_on();
