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
#pragma once

#include "main.h"
#include "hid_parser.h"

#if defined(DH_DEBUG) && defined(DH_DEBUG_HID_DUMP)

/* Print descriptor header and hex dump */
void print_descriptor_header(uint8_t const *report, int desc_len, uint8_t board_role, uint8_t dev_addr, uint8_t instance, hid_interface_t *iface);

/* Print extracted descriptor mapping */
void debug_print_extracted_mapping(const char *name, const report_val_t *val);

/* Dump HID report data with throttling (max 1/sec per interface+report_id) */
void debug_dump_hid_report(uint8_t const *report, uint16_t len, uint8_t dev_addr,
                           uint8_t instance, hid_interface_t *iface,
                           mouse_values_t *mouse_vals, hid_keyboard_report_t *kbd_report);

#else

/* Stub versions when both DH_DEBUG and DH_DEBUG_HID_DUMP are not enabled */
static inline void print_descriptor_header(uint8_t const *report, int desc_len, uint8_t board_role, uint8_t dev_addr, uint8_t instance, hid_interface_t *iface) {
    (void)report; (void)desc_len; (void)board_role; (void)dev_addr; (void)instance; (void)iface;
}

static inline void debug_print_extracted_mapping(const char *name, const report_val_t *val) {
    (void)name; (void)val;
}

static inline void debug_dump_hid_report(uint8_t const *report, uint16_t len, uint8_t dev_addr,
                                         uint8_t instance, hid_interface_t *iface,
                                         mouse_values_t *mouse_vals, hid_keyboard_report_t *kbd_report) {
    (void)report; (void)len; (void)dev_addr; (void)instance; (void)iface; (void)mouse_vals; (void)kbd_report;
}

#endif
