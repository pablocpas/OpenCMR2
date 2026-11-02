#ifndef OPENCMR2_BRUSH_RELIEF_H
#define OPENCMR2_BRUSH_RELIEF_H
#include <cmath>
#include <vector>
#include "MeshVertex.h"
// Visual-only draw-time depth for Greece1's roadside brush walls (gre01
// temp.obj bsh_sh1a/c/z parts): the source is one flat vertical quad per
// wall segment textured with an alpha-cutout bush sheet, which reads as a
// flat card. Two effects, both derived at draw time - originals, contact,
// collision records and the source perimeter are never edited:
//
// 1. Interior relief (artistic heuristic, NOT recovered geometry): the quad
//    is tessellated and interior vertices displaced along the quad normal by
//    the cutout sheet's ALPHA. Alpha measures cutout coverage, not physical
//    depth, so this only adds shading variance; edge vertices stay exact.
// 2. Back layer ("skybreak"): a second card behind the front one, offset
//    along the inward normal, TALLER, with a shifted UV window of the same
//    verified sheet and darkened. This breaks the straight top silhouette
//    with offset bush peaks and gives real parallax at oblique angles - the
//    cue the flat card lacks. No new artwork: same verified sheet, rewindowed.
//
// video.brush_relief (0..100, default 100) scales the displacement; 0 turns
// the whole path off (caller draws the original flat quad).
namespace BrushRelief {


// Object-local -> world with a row-vector matrix m[16] (Direct3D layout: v' = v*M,
// translation in m[12..14]), the contract of SceneNode::worldF / Gfx world transform.
// temp.obj wall quads are OBJECT-LOCAL meshes, so any comparison with the stage route
// (world space) must go through this. Returns false unless the 3x3 is a rigid rotation
// (unit rows within 5%, orthogonal), so offsets in local units equal world units.
inline bool RigidWorld(const float m[16]) {
    for (int r = 0; r < 3; r++) {
        float l = std::sqrt(m[r*4]*m[r*4] + m[r*4+1]*m[r*4+1] + m[r*4+2]*m[r*4+2]);
        if (!(l > 0.95f && l < 1.05f)) return false;
        for (int q = r + 1; q < 3; q++) {
            float d = m[r*4]*m[q*4] + m[r*4+1]*m[q*4+1] + m[r*4+2]*m[q*4+2];
            if (std::fabs(d) > 0.05f) return false;
        }
    }
    return true;
}
inline void PointToWorld(const float m[16], const float p[3], float o[3]) {
    for (int c = 0; c < 3; c++) o[c] = p[0]*m[c] + p[1]*m[4+c] + p[2]*m[8+c] + m[12+c];
}
inline void DirToWorld(const float m[16], const float d[3], float o[3]) {
    for (int c = 0; c < 3; c++) o[c] = d[0]*m[c] + d[1]*m[4+c] + d[2]*m[8+c];
}
inline void DirToLocal(const float m[16], const float d[3], float o[3]) {   // rigid: inverse = transpose
    for (int r = 0; r < 3; r++) o[r] = d[0]*m[r*4] + d[1]*m[r*4+1] + d[2]*m[r*4+2];
}

// Road-facing side of a wall quad, from the stage route polyline (x,z pairs,
// world units). Returns false when no route point is within maxDist. On
// success away[3] is the unit horizontal direction pointing AWAY from the
// road (the quad's own normal, sign-resolved by which side the nearest route
// point lies on), so extruding along it never reduces drivable room.
inline bool AwayFromRoad(const float *qa /*quad point A xyz*/, const float *qb /*point B xyz*/,
                         const float *normal, const float *routeXZ, int nodes,
                         float maxDist, float away[3]) {
    if (!routeXZ || nodes < 1) return false;
    float mx = (qa[0] + qb[0]) * 0.5f, mz = (qa[2] + qb[2]) * 0.5f;
    float nx = normal[0], nz = normal[2];
    float nl = std::sqrt(nx * nx + nz * nz);
    if (nl < 0.3f) return false;   // quad is not a vertical wall
    nx /= nl; nz /= nl;
    float best = 1e30f, bx = 0, bz = 0;
    for (int i = 0; i < nodes; i++) {
        float ax = routeXZ[i * 2], az = routeXZ[i * 2 + 1];
        float px = ax, pz = az;
        if (i + 1 < nodes) {   // nearest point on the segment, not just the node
            float dx = routeXZ[i * 2 + 2] - ax, dz = routeXZ[i * 2 + 3] - az;
            float l2 = dx * dx + dz * dz;
            if (l2 > 1e-6f) {
                float t = ((mx - ax) * dx + (mz - az) * dz) / l2;
                t = t < 0 ? 0 : t > 1 ? 1 : t;
                px = ax + dx * t; pz = az + dz * t;
            }
        }
        float d = (px - mx) * (px - mx) + (pz - mz) * (pz - mz);
        if (d < best) { best = d; bx = px; bz = pz; }
    }
    if (std::sqrt(best) > maxDist) return false;
    float side = (bx - mx) * nx + (bz - mz) * nz;   // >0: road lies on +n
    if (std::fabs(side) < 0.05f) return false;      // wall on the centreline: ambiguous
    float s = side > 0 ? -1.f : 1.f;
    away[0] = nx * s; away[1] = 0; away[2] = nz * s;
    return true;
}

struct Vertex {
    float x, y, z;          // displaced position
    float u, v;             // sheet UV
    float shade;            // 0..1 multiplicative tint (relief/layer darkening)
    unsigned short tri[3];  // source quad corner barycentric anchor
    float bary[3];
};

constexpr int kGridX = 24;
constexpr int kGridY = 8;
constexpr float kReliefDepth = 0.9f;    // world units at 100%
constexpr float kBackOffset = 0.45f;    // inward normal offset, world units
constexpr float kBackHeight = 1.22f;    // taller back layer breaks the skyline
constexpr float kBackUVShift = 0.31f;   // different bush columns than front
constexpr float kBackShade = 0.7f;
// Extrusion volume (road-facing face fixed): extra cards stacked away from the road.
constexpr int kVolumeSlices = 4;
constexpr float kVolumeStep = 0.8f;     // world units between slices (total 3.2)

inline float SampleAlpha(const unsigned char *alpha, int aw, int ah, float u, float v) {
    if (u < 0) u = 0; if (u > 1) u = 1;
    if (v < 0) v = 0; if (v > 1) v = 1;
    float fx = u * (aw - 1), fy = v * (ah - 1);
    int x0 = (int)fx, y0 = (int)fy;
    int x1 = x0 + 1 < aw ? x0 + 1 : x0, y1 = y0 + 1 < ah ? y0 + 1 : y0;
    float tx = fx - x0, ty = fy - y0;
    float a00 = alpha[y0 * aw + x0] / 255.f, a10 = alpha[y0 * aw + x1] / 255.f;
    float a01 = alpha[y1 * aw + x0] / 255.f, a11 = alpha[y1 * aw + x1] / 255.f;
    return (a00 * (1 - tx) + a10 * tx) * (1 - ty) + (a01 * (1 - tx) + a11 * tx) * ty;
}

// Quad corners in UV-corner order: c[0]=(0,0) c[1]=(1,0) c[2]=(0,1) c[3]=(1,1),
// matching the FixedStrip source-warp contract. normal = unit inward normal.
inline bool Build(const MeshVertexF c[4], const float normal[3],
                  const unsigned char *alpha, int aw, int ah,
                  int reliefPercent, bool backLayer, std::vector<Vertex> &out, int volumeSlices = 0) {
    if (!c || !alpha || aw < 2 || ah < 2) return false;
    out.clear();
    float amount = reliefPercent < 0 ? 0 : reliefPercent > 100 ? 100 : reliefPercent;
    amount /= 100.f;
    for (int layer = 0; layer < (backLayer ? 2 : 1) + volumeSlices; layer++) {
        int slice = layer - (backLayer ? 2 : 1) + 1;   // >=1 for volume slices
        bool vol = slice >= 1;
        bool back = !vol && layer == 1;
        float uvShift = back ? kBackUVShift : vol ? 0.17f * slice : 0.f;
        float topScale = back ? kBackHeight : vol ? 1.f + 0.05f * slice : 1.f;
        float inOff = back ? kBackOffset : vol ? kVolumeStep * slice : 0.f;
        for (int iy = 0; iy <= kGridY; iy++) {
            for (int ix = 0; ix <= kGridX; ix++) {
                float u = (float)ix / kGridX, v = (float)iy / kGridY;
                Vertex q;
                // Bilinear position inside the source quad; the back layer
                // stretches vertically from the quad's bottom edge.
                float vv = (back || vol) ? v / topScale : v;
                float px = c[0].x * (1 - u) * (1 - vv) + c[1].x * u * (1 - vv) + c[2].x * (1 - u) * vv + c[3].x * u * vv;
                float py = c[0].y * (1 - u) * (1 - vv) + c[1].y * u * (1 - vv) + c[2].y * (1 - u) * vv + c[3].y * u * vv;
                float pz = c[0].z * (1 - u) * (1 - vv) + c[1].z * u * (1 - vv) + c[2].z * (1 - u) * vv + c[3].z * u * vv;
                if (back || vol) {
                    // Extend above the original top edge: remap so v=0 keeps
                    // the shared bottom, v=1 reaches kBackHeight x the height.
                    float topY0 = c[0].y * (1 - u) + c[1].y * u, botY0 = c[2].y * (1 - u) + c[3].y * u;
                    float topZ0 = c[0].z * (1 - u) + c[1].z * u, botZ0 = c[2].z * (1 - u) + c[3].z * u;
                    float topX0 = c[0].x * (1 - u) + c[1].x * u, botX0 = c[2].x * (1 - u) + c[3].x * u;
                    py = botY0 + (py - botY0) * topScale;
                    pz = botZ0 + (pz - botZ0) * topScale;
                    px = botX0 + (px - botX0) * topScale;
                    (void)topY0; (void)topZ0; (void)topX0;
                }
                float d = 0.f;
                if (!back && amount > 0 && ix > 0 && ix < kGridX && iy > 0 && iy < kGridY)
                    d = SampleAlpha(alpha, aw, ah, u, v) * kReliefDepth * amount;
                q.x = px + normal[0] * (d + inOff);
                q.y = py + normal[1] * (d + inOff);
                q.z = pz + normal[2] * (d + inOff);
                q.u = u + uvShift; if (q.u > 1) q.u -= 1;
                q.v = v;
                q.shade = vol ? 0.88f - 0.07f * slice : back ? kBackShade : 0.8f + 0.45f * (d / kReliefDepth > 1 ? 1 : d / kReliefDepth);
                // Barycentric anchor: bottom edge = corners 2/3, top = 0/1.
                q.tri[0] = 0; q.tri[1] = 1; q.tri[2] = (u < 0.5f) ? 2 : 3;
                q.bary[0] = (1 - u) * (1 - v); q.bary[1] = u * (1 - v); q.bary[2] = 1 - q.bary[0] - q.bary[1];
                out.push_back(q);
            }
        }
    }
    return true;
}

// Triangle list indices for the grid(s); layer 1 starts after layer 0's verts.
inline void Indices(bool backLayer, std::vector<unsigned short> &out, int volumeSlices = 0) {
    out.clear();
    int layers = (backLayer ? 2 : 1) + volumeSlices;
    for (int layer = 0; layer < layers; layer++) {
        unsigned short base = (unsigned short)(layer * (kGridX + 1) * (kGridY + 1));
        for (int iy = 0; iy < kGridY; iy++)
            for (int ix = 0; ix < kGridX; ix++) {
                unsigned short a = base + iy * (kGridX + 1) + ix;
                unsigned short b = a + 1, c2 = a + kGridX + 1, d = c2 + 1;
                out.push_back(a); out.push_back(c2); out.push_back(b);
                out.push_back(b); out.push_back(c2); out.push_back(d);
            }
    }
}
}
#endif
