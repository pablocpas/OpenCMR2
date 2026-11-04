#include "Sprite.h"
#include "FloatToInt.h"
#include "Game.h"
#include "Graphics.h"
#include "GameInfo.h"
#include "Frontend.h"
#include "port/sys.h"
#include <math.h>
#include <stdio.h>

Sprite g_spriteLayer1[SPRITE_LAYER_MAX];
Sprite g_spriteLayer2[SPRITE_LAYER_MAX];
Sprite g_spriteLayer3[SPRITE_LAYER_MAX];
Sprite g_spriteLayer4[SPRITE_LAYER_MAX];
unsigned int g_spriteCount1;
unsigned int g_spriteCount2;
unsigned int g_spriteCount3;
unsigned int g_spriteCount4;
// GLOBAL: CMR2 0x0065a958
int g_spriteEnabled;

Tri2D g_tri2DLayer1[TRI2D_LAYER_MAX];
Tri2D g_tri2DLayer2[TRI2D_LAYER_MAX];
Tri2D g_tri2DLayer3[TRI2D_LAYER_MAX];
Tri2D g_tri2DLayer4[TRI2D_LAYER_MAX];
int g_tri2DEnabled;
unsigned int g_tri2DCount1;
unsigned int g_tri2DCount2;
unsigned int g_tri2DCount3;
unsigned int g_tri2DCount4;
// GLOBAL: CMR2 0x0081616c
unsigned int g_quad2DCountA;
// GLOBAL: CMR2 0x00816170
unsigned int g_quad2DCountB;

// Screen-space quads of each sprite layer, four vertices per sprite.
// GLOBAL: CMR2 0x005a2858
GfxTLVertex g_spriteVerts1[1024 * 4];
// GLOBAL: CMR2 0x005c2858
GfxTLVertex g_spriteVerts2[1024 * 4];
// GLOBAL: CMR2 0x005fe858
GfxTLVertex g_spriteVerts4[1024 * 4];
// GLOBAL: CMR2 0x0061e858
GfxTLVertex g_spriteVerts3[1024 * 4];

void Graphics_InvalidateTextureStageCache(void);

// PORT: pixels of the back buffer per pixel of the screen the game draws in,
// larger than 1 when the menus are scaled up (see MenuScreen_Begin).
static float g_spriteScreenScale = 1.0f;

// PORT: the fraction of a pixel the next queued sprite is moved by
// (Sprite_SetNextSubpixelOffset).
static int g_spriteNextSubpixel[2];

#define SPRITE_VERTEX(v, px, py, u, vv)                                             \
    (v)->sx = (float)(px);                                                          \
    (v)->sy = (float)(py);                                                          \
    (v)->sz = 0.0f;                                                                 \
    (v)->rhw = 1.0f;                                                                \
    (v)->color = colour;                                                            \
    (v)->specular = 0xff000000;                                                     \
    (v)->tu = (u);                                                                  \
    (v)->tv = (vv);                                                                 \
    if (angle != 0) {                                                               \
        p.x = (int)FTOI(__int64, (v)->sx);                                                \
        p.y = (int)FTOI(__int64, (v)->sy);                                                \
        p.z = (int)FTOI(__int64, (v)->sz);                                                \
        FixMatrix_TransformAboutPivot(&out, &p, (FixVector *)centre, &rotation);    \
        (v)->sx = (float)out.x;                                                     \
        (v)->sy = (float)out.y;                                                     \
        (v)->sz = (float)out.z;                                                     \
    }

// Builds the quads of one sprite layer (1..4), rotating them about their
// centre when needed, draws them with point filtering and empties the layer.
// match 92%: remaining diff is register allocation and scheduling of the integer temporaries.
// FUNCTION: CMR2 0x004a3650
void Sprite_DrawLayer(int layer)
{
    GfxTLVertex *pVert;
    Sprite *pSprite;
    unsigned int count;
    unsigned int n;
    float w, h;
    float u0, u1, v0, v1;
    float x0, x1, y0, y1;
    float inset;
    float t;
    DWORD colour;
    int centre[3];
    short angle;
    FixMatrix rotation;
    FixVector p;
    FixVector out;

    switch (layer) {
    case 2:
        count = g_spriteCount2;
        g_spriteCount2 = 0;
        pVert = g_spriteVerts2;
        pSprite = g_spriteLayer2;
        break;
    case 3:
        count = g_spriteCount3;
        g_spriteCount3 = 0;
        pVert = g_spriteVerts3;
        pSprite = g_spriteLayer3;
        break;
    case 4:
        count = g_spriteCount4;
        g_spriteCount4 = 0;
        pVert = g_spriteVerts4;
        pSprite = g_spriteLayer4;
        break;
    default:
        count = g_spriteCount1;
        g_spriteCount1 = 0;
        pVert = g_spriteVerts1;
        pSprite = g_spriteLayer1;
        break;
    }
    for (n = 0; n < count; n++) {
        // A sprite without texture is skipped without advancing (as the original does).
        if (pSprite->pTexture == NULL)
            continue;
        w = (float)(unsigned int)pSprite->pTexture->width;
        u0 = X87S_DIV((float)pSprite->src.x, w);
        u1 = X87S_DIV(X87S_FILD(float, (pSprite->src.x + pSprite->src.w)), w);
        h = (float)(unsigned int)pSprite->pTexture->height;
        v0 = X87S_DIV((float)pSprite->src.y, h);
        v1 = X87S_DIV(X87S_FILD(float, (pSprite->src.y + pSprite->src.h)), h);
        if (CGraphics::GetSelectedDisplayDriverIndex() == 0) {
            // PORT: half a pixel of the screen, to read texel centres; that
            // is half a texel only when the sprite keeps its size.
            if (pSprite->dst.w != pSprite->src.w && pSprite->dst.w != 0) {
                u0 += X87S_DIV(X87S_MUL(0.5f / w, X87S_LOAD(pSprite->src.w)), X87S_LOAD(pSprite->dst.w));
                u1 += X87S_DIV(X87S_MUL(0.5f / w, X87S_LOAD(pSprite->src.w)), X87S_LOAD(pSprite->dst.w));
            } else {
                u0 += 0.5f / w;
                u1 += 0.5f / w;
            }
            if (pSprite->dst.h != pSprite->src.h && pSprite->dst.h != 0) {
                v0 += X87S_DIV(X87S_MUL(0.5f / h, X87S_LOAD(pSprite->src.h)), X87S_LOAD(pSprite->dst.h));
                v1 += X87S_DIV(X87S_MUL(0.5f / h, X87S_LOAD(pSprite->src.h)), X87S_LOAD(pSprite->dst.h));
            } else {
                v0 += 0.5f / h;
                v1 += 0.5f / h;
            }
        }
        // PORT: filtered smoothly, a sprite scaled up on the back buffer reads
        // its border texels from their centres, half a texel inside its quad,
        // so the filter does not reach into the next sprite of the texture
        // (the next letter of a font); the scale stays the same. Pixel centres
        // are on integer coordinates: the quad starts half a pixel before the
        // centre of its first texel. Text drawn from a distance field is cut
        // by half a texel of its field only: its glyphs reach their cells'
        // edges.
        x0 = pSprite->dst.x;
        x1 = pSprite->dst.x + pSprite->dst.w;
        y0 = pSprite->dst.y;
        y1 = pSprite->dst.y + pSprite->dst.h;
        // PORT: at the fraction of a pixel it was queued at.
        x0 += pSprite->subpixel[0] / 256.0f;
        x1 += pSprite->subpixel[0] / 256.0f;
        y0 += pSprite->subpixel[1] / 256.0f;
        y1 += pSprite->subpixel[1] / 256.0f;
        inset = Gfx_GetSpriteTexelInset(pSprite->pTexture->pSurface);
        if (pSprite->src.w > 0 && X87S_MUL(X87S_LOAD(pSprite->dst.w), g_spriteScreenScale) > pSprite->src.w) {
            u0 = X87S_DIV((X87S_ADD(X87S_LOAD(pSprite->src.x), inset)), w);
            u1 = X87S_DIV((X87S_SUB(X87S_LOAD(pSprite->src.x + pSprite->src.w), inset)), w);
            x0 += X87S_SUB(X87S_DIV(X87S_MUL(inset, X87S_LOAD(pSprite->dst.w)), X87S_LOAD(pSprite->src.w)), 0.5f);
            x1 -= X87S_ADD(X87S_DIV(X87S_MUL(inset, X87S_LOAD(pSprite->dst.w)), X87S_LOAD(pSprite->src.w)), 0.5f);
        }
        if (pSprite->src.h > 0 && X87S_MUL(X87S_LOAD(pSprite->dst.h), g_spriteScreenScale) > pSprite->src.h) {
            v0 = X87S_DIV((X87S_ADD(X87S_LOAD(pSprite->src.y), inset)), h);
            v1 = X87S_DIV((X87S_SUB(X87S_LOAD(pSprite->src.y + pSprite->src.h), inset)), h);
            y0 += X87S_SUB(X87S_DIV(X87S_MUL(inset, X87S_LOAD(pSprite->dst.h)), X87S_LOAD(pSprite->src.h)), 0.5f);
            y1 -= X87S_ADD(X87S_DIV(X87S_MUL(inset, X87S_LOAD(pSprite->dst.h)), X87S_LOAD(pSprite->src.h)), 0.5f);
        }
        if (pSprite->param & 5) {
            t = u0;
            u0 = u1;
            u1 = t;
        }
        if (pSprite->param & 6) {
            t = v0;
            v0 = v1;
            v1 = t;
        }
        colour = GFX_RGBA(pSprite->colour[0], pSprite->colour[1], pSprite->colour[2], pSprite->colour[3]);
        *(FixVector *)centre = *(FixVector *)pSprite->centre;
        if (centre[0] == -1 && centre[1] == -1 && centre[2] == -1) {
            centre[0] = pSprite->dst.w / 2 + pSprite->dst.x;
            centre[1] = pSprite->dst.h / 2 + pSprite->dst.y;
        }
        angle = pSprite->angle;
        if (angle != 0)
            FixMatrix_RotationZ(&rotation, angle);
        SPRITE_VERTEX(pVert, x0, y0, u0, v0);
        pVert++;
        SPRITE_VERTEX(pVert, x1, y0, u1, v0);
        pVert++;
        SPRITE_VERTEX(pVert, x0, y1, u0, v1);
        pVert++;
        SPRITE_VERTEX(pVert, x1, y1, u1, v1);
        pVert++;
        pSprite++;
    }

    switch (layer) {
    case 2:
        pVert = g_spriteVerts2;
        pSprite = g_spriteLayer2;
        break;
    case 3:
        pVert = g_spriteVerts3;
        pSprite = g_spriteLayer3;
        break;
    case 4:
        pVert = g_spriteVerts4;
        pSprite = g_spriteLayer4;
        break;
    default:
        pVert = g_spriteVerts1;
        pSprite = g_spriteLayer1;
        break;
    }
    Gfx_SetStageMinFilter(0, GFX_FILTER_POINT);
    Gfx_SetStageMagFilter(0, GFX_FILTER_POINT);
    for (n = 0; n < count; n++) {
        CGraphics::ApplyTextureStageChange(0, pSprite[n].pTexture);
        Gfx_DrawPrimitiveTL(GFX_TRIANGLESTRIP, (GfxTLVertex *)&pVert[n * 4], 4);
        CGame::m_unk0x0059ce20 += 2;
    }
    Gfx_SetStageMinFilter(0, GFX_FILTER_LINEAR);
    Gfx_SetStageMagFilter(0, GFX_FILTER_LINEAR);
    Graphics_InvalidateTextureStageCache();
    CGraphics::InvalidateBlendStateCache();
}

// Exit callback of Sprite_Init.
// FUNCTION: CMR2 0x004a3640
int Sprite_Shutdown(void)
{
    g_spriteEnabled = 0;
    return 1;
}

// FUNCTION: CMR2 0x004a3260
void Sprite_Init(void)
{
    g_spriteEnabled = 1;
    g_spriteCount1 = 0;
    g_spriteCount2 = 0;
    g_spriteCount3 = 0;
    g_spriteCount4 = 0;
    CGame::RegisterCallback(Sprite_Shutdown, NULL);
}

// Exit callback of Tri2D_Init.
// FUNCTION: CMR2 0x004bc090
int Tri2D_Shutdown(void)
{
    g_tri2DCount1 = 0;
    g_tri2DCount2 = 0;
    g_tri2DCount4 = 0;
    g_quad2DCountB = 0;
    g_quad2DCountA = 0;
    g_tri2DEnabled = 0;
    return 1;
}

// FUNCTION: CMR2 0x004bb610
void Tri2D_Init(void)
{
    g_tri2DEnabled = 1;
    g_tri2DCount1 = 0;
    g_tri2DCount2 = 0;
    g_tri2DCount3 = 0;
    g_tri2DCount4 = 0;
    g_quad2DCountA = 0;
    g_quad2DCountB = 0;
    CGame::RegisterCallback(Tri2D_Shutdown, NULL);
}

// pPos is a 16.16 screen position; the colour bytes are r, g, b, a.
// FUNCTION: CMR2 0x004bb800
void Tri2D_SetVertex(GfxTLVertex *pVertex, int *pPos, BYTE *pColour)
{
    pVertex->sx = (float)(X87S_MUL(pPos[0], CGraphics::m_oneOver65536));
    pVertex->sy = (float)(X87S_MUL(pPos[1], CGraphics::m_oneOver65536));
    pVertex->color = GFX_RGBA(pColour[0], pColour[1], pColour[2], pColour[3]);
}

// match 85%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004bb650
void Tri2D_Queue(int *pA, int *pB, int *pC, BYTE *pColour, int layer)
{
    unsigned int n;
    unsigned int i3;

    if (g_tri2DEnabled != 0 && g_tri2DCount4 < TRI2D_LAYER_MAX && g_tri2DCount2 < TRI2D_LAYER_MAX && g_tri2DCount1 < TRI2D_LAYER_MAX) {
        switch (layer) {
        case 2:
            n = g_tri2DCount2;
            i3 = n * 3;
            Tri2D_SetVertex(&((GfxTLVertex *)g_tri2DLayer2)[i3 + 0], pA, pColour);
            Tri2D_SetVertex(&((GfxTLVertex *)g_tri2DLayer2)[i3 + 1], pB, pColour);
            Tri2D_SetVertex(&((GfxTLVertex *)g_tri2DLayer2)[i3 + 2], pC, pColour);
            g_tri2DCount2++;
            break;
        case 3:
            n = g_tri2DCount3;
            i3 = n * 3;
            Tri2D_SetVertex(&((GfxTLVertex *)g_tri2DLayer3)[i3 + 0], pA, pColour);
            Tri2D_SetVertex(&((GfxTLVertex *)g_tri2DLayer3)[i3 + 1], pB, pColour);
            Tri2D_SetVertex(&((GfxTLVertex *)g_tri2DLayer3)[i3 + 2], pC, pColour);
            g_tri2DCount3++;
            break;
        case 4:
            n = g_tri2DCount4;
            i3 = n * 3;
            Tri2D_SetVertex(&((GfxTLVertex *)g_tri2DLayer4)[i3 + 0], pA, pColour);
            Tri2D_SetVertex(&((GfxTLVertex *)g_tri2DLayer4)[i3 + 1], pB, pColour);
            Tri2D_SetVertex(&((GfxTLVertex *)g_tri2DLayer4)[i3 + 2], pC, pColour);
            g_tri2DCount4++;
            break;
        default:
            n = g_tri2DCount1;
            i3 = n * 3;
            Tri2D_SetVertex(&((GfxTLVertex *)g_tri2DLayer1)[i3 + 0], pA, pColour);
            Tri2D_SetVertex(&((GfxTLVertex *)g_tri2DLayer1)[i3 + 1], pB, pColour);
            Tri2D_SetVertex(&((GfxTLVertex *)g_tri2DLayer1)[i3 + 2], pC, pColour);
            g_tri2DCount1++;
            break;
        }
    }
}

// The original repeats the same block for each of the four layers.
#define QUEUE_SPRITE(arr, count, n)                                             \
    if (count >= SPRITE_LAYER_MAX)                                              \
        return;                                                                 \
    p = &arr[count];                                                            \
    count++;                                                                    \
    p->src = *pSrc;                                                             \
    p->dst = *pDst;                                                             \
    if (pUv2 != NULL) {                                                         \
        p->uv2 = *pUv2;                                                         \
    } else {                                                                    \
        p->uv2.h = 0;                                                           \
        p->uv2.w = 0;                                                           \
        p->uv2.y = 0;                                                           \
        p->uv2.x = 0;                                                           \
    }                                                                           \
    p->pTexture = pTexture;                                                     \
    p->colour[0] = pColour[0];                                                  \
    p->colour[1] = pColour[1];                                                  \
    p->colour[2] = pColour[2];                                                  \
    p->colour[3] = pColour[3];                                                  \
    p->angle = (short)((angleDeg << 12) / 360);                           \
    p->layer = n;                                                               \
    if (pCentre != NULL) {                                                      \
        p->centre[0] = pCentre[0];                                              \
        p->centre[1] = pCentre[1];                                              \
        p->centre[2] = pCentre[2];                                              \
    } else {                                                                    \
        p->centre[0] = 0;                                                       \
        p->centre[1] = 0;                                                       \
        p->centre[2] = 0;                                                       \
    }                                                                           \
    p->param = param;                                                           \
    p->subpixel[0] = subX;                                                      \
    p->subpixel[1] = subY;

void Sprite_SetNextSubpixelOffset(int x, int y)
{
    g_spriteNextSubpixel[0] = x;
    g_spriteNextSubpixel[1] = y;
}

// Appends a sprite to one of the four 2D layers.
// FUNCTION: CMR2 0x004a3290
void Sprite_Queue(SpriteRect *pSrc, SpriteRect *pDst, Texture *pTexture, int layer, short angleDeg, int *pCentre, SpriteRect *pUv2, BYTE *pColour, int param)
{
    Sprite *p;
    // PORT: the offset of Sprite_SetNextSubpixelOffset, for this sprite only.
    BYTE subX = (BYTE)(g_spriteNextSubpixel[0] >> 8);
    BYTE subY = (BYTE)(g_spriteNextSubpixel[1] >> 8);

    g_spriteNextSubpixel[0] = 0;
    g_spriteNextSubpixel[1] = 0;
    if (layer == 1) {
        QUEUE_SPRITE(g_spriteLayer1, g_spriteCount1, 1)
        return;
    }
    if (layer == 2) {
        QUEUE_SPRITE(g_spriteLayer2, g_spriteCount2, 2)
        return;
    }
    if (layer == 3) {
        QUEUE_SPRITE(g_spriteLayer3, g_spriteCount3, 3)
        return;
    }
    if (layer == 4) {
        QUEUE_SPRITE(g_spriteLayer4, g_spriteCount4, 4)
        return;
    }
}

// GLOBAL: CMR2 0x005112e8
double g_minus65536 = -65536.0;

// Fills a screen rectangle (x, y, w, h in pixels) with two 2D triangles,
// clipping it to the screen first.
// match 22%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004a5e40
int Sprite_FillRect(BYTE *unused, short *pRect, BYTE *pColour, int layer)
{
    short x;
    short y;
    short w;
    short h;
    int c[2];
    int a[2];
    int b[2];
    int x0;
    int y0;
    int x1;
    int y1;

    x = pRect[0];
    y = pRect[1];
    w = pRect[2];
    h = pRect[3];
    if (w > 0 && h > 0 && (x >= 0 || w + x >= 0) && (y >= 0 || h + y >= 0)) {
        if (x < (int)g_pGraphics->resX && y < (int)g_pGraphics->resY) {
            if (x < 0) {
                w += x;
                x = 0;
            }
            x0 = x;
            if (w + x0 > (int)g_pGraphics->resX)
                w = (short)g_pGraphics->resX - x - 1;
            if (y < 0) {
                h += y;
                y = 0;
            }
            y0 = y;
            if (h + y0 > (int)g_pGraphics->resY)
                h = (short)g_pGraphics->resX - y;
                h -= 1;
            a[0] = (int)FTOI(__int64, X87S_MUL((double)x0, CGraphics::m_65536));
            c[0] = a[0];
            a[1] = (int)FTOI(__int64, X87S_MUL((double)y0, CGraphics::m_65536));
            x1 = 0x8000 - (int)FTOI(__int64, X87S_MUL((double)(w - 1 + x0), g_minus65536));
            b[0] = x1;
            b[1] = a[1];
            y1 = 0x8000 - (int)FTOI(__int64, X87S_MUL((double)(h - 1 + y0), g_minus65536));
            c[1] = y1;
            Tri2D_Queue(a, b, c, pColour, layer);
            a[0] = x1;
            a[1] = y1;
            Tri2D_Queue(b, a, c, pColour, layer);
        }
    }
    return 1;
}

// 2D lines (up to 200): two 16.16 endpoints and two RGBA colours.
struct Line2D {
    int a[3];               // 0x0
    int b[3];               // 0xc
    BYTE colourA[4];        // 0x18
    BYTE colourB[4];        // 0x1c
    BYTE pad[4];
};

// GLOBAL: CMR2 0x0072d680
Line2D g_line2D[200];
// GLOBAL: CMR2 0x00730fc8
unsigned int g_line2DCount;
// GLOBAL: CMR2 0x00730fcc
int g_line2DInitialised;
// GLOBAL: CMR2 0x0072f2a0
int g_unk0x0072f2a0;

// Screen-space lines, drawn on their layer (see ScreenLine2D_Draw).
struct ScreenLine2D {
    int a[3];               // 0x0
    int b[3];               // 0xc
    BYTE colourA[4];        // 0x18
    BYTE colourB[4];        // 0x1c
    int layer;              // 0x20
};

// GLOBAL: CMR2 0x0072f2a8
ScreenLine2D g_screenLine2D[200];

// Exit callback of Line2D_Init.
// FUNCTION: CMR2 0x004bb5f0
int Line2D_Shutdown(void)
{
    if (g_line2DInitialised == 0)
        return 0;
    g_line2DCount = 0;
    g_line2DInitialised = 0;
    return 1;
}

// FUNCTION: CMR2 0x004bb280
void Line2D_Init(void)
{
    if (g_line2DInitialised == 0) {
        g_line2DCount = 0;
        g_unk0x0072f2a0 = 0;
        CGame::RegisterCallback(Line2D_Shutdown, NULL);
        g_line2DInitialised = 1;
    }
}

// FUNCTION: CMR2 0x004bb430
void Line2D_Queue(int *pA, int *pB, BYTE *pColourA, BYTE *pColourB)
{
    Line2D *p;

    if (g_line2DCount < 200) {
        p = &g_line2D[g_line2DCount];
        p->a[0] = pA[0];
        p->a[1] = pA[1];
        p->a[2] = pA[2];
        p->b[0] = pB[0];
        p->b[1] = pB[1];
        p->b[2] = pB[2];
        p->colourA[0] = pColourA[0];
        p->colourA[1] = pColourA[1];
        p->colourA[2] = pColourA[2];
        p->colourA[3] = pColourA[3];
        p->colourB[0] = pColourB[0];
        p->colourB[1] = pColourB[1];
        p->colourB[2] = pColourB[2];
        p->colourB[3] = pColourB[3];
        g_line2DCount++;
    }
}

// Vertex of a 3D line (D3DFVF_XYZ | NORMAL | DIFFUSE | SPECULAR | TEX2).
struct Line2DVertex {
    float x, y, z;
    float nx, ny, nz;
    DWORD diffuse;
    DWORD specular;
    float u, v;
    float u2, v2;
};

// Draws the queued screen lines of one layer (none for layer 4); layer 1
// also empties the queue.
// match 64%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004bb2b0
void ScreenLine2D_Draw(int layer)
{
    GfxTLVertex v[2];
    ScreenLine2D *p;
    unsigned int i;

    if (g_unk0x0072f2a0 == 0 || layer == 4)
        return;
    CGraphics::ApplyTextureStageChange(0, 0);
    for (i = 0, p = g_screenLine2D; i < (unsigned int)g_unk0x0072f2a0; i++, p++) {
        if (layer != p->layer)
            continue;
        v[0].sx = X87S_MUL(X87S_FILD(float, p->a[0]), CGraphics::m_oneOver65536);
        v[0].color = GFX_RGBA(p->colourA[0], p->colourA[1], p->colourA[2], p->colourA[3]);
        v[0].rhw = 1.0f;
        v[0].sy = X87S_MUL(X87S_FILD(float, p->a[1]), CGraphics::m_oneOver65536);
        v[0].specular = GFX_RGBA(p->colourB[0], p->colourB[1], p->colourB[2], p->colourB[3]);
        v[0].sz = X87S_MUL(X87S_FILD(float, p->a[2]), CGraphics::m_oneOver65536);
        v[0].tv = 0.0f;
        v[0].tu = 0.0f;
        v[1].color = GFX_RGBA(p->colourB[0], p->colourB[1], p->colourB[2], p->colourB[3]);
        v[1].sx = X87S_MUL(X87S_FILD(float, p->b[0]), CGraphics::m_oneOver65536);
        v[1].rhw = 1.0f;
        v[1].tv = 0.0f;
        v[1].tu = 0.0f;
        v[1].sy = X87S_MUL(X87S_FILD(float, p->b[1]), CGraphics::m_oneOver65536);
        v[1].sz = X87S_MUL(X87S_FILD(float, p->b[2]), CGraphics::m_oneOver65536);
        Gfx_DrawPrimitiveTL(GFX_LINELIST, (GfxTLVertex *)v, 2);
    }
    if (layer == 1)
        g_unk0x0072f2a0 = 0;
}

// Draws and empties the queued 3D lines with alpha blending.
// match 76%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004bb4c0
void Line2D_Draw(void)
{
    Line2DVertex v[2];
    Line2D *p;
    unsigned int i;

    if (g_line2DCount == 0)
        return;
    CGraphics::ApplyTextureStageChange(0, 0);
    Gfx_SetSrcBlend(GFX_BLEND_SRCALPHA);
    Gfx_SetDestBlend(GFX_BLEND_INVSRCALPHA);
    for (i = 0, p = g_line2D; i < g_line2DCount; i++, p++) {
        v[0].x = X87S_MUL(X87S_FILD(float, p->a[0]), CGraphics::m_oneOver65536);
        v[0].y = X87S_MUL(X87S_FILD(float, p->a[1]), CGraphics::m_oneOver65536);
        v[0].diffuse = GFX_RGBA(p->colourA[0], p->colourA[1], p->colourA[2], p->colourA[3]);
        v[0].z = X87S_MUL(X87S_FILD(float, p->a[2]), CGraphics::m_oneOver65536);
        v[1].diffuse = GFX_RGBA(p->colourB[0], p->colourB[1], p->colourB[2], p->colourB[3]);
        v[1].x = X87S_MUL(X87S_FILD(float, p->b[0]), CGraphics::m_oneOver65536);
        v[1].y = X87S_MUL(X87S_FILD(float, p->b[1]), CGraphics::m_oneOver65536);
        v[1].z = X87S_MUL(X87S_FILD(float, p->b[2]), CGraphics::m_oneOver65536);
        Gfx_DrawPrimitive(GFX_LINELIST, (GfxVertex *)v, 2);
    }
    g_line2DCount = 0;
}

// Draws and empties one layer (1..4) of queued 2D triangles.
// FUNCTION: CMR2 0x004bb850
void Tri2D_DrawLayer(int layer)
{
    CGraphics::SetCachedSourceDestinationBlend(5, 6);
    switch (layer) {
    case 2:
        if (g_tri2DCount2 > 0u) {
            CGraphics::ApplyTextureStageChange(0, 0);
            Gfx_DrawPrimitiveTL(GFX_TRIANGLELIST, (GfxTLVertex *)g_tri2DLayer2, g_tri2DCount2 * 3);
            CGame::m_unk0x0059ce20 += g_tri2DCount2;
            g_tri2DCount2 = 0;
        }
        break;
    case 3:
        if (g_tri2DCount3 > 0u) {
            CGraphics::ApplyTextureStageChange(0, 0);
            Gfx_DrawPrimitiveTL(GFX_TRIANGLELIST, (GfxTLVertex *)g_tri2DLayer3, g_tri2DCount3 * 3);
            CGame::m_unk0x0059ce20 += g_tri2DCount3;
            g_tri2DCount3 = 0;
        }
        break;
    case 4:
        if (g_tri2DCount4 > 0u) {
            CGraphics::ApplyTextureStageChange(0, 0);
            Gfx_DrawPrimitiveTL(GFX_TRIANGLELIST, (GfxTLVertex *)g_tri2DLayer4, g_tri2DCount4 * 3);
            CGame::m_unk0x0059ce20 += g_tri2DCount4;
            g_tri2DCount4 = 0;
        }
        break;
    default:
        if (g_tri2DCount1 > 0u) {
            CGraphics::ApplyTextureStageChange(0, 0);
            Gfx_DrawPrimitiveTL(GFX_TRIANGLELIST, (GfxTLVertex *)g_tri2DLayer1, g_tri2DCount1 * 3);
            CGame::m_unk0x0059ce20 += g_tri2DCount1;
            g_tri2DCount1 = 0;
        }
        break;
    }
    Graphics_InvalidateTextureStageCache();
    CGraphics::InvalidateBlendStateCache();
}

// GLOBAL: CMR2 0x007dd168
Quad2D g_quad2DLayerA[0x400];
// GLOBAL: CMR2 0x00803168
Quad2D g_quad2DLayerB[0x200];
// GLOBAL: CMR2 0x00730fd0
// OpenCMR2: layer C (skid marks) is 8x the original 0x800 (one entry = one triangle) so longer tyre tracks fit; the draw count stays what the game queues.
#define QUAD2D_LAYER_C_MAX 0x4000
Quad2D g_quad2DLayerC[QUAD2D_LAYER_C_MAX];
// GLOBAL: CMR2 0x007acfd0
Quad2D g_quad2DLayerD[1];
// GLOBAL: CMR2 0x00816174
unsigned int g_quad2DCountC;
// GLOBAL: CMR2 0x00816178
unsigned int g_quad2DCountD;
// GLOBAL: CMR2 0x0081618c
BYTE g_quad2DConvertOverflow;
// GLOBAL: CMR2 0x0081618d
BYTE g_quad2DOverflow;

// pDest carries either numeric layer flags or a native destination address.
// flags retains only the 32-bit render mask; it is never used to rebuild p.
// Queues a quad into the layer selected by the low bits of pDest (8, 0x10,
// 0x20, 0x40); with none of those bits set pDest is the destination itself.
// match 88%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004bbc60
void Quad2D_Queue(Quad2DVertices *pVerts, Texture *pTexture, UINT_PTR pDest)
{
    Quad2D *p;

    if (pDest & 8) {
        if (g_quad2DCountA >= 0x400) {
            if (g_quad2DOverflow == 0)
                g_quad2DOverflow = 1;
            return;
        }
        p = &g_quad2DLayerA[g_quad2DCountA];
        g_quad2DCountA++;
    } else if (pDest & 0x10) {
        if (g_quad2DCountB >= 0x200) {
            if (g_quad2DOverflow == 0)
                g_quad2DOverflow = 1;
            return;
        }
        p = &g_quad2DLayerB[g_quad2DCountB];
        g_quad2DCountB++;
    } else if (pDest & 0x20) {
        if (g_quad2DCountC >= QUAD2D_LAYER_C_MAX) {
            if (g_quad2DOverflow == 0)
                g_quad2DOverflow = 1;
            return;
        }
        p = &g_quad2DLayerC[g_quad2DCountC];
        g_quad2DCountC++;
    } else if (pDest & 0x40) {
        if (g_quad2DCountD >= 1) {
            if (g_quad2DOverflow == 0)
                g_quad2DOverflow = 1;
            return;
        }
        p = &g_quad2DLayerD[g_quad2DCountD];
        g_quad2DCountD++;
    } else {
        p = (Quad2D *)pDest;
    }
    p->flags = (unsigned int)pDest;
    p->pTexture = pTexture;
    p->verts = *pVerts;
}

void Frontend_SetObjectField118(struct Texture *pTexture, int value);
void Graphics_InvalidateTextureStageCache(void);

// Applies the texture (with its blend setup) and the z/cull flags of a queued quad.
#define QUAD2D_SET_STATE(q)                                                          \
    if ((q)->pTexture != NULL) {                                                    \
        Frontend_SetObjectField118((q)->pTexture, (q)->pTexture->blendMode);      \
        CGraphics::ApplyTextureStageChange(0, (q)->pTexture);                              \
    } else {                                                                        \
        CGraphics::ApplyTextureStageChange(0, 0);                                               \
    }

#define QUAD2D_SET_FLAGS(q)                                                          \
    if ((q)->flags & 1)                                                              \
        CGraphics::SetZEnable(0);                                                    \
    else                                                                             \
        CGraphics::SetZEnable(1);                                                    \
    if ((q)->flags & 2)                                                              \
        CGraphics::SetZWriteEnable(0);                                               \
    else                                                                             \
        CGraphics::SetZWriteEnable(1);                                               \
    if ((q)->flags & 4)                                                              \
        CGraphics::SetCullMode(1);                                                   \
    else                                                                             \
        CGraphics::SetCullMode(CGame::GetSectorDrawState());

// Copies one queued layer (8, 0x10, 0x20 or other) into the shared vertex
// buffer and draws it, flushing whenever the texture or flags change.
// match 82%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004bbd80
void Quad2D_DrawLayer(unsigned int layer)
{
    GfxVertexBuffer *pVB;
    Quad2D *pQuad;
    Quad2D *pLast;
    Texture *pLastTexture;
    unsigned int lastFlags;
    unsigned int count;
    unsigned int i;
    unsigned int n;
    BYTE *pData;

    count = 0;
    n = 0;
    pLastTexture = (Texture *)1;
    lastFlags = 0xffffffff;
    pData = (BYTE *)Gfx_LockVertexBuffer(CGraphics::m_pTextureManager->pVertexBuffer1);
    if (layer & 8) {
        count = g_quad2DCountA;
        pQuad = g_quad2DLayerA;
        g_quad2DCountA = 0;
    } else if (layer & 0x10) {
        count = g_quad2DCountB;
        pQuad = g_quad2DLayerB;
        g_quad2DCountB = 0;
    } else if (layer & 0x20) {
        count = g_quad2DCountC;
        pQuad = g_quad2DLayerC;
        g_quad2DCountC = 0;
    } else {
        count = g_quad2DCountD;
        pQuad = g_quad2DLayerD;
        g_quad2DCountD = 0;
    }
    for (i = count; i > 0; i--) {
        if (pLastTexture != pQuad->pTexture || lastFlags != pQuad->flags) {
            pVB = CGraphics::m_pTextureManager->pVertexBuffer1;
            Gfx_UnlockVertexBuffer(pVB);
            if (n > 0) {
                Gfx_DrawPrimitiveVB(GFX_TRIANGLELIST, CGraphics::m_pTextureManager->pVertexBuffer1, 0, n);
                n = 0;
            }
            pData = (BYTE *)Gfx_LockVertexBuffer(CGraphics::m_pTextureManager->pVertexBuffer1);
            if (pLastTexture != pQuad->pTexture) {
                QUAD2D_SET_STATE(pQuad);
            }
            if (lastFlags != pQuad->flags) {
                QUAD2D_SET_FLAGS(pQuad);
            }
            pLastTexture = pQuad->pTexture;
            lastFlags = pQuad->flags;
        }
        *(Quad2DVertices *)(pData + n * 0x30) = pQuad->verts;
        n += 3;
        CGame::m_unk0x0059ce20++;
        pQuad++;
    }
    Gfx_UnlockVertexBuffer(CGraphics::m_pTextureManager->pVertexBuffer1);
    if (n > 0) {
        if (layer & 8)
            pLast = &g_quad2DLayerA[count - 1];
        else if (layer & 0x10)
            pLast = &g_quad2DLayerB[count - 1];
        else if (layer & 0x20)
            pLast = &g_quad2DLayerC[count - 1];
        else
            pLast = &g_quad2DLayerD[count - 1];
        QUAD2D_SET_STATE(pLast);
        QUAD2D_SET_FLAGS(pLast);
        Gfx_DrawPrimitiveVB(GFX_TRIANGLELIST, CGraphics::m_pTextureManager->pVertexBuffer1, 0, n);
    }
    CGraphics::ApplyTextureStageChange(0, 0);
    CGraphics::SetZEnable(1);
    CGraphics::SetZWriteEnable(1);
    CGraphics::SetCullMode(CGame::GetSectorDrawState());
    Graphics_InvalidateTextureStageCache();
    CGraphics::InvalidateBlendStateCache();
}

// Converts three fixed-point vertices and queues them in the selected layer.
// FUNCTION: CMR2 0x004bba00
void Quad2D_QueueFixedTriangle(int, Quad2DInputVertex *pA,
                               Quad2DInputVertex *pB,
                               Quad2DInputVertex *pC, Texture *pTexture,
                               UINT_PTR pDest)
{
    Quad2D *p;

    if (pDest & 8) {
        if (g_quad2DCountA >= 0x400) {
            if (g_quad2DConvertOverflow == 0)
                g_quad2DConvertOverflow = 1;
            return;
        }
        p = &g_quad2DLayerA[g_quad2DCountA];
        g_quad2DCountA++;
    } else if (pDest & 0x10) {
        if (g_quad2DCountB >= 0x200) {
            if (g_quad2DConvertOverflow == 0)
                g_quad2DConvertOverflow = 1;
            return;
        }
        p = &g_quad2DLayerB[g_quad2DCountB];
        g_quad2DCountB++;
    } else if (pDest & 0x20) {
        if (g_quad2DCountC >= QUAD2D_LAYER_C_MAX) {
            if (g_quad2DConvertOverflow == 0)
                g_quad2DConvertOverflow = 1;
            return;
        }
        p = &g_quad2DLayerC[g_quad2DCountC];
        g_quad2DCountC++;
    } else if (pDest & 0x40) {
        if (g_quad2DCountD >= 1) {
            if (g_quad2DConvertOverflow == 0)
                g_quad2DConvertOverflow = 1;
            return;
        }
        p = &g_quad2DLayerD[g_quad2DCountD];
        g_quad2DCountD++;
    } else {
        p = (Quad2D *)pDest;
    }

    p->pTexture = pTexture;
    p->flags = (unsigned int)pDest;

    p->verts.v[0].x = (float)(X87S_MUL(pA->x, CGraphics::m_oneOver65536));
    p->verts.v[0].y = (float)(X87S_MUL(pA->y, CGraphics::m_oneOver65536));
    p->verts.v[0].z = (float)(X87S_MUL(pA->z, CGraphics::m_oneOver65536));
    p->verts.v[0].u = (float)(X87S_MUL(pA->u, CGraphics::m_oneOver65536));
    p->verts.v[0].v = (float)(X87S_MUL(pA->v, CGraphics::m_oneOver65536));
    p->verts.v[0].colour = GFX_RGBA(pA->colour[0], pA->colour[1],
                                      pA->colour[2], pA->colour[3]);
    p->verts.v[0].specular = 0xff000000;

    p->verts.v[1].x = (float)(X87S_MUL(pB->x, CGraphics::m_oneOver65536));
    p->verts.v[1].y = (float)(X87S_MUL(pB->y, CGraphics::m_oneOver65536));
    p->verts.v[1].z = (float)(X87S_MUL(pB->z, CGraphics::m_oneOver65536));
    p->verts.v[1].u = (float)(X87S_MUL(pB->u, CGraphics::m_oneOver65536));
    p->verts.v[1].v = (float)(X87S_MUL(pB->v, CGraphics::m_oneOver65536));
    p->verts.v[1].colour = GFX_RGBA(pB->colour[0], pB->colour[1],
                                      pB->colour[2], pB->colour[3]);
    p->verts.v[1].specular = 0xff000000;

    p->verts.v[2].x = (float)(X87S_MUL(pC->x, CGraphics::m_oneOver65536));
    p->verts.v[2].y = (float)(X87S_MUL(pC->y, CGraphics::m_oneOver65536));
    p->verts.v[2].z = (float)(X87S_MUL(pC->z, CGraphics::m_oneOver65536));
    p->verts.v[2].u = (float)(X87S_MUL(pC->u, CGraphics::m_oneOver65536));
    p->verts.v[2].v = (float)(X87S_MUL(pC->v, CGraphics::m_oneOver65536));
    p->verts.v[2].colour = GFX_RGBA(pC->colour[0], pC->colour[1],
                                      pC->colour[2], pC->colour[3]);
    p->verts.v[2].specular = 0xff000000;
}

// PORT: the in-race HUD on any screen. The HUD lays itself out for a 4:3
// screen: its textures and fonts keep their size in pixels for 640x480, or
// for 1024x768 with the high-resolution set, at positions taken as fractions
// of the screen (resX * 0xf333 >> 16) and panels sized from its width
// (resX * 75 / 640). On a larger screen it shrinks, and on a wider one its
// elements spread apart and its panels stretch around their text.
//
// Between Hud_BeginView and Hud_EndView the game draws a view's HUD on that
// 4:3 screen: resX, resY and the view rectangles are those of 640x480 or
// 1024x768. Each element (the 2D queued up to Hud_Element) is then placed on
// the real view: scaled with its height, by [video] hud_scale percent, and
// moved to the edge or the middle it is nearest to, within the widest shape
// [video] hud_aspect allows. hud_scale = 0 keeps the original's layout.

int *RallyData_GetViewScreenRectangle(int view);

struct HudFrame {
    int active;
    int view;
    int resX, resY;                     // the real screen
    float vx0, vy0, vx1, vy1;           // the view on the HUD's 4:3 screen
    float rx0, ry0, rx1, ry1;           // where its HUD goes on the real one
    float scale;
    unsigned int sprites[4], tris[4];   // queued before the current element
};

static HudFrame g_hud;

#define HUD_MIN(a, b) ((a) < (b) ? (a) : (b))
#define HUD_MAX(a, b) ((a) > (b) ? (a) : (b))

static unsigned int *const g_hudSpriteCounts[4] = { &g_spriteCount1, &g_spriteCount2, &g_spriteCount3, &g_spriteCount4 };
static Sprite *const g_hudSpriteLayers[4] = { g_spriteLayer1, g_spriteLayer2, g_spriteLayer3, g_spriteLayer4 };
static unsigned int *const g_hudTriCounts[4] = { &g_tri2DCount1, &g_tri2DCount2, &g_tri2DCount3, &g_tri2DCount4 };
static Tri2D *const g_hudTriLayers[4] = { g_tri2DLayer1, g_tri2DLayer2, g_tri2DLayer3, g_tri2DLayer4 };

// Widest width / height the HUD spreads over: "16:9", "2.39", or "full".
static float Hud_MaxAspect(void)
{
    const char *text = Sys_GetOptionText("video.hud_aspect", "16:9");
    float w = 0.0f, h = 1.0f;
    if (sscanf(text, "%f:%f", &w, &h) < 1 || w <= 0.0f || h <= 0.0f)
        return 0.0f;
    return w / h < 4.0f / 3.0f ? 4.0f / 3.0f : w / h;
}

static void Hud_MarkQueues(void)
{
    for (int i = 0; i < 4; i++) {
        g_hud.sprites[i] = *g_hudSpriteCounts[i];
        g_hud.tris[i] = *g_hudTriCounts[i];
    }
}

// Whether the game uses its high-resolution textures and fonts, drawn for
// 1024x768 rather than 640x480.
static int Sprite_HighResolutionSet(void)
{
    return CGameInfo::GetScreenWidth() >= 0x400 && CFrontend::IsTextureWidthSupported(0x400) &&
           CFrontend::IsTextureHeightSupported(0x400);
}

// Gives g_pGraphics the size of the HUD's 4:3 screen, keeping the real one.
// Returns 0 when the HUD keeps the original's layout (hud_scale = 0).
static int Hud_SetScreen(int *pResX, int *pResY)
{
    if (Sys_GetOption("video.hud_scale", 100) <= 0)
        return 0;
    *pResX = g_pGraphics->resX;
    *pResY = g_pGraphics->resY;
    if (Sprite_HighResolutionSet()) {
        g_pGraphics->resX = 1024;
        g_pGraphics->resY = 768;
    } else {
        g_pGraphics->resX = 640;
        g_pGraphics->resY = 480;
    }
    return 1;
}

// Layouts the game works out before drawing the HUD (the dash's gear marker,
// the split bars) are made on its 4:3 screen too.
static struct {
    int active;
    int resX, resY;
} g_hudLayout;

void Hud_BeginLayout(void)
{
    if (!g_hud.active && !g_hudLayout.active)
        g_hudLayout.active = Hud_SetScreen(&g_hudLayout.resX, &g_hudLayout.resY);
}

void Hud_EndLayout(void)
{
    if (g_hudLayout.active) {
        g_pGraphics->resX = g_hudLayout.resX;
        g_pGraphics->resY = g_hudLayout.resY;
        g_hudLayout.active = 0;
    }
}

void Hud_BeginView(int view)
{
    short *pReal;
    short *pVirtual;
    float aspect;
    float maxWidth;
    float k;

    if (g_hud.active || g_hudLayout.active)
        return;
    pReal = (short *)RallyData_GetViewScreenRectangle(view);
    g_hud.rx0 = pReal[0];
    g_hud.ry0 = pReal[1];
    g_hud.rx1 = pReal[0] + pReal[2];
    g_hud.ry1 = pReal[1] + pReal[3];
    if (!Hud_SetScreen(&g_hud.resX, &g_hud.resY))
        return;
    pVirtual = (short *)RallyData_GetViewScreenRectangle(view);
    g_hud.vx0 = pVirtual[0];
    g_hud.vy0 = pVirtual[1];
    g_hud.vx1 = pVirtual[0] + pVirtual[2];
    g_hud.vy1 = pVirtual[1] + pVirtual[3];
    // On a screen wider than hud_aspect the HUD keeps to the middle.
    aspect = Hud_MaxAspect();
    if (aspect > 0.0f) {
        maxWidth = (g_hud.ry1 - g_hud.ry0) * aspect * 0.75f * (g_hud.vx1 - g_hud.vx0) / (g_hud.vy1 - g_hud.vy0);
        if (g_hud.rx1 - g_hud.rx0 > maxWidth) {
            g_hud.rx0 = (g_hud.rx0 + g_hud.rx1 - maxWidth) * 0.5f;
            g_hud.rx1 = g_hud.rx0 + maxWidth;
        }
    }
    k = (g_hud.rx1 - g_hud.rx0) / (g_hud.vx1 - g_hud.vx0);
    if ((g_hud.ry1 - g_hud.ry0) / (g_hud.vy1 - g_hud.vy0) < k)
        k = (g_hud.ry1 - g_hud.ry0) / (g_hud.vy1 - g_hud.vy0);
    g_hud.scale = X87S_DIV(X87S_MUL(k, X87S_FILD(float, Sys_GetOption("video.hud_scale", 100))), 100.0f);
    g_hud.view = view;
    g_hud.active = 1;
    Hud_MarkQueues();
}

// Maps v from [v0, v1] on the HUD's screen to [r0, r1] on the real one,
// keeping its distance to the start, the middle or the end.
static float Hud_Map(float v, float v0, float v1, float r0, float r1, int anchor)
{
    if (anchor < 0)
        return r0 + (v - v0) * g_hud.scale;
    if (anchor > 0)
        return r1 - (v1 - v) * g_hud.scale;
    return (r0 + r1) * 0.5f + (v - (v0 + v1) * 0.5f) * g_hud.scale;
}

// The edge (-1, 1) or the middle (0) the span [lo, hi] is nearest to.
static int Hud_Anchor(float lo, float hi, float v0, float v1)
{
    float centre = (lo + hi) * 0.5f;
    float third = (v1 - v0) / 3.0f;
    if (centre < v0 + third)
        return -1;
    if (centre > v1 - third)
        return 1;
    return 0;
}

void Hud_Element(void)
{
    float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
    int ax, ay;
    int i;
    unsigned int n;

    if (!g_hud.active)
        return;
    for (i = 0; i < 4; i++) {
        for (n = g_hud.sprites[i]; n < *g_hudSpriteCounts[i]; n++) {
            Sprite *p = &g_hudSpriteLayers[i][n];
            x0 = HUD_MIN(x0, (float)p->dst.x);
            y0 = HUD_MIN(y0, (float)p->dst.y);
            x1 = HUD_MAX(x1, (float)(p->dst.x + p->dst.w));
            y1 = HUD_MAX(y1, (float)(p->dst.y + p->dst.h));
        }
        for (n = g_hud.tris[i]; n < *g_hudTriCounts[i]; n++) {
            for (int v = 0; v < 3; v++) {
                GfxTLVertex *p = &g_hudTriLayers[i][n].v[v];
                x0 = HUD_MIN(x0, p->sx);
                y0 = HUD_MIN(y0, p->sy);
                x1 = HUD_MAX(x1, p->sx);
                y1 = HUD_MAX(y1, p->sy);
            }
        }
    }
    if (x0 > x1) {
        Hud_MarkQueues();
        return;
    }
    ax = Hud_Anchor(x0, x1, g_hud.vx0, g_hud.vx1);
    ay = Hud_Anchor(y0, y1, g_hud.vy0, g_hud.vy1);
#define HUD_X(x) Hud_Map((x), g_hud.vx0, g_hud.vx1, g_hud.rx0, g_hud.rx1, ax)
#define HUD_Y(y) Hud_Map((y), g_hud.vy0, g_hud.vy1, g_hud.ry0, g_hud.ry1, ay)
    for (i = 0; i < 4; i++) {
        for (n = g_hud.sprites[i]; n < *g_hudSpriteCounts[i]; n++) {
            Sprite *p = &g_hudSpriteLayers[i][n];
            short left = (short)lroundf(HUD_X(p->dst.x));
            short top = (short)lroundf(HUD_Y(p->dst.y));
            p->dst.w = (short)lroundf(HUD_X(p->dst.x + p->dst.w)) - left;
            p->dst.h = (short)lroundf(HUD_Y(p->dst.y + p->dst.h)) - top;
            p->dst.x = left;
            p->dst.y = top;
            // (-1, -1, -1) is the sprite's middle, wherever it goes.
            if (p->centre[0] != -1 || p->centre[1] != -1 || p->centre[2] != -1) {
                p->centre[0] = (int)lroundf(HUD_X(p->centre[0]));
                p->centre[1] = (int)lroundf(HUD_Y(p->centre[1]));
            }
        }
        for (n = g_hud.tris[i]; n < *g_hudTriCounts[i]; n++) {
            for (int v = 0; v < 3; v++) {
                GfxTLVertex *p = &g_hudTriLayers[i][n].v[v];
                p->sx = HUD_X(p->sx);
                p->sy = HUD_Y(p->sy);
            }
        }
    }
#undef HUD_X
#undef HUD_Y
    Hud_MarkQueues();
}

void Hud_EndView(void)
{
    if (!g_hud.active)
        return;
    Hud_Element();
    g_pGraphics->resX = g_hud.resX;
    g_pGraphics->resY = g_hud.resY;
    g_hud.active = 0;
    RallyData_GetViewScreenRectangle(g_hud.view);
}

// PORT: the menus on any screen. The menus lay themselves out at positions
// taken as fractions of the screen, with textures and fonts in pixels for
// 640x480, or 1024x768 with the high-resolution set, so they shrink as the
// screen grows. Between MenuScreen_Begin and MenuScreen_End the game draws
// them on a screen of the real one's shape, as many lines high as that
// 640x480 or 1024x768 screen at [video] menu_scale = 100 (fewer, so larger,
// above 100), and at least as wide; the renderer scales it to the back
// buffer, whose pixels are still rasterised at their resolution, 3D included
// (Gfx_SetVirtualScreen). The menus take the same part of the screen at any
// resolution. menu_scale = 0 keeps the original's layout in pixels.

void FrontendScroller_RecomputeAllAttached(void);

static struct {
    int active;
    int depth;                  // MenuScreen_Begin calls inside the first
    int resX, resY;             // the real screen
    unsigned int modeX, modeY;  // the display mode it was set from
    int screenX, screenY;       // the menus' screen
} g_menuScreen;

// The menus' screen of the last frame: layouts the menus keep from frame to
// frame are worked out again when it changes.
static int g_menuScreenLastX, g_menuScreenLastY;

// The size of the menus' screen on a real one of realX x realY.
static void MenuScreen_Size(int realX, int realY, int *pX, int *pY)
{
    int scale = Sys_GetOption("video.menu_scale", 100);
    int baseX, baseY;

    *pX = realX;
    *pY = realY;
    if (scale > 0 && realX > 0 && realY > 0) {
        baseX = Sprite_HighResolutionSet() ? 1024 : 640;
        baseY = baseX * 3 / 4;
        *pY = (int)lround(X87S_DIV(X87S_MUL((double)baseY, 100.0), scale));
        // No narrower than the original's screen: the menus keep its widths.
        if ((long long)*pY * realX < (long long)baseX * realY)
            *pY = (int)(((long long)baseX * realY + realX - 1) / realX);
        *pX = (int)lround(X87S_DIV(X87S_MUL((double)*pY, realX), realY));
    }
}

void MenuScreen_Begin(void)
{
    int realX, realY;
    int x, y;

    if (g_menuScreen.active) {
        g_menuScreen.depth++;
        return;
    }
    if (g_pGraphics == NULL)
        return;
    realX = (int)g_pGraphics->resX;
    realY = (int)g_pGraphics->resY;
    MenuScreen_Size(realX, realY, &x, &y);
    if (x != realX || y != realY) {
        g_menuScreen.active = 1;
        g_menuScreen.resX = realX;
        g_menuScreen.resY = realY;
        g_menuScreen.modeX = CGameInfo::GetScreenWidth();
        g_menuScreen.modeY = CGameInfo::GetScreenHeight();
        g_menuScreen.screenX = x;
        g_menuScreen.screenY = y;
        g_pGraphics->resX = x;
        g_pGraphics->resY = y;
        Gfx_SetVirtualScreen(x, y);
        g_spriteScreenScale = X87S_DIV(X87S_FILD(float, realY), X87S_LOAD(y));
    }
    if (x != g_menuScreenLastX || y != g_menuScreenLastY) {
        // The first frame's are those the menus were built on.
        if (g_menuScreenLastX != 0)
            FrontendScroller_RecomputeAllAttached();
        g_menuScreenLastX = x;
        g_menuScreenLastY = y;
    }
}

void MenuScreen_End(void)
{
    if (!g_menuScreen.active)
        return;
    if (g_menuScreen.depth > 0) {
        g_menuScreen.depth--;
        return;
    }
    // A display mode chosen in the menus keeps its size.
    if (CGameInfo::GetScreenWidth() == g_menuScreen.modeX && CGameInfo::GetScreenHeight() == g_menuScreen.modeY) {
        g_pGraphics->resX = g_menuScreen.resX;
        g_pGraphics->resY = g_menuScreen.resY;
    }
    Gfx_SetVirtualScreen(0, 0);
    g_spriteScreenScale = 1.0f;
    g_menuScreen.active = 0;
}

// PORT: a boot screen drawn in the real screen's pixels while the menus draw
// on their own: the copyright screen, whose text is in its pictures, stays
// sharp only unscaled.
static int g_menuScreenSuspended;

void MenuScreen_Suspend(void)
{
    if (!g_menuScreen.active || g_menuScreenSuspended)
        return;
    g_menuScreenSuspended = 1;
    g_pGraphics->resX = g_menuScreen.resX;
    g_pGraphics->resY = g_menuScreen.resY;
    Gfx_SetVirtualScreen(0, 0);
    g_spriteScreenScale = 1.0f;
}

void MenuScreen_Resume(void)
{
    if (!g_menuScreenSuspended)
        return;
    g_menuScreenSuspended = 0;
    g_pGraphics->resX = g_menuScreen.screenX;
    g_pGraphics->resY = g_menuScreen.screenY;
    Gfx_SetVirtualScreen(g_menuScreen.screenX, g_menuScreen.screenY);
    g_spriteScreenScale = X87S_DIV(X87S_FILD(float, g_menuScreen.resY), X87S_LOAD(g_menuScreen.screenY));
}

// The largest menu_scale the display takes: above it the menus' screen
// would be narrower than the original's, which they keep (see MenuScreen_Size).
int MenuScreen_GetLargestScale(void)
{
    int realX, realY;
    int baseX;

    MenuScreen_GetDisplaySize(&realX, &realY);
    if (realX <= 0 || realY <= 0)
        return 100;
    baseX = Sprite_HighResolutionSet() ? 1024 : 640;
    return (int)((long long)baseX * 3 / 4 * 100 * realX / ((long long)baseX * realY));
}

void MenuScreen_GetDisplaySize(int *pWidth, int *pHeight)
{
    *pWidth = g_menuScreen.active ? g_menuScreen.resX : (int)g_pGraphics->resX;
    *pHeight = g_menuScreen.active ? g_menuScreen.resY : (int)g_pGraphics->resY;
}

// PORT: the menus over a race (pause, results) on the menus' screen. The
// race and its HUD are drawn on the real screen, in the same queues, so the
// 2D the menus queue between MenuScreen_BeginOverlay and MenuScreen_EndOverlay
// is laid out on the menus' screen and then scaled to the real one.
static struct {
    int active;
    int resX, resY;             // the real screen
    float kx, ky;               // real pixels per pixel of the menus' screen
    unsigned int sprites[4], tris[4];
} g_menuOverlay;

void MenuScreen_BeginOverlay(void)
{
    int x, y;

    if (g_menuOverlay.active || g_menuScreen.active || g_hud.active || g_pGraphics == NULL)
        return;
    MenuScreen_Size((int)g_pGraphics->resX, (int)g_pGraphics->resY, &x, &y);
    if (x == (int)g_pGraphics->resX && y == (int)g_pGraphics->resY)
        return;
    g_menuOverlay.active = 1;
    g_menuOverlay.resX = g_pGraphics->resX;
    g_menuOverlay.resY = g_pGraphics->resY;
    g_menuOverlay.kx = X87S_DIV(X87S_FILD(float, g_menuOverlay.resX), X87S_LOAD(x));
    g_menuOverlay.ky = X87S_DIV(X87S_FILD(float, g_menuOverlay.resY), X87S_LOAD(y));
    for (int i = 0; i < 4; i++) {
        g_menuOverlay.sprites[i] = *g_hudSpriteCounts[i];
        g_menuOverlay.tris[i] = *g_hudTriCounts[i];
    }
    g_pGraphics->resX = x;
    g_pGraphics->resY = y;
}

void MenuScreen_EndOverlay(void)
{
    unsigned int n;

    if (!g_menuOverlay.active)
        return;
    const float kx = g_menuOverlay.kx, ky = g_menuOverlay.ky;
    for (int i = 0; i < 4; i++) {
        for (n = g_menuOverlay.sprites[i]; n < *g_hudSpriteCounts[i]; n++) {
            Sprite *p = &g_hudSpriteLayers[i][n];
            short left = (short)lroundf(X87S_MUL(X87S_LOAD(p->dst.x), kx));
            short top = (short)lroundf(X87S_MUL(X87S_LOAD(p->dst.y), ky));
            p->dst.w = (short)lroundf(X87S_MUL(X87S_LOAD((p->dst.x + p->dst.w)), kx)) - left;
            p->dst.h = (short)lroundf(X87S_MUL(X87S_LOAD((p->dst.y + p->dst.h)), ky)) - top;
            p->dst.x = left;
            p->dst.y = top;
            // (-1, -1, -1) is the sprite's middle, wherever it goes.
            if (p->centre[0] != -1 || p->centre[1] != -1 || p->centre[2] != -1) {
                p->centre[0] = (int)lroundf(X87S_MUL(X87S_LOAD(p->centre[0]), kx));
                p->centre[1] = (int)lroundf(X87S_MUL(X87S_LOAD(p->centre[1]), ky));
            }
        }
        for (n = g_menuOverlay.tris[i]; n < *g_hudTriCounts[i]; n++)
            for (int v = 0; v < 3; v++) {
                g_hudTriLayers[i][n].v[v].sx *= kx;
                g_hudTriLayers[i][n].v[v].sy *= ky;
            }
    }
    g_pGraphics->resX = g_menuOverlay.resX;
    g_pGraphics->resY = g_menuOverlay.resY;
    g_menuOverlay.active = 0;
}
