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

void framebuffer_get_cursor(uint8_t *r, uint8_t *c) {
    uint16_t pos;
    out(0x3D4, 0x0F);
    pos = in(0x3D5);
    out(0x3D4, 0x0E);
    pos |= ((uint16_t)in(0x3D5)) << 8;
    *r = pos / 80;
    *c = pos % 80;
}

void framebuffer_write(uint8_t row, uint8_t col, char c, uint8_t fg, uint8_t bg) { 
    uint8_t attrib = (bg << 4) | (fg&0x0F);
    volatile uint16_t * where = (uint16_t *) MEMORY_FRAMEBUFFER + (row*80 + col);
    // change if c is \n
    if (c == '\n') {
        framebuffer_set_cursor(row+1, 0);
        return;
    }
    memset((void*)where, c, 1);
    memset((void*)where+1, attrib, 1);
}

void framebuffer_clear(void) {
    volatile uint8_t * where = MEMORY_FRAMEBUFFER;
    for(int i = 0 ; i < 25*80*2 ; i+=2){
        memset((void*)where+i, 0x00, 1);
        memset((void*)where+i+1, 0x07, 1);
    }
}
