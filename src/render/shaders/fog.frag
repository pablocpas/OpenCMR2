#version 450
// Optional atmosphere fog (video.fog): distance fog + ground-mist gradient + sun-ward
// glow + car-light response (video.fog_lights). Pure maths reference: render/fog_math.h.
// One full-screen pass, scene + depth in.
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D scene;
layout(set = 2, binding = 1) uniform sampler2D depthTex;
layout(std140, set = 3, binding = 0) uniform FogUniforms {
    vec4 fog;   // x density per world unit, y glow strength 0..1, z near, w far (0 = projection unknown)
    vec4 sun;   // xy projected sun (uv, v downwards), z sun visible (0/1), w aspect ratio
    vec4 extra; // x near-field clear distance in world units (0 = off), y car-light response 0..2, z depth-half-range flag (1 = GL), w unused
    vec4 lights[8];    // xy projected car light (uv, v downwards), z intensity 0..1, w wedge start ramp length (0 = none)
    vec4 lightInfo[8]; // x depth-buffer value 1 world unit in front of the lamp (<= 0: no depth gate), yzw tint
    vec4 beams[8];     // xy aspect-corrected unit screen beam direction, z wedge length, w depth ratio q: <1 forward tail, >1 backward thinning
    vec4 halos[8];     // xy glow anchor (a few units forward along the beam), z unbiased forward segment-anchor ref (legacy ref otherwise), w beam-end depth ref
    vec4 camA;         // camera world xz, camera forward xz (horizontal parts), for the world-anchored banks
    vec4 camB;         // camera right xz, 1/projection x scale, bank strength 0..1 (0 = uniform fog)
};
// World-anchored fog banks (video.fog_spatial). Two octaves of value noise on the horizontal world position: broad banks about 70 units
// across with smaller breaks inside them. The multiplier runs from 0.20 to 1.80 and averages about 1 (the chosen fog amount
// keeps its overall look); mirrored by kFogBankMin in game/FogDynamics.h, which the fog draw cull keys off.
float bankHash(vec2 p) { p = fract(p * vec2(123.34, 456.21)); p += dot(p, p + 45.32); return fract(p.x * p.y); }
float bankNoise(vec2 p)
{
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(bankHash(i), bankHash(i + vec2(1.0, 0.0)), f.x), mix(bankHash(i + vec2(0.0, 1.0)), bankHash(i + vec2(1.0, 1.0)), f.x), f.y);
}
float fogBank(vec2 wp)
{
    float n = 0.65 * bankNoise(wp / 70.0) + 0.35 * bankNoise(wp / 27.0 + 17.3);
    n = smoothstep(0.30, 0.70, n);
    return 0.20 + 1.60 * n;   // kBankMin 0.20 .. 1.80
}
float viewZFromDepth(float depth)
{
    if (fog.z > 0.0 && fog.w > fog.z) {
        // extra.z: 1 = GL half-range buffer (0.5+0.5*d_game) - un-remap before
        // the hyperbolic recovery; 0 = full-range depth (SDL_GPU).
        float d = extra.z > 0.5 ? depth * 2.0 - 1.0 : depth;
        return fog.z * fog.w / max(fog.w - d * (fog.w - fog.z), 1e-6);
    }
    // Unknown projection: ramp over the top slice of the hyperbolic raw depth.
    float t = clamp((depth - 0.990) / 0.010, 0.0, 1.0);
    return 150.0 * t * t;
}
// Beam occluder-edge fade classification (pure-maths reference:
// render/fog_math.h Fog_OccluderCounts, covered by tests/fog_math.cpp). A
// depth neighbor counts as occluder evidence for a beam when its raw buffer
// depth is in front of that beam's per-pixel segment reference (monotone
// remap-safe on both executors) and it is not near-camera geometry:
//  - projection known (fog.z > 0): view-space distance classification -
//    closer than 1.0 world unit to the camera is the car's own interior
//    (cockpit shell, dashboard), which blocks beams only inside its own
//    silhouette; open road beside it keeps the beam. Raw-depth ratios cannot
//    express this: full-range and half-range buffers map the same geometry
//    to different ratios (SDL_GPU leaf at 1.4 -> raw 0.287 vs segment at 30
//    -> raw 0.971 would fall under a 30% raw cutoff though the leaf is a
//    real occluder).
//  - projection unknown (GL recovery fallback): raw band at 30% of the
//    segment reference (covers the harness ortho cases).
bool occluderCounts(float neighborRaw, float segRef)
{
    if (neighborRaw >= segRef) return false;   // behind/at the segment: never evidence
    if (fog.z > 0.0 && fog.w > fog.z)
        return viewZFromDepth(neighborRaw) >= 1.0;
    return neighborRaw >= segRef * 0.30;
}
void main()
{
    vec2 size = vec2(textureSize(scene, 0));
    vec2 uv = gl_FragCoord.xy / size;
    vec4 center = texture(scene, uv);
    float depth = texture(depthTex, uv).r;
    float viewZ = viewZFromDepth(depth);
    // World-anchored banks: rebuild the pixel's horizontal world position (view x / depth = ndc.x / p11) and vary the density there.
    float bank = 1.0;
    if (camB.w > 0.0 && fog.z > 0.0 && depth < 0.9999) {
        float ndcX = (uv.x * 2.0 - 1.0) * camB.z;
        vec2 wp = camA.xy + (camA.zw + camB.xy * ndcX) * viewZ;
        bank = mix(1.0, fogBank(wp), clamp(camB.w, 0.0, 1.0));
    }
    float factor = 1.0 - exp(-fog.x * viewZ * bank);
    // Near-field clear distance (video.fog_clear): 0 at the camera, full curve beyond.
    if (extra.x > 0.0) {
        float ct = clamp(viewZ / extra.x, 0.0, 1.0);
        factor *= ct * ct * (3.0 - 2.0 * ct);
    }
    // Ground mist: 0.55x at the top of the frame, 1.45x at the bottom.
    factor = clamp(factor * (0.55 + 0.9 * smoothstep(0.15, 0.85, uv.y)), 0.0, 1.0);
    // The sky hazes but never turns into a flat wall.
    if (depth >= 0.9999) factor = min(factor, 0.85);
    // Near-clean air still takes the beams (they carry their own floor below).
    if (factor <= 0.0 && extra.y <= 0.0) { outColor = center; return; }
    // Sun-ward glow: the fog brightens and warms toward the projected sun.
    vec2 d = uv - sun.xy;
    d.x *= sun.w;
    float g = sun.z > 0.5 ? fog.y * pow(max(1.0 - length(d) * 1.6, 0.0), 3.0) : 0.0;
    vec3 fogColor = mix(vec3(0.62, 0.66, 0.72), vec3(0.98, 0.87, 0.70), clamp(g, 0.0, 1.0));
    fogColor *= 1.0 + g * 0.6;
    // Car lights cutting through the fog (video.fog_lights): a screen-space wedge
    // from each lamp along its projected beam direction - an approximation of the
    // air the beam lights, not world-space road illumination. Brightest at the
    // lamps and decaying along the beam (Fog_LightBeam), so the stretch between
    // the lamps and the horizon lights up instead of only the far fog. Fog makes
    // the beam brighter but is not required for it (a 30% floor keeps the beam on
    // near-clean air, e.g. the near road or the cockpit windscreen region). Where
    // the scene sits in front of the light (the car's own body, the dashboard) the
    // depth gate keeps the glow off it; the raw-depth fallback path and the GL
    // renderer pass no per-light depth and keep the glow unoccluded. The soft
    // shoulder and the headroom term keep an already bright horizon from clipping.
    vec3 lightSum = vec3(0.0);
    if (extra.y > 0.0) {
        // Alpha-cutout foliage (2026-11-06 replay 2:57 screenshot): cutout
        // texels write no depth, so pixels inside/around a foliage silhouette
        // keep the far background depth and the per-segment gate passes for
        // beam segments BEHIND the foliage. Raw neighborhood depths (4, 8
        // and 12 px rings: foliage holes are a few px, the halo around a
        // silhouette a few more; the 8 px ring catches narrow leaves 5-11 px
        // from a cutout that slip between the other two) feed the per-beam
        // occluder test below; raw space is used because the GL path passes
        // no near/far (fog.z == 0), so only raw depths compare meaningfully
        // against segRef there.
        vec2 pxA = 4.0 / size, pxB = 12.0 / size, pxC = 8.0 / size;
        float dA0 = texture(depthTex, uv + vec2( pxA.x, 0.0)).r;
        float dA1 = texture(depthTex, uv + vec2(-pxA.x, 0.0)).r;
        float dA2 = texture(depthTex, uv + vec2(0.0,  pxA.y)).r;
        float dA3 = texture(depthTex, uv + vec2(0.0, -pxA.y)).r;
        float dC0 = texture(depthTex, uv + vec2( pxC.x, 0.0)).r;
        float dC1 = texture(depthTex, uv + vec2(-pxC.x, 0.0)).r;
        float dC2 = texture(depthTex, uv + vec2(0.0,  pxC.y)).r;
        float dC3 = texture(depthTex, uv + vec2(0.0, -pxC.y)).r;
        float dB0 = texture(depthTex, uv + vec2( pxB.x, 0.0)).r;
        float dB1 = texture(depthTex, uv + vec2(-pxB.x, 0.0)).r;
        float dB2 = texture(depthTex, uv + vec2(0.0,  pxB.y)).r;
        float dB3 = texture(depthTex, uv + vec2(0.0, -pxB.y)).r;
        for (int i = 0; i < 8; i++) {
            float li = lights[i].z;
            if (li <= 0.0) continue;
            vec2 dl = uv - lights[i].xy;
            dl.x *= sun.w;
            vec2 bdir = beams[i].xy;
            // Forward endpoints can be shorter than the historical backward
            // stability floor. Do not turn that floor into extra lit road.
            float blen = max(beams[i].z, beams[i].w < 1.0 ? 0.0001 : 0.05);
            float wedge = 0.0;
            float tAxial = 0.0;
            if (dot(bdir, bdir) > 1e-6) {
                float a = dot(dl, bdir);
                float l = dl.x * bdir.y - dl.y * bdir.x;
                // NOTE (2026-11-01 investigation): a perspective-correct width
                // scales with 1/zSeg(t) = (1+(q-1)t)/zLamp AND the transverse
                // fraction (sin of the beam/view angle); applying the z-scale
                // without the transverse term over-widens near-axial beams
                // (shot3 wash regressed 12.4% -> 13.7% coverage, left-half
                // 5.7k -> 16k) and narrows transverse ones. The calibrated
                // 0.015 + 0.35a linear form stays until the full form lands.
                float t0 = clamp(a / blen, 0.0, 1.0);
                float width = 0.015 + max(a, 0.0) * 0.35;
                float lat = clamp(l / width, -1.0, 1.0);
                float lateral = 1.0 - lat * lat;
                // Short ramp at the lamp avoids a hard edge; zero behind the
                // lamp - no backward shine. The wedge owns the whole beam now
                // that the glow bubble is gone (it used to own the first 0.05).
                float axial = a <= 0.0 ? 0.0 : min(a / 0.01, 1.0);
                // Lamp at/behind the near plane: the anchor is where the beam crosses the plane, not a lamp the player can see. Fade the wedge
                // in over lights[i].w so no bright spot sits next to the camera; the lit air farther along the beam is unchanged.
                if (lights[i].w > 0.0) axial *= smoothstep(0.0, lights[i].w, a);
                // Energy-conserving thinning: a beam clipped at the near
                // plane spears close past the camera and perspective spreads
                // it over a large screen area (the 2026-11-01 rear-wash /
                // rolled-view report). The beam's view depth runs from the
                // lamp depth to the clip depth along the wedge; spreading the
                // same energy over q times the area means q times dimmer:
                // intensity ~ z(a)/zLamp = 1/(1 + (q-1)*t). Beams ending in
                // front of the near plane carry q = 1 and keep the full wedge.
                // Forward beams (q < 1) converge into the view, not past
                // the camera. Bound them to the projected endpoint, with a
                // soft tail: the old infinite wedge continued beyond the
                // vanishing point and across the other lamp in bumper view.
                if (beams[i].w < 1.0)
                    axial *= 1.0 - smoothstep(0.70, 1.0, max(a, 0.0) / blen);
                tAxial = t0;
                wedge = axial * lateral * lateral / (1.0 + 6.0 * max(a, 0.0) / blen) / (1.0 + (max(beams[i].w, 1.0) - 1.0) * tAxial);
            }
            float gate = 1.0;
            float segRef = 0.0;
            if (lightInfo[i].x > 0.0) {
                // Per-pixel segment depth, not lamp depth: a wall behind the
                // lamp but in front of the beam's far segments must block
                // their light. 1/viewZ interpolates linearly in screen space
                // along the projected beam and the raw depth ref is affine in
                // 1/viewZ, so the raw segment ref is exactly linear in t:
                // lerp the lamp ref toward the beam-end ref (halos[i].w,
                // <= 0 = unknown projection: keep the constant lamp gate).
                float anchorRef = beams[i].w < 1.0 && halos[i].z > 0.0 ? halos[i].z : lightInfo[i].x;
                segRef = halos[i].w > 0.0 ? mix(anchorRef, halos[i].w, tAxial) : lightInfo[i].x;
                // The biased lamp gate excludes nearby bodywork separately;
                // it must not distort segment depths along the whole beam.
                // Forward beams only: a backward beam approaches the camera,
                // so its segment depth gets closer while the lamp's biased
                // ref stays farther - max() would apply that farther ref
                // across the whole wedge and hide the beam's light on
                // scenery between the segment and the lamp (Devin
                // r4536372133). Backward beams gate on the segment ref alone.
                gate = step(beams[i].w < 1.0 ? max(lightInfo[i].x, segRef) : segRef, depth);
            }
            // Per-beam occluder edge fade: only neighbors IN FRONT of this
            // beam segment count - a silhouette behind the segment (road at
            // 50 beside a tree at 10, segment at 5) must not darken the
            // light, while a far background peeking through a nearer
            // silhouette (sky through a tree's cutout, tree at 25, segment
            // at 30) fades fully. Relative 25% discontinuity: distant
            // skyline transitions (both depths huge, ratio small) keep the
            // beam. Light ON the occluder is never faded (its viewZ is the
            // neighborhood minimum, so viewZ - occ stays 0).
            if (lightInfo[i].x > 0.0 && gate > 0.0 && viewZ > 1.0) {
                // Occluder evidence per beam (occluderCounts): a neighbor in
                // front of THIS segment shadows its light here; neighbors
                // behind it must not (a tree at 10 must not darken a road
                // pixel lit by a beam segment at 5), and near-camera
                // geometry (the car's own interior shell beside a distant
                // beam) must not either - it blocks the beam only inside its
                // own silhouette, and open road beside it stays lit
                // (cockpit_window_render shell case).
                float occRaw = depth;
                if (occluderCounts(dA0, segRef)) occRaw = min(occRaw, dA0);
                if (occluderCounts(dA1, segRef)) occRaw = min(occRaw, dA1);
                if (occluderCounts(dA2, segRef)) occRaw = min(occRaw, dA2);
                if (occluderCounts(dA3, segRef)) occRaw = min(occRaw, dA3);
                if (occluderCounts(dC0, segRef)) occRaw = min(occRaw, dC0);
                if (occluderCounts(dC1, segRef)) occRaw = min(occRaw, dC1);
                if (occluderCounts(dC2, segRef)) occRaw = min(occRaw, dC2);
                if (occluderCounts(dC3, segRef)) occRaw = min(occRaw, dC3);
                if (occluderCounts(dB0, segRef)) occRaw = min(occRaw, dB0);
                if (occluderCounts(dB1, segRef)) occRaw = min(occRaw, dB1);
                if (occluderCounts(dB2, segRef)) occRaw = min(occRaw, dB2);
                if (occluderCounts(dB3, segRef)) occRaw = min(occRaw, dB3);
                // Fade magnitude from view depths (relative 25%): distant
                // skyline transitions keep the beam; a far background
                // peeking through a near cutout fades fully.
                if (occRaw < depth)
                    gate *= 1.0 - clamp((viewZ - viewZFromDepth(occRaw)) / (0.25 * viewZ), 0.0, 1.0);
            }
            // Beams only: the glow bubble was removed (user steering,
            // 2026-11-01 - "keep just the beams"). halos[] stays in the
            // uniform layout, pinned to the lamp CPU-side, but is unread here.
            lightSum += lightInfo[i].yzw * (li * wedge * gate);
        }
        lightSum *= extra.y * mix(0.30, 1.0, factor);
    }
    vec3 fogged = mix(center.rgb, fogColor, factor);
    lightSum = vec3(1.0) - exp(-lightSum * 2.0);   // Fog_SoftShoulder
    float head = clamp((1.0 - max(fogged.r, max(fogged.g, fogged.b))) * 1.5, 0.0, 1.0);   // Fog_LightHeadroom
    outColor = vec4(fogged + lightSum * head, center.a);
}
