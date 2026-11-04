#ifndef OPENCMR2_FOG_CAMERA_H
#define OPENCMR2_FOG_CAMERA_H
// Camera data for the fog pass's world-anchored banks (video.fog_spatial): the shader rebuilds each pixel's horizontal world position
// from its depth and these values, so density banks stay put in the world as the car moves and the camera turns instead of sitting
// on the screen. Visual only. strength 0 = off (uniform fog, the old look). Set per view per frame by the game (Gfx_SetFogCamera),
// read by FogUniforms_Pack for both executors.
struct FogCamera {
    float posX = 0, posZ = 0;      // camera world position, horizontal plane (y is up)
    float fwdX = 0, fwdZ = 0;      // camera forward axis, horizontal part (shorter than 1 when pitched: view depth times this is the horizontal run)
    float rightX = 0, rightZ = 0;  // camera right axis, horizontal part
    float invP11 = 0;              // 1 / projection x scale: view x / view depth = ndcX * invP11
    float strength = 0;            // 0..1 bank contrast
};
inline FogCamera g_fogCamera;
#endif
