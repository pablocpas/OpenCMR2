// Behavioural gate: broken lamp glass must extinguish that end's light
// channels. Break slots 4/5 hold the rear lamp pair and 6/7 the front pair
// (damage grid column 0 is the front axle, see frontBrakeScale; slots 0/1
// are the windscreen/rear window, 2/3 the side windows), so the headlight
// channels must follow slots 6/7 while brake/reverse keep following 4/5.
// Drives the real CarLight_UpdateFlagsAndGlows with zero light points so
// only the channel-mask decisions run.
#include "port/types.h"
#include "Car.h"
#include "CarParts.h"
#include "CarLightState.h"
#include "CarInfo.h"
#include <cstdio>
#include <cstring>

extern void CarLight_UpdateFlagsAndGlows(Car *pCar);
extern CarPartSet *g_carPartSetLookup[8];
extern CarPartSet *g_carPartSets;
extern CarLightProfile *g_carLightProfiles[8];
extern CarLightTextureState g_carLightTextureStates[8];
extern int g_stageObjectTimeStep;
extern void StageObject_AnimateCarLightLevels(int car);

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d %s\n", __LINE__, #x); return 1; } } while (0)

static Car car;
static CarPartSet parts;
static CarLightProfile profile;

int main() {
    car.index = 0;
    g_carPartSetLookup[0] = &parts;
    g_carPartSets = &parts;
    profile.count = 0;
    g_carLightProfiles[0] = &profile;
    g_stageObjectTimeStep = 0x10000;

    // Rear pair broken, front intact, headlights on: the front channels
    // must stay lit (this is the reported bug - they died with the rear).
    car.field_0xb58 = 1;
    parts.partBroken[4] = parts.partBroken[5] = 1;
    CarLight_UpdateFlagsAndGlows(&car);
    CHECK(g_carLightTextureStates[0].channelMasks[0] & 1);
    CHECK(g_carLightTextureStates[0].channelMasks[1] & 1);
    CHECK(g_carLightTextureStates[0].channelMasks[0] & 0x10);
    CHECK(g_carLightTextureStates[0].channelMasks[1] & 0x10);

    // Slot 6 broken: its side's headlight channel dies, the intact side
    // stays, and the shared channel keeps shining for the intact lamp.
    memset(&parts, 0, sizeof parts);
    parts.partBroken[6] = 1;
    CarLight_UpdateFlagsAndGlows(&car);
    CHECK(!(g_carLightTextureStates[0].channelMasks[0] & 1));
    CHECK(g_carLightTextureStates[0].channelMasks[1] & 1);
    CHECK(g_carLightTextureStates[0].channelMasks[0] & 0x10);
    CHECK(g_carLightTextureStates[0].channelMasks[1] & 0x10);

    // Slot 7 broken: the other side's channel dies.
    memset(&parts, 0, sizeof parts);
    parts.partBroken[7] = 1;
    CarLight_UpdateFlagsAndGlows(&car);
    CHECK(g_carLightTextureStates[0].channelMasks[0] & 1);
    CHECK(!(g_carLightTextureStates[0].channelMasks[1] & 1));

    // Both front lamps broken: the headlight and shared channels die.
    parts.partBroken[6] = 1;
    CarLight_UpdateFlagsAndGlows(&car);
    CHECK(!(g_carLightTextureStates[0].channelMasks[0] & 1));
    CHECK(!(g_carLightTextureStates[0].channelMasks[1] & 1));
    CHECK(!(g_carLightTextureStates[0].channelMasks[0] & 0x10));
    CHECK(!(g_carLightTextureStates[0].channelMasks[1] & 0x10));

    // Brake gating still follows the rear pair.
    memset(&parts, 0, sizeof parts);
    car.field_0xb58 = 0;
    car.braking = 1;
    parts.partBroken[4] = 1;
    CarLight_UpdateFlagsAndGlows(&car);
    CHECK(!(g_carLightTextureStates[0].channelMasks[0] & 2));
    CHECK(g_carLightTextureStates[0].channelMasks[1] & 2);

    // Reverse gating still follows the rear pair.
    memset(&parts, 0, sizeof parts);
    car.braking = 0;
    car.reversing = 1;
    parts.partBroken[5] = 1;
    CarLight_UpdateFlagsAndGlows(&car);
    CHECK(g_carLightTextureStates[0].channelMasks[0] & 8);
    CHECK(!(g_carLightTextureStates[0].channelMasks[1] & 8));

    // Tail-light bleed: with the combine flag on and headlights lit, the
    // rear channel picks up two-thirds of the headlight level. Broken rear
    // lamps must suppress that bleed; intact ones must keep it. The headlight
    // channel must not carry the suppression implicitly (it did before, via
    // the wrong slot pair).
    int i;
    memset(&parts, 0, sizeof parts);
    memset(g_carLightTextureStates, 0, sizeof g_carLightTextureStates);
    car.braking = 0;
    car.reversing = 0;
    car.field_0xb58 = 1;
    parts.partBroken[4] = parts.partBroken[5] = 1;
    CarLight_UpdateFlagsAndGlows(&car);
    g_carLightTextureStates[0].combineFirstTwoChannels = 1;
    for (i = 0; i < 40; i++) StageObject_AnimateCarLightLevels(0);
    CHECK(g_carLightTextureStates[0].levels[0][0] > 0);
    CHECK(g_carLightTextureStates[0].levels[0][1] == 0);
    CHECK(g_carLightTextureStates[0].levels[1][1] == 0);

    // One rear lamp broken: the bleed dies on its side only.
    memset(&parts, 0, sizeof parts);
    memset(g_carLightTextureStates, 0, sizeof g_carLightTextureStates);
    parts.partBroken[4] = 1;
    CarLight_UpdateFlagsAndGlows(&car);
    g_carLightTextureStates[0].combineFirstTwoChannels = 1;
    for (i = 0; i < 40; i++) StageObject_AnimateCarLightLevels(0);
    CHECK(g_carLightTextureStates[0].levels[0][1] == 0);
    CHECK(g_carLightTextureStates[0].levels[1][1] > 0);

    // Rear pair intact: the original tail glow with headlights survives.
    memset(&parts, 0, sizeof parts);
    memset(g_carLightTextureStates, 0, sizeof g_carLightTextureStates);
    CarLight_UpdateFlagsAndGlows(&car);
    g_carLightTextureStates[0].combineFirstTwoChannels = 1;
    for (i = 0; i < 40; i++) StageObject_AnimateCarLightLevels(0);
    CHECK(g_carLightTextureStates[0].levels[0][1] > 0);
    CHECK(g_carLightTextureStates[0].levels[1][1] > 0);

    puts("broken lamp glass gates the light channels of its own end");
    return 0;
}
