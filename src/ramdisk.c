/*
 * This file is part of DeskHop (https://github.com/hrvach/deskhop).
 * Copyright (c) 2025 Hrvoje Cavrak
 * Based on the TinyUSB example by Ha Thach.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * See the file LICENSE for the full license text.
 */

#include "main.h"
#include "config_file.h"

#define NUMBER_OF_BLOCKS 4096
#define ACTUAL_NUMBER_OF_BLOCKS 128
#define BLOCK_SIZE       512

/* Buffer for incoming CONFIG.INI writes */
static uint8_t  config_file_buf[4096];
static bool     config_file_dirty;
static uint32_t config_file_lba_start;
static uint32_t config_file_lba_end;

/* Generated CONFIG.INI content for reads */
static char  config_generated[4096];
static bool  config_generated_valid;

/* FAT12 BPB field helpers */
static inline uint16_t read_u16le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static inline uint32_t read_u32le(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* Parse the FAT12 BPB from the disk image and find the LBA range for CONFIG.INI. */
static void find_config_ini_lba(void) {
    const uint8_t *boot = ADDR_DISK_IMAGE;

    uint16_t bytes_per_sector  = read_u16le(boot + 11);
    uint8_t  sectors_per_cluster = boot[13];
    uint16_t reserved_sectors  = read_u16le(boot + 14);
    uint8_t  num_fats          = boot[16];
    uint16_t root_entry_count  = read_u16le(boot + 17);
    uint16_t sectors_per_fat   = read_u16le(boot + 22);

    if (bytes_per_sector == 0 || sectors_per_cluster == 0)
        return;

    /* Compute layout sectors (in 512-byte units relative to disk image) */
    uint32_t fat_start     = reserved_sectors;
    uint32_t root_start    = fat_start + (uint32_t)num_fats * sectors_per_fat;
    uint32_t root_sectors  = ((uint32_t)root_entry_count * 32 + bytes_per_sector - 1) / bytes_per_sector;
    uint32_t data_start    = root_start + root_sectors;

    /* Scan root directory entries (32 bytes each) for CONFIG  INI */
    for (uint32_t i = 0; i < root_entry_count; i++) {
        uint32_t entry_lba    = root_start + (i * 32) / bytes_per_sector;
        uint32_t entry_offset = (i * 32) % bytes_per_sector;

        if (entry_lba >= ACTUAL_NUMBER_OF_BLOCKS)
            break;

        const uint8_t *entry = ADDR_DISK_IMAGE + entry_lba * bytes_per_sector + entry_offset;

        /* Empty slot or end of directory */
        if (entry[0] == 0x00)
            break;
        if (entry[0] == 0xE5)
            continue;
        /* Volume label or LFN */
        if (entry[11] & 0x08)
            continue;

        /* Compare 8.3 name: "CONFIG  INI" (8+3 = 11 chars, space-padded) */
        if (memcmp(entry, "CONFIG  INI", 11) == 0) {
            uint16_t start_cluster = read_u16le(entry + 26);
            uint32_t file_size     = read_u32le(entry + 28);

            /* Convert cluster to LBA */
            uint32_t first_lba = data_start + (uint32_t)(start_cluster - 2) * sectors_per_cluster;
            uint32_t num_sectors = (file_size + bytes_per_sector - 1) / bytes_per_sector;
            if (num_sectors == 0)
                num_sectors = sectors_per_cluster;

            config_file_lba_start = first_lba;
            config_file_lba_end   = first_lba + num_sectors;
            return;
        }
    }
}

void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {
    strcpy((char *)vendor_id, "DeskHop");
    strcpy((char *)product_id, "Config Mode");
    strcpy((char *)product_rev, "1.0");
}

bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    return true;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size) {
    *block_count = NUMBER_OF_BLOCKS;
    *block_size  = BLOCK_SIZE;
}

bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
    if (load_eject && config_file_dirty) {
        config_file_dirty = false;
        config_file_parse(&global_state, (char *)config_file_buf, sizeof(config_file_buf));
        save_config(&global_state);
        config_generated_valid = false;
    }
    return true;
}

/* Return the requested data, or -1 if out-of-bounds */
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize) {
    if (lba >= NUMBER_OF_BLOCKS)
        return -1;

    /* Initialise LBA range on first access */
    if (config_file_lba_start == 0 && config_file_lba_end == 0)
        find_config_ini_lba();

    /* Serve generated CONFIG.INI for sectors in the config file range */
    if (config_file_lba_start != config_file_lba_end &&
        lba >= config_file_lba_start && lba < config_file_lba_end) {

        if (!config_generated_valid) {
            memset(config_generated, 0, sizeof(config_generated));
            config_file_serialize(&global_state, config_generated, sizeof(config_generated));
            config_generated_valid = true;
        }

        uint32_t file_offset = (lba - config_file_lba_start) * BLOCK_SIZE + offset;
        uint32_t gen_len = (uint32_t)strlen(config_generated);

        if (file_offset < gen_len) {
            uint32_t avail = gen_len - file_offset;
            uint32_t copy_len = (avail < bufsize) ? avail : bufsize;
            memcpy(buffer, config_generated + file_offset, copy_len);
            if (copy_len < bufsize)
                memset((uint8_t *)buffer + copy_len, 0, bufsize - copy_len);
        } else {
            memset(buffer, 0, bufsize);
        }
        return (int32_t)bufsize;
    }

    /* We lie about the image size - actually it's 64 kB, not 512 kB, so if we're out of bounds, return zeros */
    if (lba >= ACTUAL_NUMBER_OF_BLOCKS)
        memset(buffer, 0x00, bufsize);
    else
        memcpy(buffer, &ADDR_DISK_IMAGE[lba * BLOCK_SIZE + offset], bufsize);

    return (int32_t)bufsize;
}

/* We're writable, so return true */
bool tud_msc_is_writable_cb(uint8_t lun) {
    return true;
}

/* Simple firmware write routine, we get 512-byte uf2 blocks with 256 byte payload */
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize) {
    if (lba >= NUMBER_OF_BLOCKS)
        return -1;

    uf2_t *uf2 = (uf2_t *)&buffer[0];

    /* Detect config file writes before UF2 check */
    if (config_file_lba_start != config_file_lba_end &&
        lba >= config_file_lba_start && lba < config_file_lba_end) {

        uint32_t file_offset = (lba - config_file_lba_start) * BLOCK_SIZE + offset;
        if (file_offset + bufsize <= sizeof(config_file_buf)) {
            memcpy(config_file_buf + file_offset, buffer, bufsize);
            config_file_dirty = true;
        }
        return (int32_t)bufsize;
    }

    /* If we're not detecting UF2 magic constants, we have nothing to do... */
    if (uf2->magicStart0 != UF2_MAGIC_START0 || uf2->magicStart1 != UF2_MAGIC_START1 || uf2->magicEnd != UF2_MAGIC_END)
        return (int32_t)bufsize;

    bool is_final_block = uf2->blockNo == (STAGING_IMAGE_SIZE / FLASH_PAGE_SIZE) - 1;
    uint32_t flash_addr = (uint32_t)ADDR_FW_RUNNING + uf2->blockNo * FLASH_PAGE_SIZE - XIP_BASE;

    if (uf2->blockNo == 0) {
        global_state.fw.checksum = 0xffffffff;

        /* Make sure nobody else touches the flash during this operation, otherwise we get empty pages */
        global_state.fw.upgrade_in_progress = true;
    }

    /* Update checksum continuously as blocks are being received */
    const uint32_t last_block_with_checksum = (STAGING_IMAGE_SIZE - FLASH_SECTOR_SIZE) / FLASH_PAGE_SIZE;
    for (int i=0; i<FLASH_PAGE_SIZE && uf2->blockNo < last_block_with_checksum; i++)
        global_state.fw.checksum = crc32_iter(global_state.fw.checksum, buffer[32 + i]);

    write_flash_page(flash_addr, &buffer[32]);

    if (is_final_block) {
        global_state.fw.checksum = ~global_state.fw.checksum;

        /* If checksums don't match, overwrite first sector and rely on ROM bootloader for recovery */
        if (global_state.fw.checksum != calculate_firmware_crc32()) {
            flash_range_erase((uint32_t)ADDR_FW_RUNNING - XIP_BASE, FLASH_SECTOR_SIZE);
            reset_usb_boot(1 << PICO_DEFAULT_LED_PIN, 0);
        }
        else {
            global_state.flash_source = FLASH_SOURCE_DIRECT;
            global_state.reboot_requested = true;
        }
    }

    /* Provide some visual indication that fw is being uploaded */
    toggle_led();
    watchdog_update();

    return (int32_t)bufsize;
}

/* This is a super-dumb, rudimentary disk, any other scsi command is simply rejected */
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize) {
    tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
    return -1;
}
