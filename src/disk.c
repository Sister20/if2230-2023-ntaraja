#include "lib-header/filesystem/disk.h"
#include "lib-header/portio.h"

//static void ATA_busy_wait() {
//    while (in(ATA_PRIMARY_STATUS) & ATA_STATUS_BSY);
//}
//
//static void ATA_DRQ_wait() {
//    while (!(in(ATA_PRIMARY_STATUS) & ATA_STATUS_DRQ));
//}

// 28 bit LBA PIO mode
void read_blocks(void *ptr, uint32_t logical_block_address, uint8_t block_count) {
//    ATA_busy_wait();
    out(ATA_PRIMARY_DRIVE, 0xE0 | ((logical_block_address >> 24) & 0x0F));
    out(ATA_PRIMARY_ERR, 0x00);
    out(ATA_PRIMARY_SECT_CNT, block_count);
    out(ATA_PRIMARY_LBA_LO, (uint8_t) logical_block_address);
    out(ATA_PRIMARY_LBA_MID, (uint8_t) (logical_block_address >> 8));
    out(ATA_PRIMARY_LBA_HI, (uint8_t) (logical_block_address >> 16));
    out(ATA_PRIMARY_STATUS, 0x20);


    for (uint32_t i = 0; i < block_count; i++) {
//        ATA_busy_wait();
//        ATA_DRQ_wait();
        while (1) {
            if (in(ATA_PRIMARY_STATUS) & ATA_STATUS_DRQ) {
                break;
            }
        }
        insw(ATA_PRIMARY_IO, ptr, HALF_BLOCK_SIZE);
        ptr += HALF_BLOCK_SIZE;
    }
}

void write_blocks(const void *ptr, uint32_t logical_block_address, uint8_t block_count) {
//    ATA_busy_wait();
    out(ATA_PRIMARY_DRIVE, 0xE0 | ((logical_block_address >> 24) & 0x0F));
    out(ATA_PRIMARY_SECT_CNT, block_count);
    out(ATA_PRIMARY_LBA_LO, (uint8_t) logical_block_address);
    out(ATA_PRIMARY_LBA_MID, (uint8_t) (logical_block_address >> 8));
    out(ATA_PRIMARY_LBA_HI, (uint8_t) (logical_block_address >> 16));
    out(ATA_PRIMARY_STATUS, 0x30);

    for (uint32_t i = 0; i < block_count; i++) {
//        ATA_busy_wait();
//        ATA_DRQ_wait();
//        for (uint32_t j = 0; j < HALF_BLOCK_SIZE; j++) {
//            out(ATA_PRIMARY_IO, ((uint16_t*) ptr)[i * HALF_BLOCK_SIZE + j]);
//        }
        while (1) {
            if (in(ATA_PRIMARY_STATUS) & ATA_STATUS_DRQ) {
                break;
            } else if (in(ATA_PRIMARY_STATUS) & ATA_STATUS_ERR) {
                return;
            }
        }
        outsw(ATA_PRIMARY_IO, ptr, HALF_BLOCK_SIZE);
        ptr += HALF_BLOCK_SIZE;
    }

    out(ATA_PRIMARY_STATUS, 0xE7);
    while(in(ATA_PRIMARY_STATUS) & ATA_STATUS_BSY);
}