#version 450
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D sceneTex;
layout(set = 2, binding = 1) uniform sampler2D depthTex;
layout(std140, set = 3, binding = 0) uniform DoFUniforms {
    float focalDistance;
    float focalRange;
    float maxBlur;
    float tunnel;  // 0..1 speed-driven tunnel vision (0 = off, identical to the old pass)
    vec4 extra;    // x aspect ratio, y peripheral (tunnel) blur radius in pixels at full tunnel, z near plane, w far plane (0 = unknown)
    vec4 carRect;  // car screen rectangle (uv, v downwards): x0 y0 x1 y1, from the car's own projected vertices; zeros when unused
    vec4 carMode;  // x blur mode (0 peripheral = the old look, 1 whole scene except the car), y car depth min, z car depth max (log depth position), w feather
    vec4 tunnelFocus; // x focus u, y focus v in gl_FragCoord space (the upload pre-flips per runtime), z 1.0 = valid (0/unset = screen centre, the old look), w reserved
};
float edge01(float a, float b, float x) { float t = clamp((x - a) / (b - a), 0.0, 1.0); return t * t * (3.0 - 2.0 * t); }
void main()
{
    vec2 pixel = 1.0 / vec2(textureSize(sceneTex, 0));
    vec2 uv = gl_FragCoord.xy * pixel;
    vec4 center = texture(sceneTex, uv);
    float depth = texture(depthTex, uv).r;
    // Raw hardware depth is hyperbolic (depth = far/(far-near) * (1 - near/z)), so with near 0.1 almost
    // every pixel sits above 0.9 and a linear |depth - focus| saturated the blur everywhere. Recover the
    // view distance and measure focus on a log scale between near and far (unit free):
    // 0 = near plane, 1 = far plane.
    float focusPos = depth;
    float viewZ = 1.0e6;   // view distance of this pixel in world units, when the projection is known
    if (extra.z > 0.0 && extra.w > extra.z) {
        float z = extra.z * extra.w / max(extra.w - depth * (extra.w - extra.z), 1e-6);
        viewZ = z;
        focusPos = clamp(log(max(z, extra.z) / extra.z) / log(extra.w / extra.z), 0.0, 1.0);
    }
    float coc = smoothstep(0.0, max(focalRange, 0.001), abs(focusPos - focalDistance));
    float tunnelRadius = 0.0;
    // Tunnel vision: blur grows with distance from the screen centre, and the sharp circle
    // closes as speed rises. The car sits low in the frame (r about 0.5) and stays sharp.
    if (tunnel > 0.0) {
        vec2 focusUv = tunnelFocus.z > 0.5 ? tunnelFocus.xy : vec2(0.5);   // road tracking: clear zone follows the road ahead (cockpit); centre otherwise
        vec2 centered = (uv - focusUv) * 2.0;
        centered.x *= extra.x;
        centered.y *= 0.5; // mirrors kTunnelVerticalScale: bonnet and car (low, centred) stay sharp, the sides blur
        float edge = length(centered);
        float inner = max(mix(1.4, 0.5, tunnel), 0.1);   // mirrors kTunnelInnerMin; tunnel may exceed 1 (overdrive up to 2.0)
        float peripheral = smoothstep(inner, inner + 0.8, edge) * tunnel;
        if (carMode.x > 0.5) {
            // Whole-scene mode: the same speed-driven tunnel strength, but blurring everywhere. The car is kept sharp by a rectangle from its own
            // projected vertices intersected with its depth range, feathered. Not a per-pixel silhouette: the road around the car is also kept sharp.
            float f = carMode.w, zp = 0.012;
            float inU = edge01(carRect.x - f, carRect.x, uv.x) * (1.0 - edge01(carRect.z, carRect.z + f, uv.x));
            float inV = edge01(carRect.y - f, carRect.y, uv.y) * (1.0 - edge01(carRect.w, carRect.w + f, uv.y));
            float inZ = edge01(carMode.y - 2.0 * zp, carMode.y - zp, focusPos) * (1.0 - edge01(carMode.z + zp, carMode.z + 2.0 * zp, focusPos));
            peripheral = tunnel * (1.0 - inU * inV * inZ);
        }
        // Cockpit: carRect.x (peripheral mode only, 0 elsewhere) is a view distance in world units. Pixels nearer than that are the interior
        // (dashboard, wheel, hands, roof): their real depth keeps them out of the tunnel blur, fading in over 40 percent more distance.
        // The road and scenery beyond are blurred as before. A depth test on the pixel, not a screen-position guess.
        if (carMode.x < 0.5 && carRect.x > 0.0) peripheral *= edge01(carRect.x, carRect.x * 1.4, viewZ);
        coc = max(coc, peripheral);
        tunnelRadius = peripheral * extra.y;
    }
    // The peripheral blur has its own radius: dof_blur (about 1 px at the default) is far too small to see.
    float radius = max(coc * max(maxBlur, 0.0), tunnelRadius);
    if (radius < 0.25) { outColor = center; return; }
    // Golden-angle disk: smoother defocus than a box, with foreground rejection
    // to avoid pulling a sharp foreground silhouette into the blurred distance.
    vec3 sum = center.rgb;
    float weight = 1.0;
    // 16 taps up to tunnel strength 1 (unchanged look), growing to 32 at strength 2 so the larger overdrive radius stays dense.
    int taps = 16 + int(16.0 * clamp(tunnel - 1.0, 0.0, 1.0) + 0.5);
    for (int i = 0; i < taps; ++i) {
        float angle = float(i) * 2.39996323;
        float r = sqrt((float(i) + 0.5) / float(taps)) * radius;
        vec2 pos = uv + vec2(cos(angle), sin(angle)) * r * pixel;
        float sampleDepth = texture(depthTex, pos).r;
        float w = 1.0 - smoothstep(0.002, 0.02, depth - sampleDepth);
        sum += texture(sceneTex, pos).rgb * w;
        weight += w;
    }
    outColor = vec4(sum / weight, center.a);
}
