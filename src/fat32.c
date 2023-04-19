#include "lib-header/filesystem/disk.h"
#include "lib-header/stdmem.h"
#include "lib-header/filesystem/fat32.h"
#include "lib-header/portio.h"

const uint8_t fs_signature[BLOCK_SIZE] = {
        'C', 'o', 'u', 'r', 's', 'e', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        'D', 'e', 's', 'i', 'g', 'n', 'e', 'd', ' ', 'b', 'y', ' ', ' ', ' ', ' ', ' ',
        'L', 'a', 'b', ' ', 'S', 'i', 's', 't', 'e', 'r', ' ', 'I', 'T', 'B', ' ', ' ',
        'M', 'a', 'd', 'e', ' ', 'w', 'i', 't', 'h', ' ', '<', '3', ' ', ' ', ' ', ' ',
        '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '2', '0', '2', '3', '\n',
        [BLOCK_SIZE - 2] = 'O',
        [BLOCK_SIZE - 1] = 'k',
};

static struct FAT32DriverState driver;

static bool cluster_dir_map[CLUSTER_MAP_SIZE] = {
        FALSE, FALSE, TRUE,
        [3 ... CLUSTER_MAP_SIZE - 1] = FALSE
};

// Temporary interrupt handling
int rtc_updating() {
    out(0x70, 0x0A);
    return in(0x71) & 0x80;
}

uint8_t get_rtc_register(uint8_t reg) {
    out(0x70, reg);
    return in(0x71);
}

void init_rtc(struct DateTime *dateTime) {
    while (rtc_updating());
    dateTime->second = get_rtc_register(0x00);
    dateTime->minute = get_rtc_register(0x02);
    dateTime->hour = get_rtc_register(0x04);
    dateTime->day = get_rtc_register(0x07);
    dateTime->month = get_rtc_register(0x08);
    dateTime->year = get_rtc_register(0x09);
}

inline uint32_t cluster_to_lba(uint32_t cluster) {
    return cluster * CLUSTER_BLOCK_COUNT;
}

void flush(uint32_t dir_cluster) {
    write_clusters(&driver.fat_table, FAT_CLUSTER_NUMBER, 1);
    write_clusters(&driver.dir_table_buf, dir_cluster, 1);
}

void init_directory_entry(struct FAT32DirectoryEntry *dir_entry, char *name, uint32_t parent_dir_cluster) {
    struct DateTime dateTime;
    init_rtc(&dateTime);
    memcpy(dir_entry->name, name, 8);
    memcpy(dir_entry->ext, "\0\0\0", 3);
    dir_entry->attribute = ATTR_SUBDIRECTORY;
    dir_entry->user_attribute = UATTR_NOT_EMPTY;
    dir_entry->undelete = 0;
    dir_entry->create_time = dateTime.hour << 11 | dateTime.minute << 5 | dateTime.second >> 1;
    dir_entry->create_date = dateTime.year << 9 | dateTime.month << 5 | dateTime.day;
    dir_entry->access_date = dateTime.year << 9 | dateTime.month << 5 | dateTime.day;
    dir_entry->cluster_high = parent_dir_cluster >> 16 & 0xFFFF;
    dir_entry->modified_time = dateTime.hour << 11 | dateTime.minute << 5 | dateTime.second >> 1;
    dir_entry->modified_date = dateTime.year << 9 | dateTime.month << 5 | dateTime.day;
    dir_entry->cluster_low = parent_dir_cluster & 0xFFFF;
    dir_entry->filesize = 0;
}

void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster) {
    init_directory_entry(&dir_table->table[0], name, parent_dir_cluster);
}

bool is_empty_storage(void) {
    uint8_t boot_sector[BLOCK_SIZE];
    read_clusters(boot_sector, BOOT_SECTOR, 1);
    return memcmp(boot_sector, fs_signature, BLOCK_SIZE) != 0;
}

void create_fat32(void) {
    write_clusters(fs_signature, BOOT_SECTOR, 1);

    struct FAT32FileAllocationTable fat = {
            .cluster_map = {CLUSTER_0_VALUE, CLUSTER_1_VALUE, FAT32_FAT_END_OF_FILE,
                    [3 ... CLUSTER_MAP_SIZE - 1] = FAT32_FAT_EMPTY_ENTRY}
    };

    struct FAT32DirectoryTable dir_table;
    init_directory_table(&dir_table, "ROOT\0\0\0\0", ROOT_CLUSTER_NUMBER);

    driver.fat_table = fat;
    write_clusters(&fat, FAT_CLUSTER_NUMBER, 1);
    write_clusters(&dir_table, ROOT_CLUSTER_NUMBER, 1);
}

void initialize_filesystem_fat32(void) {
    if (is_empty_storage()) {
        create_fat32();
    } else {
        // Read and Cache FAT into driver
        read_clusters(&driver.fat_table, FAT_CLUSTER_NUMBER, 1);
    }
}

void write_clusters(const void *ptr, uint32_t cluster_number, uint8_t cluster_count) {
    write_blocks(ptr, cluster_to_lba(cluster_number), cluster_count);
}

void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count) {
    read_blocks(ptr, cluster_to_lba(cluster_number), cluster_count);
}

/*** -- CRUD -- ***/

int32_t find_empty_entry(struct FAT32DirectoryTable *dir_table) {
    for (int32_t i = 0; i < (int32_t) (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
        if (dir_table->table[i].name[0] == 0x00) {
            return i;
        }
    }
    return -1;
}

int32_t dir_table_search(char *name, char *ext) {
    struct FAT32DirectoryTable *dir_table = &driver.dir_table_buf;
    for (int32_t i = 0; i < (int32_t) (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
        if (memcmp(dir_table->table[i].name, name, 8) == 0
            && memcmp(dir_table->table[i].ext, ext, 3) == 0) {
            return i;
        }
    }
    return -1;
}

void add_directory_entry(struct FAT32DriverRequest request, uint32_t cluster) {
    uint32_t folder = find_empty_entry(&driver.dir_table_buf);

    memcpy(&driver.dir_table_buf.table[folder].name, request.name, 8);
    memcpy(&driver.dir_table_buf.table[folder].ext, request.ext, 3);
    driver.dir_table_buf.table[folder].user_attribute = 0;
    if (request.buffer_size == 0) {
        driver.dir_table_buf.table[folder].attribute = ATTR_SUBDIRECTORY;
    } else {
        driver.dir_table_buf.table[folder].attribute = ATTR_ARCHIVE;
    }
    driver.dir_table_buf.table[folder].cluster_high = (uint16_t) (cluster >> 16);
    driver.dir_table_buf.table[folder].cluster_low = (uint16_t) cluster;
    driver.dir_table_buf.table[folder].filesize = request.buffer_size;
}

bool is_dir_cluster(uint32_t cluster) {
    for (uint32_t i = 2; i < CLUSTER_MAP_SIZE; i++) {
        if (cluster_dir_map[i]) {
            read_clusters(&driver.dir_table_buf, i, 1);
            struct FAT32DirectoryTable *dir_table = &driver.dir_table_buf;
            uint32_t j = 0;
            struct FAT32DirectoryEntry *dir_entry = &dir_table->table[j];

            while (dir_entry->user_attribute == UATTR_NOT_EMPTY) {
                if (dir_entry->attribute == ATTR_SUBDIRECTORY) {
                    cluster_dir_map[(dir_entry->cluster_high << 16) | dir_entry->cluster_low] = TRUE;
                }
                dir_entry = &dir_table->table[++j];
            }
        }
    }
    return cluster_dir_map[cluster];
}

int8_t read_directory(struct FAT32DriverRequest request) {
    read_clusters(&driver.dir_table_buf, request.parent_cluster_number, 1);
    struct FAT32DirectoryTable *dir_table = &driver.dir_table_buf;

    int32_t index = dir_table_search(request.name, request.ext);

    if (index == -1) {
        return 2; // Not found
    }

    if (dir_table->table[index].attribute == ATTR_SUBDIRECTORY && dir_table->table[index].undelete != 0xE5) {
        read_clusters(request.buf, (dir_table->table[index].cluster_high << 16) | dir_table->table[index].cluster_low,
                      1); // Needed?
        return 0; // Success
    } else if (dir_table->table[index].attribute != ATTR_SUBDIRECTORY) {
        return 1; // Not a folder
    }

    return -1; // Unknown error / prohibited for reading due to undelete flag
}

int8_t read(struct FAT32DriverRequest request) {
    read_clusters(&driver.dir_table_buf, request.parent_cluster_number, 1);
    struct FAT32DirectoryTable *dir_table = &driver.dir_table_buf;
    int32_t index = dir_table_search(request.name, request.ext);

    if (index == -1) {
        return 3; // Not found
    }

    if (dir_table->table[index].attribute != ATTR_SUBDIRECTORY) {
        return 1; // Not a file
    }

    if (request.buffer_size < dir_table->table[index].filesize) {
        return 2; // Not enough space
    }

    uint32_t cluster = (dir_table->table[index].cluster_high << 16) | dir_table->table[index].cluster_low;

    if (!cluster) {
        return -1; // Unknown error
    }

    do {
        read_clusters((struct ClusterBuffer *) (request.buf)++, cluster, 1);
        cluster = driver.fat_table.cluster_map[cluster];
    } while (cluster != FAT32_FAT_END_OF_FILE);

    return 0; // Success
}

int8_t write(struct FAT32DriverRequest request) {
    struct FAT32FileAllocationTable *fat_table = &driver.fat_table;
    read_clusters(&driver.dir_table_buf, request.parent_cluster_number, 1);
    struct FAT32DirectoryTable *dir_table = &driver.dir_table_buf;
    uint32_t empty_entry = find_empty_entry(dir_table);

    if (!is_dir_cluster(request.parent_cluster_number)) {
        return 2; // Parent folder is not a directory
    }

    int32_t index = dir_table_search(request.name, request.ext);
    if (index != -1) {
        return 1; // File / folder already exists
    }

    // Folder
    if (request.buffer_size == 0) {
        uint32_t usable_cluster = 3;
        do {
            if (fat_table->cluster_map[usable_cluster] == FAT32_FAT_EMPTY_ENTRY) {
                fat_table->cluster_map[usable_cluster] = FAT32_FAT_END_OF_FILE;
                struct FAT32DirectoryEntry new_entry;
                init_directory_entry(&new_entry, request.name, request.parent_cluster_number);
                add_directory_entry(request, usable_cluster);
                flush(request.parent_cluster_number);
                return 0;
            }
            usable_cluster++;
        } while (TRUE);
    }
    // File
    uint32_t buffer_size = request.buffer_size;
    uint32_t non_reserved_cluster = 3;
    uint32_t cluster = 0;

    do {
        if (non_reserved_cluster == CLUSTER_MAP_SIZE) {
            return -1; // No empty clusters
        }

        if (fat_table->cluster_map[non_reserved_cluster] == FAT32_FAT_EMPTY_ENTRY) {
            if (cluster > 2) {
                fat_table->cluster_map[cluster] = non_reserved_cluster;
            } else {
                struct FAT32DirectoryEntry dir_entry;
                init_directory_entry(&dir_entry, request.name, request.parent_cluster_number);
                dir_entry.attribute = ATTR_ARCHIVE;
                memcpy(&dir_table->table[empty_entry], &dir_entry, sizeof(struct FAT32DirectoryEntry));
                flush(non_reserved_cluster);
            }
            fat_table->cluster_map[non_reserved_cluster] = FAT32_FAT_END_OF_FILE;
            write_clusters((struct ClusterBuffer *) (request.buf)++, non_reserved_cluster, 1);

            buffer_size -= CLUSTER_SIZE;
            cluster = non_reserved_cluster;
        }
        non_reserved_cluster++;

    } while (buffer_size > 0);

    if (non_reserved_cluster) {
        write_clusters(&driver.fat_table, FAT_CLUSTER_NUMBER, 1);
        return 0;
    }

    return -1;
}

int8_t delete(struct FAT32DriverRequest request) {
    struct FAT32FileAllocationTable *fat_table = &driver.fat_table;
    for (uint32_t cluster = request.parent_cluster_number; cluster < CLUSTER_MAP_SIZE; cluster++) {
        // This will check every EOF which mark doubles as a directory
        if (fat_table->cluster_map[cluster] == FAT32_FAT_END_OF_FILE) {
            // Check if the directory is the one we are looking for
            read_clusters(&driver.dir_table_buf, cluster, 1);
            struct FAT32DirectoryTable *dir_table = &driver.dir_table_buf;
            for (int i = 0; i < 64; i++) {
                if (memcmp(dir_table->table[i].name, request.name, 8) == 0) {
                    if (dir_table->table[i].user_attribute == UATTR_NOT_EMPTY) {
                        return 2; // Not empty
                    }
                    uint32_t folder_cluster =
                            (dir_table->table[i].cluster_high << 16) | dir_table->table[i].cluster_low;
                    fat_table->cluster_map[folder_cluster] = FAT32_FAT_EMPTY_ENTRY;
                    memset(&dir_table->table[i], 0, sizeof(struct FAT32DirectoryEntry));
                    flush(cluster);
                    return 0; // Success
                }
            }
        }
    }
    return 1;
}