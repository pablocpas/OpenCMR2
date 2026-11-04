#include "port/sys.h"
#include <stddef.h>
#include <stdarg.h>
#include <windows.h>
#include <mmsystem.h>
#include "GameMenus.h"
#include "GameInfo.h"
#include "CategoryBestResults.h"
#include "Frontend.h"
#include "Font.h"
#include "RallyTiming.h"
#include "StageTiming.h"
#include <cstdio>
#include "Sprite.h"
#include "GenericFileLoader.h"
#include "Graphics.h"
#include "RallyData.h"
#include "NetPlayers.h"
#include "Input.h"
#include "TimingUtils.h"
#include "main.h"
#include "Game.h"
#include "StageUI.h"
#include "AIHelper.h"
#include "RegKey.h"

int NetPlayers_GetStandingCount(void);
int NetPlayers_GetPlayerRank(int index, int total);
int NetPlayers_GetStandingPlayerIndex(int index, int total);
unsigned int NetPlayers_GetStandingTime(int index, int total);
char *NetPlayers_GetStandingName(int index, int total);
unsigned int NetPlayers_GetStandingCar(int index, int total);
void GameMenus_DrawRowFrame(short row, short yOffset, char compact);
void GameMenus_DrawTextRow(int x, int y, char *pText, ...);
void FormatCentisecondsAsMinSecMSec(int iTime, char *pcFormattedTime);
extern BYTE g_menuRowFillColour[4];
extern char g_stageNumberFormat[];
extern char g_stageResultSameTime[];
extern BYTE g_unk0x00540898;
extern BYTE g_unk0x00541210;
extern int g_standingsExtraRow;
extern int g_unk0x005413f8;
extern int g_gameMenuScrollY;
extern int g_gameMenuRowYOffset;
extern char g_classRowHeaderFormat[];
BYTE RallyData_GetDriverSelectGridSlot(int param1);
int StageTiming_GetValidStartTime(int index);
void GameMenu_DrawInGameBreadcrumb(Menu *pMenu);

// GLOBAL: CMR2 0x0053e2d8
Menu g_menu0x0053e2d8;
// GLOBAL: CMR2 0x0053e4b8
Menu g_menu0x0053e4b8;
// GLOBAL: CMR2 0x0053e6a0
Menu g_menu0x0053e6a0;
// GLOBAL: CMR2 0x0053e888
Menu g_menu0x0053e888;
// GLOBAL: CMR2 0x0053ea68
Menu g_menu0x0053ea68;
// GLOBAL: CMR2 0x0053ec48
Menu g_menu0x0053ec48;
// GLOBAL: CMR2 0x0053ee28
Menu g_menu0x0053ee28;
// GLOBAL: CMR2 0x0053f008
Menu g_menu0x0053f008;
// GLOBAL: CMR2 0x0053f1e8
Menu g_menu0x0053f1e8;
// GLOBAL: CMR2 0x0053f3c8
Menu g_menu0x0053f3c8;
// GLOBAL: CMR2 0x0053f5b0
Menu g_menu0x0053f5b0;
// GLOBAL: CMR2 0x0053f790
Menu g_menu0x0053f790;
// GLOBAL: CMR2 0x0053f970
Menu g_menu0x0053f970;
// GLOBAL: CMR2 0x0053fb70
Menu g_menu0x0053fb70;
// GLOBAL: CMR2 0x0053fd58
Menu g_menu0x0053fd58;
// GLOBAL: CMR2 0x0053ff38
Menu g_menu0x0053ff38;
// GLOBAL: CMR2 0x00540118
Menu g_menu0x00540118;
// GLOBAL: CMR2 0x005402f8
Menu g_menu0x005402f8;
// GLOBAL: CMR2 0x005404d8
Menu g_menu0x005404d8;
// GLOBAL: CMR2 0x005406b8
Menu g_menu0x005406b8;
// GLOBAL: CMR2 0x005408a0
Menu g_menu0x005408a0;
// GLOBAL: CMR2 0x00540a80
Menu g_menu0x00540a80;
// GLOBAL: CMR2 0x00540c68
Menu g_menu0x00540c68;
// GLOBAL: CMR2 0x00540e50
Menu g_menu0x00540e50;
// GLOBAL: CMR2 0x00541030
Menu g_menu0x00541030;
// GLOBAL: CMR2 0x00541218
Menu g_menu0x00541218;
// GLOBAL: CMR2 0x00541400
Menu g_menu0x00541400;
// GLOBAL: CMR2 0x005416e0
Menu g_menu0x005416e0;
// GLOBAL: CMR2 0x005418d8
Menu g_menu0x005418d8;
// GLOBAL: CMR2 0x00541ae0
Menu g_menu0x00541ae0;
// GLOBAL: CMR2 0x0053fd50
DWORD g_menuBuildTime;

typedef void (*FadeCallback)(BYTE index);
short Car_GetOrderCount(void);
void NetRace_FadeOutPlayerScreen(BYTE index, FadeCallback pfnDone, int param3, int param4, int param5, char force);
BYTE *StageUI_GetRaceResultTable(void);
BOOL Network_GetSessionStateFlag(void);
int Network_CloseSession(void);
void Game_RequestOptionRefresh(void);
void Race_SetFlag3810C(void);
void Race_SetFlag37FFA(void);
void Race_SetFlag3810D(void);
void Race_ClearState38118(void);
void InRaceMenu_Close(void);
void StageUI_PublishNetworkRaceWins(void);
void NetRace_SendResultsQuit(void);
void NetRace_SendContinue(void);
void NetRace_SendClose(void);
void NetRace_SendRestart(void);
void NetPlayers_BuildFinalClassification(void);
void NetPlayers_ClearReadyFlags(void);
void CarDamage_ExportStageSnapshotToRallyRecord(int index);
void GameMenu_LeaveStageAndAdvanceChampionship(void);
struct ReplayStream;
int Replay_ResetBufferIfActive(ReplayStream *p);
void Replay_ResetActiveBufferState(void);
void Knockout_SetRoundActiveFlag(void);
unsigned int *RallyData_GetChampionshipState(void);
unsigned char RallyDataState(void);
unsigned char RallyData_GetSelectionFlag28(void);
extern ptrdiff_t g_unk0x00537f0c[6];
extern BYTE *g_playerReplayStreams[8];
extern BYTE g_menuFrameColour[4];
extern Menu *g_pHeaderMenu;
extern short g_menuRect[4];
extern char g_nameSpaceFormat[];

// Fade value of the stage end menus.
// GLOBAL: CMR2 0x00519ec8
int g_stageEndMenuFade = 0xacb49c;
// GLOBAL: CMR2 0x00541cd4
int g_unk0x00541cd4;

// Promotes every car of the race table one level (end of stage).
#define PROMOTE_CARS()                                                                  \
    for (i = 0; i < *StageUI_GetRaceResultTable(); i++)                                               \
        CGame::PromoteCallbackEntryByRule((CallbackStateMachine *)StageUI_GetRaceResultTable(), i, 1, 3)

void Race_SetFlag3810C(void);
void Race_SetFlag37FFA(void);

// Fade callback: promotes the cars and closes the network menu (restart).
// FUNCTION: CMR2 0x004014f0
void GameMenu_FinishNetworkRestartFade(BYTE index)
{
    BYTE i;

    PROMOTE_CARS();
    Race_SetFlag3810C();
    Race_SetFlag37FFA();
    InRaceMenu_Close();
}

// Fade callback: promotes the cars and closes the network menu.
// FUNCTION: CMR2 0x00401540
void GameMenu_FinishNetworkCloseFade(BYTE index)
{
    BYTE i;

    PROMOTE_CARS();
    Race_SetFlag3810C();
    InRaceMenu_Close();
}

// Fade colour of the in-race network menu transitions.
// GLOBAL: CMR2 0x00516090
int g_netMenuFadeColour = 0xacb49c;

void NetRace_SendRestart(void);
void StageUI_PublishNetworkRaceWins(void);
void NetRace_SendResultsQuit(void);

// Fade callback: promotes the cars and quits the (network) championship.
// FUNCTION: CMR2 0x004016b0
void GameMenu_FinishNetworkQuitChampionshipFade(BYTE index)
{
    BYTE i;

    PROMOTE_CARS();
    if (CGameInfo::GetConfiguredGameMode() == 4)
        *RallyData_GetChampionshipState() |= 0x800000;
    Race_SetFlag3810D();
    InRaceMenu_Close();
    if (CGameInfo::GetGameModeOptionBit19()) {
        CGame::DestroyLocalNetworkPlayer();
        Network_CloseSession();
        Game_RequestOptionRefresh();
    }
}

// Fade callback: promotes the cars and leaves the championship.
// FUNCTION: CMR2 0x00401720
void GameMenu_FinishLeaveChampionshipFade(BYTE index)
{
    BYTE i;

    PROMOTE_CARS();
    if (CGameInfo::GetConfiguredGameMode() == 4)
        *RallyData_GetChampionshipState() |= 0x800000;
    Race_SetFlag3810D();
    InRaceMenu_Close();
}

// GLOBAL: CMR2 0x0052af54
int g_unk0x0052af54;

void NetPlayers_ClearReadyFlags(void);
void NetRace_SendClose(void);

// Item callback of the in-race "restart stage" item.
// FUNCTION: CMR2 0x00401590
void GameMenu_RequestStageRestart(Menu *pMenu, int param)
{
    BYTE i;

    for (i = 0; (short)i < Car_GetOrderCount(); i++) {
        if (CGameInfo::GetConfiguredGameMode() == 8 || CGameInfo::GetConfiguredGameMode() == 1 ||
            CGameInfo::GetConfiguredGameMode() == 0)
            NetRace_FadeOutPlayerScreen(i, i != 0 ? NULL : (FadeCallback)GameMenu_FinishNetworkCloseFade, 1, 0, g_netMenuFadeColour, 1);
        else
            NetRace_FadeOutPlayerScreen(i, i != 0 ? NULL : (FadeCallback)GameMenu_FinishNetworkCloseFade, 1, 0, g_unk0x0052af54, 1);
    }
    if (CGameInfo::GetGameModeOptionBit19() && Network_GetSessionStateFlag() && CGameInfo::GetConfiguredGameMode() != 10) {
        NetPlayers_ClearReadyFlags();
        NetRace_SendClose();
    }
}

// Item callback of the network "restart" item: fades every car out.
// FUNCTION: CMR2 0x00401630
void GameMenu_RequestNetworkRestart(Menu *pMenu, int param)
{
    BYTE i;

    for (i = 0; (short)i < Car_GetOrderCount(); i++)
        NetRace_FadeOutPlayerScreen(i, i != 0 ? NULL : (FadeCallback)GameMenu_FinishNetworkRestartFade, 1, 0, g_netMenuFadeColour, 1);
    if (CGameInfo::GetGameModeOptionBit19() && Network_GetSessionStateFlag() && CGameInfo::GetConfiguredGameMode() != 10)
        NetRace_SendRestart();
}

// Item callback of the network "quit" item.
// FUNCTION: CMR2 0x00401780
void GameMenu_RequestNetworkQuit(Menu *pMenu, int param)
{
    BYTE i;

    for (i = 0; (short)i < Car_GetOrderCount(); i++)
        NetRace_FadeOutPlayerScreen(i, i != 0 ? NULL : (FadeCallback)GameMenu_FinishNetworkQuitChampionshipFade, 1, 0, g_netMenuFadeColour, 1);
}

// Item callback of the network "leave" item.
// FUNCTION: CMR2 0x004017e0
void GameMenu_RequestNetworkLeave(Menu *pMenu, int param)
{
    BYTE i;

    for (i = 0; (short)i < Car_GetOrderCount(); i++)
        NetRace_FadeOutPlayerScreen(i, i != 0 ? NULL : (FadeCallback)GameMenu_FinishLeaveChampionshipFade, 1, 0, g_netMenuFadeColour, 1);
    if (CGameInfo::GetGameModeOptionBit19() && Network_GetSessionStateFlag()) {
        StageUI_PublishNetworkRaceWins();
        NetRace_SendResultsQuit();
    }
}

// Fade callback of the "quit" item: ends the championship and leaves.
// FUNCTION: CMR2 0x00449020
void GameMenu_FinishQuitChampionshipFade(BYTE index)
{
    BYTE i;

    PROMOTE_CARS();
    if (CGameInfo::GetConfiguredGameMode() == 4)
        *RallyData_GetChampionshipState() |= 0x800000;
    if (CGameInfo::GetGameModeOptionBit19()) {
        CGame::DestroyLocalNetworkPlayer();
        Network_CloseSession();
        Game_RequestOptionRefresh();
    }
    Race_SetFlag3810D();
}

// Fade callback of the network "quit" item.
// FUNCTION: CMR2 0x00449090
void GameMenu_FinishNetworkResultsQuitFade(BYTE index)
{
    BYTE i;

    if (CGameInfo::IsInRaceMenuOpen())
        InRaceMenu_Close();
    PROMOTE_CARS();
    if (Network_GetSessionStateFlag()) {
        StageUI_PublishNetworkRaceWins();
        NetRace_SendResultsQuit();
    }
    NetPlayers_BuildFinalClassification();
    Race_SetFlag3810D();
}

// Fade callback of the "restart" item.
// FUNCTION: CMR2 0x00449ea0
void GameMenu_FinishRestartFade(BYTE index)
{
    BYTE i;

    PROMOTE_CARS();
    Race_SetFlag3810C();
}

// Fade callback of the "retry" item.
// FUNCTION: CMR2 0x00449ee0
void GameMenu_FinishRetryFade(BYTE index)
{
    BYTE i;

    PROMOTE_CARS();
    Race_SetFlag3810C();
    Race_SetFlag37FFA();
}

// Fade callback of the "continue" item.
// FUNCTION: CMR2 0x00449fe0
void GameMenu_FinishContinueFade(BYTE index)
{
    GameMenu_LeaveStageAndAdvanceChampionship();
    Race_ClearState38118();
    CGameInfo::SetInputAndGamePaused(0);
}

// Menu action "quit": fades every player out, the first one quits.
// FUNCTION: CMR2 0x00449b00
void GameMenu_RequestResultsQuit(Menu *pMenu, int param)
{
    BYTE i;

    for (i = 0; (short)i < Car_GetOrderCount(); i++)
        NetRace_FadeOutPlayerScreen(i, i != 0 ? NULL : GameMenu_FinishQuitChampionshipFade, 1, 0, g_stageEndMenuFade, 1);
}

// Menu action "quit" of the network results menu.
// FUNCTION: CMR2 0x00449b60
void GameMenu_RequestNetworkResultsQuit(Menu *pMenu, int param)
{
    NetRace_FadeOutPlayerScreen(0, GameMenu_FinishNetworkResultsQuitFade, 1, 0, g_stageEndMenuFade, 1);
}

// FUNCTION: CMR2 0x00449b80
void GameMenu_ContinueWithDefaultMode(Menu *pMenu, int param)
{
    GameMenu_RequestResultsContinue(pMenu, 0);
}

// FUNCTION: CMR2 0x00449b90
void GameMenu_SelectLastItem(Menu *pMenu, int param)
{
    pMenu->cursor = pMenu->itemCount - 1;
}

// Resizes the first item of one of the standings menus: it scans the drivers
// of the current table (overall, split or stage) and keeps the worst position,
// which becomes the scroll window (five rows below it, at most ten).
// FUNCTION: CMR2 0x00449ba0
void GameMenu_UpdateStandingsScrollRange(Menu *pMenu, int param)
{
    int i;
    int slot;
    int worstTime;
    int worstPos;

    worstPos = 0x10;
    worstTime = 0;
    if (pMenu == &g_menu0x0053f008 || pMenu == &g_menu0x0053e6a0) {
        i = 0;
        if (CGameInfo::GetConfiguredPlayerCount() > 0) {
            slot = 0xf;
            do {
                if (RallyTiming_GetOverallPositionOfDriver(slot) < worstPos)
                    worstPos = RallyTiming_GetOverallPositionOfDriver(slot);
                i++;
                slot--;
            } while (i < CGameInfo::GetConfiguredPlayerCount());
        }
    }
    if (pMenu == &g_menu0x0053e4b8) {
        i = 0;
        if (CGameInfo::GetConfiguredPlayerCount() > 0) {
            do {
                if (StageTiming_GetCurrentSplitPositionOfDriver(i) < worstPos)
                    worstPos = StageTiming_GetCurrentSplitPositionOfDriver(i);
                i++;
            } while (i < CGameInfo::GetConfiguredPlayerCount());
        }
    }
    if (pMenu == &g_menu0x00541ae0) {
        i = 0;
        if (CGameInfo::GetConfiguredPlayerCount() > 0) {
            do {
                if (RallyTiming_GetStageTimeSeconds(StageTiming_GetDriverSlot(i)) > worstTime) {
                    worstTime = RallyTiming_GetStageTimeSeconds(StageTiming_GetDriverSlot(i));
                    worstPos = RallyTiming_GetOverallPositionOfDriver(StageTiming_GetDriverSlot(i));
                }
                i++;
            } while (i < CGameInfo::GetConfiguredPlayerCount());
        }
    }
    if (worstPos > 5)
        pMenu->items[0].max = worstPos - 5;
    if (pMenu->items[0].max > 10)
        pMenu->items[0].max = 10;
}

// Sets the range of the first item to the remaining races of the championship
// (zero once it is over, one as the minimum otherwise).
// FUNCTION: CMR2 0x00449ca0
void GameMenu_UpdateRemainingRaceRange(Menu *pMenu, int param)
{
    pMenu->items[0].max = 0;
    if (NetPlayers_GetStandingCount() > 6) {
        pMenu->items[0].min = NetPlayers_GetStandingCount() - 5;
    } else {
        pMenu->items[0].min = 1;
    }
}

// FUNCTION: CMR2 0x00449e90
void GameMenu_OpenFirstItemSubmenu(Menu *pMenu, int param)
{
    Menu_SetNextAction(pMenu->items[0].pSubMenu);
}

// Menu action "restart".
// FUNCTION: CMR2 0x00449f30
void GameMenu_RequestResultsRestart(Menu *pMenu, int param)
{
    BYTE i;

    for (i = 0; (short)i < Car_GetOrderCount(); i++) {
        if (CGameInfo::GetConfiguredGameMode() == 8 || CGameInfo::GetConfiguredGameMode() == 1 || CGameInfo::GetConfiguredGameMode() == 0)
            NetRace_FadeOutPlayerScreen(i, i != 0 ? NULL : GameMenu_FinishRestartFade, 1, 0, g_stageEndMenuFade, 1);
        else
            NetRace_FadeOutPlayerScreen(i, i != 0 ? NULL : GameMenu_FinishRestartFade, 1, 0, g_unk0x00541cd4, 1);
    }
    if (CGameInfo::GetGameModeOptionBit19() && Network_GetSessionStateFlag() && CGameInfo::GetConfiguredGameMode() != 10 &&
        CGameInfo::GetConfiguredGameMode() != 12) {
        NetPlayers_ClearReadyFlags();
        NetRace_SendClose();
    }
}

// Menu action "continue".
// FUNCTION: CMR2 0x0044a000
void GameMenu_RequestResultsContinue(Menu *pMenu, int param1)
{
    BYTE i;

    if (CGameInfo::GetGameModeOptionBit19()) {
        NetRace_FadeOutPlayerScreen(0, GameMenu_FinishContinueFade, 1, 0, g_stageEndMenuFade, 1);
        if (Network_GetSessionStateFlag())
            NetRace_SendContinue();
        return;
    }
    for (i = 0; (short)i < Car_GetOrderCount(); i++)
        NetRace_FadeOutPlayerScreen(i, i != 0 ? NULL : GameMenu_FinishContinueFade, 1, 0, g_stageEndMenuFade, 1);
}

// Leaves the stage: stores every car's timing, updates the race table and
// starts the next step of the championship.
// FUNCTION: CMR2 0x0041f2b0
void GameMenu_LeaveStageAndAdvanceChampionship(void)
{
    int i;

    for (i = 0; i < (BYTE)RallyDataState(); i++)
        CarDamage_ExportStageSnapshotToRallyRecord(i);
    for (i = 0; i < *(BYTE *)g_unk0x00537f0c[5]; i++) {
        CGame::PromoteCallbackEntryByRule((CallbackStateMachine *)g_unk0x00537f0c[5], i, 0, 2);
        Replay_ResetBufferIfActive((ReplayStream *)g_playerReplayStreams[i]);
    }
    if ((BYTE)RallyData_GetSelectionFlag28() && (BYTE)CGameInfo::GetSoundOptionBit30())
        Replay_ResetActiveBufferState();
    if (CGameInfo::GetConfiguredGameMode() != 4)
        return;
    if (*RallyData_GetChampionshipState() & 0x800000) {
        Race_SetFlag3810D();
        return;
    }
    Knockout_SetRoundActiveFlag();
    Race_SetFlag3810C();
}

// FUNCTION: CMR2 0x0044a090
void GameMenu_LeaveStageAndResetMenuFlag(Menu *pMenu, int param)
{
    GameMenu_LeaveStageAndAdvanceChampionship();
    CGameInfo::SetInputAndGamePaused(0);
}

// Menu action "retry".
// FUNCTION: CMR2 0x0044a0a0
void GameMenu_RequestResultsRetry(Menu *pMenu, int param)
{
    BYTE i;

    for (i = 0; (short)i < Car_GetOrderCount(); i++)
        NetRace_FadeOutPlayerScreen(i, i != 0 ? NULL : GameMenu_FinishRetryFade, 1, 0, g_stageEndMenuFade, 1);
    if (CGameInfo::GetGameModeOptionBit19() && Network_GetSessionStateFlag() && CGameInfo::GetConfiguredGameMode() != 10)
        NetRace_SendRestart();
}

int Race_IsMultiplayerRecordMode10(void);
BYTE View_GetActiveCameraFlags(BYTE index);
char *Knockout_GetDriverNameForSide(KnockoutMatch *pMatch, int side);
int Knockout_IsHumanMatchSide(KnockoutMatch *pMatch, int side);
int Knockout_SelectDisplaySide(KnockoutMatch *pMatch, int param2);
char *Knockout_GetCarNameForSide(KnockoutMatch *pMatch, int side);
void RallyData_CopyDriverDisplayName(int id);
unsigned char RallyData_GetSelectionFlag26(void);
unsigned int RallyData_GetSelectionFlag27(void);
extern KnockoutMatch *g_pKnockoutMatch;

// Draw callback of the in-race pause header: "PAUSED" followed by a separator
// bar and the name of the driver (or car) the pause menu belongs to.
// FUNCTION: CMR2 0x0044b7b0
void GameMenu_DrawPauseHeader(Menu *pMenu)
{
    int x;
    int width;
    char result;

    Font_DrawText(2, CFrontend::GetTextString(0x47), (int)(g_pGraphics->resX * 30) / 640,
                  (int)(g_pGraphics->resY * 0x43) / 480, (int *)g_menuFrameColour, 0x11);
    width = g_pGraphics->resX;
    x = (width * 30) / 640 + Font_GetTextWidth(2, (BYTE *)CFrontend::GetTextString(0x47));
    x += (width * 8) / 640;
    g_menuRect[0] = (short)x;
    g_menuRect[1] = (short)((int)(g_pGraphics->resY * 0x25) / 480);
    g_menuRect[2] = 2;
    g_menuRect[3] = (short)((int)(g_pGraphics->resY * 0x29) / 480);
    x += 2 + (int)(g_pGraphics->resX * 8) / 640;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_menuRect, g_menuFrameColour, 2);
    if (Race_IsMultiplayerRecordMode10()) {
        if (CGameInfo::GetConfiguredGameMode() == 4)
            Font_DrawText(2, Knockout_GetCarNameForSide(g_pKnockoutMatch, View_GetActiveCameraFlags(1)), x,
                          (int)(g_pGraphics->resY * 0x43) / 480, (int *)g_menuFrameColour, 0x11);
        else
            Font_DrawText(2, (char *)RallyData_GetRecord(View_GetActiveCameraFlags(1)), x,
                          (int)(g_pGraphics->resY * 0x43) / 480, (int *)g_menuFrameColour, 0x11);
    } else if ((BYTE)RallyData_GetSelectionFlag26() && (BYTE)RallyDataState() == 1) {
        if (RallyData_GetUsableRecordCategory(View_GetActiveCameraFlags(0)) == -1)
            Font_DrawText(2, (char *)RallyData_GetRecord(View_GetActiveCameraFlags(0)), x,
                          (int)(g_pGraphics->resY * 0x43) / 480, (int *)g_menuFrameColour, 0x11);
        else
            Font_DrawText(2, CAIHelper::GetNameForID(View_GetActiveCameraFlags(0)), x,
                          (int)(g_pGraphics->resY * 0x43) / 480, (int *)g_menuFrameColour, 0x11);
    } else if (CGameInfo::GetConfiguredGameMode() == 4) {
        if (Knockout_IsHumanMatchSide(g_pKnockoutMatch, Knockout_SelectDisplaySide(g_pKnockoutMatch, View_GetActiveCameraFlags(0))))
            Font_DrawText(2, Knockout_GetCarNameForSide(g_pKnockoutMatch, Knockout_SelectDisplaySide(g_pKnockoutMatch, View_GetActiveCameraFlags(0))), x,
                          (int)(g_pGraphics->resY * 0x43) / 480, (int *)g_menuFrameColour, 0x11);
        else
            Font_DrawText(2, Knockout_GetDriverNameForSide(g_pKnockoutMatch, Knockout_SelectDisplaySide(g_pKnockoutMatch, View_GetActiveCameraFlags(0))), x,
                          (int)(g_pGraphics->resY * 0x43) / 480, (int *)g_menuFrameColour, 0x11);
    } else if ((BYTE)RallyData_GetSelectionFlag27() && !CGameInfo::GetGameModeOptionBit19()) {
        RallyData_CopyDriverDisplayName(StageTiming_GetSplitTableEntry(StageUI_GetRaceEndEventCount(), View_GetActiveCameraFlags(0)));
        Font_DrawText(2, CFrontend::m_stringDest, x, (int)(g_pGraphics->resY * 0x43) / 480, (int *)g_menuFrameColour, 0x11);
        Font_SetBlendMode(2);
        return;
    } else {
        result = RallyData_GetUsableRecordCategory(View_GetActiveCameraFlags(0) + StageUI_GetRaceEndEventCount());
        if (result == -1)
            Font_DrawText(2, (char *)RallyData_GetRecord(View_GetActiveCameraFlags(0) + StageUI_GetRaceEndEventCount()), x,
                          (int)(g_pGraphics->resY * 0x43) / 480, (int *)g_menuFrameColour, 0x11);
        else
            Font_DrawText(2, CAIHelper::GetNameForID(RallyData_GetUsableRecordCategory(View_GetActiveCameraFlags(0) + StageUI_GetRaceEndEventCount())), x,
                          (int)(g_pGraphics->resY * 0x43) / 480, (int *)g_menuFrameColour, 0x11);
    }
    Font_SetBlendMode(2);
}

// Draw callback of the "waiting for the other players" screen.
// FUNCTION: CMR2 0x0044bc30
void GameMenu_DrawWaitingForPlayers(Menu *pMenu)
{
    Font_DrawText(2, CFrontend::GetTextString(0x47), (int)(g_pGraphics->resX * 30) / 640,
                  (int)(g_pGraphics->resY * 0x43) / 480, (int *)g_menuFrameColour, 0x11);
    Font_DrawText(2, CFrontend::GetTextString(0xfa), (int)g_pGraphics->resX / 2, (int)g_pGraphics->resY / 2,
                  (int *)g_menuFrameColour, 0x12);
    Font_SetBlendMode(2);
}

unsigned int StageTiming_GetDriverSplitClock(int index, int split);
int GetStageSplitCount(void);
extern char g_minSecMSECFormatString[];

// Split times of the two drivers of the current arcade knockout match, side
// by side: name, split times, total.
// FUNCTION: CMR2 0x0044cdb0
void GameMenu_DrawKnockoutSplitComparison(void)
{
    // This caller uses full DWORD coordinate slots. DrawText consumes their
    // low signed WORDs.
    // PORT: a direct call (the original called through a cast function
    // pointer); converting to short keeps the low words.
    char names[2][20];
    int car;
    int x;
    int split;
    int time;

    if (RallyData_GetUsableRecordCategory(g_pKnockoutMatch->flags & 0x1f) == -1 &&
        RallyData_GetUsableRecordCategory((g_pKnockoutMatch->flags >> 5) & 0x1f) == -1) {
        sprintf(names[0], "%s", (char *)RallyData_GetRecord(StageUI_GetRaceEndEventCount() + (g_pKnockoutMatch->flags & 0x1f)));
        sprintf(names[1], "%s", (char *)RallyData_GetRecord(StageUI_GetRaceEndEventCount() + ((g_pKnockoutMatch->flags >> 5) & 0x1f)));
    } else if (RallyData_GetUsableRecordCategory(g_pKnockoutMatch->flags & 0x1f) == -1) {
        sprintf(names[0], "%s", (char *)RallyData_GetRecord(StageUI_GetRaceEndEventCount() + (g_pKnockoutMatch->flags & 0x1f)));
        sprintf(names[1], "%s", CAIHelper::GetNameForID(RallyData_GetUsableRecordCategory((g_pKnockoutMatch->flags >> 5) & 0x1f)));
    } else {
        sprintf(names[0], "%s", (char *)RallyData_GetRecord(StageUI_GetRaceEndEventCount() + ((g_pKnockoutMatch->flags >> 5) & 0x1f)));
        sprintf(names[1], "%s", CAIHelper::GetNameForID(RallyData_GetUsableRecordCategory(g_pKnockoutMatch->flags & 0x1f)));
    }
    for (car = 0; car < 2; car++) {
        x = (int)(g_pGraphics->resX * 30) / 640 + ((int)(g_pGraphics->resX * 0x166) / 640) * car;
        Font_DrawText(1, CFrontend::GetTextString(0x89), x, (int)(g_pGraphics->resY * 0xe6) / 480,
                      (int *)g_menuFrameColour, 0x11);
        sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x9b), names[car]);
        Font_DrawText(1, CFrontend::m_stringDest, x,
                      (int)(g_pGraphics->resY * 0xe6) / 480 - (int)(g_pGraphics->resY * 0x1e) / 480,
                      (int *)g_menuFrameColour, 0x11);
        Font_DrawText(1, CFrontend::GetTextString(0x8a), x,
                      (int)(g_pGraphics->resY * 0x19) / 480 + (int)(g_pGraphics->resY * 0xe6) / 480 +
                          (int)(GetStageSplitCount() * g_pGraphics->resY * 0x1e) / 480,
                      (int *)g_menuFrameColour, 0x11);
        x = (int)(g_pGraphics->resX * 0xb1) / 640 + ((int)(g_pGraphics->resX * 0x166) / 640) * car;
        for (split = 0; split < GetStageSplitCount(); split++) {
            FormatCentisecondsAsMinSecMSec(StageTiming_GetDriverSplitClock(car, split + 1) - StageTiming_GetDriverSplitClock(car, split),
                                           CFrontend::m_stringDest);
            Font_DrawText(1, CFrontend::m_stringDest, x,
                          (int)(g_pGraphics->resY * 0xe6) / 480 + ((int)(g_pGraphics->resY * 0x1e) / 480) * split,
                          (int *)g_menuFrameColour, 0x11);
        }
        time = StageTiming_GetValidStartTime(car);
        sprintf(CFrontend::m_stringDest, g_minSecMSECFormatString, StageTiming_GetValidStartTime(car) / 6000,
                (StageTiming_GetValidStartTime(car) / 100) % 60, time % 100);
        Font_DrawText(1, CFrontend::m_stringDest, x,
                      (int)(g_pGraphics->resY * 0x19) / 480 + (int)(g_pGraphics->resY * 0xe6) / 480 +
                          ((int)(g_pGraphics->resY * 0x1e) / 480) * split,
                      (int *)g_menuFrameColour, 0x11);
    }
}

Texture *InRaceMenu_GetCurtainTexture(void);
int NetRace_GetLastSentSplit(void);
int NetRace_GetLastSentStageIndex(void);
int StageTiming_GetCarSplitTime(int car, int index);
unsigned int RallyData_GetSelectionBits12To13(void);
unsigned int RallyData_GetSelectionBits16To19(void);
extern BYTE g_menuTextColour[4];
extern char g_noTimeText[];

// GLOBAL: CMR2 0x00519f48
char g_recordNameFormat[] = "%s, ";
// GLOBAL: CMR2 0x00519f50
char g_recordHeaderFormat[] = "%s,  ";

// Name of the ghost car (arcade record holder) shown in the time trial table.
// GLOBAL: CMR2 0x00519ee0
char g_ghostName[4] = "cps";
// Split times of the ghost car.
// GLOBAL: CMR2 0x00541ab8
int g_ghostSplits[10];

// Draw callback of the split times screen: the country flag in the corner,
// then per car the name, the time of every split and the total; in arcade
// mode the record holder and record time head the list, in the knockout mode
// GameMenu_DrawKnockoutSplitComparison draws the two drivers of the match.
// match 69%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0044bcd0
void GameMenu_DrawSplitTimes(Menu *pMenu)
{
    short rect[4];
    Texture *pFlag;
    GameInfo0xa4 *pInfo;
    BYTE *pColour;
    BOOL isGhost;
    char *pName;
    int yAdjust;
    int header;
    int count;
    int car;
    int x;
    int width;
    int yOffset;
    int split;
    int total;
    int base;

    yAdjust = 0;
    header = 0;
    GameMenu_ClearMenuListWithHighlight();
    if (InRaceMenu_GetCurtainTexture()) {
        rect[2] = ((Texture *)InRaceMenu_GetCurtainTexture())->width;
        rect[3] = ((Texture *)InRaceMenu_GetCurtainTexture())->height;
        rect[0] = (short)((int)(g_pGraphics->resX * 0x234) / 640 - rect[2] / 2);
        rect[1] = (short)((int)(g_pGraphics->resY * 0x30) / 480 - rect[3] / 2);
        pFlag = InRaceMenu_GetCurtainTexture();
        Sprite_Queue((SpriteRect *)&InRaceMenu_GetCurtainTexture()->field_0x11c, (SpriteRect *)rect, pFlag, 2, 0, NULL, NULL,
                     g_menuFrameColour, 8);
    }
    if (CGameInfo::GetConfiguredGameMode() == 4) {
        GameMenus_DrawTextRow((int)(g_pGraphics->resX * 30) / 640, (int)(g_pGraphics->resY * 0x43) / 480,
                              CFrontend::GetTextString(0x48), (char *)NULL);
        GameMenu_DrawKnockoutSplitComparison();
        Font_SetBlendMode(2);
        return;
    }
    if (CGameInfo::GetConfiguredGameMode() == 5 || CGameInfo::GetConfiguredGameMode() == 6 || CGameInfo::GetConfiguredGameMode() == 7 ||
        CGameInfo::GetConfiguredGameMode() == 11) {
        GameMenu_DrawInGameBreadcrumb(pMenu);
        x = (int)(g_pGraphics->resX * 30) / 640;
        sprintf(CFrontend::m_stringDest, g_recordHeaderFormat, CFrontend::GetTextString(0x6d));
        CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
        Font_DrawText(0, CFrontend::m_stringDest, x,
                      (int)(g_pGraphics->resY * 0x91) / 480 - (int)(g_pGraphics->resY * 0x32) / 480,
                      (int *)g_menuFrameColour, 0x11);
        x += Font_GetTextWidth(0, (BYTE *)CFrontend::m_stringDest);
        CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
        pInfo = CGameInfo::GetGameInfoFieldA4Address();
        sprintf(CFrontend::m_stringDest, g_recordNameFormat,
                pInfo->arcadeRecordTimes[(RallyData_GetSelectionBits10To11() & 0xff) * 3 + (RallyData_GetSelectionBits12To13() & 0xff)].ident);
        Font_DrawText(0, CFrontend::m_stringDest, x,
                      (int)(g_pGraphics->resY * 0x91) / 480 - (int)(g_pGraphics->resY * 0x32) / 480,
                      (int *)g_menuFrameColour, 0x11);
        width = Font_GetTextWidth(0, (BYTE *)CFrontend::m_stringDest);
        CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
        pInfo = CGameInfo::GetGameInfoFieldA4Address();
        FormatCentisecondsAsMinSecMSec(
            (pInfo->arcadeRecordTimes[(RallyData_GetSelectionBits10To11() & 0xff) * 3 + (RallyData_GetSelectionBits12To13() & 0xff)].value >>
             7) & 0xffff,
            CFrontend::m_stringDest);
        Font_DrawText(0, CFrontend::m_stringDest, x + width,
                      (int)(g_pGraphics->resY * 0x91) / 480 - (int)(g_pGraphics->resY * 0x32) / 480,
                      (int *)g_menuFrameColour, 0x11);
        yOffset = (10 - (RallyData_GetSelectionBits16To19() & 0xff)) * ((int)(g_pGraphics->resY * 10) / 480) -
                  (int)(g_pGraphics->resY * 0x12) / 480;
        if ((BYTE)RallyData_GetSelectionBits16To19() == 10)
            yOffset += (int)(g_pGraphics->resY * -10) / 480;
        for (car = 0; car < (int)(RallyDataState() & 0xff); car++) {
            x = (int)(g_pGraphics->resX * 30) / 640 + ((int)(g_pGraphics->resX * 0x166) / 640) * car;
            if ((BYTE)RallyData_GetSelectionBits16To19() != 10)
                Font_DrawText(1, CFrontend::GetTextString(0xac), x, (int)(g_pGraphics->resY * 0x91) / 480 + yOffset,
                              (int *)g_menuFrameColour, 0x11);
            sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x9b),
                    (char *)RallyData_GetRecord(StageUI_GetRaceEndEventCount() + car));
            Font_DrawText(1, CFrontend::m_stringDest, x,
                          (int)(g_pGraphics->resY * 0x1e) / 480 + yOffset + (int)(g_pGraphics->resY * 0x91) / 480,
                          (int *)g_menuFrameColour, 0x11);
            Font_DrawText(1, CFrontend::GetTextString(0x8a), x,
                          (int)(g_pGraphics->resY * 0x91) / 480 + yOffset +
                              ((RallyData_GetSelectionBits16To19() & 0xff) + 1) * ((int)(g_pGraphics->resY * 0x1e) / 480),
                          (int *)g_menuFrameColour, 0x11);
            total = 0;
            x = (int)(g_pGraphics->resX * 0xb1) / 640 + ((int)(g_pGraphics->resX * 0x166) / 640) * car;
            if (!(!CGameInfo::GetGameModeOptionBit19())) {
                for (split = 1; split <= NetRace_GetLastSentStageIndex(); split++) {
                    FormatCentisecondsAsMinSecMSec(StageTiming_GetCarSplitTime(0, split), CFrontend::m_stringDest);
                    Font_DrawText(1, CFrontend::m_stringDest, x,
                                  (int)(g_pGraphics->resY * 0x91) / 480 + yOffset +
                                      ((int)(g_pGraphics->resY * 0x1e) / 480) * split,
                                  (int *)g_menuFrameColour, 0x11);
                }
                for (split = NetRace_GetLastSentStageIndex() + 1; split < (int)(RallyData_GetSelectionBits16To19() & 0xff) + 1; split++) {
                    if (split > 0) {
                        sprintf(CFrontend::m_stringDest, "%s", g_noTimeText);
                        Font_DrawText(1, CFrontend::m_stringDest, x,
                                      (int)(g_pGraphics->resY * 0x91) / 480 + yOffset +
                                          ((int)(g_pGraphics->resY * 0x1e) / 480) * split,
                                      (int *)g_menuFrameColour, 0x11);
                    }
                }
                total = StageTiming_GetValidStartTime(0);
            } else {
                for (split = 1; split < (int)(RallyData_GetSelectionBits16To19() & 0xff) + 1; split++) {
                    FormatCentisecondsAsMinSecMSec(StageTiming_GetCarSplitTime(car, split), CFrontend::m_stringDest);
                    Font_DrawText(1, CFrontend::m_stringDest, x,
                                  (int)(g_pGraphics->resY * 0x91) / 480 + yOffset +
                                      ((int)(g_pGraphics->resY * 0x1e) / 480) * split,
                                  (int *)g_menuFrameColour, 0x11);
                    total += StageTiming_GetCarSplitTime(car, split);
                }
            }
            FormatCentisecondsAsMinSecMSec(total, CFrontend::m_stringDest);
            Font_DrawText(1, CFrontend::m_stringDest, x,
                          (int)(g_pGraphics->resY * 0x91) / 480 + yOffset +
                              ((int)(g_pGraphics->resY * 0x1e) / 480) * split,
                          (int *)g_menuFrameColour, 0x11);
        }
    } else {
        GameMenus_DrawTextRow((int)(g_pGraphics->resX * 30) / 640, (int)(g_pGraphics->resY * 0x43) / 480,
                              CFrontend::GetTextString(0x48), (char *)NULL);
        count = 2;
        if (CGameInfo::GetGameModeOptionBit19())
            count = 1;
        for (car = 0; car < count; car++) {
            if ((BYTE)RallyDataState() == 1 && car == 1) {
                pColour = g_menuTextColour;
                isGhost = TRUE;
            } else {
                pColour = g_menuFrameColour;
                isGhost = FALSE;
            }
            x = (int)(g_pGraphics->resX * 30) / 640 + ((int)(g_pGraphics->resX * 0x166) / 640) * car;
            if (GetStageSplitCount() == 2) {
                yAdjust = (int)(g_pGraphics->resY * -0x1e) / 480;
                header = (int)(g_pGraphics->resY * 0xe6) / 480 - (int)(g_pGraphics->resY * 0x91) / 480;
            }
            if (isGhost) {
                sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(0x6d));
                CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
                base = CGameInfo::GetConfiguredGameMode() == 4 ? g_pGraphics->resY * 0xe6 : g_pGraphics->resY * 0x91;
                Font_DrawText(1, CFrontend::m_stringDest, x, base / 480 + header + yAdjust, (int *)pColour, 0x11);
                pName = g_ghostName;
            } else {
                base = CGameInfo::GetConfiguredGameMode() == 4 ? g_pGraphics->resY * 0xe6 : g_pGraphics->resY * 0x91;
                Font_DrawText(1, CFrontend::GetTextString(0x89), x, base / 480 + header + yAdjust, (int *)pColour, 0x11);
                pName = (char *)RallyData_GetRecord(StageUI_GetRaceEndEventCount() + car);
            }
            sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x9b), pName);
            base = CGameInfo::GetConfiguredGameMode() == 4 ? g_pGraphics->resY * 0xe6 : g_pGraphics->resY * 0x91;
            Font_DrawText(1, CFrontend::m_stringDest, x,
                          (int)(g_pGraphics->resY * 0x1e) / 480 + base / 480 + header + yAdjust, (int *)pColour, 0x11);
            base = g_pGraphics->resY * 0x91;
            if (CGameInfo::GetConfiguredGameMode() == 4)
                base = g_pGraphics->resY * 0xe6;
            Font_DrawText(1, CFrontend::GetTextString(0x8a), x,
                          (int)(g_pGraphics->resY * 0x19) / 480 + base / 480 +
                              (int)(GetStageSplitCount() * g_pGraphics->resY * 0x1e) / 480 + header,
                          (int *)pColour, 0x11);
            x = (int)(g_pGraphics->resX * 0xb1) / 640 + ((int)(g_pGraphics->resX * 0x166) / 640) * car;
            if (!CGameInfo::GetGameModeOptionBit19() || isGhost) {
                for (split = 0; split < GetStageSplitCount(); split++) {
                    if (isGhost)
                        FormatCentisecondsAsMinSecMSec(g_ghostSplits[split + 1] - g_ghostSplits[split], CFrontend::m_stringDest);
                    else
                        FormatCentisecondsAsMinSecMSec(StageTiming_GetDriverSplitClock(car, split + 1) - StageTiming_GetDriverSplitClock(car, split), CFrontend::m_stringDest);
                    Font_DrawText(1, CFrontend::m_stringDest, x,
                                  (int)(g_pGraphics->resY * 0x91) / 480 + header +
                                      ((int)(g_pGraphics->resY * 0x1e) / 480) * split,
                                  (int *)pColour, 0x11);
                }
                if (isGhost) {
                    sprintf(CFrontend::m_stringDest, g_minSecMSECFormatString,
                            g_ghostSplits[GetStageSplitCount()] / 6000,
                            (g_ghostSplits[GetStageSplitCount()] / 100) % 60, g_ghostSplits[GetStageSplitCount()] % 100);
                    goto draw_total;
                }
            } else {
                for (split = 0; split < NetRace_GetLastSentSplit(); split++) {
                    FormatCentisecondsAsMinSecMSec(StageTiming_GetDriverSplitClock(car, split + 1) - StageTiming_GetDriverSplitClock(car, split),
                                                   CFrontend::m_stringDest);
                    Font_DrawText(1, CFrontend::m_stringDest, x,
                                  (int)(g_pGraphics->resY * 0x91) / 480 + header +
                                      ((int)(g_pGraphics->resY * 0x1e) / 480) * split,
                                  (int *)pColour, 0x11);
                }
                for (split = NetRace_GetLastSentSplit(); split < GetStageSplitCount(); split++) {
                    if (split >= 0) {
                        sprintf(CFrontend::m_stringDest, "%s", g_noTimeText);
                        Font_DrawText(1, CFrontend::m_stringDest, x,
                                      (int)(g_pGraphics->resY * 0x91) / 480 + header +
                                          ((int)(g_pGraphics->resY * 0x1e) / 480) * split,
                                      (int *)pColour, 0x11);
                    }
                }
            }
            sprintf(CFrontend::m_stringDest, g_minSecMSECFormatString, StageTiming_GetValidStartTime(car) / 6000,
                    (StageTiming_GetValidStartTime(car) / 100) % 60, StageTiming_GetValidStartTime(car) % 100);
        draw_total:
            Font_DrawText(1, CFrontend::m_stringDest, x,
                          (int)(g_pGraphics->resY * 0x19) / 480 + (int)(g_pGraphics->resY * 0x91) / 480 + header +
                              ((int)(g_pGraphics->resY * 0x1e) / 480) * split,
                          (int *)pColour, 0x11);
        }
    }
    Font_SetBlendMode(2);
}

char Race_GetBaseCarCount(void);
int StageTiming_GetBoundedFinishOrderEntry(int index);
int RallyData_IsChampionshipFinalStage(void);
void GameMenus_DrawTextRow(int x, int y, char *pText, ...);
void GameMenu_ClearMenuListWithHighlight(void);
extern char g_standingsRowFormat[];

// Initials of the holder of the current stage's record, shown as the stage
// winner outside the arcade modes.
// GLOBAL: CMR2 0x00519edc
char g_stageRecordHolder[4] = "jml";
// GLOBAL: CMR2 0x00519f68
char g_winnerFormat[] = "%s, %s";

// Position of the car in the stage order, or -1.
// FUNCTION: CMR2 0x00451850
int GameMenu_FindCarStagePosition(int car)
{
    int i;

    for (i = 0; i < (BYTE)Race_GetBaseCarCount(); i++) {
        if (StageTiming_GetBoundedFinishOrderEntry(i) == car)
            return i;
    }
    return -1;
}

// Draw callback of the stage winner screen.
// FUNCTION: CMR2 0x0044d790
void GameMenu_DrawStageWinner(Menu *pMenu)
{
    int winner;
    int best;
    int i;
    int resY;

    best = 0x10;
    winner = -1;
    GameMenu_ClearMenuListWithHighlight();
    if (CGameInfo::GetConfiguredGameMode() != 5 && CGameInfo::GetConfiguredGameMode() != 6) {
        GameMenus_DrawTextRow((int)(g_pGraphics->resX * 30) / 640, (int)(g_pGraphics->resY * 242) / 480,
                              CFrontend::GetTextString(0x49), CFrontend::GetTextString(0x46), (char *)NULL);
        sprintf(CFrontend::m_stringDest, g_winnerFormat, g_stageRecordHolder, CFrontend::GetTextString(0x4a));
    } else {
        GameMenus_DrawTextRow((int)(g_pGraphics->resX * 30) / 640, (int)(g_pGraphics->resY * 242) / 480,
                              CFrontend::GetTextString(0x49), (char *)NULL);
        for (i = 0; i < (BYTE)RallyDataState(); i++) {
            if (GameMenu_FindCarStagePosition(i) < best) {
                best = GameMenu_FindCarStagePosition(i);
                winner = i;
            }
        }
        sprintf(CFrontend::m_stringDest, g_winnerFormat, (char *)RallyData_GetRecord(winner),
                CFrontend::GetTextString(0x4a));
    }
    CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
    resY = g_pGraphics->resY;
    Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 30) / 640,
                  (resY * 10) / 480 + (resY * 242) / 480 + Font_GetLineHeight(0), (int *)g_menuFrameColour, 0x11);
    Font_SetBlendMode(2);
}

// Draw callback of the scrolling stage split table: driver, split time, car
// and position ("=" when the time equals the previous row's).
// FUNCTION: CMR2 0x0044d960
void GameMenu_DrawScrollingStageSplits(Menu *pMenu)
{
    char stage[80];
    bool isPlayer;
    int rows;
    int row;
    int pos;
    int id;
    int time;
    int player;

    rows = (g_unk0x005413f8 | g_standingsExtraRow) ? 5 : 6;
    GameMenu_ClearMenuListWithHighlight();
    if (CGameInfo::GetConfiguredGameMode() == 0 || CGameInfo::GetConfiguredGameMode() == 1) {
        if (RallyDataStageIndex() == 10)
            sprintf(stage, CRegKey::m_regKeyPathFormatValue, CFrontend::GetTextString(0xba));
        else
            sprintf(stage, g_classRowHeaderFormat, CFrontend::GetTextString(0x40), RallyDataStageIndex() + 1);
        GameMenus_DrawTextRow((int)(g_pGraphics->resX * 30) / 640, (int)(g_pGraphics->resY * 0x43) / 480,
                              CFrontend::GetTextString(RallyDataCountryIndex() & 0xff),
                              CInput::FormatString(g_classRowHeaderFormat, CFrontend::GetTextString(0x44),
                                                   (RallyDataStageIndex() >> 2) + 1),
                              stage, (char *)NULL);
    } else if (RallyDataStageIndex() == 10) {
        GameMenus_DrawTextRow((int)(g_pGraphics->resX * 30) / 640, (int)(g_pGraphics->resY * 0x43) / 480,
                              CFrontend::GetTextString(RallyDataCountryIndex() & 0xff), CFrontend::GetTextString(0xba), (char *)NULL);
    } else {
        GameMenus_DrawTextRow((int)(g_pGraphics->resX * 30) / 640, (int)(g_pGraphics->resY * 0x43) / 480,
                              CFrontend::GetTextString(RallyDataCountryIndex() & 0xff),
                              CInput::FormatString(g_classRowHeaderFormat, CFrontend::GetTextString(0x40),
                                                   RallyDataStageIndex() + 1), (char *)NULL);
    }
    for (row = 0; row < rows; row++) {
        if (g_standingsExtraRow != 0) {
            row++;
            pos = (pMenu->items[0].max - 1) + row;
        } else {
            if (g_unk0x005413f8 != 0) {
                row++;
                pos = pMenu->items[0].max + row;
            } else {
                pos = pMenu->items[0].max + row;
            }
        }
        id = StageTiming_GetDriverIDForPosition(pos);
        time = StageTiming_GetTimeForPosition(pos);
        isPlayer = false;
        for (player = 0; player < CGameInfo::GetConfiguredPlayerCount(); player++) {
            if (StageTiming_GetCurrentSplitPositionOfDriver(player) == pos) {
                isPlayer = true;
                strcpy(CFrontend::m_stringDest, (char *)RallyData_GetRecord(player));
                break;
            }
        }
        if (!isPlayer)
            strcpy(CFrontend::m_stringDest, CAIHelper::GetNameForID(id));
        Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x5c) / 640,
                      (((int)(g_pGraphics->resY * 0x82) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * row) -
                       (int)(g_pGraphics->resY * 8) / 480) - g_gameMenuScrollY,
                      (int *)g_menuFrameColour, 9);
        FormatCentisecondsAsMinSecMSec(time, CFrontend::m_stringDest);
        Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x21e) / 640,
                      (((int)(g_pGraphics->resY * 0x82) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * row) -
                       (int)(g_pGraphics->resY * 8) / 480) - g_gameMenuScrollY,
                      (int *)g_menuFrameColour, 9);
        if (isPlayer) {
            strcpy(CFrontend::m_stringDest, (char *)CFrontend::GetModeSpecificCountryText(RallyData_GetDriverRecordSelectionValue(player)));
            CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
            g_menuRect[1] = (short)((int)(g_pGraphics->resY * 0x82) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * row -
                                    g_gameMenuScrollY - 1);
            g_menuRect[3] = (short)((int)(g_pGraphics->resY * 0xa3) / 480 - (int)(g_pGraphics->resY * 0x82) / 480 + 1);
            g_menuRect[0] = (short)((int)(g_pGraphics->resX * 0x20) / 640);
            g_menuRect[2] = (short)((int)(g_pGraphics->resX * 0x41) / 640 - (int)(g_pGraphics->resX * 0x20) / 640);
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_menuRect, g_menuRowFillColour, 2);
        } else {
            strcpy(CFrontend::m_stringDest, (char *)CFrontend::GetModeSpecificCountryText(RallyData_GetDriverSelectGridSlot(id)));
            CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
            GameMenus_DrawRowFrame(row, 0, 0);
        }
        Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x5c) / 640,
                      ((int)(g_pGraphics->resY * 0xa3) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * row) -
                          g_gameMenuScrollY,
                      (int *)g_menuFrameColour, 0x21);
        if (StageTiming_GetTimeForPosition(pos) != StageTiming_GetTimeForPosition(pos - 1))
            sprintf(CFrontend::m_stringDest, g_stageNumberFormat, pos + 1);
        else
            sprintf(CFrontend::m_stringDest, "%s", g_stageResultSameTime);
        Font_DrawText(1, CFrontend::m_stringDest,
                      ((int)(g_pGraphics->resX * 0x41) / 640 - (int)(g_pGraphics->resX * 0x20) / 640) / 2 +
                          (int)(g_pGraphics->resX * 0x20) / 640,
                      (((int)(g_pGraphics->resY * 8) / 480 +
                        ((int)(g_pGraphics->resY * 0xa3) / 480 - (int)(g_pGraphics->resY * 0x82) / 480) / 2 +
                        ((int)(g_pGraphics->resY * 0x34) / 480) * row) -
                       g_gameMenuScrollY) + (int)(g_pGraphics->resY * 0x82) / 480,
                      (int *)g_menuFrameColour, 0x12);
        if (g_standingsExtraRow != 0 || g_unk0x005413f8 != 0)
            row--;
    }
    Font_SetBlendMode(2);
}

extern char g_classRowHeaderFormat[];

// Draw callback of the stage times table (table 0), with the rally / stage
// header (in championship mode 8 also the leg number).
// FUNCTION: CMR2 0x0044e130
void GameMenu_DrawStageTimeStandings(Menu *pMenu)
{
    char stage[80];
    int i;

    GameMenu_ClearMenuListWithHighlight();
    if (CGameInfo::GetConfiguredGameMode() == 8) {
        if (RallyDataStageIndex() == 10)
            sprintf(stage, CRegKey::m_regKeyPathFormatValue, CFrontend::GetTextString(0xba));
        else
            sprintf(stage, g_classRowHeaderFormat, CFrontend::GetTextString(0x40), RallyDataStageIndex() + 1);
        GameMenus_DrawTextRow((int)(g_pGraphics->resX * 30) / 640, (int)(g_pGraphics->resY * 0x43) / 480,
                              CFrontend::GetTextString(RallyDataCountryIndex() & 0xff),
                              CInput::FormatString(g_classRowHeaderFormat, CFrontend::GetTextString(0x44),
                                                   (RallyDataStageIndex() >> 2) + 1),
                              stage, (char *)NULL);
    } else if (RallyDataStageIndex() == 10) {
        GameMenus_DrawTextRow((int)(g_pGraphics->resX * 30) / 640, (int)(g_pGraphics->resY * 0x43) / 480,
                              CFrontend::GetTextString(RallyDataCountryIndex() & 0xff),
                              CFrontend::GetTextString(0xba), (char *)NULL);
    } else {
        GameMenus_DrawTextRow((int)(g_pGraphics->resX * 30) / 640, (int)(g_pGraphics->resY * 0x43) / 480,
                              CFrontend::GetTextString(RallyDataCountryIndex() & 0xff),
                              CInput::FormatString(g_classRowHeaderFormat, CFrontend::GetTextString(0x40),
                                                   RallyDataStageIndex() + 1), (char *)NULL);
    }
    i = 0;
    if (NetPlayers_GetStandingCount() > 0) {
        do {
            Font_DrawText(1, NetPlayers_GetStandingName(i, 0), (int)(g_pGraphics->resX * 0x5c) / 0x280,
                          (int)(g_pGraphics->resY * 3) / 0x1e0 +
                              (((int)(g_pGraphics->resY * 0x82) / 0x1e0 +
                                ((int)(g_pGraphics->resY * 0x2d) / 0x1e0) * i) -
                               (int)(g_pGraphics->resY * 0x14) / 0x1e0) -
                              (int)(g_pGraphics->resY * 8) / 0x1e0,
                          (int *)g_menuFrameColour, 9);
            FormatCentisecondsAsMinSecMSec(NetPlayers_GetStandingTime(i, 0), CFrontend::m_stringDest);
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x21e) / 0x280,
                          (((int)(g_pGraphics->resY * 0x82) / 0x1e0 +
                            ((int)(g_pGraphics->resY * 0x2d) / 0x1e0) * i) -
                           (int)(g_pGraphics->resY * 0x14) / 0x1e0) -
                              (int)(g_pGraphics->resY * 8) / 0x1e0,
                          (int *)g_menuFrameColour, 9);
            if (NetPlayers_GetStandingPlayerIndex(i, 0) == -2) {
                strcpy(CFrontend::m_stringDest,
                       (char *)CFrontend::GetModeSpecificCountryText(NetPlayers_GetStandingCar(i, 0)));
                CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
                g_menuRect[1] = (short)((((int)(g_pGraphics->resY * 0x82) / 0x1e0 +
                                          ((int)(g_pGraphics->resY * 0x2d) / 0x1e0) * i) -
                                         (int)(g_pGraphics->resY * 0x14) / 0x1e0) -
                                        1);
                g_menuRect[3] = (short)(((int)(g_pGraphics->resY * 0xa3) / 0x1e0 -
                                          (int)(g_pGraphics->resY * 0x82) / 0x1e0) +
                                        1);
                g_menuRect[0] = (short)((int)(g_pGraphics->resX * 0x20) / 0x280);
                g_menuRect[2] = (short)((int)(g_pGraphics->resX * 0x41) / 0x280 -
                                        (int)(g_pGraphics->resX * 0x20) / 0x280);
                Sprite_FillRect(&g_pGraphics->field309_0x150, g_menuRect, g_menuRowFillColour, 2);
            } else {
                strcpy(CFrontend::m_stringDest,
                       (char *)CFrontend::GetModeSpecificCountryText(NetPlayers_GetStandingCar(i, 0)));
                CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
                GameMenus_DrawRowFrame(i, (-(int)g_pGraphics->resY * 0x14) / 0x1e0, 1);
            }
            Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x5c) / 0x280,
                          ((int)(g_pGraphics->resY * 0xa3) / 0x1e0 +
                           ((int)(g_pGraphics->resY * 0x2d) / 0x1e0) * i) -
                              (int)(g_pGraphics->resY * 0x14) / 0x1e0,
                          (int *)g_menuFrameColour, 0x21);
            if (i == 0 || (i > 0 && NetPlayers_GetPlayerRank(i, 0) != NetPlayers_GetPlayerRank(i - 1, 0))) {
                sprintf(CFrontend::m_stringDest, g_stageNumberFormat, NetPlayers_GetPlayerRank(i, 0));
            } else {
                sprintf(CFrontend::m_stringDest, "%s", g_stageResultSameTime);
            }
            Font_DrawText(1, CFrontend::m_stringDest,
                          ((int)(g_pGraphics->resX * 0x41) / 0x280 -
                           (int)(g_pGraphics->resX * 0x20) / 0x280) / 2 +
                              (int)(g_pGraphics->resX * 0x20) / 0x280,
                          (int)(g_pGraphics->resY * 8) / 0x1e0 +
                              (int)(g_pGraphics->resY * 0x82) / 0x1e0 +
                              ((((int)(g_pGraphics->resY * 0xa3) / 0x1e0 -
                                 (int)(g_pGraphics->resY * 0x82) / 0x1e0) / 2 +
                                ((int)(g_pGraphics->resY * 0x2d) / 0x1e0) * i) -
                               (int)(g_pGraphics->resY * 0x14) / 0x1e0),
                          (int *)g_menuFrameColour, 0x12);
            i++;
        } while (i < NetPlayers_GetStandingCount());
    }
    Font_SetBlendMode(2);
}

void GameMenus_DrawRowHighlight(short row);
BYTE RallyData_GetDriverSelectGridSlot(int param1);
int StageTiming_GetValidStartTime(int index);
void GameMenu_DrawInGameBreadcrumb(Menu *pMenu);
void GameMenus_DrawRowSeparator(short row);
extern int g_gameMenuScrollY;
extern int g_gameMenuRowYOffset;

// Set when the standings menus of the stage end / pause screens show their
// extra first row.
// GLOBAL: CMR2 0x0053f5a8
int g_standingsExtraRow;
// GLOBAL: CMR2 0x005413f8
int g_unk0x005413f8;

// Draw callback of the scrolling championship points table: six rows from
// the position in item 0, each with the driver, its points, its car and its
// position; rows of the human drivers are highlighted and the top five are
// separated from the rest.
// FUNCTION: CMR2 0x0044efa0
void GameMenu_DrawScrollingChampionshipPoints(Menu *pMenu)
{
    bool isPlayer;
    int rows;
    int row;
    int pos;
    int id;
    int points;
    int player;
    int x;

    x = (int)(g_pGraphics->resX * 30) / 640;
    rows = (g_unk0x005413f8 | g_standingsExtraRow) ? 5 : 6;
    GameMenu_ClearMenuListWithHighlight();
    GameMenus_DrawTextRow(x, (int)(g_pGraphics->resY * 0x43) / 480,
                          CFrontend::GetTextString(RallyDataCountryIndex() & 0xff), CFrontend::GetTextString(0x41),
                          CFrontend::GetTextString(0x8d), (char *)NULL);
    for (row = 0; row < rows; row++) {
        if (g_standingsExtraRow != 0) {
            row++;
            pos = (pMenu->items[0].max - 1) + row;
        } else {
            if (g_unk0x005413f8 != 0)
                row++;
            pos = pMenu->items[0].max + row;
        }
        if (pos > 5 && pMenu->items[0].max < 6)
            g_gameMenuRowYOffset = (int)(g_pGraphics->resY * 10) / 480;
        else
            g_gameMenuRowYOffset = 0;
        id = RallyTiming_GetOverallPositionDriverID(pos);
        points = RallyTiming_GetStagePenalty(id, RallyDataCountryIndex());
        isPlayer = false;
        for (player = 0; player < CGameInfo::GetConfiguredPlayerCount(); player++) {
            if (RallyTiming_GetOverallPositionOfDriver(StageTiming_GetDriverSlot(player)) == pos) {
                isPlayer = true;
                strcpy(CFrontend::m_stringDest, (char *)RallyData_GetRecord(player));
                break;
            }
        }
        if (!isPlayer)
            strcpy(CFrontend::m_stringDest, CAIHelper::GetNameForID(id));
        Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x5c) / 640,
                      (((int)(g_pGraphics->resY * 0x82) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * row) -
                       (int)(g_pGraphics->resY * 8) / 480) - g_gameMenuScrollY + g_gameMenuRowYOffset,
                      (int *)g_menuFrameColour, 9);
        sprintf(CFrontend::m_stringDest, g_stageNumberFormat, points);
        Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x21e) / 640,
                      (((int)(g_pGraphics->resY * 0x82) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * row) -
                       (int)(g_pGraphics->resY * 8) / 480) - g_gameMenuScrollY + g_gameMenuRowYOffset,
                      (int *)g_menuFrameColour, 9);
        if (isPlayer) {
            strcpy(CFrontend::m_stringDest, (char *)CFrontend::GetModeSpecificCountryText(RallyData_GetDriverRecordSelectionValue(player)));
            CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
            g_menuRect[1] = (short)((int)(g_pGraphics->resY * 0x82) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * row -
                                    g_gameMenuScrollY - 1 + g_gameMenuRowYOffset);
            g_menuRect[3] = (short)((int)(g_pGraphics->resY * 0xa3) / 480 - (int)(g_pGraphics->resY * 0x82) / 480 + 1);
            g_menuRect[0] = (short)((int)(g_pGraphics->resX * 0x20) / 640);
            g_menuRect[2] = (short)((int)(g_pGraphics->resX * 0x41) / 640 - (int)(g_pGraphics->resX * 0x20) / 640);
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_menuRect, g_menuRowFillColour, 2);
        } else {
            if (pos <= 5)
                GameMenus_DrawRowHighlight(row);
            strcpy(CFrontend::m_stringDest, (char *)CFrontend::GetModeSpecificCountryText(RallyData_GetDriverSelectGridSlot(id)));
            CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
            GameMenus_DrawRowFrame(row, 0, 0);
        }
        Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x5c) / 640,
                      ((int)(g_pGraphics->resY * 0xa3) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * row) -
                          g_gameMenuScrollY + g_gameMenuRowYOffset,
                      (int *)g_menuFrameColour, 0x21);
        sprintf(CFrontend::m_stringDest, g_stageNumberFormat, pos + 1);
        Font_DrawText(1, CFrontend::m_stringDest,
                      ((int)(g_pGraphics->resX * 0x41) / 640 - (int)(g_pGraphics->resX * 0x20) / 640) / 2 +
                          (int)(g_pGraphics->resX * 0x20) / 640,
                      (((int)(g_pGraphics->resY * 8) / 480 +
                        ((int)(g_pGraphics->resY * 0xa3) / 480 - (int)(g_pGraphics->resY * 0x82) / 480) / 2 +
                        ((int)(g_pGraphics->resY * 0x34) / 480) * row) -
                       g_gameMenuScrollY) + (int)(g_pGraphics->resY * 0x82) / 480 + g_gameMenuRowYOffset,
                      (int *)g_menuFrameColour, 0x12);
        if (pos == 5 && pMenu->items[0].max != 0)
            GameMenus_DrawRowSeparator(row);
        if (g_standingsExtraRow != 0 || g_unk0x005413f8 != 0)
            row--;
    }
    Font_SetBlendMode(2);
}

// Same as GameMenu_DrawScrollingChampionshipPoints for the overall rally times table.
// FUNCTION: CMR2 0x0044fea0
void GameMenu_DrawScrollingRallyTimes(Menu *pMenu)
{
    bool isPlayer;
    int x;
    int rows;
    int row;
    int pos;
    int id;
    int time;
    int player;
    int slot;

    x = (int)(g_pGraphics->resX * 30) / 640;
    rows = (g_unk0x005413f8 | g_standingsExtraRow) ? 5 : 6;
    GameMenu_ClearMenuListWithHighlight();
    if (!g_unk0x00541210 || !g_unk0x00540898)
        GameMenus_DrawTextRow(x, (int)(g_pGraphics->resY * 0x43) / 480,
                              CFrontend::GetTextString(RallyDataCountryIndex() & 0xff), CFrontend::GetTextString(0x41), (char *)NULL);
    else
        GameMenus_DrawTextRow(x, (int)(g_pGraphics->resY * 0x43) / 480,
                              CFrontend::GetTextString(RallyDataCountryIndex() & 0xff), CFrontend::GetTextString(0x41),
                              CFrontend::GetTextString(0xef), (char *)NULL);
    for (row = 0; row < rows; row++) {
        if (g_standingsExtraRow != 0) {
            row++;
            pos = (pMenu->items[0].max - 1) + row;
        } else {
            if (g_unk0x005413f8 != 0)
                row++;
            pos = pMenu->items[0].max + row;
        }
        if (pos <= 5 || pMenu->items[0].max >= 6)
            g_gameMenuRowYOffset = 0;
        else
            g_gameMenuRowYOffset = (int)(g_pGraphics->resY * 10) / 480;
        id = RallyTiming_GetOverallPositionDriverID(pos);
        time = RallyTiming_GetOverallTimeForPosition(pos);
        isPlayer = FALSE;
        for (player = 0; player < CGameInfo::GetConfiguredPlayerCount(); player++) {
            if (RallyTiming_GetOverallPositionOfDriver(0xf - player) == pos) {
                isPlayer = TRUE;
                strcpy(CFrontend::m_stringDest, (char *)RallyData_GetRecord(player));
                break;
            }
        }
        if (!isPlayer)
            strcpy(CFrontend::m_stringDest, CAIHelper::GetNameForID(id));
        Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x5c) / 640,
                      (((int)(g_pGraphics->resY * 0x82) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * row) -
                       (int)(g_pGraphics->resY * 8) / 480) - g_gameMenuScrollY + g_gameMenuRowYOffset,
                      (int *)g_menuFrameColour, 9);
        FormatCentisecondsAsMinSecMSec(time, CFrontend::m_stringDest);
        Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x21e) / 640,
                      (((int)(g_pGraphics->resY * 0x82) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * row) -
                       (int)(g_pGraphics->resY * 8) / 480) - g_gameMenuScrollY + g_gameMenuRowYOffset,
                      (int *)g_menuFrameColour, 9);
        if (isPlayer) {
            strcpy(CFrontend::m_stringDest, (char *)CFrontend::GetModeSpecificCountryText(RallyData_GetDriverRecordSelectionValue(player)));
            CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
            g_menuRect[1] = (short)((int)(g_pGraphics->resY * 0x82) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * row -
                                    g_gameMenuScrollY - 1 + g_gameMenuRowYOffset);
            g_menuRect[3] = (short)((int)(g_pGraphics->resY * 0xa3) / 480 - (int)(g_pGraphics->resY * 0x82) / 480 + 1);
            g_menuRect[0] = (short)((int)(g_pGraphics->resX * 0x20) / 640);
            g_menuRect[2] = (short)((int)(g_pGraphics->resX * 0x41) / 640 - (int)(g_pGraphics->resX * 0x20) / 640);
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_menuRect, g_menuRowFillColour, 2);
        } else {
            if (pos <= 5)
                GameMenus_DrawRowHighlight(row);
            strcpy(CFrontend::m_stringDest, (char *)CFrontend::GetModeSpecificCountryText(RallyData_GetDriverSelectGridSlot(id)));
            CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
            GameMenus_DrawRowFrame(row, 0, 0);
        }
        Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x5c) / 640,
                      ((int)(g_pGraphics->resY * 0xa3) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * row) -
                          g_gameMenuScrollY + g_gameMenuRowYOffset,
                      (int *)g_menuFrameColour, 0x21);
        sprintf(CFrontend::m_stringDest, g_stageNumberFormat, pos + 1);
        Font_DrawText(1, CFrontend::m_stringDest,
                      ((int)(g_pGraphics->resX * 0x41) / 640 - (int)(g_pGraphics->resX * 0x20) / 640) / 2 +
                          (int)(g_pGraphics->resX * 0x20) / 640,
                      (((int)(g_pGraphics->resY * 8) / 480 +
                        ((int)(g_pGraphics->resY * 0xa3) / 480 - (int)(g_pGraphics->resY * 0x82) / 480) / 2 +
                        ((int)(g_pGraphics->resY * 0x34) / 480) * row) -
                       g_gameMenuScrollY) + (int)(g_pGraphics->resY * 0x82) / 480 + g_gameMenuRowYOffset,
                      (int *)g_menuFrameColour, 0x12);
        if (pos == 5 && pMenu->items[0].max != 0)
            GameMenus_DrawRowSeparator(row);
        if (g_standingsExtraRow != 0 || g_unk0x005413f8 != 0)
            row--;
    }
    Font_SetBlendMode(2);
}

// Draw callback of the championship table (table 1 of the network/overall
// standings), with the rally name header.
// FUNCTION: CMR2 0x004505b0
void GameMenu_DrawChampionshipTimeStandings(Menu *pMenu)
{
    int x;
    int i;

    x = (int)(g_pGraphics->resX * 30) / 640;
    GameMenu_ClearMenuListWithHighlight();
    if (!g_unk0x00541210 || !g_unk0x00540898)
        GameMenus_DrawTextRow(x, (int)(g_pGraphics->resY * 0x43) / 480,
                              CFrontend::GetTextString(RallyDataCountryIndex() & 0xff), CFrontend::GetTextString(0x41), (char *)NULL);
    else
        GameMenus_DrawTextRow(x, (int)(g_pGraphics->resY * 0x43) / 480,
                              CFrontend::GetTextString(RallyDataCountryIndex() & 0xff), CFrontend::GetTextString(0x41),
                              CFrontend::GetTextString(0xef), (char *)NULL);
    i = 0;
    if (NetPlayers_GetStandingCount() > 0) {
        do {
            Font_DrawText(1, NetPlayers_GetStandingName(i, 1), (int)(g_pGraphics->resX * 0x5c) / 0x280,
                          (int)(g_pGraphics->resY * 3) / 0x1e0 +
                              (((int)(g_pGraphics->resY * 0x82) / 0x1e0 +
                                ((int)(g_pGraphics->resY * 0x2d) / 0x1e0) * i) -
                               (int)(g_pGraphics->resY * 0x14) / 0x1e0) -
                              (int)(g_pGraphics->resY * 8) / 0x1e0,
                          (int *)g_menuFrameColour, 9);
            FormatCentisecondsAsMinSecMSec(NetPlayers_GetStandingTime(i, 1), CFrontend::m_stringDest);
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x21e) / 0x280,
                          (((int)(g_pGraphics->resY * 0x82) / 0x1e0 +
                            ((int)(g_pGraphics->resY * 0x2d) / 0x1e0) * i) -
                           (int)(g_pGraphics->resY * 0x14) / 0x1e0) -
                              (int)(g_pGraphics->resY * 8) / 0x1e0,
                          (int *)g_menuFrameColour, 9);
            if (NetPlayers_GetStandingPlayerIndex(i, 1) == -2) {
                strcpy(CFrontend::m_stringDest,
                       (char *)CFrontend::GetModeSpecificCountryText(NetPlayers_GetStandingCar(i, 1)));
                CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
                g_menuRect[1] = (short)((((int)(g_pGraphics->resY * 0x82) / 0x1e0 +
                                          ((int)(g_pGraphics->resY * 0x2d) / 0x1e0) * i) -
                                         (int)(g_pGraphics->resY * 0x14) / 0x1e0) -
                                        1);
                g_menuRect[3] = (short)(((int)(g_pGraphics->resY * 0xa3) / 0x1e0 -
                                          (int)(g_pGraphics->resY * 0x82) / 0x1e0) +
                                        1);
                g_menuRect[0] = (short)((int)(g_pGraphics->resX * 0x20) / 0x280);
                g_menuRect[2] = (short)((int)(g_pGraphics->resX * 0x41) / 0x280 -
                                        (int)(g_pGraphics->resX * 0x20) / 0x280);
                Sprite_FillRect(&g_pGraphics->field309_0x150, g_menuRect, g_menuRowFillColour, 2);
            } else {
                strcpy(CFrontend::m_stringDest,
                       (char *)CFrontend::GetModeSpecificCountryText(NetPlayers_GetStandingCar(i, 1)));
                CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
                GameMenus_DrawRowFrame(i, (-(int)g_pGraphics->resY * 0x14) / 0x1e0, 1);
            }
            Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x5c) / 0x280,
                          ((int)(g_pGraphics->resY * 0xa3) / 0x1e0 +
                           ((int)(g_pGraphics->resY * 0x2d) / 0x1e0) * i) -
                              (int)(g_pGraphics->resY * 0x14) / 0x1e0,
                          (int *)g_menuFrameColour, 0x21);
            if (i == 0 || (i > 0 && NetPlayers_GetPlayerRank(i, 1) != NetPlayers_GetPlayerRank(i - 1, 1))) {
                sprintf(CFrontend::m_stringDest, g_stageNumberFormat, NetPlayers_GetPlayerRank(i, 1));
            } else {
                sprintf(CFrontend::m_stringDest, "%s", g_stageResultSameTime);
            }
            Font_DrawText(1, CFrontend::m_stringDest,
                          ((int)(g_pGraphics->resX * 0x41) / 0x280 -
                           (int)(g_pGraphics->resX * 0x20) / 0x280) / 2 +
                              (int)(g_pGraphics->resX * 0x20) / 0x280,
                          (int)(g_pGraphics->resY * 8) / 0x1e0 +
                              (int)(g_pGraphics->resY * 0x82) / 0x1e0 +
                              ((((int)(g_pGraphics->resY * 0xa3) / 0x1e0 -
                                 (int)(g_pGraphics->resY * 0x82) / 0x1e0) / 2 +
                                ((int)(g_pGraphics->resY * 0x2d) / 0x1e0) * i) -
                               (int)(g_pGraphics->resY * 0x14) / 0x1e0),
                          (int *)g_menuFrameColour, 0x12);
            i++;
        } while (i < NetPlayers_GetStandingCount());
    }
    Font_SetBlendMode(2);
}

// Draw callback of the championship standings screen: the best driver's
// position decides between the "champion" and "rally over" headers.
// FUNCTION: CMR2 0x00450c10
void GameMenu_DrawChampionshipOutcome(Menu *pMenu)
{
    char position[100];
    int x;
    int y;
    int best;
    int i;
    int slot;
    int place;
    int resY;

    x = (int)(g_pGraphics->resX * 30) / 640;
    y = (int)(g_pGraphics->resY * 242) / 480;
    GameMenu_ClearMenuListWithHighlight();
    best = 99;
    i = 0;
    slot = best;
    if (CGameInfo::GetConfiguredPlayerCount() > 0) {
        slot = 0xf;
        do {
            if (RallyTiming_GetOverallPositionOfDriver(slot) < best)
                best = RallyTiming_GetOverallPositionOfDriver(slot);
            i++;
            slot--;
        } while (i < CGameInfo::GetConfiguredPlayerCount());
        slot = best;
    }
    if (slot < 6)
        GameMenus_DrawTextRow(x, y, CFrontend::GetTextString(RallyDataCountryIndex() & 0xff),
                              CFrontend::GetTextString(0x41), CFrontend::GetTextString(0x49), (char *)NULL);
    else
        GameMenus_DrawTextRow(x, y, CFrontend::GetTextString(RallyDataCountryIndex() & 0xff),
                              CFrontend::GetTextString(0x41), CFrontend::GetTextString(0x88), (char *)NULL);
    for (i = 0; i < CGameInfo::GetConfiguredPlayerCount(); i++) {
        place = RallyTiming_GetOverallPositionOfDriver(StageTiming_GetDriverSlot(i));
        switch (place) {
        case 0:
            sprintf(CFrontend::m_stringDest, g_standingsRowFormat, (char *)RallyData_GetRecord(i), CFrontend::GetTextString(0x51));
            break;
        case 1:
            sprintf(CFrontend::m_stringDest, g_standingsRowFormat, (char *)RallyData_GetRecord(i), CFrontend::GetTextString(0x52));
            break;
        case 2:
            sprintf(CFrontend::m_stringDest, g_standingsRowFormat, (char *)RallyData_GetRecord(i), CFrontend::GetTextString(0x53));
            break;
        default:
            sprintf(position, CFrontend::GetTextString(0x54), place + 1);
            sprintf(CFrontend::m_stringDest, g_standingsRowFormat, (char *)RallyData_GetRecord(i), position);
            break;
        }
        CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
        resY = g_pGraphics->resY;
        Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 30) / 640,
                      (resY * 10) / 480 + ((resY * 20) / 480) * i + y + Font_GetLineHeight(0),
                      (int *)g_menuFrameColour, 0x11);
    }
    if (best > 5) {
        if ((BYTE)RallyDataCountryIndex() == 7)
            sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(0xee));
        else
            sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(0xbb));
        CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
        resY = g_pGraphics->resY;
        Font_DrawText(0, CFrontend::m_stringDest, x, (resY * 10) / 480 + ((resY * 20) / 480) * i + y + Font_GetLineHeight(0),
                      (int *)g_menuFrameColour, 0x11);
    }
    Font_SetBlendMode(2);
}

// GLOBAL: CMR2 0x00519f74
char g_networkRallyEndMenuName[] = "NetworkRallyRallyEndMenu_Display";

// Placeholder draw callback of the network rally end menu.
// FUNCTION: CMR2 0x00450ed0
void GameMenu_NoOpNetworkRallyEndDraw(Menu *pMenu)
{
    Font_DrawText(0, g_networkRallyEndMenuName, 100, 100, (int *)g_menuFrameColour, 9);
}

int RallyTiming_GetStageTieBreak(int index);
int RallyTiming_GetStageTimeSeconds(int iDriver);
int RallyTiming_GetStageOrderDriverID(int iPosition);

// Stage points of the sixteen stage positions, for the tie checks below.
// GLOBAL: CMR2 0x005418c4
BYTE g_stagePositionSeconds[0x10];

// Draw callback of the scrolling stage points table: driver, car, points and
// position; tied drivers get the tie-break note (count-back) and "=" instead
// of their position.
// FUNCTION: CMR2 0x00450ef0
void GameMenu_DrawScrollingStagePoints(Menu *pMenu)
{
    bool isPlayer;
    int x;
    int rows;
    int row;
    int pos;
    int id;
    int points;
    int player;
    int i;

    x = (int)(g_pGraphics->resX * 30) / 640;
    rows = (g_unk0x005413f8 | g_standingsExtraRow) ? 5 : 6;
    GameMenu_ClearMenuListWithHighlight();
    GameMenus_DrawTextRow(x, (int)(g_pGraphics->resY * 0x43) / 480,
                          CFrontend::GetTextString(RallyDataCountryIndex() & 0xff), CFrontend::GetTextString(0x42),
                          CFrontend::GetTextString(0x8d), (char *)NULL);
    i = 0;
    do {
        g_stagePositionSeconds[i] = (BYTE)RallyTiming_GetStageTimeSeconds(RallyTiming_GetStageOrderDriverID(i));
        i++;
    } while (i < 0x10);
    for (row = 0; row < rows; row++) {
        if (g_standingsExtraRow != 0) {
            row++;
            pos = (pMenu->items[0].max - 1) + row;
        } else {
            if (g_unk0x005413f8 != 0)
                row++;
            pos = pMenu->items[0].max + row;
        }
        id = RallyTiming_GetStageOrderDriverID(pos);
        points = RallyTiming_GetStageTimeSeconds(id);
        isPlayer = FALSE;
        for (player = 0; player < CGameInfo::GetConfiguredPlayerCount(); player++) {
            if (StageTiming_GetDriverSlot(player) == id) {
                isPlayer = TRUE;
                strcpy(CFrontend::m_stringDest, (char *)RallyData_GetRecord(player));
                break;
            }
        }
        if (!isPlayer)
            strcpy(CFrontend::m_stringDest, CAIHelper::GetNameForID(id));
        Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x5c) / 640,
                      (((int)(g_pGraphics->resY * 0x82) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * row) -
                       (int)(g_pGraphics->resY * 8) / 480) - g_gameMenuScrollY,
                      (int *)g_menuFrameColour, 9);
        if (isPlayer) {
            strcpy(CFrontend::m_stringDest, (char *)CFrontend::GetModeSpecificCountryText(RallyData_GetDriverRecordSelectionValue(player)));
            CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
            g_menuRect[1] = (short)((int)(g_pGraphics->resY * 0x82) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * row -
                                    g_gameMenuScrollY - 1);
            g_menuRect[3] = (short)((int)(g_pGraphics->resY * 0xa3) / 480 - (int)(g_pGraphics->resY * 0x82) / 480 + 1);
            g_menuRect[0] = (short)((int)(g_pGraphics->resX * 0x20) / 640);
            g_menuRect[2] = (short)((int)(g_pGraphics->resX * 0x41) / 640 - (int)(g_pGraphics->resX * 0x20) / 640);
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_menuRect, g_menuRowFillColour, 2);
        } else {
            strcpy(CFrontend::m_stringDest, (char *)CFrontend::GetModeSpecificCountryText(RallyData_GetDriverSelectGridSlot(id)));
            CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
            GameMenus_DrawRowFrame(row, 0, 0);
        }
        Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x5c) / 640,
                      ((int)(g_pGraphics->resY * 0xa3) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * row) -
                          g_gameMenuScrollY,
                      (int *)g_menuFrameColour, 0x21);
        sprintf(CFrontend::m_stringDest, g_stageNumberFormat, points);
        Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x21e) / 640,
                      (((int)(g_pGraphics->resY * 0x82) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * row) -
                       (int)(g_pGraphics->resY * 8) / 480) - g_gameMenuScrollY,
                      (int *)g_menuFrameColour, 9);
        if (g_stagePositionSeconds[pos] != 0) {
            for (i = 0; i < 0x10; i++) {
                if (i != pos && g_stagePositionSeconds[pos] == g_stagePositionSeconds[i]) {
                    if (RallyTiming_GetStageTieBreak(id) == 1)
                        sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0xb9), RallyTiming_GetStageTieBreak(id));
                    else
                        sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0xb7), RallyTiming_GetStageTieBreak(id));
                    Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x1fe) / 640,
                                  (((int)(g_pGraphics->resY * 0x82) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * row) -
                                   (int)(g_pGraphics->resY * 8) / 480) - g_gameMenuScrollY,
                                  (int *)g_menuFrameColour, 0xc);
                    if (pos > 0 && RallyTiming_GetStageTieBreak(RallyTiming_GetStageOrderDriverID(pos - 1)) == RallyTiming_GetStageTieBreak(id) &&
                        g_stagePositionSeconds[pos - 1] == g_stagePositionSeconds[pos]) {
                        sprintf(CFrontend::m_stringDest, "%s", g_stageResultSameTime);
                        goto draw;
                    }
                    break;
                }
            }
        }
        sprintf(CFrontend::m_stringDest, g_stageNumberFormat, pos + 1);
    draw:
        Font_DrawText(1, CFrontend::m_stringDest,
                      ((int)(g_pGraphics->resX * 0x41) / 640 - (int)(g_pGraphics->resX * 0x20) / 640) / 2 +
                          (int)(g_pGraphics->resX * 0x20) / 640,
                      (((int)(g_pGraphics->resY * 8) / 480 +
                        ((int)(g_pGraphics->resY * 0xa3) / 480 - (int)(g_pGraphics->resY * 0x82) / 480) / 2 +
                        ((int)(g_pGraphics->resY * 0x34) / 480) * row) -
                       g_gameMenuScrollY) + (int)(g_pGraphics->resY * 0x82) / 480,
                      (int *)g_menuFrameColour, 0x12);
        if (g_standingsExtraRow != 0 || g_unk0x005413f8 != 0)
            row--;
    }
    Font_SetBlendMode(2);
}

// Draw callback of the final championship standings (header menu) screen.
// match 86%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00451690
void GameMenu_DrawFinalChampionshipStandings(Menu *pMenu)
{
    char position[100];
    char *pPosition;
    int i;
    int place;
    int resY;

    if (g_pHeaderMenu == &g_menu0x0053f790) {
        GameMenu_ClearMenuListWithHighlight();
        GameMenus_DrawTextRow((int)(g_pGraphics->resX * 30) / 640, (int)(g_pGraphics->resY * 242) / 480,
                              CFrontend::GetTextString(0x42), CFrontend::GetTextString(CGameInfo::GetConfiguredDifficulty() + 0x8e), (char *)NULL);
        for (i = 0; i < CGameInfo::GetConfiguredPlayerCount(); i++) {
            place = RallyTiming_GetStagePositionOfDriver(StageTiming_GetDriverSlot(i));
            switch (place) {
            case 0:
                sprintf(CFrontend::m_stringDest, g_standingsRowFormat, (char *)RallyData_GetRecord(i), CFrontend::GetTextString(0x51));
                break;
            case 1:
                sprintf(CFrontend::m_stringDest, g_standingsRowFormat, (char *)RallyData_GetRecord(i), CFrontend::GetTextString(0x52));
                break;
            case 2:
                sprintf(CFrontend::m_stringDest, g_standingsRowFormat, (char *)RallyData_GetRecord(i), CFrontend::GetTextString(0x53));
                break;
            default:
                sprintf(position, CFrontend::GetTextString(0x54), place + 1);
                sprintf(CFrontend::m_stringDest, g_standingsRowFormat, (char *)RallyData_GetRecord(i), position);
                break;
            }
            CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
            resY = g_pGraphics->resY;
            Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 30) / 640,
                          (resY * 242) / 480 + ((resY * 10) / 480 + Font_GetLineHeight(0)) * (i + 1),
                          (int *)g_menuFrameColour, 0x11);
        }
    } else {
        if (!(BYTE)RallyData_IsChampionshipFinalStage())
            GameMenu_ClearMenuListWithHighlight();
    }
}

extern char g_pointsFormat[];
extern char g_plusTimeFormat[];
extern BYTE g_menuRowFillColour[4];
extern char g_stageNumberFormat[];
extern char g_stageResultSameTime[];
int RallyTiming_GetPositionPoints(int position);
int StageTiming_GetValidStartTime(int index);
void GameMenu_DrawInGameBreadcrumb(Menu *pMenu);
int StageTiming_GetPendingDriverEntry(int index);
BYTE RallyData_GetDriverSelectGridSlot(int param1);
int StageTiming_GetValidStartTime(int index);
void GameMenu_DrawInGameBreadcrumb(Menu *pMenu);
void FormatCentisecondsAsMinSecMSec(int iTime, char *pcFormattedTime);
void GameMenus_DrawRowFrame(short row, short yOffset, char compact);
void GameMenu_DrawInGameBreadcrumb(Menu *pMenu);
unsigned int RallyData_GetSetupModeBits(void);
unsigned int RallyData_GetSetupLowNibble(void);
unsigned int RallyData_GetSetupFlag11(void);

// Draw callback of the stage classification table: one boxed row per car
// with its name, time (or target time, or points) and car, the players'
// rows highlighted, and the position number on the left ("=" for a tie).
// FUNCTION: CMR2 0x00451df0
void GameMenu_DrawStageClassification(Menu *pMenu)
{
    char diff[12];
    bool isPlayer;
    int i;
    int next;
    int car;
    int time;
    int flags;
    char *pFormat;

    GameMenu_ClearMenuListWithHighlight();
    GameMenu_DrawInGameBreadcrumb(pMenu);
    for (i = 0; i < (BYTE)Race_GetBaseCarCount(); i = next) {
        isPlayer = FALSE;
        car = StageTiming_GetBoundedFinishOrderEntry(i);
        time = StageTiming_GetValidStartTime(car);
        if (car < (int)(RallyDataState() & 0xff)) {
            isPlayer = TRUE;
            strcpy(CFrontend::m_stringDest, (char *)RallyData_GetRecord(car));
        } else {
            strcpy(CFrontend::m_stringDest, CAIHelper::GetNameForID(car));
        }
        Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x5c) / 640,
                      ((int)(g_pGraphics->resY * 0x82) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * i) -
                          (int)(g_pGraphics->resY * 8) / 480,
                      (int *)g_menuFrameColour, 9);
        flags = 9;
        if (g_pHeaderMenu == &g_menu0x0053ec48) {
            sprintf(CFrontend::m_stringDest, g_stageNumberFormat, RallyTiming_GetPositionPoints(i));
        } else if (!(BYTE)RallyData_GetSetupFlag11() ||
                   (RallyData_GetSetupModeBits() != 1 && RallyData_GetSetupModeBits() != 2)) {
            FormatCentisecondsAsMinSecMSec(time, CFrontend::m_stringDest);
        } else if (RallyData_GetSetupModeBits() == 1) {
            flags = 0xc;
            if (i == 0) {
                FormatCentisecondsAsMinSecMSec(time - RallyData_GetSetupLowNibble() * 100, CFrontend::m_stringDest);
            } else {
                FormatCentisecondsAsMinSecMSec(RallyData_GetSetupLowNibble() * 100, diff);
                sprintf(CFrontend::m_stringDest, g_plusTimeFormat, diff);
            }
        } else {
            sprintf(CFrontend::m_stringDest, g_pointsFormat, StageTiming_GetPendingDriverEntry(car));
        }
        Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x21e) / 640,
                      ((int)(g_pGraphics->resY * 0x82) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * i) -
                          (int)(g_pGraphics->resY * 8) / 480,
                      (int *)g_menuFrameColour, flags);
        if (isPlayer) {
            strcpy(CFrontend::m_stringDest, (char *)CFrontend::GetModeSpecificCountryText(RallyData_GetDriverRecordSelectionValue(car)));
            CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
            g_menuRect[1] = (short)((int)(g_pGraphics->resY * 0x82) / 480 +
                                    ((int)(g_pGraphics->resY * 0x34) / 480) * i - 1);
            g_menuRect[3] = (short)((int)(g_pGraphics->resY * 0xa3) / 480 - (int)(g_pGraphics->resY * 0x82) / 480 + 1);
            g_menuRect[0] = (short)((int)(g_pGraphics->resX * 0x20) / 640);
            g_menuRect[2] = (short)((int)(g_pGraphics->resX * 0x41) / 640 - (int)(g_pGraphics->resX * 0x20) / 640);
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_menuRect, g_menuRowFillColour, 2);
        } else {
            strcpy(CFrontend::m_stringDest, (char *)CFrontend::GetModeSpecificCountryText(RallyData_GetDriverSelectGridSlot(car)));
            CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
            GameMenus_DrawRowFrame(i, 0, 0);
        }
        Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x5c) / 640,
                      (int)(g_pGraphics->resY * 0xa3) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * i,
                      (int *)g_menuFrameColour, 0x21);
        if (RallyData_GetSetupModeBits() == 1 || RallyData_GetSetupModeBits() == 2) {
            next = i + 1;
            sprintf(CFrontend::m_stringDest, g_stageNumberFormat, next);
        } else if (StageTiming_GetValidStartTime(StageTiming_GetBoundedFinishOrderEntry(i)) == StageTiming_GetValidStartTime(StageTiming_GetBoundedFinishOrderEntry(i - 1))) {
            next = i + 1;
            sprintf(CFrontend::m_stringDest, g_stageResultSameTime, next);
        } else {
            next = i + 1;
            sprintf(CFrontend::m_stringDest, g_stageNumberFormat, next);
        }
        Font_DrawText(1, CFrontend::m_stringDest,
                      ((int)(g_pGraphics->resX * 0x41) / 640 - (int)(g_pGraphics->resX * 0x20) / 640) / 2 +
                          (int)(g_pGraphics->resX * 0x20) / 640,
                      (int)(g_pGraphics->resY * 8) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * i +
                          (int)(g_pGraphics->resY * 0x82) / 480 +
                          ((int)(g_pGraphics->resY * 0xa3) / 480 - (int)(g_pGraphics->resY * 0x82) / 480) / 2,
                      (int *)g_menuFrameColour, 0x12);
    }
    Font_SetBlendMode(2);
}

int Knockout_HasHumanLostMatch(KnockoutMatch *pMatch);
extern char g_keypadFormat[];

// Match of the arcade knockout table shown by the result screen.
// GLOBAL: CMR2 0x00541cd0
KnockoutMatch *g_pKnockoutMatch;

// Draw callback of the arcade knockout match result: who won, and whether
// the player goes through to the next round (or wins the final).
// FUNCTION: CMR2 0x004529c0
void GameMenu_DrawKnockoutMatchResult(Menu *pMenu)
{
    // PORT: the original prints the driver's name into four bytes of its
    // stack; the computer drivers' names ("A. DRIVER") run past them.
    char name[64];
    unsigned int *pState;
    int x;
    int y;

    x = (int)(g_pGraphics->resX * 30) / 640;
    y = (int)(g_pGraphics->resY * 242) / 480;
    pState = RallyData_GetChampionshipState();
    GameMenu_ClearMenuListWithHighlight();
    if (Knockout_HasHumanLostMatch(g_pKnockoutMatch)) {
        Font_DrawText(2, CFrontend::GetTextString(0x49), x, y, (int *)g_menuFrameColour, 0x11);
        if (g_pKnockoutMatch->time1 <= g_pKnockoutMatch->time2)
            snprintf(name, sizeof(name), "%s", (char *)RallyData_GetRecord(StageUI_GetRaceEndEventCount() + (g_pKnockoutMatch->flags & 0x1f)));
        else
            snprintf(name, sizeof(name), "%s", (char *)RallyData_GetRecord(StageUI_GetRaceEndEventCount() + ((g_pKnockoutMatch->flags >> 5) & 0x1f)));
        if ((*pState & 0x38) >= 0x20)
            sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x7d), name);
        else
            sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x7e), name);
        CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
        Font_DrawText(0, CFrontend::m_stringDest, x, (int)(g_pGraphics->resY * 5) / 480 + (y + Font_GetLineHeight(0)),
                      (int *)g_menuFrameColour, 0x11);
    } else {
        snprintf(name, sizeof(name), "%s", (char *)RallyData_GetRecord(StageUI_GetRaceEndEventCount() + (g_pKnockoutMatch->flags & 0x1f)));
        sprintf(CFrontend::m_stringDest, g_keypadFormat, CFrontend::GetTextString(0x88), name);
        Font_DrawText(2, CFrontend::m_stringDest, x, y, (int *)g_menuFrameColour, 0x11);
        if ((*pState & 0x38) < 0x20 && (*pState & 7) != 1)
            sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(0x91));
        else
            sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(0x92));
        CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
        Font_DrawText(0, CFrontend::m_stringDest, x, (int)(g_pGraphics->resY * 5) / 480 + (y + Font_GetLineHeight(0)),
                      (int *)g_menuFrameColour, 0x11);
    }
}

int Race_ReadPlayerState37F68(int index);
int Race_ReadPlayerState37F78(int index);
int Race_ReadPlayerState37F0C(int index);

// Draw callback of the stage penalties screen: for every car, its
// disqualification or retirement reason, jump start / speeding penalties
// and car damage notes, one line each.
// FUNCTION: CMR2 0x00452be0
void GameMenu_DrawStagePenalties(Menu *pMenu)
{
    int x;
    int y;
    short lines;
    int i;

    x = (int)(g_pGraphics->resX * 30) / 640;
    y = (int)(g_pGraphics->resY * 242) / 480;
    lines = 0;
    GameMenu_ClearMenuListWithHighlight();
    Font_DrawText(2, CFrontend::GetTextString(0x49), x, y, (int *)g_menuFrameColour, 0x11);
    for (i = 0; i < (int)(RallyDataState() & 0xff); i++) {
        if (Race_ReadPlayerState37F68(i) & 0x80) {
            sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x7f), CFrontend::GetModeSpecificCountryText(Race_ReadPlayerState37F78(i)));
            CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
            Font_DrawText(0, CFrontend::m_stringDest, x,
                          (int)(g_pGraphics->resY * 5) / 480 + Font_GetLineHeight(0) + y +
                              ((int)(g_pGraphics->resY * 10) / 480 + Font_GetLineHeight(0)) * i,
                          (int *)g_menuFrameColour, 0x11);
            lines++;
        } else if (Race_ReadPlayerState37F68(i) & 0x2000) {
            sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0xad),
                    CFrontend::GetTextString(Race_ReadPlayerState37F0C(i) + 0xad));
            CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
            Font_DrawText(0, CFrontend::m_stringDest, x,
                          (int)(g_pGraphics->resY * 5) / 480 + Font_GetLineHeight(0) + y +
                              ((int)(g_pGraphics->resY * 10) / 480 + Font_GetLineHeight(0)) * i,
                          (int *)g_menuFrameColour, 0x11);
            lines++;
        }
        if (Race_ReadPlayerState37F68(i) & 0x1800) {
            sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0xbf),
                    (Race_ReadPlayerState37F68(i) & 0x800) ? CFrontend::GetTextString(5) : CFrontend::GetTextString(7));
            CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
            Font_DrawText(0, CFrontend::m_stringDest, x,
                          (int)(g_pGraphics->resY * 5) / 480 + Font_GetLineHeight(0) + y +
                              ((int)(g_pGraphics->resY * 10) / 480 + Font_GetLineHeight(0)) * lines +
                              ((int)(g_pGraphics->resY * 10) / 480 + Font_GetLineHeight(0)) * i,
                          (int *)g_menuFrameColour, 0x11);
            lines++;
        }
        if (Race_ReadPlayerState37F68(i) & 0x300) {
            sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(0xc1));
            CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
            Font_DrawText(0, CFrontend::m_stringDest, x,
                          (int)(g_pGraphics->resY * 5) / 480 + Font_GetLineHeight(0) + y +
                              ((int)(g_pGraphics->resY * 10) / 480 + Font_GetLineHeight(0)) * lines +
                              ((int)(g_pGraphics->resY * 10) / 480 + Font_GetLineHeight(0)) * i,
                          (int *)g_menuFrameColour, 0x11);
            lines++;
        }
        if (Race_ReadPlayerState37F68(i) & 0x400) {
            sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(0xc0));
            CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
            Font_DrawText(0, CFrontend::m_stringDest, x,
                          (int)(g_pGraphics->resY * 5) / 480 + Font_GetLineHeight(0) + y +
                              ((int)(g_pGraphics->resY * 10) / 480 + Font_GetLineHeight(0)) * lines +
                              ((int)(g_pGraphics->resY * 10) / 480 + Font_GetLineHeight(0)) * i,
                          (int *)g_menuFrameColour, 0x11);
            lines++;
        }
    }
}

int RallyTiming_GetChampionshipTieBreak(int index);
int RallyTiming_GetChampionshipDriverAtPosition(int index);
int RallyTiming_GetChampionshipTimeSeconds(int index);

// Draw callback of the rally points table: every car in rally order with its
// car, points, tie-break note (count-back) and position ("=" for a tie).
// FUNCTION: CMR2 0x004530e0
void GameMenu_DrawRallyPointsTable(Menu *pMenu)
{
    bool isPlayer;
    bool tied;
    int i;
    int j;
    int id;

    GameMenu_ClearMenuListWithHighlight();
    GameMenus_DrawTextRow((int)(g_pGraphics->resX * 30) / 640, (int)(g_pGraphics->resY * 0x43) / 480,
                          CInput::FormatString(CFrontend::GetTextString(0x43), 0x62 - (RallyData_GetSelectionBits10To11() & 0xff)),
                          CFrontend::GetTextString(0x42), CFrontend::GetTextString(0x8d), (char *)NULL);
    for (i = 0; i < (BYTE)Race_GetBaseCarCount(); i++)
        g_stagePositionSeconds[i] = (BYTE)RallyTiming_GetChampionshipTimeSeconds(RallyTiming_GetChampionshipDriverAtPosition(i));
    for (i = 0; i < (BYTE)Race_GetBaseCarCount(); i++) {
        isPlayer = FALSE;
        tied = FALSE;
        id = RallyTiming_GetChampionshipDriverAtPosition(i);
        if (id < (int)(RallyDataState() & 0xff)) {
            isPlayer = TRUE;
            strcpy(CFrontend::m_stringDest, (char *)RallyData_GetRecord(id));
        } else {
            strcpy(CFrontend::m_stringDest, CAIHelper::GetNameForID(id));
        }
        Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x5c) / 640,
                      ((int)(g_pGraphics->resY * 0x82) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * i) -
                          (int)(g_pGraphics->resY * 8) / 480,
                      (int *)g_menuFrameColour, 9);
        sprintf(CFrontend::m_stringDest, g_stageNumberFormat, RallyTiming_GetChampionshipTimeSeconds(id));
        Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x21e) / 640,
                      ((int)(g_pGraphics->resY * 0x82) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * i) -
                          (int)(g_pGraphics->resY * 8) / 480,
                      (int *)g_menuFrameColour, 9);
        if (g_stagePositionSeconds[i] != 0) {
            for (j = 0; j < (BYTE)Race_GetBaseCarCount(); j++) {
                if (j != i && g_stagePositionSeconds[i] == g_stagePositionSeconds[j]) {
                    if (RallyTiming_GetChampionshipTieBreak(id) == 1)
                        sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0xb8), RallyTiming_GetChampionshipTieBreak(id));
                    else
                        sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0xb6), RallyTiming_GetChampionshipTieBreak(id));
                    Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x1fe) / 640,
                                  ((int)(g_pGraphics->resY * 0x82) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * i) -
                                      (int)(g_pGraphics->resY * 8) / 480,
                                  (int *)g_menuFrameColour, 0xc);
                    if (i > 0 && RallyTiming_GetChampionshipTieBreak(RallyTiming_GetChampionshipDriverAtPosition(i - 1)) == RallyTiming_GetChampionshipTieBreak(id) &&
                        g_stagePositionSeconds[i - 1] == g_stagePositionSeconds[i])
                        tied = TRUE;
                    break;
                }
            }
        }
        if (isPlayer) {
            strcpy(CFrontend::m_stringDest, (char *)CFrontend::GetModeSpecificCountryText(RallyData_GetDriverRecordSelectionValue(id)));
            CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
            g_menuRect[1] = (short)((int)(g_pGraphics->resY * 0x82) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * i - 1);
            g_menuRect[3] = (short)((int)(g_pGraphics->resY * 0xa3) / 480 - (int)(g_pGraphics->resY * 0x82) / 480 + 1);
            g_menuRect[0] = (short)((int)(g_pGraphics->resX * 0x20) / 640);
            g_menuRect[2] = (short)((int)(g_pGraphics->resX * 0x41) / 640 - (int)(g_pGraphics->resX * 0x20) / 640);
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_menuRect, g_menuRowFillColour, 2);
        } else {
            strcpy(CFrontend::m_stringDest, (char *)CFrontend::GetModeSpecificCountryText(RallyData_GetDriverSelectGridSlot(id)));
            CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
            GameMenus_DrawRowFrame(i, 0, 0);
        }
        Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x5c) / 640,
                      (int)(g_pGraphics->resY * 0xa3) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * i,
                      (int *)g_menuFrameColour, 0x21);
        if (tied)
            sprintf(CFrontend::m_stringDest, "%s", g_stageResultSameTime);
        else
            sprintf(CFrontend::m_stringDest, g_stageNumberFormat, i + 1);
        Font_DrawText(1, CFrontend::m_stringDest,
                      ((int)(g_pGraphics->resX * 0x41) / 640 - (int)(g_pGraphics->resX * 0x20) / 640) / 2 +
                          (int)(g_pGraphics->resX * 0x20) / 640,
                      (int)(g_pGraphics->resY * 8) / 480 + ((int)(g_pGraphics->resY * 0x34) / 480) * i +
                          (int)(g_pGraphics->resY * 0x82) / 480 +
                          ((int)(g_pGraphics->resY * 0xa3) / 480 - (int)(g_pGraphics->resY * 0x82) / 480) / 2,
                      (int *)g_menuFrameColour, 0x12);
    }
    Font_SetBlendMode(2);
}

int RallyTiming_GetChampionshipPointsByPosition(int index);
unsigned int RallyData_GetSelectionBits10To11(void);

// Draw callback of the rally results screen: "stage N of the rally" with the
// rally name between two separator bars (before the first driver in the top
// three), then one line per driver with its rally position.
// FUNCTION: CMR2 0x00453830
void GameMenu_DrawRallyResultsBanner(Menu *pMenu)
{
    bool shown;
    int x;
    int place;
    int i;
    int resY;
    int lineHeight;
    int *pResY;

    shown = FALSE;
    x = (int)(g_pGraphics->resX * 30) / 640;
    GameMenu_ClearMenuListWithHighlight();
    sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x43), 0x62 - (RallyData_GetSelectionBits10To11() & 0xff));
    Font_DrawText(2, CFrontend::m_stringDest, x, (int)(g_pGraphics->resY * 242) / 480, (int *)g_menuFrameColour, 0x11);
    x += Font_GetTextWidth(2, (BYTE *)CFrontend::m_stringDest);
    for (i = 0; i < (BYTE)RallyDataState(); i++) {
        place = RallyTiming_GetChampionshipPointsByPosition(i);
        if (!shown && place < 3) {
            x += (int)(g_pGraphics->resX * 8) / 640;
            pResY = &g_pGraphics->resY;
            g_menuRect[0] = (short)x;
            resY = *pResY;
            lineHeight = Font_GetLineHeight(2);
            g_menuRect[1] = (short)(resY * 242 / 480 + resY * 4 / 480 - lineHeight);
            g_menuRect[2] = 2;
            g_menuRect[3] = (short)((int)(*pResY * 41) / 480);
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_menuRect, g_menuFrameColour, 2);
            x += (int)(g_pGraphics->resX * 8) / 640;
            strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(CGameInfo::GetConfiguredDifficulty() + 0x8e));
            Font_DrawText(2, CFrontend::m_stringDest, x, (int)(g_pGraphics->resY * 242) / 480,
                          (int *)g_menuFrameColour, 0x11);
            x += Font_GetTextWidth(2, (BYTE *)CFrontend::m_stringDest);
            x += (int)(g_pGraphics->resX * 8) / 640;
            g_menuRect[0] = (short)x;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_menuRect, g_menuFrameColour, 2);
            x += (int)(g_pGraphics->resX * 8) / 640;
            Font_DrawText(2, CFrontend::GetTextString(0x49), x, (int)(g_pGraphics->resY * 242) / 480,
                          (int *)g_menuFrameColour, 0x11);
            shown = TRUE;
        }
        sprintf(CFrontend::m_stringDest, g_nameSpaceFormat, (char *)RallyData_GetRecord(i));
        switch (place) {
        case 0:
            sprintf(CFrontend::m_stringDest + strlen(CFrontend::m_stringDest), "%s", CFrontend::GetTextString(0x51));
            break;
        case 1:
            sprintf(CFrontend::m_stringDest + strlen(CFrontend::m_stringDest), "%s", CFrontend::GetTextString(0x52));
            break;
        case 2:
            sprintf(CFrontend::m_stringDest + strlen(CFrontend::m_stringDest), "%s", CFrontend::GetTextString(0x53));
            break;
        default:
            sprintf(CFrontend::m_stringDest + strlen(CFrontend::m_stringDest), CFrontend::GetTextString(0x54), place + 1);
            break;
        }
        CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
        Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 30) / 640,
                      (int)(g_pGraphics->resY * 242) / 480 +
                          ((int)(g_pGraphics->resY * 10) / 480 + Font_GetLineHeight(0)) * (i + 1),
                      (int *)g_menuFrameColour, 0x11);
    }
    Font_SetBlendMode(2);
}

// Builds the in-game menu tree (pause / results / options screens).
// FUNCTION: CMR2 0x00449100
void GameMenus_Build(void)
{
    g_menuBuildTime = Sys_GetTicks();
    Menu_Init(&g_menu0x0053ea68, 0, 0, 0, NULL, NULL, 1, 0, 0);
    Menu_AddItemType2(&g_menu0x0053ea68, 0, 0, &g_menu0x00541218, NULL, 0);
    Menu_SetCallbacks(&g_menu0x0053ea68, NULL, NULL, (MenuCallback)GameMenu_DrawPauseHeader, NULL);
    Menu_ValidateCursor(&g_menu0x0053ea68, 0);
    Menu_Init(&g_menu0x0053ff38, 0, 0, 0, NULL, NULL, 1, 0, 0);
    Menu_AddItemType2(&g_menu0x0053ff38, 0, 0, &g_menu0x0053ea68, NULL, 0);
    Menu_SetCallbacks(&g_menu0x0053ff38, NULL, NULL, (MenuCallback)GameMenu_DrawWaitingForPlayers, NULL);
    Menu_ValidateCursor(&g_menu0x0053ff38, 0);
    Menu_Init(&g_menu0x00541218, 0, 0, 0, &g_menu0x0053ea68, NULL, 1, 0, 0);
    Menu_AddItemType2(&g_menu0x00541218, 0, 0, &g_menu0x0053f5b0, NULL, 0);
    Menu_SetCallbacks(&g_menu0x00541218, NULL, NULL, (MenuCallback)GameMenu_DrawSplitTimes, NULL);
    Menu_ValidateCursor(&g_menu0x00541218, 0);
    Menu_Init(&g_menu0x0053f3c8, 0, 0, 0, &g_menu0x0053ea68, NULL, 1, 0, 0);
    Menu_AddItemType2(&g_menu0x0053f3c8, 0, 0, &g_menu0x00541400, NULL, 0);
    Menu_SetCallbacks(&g_menu0x0053f3c8, NULL, NULL, (MenuCallback)GameMenu_DrawStageResultsRows, NULL);
    Menu_ValidateCursor(&g_menu0x0053f3c8, 0);
    Menu_Init(&g_menu0x0053f5b0, 0, 0, 0, &g_menu0x00541218, NULL, 1, 0, 0);
    Menu_AddItemType2(&g_menu0x0053f5b0, 0, 0, &g_menu0x0053e4b8, NULL, 0);
    Menu_SetCallbacks(&g_menu0x0053f5b0, (MenuCallback)GameMenu_UpdateStandingsScrollRange, NULL, (MenuCallback)GameMenu_DrawStageWinner, NULL);
    Menu_ValidateCursor(&g_menu0x0053f5b0, 0);
    Menu_Init(&g_menu0x0053e4b8, 0, 0, 0, NULL, NULL, 1, 0, 0);
    Menu_AddItemType3(&g_menu0x0053e4b8, 0, 0, 0xb, 0, 0, 0, (MenuItemAction)GameMenu_OpenFirstItemSubmenu, 0);
    Menu_SetCallbacks(&g_menu0x0053e4b8, (MenuCallback)GameMenu_UpdateStandingsScrollRange, NULL, (MenuCallback)GameMenu_DrawScrollingStageSplits, NULL);
    Menu_ValidateCursor(&g_menu0x0053e4b8, 0);
    Menu_Init(&g_menu0x00540c68, 0, 0, 0, NULL, NULL, 1, 0, 0);
    Menu_AddItemType3(&g_menu0x00540c68, 0, 0, 8, 0, 0, 0, (MenuItemAction)GameMenu_OpenFirstItemSubmenu, 0);
    Menu_SetCallbacks(&g_menu0x00540c68, (MenuCallback)GameMenu_UpdateRemainingRaceRange, NULL, (MenuCallback)GameMenu_DrawStageTimeStandings, NULL);
    Menu_ValidateCursor(&g_menu0x00540c68, 0);
    Menu_Init(&g_menu0x005406b8, 0, 0, 0, NULL, NULL, 1, 0, 0);
    Menu_AddItemType2(&g_menu0x005406b8, 0, 0, NULL, NULL, 0);
    Menu_SetCallbacks(&g_menu0x005406b8, NULL, NULL, (MenuCallback)GameMenu_DrawChampionshipClassResults, NULL);
    Menu_ValidateCursor(&g_menu0x005406b8, 0);
    Menu_Init(&g_menu0x0053f008, 0, 0, 0, &g_menu0x0053e4b8, NULL, 1, 0, 0);
    Menu_AddItemType3(&g_menu0x0053f008, 0, 0, 0xb, 0, 0, 0, (MenuItemAction)GameMenu_OpenFirstItemSubmenu, 0);
    Menu_SetCallbacks(&g_menu0x0053f008, (MenuCallback)GameMenu_UpdateStandingsScrollRange, NULL, (MenuCallback)GameMenu_DrawScrollingRallyTimes, NULL);
    Menu_ValidateCursor(&g_menu0x0053f008, 0);
    Menu_Init(&g_menu0x0053fd58, 0, 0, 0, &g_menu0x00540c68, NULL, 1, 0, 0);
    Menu_AddItemType3(&g_menu0x0053fd58, 0, 0, 0xb, 0, 0, 0, (MenuItemAction)GameMenu_OpenFirstItemSubmenu, 0);
    Menu_SetCallbacks(&g_menu0x0053fd58, (MenuCallback)GameMenu_UpdateRemainingRaceRange, NULL, (MenuCallback)GameMenu_DrawChampionshipTimeStandings, NULL);
    Menu_ValidateCursor(&g_menu0x0053fd58, 0);
    Menu_Init(&g_menu0x0053e6a0, 0, 0, 0, &g_menu0x0053f008, NULL, 1, 0, 0);
    Menu_AddItemType3(&g_menu0x0053e6a0, 0, 0, 0xb, 0, 0, 0, (MenuItemAction)GameMenu_OpenFirstItemSubmenu, 0);
    Menu_SetCallbacks(&g_menu0x0053e6a0, (MenuCallback)GameMenu_UpdateStandingsScrollRange, NULL, (MenuCallback)GameMenu_DrawScrollingChampionshipPoints, NULL);
    Menu_ValidateCursor(&g_menu0x0053e6a0, 0);
    Menu_Init(&g_menu0x00540118, 0, 0, 0, &g_menu0x0053e6a0, NULL, 1, 0, 0);
    Menu_AddItemType2(&g_menu0x00540118, 0, 0, NULL, NULL, 0);
    Menu_SetCallbacks(&g_menu0x00540118, NULL, NULL, (MenuCallback)GameMenu_DrawChampionshipOutcome, NULL);
    Menu_ValidateCursor(&g_menu0x00540118, 0);
    Menu_Init(&g_menu0x00540a80, 0, 0, 0, &g_menu0x00541030, NULL, 1, 0, 0);
    Menu_AddItemType2(&g_menu0x00540a80, 0, 0, NULL, NULL, 0);
    Menu_SetCallbacks(&g_menu0x00540a80, NULL, NULL, (MenuCallback)GameMenu_NoOpNetworkRallyEndDraw, NULL);
    Menu_ValidateCursor(&g_menu0x00540a80, 0);
    Menu_Init(&g_menu0x00541030, 0, 0, 0, &g_menu0x0053fd58, NULL, 1, 0, 0);
    Menu_AddItemType2(&g_menu0x00541030, 0, 0, NULL, NULL, 0);
    Menu_SetCallbacks(&g_menu0x00541030, NULL, NULL, (MenuCallback)GameMenu_DrawStageStartList, NULL);
    Menu_ValidateCursor(&g_menu0x00541030, 0);
    Menu_Init(&g_menu0x00541ae0, 0, 0, 0, &g_menu0x00540118, NULL, 1, 0, 0);
    Menu_AddItemType3(&g_menu0x00541ae0, 0, 0, 0xb, 0, 0, 0, (MenuItemAction)GameMenu_OpenFirstItemSubmenu, 0);
    Menu_SetCallbacks(&g_menu0x00541ae0, (MenuCallback)GameMenu_UpdateStandingsScrollRange, NULL, (MenuCallback)GameMenu_DrawScrollingStagePoints, NULL);
    Menu_ValidateCursor(&g_menu0x00541ae0, 0);
    Menu_Init(&g_menu0x0053f790, 0, 0, 0, &g_menu0x00541ae0, NULL, 1, 0, 0);
    Menu_AddItemType2(&g_menu0x0053f790, 0, 0, &g_menu0x0053f970, NULL, 0);
    Menu_SetCallbacks(&g_menu0x0053f790, NULL, NULL, (MenuCallback)GameMenu_DrawFinalChampionshipStandings, NULL);
    Menu_ValidateCursor(&g_menu0x0053f790, 0);
    Menu_Init(&g_menu0x0053f970, 0, 0, 0, &g_menu0x0053f790, NULL, 1, 0, 0);
    Menu_AddItemType2(&g_menu0x0053f970, 0, 0, NULL, (MenuItemAction)GameMenu_RequestResultsContinue, 0);
    Menu_SetCallbacks(&g_menu0x0053f970, (MenuCallback)GameMenu_ContinueWithDefaultMode, NULL, (MenuCallback)GameMenu_DrawFinalChampionshipStandings, NULL);
    Menu_ValidateCursor(&g_menu0x0053f970, 0);
    Menu_Init(&g_menu0x0053fb70, 0, 0, 0, NULL, NULL, 1, 0, 0);
    Menu_AddItemType2(&g_menu0x0053fb70, 0, 0, NULL, NULL, 0);
    Menu_SetCallbacks(&g_menu0x0053fb70, NULL, NULL, (MenuCallback)GameMenu_DrawStageClassification, NULL);
    Menu_ValidateCursor(&g_menu0x0053fb70, 0);
    Menu_Init(&g_menu0x0053e2d8, 0, 0, 0, NULL, NULL, 1, 0, 0);
    Menu_AddItemType2(&g_menu0x0053e2d8, 0, 0, NULL, NULL, 0);
    Menu_SetCallbacks(&g_menu0x0053e2d8, NULL, NULL, (MenuCallback)GameMenu_DrawChampionshipDriverStandings, NULL);
    Menu_ValidateCursor(&g_menu0x0053e2d8, 0);
    Menu_Init(&g_menu0x0053ec48, 0, 0, 0, &g_menu0x0053fb70, NULL, 1, 0, 0);
    Menu_AddItemType2(&g_menu0x0053ec48, 0, 0, NULL, NULL, 0);
    Menu_SetCallbacks(&g_menu0x0053ec48, NULL, NULL, (MenuCallback)GameMenu_DrawStageClassification, NULL);
    Menu_ValidateCursor(&g_menu0x0053ec48, 0);
    Menu_Init(&g_menu0x0053e888, 0, 0, 0, &g_menu0x0053ec48, NULL, 1, 0, 0);
    Menu_AddItemType2(&g_menu0x0053e888, 0, 0, NULL, NULL, 0);
    Menu_SetCallbacks(&g_menu0x0053e888, NULL, NULL, (MenuCallback)GameMenu_DrawRallyPointsTable, NULL);
    Menu_ValidateCursor(&g_menu0x0053e888, 0);
    Menu_Init(&g_menu0x0053ee28, 0, 0, 0, &g_menu0x0053e888, NULL, 1, 0, 0);
    Menu_AddItemType2(&g_menu0x0053ee28, 0, 0, &g_menu0x00541400, NULL, 0);
    Menu_SetCallbacks(&g_menu0x0053ee28, NULL, NULL, (MenuCallback)GameMenu_DrawRallyResultsBanner, NULL);
    Menu_ValidateCursor(&g_menu0x0053ee28, 0);
    if (CGameInfo::GetConfiguredGameMode() == 4) {
    Menu_Init(&g_menu0x0053f1e8, 0, 0, 0, &g_menu0x00541218, NULL, 1, 0, 0);
    Menu_AddItemType2(&g_menu0x0053f1e8, 0, 0, &g_menu0x00541400, NULL, 0);
    Menu_SetCallbacks(&g_menu0x0053f1e8, NULL, NULL, (MenuCallback)GameMenu_DrawKnockoutMatchResult, NULL);
    Menu_ValidateCursor(&g_menu0x0053f1e8, 0);
    }
    Menu_Init(&g_menu0x00540e50, 0, 0, 0, NULL, NULL, 1, 0, 0);
    Menu_AddItemType2(&g_menu0x00540e50, 0, 0, &g_menu0x00541400, NULL, 0);
    Menu_SetCallbacks(&g_menu0x00540e50, NULL, NULL, (MenuCallback)GameMenu_DrawStagePenalties, NULL);
    Menu_ValidateCursor(&g_menu0x00540e50, 0);
    if (CGameInfo::GetGameModeOptionBit19() != 0)
        Menu_Init(&g_menu0x005402f8, 0, 0, 0, &g_menu0x005416e0, NULL, 1, 0, 1);
    else
        Menu_Init(&g_menu0x005402f8, 0, 0, 0, &g_menu0x00541400, NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x005402f8, 0, -1, NULL, (MenuItemAction)GameMenu_RequestResultsRestart, 1000);
    if (CGameInfo::GetConfiguredGameMode() == 2 || CGameInfo::GetConfiguredGameMode() == 3)
        Menu_AddItemType2(&g_menu0x005402f8, 0, 0x8b, NULL, (MenuItemAction)GameMenu_RequestResultsRetry, 0x3e9);
    Menu_AddItemType1(&g_menu0x005402f8, 0, 0xa6, 0, -1);
    Menu_SetCallbacks(&g_menu0x005402f8, (MenuCallback)GameMenu_SelectLastItem, NULL, (MenuCallback)GameMenu_DrawStandingsMenuItems, NULL);
    Menu_ValidateCursor(&g_menu0x005402f8, 0);
    if (CGameInfo::GetGameModeOptionBit19() != 0)
        Menu_Init(&g_menu0x005418d8, 0, 0, 0, &g_menu0x005416e0, NULL, 1, 0, 1);
    else
        Menu_Init(&g_menu0x005418d8, 0, 0, 0, &g_menu0x00541400, NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x005418d8, 0, 0xa8, NULL, (MenuItemAction)GameMenu_RequestResultsQuit, 1000);
    Menu_AddItemType1(&g_menu0x005418d8, 0, 0xaa, 0, -1);
    Menu_SetCallbacks(&g_menu0x005418d8, (MenuCallback)GameMenu_SelectLastItem, NULL, (MenuCallback)GameMenu_DrawStandingsMenuItems, NULL);
    Menu_ValidateCursor(&g_menu0x005418d8, 0);
    Menu_Init(&g_menu0x005408a0, 0, 0, 0, &g_menu0x005416e0, NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x005408a0, 0, 0xf6, NULL, (MenuItemAction)GameMenu_RequestNetworkResultsQuit, 1000);
    Menu_AddItemType1(&g_menu0x005408a0, 0, 0xaa, 0, -1);
    Menu_SetCallbacks(&g_menu0x005408a0, (MenuCallback)GameMenu_SelectLastItem, NULL, (MenuCallback)GameMenu_DrawStandingsMenuItems, NULL);
    Menu_ValidateCursor(&g_menu0x005408a0, 0);
    Menu_Init(&g_menu0x005404d8, 0, 0, 0, NULL, NULL, 1, 0, 0);
    Menu_SetCallbacks(&g_menu0x005404d8, (MenuCallback)GameMenu_LeaveStageAndResetMenuFlag, NULL, (MenuCallback)GameMenu_DrawStandingsMenuItems, NULL);
    Menu_ValidateCursor(&g_menu0x005404d8, 0);
}


int NetPlayers_GetClassificationCount(void);
NetClassification *NetPlayers_GetClassificationRecord(int index);
unsigned int NetPlayers_GetClassificationCarClass(int index);
unsigned int NetPlayers_GetClassificationTime(int index);
void GameMenus_DrawTextRow(int x, int y, char *pText, ...);
void GameMenus_DrawRowFrame(short row, short yOffset, char compact);
void GameMenu_DrawInGameBreadcrumb(Menu *pMenu);
char *NetPlayers_GetStandingName(int index, int total);
int NetPlayers_GetStandingPoints(int index);
int NetPlayers_GetStandingPlayerIndex(int index, int total);
unsigned int NetPlayers_GetStandingCar(int index, int total);
int NetPlayers_GetPlayerRank(int index, int total);
unsigned int NetPlayers_GetStandingTime(int index, int total);
unsigned int RallyData_GetSelectionBits10To11(void);
unsigned int RallyData_GetSelectionBits12To13(void);
void GameMenu_DrawInGameBreadcrumb(Menu *pMenu);
void GameMenus_FormatModeName(char *unused, int mode);
Texture *InRaceMenu_GetUpArrowTexture(void);
Texture *InRaceMenu_GetDownArrowTexture(void);
BYTE StageUI_GetRaceEndEventCount(void);

// GLOBAL: CMR2 0x00541cc0
short g_menuRect[4];
// GLOBAL: CMR2 0x00519ecc
BYTE g_menuFrameColour[4] = { 250, 250, 250, 255 };
// GLOBAL: CMR2 0x00540c60
int g_gameMenuScrollY;
// GLOBAL: CMR2 0x0053e698
int g_gameMenuRowYOffset;

// GLOBAL: CMR2 0x00516098
char g_classRowHeaderFormat[] = "%s %d";
// GLOBAL: CMR2 0x00516e30
char g_noTimeText[] = "--:--.--";

// Championship classification screen: header row (country plus either the
// special-stage name or "round N") and one row per class with the class name,
// its tag (from the three text ids) and the best time of the class.
// FUNCTION: CMR2 0x0044e830
void GameMenu_DrawChampionshipClassResults(Menu *pMenu)
{
    char *texts[4];
    int i;
    char *pClass;

    texts[0] = CMain::m_logFileBlankLine;
    texts[1] = CFrontend::GetTextString(0xff);
    texts[2] = CFrontend::GetTextString(0x100);
    texts[3] = CFrontend::GetTextString(0x101);
    GameMenu_ClearMenuListWithHighlight();
    if (RallyDataStageIndex() == 0xa) {
        GameMenus_DrawTextRow((int)(g_pGraphics->resX * 0x1e) / 0x280, (int)(g_pGraphics->resY * 0x43) / 0x1e0,
                              CFrontend::GetTextString((unsigned char)RallyDataCountryIndex()),
                              CFrontend::GetTextString(0xba), (char *)NULL);
    } else {
        GameMenus_DrawTextRow((int)(g_pGraphics->resX * 0x1e) / 0x280, (int)(g_pGraphics->resY * 0x43) / 0x1e0,
                              CFrontend::GetTextString((unsigned char)RallyDataCountryIndex()),
                              CInput::FormatString(g_classRowHeaderFormat, CFrontend::GetTextString(0x40),
                                                   RallyDataStageIndex() + 1), (char *)NULL);
    }
    i = 0;
    if (NetPlayers_GetClassificationCount() > 0) {
        do {
            Font_DrawText(1, NetPlayers_GetClassificationRecord(i)->name, (int)(g_pGraphics->resX * 0x5c) / 0x280, (int)(g_pGraphics->resY * 3) / 0x1e0 +
                          ((((int)(g_pGraphics->resY * 0x82) / 0x1e0 + ((int)(g_pGraphics->resY * 0x2d) / 0x1e0) * i) -
                            (int)(g_pGraphics->resY * 0x14) / 0x1e0) - (int)(g_pGraphics->resY * 8) / 0x1e0),
                          (int *)g_menuFrameColour, 9);
            Font_DrawText(1, texts[NetPlayers_GetClassificationCarClass(i)], (int)(g_pGraphics->resX * 0x5c) / 0x280 + (int)(g_pGraphics->resX * 100) / 0x280, (int)(g_pGraphics->resY * 3) / 0x1e0 +
                          ((((int)(g_pGraphics->resY * 0x82) / 0x1e0 + ((int)(g_pGraphics->resY * 0x2d) / 0x1e0) * i) -
                            (int)(g_pGraphics->resY * 0x14) / 0x1e0) - (int)(g_pGraphics->resY * 8) / 0x1e0),
                          (int *)g_menuFrameColour, 9);
            if ((int)NetPlayers_GetClassificationTime(i) == -1) {
                sprintf(CFrontend::m_stringDest, "%s", g_noTimeText);
            } else {
                FormatCentisecondsAsMinSecMSec(NetPlayers_GetClassificationTime(i), CFrontend::m_stringDest);
            }
            pClass = CFrontend::m_stringDest;
            Font_DrawText(1, pClass, (int)(g_pGraphics->resX * 0x5c) / 0x280 + (int)(g_pGraphics->resX * 0xfa) / 0x280, (int)(g_pGraphics->resY * 3) / 0x1e0 +
                          ((((int)(g_pGraphics->resY * 0x82) / 0x1e0 + ((int)(g_pGraphics->resY * 0x2d) / 0x1e0) * i) -
                            (int)(g_pGraphics->resY * 0x14) / 0x1e0) - (int)(g_pGraphics->resY * 8) / 0x1e0),
                          (int *)g_menuFrameColour, 9);
            i++;
        } while (i < NetPlayers_GetClassificationCount());
    }
    Font_SetBlendMode(2);
}

// GLOBAL: CMR2 0x00517dd8
char g_standingsRowFormat[] = "%s - %s";
// GLOBAL: CMR2 0x00519ed0
BYTE g_menuTextColour[4] = { 0xa7, 0xac, 0xdb, 0xff };

// Draws one row of the standings list: the item text (with the driver record
// appended when the item is the "go to" one) plus the small tag that follows
// it, and the row background sprite. The highlighted row uses the frame
// colour and the first background texture, the rest the text colour and the
// second one.
#define GAMEMENUS_DRAW_STANDINGS_ROW(pColour, pTexture)                                          \
    if (pItem->value == 0x3ea) {                                                                 \
        sprintf(CFrontend::m_stringDest, g_standingsRowFormat,                                   \
                CFrontend::GetTextString(pItem->id),                                             \
                (char *)RallyData_GetRecord((BYTE)(StageUI_GetRaceEndEventCount() + 1)));                        \
        Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x86) / 0x280,       \
                      (int)(g_pGraphics->resY * 0xde) / 0x1e0 +                                  \
                          ((int)(g_pGraphics->resY * 0x36) / 0x1e0) * i,                         \
                      (int *)pColour, 0x11);                                                     \
    } else {                                                                                     \
        Font_DrawText(1, CFrontend::GetTextString(pItem->id),                                    \
                      (int)(g_pGraphics->resX * 0x86) / 0x280,                                   \
                      (int)(g_pGraphics->resY * 0xde) / 0x1e0 +                                  \
                          ((int)(g_pGraphics->resY * 0x36) / 0x1e0) * i,                         \
                      (int *)pColour, 0x11);                                                     \
    }                                                                                            \
    Font_DrawText(0, CFrontend::GetTextString(pItem->id + 1),                                    \
                  (int)(g_pGraphics->resX * 0x86) / 0x280,                                       \
                  (int)(g_pGraphics->resY * 0xf) / 0x1e0 +                                       \
                      (int)(g_pGraphics->resY * 0xde) / 0x1e0 +                                  \
                      ((int)(g_pGraphics->resY * 0x36) / 0x1e0) * i,                             \
                  (int *)pColour, 0x11);                                                         \
    Sprite_Queue((SpriteRect *)&pTexture()->field_0x11c, (SpriteRect *)rect, pTexture(), 2, 0, NULL,    \
                 NULL, pColour, 8);

// GLOBAL: CMR2 0x00517dd0
char g_stageNumberFormat[] = "%d";
// GLOBAL: CMR2 0x00517dd4
char g_nameSpaceFormat[] = "%s ";
// GLOBAL: CMR2 0x00541cdc
int g_stageResultTimes[4];
// GLOBAL: CMR2 0x00541cec
BYTE g_stageResultRecords[4];

// Stage results screen: header (country plus either the special-stage name or
// "round N") and one row per record - driver, class tag, time and position -
// with a frame around the row. The stage type (1..3) shifts the rows down.
// FUNCTION: CMR2 0x0044d260
void GameMenu_DrawStageResultsRows(Menu *pMenu)
{
    int offset;
    int i;
    int *pTime;

    GameMenu_ClearMenuListWithHighlight();
    if (RallyDataStageIndex() == 0xa) {
        GameMenus_DrawTextRow((int)(g_pGraphics->resX * 0x1e) / 0x280,
                              (int)(g_pGraphics->resY * 0x43) / 0x1e0,
                              CFrontend::GetTextString((unsigned char)RallyDataCountryIndex()),
                              CFrontend::GetTextString(0xba), CFrontend::GetTextString(0x9a), (char *)NULL);
    } else {
        GameMenus_DrawTextRow((int)(g_pGraphics->resX * 0x1e) / 0x280,
                              (int)(g_pGraphics->resY * 0x43) / 0x1e0,
                              CFrontend::GetTextString((unsigned char)RallyDataCountryIndex()),
                              CInput::FormatString(g_classRowHeaderFormat, CFrontend::GetTextString(0x40),
                                                   RallyDataStageIndex() + 1),
                              CFrontend::GetTextString(0x9a), (char *)NULL);
    }
    switch (StageUI_GetRaceEndEventCount()) {
    case 1:
        offset = (int)(g_pGraphics->resY * 0x50) / 0x1e0;
        break;
    case 2:
        offset = (int)(g_pGraphics->resY * 0x32) / 0x1e0;
        break;
    case 3:
        offset = (int)(g_pGraphics->resY * 0x14) / 0x1e0;
        break;
    }
    i = 0;
    if (StageUI_GetRaceEndEventCount() + 1 > 0) {
        pTime = g_stageResultTimes;
        do {
            sprintf(CFrontend::m_stringDest, "%s", (char *)RallyData_GetRecord(g_stageResultRecords[i]));
            CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x5c) / 0x280,
                          ((int)(g_pGraphics->resY * 0x82) / 0x1e0 +
                           ((int)(g_pGraphics->resY * 0x34) / 0x1e0) * i) -
                              (int)(g_pGraphics->resY * 8) / 0x1e0 + offset,
                          (int *)g_menuFrameColour, 9);
            sprintf(CFrontend::m_stringDest, "%s",
                    (char *)CFrontend::GetModeSpecificCountryText(RallyData_GetDriverRecordSelectionValue(g_stageResultRecords[i])));
            CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
            Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x5c) / 0x280,
                          (int)(g_pGraphics->resY * 0xa3) / 0x1e0 + offset +
                              ((int)(g_pGraphics->resY * 0x34) / 0x1e0) * i,
                          (int *)g_menuFrameColour, 0x11);
            sprintf(CFrontend::m_stringDest, g_minSecMSECFormatString, *pTime / 6000,
                    (*pTime / 100) % 0x3c, *pTime % 100);
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x21e) / 0x280,
                          (int)(g_pGraphics->resY * 6) / 0x1e0 + offset +
                              (((int)(g_pGraphics->resY * 0x82) / 0x1e0 +
                                ((int)(g_pGraphics->resY * 0x34) / 0x1e0) * i) -
                               (int)(g_pGraphics->resY * 8) / 0x1e0),
                          (int *)g_menuFrameColour, 9);
            GameMenus_DrawRowFrame(i, offset, 0);
            sprintf(CFrontend::m_stringDest, g_stageNumberFormat, i + 1);
            Font_DrawText(1, CFrontend::m_stringDest,
                          ((int)(g_pGraphics->resX * 0x41) / 0x280 -
                           (int)(g_pGraphics->resX * 0x20) / 0x280) / 2 +
                              (int)(g_pGraphics->resX * 0x20) / 0x280,
                          (int)(g_pGraphics->resY * 8) / 0x1e0 +
                              ((int)(g_pGraphics->resY * 0x34) / 0x1e0) * i +
                              (int)(g_pGraphics->resY * 0x82) / 0x1e0 +
                              ((int)(g_pGraphics->resY * 0xa3) / 0x1e0 -
                               (int)(g_pGraphics->resY * 0x82) / 0x1e0) / 2 + offset,
                          (int *)g_menuFrameColour, 0x12);
            i++;
            pTime++;
        } while (i < StageUI_GetRaceEndEventCount() + 1);
    }
}

// Standings list of one menu: title, then one row per item.
// FUNCTION: CMR2 0x00453c50
void GameMenu_DrawStandingsMenuItems(Menu *pMenu)
{
    MenuItem *pItem;
    short rect[4];
    int x;
    int i;

    x = (int)(g_pGraphics->resX * 0x1e) / 0x280;
    GameMenu_ClearMenuListWithHighlight();
    Font_DrawText(2, CFrontend::GetTextString(0x56), x, (int)(g_pGraphics->resY * 0x43) / 0x1e0,
                  (int *)g_menuFrameColour, 0x11);
    rect[0] = (short)((int)(g_pGraphics->resX * 0x70) / 0x280);
    rect[2] = InRaceMenu_GetUpArrowTexture()->width;
    rect[3] = InRaceMenu_GetUpArrowTexture()->height;
    i = 0;
    pItem = pMenu->items;
    if (pMenu->itemCount > 0) {
        do {
            rect[1] = (short)(((int)(g_pGraphics->resY * 0xde) / 0x1e0 +
                               ((int)(g_pGraphics->resY * 0x36) / 0x1e0) * i) -
                              (int)(g_pGraphics->resY * 0xd) / 0x1e0);
            if (pMenu->cursor == i) {
                GAMEMENUS_DRAW_STANDINGS_ROW(g_menuFrameColour, InRaceMenu_GetUpArrowTexture)
            } else {
                GAMEMENUS_DRAW_STANDINGS_ROW(g_menuTextColour, InRaceMenu_GetDownArrowTexture)
            }
            i++;
            pItem++;
        } while (i < pMenu->itemCount);
    }
    Font_SetBlendMode(2);
}

// GLOBAL: CMR2 0x00519ed8
BYTE g_menuRowFillColour[4] = { 0xf6, 0x72, 0x28, 0xff };
// GLOBAL: CMR2 0x00519f70
char g_stageResultSameTime[] = "=";

// Stage start list: header (country plus the two stage texts) and one row per
// driver with its name, its number, the status (with the row boxed when the
// driver is not classified) and the time, which is only printed when it
// differs from the previous driver's.
// FUNCTION: CMR2 0x0044f8a0
void GameMenu_DrawStageStartList(Menu *pMenu)
{
    int x;
    int i;

    x = (int)(g_pGraphics->resX * 0x1e) / 0x280;
    GameMenu_ClearMenuListWithHighlight();
    GameMenus_DrawTextRow(x, (int)(g_pGraphics->resY * 0x43) / 0x1e0,
                          CFrontend::GetTextString((unsigned char)RallyDataCountryIndex()),
                          CFrontend::GetTextString(0x41), CFrontend::GetTextString(0x8d), (char *)NULL);
    i = 0;
    if (NetPlayers_GetStandingCount() > 0) {
        do {
            Font_DrawText(1, NetPlayers_GetStandingName(i, 1), (int)(g_pGraphics->resX * 0x5c) / 0x280,
                          (int)(g_pGraphics->resY * 3) / 0x1e0 +
                              (((int)(g_pGraphics->resY * 0x82) / 0x1e0 +
                                ((int)(g_pGraphics->resY * 0x2d) / 0x1e0) * i) -
                               (int)(g_pGraphics->resY * 0x14) / 0x1e0) -
                              (int)(g_pGraphics->resY * 8) / 0x1e0,
                          (int *)g_menuFrameColour, 9);
            sprintf(CFrontend::m_stringDest, g_stageNumberFormat, NetPlayers_GetStandingPoints(i));
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x21e) / 0x280,
                          (((int)(g_pGraphics->resY * 0x82) / 0x1e0 +
                            ((int)(g_pGraphics->resY * 0x2d) / 0x1e0) * i) -
                           (int)(g_pGraphics->resY * 0x14) / 0x1e0) -
                              (int)(g_pGraphics->resY * 8) / 0x1e0,
                          (int *)g_menuFrameColour, 9);
            if (NetPlayers_GetStandingPlayerIndex(i, 1) == -2) {
                strcpy(CFrontend::m_stringDest,
                       (char *)CFrontend::GetModeSpecificCountryText(NetPlayers_GetStandingCar(i, 1)));
                CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
                g_menuRect[1] = (short)((((int)(g_pGraphics->resY * 0x82) / 0x1e0 +
                                          ((int)(g_pGraphics->resY * 0x2d) / 0x1e0) * i) -
                                         (int)(g_pGraphics->resY * 0x14) / 0x1e0) -
                                        1);
                g_menuRect[3] = (short)(((int)(g_pGraphics->resY * 0xa3) / 0x1e0 -
                                          (int)(g_pGraphics->resY * 0x82) / 0x1e0) +
                                        1);
                g_menuRect[0] = (short)((int)(g_pGraphics->resX * 0x20) / 0x280);
                g_menuRect[2] = (short)((int)(g_pGraphics->resX * 0x41) / 0x280 -
                                        (int)(g_pGraphics->resX * 0x20) / 0x280);
                Sprite_FillRect(&g_pGraphics->field309_0x150, g_menuRect, g_menuRowFillColour, 2);
            } else {
                strcpy(CFrontend::m_stringDest,
                       (char *)CFrontend::GetModeSpecificCountryText(NetPlayers_GetStandingCar(i, 1)));
                CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
                GameMenus_DrawRowFrame(i, (-(int)g_pGraphics->resY * 0x14) / 0x1e0, 1);
            }
            Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x5c) / 0x280,
                          ((int)(g_pGraphics->resY * 0xa3) / 0x1e0 +
                           ((int)(g_pGraphics->resY * 0x2d) / 0x1e0) * i) -
                              (int)(g_pGraphics->resY * 0x14) / 0x1e0,
                          (int *)g_menuFrameColour, 0x21);
            if (i == 0 || (i > 0 && NetPlayers_GetPlayerRank(i, 1) != NetPlayers_GetPlayerRank(i - 1, 1))) {
                sprintf(CFrontend::m_stringDest, g_stageNumberFormat, NetPlayers_GetPlayerRank(i, 1));
            } else {
                sprintf(CFrontend::m_stringDest, "%s", g_stageResultSameTime);
            }
            Font_DrawText(1, CFrontend::m_stringDest,
                          ((int)(g_pGraphics->resX * 0x41) / 0x280 -
                           (int)(g_pGraphics->resX * 0x20) / 0x280) / 2 +
                              (int)(g_pGraphics->resX * 0x20) / 0x280,
                          (int)(g_pGraphics->resY * 8) / 0x1e0 +
                              (int)(g_pGraphics->resY * 0x82) / 0x1e0 +
                              ((((int)(g_pGraphics->resY * 0xa3) / 0x1e0 -
                                 (int)(g_pGraphics->resY * 0x82) / 0x1e0) / 2 +
                                ((int)(g_pGraphics->resY * 0x2d) / 0x1e0) * i) -
                               (int)(g_pGraphics->resY * 0x14) / 0x1e0),
                          (int *)g_menuFrameColour, 0x12);
            i++;
        } while (i < NetPlayers_GetStandingCount());
    }
    Font_SetBlendMode(2);
}

// GLOBAL: CMR2 0x00541cc8
Menu *g_pHeaderMenu;

// Draws the text and, right after it, the two-pixel marker rectangle that
// separates the breadcrumb entries, then moves x past both.
#define GAMEMENUS_HEADER_MARKER(pText)                                                        \
    x = x + (int)(g_pGraphics->resX * 8) / 0x280 + Font_GetTextWidth(2, (BYTE *)pText);         \
    g_menuRect[0] = (short)x;                                                                  \
    g_menuRect[1] = (short)((int)(g_pGraphics->resY * 0x25) / 0x1e0);                          \
    g_menuRect[2] = 2;                                                                         \
    g_menuRect[3] = (short)((int)(g_pGraphics->resY * 0x29) / 0x1e0);                          \
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_menuRect, g_menuFrameColour, 2);                \
    x = x + (int)(g_pGraphics->resX * 8) / 0x280 + 2;

// Breadcrumb of the in-game screens: the menu title (when it is the one that
// owns it), the stage or championship name and, for the first two stages, the
// race mode; each entry is followed by its marker rectangle.
// FUNCTION: CMR2 0x00451890
void GameMenu_DrawInGameBreadcrumb(Menu *pMenu)
{
    int flag;
    char *pText;
    int x;

    x = (int)(g_pGraphics->resX * 0x1e) / 0x280;
    if (pMenu == &g_menu0x00541218) {
        Font_DrawText(2, CFrontend::GetTextString(0xac), x, (int)(g_pGraphics->resY * 0x43) / 0x1e0,
                      (int *)g_menuFrameColour, 0x11);
        GAMEMENUS_HEADER_MARKER(CFrontend::GetTextString(0xac))
    }
    if ((BYTE)RallyData_GetSelectionBits10To11() < 2) {
        sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x43),
                0x62 - (BYTE)RallyData_GetSelectionBits10To11());
        flag = 1;
    } else {
        flag = 0;
        if ((BYTE)RallyData_GetSelectionBits12To13() == 0) {
            pText = CFrontend::GetTextString(5);
        } else {
            pText = CFrontend::GetTextString(7);
        }
        sprintf(CFrontend::m_stringDest, "%s", pText);
        CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
    }
    Font_DrawText(2, CFrontend::m_stringDest, x, (int)(g_pGraphics->resY * 0x43) / 0x1e0,
                  (int *)g_menuFrameColour, 0x11);
    if (flag) {
        GAMEMENUS_HEADER_MARKER(CFrontend::m_stringDest)
        GameMenus_FormatModeName(CFrontend::m_stringDest,
                                 (BYTE)RallyData_GetSelectionBits10To11() * 3 + (BYTE)RallyData_GetSelectionBits12To13());
        CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
        Font_DrawText(2, CFrontend::m_stringDest, x, (int)(g_pGraphics->resY * 0x43) / 0x1e0,
                      (int *)g_menuFrameColour, 0x11);
    }
    if (g_pHeaderMenu == &g_menu0x0053ec48) {
        GAMEMENUS_HEADER_MARKER(CFrontend::m_stringDest)
        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(0x8d));
        Font_DrawText(2, CFrontend::m_stringDest,
                      x,
                      (int)(g_pGraphics->resY * 0x43) / 0x1e0, (int *)g_menuFrameColour, 0x11);
    }
}

// Standings table of the championship: one row per driver with its name, its
// total time, the status (boxed when the driver is not classified) and the
// time of the row, which is only printed when it differs from the previous
// driver's (a "=" otherwise). The header comes from the shared breadcrumb
// helper, which is still pending, so that one call is the only difference.
// FUNCTION: CMR2 0x00452430
void GameMenu_DrawChampionshipDriverStandings(Menu *pMenu)
{
    int i;

    GameMenu_ClearMenuListWithHighlight();
    GameMenu_DrawInGameBreadcrumb(pMenu);
    i = 0;
    if (NetPlayers_GetStandingCount() > 0) {
        do {
            Font_DrawText(1, NetPlayers_GetStandingName(i, 0), (int)(g_pGraphics->resX * 0x5c) / 0x280,
                          (int)(g_pGraphics->resY * 3) / 0x1e0 +
                              (((int)(g_pGraphics->resY * 0x82) / 0x1e0 +
                                ((int)(g_pGraphics->resY * 0x2d) / 0x1e0) * i) -
                               (int)(g_pGraphics->resY * 0x14) / 0x1e0) -
                              (int)(g_pGraphics->resY * 8) / 0x1e0,
                          (int *)g_menuFrameColour, 9);
            FormatCentisecondsAsMinSecMSec(NetPlayers_GetStandingTime(i, 0), CFrontend::m_stringDest);
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x21e) / 0x280,
                          (((int)(g_pGraphics->resY * 0x82) / 0x1e0 +
                            ((int)(g_pGraphics->resY * 0x2d) / 0x1e0) * i) -
                           (int)(g_pGraphics->resY * 0x14) / 0x1e0) -
                              (int)(g_pGraphics->resY * 8) / 0x1e0,
                          (int *)g_menuFrameColour, 9);
            if (NetPlayers_GetStandingPlayerIndex(i, 0) == -2) {
                strcpy(CFrontend::m_stringDest,
                       (char *)CFrontend::GetModeSpecificCountryText(NetPlayers_GetStandingCar(i, 0)));
                CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
                g_menuRect[1] = (short)((((int)(g_pGraphics->resY * 0x82) / 0x1e0 +
                                          ((int)(g_pGraphics->resY * 0x2d) / 0x1e0) * i) -
                                         (int)(g_pGraphics->resY * 0x14) / 0x1e0) -
                                        1);
                g_menuRect[3] = (short)(((int)(g_pGraphics->resY * 0xa3) / 0x1e0 -
                                          (int)(g_pGraphics->resY * 0x82) / 0x1e0) +
                                        1);
                g_menuRect[0] = (short)((int)(g_pGraphics->resX * 0x20) / 0x280);
                g_menuRect[2] = (short)((int)(g_pGraphics->resX * 0x41) / 0x280 -
                                        (int)(g_pGraphics->resX * 0x20) / 0x280);
                Sprite_FillRect(&g_pGraphics->field309_0x150, g_menuRect, g_menuRowFillColour, 2);
            } else {
                strcpy(CFrontend::m_stringDest,
                       (char *)CFrontend::GetModeSpecificCountryText(NetPlayers_GetStandingCar(i, 0)));
                CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
                GameMenus_DrawRowFrame(i, (-(int)g_pGraphics->resY * 0x14) / 0x1e0, 1);
            }
            Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x5c) / 0x280,
                          ((int)(g_pGraphics->resY * 0xa3) / 0x1e0 +
                           ((int)(g_pGraphics->resY * 0x2d) / 0x1e0) * i) -
                              (int)(g_pGraphics->resY * 0x14) / 0x1e0,
                          (int *)g_menuFrameColour, 0x21);
            if (i == 0 || (i > 0 && NetPlayers_GetPlayerRank(i, 0) != NetPlayers_GetPlayerRank(i - 1, 0))) {
                sprintf(CFrontend::m_stringDest, g_stageNumberFormat, NetPlayers_GetPlayerRank(i, 0));
            } else {
                sprintf(CFrontend::m_stringDest, "%s", g_stageResultSameTime);
            }
            Font_DrawText(1, CFrontend::m_stringDest,
                          ((int)(g_pGraphics->resX * 0x41) / 0x280 -
                           (int)(g_pGraphics->resX * 0x20) / 0x280) / 2 +
                              (int)(g_pGraphics->resX * 0x20) / 0x280,
                          (int)(g_pGraphics->resY * 8) / 0x1e0 +
                              (int)(g_pGraphics->resY * 0x82) / 0x1e0 +
                              ((((int)(g_pGraphics->resY * 0xa3) / 0x1e0 -
                                 (int)(g_pGraphics->resY * 0x82) / 0x1e0) / 2 +
                                ((int)(g_pGraphics->resY * 0x2d) / 0x1e0) * i) -
                               (int)(g_pGraphics->resY * 0x14) / 0x1e0),
                          (int *)g_menuFrameColour, 0x12);
            i++;
        } while (i < NetPlayers_GetStandingCount());
    }
    Font_SetBlendMode(2);
}

// Draws the four edges of the box around one menu row.
// FUNCTION: CMR2 0x0044ec30
void GameMenus_DrawRowFrame(short row, short yOffset, char compact)
{
    int height;

    if (compact != 0) {
        height = g_pGraphics->resY * 0x2d;
    } else {
        height = g_pGraphics->resY * 0x34;
    }
    row = row * (short)(height / 0x1e0);

    g_menuRect[0] = (short)((int)(g_pGraphics->resX * 0x20) / 0x280) + 2;
    g_menuRect[1] = (short)((int)(g_pGraphics->resY * 0x82) / 0x1e0) - (short)g_gameMenuScrollY +
                    row + (short)g_gameMenuRowYOffset + yOffset;
    g_menuRect[2] = (short)((int)(g_pGraphics->resX * 0x41) / 0x280) -
                    (short)((int)(g_pGraphics->resX * 0x20) / 0x280) - 4;
    g_menuRect[3] = 2;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_menuRect, g_menuFrameColour, 2);

    g_menuRect[0] = (short)((int)(g_pGraphics->resX * 0x20) / 0x280) + 2;
    g_menuRect[1] = (short)((int)(g_pGraphics->resY * 0xa3) / 0x1e0) - (short)g_gameMenuScrollY +
                    row + (short)g_gameMenuRowYOffset - 2 + yOffset;
    g_menuRect[3] = 2;
    g_menuRect[2] = (short)((int)(g_pGraphics->resX * 0x41) / 0x280) -
                    (short)((int)(g_pGraphics->resX * 0x20) / 0x280) - 4;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_menuRect, g_menuFrameColour, 2);

    g_menuRect[0] = (short)((int)(g_pGraphics->resX * 0x41) / 0x280) - 2;
    g_menuRect[2] = 2;
    g_menuRect[1] = (short)((int)(g_pGraphics->resY * 0x82) / 0x1e0) - (short)g_gameMenuScrollY +
                    row + (short)g_gameMenuRowYOffset + yOffset;
    g_menuRect[3] = (short)((int)(g_pGraphics->resY * 0xa3) / 0x1e0) -
                    (short)((int)(g_pGraphics->resY * 0x82) / 0x1e0);
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_menuRect, g_menuFrameColour, 2);

    g_menuRect[0] = (short)((int)(g_pGraphics->resX * 0x20) / 0x280);
    g_menuRect[2] = 2;
    g_menuRect[1] = (short)((int)(g_pGraphics->resY * 0x82) / 0x1e0) - (short)g_gameMenuScrollY +
                    row + (short)g_gameMenuRowYOffset + yOffset;
    g_menuRect[3] = (short)((int)(g_pGraphics->resY * 0xa3) / 0x1e0) -
                    (short)((int)(g_pGraphics->resY * 0x82) / 0x1e0);
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_menuRect, g_menuFrameColour, 2);
}

// Draws a row of strings at (x, y), separated by a thin vertical bar.
// match 82%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00454df0
void GameMenus_DrawTextRow(int x, int y, char *pText, ...)
{
    g_menuRect[2] = 2;
    g_menuRect[1] = y - (int)(g_pGraphics->resY * 0x1e) / 0x1e0;
    g_menuRect[3] = (int)(g_pGraphics->resY * 0x29) / 0x1e0;
    char *pCur = pText;
    va_list ppNext;
    va_start(ppNext, pText);
    while (pCur != NULL) {
        CGenericFileLoader::StrLowerPolish(pCur);
        Font_DrawText(2, pCur, x, y, (int *)g_menuFrameColour, 0x11);
        sprintf(CFrontend::m_stringDest, "%s", pCur);
        pCur = va_arg(ppNext, char *);
        if (pCur == NULL) {
            break;
        }
        x += (int)(g_pGraphics->resX * 8) / 0x280 + Font_GetTextWidth(2, (BYTE *)CFrontend::m_stringDest);
        g_menuRect[0] = (short)x;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_menuRect, g_menuFrameColour, 2);
        x += (int)(g_pGraphics->resX * 8) / 0x280 + 2;
    }
    va_end(ppNext);
}

// GLOBAL: CMR2 0x00519ed4
DWORD g_menuHighlightColour = 0xbfae8072;

void *RallyData_GetAssignedCategoryRecord(unsigned int param1);
BYTE Graphics_IsRegisteredTimerRunning(BYTE *p);

void Input_ClearCharacterQueue(void);
BOOL Network_GetSessionStateFlag(void);

// GLOBAL: CMR2 0x00519ee4
int g_resultsChatLastCursor = -1;
// GLOBAL: CMR2 0x00540898
BYTE g_unk0x00540898;
// GLOBAL: CMR2 0x00541210
BYTE g_unk0x00541210;

// FUNCTION: CMR2 0x00449cd0
void GameMenu_ClearChatCharacterQueue(Menu *pMenu, int param)
{
    Input_ClearCharacterQueue();
}

int Network_RebuildSessionPlayerList(void);
void NetworkChat_SendLine(char *text);
bool Input_PopQueuedCharacter(int *pOut);

// GLOBAL: CMR2 0x00519ee8
char g_chatChars0x00519ee8[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ., !\"%&*()-=_+`/?\\:;[]{}'~";
// Set while the chat line (the item with value 0x3eb) is selected.
// GLOBAL: CMR2 0x00540e48
BYTE g_chatLineSelected;
// GLOBAL: CMR2 0x00541cf8
char g_chatLine[0x100];
// GLOBAL: CMR2 0x00541df8
int g_chatLineLength;

// Network results menu: typing on the chat line, Enter sends it, Escape
// leaves the menu. Up/down jump between the chat line and the menu items.
// match 86%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00449ce0
void GameMenu_HandleNetworkResultsChatInput(Menu *pMenu)
{
    DeviceInfo *pDevice;
    int key;
    int len;
    int maxLen;

    if (g_chatLineSelected) {
        pDevice = CInput::GetAvailableDeviceRecord(0);
        if (pDevice->pressed & 8)
            pMenu->cursor = 1;
        else if (pDevice->pressed & 4)
            pMenu->cursor = pMenu->itemCount - 1;
    }
    if (g_resultsChatLastCursor != pMenu->cursor) {
        if (pMenu->items[pMenu->cursor].value == 0x3eb) {
            Input_ClearCharacterQueue();
            g_chatLine[0] = 0;
            g_chatLineLength = 0;
            g_chatLineSelected = 1;
            Menu_SetFlags(pMenu, 0, 0, 0, 0);
        } else {
            if (pMenu->items[g_resultsChatLastCursor].value == 0x3eb)
                Menu_SetFlags(pMenu, 1, 1, 1, 1);
            g_chatLineSelected = 0;
        }
        g_resultsChatLastCursor = pMenu->cursor;
    }
    Network_RebuildSessionPlayerList();
    if (g_chatLineSelected)
        Sys_RequestTextInput(CFrontend::GetTextString(0x1c6), g_chatLine, 0xff);   // PORT
    if (!g_chatLineSelected || !Input_PopQueuedCharacter(&key))
        return;
    switch (key) {
    case 8:
        len = strlen(g_chatLine);
        if (len > 0) {
            g_chatLine[len - 1] = 0;
            g_chatLineLength--;
        }
        break;
    case 0xd:
        if (g_chatLine[0] != 0) {
            NetworkChat_SendLine(g_chatLine);
            g_chatLineLength = 0;
            g_chatLine[0] = 0;
        }
        break;
    case 0x1b:
        Menu_SetNextAction(pMenu->pParent);
        break;
    default:
        if (strchr(g_chatChars0x00519ee8, (char)key) == NULL)
            break;
        len = Font_GetTextWidth(0, (BYTE *)g_chatLine);
        if (g_pGraphics->resX >= 0x400)
            maxLen = 0x210;
        else
            maxLen = 0x14a;
        if (g_chatLineLength < 0xff && len < maxLen) {
            g_chatLine[g_chatLineLength] = (char)key;
            g_chatLine[g_chatLineLength + 1] = 0;
            g_chatLineLength++;
        }
        break;
    }
}

BYTE *NetworkLeaderboard_GetPublishedBoard(void);
char *NetworkChat_GetLine(int index);
extern char g_str0x00519fb0[];
extern char g_str0x00519fb4[];

// Format of the chat line while its cursor blinks (every other 20 frames).
// GLOBAL: CMR2 0x00519fb8
char g_str0x00519fb8[4] = "%s_";

// Draw callback of the time trial / network results menu (0x005416e0): the
// separator bars of the results box, the five rows of the current stage, the
// chat line, the ten rows of the network classification and, at the bottom,
// the standings rows of the menu items.
// MSVC6 gives the frame 4 bytes less (sub esp,0x14 vs 0x18) so every local
// moves one slot down: the original keeps the 10-row pointer p in a stack slot
// (0x14) and leaves a dead 4-byte slot at 0x24, while here p stays in a
// register and its (unused) slot ends up last (0x20). Same logic, only stack
// slot assignment and the loop's register allocation differ.
// match 85%: stack frame size/slot order and the 10-row loop register allocation
// FUNCTION: CMR2 0x004541c0
void GameMenu_DrawNetworkTimeTrialResults(Menu *pMenu)
{
    BYTE colour[4];
    char *p;
    int x;
    short rect[4];
    MenuItem *pItem;
    BYTE *pColour;
    BYTE *pEntries;
    int i;
    int y;

    rect[0] = (short)((int)(g_pGraphics->resX * 0x6c) / 0x280 - (int)(g_pGraphics->resX * 0x46) / 0x280);
    rect[1] = (short)((int)(g_pGraphics->resY * 0x55) / 0x1e0);
    rect[2] = (short)((int)(g_pGraphics->resX * 0x1a9) / 0x280);
    rect[3] = (short)((int)(g_pGraphics->resY * 0x6e) / 0x1e0);
    x = (int)(g_pGraphics->resX * 0x1e) / 0x280;
    colour[0] = 0xff;
    colour[1] = 0xff;
    colour[2] = 0xff;
    colour[3] = 0x20;
    GameMenu_ClearMenuListWithHighlight();
    if (g_chatLineSelected) {
        colour[3] = 0x20;
        pColour = g_menuFrameColour;
    } else {
        colour[3] = 0x10;
        pColour = g_menuTextColour;
    }
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, colour, 1);
    rect[1] += (short)((int)(g_pGraphics->resY * 10) / 0x1e0) + rect[3];
    rect[3] = (short)((int)(g_pGraphics->resY * 0x1e) / 0x1e0);
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, colour, 1);
    rect[0] = (short)((int)(g_pGraphics->resX * 0x1b3) / 0x280 +
                      (int)(g_pGraphics->resX * 0x6c) / 0x280 - (int)(g_pGraphics->resX * 0x46) / 0x280);
    rect[1] = (short)((int)(g_pGraphics->resY * 0x55) / 0x1e0);
    rect[2] = (short)((int)(g_pGraphics->resX * 0x82) / 0x280);
    rect[3] = (short)((int)(g_pGraphics->resY * 0x10e) / 0x1e0);
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, colour, 1);
    for (i = 4, y = 0x5a; y < 0xbe; y += 0x14, i--) {
        Font_DrawText(0, NetworkChat_GetLine(i),
                      (int)(g_pGraphics->resX * 0x76) / 0x280 - (int)(g_pGraphics->resX * 0x46) / 0x280,
                      (int)(g_pGraphics->resY * y) / 0x1e0, (int *)pColour, 9);
    }
    if (g_chatLineSelected) {
        if (!(CMain::GetFrameDelta() % 0x14 <= 9))
            sprintf(CFrontend::m_stringDest, g_str0x00519fb8, g_chatLine);
        else
            sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue, g_chatLine);
        Font_DrawText(0, CFrontend::m_stringDest,
                      (int)(g_pGraphics->resX * 0x76) / 0x280 - (int)(g_pGraphics->resX * 0x46) / 0x280,
                      (int)(g_pGraphics->resY * 0xd2) / 0x1e0, (int *)g_menuFrameColour, 9);
    }
    if (CGameInfo::GetConfiguredGameMode() != 10 && CGameInfo::GetConfiguredGameMode() != 12) {
        pEntries = NetworkLeaderboard_GetPublishedBoard();
        Font_DrawText(0, CFrontend::GetTextString(0xfe), rect[0] + (int)rect[2] / 2,
                      (int)(g_pGraphics->resY * 10) / 0x1e0 + (int)(g_pGraphics->resY * 100) / 0x1e0,
                      (int *)g_menuFrameColour, 0x12);
        if (pEntries != NULL) {
            for (i = 0, p = (char *)pEntries + 4; i < 10; i++, p += 8) {
                if (*p != 0) {
                    Font_DrawText(0, p, (int)(g_pGraphics->resX * 0x1fe) / 0x280,
                                  (int)(g_pGraphics->resY * 100) / 0x1e0 +
                                      ((int)(g_pGraphics->resY * 0x14) / 0x1e0) * (i + 3),
                                  (int *)g_menuFrameColour, 0x12);
                    sprintf(CFrontend::m_stringDest, g_stageNumberFormat, *(int *)(p + 4));
                    Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x23a) / 0x280,
                                  (int)(g_pGraphics->resY * 100) / 0x1e0 +
                                      ((int)(g_pGraphics->resY * 0x14) / 0x1e0) * (i + 3),
                                  (int *)g_menuFrameColour, 0x12);
                } else {
                    Font_DrawText(0, g_str0x00519fb4, (int)(g_pGraphics->resX * 0x1fe) / 0x280,
                                  (int)(g_pGraphics->resY * 100) / 0x1e0 +
                                      ((int)(g_pGraphics->resY * 0x14) / 0x1e0) * (i + 3),
                                  (int *)g_menuFrameColour, 0x12);
                    Font_DrawText(0, g_str0x00519fb0, (int)(g_pGraphics->resX * 0x23a) / 0x280,
                                  (int)(g_pGraphics->resY * 100) / 0x1e0 +
                                      ((int)(g_pGraphics->resY * 0x14) / 0x1e0) * (i + 3),
                                  (int *)g_menuFrameColour, 0x12);
                }
            }
        } else {
            Font_DrawText(0, CFrontend::GetTextString(0xfb), rect[0] + (int)rect[2] / 2,
                          (int)(g_pGraphics->resY * 100) / 0x1e0 + (int)(g_pGraphics->resY * 0x3c) / 0x1e0,
                          (int *)g_menuFrameColour, 0x12);
        }
    }
    Font_DrawText(2, CFrontend::GetTextString(0x56), x, (int)(g_pGraphics->resY * 0x43) / 0x1e0,
                  (int *)g_menuFrameColour, 0x11);
    rect[0] = (short)((int)(g_pGraphics->resX * 0x70) / 0x280);
    rect[2] = InRaceMenu_GetUpArrowTexture()->width;
    rect[3] = InRaceMenu_GetUpArrowTexture()->height;
    i = 1;
    pItem = pMenu->items + 1;
    if (pMenu->itemCount > 1) {
        do {
            rect[1] = (short)(((int)(g_pGraphics->resY * 0xde) / 0x1e0 +
                               ((int)(g_pGraphics->resY * 0x36) / 0x1e0) * i) -
                              (int)(g_pGraphics->resY * 0xd) / 0x1e0);
            if (pMenu->cursor == i) {
                GAMEMENUS_DRAW_STANDINGS_ROW(g_menuFrameColour, InRaceMenu_GetUpArrowTexture)
            } else {
                GAMEMENUS_DRAW_STANDINGS_ROW(g_menuTextColour, InRaceMenu_GetDownArrowTexture)
            }
            i++;
            pItem++;
        } while (i < pMenu->itemCount);
    }
    Font_SetBlendMode(2);
}

// Record time of the current stage (written only).
// GLOBAL: CMR2 0x00541cd8
int g_stageRecordTime;
extern char g_strDemoName0x00519ec4[4];

BYTE Race_GetStateByte(void);
void StageObject_AddCarSoundTimeSeconds(int index, int seconds);
unsigned char RallyDataStageIndex(void);
int Frontend_StoreSelectedDeviceOption(int index);
void Menu_QueueDefaultAction(void);
unsigned char CGameInfo::GetConfiguredPlayerCount(void);
unsigned char CGameInfo::IsConfiguredMultiplayer(void);
unsigned char CGameInfo::GetConfiguredGameMode(void);
unsigned char CGameInfo::GetConfiguredDifficulty(void);
unsigned char CGameInfo::GetGameModeOptionBit19(void);
unsigned char CGameInfo::GetSoundOptionBit30(void);
unsigned int CGameInfo::GetActiveCheatMask(void);
unsigned int CGameInfo::GetNetworkOptionBit11(void);
unsigned int RallyData_GetSelectionBits10To11(void);
unsigned char RallyData_GetSelectionFlag28(void);
unsigned int RallyData_GetSetupModeBits(void);
unsigned int RallyData_GetSetupFlag11(void);
unsigned char RallyData_GetFlag24(void);
BYTE StageTiming_GetStageClockState(void);
BYTE RallyData_GetModeStageGroup(BYTE flags, char mode);
int Race_ReadPlayerState37F68(int index);
int Knockout_HasPendingOrUnknownRoundMatch(void);
int RallyTiming_GetChampionshipPointsByPosition(int index);
int RallyTiming_GetChampionshipTimeSeconds(int index);
int RallyTiming_GetChampionshipStagePoints(int index);
int RallyTiming_GetOverallTimeCentiseconds(int index);
int RallyTiming_GetOverallPositionOfDriver(int iDriver);
int RallyTiming_GetStageTimeSeconds(int iDriver);
int RallyTiming_GetStagePositionOfDriver(int iDriver);
void Replay_SetControlStateByte(BYTE value);
void Frontend_ClearDeviceKeyFields(int index);
void Frontend_AdvanceDeviceOptionNibble(int index, int mode);
void FrontendRecords_ResetEventPoints(int index);
void Frontend_AddEventPointsAndPlacing(int index, int amount, int stat);
void Frontend_SetEventRallyStageTime(int index, int arg, int value);
void Frontend_SetEventRallyResult(int index, int value, unsigned int option, unsigned int field);
void Frontend_SetEventPlayerResult(int index, unsigned int value, unsigned int field);
void Frontend_SetEventArcadeResult(int index, unsigned int value, unsigned int field, int extra);
int Frontend_AccumulateMinimumRallyStageTime(int param_1, int param_2);
int Frontend_CopyImprovedStageRecordAndSplits(int param_1, int param_2, char *pName);
int FrontendRecords_InsertStageDeviceRecord(int param1, int index, char *pName);
int FrontendRecords_InsertStageCategoryRecord(int param1, int index, char *pName);
char FrontendRecords_InsertArcadeDeviceRecord(int param1, int index, char *pName);
unsigned char GameInfo_GetFrontendSessionFlag(void);
void GameMenu_BuildStageResultsMenu(char param1, Menu *pParent);
CategoryBestResults *RallyData_GetDriverBestResults(int index);
extern int g_unk0x00541cf0;
void GameMenu_SortStageResults(void);
void GameMenu_BuildTimeTrialResultsMenu(char param1, Menu *pParent);
void Knockout_RecordCurrentMatchResults(void);
extern Menu *g_pSavedHeaderMenu;

// Sets up the results screen after a stage: it stores the stage name, fills
// the stage result table with the state of every car, and picks the menu the
// results screens hang from (rewiring the parent/submenu pointers of that
// menu) according to the game mode and the championship state.
// match 66%: all the calls/constants of the body are in place; what is left is
// the argument evaluation order of the RallyTiming_GetChampionshipPointsByPosition/ced0/cef0, FUN_0040d3a0/
// d3d0/d070 and Race_ReadPlayerState37F68 chains (MSVC6 schedules them in another order),
// the `and eax,0xff` after Frontend_CopyImprovedStageRecordAndSplits (our prototype returns BYTE, the
// original used a 32-bit return) and one hoisted store to g_unk0x00540898.
// FUNCTION: CMR2 0x0044a1b0
void GameMenu_SetupStageResults(int param_1)
{
    char bVar1;
    char stageFlag;
    int state;
    int result;
    int i;
    int j;
    int car;
    int count;
    int time;
    int position;
    int driver;
    int minPos;
    int k;
    unsigned int *pState;
    Menu *pMenu;

    bVar1 = 0;
    if (Race_GetStateByte() != 0xff)
        StageObject_AddCarSoundTimeSeconds((char)Race_GetStateByte(), 0xb4);
    // The stage records cover the eight rally countries (88 entries). The
    // Rally Challenge runs in Japan (country 8), which has none: there the
    // original reads the split times behind the records as the name, and with
    // a profile that holds records writes tens of bytes into the four-byte
    // g_ghostName, over g_resultsChatLastCursor and the chat characters.
    // PORT: Japan has no stage record, so it has no ghost name or total. The
    // original read the split times behind the records instead and, with a
    // profile that holds records, wrote up to 53 bytes into g_ghostName: here
    // over the menu colours and format strings next to it, which made the
    // menus after a Rally Challenge misbehave and could crash on restarting.
    const int record = (RallyDataCountryIndex() & 0xff) * 11 + (RallyDataStageIndex() & 0xff);
    const int recordCount = sizeof(CGameInfo::GetGameInfoFieldA4Address()->rallyStageRecordTimes) /
                            sizeof(CGameInfo::GetGameInfoFieldA4Address()->rallyStageRecordTimes[0]);
    if (record < recordCount)
        sprintf(g_ghostName, "%s", CGameInfo::GetGameInfoFieldA4Address()->rallyStageRecordTimes[record].ident);
    else
        g_ghostName[0] = '\0';
    CGenericFileLoader::StrLowerPolish(g_ghostName);
    for (i = 0; i < 10; i++)
        g_ghostSplits[i] = Frontend_StoreSelectedDeviceOption(i);
    // The tenth split slot is the ghost's total from the stage record
    // (0x541adc lies inside g_ghostSplits).
    g_ghostSplits[9] = record < recordCount ?
        CGameInfo::GetGameInfoFieldA4Address()->rallyStageRecordTimes[record].value >> 7 & 0xffff : 0;
    if ((char)param_1 == 0)
        CGameInfo::SetInputAndGamePaused(1);
    Menu_QueueDefaultAction();
    g_unk0x00540898 = 0;
    g_unk0x00541210 = 0;
    if (CGameInfo::IsConfiguredMultiplayer() != 0) {
        g_unk0x00540898 = 0;
        if (StageUI_GetRaceEndEventCount() == (BYTE)CGameInfo::GetConfiguredPlayerCount() - 1)
            g_unk0x00540898 = 1;
    } else {
        g_unk0x00540898 = 1;
    }
    GameMenu_BuildStageResultsMenu(0, NULL);
    GameMenus_Build();
    if (CGameInfo::GetConfiguredGameMode() != 4 && (char)CGameInfo::GetActiveCheatMask() == 0 &&
        (char)CGameInfo::GetNetworkOptionBit11() == 0 && StageTiming_GetStageClockState() == 0 &&
        strncmp((char *)RallyData_GetRecord(0), g_strDemoName0x00519ec4, 3) != 0) {
        for (i = 0; i < (BYTE)RallyDataState(); i++) {
            car = (BYTE)StageUI_GetRaceEndEventCount() + i;
            count = StageTiming_GetValidStartTime(i);
            if (count > 0) {
                if ((BYTE)RallyData_GetFlag24() == 0) {
                    driver = StageTiming_GetCurrentSplitPositionOfDriver(car);
                    Frontend_SetEventRallyStageTime(car, 0, count);
                    if (CGameInfo::GetConfiguredGameMode() == 0 || CGameInfo::GetConfiguredGameMode() == 1) {
                        if (RallyDataStageIndex() == 0)
                            Frontend_ClearDeviceKeyFields(car);
                        Frontend_AdvanceDeviceOptionNibble(car, driver);
                    }
                    Frontend_MergeBestRallyStageTime(car, RallyData_GetDriverBestResults(car));
                    state = Frontend_CopyImprovedStageRecordAndSplits(i, car, (char *)RallyData_GetRecord(car));
                    stageFlag = (char)Frontend_AccumulateMinimumRallyStageTime(i, car);
                }
                if (CGameInfo::GetConfiguredGameMode() == 5) {
                    if ((BYTE)RallyData_GetSelectionBits12To13() == 2) {
                        time = RallyTiming_GetChampionshipTimeSeconds(i);
                        position = RallyTiming_GetChampionshipPointsByPosition(i);
                        Frontend_SetEventArcadeResult(car, position, time, RallyTiming_GetChampionshipStagePoints(i));
                        Frontend_MergeBestArcadeResult(car, RallyData_GetDriverBestResults(car));
                        FrontendRecords_InsertArcadeDeviceRecord(i, car, (char *)RallyData_GetRecord(car));
                    }
                    if ((Race_ReadPlayerState37F68(i) & 0x80) != 0 || (Race_ReadPlayerState37F68(i) & 0x2000) != 0 ||
                        (Race_ReadPlayerState37F68(i) & 0x1800) != 0 || (Race_ReadPlayerState37F68(i) & 0x300) != 0 ||
                        (Race_ReadPlayerState37F68(i) & 0x400) != 0)
                        bVar1 = 1;
                }
            }
        }
        if (g_unk0x00540898 != 0) {
            for (j = 0; j < (BYTE)CGameInfo::GetConfiguredPlayerCount(); j++) {
                if (CGameInfo::GetConfiguredGameMode() == 0 || CGameInfo::GetConfiguredGameMode() == 1) {
                    if (RallyDataStageIndex() ==
                        RallyData_GetModeStageGroup(RallyDataCountryIndex(), CGameInfo::GetConfiguredDifficulty())) {
                        driver = StageTiming_GetDriverSlot(j);
                        time = RallyTiming_GetOverallTimeCentiseconds(driver);
                        position = RallyTiming_GetOverallPositionOfDriver(driver);
                        Frontend_SetEventRallyResult(j, time, position, RallyTiming_GetStageTimeSeconds(driver));
                        if (CGameInfo::GetConfiguredGameMode() == 0) {
                            if ((BYTE)RallyDataCountryIndex() == 0)
                                FrontendRecords_ResetEventPoints(j);
                            Frontend_AddEventPointsAndPlacing(j, time, position);
                        }
                        Frontend_MergeBestRallyResult(j, RallyData_GetDriverBestResults(j));
                        FrontendRecords_InsertStageDeviceRecord(j, j, (char *)RallyData_GetRecord(j));
                        if ((Race_ReadPlayerState37F68(j) & 0x80) != 0 || (Race_ReadPlayerState37F68(j) & 0x2000) != 0)
                            bVar1 = 1;
                    }
                }
                if (CGameInfo::GetConfiguredGameMode() == 0 && (BYTE)RallyDataCountryIndex() == 7 &&
                    RallyDataStageIndex() ==
                        RallyData_GetModeStageGroup(RallyDataCountryIndex(), CGameInfo::GetConfiguredDifficulty())) {
                    driver = StageTiming_GetDriverSlot(j);
                    time = RallyTiming_GetStageTimeSeconds(driver);
                    position = RallyTiming_GetStagePositionOfDriver(driver);
                    Frontend_SetEventPlayerResult(j, position, time);
                    Frontend_MergeBestPlayerResult(j, RallyData_GetDriverBestResults(j));
                    FrontendRecords_InsertStageCategoryRecord(j, j, (char *)RallyData_GetRecord(j));
                }
            }
        }
    }
    if (CGameInfo::GetConfiguredGameMode() != 4 && CGameInfo::GetGameInfoFieldA4Address() != NULL) {
        g_stageRecordTime = CGameInfo::GetGameInfoFieldA4Address()->rallyStageRecordTimes[
                              (RallyDataCountryIndex() & 0xff) * 0xb + (RallyDataStageIndex() & 0xff)].value >> 7 & 0xffff;
        strcpy(g_stageRecordHolder, CGameInfo::GetGameInfoFieldA4Address()->rallyStageRecordTimes[
                                    (RallyDataCountryIndex() & 0xff) * 0xb + (RallyDataStageIndex() & 0xff)].ident);
    }
    if ((state & 1) != 0) {
        g_menu0x00541218.items[0].pSubMenu = &g_menu0x0053f5b0;
        if ((BYTE)RallyData_GetFlag24() != 0) {
            if (CGameInfo::GetGameModeOptionBit19() != 0) {
                g_menu0x0053f5b0.items[0].pSubMenu = &g_menu0x0053e2d8;
                g_menu0x0053e2d8.pParent = &g_menu0x0053f5b0;
            } else if ((BYTE)RallyData_GetSetupFlag11() != 0 &&
                       (RallyData_GetSetupModeBits() == 1 || RallyData_GetSetupModeBits() == 2)) {
                g_menu0x0053ea68.items[0].pSubMenu = &g_menu0x0053fb70;
                g_menu0x0053fb70.pParent = &g_menu0x0053ea68;
            } else {
                g_menu0x0053f5b0.items[0].pSubMenu = &g_menu0x0053fb70;
                g_menu0x0053fb70.pParent = &g_menu0x0053f5b0;
            }
        } else if (CGameInfo::GetGameModeOptionBit19() != 0) {
            if (CGameInfo::GetConfiguredGameMode() == 10) {
                g_menu0x0053f5b0.items[0].pSubMenu = &g_menu0x005406b8;
                g_menu0x005406b8.pParent = &g_menu0x0053f5b0;
            } else {
                g_menu0x0053f5b0.items[0].pSubMenu = &g_menu0x00540c68;
                g_menu0x00540c68.pParent = &g_menu0x0053f5b0;
            }
        } else {
            g_menu0x0053f5b0.items[0].pSubMenu = &g_menu0x0053e4b8;
            g_menu0x0053e4b8.pParent = &g_menu0x0053f5b0;
        }
    } else {
        if ((BYTE)RallyData_GetFlag24() != 0) {
            if (CGameInfo::GetGameModeOptionBit19() != 0) {
                g_menu0x00541218.items[0].pSubMenu = &g_menu0x0053e2d8;
                g_menu0x0053e2d8.pParent = &g_menu0x00541218;
            } else if (RallyData_GetSetupModeBits() == 1 || RallyData_GetSetupModeBits() == 2) {
                g_menu0x0053ea68.items[0].pSubMenu = &g_menu0x0053fb70;
                g_menu0x0053fb70.pParent = &g_menu0x0053ea68;
            } else {
                g_menu0x00541218.items[0].pSubMenu = &g_menu0x0053fb70;
                g_menu0x0053fb70.pParent = &g_menu0x00541218;
            }
        } else if (CGameInfo::GetGameModeOptionBit19() != 0) {
            if (CGameInfo::GetConfiguredGameMode() == 10) {
                g_menu0x00541218.items[0].pSubMenu = &g_menu0x005406b8;
                g_menu0x005406b8.pParent = &g_menu0x00541218;
            } else {
                g_menu0x00541218.items[0].pSubMenu = &g_menu0x00540c68;
                g_menu0x00540c68.pParent = &g_menu0x00541218;
            }
        } else {
            g_menu0x00541218.items[0].pSubMenu = &g_menu0x0053e4b8;
            g_menu0x0053e4b8.pParent = &g_menu0x00541218;
        }
    }
    if ((BYTE)RallyData_GetSelectionFlag28() != 0 && (BYTE)CGameInfo::GetSoundOptionBit30() != 0 && stageFlag != 0)
        Replay_SetControlStateByte(0);
    switch ((BYTE)CGameInfo::GetConfiguredGameMode()) {
    case 0:
    case 1:
        pMenu = &g_menu0x0053f008;
        g_menu0x0053e4b8.items[0].pSubMenu = pMenu;
        if (RallyDataStageIndex() !=
            RallyData_GetModeStageGroup(RallyDataCountryIndex(), CGameInfo::GetConfiguredDifficulty())) {
            g_unk0x00541210 = 0;
        } else {
            g_unk0x00541210 = 1;
            if (g_unk0x00540898 != 0) {
                if (CGameInfo::GetConfiguredGameMode() == 0) {
                    pMenu->items[0].pSubMenu = &g_menu0x0053e6a0;
                    g_menu0x0053e6a0.items[0].pSubMenu = &g_menu0x00540118;
                } else {
                    minPos = 99;
                    k = 0;
                    if (CGameInfo::GetConfiguredPlayerCount() != 0) {
                        for (i = 0xf; k < (BYTE)CGameInfo::GetConfiguredPlayerCount(); i--) {
                            if (RallyTiming_GetOverallPositionOfDriver(i) < minPos)
                                minPos = RallyTiming_GetOverallPositionOfDriver(i);
                            k++;
                        }
                    }
                    if (minPos > 5) {
                        pMenu->items[0].pSubMenu = &g_menu0x00541400;
                        g_menu0x00541400.pParent = pMenu;
                    } else {
                        pMenu->items[0].pSubMenu = &g_menu0x00540118;
                        g_menu0x00540118.pParent = pMenu;
                        g_menu0x00540118.items[0].pSubMenu = &g_menu0x00541400;
                    }
                    GameMenu_BuildStageResultsMenu(0, pMenu);
                }
                goto label_4a8d9;
            }
        }
        if (CGameInfo::IsConfiguredMultiplayer() == 0) {
            pMenu->items[0].pSubMenu = &g_menu0x00541400;
            GameMenu_BuildStageResultsMenu(1, pMenu);
        } else if (g_unk0x00540898 != 0) {
            pMenu->items[0].pSubMenu = &g_menu0x00541400;
            GameMenu_BuildStageResultsMenu(1, pMenu);
        } else if (g_menu0x00541218.items[0].pSubMenu == &g_menu0x0053f5b0) {
            g_menu0x0053f5b0.items[0].pSubMenu = &g_menu0x00541400;
            GameMenu_BuildStageResultsMenu(1, &g_menu0x0053f5b0);
        } else {
            g_menu0x00541218.items[0].pSubMenu = &g_menu0x00541400;
            GameMenu_BuildStageResultsMenu(1, &g_menu0x00541218);
        }
        result = 0x25;
        break;
    label_4a8d9:
        if (CGameInfo::GetConfiguredGameMode() == 0) {
            g_menu0x00540118.items[0].pSubMenu = &g_menu0x00541ae0;
            minPos = 99;
            k = 0;
            if (CGameInfo::GetConfiguredPlayerCount() != 0) {
                for (i = 0xf; k < (BYTE)CGameInfo::GetConfiguredPlayerCount(); i--) {
                    if (RallyTiming_GetOverallPositionOfDriver(i) < minPos)
                        minPos = RallyTiming_GetOverallPositionOfDriver(i);
                    k++;
                }
            }
            if ((BYTE)RallyDataCountryIndex() == 7 && minPos < 6) {
                g_unk0x00541cf0 = 0;
                g_menu0x00541ae0.items[0].pSubMenu = &g_menu0x0053f790;
                GameMenu_BuildStageResultsMenu(0, &g_menu0x0053f790);
            } else if (minPos <= 5) {
                g_menu0x00541ae0.items[0].pSubMenu = &g_menu0x00541400;
                GameMenu_BuildStageResultsMenu(1, &g_menu0x00541ae0);
            } else {
                g_menu0x00540118.items[0].pSubMenu = &g_menu0x00541400;
                GameMenu_BuildStageResultsMenu(0, &g_menu0x00540118);
            }
        }
        if (bVar1 != 0) {
            if (CGameInfo::GetConfiguredGameMode() == 5)
                pMenu = &g_menu0x0053e888;
            else
                pMenu = &g_menu0x00540118;
            pMenu->items[0].pSubMenu->pParent = &g_menu0x00540e50;
            g_menu0x00540e50.items[0].pSubMenu = pMenu->items[0].pSubMenu;
            g_menu0x00540e50.pParent = pMenu;
            pMenu->items[0].pSubMenu = &g_menu0x00540e50;
        }
        result = 0x25;
        break;
    case 2:
        if (CGameInfo::IsConfiguredMultiplayer() == 0) {
            GameMenu_BuildStageResultsMenu(0, &g_menu0x0053e4b8);
            g_menu0x0053e4b8.items[0].pSubMenu = &g_menu0x00541400;
            result = 0x23;
            break;
        }
        if (StageUI_GetRaceEndEventCount() == (BYTE)CGameInfo::GetConfiguredPlayerCount() - 1) {
            g_menu0x0053e4b8.items[0].pSubMenu = &g_menu0x00541400;
            g_menu0x00541400.pParent = &g_menu0x0053e4b8;
            result = 0x23;
            break;
        }
        if (g_menu0x00541218.items[0].pSubMenu == &g_menu0x0053f5b0) {
            g_menu0x0053f5b0.items[0].pSubMenu = &g_menu0x00541400;
            GameMenu_BuildStageResultsMenu(1, &g_menu0x0053f5b0);
            result = 0x23;
            break;
        }
        g_menu0x00541218.items[0].pSubMenu = &g_menu0x00541400;
        GameMenu_BuildStageResultsMenu(1, &g_menu0x00541218);
        result = 0x23;
        break;
    case 3:
        if (CGameInfo::IsConfiguredMultiplayer() == 0) {
            if (g_menu0x00541218.items[0].pSubMenu == &g_menu0x0053f5b0) {
                g_menu0x0053f5b0.items[0].pSubMenu = &g_menu0x00541400;
                GameMenu_BuildStageResultsMenu(0, &g_menu0x0053f5b0);
            } else {
                g_menu0x00541218.items[0].pSubMenu = &g_menu0x00541400;
                GameMenu_BuildStageResultsMenu(0, &g_menu0x00541218);
            }
            result = 0x23;
            break;
        }
        count = StageTiming_GetDriverSplitClock(0, GetStageSplitCount());
        g_stageResultTimes[(BYTE)StageUI_GetRaceEndEventCount()] = count;
        g_stageResultRecords[(BYTE)StageUI_GetRaceEndEventCount()] = (BYTE)StageUI_GetRaceEndEventCount();
        if (StageUI_GetRaceEndEventCount() == (BYTE)CGameInfo::GetConfiguredPlayerCount() - 1) {
            GameMenu_SortStageResults();
            if (g_menu0x00541218.items[0].pSubMenu == &g_menu0x0053f5b0) {
                g_menu0x0053f5b0.items[0].pSubMenu = &g_menu0x0053f3c8;
                g_menu0x0053f3c8.pParent = &g_menu0x0053f5b0;
            } else {
                g_menu0x00541218.items[0].pSubMenu = &g_menu0x0053f3c8;
                g_menu0x0053f3c8.pParent = &g_menu0x00541218;
            }
            GameMenu_BuildStageResultsMenu(0, &g_menu0x0053f3c8);
            result = 0x23;
            break;
        }
        if (g_menu0x00541218.items[0].pSubMenu == &g_menu0x0053f5b0) {
            g_menu0x0053f5b0.items[0].pSubMenu = &g_menu0x00541400;
            GameMenu_BuildStageResultsMenu(1, &g_menu0x0053f5b0);
        } else {
            g_menu0x00541218.items[0].pSubMenu = &g_menu0x00541400;
            GameMenu_BuildStageResultsMenu(1, &g_menu0x00541218);
        }
        result = 0x23;
        break;
    case 4:
        pState = RallyData_GetChampionshipState();
        switch ((*pState >> 3) & 7) {
        case 1:
            g_pKnockoutMatch = (KnockoutMatch *)(pState + ((*pState >> 0xc) & 0xf) * 3 + 0x16);
            break;
        case 2:
            g_pKnockoutMatch = (KnockoutMatch *)(pState + ((*pState >> 0xc) & 0xf) * 3 + 10);
            break;
        case 3:
            g_pKnockoutMatch = (KnockoutMatch *)(pState + ((*pState >> 0xc) & 0xf) * 3 + 4);
            break;
        case 4:
            g_pKnockoutMatch = (KnockoutMatch *)(pState + 1);
            break;
        }
        Knockout_RecordCurrentMatchResults();
        g_menu0x0053ea68.items[0].pSubMenu = &g_menu0x00541218;
        g_menu0x00541218.pParent = &g_menu0x0053ea68;
        g_menu0x00541218.items[0].pSubMenu = &g_menu0x0053f1e8;
        if ((*pState & 0x400000) == 0 || (BYTE)(*pState & 0x38) < 0x20)
            GameMenu_BuildStageResultsMenu(Knockout_HasPendingOrUnknownRoundMatch(), &g_menu0x0053f1e8);
        else
            GameMenu_BuildStageResultsMenu(0, &g_menu0x0053f1e8);
        result = 0x27;
        break;
    case 5:
        if ((BYTE)RallyData_GetSelectionBits10To11() != 2) {
            g_menu0x0053fb70.items[0].pSubMenu = &g_menu0x0053ec48;
            g_menu0x0053ec48.items[0].pSubMenu = &g_menu0x0053e888;
            if ((BYTE)RallyData_GetSelectionBits12To13() == 2) {
                g_menu0x0053e888.items[0].pSubMenu = &g_menu0x0053ee28;
                g_menu0x00541400.pParent = &g_menu0x0053ee28;
            } else {
                g_menu0x0053e888.items[0].pSubMenu = &g_menu0x00541400;
                GameMenu_BuildStageResultsMenu(1, &g_menu0x0053e888);
            }
        } else {
            g_menu0x0053fb70.items[0].pSubMenu = &g_menu0x00541400;
            g_menu0x00541400.pParent = &g_menu0x0053fb70;
        }
        if ((BYTE)RallyData_GetSelectionBits10To11() == 2)
            result = 0x29;
        else
            result = 0x2b;
        if (bVar1 != 0) {
            if (CGameInfo::GetConfiguredGameMode() == 5)
                pMenu = &g_menu0x0053e888;
            else
                pMenu = &g_menu0x00540118;
            pMenu->items[0].pSubMenu->pParent = &g_menu0x00540e50;
            g_menu0x00540e50.items[0].pSubMenu = pMenu->items[0].pSubMenu;
            g_menu0x00540e50.pParent = pMenu;
            pMenu->items[0].pSubMenu = &g_menu0x00540e50;
        }
        break;
    case 6:
    case 7:
        g_menu0x0053fb70.items[0].pSubMenu = &g_menu0x00541400;
        g_menu0x00541400.pParent = &g_menu0x0053fb70;
        result = 0x29;
        break;
    case 8:
        stageFlag = (char)RallyData_GetModeStageGroup(RallyDataCountryIndex(), 2);
        g_menu0x0053fd58.pParent = &g_menu0x00540c68;
        g_menu0x00540c68.items[0].pSubMenu = &g_menu0x0053fd58;
        if (RallyDataStageIndex() == (BYTE)stageFlag) {
            g_menu0x0053fd58.items[0].pSubMenu = &g_menu0x00541030;
            g_menu0x00541030.pParent = &g_menu0x0053fd58;
            g_menu0x00541030.items[0].pSubMenu = &g_menu0x005416e0;
            GameMenu_BuildTimeTrialResultsMenu(0, &g_menu0x00541030);
        } else {
            g_menu0x0053fd58.items[0].pSubMenu = &g_menu0x005416e0;
            GameMenu_BuildTimeTrialResultsMenu(1, &g_menu0x0053fd58);
        }
        result = 0x25;
        break;
    case 9:
        GameMenu_BuildTimeTrialResultsMenu(0, &g_menu0x00540c68);
        g_menu0x00540c68.items[0].pSubMenu = &g_menu0x005416e0;
        result = 0x23;
        break;
    case 10:
        GameMenu_BuildTimeTrialResultsMenu(0, &g_menu0x005406b8);
        g_menu0x005406b8.items[0].pSubMenu = &g_menu0x005416e0;
        g_menu0x005406b8.pParent = &g_menu0x00541218;
        result = 0x23;
        break;
    case 11:
    case 12:
        GameMenu_BuildTimeTrialResultsMenu(0, &g_menu0x0053e2d8);
        g_menu0x0053e2d8.items[0].pSubMenu = &g_menu0x005416e0;
        result = 0x23;
        break;
    default:
        result = param_1;
        break;
    }
    g_menu0x005402f8.items[Menu_FindItem(&g_menu0x005402f8, 0x3e8)].id = (short)result;
    g_menu0x00541400.items[Menu_FindItem(&g_menu0x00541400, 0x3e8)].id = (short)result;
    g_menu0x005416e0.items[Menu_FindItem(&g_menu0x005416e0, 0x3e8)].id = (short)result;
    if (GameInfo_GetFrontendSessionFlag() != 0) {
        g_pSavedHeaderMenu = &g_menu0x005404d8;
        g_pHeaderMenu = NULL;
    } else {
        g_pHeaderMenu = NULL;
        if ((char)param_1 != 0)
            g_pSavedHeaderMenu = &g_menu0x0053ff38;
        else
            g_pSavedHeaderMenu = &g_menu0x0053ea68;
    }
}

// Builds the stage results menu: continue (or next stage), the replay and
// save entries, then quit.
// FUNCTION: CMR2 0x0044af70
void GameMenu_BuildStageResultsMenu(char param1, Menu *pParent)
{
    Menu_Init(&g_menu0x00541400, 0, 0, 0, pParent, NULL, 1, 0, 1);
    if (param1 != 0) {
        if (CGameInfo::IsConfiguredMultiplayer() &&
            StageUI_GetRaceEndEventCount() != (unsigned int)(CGameInfo::GetConfiguredPlayerCount() - 1)) {
            Menu_AddItemType4(&g_menu0x00541400, 0, 0x21, (MenuItemAction)GameMenu_RequestResultsContinue, 0x3ea);
        } else if (g_unk0x00541210 && g_unk0x00540898 && CGameInfo::GetConfiguredGameMode() == 0) {
            Menu_AddItemType4(&g_menu0x00541400, 0, 0x80, (MenuItemAction)GameMenu_RequestResultsContinue, -1);
        } else if (CGameInfo::GetConfiguredGameMode() != 5) {
            Menu_AddItemType4(&g_menu0x00541400, 0, 0x1f, (MenuItemAction)GameMenu_RequestResultsContinue, -1);
        } else {
            Menu_AddItemType4(&g_menu0x00541400, 0, 0x95, (MenuItemAction)GameMenu_RequestResultsContinue, -1);
        }
        Menu_AddItemType2(&g_menu0x00541400, 0, -1, &g_menu0x005402f8, 0, 1000);
        Menu_AddItemType2(&g_menu0x00541400, 0, 0x2d, &g_menu0x005418d8, 0, -1);
    } else {
        Menu_AddItemType2(&g_menu0x00541400, 0, -1, NULL, (MenuItemAction)GameMenu_RequestResultsRestart, 1000);
        if (CGameInfo::GetConfiguredGameMode() == 2 || CGameInfo::GetConfiguredGameMode() == 3)
            Menu_AddItemType2(&g_menu0x00541400, 0, 0x8b, NULL, (MenuItemAction)GameMenu_RequestResultsRetry, 0x3e9);
        Menu_AddItemType2(&g_menu0x00541400, 0, 0xa8, NULL, (MenuItemAction)GameMenu_RequestResultsQuit, -1);
    }
    Menu_SetCallbacks(&g_menu0x00541400, NULL, NULL, (MenuCallback)GameMenu_DrawStandingsMenuItems, NULL);
    Menu_ValidateCursor(&g_menu0x00541400, 0);
}

// Same for the time trial / network results menu.
// FUNCTION: CMR2 0x0044b0d0
void GameMenu_BuildTimeTrialResultsMenu(char param1, Menu *pParent)
{
    g_resultsChatLastCursor = -1;
    Menu_Init(&g_menu0x005416e0, 0, 0, 0, pParent, NULL, 1, 0, 1);
    Menu_AddItemType4(&g_menu0x005416e0, 0, -1, 0, 0x3eb);
    if (param1 != 0) {
        if (Network_GetSessionStateFlag()) {
            Menu_AddItemType4(&g_menu0x005416e0, 0, 0x1f, (MenuItemAction)GameMenu_RequestResultsContinue, -1);
            Menu_AddItemType2(&g_menu0x005416e0, 0, -1, &g_menu0x005402f8, 0, 1000);
            Menu_AddItemType2(&g_menu0x005416e0, 0, 0xf6, &g_menu0x005408a0, 0, -1);
        }
        Menu_AddItemType2(&g_menu0x005416e0, 0, 0x2d, &g_menu0x005418d8, 0, -1);
    } else {
        if (Network_GetSessionStateFlag()) {
            if (CGameInfo::GetConfiguredGameMode() != 0xc)
                Menu_AddItemType2(&g_menu0x005416e0, 0, -1, NULL, (MenuItemAction)GameMenu_RequestResultsRestart, 1000);
            if (CGameInfo::GetConfiguredGameMode() == 9 || CGameInfo::GetConfiguredGameMode() == 10)
                Menu_AddItemType2(&g_menu0x005416e0, 0, 0x8b, NULL, (MenuItemAction)GameMenu_RequestResultsRetry, 0x3e9);
            Menu_AddItemType2(&g_menu0x005416e0, 0, 0xf6, NULL, (MenuItemAction)GameMenu_RequestNetworkResultsQuit, -1);
        } else if (CGameInfo::GetConfiguredGameMode() == 10) {
            Menu_AddItemType2(&g_menu0x005416e0, 0, -1, NULL, (MenuItemAction)GameMenu_RequestResultsRestart, 1000);
            Menu_AddItemType2(&g_menu0x005416e0, 0, 0x8b, NULL, (MenuItemAction)GameMenu_RequestResultsRetry, 0x3e9);
        }
        Menu_AddItemType2(&g_menu0x005416e0, 0, 0xa8, NULL, (MenuItemAction)GameMenu_RequestResultsQuit, -1);
    }
    Menu_SetCallbacks(&g_menu0x005416e0, (MenuCallback)GameMenu_ClearChatCharacterQueue, (MenuCallback)GameMenu_HandleNetworkResultsChatInput,
                      (MenuCallback)GameMenu_DrawNetworkTimeTrialResults, NULL);
    Menu_ValidateCursor(&g_menu0x005416e0, 0);
}


// Blink timer of the stage end banner.
// GLOBAL: CMR2 0x0053e880
BYTE g_stageEndBannerBlink[8];
// GLOBAL: CMR2 0x00541cf0
int g_unk0x00541cf0;
// GLOBAL: CMR2 0x00519f44
char g_nameSeparator0x00519f44[] = " - ";

// Stage end banner: the event title, a separator bar and the result text
// appear one after the other, then one line per driver with its category
// record name (or the driver name) and position.
// FUNCTION: CMR2 0x0044b3a0
void GameMenu_DrawStageEndBanner(void)
{
    int y;
    int x;
    int resY;
    int i;
    int position;
    int *pResY;
    int lineHeight;

    x = (int)(g_pGraphics->resX * 30) / 640;
    y = (int)(g_pGraphics->resY * 242) / 480;
    if (g_unk0x00541cf0 <= 0)
        return;
    if (g_unk0x00541cf0 > 0x4000 || !Graphics_IsRegisteredTimerRunning(g_stageEndBannerBlink)) {
        sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(CGameInfo::GetConfiguredDifficulty() + 0x8e));
        Font_DrawText(2, CFrontend::m_stringDest, x, y, (int *)g_menuFrameColour, 0x11);
    }
    if (g_unk0x00541cf0 > 0x8000 || !Graphics_IsRegisteredTimerRunning(g_stageEndBannerBlink)) {
        x += Font_GetTextWidth(2, (BYTE *)CFrontend::m_stringDest);
        x += (int)(g_pGraphics->resX * 8) / 640;
        pResY = &g_pGraphics->resY;
        g_menuRect[0] = (short)x;
        resY = *pResY;
        lineHeight = Font_GetLineHeight(2);
        g_menuRect[1] = (short)(resY * 242 / 480 + resY * 4 / 480 - lineHeight);
        g_menuRect[2] = 2;
        g_menuRect[3] = (short)((int)(*pResY * 41) / 480);
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_menuRect, g_menuFrameColour, 2);
        x += (int)(g_pGraphics->resX * 8) / 640;
        sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(0x93));
        Font_DrawText(2, CFrontend::m_stringDest, x, y, (int *)g_menuFrameColour, 0x11);
    }
    if (g_unk0x00541cf0 <= 0xc000 && Graphics_IsRegisteredTimerRunning(g_stageEndBannerBlink))
        return;
    for (i = 0; i < CGameInfo::GetConfiguredPlayerCount(); i++) {
        position = RallyTiming_GetStagePositionOfDriver(StageTiming_GetDriverSlot(i));
        if (strlen((char *)RallyData_GetAssignedCategoryRecord(i)) != 0 && strlen((char *)RallyData_GetAssignedCategoryRecord(i)) < 0x1e) {
            sprintf(CFrontend::m_stringDest, "%s", (char *)RallyData_GetAssignedCategoryRecord(i));
            sprintf(CFrontend::m_stringDest + strlen((char *)RallyData_GetAssignedCategoryRecord(i)), "%s", g_nameSeparator0x00519f44);
        } else {
            sprintf(CFrontend::m_stringDest, "%s", (char *)RallyData_GetRecord(i));
            sprintf(CFrontend::m_stringDest + strlen((char *)RallyData_GetRecord(i)), "%s", g_nameSeparator0x00519f44);
        }
        switch (position) {
        case 0:
            sprintf(CFrontend::m_stringDest + strlen(CFrontend::m_stringDest), "%s", CFrontend::GetTextString(0x51));
            break;
        case 1:
            sprintf(CFrontend::m_stringDest + strlen(CFrontend::m_stringDest), "%s", CFrontend::GetTextString(0x52));
            break;
        case 2:
            sprintf(CFrontend::m_stringDest + strlen(CFrontend::m_stringDest), "%s", CFrontend::GetTextString(0x53));
            break;
        default:
            sprintf(CFrontend::m_stringDest + strlen(CFrontend::m_stringDest), CFrontend::GetTextString(0x54), position + 1);
            break;
        }
        CGenericFileLoader::StrUpperPolish((BYTE *)CFrontend::m_stringDest);
        Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 30) / 640,
                      (int)(g_pGraphics->resY * 242) / 480 +
                          (i + 1) * ((int)(g_pGraphics->resY * 10) / 480 + Font_GetLineHeight(0)),
                      (int *)g_menuFrameColour, 0x11);
    }
}

// Clears the menu list: fills the whole screen with the highlight colour
// through the menu sprite buffer (the sprite layer only tints, so the menu
// colour is blended first).
// FUNCTION: CMR2 0x0044b760
void GameMenu_ClearMenuListWithHighlight(void)
{
    g_menuRect[0] = 0;
    g_menuRect[1] = 0;
    g_menuRect[2] = (short)g_pGraphics->resX;
    g_menuRect[3] = (short)g_pGraphics->resY;
    Font_SetBlendMode(2);
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_menuRect, (BYTE *)&g_menuHighlightColour, 2);
}

// Fills the row the cursor is on with the highlight colour.
// FUNCTION: CMR2 0x0044f680
void GameMenus_DrawRowHighlight(short row)
{
    DWORD colour;

    colour = g_menuHighlightColour;
    *((BYTE *)&colour + 3) = 0xff;
    g_menuRect[1] = (short)((int)(g_pGraphics->resY * 0x82) / 0x1e0) +
                    (short)((int)(g_pGraphics->resY * 0x34) / 0x1e0) * row -
                    (short)g_gameMenuScrollY - 1;
    g_menuRect[3] = (short)((int)(g_pGraphics->resY * 0xa3) / 0x1e0) -
                    (short)((int)(g_pGraphics->resY * 0x82) / 0x1e0) + 1;
    g_menuRect[0] = (short)((int)(g_pGraphics->resX * 0x20) / 0x280);
    g_menuRect[2] = (short)((int)(g_pGraphics->resX * 0x41) / 0x280) -
                    (short)((int)(g_pGraphics->resX * 0x20) / 0x280);
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_menuRect, (BYTE *)&colour, 2);
}

// Draws the thin separator under one menu row.
// FUNCTION: CMR2 0x0044f7b0
void GameMenus_DrawRowSeparator(short row)
{
    int resY;

    g_menuRect[0] = (short)((int)(g_pGraphics->resX * 0x20) / 0x280);
    resY = (int)g_pGraphics->resY;
    g_menuRect[1] = (short)((resY * 0x2f) / 0x1e0) +
                    (short)((resY * 0x82) / 0x1e0) +
                    (short)((resY * 0x34) / 0x1e0) * row -
                    (short)g_gameMenuScrollY;
    g_menuRect[2] = (short)((int)(g_pGraphics->resX * 0x240) / 0x280);
    g_menuRect[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_menuRect, g_menuFrameColour, 2);
}

// GLOBAL: CMR2 0x00519f98
char g_strInvalid[8] = "INVALID";
// GLOBAL: CMR2 0x00519fa0
char g_pointsFormat[] = "%d pts";
// GLOBAL: CMR2 0x00519fa8
char g_plusTimeFormat[] = "+ %s";

// Formats the name of one of the eight game modes into the shared string.
// FUNCTION: CMR2 0x00451ce0
void GameMenus_FormatModeName(char *unused, int mode)
{
    switch (mode) {
    case 0:
        sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(6));
        return;
    case 1:
        sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(3));
        return;
    case 2:
        sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(1));
        return;
    case 3:
        sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(4));
        return;
    case 4:
        sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(0));
        return;
    case 5:
        sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(2));
        return;
    case 6:
        sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(5));
        return;
    case 7:
        sprintf(CFrontend::m_stringDest, "%s", CFrontend::GetTextString(7));
        return;
    default:
        sprintf(CFrontend::m_stringDest, "%s", g_strInvalid);
        return;
    }
}

// Size taken from the next known global (0x538a84); nothing reads past it yet.
// GLOBAL: CMR2 0x00538130
BYTE g_unk0x00538130[0x40];

// FUNCTION: CMR2 0x0041f900
BYTE *GameMenu_GetChampionshipTransitionState(void)
{
    return g_unk0x00538130;
}

// GLOBAL: CMR2 0x005418d4
BYTE g_unk0x005418d4;
// Menu shown under the header when it is restored.
// GLOBAL: CMR2 0x00541ccc
Menu *g_pSavedHeaderMenu;

// FUNCTION: CMR2 0x0044a120
void GameMenu_ClearPauseOverlayLatch(void)
{
    g_unk0x005418d4 = 0;
}

// FUNCTION: CMR2 0x00448e60
void GameMenu_RestoreSavedHeaderMenu(void)
{
    g_pHeaderMenu = g_pSavedHeaderMenu;
}

extern Menu g_menu0x0053ea68;

// FUNCTION: CMR2 0x0044a130
int GameMenu_IsPauseHeaderActive(void)
{
    return g_pHeaderMenu == &g_menu0x0053ea68;
}

int Timer_GetValue(BYTE index);
void Graphics_StartShapedInterpolationTimer(BYTE *pSlot, int shape, int length, int param4, int start, int end, BYTE param7);

// Starts the fade timer of the pause overlay if it is not running and has
// not run yet, and samples its value.
// FUNCTION: CMR2 0x0044a150
void GameMenu_StartPauseOverlayFade(void)
{
    if (Graphics_IsRegisteredTimerRunning(g_stageEndBannerBlink) == 0 && g_unk0x00541cf0 == 0)
        Graphics_StartShapedInterpolationTimer(g_stageEndBannerBlink, 0, 200, 0, 0, 0x10000, 1);
    if (Graphics_IsRegisteredTimerRunning(g_stageEndBannerBlink))
        g_unk0x00541cf0 = Timer_GetValue(g_stageEndBannerBlink[0]);
}

// GLOBAL: CMR2 0x00541dfc
Menu *g_unk0x00541dfc;
// GLOBAL: CMR2 0x00541e00
int g_unk0x00541e00;

// Per-frame update of the header menu: waits two frames after the menu
// changes before running its callback 2 (and the network one).
// FUNCTION: CMR2 0x0044b330
void GameMenu_UpdateHeaderCallbacks(void)
{
    // PORT: laid out for the screen's size, as the other menus.
    MenuScreen_BeginOverlay();
    if (g_unk0x00541dfc == g_pHeaderMenu || g_pHeaderMenu == &g_menu0x0053ea68) {
        if (g_unk0x00541e00 > 0)
            goto tail;
        Menu_CallCallback2(g_pHeaderMenu);
        if ((BYTE)RallyData_IsChampionshipFinalStage())
            GameMenu_DrawStageEndBanner();
        g_gameMenuRowYOffset = 0;
        MenuScreen_EndOverlay();
        return;
    } else {
        g_unk0x00541dfc = g_pHeaderMenu;
        g_unk0x00541e00 = 2;
    }
tail:
    GameMenu_ClearMenuListWithHighlight();
    g_gameMenuRowYOffset = 0;
    g_unk0x00541e00--;
    MenuScreen_EndOverlay();
}

// Sorts the stage results by time (zero times, i.e. no time, go last).
// FUNCTION: CMR2 0x0044b270
void GameMenu_SortStageResults(void)
{
    int i;
    int j;
    int best = 0;
    int bestTime;
    int time;
    int record;
    BYTE recordI;

    for (i = 0; i < (int)((StageUI_GetRaceEndEventCount() & 0xff) + 1); i++) {
        bestTime = 0xffff;
        for (j = i; j < (int)((StageUI_GetRaceEndEventCount() & 0xff) + 1); j++) {
            if (g_stageResultTimes[j] != 0 && g_stageResultTimes[j] < bestTime) {
                bestTime = g_stageResultTimes[j];
                best = j;
            }
        }
        time = g_stageResultTimes[i];
        g_stageResultTimes[i] = g_stageResultTimes[best];
        g_stageResultTimes[best] = time;
        record = g_stageResultRecords[i];
        g_stageResultRecords[i] = g_stageResultRecords[best];
        g_stageResultRecords[best] = record;
    }
}

// --- 0x00448e70: stage results header fade (layer 0) -------------------------
unsigned char RallyDataState(void);
int RallyData_IsChampionshipFinalStage(void);
int NetRace_IsPlayerFadeActive(BYTE index);
int Stage_GetDriverCount(void);
int StageObject_GetCarSoundElapsedTime(int index);
void Input_TranslatePedalsToMenuKeys(void);
void Input_MergeAssignedJoystickButtons(int slot, DeviceInfo *pOut);
void Graphics_StartShapedInterpolationTimer(BYTE *pSlot, int shape, int length, int param4, int start, int end, BYTE param7);
int Timer_GetValue(BYTE index);

// Timer handle of the results header fade (first byte is the timer slot).
// GLOBAL: CMR2 0x005418c0
BYTE g_resultsHeaderFadeTimer[4];
// GLOBAL: CMR2 0x00541cf4
int g_unk0x00541cf4;

// Refreshes the header of the stage results screen: drops any driver record
// still being played in, snaps the header menu back to its saved parent and
// starts (or cancels) the fade of the results panel depending on the state of
// the header menu and the pressed buttons.
// FUNCTION: CMR2 0x00448e70
void GameMenu_RefreshResultsHeader(void)
{
    DeviceInfo *pDevice;
    unsigned int flags;
    Menu *action;
    int i;

    for (i = 0; i < (BYTE)RallyDataState(); i++) {
        if (NetRace_IsPlayerFadeActive(i) != 0)
            return;
    }
    GameMenu_RestoreSavedHeaderMenu();
    Input_TranslatePedalsToMenuKeys();
    Input_MergeAssignedJoystickButtons(0, CInput::GetAvailableDeviceRecord(0));
    pDevice = CInput::GetAvailableDeviceRecord(0);
    flags = pDevice->pressed;
    if ((BYTE)RallyData_IsChampionshipFinalStage() != 0)
        flags &= 0x10;
    if (g_pHeaderMenu == &g_menu0x0053ea68)
        flags &= 0xffdf;
    if (g_pHeaderMenu == &g_menu0x0053f008 || g_pHeaderMenu == &g_menu0x0053e4b8 ||
        g_pHeaderMenu == &g_menu0x0053e6a0 || g_pHeaderMenu == &g_menu0x00541ae0) {
        flags &= 0x3c;
        if ((flags & 4) == 0 || g_pHeaderMenu->items[0].max <= 0) {
            if ((flags & 8) != 0 &&
                (int)(BYTE)g_pHeaderMenu->items[0].max < (int)(BYTE)g_pHeaderMenu->items[0].min - 1) {
                Graphics_StartShapedInterpolationTimer(g_resultsHeaderFadeTimer, 2, 7, 0, 0, 0x10000, 0);
                g_standingsExtraRow = 1;
            }
        } else {
            Graphics_StartShapedInterpolationTimer(g_resultsHeaderFadeTimer, 2, 7, 0, 0x10000, 0, 0);
            g_unk0x005413f8 = 1;
        }
        if (Graphics_IsRegisteredTimerRunning(g_resultsHeaderFadeTimer)) {
            g_unk0x00541cf4 = Timer_GetValue(g_resultsHeaderFadeTimer[0]);
            g_gameMenuScrollY = (FixMul(((int)g_pGraphics->resY * 0x34) / 0x1e0 * 0x10000, g_unk0x00541cf4) >> 16);
        } else {
            g_standingsExtraRow = 0;
            g_unk0x005413f8 = 0;
            g_gameMenuScrollY = 0;
        }
    } else {
        g_standingsExtraRow = 0;
        g_unk0x005413f8 = 0;
        g_gameMenuScrollY = 0;
    }
    action = Menu_Update(g_pHeaderMenu, flags);
    if (action != 0) {
        g_menuBuildTime = StageObject_GetCarSoundElapsedTime(0);
        g_pSavedHeaderMenu = (Menu *)action;
    }
}
