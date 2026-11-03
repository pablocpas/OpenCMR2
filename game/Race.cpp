#include "SpeedFov.h"
#include "SpeedLookPolicy.h"
#include "render/speed_streak.h"
#include "EffectAmounts.h"
#include "France1Textures.h"
#include "DustPlume.h"
#include "AirMote.h"
#include "FogDynamics.h"
#include "CurvedRoad.h"
#include "SnowMaterial.h"
#include "SceneryPack.h"
#include "SceneryPackFormat.h"
#include <stddef.h>
#include "FloatToInt.h"
#include <windows.h>
#include "Game.h"
#include "port/diagnostics.h"
#include "RallyData.h"
#include "GameInfo.h"
#include "StageUI.h"
#include "Sprite.h"
#include "RaceCallRecord.h"
#include "FixedPoint.h"
#include "ReplayStream.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "InstallInfo.h"
#include "Frontend.h"
#include "Graphics.h"
#include "GraphicsOptions.h"
#include "StageTiming.h"
#include "GenericFileLoader.h"
#include "FileBuffer.h"
#include "Input.h"
#include "Menu.h"
#include "main.h"
#include "Sound.h"
#include "NetworkLeaderboards.h"
#include "RegKey.h"

// Race session state (0x41e210-0x420190)

unsigned char RallyDataState(void);
unsigned char RallyData_GetSelectionFlag26(void);
unsigned int RallyData_GetSelectionFlag27(void);

// GLOBAL: CMR2 0x005191a0
BYTE g_unk0x005191a0 = 0xff;

struct RaceSlotState {
    int owner;
    int pending;
    BYTE flags;
    BYTE unused[3];
};
// GLOBAL: CMR2 0x005370a0
RaceSlotState g_raceSlotState[20];

// Assigns an unused race slot and marks its owner for refresh.
// FUNCTION: CMR2 0x00417660
void Race_AssignUnusedSlot(int owner)
{
    int i = 0;
    do {
        if ((g_raceSlotState[i].flags & 2) == 0) {
            g_raceSlotState[i].flags |= 2;
            g_raceSlotState[i].owner = owner;
            g_raceSlotState[i].flags &= 0xfe;
            g_raceSlotState[i].pending = -1;
            i = 20;
        }
        i++;
    } while (i < 20);
}

int Car_IsStateC10Set(int index);
int Sound_IsPlaying(unsigned int handle);
int Sound_PlaySampleWithParameters(unsigned short id, int volume, int frequency, int loopStart, int loops, int is3D);
extern int g_coDriverCallState;

// Plays the queued co-driver calls one after another: starts the first slot's
// sample, and when it has finished moves the queue up.
// FUNCTION: CMR2 0x004176b0
void RaceCalls_UpdatePlaybackQueue(void)
{
    if ((g_raceSlotState[0].flags & 2) != 0 && Car_IsStateC10Set(0) == 0) {
        Car_IsStateC10Set(0);
        if ((g_raceSlotState[0].flags & 1) != 0) {
            if (Sound_IsPlaying(g_raceSlotState[0].pending) == 0) {
                for (int i = 1; i < 20; i++) {
                    g_raceSlotState[i - 1] = g_raceSlotState[i];
                }
                g_raceSlotState[19].flags &= 0xfc;
                g_raceSlotState[19].pending = -1;
                g_raceSlotState[19].owner = -1;
            }
        } else {
            g_raceSlotState[0].pending =
                Sound_PlaySampleWithParameters((unsigned short)g_raceSlotState[0].owner, g_coDriverCallState, 0x2b11, 0, 0, 0);
            g_raceSlotState[0].flags |= 1;
        }
    }
}
// GLOBAL: CMR2 0x00537f08
BYTE g_unk0x00537f08;
// GLOBAL: CMR2 0x00537190
int g_unk0x00537190;
// GLOBAL: CMR2 0x00537194
int g_coDriverCallState;
// GLOBAL: CMR2 0x00537394
int g_unk0x00537394;
typedef char RaceTransitionAddressWidthCheck[sizeof(ptrdiff_t) >= sizeof(void *) ? 1 : -1];
// 64BIT: slots 0..4 hold bounded numeric states/cursors; slot 5 holds the
// callback machine saved by Race_EnterCurrentModeScene, read by GameMenus.
// ptrdiff_t preserves the original MSVC6 int type and native pointer width.
// GLOBAL: CMR2 0x00537f0c
ptrdiff_t g_unk0x00537f0c[6];
// GLOBAL: CMR2 0x00537f24
int g_unk0x00537f24;
// GLOBAL: CMR2 0x00537f3c
BYTE *g_playerReplayStreams[8];
// GLOBAL: CMR2 0x00537f60
int g_unk0x00537f60;
// GLOBAL: CMR2 0x00537f68
int g_playerEventAwardFlags[4];
// GLOBAL: CMR2 0x00537f78
int g_unk0x00537f78[7];
// The frame callback flag is the sixth dword of the same race state block.
// GLOBAL: CMR2 0x00537f94
int g_unk0x00537f94;
// GLOBAL: CMR2 0x00537ffa
BYTE g_unk0x00537ffa;
// GLOBAL: CMR2 0x00537fc0
unsigned int g_unk0x00537fc0;
// GLOBAL: CMR2 0x00538108
int g_unk0x00538108;
// GLOBAL: CMR2 0x0053810c
BYTE g_unk0x0053810c;
// GLOBAL: CMR2 0x0053810d
BYTE g_unk0x0053810d;
// GLOBAL: CMR2 0x00538114
int g_unk0x00538114;
// GLOBAL: CMR2 0x00538118
int g_unk0x00538118;
// GLOBAL: CMR2 0x0053811c
BYTE g_unk0x0053811c;
// GLOBAL: CMR2 0x00537f00
int g_unk0x00537f00;
// GLOBAL: CMR2 0x00537f34
int g_unk0x00537f34[2];
// GLOBAL: CMR2 0x00537f98
int g_unk0x00537f98[8];
// GLOBAL: CMR2 0x00537fbc
BYTE g_unk0x00537fbc[2];
// GLOBAL: CMR2 0x00537fcc
int g_unk0x00537fcc;
// GLOBAL: CMR2 0x00537ff8
BYTE g_unk0x00537ff8[2];
// GLOBAL: CMR2 0x00538128
int g_unk0x00538128;
// GLOBAL: CMR2 0x0053823c
char g_unk0x0053823c[MAX_PATH];
// GLOBAL: CMR2 0x00538340
char g_unk0x00538340[MAX_PATH];
// GLOBAL: CMR2 0x00538444
char g_unk0x00538444[MAX_PATH];
// GLOBAL: CMR2 0x0053874c
char g_unk0x0053874c[MAX_PATH];
// GLOBAL: CMR2 0x00538850
int g_unk0x00538850;
// GLOBAL: CMR2 0x00538858
GenericFile g_raceFile;
// GLOBAL: CMR2 0x00538864
BYTE g_raceFileCallbackSet;
// GLOBAL: CMR2 0x00538970
int g_raceResourceState;

// GLOBAL: CMR2 0x00538110
int g_unk0x00538110;
// GLOBAL: CMR2 0x005192d4
char g_str0x005192d4[] = "%sL.rpl";
// GLOBAL: CMR2 0x005192dc
char g_str0x005192dc[] = "%sH.rpl";
// GLOBAL: CMR2 0x005192b0
char g_strGrp0x005192b0[] = "%s.grp";
// GLOBAL: CMR2 0x00519228
int g_unk0x00519228 = -1;
// GLOBAL: CMR2 0x0051922c
int g_unk0x0051922c = -1;
// GLOBAL: CMR2 0x0051948c
char g_strTempC3D[] = "TEMP.C3D";

SceneNode *RallyData_GetChallengeRenderState(void);
struct SceneNode *Sector_BuildC3DModelScene(void *data, struct SceneNode *parent, struct GenericFile *archive);
BYTE *Replay_LoadValidatedBuffer(char *path);
char *Race_GetStagePathBuffer3874C(void);
GenericFile *Race_GetLoadedStageFile(void);

// Country codes and difficulty letters of the CPU run file names; the second
// pair of tables is the one Replay_InitRaceSlots uses for the second player.
// GLOBAL: CMR2 0x00519254
char g_str0x00519254[4] = "e";
// GLOBAL: CMR2 0x00519258
char g_str0x00519258[4] = "i";
// GLOBAL: CMR2 0x0051925c
char g_str0x0051925c[4] = "n";
extern char g_str0x00519260[4];
extern char g_str0x00519268[4];
extern char g_str0x0051926c[4];
extern char g_str0x00519270[4];
extern char g_str0x00519274[4];
extern char g_str0x00519278[4];
extern char g_str0x0051927c[4];
extern char g_str0x00519280[4];
// GLOBAL: CMR2 0x005191c8
char *g_cpuRunCountries[9] = { g_str0x00519280, g_str0x0051927c, g_str0x00519278, g_str0x00519274, g_str0x00519270,
                               g_str0x0051926c, g_str0x00519268, CFrontend::m_strUK, g_str0x00519260 };
// GLOBAL: CMR2 0x005191ec
char *g_cpuRunLevels[3] = { g_str0x0051925c, g_str0x00519258, g_str0x00519254 };
// GLOBAL: CMR2 0x005191f8
char *g_cpuRunCountries2[9] = { g_str0x00519280, g_str0x0051927c, g_str0x00519278, g_str0x00519274, g_str0x00519270,
                                g_str0x0051926c, g_str0x00519268, CFrontend::m_strUK, g_str0x00519260 };
// GLOBAL: CMR2 0x0051921c
char *g_cpuRunLevels2[3] = { g_str0x0051925c, g_str0x00519258, g_str0x00519254 };
// GLOBAL: CMR2 0x00519294
char g_strCpuRun[] = "%s\\CPU_Runs\\%s%d%d%s.rpl";
// GLOBAL: CMR2 0x005192b8
char g_strChampEndRun[] = "%s\\CPU_Runs\\champend.rpl";

int Stage_GetPrimaryStartDriver(void);
BYTE *Replay_AllocateStreamBuffer(short frames, short samples, int type);
void Replay_LoadPathOrDefaultBuffer(char *path);
char *Race_GetStagePathBuffer3823C(void);
char *Race_GetStagePathBuffer38444(void);
unsigned int NetPlayers_IsPlayerPresent(int);
BYTE Car_GetDrawnFlag(int index);
int RallyData_IsChampionshipFinalStage(void);
unsigned char RallyData_GetSelectionFlag28(void);

// Loads the CPU run a player races against: the file is chosen by country,
// stage, side of the stage and difficulty. Returns whether it was found.
// FUNCTION: CMR2 0x0041c1b0
int Replay_LoadCpuOpponentRun(int player)
{
    if (player == 0)
        g_unk0x00537f78[6] = Stage_GetPrimaryStartDriver() != 0;
    else
        g_unk0x00537f78[6] = Stage_GetPrimaryStartDriver() == 0;
    sprintf(CFrontend::m_stringDest, g_strCpuRun, Race_GetStagePathBuffer3823C(), g_cpuRunCountries[(BYTE)RallyDataCountryIndex()],
            (BYTE)RallyDataStageIndex() + 1, g_unk0x00537f78[6],
            g_cpuRunLevels[CGameInfo::GetConfiguredGameMode() != 4 ? CGameInfo::GetConfiguredDifficulty()
                                                           : CGameInfo::GetGameModeOptionBits20To22() - 1]);
    g_playerReplayStreams[player] = Replay_LoadValidatedBuffer(CFrontend::m_stringDest);
    return g_playerReplayStreams[player] != NULL;
}

// Sets up the replay buffers of a race: recording buffers for network games,
// the championship-end run, the CPU runs of a head-to-head or knockout stage,
// and a recording buffer for every player that has no run loaded.
// FUNCTION: CMR2 0x0041c260
void Replay_InitRaceSlots(void)
{
    int headToHead;
    int loaded[2];
    int count;
    int i;
    int flag;

    count = RallyDataState() & 0xff;
    loaded[0] = 0;
    loaded[1] = 0;
    if (CGameInfo::GetConfiguredGameMode() == 4 || (char)RallyData_GetFlag25())
        headToHead = 1;
    else
        headToHead = 0;
    for (i = 0; i < 8; i++)
        g_playerReplayStreams[i] = NULL;
    if (CGameInfo::GetGameModeOptionBit19()) {
        g_playerReplayStreams[0] = Replay_AllocateStreamBuffer(1, 0x1d4c, 2);
        for (i = 0; i < 7; i++) {
            g_playerReplayStreams[i + 1] = NULL;
            if ((char)NetPlayers_IsPlayerPresent(i) && CGameInfo::GetConfiguredGameMode() != 10)
                g_playerReplayStreams[i + 1] = Replay_AllocateStreamBuffer(1, 0x1d4c, 2);
        }
        return;
    }
    if ((char)RallyData_IsChampionshipFinalStage()) {
        sprintf(CFrontend::m_stringDest, g_strChampEndRun, Race_GetStagePathBuffer3823C());
        g_playerReplayStreams[0] = Replay_LoadValidatedBuffer(CFrontend::m_stringDest);
        return;
    }
    if ((char)RallyData_GetSelectionFlag28() == 0) {
        for (i = 0; i < 2; i++) {
            if ((char)RallyData_GetFlag25() && CGameInfo::GetGameModeOptionBit19() == 0 && CGameInfo::IsConfiguredMultiplayer()) {
                if (i == 0) {
                    g_playerReplayStreams[0] = Replay_AllocateStreamBuffer(1, 0x186a, 0);
                } else {
                    flag = Stage_GetPrimaryStartDriver() == 0;
                    sprintf(CFrontend::m_stringDest, g_strCpuRun, Race_GetStagePathBuffer3823C(),
                            g_cpuRunCountries2[(BYTE)RallyDataCountryIndex()], (BYTE)RallyDataStageIndex() + 1,
                            flag,
                            g_cpuRunLevels2[CGameInfo::GetConfiguredGameMode() != 4 ? CGameInfo::GetConfiguredDifficulty()
                                                                           : CGameInfo::GetGameModeOptionBits20To22() - 1]);
                    g_playerReplayStreams[1] = Replay_LoadValidatedBuffer(CFrontend::m_stringDest);
                    if (i == 1)
                        return;
                }
            }
        }
        for (i = 0; i < 2; i++) {
            if (((CGameInfo::GetConfiguredGameMode() == 4 && (char)Car_GetDrawnFlag(i) == -1) ||
                 ((char)RallyData_GetFlag25() && CGameInfo::GetGameModeOptionBit19() == 0 &&
                  RallyData_GetUsableRecordCategory((BYTE)i) != -1)) &&
                Replay_LoadCpuOpponentRun(i) != 0)
                loaded[i] = 1;
        }
    }
    for (i = 0; i < count; i++) {
        if (loaded[i] != 0)
            continue;
        if ((char)RallyData_GetSelectionFlag28()) {
            if (count == 1)
                g_playerReplayStreams[i] = Replay_AllocateStreamBuffer(1, 0x1d4c, 0);
            else
                g_playerReplayStreams[i] = Replay_AllocateStreamBuffer(1, 0x186a, 0);
        } else if (count == 1 && headToHead == 0) {
            g_playerReplayStreams[i] = Replay_AllocateStreamBuffer(1, 0x1d4c, 0);
        } else {
            g_playerReplayStreams[i] = Replay_AllocateStreamBuffer(1, 0x186a, 0);
        }
    }
    if ((char)RallyData_GetSelectionFlag28() && (char)CGameInfo::GetSoundOptionBit30()) {
        sprintf(CFrontend::m_stringDest, g_strGrp0x005192b0, Race_GetStagePathBuffer38444());
        Replay_LoadPathOrDefaultBuffer(CFrontend::m_stringDest);
    }
}

// Clears the player replay slots, then builds the replay file name for the
// current game mode and loads it into slot 0.
// FUNCTION: CMR2 0x0041c510
void Replay_LoadSelectedRun(void)
{
    int i;

    for (i = 0; i < 8; i++)
        g_playerReplayStreams[i] = 0;
    if (CGameInfo::GetGraphicsOptionBits27To28() == 0) {
        sprintf(CFrontend::m_stringDest, g_str0x005192dc, Race_GetStagePathBuffer3874C());
    } else {
        sprintf(CFrontend::m_stringDest, g_str0x005192d4, Race_GetStagePathBuffer3874C());
    }
    if (CGameInfo::GetConfiguredGameMode() != 5 && CGameInfo::GetConfiguredGameMode() != 6 &&
        CGameInfo::GetConfiguredGameMode() != 7 && CGameInfo::GetConfiguredGameMode() != 0xb &&
        CGameInfo::GetConfiguredGameMode() != 0xc) {
        g_playerReplayStreams[0] = (BYTE *)Replay_LoadValidatedBuffer(CFrontend::m_stringDest);
        g_unk0x00538110 = 1;
    }
}

// Loads the stage's TEMP.C3D model into memory.
// FUNCTION: CMR2 0x0041fc50
void Race_LoadStageModel(void)
{
    void *pBuffer;
    GenericFile *pFile;

    pBuffer = CGenericFileLoader::FindFile((GenericFile *)StageTiming_GetStageFile3(), g_strTempC3D, NULL, NULL, 0);
    if (CurvedRoad_Render()) pBuffer=CurvedRoad_Render();
    if (pBuffer != NULL) {
        SceneNode *root;
        pFile = Race_GetLoadedStageFile();
        root = Sector_BuildC3DModelScene(pBuffer, RallyData_GetChallengeRenderState(), pFile);
        SnowMaterial_SetStageScene(root);   // video.snow_material: terrain snow tagging walks this scene
    }
}


// FUNCTION: CMR2 0x0041e210
void Race_SetFlag37F08(void)
{
    g_unk0x00537f08 = 1;
}

// FUNCTION: CMR2 0x0041f250
void Race_ResetStateByteToFF(void)
{
    g_unk0x005191a0 = 0xff;
}

// FUNCTION: CMR2 0x0041f270
int Race_ReadState37F60(void)
{
    return g_unk0x00537f60;
}

// FUNCTION: CMR2 0x0041f280
void Race_SetFlag3810C(void)
{
    g_unk0x0053810c = 1;
}

// FUNCTION: CMR2 0x0041f290
void Race_SetFlag37FFA(void)
{
    g_unk0x00537ffa = 1;
}

// FUNCTION: CMR2 0x0041f2a0
void Race_SetFlag3810D(void)
{
    g_unk0x0053810d = 1;
}

// FUNCTION: CMR2 0x0041f350
ReplayStream *Race_GetPlayerRecordPointer(int index)
{
    return (ReplayStream *)g_playerReplayStreams[index];
}

// FUNCTION: CMR2 0x0041f360
int Race_ConsumeTransitionLatch(void)
{
    if (g_unk0x00538114 != 0) {
        g_unk0x00538114 = 0;
        return 1;
    }
    return 0;
}

// FUNCTION: CMR2 0x0041f380
BYTE Race_GetStateByte(void)
{
    return g_unk0x005191a0;
}

// FUNCTION: CMR2 0x0041f390
void Race_ClearState38118(void)
{
    g_unk0x00538118 = 0;
}

// FUNCTION: CMR2 0x0041f3a0
int Race_IsMultiplayerRecordMode10(void)
{
    int result = 0;

    if ((BYTE)RallyDataState() > 1 && (char)((CallbackStateMachine *)StageUI_GetRaceResultTable())->records[0].packedState == 10)
        result = 1;
    return result;
}

// FUNCTION: CMR2 0x0041f3d0
int Race_GetPlayerRecordField4(BYTE index)
{
    if (g_playerReplayStreams[index] != NULL)
        return ((ReplayStream *)g_playerReplayStreams[index])->playing;
    return 0;
}

// FUNCTION: CMR2 0x0041f3f0
int Race_GetPlayerRecordFieldC(BYTE index)
{
    if (g_playerReplayStreams[index] != NULL)
        return ((ReplayStream *)g_playerReplayStreams[index])->recording;
    return 0;
}

// FUNCTION: CMR2 0x0041f410
int Race_AcceptStateTransition(void)
{
    return 1;
}

// FUNCTION: CMR2 0x0041f4b0
int Race_GetFlag38108(void)
{
    return g_unk0x00538108;
}

// FUNCTION: CMR2 0x0041f4c0
void Race_SetFlag38108(void)
{
    g_unk0x00538108 = 1;
}

// FUNCTION: CMR2 0x0041f4d0
void Race_SetFlag37F94(void)
{
    g_unk0x00537f94 = 1;
}

BYTE RallyData_FreeChallengeSceneObjects(void);
void Race_ReleaseFrameResources(void);
void StageUI_ClearRaceEndLatch(void);

// Update handler of game state 12 (state table 0x5190b0).
// FUNCTION: CMR2 0x0041f4e0
void Race_UpdateState12(CallbackStateMachine *p, BYTE index)
{
    CGame::SetFrontendResourceMode(0);
    Race_ReleaseFrameResources();
    RallyData_FreeChallengeSceneObjects();
    StageUI_ClearRaceEndLatch();
}

// FUNCTION: CMR2 0x0041f500
GenericFile *Race_GetLoadedStageFile(void)
{
    return g_raceFile.didFileLoad ? &g_raceFile : NULL;
}

void StageObject_FreeAll(void);

// Releases the race file loaded by Race_LoadSelectedStage (registered callback).
// FUNCTION: CMR2 0x0041f510
BYTE Race_ReleaseStageFile(void)
{
    if (g_raceFile.didFileLoad && g_raceFile.buffer) {
        CFileBuffer::FreeGenericFileBuffer(g_raceFile.buffer);
        g_raceFile.buffer = NULL;
    }
    g_raceFile.didFileLoad = FALSE;
    g_raceFile.fileSize = 0;
    if (g_unk0x00538850)
        StageObject_FreeAll();
    g_raceFileCallbackSet = 0;
    return 1;
}

extern char g_str0x00519348[6];
extern char g_str0x00519350[6];
extern char g_str0x00519358[7];
extern char g_str0x00519360[7];
extern char g_str0x00519368[7];
extern char g_str0x00519370[8];
extern char g_strPathFormat[];

// Long country names (FINLAND..JAPAN) and codes (FIN..JAP) of the rally
// selection file names.
// GLOBAL: CMR2 0x005192f8
char *g_countryNames[9] = { g_str0x00519370, g_str0x00519368, g_str0x00519360, g_str0x00519358,
                             g_str0x00519270, g_str0x00519350, g_str0x00519348, CFrontend::m_strUK,
                             CRegKey::m_skuJapan };
// GLOBAL: CMR2 0x0051931c
char *g_unk0x0051931c[9] = { g_str0x00519280, g_str0x0051927c, g_str0x00519278, g_str0x00519274,
                             g_str0x00519270, g_str0x0051926c, g_str0x00519268, CFrontend::m_strUK,
                             g_str0x00519260 };

// GLOBAL: CMR2 0x00519378
char g_str0x00519378[] = "%s\\%s\\%s%st.bfl";
// GLOBAL: CMR2 0x00519388
char g_str0x00519388[] = "%s\\replays\\%s-chL";
// GLOBAL: CMR2 0x0051939c
char g_str0x0051939c[] = "%s\\replays\\%s-chH";
// GLOBAL: CMR2 0x005193b0
char g_str0x005193b0[] = "%s\\Game\\DemoRuns\\%s-ch";
// GLOBAL: CMR2 0x005193c8
char g_str0x005193c8[] = "%s\\%s-ch";
// GLOBAL: CMR2 0x005193d4
char g_str0x005193d4[] = "%s\\cha%.2d%s";
// GLOBAL: CMR2 0x005193e4
char g_str0x005193e4[] = "%s\\chall";
// GLOBAL: CMR2 0x005193f0
char g_str0x005193f0[] = "UKi";
// GLOBAL: CMR2 0x005193f4
char g_str0x005193f4[] = "%s\\%s%st.bfl";
// GLOBAL: CMR2 0x00519404
char g_str0x00519404[] = "%s\\replays\\%s%.2dL";
// GLOBAL: CMR2 0x00519418
char g_str0x00519418[] = "%s\\replays\\%s%.2dH";
// GLOBAL: CMR2 0x0051942c
char g_str0x0051942c[] = "%s\\Game\\DemoRuns\\%s%.2d";
// GLOBAL: CMR2 0x00519444
char g_str0x00519444[] = "%s\\%s%.2d";
// GLOBAL: CMR2 0x00519450
char g_str0x00519450[] = "%s\\%s%.2d%s";
// GLOBAL: CMR2 0x00519464
char g_str0x00519464[4] = "lo";
// GLOBAL: CMR2 0x00519468
char g_str0x00519468[4] = "hi";
extern BYTE g_unk0x00538130[0x40];
// GLOBAL: CMR2 0x00538548
char g_unk0x00538548[MAX_PATH];

// Builds the file names of the current rally selection (the challenge names
// when the game info flags are 5..0xc, the short "hi"/"lo" variants otherwise).
// FUNCTION: CMR2 0x0041f560
void Race_BuildSelectionPaths(void)
{
    char buf[4];
    char name[4];
    int table[8] = { 6, 3, 1, 4, 0, 2, 5, 7 };

    sprintf(buf, "%s", CMain::m_logFileBlankLine);
    if (CGameInfo::GetGraphicsOptionBits27To28() == 0) {
        sprintf(buf, "%s", g_str0x00519468);
    } else {
        sprintf(buf, "%s", g_str0x00519464);
    }
    if (CGameInfo::GetConfiguredGameMode() == 5 || CGameInfo::GetConfiguredGameMode() == 6 ||
        CGameInfo::GetConfiguredGameMode() == 7 || CGameInfo::GetConfiguredGameMode() == 0xb ||
        CGameInfo::GetConfiguredGameMode() == 0xc) {
        int idx = (BYTE)RallyData_GetSelectionBits10To11() * 3 + (BYTE)RallyData_GetSelectionBits12To13();

        strcpy(name, g_unk0x0051931c[table[idx]]);
        if (idx == 7)
            strcpy(name, g_str0x005193f0);
        sprintf(g_unk0x0053823c, g_str0x005193e4, CInstallInfo::GetTracksDir());
        sprintf(g_unk0x00538340, g_str0x005193d4, g_unk0x0053823c, idx + 1, buf);
        sprintf((char *)g_unk0x00538130, g_str0x005193c8, g_unk0x0053823c, name);
        sprintf(g_unk0x0053874c, g_str0x005193b0, CInstallInfo::GetGameHDPath(),
                g_unk0x0051931c[table[idx]]);
        if (CGameInfo::GetGraphicsOptionBits27To28() == 0)
            sprintf(g_unk0x00538444, g_str0x0051939c, CInstallInfo::GetGameHDPath(),
                    g_unk0x0051931c[table[idx]]);
        else
            sprintf(g_unk0x00538444, g_str0x00519388, CInstallInfo::GetGameHDPath(),
                    g_unk0x0051931c[table[idx]]);
        sprintf(g_unk0x00538548, g_str0x00519378, CInstallInfo::GetTracksDir(),
                g_countryNames[table[idx]], g_unk0x0051931c[table[idx]], buf);
        return;
    }
    sprintf(g_unk0x0053823c, g_strPathFormat, CInstallInfo::GetTracksDir(),
            g_countryNames[(BYTE)RallyDataCountryIndex()]);
    sprintf(g_unk0x00538340, g_str0x00519450, g_unk0x0053823c,
            g_unk0x0051931c[(BYTE)RallyDataCountryIndex()], (BYTE)RallyDataStageIndex() + 1, buf);
    sprintf((char *)g_unk0x00538130, g_str0x00519444, g_unk0x0053823c,
            g_unk0x0051931c[(BYTE)RallyDataCountryIndex()], (BYTE)RallyDataStageIndex() + 1);
    sprintf(g_unk0x0053874c, g_str0x0051942c, CInstallInfo::GetGameHDPath(),
            g_unk0x0051931c[(BYTE)RallyDataCountryIndex()], (BYTE)RallyDataStageIndex() + 1);
    if (CGameInfo::GetGraphicsOptionBits27To28() == 0)
        sprintf(g_unk0x00538444, g_str0x00519418, CInstallInfo::GetGameHDPath(),
                g_unk0x0051931c[(BYTE)RallyDataCountryIndex()], (BYTE)RallyDataStageIndex() + 1);
    else
        sprintf(g_unk0x00538444, g_str0x00519404, CInstallInfo::GetGameHDPath(),
                g_unk0x0051931c[(BYTE)RallyDataCountryIndex()], (BYTE)RallyDataStageIndex() + 1);
    sprintf(g_unk0x00538548, g_str0x005193f4, g_unk0x0053823c,
            g_unk0x0051931c[(BYTE)RallyDataCountryIndex()], buf);
}

// FUNCTION: CMR2 0x0041f8e0
char *Race_GetStagePathBuffer3823C(void)
{
    return g_unk0x0053823c;
}

// FUNCTION: CMR2 0x0041f8f0
char *Race_GetStagePathBuffer38340(void)
{
    return g_unk0x00538340;
}

// FUNCTION: CMR2 0x0041f910
char *Race_GetStagePathBuffer3874C(void)
{
    return g_unk0x0053874c;
}

// FUNCTION: CMR2 0x0041f920
char *Race_GetStagePathBuffer38444(void)
{
    return g_unk0x00538444;
}

void RallyData_LoadChallengeScreenTextures(void);

BYTE *GameMenu_GetChampionshipTransitionState(void);
int Graphics_ReserveCubeMapsAndLoadEnvironment(char *name, int count, GenericFile *pFile, DWORD size);
void StageTiming_InitRaceDriverRecords(char);
void Scene_InitLighting(int *pData, int *pHeights);
#include "StageWeather.h"
void StageObject_SetLighting(const StageLightPreset *pPrimary, const StageLightPreset *pSecondary);
void RallyData_DrawLoadingProgress(int progress, char drawScene, BYTE alpha);
void Race_ResolveStageTexturesAndPalettes(void);
void Race_LoadStageLightPlacement(void);
void Race_LoadStageModel(void);
void Race_LoadHPCData(void);
void Race_LoadSurfaceList(void);
void TrackCollision_LoadFinishRecords(void);
void StageObject_LoadModelVariantsAndSaveCounts(void);
void StageTiming_LoadCspData(void);
void RallyData_LoadSelectedRouteNodes(void);
void Race_LoadCoDriverCallData(void);
StageFile *StageTiming_GetStageFile3(void);

int StageObject_SelectNonemptyRecordList(int *pList);
void Sector_RebuildStageGrid(void);
void StageTiming_RebuildSplitsAndCheckpoints(void);
void RallyData_ResetStageClockAndRecords(int keepName);
void RallyData_LayoutPlayerSplitBars(void);
void Frontend_LoadSplitTimeMirror(void);
void Replay_InitRaceSlots(void);
void StageWeather_SetupSettingTransition(void);
void StageTiming_LoadSelectedTeamArchive(void);
BYTE *AI_LoadStageRouteTables(void);
void StageObject_ForwardSessionUpdate(void);
void RallyData_RebuildSectorStaticObjectTables(void);
void NetRace_ResetPlayerFadeStates(void);
extern char g_tgaSuffix[];
extern StageLightPreset *g_stageLightPresetSecondary;
extern StageLightPreset *g_stageLightPresetPrimary;

// GLOBAL: CMR2 0x0051946c
char g_str0x0051946c[] = "%s.xhi";
// GLOBAL: CMR2 0x00519474
char g_str0x00519474[] = "%s.tre";
// GLOBAL: CMR2 0x0051947c
char g_str0x0051947c[] = "%s.tsc";

// Loads the race files of the current selection (the .tsc track, the .tre and
// .xhi lighting files and the .tga of the selection) and initialises the stage.
// FUNCTION: CMR2 0x0041f930
BYTE Race_LoadSelectedStage(void)
{
    BYTE *pTsc;
    BYTE *pTre;
    BYTE *pXhi;

    if (g_raceFileCallbackSet == 0) {
        CGame::RegisterCallback(Race_ReleaseStageFile, 0);
        g_raceFileCallbackSet = 1;
    }
    CGenericFileLoader::LoadIntoFileRecord((GenericFile *)&g_raceFile, g_unk0x00538548);
    RallyData_LoadChallengeScreenTextures();
    sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue, Race_GetStagePathBuffer38340());
    CFrontend::m_stringDest[strlen(CFrontend::m_stringDest) - 4] =
        CFrontend::m_stringDest[strlen(CFrontend::m_stringDest) - 2];
    CFrontend::m_stringDest[strlen(CFrontend::m_stringDest) - 3] =
        CFrontend::m_stringDest[strlen(CFrontend::m_stringDest) - 1];
    CFrontend::m_stringDest[strlen(CFrontend::m_stringDest) - 2] = '\0';
    // 0x41fa0e decrements the post-SCAS pointer to the NUL itself,
    // appending the suffix without deleting the final hi/lo character.
    strcat(CFrontend::m_stringDest, g_tgaSuffix);
    if ((char)RallyData_GetFlag24()) {
        sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue, g_unk0x00538548);
        strcpy(CFrontend::m_stringDest + strlen(CFrontend::m_stringDest) - 5, g_tgaSuffix);
    }
    if (CGameInfo::IsRecordFlagSet(0x10))
        Graphics_ReserveCubeMapsAndLoadEnvironment(CFrontend::m_stringDest,
                     (BYTE)RallyDataState() + (BYTE)RallyData_GetSecondarySelectionNibble(),
                     Race_GetLoadedStageFile(), 0x80);
    else
        Graphics_ReserveCubeMapsAndLoadEnvironment(CFrontend::m_stringDest, 1, Race_GetLoadedStageFile(), 0x40);
    RallyData_DrawLoadingProgress(0xf, 1, 0xff);
    France1Textures_Begin((GenericFile*)StageTiming_GetStageFile3(),(const char*)GameMenu_GetChampionshipTransitionState());
    Race_ResolveStageTexturesAndPalettes();
    Race_LoadStageLightPlacement();
    CurvedRoad_Begin((GenericFile*)StageTiming_GetStageFile3(),(const char*)GameMenu_GetChampionshipTransitionState());
    Race_LoadStageModel();
    Race_LoadHPCData();
    RallyData_DrawLoadingProgress(0x19, 1, 0xff);
    Race_LoadSurfaceList();
    TrackCollision_LoadFinishRecords();
    StageObject_LoadModelVariantsAndSaveCounts();
    RallyData_DrawLoadingProgress(0x28, 1, 0xff);
    StageTiming_LoadCspData();
    RallyData_LoadSelectedRouteNodes();
    Race_LoadCoDriverCallData();
    sprintf(CFrontend::m_stringDest, g_str0x0051947c, GameMenu_GetChampionshipTransitionState());
    pTsc = (BYTE *)CGenericFileLoader::FindFile((GenericFile *)StageTiming_GetStageFile3(),
                                                CFrontend::m_stringDest, 0, 0, 0);
    StageObject_SelectNonemptyRecordList((int *)pTsc);
    Sector_RebuildStageGrid();
    sprintf(CFrontend::m_stringDest, g_str0x00519474, GameMenu_GetChampionshipTransitionState());
    pTre = (BYTE *)CGenericFileLoader::FindFile((GenericFile *)StageTiming_GetStageFile3(),
                                                CFrontend::m_stringDest, 0, 0, 0);
    sprintf(CFrontend::m_stringDest, g_str0x0051946c, GameMenu_GetChampionshipTransitionState());
    pXhi = (BYTE *)CGenericFileLoader::FindFile((GenericFile *)StageTiming_GetStageFile3(),
                                                CFrontend::m_stringDest, 0, 0, 0);
    if (pTre != 0 && (char)CGameInfo::GetGraphicsOptionBit4() == 0)
        pTre = 0;
    Scene_InitLighting((int *)pTre, (int *)pXhi);
    StageTiming_RebuildSplitsAndCheckpoints();
    RallyData_ResetStageClockAndRecords(0);
    RallyData_LayoutPlayerSplitBars();
    if (CGameInfo::GetConfiguredGameMode() != 4)
        Frontend_LoadSplitTimeMirror();
    if (CGameInfo::GetGameInfoSessionFlag() == 0 && CGameInfo::GetConfiguredGameMode() == 3)
        Replay_InitRaceSlots();
    StageWeather_SetupSettingTransition();
    StageTiming_LoadSelectedTeamArchive();
    StageTiming_InitRaceDriverRecords(1);
    StageObject_ForwardSessionUpdate();
    RallyData_DrawLoadingProgress(0x3c, 1, 0xff);
    StageObject_SetLighting(g_stageLightPresetPrimary, g_stageLightPresetSecondary);
    RallyData_RebuildSectorStaticObjectTables();
    NetRace_ResetPlayerFadeStates();
    return 1;
}

// FUNCTION: CMR2 0x00420120
int Race_GetResourceState(void)
{
    return g_raceResourceState;
}

void Race_SetResourceStateAndCallbackMark(int value);
int Sound_GetSampleCount(void);
void StageTiming_InitStageFileTable(void);
void StageObject_ClearCarStateSlots(void);
int StageTiming_LoadStageCarsAndEffects(void);
void Sector_RebuildNodeLists(void);
void Surface_LoadCarEngineSounds(void);
void StageObject_ResetPairedCarValues(void);

// Loads the stage geometry; on success rebuilds the sector node lists and the
// surface tables. Returns whether the stage loaded.
// FUNCTION: CMR2 0x00420150
BYTE Race_LoadStageGeometry(void)
{
    BYTE ok;

    Race_SetResourceStateAndCallbackMark(Sound_GetSampleCount());
    StageTiming_InitStageFileTable();
    StageObject_ClearCarStateSlots();
    if (StageTiming_LoadStageCarsAndEffects()) {
        Sector_RebuildNodeLists();
        Surface_LoadCarEngineSounds();
        ok = 1;
    } else {
        ok = 0;
    }
    StageObject_ResetPairedCarValues();
    return ok;
}

// Car count selected by the game mode, before ghost/network additions.
// FUNCTION: CMR2 0x00420190
char Race_GetBaseCarCount(void)
{
    if ((char)RallyData_GetSelectionFlag26() && !CGameInfo::GetGameModeOptionBit19())
        return (char)RallyDataState() + (char)RallyData_GetSecondarySelectionNibble();
    if ((char)RallyData_GetSelectionFlag27() && !CGameInfo::GetGameModeOptionBit19())
        return 2;
    return (char)RallyDataState();
}

// FUNCTION: CMR2 0x00414700
int Race_IsRouteModeWithoutFlag18(void)
{
    if (RallyData_IsHeadToHeadRaceMode() && !CGameInfo::IsSplitBarEnabled())
        return 1;
    return 0;
}

// FUNCTION: CMR2 0x004174d0
int Race_IsFlag24Clear(void)
{
    return (char)RallyData_GetFlag24() == 0;
}

// GLOBAL: CMR2 0x0053708c
int g_unk0x0053708c[2];
// GLOBAL: CMR2 0x00537198
int g_unk0x00537198[2];

#include "Car.h"
#include "RallyRouteNode.h"
#include "AttractCapture.h"
int RallyData_GetActiveCarRaceRecordField0(Car *p);

// Stores the player's route position twice and frees the first five race slots.
// Car_ResetToStage also calls it for computer cars (index 2..7 in the arcade
// modes): the original then writes past both two-entry arrays, over the
// co-driver call table pointer, the arrow texture and the call records.
// FUNCTION: CMR2 0x00417780
void Race_StorePlayerRouteAndClearSlots(int player)
{
    int i;

    // PORT: only the two players have these entries. The original writes the
    // computer cars' positions past both arrays, over neighbouring globals
    // that differ here; nothing reads entries past the second.
    if (player < 2) {
        g_unk0x0053708c[player] = RallyData_GetActiveCarRaceRecordField0(Car_Get(player));
        g_unk0x00537198[player] = RallyData_GetActiveCarRaceRecordField0(Car_Get(player));
    }
    for (i = 0; i < 5; i++) {
        g_raceSlotState[i].flags &= 0xfc;
        g_raceSlotState[i].pending = -1;
        g_raceSlotState[i].owner = -1;
    }
}

// GLOBAL: CMR2 0x005371a0
int g_unk0x005371a0;
// GLOBAL: CMR2 0x005371a4
int g_unk0x005371a4[10];

// GLOBAL: CMR2 0x005371d0
RaceCallRecord g_raceCallRecords[10];
// GLOBAL: CMR2 0x00537248
int g_raceWrongWayFlags[2];

// Queues a race call (radio message) of the given call id and player.
extern unsigned int *g_coDriverCalls;
int Race_DecodeCallSlotIcon(unsigned int id, Texture **pTexture, SpriteRect *pRect, unsigned int *pFlag, BYTE *pColour);
// match 69%: the original keeps `player` in memory and a zero constant in ebx
// (register allocation only; logic matches the asm).
// FUNCTION: CMR2 0x004174e0
void Race_QueuePlayerCallout(unsigned int player, BYTE callId, BYTE prevCallId, BYTE unused)
{
    unsigned int id;
    unsigned int type;
    unsigned int b1;
    unsigned int b2;
    unsigned int b3;
    int prev;
    Texture *texture;
    BYTE colour[4];
    unsigned int flag;
    SpriteRect rect;
    int i;
    int typeCall;
    int levelCall;

    texture = 0;
    prev = 0;
    if (Car_IsStateC10Set(player) != 0)
        return;

    if (callId != 0xff) {
        id = g_coDriverCalls[callId];
        if (prevCallId != 0xff)
            prev = g_coDriverCalls[prevCallId];
        type = id >> 0x11 & 0xf;
        b2 = id & 0xf;
        b1 = id >> 4 & 3;
        b3 = id >> 0x15 & 3;
        if ((type == 0xe || type == 2 || type == 1) && b1 != 0)
            typeCall = 1;
        else
            typeCall = 0;
        levelCall = 0;
        if (b3 != 0 && b1 != 0)
            levelCall = 1;
        if (prev == 0) {
            if (typeCall) {
                id -= type * 0x20000;
                prev = type * 0x20000;
            } else if (levelCall) {
                id -= b3 * 0x200000;
                prev = b3 * 0x200000;
            }
        }
    } else {
        id = 0;
        type = player;
        b1 = player;
        b2 = player;
    }

    if (b2 == 6 && b1 == 0 && type == 0)
        return;

    for (i = 0; i < 5; i++) {
        if ((g_raceCallRecords[player * 5 + i].flags & 0x100) == 0) {
            g_raceCallRecords[player * 5 + i].primaryCallId = id;
            g_raceCallRecords[player * 5 + i].flags = (g_raceCallRecords[player * 5 + i].flags & 0xfffffd32) | 0x132;
            if (prev != 0) {
                g_raceCallRecords[player * 5 + i].secondaryCallId = prev;
                if (Race_DecodeCallSlotIcon(prev, &texture, &rect, &flag, colour) != 0)
                    g_raceCallRecords[player * 5 + i].flags = (g_raceCallRecords[player * 5 + i].flags & 0xffffff32) | 0x32;
            } else {
                g_raceCallRecords[player * 5 + i].secondaryCallId = 0;
            }
            i = 5;
        }
    }
}

// Resets the per-player race state: best values, call records and slots.
// match 48%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00416670
void Race_ResetPlayerCallState(void)
{
    int i;
    RaceCallRecord *p;

    for (i = 0; i < 2; i++) {
        g_unk0x0053708c[i] = -1;
        g_raceWrongWayFlags[i] = 0;
        g_unk0x00537198[i] = 9999;
    }
    memset(g_unk0x005371a4, 0, sizeof(g_unk0x005371a4));
    for (p = g_raceCallRecords; p < &g_raceCallRecords[10]; p += 5) {
        for (i = 0; i < 5; i++) {
            p[i].flags &= 0xfffffc00;
            p[i].secondaryCallId = 0;
            p[i].primaryCallId = 0;
        }
        g_unk0x005371a0 = 0;
    }
    for (i = 0; i < 5; i++) {
        g_raceSlotState[i].flags &= 0xfc;
        g_raceSlotState[i].pending = -1;
        g_raceSlotState[i].owner = -1;
    }
}

int InRaceMenu_IsPlayerEditingCarSetup(unsigned int param1);
BYTE *RallyData_GetDriverHudFlags(BYTE index);
extern int g_unk0x00537350;
extern int g_coDriverFirstSample;
void StageUI_DrawStageGrid(int unused, int set);
void Race_AssignSlotsFromCallRecord(unsigned int id);
int Race_DecodeCallSlotIcon(unsigned int id, Texture **pOut, SpriteRect *pRect, unsigned int *pFlag, BYTE *pColour);
void Race_DrawPlayerMessage(char *pText, int *pColour, int player, int param4, int param5, int param6);

// Colour of the race call text (white).
// GLOBAL: CMR2 0x00517e24
int g_raceCallTextColour = -1;

// Draws the five call slots of one player: the icon sprite of each call (the
// countdown wobbles it once the slot is the first one) and the timer text.
// The second parameter is unused (the original still cleans 8 bytes).
// FUNCTION: CMR2 0x004177d0
void Race_DrawPlayerCallSlots(unsigned int player, int *pViewRect)
{
    BYTE colA[4];
    BYTE colB[4];
    Texture *texA;
    Texture *texB;
    unsigned int flagA;
    unsigned int flagB;
    int yOff;
    short curveB;
    int xOff;
    int second;
    int iVar;
    int base;
    int yText;
    int extra;
    short value;
    int found;
    int bVar2;
    int n;
    int r;
    int half;
    SpriteRect rectTex;
    SpriteRect rectIcon;
    SpriteRect rectSecond;
    SpriteRect rectMain;
    RaceCallRecord *rec;
    int *pCallFlag;
    BYTE *pState;
    short curve[11];

    extra = 0;
    colB[0] = 0xff; colB[1] = 0xff; colB[2] = 0xff; colB[3] = 0xff;
    colA[0] = 0xff; colA[1] = 0xff; colA[2] = 0xff; colA[3] = 0xff;
    texB = 0;
    flagA = 0;
    flagB = 0;
    second = 0;
    xOff = 0;
    yOff = 0;
    curve[0] = 0;
    curve[1] = 0xc;
    curve[2] = 0x19;
    curve[3] = 0x4c;
    curve[4] = 0x99;
    curve[5] = 0x100;
    curve[6] = 0x180;
    curve[7] = 0x219;
    curve[8] = 0x2cc;
    curve[9] = 0x399;
    curve[10] = 0x400;

    if ((char)RallyData_GetSelectionFlag26() && g_unk0x00537350 != -1) {
        if (RallyData_IsHeadToHeadRaceMode() != 0) {
            if (player == 1)
                StageUI_DrawStageGrid(1, g_unk0x00537350);
        } else {
            StageUI_DrawStageGrid(player, g_unk0x00537350);
        }
    }

    if (RallyData_IsHeadToHeadRaceMode() == 0) {
        iVar = 0;
    } else {
        iVar = 1;
        if (CGameInfo::IsSplitBarEnabled()) {
            if (player == 1)
                yOff = (int)g_pGraphics->resY / 2;
            yOff += (int)g_pGraphics->resY / 8 + (int)g_pGraphics->resY * 10 / 0x1e0;
            xOff = (int)g_pGraphics->resX * 46 / 0x280 - ((int)g_pGraphics->resX * 29 << 10 >> 16);
        } else {
            if (player == 0)
                xOff = -((int)g_pGraphics->resX / 4);
            else
                xOff = (int)g_pGraphics->resX / 4;
            yOff = (int)g_pGraphics->resY * 9011 >> 16;
        }
    }

    if (iVar == 0) {
        pState = RallyData_GetDriverHudFlags(StageUI_GetRaceEndEventCount() + player);
        if ((*pState & 3) == 1)
            extra = 0x10;
    }
    yText = ((int)g_pGraphics->resY << 12 >> 16) + extra;

    pCallFlag = g_raceWrongWayFlags;
    if (pCallFlag[player] != 0 || InRaceMenu_IsPlayerEditingCarSetup(player) != 0) {
        sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(0x57));
        Race_DrawPlayerMessage(CFrontend::m_stringDest, &g_raceCallTextColour, player, 1, -1, -1);
    }

    if (!Race_IsFlag24Clear())
        return;

    iVar = 0;
    base = player * 5;
    do {
        rec = &g_raceCallRecords[base + iVar];
        if ((rec->flags & 0x100) == 0) {
            if (InRaceMenu_IsPlayerEditingCarSetup(player) == 0)
                goto next;
            if ((char)CGameInfo::IsInRaceMenuOpen() == 0)
                goto next;
        }

        found = Race_DecodeCallSlotIcon((rec->flags & 0x100) ? rec->primaryCallId : 0x14, &texA, &rectIcon, &flagA, colB);
        bVar2 = 0;
        if (rec->secondaryCallId != 0) {
            if (Race_DecodeCallSlotIcon(rec->secondaryCallId, &texB, &rectTex, &flagB, colA) != 0) {
                bVar2 = 1;
                second = 1;
            }
        }

        if ((rec->flags & 0x200) == 0) {
            if (RallyData_IsHeadToHeadRaceMode() == 0) {
                if (rec->primaryCallId != 0)
                    Race_AssignSlotsFromCallRecord(rec->primaryCallId);
                if (rec->secondaryCallId != 0)
                    Race_AssignSlotsFromCallRecord(rec->secondaryCallId);
            }
            rec->flags |= 0x200;
        }

        if (found == 0 && second == 0)
            return;

        rectMain.x = rectIcon.x;
        rectMain.y = (short)yText;
        rectMain.w = rectIcon.w;
        rectMain.h = rectIcon.h;
        rectSecond.y = (short)yText;
        rectSecond.w = rectIcon.w;
        rectSecond.h = rectIcon.h;

        if (iVar != 0)
            goto next;

        pState = RallyData_GetDriverHudFlags(StageUI_GetRaceEndEventCount() + player);
        if ((*pState & 3) == 2)
            goto next;

        if (bVar2) {
            n = 0x32 - (rec->flags & 0xff);
            if (n < 0xa)
                value = -curve[10 - n];
            else if (n < 0x14)
                value = 0;
            else if (n < 0x1e)
                value = -curve[n - 0x14];
            else
                value = 0x400;
            if (n < 0x14)
                curveB = 0x400;
            else if (n < 0x1e)
                curveB = -curve[0x1e - n];
            else if (n < 0x28)
                curveB = 0;
            else if (n < 0x32)
                curveB = -curve[n - 0x28];
            else
                curveB = 0x400;
        } else {
            n = 0x32 - (rec->flags & 0xff);
            if (n >= 0x28)
                value = -curve[n - 0x28];
            else if (n > 0xa)
                value = 0;
            else
                value = curve[0xa - n];
        }

        if ((rec->flags & 0x100) == 0)
            value = 0;

        rectSecond.y = (short)(rectSecond.y + (short)yOff);
        rectMain.y = (short)(rectMain.y + (short)yOff);

        if (value != 0) {
            r = FixMul((int)FTOI(__int64, X87S_MUL(((int)rectMain.w / 2), CGraphics::m_65536)), g_sinTable[(value + 0x400) & 0xfff]);
            rectMain.w = (short)(r * 2 >> 16);
        }
        if (found == 0)
            rectMain.w = 0;

        if (bVar2) {
            r = FixMul((int)FTOI(__int64, X87S_MUL(((int)rectSecond.w / 2), CGraphics::m_65536)), g_sinTable[(curveB + 0x400) & 0xfff]);
            r = r * 2 >> 16;
            rectSecond.w = (short)r;
            rectMain.x = (short)((((int)g_pGraphics->resX << 15) >> 16) - (r + (int)rectMain.w) / 2 + xOff);
            if (found != 0) {
                if (flagA == 0) {
                    Sprite_Queue(&rectIcon, &rectMain, texA, 2, 0, NULL, NULL, colB, 8);
                } else if (rectMain.w != 0) {
                    Sprite_Queue(&rectIcon, &rectMain, texA, 2, 0, NULL, NULL, colB, 1);
                }
            }
        } else {
            rectSecond.w = 0;
            r = rectMain.w;
            rectMain.x = (short)((((int)g_pGraphics->resX << 15) >> 16) - (short)r / 2 + xOff);
            if (found != 0) {
                if (flagA != 0)
                    Sprite_Queue(&rectIcon, &rectMain, texA, 2, 0, NULL, NULL, colB, 1);
                else
                    Sprite_Queue(&rectIcon, &rectMain, texA, 2, 0, NULL, NULL, colB, 8);
            }
        }

        rectSecond.x = (short)(rectMain.x + rectMain.w);
        if (second != 0 && rectSecond.w != 0) {
            if (flagB == 0)
                Sprite_Queue(&rectTex, &rectSecond, texB, 2, 0, NULL, NULL, colA, 8);
            else
                Sprite_Queue(&rectTex, &rectSecond, texB, 2, 0, NULL, NULL, colA, 1);
        }

next:
        iVar++;
    } while (iVar < 5);
}

// FUNCTION: CMR2 0x00417e60
void Race_ClearMessageDrawLatch(void)
{
    g_unk0x00537190 = 0;
}

BYTE *StageObject_GetPlayerViewRectangle(int view);
int Font_GetTextWidth(BYTE index, BYTE *text);
int Font_GetTextHeight(BYTE index, char *text);
void Font_DrawText(BYTE index, char *text, short x, short y, int *pColour, unsigned int flags);

// Shadow colour of the race call text (black).
// GLOBAL: CMR2 0x00517e28
int g_raceCallShadowColour = 0xff000000;

// Draws one race message text, centred on the given player's viewport, with an
// optional dark offset copy underneath. Only one message per frame.
// FUNCTION: CMR2 0x00417e70
void Race_DrawPlayerMessage(char *pText, int *pColour, int player, int shadow, int x, int y)
{
    short *pViewRect;
    int textY;
    int pos;
    int race;
    unsigned int flags;
    BYTE *pState;

    if (g_unk0x00537190 != 0)
        return;

    race = 1;
    g_unk0x00537190 = 1;
    pos = 0;
    pViewRect = (short *)StageObject_GetPlayerViewRectangle(player);
    if (RallyData_IsHeadToHeadRaceMode() != 0) {
        if (CGameInfo::IsSplitBarEnabled()) {
            if (player == 1)
                pos = (int)g_pGraphics->resY / 2;
            pos -= (int)g_pGraphics->resY * 9011 >> 16;
        } else if (player == 0) {
            // The original duplicates the tail under a `player == 0` test
            // (MSVC6 leaves the dead test in the else branch).
            pos = (int)g_pGraphics->resY * 9011 >> 16;
        } else {
            pos = (int)g_pGraphics->resY * 9011 >> 16;
        }
    } else {
        race = 0;
        pos = 0;
    }
    textY = pos + ((int)g_pGraphics->resY << 14 >> 16);
    if (race == 0) {
        pState = RallyData_GetDriverHudFlags(StageUI_GetRaceEndEventCount() + player);
        if ((*pState & 3) == 1)
            textY = ((int)g_pGraphics->resY << 14 >> 16) + pos + 0x10;
    }
    Font_GetTextWidth(2, (BYTE *)pText);
    Font_GetTextHeight(2, pText);

    pos = (int)pViewRect[2] / 2 + (int)pViewRect[0];
    if (x != -1) {
        pos = x;
        textY = y;
        flags = 9;
    } else {
        flags = 0x12;
    }
    if (shadow != 0)
        Font_DrawText(2, pText, pos + 1, (int)g_pGraphics->resY / 0x60 + 1 + textY, &g_raceCallShadowColour, flags);
    Font_DrawText(2, pText, pos, (int)g_pGraphics->resY / 0x60 + textY, pColour, flags);
}

// Assigns the race slots announced by a call record: every bit field of the
// id selects one slot of the race slot table.
// FUNCTION: CMR2 0x00418000
void Race_AssignSlotsFromCallRecord(unsigned int id)
{
    unsigned int f4;
    unsigned int f17;
    unsigned int f5;
    unsigned int f6;
    unsigned int f7;
    unsigned int f3;
    unsigned int f11;
    unsigned int f9;
    unsigned int f12;
    unsigned int f10;
    unsigned int f8;
    unsigned int f1;
    int slot;

    f3 = id >> 4 & 3;
    f4 = id >> 0xc & 7;
    f17 = id >> 0x17 & 1;
    f5 = id >> 0xf & 3;
    f6 = id >> 0x11 & 0xf;
    f7 = id >> 0x1c & 3;
    f11 = id >> 6 & 0xf;
    f9 = id >> 10 & 3;
    f12 = id >> 0x15 & 3;
    f10 = id & 0xf;
    f8 = id >> 0x18 & 1;
    f1 = id >> 0x19 & 7;

    if (f10 == 6) {
        if (f3 == 0 && f11 == 0 && f9 == 0 && f4 == 0 && f6 == 0 && f12 == 0 && f5 == 0 &&
            f7 == 0 && f8 == 0 && f1 == 0)
            return;
    } else if (f10 == 9) {
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x37);
    }

    switch (f11) {
    case 1:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0xe);
        break;
    case 2:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0xf);
        break;
    case 3:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x10);
        break;
    case 4:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x11);
        break;
    case 5:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x12);
        break;
    case 6:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x13);
        break;
    case 7:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x14);
        break;
    case 8:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x15);
        break;
    case 9:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x16);
        break;
    case 10:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x16);
        break;
    case 11:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x16);
        break;
    case 12:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x16);
        break;
    case 13:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x16);
        break;
    case 14:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x16);
        break;
    }


    switch (f12) {
    case 1:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x1a);
        break;
    case 2:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x1b);
        break;
    }
    switch (f9) {
    case 1:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 10);
        break;
    case 2:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x1c);
        break;
    }
    switch (f17) {
    case 1:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x1d);
        break;
    }

    switch (f10) {
    case 6:
        Race_AssignUnusedSlot(g_coDriverFirstSample);
        break;
    case 5:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x1);
        break;
    case 4:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x2);
        break;
    case 3:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x3);
        break;
    case 2:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x4);
        break;
    case 1:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x5);
        break;
    case 8:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x7);
        break;
    case 7:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x6);
        break;
    }


    switch (f3) {
    case 1:
        if (CGameInfo::IsActiveCheatEnabled(2) != 0)
            Race_AssignUnusedSlot(g_coDriverFirstSample + 9);
        else
            Race_AssignUnusedSlot(g_coDriverFirstSample + 8);
        break;
    case 2:
        if (CGameInfo::IsActiveCheatEnabled(2) != 0)
            Race_AssignUnusedSlot(g_coDriverFirstSample + 8);
        else
            Race_AssignUnusedSlot(g_coDriverFirstSample + 9);
        break;
    }

    switch (f4) {
    case 1:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0xc);
        break;
    case 2:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0xb);
        break;
    case 3:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x1e);
        break;
    }
    switch (f5) {
    case 1:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x17);
        break;
    case 2:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x1f);
        break;
    }

    switch (f6) {
    case 1:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x19);
        break;
    case 14:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x36);
        break;
    case 2:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x18);
        break;
    case 15:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x20);
        break;
    case 6:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x21);
        break;
    case 3:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x22);
        break;
    case 4:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x23);
        break;
    case 5:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x24);
        break;
    case 7:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x25);
        break;
    case 8:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x26);
        break;
    case 9:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x27);
        break;
    case 10:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x28);
        break;
    case 11:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x29);
        break;
    case 12:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x2a);
        break;
    case 13:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x2b);
        break;
    }


    switch (f7) {
    case 1:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x2c);
        break;
    case 2:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x2d);
        break;
    }
    switch (f8) {
    case 1:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x2e);
        break;
    }

    switch (f1) {
    case 1:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x2f);
        return;
    case 2:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x30);
        return;
    case 3:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x31);
        return;
    case 4:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x32);
        return;
    case 5:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x33);
        return;
    case 6:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x34);
        return;
    case 7:
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0x35);
    }
}

int StageObject_GetCarSoundElapsedTime(int index);
int Sound_PlaySampleWithParameters(unsigned short id, int volume, int frequency, int loopStart, int loops, int is3D);
void Sound_Free(unsigned int handle);
int Sound_GetVolume(unsigned int handle);
int Race_GetNormalizedCarSpeed(int car);
int NetRace_GetListenerDistanceAttenuation(unsigned int view, int listener);
void Sound_SetPlayingSlotVolume(unsigned int handle, int volume);
BYTE StageObject_GetViewWeatherStateByte(int index);
int StageObject_GetViewWeatherField54(int index);
void Sound_SetPan(unsigned int handle, int pan);
extern int g_unk0x005374c0;
extern int g_carSlotVolumes[8][4];
extern int g_carMaxVolume[8];
extern int g_unk0x00537564;

// GLOBAL: CMR2 0x00537358
unsigned int *g_coDriverCalls;

// Registered callback of 0x416720.
// FUNCTION: CMR2 0x00418550
int Race_ReleaseCoDriverCallData(void)
{
    g_coDriverCalls = 0;
    return 1;
}

// FUNCTION: CMR2 0x00418560
void Race_SetCoDriverCallState(int value)
{
    g_coDriverCallState = value;
}

// FUNCTION: CMR2 0x00418570
int Race_GetCoDriverCallState(void)
{
    return g_coDriverCallState;
}

extern SpriteRect *g_pArrowRects;
extern Texture *g_arrowTexture;

// Decodes a race call slot id into the arrow sprite rect of the call icon, the
// secondary-icon flag and the colour the icon is tinted with.
// FUNCTION: CMR2 0x00418580
int Race_DecodeCallSlotIcon(unsigned int id, Texture **pTexture, SpriteRect *pRect, unsigned int *pFlag, BYTE *pColour)
{
    unsigned int type;
    unsigned int b1;
    unsigned int b2;
    unsigned int b3;
    SpriteRect *pSrc;
    int result;
    int afterFirst;

    type = id & 0xf;
    b3 = id >> 0x15 & 3;
    b2 = id >> 0x11 & 0xf;
    b1 = id >> 4 & 3;

    if (type == 0 || type == 9)
        afterFirst = 0;
    else
        afterFirst = 1;
    if (b1 != 0 || afterFirst || b2 == 0xe || b2 == 2 || b2 == 1 || b3 != 0)
        result = 1;
    else
        result = 0;

    pColour[0] = 0xff;
    pColour[1] = 0xff;
    pColour[2] = 0xff;
    pColour[3] = 0xff;
    if (b1 == 1)
        *pFlag = 1;
    else
        *pFlag = 0;
    if (CGameInfo::IsActiveCheatEnabled(2) != 0)
        *pFlag ^= 1;

    switch (type) {
    case 2:
        pSrc = &g_pArrowRects[5];
        break;
    case 3:
        pSrc = &g_pArrowRects[4];
        break;
    case 4:
        pSrc = &g_pArrowRects[3];
        break;
    case 5:
    case 6:
        pSrc = &g_pArrowRects[2];
        break;
    case 7:
        pSrc = &g_pArrowRects[1];
        break;
    case 8:
        pSrc = &g_pArrowRects[0];
        break;
    default:
        pSrc = &g_pArrowRects[6];
        break;
    }
    *pRect = *pSrc;

    if (b2 != 0 && b1 == 0) {
        *pFlag = 0;
        *pRect = g_pArrowRects[7];
        if (b2 == 2 || b2 == 0xe) {
            pColour[0] = 0xff;
            pColour[1] = 0xff;
            pColour[2] = 0xd;
        }
        if (b2 == 1) {
            pColour[0] = 0xff;
            pColour[1] = 0;
            pColour[2] = 0;
        }
    }

    if (b3 != 0) {
        *pFlag = 0;
        *pRect = g_pArrowRects[7];
        pColour[0] = 0xff;
        if (b3 == 1) {
            pColour[1] = 0xff;
            pColour[2] = 0xd;
        } else {
            pColour[1] = 0;
            pColour[2] = 0;
        }
    }

    *pTexture = g_arrowTexture;
    return result;
}

// FUNCTION: CMR2 0x00418d20
void Race_SetCarSoundSelectionState(int value)
{
    g_unk0x00537394 = value;
}

// GLOBAL: CMR2 0x00537568
int g_surfacePrevD[8];
// GLOBAL: CMR2 0x00537588
int g_surfaceVolA[8];
// GLOBAL: CMR2 0x005375a8
StageSoundBank g_stageSoundBank;
// GLOBAL: CMR2 0x005375b0
int g_surfacePrevB[8];
// GLOBAL: CMR2 0x005375d0
unsigned int g_stageSoundCount;
// GLOBAL: CMR2 0x005375d4
BYTE g_stageSoundUsed[0x1f];
// GLOBAL: CMR2 0x005375f4
BYTE g_carSoundViewSet[8];
// GLOBAL: CMR2 0x005375fc
int g_surfacePrevA[8];
// GLOBAL: CMR2 0x0053761c
int g_surfaceSlotMax;
// GLOBAL: CMR2 0x00537620
int g_surfaceVolB[8];
// GLOBAL: CMR2 0x00537640
int g_surfaceVolC[8];
// GLOBAL: CMR2 0x00537660
int g_unk0x00537660;
// GLOBAL: CMR2 0x00537664
int g_carSoundVolumeScale;
// GLOBAL: CMR2 0x00537668
int g_surfaceVolD[8];
// GLOBAL: CMR2 0x00537788
int g_surfacePrevC[8];
// GLOBAL: CMR2 0x005377a8
RaceCarSoundState g_carSoundStates[8];
// GLOBAL: CMR2 0x00537d48
int g_wheelSlipVolume[8][4];
// GLOBAL: CMR2 0x00537dc8
int g_unk0x00537dc8;

// Starts the sound of one entry of the stage table and stores its handle, the
// random pitch and the id of the sound.
// match 54%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00418d30
void CarSound_PlaySlot(int param1, int param2, int param3, int param4, int param5)
{
    int index;

    index = param3;
    if (param5 != 0)
        g_carSoundSets[param1].handle[index] = Sound_PlaySampleWithParameters(param2, param4, 0x5622, param5, 1, 0);
    else
        g_carSoundSets[param1].handle[index] = Sound_PlaySampleWithParameters(param2, param4, 0x5622, 0, 1, 0);
    g_carSoundSets[param1].pitch[index] = rand() % 0x19 + 0x32 + StageObject_GetCarSoundElapsedTime(param1);
    g_carSoundSets[param1].surface[param3] = g_carSoundViewSet[param1];
    g_carSoundSets[param1].id[index] = param2;
}

// Stops the sound of one entry of the stage table (and forgets both the handle
// and the id).
// FUNCTION: CMR2 0x00418dd0
void CarSound_StopSlot(int param1, int param2, char param3)
{
    if (param3 != 0)
        g_carSoundSets[param1].id[param2] = -1;
    Sound_Free(g_carSoundSets[param1].handle[param2]);
    g_carSoundSets[param1].handle[param2] = -1;
}

// Returns a random value in [0, param2) that is not param1.
// FUNCTION: CMR2 0x00419b50
int CarSound_PickDifferentSampleIndex(int param1, int param2)
{
    int value;

    if (param2 == 1)
        return 0;
    value = rand() % param2;
    while (value == param1) {
        value = rand() % param2;
    }
    return value;
}

void CarSound_PlaySlot(int param1, int param2, int param3, int param4, int param5);

void CarSound_StopSlot(int param1, int param2, char param3);
void Race_MoveCarSoundSlot(int set, int dst, int src);
BYTE NetRace_GetRaceSoundMode(void);

// Moves the car's current sounds to the second bank (slots 4/5 to 6/7) on a
// sound state change, stopping the ones that don't carry over.
// match 50%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00419b90
void Race_MoveCarSoundBank(int car, RaceCarSoundState *pInfo)
{
    switch (pInfo->pattern) {
    case 6:
    case 0x15:
        if (!NetRace_GetRaceSoundMode())
            CarSound_StopSlot(car, 4, 1);
        Race_MoveCarSoundSlot(car, 4, 5);
        break;
    case 7:
        if (!NetRace_GetRaceSoundMode())
            CarSound_StopSlot(car, 4, 1);
        Race_MoveCarSoundSlot(car, 4, 5);
        Race_MoveCarSoundSlot(car, 6, 7);
        break;
    case 0xb:
        if (!NetRace_GetRaceSoundMode()) {
            CarSound_StopSlot(car, 4, 1);
            CarSound_StopSlot(car, 6, 1);
        }
        Race_MoveCarSoundSlot(car, 4, 5);
        break;
    case 0xc:
        if (!NetRace_GetRaceSoundMode()) {
            CarSound_StopSlot(car, 4, 1);
            CarSound_StopSlot(car, 6, 1);
        }
        Race_MoveCarSoundSlot(car, 4, 5);
        Race_MoveCarSoundSlot(car, 6, 7);
        break;
    case 2:
    case 0x11:
    case 0x16:
        Race_MoveCarSoundSlot(car, 4, 5);
        Race_MoveCarSoundSlot(car, 6, 7);
        break;
    case 1:
    case 4:
    case 9:
    case 0xe:
    case 0x10:
    case 0x13:
    case 0x18:
        Race_MoveCarSoundSlot(car, 4, 5);
        break;
    default:
        goto tail;
    }
tail:
    g_surfacePrevA[car] = g_surfacePrevB[car];
    g_surfacePrevD[car] = g_surfacePrevC[car];
    g_surfacePrevB[car] = 0;
    g_surfacePrevC[car] = 0;
}

// Restarts the stage sound of the slots whose surface changed while their
// sound is still playing; the new sound reuses the slot's id and volume.
// The per-car sound ids are addressed through the flat block index (car*45+i),
// as in the original.
#define g_carSoundIds ((int *)((BYTE *)g_carSoundStates + 0x44))   // 0x5377ec
// FUNCTION: CMR2 0x00419cd0
void Race_RerollCarSoundSlotsByPitch(int car, int unused)
{
    RaceCarSoundState *pState = &g_carSoundStates[car];
    StageSoundPattern *pPattern;
    int i;
    int old;

    if (pState->state == -1)
        return;
    pPattern = &g_stageSoundPatterns[g_stageSoundPatterns[pState->state].redirect];
    for (i = 0; i < 10; i++) {
        if (i == 4 && pState->state == 0x19)
            continue;
        if (pState->handle[i] == -1)
            continue;
        if (Sound_IsPlaying(pState->handle[i]) == 0)
            continue;
        if ((unsigned int)StageObject_GetCarSoundElapsedTime(car) <= (unsigned int)pState->pitch[i])
            continue;
        switch (i) {
        case 4:
            if (pPattern->choices[g_carSoundViewSet[car]] > 1 && pState->pattern == 0x19) {
                old = Sound_GetVolume(pState->handle[i]);
                CarSound_StopSlot(car, i, 0);
                CarSound_PlaySlot(car, CarSound_PickDifferentSampleIndex(g_carSoundIds[car * 45 + i],
                                               pPattern->choices[g_carSoundViewSet[car]]) +
                                  pPattern->base[g_carSoundViewSet[car]], i, old, 0);
            }
            break;
        case 5:
            if (pPattern->choices[g_carSoundViewSet[car]] > 1) {
                old = Sound_GetVolume(pState->handle[i]);
                CarSound_StopSlot(car, i, 0);
                CarSound_PlaySlot(car, CarSound_PickDifferentSampleIndex(g_carSoundIds[car * 45 + i],
                                               pPattern->choices[g_carSoundViewSet[car]]) +
                                  pPattern->base[g_carSoundViewSet[car]], i, old, 0);
            }
            break;
        case 7:
            if (pPattern->choices[g_carSoundViewSet[car] + 2] > 1) {
                old = Sound_GetVolume(pState->handle[i]);
                CarSound_StopSlot(car, i, 0);
                CarSound_PlaySlot(car, CarSound_PickDifferentSampleIndex(g_carSoundIds[car * 45 + i],
                                               pPattern->choices[g_carSoundViewSet[car] + 2]) +
                                  pPattern->base[g_carSoundViewSet[car] + 2], i, old, 0);
            }
            break;
        case 6:
            if (pPattern->choices[g_carSoundViewSet[car] + 2] > 1 && pState->pattern == 0x19) {
                old = Sound_GetVolume(pState->handle[i]);
                CarSound_StopSlot(car, i, 0);
                CarSound_PlaySlot(car, CarSound_PickDifferentSampleIndex(g_carSoundIds[car * 45 + i],
                                               pPattern->choices[g_carSoundViewSet[car] + 2]) +
                                  pPattern->base[g_carSoundViewSet[car] + 2], i, old, 0);
            }
            break;
        }
    }
}

// Restarts the sound of the slots whose surface changed: a slot of the first
// bank (4/5) uses the choices of the current state, one of the second bank
// (6/7) the secondary choices, and the sound is re-rolled from the pattern.
// FUNCTION: CMR2 0x00419ed0
void Race_RestartChangedSurfaceSounds(int car, int unused)
{
    RaceCarSoundState *pState = &g_carSoundStates[car];
    StageSoundPattern *pPattern;
    int i;
    int old;

    if (pState->state == -1)
        return;
    pPattern = &g_stageSoundPatterns[g_stageSoundPatterns[pState->state].redirect];
    for (i = 0; i < 10; i++) {
        if (pState->handle[i] == -1)
            continue;
        if (Sound_IsPlaying(pState->handle[i]) == 0)
            continue;
        if (pState->surface[i] == g_carSoundViewSet[car])
            continue;
        switch (i) {
        case 4:
            if (pState->pattern == 0x19 && pState->state != 0x19) {
                old = Sound_GetVolume(pState->handle[i]);
                CarSound_StopSlot(car, i, 1);
                CarSound_PlaySlot(car, CarSound_PickDifferentSampleIndex(-1, pPattern->choices[g_carSoundViewSet[car]]) +
                                  pPattern->base[g_carSoundViewSet[car]], i, old, 0);
            }
            break;
        case 5:
            old = Sound_GetVolume(pState->handle[i]);
            CarSound_StopSlot(car, i, 1);
            CarSound_PlaySlot(car, CarSound_PickDifferentSampleIndex(-1, pPattern->choices[g_carSoundViewSet[car]]) +
                              pPattern->base[g_carSoundViewSet[car]], i, old, 0);
            break;
        case 6:
            if (pState->pattern == 0x19) {
                old = Sound_GetVolume(pState->handle[i]);
                CarSound_StopSlot(car, i, 1);
                CarSound_PlaySlot(car, CarSound_PickDifferentSampleIndex(-1, pPattern->choices[g_carSoundViewSet[car] + 2]) +
                                  pPattern->base[g_carSoundViewSet[car] + 2], i, old, 0);
            }
            break;
        case 7:
            old = Sound_GetVolume(pState->handle[i]);
            CarSound_StopSlot(car, i, 1);
            CarSound_PlaySlot(car, CarSound_PickDifferentSampleIndex(-1, pPattern->choices[g_carSoundViewSet[car] + 2]) +
                              pPattern->base[g_carSoundViewSet[car] + 2], i, old, 0);
            break;
        }
    }
}

// Keeps the car's stage sound independent engine sample in step with the
// speed of the car: slot 8 is used on the tarmac, slot 9 on the other
// surfaces, and the other one is stopped when the surface changes.
// FUNCTION: CMR2 0x0041a0a0
void Race_UpdateIndependentEngineSound(int car, int param2)
{
    RaceCarSoundState *pRow = &g_carSoundStates[car];
    int volume;
    int engSpeed;
    int speedVol;

    speedVol = Race_GetNormalizedCarSpeed(car);
    engSpeed = FixMulShift32(Car_Get(car)->speed, 0x431168);
    if (StageObject_GetViewWeatherStateByte(car) == 1)
        volume = FixMul(StageObject_GetViewWeatherField54(car), 0x13333);
    else
        volume = 0;
    if (g_carSoundViewSet[car] == 0) {
        if (volume > 0x10000)
            volume = 0x10000;
        if (Sound_IsPlaying(pRow->handle[9]))
            CarSound_StopSlot(car, 9, 1);
        if (engSpeed < 0)
            speedVol = 0;
        else if (engSpeed > 0x64)
            speedVol = 0x8000;
        else
            speedVol = 0x10000 - FixMul(0x8000,
                            FixDiv((int)FTOI(__int64, X87S_MUL((double)engSpeed, CGraphics::m_65536)), 0x640000));
        volume = FixMul(volume, speedVol);
        if (engSpeed < 0)
            engSpeed = 0;
        else if (engSpeed > 0x64)
            engSpeed = 11000;
        else
            engSpeed = engSpeed * 11000 / 100;
        engSpeed += 0x5622;
        if (!Sound_IsPlaying(pRow->handle[8]))
            CarSound_PlaySlot(car, g_unk0x00537564, 8,
                         FixMul(NetRace_GetListenerDistanceAttenuation(car, param2), FixMul(g_carSoundVolumeScale, volume)), 0);
        Sound_SetPlayingSlotVolume(pRow->handle[8],
                     FixMul(NetRace_GetListenerDistanceAttenuation(car, param2), FixMul(g_carSoundVolumeScale, volume)));
        Sound_SetPan(pRow->handle[8], engSpeed);
    } else {
        if (volume > 0x10000)
            volume = 0x10000;
        if (Sound_IsPlaying(pRow->handle[8]))
            CarSound_StopSlot(car, 8, 1);
        if (!Sound_IsPlaying(pRow->handle[9]))
            CarSound_PlaySlot(car, g_unk0x00537dc8, 9,
                         FixMul(NetRace_GetListenerDistanceAttenuation(car, param2), FixMul(g_carSoundVolumeScale, volume)), 0);
        volume = FixMul(volume, speedVol);
        Sound_SetPlayingSlotVolume(pRow->handle[9],
                     FixMul(NetRace_GetListenerDistanceAttenuation(car, param2), FixMul(g_carSoundVolumeScale, volume)));
    }
}

// Picks the stage sound state of a car from the surfaces under its wheels and
// the sounds already playing, and applies it when it changed.
// match 51%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0041a340
void Race_UpdateCarSurfaceSoundState(int car, int unused)
{
    RaceCarSoundState *pState = &g_carSoundStates[car];
    Car *pCar;
    short value;
    short v;
    short *pSrc;
    int *pFlags;
    short *pSlot;
    int *pTime;
    int counts[4];
    char allTwo;
    int bestIndex;
    int bestCount;
    int chosen;
    int minSlack;
    int newCount;
    int i;
    int j;

    bestIndex = 0;
    chosen = 0;
    minSlack = 0xffffffff;
    pCar = Car_Get(car);
    pSrc = pCar->wheelSurface;
    pFlags = pCar->cornerOnGround;
    pSlot = pState->slotState;
    pTime = pState->slotTime;
    for (i = 4; i != 0; i--) {
        value = *pSrc;
        if (*pFlags == 0 || value > 0x1e || value < 0 || value == 0x10)
            value = -1;
        if (*pSlot != value)
            *pTime = StageObject_GetCarSoundElapsedTime(car);
        *pSlot = value;
        pSrc++;
        pFlags++;
        pSlot++;
        pTime++;
    }
    bestCount = 0;
    allTwo = 1;
    for (i = 0; i < 4; i++) {
        v = pState->slotState[i];
        counts[i] = 0;
        for (j = 0; j < 4; j++) {
            if (v == pState->slotState[j])
                counts[i]++;
        }
        if (v == -1 && counts[i] != 4)
            counts[i] = 0;
        if (counts[i] > bestCount) {
            bestIndex = i;
            bestCount = counts[i];
        }
        if (counts[i] != 2)
            allTwo = 0;
    }
    for (i = 0; i < 4; i++) {
        if (counts[i] >= 3)
            break;
        if (counts[i] == 2 && !allTwo)
            break;
    }
    if (i < 4) {
        chosen = pState->slotState[bestIndex];
    } else {
        for (i = 0; i < 4; i++) {
            // the original reads the clock again for the stored value
            if ((unsigned int)(StageObject_GetCarSoundElapsedTime(car) - pState->slotTime[i]) < (unsigned int)minSlack) {
                chosen = pState->slotState[i];
                minSlack = StageObject_GetCarSoundElapsedTime(car) - pState->slotTime[i];
            }
        }
    }
    if (chosen == -1)
        newCount = 0;
    else
        newCount = g_stageSoundPatterns[g_stageSoundPatterns[chosen].redirect].count;
    if (pState->countOld != newCount || pState->stateOld != chosen) {
        if (pState->pattern != 0x19)
            Race_MoveCarSoundBank(car, pState);
        pState->state = (short)chosen;
        pState->count = newCount;
        pState->time = StageObject_GetCarSoundElapsedTime(car);
        pState->pattern = g_carSoundStates[car].count + pState->countOld * 5;
        StageUI_ApplySoundState(car, pState);
        pState->countOld = pState->count;
        pState->stateOld = pState->state;
    }
    if (pState->pattern != 0x19 && (unsigned int)(StageObject_GetCarSoundElapsedTime(car) - pState->time) > 0x32) {
        Race_MoveCarSoundBank(car, pState);
        pState->pattern = 0x19;
    }
}

// While the car's stage sound state is group 25, switches its slot-4 sound
// between the group's throttle sample and the coasting one as the throttle
// torque turns positive or not (RaceCarSoundState::throttleSample).
// FUNCTION: CMR2 0x0041ae80
void Race_SwitchEngineSampleByThrottle(int car, int unused)
{
    RaceCarSoundState *pSet = &g_carSoundStates[car];
    int *pHandle = &pSet->handle[4];

    if (pSet->state == 0x19) {
        if (Car_Get(car)->throttleTorque > 0) {
            if (pSet->throttleSample == 0) {
                if (Sound_IsPlaying(*pHandle)) {
                    Sound_Free(*pHandle);
                    *pHandle = -1;
                }
                CarSound_PlaySlot(car, g_stageSoundPatterns[25].base[g_carSoundViewSet[car]], 4, 0, 0);
                pSet->throttleSample = 1;
            }
        } else if (pSet->throttleSample != 0) {
            if (Sound_IsPlaying(*pHandle)) {
                Sound_Free(*pHandle);
                *pHandle = -1;
            }
            CarSound_PlaySlot(car, g_stageSoundPatterns[25].base[g_carSoundViewSet[car]] + 1, 4, 0, 0x3542);
            pSet->throttleSample = 0;
        }
    }
}

// Marks the sound groups used by the stage's surfaces.
// FUNCTION: CMR2 0x0041afe0
void Race_MarkUsedSurfaceSoundGroups(BYTE *pSurfaces, unsigned int count)
{
    unsigned int i;

    memset(g_stageSoundUsed, 0, 0x1f);
    g_stageSoundCount = count;
    for (i = 0; i < g_stageSoundCount; i++)
        g_stageSoundUsed[g_stageSoundPatterns[Surface_GetMappedIndex(pSurfaces[i])].redirect] = 1;
}

// FUNCTION: CMR2 0x0041b040
void Race_SetSurfaceSoundScale(int value)
{
    g_carSoundVolumeScale = FixMul(value, 0x10000);
}

// FUNCTION: CMR2 0x0041bf50
int Race_ReadPlayerState37F68(int index)
{
    return g_playerEventAwardFlags[index];
}

// FUNCTION: CMR2 0x0041bf60
int Race_ReadPlayerState37F78(int index)
{
    return g_unk0x00537f78[index];
}

// FUNCTION: CMR2 0x0041bf70
int Race_ReadPlayerState37F0C(int index)
{
    return (int)g_unk0x00537f0c[index];
}

// FUNCTION: CMR2 0x0041d290
int Race_GetScaledState37F24(void)
{
    return g_unk0x00537f24 << 2;
}

// FUNCTION: CMR2 0x0041d2a0
int Race_ReadState37F24(void)
{
    return g_unk0x00537f24;
}

// FUNCTION: CMR2 0x0041d780
unsigned int Race_GetClampedCountdownValue(void)
{
    unsigned int value = g_unk0x00537fc0;
    if (value > 499)
        value = 499;
    return value;
}

// FUNCTION: CMR2 0x0041db00
BYTE Race_ReadTransitionState3811C(void)
{
    return g_unk0x0053811c;
}

// Callback count to unwind to when the race ends.
// GLOBAL: CMR2 0x0053896c
int g_raceCallbackMark;

void Sound_FreeAll(void);

// FUNCTION: CMR2 0x00420100
void Race_FreeSoundsAndUnwindCallbacks(void)
{
    Sound_FreeAll();
    CGame::UnwindCallbacks(g_raceCallbackMark);
}

// GLOBAL: CMR2 0x00538868
char g_raceCarPath[0x104];
// GLOBAL: CMR2 0x0051945c
char g_strPathFormat[] = "%s\\%s";

// GLOBAL: CMR2 0x005196f0
char g_strCarC1Format[] = "%s\\%sc1";
// GLOBAL: CMR2 0x005196f8
char g_strCarD3Format[] = "%s\\%sd3%d";

// GLOBAL: CMR2 0x005199b0
int g_unk0x005199b0 = 0xacb49c;

// Path of a car's texture set: "<cars dir>\<car>d3<n>" or "<cars dir>\<car>c1".
// FUNCTION: CMR2 0x00420060
char *Car_GetTextureSetPath(int car, int variant, int unused)
{
    if ((BYTE)RallyData_GetFlag24()) {
        sprintf(g_raceCarPath, g_strCarD3Format, CInstallInfo::GetCarsDir(), CFrontend::GetArchiveDirectoryEntry(car),
                variant + 1);
        return g_raceCarPath;
    }
    sprintf(g_raceCarPath, g_strCarC1Format, CInstallInfo::GetCarsDir(), CFrontend::GetArchiveDirectoryEntry(car));
    return g_raceCarPath;
}

// Path of a car's directory ("<cars dir>\<car>").
// FUNCTION: CMR2 0x004200d0
char *Car_GetDirectoryPath(int car)
{
    sprintf(g_raceCarPath, g_strPathFormat, CInstallInfo::GetCarsDir(), CFrontend::GetArchiveDirectoryEntry(car));
    return g_raceCarPath;
}

extern int g_unk0x00537f5c;
// GLOBAL: CMR2 0x00537f64
BYTE g_raceResourcesFreed;
extern unsigned int g_unk0x00537fc0;
void Sound_FreeAll(void);

/* --------------------------------------------------------------------------
   Race bootstrap / teardown transitions (0x41bf80..0x41e5c0).
   -------------------------------------------------------------------------- */

void Car_SetDrawnFlag(int, char);
void Race_UpdatePlayerViewAndDrivenCars(BYTE, int);
struct ReplayStream;
int Replay_SelectAndInitializeLane(ReplayStream *p, short lane, short start, BYTE car);
void InRaceMenu_ResetAndBuildPages(void);
void ForceFeedback_DeactivateSlots(void);
int StageTiming_GetCheckpointField2(int);
void StageTiming_LoadStageBFLArchive(void);
BYTE StageObject_LoadStageMenuSounds(void);
void Surface_ConfigureMenuInput(void);
void Replay_InitSlots(void);
void View_SetCameraType(int, int, BYTE, int);
BOOL Sound_Init(int, int, int, int);
// GLOBAL: CMR2 0x00519284
char g_strArcadeAdp0x00519284[] = "%s\\arcade%d.adp";

// GLOBAL: CMR2 0x00537ffc
int g_unk0x00537ffc;
// GLOBAL: CMR2 0x00537f2c
int g_unk0x00537f2c;

void NetRace_SetRaceControlFlag(BYTE);
void NetPlayers_ResetStatisticsSequences(void);
void Menu_SetInputStateFlag(char);
void Knockout_ClearRoundActiveFlag(void);
void RallyTiming_ResetStageResults(void);
BYTE RallyData_IsFirstAvailableStageSelected(void);
void RallyTiming_ResetOverallPlayerTimes(void);
void RallyTiming_ResetDriverSplitState(void);
void RallyTiming_RebuildChampionshipPositionMap(void);
void CFrontend::ClearFrontendPlayerInputCounters(void);
void NetRace_InitializeRaceSoundVolumes(void);
void CGame::SaveRaceCallbackDepth(void);
void Surface_LoadStageFonts(void);
BYTE SurfaceText_LoadRegionalStringTable(void);
void NetPlayers_RebuildStageResults(int, int, int);
void RallyData_InitializeRaceRendering(void);
void Race_BuildSelectionPaths(void);
void StageTiming_OpenRegionalStageArchives(void);
void StageObject_BuildInRaceActionMenu(void);
// GLOBAL: CMR2 0x00538100
BYTE g_unk0x00538100;
// GLOBAL: CMR2 0x00537fd4
BYTE g_unk0x00537fd4;

// Enters a race: resets the race state, builds the stage data of the current
// mode and starts the race scene.
// FUNCTION: CMR2 0x0041bf80
void Race_EnterCurrentModeScene(CallbackStateMachine *param1, int param2)
{
    g_unk0x00538100 = 0;
    NetRace_SetRaceControlFlag(0);
    g_unk0x00537fd4 = 0;
    NetPlayers_ResetStatisticsSequences();
    g_unk0x0053810c = 0;
    g_unk0x00537f0c[5] = (ptrdiff_t)param1;
    g_unk0x00537ffa = 0;
    g_unk0x0053810d = 0;
    g_unk0x00538108 = 1;
    g_unk0x00537f94 = 0;
    if (CGameInfo::GetGameModeOptionBit19() != 0)
        NetPlayers_RebuildStageResults(0, 0, 0);
    if ((char)param2 == 0) {
        Menu_SetInputStateFlag(0);
        CInput::SetInputRepeatTimingParameters(-1, -1, -1, -1, -1);
        if (CGameInfo::GetConfiguredGameMode() == 4)
            Knockout_ClearRoundActiveFlag();
        switch (CGameInfo::GetConfiguredGameMode()) {
        case 0:
            if (RallyDataStageIndex() == 0) {
                if ((BYTE)RallyDataCountryIndex() == 0)
                    RallyTiming_ResetStageResults();
                RallyTiming_ResetOverallPlayerTimes();
            }
            break;
        case 1:
            if (RallyDataStageIndex() == 0)
                RallyTiming_ResetOverallPlayerTimes();
            break;
        case 8:
            if (RallyData_IsFirstAvailableStageSelected() != 0)
                RallyTiming_ResetOverallPlayerTimes();
            break;
        case 5:
            if ((BYTE)RallyData_GetSelectionBits12To13() == 0)
                RallyTiming_ResetDriverSplitState();
            RallyTiming_RebuildChampionshipPositionMap();
            break;
        }
        if (CGameInfo::GetConfiguredGameMode() != 4)
            CFrontend::ClearFrontendPlayerInputCounters();
        NetRace_InitializeRaceSoundVolumes();
        Race_BuildSelectionPaths();
        CGame::SaveRaceCallbackDepth();
        StageTiming_OpenRegionalStageArchives();
        Surface_LoadStageFonts();
        SurfaceText_LoadRegionalStringTable();
        RallyData_InitializeRaceRendering();
        StageObject_BuildInRaceActionMenu();
        g_raceResourcesFreed = 0;
    }
    CGame::PromoteCallbackEntryByRule(param1, param2, 0, 2);
}

// Starts the arcade race: initialises the sound system, the replay slots and
// the race scene, and queues the arcade music track of the current rally.
// FUNCTION: CMR2 0x0041c0e0
void Race_StartArcadeScene(CallbackStateMachine *param1, int param2)
{
    char buffer[MAX_PATH];

    g_racePauseMenuActive = false;
    CGameInfo::SetInputAndGamePaused(0);
    if ((char)param2 == 0) {
        Sound_Init(0x5622, 2, 0x10, 1);
        StageTiming_LoadStageBFLArchive();
        StageObject_LoadStageMenuSounds();
        Surface_ConfigureMenuInput();
        if ((BYTE)RallyData_GetSelectionFlag26() != 0) {
            sprintf(buffer, g_strArcadeAdp0x00519284, CInstallInfo::GetMusicDir(),
                    (BYTE)RallyData_GetSelectionBits10To11() * 3 + 1 + (BYTE)RallyData_GetSelectionBits12To13());
            CSound::OpenStreamingMusicFile(buffer);
        }
        InRaceMenu_ResetAndBuildPages();
        Replay_InitSlots();
        g_unk0x00537ffc = CMain::GetFrameDelta();
    }
    CGame::PromoteCallbackEntryByRule(param1, param2, 0, 2);
    g_unk0x00537f08 = 1;
    g_unk0x00537f2c = 0;
    g_unk0x00537f78[5] = 1;
}

// Leaves the current race: releases the frame resources, tears the stage list
// down and fades the race out.
// FUNCTION: CMR2 0x0041e5c0
void Race_LeaveAndFadeOut(CallbackStateMachine *param1, char param2)
{
    int i;
    BYTE *p;

    i = 0;
    if (g_unk0x00538110 != 0) {
        g_unk0x00538110 = 0;
        g_unk0x00538114 = 1;
        Replay_SelectAndInitializeLane((ReplayStream *)g_playerReplayStreams[0], 0, 0, 0);
    }
    if (CGameInfo::GetConfiguredGameMode() == 6)
        Car_SetDrawnFlag(0, -1);
    if (param2 != 0)
        return;
    ForceFeedback_DeactivateSlots();
    if (CGameInfo::GetConfiguredGameMode() == 6) {
        if (StageTiming_GetCheckpointField2(0) >= 1)
            goto available;
        goto done;
    } else {
        if (Race_GetPlayerRecordField4(0) != 0)
            goto done;
    }
available:
    p = StageUI_GetRaceResultTable();
    if (*p > 0) {
        do {
            CGame::PromoteCallbackEntryByRule((CallbackStateMachine *)StageUI_GetRaceResultTable(), i, 1, 3);
            i++;
            p = StageUI_GetRaceResultTable();
        } while (i < *p);
    }
    Race_SetFlag3810D();
done:
    Race_UpdatePlayerViewAndDrivenCars((BYTE)param1->records[0].packedState, 1);
    g_unk0x00537f08 = 1;
}

typedef void (*FadeCallback)(BYTE index);
int NetPlayers_GetInputFrameDelta(void);
void NetPlayers_SendType16Notification(void);
int GameInfo_GetSessionField397C(void);
void GameMenu_LeaveStageAndAdvanceChampionship(void);
void NetPlayers_BuildFinalClassification(void);
unsigned int NetPlayers_IsPlayerPresent(int);
unsigned int NetPlayers_HasPlayerFinished(int);
void View_SwitchCamera(BYTE, int, int, BYTE, int);
void NetRace_DrainPendingMessages(void);
void NetRace_FadeOutPlayerScreen(BYTE, FadeCallback, int, int, int, char);
void NetRace_StartPlayerFade(BYTE, int, FadeCallback, int, int, int, int, char);
void GameMenu_FinishNetworkResultsQuitFade(BYTE);
void GameMenu_StartPauseOverlayFade(void);
BOOL Network_GetSessionStateFlag(void);
unsigned char GameInfo_GetFrontendSessionFlag(void);
int Replay_ResetBufferIfActive(ReplayStream *p);
void StageObject_SpawnRandomDebrisBurst(void);
BYTE View_GetActiveCameraFlags(BYTE);
int RallyData_IsChampionshipFinalStage(void);
void Race_StartPlayerAndOpponentReplays(int player);
extern BYTE g_unk0x0053811f;
// GLOBAL: CMR2 0x005191a4
int g_unk0x005191a4 = 0xacb49c;
// GLOBAL: CMR2 0x00537fd8
int g_unk0x00537fd8[8];

unsigned int RallyData_GetSetupFlag11(void);
BYTE StageTiming_GetCheckpointField1A(int index);
BYTE StageTiming_GetPendingDriverByteA7(void);
int StageObject_GetCollisionRecordState(void);
int StageTiming_GetCheckpointField0(int index);
int StageTiming_GetCarTimingByte81(int car);
void StageTiming_RecordDriverTimeoutFinish(int index);
int GameInfo_GetSessionField398C(void);
void NetPlayers_SendCarClass(BYTE carClass);
void CarDamage_CaptureStageSnapshot(int car);
void StageObject_UpdateCarSoundElapsedTime(int index);
int StageTiming_GetValidStartTime(int index);
int StageTiming_EstimateElapsedStageTime(void);
void NetRace_SendStageAndOverallTimes(int time);
void NetPlayers_UpdateBestRaceTime(unsigned int time);
extern int g_unk0x00537f30;
extern BYTE g_unk0x0053811d;
extern BYTE g_unk0x0053811e;
extern BYTE g_unk0x00538120;

void NetRace_SendReady(void);
void StageObject_StartCarSoundTimer(int index);
void StageObject_ResetCarSoundElapsedTime(int index);
int StageObject_GetCarSoundElapsedTime(int index);
unsigned int NetPlayers_IsPlayerReady(int index);
unsigned char RallyData_GetSelectionFlag28(void);
unsigned int RallyData_GetFlag22(void);
short Car_GetOrderCount(void);
void Car_ReloadModels(int, int, int);
void StageObject_StartReplaySession(char restart);
void StageTiming_StorePreviousCarCheckpoints(void);
void GameMenu_ClearPauseOverlayLatch(void);
void StageTiming_ResetClockAndPreviousTime(void);
struct ReplayStream;
int Replay_InitCarStreamState(ReplayStream *p, int unused, BYTE car);
int NetPlayers_GetPlayerField8(int index);
BYTE Car_GetDrawnFlag(int index);
char RallyData_GetUsableRecordCategory(BYTE param1);
void Replay_ResetActiveBufferState(void);
void Replay_RestartGhostCar(int, int);
HRESULT Sound_StartLoopingMusicStream(int param1);
void StageTiming_SetStartSelectionIndex(int value);

// Per-driver in-race update of the pre-race scene: resets the slot state, runs
// the countdown/fade of the current mode and rebuilds the player's view slots.
// FUNCTION: CMR2 0x0041d2b0
void Race_UpdateDriverPreRaceScene(CallbackStateMachine *param1, unsigned int param2)
{
    unsigned int index;
    int count;
    int i;
    BYTE **p;

    g_unk0x00538120 = 0;
    g_unk0x0053811f = 0;
    g_unk0x0053811e = 0;
    g_unk0x0053811d = 0;
    if (g_unk0x00537fd4 == 0) {
        NetRace_SendReady();
        g_unk0x00537fd4 = 1;
    }
    index = param2 & 0xff;
    g_unk0x00537f78[4] = 0;
    g_unk0x00537fcc = 0;
    g_unk0x005191a0 = 0xff;
    g_unk0x00537f98[index] = 0;
    g_unk0x00537fd8[index] = 0;
    if ((char)RallyData_GetSelectionFlag27() != 0 && CGameInfo::GetGameModeOptionBit19() == 0) {
        g_unk0x00537fbc[0] = 0;
        g_unk0x00537fbc[1] = 0;
    }
    g_unk0x00537ff8[index] = 0;
    StageObject_UpdateCarSoundElapsedTime(index);
    count = StageObject_GetCarSoundElapsedTime(index);
    if (CGameInfo::GetGameModeOptionBit19() != 0) {
        if (CGameInfo::GetConfiguredGameMode() != 10)
            NetRace_SetRaceControlFlag(1);
        NetRace_DrainPendingMessages();
        if (CGameInfo::GetConfiguredGameMode() != 10) {
            for (i = 0; i < 7; i++) {
                if ((char)NetPlayers_IsPlayerPresent(i) != 0 && (char)NetPlayers_IsPlayerReady(i) == 0) {
                    CInput::UpdateInputFrameDelta();
                    g_unk0x00537f08 = 1;
                    NetRace_SetRaceControlFlag(0);
                }
            }
        }
        if ((unsigned int)(CMain::GetFrameDelta() - NetPlayers_GetInputFrameDelta()) >
                (unsigned int)(GameInfo_GetSessionField397C() * 6000) && GameInfo_GetSessionField397C() != 0 &&
            Network_GetSessionStateFlag() != 0 &&
            (CGameInfo::GetConfiguredGameMode() == 10 || CGameInfo::GetConfiguredGameMode() == 12)) {
            if (g_unk0x00538100 != 0)
                goto fade;
            NetPlayers_SendType16Notification();
            g_unk0x00538100 = 1;
            NetRace_FadeOutPlayerScreen(0, GameMenu_FinishNetworkResultsQuitFade, 1, 0, g_unk0x005191a4, 1);
            return;
        }
        if (g_unk0x00538100 != 0) {
fade:
            Race_UpdatePlayerViewAndDrivenCars((BYTE)param1->records[index].packedState, 1);
            return;
        }
    }
    if (g_unk0x00537f08 != 0) {
        g_unk0x00537f24 = 0;
        StageObject_StartCarSoundTimer(index);
        StageObject_ResetCarSoundElapsedTime(index);
        g_unk0x00537f00 = StageObject_GetCarSoundElapsedTime(index);
        g_unk0x00537fc0 = 1;
        if (index == param1->count - 1)
            g_unk0x00537f08 = 0;
    } else if ((char)param2 == 0) {
        g_unk0x00537fc0 = count - g_unk0x00537f00;
        if (g_unk0x00537fc0 == 0)
            g_unk0x00537fc0 = 1;
    }
    if (CGameInfo::GetConfiguredGameMode() == 5 || CGameInfo::GetConfiguredGameMode() == 6 ||
        CGameInfo::GetConfiguredGameMode() == 7 || CGameInfo::GetConfiguredGameMode() == 4) {
        if ((unsigned int)g_unk0x00537fc0 >= 500) {
            GameMenu_ClearPauseOverlayLatch();
            goto big;
        }
    } else if ((unsigned int)g_unk0x00537fc0 >= 500) {
        GameMenu_ClearPauseOverlayLatch();
        goto big;
    }
    // PORT: finish the hold before taking another waiting-state physics step.
    // Checking it after the update adds one countdown step when the next
    // rendered frame also contains the next simulation step (e.g. at 30 FPS).
    if ((char)param2 == 0)
        Race_UpdatePlayerViewAndDrivenCars((BYTE)param1->records[0].packedState, 0);
    GameMenu_ClearPauseOverlayLatch();
    if (GameInfo_GetFrontendSessionFlag() == 0) {
        if (CGameInfo::GetConfiguredGameMode() == 12)
            CInput::UpdateInputFrameDelta();
        return;
    }
big:
    CGame::PromoteCallbackEntryByRule(param1, param2, 0, 2);
    StageTiming_ResetClockAndPreviousTime();
    if ((char)RallyData_GetFlag22() != 0 && (char)RallyData_GetFlag25() == 0 &&
        CGameInfo::GetGameModeOptionBit19() == 0) {
        Car_ReloadModels(RallyDataState() & 0xff,
                     Car_GetOrderCount() - (RallyDataState() & 0xff), 0);
        if ((char)RallyData_GetSelectionFlag26() != 0)
            StageObject_StartReplaySession(0);
    }
    if (((char)RallyData_GetSelectionFlag26() != 0 || (char)RallyData_GetSelectionFlag27() != 0) &&
        CGameInfo::GetGameModeOptionBit19() == 0) {
        StageTiming_StorePreviousCarCheckpoints();
        if (CGameInfo::GetConfiguredGameMode() == 4)
            StageTiming_SetStartSelectionIndex(*RallyData_GetChampionshipState() >> 12 & 0xf);
    }
    if (CGameInfo::GetGameModeOptionBit19() != 0) {
        Replay_InitCarStreamState((ReplayStream *)g_playerReplayStreams[0], 0, 0);
        i = 0;
        p = g_playerReplayStreams + 1;
        do {
            if ((char)NetPlayers_IsPlayerPresent(i) != 0)
                Replay_InitCarStreamState((ReplayStream *)*p, 0, NetPlayers_GetPlayerField8(i));
            p++;
            i++;
        } while ((INT_PTR)p < (INT_PTR)(g_playerReplayStreams + 8)); // 0x537f5c in the original
    } else if ((BYTE)RallyDataState() == 1 && (char)RallyData_GetFlag25() != 0) {
        if (CGameInfo::GetConfiguredGameMode() == 4) {
            i = 0;
            p = g_playerReplayStreams;
            do {
                if ((char)Car_GetDrawnFlag(i) == -1) {
                    Replay_ResetBufferIfActive((ReplayStream *)*p);
                    Replay_SelectAndInitializeLane((ReplayStream *)*p, 0, 0, i);
                } else {
                    Replay_InitCarStreamState((ReplayStream *)*p, 0, param2);
                }
                p++;
                i++;
            } while ((INT_PTR)p < (INT_PTR)&g_playerReplayStreams[2]);
        } else if ((char)RallyData_GetFlag25() != 0) {
            for (i = 0; i < ((char)RallyData_GetSelectionFlag28() != 0 ? 1 : 2); i++) {
                if (CGameInfo::IsConfiguredMultiplayer() != 0) {
                    if (i == 0) {
                        Replay_InitCarStreamState((ReplayStream *)g_playerReplayStreams[i], 0, param2);
                    } else {
                        Replay_ResetBufferIfActive((ReplayStream *)g_playerReplayStreams[1]);
                        Replay_SelectAndInitializeLane((ReplayStream *)g_playerReplayStreams[1], 0, 0, i);
                    }
                } else if ((char)RallyData_GetUsableRecordCategory(i) != -1) {
                    Replay_ResetBufferIfActive((ReplayStream *)g_playerReplayStreams[i]);
                    Replay_SelectAndInitializeLane((ReplayStream *)g_playerReplayStreams[i], 0, 0, i);
                } else {
                    Replay_InitCarStreamState((ReplayStream *)g_playerReplayStreams[i], 0, param2);
                }
            }
        }
    } else {
        Replay_InitCarStreamState((ReplayStream *)g_playerReplayStreams[index], 0, param2);
    }
    if ((char)RallyData_GetSelectionFlag28() != 0 && (char)CGameInfo::GetSoundOptionBit30() != 0) {
        Replay_ResetActiveBufferState();
        Replay_RestartGhostCar(0, 0);
    }
    StageObject_StartCarSoundTimer(index);
    if ((char)RallyData_GetSelectionFlag26() != 0) {
        CSound::SetMusicStreamVolume(CGameInfo::GetMasterSoundVolume());
        Sound_StartLoopingMusicStream(1);
    }
}

// Per-driver in-race update of the pre-race countdown: tracks whether the field
// has settled, advances the race-slot bookkeeping and drives the fade-out.
// FUNCTION: CMR2 0x0041d7a0
void Race_UpdateDriverCountdown(CallbackStateMachine *param1, unsigned int param2)
{
    unsigned int index;
    unsigned int cond;
    char flag;
    int i;
    int j;
    int t;

    index = param2 & 0xff;
    StageObject_UpdateCarSoundElapsedTime(index);
    flag = StageTiming_GetCheckpointField1A(index);
    if ((char)RallyData_GetSetupFlag11() != 0 && (char)StageTiming_GetPendingDriverByteA7() != 0)
        flag = 1;
    if ((char)RallyData_GetSelectionFlag27() != 0 && CGameInfo::GetGameModeOptionBit19() == 0 && StageObject_GetCollisionRecordState() != 0) {
        i = StageTiming_GetCheckpointField0(1);
        j = StageTiming_GetCheckpointField0(0);
        if (abs(j - i) < 3) {
            i = StageTiming_GetCarTimingByte81(1);
            j = StageTiming_GetCarTimingByte81(0);
            g_unk0x005191a0 = (j < i);
            if (index == (int)(char)g_unk0x005191a0) {
                StageTiming_RecordDriverTimeoutFinish(index);
                g_unk0x00537fbc[index] = 1;
                flag = 1;
            }
        }
    }
    g_unk0x0053811e = 0;
    g_unk0x0053811f = 0;
    if (CGameInfo::GetGameModeOptionBit19() != 0) {
        if (GameInfo_GetSessionField398C() > -1) {
            if (CGameInfo::GetConfiguredGameMode() != 10) {
                if (CGameInfo::GetConfiguredGameMode() != 12) {
                    if (g_unk0x0053811d != 0) {
                        t = CMain::GetFrameDelta() - g_unk0x00537f30;
                        if ((unsigned int)t > (unsigned int)(GameInfo_GetSessionField398C() * 100))
                            g_unk0x0053811e = 1;
                    } else {
                        for (i = 0; i < 7; i++) {
                            if ((char)NetPlayers_IsPlayerPresent(i) != 0 && (char)NetPlayers_HasPlayerFinished(i) != 0) {
                                g_unk0x0053811d = 1;
                                if (g_unk0x00538120 == 0) {
                                    g_unk0x00538120 = 1;
                                    g_unk0x00537f30 = CMain::GetFrameDelta();
                                }
                            }
                        }
                    }
                }
            }
        }
        if (CGameInfo::GetConfiguredGameMode() == 10 || CGameInfo::GetConfiguredGameMode() == 12) {
            if (CGameInfo::GetConfiguredGameMode() == 12)
                flag = 0;
            t = NetPlayers_GetInputFrameDelta();
            if ((unsigned int)(CMain::GetFrameDelta() - t) > (unsigned int)(GameInfo_GetSessionField397C() * 6000) &&
                Network_GetSessionStateFlag() != 0 && GameInfo_GetSessionField397C() != 0) {
                if (g_unk0x00538100 != 0)
                    goto fade;
                g_unk0x0053811f = 1;
                NetPlayers_SendType16Notification();
                g_unk0x00538100 = 1;
                NetRace_FadeOutPlayerScreen(0, GameMenu_FinishNetworkResultsQuitFade, 1, 0, g_unk0x005191a4, 1);
            }
        }
        if (g_unk0x00538100 != 0) {
fade:
            Race_UpdatePlayerViewAndDrivenCars((BYTE)param1->records[index].packedState, 1);
            return;
        }
    }
    if (flag != 0 || GameInfo_GetFrontendSessionFlag() != 0 || g_unk0x0053811e != 0) {
        if (CGameInfo::GetGameModeOptionBit19() != 0)
            NetPlayers_SendCarClass(3);
        g_unk0x00537f78[4] = g_unk0x00537f78[4] + 1;
        g_unk0x00537f98[index] = 1;
        CarDamage_CaptureStageSnapshot(index);
        g_unk0x00537f34[index] = CMain::GetFrameDelta();
        CGame::PromoteCallbackEntryByRule(param1, param2, 0, 2);
    }
    index = RallyDataState();
    cond = (g_unk0x00537f78[4] == index);
    if (CGameInfo::GetGameModeOptionBit19() != 0 && CGameInfo::GetConfiguredGameMode() == 10)
        cond = (unsigned char)flag;
    if ((char)param2 == 0) {
        Race_UpdatePlayerViewAndDrivenCars((BYTE)param1->records[0].packedState, cond);
        if (cond == 0) {
            if (GameInfo_GetFrontendSessionFlag() == 0)
                goto done;
        }
        if (CGameInfo::GetGameModeOptionBit19() != 0) {
            View_SwitchCamera(0, 7, 0xffff, View_GetActiveCameraFlags(0), 0);
            if (g_unk0x0053811f == 0) {
                NetRace_SendStageAndOverallTimes(g_unk0x0053811e != 0 ? StageTiming_EstimateElapsedStageTime() : StageTiming_GetValidStartTime(0));
                if (g_unk0x00538120 == 0) {
                    if (GameInfo_GetSessionField398C() > -1) {
                        g_unk0x00538120 = 1;
                        g_unk0x00537f30 = CMain::GetFrameDelta();
                    }
                }
                NetPlayers_UpdateBestRaceTime(StageTiming_GetValidStartTime(0));
            }
        }
    }
done:
    if (flag != 0)
        View_SwitchCamera(param2, 7, 0xffff, View_GetActiveCameraFlags(param2), 0);
}

unsigned int RallyData_GetFlag22(void);
unsigned char RallyData_GetSelectionFlag28(void);
int Replay_StopRecording(BYTE *pBuffer);
void ForceFeedback_DeactivateSlots(void);
int NetRace_IsPlayerFadeActive(BYTE index);
void StageTiming_EstimateRemainingDriverTimes(void);
void StageTiming_CommitChampionshipPoints(void);
int StageTiming_GetFinishedDriverCount(void);
unsigned int StageTiming_GetDriverSplitClock(int index, int split);
int NetPlayers_GetStandingCount(void);
unsigned int NetPlayers_GetStandingTime(int index, int total);
char *NetPlayers_GetStandingName(int index, int total);
void NetworkLeaderboard_AddWins(int index, char *name, int wins);
void NetPlayers_BuildStageStandings(unsigned int localTime);
void NetPlayers_UpdateOverallStandings(unsigned int localTime);
void RallyTiming_AwardStagePoints(void);
void NetPlayers_SendPublishedLeaderboard(void);
void GameMenu_SetupStageResults(int value);
int Sound_RestartOpenedMusicStream(void);
void CarSkid_ClearWheelTrails(void);
BYTE StageUI_GetRaceEndEventCount(void);
void StageUI_UpdateDriverEventAwards(void);
char *Race_GetStagePathBuffer38444(void);
void StageTiming_InsertDriverSplitRanking(int driver, int hundredths, int split);
int StageTiming_GetDriverSlot(int iDriver);
int Replay_Save(BYTE *pBuffer, char *pName);
int GetStageSplitCount(void);
BYTE RallyData_IsLastAvailableStageSelected(void);
BYTE RallyData_GetModeStageGroup(BYTE flags, char mode);

// In-race per-driver update: enforces the pre-race hold, saves the stage record
// once every driver is ready and refreshes the leaderboard/knockout tables.
// match 86%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// The stage-record block now reproduces the original's shape: the index is written
// inline at each use (Country/Stage re-called, the 11*country product hoisted across
// the second call), StageTiming_GetValidStartTime(index) is pre-read into a raw temp and the record is
// walked through a pointer to its .value. What is left is the tail's register
// allocation: MSVC6 dedicates EDI to the constant 0 (the original pushes the literal
// 0 in ~14 call arguments) and materialises the two leaderboard comparisons with
// setcc instead of the original's branchy 1/0 (and 3/0) selection — a byte register is
// free here in our build because our function keeps one value less live.
// match 85%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0041db10
void Race_UpdateDriverReadyAndRecordState(CallbackStateMachine *param1, unsigned int param2)
{
    unsigned int index;
    int hold;
    int i;
    BYTE ready;
    unsigned int flag;
    BYTE k;
    BYTE mode;
    BYTE **pp;
    unsigned int v;
    unsigned int t;
    unsigned int *pRecord;

    if (CGameInfo::GetConfiguredGameMode() == 10 &&
        (unsigned int)(CMain::GetFrameDelta() - NetPlayers_GetInputFrameDelta()) >
            (unsigned int)(GameInfo_GetSessionField397C() * 6000) &&
        GameInfo_GetSessionField397C() != 0 && Network_GetSessionStateFlag() != 0 && g_unk0x00538100 == 0) {
        NetPlayers_SendType16Notification();
        g_unk0x00538100 = 1;
        NetRace_FadeOutPlayerScreen(0, GameMenu_FinishNetworkResultsQuitFade, 1, 0, g_unk0x005191a4, 1);
        return;
    }
    if (g_unk0x0053811c < 100)
        g_unk0x0053811c = g_unk0x0053811c + 1;
    hold = 400;
    if ((char)RallyData_GetSelectionFlag26() != 0 && CGameInfo::GetConfiguredPlayerCount() == 2)
        hold = 800;
    index = param2 & 0xff;
    if ((unsigned int)(CMain::GetFrameDelta() - g_unk0x00537f34[index]) >= (unsigned int)hold &&
        g_unk0x00537fd8[index] == 0) {
        g_unk0x00537fd8[index] = 1;
        g_unk0x00537fcc = g_unk0x00537fcc + 1;
        if (CGameInfo::GetGameModeOptionBit19() != 0) {
            Replay_StopRecording(g_playerReplayStreams[0]);
            pp = g_playerReplayStreams + 1;
            do {
                Replay_StopRecording(*pp);
                pp++;
            } while ((INT_PTR)pp < (INT_PTR)(g_playerReplayStreams + 8)); // 0x537f5c in the original
        } else {
            Replay_StopRecording(g_playerReplayStreams[index]);
        }
        flag = (CGameInfo::GetGraphicsOptionBits27To28() == 0);
        if (CGameInfo::GetConfiguredGameMode() == 3 && g_unk0x00537ff8[index] == 0) {
            v = CGameInfo::GetPlayerRecordTable(flag)->rallyStageRecordTimes[
                    (RallyDataCountryIndex() & 0xff) * 0xb + (RallyDataStageIndex() & 0xff)].value >> 7 & 0xffff;
            if ((int)StageTiming_GetValidStartTime(index) <= (int)v) {
                t = StageTiming_GetValidStartTime(index);
                pRecord = &CGameInfo::GetPlayerRecordTable(flag)->rallyStageRecordTimes[
                    (RallyDataCountryIndex() & 0xff) * 0xb + (RallyDataStageIndex() & 0xff)].value;
                *pRecord = (t & 0xffff) << 7 | *pRecord & 0xff80007f;
                k = RallyData_GetDriverRecordSelectionValue(StageUI_GetRaceEndEventCount());
                pRecord = &CGameInfo::GetPlayerRecordTable(flag)->rallyStageRecordTimes[
                    (RallyDataCountryIndex() & 0xff) * 0xb + (RallyDataStageIndex() & 0xff)].value;
                *pRecord = k & 0x3f | *pRecord & 0xffffffc0;
                g_unk0x00538128 = 1;
                g_unk0x00519228 = RallyDataStageIndex() & 0xff;
                g_unk0x0051922c = RallyDataCountryIndex() & 0xff;
                sprintf(CFrontend::m_stringDest, g_strGrp0x005192b0, Race_GetStagePathBuffer38444());
                if (g_playerReplayStreams[index] != 0)
                    Replay_Save(g_playerReplayStreams[index], CFrontend::m_stringDest);
            }
            g_unk0x00537ff8[index] = 1;
        }
        ForceFeedback_DeactivateSlots();
    }
    if ((char)param2 != 0)
        return;
    ready = 0;
    for (k = 0; k < (BYTE)RallyDataState(); k++) {
        if (NetRace_IsPlayerFadeActive(k) != 0)
            ready = ready + 1;
    }
    flag = (g_unk0x00537fcc == (RallyDataState() & 0xff) && ready == 0);
    Race_UpdatePlayerViewAndDrivenCars((BYTE)param1->records[0].packedState, 1);
    if (CGameInfo::GetGameModeOptionBit19() != 0) {
        NetRace_DrainPendingMessages();
        if (CGameInfo::GetConfiguredGameMode() != 10 && CGameInfo::GetConfiguredGameMode() != 12) {
            for (i = 0; i < 7; i++) {
                if ((char)NetPlayers_IsPlayerPresent(i) != 0 && (char)NetPlayers_HasPlayerFinished(i) == 0) {
                    g_unk0x00537f34[0] = CMain::GetFrameDelta();
                    return;
                }
            }
        }
    }
    if (flag == 0 && GameInfo_GetFrontendSessionFlag() == 0) {
        g_unk0x00537f08 = 1;
        return;
    }
    for (k = 0; k < param1->count; k++)
        CGame::PromoteCallbackEntryByRule(param1, k, 0, 2);
    if ((char)RallyData_GetFlag22() != 0 || (char)RallyData_GetFlag24() != 0)
        StageTiming_EstimateRemainingDriverTimes();
    if ((char)RallyData_GetSelectionFlag27() != 0 && CGameInfo::GetGameModeOptionBit19() == 0) {
        if (g_unk0x00537fbc[0] != 0)
            StageTiming_InsertDriverSplitRanking(StageTiming_GetDriverSlot(0), StageTiming_GetValidStartTime(0), GetStageSplitCount());
        if (g_unk0x00537fbc[1] != 0)
            StageTiming_InsertDriverSplitRanking(StageTiming_GetDriverSlot(1), StageTiming_GetValidStartTime(1), GetStageSplitCount());
    }
    if (((char)RallyData_GetFlag22() != 0 || (char)RallyData_GetFlag24() != 0) &&
        (char)RallyData_GetSelectionFlag27() != 0 && CGameInfo::GetGameModeOptionBit19() == 0 &&
        (BYTE)RallyDataState() == 1 && StageTiming_GetFinishedDriverCount() < 2)
        StageTiming_InsertDriverSplitRanking(StageTiming_GetSplitSecondaryEntry(StageUI_GetRaceEndEventCount() & 0xff), StageTiming_GetDriverSplitClock(1, 2),
                     GetStageSplitCount());
    if (CGameInfo::IsConfiguredMultiplayer() != 0 &&
        (StageUI_GetRaceEndEventCount() & 0xff) != ((CGameInfo::GetConfiguredPlayerCount() & 0xff) - 1))
        goto label2;
    if (CGameInfo::GetConfiguredGameMode() != 0 && CGameInfo::GetConfiguredGameMode() != 1 &&
        CGameInfo::GetConfiguredGameMode() != 8)
        goto label2;
    StageTiming_AddToOverall();
    if (CGameInfo::GetConfiguredGameMode() == 0 &&
        RallyDataStageIndex() ==
            (BYTE)RallyData_GetModeStageGroup(RallyDataCountryIndex(), CGameInfo::GetConfiguredDifficulty()))
        RallyTiming_AwardStagePoints();
label2:
    if (CGameInfo::GetConfiguredGameMode() == 5)
        StageTiming_CommitChampionshipPoints();
    if (CGameInfo::IsConfiguredMultiplayer() == 0 ||
        (StageUI_GetRaceEndEventCount() & 0xff) != ((CGameInfo::GetConfiguredPlayerCount() & 0xff) - 1)) {
        if (CGameInfo::IsConfiguredMultiplayer() != 0)
            goto label4;
    }
    if (CGameInfo::GetConfiguredGameMode() == 4 || CGameInfo::GetConfiguredGameMode() == 8 ||
        CGameInfo::GetConfiguredGameMode() == 9 || CGameInfo::GetConfiguredGameMode() == 10 ||
        CGameInfo::GetConfiguredGameMode() == 11 || CGameInfo::GetConfiguredGameMode() == 12)
        goto label4;
    if ((BYTE)CGameInfo::GetActiveCheatMask() != 0) {
        for (i = 0; i < (BYTE)CGameInfo::GetConfiguredPlayerCount(); i++) {
            g_playerEventAwardFlags[i] = 0;
            g_unk0x00537f78[i] = 0;
        }
    } else {
        StageUI_UpdateDriverEventAwards();
    }
label4:
    if (CGameInfo::GetGameModeOptionBit19() != 0) {
        mode = (BYTE)CGameInfo::GetConfiguredGameMode();
        if (mode >= 8 && (mode <= 9 || mode == 11)) {
            NetPlayers_BuildStageStandings(StageTiming_GetValidStartTime(0));
            NetPlayers_UpdateOverallStandings(StageTiming_GetValidStartTime(0));
            if (CNetworkLeaderboards::GetLeaderboardId() != -1 && Network_GetSessionStateFlag() != 0) {
                NetworkLeaderboard_AddWins(CNetworkLeaderboards::GetLeaderboardId(), NetPlayers_GetStandingName(0, 0), 1);
                for (i = 1; i < NetPlayers_GetStandingCount(); i++)
                    NetworkLeaderboard_AddWins(CNetworkLeaderboards::GetLeaderboardId(), NetPlayers_GetStandingName(i, 0),
                                 NetPlayers_GetStandingTime(i, 0) == NetPlayers_GetStandingTime(0, 0));
                if (CGameInfo::GetConfiguredGameMode() == 8 && (char)RallyData_IsLastAvailableStageSelected() != 0) {
                    NetworkLeaderboard_AddWins(CNetworkLeaderboards::GetLeaderboardId(), NetPlayers_GetStandingName(0, 1), 3);
                    for (i = 1; i < NetPlayers_GetStandingCount(); i++)
                        if (NetPlayers_GetStandingTime(i, 1) == NetPlayers_GetStandingTime(0, 1))
                            NetworkLeaderboard_AddWins(CNetworkLeaderboards::GetLeaderboardId(), NetPlayers_GetStandingName(i, 1), 3);
                        else
                            NetworkLeaderboard_AddWins(CNetworkLeaderboards::GetLeaderboardId(), NetPlayers_GetStandingName(i, 1), 0);
                }
            }
        }
    }
    if (CGameInfo::GetGameModeOptionBit19() != 0 && Network_GetSessionStateFlag() != 0 &&
        CNetworkLeaderboards::GetLeaderboardId() != -1)
        NetPlayers_SendPublishedLeaderboard();
    GameMenu_SetupStageResults(0);
    if ((char)RallyData_GetSelectionFlag28() != 0 && (char)CGameInfo::GetSoundOptionBit30() != 0)
        Replay_ResetActiveBufferState();
    CarSkid_ClearWheelTrails();
    if ((char)RallyData_GetSelectionFlag26() != 0)
        Sound_RestartOpenedMusicStream();
    g_unk0x00537f08 = 1;
}

int Replay_SelectAndInitializeLane(ReplayStream *p, short lane, short start, BYTE car);
int Replay_ResetBufferIfActive(ReplayStream *p);
int NetPlayers_GetPlayerField8(int index);
void View_ResetCameras(int player);
void Replay_ResetActiveBufferState(void);
void Replay_RestartGhostCar(int, int);

// Starts the replays of a race: the player's own, every network player's, the
// knockout opponent's and the CPU runs of a head-to-head stage.
// FUNCTION: CMR2 0x0041e220
void Race_StartPlayerAndOpponentReplays(int player)
{
    int i;

    Replay_SelectAndInitializeLane((ReplayStream *)g_playerReplayStreams[(BYTE)player], 0, 0, player);
    if (CGameInfo::GetGameModeOptionBit19()) {
        for (i = 0; i < 7; i++) {
            Replay_ResetBufferIfActive((ReplayStream *)g_playerReplayStreams[i + 1]);
            Replay_SelectAndInitializeLane((ReplayStream *)g_playerReplayStreams[i + 1], 0, 0, NetPlayers_GetPlayerField8(i));
        }
    }
    if (CGameInfo::GetConfiguredGameMode() == 4) {
        for (i = 0; i < 2; i++) {
            if ((char)Car_GetDrawnFlag(i) == -1) {
                Replay_ResetBufferIfActive((ReplayStream *)g_playerReplayStreams[i]);
                Replay_SelectAndInitializeLane((ReplayStream *)g_playerReplayStreams[i], 0, 0, i);
            }
        }
    } else if ((char)RallyData_GetFlag25() && CGameInfo::GetGameModeOptionBit19() == 0) {
        for (i = 0; i < ((char)RallyData_GetSelectionFlag28() ? 1 : 2); i++) {
            if (RallyData_GetUsableRecordCategory((BYTE)i) != -1 || (CGameInfo::IsConfiguredMultiplayer() && i == 1)) {
                Replay_ResetBufferIfActive((ReplayStream *)g_playerReplayStreams[i]);
                Replay_SelectAndInitializeLane((ReplayStream *)g_playerReplayStreams[i], 0, 0, i);
            }
        }
    }
    View_ResetCameras(player);
    if ((char)RallyData_GetSelectionFlag28() && (char)CGameInfo::GetSoundOptionBit30()) {
        Replay_ResetActiveBufferState();
        Replay_RestartGhostCar(0, 0);
    }
}

// Waits for the inter-stage fade of a special stage (mode 10) and otherwise
// re-arms the per-player fade jobs of the in-race menu.
// FUNCTION: CMR2 0x0041e350
void Race_UpdateInterStageFadeJobs(CallbackStateMachine *param1, unsigned int param2)
{
    BYTE colour[4];
    int anyAlive;
    BYTE i;
    int flag;
    int x;
    int start;

    colour[0] = 0;
    colour[1] = 0;
    colour[2] = 0;
    colour[3] = 0;
    if (CGameInfo::GetConfiguredGameMode() == 10) {
        start = NetPlayers_GetInputFrameDelta();
        if ((unsigned int)(CMain::GetFrameDelta() - start) >
                (unsigned int)(GameInfo_GetSessionField397C() * 6000) &&
            Network_GetSessionStateFlag() != 0 && GameInfo_GetSessionField397C() != 0) {
            if (g_unk0x00538100 == 0) {
                NetPlayers_SendType16Notification();
                g_unk0x00538100 = 1;
                NetRace_FadeOutPlayerScreen(0, GameMenu_FinishNetworkResultsQuitFade, 1, 0, g_unk0x005191a4, 1);
                return;
            }
            goto fadeEarly;
        }
        if (g_unk0x00538100 != 0)
            goto fadeEarly;
    }
    NetRace_SetRaceControlFlag(0);
    if (GameInfo_GetFrontendSessionFlag() != 0 || g_unk0x0053811f != 0)
        GameMenu_LeaveStageAndAdvanceChampionship();
    if ((char)param2 != 0)
        return;
    anyAlive = 1;
    if ((BYTE)RallyData_IsChampionshipFinalStage() != 0 && g_playerReplayStreams[0] != 0) {
        if (g_unk0x00538118 == 0) {
            if (((ReplayStream *)g_playerReplayStreams[0])->playing != 0)
                goto done;
            View_SwitchCamera(0, 7, 0xffff, View_GetActiveCameraFlags(0), 0);
            g_unk0x00538118 = 1;
            Replay_ResetBufferIfActive((ReplayStream *)g_playerReplayStreams[0]);
            Replay_SelectAndInitializeLane((ReplayStream *)g_playerReplayStreams[0], 0, 0, 0);
        }
        if (((ReplayStream *)g_playerReplayStreams[0])->playing == 0) {
            StageObject_SpawnRandomDebrisBurst();
            GameMenu_StartPauseOverlayFade();
        }
        goto done;
    }
    i = 0;
    if ((BYTE)RallyDataState() > 0) {
        do {
            if (Race_GetPlayerRecordField4(i) != 0)
                anyAlive = 0;
            i++;
        } while (i < (BYTE)RallyDataState());
        if (anyAlive == 0)
            goto done;
    }
    i = 0;
    if ((BYTE)RallyDataState() > 0) {
        do {
            flag = 3;
            if (g_unk0x00537fd8[i] != 0) {
                flag = 2;
                g_unk0x00537fd8[i] = 0;
            }
            NetRace_StartPlayerFade(i, 0xc8000, (FadeCallback)Race_StartPlayerAndOpponentReplays, flag, 2, 0, *(unsigned int *)colour, 0);
            i++;
        } while (i < (BYTE)RallyDataState());
    }
done:
    Race_UpdatePlayerViewAndDrivenCars((BYTE)param1->records[0].packedState, 1);
    if (CGameInfo::GetGameModeOptionBit19() != 0) {
        NetRace_DrainPendingMessages();
        if (CGameInfo::GetConfiguredGameMode() == 10 || CGameInfo::GetConfiguredGameMode() == 12)
            NetPlayers_BuildFinalClassification();
        for (x = 0; x < 7; x++) {
            if ((BYTE)NetPlayers_IsPlayerPresent(x) != 0 && (BYTE)NetPlayers_HasPlayerFinished(x) == 0)
                return;
        }
    }
    g_unk0x00537f08 = 1;
    return;
fadeEarly:
    Race_UpdatePlayerViewAndDrivenCars((BYTE)param1->records[param2 & 0xff].packedState, 1);
}

// Releases the race resources once (sounds, callbacks, textures).
// FUNCTION: CMR2 0x0041e670
void Race_ReleaseFrameResources(void)
{
    g_unk0x00537fc0 = 1;
    Sound_FreeAll();
    if (g_raceResourcesFreed == 0) {
        CGame::UnwindCallbacks(g_unk0x00537f5c);
        CGraphics::EvictManagedTextureResources();
        CGraphics::FreeTextureBuffers();
        g_raceResourcesFreed = 1;
    }
}

BYTE *GameMenu_GetChampionshipTransitionState(void);
void StageLights_SetTransform(FixVector *pAxes);
void StageTiming_IndexSerializedStageTables(BYTE *pData);

void NetPlayers_UpdateLocalSplitRecords(int splitCount);
int NetRace_GetLastSentSplit(void);
void Frontend_MergeMenuKeysIntoPlayerState(void);
void StageUI_ClearRaceEndLatch(void);
void StageUI_ResetRaceEndEventCount(void);
void StageUI_RecordRaceEndEvent(char bFlag);
void RallyTiming_AddStagePenalties(void);
void RallyData_SetStageSelectionAndRefreshFlags(BYTE param1);
void RallyData_ClearDriverGroupRecords(void);
void RallyData_CopyCountryPlayerDefaults(void);
void RallyData_SelectFirstAvailableStage(void);
void NetPlayers_ResetBestTimes(void);
void NetPlayers_ResetRaceReadyAndTimeState(char resetTotal, char resetTimes);
void RallyData_SetSelectionBits12To13(BYTE param1);
void RallyData_InitKnockoutBracket(void);
void Knockout_ResetRaceStateMachine(void);
void Race_FreeSoundsAndUnwindCallbacks(void);
void StageSound_ResetRecordsAndRegisterCleanup(void);
void Frontend_AccumulateDeviceKeyCounters(void);
void CarDamage_ExportStageSnapshotToRallyRecord(int index);
BYTE RallyData_AdvanceSelectedStage(void);
void Game_SetOptionStateByte(BYTE param1);
bool RallyData_AdvanceRallySelectionPair(void);
int Knockout_GetRoundActiveFlag(void);
void Knockout_ClearRoundActiveFlag(void);
BYTE Knockout_ClearChampionshipPendingFlag(void);
void NetPlayers_ResetStageState(char keepReady, char resetTotal);
void Replay_SwapPendingSlotValue(int **pValue, int slot, int flag);

void NetRace_ResetStateAndTriangleTable(void);
void NetPlayers_ResetStatisticsSequences(void);
void NetPlayers_ResetStageState(char keepReady, char resetTotal);
void Race_ResetPlayerCallState(void);
void StageLights_Off(void);
void RallyData_RestoreAllCarRaceRecords(void);
unsigned int RallyData_GetFlag31(void);
void StageTiming_PlaceEventStartingGrid(char param_1);
short Car_GetOrderCount(void);
void Car_ReloadModels(int, int, int);
Car *Car_Get(int index);
void RallyData_FindNearestCarRouteNode(Car *pCar);
void Stage_RestoreCarsToRoutePositions(void);
void View_ResetCameras(int view);
void StageTiming_InitRaceDriverRecords(char);
void StageTiming_ResetParticipatingDriverRecords(int param_1);
void RallyData_ResetStageClockAndRecords(int keepName);
void CarSkid_ClearWheelTrails(void);
void RallyData_MarkAllElementsReached(void);
void StageObject_StartReplaySession(char restart);
void StageTiming_ResetRaceDriverTimes(void);
int Replay_ResetBufferIfActive(ReplayStream *p);
int Replay_StopRecording(BYTE *pBuffer);
void Replay_ResetActiveBufferState(void);
void Replay_SwapPendingSlotValue(int **pValue, int slot, int flag);
void ForceFeedback_ActivateIdleSlots(void);
void Frontend_MergeMenuKeysIntoPlayerState(void);
void StageUI_ResetRaceEndEventCount(void);
extern BYTE g_unk0x00538100;
extern BYTE g_unk0x0053811d;
extern BYTE g_unk0x0053811e;
extern BYTE g_unk0x0053811f;
extern BYTE g_unk0x00538120;
extern int g_unk0x00537f5c;

// Tears the current stage down: resets the race flags, stops the stage lights,
// releases the view slots and replays of every car and refreshes the HUD.
// match 90%: the original keeps the constant 0 in EBX for its ~12 uses; that
// only happens once Replay_SwapPendingSlotValue takes an int flag (pushed as
// a full register). What is left is the placement of the n = 2 block.
// FUNCTION: CMR2 0x0041e6b0
void Race_TeardownStage(CallbackStateMachine *param1, int param2, char flag)
{
    BYTE **pp;
    int n;
    int i;
    int zero = 0;

    g_unk0x00538100 = zero;
    NetRace_ResetStateAndTriangleTable();
    NetPlayers_ResetStatisticsSequences();
    g_unk0x00538120 = zero;
    g_unk0x0053811f = zero;
    g_unk0x0053811e = zero;
    g_unk0x0053811d = zero;
    NetPlayers_ResetStageState(1, 1);
    Race_ResetPlayerCallState();
    StageLights_Off();
    RallyData_RestoreAllCarRaceRecords();
    if (RallyData_GetSelectionFlag26() || RallyData_GetFlag25() || (BYTE)RallyData_GetFlag31())
        if (CGameInfo::GetConfiguredGameMode() == 4)
            StageTiming_PlaceEventStartingGrid(zero);
        else
            StageTiming_PlaceEventStartingGrid(1);
    Car_ReloadModels(zero, (int)Car_GetOrderCount(), zero);
    if (CGameInfo::GetConfiguredGameMode() == 4) {
        for (i = 0; i < Car_GetOrderCount(); i++)
            RallyData_FindNearestCarRouteNode(Car_Get(i));
    }
    Stage_RestoreCarsToRoutePositions();
    if (flag != zero) {
        for (i = 0; i < param1->count; i++) {
            CGame::PromoteCallbackEntryByRule(param1, i, 4, 2);
            View_ResetCameras(i);
            RallyData_ValidateIndex(i);
        }
    }
    StageTiming_InitRaceDriverRecords(zero);
    StageTiming_ResetParticipatingDriverRecords(1);
    RallyData_ResetStageClockAndRecords(g_unk0x0053810c & 0xff);
    CarSkid_ClearWheelTrails();
    RallyData_MarkAllElementsReached();
    if (RallyData_GetSelectionFlag26())
        StageObject_StartReplaySession(1);
    StageTiming_ResetRaceDriverTimes();
    if (CGameInfo::GetGameModeOptionBit19()) {
        n = 1;
        pp = g_playerReplayStreams + 1;
        do {
            Replay_ResetBufferIfActive((ReplayStream *)*pp);
            Replay_StopRecording(*pp);
            pp++;
        } while ((INT_PTR)pp < (INT_PTR)(g_playerReplayStreams + 8)); // 0x537f5c in the original
    } else if (CGameInfo::GetConfiguredGameMode() == 4 || (char)RallyData_GetFlag25()) {
        n = 2;
    } else {
        n = (BYTE)RallyDataState();
    }
    if (n > zero) {
        pp = g_playerReplayStreams;
        do {
            Replay_ResetBufferIfActive((ReplayStream *)*pp);
            Replay_StopRecording(*pp);
            pp++;
        } while (--n);
    }
    if ((char)RallyData_GetSelectionFlag28() && (char)CGameInfo::GetSoundOptionBit30())
        Replay_ResetActiveBufferState();
    if ((char)RallyData_GetSelectionFlag28() && (char)CGameInfo::GetSoundOptionBit30()) {
        for (i = 0; i < (BYTE)RallyDataState(); i++)
            Replay_SwapPendingSlotValue((int **)g_playerReplayStreams + i, i, (BYTE)param2 + i);
    }
    ForceFeedback_ActivateIdleSlots();
    CGameInfo::SetInputAndGamePaused(0);
    if (CGameInfo::GetConfiguredGameMode() != 4) {
        Frontend_MergeMenuKeysIntoPlayerState();
        StageUI_ResetRaceEndEventCount();
    }
    g_unk0x0053810c = zero;
}

// In-race mode/state handler: stage teardown, replay and view transitions.
// Keep the player list separate from the timer query's output slot, preserve
// the saved phase byte, and share the original stage-dependent restore exits.
// Differential fixtures cover every entry of the original five jump tables;
// teardown/replay/view/sound actions are controlled boundaries in those tests.
// FUNCTION: CMR2 0x0041e8d0
void Race_HandleStageReplayViewTransitions(CallbackStateMachine *param1, unsigned int param2)
{
    int i;
    int n;
    int skip = 0;
    int count;
    BYTE *p;
    BYTE **pp;
    CallbackStateMachine *pPlayers = param1;

    count = pPlayers->count;
    if (count >= 1) {
        p = (BYTE *)(pPlayers)->records;
        for (i = 0; i < count; i++) {
            if (*p != 11)
                return;
            p += 8;
        }
    }
    if ((char)param2 != 0)
        return;
    if (CGameInfo::IsConfiguredMultiplayer() == 0)
        CGraphics::SetClearColour(1, 0x9c, 0xb4, 0xac);
    if ((char)RallyData_GetSelectionFlag26() != 0)
        CSound::StopDirectSoundBuffer();
    if (CGameInfo::GetConfiguredGameMode() == 10)
        NetPlayers_UpdateLocalSplitRecords(NetRace_GetLastSentSplit());
    Frontend_MergeMenuKeysIntoPlayerState();
    InRaceMenu_ResetAndBuildPages();
    g_unk0x00537f08 = 1;
    *(BYTE *)&param2 = StageUI_GetRaceEndEventCount();
    if (g_unk0x0053810c != 0) {
        StageUI_ResetRaceEndEventCount();
        switch (CGameInfo::GetConfiguredGameMode()) {
        case 0:
            if (RallyDataStageIndex() ==
                (BYTE)RallyData_GetModeStageGroup(RallyDataCountryIndex(), CGameInfo::GetConfiguredDifficulty()))
                RallyTiming_AddStagePenalties();
            /* fall through */
        case 1:
            CGame::SetFrontendResourceMode(2);
            RallyData_SetStageSelectionAndRefreshFlags(0);
            for (i = 0; i < pPlayers->count; i++)
                CGame::PromoteCallbackEntryByRule(pPlayers, i, 0, 3);
            RallyData_ClearDriverGroupRecords();
            RallyData_CopyCountryPlayerDefaults();
            Race_ReleaseFrameResources();
            StageUI_ClearRaceEndLatch();
            return;
        case 8:
            CGame::SetFrontendResourceMode(2);
            RallyData_SelectFirstAvailableStage();
            for (i = 0; i < pPlayers->count; i++)
                CGame::PromoteCallbackEntryByRule(pPlayers, i, 0, 3);
            RallyData_ClearDriverGroupRecords();
            RallyData_CopyCountryPlayerDefaults();
            Race_ReleaseFrameResources();
            StageUI_ClearRaceEndLatch();
            NetPlayers_ResetBestTimes();
            NetPlayers_ResetRaceReadyAndTimeState(1, 1);
            return;
        case 2:
        case 3:
        case 9:
        case 10:
            RallyData_ClearDriverGroupRecords();
            if (g_unk0x00537ffa != 0) {
                CGame::SetFrontendResourceMode(2);
                for (i = 0; i < pPlayers->count; i++)
                    CGame::PromoteCallbackEntryByRule(pPlayers, i, 0, 3);
                Race_ReleaseFrameResources();
                StageUI_ClearRaceEndLatch();
                NetPlayers_ResetRaceReadyAndTimeState(1, 0);
                return;
            }
            if (StageUI_GetRaceResultValue() == 4) {
                Race_TeardownStage(pPlayers, param2, 0);
                Race_FreeSoundsAndUnwindCallbacks();
                Sound_FreeAll();
                StageSound_ResetRecordsAndRegisterCleanup();
                CGame::PromoteCallbackEntryByRule(pPlayers, 0, 3, 2);
                return;
            }
            break;
        case 4:
            RallyData_ClearDriverGroupRecords();
            Race_ResetStateByteToFF();
            if (Knockout_GetRoundActiveFlag() == 0) {
                for (i = 0; i < pPlayers->count; i++)
                    CGame::PromoteCallbackEntryByRule(pPlayers, i, 0, 3);
                Race_ReleaseFrameResources();
                StageUI_ClearRaceEndLatch();
                RallyData_InitKnockoutBracket();
                Knockout_ResetRaceStateMachine();
                return;
            }
            break;
        case 5:
            RallyData_SetSelectionBits12To13(0);
            for (i = 0; i < pPlayers->count; i++)
                CGame::PromoteCallbackEntryByRule(pPlayers, i, 0, 3);
            RallyData_ClearDriverGroupRecords();
            Race_ReleaseFrameResources();
            StageUI_ClearRaceEndLatch();
            return;
        }
        Race_TeardownStage(pPlayers, param2, 1);
        if (CGameInfo::GetConfiguredGameMode() != 4)
            return;
        if (Knockout_GetRoundActiveFlag() == 0)
            return;
    }
    if (g_unk0x0053810d != 0) {
        if ((char)RallyData_GetSelectionFlag26() != 0)
            CSound::CloseMusicStreamAndClearPath(0);
        if (CGameInfo::GetGameInfoSessionFlag() == 0) {
            CGameInfo::GetRecordFlagsWord((unsigned int **)&param1);
            n = CMain::GetFrameDelta();
            *(int *)param1 += n - g_unk0x00537ffc;
            g_unk0x00537ffc = CMain::GetFrameDelta();
        }
        for (i = 0; i < (BYTE)RallyDataState(); i++)
            CarDamage_ExportStageSnapshotToRallyRecord(i);
        CarDamage_ExportStageSnapshotToRallyRecord(0);
        if (CGameInfo::GetConfiguredGameMode() != 4)
            Frontend_AccumulateDeviceKeyCounters();
        if (CGameInfo::GetConfiguredGameMode() != 4) {
            for (i = 0; i < pPlayers->count; i++)
                CGame::PromoteCallbackEntryByRule(pPlayers, i, 0, 2);
        } else {
            if (Knockout_ClearChampionshipPendingFlag() != 0) {
                for (i = 0; i < pPlayers->count; i++)
                    CGame::PromoteCallbackEntryByRule(pPlayers, i, 0, 2);
                goto L_teardown;
            }
            Race_FreeSoundsAndUnwindCallbacks();
            Sound_FreeAll();
            StageSound_ResetRecordsAndRegisterCleanup();
            for (i = 0; i < pPlayers->count; i++)
                CGame::PromoteCallbackEntryByRule(pPlayers, i, 7, 2);
            g_unk0x0053810d = 0;
            return;
        }
L_teardown:
        CGame::SetFrontendResourceMode(0);
        Race_ReleaseFrameResources();
        StageUI_ClearRaceEndLatch();
        g_unk0x0053810d = 0;
        return;
    }
    switch (StageUI_GetRaceResultValue()) {
    case 0:
        switch (CGameInfo::GetConfiguredGameMode()) {
        case 0:
        case 1:
            for (i = 0; i < pPlayers->count; i++)
                CGame::PromoteCallbackEntryByRule(pPlayers, i, 0, 2);
            if ((char)RallyData_AdvanceSelectedStage() != 0) {
                if ((char)RallyDataStageIndex() != 0)
                    goto L_stage_restore;
                Game_SetOptionStateByte(1);
            }
            goto L_teardown2;
        case 8:
            for (i = 0; i < pPlayers->count; i++)
                CGame::PromoteCallbackEntryByRule(pPlayers, i, 0, 2);
            if ((char)RallyData_AdvanceSelectedStage() != 0)
                CGame::SetFrontendResourceMode(2);
            else
                CGame::SetFrontendResourceMode(0);
            NetPlayers_ResetStageState(0, 0);
            Race_ReleaseFrameResources();
            StageUI_ClearRaceEndLatch();
            NetPlayers_ResetRaceReadyAndTimeState(0, 0);
            return;
        case 2:
        case 9:
        case 10:
        case 11:
        case 12:
            for (i = 0; i < pPlayers->count; i++)
                CGame::PromoteCallbackEntryByRule(pPlayers, i, 0, 2);
            goto L_teardown2;
        case 3:
            for (i = 0; i < pPlayers->count; i++)
                CGame::PromoteCallbackEntryByRule(pPlayers, i, 0, 2);
            goto L_teardown2;
        case 5:
            for (i = 0; i < pPlayers->count; i++)
                CGame::PromoteCallbackEntryByRule(pPlayers, i, 0, 2);
            if (RallyData_AdvanceRallySelectionPair())
                goto L_restore;
            goto L_teardown2;
        case 6:
            for (i = 0; i < pPlayers->count; i++)
                CGame::PromoteCallbackEntryByRule(pPlayers, i, 0, 2);
            goto L_teardown2;
        case 7:
            for (i = 0; i < pPlayers->count; i++)
                CGame::PromoteCallbackEntryByRule(pPlayers, i, 0, 2);
            goto L_teardown2;
        }
        break;
    case 1:
        switch (CGameInfo::GetConfiguredGameMode()) {
        case 0:
        case 1:
            for (i = 0; i < pPlayers->count; i++)
                CGame::PromoteCallbackEntryByRule(pPlayers, i, 0, 2);
            if ((char)RallyData_AdvanceSelectedStage() != 0) {
                if ((char)RallyDataStageIndex() != 0)
                    goto L_stage_restore;
                Game_SetOptionStateByte(1);
            }
            goto L_teardown2;
        case 2:
            for (i = 0; i < pPlayers->count; i++)
                CGame::PromoteCallbackEntryByRule(pPlayers, i, 0, 2);
            if (pPlayers->count != 1)
                return;
            goto L_teardown2;
        case 5:
            for (i = 0; i < pPlayers->count; i++)
                CGame::PromoteCallbackEntryByRule(pPlayers, i, 0, 2);
            if (RallyData_AdvanceRallySelectionPair())
                goto L_restore;
            goto L_teardown2;
        case 6:
            for (i = 0; i < pPlayers->count; i++)
                CGame::PromoteCallbackEntryByRule(pPlayers, i, 0, 2);
            goto L_teardown2;
        }
        break;
    case 4:
        switch (CGameInfo::GetConfiguredGameMode()) {
        case 0:
        case 1:
            if ((BYTE)StageUI_GetRaceEndEventCount() + 1 == (BYTE)CGameInfo::GetConfiguredPlayerCount()) {
                CGame::PromoteCallbackEntryByRule(pPlayers, 0, 0, 2);
                if ((char)RallyData_AdvanceSelectedStage() != 0) {
                    if ((char)RallyDataStageIndex() != 0)
                        goto L_stage_restore;
                    Game_SetOptionStateByte(1);
                }
                goto L_teardown2;
            }
            Race_FreeSoundsAndUnwindCallbacks();
            Sound_FreeAll();
            if ((char)RallyData_GetSelectionFlag28() == 0)
                skip = 1;
            StageSound_ResetRecordsAndRegisterCleanup();
            CGame::PromoteCallbackEntryByRule(pPlayers, 0, 3, 2);
            StageUI_RecordRaceEndEvent(0);
            if (skip != 0)
                return;
            break;
        case 2:
            if ((BYTE)StageUI_GetRaceEndEventCount() + 1 == (BYTE)CGameInfo::GetConfiguredPlayerCount())
                goto L_winner;
            CGame::PromoteCallbackEntryByRule(pPlayers, 0, 3, 2);
            Race_FreeSoundsAndUnwindCallbacks();
            Sound_FreeAll();
            StageSound_ResetRecordsAndRegisterCleanup();
            StageUI_RecordRaceEndEvent(0);
            return;
        case 3:
            if (!((BYTE)StageUI_GetRaceEndEventCount() + 1 != (BYTE)CGameInfo::GetConfiguredPlayerCount())) {
                goto L_winner;
            } else {
                Race_FreeSoundsAndUnwindCallbacks();
                Sound_FreeAll();
                StageSound_ResetRecordsAndRegisterCleanup();
                CGame::PromoteCallbackEntryByRule(pPlayers, 0, 3, 2);
                StageUI_RecordRaceEndEvent(0);
            }
            break;
        }
        break;
    case 2:
    case 3:
        for (i = 0; i < pPlayers->count; i++) {
            if (Knockout_GetRoundActiveFlag() == 0)
                CGame::PromoteCallbackEntryByRule(pPlayers, i, 0, 2);
        }
        if (CGameInfo::GetConfiguredGameMode() == 4)
            Knockout_ClearRoundActiveFlag();
        break;
    }
    n = RallyDataState() & 0xff;
    if (CGameInfo::GetConfiguredGameMode() == 4 || (char)RallyData_GetFlag25() != 0)
        n = 2;
    if (n > 0) {
        pp = g_playerReplayStreams;
        i = n;
        do {
            Replay_ResetBufferIfActive((ReplayStream *)*pp);
            Replay_StopRecording(*pp);
            pp++;
        } while (--i);
    }
    if (CGameInfo::GetGameModeOptionBit19() != 0 && n < 8) {
        pp = g_playerReplayStreams + n;
        do {
            Replay_ResetBufferIfActive((ReplayStream *)*pp);
            Replay_StopRecording(*pp);
            pp++;
        } while ((INT_PTR)pp < (INT_PTR)(g_playerReplayStreams + 8)); // 0x537f5c in the original
    }
    if ((char)RallyData_GetSelectionFlag28() != 0 && (char)CGameInfo::GetSoundOptionBit30() != 0)
        Replay_ResetActiveBufferState();
    if ((char)RallyData_GetSelectionFlag28() == 0 || (char)CGameInfo::GetSoundOptionBit30() == 0)
        return;
    for (i = 0; i < (BYTE)RallyDataState(); i++)
        Replay_SwapPendingSlotValue((int **)g_playerReplayStreams + i, i, (BYTE)param2 + i);
    return;
L_stage_restore:
    if (RallyDataStageIndex() == 2 || RallyDataStageIndex() == 4 ||
        RallyDataStageIndex() == 6 || RallyDataStageIndex() == 8 || RallyDataStageIndex() == 10)
        CGame::SetFrontendResourceMode(2);
L_restore:
    Race_ReleaseFrameResources();
    StageUI_ClearRaceEndLatch();
    return;
L_winner:
    CGame::PromoteCallbackEntryByRule(pPlayers, 0, 0, 2);
L_teardown2:
    CGame::SetFrontendResourceMode(0);
    Race_ReleaseFrameResources();
    StageUI_ClearRaceEndLatch();
}

// GLOBAL: CMR2 0x005196e8
char g_strBspFormat[] = "%s.bsp";
// GLOBAL: CMR2 0x00519498
char g_strHpcFormat[] = "%s.hpc";

// GLOBAL: CMR2 0x005194a0
char g_strSrfFormat[] = "%s.srf";

// Loads the stage's surface list (.srf) and marks the sound groups it uses.
// FUNCTION: CMR2 0x0041fcd0
void Race_LoadSurfaceList(void)
{
    DWORD size = 0;
    BYTE *pData;

    sprintf(CFrontend::m_stringDest, g_strSrfFormat, GameMenu_GetChampionshipTransitionState());
    pData = (BYTE *)CGenericFileLoader::FindFile((GenericFile *)StageTiming_GetStageFile3(),
                                                 CFrontend::m_stringDest, 0, &size, 0);
    if (pData != NULL) {
        Race_MarkUsedSurfaceSoundGroups(pData, size);
        return;
    }
    Race_MarkUsedSurfaceSoundGroups(NULL, 0);
}

// Loads the stage's .bsp (stage light placement).
// FUNCTION: CMR2 0x00420020
void Race_LoadStageLightPlacement(void)
{
    sprintf(CFrontend::m_stringDest, g_strBspFormat, GameMenu_GetChampionshipTransitionState());
    StageLights_SetTransform((FixVector *)CGenericFileLoader::FindFile((GenericFile *)StageTiming_GetStageFile3(),
                                                                     CFrontend::m_stringDest, 0, 0, 0));
}

// Loads the stage's .hpc data when present.
// FUNCTION: CMR2 0x0041fc90
void Race_LoadHPCData(void)
{
    BYTE *pData;

    sprintf(CFrontend::m_stringDest, g_strHpcFormat, GameMenu_GetChampionshipTransitionState());
    pData = (BYTE *)CGenericFileLoader::FindFile((GenericFile *)StageTiming_GetStageFile3(), CFrontend::m_stringDest,
                                                 0, 0, 0);
    if (CurvedRoad_Contact()) pData=(BYTE*)CurvedRoad_Contact();
    if (pData != NULL)
        StageTiming_IndexSerializedStageTables(pData);
}

int Sound_IsPlaying(unsigned int handle);
int Sound_PlaySampleWithParameters(unsigned short id, int volume, int frequency, int loopStart, int loops, int is3D);
extern int g_coDriverCallState;
void Sound_SetPlayingSlotVolume(unsigned int handle, int volume);
void Sound_Free(unsigned int handle);

// Moves sound slot src of a car's sound set to slot dst.
// FUNCTION: CMR2 0x00418e20
void Race_MoveCarSoundSlot(int set, int dst, int src)
{
    g_carSoundSets[set].id[dst] = g_carSoundSets[set].id[src];
    g_carSoundSets[set].handle[dst] = g_carSoundSets[set].handle[src];
    g_carSoundSets[set].handle[src] = -1;
    g_carSoundSets[set].id[src] = -1;
}

// Car speed as a 16.16 fraction of 120 (speed units clamped to 0..120).
// FUNCTION: CMR2 0x00418e70
int Race_GetNormalizedCarSpeed(int car)
{
    int speed = FixMul(Car_Get(car)->speed, 0x431168) >> 16;

    if (speed < 0)
        speed = 0;
    else if (speed > 120)
        speed = 120;
    return FixDiv((int)FTOI(__int64, X87S_MUL(speed, CGraphics::m_65536)), 0x780000);
}

// Silences every stage sound still playing.
// FUNCTION: CMR2 0x00418ee0
void Race_StopAllStageSounds(void)
{
    CarSoundSet *pSet;
    int *pHandle;
    int i;

    pSet = g_carSoundSets;
    do {
        pHandle = pSet->handle;
        for (i = 10; i != 0; i--) {
            if (Sound_IsPlaying(*pHandle) != 0)
                Sound_SetPlayingSlotVolume(*pHandle, 0);
            pHandle++;
        }
        pSet++;
    } while ((INT_PTR)pSet < (INT_PTR)&g_carSoundSets[8]);
}

// Loads a sound sample by the name held in the caller's buffer, reading it
// from the current stage file (the same file the stage timing code uses).
// FUNCTION: CMR2 0x00418760
void Race_LoadNamedStageSample(char *name)
{
    Sound_LoadSample(name, 0, (GenericFile *)StageTiming_GetStageFile2());
}

// One sound handle per car.
// GLOBAL: CMR2 0x005373b0
int g_carSounds[8];

// Per-slot sound volumes of each car and the largest of them, the pitch of the
// engine sound just restarted (0x5374c0) and the id of the car's engine
// sample used while a stage sound group is not running.
// GLOBAL: CMR2 0x005374c0
int g_unk0x005374c0;
// GLOBAL: CMR2 0x005374c4
int g_carSlotVolumes[8][4];
// GLOBAL: CMR2 0x00537544
int g_carMaxVolume[8];
// GLOBAL: CMR2 0x00537564
int g_unk0x00537564;

// Per-car-class fixed-point offsets used when the transforms of a car are
// rebuilt (0x42af50): the base one and the extra applied while the class of the
// car changes.
// GLOBAL: CMR2 0x005199c8
int g_unk0x005199c8[14] = {
    -1310, 1310, -655, -2621, 0, 655, 0, 1310,
    655, 655, 0, 0, 1310
};
// GLOBAL: CMR2 0x00519a00
int g_unk0x00519a00[14] = {
    0, -2293, -983, -1638, -3932, -2883, -1638, -2621,
    -5505, -1769, -12910, -2293, -4718, -4718
};

// Frees the per-car sound of every car in the race.
// FUNCTION: CMR2 0x00418780
void Race_FreePerCarSounds(void)
{
    int *p;
    int i;

    i = 0;
    if ((BYTE)RallyDataState() > 0) {
        p = g_carSounds;
        do {
            if (Sound_IsPlaying(*p) != 0) {
                Sound_Free(*p);
                *p = -1;
            }
            i++;
            p++;
        } while (i < (int)(RallyDataState() & 0xff));
    }
}

// FUNCTION: CMR2 0x00420130
void Race_SetResourceStateAndCallbackMark(int value)
{
    g_raceResourceState = value;
    g_raceCallbackMark = CGame::GetCallbackCount();
}

// GLOBAL: CMR2 0x00517ed4
char g_strCodFormat[] = "%s.cod";
// GLOBAL: CMR2 0x00537094
unsigned int *g_loadedCoDriverCalls;

// Loads the stage's co-driver calls (.cod) and resets the call state.
// FUNCTION: CMR2 0x00416720
void Race_LoadCoDriverCallData(void)
{
    CGame::RegisterCallback(Race_ReleaseCoDriverCallData, 0);
    Race_ResetPlayerCallState();
    sprintf(CFrontend::m_stringDest, g_strCodFormat, GameMenu_GetChampionshipTransitionState());
    g_loadedCoDriverCalls = (unsigned int *)CGenericFileLoader::FindFile((GenericFile *)StageTiming_GetStageFile3(), CFrontend::m_stringDest, 0, 0, 0);
    if (g_loadedCoDriverCalls != NULL)
        g_coDriverCalls = g_loadedCoDriverCalls;
}

// GLOBAL: CMR2 0x00537364
int g_unk0x00537364[4];
// GLOBAL: CMR2 0x00537374
int g_unk0x00537374[4];
// GLOBAL: CMR2 0x00537384
int g_unk0x00537384[4];
// GLOBAL: CMR2 0x00537398
int g_unk0x00537398[4];

int NetRace_GetListenerDistanceAttenuation(unsigned int view, int listener);
void Sound_SetPlayingSlotVolume(unsigned int handle, int volume);

// GLOBAL: CMR2 0x005189bc
char g_str0x005189bc[] = "COLLIS6.WAV";
// GLOBAL: CMR2 0x005189c8
char g_str0x005189c8[] = "COLLIS5.WAV";
// GLOBAL: CMR2 0x005189d4
char g_str0x005189d4[] = "COLLIS4.WAV";
// GLOBAL: CMR2 0x005189e0
char g_str0x005189e0[] = "COLLIS3.WAV";
// GLOBAL: CMR2 0x005189ec
char g_str0x005189ec[] = "COLLIS2.WAV";
// GLOBAL: CMR2 0x005189f8
char g_str0x005189f8[] = "COLLIS1.WAV";
// GLOBAL: CMR2 0x00518a04
char g_str0x00518a04[] = "SWIPE4.WAV";
// GLOBAL: CMR2 0x00518a10
char g_str0x00518a10[] = "SWIPE3.WAV";
// GLOBAL: CMR2 0x00518a1c
char g_str0x00518a1c[] = "SWIPE2.WAV";
// GLOBAL: CMR2 0x00518a28
char g_str0x00518a28[] = "SWIPE1.WAV";
// GLOBAL: CMR2 0x005188e4
char *g_collisionSounds[10] = {
    g_str0x00518a28, g_str0x00518a1c, g_str0x00518a10, g_str0x00518a04, g_str0x005189f8, g_str0x005189ec, g_str0x005189e0, g_str0x005189d4, g_str0x005189c8, g_str0x005189bc,
};
// GLOBAL: CMR2 0x00518968
char g_str0x00518968[] = "LAND3.WAV";
// GLOBAL: CMR2 0x00518974
char g_str0x00518974[] = "LAND2.WAV";
// GLOBAL: CMR2 0x00518980
char g_str0x00518980[] = "LAND1.WAV";
// GLOBAL: CMR2 0x0051898c
char g_str0x0051898c[] = "ROLL4.WAV";
// GLOBAL: CMR2 0x00518998
char g_str0x00518998[] = "ROLL3.WAV";
// GLOBAL: CMR2 0x005189a4
char g_str0x005189a4[] = "ROLL2.WAV";
// GLOBAL: CMR2 0x005189b0
char g_str0x005189b0[] = "ROLL1.WAV";
// GLOBAL: CMR2 0x0051890c
char *g_rollSounds[7] = {
    g_str0x005189b0, g_str0x005189a4, g_str0x00518998, g_str0x0051898c, g_str0x00518980, g_str0x00518974, g_str0x00518968,
};
// GLOBAL: CMR2 0x00518938
char g_str0x00518938[] = "MIRROR.WAV";
// GLOBAL: CMR2 0x00518944
char g_str0x00518944[] = "SCREEN1.WAV";
// GLOBAL: CMR2 0x00518950
char g_str0x00518950[] = "HLIGHT2.WAV";
// GLOBAL: CMR2 0x0051895c
char g_str0x0051895c[] = "HLIGHT1.WAV";
// GLOBAL: CMR2 0x00518928
char *g_glassSounds[4] = {
    g_str0x0051895c, g_str0x00518950, g_str0x00518944, g_str0x00518938,
};
// GLOBAL: CMR2 0x00518a34
char g_strCollisionDir[] = "\\collision\\";

int Sound_GetSampleCount(void);
extern int g_collisionFirstSample;
extern int g_glassFirstSample;
extern int g_rollFirstSample;
BOOL Sound_LoadSample(char *name, BYTE flags, GenericFile *pFile);

// Loads the collision sound banks of the stage (swipes and impacts, rolls and
// landings, glass) and resets the per-car sound slots.
// FUNCTION: CMR2 0x004188c0
void Race_LoadCollisionSoundBanks(void)
{
    char path[260];
    char *pDir;
    GenericFile *pFile;
    int i;

    pDir = CInstallInfo::GetSoundsDir();
    pFile = (GenericFile *)StageTiming_GetStageFile0();
    g_collisionFirstSample = Sound_GetSampleCount();
    for (i = 0; i < 10; i++) {
        strcpy(path, pDir);
        strcat(path, g_strCollisionDir);
        strcat(path, g_collisionSounds[i]);
        Sound_LoadSample(path, 0, pFile);
    }
    g_rollFirstSample = Sound_GetSampleCount();
    for (i = 0; i < 7; i++) {
        strcpy(path, pDir);
        strcat(path, g_strCollisionDir);
        strcat(path, g_rollSounds[i]);
        Sound_LoadSample(path, 0, pFile);
    }
    g_glassFirstSample = Sound_GetSampleCount();
    for (i = 0; i < 4; i++) {
        strcpy(path, pDir);
        strcat(path, g_strCollisionDir);
        strcat(path, g_glassSounds[i]);
        Sound_LoadSample(path, 0, pFile);
    }
    for (i = 0; i < 4; i++)
        g_carSounds[i] = -1;
}

// Updates the volume of each player's car sound by distance to its listener.
// FUNCTION: CMR2 0x00418b00
void Race_UpdateCarSoundDistanceVolumes(CallbackStateMachine *p, BYTE index)
{
    int i;

    for (i = 0; i < (BYTE)RallyDataState(); i++) {
        if (g_carSounds[i] != -1) {
            if (Sound_IsPlaying(g_carSounds[i]) != 0)
                Sound_SetPlayingSlotVolume(g_carSounds[i], FixMul(NetRace_GetListenerDistanceAttenuation(g_unk0x00537398[i], g_unk0x00537364[i]),
                                                    FixMul(g_unk0x00537394, g_unk0x00537384[i])));
            else
                g_carSounds[i] = -1;
        }
    }
}

void Input_TranslatePedalsToMenuKeys(void);
struct DeviceInfo;
void Input_MergeAssignedJoystickButtons(int slot, DeviceInfo *pOut);
Menu *StageObject_GetInRaceActionMenu(void);
int Race_AcceptStateTransition(void);

// Update handler of the pause state (state table 0x5190b0): runs the pause
// menu until it closes.
// FUNCTION: CMR2 0x0041f420
// OpenCMR2: 1 while the Esc pause menu state is running. In replay mode (game mode 4) SetInputAndGamePaused stays 1 for the whole
// replay, so that flag alone cannot tell "pause menu open" from "replay playing": the replay camera input uses this one instead.
bool g_racePauseMenuActive = false;
bool Race_PauseMenuActive(void) { return g_racePauseMenuActive; }
void Race_UpdatePauseState(CallbackStateMachine *p, BYTE index)
{
    DeviceInfo *pDev;

    if (g_unk0x00538108 != 0) {
        if (Race_AcceptStateTransition()) {
            g_racePauseMenuActive = false;
            CGameInfo::SetInputAndGamePaused(0);
            CGame::PromoteCallbackEntryByRule(p, index, 0, 2);
            return;
        }
        g_unk0x00538108 = 0;
        return;
    }
    g_racePauseMenuActive = true;
    CGameInfo::SetInputAndGamePaused(1);
    CInput::UpdateAllAvailableDevices();
    // The race logic step (which normally runs the live tuning panel) does not run while paused: tune from here, once per pause tick.
    // Visual settings only; the simulation is not advanced. Panel keys are already filtered out of the keyboard state read above.
    TuningPanel_NoteSite("pause-tick");TuningPanel_Update();
    if (TuningPanel_PreviewActive()) { View_RefreshForTuning(); Race_ApplySpeedEffectParams(); }
    Input_TranslatePedalsToMenuKeys();
    pDev = CInput::GetAvailableDeviceRecord(0);
    Input_MergeAssignedJoystickButtons(0, pDev);
    Menu_Update(StageObject_GetInRaceActionMenu(), pDev->field_0x8);
    if (g_unk0x00537f94 != 0)
        CGame::PromoteCallbackEntryByRule(p, index, 1, 2);
}

// Plays a car sound in a free slot (or the oldest one), with its volume by
// distance to the listener.
// FUNCTION: CMR2 0x004187d0
void Race_PlayDistanceScaledCarSound(unsigned int view, unsigned short id, int volume, int listener)
{
    unsigned int oldest = 0;
    unsigned int now = CMain::GetFrameDelta();
    int slot = -1;
    int i;

    for (i = 0; i < 4; i++) {
        if (g_carSounds[i] == -1) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        slot = 0;
        for (i = 0; i < 4; i++) {
            if (now - g_unk0x00537374[i] > oldest) {
                slot = i;
                oldest = now - g_unk0x00537374[i];
            }
        }
    }
    if (g_carSounds[slot] != -1 && Sound_IsPlaying(g_carSounds[slot]))
        Sound_Free(g_carSounds[slot]);
    g_carSounds[slot] =
        Sound_PlaySampleWithParameters(id, FixMul(NetRace_GetListenerDistanceAttenuation(view, listener), FixMul(g_unk0x00537394, volume)), 0xac44, 0, 0, 0);
    g_unk0x00537364[slot] = listener;
    g_unk0x00537374[slot] = now;
    g_unk0x00537384[slot] = volume;
    g_unk0x00537398[slot] = view;
}

// First sample of the collision, glass and roll sound groups
// (Race_LoadCollisionSoundBanks).
// GLOBAL: CMR2 0x00537360
int g_collisionFirstSample;
// GLOBAL: CMR2 0x005373a8
int g_glassFirstSample;
// GLOBAL: CMR2 0x005373ac
int g_rollFirstSample;

void Race_PlayDistanceScaledCarSound(unsigned int view, unsigned short id, int volume, int listener);

// Raises the car's damage shake level by the strength of a hit.
#define CAR_SHAKE(view, strength)                                         \
    do {                                                                  \
        int level = (FixMul(strength, 0x70000) >> 16) + 1;                \
        if (level > 8)                                                    \
            level = 8;                                                    \
        if (level > Car_Get(view)->shakeLevel)                        \
            Car_Get(view)->shakeLevel = (BYTE)level;                  \
    } while (0)

// Plays a random impact sound (light or heavy set) and shakes the car.
// FUNCTION: CMR2 0x00418c30
void Race_PlayImpactAndShakeCar(unsigned int view, int volume, char heavy, int listener)
{
    if (heavy != 0)
        Race_PlayDistanceScaledCarSound(view, (unsigned short)(rand() % 3 + 4 + g_rollFirstSample), volume, listener);
    else
        Race_PlayDistanceScaledCarSound(view, (unsigned short)(rand() % 4 + g_rollFirstSample), volume, listener);
    CAR_SHAKE(view, volume);
}

// Plays the scrape sound for its strength (10 levels) and shakes the car.
// match 84%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00418ba0
void Race_PlayScrapeAndShakeCar(int view, int strength, int listener)
{
    int level;

    if (strength > 0xccc) {
        level = FixMul(strength, 0xa0000) >> 16;
        if (level >= 10)
            level = 9;
        Race_PlayDistanceScaledCarSound(view, (unsigned short)(g_collisionFirstSample + level), 0x10000, listener);
        {
            // The do/while(0) form of CAR_SHAKE shifts MSVC6's register
            // allocation of the prologue; the original expands it inline.
            int shake = (FixMul(strength, 0x70000) >> 16) + 1;
            if (shake > 8)
                shake = 8;
            if (shake > Car_Get(view)->shakeLevel)
                Car_Get(view)->shakeLevel = (BYTE)shake;
            // OpenCMR2: dirt on the lens from a hard hit
            // ([video] video.camera_effects).
            if (Sys_GetOption("video.lens_dirt",Sys_GetOption("video.camera_effects",1)) != 0)
                g_lensDirt[view] = LensDirt_Accumulate(g_lensDirt[view], shake * 20);
            // OpenCMR2: dirt on the windscreen from driving hard
            // ([video] video.windscreen_dirt).
            if (Sys_GetOption("video.windscreen_dirt", 40) != 0) {
                int hitDirt = Effect_Scale(shake * 10,Sys_GetOption("video.windscreen_dirt",40));
                g_windscreenDirt[view] = WindscreenDirt_Accumulate(g_windscreenDirt[view], hitDirt);
                Windshield_AddDrivingSpatter(view, hitDirt);   // localised blobs on the glass
            }
        }
    }
}

// Plays one of the three horn sounds (random for kind 0).
// FUNCTION: CMR2 0x00418cd0
void Race_PlayBreakingGlassSound(unsigned int view, int kind, int listener)
{
    int sound = 0;

    switch (kind) {
    case 0: sound = rand() % 2; break;
    case 2: sound = 2; break;
    }
    Race_PlayDistanceScaledCarSound(view, (unsigned short)(g_glassFirstSample + sound), 0x10000, listener);
}

void StageObject_SetBoundedWeatherKind(int value);

// GLOBAL: CMR2 0x00537350
int g_unk0x00537350;
// GLOBAL: CMR2 0x0053735c
int g_coDriverFirstSample;

// Updates the current route block and queues its first callout.
// match 59%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00416f70
void Race_AdvanceRouteCalloutBlock(int player)
{
    int remaining = 500 - Race_GetClampedCountdownValue();
    int block = remaining / 100;

    g_unk0x00537350 = -1;
    g_unk0x0053708c[player] = RallyData_GetActiveCarRaceRecordField0(Car_Get(player));
    if ((BYTE)RallyData_GetSelectionFlag26()) {
        if (remaining % 100 < 20)
            g_unk0x00537350 = 6;
        else if (block <= 4)
            g_unk0x00537350 = block + 1;
        else
            g_unk0x00537350 = -1;
    }
    RaceCalls_UpdatePlaybackQueue();
    // the original reads [slot*4 + 0x537190], i.e. this same table one player back
    if (player > 0 && g_unk0x005371a4[block + player * 5 - 5] != 0) {
        g_unk0x005371a4[block + player * 5] = 1;
        return;
    }
    if (player == 0 && g_unk0x005371a4[block] == 0) {
        g_unk0x005371a4[block] = 1;
        StageObject_SetBoundedWeatherKind(block + 1);
        if (block == 2)
            Race_AssignUnusedSlot(g_coDriverFirstSample + 0x38);
        else if (block == 1)
            Race_AssignUnusedSlot(g_coDriverFirstSample + 0x39);
        else if (block == 0)
            Race_AssignUnusedSlot(g_coDriverFirstSample + 0x3a);
    }
    g_unk0x0053708c[player] = RallyData_GetActiveCarRaceRecordField0(Car_Get(player)) - 1;
}

int View_GetActiveCameraMode(BYTE index);
void Glow_NoOpEntryCallback(BYTE a, BYTE b, int c, int d);

// Advances one player's "menu" cursor when the device that owns it (arg = the
// player's input slot) pressed the up/down/left/right buttons: the odd modes
// scroll the mode list, the even ones step the stage index, and the shared
// tail replays the two beeper sounds. Both multiplayer branches carry their own
// copy of the two beeper calls (the original's tail merge keeps the first copy).
// FUNCTION: CMR2 0x0041d0c0
void Race_UpdatePlayerSelectionCursor(int param_1)
{
    int flags;
    int i;
    BYTE value;

    if (Race_GetPlayerRecordField4((BYTE)param_1) == 0)
        return;
    flags = (int)CInput::GetAvailableDeviceRecord(param_1)->field_0x8;
    if (Race_IsMultiplayerRecordMode10() != 0) {
        if (param_1 != 0 && Race_GetPlayerRecordField4(0) != 0)
            return;
        if (flags & 0x1000) {
            if (CGameInfo::GetConfiguredGameMode() == 2 || CGameInfo::GetConfiguredGameMode() == 1 ||
                CGameInfo::GetConfiguredGameMode() == 0)
                return;
            View_SwitchCamera(1, (View_GetActiveCameraMode(1) == 7 ? 4 : 7), 0xffff, View_GetActiveCameraFlags(1), 0);
            Glow_NoOpEntryCallback(0, 1, 1, 1);
            Glow_NoOpEntryCallback(1, 1, 1, 1);
        } else {
            if ((flags & 0x2000) == 0)
                return;
            value = (BYTE)(View_GetActiveCameraFlags(1) + 1);
            if (value >= (BYTE)Race_GetBaseCarCount())
                value = 0;
            View_SetCameraType(1, 0, value, 0);
            Glow_NoOpEntryCallback(0, 1, 1, 1);
            Glow_NoOpEntryCallback(1, 1, 1, 1);
        }
        return;
    }
    if (flags & 0x1000) {
        i = 0;
        if ((BYTE)Race_GetBaseCarCount() > 0) {
            do {
                Glow_NoOpEntryCallback((BYTE)i, 0, 1, 1);
                i++;
            } while (i < (int)((BYTE)Race_GetBaseCarCount()));
        }
        View_SwitchCamera((BYTE)param_1, (View_GetActiveCameraMode((BYTE)param_1) == 7 ? 4 : 7), 0xffff,
                     View_GetActiveCameraFlags((BYTE)param_1), 0);
        return;
    }
    if ((flags & 0x2000) != 0 && (BYTE)Race_GetBaseCarCount() > 1 && (BYTE)RallyDataState() == 1) {
        value = (BYTE)(View_GetActiveCameraFlags(0) + 1);
        if (value >= (BYTE)Race_GetBaseCarCount())
            value = 0;
        View_SetCameraType(0, 0, value, 0);
        i = 0;
        if ((BYTE)Race_GetBaseCarCount() > 0) {
            do {
                Glow_NoOpEntryCallback((BYTE)i, 0, 1, 1);
                i++;
            } while (i < (int)((BYTE)Race_GetBaseCarCount()));
        }
    }
}

unsigned short RallyData_GetPrimaryStageScoreScale(void);
unsigned short RallyData_GetSecondaryStageScoreScale(void);
void RallyData_SetSelectionFlag20(unsigned int param1);
void StageTiming_CreateWhiteStageLight(void);
void StageLights_BlendPresetPair(unsigned short timePrimary, unsigned short timeSecondary, BYTE **records, StageLightPreset **pPrimary,
                  StageLightPreset **pSecondary);
void StageLights_SetPairFogDistances(StageLightPreset *pA, StageLightPreset *pB);


// GLOBAL: CMR2 0x005196dc
char g_str0x005196dc[12] = "0500.hor";

// GLOBAL: CMR2 0x005196d0
char g_str0x005196d0[12] = "0600.hor";

// GLOBAL: CMR2 0x005196c4
char g_str0x005196c4[12] = "0800.hor";

// GLOBAL: CMR2 0x005196b8
char g_str0x005196b8[12] = "1000.hor";

// GLOBAL: CMR2 0x005196ac
char g_str0x005196ac[12] = "1200.hor";

// GLOBAL: CMR2 0x005196a0
char g_str0x005196a0[12] = "1500.hor";

// GLOBAL: CMR2 0x00519694
char g_str0x00519694[12] = "1700.hor";

// GLOBAL: CMR2 0x00519688
char g_str0x00519688[12] = "1800.hor";

// GLOBAL: CMR2 0x0051967c
char g_str0x0051967c[12] = "1900.hor";

// GLOBAL: CMR2 0x00519670
char g_str0x00519670[12] = "2000.hor";

// GLOBAL: CMR2 0x00519664
char g_str0x00519664[12] = "2200.hor";

// GLOBAL: CMR2 0x00519658
char g_str0x00519658[12] = "2400.hor";

// GLOBAL: CMR2 0x0051964c
char g_str0x0051964c[12] = "0500CLO.hor";

// GLOBAL: CMR2 0x00519640
char g_str0x00519640[12] = "0600CLO.hor";

// GLOBAL: CMR2 0x00519634
char g_str0x00519634[12] = "0800CLO.hor";

// GLOBAL: CMR2 0x00519628
char g_str0x00519628[12] = "1000CLO.hor";

// GLOBAL: CMR2 0x0051961c
char g_str0x0051961c[12] = "1200CLO.hor";

// GLOBAL: CMR2 0x00519610
char g_str0x00519610[12] = "1500CLO.hor";

// GLOBAL: CMR2 0x00519604
char g_str0x00519604[12] = "1700CLO.hor";

// GLOBAL: CMR2 0x005195f8
char g_str0x005195f8[12] = "1800CLO.hor";

// GLOBAL: CMR2 0x005195ec
char g_str0x005195ec[12] = "1900CLO.hor";

// GLOBAL: CMR2 0x005195e0
char g_str0x005195e0[12] = "2000CLO.hor";

// GLOBAL: CMR2 0x005195d4
char g_str0x005195d4[12] = "2200CLO.hor";

// GLOBAL: CMR2 0x005195c8
char g_str0x005195c8[12] = "2400CLO.hor";

// GLOBAL: CMR2 0x005195bc
char g_str0x005195bc[12] = "0500STO.hor";

// GLOBAL: CMR2 0x005195b0
char g_str0x005195b0[12] = "0600STO.hor";

// GLOBAL: CMR2 0x005195a4
char g_str0x005195a4[12] = "0800STO.hor";

// GLOBAL: CMR2 0x00519598
char g_str0x00519598[12] = "1000STO.hor";

// GLOBAL: CMR2 0x0051958c
char g_str0x0051958c[12] = "1200STO.hor";

// GLOBAL: CMR2 0x00519580
char g_str0x00519580[12] = "1500STO.hor";

// GLOBAL: CMR2 0x00519574
char g_str0x00519574[12] = "1700STO.hor";

// GLOBAL: CMR2 0x00519568
char g_str0x00519568[12] = "1800STO.hor";

// GLOBAL: CMR2 0x0051955c
char g_str0x0051955c[12] = "1900STO.hor";

// GLOBAL: CMR2 0x00519550
char g_str0x00519550[12] = "2000STO.hor";

// GLOBAL: CMR2 0x00519544
char g_str0x00519544[12] = "2200STO.hor";

// GLOBAL: CMR2 0x00519538
char g_str0x00519538[12] = "2400STO.hor";

// GLOBAL: CMR2 0x0051952c
char g_str0x0051952c[12] = "0500BLI.hor";

// GLOBAL: CMR2 0x00519520
char g_str0x00519520[12] = "0600BLI.hor";

// GLOBAL: CMR2 0x00519514
char g_str0x00519514[12] = "0800BLI.hor";

// GLOBAL: CMR2 0x00519508
char g_str0x00519508[12] = "1000BLI.hor";

// GLOBAL: CMR2 0x005194fc
char g_str0x005194fc[12] = "1200BLI.hor";

// GLOBAL: CMR2 0x005194f0
char g_str0x005194f0[12] = "1500BLI.hor";

// GLOBAL: CMR2 0x005194e4
char g_str0x005194e4[12] = "1700BLI.hor";

// GLOBAL: CMR2 0x005194d8
char g_str0x005194d8[12] = "1800BLI.hor";

// GLOBAL: CMR2 0x005194cc
char g_str0x005194cc[12] = "1900BLI.hor";

// GLOBAL: CMR2 0x005194c0
char g_str0x005194c0[12] = "2000BLI.hor";

// GLOBAL: CMR2 0x005194b4
char g_str0x005194b4[12] = "2200BLI.hor";

// GLOBAL: CMR2 0x005194a8
char g_str0x005194a8[12] = "2400BLI.hor";


// The two time-of-day lighting presets (.hor files) the stage lighting
// blends, picked from the 48 resolved preset files.
// GLOBAL: CMR2 0x00538234
StageLightPreset *g_stageLightPresetSecondary;
// GLOBAL: CMR2 0x00538238
StageLightPreset *g_stageLightPresetPrimary;

// Resolves the 48 stage texture names through the generic file loader, builds
// the six palette entries of the stage geometry from the primary/secondary
// colour records and refreshes the stage colour ramps.
// FUNCTION: CMR2 0x0041fd30
void Race_ResolveStageTexturesAndPalettes(void)
{
    char *pNames[48];
    BYTE *handles[48];
    BYTE colour[6];
    StageLightPreset *pPrimary;
    StageLightPreset *pSecondary;
    int i;
    int avg;

    pNames[0] = g_str0x005196dc;
    pNames[1] = g_str0x005196d0;
    pNames[2] = g_str0x005196c4;
    pNames[3] = g_str0x005196b8;
    pNames[4] = g_str0x005196ac;
    pNames[5] = g_str0x005196a0;
    pNames[6] = g_str0x00519694;
    pNames[7] = g_str0x00519688;
    pNames[8] = g_str0x0051967c;
    pNames[9] = g_str0x00519670;
    pNames[10] = g_str0x00519664;
    pNames[11] = g_str0x00519658;
    pNames[12] = g_str0x0051964c;
    pNames[13] = g_str0x00519640;
    pNames[14] = g_str0x00519634;
    pNames[15] = g_str0x00519628;
    pNames[16] = g_str0x0051961c;
    pNames[17] = g_str0x00519610;
    pNames[18] = g_str0x00519604;
    pNames[19] = g_str0x005195f8;
    pNames[20] = g_str0x005195ec;
    pNames[21] = g_str0x005195e0;
    pNames[22] = g_str0x005195d4;
    pNames[23] = g_str0x005195c8;
    pNames[24] = g_str0x005195bc;
    pNames[25] = g_str0x005195b0;
    pNames[26] = g_str0x005195a4;
    pNames[27] = g_str0x00519598;
    pNames[28] = g_str0x0051958c;
    pNames[29] = g_str0x00519580;
    pNames[30] = g_str0x00519574;
    pNames[31] = g_str0x00519568;
    pNames[32] = g_str0x0051955c;
    pNames[33] = g_str0x00519550;
    pNames[34] = g_str0x00519544;
    pNames[35] = g_str0x00519538;
    pNames[36] = g_str0x0051952c;
    pNames[37] = g_str0x00519520;
    pNames[38] = g_str0x00519514;
    pNames[39] = g_str0x00519508;
    pNames[40] = g_str0x005194fc;
    pNames[41] = g_str0x005194f0;
    pNames[42] = g_str0x005194e4;
    pNames[43] = g_str0x005194d8;
    pNames[44] = g_str0x005194cc;
    pNames[45] = g_str0x005194c0;
    pNames[46] = g_str0x005194b4;
    pNames[47] = g_str0x005194a8;
    for (i = 0; i < 48; i++)
        handles[i] = (BYTE *)CGenericFileLoader::FindFile((GenericFile *)StageTiming_GetStageFile3(),
                                                       pNames[i], NULL, NULL, 0);
    StageLights_BlendPresetPair(RallyData_GetPrimaryStageScoreScale(), RallyData_GetSecondaryStageScoreScale(), handles, &g_stageLightPresetPrimary,
                 &g_stageLightPresetSecondary);
    StageLights_SetPairFogDistances(g_stageLightPresetPrimary, g_stageLightPresetSecondary);
    if (g_stageLightPresetPrimary != NULL && g_stageLightPresetSecondary != NULL) {
        pPrimary = g_stageLightPresetSecondary;
        pSecondary = g_stageLightPresetPrimary;
        for (i = 0; i < 6; i++) {
            avg = ((int)pSecondary->colourCorrection[i] + (int)pPrimary->colourCorrection[i]) / 2;
            if (avg > 0xff)
                avg = 0xff;
            colour[i] = (BYTE)avg;
        }
        CGraphics::SetPrimaryColourByte(colour[0]);
        CGraphics::SetPrimaryColourScale(colour[1]);
        CGraphics::SetPrimaryColourBias(colour[2]);
        CGraphics::SetSecondaryColourByte(colour[3]);
        CGraphics::SetSecondaryColourScale(colour[4]);
        CGraphics::SetSecondaryColourBias(colour[5]);
        if ((unsigned short)RallyData_GetPrimaryStageScoreScale() >= 0x834) {
            RallyData_SetSelectionFlag20(1);
            StageTiming_CreateWhiteStageLight();
            return;
        }
        RallyData_SetSelectionFlag20(0);
        StageTiming_CreateWhiteStageLight();
    }
}

// Route helpers of the per-car update below (no shared header declares them).
int RallyData_GetRouteAvailabilityState(void);
RallyRouteNode *RallyData_GetAvailableRouteNodeRecord(int index);
int RallyData_GetCarRecordValueRatio(Car *p);
BYTE StageTiming_GetCheckpointField18(int index);
int StageTiming_GetClockTime(void);
void RallyRoute_CopyIndexedNodeDirection(unsigned int nodeIndex, FixVector *pOut);

// Set while the node scan near the car's position finds a node whose callouts
// have not been delivered yet.
// GLOBAL: CMR2 0x00537354
int g_pendingCalloutNodeFound;

// Per-car route/timing update. Keeps the tracked route position
// g_unk0x0053708c[] in sync with the car's node (best, from RallyData), raises
// the wrong-way flag while the node direction projected on the
// front wheel rolling direction points backwards, scans the nodes around the
// position for pending callouts (Race_QueuePlayerCallout) and, when the leading call of
// this car expires, shifts its five call records g_raceCallRecords[car*5].
// match 50%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// Implementada; en la cola el original relee un campo redundante de g_raceCallRecords[car*5+4] que no transcribimos, y el reparto de bloques del switch difiere.
// FUNCTION: CMR2 0x00417090
void Race_UpdateCarRouteAndWrongWayState(int param_1)
{
    int cur;
    int best;
    int prev;
    int node;
    RallyRouteNode *n;
    int dot;
    int i;
    unsigned int cnt;
    FixVector dir;
    Car *pCar;
    unsigned int flags;
    BYTE callId;
    BYTE prevCallId;
    RaceCallRecord *pRec;

    if (RallyData_GetRouteAvailabilityState() == 0)
        return;

    if ((char)RallyData_GetSelectionFlag28() != '\0' &&
        ((char)RallyData_GetFlag25() != '\0' || (char)RallyData_GetFlag24() != '\0') &&
        (char)StageTiming_GetCheckpointField18(param_1) != '\0') {
        g_unk0x0053708c[param_1] = 0;
    }

    cur = g_unk0x0053708c[param_1];
    best = RallyData_GetActiveCarRaceRecordField0(Car_Get(param_1));

    if (g_unk0x005371a0 == 0 && param_1 == 0) {
        StageObject_SetBoundedWeatherKind(0);
        Race_AssignUnusedSlot(g_coDriverFirstSample + 0xd);
        g_unk0x005371a0 = 1;
    }

    if ((char)RallyData_GetSelectionFlag26() != '\0') {
        i = StageTiming_GetClockTime();
        g_unk0x00537350 = (i < 0x5a) - 1;
    }

    RaceCalls_UpdatePlaybackQueue();

    if (best < cur - 0xc)
        g_unk0x0053708c[param_1] = best;

    if (best < cur - 1 &&
        ((int)((unsigned int)RallyData_GetRouteAvailabilityState() >> 1) <= best ||
         cur <= (int)((unsigned int)RallyData_GetRouteAvailabilityState() >> 1))) {
        RallyRoute_CopyIndexedNodeDirection(best, &dir);
        pCar = Car_Get(param_1);
        dot = FixVecDot(&pCar->rearWheelDir, &dir);
        if (dot < -0x8000) {
            g_raceWrongWayFlags[param_1] = 1;
            g_unk0x00537198[param_1] = best;
        }
    }

    if ((int)((unsigned int)RallyData_GetRouteAvailabilityState() >> 1) < best &&
        cur < (int)((unsigned int)RallyData_GetRouteAvailabilityState() >> 1)) {
        g_unk0x00537198[param_1] = best;
        g_unk0x0053708c[param_1] = best;
        cur = best;
    }

    if (g_raceWrongWayFlags[param_1] != 0) {
        RallyRoute_CopyIndexedNodeDirection(best, &dir);
        pCar = Car_Get(param_1);
        dot = FixVecDot(&pCar->rearWheelDir, &dir);
        if (dot > 0x3333)
            g_raceWrongWayFlags[param_1] = 0;
    }

    if (g_unk0x00537198[param_1] < best)
        g_raceWrongWayFlags[param_1] = 0;

    g_pendingCalloutNodeFound = 0;
    i = g_unk0x0053708c[param_1] + 1;
    if (i <= g_unk0x0053708c[param_1] + 6) {
        do {
            cnt = (unsigned int)RallyData_GetRouteAvailabilityState();
            if ((int)cnt <= i || RallyData_GetAvailableRouteNodeRecord(i)->callId != -1)
                g_pendingCalloutNodeFound = 1;
            i++;
        } while (i <= g_unk0x0053708c[param_1] + 6);
    }

    if ((char)((CallbackStateMachine *)StageUI_GetRaceResultTable())->records[param_1].packedState == '\t')
        g_pendingCalloutNodeFound = 1;

    if ((g_raceCallRecords[param_1 * 5 + 1].flags & 0x100) != 0)
        g_pendingCalloutNodeFound = 1;

    if (Car_IsStateC10Set(param_1) == 0 &&
        (char)((CallbackStateMachine *)StageUI_GetRaceResultTable())->records[param_1].packedState == '\b') {
        if (!((char)RallyData_GetFlag25() != '\0' &&
              RallyData_GetCarRecordValueRatio(Car_Get(param_1)) >= 0x10000)) {
            if (g_unk0x0053708c[param_1] < best) {
                node = cur + 1;
                if (node <= best) {
                    prev = cur + 2;
                    do {
                        n = RallyData_GetAvailableRouteNodeRecord(node);
                        if (n->callId != -1 &&
                            Race_IsFlag24Clear() != 0 && Car_IsStateC10Set(param_1) == 0) {
                            int ok = 1;
                            if (prev >= 2) {
                                n = RallyData_GetAvailableRouteNodeRecord(node - 1);
                                if (n->callId != -1) {
                                    ok = 0;
                                } else {
                                    callId = (BYTE)RallyData_GetAvailableRouteNodeRecord(node)->callId;
                                    prevCallId = (BYTE)RallyData_GetAvailableRouteNodeRecord(prev)->callId;
                                }
                            } else {
                                callId = (BYTE)RallyData_GetAvailableRouteNodeRecord(node)->callId;
                                prevCallId = (BYTE)RallyData_GetAvailableRouteNodeRecord(prev)->callId;
                            }
                            if (ok)
                                Race_QueuePlayerCallout(param_1, callId, prevCallId, (BYTE)node);
                        }
                        node++;
                        prev++;
                    } while (node <= best);
                }
                g_unk0x0053708c[param_1] = best;
            }
        }
    }

    if (best < g_unk0x00537198[param_1])
        g_unk0x00537198[param_1] = best;
    if (g_unk0x00537198[param_1] + 6 < best)
        g_unk0x00537198[param_1] = best - 6;

    if (Race_IsFlag24Clear() == 0)
        return;
    flags = g_raceCallRecords[param_1 * 5].flags;
    if ((flags & 0x100) == 0)
        return;
    if ((BYTE)flags != 0x19) {
        g_raceCallRecords[param_1 * 5].flags = (flags - 1 ^ flags) & 0xff ^ flags;
    } else {
        if (g_pendingCalloutNodeFound != 0)
            g_raceCallRecords[param_1 * 5].flags = (flags - 1 ^ flags) & 0xff ^ flags;
    }
    if ((BYTE)g_raceCallRecords[param_1 * 5].flags == 0) {
        pRec = &g_raceCallRecords[param_1 * 5];
        for (i = 0; i < 4; i++)
            pRec[i] = pRec[i + 1];
        pRec[4].flags &= 0xfffffc00;
        pRec[4].primaryCallId = 0;
        pRec[4].secondaryCallId = 0;
    }
}

void Car_InterpolateRenderTransforms(void);
void StageTiming_InterpolateMovingPartMatrices(int t);
void StageObject_InterpolateAllObjectMatrices(int t);
void StageObject_InterpolateOrderedMotionRecords(int scale);
void StageObject_InterpolateSecondaryRampRecords(int t);
void Particle_Interpolate(int t);
void Dash_Interpolate(int t);
void StageObject_UpdateDebris(int scale);
void StageObject_InterpolateHeadlightGlows(int t);

// OpenCMR2: hands the speed-driven look settings (tunnel vision, speed streak, vapor trails) to the renderer from the car speed. Visual only.
// Also called while paused with the tuning panel open, so a changed setting reaches the frozen scene.
void AttractCapture_ViewSignature(int view, int *type, int *drone);
int Race_GetPlayerRecordField4(BYTE index);
void ReplayControls_Update(void);
bool View_IsDroneCamera(int view);   // Car.cpp: the camera operator's drone is the active camera of that player
void Race_ApplySpeedEffectParams(void)
{
    {
        int viewType = 0, viewDrone = 0; AttractCapture_ViewSignature(0, &viewType, &viewDrone);
        if (SpeedLook_VaporOff(Race_GetPlayerRecordField4(0) != 0)) Gfx_ClearVaporTrail();   // replay: air vortices off in every view, saved setting untouched
        // Replay: only the interior view (type 3) keeps the effects. Drone camera: never. See SpeedLookPolicy.h. Settings are not rewritten.
        if (SpeedLook_ScreenEffectsOff(viewType, Race_GetPlayerRecordField4(0) != 0, View_IsDroneCamera(0), Sys_GetOption("video.speed_effects", 1) != 0)) {
            Gfx_ClearSpeedEffects();   // targets and eased values to 0: no residual blur or lines after a camera switch
            if (!SpeedLook_VaporOff(Race_GetPlayerRecordField4(0) != 0)) Gfx_SetVaporTrail(SpeedStreak_Strength((float)Car_Get(0)->speed / 65536.0f / 1.5f, Sys_GetOption("video.speed_fov_hold", 0), Sys_GetOption("video.vapor_trails", 0) != 0 ? 100 : 0)); // world-space trails: off in replay, as set while driving
            return;
        }
    }
    {   // Cockpit (camera type 3) only: the interior's own depth keeps it out of the tunnel-vision blur. Unit: tenths of a world unit, default 16 = 1.6.
        int type = 0, drone = 0; AttractCapture_ViewSignature(0, &type, &drone);
        int tenths = Sys_GetOption("video.cockpit_blur_clear", 16); if (tenths < 0) tenths = 0; if (tenths > 60) tenths = 60;
        Gfx_SetTunnelNearClear(type == 3 ? tenths / 10.0f : 0.0f);
    }
    // OpenCMR2: speed-driven tunnel vision (visual only; reads speed, changes nothing in the sim).
    {
        float speedUnits = SpeedFov_HeldRatio((float)Car_Get(0)->speed / 65536.0f / 1.5f, Sys_GetOption("video.speed_fov_hold", 0)); // hold: overdo test only
        int strength = Sys_GetOption("video.tunnel_vision", 130); // 0..200, adjustable; 0 = off; above 100 is the experimental overdrive
        if (strength < 0) strength = 0;
        if (strength > 200) strength = 200;
        Gfx_SetSpeedFocus(speedUnits * (float)strength / 100.0f);
    }
    // OpenCMR2: speed streak (visual only): directional radial streak, 0 = off (default). Same speed ratio and overdo hold as above.
    Gfx_SetSpeedStreak(SpeedStreak_Strength((float)Car_Get(0)->speed / 65536.0f / 1.5f, Sys_GetOption("video.speed_fov_hold", 0), Sys_GetOption("video.speed_streak", 0)));
    // OpenCMR2: vapor trails (visual only, video.vapor_trails 0/1, default off): same speed ratio and overdo hold; the renderer eases and fades them in.
    Gfx_SetVaporTrail(SpeedLook_VaporOff(Race_GetPlayerRecordField4(0) != 0) ? 0.0f : SpeedStreak_Strength((float)Car_Get(0)->speed / 65536.0f / 1.5f, Sys_GetOption("video.speed_fov_hold", 0), Sys_GetOption("video.vapor_trails", 0) != 0 ? 100 : 0));
}

// Per-frame race update: advances the stage timing, the particle and dash
// effects and the debris, then refreshes them once the frame gate opens.
// FUNCTION: CMR2 0x0041d060
int StageTiming_GetClockTime(void);
void AttractCapture_ViewSignature(int view, int *type, int *drone);
void Race_UpdateFrameEffects(int param_1)
{
    Car_InterpolateRenderTransforms();
    StageTiming_InterpolateMovingPartMatrices(param_1);
    StageObject_InterpolateAllObjectMatrices(param_1);
    StageObject_InterpolateOrderedMotionRecords(param_1);
    Particle_Interpolate(param_1);
    StageObject_InterpolateSecondaryRampRecords(param_1);
    Dash_Interpolate(param_1);
    // OpenCMR2: --attract-capture reads the stage clock only (no sim input).
    if (AttractCapture::Started()) {
        int viewType = 0, drone = 0;
        AttractCapture_ViewSignature(0, &viewType, &drone);
        Car *pCapture = Car_Get(0);
        AttractCapture::Tick(StageTiming_GetClockTime(), (BYTE)RallyDataCountryIndex(), (BYTE)RallyDataStageIndex(), pCapture->speed,
                             pCapture->position.x, pCapture->position.y, pCapture->position.z, viewType, drone);
    }
    Race_ApplySpeedEffectParams();
    // OpenCMR2: windscreen dirt accumulates while driving; faster = more dirt.
    if (Sys_GetOption("video.windscreen_dirt", 40) != 0) {
        Car *pPlayer = Car_Get(0);
        int speed = pPlayer->speed;
        if (speed > 0x8000) {
            int amount = FixMul(speed, 0x0800) >> 16;
            if (amount < 1)
                amount = 1;
            static int residue=0;
            amount=Effect_Accumulate(amount,Sys_GetOption("video.windscreen_dirt",40),residue);
            g_windscreenDirt[0] = WindscreenDirt_Accumulate(g_windscreenDirt[0], amount);
            Windshield_AddDrivingSpatter(0, amount);   // localised blobs on the glass
        }
    }
    WindscreenWiper_UpdateInput();
    TuningPanel_NoteSite("race-step");TuningPanel_Update();
    ReplayControls_Update();
    if ((BYTE)RallyData_IsChampionshipFinalStage()) {
        StageObject_UpdateDebris(param_1);
    }
    if (CGameInfo::IsActiveCheatEnabled(0) != 0) {
        StageObject_InterpolateHeadlightGlows(param_1);
    }
}

int StageObject_GetWheelSlip(int carIndex, int wheelIndex);
int StageObject_GetNormalizedWheelSlip(int car, int wheel);

// Recomputes one car's stage sound levels. The four wheel-slip values are read
// and scaled, the level of each of the four stage sound slots is derived from
// the engine volume and the largest slip and then eased towards the value of
// the previous frame (the 0x05xxx4xx volume/previous tables), and the result is pushed into the slot handles. The shared
// engine sample gets its pan from the engine speed when the current pattern has
// run out, and the slot levels are muted, faded out or restored depending on
// the surface the wheels are on.
// The volumes live in the per-car tables below: the original updates them in
// place, so other readers see the muted/clamped values of this frame.
// Per-car surface sound volumes (8 cars each): see StageUI.h.

// FUNCTION: CMR2 0x0041a5c0
void Race_ComputeWheelSurfaceSoundVolumes(int param_1, int param_2)
{
    int ownVolume[4];
    Car *pCar;
    CarSoundSet *pSet;
    RaceCarSoundState *pState;
    int speedVolume;
    int ownVolumeSum;
    int speedFrac;
    int weight;
    int weightInv;
    int total;
    unsigned int i;
    int slotVolume;

    speedVolume = Race_GetNormalizedCarSpeed(param_1);
    pSet = &g_carSoundSets[param_1];
    pState = &g_carSoundStates[param_1];
    pCar = Car_Get(param_1);
    total = 0;
    for (i = 0; i < 4; i++) {
        ownVolume[i] = FixDiv(FIX_ABS(pCar->wheelLoad[0]), 0xa0000);
        if (ownVolume[i] > 0x10000)
            ownVolume[i] = 0x10000;
        ownVolume[i] = 0x10000 - ownVolume[i];
        total += ownVolume[i];
    }
    ownVolumeSum = FixDiv(total, 0x40000);
    speedFrac = FixMulShift32(pCar->speed, 0x431168);
    g_surfaceSlotMax = 0;
    g_carMaxVolume[param_1] = 0;
    for (i = 0; i < 4; i++) {
        g_wheelSlipVolume[param_1][i] = StageObject_GetWheelSlip(param_1, i);
        slotVolume = StageObject_GetNormalizedWheelSlip(param_1, i);
        g_carSlotVolumes[param_1][i] = slotVolume;
        if (g_wheelSlipVolume[param_1][i] > g_carMaxVolume[param_1])
            g_carMaxVolume[param_1] = g_wheelSlipVolume[param_1][i];
        if (slotVolume > g_surfaceSlotMax)
            g_surfaceSlotMax = slotVolume;
    }
    if (pState->pattern != 0x19) {
        weight = FixDiv(StageObject_GetCarSoundElapsedTime(param_1) - pState->time, 0x320000);
        weightInv = 0x10000 - weight;
    } else {
        weight = 0x10000;
        weightInv = 0;
    }
    g_surfaceVolA[param_1] = FixMul(weight, speedVolume);
    g_surfaceVolB[param_1] = FixMul(weightInv, speedVolume);
    g_surfaceVolD[param_1] = FixMul(weight, g_carMaxVolume[param_1]);
    g_surfaceVolC[param_1] = FixMul(g_carMaxVolume[param_1], weightInv);
    g_surfaceVolB[param_1] += g_carMaxVolume[param_1];
    g_surfaceVolA[param_1] += g_carMaxVolume[param_1];
    if (g_surfaceVolD[param_1] - g_surfacePrevD[param_1] > 0xc000)
        g_surfaceVolD[param_1] = g_surfacePrevD[param_1] + 0xc000;
    else if (g_surfaceVolD[param_1] - g_surfacePrevD[param_1] < -0xccc)
        g_surfaceVolD[param_1] = g_surfacePrevD[param_1] - 0xccc;
    if (g_surfaceVolC[param_1] - g_surfacePrevC[param_1] > 0xc000)
        g_surfaceVolC[param_1] = g_surfacePrevC[param_1] + 0xc000;
    else if (g_surfaceVolC[param_1] - g_surfacePrevC[param_1] < -0xccc)
        g_surfaceVolC[param_1] = g_surfacePrevC[param_1] - 0xccc;
    if (g_surfaceVolA[param_1] - g_surfacePrevA[param_1] > 0xccc)
        g_surfaceVolA[param_1] = g_surfacePrevA[param_1] + 0xccc;
    else if (g_surfaceVolA[param_1] - g_surfacePrevA[param_1] < -0xccc)
        g_surfaceVolA[param_1] = g_surfacePrevA[param_1] - 0xccc;
    if (g_surfaceVolB[param_1] - g_surfacePrevB[param_1] > 0xccc)
        g_surfaceVolB[param_1] = g_surfacePrevB[param_1] + 0xccc;
    else if (g_surfaceVolB[param_1] - g_surfacePrevB[param_1] < -0xccc)
        g_surfaceVolB[param_1] = g_surfacePrevB[param_1] - 0xccc;
    g_surfaceSlotMax = FixMul(g_surfaceSlotMax, 0x20000);
    if (g_surfaceSlotMax > 0x10000)
        g_surfaceSlotMax = 0x10000;
    if (pState->state == 0x19) {
        if (pCar->flag0x1d0[2] != 0 && speedFrac < 0x32) {
            g_surfaceVolA[param_1] = g_surfaceSlotMax;
            g_surfaceVolB[param_1] = g_surfaceSlotMax;
            g_surfaceVolD[param_1] = g_surfaceSlotMax;
            g_surfaceVolC[param_1] = g_surfaceSlotMax;
            g_wheelSlipVolume[param_1][0] = FixMul(g_wheelSlipVolume[param_1][0], 0x10000 - g_surfaceSlotMax);
            g_wheelSlipVolume[param_1][1] = FixMul(g_wheelSlipVolume[param_1][1], 0x10000 - g_surfaceSlotMax);
            g_wheelSlipVolume[param_1][2] = FixMul(g_wheelSlipVolume[param_1][2], 0x10000 - g_surfaceSlotMax);
            g_wheelSlipVolume[param_1][3] = FixMul(g_wheelSlipVolume[param_1][3], 0x10000 - g_surfaceSlotMax);
        } else if (pState->throttleSample != 0) {
            g_surfaceVolA[param_1] = 0;
            g_surfaceVolB[param_1] = 0;
            g_surfaceVolD[param_1] = 0;
            g_surfaceVolC[param_1] = 0;
        } else {
            if (pCar->wheelLoad[0] < 0xa0000 && pCar->wheelLoad[1] < 0xa0000 &&
                pCar->wheelLoad[2] < 0xa0000 && pCar->wheelLoad[3] < 0xa0000 &&
                (pCar->handbrake != 0 || pCar->flag0x1d0[3] != 0) &&
                (FixMul(pCar->speed, 0x431168) & 0xffff0000) > 0x50000) {
                g_surfaceVolB[param_1] = 0x20000;
                g_surfaceVolD[param_1] = 0x20000;
                g_surfaceVolC[param_1] = 0x20000;
                g_surfaceVolA[param_1] = 0x20000;
                g_wheelSlipVolume[param_1][0] = 0x10000 - ownVolumeSum;
                g_wheelSlipVolume[param_1][1] = 0x10000 - ownVolumeSum;
                g_wheelSlipVolume[param_1][2] = 0x10000 - ownVolumeSum;
                g_wheelSlipVolume[param_1][3] = 0x10000 - ownVolumeSum;
            } else {
                g_surfaceVolB[param_1] = 0;
                g_surfaceVolA[param_1] = 0;
                g_surfaceVolD[param_1] = 0;
                g_surfaceVolC[param_1] = 0;
            }
            speedFrac = FixMulShift32(pCar->speed, 0x431168);
            if (speedFrac < 0x32 && speedFrac >= 0)
                g_unk0x005374c0 = (speedFrac - 0x32) * 11025 / 50 + 0x5622;
            else
                g_unk0x005374c0 = 0x5622;
            Sound_SetPan(pSet->handle[4], g_unk0x005374c0);
        }
    }
    if (g_surfaceVolA[param_1] > 0x10000)
        g_surfaceVolA[param_1] = 0x10000;
    if (g_surfaceVolB[param_1] > 0x10000)
        g_surfaceVolB[param_1] = 0x10000;
    if (Sound_IsPlaying(pSet->handle[4]))
        Sound_SetPlayingSlotVolume(pSet->handle[4],
                     FixMul(FixMul(g_carSoundVolumeScale, g_surfaceVolA[param_1]), NetRace_GetListenerDistanceAttenuation(param_1, param_2)));
    if (Sound_IsPlaying(pSet->handle[5]))
        Sound_SetPlayingSlotVolume(pSet->handle[5],
                     FixMul(FixMul(g_carSoundVolumeScale, g_surfaceVolB[param_1]), NetRace_GetListenerDistanceAttenuation(param_1, param_2)));
    if (Sound_IsPlaying(pSet->handle[6]))
        Sound_SetPlayingSlotVolume(pSet->handle[6],
                     FixMul(FixMul(g_carSoundVolumeScale, g_surfaceVolD[param_1]), NetRace_GetListenerDistanceAttenuation(param_1, param_2)));
    if (Sound_IsPlaying(pSet->handle[7]))
        Sound_SetPlayingSlotVolume(pSet->handle[7],
                     FixMul(FixMul(g_carSoundVolumeScale, g_surfaceVolC[param_1]), NetRace_GetListenerDistanceAttenuation(param_1, param_2)));
    if (NetRace_GetRaceSoundMode()) {
        total = 0;
        for (i = 0; i < 4; i++)
            total += g_wheelSlipVolume[param_1][i];
        if (total > 0x10000)
            total = 0x10000;
        if (Sound_IsPlaying(pSet->handle[0]))
            Sound_SetPlayingSlotVolume(pSet->handle[0],
                         FixMul(NetRace_GetListenerDistanceAttenuation(param_1, param_2), FixMul(g_carSoundVolumeScale, total)));
    } else {
        if (Sound_IsPlaying(pSet->handle[0]))
            Sound_SetPlayingSlotVolume(pSet->handle[0],
                         FixMul(NetRace_GetListenerDistanceAttenuation(param_1, param_2), FixMul(g_carSoundVolumeScale, g_wheelSlipVolume[param_1][0])));
        if (Sound_IsPlaying(pSet->handle[1]))
            Sound_SetPlayingSlotVolume(pSet->handle[1],
                         FixMul(NetRace_GetListenerDistanceAttenuation(param_1, param_2), FixMul(g_carSoundVolumeScale, g_wheelSlipVolume[param_1][1])));
        if (Sound_IsPlaying(pSet->handle[2]))
            Sound_SetPlayingSlotVolume(pSet->handle[2],
                         FixMul(NetRace_GetListenerDistanceAttenuation(param_1, param_2), FixMul(g_carSoundVolumeScale, g_wheelSlipVolume[param_1][2])));
        if (Sound_IsPlaying(pSet->handle[3]))
            Sound_SetPlayingSlotVolume(pSet->handle[3],
                         FixMul(NetRace_GetListenerDistanceAttenuation(param_1, param_2), FixMul(g_carSoundVolumeScale, g_wheelSlipVolume[param_1][3])));
    }
    g_surfacePrevD[param_1] = g_surfaceVolD[param_1];
    g_surfacePrevC[param_1] = g_surfaceVolC[param_1];
    g_surfacePrevA[param_1] = g_surfaceVolA[param_1];
    g_surfacePrevB[param_1] = g_surfaceVolB[param_1];
}

// Helper implemented in Car.cpp.
int View_GetActiveCameraMode(BYTE index);

// Sets the surface byte of slot `param_1` from the surface type of the car
// part `param_2` (1/2/3 keep the original surface, anything else forces
// gravel) and then rebuilds the slot's visual state.
// FUNCTION: CMR2 0x0041af60
void Race_SetPartSurfaceAndRefreshVisuals(int param_1, int param_2)
{
    if (View_GetActiveCameraMode(param_2) != 1) {
        if (View_GetActiveCameraMode(param_2) != 2) {
            if (View_GetActiveCameraMode(param_2) != 3) {
                g_carSoundViewSet[param_1] = 1;
                goto done;
            }
        }
    }
    g_carSoundViewSet[param_1] = 0;
done:
    Race_UpdateCarSurfaceSoundState(param_1, param_2);
    Race_SwitchEngineSampleByThrottle(param_1, param_2);
    Race_ComputeWheelSurfaceSoundVolumes(param_1, param_2);
    if (NetRace_GetRaceSoundMode() == 0)
        Race_UpdateIndependentEngineSound(param_1, param_2);
    Race_RestartChangedSurfaceSounds(param_1, param_2);
    Race_RerollCarSoundSlotsByPitch(param_1, param_2);
}

extern char g_strPathConcat[];
// Co-driver speech sample table loader: the installer sound directory is
// prepended to every .WAV name and the concatenation is loaded as a stage
// sample through the sound slot of the current stage file.
// GLOBAL: CMR2 0x00518674
char g_str0x00518674[] = "\\Speech\\English\\Wavs\\ONE.WAV";
// GLOBAL: CMR2 0x00518654
char g_str0x00518654[] = "\\Speech\\English\\Wavs\\TWO.WAV";
// GLOBAL: CMR2 0x00518634
char g_str0x00518634[] = "\\Speech\\English\\Wavs\\THREE.WAV";
// GLOBAL: CMR2 0x00518614
char g_str0x00518614[] = "\\Speech\\English\\Wavs\\FOUR.WAV";
// GLOBAL: CMR2 0x005185f4
char g_str0x005185f4[] = "\\Speech\\English\\Wavs\\FIVE.WAV";
// GLOBAL: CMR2 0x005185d4
char g_str0x005185d4[] = "\\Speech\\English\\Wavs\\SIX.WAV";
// GLOBAL: CMR2 0x005185ac
char g_str0x005185ac[] = "\\Speech\\English\\Wavs\\OPENHAIRPIN.WAV";
// GLOBAL: CMR2 0x00518588
char g_str0x00518588[] = "\\Speech\\English\\Wavs\\HAIRPIN.WAV";
// GLOBAL: CMR2 0x00518568
char g_str0x00518568[] = "\\Speech\\English\\Wavs\\LEFT.WAV";
// GLOBAL: CMR2 0x00518548
char g_str0x00518548[] = "\\Speech\\English\\Wavs\\RIGHT.WAV";
// GLOBAL: CMR2 0x00518528
char g_str0x00518528[] = "\\Speech\\English\\Wavs\\LONG.WAV";
// GLOBAL: CMR2 0x00518508
char g_str0x00518508[] = "\\Speech\\English\\Wavs\\OPENS.WAV";
// GLOBAL: CMR2 0x005184e4
char g_str0x005184e4[] = "\\Speech\\English\\Wavs\\TIGHTENS.WAV";
// GLOBAL: CMR2 0x005184c8
char g_str0x005184c8[] = "\\Speech\\English\\Wavs\\GO.WAV";
// GLOBAL: CMR2 0x005184a8
char g_str0x005184a8[] = "\\Speech\\English\\Wavs\\AND.WAV";
// GLOBAL: CMR2 0x00518488
char g_str0x00518488[] = "\\Speech\\English\\Wavs\\INTO.WAV";
// GLOBAL: CMR2 0x0051846c
char g_str0x0051846c[] = "\\Speech\\English\\Wavs\\30.WAV";
// GLOBAL: CMR2 0x00518450
char g_str0x00518450[] = "\\Speech\\English\\Wavs\\50.WAV";
// GLOBAL: CMR2 0x00518434
char g_str0x00518434[] = "\\Speech\\English\\Wavs\\70.WAV";
// GLOBAL: CMR2 0x00518414
char g_str0x00518414[] = "\\Speech\\English\\Wavs\\100.WAV";
// GLOBAL: CMR2 0x005183f4
char g_str0x005183f4[] = "\\Speech\\English\\Wavs\\120.WAV";
// GLOBAL: CMR2 0x005183d4
char g_str0x005183d4[] = "\\Speech\\English\\Wavs\\150.WAV";
// GLOBAL: CMR2 0x005183b4
char g_str0x005183b4[] = "\\Speech\\English\\Wavs\\200.WAV";
// GLOBAL: CMR2 0x00518394
char g_str0x00518394[] = "\\Speech\\English\\Wavs\\OVER.WAV";
// GLOBAL: CMR2 0x00518374
char g_str0x00518374[] = "\\Speech\\English\\Wavs\\CREST.WAV";
// GLOBAL: CMR2 0x00518354
char g_str0x00518354[] = "\\Speech\\English\\Wavs\\JUMP.WAV";
// GLOBAL: CMR2 0x00518334
char g_str0x00518334[] = "\\Speech\\English\\Wavs\\CARE.WAV";
// GLOBAL: CMR2 0x00518310
char g_str0x00518310[] = "\\Speech\\English\\Wavs\\CAUTION.WAV";
// GLOBAL: CMR2 0x005182ec
char g_str0x005182ec[] = "\\Speech\\English\\Wavs\\VERYLONG.WAV";
// GLOBAL: CMR2 0x005182cc
char g_str0x005182cc[] = "\\Speech\\English\\Wavs\\TURN.WAV";
// GLOBAL: CMR2 0x005182a8
char g_str0x005182a8[] = "\\Speech\\English\\Wavs\\NARROWS.WAV";
// GLOBAL: CMR2 0x00518284
char g_str0x00518284[] = "\\Speech\\English\\Wavs\\THROUGH.WAV";
// GLOBAL: CMR2 0x00518264
char g_str0x00518264[] = "\\Speech\\English\\Wavs\\LOGS.WAV";
// GLOBAL: CMR2 0x00518244
char g_str0x00518244[] = "\\Speech\\English\\Wavs\\GATE.WAV";
// GLOBAL: CMR2 0x00518224
char g_str0x00518224[] = "\\Speech\\English\\Wavs\\BRIDGE.WAV";
// GLOBAL: CMR2 0x00518204
char g_str0x00518204[] = "\\Speech\\English\\Wavs\\ROCKS.WAV";
// GLOBAL: CMR2 0x005181e4
char g_str0x005181e4[] = "\\Speech\\English\\Wavs\\POST.WAV";
// GLOBAL: CMR2 0x005181c4
char g_str0x005181c4[] = "\\Speech\\English\\Wavs\\DITCH.WAV";
// GLOBAL: CMR2 0x005181a4
char g_str0x005181a4[] = "\\Speech\\English\\Wavs\\MUD.WAV";
// GLOBAL: CMR2 0x00518184
char g_str0x00518184[] = "\\Speech\\English\\Wavs\\RUTS.WAV";
// GLOBAL: CMR2 0x00518164
char g_str0x00518164[] = "\\Speech\\English\\Wavs\\ICE.WAV";
// GLOBAL: CMR2 0x00518140
char g_str0x00518140[] = "\\Speech\\English\\Wavs\\BADCAMBER.WAV";
// GLOBAL: CMR2 0x00518120
char g_str0x00518120[] = "\\Speech\\English\\Wavs\\ROUGH.WAV";
// GLOBAL: CMR2 0x00518100
char g_str0x00518100[] = "\\Speech\\English\\Wavs\\SLIPPY.WAV";
// GLOBAL: CMR2 0x005180e0
char g_str0x005180e0[] = "\\Speech\\English\\Wavs\\INSIDE.WAV";
// GLOBAL: CMR2 0x005180bc
char g_str0x005180bc[] = "\\Speech\\English\\Wavs\\OUTSIDE.WAV";
// GLOBAL: CMR2 0x00518098
char g_str0x00518098[] = "\\Speech\\English\\Wavs\\DONTCUT.WAV";
// GLOBAL: CMR2 0x0051807c
char g_str0x0051807c[] = "\\Speech\\English\\Wavs\\OK.WAV";
// GLOBAL: CMR2 0x00518058
char g_str0x00518058[] = "\\Speech\\English\\Wavs\\STOP_DITCH.WAV";
// GLOBAL: CMR2 0x00518030
char g_str0x00518030[] = "\\Speech\\English\\Wavs\\ONTO_TARMAC.WAV";
// GLOBAL: CMR2 0x0051800c
char g_str0x0051800c[] = "\\Speech\\English\\Wavs\\ONTO_SNOW.WAV";
// GLOBAL: CMR2 0x00517fe4
char g_str0x00517fe4[] = "\\Speech\\English\\Wavs\\ONTO_GRAVEL.WAV";
// GLOBAL: CMR2 0x00517fc0
char g_str0x00517fc0[] = "\\Speech\\English\\Wavs\\ONTO_MUD.WAV";
// GLOBAL: CMR2 0x00517f9c
char g_str0x00517f9c[] = "\\Speech\\English\\Wavs\\ONTO_ICE.WAV";
// GLOBAL: CMR2 0x00517f78
char g_str0x00517f78[] = "\\Speech\\English\\Wavs\\JUMP_MAYBE.WAV";
// GLOBAL: CMR2 0x00517f54
char g_str0x00517f54[] = "\\Speech\\English\\Wavs\\STRAIGHT.WAV";
// GLOBAL: CMR2 0x00517f2c
char g_str0x00517f2c[] = "\\Speech\\English\\Wavs\\COUNTDOWN_3.WAV";
// GLOBAL: CMR2 0x00517f04
char g_str0x00517f04[] = "\\Speech\\English\\Wavs\\COUNTDOWN_2.WAV";
// GLOBAL: CMR2 0x00517edc
char g_str0x00517edc[] = "\\Speech\\English\\Wavs\\COUNTDOWN_1.WAV";

void Race_LoadCoDriverSoundBanks(void);

// Initializes the co-driver sound samples through the original tail-call entry.
// FUNCTION: CMR2 0x00416710
void Race_InitCoDriverSamples(void)
{
    Race_LoadCoDriverSoundBanks();
}

// FUNCTION: CMR2 0x00416770
void Race_LoadCoDriverSoundBanks(void)
{
    char buf[260];
    char *pDir;

    pDir = CInstallInfo::GetSoundsDir();
    g_coDriverFirstSample = Sound_GetSampleCount();
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518674);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518654);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518634);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518614);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x005185f4);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x005185d4);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x005185ac);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518588);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518568);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518548);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518528);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518508);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x005184e4);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x005184c8);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x005184a8);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518488);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x0051846c);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518450);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518434);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518414);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x005183f4);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x005183d4);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x005183b4);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518394);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518374);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518354);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518334);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518310);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x005182ec);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x005182cc);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x005182a8);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518284);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518264);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518244);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518224);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518204);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x005181e4);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x005181c4);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x005181a4);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518184);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518164);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518140);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518120);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518100);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x005180e0);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x005180bc);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518098);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x0051807c);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518058);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00518030);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x0051800c);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00517fe4);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00517fc0);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00517f9c);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00517f78);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00517f54);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00517f2c);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00517f04);
    Race_LoadNamedStageSample(buf);
    sprintf(buf, g_strPathConcat, pDir, g_str0x00517edc);
    Race_LoadNamedStageSample(buf);
    StageTiming_FreeStageFile2();
}

// ---- Race frame update (0x41c5a0) -------------------------------------------
// Scratch globals shared by the per-frame race update.
// GLOBAL: CMR2 0x00538124
int g_unk0x00538124;
// GLOBAL: CMR2 0x00538104
int g_raceUpdateCar;
// GLOBAL: CMR2 0x00537f04
int g_raceUpdatePlayer;
// GLOBAL: CMR2 0x00537fc8
int g_unk0x00537fc8;
// GLOBAL: CMR2 0x00537f28
int g_unk0x00537f28;
// GLOBAL: CMR2 0x00537fb8
BYTE g_unk0x00537fb8;
// GLOBAL: CMR2 0x00537fd0
short *g_unk0x00537fd0;
// GLOBAL: CMR2 0x00537fc4
short g_unk0x00537fc4;

unsigned int RallyData_GetFlag21(void);
void Car_DecrementContactTimers(short *pList, short count);
short *Car_GetOrder(void);
void RallyData_UpdateLocalAndNetworkSplitPositions(void);
int RallyData_IsChampionshipFinalStage(void);
int StageTiming_GetCarSplitTime(int car, int index);
int StageUI_GetRaceResultValue(void);
int View_IsPlaybackCameraActive(void);
int StageTiming_ConsumeDashRefreshLatch(void);
void Physics_UpdateRateHold(void);
int Car_UpdateEngineNoteFalloff(int *pOut);
void Car_UpdateAndRenderAll(void);
void GameMenu_RefreshResultsHeader(void);
int Race_GetPlayerRecordFieldC(BYTE index);
void SurfaceSound_UpdatePlayerLoops(int player, int listener);
void SurfaceSound_UpdateNearestLocalCarEngines(void);
void SurfaceSound_UpdateNearestNetworkCarEngines(void);
void CarInput_UpdateRaceOrderSlot(int slot);
void CarInput_UpdateWaitingStartSlot(int slot);
void StageObject_BrakeFinishedCarSlot(int slot);
void StageObject_ResetCarStartControls(int index);
int AI_UpdateAttractDrivingControls(int car, int preview);
void RallyData_UpdateOrderedCarRoutes(void);
void StageObjects_Update(void);
void StageTiming_ResetOrderedCarRoadBooks(void);
void StageTiming_UpdateAllCarLineCrossings(void);
void StageTiming_RecordSplitGroupTimes(int group);
void StageTiming_UpdateThirdObjectRampValues(void);
void StageObject_UpdateListedCarPhysicsAndWeather(short *pOrder, short count);
void NetRace_SetPlayerFlashIntensity(unsigned int player, int t, int check);
void NetRace_UpdatePlayerFlashTimer(unsigned int player, int check);
BYTE StageTiming_GetCheckpointField19(int index);
BYTE StageTiming_GetCheckpointField1A(int index);
int StageTiming_GetCheckpointGroupIndex(int index);
void NetRace_DrainPendingMessages(void);
void Sound_UpdateMusicStreaming(void);
void Surface_StopAndFreeSounds(void);
void Race_StopAllStageSounds(void);
void Race_FreePerCarSounds(void);
int InRaceMenu_IsBackPressed(unsigned short slot);
void InRaceMenu_Open(BYTE param1);
void StageObject_AdvanceFlaggedCarSlotCounters(int car);
void StageTiming_UpdateCarWheelAndExhaustEffects(int car);
void View_BlendCameraStates(unsigned char param1, int param2);
int View_GetActiveCameraMode(BYTE index);
BYTE View_GetActiveCameraFlags(BYTE index);
int NetRace_IsPlayerFadeActive(BYTE index);
void RallyData_UpdateCarSplitBar(int param_1, int param_2);
void Race_AdvanceRouteCalloutBlock(int player);
void Race_UpdateCarRouteAndWrongWayState(int param_1);
void Race_SetPartSurfaceAndRefreshVisuals(int param_1, int param_2);
void Race_UpdateFrameEffects(int param_1);
void Race_UpdatePlayerSelectionCursor(int param_1);
void View_UpdateModeSurface(unsigned int view);
void View_UpdateCamera(BYTE view);
unsigned int StageObject_GetAnyDeviceHeldButtons(void);
bool Input_PopQueuedCharacter(int *pOut);
void Input_ClearCharacterQueue(void);
void Sound_FreeAll(void);
void Particle_UpdateAll(SceneNode *pView);
void Replay_SetControlStateByte(BYTE value);
int Replay_GetActiveBufferState(void);
void Replay_RestartGhostCar(int a, int b);
int Replay_StopRecording(BYTE *pBuffer);
void WheelTrail_Update(int carIndex);
void Dash_Update(int player);

unsigned short Input_GetControllerSlotMapping(unsigned short slot);

// Per-frame update of one player's race view: advances the race, the car
// order, the dashboards and the replay recording. The car loop walks the whole
// order but only the entrants below the player count are driven; each car's
// call state selects its co-driver handling in the switch below.
// match 68%: below the 90% bar; the remaining diff is dominated by register
// allocation, not logic. MSVC6 keeps the register-allocated constant 0 in EDI
// and `first` in EBP here, while the original has the constant in EBP and
// `first` in EDI; that swap flips the operand order of every zero comparison
// (cmp edi,ebp vs cmp ebp,edi), the zero-argument pushes, and permutes the
// spill slots (S-0x14/S-0x10/S-0xc), which in turn moves the branch/block
// layout of the switch and of the car-loop tail. Calls, constants, statement
// order and control flow were checked instruction by instruction against the
// original; the layout of GameInfo0xa4.rallyStageRecordTimes (0x1214 in the
// original vs 0x654 in our struct) is a separate data-model gap reported to the
// integrator.
// FUNCTION: CMR2 0x0041c5a0
void Race_UpdatePlayerViewAndDrivenCars(BYTE param1, int param2)
{
    short first;
    int second;
    int count;
    int player;
    int flag;
    int key;
    int k;
    int value;
    int index;
    BYTE b1;
    int b2;
    BYTE b3;

    first = 0;
    count = RallyDataState() & 0xff;
    if (View_IsPlaybackCameraActive())
        first = 1;
    second = StageTiming_ConsumeDashRefreshLatch();
    Physics_UpdateRateHold();
    g_unk0x00538124 = Car_UpdateEngineNoteFalloff(&g_unk0x00537f60);
    g_unk0x00537f0c[4] = 0;
    for (; g_unk0x00537f0c[4] < g_unk0x00538124; g_unk0x00537f0c[4]++) {
        if (Diagnostics::observer)
            Diagnostics::observer->BeginTick(param1);
        player = Car_Get(0)->simulationStepsRemaining > 0;
        // The opponent can run at a different rate. Only consume a device's
        // presses when its player runs; axes/held buttons still poll each step.
        unsigned int readyDevices = 0;
        for (int inputPlayer = 0; inputPlayer < count && inputPlayer < 2; ++inputPlayer) {
            if (Car_Get(inputPlayer)->simulationStepsRemaining > 0) {
                readyDevices |= 1u << inputPlayer; // keyboard fallback
                unsigned int device = Input_GetControllerSlotMapping(inputPlayer);
                if (device < 8)
                    readyDevices |= 1u << device;
            }
        }
        CInput::UpdateRaceDevices(readyDevices);
        if (player != 0) {
            g_unk0x00537f24++;
            if (param2 != 0 && param1 == 10)
                GameMenu_RefreshResultsHeader();
        }
        for (g_raceUpdateCar = 0; g_raceUpdateCar < Car_GetOrderCount(); g_raceUpdateCar++) {
            if (Car_Get(g_raceUpdateCar)->simulationStepsRemaining > 0) {
                if (g_raceUpdateCar < (BYTE)RallyDataState()) {
                    g_unk0x00537fb8 = (char)((CallbackStateMachine *)StageUI_GetRaceResultTable())->records[g_raceUpdateCar].packedState;
                    if (Race_GetPlayerRecordField4(g_raceUpdateCar) != 0)
                        g_unk0x00537fb8 = (char)((ReplayStream *)g_playerReplayStreams[g_raceUpdateCar])->field_0x10c;
                } else {
                    if ((BYTE)((CallbackStateMachine *)StageUI_GetRaceResultTable())->records[0].packedState <= 7) {
                        g_unk0x00537fb8 = 7;
                    } else {
                        g_unk0x00537fb8 = 8;
                        if ((char)RallyData_GetSelectionFlag28() != 0 && (char)CGameInfo::GetSoundOptionBit30() != 0) {
                            if (Replay_GetActiveBufferState() == 0)
                                g_unk0x00537fb8 = 9;
                        } else if ((char)RallyData_GetSelectionFlag27() != 0 &&
                                   (char)CGameInfo::GetGameModeOptionBit19() == 0 &&
                                   StageTiming_GetCheckpointField1A(g_raceUpdateCar) != 0) {
                            g_unk0x00537fb8 = 9;
                        }
                    }
                }
                switch (g_unk0x00537fb8) {
                case 13:
                    CarInput_UpdateRaceOrderSlot(g_raceUpdateCar);
                    break;
                case 5:
                case 7:
                    g_unk0x0053811c = 0;
                    CarInput_UpdateWaitingStartSlot(g_raceUpdateCar);
                    break;
                case 10:
                    if ((char)RallyData_GetSelectionFlag26() != 0 && g_raceUpdateCar < count) {
                        if (Race_GetPlayerRecordField4(g_raceUpdateCar) == 0) {
                            AI_UpdateAttractDrivingControls(g_raceUpdateCar, 0);
                            break;
                        }
                    }
                    if (Race_GetPlayerRecordField4(g_raceUpdateCar) == 0)
                        StageObject_ResetCarStartControls(g_raceUpdateCar);
                    else if (first == 0)
                        CarInput_UpdateRaceOrderSlot(g_raceUpdateCar);
                    break;
                case 8:
                    if (first == 0)
                        CarInput_UpdateRaceOrderSlot(g_raceUpdateCar);
                    break;
                case 9:
                    if ((char)RallyData_GetSelectionFlag26() != 0) {
                        if (g_raceUpdateCar < count)
                            AI_UpdateAttractDrivingControls(g_raceUpdateCar, 0);
                        else
                            CarInput_UpdateRaceOrderSlot(g_raceUpdateCar);
                    } else {
                        if (g_unk0x00537f98[g_raceUpdateCar] == 0) {
                            StageObject_ResetCarStartControls(g_raceUpdateCar);
                        } else {
                            if (Race_GetPlayerRecordFieldC(g_raceUpdateCar) != 0 ||
                                Race_GetPlayerRecordField4(g_raceUpdateCar) != 0)
                                StageObject_BrakeFinishedCarSlot(g_raceUpdateCar);
                            else
                                StageObject_ResetCarStartControls(g_raceUpdateCar);
                        }
                    }
                    break;
                }
                if (g_raceUpdateCar < count) {
                    Dash_Update(g_raceUpdateCar);
                    if (g_unk0x00537fb8 == 5 || g_unk0x00537fb8 == 7)
                        RallyData_UpdateCarSplitBar(g_raceUpdateCar, 1);
                    else if (g_unk0x00537fb8 == 8 || g_unk0x00537fb8 == 9)
                        RallyData_UpdateCarSplitBar(g_raceUpdateCar, 0);
                    if (param1 == 8 || param1 == 9)
                        Race_UpdateCarRouteAndWrongWayState(g_raceUpdateCar);
                    else if (param1 == 7)
                        Race_AdvanceRouteCalloutBlock(g_raceUpdateCar);
                    if (g_unk0x00537fb8 == 8 || g_unk0x00537fb8 == 5 ||
                        (g_unk0x00537fb8 == 7 && (char)CGameInfo::GetGameModeOptionBit19() == 0)) {
                        b1 = InRaceMenu_IsBackPressed(g_raceUpdateCar);
                        b2 = Race_GetPlayerRecordField4(g_raceUpdateCar);
                        b3 = NetRace_IsPlayerFadeActive(g_raceUpdateCar);
                        if (b1 != 0 && b2 == 0 && b3 == 0 &&
                            (char)CGameInfo::GetGameInfoSessionFlag() == 0) {
                            InRaceMenu_Open(g_raceUpdateCar);
                            if ((char)RallyData_GetSelectionFlag26() != 0)
                                CSound::PauseMusicStreaming();
                            Surface_StopAndFreeSounds();
                            Race_StopAllStageSounds();
                            Race_FreePerCarSounds();
                        }
                    }
                }
            }
        }
        if ((char)CGameInfo::GetGameInfoSessionFlag() != 0 && player != 0 &&
            (char)((CallbackStateMachine *)StageUI_GetRaceResultTable())->records[0].packedState == 13) {
            if (g_unk0x00537f78[5] != 0) {
                Input_ClearCharacterQueue();
                g_unk0x00537f78[5] = 0;
            }
            if (StageObject_GetAnyDeviceHeldButtons() != 0 || Input_PopQueuedCharacter(&key) != 0) {
                Replay_ResetBufferIfActive((ReplayStream *)g_playerReplayStreams[0]);
                g_unk0x00537fc8 = 0;
                for (; g_unk0x00537fc8 < *StageUI_GetRaceResultTable(); g_unk0x00537fc8++)
                    CGame::PromoteCallbackEntryByRule((CallbackStateMachine *)StageUI_GetRaceResultTable(), g_unk0x00537fc8, 1, 3);
                Race_SetFlag3810D();
            }
        }
        for (g_raceUpdatePlayer = 0; g_raceUpdatePlayer < count; g_raceUpdatePlayer++) {
            if (Car_Get(g_raceUpdatePlayer)->simulationStepsRemaining > 0) {
                if (param2 != 0)
                    Race_UpdatePlayerSelectionCursor(g_raceUpdatePlayer);
                if (View_GetActiveCameraMode((BYTE)g_raceUpdatePlayer) == 7)
                    View_UpdateModeSurface((BYTE)g_raceUpdatePlayer);
            }
        }
        if (first == 0 || second != 0) {
            if (Diagnostics::observer)
                Diagnostics::observer->ControlsReady();
            Car_UpdateAndRenderAll();
        }
        if (player != 0) {
            DustPlume_AdvanceWorld(SceneryFormat::Step(Car_Get(0)->simulationRateHz));
            // Speed motes are no longer spawned (the user rejected the look); the speed cue is the radial speed lines.
            // Air motes are different: slow drifting atmosphere dust at any speed (video.air_motes, default off).
            AirMotes_Step(SceneryFormat::Step(Car_Get(0)->simulationRateHz));
            SnowGlint_Step(SceneryFormat::Step(Car_Get(0)->simulationRateHz));
            FogDynamics_Step(SceneryFormat::Step(Car_Get(0)->simulationRateHz));
            SceneryPack_Advance(SceneryFormat::Step(Car_Get(0)->simulationRateHz)); // 25Hz authored units, not render frames
            for(int reactSlot=0;reactSlot<Car_GetOrderCount()&&reactSlot<8;reactSlot++){Car *reactCar=Car_Get(reactSlot);if(reactCar)SceneryPack_SetReactionSource(reactSlot,reactCar->position.x,reactCar->position.z,reactCar->speed,true);}
            StageObjects_Update();
        }
        for (g_raceUpdatePlayer = 0; g_raceUpdatePlayer < Car_GetOrderCount(); g_raceUpdatePlayer++) {
            if (Car_Get(g_raceUpdatePlayer)->simulationStepsRemaining > 0)
                NetRace_UpdatePlayerFlashTimer((BYTE)g_raceUpdatePlayer, 0);
        }
        if (player != 0) {
            RallyData_UpdateOrderedCarRoutes();
            StageTiming_ResetOrderedCarRoadBooks();
            StageTiming_UpdateAllCarLineCrossings();
            if ((char)RallyData_GetFlag21() != 0) {
                if (StageUI_GetRaceResultValue() == 4)
                    StageTiming_RecordSplitGroupTimes(StageUI_GetRaceEndEventCount() & 0xff);
                else
                    StageTiming_RecordSplitGroupTimes(0);
            }
            StageTiming_UpdateThirdObjectRampValues();
        }
        for (g_raceUpdatePlayer = 0; g_raceUpdatePlayer < count; g_raceUpdatePlayer++) {
            if (Car_Get((BYTE)View_GetActiveCameraFlags((BYTE)g_raceUpdatePlayer))->simulationStepsRemaining > 0)
                View_UpdateCamera((BYTE)g_raceUpdatePlayer);
        }
        if (player != 0) {
            if (first == 0 || second != 0)
                Particle_UpdateAll(g_viewNodes[g_raceUpdatePlayer]);
            if (CGameInfo::GetConfiguredGameMode() == 0xc && StageTiming_GetCheckpointField19(0) != 0) {
                NetRace_SendStageAndOverallTimes(StageTiming_GetCarSplitTime(0, StageTiming_GetCheckpointGroupIndex(0)));
                NetPlayers_UpdateBestRaceTime(StageTiming_GetCarSplitTime(0, StageTiming_GetCheckpointGroupIndex(0)));
            }
            if ((char)RallyData_GetFlag24() != 0 && (char)RallyData_GetSelectionFlag28() != 0 &&
                StageTiming_GetCheckpointField19(0) != 0) {
                Replay_ResetBufferIfActive((ReplayStream *)g_playerReplayStreams[0]);
                Replay_StopRecording(g_playerReplayStreams[0]);
                if (CGameInfo::GetSoundOptionBit30() != 0)
                    Replay_ResetActiveBufferState();
                flag = CGameInfo::GetGraphicsOptionBits27To28() == 0;
                value = StageTiming_GetCarSplitTime(0, StageTiming_GetCheckpointGroupIndex(0));
                index = (RallyData_GetSelectionBits10To11() & 0xff) * 3 + (RallyData_GetSelectionBits12To13() & 0xff);
                if (value <= (int)((CGameInfo::GetPlayerRecordTable(flag)->rallyStageRecordTimes[index].value
                                    >> 7) & 0xffff)) {
                    index = (RallyData_GetSelectionBits10To11() & 0xff) * 3 + (RallyData_GetSelectionBits12To13() & 0xff);
                    CGameInfo::GetPlayerRecordTable(flag)->rallyStageRecordTimes[index].value =
                        (value & 0xffff) << 7 |
                        CGameInfo::GetPlayerRecordTable(flag)->rallyStageRecordTimes[index].value & 0xff80007f;
                    k = RallyData_GetDriverRecordSelectionValue(0);
                    index = (RallyData_GetSelectionBits10To11() & 0xff) * 3 + (RallyData_GetSelectionBits12To13() & 0xff);
                    CGameInfo::GetPlayerRecordTable(flag)->rallyStageRecordTimes[index].value =
                        k & 0x3f |
                        CGameInfo::GetPlayerRecordTable(flag)->rallyStageRecordTimes[index].value & 0xffffffc0;
                    sprintf(CFrontend::m_stringDest, g_strGrp0x005192b0, Race_GetStagePathBuffer38444());
                    if (g_playerReplayStreams[0] != NULL)
                        Replay_Save(g_playerReplayStreams[0], CFrontend::m_stringDest);
                    if (CGameInfo::GetSoundOptionBit30() != 0) {
                        Replay_SetControlStateByte(0);
                        Replay_SwapPendingSlotValue((int **)g_playerReplayStreams, 0, 0);
                    }
                }
                Replay_InitCarStreamState((ReplayStream *)g_playerReplayStreams[0], 0, 0);
                if (CGameInfo::GetSoundOptionBit30() != 0)
                    Replay_RestartGhostCar(0, 0);
            }
        }
        if (CGameInfo::IsInRaceMenuOpen() == 0 && param1 >= 7) {
            if ((BYTE)RallyData_IsChampionshipFinalStage() != 0 && Race_GetPlayerRecordField4(0) == 0) {
                if (g_unk0x00537f2c == 0) {
                    Sound_FreeAll();
                    g_unk0x00537f2c = 1;
                }
            } else {
                k = ((BYTE)RallyDataState() == 1 && (char)RallyData_GetFlag25() != 0 &&
                     (char)CGameInfo::GetGameModeOptionBit19() == 0 && CGameInfo::GetConfiguredGameMode() != 3);
                for (g_raceUpdatePlayer = 0;
                     g_raceUpdatePlayer < (BYTE)RallyDataState() + k;
                     g_raceUpdatePlayer++) {
                    if (!(k != 1)) {
                        SurfaceSound_UpdatePlayerLoops(g_raceUpdatePlayer, 0);
                        Race_SetPartSurfaceAndRefreshVisuals(g_raceUpdatePlayer, 0);
                        Race_UpdateCarSoundDistanceVolumes((CallbackStateMachine *)g_raceUpdatePlayer, 0);
                    } else {
                        SurfaceSound_UpdatePlayerLoops(g_raceUpdatePlayer, g_raceUpdatePlayer);
                        Race_SetPartSurfaceAndRefreshVisuals(g_raceUpdatePlayer, g_raceUpdatePlayer);
                        Race_UpdateCarSoundDistanceVolumes((CallbackStateMachine *)g_raceUpdatePlayer, g_raceUpdatePlayer);
                    }
                }
            }
            if ((char)CGameInfo::GetGameModeOptionBit19() != 0) {
                if (CGameInfo::GetConfiguredGameMode() == 0xb || CGameInfo::GetConfiguredGameMode() == 0xc)
                    SurfaceSound_UpdateNearestNetworkCarEngines();
            } else if ((CGameInfo::GetConfiguredGameMode() == 5 || CGameInfo::GetConfiguredGameMode() == 6) &&
                       (BYTE)RallyDataState() == 1) {
                SurfaceSound_UpdateNearestLocalCarEngines();
            }
        }
        if (param2 == 0) {
            for (g_raceUpdatePlayer = 0; g_raceUpdatePlayer < (BYTE)RallyDataState(); g_raceUpdatePlayer++) {
            }
        }
        Car_DecrementContactTimers(Car_GetOrder(), Car_GetOrderCount());
        if (Diagnostics::observer)
            Diagnostics::observer->EndTick();
    }
    if (CGameInfo::GetConfiguredGameMode() == 8 || CGameInfo::GetConfiguredGameMode() == 9 ||
        CGameInfo::GetConfiguredGameMode() == 0xb)
        RallyData_UpdateLocalAndNetworkSplitPositions();
    g_unk0x00537f28 = Race_ReadState37F60();
    if (first != 0)
        g_unk0x00537f28 = 0x10000;
    Race_UpdateFrameEffects(g_unk0x00537f28);
    for (g_raceUpdatePlayer = 0; g_raceUpdatePlayer < (BYTE)RallyDataState(); g_raceUpdatePlayer++) {
        View_BlendCameraStates((BYTE)g_raceUpdatePlayer,
                     Car_Get((BYTE)View_GetActiveCameraFlags((BYTE)g_raceUpdatePlayer))->simulationInterpolation);
    }
    g_unk0x00537fd0 = Car_GetOrder();
    g_unk0x00537fc4 = Car_GetOrderCount();
    StageObject_UpdateListedCarPhysicsAndWeather(g_unk0x00537fd0, g_unk0x00537fc4);
    for (g_raceUpdatePlayer = 0; g_raceUpdatePlayer < Car_GetOrderCount(); g_raceUpdatePlayer++)
        NetRace_SetPlayerFlashIntensity(g_raceUpdatePlayer, g_unk0x00537f28, 0);
    if (first == 0 || second != 0) {
        if (CGameInfo::IsInRaceMenuOpen() == 0) {
            g_unk0x00537fc4 = Car_GetOrderCount();
            for (g_unk0x00537fc8 = 0; g_unk0x00537fc8 < g_unk0x00537fc4; g_unk0x00537fc8++) {
                StageObject_AdvanceFlaggedCarSlotCounters(g_unk0x00537fc8);
                WheelTrail_Update(g_unk0x00537fc8);
                if ((char)RallyData_GetSelectionFlag26() == 0 &&
                    CGameInfo::GetConfiguredGameMode() != 0xc && CGameInfo::GetConfiguredGameMode() != 0xb)
                    StageTiming_UpdateCarWheelAndExhaustEffects(g_unk0x00537fc8);
            }
        }
    }
    if ((char)CGameInfo::GetGameModeOptionBit19() != 0)
        NetRace_DrainPendingMessages();
    if ((char)RallyData_GetSelectionFlag26() != 0)
        Sound_UpdateMusicStreaming();
}
