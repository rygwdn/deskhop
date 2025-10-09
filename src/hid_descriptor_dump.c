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

#ifdef DH_DEBUG

static int indent_level = 0;

#define MAX_REPORT_THROTTLE_ENTRIES 16
typedef struct {
    uint8_t dev_addr;
    uint8_t instance;
    uint8_t report_id;
    uint64_t last_dump_time;
    uint32_t skipped_count;
} report_throttle_t;

static report_throttle_t report_throttle[MAX_REPORT_THROTTLE_ENTRIES] = {0};
static uint8_t throttle_entries_used = 0;

/* Indent level management functions */
int get_indent_level(void) {
    return indent_level;
}

void set_indent_level(int level) {
    indent_level = (level < 0) ? 0 : level;
}

void increment_indent(void) {
    indent_level++;
}

void decrement_indent(void) {
    if (indent_level > 0) {
        indent_level--;
    }
}

/* Reset state */
void reset_descriptor_state(void) {
    indent_level = 0;
}

/* Convert item_type enum to string */
static inline const char* item_type_to_str(uint8_t item_type) {
    return (item_type == DATA) ? "DATA" : "CONST";
}

/* Convert data_type enum to string */
static inline const char* data_type_to_str(uint8_t data_type) {
    switch (data_type) {
        case ARRAY: return "ARRAY";
        case VARIABLE: return "VAR";
        default: return "UNK";
    }
}

/* Convert usage_page to string */
static inline const char* usage_page_to_str(uint16_t page) {
    switch (page) {
        case HID_USAGE_PAGE_DESKTOP: return "DESKTOP";
        case HID_USAGE_PAGE_SPORT: return "SPORT";
        case HID_USAGE_PAGE_GAME: return "GAME";
        case HID_USAGE_PAGE_KEYBOARD: return "KEYBOARD";
        case HID_USAGE_PAGE_LED: return "LED";
        case HID_USAGE_PAGE_BUTTON: return "BUTTON";
        case HID_USAGE_PAGE_CONSUMER: return "CONSUMER";
        case HID_USAGE_PAGE_VENDOR: return "VENDOR";
        default: return "OTHER";
    }
}

/* Get detailed usage page name */
static const char* get_usage_page_name(uint16_t page) {
    switch (page) {
        case HID_USAGE_PAGE_DESKTOP: return "Desktop";
        case HID_USAGE_PAGE_GAME: return "Game";
        case HID_USAGE_PAGE_KEYBOARD: return "Keyboard";
        case HID_USAGE_PAGE_LED: return "LED";
        case HID_USAGE_PAGE_BUTTON: return "Button";
        case HID_USAGE_PAGE_CONSUMER: return "Consumer";
        case HID_USAGE_PAGE_VENDOR: return "Vendor";
        default: return "Unknown";
    }
}

/* Get usage name for desktop page */
static const char* get_desktop_usage_name(uint16_t usage) {
    switch (usage) {
        case HID_USAGE_DESKTOP_POINTER: return "Pointer";
        case HID_USAGE_DESKTOP_MOUSE: return "Mouse";
        case HID_USAGE_DESKTOP_JOYSTICK: return "Joystick";
        case HID_USAGE_DESKTOP_GAMEPAD: return "Gamepad";
        case HID_USAGE_DESKTOP_KEYBOARD: return "Keyboard";
        case HID_USAGE_DESKTOP_KEYPAD: return "Keypad";
        case HID_USAGE_DESKTOP_X: return "X";
        case HID_USAGE_DESKTOP_Y: return "Y";
        case HID_USAGE_DESKTOP_Z: return "Z";
        case HID_USAGE_DESKTOP_RX: return "Rx";
        case HID_USAGE_DESKTOP_RY: return "Ry";
        case HID_USAGE_DESKTOP_RZ: return "Rz";
        case HID_USAGE_DESKTOP_SLIDER: return "Slider";
        case HID_USAGE_DESKTOP_DIAL: return "Dial";
        case HID_USAGE_DESKTOP_WHEEL: return "Wheel";
        case HID_USAGE_DESKTOP_HAT_SWITCH: return "Hat Switch";
        case 0x81: return "System Control";
        default: return NULL;
    }
}

/* Get usage name for consumer page */
static const char* get_consumer_usage_name(uint16_t usage) {
    switch (usage) {
        case HID_USAGE_CONSUMER_CONTROL: return "Consumer Control";
        case HID_USAGE_CONSUMER_AC_PAN: return "AC Pan";
        case 0x0B5: return "Scan Next Track";
        case 0x0B6: return "Scan Previous Track";
        case 0x0B7: return "Stop";
        case 0x0CD: return "Play/Pause";
        case 0x0E2: return "Mute";
        case 0x0E9: return "Volume Increment";
        case 0x0EA: return "Volume Decrement";
        default: return NULL;
    }
}

/* Get usage description based on page */
static const char* get_usage_name(uint16_t page, uint16_t usage) {
    switch (page) {
        case HID_USAGE_PAGE_DESKTOP:
            return get_desktop_usage_name(usage);
        case HID_USAGE_PAGE_CONSUMER:
            return get_consumer_usage_name(usage);
        case HID_USAGE_PAGE_BUTTON:
            // Buttons are typically numbered
            return NULL;
        default:
            return NULL;
    }
}

/* Get collection type name */
static const char* get_collection_type(uint32_t type) {
    switch (type) {
        case 0x00: return "Physical";
        case 0x01: return "Application";
        case 0x02: return "Logical";
        case 0x03: return "Report";
        case 0x04: return "Named Array";
        case 0x05: return "Usage Switch";
        case 0x06: return "Usage Modifier";
        default: return "Unknown";
    }
}

/* Print indentation with specific level */
static void print_indent_level(int level) {
    if (level < 0) {
        level = 0;
    }
    for (int i = 0; i < level; i++) {
        dh_debug_printf("    ");
    }
}

/* Get current timestamp in seconds since epoch (approximation for embedded system) */
static uint64_t get_timestamp_seconds(void) {
    // Use microseconds since boot divided by 1000000 to get approximate seconds
    // Note: This won't be actual epoch time, but matches the format
    return time_us_64() / 1000000;
}

/* Print single HID item in human-readable format - DIRECTLY to output
 * Output format similar to: hidrd-convert -o spec
 * Example: Usage Page (Desktop), ; Desktop (0x01h)
 *          Usage (Mouse), ; Mouse (0x02h)
 *          Collection (Application),
 */
static void print_single_hid_item(item_t *item, parser_state_t *parser, int indent) {
    if (item == NULL || parser == NULL) {
        return;
    }
    
    print_indent_level(indent);
    
    switch (item->hdr.type) {
        case RI_TYPE_MAIN:
            switch (item->hdr.tag) {
                case RI_MAIN_INPUT: {
                    dh_debug_printf("Input (");
                    if (item->val & 0x01) dh_debug_printf("Constant");
                    else dh_debug_printf("Data");
                    if (item->val & 0x02) dh_debug_printf(",Variable");
                    else dh_debug_printf(",Array");
                    if (item->val & 0x04) dh_debug_printf(",Relative");
                    else dh_debug_printf(",Absolute");
                    dh_debug_printf("),\n");
                    break;
                }
                case RI_MAIN_OUTPUT: {
                    dh_debug_printf("Output (");
                    if (item->val & 0x01) dh_debug_printf("Constant");
                    else dh_debug_printf("Data");
                    if (item->val & 0x02) dh_debug_printf(",Variable");
                    else dh_debug_printf(",Array");
                    dh_debug_printf("),\n");
                    break;
                }
                case RI_MAIN_COLLECTION: {
                    dh_debug_printf("Collection (%s),\n", get_collection_type(item->val));
                    break;
                }
                case RI_MAIN_COLLECTION_END:
                    dh_debug_printf("End Collection,\n");
                    break;
                case RI_MAIN_FEATURE: {
                    dh_debug_printf("Feature (");
                    if (item->val & 0x01) dh_debug_printf("Constant");
                    else dh_debug_printf("Data");
                    if (item->val & 0x02) dh_debug_printf(",Variable");
                    else dh_debug_printf(",Array");
                    dh_debug_printf("),\n");
                    break;
                }
            }
            break;
            
        case RI_TYPE_GLOBAL:
            switch (item->hdr.tag) {
                case RI_GLOBAL_USAGE_PAGE: {
                    uint16_t page = item->val & 0xFFFF;  // Bounds check
                    dh_debug_printf("Usage Page (%s), ; %s (0x%02Xh)\n", 
                              get_usage_page_name(page), get_usage_page_name(page), page);
                    break;
                }
                case RI_GLOBAL_LOGICAL_MIN:
                    dh_debug_printf("Logical Minimum (%d),\n", (int32_t)item->val);
                    break;
                case RI_GLOBAL_LOGICAL_MAX:
                    dh_debug_printf("Logical Maximum (%d),\n", (int32_t)item->val);
                    break;
                case RI_GLOBAL_PHYSICAL_MIN:
                    dh_debug_printf("Physical Minimum (%d),\n", (int32_t)item->val);
                    break;
                case RI_GLOBAL_PHYSICAL_MAX:
                    dh_debug_printf("Physical Maximum (%d),\n", (int32_t)item->val);
                    break;
                case RI_GLOBAL_REPORT_SIZE:
                    dh_debug_printf("Report Size (%u),\n", item->val);
                    break;
                case RI_GLOBAL_REPORT_ID:
                    dh_debug_printf("Report ID (%u),\n", item->val);
                    break;
                case RI_GLOBAL_REPORT_COUNT:
                    dh_debug_printf("Report Count (%u),\n", item->val);
                    break;
                case RI_GLOBAL_UNIT_EXPONENT:
                    dh_debug_printf("Unit Exponent (%d),\n", (int32_t)item->val);
                    break;
                case RI_GLOBAL_UNIT:
                    dh_debug_printf("Unit (0x%02X),\n", item->val);
                    break;
                case RI_GLOBAL_PUSH:
                    dh_debug_printf("Push,\n");
                    break;
                case RI_GLOBAL_POP:
                    dh_debug_printf("Pop,\n");
                    break;
            }
            break;
            
        case RI_TYPE_LOCAL:
            switch (item->hdr.tag) {
                case RI_LOCAL_USAGE: {
                    uint16_t usage = item->val & 0xFFFF;  // Bounds check
                    uint16_t page = parser->globals[RI_GLOBAL_USAGE_PAGE].val & 0xFFFF;  // Bounds check
                    const char* usage_name = get_usage_name(page, usage);
                    
                    if (usage_name) {
                        dh_debug_printf("Usage (%s), ; %s (0x%02Xh)\n", usage_name, usage_name, usage);
                    } else if (page == HID_USAGE_PAGE_BUTTON) {
                        // Special case for buttons - show as Button(n)
                        dh_debug_printf("Usage (Button %u), ; Button %u (0x%02Xh)\n", usage, usage, usage);
                    } else {
                        dh_debug_printf("Usage (0x%04X),\n", usage);
                    }
                    break;
                }
                case RI_LOCAL_USAGE_MIN:
                    dh_debug_printf("Usage Minimum (0x%04X),\n", item->val & 0xFFFF);
                    break;
                case RI_LOCAL_USAGE_MAX:
                    dh_debug_printf("Usage Maximum (0x%04X),\n", item->val & 0xFFFF);
                    break;
                case RI_LOCAL_DESIGNATOR_INDEX:
                    dh_debug_printf("Designator Index (%u),\n", item->val);
                    break;
                case RI_LOCAL_DESIGNATOR_MIN:
                    dh_debug_printf("Designator Minimum (%u),\n", item->val);
                    break;
                case RI_LOCAL_DESIGNATOR_MAX:
                    dh_debug_printf("Designator Maximum (%u),\n", item->val);
                    break;
                case RI_LOCAL_STRING_INDEX:
                    dh_debug_printf("String Index (%u),\n", item->val);
                    break;
                case RI_LOCAL_STRING_MIN:
                    dh_debug_printf("String Minimum (%u),\n", item->val);
                    break;
                case RI_LOCAL_STRING_MAX:
                    dh_debug_printf("String Maximum (%u),\n", item->val);
                    break;
                case RI_LOCAL_DELIMITER:
                    dh_debug_printf("Delimiter (%u),\n", item->val);
                    break;
            }
            break;
    }
}

/* Print HID report element information in a formatted table */
void debug_print_report_element(report_val_t *val) {
    if (val == NULL) {
        return;
    }
    
    dh_debug_printf("HID: Off=%3u Sz=%2u ID=%u Type=%-5s DType=%-5s ",
               val->offset, val->size, val->report_id,
               item_type_to_str(val->item_type), data_type_to_str(val->data_type));
    dh_debug_printf("Page=%-8s(0x%02X) Usage=0x%04X GUsage=0x%04X ",
               usage_page_to_str(val->usage_page), val->usage_page,
               val->usage, val->global_usage);
    dh_debug_printf("UMin=0x%X UMax=0x%X\n", val->usage_min, val->usage_max);
}

/* Print descriptor header and hex dump - prints directly to output
 * Note: This prints immediately rather than buffering
 */
void print_descriptor_header(uint8_t const *report, int desc_len, uint8_t board_role,
                             uint8_t dev_addr, uint8_t instance, hid_interface_t *iface) {
    if (report == NULL || desc_len <= 0) {
        return;
    }

    /* Limit descriptor length to prevent buffer issues */
    const int MAX_DESC_LEN = 2048;
    if (desc_len > MAX_DESC_LEN) {
        desc_len = MAX_DESC_LEN;
    }

    /* Print header - mimicking usbhid-dump format */
    /* Format: bus:device:interface:DESCRIPTOR         timestamp */
    /* Use interface number 001 for OUTPUT_A (board role 0), 002 for OUTPUT_B (board role 1) */
    uint8_t interface_num = (board_role == 0) ? 1 : 2;
    uint64_t timestamp = get_timestamp_seconds();

    dh_debug_printf("Firmware Version: %u\n", (unsigned)_firmware_metadata.version);

    /* Print device identification */
    if (iface != NULL) {
        dh_debug_printf("Device: VID=0x%04X PID=0x%04X\n", iface->vid, iface->pid);
    }

    dh_debug_printf("002:%03u:%03u:DESCRIPTOR         %llu\n",
               (unsigned)dev_addr, (unsigned)interface_num, timestamp);
    
    /* Print hex data, 16 bytes per line */
    for (int i = 0; i < desc_len; i++) {
        if (i > 0 && (i % 16) == 0) {
            dh_debug_printf("\n");
        } else if (i > 0) {
            dh_debug_printf(" ");
        }
        dh_debug_printf("%02X", report[i]);
    }
    dh_debug_printf("\n\n");
    
    /* Print decoded descriptor header */
    dh_debug_printf("=== HID REPORT DESCRIPTOR (decoded) ===\n");
}

/* Print HID item - prints directly to output
 * This function is called during parsing and prints immediately
 */
void print_hid_item(item_t *item, parser_state_t *parser) {
    if (item == NULL || parser == NULL) {
        return;
    }
    
    /* Handle End Collection indent adjustment */
    int print_indent = indent_level;
    if (item->hdr.type == RI_TYPE_MAIN && 
        item->hdr.tag == RI_MAIN_COLLECTION_END) {
        if (print_indent > 0) {
            print_indent--;
        }
    }
    
    /* Print the item directly */
    print_single_hid_item(item, parser, print_indent);
    
    /* Update indent level for next item */
    if (item->hdr.type == RI_TYPE_MAIN) {
        if (item->hdr.tag == RI_MAIN_COLLECTION) {
            indent_level++;
        } else if (item->hdr.tag == RI_MAIN_COLLECTION_END) {
            if (indent_level > 0) {
                indent_level--;
            }
        }
    }
}

/* Print descriptor footer */
void print_descriptor_footer(void) {
    dh_debug_printf("\n=== END OF DESCRIPTOR ===\n\n");
}

/* Pretty-print extracted descriptor mapping from hid_report.c */
void debug_print_extracted_mapping(const char *name, const report_val_t *val) {
    if (name == NULL || val == NULL) {
        return;
    }

    /* Use current indentation level for consistent formatting */
    int current_indent = get_indent_level();
    
    print_indent_level(current_indent);
    dh_debug_printf("Extracted mapping: %s\n", name);
    
    print_indent_level(current_indent);
    dh_debug_printf("  report_id   : %d\n", val->report_id);
    print_indent_level(current_indent);
    dh_debug_printf("  offset      : %d\n", val->offset);
    print_indent_level(current_indent);
    dh_debug_printf("  offset_idx  : %d\n", val->offset_idx);
    print_indent_level(current_indent);
    dh_debug_printf("  size        : %d\n", val->size);
    print_indent_level(current_indent);
    dh_debug_printf("  item_type   : %d\n", val->item_type);
    print_indent_level(current_indent);
    dh_debug_printf("  data_type   : %d\n", val->data_type);
    print_indent_level(current_indent);
    dh_debug_printf("  usage       : %d\n", val->usage);
    print_indent_level(current_indent);
    dh_debug_printf("  usage_min   : %d\n", val->usage_min);
    print_indent_level(current_indent);
    dh_debug_printf("  usage_max   : %d\n", val->usage_max);
    print_indent_level(current_indent);
    dh_debug_printf("  global_usage: %d\n", val->global_usage);
    print_indent_level(current_indent);
    dh_debug_printf("  usage_page  : %d\n", val->usage_page);
}

void debug_dump_hid_report(uint8_t const *report, uint16_t len, uint8_t dev_addr,
                           uint8_t instance, hid_interface_t *iface,
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
        if (report_throttle[i].dev_addr == dev_addr &&
            report_throttle[i].instance == instance &&
            report_throttle[i].report_id == report_id) {
            entry = &report_throttle[i];
            break;
        }
    }

    if (entry == NULL && throttle_entries_used < MAX_REPORT_THROTTLE_ENTRIES) {
        entry = &report_throttle[throttle_entries_used++];
        entry->dev_addr = dev_addr;
        entry->instance = instance;
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

    dh_debug_printf("\n%03u:%03u:%03u:STREAM             %llu.%06llu\n",
                   BOARD_ROLE, dev_addr, instance, current_time / 1000000, current_time % 1000000);
    dh_debug_printf("Device: VID=0x%04X PID=0x%04X\n", iface->vid, iface->pid);
    for (int i = 0; i < len; i++) {
        if (i > 0 && (i % 16) == 0)
            dh_debug_printf("\n");
        else if (i > 0)
            dh_debug_printf(" ");
        dh_debug_printf("%02X", report[i]);
    }
    dh_debug_printf("\n");

    if (entry != NULL && entry->skipped_count > 0) {
        dh_debug_printf("(Skipped %u reports)\n", entry->skipped_count);
        entry->skipped_count = 0;
    }

    if (entry != NULL) {
        entry->last_dump_time = current_time;
    }

    if (mouse_vals != NULL && iface->mouse.is_found &&
        (!iface->uses_report_id || iface->mouse.report_id == report_id)) {
        dh_debug_printf("  Mouse: X=%d Y=%d Wheel=%d Pan=%d Buttons=0x%X\n",
                       mouse_vals->move_x, mouse_vals->move_y, mouse_vals->wheel,
                       mouse_vals->pan, mouse_vals->buttons);
        dh_debug_printf("    (X: off=%u sz=%u, Y: off=%u sz=%u, Wheel: off=%u sz=%u, Pan: off=%u sz=%u, Buttons: off=%u sz=%u)\n",
                       iface->mouse.move_x.offset, iface->mouse.move_x.size,
                       iface->mouse.move_y.offset, iface->mouse.move_y.size,
                       iface->mouse.wheel.offset, iface->mouse.wheel.size,
                       iface->mouse.pan.offset, iface->mouse.pan.size,
                       iface->mouse.buttons.offset, iface->mouse.buttons.size);
    }

    if (kbd_report != NULL) {
        for (int kbd_idx = 0; kbd_idx < iface->num_keyboards; kbd_idx++) {
            keyboard_t *kbd = &iface->keyboards[kbd_idx];
            if (!iface->uses_report_id || kbd->report_id == report_id) {
                dh_debug_printf("  Keyboard: Mods=0x%02X Keys=[", kbd_report->modifier);
                for (int i = 0; i < 6; i++) {
                    if (i > 0) dh_debug_printf(" ");
                    dh_debug_printf("%02X", kbd_report->keycode[i]);
                }
                dh_debug_printf("]\n");
                dh_debug_printf("    (Mods: off=%u sz=%u", kbd->modifier.offset, kbd->modifier.size);
                if (kbd->is_nkro && kbd->nkro.size > 0) {
                    dh_debug_printf(", NKRO: off=%u sz=%u", kbd->nkro.offset, kbd->nkro.size);
                }
                dh_debug_printf(")\n");
                break;
            }
        }
    }

    dh_debug_printf("\n");
}

#endif
