#include "lib-header/keyboard.h"
#include "lib-header/portio.h"
#include "lib-header/framebuffer.h"
#include "lib-header/stdmem.h"

const char keyboard_scancode_1_to_ascii_map[256] = {
      0, 0x1B, '1', '2', '3', '4', '5', '6',  '7', '8', '9',  '0',  '-', '=', '\b', '\t',
    'q',  'w', 'e', 'r', 't', 'y', 'u', 'i',  'o', 'p', '[',  ']', '\n',   0,  'a',  's',
    'd',  'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0, '\\',  'z', 'x',  'c',  'v',
    'b',  'n', 'm', ',', '.', '/',   0, '*',    0, ' ',   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0, '-',    0,    0,   0,  '+',    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,

      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
      0,    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,    0,    0,   0,    0,    0,
};

static struct KeyboardDriverState keyboard_state;

void keyboard_state_deactivate(void){
    keyboard_state.keyboard_input_on = FALSE;
}

void keyboard_state_activate(void){
    keyboard_state.keyboard_input_on = TRUE;
}

void get_keyboard_buffer(char *buf){
    memcpy(buf, keyboard_state.keyboard_buffer, keyboard_state.buffer_index);
}

bool is_keyboard_blocking(void){
    return keyboard_state.keyboard_input_on;
}

void keyboard_isr(void) {
    if (!keyboard_state.keyboard_input_on)
        keyboard_state.buffer_index = 0;
    else {
        uint8_t  scancode    = in(KEYBOARD_DATA_PORT);
        char     mapped_char = keyboard_scancode_1_to_ascii_map[scancode];
        // TODO : Implement scancode processing
        uint8_t r, c;
        framebuffer_get_cursor(&r, &c);
        if(mapped_char == '\b'){
            keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = ' ';
            keyboard_state.buffer_index--;
            framebuffer_write(r,c-1,' ',0x0F,0x00);
            framebuffer_set_cursor(r,c-1);
            keyboard_state.buffer_index--;
        }
        else if(mapped_char == '\n'){
            framebuffer_set_cursor(r+1,0);
            keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = '\0';
            keyboard_state.buffer_index = 0;
            keyboard_state.keyboard_input_on = FALSE;
        }
        else if(mapped_char != 0){
            keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = mapped_char;
            keyboard_state.buffer_index++;
            framebuffer_write(r,c,mapped_char,0x0F,0x00);
            framebuffer_set_cursor(r,c+1);
        }
        // testing input read or not
        // if(mapped_char != 0){
        //     keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = mapped_char;
        //     keyboard_state.buffer_index++;
        //     framebuffer_write(r,c,mapped_char,0x0F,0x00);
        //     framebuffer_set_cursor(r,c+1);
        // }
    }
    pic_ack(IRQ_KEYBOARD);
}
