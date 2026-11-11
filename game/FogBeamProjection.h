#ifndef OPENCMR2_FOG_BEAM_PROJECTION_H
#define OPENCMR2_FOG_BEAM_PROJECTION_H
// The fog pass's car lights (video.fog_lights): the ONE production source that
// projects this step's recorded lights (LightField) to the screen, keeping
// their view depth for the shader's depth gate. Called from StageObject's
// per-view draw; tests/fog_beam_render.cpp drives the same code into the real
// fog.frag through the GL executor. Render-only.
#include "FixedPoint.h"
#include "SceneNode.h"
#include "LightField.h"
#include "port/gfx.h"

void FixMatrix_ProjectWorldPointToView(int *pOut, FixVector *pPoint, SceneNode *pView);

// proj33/proj43: the game's projection _33/_43 (CGraphics::SetProjection), so
// each light's depth gate can be computed in depth-buffer space without the
// renderer's near/far vote.
inline int FogBeam_ProjectLights(GfxFogLight *fogLights, int maxLights, SceneNode *view, float resXf, float resYf,
                                 float proj33, float proj43)
{
    int fogLightTotal = 0;
    FixVector vOut;
    FixMatrix invView;
    FixMatrix_Invert(&invView, &view->world);
    const float aspect = resYf > 0.0f ? resXf / resYf : 1.0f;
    for (int li = 0; li < g_fogLightSourceCount && li < LIGHT_FIELD_MAX && fogLightTotal < maxLights; li++) {
        FogLightSource *src = &g_fogLightSources[li];
        FixVector vView;
        FixMatrix_TransformPoint(&vView, &src->pos, &invView);
        // Normalize the beam direction before extending it: the beam-end
        // projection below assumes a unit vector, and the asset data feeding
        // this lives outside the repo, so enforce the prerequisite here
        // instead of trusting it. Zero vectors and dirs whose length rounds
        // to zero (tiny vectors) fall back to no beam; FixVecLength /
        // FixVecScaleRecip use 64-bit intermediates. The classic glow path
        // keeps its own convention and is untouched.
        FixVector dirN = src->dir;
        bool hasBeam = src->dir.x != 0 || src->dir.y != 0 || src->dir.z != 0;
        if (hasBeam) {
            int dl = FixVecLength(&dirN);
            if (dl <= 0) { hasBeam = false; dirN.x = dirN.y = dirN.z = 0; }
            else FixVecScaleRecip(&dirN, &dirN, dl);
        }
        // The beam's screen wedge: project a point 30 world units along the
        // beam. Off-screen lamps stay: their wedge can still cross the frame
        // (the cockpit's own lamps sit below the windscreen line).
        FixVector beamEnd;
        beamEnd.x = src->pos.x + FixMul(dirN.x, 0x1e0000);
        beamEnd.y = src->pos.y + FixMul(dirN.y, 0x1e0000);
        beamEnd.z = src->pos.z + FixMul(dirN.z, 0x1e0000);
        FixVector endView = {0, 0, 0};
        if (hasBeam)
            FixMatrix_TransformPoint(&endView, &beamEnd, &invView);
        const bool lampIn = vView.z >= 0x10000, endIn = hasBeam && endView.z >= 0x10000;
        if (!lampIn && !endIn)
            continue;   // the whole beam sits behind the camera
        FixVector anchorView = vView;
        if (!lampIn) {
            // Bonnet/bumper cameras sit within a unit of the lamps: the lamp is
            // at/behind the near plane but its beam reaches into view. Anchor at
            // the near-plane crossing instead of dropping the light.
            if (endView.z - vView.z <= 0)
                continue;
            LightField_ClipBeamToNearPlane(&anchorView, &endView, &vView);
        }
        {   FixVector anchorWorld;
            FixMatrix_TransformPoint(&anchorWorld, &anchorView, &view->world);
            FixMatrix_ProjectWorldPointToView((int *)&vOut, &anchorWorld, view);
        }
        const float lampU = (float)(vOut.x >> 16) / resXf, lampV = (float)(vOut.y >> 16) / resYf;
        float endU = lampU, endV = lampV;
        if (hasBeam) {
            // Ends behind the camera need near-plane clipping: the raw
            // projection is the (-100,-100) sentinel, which mirrored the
            // wedge toward the upper-left corner - the red streak up the
            // left bank.
            FixVector endPt = endView;
            bool endOk = true;
            if (!endIn) {
                if (vView.z - endView.z > 0)   // lamp in front, end behind
                    LightField_ClipBeamToNearPlane(&endPt, &vView, &endView);
                else
                    endOk = false;   // no crossing (lamp anchored at the plane): zero run
            }
            if (endOk) {
                // Clamp the view-space run proportionally before projecting:
                // the fixed-point projector wraps int32 past |x/z| ~ 20
                // (px*65536 overflows), which flipped the wedge direction
                // 180 degrees between adjacent frames for beams ending near
                // the plane far off-axis (side view, car yaw +-10..15). A
                // proportional clamp keeps the direction exact; the length
                // only reads the run below the 0.6 screen cap, far inside
                // the clamp, so it is unaffected.
                // Reference is the projected ANCHOR: for a lamp behind the
                // near plane (bonnet camera) that is the near-plane crossing,
                // not the raw lamp view position.
                const double lz = anchorView.z / 65536.0;
                const double ez = endPt.z / 65536.0;
                double dxs = endPt.x / 65536.0 / ez - anchorView.x / 65536.0 / lz;
                double dys = endPt.y / 65536.0 / ez - anchorView.y / 65536.0 / lz;
                const double m = fabs(dxs) > fabs(dys) ? fabs(dxs) : fabs(dys);
                if (m > 4.0) {
                    const double k = 4.0 / m;
                    FixVector clamped;
                    clamped.x = (int)((anchorView.x / 65536.0 / lz + dxs * k) * ez * 65536.0);
                    clamped.y = (int)((anchorView.y / 65536.0 / lz + dys * k) * ez * 65536.0);
                    clamped.z = endPt.z;
                    endPt = clamped;
                }
                FixVector w;
                FixMatrix_TransformPoint(&w, &endPt, &view->world);
                FixMatrix_ProjectWorldPointToView((int *)&vOut, &w, view);
                endU = (float)(vOut.x >> 16) / resXf; endV = (float)(vOut.y >> 16) / resYf;
            }
        }
        // Drop only when the whole beam (lamp to end, with wedge margin) is off screen.
        float u0 = lampU < endU ? lampU : endU, u1 = lampU > endU ? lampU : endU;
        float v0 = lampV < endV ? lampV : endV, v1 = lampV > endV ? lampV : endV;
        if (u1 < -0.25f || v1 < -0.25f || u0 > 1.25f || v0 > 1.25f)
            continue;
        GfxFogLight *fl = &fogLights[fogLightTotal++];
        fl->x = lampU;
        fl->y = lampV;
        // Depth gate in depth-buffer space: the raw depth of a point half a
        // world unit BEHIND the anchor, from the game's own projection -
        // valid on both renderers even when the renderer's near/far vote is
        // ambiguous. The half-unit self-occlusion bias makes the car's own
        // bodywork around the lamp (within ~+-1 unit of it: the bonnet and
        // dash panels the halo would paint over) fail the gate, while the
        // fog over the open road - all farther than the lamp - still passes
        // (user: "the light bubble should appear in FRONT of the car").
        // Only an unknown projection (proj43 >= 0) carries no gate.
        {   float zGate = anchorView.z / 65536.0f + 0.5f;
            float d = (proj43 < 0.0f) ? proj33 + proj43 / zGate : 0.0f;
            fl->depthRef = d > 1.0f ? 1.0f : d; }
        // Raw depth of the beam's far end (near-plane-clamped), same
        // convention: the shader lerps depthRef -> beamEndRef along the wedge
        // for a per-segment gate (1/z is linear in screen space, and the
        // game's raw depth is proj33 + proj43/z, so the raw ref is exactly
        // linear along the wedge). 0 when the projection is unknown.
        {   float endZ = endView.z / 65536.0f;
            if (endZ < 1.0039f) endZ = 1.0039f;   // LightField_ClipBeamToNearPlane's plane
            float d = (proj43 < 0.0f && hasBeam) ? proj33 + proj43 / endZ : 0.0f;
            fl->beamEndRef = (d > 1.0f ? 1.0f : (d < 0.0f ? 0.0f : d)); }
        // Lamp at/behind the near plane (bonnet, bumper, car-mounted cameras): the anchor is the beam's near-plane crossing, which showed as two
        // bright spots beside the camera. Fade the wedge in over a third of the screen height from there.
        fl->startRamp = lampIn ? 0.0f : 0.33f;
        fl->intensity = src->intensity / 65536.0f;
        fl->r = src->r / 65536.0f;
        fl->g = src->g / 65536.0f;
        fl->b = src->b / 65536.0f;
        // Wedge direction AND length both come from the projected lamp->end
        // run (endU/endV above, already clipped at the near plane, so the
        // behind-camera sentinel can never reach here and the run is
        // continuous across near-plane crossings: the clip point slides
        // along the plane as the beam sweeps). An earlier formula derived
        // the LENGTH as transverse * (30/depth) * 0.5, which re-amplified
        // any off-axis angle past ~2 degrees to a full-length wedge pointing
        // straight up/down/sideways (the 2026-11-01 "headlights pointing all
        // over" driving-view report); the follow-up took the DIRECTION from
        // the view-space transverse component instead - but for a near-axial
        // beam that component is a tiny, arbitrarily-angled residual of the
        // camera pitch while the projected run is long (perspective
        // convergence toward the vanishing point): rolled car-mounted views
        // (2026-11-01 user screenshots, roll 90/180) got long wedges rotated
        // 90-180 degrees off the beam's true screen axis - the detached
        // vertical streaks. Foreshortening still supplies axial stability:
        // the projected run of a truly axial beam collapses onto the lamp,
        // and backward beams keep the extra fade band below ~6 degrees.
        // Forward offset lamps retain their real perspective-convergent run;
        // the shader softly terminates those beams instead of fading them out.
        // Beam depth ratio q = lamp view depth / end view depth, for every
        // beam (clipped or not). Two shader uses:
        // - Per-pixel segment gate: 1/z interpolates linearly in screen space
        //   along the projected beam, so the contributing segment's view depth
        //   at wedge fraction t is zLamp / (1 + (q-1)*t); a wall behind the
        //   lamp but in front of far segments must block their light (the
        //   2026-11-01 occluder experiment: the lamp-anchored gate painted
        //   13k pixels of a mid-depth wall with light from segments behind it).
        // - Energy ratio for near-plane-clipped beams only (q > 1): the beam
        //   spears close past the camera; perspective spreads the same beam
        //   energy over a large screen area (the capped 0.6-length wash), so
        //   the shader thins the wedge as the beam approaches: intensity
        //   z(a)/zLamp, i.e. energy-conserving spread, not a flat-opacity
        //   trim. The shader clamps q to >= 1 for this use, so unclipped
        //   beams keep their previous energy exactly.
        float beamQ = 1.0f;
        if (hasBeam) {
            const double lampZ = anchorView.z / 65536.0;
            double endZ = endView.z / 65536.0;
            const double clipZ = 1.0039;   // LightField_ClipBeamToNearPlane's plane (0x10100)
            if (endZ < clipZ) endZ = clipZ;   // clipped/camera-crossing: end at the near plane
            if (lampZ > 0.0) {
                double q = lampZ / endZ;
                beamQ = (float)(q < 0.05 ? 0.05 : (q > 16.0 ? 16.0 : q));
            }
        }
        {   float transverse = 0.0f;
            bool forward = false;
            if (hasBeam) {
                FixVector bd;
                FixMatrix_RotateVector(&bd, &dirN, &invView);
                const double bx = bd.x, by = bd.y, bz = bd.z;
                const double bl = sqrt(bx * bx + by * by + bz * bz);
                if (bl > 0.0) {
                    transverse = (float)(sqrt(bx * bx + by * by) / bl);
                    forward = bz > 0.0;
                }
            }
            float dx = (endU - lampU) * aspect, dy = endV - lampV;
            float run = sqrtf(dx * dx + dy * dy);
            float len = run > 0.6f ? 0.6f : run;   // screen-fraction cap
            // Forward-facing lamps converge toward the vanishing point even
            // when their direction is camera-axial. Their projected run is
            // real (offset lamps), not an unstable direction residual. Keep
            // it. Retain the fade for backward/camera-crossing beams, where
            // it prevents the old rear wash and direction flips. The weights meet
            // at one around bz=0, so the hemisphere boundary is continuous.
            float band = forward ? 1.0f : transverse / 0.10f;
            len *= band > 1.0f ? 1.0f : band;
            if (len > 1e-4f && run > 1e-4f) {
                fl->beamX = dx / run; fl->beamY = dy / run; fl->beamLen = len;
            } else { fl->beamX = 0.0f; fl->beamY = 0.0f; fl->beamLen = 0.0f; }
            // Match forward segment depths to the retained screen run.
            // Both raw depth and q are affine along this projected segment;
            // a capped run must not use the uncapped 30m endpoint's depth.
            if (forward && run > 1e-4f && beamQ < 1.0f) {
                float kept = fl->beamLen / run;
                beamQ = 1.0f + (beamQ - 1.0f) * kept;
                if (fl->beamEndRef > 0.0f) {
                    float anchorRef = proj33 + proj43 / (anchorView.z / 65536.0f);
                    fl->beamEndRef = anchorRef + (fl->beamEndRef - anchorRef) * kept;
                }
            }
            fl->beamFade = fl->beamLen > 1e-4f ? beamQ : 1.0f;
        }
        // Glow bubbles removed (user steering, 1 Nov 2026: "keep just the
        // beams"). The halo slots stay in the uniform layout (both render
        // paths share it) but are pinned to the lamp anchor with the lamp's
        // own depth gate; the fog shader no longer reads them.
        fl->haloX = lampU; fl->haloY = lampV; fl->haloRef = fl->depthRef;
        // Reuse the otherwise unused halo depth lane for the UNBIASED
        // forward segment anchor. Keep depthRef's bodywork bias separate.
        if (fl->beamFade < 1.0f && proj43 < 0.0f)
            fl->haloRef = proj33 + proj43 / (anchorView.z / 65536.0f);
    }
    return fogLightTotal;
}
#endif
