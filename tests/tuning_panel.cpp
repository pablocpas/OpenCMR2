#include "TuningPanel.h"
#include "CameraMove.h"
#include <cstdio>
#include <cstring>
static int fails;
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
int main(){
    int n; const TuningRow *r = Tuning_Rows(n);
    CHECK(n == 45);   // includes the fog clear/cull/spatial/evolve, fog lights, snow glint, dust plume, spray, exhaust smoke, splash, backfire and weather sidecar rows
    CHECK(n <= kTuningMaxRows);   // RallyData.cpp snapshot/preset arrays are sized kTuningMaxRows - outgrowing 32 crashed the drone lock toggle
    { int bi = -1; for (int i = 0; i < n; ++i) if (!std::strcmp(r[i].key, "video.blur_mode")) bi = i; CHECK(bi >= 0 && r[bi].lo == 0 && r[bi].hi == 1 && r[bi].fallback == 0); }
    { int si = -1, di = -1; for (int i = 0; i < n; ++i) { if (!std::strcmp(r[i].key, "video.drone_spring")) si = i; if (!std::strcmp(r[i].key, "video.drone_speed_drop")) di = i; }
      CHECK(si >= 0 && r[si].lo == 1 && r[si].hi == 400 && r[si].fallback == 100);   // widened rails: spring 1-400 (0 would remove the spring outright), defaults unchanged
      CHECK(di >= 0 && r[di].lo == 0 && r[di].hi == 500 && r[di].fallback == 100); } // widened rails: drop 0-500, defaults unchanged
    for (int i = 0; i < n; ++i) { CHECK(r[i].lo < r[i].hi); CHECK(r[i].step > 0); CHECK(r[i].fallback >= r[i].lo && r[i].fallback <= r[i].hi); CHECK(!std::strncmp(r[i].key, "video.", 6)); }
    CHECK(CameraMove_Effective(kCamMoveUnset, 30) == 30 && CameraMove_Effective(0, 30) == 0 && CameraMove_Effective(-5, 30) == -5);   // an explicit 0 is an override, absent is not
    CHECK(!std::strcmp(CameraMove_FamilyKey(5), "video.camera_move_chase") && !std::strcmp(CameraMove_FamilyKey(3), "video.camera_move_cockpit") && !std::strcmp(CameraMove_FamilyKey(2), "video.camera_move_car"));
    CHECK(Tuning_Step(r[0], 0, -1) == 0);          // speed fov clamps at 0
    CHECK(Tuning_Step(r[0], 498, 1) == 500 && Tuning_Step(r[0], 150, 1) == 155);       // and at 500 (101..500 is the testing range, 5 per press)
    CHECK(Tuning_Step(r[0], 12, 1) == 17);
    CHECK(Tuning_Step(r[1], 250, 1) == 250 && Tuning_Step(r[1], 0, -1) == 0 && Tuning_Step(r[1], 50, 1) == 60);  // fov dolly 0..250
    CHECK(Tuning_Step(r[1], 500, 1) == 250 && Tuning_Step(r[1], -50, -1) == 0);   // out of range snaps in
    { int ci = -1; for (int i = 0; i < n; ++i) if (!std::strcmp(r[i].key, "video.speed_fov_cockpit")) ci = i; CHECK(ci >= 0 && r[ci].lo == 0 && r[ci].hi == 100 && r[ci].fallback == 25); }
    CHECK(Tuning_Step(r[5], 1, -1) == 1);          // dof max px never below 1
    CHECK(Tuning_Select(0, -1, n) == n - 1 && Tuning_Select(n - 1, 1, n) == 0 && Tuning_Select(3, 1, n) == 4);
    // Key filtering: panel keys vanish only while open; the toggle key always; everything else untouched.
    unsigned char k[256]; for (int i = 0; i < 256; ++i) k[i] = 0x80;
    Tuning_FilterKeys(k, false, kTuningToggleDefault);
    CHECK(k[0x44] == 0); for (int i : kTuningKeys) CHECK(k[i] == 0x80);
    CHECK(k[0x1b] == 0x80 && k[0x1a] == 0x80 && k[0x39] == 0x80 && k[0xc8] == 0x80);   // gear keys, space, arrows stay
    Tuning_FilterKeys(k, true, kTuningToggleDefault);
    for (int i : kTuningKeys) CHECK(k[i] == 0);
    CHECK(k[0x1b] == 0x80 && k[0x2e] == 0x80);
    { unsigned char z[256] = {}; Tuning_FilterKeys(z, true, 0); Tuning_FilterKeys(z, true, 999); CHECK(z[0x44] == 0); }
    // No tuning key is a default driving key (arrows, space, [ ], C, numpad).
    static const int driving[] = {0xcb, 0xcd, 0xc8, 0xd0, 0x39, 0x1b, 0x1a, 0x2e, 0x4b, 0x4d};
    for (int d : driving) { for (int t : kTuningKeys) CHECK(d != t); CHECK(d != kTuningToggleDefault); }
    // Toggle key validation: default F10 is fine; driving/menu keys, digits and out-of-range values are refused.
    CHECK(Tuning_ToggleKeyAllowed(kTuningToggleDefault) && Tuning_ToggleKeyAllowed(0x43) && Tuning_ToggleKeyAllowed(0x0F));
    for (int d : driving) CHECK(!Tuning_ToggleKeyAllowed(d));
    for (int t : kTuningKeys) CHECK(!Tuning_ToggleKeyAllowed(t));
    CHECK(!Tuning_ToggleKeyAllowed(0) && !Tuning_ToggleKeyAllowed(256) && !Tuning_ToggleKeyAllowed(-3) && !Tuning_ToggleKeyAllowed(0x1c) && !Tuning_ToggleKeyAllowed(0x01));
    // Edge detection: one trigger per press, held key does not repeat, release re-arms.
    TuningEdges e; int fires = 0;
    bool seq[] = {0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1};
    for (bool d : seq) fires += e.Edge(2, d);
    CHECK(fires == 3);
    CHECK(!e.Edge(3, false));
    // Hold-to-repeat: press fires once, nothing before the delay, then one trigger per interval, release re-arms.
    { TuningRepeat r; int n = 0; double t = 1000;
      CHECK(r.Fire(2, true, t)); for (t = 1010; t < 1000 + kTuningRepeatDelayMs; t += 10) n += r.Fire(2, true, t); CHECK(n == 0);
      CHECK(r.Fire(2, true, 1000 + kTuningRepeatDelayMs)); CHECK(!r.Fire(2, true, 1000 + kTuningRepeatDelayMs + 30));
      CHECK(r.Fire(2, true, 1000 + kTuningRepeatDelayMs + 61));
      CHECK(!r.Fire(2, false, 2000)); CHECK(r.Fire(2, true, 2010)); CHECK(!r.Fire(3, false, 2010)); }
    if (!fails) std::puts("tuning_panel ok");
    return fails;
}
