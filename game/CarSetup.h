#ifndef CMR2_CAR_SETUP_H
#define CMR2_CAR_SETUP_H

#include <windows.h>

// One car setup: the seven columns of the setup screen, in the order of its
// labels (text 0x2b7..0x2bd: Tyres, Gearbox, Suspension, Power Ratio, Brake
// Bias, Brake Power, Steering). The five sliders hold 0..100 (50 centred; the
// screen shows value / 10). Car_Spawn applies them to the current car and
// reads every column sign extended.
struct CarSetup {
    char tyres;        // 0 tyre choice (Car_SetTyreType)
    char gearbox;      // 1 gear ratio table (Car_SelectGearSpeedTable)
    char suspension;   // 2 Car_SetSuspension
    char powerRatio;   // 3 front/rear drive split; 50 = even
    char brakeBias;    // 4 50 = even
    char brakePower;   // 5 maximum brake force
    char steering;     // 6 steering sensitivity
};

#endif
