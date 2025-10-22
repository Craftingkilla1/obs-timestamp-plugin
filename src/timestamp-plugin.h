#pragma once

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/platform.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

// Plugin initialization
void init_timestamp_plugin(void);
void free_timestamp_plugin(void);

// Hotkey callback
void timestamp_hotkey_callback(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed);

// Recording state callbacks
void recording_started_callback(enum obs_frontend_event event, void *data);
void recording_stopped_callback(enum obs_frontend_event event, void *data);

// Timestamp saving
void save_timestamp(uint64_t timestamp_ms, const char *comment, const char *name, const char *color);

// Configuration
const char *get_output_path(void);
void set_output_path(const char *path);

#ifdef __cplusplus
}
#endif
