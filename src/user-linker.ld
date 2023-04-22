ENTRY(_start)
OUTPUT_FORMAT("binary")

SECTIONS {
    . = 0x00000000; /* start address, may be changed later */

    .text ALIGN(4):
    {
        user-entry.o(.text)
        *(.text)
    }

    .data ALIGN(4):
    {
        *(.data)
    }

    .rodata ALIGN(4):
    {
        *(.rodata)
    }

    _linker_user_program_end = .;
    ASSERT ((_linker_user_program_end <= 1048576), "Error: user program size exceeds 1MB!")
}