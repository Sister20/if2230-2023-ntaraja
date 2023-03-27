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
    uint8_t attrib = (bg << 4) | (fg&0x0F);
    volatile uint16_t * where = (volatile uint16_t *)0xB8000 + (row*80 + col);
    memset((void*)where, c, 1);
    memset((void*)where+1, attrib, 1);
}

void framebuffer_clear(void) {
    // TODO : Implement
    volatile uint8_t * where = (volatile uint8_t *)0xB8000;
    for(int i = 0 ; i < 25*80*2 ; i+=2){
        memset((void*)where+i, 0x00, 1);
        memset((void*)where+i+1, 0x07, 1);
    }
}
