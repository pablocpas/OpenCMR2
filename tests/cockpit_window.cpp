// Cockpit window no-depth-write decision (game/CockpitWindow.h): the exterior
// window node (flag 0x14) skips depth writes only in cockpit views, only for
// car-owned nodes, only while video.cockpit_glass_fog is on.
#include "CockpitWindow.h"
#include <cstdio>

static int fails = 0;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)

int main(void) {
    // Engages: car window node + cockpit camera (view type 3) + option on.
    CHECK(CockpitWindow_SkipDepthWrite(0x14, true, 3, 1));
    // Chase (1), bumper/other cameras: unchanged.
    CHECK(!CockpitWindow_SkipDepthWrite(0x14, true, 1, 1));
    CHECK(!CockpitWindow_SkipDepthWrite(0x14, true, 5, 1));
    CHECK(!CockpitWindow_SkipDepthWrite(0x14, true, 0, 1));
    // Same node flag on a stage/scenery mesh (not car-owned): unchanged.
    CHECK(!CockpitWindow_SkipDepthWrite(0x14, false, 3, 1));
    // Other node flags in cockpit view: unchanged.
    CHECK(!CockpitWindow_SkipDepthWrite(0x0e, true, 3, 1));
    CHECK(!CockpitWindow_SkipDepthWrite(0x05, true, 3, 1));
    CHECK(!CockpitWindow_SkipDepthWrite(0x00, true, 3, 1));
    // Original Subaru interior root has the SAME type 0x14 as exterior
    // glass but owns opaque door/dash/bodywork: preserve its depth.
    CHECK(!CockpitWindow_SkipDepthWrite(0x14, true, 3, 1, true));
    struct Node { Node *pParent; } body{nullptr}, interior{&body}, child{&interior}, exteriorGlass{&body};
    CHECK(CockpitWindow_InInterior(&interior,&interior));
    CHECK(CockpitWindow_InInterior(&child,&interior));
    CHECK(!CockpitWindow_InInterior(&exteriorGlass,&interior));
    CHECK(!CockpitWindow_InInterior(&child,(Node*)nullptr));
    // Option off: unchanged.
    CHECK(!CockpitWindow_SkipDepthWrite(0x14, true, 3, 0));
    if (fails) { std::printf("%d failures\n", fails); return 1; }
    std::printf("cockpit window decision ok\n");
    return 0;
}
