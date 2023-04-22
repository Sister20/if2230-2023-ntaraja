global loader                           ; the entry symbol for ELF
global enter_protected_mode             ; go to protected mode
global set_tss_register                 ; set the TSS register to GDT entry
extern kernel_setup                     ; kernel

KERNEL_VIRTUAL_BASE equ 0xC0000000      ; kernel virtual base address
KERNEL_STACK_SIZE   equ 2097152         ; size of stack in bytes
MAGIC_NUMBER        equ 0x1BADB002      ; define the magic number constant
FLAGS               equ 0x0             ; multiboot flags
CHECKSUM            equ -MAGIC_NUMBER   ; calculate the checksum
                                        ; (magic number + checksum + flags should equal 0)

section .bss
align 4                                 ; align at 4 bytes
kernel_stack:                           ; label points to beginning of memory
    resb KERNEL_STACK_SIZE              ; reserve stack for the kernel

section .multiboot                      ; multiboot header
align 4                                 ; the code must be 4 byte aligned
    dd MAGIC_NUMBER                     ; write the magic number to the machine code,
    dd FLAGS                            ; the flags,
    dd CHECKSUM                         ; and the checksum


section .text                                  ; the code section
loader:                                        ; the loader label (defined as entry point in linker script)
    mov  esp, kernel_stack + KERNEL_STACK_SIZE ; setup stack register to proper location
    call kernel_setup
.loop:
    jmp .loop                                  ; loop forever


; More details: https://en.wikibooks.org/wiki/X86_Assembly/Protected_Mode
enter_protected_mode:
    cli
    mov  eax, [esp+4]
    lgdt [eax]
    ;       eax at this line will carry GDTR location, dont forget to use square bracket [eax]

    mov  eax, cr0
    or al, 1
    mov  cr0, eax

    ; Far jump to update cs register
    ; Warning: Invalid GDT will raise exception in any instruction below
    jmp 0x8:flush_cs

flush_cs:
    mov ax, 10h
    mov ds, ax
    mov es, ax
    mov ss, ax

    ret

set_tss_register:
    mov  eax, 0x28 | 0
    ltr  ax
    ret
