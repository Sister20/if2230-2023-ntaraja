#include "lib-header/portio.h"
#include "lib-header/stdtype.h"
#include "lib-header/stdmem.h"
#include "lib-header/gdt.h"
#include "lib-header/framebuffer.h"
#include "lib-header/kernel_loader.h"
#include "lib-header/interrupt/idt.h"
#include "lib-header/interrupt/interrupt.h"
#include "lib-header/keyboard.h"
#include "lib-header/filesystem/disk.h"
#include "lib-header/filesystem/fat32.h"

void debugPrint(uint8_t row, uint8_t col, int32_t x) {
    framebuffer_write(row, col, x + '0', 0x0F, 0x00);
}

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

    keyboard_state_activate();
    // __asm__("int $0x4");
    struct ClusterBuffer cbuf[5];
    for (uint32_t i = 0; i < 5; i++)
        for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
            cbuf[i].buf[j] = i + 'a';

    struct FAT32DriverRequest request = {
            .buf                   = cbuf,
            .name                  = "ikanaide",
            .ext                   = "uwu",
            .parent_cluster_number = ROOT_CLUSTER_NUMBER,
            .buffer_size           = 0,
    };
    read(request);

    set_tss_kernel_current_stack();
    kernel_execute_user_program((uint8_t *) 0);
    while (TRUE);
}

