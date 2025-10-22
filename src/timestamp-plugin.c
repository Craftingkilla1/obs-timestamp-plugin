#include "timestamp-plugin.h"
#include <time.h>

// Global state
static obs_hotkey_id timestamp_hotkey_id = OBS_INVALID_HOTKEY_ID;
static bool recording_active = false;
static uint64_t recording_start_time = 0;
static char output_path[512] = {0};
static uint64_t marker_counter = 0;
static char recording_output_dir[512] = {0};

// Get the default output path
static void get_default_output_path(char *buffer, size_t size)
{
    const char *config_path = obs_module_config_path("");
    if (config_path) {
        snprintf(buffer, size, "%s/timestamps.jsonl", config_path);
        bfree((void *)config_path);
    } else {
        snprintf(buffer, size, "timestamps.jsonl");
    }
}

// Ensure the config directory exists
static void ensure_config_directory_exists(void)
{
    const char *config_path = obs_module_config_path("");
    if (config_path) {
        // Create the directory if it doesn't exist
        os_mkdirs(config_path);
        blog(LOG_INFO, "Timestamp Plugin: Config directory: %s", config_path);
        bfree((void *)config_path);
    }
}

// Get the recording output directory from OBS settings
static void get_recording_output_dir(char *buffer, size_t size)
{
    config_t *config = obs_frontend_get_profile_config();
    if (!config) {
        blog(LOG_WARNING, "Timestamp Plugin: Could not get profile config");
        buffer[0] = '\0';
        return;
    }

    // Check if using advanced output mode or simple mode
    const char *mode = config_get_string(config, "Output", "Mode");
    const char *rec_path = NULL;

    if (mode && strcmp(mode, "Advanced") == 0) {
        // Advanced mode - check RecFilePath
        rec_path = config_get_string(config, "AdvOut", "RecFilePath");
    } else {
        // Simple mode - check FilePath
        rec_path = config_get_string(config, "SimpleOutput", "FilePath");
    }

    if (rec_path && *rec_path) {
        snprintf(buffer, size, "%s", rec_path);
        blog(LOG_INFO, "Timestamp Plugin: Recording output directory: %s", buffer);
    } else {
        buffer[0] = '\0';
        blog(LOG_WARNING, "Timestamp Plugin: Could not determine recording output directory");
    }
}

// Load hotkey data from OBS global config
void load_hotkey_data(void)
{
    if (timestamp_hotkey_id == OBS_INVALID_HOTKEY_ID) {
        blog(LOG_WARNING, "Timestamp Plugin: Cannot load hotkey data - hotkey not registered");
        return;
    }

    // Get OBS's global config
    config_t *config = obs_frontend_get_profile_config();
    if (!config) {
        blog(LOG_WARNING, "Timestamp Plugin: Could not get profile config");
        return;
    }

    // Load hotkey data from the "Hotkeys" section
    obs_data_t *data = obs_data_create();

    // Get the hotkey array from config
    obs_data_array_t *array = obs_data_array_create();

    // Try to load from config file
    const char *json_str = config_get_string(config, "Hotkeys", "timestamp_marker");
    if (json_str && *json_str) {
        obs_data_t *temp = obs_data_create_from_json(json_str);
        if (temp) {
            obs_data_array_t *bindings = obs_data_get_array(temp, "bindings");
            if (bindings) {
                array = bindings;
                blog(LOG_INFO, "Timestamp Plugin: Loading hotkey bindings");
            }
            obs_data_release(temp);
        }
    }

    // Load into hotkey
    obs_hotkey_load(timestamp_hotkey_id, array);

    obs_data_array_release(array);
    obs_data_release(data);

    blog(LOG_INFO, "Timestamp Plugin: Hotkey data loaded");
}

// Save hotkey data to OBS global config
void save_hotkey_data(void)
{
    if (timestamp_hotkey_id == OBS_INVALID_HOTKEY_ID) {
        blog(LOG_WARNING, "Timestamp Plugin: Cannot save hotkey data - hotkey not registered");
        return;
    }

    // Get OBS's global config
    config_t *config = obs_frontend_get_profile_config();
    if (!config) {
        blog(LOG_WARNING, "Timestamp Plugin: Could not get profile config");
        return;
    }

    // Save hotkey data
    obs_data_array_t *array = obs_hotkey_save(timestamp_hotkey_id);

    // Convert to JSON and save to config
    obs_data_t *data = obs_data_create();
    obs_data_set_array(data, "bindings", array);

    const char *json_str = obs_data_get_json(data);
    config_set_string(config, "Hotkeys", "timestamp_marker", json_str);

    obs_data_array_release(array);
    obs_data_release(data);

    blog(LOG_INFO, "Timestamp Plugin: Hotkey data saved");
}

// Save a timestamp to the output file in JSON Lines format
void save_timestamp(uint64_t timestamp_ms, const char *comment, const char *name, const char *color)
{
    if (!output_path[0]) {
        blog(LOG_ERROR, "Timestamp Plugin: Output path not set");
        return;
    }

    FILE *file = fopen(output_path, "a");
    if (!file) {
        blog(LOG_ERROR, "Timestamp Plugin: Failed to open output file: %s", output_path);
        return;
    }

    // Write JSON line
    fprintf(file, "{\"timestamp_ms\": %" PRIu64 ", \"comment\": \"%s\", \"name\": \"%s\", \"color\": \"%s\"}\n",
            timestamp_ms,
            comment ? comment : "",
            name ? name : "",
            color ? color : "blue");

    fclose(file);

    blog(LOG_INFO, "Timestamp Plugin: Saved marker at %" PRIu64 "ms: %s",
         timestamp_ms, comment ? comment : "(no comment)");
}

// Hotkey callback - called when user presses the timestamp hotkey
void timestamp_hotkey_callback(void *data, obs_hotkey_id id, obs_hotkey_t *hotkey, bool pressed)
{
    UNUSED_PARAMETER(data);
    UNUSED_PARAMETER(id);
    UNUSED_PARAMETER(hotkey);

    if (!pressed || !recording_active) {
        return;
    }

    // Calculate elapsed time since recording started
    uint64_t current_time = os_gettime_ns() / 1000000; // Convert to milliseconds
    uint64_t timestamp_ms = current_time - recording_start_time;

    // Create a default comment with marker number
    marker_counter++;
    char comment[128];
    snprintf(comment, sizeof(comment), "Marker %llu",
             (unsigned long long)marker_counter);

    // Save timestamp with default values
    // TODO: In the future, we can add a dialog to let users input custom comments
    save_timestamp(timestamp_ms, comment, "", "blue");
}

// Frontend event callback - handles recording start/stop events
static void frontend_event_callback(enum obs_frontend_event event, void *data)
{
    UNUSED_PARAMETER(data);

    switch (event) {
    case OBS_FRONTEND_EVENT_RECORDING_STARTED:
        recording_active = true;
        recording_start_time = os_gettime_ns() / 1000000; // Milliseconds
        marker_counter = 0;

        // Get the recording output directory
        get_recording_output_dir(recording_output_dir, sizeof(recording_output_dir));

        blog(LOG_INFO, "Timestamp Plugin: Recording started, clearing timestamp file");

        // Clear/create new timestamp file for this recording session
        if (output_path[0]) {
            FILE *file = fopen(output_path, "w");
            if (file) {
                // Get FPS from OBS config
                config_t *config = obs_frontend_get_profile_config();
                uint32_t fps_num = 60;
                uint32_t fps_den = 1;

                if (config) {
                    // Read FPS type and value from config
                    const char *fps_type = config_get_string(config, "Video", "FPSType");

                    if (fps_type && strcmp(fps_type, "2") == 0) {
                        // Common FPS (FPSCommon setting)
                        const char *fps_common = config_get_string(config, "Video", "FPSCommon");
                        if (fps_common) {
                            if (strcmp(fps_common, "60") == 0) {
                                fps_num = 60; fps_den = 1;
                            } else if (strcmp(fps_common, "59.94") == 0) {
                                fps_num = 60000; fps_den = 1001;
                            } else if (strcmp(fps_common, "30") == 0) {
                                fps_num = 30; fps_den = 1;
                            } else if (strcmp(fps_common, "29.97") == 0) {
                                fps_num = 30000; fps_den = 1001;
                            } else if (strcmp(fps_common, "25") == 0) {
                                fps_num = 25; fps_den = 1;
                            } else if (strcmp(fps_common, "24") == 0) {
                                fps_num = 24; fps_den = 1;
                            } else if (strcmp(fps_common, "23.976") == 0) {
                                fps_num = 24000; fps_den = 1001;
                            }
                        }
                    } else {
                        // Fractional FPS (FPSNum/FPSDen)
                        fps_num = (uint32_t)config_get_uint(config, "Video", "FPSNum");
                        fps_den = (uint32_t)config_get_uint(config, "Video", "FPSDen");
                        if (fps_den == 0) fps_den = 1; // Prevent division by zero
                    }
                }

                // Get current timestamp for metadata
                time_t now = time(NULL);
                char time_str[64];
                struct tm *tm_info = localtime(&now);
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

                // Write metadata header
                fprintf(file, "{\"metadata\": {\"recording_path\": \"%s\", \"timestamp\": \"%s\", \"fps_num\": %u, \"fps_den\": %u}}\n",
                        recording_output_dir[0] ? recording_output_dir : "",
                        time_str,
                        fps_num,
                        fps_den);

                // Add initial marker at 0
                fprintf(file, "{\"timestamp_ms\": 0, \"comment\": \"Recording Start\", \"name\": \"\", \"color\": \"blue\"}\n");
                fclose(file);
            } else {
                blog(LOG_ERROR, "Timestamp Plugin: Failed to create output file: %s", output_path);
            }
        }
        break;

    case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
        if (recording_active) {
            // Add final marker
            uint64_t current_time = os_gettime_ns() / 1000000;
            uint64_t timestamp_ms = current_time - recording_start_time;
            save_timestamp(timestamp_ms, "Recording End", "", "green");

            blog(LOG_INFO, "Timestamp Plugin: Recording stopped, final timestamp: %" PRIu64 "ms", timestamp_ms);
        }
        recording_active = false;
        break;

    default:
        break;
    }
}

// Initialize the plugin
void init_timestamp_plugin(void)
{
    // Ensure the config directory exists
    ensure_config_directory_exists();

    // Set default output path
    get_default_output_path(output_path, sizeof(output_path));
    blog(LOG_INFO, "Timestamp Plugin: Using output file: %s", output_path);

    // Register hotkey - OBS automatically handles save/load for frontend hotkeys
    timestamp_hotkey_id = obs_hotkey_register_frontend(
        "timestamp_marker",
        "Create Timestamp Marker",
        timestamp_hotkey_callback,
        NULL);

    if (timestamp_hotkey_id == OBS_INVALID_HOTKEY_ID) {
        blog(LOG_ERROR, "Timestamp Plugin: Failed to register hotkey");
    } else {
        blog(LOG_INFO, "Timestamp Plugin: Hotkey registered successfully (ID: %llu)",
             (unsigned long long)timestamp_hotkey_id);
    }

    // Register frontend event callbacks
    obs_frontend_add_event_callback(frontend_event_callback, NULL);
}

// Clean up plugin resources
void free_timestamp_plugin(void)
{
    // Unregister hotkey - OBS automatically saves hotkey state
    if (timestamp_hotkey_id != OBS_INVALID_HOTKEY_ID) {
        obs_hotkey_unregister(timestamp_hotkey_id);
        timestamp_hotkey_id = OBS_INVALID_HOTKEY_ID;
    }

    // Remove frontend event callback
    obs_frontend_remove_event_callback(frontend_event_callback, NULL);

    blog(LOG_INFO, "Timestamp Plugin: Cleaned up");
}

// Get current output path
const char *get_output_path(void)
{
    return output_path;
}

// Set output path
void set_output_path(const char *path)
{
    if (path && *path) {
        snprintf(output_path, sizeof(output_path), "%s", path);
        blog(LOG_INFO, "Timestamp Plugin: Output path set to: %s", output_path);
    }
}
