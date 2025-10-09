/*
 * This file is part of DeskHop (https://github.com/hrvach/deskhop).
 * Copyright (c) 2025 Hrvoje Cavrak
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * See the file LICENSE for the full license text.
 */

#include "main.h"

/* ================================================== *
 * ==============  Checksum Functions  ============== *
 * ================================================== */

uint8_t calc_checksum(const uint8_t *data, int length) {
    uint8_t checksum = 0;

    for (int i = 0; i < length; i++) {
        checksum ^= data[i];
    }

    return checksum;
}

bool verify_checksum(const uart_packet_t *packet) {
    uint8_t checksum = calc_checksum(packet->data, PACKET_DATA_LENGTH);
    return checksum == packet->checksum;
}

uint32_t crc32_iter(uint32_t crc, const uint8_t byte) {
    return crc32_lookup_table[(byte ^ crc) & 0xff] ^ (crc >> 8);
}

/* TODO - use DMA sniffer's built-in CRC32 */
uint32_t calc_crc32(const uint8_t *s, size_t n) {
    uint32_t crc = 0xffffffff;

    for(size_t i=0; i < n; i++) {
        crc = crc32_iter(crc, s[i]);
    }

    return ~crc;
}

uint32_t calculate_firmware_crc32(void) {
    return calc_crc32(ADDR_FW_RUNNING, STAGING_IMAGE_SIZE - FLASH_SECTOR_SIZE);
}

/* ================================================== *
 * Flash and config functions
 * ================================================== */

void wipe_config(void) {
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase((uint32_t)ADDR_CONFIG - XIP_BASE, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);
}

void write_flash_page(uint32_t target_addr, uint8_t *buffer) {
    /* Start of sector == first 256-byte page in a 4096 byte block */
    bool is_sector_start = (target_addr & 0xf00) == 0;

    uint32_t ints = save_and_disable_interrupts();
    if (is_sector_start)
        flash_range_erase(target_addr, FLASH_SECTOR_SIZE);

    flash_range_program(target_addr, buffer, FLASH_PAGE_SIZE);
    restore_interrupts(ints);
}

/*
 * Flash config format (fits in one 4KB sector):
 *   [4] magic = 0xB00B1E5
 *   [4] format_version = CURRENT_CONFIG_VERSION
 *   repeated: [2] field_idx  [1] len  [len] value
 *   [2] sentinel 0x0000
 *   [4] CRC32 of all bytes above
 */
#define CONFIG_MAGIC        0xB00B1E5
#define CONFIG_HDR_SIZE     8   /* magic(4) + version(4) */
#define CONFIG_TLV_HDR_SIZE 3   /* idx(2) + len(1) */
#define CONFIG_FOOTER_SIZE  6   /* sentinel(2) + crc32(4) */

void load_config(device_t *state) {
    const uint8_t *flash = (const uint8_t *)ADDR_CONFIG;

    /* Start from defaults */
    memcpy(&state->config, &default_config, sizeof(config_t));

    /* Verify magic */
    uint32_t magic;
    memcpy(&magic, flash, 4);
    if (magic != CONFIG_MAGIC)
        return;

    /* Find sentinel to determine payload length, then verify CRC32 */
    const uint8_t *p = flash + CONFIG_HDR_SIZE;
    const uint8_t *flash_end = flash + FLASH_SECTOR_SIZE - CONFIG_FOOTER_SIZE;
    while (p < flash_end) {
        uint16_t idx;
        memcpy(&idx, p, 2);
        if (idx == 0)
            break;
        if (p + CONFIG_TLV_HDR_SIZE > flash_end)
            break;
        p += CONFIG_TLV_HDR_SIZE + p[2];
    }
    /* p now points at sentinel; payload = everything up to and including sentinel */
    size_t payload_len = (size_t)(p - flash) + 2; /* +2 for sentinel */
    uint32_t stored_crc;
    memcpy(&stored_crc, flash + payload_len, 4);
    if (calc_crc32(flash, payload_len) != stored_crc)
        return;

    /* Apply TLV fields */
    p = flash + CONFIG_HDR_SIZE;
    while (p < flash_end) {
        uint16_t idx;
        memcpy(&idx, p, 2);
        if (idx == 0)
            break;
        uint8_t len = p[2];
        const field_map_t *f = get_field_map_entry(idx);
        if (f && !f->readonly && len == f->len) {
            uint64_t val = 0;
            memcpy(&val, p + CONFIG_TLV_HDR_SIZE, len);
            field_write(state, f, val);
        }
        p += CONFIG_TLV_HDR_SIZE + len;
    }
}

/* Static 4KB buffer for full-sector config writes */
static uint8_t config_sector_buffer[FLASH_SECTOR_SIZE];

void save_config(device_t *state) {
    memset(config_sector_buffer, 0, sizeof(config_sector_buffer));
    uint8_t *buf = config_sector_buffer;
    size_t pos = 0;

    /* Header: magic + version */
    uint32_t magic = CONFIG_MAGIC;
    uint32_t version = CURRENT_CONFIG_VERSION;
    memcpy(buf + pos, &magic, 4);   pos += 4;
    memcpy(buf + pos, &version, 4); pos += 4;

    /* TLV records for every writable field */
    for (size_t i = 0; i < get_field_map_length(); i++) {
        const field_map_t *f = get_field_map_index(i);
        if (f->readonly)
            continue;
        if (pos + CONFIG_TLV_HDR_SIZE + f->len > FLASH_SECTOR_SIZE - CONFIG_FOOTER_SIZE)
            break;
        uint64_t val = field_read(state, f);
        memcpy(buf + pos, &f->idx, 2); pos += 2;
        buf[pos++] = (uint8_t)f->len;
        memcpy(buf + pos, &val, f->len); pos += f->len;
    }

    /* Sentinel */
    buf[pos++] = 0;
    buf[pos++] = 0;

    /* CRC32 over everything written so far */
    uint32_t crc = calc_crc32(buf, pos);
    memcpy(buf + pos, &crc, 4); pos += 4;

    /* Erase sector and write all pages */
    uint32_t target_addr = (uint32_t)ADDR_CONFIG - XIP_BASE;
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(target_addr, FLASH_SECTOR_SIZE);
    flash_range_program(target_addr, config_sector_buffer, FLASH_SECTOR_SIZE);
    restore_interrupts(ints);

    /* Sync config to peer unless we are already receiving a sync from them */
    if (!state->config_sync_in_progress) {
        for (size_t i = 0; i < get_field_map_length(); i++) {
            const field_map_t *f = get_field_map_index(i);
            if (f->readonly)
                continue;
            uint64_t val = field_read(state, f);
            uint8_t pkt[PACKET_DATA_LENGTH] = {0};
            pkt[0] = (uint8_t)f->idx;
            memcpy(&pkt[1], &val, f->len);
            queue_packet(pkt, SET_VAL_MSG, PACKET_DATA_LENGTH);
        }
        send_value(0, SAVE_CONFIG_MSG);
    }
}

/* Write a magic marker to the last page of FLASH_CONFIG so the next boot
   knows we were directly flashed and should sync the peer. */
void write_direct_flash_marker(void) {
    uint8_t page[FLASH_PAGE_SIZE];
    memset(page, 0, sizeof(page));
    uint32_t magic = DIRECT_FLASH_MARKER_MAGIC;
    memcpy(page, &magic, sizeof(magic));

    uint32_t marker_addr = (uint32_t)ADDR_CONFIG - XIP_BASE + DIRECT_FLASH_MARKER_OFFSET;
    uint32_t ints = save_and_disable_interrupts();
    flash_range_program(marker_addr, page, FLASH_PAGE_SIZE);
    restore_interrupts(ints);
}

/* Check for the direct-flash marker. If found, set flash_source and erase it
   by saving config (which erases the whole sector, wiping the marker). */
bool check_and_clear_direct_flash_marker(device_t *state) {
    const uint8_t *marker_page = (const uint8_t *)ADDR_CONFIG + DIRECT_FLASH_MARKER_OFFSET;
    uint32_t magic;
    memcpy(&magic, marker_page, sizeof(magic));

    if (magic != DIRECT_FLASH_MARKER_MAGIC)
        return false;

    /* Clear the marker by saving config (erases sector, rewrites config without marker) */
    save_config(state);
    return true;
}

void reset_config_timer(device_t *state) {
    /* Once this is reached, we leave the config mode */
    state->config_mode_timer = time_us_64() + CONFIG_MODE_TIMEOUT;
}

void _configure_flash_cs(enum gpio_override gpo, uint pin_index) {
  hw_write_masked(&ioqspi_hw->io[pin_index].ctrl,
                  gpo << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                  IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);
}

bool is_bootsel_pressed(void) {
  const uint CS_PIN_INDEX = 1;
  uint32_t flags = save_and_disable_interrupts();

  /* Set chip select to high impedance */
  _configure_flash_cs(GPIO_OVERRIDE_LOW, CS_PIN_INDEX);
  sleep_us(20);

  /* Button pressed pulls pin DOWN, so invert */
  bool button_pressed = !(sio_hw->gpio_hi_in & (1u << CS_PIN_INDEX));

  /* Restore chip select state */
  _configure_flash_cs(GPIO_OVERRIDE_NORMAL, CS_PIN_INDEX);
  restore_interrupts(flags);

  return button_pressed;
}

void request_byte(device_t *state, uint32_t address) {
    uart_packet_t packet = {
        .data32[0] = address,
        .type = REQUEST_BYTE_MSG,
    };
    state->fw.byte_done = false;

    queue_try_add(&global_state.uart_tx_queue, &packet);
}

void reboot(void) {
    *((volatile uint32_t*)(PPB_BASE + 0x0ED0C)) = 0x5FA0004;
}

bool is_start_of_packet(device_t *state) {
    return (uart_rxbuf[state->dma_ptr] == START1 && uart_rxbuf[NEXT_RING_IDX(state->dma_ptr)] == START2);
}

uint32_t get_ptr_delta(uint32_t current_pointer, device_t *state) {
    uint32_t delta;

    if (current_pointer >= state->dma_ptr)
        delta = current_pointer - state->dma_ptr;
    else
        delta = DMA_RX_BUFFER_SIZE - state->dma_ptr + current_pointer;

    /* Clamp to 12 bits since it can never be bigger */
    delta = delta & 0x3FF;

    return delta;
}

void fetch_packet(device_t *state) {
    uint8_t *dst = (uint8_t *)&state->in_packet;

    for (int i = 0; i < RAW_PACKET_LENGTH; i++) {
        /* Skip the header preamble */
        if (i >= START_LENGTH)
            dst[i - START_LENGTH] = uart_rxbuf[state->dma_ptr];

        state->dma_ptr = NEXT_RING_IDX(state->dma_ptr);
    }
}

/* Validating any input is mandatory. Only packets of these type are allowed
   to be sent to the device over configuration endpoint. */
bool validate_packet(uart_packet_t *packet) {
    const enum packet_type_e ALLOWED_PACKETS[] = {
        FLASH_LED_MSG,
        GET_VAL_MSG,
        GET_ALL_VALS_MSG,
        SET_VAL_MSG,
        WIPE_CONFIG_MSG,
        SAVE_CONFIG_MSG,
        REBOOT_MSG,
        PROXY_PACKET_MSG,
    };
    uint8_t packet_type = packet->type;

    /* Proxied packets are encapsulated in the data field, but same rules apply */
    if (packet->type == PROXY_PACKET_MSG)
        packet_type = packet->data[0];

    for (int i = 0; i < ARRAY_SIZE(ALLOWED_PACKETS); i++) {
        if (ALLOWED_PACKETS[i] == packet_type)
            return true;
    }
    return false;
}


/* ================================================== *
 * Debug functions
 * ================================================== */
#ifdef DH_DEBUG

// Based on: https://github.com/raspberrypi/pico-sdk/blob/a1438dff1d38bd9c65dbd693f0e5db4b9ae91779/src/rp2_common/pico_stdio_usb/stdio_usb.c#L100-L130
static void cdc_write_str(const char *str) {
    int str_len = strlen(str);

    if (!tud_cdc_connected())
        return;

    uint64_t last_write_time = time_us_64();

    for (int bytes_written = 0; bytes_written < str_len;) {
        int bytes_remaining = str_len - bytes_written;
        int available_space = (int)tud_cdc_write_available();
        int chunk_size      = (bytes_remaining < available_space) ? bytes_remaining : available_space;

        if (chunk_size > 0) {
            int written = (int)tud_cdc_write(str + bytes_written, (uint32_t)chunk_size);
            tud_task();
            tud_cdc_write_flush();

            bytes_written += written;
            last_write_time = time_us_64();
        } else {
            tud_task();
            tud_cdc_write_flush();

            /* Timeout after 1ms if buffer stays full or connection lost */
            if (!tud_cdc_connected() || (time_us_64() > last_write_time + 1000))
                break;
        }
    }
}


int dh_debug_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    char buffer[512];

    int string_len = vsnprintf(buffer, 512, format, args);

    cdc_write_str(buffer);
    tud_cdc_write_flush();

    va_end(args);
    return string_len;
}
#else

int dh_debug_printf(const char *format, ...) {
    return 0;
}

#endif
