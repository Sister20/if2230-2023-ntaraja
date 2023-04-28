#include "lib-header/stdtype.h"
#include "lib-header/filesystem/fat32.h"

static uint32_t currenDir = ROOT_CLUSTER_NUMBER;
static struct FAT32DirectoryTable curTable; 

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

void readfname(char* s, char* name, char* ext){
    int i = 0;
    while(s[i]!='.' && s[i]!='\0'){
        name[i] = s[i];
        i++;
    }
    name[i] = '\0';
    i++;
    int j = 0;
    while(s[i]!='\0' && j!=3){
        ext[j] = s[i];
        i++;
        j++;
    }
}

int slen(const char* s){
    int len = 0;
    while(s[len]!='\0'){
        len++;
    }
    return len;
}

bool strcmp(const char* str1, const char* str2, int n) {
    int i = 0;
    while (*str1 && (*str1 == *str2) && i < n) {
        str1++;
        str2++;
        i++;
    }
    
    return i==n;
}

void cd(char* filename){
    char name[9] = "\0\0\0\0\0\0\0\0\0";
    char ext[4] = "\0\0\0\0";
    struct FAT32DirectoryTable table = curTable;
    readfname(filename, (char *)name, (char *)ext);
    struct FAT32DriverRequest request = {
            .buf = &table,
            .name = "\0\0\0\0\0\0\0\0",
            .ext = "\0\0\0",
            .parent_cluster_number = ROOT_CLUSTER_NUMBER,
            .buffer_size = 0
    };
    int32_t retcode = 0;
    for(int i = 0; i < slen(name); i++){
        request.name[i] = name[i];
    }
    for(int i = 0; i < slen(ext); i++){
        request.ext[i] = ext[i];
    }
    syscall(1, (uint32_t) &request, (uint32_t) &retcode, 0);
    curTable = table;
    currenDir = (table.table[0].cluster_high << 16 | table.table[0].cluster_low);
}

void cat(char* filename){
    char name[9] = "\0\0\0\0\0\0\0\0\0";
    char ext[4] = "\0\0\0\0";
    readfname(filename, (char *)name, (char *)ext);
    struct ClusterBuffer cl           = {0};
    struct FAT32DriverRequest request = {
            .buf                   = &cl,
            .name                  = "\0\0\0\0\0\0\0\0",
            .ext                   = "\0\0\0",
            .parent_cluster_number = currenDir,
            .buffer_size           = CLUSTER_SIZE,
    };
    int32_t retcode = 0;
    for(int i = 0; i < slen(name); i++){
        request.name[i] = name[i];
    }
    for(int i = 0; i < slen(ext); i++){
        request.ext[i] = ext[i];
    }
    syscall(0, (uint32_t) &request, (uint32_t) &retcode, 0);
    int size = 0;
    while(cl.buf[size] != '\0'){
        size++;
    }
    
    if (retcode == 0){
        syscall(5, (uint32_t) cl.buf, size, 0xf);
    }
}

void rm(char* filename) {
    char name[9] = "\0\0\0\0\0\0\0\0\0";
    char ext[4] = "\0\0\0\0";
    readfname(filename, (char *)name, (char*) ext);
    
    struct ClusterBuffer cl           = {0};
    struct FAT32DriverRequest request = {
            .buf                   = &cl,
            .name                  = "\0\0\0\0\0\0\0\0",
            .ext                   = "\0\0\0",
            .parent_cluster_number = currenDir,
            .buffer_size           = CLUSTER_SIZE,
    };
    int32_t retcode = 0;
    for(int i = 0; i < slen(name); i++){
        request.name[i] = name[i];
    }
    for(int i = 0; i < slen(ext); i++){
        request.ext[i] = ext[i];
    }
    syscall(3, (uint32_t) &request, (uint32_t) &retcode, 0);
}

int main(void) {
    static char *curdir = "";
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
        syscall(5, (uint32_t) curdir, 256, 0x09);
        syscall(5, (uint32_t) "/$ ", 3, 0xF);
        
        // puts("ntarAja@OS-IF2230", 17, 0x0A);
        // puts(":", 1, 0xF);
        // puts(curdir, length_of(curdir), 0x09);
        // puts("/", 1, 0x09);
        // puts("$ ", 2, 0xF);

        syscall(4, (uint32_t) buf, 256, 0);
        // syscall(5, (uint32_t) buf, 256, 0xF);
        if(strcmp(buf, "cat", 3)){
            cat(buf+4);
        } else if(strcmp(buf, "cd", 2)){
            cd(buf+3);
        } else if(strcmp(buf, "rm", 2)) {
            rm(buf + 3);
        }
    }

    return 0;
}
