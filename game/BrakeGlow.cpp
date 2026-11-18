// Brake disc glow renderer (video.brake_glow, default on). Visual only: reads the
// car's brake pedal, bias and speed once per consumed physics step, keeps a per-wheel heat
// (BrakeGlow.h, unit-tested) and draws an additive radial quad on the disc plane of
// each hot wheel during the 3D pass. Nothing here feeds physics, the replay stream
// or any original render state; all state changes sit inside Push/PopState.
#include <string.h>
#include <math.h>
#include "port/gfx.h"
#include "port/sys.h"
#include "Car.h"
#include "SceneNode.h"
#include "Mesh.h"
#include "MeshVertex.h"
#include "BrakeGlow.h"

#define BRAKE_GLOW_MAX_CARS 48   // g_carOrder capacity

static float s_heat[BRAKE_GLOW_MAX_CARS][4];
static float s_extent[BRAKE_GLOW_MAX_CARS][4];   // measured wheel mesh radius per wheel
static int s_extentValid[BRAKE_GLOW_MAX_CARS];
static GfxTexture *s_glowTex;
static bool s_texFailed;

// Radial mesh extent of one wheel node: the tyre radius off the mesh vertices
// (BrakeGlow_MeshExtent). Measured once per car; the glow quad scales off it
// (BrakeGlow_DiscRadius) so it stays inside the hubcap of any wheel model.
// Returns 0 when unreadable.
static float BrakeGlow_WheelExtent(SceneNode *node)
{
    if (node == NULL || node->pObject == NULL)
        return 0.f;
    Mesh *mesh = (Mesh *)node->pObject;
    return BrakeGlow_MeshExtent(mesh->pVertexData, (int)sizeof(MeshVertexF), mesh->field_0x10);
}

// 64x64 radial falloff, white RGB so the vertex diffuse carries the hot colour;
// the alpha does the soft edge under additive blending.
static GfxTexture *BrakeGlow_Texture(void)
{
    if (s_texFailed)
        return NULL;
    if (s_glowTex != NULL)
        return s_glowTex;
    GfxTexture *tex = Gfx_CreateTexture(64, 64, GFX_FORMAT_BGRA8, 1, 0);
    if (tex == NULL) {
        s_texFailed = true;
        return NULL;
    }
    GfxLockedRect lr;
    if (!Gfx_LockTexture(tex, 0, 0, &lr)) {
        Gfx_DestroyTexture(tex);   // no per-frame leak on a failed lock; remember it
        s_texFailed = true;
        return NULL;
    }
    for (int y = 0; y < 64; y++) {
        BYTE *row = (BYTE *)lr.pixels + y * lr.pitch;
        for (int x = 0; x < 64; x++) {
            float dx = (x + 0.5f - 32.f) / 32.f, dy = (y + 0.5f - 32.f) / 32.f;
            float d = sqrtf(dx * dx + dy * dy);
            float fall = d >= 1.f ? 0.f : 1.f - d * d;
            row[x * 4 + 0] = 0xff;
            row[x * 4 + 1] = 0xff;
            row[x * 4 + 2] = 0xff;
            row[x * 4 + 3] = (BYTE)(fall * 255.f);
        }
    }
    Gfx_UnlockTexture(tex, 0, 0);
    s_glowTex = tex;
    return s_glowTex;
}

// One physics step for one car (called from Car_DecrementContactTimers, once per
// consumed step): fold the current pedal/bias/speed into the heat model. dtSteps is
// that step's duration in model units (1.0 at the original 25 Hz rate), so heat gain
// tracks the simulation, not the render rate; paused frames consume no steps and
// change no heat.
void BrakeGlow_TickStep(int car, float dtSteps)
{
    if (car < 0 || car >= BRAKE_GLOW_MAX_CARS || dtSteps <= 0.f)
        return;
    if (Sys_GetOption("video.brake_glow", 1) == 0)
        return;
    Car *c = &g_carBuffer[car];
    BrakeGlow_UpdateHeat(s_heat[car], c->brakeInput, c->brakeBias, c->speed, dtSteps);
}

// Draws every hot disc in the current view. Called at world-end of the 3D pass
// (Game.cpp, before Gfx_FlushDustVolume): depth tested against the car and world,
// no depth write, additive so overlapping discs and dust brighten rather than occlude.
void BrakeGlow_Draw(void)
{
    if (Sys_GetOption("video.brake_glow", 1) == 0)
        return;
    // Car_FreeTable releases the car table without clearing g_carOrderCount, and
    // menu scenes reuse this 3D pass: never index a freed table.
    if (g_carBuffer == NULL || g_carCount <= 0 || g_carOrderCount <= 0)
        return;
    GfxTexture *tex = BrakeGlow_Texture();
    if (tex == NULL)
        return;

    static GfxVertex verts[BRAKE_GLOW_MAX_CARS * 4 * 6];
    int count = 0;
    for (int order = 0; order < g_carOrderCount && order < BRAKE_GLOW_MAX_CARS; order++) {
        int car = g_carOrder[order];
        if (car < 0 || car >= BRAKE_GLOW_MAX_CARS || car >= g_carCount)
            continue;
        for (int w = 0; w < 4; w++) {
            SceneNode *node = g_carBuffer[car].pWheelNodes[w];
            if (node == NULL)
                continue;
            if (s_extentValid[car] == 0) {
                for (int k = 0; k < 4; k++)
                    s_extent[car][k] = BrakeGlow_WheelExtent(g_carBuffer[car].pWheelNodes[k]);
                BrakeGlow_FillMissingExtents(s_extent[car]);
                s_extentValid[car] = 1;
                if (Sys_GetOption("video.brake_glow_debug", 0) != 0)
                    Sys_Log("brake glow: car %d wheel extents %.3f %.3f %.3f %.3f",
                            car, s_extent[car][0], s_extent[car][1], s_extent[car][2], s_extent[car][3]);
            }
            float radius = s_extent[car][w] > 0.f ? BrakeGlow_DiscRadius(s_extent[car][w]) : BRAKE_GLOW_RADIUS;
            float pos[4][3], uv[4][2], alpha;
            if (!BrakeGlow_BuildDiscQuad(node->worldF, radius, s_heat[car][w],
                                         pos, uv, &alpha))
                continue;
            int a = (int)(alpha * 255.f);
            if (a > 255) a = 255;
            // Dull red at the edge of the heat range toward bright orange when hot.
            DWORD diffuse = ((DWORD)a << 24) | 0xff0000 |
                            ((DWORD)(0x30 + (int)(alpha * 0x90)) << 8) | 0x18;
            static const int kTri[6] = { 0, 1, 2, 0, 2, 3 };
            for (int t = 0; t < 6; t++) {
                int v = kTri[t];
                GfxVertex *d = &verts[count++];
                memset(d, 0, sizeof *d);
                d->x = pos[v][0]; d->y = pos[v][1]; d->z = pos[v][2];
                d->diffuse = diffuse;
                d->u0 = uv[v][0]; d->v0 = uv[v][1];
            }
        }
    }
    if (count == 0)
        return;

    Gfx_PushState();
    GfxMatrix identity = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
    Gfx_SetTransform(GFX_TRANSFORM_WORLD, &identity);   // verts are already world space
    Gfx_SetLighting(FALSE);
    Gfx_SetFog(FALSE);
    Gfx_SetAlphaTest(FALSE);
    Gfx_SetDepthTest(TRUE);
    Gfx_SetDepthWrite(FALSE);
    Gfx_SetCullMode(GFX_CULL_NONE);
    Gfx_SetAlphaBlend(TRUE);
    Gfx_SetSrcBlend(GFX_BLEND_SRCALPHA);
    Gfx_SetDestBlend(GFX_BLEND_ONE);
    Gfx_SetStageColorOp(0, GFX_TOP_MODULATE);
    Gfx_SetStageColorArg1(0, GFX_TA_TEXTURE);
    Gfx_SetStageColorArg2(0, GFX_TA_DIFFUSE);
    Gfx_SetStageAlphaOp(0, GFX_TOP_MODULATE);
    Gfx_SetStageAlphaArg1(0, GFX_TA_TEXTURE);
    Gfx_SetStageAlphaArg2(0, GFX_TA_DIFFUSE);
    Gfx_SetStageColorOp(1, GFX_TOP_DISABLE);
    Gfx_SetStageAlphaOp(1, GFX_TOP_DISABLE);
    Gfx_SetStageTexCoordIndex(0, 0);
    Gfx_SetStageTextureTransform(0, GFX_TTFF_DISABLE);
    Gfx_SetStageAddress(0, GFX_ADDRESS_CLAMP);
    Gfx_SetTexture(0, tex);
    Gfx_DrawPrimitive(GFX_TRIANGLELIST, verts, count);
    Gfx_SetTexture(0, NULL);
    Gfx_PopState();
}

// Cold discs at every session/restart boundary (called beside Windshield_ResetAll).
void BrakeGlow_Reset(void)
{
    memset(s_heat, 0, sizeof s_heat);
    for (int c = 0; c < BRAKE_GLOW_MAX_CARS; c++)
        s_extentValid[c] = 0;
}
