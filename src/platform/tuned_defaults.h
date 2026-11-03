#pragma once
// Defaults the owner tuned in the F10 panels (user screenshots, 3 Nov 2026), used when a setting is
// neither in the ini nor on the command line. Explicit flags and saved settings always win.
// Only keys whose tuned value differs from the old built-in default are listed.
#include <cstring>
struct TunedDefault { const char *key; int value; };
static const TunedDefault kTunedDefaults[] = {
    {"video.speed_fov", 100}, {"video.dof_blur", 100}, {"video.dof_blur_max_px", 16},
    {"video.motion_blur", 0}, {"video.motion_blur_amount", 60}, {"video.vapor_trails", 1},
    {"video.speed_lines", 0}, {"video.speed_streak", 100}, {"video.tire_debris", 575},
    {"video.dust_plume_amount", 300}, {"video.spray_amount", 300}, {"video.exhaust_smoke_amount", 300},
    {"video.splash_amount", 300}, {"video.backfire_amount", 300}, {"video.sun_glare", 60},
    {"video.fog", 1}, {"video.fog_amount", 60}, {"video.fog_glow", 100}, {"video.fog_clear", 60}, {"video.fog_cull", 1},
    {"video.fog_spatial", 1}, {"video.fog_evolve", 60}, {"video.air_motes", 1},
    {"video.air_motes_amount", 500}, {"video.fog_lights", 40}, {"video.snow_glint", 100},
    {"video.camera_roll", 30}, {"video.drone_distance", 50},
};
inline int TunedDefault_Or(const char *key, int fallback)
{
    for (const TunedDefault &d : kTunedDefaults)
        if (std::strcmp(d.key, key) == 0) return d.value;
    return fallback;
}
