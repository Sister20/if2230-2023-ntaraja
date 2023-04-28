#include "lib-header/stdtype.h"
#include "lib-header/filesystem/fat32.h"

static uint32_t currenDir = ROOT_CLUSTER_NUMBER;
static struct FAT32DirectoryTable curTable;
static char curDirName[300] = "";

struct stack{
    char* data[100];
    int top;
};

void push(struct stack* s, char* data){
    s->top++;
    s->data[s->top] = data;
}

void pop(struct stack* s){
    s->data[s->top] = "";
    s->top--;
}

void getTop(struct stack s, char *x){
    char* c = s.data[s.top];
    int i = 0;
    while(c[i]!='\0'){
        x[i] = c[i];
        i++;
    }
}

void initiateStack(struct stack* s){
    s->top = -1;
}

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
            .parent_cluster_number = currenDir,
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

void dfsPath(char* filename, char* path, struct FAT32DirectoryTable table, char* found){
    // parse filename to name and ext
    char name[9] = "\0\0\0\0\0\0\0\0\0";
    char ext[4] = "\0\0\0\0";
    readfname(filename, (char *)name, (char *)ext);

    // iterate over all of the table
    for(int i = 1 ; i < 64 ; i++){
        if(strcmp(table.table[i].name, name, slen(name)) && strcmp(table.table[i].ext, ext, slen(ext))){
            //add name to path
            int temp_int = 0;
            if(slen(table.table[i].name) > 8){
                temp_int = 8;
            } else {
                temp_int = slen(table.table[i].name);
            }
            for(int j = 0 ; j < temp_int ; j++){
                path[slen(path)] = table.table[i].name[j];
            }

            //if have ext, add it to path
            if(slen(table.table[i].ext) > 0){
                if(slen(table.table[i].ext) > 3){
                    temp_int = 3;
                } else {
                    temp_int = slen(table.table[i].ext);
                }
                path[slen(path)] = '.';
                for(int j = 0 ; j < temp_int ; j++){
                    path[slen(path)] = table.table[i].ext[j];
                }
            }

            //add / to the end as well
            path[slen(path)] = '/';
            path[slen(path)] = '\0';

            //add path to found
            for(int j = 0 ; j < slen(path) ; j++){
                found[slen(found)] = path[j];
            }
            found[slen(found)] = '\n';
            found[slen(found)] = '\0';
        }

        if(table.table[i].attribute || table.table[i].cluster_high || table.table[i].cluster_low){
            //create new path
            char newPath[2048];
            for(int j = 0 ; j < 2048 ; j++){
                newPath[j] = '\0';
            }
            for(int j = 0 ; j < slen(path) ; j++){
                newPath[j] = path[j];
            }
            newPath[slen(path)] = '\0';

            //add current name to path
            for(int j = 0 ; j < slen(table.table[i].name) ; j++){
                newPath[slen(newPath)] = table.table[i].name[j];
            }

            //if have ext, add it to path
            if(slen(table.table[i].ext) > 0){
                newPath[slen(newPath)] = '.';
                for(int j = 0 ; j < slen(table.table[i].ext) ; j++){
                    newPath[slen(newPath)] = table.table[i].ext[j];
                }
            }

            //add / to the end as well
            newPath[slen(newPath)] = '/';
            newPath[slen(newPath)] = '\0';

            //get the new table
            struct FAT32DirectoryTable newTable = {0};
            struct FAT32DriverRequest newRequest = {
                    .buf                   = &newTable,
                    .name                  = "\0\0\0\0\0\0\0\0",
                    .ext                   = "\0\0\0",
                    .parent_cluster_number = table.table[i].cluster_high << 16 | table.table[i].cluster_low,
                    .buffer_size           = CLUSTER_SIZE,
            };

            // set the request name and ext
            for(int k = 0; k < slen(table.table[i].name); k++){
                newRequest.name[k] = table.table[i].name[k];
            }
            for(int k = 0; k < slen(table.table[i].ext); k++){
                newRequest.ext[k] = table.table[i].ext[k];
            }

            uint8_t retcode = 1;
            syscall(1, (uint32_t) &newRequest, (uint32_t) &retcode, 0);
            if (retcode == 0){
                dfsPath(filename, newPath, newTable, found);
            }
        }
    }
}

void whereis(char *filename){
    uint8_t retcode = 1;
    
    // start from root so table is dir table
    struct FAT32DirectoryTable table = {0};
    struct FAT32DriverRequest request = {
            .buf                   = &table,
            .name                  = "ROOT",
            .ext                   = "\0\0\0",
            .parent_cluster_number = ROOT_CLUSTER_NUMBER,
            .buffer_size           = CLUSTER_SIZE,
    };

    syscall(1, (uint32_t) &request, (uint32_t) &retcode, 0);

    // list to store all the paths found (for multiple file with same name)
    char found[2048];
    found[0] = '\0';
    char path[2048];
    path[0] = '/';
    path[1] = '\0';
    dfsPath(filename, path, table, found);
    if (slen(found) != 0){
        syscall(5, (uint32_t) found, slen(found), 0xf);
    } else {
        syscall(5, (uint32_t) "File not found\n", 15, 0xf);
    }
}

int main(void) {
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
        syscall(5, (uint32_t) "ntarAja@OS-IF2230:", 19, 0x0A);
        syscall(5, (uint32_t) curDirName, 300, 0x09);
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
        } else if(strcmp(buf, "whereis", 7)){
            whereis(buf+8);
        }
    }

    return 0;
}
