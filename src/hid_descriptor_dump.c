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
#include "hid_parser.h"
#include "hid_descriptor_dump.h"

#if defined(DH_DEBUG) && defined(DH_DEBUG_HID_DUMP)

#define MAX_REPORT_THROTTLE_ENTRIES 16
typedef struct {
    uint8_t report_id;
    uint64_t last_dump_time;
    uint32_t skipped_count;
} report_throttle_t;

static report_throttle_t report_throttle[MAX_REPORT_THROTTLE_ENTRIES] = {0};
static uint8_t throttle_entries_used = 0;

static void print_hex_dump(uint8_t const *data, int len) {
    for (int i = 0; i < len; i++) {
        if (i > 0 && (i % 16) == 0) {
            dh_debug_printf("\n");
        } else if (i > 0) {
            dh_debug_printf(" ");
        }
        dh_debug_printf("%02X", data[i]);
    }
    dh_debug_printf("\n");
}

static void print_header(char *name, hid_interface_t *iface) {
    uint64_t current_time = time_us_64();
    dh_debug_printf("\n%03u:%04X:%04X:%s             %llu.%06llu\n",
                   BOARD_ROLE, iface->vid, iface->pid, name, current_time / 1000000, current_time % 1000000);
}

void debug_print_descriptor_raw(uint8_t const *report, int desc_len, hid_interface_t *iface) {
    if (report == NULL || desc_len <= 0 || iface == NULL) {
        return;
    }

    const int MAX_DESC_LEN = 2048;
    if (desc_len > MAX_DESC_LEN) {
        desc_len = MAX_DESC_LEN;
    }

    print_header("DESCRIPTOR", iface);
    print_hex_dump(report, desc_len);
    dh_debug_printf("\n");
}

void debug_print_descriptor_parsed(const char *name, const report_val_t *val) {
    if (name == NULL || val == NULL) {
        return;
    }

    dh_debug_printf("Extracted mapping: %s\n", name);
    dh_debug_printf("  report_id   : %d\n", val->report_id);
    dh_debug_printf("  offset      : %d\n", val->offset);
    dh_debug_printf("  offset_idx  : %d\n", val->offset_idx);
    dh_debug_printf("  size        : %d\n", val->size);
    dh_debug_printf("  item_type   : %d\n", val->item_type);
    dh_debug_printf("  data_type   : %d\n", val->data_type);
    dh_debug_printf("  usage       : %d\n", val->usage);
    dh_debug_printf("  usage_min   : %d\n", val->usage_min);
    dh_debug_printf("  usage_max   : %d\n", val->usage_max);
    dh_debug_printf("  global_usage: %d\n", val->global_usage);
    dh_debug_printf("  usage_page  : %d\n", val->usage_page);
}

void debug_dump_hid_report(uint8_t const *report, uint16_t len, hid_interface_t *iface,
                           mouse_values_t *mouse_vals, hid_keyboard_report_t *kbd_report) {
    if (!tud_cdc_connected() || report == NULL || iface == NULL || len == 0)
        return;

    uint8_t report_id = 0;
    if (iface->uses_report_id && len > 0) {
        report_id = report[0];
    }

    uint64_t current_time = time_us_64();
    report_throttle_t *entry = NULL;

    for (int i = 0; i < throttle_entries_used; i++) {
        if (report_throttle[i].report_id == report_id) {
            entry = &report_throttle[i];
            break;
        }
    }

    if (entry == NULL && throttle_entries_used < MAX_REPORT_THROTTLE_ENTRIES) {
        entry = &report_throttle[throttle_entries_used++];
        entry->report_id = report_id;
        entry->last_dump_time = 0;
        entry->skipped_count = 0;
    }

    if (entry != NULL) {
        if (current_time - entry->last_dump_time < 1000000) {
            entry->skipped_count++;
            return;
        }
    }

    print_header("STREAM", iface);
    print_hex_dump(report, len);

    if (entry != NULL && entry->skipped_count > 0) {
        dh_debug_printf("(Skipped %u reports)\n", entry->skipped_count);
        entry->skipped_count = 0;
    }

    if (entry != NULL) {
        entry->last_dump_time = current_time;
    }

    if (mouse_vals != NULL) {
        dh_debug_printf("  Mouse: X=%d Y=%d Wheel=%d Pan=%d Buttons=0x%X\n",
                       mouse_vals->move_x, mouse_vals->move_y, mouse_vals->wheel,
                       mouse_vals->pan, mouse_vals->buttons);
    }

    if (kbd_report != NULL) {
        dh_debug_printf("  Keyboard: Mods=0x%02X Keys=[", kbd_report->modifier);
        for (int i = 0; i < 6; i++) {
            if (i > 0) dh_debug_printf(" ");
            dh_debug_printf("%02X", kbd_report->keycode[i]);
        }
        dh_debug_printf("]\n");
    }

    dh_debug_printf("\n");
}

#endif
