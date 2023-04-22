#include "lib-header/stdtype.h"

_Noreturn int main(void) {
    __asm__ volatile ("mov $0x0, %%eax"
            : : "r"(0xDEADBEEF));
    while (TRUE);
}