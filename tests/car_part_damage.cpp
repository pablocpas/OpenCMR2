// Replay part-damage reset (game/CarParts.h, CarPartDamage_ClearAccumulated):
// entering a replay session must clear the appearance flags AND the derived
// damage values together. StageTiming_FlagCarPartBreaks only ever sets flags
// and re-derives them from damageValues[i+26] over the thresholds, so clearing
// flags alone re-arms end-of-race damage (a broken windshield) on the first
// replayed step. Setup/performance scales are not accumulation and must
// survive the reset.
#include "port/types.h"
#include "CarParts.h"
#include <cstdio>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main() {
    CarPartSet s = {};
    // Race-end accumulation: values past any threshold, flags set, biases,
    // dent grid and glass burst counters used.
    for (int i = 0; i < 34; i++) { s.damageValues[i] = 0x12000 + i; s.damageBiases[i] = 0x100 + i; }
    for (int i = 0; i < 8; i++) { s.partBroken[i] = 1; s.partDamaged[i] = 1; }
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) s.damageGrid[i][j] = 7;
    s.glassCooldown = 9;
    s.glassDebrisEmitted = 3;
    // Setup/performance damage is not accumulation: it must survive.
    s.damageScales[0] = 0x4ccc;
    s.engineTorqueScale = 0x5555;
    s.frontBrakeScale = 0x6666;
    s.rearBrakeScale = 0x7777;
    s.wheelDamageDrag[0] = 8;
    s.bodyDamageDrag = 9;
    s.steeringWobble = 10;
    CarPartDamage_ClearAccumulated(&s);
    for (int i = 0; i < 34; i++) CHECK(s.damageValues[i] == 0 && s.damageBiases[i] == 0);
    for (int i = 0; i < 8; i++) CHECK(s.partBroken[i] == 0 && s.partDamaged[i] == 0);
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) CHECK(s.damageGrid[i][j] == 0);
    CHECK(s.glassCooldown == 0 && s.glassDebrisEmitted == 0);
    // Flags cannot re-arm: the accrual gate damageValues[i+26] > threshold
    // (thresholds are positive) now fails for every part.
    for (int i = 0; i < 8; i++) CHECK(!(s.damageValues[i + 26] > 0));
    CHECK(s.damageScales[0] == 0x4ccc);
    CHECK(s.engineTorqueScale == 0x5555 && s.frontBrakeScale == 0x6666 && s.rearBrakeScale == 0x7777);
    CHECK(s.wheelDamageDrag[0] == 8 && s.bodyDamageDrag == 9 && s.steeringWobble == 10);
    std::puts("car part damage replay reset: pass");
    return 0;
}
