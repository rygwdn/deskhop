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
    // Add your own device-specific quirks here:
    // if (iface->vid == YOUR_VID && iface->pid == YOUR_PID) {
    //     // Your customizations
    // }
}
