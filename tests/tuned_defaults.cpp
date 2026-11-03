// The tuned defaults (src/platform/tuned_defaults.h) and the F10 panel row fallbacks must agree,
// and every listed key must be a panel row (so no stray default).
#include "platform/tuned_defaults.h"
#include "TuningPanel.h"
#include <cstdio>
#include <cstring>
int main()
{
    int fails = 0, count = 0;
    const TuningRow *rows = Tuning_Rows(count);
    for (const TunedDefault &d : kTunedDefaults) {
        bool found = false;
        for (int i = 0; i < count; ++i)
            if (!std::strcmp(rows[i].key, d.key)) { found = true; if (rows[i].fallback != d.value) { std::printf("FAIL %s row %d table %d\n", d.key, rows[i].fallback, d.value); ++fails; } }
        if (!found) { std::printf("FAIL %s not a panel row\n", d.key); ++fails; }
    }
    if (TunedDefault_Or("video.fog_lights", 100) != 40 || TunedDefault_Or("video.unlisted_key", 7) != 7) { std::puts("FAIL lookup"); ++fails; }
    if (!fails) std::puts("tuned_defaults ok");
    return fails;
}
