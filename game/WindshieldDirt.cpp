// Glue between the windshield dirt map (WindshieldDirt.h) and the game:
// per-frame accumulation/wetness/wiping state and the cockpit-only overlay
// draw. The overlay replaces the old flat full-screen tint and the white
// 1-pixel wiper bar: dirt shows where it actually is, and the clearing arc
// tracks the cockpit model's real blade sweep (g_carWiperStates + the CIN
// profile's max angles, the same values CarInterior_AnimateWipers uses), so
// the glass clears exactly under the model's blades. Drawn through the TL
// path (both renderers, no new shader). Render-only.
#include <string.h>
#include "port/gfx.h"
#include "Graphics.h"
#include "CarInterior.h"
#include "CarParts.h"
#include "StageBlock.h"
#include "WindshieldDirt.h"
#include "GraphicsOptions.h"
#include "port/sys.h"

WindshieldMap g_windshieldMaps[WINDSHIELD_MAX_VIEWS];
// Per-view state of the resolved front glass (0 intact, 1 damaged, 2 broken,
// -1 feature off): drives the broken-glass overlay suppression.
static int g_windshieldGlassState[WINDSHIELD_MAX_VIEWS];
static int g_windshieldSideState[2][2];   // [view][slot 2/3]: 0 intact, 1 damaged, 2 broken
// F10 diagnostics: how often the overlay draw was reached and actually drawn per view, and the last gating reason
// (0 drawing, 1 broken glass, 2 no dirt). Counters only; no behaviour change.
static unsigned g_wsDrawCalls[WINDSHIELD_MAX_VIEWS], g_wsDrawn[WINDSHIELD_MAX_VIEWS];
static int g_wsLastReason[WINDSHIELD_MAX_VIEWS] = { -1, -1, -1, -1, -1, -1, -1, -1 };

void Windshield_GetDiag(int view, unsigned *calls, unsigned *drawn, int *reason, int *glassState, int *dirtTotal)
{
    int v = view & (WINDSHIELD_MAX_VIEWS - 1);
    if (calls) *calls = g_wsDrawCalls[v];
    if (drawn) *drawn = g_wsDrawn[v];
    if (reason) *reason = g_wsLastReason[v];
    if (glassState) *glassState = g_windshieldGlassState[v];
    if (dirtTotal) *dirtTotal = Windshield_TotalDirt(&g_windshieldMaps[v]);
}


int StageObject_GetViewWeatherField54(int index);
CarPartSet *StageTiming_GetCarReplayRecord(int index);

// Blade geometry in overlay uv: pivots below the dash line, a symmetric pair.
// The models sweep their own nodes about their own pivots; this overlay
// tracks their normalised angle, not their exact pivot - documented
// approximation, the per-model pivot projection is a native-asset follow-up.
static const float kPivotU[2] = { 0.34f, 0.66f };
static const float kPivotV = 1.02f;
static const float kBladeRadius = 0.72f;
static const float kBladeHalfWidth = 0.05f;   // radians
static const float kBladeMaxAngle = 1.15f;    // radians of sweep at full arc

// 1 when the resolved front glass of this car is in the broken damage state (partBroken). Read straight from the car's part record, so it
// does not depend on the visual options (video.windshield_cracks, the crack overlay). Cracked/damaged only, intact and an unresolved
// windshield return 0, so the wipers keep running for anything but a broken windshield.
int Windshield_FrontGlassBroken(int slot)
{
    int wslot = -1;
    if (slot < 0 || slot > 1 || !WindshieldGlass_GetInfo(slot, NULL, &wslot, NULL, NULL, NULL) || wslot < 0 || wslot >= 8) return 0;
    CarPartSet *parts = StageTiming_GetCarReplayRecord(slot);
    return parts != NULL && parts->partBroken[wslot] != 0 ? 1 : 0;
}

// Per-frame update for one view's map. `slot` is the cockpit slot (the wiper
// state and CIN profile tables hold two: one per split-screen cockpit view).
void Windshield_UpdateView(int view, int slot)
{
    static int frameCounter = 0;
    WindshieldMap *m = &g_windshieldMaps[view & (WINDSHIELD_MAX_VIEWS - 1)];
    int rain = StageObject_GetViewWeatherField54(slot);   // fixed, ~0..0x10000
    Windshield_UpdateWetness(m, rain * 255 / 0x8000);
    Windshield_Decay(m, frameCounter++);

    // Damage-linked windshield glass (video.windshield_cracks): the cockpit's
    // front interior glass follows the car's real damage appearance - the
    // same texture the exterior model swaps to, applied by WindshieldGlass to
    // the interior glass parts. The damage slot comes from the resolved front
    // window (wiper-pivot classified), never a hardcoded part index; cars the
    // resolve failed for keep their glass untouched. slot == local car index
    // (same mapping the wiper state uses). Independent of the wiper toggle.
    int glassState = -1;
    int diagWslot = -1, diagDamaged = -1, diagBroken = -1;
    if (Sys_GetOption("video.windshield_cracks", 1) != 0 && slot >= 0 && slot <= 1) {
        int wslot = -1;
        if (WindshieldGlass_GetInfo(slot, NULL, &wslot, NULL, NULL, NULL)) {
            CarPartSet *parts = StageTiming_GetCarReplayRecord(slot);
            if (parts != NULL) {
                diagWslot = wslot;
                diagBroken = parts->partBroken[wslot] != 0;
                diagDamaged = parts->partDamaged[wslot] != 0;
                glassState = diagBroken ? 2 : diagDamaged ? 1 : 0;
                WindshieldGlass_ApplyDamageState(slot, glassState);
                for (int sw = 0; sw < 2; sw++)   // side window damage slots 2 and 3
                    g_windshieldSideState[slot][sw] = parts->partBroken[2 + sw] != 0 ? 2 : parts->partDamaged[2 + sw] != 0 ? 1 : 0;
            }
        }
    }
    int mview = view & (WINDSHIELD_MAX_VIEWS - 1);
    if (g_windshieldGlassState[mview] != glassState) {
        // State-change diagnostics (low volume, transitions only): separates
        // "no cracks/dirt because the resolved window is already broken"
        // from selection and accumulation problems in native run logs.
        static const char *const kStateNames[4] = { "intact", "damaged", "broken", "off/unresolved" };
        int prev = g_windshieldGlassState[mview];
        Sys_Log("windshield glass: view %d slot %d state %s (was %s); wslot %d damaged %d broken %d; dirt total %d; video.windscreen_dirt %d",
                view, slot, kStateNames[glassState < 0 ? 3 : glassState],
                kStateNames[prev < 0 ? 3 : prev], diagWslot, diagDamaged, diagBroken,
                Windshield_TotalDirt(m), Sys_GetOption("video.windscreen_dirt", 40));
        g_windshieldGlassState[mview] = glassState;
    }

    if (Sys_GetOption("video.windscreen_wipers", 1) == 0 || slot < 0 || slot > 1)
        return;   // option off: the model's blades park; glass keeps its dirt
    const CarWiperState *w = &g_carWiperStates[slot];
    short *pMax = g_carInteriorProfilePointers[slot].firstWiperMaxAngle;
    if (w->mode == 0 || w->angle == 0 || pMax == NULL || *pMax <= 0)
        return;   // parked or no profile: no clearing
    float t = (float)w->angle / (float)*pMax;   // both blades share this t
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    int reversed = g_carWiperDirectionReversed[slot] != 0;
    for (int b = 0; b < 2; b++) {
        // Blades park pointing up; they sweep out toward their pillar.
        float a = (b == 0 ? -1.0f : 1.0f) * t * kBladeMaxAngle;
        if (reversed) a = -a;
        int dir = (w->sweepingForward ? 1 : -1) * (a >= 0.0f ? 1 : -1);
        if (dir == 0) dir = 1;
        Windshield_Wipe(m, kPivotU[b], kPivotV, kBladeRadius, a, kBladeHalfWidth, dir);
    }
}

// Adds driving spatters to a view's map (rain spray, mud, dust). The amount
// follows the old global-tint scaling; convert it into a few localised drops.
void Windshield_AddDrivingSpatter(int view, int amount)
{
    if (amount <= 0) return;
    WindshieldMap *m = &g_windshieldMaps[view & (WINDSHIELD_MAX_VIEWS - 1)];
    int drops = 1 + amount / 24;
    if (drops > 6) drops = 6;
    // Visibility: the old 40+amount strength / amount-only size left specks at
    // ~4% mean alpha (invisible on dark scenery). Stronger, larger specks.
    Windshield_AddSpatter(m, drops, 110 + amount, 0.6f + amount / 255.0f);
}

// ---- overlay draw ---------------------------------------------------------

static GfxTexture *g_windshieldTex;


extern Graphics *g_pGraphics;

// Draws the dirt map over the cockpit view. Called from the per-view race
// draw only when the view's camera is the cockpit (type 3); the HUD draws
// after this and stays clean. Alpha blend, the old tint's grime brown.
void Windshield_DrawOverlay(int view, int resX, int resY)
{
    WindshieldMap *m = &g_windshieldMaps[view & (WINDSHIELD_MAX_VIEWS - 1)];
    int mview = view & (WINDSHIELD_MAX_VIEWS - 1);
    // Draw-gating diagnostics (low volume, transitions only): why the dirt
    // overlay is not on screen - broken glass suppresses it, no dirt means
    // the accumulation sources never landed.
    int reason = !Windshield_OverlayVisible(g_windshieldGlassState[mview]) ? 1
               : Windshield_TotalDirt(m) == 0 ? 2 : 0;
    g_wsDrawCalls[mview]++;
    g_wsLastReason[mview] = reason;
    static int s_lastDrawReason[WINDSHIELD_MAX_VIEWS] = { -1, -1, -1, -1, -1, -1, -1, -1 };
    if (s_lastDrawReason[mview] != reason) {
        static const char *const kReasons[3] = { "drawing", "skipped: broken glass", "skipped: no dirt" };
        Sys_Log("windshield dirt overlay: view %d %s (glass state %d, dirt total %d)",
                view, kReasons[reason], g_windshieldGlassState[mview], Windshield_TotalDirt(m));
        s_lastDrawReason[mview] = reason;
    }
    if (reason != 0)
        return;
    g_wsDrawn[mview]++;
    if (g_windshieldTex == NULL) {
        g_windshieldTex = Gfx_CreateTexture(WINDSHIELD_MAP_W, WINDSHIELD_MAP_H, GFX_FORMAT_BGRA8, 1, 0);
        if (g_windshieldTex == NULL) return;
    }
    GfxLockedRect lr;
    if (!Gfx_LockTexture(g_windshieldTex, 0, 0, &lr)) return;
    for (int y = 0; y < WINDSHIELD_MAP_H; y++) {
        BYTE *row = (BYTE *)lr.pixels + y * lr.pitch;
        for (int x = 0; x < WINDSHIELD_MAP_W; x++) {
            int d = m->dirt[y * WINDSHIELD_MAP_W + x];
            row[x * 4 + 0] = 0x48;   // B: dust brown, reads on dark forest
            row[x * 4 + 1] = 0x62;   // G
            row[x * 4 + 2] = 0x7a;   // R
            row[x * 4 + 3] = (BYTE)(d > 235 ? 235 : d);
        }
    }
    Gfx_UnlockTexture(g_windshieldTex, 0, 0);

    // Do not inherit the world's alpha test, disabled texture stages, fog,
    // texgen or culling. Preserve the caller's complete state on return.
    Gfx_PushState();
    Gfx_SetDepthTest(FALSE);
    Gfx_SetAlphaTest(FALSE);
    Gfx_SetFog(FALSE);
    Gfx_SetLighting(FALSE);
    Gfx_SetCullMode(GFX_CULL_NONE);
    Gfx_SetStageColorOp(0, GFX_TOP_MODULATE);
    Gfx_SetStageColorArg1(0, GFX_TA_TEXTURE);
    Gfx_SetStageColorArg2(0, GFX_TA_DIFFUSE);
    Gfx_SetStageAlphaOp(0, GFX_TOP_SELECTARG1);
    Gfx_SetStageAlphaArg1(0, GFX_TA_TEXTURE);
    Gfx_SetStageColorOp(1, GFX_TOP_DISABLE);
    Gfx_SetStageAlphaOp(1, GFX_TOP_DISABLE);
    Gfx_SetStageTexCoordIndex(0, 0);
    Gfx_SetStageTextureTransform(0, GFX_TTFF_DISABLE);
    Gfx_SetStageAddress(0, GFX_ADDRESS_CLAMP);
    Gfx_SetTexture(0, g_windshieldTex);
    Gfx_SetAlphaBlend(TRUE);
    Gfx_SetSrcBlend(GFX_BLEND_SRCALPHA);
    Gfx_SetDestBlend(GFX_BLEND_INVSRCALPHA);
    Gfx_SetDepthWrite(FALSE);
    GfxTLVertex v[4];
    for (int i = 0; i < 4; i++) {
        v[i].sz = 0; v[i].rhw = 1.0f;
        v[i].color = 0xffffffff; v[i].specular = 0;
    }
    v[0].sx = 0;           v[0].sy = 0;           v[0].tu = 0; v[0].tv = 0;
    v[1].sx = (float)resX; v[1].sy = 0;           v[1].tu = 1; v[1].tv = 0;
    v[2].sx = 0;           v[2].sy = (float)resY; v[2].tu = 0; v[2].tv = 1;
    v[3].sx = (float)resX; v[3].sy = (float)resY; v[3].tu = 1; v[3].tv = 1;
    Gfx_DrawPrimitiveTL(GFX_TRIANGLESTRIP, v, 4);
    Gfx_PopState();
}

// Called from the 3D pass of a cockpit view (Game_DrawSceneViewport) so the
// crack pane is depth tested against the interior. The state is the one the
// previous frame's Windshield_UpdateView left.
void Windshield_DrawCrackPane(int view)
{
    int mview = view & (WINDSHIELD_MAX_VIEWS - 1);
    if (mview < 2) {
        WindshieldGlass_DrawCrackPane(mview, g_windshieldGlassState[mview]);
        if (g_windshieldGlassState[mview] >= 0)
            WindshieldGlass_DrawSidePanes(mview, g_windshieldSideState[mview][0], g_windshieldSideState[mview][1]);
        WindshieldGlass_DrawCockpitExtension(mview);
    }
}

void Windshield_ResetAll(void)
{
    for (int i = 0; i < WINDSHIELD_MAX_VIEWS; i++)
        g_windshieldGlassState[i] = -1;
    for (int i = 0; i < WINDSHIELD_MAX_VIEWS; i++)
        Windshield_Reset(&g_windshieldMaps[i], 0x51ed269bu + (uint32_t)i * 0x9e37u);
}
