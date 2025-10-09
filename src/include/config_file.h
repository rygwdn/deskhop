#pragma once
#include "main.h"

// Parse INI text into state->config. Returns number of keys parsed, -1 on fatal error.
int config_file_parse(device_t *state, const char *text, int len);

// Serialize state->config to INI text. Returns number of bytes written.
int config_file_serialize(const device_t *state, char *buf, int bufsize);
