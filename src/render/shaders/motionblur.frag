#version 450

layout(location = 0) in vec2 inTex;
layout(location = 0) out vec4 outColor;

layout(set = 2, binding = 0) uniform sampler2D current;   // back buffer: this frame
layout(set = 2, binding = 1) uniform sampler2D previous;  // previous frame

layout(std140, set = 3, binding = 0) uniform MotionBlurUniforms {
    float blurMix;        // 0..1: how much of the previous frame to blend in
    float carKeep;        // 1: keep the followed car sharp - scale the mix to 0 inside carRect
    float carFeather;     // soft edge of the car rectangle, in uv units
    float pad;
    vec4 carRect;         // followed car's screen rectangle (uv, v downwards): x0 y0 x1 y1
};

void main()
{
    vec4 c = texture(current, inTex);
    vec4 p = texture(previous, inTex);
    float m = blurMix;
    if (carKeep > 0.5) {
        // Feathered box from the car's own projected vertices (render/car_region.h): the road
        // inside the rectangle stays sharp too, and anything in front of the car is not detected.
        float f = carFeather;
        float inU = smoothstep(carRect.x - f, carRect.x, inTex.x) * (1.0 - smoothstep(carRect.z, carRect.z + f, inTex.x));
        float inV = smoothstep(carRect.y - f, carRect.y, inTex.y) * (1.0 - smoothstep(carRect.w, carRect.w + f, inTex.y));
        m *= 1.0 - inU * inV;
    }
    outColor = mix(c, p, m);
}
