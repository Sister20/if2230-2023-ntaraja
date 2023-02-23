#include "lib-header/framebuffer.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/portio.h"

// void framebuffer_set_cursor(uint8_t r, uint8_t c) {
//     // TODO : Implement
// }

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
    uint16_t attrib = (0x07 << 4) | (0x07 & 0xF);

    int i;
    for(i = 0; i < 25*80; i++){
        *(where + i) = attrib;
    }

}
