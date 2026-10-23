// Shared by the fixed-function emulation shaders. Matrices are Direct3D's
// row-vector matrices uploaded as they are, so in GLSL they act transposed:
// v * M (D3D) == M_glsl * v.

struct Light {
    vec4 diffuse;
    vec4 specular;
    vec4 ambient;
    vec4 position;      // eye space; w: type (1 point, 2 spot, 3 directional)
    vec4 direction;     // eye space; w: range
    vec4 attenuation;   // a0, a1, a2, falloff
    vec4 spot;          // cos(theta / 2), cos(phi / 2), enabled, -
};

layout(std140, set = 1, binding = 0) uniform VertexUniforms {
    mat4 worldView;
    mat4 projection;
    mat4 textureMatrix[2];
    vec4 materialDiffuse;
    vec4 materialAmbient;
    vec4 materialSpecular;
    vec4 materialEmissive;
    vec4 globalAmbient;     // w: material power
    Light lights[8];
    ivec4 lightFlags;       // lighting, colour vertex, specular, local viewer
    ivec4 sources;          // diffuse, specular, ambient, emissive (0 material, 1 colour 1, 2 colour 2)
    ivec4 fogFlags;         // vertex fog mode (3 linear) when fog is on, normalise normals, fog from specular alpha, -
    vec4 fogParams;         // start, end, -, -
    ivec4 texCoordIndex;    // stage 0, stage 1, texture transform 0, texture transform 1
    vec4 target;            // render target width, height; viewport half-pixel offset x, y (NDC)
};
