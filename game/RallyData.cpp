#include "CameraMove.h"
#include "AirMote.h"
#include "SnowMaterial.h"
#include "TuningMode.h"
#include "EffectAmounts.h"
#include "TuningPanel.h"
#include "FovPreview.h"
#include "render/effect_settings.h"
#include "DebugOverlayLayout.h"
#include "SceneryPack.h"
#include "SavedGame.h"
#include "port/sys.h"
#include "ChampionshipDriver.h"
#include "ProfileEventResults.h"
#include "CallbackState.h"
#include "FloatToInt.h"
#include <stddef.h>
#include <stdio.h>

void Race_DrawPlayerMessage(char *text, int *pColour, int car, int param4, int x, int y);
#include <string.h>
#include "RallyData.h"
#include "RallyRoute.h"
#include "SectorObjectEntry.h"
#include "MeshCollisionRecord.h"
#include "Car.h"
#include "CarResources.h"
#include "GameInfo.h"
#include "RallyTiming.h"
#include "main.h"
#include "Frontend.h"
#include "Menu.h"

// Championship save data, contiguous in memory:
//   0x52f3e0  8 driver records of 0xc4 bytes
//   0x52fa00  per-category "name edited" flags
//   0x52fa08  4 category records of 0x650 bytes
//   0x531350  16 index records of 0x30 bytes (category in bits 18..21 of +0)
// These were five separate globals in the original, not one blob: MSVC6 only
// reorders stores it can prove disjoint, i.e. to different symbols.
// PORT: one object in the port (SaveBlock, SavedGame.h): g_saveCarRecords
// (0x52f3e0), g_saveDirty (0x52fa00), g_saveProfiles (0x52fa08),
// g_unk0x00531348 and g_saveSlots (0x531350).
SaveBlock g_saveBlock;

// A save slot (g_saveSlots, 0x30 bytes): packed first word and a time.

#include "AIHelper.h"
#include "RegKey.h"
#include "Font.h"
#include "Sprite.h"
#include "Graphics.h"
#include "StageSplitData.h"
// Per-car "stage record" finish message flags (0x536e88): a separate int[2] in
// the original that overlaps the last time of the second split record.
#define g_finishStageRecordMessage ((int *)(g_stageSplitBlock + 0x8c))
#include "FixedPoint.h"
#include "GraphicsOptions.h"
#include <stdlib.h>
#include "StageTiming.h"
#include "StageUI.h"
#include "FileBuffer.h"
#include "Input.h"
#include "Glow.h"
#include "LayoutChecks.h"

int StageTiming_GetStartSlotIndex(int index);
int Race_GetPlayerRecordField4(BYTE index);
int Race_GetCoDriverCallState(void);
int Sound_PlaySampleWithParameters(unsigned short id, int volume, int frequency, int loopStart, int loops, int is3D);
void RallyData_ResetDriverRecordCategory(BYTE index, BYTE param2);
int StageTiming_GetCheckpointField2(int index);
int StageTiming_IsClockOverlayActive(void);
int StageTiming_GetPendingDriverEntry(int index);
int StageTiming_GetCarTimingByte81(int car);
int NetPlayers_GetResultPlayerIndex(int index);
unsigned int NetPlayers_GetBestLapTime(void);
char *NetPlayers_GetRecordHolderName(void);
unsigned int NetPlayers_GetSplitRecordTime(int split);
unsigned int RallyData_GetSetupModeBits(void);
unsigned int RallyData_GetSetupHighNibble(void);
unsigned int RallyData_GetSetupFlag11(void);
unsigned int RallyData_GetSelectionFlag27(void);
int RallyData_PickStageOutsideExcludedGroups(int exclude1, int exclude2);
int RallyData_PickUnexcludedGroupEvent(int exclude1, int exclude2);
BYTE Profile_WriteSaveFile(int unused, BYTE *pProfile);
bool FrontendProfile_DoesDriverMatchEntry(int param_1, BYTE *param_2);
struct Menu;
Menu *FrontendMenu_GetAlternateRallyStageSelection(void);
Menu *FrontendMenu_GetRallyStartTransition(void);
void Menu_SetNextAction(Menu *action);
void Menu_SetInputStateFlag(char param1);
unsigned short RallyData_GetPrimaryStageScoreScale(void);
void RallyData_RebuildDistinctValueList(void);
BYTE *RallyData_GetDistinctValueList(void);
int RallyData_GetDistinctValueCount(void);
void RallyData_SetDriverPairingStateValues(int param1, int param2);
int Stage_GetSplitPositionCount(void);
int Stage_GetSplitPositionFixed(int index);
int RallyData_GetRouteAvailabilityState(void);
void RallyData_GetRouteNodeGroundPosition(int index, int *pOut);

// GLOBAL: CMR2 0x0052f2a9
BYTE g_unk0x0052f2a9;

// GLOBAL: CMR2 0x0052f2a8
BYTE g_unk0x0052f2a8;
// Original 9 x 30 rally lookup table; the next data begins at 0x5166b4.
// GLOBAL: CMR2 0x0051627c
unsigned int g_unk0x0051627c[9][30] = {
    {
        0, 0, 0, 1, 1, 1, 1, 2, 2, 3,
        0, 0, 0, 1, 1, 1, 1, 2, 2, 3,
        0, 0, 1, 1, 1, 1, 2, 2, 2, 3
    },
    {
        0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 1, 1, 1, 1, 1
    },
    {
        0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 1, 1, 1, 1, 2, 3,
        0, 0, 0, 0, 1, 1, 1, 1, 2, 4
    },
    {
        0, 0, 1, 1, 2, 2, 6, 6, 7, 7,
        0, 1, 1, 2, 2, 2, 6, 7, 7, 8,
        1, 2, 2, 6, 7, 7, 7, 7, 8, 8
    },
    {
        0, 0, 0, 0, 0, 1, 1, 1, 1, 2,
        0, 0, 0, 0, 0, 1, 1, 1, 2, 3,
        0, 0, 0, 0, 0, 1, 1, 1, 2, 4
    },
    {
        0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 1, 1, 1, 1, 2, 4,
        0, 0, 0, 0, 0, 1, 1, 2, 2, 4
    },
    {
        0, 0, 1, 1, 1, 1, 1, 2, 2, 3,
        1, 0, 1, 1, 1, 2, 2, 2, 3, 4,
        1, 0, 1, 1, 2, 2, 2, 3, 4, 5
    },
    {
        1, 1, 1, 2, 2, 2, 3, 3, 4, 4,
        1, 2, 2, 2, 2, 3, 3, 4, 4, 5,
        2, 2, 2, 2, 3, 3, 4, 4, 4, 5
    },
    {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1
    }
};
CMR2_LAYOUT_CHECK(RallyPairingTablesSize, sizeof(RallyPairingTables) == 0x190);
CMR2_LAYOUT_CHECK(RallyDriverGroupTableSize, sizeof(RallyDriverGroupTable) == 0x6e8);
// GLOBAL: CMR2 0x0052ebc0
RallyDriverGroupTable g_driverGroupTable;
#define g_rallyPairingTables (g_driverGroupTable.pairing)

// Picks the AI skill of the four opponents of a slot: for each opponent a
// random window around its rating (pRatings[1], [3], [5], [7]) is spread over
// the nine skill classes of pClasses (one per 10 points), and the most
// frequent class wins. Slot 2 only uses two opponents.
// match 74%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0040d6c0
void RallyData_SelectOpponentSkillClasses(int slot, int *pClasses, char *pRatings)
{
    unsigned int counts[9];
    int classes[4];
    int total;
    int opponent;
    int bestClass;
    int lo;
    int hi;
    int range;
    int best;
    int i;
    int j;
    int *pDest;

    for (opponent = 0; opponent < 4; opponent++) {
        memset(counts, 0, sizeof(counts));
        lo = pRatings[opponent * 2 + 1] - rand() % 20 - 1;
        hi = rand() % 20 + pRatings[opponent * 2 + 1] + 1;
        if (lo < 0)
            lo = 0;
        else if (hi > 100)
            hi = 100;
        range = 0;
        for (j = lo; j < hi; j++) {
            range++;
            counts[pClasses[j / 10]]++;
        }
        best = -100;
        total = range << 16;
        bestClass = -1;
        for (i = 0; i < 9; i++) {
            counts[i] = FixMulShift32(0x640000, FixDiv(counts[i] << 16, total));
            if ((int)counts[i] > best) {
                best = counts[i];
                bestClass = i;
            }
        }
        classes[opponent] = bestClass;
    }
    pDest = &g_rallyPairingTables.skills[slot * 4];
    for (i = 0; i < 4; i++) {
        if (slot == 2 && i > 1)
            *pDest = 0;
        else
            *pDest = classes[i];
        pDest++;
    }
}



void RallyData_InterpolateGroupGrip(int group);

// Restarts the driver pairing tables of a rally: both values of every pair, the
// AI skill array and the entry array take the rally key, the two flag arrays
// are cleared, and the three groups of the rally are rebuilt.
// MSVC folds the three int[20] tables into one base register with +-0x50
// displacements in the original (they are contiguous in its .bss); our tables
// are not adjacent, so the compiler picks another induction variable and the
// opcodes differ.
// FUNCTION: CMR2 0x0040dbe0
void RallyData_ResetDriverPairingTables(int value)
{
    int i;

    for (i = 0; i < 20; i++) {
        g_rallyPairingTables.pairs[i][0] = value;
        g_rallyPairingTables.pairs[i][1] = value;
        g_rallyPairingTables.skills[i] = value;
        g_rallyPairingTables.flags[i] = 0;
        g_rallyPairingTables.otherFlags[i] = 0;
    }
    RallyData_InterpolateGroupGrip(0);
    RallyData_InterpolateGroupGrip(1);
    RallyData_InterpolateGroupGrip(2);
}

// GLOBAL: CMR2 0x0052f2ac
unsigned int g_selectedRallyData = 0;

// GLOBAL: CMR2 0x0052f2b0
unsigned int g_unk0x0052f2b0;

// GLOBAL: CMR2 0x0052f2b4
KnockoutTable g_knockout;

// Seeds the knockout bracket, separates human drivers where possible and
// puts the lower human driver index first in each all-human pairing.
// match 66%: bitfield access ordering and stack/register allocation still differ.
// FUNCTION: CMR2 0x004069c0
void RallyData_InitKnockoutBracket(void)
{
    int participants;
    int matches = 0;
    int i;
    int j;
    int pick;
    BYTE aiIndex = 0;
    KnockoutMatch *p;
    KnockoutMatch *q;
    unsigned int oldFirst;
    unsigned int oldSecond;

    g_knockout.bits.progress = 0;
    g_knockout.bits.active = 1;
    switch (g_knockout.bits.mode) {
    case 4: matches = 8; g_knockout.bits.round = 1; break;
    case 3: matches = 4; g_knockout.bits.round = 2; break;
    case 2: matches = 2; g_knockout.bits.round = 3; break;
    case 1: matches = 1; g_knockout.bits.round = 4; break;
    }
    participants = CGameInfo::GetConfiguredPlayerCount() & 0xff;
    p = g_knockout.round1;
    do {
        p->bits.played = 0;
        p->time1 = 0;
        p->time2 = 0;
        p->bits.winner = 0;
        p->bits.first = 31;
        p->bits.second = 31;
        p++;
    } while ((INT_PTR)p < (INT_PTR)&g_knockout.round1[8]);
    p = g_knockout.quarters;
    do {
        p->bits.played = 0;
        p->time1 = 0;
        p->time2 = 0;
        p->bits.winner = 0;
        p->bits.first = 31;
        p->bits.second = 31;
        p++;
    } while ((INT_PTR)p < (INT_PTR)&g_knockout.quarters[4]);
    p = g_knockout.semis;
    do {
        p->bits.played = 0;
        p->time1 = 0;
        p->time2 = 0;
        p->bits.winner = 0;
        p->bits.first = 31;
        p->bits.second = 31;
        p++;
    } while ((INT_PTR)p < (INT_PTR)&g_knockout.semis[2]);
    g_knockout.final.bits.played = 0;
    g_knockout.final.time1 = 0;
    g_knockout.final.time2 = 0;
    g_knockout.final.bits.winner = 0;
    g_knockout.final.bits.first = 31;
    g_knockout.final.bits.second = 31;
    switch (g_knockout.bits.mode) {
    case 4:
        if (CGameInfo::GetGameModeOptionBits20To22() != 0) participants = 16;
        for (i = 0; i < participants; i++) {
            if (i >= (CGameInfo::GetConfiguredPlayerCount() & 0xff))
                RallyData_ResetDriverRecordCategory((BYTE)i, aiIndex++);
            for (;;) {
                pick = rand() % 16;
                if (pick % 2 != 0) {
                    if (g_knockout.round1[pick / 2].bits.second == 31) {
                        g_knockout.round1[pick / 2].bits.second = i;
                        break;
                    }
                } else if (g_knockout.round1[pick / 2].bits.first == 31) {
                    g_knockout.round1[pick / 2].bits.first = i;
                    break;
                }
            }
        }
        break;
    case 3:
        if (CGameInfo::GetGameModeOptionBits20To22() != 0) participants = 8;
        for (i = 0; i < participants; i++) {
            if (i >= (CGameInfo::GetConfiguredPlayerCount() & 0xff))
                RallyData_ResetDriverRecordCategory((BYTE)i, aiIndex++);
            for (;;) {
                pick = rand() % 8;
                if (pick % 2 != 0) {
                    if (g_knockout.quarters[pick / 2].bits.second == 31) {
                        g_knockout.quarters[pick / 2].bits.second = i;
                        break;
                    }
                } else if (g_knockout.quarters[pick / 2].bits.first == 31) {
                    g_knockout.quarters[pick / 2].bits.first = i;
                    break;
                }
            }
        }
        break;
    case 2:
        if (CGameInfo::GetGameModeOptionBits20To22() != 0) participants = 4;
        for (i = 0; i < participants; i++) {
            if (i >= (CGameInfo::GetConfiguredPlayerCount() & 0xff))
                RallyData_ResetDriverRecordCategory((BYTE)i, aiIndex++);
            for (;;) {
                pick = rand() % 4;
                if (pick % 2 != 0) {
                    if (g_knockout.semis[pick / 2].bits.second == 31) {
                        g_knockout.semis[pick / 2].bits.second = i;
                        break;
                    }
                } else if (g_knockout.semis[pick / 2].bits.first == 31) {
                    g_knockout.semis[pick / 2].bits.first = i;
                    break;
                }
            }
        }
        break;
    case 1:
        if (CGameInfo::GetGameModeOptionBits20To22() != 0) participants = 2;
        for (i = 0; i < participants; i++) {
            if (i >= (CGameInfo::GetConfiguredPlayerCount() & 0xff))
                RallyData_ResetDriverRecordCategory((BYTE)i, aiIndex++);
            for (;;) {
                pick = rand() % 2;
                // The original assigns even draws to the second driver in this round.
                if (pick == 0) {
                    if (g_knockout.final.bits.second == 31) {
                        g_knockout.final.bits.second = i;
                        break;
                    }
                } else if (g_knockout.final.bits.first == 31) {
                    g_knockout.final.bits.first = i;
                    break;
                }
            }
        }
        break;
    }
    p = g_knockout.round1;
    for (i = 1; i <= matches; i++, p++) {
        if ((CGameInfo::GetConfiguredPlayerCount() & 0xff) > p->bits.first &&
            (CGameInfo::GetConfiguredPlayerCount() & 0xff) > p->bits.second && i < matches) {
            q = p + 1;
            for (j = matches - i; j != 0; j--, q++) {
                if ((CGameInfo::GetConfiguredPlayerCount() & 0xff) <= q->bits.first &&
                    (CGameInfo::GetConfiguredPlayerCount() & 0xff) <= q->bits.second) {
                    oldFirst = q->bits.first;
                    q->bits.first = p->bits.second;
                    p->bits.second = oldFirst;
                }
            }
        }
    }
    p = g_knockout.round1;
    do {
        if ((CGameInfo::GetConfiguredPlayerCount() & 0xff) > p->bits.first) {
            oldFirst = p->bits.first;
            oldSecond = p->bits.second;
            if ((CGameInfo::GetConfiguredPlayerCount() & 0xff) > oldSecond && oldSecond < oldFirst) {
                p->bits.first = oldSecond;
                p->bits.second = oldFirst;
            }
        }
        p++;
    } while ((INT_PTR)p < (INT_PTR)&g_knockout.round1[8]);
    p = g_knockout.quarters;
    do {
        if ((CGameInfo::GetConfiguredPlayerCount() & 0xff) > p->bits.first) {
            oldFirst = p->bits.first;
            oldSecond = p->bits.second;
            if ((CGameInfo::GetConfiguredPlayerCount() & 0xff) > oldSecond && oldSecond < oldFirst) {
                p->bits.first = oldSecond;
                p->bits.second = oldFirst;
            }
        }
        p++;
    } while ((INT_PTR)p < (INT_PTR)&g_knockout.quarters[4]);
    p = g_knockout.semis;
    do {
        if ((CGameInfo::GetConfiguredPlayerCount() & 0xff) > p->bits.first) {
            oldSecond = p->bits.second;
            oldFirst = p->bits.first;
            if ((CGameInfo::GetConfiguredPlayerCount() & 0xff) > oldSecond && oldSecond < oldFirst) {
                p->bits.first = oldSecond;
                p->bits.second = oldFirst;
            }
        }
        p++;
    } while ((INT_PTR)p < (INT_PTR)&g_knockout.semis[2]);
    if ((CGameInfo::GetConfiguredPlayerCount() & 0xff) > g_knockout.final.bits.first) {
        oldSecond = g_knockout.final.bits.second;
        if ((CGameInfo::GetConfiguredPlayerCount() & 0xff) > oldSecond && oldSecond < g_knockout.final.bits.first) {
            oldFirst = g_knockout.final.bits.first;
            g_knockout.final.bits.first = oldSecond;
            g_knockout.final.bits.second = oldFirst;
        }
    }
}

// FUNCTION: CMR2 0x00407820
unsigned int *RallyData_GetChampionshipState(void)
{
    return &g_knockout.state;
}

// Decodes the two 5-bit driver indices of the current round; which table
// they come from depends on the championship stage (bits 3-5).
// FUNCTION: CMR2 0x00407830
void RallyData_GetRoundDrivers(unsigned int *pFirst, unsigned int *pSecond)
{
    switch ((g_knockout.state >> 3) & 7) {
    case 1:
        *pFirst = g_knockout.round1[(g_knockout.state >> 0xc) & 0xf].flags & 0x1f;
        *pSecond = (g_knockout.round1[(g_knockout.state >> 0xc) & 0xf].flags >> 5) & 0x1f;
        break;
    case 2:
        *pFirst = g_knockout.quarters[(g_knockout.state >> 0xc) & 0xf].flags & 0x1f;
        *pSecond = (g_knockout.quarters[(g_knockout.state >> 0xc) & 0xf].flags >> 5) & 0x1f;
        break;
    case 3:
        *pFirst = g_knockout.semis[(g_knockout.state >> 0xc) & 0xf].flags & 0x1f;
        *pSecond = (g_knockout.semis[(g_knockout.state >> 0xc) & 0xf].flags >> 5) & 0x1f;
        break;
    case 4:
        *pFirst = g_knockout.final.flags & 0x1f;
        *pSecond = (g_knockout.final.flags >> 5) & 0x1f;
        break;
    }
}

// Stores the two times of the current round of the knockout table and marks
// the winner (bit 11: second driver faster, bit 12: first driver faster).
// FUNCTION: CMR2 0x00407940
void Knockout_SetCurrentMatchTimes(unsigned int first, unsigned int second)
{
    switch ((g_knockout.state >> 3) & 7) {
    case 1:
        g_knockout.round1[(g_knockout.state >> 0xc) & 0xf].time1 = first;
        g_knockout.round1[(g_knockout.state >> 0xc) & 0xf].time2 = second;
        if (second > first)
            g_knockout.round1[(g_knockout.state >> 0xc) & 0xf].flags =
                (g_knockout.round1[(g_knockout.state >> 0xc) & 0xf].flags & 0xffffefff) | 0x800;
        else
            g_knockout.round1[(g_knockout.state >> 0xc) & 0xf].flags =
                (g_knockout.round1[(g_knockout.state >> 0xc) & 0xf].flags & 0xfffff7ff) | 0x1000;
        break;
    case 2:
        g_knockout.quarters[(g_knockout.state >> 0xc) & 0xf].time1 = first;
        g_knockout.quarters[(g_knockout.state >> 0xc) & 0xf].time2 = second;
        if (second > first)
            g_knockout.quarters[(g_knockout.state >> 0xc) & 0xf].flags =
                (g_knockout.quarters[(g_knockout.state >> 0xc) & 0xf].flags & 0xffffefff) | 0x800;
        else
            g_knockout.quarters[(g_knockout.state >> 0xc) & 0xf].flags =
                (g_knockout.quarters[(g_knockout.state >> 0xc) & 0xf].flags & 0xfffff7ff) | 0x1000;
        break;
    case 3:
        g_knockout.semis[(g_knockout.state >> 0xc) & 0xf].time1 = first;
        g_knockout.semis[(g_knockout.state >> 0xc) & 0xf].time2 = second;
        if (second > first)
            g_knockout.semis[(g_knockout.state >> 0xc) & 0xf].flags =
                (g_knockout.semis[(g_knockout.state >> 0xc) & 0xf].flags & 0xffffefff) | 0x800;
        else
            g_knockout.semis[(g_knockout.state >> 0xc) & 0xf].flags =
                (g_knockout.semis[(g_knockout.state >> 0xc) & 0xf].flags & 0xfffff7ff) | 0x1000;
        break;
    case 4:
        g_knockout.final.time1 = first;
        g_knockout.final.time2 = second;
        if (second > first)
            g_knockout.final.flags = (g_knockout.final.flags & 0xffffefff) | 0x800;
        else
            g_knockout.final.flags = (g_knockout.final.flags & 0xfffff7ff) | 0x1000;
        break;
    }
}

// Propagates the winners through every level of the knockout bracket and
// orders AI-only pairs by driver index.
// FUNCTION: CMR2 0x00407b10
void Knockout_PropagateWinners(void)
{
    int i;

    for (i = 0; i < 8; i++) {
        if ((g_knockout.round1[i].flags & 0x1800) != 0) {
            if ((g_knockout.round1[i].flags & 0x1800) == 0x800) {
                if (i % 2 != 0)
                    g_knockout.quarters[i / 2].bits.second = g_knockout.round1[i].bits.first;
                else
                    g_knockout.quarters[i / 2].bits.first = g_knockout.round1[i].bits.first;
            } else {
                if (i % 2 != 0)
                    g_knockout.quarters[i / 2].bits.second = g_knockout.round1[i].bits.second;
                else
                    g_knockout.quarters[i / 2].bits.first = g_knockout.round1[i].bits.second;
            }
        }
    }
    for (i = 0; i < 4; i++) {
        if (g_knockout.quarters[i].bits.first < (CGameInfo::GetConfiguredPlayerCount() & 0xff)) {
            if (g_knockout.quarters[i].bits.second < (CGameInfo::GetConfiguredPlayerCount() & 0xff) &&
                g_knockout.quarters[i].bits.first > g_knockout.quarters[i].bits.second) {
                unsigned int f = g_knockout.quarters[i].bits.first;
                g_knockout.quarters[i].bits.first = g_knockout.quarters[i].bits.second;
                g_knockout.quarters[i].bits.second = f;
            }
        }
    }

    for (i = 0; i < 4; i++) {
        if ((g_knockout.quarters[i].flags & 0x1800) != 0) {
            if ((g_knockout.quarters[i].flags & 0x1800) == 0x800) {
                if (i % 2 != 0)
                    g_knockout.semis[i / 2].bits.second = g_knockout.quarters[i].bits.first;
                else
                    g_knockout.semis[i / 2].bits.first = g_knockout.quarters[i].bits.first;
            } else {
                if (i % 2 != 0)
                    g_knockout.semis[i / 2].bits.second = g_knockout.quarters[i].bits.second;
                else
                    g_knockout.semis[i / 2].bits.first = g_knockout.quarters[i].bits.second;
            }
        }
    }
    for (i = 0; i < 2; i++) {
        if (g_knockout.semis[i].bits.first < (CGameInfo::GetConfiguredPlayerCount() & 0xff)) {
            if (g_knockout.semis[i].bits.second < (CGameInfo::GetConfiguredPlayerCount() & 0xff) &&
                g_knockout.semis[i].bits.first > g_knockout.semis[i].bits.second) {
                unsigned int f = g_knockout.semis[i].bits.first;
                g_knockout.semis[i].bits.first = g_knockout.semis[i].bits.second;
                g_knockout.semis[i].bits.second = f;
            }
        }
    }

    for (i = 0; i < 2; i++) {
        if ((g_knockout.semis[i].flags & 0x1800) != 0) {
            if ((g_knockout.semis[i].flags & 0x1800) == 0x800) {
                if (i % 2 != 0)
                    g_knockout.final.bits.second = g_knockout.semis[i].bits.first;
                else
                    g_knockout.final.bits.first = g_knockout.semis[i].bits.first;
            } else {
                if (i % 2 != 0)
                    g_knockout.final.bits.second = g_knockout.semis[i].bits.second;
                else
                    g_knockout.final.bits.first = g_knockout.semis[i].bits.second;
            }
        }
    }
    if (g_knockout.final.bits.first < (CGameInfo::GetConfiguredPlayerCount() & 0xff)) {
        if (g_knockout.final.bits.second < (CGameInfo::GetConfiguredPlayerCount() & 0xff) &&
            g_knockout.final.bits.first > g_knockout.final.bits.second) {
            unsigned int f = g_knockout.final.bits.first;
            g_knockout.final.bits.first = g_knockout.final.bits.second;
            g_knockout.final.bits.second = f;
        }
    }
}

// FUNCTION: CMR2 0x004070f0
int RallyData_IsTwoUnassignedDriverMatch(void)
{
    unsigned int drivers[2];

    if (CGameInfo::GetConfiguredGameMode() == 4) {
        RallyData_GetChampionshipState();
        RallyData_GetRoundDrivers(&drivers[0], &drivers[1]);
        if (drivers[0] != drivers[1] && RallyData_GetUsableRecordCategory(drivers[0]) == -1 && RallyData_GetUsableRecordCategory(drivers[1]) == -1)
            return 1;
    }
    return 0;
}

// Number of classification entries of the current stage.
// GLOBAL: CMR2 0x00536c90
int g_splitCount;
// Classification table: per entry the route node index and the distance so far.
// GLOBAL: CMR2 0x00536ff0
int g_classificationTable[24];

// Builds the distance column of the stage classification table: walks the route
// nodes accumulating the length between consecutive nodes, and for every entry
// stores the node it reaches and the distance to it.
// The three vectors live in one aggregate: the original keeps a, b and diff in
// consecutive slots (-0x30, -0x24, -0x18), which MSVC6 only does for an aggregate.
// FUNCTION: CMR2 0x00411550
int RallyData_BuildStageRouteDistanceColumn(void)
{
    struct {
        FixVector a;
        FixVector b;
        FixVector diff;
    } v;
    int totalA;
    int totalB;
    int start;
    int i;
    int j;

    g_splitCount = 0;
    totalB = 0;
    totalA = 0;
    if ((char)RallyData_GetSelectionBits16To19() != 1) {
        g_splitCount = Stage_GetSplitPositionCount();
        if (g_splitCount > 0xc)
            g_splitCount = 0xc;
    } else {
        if ((BYTE)RallyData_GetFlag25() != 0)
            g_splitCount = Stage_GetSplitPositionCount() + 1;
        else
            g_splitCount = Stage_GetSplitPositionCount();
    }
    for (i = 0; i < RallyData_GetRouteAvailabilityState(); i++) {
        if (i > 0) {
            RallyData_GetRouteNodeGroundPosition(i - 1, (int *)&v.a);
            RallyData_GetRouteNodeGroundPosition(i, (int *)&v.b);
            v.diff.x = v.b.x - v.a.x;
            v.diff.y = v.b.y - v.a.y;
            v.diff.z = v.b.z - v.a.z;
            totalA += FixVecLength(&v.diff) / 100;
        }
    }
    for (i = 0; i < RallyData_GetRouteAvailabilityState(); i++) {
        if (i > 0) {
            RallyData_GetRouteNodeGroundPosition(i - 1, (int *)&v.a);
            RallyData_GetRouteNodeGroundPosition(i, (int *)&v.b);
            v.diff.x = v.b.x - v.a.x;
            v.diff.y = v.b.y - v.a.y;
            v.diff.z = v.b.z - v.a.z;
            totalB += FixVecLength(&v.diff) / 100;
        }
        for (j = 0; j < g_splitCount; j++) {
            if ((Stage_GetSplitPositionFixed(j) >> 0x10) == i) {
                g_classificationTable[j * 2] = i;
                g_classificationTable[j * 2 + 1] = totalB;
            }
        }
    }
    if ((BYTE)RallyData_GetFlag25() != 0) {
        start = g_splitCount - 1;
        if (start < 0xc) {
            for (; start < 0xc; start++) {
                g_classificationTable[start * 2 + 1] = totalB;
                g_classificationTable[start * 2] = RallyData_GetRouteAvailabilityState() - 1;
            }
        }
    } else if (g_splitCount < 0xc) {
        for (start = g_splitCount; start < 0xc; start++) {
            g_classificationTable[start * 2 + 1] = totalB;
            g_classificationTable[start * 2] = RallyData_GetRouteAvailabilityState() - 1;
        }
    }
    return totalB;
}

// FUNCTION: CMR2 0x00411880
int RallyData_IsHeadToHeadRaceMode(void)
{
    if (((BYTE)RallyDataState() > 1 && CGameInfo::IsConfiguredMultiplayer() == 0 && CGameInfo::GetConfiguredGameMode() != 4) || RallyData_IsTwoUnassignedDriverMatch() != 0)
        return 1;
    return 0;
}

// FUNCTION: CMR2 0x00406910
unsigned char RallyDataCountryIndex(void)
{
	return g_selectedRallyData & 0x1f;
}

// FUNCTION: CMR2 0x00406920
BYTE RallyData_GetSelectionStateByte(void)
{
    return g_unk0x0052f2a8;
}

// FUNCTION: CMR2 0x00406930
unsigned char RallyDataStageIndex(void)
{
	return g_selectedRallyData >> 5 & 0x1f;
}

// FUNCTION: CMR2 0x004074f0
unsigned char RallyDataState(void)
{
	return g_selectedRallyData >> 0xe & 3;
}

// FUNCTION: CMR2 0x00407e50
unsigned char RallyData_GetFlag24(void)
{
	return g_selectedRallyData >> 0x18 & 1;
}

// FUNCTION: CMR2 0x00407e60
unsigned char RallyData_GetFlag25(void)
{
	return g_selectedRallyData >> 0x19 & 1;
}

// Index check compiled out of the release build; kept because every
// RallyData accessor still calls it. The linker folded other empty debug
// functions into this one, so some callers pass pointers (a mesh, a trace
// string): the parameter is pointer-sized.
// FUNCTION: CMR2 0x004083f0
void RallyData_ValidateIndex(INT_PTR index)
{
}


// Increments counter 8 of this entry's category unless the entry is unassigned.
// FUNCTION: CMR2 0x00408f20
void RallyData_IncrementCategoryCounter8(int index)
{
    unsigned int record;
    unsigned int category;

    RallyData_ValidateIndex(index);
    record = *(unsigned int *)&g_saveSlots[index];
    if ((record & 0x3c0000) != 0x3c0000) {
        category = (record >> 18) & 15;
        ++g_saveProfiles[category].identifier.counters[0];
    }
}

// Increments the use count of this entry's category unless the entry is unassigned.
// FUNCTION: CMR2 0x00408f70
void RallyData_IncrementCategoryUse(int index)
{
    unsigned int record;
    unsigned int category;

    RallyData_ValidateIndex(index);
    record = *(unsigned int *)&g_saveSlots[index];
    if ((record & 0x3c0000) != 0x3c0000) {
        category = (record >> 18) & 15;
        ++g_saveProfiles[category].identifier.counters[1];
    }
}

// Increments counter 10 of this entry's category unless the entry is unassigned.
// FUNCTION: CMR2 0x00408fc0
void RallyData_IncrementCategoryCounter10(int index)
{
    unsigned int record;
    unsigned int category;

    RallyData_ValidateIndex(index);
    record = *(unsigned int *)&g_saveSlots[index];
    if ((record & 0x3c0000) != 0x3c0000) {
        category = (record >> 18) & 15;
        ++g_saveProfiles[category].identifier.counters[2];
    }
}

// Best medals of a category profile (DriverProfile::medals), each 3 - place
// (3 gold, 2 silver, 1 bronze, 0 none): the championship per difficulty, the
// arcade final per difficulty (two arcade selections on the two harder ones)
// and the rally of each of the eight countries (d<difficulty>i<country>).
// FrontendMenu_DrawProfileRecordsSummary shows them as a medal matrix.
struct CategoryMedals {
    unsigned championshipD0 : 2, championshipD1 : 2, championshipD2 : 2;
    unsigned arcadeD0 : 2, arcadeD1 : 2, arcadeD2 : 2, arcadeD1B : 2, arcadeD2B : 2;
    unsigned d0i0 : 2, d0i1 : 2, d0i2 : 2, d0i3 : 2, d0i4 : 2, d0i5 : 2, d0i6 : 2, d0i7 : 2;
    unsigned d1i0 : 2, d1i1 : 2, d1i2 : 2, d1i3 : 2, d1i4 : 2, d1i5 : 2, d1i6 : 2, d1i7 : 2;
    unsigned d2i0 : 2, d2i1 : 2, d2i2 : 2, d2i3 : 2, d2i4 : 2, d2i5 : 2, d2i6 : 2, d2i7 : 2;
};

// Records the medal of a rally finish (overall place 0..2) for `country` at
// the current difficulty if it beats the stored one, and counts the use.
// FUNCTION: CMR2 0x00409150
void RallyData_RecordRallyMedal(int index, int country, int place)
{
    CategoryMedals *pMedals;
    unsigned int medal;
    int category;

    RallyData_ValidateIndex(index);
    category = g_saveSlots[index].category;
    pMedals = (CategoryMedals *)g_saveProfiles[category].medals;
    medal = 3 - place;
    switch (CGameInfo::GetConfiguredDifficulty()) {
    case 0:
        switch (country) {
        case 0:
            if (medal > pMedals->d0i0) {
                pMedals->d0i0 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        case 1:
            if (medal > pMedals->d0i1) {
                pMedals->d0i1 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        case 2:
            if (medal > pMedals->d0i2) {
                pMedals->d0i2 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        case 3:
            if (medal > pMedals->d0i3) {
                pMedals->d0i3 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        case 4:
            if (medal > pMedals->d0i4) {
                pMedals->d0i4 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        case 5:
            if (medal > pMedals->d0i5) {
                pMedals->d0i5 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        case 6:
            if (medal > pMedals->d0i6) {
                pMedals->d0i6 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        case 7:
            if (medal > pMedals->d0i7) {
                pMedals->d0i7 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        }
        break;
    case 1:
        switch (country) {
        case 0:
            if (medal > pMedals->d1i0) {
                pMedals->d1i0 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        case 1:
            if (medal > pMedals->d1i1) {
                pMedals->d1i1 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        case 2:
            if (medal > pMedals->d1i2) {
                pMedals->d1i2 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        case 3:
            if (medal > pMedals->d1i3) {
                pMedals->d1i3 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        case 4:
            if (medal > pMedals->d1i4) {
                pMedals->d1i4 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        case 5:
            if (medal > pMedals->d1i5) {
                pMedals->d1i5 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        case 6:
            if (medal > pMedals->d1i6) {
                pMedals->d1i6 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        case 7:
            if (medal > pMedals->d1i7) {
                pMedals->d1i7 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        }
        break;
    case 2:
        switch (country) {
        case 0:
            if (medal > pMedals->d2i0) {
                pMedals->d2i0 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        case 1:
            if (medal > pMedals->d2i1) {
                pMedals->d2i1 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        case 2:
            if (medal > pMedals->d2i2) {
                pMedals->d2i2 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        case 3:
            if (medal > pMedals->d2i3) {
                pMedals->d2i3 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        case 4:
            if (medal > pMedals->d2i4) {
                pMedals->d2i4 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        case 5:
            if (medal > pMedals->d2i5) {
                pMedals->d2i5 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        case 6:
            if (medal > pMedals->d2i6) {
                pMedals->d2i6 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        case 7:
            if (medal > pMedals->d2i7) {
                pMedals->d2i7 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
            break;
        }
        break;
    }
}

// True when every medal of the driver's category profile (championship, every
// rally and arcade final at every difficulty) is exactly silver.
// FUNCTION: CMR2 0x004097b0
char RallyData_HasOnlySilverMedals(int index)
{
    CategoryMedals *pMedals;

    RallyData_ValidateIndex(index);
    if (g_saveSlots[index].category == 0xf)
        return 0;
    pMedals = (CategoryMedals *)g_saveProfiles[g_saveSlots[index].category].medals;
    if (pMedals->championshipD0 == 2 && pMedals->championshipD1 == 2 && pMedals->championshipD2 == 2 &&
        pMedals->d0i0 == 2 && pMedals->d0i1 == 2 && pMedals->d0i2 == 2 && pMedals->d0i3 == 2 &&
        pMedals->d0i4 == 2 && pMedals->d0i5 == 2 && pMedals->d0i6 == 2 && pMedals->d0i7 == 2 &&
        pMedals->d1i0 == 2 && pMedals->d1i1 == 2 && pMedals->d1i2 == 2 && pMedals->d1i3 == 2 &&
        pMedals->d1i4 == 2 && pMedals->d1i5 == 2 && pMedals->d1i6 == 2 && pMedals->d1i7 == 2 &&
        pMedals->d2i0 == 2 && pMedals->d2i1 == 2 && pMedals->d2i2 == 2 && pMedals->d2i3 == 2 &&
        pMedals->d2i4 == 2 && pMedals->d2i5 == 2 && pMedals->d2i6 == 2 && pMedals->d2i7 == 2 &&
        pMedals->arcadeD0 == 2 && pMedals->arcadeD1 == 2 && pMedals->arcadeD1B == 2 &&
        pMedals->arcadeD2 == 2 && pMedals->arcadeD2B == 2)
        return 1;
    return 0;
}

BYTE *StageUI_GetRaceResultTable(void);
int View_IsModeAvailable(BYTE index, int mode);

// match 63%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00423970
int RallyData_IsDriverViewModeAllowed(unsigned char index, int mode)
{
    int valid;

    switch (mode) {
    case 1:
    case 3:
    case 4:
    case 5:
        valid = 1;
        break;
    case 2:
        valid = ((char)((CallbackStateMachine *)StageUI_GetRaceResultTable())->records[(index & 0xff)].packedState != 10);
        break;
    default:
        return 0;
    }
    if (valid && View_IsModeAvailable(index, mode) != 0)
        return 1;
    return 0;
}

// FUNCTION: CMR2 0x004239e0
void Camera_AfterLeave(CameraRecord *pRecord)
{
    if (pRecord->type == 3)
        RallyData_ValidateIndex((INT_PTR)pRecord);
}

// FUNCTION: CMR2 0x00406940
unsigned int RallyData_GetSelectionBits10To11(void)
{
	return g_selectedRallyData >> 10 & 3;
}

// FUNCTION: CMR2 0x00406950
unsigned int RallyData_GetSelectionBits12To13(void)
{
	return g_selectedRallyData >> 12 & 3;
}

// FUNCTION: CMR2 0x00406990
unsigned int RallyData_GetSelectionBits16To19(void)
{
	return g_selectedRallyData >> 16 & 0xf;
}

// FUNCTION: CMR2 0x004069b0
unsigned int RallyData_GetKnockoutModeBits(void)
{
    return g_knockout.state & 7;
}

// FUNCTION: CMR2 0x004069a0
BYTE RallyData_GetSecondarySelectionNibble(void)
{
	BYTE result = g_unk0x0052f2a9;
	result &= 0xf;
	return result;
}

// FUNCTION: CMR2 0x00408020
void RallyData_UpdateFlags(void)
{
	g_selectedRallyData &= ~0x2000000;
	if (RallyDataStageIndex() == 10)
		g_selectedRallyData |= 0x2000000;

	g_selectedRallyData &= ~0x10000000;
	if (CGameInfo::GetConfiguredGameMode() == 7 || CGameInfo::GetConfiguredGameMode() == 3)
		g_selectedRallyData |= 0x10000000;

	g_selectedRallyData &= ~0x8000000;
	if ((g_selectedRallyData & 0x2000000) && !(g_selectedRallyData & 0x10000000))
		g_selectedRallyData |= 0x8000000;

	g_selectedRallyData &= ~0x1000000;
	if (CGameInfo::GetConfiguredGameMode() == 5 || CGameInfo::GetConfiguredGameMode() == 6 || CGameInfo::GetConfiguredGameMode() == 7 ||
	    CGameInfo::GetConfiguredGameMode() == 11 || CGameInfo::GetConfiguredGameMode() == 12)
		g_selectedRallyData |= 0x1000000;

	g_selectedRallyData &= ~0x4000000;
	if (CGameInfo::GetConfiguredGameMode() == 5 || CGameInfo::GetConfiguredGameMode() == 6)
		g_selectedRallyData |= 0x4000000;

	g_selectedRallyData &= ~0x80000000;
	if (CGameInfo::GetConfiguredGameMode() == 11 || CGameInfo::GetConfiguredGameMode() == 12)
		g_selectedRallyData |= 0x80000000;

	g_selectedRallyData &= ~0x400000;
	if (g_selectedRallyData & 0xc000000)
		g_selectedRallyData |= 0x400000;

	g_selectedRallyData &= ~0x20000000;
	if (CGameInfo::GetConfiguredGameMode() == 0 || CGameInfo::GetConfiguredGameMode() == 1 || CGameInfo::GetConfiguredGameMode() == 2 ||
	    CGameInfo::GetConfiguredGameMode() == 8 || CGameInfo::GetConfiguredGameMode() == 9 || CGameInfo::GetConfiguredGameMode() == 10)
		g_selectedRallyData |= 0x20000000;

	if (!(g_selectedRallyData & 0x1000000))
		g_selectedRallyData |= 0x40000000;

	g_selectedRallyData &= ~0x200000;
	if ((g_selectedRallyData & 0x20000000) || CGameInfo::GetConfiguredGameMode() == 4)
		g_selectedRallyData |= 0x200000;

	g_selectedRallyData &= ~0x800000;
	if (g_selectedRallyData & 0x31000000)
		g_selectedRallyData |= 0x800000;
}

// FUNCTION: CMR2 0x0040d5c0
void RallyData_ResetSelection(void)
{
	g_selectedRallyData = (g_selectedRallyData & 0xfff3c000) | 0x30000;
	g_knockout.state = (g_knockout.state & 0xfff802cc) | 0x802cc;
	g_unk0x0052f2b0 &= 0xfffffff8;
}

// FUNCTION: CMR2 0x0040d600
void RallyData_SetSelectionBits10To11(BYTE param1)
{
	g_selectedRallyData = ((param1 & 3) << 10) | (g_selectedRallyData & 0xfffff3ffU);
}

// FUNCTION: CMR2 0x0040d620
void RallyData_SetSelectionBits16To19(BYTE param1)
{
	g_selectedRallyData = ((param1 & 0xf) << 16) | (g_selectedRallyData & 0xfff0ffffU);
}

// FUNCTION: CMR2 0x0040d640
void RallyData_SetSecondarySelectionNibble(BYTE param1)
{
	g_unk0x0052f2a9 = g_unk0x0052f2a9 ^ (g_unk0x0052f2a9 ^ param1) & 0xf;
}

// FUNCTION: CMR2 0x0040d660
void RallyData_SetKnockoutModeBits(BYTE param1)
{
	g_knockout.state = (param1 & 7) | (g_knockout.state & 0xfffffff8U);
}

// FUNCTION: CMR2 0x0040d680
void RallyData_SetKnockoutBits6To8(BYTE param1)
{
	g_knockout.state = ((param1 & 7) << 6) | (g_knockout.state & 0xfffffe3fU);
}

// FUNCTION: CMR2 0x0040d6a0
void RallyData_SetKnockoutCountryNibble(BYTE param1)
{
	g_knockout.state = ((param1 & 0xf) << 16) | (g_knockout.state & 0xfff0ffffU);
}

// GLOBAL: CMR2 0x0051682c
BYTE g_unk0x0051682c[132] = {
    0x04, 0x02, 0x32, 0x32, 0x32, 0x32, 0x32, 0x04, 0x02, 0x32, 0x32, 0x32, 0x32, 0x32, 0x00, 0x02,
    0x32, 0x32, 0x32, 0x32, 0x32, 0x06, 0x02, 0x32, 0x32, 0x32, 0x32, 0x32, 0x04, 0x02, 0x32, 0x32,
    0x32, 0x32, 0x32, 0x04, 0x02, 0x32, 0x32, 0x32, 0x32, 0x32, 0x00, 0x02, 0x32, 0x32, 0x32, 0x32,
    0x32, 0x02, 0x02, 0x32, 0x32, 0x32, 0x32, 0x32, 0x00, 0x02, 0x32, 0x32, 0x32, 0x32, 0x32, 0x00,
    0xff, 0xff, 0xff, 0xff, 0x20, 0x00, 0x00, 0x00, 0x53, 0x4e, 0x4f, 0x57, 0x00, 0x00, 0x00, 0x00,
    0x53, 0x54, 0x4f, 0x52, 0x4d, 0x00, 0x00, 0x00, 0x52, 0x41, 0x49, 0x4e, 0x00, 0x00, 0x00, 0x00,
    0x44, 0x52, 0x49, 0x5a, 0x5a, 0x4c, 0x45, 0x00, 0x43, 0x4c, 0x4f, 0x55, 0x44, 0x59, 0x00, 0x00,
    0x42, 0x52, 0x49, 0x47, 0x48, 0x54, 0x00, 0x00, 0x42, 0x4c, 0x49, 0x5a, 0x5a, 0x41, 0x52, 0x44,
};

BYTE *RallyData_GetDriverSkillRecord(int index);

// Copies the country's default 7-byte settings into the four player records;
// in some championship stages the first value is bumped to the next odd one.
// The 7-byte per-country record copied as a whole.
struct CountryPlayerDefaults {
    BYTE values[7];
};

// FUNCTION: CMR2 0x00406820
void RallyData_CopyCountryPlayerDefaults(void)
{
    int i;
    BYTE *p;
    char c;

    for (i = 0; i < 4; i++) {
        p = RallyData_GetDriverSkillRecord(i);
        *(CountryPlayerDefaults *)p =
            *(CountryPlayerDefaults *)(g_unk0x0051682c + (RallyDataCountryIndex() & 0xff) * 7);
        if (g_driverGroupTable.field_0x6d0 != 0 && g_driverGroupTable.field_0x53c > 2) {
            c = p[0];
            if (c % 2 == 0 && c != 6)
                p[0] = c + 1;
        }
    }
}

// FUNCTION: CMR2 0x00406890
char *RallyData_GetCountrySevenByteRecord(void)
{
    return (char *)g_unk0x0051682c + (unsigned char)RallyDataCountryIndex() * 7;
}

// FUNCTION: CMR2 0x004068b0
void RallyData_SetCountrySelectionBits(BYTE param1)
{
    g_selectedRallyData = (param1 & 0x1f) | (g_selectedRallyData & 0xffffffe0U);
}

// FUNCTION: CMR2 0x00406960
void RallyData_SetSelectionBits12To13(BYTE param1)
{
    g_selectedRallyData = (g_selectedRallyData & 0xffffcfffU) | ((param1 & 3) << 12);
    RallyData_UpdateFlags();
}

// FUNCTION: CMR2 0x004068d0
void RallyData_SetSelectionStateByte(char param1)
{
    g_unk0x0052f2a8 = param1;
}

// FUNCTION: CMR2 0x004068e0
void RallyData_SetStageSelectionAndRefreshFlags(BYTE param1)
{
    g_selectedRallyData = (g_selectedRallyData & 0xfffffc1fU) | ((param1 & 0x1f) << 5);
    RallyData_UpdateFlags();
}

// GLOBAL: CMR2 0x005200e8
BYTE g_unk0x005200e8[0x40] = {
    0x00, 0x04, 0x64, 0x32, 0x32, 0x32, 0x32, 0x06, 0x04, 0x32, 0x32, 0x32, 0x32, 0x32, 0x04, 0x04,
    0x32, 0x32, 0x32, 0x32, 0x32, 0x04, 0x04, 0x32, 0x32, 0x32, 0x32, 0x32, 0x04, 0x04, 0x32, 0x32,
    0x32, 0x32, 0x32, 0x00, 0x04, 0x64, 0x32, 0x32, 0x32, 0x32, 0x04, 0x04, 0x00, 0x32, 0x32, 0x32,
    0x32, 0x02, 0x04, 0x32, 0x32, 0x32, 0x32, 0x32, 0x00, 0x04, 0x64, 0x32, 0x32, 0x32, 0x32,
};

// FUNCTION: CMR2 0x00494a40
char *RallyData_GetSelectedSettingSevenByteRecord(void)
{
    return (char *)g_unk0x005200e8 +
        ((unsigned char)RallyData_GetSelectionBits10To11() * 3 +
         (unsigned char)RallyData_GetSelectionBits12To13()) * 7;
}

// GLOBAL: CMR2 0x0052ea68
BYTE g_unk0x0052ea68[11];
// Sorted distinct values (9 slots; 0x4081d0 can append past them into the
// next table, as the original does), the 8 per-slot values and the count.
// GLOBAL: CMR2 0x0052ea74
int g_unk0x0052ea74[9];
// GLOBAL: CMR2 0x0052ea98
int g_unk0x0052ea98[8];
// GLOBAL: CMR2 0x0052eab8
int g_unk0x0052eab8;

// FUNCTION: CMR2 0x00408300
BYTE RallyData_IsFirstAvailableStageSelected(void)
{
    unsigned int i;
    BYTE result;

    for (i = 0; i < 0xb; i++) {
        if ((g_unk0x0052ea68[i] & 1) == 0)
            continue;
        if ((g_unk0x0052ea68[i] & 2) != 0 && !CGameInfo::IsRecordFlagSet(0xd))
            continue;
        if ((g_unk0x0052ea68[i] & 4) == 0)
            break;
    }
    result = ((g_selectedRallyData >> 5) & 0x1f) == i;
    return result;
}

// FUNCTION: CMR2 0x00408390
void RallyData_SelectFirstAvailableStage(void)
{
    unsigned int i;

    for (i = 0; i < 0xb; i++) {
        if ((g_unk0x0052ea68[i] & 1) == 0)
            continue;
        if ((g_unk0x0052ea68[i] & 2) != 0 && !CGameInfo::IsRecordFlagSet(0xd))
            continue;
        if ((g_unk0x0052ea68[i] & 4) == 0) {
            RallyData_SetStageSelectionAndRefreshFlags((BYTE)i);
            return;
        }
    }
}

// FUNCTION: CMR2 0x00408340
BYTE RallyData_IsLastAvailableStageSelected(void)
{
    unsigned int i;

    for (i = ((g_selectedRallyData >> 5) & 0x1f) + 1; i < 0xb; i++) {
        if ((g_unk0x0052ea68[i] & 1) != 0 &&
            ((g_unk0x0052ea68[i] & 2) == 0 || CGameInfo::IsRecordFlagSet(0xd)) &&
            (g_unk0x0052ea68[i] & 4) == 0)
            return 0;
    }
    return 1;
}





extern int g_profileFileCount;
extern int g_unk0x00531654[4];
extern DriverProfileIdentifier *g_pProfileIdentifiers;
void RallyData_MarkAllDriverRecordsUnassigned(void);
void RallyData_SetPlayerDefaultCarSetup(int player);

// Resets the four players' records in the save block (all cleared, then the
// default flags, setup values and car position of each one).
// match 95%: only the schedule of the g_profileFileCount store against the two
// loop pointers differs.
// FUNCTION: CMR2 0x004eadb0
void RallyData_ResetSavedPlayerRecords(void)
{
    int i;

    memset(g_saveProfiles, 0, sizeof(g_saveProfiles));
    memset(g_saveSlots, 0, 0xc0 * 4);
    g_profileFileCount = 0;
    for (i = 0; i < 4; i++) {
        g_saveProfiles[i].car = 4;
        g_saveProfiles[i].identifier.flags = (g_saveProfiles[i].identifier.flags & 0xffe1f17e) | 0x1017e;
        g_saveDirty[i] = 0;
        g_saveProfiles[i].options |= 0x20;
        g_unk0x00531654[i] = 0;
        g_saveProfiles[i].awards = 0xf11;
        RallyData_SetPlayerDefaultCarSetup(i);
    }
    RallyData_MarkAllDriverRecordsUnassigned();
    g_pProfileIdentifiers = NULL;
}

// Stores a driver (GetConfiguredGameMode()==4) or category (otherwise) name into the
// championship save data, marking the record as edited and randomising the
// two packed counters of the category record.
// FUNCTION: CMR2 0x004eae90
void RallyData_SetEditedDriverOrCategoryName(unsigned int slot, char *pName)
{
    unsigned int category;

    slot &= 0xff;
    RallyData_ValidateIndex(slot);
    if (CGameInfo::GetConfiguredGameMode() == 4) {
        strcpy(g_saveCarRecords[slot].name, pName);
        g_saveCarRecords[slot].flags |= 2;
        return;
    }
    category = g_saveSlots[slot].category;
    if (category == 0xf)
        return;
    strcpy(g_saveProfiles[category].identifier.name, pName);
    g_saveDirty[category] = 1;
    g_saveProfiles[category].identifier.flags =
        (rand() & 0xf) << 0xc |
        (g_saveProfiles[category].identifier.flags & 0xffff0fffU);
    g_saveProfiles[category].identifier.flags =
        (rand() & 0x3f) << 0x16 |
        (g_saveProfiles[category].identifier.flags & 0xf03fffffU);
}

// Copies the name into the category record of the given record index.
// FUNCTION: CMR2 0x004eaf90
void RallyData_CopyCategoryRecordName(BYTE index, char *name)
{
    unsigned int category;

    RallyData_ValidateIndex(index);
    category = g_saveSlots[index].category;
    strcpy(g_saveProfiles[category].name, name);
    g_saveDirty[category] = 1;
}

// Cheat-code names (last match wins) and the block of strings they point at.
// The test only reports the index, which is turned into a flag elsewhere.
extern BYTE g_cheatStrings[];

// GLOBAL: CMR2 0x005250fc
BYTE *g_cheatNames[20] = {
    g_cheatStrings + 0x104, g_cheatStrings + 0xfc, g_cheatStrings + 0xec,
    g_cheatStrings + 0xe0, g_cheatStrings + 0xd8, g_cheatStrings + 0xc8,
    g_cheatStrings + 0xb8, g_cheatStrings + 0xac, g_cheatStrings + 0x9c,
    g_cheatStrings + 0x90, g_cheatStrings + 0x84, g_cheatStrings + 0x74,
    g_cheatStrings + 0x64, g_cheatStrings + 0x58, g_cheatStrings + 0x44,
    g_cheatStrings + 0x34, g_cheatStrings + 0x28, g_cheatStrings + 0x18,
    g_cheatStrings + 0x10, g_cheatStrings + 0x0
};

// GLOBAL: CMR2 0x0052514c
BYTE g_cheatStrings[] =
    "turnontheice\0\0\0\0"
    "oohnice\0"
    "automode please\0"
    "morrismode\0\0"
    "shinybuttons\0\0\0\0"
    "hello razu and flea\0"
    "greatnews\0\0\0"
    "allthebuttons\0\0\0"
    "gofasterstripes\0"
    "curryforme\0\0"
    "nuttynets\0\0\0"
    "bouncybouncy\0\0\0\0"
    "wheelybig\0\0\0"
    "boingboingboing\0"
    "waveyourlefts\0\0\0"
    "eatthis\0"
    "garywildass\0"
    "showmethecossy\0\0"
    "minime\0\0"
    "evilevo";

// Looks up a cheat code among the known names: on the first match writes the
// index of the name to param_2 and returns 1, else writes nothing and returns 0.
// FUNCTION: CMR2 0x004eb290
BYTE RallyData_FindCheatNameIndex(int param_1, BYTE *param_2)
{
    BYTE **p;
    int i;
    BYTE result;

    RallyData_ValidateIndex(param_1);
    result = 0;
    i = 0;
    p = g_cheatNames;
    while ((INT_PTR)p < (INT_PTR)(g_cheatNames + 20)) {
        if (FrontendProfile_DoesDriverMatchEntry(param_1, *p)) {
            *param_2 = CGameInfo::ToggleRecordFlag(i);
            result = 1;
            break;
        }
        p++;
        i++;
    }
    return result;
}

// Writes the category profile of the record `param_1` points at when its name
// was edited and clears the edited flag; returns the result of the write.
// FUNCTION: CMR2 0x004eb370
char RallyData_SaveEditedCategoryProfile(int param_1)
{
    unsigned int category;
    char result;

    RallyData_ValidateIndex(param_1);
    result = 1;
    category = g_saveSlots[param_1].category;
    if ((g_saveProfiles[category].identifier.flags & 0x200000) == 0 &&
        g_saveDirty[category] != 0) {
        result = Profile_WriteSaveFile(0, (BYTE *)&g_saveProfiles[category]);
        if (result != 0)
            g_saveDirty[(g_saveSlots[param_1].category)] = 0;
    }
    return result;
}

// Championship medal of the driver's category at `difficulty` (0..2).
// FUNCTION: CMR2 0x004eba60
unsigned int RallyData_GetChampionshipMedal(int index, int difficulty)
{
    CategoryMedals *pMedals;

    RallyData_ValidateIndex(index);
    pMedals = (CategoryMedals *)g_saveProfiles[g_saveSlots[index].category].medals;
    switch (difficulty) {
    case 0: return pMedals->championshipD0;
    case 1: return pMedals->championshipD1;
    case 2: return pMedals->championshipD2;
    }
    return 0;
}

// Rally medal of the driver's category for `country` (0..7) at `difficulty`.
// FUNCTION: CMR2 0x004ebad0
unsigned int RallyData_GetRallyMedal(int index, int country, int difficulty)
{
    CategoryMedals *pMedals;

    RallyData_ValidateIndex(index);
    pMedals = (CategoryMedals *)g_saveProfiles[g_saveSlots[index].category].medals;
    switch (difficulty) {
    case 0:
        switch (country) {
        case 0: return pMedals->d0i0;
        case 1: return pMedals->d0i1;
        case 2: return pMedals->d0i2;
        case 3: return pMedals->d0i3;
        case 4: return pMedals->d0i4;
        case 5: return pMedals->d0i5;
        case 6: return pMedals->d0i6;
        case 7: return pMedals->d0i7;
        }
        break;
    case 1:
        switch (country) {
        case 0: return pMedals->d1i0;
        case 1: return pMedals->d1i1;
        case 2: return pMedals->d1i2;
        case 3: return pMedals->d1i3;
        case 4: return pMedals->d1i4;
        case 5: return pMedals->d1i5;
        case 6: return pMedals->d1i6;
        case 7: return pMedals->d1i7;
        }
        break;
    case 2:
        switch (country) {
        case 0: return pMedals->d2i0;
        case 1: return pMedals->d2i1;
        case 2: return pMedals->d2i2;
        case 3: return pMedals->d2i3;
        case 4: return pMedals->d2i4;
        case 5: return pMedals->d2i5;
        case 6: return pMedals->d2i6;
        case 7: return pMedals->d2i7;
        }
        break;
    }
    return 0;
}

// Arcade final medal of the driver's category at `difficulty`; the two harder
// difficulties keep one per arcade selection (`arcade` 0 or 1).
// FUNCTION: CMR2 0x004ebcd0
unsigned int RallyData_GetArcadeMedal(int index, int arcade, int difficulty)
{
    CategoryMedals *pMedals;

    RallyData_ValidateIndex(index);
    pMedals = (CategoryMedals *)g_saveProfiles[g_saveSlots[index].category].medals;
    switch (difficulty) {
    case 0:
        return pMedals->arcadeD0;
    case 1:
        if (arcade == 0)
            return pMedals->arcadeD1;
        return pMedals->arcadeD1B;
    case 2:
        if (arcade == 0)
            return pMedals->arcadeD2;
        return pMedals->arcadeD2B;
    }
    return 0;
}

void GameInfo_GetDefaultCameraParameters(FixVector *pOffset, short *pPitch, int *pLag);
int GameInfo_GetField98(void);
void RallyData_SetDriverCameraRecord(BYTE index, ChaseCameraSettings *pValues);

// Resets the settings of record `index` to the defaults of the options.
// FUNCTION: CMR2 0x004ec210
void RallyData_ResetRecordOptionDefaults(int index)
{
    ChaseCameraSettings values;

    RallyData_ValidateIndex(index);
    memset(&values, 0, sizeof(values));
    GameInfo_GetDefaultCameraParameters(&values.offset, &values.pitch, &values.lag);
    RallyData_SetDriverCameraRecord(index, &values);
}

// Marks record `index` (and its category's profile) as in use or not; when
// set, the record starts again from the defaults.
// match 65%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004eb000
void RallyData_SetPlayerProfileInUse(BYTE index, char set)
{
    unsigned int i;
    unsigned int record;
    unsigned int category;
    unsigned int bit;

    i = index;
    RallyData_ValidateIndex(i);
    record = *(unsigned int *)&g_saveSlots[i];
    category = record >> 0x12 & 0xf;
    if (category != 0xf) {
        bit = set & 1;
        record = (record & 0xfbffffff) | bit << 0x1a;
        g_saveProfiles[category].identifier.flags =
            (g_saveProfiles[category].identifier.flags & 0xffdfffff) | bit << 0x15;
        record |= 0x2000;
        *(unsigned int *)&g_saveSlots[i] = record;
        if (set != 0) {
            *(unsigned int *)&g_saveSlots[i] = record & 0xffffe000;
            g_saveSlots[i].time = 0;
            RallyData_ResetRecordOptionDefaults(i);
            g_saveProfiles[i].car = GameInfo_GetField98();
            g_saveProfiles[category].car = GameInfo_GetField98();
        }
    }
}

// Which of the 4 profiles are used by the first 4 records.
#define RALLYDATA_USED_PROFILES(used)                                             \
    for (p = 0; p < 4; p++) {                                                     \
        used[p] = 0;                                                              \
        for (pRecord = g_saveSlots; pRecord < g_saveSlots + 4; pRecord++) {          \
            if (pRecord->category == p) {                                         \
                used[p] = 1;                                                      \
                break;                                                            \
            }                                                                     \
        }                                                                         \
    }

// Number of saved profiles (named, not hidden) no record is using.
// FUNCTION: CMR2 0x004ec020
int SaveProfiles_CountAvailableProfiles(void)
{
    char used[4];
    SaveSlot *pRecord;
    DriverProfile *pProfile;
    unsigned int p;
    int count;
    unsigned int i;

    count = 0;
    RALLYDATA_USED_PROFILES(used)
    i = 0;
    for (i = 0; i < 4; i++) {
        if (!used[i] && g_saveProfiles[i].identifier.name[0] != 0 && (g_saveProfiles[i].identifier.flags & 0x200000) == 0)
            count++;
    }
    return count;
}

// Index of the n-th free saved profile, -1 if none.
// FUNCTION: CMR2 0x004ec090
int SaveProfiles_GetAvailableProfileIndex(int n)
{
    char used[4];
    SaveSlot *pRecord;
    unsigned int p;
    int count;
    unsigned int i;

    count = 0;
    RALLYDATA_USED_PROFILES(used)
    i = 0;
    for (i = 0; i < 4; i++) {
        if (used[i] == 0 && g_saveProfiles[i].identifier.name[0] != 0 && (g_saveProfiles[i].identifier.flags & 0x200000) == 0) {
            if (count == n)
                return i;
            count++;
        }
    }
    return -1;
}

// Name of the n-th free saved profile, NULL if none.
// FUNCTION: CMR2 0x004ec110
BYTE *SaveProfiles_GetAvailableProfileName(int n)
{
    char used[4];
    SaveSlot *pRecord;
    unsigned int p;
    int count;
    unsigned int i;

    count = 0;
    RALLYDATA_USED_PROFILES(used)
    i = 0;
    for (i = 0; i < 4; i++) {
        if (used[i] == 0 && g_saveProfiles[i].identifier.name[0] != 0 && (g_saveProfiles[i].identifier.flags & 0x200000) == 0) {
            if (count == n)
                return (BYTE *)g_saveProfiles[i].identifier.name;
            count++;
        }
    }
    return NULL;
}

// FUNCTION: CMR2 0x004ebfd0
void RallyData_MarkDriverRecordUnassigned(int index)
{
    unsigned int value = *(unsigned int *)&g_saveSlots[index];
    value = (value & 0xffffdfffU) | 0x3c0000;
    *(unsigned int *)&g_saveSlots[index] = value;
}

// FUNCTION: CMR2 0x004ec000
void RallyData_MarkAllDriverRecordsUnassigned(void)
{
    int i = 0;
    do {
        RallyData_MarkDriverRecordUnassigned(i);
        i++;
    } while (i < 16);
}

// Fills the split-marker rows of the stage-split editor menu, either with the
// current list of splits (up to three, or four markers in the wide layout) or
// with the empty/default layout.
// match 69%: the original keeps the split list in EDI and the row index in ESI
// (we get them swapped), so every register in the marker loop differs.
// FUNCTION: CMR2 0x004efe60
void RallyData_FillStageSplitEditorRows(Menu *param_1, int param_2, char param_3)
{
    Menu *pMenu;
    short *pSplits;
    int count;
    int i;
    int half;
    short value;

    RallyData_RebuildDistinctValueList();
    count = RallyData_GetDistinctValueCount();
    pSplits = (short *)RallyData_GetDistinctValueList();
    RallyData_SetDriverPairingStateValues(0, 0);
    pMenu = FrontendMenu_GetAlternateRallyStageSelection();
    pMenu->items[0].id = 0x156;
    pMenu = FrontendMenu_GetAlternateRallyStageSelection();
    pMenu->items[0].value = 0xffff;
    pMenu = FrontendMenu_GetAlternateRallyStageSelection();
    pMenu->items[0].enabled = param_3;
    if ((unsigned short)RallyData_GetPrimaryStageScoreScale() >= 0x834) {
        RallyData_SetDriverPairingStateValues(1, 2);
        pMenu = FrontendMenu_GetAlternateRallyStageSelection();
        pMenu->itemCount = 0;
        Menu_SetInputStateFlag(0);
        Menu_SetNextAction(FrontendMenu_GetRallyStartTransition());
        Menu_SetInputStateFlag(1);
    } else if (RallyDataStageIndex() == '\n') {
        RallyData_SetDriverPairingStateValues(1, 0);
        pMenu = FrontendMenu_GetAlternateRallyStageSelection();
        pMenu->itemCount = 0;
        Menu_SetInputStateFlag(0);
        Menu_SetNextAction(FrontendMenu_GetRallyStartTransition());
        Menu_SetInputStateFlag(1);
    } else {
        Menu_SetInputStateFlag(param_3);
        Menu_SetNextAction(FrontendMenu_GetAlternateRallyStageSelection());
        Menu_SetInputStateFlag(1);
        if (count <= 3) {
            pMenu = FrontendMenu_GetAlternateRallyStageSelection();
            pMenu->itemCount = (char)count + 1;
            if (count > 0) {
                i = 0;
                do {
                    value = *pSplits;
                    pMenu = FrontendMenu_GetAlternateRallyStageSelection();
                    pMenu->items[i + 1].id = value + 0x157;
                    pMenu = FrontendMenu_GetAlternateRallyStageSelection();
                    pMenu->items[i + 1].value = *pSplits;
                    pMenu = FrontendMenu_GetAlternateRallyStageSelection();
                    pMenu->items[i + 1].enabled = param_3;
                    pSplits += 2;
                    count--;
                    i++;
                } while (count != 0);
            }
        } else {
            pMenu = FrontendMenu_GetAlternateRallyStageSelection();
            pMenu->itemCount = 4;
            value = *pSplits;
            pMenu = FrontendMenu_GetAlternateRallyStageSelection();
            pMenu->items[1].id = value + 0x157;
            pMenu = FrontendMenu_GetAlternateRallyStageSelection();
            pMenu->items[1].value = *pSplits;
            pMenu = FrontendMenu_GetAlternateRallyStageSelection();
            pMenu->items[1].enabled = param_3;
            half = count / 2;
            value = pSplits[half * 2];
            pMenu = FrontendMenu_GetAlternateRallyStageSelection();
            pMenu->items[2].id = value + 0x157;
            pMenu = FrontendMenu_GetAlternateRallyStageSelection();
            pMenu->items[2].enabled = param_3;
            pMenu = FrontendMenu_GetAlternateRallyStageSelection();
            pMenu->items[2].value = pSplits[half * 2];
            value = pSplits[count * 2 - 2];
            pMenu = FrontendMenu_GetAlternateRallyStageSelection();
            pMenu->items[3].id = value + 0x157;
            pMenu = FrontendMenu_GetAlternateRallyStageSelection();
            pMenu->items[3].value = pSplits[count * 2 - 2];
            pMenu = FrontendMenu_GetAlternateRallyStageSelection();
            pMenu->items[3].enabled = param_3;
        }
    }
}

// The setting pair RallyData_GetDriverSettingPair returns in the rally modes;
// callers read both values.
// GLOBAL: CMR2 0x0052ea60
int g_unk0x0052ea60[2];

unsigned int RallyData_GetSelectionBits10To11(void);
unsigned char RallyData_GetSelectionFlag26(void);
unsigned int RallyData_GetSelectionBits12To13(void);

// Setting pair of a driver; in the rally modes it comes from the current
// championship position (both values the same).
// FUNCTION: CMR2 0x00407520
int *RallyData_GetDriverSettingPair(int index)
{
    if ((BYTE)RallyData_GetFlag24()) {
        if ((BYTE)RallyData_GetSelectionFlag26())
            g_unk0x0052ea60[1] = g_knockout.settingPairs[(RallyData_GetSelectionBits10To11() & 0xff) * 3 + (RallyData_GetSelectionBits12To13() & 0xff)]
                                                     [CGameInfo::GetConfiguredDifficulty() & 0xff];
        else
            g_unk0x0052ea60[1] = g_knockout.settingPairs[(RallyData_GetSelectionBits10To11() & 0xff) * 3 + (RallyData_GetSelectionBits12To13() & 0xff)][0];
        g_unk0x0052ea60[0] = g_unk0x0052ea60[1];
        return g_unk0x0052ea60;
    }
    return g_rallyPairingTables.pairs[index];
}

// FUNCTION: CMR2 0x004075b0
int *RallyData_GetDriverPairRecord(int index)
{
    return &g_rallyPairingTables.flags[index];
}

// FUNCTION: CMR2 0x004075c0
int *RallyData_GetDriverSecondaryPairRecord(int index)
{
    return &g_rallyPairingTables.otherFlags[index];
}

// FUNCTION: CMR2 0x004075d0
BYTE *RallyData_GetDriverSelectionFlagRecord(int index)
{
    return &g_driverGroupTable.field_0x6d4[index];
}

// FUNCTION: CMR2 0x004075e0
int *RallyData_GetDriverPrimaryPairRecord(int index)
{
    return &g_rallyPairingTables.skills[index];
}

// FUNCTION: CMR2 0x004075f0
unsigned int *RallyData_GetSelectedCountryValueTable(void)
{
    return g_unk0x0051627c[g_selectedRallyData & 0x1f];
}


// FUNCTION: CMR2 0x00407630
BYTE *RallyData_GetDriverSkillRecord(int index)
{
    return g_driverGroupTable.field_0x520[index];
}

// GLOBAL: CMR2 0x00516188
BYTE g_stageScoreScale[216] = {
    0x06, 0x09, 0x0c, 0x0c, 0x0e, 0x10, 0x10, 0x11, 0x08, 0x08, 0x0c, 0x0d, 0x0f, 0x10, 0x15, 0x15, 0x08, 0x0a, 0x0c, 0x0d, 0x0f, 0x0f, 0x00, 0x00,
    0x07, 0x09, 0x0d, 0x0f, 0x0f, 0x0f, 0x13, 0x13, 0x09, 0x0a, 0x0c, 0x0d, 0x0e, 0x10, 0x12, 0x12, 0x07, 0x07, 0x09, 0x0b, 0x11, 0x11, 0x00, 0x00,
    0x0a, 0x0b, 0x0c, 0x0d, 0x0f, 0x10, 0x11, 0x11, 0x08, 0x09, 0x0e, 0x0f, 0x0f, 0x0f, 0x13, 0x13, 0x06, 0x09, 0x0f, 0x0f, 0x12, 0x12, 0x00, 0x00,
    0x09, 0x09, 0x0d, 0x0d, 0x0e, 0x0f, 0x13, 0x13, 0x0b, 0x0b, 0x0c, 0x0e, 0x0f, 0x10, 0x15, 0x15, 0x08, 0x09, 0x0c, 0x0d, 0x12, 0x12, 0x00, 0x00,
    0x07, 0x09, 0x0d, 0x0f, 0x0f, 0x0f, 0x13, 0x13, 0x0b, 0x0c, 0x0c, 0x0d, 0x0e, 0x10, 0x11, 0x13, 0x06, 0x09, 0x0d, 0x0d, 0x09, 0x09, 0x00, 0x00,
    0x06, 0x09, 0x0c, 0x0c, 0x0e, 0x10, 0x10, 0x11, 0x08, 0x08, 0x0c, 0x0d, 0x0f, 0x10, 0x12, 0x13, 0x08, 0x0a, 0x15, 0x15, 0x0a, 0x0a, 0x00, 0x00,
    0x07, 0x09, 0x0d, 0x0f, 0x0f, 0x0f, 0x13, 0x13, 0x09, 0x0a, 0x0c, 0x0d, 0x0e, 0x10, 0x12, 0x12, 0x07, 0x07, 0x09, 0x0b, 0x10, 0x10, 0x00, 0x00,
    0x08, 0x0a, 0x0b, 0x0d, 0x10, 0x10, 0x12, 0x13, 0x06, 0x08, 0x0c, 0x0c, 0x0d, 0x0d, 0x15, 0x15, 0x06, 0x07, 0x0c, 0x0d, 0x12, 0x12, 0x00, 0x00,
    0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c,
};
// Score scale of the rally modes by championship, rally and difficulty (3 x 3 x 3).
// GLOBAL: CMR2 0x00516260
BYTE g_rallyScoreScale[27] = {
    0x0a, 0x0a, 0x0a, 0x0c, 0x0c, 0x0c, 0x10, 0x10, 0x10,
    0x0a, 0x0a, 0x0a, 0x0c, 0x0c, 0x0c, 0x10, 0x10, 0x10,
    0x0a, 0x0a, 0x0a, 0x0c, 0x0c, 0x0c, 0x10, 0x10, 0x10,
};

unsigned int RallyData_GetSelectionBits10To11(void);
unsigned int RallyData_GetSelectionBits12To13(void);
unsigned char RallyData_GetSelectionFlag26(void);
int RallyData_IsChampionshipFinalStage(void);

#define STAGE_SCORE_SCALE(variant)                                                                  \
    if ((BYTE)RallyData_GetFlag24()) {                                                              \
        if ((BYTE)RallyData_GetSelectionFlag26())                                                         \
            return g_rallyScoreScale[((BYTE)RallyData_GetSelectionBits10To11() * 3 + (BYTE)RallyData_GetSelectionBits12To13()) * 3 + \
                                     (BYTE)CGameInfo::GetConfiguredDifficulty()] * 100;                        \
        return g_rallyScoreScale[((BYTE)RallyData_GetSelectionBits10To11() * 3 + (BYTE)RallyData_GetSelectionBits12To13()) * 3] * 100; \
    }                                                                                               \
    if ((BYTE)RallyData_IsChampionshipFinalStage())                                                                       \
        return 2000;                                                                                \
    return g_stageScoreScale[((g_selectedRallyData & 0x1f) * 12 + ((g_selectedRallyData >> 5) & 0x1f)) * 2 + \
                             (variant)] * 100

// Score scale of the current stage (first column).
// FUNCTION: CMR2 0x00407650
unsigned short RallyData_GetPrimaryStageScoreScale(void)
{
    STAGE_SCORE_SCALE(0);
}

// Score scale of the current stage (second column).
// FUNCTION: CMR2 0x00407710
unsigned short RallyData_GetSecondaryStageScoreScale(void)
{
    STAGE_SCORE_SCALE(1);
}

// FUNCTION: CMR2 0x004077d0
int RallyData_GetIndexedStageScoreScale(int row, int column, int variant)
{
    return (unsigned int)g_stageScoreScale[(column + row * 12) * 2 + variant] * 100;
}

// Stores a driver's car (driver record, or category record when allowed).
// FUNCTION: CMR2 0x00408760
void RallyData_SetDriverCarSelection(BYTE index, int value)
{
    unsigned int category;

    RallyData_ValidateIndex(index);
    if (CGameInfo::GetConfiguredGameMode() == 4) {
        CGameInfo::SetGameInfoField98(value);
        g_saveCarRecords[index].car = value;
        g_saveCarRecords[index].flags |= 2;
        return;
    }
    category = g_saveSlots[index].category;
    if (category != 0xf) {
        if ((g_saveProfiles[category].identifier.flags & 0x200000) != 0)
            CGameInfo::SetGameInfoField98(value);
        g_saveProfiles[category].car = value;
        g_saveDirty[category] = 1;
    }
}

// Car of a driver: from the driver record in championship mode, else from its category.
// FUNCTION: CMR2 0x00408800
int RallyData_GetDriverCarSelection(BYTE index)
{
    unsigned int category;

    RallyData_ValidateIndex(index);
    if (CGameInfo::GetConfiguredGameMode() == 4)
        return g_saveCarRecords[index].car;
    category = g_saveSlots[index].category;
    if (category != 0xf)
        return g_saveProfiles[category].car;
    return 0;
}

// FUNCTION: CMR2 0x00408860
BYTE *RallyData_GetDriverCategoryProfile(int index)
{
    unsigned int category;
    RallyData_ValidateIndex(index);
    category = g_saveSlots[index].category;
    if (category != 0xf)
        return (BYTE *)&g_saveProfiles[category];
    return NULL;
}

void RallyData_MarkDriverRecordDirty(int index);

// Stores a driver's 20-byte tyre choice (driver record or category record).
// FUNCTION: CMR2 0x004088a0
void RallyData_SetDriverCameraRecord(BYTE index, ChaseCameraSettings *pValues)
{
    unsigned int category;

    if (CGameInfo::GetConfiguredGameMode() == 4) {
        memcpy(&g_saveCarRecords[index].chaseCamera, pValues, sizeof(ChaseCameraSettings));
        g_saveCarRecords[index].flags |= 2;
        return;
    }
    category = g_saveSlots[index].category;
    if (category != 0xf) {
        memcpy(g_saveProfiles[category].chaseCamera, pValues, sizeof(ChaseCameraSettings));
        RallyData_MarkDriverRecordDirty(index);
    }
}

// FUNCTION: CMR2 0x00408930
ChaseCameraSettings *RallyData_GetDriverCameraRecord(BYTE index)
{
    unsigned int category;

    RallyData_ValidateIndex(index);
    if (CGameInfo::GetConfiguredGameMode() == 4)
        return &g_saveCarRecords[index].chaseCamera;
    category = g_saveSlots[index].category;
    if (category != 0xf)
        return (ChaseCameraSettings *)g_saveProfiles[category].chaseCamera;
    return NULL;
}

// Stores a driver's byte setting (driver record or category record).
// FUNCTION: CMR2 0x00408990
void RallyData_SetDriverHudFlags(BYTE index, BYTE *pValue)
{
    unsigned int category;

    RallyData_ValidateIndex(index);
    if (CGameInfo::GetConfiguredGameMode() == 4) {
        g_saveCarRecords[index].hudFlags = *pValue;
        return;
    }
    category = g_saveSlots[index].category;
    if (category != 0xf)
        g_saveProfiles[category].hudFlags = *pValue;
}

// Driver record of a car: its knockout entry, or its team's record.
// FUNCTION: CMR2 0x00408a00
BYTE *RallyData_GetDriverHudFlags(BYTE index)
{
    unsigned int category;

    RallyData_ValidateIndex(index);
    if (CGameInfo::GetConfiguredGameMode() == 4)
        return &g_saveCarRecords[index].hudFlags;
    category = g_saveSlots[index].category;
    if (category != 0xf)
        return &g_saveProfiles[category].hudFlags;
    return NULL;
}

// Tyre record of a driver (NULL for the ghost cars of the time trials).
// FUNCTION: CMR2 0x00408a60
TyreWear *RallyData_GetDriverTyreWear(BYTE index)
{
    unsigned int category;

    if (CGameInfo::GetGameModeOptionBit19() != 0 || CGameInfo::GetConfiguredGameMode() == 5 || CGameInfo::GetConfiguredGameMode() == 6 ||
        CGameInfo::GetConfiguredGameMode() == 7 || RallyDataStageIndex() == 10) {
        if ((int)index > (int)(CGameInfo::GetConfiguredPlayerCount() - 1))
            return NULL;
    }
    if (CGameInfo::GetConfiguredGameMode() == 4)
        return &g_saveCarRecords[index].wear;
    RallyData_ValidateIndex(index);
    category = g_saveSlots[index].category;
    if (category != 0xf)
        return (TyreWear *)g_saveProfiles[category].wear;
    return NULL;
}

// Category colour of a driver's car: hue (5 bits), shade (4 bits) and value
// byte, or 0x45 each when the driver has no category.
// FUNCTION: CMR2 0x00408b10
void RallyData_GetDriverDateOfBirth(int index, unsigned int *pHue, unsigned int *pShade, unsigned int *pValue)
{
    unsigned int category;

    RallyData_ValidateIndex(index);
    category = g_saveSlots[index].category;
    if (category != 0xf) {
    if (pHue != NULL)
        *pHue = ((g_saveProfiles[category].identifier.flags) >> 16) & 0x1f;
    if (pShade != NULL)
        *pShade = ((g_saveProfiles[category].identifier.flags) >> 8) & 0xf;
    if (pValue != NULL)
        *pValue = (g_saveProfiles[category].identifier.flags) & 0xff;
    } else {

        if (pHue != NULL)
            *pHue = 0x45;
        if (pShade != NULL)
            *pShade = 0x45;
        if (pValue != NULL)
            *pValue = 0x45;
        return;

    }
}

// Stores a driver's camera offsets (y/z of pPos), heading and value.
// FUNCTION: CMR2 0x00408bd0
void RallyData_SetDriverCameraOffsets(FixVector *pOffset, short pitch, int lag, int index)
{
    ChaseCameraSettings record;

    RallyData_ValidateIndex(index);
    record.offset.x = 0;
    record.pitch = pitch;
    record.offset.y = pOffset->y;
    record.offset.z = pOffset->z;
    record.lag = lag;
    RallyData_SetDriverCameraRecord(index, &record);
}

// Reads a driver's stored position, heading and value.
// FUNCTION: CMR2 0x00408c20
void RallyData_GetDriverCameraOffsets(FixVector *pOffset, short *pPitch, int *pLag, int index)
{
    ChaseCameraSettings *p;

    RallyData_ValidateIndex(index);
    p = RallyData_GetDriverCameraRecord(index);
    pOffset->x = p->offset.x;
    pOffset->y = p->offset.y;
    pOffset->z = p->offset.z;
    *pPitch = RallyData_GetDriverCameraRecord(index)->pitch;
    *pLag = RallyData_GetDriverCameraRecord(index)->lag;
}

// FUNCTION: CMR2 0x00408c70
ProfileEventResults *RallyData_GetDriverEventResults(int index)
{
    unsigned int category;
    RallyData_ValidateIndex(index);
    category = g_saveSlots[index].category;
    if (category != 0xf)
        return (ProfileEventResults *)g_saveProfiles[category].eventResults;
    return NULL;
}

// Marks the tyre record of a driver as changed.
// FUNCTION: CMR2 0x00408d00
void RallyData_MarkDriverRecordDirty(int index)
{
    unsigned int category;

    RallyData_ValidateIndex(index);
    if (CGameInfo::GetConfiguredGameMode() == 4) {
        g_saveCarRecords[index].flags |= 2;
        return;
    }
    category = g_saveSlots[index].category;
    if (category != 0xf)
        g_saveDirty[category] = 1;
}

// FUNCTION: CMR2 0x00408d60
BYTE *RallyData_GetDriverEntryRecord(int index)
{
    return (BYTE *)&g_saveSlots[index];
}

// Track condition tables of the stage setup (4 groups of 9 entries): the value
// of a group is interpolated between the dry and the wet entry of the current
// rally by the fade of its track index.
// GLOBAL: CMR2 0x00516974
int g_unk0x00516974[9] = {
    0x50000, 0xf0000, 0x70000, -0xa0000, 0xf0000, 0x140000, 0xa0000, 0x70000, 0x70000
};
// GLOBAL: CMR2 0x00516998
int g_unk0x00516998[9] = {
    0xf0000, 0x140000, 0x140000, 0xf0000, 0xf0000, 0x140000, 0xf0000, 0xa0000, 0x140000
};
// GLOBAL: CMR2 0x005169bc
int g_unk0x005169bc[9] = {
    -0x50000, 0x50000, 0x0, -0xf0000, 0x50000, 0xa0000, 0x0, -0x50000, 0x50000
};
// GLOBAL: CMR2 0x005169e0
int g_unk0x005169e0[9] = {
    0xa0000, 0xa0000, 0xa0000, 0xf0000, 0xa0000, 0xa0000, 0xa0000, 0xc0000, 0xa0000
};

extern float g_oneOverRandMax;

// Draws the wet/dry share of every stage of the current rally: the flag of a
// stage is set when its track value passes 80 and the share of the range above
// 80 is stored per stage.
// FUNCTION: CMR2 0x0040d820
void RallyData_UpdateStageWetShares(void)
{
    int index;
    int *pFlag;
    int *pOther;
    int *pPairs;
    int i;
    int value;
    int lo;
    int hi;
    int count;
    int total;
    unsigned int counts[2];
    unsigned int *pCount;
    unsigned short track;

    index = 0;
    pOther = g_rallyPairingTables.otherFlags;
    pPairs = &g_rallyPairingTables.pairs[0][1];
    pFlag = g_rallyPairingTables.flags;
    do {
        *pFlag = 0;
        *pOther = 0;
        if (pPairs[-1] == 2 && pPairs[0] == 2) {
            if (((BYTE)RallyDataCountryIndex() == 6 &&
                 (pPairs == &g_rallyPairingTables.pairs[2][1] || pPairs == &g_rallyPairingTables.pairs[5][1] ||
                  pPairs == &g_rallyPairingTables.pairs[7][1])) ||
                ((BYTE)RallyDataCountryIndex() == 7 && (index / 4 == 0 || index / 4 == 2))) {
                track = (unsigned short)RallyData_GetIndexedStageScoreScale(g_selectedRallyData & 0x1f, index, 0);
                if (track < 0x834) {
                    value = (int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, rand()), g_oneOverRandMax), CGraphics::m_65536));
                    value = FixMulShift32(value, 0x640000);
                    if (value > 0x50)
                        *pFlag = 1;
                    lo = value - rand() % 0x14 - 1;
                    hi = rand() % 0x14 + value + 1;
                    if (lo < 0)
                        lo = 0;
                    else if (hi > 100)
                        hi = 100;
                    count = 0;
                    counts[1] = 0;
                    counts[0] = 0;
                    for (i = lo; i < hi; i++) {
                        count++;
                        if (i > 0x50)
                            counts[0]++;
                        else
                            counts[1]++;
                    }
                    total = count << 16;
                    pCount = counts;
                    i = 2;
                    do {
                        *pCount = FixMulShift32(0x640000, FixDiv(*pCount << 16, total));
                        pCount++;
                    } while (--i);
                    *pOther = (counts[0] != 0);
                }
            }
        }
        index++;
        pFlag++;
        pPairs += 2;
        pOther++;
    } while ((INT_PTR)pPairs < (INT_PTR)&g_rallyPairingTables.pairs[11][1]);
}

// Recomputes the grip byte of the four stages of a rally group, interpolating
// between the dry and the wet value of the group by the rally's track fade.
// match 70%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0040d9e0
void RallyData_InterpolateGroupGrip(int group)
{
    int index;
    int *pPairs;
    int k;
    int m;
    unsigned int n;
    int fade;
    int base;
    int track;

    index = group << 2;
    pPairs = &g_rallyPairingTables.pairs[index][1];
    k = 4;
    do {
        m = pPairs[-1];
        if (m >= 6)
            m += -3;
        if (pPairs[0] >= 6)
            m = m + -3 + pPairs[0];
        else
            m = m + pPairs[0];
        base = FixDiv(m << 16, 0xa0000);
        if (base < 0)
            base = 0;
        else if (base > 0x10000)
            base = 0x10000;
        fade = 0x10000 - base;
        m = g_unk0x00516974[(BYTE)RallyDataCountryIndex()] +
            FixMul(g_unk0x00516998[(BYTE)RallyDataCountryIndex()], fade);
        n = g_unk0x005169bc[(BYTE)RallyDataCountryIndex()] +
            FixMul(g_unk0x005169e0[(BYTE)RallyDataCountryIndex()], fade);
        track = (unsigned short)RallyData_GetIndexedStageScoreScale((BYTE)RallyDataCountryIndex(), index, 0);
        if (track <= 0x1f4 || track >= 0x834) {
            g_driverGroupTable.field_0x6d4[index] = (BYTE)(n >> 16);
        } else if (track < 0x4b0) {
            g_driverGroupTable.field_0x6d4[index] =
                (BYTE)((FixMul(FixDiv((track / 100 - 5) << 16, 0x70000), m - n) + n) >> 16);
        } else if (track > 0x640) {
            g_driverGroupTable.field_0x6d4[index] =
                (BYTE)((FixMul(FixDiv((track / 100 - 0x10) << 16, 0x50000), n - m) + m) >> 16);
        } else {
            g_driverGroupTable.field_0x6d4[index] = (BYTE)(m >> 16);
        }
        index++;
        pPairs += 2;
    } while (--k);
}

// FUNCTION: CMR2 0x0040df30
void RallyData_ClearDriverSkillFlags(void)
{
    BYTE zero = 0;
    int i = 0;
    do {
        BYTE *p = RallyData_GetDriverSkillRecord(i);
        i++;
        p[4] = zero;
        p[5] = zero;
        p[1] = zero;
        p[3] = zero;
        p[2] = zero;
        p[0] = zero;
        p[6] = zero;
    } while (i < 4);
}

// Grid row of the driver for the given position flag.
// FUNCTION: CMR2 0x00407150
BYTE RallyData_GetDriverGridRow(BYTE param1, char param2)
{
    if (param2 == 0)
        return (bool)(param1 % 2) + 4;
    if (param2 == 1)
        return (bool)(param1 % 2) + 8;
    return (bool)(param1 % 2) + 10;
}

BYTE RallyData_GetDriverGridRow(BYTE param1, char param2);

// Stage group of a mode: in network games the last available one.
// FUNCTION: CMR2 0x004071c0
BYTE RallyData_GetModeStageGroup(BYTE flags, char mode)
{
    int result = 0;
    int count;
    int i;

    if (CGameInfo::GetGameModeOptionBit19()) {
        count = RallyData_GetDriverGridRow(flags, 2);
        for (i = 0; i < count; i++) {
            if ((g_unk0x0052ea68[i] & 1) != 0 && ((g_unk0x0052ea68[i] & 2) == 0 || CGameInfo::IsRecordFlagSet(0xd)) &&
                (g_unk0x0052ea68[i] & 4) == 0)
                result = i;
        }
        return result;
    }
    if (((int)(BYTE)flags % 2) != 0)
        return 10;
    switch ((BYTE)mode) {
    case 0: return 3;
    case 1: return 7;
    case 2: return 9;
    default: return 0;
    }
}

// Whether the championship has just finished its last rally (rally 8, stage 11).
// FUNCTION: CMR2 0x00407270
int RallyData_IsChampionshipFinalStage(void)
{
    BYTE mode;
    BYTE rallyClass;

    mode = CGameInfo::GetConfiguredGameMode();
    rallyClass = CGameInfo::GetConfiguredDifficulty();
    RallyData_GetDriverGridRow(g_selectedRallyData & 0x1f, rallyClass);
    if (mode == 0 && (g_selectedRallyData & 0x1f) == 8 && (g_selectedRallyData & 0x3e0) == 0x160)
        return 1;
    return 0;
}

void RallyData_UpdateFlags(void);

// Advances g_selectedRallyData to the next stage (bits 5-9) and, at the end of
// a rally, to the next rally (bits 0-4). Returns 0 when the championship is over.
// FUNCTION: CMR2 0x004072d0
BYTE RallyData_AdvanceSelectedStage(void)
{
    BYTE mode;
    BYTE count;
    unsigned int stage;
    int i;

    mode = CGameInfo::GetConfiguredGameMode();
    count = CGameInfo::GetConfiguredDifficulty();
    if (CGameInfo::GetGameModeOptionBit19()) {
        count = RallyData_GetDriverGridRow(g_selectedRallyData & 0x1f, 2);
        g_selectedRallyData ^= (((g_selectedRallyData & 0xffffffe0) + 0x20) ^ g_selectedRallyData) & 0x3e0;
        for (stage = (g_selectedRallyData >> 5) & 0x1f; stage < count; stage = (g_selectedRallyData >> 5) & 0x1f) {
            if ((g_unk0x0052ea68[stage] & 1) &&
                (!(g_unk0x0052ea68[stage] & 2) || CGameInfo::IsRecordFlagSet(0xd)) &&
                !(g_unk0x0052ea68[(g_selectedRallyData >> 5) & 0x1f] & 4))
                goto done;
            g_selectedRallyData ^= (((g_selectedRallyData & 0xffffffe0) + 0x20) ^ g_selectedRallyData) & 0x3e0;
        }
        return 0;
    }
    count = RallyData_GetDriverGridRow(g_selectedRallyData & 0x1f, count);
    if (mode != 0 && mode != 1)
        return 0;
    g_selectedRallyData ^= (((g_selectedRallyData & 0xffffffe0) + 0x20) ^ g_selectedRallyData) & 0x3e0;
    stage = (g_selectedRallyData >> 5) & 0x1f;
    if (stage != count && (g_selectedRallyData & 0x3e0) <= 0x140) {
        if (stage + 1 == count && (g_selectedRallyData & 1)) {
            g_selectedRallyData = (g_selectedRallyData & 0xfffffd5f) | 0x140;
            RallyData_UpdateFlags();
            return 1;
        }
        goto done;
    }
    if (mode == 1)
        return 0;
    g_selectedRallyData ^= ((g_selectedRallyData + 1) ^ g_selectedRallyData) & 0x1f;
    if ((g_selectedRallyData & 0x1f) == 8) {
        for (i = 0; i < CGameInfo::GetConfiguredPlayerCount(); i++) {
            if (RallyTiming_GetStagePositionOfDriver(StageTiming_GetDriverSlot(i)) < 3)
                goto done;
        }
        return 0;
    }
    if ((g_selectedRallyData & 0x1f) == 9)
        return 0;
    g_selectedRallyData &= 0xfffffc1f;
done:
    RallyData_UpdateFlags();
    return 1;
}


// Returns the record of the category that the index belongs to, or NULL when
// the record has no category.
// FUNCTION: CMR2 0x00408470
void *RallyData_GetAssignedCategoryRecord(unsigned int param1)
{
    unsigned int category;

    category = g_saveSlots[(param1 & 0xff)].category;
    RallyData_ValidateIndex(param1 & 0xff);
    if (category != 0xf)
        return (BYTE *)g_saveProfiles[category].name;
    return NULL;
}

// Returns 1 when the record can be used (and 0 when it is full), unless the
// record is one of the "always usable" ones whose flag lacks the category.
// FUNCTION: CMR2 0x004085a0
BYTE RallyData_IsDriverRecordUsable(BYTE param1)
{
    RallyData_ValidateIndex(param1);
    if ((*(unsigned int *)&g_saveSlots[param1] & 0x3c0000) != 0x3c0000) {
        if (CGameInfo::GetConfiguredGameMode() == 4)
            return 1;
        return (*(unsigned int *)&g_saveSlots[param1] >> 0x1a) & 1;
    }
    return 0;
}

// Stores a driver's 5/6-bit setting in the driver or category record.
// match 82%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00408600
void RallyData_SetDriverCategoryOption(BYTE index, BYTE value)
{
    unsigned int record;
    unsigned int category;

    RallyData_ValidateIndex(index);
    if (CGameInfo::GetConfiguredGameMode() == 4) {
        g_saveCarRecords[index].options = value;
        g_saveCarRecords[index].flags |= 2;
        return;
    }
    record = *(unsigned int *)&g_saveSlots[index];
    category = (record >> 0x12) & 0xf;
    if (category != 0xf) {
        g_saveDirty[category] = 1;
        *(unsigned int *)&g_saveSlots[index] = ((value ^ record) & 0x3f) ^ record;
        g_saveProfiles[category].options ^=
            (g_saveProfiles[category].options ^ value) & 0x1f;
    }
}

// FUNCTION: CMR2 0x004086b0
BYTE RallyData_GetDriverRecordSelectionValue(BYTE index)
{
    if (CGameInfo::GetConfiguredGameMode() == 4)
        return (BYTE)g_saveCarRecords[index].options;
    return *(int *)&g_saveSlots[index] & 0x3f;
}

// Returns bit 0 of the record flag, or bit 5 of the category flag when the
// record belongs to a category.
// FUNCTION: CMR2 0x004086f0
BYTE RallyData_GetDriverAutomaticGearbox(BYTE param1)
{
    unsigned int category;
    BYTE flag;

    param1 = param1 & 0xff;
    RallyData_ValidateIndex(param1);
    if (CGameInfo::GetConfiguredGameMode() == 4) {
        flag = g_saveCarRecords[param1].flags;
        flag &= 1;
        return flag;
    }
    category = g_saveSlots[param1].category;
    if (category != 0xf)
        return (g_saveProfiles[category].options >> 5) & 1;
    return 1;
}

// GLOBAL: CMR2 0x00536ecc
int g_unk0x00536ecc;
// GLOBAL: CMR2 0x00537060
int g_unk0x00537060;

// Plays the looping sound of the stage that is not the one already playing.
// FUNCTION: CMR2 0x00411ab0
void RallyData_PlayAlternateStageLoopSound(BYTE param1, int param2)
{
    if (CGameInfo::GetGameInfoSessionFlag() != 0)
        return;
    if (Race_GetPlayerRecordField4(param1) != 0)
        return;
    if (param2 != 0) {
        Sound_PlaySampleWithParameters(g_unk0x00536ecc, Race_GetCoDriverCallState() / 2, 0x5622, 0, 0, 0);
        return;
    }
    Sound_PlaySampleWithParameters(g_unk0x00537060, Race_GetCoDriverCallState() / 2, 0x2b11, 0, 0, 0);
}

int Stage_GetSplitPositionCount(void);
int Stage_GetSplitPositionFixed(int index);

// Finds the checkpoint before distance (whole units, plus percent/100) and
// the 16.16 fraction of the way to the next one.
// FUNCTION: CMR2 0x00411e40
void RallyData_FindCheckpointAndFractionAtDistance(int *pOut, int distance, int percent)
{
    int found = -1;
    int i;
    int next;
    int current;
    int adjustment = (percent << 16) / 100;

    for (i = Stage_GetSplitPositionCount() - 1; i >= 0; i--) {
        if ((Stage_GetSplitPositionFixed(i) >> 16) <= distance) {
            found = i;
            i = 0;
        }
    }
    if (found != -1 && found != Stage_GetSplitPositionCount() - 1) {
        next = Stage_GetSplitPositionFixed(found + 1);
        current = Stage_GetSplitPositionFixed(found);
        int fraction = FixDiv(distance * 0x10000 - current + adjustment, next - current);
        pOut[0] = found;
        pOut[1] = fraction;
        return;
    }
    pOut[0] = Stage_GetSplitPositionCount() - 1;
    pOut[1] = 0;
}

// Copies the name of the driver with the given id (a human player's name, or
// the AI driver name) into CFrontend::m_stringDest.
// FUNCTION: CMR2 0x004125a0
void RallyData_CopyDriverDisplayName(int id)
{
    char *pName;
    int i;

    for (i = 0; i < CGameInfo::GetConfiguredPlayerCount(); i++) {
        if (id == StageTiming_GetDriverSlot(i)) {
            pName = (char *)RallyData_GetRecord(i);
            goto done;
        }
    }
    pName = CAIHelper::GetNameForID(id);
done:
    sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue, pName);
}

extern char g_stageNumberFormat[];
extern char g_nameSpaceFormat[];
extern char g_standingsRowFormat[];
extern double g_minus65536;
extern BYTE g_gapTextColour[4];

// GLOBAL: CMR2 0x005170bc
unsigned int g_stageHudPanelColour = 0x96dcbebc;
// GLOBAL: CMR2 0x005170c4
unsigned int g_stageHudTextColour = 0xffffffff;
// GLOBAL: CMR2 0x005170e8
unsigned int g_unk0x005170e8 = 0xffffffff;
// GLOBAL: CMR2 0x00517dc0
char g_positionFormat[] = "%d / %d  %d";
// GLOBAL: CMR2 0x00517dcc
char g_stageSlashFormat[] = " / ";

BYTE *StageObject_GetPlayerViewRectangle(int view);
int StageTiming_GetFinishOrderSecondaryEntry(int index);
int StageTiming_GetClockTime(void);
int RallyData_GetActiveCarRaceRecordField0(Car *p);
int RallyData_GetRouteAvailabilityState(void);
extern int g_unk0x00536c88[2];
extern int g_unk0x00536c00[2];
extern int g_finishLeaderMessage[2];
void StageUI_NoOpMapEvent(int, short *);
BYTE StageTiming_GetCheckpointField1A(int index);
extern BYTE g_gapTextColour[4];
BYTE StageTiming_GetPendingDriverByteA7(void);

// Draws the time-gap text of a car while the stage is running, for the two
// rally modes that the input mode selects.
// FUNCTION: CMR2 0x00412970
void RallyData_DrawRunningTimeGap(int car, short *pRect)
{
    if (StageTiming_GetPendingDriverByteA7() != 0) {
        switch (RallyData_GetSetupModeBits()) {
        case 1:
            if (StageTiming_GetFinishOrderSecondaryEntry(car) == 0)
                Race_DrawPlayerMessage(CFrontend::GetTextString(0xbe), (int *)g_gapTextColour, car, 0, -1,
                             -1);
            break;
        case 2:
            if (StageTiming_GetFinishOrderSecondaryEntry(car) == 0)
                Race_DrawPlayerMessage(CFrontend::GetTextString(0xbe), (int *)g_gapTextColour, car, 0, -1,
                             -1);
            break;
        }
    }
}

// Temporary scaffolding for the RallyData helpers RallyData_DrawAndHandleCarHUD dispatches to:
// every one is a real function of the reference exe (empty body, stdcall
// argument count from its ret N) so that its call sites are in place. Delete
// each entry as its implementation lands (0x415f50 belongs to StageUI.cpp and
// 0x417e70 to Race.cpp).
void RallyData_DrawSplitStandingsPanel(int car, short *pRect);
void StageUI_DrawHudRouteMap(int car, short *pRect);
void RallyData_DrawStageTimePanel(int car, short *pRect);
void RallyData_DrawTargetTimeGap(int car, short *pRect);

char *NetPlayers_GetPlayerName(int index);
int NetPlayers_GetPlayerID(int index);
BYTE *NetPlayers_GetPlayerNameColour(int id);
DWORD Network_GetLocalPlayerID(void);
extern unsigned int g_stageResultTextColour;
extern unsigned int g_stageResultPanelColour;

// GLOBAL: CMR2 0x005170d8
unsigned int g_unk0x005170d8 = 0x96ff9265;
// GLOBAL: CMR2 0x00517df0
char g_strOneDigit[] = "%1d";

// Network race standings panel: one row per player in result order, with the
// local player highlighted, the player's colour square and the position.
// match 68%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00415480
void RallyData_DrawNetworkStandingsPanel(short *pRect)
{
    short panel[4];
    int i;
    int textY;
    int id;
    short square[4];
    BYTE *pColour;
    char number[4];
    char name[256];

    panel[1] = (short)((int)(g_pGraphics->resY << 12) >> 16) + pRect[1];
    textY = ((int)(g_pGraphics->resY * 0x1b32) >> 16) + pRect[1] - 2;
    panel[0] = (short)((int)(g_pGraphics->resX * 0xc00) >> 16) + pRect[0];
    panel[2] = (short)((int)(g_pGraphics->resX * 0x47ae) >> 16);
    panel[3] = (short)((int)(g_pGraphics->resY * 0xccc) >> 16) - 2;
    for (i = 0; i < 8; i++) {
        id = NetPlayers_GetResultPlayerIndex(i);
        if (id == -1)
            continue;
        if (id == -2) {
            sprintf(name, CRegKey::m_regKeyPathFormatValue, (char *)RallyData_GetRecord(0));
            Sprite_FillRect(&g_pGraphics->field309_0x150, panel, (BYTE *)&g_unk0x005170d8, 2);
            pColour = NetPlayers_GetPlayerNameColour(Network_GetLocalPlayerID());
        } else {
            char *pName = NetPlayers_GetPlayerName(id);
            if (pName != NULL)
                sprintf(name, CRegKey::m_regKeyPathFormatValue, pName);
            else
                sprintf(name, "%s", CMain::m_logFileBlankLine);
            Sprite_FillRect(&g_pGraphics->field309_0x150, panel, (BYTE *)&g_stageResultPanelColour, 2);
            pColour = NetPlayers_GetPlayerNameColour(NetPlayers_GetPlayerID(id));
        }
        square[2] = (short)((int)(g_pGraphics->resX * 10) / 640);
        square[3] = (short)((int)(g_pGraphics->resY * 10) / 480);
        square[0] = panel[2] - (short)((int)(g_pGraphics->resX * 10) / 640) + panel[0] - square[2] / 2;
        square[1] = panel[3] / 2 + panel[1] - square[3] / 2;
        Sprite_FillRect(&g_pGraphics->field309_0x150, square, pColour, 2);
        sprintf(number, g_strOneDigit, i + 1);
        Font_DrawText(0, number, (short)(((int)(g_pGraphics->resX * 0xf95) >> 16) + pRect[0]), textY,
                      (int *)&g_stageResultTextColour, 0x22);
        Font_DrawText(0, name, (short)(((int)(g_pGraphics->resX * 0x132a) >> 16) + pRect[0]), textY,
                      (int *)&g_stageResultTextColour, 0x21);
        panel[1] += panel[3];
        textY += panel[3];
    }
}

// Draws a car's on-stage HUD: the position/points line plus the per-car
// action icons, then dispatches the current input to the matching handler.
// FUNCTION: CMR2 0x004125f0
void RallyData_DrawAndHandleCarHUD(int car, short *pRect)
{
    BYTE colour[4] = { 0xff, 0xff, 0xff, 0xff };
    short rect[4];
    int *pView;

    if ((BYTE)CGameInfo::GetGraphicsOptionBit29()) {
        Font_DrawText(0, CInput::FormatString(g_positionFormat,
                          RallyData_GetActiveCarRaceRecordField0(Car_Get(car)), RallyData_GetRouteAvailabilityState(),
                          StageTiming_GetClockTime() / 100),
                      pRect[0] + 10, pRect[3] + pRect[1] - 10, (int *)colour, 0x21);
    }
    // PORT: each panel is an element of the HUD (see Hud_Element).
    Hud_Element();
    pView = (int *)StageObject_GetPlayerViewRectangle(car);
    *(int *)&rect[0] = pView[0];
    *(int *)&rect[2] = pView[1];
    if ((BYTE)RallyData_GetSetupFlag11()) {
        RallyData_DrawRunningTimeGap(car, pRect);
    } else if (RallyData_IsHeadToHeadRaceMode()) {
        if (((BYTE)RallyData_GetFlag24() || (BYTE)RallyData_GetFlag25()) && StageTiming_GetCheckpointField1A(car) &&
            StageTiming_GetFinishOrderSecondaryEntry(car) == 0) {
            Race_DrawPlayerMessage(CFrontend::GetTextString(0xbe), (int *)g_gapTextColour, car, 0, -1, -1);
        }
    }
    Hud_Element();
    if (g_unk0x00536c88[car] != 0) {
        sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(0xbd));
        Race_DrawPlayerMessage(CFrontend::m_stringDest, (int *)&g_unk0x005170e8, car, 1, -1, -1);
    }
    if (g_unk0x00536c00[car] != 0) {
        sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(0x30));
        Race_DrawPlayerMessage(CFrontend::m_stringDest, (int *)&g_unk0x005170e8, car, 1, -1, -1);
    }
    if (g_finishStageRecordMessage[car] != 0) {
        sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(0x6a));
        Race_DrawPlayerMessage(CFrontend::m_stringDest, (int *)&g_unk0x005170e8, car, 1, -1, -1);
    }
    if (g_finishLeaderMessage[car] != 0) {
        sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(0x83));
        Race_DrawPlayerMessage(CFrontend::m_stringDest, (int *)&g_unk0x005170e8, car, 1, -1, -1);
    }
    Hud_Element();
    if ((*RallyData_GetDriverHudFlags(StageUI_GetRaceEndEventCount() + (char)car) & 4) != 0) {
        if (CGameInfo::GetConfiguredGameMode() == 7 || CGameInfo::GetConfiguredGameMode() == 5 ||
            CGameInfo::GetConfiguredGameMode() == 6 || CGameInfo::GetConfiguredGameMode() == 3 ||
            CGameInfo::GetConfiguredGameMode() == 0xa || CGameInfo::GetConfiguredGameMode() == 0xc) {
            if (RallyData_IsHeadToHeadRaceMode() == 0 || CGameInfo::IsSplitBarEnabled() != 0) {
                RallyData_DrawStageResultRows(car, rect);
            }
        } else if ((BYTE)RallyData_GetSelectionFlag27() && !CGameInfo::GetGameModeOptionBit19()) {
            StageUI_NoOpMapEvent(car, rect);
        } else if (CGameInfo::GetConfiguredGameMode() == 2 || CGameInfo::GetConfiguredGameMode() == 0 ||
                   CGameInfo::GetConfiguredGameMode() == 1) {
            RallyData_DrawSplitStandingsPanel(car, rect);
        } else if (CGameInfo::GetConfiguredGameMode() == 8 || CGameInfo::GetConfiguredGameMode() == 9 ||
                   CGameInfo::GetConfiguredGameMode() == 0xb) {
            RallyData_DrawNetworkStandingsPanel(rect);
        }
    }
    Hud_Element();
    if ((*RallyData_GetDriverHudFlags(StageUI_GetRaceEndEventCount() + (char)car) & 8) != 0) {
        if (CGameInfo::GetConfiguredGameMode() == 5 || CGameInfo::GetConfiguredGameMode() == 6 ||
            CGameInfo::GetConfiguredGameMode() == 7 || CGameInfo::GetConfiguredGameMode() == 0xb ||
            CGameInfo::GetConfiguredGameMode() == 0xc || (BYTE)RallyData_GetFlag25()) {
            RallyData_DrawCarStageGapPanel(car, rect);
        } else {
            RallyData_DrawStageTimePanel(car, rect);
        }
    }
    Hud_Element();
    if ((*RallyData_GetDriverHudFlags(StageUI_GetRaceEndEventCount() + (char)car) & 0x20) != 0) {
        if (CGameInfo::GetConfiguredGameMode() == 5 || CGameInfo::GetConfiguredGameMode() == 6 ||
            CGameInfo::GetConfiguredGameMode() == 7 || CGameInfo::GetConfiguredGameMode() == 0xb ||
            CGameInfo::GetConfiguredGameMode() == 0xc || (BYTE)RallyData_GetFlag25()) {
            StageUI_DrawHudRouteMap(car, rect);
        }
        Hud_Element();
        RallyData_DrawTargetTimeGap(car, rect);
    }
}

// Draws a car's stage gap, stage number and position in the on-stage HUD.
// match 55%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004129d0
void RallyData_DrawCarStageGapPanel(int car, short *pRect)
{
    short rect[4];
    int xShift = 0;
    int yShift = 0;
    int marginX = (int)g_pGraphics->resX * 3 / 640;
    int wideMarginX = (int)g_pGraphics->resX * 44 / 640;
    int topMarginY = (int)g_pGraphics->resY * 3 / 480;
    int mode;
    int stage;
    int count;
    int x;
    int y;
    int i;
    int pixelFixed;
    int dimensionFixed;

    if (RallyData_IsHeadToHeadRaceMode()) {
        if (CGameInfo::IsSplitBarEnabled()) {
            if (car == 1)
                yShift = (int)g_pGraphics->resY / 2;
        } else if (car == 0) {
            xShift = -((int)g_pGraphics->resX / 2);
        }
    }
    rect[0] = (short)(((int)g_pGraphics->resX * 0xab9b >> 16) + xShift);
    rect[1] = (short)(((int)g_pGraphics->resY << 12 >> 16) + yShift);
    rect[2] = (short)(((int)g_pGraphics->resX * 0x4865 >> 16) + 1);
    rect[3] = (short)((int)g_pGraphics->resY * 53 / 480);

    if (StageTiming_IsClockOverlayActive()) {
        if (CGameInfo::GetGameLanguage() == 1 || CGameInfo::GetGameLanguage() == 3 ||
            CGameInfo::GetGameLanguage() == 2)
            rect[3] += (short)((int)g_pGraphics->resY * 24 / 480 * 2);
        else
            rect[3] += (short)((int)g_pGraphics->resY * 24 / 480);
    }
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, (BYTE *)&g_stageHudPanelColour, 2);
    if (StageTiming_IsClockOverlayActive()) {
        if (CGameInfo::GetGameLanguage() == 1 || CGameInfo::GetGameLanguage() == 3 ||
            CGameInfo::GetGameLanguage() == 2)
            rect[3] += (short)((int)g_pGraphics->resY * -24 / 480 * 2);
        else
            rect[3] += (short)((int)g_pGraphics->resY * -24 / 480);
    }

    pixelFixed = (int)FTOI(__int64, X87S_MUL((double)(rect[3] + rect[1] - topMarginY + 1), CGraphics::m_65536));
    dimensionFixed = (int)FTOI(__int64, X87S_MUL((double)g_pGraphics->resY, CGraphics::m_65536));
    y = FixDiv(pixelFixed, dimensionFixed);
    pixelFixed = (int)FTOI(__int64, X87S_MUL((double)(rect[0] + 16 + marginX), CGraphics::m_65536));
    dimensionFixed = (int)FTOI(__int64, X87S_MUL((double)g_pGraphics->resX, CGraphics::m_65536));
    x = FixDiv(pixelFixed, dimensionFixed);
    // The running time UpdateCarSplitBar stores at 0x536e0c (times[0]).
    FormatGapToLeader(g_stageSplitData[car].times[0], 4, 4, x, y,
                      &g_stageHudTextColour, 0x21, NULL, 0);

    stage = StageTiming_GetCheckpointField2(car);
    count = RallyData_GetSelectionBits16To19() & 0xff;
    mode = 0;
    if (RallyData_GetSetupFlag11()) {
        int uiState = RallyData_GetSetupModeBits();
        if (uiState == 1)
            mode = 1;
        else if (uiState == 2)
            mode = 2;
    }

    if (mode == 2) {
        sprintf(CFrontend::m_stringDest, g_nameSpaceFormat, CFrontend::GetTextString(0xbc));
        y = rect[1] + topMarginY - (int)g_pGraphics->resY * 2 / 480;
        x = rect[0] + marginX;
        Font_DrawText(0, CFrontend::m_stringDest, x, y, (int *)&g_stageHudTextColour, 9);
        x += Font_GetTextWidth(0, (BYTE *)CFrontend::m_stringDest);
        sprintf(CFrontend::m_stringDest, g_stageNumberFormat, StageTiming_GetPendingDriverEntry(car));
        Font_DrawText(5, CFrontend::m_stringDest, x, rect[1] + topMarginY,
                      (int *)&g_stageHudTextColour, 9);
        x += Font_GetTextWidth(5, (BYTE *)CFrontend::m_stringDest);
        sprintf(CFrontend::m_stringDest, "%s", g_stageSlashFormat);
        Font_DrawText(0, CFrontend::m_stringDest, x, y, (int *)&g_stageHudTextColour, 9);
        x += Font_GetTextWidth(0, (BYTE *)CFrontend::m_stringDest);
        sprintf(CFrontend::m_stringDest, g_stageNumberFormat, RallyData_GetSetupHighNibble());
        Font_DrawText(5, CFrontend::m_stringDest, x, rect[1] + topMarginY,
                      (int *)&g_stageHudTextColour, 9);
    } else {
        if (CGameInfo::GetConfiguredGameMode() == 7 || CGameInfo::GetConfiguredGameMode() == 12 || mode == 1) {
            if (stage < 0)
                stage = 0;
            sprintf(CFrontend::m_stringDest, g_nameSpaceFormat, CFrontend::GetTextString(0x2f));
            x = rect[0] + marginX;
            y = rect[1] + topMarginY - (int)g_pGraphics->resY * 2 / 480;
            Font_DrawText(0, CFrontend::m_stringDest, x, y, (int *)&g_stageHudTextColour, 9);
            x += Font_GetTextWidth(0, (BYTE *)CFrontend::m_stringDest);
            sprintf(CFrontend::m_stringDest, g_stageNumberFormat, (stage + 1) % 100);
            Font_DrawText(5, CFrontend::m_stringDest, x, rect[1] + topMarginY,
                          (int *)&g_stageHudTextColour, 9);
        } else if (!(BYTE)RallyData_GetSelectionFlag27()) {
            if (stage < 0)
                stage = 0;
            if (stage < count)
                stage++;
            sprintf(CFrontend::m_stringDest, g_nameSpaceFormat, CFrontend::GetTextString(0x2f), stage, count);
            x = rect[0] + marginX;
            y = rect[1] + topMarginY - (int)g_pGraphics->resY * 2 / 480;
            Font_DrawText(0, CFrontend::m_stringDest, x, y, (int *)&g_stageHudTextColour, 9);
            x += Font_GetTextWidth(0, (BYTE *)CFrontend::m_stringDest);
            sprintf(CFrontend::m_stringDest, g_stageNumberFormat, stage % 100);
            Font_DrawText(5, CFrontend::m_stringDest, x, rect[1] + topMarginY,
                          (int *)&g_stageHudTextColour, 9);
            x += Font_GetTextWidth(5, (BYTE *)CFrontend::m_stringDest);
            sprintf(CFrontend::m_stringDest, "%s", g_stageSlashFormat);
            Font_DrawText(0, CFrontend::m_stringDest, x, y, (int *)&g_stageHudTextColour, 9);
            x += Font_GetTextWidth(0, (BYTE *)CFrontend::m_stringDest);
            sprintf(CFrontend::m_stringDest, g_stageNumberFormat, count);
            Font_DrawText(5, CFrontend::m_stringDest, x, rect[1] + topMarginY,
                          (int *)&g_stageHudTextColour, 9);
        }
    }

    if (CGameInfo::GetConfiguredGameMode() != 7 && CGameInfo::GetConfiguredGameMode() != 12 &&
        (RallyDataStageIndex() != 10 || CGameInfo::GetConfiguredGameMode() != 10)) {
        sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(0x99));
        x = rect[0] + rect[2] - wideMarginX;
        y = rect[1] + topMarginY - (int)g_pGraphics->resY * 2 / 480;
        Font_DrawText(0, CFrontend::m_stringDest, x, y, (int *)&g_stageHudTextColour, 12);
        if (CGameInfo::GetConfiguredGameMode() == 11 ||
            (CGameInfo::GetGameModeOptionBit19() && RallyData_GetFlag25())) {
            for (i = 0; i < 8; i++) {
                if (NetPlayers_GetResultPlayerIndex(i) == -2)
                    sprintf(CFrontend::m_stringDest, g_stageNumberFormat, i + 1);
            }
        } else {
            sprintf(CFrontend::m_stringDest, g_stageNumberFormat, StageTiming_GetCarTimingByte81(car) + 1);
        }
        Font_DrawText(6, CFrontend::m_stringDest, rect[0] + rect[2] - marginX,
                      rect[1] + topMarginY, (int *)&g_stageHudTextColour, 12);
    }
}

BYTE StageTiming_GetPendingDriverByteA7(void);
BYTE StageTiming_GetPendingDriverByteA6(void);
int StageTiming_GetPendingDriverState(void);
int StageTiming_GetPendingDriverCount(void);
int StageTiming_GetPendingDriverEntry(int index);
int StageTiming_GetCarTimingByte81(int car);
BYTE StageTiming_GetCheckpointField19(int index);
void Sound_SetPan(unsigned int handle, int pan);
unsigned int RallyData_GetSetupModeBits(void);
unsigned int RallyData_GetSetupLowNibble(void);

// GLOBAL: CMR2 0x00536c08
BYTE g_unk0x00536c08[4];
// GLOBAL: CMR2 0x00536c88
int g_unk0x00536c88[2];
// GLOBAL: CMR2 0x00536cac
BYTE g_unk0x00536cac[4];
// GLOBAL: CMR2 0x00536cb0
int g_unk0x00536cb0;
// GLOBAL: CMR2 0x00536fec
int g_unk0x00536fec;
// GLOBAL: CMR2 0x00537050
int g_stageFinishBeepPlayed;

// Per-frame stage sounds: the start beep once the stage is over, and the
// countdown beeps (rising in pitch over the last five seconds) while racing.
// FUNCTION: CMR2 0x004118b0
void RallyData_UpdateCountdownAndFinishSounds(int car)
{
    int elapsed;
    int remaining;
    int handle;

    if (StageTiming_GetPendingDriverByteA7()) {
        if (g_stageFinishBeepPlayed == 0) {
            g_stageFinishBeepPlayed = 1;
            if (RallyData_GetSetupModeBits() == 1)
                Sound_PlaySampleWithParameters(g_unk0x00536cb0 + 2, Race_GetCoDriverCallState(), 0xac44, 0, 0, 0);
            else
                Sound_PlaySampleWithParameters(g_unk0x00536cb0 + 1, Race_GetCoDriverCallState(), 0xac44, 0, 0, 0);
        }
        return;
    }
    switch (RallyData_GetSetupModeBits()) {
    case 1:
        if (StageTiming_GetCheckpointField19(car)) {
            if (StageTiming_GetCarTimingByte81(car) == 0)
                Sound_PlaySampleWithParameters(g_unk0x00536cb0 + 1, Race_GetCoDriverCallState(), 0xac44, 0, 0, 0);
            else
                RallyData_PlayAlternateStageLoopSound(car, 1);
            g_unk0x00536fec = RallyData_GetSetupLowNibble() - StageTiming_GetPendingDriverCount() / 100;
        }
        if (StageTiming_GetPendingDriverByteA6() && StageTiming_GetPendingDriverState() != car) {
            elapsed = StageTiming_GetPendingDriverCount() / 100;
            remaining = RallyData_GetSetupLowNibble() - elapsed;
            if (remaining < g_unk0x00536fec) {
                g_unk0x00536fec = remaining;
                if (remaining > 5)
                    elapsed = 0xac44;
                else
                    elapsed = (6 - remaining) * 0xac44 / 0x30 + 0xac44;
                if (remaining != 0) {
                    handle = Sound_PlaySampleWithParameters(g_unk0x00536cb0, Race_GetCoDriverCallState(), elapsed, 0, 0, 0);
                    Sound_SetPan(handle, elapsed);
                }
            }
        }
        break;
    case 2:
        if (StageTiming_GetCheckpointField19(car) && g_unk0x00536cac[car] != (BYTE)StageTiming_GetPendingDriverEntry(car)) {
            g_unk0x00536cac[car] = StageTiming_GetPendingDriverEntry(car);
            g_unk0x00536c88[car] = 1;
            g_unk0x00536c08[car] = 0x7d;
            Sound_PlaySampleWithParameters(g_unk0x00536cb0 + 1, Race_GetCoDriverCallState(), 0xac44, 0, 0, 0);
        }
        break;
    }
}

BYTE StageTiming_GetCheckpointField17(int index);
BYTE StageTiming_GetCheckpointField18(int index);
int StageTiming_GetCheckpointGroupIndex(int index);
int StageTiming_GetCheckpointSplitIndex(int index);
int StageTiming_GetCheckpointField14(int index);
int StageTiming_GetCarSplitTime(int car, int index);
void NetRace_SendStageTime(int index, int value);
int NetPlayers_IsNewLocalRecord(void);
unsigned int NetPlayers_GetPreviousBestTime(void);

// GLOBAL: CMR2 0x00536bfc
int g_splitTargetsLoaded;
// Per car: the HUD gap shows a blank time (no reference yet).
// GLOBAL: CMR2 0x00536c0c
int g_splitGapBlank[2];
// GLOBAL: CMR2 0x00536c14
int g_stageStartRecorded;
// Per car: frames the split gap stays on the HUD (75 after a split).
// GLOBAL: CMR2 0x00536c20
BYTE g_splitGapDisplayFrames[4];
// GLOBAL: CMR2 0x00536c40
int g_stageTargetTime;
// Split time colours, 0x28 entries per car.
// Storage and overlapping views are declared in StageSplitData.h.
// Reference split times, g_splitTargetTimes[0] doubles as a "no reference" flag; cleared as [1..12].
// g_splitTargetTimes is the referenceTimes member of the shared split block.
// GLOBAL: CMR2 0x00537064
int g_lastSplitGroup;
// GLOBAL: CMR2 0x00537068
int g_stageDefaultTargetTime;
// GLOBAL: CMR2 0x0051709c
unsigned int g_unk0x0051709c = 0xffa4fa2e;
// GLOBAL: CMR2 0x005170a0
unsigned int g_unk0x005170a0 = 0xff00008a;
// GLOBAL: CMR2 0x005170e0
unsigned int g_unk0x005170e0[2] = {0x00ffffff, 0x00ffffff};

// Records the time of the split the car just passed, colours it against the
// reference time and updates the car's position at that split.
// match 85%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00413330
void RallyData_RecordPassedSplit(int car)
{
    int split;
    int time;
    int delta;
    int reference;

    g_splitGapBlank[car] = 0;
    if (!StageTiming_GetCheckpointField17(car))
        return;
    if ((BYTE)RallyData_GetFlag25())
        split = StageTiming_GetCheckpointField14(car);
    else
        split = StageTiming_GetCheckpointSplitIndex(car);
    if (StageTiming_GetCheckpointField19(car)) {
        split = 1;
        g_stageSplitData[car].times[split + 1] = StageTiming_GetCarSplitTime(car, StageTiming_GetCheckpointGroupIndex(car));
        g_stageSplitData[car].targetTime = g_stageTargetTime;
        g_stageSplitData[car].lastSplitTime = StageTiming_GetCarSplitTime(car, StageTiming_GetCheckpointGroupIndex(car));
        g_splitTargetsLoaded = 0;
    } else if (split != 0) {
        g_stageSplitData[car].times[split + 1] = g_stageSplitData[car].times[0];
        g_stageSplitData[car].targetTime = g_splitTargetTimes[split + 1];
        g_stageSplitData[car].lastSplitTime = g_stageSplitData[car].times[0];
    }
    if (StageTiming_GetCheckpointField18(car))
        g_splitTargetsLoaded = 0;
    if (g_stageTargetTime != g_stageDefaultTargetTime) {
        time = g_stageSplitData[car].times[split + 1];
        if (g_splitTargetTimes[split + 1] - g_splitTargetTimes[split] <= time - g_stageSplitData[car].times[split])
            g_splitTimeColours[car * 0x28 + split] = g_unk0x005170a0;
        else
            g_splitTimeColours[car * 0x28 + split] = g_unk0x0051709c;
        if (split != 0) {
            RallyData_PlayAlternateStageLoopSound(car, (time < g_stageSplitData[car].targetTime) ? 1 : 0);
        }
    }
    g_stageSplitData[car].split = split;
    if (CGameInfo::GetConfiguredGameMode() != 5 && CGameInfo::GetConfiguredGameMode() != 6 && split != 0) {
        ((BYTE *)&g_unk0x005170e0[car])[3] = 0xff;
        g_splitGapDisplayFrames[car] = 0x4b;
    }
    if (CGameInfo::GetConfiguredGameMode() != 5 && CGameInfo::GetConfiguredGameMode() != 6 &&
        CGameInfo::GetConfiguredGameMode() != 7 && CGameInfo::GetConfiguredGameMode() != 4) {
        if (StageTiming_GetSplitDisplayState()) {
            g_stageSplitData[car].position =
                StageTiming_GetSplitPositionOfDriver(StageUI_GetRaceEndEventCount() + car, g_stageSplitData[car].split);
            return;
        }
        g_stageSplitData[car].position = 0xf;
    }
}

// Same as RallyData_RecordPassedSplit for the stage start and the time-trial ghost.
// match 57%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00413610
void RallyData_RecordStageStartAndGhostSplit(int car)
{
    int time;

    g_splitTargetTimes[0] = 0;
    if (StageTiming_GetCheckpointField19(car)) {
        g_lastSplitGroup = StageTiming_GetCheckpointGroupIndex(car);
        if (CGameInfo::GetConfiguredGameMode() == 0xc) {
            time = StageTiming_GetCarSplitTime(0, 0);
            g_stageSplitData[car].times[0] = time;
            g_stageSplitData[car].times[g_lastSplitGroup + 1] = time;
            g_stageSplitData[car].lastSplitTime = g_stageSplitData[car].times[0];
            g_stageSplitData[car].split = g_lastSplitGroup;
            NetRace_SendStageTime(g_lastSplitGroup, g_stageSplitData[car].times[0]);
            g_stageStartRecorded = 1;
        } else {
            if (g_lastSplitGroup >= 1 && g_lastSplitGroup < 10) {
                g_stageSplitData[car].times[g_lastSplitGroup + 1] = g_stageSplitData[car].times[0];
                g_stageSplitData[car].lastSplitTime = g_stageSplitData[car].times[0];
                g_stageSplitData[car].split = g_lastSplitGroup;
                NetRace_SendStageTime(g_lastSplitGroup, g_stageSplitData[car].times[0]);
            }
            g_stageStartRecorded = 1;
        }
    } else if (g_stageStartRecorded == 0) {
        return;
    }
    if (CGameInfo::GetConfiguredGameMode() == 0xc) {
        CGameInfo::GetNetworkStageBestTime(g_lastSplitGroup);
        time = CGameInfo::GetNetworkStageBestTime(g_lastSplitGroup);
        if (NetPlayers_IsNewLocalRecord())
            time = NetPlayers_GetPreviousBestTime();
        g_stageSplitData[car].targetTime = time;
        if (g_stageSplitData[car].lastSplitTime < time) {
            RallyData_PlayAlternateStageLoopSound(car, 1);
            g_stageStartRecorded = 0;
        } else if (time == 0) {
            g_splitTargetTimes[0] = 1;
        } else {
            RallyData_PlayAlternateStageLoopSound(car, 0);
            g_stageStartRecorded = 0;
        }
        if (g_lastSplitGroup == 0 && CGameInfo::GetConfiguredGameMode() != 0xc)
            return;
    } else {
        time = CGameInfo::GetNetworkStageBestTime(g_lastSplitGroup);
        g_stageSplitData[car].targetTime = time;
        if (g_stageSplitData[car].lastSplitTime < time) {
            RallyData_PlayAlternateStageLoopSound(car, 1);
            g_stageStartRecorded = 0;
        } else if (time == 0) {
            g_splitTargetTimes[0] = 1;
        } else {
            RallyData_PlayAlternateStageLoopSound(car, 0);
            g_stageStartRecorded = 0;
        }
        if (g_lastSplitGroup == 0 && CGameInfo::GetConfiguredGameMode() != 0xc)
            return;
    }
    ((BYTE *)&g_unk0x005170e0[car])[3] = 0xff;
    g_splitGapDisplayFrames[car] = 0x4b;
}

BYTE StageTiming_GetCarTimingByte84(int car);
BYTE RallyData_IsDriverRecordUsable(BYTE param1);
extern BYTE g_itemColour[4];
extern char g_classRowHeaderFormat[];
int RallyData_DrawListItem(int x, int y, char *pText, char last, BYTE alpha);
CategoryBestResults *RallyData_GetDriverBestResults(int index);
unsigned char RallyData_GetSelectionFlag26(void);
unsigned int RallyData_GetSetupFlag11(void);

// GLOBAL: CMR2 0x00536c00
int g_unk0x00536c00[2];
// GLOBAL: CMR2 0x00536c28
int g_finishLeaderMessage[2];
// GLOBAL: CMR2 0x00536c3c
int g_profileBestStageTime;
// GLOBAL: CMR2 0x00536ec4
int g_unk0x00536ec4[2];
// Name of the fastest driver on the stage so far (8 bytes in the original;
// longer names run into the next fields as there), the last split passed and
// the screen width the split bars were laid out for; kept as one block up to
// 0x536fe0.
struct StageLeaderBlock {
    char name[8];
    int lastSplit;     // 0x536ed8
    int splitBarResX;  // 0x536edc
    BYTE rest[0x100];
};
// GLOBAL: CMR2 0x00536ed0
StageLeaderBlock g_stageLeader;
// GLOBAL: CMR2 0x0053706c
int g_stageLeaderTime;

// GLOBAL: CMR2 0x005170d0
unsigned int g_stageResultTextColour = 0xb4ffffff;
// GLOBAL: CMR2 0x005170d4
unsigned int g_stageResultPanelColour = 0x96dcbebc;

// Renders each stage result row, including the player's, record and network
// result variants. A qualifying player has no second row.
// match 52%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004147f0
void RallyData_DrawStageResultRows(int car, short *position)
{
    CategoryBestResults *record;
    GameInfo0xa4 *gameInfo;
    unsigned int carTime;
    unsigned int recordTime;
    unsigned int time;
    char recordName[8];
    short rect[4];
    int baseHeight;
    int rows;
    int row;
    int shownRow;
    int fade;
    int highlight;
    int offset;
    int shiftedOffset;
    int currentOffset;
    int textY;
    int pixelFixed;
    int dimensionFixed;
    BYTE colour[4];
    BYTE base[4];
    int index;

    if (CGameInfo::GetConfiguredGameMode() == 3) {
        record = RallyData_GetDriverBestResults((StageUI_GetRaceEndEventCount() & 0xff) + car);
        index = (RallyDataCountryIndex() & 0xff) * 12 + (RallyDataStageIndex() & 0xff);
        carTime = record->rallyStageTimes[0][index].time; // [country][stage] as one index
        index = (RallyDataCountryIndex() & 0xff) * 11;
        index += (RallyDataStageIndex() & 0xff);
        gameInfo = CGameInfo::GetGameInfoFieldA4Address();
        recordTime = (gameInfo->rallyStageRecordTimes[index].value >> 7) & 0xffff;
        index = (RallyDataCountryIndex() & 0xff) * 11 + (RallyDataStageIndex() & 0xff);
        gameInfo = CGameInfo::GetGameInfoFieldA4Address();
        sprintf(recordName, "%s", gameInfo->rallyStageRecordTimes[index].ident);
    } else {
        record = RallyData_GetDriverBestResults((StageUI_GetRaceEndEventCount() & 0xff) + car);
        carTime = record->arcadeStageTimes[RallyData_GetSelectionBits10To11() & 0xff][RallyData_GetSelectionBits12To13() & 0xff].time;
        index = (RallyData_GetSelectionBits10To11() & 0xff) * 3;
        index += (RallyData_GetSelectionBits12To13() & 0xff);
        gameInfo = CGameInfo::GetGameInfoFieldA4Address();
        recordTime = (gameInfo->arcadeRecordTimes[index].value >> 7) & 0xffff;
        index = (RallyData_GetSelectionBits10To11() & 0xff) * 3 + (RallyData_GetSelectionBits12To13() & 0xff);
        gameInfo = CGameInfo::GetGameInfoFieldA4Address();
        sprintf(recordName, "%s", gameInfo->arcadeRecordTimes[index].ident);
    }

    baseHeight = (int)g_pGraphics->resY * 0xccc >> 16;
    rect[1] = (short)(((int)g_pGraphics->resY << 12 >> 16) + position[1]);
    if (CGameInfo::GetConfiguredGameMode() == 5 || CGameInfo::GetConfiguredGameMode() == 6 ||
        CGameInfo::GetConfiguredGameMode() == 7) {
        rows = 4;
        if ((BYTE)CGameInfo::GetNetworkOptionBit11())
            rows = 2;
    } else {
        rows = 3;
    }
    if (CGameInfo::GetConfiguredGameMode() == 10 || CGameInfo::GetConfiguredGameMode() == 12)
        rows = 2;

    offset = 0;
    shiftedOffset = -baseHeight;
    for (row = 0; row < rows; row++, offset += baseHeight, shiftedOffset += baseHeight) {
        currentOffset = offset;
        shownRow = row;
        rect[0] = (short)(((int)g_pGraphics->resX * 0xc00 >> 16) + position[0]);
        rect[2] = (short)((int)g_pGraphics->resX * 0x47ae >> 16);
        rect[3] = (short)((int)g_pGraphics->resY * 0xccc >> 16);
        fade = (signed char)g_unk0x00536c08[car] - 0x77;
        if (fade < 0)
            fade = 0;

        if (CGameInfo::GetConfiguredGameMode() != 10 && CGameInfo::GetConfiguredGameMode() != 12) {
            if ((BYTE)CGameInfo::GetNetworkOptionBit11()) {
                shownRow = row + 2;
            } else if (RallyData_IsDriverRecordUsable((BYTE)(StageUI_GetRaceEndEventCount() + car))) {
                if (row == 1)
                    continue;
                if (row >= 2)
                    currentOffset = shiftedOffset;
            }
        }

        if (shownRow == 0) {
            if (CGameInfo::GetConfiguredGameMode() == 10 || CGameInfo::GetConfiguredGameMode() == 12) {
                sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue, RallyData_GetRecord(0));
                time = NetPlayers_GetBestLapTime();
            } else {
                sprintf(CFrontend::m_stringDest, g_standingsRowFormat,
                        CFrontend::GetTextString(0x69), recordName);
                time = recordTime;
            }
            if (g_unk0x00536c00[car] != 0 && fade != 0)
                highlight = 1;
            else
                highlight = 0;
        } else if (shownRow == 1) {
            if (CGameInfo::GetConfiguredGameMode() == 10 || CGameInfo::GetConfiguredGameMode() == 12) {
                sprintf(CFrontend::m_stringDest, g_standingsRowFormat,
                        CFrontend::GetTextString(0x84), NetPlayers_GetRecordHolderName());
                if (CGameInfo::GetConfiguredGameMode() == 10) {
                    // The original keeps the previous row's highlight here.
                    if (RallyData_GetFlag25())
                        time = NetPlayers_GetSplitRecordTime(2);
                    else
                        time = NetPlayers_GetSplitRecordTime(8);
                } else {
                    time = CGameInfo::GetNetworkStageBestTime(0);
                    if (g_finishLeaderMessage[car] != 0 && fade != 0)
                        highlight = 1;
                    else
                        highlight = 0;
                }
            } else {
                sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue,
                        RallyData_GetRecord((BYTE)(StageUI_GetRaceEndEventCount() + car)));
                time = carTime;
                if (g_finishStageRecordMessage[car] != 0 && fade != 0)
                    highlight = 1;
                else
                    highlight = 0;
            }
        } else if (shownRow == 2) {
            sprintf(CFrontend::m_stringDest, g_standingsRowFormat,
                    CFrontend::GetTextString(0x84), g_stageLeader.name);
            time = g_stageLeaderTime;
            if (g_finishLeaderMessage[car] != 0 && fade != 0)
                highlight = 1;
            else
                highlight = 0;
        } else if (shownRow == 3) {
            sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue,
                    CFrontend::GetTextString(0xa5));
            time = StageTiming_GetCarSplitTime(car, StageTiming_GetCheckpointGroupIndex(car));
            if (g_unk0x00536ec4[car] != 0 && fade != 0)
                highlight = 1;
            else
                highlight = 0;
        }

        *(unsigned int *)base = g_stageResultPanelColour;
        if (highlight) {
            if (fade > 3) {
                colour[0] = 0xff;
                colour[1] = 0xff;
                colour[2] = 0xff;
                colour[3] = g_gapTextColour[3];
            } else {
                colour[0] = base[0] + (0xff - base[0]) * fade / 3;
                colour[1] = base[1] + (0xff - base[1]) * fade / 3;
                colour[2] = base[2] + (0xff - base[2]) * fade / 3;
                colour[3] = base[3] + (g_gapTextColour[3] - base[3]) * fade / 3;
            }
        } else {
            *(unsigned int *)colour = *(unsigned int *)base;
        }
        Sprite_FillRect(&g_pGraphics->field309_0x150, rect, colour, 2);
        textY = position[1] + currentOffset + 1 + ((int)g_pGraphics->resY * 0x1b32 >> 16);
        Font_DrawText(0, CFrontend::m_stringDest,
                      (short)((int)g_pGraphics->resX * 0xf95 >> 16) + position[0],
                      textY, (int *)&g_stageResultTextColour, 0x21);
        if (time != 0) {
            pixelFixed = (int)FTOI(__int64, X87S_MUL((double)(textY - 1), CGraphics::m_65536));
            dimensionFixed = (int)FTOI(__int64, X87S_MUL((double)g_pGraphics->resY, CGraphics::m_65536));
            FormatGapToLeader(time, 5, 5,
                              0x51e4 - (int)FTOI(__int64, X87S_MUL((double)position[0], g_minus65536)),
                              FixDiv(pixelFixed, dimensionFixed), &g_stageResultTextColour, 0x24, NULL, 0);
        } else {
            pixelFixed = (int)FTOI(__int64, X87S_MUL((double)(textY - 1), CGraphics::m_65536));
            dimensionFixed = (int)FTOI(__int64, X87S_MUL((double)g_pGraphics->resY, CGraphics::m_65536));
            FormatGapToLeader(-1, 5, 5,
                              0x51e4 - (int)FTOI(__int64, X87S_MUL((double)position[0], g_minus65536)),
                              FixDiv(pixelFixed, dimensionFixed), &g_stageResultTextColour, 0x24, NULL, 0);
        }
        rect[1] += (short)baseHeight;
    }
}

// Called when the car crosses the finish line: raises the finish messages
// (stage record, qualification, best time) for the stage end screen.
// FUNCTION: CMR2 0x00415a60
void RallyData_SetFinishLineResultMessages(int car)
{
    int time;

    if (!StageTiming_GetCheckpointField19(car))
        return;
    time = StageTiming_GetCarSplitTime(car, StageTiming_GetCheckpointGroupIndex(car));
    g_unk0x00536c00[car] = 0;
    g_unk0x00536c88[car] = 0;
    g_finishLeaderMessage[car] = 0;
    g_finishStageRecordMessage[car] = 0;
    g_unk0x00536ec4[car] = 0;
    if ((!(BYTE)CGameInfo::GetNetworkOptionBit11() || !(BYTE)RallyData_GetFlag24()) && !CGameInfo::GetGameModeOptionBit19()) {
        if (StageTiming_GetCarTimingByte84(car)) {
            g_unk0x00536c00[car] = 1;
            g_unk0x00536c08[car] = 0x7d;
        }
        if ((g_profileBestStageTime == 0 || g_profileBestStageTime > time) &&
            !RallyData_IsDriverRecordUsable((BYTE)(StageUI_GetRaceEndEventCount() + car))) {
            g_finishStageRecordMessage[car] = 1;
            g_unk0x00536c08[car] = 0x7d;
        }
    }
    if (CGameInfo::GetGameModeOptionBit19()) {
        if (CGameInfo::GetConfiguredGameMode() != 0xc || !NetPlayers_IsNewLocalRecord())
            goto done;
        g_stageLeaderTime = CGameInfo::GetNetworkStageBestTime(0);
        sprintf(g_stageLeader.name, "%s", (char *)RallyData_GetRecord(0));
        g_finishLeaderMessage[car] = 1;
        g_unk0x00536c08[car] = 0x7d;
    } else {
        if (g_stageLeaderTime != 0 && g_stageLeaderTime <= time)
            goto done;
        g_stageLeaderTime = time;
        sprintf(g_stageLeader.name, "%s", (char *)RallyData_GetRecord(StageUI_GetRaceEndEventCount() + car));
        g_finishLeaderMessage[car] = 1;
        g_unk0x00536c08[car] = 0x7d;
    }
done:
    if ((BYTE)RallyData_GetSelectionFlag26())
        RallyData_GetSetupFlag11();
    g_unk0x00536ec4[car] = 1;
    g_unk0x00536c08[car] = 0x7d;
}

// FUNCTION: CMR2 0x004074a0
bool RallyData_AdvanceRallySelectionPair(void)
{
    unsigned int v;

    if (CGameInfo::GetConfiguredGameMode() == 5) {
        v = g_selectedRallyData;
        v ^= (((v & 0xfffff000) + 0x1000) ^ v) & 0x3000;
        g_selectedRallyData = v;
        if ((v & 0x3000) >= 0x3000)
            return false;
        if ((v & 0xc00) != 0x800)
            return true;
    }
    return false;
}

// GLOBAL: CMR2 0x005167e0
int g_unk0x005167e0[16] = {
    0x00726d63, 0x006b7265, 0x00677564, 0x00636162, 0x00747a74, 0x006b6961,
    0x00627568, 0x00676e69, 0x006e6f6a, 0x00757274, 0x006e6f6a, 0x0073616b,
    0x006e6163, 0x00706f63, 0x007a7475, 0x080a0900,
};   // 3-letter country codes

// FUNCTION: CMR2 0x00407f20
int *RallyData_GetDriverNameIndexRecord(int index)
{
    int i;

    if (CGameInfo::GetConfiguredGameMode() == 5 ||
        CGameInfo::GetConfiguredGameMode() == 6 ||
        CGameInfo::GetConfiguredGameMode() == 7) {
        i = index;
        if (i < 0 || i > 5)
            i = 0;
        i = CAIHelper::ResolveNameIndex(i);
    } else {
        i = index;
        if (i < 0 || i > 0xf)
            i = 0;
    }
    return &g_unk0x005167e0[i];
}

// FUNCTION: CMR2 0x0040fe50
char *RallyData_GetSelectedSettingText(void)
{
    int table[8];

    table[0] = 6;
    table[1] = 3;
    table[2] = 1;
    table[3] = 4;
    table[4] = 0;
    table[5] = 2;
    table[6] = 5;
    table[7] = 7;
    return CFrontend::GetTextString(table[(RallyData_GetSelectionBits10To11() & 0xff) * 3 +
                                          (RallyData_GetSelectionBits12To13() & 0xff)]);
}

extern char g_noTimeText[];
BYTE RallyData_IsDriverRecordUsable(BYTE param1);
extern BYTE g_itemColour[4];
extern char g_classRowHeaderFormat[];
int RallyData_DrawListItem(int x, int y, char *pText, char last, BYTE alpha);
CategoryBestResults *RallyData_GetDriverBestResults(int index);

// GLOBAL: CMR2 0x00516e3c
char g_loadRecordTimeFormat[] = "%.2d:%.2d.%.2d";

#define LOAD_TIME_TEXT(t) sprintf(CFrontend::m_stringDest, g_loadRecordTimeFormat, (t) / 6000, (int)(((t) / 100) % 60), (t) % 100)

// Returns 1 when the stage must end early: a championship with more lost
// than remaining rounds, or a replay being skipped.
// FUNCTION: CMR2 0x004100a0
int RallyData_ShouldEndStageEarly(void)
{
    unsigned int *pState = RallyData_GetChampionshipState();

    if (CGameInfo::GetConfiguredGameMode() == 4 && ((*pState >> 3) & 7) > 5u - (*pState & 7))
        return 1;
    if (CGameInfo::IsConfiguredMultiplayer() && StageUI_GetRaceEndEventCount()) {
        CGraphics::ClearTarget();
        return 1;
    }
    return 0;
}

// Loading screen text: the breadcrumb (game mode, rally, stage), the stage
// record and the best time of every player on it, faded in with alpha.
// match 75%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00410100
void RallyData_DrawFadedLoadingText(BYTE alpha)
{
    BYTE colour[4];
    unsigned int time;
    int x;
    int i;
    int shown;

    colour[0] = g_itemColour[0];
    colour[1] = g_itemColour[1];
    colour[2] = g_itemColour[2];
    colour[3] = alpha;
    x = (int)(g_pGraphics->resX * 30) / 640;
    if ((BYTE)RallyData_IsChampionshipFinalStage()) {
        RallyData_DrawListItem(x, g_pGraphics->resY / 2, CFrontend::GetTextString(0x93), 1, alpha);
        return;
    }
    switch (CGameInfo::GetConfiguredGameMode()) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 8:
    case 9:
    case 10:
        x = RallyData_DrawListItem(x, g_pGraphics->resY / 2, CFrontend::GetTextString(0x6b), 0, alpha);
        x = RallyData_DrawListItem(x, g_pGraphics->resY / 2, CFrontend::GetTextString(0x41), 0, alpha);
        sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue,
                CFrontend::GetTextString(RallyDataCountryIndex() & 0xff));
        CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
        x = RallyData_DrawListItem(x, g_pGraphics->resY / 2, CFrontend::m_stringDest, 0, alpha);
        if (RallyDataStageIndex() == 10)
            strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(0xba));
        else
            sprintf(CFrontend::m_stringDest, g_classRowHeaderFormat, CFrontend::GetTextString(0x40),
                    RallyDataStageIndex() + 1);
        RallyData_DrawListItem(x, g_pGraphics->resY / 2, CFrontend::m_stringDest, 1, alpha);
        time = (CGameInfo::GetGameInfoFieldA4Address()
                    ->rallyStageRecordTimes[(RallyDataCountryIndex() & 0xff) * 11 + (RallyDataStageIndex() & 0xff)]
                    .value >> 7) & 0xffff;
        LOAD_TIME_TEXT(time);
        Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x268) / 640,
                      (int)(g_pGraphics->resY * 0x1a6) / 480, (int *)colour, 0x24);
        sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue,
                CGameInfo::GetGameInfoFieldA4Address()
                    ->rallyStageRecordTimes[(RallyDataCountryIndex() & 0xff) * 11 + (RallyDataStageIndex() & 0xff)]
                    .ident);
        Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x22a) / 640,
                      (int)(g_pGraphics->resY * 0x1a6) / 480, (int *)colour, 0x24);
        Font_DrawText(0, CFrontend::GetTextString(0x6d), (int)(g_pGraphics->resX * 0x268) / 640,
                      (int)(g_pGraphics->resY * 0x1a6) / 480 - (int)(g_pGraphics->resY * 0x18) / 480, (int *)colour,
                      0x24);
        shown = 0;
        for (i = 0; i < (int)(RallyDataState() & 0xff); i++) {
            if (!RallyData_IsDriverRecordUsable(i)) {
                shown++;
                if (RallyData_GetDriverBestResults(i)
                        ->rallyStageTimes[RallyDataCountryIndex() & 0xff][RallyDataStageIndex() & 0xff].header & 0x80) {
                    time = RallyData_GetDriverBestResults(i)
                               ->rallyStageTimes[RallyDataCountryIndex() & 0xff][RallyDataStageIndex() & 0xff].time;
                    LOAD_TIME_TEXT(time);
                } else {
                    sprintf(CFrontend::m_stringDest, "%s", g_noTimeText);
                }
                Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x268) / 640,
                              (int)(g_pGraphics->resY * 0x1a6) / 480 - ((int)(g_pGraphics->resY * 0x18) / 480) * (shown + 2),
                              (int *)colour, 0x24);
                sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue, (char *)RallyData_GetRecord(i));
                Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x22a) / 640,
                              (int)(g_pGraphics->resY * 0x1a6) / 480 - ((int)(g_pGraphics->resY * 0x18) / 480) * (shown + 2),
                              (int *)colour, 0x24);
            }
        }
        if (shown > 0)
            Font_DrawText(0, CFrontend::GetTextString(0x70), (int)(g_pGraphics->resX * 0x268) / 640,
                          (int)(g_pGraphics->resY * 0x1a6) / 480 - ((int)(g_pGraphics->resY * 0x18) / 480) * (shown + 3),
                          (int *)colour, 0x24);
        break;
    case 4:
        x = RallyData_DrawListItem(x, g_pGraphics->resY / 2, CFrontend::GetTextString(0x6b), 0, alpha);
        x = RallyData_DrawListItem(x, g_pGraphics->resY / 2, CFrontend::GetTextString(0x6f), 0, alpha);
        sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue,
                CFrontend::GetTextString(RallyDataCountryIndex() & 0xff));
        CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
        RallyData_DrawListItem(x, g_pGraphics->resY / 2, CFrontend::m_stringDest, 1, alpha);
        return;
    case 5:
    case 6:
    case 7:
    case 11:
    case 12:
        x = RallyData_DrawListItem(x, g_pGraphics->resY / 2, CFrontend::GetTextString(0x6b), 0, alpha);
        x = RallyData_DrawListItem(x, g_pGraphics->resY / 2, CFrontend::GetTextString(0x6c), 0, alpha);
        sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue, RallyData_GetSelectedSettingText());
        CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
        RallyData_DrawListItem(x, g_pGraphics->resY / 2, CFrontend::m_stringDest, 1, alpha);
        time = (CGameInfo::GetGameInfoFieldA4Address()
                    ->arcadeRecordTimes[(RallyData_GetSelectionBits10To11() & 0xff) * 3 + (RallyData_GetSelectionBits12To13() & 0xff)]
                    .value >> 7) & 0xffff;
        LOAD_TIME_TEXT(time);
        Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x268) / 640,
                      (int)(g_pGraphics->resY * 0x1a6) / 480, (int *)colour, 0x24);
        sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue,
                CGameInfo::GetGameInfoFieldA4Address()
                    ->arcadeRecordTimes[(RallyData_GetSelectionBits10To11() & 0xff) * 3 + (RallyData_GetSelectionBits12To13() & 0xff)]
                    .ident);
        Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x22a) / 640,
                      (int)(g_pGraphics->resY * 0x1a6) / 480, (int *)colour, 0x24);
        Font_DrawText(0, CFrontend::GetTextString(0x6d), (int)(g_pGraphics->resX * 0x268) / 640,
                      (int)(g_pGraphics->resY * 0x1a6) / 480 - (int)(g_pGraphics->resY * 0x18) / 480, (int *)colour,
                      0x24);
        shown = 0;
        for (i = 0; i < (int)(RallyDataState() & 0xff); i++) {
            if (!RallyData_IsDriverRecordUsable(i)) {
                shown++;
                if (RallyData_GetDriverBestResults(i)
                        ->arcadeStageTimes[RallyData_GetSelectionBits10To11() & 0xff][RallyData_GetSelectionBits12To13() & 0xff]
                        .header & 0x80) {
                    time = RallyData_GetDriverBestResults(i)
                               ->arcadeStageTimes[RallyData_GetSelectionBits10To11() & 0xff][RallyData_GetSelectionBits12To13() & 0xff]
                               .time;
                    LOAD_TIME_TEXT(time);
                } else {
                    sprintf(CFrontend::m_stringDest, "%s", g_noTimeText);
                }
                Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x268) / 640,
                              (int)(g_pGraphics->resY * 0x1a6) / 480 - ((int)(g_pGraphics->resY * 0x18) / 480) * (shown + 2),
                              (int *)colour, 0x24);
                sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue, (char *)RallyData_GetRecord(i));
                Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x22a) / 640,
                              (int)(g_pGraphics->resY * 0x1a6) / 480 - ((int)(g_pGraphics->resY * 0x18) / 480) * (shown + 2),
                              (int *)colour, 0x24);
            }
        }
        if (shown > 0)
            Font_DrawText(0, CFrontend::GetTextString(0x70), (int)(g_pGraphics->resX * 0x268) / 640,
                          (int)(g_pGraphics->resY * 0x1a6) / 480 - ((int)(g_pGraphics->resY * 0x18) / 480) * (shown + 3),
                          (int *)colour, 0x24);
        break;
    default:
        return;
    }
}

// GLOBAL: CMR2 0x0058c938
BYTE *g_sectorEntryCarFlags;
// GLOBAL: CMR2 0x0058c94c
SectorObjectEntry *g_movingSectorEntries;
// GLOBAL: CMR2 0x0058c958
int *g_sectorEntryRaised;
// GLOBAL: CMR2 0x0058ca6c
unsigned int g_unk0x0058ca6c;

// Per-sector object lists: sector -> list index (-1 none), entry count and
// 4-byte entries, for two kinds of stage objects.
// GLOBAL: CMR2 0x0058c940
short *g_sectorListIndexA;
// GLOBAL: CMR2 0x0058c960
unsigned short *g_sectorListCountA;
// GLOBAL: CMR2 0x0058ca78
SectorObjectEntry **g_sectorListEntriesA;
// GLOBAL: CMR2 0x0058c93c
short *g_sectorListIndexB;
// GLOBAL: CMR2 0x0058c964
unsigned short *g_sectorListCountB;
// GLOBAL: CMR2 0x0058c954
SectorObjectEntry **g_sectorListEntriesB;

extern int g_sectorCount;

// FUNCTION: CMR2 0x00471b30
SectorObjectEntry **Sector_GetListA(unsigned int sector, unsigned int *pCount)
{
    int index;

    *pCount = 0;
    if (g_sectorListIndexA != NULL && sector < (unsigned int)g_sectorCount &&
        (index = g_sectorListIndexA[sector]) != -1) {
        *pCount = g_sectorListCountA[index];
        return g_sectorListEntriesA + index;
    }
    return NULL;
}

// FUNCTION: CMR2 0x00471b80
SectorObjectEntry **Sector_GetListB(unsigned int sector, unsigned int *pCount)
{
    int index;

    *pCount = 0;
    if (g_sectorListIndexB != NULL && sector < (unsigned int)g_sectorCount &&
        (index = g_sectorListIndexB[sector]) != -1) {
        *pCount = g_sectorListCountB[index];
        return g_sectorListEntriesB + index;
    }
    return NULL;
}

// Championship state entry for the current round kind (bits 3..5 of the
// state; bits 12..15 select the round within kinds 1..3).
// FUNCTION: CMR2 0x00473620
unsigned int *RallyData_GetRoundEntry(void)
{
    unsigned int *pState;
    unsigned int state;

    pState = RallyData_GetChampionshipState();
    state = *pState;
    switch ((state >> 3) & 7) {
    case 1:
        return pState + ((state >> 12) & 0xf) * 3 + 0x16;
    case 2:
        return pState + ((state >> 12) & 0xf) * 3 + 10;
    case 3:
        return pState + ((state >> 12) & 0xf) * 3 + 4;
    case 4:
        return pState + 1;
    default:
        return NULL;
    }
}

// Moves each element's object in or out of the way to match the car's
// reached flags (multi-player only).
// match 29%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00471bf0
void RallyData_ApplyPlayerFlagsToObjects(int car)
{
    int i;
    BYTE mask;
    StageObject *pObject;

    if ((BYTE)RallyDataState() > 1) {
        mask = 1 << car;
        for (i = 0; i < (int)g_unk0x0058ca6c; i++) {
            if ((g_sectorEntryCarFlags[i] & mask) > 0 && g_sectorEntryRaised[i] == 0) {
                pObject = g_movingSectorEntries[i].pObject;
                pObject->position.y += 0x3e80000;
                g_movingSectorEntries[i].pObject->visible = 0xff;
                g_sectorEntryRaised[i] = 1;
            } else if ((g_sectorEntryCarFlags[i] & mask) == 0 && g_sectorEntryRaised[i] != 0) {
                pObject = g_movingSectorEntries[i].pObject;
                pObject->position.y -= 0x3e80000;
                g_movingSectorEntries[i].pObject->visible = 0;
                g_sectorEntryRaised[i] = 0;
            }
        }
    } else {
        RallyData_ValidateIndex(car);
    }
}

// Copies the 12-byte vector and, when the entry is not already flagged,
// raises the destination's Y component by 0x3e80000.
// FUNCTION: CMR2 0x00471cc0
void RallyData_CopyRaisedElementVector(int *pDest, SectorObjectEntry **pParam1)
{
    StageObject *pSrc;
    SectorObjectEntry *pp;
    ptrdiff_t index;

    pp = *pParam1;
    pSrc = pp->pObject;
    *(FixVector *)pDest = pSrc->position;
    index = ((size_t)((UINT_PTR)pParam1[0] - (UINT_PTR)g_movingSectorEntries) / sizeof(SectorObjectEntry));
    if (index >= (int)g_unk0x0058ca6c || index < 0)
        return;
    if ((BYTE)RallyDataState() > 1) {
        if (g_sectorEntryRaised[index] != 0)
            return;
        pDest[1] = pDest[1] + 0x3e80000;
    } else {
        if ((g_sectorEntryCarFlags[index] & 1) != 0)
            return;
        pDest[1] += 0x3e80000;
    }
}


// Bumps the 0x7f80-masked field of the entry selected by each 0x30-byte record.
// FUNCTION: CMR2 0x004ec1a0
void RallyData_IncrementSelectedCategoryCounters(void)
{
    struct ProgressFlags { unsigned int low : 7; unsigned int progress : 8; unsigned int rest : 17; };
    ProgressFlags *pEntry;
    int i = 0;
    if (CGameInfo::GetConfiguredPlayerCount() <= i)
        return;
    do {
        unsigned int value = g_saveProfiles[g_saveSlots[i].category].options;
        pEntry = (ProgressFlags *)&g_saveProfiles[g_saveSlots[i].category].options;
        if ((value & 0x7f80) < 0x7f80)
            *(unsigned int *)pEntry = value ^ ((value ^ ((value & 0xffffff80) + 0x80)) & 0x7f80);
        i++;
    } while (i < CGameInfo::GetConfiguredPlayerCount());
}

// Default setup of a player's car: validates the player's rally data (except
// in mode 4) and stores the default position and heading values.
// FUNCTION: CMR2 0x004ec260
void RallyData_SetPlayerDefaultCarSetup(int player)
{
    ChaseCameraSettings values;

    if (CGameInfo::GetConfiguredGameMode() != 4)
        RallyData_ValidateIndex(player);
    values.pitch = 0x2d;
    values.offset.x = 0;
    values.offset.y = 0x18000;
    values.offset.z = 0x68000;
    values.lag = 0xe0000;
    RallyData_SetDriverCameraRecord((BYTE)player, &values);
}

// GLOBAL: CMR2 0x00520128
BYTE g_unk0x00520128[0x28] = {
    0x04, 0x02, 0x3c, 0x32, 0x32, 0x32, 0x32, 0x06, 0x02, 0x28, 0x46, 0x32, 0x32, 0x32, 0x04, 0x02,
    0x1e, 0x32, 0x32, 0x32, 0x32, 0x02, 0x02, 0x32, 0x32, 0x32, 0x32, 0x32, 0x04, 0x02, 0x32, 0x32,
    0x32, 0x32, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00,
};


// Countries of the championship flag row, in display order (OptionMenu_DrawCountryFlags
// walks it as an array, so it must be one block).
// GLOBAL: CMR2 0x0082c698
int g_championshipFlagCountries[6];
// GLOBAL: CMR2 0x0082c6bc
int g_unk0x0082c6bc;

// FUNCTION: CMR2 0x00503e00
void RallyData_BuildCountryWeatherIndexMap(void)
{
    if ((unsigned char)RallyDataCountryIndex() == 3) {
        g_championshipFlagCountries[0] = 0;
        g_unk0x0082c6bc = 6;
        g_championshipFlagCountries[1] = 1;
        g_championshipFlagCountries[2] = 2;
        g_championshipFlagCountries[3] = 6;
        g_championshipFlagCountries[4] = 7;
        g_championshipFlagCountries[5] = 8;
    } else {
        g_unk0x0082c6bc = 6;
        g_championshipFlagCountries[0] = 0;
        g_championshipFlagCountries[1] = 1;
        g_championshipFlagCountries[2] = 2;
        g_championshipFlagCountries[3] = 3;
        g_championshipFlagCountries[4] = 4;
        g_championshipFlagCountries[5] = 5;
    }
}

#include "InstallInfo.h"
#include "Game.h"
GenericFile *OptionMenu_GetCommonArchive(void);
GenericFile *OptionMenu_GetStageArchive(void);
int StageTiming_NoOpResourceRelease(void);

extern char g_str0x00519268[4];
extern char g_str0x0051926c[4];
extern char g_str0x00519270[4];
extern char g_str0x00519274[4];
extern char g_str0x00519278[4];
extern char g_str0x0051927c[4];
extern char g_str0x00519280[4];
// GLOBAL: CMR2 0x00519348
char g_str0x00519348[6] = "ITALY";
// GLOBAL: CMR2 0x00519350
char g_str0x00519350[6] = "KENYA";
// GLOBAL: CMR2 0x00519358
char g_str0x00519358[7] = "SWEDEN";
// GLOBAL: CMR2 0x00519360
char g_str0x00519360[7] = "FRANCE";
// GLOBAL: CMR2 0x00519368
char g_str0x00519368[7] = "GREECE";
// GLOBAL: CMR2 0x00519370
char g_str0x00519370[8] = "FINLAND";
// GLOBAL: CMR2 0x0052726c
char g_str0x0052726c[10] = "AUSTRALIA";
// GLOBAL: CMR2 0x00527278
char g_str0x00527278[8] = "SSHOWER";
// GLOBAL: CMR2 0x00527280
char g_str0x00527280[6] = "CLEAR";

// GLOBAL: CMR2 0x0052714c
char *g_unk0x0052714c[8] = {
    g_str0x00519370, g_str0x00519368, g_str0x00519360, g_str0x00519358, g_str0x0052726c, g_str0x00519350, g_str0x00519348, CFrontend::m_strUK
};
// GLOBAL: CMR2 0x0052716c
char *g_unk0x0052716c[8] = {
    g_str0x00519280, g_str0x0051927c, g_str0x00519278, g_str0x00519274, g_str0x00519270, g_str0x0051926c, g_str0x00519268, CFrontend::m_strUK
};
// GLOBAL: CMR2 0x00527128
char *g_unk0x00527128[9] = {
    (char *)(g_unk0x0051682c + 0x70), g_str0x00527280, (char *)(g_unk0x0051682c + 0x68), (char *)(g_unk0x0051682c + 0x60), (char *)(g_unk0x0051682c + 0x58), (char *)(g_unk0x0051682c + 0x50), g_str0x00527278, (char *)(g_unk0x0051682c + 0x48), (char *)(g_unk0x0051682c + 0x78)
};

// Weather map textures of the current rally. One block in the original: the
// clear loop in RallyData_LoadCurrentRallyWeatherTextures runs five times and spills into the next fields,
// and RallyData_SetupWeatherTextureEntries indexes past maps[3]. As separate globals the clear loop
// overran into unrelated data (it zeroed g_selectedRallyData: France asked for
// the CD because the weather textures were looked up under FINLAND).
struct WeatherMapSet {
    unsigned int count; // 0x00, low byte used
    void *mainMap;      // 0x04
    Texture *maps[4];   // 0x08
    short rects[8];     // 0x18, x/y pairs
    BYTE slots[8];      // 0x28
};
// GLOBAL: CMR2 0x0082cb48
WeatherMapSet g_weatherMaps;
// GLOBAL: CMR2 0x0082c690
void *g_unk0x0082c690;
extern int g_optionPreviewPanelCount;
// GLOBAL: CMR2 0x0082ca20
Texture *g_unk0x0082ca20[9];
// GLOBAL: CMR2 0x0082c9e8
void *g_unk0x0082c9e8;
// Source rectangle of the map texture (x, y, w, h).
// GLOBAL: CMR2 0x0082c9f4
short g_mapSrcRect[4];
// GLOBAL: CMR2 0x0082c9fc
short g_unk0x0082c9fc;
// GLOBAL: CMR2 0x0082c9fe
short g_unk0x0082c9fe;
// GLOBAL: CMR2 0x0082ca00
short g_unk0x0082ca00;
// GLOBAL: CMR2 0x0082ca02
short g_unk0x0082ca02;
// Weather map symbol positions per country: an (x, y) pair for each of the
// eleven stage slots, indexed [country][slot * 2]. The decompilation had kept
// only the first country, so the others read the strings that follow.
// GLOBAL: CMR2 0x0052718c
BYTE g_weatherSymbolPositions[8][22] = {
    { 0x94, 0x99, 0x91, 0x8b, 0xa0, 0x8e, 0x94, 0x99, 0x7d, 0x4c, 0x75, 0x50, 0x6f, 0x44, 0x7d, 0x4c, 0x82, 0x9c, 0x79, 0xa5, 0x00, 0x00 },
    { 0x5c, 0x68, 0x5d, 0x62, 0x57, 0x68, 0x5b, 0x63, 0x54, 0x57, 0x4f, 0x5f, 0x57, 0x5c, 0x58, 0x56, 0x6e, 0x47, 0x73, 0x4b, 0x70, 0x49 },
    { 0xcf, 0x86, 0xd8, 0x83, 0xd4, 0x89, 0xdb, 0x87, 0xcd, 0x98, 0xd4, 0x91, 0xcf, 0x92, 0xd9, 0x8f, 0xd1, 0x9b, 0xce, 0x90, 0x00, 0x00 },
    { 0x6f, 0x59, 0x75, 0x55, 0x7e, 0x51, 0x7b, 0x57, 0x76, 0x88, 0x72, 0x82, 0x7b, 0x82, 0x76, 0x7e, 0x74, 0xac, 0x6e, 0xa8, 0x72, 0xaa },
    { 0x49, 0x60, 0x4d, 0x59, 0x4f, 0x60, 0x49, 0x64, 0x5c, 0x37, 0x61, 0x38, 0x5d, 0x32, 0x5b, 0x3b, 0x25, 0x59, 0x2b, 0x61, 0x00, 0x00 },
    { 0x54, 0x7a, 0x57, 0x74, 0x56, 0x7a, 0x57, 0x82, 0x71, 0x6a, 0x68, 0x68, 0x68, 0x70, 0x6c, 0x6e, 0x4f, 0x65, 0x54, 0x60, 0x53, 0x63 },
    { 0x5b, 0x42, 0x62, 0x3a, 0x62, 0x48, 0x64, 0x3f, 0x46, 0x35, 0x4a, 0x39, 0x4b, 0x33, 0x3f, 0x37, 0x87, 0x6b, 0x7f, 0x67, 0x00, 0x00 },
    { 0x7b, 0x6e, 0x7f, 0x6c, 0x7b, 0x6a, 0x7f, 0x66, 0x7b, 0x88, 0x7e, 0x85, 0x7c, 0x7f, 0x7e, 0x7b, 0x6c, 0x91, 0x6e, 0x95, 0x6d, 0x93 },
};
// GLOBAL: CMR2 0x00527300
char g_str0x00527300[35] = "%s\\Textures\\Weather\\%s\\MainMap.tga";
// GLOBAL: CMR2 0x005272dc
char g_str0x005272dc[34] = "%s\\Textures\\Weather\\%s\\%s%.2d.tga";
// GLOBAL: CMR2 0x005272b4
char g_str0x005272b4[38] = "%s\\Textures\\Weather\\Symbols\\%d\\%s.tga";
// GLOBAL: CMR2 0x00527290
char g_str0x00527290[33] = "%s\\Textures\\Symbols\\%d\\Arrow.tga";

// 0x50-byte entry of the table at 0x82c6c8 (FixInterp in GameInfo.h).
extern FixInterp g_optionPreviewPanels[16];
extern short g_optionPreviewExpandedRect[4];

// 0x50-byte entry of 0x82c6c8 as seen by the per-car split panel.
extern BYTE g_selectedOptionPreviewPanel;
extern int g_unk0x0082c6c0;
extern int g_unk0x0082cb44;

// Builds the weather textures of the current rally: the main map, the
// per-weather maps and the weather symbols, then rebinds the stage state.
// match 65%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00503ea0
void RallyData_LoadCurrentRallyWeatherTextures(void)
{
    int i;
    int stage;
    int index;

    *(BYTE *)&g_weatherMaps.count = 0;
    for (i = 0; i < 5; i++) {
        g_weatherMaps.maps[i] = NULL;
        g_weatherMaps.rects[i * 2] = 0;
        g_weatherMaps.rects[i * 2 + 1] = 0;
    }
    stage = (BYTE)RallyDataStageIndex() >> 2;
    if (stage < 2)
        *(BYTE *)&g_weatherMaps.count = 4;
    else
        *(BYTE *)&g_weatherMaps.count = RallyDataCountryIndex() % 2 ? 3 : 2;
    for (i = 0; i < 4; i++) {
        if (i < (int)(g_weatherMaps.count & 0xff)) {
            g_weatherMaps.slots[i] = (BYTE)(stage * 4 + i);
            g_weatherMaps.rects[i * 2] = g_weatherSymbolPositions[RallyDataCountryIndex()][g_weatherMaps.slots[i] * 2];
            g_weatherMaps.rects[i * 2 + 1] = g_weatherSymbolPositions[RallyDataCountryIndex()][g_weatherMaps.slots[i] * 2 + 1];
        }
    }
    sprintf(CFrontend::m_stringDest, g_str0x00527300, CInstallInfo::GetSetupRepDir(),
            g_unk0x0052714c[RallyDataCountryIndex()]);
    g_weatherMaps.mainMap = CTexture::FindLoadTexture(OptionMenu_GetStageArchive(),
                                                CFrontend::m_stringDest, 0, NULL, false, 0);
    for (i = 0; i < 4; i++) {
        if (i < (int)(g_weatherMaps.count & 0xff)) {
            sprintf(CFrontend::m_stringDest, g_str0x005272dc, CInstallInfo::GetSetupRepDir(),
                    g_unk0x0052714c[RallyDataCountryIndex()],
                    g_unk0x0052716c[RallyDataCountryIndex()],
                    g_weatherMaps.slots[i] + 1);
            g_weatherMaps.maps[i] = CTexture::FindLoadTexture(OptionMenu_GetStageArchive(),
                                                                 CFrontend::m_stringDest, 0, NULL, false, 0);
        }
    }
    for (i = 0; i < 9; i++) {
        sprintf(CFrontend::m_stringDest, g_str0x005272b4, CInstallInfo::GetSetupRepDir(),
                0x280, g_unk0x00527128[i]);
        g_unk0x0082ca20[i] = CTexture::FindLoadTexture(OptionMenu_GetCommonArchive(),
                                                             CFrontend::m_stringDest, 0, NULL, false, 0);
    }
    sprintf(CFrontend::m_stringDest, g_str0x00527290, CInstallInfo::GetSetupRepDir(), 0x280);
    g_unk0x0082c690 = CTexture::FindLoadTexture(OptionMenu_GetCommonArchive(),
                                                CFrontend::m_stringDest, 0, NULL, false, 0);
    CGame::RegisterCallback((void *)StageTiming_NoOpResourceRelease, 0);
}

// Sets up the weather entries of the stage: their texture, screen rectangle and
// the fixed texture coordinates.
// FUNCTION: CMR2 0x005040f0
void RallyData_SetupWeatherTextureEntries(void)
{
    FixInterp *pEntry;
    int index;
    int i;

    g_unk0x0082c9e8 = g_weatherMaps.mainMap;
    g_optionPreviewExpandedRect[0] = 0x1c;
    g_optionPreviewExpandedRect[1] = 0x72;
    g_optionPreviewExpandedRect[2] = 0x100;
    g_optionPreviewExpandedRect[3] = 0xf6;
    *(int *)&g_mapSrcRect[0] = *(int *)&((Texture *)g_weatherMaps.mainMap)->field_0x11c;
    *(int *)&g_mapSrcRect[2] = *(int *)&((Texture *)g_weatherMaps.mainMap)->width;
    g_mapSrcRect[3] = 0xf6;
    g_unk0x0082c9fc = 0xd8;
    g_unk0x0082c9fe = 0x7c;
    g_unk0x0082ca00 = 0x3b;
    g_unk0x0082ca02 = 0x39;
    g_optionPreviewPanelCount = CGameInfo::GetPreviewLayoutMode();
    for (i = 0; i < g_optionPreviewPanelCount; i++) {
        pEntry = g_optionPreviewPanels + i;
        pEntry->end = 0;
        pEntry->start = 0;
        pEntry->current = 0;
        pEntry->phase1 = 0;
        pEntry->phase2 = 0;
        pEntry->active = 0;
        pEntry->distance = 0;
        pEntry->startTime3 = 0;
        pEntry->startTime2 = 0;
        pEntry->startTime = 0;
        index = (RallyDataStageIndex() & 0xff) % 4 + i;
        pEntry->texture = g_weatherMaps.maps[index];
        pEntry->count = g_weatherMaps.slots[index];
        pEntry->srcX1 = g_weatherMaps.rects[index * 2] + g_optionPreviewExpandedRect[0];
        pEntry->srcY1 = g_weatherMaps.rects[index * 2 + 1];
        pEntry->srcY1 += g_optionPreviewExpandedRect[1];
        pEntry->srcX2 = 10;
        pEntry->srcY2 = 9;
        pEntry->u0 = (g_mapSrcRect[0] << 16) +
                     FixMul(FixDiv((pEntry->srcX1 - g_optionPreviewExpandedRect[0]) << 16, g_optionPreviewExpandedRect[2] << 16),
                            g_mapSrcRect[2] << 16);
        pEntry->u1 = (g_mapSrcRect[0] << 16) +
                     FixMul(FixDiv((pEntry->srcX1 - g_optionPreviewExpandedRect[0] + pEntry->srcX2) << 16,
                                   g_optionPreviewExpandedRect[2] << 16),
                            g_mapSrcRect[2] << 16);
        pEntry->v0 = (g_mapSrcRect[1] << 16) +
                     FixMul(FixDiv((pEntry->srcY1 - g_optionPreviewExpandedRect[1]) << 16, g_optionPreviewExpandedRect[3] << 16),
                            g_mapSrcRect[3] << 16);
        pEntry->v1 = (g_mapSrcRect[1] << 16) +
                     FixMul(FixDiv((pEntry->srcY1 - g_optionPreviewExpandedRect[1] + pEntry->srcY2) << 16,
                                   g_optionPreviewExpandedRect[3] << 16),
                            g_mapSrcRect[3] << 16);
    }
    g_selectedOptionPreviewPanel = 0;
    g_unk0x0082c6c0 = CMain::GetFrameDelta();
    g_unk0x0082cb44 = 0;
    RallyData_BuildCountryWeatherIndexMap();
}

// Copies the per-driver stage times into the 0x30-byte records, first for the
// used drivers (in reverse) and then for the unused ones.
// FUNCTION: CMR2 0x00408d80
void RallyData_CopyDriverStageTimesToRecords(void)
{
    int i;
    int n;
    int index;

    for (i = 0; i < (BYTE)CGameInfo::GetConfiguredPlayerCount(); i++) {
        index = 15 - i;
        g_saveSlots[i].seconds = RallyTiming_GetStageTimeSeconds(index);
        g_saveSlots[i].time = RallyTiming_GetOverallTimeCentiseconds(index);
    }
    n = 16 - (BYTE)CGameInfo::GetConfiguredPlayerCount();
    for (i = 0; i < n; i++) {
        index = (BYTE)CGameInfo::GetConfiguredPlayerCount() + i;
        g_saveSlots[index].seconds = RallyTiming_GetStageTimeSeconds(i);
        g_saveSlots[index].time = RallyTiming_GetOverallTimeCentiseconds(i);
    }
}


void RallyData_ResetPlayerRouteProbes(void);

// Per car progress along the route, 0x18 bytes
#include "RaceRecord.h"

// GLOBAL: CMR2 0x00538a78
unsigned int g_routeProbeBestDistance[2];
// GLOBAL: CMR2 0x00538a80
unsigned short g_routeProbeIndex[2];
// GLOBAL: CMR2 0x00538a84
int g_routeNodeCount;
// GLOBAL: CMR2 0x00538a88
int g_unk0x00538a88;
// GLOBAL: CMR2 0x00538a94
int g_unk0x00538a94;
// GLOBAL: CMR2 0x00538a98
short g_routeProbeBestIndex[2];
// GLOBAL: CMR2 0x00538aa8
RaceRecord g_raceRecords[8];
// GLOBAL: CMR2 0x00538c8c
int g_routeProbeCycles[2];

// Restarts race record index; the two players also reset their route probe.
// FUNCTION: CMR2 0x004207a0
void RallyData_ResetRaceRecordAndRouteProbe(int index)
{
    RaceRecord *pRecord = &g_raceRecords[index];
    pRecord->routeNodeIndex = pRecord->resetRouteNodeIndex;
    pRecord->field_0x8 = 0;
    pRecord->field_0x14 = 0;
    pRecord->field_0xc = 0;
    if (index < 2) {
        g_routeProbeCycles[index] = 0;
        g_routeProbeBestDistance[index] = 0;
        g_routeProbeBestIndex[index] = 0;
        g_routeProbeIndex[index] = 0;
    }
}

// Sets the car's race record position.
// FUNCTION: CMR2 0x004213d0
void RallyData_SetCarRaceRecordPosition(Car *pCar, int value)
{
    if (g_routeNodeCount != 0) {
        g_raceRecords[pCar->index].routeNodeIndex = value;
        g_raceRecords[pCar->index].field_0x14 = (short)value;
        if (pCar->index < 2)
            g_routeProbeCycles[pCar->index] = 0;
    }
}

// The original cursor is biased to field_0x8; use the owned record layout.
// Restores the eight cars' race records before reinitializing their routes.
// FUNCTION: CMR2 0x004207f0
void RallyData_RestoreAllCarRaceRecords(void)
{
    // 64BIT: BYTE*/record-field variants change MSVC6 scheduling. This local
    // numeric cursor retains the original unsigned-int type via size_t,
    // which is pointer-wide on both supported ABIs (checked in RaceRecord.h).
    size_t recordAddress = (size_t)&g_raceRecords[0].field_0x8;
    do {
        *(int *)recordAddress = 0;
        *(int *)(recordAddress - offsetof(RaceRecord, field_0x8)) = *(int *)(recordAddress - (offsetof(RaceRecord, field_0x8) - offsetof(RaceRecord, resetRouteNodeIndex)));
        *(short *)(recordAddress + (offsetof(RaceRecord, field_0x14) - offsetof(RaceRecord, field_0x8))) = 0;
        *(int *)(recordAddress + (offsetof(RaceRecord, field_0xc) - offsetof(RaceRecord, field_0x8))) = 0;
        recordAddress += sizeof(g_raceRecords[0]);
    } while ((INT_PTR)recordAddress < (INT_PTR)(g_raceRecords + 8) + (int)offsetof(RaceRecord, field_0x8));
    RallyData_ResetPlayerRouteProbes();
}

// FUNCTION: CMR2 0x00420820
void RallyData_ResetPlayerRouteProbes(void)
{
    int i;

    *(int *)g_routeProbeIndex = 0;
    *(int *)g_routeProbeBestIndex = 0;
    for (i = 0; i < 2; i++) {
        g_routeProbeCycles[i] = 0;
        g_routeProbeBestDistance[i] = 0;
    }
}

void RallyData_GetRouteNodeGroundPosition(int index, int *pOut);
unsigned int RallyData_GetSelectionFlag27(void);
void RallyData_ProjectCarRouteSegmentProgress(Car *pCar, int *pProgress);

// Keep the two horizontal components in range before the original fixed point
// length and reciprocal scale helpers run.
#define ROUTE_NORMALIZE(v) do { \
    if (FIX_ABS((v).x) > 0x640000 || FIX_ABS((v).z) > 0x640000) { \
        (v).x /= 512; \
        (v).z /= 512; \
    } \
    int length = FixVecLength(&(v)); \
    if (length == 0) { \
        (v).x = 0; (v).y = 0; (v).z = 0; \
    } else { \
        FixVecScaleRecip(&(v), &(v), length); \
    } \
} while (0)

// Tracks a car along the stage route. Local players periodically probe nearby
// nodes; then the forward and previous segment tests advance or retreat the
// route index and update progress along that segment.
// match 46%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00420a30
void RallyData_UpdateCarRoute(Car *pCar)
{
    int slot = (signed char)pCar->index;
    RaceRecord *record = &g_raceRecords[slot];
    FixVector point;
    FixVector other;
    FixVector delta;
    FixVector scaled;
    FixVector routeDir;
    int node;
    int previous;
    int i;
    int length;
    unsigned int distance;

    if (g_routeNodeCount == 0)
        return;

    if (slot < (int)(RallyDataState() & 0xff) && RallyData_GetFlag25() == 0 && RallyData_GetFlag24() == 0) {
        for (i = 0; i < 25; i++) {
            short probe = (short)g_routeProbeIndex[slot];
            RallyData_GetRouteNodeGroundPosition((unsigned short)probe, (int *)&point);
            delta.x = point.x - pCar->position.x;
            delta.y = 0;
            delta.z = point.z - pCar->position.z;
            FixVecScale(&scaled, &delta, 0x28f);
            distance = (unsigned int)FixVecDot(&scaled, &scaled);
            if (distance <= g_routeProbeBestDistance[slot] || probe == g_routeProbeBestIndex[slot]) {
                g_routeProbeBestDistance[slot] = distance;
                g_routeProbeBestIndex[slot] = probe;
            }
            g_routeProbeIndex[slot] = (unsigned short)(probe + 1);
            if (g_routeProbeIndex[slot] == (unsigned int)g_unk0x00538a88)
                g_routeProbeIndex[slot] = 0;
        }
        if (++g_routeProbeCycles[slot] > 25) {
            record->routeNodeIndex = (unsigned short)g_routeProbeBestIndex[slot];
            g_routeProbeCycles[slot] = 0;
        }
    }

    node = record->routeNodeIndex;
    record->field_0x14 = (short)node;
    RallyData_GetRouteNodeGroundPosition(node, (int *)&point);
    delta.x = point.x - pCar->position.x;
    delta.y = 0;
    delta.z = point.z - pCar->position.z;
    ROUTE_NORMALIZE(delta);
    RallyRoute_GetNodeDirection(&routeDir, node);

    if (FixVecDot(&delta, &routeDir) < 0) {
        if (g_unk0x00538a94 == 0) {
            if ((unsigned int)node < (unsigned int)(g_unk0x00538a88 - 1)) {
                record->field_0x14++;
                record->routeNodeIndex++;
                record->field_0x8++;
            } else {
                record->field_0x14++;
                record->field_0x8 = g_routeNodeCount + 1;
            }
        } else if ((unsigned int)node < (unsigned int)(g_routeNodeCount - 1)) {
            record->field_0x14++;
            record->routeNodeIndex++;
            record->field_0x8++;
        } else {
            record->routeNodeIndex = 0;
            record->field_0x14 = 0;
            record->field_0x8++;
        }
        record->field_0xc = 0;
        if (slot < (int)(RallyDataState() & 0xff) && (BYTE)RallyData_GetSelectionFlag27() == 0)
            g_routeProbeCycles[slot] = 0;
        RallyData_ProjectCarRouteSegmentProgress(pCar, (int *)record);
        return;
    }

    if (g_unk0x00538a94 != 0 || node != 0) {
        previous = node;
        if (previous == 0)
            previous = g_routeNodeCount;
        previous--;
        RallyData_GetRouteNodeGroundPosition(previous, (int *)&point);
        delta.x = point.x - pCar->position.x;
        delta.y = 0;
        delta.z = point.z - pCar->position.z;
        ROUTE_NORMALIZE(delta);
        RallyRoute_GetNodeDirection(&routeDir, previous);
        if (FixVecDot(&delta, &routeDir) > 0) {
            record->routeNodeIndex = previous;
            record->field_0x14 = (short)previous;
            if (g_unk0x00538a94 == 0 && (unsigned int)previous == (unsigned int)(g_routeNodeCount - 2))
                record->field_0x8 = previous;
            else
                record->field_0x8--;
            RallyData_GetRouteNodeGroundPosition(node, (int *)&point);
            RallyData_GetRouteNodeGroundPosition(previous, (int *)&other);
            delta.x = point.x - other.x;
            delta.y = 0;
            delta.z = point.z - other.z;
            if (FIX_ABS(delta.x) > 0x640000 || FIX_ABS(delta.z) > 0x640000) {
                delta.x /= 512;
                delta.z /= 512;
                length = FixVecLength(&delta) << 9;
            } else {
                length = FixVecLength(&delta);
            }
            record->field_0xc += length;
        }
    }
    RallyData_ProjectCarRouteSegmentProgress(pCar, (int *)record);
}

#undef ROUTE_NORMALIZE

// FUNCTION: CMR2 0x00421370
int RallyData_GetActiveCarRaceRecordField0(Car *p)
{
    if (g_routeNodeCount == 0)
        return 0;
    return g_raceRecords[p->index].routeNodeIndex;
}

// FUNCTION: CMR2 0x004213a0
short RallyData_GetActiveCarRaceRecordField14(Car *p)
{
    if (g_routeNodeCount == 0)
        return 0;
    return g_raceRecords[p->index].field_0x14;
}

// FUNCTION: CMR2 0x00421420
int RallyData_GetRouteAvailabilityState(void)
{
    return g_routeNodeCount;
}

// FUNCTION: CMR2 0x00421430
int RallyData_GetRouteStateValue(void)
{
    return g_unk0x00538a88;
}

// FUNCTION: CMR2 0x00421440
RallyRouteNode *RallyData_GetAvailableRouteNodeRecord(int index)
{
    if (g_routeNodeCount == 0)
        return NULL;
    return g_routeNodes + index;
}

int RallyData_GetRouteAdditionalState(void);
void RallyData_GetRouteNodeGroundPosition(int index, int *pOut);

// Progress of the car along the route segment that ends at node pProgress[0]:
// the car position projected on the segment direction, as a 16.16 fraction
// of the segment length (stored in pProgress[4]).
// FUNCTION: CMR2 0x00421230
void RallyData_ProjectCarRouteSegmentProgress(Car *pCar, int *pProgress)
{
    FixVector dir;
    FixVector end;
    FixVector start;
    FixVector delta;
    int node;
    int prev;
    int length;
    int along;

    node = *pProgress;
    prev = node - 1;
    if (prev < 0)
        prev += g_routeNodeCount;
    if (!RallyData_GetRouteAdditionalState() && node <= 0) {
        pProgress[4] = 0;
        return;
    }
    RallyRoute_GetNodeDirection(&dir, prev);
    RallyData_GetRouteNodeGroundPosition(node, (int *)&end);
    RallyData_GetRouteNodeGroundPosition(prev, (int *)&start);
    delta.x = end.x - start.x;
    delta.y = end.y - start.y;
    delta.z = end.z - start.z;
    length = FixVecDot(&delta, &dir);
    delta.x = pCar->position.x - start.x;
    delta.y = pCar->position.y - start.y;
    delta.z = pCar->position.z - start.z;
    along = FixVecDot(&delta, &dir);
    pProgress[4] = FixDiv(along, length);
}

// FUNCTION: CMR2 0x00421500
int RallyData_GetRouteAdditionalState(void)
{
    return g_unk0x00538a94;
}

// FUNCTION: CMR2 0x00421510
RaceRecord *RallyData_GetCarRaceRecord(int index)
{
    return &g_raceRecords[index];
}

// FUNCTION: CMR2 0x00421530
void RallyData_GetRouteNodeGroundPosition(int index, int *pOut)
{
    int x = g_routeNodes[index].position.x;
    pOut[1] = 0;
    pOut[0] = x;
    pOut[2] = g_routeNodes[index].position.z;
}
// GLOBAL: CMR2 0x00538c94
int g_unk0x00538c94;

// FUNCTION: CMR2 0x004209d0
int RallyData_GetCarRaceRecordField10(Car *p)
{
    return g_raceRecords[p->index].field_0x10;
}

// Scales the per-record value at p[0xb1a] to a 0..0x10000 ratio.
// FUNCTION: CMR2 0x00421470
int RallyData_GetCarRecordValueRatio(Car *p)
{
    int value;
    int result;

    if (g_routeNodeCount == 0)
        return 0;
    if (g_unk0x00538a94 != 0) {
        value = g_raceRecords[p->index].field_0x8;
        if (value >= (int)(RallyData_GetSelectionBits16To19() & 0xff) * g_routeNodeCount)
            return 0x10000;
    } else {
        value = g_raceRecords[p->index].field_0x8;
        if (value >= g_routeNodeCount)
            return 0x10000;
    }
    result = (value << 16) / (g_unk0x00538c94 >> 16);
    if (result < 0)
        return 0;
    if (result > 0x10000)
        return 0x10000;
    return result;
}

// Per-category records of 0x650 bytes (runs up to the 0x531350 table).

// Record of the selected entry: the 196-byte entry in championship mode
// (mode 4), otherwise the 0x650-byte category record (NULL when the
// category nibble is 15).
// FUNCTION: CMR2 0x00408400
void *RallyData_GetRecord(BYTE index)
{
    unsigned int category;

    if (CGameInfo::GetConfiguredGameMode() == 4)
        return (BYTE *)&g_saveCarRecords[index];
    RallyData_ValidateIndex(index);
    category = g_saveSlots[index].category;
    if (category != 0xf)
        return (BYTE *)g_saveProfiles[category].identifier.name;
    return NULL;
}

// Returns the 4-bit category of the record, or -1 when it is not usable.
// FUNCTION: CMR2 0x00408500
char RallyData_GetUsableRecordCategory(BYTE param1)
{
    unsigned int index;

    if (CGameInfo::GetConfiguredGameMode() == 4) {
        index = param1 & 0xff;
        // The record is compared before the index is checked: for the
        // knockout's computer drivers (8 and over) it reads past the eight
        // records, then ignores the result.
        // PORT: the index first.
        if ((BYTE)param1 < 8 && strcmp(g_saveCarRecords[index].name, CMain::m_logFileBlankLine) != 0)
            return -1;
        return (char)((*(unsigned int *)&g_saveSlots[index] >> 0xe) & 0xf);
    }
    if ((*(unsigned int *)&g_saveSlots[(param1 & 0xff)] & 0x2000) != 0)
        return -1;
    return (char)((*(unsigned int *)&g_saveSlots[(param1 & 0xff)] >> 0xe) & 0xf);
}

// Returns bit 26 of the selected rally data (a per-rally flag).
// FUNCTION: CMR2 0x00407e70
unsigned char RallyData_GetSelectionFlag26(void)
{
    return (g_selectedRallyData >> 26) & 1;
}

// Best results of the driver's profile, or NULL when the driver has none.
// FUNCTION: CMR2 0x00408cb0
CategoryBestResults *RallyData_GetDriverBestResults(int index)
{
    unsigned int value;

    RallyData_ValidateIndex(index);
    value = *(unsigned int *)&g_saveSlots[index];
    if ((value & 0x3c0000) != 0x3c0000)
        return &g_saveProfiles[(value >> 0x12) & 0xf].bestResults;
    return NULL;
}

// FUNCTION: CMR2 0x00407e90
unsigned int RallyData_GetSelectionFlag27(void)
{
    return (g_selectedRallyData >> 27) & 1;
}

// FUNCTION: CMR2 0x00407ea0
unsigned char RallyData_GetSelectionFlag28(void)
{
    return (g_selectedRallyData >> 28) & 1;
}

// Neutral type for now: nothing implemented reads it yet.
// GLOBAL: CMR2 0x00536be0
SceneNode *g_unk0x00536be0;
// GLOBAL: CMR2 0x00536be4
SceneNode *g_unk0x00536be4;

// FUNCTION: CMR2 0x0040eeb0
SceneNode *RallyData_GetChallengeSceneState(void)
{
    return g_unk0x00536be4;
}

// Tail-call thunk (the original compiles it to a bare jmp): clears the
// depth buffer before the on-stage 3D pass.
// FUNCTION: CMR2 0x0040eec0
BOOL RallyData_ClearStageDepthBuffer(void)
{
    return CGraphics::ClearZBuffer();
}

/* --------------------------------------------------------------------------
   Per-frame handlers of the rally-data menu screens (0x410c80..0x411020).
   -------------------------------------------------------------------------- */

static int g_debugCameraView=-1;
void DebugOverlays_RequestView(int view) { g_debugCameraView=view; }
void RallyData_UpdateViewFrameAndDebugOverlay(CallbackStateMachine *, int, int);
void RallyData_DrawViewEndFrameOverlay(CallbackStateMachine *, int);
void RallyData_DrawLoadingProgress(int progress, char drawScene, BYTE alpha);
void RallyData_DrawHeadToHeadDriverCaption(int, int);
void InRaceMenu_DrawActivePage(void);
SceneNode *RallyData_GetChallengeRenderState(void);
int Game_PrepareScene(SceneNode *, SceneNode *, void *, int);
int Game_DrawSceneViewport(struct SceneNode *pRoot, struct SceneNode *pCamera, void *pRect, int bit, BYTE flag);
void Graphics_PresentFrameAndResetCounters(void);
float Graphics_GetFrameRate(void);
void GameMenu_UpdateHeaderCallbacks(void);
BYTE Race_ReadTransitionState3811C(void);
extern char g_strFpsFormat0x00516e14[];
extern char g_strHardwareTl0x00516d54[];
extern char g_strSoftwareTl0x00516d40[];
// GLOBAL: CMR2 0x0053811e
BYTE g_unk0x0053811e;
// GLOBAL: CMR2 0x0053811f
BYTE g_unk0x0053811f;
// GLOBAL: CMR2 0x00516ce4
int g_unk0x00516ce4 = -1;

// Pumps the shared loading screen (three passes) of a challenge/menu stage:
// clears the target, prepares the scene, draws the optional FPS/T&L overlay and
// fades the loading text in and out.
// FUNCTION: CMR2 0x00410c80
void RallyData_RunChallengeLoadingScreen(int param1, char param2)
{
    short rect[4];
    BYTE colour[4];
    int i;

    // PORT: the loading screen laid out for the screen's size, as the menus.
    MenuScreen_Begin();
    rect[0] = 0;
    rect[1] = 0;
    rect[2] = *(short *)g_pGraphics;
    rect[3] = *(short *)((BYTE *)g_pGraphics + 4);
    if (RallyData_ShouldEndStageEarly() == 0 && param2 == 0) {
        CGraphics::SetClearColour(1, 0x9c, 0xb4, 0xac);
        i = 3;
        do {
            CGraphics::ClearTarget();
            CGraphics::ClearZBuffer();
            Game_PrepareScene(RallyData_GetChallengeRenderState(),
                              g_unk0x00536be4, rect, 0);
            if (Graphics_DebugDrawWanted()) {
                colour[0] = 0xff;
                colour[1] = 0xff;
                colour[2] = 0xff;
                colour[3] = 0xff;
                sprintf(CFrontend::m_stringDest, g_strFpsFormat0x00516e14, Graphics_GetFrameRate());
                Font_DrawText(0, CFrontend::m_stringDest, 0, 0, (int *)colour, 9);
                if (CGraphics::GetSelectedRenderDeviceSurfaceCaps() == 2)
                    sprintf(CFrontend::m_stringDest, "%s", g_strHardwareTl0x00516d54);
                else
                    sprintf(CFrontend::m_stringDest, "%s", g_strSoftwareTl0x00516d40);
                Font_DrawText(0, CFrontend::m_stringDest, 0, 0x5a, (int *)colour, 9);
            }
            RallyData_DrawFadedLoadingText(0xff);
            RallyData_DrawLoadingProgress(0, 0, 0xff);
            Game_DrawSceneViewport(RallyData_GetChallengeRenderState(), g_unk0x00536be4, rect, 0, 0);
            Graphics_PresentFrameAndResetCounters();
            RallyData_DrawLoadingProgress(0, 0, 0xff);
        } while (--i);
    }
    MenuScreen_End();
}

// Advances one step of the current rally-data menu entry: grows the panel and
// fires the item's action when the last step is reached.
// FUNCTION: CMR2 0x00410de0
void RallyData_GrowEntryAndInvokeAction(CallbackStateMachine *param1, unsigned int param2)
{
    Font_SetBlendMode(2);
    if (CGameInfo::GetConfiguredGameMode() != 4) {
        RallyData_UpdateViewFrameAndDebugOverlay(param1, param2, 1);
        RallyData_DrawViewEndFrameOverlay(param1, param2 & 0xff);
    }
}

// Advances one step of a rally-data menu entry and, once the panel is fully
// grown, asks the screen to refresh its records.
// FUNCTION: CMR2 0x00410e20
void RallyData_GrowEntryAndRefreshRecords(CallbackStateMachine *param1, unsigned int param2)
{
    RallyData_UpdateViewFrameAndDebugOverlay(param1, param2, 1);
    if ((BYTE)RallyData_GetSelectionFlag27() != 0) {
        if ((BYTE)CGameInfo::GetGameModeOptionBit19() == 0)
            RallyData_DrawHeadToHeadDriverCaption(0, param2 & 0xff);
    }
    RallyData_DrawViewEndFrameOverlay(param1, param2 & 0xff);
}

// Advances one step of a rally-data menu entry, always in "growing" mode.
// FUNCTION: CMR2 0x00410e70
void RallyData_GrowEntryPanel(CallbackStateMachine *param1, unsigned int param2)
{
    RallyData_UpdateViewFrameAndDebugOverlay(param1, param2, 1);
    RallyData_DrawViewEndFrameOverlay(param1, param2 & 0xff);
}

// Advances one step of a rally-data menu entry and notifies the screen when the
// entry reaches its last step.
// FUNCTION: CMR2 0x00410ea0
void RallyData_GrowEntryAndNotifyCompletion(CallbackStateMachine *param1, unsigned int param2)
{
    RallyData_UpdateViewFrameAndDebugOverlay(param1, param2, 1);
    if ((param2 & 0xff) == param1->count - 1)
        InRaceMenu_DrawActivePage();
    RallyData_DrawViewEndFrameOverlay(param1, param2 & 0xff);
}

// Advances one step of a rally-data menu entry while the entry's value is not
// available yet, and overlays the "no time" / "no best time" label when the
// screen has one pending.
// FUNCTION: CMR2 0x00410ee0
void RallyData_UpdateUnavailableEntryText(CallbackStateMachine *param1, unsigned int param2)
{
    BYTE colour[4];

    colour[0] = 0xff;
    colour[1] = 0xff;
    colour[2] = 0xff;
    colour[3] = 0xff;
    if ((BYTE)Race_ReadTransitionState3811C() < 100 || (BYTE)RallyDataState() == 1)
        RallyData_UpdateViewFrameAndDebugOverlay(param1, param2, 1);
    else
        RallyData_UpdateViewFrameAndDebugOverlay(param1, param2, 0);
    if (g_unk0x0053811e != 0) {
        Font_DrawText(2, CFrontend::GetTextString(0xf8), (int)g_pGraphics->resX / 2,
                      (int)g_pGraphics->resY / 2, (int *)colour, 0x12);
    } else if (g_unk0x0053811f != 0) {
        Font_DrawText(2, CFrontend::GetTextString(0xf9), (int)g_pGraphics->resX / 2,
                      (int)g_pGraphics->resY / 2, (int *)colour, 0x12);
    }
    RallyData_DrawViewEndFrameOverlay(param1, param2 & 0xff);
}

// Advances one step of a rally-data menu entry while shrinking it, and draws
// the "stage started" banner at the centre of the screen.
// FUNCTION: CMR2 0x00410fa0
void RallyData_ShrinkEntryAndDrawStageStarted(CallbackStateMachine *param1, unsigned int param2)
{
    RallyData_UpdateViewFrameAndDebugOverlay(param1, param2, 0);
    Font_DrawText(2, CFrontend::GetTextString(0x71),
                  (int)g_pGraphics->resX * 0x1e / 0x280,
                  (int)g_pGraphics->resY * 0x43 / 0x1e0,
                  &g_unk0x00516ce4, 0x11);
    RallyData_DrawViewEndFrameOverlay(param1, param2 & 0xff);
}

// Advances one step of the current menu entry in "shrinking" mode and notifies
// the screen when the entry reaches its last step.
// FUNCTION: CMR2 0x00411020
void RallyData_ShrinkEntryAndNotifyCompletion(CallbackStateMachine *param1, unsigned int param2)
{
    RallyData_UpdateViewFrameAndDebugOverlay(param1, param2, 0);
    if ((param2 & 0xff) == param1->count - 1)
        GameMenu_UpdateHeaderCallbacks();
    RallyData_DrawViewEndFrameOverlay(param1, param2 & 0xff);
}

// FUNCTION: CMR2 0x00411060
SceneNode *RallyData_GetChallengeRenderState(void)
{
    return g_unk0x00536be0;
}

// FUNCTION: CMR2 0x004082e0
unsigned int RallyData_GetSetupFlag11(void)
{
    return (g_unk0x0052f2b0 >> 0xb) & 1;
}

// FUNCTION: CMR2 0x004082b0
unsigned int RallyData_GetSetupModeBits(void)
{
    return g_unk0x0052f2b0 & 7;
}

// Table of 0x148-byte records; count derived from the next known global
// (0x52f2a9), so it may cover further undeclared values.

// FUNCTION: CMR2 0x00407610
RallyCarDamageState *RallyData_GetDriverGroupRecord(int index)
{
    return &g_driverGroupTable.groups[index].damage;
}

// GLOBAL: CMR2 0x00516cd0
BYTE g_itemColour[4] = { 255, 255, 255, 255 };
// GLOBAL: CMR2 0x00536bd8
short g_itemRect[4];
// GLOBAL: CMR2 0x00516cd4
BYTE g_loadBarColourLit[4] = { 0xbd, 0xb6, 0xbd, 0xff };
// GLOBAL: CMR2 0x00516cd8
BYTE g_unk0x00516cd8[4] = { 0x00, 0x00, 0x00, 0xff };
// GLOBAL: CMR2 0x00516cdc
BYTE g_loadBarColour[4] = { 0xff, 0xff, 0xff, 0xff };

// Builds the frontend scene: root node, camera node and projection.
// FUNCTION: CMR2 0x0040ef10
void RallyData_CreateFrontendChallengeScene(void)
{
    FixVector position;
    FixAngles angles;
    SceneNode *pNode;

    position.x = 0;
    position.y = 0;
    position.z = -0xa0000;
    angles.x = 0;
    angles.y = 0;
    angles.z = 0;
    angles.pad = 0;
    g_unk0x00536be0 = SceneNode_CreateRoot();
    g_unk0x00536be4 = SceneType2_Create(&position, &angles, NULL, g_unk0x00536be0);
    for (pNode = g_unk0x00536be0; pNode != NULL; pNode = pNode->pParent)
        pNode->dirty = 1;
    CGraphics::SetProjection(0x25645, 0x4326e, 0xfa0000, 0x10000);
}

// Draws one item of a horizontal list and, unless it is the last one, the thin
// separator after it; returns the x the next item starts at.
// FUNCTION: CMR2 0x0040fd30
int RallyData_DrawListItem(int x, int y, char *pText, char last, BYTE alpha)
{
    BYTE colour[4];
    int px;

    px = x;
    colour[0] = g_itemColour[0];
    colour[1] = g_itemColour[1];
    colour[2] = g_itemColour[2];
    colour[3] = alpha;
    Font_DrawText(2, pText, px, y, (int *)colour, 0x11);
    if (last == 0) {
        px += Font_GetTextWidth(2, (BYTE *)pText) + (int)(g_pGraphics->resX * 10) / 0x280;
        g_itemRect[0] = (short)px;
        g_itemRect[1] = (short)((int)(g_pGraphics->resY * 200) / 0x1e0);
        g_itemRect[2] = 2;
        g_itemRect[3] = (short)((int)(g_pGraphics->resY * 0x3c) / 0x1e0);
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_itemRect, colour, 1);
        px += (int)(g_pGraphics->resX * 10) / 0x280;
    }
    return px;
}

int RallyData_ShouldEndStageEarly(void);
SceneNode *RallyData_GetChallengeRenderState(void);
int Game_DrawSceneViewport(struct SceneNode *pRoot, struct SceneNode *pCamera, void *pRect, int bit, BYTE flag);
void Graphics_PresentFrameAndResetCounters(void);

// Draws the loading bar: eleven blocks, lit up to the given progress (0-99, 100
// or more draws them all unlit), and optionally renders the frontend scene.
// FUNCTION: CMR2 0x0040fec0
void RallyData_DrawLoadingProgress(int progress, char drawScene, BYTE alpha)
{
    BYTE colour[4];
    BYTE colourLit[4];
    int i;
    int limit;
    int wide;
    int narrow;

    colour[0] = g_loadBarColour[0];
    colour[1] = g_loadBarColour[1];
    colour[2] = g_loadBarColour[2];
    colour[3] = alpha;
    colourLit[0] = g_loadBarColourLit[0];
    colourLit[1] = g_loadBarColourLit[1];
    colourLit[2] = g_loadBarColourLit[2];
    colourLit[3] = alpha;
    if (RallyData_ShouldEndStageEarly() != 0)
        return;
    // PORT: the loading screen laid out for the screen's size, as the menus.
    if (drawScene)
        MenuScreen_Begin();
    i = 0;
    limit = 100;
    do {
        wide = (int)(g_pGraphics->resX * 8) / 640;
        narrow = (int)(g_pGraphics->resX * 3) / 640;
        g_itemRect[0] = (short)((int)(g_pGraphics->resX * 616) / 640 + (wide + narrow) * i - narrow * 10 - wide * 11);
        g_itemRect[1] = (short)((int)(g_pGraphics->resY * 434) / 480);
        g_itemRect[2] = (short)((int)(g_pGraphics->resX * 8) / 640);
        g_itemRect[3] = (short)((int)(g_pGraphics->resX * 8) / 640);
        if (progress < 100 && limit / 11 >= progress)
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_itemRect, colourLit, 1);
        else
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_itemRect, colour, 1);
        i++;
        limit += 100;
    } while (limit < 1200);
    if (drawScene) {
        short rect[4];

        rect[0] = 0;
        rect[1] = 0;
        rect[2] = (short)g_pGraphics->resX;
        rect[3] = (short)g_pGraphics->resY;
        Game_DrawSceneViewport(RallyData_GetChallengeRenderState(), g_unk0x00536be4, rect, 0, 0);
        Graphics_PresentFrameAndResetCounters();
        MenuScreen_End();
    }
}


// FUNCTION: CMR2 0x0040df60
void RallyData_SetDriverPairingStateValues(int param1, int param2)
{
    g_driverGroupTable.field_0x6d0 = param1;
    g_driverGroupTable.field_0x53c = param2;
}

unsigned int RallyData_GetKnockoutCountryNibble(void);

// FUNCTION: CMR2 0x004070c0
void RallyData_SelectKnockoutCountryStage(void)
{
    unsigned int value = RallyData_GetKnockoutCountryNibble() & 0xff;

    RallyData_SetCountrySelectionBits((BYTE)value);
    RallyData_SetStageSelectionAndRefreshFlags(10);
}

// FUNCTION: CMR2 0x004070e0
unsigned int RallyData_GetKnockoutCountryNibble(void)
{
    return g_knockout.state >> 16 & 0xf;
}

// FUNCTION: CMR2 0x00407500
void RallyData_SetSelectionBits14To15(BYTE param1)
{
    g_selectedRallyData = ((param1 & 3) << 14) | (g_selectedRallyData & 0xffff3fffU);
}

// FUNCTION: CMR2 0x00407800
void RallyData_SetSelectionFlag20(unsigned int param1)
{
    g_selectedRallyData = ((param1 & 1) << 20) | (g_selectedRallyData & 0xffefffffU);
}

// FUNCTION: CMR2 0x00407e20
unsigned int RallyData_GetFlag21(void)
{
    return g_selectedRallyData >> 21 & 1;
}

// FUNCTION: CMR2 0x00407e30
unsigned int RallyData_GetFlag22(void)
{
    return g_selectedRallyData >> 22 & 1;
}

// FUNCTION: CMR2 0x00407e40
unsigned int RallyData_GetFlag23(void)
{
    return g_selectedRallyData >> 23 & 1;
}

// FUNCTION: CMR2 0x00407e80
unsigned int RallyData_GetFlag31(void)
{
    return g_selectedRallyData >> 31;
}

// FUNCTION: CMR2 0x00407eb0
unsigned int RallyData_GetFlag30(void)
{
    return g_selectedRallyData >> 30 & 1;
}

// FUNCTION: CMR2 0x00408290
void RallyData_ClearDriverGroupRecords(void)
{
    memset(g_driverGroupTable.groups, 0, 4 * sizeof(DriverCarRecord));
}

// FUNCTION: CMR2 0x004082c0
unsigned int RallyData_GetSetupHighNibble(void)
{
    return g_unk0x0052f2b0 >> 7 & 0xf;
}

// FUNCTION: CMR2 0x004082d0
unsigned int RallyData_GetSetupLowNibble(void)
{
    return g_unk0x0052f2b0 >> 3 & 0xf;
}

// FUNCTION: CMR2 0x004082f0
BYTE *RallyData_GetStageAvailabilityFlags(void)
{
    return g_unk0x0052ea68;
}

// FUNCTION: CMR2 0x004083d0
RallyDriverGroupTable *RallyData_GetDriverGroupTable(void)
{
    return &g_driverGroupTable;
}

// FUNCTION: CMR2 0x004084c0
void RallyData_ResetDriverRecordCategory(BYTE index, BYTE param2)
{
    *(unsigned int *)&g_saveSlots[index] =
        ((param2 & 0xf) << 14) | (*(unsigned int *)&g_saveSlots[index] & 0xfffc0000U);
    g_saveSlots[index].time = 0;
}

// FUNCTION: CMR2 0x0040df80
void RallyData_SetDistinctValueEntry(int index, int value)
{
    g_unk0x0052ea98[index] = value;
}

// GLOBAL: CMR2 0x005337c8
int g_unk0x005337c8;
// GLOBAL: CMR2 0x005337d0
int g_unk0x005337d0;
// GLOBAL: CMR2 0x005337ec
int g_unk0x005337ec;
// The event table's slot 16 holds the event the other five slots are built from.
extern int g_unk0x00533758[21];

// Fills the six per-slot values of the current event: a fixed set when the
// event's group is one of the special ones, otherwise the events picked from
// the event's group avoiding the previous one.
// FUNCTION: CMR2 0x0040dfa0
void RallyData_FillEventSlotSelections(void)
{
    if (!RallyData_GetSecondarySelectionNibble())
        return;
    g_unk0x005337ec = CFrontend::GetArchivePrimaryIDEntry(RallyData_GetDriverRecordSelectionValue(0));
    g_unk0x00533758[16] = RallyData_GetDriverRecordSelectionValue(0);
    if (g_unk0x005337ec >= 0 && g_unk0x005337ec <= 5 || g_unk0x005337ec == 0xc) {
        if (CGameInfo::GetGameInfoSessionFlag()) {
            RallyData_SetDistinctValueEntry(0, 0);
            RallyData_SetDistinctValueEntry(3, 0);
            RallyData_SetDistinctValueEntry(1, 8);
            RallyData_SetDistinctValueEntry(4, 8);
            RallyData_SetDistinctValueEntry(2, 10);
            RallyData_SetDistinctValueEntry(5, 10);
            return;
        }
        g_unk0x005337d0 = 0;
        RallyData_SetDistinctValueEntry(0, g_unk0x00533758[16]);
        RallyData_SetDistinctValueEntry(3, g_unk0x00533758[16]);
        g_unk0x005337c8 = RallyData_PickStageOutsideExcludedGroups(g_unk0x00533758[16], -1);
        RallyData_SetDistinctValueEntry(1, g_unk0x005337c8);
        RallyData_SetDistinctValueEntry(4, g_unk0x005337c8);
        g_unk0x005337c8 = RallyData_PickStageOutsideExcludedGroups(g_unk0x00533758[16], g_unk0x005337c8);
    } else {
        if (g_unk0x005337ec == 8) {
            RallyData_SetDistinctValueEntry(0, 0x10);
            RallyData_SetDistinctValueEntry(3, 0x10);
            RallyData_SetDistinctValueEntry(1, 0x10);
            RallyData_SetDistinctValueEntry(4, 0x10);
            RallyData_SetDistinctValueEntry(2, 0x10);
            RallyData_SetDistinctValueEntry(5, 0x10);
            return;
        }
        if (g_unk0x005337ec == 0xd) {
            RallyData_SetDistinctValueEntry(0, 0x15);
            RallyData_SetDistinctValueEntry(3, 0x15);
            RallyData_SetDistinctValueEntry(1, 0x15);
            RallyData_SetDistinctValueEntry(4, 0x15);
            RallyData_SetDistinctValueEntry(2, 0x15);
            RallyData_SetDistinctValueEntry(5, 0x15);
            return;
        }
        RallyData_SetDistinctValueEntry(0, g_unk0x00533758[16]);
        RallyData_SetDistinctValueEntry(3, g_unk0x00533758[16]);
        g_unk0x005337c8 = RallyData_PickUnexcludedGroupEvent(g_unk0x00533758[16], -1);
        RallyData_SetDistinctValueEntry(1, g_unk0x005337c8);
        RallyData_SetDistinctValueEntry(4, g_unk0x005337c8);
        g_unk0x005337c8 = RallyData_PickUnexcludedGroupEvent(g_unk0x00533758[16], g_unk0x005337c8);
    }
    RallyData_SetDistinctValueEntry(2, g_unk0x005337c8);
    RallyData_SetDistinctValueEntry(5, g_unk0x005337c8);
}

// GLOBAL: CMR2 0x005337c4
int g_unk0x005337c4;
// GLOBAL: CMR2 0x005337d4
int g_unk0x005337d4[6];

// Picks a random stage (0..11) whose group is not one of the two excluded ones.
// FUNCTION: CMR2 0x0040e180
int RallyData_PickStageOutsideExcludedGroups(int exclude1, int exclude2)
{
    memset(g_unk0x005337d4, 0, sizeof(g_unk0x005337d4));
    // PORT: the table has the six teams the stages are picked from, but the
    // player's car can be the Ford Racing Puma (id 12): the original writes
    // its entry past the table, over memory nothing uses there; here it would
    // land on another global. Only the six entries are read back.
    if (exclude1 >= 0 && CFrontend::GetArchivePrimaryIDEntry(exclude1) < 6)
        g_unk0x005337d4[CFrontend::GetArchivePrimaryIDEntry(exclude1)] = 1;
    if (exclude2 >= 0 && CFrontend::GetArchivePrimaryIDEntry(exclude2) < 6)
        g_unk0x005337d4[CFrontend::GetArchivePrimaryIDEntry(exclude2)] = 1;
    g_unk0x005337c4 = rand() % 12;
    while (g_unk0x005337d4[CFrontend::GetArchivePrimaryIDEntry(g_unk0x005337c4)] == 1)
        g_unk0x005337c4 = rand() % 12;
    return g_unk0x005337c4;
}

// FUNCTION: CMR2 0x0040e330
void RallyData_SetSetupFlag11(char param1)
{
    if (param1 != 0) {
        g_unk0x0052f2b0 |= 0x800;
        return;
    }
    g_unk0x0052f2b0 &= ~0x800;
}

// FUNCTION: CMR2 0x0040e360
void RallyData_SetSetupModeBits(unsigned int param1)
{
    g_unk0x0052f2b0 = g_unk0x0052f2b0 ^ (g_unk0x0052f2b0 ^ param1) & 7;
}

// FUNCTION: CMR2 0x0040e380
void RallyData_SetSetupHighNibble(unsigned int param1)
{
    g_unk0x0052f2b0 = ((param1 & 0xf) << 7) | (g_unk0x0052f2b0 & 0xfffff87fU);
}

// FUNCTION: CMR2 0x0040e3a0
void RallyData_SetSetupLowNibble(unsigned int param1)
{
    g_unk0x0052f2b0 = ((param1 & 0xf) << 3) | (g_unk0x0052f2b0 & 0xffffff87U);
}

// GLOBAL: CMR2 0x0051681c
BYTE g_unk0x0051681c[15] = { 0x00, 0x09, 0x0a, 0x08, 0x04, 0x04, 0x09, 0x00,
                             0x0a, 0x0b, 0x08, 0x0b, 0x04, 0x00, 0x0a };

// Row/slot of the driver select screen for one list index: the rally modes
// (5..7) only have six slots and ask the AI helper for the row, everything
// else uses a 15-entry lookup table.
// FUNCTION: CMR2 0x00407fc0
BYTE RallyData_GetDriverSelectGridSlot(int param1)
{
    if (CGameInfo::GetConfiguredGameMode() == 5 || CGameInfo::GetConfiguredGameMode() == 6 ||
        CGameInfo::GetConfiguredGameMode() == 7) {
        if (param1 >= 0 && param1 < 6)
            return StageTiming_GetStartSlotIndex(param1);
        return 0;
    }
    if (param1 < 0)
        return 0;
    if (param1 >= 0xf)
        return 0;
    return g_unk0x0051681c[param1];
}

// FUNCTION: CMR2 0x00408010
int RallyData_GetDistinctValueEntry(int index)
{
    return g_unk0x0052ea98[index];
}

// Builds the sorted list of distinct values from the 30 entries of the table
// at 0x4075f0 (shifting at most the first 9).
// FUNCTION: CMR2 0x004081d0
void RallyData_RebuildDistinctValueList(void)
{
    int *pValues;
    int remaining;
    int value;
    int pos;
    int i;

    pValues = (int *)RallyData_GetSelectedCountryValueTable();
    g_unk0x0052eab8 = 1;
    g_unk0x0052ea74[0] = *pValues;
    for (remaining = 0; remaining < 30; remaining++) {
        value = pValues[remaining];
        pos = -1;
        for (i = 0; i < g_unk0x0052eab8; i++) {
            if (value <= g_unk0x0052ea74[i]) {
                pos = i;
                i = g_unk0x0052eab8;
            }
        }
        if (pos == -1) {
            g_unk0x0052ea74[g_unk0x0052eab8] = value;
            g_unk0x0052eab8++;
        } else if (g_unk0x0052ea74[pos] != value) {
            for (i = 7; i >= pos; i--)
                g_unk0x0052ea74[i + 1] = g_unk0x0052ea74[i];
            g_unk0x0052ea74[pos] = value;
            g_unk0x0052eab8++;
        }
    }
}

// FUNCTION: CMR2 0x00408270
BYTE *RallyData_GetDistinctValueList(void)
{
    return (BYTE *)g_unk0x0052ea74;
}

// FUNCTION: CMR2 0x00408280
int RallyData_GetDistinctValueCount(void)
{
    return g_unk0x0052eab8;
}

void CarLight_InvalidateAppliedLevels(void);
void StageObject_ResetBodyTextureCaches(void);
void StageObject_FreeAll(void);
void Mesh_FreeClones(void);

// FUNCTION: CMR2 0x00411110
void RallyData_ResetDamageAndTextureCaches(void)
{
    CarLight_InvalidateAppliedLevels();
    StageObject_ResetBodyTextureCaches();
}

// Frees the stage objects, the stage root node and the cloned meshes.
// FUNCTION: CMR2 0x0040eef0
BYTE RallyData_FreeChallengeSceneObjects(void)
{
    StageObject_FreeAll();
    if (g_unk0x00536be0 != 0)
        SceneNode_Destroy(g_unk0x00536be0);
    Mesh_FreeClones();
    return 1;
}

short *Car_GetOrder(void);
short Car_GetOrderCount(void);

// Updates the route of every car, last in the race order first.
// FUNCTION: CMR2 0x004209f0
void RallyData_UpdateOrderedCarRoutes(void)
{
    short *pOrder;
    int i;

    pOrder = Car_GetOrder();
    i = Car_GetOrderCount() - 1;
    if (i >= 0) {
        pOrder += i;
        int remaining = i + 1;
        do {
            RallyData_UpdateCarRoute(Car_Get(*pOrder));
            pOrder--;
            remaining--;
        } while (remaining != 0);
    }
}

// Whether flag bit `bit` is set for the entry that pEntry points into.
// FUNCTION: CMR2 0x00471d40
int RallyData_IsElementFlagSet(SectorObjectEntry **pEntry, int bit)
{
    ptrdiff_t index;
    BYTE mask;

    index = (size_t)((UINT_PTR)*pEntry - (UINT_PTR)g_movingSectorEntries) / sizeof(SectorObjectEntry);
    if (index < (int)g_unk0x0058ca6c && index >= 0) {
        mask = 1 << bit;
        return (g_sectorEntryCarFlags[index] & mask) != 0;
    }
    return 0;
}

// Sets or clears bit `bit` of the flag byte of the 8-byte element *pp points at.
// FUNCTION: CMR2 0x00471d80
void RallyData_SetElementFlagBit(SectorObjectEntry **pp, int bit, int set)
{
    ptrdiff_t index = (size_t)((UINT_PTR)*pp - (UINT_PTR)g_movingSectorEntries) / sizeof(SectorObjectEntry);
    BYTE mask;

    if (index < (int)g_unk0x0058ca6c && index >= 0) {
        mask = 1 << bit;
        if (set == 0) {
            g_sectorEntryCarFlags[index] &= ~mask;
            return;
        }
        g_sectorEntryCarFlags[index] |= mask;
    }
}

unsigned char RallyDataState(void);

// Marks the element as reached by the car; the first time, in single player,
// pushes its object out of the way.
// FUNCTION: CMR2 0x00470240
void RallyData_MarkElementReachedByCar(SectorObjectEntry *pElement, int car)
{
    if (RallyData_IsElementFlagSet(&pElement, car) == 0) {
        RallyData_SetElementFlagBit(&pElement, car, 1);
        if (Car_Get(car)->isGhost == 0 && (BYTE)RallyDataState() == 1) {
            pElement->pObject->position.y += 0x3e80000;
            pElement->pObject->visible = 0xff;
        }
    }
}

// Row of the 7-byte table g_unk0x00520128 for the current country.
// FUNCTION: CMR2 0x00494a70
BYTE *RallyData_GetCountrySettingSevenByteRecord(void)
{
    int n;

    switch ((BYTE)RallyDataCountryIndex()) {
    case 1:
    case 2:
        n = 0;
        break;
    case 3:
        n = 1;
        break;
    case 5:
        n = 2;
        break;
    case 7:
        n = 3;
        break;
    case 8:
        n = 4;
        break;
    default:
        n = 0;
        break;
    }
    return g_unk0x00520128 + n * 7;
}

unsigned int StageObject_GetLoadedVariantDataAndState(SectorObjectEntry **pOut);
void StageObject_ResetMovingObjectCountsAndPhases(void);

// Marks every element as reached by every car (start of a replay).
// FUNCTION: CMR2 0x004702a0
void RallyData_MarkAllElementsReached(void)
{
    SectorObjectEntry *pEntries;
    int count;
    int car;
    int i;

    count = StageObject_GetLoadedVariantDataAndState(&pEntries);
    for (car = 0; car < Car_GetOrderCount(); car++)
        for (i = 0; i < count; i++)
            RallyData_MarkElementReachedByCar(pEntries + i, car);
    StageObject_ResetMovingObjectCountsAndPhases();
}

// Sets bit `bit` of the driver's category award mask; returns 0 when the
// driver has no category or already had it.
// FUNCTION: CMR2 0x00409010
BYTE RallyData_SetCategoryAwardBit(int index, int bit)
{
    unsigned int category;
    unsigned int mask;

    RallyData_ValidateIndex(index);
    if ((*(unsigned int *)&g_saveSlots[index] & 0x3c0000) == 0x3c0000)
        return 0;
    category = g_saveSlots[index].category;
    mask = 1 << bit;
    if ((mask & g_saveProfiles[category].awards) != 0)
        return 0;
    g_saveProfiles[category].awards |= mask;
    g_saveDirty[category] = 1;
    RallyData_IncrementCategoryCounter8(index);
    return 1;
}

// Records the championship medal (final place 0..2) of the driver's category at
// the current difficulty if it beats the stored one.
// FUNCTION: CMR2 0x00409090
void RallyData_RecordChampionshipMedal(int index, int place)
{
    CategoryMedals *pMedals;
    unsigned int medal;

    RallyData_ValidateIndex(index);
    pMedals = (CategoryMedals *)g_saveProfiles[g_saveSlots[index].category].medals;
    medal = 3 - place;
    switch (CGameInfo::GetConfiguredDifficulty()) {
    case 0:
        if (medal > pMedals->championshipD0) {
            pMedals->championshipD0 = medal;
            RallyData_IncrementCategoryUse(index);
        }
        break;
    case 1:
        if (medal > pMedals->championshipD1) {
            pMedals->championshipD1 = medal;
            RallyData_IncrementCategoryUse(index);
        }
        break;
    case 2:
        if (medal > pMedals->championshipD2) {
            pMedals->championshipD2 = medal;
            RallyData_IncrementCategoryUse(index);
        }
        break;
    }
}

// Split position of each player (index, 16.16 fraction to the next split).
// GLOBAL: CMR2 0x00536c48
int g_playerSplitPositions[8][2];
// Three split rows shown around each player's rank.
// GLOBAL: CMR2 0x00536c94
int g_splitNearbyRankRows[2][3];

int RallyData_GetCarRaceRecordField10(Car *p);
int StageTiming_GetCheckpointField0(int index);
int NetPlayers_GetPlayerID(int index);
int NetPlayers_GetResultField8ByID(int id);
int NetPlayers_GetResultField4ByID(int id);
int NetPlayers_GetPlayerField8(int index);
unsigned int NetPlayers_IsPlayerPresent(int index);
void RallyData_FindCheckpointAndFractionAtDistance(int *pOut, int distance, int percent);

// Updates the split position of the player and of every network player.
// FUNCTION: CMR2 0x00411f00
void RallyData_UpdateLocalAndNetworkSplitPositions(void)
{
    int i;
    int percent;

    percent = RallyData_GetCarRaceRecordField10(Car_Get(0)) * 100 >> 16;
    RallyData_FindCheckpointAndFractionAtDistance(g_playerSplitPositions[0], StageTiming_GetCheckpointField0(0), percent);
    for (i = 0; i < 7; i++) {
        if ((BYTE)NetPlayers_IsPlayerPresent(i))
            RallyData_FindCheckpointAndFractionAtDistance(g_playerSplitPositions[NetPlayers_GetPlayerField8(i)], NetPlayers_GetResultField4ByID(NetPlayers_GetPlayerID(i)),
                         NetPlayers_GetResultField8ByID(NetPlayers_GetPlayerID(i)));
    }
}

BYTE StageTiming_GetSplitDisplayState(void);
int StageTiming_GetSplitPositionOfDriver(int iDriver, int iSplit);
int StageTiming_GetSplitDriverCount(int iSplit);

// Picks the three split rows to show around the player's rank.
// match 53%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00414720
void RallyData_SelectNearbyRankSplitRows(int car)
{
    int position;

    if (g_stageSplitData[car].split == 0)
        return;
    if (StageTiming_GetSplitDisplayState()) {
        position = StageTiming_GetSplitPositionOfDriver((StageUI_GetRaceEndEventCount() & 0xff) + car, g_stageSplitData[car].split);
    } else {
        position = 15;
    }
    if (position > 0) {
        if (position < StageTiming_GetSplitDriverCount(g_stageSplitData[car].split) - 1) {
            g_splitNearbyRankRows[car][0] = position - 1;
            g_splitNearbyRankRows[car][1] = position;
            g_splitNearbyRankRows[car][2] = position + 1;
            return;
        }
        g_splitNearbyRankRows[car][0] = position - 2;
        g_splitNearbyRankRows[car][1] = position - 1;
        g_splitNearbyRankRows[car][2] = position;
        return;
    }
    g_splitNearbyRankRows[car][0] = position;
    g_splitNearbyRankRows[car][1] = position + 1;
    g_splitNearbyRankRows[car][2] = position + 2;
}

BYTE StageTiming_GetCheckpointField1A(int index);
void RallyData_SetFinishLineResultMessages(int car);

// Counts down the car's split display and, when it has improved, records the
// best time of the stage with the driver's name.
// match 77%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00415990
void RallyData_UpdateSplitDisplayAndStageRecord(int car)
{
    if (g_unk0x00536c08[car] != 0)
        g_unk0x00536c08[car]--;
    if (g_unk0x00536c08[car] == 0) {
        g_unk0x00536c00[car] = 0;
        g_unk0x00536c88[car] = 0;
        g_finishStageRecordMessage[car] = 0;
        g_finishLeaderMessage[car] = 0;
        g_unk0x00536ec4[car] = 0;
    }
    if (!(BYTE)RallyData_GetFlag24() &&
        (!(BYTE)RallyData_GetFlag25() || CGameInfo::GetConfiguredGameMode() != 3 || CGameInfo::GetGameModeOptionBit19())) {
        if (StageTiming_GetCheckpointField1A(car)) {
            int t = g_stageSplitData[car].times[0];
            if (g_stageLeaderTime == 0 || g_stageLeaderTime > t) {
                g_stageLeaderTime = t;
                sprintf(g_stageLeader.name, "%s", (char *)RallyData_GetRecord(StageUI_GetRaceEndEventCount() + (char)car));
            }
        }
    } else {
        RallyData_SetFinishLineResultMessages(car);
    }
}

BYTE StageTiming_GetCheckpointField17(int index);
void NetRace_SendSplitTime(int index, int value);
unsigned int NetPlayers_GetSplitRecordTime(int split);
void RallyData_PlayAlternateStageLoopSound(BYTE param1, int param2);

// On passing a split: stores the split time, fetches the time to beat and
// starts the split display (ahead/behind) for the car.
// FUNCTION: CMR2 0x00413520
void RallyData_RecordPassedSplitAndTimeToBeat(int car)
{
    int split;
    int target;

    g_splitTargetTimes[0] = 0;
    if (StageTiming_GetCheckpointField17(car) != 0) {
        split = StageTiming_GetCheckpointField14(car);
        g_stageLeader.lastSplit = split;
        if (split > 0 && split <= 8) {
            g_stageSplitData[car].times[split + 1] = g_stageSplitData[car].times[0];
            g_stageSplitData[car].lastSplitTime = g_stageSplitData[car].times[0];
            g_stageSplitData[car].split = split;
            NetRace_SendSplitTime(split, g_stageSplitData[car].times[0]);
            split = g_stageLeader.lastSplit;
        }
        g_stageStartRecorded = 1;
    } else {
        if (g_stageStartRecorded == 0)
            return;
        split = g_stageLeader.lastSplit;
    }
    target = NetPlayers_GetSplitRecordTime(split);
    g_stageSplitData[car].targetTime = target;
    if (g_stageSplitData[car].lastSplitTime < target) {
        RallyData_PlayAlternateStageLoopSound((BYTE)car, 1);
    } else {
        if (target == 0) {
            g_splitTargetTimes[0] = 1;
            goto show;
        }
        RallyData_PlayAlternateStageLoopSound((BYTE)car, 0);
    }
    g_stageStartRecorded = 0;
show:
    if (g_stageLeader.lastSplit != 0) {
        ((BYTE *)g_unk0x005170e0)[car * 4 + 3] = 0xff;
        g_splitGapDisplayFrames[car] = 0x4b;
    }
}

// Whether a driver's category has award `bit` (with `check`, some cheats
// count as having it).
// match 83%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00408e30
bool RallyData_HasCategoryAward(int index, int bit, char check)
{
    RallyData_ValidateIndex(index);
    if ((*(unsigned int *)&g_saveSlots[index] & 0x3c0000) == 0x3c0000)
        return false;
    if (check != 0) {
        if (CGameInfo::IsRecordFlagSet(0xc))
            return true;
        if (bit == 7) {
            if (CGameInfo::IsRecordFlagSet(0))
                return true;
        } else if (bit == 0x10) {
            if (CGameInfo::IsRecordFlagSet(1))
                return true;
        } else if (bit == 0xf) {
            if (CGameInfo::IsRecordFlagSet(2))
                return true;
        } else if (bit == 0x14) {
            if (CGameInfo::IsRecordFlagSet(3))
                return true;
        }
        if (bit == 7 && CGameInfo::IsRecordFlagSet(0x12))
            return true;
    }
    return (g_saveProfiles[(g_saveSlots[index].category)].awards &
            (1 << bit)) ? true : false;
}

// GLOBAL: CMR2 0x0058c944
ObjectPointBounds *g_objectPointBounds;
// GLOBAL: CMR2 0x0058c948
SectorObjectEntry *g_staticSectorEntries;
// GLOBAL: CMR2 0x0058ca70
SectorObjectBounds *g_sectorObjectBounds;

#define FREE_AND_CLEAR(p)                            \
    if ((p) != NULL) {                               \
        CFileBuffer::FreeGenericFileBuffer((void *)(p)); \
        (p) = NULL;                                  \
    }

// Releases the sector lists and element tables (registered callback of 0x471dd0).
// FUNCTION: CMR2 0x00472720
int RallyData_FreeSectorElementTables(void)
{
    FREE_AND_CLEAR(g_staticSectorEntries)
    FREE_AND_CLEAR(g_sectorEntryRaised)
    FREE_AND_CLEAR(g_sectorObjectBounds)
    FREE_AND_CLEAR(g_sectorListEntriesA)
    FREE_AND_CLEAR(g_objectPointBounds)
    FREE_AND_CLEAR(g_sectorListIndexA)
    FREE_AND_CLEAR(g_sectorListCountA)
    FREE_AND_CLEAR(g_movingSectorEntries)
    FREE_AND_CLEAR(g_sectorListCountB)
    FREE_AND_CLEAR(g_sectorListIndexB)
    FREE_AND_CLEAR(g_sectorListEntriesB)
    FREE_AND_CLEAR(g_sectorEntryCarFlags)
    return 1;
}

BYTE *StageObject_GetViewRectangleIndex(int index);
struct StageTableEntry;
StageTableEntry *StageObject_GetScreenRectangleEntry(int index);
void StageObject_InitScreenRectangles(void);
int Race_IsMultiplayerRecordMode10(void);

// Screen rectangle of a view: full screen, or the half chosen by the split
// direction (vertical or horizontal).
// match 57%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0040f050
int *RallyData_GetViewScreenRectangle(int view)
{
    int *pRect;
    int *pSource;

    StageObject_InitScreenRectangles();
    pRect = (int *)StageObject_GetViewRectangleIndex(view);
    if ((BYTE)RallyDataState() == 1 || Race_IsMultiplayerRecordMode10() != 0) {
        pSource = (int *)StageObject_GetScreenRectangleEntry(0);
    } else if (view == 0) {
        if (CGameInfo::IsSplitBarEnabled())
            pSource = (int *)StageObject_GetScreenRectangleEntry(1);
        else
            pSource = (int *)StageObject_GetScreenRectangleEntry(3);
    } else {
        if (CGameInfo::IsSplitBarEnabled())
            pSource = (int *)StageObject_GetScreenRectangleEntry(2);
        else
            pSource = (int *)StageObject_GetScreenRectangleEntry(4);
    }
    pRect[0] = pSource[0];
    pRect[1] = pSource[1];
    return (int *)pRect;
}

extern int g_unk0x00536fe0;
int StageTiming_GetCarTimingByte82(int car);

// GLOBAL: CMR2 0x00536c30
int g_unk0x00536c30[3];

// Updates the split display rows for a car.
// match 58%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00415870
void RallyData_UpdateCarSplitDisplayRows(int car)
{
    int i;
    int position;

    if (RallyData_GetFlag24() || RallyData_GetFlag25()) {
        for (i = 0; i < 3; i++) {
            if (g_unk0x00536c30[i] > 0)
                g_unk0x00536c30[i]--;
        }
        if (g_unk0x00536fe0 != 0) {
            for (i = 0; i < 3; i++)
                g_splitNearbyRankRows[car][i] = -1;
            position = StageTiming_GetCarTimingByte82(car);
            g_stageSplitData[car].position = position;
            if (position != -1) {
                if (position > 0) {
                    if (position < 5) {
                        g_splitNearbyRankRows[car][0] = position - 1;
                        g_splitNearbyRankRows[car][1] = position;
                        g_splitNearbyRankRows[car][2] = position + 1;
                    } else {
                        g_splitNearbyRankRows[car][0] = position - 2;
                        g_splitNearbyRankRows[car][1] = position - 1;
                        g_splitNearbyRankRows[car][2] = position;
                    }
                } else {
                    g_splitNearbyRankRows[car][0] = position;
                    g_splitNearbyRankRows[car][1] = position + 1;
                    g_splitNearbyRankRows[car][2] = position + 2;
                }
            }
        } else {
            for (i = 0; i < 3; i++)
                g_splitNearbyRankRows[car][i] = i;
            g_stageSplitData[car].position = StageTiming_GetCarTimingByte81(car);
        }
    }
    if ((BYTE)CGameInfo::GetConfiguredGameMode() == 5 || (BYTE)CGameInfo::GetConfiguredGameMode() == 6)
        StageTiming_GetCheckpointField19(car);
    else if (StageTiming_GetCheckpointField17(car))
        g_splitGapDisplayFrames[car] = 0x4b;
}

// GLOBAL: CMR2 0x00533758
int g_unk0x00533758[21];
// GLOBAL: CMR2 0x005337cc
int g_unk0x005337cc;

// Picks an unexcluded event from the three or four events in its group.
// match 58%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0040e210
int RallyData_PickUnexcludedGroupEvent(int exclude1, int exclude2)
{
    if (exclude1 != 0x13 && exclude1 != 0x11 && exclude1 != 0x12) {
        g_unk0x00533758[17] = 0;
        g_unk0x00533758[18] = 0;
        g_unk0x00533758[19] = 0;
        g_unk0x00533758[20] = 0;
        if (exclude1 >= 0)
            g_unk0x00533758[5 + exclude1] = 1;
        if (exclude2 >= 0)
            g_unk0x00533758[5 + exclude2] = 1;
        g_unk0x005337cc = rand() % 4;
        while (g_unk0x00533758[17 + g_unk0x005337cc] == 1)
            g_unk0x005337cc = rand() % 4;
        return g_unk0x005337cc + 12;
    }
    g_unk0x00533758[17] = 0;
    g_unk0x00533758[18] = 0;
    g_unk0x00533758[19] = 0;
    if (exclude1 >= 0)
        g_unk0x00533758[exclude1] = 1;
    if (exclude2 >= 0)
        g_unk0x00533758[exclude2] = 1;
    g_unk0x005337cc = rand() % 3;
    while (g_unk0x00533758[17 + g_unk0x005337cc] == 1)
        g_unk0x005337cc = rand() % 3;
    return g_unk0x005337cc + 0x11;
}

int StageTiming_GetSplitIndexValue(int index);
int StageTiming_GetSplitTableEntry(int split, int index);
int StageTiming_GetSplitDriverIDForPosition(int position, int split);
int StageTiming_GetSplitPositionOfDriver(int driver, int split);
unsigned int Knockout_GetCurrentDriverField(unsigned int *pHigh);

// Formats the name shown for a driver in the current result list.
// FUNCTION: CMR2 0x00415750
void RallyData_FormatResultDriverName(int driver, int split, int useLongName, int useSplit)
{
    int entry;
    int row;
    char *name;

    sprintf(CFrontend::m_stringDest, "%s", CMain::m_logFileBlankLine);
    if (useSplit != 0)
        entry = StageTiming_GetSplitDriverIDForPosition(driver, split);
    else
        entry = StageTiming_GetSplitTableEntry(StageUI_GetRaceEndEventCount(), StageTiming_GetSplitIndexValue(driver));
    if (entry == -1) {
        sprintf(CFrontend::m_stringDest, "%s", CMain::m_logFileBlankLine);
        return;
    }
    for (row = 0; row < (BYTE)CGameInfo::GetConfiguredPlayerCount(); row++) {
        int candidate = useSplit != 0 ? StageTiming_GetSplitPositionOfDriver(row, split) : StageTiming_GetCarTimingByte81(row);
        if (candidate == driver) {
            if (CGameInfo::GetConfiguredGameMode() != 4)
                name = (char *)RallyData_GetRecord((BYTE)row);
            else
                name = (char *)RallyData_GetRecord((BYTE)Knockout_GetCurrentDriverField((unsigned int *)entry));
            goto done;
        }
    }
    if (CGameInfo::GetConfiguredGameMode() != 4) {
        if (useLongName != 0)
            name = (char *)RallyData_GetDriverNameIndexRecord(entry);
        else
            name = CAIHelper::GetNameForID(entry);
    } else {
        if (useLongName != 0)
            name = (char *)RallyData_GetDriverNameIndexRecord(Knockout_GetCurrentDriverField((unsigned int *)entry));
        else
            name = CAIHelper::GetNameForID(Knockout_GetCurrentDriverField((unsigned int *)entry));
    }
done:
    sprintf(CFrontend::m_stringDest, "%s", name);
    CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
}

int StageTiming_GetSplitLeaderTime(int car);
int StageTiming_GetClockTime(void);
int StageTiming_GetCarTimingByte82(int car);
int StageTiming_GetCarTimingByte80(int car);
BYTE StageTiming_GetCarTimingByte83(int car);
int StageTiming_GetGroupedCheckpointValue(int index, int car);

// Per car: a split was passed before the leader's time there was known.
// GLOBAL: CMR2 0x00536c18
int g_splitGapAwaitingLeader[2];
// Per car: gap to the leader at the last split, shown by the HUD.
// GLOBAL: CMR2 0x00536fe4
int g_splitGapTime[2];
// Per car: a split gap has been recorded since the stage start.
// GLOBAL: CMR2 0x00537054
int g_splitGapRecorded[2];

// Updates the split timer and stage sound for a player.
// FUNCTION: CMR2 0x00413200
void RallyData_UpdatePlayerSplitTimerAndSound(int car)
{
    if (StageTiming_GetCheckpointField17(car)) {
        g_splitGapRecorded[car] = 1;
        g_splitGapTime[car] = StageTiming_GetClockTime() - StageTiming_GetSplitLeaderTime(car);
        if (StageTiming_GetCarTimingByte82(car) > 0) {
            RallyData_PlayAlternateStageLoopSound(car, 0);
            ((BYTE *)g_unk0x005170e0)[car * 4 + 3] = 0xff;
            g_splitGapDisplayFrames[car] = 0x4b;
            g_splitGapBlank[car] = 0;
            g_splitGapAwaitingLeader[car] = 0;
            g_stageSplitData[car].split = 1;
        } else {
            RallyData_PlayAlternateStageLoopSound(car, 1);
            g_splitGapAwaitingLeader[car] = 1;
            g_splitGapBlank[car] = 1;
            g_stageSplitData[car].split = 1;
        }
    }
    if (StageTiming_GetCarTimingByte83(car)) {
        if (g_splitGapAwaitingLeader[car] == 0)
            return;
        g_splitGapRecorded[car] = 1;
        g_splitGapBlank[car] = 0;
        int index = StageTiming_GetCarTimingByte80(car);
        int now = StageTiming_GetClockTime();
        g_splitGapTime[car] = StageTiming_GetGroupedCheckpointValue(index, car) - now;
        ((BYTE *)g_unk0x005170e0)[car * 4 + 3] = 0xff;
        g_splitGapDisplayFrames[car] = 0x4b;
        g_stageSplitData[car].split = 1;
        g_splitGapAwaitingLeader[car] = 0;
    }
    if (g_splitGapAwaitingLeader[car] != 0 && g_splitGapRecorded[car] != 0) {
        ((BYTE *)g_unk0x005170e0)[car * 4 + 3] = 0xff;
        g_splitGapDisplayFrames[car] = 0x4b;
    }
}



/* --------------------------------------------------------------------------
   Rally-data input dispatch and per-category difficulty records.
   -------------------------------------------------------------------------- */

void RallyData_UpdatePlayerSplitTimerAndSound(int car);
void RallyData_RecordPassedSplit(int car);
void RallyData_RecordPassedSplitAndTimeToBeat(int car);
void RallyData_RecordStageStartAndGhostSplit(int car);
void RallyData_IncrementCategoryUse(int index);
void Graphics_PresentFrameAndResetCounters(void);

// Notifies the menu handler when the received value is one below the key code.
// FUNCTION: CMR2 0x0040eed0
void RallyData_NotifyAdjacentMenuKeyValue(CallbackStateMachine *pKey, int value)
{
    if ((value & 0xff) == pKey->count - 1)
        Graphics_PresentFrameAndResetCounters();
}

// Dispatches the current game state to the handler of the active game mode.
// FUNCTION: CMR2 0x00413160
void RallyData_DispatchActiveGameModeState(int car)
{
    if (CGameInfo::GetGameModeOptionBit19() != 0) {
        switch (CGameInfo::GetConfiguredGameMode()) {
        case 8:
        case 9:
        case 10:
            RallyData_RecordPassedSplitAndTimeToBeat(car);
            break;
        case 11:
        case 12:
            RallyData_RecordStageStartAndGhostSplit(car);
            break;
        }
    } else if (CGameInfo::GetConfiguredGameMode() != 6 && CGameInfo::GetConfiguredGameMode() != 5) {
        if ((BYTE)RallyData_GetSelectionFlag27() != 0)
            RallyData_UpdatePlayerSplitTimerAndSound(car);
        else
            RallyData_RecordPassedSplit(car);
    } else {
        RallyData_UpdatePlayerSplitTimerAndSound(car);
    }
}

// Records the arcade final medal (place 0..2) of the driver's category at the
// current difficulty and arcade selection if it beats the stored one.
// FUNCTION: CMR2 0x00409680
void RallyData_RecordArcadeMedal(int index, int arcade, int place)
{
    CategoryMedals *pMedals;
    unsigned char mode;
    unsigned int medal;

    RallyData_ValidateIndex(index);
    pMedals = (CategoryMedals *)g_saveProfiles[g_saveSlots[index].category].medals;
    medal = 3 - place;
    mode = CGameInfo::GetConfiguredDifficulty();
    switch (mode) {
    case 0:
        if (medal > pMedals->arcadeD0) {
            pMedals->arcadeD0 = medal;
            RallyData_IncrementCategoryUse(index);
        }
        break;
    case 1:
        if (arcade == 0) {
            if (medal > pMedals->arcadeD1) {
                pMedals->arcadeD1 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
        } else if (medal > pMedals->arcadeD1B) {
            pMedals->arcadeD1B = medal;
            RallyData_IncrementCategoryUse(index);
            return;
        }
        break;
    case 2:
        if (arcade == 0) {
            if (medal > pMedals->arcadeD2) {
                pMedals->arcadeD2 = medal;
                RallyData_IncrementCategoryUse(index);
                return;
            }
        } else if (medal > pMedals->arcadeD2B) {
            pMedals->arcadeD2B = medal;
            RallyData_IncrementCategoryUse(index);
            return;
        }
        break;
    }
}

/* Sound samples used by the rally-data screens (original .data at 0x517d00). */
// GLOBAL: CMR2 0x00517d00
char g_strTimeoutWav[12] = "timeout.wav";
// GLOBAL: CMR2 0x00517d0c
char g_strCrossWav[12] = "cross.wav";
// GLOBAL: CMR2 0x00517d18
char g_strBlipWav[12] = "blip.wav";
// GLOBAL: CMR2 0x00517d24
char g_strDownWav[12] = "down.wav";
// GLOBAL: CMR2 0x00517d30
char g_strUpWav[8] = "up.wav";

struct StageFile;
int Sound_GetLoadedSampleCount(void);
BOOL Sound_LoadSample(char *name, BYTE flags, GenericFile *pFile);
StageFile *StageTiming_GetStageFile0(void);

// Loads the navigation sound samples of the rally-data screens.
// FUNCTION: CMR2 0x00411120
void RallyData_LoadNavigationSounds(void)
{
    g_unk0x00536ecc = Sound_GetLoadedSampleCount();
    Sound_LoadSample(g_strUpWav, 0, (GenericFile *)StageTiming_GetStageFile0());
    g_unk0x00537060 = Sound_GetLoadedSampleCount();
    Sound_LoadSample(g_strDownWav, 0, (GenericFile *)StageTiming_GetStageFile0());
    g_unk0x00536cb0 = Sound_GetLoadedSampleCount();
    Sound_LoadSample(g_strBlipWav, 0, (GenericFile *)StageTiming_GetStageFile0());
    Sound_LoadSample(g_strCrossWav, 0, (GenericFile *)StageTiming_GetStageFile0());
    Sound_LoadSample(g_strTimeoutWav, 0, (GenericFile *)StageTiming_GetStageFile0());
}

struct Menu;
void Menu_CallCallback2(Menu *pMenu);
int Race_GetFlag38108(void);
Menu *StageObject_GetInRaceActionMenu(void);
int Game_PrepareScene(SceneNode *pRoot, SceneNode *pCamera, void *pRect, int param);
int Game_DrawSceneViewport(struct SceneNode *pRoot, struct SceneNode *pCamera, void *pRect, int bit, BYTE flag);
void Graphics_PresentFrameAndResetCounters(void);

void CameraEffects_DrawOverlays(short *pRect);

// Sets up the projection, clears the targets and draws the challenge scene.
// FUNCTION: CMR2 0x00411070
void RallyData_DrawProjectedChallengeScene(int param1, int param2)
{
    short rect[4];

    rect[0] = 0;
    rect[1] = 0;
    rect[2] = *(short *)g_pGraphics;
    rect[3] = *(short *)((BYTE *)g_pGraphics + 4);
    CGraphics::SetProjection(0x25645, 0x4326e, 0xfa0000, 0x10000);
    CGraphics::ClearTarget();
    CGraphics::ClearZBuffer();
    if (Race_GetFlag38108() == 0)
        Menu_CallCallback2(StageObject_GetInRaceActionMenu());
    Game_PrepareScene(g_unk0x00536be0, g_unk0x00536be4, rect, 0);
    DebugOverlays_RequestView(0); // paused fixed-projection scene still needs late tuning UI
    Game_DrawSceneViewport(g_unk0x00536be0, g_unk0x00536be4, rect, 0, 1);
    // OpenCMR2: camera effects — dirt on the lens and a sun lens flare,
    // drawn over the finished frame ([video] video.camera_effects).
    CameraEffects_DrawOverlays(rect);
    TuningPanel_NotePausedDraw("challenge scene (fixed projection)");
    Graphics_PresentFrameAndResetCounters();
}

// OpenCMR2: per-view dirt-on-the-lens level (0 = clean, 255 = obscured).
// Accumulates on impacts, decays each frame; stages start clean.
unsigned int g_lensDirt[8];

// OpenCMR2: per-view dirt-on-the-windscreen level (0 = clean, 255 = obscured).
// Accumulates from driving in rain/dust/mud; cleared by wipers.
unsigned int g_windscreenDirt[8];

// OpenCMR2: per-view windscreen wiper state.
struct WindscreenWiper g_windscreenWipers[8];

// OpenCMR2: last-read state of the wiper action key, for edge detection.
int g_windscreenWiperKeyPrevState = 0;

// OpenCMR2: frames remaining in a manually-triggered wiper sweep. When > 0,
// the wiper sweeps once and then stops (unlike the always-on setting).
int g_windscreenWiperManualFrames = 0;

// ---- Live effect tuning (visual only) ---------------------------------------------------------
// F10 (video.tuning_key, a DirectInput scancode) opens a panel on the right. Digit keys 1/2 pick a row, 3/4 change it, 5 resets
// the row and 6 resets all rows to the values they had when the panel was first opened this launch (after any menu edits), 7 saves the shown values to opencmr2.ini. Changes are
// per launch until saved. While open, these keys are removed from the game's keyboard state (see CInput::ReadKeyboardState).
static bool g_tuneOpen = false, g_tuneSnapshotTaken = false;
// F10 cycles (game/TuningMode.h) 0 = debug info only (the start), 1 = debug info and adjustments, 2 = nothing, 3 = frame rate only. g_tuneOpen means "adjustments shown" (mode 1) and is what
// captures the adjustment keys. g_tuneEngaged is separate: once F10 was pressed, the tuned look stays live (paused scene too) in every mode, so both
// overlays can be hidden to judge the effects.
static int g_tuneMode = 0;
static bool g_tuneEngaged = false;
static int g_tuneRow = 0, g_tuneSnapshot[kTuningMaxRows], g_camMoveSnapshot[16];   // the camera move row edits one camera type: the snapshot holds every type's raw per-type value (kCamMoveUnset = no override)
void AttractCapture_ViewSignature(int view, int *type, int *drone);
static bool TuneIsCamMove(const TuningRow &r) { return !strcmp(r.key, "video.camera_move_active"); }
static int TuneCamType(void) { int vt = 0, dr = 0; AttractCapture_ViewSignature(0, &vt, &dr); return vt < 0 || vt > 15 ? 0 : vt; }
static void TuneCamKey(int type, char *buf, size_t n) { snprintf(buf, n, "video.camera_move_type%d", type); }
// The value shown and stepped: for the camera row, the per-type value of the camera in use, else its family value.
static int TuneGet(const TuningRow &r)
{
    if (!TuneIsCamMove(r)) return Sys_GetOption(r.key, r.fallback);
    char k[40]; int t = TuneCamType(); TuneCamKey(t, k, sizeof k);
    return CameraMove_Effective(Sys_GetOption(k, kCamMoveUnset), Sys_GetOption(CameraMove_FamilyKey(t), 0));
}
static const char *g_tuneSite = "?";
void TuningPanel_NoteSite(const char *site) { g_tuneSite = site; }
void TuningPanel_NotePausedDraw(const char *what)
{
    static double last = 0;
    double now = Sys_GetTicksPrecise();
    if (now - last < 1000.0) return;
    last = now;
    Sys_Log("tuning diag: draw %s (tuning engaged=%d open=%d, race menu open=%d)", what, (int)TuningPanel_PreviewActive(), (int)TuningPanel_IsOpen(), (int)CGameInfo::IsInRaceMenuOpen());
}
static void TuneSet(const TuningRow &r, int v)
{
    Sys_Log("tuning diag: set %s=%d from %s (race menu open=%d)", r.key, v, g_tuneSite, (int)CGameInfo::IsInRaceMenuOpen());
    if (!TuneIsCamMove(r)) { Sys_SetOptionTemporary(r.key, v); return; }
    char k[40]; TuneCamKey(TuneCamType(), k, sizeof k); Sys_SetOptionTemporary(k, v);
}
static unsigned g_tuneSavedStamp = 0, g_tuneFrame = 0;
// Sprite/triangle layer 4 is the BACKGROUND layer: Game_DrawSceneViewport draws it before the 3D scene, so the scene covers it. Layers 3, 2, 1 are drawn after the scene. Layer 1 holds only 1023 sprites and the debug pane uses it, so debug UI uses its emptied layer3 queue in a dedicated late pass above the pause wash.
static const int kTuneOverlayLayer = 3;
static TuningEdges g_tuneEdges;
bool TuningPanel_IsOpen(void) { return g_tuneOpen; }
bool TuningPanel_PreviewActive(void) { return g_tuneEngaged; }
bool TuningPanel_InfoShown(void) { return TuningMode_InfoShown(g_tuneMode); }
bool TuningPanel_FpsOnly(void) { return TuningMode_FpsOnly(g_tuneMode); }
int TuningPanel_ToggleKey(void)
{
    static int checked = -1, result = kTuningToggleDefault;
    int key = Sys_GetOption("video.tuning_key", kTuningToggleDefault);
    if (key == checked) return result;
    checked = key;
    bool ok = Tuning_ToggleKeyAllowed(key);
    // Also refuse a key the player has bound to a driving action on any attached keyboard device.
    for (int device = 0; ok && device < CInput::CountAttachedInputDevices(); device++) {
        DeviceInfo *info = CInput::GetAvailableDeviceRecord(device);
        if (info && info->field_0x0 == 1)
            for (int action = 0; action < 9; action++)
                if ((&info->keyboard.field_0x468)[action] == key) ok = false;
    }
    if (!ok) Sys_Log("tuning panel: video.tuning_key=%d is a driving/menu key or invalid; using F10 (68)", key);
    result = ok ? key : kTuningToggleDefault;
    return result;
}
extern "C" double Sys_GetTicksPrecise(void);
void TuningPanel_Update(void)
{
    ++g_tuneFrame;
    {   // Diagnostics: log once when the calling site changes while tuning is engaged, so a pause entry point that never reaches the panel shows in the log.
        static const char *lastSite = nullptr;
        if (g_tuneEngaged && lastSite != g_tuneSite) { lastSite = g_tuneSite; Sys_Log("tuning diag: panel update now called from %s (open=%d mode=%d)", g_tuneSite, (int)g_tuneOpen, g_tuneMode); }
    }
    int count = 0;
    const TuningRow *rows = Tuning_Rows(count);
    if (g_tuneEdges.Edge(0, Input_IsKeyDown(TuningPanel_ToggleKey()) != 0)) { g_tuneMode = TuningMode_Next(g_tuneMode); g_tuneOpen = TuningMode_AdjustShown(g_tuneMode); g_tuneEngaged = true; }
    bool pressed[7];
    static TuningRepeat repeat;
    double nowMs = Sys_GetTicksPrecise();
    for (int i = 0; i < 7; ++i) {
        bool down = Input_IsKeyDown(kTuningKeys[i]) != 0;
        bool edge = g_tuneEdges.Edge(1 + i, down);
        // Row and value keys (1..4) repeat while held; reset, reset all and save stay single presses.
        pressed[i] = i < 4 ? repeat.Fire(i, down, nowMs) : edge;
    }
    if (!g_tuneOpen) return;
    if (count > kTuningMaxRows) count = kTuningMaxRows;   // snapshot/preset arrays are sized kTuningMaxRows; the tests pin the row list below it
    bool prevRow = pressed[0], nextRow = pressed[1], down = pressed[2], up = pressed[3], resetRow = pressed[4], resetAll = pressed[5], save = pressed[6];
    if (!g_tuneSnapshotTaken) {
        for (int i = 0; i < count; ++i) g_tuneSnapshot[i] = Sys_GetOption(rows[i].key, rows[i].fallback);
        for (int t = 0; t < 16; ++t) { char k[40]; TuneCamKey(t, k, sizeof k); g_camMoveSnapshot[t] = Sys_GetOption(k, kCamMoveUnset); }
        g_tuneSnapshotTaken = true;
    }
    bool changed = false;
    if (prevRow) g_tuneRow = Tuning_Select(g_tuneRow, -1, count);
    if (nextRow) g_tuneRow = Tuning_Select(g_tuneRow, 1, count);
    if ((down || up) && !(Sys_GetOption("video.drone_locked",0)&&Tuning_DroneParameter(rows[g_tuneRow]))) {
        const TuningRow &r = rows[g_tuneRow];
        int dronePreset[kTuningMaxRows]={};
        if(!strcmp(r.key,"video.drone_locked"))for(int i=0;i<count;i++)dronePreset[i]=Sys_GetOption(rows[i].key,rows[i].fallback);
        TuneSet(r, Tuning_Step(r, TuneGet(r), up ? 1 : -1));
        // Lock freezes chosen parameters, not adaptive camera motion. Save
        // the drone preset immediately through the existing INI save path.
        if(!strcmp(r.key,"video.drone_locked")){
            for(int i=0;i<count;i++)if(Tuning_DroneParameter(rows[i]))Sys_SetOption(rows[i].key,dronePreset[i]);
            Sys_SetOption("video.drone_locked",Sys_GetOption("video.drone_locked",0));
        }
        changed = true;
    }
    if (resetRow && !(Sys_GetOption("video.drone_locked",0)&&(Tuning_DroneParameter(rows[g_tuneRow])||!strcmp(rows[g_tuneRow].key,"video.drone_locked")))) {
        if (TuneIsCamMove(rows[g_tuneRow])) TuneSet(rows[g_tuneRow], g_camMoveSnapshot[TuneCamType()]);   // back to this camera's opening state (override or none)
        else Sys_SetOptionTemporary(rows[g_tuneRow].key, g_tuneSnapshot[g_tuneRow]);
        changed = true;
    }
    if (resetAll) {
        for (int i = 0; i < count; ++i) if (!TuneIsCamMove(rows[i])&&!(Sys_GetOption("video.drone_locked",0)&&(Tuning_DroneParameter(rows[i])||!strcmp(rows[i].key,"video.drone_locked")))) Sys_SetOptionTemporary(rows[i].key, g_tuneSnapshot[i]);
        for (int t = 0; t < 16; ++t) { char k[40]; TuneCamKey(t, k, sizeof k); Sys_SetOptionTemporary(k, g_camMoveSnapshot[t]); }
        changed = true;
    }
    if (save) {
        for (int i = 0; i < count; ++i) if (strcmp(rows[i].key, "video.fov_preview") && !TuneIsCamMove(rows[i])) Sys_SetOption(rows[i].key, Sys_GetOption(rows[i].key, rows[i].fallback));   // the FOV preview is a per-launch debug view, never saved
        for (int t = 0; t < 16; ++t) { char k[40]; TuneCamKey(t, k, sizeof k); int v = Sys_GetOption(k, kCamMoveUnset); if (v != kCamMoveUnset) Sys_SetOption(k, v); }   // only camera types that have an override
        g_tuneSavedStamp = g_tuneFrame;
    }
    if (changed) EffectSettings::Apply();
}
// OpenCMR2 replay controls (keyboard): , slower  . faster  Space pause  Tab next camera. Active only while player 0's stream is playing back.
// The recorded controls still run through the unchanged physics; only the time that feeds the step count is scaled (Car.cpp).
#include "ReplayControls.h"
#include "FreeCam.h"
#include "ReplayChaseOrbit.h"
#include "ReplayStream.h"
int Race_GetPlayerRecordField4(BYTE index);
int View_GetActiveCameraMode(BYTE index);
BYTE View_GetActiveCameraFlags(BYTE index);
void View_SwitchCamera(BYTE view, int type, int param, BYTE target, int blend);
int View_FindFreeModeSlot(unsigned char view);
ReplayStream *Race_GetPlayerRecordPointer(int index);
static ReplayControls g_replayControls;
static TuningEdges g_replayEdges;
bool ReplayControls_Active(void) { return Race_GetPlayerRecordField4(0) != 0; }
BYTE InRaceMenu_IsOpening(void);
// A menu owns the keyboard whenever the game is in menu/pause input mode (SetInputAndGamePaused(1): the Esc pause menu, the in-race menu and its opening
// animation). CInput::ReadKeyboardDevice reads the arrows for the menu only in that mode, so the replay camera and replay keys must not clear them then.
static bool ReplayMenuOwnsInput(void) { return CGameInfo::IsInRaceMenuOpen() != 0 || InRaceMenu_IsOpening() != 0 || CGameInfo::IsInputAndGamePaused() != 0; }
float ReplayControls_TimeScale(void) { return ReplayControls_Active() ? g_replayControls.Scale() : 1.0f; }
// A replay key is usable when it is not on the game's refusal list (Esc, Return, arrows, [ ], C, F1/F2 ...). Space is allowed here although the list refuses it
// for the F10 key: replay is the one place it is free, because the recorded controls run the car and live driving keys do nothing. The same reasoning drops the
// driving-binding check: a key bound to a driving action is still a replay key while a replay plays, and the game does not see it (Input.cpp clears it).
static bool ReplayKeyUsable(int key)
{
    return key == INPUT_KEY_SPACE || Tuning_ToggleKeyAllowed(key);
}
bool ReplayControls_KeyIsReplayKey(int key) { return ReplayControls_Active() && !ReplayMenuOwnsInput() && ReplayKeyUsable(key) && (key == kReplayKeySlower || key == kReplayKeyFaster || key == kReplayKeyPause || key == kReplayKeyCamera); }
int View_CycleReplayCamera(unsigned char view);
void View_UpdateCamera(BYTE view);
int View_FreeCamActive(void);
void View_FreeCamStop(void);
void View_FreeCamFrame(unsigned char view, const FreeCamInput *in, int reset);
void View_ReplayChaseReset();
void View_ReplayChaseFrame(unsigned,int,int,int,int,int);
static const int kFreeCamKeys[] = {0x11, 0x1f, 0x1e, 0x20, 0x10, 0x12, 0x13, 0x2a, 0x36, 0xc8, 0xd0, 0xcb, 0xcd};
bool ReplayControls_KeyIsFreeCamKey(int key)
{
    if (!ReplayControls_Active() || ReplayChaseOrbit::InputCaptured(ReplayMenuOwnsInput(), TuningPanel_IsOpen())) return false;
    if(!View_FreeCamActive())return ReplayChaseOrbit::Chase(View_GetActiveCameraMode(0)) && ReplayChaseOrbit::Key(key);
    for (int k : kFreeCamKeys) if (k == key) return true;
    return false;
}
static void ReplayControls_CycleCamera(void)
{
    View_CycleReplayCamera(0);
    // The camera blend advances per sim step. While paused no step runs, so finish the blend by hand (camera state only) and the new view shows at once.
    if (g_replayControls.paused)
        for (int i = 0; i < 20; i++) View_UpdateCamera(0);
}
void ReplayControls_Update(void)
{
    if (!ReplayControls_Active()) { g_replayControls.Reset(); View_FreeCamStop(); View_ReplayChaseReset(); return; }   // replay over or stage changed: the free camera never carries over
    if (ReplayKeyUsable(kReplayKeySlower) && g_replayEdges.Edge(0, Input_IsKeyDown(kReplayKeySlower) != 0)) g_replayControls.Slower();
    if (ReplayKeyUsable(kReplayKeyFaster) && g_replayEdges.Edge(1, Input_IsKeyDown(kReplayKeyFaster) != 0)) g_replayControls.Faster();
    if (ReplayKeyUsable(kReplayKeyPause) && g_replayEdges.Edge(2, Input_IsKeyDown(kReplayKeyPause) != 0)) g_replayControls.TogglePause();
    if (ReplayKeyUsable(kReplayKeyCamera) && g_replayEdges.Edge(3, Input_IsKeyDown(kReplayKeyCamera) != 0)) ReplayControls_CycleCamera();
    if(ReplayChaseOrbit::InputCaptured(ReplayMenuOwnsInput(), TuningPanel_IsOpen())){View_ReplayChaseFrame(0,0,0,0,0,0);return;}
    bool cameraReset=g_replayEdges.Edge(4,Input_IsKeyDown(0x13)!=0);
    if(!View_FreeCamActive())View_ReplayChaseFrame(0,Input_IsKeyDown(0xcb)!=0,Input_IsKeyDown(0xcd)!=0,Input_IsKeyDown(0xc8)!=0,Input_IsKeyDown(0xd0)!=0,cameraReset);
    if (View_FreeCamActive()) {   // arrows and WASD are free in replay: the recorded controls drive the car
        FreeCamInput in = {};
        in.forward = Input_IsKeyDown(0x11) != 0; in.back = Input_IsKeyDown(0x1f) != 0; in.left = Input_IsKeyDown(0x1e) != 0; in.right = Input_IsKeyDown(0x20) != 0;
        in.down = Input_IsKeyDown(0x10) != 0; in.up = Input_IsKeyDown(0x12) != 0; in.fast = Input_IsKeyDown(0x2a) != 0 || Input_IsKeyDown(0x36) != 0;
        in.lookUp = Input_IsKeyDown(0xc8) != 0; in.lookDown = Input_IsKeyDown(0xd0) != 0; in.turnLeft = Input_IsKeyDown(0xcb) != 0; in.turnRight = Input_IsKeyDown(0xcd) != 0;
        View_FreeCamFrame(0, &in, cameraReset ? 1 : 0);
    }
}
// Small bottom-centre line: replay time (the stage clock, which the re-run reproduces), speed, camera. Drawn only while replaying.
static const char *ReplayCameraName(int type)
{
    switch (type) { case 7: return "auto"; case 4: return "chase"; case 5: return "drone/chase"; case 1: return "car"; case 2: return "bonnet"; case 3: return "cockpit"; case 10: return "bumper"; default: return "view"; }
}
void ReplayControls_Draw(void)
{
    if (!ReplayControls_Active()) return;
    char clock[40], speed[24], text[120];
    clock[0] = 0;
    ReplayStream *stream = Race_GetPlayerRecordPointer(0);
    if (stream != NULL && stream->type == 0 && stream->pFrames != NULL && stream->pLaneSamples != NULL && stream->lane >= 0 && stream->lane < stream->laneCount) {
        // Elapsed and total from the recorded control packets (steps), not the stage clock, so both use one unit. Total is cached per lane.
        static ReplayStream *cachedStream = NULL; static int cachedLane = -1, cachedPackets = -1; static long cachedTotal = 0;
        const unsigned char *packets = (const unsigned char *)stream->pFrames + stream->samplesPerLane * stream->lane * 4;
        int count = stream->pLaneSamples[stream->lane];
        if (cachedStream != stream || cachedLane != stream->lane || cachedPackets != count) { cachedStream = stream; cachedLane = stream->lane; cachedPackets = count; cachedTotal = ReplayStepsTotal(packets, count); }
        int frame = stream->frame < 0 ? 0 : stream->frame > count ? count : stream->frame;
        float rate = Car_Get(0)->simulationRateHz;
        if (rate > 1.0f && count > 0) {
            char a[16], b[16];
            ReplayClock_FormatCentiseconds((int)(ReplayStepsBefore(packets, frame, stream->field_0x21) * 100.0f / rate), a, sizeof a);
            ReplayClock_FormatCentiseconds((int)(cachedTotal * 100.0f / rate), b, sizeof b);
            snprintf(clock, sizeof clock, "%s / %s", a, b);
        }
    }
    if (!clock[0]) ReplayClock_FormatCentiseconds(StageTiming_GetClockTime(), clock, sizeof clock);   // not a controls stream: the stage clock only, no total
    g_replayControls.Label(speed, sizeof speed);
    snprintf(text, sizeof text, "replay %s  %s  cam %s  , . space tab", clock, speed, View_FreeCamActive() ? "free (wasd qe arrows r)" : ReplayChaseOrbit::Chase(View_GetActiveCameraMode(0)) ? "chase (arrows orbit, r reset)" : ReplayCameraName(View_GetActiveCameraMode(0)));
    DebugOverlayLayout layout = DebugOverlayLayout::For(g_pGraphics->resX, g_pGraphics->resY, 1);
    int width = std::min((int)strlen(text) * layout.font * 6 / 10 + 2 * layout.pad, g_pGraphics->resX - 2 * layout.x);
    short panel[4] = {(short)((g_pGraphics->resX - width) / 2), (short)(g_pGraphics->resY - layout.height - layout.y), (short)width, (short)layout.height};
    BYTE background[4] = {12, 16, 22, 150}, colour[4] = {240, 246, 255, 255};
    Sprite_FillRect(NULL, panel, background, kTuneOverlayLayer);
    Font_DrawTextScaled(1, text, panel[0] + layout.pad, panel[1] + layout.pad, layout.font, width - 2 * layout.pad, colour, kTuneOverlayLayer);
}
void TuningPanel_Draw(void)
{
    if (!g_tuneOpen) return;
    int count = 0;
    const TuningRow *rows = Tuning_Rows(count);
    int visible=std::min(count,12),first=std::max(0,std::min(g_tuneRow-visible/2,count-visible));
    DebugOverlayLayout layout = DebugOverlayLayout::For(g_pGraphics->resX, g_pGraphics->resY, visible + 3);
    short panel[4] = {(short)(g_pGraphics->resX - layout.width - layout.x), (short)(g_pGraphics->resY * 23 / 100), (short)layout.width, (short)layout.height};
    BYTE background[4] = {12, 16, 22, 220}, colour[4] = {240, 246, 255, 255}, lit[4] = {110, 255, 140, 255};
    Sprite_FillRect(NULL, panel, background, kTuneOverlayLayer);
    char text[96];
    int tx = panel[0] + layout.pad, ty = panel[1] + layout.pad, row = 0;
    snprintf(text, sizeof text, "TUNING  1/2 row  3/4 value (%d/%d)",g_tuneRow+1,count);
    Font_DrawTextScaled(1, text, tx, ty + row++ * layout.line, layout.font, layout.width - 2 * layout.pad, colour, kTuneOverlayLayer);
    for (int i = first; i < first+visible; ++i) {
        if (TuneIsCamMove(rows[i])) {
            char k[40]; int t = TuneCamType(); TuneCamKey(t, k, sizeof k);
            snprintf(text, sizeof text, "%s Camera type %d move (%s) 0.05 units: %d", i == g_tuneRow ? ">" : " ", t, Sys_GetOption(k, kCamMoveUnset) == kCamMoveUnset ? "family" : "own", TuneGet(rows[i]));
        } else
        snprintf(text, sizeof text, "%s %s: %d", i == g_tuneRow ? ">" : " ", rows[i].label, Sys_GetOption(rows[i].key, rows[i].fallback));
        Font_DrawTextScaled(1, text, tx, ty + row++ * layout.line, layout.font, layout.width - 2 * layout.pad, i == g_tuneRow ? lit : colour, kTuneOverlayLayer);
    }
    snprintf(text, sizeof text, "5 row reset  6 all to opening  7 save");
    Font_DrawTextScaled(1, text, tx, ty + row++ * layout.line, layout.font, layout.width - 2 * layout.pad, colour, kTuneOverlayLayer);
    snprintf(text, sizeof text, g_tuneSavedStamp && g_tuneFrame - g_tuneSavedStamp < 100 ? "saved to opencmr2.ini; F10: next view" : "F10: hidden -> FPS only -> full; unsaved");
    Font_DrawTextScaled(1, text, tx, ty + row++ * layout.line, layout.font, layout.width - 2 * layout.pad, colour, kTuneOverlayLayer);
}
// Returns 1 when the wiper key was just pressed this frame (edge trigger),
// 0 otherwise. Resets the edge after reading.
int WindscreenWiperKeyPressed(void)
{
    int key = Sys_GetOption("video.windscreen_wiper_key", WINDSREEN_WIPER_KEY_DEFAULT);
    int down = key>0 && key<256 && Input_IsKeyDown(key) != 0 ? 1 : 0;
    // An implicit W must never also toggle the wiper when used to accelerate.
    // Explicit saved choices remain the player's responsibility and are not changed.
    if (!Sys_GetOptionText("video.windscreen_wiper_key", nullptr)) {
        bool conflict=false;
        for(int device=0;device<CInput::CountAttachedInputDevices();device++) {
            DeviceInfo *info=CInput::GetAvailableDeviceRecord(device);
            if(info && info->field_0x0==1)
                for(int action=0;action<9;action++)
                    if((&info->keyboard.field_0x468)[action]==key)conflict=true;
        }
        if(conflict)down=0;
    }
    int pressed = down && !g_windscreenWiperKeyPrevState;
    g_windscreenWiperKeyPrevState = down;
    return pressed;
}

// One input edge controls BOTH overlay and actual cockpit blades through the
// shared option. Held keys do not toggle repeatedly; independent of dirt amount.
void WindscreenWiper_UpdateInput(void)
{
    if (!WindscreenWiperKeyPressed()) return;
    int enabled = WindscreenWiper_Toggle(Sys_GetOption("video.windscreen_wipers", 1) != 0, true);
    Sys_SetOption("video.windscreen_wipers", enabled);
    g_windscreenWiperManualFrames = 0;
    for (int view=0;view<8;view++) {
        g_windscreenWipers[view].active = enabled;
        if(enabled)g_windscreenWipers[view].phase = g_windscreenWipers[view].dir = 0;
    }
}

// Draws the dirt overlay (dark tint, alpha from dirt level) and the sun lens
// flare (additive bright sprite at screen centre, scaled by sun visibility)
// over the finished frame.
static void CameraEffects_DrawOverlays(short *pRect)
{
    extern short g_sunVisibility;
    int resX = pRect[2];
    int resY = pRect[3];

    // ---- dirt on the lens -------------------------------------------------
    for (int view = 0; view < 8; view++) {
        if (g_lensDirt[view] == 0 || !Sys_GetOption("video.lens_dirt",Sys_GetOption("video.camera_effects",1)))
            continue;
        BYTE colour[4] = { 0x20, 0x18, 0x10, (BYTE)Effect_Scale(g_lensDirt[view],Sys_GetOption("video.lens_dirt_amount",100)) };
        short rect[4] = { 0, 0, (short)resX, (short)resY };
        Sprite_FillRect(&g_pGraphics->field309_0x150, rect, colour, 1);
        // Decay each frame so dirt clears after a few seconds.
        g_lensDirt[view] = LensDirt_Decay(g_lensDirt[view], 4);
    }

    // The old flat windscreen tint and 1px wiper bar were replaced by the
    // windshield-local dirt map (WindshieldDirt.cpp), drawn per cockpit view
    // in the race frame; the pause scene shows the last rendered frame.

    // ---- lens flare -------------------------------------------------------
    if (g_sunVisibility < 10 || !Sys_GetOption("video.lens_flare",Sys_GetOption("video.camera_effects",1)))
        return;
    int brightness = Effect_Scale(Flare_Brightness(g_sunVisibility),Sys_GetOption("video.lens_flare_amount",100));
    if (brightness == 0)
        return;
    // Additive sun sprite at screen centre.
    Gfx_SetAlphaBlend(TRUE);
    Gfx_SetSrcBlend(GFX_BLEND_ONE);
    Gfx_SetDestBlend(GFX_BLEND_ONE);
    Gfx_SetDepthWrite(FALSE);
    GfxTLVertex v[4];
    int cx = resX / 2, cy = resY / 2;
    int size = resX / 6;
    for (int i = 0; i < 4; i++) {
        v[i].sz = 0;
        v[i].rhw = 1.0f;
        v[i].color = GFX_RGBA((BYTE)brightness, (BYTE)brightness, (BYTE)(brightness*0xe0/255), 0xff);
        v[i].specular = 0;
    }
    v[0].sx = cx - size; v[0].sy = cy - size; v[0].tu = 0; v[0].tv = 0;
    v[1].sx = cx + size; v[1].sy = cy - size; v[1].tu = 1; v[1].tv = 0;
    v[2].sx = cx - size; v[2].sy = cy + size; v[2].tu = 0; v[2].tv = 1;
    v[3].sx = cx + size; v[3].sy = cy + size; v[3].tu = 1; v[3].tv = 1;
    Gfx_DrawPrimitiveTL(GFX_TRIANGLESTRIP, v, 4);
    Gfx_SetDepthWrite(TRUE);
    Gfx_SetAlphaBlend(FALSE);
}


extern char g_strFpsFormat0x00516e14[10];

void Scene_BeginShadowBatch(void);
void Scene_EndShadowBatch(void);
SceneNode *SceneNode_FindByType(SceneNode *pNode, unsigned int type);
void Scene_EmitNodeShadowGeometry(SceneNode *pNode, int param2, BYTE param3);
void Scene_CollectNearbyLightZones(SceneNode *pNode, int radius, short *pSector);
void StageObject_AverageWheelGroundLighting(unsigned int param1, int param2);
int Render_GetDetailDistanceScale(void);
BYTE View_GetActiveCameraFlags(BYTE index);
void View_SelectVisibleCars(unsigned int view, short *pRect);
void CarDamage_UpdateOrderedCarsOffRoadState(short *param1, short param2, int *pViewRect, int param4);
void StageObject_PositionRearViewLightNodes(unsigned int param1);
BYTE StageObject_GetCarRecordState(int index);
void StageObject_ResetCarNodeViewFlags(Car *pCar);
void StageObject_ResetCarPartNodeValues(BYTE *pCar);
void Replay_HideFinishedGhostCarNodes(void);
void StageObject_StampPendingEventDraws(int unused);
void StageObject_SpawnCarWheelSkidEffects(int param1);
void StageObject_PositionSplitViewNodes(int param1);
void StageObject_UpdateRaceSkidTrails(int param1);
void StageObject_RebuildCarExhaustPoints(int car);
void StageObject_UpdateSkidTrails(int carIndex);
void StageObject_RebuildViewWeatherLighting(int index);
void StageObject_UpdateProjectedDistanceFade(int param1);
void StageObject_RebuildOrderedWheelVisibility(void);
unsigned int NetPlayers_GetPlayerFlag23(int index);
void SceneNode_SetViewMaskTree(SceneNode *pNode, BYTE mask);
int Graphics_RenderNodeCubeMapFaces(SceneNode *pNode, int bit);
void Graphics_SetShadowGeometryState(int value);
float Graphics_GetFrameRate(void);
void StageTiming_DrawProjectedCarMarker(int param1, int param2);
void StageTiming_DrawClockAndCountdown(void);
void View_SetPlayerProjection(unsigned int player);
extern int g_viewZooms[2];   // the camera record's zoom before speed FOV
void View_GetEffectiveProjection(unsigned int view, int *zoom, int *fovY, int *fovSetting);
void View_GetFovPreview(int *preview, int *base);
void View_GetMeasuredFov(unsigned int view, double *vertDeg, double *horzDeg);
void AttractCapture_ViewSignature(int view, int *type, int *drone);
void SpeedFov_GetStatus(int view, int *pct, int *shown);
void View_GetFovDolly(unsigned int view, int *scale, int *widening);
void View_GetCockpitShift(unsigned int view, int *widening, int *step);
int View_CameraMoveStep(int cameraType, int view);
void StageObject_ApplyInterpolatedNodeMatrices(int param1);
void Race_DrawPlayerCallSlots(unsigned int player, int *pViewRect);
void Dash_Draw(int player, int *pViewRect);
void Race_ClearMessageDrawLatch(void);
int InRaceMenu_IsPlayerEditingCarSetup(unsigned int param1);

// Set while the shadow geometry of the current view has been streamed.
// GLOBAL: CMR2 0x00536ad0
int g_viewShadowStreamed;
// Screen rectangle of the view being drawn, filled by RallyData_GetViewScreenRectangle.
// GLOBAL: CMR2 0x00536ad4
int *g_viewScreenRect;
// 1 / 1024: kilobytes to megabytes.
// GLOBAL: CMR2 0x005112f8
float g_kilobytesToMegabytes = 9.765625e-04f;
// 3 / (8 * 1024 * 1024): frame buffer bytes to megabytes.
// GLOBAL: CMR2 0x005112f4
float g_frameBufferBytesToMegabytes = 3.5762786865234375e-07f;

// GLOBAL: CMR2 0x00516ce8
char g_strDrawDistance0x00516ce8[87] =
    "Draw Distance, Actual(scaled) : %d  Requested : %d  Maximum : %d  FrontEnd Setting: %d";
// GLOBAL: CMR2 0x00516d40
char g_strSoftwareTl0x00516d40[19] = "Using Software T&L";
// GLOBAL: CMR2 0x00516d54
char g_strHardwareTl0x00516d54[19] = "Using Hardware T&L";
// GLOBAL: CMR2 0x00516d68
char g_strVbMemUsed0x00516d68[30] = "VB mem used = %d K (%0.1f MB)";
// GLOBAL: CMR2 0x00516d88
char g_strVramUsed0x00516d88[66] =
    "VRAM used: Textures = %d K (%0.1f MB)   Buffers = %d K (%0.1f MB)";
// GLOBAL: CMR2 0x00516dcc
char g_strTotalPolygons0x00516dcc[22] = " Total Polygons: %d  ";
// GLOBAL: CMR2 0x00516de4
char g_strOtherPolygons0x00516de4[22] = " Other Polygons: %d  ";
// GLOBAL: CMR2 0x00516dfc
char g_strScenePolygons0x00516dfc[22] = " Scene Polygons: %d  ";

static void DebugCameraReadout_Draw(int param2)
{
    // OpenCMR2: the camera readout, big and high contrast, short lines so nothing clips. Names are only what the source states; the rest are numbers.
    if (Graphics_DebugDrawWanted() && TuningPanel_InfoShown() && !TuningPanel_FpsOnly() && (CGameInfo::IsInRaceMenuOpen() == 0 || TuningPanel_PreviewActive())) {
        int vt = 0, dr = 0, zr = 0, fy = 0, fs = 70;
        AttractCapture_ViewSignature((int)param2, &vt, &dr);
        View_GetEffectiveProjection(param2, &zr, &fy, &fs);
        double vd = 0, hd = 0, cd = 0, cf = 0, cs = 0, cu = 0;
        View_GetMeasuredFov(param2, &vd, &hd);
        View_GetCameraCarOffset(param2, &cd, &cf, &cs, &cu);
        int sp = 0, sh = 0; SpeedFov_GetStatus((int)param2, &sp, &sh);
        int eff = View_CameraMoveStep(vt, (int)param2);
        const char *name = vt == 3 ? "COCKPIT" : vt == 2 ? "BONNET" : (vt >= 4 && vt <= 6) ? (dr && vt == 5 ? "CHASE (DRONE VARIANT)" : "CHASE") : vt == 1 || vt == 10 ? "CAR-MOUNTED (UNNAMED IN SOURCE)" : "UNNAMED IN SOURCE";
        // Compact: three short lines at the debug text size, bottom-left, clear of the right-hand panel and the speedometer.
        char big[3][112];
        snprintf(big[0], 112, "Camera %s type %d | FOV %.1f v %.1f h", name, vt, vd, hd);
        snprintf(big[1], 112, "Widening x%.2f (speed FOV %d, speed %d)", sp > 0 ? 1.0 + (sp > 500 ? 500 : sp) * (sh / 65536.0) / 100.0 : 1.0, sp, sh * 100 / 65536);
        snprintf(big[2], 112, "Fwd(+)/back(-) %+.2f side %+.2f up %+.2f dist %.2f | move %+.2f", cf, cs, cu, cd, eff / 65536.0);
        int bigFont = std::clamp((int)g_pGraphics->resY / 64, 14, 24), bigLine = bigFont + bigFont / 4, pad = bigFont / 2;
        int bw = (int)g_pGraphics->resX * 46 / 100, bx = (int)g_pGraphics->resX * 2 / 100;
        int bh = 3 * bigLine + 2 * pad, by = (int)g_pGraphics->resY - bh - (int)g_pGraphics->resY * 3 / 100;
        short bp[4] = {(short)bx, (short)by, (short)bw, (short)bh};
        BYTE bbg[4] = {0, 0, 0, 170}, bfg[4] = {255, 236, 90, 255}, bwhite[4] = {255, 255, 255, 255};
        Sprite_FillRect(NULL, bp, bbg, kTuneOverlayLayer);
        for (int k = 0; k < 3; ++k) Font_DrawTextScaled(1, big[k], bx + pad, by + pad + k * bigLine, bigFont, bw - 2 * pad, k == 0 ? bfg : bwhite, kTuneOverlayLayer);
    }
}
void DebugOverlays_DrawLate(void)
{
    if(g_debugCameraView<0) return;
    TuningPanel_Draw();ReplayControls_Draw();
    DebugCameraReadout_Draw(g_debugCameraView);
    g_debugCameraView=-1;
}

// Per-view frame update of the rally data screens: prepares the cars of the
// view, streams their shadow geometry, refreshes the on-screen elements and,
// when the graphics debug flag (bit 2) is set, prints the FPS / polygon /
// VRAM overlay.
// match 78%: the logic is all there; the difference is MSVC's register
// allocation (the original keeps the raw view in EBX and the masked view in
// EBP, here it is the other way round) and the frame layout (the original
// reuses the incoming parameter slot for the overlay colour).
// FUNCTION: CMR2 0x0040f0c0
void RallyData_UpdateViewFrameAndDebugOverlay(CallbackStateMachine *param1, int param2, int param3)
{
    int orderCount;
    short *pOrder;
    short *pOrderEnd;
    int n;
    int i;
    int pixels;
    short resX;
    int resY;
    int resZ;
    SceneNode *target;
    unsigned int vramKB;
    float vramMB;
    float buffersMB;
    Car *pCar;
    SceneNode *pNode;

    int view = param2;
    int maskedView = param2 & 0xff;

    if ((BYTE)view == 0) {
        g_viewShadowStreamed = 0;
        if (Race_IsMultiplayerRecordMode10() != 0)
            return;
        if (CGame::GetGameInputFocusState() != 0) {
            RallyData_ResetDamageAndTextureCaches();
            CGame::SetGameInputFocusState(0);
        }
    }
    pOrder = Car_GetOrder();
    orderCount = Car_GetOrderCount();
    RallyData_ClearStageDepthBuffer();
    g_viewScreenRect = RallyData_GetViewScreenRectangle(maskedView);
    RallyData_ApplyPlayerFlagsToObjects(maskedView);
    StageObject_ApplyInterpolatedNodeMatrices(maskedView);
    if (CGraphics::GetRasterCapabilityField84() != 0)
        Car_ApplyViewTransforms(maskedView);
    Car_UpdateViewNodes(maskedView);
    StageObject_RebuildViewWeatherLighting(maskedView);
    StageObject_PositionSplitViewNodes(maskedView);
    if ((BYTE)view == 0 && (BYTE)RallyDataState() == 1 &&
        CGameInfo::IsInRaceMenuOpen() == 0 &&
        (StageTiming_GetStartTableRecord(0)->detailCode == 'A' || StageTiming_GetStartTableRecord(0)->detailCode == 'C')) {
        StageObject_SpawnCarWheelSkidEffects(0);
        StageObject_StampPendingEventDraws(0);
    }
    for (i = 0; i < orderCount; i++)
        StageObject_AverageWheelGroundLighting(i, maskedView);
    View_SelectVisibleCars(maskedView, (short *)g_viewScreenRect);
    Replay_HideFinishedGhostCarNodes();
    StageObject_PositionRearViewLightNodes(View_GetActiveCameraFlags(maskedView));
    Game_PrepareScene(RallyData_GetChallengeRenderState(), g_viewNodes[maskedView],
                      g_viewScreenRect, maskedView);
    if (g_viewShadowStreamed == 0) {
        g_viewShadowStreamed = 1;
        Scene_BeginShadowBatch();
        n = orderCount;
        if (orderCount - 1 >= 0) {
            pOrderEnd = &pOrder[orderCount - 1];
            do {
                pCar = Car_Get(*pOrderEnd);
                pNode = pCar->pAlternateBodyNode;
                if (pNode != NULL && pNode->viewMask > 0)
                    Scene_CollectNearbyLightZones(pCar->pAlternateBodyNode, pCar->collisionRadius,
                                 &pCar->sector);
                else
                    Scene_CollectNearbyLightZones(pCar->pSceneRoot, pCar->collisionRadius,
                                 &pCar->sector);
                Scene_EmitNodeShadowGeometry(pCar->pBodyNode, pCar->field_0xa70,
                             StageObject_GetCarRecordState(pCar->index));
                Scene_EmitNodeShadowGeometry(pCar->pSceneRoot, pCar->field_0xa70,
                             StageObject_GetCarRecordState(pCar->index));
                if (pCar->pAlternateBodyNode != NULL)
                    Scene_EmitNodeShadowGeometry(pCar->pAlternateBodyNode, pCar->field_0xa70,
                                 StageObject_GetCarRecordState(pCar->index));
                pOrderEnd--;
            } while (--n != 0);
        }
        Scene_EndShadowBatch();
    }
    StageObject_RebuildOrderedWheelVisibility();
    if (CGameInfo::GetGameModeOptionBit19() != 0) {
        for (i = 0; i < 7; i++) {
            if ((BYTE)NetPlayers_IsPlayerPresent(i) == 0 && (BYTE)NetPlayers_GetPlayerFlag23(i) != 0)
                StageObject_ResetCarNodeViewFlags(Car_Get(NetPlayers_GetPlayerField8(i)));
        }
    }
    for (i = orderCount - 1; i >= 0; i--) {
        pCar = Car_Get(pOrder[i]);
        if (!CGameInfo::IsRecordFlagSet(0x10) && i != 0) {
            StageObject_ResetCarNodeViewFlags(pCar);
            continue;
        }
        if ((g_pGraphics->field913_0x3bc & 0x80) != 0) {
            pNode = pCar->pAlternateBodyNode;
            if (pNode != NULL && pNode->viewMask > 0) {
                SceneNode_SetViewMaskTree(pCar->pSceneRoot, 0);
                SceneNode_SetViewMaskTree(pCar->pAlternateBodyNode, 0);
                Graphics_RenderNodeCubeMapFaces(pCar->pAlternateBodyNode, maskedView);
            } else {
                SceneNode_SetViewMaskTree(pCar->pSceneRoot, 0);
                SceneNode_SetViewMaskTree(pCar->pBodyNode, 0);
                Graphics_RenderNodeCubeMapFaces(pCar->pBodyNode, maskedView);
            }
        }
        pNode = pCar->pAlternateBodyNode;
        if (pNode != NULL && pNode->viewMask > 0) {
            target = SceneNode_FindByType(pCar->pAlternateBodyNode, 0x14);
            if (CGame::GetObjectRenderMode() != 0) {
                SceneNode_SetViewMaskTree(pCar->pSceneRoot, 0);
                SceneNode_SetViewMaskTree(pCar->pAlternateBodyNode, 0);
                if (target != 0)
                    SceneNode_SetViewMaskTree(target, 1);
            } else {
                SceneNode_SetViewMaskTree(pCar->pSceneRoot, 1);
                SceneNode_SetViewMaskTree(pCar->pAlternateBodyNode, 1);
                if (target != 0)
                    SceneNode_SetViewMaskTree(target, 0);
            }
        } else {
            target = SceneNode_FindByType(pCar->pBodyNode, 0x14);
            if (CGame::GetObjectRenderMode() != 0) {
                SceneNode_SetViewMaskTree(pCar->pSceneRoot, 0);
                SceneNode_SetViewMaskTree(pCar->pBodyNode, 0);
                if (target != 0)
                    SceneNode_SetViewMaskTree(target, 1);
            } else {
                SceneNode_SetViewMaskTree(pCar->pSceneRoot, 1);
                SceneNode_SetViewMaskTree(pCar->pBodyNode, 1);
                if (target != 0)
                    SceneNode_SetViewMaskTree(target, 0);
            }
        }
        StageObject_ResetCarNodeViewFlags(pCar);
        StageObject_ResetCarPartNodeValues((BYTE *)pCar);
    }
    Graphics_SetShadowGeometryState(!CGameInfo::IsRecordFlagSet(0xf));
    View_SetPlayerProjection(param2);
    if (Graphics_DebugDrawWanted() && TuningPanel_InfoShown()) {
        if(TuningPanel_FpsOnly()||Sys_GetOption("video.debug_draw",OPENCMR2_DEBUG_DRAW_DEFAULT)==1){
            char text[32];snprintf(text,sizeof text,"FPS %.1f",Graphics_GetFrameRate());BYTE color[4]={240,246,255,255};
            BYTE background[4]={12,16,22,220};short panel[4]={8,8,128,22};
            Sprite_FillRect(NULL,panel,background,1);
            Font_DrawTextScaled(1,text,12,12,14,120,color);
        }else{
        // Compact single-view diagnostics: left edge, away from central pace
        // notes and the right-hand race timer. Real glyph height controls scale.
        BYTE background[4] = {12, 16, 22, 220};
        BYTE colour[4] = {240, 246, 255, 255};
        unsigned long long tick=0;unsigned frame=0,changes=0;
        bool spectator=SceneryPack_GetAnimationStatus(&tick,&frame,&changes);
        auto layout=DebugOverlayLayout::For(g_pGraphics->resX,g_pGraphics->resY,(spectator?6:5)+11+5+2+5+2+2);   // +5: the Speed FOV, FOV dolly, FOV preview and footer rows were drawn below the panel in the 12:04 native screenshot (panel ended at "Measured")
        short panel[4] = {(short)layout.x,(short)layout.y,(short)layout.width,(short)layout.height};
        Sprite_FillRect(NULL, panel, background, 1);
        char text[160];
        snprintf(text, sizeof(text), "FPS %.1f | draws %u | mem %lluM", Graphics_GetFrameRate(), Gfx_GetFrameDrawCalls(), Sys_GetProcessMemoryBytes() >> 20);
        Font_DrawTextScaled(1,text,layout.x+layout.pad,layout.y+layout.pad,layout.font,layout.width-2*layout.pad,colour);
        snprintf(text, sizeof(text), "Triangles %u | vertices %u", Gfx_GetFrameTriangles(), Gfx_GetFrameVertices());
        Font_DrawTextScaled(1,text,layout.x+layout.pad,layout.y+layout.pad+layout.line,layout.font,layout.width-2*layout.pad,colour);
        if(spectator){
            snprintf(text,sizeof(text),"Crowd tick %llu | frame %u | changes %u",tick,frame,changes);
            Font_DrawTextScaled(1,text,layout.x+layout.pad,layout.y+layout.pad+2*layout.line,layout.font,layout.width-2*layout.pad,colour);
        }
        {   // Per-frame scenery counts (differences of running totals; counts, not timing).
            static unsigned long long pcd=0,pct=0,ptt=0,ptu=0;unsigned long long cd=0,ct=0,tt=0,tu=0;
            SceneryPack_GetFrameTotals(&cd,&ct,&tt,&tu);
            snprintf(text,sizeof(text),"This frame: crowd %llu cards %llu tris | trees %llu tris, %llu rustled verts",cd-pcd,ct-pct,tt-ptt,tu-ptu);
            pcd=cd;pct=ct;ptt=tt;ptu=tu;
            Font_DrawTextScaled(1,text,layout.x+layout.pad,layout.y+layout.pad+(spectator?4:3)*layout.line,layout.font,layout.width-2*layout.pad,colour);        }
        {   // Wall-clock ms spent in the scenery sidecar draws last frame (diagnostic build), next to the frame time.
            static unsigned long long prn=0,ptn=0,pcl=0;unsigned long long rn=0,tn=0,cl=0;
            SceneryPack_GetTimeTotals(&rn,&tn,&cl);
            static unsigned long long pwm=0,psb=0;unsigned long long wm=0,sb=0;SceneryPack_GetTimeSplit(&wm,&sb);
            snprintf(text,sizeof(text),"Scenery CPU ms: all %.2f | whole trees %.2f (wind math %.2f, record draws %.2f) %llu calls",(tn-ptn)/1e6,(rn-prn)/1e6,(wm-pwm)/1e6,(sb-psb)/1e6,cl-pcl);
            pwm=wm;psb=sb;
            prn=rn;ptn=tn;pcl=cl;
            Font_DrawTextScaled(1,text,layout.x+layout.pad,layout.y+layout.pad+(spectator?5:4)*layout.line,layout.font,layout.width-2*layout.pad,colour);
        }
        static char hitchText[200]="";
        {   // Same-interval split (diagnostic): everything below is a difference between two consecutive pane draws, so all numbers cover the same frame.
            static double pt=0,pe=0,pw=0;static unsigned long long ptn2=0;
            double now=Sys_GetTicksPrecise(),ex=0,wt=0;Gfx_GetPresentTimeTotals(&ex,&wt);
            unsigned long long rn=0,tn=0,cl=0;SceneryPack_GetTimeTotals(&rn,&tn,&cl);
            double interval=pt>0?now-pt:0,sc=(tn-ptn2)/1e6;
            snprintf(text,sizeof(text),"Frame split ms: interval %.1f | executor encode+submit %.1f (swapchain wait %.1f) | scenery %.1f | rest %.1f",interval,ex-pe,wt-pw,sc,interval-(ex-pe)-sc);
            {   // Worst single frame over ~5 s windows (diagnostic): hitches that a one-frame readout misses. Timing only, no cause claimed.
                static double winStart=0,worst=0,wEx=0,wSc=0,lastWorst=0,lWEx=0,lWSc=0;static unsigned over25=0,frames=0,lastOver=0,lastFrames=0;
                const unsigned long long presents=Gfx_GetPresentCount();static unsigned long long prevPresents=0;
                if(prevPresents==0||presents!=prevPresents+1){   // frames were presented without this pane (pane off, menu, pause): restart the window, but keep a genuine long single frame
                    winStart=0;worst=0;over25=0;frames=0;lastFrames=0;snprintf(hitchText,sizeof(hitchText),"Hitch block: collecting 5 s after pane/menu gap");interval=0;
                } else if(interval>0){
                    if(winStart==0)winStart=now;
                    frames++;if(interval>25.0)over25++;
                    if(interval>worst){worst=interval;wEx=ex-pe;wSc=sc;}
                }
                prevPresents=presents;
                if(now-winStart>=5000.0&&winStart>0){lastWorst=worst;lWEx=wEx;lWSc=wSc;lastOver=over25;lastFrames=frames;worst=0;over25=0;frames=0;winStart=now;}
                if(lastFrames)snprintf(hitchText,sizeof(hitchText),"Last full 5 s block: worst frame %.1f ms (executor %.1f, scenery %.1f) | over 25 ms: %u of %u",lastWorst,lWEx,lWSc,lastOver,lastFrames);
            }
            pt=now;pe=ex;pw=wt;ptn2=tn;
            Font_DrawTextScaled(1,text,layout.x+layout.pad,layout.y+layout.pad+(spectator?6:5)*layout.line,layout.font,layout.width-2*layout.pad,colour);
        }
        unsigned treeDraws=0,windDraws=0,originalTrees=0;
        SceneryPack_GetTreeStatus(&treeDraws,&windDraws,&originalTrees);
        snprintf(text,sizeof(text),"Tree draws %u | wind %u | original %u",treeDraws,windDraws,originalTrees);
        Font_DrawTextScaled(1,text,layout.x+layout.pad,layout.y+layout.pad+(spectator?3:2)*layout.line,layout.font,layout.width-2*layout.pad,colour);
        int row=spectator?7:6;
        auto line=[&](const char*s,const BYTE*c=nullptr){Font_DrawTextScaled(1,(char*)s,layout.x+layout.pad,layout.y+layout.pad+row++*layout.line,layout.font,layout.width-2*layout.pad,(BYTE*)(c?c:colour));};
        {
            // (Placed first: the panel has a fixed height and rows after the atmosphere block were clipped.)
            // Windshield (cockpit dirt/cracks) state, so a screenshot shows why nothing is drawn: option, glass lookup, overlay draw gating.
            unsigned wsCalls=0,wsDrawn=0;int wsReason=-1,wsGlass=-1,wsDirt=0,wsSlot=-1;
            int mv=(int)(param2&1);
            Windshield_GetDiag(mv,&wsCalls,&wsDrawn,&wsReason,&wsGlass,&wsDirt);
            bool wsResolved=WindshieldGlass_GetInfo(mv,NULL,&wsSlot,NULL,NULL,NULL)!=0;
            const char *wsWhy[]={"drawing","broken glass","no dirt"};
            static const BYTE wsLit[4]={110,255,140,255},wsDim[4]={150,160,170,255};
            // Two short rows so the decisive reason is never clipped by the panel width (about 88 characters).
            snprintf(text,sizeof text,"Windshield: dirt %d wipe %d crack %d | glass %s %d",
                Sys_GetOption("video.windscreen_dirt",40),Sys_GetOption("video.windscreen_wipers",1),Sys_GetOption("video.windshield_cracks",1),
                (wsResolved&&Sys_GetOption("video.windshield_crack_overlay",0)==0)?"ok":WindshieldGlass_FailReason(mv),wsGlass);
            line(text,wsResolved?wsLit:wsDim);
            snprintf(text,sizeof text,"Windshield overlay: draw %u/%u | dirt %d | %s",wsDrawn,wsCalls,wsDirt,wsReason>=0&&wsReason<3?wsWhy[wsReason]:"never reached");
            line(text,wsDrawn>0?wsLit:wsDim);
        }
        if(hitchText[0])line(hitchText);
        for(int i=0;i<3;i++)line(Sys_GetLaunchSummary(i));
        GfxEffectStatus status=Gfx_GetEffectStatus();
        auto effect=[&](unsigned bit){return status.submitted<0?"?":!(status.requested&bit)?"off":status.encoded&bit?"enc":"skip";};
        snprintf(text,sizeof text,"AA:%s bloom:%s blur:%s",effect(1),effect(2),effect(4));line(text);
        snprintf(text,sizeof text,"DoF:%s rays:%s grade:%s",effect(8),effect(16),effect(32));line(text);
        snprintf(text,sizeof text,"shadow:%s rain/snow:%s",effect(64),effect(128));line(text);
        snprintf(text,sizeof text,"submission:%s",status.submitted<0?"unknown":status.submitted?"accepted":"failed");line(text);
        snprintf(text,sizeof text,"MetalFX:%s",status.metalFX<0?"off/unknown":status.metalFX?"encoded":"fallback");line(text);
        const char *raysReasons[]={"unknown","off","encoded","no single world view",
            "sun offscreen or unavailable","sampled depth unavailable","amount zero","resource or pass unavailable"};
        int reason=Gfx_GetRaysStatus();
        snprintf(text,sizeof text,"rays reason:%s",reason>=0&&reason<8?raysReasons[reason]:"unknown");line(text);
        {
            // Effects light up green only while contributing: encoded this frame, with a nonzero amount.
            BYTE lit[4]={110,255,140,255},dim[4]={150,160,170,255};
            GfxDofStatus dof=Gfx_GetDofStatus();
            const char *dofWhy[]={"unknown","depth_of_field off","no sampled depth","range or blur is 0","pass unavailable","encoded","not a race frame"};
            int tv=Sys_GetOption("video.tunnel_vision",130);
            bool gl=false,dofOn=dof.reason==5;   // both backends report this status now
            if(gl)snprintf(text,sizeof text,"DoF pass: n/a (OpenGL backend)");
            else if(dofOn&&dof.blur<=0.0f)snprintf(text,sizeof text,"DoF pass: encoded for tunnel vision only (depth blur 0)");
            else if(dofOn&&dof.carDepth>0.0f)snprintf(text,sizeof text,"DoF pass: encoded, range %.2f blur %.1f, focus on car, depth %.1f world units (pos %.2f)",dof.range,dof.blur,dof.carDepth,dof.focus);
            else if(dofOn&&dof.focus>0.0f)snprintf(text,sizeof text,"DoF pass: encoded, range %.2f blur %.1f, fixed focus %.2f",dof.range,dof.blur,dof.focus);
            else if(dofOn)snprintf(text,sizeof text,"DoF pass: encoded, range %.2f blur %.1f",dof.range,dof.blur);
            else snprintf(text,sizeof text,"DoF pass: off (%s)",dofWhy[dof.reason>=0&&dof.reason<7?dof.reason:0]);
            if(dofOn&&Sys_GetOption("video.dof_focus_car",0)!=0&&dof.carDepth<=0.0f){size_t l=strlen(text);snprintf(text+l,sizeof text-l," [car focus unavailable: fixed plane]");}
            line(text,dofOn?lit:dim);
            bool tvOn=dofOn&&tv>0&&dof.smooth>0.02f;
            if(gl)snprintf(text,sizeof text,"Tunnel vision: n/a (OpenGL backend)");
            else if(!dofOn)snprintf(text,sizeof text,"Tunnel vision: off (needs DoF pass)");
            else if(tv<=0)snprintf(text,sizeof text,"Tunnel vision: off (video.tunnel_vision=0)");
            else snprintf(text,sizeof text,"Tunnel vision: %s %d%% (speed target %d%%)",tvOn?"encoded":"idle",(int)(dof.smooth*100.0f+0.5f),(int)(dof.target*100.0f+0.5f));
            line(text,tvOn?lit:dim);
            if(Sys_GetOption("video.blur_mode",0)!=0){
                if(dof.blurMode==1)snprintf(text,sizeof text,"Blur mode: whole scene except car; car rect u %.2f..%.2f v %.2f..%.2f (bounding rectangle + depth range, not a silhouette)",dof.carU0,dof.carU1,dof.carV0,dof.carV1);
                else snprintf(text,sizeof text,"Blur mode: whole scene requested but %s: peripheral used",dofOn?"no car region (cockpit/hidden car) or backend without it":"DoF pass not encoded");
            }else snprintf(text,sizeof text,"Blur mode: peripheral (video.blur_mode=0)");
            line(text,dof.blurMode==1?lit:dim);
            bool mb=!gl&&(status.encoded&4)!=0;
            snprintf(text,sizeof text,"Motion blur (separate effect): %s",mb?"encoded":"off");
            line(text,mb?lit:dim);
            {
                int ss=Sys_GetOption("video.speed_streak",0);bool ssOn=(status.encoded&256)!=0;
                if(ss<=0)snprintf(text,sizeof text,"Speed streak: off (video.speed_streak=0)");
                else snprintf(text,sizeof text,"Speed streak: %s (setting %d%%, centre zone kept clear)",ssOn?"encoded":"idle",ss);
                line(text,ssOn?lit:dim);
            }
            {
                bool fogEnc=(status.encoded&512)!=0;
                if(Sys_GetOption("video.fog",0)==0)snprintf(text,sizeof text,"Fog: off (video.fog=0)");
                else snprintf(text,sizeof text,"Fog: %s (amount %d%%, glow %d%%)",fogEnc?"encoded":"requested, not encoded",Sys_GetOption("video.fog_amount",65),Sys_GetOption("video.fog_glow",60));
                line(text,fogEnc?lit:dim);
            if(fogEnc)
            {   // Diagnostic (cockpit fog): how the fog pass got view distance this frame, and what the flagged interior parts did.
                // Not a pixel sample: it cannot say what depth the windscreen pixels hold. "no glass profile" above is the windscreen
                // dirt/crack overlay and is unrelated to this classification.
                extern unsigned g_interiorDiag[4];
                static unsigned prev[4]={0,0,0,0},delta[4]={0,0,0,0};
                if(memcmp(prev,g_interiorDiag,sizeof prev)!=0){for(int di=0;di<4;++di){delta[di]=g_interiorDiag[di]-prev[di];prev[di]=g_interiorDiag[di];}}
                GfxFogDepthStatus fd=Gfx_GetFogDepthStatus();
                const char *mode=fd.mode==1?"single projection (true distance)":fd.mode==2?"AMBIGUOUS projection (raw-depth ramp)":fd.mode==3?"no usable projection (raw-depth ramp)":"no data";
                snprintf(text,sizeof text,"Fog depth: %s, %d near/far pair(s), dominant near %.3f far %.1f (%d draws)",mode,fd.pairs,fd.nearZ,fd.farZ,fd.votes);
                line(text,fd.mode==1?lit:dim);
                snprintf(text,sizeof text,"Flagged-interior draws (last change; counts, not windscreen proof): no-depth-write %u, blend-off %u, no-semi-alpha %u, batch-path meshes %u (cockpit_glass_fog=%d)",delta[0],delta[1],delta[2],delta[3],Sys_GetOption("video.cockpit_glass_fog",1));
                line(text,delta[0]>0?lit:dim);
            }
            }
            {
                int al=0;unsigned ad=0;AirMote_GetStats(&al,&ad);
                if(Sys_GetOption("video.air_motes",0)==0)snprintf(text,sizeof text,"Air motes: off (video.air_motes=0)");
                else snprintf(text,sizeof text,"Air motes: live %d, queued %u total (queued is not visible pixels)",al,ad);
                line(text,Sys_GetOption("video.air_motes",0)&&al>0?lit:dim);
            }
            {
                int tagged=0,applied=0;SnowMaterial_GetStats(&tagged,&applied);
                int sm=Sys_GetOption("video.snow_material",0);
                if(sm<=0)snprintf(text,sizeof text,"Snow material: off (video.snow_material=0)");
                else snprintf(text,sizeof text,"Snow material: %s, %d ground vertices tagged snow (setting %d%%)",applied?"lifting":"tagged, no lift",tagged,sm);
                line(text,sm>0&&applied?lit:dim);
            }
        }
        {
            int viewType=0,drone=0,zoomRaw=0,fovYRaw=0,fovSet=70;
            AttractCapture_ViewSignature((int)param2,&viewType,&drone);
            View_GetEffectiveProjection(param2,&zoomRaw,&fovYRaw,&fovSet);
            if(drone&&viewType==5)snprintf(text,sizeof text,"Camera: type %d drone (live, view %d), video.drone_distance %d%% in effect",viewType,(int)param2,Sys_GetOption("video.drone_distance",50));
            else snprintf(text,sizeof text,"Camera: type %d%s (live, view %d)",viewType,(drone&&viewType==5)?" drone":"",(int)param2);
            line(text);
            snprintf(text,sizeof text,"video.fov number %d (legacy scale, 70 original, higher = wider; not an angle); zoom x%.2f (camera base x%.2f), proj scale %.2f",fovSet,zoomRaw/65536.0,g_viewZooms[param2&1]/65536.0,fovYRaw/65536.0);
            line(text);
            {double vd=0,hd=0;View_GetMeasuredFov(param2,&vd,&hd);snprintf(text,sizeof text,"Measured from the projection matrix: vertical %.1f deg, horizontal %.1f deg (this view, now)",vd,hd);line(text);}
            {int pv=0,pbase=70;View_GetFovPreview(&pv,&pbase);
             if(pv>0){snprintf(text,sizeof text,"FOV PREVIEW video.fov number %d (debug; saved %d; x%.2f vs saved, higher = wider). Speed FOV %% below is separate.",pv,pbase,FovPreview_Magnification(pv,pbase));line(text);}}
            int sfPct=0,sfShown=0;SpeedFov_GetStatus((int)param2,&sfPct,&sfShown);
            snprintf(text,sizeof text,"Speed FOV %d%% (eased speed %d%%%s)",sfPct,sfShown*100/65536,Sys_GetOption("video.speed_fov_hold",0)?", HELD at full speed for FOV and tunnel (overdo test)":"");
            line(text);
            {int dollyScale=0,widening=0;View_GetFovDolly(param2,&dollyScale,&widening);
             if(viewType>=4&&viewType<=6){snprintf(text,sizeof text,"FOV dolly %d%%: view widened x%.2f vs saved, camera distance x%.2f (visual only, chase cameras)",Sys_GetOption("video.fov_dolly",150),widening/65536.0,dollyScale/65536.0);}
             else{snprintf(text,sizeof text,"FOV dolly %d%%: not applied to camera type %d (chase types 4-6 only)",Sys_GetOption("video.fov_dolly",150),viewType);}
             line(text);
             {int eff=View_CameraMoveStep(viewType,(int)param2);const char*k=(viewType>=4&&viewType<=6)?"video.camera_move_chase":viewType==3?"video.camera_move_cockpit":"video.camera_move_car";char tk2[40];snprintf(tk2,sizeof tk2,"video.camera_move_type%d",viewType);int mv=CameraMove_Effective(Sys_GetOption(tk2,kCamMoveUnset),Sys_GetOption(k,0));snprintf(text,sizeof text,"Camera move (%s) %d steps at full speed; now %+.2f model units along camera forward (0 standing still; + forward, - back)",k+6,mv,eff/65536.0);line(text);}
             if(viewType==3){int cw=0,cs=0;View_GetCockpitShift(param2,&cw,&cs);snprintf(text,sizeof text,"Cockpit eye shift setting %d: view widened x%.2f, eye moved %+.3f model units along camera forward (0.2 = driver mount height)",Sys_GetOption("video.cockpit_eye_shift",0),cw/65536.0,cs/65536.0);line(text);}}
        }
        line("enc=encoded; skip=requested, not encoded");
        snprintf(text,sizeof text,"Sun glare %d%%: restrained eye response; sun/depth/cloud gated (not pixel proof)",Sys_GetOption("video.sun_glare",0));line(text);
        line("Previous frame; encoded is not pixel proof");
        }
    }
    DebugOverlays_RequestView(param2);
    CarDamage_UpdateOrderedCarsOffRoadState(pOrder, (short)orderCount, g_viewScreenRect, maskedView);
    if (CGameInfo::GetGameModeOptionBit19() != 0) {
        if (CGameInfo::GetConfiguredGameMode() != 0xa) {
            if ((char)((CallbackStateMachine *)StageUI_GetRaceResultTable())->records[0].packedState == '\n')
                StageTiming_DrawProjectedCarMarker(0, 0);
            for (i = 0; i < 7; i++) {
                if ((BYTE)NetPlayers_IsPlayerPresent(i) != 0)
                    StageTiming_DrawProjectedCarMarker(NetPlayers_GetPlayerField8(i), 0);
            }
        }
        StageTiming_DrawClockAndCountdown();
    }
    if (RallyData_GetSelectionFlag28() != 0 && CGameInfo::GetSoundOptionBit30() != 0)
        StageObject_UpdateProjectedDistanceFade(maskedView);
    if (CGameInfo::IsInRaceMenuOpen() == 0) {
        StageObject_RebuildCarExhaustPoints(maskedView);
        StageObject_UpdateSkidTrails(maskedView);
    }
    StageObject_UpdateRaceSkidTrails(maskedView);
    Race_ClearMessageDrawLatch();
    // OpenCMR2: windshield-local dirt/wetness (video.windscreen_dirt /
    // video.windscreen_wipers): cockpit camera only, before the HUD so the
    // HUD stays clean. Chase and other cameras never see the glass overlay.
    {   int viewType = 0, drone = 0;
        AttractCapture_ViewSignature(param2, &viewType, &drone);
        if (viewType == 3 && param3 != 0) {
            Windshield_UpdateView(maskedView, maskedView < 2 ? maskedView : 0);
            Windshield_DrawOverlay(maskedView, g_viewScreenRect[2], g_viewScreenRect[3]);
        }
    }
    if (CGameInfo::IsInRaceMenuOpen() != 0 && InRaceMenu_IsPlayerEditingCarSetup(maskedView) == 0)
        return;
    if ((BYTE)RallyData_IsChampionshipFinalStage() != 0)
        return;
    if ((BYTE)RallyDataState() == 1 && param3 != 0) {
        // PORT: the HUD laid out for the screen, element by element.
        Hud_BeginView(maskedView);
        Dash_Draw(maskedView, g_viewScreenRect);
        Hud_Element();
        RallyData_DrawAndHandleCarHUD(maskedView, (short *)g_viewScreenRect);
        Hud_Element();
        Race_DrawPlayerCallSlots(maskedView, g_viewScreenRect);
        Hud_EndView();
    }
    if ((BYTE)RallyDataState() == 2 && param3 != 0) {
        // PORT: the HUD laid out for the screen, element by element.
        Hud_BeginView(maskedView);
        Dash_Draw(maskedView, g_viewScreenRect);
        Hud_Element();
        RallyData_DrawAndHandleCarHUD(maskedView, (short *)g_viewScreenRect);
        Hud_Element();
        Race_DrawPlayerCallSlots(maskedView, g_viewScreenRect);
        Hud_EndView();
    }
}

// Finds the route node nearest to a car (only nodes within 100.0 in both
// ground axes are considered) and stores it as the car's current and previous
// progress record.
// FUNCTION: CMR2 0x00420850
void RallyData_FindNearestCarRouteNode(Car *pCar)
{
    FixVector pos;
    FixVector d;
    unsigned int i;
    int best;
    unsigned int bestIndex;
    int dx;
    int dz;

    best = 0x7d000000;
    bestIndex = 0;
    if (g_unk0x00538a94 != 0) {
        for (i = 0; i < g_routeNodeCount; i++) {
            RallyData_GetRouteNodeGroundPosition(i, (int *)&pos);
            dx = pos.x - pCar->position.x;
            d.x = dx;
            dz = pos.z - pCar->position.z;
            d.y = 0;
            d.z = dz;
            if (FIX_ABS(dx) < 0x640000 && FIX_ABS(dz) < 0x640000) {
                int len = FixVecLength(&d);
                if (len < best) {
                    best = len;
                    bestIndex = i;
                }
            }
        }
        if (best != 0x7d000000) {
            g_raceRecords[pCar->index].routeNodeIndex = bestIndex;
            g_raceRecords[pCar->index].resetRouteNodeIndex = bestIndex;
        }
    }
}

char *Race_GetStagePathBuffer38340(void);
GenericFile *Race_GetLoadedStageFile(void);

// Textures of the challenge screens (net-play blob of each player and the blob
// of the current challenge).
// GLOBAL: CMR2 0x00537070
Texture *g_challengeTextures;
// GLOBAL: CMR2 0x00537074
Texture *g_unk0x00537074;
// GLOBAL: CMR2 0x00537078
Texture *g_unk0x00537078;
// GLOBAL: CMR2 0x0053707c
Texture *g_unk0x0053707c;
// GLOBAL: CMR2 0x00517d38
char g_strChallengeTrackName0x00517d38[22] = "%s\\chall\\cha%.2d.tga";
// GLOBAL: CMR2 0x00517d50
char g_strChalblobTga0x00517d50[33] = "\\NEWIMAGE\\osd\\chall\\chalblob.tga";
// GLOBAL: CMR2 0x00517d74
char g_strNetblobrTga0x00517d74[33] = "\\NEWIMAGE\\osd\\chall\\netblobr.tga";
extern char g_strPathConcat[];
// GLOBAL: CMR2 0x00517da0
char g_strNetblobTga0x00517da0[32] = "\\NEWIMAGE\\osd\\chall\\netblob.tga";

// Loads the textures of the challenge screens: the net-play blobs for the two
// network challenge screens and the challenge blob of the current track (the
// track one is built from the install dir or, for the DD/network tracks, from
// the name of the loaded file with the extension changed to .DDS).
// FUNCTION: CMR2 0x00411280
void RallyData_LoadChallengeScreenTextures(void)
{
    bool loaded;
    int track;

    if (CGameInfo::GetConfiguredGameMode() == 8 || CGameInfo::GetConfiguredGameMode() == 9) {
        sprintf(CFrontend::m_stringDest, g_strPathConcat, CInstallInfo::GetTexturesDirectory(),
                g_strNetblobTga0x00517da0);
        g_challengeTextures = CTexture::FindLoadTexture((GenericFile *)StageTiming_GetStageFile1(),
                                                    CFrontend::m_stringDest, &loaded, 0, 0, 0);
        sprintf(CFrontend::m_stringDest, g_strPathConcat, CInstallInfo::GetTexturesDirectory(),
                g_strNetblobrTga0x00517d74);
        g_unk0x00537074 = CTexture::FindLoadTexture((GenericFile *)StageTiming_GetStageFile1(),
                                                    CFrontend::m_stringDest, &loaded, 0, 0, 0);
    }
    if (CGameInfo::GetConfiguredGameMode() != 5 && CGameInfo::GetConfiguredGameMode() != 6 &&
        CGameInfo::GetConfiguredGameMode() != 7 && CGameInfo::GetConfiguredGameMode() != 4 &&
        CGameInfo::GetConfiguredGameMode() != 0xb && CGameInfo::GetConfiguredGameMode() != 0xc &&
        (BYTE)RallyData_GetFlag25() == 0)
        return;
    sprintf(CFrontend::m_stringDest, g_strPathConcat, CInstallInfo::GetTexturesDirectory(),
            g_strChalblobTga0x00517d50);
    g_unk0x00537078 = CTexture::FindLoadTexture((GenericFile *)StageTiming_GetStageFile1(),
                                                CFrontend::m_stringDest, &loaded, 0, 0, 0);
    if (CGameInfo::GetConfiguredGameMode() == 4 || (BYTE)RallyData_GetFlag25() != 0) {
        sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue, Race_GetStagePathBuffer38340());
        track = strlen(CFrontend::m_stringDest);
        CFrontend::m_stringDest[track - 2] = '.';
        CFrontend::m_stringDest[track - 1] = 'D';
        CFrontend::m_stringDest[track] = 'D';
        CFrontend::m_stringDest[track + 1] = 'S';
        CFrontend::m_stringDest[track + 2] = '\0';
    } else {
        track = (RallyData_GetSelectionBits10To11() & 0xff) * 3;
        track += RallyData_GetSelectionBits12To13() & 0xff;
        sprintf(CFrontend::m_stringDest, g_strChallengeTrackName0x00517d38,
                CInstallInfo::GetTracksDir(), track + 1);
    }
    g_unk0x0053707c = CTexture::FindLoadTexture(Race_GetLoadedStageFile(), CFrontend::m_stringDest, &loaded, 0,
                                                0, 0);
}

DWORD Network_GetLocalPlayerID(void);
int NetPlayers_FindPlayerByField8(int value);
int NetPlayers_GetPlayerID(int index);
BYTE *NetPlayers_GetPlayerNameColour(int id);

// Draws the two sprites (shadow and icon) of a race-position marker, clamping
// the arrow's x span to the marker track width read from the HUD rect.
// match 44%: same instruction count and logic; only register allocation and
// load scheduling differ from the original (MSVC6 slot assignment).
// FUNCTION: CMR2 0x00413e00
void RallyData_DrawRacePositionMarker(short *param_1, int param_2)
{
    short *pMarker;
    Texture *pTexture;
    SpriteRect src;
    SpriteRect dst;
    int local_8;
    short diff;
    int i;

    pMarker = param_1 + g_playerSplitPositions[param_2][0] * 4;
    local_8 = (int)pMarker[2] << 16;
    local_8 = FixMul(local_8, g_playerSplitPositions[param_2][1]) >> 16;
    pTexture = g_challengeTextures;
    dst.x = (short)(local_8 - pTexture->width / 2 + pMarker[0]);
    dst.y = pMarker[1];
    dst.w = pTexture->width;
    dst.h = pTexture->height;
    src = *(SpriteRect *)&pTexture->field_0x11c;

    i = (int)*param_1 - (int)dst.x;
    if (i > 0) {
        diff = (short)i;
        src.x += diff;
        dst.x += diff;
        dst.w -= diff;
        src.w -= diff;
    }
    i = (int)dst.w - (int)param_1[g_splitCount * 4 - 4]
        - (int)param_1[g_splitCount * 4 - 2] + (int)dst.x;
    if (0 < i) {
        diff = (short)i;
        dst.w -= diff;
        src.w -= diff;
    }

    if (param_2 == 0) {
        Sprite_Queue(&src, &dst, g_challengeTextures, 2, 0, 0, 0,
                     NetPlayers_GetPlayerNameColour((int)Network_GetLocalPlayerID()), 8);
        dst.y = (short)((pMarker[3] - g_challengeTextures->height) + pMarker[1]);
        Sprite_Queue(&src, &dst, g_unk0x00537074, 2, 0, 0, 0,
                     NetPlayers_GetPlayerNameColour((int)Network_GetLocalPlayerID()), 8);
        return;
    }
    Sprite_Queue(&src, &dst, g_challengeTextures, 2, 0, 0, 0,
                 NetPlayers_GetPlayerNameColour(NetPlayers_GetPlayerID(NetPlayers_FindPlayerByField8(param_2))), 8);
    dst.y = (short)((pMarker[3] - g_challengeTextures->height) + pMarker[1]);
    Sprite_Queue(&src, &dst, g_unk0x00537074, 2, 0, 0, 0,
                 NetPlayers_GetPlayerNameColour(NetPlayers_GetPlayerID(NetPlayers_FindPlayerByField8(param_2))), 8);
}

// One entry of the per-car split-bar geometry table: the divider marker rect
// (or one of the two highlight rects that follow the 0x14 markers of a car).
struct SplitMarker {
    short x;
    short y;
    short w;
    short h;
};

// Per-car split-bar rects: 0x14 marker rects per car, then the two rectangles
// the highlighted split is drawn from.
// GLOBAL: CMR2 0x00536cb8
SplitMarker g_splitBarRects[0x14 * 8];
// Colour of the one-pixel marker between two middle splits (opaque white).
// GLOBAL: CMR2 0x005170ac
unsigned int g_splitMiddleMarkerColour = 0xffffffff;
// Colour the highlighted split is drawn with; the routine rewrites it with the
// split's own colour (alpha forced to 0xff) before drawing.
// GLOBAL: CMR2 0x005170cc
unsigned int g_highlightedSplitColour = 0xffb49696;

// Draws one car's split-time bar: every split is a one-pixel horizontal divider
// in the split's own colour, the split indexed by g_stageSplitData[car].split is
// the two highlight rects instead, and the dividers between the middle splits
// get a one-pixel vertical marker.
// match 55.79%: implementada; MSVC6 elige EBX como contador del bucle donde el original usa EBP
// (un registro callee-saved mas) y no reutiliza la ranura de pila del color para el marcador.
// FUNCTION: CMR2 0x004143f0
void RallyData_DrawCarSplitTimeBar(int car, short *pRect)
{
    int i;
    unsigned int colour;
    SplitMarker marker;

    if ((BYTE)RallyData_GetSelectionFlag28() == 0 ||
        (g_stageTargetTime != g_stageDefaultTargetTime && g_stageTargetTime != 0)) {
        if (RallyData_IsHeadToHeadRaceMode() != 0)
            CGameInfo::IsSplitBarEnabled();
        for (i = 0; i <= g_splitCount; i++) {
            if (i != g_stageSplitData[car].split) {
                Sprite_FillRect(&g_pGraphics->field309_0x150,
                                (short *)&g_splitBarRects[i + car * 0x14],
                                (BYTE *)&g_splitTimeColours[car * 0x28 + i + 1], 2);
            } else {
                g_highlightedSplitColour = (unsigned int)g_splitTimeColours[car * 0x28 + i + 1];
                ((BYTE *)&g_highlightedSplitColour)[3] = 0xff;
                Sprite_FillRect(&g_pGraphics->field309_0x150,
                                (short *)(g_splitTimeColours + car * 0x28 + 13),
                                (BYTE *)&g_highlightedSplitColour, 2);
                colour = (unsigned int)g_splitTimeColours[car * 0x28 + i + 1];
                Sprite_FillRect(&g_pGraphics->field309_0x150,
                                (short *)(g_splitTimeColours + car * 0x28 + 15),
                                (BYTE *)&colour, 2);
            }
            if (i > 0 && i < g_splitCount - 1) {
                marker = g_splitBarRects[i + car * 0x14];
                marker.w = 1;
                Sprite_FillRect(&g_pGraphics->field309_0x150, (short *)&marker,
                                (BYTE *)&g_splitMiddleMarkerColour, 2);
            }
        }
    }
}

// Same split-bar drawing as RallyData_DrawCarSplitTimeBar, plus the leader markers of every car
// (RallyData_DrawRacePositionMarker) once the two stage textures are loaded.
// match 64.75%: implementada; misma diferencia de reparto de registros que 0x4143f0
// (contador del bucle en EBX en vez de EBP) mas el orden de las cargas de la cola.
// FUNCTION: CMR2 0x00414550
void RallyData_DrawSplitBarAndLeaderMarkers(int car, short *pRect)
{
    int i;
    int j;
    unsigned int colour;
    SplitMarker marker;

    if ((BYTE)RallyData_GetSelectionFlag28() == 0 ||
        (g_stageTargetTime != g_stageDefaultTargetTime && g_stageTargetTime != 0)) {
        if (RallyData_IsHeadToHeadRaceMode() != 0)
            CGameInfo::IsSplitBarEnabled();
        for (i = 0; i <= g_splitCount; i++) {
            if (i != g_stageSplitData[car].split) {
                Sprite_FillRect(&g_pGraphics->field309_0x150,
                                (short *)&g_splitBarRects[i + car * 0x14],
                                (BYTE *)&g_splitTimeColours[car * 0x28 + i + 1], 2);
            } else {
                g_highlightedSplitColour = (unsigned int)g_splitTimeColours[car * 0x28 + i + 1];
                ((BYTE *)&g_highlightedSplitColour)[3] = 0xff;
                Sprite_FillRect(&g_pGraphics->field309_0x150,
                                (short *)(g_splitTimeColours + car * 0x28 + 13),
                                (BYTE *)&g_highlightedSplitColour, 2);
                colour = (unsigned int)g_splitTimeColours[car * 0x28 + i + 1];
                Sprite_FillRect(&g_pGraphics->field309_0x150,
                                (short *)(g_splitTimeColours + car * 0x28 + 15),
                                (BYTE *)&colour, 2);
            }
            if (i > 0 && i < g_splitCount - 1) {
                marker = g_splitBarRects[i + car * 0x14];
                marker.w = 1;
                Sprite_FillRect(&g_pGraphics->field309_0x150, (short *)&marker,
                                (BYTE *)&g_splitMiddleMarkerColour, 2);
            }
        }
        if (g_challengeTextures != 0 && g_unk0x00537074 != 0) {
            for (j = 6; j >= 0; j--) {
                if ((BYTE)NetPlayers_IsPlayerPresent(j) != 0)
                    RallyData_DrawRacePositionMarker((short *)&g_splitBarRects[car * 0x14], NetPlayers_GetPlayerField8(j));
            }
            RallyData_DrawRacePositionMarker((short *)&g_splitBarRects[car * 0x14], 0);
        }
    }
}

// Draws the four one-pixel edges of a HUD rectangle. When param_3 is non-zero
// the rectangle is first clipped to the viewport (the 0x82c9ec rect scaled to
// the current resolution) and only the edges that still touch the visible area
// are drawn - an edge is skipped only when both of its corners were clipped.
// match 50.92%: implementada; el original gasta 12 bytes mas de marco (ranura propia para w/h)
// y usa EBP/EBX/EDI de otra forma; la logica y las divisiones coinciden.
// FUNCTION: CMR2 0x00504eb0
void RallyData_DrawClippedHudRectangleEdges(short *param_1, BYTE *param_2, int param_3)
{
    struct ClipPoint {
        int x;
        int y;
    };
    short rect[4];
    ClipPoint corner[4];
    int clipped[4];
    int left;
    int top;
    int right;
    int bottom;
    int i;
    int w;
    int h;

    if (param_3 != 0) {
        left = (int)g_optionPreviewExpandedRect[0] * (int)g_pGraphics->resX / 0x280;
        top = (int)g_optionPreviewExpandedRect[1] * (int)g_pGraphics->resY / 0x1e0;
        right = (int)g_optionPreviewExpandedRect[2] * (int)g_pGraphics->resX / 0x280 + left;
        bottom = (int)g_optionPreviewExpandedRect[3] * (int)g_pGraphics->resY / 0x1e0 + top;

        corner[0].x = param_1[0];
        corner[0].y = param_1[1];
        corner[1].x = param_1[0] + param_1[2];
        corner[1].y = param_1[1];
        corner[2].x = param_1[0] + param_1[2];
        corner[2].y = param_1[1] + param_1[3];
        corner[3].x = param_1[0];
        corner[3].y = param_1[1] + param_1[3];
        for (i = 0; i < 4; i++) {
            clipped[i] = 0;
            if (corner[i].x < left) {
                corner[i].x = left;
                clipped[i] = 1;
            } else if (corner[i].x > right) {
                corner[i].x = right;
                clipped[i] = 1;
            }
            if (corner[i].y < top) {
                corner[i].y = top;
                clipped[i] = 1;
            } else if (corner[i].y > bottom) {
                corner[i].y = bottom;
                clipped[i] = 1;
            }
        }
        w = corner[1].x - corner[0].x;
        h = corner[2].y - corner[1].y;

        rect[0] = (short)corner[0].x;
        rect[1] = (short)corner[0].y;
        rect[2] = (short)w;
        rect[3] = 1;
        if (clipped[0] == 0 || clipped[1] == 0)
            Sprite_FillRect(&g_pGraphics->field309_0x150, rect, param_2, 1);
        rect[1] = (short)corner[3].y;
        if (clipped[2] == 0 || clipped[3] == 0)
            Sprite_FillRect(&g_pGraphics->field309_0x150, rect, param_2, 1);
        rect[0] = (short)corner[0].x;
        rect[1] = (short)corner[0].y;
        rect[2] = 1;
        rect[3] = (short)h;
        if (clipped[0] == 0 || clipped[3] == 0)
            Sprite_FillRect(&g_pGraphics->field309_0x150, rect, param_2, 1);
        rect[0] = (short)(corner[0].x + w - 1);
        if (clipped[1] == 0 || clipped[2] == 0)
            Sprite_FillRect(&g_pGraphics->field309_0x150, rect, param_2, 1);
    } else {
        rect[0] = param_1[0];
        rect[1] = param_1[1];
        rect[2] = param_1[2];
        rect[3] = 1;
        Sprite_FillRect(&g_pGraphics->field309_0x150, rect, param_2, 1);
        rect[1] = (short)(param_1[1] + param_1[3]);
        Sprite_FillRect(&g_pGraphics->field309_0x150, rect, param_2, 1);
        rect[0] = param_1[0];
        rect[1] = param_1[1];
        rect[2] = 1;
        rect[3] = param_1[3];
        Sprite_FillRect(&g_pGraphics->field309_0x150, rect, param_2, 1);
        rect[0] = (short)(param_1[0] + param_1[2] - 1);
        Sprite_FillRect(&g_pGraphics->field309_0x150, rect, param_2, 1);
    }
}

// Same 0x50-byte entry as FixInterp, seen with the fields the split-bar
// texture mapping uses.

// Gradients the split-time texture is mapped through.
// GLOBAL: CMR2 0x005270e4
BYTE g_splitTimeGradients[8] = { 0xff, 0xff, 0xff, 0xff, 0x57, 0x57, 0x57, 0xff };

// Queues the split-time panel: builds the source rect from the current car's
// entry in the 0x82c6c8 table (or the two texture fields when no car is
// selected) and the destination rect from the segment rect at 0x82c9ec scaled
// to the current resolution.
// FUNCTION: CMR2 0x00505590
void RallyData_QueueSelectedCarSplitPanel(void)
{
    short rect[4];
    short src[4];
    if (g_selectedOptionPreviewPanel != 0xff) {
        FixInterp *pEntry = g_optionPreviewPanels + (signed char)g_selectedOptionPreviewPanel;
        int start = pEntry->dstX1 << 16;
        int end = (pEntry->dstX2 + pEntry->dstX1) << 16;
        int first = FixDiv(start - (g_optionPreviewExpandedRect[0] << 16), g_optionPreviewExpandedRect[2] << 16);
        int second = FixDiv(end - (g_optionPreviewExpandedRect[0] << 16), g_optionPreviewExpandedRect[2] << 16);
        int scale = FixDiv(pEntry->u0 - pEntry->u1, first - second);
        rect[0] = (short)((pEntry->u0 - FixMul(first, scale)) >> 16);
        rect[2] = (short)(scale >> 16);
        start = pEntry->dstY1 << 16;
        end = (pEntry->dstY2 + pEntry->dstY1) << 16;
        first = FixDiv(start - (g_optionPreviewExpandedRect[1] << 16), g_optionPreviewExpandedRect[3] << 16);
        second = FixDiv(end - (g_optionPreviewExpandedRect[1] << 16), g_optionPreviewExpandedRect[3] << 16);
        scale = FixDiv(pEntry->v0 - pEntry->v1, first - second);
        rect[1] = (short)((pEntry->v0 - FixMul(first, scale)) >> 16);
        rect[3] = (short)(scale >> 16);
    } else {
        *(int *)&rect[0] = *(int *)&g_mapSrcRect[0];
        *(int *)&rect[2] = *(int *)&g_mapSrcRect[2];
    }
    src[0] = (short)((int)g_optionPreviewExpandedRect[0] * (int)g_pGraphics->resX / 0x280);
    src[1] = (short)((int)g_optionPreviewExpandedRect[1] * (int)g_pGraphics->resY / 0x1e0);
    src[2] = (short)((int)g_optionPreviewExpandedRect[2] * (int)g_pGraphics->resX / 0x280);
    src[3] = (short)((int)g_optionPreviewExpandedRect[3] * (int)g_pGraphics->resY / 0x1e0);
    Sprite_Queue((SpriteRect *)rect, (SpriteRect *)src, (Texture *)g_unk0x0082c9e8, 3, 0, 0, 0,
                 (BYTE *)&g_splitTimeGradients, 8);
}

// Marker colour of the split bar, plus the alpha byte of the neighbouring entry
// of the HUD colour table that is copied into the bar colour.
// GLOBAL: CMR2 0x005170a7
BYTE g_unk0x005170a7 = 0x96;
// GLOBAL: CMR2 0x005170a8
unsigned int g_unk0x005170a8 = 0x28ddbdbc;
// GLOBAL: CMR2 0x005170aa
BYTE g_unk0x005170aa = 0xdd;

// Lays out both players' stage-classification split bars: the bar spans from the
// start position (mode/graphics dependent) up to the end position, and every
// split's segment gets its x (interpolated by that split's reference time), its
// y (centred on the screen), its height and its colour stored in the marker
// table and in g_splitTimeColours; a second pass gives every segment the width up
// to the next split's x.
// match 53%: implementada, MSVC6 usa pila 0x30 vs 0x2c y ordena algunos temporales distinto; la estructura es identica
// FUNCTION: CMR2 0x00411b20
void RallyData_LayoutPlayerSplitBars(void)
{
    int total;
    int barStart;
    int barEnd;
    int span;
    int car;
    int split;
    int xOff;
    int yOff;
    int yPos;
    int uTime;
    int other;
    unsigned int markerColour;
    unsigned int barColour;
    int *pTime;
    int *pRef;
    int *pColour;
    short *pGeo;
    SplitMarker *pSpan;

    total = RallyData_BuildStageRouteDistanceColumn();
    // PORT: the bars are laid out on the HUD's screen (see Hud_BeginLayout).
    Hud_BeginLayout();
    if (CGameInfo::GetConfiguredGameMode() == 8 || CGameInfo::GetConfiguredGameMode() == 9 ||
        CGameInfo::GetConfiguredGameMode() == 0xb) {
        barStart = (int)g_pGraphics->resX * 0xc00 >> 16;
        barEnd = (int)g_pGraphics->resX * 0xf400 >> 16;
    } else if (RallyData_IsHeadToHeadRaceMode() != 0 && CGameInfo::IsSplitBarEnabled() == 0) {
        barStart = ((int)g_pGraphics->resX * 0xc934 >> 16) - 10;
        barEnd = (int)g_pGraphics->resX * 0xf400 >> 16;
    } else {
        barStart = (int)g_pGraphics->resX * 0xab9b >> 16;
        barEnd = (int)g_pGraphics->resX * 0xf400 >> 16;
    }
    span = (barEnd - barStart) << 16;
    if (total == 0)
        total = 1;
    if (g_splitCount != 0) {
        car = 0;
        do {
            split = 0;
            pRef = &g_splitTargetTimes[1];
            pColour = &g_splitTimeColours[car * 0x28 + 1];
            pTime = &g_classificationTable[1];
            // One marker is {short x, y, w, h}; the original walks it through a
            // short* anchored on y, so x sits at -1 and the height h at +2.
            pGeo = (short *)&g_splitBarRects[car * 0x14] + 1;
            do {
                xOff = 0;
                yOff = 0;
                uTime = *pTime;
                if (RallyData_IsHeadToHeadRaceMode() != 0) {
                    if (CGameInfo::IsSplitBarEnabled() == 0) {
                        if (car == 0)
                            xOff = -((int)g_pGraphics->resX / 2);
                    } else if (car == 1) {
                        yOff = (int)g_pGraphics->resY / 2;
                    }
                }
                uTime = FixDiv(uTime, total);
                pGeo[-1] = (short)((FixMul(uTime, span) >> 16) + barStart + xOff);
                if (CGameInfo::GetConfiguredGameMode() == 8 || CGameInfo::GetConfiguredGameMode() == 9 ||
                    CGameInfo::GetConfiguredGameMode() == 0xb)
                    yPos = (int)g_pGraphics->resY * 0x554 >> 16;
                else
                    yPos = (int)g_pGraphics->resY << 0xc >> 16;
                pGeo[0] = (short)(yPos + yOff);
                markerColour = g_unk0x005170a8;
                pGeo[2] = (short)((int)g_pGraphics->resY * 0xaac >> 16) + 1;
                *pColour = markerColour;
                if (split == g_splitCount)
                    pGeo[-1] = (short)(barEnd + xOff);
                ((BYTE *)pColour)[0] = (BYTE)markerColour;
                ((BYTE *)pColour)[1] = (BYTE)(markerColour >> 8);
                ((BYTE *)pColour)[2] = g_unk0x005170aa;
                ((BYTE *)pColour)[3] = g_unk0x005170a7;
                if (CGameInfo::GetConfiguredGameMode() == 4) {
                    if (g_stageSplitData[car].times[split + 1] != 0) {
                        other = (car + 1) % 2;
                        barColour = g_unk0x0051709c;
                        if (g_stageSplitData[other].times[split + 1] == 0 ||
                            g_stageSplitData[car].times[split + 1] <
                                g_stageSplitData[other].times[split + 1])
                            goto storeSplitColour;
                        pColour[-1] = g_unk0x005170a0;
                    }
                } else if (g_stageSplitData[car].times[split + 1] != 0) {
                    barColour = g_unk0x005170a0;
                    if (g_stageSplitData[car].times[split + 1] -
                            g_stageSplitData[car].times[split] <
                        pRef[0] - pRef[-1])
                        pColour[-1] = g_unk0x0051709c;
                    else {
storeSplitColour:
                        pColour[-1] = barColour;
                    }
                }
                pTime += 2;
                pGeo += 4;
                split++;
                pRef++;
                pColour++;
            } while ((INT_PTR)pTime < (INT_PTR)(g_classificationTable + 25));
            // Every split's segment runs up to the next split's x.
            pSpan = (SplitMarker *)&g_splitBarRects[car * 0x14];
            split = 0xc;
            do {
                split--;
                pSpan->w = pSpan[1].x - pSpan->x;
                pSpan++;
            } while (split != 0);
            car++;
        } while (car <= 1);
    }
    Hud_EndLayout();
}


// Two packed colours (blue-ish and red) used as the base of the split panel.
// GLOBAL: CMR2 0x005270ec
int g_unk0x005270ec = 0xff005cf3;
// GLOBAL: CMR2 0x005270f0
int g_unk0x005270f0 = 0xff0000ff;

// 0x50-byte entry of 0x82c6c8 as seen by the per-car split draw: source and
// destination rects plus the shared easing value at 0x1c.

// Draws the split-time panels of every car of the current rally. The active
// entry (0x82ca1c) gives the destination rect of each panel and the two scale
// factors used to map the source rect of the others into it; a white panel
// fading with 0x82cb44 is drawn over the active one.
// match 58%: implementada; el original coloca los locales en otro orden (color en
// -0xc..-0x1, rect en -0x28..-0x22) y reserva 0x30 de marco con un temporal para
// 0x10000-0x82cb44; la logica, las divisiones 32.16 y las constantes coinciden.
// FUNCTION: CMR2 0x005051c0
void RallyData_DrawCarSplitTimePanels(void)
{
    FixInterp *p;
    FixInterp *pEntry;
    BYTE colour[12];
    int scaleX;
    int scaleY;
    int i;
    int alpha;
    int grow;
    short rect[4];

    if (g_selectedOptionPreviewPanel == 0xff)
        return;
    p = g_optionPreviewPanels + (signed char)g_selectedOptionPreviewPanel;
    alpha = 0xff0000 - FixMul(p->current, 0xff0000);
    *(int *)&colour[0] = g_unk0x005270ec;
    *(int *)&colour[8] = g_unk0x005270f0;
    *(int *)&colour[4] = g_unk0x005270f0;
    colour[11] = (BYTE)(alpha >> 16);
    colour[3] = (BYTE)(alpha >> 16);
    colour[7] = (BYTE)(FixMul(alpha, 0x4000) >> 16);
    if (alpha == 0)
        return;
    scaleX = FixDiv((int)p->dstX2 << 16, (int)p->srcX2 << 16);
    scaleY = FixDiv((int)p->dstY2 << 16, (int)p->srcY2 << 16);
    for (i = 0; i < g_optionPreviewPanelCount; i++) {
        pEntry = g_optionPreviewPanels + i;
        if (i == (signed char)g_selectedOptionPreviewPanel)
            continue;
        rect[0] = (short)(p->dstX1 + (FixMul((pEntry->srcX1 - p->srcX1) << 16, scaleX) >> 16));
        rect[1] = (short)(p->dstY1 + (FixMul(scaleY, (pEntry->srcY1 - p->srcY1) << 16) >> 16));
        rect[2] = (short)(FixMul(pEntry->srcX2 << 16, scaleX) >> 16);
        rect[3] = (short)(FixMul(scaleY, pEntry->srcY2 << 16) >> 16);
        rect[0] = (short)((int)rect[0] * (int)g_pGraphics->resX / 0x280);
        rect[1] = (short)((int)rect[1] * (int)g_pGraphics->resY / 0x1e0);
        rect[2] = (short)((int)rect[2] * (int)g_pGraphics->resX / 0x280);
        rect[3] = (short)((int)rect[3] * (int)g_pGraphics->resY / 0x1e0);
        RallyData_DrawClippedHudRectangleEdges(rect, colour, 1);
    }
    rect[0] = (short)((int)p->dstX1 * (int)g_pGraphics->resX / 0x280);
    rect[1] = (short)((int)p->dstY1 * (int)g_pGraphics->resY / 0x1e0);
    rect[2] = (short)((int)p->dstX2 * (int)g_pGraphics->resX / 0x280);
    rect[3] = (short)((int)p->dstY2 * (int)g_pGraphics->resY / 0x1e0);
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, colour, 1);
    RallyData_DrawClippedHudRectangleEdges(rect, &colour[8], 0);
    *(int *)&rect[0] = *(int *)&p->dstX1;
    *(int *)&rect[2] = *(int *)&p->dstX2;
    grow = (FixMul(g_unk0x0082cb44, 0x80000) >> 16);
    rect[0] = (short)(rect[0] - grow);
    rect[1] = (short)(rect[1] - grow);
    rect[2] = (short)(rect[2] + grow * 2);
    rect[3] = (short)(rect[3] + grow * 2);
    rect[0] = (short)((int)rect[0] * (int)g_pGraphics->resX / 0x280);
    rect[1] = (short)((int)rect[1] * (int)g_pGraphics->resY / 0x1e0);
    rect[2] = (short)((int)rect[2] * (int)g_pGraphics->resX / 0x280);
    rect[3] = (short)((int)rect[3] * (int)g_pGraphics->resY / 0x1e0);
    colour[11] = (BYTE)(FixMul(alpha, 0x10000 - g_unk0x0082cb44) >> 16);
    RallyData_DrawClippedHudRectangleEdges(rect, &colour[8], 0);
}


extern BYTE g_optionSlotTextureSets[0x14];
extern BYTE g_unk0x0082ca18;

// Two packed HUD colours (grey with alpha) used by the split panel.
// GLOBAL: CMR2 0x005270fc
int g_unk0x005270fc = 0xff4f4f4f;
// GLOBAL: CMR2 0x00527100
int g_unk0x00527100 = 0x804f4f4f;
// Per-rally accent colours indexed by RallyDataCountryIndex().
// GLOBAL: CMR2 0x00527104
BYTE g_rallyAccentColours[0x138] = {
    0xa4, 0xd3, 0xae, 0xff, 0xb7, 0xa8, 0x76, 0xff, 0xbd, 0xdb, 0xad, 0xff, 0xa7, 0xd1, 0xc1, 0xff,
    0xbe, 0x8d, 0x65, 0xff, 0xe6, 0xbf, 0x8b, 0xff, 0xc2, 0xc8, 0x8f, 0xff, 0x95, 0xbd, 0x81, 0xff,
    0xb7, 0xa8, 0x76, 0xff, 0x9c, 0x68, 0x51, 0x00, 0x80, 0x72, 0x52, 0x00, 0x94, 0x68, 0x51, 0x00,
    0x8c, 0x68, 0x51, 0x00, 0x84, 0x68, 0x51, 0x00, 0x7c, 0x68, 0x51, 0x00, 0x78, 0x72, 0x52, 0x00,
    0x74, 0x68, 0x51, 0x00, 0xa4, 0x68, 0x51, 0x00, 0x70, 0x93, 0x51, 0x00, 0x68, 0x93, 0x51, 0x00,
    0x60, 0x93, 0x51, 0x00, 0x58, 0x93, 0x51, 0x00, 0x6c, 0x72, 0x52, 0x00, 0x50, 0x93, 0x51, 0x00,
    0x48, 0x93, 0x51, 0x00, 0x64, 0x92, 0x51, 0x00, 0x80, 0x92, 0x51, 0x00, 0x7c, 0x92, 0x51, 0x00,
    0x78, 0x92, 0x51, 0x00, 0x74, 0x92, 0x51, 0x00, 0x70, 0x92, 0x51, 0x00, 0x6c, 0x92, 0x51, 0x00,
    0x68, 0x92, 0x51, 0x00, 0x64, 0x92, 0x51, 0x00, 0x94, 0x99, 0x91, 0x8b, 0xa0, 0x8e, 0x94, 0x99,
    0x7d, 0x4c, 0x75, 0x50, 0x6f, 0x44, 0x7d, 0x4c, 0x82, 0x9c, 0x79, 0xa5, 0x00, 0x00, 0x5c, 0x68,
    0x5d, 0x62, 0x57, 0x68, 0x5b, 0x63, 0x54, 0x57, 0x4f, 0x5f, 0x57, 0x5c, 0x58, 0x56, 0x6e, 0x47,
    0x73, 0x4b, 0x70, 0x49, 0xcf, 0x86, 0xd8, 0x83, 0xd4, 0x89, 0xdb, 0x87, 0xcd, 0x98, 0xd4, 0x91,
    0xcf, 0x92, 0xd9, 0x8f, 0xd1, 0x9b, 0xce, 0x90, 0x00, 0x00, 0x6f, 0x59, 0x75, 0x55, 0x7e, 0x51,
    0x7b, 0x57, 0x76, 0x88, 0x72, 0x82, 0x7b, 0x82, 0x76, 0x7e, 0x74, 0xac, 0x6e, 0xa8, 0x72, 0xaa,
    0x49, 0x60, 0x4d, 0x59, 0x4f, 0x60, 0x49, 0x64, 0x5c, 0x37, 0x61, 0x38, 0x5d, 0x32, 0x5b, 0x3b,
    0x25, 0x59, 0x2b, 0x61, 0x00, 0x00, 0x54, 0x7a, 0x57, 0x74, 0x56, 0x7a, 0x57, 0x82, 0x71, 0x6a,
    0x68, 0x68, 0x68, 0x70, 0x6c, 0x6e, 0x4f, 0x65, 0x54, 0x60, 0x53, 0x63, 0x5b, 0x42, 0x62, 0x3a,
    0x62, 0x48, 0x64, 0x3f, 0x46, 0x35, 0x4a, 0x39, 0x4b, 0x33, 0x3f, 0x37, 0x87, 0x6b, 0x7f, 0x67,
    0x00, 0x00, 0x7b, 0x6e, 0x7f, 0x6c, 0x7b, 0x6a, 0x7f, 0x66, 0x7b, 0x88, 0x7e, 0x85, 0x7c, 0x7f,
    0x7e, 0x7b, 0x6c, 0x91, 0x6e, 0x95, 0x6d, 0x93,
};

// Marker widths per record slot (narrow / wide layouts).
// GLOBAL: CMR2 0x0052723c
BYTE g_splitMarkerWidths[12] = { 0x14, 0x16, 0x16, 0x11, 0x11, 0x0d, 0x0e, 0x0b, 0x0b, 0x00, 0x00, 0x00 };
// GLOBAL: CMR2 0x00527248
BYTE g_unk0x00527248[12] = { 0x2d, 0x30, 0x31, 0x28, 0x28, 0x21, 0x22, 0x1d, 0x1e, 0x00, 0x00, 0x00 };


void OptionMenu_DrawTransitionTextShortCoords(int index, int font1, int font2, char *text, short x, short y,
                  int *pColour1, int *pColour2, unsigned int flags);
void OptionMenu_TransformHudPointToScreen(FixInterp *p, short *pX, short *pY);

// Draws the split-times panel of car slot param1: the animated panel sprite,
// then one row per opponent (the car marker plus its faded preview rect), then
// the elapsed/current time markers and, when the panel is fading in, its
// outline.
// FUNCTION: CMR2 0x005044d0
void RallyData_DrawCarSplitTimesPanel(int param1)
{
    FixInterp *pEntry;
    short src[4];
    short outline[4];
    short rect[4];
    BYTE colour[4];
    BYTE textColour[4];
    BYTE shadowColour[4];
    BYTE accent[4];
    BYTE outlineColour[4];
    int scale;
    int over;
    int t;
    int x;
    int y;
    int i;

    if (g_unk0x0082c690 == NULL || param1 == -1)
        return;
    pEntry = g_optionPreviewPanels + param1;
    rect[0] = (short)((int)pEntry->dstX1 * (int)g_pGraphics->resX / 0x280);
    rect[1] = (short)((int)pEntry->dstY1 * (int)g_pGraphics->resY / 0x1e0);
    rect[2] = (short)((int)pEntry->dstX2 * (int)g_pGraphics->resX / 0x280);
    rect[3] = (short)((int)pEntry->dstY2 * (int)g_pGraphics->resY / 0x1e0);
    *(int *)colour = *(int *)g_splitTimeGradients;
    *(int *)textColour = g_unk0x005270fc;
    *(int *)shadowColour = g_unk0x00527100;
    colour[3] = (BYTE)(FixMul(pEntry->current, 0xff0000) >> 16);
    if (pEntry->current == 0)
        return;
    *(int *)&src[0] = *(int *)&pEntry->texture->field_0x11c;
    *(int *)&src[2] = *(int *)&pEntry->texture->width;
    src[3] = g_mapSrcRect[3];
    if (CGameInfo::GetScreenWidth() >= 0x400 && CFrontend::IsTextureWidthSupported(0x400) &&
        CFrontend::IsTextureHeightSupported(0x400)) {
        src[2] = (short)((int)g_optionPreviewExpandedRect[2] * (int)g_pGraphics->resX / 0x280);
        src[3] = (short)((int)g_optionPreviewExpandedRect[3] * (int)g_pGraphics->resY / 0x1e0);
    }
    Sprite_Queue((SpriteRect *)src, (SpriteRect *)rect, pEntry->texture, 3, 0, NULL, NULL, colour, 8);
    x = 0x3c;
    rect[3] = 0x28;
    rect[0] = 0x29;
    rect[2] = 0x62;
    rect[1] = (short)(g_unk0x0082c9fe + g_unk0x0082ca02 - 0x28);
    y = rect[1] + 0x14;
    if (pEntry->current == 0x10000)
        OptionMenu_DrawTransitionTextShortCoords(3, 1, 1, CFrontend::GetTextString(0x136), (int)g_pGraphics->resX * 0x29 / 0x280,
                     (int)g_pGraphics->resY * 0x89 / 0x1e0, (int *)textColour, (int *)shadowColour, 0x11);
    rect[0] = (short)(rect[0] + (short)param1 * 0x3c);
    rect[0] = (short)((int)rect[0] * (int)g_pGraphics->resX / 0x280);
    rect[2] = (short)((int)rect[2] * (int)g_pGraphics->resX / 0x280);
    rect[1] = (short)((int)rect[1] * (int)g_pGraphics->resY / 0x1e0);
    rect[3] = (short)((int)rect[3] * (int)g_pGraphics->resY / 0x1e0);
    scale = FixMul(pEntry->phase2, 0x20000);
    if (scale > 0x10000)
        scale = 0x10000;
    rect[2] = (short)(FixMul(rect[2] << 16, scale) >> 16);
    rect[3] = (short)(FixMul(rect[3] << 16, scale) >> 16);
    *(int *)accent = *(int *)&g_rallyAccentColours[(RallyDataCountryIndex() & 0xff) * 4];
    accent[0] = (BYTE)((accent[0] >> 2) * 3);
    accent[1] = (BYTE)((accent[1] >> 2) * 3);
    accent[2] = (BYTE)((accent[2] >> 2) * 3);
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, accent, 1);
    scale = FixDiv(pEntry->dstX2 << 16, g_optionPreviewExpandedRect[2] << 16);
    if (pEntry->phase1 != 0) {
        t = FixMul(pEntry->current, pEntry->phase1);
        colour[3] = textColour[3] = (BYTE)(FixMul(t, 0xff0000) >> 16);
        for (i = 0; i < (g_unk0x0082ca18 & 0xff); i++) {
            rect[0] = (short)x;
            rect[1] = (short)y;
            OptionMenu_TransformHudPointToScreen(pEntry, &rect[0], &rect[1]);
            if (g_optionSlotTextureSets[i] == 100) {
                if (CGameInfo::GetScreenWidth() >= 0x400 && CFrontend::IsTextureWidthSupported(0x400) &&
                    CFrontend::IsTextureHeightSupported(0x400))
                    rect[1] += 6;
                else
                    rect[1] += 4;
                OptionMenu_DrawTransitionTextShortCoords(3, 1, 1, CFrontend::GetTextString(0xfe), rect[0], rect[1],
                             (int *)textColour, (int *)shadowColour, 0x12);
            } else {
                rect[2] = (short)(FixMul(scale, g_unk0x0082ca20[g_optionSlotTextureSets[i]]->width << 16) >> 16);
                rect[3] = (short)(FixMul(scale, g_unk0x0082ca20[g_optionSlotTextureSets[i]]->height << 16) >> 16);
                rect[0] -= (short)(FixMul(scale, rect[2] / 2 << 16) >> 16);
                if (CGameInfo::GetScreenWidth() >= 0x400 && CFrontend::IsTextureWidthSupported(0x400) &&
                    CFrontend::IsTextureHeightSupported(0x400))
                    t = FixMul(scale, g_unk0x00527248[g_optionSlotTextureSets[i]] << 16);
                else
                    t = FixMul(scale, g_splitMarkerWidths[g_optionSlotTextureSets[i]] << 16);
                rect[1] -= (short)(t >> 16);
                Sprite_Queue((SpriteRect *)&g_unk0x0082ca20[g_optionSlotTextureSets[i]]->field_0x11c, (SpriteRect *)rect,
                             g_unk0x0082ca20[g_optionSlotTextureSets[i]], 1, 0, NULL, NULL, colour, 8);
            }
            x += 0x1e;
            // the original reads the count as a masked dword here (byte everywhere else)
            if (i != (*(int *)&g_unk0x0082ca18 & 0xff) - 1) {
                rect[0] = (short)x;
                rect[1] = (short)y;
                OptionMenu_TransformHudPointToScreen(pEntry, &rect[0], &rect[1]);
                rect[2] = (short)(FixMul(scale, ((Texture *)g_unk0x0082c690)->width << 16) >> 16);
                rect[3] = (short)(FixMul(scale, ((Texture *)g_unk0x0082c690)->height << 16) >> 16);
                rect[0] += -(rect[2] / 2);
                rect[1] += -(rect[3] / 2);
                Sprite_Queue((SpriteRect *)&((Texture *)g_unk0x0082c690)->field_0x11c, (SpriteRect *)rect,
                             (Texture *)g_unk0x0082c690, 1, 0, NULL, NULL, colour, 8);
                x += 0x1e;
            }
        }
    }
    if (pEntry->phase2 != 0) {
        scale = FixMul(pEntry->phase2, 0x20000);
        over = scale - 0x10000;
        if (scale > 0x10000)
            scale = 0x10000;
        rect[0] = (short)((int)g_unk0x0082c9fc * (int)g_pGraphics->resX / 0x280);
        rect[1] = (short)((int)g_unk0x0082c9fe * (int)g_pGraphics->resY / 0x1e0);
        rect[2] = (short)((FixMul(g_unk0x0082ca00 << 16, scale) >> 16) * (int)g_pGraphics->resX / 0x280);
        rect[3] = (short)((FixMul(g_unk0x0082ca02 << 16, scale) >> 16) * (int)g_pGraphics->resY / 0x1e0);
        if (over >= 0) {
            colour[3] = (BYTE)(FixMul(FixMul(pEntry->current, over), 0xff0000) >> 16);
            Sprite_Queue((SpriteRect *)g_mapSrcRect, (SpriteRect *)rect, (Texture *)g_unk0x0082c9e8, 3, 0,
                         NULL, NULL, colour, 8);
            x = FixDiv((pEntry->srcX1 - g_optionPreviewExpandedRect[0]) << 16, g_optionPreviewExpandedRect[2] << 16);
            outline[0] = (short)((FixMul(x, g_unk0x0082ca00 << 16) >> 16) + g_unk0x0082c9fc);
            x = FixDiv((pEntry->srcY1 - g_optionPreviewExpandedRect[1]) << 16, g_optionPreviewExpandedRect[3] << 16);
            outline[1] = (short)((FixMul(x, g_unk0x0082ca02 << 16) >> 16) + g_unk0x0082c9fe);
            outline[2] = (short)(FixMul(pEntry->srcX2 << 16, FixDiv(g_unk0x0082ca00 << 16, g_optionPreviewExpandedRect[2] << 16)) >> 16);
            outline[3] = (short)(FixMul(pEntry->srcY2 << 16, FixDiv(g_unk0x0082ca02 << 16, g_optionPreviewExpandedRect[3] << 16)) >> 16);
            outline[2] = (short)((int)outline[2] * (int)g_pGraphics->resX * 2 / 0x280);
            outline[3] = (short)((int)outline[3] * (int)g_pGraphics->resY * 2 / 0x1e0);
            outline[0] = (short)((int)outline[0] * (int)g_pGraphics->resX / 0x280);
            outline[1] = (short)((int)outline[1] * (int)g_pGraphics->resY / 0x1e0);
            *(int *)outlineColour = g_unk0x005270ec;
            outlineColour[3] = colour[3];
            Sprite_FillRect(&g_pGraphics->field309_0x150, outline, outlineColour, 1);
        }
        RallyData_DrawClippedHudRectangleEdges(rect, &g_splitTimeGradients[4], 0);
    }
}


// Resets the split display of both cars for a new stage: clears the reference
// split times, the segment colours and markers of the whole 0x536xxx block and
// the two 0x48-byte records in front of g_stageSplitData, then rebuilds the
// split bar through 0x411b20.
// match 46%: implementada; MSVC6 programa las ocho asignaciones justo tras el primer
// rep stosd y parte el borrado de la ficha en 1 store + rep stosd de 12, mientras que
// nosotros las agrupa al final y hace un solo rep stosd de 13; los bytes y el orden
// de las llamadas coinciden.
// FUNCTION: CMR2 0x004111a0
void RallyData_ResetPlayerSplitDisplays(void)
{
    int i;
    int car;
    int *p;

    RallyData_BuildStageRouteDistanceColumn();
    for (i = 1; i < 0xd; i++)
        g_splitTargetTimes[i] = 0;
    g_splitGapBlank[0] = 0;
    g_splitGapRecorded[0] = 0;
    g_splitGapBlank[1] = 0;
    g_splitGapRecorded[1] = 0;
    g_splitGapAwaitingLeader[0] = 0;
    g_splitGapTime[0] = 0;
    g_splitGapAwaitingLeader[1] = 0;
    g_splitGapTime[1] = 0;
    g_splitTargetsLoaded = 0;
    g_stageFinishBeepPlayed = 0;
    g_stageTargetTime = 0;
    g_profileBestStageTime = 0;
    *(short *)&g_splitGapDisplayFrames[0] = 0;
    for (i = 0; i < 6; i++)
        ((int *)g_splitNearbyRankRows)[i] = -1;
    // The original walks a record base 4 bytes in front of g_stageSplitData, so
    // the dword right before the array gets cleared as well.
    for (car = 0; car < 2; car++) {
        p = (int *)((char *)g_stageSplitData + car * 0x48 - 4);
        p[3] = -1;
        p[2] = 0;
        for (i = 6; i < 0x12; i++)
            p[i] = 0;
        p[5] = 0;
        p[0] = 0;
        p[1] = 0;
    }
    for (i = 0; i < 8; i++) {
        g_playerSplitPositions[i][0] = 0;
        g_playerSplitPositions[i][1] = 0;
    }
    g_unk0x00536c30[0] = 0;
    g_unk0x00536c30[1] = 0;
    g_unk0x00536c30[2] = 0;
    RallyData_LayoutPlayerSplitBars();
}

// ---------------------------------------------------------------------------
#include "Sector.h"
#include "Mesh.h"

extern int g_unk0x0058c950;
extern int g_unk0x0058ca68;
extern StageObject *g_stageObjects[6000];
StageObject *StageObject_GetNext(StageObject *pObject);
void Collision_BuildObjectPointBounds(int *param_1, FixVector *param_2, SectorObjectBounds *param_3);
void StageObject_LoadAndClassifyMeshes(void);
void StageObject_ResetMovingObjectCountsAndPhases(void);

// Total number of 8-byte scratch records of the "B" kind of stage objects.
// GLOBAL: CMR2 0x0058c95c
int g_unk0x0058c95c;

// Rebuilds the per-sector static object tables after a stage is loaded.
// Counts the stage objects of the active range (and how many carry a mesh),
// allocates the per-object scratch area and the two sorted list tables, sizes
// them, fills them from every sector's object list and finally, for every
// object record that has a mesh, builds the world matrix of the object, its
// eight corner points, the length of the box diagonal (through the square-root
// table) and its right/up/forward axes.
// match 60%: logica y constantes exactas (incluido FixSqrt por tabla y el
// barrido del coste por objeto); difiere el reparto de registros, el plegado de
// los bucles de coste y el orden de los memset/alloc en la cola.
// FUNCTION: CMR2 0x00471dd0
void RallyData_RebuildSectorStaticObjectTables(void)
{
    FixMatrix matrix;
    FixVector corners[8];
    FixVector basis[3];
    FixVector v;
    short listIndexA;
    short listIndexB;
    int sectorOffset;
    int countA;
    int countB;
    int totalObjects;
    int countRecords;
    int a;
    int b;
    int sector;
    unsigned int i;
    int n;
    int j;
    int k;
    short sumCost;
    int scratchOffset;
    int recordOffset;
    int offsetA;
    int offsetB;
    int hx;
    int hy;
    int hz;
    int length;
    SectorObjectEntry *pEntry;
    SectorObjectBounds *pScratch;
    ObjectPointBounds *pRecordScratch;
    StageObject *pObject;
    StageObject *pNext;
    Mesh *pMesh;
    MeshCollisionRecord *pRecord;
    StageObject **ppObjects;

    sectorOffset = g_unk0x0058c950 - g_unk0x0058ca68;
    totalObjects = 0;
    g_sectorObjectBounds = NULL;
    g_sectorListEntriesA = NULL;
    g_staticSectorEntries = NULL;
    g_sectorEntryRaised = NULL;
    g_objectPointBounds = NULL;
    g_sectorListIndexA = NULL;
    g_sectorListCountA = NULL;
    g_sectorListEntriesB = NULL;
    g_movingSectorEntries = NULL;
    g_sectorEntryCarFlags = NULL;
    g_sectorListIndexB = NULL;
    g_sectorListCountB = NULL;
    countA = 0;
    if (sectorOffset != 0) {
        ppObjects = &g_stageObjects[g_unk0x0058ca68];
        n = sectorOffset;
        do {
            pObject = *ppObjects;
            ppObjects++;
            totalObjects += pObject->pMesh->sizeUnits;
            n--;
        } while (n != 0);
    }
    if (g_sectorCount != 0) {
        g_sectorListIndexA = (short *)CFileBuffer::AllocateLockedBuffer(g_sectorCount * 2);
        g_sectorListIndexB = (short *)CFileBuffer::AllocateLockedBuffer(g_sectorCount * 2);
    }
    for (i = 0; i < g_sectorCount; i++) {
        g_sectorListIndexA[i] = -1;
        g_sectorListIndexB[i] = -1;
    }
    countB = 0;
    g_unk0x0058c95c = 0;
    countA = 0;
    g_unk0x0058ca6c = 0;
    countB = 0;
    countRecords = 0;
    if (g_sectorCount != 0) {
        sector = 0;
        do {
            a = countA;
            b = countB;

            for (pObject = g_sectors[sector]->pObjects; pObject != NULL;
                 pObject = StageObject_GetNext(pObject)) {
                if (pObject->pMesh != NULL &&
                    (pObject->pMesh->sizeUnits) != 0) {
                    if ((pObject->flags & 0x2001000) == 0) {
                        g_unk0x0058c95c += pObject->pMesh->sizeUnits;
                        if (g_sectorListIndexA[sector] == -1) {
                            g_sectorListIndexA[sector] = (short)a;
                            a++;
                        }
                    } else {
                        g_unk0x0058ca6c += pObject->pMesh->sizeUnits;
                        if (g_sectorListIndexB[sector] == -1) {
                            g_sectorListIndexB[sector] = (short)b;
                            b++;
                        }
                    }
                    for (pRecord = pObject->pMesh->pCollisionRecords; pRecord != NULL;
                         pRecord = pRecord->pNext) {
                        if (pRecord->type == 0)
                            countRecords++;
                    }
                }
            }
            countA = a;
            countB = b;
            sector++;
        } while (sector < g_sectorCount);
    }
    if (totalObjects != 0) {
        g_sectorObjectBounds = (SectorObjectBounds *)CFileBuffer::AllocateLockedBuffer(totalObjects * sizeof(*g_sectorObjectBounds));
        memset(g_sectorObjectBounds, 0, totalObjects * sizeof(*g_sectorObjectBounds));
    }
    if (countA != 0) {
        g_sectorListEntriesA = (SectorObjectEntry **)CFileBuffer::AllocateLockedBuffer(countA * sizeof(*g_sectorListEntriesA));
        g_sectorListCountA = (unsigned short *)CFileBuffer::AllocateLockedBuffer(countA * 2);
        memset(g_sectorListEntriesA, 0, countA * sizeof(*g_sectorListEntriesA));
        memset(g_sectorListCountA, 0, countA * 2);
    }
    if (countB != 0) {
        g_sectorListEntriesB = (SectorObjectEntry **)CFileBuffer::AllocateLockedBuffer(countB * sizeof(*g_sectorListEntriesB));
        g_sectorListCountB = (unsigned short *)CFileBuffer::AllocateLockedBuffer(countB * 2);
    }
    if (g_unk0x0058c95c != 0) {
        g_staticSectorEntries = (SectorObjectEntry *)CFileBuffer::AllocateLockedBuffer(g_unk0x0058c95c * sizeof(*g_staticSectorEntries));
    }
    if (g_unk0x0058ca6c != 0) {
        g_movingSectorEntries = (SectorObjectEntry *)CFileBuffer::AllocateLockedBuffer(g_unk0x0058ca6c * sizeof(*g_movingSectorEntries));
        g_sectorEntryCarFlags = (BYTE *)CFileBuffer::AllocateLockedBuffer(g_unk0x0058ca6c);
        g_sectorEntryRaised = (int *)CFileBuffer::AllocateLockedBuffer(g_unk0x0058ca6c * sizeof(*g_sectorEntryRaised));
    }
    if (countRecords != 0) {
        g_objectPointBounds = (ObjectPointBounds *)CFileBuffer::AllocateLockedBuffer(countRecords * sizeof(*g_objectPointBounds));
        memset(g_objectPointBounds, 0, countRecords * sizeof(*g_objectPointBounds));
    }
    for (i = 0; i < countA; i++) {
        g_sectorListEntriesA[i] = 0;
        g_sectorListCountA[i] = 0;
    }
    for (i = 0; i < countB; i++) {
        g_sectorListEntriesB[i] = 0;
        g_sectorListCountB[i] = 0;
    }
    if (g_sectorCount != 0) {
        sector = 0;
        do {
            for (pObject = g_sectors[sector]->pObjects; pObject != NULL;
                 pObject = StageObject_GetNext(pObject)) {
                if (pObject->pMesh != NULL &&
                    (pObject->pMesh->sizeUnits) != 0) {
                    if ((pObject->flags & 0x2001000) == 0) {
                        listIndexA = g_sectorListIndexA[sector];
                        if (listIndexA != -1)
                            g_sectorListCountA[listIndexA] +=
                                pObject->pMesh->sizeUnits;
                    } else {
                        listIndexB = g_sectorListIndexB[sector];
                        if (listIndexB != -1)
                            g_sectorListCountB[listIndexB] +=
                                pObject->pMesh->sizeUnits;
                    }
                }
            }
            sector++;
        } while (sector < g_sectorCount);
    }
    pScratch = (SectorObjectBounds *)g_movingSectorEntries;
    for (i = 0; i < countB; i++) {
        g_sectorListEntriesB[i] = (SectorObjectEntry *)pScratch;
        pScratch = (SectorObjectBounds *)((SectorObjectEntry *)pScratch + g_sectorListCountB[i]);
    }
    pScratch = (SectorObjectBounds *)g_staticSectorEntries;
    for (i = 0; i < countA; i++) {
        g_sectorListEntriesA[i] = (SectorObjectEntry *)pScratch;
        pScratch = (SectorObjectBounds *)((SectorObjectEntry *)pScratch + g_sectorListCountA[i]);
    }
    for (i = 0; i < (int)g_unk0x0058ca6c; i++) {
        g_sectorEntryCarFlags[i] = 0xff;
        g_sectorEntryRaised[i] = 1;
    }
    CGame::RegisterCallback((void *)RallyData_FreeSectorElementTables, NULL);
    countRecords = 0;
    sector = 0;
    if (g_sectorCount != 0) {
        do {
            offsetA = 0;
            offsetB = 0;
            for (pObject = g_sectors[sector]->pObjects; pObject != NULL;
                 pObject = StageObject_GetNext(pObject)) {
                if (pObject->pMesh == NULL)
                    continue;
                if ((pObject->flags & 0x8000) == 0 &&
                    (char)RallyDataState() == 2) {
                    pObject->visible = 0;
                }
                pMesh = pObject->pMesh;
                if ((char)pMesh->sizeUnits == 0)
                    continue;
                sumCost = 0;
                for (j = 0; j < sectorOffset; j++) {
                    if (pObject == g_stageObjects[g_unk0x0058ca68 + j])
                        break;
                    sumCost += g_stageObjects[g_unk0x0058ca68 + j]->pMesh->sizeUnits;
                }
                pRecord = pMesh->pCollisionRecords;
                if (pRecord == NULL)
                    continue;
                scratchOffset = sumCost * sizeof(SectorObjectBounds);
                recordOffset = countRecords * sizeof(ObjectPointBounds);
                do {
                    if ((pObject->flags & 0x2001000) == 0) {
                        listIndexA = g_sectorListIndexA[sector];
                        if (listIndexA != -1) {
                            pEntry = g_sectorListEntriesA[listIndexA] + offsetA;
                            offsetA++;
                        }
                    } else {
                        listIndexB = g_sectorListIndexB[sector];
                        if (listIndexB != -1) {
                            pEntry = g_sectorListEntriesB[listIndexB] + offsetB;
                            offsetB++;
                        }
                    }
                    pEntry->pObject = pObject;
                    pScratch = (SectorObjectBounds *)((BYTE *)g_sectorObjectBounds + scratchOffset);
                    scratchOffset += sizeof(SectorObjectBounds);
                    pEntry->pBounds = pScratch;
                    hx = pRecord->width;
                    pRecordScratch = (ObjectPointBounds *)((BYTE *)g_objectPointBounds + recordOffset);
                    hy = pRecord->length;
                    hz = pRecord->height;
                    if (pRecord->type == 0) {
                        recordOffset += sizeof(ObjectPointBounds);
                        pEntry->pBounds->pProjection = pRecordScratch;
                        countRecords++;
                        corners[0].x = FixMul(hx, 0x8000);
                        corners[0].y = 0;
                        corners[0].z = FixMul(hy, 0x8000);
                        corners[1].x = FixMul(hx, 0x8000);
                        corners[1].y = 0;
                        corners[1].z = -FixMul(hy, 0x8000);
                        corners[2].x = -FixMul(hx, 0x8000);
                        corners[2].y = 0;
                        corners[2].z = -FixMul(hy, 0x8000);
                        corners[3].x = -FixMul(hx, 0x8000);
                        corners[3].y = 0;
                        corners[3].z = FixMul(hy, 0x8000);
                        corners[4].x = FixMul(hx, 0x8000);
                        corners[4].y = hz;
                        corners[4].z = FixMul(hy, 0x8000);
                        corners[5].x = FixMul(hx, 0x8000);
                        corners[5].y = hz;
                        corners[5].z = -FixMul(hy, 0x8000);
                        corners[6].x = -FixMul(hx, 0x8000);
                        corners[6].y = hz;
                        corners[6].z = -FixMul(hy, 0x8000);
                        corners[7].x = -FixMul(hx, 0x8000);
                        corners[7].y = hz;
                        corners[7].z = FixMul(hy, 0x8000);
                        FixMatrix_Multiply(&matrix, &pRecord->matrix,
                                           &pObject->matrix);
                        for (k = 0; k < 8; k++) {
                            FixMatrix_RotateVector(&v, &corners[k], &matrix);
                            corners[k] = v;
                        }
                        v.x = corners[0].x - corners[6].x;
                        v.y = corners[0].y - corners[6].y;
                        v.z = corners[0].z - corners[6].z;
                        length = FixVecLength(&v);
                        pEntry->pBounds->radius = length;
                        pEntry->pBounds->radius = FixMul(pEntry->pBounds->radius, 0x8000);
                        FixMatrix_GetRight(&basis[0], &matrix);
                        FixMatrix_GetUp(&basis[1], &matrix);
                        FixMatrix_GetForward(&basis[2], &matrix);
                        Collision_BuildObjectPointBounds((int *)&basis[0], &corners[0], pEntry->pBounds);
                    } else {
                        pEntry->pBounds->pProjection = 0;
                        pEntry->pBounds->radius = hx;
                    }
                    pRecord = pRecord->pNext;
                } while (pRecord != NULL);
            }
            sector++;
        } while (sector < g_sectorCount);
    }
    StageObject_LoadAndClassifyMeshes();
    StageObject_ResetMovingObjectCountsAndPhases();
}

// ---------------------------------------------------------------------------
// Splits of the current car (0x411f70).

short StageTiming_GetCheckpointField10(int index);
int StageTiming_GetClampedElapsedCarTime(int car);
BYTE StageTiming_GetCheckpointField1A(int index);
int StageTiming_GetValidStartTime(int index);
int StageTiming_GetClockTime(void);
void StageUI_NoOpMapRelease(void);
int Frontend_StoreSelectedDeviceOption(int index);

// Screen height and split bar setting the split bars were laid out for (the
// width is g_stageLeader.splitBarResX); a change lays them out again.
// GLOBAL: CMR2 0x0051711c
int g_splitBarResY = -1;
// GLOBAL: CMR2 0x00537080
int g_splitBarEnabled;
// Per-car split bar entry: [0] split index, [0xc] current split time.

// Per-frame update of one car's split bar: detects a graphics mode change and
// rebuilds the split reference table, initialises the per-car split state from
// the current game state, converts the car's current split time into the bar
// coordinates and finally redraws the bar.
// match 71%: logica, constantes y orden de llamadas exactos; difieren el reparto
// de registros (car*0x48 en EDI vs ESI), el plegado del calculo del indice del
// marcador y el orden de algunas comparaciones del bloque de estado.
// The two bar rectangles (x, y, w, h each) kept at ints 13..16 of a car's
// split display record.
#define CAR_SPLIT_BAR(car, k) (((short *)&g_splitTimeColours[(car) * 0x28 + 13])[k])

// FUNCTION: CMR2 0x00411f70
void RallyData_UpdateCarSplitBar(int param_1, int param_2)
{
    CategoryBestResults *pInfo;
    int splitTime;
    int marker;
    int splitIndex;
    int i;
    int first;
    int range;
    int fallback;

    if (g_pGraphics->resX != g_stageLeader.splitBarResX ||
        g_pGraphics->resY != g_splitBarResY ||
        g_splitBarEnabled != (CGameInfo::IsSplitBarEnabled() & 0xff)) {
        RallyData_LayoutPlayerSplitBars();
        g_stageLeader.splitBarResX = g_pGraphics->resX;
        g_splitBarResY = g_pGraphics->resY;
        g_splitBarEnabled = CGameInfo::IsSplitBarEnabled() & 0xff;
    }
    if (g_splitTargetsLoaded == 0 &&
        (CGameInfo::GetConfiguredGameMode() != 7 || g_splitGapDisplayFrames[param_1] == 0)) {
        if (CGameInfo::GetConfiguredGameMode() == 3 || CGameInfo::GetConfiguredGameMode() == 7) {
            RallyData_ResetPlayerSplitDisplays();
            g_splitTargetsLoaded = 1;
            if (CGameInfo::GetConfiguredGameMode() == 7) {
                g_stageTargetTime = CFrontend::GetCurrentRecordOptionValue();
                pInfo = RallyData_GetDriverBestResults((StageUI_GetRaceEndEventCount() & 0xff) + param_1);
                g_profileBestStageTime = pInfo->arcadeStageTimes[RallyData_GetSelectionBits10To11() & 0xff]
                                                                [RallyData_GetSelectionBits12To13() & 0xff].time;
            }
            if (CGameInfo::GetConfiguredGameMode() == 3) {
                g_stageTargetTime = CGameInfo::GetGameInfoFieldA4Address()->rallyStageRecordTimes[
                    (RallyDataCountryIndex() & 0xff) * 0xb + (RallyDataStageIndex() & 0xff)].value >> 7 & 0xffff;
                pInfo = RallyData_GetDriverBestResults((StageUI_GetRaceEndEventCount() & 0xff) + param_1);
                g_profileBestStageTime = pInfo->rallyStageTimes[RallyDataCountryIndex() & 0xff]
                                                               [RallyDataStageIndex() & 0xff].time;
            }
            for (i = 0; i < 12; i++)
                g_splitTargetTimes[i + 1] = Frontend_StoreSelectedDeviceOption(i);
        } else {
            if (StageTiming_GetSplitDisplayState() != 0) {
                g_splitTargetsLoaded = 1;
                for (i = 0; i < 12; i++)
                    g_splitTargetTimes[i + 1] = StageTiming_GetSplitTimeForPosition(0, i);
            }
            if (CGameInfo::GetConfiguredGameMode() == 6 || CGameInfo::GetConfiguredGameMode() == 5) {
                g_stageTargetTime = CFrontend::GetCurrentRecordOptionValue();
                pInfo = RallyData_GetDriverBestResults((StageUI_GetRaceEndEventCount() & 0xff) + param_1);
                g_profileBestStageTime = pInfo->arcadeStageTimes[RallyData_GetSelectionBits10To11() & 0xff]
                                                                [RallyData_GetSelectionBits12To13() & 0xff].time;
            }
        }
    }
    if (CGameInfo::GetConfiguredGameMode() == 5 || CGameInfo::GetConfiguredGameMode() == 6 ||
        CGameInfo::GetConfiguredGameMode() == 7 ||
        (CGameInfo::GetConfiguredGameMode() == 4 || RallyData_GetFlag25() != 0)) {
        StageUI_NoOpMapRelease();
    }
    splitTime = (unsigned short)StageTiming_GetCheckpointField10(param_1);
    if (CGameInfo::GetConfiguredGameMode() != 7 && CGameInfo::GetConfiguredGameMode() != 0xc &&
        (CGameInfo::GetConfiguredGameMode() != 3 || RallyData_GetFlag25() == 0)) {
        g_stageSplitData[param_1].times[0] = StageTiming_GetClockTime();
        if (StageTiming_GetCheckpointField1A(param_1) != 0 && param_2 == 0)
            g_stageSplitData[param_1].times[0] = StageTiming_GetValidStartTime(param_1);
        else if (param_2 != 0)
            g_stageSplitData[param_1].times[0] = 0;
    } else {
        g_stageSplitData[param_1].times[0] = StageTiming_GetClampedElapsedCarTime(param_1);
        if (CGameInfo::GetConfiguredGameMode() == 3 && StageTiming_GetCheckpointField1A(param_1) != 0 &&
            param_2 == 0)
            g_stageSplitData[param_1].times[0] = StageTiming_GetValidStartTime(param_1);
        else if (param_2 != 0)
            g_stageSplitData[param_1].times[0] = 0;
    }
    RallyData_UpdateSplitDisplayAndStageRecord(param_1);
    StageTiming_GetSplitDisplayState();
    splitIndex = g_stageSplitData[param_1].split;
    first = g_classificationTable[splitIndex * 2];
    CAR_SPLIT_BAR(param_1, 0) = g_splitBarRects[splitIndex + param_1 * 0x14].x;
    CAR_SPLIT_BAR(param_1, 1) = g_splitBarRects[splitIndex + param_1 * 0x14].y;
    CAR_SPLIT_BAR(param_1, 3) = g_splitBarRects[splitIndex + param_1 * 0x14].h;
    marker = g_splitBarRects[splitIndex + param_1 * 0x14].w;
    range = g_classificationTable[splitIndex * 2 + 2] - first;
    if (range != 0)
        fallback = (splitTime - first) * marker / range;
    else
        fallback = 0;
    CAR_SPLIT_BAR(param_1, 2) = fallback;
    CAR_SPLIT_BAR(param_1, 4) = CAR_SPLIT_BAR(param_1, 0) + fallback;
    CAR_SPLIT_BAR(param_1, 5) = CAR_SPLIT_BAR(param_1, 1);
    CAR_SPLIT_BAR(param_1, 6) = marker - fallback;
    CAR_SPLIT_BAR(param_1, 7) = CAR_SPLIT_BAR(param_1, 3);
    RallyData_DispatchActiveGameModeState(param_1);
    if (RallyData_GetSelectionFlag26() != 0 && (BYTE)RallyData_GetSetupFlag11() != 0)
        RallyData_UpdateCountdownAndFinishSounds(param_1);
    if (CGameInfo::GetConfiguredGameMode() == 5 || CGameInfo::GetConfiguredGameMode() == 6 ||
        CGameInfo::GetConfiguredGameMode() == 7 || RallyData_GetFlag25() != 0)
        RallyData_UpdateCarSplitDisplayRows(param_1);
    else
        RallyData_SelectNearbyRankSplitRows(param_1);
    if (g_splitGapDisplayFrames[param_1] != 0)
        g_splitGapDisplayFrames[param_1]--;
}

void RallyData_ResetPlayerSplitDisplays(void);
void StageUI_FitMapToRoute(void);

// Resets the stage timing state: clock rate (24000 in the flag-25/24 modes,
// 48000 otherwise), split/lap counters and, on a fresh start, the record name.
// FUNCTION: CMR2 0x00411450
void RallyData_ResetStageClockAndRecords(int keepName)
{
    int i;

    if ((char)RallyData_GetFlag25() == 0 && (char)RallyData_GetFlag24() == 0)
        g_stageDefaultTargetTime = 48000;
    else
        g_stageDefaultTargetTime = 24000;
    RallyData_ResetPlayerSplitDisplays();
    g_stageFinishBeepPlayed = 0;
    *(short *)g_unk0x00536c08 = 0;
    *(short *)g_unk0x00536cac = 0;
    for (i = 0; i < 2; i++) {
        g_unk0x00536c00[i] = 0;
        g_finishStageRecordMessage[i] = 0;
        g_finishLeaderMessage[i] = 0;
        g_unk0x00536ec4[i] = 0;
        g_unk0x00536c88[i] = 0;
    }
    if (keepName == 0) {
        g_stageLeaderTime = 0;
        sprintf(g_stageLeader.name, "%s", CMain::m_logFileBlankLine);
    }
    if (CGameInfo::GetConfiguredGameMode() == 5 || CGameInfo::GetConfiguredGameMode() == 6 || CGameInfo::GetConfiguredGameMode() == 7 ||
        CGameInfo::GetConfiguredGameMode() == 4 || CGameInfo::GetConfiguredGameMode() == 11 || CGameInfo::GetConfiguredGameMode() == 12 ||
        (char)RallyData_GetFlag25())
        StageUI_FitMapToRoute();
    g_stageStartRecorded = 0;
    g_splitTargetTimes[0] = 0;
}

void Particle_BuildTriangleStripIndices(void);
void Particle_Init(int typeCount, int particleCount);
void Graphics_CreateSharedWriteOnlyVertexBuffer(void);
void Dash_Reset(void);
void StageTiming_ResetCheckpointSlotStates(void);
void CarLights_LoadTextures(void);
void StageLights_LoadTextures(void);
void StageTiming_LoadWeatherParticleTextures(void);
void StageTiming_LoadEffectParticleTextures(void);
void StageUI_LoadCoDriverArrows(void);
void Dash_LoadTextures(void);
void InRaceMenu_LoadTextures(void);
void RallyData_CreateFrontendChallengeScene(void);

// Loads the textures of the in-race HUD and stage objects.
// FUNCTION: CMR2 0x0040f020
void RallyData_LoadRaceHUDTextures(void)
{
    CarLights_LoadTextures();
    StageLights_LoadTextures();
    StageTiming_LoadWeatherParticleTextures();
    StageTiming_LoadEffectParticleTextures();
    StageUI_LoadCoDriverArrows();
    Dash_LoadTextures();
    InRaceMenu_LoadTextures();
}

// Race start: font blending, scene, particle system, render settings and all
// the textures the race needs.
// FUNCTION: CMR2 0x0040efa0
void RallyData_InitializeRaceRendering(void)
{
    Font_SetBlendMode(2);
    RallyData_CreateFrontendChallengeScene();
    RallyData_ValidateIndex(1);
    if (CGameInfo::IsActiveCheatEnabled(2) != 0)
        CGame::SetSectorDrawState(2);
    else
        CGame::SetSectorDrawState(3);
    RallyData_IsChampionshipFinalStage();
    Particle_BuildTriangleStripIndices();
    // Pool 400 -> 1200 for the deliberately heavy particle amounts (user
    // request): chips guard at 60% live, plume reserves 64, motes still
    // evict first. Record is 0x6c bytes on Win32 (LayoutChecks), a few
    // hundred bytes here: the bump costs well under 1 MB.
    Particle_Init(0x24, 1200);   // 0x22 air motes (video.air_motes), 0x23 snow glints (video.snow_glint); original types unchanged
    if (CGameInfo::IsActiveCheatEnabled(0) != 0)
        Glow_AllocateEntryTable(4);
    else
        Glow_AllocateEntryTable(0xa0);
    if (g_pGraphics->field913_0x3bc & 0x20)
        Graphics_CreateSharedWriteOnlyVertexBuffer();
    Dash_Reset();
    RallyData_LoadRaceHUDTextures();
    StageTiming_ResetCheckpointSlotStates();
}

// Largest class step allowed between two consecutive opponents, and the extra
// random hold length, per car class.
// GLOBAL: CMR2 0x0051695c
BYTE g_opponentClassMaxStep[9] = {0, 0, 1, 1, 1, 1, 0, 1, 1};
// GLOBAL: CMR2 0x00516968
BYTE g_unk0x00516968[9] = {2, 2, 3, 2, 2, 3, 2, 2, 3};
// Minimum hold length per car class (never written by the game: all zero).
// GLOBAL: CMR2 0x005338f0
BYTE g_opponentClassMinHold[9];

// GLOBAL: CMR2 0x00516a04
char g_strHere[] = "here\n";

// Picks the opponent line-up of the three groups: random classes and ratings
// drawn from the rally's class table, with runs of the same class and limited
// jumps between neighbours. Network games reuse the host's seed instead.
// FUNCTION: CMR2 0x0040dc30
void RallyData_PickOpponentLineups(void)
{
    int classes[8];
    char ratings[8];
    int split;
    int *pClass;
    int row;
    int hold;
    int group;
    char rating;
    int cls;
    int prev;
    int a;
    int b;
    int d;
    int n;
    int wrap;
    int maxStep;
    char c;
    int k;
    int i;
    int *pOut;
    int *pIn;

    if ((CGameInfo::GetConfiguredGameMode() == 2 || CGameInfo::GetConfiguredGameMode() == 9) && g_driverGroupTable.field_0x6d0 != 0) {
        puts(g_strHere);
        RallyData_ResetDriverPairingTables(g_driverGroupTable.field_0x53c);
        return;
    }
    srand(Sys_GetTicks());
    if ((BYTE)RallyDataState() > 1 && CGameInfo::IsConfiguredMultiplayer() == 0)
        split = 1;
    else
        split = 0;
    cls = split;
    group = 0;
    do {
        hold = 0;
        pClass = classes;
        for (row = 0; row < 8; row++, pClass++) {
            if ((char)RallyData_GetSelectionFlag28()) {
                ratings[row] = 0;
                *pClass = g_unk0x0051627c[g_selectedRallyData & 0x1f][group * 10];
                continue;
            }
            if (hold <= 0) {
                prev = cls;
                hold = (int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, rand()), g_oneOverRandMax), CGraphics::m_65536));
                hold = FixMul(hold, 0xa0000);
                cls = g_unk0x0051627c[g_selectedRallyData & 0x1f][group * 10 + (hold >> 16)];
                rating = (char)FixMulShift32(0xa0000, hold);
                if (row > 0) {
                    wrap = 0;
                    a = prev;
                    if (prev >= 6) {
                        a = prev - 3;
                        wrap = 1;
                    }
                    b = cls;
                    if (cls >= 6) {
                        b = cls - 3;
                        wrap = 1;
                    }
                    d = b - a;
                    maxStep = g_unk0x00516968[prev];
                    if (abs(d) > maxStep) {
                        if (d <= 0)
                            maxStep = -maxStep;
                        d = maxStep;
                    }
                    n = d + a;
                    if (n >= 3 && wrap)
                        n += 3;
                    if (cls != n) {
                        c = -1;
                        cls = n;
                        for (k = 0; k < 10; k++) {
                            if ((int)g_unk0x0051627c[g_selectedRallyData & 0x1f][group * 10 + k] >= n) {
                                c = (char)k;
                                break;
                            }
                        }
                        rating = (char)(rand() % 10) + c * 10;
                    }
                }
                hold = (rand() % (g_opponentClassMaxStep[cls] + 1) + g_opponentClassMinHold[cls]) * 2;
                if (row != 0)
                    hold++;
            } else {
                hold--;
            }
            if (group == 2 && row > 3) {
                *pClass = 0;
                ratings[row] = 0;
            } else {
                *pClass = cls;
                ratings[row] = rating;
            }
        }
        pOut = &g_rallyPairingTables.pairs[group * 4][1];
        for (i = 0; i < 4; i++) {
            pOut[-1] = classes[i * 2];
            if (split == 0)
                pOut[0] = classes[i * 2 + 1];
            else
                pOut[0] = classes[i * 2];
            pOut += 2;
        }
        RallyData_SelectOpponentSkillClasses(group, (int *)&g_unk0x0051627c[g_selectedRallyData & 0x1f][group * 10], ratings);
        RallyData_InterpolateGroupGrip(group);
        group++;
    } while (group < 3);
    for (i = 0; i < 11; i++) {
        if ((unsigned short)RallyData_GetIndexedStageScoreScale(g_selectedRallyData & 0x1f, i, 0) >= 0x834) {
            g_rallyPairingTables.pairs[i][0] = 2;
            g_rallyPairingTables.pairs[i][1] = 2;
            g_rallyPairingTables.skills[i] = 2;
        }
    }
    RallyData_UpdateStageWetShares();
}

#include "GenericFileLoader.h"

char *Knockout_GetCarNameForSide(KnockoutMatch *pMatch, int side);
char *Knockout_GetDriverNameForSide(KnockoutMatch *pMatch, int side);
BYTE StageTiming_GetSplitDisplayState(void);
BYTE StageUI_GetRaceEndEventCount(void);
int StageTiming_GetSplitTableEntry(int iSplit, int iIndex);
void Race_DrawPlayerMessage(char *pText, int *pColour, int player, int shadow, int x, int y);
extern BYTE g_gapTextColour[4];

// GLOBAL: CMR2 0x00516e20
char g_strVersusFormat[] = "%s %s %s";

// Shows the "<driver> vs <driver>" caption of a head-to-head stage (knockout
// match or two-player race), either at the bottom of the screen or in the
// given player's view.
// FUNCTION: CMR2 0x00412390
void RallyData_DrawHeadToHeadDriverCaption(int bottom, int player)
{
    char name2[32];
    char name1[32];

    if (RallyData_IsHeadToHeadRaceMode() != 0 && player != 0)
        return;
    if (CGameInfo::GetConfiguredGameMode() == 4) {
        if (RallyData_GetUsableRecordCategory(*(BYTE *)RallyData_GetRoundEntry() & 0x1f) == -1)
            Knockout_GetCarNameForSide((KnockoutMatch *)RallyData_GetRoundEntry(), 0);
        else
            Knockout_GetDriverNameForSide((KnockoutMatch *)RallyData_GetRoundEntry(), 0);
        sprintf(name1, CRegKey::m_regKeyPathFormatValue, CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest));
        if (RallyData_GetUsableRecordCategory((BYTE)(*RallyData_GetRoundEntry() >> 5) & 0x1f) == -1)
            Knockout_GetCarNameForSide((KnockoutMatch *)RallyData_GetRoundEntry(), 1);
        else
            Knockout_GetDriverNameForSide((KnockoutMatch *)RallyData_GetRoundEntry(), 1);
        sprintf(name2, CRegKey::m_regKeyPathFormatValue, CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest));
    } else {
        if (StageTiming_GetSplitDisplayState() == 0)
            return;
        RallyData_CopyDriverDisplayName(StageTiming_GetSplitTableEntry(StageUI_GetRaceEndEventCount(), 0));
        sprintf(name1, CRegKey::m_regKeyPathFormatValue, CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest));
        RallyData_CopyDriverDisplayName(StageTiming_GetSplitTableEntry(StageUI_GetRaceEndEventCount(), 1));
        sprintf(name2, CRegKey::m_regKeyPathFormatValue, CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest));
    }
    sprintf(CFrontend::m_stringDest, g_strVersusFormat, name1, CFrontend::GetTextString(0x75), name2);
    if (bottom != 0) {
        RallyData_DrawListItem((int)(g_pGraphics->resX * 24) / 640, (int)(g_pGraphics->resY * 24) / 480,
                               CFrontend::m_stringDest, 1, 0xff);
        return;
    }
    Race_DrawPlayerMessage(CFrontend::m_stringDest, (int *)g_gapTextColour, player, 1, (int)(g_pGraphics->resX * 24) / 640,
                 (int)(g_pGraphics->resY * 24) / 480);
}

void NetRace_DrawPlayerFlashOverlay(unsigned int player, short *pRect, int check);
BYTE Race_GetStateByte(void);
int GameMenu_IsPauseHeaderActive(void);
extern BYTE g_unk0x00536ac8;
extern char g_keypadFormat[];

// GLOBAL: CMR2 0x00516ce0
BYTE g_unk0x00516ce0[4] = { 0x72, 0x80, 0xae, 0xbf };
// GLOBAL: CMR2 0x00516e2c
char g_strName0x00516e2c[] = "%s";
// GLOBAL: CMR2 0x00536acc
unsigned int g_unk0x00536acc;
// GLOBAL: CMR2 0x00536be8
int g_unk0x00536be8;

// End-of-frame overlay of a view. On the last view it draws, per player, the
// network status box, the head-to-head banners ("waiting for" / "winner") and
// the split-screen divider; then the fade-in of the first 2.25 s of a stage
// (with the loading bar) and finally renders the view's scene.
// FUNCTION: CMR2 0x0040f8d0
void RallyData_DrawViewEndFrameOverlay(CallbackStateMachine *pKey, int view)
{
    BYTE colour[4];
    char name[8];
    short rect[4];
    short fadeRect[4];
    BYTE fadeColour[4];
    int i;
    unsigned short other;
    CallbackStateRecord *pRecords;
    unsigned int now;
    unsigned int alpha;

    colour[0] = 0xff;
    colour[1] = 0xff;
    colour[2] = 0xff;
    colour[3] = 0xff;
    if (CGameInfo::GetGraphicsOptionBits27To28() == 1 && CGraphics::GetTextureMemoryKilobytes() > 0x800 && g_unk0x00536be8 == 0)
        g_unk0x00536be8 = 1;
    if (view == 0 && Race_IsMultiplayerRecordMode10() != 0)
        return;
    if (g_unk0x00536ac8) {
        g_unk0x00536acc = CMain::GetFrameDelta();
        g_unk0x00536ac8 = 0;
    }
    if (view == pKey->count - 1) {
        for (i = 0; i < (BYTE)RallyDataState(); i++) {
            NetRace_DrawPlayerFlashOverlay(i, (short *)StageObject_GetPlayerViewRectangle(i), CGameInfo::IsInRaceMenuOpen() ? 1 : 0);
            pRecords = pKey->records;
            if ((BYTE)pRecords[i].packedState == 9) {
                other = (i + 1) % 2;
                if ((BYTE)pRecords[other].packedState == 8 && CGameInfo::IsInRaceMenuOpen() == 0 && (BYTE)RallyDataState() > 1 &&
                    Race_ReadTransitionState3811C() == 100) {
                    if ((char)RallyData_GetSelectionFlag26()) {
                        if ((char)RallyData_GetSetupFlag11() == 0 || RallyData_GetSetupModeBits() != 0)
                            goto divider;
                        Sprite_FillRect(&g_pGraphics->field309_0x150, (short *)StageObject_GetPlayerViewRectangle(i), g_unk0x00516ce0, 2);
                        sprintf(CFrontend::m_stringDest, g_strVersusFormat, CFrontend::GetTextString(0x85),
                                CFrontend::GetTextString(0x87), (char *)RallyData_GetRecord(other));
                    } else {
                        Sprite_FillRect(&g_pGraphics->field309_0x150, (short *)StageObject_GetPlayerViewRectangle(i), g_unk0x00516ce0, 2);
                        if (Race_GetStateByte() != 0xff && (char)Race_GetStateByte() == i) {
                            if (CGameInfo::GetConfiguredGameMode() == 4)
                                sprintf(name, g_strName0x00516e2c,
                                        Knockout_GetCarNameForSide((KnockoutMatch *)RallyData_GetRoundEntry(), i));
                            else
                                sprintf(name, g_strName0x00516e2c, (char *)RallyData_GetRecord(i));
                            sprintf(CFrontend::m_stringDest, g_keypadFormat, CFrontend::GetTextString(0x94), name);
                        } else {
                            if (!(CGameInfo::GetConfiguredGameMode() == 4)) {
                                sprintf(name, g_strName0x00516e2c, (char *)RallyData_GetRecord(other));
                            } else {
                                sprintf(name, g_strName0x00516e2c,
                                        Knockout_GetCarNameForSide((KnockoutMatch *)RallyData_GetRoundEntry(), other));
                            }
                            sprintf(CFrontend::m_stringDest, g_strVersusFormat, CFrontend::GetTextString(0x86),
                                    CFrontend::GetTextString(0x87), name);
                        }
                    }
                    Race_DrawPlayerMessage(CFrontend::m_stringDest, (int *)colour, i, 0, -1, -1);
                }
            }
        divider:
            if (Race_IsMultiplayerRecordMode10() == 0 && RallyData_IsHeadToHeadRaceMode() != 0 && CGameInfo::IsInRaceMenuOpen() == 0 && i == 1 &&
                ((BYTE)pRecords[1].packedState != 10 || GameMenu_IsPauseHeaderActive() != 0)) {
                if (CGameInfo::IsSplitBarEnabled()) {
                    rect[0] = 0;
                    rect[2] = ((short *)StageObject_GetPlayerViewRectangle(1))[2];
                    rect[3] = 1;
                    rect[1] = ((short *)StageObject_GetPlayerViewRectangle(1))[1];
                } else {
                    rect[0] = ((short *)StageObject_GetPlayerViewRectangle(1))[0];
                    rect[2] = 1;
                    rect[1] = ((short *)StageObject_GetPlayerViewRectangle(1))[1];
                    rect[3] = ((short *)StageObject_GetPlayerViewRectangle(1))[3];
                }
                Sprite_FillRect(&g_pGraphics->field309_0x150, rect, g_unk0x00516cd8, 2);
            }
        }
    }
    if (pKey->records[view].bits.state < 8 && CGameInfo::GetConfiguredGameMode() != 4) {
        // PORT: the loading screen fading out, laid out as the menus.
        MenuScreen_BeginOverlay();
        now = CMain::GetFrameDelta();
        fadeRect[0] = 0;
        fadeRect[1] = 0;
        fadeRect[2] = (short)g_pGraphics->resX;
        fadeRect[3] = (short)g_pGraphics->resY;
        if (now - g_unk0x00536acc < 0xe1) {
            if (now - g_unk0x00536acc < 0x19)
                alpha = 0xff;
            else
                alpha = 0xff - (now * 0xff - g_unk0x00536acc * 0xff - 0x18e7) / 200;
            fadeColour[0] = 0x9c;
            fadeColour[1] = 0xb4;
            fadeColour[2] = 0xac;
            fadeColour[3] = (BYTE)alpha;
            Sprite_FillRect(&g_pGraphics->field309_0x150, fadeRect, fadeColour, 1);
            RallyData_DrawFadedLoadingText((BYTE)alpha);
            RallyData_DrawLoadingProgress(100, 0, (BYTE)alpha);
        }
        MenuScreen_EndOverlay();
    }
    g_viewScreenRect = RallyData_GetViewScreenRectangle(view);
    // Only this actual race viewport, with no in-race menu, admits added rain/snow.
    // Splash/options/stage-information scenes use the same scene renderer but
    // never enter this explicit scope.
    extern void StageObject_PrepareVolumetricClouds(void);
    StageObject_PrepareVolumetricClouds();
    // OpenCMR2: the in-race (Esc) menu marks the frame as not a race world, which switches every race effect off in the renderer (depth of field,
    // tunnel vision, speed streak, vapor trails, motion blur, particles, clouds). With the debug tuning panel open they stay on, so the settings can
    // be seen while paused. The car setup editor keeps its preview as it was.
    Gfx_SetRaceWorld(CGameInfo::IsInRaceMenuOpen()==0 || (TuningPanel_PreviewActive() && InRaceMenu_IsPlayerEditingCarSetup(view)==0));
    Game_DrawSceneViewport(RallyData_GetChallengeRenderState(), g_viewNodes[view], g_viewScreenRect, view, 1);
    Gfx_SetRaceWorld(FALSE);
    RallyData_NotifyAdjacentMenuKeyValue(pKey, view);
}

int InRaceMenu_IsPlayerEditingCarSetup(unsigned int param1);

// Gap display of the stage HUD: time against the target (ahead in one colour,
// behind in the other) or a blank gap when there is no reference.
// FUNCTION: CMR2 0x004137e0
void RallyData_DrawTargetTimeGap(int car, short *pRect)
{
    int reference;
    int time;
    int split;
    char shown;
    int lastMode;
    short x;
    short y;
    int dx;
    int dy;
    int yFixed;
    int xFixed;

    if (CGameInfo::GetConfiguredGameMode() == 6 || CGameInfo::GetConfiguredGameMode() == 5 ||
        ((char)RallyData_GetSelectionFlag27() && CGameInfo::GetGameModeOptionBit19() == 0)) {
        reference = 0;
        split = g_stageSplitData[car].split;
        time = g_splitGapTime[car];
    } else {
        g_splitGapBlank[car] = 0;
        time = g_stageSplitData[car].lastSplitTime;
        split = g_stageSplitData[car].split;
        reference = g_stageSplitData[car].targetTime;
        if ((CGameInfo::GetConfiguredGameMode() == 3 || CGameInfo::GetConfiguredGameMode() == 7) &&
            (InRaceMenu_IsPlayerEditingCarSetup(car) == 0 || CGameInfo::IsInRaceMenuOpen() == 0)) {
            if (g_stageTargetTime == g_stageDefaultTargetTime)
                return;
            if (g_stageTargetTime == 0)
                return;
        }
    }
    shown = g_splitGapDisplayFrames[car] != 0 ? -1 : 0;
    if (RallyData_IsHeadToHeadRaceMode())
        CGameInfo::IsSplitBarEnabled();
    lastMode = 0;
    CGameInfo::GetConfiguredGameMode();
    if (CGameInfo::GetConfiguredGameMode() == 12)
        lastMode = 1;
    if (InRaceMenu_IsPlayerEditingCarSetup(car) != 0 && CGameInfo::IsInRaceMenuOpen() != 0 && (time == -1 || shown == 0 || split == 0)) {
        shown = -1;
        if (split <= 0) {
            time = 0x1c3e;
            reference = 0x1bbc;
            goto draw;
        }
    }
    if (time == -1)
        return;
    if (shown == 0)
        return;
    if (split == 0 && lastMode == 0)
        return;
draw:
    dx = 0;
    dy = 0;
    if (RallyData_IsHeadToHeadRaceMode()) {
        if (car == 0) {
            if (CGameInfo::IsSplitBarEnabled())
                dy = -((int)g_pGraphics->resY / 2);
        } else if (car == 1 && CGameInfo::IsSplitBarEnabled() == 0) {
            dx = (int)g_pGraphics->resX / 2;
        }
    }
    x = (short)((int)(g_pGraphics->resX * 0xc00) >> 16) + dx;
    y = (short)(((int)(g_pGraphics->resY * 0xd668) >> 16) + dy) + (short)((int)(g_pGraphics->resY * 0xccc) >> 16) - 1;
#define GAP_POS()                                                                                   \
    yFixed = FixDiv((int)FTOI(__int64, X87S_MUL((double)(y + 1), CGraphics::m_65536)),                          \
                    (int)FTOI(__int64, X87S_MUL((double)(int)g_pGraphics->resY, CGraphics::m_65536)));          \
    xFixed = FixDiv((int)FTOI(__int64, X87S_MUL((double)(x + 4), CGraphics::m_65536)),                          \
                    (int)FTOI(__int64, X87S_MUL((double)(int)g_pGraphics->resX, CGraphics::m_65536)))
    if (CGameInfo::GetConfiguredGameMode() == 8 || CGameInfo::GetConfiguredGameMode() == 9 || CGameInfo::GetConfiguredGameMode() == 10 ||
        CGameInfo::GetConfiguredGameMode() == 11 || CGameInfo::GetConfiguredGameMode() == 12) {
        if (g_splitTargetTimes[0] == 0) {
            if (time <= reference) {
                GAP_POS();
                PrepareFormatGapToLeader(reference - time, 0, 4, 4, xFixed, yFixed, &g_unk0x0051709c, 9, 1, 1);
            } else {
                GAP_POS();
                PrepareFormatGapToLeader(time - reference, 0, 4, 4, xFixed, yFixed, &g_unk0x005170a0, 9, 0, 1);
            }
        } else {
            GAP_POS();
            PrepareFormatGapToLeader(-1, 0, 4, 4, xFixed, yFixed, &g_unk0x0051709c, 9, 1, 1);
        }
    } else {
        if (CGameInfo::GetConfiguredGameMode() == 12)
            return;
        if (g_splitGapBlank[car] == 0) {
            if (time <= reference) {
                GAP_POS();
                PrepareFormatGapToLeader(reference - time, 0, 4, 4, xFixed, yFixed, &g_unk0x0051709c, 9, 1, 1);
            } else {
                GAP_POS();
                PrepareFormatGapToLeader(time - reference, 0, 4, 4, xFixed, yFixed, &g_unk0x005170a0, 9, 0, 1);
            }
        } else {
            GAP_POS();
            PrepareFormatGapToLeader(-1, 0, 4, 4, xFixed, yFixed, &g_unk0x0051709c, 9, 1, 1);
        }
    }
#undef GAP_POS
}

void RallyData_DrawCarSplitTimeBar(int car, short *pRect);
void RallyData_DrawSplitBarAndLeaderMarkers(int car, short *pRect);
int StageTiming_IsClockOverlayActive(void);
int Race_IsRouteModeWithoutFlag18(void);

// Stage time panel of the HUD: the split boxes, a background panel (taller when
// the subtitles need room) and the running stage time.
// FUNCTION: CMR2 0x00413fe0
void RallyData_DrawStageTimePanel(int car, short *pRect)
{
    short rect[4];
    int dx;
    int dy;
    int yFixed;
    int xFixed;

    if (CGameInfo::GetConfiguredGameMode() == 8 || CGameInfo::GetConfiguredGameMode() == 9 || CGameInfo::GetConfiguredGameMode() == 11)
        RallyData_DrawSplitBarAndLeaderMarkers(car, pRect);
    else
        RallyData_DrawCarSplitTimeBar(car, pRect);
    dx = 0;
    dy = 0;
    rect[2] = (short)((int)(g_pGraphics->resX * 0x4865) >> 16) + 1;
    rect[0] = (short)((int)(g_pGraphics->resX * 0xab9b) >> 16);
    if (RallyData_IsHeadToHeadRaceMode()) {
        if (CGameInfo::IsSplitBarEnabled()) {
            if (car == 1)
                dy = (int)g_pGraphics->resY / 2;
        } else {
            rect[2] = (short)((int)(g_pGraphics->resX * 0x2acc) >> 16) + 11;
            rect[0] = (short)((int)(g_pGraphics->resX * 0xc934) >> 16) - 10;
            if (car == 0)
                dx = -((int)g_pGraphics->resX / 2);
        }
    }
    rect[0] += (short)dx;
    if (CGameInfo::GetConfiguredGameMode() == 8 || CGameInfo::GetConfiguredGameMode() == 9 || CGameInfo::GetConfiguredGameMode() == 11) {
        rect[1] = (short)((int)(g_pGraphics->resY << 12) >> 16) + dy;
        rect[3] = (short)((int)(g_pGraphics->resY << 13) >> 16);
    } else {
        rect[1] = (short)((int)(g_pGraphics->resY * 0x1aac) >> 16) + dy;
        rect[3] = (short)((int)(g_pGraphics->resY * 0x1554) >> 16);
    }
    if (StageTiming_IsClockOverlayActive()) {
        if (CGameInfo::GetGameLanguage() == 1 || CGameInfo::GetGameLanguage() == 3 || CGameInfo::GetGameLanguage() == 2)
            rect[3] += (short)((int)(g_pGraphics->resY * 24) / 480 * 2);
        else
            rect[3] += (short)((int)(g_pGraphics->resY * 24) / 480);
    }
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, (BYTE *)&g_stageHudPanelColour, 2);
    if (StageTiming_IsClockOverlayActive()) {
        if (CGameInfo::GetGameLanguage() == 1 || CGameInfo::GetGameLanguage() == 3 || CGameInfo::GetGameLanguage() == 2)
            rect[3] += (short)((int)(g_pGraphics->resY * -24) / 480 * 2);
        else
            rect[3] += (short)((int)(g_pGraphics->resY * -24) / 480);
    }
    if (Race_IsRouteModeWithoutFlag18()) {
        if (car == 0)
            rect[0] += 7;
        else
            rect[0] += 10;
        yFixed = FixDiv((int)FTOI(__int64, X87S_MUL((double)pRect[1], CGraphics::m_65536)),
                        (int)FTOI(__int64, X87S_MUL((double)(int)g_pGraphics->resY, CGraphics::m_65536)));
        xFixed = FixDiv((int)FTOI(__int64, X87S_MUL((double)rect[0], CGraphics::m_65536)),
                        (int)FTOI(__int64, X87S_MUL((double)(int)g_pGraphics->resX, CGraphics::m_65536)));
        FormatGapToLeader(g_stageSplitData[car].times[0], 4, 4, xFixed, yFixed + 0x2e66,
                          &g_stageHudTextColour, 0x21, NULL, 0);
    } else {
        yFixed = FixDiv((int)FTOI(__int64, X87S_MUL((double)pRect[1], CGraphics::m_65536)),
                        (int)FTOI(__int64, X87S_MUL((double)(int)g_pGraphics->resY, CGraphics::m_65536)));
        xFixed = FixDiv((int)FTOI(__int64, X87S_MUL((double)(pRect[0] + dx), CGraphics::m_65536)),
                        (int)FTOI(__int64, X87S_MUL((double)(int)g_pGraphics->resX, CGraphics::m_65536)));
        FormatGapToLeader(g_stageSplitData[car].times[0], 4, 4, xFixed + 0xaf30, yFixed + 0x2e66,
                          &g_stageHudTextColour, 0x21, NULL, 0);
    }
    Race_IsRouteModeWithoutFlag18();
}

int StageTiming_GetSplitDriverIDForPosition(int iPosition, int iSplit);
int StageTiming_GetSplitTimeForPosition(int iPosition, int iSplit);

// GLOBAL: CMR2 0x00517de0
char g_strADriver[] = "A. DRIVER";
// GLOBAL: CMR2 0x00517dec
char g_strAdr[] = "ADR";

// Split standings panel of the HUD: three rows around the car's position at
// its last split, each with the position, the driver name (short form in split
// screen) and the split time.
// match 72%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00414ed0
void RallyData_DrawSplitStandingsPanel(int car, short *pRect)
{
    short panel[4];
    char positionText[4];
    int *pPosition;
    int textY;
    int rows;
    int time;
    int width;

    pPosition = g_splitNearbyRankRows[car];
    if (*pPosition == -1 && InRaceMenu_IsPlayerEditingCarSetup(car) == 0 && CGameInfo::IsInRaceMenuOpen() == 0)
        return;
    if (RallyData_IsHeadToHeadRaceMode() && CGameInfo::IsSplitBarEnabled() == 0) {
        panel[1] = (short)((int)(g_pGraphics->resY << 12) >> 16) + pRect[1];
        textY = ((int)(g_pGraphics->resY * 0x1b32) >> 16) + pRect[1] - 2;
        rows = 3;
        do {
            panel[0] = (short)((int)(g_pGraphics->resX * 0xc00) >> 16) + pRect[0];
            panel[2] = (short)((int)(g_pGraphics->resX * 0x3333) >> 16);
            panel[3] = (short)((int)(g_pGraphics->resY * 0xccc) >> 16) - 2;
            if (*pPosition == g_stageSplitData[car].position)
                Sprite_FillRect(&g_pGraphics->field309_0x150, panel, (BYTE *)&g_unk0x005170d8, 2);
            else
                Sprite_FillRect(&g_pGraphics->field309_0x150, panel, (BYTE *)&g_stageResultPanelColour, 2);
            if (*pPosition != -1 &&
                (StageTiming_GetSplitDisplayState() == 0 ||
                 StageTiming_GetSplitDriverIDForPosition(*pPosition, g_stageSplitData[car].split) != -1)) {
                sprintf(positionText, g_strOneDigit, *pPosition + 1);
                Font_DrawText(0, positionText, (short)(((int)(g_pGraphics->resX * 0xf95) >> 16) + pRect[0]), textY,
                              (int *)&g_stageResultTextColour, 0x22);
                if (StageTiming_GetSplitDisplayState() != 0) {
                    RallyData_FormatResultDriverName(*pPosition, g_stageSplitData[car].split, 1, 1);
                    Font_DrawText(0, CFrontend::m_stringDest,
                                  (short)(((int)(g_pGraphics->resX * 0x132a) >> 16) + pRect[0]), textY,
                                  (int *)&g_stageResultTextColour, 0x21);
                    time = StageTiming_GetSplitTimeForPosition(*pPosition, g_stageSplitData[car].split);
                } else {
                    Font_DrawText(0, g_strAdr, (short)(((int)(g_pGraphics->resX * 0x132a) >> 16) + pRect[0]), textY,
                                  (int *)&g_stageResultTextColour, 0x21);
                    time = 3000;
                }
                FormatGapToLeader(time, 5, 5,
                                  FixDiv((int)FTOI(__int64, X87S_MUL((double)pRect[0], CGraphics::m_65536)),
                                         (int)FTOI(__int64, X87S_MUL((double)(int)g_pGraphics->resX, CGraphics::m_65536))) +
                                      0x3d69,
                                  FixDiv(textY - 1, g_pGraphics->resY), &g_stageResultTextColour, 0x24, NULL, 0);
            }
            pPosition++;
            panel[1] += panel[3];
            textY += panel[3];
        } while (--rows);
        return;
    }
    panel[1] = (short)((int)(g_pGraphics->resY << 12) >> 16) + pRect[1];
    textY = ((int)(g_pGraphics->resY * 0x1b32) >> 16) + pRect[1] - 2;
    rows = 3;
    do {
        panel[0] = (short)((int)(g_pGraphics->resX * 0xc00) >> 16) + pRect[0];
        panel[2] = (short)((int)(g_pGraphics->resX * 0x47ae) >> 16);
        panel[3] = (short)((int)(g_pGraphics->resY * 0xccc) >> 16) - 2;
        if (*pPosition == g_stageSplitData[car].position)
            Sprite_FillRect(&g_pGraphics->field309_0x150, panel, (BYTE *)&g_unk0x005170d8, 2);
        else
            Sprite_FillRect(&g_pGraphics->field309_0x150, panel, (BYTE *)&g_stageResultPanelColour, 2);
        if (*pPosition != -1) {
            if (StageTiming_GetSplitDisplayState())
                StageTiming_GetSplitDriverIDForPosition(*pPosition, g_stageSplitData[car].split);
            sprintf(positionText, g_strOneDigit, *pPosition + 1);
            Font_DrawText(0, positionText, (short)(((int)(g_pGraphics->resX * 0xf95) >> 16) + pRect[0]), textY,
                          (int *)&g_stageResultTextColour, 0x22);
            RallyData_FormatResultDriverName(*pPosition, g_stageSplitData[car].split, 0, 1);
            width = Font_GetTextWidth(0, (BYTE *)CFrontend::m_stringDest);
            if (width > (int)(g_pGraphics->resX * 0x59) / 640)
                RallyData_FormatResultDriverName(*pPosition, g_stageSplitData[car].split, 1, 1);
            if (StageTiming_GetSplitDisplayState() != 0) {
                Font_DrawText(0, CFrontend::m_stringDest, (short)(((int)(g_pGraphics->resX * 0x132a) >> 16) + pRect[0]),
                              textY, (int *)&g_stageResultTextColour, 0x21);
                time = StageTiming_GetSplitTimeForPosition(*pPosition, g_stageSplitData[car].split);
            } else {
                Font_DrawText(0, g_strADriver, (short)(((int)(g_pGraphics->resX * 0x132a) >> 16) + pRect[0]), textY,
                              (int *)&g_stageResultTextColour, 0x21);
                time = 3000;
            }
            FormatGapToLeader(time, 5, 5,
                              FixDiv((int)FTOI(__int64, X87S_MUL((double)pRect[0], CGraphics::m_65536)),
                                     (int)FTOI(__int64, X87S_MUL((double)(int)g_pGraphics->resX, CGraphics::m_65536))) +
                                  0x51e4,
                              FixDiv(textY - 1, g_pGraphics->resY), &g_stageResultTextColour, 0x24, NULL, 0);
        }
        pPosition++;
        panel[1] += panel[3];
        textY += panel[3];
    } while (--rows);
}

// 0x50-byte entry of the table at 0x82c6c8 as seen by the panel enter helper:
// the scoreboard texture slot picked for the panel.

void OptionMenu_DrawResultRows(int param1);
void OptionMenu_DrawStageSummaryPanel(int param1, int param2);
void OptionMenu_DrawCountryFlags(void);
void RallyData_DrawCarSplitTimesPanel(int param1);

void OptionMenu_DrawThreeColourGradientStrips(short *pRect, int unused, unsigned int count, BYTE *pColours);

// GLOBAL: CMR2 0x00527288
char g_str0x00527288[8] = "%d\260c";

// Draws the selected rally's temperature over the bottom of the option panel:
// the label rect is scaled from the current resolution, the alpha ramps from
// the temperature value and the string is drawn with the panel colour.
// FUNCTION: CMR2 0x00503c80
void RallyData_DrawSelectedRallyTemperature(int param_1)
{
    BYTE colour[12];
    short rect[4];
    char *pStr;
    int v;
    int x;
    int y;

    rect[0] = (short)((int)g_pGraphics->resX * 0x1c / 0x280);
    rect[1] = (short)((int)g_pGraphics->resY * 0x16f / 0x1e0);
    rect[2] = (short)((int)g_pGraphics->resX * 0x100 / 0x280);
    rect[3] = (short)((int)g_pGraphics->resY * 0xc / 0x1e0);
    colour[0] = 0x97;
    colour[1] = 0xc2;
    colour[2] = 0xcd;
    colour[3] = 0;
    colour[4] = 0xde;
    colour[5] = 0xd2;
    colour[6] = 0x74;
    colour[7] = 0;
    colour[8] = 0xff;
    colour[9] = 0x7f;
    colour[10] = 0;
    colour[11] = 0;
    OptionMenu_DrawThreeColourGradientStrips(rect, 1, 0xb, colour);
    pStr = (char *)RallyData_GetDriverSelectionFlagRecord(param_1);
    v = *pStr * 0x10000;
    param_1 = FixMul(v + 0x140000, 0x3a9);
    if (param_1 < 0)
        param_1 = 0;
    else if (param_1 > 0x10000)
        param_1 = 0x10000;
    x = (FixMul((int)rect[2] << 16, param_1) >> 16) + rect[0];
    y = rect[3] + rect[1];
    sprintf(CFrontend::m_stringDest, g_str0x00527288, v >> 0x10);
    Font_DrawText(0, CFrontend::m_stringDest, x, y, &g_unk0x005270fc, 0x12);
}

// Enters the option panel selected by 0x82ca1c: rebinds the panel textures and
// the scoreboard texture slot, then rescales the segment rect at 0x82c9ec to
// the current resolution and draws its outline.
// match 88%: every instruction matches except the frame layout of the rect
// transform; the original stores the two dwords of 0x82c9ec into its slots
// (keeping rect[0] in EAX for the first `movsx`) and pushes the call arguments
// later than we do, so the four ESP+disp operands differ. Source-level
// reordering (declaration order, separate locals, reading the global directly)
// does not move MSVC 6's scheduling.
// FUNCTION: CMR2 0x005043b0
void RallyData_EnterSelectedOptionPanel(void)
{
    FixInterp *pEntry;
    short rect[4];

    RallyData_QueueSelectedCarSplitPanel();
    RallyData_DrawCarSplitTimesPanel((int)(signed char)g_selectedOptionPreviewPanel);
    RallyData_DrawCarSplitTimePanels();
    OptionMenu_DrawCountryFlags();
    if (g_selectedOptionPreviewPanel != 0xff) {
        pEntry = g_optionPreviewPanels + (signed char)g_selectedOptionPreviewPanel;
        OptionMenu_DrawResultRows(RallyDataCountryIndex() & 0xff);
        OptionMenu_DrawStageSummaryPanel(RallyDataCountryIndex() & 0xff, pEntry->count);
        RallyData_DrawSelectedRallyTemperature(pEntry->count);
    }
    *(int *)&rect[0] = *(int *)&g_optionPreviewExpandedRect[0];
    *(int *)&rect[2] = *(int *)&g_optionPreviewExpandedRect[2];
    rect[0] = (short)((int)rect[0] * (int)g_pGraphics->resX / 0x280);
    rect[1] = (short)((int)rect[1] * (int)g_pGraphics->resY / 0x1e0);
    rect[2] = (short)((int)rect[2] * (int)g_pGraphics->resX / 0x280);
    rect[3] = (short)((int)rect[3] * (int)g_pGraphics->resY / 0x1e0);
    RallyData_DrawClippedHudRectangleEdges(rect, &g_splitTimeGradients[4], 0);
}
BYTE *GameMenu_GetChampionshipTransitionState(void);

// GLOBAL: CMR2 0x00519704
char g_str0x00519704[8] = "%s.cat";

// Loads the route node file of the selected rally ("<rally>.cat") into
// 0x538a9c: stores the node count in 0x538a88 and in 0x538a84 the index of the
// last node carrying the flag bit, works out whether the route wraps
// (0x538a94) and its scale (0x538c94), drops the direction cache keys, clears
// the per-car race records and rebinds the route probe callback.
// match 78%: implemented from the disassembly; the sequence matches instruction
// by instruction except the record-clearing loop, where MSVC 6 biases the
// induction variable by +4 instead of +8 (so three jump displacements and the
// loop bound operand differ), one sign-fix register (EDX vs ECX), and the loop
// bound symbol, which reccmp renders with each image's own symbol table
// (CONOCIMIENTO 6.u: our .bss layout is not the original's).
// FUNCTION: CMR2 0x00420630
void RallyData_LoadSelectedRouteNodes(void)
{
    DWORD count;
    int start[3];
    int end[3];
    int index;
    int dx;
    int dy;
    int value;
    BYTE *pFile;
    BYTE *pEntry;

    count = 0;
    sprintf(CFrontend::m_stringDest, g_str0x00519704, GameMenu_GetChampionshipTransitionState());
    pFile = (BYTE *)CGenericFileLoader::FindFile(
        (GenericFile *)StageTiming_GetStageFile3(), CFrontend::m_stringDest, 0, &count, 0);
    g_unk0x00538a94 = 0;
    g_routeDirKey[0] = g_routeDirKey[1] = g_routeDirKey[2] = -1;
    g_routeDirCount = 0;
    if (pFile == NULL) {
        g_routeNodes = NULL;
        g_routeNodeCount = 0;
        g_unk0x00538a88 = 0;
    } else {
        g_routeNodes = (RallyRouteNode *)pFile;
        g_unk0x00538a88 = count / sizeof(*g_routeNodes);
        g_routeNodeCount = g_unk0x00538a88;
        index = g_unk0x00538a88 - 1;
        if (index > 0) {
            pEntry = &g_routeNodes[index].flags;
            do {
                if ((*pEntry & 1) != 0) {
                    g_routeNodeCount = index + 1;
                    break;
                }
                index--;
                pEntry -= sizeof(*g_routeNodes);
            } while (index > 0);
        }
        RallyData_GetRouteNodeGroundPosition(0, start);
        RallyData_GetRouteNodeGroundPosition(g_routeNodeCount - 1, end);
        dy = end[2] - start[2];
        dx = end[0] - start[0];
        if (dx < 0)
            dx = -dx;
        value = g_routeNodeCount;
        if (dx < 0x320000) {
            if (dy < 0)
                dy = -dy;
            if (dy < 0x320000) {
                g_unk0x00538a94 = 1;
                value = (RallyData_GetSelectionBits16To19() & 0xff) * g_routeNodeCount;
            }
        }
        g_unk0x00538c94 = value << 16;
    }
    for (index = 0; index < 8; index++) {
        g_raceRecords[index].routeNodeIndex = 0;
        g_raceRecords[index].resetRouteNodeIndex = 0;
        g_raceRecords[index].field_0x8 = 0;
        g_raceRecords[index].field_0xc = 0;
    }
    RallyData_ResetPlayerRouteProbes();
    CGame::RegisterCallback((void *)StageTiming_NoOpResourceRelease, 0);
}
