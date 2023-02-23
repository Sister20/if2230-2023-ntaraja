#include "lib-header/framebuffer.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/portio.h"

void framebuffer_set_cursor(uint8_t r, uint8_t c) {
    // TODO : Implement
    uint16_t pos = r * 80 + c;
    out(0x3D4, 0x0F);
	out(0x3D5, (uint8_t) (pos & 0xFF));
	out(0x3D4, 0x0E);
	out(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) {
    // TODO : Implement
    uint16_t attrib = (bg << 4) | (fg & 0xF);
    volatile uint16_t * where;
    where = (volatile uint16_t *)0xB8000 + (row*80 + col);
    *where = c | (attrib << 8);
}

void framebuffer_clear(void) {
    // TODO : Implement
    volatile uint16_t * where;
    where = (volatile uint16_t *)0xB8000;
    uint16_t attrib = 0x00;
    memset((void*)where, attrib, 25*80*2);
}
