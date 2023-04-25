#include "lib-header/terminal.h"

const size_t VGA_WIDTH = 80;
const size_t VGA_HEIGHT = 25;
size_t row;
size_t column;
uint8_t background_color;

void init_terminal(void) {
    row = 0;
    column = 0;
    background_color = 0x0F;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            framebuffer_write(row, column, ' ', (uint8_t)background_color, (uint8_t)background_color);
        }
    }
}

void terminal_setBackgroundColor(uint8_t color) {
    background_color = color;
}

void putch(char c) {
    if (c == '\n') {
        row++;
        column = 0;
    } else {
        framebuffer_write(row, column, c, (uint8_t)background_color, (uint8_t)background_color);
        if (++column == VGA_WIDTH) {
            column = 0;
            if (++row == VGA_HEIGHT)
                row = 0;
        }
    }
}

void puts(const char* data) {
    int i = 0;
    while(data[i] != '\0'){
        i++;
        putch(data[i]);
    }
}