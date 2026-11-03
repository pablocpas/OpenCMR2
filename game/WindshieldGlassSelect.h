#ifndef WINDSHIELD_GLASS_SELECT_H
#define WINDSHIELD_GLASS_SELECT_H
// Pure helpers of the exterior-glass overlay (no engine types), unit-tested in tests/windshield_glass_select.cpp.

// Native triangle-group test (Game_SetTriangleField2CByGroup): the low 7 bits of
// the group are matched against the triangle flags, the next 7 bits against
// flags >> 9. Group 0x100 is therefore flag bit 0x400; a plain flags & group is
// not the native test.
static inline int WgTriInGroup(unsigned flags, unsigned group)
{
    unsigned low = group & 0x7f, high = (group >> 7) & 0x7f;
    return (flags & low) != 0 || (high & (flags >> 9)) != 0;
}

// Scene-node world matrix: 16 16.16 fixed-point values to 16 floats.
static inline void WgWorldFromFixed(const int *fixed16x16, float *out16)
{
    for (int i = 0; i < 16; i++) out16[i] = fixed16x16[i] / 65536.0f;
}
#endif
