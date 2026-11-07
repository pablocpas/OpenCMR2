#ifndef OPENCMR2_WINDSHIELD_DIRT_H
#define OPENCMR2_WINDSHIELD_DIRT_H
// Windshield-local dirt and wetness (video.windscreen_dirt /
// video.windscreen_wipers): replaces the flat full-screen tint. Each view owns
// a low-res dirt map over the windscreen region; spatters land as localised
// blobs, rain wets the glass, and the cockpit's real wiper blades (their
// actual sweep angle, see CarInterior_AnimateWipers) clear the arc they pass
// over. Wet dirt smears: part of what the blade lifts is deposited at the
// trailing edge of the swept band as a streak, so wiping a wet screen trades
// a clean arc for smeared edges until the glass dries. Dry dirt lifts almost
// clean with a faint residue. Render-only: nothing here feeds physics, the
// camera, the replay stream or the original wiper logic. The glass-registered
// pane draw lives in WindshieldDirt.cpp / WindshieldGlass.cpp; all maths here is
// unit-tested in tests/windshield_dirt.cpp.
#include <stdint.h>
#include <math.h>

#define WINDSHIELD_MAP_W 96
#define WINDSHIELD_MAP_H 64
#define WINDSHIELD_MAX_VIEWS 8

struct WindshieldMap {
    uint8_t dirt[WINDSHIELD_MAP_W * WINDSHIELD_MAP_H];   // 0 clean .. 255 opaque grime
    uint8_t wet;        // 0 dry .. 255 soaked (ramps with the view's rain state)
    uint32_t rng;       // own LCG: render-only determinism, never the sim's RAND
};

inline void Windshield_Reset(WindshieldMap *m, uint32_t seed)
{
    for (int i = 0; i < WINDSHIELD_MAP_W * WINDSHIELD_MAP_H; i++) m->dirt[i] = 0;
    m->wet = 0;
    m->rng = seed ? seed : 0x9e3779b9u;
}

inline uint32_t Windshield_Rand(WindshieldMap *m)
{
    m->rng = m->rng * 1664525u + 1013904223u;
    return m->rng >> 8;
}

// The windscreen region in map space. The map is glass-local (see
// WindshieldGlassFrame.h): its edges are the bounds of the real front-glass
// mesh, and the pane that shows it is drawn only on the glass triangles. This
// mask only keeps a thin rim of the bounds free so wiped and smeared dirt
// stays off the frame.
inline bool Windshield_InsideGlass(float u, float v)
{
    return u > 0.01f && u < 0.99f && v > 0.01f && v < 0.99f;
}

// One spatter blob: `count` drops at random glass positions, radius scaled by
// size01, strength by amount (0..255 per drop centre, feathered to the rim).
inline void Windshield_AddSpatter(WindshieldMap *m, int count, int strength, float size01)
{
    if (strength <= 0) return;
    float radius = 0.012f + 0.030f * (size01 < 0.0f ? 0.0f : size01 > 1.0f ? 1.0f : size01);
    for (int i = 0; i < count; i++) {
        float cu = (Windshield_Rand(m) & 0xffff) / 65535.0f;
        float cv = (Windshield_Rand(m) & 0xffff) / 65535.0f;
        if (!Windshield_InsideGlass(cu, cv)) continue;
        int cx = (int)(cu * WINDSHIELD_MAP_W), cy = (int)(cv * WINDSHIELD_MAP_H);
        int r = 1 + (int)(radius * WINDSHIELD_MAP_W);
        for (int y = cy - r; y <= cy + r; y++) {
            if (y < 0 || y >= WINDSHIELD_MAP_H) continue;
            for (int x = cx - r; x <= cx + r; x++) {
                if (x < 0 || x >= WINDSHIELD_MAP_W) continue;
                float u = (x + 0.5f) / WINDSHIELD_MAP_W, v = (y + 0.5f) / WINDSHIELD_MAP_H;
                if (!Windshield_InsideGlass(u, v)) continue;
                float d2 = (float)((x - cx) * (x - cx) + (y - cy) * (y - cy));
                float fall = 1.0f - d2 / (float)(r * r + 1);
                if (fall <= 0.0f) continue;
                int add = (int)(strength * fall);
                int cell = m->dirt[y * WINDSHIELD_MAP_W + x] + add;
                m->dirt[y * WINDSHIELD_MAP_W + x] = (uint8_t)(cell > 255 ? 255 : cell);
            }
        }
    }
}

// Wetness ramps toward target (0..255) - quick to wet, slow to dry.
inline void Windshield_UpdateWetness(WindshieldMap *m, int target)
{
    if (target < 0) target = 0; if (target > 255) target = 255;
    if (m->wet < target) m->wet = (uint8_t)((m->wet + 8 > target) ? target : m->wet + 8);
    else if (m->wet > target) m->wet = (uint8_t)((m->wet - 1 < target) ? target : m->wet - 1);
}

// Slow universal fade (evaporation, vibration): a clean screen emerges over
// minutes even without wipers. Called per rendered frame.
inline void Windshield_Decay(WindshieldMap *m, int frameCounter)
{
    if ((frameCounter & 0x0f) != 0) return;   // one cell-sweep every 16 frames
    for (int i = 0; i < WINDSHIELD_MAP_W * WINDSHIELD_MAP_H; i++)
        if (m->dirt[i] > 0) m->dirt[i]--;
}

// Reach of a blade pivoted at (pivotU, pivotV): the farthest glass corner, in
// width units (v scaled by aspect = glass height / width). A blade this long
// reaches every cell of the glass.
inline float Windshield_BladeReach(float aspect, float pivotU, float pivotV)
{
    float best = 0.0f;
    for (int c = 0; c < 4; c++) {
        float du = (c & 1 ? 1.0f : 0.0f) - pivotU;
        float dv = ((c & 2 ? 1.0f : 0.0f) - pivotV) * aspect;
        float d = sqrtf(du * du + dv * dv);
        if (d > best) best = d;
    }
    return best;
}

// One blade sweep over the angle interval [aMin, aMax] (radians from straight
// up, + toward screen right), widened by `halfWidth` each side. Passing the
// previous and current blade angle clears everything the blade crossed between
// two frames, so a fast sweep leaves no stripes. The map is glass-local and
// not square: distances are measured in width units, so `aspect` (glass
// height / width) converts the v axis. `radius` is in width units. Wet glass
// smears: `lifted` dirt is partly moved to the trailing rim of the band
// instead of deleted (trailing = opposite the sweep direction `dir`, +1/-1).
// Dry glass keeps a faint residue. Returns the total dirt removed.
inline int Windshield_WipeSector(WindshieldMap *m, float aspect, float pivotU, float pivotV,
                                 float radius, float aMin, float aMax, float halfWidth, int dir)
{
    int removed = 0;
    float r2 = radius * radius;
    float inner = radius * 0.25f;   // blades do not reach the pivot
    float lo = aMin - halfWidth, hi = aMax + halfWidth;
    float angle = 0.5f * (aMin + aMax);
    for (int y = 0; y < WINDSHIELD_MAP_H; y++) {
        for (int x = 0; x < WINDSHIELD_MAP_W; x++) {
            float u = (x + 0.5f) / WINDSHIELD_MAP_W, v = (y + 0.5f) / WINDSHIELD_MAP_H;
            if (!Windshield_InsideGlass(u, v)) continue;
            float du = u - pivotU, dv = (v - pivotV) * aspect;
            float d2 = du * du + dv * dv;
            if (d2 > r2 || d2 < inner * inner) continue;
            float cellAngle = atan2f(du, -dv);   // 0 = straight up, + = screen right
            if (cellAngle < lo || cellAngle > hi) continue;
            int idx = y * WINDSHIELD_MAP_W + x;
            int before = m->dirt[idx];
            if (before == 0) continue;
            // Wetness decides the split: lifted-and-gone vs smeared to the rim.
            int smear = before * m->wet / 512;          // up to half when soaked
            int residue = before * (255 - m->wet) / 2048;  // faint film when dry
            int gone = before - smear - residue;
            if (gone < 0) gone = 0;
            m->dirt[idx] = (uint8_t)residue;
            removed += gone;
            if (smear > 0) {
                // Deposit on the trailing rim: a step beyond the band edge on
                // the side the blade came from.
                float rimAngle = angle + (dir > 0 ? -(hi - angle) : (hi - angle)) * 1.6f;
                float rr = sqrtf(d2);
                float su = pivotU + sinf(rimAngle) * rr, sv = pivotV - cosf(rimAngle) * rr / aspect;
                int sx = (int)(su * WINDSHIELD_MAP_W), sy = (int)(sv * WINDSHIELD_MAP_H);
                if (sx >= 0 && sx < WINDSHIELD_MAP_W && sy >= 0 && sy < WINDSHIELD_MAP_H &&
                    Windshield_InsideGlass((sx + 0.5f) / WINDSHIELD_MAP_W, (sy + 0.5f) / WINDSHIELD_MAP_H)) {
                    int tidx = sy * WINDSHIELD_MAP_W + sx;
                    int t = m->dirt[tidx] + smear;
                    m->dirt[tidx] = (uint8_t)(t > 255 ? 255 : t);
                }
            }
        }
    }
    return removed;
}

// Single-angle blade pass on a square map (aspect 1), the original form.
inline int Windshield_Wipe(WindshieldMap *m, float pivotU, float pivotV, float radius,
                           float angle, float halfWidth, int dir)
{
    return Windshield_WipeSector(m, 1.0f, pivotU, pivotV, radius, angle, angle, halfWidth, dir);
}

// Broken-glass overlay suppression (video.windshield_cracks): when the car's
// resolved front glass is BROKEN (state 2), the dirt overlay hides so the
// real swapped broken-glass texture shows without a dirt film floating over
// its holes. Intact and damaged glass keep the full dirt/wiper behaviour.
inline int Windshield_OverlayVisible(int glassState)
{
    return glassState != 2;
}

// Cracked-glass dirt fade (video.windshield_dirt_fade_cracked, default 0):
// cracked (state 1) glass can draw the dirt film at half strength, so the
// crack reads as damaging the view, not just retexturing it. Off keeps the
// original full-strength dirt on cracked glass. Broken (state 2) always
// hides the overlay (see Windshield_OverlayVisible).
inline int Windshield_DirtAlphaScale(int glassState, int fadeCracked)
{
    if (glassState == 2) return 0;
    if (glassState == 1 && fadeCracked) return 128;
    return 255;
}

// Dry-weather auto wipe (hysteresis). The blades used to sweep constantly when
// dry, which kept the swept (lower) glass permanently clean. Now a dry sweep
// starts only once the mean dirt per map cell passes kDryWipeStart, and the
// blades park (after finishing the sweep) once it falls below kDryWipeStop.
static const int kDryWipeStart = 12;
static const int kDryWipeStop = 3;
inline int Windshield_DryWipeWantedForTotal(int totalDirt, int running)
{
    int mean = totalDirt / (WINDSHIELD_MAP_W * WINDSHIELD_MAP_H);
    return running ? mean >= kDryWipeStop : mean >= kDryWipeStart;
}

// Total dirt (for "is it worth drawing" and the F10 line).
inline int Windshield_TotalDirt(const WindshieldMap *m)
{
    int total = 0;
    for (int i = 0; i < WINDSHIELD_MAP_W * WINDSHIELD_MAP_H; i++) total += m->dirt[i];
    return total;
}

#endif
