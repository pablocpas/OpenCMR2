#ifndef _RALLY_DATA_H
#define _RALLY_DATA_H

#include "CarImpact.h"
#include "CarSetup.h"

// Arcade knockout table: state bits 0-2 mode, 3-5 round (1 = first round ..
// 4 = final), 6-8, 12-15 current match, 16-19. Each match holds the two
// driver indices (bits 0-4 and 5-9), the winner (bit 11 second, bit 12 first)
// and the two times.
struct KnockoutMatch {
    union {
        unsigned int flags;
        struct {
            unsigned int first : 5;
            unsigned int second : 5;
            unsigned int played : 1;
            unsigned int winner : 2;
            unsigned int reserved : 19;
        } bits;
    };
    unsigned int time1;
    unsigned int time2;
};

struct KnockoutTable {
    union {
        unsigned int state;     // 0x52f2b4
        struct {
            unsigned int mode : 3;
            unsigned int round : 3;
            unsigned int unknown6 : 3;
            unsigned int active : 1;
            unsigned int progress : 6;
            unsigned int reserved : 16;
        } bits;
    };
    KnockoutMatch final;        // 0x52f2b8
    KnockoutMatch semis[2];     // 0x52f2c4
    KnockoutMatch quarters[4];  // 0x52f2dc
    KnockoutMatch round1[8];    // 0x52f30c
    int settingPairs[9][3];     // 0x52f36c value of both settings of an arcade driver,
                                //          per arcade * 3 + round and difficulty
};

struct Car;
void RallyData_DrawCarStageGapPanel(int car, short *pRect);
void RallyData_DrawStageResultRows(int car, short *position);
void RallyData_UpdateCarRoute(Car *pCar);

extern KnockoutTable g_knockout;
void RallyData_InitKnockoutBracket(void);

unsigned char RallyDataCountryIndex(void);
unsigned char RallyDataStageIndex(void);
unsigned int RallyData_GetSelectionBits10To11(void);
unsigned int RallyData_GetSelectionBits12To13(void);
unsigned int RallyData_GetSelectionBits16To19(void);
BYTE RallyData_GetSecondarySelectionNibble(void);
unsigned char RallyDataState(void);
void RallyData_UpdateFlags(void);
void RallyData_ResetSelection(void);
void RallyData_SetSelectionBits10To11(BYTE param1);
void RallyData_SetSelectionBits16To19(BYTE param1);
void RallyData_SetSecondarySelectionNibble(BYTE param1);
void RallyData_SetKnockoutModeBits(BYTE param1);
void RallyData_SetKnockoutBits6To8(BYTE param1);
void RallyData_SetKnockoutCountryNibble(BYTE param1);

int RallyData_IsTwoUnassignedDriverMatch(void);
unsigned int *RallyData_GetChampionshipState(void);
void RallyData_GetRoundDrivers(unsigned int *pFirst, unsigned int *pSecond);
unsigned char RallyData_GetFlag24(void);
unsigned char RallyData_GetFlag25(void);
void RallyData_ValidateIndex(INT_PTR index);
void *RallyData_GetRecord(BYTE index);
char RallyData_GetUsableRecordCategory(BYTE param1);
BYTE RallyData_GetDriverRecordSelectionValue(BYTE index);
int RallyData_IsHeadToHeadRaceMode(void);

struct RallyPairingTables {
    int pairs[20][2];
    int skills[20];
    int flags[20];
    int otherFlags[20];
};
// A driver's car record of the group table (the save image keeps the same
// 0x148 bytes as SavedGameCarRecord). Kept as its own type: without it
// OptionMenu_DrawResultsRallyInfo (GameInfo.cpp) swaps the operands of an add.
struct DriverCarRecord {
    RallyCarDamageState damage;
};
// The four driver group records and the tables after them form one block:
// the network setup packet copies them as offsets from the start of the
// records (FrontendNetwork_WriteSetupPacket/ReadSetupPacket).
struct RallyDriverGroupTable {
    DriverCarRecord groups[4];      // 0x000 each driver's car damage between stages
    CarSetup playerCarSetups[4];    // 0x520 each player's car setup
    int field_0x53c;                // 0x53c
    RallyPairingTables pairing;     // 0x540
    int field_0x6d0;                // 0x6d0
    BYTE field_0x6d4[0x14];         // 0x6d4
};

#endif
