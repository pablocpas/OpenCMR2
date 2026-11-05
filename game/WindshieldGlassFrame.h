#ifndef OPENCMR2_WINDSHIELD_GLASS_FRAME_H
#define OPENCMR2_WINDSHIELD_GLASS_FRAME_H
// Glass-local coordinates for the windshield dirt pane. The frame is built
// from the vertices of the real front-glass triangles (the same exterior
// inner faces the crack pane draws), so dirt is registered to the glass mesh
// and cannot appear on the dash, pillars, roof or outside the pane.
//
// Car space: Z is the lateral axis (see WindshieldGlass.h), Y is up, X is the
// forward axis. Map space: u runs along Z across the glass, v runs DOWN the
// glass (v = 0 top edge, v = 1 bottom edge), the same convention as the dirt
// map rows and texture coordinates. Pure maths, unit-tested in
// tests/windshield_dirt.cpp.
#include <math.h>

struct WsGlassFrame {
    int valid;
    float zMin, zMax, yMin, yMax;
    float widthUnits;    // physical width of the glass (Z extent)
    float heightUnits;   // physical height along the raked glass surface
    float aspect;        // heightUnits / widthUnits
};

// xyz: n vertices, 3 floats each. Needs a non-degenerate glass: a window with
// no lateral or vertical extent yields an invalid frame and nothing is drawn.
inline WsGlassFrame WsGlassFrame_FromVertices(const float *xyz, int n)
{
    WsGlassFrame f = {};
    if (xyz == 0 || n < 3) return f;
    float xMin = xyz[0], xMax = xyz[0];
    f.zMin = f.zMax = xyz[2];
    f.yMin = f.yMax = xyz[1];
    for (int i = 1; i < n; i++) {
        float x = xyz[i * 3], y = xyz[i * 3 + 1], z = xyz[i * 3 + 2];
        if (x < xMin) xMin = x;
        if (x > xMax) xMax = x;
        if (y < f.yMin) f.yMin = y;
        if (y > f.yMax) f.yMax = y;
        if (z < f.zMin) f.zMin = z;
        if (z > f.zMax) f.zMax = z;
    }
    f.widthUnits = f.zMax - f.zMin;
    float dy = f.yMax - f.yMin, dx = xMax - xMin;
    f.heightUnits = sqrtf(dy * dy + dx * dx);
    if (!(f.widthUnits > 1e-4f) || !(dy > 1e-4f) || !(f.heightUnits > 1e-4f)) return f;
    f.aspect = f.heightUnits / f.widthUnits;
    f.valid = 1;
    return f;
}

inline void WsGlassFrame_UV(const WsGlassFrame *f, float y, float z, float *u, float *v)
{
    *u = (z - f->zMin) / (f->zMax - f->zMin);
    *v = (f->yMax - y) / (f->yMax - f->yMin);
}

#endif
