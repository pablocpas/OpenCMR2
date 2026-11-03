// Synthetic (non-proprietary) regression checks for the exterior glass overlay helpers.
#include <cstdio>
#include "WindshieldGlassSelect.h"
static int fails;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)
int main()
{
    // Flag values seen on a real exterior body mesh: pure front glass, rear window inner and outer copy, side glass.
    const unsigned frontInner = 0x0400, frontOuter = 0x8500, rearOuter = 0x8120, rearInner = 0x0020, sideZpos = 0x0040, sideZneg = 0x0200;
    // Group 0x100 (front windshield) is flag bit 0x400, not the raw bit 0x100.
    CHECK(WgTriInGroup(frontInner, 0x100));
    CHECK(WgTriInGroup(frontOuter, 0x100));
    CHECK(!WgTriInGroup(rearOuter, 0x100));   // the old flags & group test picked this triangle
    CHECK(!WgTriInGroup(rearInner, 0x100));
    CHECK(!WgTriInGroup(sideZpos, 0x100));
    CHECK(((rearOuter & 0x100) != 0));        // documents the old wrong behaviour
    // Other groups.
    CHECK(WgTriInGroup(rearInner, 0x20));
    CHECK(WgTriInGroup(rearOuter, 0x20));
    CHECK(WgTriInGroup(sideZpos, 0x40));
    CHECK(WgTriInGroup(sideZneg, 0x80));
    CHECK(!WgTriInGroup(sideZneg, 0x40));
    CHECK(!WgTriInGroup(0, 0x100));
    // Inner faces are the ones with flag 0x8000 clear.
    CHECK((frontInner & 0x8000) == 0 && (frontOuter & 0x8000) != 0);
    // World matrix conversion: identity, translation and a scale.
    int id[16] = { 65536,0,0,0, 0,65536,0,0, 0,0,65536,0, 0,0,0,65536 };
    float f[16];
    WgWorldFromFixed(id, f);
    for (int i = 0; i < 16; i++) CHECK(f[i] == ((i % 5 == 0) ? 1.0f : 0.0f));
    int tr[16] = { 32768,0,0,0, 0,65536,0,0, 0,0,65536,0, 131072,-65536,98304,65536 };
    WgWorldFromFixed(tr, f);
    CHECK(f[0] == 0.5f && f[12] == 2.0f && f[13] == -1.0f && f[14] == 1.5f && f[15] == 1.0f);
    if (!fails) std::printf("windshield glass select ok\n");
    return fails ? 1 : 0;
}
