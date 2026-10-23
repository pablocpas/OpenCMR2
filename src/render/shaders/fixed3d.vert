#version 450
#extension GL_GOOGLE_include_directive : require
#include "common.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inDiffuse;    // B, G, R, A bytes
layout(location = 3) in vec4 inSpecular;
layout(location = 4) in vec2 inTex0;
layout(location = 5) in vec2 inTex1;

layout(location = 0) out vec4 outDiffuse;
layout(location = 1) out vec4 outSpecular;
layout(location = 2) out vec4 outTex0;
layout(location = 3) out vec4 outTex1;
layout(location = 4) out float outFog;
layout(location = 5) out float outEyeDepth;
layout(location = 6) out vec3 outEyePosition;  // for the distance fade's range

vec4 MaterialColor(int source, vec4 material, vec4 color1, vec4 color2)
{
    if (lightFlags.y != 0) {
        if (source == 1)
            return color1;
        if (source == 2)
            return color2;
    }
    return material;
}

vec4 TexCoord(int stage, vec3 eyePosition, vec3 eyeNormal)
{
    int index = texCoordIndex[stage];
    int transform = texCoordIndex[stage + 2];
    vec4 tc;
    if ((index & 0xffff0000) == 0x30000) {
        // Camera-space reflection vector.
        vec3 e = normalize(-eyePosition);
        tc = vec4(2.0 * dot(e, eyeNormal) * eyeNormal - e, 1.0);
    } else {
        vec2 uv = (index & 0xffff) == 1 ? inTex1 : inTex0;
        tc = vec4(uv, 1.0, 0.0);
    }
    if (transform != 0) {
        vec4 t = textureMatrix[stage] * vec4(tc.xyz, 1.0);
        tc = vec4(t.xyz, 1.0);
    }
    return tc;
}

void main()
{
    vec4 color1 = inDiffuse.zyxw;
    vec4 color2 = inSpecular.zyxw;
    vec4 eye = worldView * vec4(inPosition, 1.0);
    vec3 n = mat3(worldView) * inNormal;
    if (fogFlags.y != 0 && dot(n, n) > 0.0)
        n = normalize(n);

    vec4 diffuse = color1;
    vec4 specular = color2;
    if (lightFlags.x != 0) {
        vec4 matDiffuse = MaterialColor(sources.x, materialDiffuse, color1, color2);
        vec4 matSpecular = MaterialColor(sources.y, materialSpecular, color1, color2);
        vec4 matAmbient = MaterialColor(sources.z, materialAmbient, color1, color2);
        vec4 matEmissive = MaterialColor(sources.w, materialEmissive, color1, color2);
        vec3 ambient = globalAmbient.rgb;
        vec3 diffuseSum = vec3(0.0);
        vec3 specularSum = vec3(0.0);
        vec3 toEye = lightFlags.w != 0 ? normalize(-eye.xyz) : vec3(0.0, 0.0, -1.0);
        for (int i = 0; i < 8; i++) {
            Light l = lights[i];
            if (l.spot.z == 0.0)
                continue;
            int type = int(l.position.w);
            vec3 L;
            float atten = 1.0;
            float spotFactor = 1.0;
            if (type == 3) {
                L = normalize(-l.direction.xyz);
            } else {
                vec3 toLight = l.position.xyz - eye.xyz;
                float d = length(toLight);
                if (d > l.direction.w)
                    continue;
                L = d > 0.0 ? toLight / d : vec3(0.0, 0.0, 1.0);
                float denom = l.attenuation.x + l.attenuation.y * d + l.attenuation.z * d * d;
                atten = denom > 0.0 ? 1.0 / denom : 1.0;
                if (type == 2) {
                    float rho = dot(-L, normalize(l.direction.xyz));
                    if (rho <= l.spot.y)
                        spotFactor = 0.0;
                    else if (rho < l.spot.x)
                        spotFactor = pow((rho - l.spot.y) / max(l.spot.x - l.spot.y, 1e-6), l.attenuation.w);
                }
            }
            float scale = atten * spotFactor;
            ambient += l.ambient.rgb * scale;
            float nDotL = max(dot(n, L), 0.0);
            diffuseSum += l.diffuse.rgb * nDotL * scale;
            if (lightFlags.z != 0 && nDotL > 0.0) {
                vec3 h = normalize(toEye + L);
                specularSum += l.specular.rgb * pow(max(dot(n, h), 0.0), globalAmbient.w) * scale;
            }
        }
        diffuse = vec4(clamp(matEmissive.rgb + ambient * matAmbient.rgb + diffuseSum * matDiffuse.rgb, 0.0, 1.0),
                       matDiffuse.a);
        specular = vec4(clamp(specularSum * matSpecular.rgb, 0.0, 1.0), color2.a);
    }

    outDiffuse = diffuse;
    outSpecular = specular;
    outTex0 = TexCoord(0, eye.xyz, n);
    outTex1 = TexCoord(1, eye.xyz, n);
    outEyeDepth = eye.z;
    outEyePosition = eye.xyz;
    outFog = fogFlags.z != 0 ? color2.a : 1.0;
    if (fogFlags.x == 3) {
        float range = fogParams.y - fogParams.x;
        outFog = range != 0.0 ? clamp((fogParams.y - eye.z) / range, 0.0, 1.0) : 1.0;
    }

    vec4 clip = projection * eye;
    clip.xy += target.zw * clip.w;
    gl_Position = clip;
}
