// Windshield dirt map maths (game/WindshieldDirt.h): spatter locality and
// glass masking, wetness ramp, dry vs wet wipe behaviour, decay,
// determinism. Pure CPU, no renderer.
#include "WindshieldDirt.h"
#include "WindshieldGlassFrame.h"
#include <cstdio>
#include <cstring>
#include <cmath>

static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)

int main()
{
    // Spatter: dirt lands only inside the glass mask, localised.
    WindshieldMap m;
    Windshield_Reset(&m, 42);
    Windshield_AddSpatter(&m, 24, 200, 0.5f);
    CHECK(Windshield_TotalDirt(&m) > 0);
    for (int y = 0; y < WINDSHIELD_MAP_H; y++)
        for (int x = 0; x < WINDSHIELD_MAP_W; x++) {
            float u = (x + 0.5f) / WINDSHIELD_MAP_W, v = (y + 0.5f) / WINDSHIELD_MAP_H;
            if (!Windshield_InsideGlass(u, v))
                CHECK(m.dirt[y * WINDSHIELD_MAP_W + x] == 0);
        }
    // Determinism: same seed, same map.
    WindshieldMap m2;
    Windshield_Reset(&m2, 42);
    Windshield_AddSpatter(&m2, 24, 200, 0.5f);
    CHECK(memcmp(m.dirt, m2.dirt, sizeof m.dirt) == 0);

    // Wetness: ramps up fast, down slow, clamps.
    WindshieldMap w;
    Windshield_Reset(&w, 7);
    for (int i = 0; i < 60; i++) Windshield_UpdateWetness(&w, 255);
    CHECK(w.wet == 255);
    for (int i = 0; i < 60; i++) Windshield_UpdateWetness(&w, 0);
    CHECK(w.wet < 255 && w.wet > 0);   // slow drying

    // Dry wipe: the arc clears almost fully, faint residue, no smear deposit.
    WindshieldMap d;
    Windshield_Reset(&d, 9);
    for (int i = 0; i < WINDSHIELD_MAP_W * WINDSHIELD_MAP_H; i++)
        d.dirt[i] = Windshield_InsideGlass((i % WINDSHIELD_MAP_W + 0.5f) / WINDSHIELD_MAP_W,
                                           (i / WINDSHIELD_MAP_W + 0.5f) / WINDSHIELD_MAP_H) ? 200 : 0;
    d.wet = 0;
    int cx = (int)(0.216f * WINDSHIELD_MAP_W), cy = (int)(0.536f * WINDSHIELD_MAP_H);   // on the blade at angle -0.25
    int before = d.dirt[cy * WINDSHIELD_MAP_W + cx];
    int removed = Windshield_Wipe(&d, 0.34f, 1.02f, 0.72f, -0.25f, 0.05f, 1);
    CHECK(before == 200 && removed > 0);
    CHECK(d.dirt[cy * WINDSHIELD_MAP_W + cx] <= 200 / 8 + 1);   // residue <= 12.5%
    // Off-arc cells untouched.
    CHECK(d.dirt[8] == 0 || Windshield_InsideGlass(0.5f, 0.1f));

    // Wet wipe: less removed, and a streak appears at the band rim.
    WindshieldMap s;
    Windshield_Reset(&s, 9);
    for (int i = 0; i < WINDSHIELD_MAP_W * WINDSHIELD_MAP_H; i++)
        s.dirt[i] = Windshield_InsideGlass((i % WINDSHIELD_MAP_W + 0.5f) / WINDSHIELD_MAP_W,
                                           (i / WINDSHIELD_MAP_W + 0.5f) / WINDSHIELD_MAP_H) ? 200 : 0;
    s.wet = 255;
    int removedWet = Windshield_Wipe(&s, 0.34f, 1.02f, 0.72f, -0.25f, 0.05f, 1);
    CHECK(removedWet < removed);   // wet glass keeps more dirt behind
    CHECK(s.dirt[cy * WINDSHIELD_MAP_W + cx] <= 200 / 8 + 1);


    // Glass-local frame: built from the glass vertices, u across Z, v down the
    // glass (top = 0), aspect from the raked surface height over the width.
    {
        // A raked windscreen: 1.2 wide (Z), 0.4 tall (Y), 0.3 deep (X).
        const float quad[12] = { 0.f, 1.0f, -0.6f,   0.f, 1.0f, 0.6f,
                                 0.3f, 0.6f, 0.6f,   0.3f, 0.6f, -0.6f };
        WsGlassFrame f = WsGlassFrame_FromVertices(quad, 4);
        CHECK(f.valid == 1);
        CHECK(std::fabs(f.widthUnits - 1.2f) < 1e-5f);
        CHECK(std::fabs(f.heightUnits - 0.5f) < 1e-5f);   // sqrt(0.4^2 + 0.3^2)
        CHECK(std::fabs(f.aspect - 0.5f / 1.2f) < 1e-5f);
        float u, v;
        WsGlassFrame_UV(&f, 1.0f, -0.6f, &u, &v);   // top edge, one side
        CHECK(std::fabs(u) < 1e-5f && std::fabs(v) < 1e-5f);
        WsGlassFrame_UV(&f, 0.6f, 0.6f, &u, &v);    // bottom edge, other side
        CHECK(std::fabs(u - 1.0f) < 1e-5f && std::fabs(v - 1.0f) < 1e-5f);
        // Every glass vertex maps inside [0,1]^2: dirt never leaves the pane.
        for (int i = 0; i < 4; i++) {
            WsGlassFrame_UV(&f, quad[i * 3 + 1], quad[i * 3 + 2], &u, &v);
            CHECK(u >= -1e-5f && u <= 1.00001f && v >= -1e-5f && v <= 1.00001f);
        }
        // Degenerate glass (no lateral or vertical extent) is rejected.
        const float flat[9] = { 0.f, 1.f, 0.f,  0.f, 1.f, 0.f,  0.f, 1.f, 0.f };
        CHECK(WsGlassFrame_FromVertices(flat, 3).valid == 0);
        CHECK(WsGlassFrame_FromVertices(nullptr, 0).valid == 0);
    }

    // Mask: only the thin rim of the glass bounds is excluded.
    CHECK(Windshield_InsideGlass(0.5f, 0.5f));
    CHECK(Windshield_InsideGlass(0.05f, 0.05f));
    CHECK(!Windshield_InsideGlass(0.0f, 0.5f));
    CHECK(!Windshield_InsideGlass(0.5f, 1.0f));

    // Wiper reach: the blade reaches every glass cell. Regression for the
    // upper-frame blobs the wipers never cleared (the old radius was 0.72 of
    // the glass HEIGHT vertically, ending at v ~ 0.3).
    {
        const float aspect = 0.42f, pivotV = 1.04f;
        for (int b = 0; b < 2; b++) {
            float pivotU = b == 0 ? 0.34f : 0.66f;
            float reach = Windshield_BladeReach(aspect, pivotU, pivotV);
            for (int c = 0; c < 4; c++) {
                float du = (c & 1 ? 1.0f : 0.0f) - pivotU;
                float dv = ((c & 2 ? 1.0f : 0.0f) - pivotV) * aspect;
                CHECK(std::sqrt(du * du + dv * dv) <= reach + 1e-5f);
            }
        }
        // One wide sweep over the full blade range clears the top row on the
        // blade's own side (the old geometry never reached it).
        WindshieldMap a;
        Windshield_Reset(&a, 11);
        for (int i = 0; i < WINDSHIELD_MAP_W * WINDSHIELD_MAP_H; i++) a.dirt[i] = 200;
        a.wet = 0;
        float reach = Windshield_BladeReach(aspect, 0.34f, pivotV);
        int top = 2, col = (int)(0.20f * WINDSHIELD_MAP_W);   // upper frame, left of the left pivot
        CHECK(a.dirt[top * WINDSHIELD_MAP_W + col] == 200);
        Windshield_WipeSector(&a, aspect, 0.34f, pivotV, reach, -1.15f, 0.0f, 0.05f, 1);
        CHECK(a.dirt[top * WINDSHIELD_MAP_W + col] <= 200 / 8 + 1);
        // The old geometry (aspect 1, radius 0.72, pivot 1.02) misses that cell.
        WindshieldMap o;
        Windshield_Reset(&o, 11);
        for (int i = 0; i < WINDSHIELD_MAP_W * WINDSHIELD_MAP_H; i++) o.dirt[i] = 200;
        Windshield_WipeSector(&o, 1.0f, 0.34f, 1.02f, 0.72f, -1.15f, 0.0f, 0.05f, 1);
        CHECK(o.dirt[top * WINDSHIELD_MAP_W + col] == 200);
    }

    // Sector sweep: a fast blade (big angle step between frames) leaves no
    // uncleared stripe between the previous and the current angle.
    {
        WindshieldMap a;
        Windshield_Reset(&a, 13);
        for (int i = 0; i < WINDSHIELD_MAP_W * WINDSHIELD_MAP_H; i++) a.dirt[i] = 200;
        a.wet = 0;
        const float aspect = 0.42f, pivotV = 1.04f;
        float reach = Windshield_BladeReach(aspect, 0.34f, pivotV);
        Windshield_WipeSector(&a, aspect, 0.34f, pivotV, reach, -0.9f, -0.3f, 0.05f, 1);
        // A cell at angle -0.6 (mid-interval, well beyond halfWidth of either end), mid-radius.
        float r = reach * 0.6f, ang = -0.6f;
        float cu = 0.34f + std::sin(ang) * r, cv = pivotV - std::cos(ang) * r / aspect;
        int cx2 = (int)(cu * WINDSHIELD_MAP_W), cy2 = (int)(cv * WINDSHIELD_MAP_H);
        CHECK(cx2 >= 0 && cx2 < WINDSHIELD_MAP_W && cy2 >= 0 && cy2 < WINDSHIELD_MAP_H);
        CHECK(a.dirt[cy2 * WINDSHIELD_MAP_W + cx2] <= 200 / 8 + 1);
    }

    // Decay: a dirty map slowly cleans on the 64-frame sweep.
    WindshieldMap e;
    Windshield_Reset(&e, 3);
    Windshield_AddSpatter(&e, 24, 100, 0.5f);
    int t0 = Windshield_TotalDirt(&e);
    for (int f = 0; f < 64; f++) Windshield_Decay(&e, f);
    CHECK(Windshield_TotalDirt(&e) < t0);

    // Visibility regression: driving spatters at the real rate (11/s, amount 1)
    // must leave a readable map after ~20 s (old model: ~0% of glass >= 25%).
    WindshieldMap v;
    Windshield_Reset(&v, 5);
    int f = 0;
    for (int step = 0; step < 220; step++) {
        Windshield_AddSpatter(&v, 1, 111, 0.604f);
        for (int k = 0; k < 5; k++) Windshield_Decay(&v, f++ * 1);
    }
    int glass = 0, strong = 0;
    for (int y = 0; y < WINDSHIELD_MAP_H; y++)
        for (int x = 0; x < WINDSHIELD_MAP_W; x++)
            if (Windshield_InsideGlass((x + 0.5f) / WINDSHIELD_MAP_W, (y + 0.5f) / WINDSHIELD_MAP_H)) {
                glass++; if (v.dirt[y * WINDSHIELD_MAP_W + x] >= 64) strong++;
            }
    CHECK(strong * 100 >= glass * 5);

    // Broken-glass overlay suppression: the dirt overlay hides only when the
    // resolved front glass is BROKEN; intact, damaged and feature-off keep it.
    CHECK(Windshield_OverlayVisible(-1) == 1);
    CHECK(Windshield_OverlayVisible(0) == 1);
    CHECK(Windshield_OverlayVisible(1) == 1);
    CHECK(Windshield_OverlayVisible(2) == 0);

    // Dry auto wipe hysteresis: idle when clean, start when dirty, park when clean again.
    int cells = WINDSHIELD_MAP_W * WINDSHIELD_MAP_H;
    CHECK(Windshield_DryWipeWantedForTotal(0, 0) == 0);
    CHECK(Windshield_DryWipeWantedForTotal(cells * (kDryWipeStart - 1), 0) == 0);
    CHECK(Windshield_DryWipeWantedForTotal(cells * kDryWipeStart, 0) == 1);
    CHECK(Windshield_DryWipeWantedForTotal(cells * (kDryWipeStop + 1), 1) == 1);
    CHECK(Windshield_DryWipeWantedForTotal(cells * (kDryWipeStop - 1), 1) == 0);

    std::printf(fails ? "FAILED\n" : "windshield dirt ok\n");
    return fails ? 1 : 0;
}
