#include "screen.h"
#include "../kernel/low_level.h"

/* forward declarations */
int get_screen_offset(int col, int row);
int get_cursor();
void set_cursor(int offset);

void print(char *message) {
    print_at(message, -1, -1);
}

void print_at(char *message, int col, int row) {
    if (col >= 0 && row >= 0) {
        set_cursor(get_screen_offset(col, row));
    } 

    int i = 0;
    while (message[i] != 0) {
        print_char(message[i++], col, row, WHITE_ON_BLACK);
   }
}

/* print a char on the screen at col, row, or at cursor position */
/* this is the only print function that actually prints. 
 * the other functions just wrap it. */
/* i.e. lowest-level print. */
void print_char(char c, int col, int row, char attr_byte) {
    unsigned char *vidmem = (unsigned char *) VIDEO_ADDRESS;

    if (!attr_byte) {
        attr_byte = WHITE_ON_BLACK;
    }

    int offset;
    if (col >= 0 && row >= 0) {
        offset = get_screen_offset(col, row);
    } else {
        offset = get_cursor();
    }

    if (c == '\n') {
        int rows = offset / (2*MAX_COLS);
        offset = get_screen_offset(79, rows);
    } else {
        vidmem[offset] = c;
        vidmem[offset+1] = attr_byte;
    }

    offset += 2;
    // offset = handle_scrolling(offset);
    set_cursor(offset);
}

int get_screen_offset(int col, int row) {
    return ((row * MAX_COLS) + col) * 2;
}

int get_cursor() {
    // reg 14: high byte of cursor's offset
    // reg 15: low byte of cursor's offset
    port_byte_out(REG_SCREEN_CTRL, 14);
    int offset = port_byte_in(REG_SCREEN_DATA) << 8;
    port_byte_out(REG_SCREEN_CTRL, 15);
    offset += port_byte_in(REG_SCREEN_DATA);

    return offset * 2;
}

void set_cursor(int offset) {
    // convert from cell offset to char offset
    offset /= 2;

    port_byte_out(REG_SCREEN_CTRL, 14);
    // just take the top byte
    port_byte_out(REG_SCREEN_DATA, (unsigned char)(offset >> 8));

    port_byte_out(REG_SCREEN_CTRL, 15);
    // just take the bottom byte
    port_byte_out(REG_SCREEN_DATA, (unsigned char) offset);
}

void clear_screen() {
    int row = 0;
    int col = 0;

    for (row = 0; row < MAX_ROWS; row++) {
        for (col = 0; col < MAX_COLS; col++) {
            print_char(' ', col, row, WHITE_ON_BLACK);
        }
    }

    set_cursor(get_screen_offset(0, 0));
}
