// Behavioural pins for the per-condition race grade (video.color_grading).
// The module turns live stage signals (weather kind, sun elevation, light
// warmth) into one of three looks - day / golden hour / wet - blended
// smoothly, always exact-neutral when the effect is off.
#include "GradeConditions.h"
#include <cmath>
#include <cstdio>
#include <initializer_list>
#define CHECK(x) do{if(!(x)){printf("FAIL line%d %s\n",__LINE__,#x);return 1;}}while(0)
static bool near(int a,int b,int tol){return std::abs(a-b)<=tol;}
int main(){
    // Strength 0 is the exact neutral grade for every condition: the "off"
    // path of video.color_grading must stay bit-identical to the old shader
    // identity regardless of weather, sun or light.
    for (float wet : {0.0f, 0.5f, 1.0f})
      for (float el : {-0.2f, 0.1f, 0.6f})
        for (float w : {0.0f, 0.5f}) {
            GradeConditions::Grade g = GradeConditions::Compute(wet, el, w, 0);
            CHECK(g.temp==0 && g.tint==0 && g.saturation==100 && g.contrast==100);
        }
    // Clear midday: the day look adds warmth, saturation and a firmer
    // S-curve (this is the "more dramatic daytime" ask).
    GradeConditions::Grade day = GradeConditions::Compute(0.0f, 0.6f, 0.0f, 100);
    CHECK(day.temp > 0 && day.saturation > 100 && day.contrast > 108);
    // Golden hour = low sun plus warm light: warmer and deeper than the day
    // look at the same strength.
    GradeConditions::Grade golden = GradeConditions::Compute(0.0f, 0.25f, 0.6f, 100);
    CHECK(golden.temp > day.temp && golden.contrast > day.contrast);
    // A low sun with neutral light is dusk, not golden hour: the warmth gate
    // must keep the look near the day recipe.
    GradeConditions::Grade dusk = GradeConditions::Compute(0.0f, 0.25f, 0.0f, 100);
    CHECK(near(dusk.temp, day.temp, 6));
    // Wet (rain or snow mapped to wet) overrides sun position: cooler,
    // desaturated, and the deepest S-curve of the three looks.
    GradeConditions::Grade wetG = GradeConditions::Compute(1.0f, 0.6f, 0.0f, 100);
    CHECK(wetG.temp < 0 && wetG.saturation < 100 && wetG.contrast > golden.contrast);
    // The golden blend moves smoothly across its sun-elevation band: no
    // adjacent-sample jump in any channel.
    GradeConditions::Grade prev = GradeConditions::Compute(0.0f, 0.05f, 0.6f, 100);
    for (int i = 1; i <= 40; ++i) {
        GradeConditions::Grade cur = GradeConditions::Compute(0.0f, 0.05f + 0.02f*i, 0.6f, 100);
        CHECK(std::abs(cur.temp-prev.temp)<=3 && std::abs(cur.contrast-prev.contrast)<=3);
        prev = cur;
    }
    // Half strength halves the departure from neutral (linear strength scale,
    // matching the base recipe's per-100 scaling).
    GradeConditions::Grade half = GradeConditions::Compute(0.0f, 0.6f, 0.0f, 50);
    CHECK(near(half.temp, day.temp/2, 2) && near(half.contrast-100, (day.contrast-100)/2, 2));
    // Wet smoothing: one frame at 60fps moves only a small step toward the
    // target, 2 seconds nearly completes it, and the value clamps to [0,1].
    float w = 0.0f;
    w = GradeConditions::Step(w, 1.0f, 1.0f/60.0f);
    CHECK(w > 0.0f && w < 0.1f);
    for (int i = 0; i < 120; ++i) w = GradeConditions::Step(w, 1.0f, 1.0f/60.0f);
    CHECK(w > 0.9f && w <= 1.0f);
    w = GradeConditions::Step(1.0f, 0.0f, 10.0f);
    CHECK(w == 0.0f);
    printf("grade conditions OK\n");
    return 0;
}
