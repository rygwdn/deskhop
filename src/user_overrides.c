/*
 * This file is part of DeskHop (https://github.com/hrvach/deskhop).
 *
 * User-specific device quirks and overrides.
 * This file is intended to be excluded from version control to allow
 * users to maintain their own device-specific customizations.
 */

#include "main.h"
#include "user_overrides.h"

void apply_device_specific_quirks(mouse_values_t *values, hid_interface_t *iface, bool buttons_extracted) {
    const uint16_t KENSINGTON_VID = 0x047D;
    const uint16_t SMART_MOUSE_PID = 0x8018;

    if (iface->vid == KENSINGTON_VID && iface->pid == SMART_MOUSE_PID) {
        // Invert scroll wheel for trackball
        values->wheel = -values->wheel;

        // Default button mapping is weird, so we remap them
        if (buttons_extracted) {
            uint8_t new_buttons = 0;
            if (values->buttons & (1 << 2)) new_buttons |= (1 << 0); // 3 -> 1
            if (values->buttons & (1 << 3)) new_buttons |= (1 << 1); // 4 -> 2
            if (values->buttons & (1 << 0)) new_buttons |= (1 << 2); // 1 -> 3
            if (values->buttons & (1 << 1)) new_buttons |= (1 << 3); // 2 -> 4
            values->buttons = new_buttons;
        }
    }

    // Add your own device-specific quirks here:
    // if (iface->vid == YOUR_VID && iface->pid == YOUR_PID) {
    //     // Your customizations
    // }
}
