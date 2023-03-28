#include "lib-header/filesystem/disk.h"
#include "lib-header/stdmem.h"
#include "lib-header/filesystem/fat32.h"
#include "lib-header/portio.h"

const uint8_t fs_signature[BLOCK_SIZE] = {
        'C', 'o', 'u', 'r', 's', 'e', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ',
        'D', 'e', 's', 'i', 'g', 'n', 'e', 'd', ' ', 'b', 'y', ' ', ' ', ' ', ' ',  ' ',
        'L', 'a', 'b', ' ', 'S', 'i', 's', 't', 'e', 'r', ' ', 'I', 'T', 'B', ' ',  ' ',
        'M', 'a', 'd', 'e', ' ', 'w', 'i', 't', 'h', ' ', '<', '3', ' ', ' ', ' ',  ' ',
        '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '2', '0', '2', '3', '\n',
        [BLOCK_SIZE-2] = 'O',
        [BLOCK_SIZE-1] = 'k',
};

struct DateTime dateTime;
struct ClusterBuffer clusterBuffer;
struct FAT32DriverState driver;

// Temporary interrupt handling
int rtc_updating() {
    out(0x70, 0x0A);
    return in(0x71) & 0x80;
}

static uint8_t get_rtc_register(uint8_t reg) {
    out(0x70, reg);
    return in(0x71);
}

static void init_rtc() {
    while (rtc_updating());

    dateTime.second = get_rtc_register(0x00);
    dateTime.minute = get_rtc_register(0x02);
    dateTime.hour = get_rtc_register(0x04);
    dateTime.day = get_rtc_register(0x07);
    dateTime.month = get_rtc_register(0x08);
    dateTime.year = get_rtc_register(0x09);
}

size_t str_length(const char *str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

char *padded_string(const char *str) {
    static char padded[8];
    memset(padded, ' ', 8);
    memcpy(padded, str, str_length(str));
    return padded;
}

uint32_t cluster_to_lba(uint32_t cluster) {
    return (cluster - 2) * CLUSTER_BLOCK_COUNT + ROOT_CLUSTER_NUMBER;
}

void init_directory_table(struct FAT32DirectoryTable* dir_table, char* name, uint32_t parent_dir_cluster) {
    init_rtc();

    memcpy(dir_table->table[0].name, padded_string(name), 8);
    memcpy(dir_table->table[0].ext, "   ", 3);
    dir_table->table[0].attribute = 0x10;
    dir_table->table[0].user_attribute = UATTR_NOT_EMPTY;
    dir_table->table[0].undelete = 0;
    dir_table->table[0].create_time = dateTime.hour << 11 | dateTime.minute << 5 | dateTime.second >> 1;
    dir_table->table[0].create_date = dateTime.year << 9 | dateTime.month << 5 | dateTime.day;
    dir_table->table[0].access_date = dateTime.year << 9 | dateTime.month << 5 | dateTime.day;
    dir_table->table[0].cluster_high = 0;
    dir_table->table[0].modified_time = dateTime.hour << 11 | dateTime.minute << 5 | dateTime.second >> 1;
    dir_table->table[0].modified_date = dateTime.year << 9 | dateTime.month << 5 | dateTime.day;
    dir_table->table[0].cluster_low = parent_dir_cluster;
    dir_table->table[0].filesize = 0;
}

bool is_empty_storage(void) {
    uint8_t boot_sector[BLOCK_SIZE];
    read_clusters(boot_sector, 0, 1);
    return memcmp(boot_sector, fs_signature, BLOCK_SIZE) != 0;
}

void create_fat32(void) {
    write_clusters(fs_signature, 0, 1);
    struct FAT32FileAllocationTable fat;
    fat.cluster_map[0] = CLUSTER_0_VALUE;
    fat.cluster_map[1] = CLUSTER_1_VALUE;
    fat.cluster_map[2] = FAT32_FAT_END_OF_FILE;
    write_clusters(fat.cluster_map, 1, 1);
    struct FAT32DirectoryTable dir_table;
    init_directory_table(&dir_table, "ROOT", 0);
    write_clusters(dir_table.table, 2, 1);
}

// TODO: Finish
void initialize_filesystem_fat32(void) {
    if (is_empty_storage()) {
        create_fat32();
    } else {
        // Read and Cache FAT into driver
        struct FAT32FileAllocationTable fat;
        struct FAT32DirectoryTable dir_table;
        read_clusters(&fat, 1, 1);
        read_clusters(&dir_table, 2, 1);
    }
}

void write_clusters(const void* ptr, uint32_t cluster_number, uint8_t cluster_count) {
    write_blocks(ptr, cluster_to_lba(cluster_number), cluster_count * CLUSTER_BLOCK_COUNT);
}

void read_clusters(void* ptr, uint32_t cluster_number, uint8_t cluster_count) {
    read_blocks(ptr, cluster_to_lba(cluster_number), cluster_count * CLUSTER_BLOCK_COUNT);
}

/* -- CRUD -- */

// TODO: Implement
int8_t read_directory(struct FAT32DriverRequest request) {
    struct FAT32DirectoryTable dir_table;
    read_clusters(&dir_table, 1, 1);
    if (request.buffer_size != sizeof (struct FAT32DirectoryEntry)) {
        return -1; // Invalid buffer size
    }

    uint32_t cluster_number = request.parent_cluster_number;
    if (memcmp(dir_table.table[cluster_number].name, request.name, 8) == 0) {
        if (dir_table.table[cluster_number].attribute == 0x10) {
            request.buf = &dir_table.table[cluster_number];
            return 0; // Success
        }
        return 1; // Not a folder
    }
    return 2; // Not found
}

// TODO: Implement
int8_t read(struct FAT32DriverRequest request) {
    struct FAT32DirectoryTable dir_table;
    read_clusters(&dir_table, 1, 1);

    uint16_t cluster_number = request.parent_cluster_number;
    if (memcmp(dir_table.table[cluster_number].name, request.name, 8) == 0) {
        if (dir_table.table[cluster_number].attribute == 0x10 || memcmp(dir_table.table[cluster_number].ext, request.ext, 3) != 0) {
            return 1; // Not a file
        }
        struct ClusterBuffer cbuf;
        read_clusters(&cbuf, cluster_number, 1);
        memcpy(request.buf, cbuf.buf, request.buffer_size);
        if (memcmp(request.buf, cbuf.buf, request.buffer_size))
        return 0; // Success
    }
    return 2; // Not enough buffer
}

// TODO: Implement
int8_t write(struct FAT32DriverRequest request) {
    struct FAT32DirectoryTable dir_table;
    read_clusters(&dir_table, 1, 1);

    uint16_t cluster_number = request.parent_cluster_number;
    if (memcmp(dir_table.table[cluster_number].name, request.name, 8) == 0) {
        if (dir_table.table[cluster_number].attribute == 0x10 || memcmp(dir_table.table[cluster_number].ext, request.ext, 3) != 0) {
            // Make a folder
            struct FAT32DirectoryEntry dir_entry;
            memcpy(dir_entry.name, padded_string(request.name), 8);
            memcpy(dir_entry.ext, "   ", 3);
            dir_entry.attribute = 0x10;
            dir_entry.user_attribute = ATTR_SUBDIRECTORY;
            dir_entry.undelete = 0;
            dir_entry.create_time = dateTime.hour << 11 | dateTime.minute << 5 | dateTime.second >> 1;
            dir_entry.create_date = dateTime.year << 9 | dateTime.month << 5 | dateTime.day;
            dir_entry.access_date = dateTime.year << 9 | dateTime.month << 5 | dateTime.day;
            dir_entry.cluster_high = 0;
            dir_entry.modified_time = dateTime.hour << 11 | dateTime.minute << 5 | dateTime.second >> 1;
            dir_entry.modified_date = dateTime.year << 9 | dateTime.month << 5 | dateTime.day;
            dir_entry.cluster_low = cluster_number;
            dir_entry.filesize = 0;
        }
        if (dir_table.table[cluster_number].attribute != 0x01) {
            struct ClusterBuffer cbuf;
            memcpy(cbuf.buf, request.buf, request.buffer_size);
            write_clusters(&cbuf, cluster_number, 1);
            return 0; // Success
        }
        return -1;
    }
    return 2; // Not found
}

// TODO: Implement
int8_t delete(struct FAT32DriverRequest request) {
    struct FAT32DirectoryTable dir_table;
    read_clusters(&dir_table, 1, 1);

    uint16_t cluster_number = request.parent_cluster_number;
    if (memcmp(dir_table.table[cluster_number].name, request.name, 8) == 0) {
        if (dir_table.table[cluster_number].attribute == 0x10 || memcmp(dir_table.table[cluster_number].ext, request.ext, 3) != 0) {
            return 1; // Not a file
        }
        dir_table.table[cluster_number].attribute = 0x01;
        write_clusters(&dir_table, 1, 1);
        return 0; // Success
    }
    return 2; // Not found
}