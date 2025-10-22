#include "timestamp-plugin.h"

// Global state
static obs_hotkey_id timestamp_hotkey_id = OBS_INVALID_HOTKEY_ID;
static bool recording_active = false;
static uint64_t recording_start_time = 0;
static char output_path[512] = {0};
static uint64_t marker_counter = 0;

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

        blog(LOG_INFO, "Timestamp Plugin: Recording started, clearing timestamp file");

        // Clear/create new timestamp file for this recording session
        if (output_path[0]) {
            FILE *file = fopen(output_path, "w");
            if (file) {
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

    // Register hotkey
    timestamp_hotkey_id = obs_hotkey_register_frontend(
        "timestamp_marker",
        "Create Timestamp Marker",
        timestamp_hotkey_callback,
        NULL);

    if (timestamp_hotkey_id == OBS_INVALID_HOTKEY_ID) {
        blog(LOG_ERROR, "Timestamp Plugin: Failed to register hotkey");
    } else {
        blog(LOG_INFO, "Timestamp Plugin: Hotkey registered successfully");
    }

    // Register frontend event callbacks
    obs_frontend_add_event_callback(frontend_event_callback, NULL);
}

// Clean up plugin resources
void free_timestamp_plugin(void)
{
    // Unregister hotkey
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
