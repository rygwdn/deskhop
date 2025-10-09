/*
 * This file is part of DeskHop (https://github.com/hrvach/deskhop).
 *
 * User-specific device quirks and overrides.
 * This file is intended to be excluded from version control to allow
 * users to maintain their own device-specific customizations.
 */
#pragma once

#include "main.h"

/**
 * Apply device-specific quirks to mouse input values.
 *
 * This function allows users to customize mouse behavior for specific devices
 * based on their VID/PID. Common use cases include:
 * - Inverting scroll wheel direction
 * - Remapping mouse buttons
 * - Adjusting sensitivity or acceleration
 *
 * @param values  Pointer to mouse_values_t structure containing input values
 *                that can be modified in place
 * @param iface   Pointer to hid_interface_t containing device information
 *                (vid, pid, protocol, etc.)
 * @param buttons_extracted  Whether buttons were extracted from the report
 */
void apply_device_specific_quirks(mouse_values_t *values, hid_interface_t *iface, bool buttons_extracted);
