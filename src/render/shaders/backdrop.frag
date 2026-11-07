#version 450

layout(set = 2, binding = 0) uniform sampler2D source;
layout(location = 0) out vec4 outColor;

// Capture the back buffer through the render path. Source and destination
// have the same dimensions; retain the exact texel, including its alpha.
void main()
{
    outColor = texelFetch(source, ivec2(gl_FragCoord.xy), 0);
}
