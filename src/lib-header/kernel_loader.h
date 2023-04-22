#ifndef _KERNEL_LOADER
#define _KERNEL_LOADER

/**
 * Load GDT from gdtr and launch protected mode. This function defined in asm source code.
 * 
 * @param gdtr Pointer to already defined & initialized GDTR
 * @warning Invalid address / definition of GDT will cause bootloop after calling this function.
 */
extern void enter_protected_mode(struct GDTR *gdtr);

extern uint32_t _linker_kernel_virtual_addr_start;
extern uint32_t _linker_kernel_virtual_addr_end;
extern uint32_t _linker_kernel_physical_addr_start;
extern uint32_t _linker_kernel_physical_addr_end;
extern uint32_t _linker_kernel_stack_top;

/**
 * Executes user program from the kernel.
 * @param virtual_addr Pointer into the virtual address space of the user program.
 * @warning Pointed memory is assumed to be properly loaded.
 */
extern void kernel_execute_user_program(void *virtual_addr);

/**
 * Set TSS register to GDT_TSS_SEGMENT_SELECTOR with Ring 0.
 */
extern void set_tss_register(void);

#endif