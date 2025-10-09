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
#include "config_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int config_file_parse(device_t *state, const char *text, int len) {
    static char line_buf[256];
    int parsed = 0;
    const char *p = text;
    const char *end = text + len;

    while (p < end) {
        /* Read one line */
        const char *lstart = p;
        while (p < end && *p != '\n' && *p != '\r')
            p++;
        int llen = (int)(p - lstart);
        while (p < end && (*p == '\n' || *p == '\r'))
            p++;

        if (llen <= 0 || llen >= (int)sizeof(line_buf))
            continue;

        memcpy(line_buf, lstart, llen);
        line_buf[llen] = '\0';

        /* Skip comments and blank lines */
        if (line_buf[0] == '#' || line_buf[0] == '\0')
            continue;

        char *eq = strchr(line_buf, '=');
        if (!eq)
            continue;

        *eq = '\0';
        char *key = line_buf;
        char *val = eq + 1;

        /* Trim trailing whitespace from key */
        char *kend = eq - 1;
        while (kend >= key && (*kend == ' ' || *kend == '\t'))
            *kend-- = '\0';

        const field_map_t *f = get_field_map_by_name(key);
        if (!f || f->readonly)
            continue;

        field_write(state, f, (uint64_t)strtoull(val, NULL, 0));
        parsed++;
    }

    return parsed;
}

int config_file_serialize(const device_t *state, char *buf, int bufsize) {
    int pos = 0;

#define W(...) pos += snprintf(buf + pos, bufsize - pos, __VA_ARGS__)

    W("# DeskHop configuration\n");

    for (size_t i = 0; i < get_field_map_length(); i++) {
        const field_map_t *f = get_field_map_index(i);
        if (f->readonly || !f->name)
            continue;
        W("%s=%llu\n", f->name, (unsigned long long)field_read(state, f));
    }

#undef W

    return pos;
}
