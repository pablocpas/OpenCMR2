#ifndef CMR2_COUNTRY_CAR_SETUPS_H
#define CMR2_COUNTRY_CAR_SETUPS_H

#include "CarSetup.h"

// Default car setup of each country and the strings stored after it, one
// block (CMR2 0x0051682c): the menus measure `space` with Font_GetTextWidth and
// the weather name table g_weatherSymbolNames points into the names.
// Not in CarSetup.h: SavedGame.h includes that one, and this type there
// shifts a jump in FrontendMenu_DrawRallySummary.
struct CountryCarSetupBlock {
    CarSetup countrySetups[9];  // 0x00 by RallyDataCountryIndex
    BYTE pad;                   // 0x3f
    int unknown40;              // 0x40 -1, no reader found
    char space[4];              // 0x44 " "
    char weatherNames[6][8];    // 0x48 SNOW STORM RAIN DRIZZLE CLOUDY BRIGHT
    char blizzard[12];          // 0x78 BLIZZARD
};

#endif
