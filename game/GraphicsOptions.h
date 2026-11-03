#ifndef _GRAPHICS_OPTIONS_H
#define _GRAPHICS_OPTIONS_H

// OpenCMR2: pure helpers behind the advanced graphics page's rows: the user
// draw distance ([video] video.draw_distance) and the texture filtering level
// ([video] video.anisotropy). Both are saved in opencmr2.ini, not in the save
// game's graphics bits. Kept free of game and renderer includes so the tests
// can exercise the mappings alone.

#include "FixedPoint.h"
#include <cstdint>

// User-visible defaults; explicit saved Off still wins.
#define OPENCMR2_DEBUG_DRAW_DEFAULT 1
// Older menu builds saved the selector index (1..3 = 64/128/256, 0 = original)
// instead of the size. No valid size lies in 1..63, so read those as indexes.
inline int ReflectionCube_Normalize(int requested)
{
    static const int sizes[4] = { 0, 64, 128, 256 };
    return requested >= 1 && requested <= 3 ? sizes[requested] : requested;
}

inline int ReflectionCube_Size(int requested, int original)
{
    requested = ReflectionCube_Normalize(requested);
    if (requested <= 0) return original; // preserve native resolution when requested
    if (requested <= 64) return 64;
    if (requested <= 128) return 128;
    return 256; // bounded six-face cost; no unlimited texture allocation
}


struct StageObject;

// The user draw distance as a percent of the stage's far plane, clamped to a
// sane range so the far plane can't be pushed to a useless extreme (a quarter
// of the stage distance up to four times it). 100 is the original distance.
inline int View_ClampDrawDistancePercent(int percent)
{
    return percent < 25 ? 25 : percent > 400 ? 400 : percent;
}

// The draw-distance steps the menu offers, in percent.
#define DRAW_DISTANCE_COUNT 7
inline const int *DrawDistance_Percents(void)
{
    static const int percents[DRAW_DISTANCE_COUNT] = { 25, 50, 75, 100, 125, 150, 200 };
    return percents;
}

// The menu index of a percent (the nearest step offered; the setting itself
// may hold any value) and the percent of an index.
inline int DrawDistance_PercentToIndex(int percent)
{
    const int *percents = DrawDistance_Percents();
    int best = 0;
    int bestDiff = percent - percents[0];
    if (bestDiff < 0)
        bestDiff = -bestDiff;
    for (int i = 1; i < DRAW_DISTANCE_COUNT; i++) {
        int diff = percent - percents[i];
        if (diff < 0)
            diff = -diff;
        if (diff < bestDiff) {
            bestDiff = diff;
            best = i;
        }
    }
    return best;
}

inline int DrawDistance_IndexToPercent(int index)
{
    if (index < 0)
        index = 0;
    if (index > DRAW_DISTANCE_COUNT - 1)
        index = DRAW_DISTANCE_COUNT - 1;
    return DrawDistance_Percents()[index];
}

// The anisotropic filtering levels the menu offers; 1 is off, the original's
// point sampling without anisotropy.
#define ANISOTROPY_LEVEL_COUNT 5
inline const int *Anisotropy_Levels(void)
{
    static const int levels[ANISOTROPY_LEVEL_COUNT] = { 1, 2, 4, 8, 16 };
    return levels;
}

// The menu index of a level (the nearest offered) and the level of an index.
inline int Anisotropy_LevelToIndex(int level)
{
    const int *levels = Anisotropy_Levels();
    int best = 0;
    int bestDiff = level - levels[0];
    if (bestDiff < 0)
        bestDiff = -bestDiff;
    for (int i = 1; i < ANISOTROPY_LEVEL_COUNT; i++) {
        int diff = level - levels[i];
        if (diff < 0)
            diff = -diff;
        if (diff < bestDiff) {
            bestDiff = diff;
            best = i;
        }
    }
    return best;
}

inline int Anisotropy_IndexToLevel(int index)
{
    if (index < 0)
        index = 0;
    if (index > ANISOTROPY_LEVEL_COUNT - 1)
        index = ANISOTROPY_LEVEL_COUNT - 1;
    return Anisotropy_Levels()[index];
}

// Reserved runtime-only flag. No collision code may set or honor this bit
// without a verified breakable-tree classification and contact contract.
// Legacy helper bit in StageObject::flags marking a tree smashed
// by the car. Bit 28 (0x10000000) is unused by the original's flag masks
// (0x8000, 0x4000, 0x2001000). Stages reload with fresh trees, so the bit
// need not be part of the on-disk record semantics.
#define STAGE_OBJECT_BROKEN 0x10000000u

// OpenCMR2: whether a hit at the given car speed (16.16) smashes a breakable
// tree. Pure: no renderer or game state; the caller gates on
// [video] video.tree_break. Below the threshold the tree is solid and the
// crash stops the car; at or above it the tree is smashed and the car drives
// through. Tunable constant: roughly full driving speed.
inline bool TreeBreak_ShouldBreak(int speed)
{
    return speed >= 0x20000;
}

// OpenCMR2: per-object fall state for broken trees. A smashed tree falls
// over slowly instead of vanishing: `angle` grows from 0 to 0x10000 (90 deg)
// as the tree tips to the ground. Keyed by StageObject pointer like the
// animation table. [video] video.tree_break gates the whole system.
struct TreeFallState {
    struct StageObject *pObject;  // key: the stage object this state belongs to
    int angle;       // current fall angle in 16.16 degrees, 0..0x10000
    int angularVel;  // degrees per frame, grows as the tree falls
    int active;      // 1 once falling has started
};

// Table of per-object fall state, defined in Game.cpp.
extern struct TreeFallState *g_treeFallStates;
extern unsigned int g_treeFallStateCount;
void TreeFall_ClearTable(void);

// Advances a tree's fall by one frame: accelerates the angle, clamps at 90
// degrees. Returns the current angle (0 = upright, 0x10000 = fully fallen).
inline int TreeFall_Advance(int *angle, int *angularVel)
{
    if (*angularVel < 0x200)
        *angularVel += 0x10;
    *angle += *angularVel;
    if (*angle > 0x10000)
        *angle = 0x10000;
    return *angle;
}

// Whether a fallen tree should still be drawn (angle < 90 deg).
inline bool TreeFall_ShouldDraw(int angle)
{
    return angle < 0x10000;
}

// OpenCMR2: dirt on the camera lens. A level of 0 is a clean lens; 255 is
// completely obscured. Impacts and off-road driving add dirt, each frame
// decays it. Saved nowhere: a fresh stage starts clean.
// [video] video.camera_effects gates the whole system.
#define LENS_DIRT_MAX 255u

// Adds `amount` of dirt to `level`, clamping at LENS_DIRT_MAX.
inline unsigned int LensDirt_Accumulate(unsigned int level, int amount)
{
    unsigned int sum = level + (unsigned int)amount;
    return sum > LENS_DIRT_MAX ? LENS_DIRT_MAX : sum;
}

// Decays `level` by `amount` per frame, flooring at 0.
inline unsigned int LensDirt_Decay(unsigned int level, int amount)
{
    return level < (unsigned int)amount ? 0 : level - (unsigned int)amount;
}

// Whether the dirt level is worth drawing (any non-zero level shows).
inline bool LensDirt_ShouldDraw(unsigned int level)
{
    return level != 0;
}

// Per-view dirt-on-the-lens levels, defined in RallyData.cpp.
extern unsigned int g_lensDirt[8];

// OpenCMR2: human-operator camera shake. Amplitude grows with car speed and
// impact strength, clamped so the camera never leaves the track. The shake is
// applied as a smoothed per-frame jitter to the chase-camera position so the
// camera drifts like a handheld operator instead of jumping randomly.
// [video] video.camera_effects gates it.
#define SHAKE_MAX_AMPLITUDE 0x18000

// Shake amplitude (16.16) from speed and impact level (0..100).
inline int Shake_Amplitude(int speed, int impact)
{
    int amp = FixMul(speed, 0x0c00) + FixMul(impact, 0x0c00);
    return amp > SHAKE_MAX_AMPLITUDE ? SHAKE_MAX_AMPLITUDE : amp;
}

// Smooths a shake direction over several frames so the camera drifts instead
// of teleporting randomly every frame ("Parkinsons"). `target` is the new
// random direction; `current` is the smoothed vector carried between frames.
// Returns the smoothed vector scaled by `amp`.
inline void Shake_Smooth(FixVector *current, const FixVector *target, int amp, int *frameCounter)
{
    (*frameCounter)++;
    if (*frameCounter >= 8) {
        *frameCounter = 0;
        current->x = target->x;
        current->y = target->y;
        current->z = target->z;
    } else {
        // 75% keep previous, 25% move towards target (component-wise).
        FixVecScale(current, current, 0xc000);
        FixVector delta;
        delta.x = target->x - current->x;
        delta.y = target->y - current->y;
        delta.z = target->z - current->z;
        FixVecScale(&delta, &delta, 0x4000);
        current->x += delta.x;
        current->y += delta.y;
        current->z += delta.z;
    }
    FixVecScale(current, current, amp);
}

// OpenCMR2: lens flare brightness from sun visibility (0..100).
#define FLARE_MAX_BRIGHTNESS 0xff

// Flare brightness (0..0xff) from sun visibility, off below a faint threshold.
inline int Flare_Brightness(int sunVisibility)
{
    if (sunVisibility < 10)
        return 0;
    return (sunVisibility * FLARE_MAX_BRIGHTNESS) / 100;
}

// OpenCMR2: animated scenery idle cycles (crowds, trees). A runtime-only flag
// bit in StageObject::flags marks an object as animated; per-object animation
// state lives in a global table keyed by the StageObject pointer (the draw
// path reinterprets the same bytes as a world matrix, so it cannot share the
// object's unused tail). [video] video.animated_scenery gates the whole system.
#define STAGE_OBJECT_ANIMATED 0x40000000u

// Per-object animation state. Kept in a table, not on the object.
struct AnimState {
    struct StageObject *pObject;  // key: the stage object this state belongs to
    int baseTexture;   // first texture of the cycle
    uint8_t frame;     // current frame (0..frameCount-1)
    uint8_t frameCount;// frames in the cycle
    uint8_t timer;     // ticks until the next frame advance
    uint8_t initialized;// 1 once baseTexture/frameCount are set
};

// Table of per-object animation state, defined in Game.cpp. Cleared when a
// stage is unloaded so a fresh stage starts its objects still.
extern struct AnimState *g_animStates;
extern unsigned int g_animStateCount;
int AnimObject_Update(StageObject *object);
void AnimObject_ClearTable(void);

// Advances the animation frame by one tick: decrements the timer, and when it
// reaches zero, wraps the frame within frameCount and resets the timer.
inline int AnimFrame_Advance(int frame, int frameCount, int *timer)
{
    if (!timer) return 0;
    if (frameCount <= 1) { *timer = 5; return 0; }
    (*timer)--;
    if (*timer <= 0) {
        *timer = 5; // ticks per frame (about 1/12 s at 60 fps)
        return (frame + 1) % frameCount;
    }
    return frame;
}

// Animated texture index for an object: base texture + current frame.
inline int AnimTexture_Index(int baseTexture, int frame)
{
    return baseTexture + frame;
}

// OpenCMR2: dirt on the windscreen. A level of 0 is a clean windscreen; 255 is
// completely obscured. Driving in rain/dust/mud adds dirt, wipers clear it.
// [video] video.windscreen_dirt gates accumulation; video.windscreen_wipers
// gates clearing.
#define WINDSCREEN_DIRT_MAX 255u

// Adds `amount` of dirt to `level`, clamping at WINDSCREEN_DIRT_MAX.
inline unsigned int WindscreenDirt_Accumulate(unsigned int level, int amount)
{
    unsigned int sum = level + (unsigned int)amount;
    return sum > WINDSCREEN_DIRT_MAX ? WINDSCREEN_DIRT_MAX : sum;
}

// Decays `level` by `amount` per frame, flooring at 0.
inline unsigned int WindscreenDirt_Decay(unsigned int level, int amount)
{
    return level < (unsigned int)amount ? 0 : level - (unsigned int)amount;
}

// Whether the dirt level is worth drawing (any non-zero level shows).
inline bool WindscreenDirt_ShouldDraw(unsigned int level)
{
    return level != 0;
}

// Per-view dirt-on-the-windscreen levels, defined in RallyData.cpp.
// Superseded for rendering by the windshield-local dirt map below; kept for
// the old pause-scene overlay until that call site migrates.
extern unsigned int g_windscreenDirt[8];

// Windshield-local dirt map (game/WindshieldDirt.{h,cpp}): localised spatter,
// wetness-dependent smearing, and blade-arc clearing driven by the cockpit
// model's real sweep. Cockpit camera (type 3) only; the HUD draws over it.
void Windshield_UpdateView(int view, int slot);
void Windshield_AddDrivingSpatter(int view, int amount);
void Windshield_DrawOverlay(int view, int resX, int resY);
void Windshield_GetDiag(int view, unsigned *calls, unsigned *drawn, int *reason, int *glassState, int *dirtTotal);   // F10 diagnostics only
void Windshield_ResetAll(void);
// Cockpit windshield glass matching the car's real damage appearance
// (video.windshield_cracks): per-car resolve of the front window quad, its
// damage slot/material group and the exterior damaged/broken texture ids, and
// the interior front-glass texture swap. Every failure path keeps the
// feature off for that car. (game/WindshieldGlass.cpp)
int WindshieldGlass_SetupCar(int car);
int WindshieldGlass_ResolveForCar(int car);
void WindshieldGlass_RetryPending(void);
const char *WindshieldGlass_FailReason(int car);   // F10 diagnostics only
int WindshieldGlass_ApplyDamageState(int car, int state);
int WindshieldGlass_DrawCrackPane(int car, int state);
void Windshield_DrawCrackPane(int view);
int WindshieldGlass_GetInfo(int car, int *window, int *slot, unsigned *group,
                            int *damagedTexture, int *brokenTexture);
void WindshieldGlass_RestoreAll(void);
void WindshieldGlass_ResetCache(void);

// OpenCMR2: windscreen wipers. `video.windscreen_wipers` turns them on/off.
// The wiper sweeps back and forth across the screen, clearing dirt as it
// passes. State is per-view, kept in a global table.
#define WIPER_SWEEP_TIME 60 // frames for one full sweep (about 1 second at 60 fps)

// Per-view wiper state.
struct WindscreenWiper {
    uint8_t active;    // 1 if wipers are on
    uint8_t phase;     // sweep phase (0..WIPER_SWEEP_TIME-1)
    uint8_t dir;       // sweep direction: 0 = left-to-right, 1 = right-to-left
};

// Table of per-view wiper state.
extern struct WindscreenWiper g_windscreenWipers[8];

// OpenCMR2: frames remaining in a manually-triggered wiper sweep. When > 0,
// the wiper sweeps once and then stops (unlike the always-on setting).
extern int g_windscreenWiperManualFrames;

// Returns the wiper sweep position (0.0 = left edge, 1.0 = right edge) for
// the given view, advancing the phase by one tick.
inline float WindscreenWiper_Sweep(int view)
{
    struct WindscreenWiper *w = &g_windscreenWipers[view];
    if (!w->active && g_windscreenWiperManualFrames <= 0)
        return -1.0f; // wipers off: no sweep
    if (g_windscreenWiperManualFrames > 0) {
        // Manual sweep triggered by the action button: sweep once, then stop.
        w->active = 1;
        g_windscreenWiperManualFrames--;
        if (g_windscreenWiperManualFrames == 0)
            w->active = 0;
    }
    w->phase++;
    if (w->phase >= WIPER_SWEEP_TIME) {
        w->phase = 0;
        w->dir = w->dir ? 0 : 1; // reverse direction at end of sweep
    }
    float t = (float)w->phase / (float)(WIPER_SWEEP_TIME - 1);
    return w->dir ? t : 1.0f - t;
}


// Clears dirt under the wiper sweep at the given position (0..1 across screen).
inline void WindscreenWiper_ClearDirt(int view, float pos)
{
    // The wiper is a vertical band about 6% of screen width wide.
    float band = 0.06f;
    // Dirt is cleared in a circular/oval region centered on the sweep.
    // We clear a fixed amount proportional to the dirt level.
    unsigned int *level = &g_windscreenDirt[view];
    if (*level == 0)
        return;
    // Clear more when dirt is heavy.
    unsigned int clear = (*level > 128) ? 15 : (*level > 64) ? 10 : 5;
    *level = *level < clear ? 0 : *level - clear;
}

// OpenCMR2: windscreen wiper action button. `video.windscreen_wiper_key`
// holds a DIK physical key code; each rising edge toggles continuous wiping.
// Explicit saved keys are retained. Default W is SDL_SCANCODE_W on every port.
#define WINDSREEN_WIPER_KEY_DEFAULT 0x11  // DIK_W
inline bool WindscreenWiper_Toggle(bool enabled, bool pressed) { return pressed ? !enabled : enabled; }

// Returns 1 when the wiper key was just pressed this frame (edge trigger),
// 0 otherwise. Defined in RallyData.cpp.
int WindscreenWiperKeyPressed(void);
void WindscreenWiper_UpdateInput(void);
void TuningPanel_Update(void);
// Diagnostics only: names the caller of the next TuningPanel_Update (race step, pause tick, Esc menu) for the tuning log lines. No behaviour.
void TuningPanel_NoteSite(const char *site);
// Diagnostics only: the paused scene draw reports itself, once per second at most.
void TuningPanel_NotePausedDraw(const char *what);
void View_RefreshForTuning(void);
void View_GetCameraCarOffset(unsigned view, double *dist, double *fwd, double *side, double *up);
void Race_ApplySpeedEffectParams(void);
void TuningPanel_Draw(void);
void DebugOverlays_DrawLate(void);
void DebugOverlays_RequestView(int view);
bool TuningPanel_IsOpen(void);
bool TuningPanel_PreviewActive(void);
bool TuningPanel_InfoShown(void);
bool TuningPanel_FpsOnly(void);
int TuningPanel_ToggleKey(void);

// OpenCMR2: motion blur between frames. `video.motion_blur` is 0 (off) to
// MOTION_BLUR_MAX; the mix factor is how much of the previous frame to blend
// into the current one. A value of 0 disables it; the useful range is small
// (a few tens) before ghosting dominates. [video] video.motion_blur gates it.
#define MOTION_BLUR_MAX 100

// Blend factor (0..255) of the previous frame into the current one, from the
// user's strength setting. 0 means no blur; MOTION_BLUR_MAX maps to 255.
inline int MotionBlur_Mix(int strength)
{
    if (strength <= 0)
        return 0;
    if (strength >= MOTION_BLUR_MAX)
        return 255;
    return (strength * 255) / MOTION_BLUR_MAX;
}

// OpenCMR2: bloom intensity. `video.bloom` is 0 (off) to BLOOM_MAX; the mix
// factor scales how strongly the blurred bright pixels are added back.
// [video] video.bloom gates it.
#define BLOOM_MAX 100

// Blend factor (0..255) of the bloom layer into the scene, from the user's
// intensity setting. 0 means no bloom; BLOOM_MAX maps to 255.
inline int Bloom_Intensity(int strength)
{
    if (strength <= 0)
        return 0;
    if (strength >= BLOOM_MAX)
        return 255;
    return (strength * 255) / BLOOM_MAX;
}

// OpenCMR2: directional shadow depth bias. `video.shadows` is 0 (off) or 1
// (on); `bias` (0..100) is converted to a depth bias (hundredths of a depth
// unit). [video] video.shadows gates it.
#define SHADOW_BIAS_MAX 100

// Bias (0..100) clamped to the valid range.
inline int Shadow_Bias(int bias)
{
    if (bias < 0)
        return 0;
    if (bias > SHADOW_BIAS_MAX)
        return SHADOW_BIAS_MAX;
    return bias;
}

#endif
