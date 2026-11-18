// Brake disc glow heat model (game/BrakeGlow.h): braking at speed heats the discs,
// the front axle follows the brake bias, coasting and airflow cool them, and heat
// stays clamped. Visual-only feature; this pins the decision logic.
#include "BrakeGlow.h"
#include <cstdio>
#include <cmath>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main() {
    // 1. Braking at speed heats every wheel.
    { float h[4] = {}; for (int i = 0; i < 50; i++) BrakeGlow_UpdateHeat(h, 65536, 39321, 78643, 1.f);
      for (int w = 0; w < 4; w++) CHECK(h[w] > 0.5f); }
    // 2. Front bias heats the front axle more than the rear.
    { float h[4] = {}; for (int i = 0; i < 40; i++) BrakeGlow_UpdateHeat(h, 65536, 39321, 78643, 1.f);
      CHECK(h[0] > h[2] + 0.1f && h[1] > h[3] + 0.1f); }
    // 3. Reversed bias flips the hotter axle.
    { float h[4] = {}; for (int i = 0; i < 40; i++) BrakeGlow_UpdateHeat(h, 65536, 26214, 78643, 1.f);
      CHECK(h[2] > h[0] + 0.1f && h[3] > h[1] + 0.1f); }
    // 4. Even bias keeps the axles level.
    { float h[4] = {}; for (int i = 0; i < 40; i++) BrakeGlow_UpdateHeat(h, 65536, 32768, 78643, 1.f);
      CHECK(std::fabs(h[0] - h[2]) < 0.02f); }
    // 5. Releasing the pedal cools back down; speed cools faster than standing.
    { float h[4] = {}; for (int i = 0; i < 60; i++) BrakeGlow_UpdateHeat(h, 65536, 39321, 78643, 1.f);
      float parked[4] = { h[0], h[1], h[2], h[3] }, moving[4] = { h[0], h[1], h[2], h[3] };
      for (int i = 0; i < 10; i++) { BrakeGlow_UpdateHeat(parked, 0, 39321, 0, 1.f); BrakeGlow_UpdateHeat(moving, 0, 39321, 98304, 1.f); }
      CHECK(moving[0] < parked[0]);
      for (int i = 0; i < 400; i++) BrakeGlow_UpdateHeat(h, 0, 39321, 78643, 1.f);
      for (int w = 0; w < 4; w++) CHECK(h[w] == 0.f); }
    // 6. Heat never exceeds 1 under endless braking, and never goes negative.
    { float h[4] = {}; for (int i = 0; i < 2000; i++) BrakeGlow_UpdateHeat(h, 131072, 65536, 114688, 1.f);
      for (int w = 0; w < 4; w++) CHECK(h[w] <= 1.f && h[w] >= 0.f); }
    // 7. No pedal, no heat, from cold.
    { float h[4] = {}; for (int i = 0; i < 100; i++) BrakeGlow_UpdateHeat(h, 0, 39321, 114688, 1.f);
      for (int w = 0; w < 4; w++) CHECK(h[w] == 0.f); }
    // 8. Same braking heats less at walking pace than at speed.
    { float slow[4] = {}, fast[4] = {};
      for (int i = 0; i < 40; i++) { BrakeGlow_UpdateHeat(slow, 65536, 39321, 13107, 1.f); BrakeGlow_UpdateHeat(fast, 65536, 39321, 98304, 1.f); }
      CHECK(fast[0] > slow[0]); }
    // 9a. At a real top-speed brake the discs reach visible heat within a second.
    { float h[4] = {}; for (int i = 0; i < 25; i++) BrakeGlow_UpdateHeat(h, 65536, 39321, 98304, 1.f);
      CHECK(h[0] > BRAKE_GLOW_THRESHOLD && h[0] > 0.4f); }
    // 9b. Standing on the pedal at a standstill barely heats (no kinetic energy).
    { float h[4] = {}; for (int i = 0; i < 100; i++) BrakeGlow_UpdateHeat(h, 65536, 39321, 0, 1.f);
      CHECK(h[0] < 0.3f); }
    // 9. Half the timestep, less heat per call (frame-rate independence).
    { float a[4] = {}, b[4] = {};
      BrakeGlow_UpdateHeat(a, 65536, 39321, 78643, 1.f); BrakeGlow_UpdateHeat(b, 65536, 39321, 78643, 0.5f);
      CHECK(b[0] < a[0] && b[0] > 0.f); }
    // 10. A cold disc draws nothing; just past the threshold it draws.
    { float p[4][3], uv[4][2], a;
      float ident[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
      CHECK(BrakeGlow_BuildDiscQuad(ident, BRAKE_GLOW_RADIUS, 0.f, p, uv, &a) == 0);
      CHECK(BrakeGlow_BuildDiscQuad(ident, BRAKE_GLOW_RADIUS, BRAKE_GLOW_THRESHOLD, p, uv, &a) == 0);
      CHECK(BrakeGlow_BuildDiscQuad(ident, BRAKE_GLOW_RADIUS, 0.5f, p, uv, &a) == 1); }
    // 11. Hot quad: centred on the wheel position, lies in the up/forward plane
    // (zero extent along the axle/right axis), spans the radius, uv covers 0..1.
    { float p[4][3], uv[4][2], a;
      float world[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 10.f,0.35f,-4.f,1 };
      CHECK(BrakeGlow_BuildDiscQuad(world, BRAKE_GLOW_RADIUS, 0.6f, p, uv, &a) == 1);
      CHECK(std::fabs(a - 0.6f) < 1e-6f);
      float cx = 0, cy = 0, cz = 0;
      for (int v = 0; v < 4; v++) { cx += p[v][0]; cy += p[v][1]; cz += p[v][2];
        CHECK(std::fabs(p[v][0] - 10.f) < 1e-4f);              // axle axis: no extent
        CHECK(std::fabs(std::fabs(p[v][1] - 0.35f) - BRAKE_GLOW_RADIUS) < 1e-4f);
        CHECK(std::fabs(std::fabs(p[v][2] + 4.f) - BRAKE_GLOW_RADIUS) < 1e-4f); }
      CHECK(std::fabs(cx / 4 - 10.f) < 1e-4f && std::fabs(cy / 4 - 0.35f) < 1e-4f && std::fabs(cz / 4 + 4.f) < 1e-4f);
      CHECK(uv[0][0] == 0.f && uv[0][1] == 0.f && uv[2][0] == 1.f && uv[2][1] == 1.f); }
    // 12. Rotated wheel (steered): the quad follows the node's forward axis and
    // still has no extent along its right (axle) axis.
    { float p[4][3], uv[4][2], a;
      float world[16] = { 0,0,-1,0, 0,1,0,0, 1,0,0,0, 1.f,0.3f,2.f,1 };  // right=-z, forward=+x
      CHECK(BrakeGlow_BuildDiscQuad(world, BRAKE_GLOW_RADIUS, 0.9f, p, uv, &a) == 1);
      for (int v = 0; v < 4; v++) {
        float axle = (p[v][0]-1.f)*0.f + (p[v][1]-0.3f)*0.f + (p[v][2]-2.f)*(-1.f);  // dot with right
        CHECK(std::fabs(axle) < 1e-4f);
        CHECK(std::fabs(std::fabs(p[v][0] - 1.f) - BRAKE_GLOW_RADIUS) < 1e-4f); } }
    // 13. Disc radius scales off the measured wheel extent and clamps: a Subaru-sized
    // wheel (~0.31 tyre radius) gets a disc inside the hubcap, huge and tiny meshes
    // stay in sane bounds.
    { float r = BrakeGlow_DiscRadius(0.31f);
      CHECK(r > 0.14f && r < 0.20f);
      CHECK(BrakeGlow_DiscRadius(1.0f) == 0.30f);
      CHECK(BrakeGlow_DiscRadius(0.1f) == 0.10f);
      // the quad built with that radius never spans past 60% of the wheel
      float p4[4][3], uv4[4][2], a4;
      float ident[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
      CHECK(BrakeGlow_BuildDiscQuad(ident, BrakeGlow_DiscRadius(0.31f), 0.5f, p4, uv4, &a4) == 1);
      for (int v = 0; v < 4; v++)
        CHECK(std::fabs(p4[v][1]) <= 0.6f * 0.31f + 1e-4f && std::fabs(p4[v][2]) <= 0.6f * 0.31f + 1e-4f); }
    // 14. Mesh extent: half the largest bounding-box dimension of a raw vertex
    // stream (the engine's single 0x30 layout, position first), robust to the
    // mesh origin sitting away from the hub; unreadable streams return 0.
    { struct V { float p[3]; float rest[9]; } tyre[8] = {};   // 0x30 stride
      // wheel 0.62 across in z, offset from the origin: z in [-0.31, 0.31],
      // y in [-0.05, 0.57] (also 0.62), x in [0.10, 0.30] -> extent 0.31
      for (int i = 0; i < 8; i++) {
        tyre[i].p[0] = (i & 1) ? 0.30f : 0.10f;
        tyre[i].p[1] = (i & 2) ? 0.57f : -0.05f;
        tyre[i].p[2] = (i & 4) ? 0.31f : -0.31f; }
      CHECK(std::fabs(BrakeGlow_MeshExtent(tyre, 0x30, 8) - 0.31f) < 1e-6f);
      // y narrower than z: z wins
      tyre[0].p[2] = 0.9f;
      CHECK(std::fabs(BrakeGlow_MeshExtent(tyre, 0x30, 8) - 0.605f) < 1e-6f);
      CHECK(BrakeGlow_MeshExtent(0, 0x30, 8) == 0.f);
      CHECK(BrakeGlow_MeshExtent(tyre, 0x30, 0) == 0.f);
      CHECK(BrakeGlow_MeshExtent(tyre, 0x30, -3) == 0.f);
      CHECK(BrakeGlow_MeshExtent(tyre, 0x30, 100001) == 0.f);
      CHECK(BrakeGlow_MeshExtent(tyre, 8, 8) == 0.f); }   // stride too small for xyz
    // 15. A wheel whose mesh is unreadable (extent 0) borrows the largest
    // measured sibling extent - the four wheels of a car share a tyre radius,
    // so any readable sibling beats the oversized fixed fallback. All-zero
    // stays zero, measured wheels are untouched.
    { float e[4] = { 0.31f, 0.f, 0.30f, 0.f };
      BrakeGlow_FillMissingExtents(e);
      CHECK(e[0] == 0.31f && e[1] == 0.31f && e[2] == 0.30f && e[3] == 0.31f);
      float z[4] = {};
      BrakeGlow_FillMissingExtents(z);
      for (int w = 0; w < 4; w++) CHECK(z[w] == 0.f); }
    std::puts("brake glow heat model: pass");
    return 0;
}
