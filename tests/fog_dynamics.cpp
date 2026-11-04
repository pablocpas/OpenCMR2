#include "FogDynamics.h"
#include <cstdio>
#include <cmath>
static int fails;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
int main(){
    // Spatial pockets: bounded to +/-50% of the base amount, smooth, deterministic.
    for (int fi = 0; fi <= 100; ++fi) for (int pi = 0; pi < 8; ++pi) {
        float m = FogDynamics_SpatialMult(fi / 100.f, pi / 8.f);
        CHECK(m >= 0.5f - 1e-6f && m <= 1.5f + 1e-6f);
    }
    CHECK(FogDynamics_SpatialMult(0.3f, 0.25f) == FogDynamics_SpatialMult(0.3f, 0.25f));   // same stage, same layout
    CHECK(FogDynamics_SpatialMult(0.3f, 0.25f) != FogDynamics_SpatialMult(0.3f, 0.5f));    // different stage phase, different layout
    // Stage-wide trend: neutral at 100, +/-50% at the endpoints at the extremes, neutral mid-stage.
    CHECK(std::fabs(FogDynamics_EvolveMult(0.f, 100) - 1.f) < 1e-6f && std::fabs(FogDynamics_EvolveMult(1.f, 100) - 1.f) < 1e-6f);
    CHECK(std::fabs(FogDynamics_EvolveMult(0.f, 200) - 0.5f) < 1e-6f && std::fabs(FogDynamics_EvolveMult(1.f, 200) - 1.5f) < 1e-6f);
    CHECK(std::fabs(FogDynamics_EvolveMult(0.f, 0) - 1.5f) < 1e-6f && std::fabs(FogDynamics_EvolveMult(1.f, 0) - 0.5f) < 1e-6f);
    CHECK(std::fabs(FogDynamics_EvolveMult(0.5f, 200) - 1.f) < 1e-6f);
    CHECK(std::fabs(FogDynamics_EvolveMult(0.5f, -50) - 1.f) < 1e-6f);   // out-of-range option clamps, not crashes
    // Combined multiplier: off options are identity; clamps hold.
    CHECK(FogDynamics_Mult(0.4f, 0.1f, 0, 100) == 1.f);
    CHECK(FogDynamics_Mult(1.f, 0.99f, 1, 200) <= 2.f && FogDynamics_Mult(0.f, 0.99f, 1, 0) >= 0.05f);
    // Cull floor: the clearest reach of the curves, so clearing fog cannot pop culled geometry.
    CHECK(std::fabs(FogDynamics_MinMult(0, 100) - 1.f) < 1e-6f);
    CHECK(std::fabs(FogDynamics_MinMult(1, 100) - 0.1f) < 1e-6f);
    CHECK(std::fabs(FogDynamics_MinMult(0, 200) - 0.5f) < 1e-6f && std::fabs(FogDynamics_MinMult(0, 0) - 0.5f) < 1e-6f);
    CHECK(std::fabs(FogDynamics_MinMult(1, 200) - 0.05f) < 1e-6f);
    CHECK(FogDynamics_MinMult(1, 0) >= 0.05f);
    std::printf(fails ? "FAILED\n" : "fog dynamics ok\n");
    return fails ? 1 : 0;
}
