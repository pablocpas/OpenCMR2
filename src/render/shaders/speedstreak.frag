#version 450
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D scene;
layout(set = 2, binding = 1) uniform sampler2D depthTex;   // scene depth, read only to hide vapor trails behind nearer geometry
layout(std140, set = 3, binding = 0) uniform SpeedStreakUniforms {
    vec4 settings; // strength 0..1, aspect (width/height), time in seconds, speed-lines amount 0..1
    vec4 trailBox[4];  // vapor trails: per trail the screen box (uv x0 y0 x1 y1, v downwards); all zero = no trails
    vec4 trailPts[28]; // 7 points per trail: uv x, uv y, half width (screen heights, 0 = point unusable), opacity
    vec4 trailZ[28];   // x = view depth of that point (world units)
    vec4 depthInfo;    // x near plane, y far plane (0 = no depth: trails off), z depth tolerance (world units, the car's own surface), w fade width (world units)
    vec4 cockpitDepth; // x clear distance (0 disabled), y near, z far, w reserved
    vec4 tunnelLines;  // x tunnel vision strength 0..2 (eased, same as dof.frag), y 1 = radial lines inside the tunnel vision zone
    vec4 tunnelFocus;  // x focus u, y focus v in gl_FragCoord space (the upload pre-flips per runtime), z 1.0 = valid (0/unset = screen centre, the old look), w reserved
};
// Mirrors src/render/speed_streak.h. Directional radial streak: samples run from the pixel toward the screen centre, so the
// scenery smears outward. The clear inner zone is a radial exclusion, not a car mask.
const float INNER = 0.30;
const float RAMP = 0.30;
const float MAXLEN = 0.10;
const int TAPS = 12;
// Speed lines (mirrors src/render/speed_lines.h): thin rays that converge on the screen centre, drawn over the frame. Visual only.
const float LINES_RAYS = 80.0;
const float LINES_INNER = 0.34;      // clear zone radius (screen heights, aspect corrected), a little outside the streak's inner zone
const float LINES_FADE = 0.22;       // width of the fade-in from clear to full
const float LINES_ONSET = 0.25;      // streak strength (speed ratio x setting) where the lines start to appear
const float LINES_FULL = 0.80;       // streak strength where the lines are fully shown
const float LINES_OPACITY = 0.55;    // brightest a line gets
float hash1(float n) { return fract(sin(n * 127.1 + 311.7) * 43758.5453); }
float speedLines(vec2 pc, float time, float soft, float thin)   // soft 0 = the normal look; > 0 widens the ray and dash edges (vague lines)
{
    float r = length(pc);
    float a = atan(pc.y, pc.x) / 6.2831853 + 0.5;       // 0..1 around the centre
    float f = a * LINES_RAYS;
    float id = floor(f);
    float h = hash1(id);
    if (h < 0.45) return 0.0;                              // about half the rays are absent
    float local = abs(fract(f) - 0.5);                     // 0 on the ray's axis, 0.5 between rays
    float halfWidth = mix(0.03, 0.09, clamp(r * 1.2, 0.0, 1.0)) * thin; // thin near the centre, wider outward
    float line = 1.0 - smoothstep(halfWidth * (0.35 - 0.3 * soft), halfWidth * (1.0 + 1.2 * soft), local);
    float dash = fract(r * mix(2.2, 4.0, hash1(id + 9.0)) - time * (1.6 + 2.4 * h) + hash1(id + 3.0));  // segments run outward
    float seg = smoothstep(0.0, 0.18 + 0.25 * soft, dash) * (1.0 - smoothstep(0.55 - 0.2 * soft, 0.8 + 0.1 * soft, dash));
    float zone = smoothstep(LINES_INNER, LINES_INNER + LINES_FADE, r);
    return line * seg * zone * (0.45 + 0.55 * h);
}
// Vapor trails: soft lines through the projected history points of the car's rear anchors (src/render/vapor_trail.h). Visual only.
float vaporTrails(vec2 uv, float aspect, float time)
{
    float best = 0.0;
    vec2 pq = vec2(uv.x * aspect, uv.y);
    float sceneZ = 1e30;
    bool haveBox = false;
    for (int k = 0; k < 4; ++k) { vec4 bx = trailBox[k]; if (uv.x >= bx.x && uv.x <= bx.z && uv.y >= bx.y && uv.y <= bx.w) haveBox = true; }
    if (!haveBox || depthInfo.y <= depthInfo.x) return 0.0;
    float dep = textureLod(depthTex, uv, 0.0).r;   // explicit lod: this runs under a non-uniform branch
    sceneZ = depthInfo.x * depthInfo.y / max(depthInfo.y - dep * (depthInfo.y - depthInfo.x), 1e-6);   // same recovery as dof.frag
    for (int k = 0; k < 4; ++k) {
        vec4 bx = trailBox[k];
        if (uv.x < bx.x || uv.x > bx.z || uv.y < bx.y || uv.y > bx.w) continue;
        for (int i = 0; i < 6; ++i) {
            vec4 a = trailPts[k * 7 + i], b = trailPts[k * 7 + i + 1];
            if (a.z <= 0.0 || b.z <= 0.0) continue;
            vec2 pa = vec2(a.x * aspect, a.y), pb = vec2(b.x * aspect, b.y), ba = pb - pa;
            float t = clamp(dot(pq - pa, ba) / max(dot(ba, ba), 1e-9), 0.0, 1.0);
            float d = length(pq - pa - ba * t);
            float hw = mix(a.z, b.z, t), al = mix(a.w, b.w, t);
            float tz = 1.0 / mix(1.0 / max(trailZ[k * 7 + i].x, 1e-3), 1.0 / max(trailZ[k * 7 + i + 1].x, 1e-3), t);   // perspective-correct depth along the segment
            float vis = 1.0 - smoothstep(0.0, max(depthInfo.w, 1e-3), tz - (sceneZ + depthInfo.z));                    // hidden behind nearer geometry
            // Two thin strands twisting around the trail axis over a faint haze, so the trail reads as a vortex instead of one flat line. Procedural, no texture.
            // The whole thing is about 30% fainter than the single soft line it replaces.
            vec2 nrm = normalize(vec2(-ba.y, ba.x) + vec2(1e-6, 0.0));
            float sd = dot(pq - pa - ba * t, nrm);
            float sPos = float(i) + t;
            float swirl = hw * 0.55 * sin(sPos * 1.9 - time * 3.0);
            float strands = max(1.0 - smoothstep(0.0, hw * 0.4, abs(sd - swirl)), 1.0 - smoothstep(0.0, hw * 0.4, abs(sd + swirl)));
            float haze = 1.0 - smoothstep(0.0, hw, d);
            best = max(best, (0.7 * strands + 0.3 * haze) * 0.7 * al * vis);
        }
    }
    return best;
}
void main()
{
    vec2 uv = gl_FragCoord.xy / vec2(textureSize(scene, 0));
    vec4 base = texture(scene, uv);
    vec2 focusUv = tunnelFocus.z > 0.5 ? tunnelFocus.xy : vec2(0.5);   // road tracking: streak direction and tunnel-lines zone share the dof clear circle's focus
    vec2 c = uv - focusUv;
    float r = length(c * vec2(settings.y, 1.0));
    float w = smoothstep(INNER, INNER + RAMP, r) * clamp(settings.x, 0.0, 1.0);
    // The radial streak runs after tunnel blur. Preserve the same nearby cockpit
    // surfaces here instead of smearing them again in this later pass.
    float cockpitEffect = 1.0;
    if (cockpitDepth.x > 0.0 && cockpitDepth.y > 0.0 && cockpitDepth.z > cockpitDepth.y) {
        float d = texture(depthTex, uv).r;
        float z = cockpitDepth.y * cockpitDepth.z / max(cockpitDepth.z - d * (cockpitDepth.z - cockpitDepth.y), 1e-6);
        cockpitEffect = smoothstep(cockpitDepth.x, cockpitDepth.x * 1.4, z);
    }
    w *= cockpitEffect;
    float linesAmount = smoothstep(LINES_ONSET, LINES_FULL, clamp(settings.w, 0.0, 1.0));
    float lines = linesAmount > 0.0 ? speedLines(c * vec2(settings.y, 1.0), settings.z, 0.0, 1.0) * linesAmount * LINES_OPACITY : 0.0;
    // Lines inside the tunnel vision zone (video.tunnel_lines): the same rays, shown only where dof.frag blurs (same clear circle), so the centre and
    // the road ahead stay clear. They need no speed streak setting. Strength 1 is the full 100% look; above that they brighten a little.
    if (tunnelLines.y > 0.5 && tunnelLines.x > 0.0) {
        float tl = tunnelLines.x;
        vec2 tc = c * 2.0; tc.x *= settings.y; tc.y *= 0.5;                   // mirrors dof.frag
        float inner = max(mix(1.4, 0.5, tl), 0.1);
        float zoneT = smoothstep(inner, inner + 0.8, length(tc));
        // Only once the tunnel vision is really active (eased strength, so it is 0 standing still and fades in as the clear circle closes), faint and vague,
        // and only deep in the blurred area: zoneT squared keeps the lines off the edge of the clear circle.
        float amountT = smoothstep(0.6, 1.2, clamp(tl, 0.0, 1.2));
        float lt = speedLines(c * vec2(settings.y, 1.0), settings.z, 1.0, 0.55) * zoneT * zoneT * amountT * LINES_OPACITY * 0.12 * (1.0 + 0.3 * clamp(tl - 1.0, 0.0, 1.0));
        lines = max(lines, lt);
    }
    lines *= cockpitEffect;
    lines = max(lines, vaporTrails(uv, settings.y, settings.z));
    if (w <= 0.0) { outColor = vec4(mix(base.rgb, vec3(0.93, 0.95, 1.0), lines), base.a); return; }
    vec2 toCenter = -c * (w * MAXLEN);
    vec3 sum = base.rgb * 2.0;
    float total = 2.0;
    for (int i = 1; i <= TAPS; ++i) {
        float t = float(i) / float(TAPS);
        float k = 1.0 - 0.6 * t;
        sum += texture(scene, uv + toCenter * t).rgb * k;
        total += k;
    }
    outColor = vec4(mix(sum / total, vec3(0.93, 0.95, 1.0), lines), base.a);
}
