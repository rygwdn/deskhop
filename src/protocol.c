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

const field_map_t api_field_map[] = {
/* Index, Rdonly, Type, Len, Offset in struct, Name */
    { 0,  true,  UINT8,  1, offsetof(device_t, active_output),  "active_output" },
    { 1,  true,  INT16,  2, offsetof(device_t, pointer_x),      "pointer_x" },
    { 2,  true,  INT16,  2, offsetof(device_t, pointer_y),      "pointer_y" },
    { 3,  true,  INT16,  2, offsetof(device_t, mouse_buttons),  "mouse_buttons" },

    /* Output A */
    { 10, false, UINT32, 4, offsetof(device_t, config.output[0].number),                      "output_a_number" },
    { 11, false, UINT32, 4, offsetof(device_t, config.output[0].screen_count),                "output_a_screen_count" },
    { 12, false, INT32,  4, offsetof(device_t, config.output[0].speed_x),                     "output_a_speed_x" },
    { 13, false, INT32,  4, offsetof(device_t, config.output[0].speed_y),                     "output_a_speed_y" },
    { 14, false, INT32,  4, offsetof(device_t, config.output[0].border.top),                  "output_a_border_top" },
    { 15, false, INT32,  4, offsetof(device_t, config.output[0].border.bottom),               "output_a_border_bottom" },
    { 16, false, UINT8,  1, offsetof(device_t, config.output[0].os),                          "output_a_os" },
    { 17, false, UINT8,  1, offsetof(device_t, config.output[0].pos),                         "output_a_pos" },
    { 18, false, UINT8,  1, offsetof(device_t, config.output[0].mouse_park_pos),              "output_a_mouse_park_pos" },
    { 19, false, UINT8,  1, offsetof(device_t, config.output[0].screensaver.mode),            "output_a_screensaver_mode" },
    { 20, false, UINT8,  1, offsetof(device_t, config.output[0].screensaver.only_if_inactive),"output_a_screensaver_only_if_inactive" },

    /* Until we increase the payload size from 8 bytes, clamp to avoid exceeding the field size */
    { 21, false, UINT64, 7, offsetof(device_t, config.output[0].screensaver.idle_time_us),    "output_a_screensaver_idle_time_us" },
    { 22, false, UINT64, 7, offsetof(device_t, config.output[0].screensaver.max_time_us),     "output_a_screensaver_max_time_us" },

    /* Output B */
    { 40, false, UINT32, 4, offsetof(device_t, config.output[1].number),                      "output_b_number" },
    { 41, false, UINT32, 4, offsetof(device_t, config.output[1].screen_count),                "output_b_screen_count" },
    { 42, false, INT32,  4, offsetof(device_t, config.output[1].speed_x),                     "output_b_speed_x" },
    { 43, false, INT32,  4, offsetof(device_t, config.output[1].speed_y),                     "output_b_speed_y" },
    { 44, false, INT32,  4, offsetof(device_t, config.output[1].border.top),                  "output_b_border_top" },
    { 45, false, INT32,  4, offsetof(device_t, config.output[1].border.bottom),               "output_b_border_bottom" },
    { 46, false, UINT8,  1, offsetof(device_t, config.output[1].os),                          "output_b_os" },
    { 47, false, UINT8,  1, offsetof(device_t, config.output[1].pos),                         "output_b_pos" },
    { 48, false, UINT8,  1, offsetof(device_t, config.output[1].mouse_park_pos),              "output_b_mouse_park_pos" },
    { 49, false, UINT8,  1, offsetof(device_t, config.output[1].screensaver.mode),            "output_b_screensaver_mode" },
    { 50, false, UINT8,  1, offsetof(device_t, config.output[1].screensaver.only_if_inactive),"output_b_screensaver_only_if_inactive" },
    { 51, false, UINT64, 7, offsetof(device_t, config.output[1].screensaver.idle_time_us),    "output_b_screensaver_idle_time_us" },
    { 52, false, UINT64, 7, offsetof(device_t, config.output[1].screensaver.max_time_us),     "output_b_screensaver_max_time_us" },

    /* Common config */
    { 70, false, UINT32, 4, offsetof(device_t, config.version),                "version" },
    { 71, false, UINT8,  1, offsetof(device_t, config.force_mouse_boot_mode),  "force_mouse_boot_mode" },
    { 72, false, UINT8,  1, offsetof(device_t, config.force_kbd_boot_protocol),"force_kbd_boot_protocol" },
    { 73, false, UINT8,  1, offsetof(device_t, config.kbd_led_as_indicator),   "kbd_led_as_indicator" },
    { 74, false, UINT8,  1, offsetof(device_t, config.hotkey_toggle),          "hotkey_toggle" },
    { 75, false, UINT8,  1, offsetof(device_t, config.enable_acceleration),    "enable_acceleration" },
    { 76, false, UINT8,  1, offsetof(device_t, config.enforce_ports),          "enforce_ports" },
    { 77, false, UINT16, 2, offsetof(device_t, config.jump_threshold),         "jump_threshold" },

    /* Firmware */
    { 78, true,  UINT16, 2, offsetof(device_t, _running_fw.version),  "fw_version" },
    { 79, true,  UINT32, 4, offsetof(device_t, _running_fw.checksum), "fw_checksum" },

    { 80, true,  UINT8,  1, offsetof(device_t, keyboard_connected), "keyboard_connected" },
    { 81, true,  UINT8,  1, offsetof(device_t, switch_lock),        "switch_lock" },
    { 82, true,  UINT8,  1, offsetof(device_t, relative_mouse),     "relative_mouse" },

    /* Global accel curve — 8 points, each speed(u16) + factor(u16) */
    { 100, false, UINT16, 2, offsetof(device_t, config.accel_curve[0].speed),  "accel_curve_0_speed"  },
    { 101, false, UINT16, 2, offsetof(device_t, config.accel_curve[0].factor), "accel_curve_0_factor" },
    { 102, false, UINT16, 2, offsetof(device_t, config.accel_curve[1].speed),  "accel_curve_1_speed"  },
    { 103, false, UINT16, 2, offsetof(device_t, config.accel_curve[1].factor), "accel_curve_1_factor" },
    { 104, false, UINT16, 2, offsetof(device_t, config.accel_curve[2].speed),  "accel_curve_2_speed"  },
    { 105, false, UINT16, 2, offsetof(device_t, config.accel_curve[2].factor), "accel_curve_2_factor" },
    { 106, false, UINT16, 2, offsetof(device_t, config.accel_curve[3].speed),  "accel_curve_3_speed"  },
    { 107, false, UINT16, 2, offsetof(device_t, config.accel_curve[3].factor), "accel_curve_3_factor" },
    { 108, false, UINT16, 2, offsetof(device_t, config.accel_curve[4].speed),  "accel_curve_4_speed"  },
    { 109, false, UINT16, 2, offsetof(device_t, config.accel_curve[4].factor), "accel_curve_4_factor" },
    { 110, false, UINT16, 2, offsetof(device_t, config.accel_curve[5].speed),  "accel_curve_5_speed"  },
    { 111, false, UINT16, 2, offsetof(device_t, config.accel_curve[5].factor), "accel_curve_5_factor" },
    { 112, false, UINT16, 2, offsetof(device_t, config.accel_curve[6].speed),  "accel_curve_6_speed"  },
    { 113, false, UINT16, 2, offsetof(device_t, config.accel_curve[6].factor), "accel_curve_6_factor" },
    { 114, false, UINT16, 2, offsetof(device_t, config.accel_curve[7].speed),  "accel_curve_7_speed"  },
    { 115, false, UINT16, 2, offsetof(device_t, config.accel_curve[7].factor), "accel_curve_7_factor" },

    /* Device 0 */
    { 120, false, UINT16, 2, offsetof(device_t, config.devices[0].vid),            "device_0_vid" },
    { 121, false, UINT16, 2, offsetof(device_t, config.devices[0].pid),            "device_0_pid" },
    { 122, false, UINT8,  1, offsetof(device_t, config.devices[0].invert_scroll),  "device_0_invert_scroll" },
    { 123, false, UINT8,  1, offsetof(device_t, config.devices[0].use_accel_curve),"device_0_use_accel_curve" },
    { 124, false, UINT8,  1, offsetof(device_t, config.devices[0].button_map[0]),  "device_0_button_map_0" },
    { 125, false, UINT8,  1, offsetof(device_t, config.devices[0].button_map[1]),  "device_0_button_map_1" },
    { 126, false, UINT8,  1, offsetof(device_t, config.devices[0].button_map[2]),  "device_0_button_map_2" },
    { 127, false, UINT8,  1, offsetof(device_t, config.devices[0].button_map[3]),  "device_0_button_map_3" },
    { 128, false, UINT8,  1, offsetof(device_t, config.devices[0].button_map[4]),  "device_0_button_map_4" },
    { 129, false, UINT8,  1, offsetof(device_t, config.devices[0].button_map[5]),  "device_0_button_map_5" },
    { 130, false, UINT8,  1, offsetof(device_t, config.devices[0].button_map[6]),  "device_0_button_map_6" },
    { 131, false, UINT8,  1, offsetof(device_t, config.devices[0].button_map[7]),  "device_0_button_map_7" },
    { 132, false, UINT16, 2, offsetof(device_t, config.devices[0].accel_curve[0].speed),  "device_0_accel_curve_0_speed"  },
    { 133, false, UINT16, 2, offsetof(device_t, config.devices[0].accel_curve[0].factor), "device_0_accel_curve_0_factor" },
    { 134, false, UINT16, 2, offsetof(device_t, config.devices[0].accel_curve[1].speed),  "device_0_accel_curve_1_speed"  },
    { 135, false, UINT16, 2, offsetof(device_t, config.devices[0].accel_curve[1].factor), "device_0_accel_curve_1_factor" },
    { 136, false, UINT16, 2, offsetof(device_t, config.devices[0].accel_curve[2].speed),  "device_0_accel_curve_2_speed"  },
    { 137, false, UINT16, 2, offsetof(device_t, config.devices[0].accel_curve[2].factor), "device_0_accel_curve_2_factor" },
    { 138, false, UINT16, 2, offsetof(device_t, config.devices[0].accel_curve[3].speed),  "device_0_accel_curve_3_speed"  },
    { 139, false, UINT16, 2, offsetof(device_t, config.devices[0].accel_curve[3].factor), "device_0_accel_curve_3_factor" },
    { 140, false, UINT16, 2, offsetof(device_t, config.devices[0].accel_curve[4].speed),  "device_0_accel_curve_4_speed"  },
    { 141, false, UINT16, 2, offsetof(device_t, config.devices[0].accel_curve[4].factor), "device_0_accel_curve_4_factor" },
    { 142, false, UINT16, 2, offsetof(device_t, config.devices[0].accel_curve[5].speed),  "device_0_accel_curve_5_speed"  },
    { 143, false, UINT16, 2, offsetof(device_t, config.devices[0].accel_curve[5].factor), "device_0_accel_curve_5_factor" },
    { 144, false, UINT16, 2, offsetof(device_t, config.devices[0].accel_curve[6].speed),  "device_0_accel_curve_6_speed"  },
    { 145, false, UINT16, 2, offsetof(device_t, config.devices[0].accel_curve[6].factor), "device_0_accel_curve_6_factor" },
    { 146, false, UINT16, 2, offsetof(device_t, config.devices[0].accel_curve[7].speed),  "device_0_accel_curve_7_speed"  },
    { 147, false, UINT16, 2, offsetof(device_t, config.devices[0].accel_curve[7].factor), "device_0_accel_curve_7_factor" },

    /* Device 1 */
    { 150, false, UINT16, 2, offsetof(device_t, config.devices[1].vid),            "device_1_vid" },
    { 151, false, UINT16, 2, offsetof(device_t, config.devices[1].pid),            "device_1_pid" },
    { 152, false, UINT8,  1, offsetof(device_t, config.devices[1].invert_scroll),  "device_1_invert_scroll" },
    { 153, false, UINT8,  1, offsetof(device_t, config.devices[1].use_accel_curve),"device_1_use_accel_curve" },
    { 154, false, UINT8,  1, offsetof(device_t, config.devices[1].button_map[0]),  "device_1_button_map_0" },
    { 155, false, UINT8,  1, offsetof(device_t, config.devices[1].button_map[1]),  "device_1_button_map_1" },
    { 156, false, UINT8,  1, offsetof(device_t, config.devices[1].button_map[2]),  "device_1_button_map_2" },
    { 157, false, UINT8,  1, offsetof(device_t, config.devices[1].button_map[3]),  "device_1_button_map_3" },
    { 158, false, UINT8,  1, offsetof(device_t, config.devices[1].button_map[4]),  "device_1_button_map_4" },
    { 159, false, UINT8,  1, offsetof(device_t, config.devices[1].button_map[5]),  "device_1_button_map_5" },
    { 160, false, UINT8,  1, offsetof(device_t, config.devices[1].button_map[6]),  "device_1_button_map_6" },
    { 161, false, UINT8,  1, offsetof(device_t, config.devices[1].button_map[7]),  "device_1_button_map_7" },
    { 162, false, UINT16, 2, offsetof(device_t, config.devices[1].accel_curve[0].speed),  "device_1_accel_curve_0_speed"  },
    { 163, false, UINT16, 2, offsetof(device_t, config.devices[1].accel_curve[0].factor), "device_1_accel_curve_0_factor" },
    { 164, false, UINT16, 2, offsetof(device_t, config.devices[1].accel_curve[1].speed),  "device_1_accel_curve_1_speed"  },
    { 165, false, UINT16, 2, offsetof(device_t, config.devices[1].accel_curve[1].factor), "device_1_accel_curve_1_factor" },
    { 166, false, UINT16, 2, offsetof(device_t, config.devices[1].accel_curve[2].speed),  "device_1_accel_curve_2_speed"  },
    { 167, false, UINT16, 2, offsetof(device_t, config.devices[1].accel_curve[2].factor), "device_1_accel_curve_2_factor" },
    { 168, false, UINT16, 2, offsetof(device_t, config.devices[1].accel_curve[3].speed),  "device_1_accel_curve_3_speed"  },
    { 169, false, UINT16, 2, offsetof(device_t, config.devices[1].accel_curve[3].factor), "device_1_accel_curve_3_factor" },
    { 170, false, UINT16, 2, offsetof(device_t, config.devices[1].accel_curve[4].speed),  "device_1_accel_curve_4_speed"  },
    { 171, false, UINT16, 2, offsetof(device_t, config.devices[1].accel_curve[4].factor), "device_1_accel_curve_4_factor" },
    { 172, false, UINT16, 2, offsetof(device_t, config.devices[1].accel_curve[5].speed),  "device_1_accel_curve_5_speed"  },
    { 173, false, UINT16, 2, offsetof(device_t, config.devices[1].accel_curve[5].factor), "device_1_accel_curve_5_factor" },
    { 174, false, UINT16, 2, offsetof(device_t, config.devices[1].accel_curve[6].speed),  "device_1_accel_curve_6_speed"  },
    { 175, false, UINT16, 2, offsetof(device_t, config.devices[1].accel_curve[6].factor), "device_1_accel_curve_6_factor" },
    { 176, false, UINT16, 2, offsetof(device_t, config.devices[1].accel_curve[7].speed),  "device_1_accel_curve_7_speed"  },
    { 177, false, UINT16, 2, offsetof(device_t, config.devices[1].accel_curve[7].factor), "device_1_accel_curve_7_factor" },

    /* Device 2 */
    { 180, false, UINT16, 2, offsetof(device_t, config.devices[2].vid),            "device_2_vid" },
    { 181, false, UINT16, 2, offsetof(device_t, config.devices[2].pid),            "device_2_pid" },
    { 182, false, UINT8,  1, offsetof(device_t, config.devices[2].invert_scroll),  "device_2_invert_scroll" },
    { 183, false, UINT8,  1, offsetof(device_t, config.devices[2].use_accel_curve),"device_2_use_accel_curve" },
    { 184, false, UINT8,  1, offsetof(device_t, config.devices[2].button_map[0]),  "device_2_button_map_0" },
    { 185, false, UINT8,  1, offsetof(device_t, config.devices[2].button_map[1]),  "device_2_button_map_1" },
    { 186, false, UINT8,  1, offsetof(device_t, config.devices[2].button_map[2]),  "device_2_button_map_2" },
    { 187, false, UINT8,  1, offsetof(device_t, config.devices[2].button_map[3]),  "device_2_button_map_3" },
    { 188, false, UINT8,  1, offsetof(device_t, config.devices[2].button_map[4]),  "device_2_button_map_4" },
    { 189, false, UINT8,  1, offsetof(device_t, config.devices[2].button_map[5]),  "device_2_button_map_5" },
    { 190, false, UINT8,  1, offsetof(device_t, config.devices[2].button_map[6]),  "device_2_button_map_6" },
    { 191, false, UINT8,  1, offsetof(device_t, config.devices[2].button_map[7]),  "device_2_button_map_7" },
    { 192, false, UINT16, 2, offsetof(device_t, config.devices[2].accel_curve[0].speed),  "device_2_accel_curve_0_speed"  },
    { 193, false, UINT16, 2, offsetof(device_t, config.devices[2].accel_curve[0].factor), "device_2_accel_curve_0_factor" },
    { 194, false, UINT16, 2, offsetof(device_t, config.devices[2].accel_curve[1].speed),  "device_2_accel_curve_1_speed"  },
    { 195, false, UINT16, 2, offsetof(device_t, config.devices[2].accel_curve[1].factor), "device_2_accel_curve_1_factor" },
    { 196, false, UINT16, 2, offsetof(device_t, config.devices[2].accel_curve[2].speed),  "device_2_accel_curve_2_speed"  },
    { 197, false, UINT16, 2, offsetof(device_t, config.devices[2].accel_curve[2].factor), "device_2_accel_curve_2_factor" },
    { 198, false, UINT16, 2, offsetof(device_t, config.devices[2].accel_curve[3].speed),  "device_2_accel_curve_3_speed"  },
    { 199, false, UINT16, 2, offsetof(device_t, config.devices[2].accel_curve[3].factor), "device_2_accel_curve_3_factor" },
    { 200, false, UINT16, 2, offsetof(device_t, config.devices[2].accel_curve[4].speed),  "device_2_accel_curve_4_speed"  },
    { 201, false, UINT16, 2, offsetof(device_t, config.devices[2].accel_curve[4].factor), "device_2_accel_curve_4_factor" },
    { 202, false, UINT16, 2, offsetof(device_t, config.devices[2].accel_curve[5].speed),  "device_2_accel_curve_5_speed"  },
    { 203, false, UINT16, 2, offsetof(device_t, config.devices[2].accel_curve[5].factor), "device_2_accel_curve_5_factor" },
    { 204, false, UINT16, 2, offsetof(device_t, config.devices[2].accel_curve[6].speed),  "device_2_accel_curve_6_speed"  },
    { 205, false, UINT16, 2, offsetof(device_t, config.devices[2].accel_curve[6].factor), "device_2_accel_curve_6_factor" },
    { 206, false, UINT16, 2, offsetof(device_t, config.devices[2].accel_curve[7].speed),  "device_2_accel_curve_7_speed"  },
    { 207, false, UINT16, 2, offsetof(device_t, config.devices[2].accel_curve[7].factor), "device_2_accel_curve_7_factor" },

    /* Device 3 */
    { 210, false, UINT16, 2, offsetof(device_t, config.devices[3].vid),            "device_3_vid" },
    { 211, false, UINT16, 2, offsetof(device_t, config.devices[3].pid),            "device_3_pid" },
    { 212, false, UINT8,  1, offsetof(device_t, config.devices[3].invert_scroll),  "device_3_invert_scroll" },
    { 213, false, UINT8,  1, offsetof(device_t, config.devices[3].use_accel_curve),"device_3_use_accel_curve" },
    { 214, false, UINT8,  1, offsetof(device_t, config.devices[3].button_map[0]),  "device_3_button_map_0" },
    { 215, false, UINT8,  1, offsetof(device_t, config.devices[3].button_map[1]),  "device_3_button_map_1" },
    { 216, false, UINT8,  1, offsetof(device_t, config.devices[3].button_map[2]),  "device_3_button_map_2" },
    { 217, false, UINT8,  1, offsetof(device_t, config.devices[3].button_map[3]),  "device_3_button_map_3" },
    { 218, false, UINT8,  1, offsetof(device_t, config.devices[3].button_map[4]),  "device_3_button_map_4" },
    { 219, false, UINT8,  1, offsetof(device_t, config.devices[3].button_map[5]),  "device_3_button_map_5" },
    { 220, false, UINT8,  1, offsetof(device_t, config.devices[3].button_map[6]),  "device_3_button_map_6" },
    { 221, false, UINT8,  1, offsetof(device_t, config.devices[3].button_map[7]),  "device_3_button_map_7" },
    { 222, false, UINT16, 2, offsetof(device_t, config.devices[3].accel_curve[0].speed),  "device_3_accel_curve_0_speed"  },
    { 223, false, UINT16, 2, offsetof(device_t, config.devices[3].accel_curve[0].factor), "device_3_accel_curve_0_factor" },
    { 224, false, UINT16, 2, offsetof(device_t, config.devices[3].accel_curve[1].speed),  "device_3_accel_curve_1_speed"  },
    { 225, false, UINT16, 2, offsetof(device_t, config.devices[3].accel_curve[1].factor), "device_3_accel_curve_1_factor" },
    { 226, false, UINT16, 2, offsetof(device_t, config.devices[3].accel_curve[2].speed),  "device_3_accel_curve_2_speed"  },
    { 227, false, UINT16, 2, offsetof(device_t, config.devices[3].accel_curve[2].factor), "device_3_accel_curve_2_factor" },
    { 228, false, UINT16, 2, offsetof(device_t, config.devices[3].accel_curve[3].speed),  "device_3_accel_curve_3_speed"  },
    { 229, false, UINT16, 2, offsetof(device_t, config.devices[3].accel_curve[3].factor), "device_3_accel_curve_3_factor" },
    { 230, false, UINT16, 2, offsetof(device_t, config.devices[3].accel_curve[4].speed),  "device_3_accel_curve_4_speed"  },
    { 231, false, UINT16, 2, offsetof(device_t, config.devices[3].accel_curve[4].factor), "device_3_accel_curve_4_factor" },
    { 232, false, UINT16, 2, offsetof(device_t, config.devices[3].accel_curve[5].speed),  "device_3_accel_curve_5_speed"  },
    { 233, false, UINT16, 2, offsetof(device_t, config.devices[3].accel_curve[5].factor), "device_3_accel_curve_5_factor" },
    { 234, false, UINT16, 2, offsetof(device_t, config.devices[3].accel_curve[6].speed),  "device_3_accel_curve_6_speed"  },
    { 235, false, UINT16, 2, offsetof(device_t, config.devices[3].accel_curve[6].factor), "device_3_accel_curve_6_factor" },
    { 236, false, UINT16, 2, offsetof(device_t, config.devices[3].accel_curve[7].speed),  "device_3_accel_curve_7_speed"  },
    { 237, false, UINT16, 2, offsetof(device_t, config.devices[3].accel_curve[7].factor), "device_3_accel_curve_7_factor" },
};

const field_map_t* get_field_map_by_name(const char *name) {
    for (unsigned int i = 0; i < ARRAY_SIZE(api_field_map); i++) {
        if (api_field_map[i].name && strcmp(api_field_map[i].name, name) == 0)
            return &api_field_map[i];
    }
    return NULL;
}

uint64_t field_read(const device_t *state, const field_map_t *f) {
    const uint8_t *ptr = (const uint8_t *)state + f->offset;
    uint64_t val = 0;
    switch (f->type) {
        case UINT8:  case BOOL: val = *(const uint8_t  *)ptr; break;
        case UINT16:            val = *(const uint16_t *)ptr; break;
        case UINT32:            val = *(const uint32_t *)ptr; break;
        case UINT64:            memcpy(&val, ptr, sizeof(uint64_t)); break;
        case INT8:              val = (uint64_t)(int64_t)*(const int8_t  *)ptr; break;
        case INT16:             val = (uint64_t)(int64_t)*(const int16_t *)ptr; break;
        case INT32:             val = (uint64_t)(int64_t)*(const int32_t *)ptr; break;
        case INT64:             memcpy(&val, ptr, sizeof(int64_t)); break;
    }
    return val;
}

void field_write(device_t *state, const field_map_t *f, uint64_t val) {
    uint8_t *ptr = (uint8_t *)state + f->offset;
    switch (f->type) {
        case UINT8:  case BOOL: *(uint8_t  *)ptr = (uint8_t )val; break;
        case UINT16:            *(uint16_t *)ptr = (uint16_t)val; break;
        case UINT32:            *(uint32_t *)ptr = (uint32_t)val; break;
        case UINT64:            memcpy(ptr, &val, sizeof(uint64_t)); break;
        case INT8:              *(int8_t  *)ptr = (int8_t )val; break;
        case INT16:             *(int16_t *)ptr = (int16_t)val; break;
        case INT32:             *(int32_t *)ptr = (int32_t)val; break;
        case INT64:             memcpy(ptr, &val, sizeof(int64_t)); break;
    }
}

const field_map_t* get_field_map_entry(uint32_t index) {
    for (unsigned int i = 0; i < ARRAY_SIZE(api_field_map); i++) {
        if (api_field_map[i].idx == index) {
            return &api_field_map[i];
        }
    }

    return NULL;
}


const field_map_t* get_field_map_index(uint32_t index) {
    /* Clamp potential overflows to last element. */
    if (index >= ARRAY_SIZE(api_field_map))
        index = ARRAY_SIZE(api_field_map) - 1;

    return &api_field_map[index];
}

size_t get_field_map_length(void) {
    return ARRAY_SIZE(api_field_map);
}

void _queue_packet(uint8_t *payload, device_t *state, uint8_t type, uint8_t len, uint8_t id, uint8_t inst) {
    hid_generic_pkt_t generic_packet = {
        .instance = inst,
        .report_id = id,
        .type = type,
        .len = len,
    };

    memcpy(generic_packet.data, payload, len);
    queue_try_add(&state->hid_queue_out, &generic_packet);
}

void queue_cfg_packet(uart_packet_t *packet, device_t *state) {
    uint8_t raw_packet[RAW_PACKET_LENGTH];
    write_raw_packet(raw_packet, packet);
    _queue_packet(raw_packet, state, 0, RAW_PACKET_LENGTH, REPORT_ID_VENDOR, ITF_NUM_HID_VENDOR);
}

void queue_cc_packet(uint8_t *payload, device_t *state) {
    _queue_packet(payload, state, 1, CONSUMER_CONTROL_LENGTH, REPORT_ID_CONSUMER, ITF_NUM_HID);
}

void queue_system_packet(uint8_t *payload, device_t *state) {
    _queue_packet(payload, state, 2, SYSTEM_CONTROL_LENGTH, REPORT_ID_SYSTEM, ITF_NUM_HID);
}
