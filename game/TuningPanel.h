#ifndef OPENCMR2_TUNING_PANEL_H
#define OPENCMR2_TUNING_PANEL_H
#include <cstring>
// Live, in-race tuning of the visual effects (no physics). Pure row maths; the key handling and drawing live in RallyData.cpp.
struct TuningRow { const char *label, *key; int lo, hi, step, fallback; };
// RallyData.cpp sizes its tuning snapshot/preset arrays by this bound; keep the row
// list below it (pinned in tests/tuning_panel.cpp). Rows outgrowing 32 overflowed those
// arrays: stack smashing abort on the drone lock toggle.
constexpr int kTuningMaxRows = 64;
inline const TuningRow *Tuning_Rows(int &count)
{
    static const TuningRow rows[] = {
        {"Speed FOV %", "video.speed_fov", 0, 500, 5, 100},
        {"FOV dolly % (camera closer when wider)", "video.fov_dolly", 0, 250, 10, 150},
        {"Cockpit eye shift % (sign tuned natively)", "video.cockpit_eye_shift", -100, 100, 10, 0},
        {"DoF (0/1)", "video.depth_of_field", 0, 1, 1, 0},
        {"DoF blur %", "video.dof_blur", 0, 100, 10, 100},
        {"DoF max px at 100%", "video.dof_blur_max_px", 1, 40, 2, 16},
        {"Blur mode (0 peripheral, 1 whole scene minus car)", "video.blur_mode", 0, 1, 1, 0},
        {"Tunnel vision %", "video.tunnel_vision", 0, 200, 10, 130},
        {"Radial lines inside the tunnel vision zone (0/1)", "video.tunnel_lines", 0, 1, 1, 1},
        {"Tunnel road tracking, cockpit (0/1)", "video.tunnel_track", 0, 1, 1, 1},
        {"Motion blur (0/1)", "video.motion_blur", 0, 1, 1, 0},
        {"Motion blur %", "video.motion_blur_amount", 0, 100, 10, 60},
        {"Vapor trails behind car (0/1)", "video.vapor_trails", 0, 1, 1, 1},
        {"Radial speed lines (0/1)", "video.speed_lines", 0, 1, 1, 0},
        {"Speed streak %", "video.speed_streak", 0, 100, 10, 100},
        {"FOV preview, video.fov number (0 off)", "video.fov_preview", 0, 150, 5, 0},   // absolute video.fov, debug only, a legacy scale number not an angle, below 20 acts as 20; Speed FOV % is separate
        {"Tyre debris (0 off; chips per wheel per step x100, loose/wet ground)", "video.tire_debris", 0, 2000, 25, 575},
        {"Dust/snow plume amount % (rate and cloud cap)", "video.dust_plume_amount", 0, 300, 25, 300},
        {"Wheel spray amount % (wet/loose ground)", "video.spray_amount", 0, 300, 25, 300},
        {"Exhaust smoke amount %", "video.exhaust_smoke_amount", 0, 300, 25, 300},
        {"Wheel splash amount % (standing water)", "video.splash_amount", 0, 300, 25, 300},
        {"Backfire sparks amount %", "video.backfire_amount", 0, 300, 25, 300},
        {"Rain/snow amount % (0 hides; port sidecar, original pool untouched)", "video.weather_particle_amount", 0, 300, 25, 200},
        {"Cockpit speed FOV % of Speed FOV (cockpit only)", "video.speed_fov_cockpit", 0, 100, 5, 25},
        {"Active camera forward(+)/back(-) at full speed, 0.05 units", "video.camera_move_active", -200, 200, 5, 0},   // special row: edits video.camera_move_type<N> of the camera in use; RallyData.cpp resolves it
        {"Sun glare %", "video.sun_glare", 0, 100, 5, 60},
        {"Fog (0/1)", "video.fog", 0, 1, 1, 1},
        {"Fog amount % (100 = deliberately heavy)", "video.fog_amount", 0, 100, 10, 60},
        {"Fog sun glow %", "video.fog_glow", 0, 100, 10, 100},
        {"Fog near clear dist, world units (0 off; global, not road-only)", "video.fog_clear", 0, 200, 10, 60},
        {"Fog draw cull (0/1, experimental, unmeasured)", "video.fog_cull", 0, 1, 1, 1},
        {"Fog spatial pockets (0/1)", "video.fog_spatial", 0, 1, 1, 1},
        {"Fog evolve (100 neutral; < clears, > thickens)", "video.fog_evolve", 0, 200, 10, 60},
        {"Air motes (0/1)", "video.air_motes", 0, 1, 1, 1},
        {"Air motes amount %", "video.air_motes_amount", 0, 500, 25, 500},
        {"Fog light response %", "video.fog_lights", 0, 200, 10, 40},
        {"Snow glints %", "video.snow_glint", 0, 200, 10, 100},
        {"Corner tilt % (up to four degrees)", "video.camera_roll", 0, 100, 5, 30},
        {"Drone preset locked (unlock to tune)", "video.drone_locked", 0, 1, 1, 0},
        {"Drone distance %", "video.drone_distance", 10, 300, 5, 50},
        {"Drone height %", "video.drone_height", 25, 400, 5, 100},
        {"Drone lateral follow % (lower floats more)", "video.drone_lateral", 10, 400, 5, 100},
        {"Drone accel/brake spring %", "video.drone_spring", 1, 400, 5, 100},
        {"Drone speed height drop %", "video.drone_speed_drop", 0, 500, 5, 100},
        {"Brush wall depth (0-100)", "video.brush_relief", 0, 100, 5, 100},
    };
    count = (int)(sizeof(rows) / sizeof(*rows));
    return rows;
}
inline bool Tuning_DroneParameter(const TuningRow &r){return std::strncmp(r.key,"video.drone_",12)==0&&std::strcmp(r.key,"video.drone_locked")!=0;}
// One step in direction dir (+1/-1), clamped to the row range. A value already outside the range snaps in.
inline int Tuning_Step(const TuningRow &row, int value, int dir)
{
    long v = (long)value + (long)dir * row.step;
    if (v < row.lo) v = row.lo;
    if (v > row.hi) v = row.hi;
    return (int)v;
}
// Row selection wraps around.
inline int Tuning_Select(int index, int dir, int count) { return count <= 0 ? 0 : ((index + dir) % count + count) % count; }
// Physical-key scancodes (DirectInput numbering, the same on every keyboard layout): the digit row 1..7 and F10.
// The old [ ] keys were the default gear shifts, so they are not used. While the panel is open, its keys are removed from
// the keyboard state the game reads, so they never also drive the car or a menu.
static const int kTuningToggleDefault = 0x44;                       // F10
static const int kTuningKeys[7] = {0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}; // 1 prev row, 2 next row, 3 down, 4 up, 5 reset row, 6 reset all, 7 save
// A toggle key is refused when it is a default driving/menu key (arrows, space, [ ], C, numpad arrows, Return, Escape, F1, F2),
// one of the tuning digit keys, or outside 1..255. The caller also refuses keys bound to driving actions in the player's controls.
inline bool Tuning_ToggleKeyAllowed(int key)
{
    if (key < 1 || key > 255) return false;
    static const int unsafe[] = {0x01, 0x1c, 0x9c, 0x39, 0x1a, 0x1b, 0x2e, 0x3b, 0x3c, 0xc8, 0xcb, 0xcd, 0xd0, 0x4b, 0x4d, 0x48, 0x50};
    for (int u : unsafe) if (key == u) return false;
    for (int t : kTuningKeys) if (key == t) return false;
    return true;
}
inline void Tuning_FilterKeys(unsigned char keys[256], bool open, int toggleKey)
{
    if (toggleKey > 0 && toggleKey < 256) keys[toggleKey] = 0;
    if (open)
        for (int k : kTuningKeys) keys[k] = 0;
}
// Edge detector: true once per press (key went from up to down), never while held.
struct TuningEdges {
    bool prev[8] = {};
    bool Edge(int slot, bool down) { bool e = down && !prev[slot]; prev[slot] = down; return e; }
};
// Hold-to-repeat for the row and value keys: one trigger on the press, then after kTuningRepeatDelayMs one trigger every
// kTuningRepeatIntervalMs while the key stays down. Time is wall-clock milliseconds, so it behaves the same in a race tick and a pause tick.
static const double kTuningRepeatDelayMs = 350.0, kTuningRepeatIntervalMs = 60.0;
struct TuningRepeat {
    bool held[8] = {};
    double nextMs[8] = {};
    bool Fire(int slot, bool down, double nowMs)
    {
        if (!down) { held[slot] = false; return false; }
        if (!held[slot]) { held[slot] = true; nextMs[slot] = nowMs + kTuningRepeatDelayMs; return true; }
        if (nowMs >= nextMs[slot]) { nextMs[slot] = nowMs + kTuningRepeatIntervalMs; return true; }
        return false;
    }
};
#endif
