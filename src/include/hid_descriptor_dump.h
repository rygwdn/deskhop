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

#ifdef DH_DEBUG_HID_DUMP

/* Print descriptor header and hex dump (called at start of parsing) */
void print_descriptor_header(uint8_t const *report, int desc_len, uint8_t board_role, uint8_t dev_addr, uint8_t instance, hid_interface_t *iface);

/* Print footer for descriptor (called after parsing) */
void print_descriptor_footer(void);

/* Reset state (called at start of parsing) */
void reset_descriptor_state(void);

/* Print an HID item immediately (called during parsing) */
void print_hid_item(item_t *item, parser_state_t *parser);

/* Print HID report element information in a formatted table */
void debug_print_report_element(report_val_t *val);

/* Pretty-print extracted descriptor mapping from hid_report.c */
void debug_print_extracted_mapping(const char *name, const report_val_t *val);

/* Dump HID report data with throttling (max 1/sec per interface+report_id) */
void debug_dump_hid_report(uint8_t const *report, uint16_t len, uint8_t dev_addr,
                           uint8_t instance, hid_interface_t *iface,
                           mouse_values_t *mouse_vals, hid_keyboard_report_t *kbd_report);

/* Get current indent level */
int get_indent_level(void);

/* Set indent level */
void set_indent_level(int level);

/* Increment indent level */
void increment_indent(void);

/* Decrement indent level */
void decrement_indent(void);

#else

/* Dummy versions when DH_DEBUG is not enabled */
static inline void print_descriptor_header(uint8_t const *report, int desc_len, uint8_t board_role, uint8_t dev_addr, uint8_t instance, hid_interface_t *iface) {
    (void)report;
    (void)desc_len;
    (void)board_role;
    (void)dev_addr;
    (void)instance;
    (void)iface;
}

static inline void print_descriptor_footer(void) {}

static inline void reset_descriptor_state(void) {}

static inline void print_hid_item(item_t *item, parser_state_t *parser) {
    (void)item;
    (void)parser;
}

static inline void debug_print_report_element(report_val_t *val) {
    (void)val;
}

static inline void debug_print_extracted_mapping(const char *name, const report_val_t *val) {
    (void)name;
    (void)val;
}

static inline void debug_dump_hid_report(uint8_t const *report, uint16_t len, uint8_t dev_addr,
                                         uint8_t instance, hid_interface_t *iface,
                                         mouse_values_t *mouse_vals, hid_keyboard_report_t *kbd_report) {
    (void)report;
    (void)len;
    (void)dev_addr;
    (void)instance;
    (void)iface;
    (void)mouse_vals;
    (void)kbd_report;
}

static inline int get_indent_level(void) {
    return 0;
}

static inline void set_indent_level(int level) {
    (void)level;
}

static inline void increment_indent(void) {}

static inline void decrement_indent(void) {}

#endif
