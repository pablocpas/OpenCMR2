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
// camera, the replay stream or the original wiper logic. The 2D overlay draw
// lives in WindshieldDirt.cpp; all maths here is unit-tested in
// tests/windshield_dirt.cpp.
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

// The windscreen region in map space: above the dash line, inside the A
// pillars. Fractions of the map; the dash band and pillar margins take no
// dirt and are never drawn. A coarse approximation of the real glass opening
// (exact glass-poly mapping is a native-asset follow-up).
inline bool Windshield_InsideGlass(float u, float v)
{
    if (v < 0.04f || v > 0.66f) return false;             // header rail / dash line
    float halfWidth = 0.46f - 0.10f * (v - 0.04f);        // pillars taper inward downward
    float du = u - 0.5f; if (du < 0.0f) du = -du;
    return du < halfWidth;
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

// One blade pass: clears the band of radius `radius` (map units, uv) around
// the pivot at angle `angle` (radians from straight up, + toward driver-side
// screen right), blade half-width `halfWidth` radians. Wet glass smears:
// `lifted` dirt is partly moved to the trailing rim of the band instead of
// deleted (trailing = opposite the sweep direction `dir`, +1/-1). Dry glass
// keeps a faint residue. Returns the total dirt removed (diagnostics).
inline int Windshield_Wipe(WindshieldMap *m, float pivotU, float pivotV, float radius,
                           float angle, float halfWidth, int dir)
{
    int removed = 0;
    float r2 = radius * radius;
    float inner = radius * 0.25f;   // blades do not reach the pivot
    for (int y = 0; y < WINDSHIELD_MAP_H; y++) {
        for (int x = 0; x < WINDSHIELD_MAP_W; x++) {
            float u = (x + 0.5f) / WINDSHIELD_MAP_W, v = (y + 0.5f) / WINDSHIELD_MAP_H;
            if (!Windshield_InsideGlass(u, v)) continue;
            float du = u - pivotU, dv = v - pivotV;
            float d2 = du * du + dv * dv;
            if (d2 > r2 || d2 < inner * inner) continue;
            float cellAngle = atan2f(du, -dv);   // 0 = straight up, + = screen right
            float rel = cellAngle - angle;
            if (rel < 0.0f) rel = -rel;
            if (rel > halfWidth) continue;
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
                float rimAngle = angle + (dir > 0 ? -halfWidth : halfWidth) * 1.6f;
                float rr = sqrtf(d2);
                float su = pivotU + sinf(rimAngle) * rr, sv = pivotV - cosf(rimAngle) * rr;
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

// Broken-glass overlay suppression (video.windshield_cracks): when the car's
// resolved front glass is BROKEN (state 2), the dirt overlay hides so the
// real swapped broken-glass texture shows without a dirt film floating over
// its holes. Intact and damaged glass keep the full dirt/wiper behaviour.
// Limitation: the fragments that remain in a broken windscreen also lose
// their dirt until the overlay is anchored to the glass geometry.
inline int Windshield_OverlayVisible(int glassState)
{
    return glassState != 2;
}

// Total dirt (for "is it worth drawing" and the F10 line).
inline int Windshield_TotalDirt(const WindshieldMap *m)
{
    int total = 0;
    for (int i = 0; i < WINDSHIELD_MAP_W * WINDSHIELD_MAP_H; i++) total += m->dirt[i];
    return total;
}

#endif
