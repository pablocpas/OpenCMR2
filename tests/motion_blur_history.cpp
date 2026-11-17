// Motion blur history ping-pong (render/motion_blur_history.h): the blend never writes the slot it reads, the
// written slot becomes next frame's history (the invariant the old per-frame copy maintained), and every reset
// path (toggle, resize, non-race frame) forces a re-seed so no uninitialized or stale frame is ever blended in.
#include "render/motion_blur_history.h"
#include <stdio.h>
static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); ++fails; } } while (0)
int main()
{
    MotionBlurHistory h = MotionBlurHistory_Reset();
    CHECK(h.read == 0 && h.seeded == 0);
    CHECK(MotionBlurHistory_Write(h) != h.read);
    // Seed (first frame): history exists, the index is unchanged, the write slot is still the other one.
    MotionBlurHistory_Seed(h);
    CHECK(h.seeded == 1 && h.read == 0 && MotionBlurHistory_Write(h) == 1);
    // Steady state: write becomes read, read and write never alias, over many frames.
    for (int i = 0; i < 50; ++i) {
        int w = MotionBlurHistory_Write(h);
        CHECK(w != h.read);
        MotionBlurHistory_Commit(h);
        CHECK(h.read == w);            // next frame's history is this frame's blend output
        CHECK(h.seeded == 1);
    }
    CHECK(h.read == 0);                // even number of commits from slot 0, back where it started
    // A reset (toggle / resize / non-race frame) forces a re-seed before any blend reads history.
    h = MotionBlurHistory_Reset();
    CHECK(h.seeded == 0 && h.read == 0);
    if (!fails) puts("motion_blur_history ok");
    return fails;
}
