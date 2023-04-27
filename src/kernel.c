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

    struct ClusterBuffer cbuf[1];
    for (int i = 0; i < CLUSTER_SIZE; i++) {
        cbuf[0].buf[i] = i + 'a';
    }

    struct FAT32DriverRequest ikanaide = {
            .buf = cbuf,
            .name = "ikanaide",
            .ext = "\0\0\0",
            .parent_cluster_number = ROOT_CLUSTER_NUMBER,
            .buffer_size = CLUSTER_SIZE
    };
    delete(ikanaide);
    write(ikanaide);

    memcpy(ikanaide.name, "daijoubu", 8);
    write(ikanaide);

    set_tss_kernel_current_stack();
    kernel_execute_user_program((uint8_t *) 0);

    while (TRUE);
}

