#include "lib-header/filesystem/disk.h"
#include "lib-header/stdmem.h"
#include "lib-header/filesystem/fat32.h"
#include "lib-header/portio.h"
// #include "lib-header/portio.h"

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

static struct FAT32FileAllocationTable *fat_table = &driver.fat_table;
static struct FAT32DirectoryTable *dir_table = &driver.dir_table_buf;

static bool cluster_dir_map[CLUSTER_MAP_SIZE] = {
        FALSE, FALSE, TRUE,
        [3 ... CLUSTER_MAP_SIZE - 1] = FALSE
};

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

void read_to_fat_table(uint32_t cluster, uint8_t count) {
    memset(&driver.cluster_buf, 0, CLUSTER_SIZE);
    read_clusters(&driver.cluster_buf, cluster, count);
    memcpy(&driver.fat_table, &driver.cluster_buf, sizeof(struct FAT32FileAllocationTable));
    memset(&driver.cluster_buf, 0, CLUSTER_SIZE);
}

void read_to_dir_table(uint32_t cluster, uint8_t count) {
    memset(&driver.cluster_buf, 0, CLUSTER_SIZE);
    read_clusters(&driver.cluster_buf, cluster, count);
    memcpy(&driver.dir_table_buf, &driver.cluster_buf, sizeof(struct FAT32DirectoryTable));
    memset(&driver.cluster_buf, 0, CLUSTER_SIZE);
}

void flush(uint32_t dir_cluster) {
    memset(&driver.cluster_buf, 0, CLUSTER_SIZE);
    memcpy(&driver.cluster_buf, &driver.fat_table, sizeof(struct FAT32FileAllocationTable));
    write_clusters(&driver.cluster_buf, FAT_CLUSTER_NUMBER, 1);

    memset(&driver.cluster_buf, 0, CLUSTER_SIZE);
    memcpy(&driver.cluster_buf, &driver.dir_table_buf, sizeof(struct FAT32DirectoryTable));
    write_clusters(&driver.cluster_buf, dir_cluster, 1);
//    write_clusters((struct ClusterBuffer *) &driver.fat_table, FAT_CLUSTER_NUMBER, 1);
//    write_clusters((struct ClusterBuffer *) &driver.dir_table_buf, dir_cluster, 1);
}

void init_directory_entry(struct FAT32DirectoryEntry *dir_entry, char *name, uint32_t cluster) {
    struct DateTime dateTime;
    init_rtc(&dateTime);
    memcpy(dir_entry->name, name, 8);
    memcpy(dir_entry->ext, "\0\0\0", 3);
    dir_entry->attribute = ATTR_SUBDIRECTORY;
    dir_entry->user_attribute = 0;
    dir_entry->undelete = 0;
    dir_entry->create_time = dateTime.hour << 11 | dateTime.minute << 5 | dateTime.second >> 1;
    dir_entry->create_date = dateTime.year << 9 | dateTime.month << 5 | dateTime.day;
    dir_entry->access_date = dateTime.year << 9 | dateTime.month << 5 | dateTime.day;
    dir_entry->cluster_high = cluster >> 16 & 0xFFFF;
    dir_entry->modified_time = dateTime.hour << 11 | dateTime.minute << 5 | dateTime.second >> 1;
    dir_entry->modified_date = dateTime.year << 9 | dateTime.month << 5 | dateTime.day;
    dir_entry->cluster_low = cluster & 0xFFFF;
    dir_entry->filesize = 0;
}

void init_directory_table(struct FAT32DirectoryTable *dir, char *name, uint32_t parent_dir_cluster) {
    init_directory_entry(&dir->table[0], name, parent_dir_cluster);
    dir->table[0].user_attribute = UATTR_NOT_EMPTY;
}

bool is_empty_storage(void) {
    uint8_t boot_sector[BLOCK_SIZE];
    read_clusters(&driver.cluster_buf, BOOT_SECTOR, 1);
    memcpy(boot_sector, &driver.cluster_buf, BLOCK_SIZE);
    return memcmp(boot_sector, fs_signature, BLOCK_SIZE) != 0;
}

void create_fat32(void) {
    struct ClusterBuffer temp_cbuf = {0};
    memcpy(&temp_cbuf, fs_signature, BLOCK_SIZE);
    write_clusters(&temp_cbuf, BOOT_SECTOR, 1);

    struct FAT32FileAllocationTable fat = {
            .cluster_map = {CLUSTER_0_VALUE, CLUSTER_1_VALUE, FAT32_FAT_END_OF_FILE,
                    [3 ... CLUSTER_MAP_SIZE - 1] = FAT32_FAT_EMPTY_ENTRY}
    };

    struct FAT32DirectoryTable root_dir_table = {0};
    init_directory_table(&root_dir_table, "ROOT\0\0\0\0", ROOT_CLUSTER_NUMBER);
    driver.fat_table = fat;

    memset(&temp_cbuf, 0, CLUSTER_SIZE);
    memcpy(&temp_cbuf, &fat_table, sizeof(struct FAT32FileAllocationTable));

    write_clusters(&temp_cbuf, FAT_CLUSTER_NUMBER, 1);

    memset(&temp_cbuf, 0, CLUSTER_SIZE);
    memcpy(&temp_cbuf, &root_dir_table, sizeof(struct FAT32DirectoryTable));

    write_clusters(&temp_cbuf, ROOT_CLUSTER_NUMBER, 1);
//    write_clusters(&root_dir_table, ROOT_CLUSTER_NUMBER, 1);
}

void initialize_filesystem_fat32(void) {
    if (is_empty_storage()) {
        create_fat32();
    } else {
        // Read and Cache FAT into driver
//        read_clusters(&driver.cluster_buf, FAT_CLUSTER_NUMBER, 1);
//        memcpy(&driver.fat_table, &driver.cluster_buf, sizeof(struct FAT32FileAllocationTable));
        read_to_fat_table(FAT_CLUSTER_NUMBER, 1);
    }
}

void write_clusters(const void *ptr, uint32_t cluster_number, uint8_t cluster_count) {
    write_blocks(ptr, cluster_to_lba(cluster_number), cluster_count * CLUSTER_BLOCK_COUNT);
}

void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count) {
    read_blocks(ptr, cluster_to_lba(cluster_number), cluster_count * CLUSTER_BLOCK_COUNT);
}

/*** -- CRUD -- ***/
const char empty_name[8] = {0};

int32_t find_empty_entry(struct FAT32DirectoryTable *dir) {
    for (int32_t i = 0; i < (int32_t) (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
        if (memcmp(dir->table[i].name, empty_name, 8) == 0) {
            return i;
        }
    }
    return -1;
}

int32_t dir_table_search(char *name, char *ext) {
    for (int32_t i = 0; i < (int32_t) (CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
        if (memcmp(dir_table->table[i].name, name, 8) == 0
            && memcmp(dir_table->table[i].ext, ext, 3) == 0) {
            return i;
        }
    }
    return -1;
}

void add_directory_entry(struct FAT32DriverRequest request, uint32_t cluster) {
    uint32_t folder = find_empty_entry(dir_table);

    memcpy(dir_table->table[folder].name, request.name, 8);
    memcpy(dir_table->table[folder].ext, request.ext, 3);
    dir_table->table[folder].user_attribute = 0;
    if (request.buffer_size == 0) {
        dir_table->table[folder].attribute = ATTR_SUBDIRECTORY;
    } else {
        dir_table->table[folder].attribute = ATTR_ARCHIVE;
    }
    dir_table->table[folder].cluster_high = (uint16_t) (cluster >> 16);
    dir_table->table[folder].cluster_low = (uint16_t) cluster;
    dir_table->table[folder].filesize = request.buffer_size;
}

// TODO: Change to use ClusterBuffer
bool is_dir_cluster(uint32_t cluster) {
    struct FAT32DirectoryTable temp_table;
    for (uint32_t i = 2; i < CLUSTER_MAP_SIZE; i++) {
        if (cluster_dir_map[i]) {
            read_clusters(&temp_table, i, 1);
            uint32_t j = 0;
            struct FAT32DirectoryEntry *dir_entry = &temp_table.table[j];

            while (dir_entry->user_attribute == UATTR_NOT_EMPTY) {
                if (dir_entry->attribute == ATTR_SUBDIRECTORY) {
                    cluster_dir_map[(dir_entry->cluster_high << 16) | dir_entry->cluster_low] = TRUE;
                }
                dir_entry = &temp_table.table[++j];
            }
        }
    }
    return cluster_dir_map[cluster];
}

// TODO: Change to use ClusterBuffer
int8_t read_directory(struct FAT32DriverRequest request) {
//    read_clusters(&driver.dir_table_buf, request.parent_cluster_number, 1);
    read_to_dir_table(request.parent_cluster_number, 1);

    int32_t index = dir_table_search(request.name, request.ext);

    if (index == -1) {
        return 2; // Not found
    }

    if (dir_table->table[index].attribute == ATTR_SUBDIRECTORY && dir_table->table[index].undelete != 0xE5) {
        request.buf = &dir_table->table[index];
        return 0; // Success
    } else if (dir_table->table[index].attribute != ATTR_SUBDIRECTORY) {
        return 1; // Not a folder
    }

    return -1; // Unknown error / prohibited for reading due to undelete flag
}

int8_t read(struct FAT32DriverRequest request) {
//    read_clusters(&driver.cluster_buf, request.parent_cluster_number, 1);
    read_to_dir_table(request.parent_cluster_number, 1);
    int32_t index = dir_table_search(request.name, request.ext);

    if (index == -1) {
        return 3; // Not found
    }

    if (dir_table->table[index].attribute != ATTR_SUBDIRECTORY || dir_table->table[index].attribute == ATTR_ARCHIVE) {
        return 1; // Not a file
    }

    if (request.buffer_size < dir_table->table[index].filesize) {
        return 2; // Not enough space
    }

    uint32_t cluster = (dir_table->table[index].cluster_high << 16) | dir_table->table[index].cluster_low;

    if (!cluster) {
        return -1; // Unknown error
    }

    struct ClusterBuffer cbuf[(request.buffer_size / CLUSTER_SIZE) + 1];
    uint32_t count = 0;

    do {
//        read_clusters((struct ClusterBuffer *)(request.buf)++, cluster, 1);
        read_clusters(&cbuf[count], cluster, 1);
        cluster = driver.fat_table.cluster_map[cluster];
        count++;
    } while (driver.fat_table.cluster_map[cluster] != FAT32_FAT_END_OF_FILE && cluster != FAT32_FAT_END_OF_FILE);

    memcpy(request.buf, &cbuf, request.buffer_size);

    return 0; // Success
}

int8_t write(struct FAT32DriverRequest request) {
    if (memcmp(request.name, empty_name, 8) == 0) {
        return -1; // No name, invalid file
    }

//    read_clusters(&driver.dir_table_buf, request.parent_cluster_number, 1);
    read_to_dir_table(request.parent_cluster_number, 1);

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
                struct FAT32DirectoryTable new_table;
                init_directory_entry(&new_table.table[0], request.name, usable_cluster);
                add_directory_entry(request, usable_cluster);
                flush(usable_cluster);
                return 0;
            }
            usable_cluster++;
        } while (TRUE);
    }
    // File
    uint32_t buffer_size = request.buffer_size;
    uint32_t head = 3;
    uint32_t tail = head;
    uint32_t count = 0;

    struct ClusterBuffer cbuf[buffer_size / CLUSTER_SIZE + 1];
    memset(cbuf, 0, sizeof(struct ClusterBuffer) * (buffer_size / CLUSTER_SIZE + 1));
    memcpy(cbuf, request.buf, sizeof(struct ClusterBuffer) * (buffer_size / CLUSTER_SIZE + 1));

    do {
        if (head == CLUSTER_MAP_SIZE) {
            return -1;
        }

        if (fat_table->cluster_map[head] == FAT32_FAT_EMPTY_ENTRY /*|| (fat_table->cluster_map[head] == FAT32_FAT_END_OF_FILE && fat_table->cluster_map[head + 1] == FAT32_FAT_EMPTY_ENTRY)*/) {
            if (memcmp(dir_table->table[empty_entry].name, empty_name, 8) == 0) {
                struct FAT32DirectoryEntry dir_entry;
                init_directory_entry(&dir_entry, request.name, head);
                memcpy(&dir_table->table[empty_entry], &dir_entry, sizeof(struct FAT32DirectoryEntry));
            }

//            write_clusters((struct ClusterBuffer *)(request.buf)++, tail, 1);
            write_clusters(&cbuf[count], tail, 1);
            count++;

            // if (fat_table->cluster_map[head] == FAT32_FAT_END_OF_FILE && fat_table->cluster_map[head + 1] == FAT32_FAT_EMPTY_ENTRY) {
            //     tail++;
            //     head = tail;
            // }

            buffer_size = buffer_size < CLUSTER_SIZE ? 0 : buffer_size - CLUSTER_SIZE;

            do {
                head++;
            } while (fat_table->cluster_map[head] != FAT32_FAT_EMPTY_ENTRY);

            fat_table->cluster_map[tail] = head;
            if (buffer_size == 0) {
                fat_table->cluster_map[head] = FAT32_FAT_END_OF_FILE;
            }
        } else {
            head++;
        }
        tail = head;
    } while (buffer_size > 0);

    for (uint8_t i = 0; i < 64; i++) {
        if ((uint32_t)(dir_table->table[i].cluster_high << 16 | dir_table->table[i].cluster_low) == request.parent_cluster_number) {
            dir_table->table[i].user_attribute = UATTR_NOT_EMPTY;
            break;
        }
    }

    flush(request.parent_cluster_number);

    return 0;
}

int8_t delete(struct FAT32DriverRequest request) {
    for (uint32_t cluster = request.parent_cluster_number; cluster < CLUSTER_MAP_SIZE; cluster++) {
        // This will check every EOF which mark doubles as a directory
        // (How the hell did I forget to check for files)
        if (fat_table->cluster_map[cluster] == FAT32_FAT_END_OF_FILE) {
            // Check if the directory is the one we are looking for
            read_clusters(&driver.dir_table_buf, cluster, 1);
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
        } else if (fat_table->cluster_map[cluster] != FAT32_FAT_EMPTY_ENTRY) {
            read_clusters(&driver.dir_table_buf, request.parent_cluster_number, 1);
            for (int i = 0; i < 64; i++) {
                if (memcmp(dir_table->table[i].name, request.name, 8) == 0 && memcmp(dir_table->table[i].ext, request.ext, 3) == 0) {
                    uint32_t file_cluster = (dir_table->table[i].cluster_high << 16) | dir_table->table[i].cluster_low;

                    do {
                        uint32_t next_cluster = fat_table->cluster_map[file_cluster];
                        fat_table->cluster_map[file_cluster] = FAT32_FAT_EMPTY_ENTRY;
                        file_cluster = next_cluster;
                    } while (fat_table->cluster_map[file_cluster] != FAT32_FAT_END_OF_FILE);

                    memset(&dir_table->table[i], 0, sizeof(struct FAT32DirectoryEntry));

                    //  write_clusters(0, file_cluster, 1); File will be overwritten in the next write operation anyway
                    flush(cluster);
                    return 0; // Success
                }
            }
        }
    }
    return 1;
}