#ifndef CMR2_STAGE_TIME_PROFILE_H
#define CMR2_STAGE_TIME_PROFILE_H

// Five signed coefficients used to rank the sixteen drivers. The random
// interpolation runs from randomAdd to -randomSubtract; neither is clamped.
struct StageDriverTimeTuning {
    signed char baseValue;
    signed char randomSubtract;
    signed char randomAdd;
    signed char blend0Adjustment;
    signed char blend1Adjustment;
};

// ON-DISK .tm0/.tm1/.tm2 profile, selected by difficulty/game mode.
// Both banks contain ten split rows of sixteen signed time samples.
struct StageTimeProfile {
    StageDriverTimeTuning drivers[16]; // 0x000
    int blend0Times[10][16];          // 0x050
    int blend1Times[10][16];          // 0x2d0
};

#endif
