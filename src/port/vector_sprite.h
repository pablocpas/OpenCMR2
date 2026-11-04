#ifndef OPENCMR2_VECTOR_SPRITE_H
#define OPENCMR2_VECTOR_SPRITE_H
#include "port/gfx.h"

#ifdef __cplusplus
extern "C" {
#endif

// Traces the original co-driver/menu icons and arcade maps. Coordinates and
// native pixels stay unchanged. Dotted selection maps retain their circular
// points.
void VectorSprite_Replace(const char *name, GfxTexture *texture);

#ifdef __cplusplus
}
#endif
#endif
