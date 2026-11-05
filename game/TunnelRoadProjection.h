#ifndef OPENCMR2_TUNNEL_ROAD_PROJECTION_H
#define OPENCMR2_TUNNEL_ROAD_PROJECTION_H
#include "FixedPoint.h"
#include "SceneNode.h"
#include <cmath>

void FixMatrix_ProjectWorldPointToView(int *out, FixVector *point, SceneNode *view);
// The production projector returns 16.16 PIXELS, not integer pixels. Keep the
// subpixel part when normalizing; omitting 65536 pinned every ordinary point
// to the maximum clamp, regardless of road direction. Output is v-down.
inline int TunnelRoad_ProjectPoint(FixVector *point, SceneNode *view,
                                  int width, int height, float *u, float *v)
{
    *u = *v = 0.5f;
    if (!view || width <= 0 || height <= 0) return 0;
    int projected[2];
    FixMatrix_ProjectWorldPointToView(projected, point, view);
    if (projected[0] == -0x640000 && projected[1] == -0x640000) return 0;
    *u = projected[0] / (65536.0f * width);
    *v = projected[1] / (65536.0f * height);
    *u = std::fmax(0.25f, std::fmin(0.75f, *u));
    *v = std::fmax(0.30f, std::fmin(0.70f, *v));
    return 1;
}
#endif
