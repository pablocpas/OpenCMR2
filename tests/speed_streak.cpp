#include "render/speed_streak.h"
#include <cstdio>
static int fails;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
int main(){
    // The clear zone: no streak inside the inner radius at any strength.
    CHECK(SpeedStreak_Weight(0.0f,1.0f)==0.0f&&SpeedStreak_Weight(kSpeedStreakInner,1.0f)==0.0f);
    CHECK(SpeedStreak_Length(0.2f,1.0f)==0.0f);
    // Full at and beyond the ramp end; scales with strength; clamps.
    CHECK(SpeedStreak_Weight(kSpeedStreakInner+kSpeedStreakRamp,1.0f)==1.0f&&SpeedStreak_Weight(1.0f,1.0f)==1.0f);
    CHECK(SpeedStreak_Weight(1.0f,0.5f)==0.5f&&SpeedStreak_Weight(1.0f,0.0f)==0.0f&&SpeedStreak_Weight(1.0f,2.0f)==1.0f&&SpeedStreak_Weight(1.0f,-1.0f)==0.0f);
    // Monotonic outward: the streak never shrinks farther from the centre.
    float prev=0.0f;for(int i=0;i<=100;++i){float w=SpeedStreak_Weight(i*0.01f,1.0f);CHECK(w>=prev);prev=w;}
    // Longest sample line is the stated fraction.
    CHECK(SpeedStreak_Length(1.0f,1.0f)==kSpeedStreakMaxLen);
    // Strength: parked is zero; ratio clamps; option scales; the hold (overdo test) forces full speed only.
    CHECK(SpeedStreak_Strength(0.0f,0,100)==0.0f&&SpeedStreak_Strength(2.0f,0,100)==1.0f&&SpeedStreak_Strength(0.5f,0,50)==0.25f);
    CHECK(SpeedStreak_Strength(0.0f,1,100)==1.0f&&SpeedStreak_Strength(0.0f,1,0)==0.0f&&SpeedStreak_Strength(0.5f,0,0)==0.0f&&SpeedStreak_Strength(-1.0f,0,100)==0.0f);
    CHECK(SpeedStreak_Strength(1.0f,0,500)==1.0f&&SpeedStreak_Strength(1.0f,0,-5)==0.0f);
    // Car protection: the streak weight is multiplied out over the car's view-depth band
    // and ramps back across the fade, so the chase/drone car stays sharp while the
    // scenery behind it keeps smearing.
    CHECK(SpeedStreak_CarProtect(7.0f, 6.0f, 8.0f, 1.0f)==0.0f);        // inside the band: fully protected
    CHECK(SpeedStreak_CarProtect(6.0f, 6.0f, 8.0f, 1.0f)==0.0f&&SpeedStreak_CarProtect(8.0f, 6.0f, 8.0f, 1.0f)==0.0f);   // band edges protected
    CHECK(SpeedStreak_CarProtect(9.0f, 6.0f, 8.0f, 1.0f)==1.0f);          // one fade past the band: full streak
    CHECK(SpeedStreak_CarProtect(5.0f, 6.0f, 8.0f, 1.0f)==1.0f);          // one fade in front: full streak
    CHECK(SpeedStreak_CarProtect(8.5f, 6.0f, 8.0f, 1.0f)==0.5f);          // mid-fade behind
    CHECK(SpeedStreak_CarProtect(5.5f, 6.0f, 8.0f, 1.0f)==0.5f);          // mid-fade in front
    {   // monotonic away from the band on both sides
        float p=0.0f; for(int i=0;i<=20;++i){float m=SpeedStreak_CarProtect(8.0f+i*0.1f,6.0f,8.0f,2.0f);CHECK(m>=p);p=m;}
        p=0.0f; for(int i=0;i<=20;++i){float m=SpeedStreak_CarProtect(6.0f-i*0.1f,6.0f,8.0f,2.0f);CHECK(m>=p);p=m;}
    }
    // Per-tap rejection (mirrors the speedstreak.frag radial loop: 12 taps toward the
    // focus, each tap's weight killed when the TAP's own depth sits in the car band):
    // a background pixel just outside the car must not pull car colour outward.
    struct TapScene { float z[13]; float rgb[13][3]; };
    auto accum = [](const TapScene &sc, float out[3], bool protect) {
        float sum[3] = { sc.rgb[0][0]*2.f, sc.rgb[0][1]*2.f, sc.rgb[0][2]*2.f };
        float total = 2.0f;
        for (int i = 1; i <= 12; ++i) {
            float t = (float)i / 12.0f;
            float k = 1.0f - 0.6f * t;
            if (protect) k *= SpeedStreak_CarProtect(sc.z[i], 6.0f, 8.0f, 1.0f);
            for (int c = 0; c < 3; ++c) sum[c] += sc.rgb[i][c] * k;
            total += k;
        }
        for (int c = 0; c < 3; ++c) out[c] = sum[c] / total;
    };
    {   // Devin's example: pixel just outside the car, half its taps cross the car.
        TapScene sc; sc.z[0] = 100.f; sc.rgb[0][0] = sc.rgb[0][1] = sc.rgb[0][2] = 0.5f;
        for (int i = 1; i <= 12; ++i) {
            bool car = i <= 6;
            sc.z[i] = car ? 7.0f : 100.f;
            sc.rgb[i][0] = car ? 1.0f : 0.5f; sc.rgb[i][1] = 0.5f; sc.rgb[i][2] = 0.5f;
        }
        float off[3], on[3];
        accum(sc, off, false); accum(sc, on, true);
        CHECK(off[0] > 0.6f);                                   // unprotected: car red leaks in
        CHECK(on[0] < 0.52f && on[1] > 0.49f && on[2] > 0.49f); // protected: stays background grey
    }
    {   // Every tap inside the band: weights die, the divide still rests on the base
        // weight (2.0), so the pixel keeps its own colour - no blowup, no NaN.
        TapScene sc; sc.z[0] = 7.0f; sc.rgb[0][0] = 1.0f; sc.rgb[0][1] = 0.0f; sc.rgb[0][2] = 0.0f;
        for (int i = 1; i <= 12; ++i) { sc.z[i] = 7.0f; sc.rgb[i][0] = 1.0f; sc.rgb[i][1] = 0.0f; sc.rgb[i][2] = 0.0f; }
        float on[3];
        accum(sc, on, true);
        CHECK(on[0] == 1.0f && on[1] == 0.0f && on[2] == 0.0f);
    }
    if(!fails)std::printf("speed_streak ok\n");
    return fails;
}
