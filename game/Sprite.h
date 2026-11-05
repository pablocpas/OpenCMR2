#ifndef _SPRITE_H
#define _SPRITE_H

#include "Texture.h"

struct SpriteRect {
    short x;
    short y;
    short w;
    short h;
};

// Queued 2D sprite (0x38 bytes); four layers of up to 0x1fff sprites each. Raised from the
// original 0x3ff: the F10 diagnostic panel queues one sprite per glyph and is up to 41 rows
// of up to 159 characters (about 6.5k sprites on layer 1 with the HUD), so 0x3ff silently
// truncated it mid-row (the panel died at "fla" on a native screenshot). The matching
// vertex buffers in Sprite.cpp scale with the same constant.
struct Sprite {
    SpriteRect src;         // 0x0  u, v, w, h in the texture
    SpriteRect dst;         // 0x8  x, y, w, h on screen
    SpriteRect uv2;         // 0x10 optional second rect (zeroed when NULL)
    Texture *pTexture;      // 0x18
    BYTE colour[4];         // 0x1c
    int layer;              // 0x20
    short angle;            // 0x24 12-bit angle (from degrees in 16.16)
    BYTE subpixel[2];       // 0x26 PORT: x, y offset in 1/256 pixel (padding in the original)
    int centre[3];          // 0x28
    int param;              // 0x34
};

#define SPRITE_LAYER_MAX 0x1fff

// GLOBAL: CMR2 0x005f0858
extern Sprite g_spriteLayer1[SPRITE_LAYER_MAX];
// GLOBAL: CMR2 0x0064c958
extern Sprite g_spriteLayer2[SPRITE_LAYER_MAX];
// GLOBAL: CMR2 0x005e2858
extern Sprite g_spriteLayer3[SPRITE_LAYER_MAX];
// GLOBAL: CMR2 0x0063e958
extern Sprite g_spriteLayer4[SPRITE_LAYER_MAX];
// GLOBAL: CMR2 0x0065a95c
extern unsigned int g_spriteCount1;
// GLOBAL: CMR2 0x0065a960
extern unsigned int g_spriteCount2;
// GLOBAL: CMR2 0x0065a964
extern unsigned int g_spriteCount3;
// GLOBAL: CMR2 0x0065a968
extern unsigned int g_spriteCount4;

// Fixed-point vertex handed to Quad2D_QueueFixedTriangle.
struct Quad2DInputVertex {
    int x;
    int y;
    int z;
    BYTE colour[4];
    int u;
    int v;
};

// Runtime render records; pTexture expands with native pointer width.
struct Quad2DRenderVertex {
    float x;
    float y;
    float z;
    BYTE pad0x0c[0xc];
    DWORD colour;
    DWORD specular;
    float u;
    float v;
    BYTE pad0x28[8];
};

// Three 0x30-byte render vertices plus texture and flags.
struct Quad2DVertices {
    Quad2DRenderVertex v[3];
};

struct Quad2D {
    Quad2DVertices verts;   // 0x0
    Texture *pTexture;      // 0x90
    unsigned int flags;     // 0x94
};

// A destination is either layer flags or a native address.
void Quad2D_Queue(Quad2DVertices *pVerts, Texture *pTexture, UINT_PTR pDest);
void Quad2D_QueueFixedTriangle(int, Quad2DInputVertex *pA, Quad2DInputVertex *pB, Quad2DInputVertex *pC,
                               Texture *pTexture, UINT_PTR pDest);

// Queued 2D triangle (three transformed vertices); four layers of 0x400.
struct Tri2D {
    GfxTLVertex v[3];
};

#define TRI2D_LAYER_MAX 0x400

// GLOBAL: CMR2 0x007ad068
extern Tri2D g_tri2DLayer1[TRI2D_LAYER_MAX];
// GLOBAL: CMR2 0x0077cfd0
extern Tri2D g_tri2DLayer2[TRI2D_LAYER_MAX];
// GLOBAL: CMR2 0x007c5068
extern Tri2D g_tri2DLayer3[TRI2D_LAYER_MAX];
// GLOBAL: CMR2 0x00794fd0
extern Tri2D g_tri2DLayer4[TRI2D_LAYER_MAX];
// GLOBAL: CMR2 0x00816168
extern int g_tri2DEnabled;
// GLOBAL: CMR2 0x0081617c
extern unsigned int g_tri2DCount1;
// GLOBAL: CMR2 0x00816180
extern unsigned int g_tri2DCount2;
// GLOBAL: CMR2 0x00816184
extern unsigned int g_tri2DCount3;
// GLOBAL: CMR2 0x00816188
extern unsigned int g_tri2DCount4;

void Sprite_Init(void);
void Sprite_Queue(SpriteRect *pSrc, SpriteRect *pDst, Texture *pTexture, int layer, short angleDeg, int *pCentre, SpriteRect *pUv2, BYTE *pColour, int param);
// PORT: moves the next queued sprite right and down by a fraction of a pixel
// (16.16, 0 to 0xffff).
void Sprite_SetNextSubpixelOffset(int x, int y);
int Sprite_Shutdown(void);
int Sprite_FillRect(BYTE *unused, short *pRect, BYTE *pColour, int layer);
void Line2D_Init(void);
void Line2D_Queue(int *pA, int *pB, BYTE *pColourA, BYTE *pColourB);
int Line2D_Shutdown(void);
void Tri2D_Init(void);
void Tri2D_Queue(int *pA, int *pB, int *pC, BYTE *pColour, int layer);
void Tri2D_SetVertex(GfxTLVertex *pVertex, int *pPos, BYTE *pColour);
int Tri2D_Shutdown(void);

// PORT: lays the in-race HUD out on any screen (see Sprite.cpp).
void Hud_BeginView(int view);
void Hud_Element(void);
void Hud_EndView(void);
void Hud_BeginLayout(void);
void Hud_EndLayout(void);
// PORT: the menus laid out for the screen's size (Sprite.cpp).
void MenuScreen_Begin(void);
void MenuScreen_End(void);
// A screen drawn in the real screen's pixels between the menus' own.
void MenuScreen_Suspend(void);
void MenuScreen_Resume(void);
// The menus over a race, drawn among its own 2D.
void MenuScreen_BeginOverlay(void);
void MenuScreen_EndOverlay(void);
// The display's size, also while the menus draw on their own screen.
void MenuScreen_GetDisplaySize(int *pWidth, int *pHeight);
// The largest [video] menu_scale the display takes.
int MenuScreen_GetLargestScale(void);

#endif
