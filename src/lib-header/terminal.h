#ifndef TERMINAL_H
#define TERMINAL_H

#include "framebuffer.h"

extern const size_t VGA_WIDTH;
extern const size_t VGA_HEIGHT;
extern size_t row;
extern size_t column;
extern uint8_t background_color;

void init_terminal(void);
void terminal_setBackgroundColor(uint8_t color);
void putch(char c);
void puts(char *str, uint32_t len, uint32_t color);

#endif