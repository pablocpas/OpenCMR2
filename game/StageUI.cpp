#include <stddef.h>
#include "FloatToInt.h"
#include "RaceCallbackStorage.h"
#include "NetworkLeaderboards.h"
#include "NetPlayers.h"
#include "Game.h"
#include "StageUI.h"
#include <cstdio>
#include <cstring>
#include "Frontend.h"
#include "GenericFileLoader.h"
#include "Font.h"
#include "Sprite.h"
#include "Graphics.h"
#include "RallyData.h"
#include "InstallInfo.h"
#include "StageTiming.h"
#include "GameInfo.h"
#include "Texture.h"
#include "Input.h"
#include "Menu.h"
#include "PauseMenus.h"

// FUNCTION: CMR2 0x00415bc0
void StageUI_NoOpMapEvent(int, short *)
{
}

// FUNCTION: CMR2 0x00415f40
void StageUI_NoOpMapRelease(void)
{
}

// GLOBAL: CMR2 0x00517e14
char g_positiveSymbol[2] = "+";

// GLOBAL: CMR2 0x00517e18
char g_negativeSymbol[2] = "-";

// GLOBAL: CMR2 0x00537084
char g_emptyString[4];
// GLOBAL: CMR2 0x00517df4
char g_strGapUnknown[12] = "%s--:--.--";
// GLOBAL: CMR2 0x00517e00
char g_strGapTime[20] = "%s%02d:%02d.%02d";
// GLOBAL: CMR2 0x005170f4
BYTE g_gapTextColour[4] = { 0xff, 0xff, 0xff, 0xff };

// Formats a time gap as "+mm:ss.hh" and draws it, boxed when param_9 is set.
// match 84%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00415bd0
void FormatGapToLeader(int iLeaderGap, unsigned int fontIndex, unsigned char param_3, int x, int y, void *pColour, unsigned int flags, char *pcNegPosSymbol, int drawBox)
{
    char text[16];
    short rect[4];
    int px;

    if (pcNegPosSymbol != NULL) {
        sprintf((char *)&pcNegPosSymbol, pcNegPosSymbol);
    } else {
        sprintf((char *)&pcNegPosSymbol, g_emptyString);
    }
    px = (int)g_pGraphics->resX * x >> 0x10;
    if (iLeaderGap != -1) {
        int q = iLeaderGap / 6000;
        int r = iLeaderGap - q * 6000;
        sprintf(text, g_strGapTime, &pcNegPosSymbol, q % 100, r / 100, r - (r / 100) * 100);
    } else {
        sprintf(text, g_strGapUnknown, &pcNegPosSymbol);
    }
    if (drawBox != 0) {
        rect[0] = (short)((int)g_pGraphics->resX * x >> 0x10) -
                  (short)((int)g_pGraphics->resX / 0xa0) - 2;
        rect[2] = (short)((int)g_pGraphics->resX / 0x50) +
                  (short)Font_GetTextWidth(fontIndex, (BYTE *)text) + 4;
        rect[1] = (short)((int)g_pGraphics->resY * y >> 0x10) -
                  (short)((int)g_pGraphics->resY / 0xf0) - 2;
        rect[3] = (short)((int)g_pGraphics->resY / 0x140) +
                  (short)Font_GetTextHeight(fontIndex, text) + 4;
        Sprite_FillRect(&g_pGraphics->field309_0x150, rect, (BYTE *)pColour, 2);
    }
    Font_DrawText(fontIndex, text, px, (int)g_pGraphics->resY * y >> 0x10,
                  (int *)g_gapTextColour, flags);
}

// FUNCTION: CMR2 0x00415db0
void PrepareFormatGapToLeader(int iLeaderGap, unsigned char param_2, unsigned int param_3, unsigned char param_4, int param_5, int param_6, void *param_7, unsigned int param_8, BOOL bIsAhead, int param_10)
{
    if (bIsAhead)
    {
        FormatGapToLeader(iLeaderGap, param_3, param_4, param_5, param_6, param_7, param_8, g_negativeSymbol, param_10);
        return;
    }

    FormatGapToLeader(iLeaderGap, param_3, param_4, param_5, param_6, param_7, param_8, g_positiveSymbol, param_10);
}

// GLOBAL: CMR2 0x00537dcc
BYTE g_unk0x00537dcc;

// Release callback of StageSound_ResetRecordsAndRegisterCleanup.
// FUNCTION: CMR2 0x00418fe0
int StageUI_ReleaseHUDResources(void)
{
    g_unk0x00537dcc = 0;
    return 1;
}

// Resets the eight stage sound records and registers their callback once.
// match 60%: remaining diff is codegen-only. The original keeps the loop cursor
// on the countOld field (edx = 0x53784c, `cmp edx,0x537dec; jl`); MSVC6 always
// biases our IV (countOld with the struct pointer, count with an int cursor) and
// pays an extra `lea ecx,[eax-0xa4]; cmp ecx,0x537d48; jl`, so the body matches
// but the compare + register names differ.
// FUNCTION: CMR2 0x00418f20
void StageSound_ResetRecordsAndRegisterCleanup(void)
{
    int car;
    int i;

    g_unk0x00537660 = 0;
    memset(g_surfacePrevB, 0, sizeof(g_surfacePrevB));
    memset(g_surfacePrevA, 0, sizeof(g_surfacePrevA));
    memset(g_surfacePrevC, 0, sizeof(g_surfacePrevC));
    memset(g_surfacePrevD, 0, sizeof(g_surfacePrevD));
    for (car = 0; car < 8; car++) {
        for (i = 0; i < 10; i++) {
            g_carSoundStates[car].handle[i] = -1;
            g_carSoundStates[car].id[i] = -1;
        }
        g_carSoundStates[car].countOld = 0;
        g_carSoundStates[car].count = 0;
        memset(g_carSoundStates[car].slotState, 0xff, sizeof(g_carSoundStates[car].slotState));
        g_carSoundStates[car].stateOld = -1;
        g_carSoundStates[car].state = -1;
        g_carSoundStates[car].pattern = 0x19;
    }
    if (g_unk0x00537dcc == 0) {
        CGame::RegisterCallback(StageUI_ReleaseHUDResources, NULL);
        g_unk0x00537dcc = 1;
    }
}


// Each sound state can redirect to a shared pattern. The base sound ids are
// filled in when the stage sound bank is loaded.
// GLOBAL: CMR2 0x00518ca8
StageSoundPattern g_stageSoundPatterns[31] = {
    {{1, 1, 0, 0}, 0, {0, 0, 0}, 1}, // 0
    {{1, 1, 0, 0}, 1, {0, 0, 0}, 1}, // 1
    {{3, 3, 0, 0}, 2, {0, 0, 0}, 1}, // 2
    {{1, 1, 0, 0}, 3, {0, 0, 0}, 1}, // 3
    {{3, 1, 0, 0}, 4, {0, 0, 0}, 1}, // 4
    {{0, 0, 0, 0}, 4, {0, 0, 0}, 1}, // 5
    {{5, 3, 1, 1}, 6, {0, 0, 0}, 2}, // 6
    {{0, 0, 0, 0}, 1, {0, 0, 0}, 1}, // 7
    {{0, 0, 0, 0}, 1, {0, 0, 0}, 1}, // 8
    {{0, 0, 0, 0}, 6, {0, 0, 0}, 1}, // 9
    {{0, 0, 0, 0}, 6, {0, 0, 0}, 1}, // 10
    {{0, 0, 0, 0}, 6, {0, 0, 0}, 1}, // 11
    {{0, 0, 0, 0}, 0, {0, 0, 0}, 1}, // 12
    {{3, 1, 1, 1}, 13, {0, 0, 0}, 2}, // 13
    {{1, 1, 0, 0}, 14, {0, 0, 0}, 1}, // 14
    {{0, 0, 0, 0}, 14, {0, 0, 0}, 1}, // 15
    {{0, 0, 0, 0}, 25, {0, 0, 0}, 1}, // 16
    {{1, 1, 1, 1}, 17, {0, 0, 0}, 2}, // 17
    {{1, 1, 1, 1}, 18, {0, 0, 0}, 2}, // 18
    {{1, 1, 0, 0}, 19, {0, 0, 0}, 1}, // 19
    {{0, 0, 0, 0}, 19, {0, 0, 0}, 1}, // 20
    {{0, 0, 0, 0}, 19, {0, 0, 0}, 1}, // 21
    {{3, 1, 1, 1}, 22, {0, 0, 0}, 2}, // 22
    {{0, 0, 0, 0}, 17, {0, 0, 0}, 1}, // 23
    {{1, 1, 4, 4}, 24, {0, 0, 0}, 4}, // 24
    {{2, 2, 4, 4}, 25, {0, 0, 0}, 3}, // 25
    {{0, 0, 0, 0}, 0, {0, 0, 0}, 1}, // 26
    {{0, 0, 0, 0}, 6, {0, 0, 0}, 1}, // 27
    {{0, 0, 0, 0}, 1, {0, 0, 0}, 1}, // 28
    {{1, 1, 4, 4}, 29, {0, 0, 0}, 4}, // 29
    {{0, 0, 0, 0}, 17, {0, 0, 0}, 1} // 30
};

void CarSound_PlaySlot(int channel, int sound, int slot, int volume, int flags);
void CarSound_StopSlot(int channel, int slot, char clear);
int CarSound_PickDifferentSampleIndex(int exclude, int count);
BYTE NetRace_GetRaceSoundMode(void);

#define STAGE_PLAY_PRIMARY(slot) do { \
    CarSound_PlaySlot(channel, pPattern->base[(BYTE)g_carSoundViewSet[channel]] + \
                 CarSound_PickDifferentSampleIndex(-1, (int)pPattern->choices[(BYTE)g_carSoundViewSet[channel]]), slot, 0, 0); \
} while (0)
#define STAGE_PLAY_SECONDARY(slot) do { \
    CarSound_PlaySlot(channel, pPattern->base[(BYTE)g_carSoundViewSet[channel] + 2] + \
                 CarSound_PickDifferentSampleIndex(-1, (int)pPattern->choices[(BYTE)g_carSoundViewSet[channel] + 2]), slot, 0, 0); \
} while (0)
#define STAGE_PLAY_DIRECT(slot) do { \
    CarSound_PlaySlot(channel, pPattern->base[(BYTE)g_carSoundViewSet[channel]], slot, 0, 0); \
} while (0)
#define STAGE_STOP(slot) CarSound_StopSlot(channel, slot, 1)
#define STAGE_PLAY_EXTRA() do { STAGE_PLAY_SECONDARY(1); STAGE_PLAY_SECONDARY(2); STAGE_PLAY_SECONDARY(3); } while (0)
#define STAGE_STOP_EXTRA() do { STAGE_STOP(1); STAGE_STOP(2); STAGE_STOP(3); } while (0)

// GLOBAL: CMR2 0x0051900c
char g_strSurfSndOut[] = "%s\\SurfSnd\\30_o00.wav";
// GLOBAL: CMR2 0x00519024
char g_strSurfSndIn[] = "%s\\SurfSnd\\30_i00.wav";
// GLOBAL: CMR2 0x0051903c
char g_strSurfSndSkidOut[] = "%s\\SurfSnd\\%.2d_sko%.2d.wav";
// GLOBAL: CMR2 0x00519058
char g_strSurfSndSkidIn[] = "%s\\SurfSnd\\%.2d_ski%.2d.wav";
// GLOBAL: CMR2 0x00519074
char g_strSurfSndRollOut[] = "%s\\SurfSnd\\%.2d_o%.2d.wav";
// GLOBAL: CMR2 0x00519090
char g_strSurfSndRollIn[] = "%s\\SurfSnd\\%.2d_i%.2d.wav";

extern int g_unk0x00537564;
int Sound_GetSampleCount(void);
BOOL Sound_LoadSample(char *name, BYTE flags, GenericFile *pFile);
void StageTiming_FreeStageFile5(void);

// Loads the surface sounds of the stage: for every surface group in use its
// rolling (inside/outside) and skidding (inside/outside) variations, recording
// where each set starts; then the two fallback samples.
// FUNCTION: CMR2 0x00418ff0
void StageSound_LoadSurfaceVariations(void)
{
    char *pDir;
    StageSoundPattern *pPattern;
    int group;
    int i;

    pDir = CInstallInfo::GetSoundsDir();
    g_stageSoundLoaded = 0;
    g_stageSoundFirst = Sound_GetSampleCount();
    for (group = 0, pPattern = g_stageSoundPatterns; group < 31; pPattern++, group++) {
        if (g_stageSoundUsed[group] != 0) {
        pPattern->base[0] = g_stageSoundLoaded + g_stageSoundFirst;
        pPattern->base[1] = pPattern->base[0] + pPattern->choices[0];
        pPattern->base[2] = pPattern->base[1] + pPattern->choices[1];
        pPattern->base[3] = pPattern->choices[2] + pPattern->base[2];
        for (i = 0; i < pPattern->choices[0]; i++) {
            sprintf(CFrontend::m_stringDest, g_strSurfSndRollIn, pDir, group, i);
            Sound_LoadSample(CFrontend::m_stringDest, 0, (GenericFile *)StageTiming_GetStageFile5());
            g_stageSoundLoaded++;
        }
        for (i = 0; i < pPattern->choices[1]; i++) {
            sprintf(CFrontend::m_stringDest, g_strSurfSndRollOut, pDir, group, i);
            Sound_LoadSample(CFrontend::m_stringDest, 0, (GenericFile *)StageTiming_GetStageFile5());
            g_stageSoundLoaded++;
        }
        for (i = 0; i < pPattern->choices[2]; i++) {
            sprintf(CFrontend::m_stringDest, g_strSurfSndSkidIn, pDir, group, i);
            Sound_LoadSample(CFrontend::m_stringDest, 0, (GenericFile *)StageTiming_GetStageFile5());
            g_stageSoundLoaded++;
        }
        for (i = 0; i < pPattern->choices[3]; i++) {
            sprintf(CFrontend::m_stringDest, g_strSurfSndSkidOut, pDir, group, i);
            Sound_LoadSample(CFrontend::m_stringDest, 0, (GenericFile *)StageTiming_GetStageFile5());
            g_stageSoundLoaded++;
        }
        }
    }
    g_unk0x00537564 = Sound_GetSampleCount();
    sprintf(CFrontend::m_stringDest, g_strSurfSndIn, pDir);
    Sound_LoadSample(CFrontend::m_stringDest, 0, (GenericFile *)StageTiming_GetStageFile5());
    g_stageSoundLoaded++;
    g_unk0x00537dc8 = Sound_GetSampleCount();
    sprintf(CFrontend::m_stringDest, g_strSurfSndOut, pDir);
    Sound_LoadSample(CFrontend::m_stringDest, 0, (GenericFile *)StageTiming_GetStageFile5());
    g_stageSoundLoaded++;
    StageTiming_FreeStageFile5();
    StageSound_ResetRecordsAndRegisterCleanup();
}

// Applies one stage sound state to its active channel.  The switch mirrors
// the combinations of continuous, secondary and network-gated sound slots.
// match 35%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00419200
void StageUI_ApplySoundState(int channel, RaceCarSoundState *pState)
{
    RaceCarSoundState *pRaceCarSoundState = pState;
    StageSoundPattern *pPattern = NULL;
    short pattern = pRaceCarSoundState->state;

    if (pattern != -1)
        pPattern = &g_stageSoundPatterns[(int)g_stageSoundPatterns[pattern].redirect];

    switch (pRaceCarSoundState->pattern) {
    case 1:
        if (pPattern != NULL) {
            STAGE_PLAY_PRIMARY(5);
        }
        return;
    case 2:
        if (pPattern != NULL) {
            STAGE_PLAY_PRIMARY(5);
            STAGE_PLAY_SECONDARY(7);
        }
        return;
    case 3:
        if (pPattern != NULL) {
            STAGE_PLAY_DIRECT(4);
            STAGE_PLAY_SECONDARY(0);
            if (NetRace_GetRaceSoundMode() == 0) STAGE_PLAY_EXTRA();
        }
        return;
    case 4:
        if (pPattern != NULL) {
            STAGE_PLAY_PRIMARY(5);
            STAGE_PLAY_SECONDARY(0);
            if (NetRace_GetRaceSoundMode() == 0) STAGE_PLAY_EXTRA();
        }
        return;
    case 5:
        STAGE_STOP(4);
        return;
    case 6:
        if (pPattern != NULL) {
            if (NetRace_GetRaceSoundMode() != 0) STAGE_STOP(4);
            STAGE_PLAY_PRIMARY(5);
        }
        return;
    case 7:
        if (pPattern != NULL) {
            if (NetRace_GetRaceSoundMode() != 0) STAGE_STOP(4);
            STAGE_PLAY_PRIMARY(5);
            STAGE_PLAY_SECONDARY(7);
        }
        return;
    case 8:
        STAGE_STOP(4);
        if (pPattern == NULL) return;
        if (NetRace_GetRaceSoundMode() == 0) STAGE_PLAY_EXTRA();
        STAGE_PLAY_DIRECT(4);
        STAGE_PLAY_SECONDARY(0);
        return;
    case 9:
        STAGE_STOP(4);
        if (pPattern == NULL) return;
        if (NetRace_GetRaceSoundMode() == 0) STAGE_PLAY_EXTRA();
        STAGE_PLAY_PRIMARY(5);
        STAGE_PLAY_SECONDARY(0);
        return;
    case 10:
        STAGE_STOP(4);
        STAGE_STOP(6);
        return;
    case 11:
        if (pPattern == NULL) return;
        if (NetRace_GetRaceSoundMode() != 0) {
            STAGE_STOP(4);
            STAGE_STOP(6);
        }
        STAGE_PLAY_PRIMARY(5);
        return;
    case 12:
        if (pPattern == NULL) return;
        if (NetRace_GetRaceSoundMode() != 0) {
            STAGE_STOP(4);
            STAGE_STOP(6);
        }
        STAGE_PLAY_PRIMARY(5);
        STAGE_PLAY_SECONDARY(7);
        return;
    case 13:
        STAGE_STOP(4);
        STAGE_STOP(6);
        if (pPattern != NULL) {
            STAGE_PLAY_DIRECT(4);
            STAGE_PLAY_SECONDARY(0);
            if (NetRace_GetRaceSoundMode() == 0) STAGE_PLAY_EXTRA();
        }
        return;
    case 14:
        STAGE_STOP(4);
        STAGE_STOP(6);
        if (pPattern != NULL) {
            STAGE_PLAY_PRIMARY(5);
            STAGE_PLAY_SECONDARY(0);
            if (NetRace_GetRaceSoundMode() == 0) STAGE_PLAY_EXTRA();
        }
        return;
    case 16:
        STAGE_STOP(4);
        STAGE_STOP(0);
        if (NetRace_GetRaceSoundMode() == 0) STAGE_STOP_EXTRA();
        if (pPattern != NULL) STAGE_PLAY_PRIMARY(5);
        return;
    case 17:
        STAGE_STOP(4);
        STAGE_STOP(0);
        if (NetRace_GetRaceSoundMode() == 0) STAGE_STOP_EXTRA();
        if (pPattern != NULL) {
            STAGE_PLAY_PRIMARY(5);
            STAGE_PLAY_SECONDARY(7);
        }
        return;
    case 19:
        STAGE_STOP(4);
        if (pPattern != NULL) STAGE_PLAY_PRIMARY(5);
        return;
    case 15:
    case 20:
        STAGE_STOP(4);
        STAGE_STOP(0);
        if (NetRace_GetRaceSoundMode() == 0) STAGE_STOP_EXTRA();
        return;
    case 21:
        STAGE_STOP(0);
        if (NetRace_GetRaceSoundMode() == 0) STAGE_STOP_EXTRA();
        else STAGE_STOP(4);
        if (pPattern != NULL) STAGE_PLAY_PRIMARY(5);
        return;
    case 22:
        STAGE_STOP(4);
        STAGE_STOP(0);
        if (NetRace_GetRaceSoundMode() == 0) STAGE_STOP_EXTRA();
        if (pPattern != NULL) {
            STAGE_PLAY_PRIMARY(5);
            STAGE_PLAY_SECONDARY(7);
        }
        return;
    case 23:
        STAGE_STOP(4);
        if (pPattern != NULL) STAGE_PLAY_DIRECT(4);
        return;
    case 24:
        STAGE_STOP(4);
        if (pPattern != NULL) STAGE_PLAY_PRIMARY(5);
        return;
    }
    return;
}
#undef STAGE_PLAY_PRIMARY
#undef STAGE_PLAY_SECONDARY
#undef STAGE_PLAY_DIRECT
#undef STAGE_STOP
#undef STAGE_PLAY_EXTRA
#undef STAGE_STOP_EXTRA


// Frame/tick counters shared by the in-game UI (0x537df0..0x537efc).
// GLOBAL: CMR2 0x00537dd0
RaceCallbackStorage g_raceCallbackStorage;
// GLOBAL: CMR2 0x00537df0
int g_unk0x00537df0;
// GLOBAL: CMR2 0x00537ef4
BYTE g_raceEndLatch;
// GLOBAL: CMR2 0x00537ef5
BYTE g_unk0x00537ef5;
// GLOBAL: CMR2 0x00537ef6
BYTE g_raceEndEventCount;
// GLOBAL: CMR2 0x00537ef8
int g_raceEndPending;
// GLOBAL: CMR2 0x00537efc
int g_raceEndState;

// FUNCTION: CMR2 0x0041b300
void StageUI_ClearRaceEndLatch(void)
{
    g_raceEndLatch = 0;
}

// FUNCTION: CMR2 0x0041b310
void StageUI_SetRaceEndPending(void)
{
    g_raceEndPending = 1;
}

// FUNCTION: CMR2 0x0041b320
int StageUI_GetRaceEndState(void)
{
    return g_raceEndState;
}

// FUNCTION: CMR2 0x0041b330
void StageUI_ClearRaceEndState(void)
{
    g_raceEndState = 0;
}

// FUNCTION: CMR2 0x0041b340
void StageUI_RecordRaceEndEvent(char bFlag)
{
    if (bFlag != 0)
        g_unk0x00537ef5 = 1;
    g_raceEndEventCount++;
}

// FUNCTION: CMR2 0x0041b360
void StageUI_ResetRaceEndEventCount(void)
{
    g_raceEndEventCount = 0;
}

// FUNCTION: CMR2 0x0041b370
BYTE StageUI_GetRaceEndEventCount(void)
{
    return g_raceEndEventCount;
}

// FUNCTION: CMR2 0x0041b380
int StageUI_GetRaceResultValue(void)
{
    return g_unk0x00537df0;
}

// FUNCTION: CMR2 0x0041b390
BYTE *StageUI_GetRaceResultTable(void)
{
    return ((BYTE *)&g_raceCallbackStorage.machine);
}

void NetPlayers_BuildFinalClassification(void);
BOOL Network_GetSessionStateFlag(void);
unsigned int NetPlayers_GetClassificationTime(int index);
int NetPlayers_GetClassificationCount(void);
void NetPlayers_SendPublishedLeaderboard(void);
struct NetClassification *NetPlayers_GetClassificationRecord(int index);
void NetworkLeaderboard_AddWins(int index, char *name, int wins);

// End of a network race: gives a leaderboard win to every driver with the
// winning time and registers the others with no win.
// match 84%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0041b3a0
void StageUI_PublishNetworkRaceWins(void)
{
    int i;

    NetPlayers_BuildFinalClassification();
    if (CNetworkLeaderboards::GetLeaderboardId() == -1)
        return;
    if (Network_GetSessionStateFlag() && NetPlayers_GetClassificationTime(0) != -1) {
        NetworkLeaderboard_AddWins(CNetworkLeaderboards::GetLeaderboardId(), NetPlayers_GetClassificationRecord(0)->name, 1);
        for (i = 1; i < NetPlayers_GetClassificationCount(); i++) {
            if (NetPlayers_GetClassificationTime(i) == NetPlayers_GetClassificationTime(0))
                NetworkLeaderboard_AddWins(CNetworkLeaderboards::GetLeaderboardId(), NetPlayers_GetClassificationRecord(i)->name, 1);
            else
                NetworkLeaderboard_AddWins(CNetworkLeaderboards::GetLeaderboardId(), NetPlayers_GetClassificationRecord(i)->name, 0);
        }
    }
    for (i = 0; i < NetPlayers_GetClassificationCount(); i++)
        NetworkLeaderboard_AddWins(CNetworkLeaderboards::GetLeaderboardId(), NetPlayers_GetClassificationRecord(i)->name, 0);
    NetPlayers_SendPublishedLeaderboard();
}


// GLOBAL: CMR2 0x0051c988
BYTE g_barBackColour[4] = { 156, 180, 172, 255 };
// GLOBAL: CMR2 0x0051c990
BYTE g_barTextColour[4] = { 255, 255, 255, 255 };
extern PauseMenus g_pauseMenus;
// The pause menu being shown: main or stageData (Menu_Update returns the next).
// GLOBAL: CMR2 0x0051c97c
Menu *g_pPauseMenu = &g_pauseMenus.main;

// FUNCTION: CMR2 0x00475a40
Menu *StageUI_GetCurrentPauseMenu(void)
{
    return g_pPauseMenu;
}
// GLOBAL: CMR2 0x0058ca90
PauseMenus g_pauseMenus;
// GLOBAL: CMR2 0x0058cf64
int g_unk0x0058cf64;
// GLOBAL: CMR2 0x0058cf7c
int g_knockoutRaceState;

void StageUI_NoOpPauseItemEvent(Menu *pMenu, int param);
void StageUI_RequestPauseMenuExit(Menu *pMenu, int param);
void StageObject_DrawInRacePauseMenu(Menu *pMenu);
void StageObject_DrawPauseStageDataPanel(Menu *pMenu);
extern int g_unk0x0058cf6c;

// Builds the two menus of the in-race pause screen: the first one holds the
// entry 0x63 that opens the second (and 0x72 back), plus the entries whose
// actions are the patch callbacks.
// FUNCTION: CMR2 0x00473360
void StageUI_BuildPauseMenus(void)
{
    g_pPauseMenu = &g_pauseMenus.main;
    Menu_Init(&g_pauseMenus.main, 0, 0, 0, NULL, NULL, 1, 0, 1);
    Menu_AddItemType4(&g_pauseMenus.main, 0, 0x13, (MenuItemAction)StageUI_NoOpPauseItemEvent, 0);
    Menu_AddItemType2(&g_pauseMenus.main, 0, 0x63, &g_pauseMenus.stageData, 0, 1);
    Menu_SetCallbacks(&g_pauseMenus.main, NULL, NULL, (MenuCallback)StageObject_DrawInRacePauseMenu, NULL);
    Menu_ValidateCursor(&g_pauseMenus.main, 0);
    Menu_Init(&g_pauseMenus.stageData, 0, 0, 0, NULL, NULL, 1, 0, 1);
    Menu_AddItemType4(&g_pauseMenus.stageData, 0, 0x72, (MenuItemAction)StageUI_RequestPauseMenuExit, 0);
    Menu_AddItemType2(&g_pauseMenus.stageData, 0, 0x73, &g_pauseMenus.main, 0, 1);
    Menu_SetCallbacks(&g_pauseMenus.stageData, NULL, NULL, (MenuCallback)StageObject_DrawPauseStageDataPanel, NULL);
    Menu_ValidateCursor(&g_pauseMenus.stageData, 0);
    g_unk0x0058cf6c = 1;
}

// Item callbacks of the menu built by 0x473360.
// FUNCTION: CMR2 0x00473450
void StageUI_NoOpPauseItemEvent(Menu *pMenu, int param)
{
    g_unk0x0058cf64 = 1;
}

// FUNCTION: CMR2 0x00473460
void StageUI_RequestPauseMenuExit(Menu *pMenu, int param)
{
    g_knockoutRaceState = 7;
}

// Draws the championship banner across the top of the screen: the championship
// name, the class it is run in and, on the longer championships, the round.
// match 79%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00475a50
void StageUI_DrawChampionshipBar(void)
{
    short screen[4];
    short bar[4];
    unsigned int *pState;
    unsigned int flags;
    int x;
    int width;

    screen[0] = 0;
    screen[1] = 0;
    screen[2] = (short)g_pGraphics->resX;
    screen[3] = (short)g_pGraphics->resY;
    bar[0] = 0;
    bar[2] = 2;
    bar[1] = (short)((int)(g_pGraphics->resY * 0x1b) / 0x1e0);
    bar[3] = (short)((int)(g_pGraphics->resY * 0x29) / 0x1e0);
    pState = RallyData_GetChampionshipState();
    flags = *pState;
    x = (int)(g_pGraphics->resX * 0x1e) / 0x280;
    Sprite_FillRect(&g_pGraphics->field309_0x150, screen, g_barBackColour, 4);
    sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x7c));
    CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
    Font_DrawText(2, CFrontend::m_stringDest, x, (int)(g_pGraphics->resY * 0x39) / 0x1e0,
                  (int *)g_barTextColour, 0x11);
    width = Font_GetTextWidth(2, (BYTE *)CFrontend::m_stringDest);
    x = (int)(g_pGraphics->resX * 5) / 0x280 + x + width;
    bar[0] = (short)x;
    Sprite_FillRect(&g_pGraphics->field309_0x150, bar, g_barTextColour, 1);
    x = x + 2 + (int)(g_pGraphics->resX * 5) / 0x280;

    switch ((flags >> 3) & 7) {
    case 1:
        sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x4b), 1);
        break;
    case 2:
        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(0x4c));
        break;
    case 3:
        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(0x4d));
        break;
    case 4:
        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(0x4e));
        break;
    }

    CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
    Font_DrawText(2, CFrontend::m_stringDest, x, (int)(g_pGraphics->resY * 0x39) / 0x1e0,
                  (int *)g_barTextColour, 0x11);

    if (g_pPauseMenu == &g_pauseMenus.main) {
        if (g_knockoutRaceState > 1 && g_knockoutRaceState < 6) {
            unsigned int round;

            width = Font_GetTextWidth(2, (BYTE *)CFrontend::m_stringDest);
            x = width + (int)(g_pGraphics->resX * 5) / 0x280 + x;
            bar[0] = (short)x;
            bar[2] = 2;
            bar[1] = (short)((int)(g_pGraphics->resY * 0x1b) / 0x1e0);
            bar[3] = (short)((int)(g_pGraphics->resY * 0x29) / 0x1e0);
            Sprite_FillRect(&g_pGraphics->field309_0x150, bar, g_barTextColour, 1);
            if (g_knockoutRaceState == 3 || (*pState & 0x400000) != 0) {
                round = (*pState >> 0xc) & 0xf;
            } else {
                round = ((*pState >> 0xc) & 0xf) + 1;
            }
            sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x4f), round);
            Font_DrawText(2, CFrontend::m_stringDest, x + 2 + (int)(g_pGraphics->resX * 5) / 0x280,
                          (int)(g_pGraphics->resY * 0x39) / 0x1e0, (int *)g_barTextColour, 0x11);
        }
    } else {
        width = Font_GetTextWidth(2, (BYTE *)CFrontend::m_stringDest);
        x = width + (int)(g_pGraphics->resX * 5) / 0x280 + x;
        bar[0] = (short)x;
        bar[2] = 2;
        bar[1] = (short)((int)(g_pGraphics->resY * 0x1b) / 0x1e0);
        bar[3] = (short)((int)(g_pGraphics->resY * 0x29) / 0x1e0);
        Sprite_FillRect(&g_pGraphics->field309_0x150, bar, g_barTextColour, 1);
        Font_DrawText(2, CFrontend::GetTextString(0x7b), x + 2 + (int)(g_pGraphics->resX * 5) / 0x280,
                      (int)(g_pGraphics->resY * 0x39) / 0x1e0, (int *)g_barTextColour, 0x11);
    }
}

// GLOBAL: CMR2 0x0051c9f4
BYTE g_gridBackColour[4] = { 0, 0, 0, 255 };
// GLOBAL: CMR2 0x0051c9f8
BYTE g_gridColour2[4] = { 153, 255, 0, 255 };
// GLOBAL: CMR2 0x0051c9fc
BYTE g_gridColour1[4] = { 255, 178, 0, 255 };
// GLOBAL: CMR2 0x0051ca00
char g_stageGrid[7][0x294] = {
    { // Pattern 0: 20 rows of 33 cells.
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0,
    },
    { // Pattern 1: 20 rows of 33 cells.
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    },
    { // Pattern 2: 20 rows of 33 cells.
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    },
    { // Pattern 3: 20 rows of 33 cells.
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    },
    { // Pattern 4: 20 rows of 33 cells.
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    },
    { // Pattern 5: 20 rows of 33 cells.
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    },
    { // Pattern 6: 20 rows of 33 cells.
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    },
};

// Draws the little stage grid: a background panel and one cell per set entry.
// match 65%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00477f70
void StageUI_DrawStageGrid(int unused, int set)
{
    short rect[4];
    int n;
    int i;
    int cell;
    int resX;
    short top;
    short cellW;
    short cellH;
    short left;

    resX = (int)(g_pGraphics->resX * 6) / 0x280;
    cell = (int)(g_pGraphics->resY * 6) / 0x1e0;
    left = (short)((int)(g_pGraphics->resX * 0x140) / 0x280);
    if (RallyData_IsHeadToHeadRaceMode() != 0) {
        top = (short)(((int)g_pGraphics->resY + cell * -0x14) / 2) + 4;
    } else {
        top = (short)((int)(g_pGraphics->resY << 0xc) >> 0x10);
    }
    cellW = (short)resX;
    cellH = (short)cell;
    rect[1] = top;
    rect[2] = cellW * 0x14;
    rect[3] = cellH * 0x15 - 1;
    rect[0] = left + cellW * -9;
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, g_gridBackColour, 3);

    rect[2] = (short)((int)(g_pGraphics->resX * 4) / 0x280);
    i = 8;
    rect[3] = (short)((int)(g_pGraphics->resY * 4) / 0x1e0);
    do {
        char *pCell = &g_stageGrid[0][0] + set * 0x294 + i;
        int n = 0x14;

        rect[0] = ((short)i - 0x10) * cellW + left;
        rect[1] = top;
        do {
            if (*pCell != 0) {
                BYTE *pColour = g_gridColour2;

                if (*pCell != 2) {
                    pColour = g_gridColour1;
                }
                Sprite_FillRect(&g_pGraphics->field309_0x150, rect, pColour, 3);
            }
            rect[1] = rect[1] + cellH;
            pCell = pCell + 0x21;
            n--;
        } while (n != 0);
        i++;
    } while (i < 0x1a);
}

// GLOBAL: CMR2 0x0058ca88
CallbackStateMachine *g_unk0x0058ca88;
extern int g_unk0x0058cf6c;
void Race_SetFlag3810D(void);

// Callback of the "retire" item: flags the championship and promotes the cars.
// FUNCTION: CMR2 0x004734f0
void StageUI_RetireFromRace(Menu *pMenu)
{
    int i;

    *RallyData_GetChampionshipState() |= 0x800000;
    g_unk0x0058cf6c = 0;
    Race_SetFlag3810D();
    for (i = 0; i < g_unk0x0058ca88->count; i++)
        CGame::PromoteCallbackEntryByRule(g_unk0x0058ca88, i, 1, 2);
}

extern int g_knockoutRaceState;
extern PauseMenus g_pauseMenus;
void Graphics_StartShapedInterpolationTimer(BYTE *p, int, int, int, int, int, BYTE);

// Fade callback of the in-race state machine: releases the scene callbacks and
// restarts the fade out of the stage objects.
// FUNCTION: CMR2 0x00473540
void StageUI_ReleaseSceneAndFadeStage(BYTE index)
{
    int i;

    i = 0;
    if (g_unk0x0058ca88->count > 0) {
        do {
            CGame::PromoteCallbackEntryByRule(g_unk0x0058ca88, i, 0, 2);
            i++;
        } while (i < (int)g_unk0x0058ca88->count);
    }
    CGameInfo::SetInputAndGamePaused(0);
    StageUI_ClearRaceEndState();
    g_knockoutRaceState = 5;
    Graphics_StartShapedInterpolationTimer(&g_pauseMenus.textTimer, 2, 7, 0, 0, 0x10000, 0);
}

// Co-driver arrow sprites: 64-pixel cells for low resolutions, 102-pixel
// cells when the screen is at least 1024 wide and the texture fits.
// GLOBAL: CMR2 0x00517e30
SpriteRect g_arrowRectsSmall[8] = {
    { 0, 0, 64, 64 }, { 64, 0, 64, 64 }, { 128, 0, 64, 64 }, { 192, 0, 64, 64 },
    { 0, 64, 64, 64 }, { 64, 64, 64, 64 }, { 128, 64, 64, 64 }, { 192, 64, 64, 64 },
};
// GLOBAL: CMR2 0x00517e70
SpriteRect g_arrowRectsLarge[8] = {
    { 1, 0, 102, 102 }, { 103, 0, 102, 102 }, { 205, 0, 102, 102 }, { 307, 0, 102, 102 },
    { 1, 102, 102, 102 }, { 103, 102, 102, 102 }, { 205, 102, 102, 102 }, { 307, 109, 107, 92 },
};
// GLOBAL: CMR2 0x00517eb0
char g_strArrowsTexture[] = "\\NEWIMAGE\\OSD\\CODRIV\\ARROWS4.TGA";
// GLOBAL: CMR2 0x00517d98
char g_strPathConcat[] = "%s%s";
// GLOBAL: CMR2 0x00537088
SpriteRect *g_pArrowRects;
// GLOBAL: CMR2 0x00537098
Texture *g_arrowTexture;

// Loads the co-driver arrow texture and picks the sprite layout for the resolution.
// FUNCTION: CMR2 0x004165e0
void StageUI_LoadCoDriverArrows(void)
{
    char path[MAX_PATH];
    bool loaded;

    sprintf(path, g_strPathConcat, CInstallInfo::GetTexturesDirectory(), g_strArrowsTexture);
    g_arrowTexture = CTexture::FindLoadTexture((GenericFile *)StageTiming_GetStageFile1(), path, &loaded, 0, 0, 0);
    if (CGameInfo::GetScreenWidth() >= 0x400 && CFrontend::IsTextureWidthSupported(0x400) && CFrontend::IsTextureHeightSupported(0x400)) {
        g_pArrowRects = g_arrowRectsLarge;
        return;
    }
    g_pArrowRects = g_arrowRectsSmall;
}

// GLOBAL: CMR2 0x0058ca8c
BYTE g_unk0x0058ca8c[4];

void Input_TranslatePedalsToMenuKeys(void);
void Input_MergeAssignedJoystickButtons(int slot, DeviceInfo *pOut);
BYTE Graphics_IsRegisteredTimerRunning(BYTE *p);

// Runs the current in-race menu for one frame (with no input while one of
// the two key latches is held) and follows the menu it returns.
// FUNCTION: CMR2 0x00473470
void StageUI_UpdatePauseMenu(void)
{
    DeviceInfo *pDev;
    Menu *next;

    CInput::UpdateAllAvailableDevices();
    Input_TranslatePedalsToMenuKeys();
    if (!Graphics_IsRegisteredTimerRunning(&g_pauseMenus.bracketTimer) && !Graphics_IsRegisteredTimerRunning(g_unk0x0058ca8c)) {
        pDev = CInput::GetAvailableDeviceRecord(0);
        Input_MergeAssignedJoystickButtons(0, pDev);
        next = Menu_Update(g_pPauseMenu, pDev->pressed);
    } else {
        next = Menu_Update(g_pPauseMenu, 0);
    }
    if (next != 0) {
        g_pauseMenus.main.cursor = 0;
        g_pauseMenus.stageData.cursor = 1;
        g_pPauseMenu = next;
    }
}

// Stage map layout: route extent, centre and scale.
// GLOBAL: CMR2 0x00536bf0
FixVector g_stageMapCentre;
// GLOBAL: CMR2 0x00536c24
int g_unk0x00536c24;
// GLOBAL: CMR2 0x00536fe0
int g_unk0x00536fe0;
// GLOBAL: CMR2 0x0053705c
unsigned int g_stageMapScale;

void RallyData_GetRouteNodeGroundPosition(int index, int *pOut);
int RallyData_GetRouteAvailabilityState(void);

// Fits the stage map to the route: centre and scale of the larger extent.
// match 37%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00415e30
void StageUI_FitMapToRoute(void)
{
    int maxX = -0x7d000000;
    int maxZ = -0x7d000000;
    int minZ = 0x7d000000;
    int minX = 0x7d000000;
    int point[3];
    int i;
    int dx;
    int dz;

    g_unk0x00536fe0 = 0;
    if ((unsigned int)RallyData_GetRouteAvailabilityState() < 100)
        g_unk0x00536c24 = 1;
    else
        g_unk0x00536c24 = (unsigned int)RallyData_GetRouteAvailabilityState() / 100 + 1;
    for (i = 0; i < RallyData_GetRouteAvailabilityState(); i++) {
        RallyData_GetRouteNodeGroundPosition(i, point);
        if (point[0] > maxX)
            maxX = point[0];
        if (point[0] < minX)
            minX = point[0];
        if (point[2] > maxZ)
            maxZ = point[2];
        if (point[2] < minZ)
            minZ = point[2];
    }
    dx = maxX - minX;
    dz = maxZ - minZ;
    if (dz > dx)
        g_stageMapScale = FixDiv(0xa3d7, dz);
    else
        g_stageMapScale = FixDiv(0xa3d7, dx);
    g_stageMapCentre.y = 0;
    g_stageMapCentre.x = (minX + maxX) / 2;
    g_stageMapCentre.z = (maxZ + minZ) / 2;
}

extern Texture *g_challengeMapTexture;
extern Texture *g_challengeBlobTexture;
extern double g_minus65536;
int NetPlayers_GetResultPlayerIndex(int index);
int NetPlayers_GetResultField4ByID(int id);
int NetPlayers_GetPlayerID(int index);
BYTE *NetPlayers_GetPlayerNameColour(int id);
DWORD Network_GetLocalPlayerID(void);
int StageTiming_GetCheckpointField0(int index);
int Stage_GetNextCheckpoint(int index);
short Car_GetOrderCount(void);
struct Car *Car_Get(int index);
int RallyData_GetActiveCarRaceRecordField0(Car *p);
int RallyData_IsHeadToHeadRaceMode(void);
unsigned char RallyDataState(void);
unsigned char RallyData_GetFlag25(void);

// Dot colour of each car on the stage map, in race order.
// GLOBAL: CMR2 0x005170fc
BYTE g_stageMapCarColours[8][4] = {
    { 0x1a, 0x3b, 0xa4, 0xff }, { 0xb2, 0x0f, 0x0f, 0xff }, { 0x2c, 0x6f, 0x15, 0xff }, { 0x6c, 0x23, 0x86, 0xff },
    { 0xb4, 0x04, 0x79, 0xff }, { 0xbd, 0x57, 0x03, 0xff }, { 0xff, 0xea, 0x00, 0xff }, { 0xff, 0x8d, 0x00, 0xff },
};
// GLOBAL: CMR2 0x00517e1c
char g_strMapNumber[] = "%d";

// Screen position of a route point on the stage map (map = x, y, w, h).
// t = FixMul(d.x or -d.z, g_stageMapScale) + 0x8000: the route point scaled to 0..1.
#define MAP_X(t, map) \
    ((FixMul(t, (int)FTOI(__int64, X87S_MUL((double)(map)[2], CGraphics::m_65536))) - \
      (int)FTOI(__int64, X87S_MUL((double)(map)[0], g_minus65536))) >> 16)
#define MAP_Y(t, map) \
    ((FixMul(t, (int)FTOI(__int64, X87S_MUL((double)(map)[3], CGraphics::m_65536))) - \
      (int)FTOI(__int64, X87S_MUL((double)(map)[1], g_minus65536))) >> 16)

// Stage map of the HUD: the route thumbnail, a dot per car (network players,
// or every car in race order) and the number of each local player.
// FUNCTION: CMR2 0x00415f50
void StageUI_DrawHudRouteMap(int car, short *pRect)
{
    int point[3];
    FixVector d;
    short dotSrc[4];
    short mapSrc[4];
    short dot[4];
    short map[4];
    char number[6];
    BYTE colour[4];
    int netColour;
    int i;
    int id;
    int index;
    int x;
    int y;
    int t;

    colour[0] = 0xff;
    colour[1] = 0xff;
    colour[2] = 0xff;
    colour[3] = 0xff;
    dotSrc[0] = 0;
    dotSrc[1] = 0;
    dotSrc[2] = 0;
    dotSrc[3] = 0;
    mapSrc[0] = 0;
    mapSrc[1] = 0;
    mapSrc[2] = 0;
    mapSrc[3] = 0;
    if (g_challengeMapTexture != NULL) {
        mapSrc[2] = g_challengeMapTexture->width - 1;
        mapSrc[3] = g_challengeMapTexture->height - 1;
        dotSrc[2] = g_challengeBlobTexture->width - 1;
        dotSrc[3] = g_challengeBlobTexture->height - 1;
        if (RallyData_IsHeadToHeadRaceMode() && CGameInfo::IsSplitBarEnabled())
            return;
        map[0] = (short)((int)(g_pGraphics->resX * 0x731) >> 16) + pRect[0];
        map[2] = (short)((int)(g_pGraphics->resX * 0x3f9e) >> 16);
        map[2] = (short)((int)(mapSrc[2] * g_pGraphics->resX) / 640);
        map[1] = (short)((int)(g_pGraphics->resY * 0xea3d) >> 16);
        map[3] = (short)((int)(g_pGraphics->resY * 0x5319) >> 16);
        map[3] = (short)((int)(mapSrc[3] * g_pGraphics->resY) / 480);
        map[1] -= map[3];
        Sprite_Queue((SpriteRect *)mapSrc, (SpriteRect *)map, g_challengeMapTexture, 2, 0, NULL, NULL, colour, 8);
    }
    if (CGameInfo::GetConfiguredGameMode() == 11 || CGameInfo::GetConfiguredGameMode() == 12 ||
        (CGameInfo::GetGameModeOptionBit19() && (char)RallyData_GetFlag25() && CGameInfo::GetConfiguredGameMode() != 10)) {
        for (i = 0; i < 8; i++) {
            id = NetPlayers_GetResultPlayerIndex(i);
            if (id == -1)
                continue;
            if (id == -2) {
                index = StageTiming_GetCheckpointField0(0);
                netColour = *(int *)NetPlayers_GetPlayerNameColour(Network_GetLocalPlayerID());
            } else {
                index = NetPlayers_GetResultField4ByID(NetPlayers_GetPlayerID(id));
                netColour = *(int *)NetPlayers_GetPlayerNameColour(NetPlayers_GetPlayerID(id));
            }
            RallyData_GetRouteNodeGroundPosition(Stage_GetNextCheckpoint(index), point);
            d.x = point[0] - g_stageMapCentre.x;
            d.y = point[1] - g_stageMapCentre.y;
            d.z = point[2] - g_stageMapCentre.z;
            t = FixMul(d.x, g_stageMapScale) + 0x8000;
            x = MAP_X(t, map);
            t = FixMul(-d.z, g_stageMapScale) + 0x8000;
            y = MAP_Y(t, map);
            dot[0] = (short)x - (short)((int)(g_pGraphics->resX * 5) / 640);
            dot[1] = (short)y - (short)((int)(g_pGraphics->resY * 5) / 480);
            dot[2] = (short)((int)(g_pGraphics->resX * 10) / 640);
            dot[3] = (short)((int)(g_pGraphics->resY * 10) / 480);
            Sprite_Queue((SpriteRect *)dotSrc, (SpriteRect *)dot, g_challengeBlobTexture, 2, 0, NULL, NULL,
                         (BYTE *)&netColour, 8);
        }
    } else {
        for (i = Car_GetOrderCount() - 1; i >= 0; i--) {
            index = RallyData_GetActiveCarRaceRecordField0(Car_Get(i));
            RallyData_GetRouteNodeGroundPosition(index, point);
            d.x = point[0] - g_stageMapCentre.x;
            d.y = point[1] - g_stageMapCentre.y;
            d.z = point[2] - g_stageMapCentre.z;
            t = FixMul(d.x, g_stageMapScale) + 0x8000;
            x = MAP_X(t, map);
            t = FixMul(-d.z, g_stageMapScale) + 0x8000;
            y = MAP_Y(t, map);
            dot[0] = (short)x - (short)((int)(g_pGraphics->resX * 6) / 640);
            dot[1] = (short)y - (short)((int)(g_pGraphics->resY * 6) / 480);
            dot[2] = (short)((int)(g_pGraphics->resX * 12) / 640);
            dot[3] = (short)((int)(g_pGraphics->resY * 12) / 480);
            Sprite_Queue((SpriteRect *)dotSrc, (SpriteRect *)dot, g_challengeBlobTexture, 2, 0, NULL, NULL,
                         g_stageMapCarColours[i], 8);
        }
    }
    for (i = 0; i < (BYTE)RallyDataState(); i++) {
        sprintf(number, g_strMapNumber, i + 1);
        index = StageTiming_GetCheckpointField0(i);
        RallyData_GetRouteNodeGroundPosition(Stage_GetNextCheckpoint(index), point);
        d.x = point[0] - g_stageMapCentre.x;
        d.y = point[1] - g_stageMapCentre.y;
        d.z = point[2] - g_stageMapCentre.z;
        t = FixMul(d.x, g_stageMapScale) + 0x8000;
        x = MAP_X(t, map);
        t = FixMul(-d.z, g_stageMapScale) + 0x8000;
        y = MAP_Y(t, map);
        Font_DrawText(0, number, x, y + 7, (int *)g_gapTextColour, 0x12);
    }
}
#undef MAP_X
#undef MAP_Y

BYTE RallyData_IsDriverRecordUsable(BYTE param1);
BYTE RallyData_GetDriverGridRow(BYTE param1, char param2);
BYTE RallyData_SetCategoryAwardBit(int index, int bit);
void RallyData_RecordChampionshipMedal(int index, int place);
void RallyData_RecordRallyMedal(int index, int item, int level);
void RallyData_RecordArcadeMedal(int index, int player, int difficulty);
char RallyData_HasOnlySilverMedals(int param_1);
bool RallyData_HasCategoryAward(int index, int bit, char check);
int RallyTiming_GetChampionshipPointsByPosition(int index);
int StageTiming_GetFinishOrderSecondaryEntry(int index);
int RallyTiming_GetStagePositionOfDriver(int iDriver);
int RallyTiming_GetOverallPositionOfDriver(int iDriver);
extern int g_playerEventAwardFlags[];
extern int g_unk0x00537f78[];
extern ptrdiff_t g_unk0x00537f0c[];

// Bit `(country + 1) / 2` of each of the three event groups of the second game
// info flag word, indexed by [stage * 4 + country / 2].
// GLOBAL: CMR2 0x00519198
int g_unk0x00519198[12] = {
    0x516034, 0x960000, 0xff, 0xacb49c, 7, 0x10, 0xf, 0x14, 0xc, 0x11, 0x13, 0x12
};

// Per-driver event progress: clears the flags of every driver slot, sets the
// stage and award bits of the current event, stores the place-based awards and
// finally updates the game callback group when a driver completed an event.
// The logic is faithful; the residual diff is codegen shape: the original
// addresses the two game-info flag words as a bitfield struct (its masked
// xor/carry read-modify-writes for the 1/2/4-bit fields) and keeps the loop
// counter in edi (ours lands in ebx), which shifts many operand encodings.
// match 57%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0041b460
void StageUI_UpdateDriverEventAwards(void)
{
    unsigned int *pFlags;
    BYTE flag;
    BYTE bAward;
    int stage;
    unsigned int bit;
    int pos;
    int i;

    flag = 0;
    pFlags = CGameInfo::GetGameInfoField9CAddress();
    for (i = 0; i < (BYTE)CGameInfo::GetConfiguredPlayerCount(); i++) {
        g_playerEventAwardFlags[i] = 0;
        g_unk0x00537f78[i] = 0;
        bAward = RallyData_IsDriverRecordUsable((BYTE)i);
        pos = StageTiming_GetDriverSlot(i);
        switch (CGameInfo::GetConfiguredGameMode()) {
        case 0:
        case 1:
            if (CGameInfo::GetConfiguredGameMode() == 0) {
                if ((BYTE)RallyDataCountryIndex() == 7 && RallyDataStageIndex() == 0xa &&
                    RallyTiming_GetStagePositionOfDriver(pos) <= 2)
                    RallyData_RecordChampionshipMedal(i, RallyTiming_GetStagePositionOfDriver(pos));
                if (CGameInfo::GetConfiguredDifficulty() == 1 && (BYTE)RallyDataCountryIndex() == 7 &&
                    RallyDataStageIndex() == 0xa && RallyTiming_GetStagePositionOfDriver(pos) == 0 && (pFlags[0] & 1) == 0) {
                    pFlags[0] |= 1;
                    g_playerEventAwardFlags[i] |= 1;
                    CGame::SetStartupFlag();
                }
                if (CGameInfo::GetConfiguredDifficulty() == 0 && (BYTE)RallyDataCountryIndex() == 7 &&
                    RallyDataStageIndex() == 0xa && RallyTiming_GetStagePositionOfDriver(pos) == 0 && bAward == 0 &&
                    RallyData_SetCategoryAwardBit(i, 0x15)) {
                    g_unk0x00537f78[i] = 0x15;
                    g_playerEventAwardFlags[i] |= 0x80;
                }
            }
            stage = (BYTE)RallyData_GetDriverGridRow((BYTE)RallyDataCountryIndex(),
                                       (char)CGameInfo::GetConfiguredDifficulty()) - 1;
            if (RallyDataStageIndex() == stage || RallyDataStageIndex() == 0xa) {
                if (RallyTiming_GetOverallPositionOfDriver(pos) <= 2)
                    RallyData_RecordRallyMedal(i, RallyDataCountryIndex() & 0xff,
                                 RallyTiming_GetOverallPositionOfDriver(pos));
            }
            stage = (BYTE)RallyData_GetDriverGridRow((BYTE)RallyDataCountryIndex(),
                                       (char)CGameInfo::GetConfiguredDifficulty()) - 1;
            if (RallyDataStageIndex() == stage || RallyDataStageIndex() == 0xa) {
                if (RallyTiming_GetOverallPositionOfDriver(pos) <= 5) {
                    switch (CGameInfo::GetConfiguredDifficulty()) {
                    case 2:
                        if (CGameInfo::GetConfiguredGameMode() == 0) {
                            if ((BYTE)RallyDataCountryIndex() + 1 == (pFlags[0] >> 0x10 & 0xf) &&
                                (BYTE)RallyDataCountryIndex() != 7) {
                                pFlags[0] = ((pFlags[0] & 0xffff0000) + 0x10000 ^ pFlags[0]) & 0xf0000 ^ pFlags[0]; // field++
                                g_playerEventAwardFlags[i] |= 8;
                                CGame::SetStartupFlag();
                            }
                        }
                        if (((BYTE)RallyDataCountryIndex() + 1) % 2 != 0) {
                            bit = 1 << (((BYTE)RallyDataCountryIndex() + 1) / 2);
                            if ((pFlags[1] >> 10 & bit & 0x1f) == 0) {
                                pFlags[1] |= (bit & 0x1f) | ((bit & 0x1f) << 5) | ((bit & 0x1f) << 10);
                                g_playerEventAwardFlags[i] |= 0x40;
                                CGame::SetStartupFlag();
                            }
                        }
                        if (RallyTiming_GetOverallPositionOfDriver(pos) != 0)
                            break;
                        if ((BYTE)RallyDataCountryIndex() % 2 == 0)
                            goto setflag;
                    award:
                        if (bAward == 0 &&
                            RallyData_SetCategoryAwardBit(i, g_unk0x00519198[
                                ((BYTE)RallyDataCountryIndex() >> 1) +
                                CGameInfo::GetConfiguredDifficulty() * 4])) {
                            g_playerEventAwardFlags[i] |= 0x80;
                            g_unk0x00537f78[i] = g_unk0x00519198[
                                ((BYTE)RallyDataCountryIndex() >> 1) +
                                CGameInfo::GetConfiguredDifficulty() * 4];
                        }
                        break;
                    case 1:
                        if (CGameInfo::GetConfiguredGameMode() == 0) {
                            if ((BYTE)RallyDataCountryIndex() + 1 == (pFlags[0] >> 12 & 0xf) &&
                                (BYTE)RallyDataCountryIndex() != 7) {
                                pFlags[0] = ((pFlags[0] & 0xfffff000) + 0x1000 ^ pFlags[0]) & 0xf000 ^ pFlags[0]; // field++
                                if ((pFlags[0] & 0xf00) < (pFlags[0] & 0xf000) >> 4)
                                    pFlags[0] = (pFlags[0] & 0xfffff0ff) | ((pFlags[0] & 0xf000) >> 4);
                                g_playerEventAwardFlags[i] |= 4;
                                CGame::SetStartupFlag();
                            }
                        }
                        if (((BYTE)RallyDataCountryIndex() + 1) % 2 != 0) {
                            bit = 1 << (((BYTE)RallyDataCountryIndex() + 1) / 2);
                            if ((pFlags[1] >> 5 & bit & 0x1f) == 0) {
                                pFlags[1] |= (bit & 0x1f) | ((bit & 0x1f) << 5);
                                g_playerEventAwardFlags[i] |= 0x20;
                                CGame::SetStartupFlag();
                            }
                        }
                        if (RallyTiming_GetOverallPositionOfDriver(pos) != 0)
                            break;
                        if ((BYTE)RallyDataCountryIndex() % 2 != 0)
                            goto award;
                    setflag:
                        flag = 1;
                        break;
                    case 0:
                        if (CGameInfo::GetConfiguredGameMode() == 0) {
                            if ((BYTE)RallyDataCountryIndex() + 1 == (pFlags[0] >> 8 & 0xf) &&
                                (BYTE)RallyDataCountryIndex() != 7) {
                                pFlags[0] = ((pFlags[0] & 0xffffff00) + 0x100 ^ pFlags[0]) & 0xf00 ^ pFlags[0]; // field++
                                g_playerEventAwardFlags[i] |= 2;
                                CGame::SetStartupFlag();
                            }
                        }
                        if (((BYTE)RallyDataCountryIndex() + 1) % 2 != 0) {
                            bit = 1 << (((BYTE)RallyDataCountryIndex() + 1) / 2);
                            if ((pFlags[1] & bit & 0x1f) == 0) {
                                pFlags[1] |= bit & 0x1f;
                                g_playerEventAwardFlags[i] |= 0x10;
                                CGame::SetStartupFlag();
                            }
                        }
                        break;
                    }
                }
            }
            break;
        case 2:
        case 3:
        case 4:
            break;
        case 5:
            if ((BYTE)RallyData_GetSelectionBits12To13() == 2 && RallyTiming_GetChampionshipPointsByPosition(i) <= 2)
                RallyData_RecordArcadeMedal(i, (BYTE)RallyData_GetSelectionBits10To11(), RallyTiming_GetChampionshipPointsByPosition(i));
            if (RallyData_GetDriverRecordSelectionValue((BYTE)i) < 0xc && (BYTE)RallyData_GetSelectionBits12To13() == 2 &&
                RallyTiming_GetChampionshipPointsByPosition(i) == 0) {
                if ((BYTE)RallyData_GetSelectionBits10To11() == 0) {
                    switch (CGameInfo::GetConfiguredDifficulty()) {
                    case 2:
                        if ((pFlags[0] & 0xc0) == 0x40) {
                            pFlags[0] = ((pFlags[0] & 0xffffffc0) + 0x40 ^ pFlags[0]) & 0xc0 ^ pFlags[0]; // field++
                            g_playerEventAwardFlags[i] |= 0x200;
                            CGame::SetStartupFlag();
                        }
                        if (bAward == 0 && RallyData_SetCategoryAwardBit(i, 2)) {
                            g_unk0x00537f78[i] = 2;
                            g_playerEventAwardFlags[i] |= 0x80;
                        }
                        break;
                    case 1:
                        if ((pFlags[0] & 0x30) == 0x10) {
                            pFlags[0] = ((pFlags[0] & 0xfffffff0) + 0x10 ^ pFlags[0]) & 0x30 ^ pFlags[0]; // field++
                            g_playerEventAwardFlags[i] |= 0x100;
                            CGame::SetStartupFlag();
                        }
                        if (bAward == 0 && RallyData_SetCategoryAwardBit(i, 5)) {
                            g_unk0x00537f78[i] = 5;
                            g_playerEventAwardFlags[i] |= 0x80;
                        }
                        break;
                    }
                } else {
                    switch (CGameInfo::GetConfiguredDifficulty()) {
                    case 2:
                        if (bAward == 0 && RallyData_SetCategoryAwardBit(i, 3)) {
                            g_unk0x00537f78[i] = 3;
                            g_playerEventAwardFlags[i] |= 0x80;
                        }
                        if ((pFlags[0] & 0x200000) == 0) {
                            pFlags[0] |= 0x200000;
                            g_playerEventAwardFlags[i] |= 0x1000;
                        }
                        break;
                    case 1:
                        if ((pFlags[0] & 2) == 0) {
                            pFlags[0] = ((pFlags[0] & 0xfffffffe) - 1 ^ pFlags[0]) & 2 ^ pFlags[0]; // 1-bit field--
                            g_playerEventAwardFlags[i] |= 0x400;
                            CGame::SetStartupFlag();
                        }
                        if ((pFlags[0] & 0x100000) == 0) {
                            pFlags[0] |= 0x100000;
                            g_playerEventAwardFlags[i] |= 0x800;
                            CGame::SetStartupFlag();
                        }
                        if (bAward == 0 && RallyData_SetCategoryAwardBit(i, 6)) {
                            g_unk0x00537f78[i] = 6;
                            g_playerEventAwardFlags[i] |= 0x80;
                        }
                        break;
                    }
                }
            }
            break;
        case 6:
            if (RallyData_GetDriverRecordSelectionValue((BYTE)i) < 0xc && bAward == 0 &&
                (BYTE)RallyData_GetSelectionBits10To11() == 2 && StageTiming_GetFinishOrderSecondaryEntry(i) == 0) {
                if ((BYTE)RallyData_GetSelectionBits12To13() == 0) {
                    if (RallyData_HasCategoryAward(i, 0xc, 0) && RallyData_SetCategoryAwardBit(i, 0xd)) {
                        g_unk0x00537f78[i] = 0xd;
                        g_playerEventAwardFlags[i] |= 0x80;
                    }
                } else {
                    if (RallyData_HasCategoryAward(i, 0xc, 0) && RallyData_SetCategoryAwardBit(i, 0xe)) {
                        g_unk0x00537f78[i] = 0xe;
                        g_playerEventAwardFlags[i] |= 0x80;
                    }
                }
            }
            break;
        }
        if (flag) {
            if (CGameInfo::GetConfiguredDifficulty() == 1) {
                switch ((BYTE)RallyDataCountryIndex()) {
                case 0:
                    if (CGameInfo::GetUnlockFlagMask(0) == 0)
                        CGameInfo::SetUnlockFlag(0, 1);
                    g_unk0x00537f0c[i] = 1;
                    g_playerEventAwardFlags[i] |= 0x2000;
                    break;
                case 2:
                    if (CGameInfo::GetUnlockFlagMask(2) == 0)
                        CGameInfo::SetUnlockFlag(2, 1);
                    g_unk0x00537f0c[i] = 3;
                    g_playerEventAwardFlags[i] |= 0x2000;
                    break;
                case 4:
                    if (CGameInfo::GetUnlockFlagMask(4) == 0)
                        CGameInfo::SetUnlockFlag(4, 1);
                    g_unk0x00537f0c[i] = 5;
                    g_playerEventAwardFlags[i] |= 0x2000;
                    break;
                case 6:
                    if (CGameInfo::GetUnlockFlagMask(6) == 0)
                        CGameInfo::SetUnlockFlag(6, 1);
                    g_unk0x00537f0c[i] = 7;
                    g_playerEventAwardFlags[i] |= 0x2000;
                    break;
                }
            }
            if (CGameInfo::GetConfiguredDifficulty() == 2) {
                switch ((BYTE)RallyDataCountryIndex()) {
                case 0:
                    if (CGameInfo::GetUnlockFlagMask(1) == 0)
                        CGameInfo::SetUnlockFlag(1, 1);
                    g_unk0x00537f0c[i] = 2;
                    g_playerEventAwardFlags[i] |= 0x2000;
                    break;
                case 2:
                    if (CGameInfo::GetUnlockFlagMask(3) == 0)
                        CGameInfo::SetUnlockFlag(3, 1);
                    g_unk0x00537f0c[i] = 4;
                    g_playerEventAwardFlags[i] |= 0x2000;
                    break;
                case 4:
                    if (CGameInfo::GetUnlockFlagMask(5) == 0)
                        CGameInfo::SetUnlockFlag(5, 1);
                    g_unk0x00537f0c[i] = 6;
                    g_playerEventAwardFlags[i] |= 0x2000;
                    break;
                case 6:
                    if (CGameInfo::GetUnlockFlagMask(7) == 0)
                        CGameInfo::SetUnlockFlag(7, 1);
                    g_unk0x00537f0c[i] = 8;
                    g_playerEventAwardFlags[i] |= 0x2000;
                    break;
                }
            }
        }
        if (bAward == 0 && RallyData_HasOnlySilverMedals(i) && RallyData_SetCategoryAwardBit(i, 1)) {
            g_unk0x00537f78[i] = 1;
            g_playerEventAwardFlags[i] |= 0x80;
        }
    }
    for (i = 0; i < (BYTE)CGameInfo::GetConfiguredPlayerCount(); i++) {
        if (g_playerEventAwardFlags[i] & 0x2000) {
            CGame::SetStartupFlag();
            break;
        }
    }
}

