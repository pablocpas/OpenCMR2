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
#include "WindshieldGlassFrame.h"
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

// F10 wipe diagnostics: last blade progress (0..1000, -1 parked), dirt cleared by the last sweep, glass aspect (x1000, 0 = no glass frame).
static int g_wsWipeT[WINDSHIELD_MAX_VIEWS] = { -1, -1, -1, -1, -1, -1, -1, -1 };
static int g_wsWipeCleared[WINDSHIELD_MAX_VIEWS];
static int g_wsGlassAspect[WINDSHIELD_MAX_VIEWS];
static float g_wsPrevT[WINDSHIELD_MAX_VIEWS] = { -1, -1, -1, -1, -1, -1, -1, -1 };

void Windshield_GetWipeDiag(int view, int *tPermille, int *cleared, int *aspectPermille)
{
    int v = view & (WINDSHIELD_MAX_VIEWS - 1);
    if (tPermille) *tPermille = g_wsWipeT[v];
    if (cleared) *cleared = g_wsWipeCleared[v];
    if (aspectPermille) *aspectPermille = g_wsGlassAspect[v];
}

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

// Blade geometry in glass-local uv (u across the glass, v down it, v > 1 is
// below the glass): a symmetric pair pivoting just under the bottom edge. The
// models sweep their own nodes about their own pivots; this tracks their
// normalised angle, not their exact pivot - documented approximation, the
// per-model pivot projection is a native-asset follow-up. The blade reach is
// computed from the glass shape (Windshield_BladeReach) so it covers the whole
// glass whatever its proportions.
static const float kPivotU[2] = { 0.34f, 0.66f };
static const float kPivotV = 1.04f;
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

// Dry auto wipe decision for the wiper sweep (CarInterior_UpdateWiperSweep).
int Windshield_DryWipeWanted(int slot, int running)
{
    return Windshield_DryWipeWantedForTotal(Windshield_TotalDirt(&g_windshieldMaps[slot & (WINDSHIELD_MAX_VIEWS - 1)]), running);
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

    g_wsWipeT[mview] = -1;
    if (Sys_GetOption("video.windscreen_wipers", 1) == 0 || slot < 0 || slot > 1)
        return;   // option off: the model's blades park; glass keeps its dirt
    WsGlassFrame frame;
    if (!WindshieldGlass_GetFrontFrame(slot, &frame) || !frame.valid) {
        g_wsGlassAspect[mview] = 0;
        return;   // no glass mesh to register to: nothing is drawn, nothing to clear
    }
    g_wsGlassAspect[mview] = (int)(frame.aspect * 1000.0f);
    const CarWiperState *w = &g_carWiperStates[slot];
    short *pMax = g_carInteriorProfilePointers[slot].firstWiperMaxAngle;
    if (w->mode == 0 || w->angle == 0 || pMax == NULL || *pMax <= 0) {
        g_wsPrevT[mview] = -1.0f;
        return;   // parked or no profile: no clearing
    }
    float t = (float)w->angle / (float)*pMax;   // both blades share this t
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    float prevT = g_wsPrevT[mview] < 0.0f ? t : g_wsPrevT[mview];
    g_wsPrevT[mview] = t;
    g_wsWipeT[mview] = (int)(t * 1000.0f);
    int reversed = g_carWiperDirectionReversed[slot] != 0;
    int cleared = 0;
    for (int b = 0; b < 2; b++) {
        // Blades park pointing up; they sweep out toward their pillar. The
        // sector spans the previous to the current angle so nothing the blade
        // crossed between two frames is skipped.
        float sign = (b == 0 ? -1.0f : 1.0f) * (reversed ? -1.0f : 1.0f);
        float a0 = sign * prevT * kBladeMaxAngle, a1 = sign * t * kBladeMaxAngle;
        float aMin = a0 < a1 ? a0 : a1, aMax = a0 < a1 ? a1 : a0;
        int dir = (w->sweepingForward ? 1 : -1) * (a1 >= 0.0f ? 1 : -1);
        if (dir == 0) dir = 1;
        float reach = Windshield_BladeReach(frame.aspect, kPivotU[b], kPivotV);
        cleared += Windshield_WipeSector(m, frame.aspect, kPivotU[b], kPivotV, reach, aMin, aMax, kBladeHalfWidth, dir);
    }
    g_wsWipeCleared[mview] = cleared;
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

// ---- glass pane draw ------------------------------------------------------

static GfxTexture *g_windshieldTex;

// Glass-registered dirt pane. The dirt map is a texture in glass-local uv and
// is drawn ONLY on the real front-glass triangles (WindshieldGlass_DrawDirtPane),
// depth tested in the 3D pass, so the dash, pillars, roof, wheel and hands
// occlude it and nothing appears where there is no glass. Called from the 3D
// pass of a cockpit view (Windshield_DrawCrackPane); the HUD stays clean.
// Alpha blend, the old tint's grime brown.
static void Windshield_DrawDirtPane(int view)
{
    WindshieldMap *m = &g_windshieldMaps[view & (WINDSHIELD_MAX_VIEWS - 1)];
    int mview = view & (WINDSHIELD_MAX_VIEWS - 1);
    // Draw-gating diagnostics (low volume, transitions only): why the dirt
    // pane is not on screen - broken glass suppresses it, no dirt means
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
    if (g_windshieldTex == NULL) {
        g_windshieldTex = Gfx_CreateTexture(WINDSHIELD_MAP_W, WINDSHIELD_MAP_H, GFX_FORMAT_BGRA8, 1, 0);
        if (g_windshieldTex == NULL) return;
    }
    GfxLockedRect lr;
    if (!Gfx_LockTexture(g_windshieldTex, 0, 0, &lr)) return;
    int alphaScale = Windshield_DirtAlphaScale(g_windshieldGlassState[mview],
                                               Sys_GetOption("video.windshield_dirt_fade_cracked", 0));
    for (int y = 0; y < WINDSHIELD_MAP_H; y++) {
        BYTE *row = (BYTE *)lr.pixels + y * lr.pitch;
        for (int x = 0; x < WINDSHIELD_MAP_W; x++) {
            int d = m->dirt[y * WINDSHIELD_MAP_W + x] * alphaScale / 255;
            row[x * 4 + 0] = 0x48;   // B: dust brown, reads on dark forest
            row[x * 4 + 1] = 0x62;   // G
            row[x * 4 + 2] = 0x7a;   // R
            row[x * 4 + 3] = (BYTE)(d > 235 ? 235 : d);
        }
    }
    Gfx_UnlockTexture(g_windshieldTex, 0, 0);
    if (WindshieldGlass_DrawDirtPane(mview, g_windshieldTex))
        g_wsDrawn[mview]++;
}

// Called from the 3D pass of a cockpit view (Game_DrawSceneViewport) so the
// crack pane is depth tested against the interior. The state is the one the
// previous frame's Windshield_UpdateView left.
void Windshield_DrawCrackPane(int view)
{
    int mview = view & (WINDSHIELD_MAX_VIEWS - 1);
    if (mview < 2) {
        WindshieldGlass_DrawCrackPane(mview, g_windshieldGlassState[mview]);
        Windshield_DrawDirtPane(mview);
        if (g_windshieldGlassState[mview] >= 0)
            WindshieldGlass_DrawSidePanes(mview, g_windshieldSideState[mview][0], g_windshieldSideState[mview][1]);
        WindshieldGlass_DrawCockpitExtension(mview);
        WindshieldGlass_DrawCockpitMesh(mview);
    }
}

void Windshield_ResetAll(void)
{
    for (int i = 0; i < WINDSHIELD_MAX_VIEWS; i++)
        g_windshieldGlassState[i] = -1;
    for (int i = 0; i < WINDSHIELD_MAX_VIEWS; i++)
        Windshield_Reset(&g_windshieldMaps[i], 0x51ed269bu + (uint32_t)i * 0x9e37u);
}
