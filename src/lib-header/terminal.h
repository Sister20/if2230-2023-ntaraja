#ifndef TERMINAL_H
#define TERMINAL_H

#include "framebuffer.h"

extern const uint8_t VGA_WIDTH;
extern const uint8_t VGA_HEIGHT;
extern uint8_t row;
extern uint8_t column;
extern uint8_t background_color;

void init_terminal(void);
void terminal_setBackgroundColor(uint8_t color);
// void putch(char c);
void puts(char *str, uint32_t len, uint32_t color);

#endif