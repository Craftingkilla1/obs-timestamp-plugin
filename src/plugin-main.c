#include "timestamp-plugin.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-timestamp-plugin", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
    return "OBS Timestamp Marker Plugin - Create markers during recording for Premiere Pro";
}

MODULE_EXPORT const char *obs_module_name(void)
{
    return "OBS Timestamp Marker";
}

bool obs_module_load(void)
{
    blog(LOG_INFO, "OBS Timestamp Plugin v1.0.0 loaded");
    init_timestamp_plugin();
    return true;
}

void obs_module_unload(void)
{
    blog(LOG_INFO, "OBS Timestamp Plugin unloading");
    free_timestamp_plugin();
}
