#include "lib-header/terminal.h"
#include "lib-header/stdmem.h"

const uint8_t VGA_WIDTH = 80;
const uint8_t VGA_HEIGHT = 25;
uint8_t row;
uint8_t column;
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

// void putch(char c) {
//     if (c == '\n') {
//         row++;
//         column = 0;
//     } else {
//         framebuffer_write(row, column, c, (uint8_t)background_color, (uint8_t)background_color);
//         if (++column == VGA_WIDTH) {
//             column = 0;
//             if (++row == VGA_HEIGHT)
//                 row = 0;
//         }
//     }
// }

void puts(char* data, uint32_t len, uint32_t color) {
    for (size_t i = 0; i < len; i++) {
        if(data[i] == '\0'){
            break;
        }
        framebuffer_get_cursor(&row, &column);
        if(data[i] == '\n'){
            if (row == 24){
                for (int i = 0; i < 80*24*2; i+=2) {
                    memcpy((void*)MEMORY_FRAMEBUFFER+i, (void*)MEMORY_FRAMEBUFFER+i+160, 1);
                    memcpy((void*)MEMORY_FRAMEBUFFER+i+1, (void*)MEMORY_FRAMEBUFFER+i+161, 1);
                }
                for (int i = 0; i < 80*2; i+=2) {
                    memset((void*)MEMORY_FRAMEBUFFER+80*24*2+i, 0x00, 1);
                    memset((void*)MEMORY_FRAMEBUFFER+80*24*2+i+1, 0x07, 1);
                }
            }  else {
                row++;
            }
            column = 0;
            framebuffer_set_cursor(row, column);
            continue;
        }
        framebuffer_write(row, column, data[i], (uint8_t)color, (uint8_t)background_color);
        if (++column == VGA_WIDTH) {
            column = 0;
            if (++row == VGA_HEIGHT)
                row = VGA_HEIGHT-1;
        }
        framebuffer_set_cursor(row, column);
    }
}