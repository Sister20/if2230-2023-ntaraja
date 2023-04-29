#include "lib-header/stdtype.h"
#include "lib-header/filesystem/fat32.h"

static uint32_t currenDir = ROOT_CLUSTER_NUMBER;
static struct FAT32DirectoryTable curTable;
static char curDirName[300] = "/\0";
static struct FAT32DirectoryTable rootTable;

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

void splitPath(char* path, char* pre, char* post){
    int i = 1;
    // initialize pre and post
    for (int i = 0 ; i < 300 ; i++){
        pre[i] = '\0';
        post[i] = '\0';
    }
    while(path[i]!='/' && path[i]!='\0'){
        pre[i-1] = path[i];
        i++;
    }
    pre[i] = '\0';
    i++;
    post[0] = '/';
    int j = 1;
    while(path[i]!='\0'){
        post[j] = path[i];
        i++;j++;
    }
}

bool clusterExist(struct FAT32DirectoryTable table, uint32_t clusterNumber){
    for (int i = 1 ; i < 64 ; i++){
        if (clusterNumber == (uint32_t)(table.table[i].cluster_high << 16 | table.table[i].cluster_low)){
            return TRUE;
        }
    }
    return FALSE;
}

void cd(char* filename){
    // check if ..
    if (strcmp("..\0", filename, 2)){
        if (currenDir == ROOT_CLUSTER_NUMBER){
            syscall(5, (uint32_t) "Already at root directory\n", 27, 0xf);
            return;
        }
        // currenDir = (uint32_t)curTable.table[0].cluster_high << 16 | curTable.table[0].cluster_low; // target cluster number
    
        // start from root so curtable is set to root table
        struct FAT32DirectoryTable table = rootTable;

        // iterate from root to parent
        char temp[300] = "\0";
        temp[0] = '/';
        temp[1] = '\0';
        int i = 0;
        char pre[300] = "\0";
        char post[300] = "\0";
        // initialize post as curDirName
        for (int i = 0 ; i < slen(curDirName); i++){
            post[i] = curDirName[i];
        }
        while(!clusterExist(table, currenDir)){
            // save current directory name
            char tempChar[300] = "\0";
            for (int j = 0 ; j < 300 ; j++){
                tempChar[j] = '\0';
            }
            for (int j = 0 ; j < slen(post); j++){
                tempChar[j] = post[j];
            }
            splitPath(tempChar, pre, post);
            i = 0;
            while(pre[i]!='\0'){
                temp[i] = pre[i];
                i++;
            }
            temp[i] = '/';

            // split pre into name and ext
            char name[9] = "\0\0\0\0\0\0\0\0\0";
            char ext[4] = "\0\0\0\0";
            readfname(pre, name, ext);
            uint8_t retcode = 0;

            // int temp_int = 1;
            // while(!(strcmp(curTable.table[temp_int].name, name, slen(name)) && strcmp(curTable.table[temp_int].ext, ext, slen(ext)))){
            //     temp_int++;
            // }
            struct FAT32DriverRequest tempRequest = {
                .buf                    = &table,
                .name                   = "\0\0\0\0\0\0\0\0",
                .ext                    = "\0\0\0",
                .parent_cluster_number  = (table.table[0].cluster_high << 16 | table.table[0].cluster_low),
                .buffer_size            = 0,
            };

            // clear tempRequest name and ext
            for (int j = 0; j < 8; j++){
                tempRequest.name[j] = '\0';
            }
            for (int j = 0; j < 3; j++){
                tempRequest.ext[j] = '\0';
            }

            // set it to name and ext
            int temp_int = 0;
            if (slen(name) > 8){
                temp_int = 8;
            } else {
                temp_int = slen(name);
            }
            for (int j = 0; j < temp_int; j++){
                tempRequest.name[j] = name[j];
            }
            if (slen(ext) > 3){
                temp_int = 3;
            } else {
                temp_int = slen(ext);
            }
            for (int j = 0; j < temp_int; j++){
                tempRequest.ext[j] = ext[j];
            }
            syscall(1, (uint32_t) &tempRequest, (uint32_t)&retcode, 0);
            if (retcode != 0){
                syscall(5, (uint32_t) "Directory not found\n", 21, 0xf);
                return;
            }
        }
        // save current directory name
        for(int j = 0 ; j < 300 ; j++){
            curDirName[j] = '\0';
        }
        for(int j = 0 ; j < slen(temp) ; j++){
            curDirName[j] = temp[j];
        }
        curTable = table;
        currenDir = (table.table[0].cluster_high << 16 | table.table[0].cluster_low);

        return;
    }

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
    if(retcode != 0){
        syscall(5, (uint32_t) "No such directory\n", 18, 0xf);
        return;
    }
    for (int i = 0; i < slen(name); i++){
        curDirName[slen(curDirName)] = name[i];
    }
    curDirName[slen(curDirName)] = '/';
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
    
    struct FAT32DirectoryTable table;
    struct FAT32DriverRequest request = {
            .buf                   = &table,
            .name                  = "\0\0\0\0\0\0\0\0",
            .ext                   = "\0\0\0",
            .parent_cluster_number = currenDir,
            .buffer_size           = 0,
    };
    int32_t retcode = 0;
    for(int i = 0; i < slen(name); i++){
        request.name[i] = name[i];
    }
    for(int i = 0; i < slen(ext); i++){
        request.ext[i] = ext[i];
    }
    syscall(1, (uint32_t) &request, (uint32_t) &retcode, 0);
    if(retcode == 0){
        if(slen(table.table[1].name) == 0 && slen(table.table[1].ext) == 0){
            syscall(3, (uint32_t) &request, (uint32_t) &retcode, 0);
        }
    }
    else if (retcode == 1){
        syscall(3, (uint32_t) &request, (uint32_t) &retcode, 0);
    }
    else {
        syscall(5, (uint32_t) "rm: cannot remove '", 20, 0xf);
        syscall(5, (uint32_t) filename, 8, 0xf);
        syscall(5, (uint32_t) "': No such file or directory\n", 30, 0xf);
    }
}

void mkdir(char* filename) {
    char name[9] = "\0\0\0\0\0\0\0\0\0";
    char ext[4] = "\0\0\0\0";
    readfname(filename, (char *)name, (char*) ext);
    bool existDuplicate = FALSE;
    struct FAT32DirectoryTable table = curTable;

    for(int i = 1 ; i < 64 ; i++){
        if(table.table[i].attribute || table.table[i].cluster_high || table.table[i].cluster_low){
            if (strcmp(table.table[i].name, filename, slen(filename)) && slen(table.table[i].ext) == 0) {
                syscall(5, (uint32_t) "mkdir: cannot create directory '", 33, 0xf);
                syscall(5, (uint32_t) filename, 8, 0xf);
                syscall(5, (uint32_t) "': File exists\n", 16, 0xf);
                existDuplicate = TRUE;
                break;
            }
        }
    }

    if (!existDuplicate) {
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
        syscall(2, (uint32_t) &request, (uint32_t) &retcode, 0);
    }
}

void ls(){
    uint32_t retcode;
    struct FAT32DriverRequest request2 = {
            .buf                   = &curTable,
            .name                  = "\0\0\0\0\0\0\0\0",
            .ext                   = "\0\0\0",
            .parent_cluster_number = currenDir,
            .buffer_size           = 0,
    };
    for(int i = 0; i < slen(curTable.table[0].name); i++){
        request2.name[i] = curTable.table[0].name[i];
    }
    syscall(1, (uint32_t) &request2, (uint32_t) &retcode, 0);

    struct FAT32DirectoryTable table = curTable;
    for(int i = 1 ; i < 64 ; i++){
        if(table.table[i].attribute || table.table[i].cluster_high || table.table[i].cluster_low){
            int temp_int = 0;
            if(slen(table.table[i].name) > 8){
                temp_int = 8;
            } else {
                temp_int = slen(table.table[i].name);
            }
            syscall(5, (uint32_t) table.table[i].name, temp_int, 0xf);
            temp_int = 0;
            if(slen(table.table[i].ext) > 3){
                temp_int = 3;
            } else {
                temp_int = slen(table.table[i].ext);
            }
            if(temp_int > 0){
                syscall(5, (uint32_t) ".", 1, 0xf);
            }
            syscall(5, (uint32_t) table.table[i].ext, temp_int, 0xf);
            syscall(5, (uint32_t) "\n", 1, 0xf);
        }
    }
}

void dfsPath(char* filename, char* path, struct FAT32DirectoryTable table, char* found){
    // parse filename to name and ext
    char savedPath[300] = "\0";
    for (int i = 0; i < slen(path); i++) {
        savedPath[i] = path[i];
    }
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

                // reset path
                for (int j = 0; j < 300 ; j++){
                    path[j] = '\0';
                }
                for (int j = 0; j < slen(savedPath); j++){
                    path[j] = savedPath[j];
                }
            }
        }
    }
}

void whereis(char *filename){
    
    // start from root so table is root table
    struct FAT32DirectoryTable table = {0};
    table = rootTable;

    // list to store all the paths found (for multiple file with same name)
    char found[2048];
    for (int i = 0; i < 2048; i++){
        found[i] = '\0';
    }
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

void flipcoin(){
    static uint32_t rand_number = 782439012;
    // create a random number using lcg
    rand_number = (rand_number * 1103515245 + 12345) % 4294967296;
    // get the last bit of the random number
    uint32_t last_bit = rand_number & 1;
    if (last_bit == 0){
        syscall(5, (uint32_t) "Heads\n", 6, 0xf);
    } else {
        syscall(5, (uint32_t) "Tails\n", 6, 0xf);
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

    struct FAT32DriverRequest request2 = {
            .buf                   = &curTable,
            .name                  = "ROOT",
            .ext                   = "\0\0\0",
            .parent_cluster_number = ROOT_CLUSTER_NUMBER,
            .buffer_size           = CLUSTER_SIZE,
    };
    syscall(1, (uint32_t) &request2, (uint32_t) &retcode, 0);
    rootTable = curTable;

    while (TRUE) {
        // ntarAja in bios green light
        syscall(5, (uint32_t) "ntarAja@OS-IF2230:", 19, 0x0A);
        syscall(5, (uint32_t) curDirName, 300, 0x09);
        syscall(5, (uint32_t) "$ ", 2, 0xF);
        
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
        } else if(strcmp(buf, "ls", 2)){
            ls();
        } else if(strcmp(buf, "mkdir", 5)) {
            mkdir(buf + 6);
        } else if(strcmp(buf, "flip a coin", 11)) {
            flipcoin();
        }
    }

    return 0;
}
