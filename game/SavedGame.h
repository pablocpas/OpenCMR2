#ifndef CMR2_SAVED_GAME_H
#define CMR2_SAVED_GAME_H

#include <stddef.h>
#include <windows.h>

#include "CategoryBestResults.h"
#include "CarSetup.h"

// Pointer-free records from the original .rcs format. Unknown bytes stay
// opaque; no native addresses or compiler-dependent global ordering is saved.
struct SavedGameOptions {
    unsigned mode : 7;
    unsigned difficulty : 3;
    unsigned playerCount : 4;
    unsigned multiplayer : 1;
    unsigned reserved : 17;
};

struct SavedGameDriver {
    unsigned car : 6;
    unsigned entryFlag : 1;
    unsigned seconds : 7;
    unsigned reserved : 18;
    int time;
};

// flags: the date of birth entered for the profile (bits 0..7 year, 8..11
// month, 16..20 day), random tags in bits 12..15 and 22..27, and bit 21 set on
// profiles that are never saved. The .pps file name is built from it.
struct DriverProfileIdentifier {
    char name[4];
    unsigned int flags;
    BYTE counters[4]; // awards won, times used, third counter (cleared on load)
};

// The five option values occupy bits 0..12 of profile word +0x5c.
struct DriverProfileSetup {
    unsigned value0 : 3;
    unsigned value1 : 3;
    unsigned value2 : 2;
    unsigned value3 : 2;
    unsigned value4 : 3;
    unsigned reserved : 19;
};

// One saved driver profile (CMR2 0x0052fa08, four of them). The HUD flags
// The tail is one anonymous member: a sixteenth member of this struct changes
// the operand order of an addition in OptionMenu_DrawResultsRallyInfo.
struct DriverProfile {
    BYTE header[0x10];
    DriverProfileIdentifier identifier;  // 0x010
    char name[0x30];                     // 0x01c, compared in lower case
    unsigned int awards;                 // 0x04c, bit mask (RallyData_SetCategoryAwardBit)
    BYTE reserved1[4];                   // 0x050
    unsigned int options;                // 0x054, bits 0..4 car, 5 automatic gearbox, 6, 7..14 age
    int car;                             // 0x058
    DriverProfileSetup setup;            // 0x05c
    BYTE eventResults[0x24];             // 0x060, ProfileEventResults
    CategoryBestResults bestResults;     // 0x084
    BYTE hudFlags;                       // 0x588, 4/8/0x10/0x20: HUD panels shown
    BYTE reserved2[3];
    BYTE wear[0xa0];                     // 0x58c, TyreWear (ChampionshipDriver.h)
    unsigned int medals[2];              // 0x62c, CategoryMedals
    struct {
        int chaseCamera[5];              // 0x634, ChaseCameraSettings (ChaseCamera.h)
        BYTE reserved3[8];               // 0x648
    };
};


struct SaveSlot {
    unsigned car : 6;
    unsigned seconds : 7;
    unsigned bit13 : 1;
    unsigned level : 4;
    unsigned category : 4;
    unsigned bits22 : 3;
    unsigned bit25 : 1;
    unsigned rest : 6;
    int time;
    BYTE pad[0x28];
};

// PORT: the championship save data as one object, laid out as it was in
// the original (0x52f3e0..0x531650), where five globals followed each other.
// Code reads past one into the next: the Rally Challenge results read
// driver records past the eight, from the profiles after them. As
// separate globals, alignment padding moved those reads eight bytes, and a
// sanitizer build's guards elsewhere.
// The driver records are ChampionshipDriver (ChampionshipDriver.h, which
// includes this header), so they are kept as bytes here and given their type
// where they are read.
struct ChampionshipDriver;
struct SaveBlock {
    BYTE carRecords[8 * 0xc4];  // 0x52f3e0
    BYTE dirty[8];              // 0x52fa00
    DriverProfile profiles[4];  // 0x52fa08
    BYTE unk0x00531348[8];      // 0x531348
    SaveSlot slots[16];         // 0x531350
};
static_assert(offsetof(SaveBlock, profiles) == 0x52fa08 - 0x52f3e0, "SaveBlock layout");
static_assert(offsetof(SaveBlock, slots) == 0x531350 - 0x52f3e0, "SaveBlock layout");
extern SaveBlock g_saveBlock;
#define g_saveCarRecords (*(ChampionshipDriver (*)[8])g_saveBlock.carRecords)
#define g_saveDirty (g_saveBlock.dirty)
#define g_saveProfiles (g_saveBlock.profiles)
#define g_unk0x00531348 (g_saveBlock.unk0x00531348)
#define g_saveSlots (g_saveBlock.slots)

struct SavedGameCarRecord { BYTE data[0x148]; };

struct SavedGameStage {
    unsigned country : 5;
    unsigned stage : 5;
    unsigned reserved : 22;
};

// CMR2 0x0081a858..0x0081b04c, written by 0x004f5190 and read by
// 0x004fb400. The old separate globals even overlapped the first 0x34 bytes.
struct SavedGameImage {
    BYTE header[0x10];
    char name[0x20];
    SavedGameOptions options;
    SavedGameDriver drivers[16];
    DriverProfileSetup playerSetup[4];
    DriverProfileIdentifier playerIdentifiers[4];
    SavedGameCarRecord playerCars[4];
    CarSetup playerCarSetups[4];
    int settingPairs[20][2];
    int primaryValues[20];
    BYTE selectionFlags[20];
    int values[20];
    int secondaryValues[20];
    BYTE selectionState;
    BYTE reserved1[3];
    SavedGameStage stage;
    BYTE tieBreak[16];
    BYTE reserved2[8];
};

// These disk-layout checks apply on every platform, including 64-bit builds.
typedef char SavedGameLayoutCheck1[(sizeof(SavedGameOptions) == 4) ? 1 : -1];
typedef char SavedGameLayoutCheck2[(sizeof(SavedGameDriver) == 8) ? 1 : -1];
typedef char SavedGameLayoutCheck3[(sizeof(DriverProfileIdentifier) == 12) ? 1 : -1];
typedef char SavedGameLayoutCheck4[(sizeof(DriverProfileSetup) == 4) ? 1 : -1];
typedef char SavedGameLayoutCheck5[(sizeof(DriverProfile) == 0x650) ? 1 : -1];
typedef char SavedGameLayoutCheck6[(offsetof(DriverProfile, setup) == 0x5c) ? 1 : -1];
typedef char SavedGameLayoutCheck7[(sizeof(SaveSlot) == 0x30) ? 1 : -1];
typedef char SavedGameLayoutCheck8[(sizeof(SavedGameImage) == 0x7f4) ? 1 : -1];
typedef char SavedGameLayoutCheck9[(offsetof(SavedGameImage, options) == 0x30) ? 1 : -1];
typedef char SavedGameLayoutCheck10[(offsetof(SavedGameImage, drivers) == 0x34) ? 1 : -1];
typedef char SavedGameLayoutCheck11[(offsetof(SavedGameImage, playerSetup) == 0xb4) ? 1 : -1];
typedef char SavedGameLayoutCheck12[(offsetof(SavedGameImage, playerIdentifiers) == 0xc4) ? 1 : -1];
typedef char SavedGameLayoutCheck13[(offsetof(SavedGameImage, playerCars) == 0xf4) ? 1 : -1];
typedef char SavedGameLayoutCheck14[(offsetof(SavedGameImage, playerCarSetups) == 0x614) ? 1 : -1];
typedef char SavedGameLayoutCheck15[(offsetof(SavedGameImage, settingPairs) == 0x630) ? 1 : -1];
typedef char SavedGameLayoutCheck16[(offsetof(SavedGameImage, primaryValues) == 0x6d0) ? 1 : -1];
typedef char SavedGameLayoutCheck17[(offsetof(SavedGameImage, selectionFlags) == 0x720) ? 1 : -1];
typedef char SavedGameLayoutCheck18[(offsetof(SavedGameImage, values) == 0x734) ? 1 : -1];
typedef char SavedGameLayoutCheck19[(offsetof(SavedGameImage, secondaryValues) == 0x784) ? 1 : -1];
typedef char SavedGameLayoutCheck20[(offsetof(SavedGameImage, selectionState) == 0x7d4) ? 1 : -1];
typedef char SavedGameLayoutCheck21[(offsetof(SavedGameImage, stage) == 0x7d8) ? 1 : -1];
typedef char SavedGameLayoutCheck22[(offsetof(SavedGameImage, tieBreak) == 0x7dc) ? 1 : -1];

bool SavedGames_IsValidSessionImage(const void *buffer, size_t size);
BYTE SavedGames_WriteSessionImage(int index, const char *name);
const SavedGameImage *SavedGames_GetRecord(int index);
unsigned int FrontendProfile_ApplyDriverProfileBlock(const SavedGameImage *image);

#endif
