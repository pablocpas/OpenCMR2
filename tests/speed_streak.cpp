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
    if(!fails)std::printf("speed_streak ok\n");
    return fails;
}
