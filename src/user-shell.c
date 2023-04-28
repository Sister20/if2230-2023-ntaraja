#include "lib-header/stdtype.h"
#include "lib-header/filesystem/fat32.h"

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

int length_of(const char* str){
    int i = 0;
    while (str[i] != '\0'){i++;str++;}
    return i;
}

int main(void) {
    static char *curdir;
    struct ClusterBuffer cl           = {0};
    struct FAT32DriverRequest request = {
            .buf                   = &cl,
            .name                  = "ikanaide",
            .ext                   = "\0\0\0",
            .parent_cluster_number = ROOT_CLUSTER_NUMBER,
            .buffer_size           = CLUSTER_SIZE,
    };
    int32_t retcode;
    syscall(0, (uint32_t) &request, (uint32_t) &retcode, 0);
    if (retcode == 0){
        // syscall(5, (uint32_t) "owo\n", 4, 0xF);
        // syscall(5, (uint32_t) cl.buf, 10, 0xf);
    }
    char buf[256];
    while (TRUE) {
        // ntarAja in bios green light
        syscall(5, (uint32_t) "ntarAja@OS-IF2230:", 18, 0x0A);
        syscall(5, (uint32_t) curdir, length_of(curdir), 0x09);
        syscall(5, (uint32_t) "/$ ", 3, 0xF);
        
        // puts("ntarAja@OS-IF2230", 17, 0x0A);
        // puts(":", 1, 0xF);
        // puts(curdir, length_of(curdir), 0x09);
        // puts("/", 1, 0x09);
        // puts("$ ", 2, 0xF);

        syscall(4, (uint32_t) buf, 256, 0);
        // syscall(5, (uint32_t) buf, 256, 0xF);
    }

    return 0;
}
