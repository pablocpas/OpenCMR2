#ifndef _STAGE_TIMING_H
#define _STAGE_TIMING_H
#if defined(_MSC_VER) && _MSC_VER <= 1200
#ifndef CMR2_LAYOUT_CHECK
#define CMR2_LAYOUT_CHECK(name, condition) typedef char name[condition ? 1 : -1]
#endif
#else
#include "LayoutChecks.h"
#endif

struct PartState;
struct CarSceneRecord;
struct CarPartSet;

// Four moving-part slots, each owning one PartState array indexed by car.
// The mode bytes at +0x10 are used by the corresponding object transforms.
struct CarPartStateTables {
    PartState *parts[4];
    BYTE modes[4];
};
CMR2_LAYOUT_CHECK(CarPartStateTablesSize, sizeof(CarPartStateTables) == 0x14);
extern CarPartStateTables g_carPartStateTables;

struct ReplayLevelState {
    int levels[16];
    int bufferCount;
    int pending[8];
};
CMR2_LAYOUT_CHECK(ReplayLevelSizeCheck, sizeof(ReplayLevelState) == 0x64);
extern ReplayLevelState g_replayLevelState;

int StageTiming_GetDriverIDForPosition(int positionIx);
int StageTiming_GetTimeForPosition(int iPosition);

struct StageFile {
    void *buffer;
    unsigned int size;
    BOOL loaded;
    int field_0xc;
};

void StageTiming_FreeStageFile5(void);
void StageTiming_FreeStageFile2(void);
StageFile *StageTiming_GetStageFile0(void);
StageFile *StageTiming_GetStageFile1(void);
StageFile *StageTiming_GetStageFile2(void);
StageFile *StageTiming_GetStageFile3(void);
StageFile *StageTiming_GetStageFile4(void);
StageFile *StageTiming_GetStageFile5(void);
StageFile *StageTiming_GetStageFile6(void);
BYTE *StageTiming_GetStageArchiveState(void);
void StageTiming_ClearSplitDisplayFlags(void);
int StageTiming_GetSplitSecondaryEntry(int iSplit);
int StageTiming_GetSplitTableEntry(int iSplit, int iIndex);
BYTE StageTiming_GetSplitDisplayState(void);
int StageTiming_GetSplitDriverCount(int iSplit);
int StageTiming_GetSplitPositionOfDriver(int iDriver, int iSplit);
int StageTiming_GetCurrentSplitPositionOfDriver(int iDriver);
int StageTiming_GetSplitDriverIDForPosition(int iPosition, int iSplit);
void StageTiming_AddToOverall(void);
int StageTiming_GetSplitTimeForPosition(int iPosition, int iSplit);
int StageTiming_GetCurrentSplitTimeForDriver(int iDriver);
int StageTiming_GetDriverSlot(int iDriver);
void StageTiming_GetSplitTimesForPositions(int iPosition1, int iPosition2, int *piTime1, int *piTime2);
void StageTiming_Reset(void);
void StageTiming_RebuildSplitPositions(void);

CarSceneRecord *StageTiming_GetStartTableRecord(int index);
void StageDeform_ApplyRadialDent(void);
void StageDeform_ApplyPlanarDent(void);
int StageTiming_FindModelPartByNodeType(unsigned int type, CarPartSet *pModel);
CarPartSet *StageTiming_GetCarReplayRecord(int index);
CarPartSet *CarPart_GetSet(int carIndex);

#include "StageWeatherParticle.h"

#endif
