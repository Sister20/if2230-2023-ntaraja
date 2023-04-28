#include "lib-header/portio.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/gdt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/kernel_loader.h"
#include "lib-header/interrupt/idt.h"
#include "lib-header/interrupt/interrupt.h"
#include "lib-header/keyboard.h"
#include "lib-header/filesystem/fat32.h"
#include "lib-header/paging.h"

_Noreturn void kernel_setup(void) {
    enter_protected_mode(&_gdt_gdtr);
    pic_remap();
    activate_keyboard_interrupt();
    initialize_idt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    initialize_filesystem_fat32();
    gdt_install_tss();
    set_tss_register();

    allocate_single_user_page_frame((uint8_t *) 0);
    
    struct FAT32DriverRequest request = {
            .buf                   = (uint8_t *) 0,
            .name                  = "shell",
            .ext                   = "\0\0\0",
            .parent_cluster_number = ROOT_CLUSTER_NUMBER,
            .buffer_size           = 0x100000,
    };
    read(request);

    struct ClusterBuffer cbuf[2];
    for(int j = 0; j < 2; j++){
        for (int i = 0; i < CLUSTER_SIZE; i++) {
            cbuf[j].buf[i] = 'a' + j;
        }
    }
    
    struct FAT32DirectoryTable table;

    struct FAT32DriverRequest ikanaide = {
            .buf = &table,
            .name = "ikanaide",
            .ext = "\0\0\0",
            .parent_cluster_number = ROOT_CLUSTER_NUMBER,
            .buffer_size = 0
    };
    write(ikanaide);
    read_directory(ikanaide);
    struct FAT32DriverRequest new_file = {
            .buf = cbuf,
            .name = "filefile",
            .ext = "exe",
            .parent_cluster_number = (table.table[0].cluster_high << 16 | table.table[0].cluster_low),
            .buffer_size = CLUSTER_SIZE
    };
    write(new_file);

    set_tss_kernel_current_stack();
    kernel_execute_user_program((uint8_t *) 0);

    while (TRUE);
}

