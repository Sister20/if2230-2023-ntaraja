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

void kernel_setup(void) {
    enter_protected_mode(&_gdt_gdtr);
    pic_remap();
    activate_keyboard_interrupt();
    initialize_idt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    initialize_filesystem_fat32();
//    keyboard_state_activate();
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
    debugPrint(8, 3, write(request));
//    write(request);
    memcpy(request.name, "kano1\0\0\0", 8);
//    write(request);
    debugPrint(9, 3, write(request));

    memcpy(request.name, "ikanaide", 8);
    memcpy(request.ext, "\0\0\0", 3);
//    delete(request); // Delete first folder, thus creating hole in FS
    debugPrint(10, 3, delete(request));

    memcpy(request.name, "daijoubu", 8);
    request.buffer_size = 5*CLUSTER_SIZE;
    write(request); // Create fragmented file "daijoubu"
//    debugPrint(11, 3, write(request));
    struct ClusterBuffer readcbuf;
    read_clusters(&readcbuf, ROOT_CLUSTER_NUMBER+3 /* changed from 1 because delete is broken */, 1);
//     If read properly, readcbuf should filled with 'a'

    request.buffer_size = CLUSTER_SIZE;
//    debugPrint(12, 3, read(request));
    read(request);   // Failed read due not enough buffer size
    request.buffer_size = 5*CLUSTER_SIZE;
    read(request);   // Success read on file "daijoubu"
//    debugPrint(13, 3, read(request));   // Success read on file "daijoubu"

    framebuffer_write(6, 3, 'F', 0x0F, 0x00);
    framebuffer_write(6, 4, 'A', 0x0F, 0x00);
    framebuffer_write(6, 5, 'T', 0x0F, 0x00);
    framebuffer_write(6, 6, '3', 0x0F, 0x00);
    framebuffer_write(6, 7, '2', 0x0F, 0x00);
    while (TRUE);
}

