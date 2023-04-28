#include "lib-header/keyboard.h"
#include "lib-header/portio.h"
#include "lib-header/framebuffer.h"
#include "lib-header/stdmem.h"

// IF THERE IS ANYTHING THAT IS IMPLEMENTED AND NEED TO BE CHANGED IN THE FUTURE, YOU CAN PUT IT IN THE FEEDBACK AND IN MILESTONE 3 IT WILL BE CHANGED/IMPLEMENTED

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

struct KeyboardDriverState keyboard_state;

/**
 * @brief      Clear the keyboard buffer
*/
void keyboard_clear_buffer(void){
    keyboard_state.buffer_index = 0;
    keyboard_state.buffer_index_max = 0;
    memset(keyboard_state.keyboard_buffer, 0, KEYBOARD_BUFFER_SIZE);
}

/**
 * @brief      Write a character to the keyboard buffer at the current cursor position
*/
void keyboard_insert(char ch){
    // no more space in buffer, no replacement (idk if needed, if needed just give feedback and in milestone 3 will be implemented)
    if(keyboard_state.buffer_index_max == KEYBOARD_BUFFER_SIZE - 1){
        return;
    }
    uint8_t r,c,col,row;
    framebuffer_get_cursor(&r, &c);
    for(uint8_t ii = keyboard_state.buffer_index_max; ii > keyboard_state.buffer_index; ii--){
        keyboard_state.keyboard_buffer[ii] = keyboard_state.keyboard_buffer[ii-1];
        col = c + ii - keyboard_state.buffer_index;
        row = r + (col / 80);
        col = col % 80;
        framebuffer_write(row,col,keyboard_state.keyboard_buffer[ii],0x0F,0x00);
    }
    keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = ch;
    col = c;
    row = r + (col / 80);
    framebuffer_write(row,col,ch,0x0F,0x00);
    keyboard_state.buffer_index++;
    keyboard_state.buffer_index_max++;
    framebuffer_set_cursor(r,++c);
}

/**
 * @brief     Delete a character from the keyboard buffer at the current cursor position
*/
void keyboard_delete(void){
    // no need to delete if buffer is empty
    if(keyboard_state.buffer_index == 0){
        return;
    }
    uint8_t r,c,row,col;
    framebuffer_get_cursor(&r, &c);
    for(uint8_t ii = keyboard_state.buffer_index-1; ii <= keyboard_state.buffer_index_max; ii++){
        keyboard_state.keyboard_buffer[ii] = keyboard_state.keyboard_buffer[ii+1];
        col = c + ii - keyboard_state.buffer_index;
        row = r + (col / 80);
        col = col % 80;
        framebuffer_write(row,col,keyboard_state.keyboard_buffer[ii],0x0F,0x00);
    }
    keyboard_state.keyboard_buffer[keyboard_state.buffer_index_max] = ' ';
    col = c + keyboard_state.buffer_index_max - keyboard_state.buffer_index;
    row = r + (col / 80);
    col = col % 80;
    framebuffer_write(row,col,' ',0x0F,0x00);
    framebuffer_set_cursor(r,--c);
    keyboard_state.buffer_index--;
    keyboard_state.buffer_index_max--;
}

void keyboard_state_deactivate(void){
    keyboard_state.keyboard_input_on = FALSE;
}

void keyboard_state_activate(void){
    keyboard_state.keyboard_input_on = TRUE;
}

void get_keyboard_buffer(char *buf){
    memcpy(buf, keyboard_state.keyboard_buffer, KEYBOARD_BUFFER_SIZE);
    keyboard_clear_buffer();
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
        uint8_t r, c;
        framebuffer_get_cursor(&r, &c);
        if(mapped_char == '\b'){
            keyboard_delete();
        }
        else if(mapped_char == '\n'){
            framebuffer_write(r,c,'\n',0x0F,0x00);
            // framebuffer_set_cursor(r+1,0);
            keyboard_state.keyboard_buffer[keyboard_state.buffer_index] = '\0';
            keyboard_state.buffer_index++;keyboard_state.buffer_index_max++;
            keyboard_state.keyboard_input_on = FALSE;
            // keyboard_clear_buffer();
            // keyboard_state.buffer_index_max = 0;
            // keyboard_state.buffer_index = 0;
        }
        // tab is 4 spaces, default of most IDEs
        else if(mapped_char == '\t'){
            for(int ii = 0 ; ii <  4 && keyboard_state.buffer_index < KEYBOARD_BUFFER_SIZE - 1; ii++){
                keyboard_insert(' ');
            }
        }
        // handling left right arrow
        else if(mapped_char == 0){
            if(scancode == 0x4B){
                if(keyboard_state.buffer_index > 0){
                    keyboard_state.buffer_index--;
                    if(c == 0){
                        framebuffer_set_cursor(r-1,79);
                    }
                    else{
                        framebuffer_set_cursor(r,c-1);
                    }
                }
            }
            else if(scancode == 0x4D){
                if(keyboard_state.buffer_index < keyboard_state.buffer_index_max){
                    keyboard_state.buffer_index++;
                    if(c == 79){
                        framebuffer_set_cursor(r+1,0);
                    }
                    else{
                        framebuffer_set_cursor(r,c+1);
                    }
                }
            }
        }
        else if(mapped_char != 0){
            // must press enter if buffer is full
            if(keyboard_state.buffer_index_max < KEYBOARD_BUFFER_SIZE-1){
                keyboard_insert(mapped_char);
            }
        }
    }
    pic_ack(IRQ_KEYBOARD);
}
