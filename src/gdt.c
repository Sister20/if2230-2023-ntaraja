#include "lib-header/interrupt/interrupt.h"
#include "lib-header/gdt.h"

static struct GlobalDescriptorTable global_descriptor_table = {
        .table = {
                [0] = { // Null descriptor
                        .segment_low = 0x0000,
                        .base_low = 0x0000,

                        .base_mid = 0x00,
                        .type_bit = 0x0,
                        .non_system = 0x0,
                        .descriptor_privilege_level = 0x0,
                        .present = 0x0,
                        .segment_high = 0x0,
                        .available = 0x0,
                        .long_mode = 0x0,
                        .default_operation_size = 0x0,
                        .granularity = 0x0,
                        .base_high = 0x00
                },
                [1] = { // Kernel mode code segment descriptor
                        .segment_low = 0xFFFF,
                        .base_low = 0x0000,

                        .base_mid = 0x00,
                        .type_bit = 0xA,
                        .non_system = 0x1,
                        .descriptor_privilege_level = 0x0,
                        .present = 0x1,

                        .segment_high = 0xF,
                        .available = 0x0,
                        .long_mode = 0x1,
                        .default_operation_size = 0x1,
                        .granularity = 0x1,
                        .base_high = 0x00
                },
                [2] = { // Kernel mode data segment descriptor
                        .segment_low = 0xFFFF,
                        .base_low = 0x0000,

                        .base_mid = 0x00,
                        .type_bit = 0x2,
                        .non_system = 0x1,
                        .descriptor_privilege_level = 0x0,
                        .present = 0x1,

                        .segment_high = 0xF,
                        .available = 0x0,
                        .long_mode = 0x0,
                        .default_operation_size = 0x1,
                        .granularity = 0x1,
                        .base_high = 0x00
                },
                [3] = { // User mode code segment descriptor
                        .segment_low = 0xFFFF,
                        .base_low = 0x0000,

                        .base_mid = 0x00,
                        .type_bit = 0xA,
                        .non_system = 0x1,
                        .descriptor_privilege_level = 0x3,
                        .present = 0x1,

                        .segment_high = 0xF,
                        .available = 0x0,
                        .long_mode = 0x1,
                        .default_operation_size = 0x1,
                        .granularity = 0x1,
                        .base_high = 0x00
                },
                [4] = { // User mode data segment descriptor
                        .segment_low = 0xFFFF,
                        .base_low = 0x0000,

                        .base_mid = 0x00,
                        .type_bit = 0x2,
                        .non_system = 0x1,
                        .descriptor_privilege_level = 0x3,
                        .present = 0x1,

                        .segment_high = 0xF,
                        .available = 0x0,
                        .long_mode = 0x0,
                        .default_operation_size = 0x1,
                        .granularity = 0x1,
                        .base_high = 0x00
                },
                [5] = { // TSS descriptor
                        .segment_low = sizeof(struct TSSEntry),
                        .base_low = 0x0000,

                        .base_mid = 0x00,
                        .type_bit = 0x9, // 0b1001
                        .non_system = 0x0,
                        .descriptor_privilege_level = 0x0,
                        .present = 0x1,

                        .segment_high = (sizeof(struct TSSEntry) & (0xF << 16)) >> 16,
                        .available = 0x0,
                        .long_mode = 0x0,
                        .default_operation_size = 0x1,
                        .granularity = 0x0,
                        .base_high = 0x00
                }
        }
};

struct GDTR _gdt_gdtr = {
        .size = sizeof(struct GlobalDescriptorTable),
        .address = &global_descriptor_table
};

void gdt_install_tss(void) {
    uint32_t base = (uint32_t) &_interrupt_tss_entry;
    global_descriptor_table.table[5].base_high = (base & (0xFF << 24)) >> 24;
    global_descriptor_table.table[5].base_mid = (base & (0xFF << 16)) >> 16;
    global_descriptor_table.table[5].base_low = base & 0xFFFF;
}