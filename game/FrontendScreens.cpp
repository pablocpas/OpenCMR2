#include "SavedGame.h"
#include "port/sys.h"
#include "ChampionshipDriver.h"
#include "Frontend.h"
#include "FloatToInt.h"
#include "Game.h"
#include "GameInfo.h"
#include "RallyData.h"
#include "Graphics.h"
#include "Input.h"
#include "Font.h"
#include "GenericFileLoader.h"
#include <string.h>
#include <stdio.h>
#include "FrontendMenus.h"
#include "InstallInfo.h"
#include "Sprite.h"
#include "RegKey.h"
#include "FixedPoint.h"
#include <stdlib.h>
#include <stddef.h>
#include "FrontendDraw.h"
#include "Sound.h"
#include "main.h"
#include "NetworkLeaderboards.h"
#include "NetPlayers.h"
#include "FileBuffer.h"
#include "RecordBits.h"
#include "StageInfo.h"

extern int g_unk0x00819744;

#define PATH_X() ((int)(g_pGraphics->resX * 24) / 640)
#define PATH_Y() ((int)(g_pGraphics->resY * 38) / 480)

// Callbacks of the frontend screens, hooked to the menus built in
// FrontendMenus.cpp.

// GLOBAL: CMR2 0x00818f10
char g_unk0x00818f10;
// GLOBAL: CMR2 0x00525398
int g_scrollerSelection = -1;
// GLOBAL: CMR2 0x0052539c
int g_unk0x0052539c = -1;
// GLOBAL: CMR2 0x0081912c
int g_scrollerSelectionTimestamp;
// GLOBAL: CMR2 0x00819048
BYTE g_profilePlayersRemaining;
// GLOBAL: CMR2 0x00819130
char g_unk0x00819130[12];
// The 12 frontend scrollers are one contiguous array in the original (see
// FrontendScroller_ResetAll); the old per-address names are views (FrontendMenus.h).
// GLOBAL: CMR2 0x00819140
MenuScroller g_menuScrollers[12];
// GLOBAL: CMR2 0x008196e0
unsigned int g_recordsTableMode;
// GLOBAL: CMR2 0x008196e4
int g_creditsStartTime;
// Per-screen colour/palette ids, indexed by the screen number in field +7.
// GLOBAL: CMR2 0x008196e8
int g_screenPaletteIds[23];
// GLOBAL: CMR2 0x00819864
BYTE g_unk0x00819864;
// GLOBAL: CMR2 0x00819880
int g_profileCurrentPlayer;
// Sample index of the first frontend sound (move, select, back, error, toggle)
// GLOBAL: CMR2 0x00819888
int g_menuSoundBase;
// GLOBAL: CMR2 0x0081988c
int g_unk0x0081988c;
// The four "dot" textures of the frontend (dot00..dot03)
struct MenuDotTrail {
    Texture *textures[4];
    int positions[15];
};
// GLOBAL: CMR2 0x00819e94
MenuDotTrail g_menuDotTrail;
#define g_menuDotTextures (g_menuDotTrail.textures)
#define g_menuTrailPos (g_menuDotTrail.positions)
// GLOBAL: CMR2 0x0081987c
int g_creditsScrollOffset;
// GLOBAL: CMR2 0x0082a924
int g_savedGameListSelection;
// GLOBAL: CMR2 0x0082aa3c
int g_savedGameListTop;
// GLOBAL: CMR2 0x0082aa40
int g_unk0x0082aa40;
// GLOBAL: CMR2 0x0082ac48
int g_unk0x0082ac48;
// GLOBAL: CMR2 0x00525288
int g_selectedNetworkSession = -1;
// GLOBAL: CMR2 0x0052528c
int g_unk0x0052528c = -1;
// GLOBAL: CMR2 0x00525330
char g_nameChars0x00525330[68] = "abcdefghijklmnopqrstuvwxyz. ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
// GLOBAL: CMR2 0x00818cd8
char *g_pJoinPasswordText;
// GLOBAL: CMR2 0x00818ce0
int g_unk0x00818ce0;
// GLOBAL: CMR2 0x00818ce8
int g_unk0x00818ce8;
// GLOBAL: CMR2 0x00818d00
int g_lobbyLastCursor;
// Filtered stage/route ids, one int per entry (up to 0x16).
// GLOBAL: CMR2 0x00818d18
int g_filteredRouteIds[0x16];
// Index of the route/stage the setup screens currently point at.
// GLOBAL: CMR2 0x00818d70
int g_setupRouteIndex;
// Route ids of the four selected stages of the outgoing setup packet.
// GLOBAL: CMR2 0x00818d74
int g_setupSelectedRouteIds[4];
// Name of the network session being created or joined (edited on the
// session setup screen).
// GLOBAL: CMR2 0x00818ebc
char g_sessionName[20];
// GLOBAL: CMR2 0x00818ed0
BYTE g_lobbyChatSelected;
// GLOBAL: CMR2 0x00818d04
BYTE g_unk0x00818d04;
// GLOBAL: CMR2 0x00818da8
char g_joinPassword[256];
// GLOBAL: CMR2 0x00818ed4
int g_unk0x00818ed4[8];
// GLOBAL: CMR2 0x00818ef4
BYTE g_unk0x00818ef4;
// Password of the network session being created or joined.
// GLOBAL: CMR2 0x00818ef8
char g_sessionPassword[20];
// Time the host's setup packet was last applied (written only).
// GLOBAL: CMR2 0x00818f0c
unsigned int g_hostSetupAppliedTime;
// GLOBAL: CMR2 0x00819014
BYTE g_unk0x00819014;
// GLOBAL: CMR2 0x0081901c
int g_unk0x0081901c;
// GLOBAL: CMR2 0x00819020
int g_unk0x00819020;
// GLOBAL: CMR2 0x00819024
BYTE g_unk0x00819024;
// GLOBAL: CMR2 0x00819038
BYTE g_unk0x00819038;
// GLOBAL: CMR2 0x00819040
unsigned int g_unk0x00819040;
// GLOBAL: CMR2 0x00819044
int g_unk0x00819044;
// GLOBAL: CMR2 0x0081986c
BYTE g_unk0x0081986c;
// GLOBAL: CMR2 0x008182b4
int g_profileHeaderPlayerNumber;
// GLOBAL: CMR2 0x008189a4
BYTE g_profileNameEntryFlag;
// GLOBAL: CMR2 0x00818abc
Menu *g_pMenu0x00818abc;
// GLOBAL: CMR2 0x00818ac0
Menu *g_pMenu0x00818ac0;
// GLOBAL: CMR2 0x00818ac4
BYTE g_unk0x00818ac4;
// GLOBAL: CMR2 0x00818f14
BYTE g_lobbyChatLine[0xe0];
// GLOBAL: CMR2 0x00819030
Menu *g_pProfileRenameMenu;
// GLOBAL: CMR2 0x00819050
int g_unk0x00819050;
// GLOBAL: CMR2 0x008190f4
BYTE g_profileEnteredName[0x30];
// GLOBAL: CMR2 0x00819124
Menu *g_pProfileCompletionMenu;
// GLOBAL: CMR2 0x00819748
BYTE g_unk0x00819748;
// GLOBAL: CMR2 0x0081975c
int g_unk0x0081975c;
// GLOBAL: CMR2 0x00819870
Menu *g_pProfileReturnMenu;
// GLOBAL: CMR2 0x00819878
BYTE g_quitCancelled;
// GLOBAL: CMR2 0x00819884
int g_unk0x00819884;
// GLOBAL: CMR2 0x00826138
int g_networkMessageState;
// GLOBAL: CMR2 0x0081903c
unsigned int g_unk0x0081903c;
// GLOBAL: CMR2 0x00819754
unsigned int g_unk0x00819754;
// GLOBAL: CMR2 0x00819860
unsigned int g_unk0x00819860;
// GLOBAL: CMR2 0x00819868
int g_unk0x00819868;

// FUNCTION: CMR2 0x004d27c0
void FrontendRecords_SetProfileMode(BYTE value)
{
    g_recordsProfileMode = value;
}

// FUNCTION: CMR2 0x004d4c40
void FrontendMenu_DrawProfileActions(Menu *pMenu)
{
    char *text[3];

    FrontendDraw_PlayTime();
    text[0] = CFrontend::GetTextString(0xb);
    text[1] = CFrontend::GetTextString(pMenu->field_0x4);
    text[2] = (char *)RallyData_GetRecord(0);
    FrontendDraw_Breadcrumb(PATH_X(), PATH_Y(), text, 3);
    FrontendDraw_MenuList(pMenu, (char *)RallyData_GetRecord(0), -1, -1, 0, 1);
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

void Frontend_DrawRectangleOutline(short *pRect, BYTE *pColour);

// Lays a matrix-textured rect at (param_1, param_2) scaled to the screen and
// queues it through Frontend_DrawRectangleOutline; for param_3 in 1..3 the rect is replaced by
// the medal texture of that place, centred on the same point.
// FUNCTION: CMR2 0x004d4f90
void FrontendDraw_MatrixMedalRect(int param_1, int param_2, int param_3)
{
    Texture *pTexture;

    g_frontendDrawRect[0] = param_1 - (int)(g_pGraphics->resX * 0x30) / 0x280 / 2;
    g_frontendDrawRect[1] = param_2;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 0x30) / 640;
    g_frontendDrawRect[3] = (int)(g_pGraphics->resY * 0x1c) / 480;
    Frontend_DrawRectangleOutline(g_frontendDrawRect, (BYTE *)g_colourText0x0052496c);
    if (param_3 >= 1 && param_3 <= 3) {
        pTexture = (&CFrontend::m_pLgMatrixTexture)[param_3];
        g_frontendDrawRect[0] = param_1 - pTexture->width / 2;
        g_frontendDrawRect[1] = param_2;
        g_frontendDrawRect[2] = pTexture->width;
        g_frontendDrawRect[3] = pTexture->height;
        Sprite_Queue((SpriteRect *)&pTexture->field_0x11c, (SpriteRect *)g_frontendDrawRect, pTexture, 1, 0, NULL, NULL,
                     (BYTE *)g_colourWhite0x00524968, 8);
    }
}

// GLOBAL: CMR2 0x00524c88
char g_strDmdFormat[12] = "%s%.2d.dmd";
// GLOBAL: CMR2 0x00524c94
char g_strChaDmdFormat[12] = "cha%.2d.dmd";
// GLOBAL: CMR2 0x00524ca0
char g_strDmdBfl[36] = "%s\\frontend\\Textures\\dmd\\dmd.bfl";

// Stage map files (.dmd) of the 8 rallies (10 or 11 stages each) plus the
// 8 of the championship ("cha").
// GLOBAL: CMR2 0x008185bc
void *g_dmdFiles[9][11];

char GameInfo_AreOptionsAvailable(void);

// Finds the stage map files of every rally in the common frontend archive.
// FUNCTION: CMR2 0x004d5ca0
void FrontendMap_FindStageFiles(void)
{
    int counts[9] = { 10, 11, 10, 11, 10, 11, 10, 11, 8 };
    char names[8][4] = { "fin", "gre", "fra", "swe", "aus", "ken", "ita", "uk" };
    void **ppFile;
    int count;
    int i;
    int k;

    // the original builds this path and never uses it
    sprintf(CFrontend::m_stringDest, g_strDmdBfl, CInstallInfo::GetGameCDPath());
    for (i = 0; i < 9; i++) {
        count = counts[i];
        if (count > 0) {
            ppFile = g_dmdFiles[i];
            for (k = 0; k < count; k++) {
                *ppFile = NULL;
                if (i == 8)
                    sprintf(CFrontend::m_stringDest, g_strChaDmdFormat, k + 1);
                else
                    sprintf(CFrontend::m_stringDest, g_strDmdFormat, names[i], k + 1);
                *ppFile = CGenericFileLoader::FindFile(CFrontend::GetCommonFrontendArchive(), CFrontend::m_stringDest, NULL, NULL, 0);
                ppFile++;
            }
        }
    }
    CGame::RegisterCallback((void *)GameInfo_AreOptionsAvailable, NULL);
}

// The four stage-map cell colours used by the map blitter below (index 0 is
// the transparent background).
// GLOBAL: CMR2 0x00524b90
BYTE g_palette0x00524b90[4][4] = { { 0x64, 0x7c, 0xa1, 0xff },
                                   { 0xff, 0xff, 0xff, 0xff },
                                   { 0xff, 0x00, 0x00, 0xff },
                                   { 0x00, 0xff, 0x00, 0xff } };

// Blits the stage map of the given rally (or championship when param_2 is set)
// as a 36x36 grid of 2-bit cells, each cell queued with the colour of its
// value; leaves g_frontendDrawRect scaled to the screen.
// match 42%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// logic verified against the dump; the original keeps the texture in EDI and
// the cell value in BL/param_1 while this build uses EBP/ESI (register
// allocation), so the body is instruction-for-instruction different.
// FUNCTION: CMR2 0x004d5de0
void FrontendMap_DrawStageCellGrid(unsigned int param_1, BYTE param_2)
{
    BYTE *pMap;
    unsigned int which;
    int i;
    int cols;
    int rows;
    BYTE pixel;
    BYTE index;

    g_frontendDrawRect[2] = CFrontend::m_pSmMatrixTexture->width;
    g_frontendDrawRect[3] = CFrontend::m_pSmMatrixTexture->height;
    if (param_2 != 0)
        which = 8;
    else
        which = RallyDataCountryIndex() & 0xff;
    if (g_dmdFiles[which][param_1] != NULL) {
        pMap = (BYTE *)g_dmdFiles[which][param_1];
        rows = 0x24;
        g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 0x4b) / 480;
        do {
            cols = 9;
            g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 0x3c) / 640;
            do {
                pixel = *pMap++;
                for (i = 0; i < 4; i++) {
                    switch (i) {
                    case 0:
                        index = pixel & 3;
                        break;
                    case 1:
                        index = pixel >> 2 & 3;
                        break;
                    case 2:
                        index = pixel >> 4 & 3;
                        break;
                    case 3:
                        index = pixel >> 6;
                        break;
                    }
                    if (index != 0)
                        Sprite_Queue((SpriteRect *)&CFrontend::m_pSmMatrixTexture->field_0x11c,
                                     (SpriteRect *)g_frontendDrawRect, CFrontend::m_pSmMatrixTexture, 4, 0, NULL, NULL,
                                     g_palette0x00524b90[index], 8);
                    g_frontendDrawRect[0] += (int)(g_pGraphics->resX * 8) / 640;
                }
            } while (--cols);
            g_frontendDrawRect[1] += (int)(g_pGraphics->resY * 8) / 480;
        } while (--rows);
    }
}

// FUNCTION: CMR2 0x004d6290
void FrontendMenu_DrawCarClass(Menu *pMenu)
{
    char *text[2];

    FrontendDraw_PlayTime();
    switch (CGameInfo::GetConfiguredGameMode()) {
    case 0:
        text[0] = CFrontend::GetTextString(0xe7);
        text[1] = CFrontend::GetTextString(0xc);
        break;
    case 1:
        text[0] = CFrontend::GetTextString(0xe7);
        text[1] = CFrontend::GetTextString(0xd);
        break;
    case 2:
        text[0] = CFrontend::GetTextString(0xe7);
        text[1] = CFrontend::GetTextString(0xf);
        break;
    case 3:
        text[0] = CFrontend::GetTextString(0xe7);
        text[1] = CFrontend::GetTextString(0x10);
        break;
    case 4:
        text[0] = CFrontend::GetTextString(0xe7);
        text[1] = CFrontend::GetTextString(0x11);
        break;
    case 5:
        text[0] = CFrontend::GetTextString(0x94);
        text[1] = CFrontend::GetTextString(0xc);
        break;
    case 6:
        text[0] = CFrontend::GetTextString(0x94);
        text[1] = CFrontend::GetTextString(0xe2);
        break;
    case 7:
        text[0] = CFrontend::GetTextString(0x94);
        text[1] = CFrontend::GetTextString(0x10);
        break;
    }
    FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, 3, text, 2);
    FrontendDraw_MenuList(pMenu, NULL, -1, -1, 0, 1);
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

// FUNCTION: CMR2 0x004d63e0
void FrontendMenu_DrawMultiplayerCarClass(Menu *pMenu)
{
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, -1, NULL, -1);
    FrontendDraw_MenuList(pMenu, NULL, -1, -1, 0, 1);
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

// Text lines built by the screen code above the title (rally name, date and
// the list of stages); the third is the one built by FrontendMenu_DrawMultiplayerStageSelection.
// Texts of the championship entry selection, each up to the next global.
// GLOBAL: CMR2 0x00818368
char g_unk0x00818368[0x64];
// GLOBAL: CMR2 0x008183cc
char g_unk0x008183cc[0x64];
// Text of the championship entry selection (FrontendMenu_DrawMultiplayerStageSelection:
// heading, profile name and date), up to the next global at 0x8185b8.
// GLOBAL: CMR2 0x00818554
char g_unk0x00818554[0x64];

// Draws the header of the rally-info screen: the framed title rect, the
// "event: date" lines and the list of stages, all scaled to the screen.
// match 83%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004d65c0
void FrontendDraw_RallyInfoHeader(void)
{
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 0x3c) / 640;
    g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 0x4c) / 480;
    g_frontendDrawRect[2] = g_pGraphics->resX - (int)(g_pGraphics->resX * 0x3c) / 640 * 2;
    g_frontendDrawRect[3] = (int)(g_pGraphics->resY * 100) / 480;
    Frontend_DrawRectangleOutline(g_frontendDrawRect, (BYTE *)g_colourText0x0052496c);
    Font_DrawText(0, CFrontend::GetTextString(0xe4), (int)(g_pGraphics->resX * 0x3c) / 640 + 2,
                  (int)(g_pGraphics->resY * 0x4c) / 480 + 1, (int *)g_colourText0x0052496c, 9);
    Font_DrawText(1, g_unk0x008183cc,
                  (int)(g_pGraphics->resX * 0xdc) / 640 - (int)(g_pGraphics->resX * 10) / 640,
                  (int)(g_pGraphics->resY * 0x23) / 480 + (int)(g_pGraphics->resY * 0x4c) / 480,
                  (int *)g_colourText0x0052496c, 0x14);
    Font_DrawText(1, g_unk0x00818554, (int)(g_pGraphics->resX * 0xdc) / 640,
                  (int)(g_pGraphics->resY * 0x23) / 480 + (int)(g_pGraphics->resY * 0x4c) / 480,
                  (int *)g_colourWhite0x00524968, 0x11);
    Font_DrawText(1, g_unk0x00818368, g_pGraphics->resX - (int)(g_pGraphics->resX * 0x3c) / 640 - 6,
                  (int)(g_pGraphics->resY * 0x4c) / 480 + (int)(g_pGraphics->resY * 100) / 480 - 4,
                  (int *)g_colourWhite0x00524968, 0x24);
}

// Time-attack style screen: menu path, the title taken from the id of the
// first item and the list of strings selected by that item, plus the
// underline of the highlighted row.
// FUNCTION: CMR2 0x004d9450
void FrontendMenu_DrawTimeAttackOptions(Menu *pMenu)
{
    short rect[4];
    int y;
    int i;
    int x;
    BYTE *pColour;

    rect[0] = (int)(g_pGraphics->resX * 100) / 640;
    rect[1] = 0;
    rect[2] = CFrontend::m_pAr640ATexture->width;
    rect[3] = CFrontend::m_pAr640ATexture->height;
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, 2, NULL, -1);
    y = ((int)(g_pGraphics->resY * 56) / 480 + (int)(g_pGraphics->resY * 374) / 480) / 2 -
        ((int)(g_pGraphics->resY * 36) / 480 * pMenu->itemCount) / 2;
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 99) / 640;
    g_frontendDrawRect[1] = y;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 282) / 640;
    g_frontendDrawRect[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourShadowWhite0x00524974, 1);
    g_frontendDrawRect[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourWhite0x00524968, 1);
    rect[1] = y + (int)(g_pGraphics->resY * 20) / 480 - CFrontend::m_pAr640ATexture->height / 2;
    Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, (SpriteRect *)rect,
                 CFrontend::m_pAr640ATexture, 1, 0, 0, NULL, g_colourWhite0x00524968, 8);
    strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pMenu->items[0].id));
    x = (int)(g_pGraphics->resX * 122) / 640;
    Font_DrawText(1, CFrontend::m_stringDest, x,
                  (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1],
                  (int *)g_colourWhite0x00524968, 0x11);
    for (i = 0; i < pMenu->items[0].min; i++) {
        pColour = g_colourWhite0x00524968;
        if (pMenu->items[0].max != i)
            pColour = g_colourText0x0052496c;
        x = (int)(g_pGraphics->resX * 10) / 640 + x + (Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest));
        Font_DrawText(1, CFrontend::GetTextString(i + 0x131), x,
                      (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1],
                      (int *)pColour, 0x11);
        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(i + 0x131));
    }
    g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 36) / 480 + y;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourShadowWhite0x00524974, 1);
    g_frontendDrawRect[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourWhite0x00524968, 1);
}

// FUNCTION: CMR2 0x004da630
void FrontendMenu_DrawAlternateRallyStageSelection(Menu *pMenu)
{
    char *text[2];

    switch (CGameInfo::GetConfiguredGameMode()) {
    case 1:
        text[0] = CFrontend::GetTextString(0xe7);
        text[1] = CFrontend::GetTextString(0xd);
        break;
    case 2:
        text[0] = CFrontend::GetTextString(0xe7);
        text[1] = CFrontend::GetTextString(0xf);
        break;
    case 3:
        text[0] = CFrontend::GetTextString(0xe7);
        text[1] = CFrontend::GetTextString(0x10);
        break;
    }
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, 3, text, 2);
    FrontendDraw_MenuList(FrontendMenu_GetAlternateRallyStageSelection(), NULL, -1, -1, 0, 1);
    FrontendDraw_HelpText(CFrontend::GetTextString(0x57), 1);
}

void FrontendDraw_AnimatedMatrixBackground(short x0, short y0, char *pMap);
extern char g_matrixMaps[11][0xd8];

// Rally/route selection screen: the two breadcrumb lines depend on the mode,
// and behind them is the matrix map of the selected route.
// FUNCTION: CMR2 0x004d9ad0
void FrontendMenu_DrawRallySelection(Menu *pMenu)
{
    char *text[2];
    int cellW;

    switch (CGameInfo::GetConfiguredGameMode()) {
    case 1:
        text[0] = CFrontend::GetTextString(0xe7);
        text[1] = CFrontend::GetTextString(0xd);
        break;
    case 2:
        text[0] = CFrontend::GetTextString(0xe7);
        text[1] = CFrontend::GetTextString(0xf);
        break;
    case 3:
        text[0] = CFrontend::GetTextString(0xe7);
        text[1] = CFrontend::GetTextString(0x10);
        break;
    }
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, 3, text, 2);
    FrontendDraw_ScrollerRow(FrontendScroller_GetRallySelectionScroller(), 1);
    cellW = (int)(g_pGraphics->resX * 18) / 640;
    FrontendDraw_AnimatedMatrixBackground((cellW - (int)(g_pGraphics->resX * 11) / 640) / 2 - cellW * 18 / 2 + (int)g_pGraphics->resX / 2,
                 (int)(g_pGraphics->resY * 100) / 480, g_matrixMaps[pMenu->cursor]);
    FrontendDraw_HelpText(CFrontend::GetTextString(0x57), 1);
}

// Network connection list.
// FUNCTION: CMR2 0x004dc7b0
void FrontendMenu_DrawNetworkConnection(Menu *pMenu)
{
    char *text[2];
    int y;
    unsigned int i;
    BYTE *pColour;

    FrontendDraw_PlayTime();
    text[0] = CFrontend::GetTextString(0x12);
    text[1] = CFrontend::GetTextString(0x3c);
    FrontendDraw_Breadcrumb(PATH_X(), PATH_Y(), text, 2);
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
    y = ((int)(g_pGraphics->resY * 8) / 480 + (int)(g_pGraphics->resY * 38) / 480 + (int)(g_pGraphics->resY * 384) / 480) / 2 -
        (int)((int)(g_pGraphics->resY * 20) / 480 * pMenu->items[0].min) / 2;
    for (i = 0; (int)i < pMenu->items[0].min; i++) {
        pColour = g_colourWhite0x00524968;
        if (i != pMenu->items[0].max)
            pColour = g_colourText0x0052496c;
        Font_DrawText(1, CGame::GetConnection(i)->name, (int)g_pGraphics->resX / 2, y, (int *)pColour, 0x12);
        y += (int)(g_pGraphics->resY * 20) / 480;
    }
}

// Draws a settings screen: one row per item (title plus the strings of the
// current value) with a separator line under each, coloured by the cursor.
// match 73%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004e0770
void FrontendMenu_DrawAdvancedGraphicsOptions(Menu *pMenu)
{
    short rect[4];
    MenuItem *pItem;
    BYTE *pColour;
    BYTE *pSelColour;
    BYTE *pUnselColour;
    BYTE *pLineColour;
    BYTE *pLineShadow;
    int y;
    int x;
    int i;
    int j;
    int index;

    rect[0] = (int)(g_pGraphics->resX * 100) / 640;
    rect[1] = 0;
    rect[2] = CFrontend::m_pAr640ATexture->width;
    rect[3] = CFrontend::m_pAr640ATexture->height;
    y = ((int)(g_pGraphics->resY * 8) / 480 + (int)(g_pGraphics->resY * 38) / 480 +
         (int)(g_pGraphics->resY * 384) / 480) / 2 -
        ((int)(g_pGraphics->resY * 36) / 480 * pMenu->itemCount) / 2;
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, 2, NULL, -1);
    if (pMenu->cursor == 0) {
        pLineColour = g_colourWhite0x00524968;
        pLineShadow = g_colourShadowWhite0x00524974;
    } else {
        pLineColour = g_colourText0x0052496c;
        pLineShadow = g_colourShadowText0x00524978;
    }
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 99) / 640;
    g_frontendDrawRect[1] = y;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 282) / 640;
    g_frontendDrawRect[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineShadow, 1);
    g_frontendDrawRect[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineColour, 1);
    pItem = pMenu->items;
    for (i = 0; i < pMenu->itemCount; i++) {
        x = (int)(g_pGraphics->resX * 122) / 640;
        rect[1] = y + (int)(g_pGraphics->resY * 2) / 480 + (int)(g_pGraphics->resY * 18) / 480 +
                  ((int)(g_pGraphics->resY * 36) / 480 * i -
                   CFrontend::m_pAr640ATexture->height / 2);
        if (pMenu->cursor == i) {
            pColour = g_colourWhite0x00524968;
            pSelColour = g_colourWhite0x00524968;
            pUnselColour = g_colourText0x0052496c;
            Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, (SpriteRect *)rect, CFrontend::m_pAr640ATexture, 1, 0, 0,
                         NULL, pColour, 8);
        } else {
            if (pItem->enabled) {
                pColour = g_colourText0x0052496c;
                pSelColour = g_colourWhite0x00524968;
                pUnselColour = g_colourText0x0052496c;
            } else {
                pColour = g_colourDim0x00524970;
                pSelColour = g_colourText0x0052496c;
                pUnselColour = g_colourDim0x00524970;
            }
            Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, (SpriteRect *)rect, CFrontend::m_pAr640DTexture, 1, 0, 0,
                         NULL, pColour, 8);
        }
        switch (pItem->value) {
        case 8:
            strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pItem->id));
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 122) / 640,
                          (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
            for (j = 0; j < pItem->min; j++) {
                if (pItem->max == j)
                    pColour = pSelColour;
                else
                    pColour = pUnselColour;
                x += (int)(g_pGraphics->resX * 10) / 640 +
                     Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
                Font_DrawText(1, CFrontend::GetTextString(j + 0x1b9), x,
                              (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(j + 0x1b9));
            }
            break;
        case 9:
            strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pItem->id));
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 122) / 640,
                          (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
            index = 0x1b3;
            for (j = 0; j < pItem->min; j++) {
                if (j == pItem->max)
                    pColour = pSelColour;
                else
                    pColour = pUnselColour;
                x += (int)(g_pGraphics->resX * 10) / 640 +
                     Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
                Font_DrawText(1, CFrontend::GetTextString(index), x,
                              (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(index));
                index--;
            }
            break;
        case 10:
            strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pItem->id));
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 122) / 640,
                          (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
            for (j = 0; j < pItem->min; j++) {
                if (CFrontend::IsTextureWidthSupported(0x400) && CFrontend::IsTextureHeightSupported(0x400)) {
                    if (pItem->max == j)
                        pColour = pSelColour;
                    else
                        pColour = pUnselColour;
                } else {
                    if (j == 0) {
                        pColour = g_colourDim0x00524970;
                    } else {
                        pColour = g_colourWhite0x00524968;
                    }
                }
                x += (int)(g_pGraphics->resX * 10) / 640 +
                     Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
                Font_DrawText(1, CFrontend::GetTextString(0x1b3 - j), x,
                              (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(0x1b3 - j));
            }
            break;
        case 4:
            strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(0x1d2));
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 122) / 640,
                          (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
            for (j = 0; j < pItem->min; j++) {
                if (pItem->max == j)
                    pColour = pSelColour;
                else
                    pColour = pUnselColour;
                x += (int)(g_pGraphics->resX * 10) / 640 +
                     Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
                switch (j) {
                case 0:
                    index = 0x1d6;
                    break;
                case 1:
                    index = 0x1d5;
                    break;
                default:
                    index = 0x133;
                    break;
                }
                Font_DrawText(1, CFrontend::GetTextString(index), x,
                              (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(index));
            }
            break;
        default:
            Font_DrawText(1, CFrontend::GetTextString(pItem->id), (int)(g_pGraphics->resX * 122) / 640,
                          (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
            break;
        }
        if (pMenu->cursor == i + 1 || pMenu->cursor == i) {
            pLineColour = g_colourWhite0x00524968;
            pLineShadow = g_colourShadowWhite0x00524974;
        } else {
            pLineColour = g_colourText0x0052496c;
            pLineShadow = g_colourShadowText0x00524978;
        }
        g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 36) / 480 * (i + 1) + y;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineShadow, 1);
        g_frontendDrawRect[1]++;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineColour, 1);
        pItem++;
    }
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}
void *FrontendCredits_GetNameTable(void);
int FrontendCredits_GetEntryCount(void);
void *FrontendCredits_GetRoleTable(void);
int FrontendCredits_GetStartTime(void);

// Draws the two columns of text of the scrolling credits screen; when every
// line has scrolled past the top the screen finishes.
// match 86%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004dec30
void FrontendMenu_DrawCredits(Menu *pMenu)
{
    char **ppText;
    unsigned int elapsed;
    bool allOffScreen;
    int i;

    allOffScreen = true;
    ppText = (char **)FrontendCredits_GetNameTable();
    elapsed = (unsigned int)CFrontend::GetFrontendTimestamp();
    elapsed -= FrontendCredits_GetStartTime();
    elapsed /= 30;
    i = 0;
    while (i < FrontendCredits_GetEntryCount()) {
        ((int *)FrontendCredits_GetRoleTable())[i] = ((int)g_pGraphics->resY * 40) / 480 * i - elapsed + 1
            + FrontendCredits_GetScrollOffset() + (int)g_pGraphics->resY + Font_GetLineHeight(2);
        if (((int *)FrontendCredits_GetRoleTable())[i] < (int)g_pGraphics->resY + 100
            && ((int *)FrontendCredits_GetRoleTable())[i] > -100) {
            Font_DrawText(2, ppText[0], (int)g_pGraphics->resX / 2 - 10,
                          ((unsigned short *)FrontendCredits_GetRoleTable())[i * 2],
                          (int *)g_colourWhite0x00524968, 0x14);
            Font_DrawText(2, ppText[1], (int)g_pGraphics->resX / 2 + 10,
                          ((unsigned short *)FrontendCredits_GetRoleTable())[i * 2],
                          (int *)g_colourWhite0x00524968, 0x11);
        }
        if (((int *)FrontendCredits_GetRoleTable())[i] > -10)
            allOffScreen = false;
        i++;
        ppText += 2;
    }
    if (allOffScreen)
        FrontendMenu_EnterCredits(0, 0);
}

// FUNCTION: CMR2 0x004e1890
void FrontendMenu_DrawRenderDeviceOptions(Menu *pMenu)
{
    char *text[1];

    text[0] = CFrontend::GetTextString(0x8e);
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, 2, text, 1);
    FrontendDraw_MenuList(pMenu, NULL, -1, -1, 0, 1);
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

// FUNCTION: CMR2 0x004e1fb0
void FrontendMenu_DrawRallyModes(Menu *pMenu)
{
    char *text[1];

    text[0] = CFrontend::GetTextString(0xd9);
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, 2, text, 1);
    FrontendDraw_MenuList(pMenu, NULL, -1, -1, 0, 1);
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

// FUNCTION: CMR2 0x004e2040
void FrontendMenu_DrawSingleRallyModes(Menu *pMenu)
{
    char *text[2];

    text[0] = CFrontend::GetTextString(0xe7);
    text[1] = CFrontend::GetTextString(0xc);
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, 3, text, 2);
    FrontendDraw_MenuList(pMenu, NULL, -1, -1, 0, 1);
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

// FUNCTION: CMR2 0x004e2ab0
void FrontendMenu_DrawQuitConfirmation(Menu *pMenu)
{
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, 1, NULL, -1);
    FrontendDraw_MenuList(pMenu, CFrontend::GetTextString(0x89), -1, -1, 0, 1);
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

// The flag (a g_matrixMaps index) each entry of the language menu shows:
// English, French, Spanish, Italian, German.
// GLOBAL: CMR2 0x00524008
int g_languageFlagMaps[5] = { 10, 2, 9, 6, 8 };
// 18x12 cell maps (colour indices into g_matrixColours) of the background.
// GLOBAL: CMR2 0x0052401c
char g_matrixMaps[11][0xd8] = {
    0x01, 0x01, 0x01, 0x01, 0x01, 0x06, 0x06, 0x06, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x06,
    0x06, 0x06, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x06, 0x06, 0x06, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x06, 0x06, 0x06, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x01, 0x01, 0x01, 0x01, 0x01, 0x06, 0x06, 0x06, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x06, 0x06, 0x06, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x06,
    0x06, 0x06, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x06, 0x06, 0x06, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x06, 0x06, 0x06, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x06, 0x06, 0x01, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x01, 0x06, 0x06, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x01, 0x06, 0x06, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x06, 0x06, 0x01, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x05,
    0x05, 0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x05,
    0x05, 0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x01, 0x02, 0x04, 0x04, 0x02, 0x04, 0x04, 0x02, 0x01, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x01, 0x01, 0x01, 0x02, 0x01,
    0x01, 0x01, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x01, 0x04, 0x04, 0x04, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x01, 0x04, 0x04, 0x04, 0x04, 0x01, 0x01, 0x01, 0x02, 0x01, 0x01, 0x01, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x01, 0x04,
    0x01, 0x02, 0x04, 0x04, 0x02, 0x04, 0x04, 0x02, 0x01, 0x04, 0x04, 0x04, 0x01, 0x04, 0x04, 0x04, 0x01, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x01, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x01, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x01, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x01, 0x01, 0x01, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x01, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x01, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x01, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x01, 0x02, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x01, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00,
    0x00, 0x02, 0x01, 0x02, 0x00, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x02, 0x02, 0x02, 0x00, 0x00,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00, 0x02, 0x02, 0x01, 0x02, 0x02, 0x00, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x01, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x01, 0x02, 0x01, 0x02, 0x01, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x01, 0x03, 0x03, 0x03, 0x01, 0x03,
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x01, 0x01, 0x04, 0x04, 0x04, 0x01, 0x02, 0x02, 0x01, 0x04, 0x04, 0x04, 0x01, 0x01, 0x02, 0x02, 0x01, 0x02, 0x02, 0x02, 0x01, 0x01,
    0x04, 0x01, 0x02, 0x02, 0x01, 0x04, 0x01, 0x01, 0x02, 0x02, 0x02, 0x01, 0x04, 0x01, 0x01, 0x02, 0x02, 0x02, 0x01, 0x01, 0x02, 0x02, 0x01, 0x01,
    0x02, 0x02, 0x02, 0x01, 0x01, 0x04, 0x04, 0x04, 0x04, 0x01, 0x01, 0x02, 0x02, 0x01, 0x02, 0x02, 0x01, 0x02, 0x02, 0x01, 0x01, 0x04, 0x04, 0x04,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x04, 0x04, 0x04, 0x01, 0x01, 0x02, 0x02, 0x01, 0x02, 0x02, 0x01, 0x02, 0x02, 0x01, 0x01, 0x04, 0x04, 0x04, 0x04, 0x01, 0x01, 0x02, 0x02, 0x02,
    0x01, 0x01, 0x02, 0x02, 0x01, 0x01, 0x02, 0x02, 0x02, 0x01, 0x01, 0x04, 0x01, 0x02, 0x02, 0x02, 0x01, 0x01, 0x04, 0x01, 0x02, 0x02, 0x01, 0x04,
    0x01, 0x01, 0x02, 0x02, 0x02, 0x01, 0x02, 0x02, 0x01, 0x01, 0x04, 0x04, 0x04, 0x01, 0x02, 0x02, 0x01, 0x04, 0x04, 0x04, 0x01, 0x01, 0x02, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
};

void FrontendDraw_AnimatedMatrixBackground(short x0, short y0, char *pMap);

// Text of the network waiting-room screen (0x4e2b40).
// GLOBAL: CMR2 0x00524e10
char g_str0x00524e10[8] = "Back";
// GLOBAL: CMR2 0x00524e18
char g_str0x00524e18[8] = "Start";
// GLOBAL: CMR2 0x00524e24
char g_str0x00524e24[16] = "Waiting Room";
// GLOBAL: CMR2 0x00524e34
char g_str0x00524e34[8] = "Network";
// GLOBAL: CMR2 0x00524e3c
char g_str0x00524e3c[20] = "Peugeot 205 T16, MT";
// GLOBAL: CMR2 0x00524e50
char g_str0x00524e50[8] = "SteppyS";
// GLOBAL: CMR2 0x00524e58
char g_str0x00524e58[24] = "Subaru Impreza WRC, AT";
// GLOBAL: CMR2 0x00524e70
char g_str0x00524e70[8] = "LeeM";
// GLOBAL: CMR2 0x00524e78
char g_str0x00524e78[8] = "Setup";
// GLOBAL: CMR2 0x00524e80
char g_str0x00524e80[16] = "Spectator: No";
// GLOBAL: CMR2 0x00524e90
char g_str0x00524e90[24] = "Transmission: Automatic";
// GLOBAL: CMR2 0x00524ea8
char g_str0x00524ea8[24] = "Car: Ford Focus 2000";
// GLOBAL: CMR2 0x00524ec0
char g_str0x00524ec0[16] = "Blah blah blah";
// GLOBAL: CMR2 0x00524ed0
char g_str0x00524ed0[24] = "SteppyS: Bimble bomble";
// GLOBAL: CMR2 0x00524ee8
char g_str0x00524ee8[24] = "LeeM : Wheeeeeeeeeeeeee";
// GLOBAL: CMR2 0x00524f00
char g_str0x00524f00[16] = "JamieL: Bananas";

// Parameter block of FrontendDraw_RallyEntryList: two flag bytes, the entry count, the
// selected entry and the string pointer array at +0x14.
struct Unk0x4e2b40 {
    BYTE field_0x0[6];
    BYTE field_0x6;
    BYTE field_0x7;
    BYTE field_0x8[2];
    BYTE count;
    BYTE selected;
    BYTE field_0xc[8];
    char *strings[6];
};

// Draw callback of the network waiting-room screen: the breadcrumb, the play
// time, the three peer names of the sample room and the player list.
// match 83%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// logic verified against the dump; the remaining diff is the 16-bit coordinate
// arithmetic the original gets at the Font_DrawText call sites (Font_DrawText
// is declared with int x/unsigned int y here, the original used narrower
// parameters) plus a couple of register choices.
// match 89%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004e2b40
void FrontendMenu_DrawNetworkWaitingRoom(Menu *pMenu)
{
    Unk0x4e2b40 list;
    short rowHeight;

    list.strings[0] = g_str0x00524e34;
    list.strings[1] = g_str0x00524e24;
    FrontendDraw_Breadcrumb(PATH_X(), PATH_Y(), list.strings, 2);
    FrontendDraw_PlayTime();
    g_frontendDrawRect[0] = 0x1e;
    g_frontendDrawRect[1] = 0x46;
    g_frontendDrawRect[2] = 0x190;
    g_frontendDrawRect[3] = 0x96;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourShadowText0x00524978, 4);
    Font_DrawText(1, g_str0x00524f00, g_frontendDrawRect[0] + 10, g_frontendDrawRect[1] + 10,
                  (int *)g_colourWhite0x00524968, 9);
    Font_DrawText(1, g_str0x00524ee8, g_frontendDrawRect[0] + 10, g_frontendDrawRect[1] + 0x1e,
                  (int *)g_colourWhite0x00524968, 9);
    Font_DrawText(1, g_str0x00524ed0, g_frontendDrawRect[0] + 10, g_frontendDrawRect[1] + 0x32,
                  (int *)g_colourWhite0x00524968, 9);
    rowHeight = g_frontendDrawRect[3] + 10;
    g_frontendDrawRect[2] = 0x190;
    g_frontendDrawRect[1] += rowHeight;
    g_frontendDrawRect[3] = 0x14;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourShadowText0x00524978, 4);
    Font_DrawText(1, g_str0x00524ec0, g_frontendDrawRect[0] + 10, g_frontendDrawRect[1],
                  (int *)g_colourWhite0x00524968, 9);
    list.selected = 2;
    list.count = 6;
    list.field_0x6 |= 3;
    list.field_0x7 = 3;
    list.strings[0] = g_str0x00524ea8;
    list.strings[1] = g_str0x00524e90;
    list.strings[2] = g_str0x00524e80;
    list.strings[3] = g_str0x00524e18;
    list.strings[4] = g_str0x00524e78;
    list.strings[5] = g_str0x00524e10;
    FrontendDraw_RallyEntryList((BYTE *)&list, NULL, 0x172, list.strings);
    Font_DrawText(1, g_str0x00524e70, 0x1b8, 0x46, (int *)g_colourWhite0x00524968, 9);
    Font_DrawText(1, g_str0x00524e58, 0x1b8, 0x5a, (int *)g_colourWhite0x00524968, 9);
    Font_DrawText(1, g_str0x00524e50, 0x1b8, 0x78, (int *)g_colourWhite0x00524968, 9);
    Font_DrawText(1, g_str0x00524e3c, 0x1b8, 0x8c, (int *)g_colourWhite0x00524968, 9);
}

// Draw callback of the main menu: the menu scroller and, behind it, the
// background matrix picture of the selected entry.
// FUNCTION: CMR2 0x004e2da0
void FrontendMenu_DrawLanguage(Menu *pMenu)
{
    int cellW;

    FrontendDraw_ScrollerRow(FrontendScroller_GetLanguageScroller(), 1);
    cellW = (int)(g_pGraphics->resX * 18) / 640;
    FrontendDraw_AnimatedMatrixBackground((cellW - (int)(g_pGraphics->resX * 11) / 640) / 2 - cellW * 18 / 2 + (int)g_pGraphics->resX / 2,
                 (int)(g_pGraphics->resY * 100) / 480, g_matrixMaps[g_languageFlagMaps[pMenu->cursor]]);
}

// FUNCTION: CMR2 0x004e3230
void FrontendMenu_DrawRecordsRoot(Menu *pMenu)
{
    char *text[4];

    if (g_recordsProfileMode != 0) {
        text[0] = CFrontend::GetTextString(0xb);
        text[1] = CFrontend::GetTextString(0x59);
        text[2] = (char *)RallyData_GetRecord(0);
        text[3] = CFrontend::GetTextString(pMenu->field_0x4);
        FrontendDraw_Breadcrumb(PATH_X(), PATH_Y(), text, 4);
    } else {
        FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, 2, NULL, -1);
    }
    FrontendDraw_PlayTime();
    FrontendDraw_MenuList(pMenu, NULL, -1, -1, 0, 1);
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

extern char g_strTwoDigits[8];
extern char g_strGearboxManual[4];
extern char g_strGearboxAuto[4];
extern char g_loadRecordTimeFormat[];
extern BYTE g_colourTitle0x00524984[4];

// Draw callback of the stage record pages: title of the table shown (cycled by
// FrontendRecords_CycleTableMode), the column headers and the five entries (position, stage,
// surface, gearbox and record time).
// FUNCTION: CMR2 0x004e4130
void FrontendMenu_DrawStageRecords(Menu *pMenu)
{
    GameInfo0xa4SubStruct12 *pEntry;
    int table;
    int y0;
    int y;
    int row;

    table = FrontendRecords_GetTableMode();
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, 2, NULL, -1);
    switch (table) {
    case 0:
        Font_DrawText(2, CFrontend::GetTextString(0x167), (int)g_pGraphics->resX / 2, (int)(g_pGraphics->resY * 100) / 480,
                      (int *)g_colourWhite0x00524968, 10);
        break;
    case 1:
        Font_DrawText(2, CFrontend::GetTextString(0x168), (int)g_pGraphics->resX / 2, (int)(g_pGraphics->resY * 100) / 480,
                      (int *)g_colourWhite0x00524968, 10);
        break;
    case 2:
        Font_DrawText(2, CFrontend::GetTextString(0x169), (int)g_pGraphics->resX / 2, (int)(g_pGraphics->resY * 100) / 480,
                      (int *)g_colourWhite0x00524968, 10);
        break;
    }
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 30) / 640;
    g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 200) / 480 - (int)(g_pGraphics->resY * 25) / 480;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 640) / 640 + (int)(g_pGraphics->resX * 30) / 640 * -2;
    g_frontendDrawRect[3] = (int)(g_pGraphics->resY * 25) / 480;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourShadowText0x00524978, 4);
    y0 = (int)(g_pGraphics->resY * 19) / 480 + ((int)(g_pGraphics->resY * 200) / 480 - (int)(g_pGraphics->resY * 25) / 480);
    Font_DrawText(0, CFrontend::GetTextString(0x16a), (int)(g_pGraphics->resX * 140) / 640, y0, (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16b), (int)(g_pGraphics->resX * 270) / 640, y0, (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16c), (int)(g_pGraphics->resX * 395) / 640, y0, (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16d), (int)(g_pGraphics->resX * 445) / 640, y0, (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x174), (int)(g_pGraphics->resX * 520) / 640, y0, (int *)g_colourTitle0x00524984, 0x12);
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 30) / 640;
    g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 200) / 480;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 640) / 640 + (int)(g_pGraphics->resX * 30) / 640 * -2;
    g_frontendDrawRect[3] = 1;
    for (row = 0; row < 5; row++) {
        y = (int)(g_pGraphics->resY * 19) / 480 + (int)(g_pGraphics->resY * 200) / 480 + (int)(g_pGraphics->resY * 25) / 480 * row;
        pEntry = &CGameInfo::GetGameInfoFieldA4Address()->secondLoop[15 * pMenu->cursor + 5 * table + row];
        sprintf(CFrontend::m_stringDest, g_strTwoDigits, row + 1);
        Font_DrawText(3, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 100) / 640, y, (int *)g_colourText0x0052496c, 0x12);
        Font_DrawText(1, pEntry->ident, (int)(g_pGraphics->resX * 140) / 640, y, (int *)g_colourWhite0x00524968, 0x12);
        Font_DrawText(1, CFrontend::GetModeSpecificCountryText(pEntry->flags & 0x3f), (int)(g_pGraphics->resX * 270) / 640, y,
                      (int *)g_colourWhite0x00524968, 0x12);
        if ((pEntry->flags & 0x40) != 0)
            Font_DrawText(1, g_strGearboxAuto, (int)(g_pGraphics->resX * 395) / 640, y,
                          (int *)g_colourWhite0x00524968, 0x12);
        else
            Font_DrawText(1, g_strGearboxManual, (int)(g_pGraphics->resX * 395) / 640, y,
                          (int *)g_colourWhite0x00524968, 0x12);
        sprintf(CFrontend::m_stringDest, g_strTwoDigits, (pEntry->flags >> 7 & 0xf) + 1);
        Font_DrawText(3, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 445) / 640, y, (int *)g_colourWhite0x00524968, 0x12);
        sprintf(CFrontend::m_stringDest, g_loadRecordTimeFormat, pEntry->value / 6000, pEntry->value / 100 % 60, pEntry->value % 100);
        Font_DrawText(3, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 520) / 640, y, (int *)g_colourWhite0x00524968, 0x12);
        g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 200) / 480) + (short)((int)(g_pGraphics->resY * 25) / 480) * ((short)row + 1);
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourText0x0052496c, 1);
    }
    FrontendDraw_ScrollerRow(FrontendScroller_GetStageRecordsScroller(), 1);
    FrontendDraw_HelpText(CFrontend::GetTextString(0x16f), 1);
}

// FUNCTION: CMR2 0x004e7770
void FrontendProfile_SetHeaderMode(int value)
{
    g_profileHeaderPlayerNumber = value;
}

// GLOBAL: CMR2 0x00818274
char g_profileHeaderText[0x40];

// FUNCTION: CMR2 0x004e7780
void FrontendProfile_SetHeaderText(const char *text)
{
    strcpy(g_profileHeaderText, text);
}

// FUNCTION: CMR2 0x004e77b0
void FrontendProfile_SetNameEntryFlag(BYTE value)
{
    g_profileNameEntryFlag = value;
}

// Network game: the name being typed, with a blinking cursor.
extern char g_strLabelSpacedText[];
extern char g_strTextCursor[];

// FUNCTION: CMR2 0x004e9820
void FrontendMenu_DrawJoinPasswordEntry(Menu *pMenu)
{
    char *text[3];

    text[0] = CFrontend::GetTextString(0);
    text[1] = CFrontend::GetTextString(0x12);
    text[2] = CFrontend::GetTextString(0x1e7);
    FrontendDraw_PlayTime();
    FrontendDraw_Breadcrumb(PATH_X(), PATH_Y(), text, 3);
    Font_DrawText(1, CFrontend::GetTextString(0x1e8), (int)g_pGraphics->resX / 2, (int)g_pGraphics->resY / 4,
                  (int *)g_colourWhite0x00524968, 0x12);
    sprintf(CFrontend::m_stringDest, g_strLabelSpacedText, CFrontend::GetTextString(0x1c3), g_joinPassword);
    if ((int)CMain::GetFrameDelta() % 20 > 9)
        strcat(CFrontend::m_stringDest, g_strTextCursor);
    Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 260) / 640, (int)g_pGraphics->resY / 2,
                  (int *)g_colourWhite0x00524968, 0x11);
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}


Menu *FrontendMenu_GetInitialMenu(BYTE param1);
Menu *FrontendMenu_GetOptions(void);
Menu *FrontendMenu_GetLanguage(void);
Menu *FrontendMenu_GetMain(void);
Menu *FrontendMenu_GetNetworkSessionBrowser(void);
Menu *FrontendMenu_GetNetworkSessionSetup(void);
Menu *FrontendMenu_GetRallyModes(void);
Menu *FrontendMenu_GetNetworkPlayerSetup(void);
Menu *FrontendMenu_GetNetworkCarSetup(void);
Menu *FrontendMenu_GetNetworkRallySetup(void);
Menu *FrontendMenu_GetNetworkStageSetup(void);
Menu *FrontendMenu_GetNetworkExtendedStageSetup(void);
Menu *FrontendMenu_GetNetworkStageTimes(void);
void FrontendMenu_BuildNetworkWaitingRoom(void);
void FrontendMenu_BuildLanguage(void);
void FrontendMenu_BuildMain(void);
void FrontendMenu_BuildRallyProfile(void);
void FrontendMenu_BuildRallyProfileDetails(void);
void FrontendMenu_BuildProfileActions(void);
void FrontendMenu_BuildProfileSettings(void);
void FrontendMenu_BuildProfileRecordsSummary(void);
void FrontendMenu_BuildRallyModes(void);
void FrontendMenu_BuildSingleRallyModes(void);
void FrontendMenu_BuildSingleRallySelection(void);
void FrontendMenu_BuildRallyStartTransition(void);
void FrontendMenu_BuildNetworkConnection(void);
void FrontendMenu_BuildNetworkSessionBrowser(void);
void FrontendMenu_BuildNetworkSessionDetails(void);
void FrontendMenu_BuildNetworkSessionSetup(void);
void FrontendMenu_BuildNetworkStageTimes(void);
void FrontendMenu_BuildOptions(void);
void FrontendMenu_BuildCheats(void);
void FrontendMenu_BuildGameOptions(void);
void FrontendMenu_BuildGraphicsOptions(void);
void FrontendMenu_BuildAdvancedGraphicsOptions(void);
void FrontendMenu_BuildRenderDeviceOptions(void);
void FrontendMenu_BuildDisplayDevice(void);
void FrontendMenu_BuildDisplayMode(void);
void FrontendMenu_BuildSoundOptions(void);
void FrontendMenu_BuildRecords(void);
void FrontendMenu_BuildProfileRecords(void);
void FrontendMenu_BuildHighScores(void);
void FrontendMenu_BuildStageRecords(void);
void FrontendMenu_BuildBestStageTimes(void);
void FrontendMenu_BuildTransmissionRecords(void);
void FrontendMenu_BuildChampionshipRecords(void);
void FrontendMenu_BuildProfileHighScores(void);
void FrontendMenu_BuildProfileStageRecords(void);
void FrontendMenu_BuildProfileBestStageTimes(void);
void FrontendMenu_BuildProfileTransmissionRecords(void);
void FrontendMenu_BuildProfileChampionshipRecords(void);
void FrontendMenu_BuildCredits(void);
void FrontendMenu_BuildQuitConfirmation(void);
void FrontendMenu_BuildSingleRallyDifficulty(char difficulty);
void FrontendMenu_BuildChampionshipDifficulty(char difficulty);
void FrontendMenu_BuildTimeTrialDifficulty(char difficulty);
void FrontendMenu_BuildMultiplayerDifficulty(char difficulty);
void FrontendMenu_BuildMultiplayerExtendedDifficulty(char difficulty);
void FrontendMenu_BuildSingleRallyCarClass(int unused);
void FrontendMenu_BuildChampionshipCarClass(int unused);
void FrontendMenu_BuildTimeTrialCarClass(int unused);
void FrontendMenu_BuildMultiplayerCarClass(void);
void FrontendMenu_BuildSingleRallyTransmission(char choice);
void FrontendMenu_BuildChampionshipTransmission(char choice);
void FrontendMenu_BuildTimeTrialTransmission(char choice);
void FrontendMenu_BuildMultiplayerProfile(void);
void FrontendMenu_BuildMultiplayerStageSelection(void);
void FrontendMenu_BuildProfileNameEntry(void);
void FrontendMenu_BuildProfileRenameEntry(void);
void FrontendMenu_BuildProfileDateEntry(void);
void FrontendMenu_BuildCarSetup(void);
void FrontendMenu_BuildPaletteSelection(void);
void FrontendMenu_BuildPaletteConfirmation(void);
void FrontendMenu_BuildRallySelection(void);
void FrontendMenu_BuildRallyStageSelection(void);
void FrontendMenu_BuildAlternateRallyStageSelection(void);
void FrontendMenu_BuildMultiplayerRaceSettings(void);
void FrontendMenu_BuildQuitTransition(void);
void FrontendMenu_BuildNetworkPlayerSetup(void);
void FrontendMenu_BuildNetworkCarSetup(void);
void FrontendMenu_BuildNetworkRallySetup(void);
void FrontendMenu_BuildNetworkStageSetup(void);
void FrontendMenu_BuildNetworkExtendedStageSetup(void);
void FrontendMenu_BuildJoinPasswordEntry(void);
void FrontendMenu_BuildNetworkLeaderboard(void);
void FrontendMenu_BuildNetworkMessage(void);
void FrontendMenu_BuildArcade(void);
void FrontendMenu_BuildArcadeChampionshipPlayerCount(char players);
void FrontendMenu_BuildArcadePlayerCount(char players);
void FrontendMenu_BuildArcadeGameType(char unused);
void FrontendMenu_BuildArcadeCarClass(int unused);
void FrontendMenu_BuildChampionshipRouteSelection(void);
void FrontendMenu_BuildChampionshipEntry(void);
void FrontendMenu_BuildAlternateChampionshipRouteSelection(void);
void FrontendMenu_BuildAlternateChampionshipEntry(void);
void FrontendMenu_BuildArcadeChampionshipRouteSelection(void);
void FrontendMenu_BuildArcadeChampionshipEntry(void);
void FrontendMenu_BuildArcadeChampionshipTransmission(void);
void FrontendMenu_BuildArcadeRallySelection(void);
void FrontendMenu_BuildArcadeChampionshipRallySelection(void);
void FrontendMenu_BuildQuickRaceSettings(void);
void FrontendMenu_BuildQuickRaceAdvancedSettings(void);
void FrontendMenu_BuildAllControlsPages(void);
BYTE Game_GetSecondaryOptionStateByte(void);
void Input_ClearCharacterQueue(void);
void Menu_QueueDefaultAction(void);
void Menu_SetInputStateFlag(char param1);
void Frontend_SelectTextLanguage(int language);
BOOL Network_GetSessionStateFlag(void);
void GameInfo_SetSoundOptionBit22(BYTE param1);
void FrontendScroller_SetSelection(int value);
extern BYTE g_unk0x00818ac4;
extern int g_menuEnterTime;

// GLOBAL: CMR2 0x0052f3d8
int g_unk0x0052f3d8;
// Name of the demo profile used by quick race.
// GLOBAL: CMR2 0x00519ec4
char g_strDemoName0x00519ec4[4] = "DEM";

void Profile_AssignAvailableCategory(int index, int profile);
void Profile_ResetCategoryData(int index);
void RallyData_SetPlayerProfileInUse(BYTE index, char set);
void RallyData_SetEditedDriverOrCategoryName(unsigned int slot, char *pName);
void RallyData_SetSetupFlag11(char param1);
void RallyData_SetStageSelectionAndRefreshFlags(BYTE param1);
void RallyData_FillEventSlotSelections(void);
void RallyData_SetDriverCategoryOption(BYTE index, BYTE value);
void RallyData_SetDriverAutomaticGearbox(BYTE index, BYTE flag);
void GameInfo_SetConfiguredPlayerCount(BYTE param1);
void GameInfo_SetConfiguredDifficulty(BYTE param1);
void GameInfo_SetConfiguredGameMode(BYTE param1);

// Starts a fresh quick-race/championship session: resets the timing and rally
// data and selects the difficulty-dependent defaults.
// FUNCTION: CMR2 0x004e9e40
void FrontendSession_InitFreshRace(void)
{
    int values[6] = { 5, 3, 2, 2, 0, 0 };
    int index;

    index = g_unk0x0052f3d8;
    g_unk0x0052f3d8 = (g_unk0x0052f3d8 + 1) % 4;
    Profile_AssignAvailableCategory(0, -1);
    Profile_ResetCategoryData(0);
    RallyData_SetPlayerProfileInUse(0, 1);
    RallyData_SetEditedDriverOrCategoryName(0, g_strDemoName0x00519ec4);
    RallyData_SetDriverCategoryOption(0, 0);
    RallyData_SetDriverAutomaticGearbox(0, 1);
    CGameInfo::SaveFrontendOptionSettings();
    if (index == 3) {
        GameInfo_SetConfiguredGameMode(6);
        GameInfo_SetConfiguredPlayerCount(1);
        GameInfo_SetConfiguredDifficulty(2);
        RallyData_SetSetupFlag11(0);
        RallyData_SetSecondarySelectionNibble(5);
        RallyData_SetSelectionBits10To11(0);
        RallyData_SetSelectionBits12To13(0);
        RallyData_SetSelectionBits16To19(1);
        RallyData_FillEventSlotSelections();
        RallyData_SetCountrySelectionBits(5);
        RallyData_SetStageSelectionAndRefreshFlags(2);
        return;
    }
    GameInfo_SetConfiguredGameMode(2);
    RallyData_SetDriverPairingStateValues(1, 0);
    GameInfo_SetConfiguredPlayerCount(1);
    GameInfo_SetConfiguredDifficulty(1);
    RallyData_SetSetupFlag11(0);
    RallyData_SetSelectionBits16To19(1);
    RallyData_SetCountrySelectionBits((BYTE)values[index]);
    RallyData_SetStageSelectionAndRefreshFlags((BYTE)values[index + 3]);
}

// Builds every frontend menu and picks the first one: after a network game
// the network lobby; otherwise the language menu (first run) or the main
// menu, and when coming back from a race (`back`) the menus of the game
// mode that was played, with their cursors.
// FUNCTION: CMR2 0x004e9f70
void FrontendMenu_BuildPagesAndSelectInitial(BYTE param1, BYTE back)
{
    Menu *pMenu;
    unsigned int region;

    if (CGameInfo::GetGameModeOptionBit19() != 0) {
        if (Network_GetSessionStateFlag() != 0) {
            switch (CGameInfo::GetConfiguredGameMode()) {
            case 8:
                pMenu = FrontendMenu_GetNetworkPlayerSetup();
                break;
            case 9:
                pMenu = FrontendMenu_GetNetworkCarSetup();
                break;
            case 10:
                pMenu = FrontendMenu_GetNetworkRallySetup();
                break;
            case 11:
                pMenu = FrontendMenu_GetNetworkStageSetup();
                break;
            case 12:
                pMenu = FrontendMenu_GetNetworkExtendedStageSetup();
                break;
            default:
                goto lobby;
            }
        } else {
            pMenu = FrontendMenu_GetNetworkSessionBrowser();
        }
        FrontendMenu_GetNetworkSessionSetup()->pParent = pMenu;
    lobby:
        g_pMenu0x00818abc = NULL;
        if (Game_GetSecondaryOptionStateByte() != 0) {
            g_pMenu0x00818ac0 = FrontendMenu_GetNetworkStageTimes();
        } else {
            g_pMenu0x00818ac0 = FrontendMenu_GetNetworkSessionSetup();
            FrontendMenu_GetNetworkSessionSetup()->items[Menu_FindItem(FrontendMenu_GetNetworkSessionSetup(), 3)].max = 1;
        }
        Input_ClearCharacterQueue();
        Frontend_SelectTextLanguage(CGameInfo::GetGameLanguage());
        return;
    }
    g_pMenu0x00818abc = NULL;
    g_pMenu0x00818ac0 = FrontendMenu_GetInitialMenu(param1);
    g_menuEnterTime = Sys_GetTicks();
    if (GameInfo_AreOptionsAvailable() == 0 && CGameInfo::GetRecordFlagsWord(0) > 180000)
        GameInfo_SetSoundOptionBit22(1);
    Menu_QueueDefaultAction();
    Menu_SetInputStateFlag(0);
    CInput::SetMenuSounds(-1, -1, -1, -1, -1);
    FrontendMenu_BuildMain();
    g_unk0x00818ac4 = 0;
    FrontendMenu_BuildNetworkWaitingRoom();
    FrontendMenu_BuildLanguage();
    region = CGameInfo::GetGameRegion();
    if (region == 2 || region == 3) {
        Frontend_SelectTextLanguage(0);
        FrontendMenu_BuildAllControlsPages();
    } else if (param1 != 0) {
        Frontend_SelectTextLanguage(0);
    } else {
        Frontend_SelectTextLanguage(CGameInfo::GetGameLanguage());
        Menu_SetParent(FrontendMenu_GetLanguage(), FrontendMenu_GetOptions());
        FrontendMenu_GetLanguage()->items[0].pSubMenu = FrontendMenu_GetOptions();
        FrontendMenu_GetLanguage()->items[1].pSubMenu = FrontendMenu_GetOptions();
        FrontendMenu_GetLanguage()->items[2].pSubMenu = FrontendMenu_GetOptions();
        FrontendMenu_GetLanguage()->items[3].pSubMenu = FrontendMenu_GetOptions();
        FrontendMenu_GetLanguage()->items[4].pSubMenu = FrontendMenu_GetOptions();
        Menu_SetCursor(FrontendMenu_GetLanguage(), CGameInfo::GetGameLanguage());
    }
    FrontendMenu_BuildRallyProfile();
    FrontendMenu_BuildRallyProfileDetails();
    FrontendMenu_BuildProfileActions();
    FrontendMenu_BuildProfileSettings();
    FrontendMenu_BuildProfileRecordsSummary();
    FrontendMenu_BuildRallyModes();
    FrontendMenu_BuildSingleRallyModes();
    FrontendMenu_BuildSingleRallySelection();
    FrontendMenu_BuildRallyStartTransition();
    FrontendMenu_BuildArcade();
    FrontendMenu_BuildArcadeChampionshipPlayerCount(CGameInfo::GetConfiguredPlayerCount());
    FrontendMenu_BuildArcadePlayerCount(CGameInfo::GetConfiguredPlayerCount());
    FrontendMenu_BuildArcadeGameType(CGameInfo::GetConfiguredDifficulty());
    FrontendMenu_BuildArcadeCarClass(CGameInfo::GetConfiguredDifficulty());
    FrontendMenu_BuildChampionshipRouteSelection();
    FrontendMenu_BuildChampionshipEntry();
    FrontendMenu_BuildAlternateChampionshipRouteSelection();
    FrontendMenu_BuildAlternateChampionshipEntry();
    FrontendMenu_BuildArcadeChampionshipRouteSelection();
    FrontendMenu_BuildArcadeChampionshipEntry();
    FrontendMenu_BuildArcadeChampionshipTransmission();
    FrontendMenu_BuildArcadeRallySelection();
    FrontendMenu_BuildArcadeChampionshipRallySelection();
    FrontendMenu_BuildNetworkConnection();
    FrontendMenu_BuildNetworkSessionBrowser();
    FrontendMenu_BuildNetworkSessionDetails();
    FrontendMenu_BuildNetworkSessionSetup();
    FrontendMenu_BuildNetworkStageTimes();
    FrontendMenu_BuildNetworkPlayerSetup();
    FrontendMenu_BuildNetworkCarSetup();
    FrontendMenu_BuildNetworkRallySetup();
    FrontendMenu_BuildNetworkStageSetup();
    FrontendMenu_BuildNetworkExtendedStageSetup();
    FrontendMenu_BuildNetworkMessage();
    FrontendMenu_BuildJoinPasswordEntry();
    FrontendMenu_BuildNetworkLeaderboard();
    FrontendMenu_BuildOptions();
    FrontendMenu_BuildGameOptions();
    FrontendMenu_BuildGraphicsOptions();
    FrontendMenu_BuildScreenOptions();     // PORT
    FrontendMenu_BuildInputControls();     // PORT
    FrontendMenu_BuildRenderDeviceOptions();
    FrontendMenu_BuildAdvancedGraphicsOptions();
    FrontendMenu_BuildDisplayDevice();
    FrontendMenu_BuildDisplayMode();
    FrontendMenu_BuildSoundOptions();
    FrontendMenu_BuildRecords();
    FrontendMenu_BuildHighScores();
    FrontendMenu_BuildStageRecords();
    FrontendMenu_BuildBestStageTimes();
    FrontendMenu_BuildTransmissionRecords();
    FrontendMenu_BuildChampionshipRecords();
    FrontendMenu_BuildProfileRecords();
    FrontendMenu_BuildProfileHighScores();
    FrontendMenu_BuildProfileStageRecords();
    FrontendMenu_BuildProfileBestStageTimes();
    FrontendMenu_BuildProfileTransmissionRecords();
    FrontendMenu_BuildProfileChampionshipRecords();
    if (param1 == 0)
        FrontendMenu_BuildAllControlsPages();
    FrontendMenu_BuildCheats();
    FrontendMenu_BuildCredits();
    FrontendMenu_BuildQuitConfirmation();
    FrontendMenu_BuildSingleRallyDifficulty(CGameInfo::GetConfiguredPlayerCount());
    FrontendMenu_BuildChampionshipDifficulty(CGameInfo::GetConfiguredPlayerCount());
    FrontendMenu_BuildTimeTrialDifficulty(CGameInfo::GetConfiguredPlayerCount());
    FrontendMenu_BuildMultiplayerDifficulty(CGameInfo::GetConfiguredPlayerCount());
    FrontendMenu_BuildMultiplayerExtendedDifficulty(CGameInfo::GetConfiguredPlayerCount());
    FrontendMenu_BuildSingleRallyCarClass(CGameInfo::GetConfiguredDifficulty());
    FrontendMenu_BuildChampionshipCarClass(CGameInfo::GetConfiguredDifficulty());
    FrontendMenu_BuildTimeTrialCarClass(CGameInfo::GetConfiguredDifficulty());
    FrontendMenu_BuildSingleRallyTransmission(CGameInfo::IsConfiguredMultiplayer());
    FrontendMenu_BuildChampionshipTransmission(CGameInfo::IsConfiguredMultiplayer());
    FrontendMenu_BuildTimeTrialTransmission(CGameInfo::IsConfiguredMultiplayer());
    FrontendMenu_BuildMultiplayerProfile();
    FrontendMenu_BuildMultiplayerStageSelection();
    FrontendMenu_BuildProfileNameEntry();
    FrontendMenu_BuildProfileRenameEntry();
    FrontendMenu_BuildProfileDateEntry();
    FrontendMenu_BuildCarSetup();
    FrontendMenu_BuildPaletteSelection();
    FrontendMenu_BuildPaletteConfirmation();
    FrontendMenu_BuildRallySelection();
    FrontendMenu_BuildRallyStageSelection();
    FrontendMenu_BuildAlternateRallyStageSelection();
    FrontendMenu_BuildQuickRaceSettings();
    FrontendMenu_BuildQuickRaceAdvancedSettings();
    FrontendMenu_BuildMultiplayerCarClass();
    FrontendMenu_BuildMultiplayerRaceSettings();
    FrontendMenu_BuildQuitTransition();
    if (back != 0) {
        FrontendMenu_GetMain()->cursor = 1;
        FrontendScroller_SetSelection(1);
        FrontendMenu_GetRallyModes()->cursor = 2;
        FrontendMenu_GetArcade()->cursor = 1;
        CFrontend::CacheCarClassTextLabels();
        return;
    }
    switch (CGameInfo::GetConfiguredGameMode()) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
        FrontendMenu_GetRallyModes()->cursor = CGameInfo::GetConfiguredGameMode();
        FrontendMenu_GetMain()->cursor = 1;
        FrontendScroller_SetSelection(1);
        FrontendMenu_GetArcade()->cursor = 1;
        CFrontend::CacheCarClassTextLabels();
        return;
    case 5:
    case 6:
    case 7:
        FrontendMenu_GetArcade()->cursor = CGameInfo::GetConfiguredGameMode() - 5;
        FrontendMenu_GetMain()->cursor = 2;
        FrontendScroller_SetSelection(2);
        FrontendMenu_GetRallyModes()->cursor = 2;
        CFrontend::CacheCarClassTextLabels();
        return;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
        FrontendMenu_GetMain()->cursor = 3;
        FrontendScroller_SetSelection(3);
        break;
    }
    CFrontend::CacheCarClassTextLabels();
}

// FUNCTION: CMR2 0x004ea470
void FrontendMenu_ResetActiveMenu(void)
{
    g_pMenu0x00818abc = g_pMenu0x00818ac0;
}

extern int g_frontendOverlayMode;
void Input_TranslatePedalsToMenuKeys(void);
void Input_MergeAssignedJoystickButtons(int slot, DeviceInfo *pOut);
unsigned short Input_GetControllerSlotMapping(unsigned short slot);
void Sound_UpdateMusicStreaming(void);

// Time (CFrontend::GetFrontendTimestamp) at which the current menu was entered.
// GLOBAL: CMR2 0x008189b8
int g_menuEnterTime;

// Frontend per-frame update: reads the input, keeps the music streaming and
// runs the current menu, switching to the menu it returns.
// FUNCTION: CMR2 0x004ea510
void FrontendMenu_UpdateAndSwitchActive(void)
{
    DeviceInfo *pDevice;
    unsigned int input;
    Menu *pNext;

    pDevice = CInput::GetAvailableDeviceRecord(g_frontendOverlayMode);
    FrontendMenu_ResetActiveMenu();
    CInput::UpdateAllAvailableDevices();
    Input_TranslatePedalsToMenuKeys();
    Sound_UpdateMusicStreaming();
    if (g_pMenu0x00818abc != FrontendMenu_GetDeviceBindings() && g_pMenu0x00818abc != FrontendMenu_GetDeviceCalibration())
        Input_MergeAssignedJoystickButtons(g_frontendOverlayMode, pDevice);
    input = pDevice->pressed;
    if (g_frontendOverlayMode == 1)
        input |= CInput::GetAvailableDeviceRecord(Input_GetControllerSlotMapping(0))->pressed & 0x20;
    pNext = (Menu *)Menu_Update(g_pMenu0x00818abc, input);
    if (pNext != NULL) {
        g_menuEnterTime = CFrontend::GetFrontendTimestamp();
        g_pMenu0x00818ac0 = pNext;
    }
}

// FUNCTION: CMR2 0x004ea5b0
void FrontendMenu_DrawActiveMenu(void)
{
    Menu_CallCallback2(g_pMenu0x00818abc);
}

// FUNCTION: CMR2 0x004ea5c0
void FrontendMenu_SetInputState(BYTE value)
{
    g_unk0x00818ac4 = value;
}

// FUNCTION: CMR2 0x004ea5d0
Menu *FrontendMenu_GetActiveMenu(void)
{
    return g_pMenu0x00818abc;
}

extern BYTE g_unk0x00818ce4;
NetPlayerID Network_GetLocalPlayerID(void);
BYTE RallyData_GetDriverRecordSelectionValue(BYTE index);
BYTE RallyData_GetDriverAutomaticGearbox(BYTE param1);
void NetPlayers_SetNotificationMask(void *param);

// Sends this machine's player description (id, car, flags) to the network
// player list.
// FUNCTION: CMR2 0x004ec2b0
void FrontendNetwork_SendPlayerDescription(void)
{
    NetPlayerInfo info;

    memset(&info, 0, sizeof(info));
    info.id = Network_GetLocalPlayerID();
    info.bits.car = RallyData_GetDriverRecordSelectionValue(0);
    info.bits.bit5 = RallyData_GetDriverAutomaticGearbox(0);
    info.bits.bit6 = g_unk0x00818ce4;
    info.bits.active = 1;
    info.bits.ready = 0;
    info.bits.finished = 0;
    NetPlayers_SetNotificationMask(&info);
}

void RallyData_PickOpponentLineups(void);
RallyDriverGroupTable *RallyData_GetDriverGroupTable(void);
BYTE *RallyData_GetStageAvailabilityFlags(void);
void NetPlayers_AddOrUpdatePlayerInfo(NetPlayerID *pId, NetPlayerInfo *pInfo, char add);
void NetPlayers_RemovePlayerByID(int *pId);
void NetPlayers_ResetRaceReadyAndTimeState(char resetTotal, char resetTimes);
void NetPlayers_BuildFinalClassification(void);
void NetPlayers_ReceivePublishedLeaderboard(char valid, BYTE *p);
void NetPlayers_RebuildSortedPlayerIDs(void);
void GameInfo_SetSessionField397C(int param1);
LPVOID *Network_GetSessionNamePointer(void);
LPVOID *Network_GetSessionPasswordPointer(void);
void Network_SetSessionStateFlag(BOOL param1);
void Session_SetOpen(char open);
int Session_GetUserValue(BYTE index);
void Network_SetSessionDescription(NetSessionDesc *pDesc);
void Network_RemoveSessionPlayerByID(NetPlayerID *pId);
int Network_CreateLocalPlayer(char *param1, char *param2, void *param3, int param4);
int Network_PollReceivedMessageBuffer(DWORD *param1, void **param2);
char Network_SendPlayerMessage(int to, int guaranteed, void *data, int size);
void NetworkChat_AppendLine(NetPlayerID *pFrom, char *text, char local);
void RallyData_SetStageSelectionAndRefreshFlags(BYTE param1);
void RallyData_SetDriverCategoryOption(BYTE index, BYTE value);
void RallyData_SetDriverAutomaticGearbox(BYTE index, BYTE flag);
void FrontendMenu_EnablePlayerSetupItems(void);
void RallyData_FillEventSlotSelections(void);
void GameInfo_SetSessionField3990(int param1);
unsigned int Network_GetEnumeratedSessionCount(void);
int Network_JoinEnumeratedSession(BYTE index, char *pPassword, BYTE *pInvalidPassword);
BYTE Network_IsSessionFlag10Set(BYTE index);
char Session_SetMaxPlayers(int count);
void Session_SetUserValue(BYTE index, int value);
void Session_SetName(LPVOID pName);
void Session_SetPassword(LPVOID pPassword);
void FrontendNetwork_SetMessageState(int value);

// The player setup data (0x540..0x6e7 of the rally data block) is mirrored
// into the outgoing packet and back. 0x18 bytes of it are per-player rows,
// the rest are the current selection values.
#define NET_SETUP_COUNT 0x14

// Player setup packet (type 3 from a joining player, 2 from the host when the
// race starts); 0x1c4 bytes.
struct NetSetupPacket {
    BYTE type;                  // 0x0
    BYTE field_0x1[3];
    unsigned int gameMode : 8;  // 0x4
    unsigned int country : 4;   // bits 8..11
    unsigned int stage : 4;     // bits 12..15
    unsigned int rallyA : 2;    // bits 16..17
    unsigned int rallyB : 2;    // bits 18..19
    unsigned int rallyC : 4;    // bits 20..23
    unsigned int unused : 8;
    BYTE stageStates[0xc];      // 0x8 CGameInfo::GetStageOptionState of the 11 stages
    int field_0x14;             // 0x14 GameInfo_GetSessionField397C
    RallyPairingTables pairing; // 0x18 RallyDriverGroupTable::pairing
    BYTE field_0x1a8[0x14];     // 0x1a8 RallyDriverGroupTable::field_0x6d4
    int field_0x1bc;            // 0x1bc RallyDriverGroupTable::field_0x53c
    int field_0x1c0;            // 0x1c0 RallyDriverGroupTable::field_0x6d0
};

// Copies the local player's setup (20 rows) into the outgoing packet.
// FUNCTION: CMR2 0x004ec320
void FrontendNetwork_WriteSetupPacket(NetSetupPacket *pPacket)
{
    RallyDriverGroupTable *pState;
    int i;

    RallyData_PickOpponentLineups();
    pState = RallyData_GetDriverGroupTable();
    for (i = 0; i < NET_SETUP_COUNT; i++) {
        pPacket->pairing.pairs[i][0] = pState->pairing.pairs[i][0];
        pPacket->pairing.pairs[i][1] = pState->pairing.pairs[i][1];
        pPacket->pairing.skills[i] = pState->pairing.skills[i];
        pPacket->pairing.flags[i] = pState->pairing.flags[i];
        pPacket->pairing.otherFlags[i] = pState->pairing.otherFlags[i];
        pPacket->field_0x1a8[i] = pState->field_0x6d4[i];
        pPacket->field_0x1bc = pState->field_0x53c;
        pPacket->field_0x1c0 = pState->field_0x6d0;
    }
}

// Copies a received setup packet (20 rows) into the local rally data block.
// FUNCTION: CMR2 0x004ec3c0
void FrontendNetwork_ReadSetupPacket(NetSetupPacket *pPacket)
{
    RallyDriverGroupTable *pState = RallyData_GetDriverGroupTable();
    int i;

    for (i = 0; i < NET_SETUP_COUNT; i++) {
        pState->pairing.pairs[i][0] = pPacket->pairing.pairs[i][0];
        pState->pairing.pairs[i][1] = pPacket->pairing.pairs[i][1];
        pState->pairing.skills[i] = pPacket->pairing.skills[i];
        pState->pairing.flags[i] = pPacket->pairing.flags[i];
        pState->pairing.otherFlags[i] = pPacket->pairing.otherFlags[i];
        pState->field_0x6d4[i] = pPacket->field_0x1a8[i];
        pState->field_0x53c = pPacket->field_0x1bc;
        pState->field_0x6d0 = pPacket->field_0x1c0;
    }
}


// Builds and sends this machine's player setup packet.
// FUNCTION: CMR2 0x004ec460
void FrontendNetwork_SendSetupPacket(void)
{
    NetSetupPacket packet;

    packet.type = 3;
    packet.gameMode = CGameInfo::GetConfiguredGameMode();
    switch (CGameInfo::GetConfiguredGameMode()) {
    case 8:
    case 9:
    case 10:
        packet.country = RallyDataCountryIndex();
        packet.stage = RallyDataStageIndex();
        break;
    case 0xb:
    case 0xc:
        packet.rallyA = RallyData_GetSelectionBits10To11();
        packet.rallyB = RallyData_GetSelectionBits12To13();
        packet.rallyC = RallyData_GetSelectionBits16To19();
        break;
    }
    packet.field_0x14 = GameInfo_GetSessionField397C();
    FrontendNetwork_WriteSetupPacket(&packet);
    Network_SendPlayerMessage(0, 1, &packet, 0x1c4);
}

// System notifications for both host and guests. The provider builds native
// records containing pointers into the receive buffer, not 32-bit wire words.
// FUNCTION: CMR2 0x004ec560
void FrontendNetwork_HandleJoinerMessage(NetPlayerID *pFrom, unsigned int *pData)
{
    unsigned int type;

    type = pData[0];
    switch (type) {
    case NET_SYS_DESTROYPLAYER: {
        NetMsgDestroyPlayer *message = (NetMsgDestroyPlayer *)pData;
        Network_RemoveSessionPlayerByID(&message->dpId);
        NetPlayers_RemovePlayerByID((int *)&message->dpId);
        return;
    }
    case NET_SYS_CREATEPLAYER: {
        NetMsgCreatePlayer *message = (NetMsgCreatePlayer *)pData;
        if (message->lpData && message->dwDataSize >= sizeof(NetPlayerInfo))
            NetPlayers_AddOrUpdatePlayerInfo(&message->dpId, (NetPlayerInfo *)message->lpData, 1);
        FrontendNetwork_SendPlayerDescription();
        FrontendNetwork_SendSetupPacket();
        return;
    }
    case NET_SYS_HOST:
        Network_SetSessionStateFlag(1);
        if (Network_GetSessionNamePointer() != NULL)
            strcpy(g_sessionName, (char *)Network_GetSessionNamePointer());
        if (Network_GetSessionPasswordPointer() != NULL)
            strcpy(g_sessionPassword, (char *)Network_GetSessionPasswordPointer());
        g_unk0x00818ce4 = 1;
        FrontendNetwork_SendPlayerDescription();
        switch (CGameInfo::GetConfiguredGameMode()) {
        case 8:
            FrontendMenu_GetNetworkSessionSetup()->pParent = FrontendMenu_GetNetworkPlayerSetup();
            break;
        case 9:
            FrontendMenu_GetNetworkSessionSetup()->pParent = FrontendMenu_GetNetworkCarSetup();
            break;
        case 10:
            FrontendMenu_GetNetworkSessionSetup()->pParent = FrontendMenu_GetNetworkRallySetup();
            break;
        case 0xb:
            FrontendMenu_GetNetworkSessionSetup()->pParent = FrontendMenu_GetNetworkStageSetup();
            break;
        case 0xc:
            FrontendMenu_GetNetworkSessionSetup()->pParent = FrontendMenu_GetNetworkExtendedStageSetup();
            break;
        }
        if (FrontendMenu_GetActiveMenu() != FrontendMenu_GetNetworkSessionDetails()) {
            Session_SetOpen(1);
            return;
        }
        Session_SetOpen(0);
        return;
    case NET_SYS_SETSESSIONDESC: {
        NetMsgSetSessionDesc *message = (NetMsgSetSessionDesc *)pData;
        Network_SetSessionDescription(&message->dpDesc);
        GameInfo_SetConfiguredGameMode((BYTE)Session_GetUserValue(0));
        GameInfo_SetSessionField398C(Session_GetUserValue(2));
        return;
    }
    case NET_SYS_SETPLAYERDATA: {
        NetMsgSetPlayerData *message = (NetMsgSetPlayerData *)pData;
        if (message->lpData && message->dwDataSize >= sizeof(NetPlayerInfo))
            NetPlayers_AddOrUpdatePlayerInfo(&message->dpId, (NetPlayerInfo *)message->lpData, 1);
        return;
    }
    }
}

// Message handler of the host side: applies the setup packet of a joining
// player and starts the race for it.
// FUNCTION: CMR2 0x004ec6f0
void FrontendNetwork_HandleHostMessage(NetPlayerID *pFrom, BYTE *pMsg)
{
    NetSetupPacket *pSetup = (NetSetupPacket *)pMsg;
    BYTE *pState = RallyData_GetStageAvailabilityFlags();
    MenuItem *pItem;
    int i;

    switch (pMsg[0]) {
    case 0:
        NetworkChat_AppendLine(pFrom, (char *)(pMsg + 1), 0);
        return;
    case 2:
    case 3:
        for (i = 0; i < 0xb; i++)
            pState[i] = pSetup->stageStates[i];
        GameInfo_SetConfiguredGameMode(pSetup->gameMode);
        switch (CGameInfo::GetConfiguredGameMode()) {
        case 8:
            RallyData_SetCountrySelectionBits(pSetup->country);
            for (i = 0; i < 0xb; i++) {
                if ((pState[i] & 1) != 0 && ((pState[i] & 2) == 0 || CGameInfo::IsRecordFlagSet(0xd))
                    && (pState[i] & 4) == 0) {
                    RallyData_SetStageSelectionAndRefreshFlags(i);
                    break;
                }
            }
            break;
        case 9:
        case 10:
            RallyData_SetCountrySelectionBits(pSetup->country);
            RallyData_SetStageSelectionAndRefreshFlags(pSetup->stage);
            break;
        case 0xb:
        case 0xc:
            RallyData_SetStageSelectionAndRefreshFlags(0);
            RallyData_SetSelectionBits10To11(pSetup->rallyA);
            RallyData_SetSelectionBits12To13(pSetup->rallyB);
            RallyData_SetSelectionBits16To19(pSetup->rallyC);
            break;
        }
        GameInfo_SetSessionField397C(pSetup->field_0x14);
        pItem = Menu_GetItem(FrontendMenu_GetNetworkSessionSetup(), 1);
        RallyData_SetDriverCategoryOption(0, g_filteredRouteIds[pItem->max]);
        pItem = Menu_GetItem(FrontendMenu_GetNetworkSessionSetup(), 2);
        RallyData_SetDriverAutomaticGearbox(0, pItem->max);
        FrontendNetwork_ReadSetupPacket(pSetup);
        g_hostSetupAppliedTime = CMain::GetFrameDelta();
        NetPlayers_ResetStageState(0, 1);
        if (pSetup->type == 2) {
            Menu_SetNextAction(FrontendMenu_GetRallyStartTransition());
            NetPlayers_ResetRaceReadyAndTimeState(1, 1);
            NetPlayers_ResetBestTimes();
            NetPlayers_RebuildSortedPlayerIDs();
            return;
        }
        break;
    case 0x11:
        NetPlayers_ReceivePublishedLeaderboard(pMsg[1], (BYTE *)(pMsg + 4));
        NetPlayers_BuildFinalClassification();
        break;
    }
}

// Drains the network message queue, dispatching each message to the handler
// of its direction.
// FUNCTION: CMR2 0x004ec8d0
void FrontendNetwork_DrainMessageQueue(void)
{
    NetPlayerID from;
    void *pData;

    while (Network_PollReceivedMessageBuffer(&from, &pData) != 0) {
        if (from == 0)
            FrontendNetwork_HandleJoinerMessage(&from, (unsigned int *)pData);
        else
            FrontendNetwork_HandleHostMessage(&from, (BYTE *)pData);
    }
}

bool Network_EnumerateServiceProviders(void);

// Callback of the network session menu: creates the DirectPlay session and
// lobby; the connection count becomes the value of the session type item.
// FUNCTION: CMR2 0x004ec9a0
void FrontendMenu_EnterNetworkConnection(Menu *pMenu, int param)
{
    g_unk0x00818f10 = 0;
    if (CGame::CreateDirectPlay()) {
        if (CGame::CreateDirectPlayLobby()) {
            if (Network_EnumerateServiceProviders()) {
                FrontendMenu_GetNetworkConnection()->items[0].min = (BYTE)CGame::GetConnectionCount();
                FrontendMenu_GetNetworkConnection()->items[0].max = 0;
                g_unk0x00818f10 = 1;
                return;
            }
            CGame::DestroyDirectPlayLobby();
            CGame::DestroyDirectPlay();
            Menu_SetNextAction(pMenu->pParent);
            return;
        }
        CGame::DestroyDirectPlay();
    }
}

// Sends the local player's description to the DirectPlay lobby. The original
// leaks the return value of the send call through this function.
// FUNCTION: CMR2 0x004eca10
char FrontendNetwork_SendLobbyPlayerDescription(void)
{
    unsigned int info[4];

    info[0] = Network_GetLocalPlayerID();
    info[1] = (RallyData_GetDriverRecordSelectionValue(0) & 0x1f) | (info[1] & 0xffffffe0) | 0x80;
    return Network_CreateLocalPlayer((char *)RallyData_GetRecord(0), (char *)RallyData_GetRecord(0), info, 0x10);
}

// The GUID of the session the browser list currently points at.
int Network_CopyEnumeratedSessionGUID(BYTE index, NetGuid *pOut);
// GLOBAL: CMR2 0x00818cf0
NetGuid g_unk0x00818cf0;
// GLOBAL: CMR2 0x00819018
int g_unk0x00819018;

// Per-frame step of the network session browser: enumerates the sessions,
// keeps the cursor on the previously selected one (or moves it with the
// up/down buttons), scrolls the five visible rows and fills the menu.
// match 58%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// logic verified against the dump; the difference is MSVC's basic-block
// order (the original keeps the -1/-2 exits and the first-time setup after
// the main return).
// FUNCTION: CMR2 0x004ecaf0
void FrontendMenu_UpdateNetworkSessionBrowser(Menu *pMenu)
{
    DeviceInfo *pDevice;
    NetGuid guid;
    int status;
    int i;

    pDevice = CInput::GetAvailableDeviceRecord(0);
    if (g_unk0x00818ef4 == 0)
        return;
    // PORT: the original showed the cursor and the desktop for the DirectPlay
    // dialogs around this call; the network layer shows none.
    status = CGameInfo::EnumerateNetworkSessionsWithUserData(g_sessionPassword);
    if (status == 1) {
        g_unk0x00818d04 = 1;
        g_unk0x00819018 = Network_GetEnumeratedSessionCount();
        if (g_unk0x00819014 != 0) {
            if (g_unk0x00819018 > 0) {
                if (g_unk0x0052528c != g_unk0x00819018) {
                    if (g_unk0x00819024 != 0) {
                        for (i = 0; i < g_unk0x00819018; i++) {
                            Network_CopyEnumeratedSessionGUID((BYTE)i, &guid);
                            if (memcmp(&guid, &g_unk0x00818cf0, sizeof(guid)) == 0) {
                                g_selectedNetworkSession = i;
                                break;
                            }
                        }
                    } else {
                        g_selectedNetworkSession = -1;
                    }
                }
                g_unk0x0052528c = g_unk0x00819018;
                if (g_selectedNetworkSession == -1) {
                    g_selectedNetworkSession = 0;
                    Network_CopyEnumeratedSessionGUID(0, &g_unk0x00818cf0);
                    g_unk0x00819024 = 1;
                } else if ((pDevice->pressed & 0x20) != 0) {
                    g_unk0x00819014 = 0;
                    g_selectedNetworkSession = -1;
                    g_unk0x00819024 = 0;
                    CGameInfo::EnumerateNetworkSessions();
                    g_unk0x00818ef4 = 0;
                } else {
                    Menu_SetFlags(pMenu, 0, 0, 1, 0);
                    if ((pDevice->pressed & 4) != 0 && g_selectedNetworkSession > 0) {
                        g_selectedNetworkSession--;
                        Network_CopyEnumeratedSessionGUID((BYTE)g_selectedNetworkSession, &g_unk0x00818cf0);
                        g_unk0x00819024 = 1;
                    } else if ((pDevice->pressed & 8) != 0 && g_selectedNetworkSession < g_unk0x00819018 - 1) {
                        g_selectedNetworkSession++;
                        Network_CopyEnumeratedSessionGUID((BYTE)g_selectedNetworkSession, &g_unk0x00818cf0);
                        g_unk0x00819024 = 1;
                    }
                }
            } else {
                g_unk0x00819014 = 0;
                g_selectedNetworkSession = -1;
                g_unk0x0052528c = -1;
                g_unk0x00819024 = 0;
                g_unk0x0081901c = 0;
            }
            if (g_selectedNetworkSession < g_unk0x0081901c) {
                g_unk0x0081901c--;
                if (g_unk0x0081901c < 0)
                    g_unk0x0081901c = 0;
            }
            if (g_selectedNetworkSession >= g_unk0x0081901c + 5)
                g_unk0x0081901c++;
        } else {
            g_selectedNetworkSession = 0;
            g_unk0x0052528c = -1;
            g_unk0x00819024 = 0;
            Menu_SetFlags(pMenu, 1, 1, 1, 1);
        }
        if (g_unk0x00819018 > 5)
            g_unk0x00819020 = 5;
        else
            g_unk0x00819020 = g_unk0x00819018;
    } else if (status != -1 && status == -2) {
        CGame::SkipNextCallbackRenderPass();
        Menu_SetNextAction(pMenu->pParent);
    }
}


// FUNCTION: CMR2 0x004eca60
void FrontendMenu_EnterNetworkSessionBrowser(Menu *pMenu, char param)
{
    Menu_SetFlags(pMenu, 1, 1, 1, 1);
    g_unk0x00818ef4 = 0;
    g_unk0x00819024 = 0;
    g_unk0x00819014 = 0;
    g_unk0x0052528c = -1;
    g_selectedNetworkSession = -1;
    Input_ClearCharacterQueue();
    sprintf(g_joinPassword, CMain::m_logFileBlankLine);
    g_unk0x0081901c = 0;
    g_unk0x00819020 = 0;
    pMenu->items[0].id = 0x1ff;
    if (param == 0) {
        g_unk0x00818d04 = 0;
        return;
    }
    g_unk0x00818d04 = 1;
    Input_ClearCharacterQueue();
}

// Item action of the session browser: joins the session the list points at
// and moves on to the car select or the error screen.
// FUNCTION: CMR2 0x004ecd60
void FrontendNetwork_JoinSelectedSession(Menu *pMenu, int param)
{
    if (g_unk0x00818ef4 != 0) {
        if (g_unk0x00819014 != 0 && g_unk0x00819024 != 0 && g_selectedNetworkSession >= 0
            && (int)g_selectedNetworkSession < (int)Network_GetEnumeratedSessionCount()) {
            g_unk0x00818ce4 = 0;
            NetPlayers_ResetAllTables();
            NetworkChat_ClearLog();
            g_unk0x00818ef4 = 0;
            if (Network_IsSessionFlag10Set((BYTE)g_selectedNetworkSession) == 0) {
                // The original reuses the menu parameter slot for the
                // invalid-password flag of the join call (its value is unused).
                if (Network_JoinEnumeratedSession((BYTE)g_selectedNetworkSession, g_joinPassword, (BYTE *)&pMenu) != 0) {
                    GameInfo_SetConfiguredGameMode((BYTE)Session_GetUserValue(0));
                    GameInfo_SetSessionField398C(Session_GetUserValue(2));
                    Network_GetLocalPlayerID();
                    RallyData_GetDriverRecordSelectionValue(0);
                    if (FrontendNetwork_SendLobbyPlayerDescription() != 0) {
                        Menu_SetParent(FrontendMenu_GetNetworkSessionSetup(), FrontendMenu_GetNetworkSessionBrowser());
                        Menu_SetNextAction(FrontendMenu_GetNetworkSessionSetup());
                        return;
                    }
                    Network_CloseSession();
                    FrontendNetwork_SetMessageState(3);
                    Menu_SetNextAction(FrontendMenu_GetNetworkMessage());
                    return;
                }
            }
            if (Network_IsSessionFlag10Set((BYTE)g_selectedNetworkSession) != 0) {
                Menu_SetNextAction(FrontendMenu_GetJoinPasswordEntry());
                return;
            }
            Network_CloseSession();
            FrontendNetwork_SetMessageState(1);
            Menu_SetNextAction(FrontendMenu_GetNetworkMessage());
            return;
        }
        if ((int)Network_GetEnumeratedSessionCount() > 0)
            g_unk0x00819014 = 1;
    } else {
        pMenu->items[0].id = 0x1ec;
        g_unk0x00818ef4 = 1;
    }
}

BYTE GameInfo_GetLastNetworkGameMode(void);
int Network_HostNamedSession(char *pSessionName, char *pPassword, DWORD user1, DWORD user2, DWORD user3,
                 DWORD user4, DWORD maxPlayers);

// Item action of the network session menu: refreshes the session data and
// starts the DirectPlay session, restoring the window when the game had left
// it hidden for the network dialog.
// FUNCTION: CMR2 0x004ecea0
void FrontendNetwork_CreateSelectedSession(Menu *pMenu, int param)
{
    g_unk0x00818ef4 = 0;
    g_unk0x00818ce4 = 1;
    NetPlayers_ResetAllTables();
    NetworkChat_ClearLog();
    GameInfo_SetConfiguredGameMode(GameInfo_GetLastNetworkGameMode());
    // PORT: no DirectPlay dialogs (the original showed the cursor and the
    // desktop for them).
    if (Network_HostNamedSession(CGameInfo::GetSessionName(), CGameInfo::GetSessionPassword(),
                     CGameInfo::GetConfiguredGameMode() & 0xff, 0, 0, 0, 8) != 0) {
        if (g_unk0x00818d04 == 0)
            g_unk0x00818d04 = 1;
        if (FrontendNetwork_SendLobbyPlayerDescription() != 0) {
            Menu_SetNextAction(FrontendMenu_GetNetworkSessionDetails());
            return;
        }
    }
}

bool Network_AddSessionPlayerSlot(BYTE param1);

// Item action of the network leaderboard list: goes on to the leaderboard of
// the slot shown in the first row, but only while it is still connected.
// FUNCTION: CMR2 0x004ecf80
void FrontendNetwork_OpenConnectedLeaderboard(Menu *pMenu, int param)
{
    if (Network_AddSessionPlayerSlot(pMenu->items[0].max))
        Menu_SetNextAction(FrontendMenu_GetNetworkSessionBrowser());
}

// FUNCTION: CMR2 0x004ecfa0
void FrontendNetwork_LeaveConnectionPages(Menu *pMenu, char param)
{
    if (param != 0 && g_unk0x00818f10 != 0) {
        CGame::DestroyDirectPlayLobby();
        CGame::DestroyDirectPlay();
        g_unk0x00818f10 = 0;
    }
}

void Session_SetOpen(char open);
DWORD Network_GetSessionPlayerCount(int index);
BYTE RallyData_GetDriverAutomaticGearbox(BYTE param1);
int GameInfo_GetSessionField398C(void);
int GameInfo_GetSessionField3990(void);
BYTE GameInfo_GetLastNetworkGameMode(void);

// Prepares the "record your name" screen: copies the selected car/driver
// names, clamps the letter rows to the current values and rebuilds the
// keyboard rows.
// FUNCTION: CMR2 0x004ecfd0
void FrontendMenu_EnterNetworkSessionDetails(Menu *pMenu, char param)
{
    Input_ClearCharacterQueue();
    Session_SetOpen(0);
    if ((int)Network_GetSessionPlayerCount(-1) <= 6)
        Menu_GetItem(pMenu, 1)->min = 5;
    else
        Menu_GetItem(pMenu, 1)->min = 3;
    if (Menu_GetItem(pMenu, 1)->max >= Menu_GetItem(pMenu, 1)->min)
        Menu_GetItem(pMenu, 1)->max = Menu_GetItem(pMenu, 1)->min - 1;
    if (param == 0) {
        strcpy(g_sessionName, CGameInfo::GetSessionName());
        strcpy(g_sessionPassword, CGameInfo::GetSessionPassword());
        Menu_GetItem(pMenu, 2)->max = GameInfo_GetSessionField398C() + 1;
        Menu_GetItem(pMenu, 3)->max = GameInfo_GetSessionField3990() - 2;
    }
    Menu_GetItem(pMenu, 1)->max = GameInfo_GetLastNetworkGameMode() - 8;
}

// Item action of the "create session" screen: validates the session name,
// player count and password items and sends the session setup of this host.
// FUNCTION: CMR2 0x004ed340
void FrontendNetwork_ApplyHostSessionDetails(Menu *pMenu, int param)
{
    if (strcmp(CMain::m_logFileBlankLine, g_sessionName) == 0) {
        pMenu->cursor = 0;
        Menu_PlaySoundId(3);
        return;
    }
    Session_SetUserValue(2, Menu_GetItem(pMenu, 2)->max - 1);
    GameInfo_SetSessionField398C(Menu_GetItem(pMenu, 2)->max - 1);
    if (Session_SetMaxPlayers(Menu_GetItem(pMenu, 3)->max + 2)) {
        GameInfo_SetSessionField3990(Menu_GetItem(pMenu, 3)->max + 2);
        Session_SetName(g_sessionName);
        CGameInfo::SetSessionName(g_sessionName);
        Session_SetPassword(g_sessionPassword);
        CGameInfo::SetSessionPassword(g_sessionPassword);
        GameInfo_SetConfiguredGameMode(Menu_GetItem(pMenu, 1)->max + 8);
        Session_SetUserValue(0, Menu_GetItem(pMenu, 1)->max + 8);
        Session_SetOpen(1);
        switch (Menu_GetItem(pMenu, 1)->max) {
        case 0:
            Menu_SetNextAction(FrontendMenu_GetNetworkPlayerSetup());
            break;
        case 1:
            Menu_SetNextAction(FrontendMenu_GetNetworkCarSetup());
            break;
        case 2:
            Menu_SetNextAction(FrontendMenu_GetNetworkRallySetup());
            break;
        case 3:
            Menu_SetNextAction(FrontendMenu_GetNetworkStageSetup());
            break;
        default:
            Menu_SetNextAction(FrontendMenu_GetNetworkExtendedStageSetup());
            break;
        }
        FrontendNetwork_SendSetupPacket();
        return;
    }
    Menu_PlaySoundId(3);
    Menu_SelectItem(pMenu, 3);
}

// FUNCTION: CMR2 0x004ed500
void FrontendMenu_LeaveNetworkSessionDetails(Menu *pMenu, char param)
{
    if (param != 0) {
        CGame::DestroyLocalNetworkPlayer();
        Network_CloseSession();
    } else {
        GameInfo_SetConfiguredGameMode(Menu_GetItem(pMenu, 1)->max + 8);
    }
}


// Rebuilds a front-end stage/route list menu from the current selection.
bool RallyData_HasCategoryAward(int index, int bit, char check);
BYTE RallyData_GetDriverAutomaticGearbox(BYTE param1);

// FUNCTION: CMR2 0x004ed530
void FrontendMenu_EnterNetworkSessionSetup(Menu *pMenu, int unused)
{
    int i;
    int index;
    int *pEntry;

    g_lobbyChatSelected = 1;
    g_unk0x00818ce8 = -1;
    g_unk0x00818ce0 = -1;
    FrontendMenu_SetInputState(0);
    Menu_SetFlags(pMenu, 1, 1, 1, 1);
    pMenu->cursor = 0;
    g_lobbyLastCursor = 0;
    index = 0;
    pEntry = g_filteredRouteIds;
    for (i = 0; i < 0x16; i++) {
        if (RallyData_HasCategoryAward(0, i, 1)) {
            *pEntry = i;
            if (i == (RallyData_GetDriverRecordSelectionValue(0) & 0xff)) {
                Menu_GetItem(pMenu, 1)->max = index;
            }
            index++;
            pEntry++;
        }
    }
    if (RallyData_GetDriverAutomaticGearbox(0) != 0)
        Menu_GetItem(pMenu, 2)->max = 1;
    else
        Menu_GetItem(pMenu, 2)->max = 0;
    if (g_unk0x00818ce4 != 0) {
        Menu_GetItem(pMenu, 3)->max = 0;
        Menu_GetItem(pMenu, 1)->min = index;
        return;
    }
    Menu_GetItem(pMenu, 3)->max = 1;
    Menu_GetItem(pMenu, 1)->min = index;
}

// Start action of the "host race" screen: copies the selected rally setup
// into the rally data block and sends the host setup packet (type 2).
// FUNCTION: CMR2 0x004ed610
void FrontendNetwork_StartHostRace(Menu *pMenu, int param)
{
    BYTE *pState = RallyData_GetStageAvailabilityFlags();
    NetSetupPacket packet;
    int i;

    if (Network_GetSessionStateFlag()) {
        Session_SetOpen(0);
        for (i = 0; i < 0xb; i++) {
            packet.stageStates[i] = CGameInfo::GetStageOptionState((BYTE)RallyDataCountryIndex(), i);
            if (CGameInfo::IsRecordFlagSet(0xd))
                packet.stageStates[i] &= 0xfd;
            pState[i] = packet.stageStates[i];
        }
        if (CGameInfo::GetConfiguredGameMode() == 8) {
            for (i = 0; i < 0xb; i++) {
                if ((CGameInfo::GetStageOptionState((BYTE)RallyDataCountryIndex(), i) & 1) != 0
                    && ((CGameInfo::GetStageOptionState((BYTE)RallyDataCountryIndex(), i) & 2) == 0
                        || CGameInfo::IsRecordFlagSet(0xd))
                    && (CGameInfo::GetStageOptionState((BYTE)RallyDataCountryIndex(), i) & 4) == 0) {
                    RallyData_SetStageSelectionAndRefreshFlags(i);
                    break;
                }
            }
        }
        RallyData_SetDriverCategoryOption(0, g_filteredRouteIds[Menu_GetItem(pMenu, 1)->max]);
        RallyData_SetDriverAutomaticGearbox(0, Menu_GetItem(pMenu, 2)->max);
        packet.type = 2;
        packet.gameMode = CGameInfo::GetConfiguredGameMode();
        switch (CGameInfo::GetConfiguredGameMode()) {
        case 8:
        case 9:
        case 10:
            packet.country = RallyDataCountryIndex();
            packet.stage = RallyDataStageIndex();
            break;
        case 0xb:
        case 0xc:
            packet.rallyA = RallyData_GetSelectionBits10To11();
            packet.rallyB = RallyData_GetSelectionBits12To13();
            packet.rallyC = RallyData_GetSelectionBits16To19();
            break;
        }
        packet.field_0x14 = GameInfo_GetSessionField397C();
        FrontendNetwork_WriteSetupPacket(&packet);
        Network_SendPlayerMessage(0, 1, &packet, 0x1c4);
        NetPlayers_ResetRaceReadyAndTimeState(1, 1);
        NetPlayers_ResetBestTimes();
        NetPlayers_ResetStageState(0, 1);
        CGame::SetProfileSelectionState(1);
        Menu_SetNextAction(FrontendMenu_GetRallyStartTransition());
        NetPlayers_RebuildSortedPlayerIDs();
    }
}

// FUNCTION: CMR2 0x004edb30
void FrontendMenu_LeaveNetworkSessionSetup(Menu *pMenu, char param)
{
    if (param != 0 && !Network_GetSessionStateFlag()) {
        CGame::DestroyLocalNetworkPlayer();
        Network_CloseSession();
    }
}

// FUNCTION: CMR2 0x004edb50
BYTE *FrontendNetwork_GetLobbyChatLine(void)
{
    return g_lobbyChatLine;
}

// Update callback of the session browser menu: drains the network queue.
// FUNCTION: CMR2 0x004edb60
void FrontendMenu_UpdateNetworkStageTimes(Menu *pMenu)
{
    FrontendNetwork_DrainMessageQueue();
    Input_ClearCharacterQueue();
}

// FUNCTION: CMR2 0x004edb70
void FrontendMenu_EnterNetworkPlayerSetup(Menu *pMenu, int param)
{
    unsigned int *pFlags = CGameInfo::GetGameInfoField9CAddress();
    int values[2];
    int i;

    GameInfo_GetPlayerOptionNibbles(&values[0], &values[1]);
    if (CGameInfo::IsRecordFlagSet(0xd))
        Menu_GetItem(pMenu, 1)->min = 8;
    else
        Menu_GetItem(pMenu, 1)->min = *pFlags >> 8 & 0xf;
    Menu_GetItem(pMenu, 1)->max = values[0];
    if (Menu_GetItem(pMenu, 1)->max > Menu_GetItem(pMenu, 1)->min)
        Menu_GetItem(pMenu, 1)->max = 0;
    if (CGameInfo::IsRecordFlagSet(0xd)) {
        Menu_GetItem(pMenu, 0)->min = values[0] % 2 + 10;
        return;
    }
    for (i = 0; i < 11; i++) {
        if (CGameInfo::GetStageOptionState(values[0], i) & 2) {
            Menu_GetItem(pMenu, 0)->min = i;
            break;
        }
    }
    if (values[0] % 2 != 0 && (i == 4 || i == 8) && !(CGameInfo::GetStageOptionState(values[0], 10) & 2))
        Menu_GetItem(pMenu, 0)->min++;
}

// FUNCTION: CMR2 0x004edca0
void FrontendNetwork_ToggleStageOption(Menu *pMenu, int param)
{
    int value = Menu_GetItem(pMenu, 0)->max;
    BYTE flags;

    if ((Menu_GetItem(pMenu, 0)->min == 5 && Menu_GetItem(pMenu, 0)->max == 4) ||
        (Menu_GetItem(pMenu, 0)->min == 9 && Menu_GetItem(pMenu, 0)->max == 8))
        value = 10;
    flags = CGameInfo::GetStageOptionState(Menu_GetItem(pMenu, 1)->max, value);
    if (flags & 1) {
        CGameInfo::SetStageOptionState(Menu_GetItem(pMenu, 1)->max, value, flags & ~1);
        return;
    }
    CGameInfo::SetStageOptionState(Menu_GetItem(pMenu, 1)->max, value, flags | 1);
}

// Item action of the "join session" screen: applies the selected role and
// opens the session, then continues to the car select screen.
// FUNCTION: CMR2 0x004edd50
void FrontendNetwork_ApplyPlayerRoleAndJoin(Menu *pMenu, int param)
{
    int i;
    int value;

    for (i = 0; i < Menu_GetItem(pMenu, 0)->min; i++) {
        if ((Menu_GetItem(pMenu, 0)->min == 5 && i == 4)
            || (Menu_GetItem(pMenu, 0)->min == 9 && i == 8))
            value = 10;
        else
            value = i;
        if ((CGameInfo::GetStageOptionState(Menu_GetItem(pMenu, 1)->max, value) & 1) != 0) {
            RallyData_SetStageSelectionAndRefreshFlags(0);
            RallyData_SetCountrySelectionBits(Menu_GetItem(pMenu, 1)->max);
            FrontendMenu_GetNetworkSessionSetup()->pParent = pMenu;
            Menu_SetNextAction(FrontendMenu_GetNetworkSessionSetup());
            break;
        }
    }
    FrontendNetwork_SendSetupPacket();
}

// Change callback of the "join session" menu: keeps the role item's minimum
// (the number of players) in sync with the selected player count.
// FUNCTION: CMR2 0x004ede10
void FrontendMenu_UpdateNetworkPlayerSetup(Menu *pMenu)
{
    int i;

    if (CGameInfo::IsRecordFlagSet(0xd)) {
        Menu_GetItem(pMenu, 0)->min = Menu_GetItem(pMenu, 1)->max % 2 + 10;
        FrontendNetwork_DrainMessageQueue();
        return;
    }
    for (i = 0; i < 0xb; i++) {
        if ((CGameInfo::GetStageOptionState(Menu_GetItem(pMenu, 1)->max, i) & 2) != 0) {
            Menu_GetItem(pMenu, 0)->min = i;
            break;
        }
    }
    if (Menu_GetItem(pMenu, 1)->max % 2 != 0 && (i == 4 || i == 8)
        && (CGameInfo::GetStageOptionState(Menu_GetItem(pMenu, 1)->max, 10) & 2) == 0)
        Menu_GetItem(pMenu, 0)->min++;
    FrontendNetwork_DrainMessageQueue();
}

// Init callback of the "join session" menu: sets the allowed player count
// from the game mode flags and fills the setup items of the local player.
// FUNCTION: CMR2 0x004edef0
void FrontendMenu_EnterNetworkCarSetup(Menu *pMenu, int param)
{
    unsigned int *pFlags = CGameInfo::GetGameInfoField9CAddress();
    int values[2];
    int min;

    if (CGameInfo::IsRecordFlagSet(0xd)) {
        min = 8;
    } else {
        unsigned int v = *pFlags;

        min = 4;
        if ((int)((v >> 8) & 0xf) > min)
            min = (v >> 8) & 0xf;
        if ((int)((v >> 0xc) & 0xf) > min)
            min = (v >> 0xc) & 0xf;
        if ((v & 1) != 0 && (int)((v >> 0x10) & 0xf) > min)
            min = (v >> 0x10) & 0xf;
    }
    Menu_GetItem(pMenu, 0)->min = min;
    if ((char)param != 0) {
        values[0] = (BYTE)RallyDataCountryIndex();
        values[1] = (BYTE)RallyDataStageIndex();
    } else {
        GameInfo_GetPlayerOptionNibbles(&values[0], &values[1]);
        Menu_GetItem(pMenu, 0)->max = values[0];
        Menu_GetItem(pMenu, 1)->max = values[1];
    }
    if (CGameInfo::IsRecordFlagSet(0xd)) {
        min = (values[0] % 2 != 0) + 0xa;
    } else {
        min = (int)((*pFlags >> 8) & 0xf) < values[0] + 1 ? 4 : *(volatile int *)&param;
        if ((int)((*pFlags >> 0xc) & 0xf) < values[0] + 1)
            min = 8;
        if ((*pFlags & 1) != 0 && (int)((*pFlags >> 0x10) & 0xf) < values[0] + 1)
            min = 0xa;
        if (values[0] % 2 != 0) {
            unsigned int mask = 1 << ((values[0] + 1) / 2 - 1);

            if ((pFlags[1] & mask & 0x1f) != 0
                || ((short)((pFlags[1] >> 5) & 0x1f) & mask) != 0
                || ((short)((pFlags[1] >> 10) & 0x1f) & mask) != 0)
                min++;
        }
    }
    Menu_GetItem(pMenu, 1)->min = min;
    if ((char)param == 0)
        Menu_GetItem(pMenu, 2)->max = 0;
    g_setupRouteIndex = values[0];
    FrontendMenu_UpdateNetworkCarSetup(pMenu);
}

// Item action of the "join" screen: applies the selected rally/route and
// sends the join setup of this player.
// FUNCTION: CMR2 0x004ee090
void FrontendNetwork_ApplyRallyAndJoin(Menu *pMenu, int param)
{
    if ((Menu_GetItem(pMenu, 1)->min == 5 && Menu_GetItem(pMenu, 1)->max == 4)
        || (Menu_GetItem(pMenu, 1)->min == 9 && Menu_GetItem(pMenu, 1)->max == 8)
        || (Menu_GetItem(pMenu, 1)->min == 0xb && Menu_GetItem(pMenu, 1)->max == 0xa))
        RallyData_SetStageSelectionAndRefreshFlags(10);
    else
        RallyData_SetStageSelectionAndRefreshFlags(Menu_GetItem(pMenu, 1)->max);
    RallyData_SetCountrySelectionBits(Menu_GetItem(pMenu, 0)->max);
    if (RallyDataStageIndex() != 0xa && g_setupSelectedRouteIds[Menu_GetItem(pMenu, 2)->max] != -1)
        RallyData_SetDriverPairingStateValues(1, g_setupSelectedRouteIds[Menu_GetItem(pMenu, 2)->max]);
    else
        RallyData_SetDriverPairingStateValues(1, 0);
    FrontendMenu_GetNetworkSessionSetup()->pParent = pMenu;
    Menu_SetNextAction(FrontendMenu_GetNetworkSessionSetup());
    FrontendNetwork_SendSetupPacket();
}

int GameInfo_GetSessionField3988(void);

// Update callback of the two-player rally menu: limits the rally list to the
// unlocked events and mirrors the rally/stage reached in the profile.
// FUNCTION: CMR2 0x004ee460
void FrontendMenu_EnterNetworkRallySetup(Menu *pMenu, int param)
{
    unsigned int *pFlags = CGameInfo::GetGameInfoField9CAddress();
    int values[2];
    int count;

    if (CGameInfo::IsRecordFlagSet(0xd)) {
        count = 8;
    } else {
        count = 4;
        if ((int)(*pFlags >> 8 & 0xf) > count)
            count = (int)(*pFlags >> 8 & 0xf);
        if ((int)(*pFlags >> 0xc & 0xf) > count)
            count = (int)(*pFlags >> 0xc & 0xf);
        if ((*pFlags & 1) != 0 && (int)(*pFlags >> 0x10 & 0xf) > count)
            count = (int)(*pFlags >> 0x10 & 0xf);
    }
    Menu_GetItem(pMenu, 0)->min = count;
    GameInfo_GetPlayerOptionNibbles(&values[0], &values[1]);
    Menu_GetItem(pMenu, 0)->max = values[0];
    Menu_GetItem(pMenu, 1)->max = values[1];
    if (CGameInfo::IsRecordFlagSet(0xd)) {
        count = (values[0] % 2 != 0) + 10;
    } else {
        count = (int)(*pFlags >> 8 & 0xf) < values[0] + 1 ? 4 : param;
        if ((int)(*pFlags >> 0xc & 0xf) < values[0] + 1)
            count = 8;
        if ((*pFlags & 1) != 0 && (int)(*pFlags >> 0x10 & 0xf) < values[0] + 1)
            count = 0xa;
        if (values[0] % 2 != 0) {
            unsigned int mask = 1 << ((values[0] + 1) / 2 - 1);
            if ((pFlags[1] & mask & 0x1f) != 0 ||
                ((short)((pFlags[1] >> 5) & 0x1f) & mask) != 0 ||
                ((short)((pFlags[1] >> 10) & 0x1f) & mask) != 0)
                count++;
        }
    }
    Menu_GetItem(pMenu, 1)->min = count;
    if ((param & 0xff) == 0) {
        Menu_GetItem(pMenu, 2)->max = GameInfo_GetSessionField397C();
        if (Menu_GetItem(pMenu, 2)->max != 0)
            Menu_GetItem(pMenu, 2)->max -= 4;
    }
    FrontendMenu_UpdateNetworkRallySetup(pMenu);
}

// Item action of the two-player rally menu: selects the rally and stage and
// moves on to the driver selection.
// FUNCTION: CMR2 0x004ee600
void FrontendNetwork_ApplyTwoPlayerRally(Menu *pMenu, int param)
{
    if ((Menu_GetItem(pMenu, 1)->min == 5 && Menu_GetItem(pMenu, 1)->max == 4) ||
        (Menu_GetItem(pMenu, 1)->min == 9 && Menu_GetItem(pMenu, 1)->max == 8) ||
        (Menu_GetItem(pMenu, 1)->min == 0xb && Menu_GetItem(pMenu, 1)->max == 0xa))
        RallyData_SetStageSelectionAndRefreshFlags(10);
    else
        RallyData_SetStageSelectionAndRefreshFlags(Menu_GetItem(pMenu, 1)->max);
    RallyData_SetCountrySelectionBits(Menu_GetItem(pMenu, 0)->max);
    if (Menu_GetItem(pMenu, 2)->max != 0)
        GameInfo_SetSessionField397C(Menu_GetItem(pMenu, 2)->max + 4);
    else
        GameInfo_SetSessionField397C(Menu_GetItem(pMenu, 2)->max);
    RallyData_SetDriverPairingStateValues(1, 0);
    FrontendMenu_GetNetworkSessionSetup()->pParent = pMenu;
    Menu_SetNextAction(FrontendMenu_GetNetworkSessionSetup());
    FrontendNetwork_SendSetupPacket();
}

// Initialises the rally/stage select menu from the unlocked rally count.
// FUNCTION: CMR2 0x004ee850
void FrontendMenu_EnterNetworkStageSetup(Menu *pMenu, int param)
{
    int order[8];
    unsigned int *pFlags;
    unsigned int count;
    unsigned int n;
    int i;

    order[0] = 6;
    order[1] = 3;
    order[2] = 1;
    order[3] = 4;
    order[4] = 0;
    order[5] = 2;
    order[6] = 5;
    order[7] = 7;
    i = 0;
    pFlags = CGameInfo::GetGameInfoField9CAddress();
    count = 6;
    if (!CGameInfo::IsRecordFlagSet(0xd)) {
        count = (*pFlags >> 2 & 3) * 3;
        n = (*pFlags >> 4 & 3) * 3;
        if (n > count)
            count = n;
        n = (*pFlags >> 6 & 3) * 3;
        if (n > count)
            count = n;
    }
    do {
        if (i < (int)count || CGameInfo::IsRecordFlagSet(0xd))
            g_unk0x00818ed4[i] = order[i];
        i++;
    } while (i < 6);
    if ((*pFlags & 0x100000) || CGameInfo::IsRecordFlagSet(0xd)) {
        count++;
        g_unk0x00818ed4[i] = 5;
    }
    if ((*pFlags & 0x200000) || CGameInfo::IsRecordFlagSet(0xd)) {
        count++;
        g_unk0x00818ed4[i + 1] = 7;
    }
    Menu_GetItem(pMenu, 0)->min = count;
    Menu_GetItem(pMenu, 0)->max = 0;
    for (i = 0; i < (int)count; i++) {
        if (g_unk0x00818ed4[i] == GameInfo_GetSessionField3984()) {
            Menu_GetItem(pMenu, 0)->max = i;
            break;
        }
    }
    Menu_GetItem(pMenu, 1)->max = GameInfo_GetSessionField3988() - 1;
}

void GameInfo_SetSessionField3984(int param1);
void GameInfo_SetSessionField3988(int param1);

// Item action of the single-player rally menu: selects the rally and stage
// picked in the menu.
// FUNCTION: CMR2 0x004ee9b0
void FrontendNetwork_ApplySinglePlayerRally(Menu *pMenu, int param)
{
    int order[8] = { 6, 3, 1, 4, 0, 2, 5, 7 };
    int i;

    i = 0;
    for (i = 0; i < 8; i++) {
        if (order[i] == g_unk0x00818ed4[Menu_GetItem(pMenu, 0)->max])
            break;
    }
    RallyData_SetStageSelectionAndRefreshFlags(0);
    RallyData_SetSelectionBits10To11(i / 3);
    RallyData_SetSelectionBits12To13(i % 3);
    RallyData_SetSelectionBits16To19(Menu_GetItem(pMenu, 1)->max + 1);
    GameInfo_SetSessionField3988(Menu_GetItem(pMenu, 1)->max + 1);
    GameInfo_SetSessionField3984(g_unk0x00818ed4[Menu_GetItem(pMenu, 0)->max]);
    FrontendMenu_GetNetworkSessionSetup()->pParent = pMenu;
    Menu_SetNextAction(FrontendMenu_GetNetworkSessionSetup());
    FrontendNetwork_SendSetupPacket();
}

// FUNCTION: CMR2 0x004eeab0
void FrontendMenu_EnterNetworkExtendedStageSetup(Menu *pMenu, char param)
{
    int order[8];
    unsigned int *pFlags;
    unsigned int count;
    unsigned int n;
    int i;

    order[0] = 6;
    order[1] = 3;
    order[2] = 1;
    order[3] = 4;
    order[4] = 0;
    order[5] = 2;
    order[6] = 5;
    order[7] = 7;
    i = 0;
    pFlags = CGameInfo::GetGameInfoField9CAddress();
    count = 6;
    if (!CGameInfo::IsRecordFlagSet(0xd)) {
        count = (*pFlags >> 2 & 3) * 3;
        n = (*pFlags >> 4 & 3) * 3;
        if (n > count)
            count = n;
        n = (*pFlags >> 6 & 3) * 3;
        if (n > count)
            count = n;
    }
    do {
        if (i < (int)count || CGameInfo::IsRecordFlagSet(0xd))
            g_unk0x00818ed4[i] = order[i];
        i++;
    } while (i < 6);
    if ((*pFlags & 0x100000) || CGameInfo::IsRecordFlagSet(0xd)) {
        count++;
        g_unk0x00818ed4[i] = 5;
    }
    if ((*pFlags & 0x200000) || CGameInfo::IsRecordFlagSet(0xd)) {
        count++;
        g_unk0x00818ed4[i + 1] = 7;
    }
    Menu_GetItem(pMenu, 0)->min = count;
    Menu_GetItem(pMenu, 0)->max = 0;
    for (i = 0; i < (int)count; i++) {
        if (g_unk0x00818ed4[i] == GameInfo_GetSessionField3984()) {
            Menu_GetItem(pMenu, 0)->max = i;
            break;
        }
    }
    if (param == 0) {
        Menu_GetItem(pMenu, 1)->max = GameInfo_GetSessionField397C();
        if (Menu_GetItem(pMenu, 1)->max != 0)
            Menu_GetItem(pMenu, 1)->max -= 4;
    }
}

// Update callback of the network session menu: pumps the DirectPlay message
// queue.
// FUNCTION: CMR2 0x004eec30
void FrontendNetwork_PumpMenuMessages(Menu *pMenu)
{
    FrontendNetwork_DrainMessageQueue();
}

void GameInfo_SetSessionField3984(int param1);

// Item action of the championship rally menu: stores the rally and stage
// picked in the menu.
// FUNCTION: CMR2 0x004eec40
void FrontendNetwork_ApplyChampionshipRally(Menu *pMenu, int param)
{
    int order[8] = { 6, 3, 1, 4, 0, 2, 5, 7 };
    int i;

    i = 0;
    for (i = 0; i < 8; i++) {
        if (order[i] == g_unk0x00818ed4[Menu_GetItem(pMenu, 0)->max])
            break;
    }
    RallyData_SetStageSelectionAndRefreshFlags(0);
    RallyData_SetSelectionBits10To11(i / 3);
    RallyData_SetSelectionBits12To13(i % 3);
    GameInfo_SetSessionField3984(g_unk0x00818ed4[Menu_GetItem(pMenu, 0)->max]);
    if (Menu_GetItem(pMenu, 1)->max != 0)
        GameInfo_SetSessionField397C(Menu_GetItem(pMenu, 1)->max + 4);
    else
        GameInfo_SetSessionField397C(Menu_GetItem(pMenu, 1)->max);
    FrontendMenu_GetNetworkSessionSetup()->pParent = pMenu;
    Menu_SetNextAction(FrontendMenu_GetNetworkSessionSetup());
    FrontendNetwork_SendSetupPacket();
}

// FUNCTION: CMR2 0x004eed50
void FrontendMenu_EnterJoinPasswordEntry(Menu *pMenu, int param)
{
    Menu_SetFlags(pMenu, 0, 0, 1, 1);
    Input_ClearCharacterQueue();
    sprintf(g_joinPassword, CMain::m_logFileBlankLine);
}

// Name entry: appends the typed character or deletes the last one.
// FUNCTION: CMR2 0x004eed80
void FrontendMenu_UpdateJoinPasswordEntry(Menu *pMenu)
{
    int key;
    int len;

    g_pJoinPasswordText = g_joinPassword;
    strcpy(CFrontend::m_stringDest, g_joinPassword);
    Sys_RequestTextInput(CFrontend::GetTextString(0x1e7), g_joinPassword, 19);   // PORT
    if (!Input_PopQueuedCharacter(&key))
        return;
    if (key != 8) {
        len = strlen(CFrontend::m_stringDest);
        if (len < 19 && strchr(g_nameChars0x00525330, (char)key) != NULL) {
            CFrontend::m_stringDest[len] = (char)key;
            CFrontend::m_stringDest[len + 1] = 0;
            Menu_PlaySoundId(1);
        }
    } else if (CFrontend::m_stringDest[0] != 0) {
        CFrontend::m_stringDest[strlen(CFrontend::m_stringDest) - 1] = 0;
        Menu_PlaySoundId(2);
    }
    strcpy(g_pJoinPasswordText, CFrontend::m_stringDest);
}

void NetworkChat_ClearLog(void);
void GameInfo_ResetSessionTimestamp(void);
void FrontendNetwork_SetMessageState(int value);
int Network_JoinEnumeratedSession(BYTE index, char *pPassword, BYTE *pInvalidPassword);

// Callback that joins a DirectPlay session with the network settings of the
// menu: sends the local player description and opens the session or the lobby.
// FUNCTION: CMR2 0x004eee60
void FrontendNetwork_JoinNamedSession(Menu *pMenu, int param)
{
    BYTE invalidPassword;

    g_unk0x00818ce4 = 0;
    NetPlayers_ResetAllTables();
    NetworkChat_ClearLog();
    g_unk0x00818ef4 = 0;
    if (Network_JoinEnumeratedSession((BYTE)g_selectedNetworkSession, g_joinPassword, &invalidPassword) != 0) {
        GameInfo_SetConfiguredGameMode((BYTE)Session_GetUserValue(0));
        GameInfo_SetSessionField398C(Session_GetUserValue(2));
        Network_GetLocalPlayerID();
        RallyData_GetDriverRecordSelectionValue(0);
        if (FrontendNetwork_SendLobbyPlayerDescription() != 0) {
            Menu *pLobby = FrontendMenu_GetNetworkSessionBrowser();
            Menu_SetParent(FrontendMenu_GetNetworkSessionSetup(), pLobby);
            Menu_SetNextAction(FrontendMenu_GetNetworkSessionSetup());
        } else {
            Network_CloseSession();
            FrontendNetwork_SetMessageState(3);
            Menu_SetNextAction(FrontendMenu_GetNetworkMessage());
        }
        return;
    }
    if (invalidPassword != 0) {
        Menu_SetNextAction(FrontendMenu_GetJoinPasswordEntry());
        return;
    }
    Network_CloseSession();
    FrontendNetwork_SetMessageState(1);
    Menu_SetNextAction(FrontendMenu_GetNetworkMessage());
}

// FUNCTION: CMR2 0x004eef30
void FrontendMenu_EnterNetworkLeaderboard(Menu *pMenu, int param)
{
    Menu_GetItem(pMenu, 0)->min = CNetworkLeaderboards::GetTotalLeaderboards();
    if (Menu_GetItem(pMenu, 0)->min != 0) {
        Menu_GetItem(pMenu, 0)->enabled = 1;
        if (CNetworkLeaderboards::GetLeaderboardId() == -1)
            CNetworkLeaderboards::SetLeaderboardId(0);
        Menu_GetItem(pMenu, 0)->max = CNetworkLeaderboards::GetLeaderboardId();
        return;
    }
    Menu_GetItem(pMenu, 0)->enabled = 0;
}


// Item action of a leaderboard entry: shows the leaderboard of the slot the
// item stands for and goes back to the list.
// FUNCTION: CMR2 0x004eefb0
void FrontendLeaderboard_SelectEntry(Menu *pMenu, int param)
{
    CNetworkLeaderboards::SetLeaderboardId(Menu_GetItem(pMenu, 0)->max);
    Menu_GoBack(pMenu);
}

// Adds a new leaderboard while fewer than 0x20 slots are in use.
// FUNCTION: CMR2 0x004eefe0
void FrontendLeaderboard_AddEntry(Menu *pMenu, int param)
{
    if (CNetworkLeaderboards::GetTotalLeaderboards() < 0x20) {
        CNetworkLeaderboards::AddLeaderboard();
        CNetworkLeaderboards::SetLeaderboardId(
            CNetworkLeaderboards::GetTotalLeaderboards() - 1);
    }
}

// Keeps the leaderboard list consistent after the selected entry changes.
// FUNCTION: CMR2 0x004ef000
void FrontendLeaderboard_RefreshSelection(Menu *pMenu, int param)
{
    if (CNetworkLeaderboards::GetTotalLeaderboards() > 0)
        CNetworkLeaderboards::RemoveLeaderboard(Menu_GetItem(pMenu, 0)->max);
}

// FUNCTION: CMR2 0x004ef030
void FrontendMenu_UpdateNetworkLeaderboard(Menu *pMenu)
{
    Menu_GetItem(pMenu, 0)->min = CNetworkLeaderboards::GetTotalLeaderboards();
    if (Menu_GetItem(pMenu, 0)->min != 0) {
        Menu_GetItem(pMenu, 2)->enabled = 1;
        if (!Menu_GetItem(pMenu, 0)->enabled) {
            Menu_GetItem(pMenu, 0)->enabled = 1;
            if (CNetworkLeaderboards::GetLeaderboardId() == -1)
                CNetworkLeaderboards::SetLeaderboardId(0);
            Menu_GetItem(pMenu, 0)->max = CNetworkLeaderboards::GetLeaderboardId();
        }
        if (CNetworkLeaderboards::GetTotalLeaderboards() < 32)
            Menu_GetItem(pMenu, 1)->enabled = 1;
        else
            Menu_GetItem(pMenu, 1)->enabled = 0;
        if (Menu_GetItem(pMenu, 0)->max >= Menu_GetItem(pMenu, 0)->min)
            Menu_GetItem(pMenu, 0)->max = Menu_GetItem(pMenu, 0)->min - 1;
    } else {
        Menu_GetItem(pMenu, 2)->enabled = 0;
        Menu_GetItem(pMenu, 1)->enabled = 1;
    }
}

// Resets the 12 frontend scrollers: no menu attached, item gap scaled from
// 24 pixels at 640 wide.
// FUNCTION: CMR2 0x004ef150
void FrontendScroller_ResetAll(void)
{
    int i;

    for (i = 0; i < 12; i++) {
        g_menuScrollers[i].pMenu = NULL;
        g_menuScrollers[i].spacing = (int)(g_pGraphics->resX * 24) / 640;
    }
}

// Recomputes every scroller attached to a menu: gap, reset position and the
// width of each item's text.
// match 87%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004ef190
void FrontendScroller_RecomputeAllAttached(void)
{
    MenuScroller *p;
    int k;

    p = g_menuScrollers;
    do {
        p->spacing = (int)(g_pGraphics->resX * 24) / 640;
        p->offset = 0;
        p->startOffset = 0;
        if (p->pMenu != NULL) {
            p->count = p->pMenu->itemCount;
            for (k = 0; k < p->pMenu->itemCount; k++) {
                strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(p->pMenu->items[k].id));
                CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
                p->widths[k] = Font_GetTextWidth(2, (BYTE *)CFrontend::m_stringDest);
            }
        }
        p++;
    } while ((INT_PTR)&p->pMenu < (INT_PTR)&g_menuScrollers[12].pMenu); // 0x819754 in the original
}

// FUNCTION: CMR2 0x004ef480
void FrontendScroller_GetSelectionAndTimestamp(int *pOut1, int *pOut2)
{
    *pOut1 = g_scrollerSelection;
    *pOut2 = g_scrollerSelectionTimestamp;
}

// FUNCTION: CMR2 0x004ef4a0
void FrontendScroller_SetSelection(int value)
{
    g_scrollerSelection = value;
    g_unk0x0052539c = value;
}

// FUNCTION: CMR2 0x004ef4c0
void FrontendMenu_EnterSingleRallySelection(Menu *pMenu, int param)
{
    g_unk0x0082aa40 = SavedGames_GetCount();
    g_savedGameListSelection = 0;
    g_savedGameListTop = 0;
    g_unk0x0082ac48 = 0;
}

// Scroll of a 13-row list: g_savedGameListSelection is the selected row,
// g_savedGameListTop the first visible one.
// FUNCTION: CMR2 0x004ef4e0
void FrontendMenu_UpdateSingleRallySelection(Menu *pMenu)
{
    DeviceInfo *pDevice;

    g_unk0x0082aa40 = SavedGames_GetCount();
    Menu_SetFlags(pMenu, 0, 0, 1, 1);
    pDevice = CInput::GetAvailableDeviceRecord(0);
    if (g_unk0x0082aa40 > 0) {
        if (g_savedGameListSelection == -1)
            g_savedGameListSelection = 0;
        else if ((pDevice->pressed & 4) && g_savedGameListSelection > 0)
            g_savedGameListSelection--;
        else if ((pDevice->pressed & 8) && g_savedGameListSelection < g_unk0x0082aa40 - 1)
            g_savedGameListSelection++;
    } else {
        g_savedGameListSelection = -1;
        g_savedGameListTop = 0;
    }
    if (g_savedGameListSelection < g_savedGameListTop) {
        g_savedGameListTop--;
        if (g_savedGameListTop < 0)
            g_savedGameListTop = 0;
    }
    if (g_savedGameListSelection >= g_savedGameListTop + 13)
        g_savedGameListTop++;
    g_unk0x0082ac48 = 13;
    if (g_unk0x0082aa40 <= 13)
        g_unk0x0082ac48 = g_unk0x0082aa40;
}

void Frontend_SetDebugOverlayChannels(BYTE param1, BYTE param2, BYTE param3);

// Callback of the rally options screen: arms the rally data flag from the game
// mode and then either starts a fresh session or requests the screen change.
// FUNCTION: CMR2 0x004ef590
void FrontendMenu_UpdateRallyStartTransition(Menu *pMenu)
{
    if (CGameInfo::GetConfiguredPlayerCount() == 2 && CGameInfo::GetConfiguredGameMode() == 6)
        RallyData_SetSetupFlag11(1);
    else
        RallyData_SetSetupFlag11(0);
    if (CGameInfo::GetGameInfoSessionFlag() != 0) {
        FrontendSession_InitFreshRace();
        Frontend_SetDebugOverlayChannels(0, 0, 1);
    } else {
        Frontend_SetDebugOverlayChannels(1, 0, 0);
    }
}

// Callback that clears the debug overlay channels.
// FUNCTION: CMR2 0x004ef5e0
void FrontendMenu_UpdateQuitTransition(Menu *pMenu)
{
    Frontend_SetDebugOverlayChannels(0, 0, 0);
}

// FUNCTION: CMR2 0x004ef5f0
void FrontendMenu_EnterRallyModes(Menu *pMenu, int param)
{
    RallyData_SetSelectionBits16To19(1);
}

void RallyData_SetStageSelectionAndRefreshFlags(BYTE param1);

// Callback of the rally options menu: copies the current cursor to the
// matching options sub-menu and seeds its value from the game info.
// FUNCTION: CMR2 0x004ef600
void FrontendMenu_LeaveRallyModes(Menu *pMenu, int param)
{
    switch (pMenu->cursor) {
    case 0:
        GameInfo_SetConfiguredGameMode(0);
        RallyData_SetCountrySelectionBits(0);
        RallyData_SetStageSelectionAndRefreshFlags(0);
        if (CGameInfo::GetConfiguredPlayerCount() < 5) {
            FrontendMenu_GetSingleRallyDifficulty()->items[0].max = CGameInfo::GetConfiguredPlayerCount() - 1;
            return;
        }
        FrontendMenu_GetSingleRallyDifficulty()->items[0].max = 0;
        return;
    case 1:
        GameInfo_SetConfiguredGameMode(1);
        RallyData_SetStageSelectionAndRefreshFlags(0);
        if (CGameInfo::GetConfiguredPlayerCount() < 5) {
            FrontendMenu_GetChampionshipDifficulty()->items[0].max = CGameInfo::GetConfiguredPlayerCount() - 1;
            return;
        }
        FrontendMenu_GetChampionshipDifficulty()->items[0].max = 0;
        return;
    case 2:
        GameInfo_SetConfiguredGameMode(2);
        if (CGameInfo::GetConfiguredPlayerCount() < 5) {
            FrontendMenu_GetTimeTrialDifficulty()->items[0].max = CGameInfo::GetConfiguredPlayerCount() - 1;
            return;
        }
        FrontendMenu_GetTimeTrialDifficulty()->items[0].max = 0;
        return;
    case 3:
        GameInfo_SetConfiguredGameMode(3);
        if (CGameInfo::GetConfiguredPlayerCount() < 5) {
            FrontendMenu_GetMultiplayerDifficulty()->items[0].max = CGameInfo::GetConfiguredPlayerCount() - 1;
            return;
        }
        FrontendMenu_GetMultiplayerDifficulty()->items[0].max = 0;
        return;
    case 4:
        GameInfo_SetConfiguredGameMode(4);
        if (CGameInfo::GetConfiguredPlayerCount() < 9) {
            FrontendMenu_GetMultiplayerExtendedDifficulty()->items[0].max = CGameInfo::GetConfiguredPlayerCount() - 1;
            return;
        }
        FrontendMenu_GetMultiplayerExtendedDifficulty()->items[0].max = 0;
        break;
    }
}

// FUNCTION: CMR2 0x004ef8f0
void FrontendMenu_EnterCarClass(Menu *pMenu, int param)
{
    if ((*CGameInfo::GetGameInfoField9CAddress() & 1) || CGameInfo::IsRecordFlagSet(0xd))
        pMenu->items[2].enabled = 1;
    else
        pMenu->items[2].enabled = 0;
    pMenu->cursor = CGameInfo::GetConfiguredDifficulty();
}

// FUNCTION: CMR2 0x004ef950
void FrontendMenu_EnterMultiplayerCarClass(Menu *pMenu, int param)
{
    pMenu->cursor = CGameInfo::GetGameModeOptionBits20To22();
}

// FUNCTION: CMR2 0x004ef960
void FrontendMenu_SelectMultiplayerClass(Menu *pMenu, int param)
{
    GameInfo_SetGameModeOptionBits20To22(pMenu->cursor);
}

// FUNCTION: CMR2 0x004efb50
void FrontendMenu_UpdateRallySelection(Menu *pMenu)
{
    RallyData_SetCountrySelectionBits(pMenu->cursor);
    FrontendScroller_UpdateCursorSlide(FrontendScroller_GetRallySelectionScroller());
}

// FUNCTION: CMR2 0x004efdc0
void FrontendMenu_UpdateRallyStageSelection(Menu *pMenu)
{
    FrontendScroller_UpdateCursorSlide(FrontendScroller_GetDisplayDeviceScroller());
}

// FUNCTION: CMR2 0x004efdd0
void FrontendMenu_UpdateAlternateRallyStageSelection(Menu *pMenu)
{
    FrontendScroller_UpdateCursorSlide(FrontendScroller_GetAlternateStageScroller());
}

// FUNCTION: CMR2 0x004f0050
void FrontendMenu_SelectAlternateRallyStage(Menu *pMenu, MenuItem *pItem)
{
    if (pItem->value == -1) {
        RallyData_SetDriverPairingStateValues(0, 0);
        Menu_SetNextAction(FrontendMenu_GetRallyStartTransition());
    } else {
        RallyData_SetDriverPairingStateValues(1, pItem->value);
        Menu_SetNextAction(FrontendMenu_GetRallyStartTransition());
    }
}

// FUNCTION: CMR2 0x004f0600
void FrontendMenu_LeaveMultiplayerStageSelection(Menu *pMenu, int param)
{
    Frontend_SetOverlayMode(0);
    Menu_SetInputStateFlag(1);
}

void RallyData_SetEditedDriverOrCategoryName(unsigned int slot, char *pName);

// Enters (param != 0) or leaves the category name-entry screen, resetting the
// letter rows and the stored name.
// FUNCTION: CMR2 0x004f0d30
void FrontendMenu_EnterProfileNameEntry(Menu *pMenu, char param)
{
    Frontend_SetOverlayMode(g_profileCurrentPlayer);
    Menu_SetActionLatch(1);
    FrontendProfile_SetNameEntryFlag(1);
    if (param) {
        pMenu->cursor = 2;
        pMenu->items[2].max = pMenu->items[2].min - 1;
    } else {
        pMenu->cursor = 0;
        pMenu->items[0].max = 0;
        RallyData_SetEditedDriverOrCategoryName(FrontendProfile_GetCurrentPlayer(), CMain::m_logFileBlankLine);
    }
    Input_ClearCharacterQueue();
    g_unk0x0081986c = 0;
}
// Update callback of the "select stage" screen: when the row in the first
// item moves, the list slides sideways by the distance between the old and
// the new row (the shortest way round) and eases back to the middle.
// match 59%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004f0620
void FrontendMenu_UpdateMultiplayerStageSelection(Menu *pMenu)
{
    int index;
    int old;
    int other;
    int steps;
    unsigned int elapsed;

    old = g_unk0x00819044;
    index = pMenu->items[0].max;
    if (index != old) {
        if (old != -1) {
            if (index > old) {
                other = index - old;
                steps = pMenu->items[0].min - index + old;
            } else {
                steps = old - index;
                other = pMenu->items[0].min - old + index;
            }
            if (steps < other) {
                while (steps + 1 > 1) {
                    g_unk0x00819868 += (int)(g_pGraphics->resX * -0x50) / 640;
                    steps--;
                }
            } else if (steps > other) {
                while (other > 0) {
                    g_unk0x00819868 += (int)(g_pGraphics->resX * 0x50) / 640;
                    other--;
                }
            } else {
                int resX = g_pGraphics->resX;
                if ((pMenu->moveFlags & 2) != 0)
                    other = resX * -5;
                else
                    other = resX * 5;
                g_unk0x00819868 += (other << 4) / 640;
            }
            g_unk0x00819884 = g_unk0x00819868;
        }
        g_unk0x0081975c = CFrontend::GetFrontendTimestamp();
        g_unk0x00819044 = pMenu->items[0].max;
    }
    if (g_unk0x00819044 != -1) {
        if ((unsigned int)(CFrontend::GetFrontendTimestamp() - g_unk0x0081975c) > 0xfa) {
            g_unk0x00819868 = 0;
            return;
        }
        if (g_unk0x00819884 > 0)
            g_unk0x00819868 = g_unk0x00819884 - (unsigned int)((CFrontend::GetFrontendTimestamp() - g_unk0x0081975c) * g_unk0x00819884) / 0xfa;
        else
            g_unk0x00819868 = (unsigned int)(-((CFrontend::GetFrontendTimestamp() - g_unk0x0081975c) * g_unk0x00819884)) / 0xfa + g_unk0x00819884;
    }
}

// FUNCTION: CMR2 0x004f0e60
void FrontendMenu_LeaveProfileRenameEntry(Menu *pMenu, int param)
{
    Frontend_SetOverlayMode(0);
    Menu_SetActionLatch(0);
}

Menu *FrontendProfile_GetRenameMenu(void);
void RallyData_SetEditedDriverOrCategoryName(unsigned int slot, char *pName);

// Characters of the three name-picker rows (a-j, k-t and u-z plus '.', space
// and the delete/OK entries).
// GLOBAL: CMR2 0x00525374
char g_nameRow0x00525374[12] = "abcdefghij";
// GLOBAL: CMR2 0x00525380
char g_nameRow0x00525380[12] = "klmnopqrst";
// GLOBAL: CMR2 0x0052538c
char g_nameRow0x0052538c[12] = "uvwxyz. <_";

// Item picker of the name entry screens: appends the character of the picked
// column to the name; the '<' entry deletes the last character and '_'
// accepts the name.
// FUNCTION: CMR2 0x004f1040
void FrontendProfile_PickNameCharacter(Menu *pMenu, int param)
{
    char *chars;
    int len;

    strcpy(CFrontend::m_stringDest, (char *)RallyData_GetRecord(FrontendProfile_GetCurrentPlayer()));
    len = strlen(CFrontend::m_stringDest);
    if (pMenu->items[pMenu->cursor].value == 2 && g_nameRow0x0052538c[pMenu->items[2].max] == '<') {
        if (CFrontend::m_stringDest[0] != 0) {
            CFrontend::m_stringDest[strlen(CFrontend::m_stringDest) - 1] = 0;
            Menu_PlaySoundId(2);
        }
    } else if (pMenu->items[pMenu->cursor].value == 2 && g_nameRow0x0052538c[pMenu->items[2].max] == '_') {
        if (len > 0)
            Menu_SetNextAction(FrontendProfile_GetRenameMenu());
        return;
    } else if (len < 3) {
        switch (pMenu->items[pMenu->cursor].value) {
        case 2:
            chars = g_nameRow0x0052538c;
            break;
        case 1:
            chars = g_nameRow0x00525380;
            break;
        case 0:
            chars = g_nameRow0x00525374;
            break;
        }
        CFrontend::m_stringDest[len] = chars[pMenu->items[pMenu->cursor].max];
        CFrontend::m_stringDest[len + 1] = 0;
        if (len == 2) {
            pMenu->cursor = len;
            pMenu->items[2].max = 9;
        }
        Menu_PlaySoundId(1);
    } else {
        Menu_PlaySoundId(3);
    }
    RallyData_SetEditedDriverOrCategoryName(FrontendProfile_GetCurrentPlayer(), CFrontend::m_stringDest);
}

// FUNCTION: CMR2 0x004f1160
void FrontendMenu_EnterProfileRenameEntry(Menu *pMenu, char param)
{
    g_unk0x00819040 = CMain::GetFrameDelta();
    g_unk0x0081986c = 1;
    Menu_SetActionLatch(1);
    g_unk0x00819038 = 1;
    Frontend_SetOverlayMode(FrontendProfile_GetCurrentPlayer());
    if (param != 0) {
        pMenu->cursor = 2;
        pMenu->items[2].max = pMenu->items[2].min - 1;
    } else {
        pMenu->cursor = 0;
        pMenu->items[0].max = 0;
    }
    Input_ClearCharacterQueue();
    pMenu->cursor = 2;
    pMenu->items[2].max = pMenu->items[2].min - 1;
}

// GLOBAL: CMR2 0x005253f4
char g_nameChars0x005253f4[] = "abcdefghijklmnopqrstuvwxyz. ABCDEFGHIJKLMNOPQRSTUVWXYZ";
// GLOBAL: CMR2 0x0052542c
char g_upperChars0x0052542c[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

void *RallyData_GetAssignedCategoryRecord(unsigned int param1);
void RallyData_CopyCategoryRecordName(BYTE index, char *name);

// Name entry of the category record: typed letters also move the cursor of
// the three letter rows (a-j, k-t, u-z plus '.', space, delete and OK).
// FUNCTION: CMR2 0x004f11d0
void FrontendMenu_UpdateProfileRenameEntry(Menu *pMenu)
{
    int key;
    int len;

    if (!g_unk0x00819038)
        return;
    strcpy(CFrontend::m_stringDest, (char *)RallyData_GetAssignedCategoryRecord(FrontendProfile_GetCurrentPlayer()));
    if (Input_PopQueuedCharacter(&key)) {
        if (key != 8) {
            len = strlen(CFrontend::m_stringDest);
            if (len >= 0x2b || strchr(g_nameChars0x005253f4, (char)key) == NULL)
                goto done;
            if (strchr(g_upperChars0x0052542c, (char)key) != NULL)
                key += 0x20;
            CFrontend::m_stringDest[len] = (char)key;
            CFrontend::m_stringDest[len + 1] = 0;
            if ((char)key >= 'a' && (char)key <= 'j') {
                pMenu->cursor = 0;
                pMenu->items[0].max = (char)key - 'a';
            }
            if ((char)key >= 'k' && (char)key <= 't') {
                pMenu->cursor = 1;
                pMenu->items[1].max = (char)key - 'k';
            }
            if ((char)key >= 'u' && (char)key <= 'z') {
                pMenu->cursor = 2;
                pMenu->items[2].max = (char)key - 'u';
            }
            if ((char)key == '.') {
                pMenu->cursor = 2;
                pMenu->items[2].max = 7;
            } else if ((char)key == ' ') {
                pMenu->cursor = 2;
                pMenu->items[2].max = 8;
            }
            g_unk0x00819040 = CMain::GetFrameDelta();
            g_unk0x0081986c = 1;
            Menu_PlaySoundId(1);
        } else {
            if (CFrontend::m_stringDest[0] != 0) {
                CFrontend::m_stringDest[strlen(CFrontend::m_stringDest) - 1] = 0;
                Menu_PlaySoundId(2);
            }
            pMenu->cursor = 2;
            pMenu->items[2].max = 8;
            g_unk0x00819040 = CMain::GetFrameDelta();
            g_unk0x0081986c = 1;
        }
    } else {
        if (!g_unk0x0081986c || CMain::GetFrameDelta() - g_unk0x00819040 <= 10)
            goto done;
        g_unk0x00819040 = -1;
        pMenu->cursor = 2;
        pMenu->items[2].max = 9;
        g_unk0x0081986c = 0;
        Menu_PlaySoundId(0);
    }
done:
    RallyData_CopyCategoryRecordName(FrontendProfile_GetCurrentPlayer(), CFrontend::m_stringDest);
    strcpy((char *)g_profileEnteredName, CFrontend::m_stringDest);
}

void Profile_ResetRecordCategory(int index);
void FrontendProfile_SetCurrentPlayer(int value);
Menu *FrontendProfile_GetCompletionMenu(void);
BYTE RallyData_FindCheatNameIndex(int param_1, BYTE *param_2);
char RallyData_SaveEditedCategoryProfile(int param_1);
BYTE Profile_LoadAndLinkSavedRecord(int param_1, int param_2);
void FrontendProfile_SetDateOfBirth(int param_1, unsigned int param_2, unsigned int param_3, unsigned int param_4);
BYTE RallyData_IsDriverRecordUsable(BYTE param1);

extern BYTE g_newProfileUndoPending;

// Item picker of the second name-entry screen: appends the picked character of
// the three letter rows to the name; the '<' entry deletes the last character
// and '_' accepts the entry (when its buffer is a known cheat code, the cheat
// is applied).
// match 76%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// logic verified against the dump; the remaining diff is register assignment (the original frees an
// extra callee-saved register and materialises the sound id with two pushes instead of a branchless
// select) and MSVC resolving `m_stringDest - 2` to a different symbol on our side.
// FUNCTION: CMR2 0x004f13c0
void FrontendProfile_PickRenameCharacterOrCheat(Menu *pMenu, int param)
{
    char *chars;
    char key;
    int len;

    strcpy(CFrontend::m_stringDest, (char *)RallyData_GetAssignedCategoryRecord(FrontendProfile_GetCurrentPlayer()));
    len = strlen(CFrontend::m_stringDest);
    if (pMenu->items[pMenu->cursor].value == 2 && g_nameRow0x0052538c[pMenu->items[2].max] == '<') {
        if (CFrontend::m_stringDest[0] != 0) {
            *(CFrontend::m_stringDest + strlen(CFrontend::m_stringDest) - 1) = 0;
            Menu_PlaySoundId(2);
        }
    } else if (pMenu->items[pMenu->cursor].value == 2 && g_nameRow0x0052538c[pMenu->items[2].max] == '_') {
        if (RallyData_FindCheatNameIndex(FrontendProfile_GetCurrentPlayer(), (BYTE *)&key) != 0) {
            if (key != 0)
                Menu_PlaySoundId(0);
            else
                Menu_PlaySoundId(3);
            if (RallyData_IsDriverRecordUsable((BYTE)FrontendProfile_GetCurrentPlayer())) {
                g_unk0x00819744--;
                RallyData_SetPlayerProfileInUse(FrontendProfile_GetCurrentPlayer(), 0);
            }
            Profile_ResetCategoryData(FrontendProfile_GetCurrentPlayer());
            Profile_ResetRecordCategory(FrontendProfile_GetCurrentPlayer());
            g_profilePlayersRemaining++;
            if (FrontendRecords_GetProfileMode())
                Menu_SetNextAction(FrontendMenu_GetRallyProfile());
            else
                Menu_SetNextAction(FrontendMenu_GetMultiplayerProfile());
            g_unk0x00819038 = 0;
            return;
        }
        if (len > 0) {
            Menu_SetNextAction(FrontendMenu_GetProfileDateEntry());
            g_unk0x00819038 = 0;
        }
        return;
    } else if (len < 0x2b) {
        switch (pMenu->items[pMenu->cursor].value) {
        case 2:
            chars = g_nameRow0x0052538c;
            break;
        case 1:
            chars = g_nameRow0x00525380;
            break;
        case 0:
            chars = g_nameRow0x00525374;
            break;
        }
        CFrontend::m_stringDest[len] = chars[pMenu->items[pMenu->cursor].max];
        CFrontend::m_stringDest[len + 1] = 0;
        Menu_PlaySoundId(1);
    } else {
        Menu_PlaySoundId(3);
    }
    RallyData_CopyCategoryRecordName(FrontendProfile_GetCurrentPlayer(), CFrontend::m_stringDest);
    strcpy((char *)g_profileEnteredName, CFrontend::m_stringDest);
}

// Left/right item callback of the name-entry screen: recomputes the column
// offsets of the three letter rows and steers the next screen when the name is
// complete.
// FUNCTION: CMR2 0x004f15d0
void FrontendProfile_ApplyDateEntry(Menu *pMenu, int param)
{
    FrontendProfile_SetDateOfBirth(FrontendProfile_GetCurrentPlayer(), pMenu->items[2].max + 1, pMenu->items[1].max + 1,
                 pMenu->items[0].max);
    if (RallyData_SaveEditedCategoryProfile(FrontendProfile_GetCurrentPlayer()) != 0) {
        Menu_SetNextAction(FrontendProfile_GetCompletionMenu());
        g_newProfileUndoPending = 0;
        if (FrontendRecords_GetProfileMode()) {
            RallyData_SetPlayerProfileInUse(FrontendProfile_GetCurrentPlayer(), 0);
            Profile_ResetRecordCategory(FrontendProfile_GetCurrentPlayer());
        }
    }
}

// Days in the month of the date edited by items 0 (year), 1 (month) and 2 (day)
// FUNCTION: CMR2 0x004f1640
void FrontendMenu_UpdateProfileDateEntry(Menu *pMenu)
{
    int days;
    int day = pMenu->items[2].max + 1;
    int year = pMenu->items[0].max + 1850;

    switch (pMenu->items[1].max) {
    case 3:
    case 5:
    case 8:
    case 10:
        pMenu->items[2].min = days = 30;
        break;
    case 1:
        if (year % 4 == 0)
            pMenu->items[2].min = days = 29;
        else
            pMenu->items[2].min = days = 28;
        break;
    case 0:
    case 2:
    case 4:
    case 6:
    case 7:
    case 9:
    case 11:
        pMenu->items[2].min = days = 31;
        break;
    }
    if (day > days)
        pMenu->items[2].max = days - 1;
}


// Maps a screen index to its palette/colour id and stores it for the current mode.
void RallyData_SetDriverCategoryOption(BYTE index, BYTE value);

// Update callback of the palette menu: shows the colour of the palette the
// profile uses and resets the scrolled list.
// FUNCTION: CMR2 0x004f1960
void FrontendMenu_EnterPaletteSelection(Menu *pMenu, char param)
{
    pMenu->items[0].max = RallyData_GetDriverAutomaticGearbox(CGameInfo::GetConfiguredPlayerCount() + (0xff - g_profilePlayersRemaining));
    Frontend_SetOverlayMode((CGameInfo::GetConfiguredPlayerCount() & 0xff) - (g_profilePlayersRemaining & 0xff) - 1);
    if (param != 0 && CGameInfo::GetConfiguredGameMode() != 4)
        FrontendScroller_InitRallyStageSelection(FrontendScroller_GetRouteEntryScroller()->pMenu, 0);
}

// Keeps the palette screen's OK item disabled while the palette id is invalid.
// FUNCTION: CMR2 0x004f19d0
void FrontendMenu_EnterPaletteConfirmation(Menu *pMenu, int param)
{
    if (RallyData_GetDriverAutomaticGearbox(CGameInfo::GetConfiguredPlayerCount() + (0xff - g_profilePlayersRemaining)) != 0)
        pMenu->items[0].max = 1;
    else
        pMenu->items[0].max = 0;
}
void RallyData_GetDriverDateOfBirth(int index, unsigned int *pHue, unsigned int *pShade, unsigned int *pValue);

// Callback of the car colour menu: reads the current driver's category colour
// and reflects it onto the colour picker items.
// FUNCTION: CMR2 0x004f16f0
void FrontendProfile_FillDateOfBirthPicker(Menu *pMenu, int param)
{
    int shade;
    int value;
    int hue;
    BYTE idx;

    Frontend_SetOverlayMode((CGameInfo::GetConfiguredPlayerCount() & 0xff) - (g_profilePlayersRemaining & 0xff) - 1);
    RallyData_GetDriverDateOfBirth(FrontendProfile_GetCurrentPlayer(), (unsigned int *)&hue, (unsigned int *)&shade, (unsigned int *)&value);
    pMenu->items[2].max = 0;
    pMenu->cursor = 0;
    pMenu->items[1].max = (BYTE)shade - 1;
    pMenu->items[0].max = value;
    idx = pMenu->items[1].max;
    switch (idx) {
    case 3:
    case 5:
    case 8:
    case 0xa:
        pMenu->items[2].min = 0x1e;
        return;
    case 1:
        break;
    case 0:
    case 2:
    case 4:
    case 6:
    case 7:
    case 9:
    case 0xb:
        pMenu->items[2].min = 0x1f;
        return;
    default:
        return;
    }
    if (value % 4 == 0)
        pMenu->items[2].min = 0x1d;
    else
        pMenu->items[2].min = 0x1c;
}
// Enter callback of a rally menu: lists the stages of the rally that are in
// the save as text ids, points the cursor at the one that is selected in the
// rally data and measures them for the scroller.
// match 86%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004f17d0
void FrontendScroller_InitRallyStageSelection(Menu *pMenu, int param)
{
    MenuScroller *pScroller;
    MenuItem *pItem;
    MenuItem *p;
    int *pPalette;
    int *pWidths;
    int i;
    int count;

    count = 0;
    Frontend_SetOverlayMode(CGameInfo::GetConfiguredPlayerCount() - g_profilePlayersRemaining - 1);
    pScroller = FrontendScroller_GetRouteEntryScroller();
    pScroller->pMenu = pMenu;
    pPalette = g_screenPaletteIds;
    pItem = pMenu->items;
    pMenu->cursor = 0;
    for (i = 0, p = pItem; i < 0x16; i++) {
        if (RallyData_HasCategoryAward(CGameInfo::GetConfiguredPlayerCount() - g_profilePlayersRemaining - 1, i, 1)) {
            *pPalette = i;
            p->id = i + 0x98;
            if (i == (RallyData_GetDriverRecordSelectionValue(CGameInfo::GetConfiguredPlayerCount() - g_profilePlayersRemaining - 1) & 0xff))
                pMenu->cursor = count;
            count++;
            pPalette++;
            p++;
        }
    }
    pMenu->itemCount = count;
    pScroller = FrontendScroller_GetRouteEntryScroller();
    pScroller->offset = 0;
    pScroller->startOffset = 0;
    pScroller->current = pMenu->cursor;
    pScroller->previous = pMenu->cursor;
    pScroller->spacing = PATH_X();
    pScroller->count = pMenu->itemCount;
    pScroller->offset = 0;
    pScroller->startOffset = 0;
    for (i = 0, p = pItem, pWidths = g_menuScroller0x00819230.widths; i < pScroller->count; i++) {
        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(p->id));
        CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
        *pWidths = Font_GetTextWidth(2, (BYTE *)CFrontend::m_stringDest);
        pWidths++;
        p++;
    }
}

void GameInfo_ResetSessionTimestamp(void);
void FrontendMenu_EnablePlayerSetupItems(void);
void FrontendProfile_SetupNextPlayer(Menu *pMenu, int param);

// Item action of the palette menu: opens the palette editor of the current
// screen mode.
// FUNCTION: CMR2 0x004f1a40
void FrontendProfile_OpenPaletteEditor(Menu *pMenu, int param)
{
    Frontend_SetOverlayMode(0);
    RallyData_SetDriverAutomaticGearbox(CGameInfo::GetConfiguredPlayerCount() + (0xff - g_profilePlayersRemaining), pMenu->items[0].max);
    if (g_profilePlayersRemaining == 0) {
        switch (CGameInfo::GetConfiguredGameMode()) {
        case 0:
            Menu_SetNextAction(FrontendMenu_GetRallyStartTransition());
            GameInfo_ResetSessionTimestamp();
            return;
        case 1:
        case 2:
        case 3:
            FrontendMenu_EnablePlayerSetupItems();
            Menu_SetParent(FrontendMenu_GetRallySelection(), pMenu);
            Menu_SetNextAction(FrontendMenu_GetRallySelection());
            return;
        case 4:
            Menu_SetParent(FrontendMenu_GetMultiplayerRaceSettings(), pMenu);
            Menu_SetNextAction(FrontendMenu_GetMultiplayerRaceSettings());
            return;
        }
    } else {
        Menu_SetParent(FrontendMenu_GetMultiplayerProfile(), pMenu);
        if (CGameInfo::GetConfiguredGameMode() != 4) {
            Menu_SetNextAction(FrontendMenu_GetMultiplayerProfile());
            return;
        }
        FrontendProfile_SetupNextPlayer(pMenu, param);
        Menu_SetNextAction(FrontendMenu_GetProfileNameEntry());
    }
}

// FUNCTION: CMR2 0x004f1a10
void FrontendProfile_SelectPalette(Menu *pMenu, int unused)
{
    RallyData_SetDriverCategoryOption(
        CGameInfo::GetConfiguredPlayerCount() + (0xff - g_profilePlayersRemaining),
        (unsigned char)g_screenPaletteIds[pMenu->cursor]);
}

// Item callback of the championship entry screens: toggles the selected
// entry's championship flag and moves to the next or the parent menu.
// FUNCTION: CMR2 0x004f1b30
void FrontendChampionship_ToggleAndAdvanceEntry(Menu *pMenu, int param)
{
    RallyData_SetDriverAutomaticGearbox(CGameInfo::GetConfiguredPlayerCount() + (0xff - g_profilePlayersRemaining), pMenu->items[0].max);
    if (g_profilePlayersRemaining == 0) {
        FrontendMenu_EnablePlayerSetupItems();
        Menu_SetNextAction(FrontendMenu_GetMultiplayerRaceSettings());
        return;
    }
    Menu_SetParent(FrontendMenu_GetMultiplayerProfile(), pMenu);
    Menu_SetNextAction(FrontendMenu_GetMultiplayerProfile());
}

// FUNCTION: CMR2 0x004f1b90
void FrontendMenu_ClearOverlayMode(Menu *pMenu, int param)
{
    Frontend_SetOverlayMode(0);
}

// FUNCTION: CMR2 0x004f1ba0
BYTE FrontendProfile_GetPlayersRemaining(void)
{
    return g_profilePlayersRemaining;
}

// FUNCTION: CMR2 0x004f1bb0
void FrontendProfile_SetPlayersRemaining(int value)
{
    g_profilePlayersRemaining = value;
}

// FUNCTION: CMR2 0x004f1bc0
void FrontendProfile_SetReturnMenu(Menu *value)
{
    g_pProfileReturnMenu = value;
}

// FUNCTION: CMR2 0x004f1bd0
void FrontendMenu_EnterCredits(Menu *pMenu, int param)
{
    g_unk0x00819864 = 1;
    g_creditsScrollOffset = 0;
    g_creditsStartTime = CFrontend::GetFrontendTimestamp();
}

// FUNCTION: CMR2 0x004f1bf0
int FrontendCredits_GetStartTime(void)
{
    return g_creditsStartTime;
}

// FUNCTION: CMR2 0x004f1c00
void FrontendMenu_EnterGraphicsOptions(Menu *pMenu, int param)
{
    if (CFrontend::GetDeviceCapabilityFieldA8()) {
        if ((BYTE)CGameInfo::GetGraphicsOptionBit4() && (g_pGraphics->field913_0x3bc & 0x40))
            Menu_GetItem(pMenu, 1)->max = 0;
        else
            Menu_GetItem(pMenu, 1)->max = 1;
        Menu_GetItem(pMenu, 1)->enabled = 1;
    } else {
        Menu_GetItem(pMenu, 1)->enabled = 0;
    }
    if (CGameInfo::GetGraphicsOptionBits18To19() == 1 && (g_pGraphics->field913_0x3bc & 1))
        Menu_GetItem(pMenu, 4)->max = 1;
    else if (CGameInfo::GetGraphicsOptionBits18To19() == 2 && (g_pGraphics->field913_0x3bc & 2))
        Menu_GetItem(pMenu, 4)->max = 0;
    else
        Menu_GetItem(pMenu, 4)->max = 2;
    CGameInfo::GetGraphicsOptionBits21To24();
    if ((unsigned int)CGameInfo::GetGraphicsOptionBits21To24() > 9)
        CGameInfo::SetGraphicsOptionBits21To24(9);
    Menu_GetItem(pMenu, 5)->max = CGameInfo::GetGraphicsOptionBits21To24();
}

// FUNCTION: CMR2 0x004f1d00
void FrontendMenu_EnterAdvancedGraphicsOptions(Menu *pMenu, int param)
{
    Menu_GetItem(pMenu, 8)->max = CGameInfo::GetGraphicsOptionBits25To26();
    if (CGameInfo::GetGraphicsOptionBits27To28() != 0)
        Menu_GetItem(pMenu, 9)->max = 1;
    else
        Menu_GetItem(pMenu, 9)->max = 0;
    if (CGameInfo::GetPreviewMode() != 0) {
        Menu_GetItem(pMenu, 10)->max = 1;
        return;
    }
    Menu_GetItem(pMenu, 10)->max = 0;
}

// FUNCTION: CMR2 0x004f1d60
void FrontendMenu_UpdateAdvancedGraphicsOptions(Menu *pMenu)
{
    if (CFrontend::IsTextureWidthSupported(0x400) && CFrontend::IsTextureHeightSupported(0x400))
        return;
    if (pMenu->items[pMenu->cursor].value == 10) {
        Menu_SetFlags(pMenu, 1, 0, 1, 1);
        return;
    }
    Menu_SetFlags(pMenu, 1, 1, 1, 1);
}

// FUNCTION: CMR2 0x004f1db0
void FrontendMenu_ApplyAdvancedGraphicsOptions(Menu *pMenu, int param)
{
    CGameInfo::SetGraphicsOptionBits25To26(Menu_GetItem(pMenu, 8)->max);
    switch (Menu_GetItem(pMenu, 9)->max) {
    case 1:
        CGameInfo::SetGraphicsOptionBits27To28(1);
        break;
    case 0:
        CGameInfo::SetGraphicsOptionBits27To28(0);
        break;
    }
    switch (Menu_GetItem(pMenu, 10)->max) {
    case 1:
        CGameInfo::SetPreviewMode(2);
        break;
    case 0:
        if (CFrontend::IsTextureWidthSupported(0x400) && CFrontend::IsTextureHeightSupported(0x400))
            CGameInfo::SetPreviewMode(0);
        else
            CGameInfo::SetPreviewMode(2);
        break;
    }
}

// FUNCTION: CMR2 0x004f1e40
void FrontendMenu_ApplyGraphicsOptions(Menu *pMenu, MenuItem *pItem)
{
    if (CFrontend::GetDeviceCapabilityFieldA8()) {
        if (Menu_GetItem(pMenu, 1)->max == 0) {
            CGameInfo::SetGraphicsOptionBit4(1);
            g_pGraphics->field913_0x3bc |= 0x40;
        } else {
            CGameInfo::SetGraphicsOptionBit4(0);
            g_pGraphics->field913_0x3bc &= ~0x40;
        }
    }
    switch (Menu_GetItem(pMenu, 4)->max) {
    case 0:
        CGameInfo::SetGraphicsOptionBits18To19(2);
        g_pGraphics->field913_0x3bc &= ~1;
        g_pGraphics->field913_0x3bc |= 2;
        break;
    case 1:
        CGameInfo::SetGraphicsOptionBits18To19(1);
        g_pGraphics->field913_0x3bc |= 1;
        g_pGraphics->field913_0x3bc &= ~2;
        break;
    case 2:
        CGameInfo::SetGraphicsOptionBits18To19(0);
        g_pGraphics->field913_0x3bc &= ~1;
        g_pGraphics->field913_0x3bc &= ~2;
        break;
    }
    CGameInfo::SetGraphicsOptionBits21To24(Menu_GetItem(pMenu, 5)->max);
    g_pGraphics->field917_0x3c0 = Menu_GetItem(pMenu, 5)->max;
}

// FUNCTION: CMR2 0x004f1f70
void FrontendMenu_LeaveGraphicsOptions(Menu *pMenu, char param)
{
    if (param == 0)
        FrontendMenu_ApplyGraphicsOptions(pMenu, &pMenu->items[pMenu->cursor]);
}

DWORD Graphics_GetDeviceCaps98(void);
DWORD Graphics_GetDeviceCaps9C(void);
DWORD Graphics_GetDeviceCapsA4(void);

// Callback of the options menu: enables the items supported by the current
// device and puts the cursor on the active option group.
// FUNCTION: CMR2 0x004f1fa0
void FrontendMenu_EnterRenderDeviceOptions(Menu *pMenu, int param)
{
    pMenu->items[0].min = 4;
    pMenu->items[0].enabled |= 1;
    pMenu->items[1].enabled = Graphics_GetDeviceCaps98();
    pMenu->items[2].enabled = Graphics_GetDeviceCaps98() && Graphics_GetDeviceCaps9C();
    pMenu->items[3].enabled = Graphics_GetDeviceCaps98() && Graphics_GetDeviceCapsA4();
    switch (CGameInfo::GetGraphicsOptionBits1To2()) {
    case 1:
        pMenu->cursor = 1;
        return;
    case 2:
        pMenu->cursor = 2;
        return;
    case 3:
        pMenu->cursor = 3;
        return;
    default:
        pMenu->cursor = 0;
        return;
    }
}

// FUNCTION: CMR2 0x004f2050
void FrontendMenu_ApplyRenderDeviceOptions(Menu *pMenu, int param)
{
    switch (pMenu->cursor) {
    case 0:
        CGameInfo::SetGraphicsOptionBits1To2(0);
        g_pGraphics->field913_0x3bc &= ~8;
        g_pGraphics->field913_0x3bc &= ~0x10;
        g_pGraphics->field913_0x3bc &= ~0x80;
        break;
    case 1:
        CGameInfo::SetGraphicsOptionBits1To2(1);
        g_pGraphics->field913_0x3bc |= 8;
        g_pGraphics->field913_0x3bc &= ~0x10;
        g_pGraphics->field913_0x3bc &= ~0x80;
        break;
    case 2:
        CGameInfo::SetGraphicsOptionBits1To2(2);
        g_pGraphics->field913_0x3bc |= 8;
        g_pGraphics->field913_0x3bc |= 0x10;
        g_pGraphics->field913_0x3bc &= ~0x80;
        break;
    case 3:
        CGameInfo::SetGraphicsOptionBits1To2(3);
        g_pGraphics->field913_0x3bc |= 8;
        g_pGraphics->field913_0x3bc &= ~0x10;
        g_pGraphics->field913_0x3bc |= 0x80;
        break;
    }
    Menu_SetNextAction(pMenu->pParent);
}

// FUNCTION: CMR2 0x004f23c0
int FrontendMenu_GetFirstVisibleDisplayDevice(void)
{
    return g_firstVisibleDisplayDevice;
}

// FUNCTION: CMR2 0x004f23d0
void FrontendMenu_UpdateDisplayMode(Menu *pMenu)
{
    unsigned int count = CGraphics::GetDisplayCount();
    if (count > 10)
        count = 10;
    if (pMenu->items[0].max < g_firstVisibleDisplayDevice)
        g_firstVisibleDisplayDevice--;
    if (pMenu->items[0].max >= (int)(count + g_firstVisibleDisplayDevice))
        g_firstVisibleDisplayDevice++;
    if (CGame::GetGameInputFocusState() != 0)
        FrontendMenu_EnterDisplayMode(pMenu, 0);
}

void FrontendText_ReloadFonts(void);

// Item action of the video menu: switches the game to the display mode
// selected in the first row and rebuilds the frontend on top of it.
// FUNCTION: CMR2 0x004f2430
void FrontendMenu_ApplyDisplayMode(Menu *pMenu, int param)
{
    DWORD width;
    DWORD height;
    DWORD depth;
    int mode;

    CGraphics::GetDisplayMode(pMenu->items[0].max, &width, &height, &depth);
    CGameInfo::SetScreenWidth(width);
    CGameInfo::SetScreenHeight(height);
    CGameInfo::SetColourDepth(depth);
    CMain::SetGameActiveState(1);
    mode = CGraphics::GetSelectedRenderDeviceIndex();
    CGraphics::RecreateGraphicsDeviceAndResources(width, height, depth, CGameInfo::GetGraphicsOptionBits5To8(), mode);
    CGameInfo::SetScreenWidth(g_pGraphics->resX);
    CGameInfo::SetScreenHeight(g_pGraphics->resY);
    CGameInfo::SetColourDepth(g_pGraphics->depth);
    CFrontend::LoadFrontendResourceArchives();
    Graphics_ReloadAllTextures();
    FrontendText_ReloadFonts();
    FrontendScroller_RecomputeAllAttached();
    CMain::SetGameActiveState(0);
    Menu_SetNextAction(pMenu->pParent);
}

// FUNCTION: CMR2 0x004f24f0
MenuScroller *FrontendScroller_GetMainScroller(void)
{
    return &g_menuScroller0x00819140;
}

// FUNCTION: CMR2 0x004f2500
MenuScroller *FrontendScroller_GetRouteEntryScroller(void)
{
    return &g_menuScroller0x00819230;
}

// FUNCTION: CMR2 0x004f2510
MenuScroller *FrontendScroller_GetLanguageScroller(void)
{
    return &g_menuScroller0x008192a8;
}

// FUNCTION: CMR2 0x004f2520
MenuScroller *FrontendScroller_GetDifficultyScroller(void)
{
    return &g_menuScroller0x00819320;
}

// FUNCTION: CMR2 0x004f2530
MenuScroller *FrontendScroller_GetRallySelectionScroller(void)
{
    return &g_menuScroller0x00819398;
}

// FUNCTION: CMR2 0x004f2540
MenuScroller *FrontendScroller_GetStageRecordsScroller(void)
{
    return &g_menuScroller0x00819410;
}

// FUNCTION: CMR2 0x004f2550
MenuScroller *FrontendScroller_GetBestStageTimesScroller(void)
{
    return &g_menuScroller0x00819488;
}

// FUNCTION: CMR2 0x004f2560
MenuScroller *FrontendScroller_GetTransmissionRecordsScroller(void)
{
    return &g_menuScroller0x00819500;
}

// FUNCTION: CMR2 0x004f2570
MenuScroller *FrontendScroller_GetDisplayDeviceScroller(void)
{
    return &g_menuScroller0x00819578;
}

// FUNCTION: CMR2 0x004f2580
MenuScroller *FrontendScroller_GetAlternateStageScroller(void)
{
    return &g_menuScroller0x00819668;
}

// FUNCTION: CMR2 0x004f2590
MenuScroller *FrontendScroller_GetArcadeRallyScroller(void)
{
    return &g_menuScroller0x008195f0;
}

// FUNCTION: CMR2 0x004f25a0
int FrontendCredits_GetScrollOffset(void)
{
    return g_creditsScrollOffset;
}

// FUNCTION: CMR2 0x004f25b0
int FrontendScroller_GetSelection(void)
{
    return g_scrollerSelection;
}

// FUNCTION: CMR2 0x004f25c0
int FrontendScroller_GetSelectionTimestamp(void)
{
    return g_scrollerSelectionTimestamp;
}

// FUNCTION: CMR2 0x004f2840
void FrontendMenu_EnterSoundOptions(Menu *pMenu, int param)
{
    g_unk0x0081903c = CGameInfo::GetMasterSoundVolume();
    g_unk0x00819860 = CGameInfo::GetEffectsSoundVolume();
    g_unk0x00819754 = CGameInfo::GetCoDriverSoundVolume();
    pMenu->items[0].max = (int)CGameInfo::GetMasterSoundVolume() / 10;
    pMenu->items[1].max = (int)CGameInfo::GetEffectsSoundVolume() / 10;
    pMenu->items[2].max = (int)CGameInfo::GetCoDriverSoundVolume() / 10;
}

// FUNCTION: CMR2 0x004f2b70
void FrontendMenu_LeaveSoundOptions(Menu *pMenu, char param)
{
    if (param != 0) {
        CGameInfo::SetMasterSoundVolume(g_unk0x0081903c);
        CGameInfo::SetEffectsSoundVolume(g_unk0x00819860);
        CGameInfo::SetCoDriverSoundVolume(g_unk0x00819754);
        CInput::SetMenuSoundVolume((int)(CGameInfo::GetEffectsSoundVolume() << 16) / 100 / 4);
        CSound::NoOpSoundDeviceCallback();
        return;
    }
    FrontendMenu_UpdateSoundOptions(pMenu);
}

// FUNCTION: CMR2 0x004f2be0
int FrontendProfile_GetCurrentPlayer(void)
{
    return g_profileCurrentPlayer;
}

// FUNCTION: CMR2 0x004f2bf0
void FrontendProfile_SetCurrentPlayer(int value)
{
    g_profileCurrentPlayer = value;
}

// FUNCTION: CMR2 0x004f2c00
Menu *FrontendProfile_GetRenameMenu(void)
{
    return g_pProfileRenameMenu;
}

// FUNCTION: CMR2 0x004f2c10
void FrontendProfile_SetRenameMenu(Menu *value)
{
    g_pProfileRenameMenu = value;
}

// FUNCTION: CMR2 0x004f2c20
Menu *FrontendProfile_GetCompletionMenu(void)
{
    return g_pProfileCompletionMenu;
}

// FUNCTION: CMR2 0x004f2c30
void FrontendProfile_SetCompletionMenu(Menu *value)
{
    g_pProfileCompletionMenu = value;
}

// FUNCTION: CMR2 0x004f2c40
void FrontendMenu_EnterGameOptions(Menu *pMenu, char param)
{
    if (param == 0) {
        if (CGameInfo::IsSplitBarEnabled())
            pMenu->items[Menu_FindItem(pMenu, 0)].max = 1;
        else
            pMenu->items[Menu_FindItem(pMenu, 0)].max = 0;
        if ((BYTE)CGameInfo::GetSoundOptionBit30())
            pMenu->items[Menu_FindItem(pMenu, 2)].max = 0;
        else
            pMenu->items[Menu_FindItem(pMenu, 2)].max = 1;
        if ((BYTE)CGameInfo::IsDashOptionEnabled())
            pMenu->items[Menu_FindItem(pMenu, 3)].max = 1;
        else
            pMenu->items[Menu_FindItem(pMenu, 3)].max = 0;
        if (CGameInfo::GetNetworkOptionBits1To2() == 0)
            pMenu->items[Menu_FindItem(pMenu, 4)].max = 0;
        if (CGameInfo::GetNetworkOptionBits1To2() == 1)
            pMenu->items[Menu_FindItem(pMenu, 4)].max = 1;
        if (CGameInfo::GetNetworkOptionBits1To2() == 2)
            pMenu->items[Menu_FindItem(pMenu, 4)].max = 2;
    }
}


// Applies the front-end options from the menu items back to the game info.
void GameInfo_SetSoundOptionBit30(BYTE param1);
void GameInfo_SetNetworkOptionBits1To2(unsigned int param1);

// FUNCTION: CMR2 0x004f2d20
void FrontendMenu_ApplyGameOptions(Menu *pMenu, int param)
{
    int index;

    index = Menu_FindItem(pMenu, 0);
    CGameInfo::SetSplitBarEnabled(pMenu->items[index].max);
    index = Menu_FindItem(pMenu, 2);
    GameInfo_SetSoundOptionBit30(1 - pMenu->items[index].max);
    index = Menu_FindItem(pMenu, 3);
    CGameInfo::SetDashOptionEnabled(pMenu->items[index].max);
    index = Menu_FindItem(pMenu, 4);
    GameInfo_SetNetworkOptionBits1To2(pMenu->items[index].max);
}

// FUNCTION: CMR2 0x004f2d90
void FrontendScroller_InitStageRecords(Menu *pMenu, int param)
{
    MenuScroller *p;
    int i;

    p = FrontendScroller_GetStageRecordsScroller();
    p->startTime = CFrontend::GetFrontendTimestamp();
    p->pMenu = pMenu;
    p = FrontendScroller_GetStageRecordsScroller();
    p->spacing = (int)(g_pGraphics->resX * 24) / 640;
    p->count = pMenu->itemCount;
    p->offset = 0;
    p->startOffset = 0;
    for (i = 0; i < pMenu->itemCount; i++) {
        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pMenu->items[i].id));
        CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
        p->widths[i] = Font_GetTextWidth(2, (BYTE *)CFrontend::m_stringDest);
    }
}

// FUNCTION: CMR2 0x004f2e70
void FrontendScroller_InitBestStageTimes(Menu *pMenu, int param)
{
    MenuScroller *p;
    int i;

    p = FrontendScroller_GetBestStageTimesScroller();
    p->startTime = CFrontend::GetFrontendTimestamp();
    p->pMenu = pMenu;
    p->spacing = (int)(g_pGraphics->resX * 24) / 640;
    p->count = pMenu->itemCount;
    p->offset = 0;
    p->startOffset = 0;
    for (i = 0; i < pMenu->itemCount; i++) {
        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pMenu->items[i].id));
        CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
        p->widths[i] = Font_GetTextWidth(2, (BYTE *)CFrontend::m_stringDest);
    }
}

// FUNCTION: CMR2 0x004f2f40
void FrontendScroller_InitTransmissionRecords(Menu *pMenu, int param)
{
    MenuScroller *p;
    int i;

    p = FrontendScroller_GetTransmissionRecordsScroller();
    p->startTime = CFrontend::GetFrontendTimestamp();
    p->pMenu = pMenu;
    p->spacing = (int)(g_pGraphics->resX * 24) / 640;
    p->count = pMenu->itemCount;
    p->offset = 0;
    p->startOffset = 0;
    for (i = 0; i < pMenu->itemCount; i++) {
        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pMenu->items[i].id));
        CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
        p->widths[i] = Font_GetTextWidth(2, (BYTE *)CFrontend::m_stringDest);
    }
}

// FUNCTION: CMR2 0x004f3010
void FrontendMenu_EnterRallyStageSelection(Menu *pMenu, int param)
{
    MenuScroller *p;
    int i;

    for (i = 0; i < 12; i++) {
        if (g_unk0x00819130[i] == 1)
            pMenu->items[i].enabled = 1;
        else
            pMenu->items[i].enabled = 0;
    }
    p = FrontendScroller_GetDisplayDeviceScroller();
    p->startTime = CFrontend::GetFrontendTimestamp();
    p->count = pMenu->itemCount;
    p->pMenu = pMenu;
    p->offset = 0;
    p->startOffset = 0;
    p->current = pMenu->cursor;
    p->previous = pMenu->cursor;
    p->spacing = (int)(g_pGraphics->resX * 24) / 640;
    p->count = pMenu->itemCount;
    p->offset = 0;
    p->startOffset = 0;
    for (i = 0; i < pMenu->itemCount; i++) {
        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pMenu->items[i].id));
        CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
        p->widths[i] = Font_GetTextWidth(2, (BYTE *)CFrontend::m_stringDest);
    }
}

// FUNCTION: CMR2 0x004f3120
void FrontendMenu_EnterAlternateRallyStageSelection(Menu *pMenu, int param)
{
    MenuScroller *p;
    int i;

    for (i = 0; i < pMenu->itemCount; i++)
        pMenu->items[i].enabled = 1;
    p = FrontendScroller_GetAlternateStageScroller();
    p->startTime = CFrontend::GetFrontendTimestamp();
    p->count = pMenu->itemCount;
    p->pMenu = pMenu;
    p->spacing = (int)(g_pGraphics->resX * 24) / 640;
    p->count = pMenu->itemCount;
    p->offset = 0;
    p->startOffset = 0;
    for (i = 0; i < pMenu->itemCount; i++) {
        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pMenu->items[i].id));
        CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
        p->widths[i] = Font_GetTextWidth(2, (BYTE *)CFrontend::m_stringDest);
    }
}

// FUNCTION: CMR2 0x004f36e0
void FrontendMenu_EnterRallySelection(Menu *pMenu, int param)
{
    MenuScroller *p;
    int i;

    p = FrontendScroller_GetRallySelectionScroller();
    p->startTime = CFrontend::GetFrontendTimestamp();
    p->pMenu = pMenu;
    p->count = pMenu->itemCount;
    p->offset = 0;
    p->startOffset = 0;
    p->current = pMenu->cursor;
    p->previous = pMenu->cursor;
    p->spacing = (int)(g_pGraphics->resX * 24) / 640;
    p->count = pMenu->itemCount;
    p->offset = 0;
    p->startOffset = 0;
    for (i = 0; i < pMenu->itemCount; i++) {
        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(i + 0x27));
        CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
        p->widths[i] = Font_GetTextWidth(2, (BYTE *)CFrontend::m_stringDest);
    }
}

// Slides the scroller towards the menu cursor, the short way round, in 250 ms
// match 58%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004f37c0
void FrontendScroller_UpdateCursorSlide(MenuScroller *p)
{
    Menu *pMenu = p->pMenu;
    int target;
    int cur;
    int fwd;
    int back;
    int idx;
    int i;

    if (pMenu->cursor != p->current) {
        if (p->current != -1) {
            if (pMenu == FrontendMenu_GetMain()) {
                g_scrollerSelectionTimestamp = CFrontend::GetFrontendTimestamp();
                g_scrollerSelection = g_unk0x0052539c;
                g_unk0x0052539c = pMenu->cursor;
            }
            target = pMenu->cursor;
            cur = p->current;
            if (target > cur) {
                fwd = target - cur;
                back = pMenu->itemCount - target + cur;
            } else {
                back = cur - target;
                fwd = pMenu->itemCount - cur + target;
            }
            if (back < fwd) {
                while (back-- >= 1) {
                    cur--;
                    idx = cur;
                    if (idx < 0)
                        idx = p->count + cur;
                    p->offset -= p->widths[idx] + p->spacing;
                }
            } else if (back > fwd) {
                while (fwd-- > 0) {
                    idx = cur;
                    if (idx >= p->count)
                        idx = cur - p->count;
                    p->offset += p->widths[idx] + p->spacing;
                    cur++;
                }
            } else if (pMenu->moveFlags & 1) {
                cur--;
                if (cur < 0)
                    cur += p->count;
                p->offset -= p->widths[cur] + p->spacing;
            } else {
                if (cur >= p->count)
                    cur -= p->count;
                p->offset += p->widths[cur] + p->spacing;
            }
            p->startOffset = p->offset;
        }
        p->startTime = CFrontend::GetFrontendTimestamp();
        p->previous = p->current;
        p->current = pMenu->cursor;
    }
    if (p->current != -1) {
        if ((unsigned int)(CFrontend::GetFrontendTimestamp() - p->startTime) > 250) {
            p->offset = 0;
            p->startOffset = 0;
            return;
        }
        if (p->startOffset > 0) {
            p->offset = p->startOffset - (unsigned int)((CFrontend::GetFrontendTimestamp() - p->startTime) * p->startOffset) / 250;
            return;
        }
        p->offset = (unsigned int)-((CFrontend::GetFrontendTimestamp() - p->startTime) * p->startOffset) / 250 + p->startOffset;
    }
}

// FUNCTION: CMR2 0x004f3970
void FrontendScroller_UpdateStageRecords(Menu *pMenu)
{
    FrontendScroller_UpdateCursorSlide(FrontendScroller_GetStageRecordsScroller());
}

// FUNCTION: CMR2 0x004f3980
void FrontendScroller_UpdateBestStageTimes(Menu *pMenu)
{
    FrontendScroller_UpdateCursorSlide(FrontendScroller_GetBestStageTimesScroller());
}

// FUNCTION: CMR2 0x004f3990
void FrontendScroller_UpdateTransmissionRecords(Menu *pMenu)
{
    if (FrontendRecords_GetTableMode() == 0) {
        pMenu->cursor = 0;
        pMenu->items[1].enabled = 0;
    } else {
        pMenu->items[1].enabled = 1;
    }
    FrontendScroller_UpdateCursorSlide(FrontendScroller_GetTransmissionRecordsScroller());
}

// FUNCTION: CMR2 0x004f39d0
void FrontendScroller_UpdateChampionshipEntry(Menu *pMenu)
{
    FrontendScroller_UpdateCursorSlide(FrontendScroller_GetRouteEntryScroller());
}

// Applying the 3D car preview screen: when the preview is up, the block of the
// selected car is handed to the frontend state machine.
// FUNCTION: CMR2 0x004f3a00
void FrontendCar_ApplyPreviewSelection(Menu *pMenu, int param)
{
    char ok;

    if (g_unk0x0082aa40 > 0) {
        ok = FrontendProfile_ApplyDriverProfileBlock(SavedGames_GetRecord(g_savedGameListSelection));
        if (ok != 0)
            Frontend_SetDebugOverlayChannels(0, 1, 0);
    }
}

// FUNCTION: CMR2 0x004f3a30
BYTE FrontendChampionship_GetRouteSelectionFlag(void)
{
    return g_unk0x00819748;
}

// FUNCTION: CMR2 0x004f3a40
int FrontendChampionship_GetRouteIndexOffset(void)
{
    return g_unk0x00819050;
}

// FUNCTION: CMR2 0x004f3a60
int FrontendRecords_GetTableMode(void)
{
    return g_recordsTableMode;
}

// FUNCTION: CMR2 0x004f3a90
void FrontendRecords_CycleTableMode(Menu *pMenu, int param)
{
    g_recordsTableMode++;
    if (g_recordsTableMode > 2)
        g_recordsTableMode = 0;
    Menu_PlaySoundId(4);
}

// FUNCTION: CMR2 0x004f3ac0
void FrontendMenu_EnterProfileActions(Menu *pMenu, char param)
{
    if (param == 0)
        pMenu->cursor = 0;
}

// FUNCTION: CMR2 0x004f3ae0
void FrontendScroller_UpdateMain(Menu *pMenu)
{
    FrontendScroller_UpdateCursorSlide(FrontendScroller_GetMainScroller());
}

// FUNCTION: CMR2 0x004f3af0
BYTE *FrontendProfile_GetEnteredNameBuffer(void)
{
    return g_profileEnteredName;
}

// FUNCTION: CMR2 0x004f3b00
void FrontendMenu_EnterQuitConfirmation(Menu *pMenu, int param)
{
    g_quitCancelled = 0;
    pMenu->cursor = 1;
}

// FUNCTION: CMR2 0x004f3b20
void FrontendMenu_CancelQuit(Menu *pMenu, int param)
{
    g_quitCancelled = 1;
}

// FUNCTION: CMR2 0x004f3b30
void FrontendMenu_LeaveQuitConfirmation(Menu *pMenu, char param)
{
    if (g_quitCancelled == 0 && param == 0)
        Menu_SetNextAction(FrontendMenu_GetQuitTransition());
}

// Frontend sound names and path
// GLOBAL: CMR2 0x0052544c
char g_menuSoundNames[5][9] = {"move", "select", "back", "error", "toggle"};
// GLOBAL: CMR2 0x0051ea44
char g_strMenuSoundFormat[24] = "%s\\Sounds\\menu\\%s.wav";
// GLOBAL: CMR2 0x00525a84
char g_strMenuDotFormat[32] = "%s\\frontend\\Textures\\dot0%d.tga";

// Loads the five frontend sounds from the common frontend archive.
// FUNCTION: CMR2 0x004f3b50
BYTE FrontendAudio_LoadSounds(void)
{
    BYTE ok;
    int i;

    ok = 1;
    g_menuSoundBase = 0;
    for (i = 0; i < 5; i++) {
        sprintf(CFrontend::m_stringDest, g_strMenuSoundFormat, CInstallInfo::GetGameCDPath(), g_menuSoundNames[i]);
        if (!Sound_LoadSample(CFrontend::m_stringDest, 0, CFrontend::GetCommonFrontendArchive()))
            ok = 0;
    }
    return ok;
}

// Sets the input repeat rate from the options and binds the five frontend
// sounds to the menu actions.
// FUNCTION: CMR2 0x004f3bb0
void FrontendAudio_ConfigureMenuSounds(void)
{
    g_unk0x0081988c = -1;
    CInput::SetMenuSoundVolume((int)(CGameInfo::GetEffectsSoundVolume() << 16) / 100 / 4);
    CInput::SetMenuSounds(g_menuSoundBase, g_menuSoundBase + 1, g_menuSoundBase + 2, g_menuSoundBase + 3,
                         g_menuSoundBase + 4);
    Menu_SetInputStateFlag(1);
}

// Loads the four "dot" textures of the frontend.
// FUNCTION: CMR2 0x004f3f60
void FrontendAnimation_LoadDotTextures(void)
{
    int i;

    for (i = 0; i < 4; i++) {
        sprintf(CFrontend::m_stringDest, g_strMenuDotFormat, CInstallInfo::GetGameCDPath(), i);
        g_menuDotTextures[i] =
            CTexture::FindLoadTexture(CGenericFileLoader::GetGenericFile(), CFrontend::m_stringDest, 0, 0, 0, 0);
    }
}

// --- Main menu animation: the letters of "colinmcrae", a trail of dots and
// six dot streams run along a closed 16-segment spline; each kind of main
// menu entry has its own path and the path morphs into the next one over
// 250 ms when the cursor moves.

// Path used with cheat 0xe.
// GLOBAL: CMR2 0x00525490
int g_menuPathCheat[19][2] = {
    324, 157, 371, 152, 366, 91, 395, 108,
    463, 133, 391, 217, 405, 340, 385, 352,
    366, 276, 361, 330, 374, 337, 356, 356,
    239, 357, 225, 341, 245, 320, 246, 244,
    289, 181, 371, 152, 414, 54,
};
// One path (19 points, 16.16 spline control points in 640x480 units) per
// animation mode.
// GLOBAL: CMR2 0x00525528
int g_menuPaths[9][19][2] = {
    216, 133, 260, 100, 307, 89, 349, 95, 398, 127, 426, 172, 430, 229, 405, 306, 340, 314, 261, 315, 266, 279, 286, 224, 252, 194, 207, 197, 177, 192, 192, 176, 216, 133, 260, 100, 307, 89,
    536, 122, 520, 113, 479, 110, 437, 112, 494, 131, 468, 144, 404, 144, 349, 156, 439, 174, 509, 192, 404, 212, 219, 203, 75, 215, 153, 242, 142, 284, 234, 320, 400, 294, 536, 294, 648, 238,
    221, 108, 279, 104, 340, 108, 400, 117, 473, 148, 505, 192, 499, 240, 459, 274, 393, 272, 315, 262, 232, 271, 144, 291, 93, 254, 84, 205, 108, 154, 165, 121, 221, 108, 279, 104, 340, 108,
    49, 158, 512, 158, 608, 171, 542, 188, 608, 204, 544, 226, 608, 244, 543, 264, 587, 294, 110, 301, 27, 283, 85, 264, 25, 247, 87, 224, 24, 206, 87, 185, 49, 158, 512, 158, 608, 171,
    219, 115, 302, 88, 385, 112, 441, 188, 465, 312, 428, 322, 405, 278, 387, 235, 355, 216, 303, 239, 245, 216, 219, 232, 197, 276, 175, 322, 138, 303, 167, 189, 219, 115, 302, 88, 385, 112,
    270, 174, 332, 125, 382, 84, 390, 92, 391, 163, 410, 206, 409, 237, 394, 271, 394, 350, 374, 343, 326, 306, 269, 266, 203, 263, 187, 252, 186, 196, 197, 177, 270, 174, 332, 125, 382, 84,
    229, 74, 324, 67, 399, 69, 476, 82, 505, 114, 509, 201, 505, 289, 477, 323, 399, 336, 324, 341, 244, 338, 166, 323, 135, 289, 129, 211, 135, 115, 166, 82, 244, 71, 324, 67, 428, 73,
    291, 88, 312, 73, 329, 86, 326, 118, 354, 134, 367, 187, 350, 217, 342, 309, 360, 331, 313, 327, 268, 330, 284, 306, 271, 219, 253, 185, 267, 131, 294, 117, 291, 88, 312, 73, 329, 86,
    178, 224, 199, 282, 250, 326, 322, 334, 386, 306, 421, 254, 428, 191, 404, 143, 354, 174, 303, 210, 256, 243, 201, 276, 178, 225, 186, 163, 232, 107, 296, 86, 358, 99, 407, 140, 428, 191,
};
// Spline tension (tangents are scaled by 1 - tension).
// GLOBAL: CMR2 0x00525a80
int g_menuPathTension = 0x20000;
// GLOBAL: CMR2 0x00525480
char g_strColinMcrae[12] = "colinmcrae";
// Time of the last animation update, -1 = none yet.
// GLOBAL: CMR2 0x0052548c
int g_menuAnimTime = -1;

// GLOBAL: CMR2 0x00819990
int *g_pMenuPath;
// Position (0..1 along the path) of each of the 200 letters.
// GLOBAL: CMR2 0x00819994
int g_menuLetterPos[200];
// Path of the entry under the cursor.
// GLOBAL: CMR2 0x00819cb4
int g_menuPathMode;
// Six streams of 10 dots (head first).
// GLOBAL: CMR2 0x00819cb8
int g_menuStreamPos[6][10];
// Which of the three paths the entries of type 4 cycle through.
// GLOBAL: CMR2 0x00819da8
int g_menuPathVariant;
// Path being morphed from.
// GLOBAL: CMR2 0x00819dac
int g_menuPathPrevMode;
// GLOBAL: CMR2 0x00819db0
int g_menuStreamPoint[6][2];
// Speed of each stream (16.16, 0.5..1.5, random walk).
// GLOBAL: CMR2 0x00819de0
int g_menuStreamSpeed[6];
// Path while morphing from one mode to another.
// GLOBAL: CMR2 0x00819df8
int g_menuPathMorph[19][2];
// GLOBAL: CMR2 0x00819e90
int g_menuPathInit;
// GLOBAL: CMR2 0x0081a620
short g_menuDotRect[4];

// PORT: the last point of the path FrontendAnimation_EvaluatePathSpline
// worked out, in 16.16 640x480 units: it returns whole units only.
static int g_menuPathPointFine[2];

// PORT: a point of the path in 16.16 640x480 units on the screen: the whole
// pixel in pX, pY and its fraction for the next sprite, so that what moves
// along the path glides instead of stepping a 640x480 pixel at a time, two
// or more pixels of a larger screen.
static void FrontendAnimation_PlaceOnScreen(const int *pFine, int dx, int dy, short *pX, short *pY)
{
    long long x = (long long)g_pGraphics->resX * pFine[0] / 640 + ((long long)dx << 16);
    long long y = (long long)g_pGraphics->resY * pFine[1] / 480 + ((long long)dy << 16);

    *pX = (short)(x >> 16);
    *pY = (short)(y >> 16);
    Sprite_SetNextSubpixelOffset((int)(x & 0xffff), (int)(y & 0xffff));
}

// Point of the path at t (16.16, 0..1): cubic Hermite spline through the
// control points 1..17 with Catmull-Rom style tangents.
// match 54%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004f3c10
void FrontendAnimation_EvaluatePathSpline(int *pPoints, int t, int *pOut)
{
    int *p;
    int seg;
    int u;
    int u2;
    int u3;
    int h00;
    int h01;
    int h10;
    int h11;
    int tanA;
    int tanB;
    int x;

    if (t == 0) {
        pOut[0] = pPoints[2];
        pOut[1] = pPoints[3];
        g_menuPathPointFine[0] = pOut[0] << 16; // PORT
        g_menuPathPointFine[1] = pOut[1] << 16;
        return;
    }
    if (t == 0x10000) {
        pOut[0] = pPoints[0x22];
        pOut[1] = pPoints[0x23];
        g_menuPathPointFine[0] = pOut[0] << 16; // PORT
        g_menuPathPointFine[1] = pOut[1] << 16;
        return;
    }
    seg = ((t << 4) >> 16) + 1;
    u = (((seg << 16) - 0x10000) / 16 - t + 0x1000) << 4;
    u2 = FixMul(u, u);
    u3 = FixMul(u, u2);
    h00 = u3 * 2 + 0x10000 - u2 * 3;
    h01 = u2 * 3 - u3 * 2;
    h10 = u3 - u2 * 2 + u;
    h11 = u3 - u2;
    p = &pPoints[seg * 2];
    tanA = (p[2] - p[-2]) * (0x10000 - g_menuPathTension) / 2;
    tanB = (p[4] - p[0]) * (0x10000 - g_menuPathTension) / 2;
    x = p[2] * h00 + (FixMul(h11, tanA) + p[0] * h01);
    x += FixMul(h10, tanB);
    tanA = (p[3] - p[-1]) * (0x10000 - g_menuPathTension) / 2;
    tanB = (p[5] - p[1]) * (0x10000 - g_menuPathTension) / 2;
    pOut[0] = x >> 16;
    pOut[1] = (p[3] * h00 + (FixMul(h11, tanA) + p[1] * h01) + FixMul(h10, tanB)) >> 16;
    g_menuPathPointFine[0] = x; // PORT
    g_menuPathPointFine[1] = p[3] * h00 + (FixMul(h11, tanA) + p[1] * h01) + FixMul(h10, tanB);
}

// Resets the main menu animation: letters spread along the path, dots at
// the start, and the path of the entry under the cursor.
// FUNCTION: CMR2 0x004f3dd0
void FrontendAnimation_ResetMainPath(void)
{
    int *pPos;
    int i;
    int j;
    int k;

    i = 0;
    pPos = g_menuLetterPos;
    do {
        *pPos++ = (i << 16) / 200;
        i++;
    } while ((INT_PTR)pPos < (INT_PTR)(g_menuLetterPos + 200)); // 0x819cb4 in the original
    memset(g_menuTrailPos, 0, sizeof(g_menuTrailPos));
    k = 0;
    pPos = g_menuStreamPos[0];
    memset(g_menuStreamSpeed, 0, sizeof(g_menuStreamSpeed));
    do {
        for (j = 0; j < 10; j++)
            pPos[j] = k / 6;
        pPos += 10;
        k += 0x4000;
    } while ((INT_PTR)pPos < (INT_PTR)&g_menuStreamPos[6][0]); // 0x819da8 in the original
    g_menuAnimTime = -1;
    switch (FrontendMenu_GetMain()->items[FrontendMenu_GetMain()->cursor].value) {
    case 0:
        g_menuPathVariant = g_menuPathPrevMode = g_menuPathMode = 0;
        break;
    case 1:
        g_menuPathVariant = g_menuPathPrevMode = g_menuPathMode = 1;
        break;
    case 2:
        g_menuPathVariant = g_menuPathPrevMode = g_menuPathMode = 2;
        break;
    case 3:
        g_menuPathVariant = g_menuPathPrevMode = g_menuPathMode = 3;
        break;
    case 4: {
        unsigned int t;
        g_menuPathMode = 4;
        g_menuPathPrevMode = 4;
        t = CFrontend::GetFrontendTimestamp();
        g_menuPathVariant = (t - FrontendScroller_GetSelectionTimestamp()) / 500 % 3;
        break;
    }
    case 5:
        g_menuPathVariant = g_menuPathPrevMode = g_menuPathMode = 7;
        break;
    case 6:
        g_menuPathVariant = g_menuPathPrevMode = g_menuPathMode = 8;
        break;
    }
}

// Distance to move this frame at `speed` (16.16 per second), from the time
// since the last update (0.02 s the first time).
// FUNCTION: CMR2 0x004f3fb0
void FrontendAnimation_GetFrameDistance(int *pOut, int speed)
{
    if (g_menuAnimTime == -1) {
        *pOut = FixMul(speed, FixDiv(20 << 16, 1000 << 16));
        return;
    }
    *pOut = FixMul(speed, FixDiv((int)FTOI(__int64, X87S_MUL((unsigned int)(CFrontend::GetFrontendTimestamp() - g_menuAnimTime), CGraphics::m_65536)),
                                 1000 << 16));
}

// Wraps a position along the path into 0..1.
#define MENU_WRAP(v)                        \
    if ((v) < 0)                            \
        (v) = (v) % 0x10000 + 0x10000;      \
    else if ((v) > 0)                       \
        (v) = (v) % 0x10000;

// Main menu animation update: moves the letters, the trail and the streams
// along the path and picks (or morphs) the path of the entry under the cursor.
// match 72%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004f4050
void FrontendAnimation_UpdateMainPath(void)
{
    int *pPos;
    int *pSpeed;
    int *pPoint;
    unsigned int elapsed;
    int step;
    int f;
    int dx;
    int dy;
    int j;

    CInput::GetAvailableDeviceRecord(0);
    FrontendAnimation_GetFrameDistance(&step, 0xccc);
    pPos = g_menuLetterPos;
    do {
        *pPos -= step;
        MENU_WRAP(*pPos)
        pPos++;
    } while (pPos < &g_menuLetterPos[200]);
    if (CGameInfo::IsRecordFlagSet(0xe)) {
        g_pMenuPath = g_menuPathCheat[0];
    } else if (g_menuPathInit == 0) {
        g_menuPathInit = 1;
        g_pMenuPath = g_menuPaths[g_menuPathMode][0];
    } else if (g_menuPathInit == 1) {
        switch (FrontendMenu_GetMain()->items[FrontendMenu_GetMain()->cursor].value) {
        case 0:
            g_menuPathMode = 0;
            break;
        case 1:
            g_menuPathMode = 1;
            break;
        case 2:
            g_menuPathMode = 2;
            break;
        case 3:
            g_menuPathMode = 3;
            break;
        case 4:
            g_menuPathMode = g_menuPathVariant + 4;
            g_menuPathVariant = (unsigned int)(CFrontend::GetFrontendTimestamp() - FrontendScroller_GetSelectionTimestamp()) / 500 % 3;
            break;
        case 5:
            g_menuPathMode = 7;
            break;
        case 6:
            g_menuPathMode = 8;
            break;
        }
        if (FrontendScroller_GetSelection() == -1) {
            g_menuPathPrevMode = g_menuPathMode;
        } else if (FrontendMenu_GetMain()->items[FrontendMenu_GetMain()->cursor].value == 4
                   && (unsigned int)(CFrontend::GetFrontendTimestamp() - FrontendScroller_GetSelectionTimestamp()) > 250) {
            g_menuPathPrevMode = (unsigned int)(CFrontend::GetFrontendTimestamp() - FrontendScroller_GetSelectionTimestamp()) / 500 % 3 + 3;
            if (g_menuPathPrevMode <= 3)
                g_menuPathPrevMode = 6;
        } else {
            switch (FrontendMenu_GetMain()->items[FrontendScroller_GetSelection()].value) {
            case 0:
                g_menuPathPrevMode = 0;
                break;
            case 1:
                g_menuPathPrevMode = 1;
                break;
            case 2:
                g_menuPathPrevMode = 2;
                break;
            case 3:
                g_menuPathPrevMode = 3;
                break;
            case 4:
                g_menuPathPrevMode = g_menuPathVariant + 4;
                break;
            case 5:
                g_menuPathPrevMode = 7;
                break;
            case 6:
                g_menuPathPrevMode = 8;
                break;
            }
        }
        if (FrontendMenu_GetMain()->items[FrontendMenu_GetMain()->cursor].value == 4)
            elapsed = (unsigned int)(CFrontend::GetFrontendTimestamp() - FrontendScroller_GetSelectionTimestamp()) % 500;
        else
            elapsed = CFrontend::GetFrontendTimestamp() - FrontendScroller_GetSelectionTimestamp();
        if (elapsed > 250) {
            g_pMenuPath = g_menuPaths[g_menuPathMode][0];
        } else {
            f = FixDiv((int)FTOI(__int64, X87S_MUL(elapsed, CGraphics::m_65536)), 250 << 16);
            j = 0;
            pPoint = g_menuPathMorph[0];
            do {
                dy = g_menuPaths[g_menuPathMode][j][1] - g_menuPaths[g_menuPathPrevMode][j][1];
                dx = g_menuPaths[g_menuPathMode][j][0] - g_menuPaths[g_menuPathPrevMode][j][0];
                pPoint[0] = g_menuPaths[g_menuPathPrevMode][j][0] + (FixMul((int)FTOI(__int64, X87S_MUL(dx, CGraphics::m_65536)), f) >> 16);
                pPoint[1] = g_menuPaths[g_menuPathPrevMode][j][1] + (FixMul((int)FTOI(__int64, X87S_MUL(dy, CGraphics::m_65536)), f) >> 16);
                pPoint += 2;
                j++;
            } while (pPoint < g_menuPathMorph[19]);
            g_pMenuPath = g_menuPathMorph[0];
        }
    }
    FrontendAnimation_GetFrameDistance(&step, 0x1333);
    g_menuTrailPos[0] -= step;
    MENU_WRAP(g_menuTrailPos[0])
    for (pPos = &g_menuTrailPos[14]; pPos > g_menuTrailPos; pPos--)
        *pPos = pPos[-1];
    pSpeed = g_menuStreamSpeed;
    do {
        *pSpeed += FixDiv((int)FTOI(__int64, X87S_MUL((rand() % 101 - 50), CGraphics::m_65536)), 1000 << 16);
        if (*pSpeed > 0x18000)
            *pSpeed = 0x18000;
        if (*pSpeed < 0x8000)
            *pSpeed = 0x8000;
        pSpeed++;
    } while (pSpeed < &g_menuStreamSpeed[6]);
    FrontendAnimation_GetFrameDistance(&step, 0x3333);
    pPos = g_menuStreamPos[0];
    pSpeed = g_menuStreamSpeed;
    do {
        *pPos -= FixMul(step, *pSpeed);
        MENU_WRAP(*pPos)
        for (j = 9; j != 0; j--)
            pPos[j] = pPos[j - 1];
        pSpeed++;
        pPos += 10;
    } while (pSpeed < &g_menuStreamSpeed[6]);
    g_menuAnimTime = CFrontend::GetFrontendTimestamp();
}

// Draws the 200 letters of "colinmcrae" along the path.
// FUNCTION: CMR2 0x004f45a0
void FrontendAnimation_DrawLetters(void)
{
    BYTE colour[4];
    int point[2];
    unsigned int i;
    int *pPos;
    short x, y;

    colour[0] = 0xff;
    colour[1] = 0xff;
    colour[2] = 0xff;
    colour[3] = 0x80;
    Font_Select(0, (int *)colour);
    i = 0;
    pPos = g_menuLetterPos;
    do {
        FrontendAnimation_EvaluatePathSpline(g_pMenuPath, *pPos, point);
        // PORT: at the fraction of a pixel the path is at.
        FrontendAnimation_PlaceOnScreen(g_menuPathPointFine, 0, 0, &x, &y);
        Font_DrawChar(g_strColinMcrae[i % strlen(g_strColinMcrae)], x, y);
        pPos++;
        i++;
    } while ((INT_PTR)pPos < (INT_PTR)(g_menuLetterPos + 200)); // 0x819cb4 in the original
}

// Draws the trail of 15 dots, fading out towards the tail.
// FUNCTION: CMR2 0x004f4650
void FrontendAnimation_DrawDotTrail(void)
{
    BYTE colour[4];
    int point[2];
    Texture *pTexture;
    int k;
    int fade;

    colour[0] = 0xff;
    colour[1] = 0xff;
    colour[2] = 0xff;
    k = 14 * 4;
    fade = 14 * 0xff;
    do {
        colour[3] = 0xff - fade / 15;
        FrontendAnimation_EvaluatePathSpline(g_pMenuPath, *(int *)((BYTE *)g_menuTrailPos + k), point);
        // PORT: at the fraction of a pixel the path is at.
        FrontendAnimation_PlaceOnScreen(g_menuPathPointFine, -2, 3, &g_menuDotRect[0], &g_menuDotRect[1]);
        pTexture = g_menuDotTextures[k / 15];
        g_menuDotRect[2] = pTexture->width;
        g_menuDotRect[3] = pTexture->height;
        Sprite_Queue((SpriteRect *)&pTexture->field_0x11c, (SpriteRect *)g_menuDotRect, pTexture, 1, 0, NULL, NULL, colour, 8);
        fade -= 0xff;
        k -= 4;
    } while (fade >= 0);
}

// Draws the six streams of 10 dots, fading out towards their tails.
// FUNCTION: CMR2 0x004f4760
void FrontendAnimation_DrawDotStreams(void)
{
    BYTE colour[4];
    Texture *pTexture;
    int *pPoint;
    int *pPos;
    int *pStream;
    unsigned int k;
    int fade;

    colour[0] = 0xff;
    colour[1] = 0xff;
    colour[2] = 0xff;
    pStream = &g_menuStreamPos[0][9];
    pPoint = &g_menuStreamPoint[0][1];
    do {
        pPos = pStream;
        k = 9 * 4;
        fade = 9 * 0xff;
        do {
            colour[3] = 0xff - fade / 10;
            FrontendAnimation_EvaluatePathSpline(g_pMenuPath, *pPos, pPoint - 1);
            // PORT: at the fraction of a pixel the path is at.
            FrontendAnimation_PlaceOnScreen(g_menuPathPointFine, -2, 3, &g_menuDotRect[0], &g_menuDotRect[1]);
            pTexture = g_menuDotTextures[(int)k / 10];
            g_menuDotRect[2] = pTexture->width;
            g_menuDotRect[3] = pTexture->height;
            Sprite_Queue((SpriteRect *)&pTexture->field_0x11c, (SpriteRect *)g_menuDotRect, pTexture, 1, 0, NULL, NULL, colour, 8);
            fade -= 0xff;
            k -= 4;
            pPos--;
        } while (fade >= 0);
        pPoint += 2;
        pStream += 10;
    } while ((INT_PTR)pPoint < (INT_PTR)(g_menuStreamPoint[6] + 1));

}

// Time of the last input on the main menu (for the attract mode).
// GLOBAL: CMR2 0x0081904c
DWORD g_mainMenuInputTime;

void GameInfo_SetGameModeBits0To2(BYTE param1);
void Frontend_SelectTextLanguage(int language);
bool FrontendCredits_ReleaseText(void);
void FrontendCredits_LoadText(char registerRelease);
void FrontendMenu_BuildAllControlsPages(void);
unsigned int StageObject_GetAnyDeviceHeldButtons(void);
void GameInfo_SetConfiguredPlayerCount(BYTE param1);
void GameInfo_SetConfiguredGameMode(BYTE param1);
void GameInfo_SetConfiguredMultiplayer(BYTE param1);
void GameInfo_SetPlayerOptionNibbles(unsigned int param1, unsigned int param2);
void NetworkChat_ClearLog(void);
void NetPlayers_ResetAllTables(void);
void RallyData_SetCountrySelectionBits(BYTE param1);
void RallyData_SetStageSelectionAndRefreshFlags(BYTE param1);

// Leaving the language menu: applies the chosen language (texts, fonts,
// credits), rebuilds the scrollers and, the first time, the controls menu,
// and makes the main menu the parent of the language menu and its entries.
// FUNCTION: CMR2 0x004ef270
void FrontendMenu_LeaveLanguage(Menu *pMenu, char back)
{
    int i;

    if (back == 0) {
        GameInfo_SetGameModeBits0To2(pMenu->cursor);
        Frontend_SelectTextLanguage(pMenu->cursor);
        FrontendCredits_ReleaseText();
        FrontendCredits_LoadText(0);
        if (CGameInfo::GetGameLanguage() == 0)
            CGameInfo::SetDashOptionEnabled(1);
        else
            CGameInfo::SetDashOptionEnabled(0);
        CFrontend::CacheCarClassTextLabels();
        FrontendScroller_RecomputeAllAttached();
        if (pMenu->pParent == NULL)
            FrontendMenu_BuildAllControlsPages();
        Menu_SetParent(pMenu, FrontendMenu_GetOptions());
        for (i = 0; i < pMenu->itemCount; i++)
            Menu_SetItemSubMenu(pMenu, i, FrontendMenu_GetOptions());
    }
}

// Entering the main menu: resets the animation and the attract-mode timer and
// sets up the scroller of the main menu.
// FUNCTION: CMR2 0x004ef300
void FrontendMenu_EnterMain(Menu *pMenu, int param)
{
    MenuScroller *p;
    int k;

    g_mainMenuInputTime = Sys_GetTicks();
    FrontendAnimation_ResetMainPath();
    p = FrontendScroller_GetMainScroller();
    p->offset = 0;
    p->startOffset = 0;
    p->current = pMenu->cursor;
    p->previous = pMenu->cursor;
    p->spacing = (int)(g_pGraphics->resX * 24) / 640;
    p->count = pMenu->itemCount;
    p->offset = 0;
    p->current = pMenu->cursor;
    for (k = 0; k < pMenu->itemCount; k++) {
        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pMenu->items[k].id));
        CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
        p->widths[k] = Font_GetTextWidth(2, (BYTE *)CFrontend::m_stringDest);
    }
    p->startTime = CFrontend::GetFrontendTimestamp();
    p->pMenu = pMenu;
    CGameInfo::SetGameModeOptionBit19(0);
}

// Update callback of the main menu: scroller, animation, and the attract
// mode after 30.5 s without input.
// FUNCTION: CMR2 0x004ef420
void FrontendMenu_UpdateMain(Menu *pMenu)
{
    DWORD now;

    FrontendScroller_UpdateCursorSlide(FrontendScroller_GetMainScroller());
    FrontendAnimation_UpdateMainPath();
    now = Sys_GetTicks();
    if (StageObject_GetAnyDeviceHeldButtons() != 0)
        g_mainMenuInputTime = Sys_GetTicks();
    if ((int)(now - g_mainMenuInputTime) > 30500) {
        CGameInfo::SetGameInfoSessionFlag(1);
        Menu_SetNextAction(FrontendMenu_GetRallyStartTransition());
    }
}

// Draw callback of the main menu: path, carousel, the "colinmcrae" letters
// and, for some entries, the dot trail or the dot streams.
// FUNCTION: CMR2 0x004d4ba0
void FrontendMenu_DrawMain(Menu *pMenu)
{
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, (int)(g_pGraphics->resX * 24) / 640, (int)(g_pGraphics->resY * 38) / 480, 1, -1, NULL, -1);
    FrontendDraw_Carousel(pMenu, 1, NULL);
    FrontendAnimation_DrawLetters();
    if (pMenu->items[pMenu->cursor].value == 1)
        FrontendAnimation_DrawDotTrail();
    if (pMenu->items[pMenu->cursor].value == 2)
        FrontendAnimation_DrawDotStreams();
}

// Leaving the main menu: remembers the entry; the first entry (single
// player) resets the session differently from the others.
// FUNCTION: CMR2 0x004f25d0
void FrontendMenu_LeaveMain(Menu *pMenu, int param)
{
    g_scrollerSelection = pMenu->cursor;
    if (pMenu->items[pMenu->cursor].value == 0) {
        FrontendRecords_SetProfileMode(1);
        GameInfo_SetConfiguredGameMode(2);
        NetPlayers_ResetAllTables();
        NetworkChat_ClearLog();
        return;
    }
    FrontendRecords_SetProfileMode(0);
    NetPlayers_ResetAllTables();
    NetworkChat_ClearLog();
}

// Item callback of the championship entry: sets up a new championship.
// FUNCTION: CMR2 0x004ec930
void FrontendChampionship_StartNewEntry(Menu *pMenu, int param)
{
    GameInfo_SetConfiguredPlayerCount(1);
    CGameInfo::SetGameModeOptionBit19(1);
    GameInfo_SetConfiguredMultiplayer(0);
    FrontendProfile_SetReturnMenu(pMenu);
    Menu_SetParent(FrontendMenu_GetMultiplayerProfile(), pMenu);
    FrontendProfile_SetPlayersRemaining(CGameInfo::GetConfiguredPlayerCount());
    if (CGameInfo::GetConfiguredGameMode() < 8) {
        GameInfo_SetConfiguredGameMode(8);
        RallyData_SetCountrySelectionBits(0);
        RallyData_SetStageSelectionAndRefreshFlags(0);
        GameInfo_SetPlayerOptionNibbles(0, 0);
    }
}

// Entering the language menu: puts the cursor on the current language and
// sets up its scroller.
// FUNCTION: CMR2 0x004f3400
void FrontendMenu_EnterLanguage(Menu *pMenu, int param)
{
    MenuScroller *p;
    int language;
    int k;

    language = CGameInfo::GetGameLanguage();
    if (language == 1000)
        FrontendMenu_GetLanguage()->cursor = 0;
    else
        FrontendMenu_GetLanguage()->cursor = language;
    p = FrontendScroller_GetLanguageScroller();
    p->current = FrontendMenu_GetLanguage()->cursor;
    p->previous = FrontendMenu_GetLanguage()->cursor;
    p->spacing = (int)(g_pGraphics->resX * 24) / 640;
    p->count = FrontendMenu_GetLanguage()->itemCount;
    p->offset = 0;
    p->startOffset = 0;
    for (k = 0; k < FrontendMenu_GetLanguage()->itemCount; k++) {
        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(FrontendMenu_GetLanguage()->items[k].id));
        CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
        p->widths[k] = Font_GetTextWidth(2, (BYTE *)CFrontend::m_stringDest);
    }
    p->startTime = CFrontend::GetFrontendTimestamp();
    p->pMenu = pMenu;
}

// Update callback of the language menu.
// FUNCTION: CMR2 0x004f39e0
void FrontendMenu_UpdateLanguage(Menu *pMenu)
{
    FrontendScroller_UpdateCursorSlide(FrontendScroller_GetLanguageScroller());
}

// Set when the new-profile entry created a profile that "back" must undo.
// GLOBAL: CMR2 0x00819879
BYTE g_newProfileUndoPending;
// "Load <name>" texts of the up to four free saved profiles.
// GLOBAL: CMR2 0x00819054
char g_profileEntryTexts[4][40];

void Profile_AssignAvailableCategory(int index, int profile);
void Profile_ResetCategoryData(int index);
void RallyData_SetPlayerProfileInUse(BYTE index, char set);
void Profile_ResetRecordCategory(int index);
int SaveProfiles_CountAvailableProfiles(void);
int SaveProfiles_GetAvailableProfileIndex(int n);
BYTE *SaveProfiles_GetAvailableProfileName(int n);

// Item callback: makes this menu the parent of the next one.
// FUNCTION: CMR2 0x004f0c40
void FrontendMenu_SetNextParent(Menu *pMenu, int param)
{
    FrontendMenu_GetMultiplayerStageSelection()->pParent = pMenu;
}

// Selects the current player-profile slot of the player setup screen: resets
// the working copy and continues to the profile menu, or plays the error sound
// when the slot does not exist.
// FUNCTION: CMR2 0x004f26f0
void FrontendProfile_SelectPlayerSlot(Menu *pMenu, int param)
{
    unsigned int index;
    int count;
    char ok;

    FrontendProfile_SetCurrentPlayer(0);
    Profile_AssignAvailableCategory(0, -1);
    Profile_ResetCategoryData(0);
    RallyData_SetPlayerProfileInUse(0, 0);
    index = pMenu->items[0].max;
    count = Profile_GetFileCount();
    if ((int)index < count) {
        ok = Profile_LoadAndLinkSavedRecord(0, index);
        if (ok != 0) {
            Menu_SetNextAction(FrontendMenu_GetProfileActions());
            return;
        }
    } else {
        Menu_PlaySoundId(3);
    }
}

// Item callback of "new profile": gives player 1 a fresh profile and goes to
// the name entry.
// FUNCTION: CMR2 0x004f2750
void FrontendProfile_CreateNewProfile(Menu *pMenu, int param)
{
    g_newProfileUndoPending = 1;
    FrontendProfile_SetCurrentPlayer(0);
    Profile_AssignAvailableCategory(FrontendProfile_GetCurrentPlayer(), -1);
    Profile_ResetCategoryData(FrontendProfile_GetCurrentPlayer());
    RallyData_SetPlayerProfileInUse(FrontendProfile_GetCurrentPlayer(), 0);
    Menu_SetParent(FrontendMenu_GetProfileNameEntry(), pMenu);
    FrontendProfile_SetRenameMenu(FrontendMenu_GetProfileRenameEntry());
    FrontendProfile_SetCompletionMenu(pMenu);
    FrontendProfile_SetHeaderText(CFrontend::GetTextString(0xb));
    g_profilePlayersRemaining--;
    Menu_SetNextAction(FrontendMenu_GetProfileNameEntry());
}

// Item callback of a saved profile: loads it for the current player.
// FUNCTION: CMR2 0x004f27d0
void FrontendProfile_LoadSelectedProfile(Menu *pMenu, int param)
{
    int profile;

    profile = SaveProfiles_GetAvailableProfileIndex(pMenu->cursor - 2);
    FrontendProfile_SetCurrentPlayer(0);
    Profile_AssignAvailableCategory((CGameInfo::GetConfiguredPlayerCount() & 0xff) - (g_profilePlayersRemaining & 0xff), profile);
    RallyData_SetPlayerProfileInUse(CGameInfo::GetConfiguredPlayerCount() - g_profilePlayersRemaining, 0);
    Menu_SetNextAction(FrontendMenu_GetProfileActions());
    g_profilePlayersRemaining--;
}

// Entering the profile menu (back: undoes the profile created by "new").
// Lists the free saved profiles and puts the cursor on the first one.
// match 84%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004f2620
void FrontendMenu_EnterRallyProfile(Menu *pMenu, char back)
{
    MenuItem *pItem;
    char *pText;
    int i;

    if (back != 0) {
        if (g_newProfileUndoPending != 0) {
            RallyData_SetPlayerProfileInUse(0, 0);
            Profile_ResetCategoryData(0);
            Profile_ResetRecordCategory(0);
        } else {
            Profile_ResetRecordCategory(0);
        }
    }
    g_newProfileUndoPending = 0;
    FrontendProfile_SetHeaderMode(1);
    GameInfo_SetConfiguredPlayerCount(1);
    g_profilePlayersRemaining = 1;
    g_pProfileReturnMenu = pMenu->pParent;
    CSound::NoOpSoundDeviceCallback();
    for (i = 0; i < 4; i++) {
        if (i < SaveProfiles_CountAvailableProfiles()) {
            sprintf(g_profileEntryTexts[i], CFrontend::GetTextString(0x17f), SaveProfiles_GetAvailableProfileName(i));
            pMenu->items[i + 2].enabled = 1;
            pMenu->items[i + 2].visible = 1;
            pMenu->items[i + 2].text = g_profileEntryTexts[i];
        } else {
            pMenu->items[i + 2].enabled = 0;
            pMenu->items[i + 2].visible = 0;
        }
    }
    if (SaveProfiles_CountAvailableProfiles() > 0) {
        pMenu->cursor = 2;
        return;
    }
    pMenu->cursor = 0;
}

// Draw callback of the profile menu.
// FUNCTION: CMR2 0x004d9a40
void FrontendMenu_DrawRallyProfile(Menu *pMenu)
{
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, (int)(g_pGraphics->resX * 24) / 640, (int)(g_pGraphics->resY * 38) / 480, 1, 1, NULL, 1);
    FrontendDraw_MenuList(pMenu, NULL, -1, -1, 0, 1);
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
    FrontendDraw_HelpText(CFrontend::GetTextString(0x57), 1);
}

// Leaving the profile menu: once every player has a profile, goes on.
// FUNCTION: CMR2 0x004f07e0
void FrontendMenu_LeavePlayerProfile(Menu *pMenu, char back)
{
    Frontend_SetOverlayMode(0);
    if (back != 0 && g_profilePlayersRemaining == CGameInfo::GetConfiguredPlayerCount()) {
        g_profilePlayersRemaining = CGameInfo::GetConfiguredPlayerCount();
        Menu_SetNextAction(g_pProfileReturnMenu);
    }
}

// Draw callback of the options menu.
// FUNCTION: CMR2 0x004e2590
void FrontendMenu_DrawOptions(Menu *pMenu)
{
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, (int)(g_pGraphics->resX * 24) / 640, (int)(g_pGraphics->resY * 38) / 480, 1, 1, NULL, -1);
    FrontendDraw_MenuList(pMenu, NULL, -1, -1, 0, 1);
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

// Entering the options menu: the cheats entry is only there once something
// is unlocked or a cheat is on.
// FUNCTION: CMR2 0x004f29b0
void FrontendMenu_EnterOptions(Menu *pMenu, int param)
{
    Frontend_SetOverlayMode(0);
    if (CGameInfo::GetUnlockFlagMask(7) || CGameInfo::GetUnlockFlagMask(1) || CGameInfo::GetUnlockFlagMask(2)
        || CGameInfo::GetUnlockFlagMask(3) || CGameInfo::GetUnlockFlagMask(4) || CGameInfo::GetUnlockFlagMask(5)
        || CGameInfo::GetUnlockFlagMask(6) || CGameInfo::GetUnlockFlagMask(0) || CGameInfo::IsRecordFlagSet(4)
        || CGameInfo::IsRecordFlagSet(5) || CGameInfo::IsRecordFlagSet(6) || CGameInfo::IsRecordFlagSet(7)
        || CGameInfo::IsRecordFlagSet(8) || CGameInfo::IsRecordFlagSet(9) || CGameInfo::IsRecordFlagSet(10)
        || CGameInfo::IsRecordFlagSet(0xb)) {
        Menu_GetItem(pMenu, 1000)->enabled = 1;
        Menu_GetItem(pMenu, 1000)->visible = 1;
    } else {
        Menu_GetItem(pMenu, 1000)->enabled = 0;
        Menu_GetItem(pMenu, 1000)->visible = 0;
    }
}

void GameInfo_SaveConfiguredCheats(void);
void GameInfo_RestoreConfiguredCheats(void);
void GameInfo_SetConfiguredCheat(int bit, int value);

// Entering the cheats menu: each of the 8 entries is available when its
// extra is unlocked or its cheat code was typed, and shows its state.
// FUNCTION: CMR2 0x004f28c0
void FrontendMenu_EnterCheats(Menu *pMenu, int param)
{
    MenuItem *pItem;
    bool cheat;
    int i;

    GameInfo_SaveConfiguredCheats();
    i = 0;
    pItem = pMenu->items;
    do {
        pItem->enabled = 0;
        pItem->visible = 1;
        pItem->max = 0;
        if (i % 2 != 0) {
            cheat = CGameInfo::IsRecordFlagSet(i / 2 + 8);
            if (CGameInfo::GetUnlockFlagMask(i) != 0 || cheat) {
                pItem->enabled = 1;
                if (CGameInfo::IsConfiguredCheatEnabled(i))
                    pItem->max = 1;
            }
        } else {
            cheat = CGameInfo::IsRecordFlagSet(i / 2 + 4);
            if (CGameInfo::GetUnlockFlagMask(i) != 0 || cheat) {
                pItem->enabled = 1;
                if (CGameInfo::IsConfiguredCheatEnabled(i))
                    pItem->max = 1;
            }
        }
        i++;
        pItem++;
    } while (i < 8);
}

// Leaving the cheats menu: stores the on/off state of each cheat (back:
// restores them first).
// FUNCTION: CMR2 0x004f2970
void FrontendMenu_LeaveCheats(Menu *pMenu, char back)
{
    int i;

    if (back != 0) {
        GameInfo_RestoreConfiguredCheats();
        return;
    }
    for (i = 0; i < 8; i++)
        GameInfo_SetConfiguredCheat(i, pMenu->items[i].max);
}

// Item callback: back to the parent menu.
// FUNCTION: CMR2 0x004f3a50
void FrontendMenu_ReturnToParent(Menu *pMenu, int param)
{
    Menu_SetNextAction(pMenu->pParent);
}

// GLOBAL: CMR2 0x00524dcc
char g_strLockedCheat[4] = "...";

// Draw callback of the cheats menu: name and on/off of each cheat, "..."
// for the ones not available yet.
// FUNCTION: CMR2 0x004ded80
void FrontendMenu_DrawCheats(Menu *pMenu)
{
    short icon[4];
    BYTE *pShadow;
    BYTE *pColour;
    BYTE *pRow;
    MenuItem *pItem;
    int maxWidth;
    int width;
    int count;
    int x0;
    short y0;
    int i;

    maxWidth = 0;
    icon[0] = (int)(g_pGraphics->resX * 100) / 640;
    icon[1] = 0;
    icon[2] = CFrontend::m_pAr640ATexture->width;
    icon[3] = CFrontend::m_pAr640ATexture->height;
    for (i = 0; i < 8; i++) {
        width = Font_GetTextWidth(1, (BYTE *)CFrontend::GetTextString(i + 0x149));
        if (width > maxWidth)
            maxWidth = width;
    }
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, (int)(g_pGraphics->resX * 24) / 640, (int)(g_pGraphics->resY * 38) / 480, 1, 2, NULL, -1);
    count = 0;
    for (i = pMenu->itemCount, pItem = pMenu->items; i > 0; i--, pItem++) {
        if (pItem->visible)
            count++;
    }
    x0 = (int)(g_pGraphics->resX * 0x7a) / 640;
    y0 = (short)(((int)(g_pGraphics->resY * 8) / 480 + (int)(g_pGraphics->resY * 38) / 480 + (int)(g_pGraphics->resY * 384) / 480) / 2)
         - (short)((int)(g_pGraphics->resY * 36) / 480 * count / 2);
    if (pMenu->cursor == 0) {
        pColour = g_colourWhite0x00524968;
        pShadow = g_colourShadowWhite0x00524974;
    } else if (pMenu->items[0].enabled) {
        pColour = g_colourText0x0052496c;
        pShadow = g_colourShadowText0x00524978;
    } else {
        pColour = g_colourDim0x00524970;
        pShadow = g_colourShadowDim0x0052497c;
    }
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 99) / 640;
    g_frontendDrawRect[1] = y0;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 282) / 640;
    g_frontendDrawRect[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pShadow, 1);
    g_frontendDrawRect[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pColour, 1);
    for (i = 0; i < count; i++) {
        pItem = &pMenu->items[i];
        icon[1] = (int)(g_pGraphics->resY * 20) / 480 + y0 + (int)(g_pGraphics->resY * 36) / 480 * (short)i
                  - CFrontend::m_pAr640ATexture->height / 2;
        if (pMenu->cursor == i) {
            pRow = g_colourWhite0x00524968;
            Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, (SpriteRect *)icon, CFrontend::m_pAr640ATexture, 1, 0, NULL, NULL, pRow, 8);
        } else {
            pRow = g_colourText0x0052496c;
            if (!pItem->enabled)
                pRow = g_colourDim0x00524970;
            Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, (SpriteRect *)icon, CFrontend::m_pAr640DTexture, 1, 0, NULL, NULL, pRow, 8);
        }
        if (pItem->enabled) {
            Font_DrawText(1, CFrontend::GetTextString(pItem->id), x0, (short)((int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1]), (int *)pRow, 0x11);
            if (pItem->max != 0) {
                Font_DrawText(1, CFrontend::GetTextString(0x133), (int)(g_pGraphics->resX * 15) / 640 + maxWidth + x0, (short)((int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1]),
                              (int *)g_colourText0x0052496c, 0x11);
                Font_DrawText(1, CFrontend::GetTextString(0x134),
                              (int)(g_pGraphics->resX * 25) / 640 + maxWidth + Font_GetTextWidth(1, (BYTE *)CFrontend::GetTextString(0x133)) + x0,
                              (short)((int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1]), (int *)g_colourWhite0x00524968, 0x11);
            } else {
                Font_DrawText(1, CFrontend::GetTextString(0x133), (int)(g_pGraphics->resX * 15) / 640 + maxWidth + x0, (short)((int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1]),
                              (int *)g_colourWhite0x00524968, 0x11);
                Font_DrawText(1, CFrontend::GetTextString(0x134),
                              (int)(g_pGraphics->resX * 25) / 640 + maxWidth + Font_GetTextWidth(1, (BYTE *)CFrontend::GetTextString(0x133)) + x0,
                              (short)((int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1]), (int *)g_colourText0x0052496c, 0x11);
            }
        } else {
            Font_DrawText(1, g_strLockedCheat, x0, (short)((int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1]), (int *)pRow, 0x11);
        }
        if (pMenu->cursor == i || pMenu->cursor == i + 1) {
            pColour = g_colourWhite0x00524968;
            pShadow = g_colourShadowWhite0x00524974;
        } else if (!pItem->enabled && !pItem[1].enabled) {
            pColour = g_colourDim0x00524970;
            pShadow = g_colourShadowDim0x0052497c;
        } else {
            pColour = g_colourText0x0052496c;
            pShadow = g_colourShadowText0x00524978;
        }
        g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 36) / 480) * ((short)i + 1) + y0;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pShadow, 1);
        g_frontendDrawRect[1]++;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pColour, 1);
    }
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

// Entering the display device menu: one entry per device, the current one
// under the cursor, and only the usable ones enabled.
// FUNCTION: CMR2 0x004f21c0
void FrontendMenu_EnterDisplayDevice(Menu *pMenu, int param)
{
    MenuItem *pItem;
    unsigned int i;

    pMenu->itemCount = CGraphics::GetDisplayDriverCount();
    pMenu->cursor = CGameInfo::GetGraphicsOptionBits5To8();
    i = 0;
    if ((unsigned int)CGraphics::GetDisplayDriverCount() > 0) {
        pItem = pMenu->items;
        do {
            if (CGraphics::GetTextureFormatCap1(i) != 0)
                pItem->enabled = 1;
            else
                pItem->enabled = 0;
            i++;
            pItem++;
        } while (i < (unsigned int)CGraphics::GetDisplayDriverCount());
    }
}

// Item callback of a display device: switches to it, re-creating the
// display, the textures and the fonts with the default video settings.
// FUNCTION: CMR2 0x004f2210
void FrontendMenu_SelectDisplayDevice(Menu *pMenu, int param)
{
    CGameInfo::SetGraphicsOptionBits5To8(pMenu->cursor);
    CGraphics::SetSelectedDisplayDriverIndex(pMenu->cursor);
    CMain::SetGameActiveState(1);
    CGraphics::RecreateGraphicsDeviceAndResources(g_pGraphics->resX, g_pGraphics->resY, g_pGraphics->depth, CGameInfo::GetGraphicsOptionBits5To8(),
                            CGraphics::GetSelectedRenderDeviceIndex());
    CGameInfo::SetScreenWidth(g_pGraphics->resX);
    CGameInfo::SetScreenHeight(g_pGraphics->resY);
    CGameInfo::SetColourDepth(g_pGraphics->depth);
    CFrontend::LoadFrontendResourceArchives();
    Graphics_ReloadAllTextures();
    FrontendText_ReloadFonts();
    FrontendScroller_RecomputeAllAttached();
    CMain::SetGameActiveState(0);
    CGameInfo::SetGraphicsOptionBits1To2(0);
    g_pGraphics->field913_0x3bc &= ~8;
    g_pGraphics->field913_0x3bc &= ~0x10;
    g_pGraphics->field913_0x3bc &= ~0x80;
    CGameInfo::SetGraphicsOptionBit3(0);
    g_pGraphics->field913_0x3bc &= ~0x20;
    CGameInfo::SetGraphicsOptionBit4(0);
    g_pGraphics->field913_0x3bc &= ~0x40;
    CGameInfo::SetGraphicsOptionBits18To19(0);
    g_pGraphics->field913_0x3bc &= ~1;
    g_pGraphics->field913_0x3bc &= ~2;
    CGameInfo::SetGraphicsOptionBits21To24(1);
    g_pGraphics->field917_0x3c0 = 1;
    CGameInfo::SetGraphicsOptionBits25To26(1);
    CGameInfo::SetGraphicsOptionBits27To28(1);
    CGameInfo::SetPreviewMode(2);
    Menu_SetNextAction(pMenu->pParent);
}

// GLOBAL: CMR2 0x005250a4
char g_strTwoDigits[8] = "%.2d";
// Gearbox of a record: manual / automatic.
// GLOBAL: CMR2 0x00524db4
char g_strGearboxManual[4] = "MT";
// GLOBAL: CMR2 0x00524db8
char g_strGearboxAuto[4] = "AT";

extern BYTE g_colourTitle0x00524984[4];

// Draw callback of the high score pages: title of the table shown (cycled
// by FrontendRecords_CycleTableMode), the column headers and the 5 entries (position, name,
// car, gearbox and two numbers).
// FUNCTION: CMR2 0x004e3340
void FrontendMenu_DrawHighScores(Menu *pMenu)
{
    GameInfo0xa4SubStruct12 *pEntry;
    int table;
    int title;
    int y0;
    int y;
    int offset;
    int i;

    table = FrontendRecords_GetTableMode();
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, (int)(g_pGraphics->resX * 24) / 640, (int)(g_pGraphics->resY * 38) / 480, 1, 2, NULL, -1);
    switch (table) {
    case 0:
        Font_DrawText(2, CFrontend::GetTextString(0x167), (int)g_pGraphics->resX / 2, (int)(g_pGraphics->resY * 100) / 480,
                      (int *)g_colourWhite0x00524968, 10);
        break;
    case 1:
        Font_DrawText(2, CFrontend::GetTextString(0x168), (int)g_pGraphics->resX / 2, (int)(g_pGraphics->resY * 100) / 480,
                      (int *)g_colourWhite0x00524968, 10);
        break;
    case 2:
        Font_DrawText(2, CFrontend::GetTextString(0x169), (int)g_pGraphics->resX / 2, (int)(g_pGraphics->resY * 100) / 480,
                      (int *)g_colourWhite0x00524968, 10);
        break;
    default:
        goto header;
    }
header:
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 30) / 640;
    g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 200) / 480 - (int)(g_pGraphics->resY * 25) / 480;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 640) / 640 + (int)(g_pGraphics->resX * 30) / 640 * -2;
    g_frontendDrawRect[3] = (int)(g_pGraphics->resY * 25) / 480;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourShadowText0x00524978, 4);
    y0 = (int)(g_pGraphics->resY * 19) / 480 + ((int)(g_pGraphics->resY * 200) / 480 - (int)(g_pGraphics->resY * 25) / 480);
    Font_DrawText(0, CFrontend::GetTextString(0x16a), (int)(g_pGraphics->resX * 150) / 640, y0, (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16b), (int)(g_pGraphics->resX * 290) / 640, y0, (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16c), (int)(g_pGraphics->resX * 430) / 640, y0, (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16d), (int)(g_pGraphics->resX * 485) / 640, y0, (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16e), (int)(g_pGraphics->resX * 540) / 640, y0, (int *)g_colourTitle0x00524984, 0x12);
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 30) / 640;
    g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 200) / 480;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 640) / 640 + (int)(g_pGraphics->resX * 30) / 640 * -2;
    g_frontendDrawRect[3] = 1;
    i = 0;
    offset = table * 0x3c;
    do {
        y = (int)(g_pGraphics->resY * 19) / 480 + (int)(g_pGraphics->resY * 200) / 480 + (int)(g_pGraphics->resY * 25) / 480 * i;
        pEntry = (GameInfo0xa4SubStruct12 *)((BYTE *)CGameInfo::GetGameInfoFieldA4Address() + offset);
        sprintf(CFrontend::m_stringDest, g_strTwoDigits, i + 1);
        Font_DrawText(3, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 100) / 640, y, (int *)g_colourText0x0052496c, 0x12);
        Font_DrawText(1, pEntry->ident, (int)(g_pGraphics->resX * 150) / 640, y, (int *)g_colourWhite0x00524968, 0x12);
        Font_DrawText(1, CFrontend::GetModeSpecificCountryText(pEntry->flags & 0x3f), (int)(g_pGraphics->resX * 290) / 640, y,
                      (int *)g_colourWhite0x00524968, 0x12);
        if ((pEntry->flags & 0x40) != 0)
            Font_DrawText(1, g_strGearboxAuto, (int)(g_pGraphics->resX * 430) / 640, y,
                          (int *)g_colourWhite0x00524968, 0x12);
        else
            Font_DrawText(1, g_strGearboxManual, (int)(g_pGraphics->resX * 430) / 640, y,
                          (int *)g_colourWhite0x00524968, 0x12);
        sprintf(CFrontend::m_stringDest, g_strTwoDigits, (pEntry->flags >> 0xe & 0xf) + 1);
        Font_DrawText(3, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 485) / 640, y, (int *)g_colourWhite0x00524968, 0x12);
        sprintf(CFrontend::m_stringDest, g_strTwoDigits, pEntry->flags >> 7 & 0x7f);
        Font_DrawText(3, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 540) / 640, y, (int *)g_colourWhite0x00524968, 0x12);
        g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 200) / 480) + (short)((int)(g_pGraphics->resY * 25) / 480) * ((short)i + 1);
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourText0x0052496c, 1);
        offset += 0xc;
        i++;
    } while (i < 5);
    FrontendDraw_HelpText(CFrontend::GetTextString(0x16f), 1);
}

CategoryBestResults *RallyData_GetDriverBestResults(int index);
BYTE *FrontendRecords_GetEventRecordText(int index);

// Draw callback of the record page: the record of the table shown (car,
// gearbox, two numbers and its time), or "no record".
// FUNCTION: CMR2 0x004e3a80
void FrontendMenu_DrawProfileHighScores(Menu *pMenu)
{
    unsigned int *pSecond;
    unsigned int *pRecord;
    int table;
    int title;
    int y0;
    int y;

    table = FrontendRecords_GetTableMode();
    FrontendDraw_PlayTime();
    FrontendDraw_MenuTitle(pMenu);
    switch (table) {
    case 0:
        Font_DrawText(2, CFrontend::GetTextString(0x167), (int)g_pGraphics->resX / 2, (int)(g_pGraphics->resY * 100) / 480,
                      (int *)g_colourWhite0x00524968, 10);
        break;
    case 1:
        Font_DrawText(2, CFrontend::GetTextString(0x168), (int)g_pGraphics->resX / 2, (int)(g_pGraphics->resY * 100) / 480,
                      (int *)g_colourWhite0x00524968, 10);
        break;
    case 2:
        Font_DrawText(2, CFrontend::GetTextString(0x169), (int)g_pGraphics->resX / 2, (int)(g_pGraphics->resY * 100) / 480,
                      (int *)g_colourWhite0x00524968, 10);
        break;
    default:
        goto header;
    }
header:
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 30) / 640;
    g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 200) / 480 - (int)(g_pGraphics->resY * 25) / 480;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 640) / 640 + (int)(g_pGraphics->resX * 30) / 640 * -2;
    g_frontendDrawRect[3] = (int)(g_pGraphics->resY * 25) / 480;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourShadowText0x00524978, 4);
    y0 = (int)(g_pGraphics->resY * 19) / 480 + ((int)(g_pGraphics->resY * 200) / 480 - (int)(g_pGraphics->resY * 25) / 480);
    Font_DrawText(0, CFrontend::GetTextString(0x16b), (int)(g_pGraphics->resX * 180) / 640, y0, (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16c), (int)(g_pGraphics->resX * 310) / 640, y0, (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16d), (int)(g_pGraphics->resX * 370) / 640, y0, (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16e), (int)(g_pGraphics->resX * 435) / 640, y0, (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x170), (int)(g_pGraphics->resX * 520) / 640, y0, (int *)g_colourTitle0x00524984, 0x12);
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 30) / 640;
    g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 200) / 480;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 640) / 640 + (int)(g_pGraphics->resX * 30) / 640 * -2;
    g_frontendDrawRect[3] = 1;
    y = (int)(g_pGraphics->resY * 19) / 480 + (int)(g_pGraphics->resY * 200) / 480;
    if ((RallyData_GetDriverBestResults(0)->playerResults[table].result.header & 0x80) != 0) {
        pSecond = &RallyData_GetDriverBestResults(0)->playerResults[table].result.result;
        pRecord = &RallyData_GetDriverBestResults(0)->playerResults[table].result.header;
        Font_DrawText(1, CFrontend::GetModeSpecificCountryText(*pRecord & 0x3f), (int)(g_pGraphics->resX * 180) / 640, y,
                      (int *)g_colourWhite0x00524968, 0x12);
        if ((*pRecord & 0x40) != 0)
            Font_DrawText(1, g_strGearboxAuto, (int)(g_pGraphics->resX * 310) / 640, y,
                          (int *)g_colourWhite0x00524968, 0x12);
        else
            Font_DrawText(1, g_strGearboxManual, (int)(g_pGraphics->resX * 310) / 640, y,
                          (int *)g_colourWhite0x00524968, 0x12);
        sprintf(CFrontend::m_stringDest, g_strTwoDigits, (*pSecond & 0xf) + 1);
        Font_DrawText(3, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 370) / 640, y, (int *)g_colourWhite0x00524968, 0x12);
        sprintf(CFrontend::m_stringDest, g_strTwoDigits, *pSecond >> 6 & 0xff);
        Font_DrawText(3, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 435) / 640, y, (int *)g_colourWhite0x00524968, 0x12);
        Font_DrawText(0, (char *)FrontendRecords_GetEventRecordText(table), (int)(g_pGraphics->resX * 520) / 640, y, (int *)g_colourWhite0x00524968, 0x12);
    } else {
        Font_DrawText(1, CFrontend::GetTextString(0x171), (int)g_pGraphics->resX / 2, y, (int *)g_colourWhite0x00524968, 0x12);
    }
    g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 25) / 480) + (short)((int)(g_pGraphics->resY * 200) / 480);
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourText0x0052496c, 1);
    FrontendDraw_HelpText(CFrontend::GetTextString(0x16f), 1);
}

extern char g_loadRecordTimeFormat[];
BYTE *FrontendRecords_GetStageRecordText(int row, int column);

// Draw callback of the stage records page of a rally: for each stage (10,
// or 11 for every second rally) the record car, gearbox, time and holder.
// match 65%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004e5630
void FrontendMenu_DrawProfileBestStageTimes(Menu *pMenu)
{
    unsigned int *pRecord;
    unsigned int time;
    CategoryBestResults *pRecords;
    char rally;
    int odd;
    int y0;
    int y;
    int i;

    FrontendRecords_GetTableMode();
    FrontendDraw_PlayTime();
    FrontendDraw_MenuTitle(pMenu);
    odd = pMenu->cursor % 2;
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 30) / 640;
    g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 90) / 480 - (int)(g_pGraphics->resY * 25) / 480;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 640) / 640 + (int)(g_pGraphics->resX * 30) / 640 * -2;
    g_frontendDrawRect[3] = (int)(g_pGraphics->resY * 25) / 480;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourShadowText0x00524978, 4);
    y0 = (int)(g_pGraphics->resY * 19) / 480 + ((int)(g_pGraphics->resY * 90) / 480 - (int)(g_pGraphics->resY * 25) / 480);
    Font_DrawText(0, CFrontend::GetTextString(0x17d), (int)(g_pGraphics->resX * 100) / 640, y0, (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16b), (int)(g_pGraphics->resX * 230) / 640, y0, (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16c), (int)(g_pGraphics->resX * 355) / 640, y0, (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x174), (int)(g_pGraphics->resX * 415) / 640, y0, (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x170), (int)(g_pGraphics->resX * 520) / 640, y0, (int *)g_colourTitle0x00524984, 0x12);
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 30) / 640;
    g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 90) / 480;
    g_frontendDrawRect[3] = 1;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 640) / 640 + (int)(g_pGraphics->resX * 30) / 640 * -2;
    i = 0;
    do {
        y = (int)(g_pGraphics->resY * 19) / 480 + (int)(g_pGraphics->resY * 90) / 480 + (int)(g_pGraphics->resY * 25) / 480 * i;
        pRecords = RallyData_GetDriverBestResults(0);
        rally = pMenu->cursor;
        pRecord = &RallyData_GetDriverBestResults(0)->rallyStageTimes[pMenu->cursor][i].header;
        sprintf(CFrontend::m_stringDest, g_strTwoDigits, i + 1);
        Font_DrawText(3, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 100) / 640, y, (int *)g_colourText0x0052496c, 0x12);
        if ((*pRecord & 0x80) != 0) {
            Font_DrawText(1, CFrontend::GetModeSpecificCountryText(*pRecord & 0x3f), (int)(g_pGraphics->resX * 230) / 640, y,
                          (int *)g_colourWhite0x00524968, 0x12);
            if ((*pRecord & 0x40) != 0)
                Font_DrawText(1, g_strGearboxAuto,
                              (int)(g_pGraphics->resX * 355) / 640, y, (int *)g_colourWhite0x00524968, 0x12);
            else
                Font_DrawText(1, g_strGearboxManual,
                              (int)(g_pGraphics->resX * 355) / 640, y, (int *)g_colourWhite0x00524968, 0x12);
            time = pRecords->rallyStageTimes[rally][i].time;
            sprintf(CFrontend::m_stringDest, g_loadRecordTimeFormat, time / 6000, time / 100 % 60, time % 100);
            Font_DrawText(3, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 415) / 640, y, (int *)g_colourWhite0x00524968, 0x12);
            Font_DrawText(0, (char *)FrontendRecords_GetStageRecordText(pMenu->cursor, i), (int)(g_pGraphics->resX * 520) / 640, y,
                          (int *)g_colourWhite0x00524968, 0x12);
        } else {
            Font_DrawText(1, CFrontend::GetTextString(0x171), (int)g_pGraphics->resX / 2, y, (int *)g_colourWhite0x00524968, 0x12);
        }
        g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 90) / 480) + (short)((int)(g_pGraphics->resY * 25) / 480) * ((short)i + 1);
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourText0x0052496c, 1);
        i++;
    } while (i < (odd != 0) + 10);
    FrontendDraw_ScrollerRow(FrontendScroller_GetBestStageTimesScroller(), 1);
    FrontendDraw_HelpText(CFrontend::GetTextString(0x172), 1);
}

int GameInfo_GetField98(void);
void RallyData_SetPlayerDefaultCarSetup(int player);

// 7x15 dot icon drawn by FrontendDraw_RipplingDotIcon (the rest of the block is unused).
// GLOBAL: CMR2 0x00523f30
char g_dotIconMap[216] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00,
    0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01,
    0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x01, 0x01, 0x00,
    0x00, 0x01, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01,
};
// Ripple phase of the dot icons.
// GLOBAL: CMR2 0x008189b4
short g_dotIconPhase;


void FrontendChampionship_InitEntries(void);
int Frontend_ComputeRippleHeight(int *pCentre, int x, int y, int phase, int wavelength);
void Profile_AssignAvailableCategory(int index, int profile);
void Profile_ResetCategoryData(int index);
void RallyData_SetPlayerProfileInUse(BYTE index, char set);
void RallyData_MarkAllDriverRecordsUnassigned(void);

// Draws one 7x15 dot icon at (x, y), each dot shaded by the ripple.
// match 71%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004d5ad0
void FrontendDraw_RipplingDotIcon(int x, int y, short phase)
{
    BYTE colour[4];
    int centre[2];
    char *pDot;
    int u;
    int b;
    int col;
    int row;

    colour[0] = 0xff;
    colour[1] = 0xff;
    colour[2] = 0xff;
    colour[3] = 0xff;
    centre[0] = 0;
    centre[1] = 0;
    g_frontendDrawRect[2] = CFrontend::m_pSmMatrixTexture->width;
    g_frontendDrawRect[3] = CFrontend::m_pSmMatrixTexture->height;
    centre[0] = 0;
    centre[1] = 0;
    g_frontendDrawRect[0] = x;
    col = 0;
    do {
        row = 0;
        u = (int)FTOI(__int64, X87S_MUL(((int)(g_pGraphics->resX * 18) / 640 * col + x), CGraphics::m_65536)) / (int)g_pGraphics->resX;
        g_frontendDrawRect[1] = y;
        pDot = &g_dotIconMap[col];
        do {
            if (*pDot != 0) {
                b = Frontend_ComputeRippleHeight(centre, u,
                                 (int)FTOI(__int64, X87S_MUL(((int)(g_pGraphics->resY * 18) / 480 * row + y), CGraphics::m_65536))
                                     / (int)g_pGraphics->resX,
                                 phase, 0x140000) / 4 + 0xc000;
                colour[0] = FixMulShift32(0xff0000, b);
                colour[1] = colour[0];
                colour[2] = colour[0];
                Sprite_Queue((SpriteRect *)&CFrontend::m_pSmMatrixTexture->field_0x11c, (SpriteRect *)g_frontendDrawRect,
                             CFrontend::m_pSmMatrixTexture, 1, 0, NULL, NULL, colour, 8);
            }
            g_frontendDrawRect[1] += (int)(g_pGraphics->resY * 8) / 480;
            pDot += 7;
            row++;
        } while (row < 15);
        g_frontendDrawRect[0] += (int)(g_pGraphics->resX * 8) / 640;
        col++;
    } while (col < 7);
}

// Draw callback of the difficulty pages: title from the game mode, the
// scroller and one dot icon per difficulty level up to the selected one.
// match 67%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004d5fb0
void FrontendMenu_DrawDifficulty(Menu *pMenu)
{
    char *text[2];
    int x;
    int y;
    int i;

    g_dotIconPhase = (short)(((CMain::GetFrameDelta() + 1) * -0x2000) / 360);
    switch (CGameInfo::GetConfiguredGameMode()) {
    case 0:
        text[0] = CFrontend::GetTextString(0xe7);
        text[1] = CFrontend::GetTextString(0xc);
        break;
    case 1:
        text[0] = CFrontend::GetTextString(0xe7);
        text[1] = CFrontend::GetTextString(0xd);
        break;
    case 2:
        text[0] = CFrontend::GetTextString(0xe7);
        text[1] = CFrontend::GetTextString(0xf);
        break;
    case 3:
        text[0] = CFrontend::GetTextString(0xe7);
        text[1] = CFrontend::GetTextString(0x10);
        break;
    case 4:
        text[0] = CFrontend::GetTextString(0xe7);
        text[1] = CFrontend::GetTextString(0x11);
        break;
    case 5:
        text[0] = CFrontend::GetTextString(0x94);
        text[1] = CFrontend::GetTextString(0xc);
        break;
    case 6:
        text[0] = CFrontend::GetTextString(0x94);
        text[1] = CFrontend::GetTextString(0xe2);
        break;
    case 7:
        text[0] = CFrontend::GetTextString(0x94);
        text[1] = CFrontend::GetTextString(0x10);
        break;
    }
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, (int)(g_pGraphics->resX * 24) / 640, (int)(g_pGraphics->resY * 38) / 480, 1, 3, text, 2);
    FrontendDraw_ScrollerRow(FrontendScroller_GetDifficultyScroller(), 1);
    y = ((int)(g_pGraphics->resY * 8) / 480 - (int)(g_pGraphics->resY * 6) / 480) / 2
        - (int)(g_pGraphics->resY * 8) / 480 * 15 / 2 + (int)g_pGraphics->resY / 2;
    i = 0;
    x = (int)g_pGraphics->resX / 2
        - (((int)(g_pGraphics->resX * 6) / 640 + (int)(g_pGraphics->resX * 8) / 640 * 6) * (pMenu->cursor + 1)
           + (int)(g_pGraphics->resX * 10) / 640 * pMenu->cursor) / 2;
    for (; i < pMenu->cursor + 1; i++) {
        FrontendDraw_RipplingDotIcon(x, y, g_dotIconPhase);
        x += (int)(g_pGraphics->resX * 10) / 640 + (int)(g_pGraphics->resX * 6) / 640 + (int)(g_pGraphics->resX * 8) / 640 * 6;
    }
    FrontendDraw_HelpText(CFrontend::GetTextString(0x57), 1);
}

// Returns the three performance values (speed, acceleration, grip) of a car
// class: the fixed table values for the known classes, random ones otherwise.
// FUNCTION: CMR2 0x004d7c00
void FrontendCar_GetClassPerformance(int param_1, int *param_2, int *param_3, int *param_4)
{
    int scale = 0x75c2;
    int iVar2;
    int iVar3;

    switch (param_1) {
    case 0:
    case 3:
    case 4:
        param_1 = 0x570a;
        iVar2 = 1000;
        iVar3 = 0;
        break;
    case 6:
        param_1 = 0x63d7;
        iVar2 = 0x3a6;
        iVar3 = 0;
        break;
    case 7:
        param_1 = 0x4a3d;
        iVar2 = 0x3c7;
        iVar3 = 2;
        break;
    case 2:
        param_1 = 0x5687;
        iVar2 = 1000;
        iVar3 = 0;
        break;
    case 1:
        param_1 = 0x570a;
        iVar2 = 0x400;
        iVar3 = 0;
        break;
    case 8:
        param_1 = 0x3ae1;
        iVar2 = 0x1bf;
        iVar3 = 1;
        break;
    case 0xd:
        param_1 = 0x3ae1;
        iVar2 = 0x1bf;
        iVar3 = 2;
        break;
    case 5:
        param_1 = 0x5c28;
        iVar2 = 1000;
        iVar3 = 0;
        break;
    case 9:
        param_1 = 0x6666;
        iVar2 = 0x345;
        iVar3 = 0;
        break;
    case 0xb:
        param_1 = scale;
        iVar2 = 0x2fc;
        iVar3 = 0;
        break;
    case 10:
        param_1 = 0x570a;
        iVar2 = 0x2dc;
        iVar3 = 2;
        break;
    case 0xc:
        param_1 = 0x570a;
        iVar2 = 0x34e;
        iVar3 = 0;
        break;
    default:
        iVar2 = rand();
        param_1 = iVar2 % 0xffff;
        iVar2 = rand();
        iVar2 = iVar2 % 1000;
        iVar3 = rand();
        iVar3 = iVar3 % 3;
    }
    param_1 = FixDiv(param_1, scale);
    *param_2 = (FixMul(param_1, 0xb0000) >> 16);
    *param_3 = iVar2 * 11 / 1024;
    *param_4 = iVar3;
}

// Draws the progress bar of a game mode page: param_3 small rects in a row,
// the first param_4 highlighted, and the label of param_5 underneath.
// FUNCTION: CMR2 0x004d8330
int FrontendDraw_ModeProgressBar(int param_1, int param_2, int param_3, int param_4, char *param_5)
{
    short rect[4];
    int i;
    int x;
    int step;
    int scale;

    rect[1] = param_2;
    rect[2] = (int)(g_pGraphics->resX * 8) / 640;
    rect[3] = (int)(g_pGraphics->resY * 8) / 480;
    x = param_1 << 16;
    rect[0] = (short)(x >> 16);
    scale = 0xa0000;
    step = FixMul(g_pGraphics->resX << 16, FixDiv(scale, 0x2800000));
    for (i = 0; i < param_3; i++) {
        if (i < param_4)
            Sprite_FillRect(&g_pGraphics->field309_0x150, rect, g_colourWhite0x00524968, 1);
        else
            Sprite_FillRect(&g_pGraphics->field309_0x150, rect, g_colourText0x0052496c, 1);
        x += step;
        rect[0] = (short)(x >> 16);
    }
    if (param_5 != 0) {
        Font_DrawText(1, param_5, *(int *)rect, (int)(g_pGraphics->resY * 0x18) / 0x1e0 + param_2,
                      (int *)g_colourText0x0052496c, 0x14);
        return rect[0];
    }
    return rect[0];
}

// Clears the championship data and sets up the 8 championship entries.
// FUNCTION: CMR2 0x004eae40
void FrontendChampionship_InitEntries(void)
{
    int i;

    memset(g_saveCarRecords, 0, sizeof(g_saveCarRecords));
    i = 0;
    do {
        g_saveCarRecords[i].flags = (g_saveCarRecords[i].flags & 0xfd) | 1;
        g_saveCarRecords[i].car = GameInfo_GetField98();
        RallyData_SetPlayerDefaultCarSetup(i);
        g_saveCarRecords[i].hudFlags = (g_saveCarRecords[i].hudFlags & 0xfc) | 0x3c;
        i++;
    } while (i < 8);
}

extern DriverProfileIdentifier *g_pProfileIdentifiers;

// Sets the automatic gearbox flag of a driver (bit 5 of the profile options,
// bit 0 of the championship entry's flags in game mode 4, which is also marked
// edited).
// FUNCTION: CMR2 0x004eb0c0
void RallyData_SetDriverAutomaticGearbox(BYTE index, BYTE flag)
{
    unsigned int category;
    BYTE *pEntry;

    RallyData_ValidateIndex(index);
    if (CGameInfo::GetConfiguredGameMode() == 4) {
        pEntry = &g_saveCarRecords[index].flags;
        *pEntry = ((*pEntry ^ flag) & 1) ^ *pEntry | 2;
        return;
    }
    category = g_saveSlots[index].category;
    if (category != 0xf) {
        g_saveProfiles[category].options =
            (g_saveProfiles[category].options & 0xffffffdf) | (flag & 1) << 5;
        g_saveDirty[category] = 1;
    }
}

void RallyTiming_SetStageTimeSeconds(int index, int seconds);
void RallyTiming_SetOverallTimeRaw(int iDriver, int iCentiseconds);
void RallyTiming_SortStageAndOverallResults(void);

// Copies the recorded stage times of the pending drivers from the save block
// into the timing system.
// FUNCTION: CMR2 0x004eb160
void FrontendChampionship_RestoreStageTimes(void)
{
    int i;
    int n;
    int index;

    for (i = 0; i < (BYTE)CGameInfo::GetConfiguredPlayerCount(); i++) {
        index = 15 - i;
        RallyTiming_SetStageTimeSeconds(index, g_saveSlots[i].seconds);
        RallyTiming_SetOverallTimeRaw(index, g_saveSlots[i].time);
    }
    n = 16 - (BYTE)CGameInfo::GetConfiguredPlayerCount();
    for (i = 0; i < n; i++) {
        index = (BYTE)CGameInfo::GetConfiguredPlayerCount() + i;
        RallyTiming_SetStageTimeSeconds(i, g_saveSlots[index].seconds);
        RallyTiming_SetOverallTimeRaw(i, g_saveSlots[index].time);
    }
    RallyTiming_SortStageAndOverallResults();
}

// True if the profile name of a driver matches the category record that the
// given championship entry points at.
// match 79%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004eb200
bool FrontendProfile_DoesDriverMatchEntry(int param_1, BYTE *param_2)
{
    unsigned int category;
    DriverProfile *pProfile;

    RallyData_ValidateIndex(param_1);
    category = g_saveSlots[param_1].category;
    pProfile = &g_saveProfiles[category];
    CGenericFileLoader::StrLowerPolish((char *)param_2);
    if (strcmp((char *)param_2, pProfile->name) == 0)
        return 1;
    return 0;
}

// True if the profile of the given index already matches the category record
// that one of the championship entries points at.
// FUNCTION: CMR2 0x004ebd60
bool FrontendProfile_IsAlreadyInChampionship(int index)
{
    BYTE *pRecord;
    DriverProfileIdentifier *pProfile;
    DriverProfileIdentifier *pCategory;
    unsigned int category;
    unsigned int diff;

    pProfile = &g_pProfileIdentifiers[index];
    for (pRecord = (BYTE *)g_saveSlots; (INT_PTR)pRecord < (INT_PTR)(g_saveSlots + 16); pRecord += sizeof(SaveSlot)) {
        category = ((SaveSlot *)pRecord)->category;
        if (category == 0xf)
            continue;
        pCategory = &g_saveProfiles[category].identifier;
        diff = pProfile->flags ^ pCategory->flags;
        if ((diff & 0x1f0f00) != 0 || (char)diff != 0 || (diff & 0xfc0f000) != 0)
            continue;
        if (strcmp(pProfile->name, pCategory->name) == 0)
            return 1;
    }
    return 0;
}

// Sets the colour word of the category record that the driver's entry points
// at. param_2 is the 5-bit shade index, param_3 the 4-bit hue.
// FUNCTION: CMR2 0x004ebe10
void FrontendProfile_SetDateOfBirth(int param_1, unsigned int param_2, unsigned int param_3, unsigned int param_4)
{
    unsigned int category;
    unsigned int colour;

    RallyData_ValidateIndex(param_1);
    category = g_saveSlots[param_1].category;
    colour = ((((param_2 & 0x1f) << 8) | (param_3 & 0xf)) << 8) |
             (g_saveProfiles[category].identifier.flags & 0xffe0f0ff);
    g_saveProfiles[category].identifier.flags = ((colour ^ param_4) & 0xff) ^ colour;
}

// Next player: gives the player a profile and goes to the name entry (or
// for championship mode 4 to the championship screen).
// FUNCTION: CMR2 0x004f0ac0
void FrontendProfile_SetupNextPlayer(Menu *pMenu, int param)
{
    Menu *pNext;

    g_unk0x00819744++;
    FrontendProfile_SetCurrentPlayer((CGameInfo::GetConfiguredPlayerCount() & 0xff) - (g_profilePlayersRemaining & 0xff));
    if (CGameInfo::GetConfiguredGameMode() != 4) {
        switch (CGameInfo::GetConfiguredGameMode()) {
        case 5:
            pNext = FrontendMenu_GetChampionshipRouteSelection();
            break;
        case 6:
            pNext = FrontendMenu_GetAlternateChampionshipRouteSelection();
            break;
        case 7:
            pNext = FrontendMenu_GetArcadeChampionshipRouteSelection();
            break;
        default:
            pNext = FrontendMenu_GetCarSetup();
            break;
        }
        Profile_AssignAvailableCategory((CGameInfo::GetConfiguredPlayerCount() & 0xff) - (g_profilePlayersRemaining & 0xff), -1);
        Profile_ResetCategoryData((CGameInfo::GetConfiguredPlayerCount() & 0xff) - (g_profilePlayersRemaining & 0xff));
        RallyData_SetPlayerProfileInUse(CGameInfo::GetConfiguredPlayerCount() - g_profilePlayersRemaining, 0);
        Menu_SetParent(FrontendMenu_GetProfileNameEntry(), pMenu);
        RallyData_SetPlayerProfileInUse(CGameInfo::GetConfiguredPlayerCount() - g_profilePlayersRemaining, 1);
        if (CGameInfo::GetGameModeOptionBit19() != 0) {
            FrontendProfile_SetRenameMenu(FrontendMenu_GetNetworkConnection());
        } else {
            FrontendProfile_SetRenameMenu(pNext);
            Menu_SetParent(pNext, pMenu);
        }
    } else {
        Menu_SetParent(FrontendMenu_GetProfileNameEntry(), pMenu);
        FrontendProfile_SetRenameMenu(FrontendMenu_GetPaletteSelection());
        FrontendMenu_GetPaletteConfirmation();
    }
    sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0xdc),
            (CGameInfo::GetConfiguredPlayerCount() & 0xff) - (g_profilePlayersRemaining & 0xff) + 1);
    FrontendProfile_SetHeaderText(CFrontend::m_stringDest);
    g_profilePlayersRemaining--;
    Menu_SetNextAction(FrontendMenu_GetProfileNameEntry());
}

// Item callback of a difficulty: stores it and moves on to the next screen
// of the game mode.
// FUNCTION: CMR2 0x004ef7c0
void FrontendMenu_SelectDifficulty(Menu *pMenu, int param)
{
    GameInfo_SetConfiguredPlayerCount(pMenu->cursor + 1);
    RallyData_MarkAllDriverRecordsUnassigned();
    CSound::NoOpSoundDeviceCallback();
    if (CGameInfo::GetConfiguredGameMode() == 4) {
        FrontendChampionship_InitEntries();
        GameInfo_SetConfiguredMultiplayer(0);
        FrontendMenu_GetPaletteSelection()->pfnCallback1 = NULL;
        FrontendMenu_GetPaletteSelection()->pParent = FrontendMenu_GetProfileNameEntry();
        Menu_SetParent(FrontendMenu_GetProfileNameEntry(), pMenu);
        g_profilePlayersRemaining = CGameInfo::GetConfiguredPlayerCount();
        FrontendProfile_SetupNextPlayer(pMenu, param);
        return;
    }
    FrontendMenu_GetPaletteSelection()->pfnCallback1 = (MenuCallback)FrontendScroller_UpdateChampionshipEntry;
    FrontendMenu_GetPaletteSelection()->pParent = FrontendMenu_GetCarSetup();
    if (CGameInfo::GetConfiguredPlayerCount() == 1) {
        GameInfo_SetConfiguredMultiplayer(0);
    } else {
        if (CGameInfo::GetConfiguredGameMode() != 3 && CGameInfo::GetConfiguredPlayerCount() == 2) {
            switch (CGameInfo::GetConfiguredGameMode()) {
            case 0:
                Menu_SetNextAction(FrontendMenu_GetSingleRallyTransmission());
                return;
            case 1:
                Menu_SetNextAction(FrontendMenu_GetChampionshipTransmission());
                return;
            case 2:
                Menu_SetNextAction(FrontendMenu_GetTimeTrialTransmission());
                return;
            }
            return;
        }
        GameInfo_SetConfiguredMultiplayer(1);
    }
    g_pProfileReturnMenu = pMenu;
    Menu_SetParent(FrontendMenu_GetMultiplayerProfile(), pMenu);
    g_profilePlayersRemaining = CGameInfo::GetConfiguredPlayerCount();
    Menu_SetNextAction(FrontendMenu_GetMultiplayerProfile());
}

// Entering a difficulty page: sets up its scroller.
// FUNCTION: CMR2 0x004f3610
void FrontendScroller_InitDifficulty(Menu *pMenu, int param)
{
    MenuScroller *p;
    int k;

    p = FrontendScroller_GetDifficultyScroller();
    p->startTime = CFrontend::GetFrontendTimestamp();
    p->pMenu = pMenu;
    p->count = pMenu->itemCount;
    for (k = 0; k < pMenu->itemCount; k++) {
        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pMenu->items[k].id));
        CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
        p->widths[k] = Font_GetTextWidth(2, (BYTE *)CFrontend::m_stringDest);
    }
    p->offset = 0;
    p->startOffset = 0;
    p->current = pMenu->cursor;
    p->previous = pMenu->cursor;
}

// Update callback of the difficulty pages.
// FUNCTION: CMR2 0x004f39f0
void FrontendScroller_UpdateDifficulty(Menu *pMenu)
{
    FrontendScroller_UpdateCursorSlide(FrontendScroller_GetDifficultyScroller());
}

// Entering the 8-level difficulty page: sets up its scroller with all 8
// levels enabled.
// FUNCTION: CMR2 0x004f3530
void FrontendMenu_EnterMultiplayerExtendedDifficulty(Menu *pMenu, int param)
{
    MenuScroller *p;
    int k;

    pMenu->itemCount = 8;
    p = FrontendScroller_GetDifficultyScroller();
    p->startTime = CFrontend::GetFrontendTimestamp();
    p->pMenu = pMenu;
    p->count = pMenu->itemCount;
    for (k = 0; k < pMenu->itemCount; k++) {
        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pMenu->items[k].id));
        CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
        p->widths[k] = Font_GetTextWidth(2, (BYTE *)CFrontend::m_stringDest);
        if (k < 8)
            pMenu->items[k].enabled = 1;
        else
            pMenu->items[k].enabled = 0;
    }
    p->offset = 0;
    p->startOffset = 0;
    p->current = pMenu->cursor;
    p->previous = pMenu->cursor;
}

// Draw callback of the two-choice pages after the difficulty: the same
// title as the difficulty pages, the list and the scroller.
// FUNCTION: CMR2 0x004d6460
void FrontendMenu_DrawTransmissionChoice(Menu *pMenu)
{
    char *text[2];

    switch (CGameInfo::GetConfiguredGameMode()) {
    case 0:
        text[0] = CFrontend::GetTextString(0xe7);
        text[1] = CFrontend::GetTextString(0xc);
        break;
    case 1:
        text[0] = CFrontend::GetTextString(0xe7);
        text[1] = CFrontend::GetTextString(0xd);
        break;
    case 2:
        text[0] = CFrontend::GetTextString(0xe7);
        text[1] = CFrontend::GetTextString(0xf);
        break;
    case 3:
        text[0] = CFrontend::GetTextString(0xe7);
        text[1] = CFrontend::GetTextString(0x10);
        break;
    case 4:
        text[0] = CFrontend::GetTextString(0xe7);
        text[1] = CFrontend::GetTextString(0x11);
        break;
    case 5:
        text[0] = CFrontend::GetTextString(0x94);
        text[1] = CFrontend::GetTextString(0xc);
        break;
    case 6:
        text[0] = CFrontend::GetTextString(0x94);
        text[1] = CFrontend::GetTextString(0xe2);
        break;
    case 7:
        text[0] = CFrontend::GetTextString(0x94);
        text[1] = CFrontend::GetTextString(0x10);
        break;
    }
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, (int)(g_pGraphics->resX * 24) / 640, (int)(g_pGraphics->resY * 38) / 480, 1, 3, text, 2);
    FrontendDraw_MenuList(pMenu, NULL, -1, -1, 0, 1);
    FrontendDraw_ScrollerRow(FrontendScroller_GetDifficultyScroller(), 0);
    FrontendDraw_HelpText(CFrontend::GetTextString(0x57), 1);
}

// Update callback of the two-choice pages: the first choice is always
// available.
// FUNCTION: CMR2 0x004ef410
void FrontendMenu_UpdateTransmissionChoice(Menu *pMenu)
{
    pMenu->items[0].enabled = 1;
}

// Item callback of the two-choice pages: stores the choice and goes to the
// next screen of the mode.
// FUNCTION: CMR2 0x004efb20
void FrontendMenu_SelectTransmission(Menu *pMenu, int param)
{
    GameInfo_SetConfiguredMultiplayer(pMenu->cursor);
    g_pProfileReturnMenu = pMenu;
    Menu_SetParent(FrontendMenu_GetMultiplayerProfile(), pMenu);
    g_profilePlayersRemaining = CGameInfo::GetConfiguredPlayerCount();
}

BYTE RallyData_IsDriverRecordUsable(BYTE param1);

// Menu the current game mode continues to after the player setup.
#define FRONTEND_MODE_NEXT_MENU(pNext)          \
    switch (CGameInfo::GetConfiguredGameMode()) {        \
    case 4:                                     \
        pNext = FrontendMenu_GetPaletteConfirmation();                 \
        break;                                  \
    case 5:                                     \
        pNext = FrontendMenu_GetChampionshipRouteSelection();                 \
        break;                                  \
    case 6:                                     \
        pNext = FrontendMenu_GetAlternateChampionshipRouteSelection();                 \
        break;                                  \
    case 7:                                     \
        pNext = FrontendMenu_GetArcadeChampionshipRouteSelection();                 \
        break;                                  \
    default:                                    \
        pNext = FrontendMenu_GetCarSetup();                 \
        break;                                  \
    }

// Draw callback of the player profile menu: "<mode> | Player N | <menu>".
// FUNCTION: CMR2 0x004d9880
void FrontendMenu_DrawMultiplayerProfile(Menu *pMenu)
{
    char *text[4];

    sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0xdc), g_profileHeaderPlayerNumber);
    text[2] = CFrontend::m_stringDest;
    text[3] = CFrontend::GetTextString(pMenu->field_0x4);
    if (CGameInfo::GetGameModeOptionBit19() != 0) {
        text[0] = CFrontend::GetTextString(0x12);
        text[1] = NULL;
        text[2] = NULL;
    } else {
        switch (CGameInfo::GetConfiguredGameMode()) {
        case 0:
            text[0] = CFrontend::GetTextString(0xe7);
            text[1] = CFrontend::GetTextString(0xc);
            break;
        case 1:
            text[0] = CFrontend::GetTextString(0xe7);
            text[1] = CFrontend::GetTextString(0xd);
            break;
        case 2:
            text[0] = CFrontend::GetTextString(0xe7);
            text[1] = CFrontend::GetTextString(0xf);
            break;
        case 3:
            text[0] = CFrontend::GetTextString(0xe7);
            text[1] = CFrontend::GetTextString(0x10);
            break;
        case 4:
            text[0] = CFrontend::GetTextString(0xe7);
            text[1] = CFrontend::GetTextString(0xe);
            break;
        case 5:
            text[0] = CFrontend::GetTextString(0x94);
            text[1] = CFrontend::GetTextString(0xc);
            break;
        case 6:
            text[0] = CFrontend::GetTextString(0x94);
            text[1] = CFrontend::GetTextString(0xe2);
            break;
        case 7:
            text[0] = CFrontend::GetTextString(0x94);
            text[1] = CFrontend::GetTextString(0x10);
            break;
        }
    }
    FrontendDraw_PlayTime();
    FrontendDraw_Breadcrumb((int)(g_pGraphics->resX * 24) / 640, (int)(g_pGraphics->resY * 38) / 480, text, 4);
    FrontendDraw_MenuList(pMenu, NULL, -1, -1, 0, 1);
    FrontendDraw_HelpText(CFrontend::GetTextString(0x57), 1);
    if (CGameInfo::GetGameModeOptionBit19() != 0)
        FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

// Enables one item per selected car/difficulty level (and up to the fourth
// one) of the two player-setup menus, then stores the number of gears and
// the steering aid read from the game info.
// FUNCTION: CMR2 0x004f02e0
void FrontendMenu_EnablePlayerSetupItems(void)
{
    unsigned int *pInfo = CGameInfo::GetGameInfoField9CAddress();
    int value0;
    int value1;
    unsigned int count;
    int i;

    if (CGameInfo::IsRecordFlagSet(0xd)) {
        count = 8;
    } else if (CGameInfo::GetConfiguredGameMode() == 3 || CGameInfo::GetConfiguredGameMode() == 2) {
        count = *pInfo >> 8 & 0xf;
        if ((*pInfo >> 0xc & 0xf) > count)
            count = *pInfo >> 0xc & 0xf;
        if ((*pInfo >> 0x10 & 0xf) > count)
            count = *pInfo >> 0x10 & 0xf;
    } else {
        switch (CGameInfo::GetConfiguredDifficulty()) {
        case 0:
            count = *pInfo >> 8 & 0xf;
            break;
        case 1:
            count = *pInfo >> 0xc & 0xf;
            break;
        case 2:
            count = *pInfo >> 0x10 & 0xf;
            break;
        }
    }
    for (i = 0; i < 8u; i++) {
        if (i < count || (CGameInfo::GetConfiguredGameMode() != 1 && i < 4))
            FrontendMenu_GetRallySelection()->items[i].enabled = 1;
        else
            FrontendMenu_GetRallySelection()->items[i].enabled = 0;
    }
    GameInfo_GetPlayerOptionNibbles(&value0, &value1);
    FrontendMenu_GetRallySelection()->cursor = (char)value0;
    FrontendMenu_GetRallyStageSelection()->cursor = (char)value1;
}

// Entering the player profile menu (back: undoes the previous player's
// profile). Lists the free saved profiles.
// match 45%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004f03f0
void FrontendMenu_EnterMultiplayerProfile(Menu *pMenu, char back)
{
    MenuItem *pItem;
    char *pText;
    int i;

    if (back != 0) {
        if (RallyData_IsDriverRecordUsable(CGameInfo::GetConfiguredPlayerCount() + (-1 - g_profilePlayersRemaining)) || g_newProfileUndoPending != 0) {
            if (RallyData_IsDriverRecordUsable(CGameInfo::GetConfiguredPlayerCount() + (-1 - g_profilePlayersRemaining))) {
                g_unk0x00819744--;
                RallyData_SetPlayerProfileInUse(CGameInfo::GetConfiguredPlayerCount() + (-1 - g_profilePlayersRemaining), 0);
            }
            Profile_ResetCategoryData((CGameInfo::GetConfiguredPlayerCount() & 0xff) - (g_profilePlayersRemaining & 0xff) - 1);
        }
        Profile_ResetRecordCategory((CGameInfo::GetConfiguredPlayerCount() & 0xff) - (g_profilePlayersRemaining & 0xff) - 1);
        g_profilePlayersRemaining++;
    }
    g_newProfileUndoPending = 0;
    FrontendProfile_SetHeaderMode((CGameInfo::GetConfiguredPlayerCount() & 0xff) - FrontendProfile_GetPlayersRemaining() + 1);
    Frontend_SetOverlayMode((CGameInfo::GetConfiguredPlayerCount() & 0xff) - FrontendProfile_GetPlayersRemaining());
    CSound::NoOpSoundDeviceCallback();
    for (i = 0; i < 4; i++) {
        if (i < SaveProfiles_CountAvailableProfiles()) {
            sprintf(g_profileEntryTexts[i], CFrontend::GetTextString(0x17e), SaveProfiles_GetAvailableProfileName(i));
            pMenu->items[i + 3].enabled = 1;
            pMenu->items[i + 3].visible = 1;
            pMenu->items[i + 3].text = g_profileEntryTexts[i];
        } else {
            pMenu->items[i + 3].enabled = 0;
            pMenu->items[i + 3].visible = 0;
        }
    }
    if (SaveProfiles_CountAvailableProfiles() > 0) {
        pMenu->cursor = 3;
        return;
    }
    pMenu->cursor = 2;
}

// Item callback of "new profile" in the player profile menu.
// FUNCTION: CMR2 0x004f0960
void FrontendProfile_CreateNewPlayerProfile(Menu *pMenu, int param)
{
    Menu *pNext;

    g_newProfileUndoPending = 1;
    FRONTEND_MODE_NEXT_MENU(pNext)
    FrontendProfile_SetCurrentPlayer((CGameInfo::GetConfiguredPlayerCount() & 0xff) - (g_profilePlayersRemaining & 0xff));
    Profile_AssignAvailableCategory((CGameInfo::GetConfiguredPlayerCount() & 0xff) - (g_profilePlayersRemaining & 0xff), -1);
    Profile_ResetCategoryData((CGameInfo::GetConfiguredPlayerCount() & 0xff) - (g_profilePlayersRemaining & 0xff));
    RallyData_SetPlayerProfileInUse(CGameInfo::GetConfiguredPlayerCount() - g_profilePlayersRemaining, 0);
    Menu_SetParent(FrontendMenu_GetProfileNameEntry(), pMenu);
    FrontendProfile_SetRenameMenu(FrontendMenu_GetProfileRenameEntry());
    if (CGameInfo::GetGameModeOptionBit19() != 0)
        FrontendProfile_SetCompletionMenu(FrontendMenu_GetNetworkConnection());
    else
        FrontendProfile_SetCompletionMenu(pNext);
    sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0xdc),
            (CGameInfo::GetConfiguredPlayerCount() & 0xff) - (g_profilePlayersRemaining & 0xff) + 1);
    FrontendProfile_SetHeaderText(CFrontend::m_stringDest);
    g_profilePlayersRemaining--;
    Menu_SetParent(pNext, pMenu);
    Menu_SetNextAction(FrontendMenu_GetProfileNameEntry());
}

// Item callback of a saved profile in the player profile menu.
// FUNCTION: CMR2 0x004f0c50
void FrontendProfile_SelectSavedPlayerProfile(Menu *pMenu, int param)
{
    Menu *pNext;
    int profile;

    profile = SaveProfiles_GetAvailableProfileIndex(pMenu->cursor - 3);
    FRONTEND_MODE_NEXT_MENU(pNext)
    Profile_AssignAvailableCategory((CGameInfo::GetConfiguredPlayerCount() & 0xff) - (g_profilePlayersRemaining & 0xff), profile);
    RallyData_SetPlayerProfileInUse(CGameInfo::GetConfiguredPlayerCount() - g_profilePlayersRemaining, 0);
    g_profilePlayersRemaining--;
    if (CGameInfo::GetGameModeOptionBit19() != 0) {
        Menu_SetParent(FrontendMenu_GetNetworkConnection(), pMenu);
        Menu_SetNextAction(FrontendMenu_GetNetworkConnection());
        return;
    }
    Menu_SetParent(pNext, pMenu);
    Menu_SetNextAction(pNext);
}

// Callback that steps the player list back one player while setting up a
// championship.
// FUNCTION: CMR2 0x004f0da0
void FrontendMenu_LeaveProfileNameEntry(int param_1, char param_2)
{
    Frontend_SetOverlayMode(0);
    Menu_SetActionLatch(0);
    if (param_2 != 0) {
        FrontendProfile_SetNameEntryFlag(0);
        if (CGameInfo::GetConfiguredGameMode() == 4) {
            if (FrontendRecords_GetProfileMode() == 0) {
                g_unk0x00819744--;
                if (g_unk0x00819744 > 0) {
                    g_profilePlayersRemaining++;
                    FrontendProfile_SetCurrentPlayer((CGameInfo::GetConfiguredPlayerCount() & 0xff) - (g_profilePlayersRemaining & 0xff) - 1);
                    sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0xdc),
                            (CGameInfo::GetConfiguredPlayerCount() & 0xff) - (g_profilePlayersRemaining & 0xff));
                    FrontendProfile_SetHeaderText(CFrontend::m_stringDest);
                    return;
                }
                Menu_SetNextAction(FrontendMenu_GetMultiplayerExtendedDifficulty());
            }
        }
    }
}

// GLOBAL: CMR2 0x00524d38
char g_strLabelText[8] = "%s: %s";
// GLOBAL: CMR2 0x00524d40
char g_strLabelNumber[8] = "%s: %d";
// GLOBAL: CMR2 0x00524d60
char g_strTextCursor[] = "_";
// GLOBAL: CMR2 0x00524d64
char g_strLabelSpacedText[8] = "%s : %s";

unsigned int RallyData_GetKnockoutModeBits(void);
unsigned int RallyData_GetKnockoutCountryNibble(void);
void RallyData_SetKnockoutModeBits(BYTE param1);
void RallyData_SetKnockoutBits6To8(BYTE param1);
void RallyData_SetKnockoutCountryNibble(BYTE param1);
void RallyData_InitKnockoutBracket(void);
void RallyData_SelectKnockoutCountryStage(void);
void GameInfo_SetGameModeOptionBits20To22(BYTE param1);
int GameInfo_CountSetBits(unsigned int value);

// Number of stages the player count allows: 1-2 players 1, 3-4 players 2,
// more 3.
#define FRONTEND_PLAYER_GROUP(call)         \
    switch (CGameInfo::GetConfiguredPlayerCount()) {    \
    case 1:                                 \
    case 2:                                 \
        call(1);                            \
        break;                              \
    case 3:                                 \
    case 4:                                 \
        call(2);                            \
        break;                              \
    default:                                \
        call(3);                            \
        break;                              \
    }

// Entering the multiplayer race settings page: loads the current settings
// (and the ranges the player count allows) into its entries.
// FUNCTION: CMR2 0x004ef970
void FrontendMenu_EnterMultiplayerRaceSettings(Menu *pMenu, int param)
{
    switch (CGameInfo::GetConfiguredPlayerCount()) {
    case 1:
    case 2:
        g_unk0x00819050 = 0;
        break;
    case 3:
    case 4:
        g_unk0x00819050 = 1;
        break;
    default:
        g_unk0x00819050 = 2;
        break;
    }
    if (CGameInfo::GetConfiguredPlayerCount() == 1) {
        if (CGameInfo::GetGameModeOptionBits20To22() == 0) {
            GameInfo_SetGameModeOptionBits20To22(CGameInfo::GetConfiguredDifficulty() + 1);
            pMenu->items[0].max = CGameInfo::GetConfiguredDifficulty();
        } else {
            pMenu->items[0].max = CGameInfo::GetGameModeOptionBits20To22() - 1;
        }
        pMenu->items[0].min = 3;
        g_unk0x00819748 = 1;
    } else {
        pMenu->items[0].max = CGameInfo::GetGameModeOptionBits20To22();
        pMenu->items[0].min = 4;
        g_unk0x00819748 = 0;
    }
    if (CGameInfo::GetGameModeOptionBits20To22() == 0) {
        FRONTEND_PLAYER_GROUP(RallyData_SetKnockoutModeBits)
        pMenu->items[1].enabled = 0;
        pMenu->items[1].max = RallyData_GetKnockoutModeBits() - 1;
    } else {
        pMenu->items[1].enabled = 1;
    }
    pMenu->items[1].min = 4 - g_unk0x00819050;
    pMenu->items[1].max = RallyData_GetKnockoutModeBits() - 1;
    if (pMenu->items[1].max >= pMenu->items[1].min)
        pMenu->items[1].max = pMenu->items[1].min - 1;
    switch ((BYTE)RallyData_GetKnockoutCountryNibble()) {
    case 1:
        pMenu->items[2].max = 1;
        break;
    case 3:
        pMenu->items[2].max = 2;
        break;
    case 5:
        pMenu->items[2].max = 3;
        break;
    case 8:
        pMenu->items[2].max = 0;
        break;
    case 7:
        pMenu->items[2].max = 4;
        break;
    default:
        pMenu->items[2].max = 0;
        break;
    }
    if (CGameInfo::IsRecordFlagSet(0xd))
        pMenu->items[2].min = 5;
    else
        pMenu->items[2].min = GameInfo_CountSetBits(CGameInfo::GetGameInfoField9CAddress()[1] & 0x1f) + 1;
    if (pMenu->items[2].max >= pMenu->items[2].min)
        pMenu->items[2].max = pMenu->items[2].min - 1;
}

// Item callback of "start" on the multiplayer race settings page: stores
// the settings and starts the knockout.
// FUNCTION: CMR2 0x004f01c0
void FrontendMenu_StartMultiplayerKnockout(Menu *pMenu, int param)
{
    if (g_unk0x00819748 != 0)
        GameInfo_SetGameModeOptionBits20To22(pMenu->items[0].max + 1);
    else
        GameInfo_SetGameModeOptionBits20To22(pMenu->items[0].max);
    RallyData_SetKnockoutModeBits(pMenu->items[1].max + g_unk0x00819050 + 1);
    RallyData_SetKnockoutBits6To8(1);
    switch (pMenu->items[2].max) {
    case 1:
        RallyData_SetKnockoutCountryNibble(1);
        break;
    case 2:
        RallyData_SetKnockoutCountryNibble(3);
        break;
    case 3:
        RallyData_SetKnockoutCountryNibble(5);
        break;
    case 4:
        RallyData_SetKnockoutCountryNibble(7);
        break;
    case 0:
        RallyData_SetKnockoutCountryNibble(8);
        break;
    default:
        RallyData_SetKnockoutCountryNibble(8);
        break;
    }
    RallyData_InitKnockoutBracket();
    RallyData_SelectKnockoutCountryStage();
    Menu_SetNextAction(FrontendMenu_GetRallyStartTransition());
}

// Update callback of the multiplayer race settings page.
// FUNCTION: CMR2 0x004f0250
void FrontendMenu_UpdateMultiplayerRaceSettings(Menu *pMenu)
{
    if (g_unk0x00819748 != 0)
        GameInfo_SetGameModeOptionBits20To22(pMenu->items[0].max + 1);
    else
        GameInfo_SetGameModeOptionBits20To22(pMenu->items[0].max);
    if (CGameInfo::GetGameModeOptionBits20To22() == 0) {
        FRONTEND_PLAYER_GROUP(RallyData_SetKnockoutModeBits)
        pMenu->items[1].enabled = 0;
        pMenu->items[1].max = RallyData_GetKnockoutModeBits() + (-1 - g_unk0x00819050);
        return;
    }
    pMenu->items[1].enabled = 1;
}

// Draw callback of the rally menu: one row per rally, drawn in the layout of
// its item value (title only, or with the stage names of the rally), over the
// strip of the selected rows, plus the carousel.
// FUNCTION: CMR2 0x004df410
void FrontendMenu_DrawGameOptions(Menu *pMenu)
{
    SpriteRect dst;
    BYTE *pColour;
    BYTE *pShadow;
    int top;
    int x;
    int i;
    int j;

    dst.x = (int)(g_pGraphics->resX * 100) / 640;
    dst.y = 0;
    dst.w = CFrontend::m_pAr640ATexture->width;
    dst.h = CFrontend::m_pAr640ATexture->height;
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, 2, NULL, -1);
    top = ((int)(g_pGraphics->resY * 8) / 480 + (int)(g_pGraphics->resY * 38) / 480 +
           (int)(g_pGraphics->resY * 384) / 480) / 2 -
          (int)((int)(g_pGraphics->resY * 36) / 480 * pMenu->itemCount) / 2;
    if (pMenu->cursor == 0) {
        pColour = g_colourWhite0x00524968;
        pShadow = g_colourShadowWhite0x00524974;
    } else {
        pColour = g_colourText0x0052496c;
        pShadow = g_colourShadowText0x00524978;
    }
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 99) / 640;
    g_frontendDrawRect[1] = top;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 282) / 640;
    g_frontendDrawRect[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pShadow, 1);
    g_frontendDrawRect[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pColour, 1);
    for (i = 0; i < pMenu->itemCount; i++) {
        dst.y = (int)(g_pGraphics->resY * 20) / 480 +
                (top + ((int)(g_pGraphics->resY * 36) / 480 * i - CFrontend::m_pAr640ATexture->height / 2));
        if (pMenu->cursor == i) {
            pColour = g_colourWhite0x00524968;
            Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, &dst, CFrontend::m_pAr640ATexture, 1, 0, NULL, NULL, pColour, 8);
        } else {
            pColour = g_colourText0x0052496c;
            Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, &dst, CFrontend::m_pAr640DTexture, 1, 0, NULL, NULL, pColour, 8);
        }
        switch (pMenu->items[i].value) {
        case 3:
            strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pMenu->items[i].id));
            x = (int)(g_pGraphics->resX * 122) / 640;
            Font_DrawText(1, CFrontend::m_stringDest, x,
                          (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
            for (j = 0; j < pMenu->items[i].min; j++) {
                pColour = g_colourWhite0x00524968;
                if (pMenu->items[i].max != j)
                    pColour = g_colourText0x0052496c;
                x = (int)(g_pGraphics->resX * 10) / 640 + x +
                    Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
                Font_DrawText(1, CFrontend::GetTextString(j + 0x135), x,
                              (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(j + 0x135));
            }
            break;
        case 4:
            strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pMenu->items[i].id));
            x = (int)(g_pGraphics->resX * 122) / 640;
            Font_DrawText(1, CFrontend::m_stringDest, x,
                          (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
            for (j = 0; j < pMenu->items[i].min; j++) {
                pColour = g_colourWhite0x00524968;
                if (pMenu->items[i].max != j)
                    pColour = g_colourText0x0052496c;
                x = (int)(g_pGraphics->resX * 10) / 640 + x +
                    Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
                Font_DrawText(1, CFrontend::GetTextString(j + 0x152), x,
                              (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(j + 0x152));
            }
            break;
        case 2:
            strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pMenu->items[i].id));
            x = (int)(g_pGraphics->resX * 122) / 640;
            Font_DrawText(1, CFrontend::m_stringDest, x,
                          (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
            for (j = 0; j < pMenu->items[i].min; j++) {
                pColour = g_colourWhite0x00524968;
                if (pMenu->items[i].max != j)
                    pColour = g_colourText0x0052496c;
                x = (int)(g_pGraphics->resX * 10) / 640 + x +
                    Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
                Font_DrawText(1, CFrontend::GetTextString(0x134 - j), x,
                              (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(0x134 - j));
            }
            break;
        case 0:
            strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pMenu->items[i].id));
            x = (int)(g_pGraphics->resX * 122) / 640;
            Font_DrawText(1, CFrontend::m_stringDest, x,
                          (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
            for (j = 0; j < pMenu->items[i].min; j++) {
                pColour = g_colourWhite0x00524968;
                if (pMenu->items[i].max != j)
                    pColour = g_colourText0x0052496c;
                x = (int)(g_pGraphics->resX * 10) / 640 + x +
                    Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
                Font_DrawText(1, CFrontend::GetTextString(j + 0x137), x,
                              (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(j + 0x137));
            }
            break;
        default:
            Font_DrawText(1, CFrontend::GetTextString(pMenu->items[i].id),
                          (int)(g_pGraphics->resX * 122) / 640,
                          (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
            break;
        }
        if (pMenu->cursor == i || pMenu->cursor == i + 1) {
            pColour = g_colourWhite0x00524968;
            pShadow = g_colourShadowWhite0x00524974;
        } else {
            pColour = g_colourText0x0052496c;
            pShadow = g_colourShadowText0x00524978;
        }
        g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 36) / 480 * (i + 1) + top;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pShadow, 1);
        g_frontendDrawRect[1]++;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pColour, 1);
    }
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

// Draw callback of the multiplayer race settings page.
// match 84%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004e1230
void FrontendMenu_DrawMultiplayerRaceSettings(Menu *pMenu)
{
    short icon[4];
    char *text[3];
    BYTE *pShadow;
    BYTE *pColour;
    short y0;
    int id;
    int i;

    icon[0] = (int)(g_pGraphics->resX * 100) / 640;
    icon[1] = 0;
    icon[2] = CFrontend::m_pAr640ATexture->width;
    icon[3] = CFrontend::m_pAr640ATexture->height;
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 440) / 640;
    g_frontendDrawRect[3] = (int)(g_pGraphics->resY * 100) / 480;
    g_frontendDrawRect[1] = (short)(((int)(g_pGraphics->resY * 56) / 480 + (int)(g_pGraphics->resY * 374) / 480) / 2)
                         - g_frontendDrawRect[3] / 2;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 160) / 640;
    FrontendDraw_PlayTime();
    text[0] = CFrontend::GetTextString(0xe7);
    text[1] = CFrontend::GetTextString(0xe);
    text[2] = CFrontend::GetTextString(0x65);
    FrontendDraw_Breadcrumb((int)(g_pGraphics->resX * 24) / 640, (int)(g_pGraphics->resY * 38) / 480, text, 3);
    y0 = (short)(((int)(g_pGraphics->resY * 56) / 480 + (int)(g_pGraphics->resY * 374) / 480) / 2)
         - (short)((int)(g_pGraphics->resY * 36) / 480 * pMenu->itemCount / 2);
    if (pMenu->cursor == 0) {
        pColour = g_colourWhite0x00524968;
        pShadow = g_colourShadowWhite0x00524974;
    } else {
        pColour = g_colourText0x0052496c;
        pShadow = g_colourShadowText0x00524978;
    }
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 99) / 640;
    g_frontendDrawRect[1] = y0;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 282) / 640;
    g_frontendDrawRect[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pShadow, 1);
    g_frontendDrawRect[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pColour, 1);
    for (i = 0; i < pMenu->itemCount; i++) {
        icon[1] = (int)(g_pGraphics->resY * 20) / 480 + y0 + (int)(g_pGraphics->resY * 36) / 480 * (short)i
                  - CFrontend::m_pAr640ATexture->height / 2;
        if (pMenu->cursor == i) {
            pColour = g_colourWhite0x00524968;
            Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, (SpriteRect *)icon, CFrontend::m_pAr640ATexture, 1, 0, NULL, NULL, pColour, 8);
        } else {
            if (pMenu->items[i].enabled)
                pColour = g_colourText0x0052496c;
            else
                pColour = g_colourDim0x00524970;
            Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, (SpriteRect *)icon, CFrontend::m_pAr640DTexture, 1, 0, NULL, NULL, pColour, 8);
        }
        switch (i) {
        case 0:
            if (FrontendChampionship_GetRouteSelectionFlag() != 0) {
                switch (pMenu->items[0].max) {
                case 0:
                    sprintf(CFrontend::m_stringDest, g_strLabelText, CFrontend::GetTextString(0x142), CFrontend::GetTextString(0x43));
                    break;
                case 1:
                    sprintf(CFrontend::m_stringDest, g_strLabelText, CFrontend::GetTextString(0x142), CFrontend::GetTextString(0x44));
                    break;
                case 2:
                    sprintf(CFrontend::m_stringDest, g_strLabelText, CFrontend::GetTextString(0x142), CFrontend::GetTextString(0x45));
                    break;
                }
            } else {
                switch (pMenu->items[0].max) {
                case 0:
                    sprintf(CFrontend::m_stringDest, g_strLabelText, CFrontend::GetTextString(0x142), CFrontend::GetTextString(0x133));
                    break;
                case 1:
                    sprintf(CFrontend::m_stringDest, g_strLabelText, CFrontend::GetTextString(0x142), CFrontend::GetTextString(0x43));
                    break;
                case 2:
                    sprintf(CFrontend::m_stringDest, g_strLabelText, CFrontend::GetTextString(0x142), CFrontend::GetTextString(0x44));
                    break;
                case 3:
                    sprintf(CFrontend::m_stringDest, g_strLabelText, CFrontend::GetTextString(0x142), CFrontend::GetTextString(0x45));
                    break;
                }
            }
            goto draw;
        case 1:
            sprintf(CFrontend::m_stringDest, g_strLabelNumber, CFrontend::GetTextString(0x143),
                    FrontendChampionship_GetRouteIndexOffset() + pMenu->items[1].max + 1);
            break;
        case 2:
            if (pMenu->items[2].max == 0)
                sprintf(CFrontend::m_stringDest, g_strLabelSpacedText, CFrontend::GetTextString(0x31),
                        CFrontend::GetTextString(0x2f));
            else
                sprintf(CFrontend::m_stringDest, g_strLabelSpacedText, CFrontend::GetTextString(0x31),
                        CFrontend::GetTextString(pMenu->items[2].max * 2 + 0x26));
            break;
        case 3:
            strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(0x67));
            break;
        case 4:
            strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(0x1b));
            break;
        }
    draw:
        Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 640,
                      (short)((int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1]), (int *)pColour, 0x11);
        if (pMenu->cursor == i || pMenu->cursor == i + 1) {
            pColour = g_colourWhite0x00524968;
            pShadow = g_colourShadowWhite0x00524974;
        } else {
            pColour = g_colourText0x0052496c;
            pShadow = g_colourShadowText0x00524978;
        }
        g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 36) / 480 * ((short)i + 1) + y0;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pShadow, 1);
        g_frontendDrawRect[1]++;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pColour, 1);
    }
    FrontendDraw_HelpText(CFrontend::GetTextString(0x57), 1);
}

int FrontendNetwork_GetMessageState(void);

// Draw callback of the network message page: title and the message of the
// current network state.
// FUNCTION: CMR2 0x004e96d0
void FrontendMenu_DrawNetworkMessage(Menu *pMenu)
{
    char *text[2];
    int id;

    text[0] = CFrontend::GetTextString(0);
    text[1] = CFrontend::GetTextString(0x1e2);
    FrontendDraw_PlayTime();
    FrontendDraw_Breadcrumb((int)(g_pGraphics->resX * 24) / 640, (int)(g_pGraphics->resY * 38) / 480, text, 2);
    switch (FrontendNetwork_GetMessageState()) {
    case 0:
        Font_DrawText(1, CFrontend::GetTextString(0x1e3), (int)g_pGraphics->resX / 2, (int)g_pGraphics->resY / 2,
                      (int *)g_colourWhite0x00524968, 0x12);
        break;
    case 1:
        Font_DrawText(1, CFrontend::GetTextString(0x1e4), (int)g_pGraphics->resX / 2, (int)g_pGraphics->resY / 2,
                      (int *)g_colourWhite0x00524968, 0x12);
        break;
    case 2:
        Font_DrawText(1, CFrontend::GetTextString(0x1e5), (int)g_pGraphics->resX / 2, (int)g_pGraphics->resY / 2,
                      (int *)g_colourWhite0x00524968, 0x12);
        break;
    case 3:
        Font_DrawText(1, CFrontend::GetTextString(0x1e6), (int)g_pGraphics->resX / 2, (int)g_pGraphics->resY / 2,
                      (int *)g_colourWhite0x00524968, 0x12);
        break;
    default:
        goto done;
    }
done:
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

// Draw callback of the arcade menu.
// FUNCTION: CMR2 0x004e2500
void FrontendMenu_DrawArcade(Menu *pMenu)
{
    char *text[1];

    text[0] = CFrontend::GetTextString(0x94);
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, (int)(g_pGraphics->resX * 24) / 640, (int)(g_pGraphics->resY * 38) / 480, 1, 2, text, 1);
    FrontendDraw_MenuList(pMenu, NULL, -1, -1, 0, 1);
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

void FrontendProfile_SetPlayersRemaining(int value);

// Leaving the arcade menu: sets the arcade mode of the entry chosen.
// FUNCTION: CMR2 0x004ef740
void FrontendMenu_LeaveArcade(Menu *pMenu, char back)
{
    if (back != 0)
        return;
    GameInfo_SetConfiguredMultiplayer(0);
    switch (pMenu->cursor) {
    case 0:
        GameInfo_SetConfiguredGameMode(5);
        break;
    case 1:
        GameInfo_SetConfiguredGameMode(6);
        break;
    case 2:
        GameInfo_SetConfiguredGameMode(7);
        GameInfo_SetConfiguredPlayerCount(1);
        FrontendProfile_SetPlayersRemaining(CGameInfo::GetConfiguredPlayerCount());
        GameInfo_SetConfiguredMultiplayer(0);
        RallyData_SetCountrySelectionBits(0);
        RallyData_SetStageSelectionAndRefreshFlags(0);
        return;
    }
    RallyData_SetCountrySelectionBits(0);
    RallyData_SetStageSelectionAndRefreshFlags(0);
}

void GameInfo_SetConfiguredDifficulty(BYTE param1);
void FrontendMenu_SetupSinglePlayerArcade(void);

// Item callback of an arcade game type.
// FUNCTION: CMR2 0x004ef930
void FrontendMenu_SelectCarClass(Menu *pMenu, int param)
{
    GameInfo_SetConfiguredDifficulty(pMenu->cursor);
    if (CGameInfo::GetConfiguredGameMode() == 5)
        FrontendMenu_SetupSinglePlayerArcade();
}

// GLOBAL: CMR2 0x00524d48
char g_strLabelColon[4] = "%s:";
// Game type the second quick race page was last showing.
// GLOBAL: CMR2 0x00829320
unsigned int g_quickRaceShownGameType;

unsigned int GameInfo_GetNetworkOptionBits8To10(void);
unsigned int GameInfo_GetNetworkOptionBits3To7(void);
unsigned int GameInfo_GetNetworkOptionBits12To15(void);
unsigned int GameInfo_GetNetworkOptionBits16To19(void);
void GameInfo_SetNetworkOptionBits8To10(unsigned int param1);
void GameInfo_SetNetworkOptionBits3To7(unsigned int param1);
void GameInfo_SetNetworkOptionBit11(BYTE param1);
void GameInfo_SetNetworkOptionBits12To15(unsigned int param1);
void GameInfo_SetNetworkOptionBits16To19(unsigned int param1);
unsigned int RallyData_GetSetupModeBits(void);
void RallyData_SetSelectionBits16To19(BYTE param1);
void RallyData_SetSetupModeBits(unsigned int param1);
void RallyData_SetSetupHighNibble(unsigned int param1);
void RallyData_SetSetupLowNibble(unsigned int param1);
MenuScroller *FrontendScroller_GetArcadeRallyScroller(void);

// Draws the on/off choice of a settings row after the text in m_stringDest.
// Draws the yes/no pair of a boolean row, the current choice in white.
#define QUICKRACE_DRAW_CHOICE(value, y) \
    if ((value) != 0) { \
        Font_DrawText(1, CFrontend::GetTextString(0x133), (int)(g_pGraphics->resX * 10) / 640 + (int)(g_pGraphics->resX * 0x7a) / 640 + Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest), y, (int *)g_colourText0x0052496c, 0x11); \
        Font_DrawText(1, CFrontend::GetTextString(0x134), (int)(g_pGraphics->resX * 20) / 640 + (int)(g_pGraphics->resX * 0x7a) / 640 + Font_GetTextWidth(1, (BYTE *)CFrontend::GetTextString(0x133)) + Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest), y, (int *)g_colourWhite0x00524968, 0x11); \
    } else { \
        Font_DrawText(1, CFrontend::GetTextString(0x133), (int)(g_pGraphics->resX * 10) / 640 + (int)(g_pGraphics->resX * 0x7a) / 640 + Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest), y, (int *)g_colourWhite0x00524968, 0x11); \
        Font_DrawText(1, CFrontend::GetTextString(0x134), (int)(g_pGraphics->resX * 20) / 640 + (int)(g_pGraphics->resX * 0x7a) / 640 + Font_GetTextWidth(1, (BYTE *)CFrontend::GetTextString(0x133)) + Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest), y, (int *)g_colourText0x0052496c, 0x11); \
    }

// Draws the frame of a quick race page: path, help, header line; returns the
// y of the first row.
#define QUICKRACE_FRAME()                                                                                   \
    icon[0] = (int)(g_pGraphics->resX * 100) / 640;                                                         \
    icon[1] = 0;                                                                                            \
    icon[2] = CFrontend::m_pAr640ATexture->width;                                                           \
    icon[3] = CFrontend::m_pAr640ATexture->height;                                                          \
    text[0] = CFrontend::GetTextString(0x94);                                                               \
    text[1] = CFrontend::GetTextString(0xe2);                                                               \
    FrontendDraw_PlayTime();                                                                                \
    FrontendDraw_MenuPath(pMenu, (int)(g_pGraphics->resX * 24) / 640, (int)(g_pGraphics->resY * 38) / 480, 1, 3, text, 2); \
    FrontendDraw_HelpText(CFrontend::GetTextString(0x57), 1);                                               \
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 440) / 640;                                              \
    g_frontendDrawRect[3] = (int)(g_pGraphics->resY * 100) / 480;                                              \
    g_frontendDrawRect[1] = ((int)(g_pGraphics->resY * 56) / 480 + (int)(g_pGraphics->resY * 374) / 480) / 2   \
                         - g_frontendDrawRect[3] / 2;                                                          \
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 160) / 640;                                              \
    y0 = (short)(((int)(g_pGraphics->resY * 56) / 480 + (int)(g_pGraphics->resY * 374) / 480) / 2)           \
         - (short)((int)(g_pGraphics->resY * 36) / 480 * pMenu->itemCount / 2);                              \
    if (pMenu->cursor == 0) {                                                                               \
        pColour = g_colourWhite0x00524968;                                                                  \
        pShadow = g_colourShadowWhite0x00524974;                                                            \
    } else {                                                                                                \
        pColour = g_colourText0x0052496c;                                                                   \
        pShadow = g_colourShadowText0x00524978;                                                             \
    }                                                                                                       \
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 99) / 640;                                               \
    g_frontendDrawRect[1] = y0;                                                                                \
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 282) / 640;                                              \
    g_frontendDrawRect[3] = 1;                                                                                 \
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pShadow, 1);                                 \
    g_frontendDrawRect[1]++;                                                                                   \
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pColour, 1);

// Icon and colour of row i of a quick race page.
#define QUICKRACE_ROW_ICON(i)                                                                               \
    icon[1] = (int)(g_pGraphics->resY * 20) / 480 + y0 + (int)(g_pGraphics->resY * 36) / 480 * (short)(i)   \
              - CFrontend::m_pAr640ATexture->height / 2;                                                    \
    if (pMenu->cursor == (i)) {                                                                             \
        pColour = g_colourWhite0x00524968;                                                                  \
        Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, (SpriteRect *)icon, CFrontend::m_pAr640ATexture, 1, 0, NULL, NULL, pColour, 8); \
    } else {                                                                                                \
        pColour = g_colourText0x0052496c;                                                                   \
        if (!pMenu->items[i].enabled)                                                                       \
            pColour = g_colourDim0x00524970;                                                                \
        Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, (SpriteRect *)icon, CFrontend::m_pAr640DTexture, 1, 0, NULL, NULL, pColour, 8); \
    }

// Separator under row i of a quick race page.
#define QUICKRACE_ROW_LINE(i)                                                                               \
    if (pMenu->cursor == (i) || pMenu->cursor == (i) + 1) {                                                 \
        pColour = g_colourWhite0x00524968;                                                                  \
        pShadow = g_colourShadowWhite0x00524974;                                                            \
    } else {                                                                                                \
        pColour = g_colourText0x0052496c;                                                                   \
        pShadow = g_colourShadowText0x00524978;                                                             \
    }                                                                                                       \
    g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 36) / 480 * ((short)(i) + 1) + y0;                       \
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pShadow, 1);                                 \
    g_frontendDrawRect[1]++;                                                                                   \
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pColour, 1);

// Draw callback of the first quick race page (stages, cars, ...).
// FUNCTION: CMR2 0x004da710
void FrontendMenu_DrawQuickRaceSettings(Menu *pMenu)
{
    short icon[4];
    char *text[2];
    BYTE *pShadow;
    BYTE *pColour;
    Texture *pTexture;
    short y0;
    int x0;
    int y;
    int i;

    QUICKRACE_FRAME()
    for (i = 0; i < pMenu->itemCount; i++) {
        QUICKRACE_ROW_ICON(i)
        switch (pMenu->items[i].value) {
        case 0:
            sprintf(CFrontend::m_stringDest, g_strLabelNumber, CFrontend::GetTextString(0x195), pMenu->items[0].max + 1);
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 640, (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
            break;
        case 1:
            sprintf(CFrontend::m_stringDest, g_strLabelNumber, CFrontend::GetTextString(0x196), pMenu->items[1].max + 1);
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 640, (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
            break;
        case 2:
            sprintf(CFrontend::m_stringDest, g_strLabelColon, CFrontend::GetTextString(0x19e));
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 640, (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
            QUICKRACE_DRAW_CHOICE(pMenu->items[i].max, (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1])
            break;
        default:
            sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue, CFrontend::GetTextString(pMenu->items[i].id),
                    pMenu->items[1].max + 1);
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 640, (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
            break;
        }
        QUICKRACE_ROW_LINE(i)
    }
    FrontendDraw_HelpText(CFrontend::GetTextString(0x57), 1);
    FrontendDraw_ScrollerRow(FrontendScroller_GetArcadeRallyScroller(), 0);
}

// Draw callback of the second quick race page (game type and its settings).
// FUNCTION: CMR2 0x004daf90
void FrontendMenu_DrawQuickRaceAdvancedSettings(Menu *pMenu)
{
    short icon[4];
    char *text[2];
    BYTE *pShadow;
    BYTE *pColour;
    Texture *pTexture;
    short y0;
    int x0;
    int y;
    int i;

    QUICKRACE_FRAME()
    for (i = 0; i < pMenu->itemCount; i++) {
        QUICKRACE_ROW_ICON(i)
        switch (pMenu->items[i].value) {
        case 3:
            sprintf(CFrontend::m_stringDest, g_strLabelText, CFrontend::GetTextString(0x1a3),
                    CFrontend::GetTextString(pMenu->items[i].max + 0x1a4));
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 640, (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
            break;
        case 4:
            sprintf(CFrontend::m_stringDest, g_strLabelNumber, CFrontend::GetTextString(0x195), pMenu->items[i].max + 1);
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 640, (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
            break;
        case 6:
            sprintf(CFrontend::m_stringDest, g_strLabelNumber, CFrontend::GetTextString(0x1a1), pMenu->items[i].max + 1);
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 640, (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
            break;
        case 5:
            sprintf(CFrontend::m_stringDest, g_strLabelNumber, CFrontend::GetTextString(0x1a2), pMenu->items[i].max + 1);
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 640, (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
            break;
        case 2:
            sprintf(CFrontend::m_stringDest, g_strLabelColon, CFrontend::GetTextString(0x19e));
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 640, (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
            QUICKRACE_DRAW_CHOICE(pMenu->items[i].max, (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1])
            break;
        default:
            sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue, CFrontend::GetTextString(pMenu->items[i].id),
                    pMenu->items[1].max + 1);
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 640, (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
            break;
        }
        QUICKRACE_ROW_LINE(i)
    }
    FrontendDraw_HelpText(CFrontend::GetTextString(0x57), 1);
    FrontendDraw_ScrollerRow(FrontendScroller_GetArcadeRallyScroller(), 0);
}

// Item callback of "start" on the first quick race page.
// FUNCTION: CMR2 0x004f0090
void FrontendMenu_StartQuickRace(Menu *pMenu, int param)
{
    RallyData_SetSelectionBits16To19(pMenu->items[0].max + 1);
    GameInfo_SetNetworkOptionBits3To7(pMenu->items[0].max + 1);
    if (CGameInfo::GetConfiguredPlayerCount() == 2) {
        RallyData_SetSecondarySelectionNibble(0);
        GameInfo_SetNetworkOptionBit11(pMenu->items[1].max);
    } else {
        RallyData_SetSecondarySelectionNibble(pMenu->items[1].max + 1);
        GameInfo_SetNetworkOptionBits8To10(pMenu->items[1].max + 1);
        GameInfo_SetNetworkOptionBit11(0);
    }
    Menu_SetNextAction(FrontendMenu_GetRallyStartTransition());
}

// Entering the first quick race page: loads its settings.
// FUNCTION: CMR2 0x004f3220
void FrontendMenu_EnterQuickRaceSettings(Menu *pMenu, int param)
{
    pMenu->items[0].max = GameInfo_GetNetworkOptionBits3To7() - 1;
    if (CGameInfo::GetConfiguredPlayerCount() == 2) {
        pMenu->items[1].value = 2;
        pMenu->items[1].max = 0;
        pMenu->items[1].min = 2;
        pMenu->items[1].max = (BYTE)CGameInfo::GetNetworkOptionBit11() != 0;
        pMenu->cursor = 2;
        return;
    }
    pMenu->items[1].value = 1;
    pMenu->items[1].min = 5;
    BYTE max = GameInfo_GetNetworkOptionBits8To10() - 1;
    pMenu->cursor = 2;
    pMenu->items[1].max = max;
}

// Setting row of the game type chosen in entry 0 of the second quick race
// page.
#define QUICKRACE_TYPE_SETTING(pMenu, type)                          \
    switch (type) {                                                  \
    case 0: {                                                        \
        BYTE m = GameInfo_GetNetworkOptionBits3To7() - 1;                                 \
        pMenu->items[1].min = 10;                                    \
        pMenu->items[1].max = m;                                     \
        pMenu->items[1].value = 4;                                   \
        break; }                                                     \
    case 1: {                                                        \
        BYTE m = GameInfo_GetNetworkOptionBits12To15() - 1;                                 \
        pMenu->items[1].min = 10;                                    \
        pMenu->items[1].max = m;                                     \
        pMenu->items[1].value = 6;                                   \
        break; }                                                     \
    case 2: {                                                        \
        BYTE m = GameInfo_GetNetworkOptionBits16To19() - 1;                                 \
        pMenu->items[1].min = 10;                                    \
        pMenu->items[1].max = m;                                     \
        pMenu->items[1].value = 5;                                   \
        break; }                                                     \
    }

// Item callback of "start" on the second quick race page.
// FUNCTION: CMR2 0x004f0110
void FrontendMenu_StartAdvancedQuickRace(Menu *pMenu, int param)
{
    RallyData_SetSecondarySelectionNibble(0);
    RallyData_SetSetupModeBits(pMenu->items[0].max);
    GameInfo_SetNetworkOptionBit11(pMenu->items[2].max);
    switch (RallyData_GetSetupModeBits()) {
    case 0:
        RallyData_SetSelectionBits16To19(pMenu->items[1].max + 1);
        GameInfo_SetNetworkOptionBits3To7(pMenu->items[1].max + 1);
        break;
    case 1:
        RallyData_SetSetupLowNibble(pMenu->items[1].max + 1);
        GameInfo_SetNetworkOptionBits12To15(pMenu->items[1].max + 1);
        break;
    case 2:
        RallyData_SetSetupHighNibble(pMenu->items[1].max + 1);
        GameInfo_SetNetworkOptionBits16To19(pMenu->items[1].max + 1);
        break;
    }
    Menu_SetNextAction(FrontendMenu_GetRallyStartTransition());
}

// Entering the second quick race page: loads its settings.
// FUNCTION: CMR2 0x004f3280
void FrontendMenu_EnterQuickRaceAdvancedSettings(Menu *pMenu, int param)
{
    unsigned int type;

    g_quickRaceShownGameType = RallyData_GetSetupModeBits();
    pMenu->items[0].max = RallyData_GetSetupModeBits();
    type = RallyData_GetSetupModeBits();
    QUICKRACE_TYPE_SETTING(pMenu, type)
    if ((BYTE)CGameInfo::GetNetworkOptionBit11() != 0) {
        pMenu->items[2].max = 1;
        pMenu->cursor = 3;
        return;
    }
    pMenu->items[2].max = 0;
    pMenu->cursor = 3;
}

// Update callback of the second quick race page: when the game type changes
// the setting row follows it; the setting is stored as it changes.
// FUNCTION: CMR2 0x004f3310
void FrontendMenu_UpdateQuickRaceAdvancedSettings(Menu *pMenu)
{
    unsigned int type;

    type = pMenu->items[0].max;
    if (g_quickRaceShownGameType != type) {
        QUICKRACE_TYPE_SETTING(pMenu, type)
        g_quickRaceShownGameType = pMenu->items[0].max;
    }
    switch (pMenu->items[0].max) {
    case 0:
        RallyData_SetSelectionBits16To19(pMenu->items[1].max + 1);
        GameInfo_SetNetworkOptionBits3To7(pMenu->items[1].max + 1);
        break;
    case 1:
        RallyData_SetSetupLowNibble(pMenu->items[1].max + 1);
        GameInfo_SetNetworkOptionBits12To15(pMenu->items[1].max + 1);
        break;
    case 2:
        RallyData_SetSetupHighNibble(pMenu->items[1].max + 1);
        GameInfo_SetNetworkOptionBits16To19(pMenu->items[1].max + 1);
        break;
    }
    FrontendScroller_UpdateCursorSlide(FrontendScroller_GetArcadeRallyScroller());
}

// Draw callback of the display device menu: one row per device name.
// FUNCTION: CMR2 0x004e1920
void FrontendMenu_DrawDisplayDevice(Menu *pMenu)
{
    char name[80];
    char description[80];
    short icon[4];
    BYTE *pShadow;
    BYTE *pColour;
    Texture *pTexture;
    short y0;
    int i;

    icon[0] = (int)(g_pGraphics->resX * 100) / 640;
    icon[1] = 0;
    icon[2] = CFrontend::m_pAr640ATexture->width;
    icon[3] = CFrontend::m_pAr640ATexture->height;
    y0 = (short)(((int)(g_pGraphics->resY * 8) / 480 + (int)(g_pGraphics->resY * 38) / 480 + (int)(g_pGraphics->resY * 384) / 480) / 2)
         - (short)((int)(g_pGraphics->resY * 36) / 480 * pMenu->itemCount / 2);
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, (int)(g_pGraphics->resX * 24) / 640, (int)(g_pGraphics->resY * 38) / 480, 1, 2, NULL, -1);
    if (pMenu->cursor == 0) {
        pColour = g_colourWhite0x00524968;
        pShadow = g_colourShadowWhite0x00524974;
    } else {
        pColour = g_colourText0x0052496c;
        pShadow = g_colourShadowText0x00524978;
    }
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 99) / 640;
    g_frontendDrawRect[1] = y0;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 282) / 640;
    g_frontendDrawRect[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pShadow, 1);
    g_frontendDrawRect[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pColour, 1);
    for (i = 0; i < pMenu->itemCount; i++) {
        icon[1] = (int)(g_pGraphics->resY * 2) / 480 + (int)(g_pGraphics->resY * 18) / 480 + y0
                  + ((short)((int)(g_pGraphics->resY * 36) / 480) * (short)i - CFrontend::m_pAr640ATexture->height / 2);
        if (pMenu->cursor == i) {
            pColour = g_colourWhite0x00524968;
            Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, (SpriteRect *)icon,
                         CFrontend::m_pAr640ATexture, 1, 0, NULL, NULL, pColour, 8);
        } else {
            pColour = g_colourText0x0052496c;
            if (!pMenu->items[i].enabled)
                pColour = g_colourDim0x00524970;
            Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, (SpriteRect *)icon,
                         CFrontend::m_pAr640DTexture, 1, 0, NULL, NULL, pColour, 8);
        }
        CGraphics::GetDisplayDeviceNames(i, description, name);
        sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue, description);
        Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 640,
                      (short)((int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1]), (int *)pColour, 0x11);
        if (pMenu->cursor == i + 1 || pMenu->cursor == i) {
            pColour = g_colourWhite0x00524968;
            pShadow = g_colourShadowWhite0x00524974;
        } else {
            pColour = g_colourText0x0052496c;
            pShadow = g_colourShadowText0x00524978;
        }
        g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 36) / 480) * ((short)i + 1) + y0;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pShadow, 1);
        g_frontendDrawRect[1]++;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pColour, 1);
    }
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

// FUNCTION: CMR2 0x004f92e0
void FrontendNetwork_SetMessageState(int value)
{
    g_networkMessageState = value;
}

// FUNCTION: CMR2 0x004f92f0
int FrontendNetwork_GetMessageState(void)
{
    return g_networkMessageState;
}

// FUNCTION: CMR2 0x004faa00
void FrontendMenu_EnterArcadeCarClass(Menu *pMenu, int param)
{
    char cursor;

    if ((*CGameInfo::GetGameInfoField9CAddress() & 2) || CGameInfo::IsRecordFlagSet(0xd))
        pMenu->items[2].enabled = 1;
    else
        pMenu->items[2].enabled = 0;
    cursor = CGameInfo::GetConfiguredDifficulty();
    pMenu->cursor = cursor;
    if (cursor >= pMenu->itemCount)
        pMenu->cursor = pMenu->itemCount - 1;
}

// Item callback of the entry value screens: stores the value selected for the
// entry and moves to the next or the parent menu.
// FUNCTION: CMR2 0x004facd0
void FrontendChampionship_SelectRouteEntry(Menu *pMenu, int param)
{
    RallyData_SetDriverCategoryOption(
        CGameInfo::GetConfiguredPlayerCount() + (0xff - g_profilePlayersRemaining),
        (unsigned char)g_screenPaletteIds[pMenu->cursor]);
    RallyData_FillEventSlotSelections();
    if (g_profilePlayersRemaining == 0) {
        Menu_SetNextAction(FrontendMenu_GetArcadeChampionshipTransmission());
        return;
    }
    Menu_SetParent(FrontendMenu_GetMultiplayerProfile(), pMenu);
    Menu_SetNextAction(FrontendMenu_GetMultiplayerProfile());
}

// Item callback of the championship value screens: toggles the entry's
// championship flag and moves to the next or the parent menu.
// FUNCTION: CMR2 0x004fad40
void FrontendChampionship_ToggleRouteEntry(Menu *pMenu, int param)
{
    RallyData_SetDriverAutomaticGearbox(CGameInfo::GetConfiguredPlayerCount() + (0xff - g_profilePlayersRemaining), pMenu->items[0].max);
    if (g_profilePlayersRemaining == 0) {
        Menu_SetNextAction(FrontendMenu_GetArcadeChampionshipTransmission());
        return;
    }
    Menu_SetNextAction(FrontendMenu_GetMultiplayerProfile());
}

// Change callback of the entry value screens: stores the value selected for
// the highlighted entry and refills the event list from it.
// FUNCTION: CMR2 0x004fad90
void FrontendChampionship_UpdateAlternateRouteEntry(Menu *pMenu, int param)
{
    Frontend_SetOverlayMode(0);
    RallyData_SetDriverCategoryOption(
        CGameInfo::GetConfiguredPlayerCount() + (0xff - g_profilePlayersRemaining),
        (unsigned char)g_screenPaletteIds[pMenu->cursor]);
    RallyData_FillEventSlotSelections();
}

// Item callback of the shared entry screens: applies the value of the
// highlighted entry and rewires the parent of the screen the mode goes to.
// FUNCTION: CMR2 0x004fadd0
void FrontendChampionship_ApplySharedRouteEntry(Menu *pMenu, int param)
{
    RallyData_SetDriverAutomaticGearbox(CGameInfo::GetConfiguredPlayerCount() + (0xff - g_profilePlayersRemaining), pMenu->items[0].max);
    if (g_profilePlayersRemaining == 0) {
        if (CGameInfo::GetConfiguredPlayerCount() == 2) {
            FrontendMenu_GetMultiplayerProfile()->pParent = pMenu;
            FrontendMenu_GetArcadeRallySelection()->pParent = pMenu;
            Menu_SetNextAction(FrontendMenu_GetArcadeRallySelection());
        } else {
            FrontendMenu_GetArcadeRallySelection()->pParent = FrontendMenu_GetArcadeCarClass();
            FrontendMenu_GetArcadeCarClass()->pParent = pMenu;
            Menu_SetNextAction(FrontendMenu_GetArcadeCarClass());
        }
    } else {
        if (CGameInfo::GetConfiguredPlayerCount() == 2)
            FrontendMenu_GetMultiplayerProfile()->pParent = pMenu;
        Menu_SetNextAction(FrontendMenu_GetMultiplayerProfile());
    }
}

// Change callback of the second entry value screens: like FrontendChampionship_UpdateAlternateRouteEntry but
// without resetting the mode's selection first.
// FUNCTION: CMR2 0x004fae70
void FrontendChampionship_UpdateArcadeRouteEntry(Menu *pMenu, int param)
{
    RallyData_SetDriverCategoryOption(
        CGameInfo::GetConfiguredPlayerCount() + (0xff - g_profilePlayersRemaining),
        (unsigned char)g_screenPaletteIds[pMenu->cursor]);
    RallyData_FillEventSlotSelections();
}

// Item callback of the second championship value screens: toggles the entry's
// championship flag and moves to the next or the parent menu.
// FUNCTION: CMR2 0x004faea0
void FrontendChampionship_ToggleArcadeRouteEntry(Menu *pMenu, int param)
{
    RallyData_SetDriverAutomaticGearbox(CGameInfo::GetConfiguredPlayerCount() + (0xff - g_profilePlayersRemaining), pMenu->items[0].max);
    if (g_profilePlayersRemaining == 0) {
        Menu_SetNextAction(FrontendMenu_GetArcadeChampionshipRallySelection());
        return;
    }
    Menu_SetNextAction(FrontendMenu_GetMultiplayerProfile());
}

// FUNCTION: CMR2 0x004faef0
void FrontendMenu_EnterArcadeChampionshipTransmission(Menu *pMenu, int param)
{
    unsigned int *pFlags = CGameInfo::GetGameInfoField9CAddress();
    unsigned int count;
    unsigned int i;
    char cursor;

    switch (CGameInfo::GetConfiguredDifficulty()) {
    case 2:
        if (CGameInfo::IsRecordFlagSet(0xd))
            count = 2;
        else
            count = *pFlags >> 6 & 3;
        break;
    case 1:
        if (CGameInfo::IsRecordFlagSet(0xd))
            count = 2;
        else
            count = *pFlags >> 4 & 3;
        break;
    case 0:
        count = *pFlags >> 2 & 3;
        break;
    }
    for (i = 0; i < 2; i++) {
        if (i < count)
            FrontendMenu_GetArcadeChampionshipTransmission()->items[i].enabled = 1;
        else
            FrontendMenu_GetArcadeChampionshipTransmission()->items[i].enabled = 0;
    }
    cursor = RallyData_GetSelectionBits10To11();
    FrontendMenu_GetArcadeChampionshipTransmission()->cursor = cursor;
    if (FrontendMenu_GetArcadeChampionshipTransmission()->cursor >= FrontendMenu_GetArcadeChampionshipTransmission()->itemCount)
        FrontendMenu_GetArcadeChampionshipTransmission()->cursor = FrontendMenu_GetArcadeChampionshipTransmission()->itemCount - 1;
}

// FUNCTION: CMR2 0x004fafd0
void FrontendMenu_UpdateArcadeChampionshipTransmission(Menu *pMenu)
{
    RallyData_SetSelectionBits10To11(pMenu->cursor);
}

// FUNCTION: CMR2 0x004fafe0
void FrontendMenu_SelectArcadeChampionshipTransmission(Menu *pMenu, int param)
{
    RallyData_SetSelectionBits10To11(pMenu->cursor);
    RallyData_SetSelectionBits12To13(0);
    RallyData_SetSelectionBits16To19(3);
    Menu_SetNextAction(FrontendMenu_GetRallyStartTransition());
}

// Item callback that rebuilds a horizontally scrolling menu: the visible item
// count comes from the current difficulty and the item widths are measured
// from the localised strings.
// FUNCTION: CMR2 0x004fb010
void FrontendScroller_InitArcadeRally(Menu *pMenu, int param)
{
    MenuScroller *pScroller;
    unsigned int *pFlags;
    unsigned int count;
    unsigned int n;
    int i;

    Frontend_SetOverlayMode(0);
    pScroller = FrontendScroller_GetArcadeRallyScroller();
    pScroller->startTime = CFrontend::GetFrontendTimestamp();
    pScroller->count = pMenu->itemCount;
    pScroller->pMenu = pMenu;
    for (i = 0; i < 8; i++) {
        FrontendMenu_GetArcadeRallySelection()->items[i].enabled &= ~1;
        FrontendMenu_GetArcadeChampionshipRallySelection()->items[i].enabled &= ~1;
    }
    pFlags = CGameInfo::GetGameInfoField9CAddress();
    if (CGameInfo::GetConfiguredGameMode() == 7) {
        count = (*pFlags >> 2 & 3) * 3;
        n = (*pFlags >> 4 & 3) * 3;
        if (n > count)
            count = n;
        n = (*pFlags >> 6 & 3) * 3;
        if (n > count)
            count = n;
    } else if (CGameInfo::GetConfiguredPlayerCount() == 1) {
        switch (CGameInfo::GetConfiguredDifficulty()) {
        case 0:
            count = (*pFlags >> 2 & 3) * 3;
            break;
        case 1:
            count = (*pFlags >> 4 & 3) * 3;
            break;
        case 2:
            count = (*pFlags >> 6 & 3) * 3;
            break;
        }
    } else {
        count = (*pFlags >> 6 & 3) * 3;
        n = (*pFlags >> 4 & 3) * 3;
        if (count < n)
            count = n;
        n = (*pFlags >> 2 & 3) * 3;
        if (count < n)
            count = n;
    }
    for (i = 0; i < 6; i++) {
        if (i < (int)count || CGameInfo::IsRecordFlagSet(0xd)) {
            FrontendMenu_GetArcadeRallySelection()->items[i].enabled |= 1;
            FrontendMenu_GetArcadeChampionshipRallySelection()->items[i].enabled |= 1;
        } else {
            FrontendMenu_GetArcadeRallySelection()->items[i].enabled &= ~1;
            FrontendMenu_GetArcadeChampionshipRallySelection()->items[i].enabled &= ~1;
        }
    }
    if (CGameInfo::GetConfiguredGameMode() == 7) {
        pScroller->count = 8;
        pMenu->itemCount = 8;
        if ((*pFlags & 0x100000) || CGameInfo::IsRecordFlagSet(0xd))
            FrontendMenu_GetArcadeChampionshipRallySelection()->items[6].enabled |= 1;
        if ((*pFlags & 0x200000) || CGameInfo::IsRecordFlagSet(0xd))
            FrontendMenu_GetArcadeChampionshipRallySelection()->items[7].enabled |= 1;
    } else if (CGameInfo::GetConfiguredDifficulty() == 0 && CGameInfo::GetConfiguredPlayerCount() == 1) {
        pScroller->count = 3;
        pMenu->itemCount = 3;
        for (i = count; i < pScroller->count; i++)
            FrontendMenu_GetArcadeRallySelection()->items[i].enabled &= ~1;
    } else {
        pScroller->count = 8;
        pMenu->itemCount = 8;
        if ((*pFlags & 0x100000) || CGameInfo::IsRecordFlagSet(0xd))
            FrontendMenu_GetArcadeRallySelection()->items[6].enabled |= 1;
        if ((*pFlags & 0x200000) || CGameInfo::IsRecordFlagSet(0xd))
            FrontendMenu_GetArcadeRallySelection()->items[7].enabled |= 1;
    }
    pScroller->spacing = (int)(g_pGraphics->resX * 24) / 640;
    pScroller->count = pMenu->itemCount;
    pScroller->offset = 0;
    pScroller->startOffset = 0;
    for (i = 0; i < pMenu->itemCount; i++) {
        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pMenu->items[i].id));
        CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
        pScroller->widths[i] = Font_GetTextWidth(2, (BYTE *)CFrontend::m_stringDest);
    }
}

// FUNCTION: CMR2 0x004fb360
void FrontendScroller_UpdateArcadeRally(Menu *pMenu)
{
    FrontendScroller_UpdateCursorSlide(FrontendScroller_GetArcadeRallyScroller());
}


// Selects the menu row/page for a front-end list screen.
// FUNCTION: CMR2 0x004fb370
void FrontendMenu_SelectArcadeRallyRow(Menu *pMenu, MenuItem *unused)
{
    RallyData_SetSelectionBits10To11(pMenu->cursor / 3);
    RallyData_SetSelectionBits12To13(pMenu->cursor % 3);
    if (CGameInfo::GetConfiguredPlayerCount() == 2)
        RallyData_SetSecondarySelectionNibble(0);
    else
        RallyData_SetSecondarySelectionNibble(5);
    if (CGameInfo::GetConfiguredGameMode() == 6) {
        if (CGameInfo::GetConfiguredPlayerCount() == 2)
            Menu_SetNextAction(FrontendMenu_GetQuickRaceAdvancedSettings());
        else
            Menu_SetNextAction(FrontendMenu_GetQuickRaceSettings());
        return;
    }
    RallyData_SetSelectionBits16To19(3);
    Menu_SetNextAction(FrontendMenu_GetRallyStartTransition());
}

void GameInfo_SetConfiguredDifficulty(BYTE param1);
void GameInfo_SetConfiguredPlayerCount(BYTE param1);
void GameInfo_SetConfiguredMultiplayer(BYTE param1);
void RallyData_ResetSavedPlayerRecords(void);
void RallyData_SetPlayerProfileInUse(BYTE index, char set);
void Profile_AssignAvailableCategory(int index, int profile);
void Profile_ResetCategoryData(int index);
char *Profile_BuildSavePath(char *pName);
void *Profile_ReadLocalFile(char *param1, int param2);
BYTE *RallyData_GetDriverCategoryProfile(int index);
RallyCarDamageState *RallyData_GetDriverGroupRecord(int index);
CarSetup *RallyData_GetPlayerCarSetup(int index);
BYTE *RallyData_GetDriverEntryRecord(int index);
BYTE RallyData_SetCategoryAwardBit(int index, int bit);
BYTE *RallyData_GetDriverSelectionFlagRecord(int index);
int *RallyData_GetDriverSettingPair(int index);
int *RallyData_GetDriverPrimaryPairRecord(int index);
int *RallyData_GetDriverPairRecord(int index);
int *RallyData_GetDriverSecondaryPairRecord(int index);
void RallyTiming_SetStageTieBreak(int index, char value);
void RallyData_SetStageSelectionAndRefreshFlags(BYTE param1);
void RallyData_SetSelectionStateByte(char param1);
void RallyData_SetSetupFlag11(char param1);

// Restores the original session image: player profiles, cars, controllers,
// driver times and event tables. Unsaved profile bits and the player/AI timing
// order retain their original behaviour.
// FUNCTION: CMR2 0x004fb400
unsigned int FrontendProfile_ApplyDriverProfileBlock(const SavedGameImage *pBlock)
{
    if (!SavedGames_IsValidSessionImage(pBlock, sizeof(SavedGameImage)))
        return 0;
    const SavedGameImage &image = *pBlock;
    GameInfo_SetConfiguredGameMode(image.options.mode);
    GameInfo_SetConfiguredDifficulty(image.options.difficulty);
    GameInfo_SetConfiguredPlayerCount(image.options.playerCount);
    GameInfo_SetConfiguredMultiplayer(image.options.multiplayer);
    RallyData_ResetSavedPlayerRecords();
    {
    for (unsigned i = 0; i < image.options.playerCount; i++) {
        Profile_AssignAvailableCategory(i, -1);
        Profile_ResetCategoryData(i);
        RallyData_SetPlayerProfileInUse((BYTE)i, 0);
        DriverProfile &profile = *(DriverProfile *)RallyData_GetDriverCategoryProfile(i);
        // Preserve the unsaved bits of the original profile option word.
        profile.setup.value0 = image.playerSetup[i].value0;
        profile.setup.value1 = image.playerSetup[i].value1;
        profile.setup.value2 = image.playerSetup[i].value2;
        profile.setup.value3 = image.playerSetup[i].value3;
        profile.setup.value4 = image.playerSetup[i].value4;
        profile.identifier = image.playerIdentifiers[i];
        memcpy(RallyData_GetDriverGroupRecord(i), &image.playerCars[i], sizeof(image.playerCars[i]));
        memcpy(RallyData_GetPlayerCarSetup(i), &image.playerCarSetups[i], sizeof(image.playerCarSetups[i]));
        if ((profile.identifier.flags & 0x200000) == 0) {
            BYTE *buffer = (BYTE *)Profile_ReadLocalFile(Profile_BuildSavePath((char *)&profile.identifier), 0);
            if (buffer != NULL && CGenericFileLoader::m_fileSize == sizeof(DriverProfile)) {
                profile = *(DriverProfile *)buffer;
                profile.identifier = image.playerIdentifiers[i];
                CFileBuffer::FreeGenericFileBuffer(buffer);
            } else {
                CFileBuffer::FreeGenericFileBuffer(buffer);
                RallyData_SetPlayerProfileInUse((BYTE)i, 0);
            }
        }
    }
    }
    {
    for (int i = 0; i < 16; i++) {
        SaveSlot &slot = *(SaveSlot *)RallyData_GetDriverEntryRecord(i);
        slot.car = image.drivers[i].car;
        slot.seconds = image.drivers[i].seconds;
        slot.time = image.drivers[i].time;
        RallyTiming_SetStageTieBreak(i, image.tieBreak[i]);
    }
    }
    {
    for (unsigned i = 0; i < image.options.playerCount; i++) {
        RallyData_SetDriverAutomaticGearbox((BYTE)i, image.drivers[i].entryFlag);
        const SaveSlot &slot = *(const SaveSlot *)RallyData_GetDriverEntryRecord(i);
        RallyData_SetCategoryAwardBit(i, slot.car);
    }
    }
    memcpy(RallyData_GetDriverSettingPair(0), image.settingPairs, sizeof(image.settingPairs));
    memcpy(RallyData_GetDriverPrimaryPairRecord(0), image.primaryValues, sizeof(image.primaryValues));
    memcpy(RallyData_GetDriverSelectionFlagRecord(0), image.selectionFlags, sizeof(image.selectionFlags));
    memcpy(RallyData_GetDriverPairRecord(0), image.values, sizeof(image.values));
    memcpy(RallyData_GetDriverSecondaryPairRecord(0), image.secondaryValues, sizeof(image.secondaryValues));
    RallyData_SetCountrySelectionBits(image.stage.country);
    RallyData_SetStageSelectionAndRefreshFlags(image.stage.stage);
    RallyData_SetSelectionStateByte(image.selectionState);
    FrontendChampionship_RestoreStageTimes();
    RallyUnlockProgress &progress = CGameInfo::m_gameInfo.rallyUnlockProgress;
    const unsigned reached = image.stage.country + 1;
    if (progress.easyRallies < reached)
        progress.easyRallies = reached;
    if (image.options.difficulty >= 1 && progress.normalRallies < reached)
        progress.normalRallies = reached;
    if (image.options.difficulty >= 2 && progress.hardRallies < reached)
        progress.hardRallies = reached;
    RallyData_SetSetupFlag11(0);
    return 1;
}

void FrontendProfile_BuildGuestIdentifier(unsigned int param_1, unsigned int param_2, BYTE param_3, char *param_4);

// Three 12-byte keyboard-style tables scrambled by FrontendProfile_ScrambleIdentifier.
// GLOBAL: CMR2 0x00526ea4
char g_profileScrambleTables[12] = "qaz2wsx3e";
// GLOBAL: CMR2 0x00526eb0
char g_unk0x00526eb0[12] = "/-['=]\\`!Q";
// GLOBAL: CMR2 0x00526ebc
char g_unk0x00526ebc[12] = "\\`!QAZ@WSX";

// Scrambles the dword *pNumber with the three key tables above (seeded by
// *pByte) and turns the result into an identifier string through
// FrontendProfile_BuildGuestIdentifier.
// match 48%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// logic verified against the dump; remaining diff is the operand evaluation
// order of the byte sums and the register/stack split (the original keeps the
// loop pointer in EDI descending to &g_profileScrambleTables[6]).
// FUNCTION: CMR2 0x004fb8d0
void FrontendProfile_ScrambleIdentifier(unsigned int param_1, unsigned int *pNumber, char *pByte, char *pOut)
{
    BYTE bytes[4];
    char *p;
    char seed;
    int i;

    seed = *pByte;
    p = &g_profileScrambleTables[10];
    bytes[0] = (BYTE)*pNumber;
    bytes[1] = (BYTE)(*pNumber >> 8);
    bytes[2] = (BYTE)(*pNumber >> 16);
    bytes[3] = (BYTE)(*pNumber >> 24);
    i = 0;
    do {
        bytes[i] += g_unk0x00526eb0[i] + g_unk0x00526ebc[i] + g_profileScrambleTables[i] + *p * 3;
        i++;
        p--;
    // ptrdiff_t preserves the signed Win32 comparison and native address width.
    } while ((ptrdiff_t)p > (ptrdiff_t)(g_profileScrambleTables + 6));
    FrontendProfile_BuildGuestIdentifier(param_1, (bytes[3] << 24) + (bytes[2] << 16) + (bytes[1] << 8) + bytes[0],
                 g_profileScrambleTables[4] + g_unk0x00526eb0[4] + g_unk0x00526ebc[4] + g_profileScrambleTables[6] +
                     g_unk0x00526eb0[6] + g_unk0x00526ebc[6] + seed,
                 pOut);
}

// Builds a profile-less identifier string: the decimal digits of param_2
// followed by five checksum digits derived from the byte sum of param_2 and
// param_3.
// FUNCTION: CMR2 0x004fb9c0
void FrontendProfile_BuildGuestIdentifier(unsigned int param_1, unsigned int param_2, BYTE param_3, char *param_4)
{
    unsigned short checksum;
    BYTE sum;
    int i;
    int n;

    i = 0;
    sum = (BYTE)(param_2 >> 0x18) + (BYTE)(param_2 >> 0x10) + (BYTE)(param_2 >> 8) + (BYTE)param_2 + param_3 + 2;
    if ((int)param_1 > 0) {
        param_2 ^= param_1;
        param_3 ^= (BYTE)param_1;
    }
    while (param_2 > 0) {
        param_4[i] = (char)(param_2 % 10) + '0';
        i++;
        param_2 /= 10;
    }
    checksum = (unsigned short)(param_3 * 0x100 + sum);
    for (n = 5; n != 0; n--) {
        param_4[i] = (char)(checksum % 10) + '0';
        i++;
        checksum /= 10;
    }
    param_4[i] = 0;
}

// ---------------------------------------------------------------------------
// Cascade layer 0 (W136): forward declarations for cross-module callees.
// ---------------------------------------------------------------------------
char *Network_GetEnumeratedSessionName(BYTE index);
int Session_GetListedUserValue(unsigned int session, BYTE index);
DWORD Network_GetSessionPlayerCount(int index);
DWORD Network_GetSessionMaxPlayers(BYTE index);
BYTE *Profile_GetFileIdentifier(int index);
void RallyData_SetDriverCategoryOption(BYTE index, BYTE value);
unsigned int NetPlayers_GetPlayerFlag6(int index);
int Font_GetTextWidth(BYTE index, BYTE *text);
void NetworkChat_SendLine(char *text);
unsigned int SavedGames_GetDateMiddleBits(int index);
unsigned int SavedGames_GetDateLowBits(int index);
unsigned int SavedGames_GetDifficulty(int index);
void *SavedGames_GetFileName(int index);
BYTE *SavedGames_GetRecordData(int index);
int Network_RebuildSessionPlayerList(void);
extern char g_stageNumberFormat[];

// Draws the "stage select" screen: breadcrumb title, the four column headers
// and, when the entry list is open, one row per entry (name, opponent, stage,
// number and time).
// FUNCTION: CMR2 0x004dc930
void FrontendMenu_DrawNetworkSessionBrowser(Menu *pMenu)
{
    char *names[2];
    int *pColour;
    int *pSelColour;
    int *pUnselColour;
    int row;
    int index;
    int x;

    FrontendDraw_PlayTime();
    names[0] = CFrontend::GetTextString(0x12);
    names[1] = CFrontend::GetTextString(0x35);
    FrontendDraw_Breadcrumb((int)(g_pGraphics->resX * 0x18) / 0x280,
                            (int)(g_pGraphics->resY * 0x26) / 0x1e0, names, 2);
    row = 0;
    if (g_unk0x00819014 != 0) {
        pColour = (int *)g_colourWhite0x00524968;
        pUnselColour = (int *)g_colourText0x0052496c;
        pSelColour = (int *)g_colourWhite0x00524968;
    } else {
        pColour = (int *)g_colourText0x0052496c;
        pSelColour = (int *)g_colourDim0x00524970;
        pUnselColour = (int *)g_colourDim0x00524970;
    }
    Font_DrawText(1, CFrontend::GetTextString(0x7f), (int)(g_pGraphics->resX * 100) / 0x280,
                  (int)(g_pGraphics->resY * 0x37) / 0x1e0, pColour, 10);
    Font_DrawText(1, CFrontend::GetTextString(0x1a3), (int)(g_pGraphics->resX * 300) / 0x280,
                  (int)(g_pGraphics->resY * 0x37) / 0x1e0, pColour, 10);
    Font_DrawText(1, CFrontend::GetTextString(0x20e), (int)(g_pGraphics->resX * 0x1c2) / 0x280,
                  (int)(g_pGraphics->resY * 0x37) / 0x1e0, pColour, 10);
    Font_DrawText(1, CFrontend::GetTextString(0x20f), (int)(g_pGraphics->resX * 0x226) / 0x280,
                  (int)(g_pGraphics->resY * 0x37) / 0x1e0, pColour, 10);
    if (g_unk0x00818ef4 != 0 && g_unk0x0081901c < g_unk0x00819020 + g_unk0x0081901c) {
        index = g_unk0x0081901c;
        do {
            pColour = pSelColour;
            if (g_selectedNetworkSession != index)
                pColour = pUnselColour;
            Font_DrawText(1, Network_GetEnumeratedSessionName(index), (int)(g_pGraphics->resX * 100) / 0x280, (int)(g_pGraphics->resY * 100) / 0x1e0 + ((int)(g_pGraphics->resY * 0x14) / 0x1e0) * row, pColour, 10);
            Font_DrawText(1, CGameInfo::GetCodeEntryText(Session_GetListedUserValue(index, 0) - 8),
                          (int)(g_pGraphics->resX * 300) / 0x280, (int)(g_pGraphics->resY * 100) / 0x1e0 + ((int)(g_pGraphics->resY * 0x14) / 0x1e0) * row, pColour, 10);
            sprintf(CFrontend::m_stringDest, g_stageNumberFormat, Network_GetSessionPlayerCount(index));
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x1c2) / 0x280, (int)(g_pGraphics->resY * 100) / 0x1e0 + ((int)(g_pGraphics->resY * 0x14) / 0x1e0) * row, pColour, 10);
            sprintf(CFrontend::m_stringDest, g_stageNumberFormat, Network_GetSessionMaxPlayers(index));
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x226) / 0x280, (int)(g_pGraphics->resY * 100) / 0x1e0 + ((int)(g_pGraphics->resY * 0x14) / 0x1e0) * row, pColour, 10);
            row++;
            index++;
        } while (index < g_unk0x00819020 + g_unk0x0081901c);
    }
    FrontendDraw_MenuList(pMenu, NULL, (int)(g_pGraphics->resY * 0x140) / 0x1e0, -1, 0, 1);
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

// ---------------------------------------------------------------------------
// Globals shared by the cascade-layer-0 screens implemented below.
// ---------------------------------------------------------------------------
// GLOBAL: CMR2 0x00524dbc
char g_strFlagSS[4] = "SS";
// GLOBAL: CMR2 0x00524dc0
char g_strNum10[4] = "10";
// GLOBAL: CMR2 0x00524dc4
char g_strNum5[4] = "5";
// GLOBAL: CMR2 0x00524dc8
char g_strNum1[4] = "1";
// GLOBAL: CMR2 0x00524e04
char g_strDisplayModeFormat[12] = "%dx%d:%dbit";
// GLOBAL: CMR2 0x00525290
char g_strValidLetters[68] = "abcdefghijklmnopqrstuvwxyz. ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
// GLOBAL: CMR2 0x005252d4
char g_strValidChars[92] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ., !\"%&*()-=_+`/?\\:;[]{}'~";
// GLOBAL: CMR2 0x00818cdc
char *g_pSessionDetailsText;
// GLOBAL: CMR2 0x00818d08
int g_unk0x00818d08[4];
// GLOBAL: CMR2 0x00818d84
char g_unk0x00818d84[0x24];
// GLOBAL: CMR2 0x00819028
int g_lobbyChatLineLength;

extern char g_keypad2[];
extern char g_keypad3[];
extern char g_keypad4[];
extern char g_keypad6[];
extern char g_keypad7[];
extern char g_keypad8[];
extern char g_keypad9[];
void RallyData_SetCountrySelectionBits(BYTE param1);
void RallyData_RebuildDistinctValueList(void);
int RallyData_GetDistinctValueCount(void);
BYTE *RallyData_GetDistinctValueList(void);

// Draws the network "waiting room" player list: a line per session built from
// the player name, the car and the control assignment, plus the controller
// icon; when there is no session it shows the waiting message.
// FUNCTION: CMR2 0x004e20e0
void FrontendMenu_DrawSingleRallySelection(Menu *pMenu)
{
    char *names[11];
    char *pText[3];
    int index;
    int row;

    names[0] = g_strNum1;
    names[1] = g_keypad2;
    names[2] = g_keypad3;
    names[3] = g_keypad4;
    names[4] = g_strNum5;
    names[5] = g_keypad6;
    names[6] = g_keypad7;
    names[7] = g_keypad8;
    names[8] = g_keypad9;
    names[9] = g_strNum10;
    names[10] = g_strFlagSS;
    pText[0] = CFrontend::GetTextString(0xe7);
    pText[1] = CFrontend::GetTextString(0xc);
    pText[2] = CFrontend::GetTextString(0x8a);
    FrontendDraw_PlayTime();
    FrontendDraw_Breadcrumb((int)(g_pGraphics->resX * 0x18) / 0x280,
                            (int)(g_pGraphics->resY * 0x26) / 0x1e0, pText, 3);
    if (g_unk0x0082ac48 > 0) {
        for (index = 0; index < g_unk0x0082ac48; index++) {
            sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x210),
                    SavedGames_GetDifficulty(g_savedGameListTop + index),
                    CFrontend::GetTextString(SavedGames_GetDateLowBits(g_savedGameListTop + index) + 0x27),
                    names[SavedGames_GetDateMiddleBits(g_savedGameListTop + index)]);
            if (g_savedGameListSelection == g_savedGameListTop + index) {
                Font_DrawText(0, (char *)SavedGames_GetFileName(g_savedGameListTop + index), (int)(g_pGraphics->resX * 0x32) / 0x280, (int)(g_pGraphics->resY * 100) / 0x1e0 + ((int)(g_pGraphics->resY * 0x14) / 0x1e0) * index,
                              (int *)g_colourWhite0x00524968, 0x11);
                Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x9b) / 0x280,
                              (int)(g_pGraphics->resY * 100) / 0x1e0 + ((int)(g_pGraphics->resY * 0x14) / 0x1e0) * index,
                              (int *)g_colourWhite0x00524968, 0x11);
                Font_DrawText(0, (char *)SavedGames_GetRecordData(g_savedGameListTop + index), (int)(g_pGraphics->resX * 0xff) / 0x280,
                              (int)(g_pGraphics->resY * 100) / 0x1e0 + ((int)(g_pGraphics->resY * 0x14) / 0x1e0) * index,
                              (int *)g_colourWhite0x00524968, 0x11);
            } else {
                Font_DrawText(0, (char *)SavedGames_GetFileName(g_savedGameListTop + index), (int)(g_pGraphics->resX * 0x32) / 0x280, (int)(g_pGraphics->resY * 100) / 0x1e0 + ((int)(g_pGraphics->resY * 0x14) / 0x1e0) * index,
                              (int *)g_colourText0x0052496c, 0x11);
                Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x9b) / 0x280,
                              (int)(g_pGraphics->resY * 100) / 0x1e0 + ((int)(g_pGraphics->resY * 0x14) / 0x1e0) * index,
                              (int *)g_colourText0x0052496c, 0x11);
                Font_DrawText(0, (char *)SavedGames_GetRecordData(g_savedGameListTop + index), (int)(g_pGraphics->resX * 0xff) / 0x280,
                              (int)(g_pGraphics->resY * 100) / 0x1e0 + ((int)(g_pGraphics->resY * 0x14) / 0x1e0) * index,
                              (int *)g_colourText0x0052496c, 0x11);
            }
        }
    } else {
        Font_DrawText(0, CFrontend::GetTextString(0x20d), (int)g_pGraphics->resX / 2,
                      (int)g_pGraphics->resY / 2, (int *)g_colourText0x0052496c, 0x12);
    }
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

// Refreshes the "car setup" network menu: rebuilds the transmission item, its
// range and the list of available gearbox types.
// match 57%: asignacion de registros en el recuento y en la tabla de niveles
// FUNCTION: CMR2 0x004ee170
void FrontendMenu_UpdateNetworkCarSetup(Menu *pMenu)
{
    unsigned int *pFlags;
    int max;
    int level;
    unsigned int mask;
    BYTE bits;
    int count;
    int n;
    int i;
    char *pDest;
    int *pList;

    pFlags = CGameInfo::GetGameInfoField9CAddress();
    max = Menu_GetItem(pMenu, 0)->max;
    if (max != g_setupRouteIndex) {
        Menu_GetItem(pMenu, 2)->max = 0;
        g_setupRouteIndex = max;
    }
    if (CGameInfo::IsRecordFlagSet(0xd) == 0) {
        level = max + 1;
        count = 1;
        if ((int)(*pFlags >> 8 & 0xf) >= level)
            count = 4;
        if ((int)(*pFlags >> 0xc & 0xf) >= level)
            count = 8;
        if ((*pFlags & 1) != 0 && (int)(*pFlags >> 0x10 & 0xf) >= level)
            count = 10;
    } else {
        count = 10;
    }
    i = 0;
    if (count > 0) {
        pDest = g_unk0x00818d84;
        do {
            i++;
            sprintf(pDest, g_stageNumberFormat, i);
            pDest += 3;
        } while (i < count);
    }
    sprintf(&g_unk0x00818d84[i * 3], g_strFlagSS);
    if (!(CGameInfo::IsRecordFlagSet(0xd) == 0)) {
        count = (max % 2 != 0) + 10;
    } else {
        if (max % 2 != 0) {
            mask = 1 << ((max + 1) / 2 - 1);
            bits = (BYTE)mask;
            if ((pFlags[1] & mask & 0x1f) != 0 ||
                ((BYTE)(pFlags[1] >> 5 & 0x1f) & bits) != 0 ||
                ((BYTE)(pFlags[1] >> 10 & 0x1f) & bits) != 0)
                count++;
        }
    }
    Menu_GetItem(pMenu, 1)->min = (char)count;
    if (Menu_GetItem(pMenu, 1)->max >= Menu_GetItem(pMenu, 1)->min)
        Menu_GetItem(pMenu, 1)->max = Menu_GetItem(pMenu, 1)->min - 1;
    if ((Menu_GetItem(pMenu, 1)->min == 5 && Menu_GetItem(pMenu, 1)->max == 4) ||
        (Menu_GetItem(pMenu, 1)->min == 9 && Menu_GetItem(pMenu, 1)->max == 8) ||
        (Menu_GetItem(pMenu, 1)->min == 0xb && Menu_GetItem(pMenu, 1)->max == 0xa))
        Menu_GetItem(pMenu, 2)->enabled = 0;
    else
        Menu_GetItem(pMenu, 2)->enabled = 1;
    RallyData_SetCountrySelectionBits((BYTE)max);
    RallyData_RebuildDistinctValueList();
    n = RallyData_GetDistinctValueCount();
    pList = (int *)RallyData_GetDistinctValueList();
    g_unk0x00818d08[0] = 0x156;
    g_setupSelectedRouteIds[0] = -1;
    if (n <= 3) {
        Menu_GetItem(pMenu, 2)->min = (char)n + 1;
        if (0 < n) {
            for (i = 1; i <= n; i++) {
                g_unk0x00818d08[i] = pList[i - 1] + 0x157;
                g_setupSelectedRouteIds[i] = pList[i - 1];
            }
            FrontendNetwork_DrainMessageQueue();
            return;
        }
    } else {
        Menu_GetItem(pMenu, 2)->min = 4;
        g_unk0x00818d08[1] = pList[0] + 0x157;
        g_setupSelectedRouteIds[1] = pList[0];
        g_unk0x00818d08[2] = pList[n / 2] + 0x157;
        g_setupSelectedRouteIds[2] = pList[n / 2];
        g_unk0x00818d08[3] = pList[n - 1] + 0x157;
        g_setupSelectedRouteIds[3] = pList[n - 1];
    }
    FrontendNetwork_DrainMessageQueue();
}

// Draws one row of the championship standings: the entry name placed at
// (param_1 + param_2), faded by the distance; returns whether the row lands
// inside the screen.
// FUNCTION: CMR2 0x004d6870
BYTE FrontendDraw_ChampionshipEntryRow(int param_1, int param_2)
{
    BYTE colourText[4];
    BYTE colourShadow[4];
    int count;
    int index;
    int offset;
    int alpha;

    colourText[0] = g_colourText0x0052496c[0];
    colourText[1] = g_colourText0x0052496c[1];
    colourText[2] = g_colourText0x0052496c[2];
    colourText[3] = g_colourText0x0052496c[3];
    colourShadow[0] = g_colourShadowText0x00524978[0];
    colourShadow[1] = g_colourShadowText0x00524978[1];
    colourShadow[2] = g_colourShadowText0x00524978[2];
    offset = abs(param_2);
    alpha = colourText[3] - offset * 0x32;
    colourText[3] = alpha < 0 ? 0 : alpha;
    colourShadow[3] = (BYTE)offset * 0x32;
    alpha = colourShadow[3];
    colourShadow[3] = alpha < 0 ? 0 : alpha;
    index = (param_1 + param_2) % Profile_GetFileCount();
    if (index < 0)
        index += Profile_GetFileCount();
    sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue, (char *)Profile_GetFileIdentifier(index));
    if (FrontendProfile_IsAlreadyInChampionship(index)) {
        Font_DrawText(2, CFrontend::m_stringDest,
                      g_pGraphics->resX / 2 + g_unk0x00819868 + ((int)(g_pGraphics->resX * 0x50) / 0x280) * param_2,
                      (int)(g_pGraphics->resY * 0x118) / 0x1e0, (int *)colourShadow, 0x12);
    } else if (param_2 == 0) {
        Font_DrawText(2, CFrontend::m_stringDest, g_pGraphics->resX / 2 + g_unk0x00819868,
                      (int)(g_pGraphics->resY * 0x118) / 0x1e0, (int *)g_colourWhite0x00524968, 0x12);
    } else {
        Font_DrawText(2, CFrontend::m_stringDest,
                      g_pGraphics->resX / 2 + g_unk0x00819868 + ((int)(g_pGraphics->resX * 0x50) / 0x280) * param_2,
                      (int)(g_pGraphics->resY * 0x118) / 0x1e0, (int *)colourText, 0x12);
    }
    count = (int)g_pGraphics->resX;
    index = ((count * 0x50) / 0x280) * param_2 + count / 2;
    if (0 < index && index < (count * 0x280) / 0x280)
        return 1;
    return 0;
}

// Draws the display-setup screen: title, the two percentage bars (display and
// mode) and the list of available modes.
// FUNCTION: CMR2 0x004e1d70
void FrontendMenu_DrawDisplayMode(Menu *pMenu)
{
    BYTE colourTop[4];
    BYTE colourBottom[4];
    short rect[4];
    int count;
    int mode;
    int barHeight;
    int barTop;
    int i;
    DWORD width;
    DWORD height;
    DWORD depth;

    colourTop[0] = 0xff;
    colourTop[1] = 0xff;
    colourTop[2] = 0xff;
    colourTop[3] = 0x20;
    colourBottom[0] = 0xff;
    colourBottom[1] = 0xff;
    colourBottom[2] = 0xff;
    colourBottom[3] = 0x80;
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, (int)(g_pGraphics->resX * 0x18) / 0x280,
                          (int)(g_pGraphics->resY * 0x26) / 0x1e0, 1, 2, NULL, -1);
    count = (int)CGraphics::GetDisplayCount();
    if (count > 10)
        count = 10;
    mode = FrontendMenu_GetFirstVisibleDisplayDevice();
    barHeight = (DWORD)(count * 100) / CGraphics::GetDisplayCount();
    barTop = (DWORD)(mode * 100) / CGraphics::GetDisplayCount();
    rect[2] = 0x14;
    rect[3] = 0xd2;
    rect[0] = (short)((int)g_pGraphics->resX / 2) + 0x50;
    rect[1] = 0x5f;
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, colourTop, 1);
    rect[2] = 10;
    rect[3] = (short)(barHeight * 200 / 100);
    rect[0] = (short)((int)g_pGraphics->resX / 2) + 0x55;
    rect[1] = (short)(barTop * 200 / 100 + 100);
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, colourBottom, 1);
    for (i = 0; i < count; i++) {
        CGraphics::GetDisplayMode(i + mode, &width, &height, &depth);
        sprintf(CFrontend::m_stringDest, g_strDisplayModeFormat, width, height, depth);
        if (i + mode == pMenu->items[0].max)
            Font_DrawText(1, CFrontend::m_stringDest, (int)g_pGraphics->resX / 2, (i * 5 + 0x19) * 4,
                          (int *)g_colourWhite0x00524968, 10);
        else
            Font_DrawText(1, CFrontend::m_stringDest, (int)g_pGraphics->resX / 2, (i * 5 + 0x19) * 4,
                          (int *)g_colourText0x0052496c, 10);
    }
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

// Network car-setup screen callback: keeps the transmission and car items in
// sync with the devices, and handles the "player name" text entry.
// match 43%: asignacion de registros; mismo flujo (texto, cursores y dispositivos)
// FUNCTION: CMR2 0x004ed840
void FrontendMenu_UpdateNetworkSessionSetup(Menu *pMenu)
{
    int key;
    int b;
    char c;
    int i;
    unsigned int len;
    char *pc;
    DeviceInfo *pDevice;

    b = 1;
    if (g_lobbyChatSelected != 0) {
        pDevice = CInput::GetAvailableDeviceRecord(0);
        if (!((pDevice->pressed & 8) == 0)) {
            pMenu->cursor = 1;
        } else {
            if ((pDevice->pressed & 4) != 0)
                pMenu->cursor = pMenu->itemCount - 1;
        }
    }
    if (g_unk0x00818ce8 != Menu_GetItem(pMenu, 1)->max) {
        RallyData_SetDriverCategoryOption(0, (BYTE)g_filteredRouteIds[Menu_GetItem(pMenu, 1)->max]);
        FrontendNetwork_SendPlayerDescription();
        g_unk0x00818ce8 = Menu_GetItem(pMenu, 1)->max;
    }
    if (g_unk0x00818ce0 != Menu_GetItem(pMenu, 2)->max) {
        RallyData_SetDriverAutomaticGearbox(0, Menu_GetItem(pMenu, 2)->max);
        FrontendNetwork_SendPlayerDescription();
        g_unk0x00818ce0 = Menu_GetItem(pMenu, 2)->max;
    }
    if (g_lobbyLastCursor != pMenu->cursor) {
        if (pMenu->items[pMenu->cursor].value != 0) {
            if (pMenu->items[g_lobbyLastCursor].value == 0) {
                FrontendMenu_SetInputState(0);
                Menu_SetFlags(pMenu, 1, 1, 1, 1);
            }
            g_lobbyChatSelected = 0;
        } else {
            Input_ClearCharacterQueue();
            g_lobbyChatLine[0] = 0;
            g_lobbyChatLineLength = 0;
            g_lobbyChatSelected = 1;
            FrontendMenu_SetInputState(1);
            Menu_SetFlags(pMenu, 0, 0, 0, 0);
        }
        g_lobbyLastCursor = pMenu->cursor;
    }
    Network_RebuildSessionPlayerList();
    if (g_lobbyChatSelected != 0)
        Sys_OfferTextInput(CFrontend::GetTextString(0x1c6), (char *)g_lobbyChatLine, 0xff);   // PORT
    if (g_lobbyChatSelected != 0 && Input_PopQueuedCharacter(&key)) {
        switch (key) {
        default:
            pc = strchr(g_strValidChars, (char)key);
            if (pc != NULL) {
                i = Font_GetTextWidth(1, g_lobbyChatLine);
                int maxWidth = (int)(((g_pGraphics->resX < 0x400) - 1 & 0xc6) + 0x14a);
                if (g_lobbyChatLineLength < 0xff && i < maxWidth) {
                    g_lobbyChatLine[g_lobbyChatLineLength] = key;
                    g_lobbyChatLine[g_lobbyChatLineLength + 1] = 0;
                    g_lobbyChatLineLength++;
                }
            }
            break;
        case 0x1b:
            g_lobbyChatSelected = 0;
            FrontendMenu_SetInputState(0);
            Menu_SetFlags(pMenu, 1, 1, 1, 1);
            pMenu->cursor = 1;
            break;
        case 0xd:
            if (g_lobbyChatLine[0] != 0) {
                NetworkChat_SendLine((char *)g_lobbyChatLine);
                g_lobbyChatLineLength = 0;
                g_lobbyChatLine[0] = 0;
            }
            break;
        case 8:
            len = strlen((char *)g_lobbyChatLine);
            if (0 < (int)len) {
                g_lobbyChatLine[len - 1] = 0;
                g_lobbyChatLineLength--;
            }
            break;
        }
    }
    if (Network_GetSessionStateFlag() != 0) {
        i = 0;
        do {
            c = (char)NetPlayers_IsPlayerPresent(i);
            if (c != 0 && (BYTE)NetPlayers_GetPlayerFlag6(i) == 0) {
                b = 0;
                break;
            }
            i++;
        } while (i < 7);
    } else {
        if (g_unk0x00818ce4 != 0) {
            if (Menu_GetItem(pMenu, 3)->max == 1) {
                g_unk0x00818ce4 = 0;
                FrontendNetwork_SendPlayerDescription();
            }
            if (g_unk0x00818ce4 != 0)
                goto LAB_004edb02;
        }
        if (Menu_GetItem(pMenu, 3)->max == 0) {
            g_unk0x00818ce4 = 1;
            FrontendNetwork_SendPlayerDescription();
        }
    }
LAB_004edb02:
    Menu_GetItem(pMenu, 3)->enabled = b;
    FrontendNetwork_DrainMessageQueue();
}

// Network car-setup screen callback: rebuilds the item ranges and mirrors the
// edited name string back into the menu entry.
// match 65%: los strcpy/strlen del original se expanden inline; los nuestros llaman a la CRT
// FUNCTION: CMR2 0x004ed100
void FrontendMenu_UpdateNetworkSessionDetails(Menu *pMenu)
{
    int key;
    char c;
    int i;
    int n;
    unsigned int len;
    char *pc;
    char *pSrc;
    char *pDst;
    short value;

    if (Menu_GetItem(pMenu, 1)->max == 2 || Menu_GetItem(pMenu, 1)->max == 4)
        Menu_GetItem(pMenu, 2)->enabled = 0;
    else
        Menu_GetItem(pMenu, 2)->enabled = 1;
    if ((int)Network_GetSessionPlayerCount(-1) <= 6)
        Menu_GetItem(pMenu, 1)->min = 5;
    else
        Menu_GetItem(pMenu, 1)->min = 3;
    if (Menu_GetItem(pMenu, 1)->max >= Menu_GetItem(pMenu, 1)->min)
        Menu_GetItem(pMenu, 1)->max = Menu_GetItem(pMenu, 1)->min - 1;
    if (Menu_GetItem(pMenu, 1)->max >= 3)
        Menu_GetItem(pMenu, 3)->min = 5;
    else
        Menu_GetItem(pMenu, 3)->min = 7;
    if (Menu_GetItem(pMenu, 3)->max >= Menu_GetItem(pMenu, 3)->min)
        Menu_GetItem(pMenu, 3)->max = Menu_GetItem(pMenu, 3)->min - 1;
    value = pMenu->items[pMenu->cursor].value;
    if (value == 0 || value == 4) {
        if (pMenu->items[pMenu->cursor].value == 0)
            g_pSessionDetailsText = (char *)g_sessionName;
        else
            g_pSessionDetailsText = (char *)g_sessionPassword;
        strcpy(CFrontend::m_stringDest, g_pSessionDetailsText);
        Sys_RequestTextInput(CFrontend::GetTextString(value == 0 ? 0x1bf : 0x1c3), g_pSessionDetailsText, 0x13);   // PORT
        if (Input_PopQueuedCharacter(&key) != 0) {
            if (key != 8) {
                i = Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
                n = ((g_pGraphics->resX < 0x400) - 1 & 0x54) + 0x8c;
                len = strlen(CFrontend::m_stringDest);
                if ((int)len < 0x13 && i < n) {
                    pc = strchr(g_strValidLetters, (char)key);
                    if (pc != NULL) {
                        CFrontend::m_stringDest[len] = (char)key;
                        CFrontend::m_stringDest[len + 1] = 0;
                        Menu_PlaySoundId(1);
                    }
                }
            } else {
                if (CFrontend::m_stringDest[0] != 0) {
                    n = 2;
                    len = strlen(CFrontend::m_stringDest);
                    CFrontend::m_stringDest[len - 1] = 0;
                    Menu_PlaySoundId(n);
                }
            }
        }
        strcpy(g_pSessionDetailsText, CFrontend::m_stringDest);
    }
    FrontendNetwork_DrainMessageQueue();
}

void Profile_RebuildFileList(void);

// Prepares the stage-selection screen before it is shown: reloads the profile
// list and picks a random starting stage and player.
// FUNCTION: CMR2 0x004f0580
void FrontendMenu_EnterMultiplayerStageSelection(Menu *pMenu, int param)
{
    Frontend_SetOverlayMode((CGameInfo::GetConfiguredPlayerCount() & 0xff) - FrontendProfile_GetPlayersRemaining());
    Profile_RebuildFileList();
    pMenu->items[0].max = 0;
    pMenu->items[0].min = (BYTE)Profile_GetFileCount();
    pMenu->items[0].enabled = (0 < Profile_GetFileCount());
    g_unk0x00819868 = 0;
    g_unk0x00819044 = -1;
    if ((char)param == 0) {
        pMenu->cursor = 0;
        pMenu->items[0].max = 0;
        g_profilePlayersRemaining--;
    }
}

// GLOBAL: CMR2 0x00524d4c
char g_strClassRowFormat[20] = "%s  (%s, %s, %s)";

// Draws the network "class" selection screen: one row per class with its name
// and its two columns of allowed values.
// match 56%: asignacion de registros en el bucle de filas y en los sprintf de la fila 0/1
// match 67%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004db850
void FrontendMenu_DrawArcadeChampionshipTransmission(Menu *pMenu)
{
    short rect[4];
    char *pTexts[2];
    MenuItem *pItem;
    int *pColour;
    int *pLineColour;
    int *pShadow;
    int count;
    int baseY;

    rect[0] = (short)((int)(g_pGraphics->resX * 100) / 0x280);
    rect[1] = 0;
    rect[2] = CFrontend::m_pAr640ATexture->width;
    rect[3] = CFrontend::m_pAr640ATexture->height;
    FrontendDraw_PlayTime();
    pTexts[0] = CFrontend::GetTextString(0x94);
    pTexts[1] = CFrontend::GetTextString(0xc);
    FrontendDraw_MenuPath(pMenu, (int)(g_pGraphics->resX * 0x18) / 0x280,
                          (int)(g_pGraphics->resY * 0x26) / 0x1e0, 1, 3, pTexts, 2);
    FrontendDraw_HelpText(CFrontend::GetTextString(0x57), 1);
    baseY = ((int)(g_pGraphics->resY * 0x38) / 0x1e0 + (int)(g_pGraphics->resY * 0x176) / 0x1e0) / 2 -
            (((int)(g_pGraphics->resY * 0x24) / 0x1e0) * pMenu->itemCount) / 2;
    if (pMenu->cursor == 0) {
        pColour = (int *)g_colourWhite0x00524968;
        pShadow = (int *)g_colourShadowWhite0x00524974;
    } else {
        pColour = (int *)g_colourText0x0052496c;
        pShadow = (int *)g_colourShadowText0x00524978;
    }
    g_frontendDrawRect[0] = (short)((int)(g_pGraphics->resX * 99) / 0x280);
    g_frontendDrawRect[1] = (short)baseY;
    g_frontendDrawRect[2] = (short)((int)(g_pGraphics->resX * 0x11a) / 0x280);
    g_frontendDrawRect[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, (BYTE *)pShadow, 1);
    g_frontendDrawRect[1] = g_frontendDrawRect[1] + 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, (BYTE *)pColour, 1);
    count = 0;
    pItem = pMenu->items;
    if (pMenu->itemCount > 0) {
        do {
            rect[1] = (short)((int)(g_pGraphics->resY * 0x14) / 0x1e0 + baseY +
                ((int)(g_pGraphics->resY * 0x24) / 0x1e0) * count -
                CFrontend::m_pAr640ATexture->height / 2);
            if (pMenu->cursor == count) {
                pColour = (int *)g_colourWhite0x00524968;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, (SpriteRect *)rect, CFrontend::m_pAr640ATexture, 1, 0, NULL, NULL,
                             (BYTE *)pColour, 8);
            } else {
                pColour = (int *)g_colourText0x0052496c;
                if ((pItem->enabled & 1) == 0)
                    pColour = (int *)g_colourDim0x00524970;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, (SpriteRect *)rect, CFrontend::m_pAr640DTexture, 1, 0, NULL, NULL,
                             (BYTE *)pColour, 8);
            }
            switch (pItem->value) {
            case 0:
                    sprintf(CFrontend::m_stringDest, g_strClassRowFormat,
                            CFrontend::GetTextString(0xe8), CFrontend::GetTextString(0x2b),
                            CFrontend::GetTextString(0x27), CFrontend::GetTextString(0x29));
                    Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 0x280,
                                  (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], pColour, 0x11);
                break;
            case 1:
                    sprintf(CFrontend::m_stringDest, g_strClassRowFormat,
                            CFrontend::GetTextString(0xe9), CFrontend::GetTextString(0x2d),
                            CFrontend::GetTextString(0x2a), CFrontend::GetTextString(0x28));
                    Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 0x280,
                                  (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], pColour, 0x11);
                break;
            default:
                    Font_DrawText(1, CFrontend::GetTextString(pItem->id), (int)(g_pGraphics->resX * 0x7a) / 0x280,
                                  (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], pColour, 0x11);

                break;
            }
            if (pMenu->cursor == count || pMenu->cursor == count + 1) {
                pLineColour = (int *)g_colourWhite0x00524968;
                pShadow = (int *)g_colourShadowWhite0x00524974;
            } else if ((pItem->enabled & 1) == 0 && (pItem[1].enabled & 1) == 0) {
                pLineColour = (int *)g_colourDim0x00524970;
                pShadow = (int *)g_colourShadowDim0x0052497c;
            } else {
                pLineColour = (int *)g_colourText0x0052496c;
                pShadow = (int *)g_colourShadowText0x00524978;
            }
            g_frontendDrawRect[1] = (short)(((int)(g_pGraphics->resY * 0x24) / 0x1e0) * (count + 1)) + baseY;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, (BYTE *)pShadow, 1);
            g_frontendDrawRect[1] = g_frontendDrawRect[1] + 1;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, (BYTE *)pLineColour, 1);
            pItem++;
            count++;
        } while (count < pMenu->itemCount);
    }
}

extern char g_classRowHeaderFormat[];
BYTE *FrontendRecords_GetCarClassRecordText(int row, int column);

// Draws the network "car" selection screen: one row per car with its name and
// the value columns, plus the highlight bars.
// FUNCTION: CMR2 0x004e2610
void FrontendMenu_DrawSoundOptions(Menu *pMenu)
{
    short rect[4];
    MenuItem *pItem;
    int *pColour;
    int *pLineColour;
    int *pShadow;
    int count;
    int baseY;

    rect[0] = (short)((int)(g_pGraphics->resX * 100) / 0x280);
    rect[1] = 0;
    rect[2] = CFrontend::m_pAr640ATexture->width;
    rect[3] = CFrontend::m_pAr640ATexture->height;
    baseY = ((int)(g_pGraphics->resY * 8) / 0x1e0 + (int)(g_pGraphics->resY * 0x26) / 0x1e0 +
             (int)(g_pGraphics->resY * 0x180) / 0x1e0) / 2 -
            (((int)(g_pGraphics->resY * 0x24) / 0x1e0) * pMenu->itemCount) / 2;
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, (int)(g_pGraphics->resX * 0x18) / 0x280,
                          (int)(g_pGraphics->resY * 0x26) / 0x1e0, 1, 2, NULL, -1);
    if (pMenu->cursor == 0) {
        pColour = (int *)g_colourWhite0x00524968;
        pShadow = (int *)g_colourShadowWhite0x00524974;
    } else {
        pColour = (int *)g_colourText0x0052496c;
        pShadow = (int *)g_colourShadowText0x00524978;
    }
    g_frontendDrawRect[0] = (short)((int)(g_pGraphics->resX * 99) / 0x280);
    g_frontendDrawRect[1] = (short)baseY;
    g_frontendDrawRect[2] = (short)((int)(g_pGraphics->resX * 0x11a) / 0x280);
    g_frontendDrawRect[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, (BYTE *)pShadow, 1);
    g_frontendDrawRect[1] = g_frontendDrawRect[1] + 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, (BYTE *)pColour, 1);
    count = 0;
    pItem = pMenu->items;
    if (pMenu->itemCount > 0) {
        do {
            rect[1] = (short)((int)(g_pGraphics->resY * 2) / 0x1e0 + (int)(g_pGraphics->resY * 0x12) / 0x1e0 + baseY +
                ((int)(g_pGraphics->resY * 0x24) / 0x1e0) * count -
                CFrontend::m_pAr640ATexture->height / 2);
            if (pMenu->cursor == count) {
                pColour = (int *)g_colourWhite0x00524968;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, (SpriteRect *)rect,
                             CFrontend::m_pAr640ATexture, 1, 0, NULL, NULL, (BYTE *)g_colourWhite0x00524968, 8);
            } else {
                pColour = (int *)g_colourText0x0052496c;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, (SpriteRect *)rect,
                             CFrontend::m_pAr640DTexture, 1, 0, NULL, NULL, (BYTE *)g_colourText0x0052496c, 8);
            }
            switch (pItem->value) {
            case 0:
            case 1:
            case 2:
                sprintf(CFrontend::m_stringDest, g_classRowHeaderFormat,
                        CFrontend::GetTextString(pItem->id), pItem->max * 10);
                Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 0x280,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], pColour, 0x11);
                break;
            case 4:
                Font_DrawText(1, CFrontend::GetTextString(pItem->id), (int)(g_pGraphics->resX * 0x7a) / 0x280,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], pColour, 0x11);
                break;
            }
            if (pMenu->cursor == count + 1 || pMenu->cursor == count) {
                pLineColour = (int *)g_colourWhite0x00524968;
                pShadow = (int *)g_colourShadowWhite0x00524974;
            } else {
                pLineColour = (int *)g_colourText0x0052496c;
                pShadow = (int *)g_colourShadowText0x00524978;
            }
            g_frontendDrawRect[1] = (short)(((int)(g_pGraphics->resY * 0x24) / 0x1e0) * (count + 1)) + baseY;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, (BYTE *)pShadow, 1);
            g_frontendDrawRect[1] = g_frontendDrawRect[1] + 1;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, (BYTE *)pLineColour, 1);
            pItem++;
            count++;
        } while (count < pMenu->itemCount);
    }
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

// Draws the network "stage times" screen: the three mode titles, the column
// headers, the per-mode rows with the time of each stage and the help text.
// match 86%: ranuras de pila de los temporales de las cabeceras
// match 86%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004e48b0
void FrontendMenu_DrawProfileStageRecords(Menu *pMenu)
{
    BYTE *pBase;
    unsigned int *pEntry;
    unsigned int *pFlags;
    unsigned int time;
    int mode;
    int x;
    int y;
    int index;

    mode = FrontendRecords_GetTableMode();
    FrontendDraw_PlayTime();
    FrontendDraw_MenuTitle(pMenu);
    switch (mode) {
    case 0:
        Font_DrawText(2, CFrontend::GetTextString(0x167), (int)g_pGraphics->resX / 2,
                      (int)(g_pGraphics->resY * 100) / 0x1e0, (int *)g_colourWhite0x00524968, 10);
        break;
    case 1:
        Font_DrawText(2, CFrontend::GetTextString(0x168), (int)g_pGraphics->resX / 2,
                      (int)(g_pGraphics->resY * 100) / 0x1e0, (int *)g_colourWhite0x00524968, 10);
        break;
    case 2:
        Font_DrawText(2, CFrontend::GetTextString(0x169), (int)g_pGraphics->resX / 2,
                      (int)(g_pGraphics->resY * 100) / 0x1e0, (int *)g_colourWhite0x00524968, 10);
        break;
    }
    g_frontendDrawRect[0] = (short)((int)(g_pGraphics->resX * 0x1e) / 0x280);
    g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 200) / 0x1e0 -
                                 (int)(g_pGraphics->resY * 0x19) / 0x1e0);
    g_frontendDrawRect[2] = (short)((int)(g_pGraphics->resX * 0x280) / 0x280 -
                                 ((int)(g_pGraphics->resX * 0x1e) / 0x280) * 2);
    g_frontendDrawRect[3] = (short)((int)(g_pGraphics->resY * 0x19) / 0x1e0);
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourShadowText0x00524978, 4);
    y = (int)(g_pGraphics->resY * 0x13) / 0x1e0 +
        ((int)(g_pGraphics->resY * 200) / 0x1e0 - (int)(g_pGraphics->resY * 0x19) / 0x1e0);
    Font_DrawText(0, CFrontend::GetTextString(0x16b), (int)(g_pGraphics->resX * 0xb4) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16c), (int)(g_pGraphics->resX * 0x136) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16d), (int)(g_pGraphics->resX * 0x172) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x174), (int)(g_pGraphics->resX * 0x1b3) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x170), (int)(g_pGraphics->resX * 0x212) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    g_frontendDrawRect[0] = (short)((int)(g_pGraphics->resX * 0x1e) / 0x280);
    g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 200) / 0x1e0);
    g_frontendDrawRect[2] = (short)((int)(g_pGraphics->resX * 0x280) / 0x280 -
                                 ((int)(g_pGraphics->resX * 0x1e) / 0x280) * 2);

    g_frontendDrawRect[3] = 1;
    y = (int)(g_pGraphics->resY * 0x13) / 0x1e0 + (int)(g_pGraphics->resY * 200) / 0x1e0;
    if ((RallyData_GetDriverBestResults(0)->rallyResults[pMenu->cursor][mode].header & 0x80) != 0) {
        pEntry = &RallyData_GetDriverBestResults(0)->rallyResults[pMenu->cursor][mode].result;
        pFlags = &RallyData_GetDriverBestResults(0)->rallyResults[pMenu->cursor][mode].header;
        Font_DrawText(1, CFrontend::GetModeSpecificCountryText(*pFlags & 0x3f),
                      (int)(g_pGraphics->resX * 0xb4) / 0x280, y,
                      (int *)g_colourWhite0x00524968, 0x12);
        if (!((*pFlags & 0x40) == 0))
            Font_DrawText(1, g_strGearboxAuto, (int)(g_pGraphics->resX * 0x136) / 0x280, y,
                          (int *)g_colourWhite0x00524968, 0x12);
        else
            Font_DrawText(1, g_strGearboxManual, (int)(g_pGraphics->resX * 0x136) / 0x280, y,
                          (int *)g_colourWhite0x00524968, 0x12);
        sprintf(CFrontend::m_stringDest, g_strTwoDigits, (*pEntry & 0xf) + 1);
        Font_DrawText(3, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x172) / 0x280, y,
                      (int *)g_colourWhite0x00524968, 0x12);
        time = ((unsigned int *)pEntry)[1];
        sprintf(CFrontend::m_stringDest, g_loadRecordTimeFormat, time / 6000,
                (int)((time / 100) % 0x3c), time % 100);
        Font_DrawText(3, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x1b3) / 0x280, y,
                      (int *)g_colourWhite0x00524968, 0x12);
        Font_DrawText(0, (char *)FrontendRecords_GetCarClassRecordText(pMenu->cursor, mode),
                      (int)(g_pGraphics->resX * 0x212) / 0x280, y,
                      (int *)g_colourWhite0x00524968, 0x12);
    } else {
        Font_DrawText(1, CFrontend::GetTextString(0x171), (int)g_pGraphics->resX / 2, y,
                      (int *)g_colourWhite0x00524968, 0x12);
    }
    g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 0x19) / 0x1e0 +
                                 (int)(g_pGraphics->resY * 200) / 0x1e0);
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourText0x0052496c, 1);
    FrontendDraw_ScrollerRow(FrontendScroller_GetStageRecordsScroller(), 1);
    FrontendDraw_HelpText(CFrontend::GetTextString(0x16f), 1);
}

// Draws the network "best times" screen: the mode headers, the two highlight
// bars and one row per stage with its time.
// FUNCTION: CMR2 0x004e4fc0
void FrontendMenu_DrawBestStageTimes(Menu *pMenu)
{
    GameInfo0xa4SubStruct8 *pRow;
    unsigned int time;
    int count;
    int i;
    int x;
    int y;

    FrontendRecords_GetTableMode();
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, (int)(g_pGraphics->resX * 0x18) / 0x280,
                          (int)(g_pGraphics->resY * 0x26) / 0x1e0, 1, 2, NULL, -1);
    count = 10 + (pMenu->cursor % 2 != 0);
    g_frontendDrawRect[0] = (short)((int)(g_pGraphics->resX * 0x1e) / 0x280);
    g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 0x5a) / 0x1e0 -
                                 (int)(g_pGraphics->resY * 0x19) / 0x1e0);
    g_frontendDrawRect[2] = (short)((int)(g_pGraphics->resX * 0x280) / 0x280 -
                                 ((int)(g_pGraphics->resX * 0x1e) / 0x280) * 2);
    g_frontendDrawRect[3] = (short)((int)(g_pGraphics->resY * 0x19) / 0x1e0);
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourShadowText0x00524978, 4);
    y = (int)(g_pGraphics->resY * 0x13) / 0x1e0 +
        ((int)(g_pGraphics->resY * 0x5a) / 0x1e0 - (int)(g_pGraphics->resY * 0x19) / 0x1e0);
    Font_DrawText(0, CFrontend::GetTextString(0x17d), (int)(g_pGraphics->resX * 100) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16a), (int)(g_pGraphics->resX * 0x96) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16b), (int)(g_pGraphics->resX * 0x122) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16c), (int)(g_pGraphics->resX * 0x1b8) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x174), (int)(g_pGraphics->resX * 0x208) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    g_frontendDrawRect[0] = (short)((int)(g_pGraphics->resX * 0x1e) / 0x280);
    g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 0x5a) / 0x1e0);
    g_frontendDrawRect[2] = (short)((int)(g_pGraphics->resX * 0x280) / 0x280 -
                                 ((int)(g_pGraphics->resX * 0x1e) / 0x280) * 2);
    g_frontendDrawRect[3] = 1;
    for (i = 0; i < count; i++) {
        y = (int)(g_pGraphics->resY * 0x13) / 0x1e0 + (int)(g_pGraphics->resY * 0x5a) / 0x1e0 +
            ((int)(g_pGraphics->resY * 0x19) / 0x1e0) * i;
        pRow = &CGameInfo::GetGameInfoFieldA4Address()->rallyStageRecordTimes[pMenu->cursor * 0xb + i];
        sprintf(CFrontend::m_stringDest, g_strTwoDigits, i + 1);
        Font_DrawText(3, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 100) / 0x280, y,
                      (int *)g_colourText0x0052496c, 0x12);
        Font_DrawText(1, pRow->ident, (int)(g_pGraphics->resX * 0x96) / 0x280, y,
                      (int *)g_colourWhite0x00524968, 0x12);
        Font_DrawText(1, CFrontend::GetModeSpecificCountryText(pRow->bits.car),
                      (int)(g_pGraphics->resX * 0x122) / 0x280, y, (int *)g_colourWhite0x00524968, 0x12);
        if (pRow->bits.automatic != 0)
            Font_DrawText(1, g_strGearboxAuto, (int)(g_pGraphics->resX * 0x1b8) / 0x280, y,
                          (int *)g_colourWhite0x00524968, 0x12);
        else
            Font_DrawText(1, g_strGearboxManual, (int)(g_pGraphics->resX * 0x1b8) / 0x280, y,
                          (int *)g_colourWhite0x00524968, 0x12);
        time = pRow->bits.time;
        sprintf(CFrontend::m_stringDest, g_loadRecordTimeFormat, time / 6000,
                (int)((time / 100) % 0x3c), time % 100);
        Font_DrawText(3, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x208) / 0x280, y,
                      (int *)g_colourWhite0x00524968, 0x12);
        g_frontendDrawRect[1] = (short)(((int)(g_pGraphics->resY * 0x5a) / 0x1e0) +
                                     ((int)(g_pGraphics->resY * 0x19) / 0x1e0) * (i + 1));
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourText0x0052496c, 1);
    }
    FrontendDraw_ScrollerRow(FrontendScroller_GetBestStageTimesScroller(), 1);
    FrontendDraw_HelpText(CFrontend::GetTextString(0x172), 1);
}

BYTE *FrontendRecords_GetProfileCarClassRecordText(int row, int column);
MenuScroller *FrontendScroller_GetTransmissionRecordsScroller(void);

// Draws the network "records" screen: mode title, the two bars, the column
// headers and five rows with the player, car, gearbox and two values.
// FUNCTION: CMR2 0x004e5c90
void FrontendMenu_DrawTransmissionRecords(Menu *pMenu)
{
    GameInfo0xa4SubStruct12 *pRow;
    int mode;
    int i;
    int x;
    int y;

    mode = FrontendRecords_GetTableMode();
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, (int)(g_pGraphics->resX * 0x18) / 0x280,
                          (int)(g_pGraphics->resY * 0x26) / 0x1e0, 1, 2, NULL, -1);
    switch (mode) {
    case 0:
        Font_DrawText(2, CFrontend::GetTextString(0x167), (int)g_pGraphics->resX / 2,
                      (int)(g_pGraphics->resY * 0x4b) / 0x1e0, (int *)g_colourWhite0x00524968, 10);
        break;
    case 1:
        Font_DrawText(2, CFrontend::GetTextString(0x168), (int)g_pGraphics->resX / 2,
                      (int)(g_pGraphics->resY * 0x4b) / 0x1e0, (int *)g_colourWhite0x00524968, 10);
        break;
    case 2:
        Font_DrawText(2, CFrontend::GetTextString(0x169), (int)g_pGraphics->resX / 2,
                      (int)(g_pGraphics->resY * 0x4b) / 0x1e0, (int *)g_colourWhite0x00524968, 10);
        break;
    }
    g_frontendDrawRect[0] = (short)((int)(g_pGraphics->resX * 0x1e) / 0x280);
    g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 0xa0) / 0x1e0 -
                                 (int)(g_pGraphics->resY * 0x19) / 0x1e0);
    g_frontendDrawRect[2] = (short)((int)(g_pGraphics->resX * 0x280) / 0x280 -
                                 ((int)(g_pGraphics->resX * 0x1e) / 0x280) * 2);
    g_frontendDrawRect[3] = (short)((int)(g_pGraphics->resY * 0x19) / 0x1e0);
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourShadowText0x00524978, 4);
    y = (int)(g_pGraphics->resY * 0x13) / 0x1e0 +
        ((int)(g_pGraphics->resY * 0xa0) / 0x1e0 - (int)(g_pGraphics->resY * 0x19) / 0x1e0);
    Font_DrawText(0, CFrontend::GetTextString(0x16a), (int)(g_pGraphics->resX * 0x96) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16b), (int)(g_pGraphics->resX * 0x122) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16c), (int)(g_pGraphics->resX * 0x1ae) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16e), (int)(g_pGraphics->resX * 0x21c) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16d), (int)(g_pGraphics->resX * 0x1e5) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    g_frontendDrawRect[0] = (short)((int)(g_pGraphics->resX * 0x1e) / 0x280);
    g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 0xa0) / 0x1e0);
    g_frontendDrawRect[2] = (short)((int)(g_pGraphics->resX * 0x280) / 0x280 -
                                 ((int)(g_pGraphics->resX * 0x1e) / 0x280) * 2);

    g_frontendDrawRect[3] = 1;
    for (i = 0; i < 5; i++) {
        y = (int)(g_pGraphics->resY * 0x13) / 0x1e0 + (int)(g_pGraphics->resY * 0xa0) / 0x1e0 +
            ((int)(g_pGraphics->resY * 0x19) / 0x1e0) * i;
        pRow = &CGameInfo::GetGameInfoFieldA4Address()->thirdLoop[(mode + pMenu->cursor * 3) * 5 + i];
        sprintf(CFrontend::m_stringDest, g_strTwoDigits, i + 1);
        Font_DrawText(3, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 100) / 0x280, y,
                      (int *)g_colourText0x0052496c, 0x12);
        Font_DrawText(1, pRow->ident, (int)(g_pGraphics->resX * 0x96) / 0x280, y,
                      (int *)g_colourWhite0x00524968, 0x12);
        Font_DrawText(1, CFrontend::GetModeSpecificCountryText(SCORE_BITS(pRow).car),
                      (int)(g_pGraphics->resX * 0x122) / 0x280, y, (int *)g_colourWhite0x00524968, 0x12);
        if (SCORE_BITS(pRow).automatic != 0)
            Font_DrawText(1, g_strGearboxAuto, (int)(g_pGraphics->resX * 0x1ae) / 0x280, y,
                          (int *)g_colourWhite0x00524968, 0x12);
        else
            Font_DrawText(1, g_strGearboxManual, (int)(g_pGraphics->resX * 0x1ae) / 0x280, y,
                          (int *)g_colourWhite0x00524968, 0x12);
        sprintf(CFrontend::m_stringDest, g_strTwoDigits, SCORE_BITS(pRow).score);
        Font_DrawText(3, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x21c) / 0x280, y,
                      (int *)g_colourWhite0x00524968, 0x12);
        sprintf(CFrontend::m_stringDest, g_strTwoDigits, SCORE_BITS(pRow).level + 1);
        Font_DrawText(3, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x1e5) / 0x280, y,
                      (int *)g_colourWhite0x00524968, 0x12);
        g_frontendDrawRect[1] = (short)(((int)(g_pGraphics->resY * 0xa0) / 0x1e0) +
                                     (short)((int)(g_pGraphics->resY * 0x19) / 0x1e0) * (i + 1));
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourText0x0052496c, 1);
    }
    FrontendDraw_ScrollerRow(FrontendScroller_GetTransmissionRecordsScroller(), 1);
    FrontendDraw_HelpText(CFrontend::GetTextString(0x16f), 1);
}

// Draws the network "car records" screen: mode title, the two bars, the column
// headers and the selected row with its car, gearbox and two values.
// match 84%: ranuras de pila y orden de los dos bloques de barra
// match 84%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004e63d0
void FrontendMenu_DrawProfileTransmissionRecords(Menu *pMenu)
{
    unsigned int *pRow;
    unsigned int *pFlags;
    int mode;
    int x;
    int y;

    mode = FrontendRecords_GetTableMode();
    FrontendDraw_PlayTime();
    FrontendDraw_MenuTitle(pMenu);
    switch (mode) {
    case 0:
        Font_DrawText(2, CFrontend::GetTextString(0x167), (int)g_pGraphics->resX / 2,
                      (int)(g_pGraphics->resY * 100) / 0x1e0, (int *)g_colourWhite0x00524968, 10);
        break;
    case 1:
        Font_DrawText(2, CFrontend::GetTextString(0x168), (int)g_pGraphics->resX / 2,
                      (int)(g_pGraphics->resY * 100) / 0x1e0, (int *)g_colourWhite0x00524968, 10);
        break;
    case 2:
        Font_DrawText(2, CFrontend::GetTextString(0x169), (int)g_pGraphics->resX / 2,
                      (int)(g_pGraphics->resY * 100) / 0x1e0, (int *)g_colourWhite0x00524968, 10);
        break;
    }
    g_frontendDrawRect[0] = (short)((int)(g_pGraphics->resX * 0x1e) / 0x280);
    g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 200) / 0x1e0 -
                                 (int)(g_pGraphics->resY * 0x19) / 0x1e0);
    g_frontendDrawRect[2] = (short)((int)(g_pGraphics->resX * 0x280) / 0x280 -
                                 ((int)(g_pGraphics->resX * 0x1e) / 0x280) * 2);
    g_frontendDrawRect[3] = (short)((int)(g_pGraphics->resY * 0x19) / 0x1e0);
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourShadowText0x00524978, 4);
    y = (int)(g_pGraphics->resY * 0x13) / 0x1e0 +
        ((int)(g_pGraphics->resY * 200) / 0x1e0 - (int)(g_pGraphics->resY * 0x19) / 0x1e0);
    Font_DrawText(0, CFrontend::GetTextString(0x16b), (int)(g_pGraphics->resX * 0xbe) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16c), (int)(g_pGraphics->resX * 0x14a) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16e), (int)(g_pGraphics->resX * 0x1ae) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16d), (int)(g_pGraphics->resX * 0x17c) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x170), (int)(g_pGraphics->resX * 0x1fe) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    g_frontendDrawRect[0] = (short)((int)(g_pGraphics->resX * 0x1e) / 0x280);
    g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 200) / 0x1e0);
    g_frontendDrawRect[2] = (short)((int)(g_pGraphics->resX * 0x280) / 0x280 -
                                 ((int)(g_pGraphics->resX * 0x1e) / 0x280) * 2);

    g_frontendDrawRect[3] = 1;
    y = (int)(g_pGraphics->resY * 0x13) / 0x1e0 + (int)(g_pGraphics->resY * 200) / 0x1e0;
    pRow = &RallyData_GetDriverBestResults(0)->arcadeResults[pMenu->cursor][mode].result;
    pFlags = &RallyData_GetDriverBestResults(0)->arcadeResults[pMenu->cursor][mode].header;
    if ((*pFlags & 0x80) != 0) {
        Font_DrawText(1, CFrontend::GetModeSpecificCountryText(*pFlags & 0x3f),
                      (int)(g_pGraphics->resX * 0xbe) / 0x280, y, (int *)g_colourWhite0x00524968, 0x12);
        if (!((*pFlags & 0x40) == 0))
            Font_DrawText(1, g_strGearboxAuto, (int)(g_pGraphics->resX * 0x14a) / 0x280, y,
                          (int *)g_colourWhite0x00524968, 0x12);
        else
            Font_DrawText(1, g_strGearboxManual, (int)(g_pGraphics->resX * 0x14a) / 0x280, y,
                          (int *)g_colourWhite0x00524968, 0x12);
        sprintf(CFrontend::m_stringDest, g_strTwoDigits, *pRow >> 5 & 0x3f);
        Font_DrawText(3, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x1ae) / 0x280, y,
                      (int *)g_colourWhite0x00524968, 0x12);
        sprintf(CFrontend::m_stringDest, g_strTwoDigits, (*pRow & 7) + 1);
        Font_DrawText(3, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x17c) / 0x280, y,
                      (int *)g_colourWhite0x00524968, 0x12);
        Font_DrawText(0, (char *)FrontendRecords_GetProfileCarClassRecordText(pMenu->cursor, mode),
                      (int)(g_pGraphics->resX * 0x1fe) / 0x280, y,
                      (int *)g_colourWhite0x00524968, 0x12);
    } else {
        Font_DrawText(1, CFrontend::GetTextString(0x171), (int)g_pGraphics->resX / 2, y,
                      (int *)g_colourWhite0x00524968, 0x12);
    }
    g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 200) / 0x1e0 +
                                 (int)(g_pGraphics->resY * 0x19) / 0x1e0);
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourText0x0052496c, 1);
    FrontendDraw_ScrollerRow(FrontendScroller_GetTransmissionRecordsScroller(), 1);
    FrontendDraw_HelpText(CFrontend::GetTextString(0x16f), 1);
}

BYTE *FrontendRecords_GetChampionshipRecordText(int index);

// Draws the network "championship standings" screen: the column headers and
// eight rows with the player, car, gearbox and time.
// FUNCTION: CMR2 0x004e6a80
void FrontendMenu_DrawChampionshipRecords(Menu *pMenu)
{
    int order[8];
    BYTE *pRow;
    unsigned int time;
    int i;
    int x;
    int y;

    FrontendRecords_GetTableMode();
    order[0] = 6;
    order[1] = 3;
    order[2] = 1;
    order[3] = 4;
    order[4] = 0;
    order[5] = 2;
    order[6] = 5;
    order[7] = 7;
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, (int)(g_pGraphics->resX * 0x18) / 0x280,
                          (int)(g_pGraphics->resY * 0x26) / 0x1e0, 1, 2, NULL, -1);
    g_frontendDrawRect[0] = (short)((int)(g_pGraphics->resX * 0x1e) / 0x280);
    g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 0xa0) / 0x1e0 -
                                 (int)(g_pGraphics->resY * 0x19) / 0x1e0);
    g_frontendDrawRect[2] = (short)((int)(g_pGraphics->resX * 0x280) / 0x280 -
                                 ((int)(g_pGraphics->resX * 0x1e) / 0x280) * 2);
    g_frontendDrawRect[3] = (short)((int)(g_pGraphics->resY * 0x19) / 0x1e0);
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourShadowText0x00524978, 4);
    y = (int)(g_pGraphics->resY * 0x13) / 0x1e0 +
        ((int)(g_pGraphics->resY * 0xa0) / 0x1e0 - (int)(g_pGraphics->resY * 0x19) / 0x1e0);
    Font_DrawText(0, CFrontend::GetTextString(0x173), (int)(g_pGraphics->resX * 0x82) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16a), (int)(g_pGraphics->resX * 0xd2) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16b), (int)(g_pGraphics->resX * 0x159) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16c), (int)(g_pGraphics->resX * 0x1d6) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x174), (int)(g_pGraphics->resX * 0x20d) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    g_frontendDrawRect[0] = (short)((int)(g_pGraphics->resX * 0x1e) / 0x280);
    g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 0xa0) / 0x1e0);
    g_frontendDrawRect[2] = (short)((int)(g_pGraphics->resX * 0x280) / 0x280 -
                                 ((int)(g_pGraphics->resX * 0x1e) / 0x280) * 2);
    g_frontendDrawRect[3] = 1;
    for (i = 0; i < 8; i++) {
        y = (int)(g_pGraphics->resY * 0x13) / 0x1e0 + (int)(g_pGraphics->resY * 0xa0) / 0x1e0 +
            ((int)(g_pGraphics->resY * 0x19) / 0x1e0) * i;
        // arcadeRecordTimes[(i / 3) * 3 + i % 3] (ident, then car, automatic and time
        // bits). Typed like the stage list, it shifts registers in
        // FrontendMenu_DrawRallyReport (98.51% -> 98.38%).
        pRow = (BYTE *)CGameInfo::GetGameInfoFieldA4Address() + 0x1210 + ((i / 3) * 3 + i % 3) * 8;
        Font_DrawText(1, CFrontend::GetTextString(order[i] + 0x27), (int)(g_pGraphics->resX * 0x82) / 0x280,
                      y, (int *)g_colourText0x0052496c, 0x12);
        Font_DrawText(1, (char *)pRow, (int)(g_pGraphics->resX * 0xd2) / 0x280, y,
                      (int *)g_colourWhite0x00524968, 0x12);
        Font_DrawText(1, CFrontend::GetModeSpecificCountryText(*(unsigned int *)(pRow + 4) & 0x3f),
                      (int)(g_pGraphics->resX * 0x159) / 0x280, y, (int *)g_colourWhite0x00524968, 0x12);
        if ((*(unsigned int *)(pRow + 4) & 0x40) != 0)
            Font_DrawText(1, g_strGearboxAuto, (int)(g_pGraphics->resX * 0x1d6) / 0x280, y,
                          (int *)g_colourWhite0x00524968, 0x12);
        else
            Font_DrawText(1, g_strGearboxManual, (int)(g_pGraphics->resX * 0x1d6) / 0x280, y,
                          (int *)g_colourWhite0x00524968, 0x12);
        time = *(unsigned int *)(pRow + 4) >> 7 & 0xffff;
        sprintf(CFrontend::m_stringDest, g_loadRecordTimeFormat, time / 6000,
                (int)((time / 100) % 0x3c), time % 100);
        Font_DrawText(3, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x20d) / 0x280, y,
                      (int *)g_colourWhite0x00524968, 0x12);
        g_frontendDrawRect[1] = (short)(((int)(g_pGraphics->resY * 0xa0) / 0x1e0) +
                                     ((int)(g_pGraphics->resY * 0x19) / 0x1e0) * (i + 1));
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourText0x0052496c, 1);
    }
    FrontendDraw_HelpText(CFrontend::GetTextString(0x172), 1);
}

// Draws the network "championship" screen: the column headers and eight rows
// with the player, car, gearbox and time.
// match 85%: ranuras de pila del marco (sub esp) y de los temporales
// match 84%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004e7120
void FrontendMenu_DrawProfileChampionshipRecords(Menu *pMenu)
{
    int order[8] = {6, 3, 1, 4, 0, 2, 5, 7};
    CategoryBestResults *pBase;
    unsigned int *pFlags;
    unsigned int time;
    int i;
    int n;
    int x;
    int y;

    FrontendRecords_GetTableMode();
    FrontendDraw_PlayTime();
    FrontendDraw_MenuTitle(pMenu);
    g_frontendDrawRect[0] = (short)((int)(g_pGraphics->resX * 0x1e) / 0x280);
    g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 0xa0) / 0x1e0 -
                                 (int)(g_pGraphics->resY * 0x19) / 0x1e0);
    g_frontendDrawRect[2] = (short)((int)(g_pGraphics->resX * 0x280) / 0x280 -
                                 ((int)(g_pGraphics->resX * 0x1e) / 0x280) * 2);
    g_frontendDrawRect[3] = (short)((int)(g_pGraphics->resY * 0x19) / 0x1e0);
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourShadowText0x00524978, 4);
    y = (int)(g_pGraphics->resY * 0x13) / 0x1e0 +
        ((int)(g_pGraphics->resY * 0xa0) / 0x1e0 - (int)(g_pGraphics->resY * 0x19) / 0x1e0);
    Font_DrawText(0, CFrontend::GetTextString(0x173), (int)(g_pGraphics->resX * 100) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16b), (int)(g_pGraphics->resX * 0x109) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x16c), (int)(g_pGraphics->resX * 0x181) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x174), (int)(g_pGraphics->resX * 0x1b8) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    Font_DrawText(0, CFrontend::GetTextString(0x170), (int)(g_pGraphics->resX * 0x21c) / 0x280, y,
                  (int *)g_colourTitle0x00524984, 0x12);
    g_frontendDrawRect[0] = (short)((int)(g_pGraphics->resX * 0x1e) / 0x280);
    g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 0xa0) / 0x1e0);
    g_frontendDrawRect[2] = (short)((int)(g_pGraphics->resX * 0x280) / 0x280 -
                                 ((int)(g_pGraphics->resX * 0x1e) / 0x280) * 2);

    g_frontendDrawRect[3] = 1;
    for (i = 0; i < 8; i++) {
        y = (int)(g_pGraphics->resY * 0x13) / 0x1e0 + (int)(g_pGraphics->resY * 0xa0) / 0x1e0 +
            ((int)(g_pGraphics->resY * 0x19) / 0x1e0) * i;
        n = (i / 3) * 3 + i % 3; // [group][stage] as one index
        pBase = RallyData_GetDriverBestResults(0);
        pFlags = &RallyData_GetDriverBestResults(0)->arcadeStageTimes[0][n].header;
        Font_DrawText(1, CFrontend::GetTextString(order[i] + 0x27), (int)(g_pGraphics->resX * 100) / 0x280,
                      y, (int *)g_colourText0x0052496c, 0x12);
        if ((*pFlags & 0x80) != 0) {
            Font_DrawText(1, CFrontend::GetModeSpecificCountryText(*pFlags & 0x3f),
                          (int)(g_pGraphics->resX * 0x109) / 0x280, y, (int *)g_colourWhite0x00524968, 0x12);
            if (!((*pFlags & 0x40) == 0))
                Font_DrawText(1, g_strGearboxAuto, (int)(g_pGraphics->resX * 0x181) / 0x280, y,
                              (int *)g_colourWhite0x00524968, 0x12);
            else
                Font_DrawText(1, g_strGearboxManual, (int)(g_pGraphics->resX * 0x181) / 0x280, y,
                              (int *)g_colourWhite0x00524968, 0x12);
            time = pBase->arcadeStageTimes[0][n].time;
            sprintf(CFrontend::m_stringDest, g_loadRecordTimeFormat, time / 6000,
                    (int)((time / 100) % 0x3c), time % 100);
            Font_DrawText(3, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x1b8) / 0x280, y,
                          (int *)g_colourWhite0x00524968, 0x12);
            Font_DrawText(0, (char *)FrontendRecords_GetChampionshipRecordText(i), (int)(g_pGraphics->resX * 0x21c) / 0x280, y,
                          (int *)g_colourWhite0x00524968, 0x12);
        } else {
            Font_DrawText(1, CFrontend::GetTextString(0x171), (int)g_pGraphics->resX / 2, y,
                          (int *)g_colourWhite0x00524968, 0x12);
        }
        g_frontendDrawRect[1] = (short)(((int)(g_pGraphics->resY * 0xa0) / 0x1e0) +
                                     ((int)(g_pGraphics->resY * 0x19) / 0x1e0) * (i + 1));
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, g_colourText0x0052496c, 1);
    }
    FrontendDraw_HelpText(CFrontend::GetTextString(0x172), 1);
}

// Draws the "game setup" screen: the breadcrumb title, one row per item with
// its icon and its label (text, stage list, gearbox or plain text).
// FUNCTION: CMR2 0x004e7ed0
void FrontendMenu_DrawNetworkCarSetup(Menu *pMenu)
{
    short rect[4];
    char *pTexts[3];
    MenuItem *pItem;
    Texture *pTexture;
    int *pColour;
    int *pLineColour;
    int *pShadow;
    int count;
    int baseY;
    int x;

    rect[0] = (short)((int)(g_pGraphics->resX * 100) / 0x280);
    rect[1] = 0;
    rect[2] = CFrontend::m_pAr640ATexture->width;
    rect[3] = CFrontend::m_pAr640ATexture->height;
    pTexts[0] = CFrontend::GetTextString(0x12);
    pTexts[1] = CFrontend::GetTextString(0x1d9);
    pTexts[2] = CFrontend::GetTextString(0x1db);
    FrontendDraw_Breadcrumb((int)(g_pGraphics->resX * 0x18) / 0x280,
                            (int)(g_pGraphics->resY * 0x26) / 0x1e0, pTexts, 3);
    baseY = ((int)(g_pGraphics->resY * 8) / 0x1e0 + (int)(g_pGraphics->resY * 0x26) / 0x1e0 +
             (int)(g_pGraphics->resY * 0x180) / 0x1e0) / 2 -
            (((int)(g_pGraphics->resY * 0x24) / 0x1e0) * pMenu->itemCount) / 2;
    FrontendDraw_PlayTime();
    if (pMenu->cursor == 0) {
        pColour = (int *)g_colourWhite0x00524968;
        pShadow = (int *)g_colourShadowWhite0x00524974;
    } else {
        pColour = (int *)g_colourText0x0052496c;
        pShadow = (int *)g_colourShadowText0x00524978;
    }
    g_frontendDrawRect[0] = (short)((int)(g_pGraphics->resX * 99) / 0x280);
    g_frontendDrawRect[1] = (short)baseY;
    g_frontendDrawRect[2] = (short)((int)(g_pGraphics->resX * 0x11a) / 0x280);
    g_frontendDrawRect[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, (BYTE *)pShadow, 1);
    g_frontendDrawRect[1] = g_frontendDrawRect[1] + 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, (BYTE *)pColour, 1);
    count = 0;
    pItem = pMenu->items;
    if (pMenu->itemCount > 0) {
        do {
            x = (int)(g_pGraphics->resX * 0x7a) / 0x280;
            rect[1] = (short)((int)(g_pGraphics->resY * 2) / 0x1e0 + (int)(g_pGraphics->resY * 0x12) / 0x1e0 + baseY +
                ((int)(g_pGraphics->resY * 0x24) / 0x1e0) * count -
                CFrontend::m_pAr640ATexture->height / 2);
            if (pMenu->cursor == count) {
                pColour = (int *)g_colourWhite0x00524968;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, (SpriteRect *)rect,
                             CFrontend::m_pAr640ATexture, 1, 0, NULL, NULL, (BYTE *)g_colourWhite0x00524968, 8);
            } else {
                pColour = (int *)g_colourText0x0052496c;
                if ((pItem->enabled & 1) == 0)
                    pColour = (int *)g_colourDim0x00524970;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, (SpriteRect *)rect,
                             CFrontend::m_pAr640DTexture, 1, 0, NULL, NULL, (BYTE *)pColour, 8);
            }
            switch (pItem->value) {
            case 1:
                sprintf(CFrontend::m_stringDest, g_strLabelText, CFrontend::GetTextString(0x31),
                        &g_unk0x00818d84[Menu_GetItem(pMenu, 1)->max * 3]);
                Font_DrawText(1, CFrontend::m_stringDest, x,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], pColour, 0x11);
                break;
            case 0:
                sprintf(CFrontend::m_stringDest, g_strLabelText, CFrontend::GetTextString(0x30),
                        CFrontend::GetTextString(Menu_GetItem(pMenu, 0)->max + 0x27));
                Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 0x280,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], pColour, 0x11);
                break;
            case 2:
                if ((Menu_GetItem(pMenu, 2)->enabled & 1) != 0)
                    sprintf(CFrontend::m_stringDest, g_strLabelText, CFrontend::GetTextString(0x1dc),
                            CFrontend::GetTextString(g_unk0x00818d08[Menu_GetItem(pMenu, 2)->max]));
                else
                    sprintf(CFrontend::m_stringDest, g_strLabelText, CFrontend::GetTextString(0x1dc),
                            CFrontend::GetTextString(0x156));
                Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 0x280,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], pColour, 0x11);
                break;
            case 3:
            case 4:
                Font_DrawText(1, CFrontend::GetTextString(pItem->id), (int)(g_pGraphics->resX * 0x7a) / 0x280,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], pColour, 0x11);
                break;
            }
            if (pMenu->cursor == count + 1 || pMenu->cursor == count) {
                pLineColour = (int *)g_colourWhite0x00524968;
                pShadow = (int *)g_colourShadowWhite0x00524974;
            } else {
                pLineColour = (int *)g_colourText0x0052496c;
                pShadow = (int *)g_colourShadowText0x00524978;
            }
            g_frontendDrawRect[1] = (short)(((int)(g_pGraphics->resY * 0x24) / 0x1e0) * (count + 1)) + baseY;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, (BYTE *)pShadow, 1);
            g_frontendDrawRect[1] = g_frontendDrawRect[1] + 1;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, (BYTE *)pLineColour, 1);
            pItem++;
            count++;
        } while (count < pMenu->itemCount);
    }
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

// FUNCTION: CMR2 0x004f0820
void FrontendProfile_ApplyStageSelectionAndAdvance(Menu *pMenu, int param)
{
    Menu *pScreen;
    unsigned int index;
    BYTE result;

    if (FrontendRecords_GetProfileMode()) {
        FrontendProfile_SelectPlayerSlot(pMenu, param);
        return;
    }
    g_profilePlayersRemaining++;
    switch (CGameInfo::GetConfiguredGameMode()) {
    case 4:
        pScreen = FrontendMenu_GetPaletteConfirmation();
        break;
    case 5:
        pScreen = FrontendMenu_GetChampionshipRouteSelection();
        break;
    case 6:
        pScreen = FrontendMenu_GetAlternateChampionshipRouteSelection();
        break;
    case 7:
        pScreen = FrontendMenu_GetArcadeChampionshipRouteSelection();
        break;
    default:
        pScreen = FrontendMenu_GetCarSetup();
    }
    index = pMenu->items[0].max;
    if ((int)index < Profile_GetFileCount() && !FrontendProfile_IsAlreadyInChampionship(index)) {
        result = (BYTE)Profile_LoadAndLinkSavedRecord((CGameInfo::GetConfiguredPlayerCount() & 0xff) - (g_profilePlayersRemaining & 0xff), index);
        RallyData_SetPlayerProfileInUse(CGameInfo::GetConfiguredPlayerCount() - g_profilePlayersRemaining, 0);
        if (result != 0) {
            if (CGameInfo::GetGameModeOptionBit19() != 0) {
                Menu_SetNextAction(FrontendMenu_GetNetworkConnection());
                g_profilePlayersRemaining--;
                return;
            }
            Menu_SetParent(pScreen, FrontendMenu_GetMultiplayerProfile());
            Menu_SetNextAction(pScreen);
            g_profilePlayersRemaining--;
            return;
        }
    } else {
        Menu_PlaySoundId(3);
    }
    g_profilePlayersRemaining--;
}

// ---------------------------------------------------------------------------
// Rally and championship screens (cascade pass). Appended at the end of the
// file so the file:line of the functions above does not move.

void FrontendMenu_DrawRallyReport(Menu *pMenu);
void FrontendRecords_BuildScrambledBestTimeTables(void);

// GLOBAL: CMR2 0x00524c78
char g_strDate[16] = "%.2d.%.2d.%.4d";

// Draws the options screen: the breadcrumb, the play time, the three setting
// rows and the date of the loaded rally data.
// FUNCTION: CMR2 0x004d4cf0
void FrontendMenu_DrawProfileSettings(Menu *pMenu)
{
    char *text[4];
    char *pRecord;
    unsigned int date;
    int y;

    y = (int)(g_pGraphics->resY * 0x78) / 0x1e0;
    text[0] = CFrontend::GetTextString(0xb);
    text[1] = CFrontend::GetTextString(0x59);
    text[2] = (char *)RallyData_GetRecord(0);
    text[3] = CFrontend::GetTextString(pMenu->field_0x4);
    FrontendDraw_Breadcrumb(PATH_X(), PATH_Y(), text, 4);
    pRecord = (char *)RallyData_GetDriverCategoryProfile(0);
    FrontendDraw_PlayTime();
    Font_DrawText(1, CFrontend::GetTextString(0x7e), (int)(g_pGraphics->resX * 0x32) / 0x280, y,
                  (int *)g_colourText0x0052496c, 9);
    Font_DrawText(1, pRecord + 0x10, (int)(g_pGraphics->resX * 0x3c) / 0x280, y + 0x14,
                  (int *)g_colourWhite0x00524968, 9);
    y = y + 0x1e + (int)(g_pGraphics->resY * 0x24) / 0x1e0;
    Font_DrawText(1, CFrontend::GetTextString(0x7f), (int)(g_pGraphics->resX * 0x32) / 0x280, y,
                  (int *)g_colourText0x0052496c, 9);
    Font_DrawText(1, pRecord + 0x1c, (int)(g_pGraphics->resX * 0x3c) / 0x280, y + 0x14,
                  (int *)g_colourWhite0x00524968, 9);
    y = y + 0x1e + (int)(g_pGraphics->resY * 0x24) / 0x1e0;
    Font_DrawText(1, CFrontend::GetTextString(0x80), (int)(g_pGraphics->resX * 0x32) / 0x280, y,
                  (int *)g_colourText0x0052496c, 9);
    date = *(unsigned int *)(pRecord + 0x14);
    sprintf(CFrontend::m_stringDest, g_strDate, date >> 0x10 & 0x1f, date >> 8 & 0xf, (date & 0xff) + 0x73a);
    Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x3c) / 0x280, y + 0x14,
                  (int *)g_colourWhite0x00524968, 9);
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

// Draws the header of the rally sub-menu and the car of the selected rally.
// FUNCTION: CMR2 0x004dc710
void FrontendMenu_DrawArcadeRallySelection(Menu *pMenu)
{
    char *text[2];

    text[0] = CFrontend::GetTextString(0x94);
    text[1] = CFrontend::GetTextString(0xe2);
    FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, 3, text, 2);
    FrontendDraw_PlayTime();
    FrontendDraw_ScrollerRow(FrontendScroller_GetArcadeRallyScroller(), 1);
    FrontendMenu_DrawRallyReport(pMenu);
}

// Clears the selected carousel entry and refreshes the owning screen when the
// game is in the frontend.
// FUNCTION: CMR2 0x004f3a70
void FrontendRecords_ResetCarouselSelection(Menu *pMenu, int param)
{
    g_recordsTableMode = 0;
    if (FrontendRecords_GetProfileMode())
        FrontendRecords_BuildScrambledBestTimeTables();
}

// Character tables of the name entry keyboard: every key it accepts and the
// uppercase letters, used to fold the pressed key to lower case.
// GLOBAL: CMR2 0x005253a0
char g_strKeyChars0x005253a0[56] = "abcdefghijklmnopqrstuvwxyz. ABCDEFGHIJKLMNOPQRSTUVWXYZ";
// GLOBAL: CMR2 0x005253d8
char g_strUpperChars0x005253d8[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

// Renders the name entry screen: it keeps the typed name in m_stringDest and
// follows the key read by the input helper, moving the cursor to the row of
// the typed character.
// match 83%: el original calcula el rectangulo en otro orden y reutiliza
// registros en el bloque de la tecla; misma logica
// FUNCTION: CMR2 0x004f0e80
void FrontendMenu_UpdateProfileNameEntry(Menu *pMenu)
{
    char c;
    int key;
    int len;

    strcpy(CFrontend::m_stringDest, (char *)RallyData_GetRecord(FrontendProfile_GetCurrentPlayer()));
    if (Input_PopQueuedCharacter(&key)) {
        if (key != 8) {
            len = strlen(CFrontend::m_stringDest);
            if (len < 3 && strchr(g_strKeyChars0x005253a0, (char)key) != NULL) {
                if (strchr(g_strUpperChars0x005253d8, (char)key) != NULL)
                    key += 0x20;
                c = (char)key;
                CFrontend::m_stringDest[len] = c;
                CFrontend::m_stringDest[len + 1] = 0;
                if ('a' <= c && c <= 'j') {
                    pMenu->cursor = 0;
                    pMenu->items[0].max = c - 0x61;
                }
                if ('k' <= c && c <= 't') {
                    pMenu->cursor = 1;
                    pMenu->items[1].max = c - 0x6b;
                }
                if ('u' <= c && c <= 'z') {
                    pMenu->cursor = 2;
                    pMenu->items[2].max = c - 0x75;
                }
                if (c == '.') {
                    pMenu->cursor = 2;
                    pMenu->items[2].max = 7;
                } else if (c == ' ') {
                    pMenu->cursor = 2;
                    pMenu->items[2].max = 8;
                }
                g_unk0x00819040 = CMain::GetFrameDelta();
                g_unk0x0081986c = 1;
                Menu_PlaySoundId(1);
            }
        } else {
            if (CFrontend::m_stringDest[0] != 0) {
                CFrontend::m_stringDest[strlen(CFrontend::m_stringDest) - 1] = 0;
                Menu_PlaySoundId(2);
            }
            pMenu->cursor = 2;
            pMenu->items[2].max = 8;
            g_unk0x00819040 = CMain::GetFrameDelta();
            g_unk0x0081986c = 1;
        }
    } else if (g_unk0x0081986c != 0 && CMain::GetFrameDelta() - g_unk0x00819040 > 10) {
        g_unk0x00819040 = -1;
        pMenu->cursor = 2;
        pMenu->items[2].max = 9;
        g_unk0x0081986c = 0;
        Menu_PlaySoundId(0);
    }
    RallyData_SetEditedDriverOrCategoryName(FrontendProfile_GetCurrentPlayer(), CFrontend::m_stringDest);
}

// Draws the car setup screen: one banner per item with the value of the
// selected column, plus the highlight bars.
// match 62%: asignacion de registros y ranuras de pila del bucle de filas
// FUNCTION: CMR2 0x004d7750
void FrontendMenu_DrawProfileDateEntry(Menu *pMenu)
{
    short rect[4];
    char *text[2];
    MenuItem *pItem;
    int *pColour;
    int *pLineColour;
    int *pShadow;
    int count;
    int baseY;
    int x;

    rect[0] = (short)((int)(g_pGraphics->resX * 100) / 0x280);
    rect[1] = 0;
    rect[2] = CFrontend::m_pAr640ATexture->width;
    rect[3] = CFrontend::m_pAr640ATexture->height;
    text[0] = g_profileHeaderText;
    text[1] = CFrontend::GetTextString(0x17b);
    FrontendDraw_MenuPath(pMenu, (int)(g_pGraphics->resX * 0x18) / 0x280,
                          (int)(g_pGraphics->resY * 0x26) / 0x1e0, 1, 3, text, 2);
    FrontendDraw_PlayTime();
    baseY = ((int)(g_pGraphics->resY * 0x38) / 0x1e0 + (int)(g_pGraphics->resY * 0x176) / 0x1e0) / 2 -
            (((int)(g_pGraphics->resY * 0x24) / 0x1e0) * pMenu->itemCount) / 2;
    if (pMenu->cursor == 0) {
        pColour = (int *)g_colourWhite0x00524968;
        pShadow = (int *)g_colourShadowWhite0x00524974;
    } else {
        pColour = (int *)g_colourText0x0052496c;
        pShadow = (int *)g_colourShadowText0x00524978;
    }
    g_frontendDrawRect[0] = (short)((int)(g_pGraphics->resX * 99) / 0x280);
    g_frontendDrawRect[1] = (short)baseY;
    g_frontendDrawRect[2] = (short)((int)(g_pGraphics->resX * 0x11a) / 0x280);
    g_frontendDrawRect[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, (BYTE *)pShadow, 1);
    g_frontendDrawRect[1] = g_frontendDrawRect[1] + 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, (BYTE *)pColour, 1);
    pItem = pMenu->items;
    for (count = 0; count < pMenu->itemCount; count++, pItem++) {
        rect[1] = (short)((int)(g_pGraphics->resY * 0x14) / 0x1e0 + baseY +
                          (int)(g_pGraphics->resY * 0x24) / 0x1e0 * count -
                          CFrontend::m_pAr640ATexture->height / 2);
        if (pMenu->cursor == count) {
            pColour = (int *)g_colourWhite0x00524968;
            Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, (SpriteRect *)rect,
                         CFrontend::m_pAr640ATexture, 1, 0, NULL, NULL, (BYTE *)pColour, 8);
        } else {
            pColour = (int *)g_colourText0x0052496c;
            Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, (SpriteRect *)rect,
                         CFrontend::m_pAr640DTexture, 1, 0, NULL, NULL, (BYTE *)pColour, 8);
        }
        switch (pItem->value) {
        case 0:
            sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0xec), pMenu->items[0].max + 0x73a);
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 0x280,
                          (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], pColour, 0x11);
            break;
        case 1:
            sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0xeb),
                    CFrontend::GetTextString(pMenu->items[1].max + 0xed));
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 0x280,
                          (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], pColour, 0x11);
            break;
        case 2:
            sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0xea), pMenu->items[2].max + 1);
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 0x280,
                          (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], pColour, 0x11);
            break;
        default:
            x = (int)(g_pGraphics->resX * 0x7a) / 0x280;
            Font_DrawText(1, CFrontend::GetTextString(pItem->id), x,
                          (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], pColour, 0x11);
            break;
        }
        if (pMenu->cursor == count || pMenu->cursor == count + 1) {
            pLineColour = (int *)g_colourWhite0x00524968;
            pShadow = (int *)g_colourShadowWhite0x00524974;
        } else {
            pLineColour = (int *)g_colourText0x0052496c;
            pShadow = (int *)g_colourShadowText0x00524978;
        }
        g_frontendDrawRect[1] = (short)(((int)(g_pGraphics->resY * 0x24) / 0x1e0) * (count + 1) + baseY);
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, (BYTE *)pShadow, 1);
        g_frontendDrawRect[1] = g_frontendDrawRect[1] + 1;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, (BYTE *)pLineColour, 1);
    }
    FrontendDraw_HelpText(CFrontend::GetTextString(0x57), 1);
}

// Two string views the name entry screen puts in its menu path: the record
// name and the "enter name" prompt.
// They must be contiguous: the pair is passed to FrontendDraw_MenuPath as a
// two-entry name array.
// GLOBAL: CMR2 0x0081854c
char *g_nameEntryPath0x0081854c[2];

// The 30 keys of the name entry keyboard, three rows of ten.
// GLOBAL: CMR2 0x005249a8
char g_strNameRow0x005249a8[12] = "abcdefghij";
// GLOBAL: CMR2 0x005249b4
char g_strNameRow0x005249b4[12] = "klmnopqrst";
// GLOBAL: CMR2 0x005249c0
char g_strNameRow0x005249c0[12] = "uvwxyz. <_";
// GLOBAL: CMR2 0x00524d08
char g_strCharFormat0x00524d08[3] = "%c";

// Draws the name entry keyboard: the three rows of keys with the selected one
// highlighted and the name typed so far centred under them.
// FUNCTION: CMR2 0x004d6f10
void FrontendMenu_DrawProfileNameEntry(Menu *pMenu)
{
    char name[4];
    char text[2];
    char *pText;
    int *pColour;
    unsigned char font;
    int len;
    int row;
    int col;
    int i;

    g_nameEntryPath0x0081854c[0] = g_profileHeaderText;
    if (g_profileNameEntryFlag != 0) {
        if (RallyData_IsDriverRecordUsable((BYTE)FrontendProfile_GetCurrentPlayer()))
            g_nameEntryPath0x0081854c[1] = CFrontend::GetTextString(0xe5);
        else
            g_nameEntryPath0x0081854c[1] = CFrontend::GetTextString(0x17b);
    }
    FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, 3, g_nameEntryPath0x0081854c, 2);
    FrontendDraw_PlayTime();
    for (row = 0; row < 3; row++) {
        for (col = 0; col < 10; col++) {
            font = 2;
            switch (row) {
            case 0:
                sprintf(CFrontend::m_stringDest, g_strCharFormat0x00524d08, g_strNameRow0x005249a8[col]);
                break;
            case 1:
                sprintf(CFrontend::m_stringDest, g_strCharFormat0x00524d08, g_strNameRow0x005249b4[col]);
                break;
            case 2:
                if (g_strNameRow0x005249c0[col] == '<') {
                    font = 1;
                    strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(0x13f));
                } else if (g_strNameRow0x005249c0[col] == '_') {
                    font = 1;
                    strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(0x140));
                } else {
                    sprintf(CFrontend::m_stringDest, g_strCharFormat0x00524d08, g_strNameRow0x005249c0[col]);
                }
                break;
            }
            if (col == pMenu->items[pMenu->cursor].max && pMenu->items[pMenu->cursor].value == row)
                pColour = (int *)g_colourWhite0x00524968;
            else
                pColour = (int *)g_colourText0x0052496c;
            if (g_strNameRow0x005249c0[col] == ' ' && row == 2) {
                g_frontendDrawRect[0] = (short)(((col + 2) * g_pGraphics->resX) / 0xe);
                g_frontendDrawRect[1] = (short)(((int)(g_pGraphics->resY * 6)) / 8) -
                                     (short)(Font_GetTextHeight(font, CMain::m_logFileBlankLine) / 2);
                g_frontendDrawRect[2] = (short)(g_pGraphics->resX / 0xe);
                g_frontendDrawRect[3] = (short)(Font_GetTextHeight(font, CMain::m_logFileBlankLine) / 2);
                Frontend_DrawRectangleOutline(g_frontendDrawRect, (BYTE *)pColour);
            }
            Font_DrawText(font, CFrontend::m_stringDest,
                          g_pGraphics->resX / 0x1c + ((col + 2) * g_pGraphics->resX) / 0xe,
                          (g_pGraphics->resY * (row + 4)) / 8, pColour, 0x12);
        }
    }
    if (g_profileNameEntryFlag != 0) {
        strcpy(CFrontend::m_stringDest, (char *)RallyData_GetRecord(FrontendProfile_GetCurrentPlayer()));
        strcpy(name, (char *)RallyData_GetRecord(FrontendProfile_GetCurrentPlayer()));
    } else {
        sprintf(CFrontend::m_stringDest, CMain::m_logFileBlankLine);
        sprintf(name, CMain::m_logFileBlankLine);
    }
    while (strlen(CFrontend::m_stringDest) < 3) {
        CFrontend::m_stringDest[strlen(CFrontend::m_stringDest) + 1] = 0;
        CFrontend::m_stringDest[strlen(CFrontend::m_stringDest)] = (char)(rand() % 0x1a) + 'a';
    }
    for (i = 0; i < 3; i++) {
        text[0] = CFrontend::m_stringDest[i];
        text[1] = 0;
        len = (int)strlen(name);
        if (i < len)
            pColour = (int *)g_colourWhite0x00524968;
        else
            pColour = (int *)g_colourText0x0052496c;
        Font_DrawText(2, text, ((i - 1) * g_pGraphics->resX) / 0xe + g_pGraphics->resX / 2,
                      (int)(g_pGraphics->resY * 2) / 8, pColour, 0x12);
    }
    FrontendDraw_HelpText(CFrontend::GetTextString(0x57), 1);
    if (CGameInfo::GetGameModeOptionBit19() != 0)
        FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, 0);
}

// Scratch buffer of the rally screens: holds the formatted rally/stage name.
// GLOBAL: CMR2 0x008188a4
char g_str0x008188a4[0x100];

// Formats of the two info lines of the rally info screen (event and date).
// GLOBAL: CMR2 0x00524ce4
char g_str0x00524ce4[24] = "%s\n%s\n%.2d.%.2d.%.4d";
// GLOBAL: CMR2 0x00524cfc
char g_str0x00524cfc[12] = "%s:\n%s:\n%s:";

// Draws the rally info screen: the breadcrumb with the event, the list of
// stages of the rally and the summary lines of the selected stage.
// match 74%: reparto de registros en el switch del breadcrumb y en las dos
// lineas de sprintf
// FUNCTION: CMR2 0x004d6a60
void FrontendMenu_DrawMultiplayerStageSelection(Menu *pMenu)
{
    char *text[4];
    BYTE *pRecord;
    unsigned int flags;
    int value;
    int count;
    int i;

    FrontendDraw_PlayTime();
    if (g_recordsProfileMode != 0) {
        sprintf(g_str0x008188a4, CFrontend::GetTextString(0xb));
        text[0] = g_str0x008188a4;
        FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, 2, text, 1);
    } else {
        sprintf(g_str0x008188a4, CFrontend::GetTextString(0xdc), g_profileHeaderPlayerNumber);
        text[2] = g_str0x008188a4;
        text[3] = CFrontend::GetTextString(0x17a);
        if (CGameInfo::GetGameModeOptionBit19() != 0) {
            text[0] = CFrontend::GetTextString(0x12);
            text[1] = 0;
            text[2] = 0;
        } else {
            switch (CGameInfo::GetConfiguredGameMode()) {
            case 0:
                text[0] = CFrontend::GetTextString(0xe7);
                text[1] = CFrontend::GetTextString(0xc);
                break;
            case 1:
                text[0] = CFrontend::GetTextString(0xe7);
                text[1] = CFrontend::GetTextString(0xd);
                break;
            case 2:
                text[0] = CFrontend::GetTextString(0xe7);
                text[1] = CFrontend::GetTextString(0xf);
                break;
            case 3:
                text[0] = CFrontend::GetTextString(0xe7);
                text[1] = CFrontend::GetTextString(0x10);
                break;
            case 4:
                text[0] = CFrontend::GetTextString(0xe7);
                text[1] = CFrontend::GetTextString(0xe);
                break;
            case 5:
                text[0] = CFrontend::GetTextString(0x94);
                text[1] = CFrontend::GetTextString(0xc);
                break;
            case 6:
                text[0] = CFrontend::GetTextString(0x94);
                text[1] = CFrontend::GetTextString(0xe2);
                break;
            case 7:
                text[0] = CFrontend::GetTextString(0x94);
                text[1] = CFrontend::GetTextString(0x10);
                break;
            }
        }
        FrontendDraw_Breadcrumb(PATH_X(), PATH_Y(), text, 4);
    }
    count = Profile_GetFileCount();
    if (count > 0) {
        // the two counts are kept in the original but the result is discarded
        value = pMenu->items[0].max;
        FrontendDraw_ChampionshipEntryRow(value, 0);
        i = 1;
        while (FrontendDraw_ChampionshipEntryRow(value, i))
            i++;
        i = -1;
        while (FrontendDraw_ChampionshipEntryRow(value, i))
            i--;
    } else {
        Font_DrawText(2, CFrontend::GetTextString(0x1be), g_pGraphics->resX / 2,
                      (int)(g_pGraphics->resY * 0x118) / 0x1e0, (int *)g_colourText0x0052496c, 0x12);
    }
    FrontendDraw_MenuList(pMenu, NULL, (int)(g_pGraphics->resY * 0x184) / 0x1e0, -1, 1, 1);
    if (pMenu->cursor == 0) {
        count = Profile_GetFileCount();
        if (0 < count) {
            pRecord = Profile_GetFileIdentifier(pMenu->items[0].max);
            sprintf(g_unk0x008183cc, g_str0x00524cfc, CFrontend::GetTextString(0x139),
                    CFrontend::GetTextString(0x7e), CFrontend::GetTextString(0x80));
            flags = *(unsigned int *)(pRecord + 4);
            sprintf(g_unk0x00818554, g_str0x00524ce4, CFrontend::GetTextString(0x13a), pRecord,
                    flags >> 0x10 & 0x1f, flags >> 8 & 0xf, (flags & 0xff) + 0x73a);
            if (FrontendProfile_IsAlreadyInChampionship(pMenu->items[0].max)) {
                strcpy(g_unk0x00818368, CFrontend::GetTextString(0xdf));
            } else {
                sprintf(g_unk0x00818368, CFrontend::GetTextString(0x13b));
                Font_Unused(g_unk0x00818368, Frontend_GetOverlayMode());
                FrontendDraw_RallyInfoHeader();
                FrontendDraw_HelpText(CFrontend::GetTextString(0xf9), 1);
                return;
            }
        } else {
            strcpy(g_unk0x008183cc, CMain::m_logFileBlankLine);
            strcpy(g_unk0x00818554, CMain::m_logFileBlankLine);
            strcpy(g_unk0x00818368, CMain::m_logFileBlankLine);
        }
    } else {
        strcpy(g_unk0x008183cc, CMain::m_logFileBlankLine);
        strcpy(g_unk0x00818554, CMain::m_logFileBlankLine);
        strcpy(g_unk0x00818368, CMain::m_logFileBlankLine);
    }
    FrontendDraw_RallyInfoHeader();
    FrontendDraw_HelpText(CFrontend::GetTextString(0xf9), 1);
}

// Draws the championship name entry screen: the keyboard grid, and under it
// the name of the current record with the characters not typed yet scrambled.
// match 59%: reparto de registros y ranuras de pila del bucle 3x10
// match 62%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004d7380
void FrontendMenu_DrawProfileRenameEntry(Menu *pMenu)
{
    char *text[2];
    char ch[2];
    MenuItem *pItem;
    int *pColour;
    unsigned char font;
    int row;
    int col;
    int i;

    text[0] = g_profileHeaderText;
    text[1] = CFrontend::GetTextString(0x17b);
    FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, 3, text, 2);
    FrontendDraw_PlayTime();
    for (row = 0; row < 3; row++) {
        for (col = 0; col < 10; col++) {
            font = 2;
            if (row == 0) {
                sprintf(CFrontend::m_stringDest, g_strCharFormat0x00524d08, g_strNameRow0x005249a8[col]);
            } else if (row == 1) {
                sprintf(CFrontend::m_stringDest, g_strCharFormat0x00524d08, g_strNameRow0x005249b4[col]);
            } else {
                if (g_strNameRow0x005249c0[col] == '<') {
                    font = 1;
                    strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(0x13f));
                } else if (g_strNameRow0x005249c0[col] == '_') {
                    font = 1;
                    strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(0x140));
                } else {
                    sprintf(CFrontend::m_stringDest, g_strCharFormat0x00524d08, g_strNameRow0x005249c0[col]);
                }
            }
            pItem = &pMenu->items[pMenu->cursor];
            if (col != pItem->max || row != pItem->value)
                pColour = (int *)g_colourText0x0052496c;
            else
                pColour = (int *)g_colourWhite0x00524968;
            if (g_strNameRow0x005249c0[col] == ' ' && row == 2) {
                g_frontendDrawRect[0] = (short)(((col + 2) * g_pGraphics->resX) / 0xe);
                g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 6) / 8) -
                                     (short)(Font_GetTextHeight(font, CMain::m_logFileBlankLine) / 2);
                g_frontendDrawRect[2] = (short)(g_pGraphics->resX / 0xe);
                g_frontendDrawRect[3] = (short)(Font_GetTextHeight(font, CMain::m_logFileBlankLine) / 2);
                Frontend_DrawRectangleOutline(g_frontendDrawRect, (BYTE *)pColour);
            }
            Font_DrawText(font, CFrontend::m_stringDest,
                          g_pGraphics->resX / 0x1c + ((col + 2) * g_pGraphics->resX) / 0xe,
                          (int)(g_pGraphics->resY * (row + 4)) / 8, pColour, 0x12);
        }
    }
    strcpy(CFrontend::m_stringDest, (char *)FrontendProfile_GetEnteredNameBuffer());
    while (strlen(CFrontend::m_stringDest) < 43) {
        CFrontend::m_stringDest[strlen(CFrontend::m_stringDest) + 1] = 0;
        CFrontend::m_stringDest[strlen(CFrontend::m_stringDest)] = (char)(rand() % 0x1a) + 'a';
    }
    for (i = 0; i < 43; i++) {
        ch[0] = CFrontend::m_stringDest[i];
        ch[1] = 0;
        if (i < (int)strlen((char *)FrontendProfile_GetEnteredNameBuffer()))
            pColour = (int *)g_colourWhite0x00524968;
        else
            pColour = (int *)g_colourText0x0052496c;
        Font_DrawText(1, ch, ((i + 3) * g_pGraphics->resX) / 0x30, (int)(g_pGraphics->resY * 2) / 8, pColour, 0x12);
    }
    FrontendDraw_HelpText(CFrontend::GetTextString(0x57), 1);
}

// Draws the stage selection screen: the rally name and date, the record car of
// the selected stage and the list of the stages of the rally.
// match 90%: reparto de registros en el bucle de los nombres de etapa
// FUNCTION: CMR2 0x004d8480
void FrontendMenu_DrawSavedStageSelection(Menu *pMenu)
{
    short rect[4];
    // PORT: the original copies the driver's name into four bytes of its
    // stack, past which a longer name ran over other locals; here it reached
    // the return address.
    char name[64];
    char *text[2];
    int *pColour;
    int baseY;
    int x;
    int i;

    rect[0] = (short)((int)(g_pGraphics->resX * 100) / 0x280);
    rect[1] = 0;
    rect[2] = CFrontend::m_pAr640ATexture->width;
    rect[3] = CFrontend::m_pAr640ATexture->height;
    FrontendDraw_PlayTime();
    sprintf(g_str0x008188a4, CFrontend::GetTextString(0xdc),
            (CGameInfo::GetConfiguredPlayerCount() & 0xff) - FrontendProfile_GetPlayersRemaining());
    snprintf(name, sizeof(name), "%s",
             (char *)RallyData_GetRecord(CGameInfo::GetConfiguredPlayerCount() - 1 - FrontendProfile_GetPlayersRemaining()));
    text[0] = g_str0x008188a4;
    text[1] = name;
    FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, 3, text, 2);
    baseY = ((int)(g_pGraphics->resY * 0x38) / 0x1e0 + (int)(g_pGraphics->resY * 0x176) / 0x1e0) / 2 -
            (((int)(g_pGraphics->resY * 0x24) / 0x1e0) * pMenu->itemCount) / 2;
    g_frontendDrawRect[0] = (short)((int)(g_pGraphics->resX * 99) / 0x280);
    g_frontendDrawRect[1] = (short)baseY;
    g_frontendDrawRect[2] = (short)((int)(g_pGraphics->resX * 0x11a) / 0x280);
    g_frontendDrawRect[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, (BYTE *)g_colourShadowWhite0x00524974, 1);
    g_frontendDrawRect[1] = g_frontendDrawRect[1] + 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, (BYTE *)g_colourWhite0x00524968, 1);
    rect[1] = (short)((int)(g_pGraphics->resY * 0x14) / 0x1e0) -
              CFrontend::m_pAr640ATexture->height / 2 + (short)baseY;
    Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, (SpriteRect *)rect,
                 CFrontend::m_pAr640ATexture, 1, 0, NULL, NULL, (BYTE *)g_colourWhite0x00524968, 8);
    strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pMenu->items[0].id));
    x = (int)(g_pGraphics->resX * 0x7a) / 0x280;
    Font_DrawText(1, CFrontend::m_stringDest, x,
                  (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1],
                  (int *)g_colourWhite0x00524968, 0x11);
    for (i = 0; i < pMenu->items[0].min; i++) {
        pColour = (int *)g_colourWhite0x00524968;
        if (pMenu->items[0].max != i)
            pColour = (int *)g_colourText0x0052496c;
        x = (int)(g_pGraphics->resX * 10) / 0x280 + x + Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
        Font_DrawText(1, CFrontend::GetTextString(i + 0x131), x,
                      (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], pColour, 0x11);
        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(i + 0x131));
    }
    g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 0x24) / 0x1e0) + (short)baseY;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, (BYTE *)g_colourShadowWhite0x00524974, 1);
    g_frontendDrawRect[1] = g_frontendDrawRect[1] + 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, (BYTE *)g_colourWhite0x00524968, 1);
    if (CGameInfo::GetConfiguredGameMode() != 4)
        FrontendDraw_ScrollerRow(FrontendScroller_GetRouteEntryScroller(), 0);
    FrontendDraw_HelpText(CFrontend::GetTextString(0x57), 1);
}

// Text of an empty leaderboard row and of its time column.
// GLOBAL: CMR2 0x00519fb0
char g_str0x00519fb0[4] = "0";
// GLOBAL: CMR2 0x00519fb4
char g_str0x00519fb4[4] = "---";
// Arrows of the "1/8" leaderboard pager.
// GLOBAL: CMR2 0x005250d4
char g_str0x005250d4[4] = ">";
// GLOBAL: CMR2 0x005250d8
char g_str0x005250d8[4] = "<";

extern char g_stageNumberFormat[];

// Draws the leaderboard screen: the pager line, the ten names with their wins
// and the list of leaderboards.
// match 73%: reparto de registros en el bucle de las diez filas
// FUNCTION: CMR2 0x004e9990
void FrontendMenu_DrawNetworkLeaderboard(Menu *pMenu)
{
    char *text[3];
    NetworkLeaderboard *pBoard;
    NetworkLeaderboardEntry *pEntry;
    int *pColour;
    int y;
    int i;

    text[0] = CFrontend::GetTextString(0x12);
    text[1] = CFrontend::GetTextString(0x1e9);
    text[2] = CFrontend::GetTextString(0x1c4);
    FrontendDraw_PlayTime();
    FrontendDraw_Breadcrumb(PATH_X(), PATH_Y(), text, 3);
    pColour = (int *)g_colourWhite0x00524968;
    if (pMenu->items[pMenu->cursor].value != 0)
        pColour = (int *)g_colourText0x0052496c;
    if (!(CNetworkLeaderboards::GetTotalLeaderboards() == 0)) {
        sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x1ea),
                Menu_GetItem(pMenu, 0)->max + 1, Menu_GetItem(pMenu, 0)->min);
        Font_DrawText(1, CFrontend::m_stringDest, g_pGraphics->resX / 2,
                      (int)(g_pGraphics->resY * 0x46) / 0x1e0, pColour, 0x12);
        pBoard = CNetworkLeaderboards::GetLoadedLeaderboard(Menu_GetItem(pMenu, 0)->max);
        for (i = 0; i < 10; i++) {
            pEntry = &pBoard->entries[i];
            if (pEntry->name[0] != 0) {
                Font_DrawText(0, pEntry->name, (int)(g_pGraphics->resX * 0x118) / 0x280,
                              (int)(g_pGraphics->resY * 0x5a) / 0x1e0 +
                                  ((int)(g_pGraphics->resY * 0xf) / 0x1e0) * i, pColour, 0x12);
                sprintf(CFrontend::m_stringDest, g_stageNumberFormat, pEntry->wins);
                y = (int)(g_pGraphics->resY * 0x5a) / 0x1e0 + ((int)(g_pGraphics->resY * 0xf) / 0x1e0) * i;
                Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x154) / 0x280, y,
                              pColour, 0x12);
            } else {
                Font_DrawText(0, g_str0x00519fb4, (int)(g_pGraphics->resX * 0x118) / 0x280,
                              (int)(g_pGraphics->resY * 0x5a) / 0x1e0 +
                                  ((int)(g_pGraphics->resY * 0xf) / 0x1e0) * i, pColour, 0x12);
                y = (int)(g_pGraphics->resY * 0x5a) / 0x1e0 + ((int)(g_pGraphics->resY * 0xf) / 0x1e0) * i;
                Font_DrawText(0, g_str0x00519fb0, (int)(g_pGraphics->resX * 0x154) / 0x280, y, pColour, 0x12);
            }
        }
        if (Menu_GetItem(pMenu, 0)->min > 0) {
            Font_DrawText(1, g_str0x005250d8,
                          (int)(g_pGraphics->resX * 0x118) / 0x280 - (int)(g_pGraphics->resX * 0x32) / 0x280,
                          (int)(g_pGraphics->resY * 0xa0) / 0x1e0, pColour, 0x12);
            Font_DrawText(1, g_str0x005250d4,
                          (int)(g_pGraphics->resX * 0x3c) / 0x280 + (int)(g_pGraphics->resX * 0x154) / 0x280,
                          (int)(g_pGraphics->resY * 0xa0) / 0x1e0, pColour, 0x12);
        }
    } else {
        Font_DrawText(1, CFrontend::GetTextString(0x1eb), g_pGraphics->resX / 2,
                      (int)(g_pGraphics->resY) / 4, pColour, 0x12);
    }
    FrontendDraw_MenuList(pMenu, NULL, (int)(g_pGraphics->resY * 0x140) / 0x1e0, -1, 1, 1);
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, 0);
}

extern unsigned int RallyData_GetChampionshipMedal(int param_1, int param_2);
extern unsigned int RallyData_GetRallyMedal(int param_1, int param_2, int param_3);
extern unsigned int RallyData_GetArcadeMedal(int param_1, int param_2, int param_3);

// Draws the multiplayer entry screen: breadcrumb and play time, the three
// driver columns with their flags, the three car rows and the bottom row of
// stage options, laying everything out from the current screen size (the flag
// sprite is 0x12x0xc, 0x1c/0x12 in the 1024 mode).
// match 71.35% (auditado W165): el unico diff de forma era el umbral (GetScreenWidth() >= 0x400; el
// original usa cmp eax,0x400 / jb); el resto es reordenado de los bloques de division por constante
// y reparto de registros, con las mismas llamadas y constantes.
// match 77%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004d50a0
void FrontendMenu_DrawProfileRecordsSummary(Menu *pMenu)
{
    SpriteRect src;
    char *text[4];
    int i;
    int j;

    src.x = 0;
    src.y = 0;
    src.w = 0x12;
    src.h = 0xc;
    if (CGameInfo::GetScreenWidth() >= 0x400) {
        if (CFrontend::IsTextureWidthSupported(0x400) != 0) {
            if (CFrontend::IsTextureHeightSupported(0x400) != 0) {
                src.w = 0x1c;
                src.h = 0x12;
            }
        }
    }
    text[0] = CFrontend::GetTextString(0xb);
    text[1] = CFrontend::GetTextString(0x59);
    text[2] = (char *)RallyData_GetRecord(0);
    text[3] = CFrontend::GetTextString(pMenu->field_0x4);
    FrontendDraw_Breadcrumb((int)(g_pGraphics->resX * 0x18) / 0x280,
                            (int)(g_pGraphics->resY * 0x26) / 0x1e0, text, 4);
    FrontendDraw_PlayTime();

    // Driver columns.
    Font_DrawText(1, CFrontend::GetTextString(0xc), (int)g_pGraphics->resX / 2,
                  (int)(g_pGraphics->resY * 0x46) / 0x1e0,
                  (int *)g_colourWhite0x00524968, 0x12);
    for (i = 0; i < 3; i++) {
        FrontendDraw_MatrixMedalRect(((int)g_pGraphics->resX / 4) * (i + 1),
                     (int)(g_pGraphics->resY * 0x19) / 0x1e0 +
                     (int)(g_pGraphics->resY * 0x46) / 0x1e0,
                     RallyData_GetChampionshipMedal(0, i));
        Font_DrawText(1, CFrontend::GetTextString(i + 0xd0),
                      ((int)g_pGraphics->resX / 4) * (i + 1),
                      (int)(g_pGraphics->resY * 0x19) / 0x1e0 +
                      (int)(g_pGraphics->resY * 0x46) / 0x1e0 -
                      (int)(g_pGraphics->resY * 4) / 0x1e0,
                      (int *)g_colourText0x0052496c, 0x22);
    }

    // Car rows.
    Font_DrawText(1, CFrontend::GetTextString(0xd), (int)g_pGraphics->resX / 2,
                  (int)(g_pGraphics->resY * 0xa0) / 0x1e0,
                  (int *)g_colourWhite0x00524968, 0x12);
    for (i = 0; i < 3; i++) {
        Font_DrawText(1, CFrontend::GetTextString(i + 0xd0),
                      (int)(g_pGraphics->resX * 0xb4) / 0x280 -
                      (int)(g_pGraphics->resX << 5) / 0x280,
                      (int)(g_pGraphics->resY * 10) / 0x1e0 +
                      (int)(g_pGraphics->resY * 0x18) / 0x1e0 +
                      (int)(g_pGraphics->resY * 0xa0) / 0x1e0 +
                      ((int)(g_pGraphics->resY * 0x1e) / 0x1e0) * i,
                      (int *)g_colourText0x0052496c, 0xc);
    }

    // Flag of every driver in the race.
    for (i = 0; i < 8; i++) {
        g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 0xb4) / 0x280 +
                             ((int)(g_pGraphics->resX * 0x32) / 0x280) * i -
                             src.w / 2;
        g_frontendDrawRect[2] = src.w;
        g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 10) / 0x1e0 +
                             (int)(g_pGraphics->resY * 0xa0) / 0x1e0;
        g_frontendDrawRect[3] = src.h;
        Sprite_Queue(&src, (SpriteRect *)g_frontendDrawRect,
                     CFrontend::m_pTinyFlags[i], 1, 0, NULL, NULL,
                     g_colourWhite0x00524968, 8);
        for (j = 0; j < 3; j++) {
            FrontendDraw_MatrixMedalRect((int)(g_pGraphics->resX * 0xb4) / 0x280 +
                         ((int)(g_pGraphics->resX * 0x32) / 0x280) * i,
                         (int)(g_pGraphics->resY * 10) / 0x1e0 +
                         (int)(g_pGraphics->resY * 0x18) / 0x1e0 +
                         (int)(g_pGraphics->resY * 0xa0) / 0x1e0 +
                         ((int)(g_pGraphics->resY * 0x1e) / 0x1e0) * j,
                         RallyData_GetRallyMedal(0, i, j));
        }
    }

    // Stage options.
    Font_DrawText(1, CFrontend::GetTextString(0xd6), (int)g_pGraphics->resX / 2,
                  (int)(g_pGraphics->resY * 0x145) / 0x1e0,
                  (int *)g_colourWhite0x00524968, 0x12);
    for (i = 0; i < 3; i++) {
        if (i == 0) {
            Font_DrawText(1, CFrontend::GetTextString(0xe9),
                          (int)(g_pGraphics->resX * 300) / 0x280,
                          (int)(g_pGraphics->resY * 5) / 0x1e0 +
                          (int)(g_pGraphics->resY * 0x18) / 0x1e0 +
                          (int)(g_pGraphics->resY * 0x145) / 0x1e0 -
                          (int)(g_pGraphics->resY * 4) / 0x1e0,
                          (int *)g_colourText0x0052496c, 0x22);
            Font_DrawText(1, CFrontend::GetTextString(0xe8),
                          (int)(g_pGraphics->resX * 0x17c) / 0x280,
                          (int)(g_pGraphics->resY * 5) / 0x1e0 +
                          (int)(g_pGraphics->resY * 0x18) / 0x1e0 +
                          (int)(g_pGraphics->resY * 0x145) / 0x1e0 -
                          (int)(g_pGraphics->resY * 4) / 0x1e0,
                          (int *)g_colourText0x0052496c, 0x22);
        }
        Font_DrawText(1, CFrontend::GetTextString(i + 0xd0),
                      (int)(g_pGraphics->resX * 300) / 0x280 -
                      (int)(g_pGraphics->resX << 5) / 0x280,
                      (int)(g_pGraphics->resY * 5) / 0x1e0 +
                      (int)(g_pGraphics->resY * 0x18) / 0x1e0 +
                      (int)(g_pGraphics->resY * 0x145) / 0x1e0 +
                      ((int)(g_pGraphics->resY * 0x1e) / 0x1e0) * i,
                      (int *)g_colourText0x0052496c, 0xc);
        FrontendDraw_MatrixMedalRect((int)(g_pGraphics->resX * 300) / 0x280,
                     (int)(g_pGraphics->resY * 5) / 0x1e0 +
                     (int)(g_pGraphics->resY * 0x18) / 0x1e0 +
                     (int)(g_pGraphics->resY * 0x145) / 0x1e0 +
                     ((int)(g_pGraphics->resY * 0x1e) / 0x1e0) * i,
                     RallyData_GetArcadeMedal(0, 0, i));
        if (i != 0) {
            FrontendDraw_MatrixMedalRect((int)(g_pGraphics->resX * 0x17c) / 0x280,
                         (int)(g_pGraphics->resY * 5) / 0x1e0 +
                         (int)(g_pGraphics->resY * 0x18) / 0x1e0 +
                         (int)(g_pGraphics->resY * 0x145) / 0x1e0 +
                         ((int)(g_pGraphics->resY * 0x1e) / 0x1e0) * i,
                         RallyData_GetArcadeMedal(0, 1, i));
        } else {
            g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 0x17c) / 0x280 -
                                 ((int)(g_pGraphics->resX * 0x30) / 0x280) / 2;
            g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 5) / 0x1e0 +
                                 (int)(g_pGraphics->resY * 0x18) / 0x1e0 +
                                 (int)(g_pGraphics->resY * 0x145) / 0x1e0;
            g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 0x30) / 0x280;
            g_frontendDrawRect[3] = (int)(g_pGraphics->resY * 0x1c) / 0x1e0;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect,
                            g_colourDim0x00524970, 4);
            Frontend_DrawRectangleOutline(g_frontendDrawRect, g_colourText0x0052496c);
        }
    }
}
void RallyData_FillStageSplitEditorRows(Menu *param_1, int param_2, char param_3);

// Item action of the stage-split list: opens the split editor of the selected
// row, or the next screen of the current game mode.
// FUNCTION: CMR2 0x004efde0
void FrontendMenu_SelectRallyStage(Menu *pMenu, int param)
{
    RallyData_SetStageSelectionAndRefreshFlags(pMenu->cursor);
    if (g_unk0x00819130[pMenu->cursor] == 1) {
        if (CGameInfo::GetConfiguredGameMode() == 2) {
            RallyData_FillStageSplitEditorRows(pMenu, param, 1);
            return;
        }
        if (CGameInfo::GetConfiguredGameMode() == 6) {
            if (CGameInfo::GetConfiguredPlayerCount() == 2) {
                Menu_SetNextAction(FrontendMenu_GetQuickRaceAdvancedSettings());
                return;
            }
            Menu_SetNextAction(FrontendMenu_GetQuickRaceSettings());
            return;
        }
        Menu_SetNextAction(FrontendMenu_GetRallyStartTransition());
    }
}

// Enables the rally rows of the stage list from the current session flags and
// selects the first enabled row.
// FUNCTION: CMR2 0x004efb70
void FrontendMenu_EnableRallyStageRows(Menu *pMenu, int param)
{
    unsigned int *pInfo;
    int limit;

    RallyData_SetCountrySelectionBits(pMenu->cursor);
    if (CGameInfo::GetConfiguredGameMode() == 2)
        RallyData_FillStageSplitEditorRows(pMenu, param, 0);
    if (CGameInfo::GetConfiguredGameMode() == 1) {
        RallyData_SetStageSelectionAndRefreshFlags(0);
        Menu_SetNextAction(FrontendMenu_GetRallyStartTransition());
        GameInfo_ResetSessionTimestamp();
        return;
    }
    limit = 1;
    pInfo = CGameInfo::GetGameInfoField9CAddress();
    memset(g_unk0x00819130, 1, 11);
    if (CGameInfo::IsRecordFlagSet(0xd) != 0) {
        if ((BYTE)RallyDataCountryIndex() % 2 != 0) {
            FrontendMenu_GetRallyStageSelection()->itemCount = 0xb;
        } else {
            g_unk0x00819130[10] = 0xff;
            FrontendMenu_GetRallyStageSelection()->itemCount = 10;
        }
    } else {
        if ((BYTE)(*pInfo >> 8 & 0xf) >= (BYTE)RallyDataCountryIndex() + 1)
            limit = 4;
        if ((BYTE)(*pInfo >> 0xc & 0xf) >= (BYTE)RallyDataCountryIndex() + 1)
            limit = 8;
        if ((BYTE)RallyDataCountryIndex() + 1 <= (BYTE)(*pInfo >> 0x10 & 0xf) && (*pInfo & 1) != 0)
            limit = 10;
        if (limit < 11)
            memset(&g_unk0x00819130[limit], 0, 11 - limit);
        if (CGameInfo::GetConfiguredGameMode() == 2) {
            switch (CGameInfo::GetConfiguredDifficulty()) {
            case 0:
                memset(&g_unk0x00819130[4], 0, 6);
                break;
            case 1:
                memset(&g_unk0x00819130[8], 0, 2);
                break;
            }
        }
        if ((BYTE)RallyDataCountryIndex() % 2 != 0) {
            if (((1 << ((BYTE)RallyDataCountryIndex() + 1) / 2 - 1) & pInfo[1] & 0x1f) != 0 ||
                (((pInfo[1] >> 5) & 0x1f) & (1 << ((BYTE)RallyDataCountryIndex() + 1) / 2 - 1)) != 0) {
                g_unk0x00819130[10] = 1;
            } else {
                g_unk0x00819130[10] = 0;
                if ((((pInfo[1] >> 10) & 0x1f) & (1 << ((BYTE)RallyDataCountryIndex() + 1) / 2 - 1)) != 0)
                    g_unk0x00819130[10] = 1;
            }
            FrontendMenu_GetRallyStageSelection()->itemCount = 0xb;
        } else {
            g_unk0x00819130[10] = 0xff;
            FrontendMenu_GetRallyStageSelection()->itemCount = 10;
        }
    }
    if (FrontendMenu_GetRallyStageSelection()->cursor >= FrontendMenu_GetRallyStageSelection()->itemCount)
        FrontendMenu_GetRallyStageSelection()->cursor = FrontendMenu_GetRallyStageSelection()->itemCount - 1;
    Menu_SetNextAction(FrontendMenu_GetRallyStageSelection());
}

// Palette index (g_screenPaletteIds) -> livery index of m_unk0x00818530.
// GLOBAL: CMR2 0x005249cc
int g_paletteLiveryIndex[23] = {
    1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 0, 2, 0,
};

// Source rects (x, y, w, h) of the car picture per palette: 640x480 build.
// GLOBAL: CMR2 0x00524a28
short g_carPictureRectsLow[22][4] = {
    0, 0, 112, 62,
    0, 186, 112, 62,
    0, 62, 112, 62,
    0, 124, 112, 62,
    144, 0, 112, 62,
    144, 62, 112, 62,
    144, 186, 112, 62,
    144, 124, 112, 62,
    0, 0, 112, 62,
    0, 186, 112, 62,
    0, 124, 112, 62,
    0, 62, 112, 62,
    144, 186, 112, 62,
    144, 62, 112, 62,
    144, 124, 112, 62,
    0, 0, 112, 62,
    0, 124, 112, 62,
    0, 62, 112, 62,
    144, 0, 112, 62,
    0, 186, 112, 62,
    144, 0, 112, 62,
    144, 62, 112, 62,
};

// Source rects (x, y, w, h) of the car picture per palette: high resolution.
// GLOBAL: CMR2 0x00524ad8
short g_carPictureRectsHigh[22][4] = {
    0, 0, 180, 100,
    0, 300, 180, 100,
    0, 100, 180, 100,
    0, 200, 180, 100,
    230, 0, 180, 100,
    230, 100, 180, 100,
    230, 300, 180, 100,
    230, 200, 180, 100,
    0, 0, 180, 100,
    0, 300, 180, 100,
    0, 200, 180, 100,
    0, 100, 180, 100,
    230, 300, 180, 100,
    230, 100, 180, 100,
    230, 200, 180, 100,
    0, 0, 180, 100,
    0, 200, 180, 100,
    0, 100, 180, 100,
    230, 0, 180, 100,
    0, 300, 180, 100,
    230, 0, 180, 100,
    230, 100, 180, 100,
};

// Draws the car page: the breadcrumbs, the front and back pictures of the
// selected car and its three performance bars.
// match 88%: el unico diff que queda es asignacion de registros (pMenu acaba en
// EDI en vez de EBX, y la textura al reves) y el orden de algunas cargas; la
// logica y todos los operandos coinciden.
// FUNCTION: CMR2 0x004d7db0
void FrontendMenu_DrawCarSetup(Menu *pMenu)
{
    short (*pRects)[4];
    char label[4];
    char *names[2];
    Texture *pTexture;
    SpriteRect dst;
    SpriteRect src;
    int speed;
    int accel;
    int grip;
    short height;
    int x;

    if (CGameInfo::GetScreenWidth() < 0x400 || CFrontend::IsTextureWidthSupported(0x400) == 0 ||
        CFrontend::IsTextureHeightSupported(0x400) == 0)
        pRects = g_carPictureRectsLow;
    else
        pRects = g_carPictureRectsHigh;
    FrontendDraw_PlayTime();
    sprintf(g_str0x008188a4, CFrontend::GetTextString(0xdc),
            (BYTE)CGameInfo::GetConfiguredPlayerCount() - (BYTE)FrontendProfile_GetPlayersRemaining());
    strcpy(label, (char *)RallyData_GetRecord((BYTE)(0xff - (BYTE)FrontendProfile_GetPlayersRemaining() + (BYTE)CGameInfo::GetConfiguredPlayerCount())));
    names[0] = g_str0x008188a4;
    names[1] = label;
    FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, 3, names, 2);
    FrontendDraw_ScrollerRow(FrontendScroller_GetRouteEntryScroller(), 1);
    if (CGameInfo::GetScreenWidth() >= 0x400 && CFrontend::IsTextureWidthSupported(0x400) && CFrontend::IsTextureHeightSupported(0x400))
        height = 0x1ce;
    else
        height = 0xe8;
    pTexture = CFrontend::m_unk0x0081884c[g_screenPaletteIds[pMenu->cursor]];
    if (pTexture != NULL) {
        src = *(SpriteRect *)&pTexture->field_0x11c;
        dst = *(SpriteRect *)&pTexture->field_0x11c;
        src.h = height;
        dst.h = height;
        dst.x = (int)(g_pGraphics->resX * 0x8e) / 0x280;
        dst.y = (int)(g_pGraphics->resY * 0x147) / 0x1e0;
        dst.x -= dst.w / 2;
        dst.y -= height;
        Sprite_Queue(&src, &dst, pTexture, 1, 0, 0, NULL, g_colourWhite0x00524968, 8);
    }
    pTexture = CFrontend::m_unk0x008182cc[g_screenPaletteIds[pMenu->cursor]];
    if (pTexture != NULL) {
        src = *(SpriteRect *)&pTexture->field_0x11c;
        dst = *(SpriteRect *)&pTexture->field_0x11c;
        src.h = height;
        dst.h = height;
        dst.x = (int)(g_pGraphics->resX * 0x165) / 0x280;
        dst.y = (int)(g_pGraphics->resY * 0x147) / 0x1e0;
        dst.x -= dst.w / 2;
        dst.y -= height;
        Sprite_Queue(&src, &dst, pTexture, 1, 0, 0, NULL, g_colourWhite0x00524968, 8);
    }
    pTexture = CFrontend::m_unk0x00818530[g_paletteLiveryIndex[g_screenPaletteIds[pMenu->cursor]]];
    if (pTexture != NULL) {
        g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 0x22c) / 0x280;
        g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 0xab) / 0x1e0;
        g_frontendDrawRect[2] = pRects[g_screenPaletteIds[pMenu->cursor]][2];
        g_frontendDrawRect[3] = pRects[g_screenPaletteIds[pMenu->cursor]][3];
        g_frontendDrawRect[0] -= g_frontendDrawRect[2] / 2;
        g_frontendDrawRect[1] -= g_frontendDrawRect[3] / 2;
        Sprite_Queue((SpriteRect *)pRects[g_screenPaletteIds[pMenu->cursor]], (SpriteRect *)g_frontendDrawRect, pTexture, 1, 0, 0, NULL,
                     g_colourWhite0x00524968, 8);
    }
    FrontendCar_GetClassPerformance(CFrontend::GetArchivePrimaryIDEntry(g_screenPaletteIds[pMenu->cursor]), &speed, &accel, &grip);
    FrontendDraw_ModeProgressBar((int)(g_pGraphics->resX * 500) / 640, (int)(g_pGraphics->resY * 268) / 480, 0xb, speed,
                 CFrontend::GetTextString(0x186));
    x = FrontendDraw_ModeProgressBar((int)(g_pGraphics->resX * 500) / 640, (int)(g_pGraphics->resY * 300) / 480, 0xb, accel,
                     CFrontend::GetTextString(0x187));
    sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue, CFrontend::GetTextString(grip + 0x183));
    Font_DrawText(1, CFrontend::m_stringDest, x, (int)(g_pGraphics->resY * 245) / 480,
                  (int *)g_colourText0x0052496c, 0x14);
    FrontendDraw_HelpText(CFrontend::GetTextString(0x57), 1);
}

// Byte for byte the same as FrontendMenu_DrawCarSetup in the original.
// FUNCTION: CMR2 0x004d8950
void FrontendMenu_DrawPaletteSelection(Menu *pMenu)
{
    short (*pRects)[4];
    char label[4];
    char *names[2];
    Texture *pTexture;
    SpriteRect dst;
    SpriteRect src;
    int speed;
    int accel;
    int grip;
    short height;
    int x;

    if (CGameInfo::GetScreenWidth() < 0x400 || CFrontend::IsTextureWidthSupported(0x400) == 0 ||
        CFrontend::IsTextureHeightSupported(0x400) == 0)
        pRects = g_carPictureRectsLow;
    else
        pRects = g_carPictureRectsHigh;
    FrontendDraw_PlayTime();
    sprintf(g_str0x008188a4, CFrontend::GetTextString(0xdc),
            (BYTE)CGameInfo::GetConfiguredPlayerCount() - (BYTE)FrontendProfile_GetPlayersRemaining());
    strcpy(label, (char *)RallyData_GetRecord((BYTE)(0xff - (BYTE)FrontendProfile_GetPlayersRemaining() + (BYTE)CGameInfo::GetConfiguredPlayerCount())));
    names[0] = g_str0x008188a4;
    names[1] = label;
    FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, 3, names, 2);
    FrontendDraw_ScrollerRow(FrontendScroller_GetRouteEntryScroller(), 1);
    if (CGameInfo::GetScreenWidth() >= 0x400 && CFrontend::IsTextureWidthSupported(0x400) && CFrontend::IsTextureHeightSupported(0x400))
        height = 0x1ce;
    else
        height = 0xe8;
    pTexture = CFrontend::m_unk0x0081884c[g_screenPaletteIds[pMenu->cursor]];
    if (pTexture != NULL) {
        src = *(SpriteRect *)&pTexture->field_0x11c;
        dst = *(SpriteRect *)&pTexture->field_0x11c;
        src.h = height;
        dst.h = height;
        dst.x = (int)(g_pGraphics->resX * 0x8e) / 0x280;
        dst.y = (int)(g_pGraphics->resY * 0x147) / 0x1e0;
        dst.x -= dst.w / 2;
        dst.y -= height;
        Sprite_Queue(&src, &dst, pTexture, 1, 0, 0, NULL, g_colourWhite0x00524968, 8);
    }
    pTexture = CFrontend::m_unk0x008182cc[g_screenPaletteIds[pMenu->cursor]];
    if (pTexture != NULL) {
        src = *(SpriteRect *)&pTexture->field_0x11c;
        dst = *(SpriteRect *)&pTexture->field_0x11c;
        src.h = height;
        dst.h = height;
        dst.x = (int)(g_pGraphics->resX * 0x165) / 0x280;
        dst.y = (int)(g_pGraphics->resY * 0x147) / 0x1e0;
        dst.x -= dst.w / 2;
        dst.y -= height;
        Sprite_Queue(&src, &dst, pTexture, 1, 0, 0, NULL, g_colourWhite0x00524968, 8);
    }
    pTexture = CFrontend::m_unk0x00818530[g_paletteLiveryIndex[g_screenPaletteIds[pMenu->cursor]]];
    if (pTexture != NULL) {
        g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 0x22c) / 0x280;
        g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 0xab) / 0x1e0;
        g_frontendDrawRect[2] = pRects[g_screenPaletteIds[pMenu->cursor]][2];
        g_frontendDrawRect[3] = pRects[g_screenPaletteIds[pMenu->cursor]][3];
        g_frontendDrawRect[0] -= g_frontendDrawRect[2] / 2;
        g_frontendDrawRect[1] -= g_frontendDrawRect[3] / 2;
        Sprite_Queue((SpriteRect *)pRects[g_screenPaletteIds[pMenu->cursor]], (SpriteRect *)g_frontendDrawRect, pTexture, 1, 0, 0, NULL,
                     g_colourWhite0x00524968, 8);
    }
    FrontendCar_GetClassPerformance(CFrontend::GetArchivePrimaryIDEntry(g_screenPaletteIds[pMenu->cursor]), &speed, &accel, &grip);
    FrontendDraw_ModeProgressBar((int)(g_pGraphics->resX * 500) / 640, (int)(g_pGraphics->resY * 268) / 480, 0xb, speed,
                 CFrontend::GetTextString(0x186));
    x = FrontendDraw_ModeProgressBar((int)(g_pGraphics->resX * 500) / 640, (int)(g_pGraphics->resY * 300) / 480, 0xb, accel,
                     CFrontend::GetTextString(0x187));
    sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue, CFrontend::GetTextString(grip + 0x183));
    Font_DrawText(1, CFrontend::m_stringDest, x, (int)(g_pGraphics->resY * 245) / 480,
                  (int *)g_colourText0x0052496c, 0x14);
    FrontendDraw_HelpText(CFrontend::GetTextString(0x57), 1);
}

// Draws a settings list page: the breadcrumbs, the framed title and one row
// per menu item (the row sprite, its label and the current value).
// match 67%: WIP. La estructura y los operandos son los del original pero el
// reparto de registros/slots del bucle y el orden de las cargas no cuadran
// todavia; falta afinar la forma del fuente (no hay bug de logica conocido).
// FUNCTION: CMR2 0x004e8b60
void FrontendMenu_DrawNetworkStageSetup(Menu *pMenu)
{
    SpriteRect rect;
    char *text[3];
    MenuItem *pItem;
    BYTE *pColour;
    BYTE *pLineColour;
    BYTE *pLineShadow;
    int top;
    int i;

    rect.x = (int)(g_pGraphics->resX * 100) / 640;
    rect.y = 0;
    rect.w = CFrontend::m_pAr640ATexture->width;
    rect.h = CFrontend::m_pAr640ATexture->height;
    text[0] = CFrontend::GetTextString(0x12);
    text[1] = CFrontend::GetTextString(0x1d9);
    text[2] = CFrontend::GetTextString(0xd7);
    FrontendDraw_Breadcrumb((int)(g_pGraphics->resX * 0x18) / 0x280, (int)(g_pGraphics->resY * 0x26) / 0x1e0, text, 3);
    top = ((int)(g_pGraphics->resY * 8) / 0x1e0 + (int)(g_pGraphics->resY * 0x26) / 0x1e0 +
           (int)(g_pGraphics->resY * 0x180) / 0x1e0) / 2 -
          ((int)(g_pGraphics->resY * 0x24) / 0x1e0 * pMenu->itemCount) / 2;
    FrontendDraw_PlayTime();
    if (pMenu->cursor == 0) {
        pLineColour = g_colourWhite0x00524968;
        pLineShadow = g_colourShadowWhite0x00524974;
    } else {
        pLineColour = g_colourText0x0052496c;
        pLineShadow = g_colourShadowText0x00524978;
    }
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 99) / 640;
    g_frontendDrawRect[1] = top;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 0x11a) / 0x280;
    g_frontendDrawRect[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineShadow, 1);
    g_frontendDrawRect[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineColour, 1);
    i = 0;
    pItem = pMenu->items;
    if (pMenu->itemCount > 0) {
        do {
            rect.y = (int)(g_pGraphics->resY * 2) / 0x1e0 + (int)(g_pGraphics->resY * 0x12) / 0x1e0 + top +
                     ((int)(g_pGraphics->resY * 0x24) / 0x1e0 * i - CFrontend::m_pAr640ATexture->height / 2);
            if (pMenu->cursor == i) {
                pColour = g_colourWhite0x00524968;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, &rect,
                             CFrontend::m_pAr640ATexture, 1, 0, 0, NULL, g_colourWhite0x00524968, 8);
            } else {
                pColour = g_colourText0x0052496c;
                if (pItem->enabled == 0)
                    pColour = g_colourDim0x00524970;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, &rect,
                             CFrontend::m_pAr640DTexture, 1, 0, 0, NULL, pColour, 8);
            }
            switch (pItem->value) {
            case 0:
                sprintf(CFrontend::m_stringDest, g_strLabelText, CFrontend::GetTextString(0x30),
                        CFrontend::GetTextString(g_unk0x00818ed4[Menu_GetItem(pMenu, 0)->max] + 0x27));
                Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 0x280,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            case 1:
                sprintf(CFrontend::m_stringDest, g_strLabelNumber, CFrontend::GetTextString(0x195),
                        Menu_GetItem(pMenu, 1)->max + 1);
                Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 0x280,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            case 2:
            case 3:
                Font_DrawText(1, CFrontend::GetTextString(pItem->id), (int)(g_pGraphics->resX * 0x7a) / 0x280,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            }
            if (pMenu->cursor == i + 1 || pMenu->cursor == i) {
                pLineColour = g_colourWhite0x00524968;
                pLineShadow = g_colourShadowWhite0x00524974;
            } else {
                pLineColour = g_colourText0x0052496c;
                pLineShadow = g_colourShadowText0x00524978;
            }
            g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 0x24) / 0x1e0 * (i + 1) + top;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineShadow, 1);
            g_frontendDrawRect[1]++;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineColour, 1);
            i++;
            pItem++;
        } while (i < pMenu->itemCount);
    }
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

extern char g_str0x00524d0c[];  // "%d%% %s"   (GameInfo.cpp)
extern char g_str0x00524d14[];  // "%s: "      (GameInfo.cpp)
extern char g_str0x00524d20[];  // "%s: %d.%d" (GameInfo.cpp)
extern char g_str0x00524d2c[];  // "%s: %d.0"  (GameInfo.cpp)
BYTE FrontendRecords_GetStageEntryPayload(StageSurfaceShare **out, int row, int column);
const char *FrontendRecords_GetStageName(int row, int column);
StageInfoEntry *FrontendRecords_GetStageEntry(int row, int column);

// Placeholder shown in the stage rows of the rally summary when the stage has
// no recorded time yet.
extern char g_noTimeText[];
// Distance unit appended to the distance of the stage.
extern char g_str0x00524d1c[];

// Draws the rally summary screen: the event banner and the menu path, the
// stage name with its distance, surface and severity, the record time of the
// selected stage and one row per stage of the rally with its record time.
// FUNCTION: CMR2 0x004d9c40
void FrontendMenu_DrawRallySummary(Menu *pMenu)
{
    char *text[2];
    StageSurfaceShare *table;
    unsigned int country;
    unsigned int time;
    BYTE rating[2];
    int column;
    int x;
    int y;
    int width;
    int count;
    int limit;
    int i;
    int n;

    country = RallyDataCountryIndex() & 0xff;
    column = pMenu->cursor;
    switch (CGameInfo::GetConfiguredGameMode()) {
    case 2:
        text[0] = CFrontend::GetTextString(0xe7);
        text[1] = CFrontend::GetTextString(0xf);
        break;
    case 3:
        text[0] = CFrontend::GetTextString(0xe7);
        text[1] = CFrontend::GetTextString(0x10);
        break;
    }
    FrontendDraw_ScrollerRow(FrontendScroller_GetDisplayDeviceScroller(), 1);
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, (int)(g_pGraphics->resX * 0x18) / 0x280,
                          (int)(g_pGraphics->resY * 0x26) / 0x1e0, 1, 3, text, 2);
    if (CFrontend::m_pSetupRepBanners[RallyDataCountryIndex() & 0xff] != NULL) {
        g_frontendDrawRect[0] = (short)((int)(g_pGraphics->resX * 0x19f) / 0x280);
        g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 0x4b) / 0x1e0);
        g_frontendDrawRect[2] = CFrontend::m_pSetupRepBanners[RallyDataCountryIndex() & 0xff]->width;
        g_frontendDrawRect[3] = CFrontend::m_pSetupRepBanners[RallyDataCountryIndex() & 0xff]->height;
        Sprite_Queue((SpriteRect *)&CFrontend::m_pSetupRepBanners[RallyDataCountryIndex() & 0xff]->field_0x11c,
                     (SpriteRect *)g_frontendDrawRect,
                     CFrontend::m_pSetupRepBanners[RallyDataCountryIndex() & 0xff],
                     1, 0, NULL, NULL, g_colourWhite0x00524968, 8);
    }
    FrontendMap_DrawStageCellGrid(pMenu->cursor, 0);
    x = (int)(g_pGraphics->resX * 0x19f) / 0x280;
    y = (int)(g_pGraphics->resY * 0x4b) / 0x1e0 + (int)(g_pGraphics->resY * 0x50) / 0x1e0;
    sprintf(CFrontend::m_stringDest, g_strLabelText, CFrontend::GetTextString(0x18b),
            FrontendRecords_GetStageName(country, column));
    Font_DrawText(1, CFrontend::m_stringDest, x, y, (int *)g_colourText0x0052496c, 0x21);
    y += (int)(g_pGraphics->resY * 0x10) / 0x1e0;
    rating[0] = FrontendRecords_GetStageEntry(country, column)->lengthKm;
    rating[1] = FrontendRecords_GetStageEntry(country, column)->lengthTenths;
    if (rating[1] == 0)
        sprintf(CFrontend::m_stringDest, g_str0x00524d2c, CFrontend::GetTextString(0x18c), rating[0]);
    else
        sprintf(CFrontend::m_stringDest, g_str0x00524d20, CFrontend::GetTextString(0x18c), rating[0], rating[1]);
    strcat(CFrontend::m_stringDest, g_str0x00524d1c);
    Font_DrawText(1, CFrontend::m_stringDest, x, y, (int *)g_colourText0x0052496c, 0x21);
    y += (int)(g_pGraphics->resY * 0x10) / 0x1e0;
    sprintf(CFrontend::m_stringDest, g_str0x00524d14, CFrontend::GetTextString(0x18d));
    Font_DrawText(1, CFrontend::m_stringDest, x, y, (int *)g_colourText0x0052496c, 0x21);
    y += (int)(g_pGraphics->resY * 0x10) / 0x1e0;
    width = 0;
    limit = FrontendRecords_GetStageEntryPayload(&table, country, column) & 0xff;
    if (limit >= 2)
        limit = 2;
    for (i = 0; i < limit; i++) {
        sprintf(CFrontend::m_stringDest, g_str0x00524d0c, table[i].percent,
                CFrontend::GetTextString(table[i].surface + 0x18e));
        Font_DrawText(1, CFrontend::m_stringDest, x, y, (int *)g_colourText0x0052496c, 0x21);
        y += (int)(g_pGraphics->resY * 0x10) / 0x1e0;
        n = Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
        if (n > width)
            width = n;
    }
    time = CGameInfo::GetGameInfoFieldA4Address()->rallyStageRecordTimes[
               (RallyDataCountryIndex() & 0xff) * 0xb + pMenu->cursor].value >> 7 & 0xffff;
    sprintf(CFrontend::m_stringDest, g_loadRecordTimeFormat, time / 6000,
            (int)((time / 100) % 0x3c), time % 100);
    Font_DrawText(1, CFrontend::m_stringDest,
                  (int)(g_pGraphics->resX * 0x3a) / 0x280 + (int)(g_pGraphics->resX * 0x19f) / 0x280,
                  (int)(g_pGraphics->resY * 0x4b) / 0x1e0 +
                      ((int)(g_pGraphics->resY * 8) / 0x1e0) * 0x24,
                  (int *)g_colourText0x0052496c, 0x21);
    sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue,
            CGameInfo::GetGameInfoFieldA4Address()->rallyStageRecordTimes[
                (RallyDataCountryIndex() & 0xff) * 0xb + pMenu->cursor].ident);
    Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x19f) / 0x280,
                  (int)(g_pGraphics->resY * 0x4b) / 0x1e0 +
                      ((int)(g_pGraphics->resY * 8) / 0x1e0) * 0x24,
                  (int *)g_colourText0x0052496c, 0x21);
    Font_DrawText(1, CFrontend::GetTextString(0x193), (int)(g_pGraphics->resX * 0x19f) / 0x280,
                  (int)(g_pGraphics->resY * 0x4b) / 0x1e0 +
                      ((int)(g_pGraphics->resY * 8) / 0x1e0) * 0x24 -
                      (int)(g_pGraphics->resY * 0x10) / 0x1e0,
                  (int *)g_colourText0x0052496c, 0x21);
    count = 0;
    width = 0;
    for (n = 0; n < (CGameInfo::GetConfiguredPlayerCount() & 0xff); n++) {
        if (RallyData_IsDriverRecordUsable(n) == 0) {
            count++;
            if ((RallyData_GetDriverBestResults(n)->rallyStageTimes[RallyDataCountryIndex()][pMenu->cursor].header & 0x80) != 0) {
                time = RallyData_GetDriverBestResults(n)->rallyStageTimes[RallyDataCountryIndex()][pMenu->cursor].time;
                sprintf(CFrontend::m_stringDest, g_loadRecordTimeFormat, time / 6000,
                        (int)((time / 100) % 0x3c), time % 100);
            } else {
                sprintf(CFrontend::m_stringDest, g_noTimeText);
            }
            Font_DrawText(1, CFrontend::m_stringDest,
                          (int)(g_pGraphics->resX * 0x3a) / 0x280 + (int)(g_pGraphics->resX * 0x19f) / 0x280,
                          (int)(g_pGraphics->resY * 0x4b) / 0x1e0 +
                              (((int)(g_pGraphics->resY * 8) / 0x1e0) * 0x24 -
                               ((int)(g_pGraphics->resY * 0x10) / 0x1e0) * (count + 2)),
                          (int *)g_colourText0x0052496c, 0x21);
            sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue,
                    (char *)RallyData_GetRecord((BYTE)n));
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x19f) / 0x280,
                          (int)(g_pGraphics->resY * 0x4b) / 0x1e0 +
                              (((int)(g_pGraphics->resY * 8) / 0x1e0) * 0x24 -
                               ((int)(g_pGraphics->resY * 0x10) / 0x1e0) * (count + 2)),
                          (int *)g_colourText0x0052496c, 0x21);
        }
    }
    if (count > 0)
        Font_DrawText(1, CFrontend::GetTextString(0x194), (int)(g_pGraphics->resX * 0x19f) / 0x280,
                      (int)(g_pGraphics->resY * 0x4b) / 0x1e0 +
                          (((int)(g_pGraphics->resY * 8) / 0x1e0) * 0x24 -
                           ((int)(g_pGraphics->resY * 0x10) / 0x1e0) * (count + 3)),
                      (int *)g_colourText0x0052496c, 0x21);
    FrontendDraw_HelpText(CFrontend::GetTextString(0x57), 1);
}


// ---------------------------------------------------------------------------
// Rally report (cascade pass).

BYTE FrontendRecords_GetEventEntryPayload(StageSurfaceShare **out, int index);
const char *FrontendRecords_GetEventName(int index);
StageInfoEntry *FrontendRecords_GetEventEntry(int index);

extern char g_str0x00524d0c[];
extern char g_str0x00524d14[];
extern char g_str0x00524d1c[];
extern char g_str0x00524d20[];
extern char g_str0x00524d2c[];
extern char g_noTimeText[];

// Draws the rally report screen: the country banner of the selected rally, its
// name, the surface/distances of the event, the record time and holder of the
// selected stage, the class rows of the event and the times of every drawn
// driver.
// FUNCTION: CMR2 0x004dbd80
void FrontendMenu_DrawRallyReport(Menu *pMenu)
{
    int order[8] = { 6, 3, 1, 4, 0, 2, 5, 7 };
    StageSurfaceShare *table;
    int x;
    int y;
    int width;
    int shown;
    int i;
    int count;
    int w;
    BYTE entry[2];
    unsigned int time;

    RallyDataCountryIndex();
    g_frontendDrawRect[0] = (short)((int)(g_pGraphics->resX * 0x19f) / 0x280);
    g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 0x4b) / 0x1e0);
    g_frontendDrawRect[2] = CFrontend::m_pSetupRepBanners[order[pMenu->cursor]]->width;
    g_frontendDrawRect[3] = CFrontend::m_pSetupRepBanners[order[pMenu->cursor]]->height;
    Sprite_Queue((SpriteRect *)&CFrontend::m_pSetupRepBanners[order[pMenu->cursor]]->field_0x11c,
                 (SpriteRect *)g_frontendDrawRect, CFrontend::m_pSetupRepBanners[order[pMenu->cursor]], 1, 0, NULL, NULL,
                 (BYTE *)g_colourWhite0x00524968, 8);
    FrontendMap_DrawStageCellGrid(pMenu->cursor, 1);
    x = (int)(g_pGraphics->resX * 0x19f) / 0x280;
    y = (int)(g_pGraphics->resY * 0x4b) / 0x1e0 + (int)(g_pGraphics->resY * 0x50) / 0x1e0;
    sprintf(CFrontend::m_stringDest, g_strLabelText, CFrontend::GetTextString(0x18b), FrontendRecords_GetEventName(pMenu->cursor));
    Font_DrawText(1, CFrontend::m_stringDest, x, y, (int *)g_colourText0x0052496c, 0x21);
    y += (int)(g_pGraphics->resY * 0x10) / 0x1e0;
    entry[0] = FrontendRecords_GetEventEntry(pMenu->cursor)->lengthKm;
    entry[1] = FrontendRecords_GetEventEntry(pMenu->cursor)->lengthTenths;
    if (entry[1] == 0)
        sprintf(CFrontend::m_stringDest, g_str0x00524d2c, CFrontend::GetTextString(0x18c), entry[0]);
    else
        sprintf(CFrontend::m_stringDest, g_str0x00524d20, CFrontend::GetTextString(0x18c), entry[0], entry[1]);
    strcat(CFrontend::m_stringDest, g_str0x00524d1c);
    Font_DrawText(1, CFrontend::m_stringDest, x, y, (int *)g_colourText0x0052496c, 0x21);
    y += (int)(g_pGraphics->resY * 0x10) / 0x1e0;
    sprintf(CFrontend::m_stringDest, g_str0x00524d14, CFrontend::GetTextString(0x18d));
    Font_DrawText(1, CFrontend::m_stringDest, x, y, (int *)g_colourText0x0052496c, 0x21);
    width = 0;
    y += (int)(g_pGraphics->resY * 0x10) / 0x1e0;
    count = FrontendRecords_GetEventEntryPayload(&table, pMenu->cursor) & 0xff;
    if (count >= 2)
        count = 2;
    for (i = 0; i < count; i++) {
        sprintf(CFrontend::m_stringDest, g_str0x00524d0c, table[i].percent,
                CFrontend::GetTextString(table[i].surface + 0x18e));
        Font_DrawText(1, CFrontend::m_stringDest, x, y, (int *)g_colourText0x0052496c, 0x21);
        y += (int)(g_pGraphics->resY * 0x10) / 0x1e0;
        w = Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
        if (w > width)
            width = w;
    }
    time = CGameInfo::GetGameInfoFieldA4Address()->arcadeRecordTimes[
               (pMenu->cursor / 3) * 3 + pMenu->cursor % 3].value >> 7 & 0xffff;
    sprintf(CFrontend::m_stringDest, g_loadRecordTimeFormat, time / 6000, (int)((time / 100) % 0x3c), time % 100);
    Font_DrawText(1, CFrontend::m_stringDest,
                  (int)(g_pGraphics->resX * 0x3a) / 0x280 + (int)(g_pGraphics->resX * 0x19f) / 0x280,
                  (int)(g_pGraphics->resY * 0x4b) / 0x1e0 + (int)(g_pGraphics->resY * 8) / 0x1e0 * 0x24,
                  (int *)g_colourText0x0052496c, 0x21);
    sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue,
            CGameInfo::GetGameInfoFieldA4Address()->arcadeRecordTimes[(pMenu->cursor / 3) * 3 + pMenu->cursor % 3].ident);
    Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x19f) / 0x280,
                  (int)(g_pGraphics->resY * 0x4b) / 0x1e0 + (int)(g_pGraphics->resY * 8) / 0x1e0 * 0x24,
                  (int *)g_colourText0x0052496c, 0x21);
    Font_DrawText(1, CFrontend::GetTextString(0x193), (int)(g_pGraphics->resX * 0x19f) / 0x280,
                  (int)(g_pGraphics->resY * 0x4b) / 0x1e0 + (int)(g_pGraphics->resY * 8) / 0x1e0 * 0x24 -
                      (int)(g_pGraphics->resY * 0x10) / 0x1e0,
                  (int *)g_colourText0x0052496c, 0x21);
    i = 0;
    shown = 0;
    while (i < (int)(CGameInfo::GetConfiguredPlayerCount() & 0xff)) {
        if (RallyData_IsDriverRecordUsable(i) == 0) {
            shown++;
            if ((RallyData_GetDriverBestResults(i)->arcadeStageTimes[pMenu->cursor / 3][pMenu->cursor % 3].header & 0x80) != 0) {
                time = RallyData_GetDriverBestResults(i)->arcadeStageTimes[pMenu->cursor / 3][pMenu->cursor % 3].time;
                sprintf(CFrontend::m_stringDest, g_loadRecordTimeFormat, time / 6000, (int)((time / 100) % 0x3c),
                        time % 100);
            } else {
                sprintf(CFrontend::m_stringDest, g_noTimeText);
            }
            Font_DrawText(1, CFrontend::m_stringDest,
                          (int)(g_pGraphics->resX * 0x3a) / 0x280 + (int)(g_pGraphics->resX * 0x19f) / 0x280,
                          (int)(g_pGraphics->resY * 0x4b) / 0x1e0 + (int)(g_pGraphics->resY * 8) / 0x1e0 * 0x24 -
                              (int)(g_pGraphics->resY * 0x10) / 0x1e0 * (shown + 2),
                          (int *)g_colourText0x0052496c, 0x21);
            sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue, (char *)RallyData_GetRecord(i));
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x19f) / 0x280,
                          (int)(g_pGraphics->resY * 0x4b) / 0x1e0 + (int)(g_pGraphics->resY * 8) / 0x1e0 * 0x24 -
                              (int)(g_pGraphics->resY * 0x10) / 0x1e0 * (shown + 2),
                          (int *)g_colourText0x0052496c, 0x21);
        }
        i++;
    }
    if (shown > 0)
        Font_DrawText(1, CFrontend::GetTextString(0x194), (int)(g_pGraphics->resX * 0x19f) / 0x280,
                      (int)(g_pGraphics->resY * 0x4b) / 0x1e0 + (int)(g_pGraphics->resY * 8) / 0x1e0 * 0x24 -
                          (int)(g_pGraphics->resY * 0x10) / 0x1e0 * (shown + 3),
                      (int *)g_colourText0x0052496c, 0x21);
    FrontendDraw_HelpText(CFrontend::GetTextString(0x57), 1);
}


// ---------------------------------------------------------------------------
// Video options screen.
// ---------------------------------------------------------------------------
// GLOBAL: CMR2 0x00524dd0
char g_strSwitchOptionFormat[12] = "%s  < %d >";
// GLOBAL: CMR2 0x00524ddc
char g_strResModeFormat[20] = "%s (%dx%dx%d)...";
// GLOBAL: CMR2 0x00524df0
char g_strDeviceOptionFormat[12] = "%s (%s)...";
// GLOBAL: CMR2 0x00524dfc
char g_strEllipsisFormat[8] = "%s...";

// Draw callback of the video options screen: one row per item, the content of
// each row depending on the item value (driver list, resolution, device...).
// match 49%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004dfe20
void FrontendMenu_DrawGraphicsOptions(Menu *pMenu)
{
    char description[80];
    char name[80];
    short icon[4];
    BYTE *pColour;
    BYTE *pSelected;
    BYTE *pUnselected;
    BYTE *pShadow;
    MenuItem *pItem;
    int textId;
    int width;
    int y0;
    int x;
    int i;
    int j;

    icon[1] = 0;
    icon[0] = (int)(g_pGraphics->resX * 100) / 640;
    icon[2] = CFrontend::m_pAr640ATexture->width;
    icon[3] = CFrontend::m_pAr640ATexture->height;
    y0 = ((int)(g_pGraphics->resY * 8) / 480 + (int)(g_pGraphics->resY * 38) / 480 +
          (int)(g_pGraphics->resY * 384) / 480) / 2 -
         (int)(g_pGraphics->resY * 36) / 480 * pMenu->itemCount / 2;
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, (int)(g_pGraphics->resX * 24) / 640, (int)(g_pGraphics->resY * 38) / 480, 1, 2, NULL, -1);
    if (pMenu->cursor == 0) {
        pColour = g_colourWhite0x00524968;
        pShadow = g_colourShadowWhite0x00524974;
    } else {
        pColour = g_colourText0x0052496c;
        pShadow = g_colourShadowText0x00524978;
    }
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 99) / 640;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 282) / 640;

    g_frontendDrawRect[3] = 1;
    g_frontendDrawRect[1] = y0;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pShadow, 1);
    g_frontendDrawRect[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pColour, 1);
    i = 0;
    pItem = pMenu->items;
    if (pMenu->itemCount > 0) {
        do {
            x = (int)(g_pGraphics->resX * 0x7a) / 640;
            icon[1] = (int)(g_pGraphics->resY * 2) / 480 + (int)(g_pGraphics->resY * 18) / 480 + y0 +
                      ((int)(g_pGraphics->resY * 36) / 480 * i - CFrontend::m_pAr640ATexture->height / 2);
            if (pMenu->cursor == i) {
                pColour = g_colourWhite0x00524968;
                pSelected = g_colourWhite0x00524968;
                pUnselected = g_colourText0x0052496c;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, (SpriteRect *)icon,
                             CFrontend::m_pAr640ATexture, 1, 0, 0, NULL, pColour, 8);
            } else {
                if (pItem->enabled) {
                    pColour = g_colourText0x0052496c;
                    pSelected = g_colourWhite0x00524968;
                } else {
                    pColour = g_colourDim0x00524970;
                    pSelected = g_colourDim0x00524970;
                }
                pUnselected = pColour;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, (SpriteRect *)icon,
                             CFrontend::m_pAr640DTexture, 1, 0, 0, NULL, pColour, 8);
            }
            switch (pItem->value) {
            case 0:
                switch (CGameInfo::GetGraphicsOptionBits1To2()) {
                case 1:
                    sprintf(CFrontend::m_stringDest, g_strEllipsisFormat, CFrontend::GetTextString(0x1cd));
                    break;
                case 2:
                    sprintf(CFrontend::m_stringDest, g_strEllipsisFormat, CFrontend::GetTextString(0x1cc));
                    break;
                case 3:
                    sprintf(CFrontend::m_stringDest, g_strEllipsisFormat, CFrontend::GetTextString(0x1cb));
                    break;
                default:
                    sprintf(CFrontend::m_stringDest, g_strEllipsisFormat, CFrontend::GetTextString(0x1ce));
                    break;
                }
                Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 640,
                              (int)(g_pGraphics->resY * 0x18) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            case 1:
                strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(0x1cf));
                Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 640,
                              (int)(g_pGraphics->resY * 0x18) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                j = 0;
                if (pItem->min != 0) {
                    textId = 0x134;
                    do {
                        pColour = pSelected;
                        if (pItem->max != j)
                            pColour = pUnselected;
                        x = (int)(g_pGraphics->resX * 10) / 640 + x + Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
                        Font_DrawText(1, CFrontend::GetTextString(textId), x,
                                      (int)(g_pGraphics->resY * 0x18) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(textId));
                        j++;
                        textId--;
                    } while (j < pItem->min);
                }
                break;
            case 2:
                CGraphics::GetDisplayDeviceNames(CGraphics::GetSelectedDisplayDriverIndex(), description, name);
                sprintf(CFrontend::m_stringDest, g_strDeviceOptionFormat, CFrontend::GetTextString(0x1d0), description);
                Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 640,
                              (int)(g_pGraphics->resY * 0x18) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            case 3: {
                // PORT: the display's size, not that of the menus' screen.
                int displayX, displayY;
                MenuScreen_GetDisplaySize(&displayX, &displayY);
                sprintf(CFrontend::m_stringDest,
                        CInput::FormatString(g_strResModeFormat, CFrontend::GetTextString(0x1d1), displayX,
                                             displayY, g_pGraphics->depth));
                Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 640,
                              (int)(g_pGraphics->resY * 0x18) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            }
            case 4:
                strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(0x1d2));
                Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 640,
                              (int)(g_pGraphics->resY * 0x18) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                j = 0;
                if (pItem->min != 0) {
                    do {
                        if (pItem->max == j)
                            pColour = pSelected;
                        else
                            pColour = pUnselected;
                        x = (int)(g_pGraphics->resX * 10) / 640 + x + Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
                        if (j == 0)
                            textId = 0x1d6;
                        else if (j == 1)
                            textId = 0x1d5;
                        else
                            textId = 0x133;
                        Font_DrawText(1, CFrontend::GetTextString(textId), x,
                                      (int)(g_pGraphics->resY * 0x18) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(textId));
                        j++;
                    } while (j < pItem->min);
                }
                break;
            case 5:
                sprintf(CFrontend::m_stringDest, g_strSwitchOptionFormat, CFrontend::GetTextString(0x1d3),
                        Menu_GetItem(pMenu, 5)->max + 1);
                Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 640,
                              (int)(g_pGraphics->resY * 0x18) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            case 6:
            case 7:
            case 0xe:
                Font_DrawText(1, CFrontend::GetTextString(pItem->id), (int)(g_pGraphics->resX * 0x7a) / 640,
                              (int)(g_pGraphics->resY * 0x18) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            // PORT: the screen options (FrontendMenu_BuildScreenOptions).
            case 0xf:
                Font_DrawText(1, FrontendMenu_GetScreenOptionsLabel(), (int)(g_pGraphics->resX * 0x7a) / 640,
                              (int)(g_pGraphics->resY * 0x18) / 480 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            default:
                break;
            }
            if (pMenu->cursor == i + 1 || pMenu->cursor == i) {
                pColour = g_colourWhite0x00524968;
                pShadow = g_colourShadowWhite0x00524974;
            } else {
                pColour = g_colourText0x0052496c;
                pShadow = g_colourShadowText0x00524978;
            }
            g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 36) / 480 * (i + 1) + y0;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pShadow, 1);
            g_frontendDrawRect[1]++;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pColour, 1);
            pItem++;
            i++;
        } while (i < pMenu->itemCount);
    }
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}


// Value formats of the item labels of the settings list pages: a plain
// number, the OFF state and the seconds of a timed option, and the text form.
// GLOBAL: CMR2 0x00524d6c
char g_strItemValueFmt[12] = "%s < %d >";
// GLOBAL: CMR2 0x00524d78
char g_strItemOffFmt[12] = "%s < OFF >";
// GLOBAL: CMR2 0x00524d84
char g_strItemSecondsFmt[12] = "%s < %ds >";
// GLOBAL: CMR2 0x00524d90
char g_strItemTextFmt[12] = "%s < %s >";

// Draws a settings list page: the menu path, the framed title and one row per
// menu item with the label built from the item value and the separator line
// under the row.
// FUNCTION: CMR2 0x004dce00
void FrontendMenu_DrawNetworkSessionDetails(Menu *pMenu)
{
    SpriteRect rect;
    MenuItem *pItem;
    BYTE *pColour;
    BYTE *pLineColour;
    BYTE *pLineShadow;
    int top;
    int blink;
    int i;

    rect.x = (int)(g_pGraphics->resX * 100) / 640;
    rect.y = 0;
    rect.w = CFrontend::m_pAr640ATexture->width;
    rect.h = CFrontend::m_pAr640ATexture->height;
    top = ((int)(g_pGraphics->resY * 8) / 0x1e0 + (int)(g_pGraphics->resY * 0x26) / 0x1e0 +
           (int)(g_pGraphics->resY * 0x180) / 0x1e0) / 2 -
          ((int)(g_pGraphics->resY * 0x24) / 0x1e0 * pMenu->itemCount) / 2;
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, 2, NULL, -1);
    if (pMenu->cursor == 0) {
        pLineColour = g_colourWhite0x00524968;
        pLineShadow = g_colourShadowWhite0x00524974;
    } else {
        pLineColour = g_colourText0x0052496c;
        pLineShadow = g_colourShadowText0x00524978;
    }
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 99) / 640;
    g_frontendDrawRect[1] = top;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 0x11a) / 0x280;
    g_frontendDrawRect[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineShadow, 1);
    g_frontendDrawRect[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineColour, 1);
    i = 0;
    pItem = pMenu->items;
    if (pMenu->itemCount > 0) {
        do {
            rect.y = (int)(g_pGraphics->resY * 2) / 0x1e0 + (int)(g_pGraphics->resY * 0x12) / 0x1e0 + top +
                     ((int)(g_pGraphics->resY * 0x24) / 0x1e0 * i -
                      CFrontend::m_pAr640ATexture->height / 2);
            if (pMenu->cursor == i) {
                pColour = g_colourWhite0x00524968;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, &rect,
                             CFrontend::m_pAr640ATexture, 1, 0, 0, NULL, g_colourWhite0x00524968, 8);
            } else {
                pColour = g_colourText0x0052496c;
                if (pItem->enabled == 0)
                    pColour = g_colourDim0x00524970;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, &rect,
                             CFrontend::m_pAr640DTexture, 1, 0, 0, NULL, pColour, 8);
            }
            switch (pItem->value) {
            case 0:
                sprintf(CFrontend::m_stringDest, g_strLabelText, CFrontend::GetTextString(0x1bf),
                        g_sessionName);
                blink = (int)CMain::GetFrameDelta() % 20;
                if (pMenu->cursor == i && blink > 9)
                    strcat(CFrontend::m_stringDest, g_strTextCursor);
                Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 0x280,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            case 1:
                sprintf(CFrontend::m_stringDest, g_strItemTextFmt, CFrontend::GetTextString(0x1c0),
                        CGameInfo::GetCodeEntryText(Menu_GetItem(pMenu, 1)->max));
                Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 0x280,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            case 2:
                if (Menu_GetItem(pMenu, 2)->max > 0) {
                    sprintf(CFrontend::m_stringDest, g_strItemSecondsFmt, CFrontend::GetTextString(0x1c1),
                            Menu_GetItem(pMenu, 2)->max - 1);
                } else {
                    sprintf(CFrontend::m_stringDest, g_strItemOffFmt, CFrontend::GetTextString(0x1c1));
                }
                Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 0x280,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            case 3:
                sprintf(CFrontend::m_stringDest, g_strItemValueFmt, CFrontend::GetTextString(0x1c2),
                        Menu_GetItem(pMenu, 3)->max + 2);
                Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 0x280,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            case 4:
                sprintf(CFrontend::m_stringDest, g_strLabelSpacedText, CFrontend::GetTextString(0x1c3),
                        g_sessionPassword);
                blink = (int)CMain::GetFrameDelta() % 20;
                if (pMenu->cursor == i && blink > 9)
                    strcat(CFrontend::m_stringDest, g_strTextCursor);
                Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 0x280,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            case 5:
                Font_DrawText(1, CFrontend::GetTextString(0x1c4), (int)(g_pGraphics->resX * 0x7a) / 0x280,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            default:
                Font_DrawText(1, CFrontend::GetTextString(pItem->id), (int)(g_pGraphics->resX * 0x7a) / 0x280,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            }
            if (pMenu->cursor == i + 1 || pMenu->cursor == i) {
                pLineColour = g_colourWhite0x00524968;
                pLineShadow = g_colourShadowWhite0x00524974;
            } else {
                pLineColour = g_colourText0x0052496c;
                pLineShadow = g_colourShadowText0x00524978;
            }
            g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 0x24) / 0x1e0 * (i + 1) + top;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineShadow, 1);
            g_frontendDrawRect[1]++;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineColour, 1);
            i++;
            pItem++;
        } while (i < pMenu->itemCount);
    }
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}


// Player-count labels of a session: "1 " .. "10 " (the split-screen entry
// uses g_strFlagSS, at 0x00524dbc).
// GLOBAL: CMR2 0x005250d0
char g_strSessionPlayers1[4] = "1 ";
// GLOBAL: CMR2 0x005250cc
char g_strSessionPlayers2[4] = "2 ";
// GLOBAL: CMR2 0x005250c8
char g_strSessionPlayers3[4] = "3 ";
// GLOBAL: CMR2 0x005250c4
char g_strSessionPlayers4[4] = "4 ";
// GLOBAL: CMR2 0x005250c0
char g_strSessionPlayers5[4] = "5 ";
// GLOBAL: CMR2 0x005250bc
char g_strSessionPlayers6[4] = "6 ";
// GLOBAL: CMR2 0x005250b8
char g_strSessionPlayers7[4] = "7 ";
// GLOBAL: CMR2 0x005250b4
char g_strSessionPlayers8[4] = "8 ";
// GLOBAL: CMR2 0x005250b0
char g_strSessionPlayers9[4] = "9 ";
// GLOBAL: CMR2 0x005250ac
char g_strSessionPlayers10[4] = "10 ";

// Draws the session setup screen: the breadcrumb and play time, the row
// highlight strip, one row per item (its label, plus for the player count the
// allowed player numbers) and the carousel.
// FUNCTION: CMR2 0x004e77c0
void FrontendMenu_DrawNetworkPlayerSetup(Menu *pMenu)
{
    SpriteRect rect;
    char *names[11];
    char *text[3];
    MenuItem *pItem;
    char **pName;
    BYTE *pColour;
    BYTE *pLineColour;
    BYTE *pLineShadow;
    int x;
    BYTE flags;
    int index;
    int j;
    int top;
    int i;

    rect.x = (int)(g_pGraphics->resX * 100) / 640;
    rect.y = 0;
    rect.w = CFrontend::m_pAr640ATexture->width;
    rect.h = CFrontend::m_pAr640ATexture->height;
    names[0] = g_strSessionPlayers1;
    names[1] = g_strSessionPlayers2;
    names[2] = g_strSessionPlayers3;
    names[3] = g_strSessionPlayers4;
    names[4] = g_strSessionPlayers5;
    names[5] = g_strSessionPlayers6;
    names[6] = g_strSessionPlayers7;
    names[7] = g_strSessionPlayers8;
    names[8] = g_strSessionPlayers9;
    names[9] = g_strSessionPlayers10;
    names[10] = g_strFlagSS;
    text[0] = CFrontend::GetTextString(0x12);
    text[1] = CFrontend::GetTextString(0x1d9);
    text[2] = CFrontend::GetTextString(0x1da);
    FrontendDraw_Breadcrumb((int)(g_pGraphics->resX * 0x18) / 0x280, (int)(g_pGraphics->resY * 0x26) / 0x1e0, text, 3);
    top = ((int)(g_pGraphics->resY * 8) / 0x1e0 + (int)(g_pGraphics->resY * 0x26) / 0x1e0 +
           (int)(g_pGraphics->resY * 0x180) / 0x1e0) / 2 -
          ((int)(g_pGraphics->resY * 0x24) / 0x1e0 * pMenu->itemCount) / 2;
    FrontendDraw_PlayTime();
    if (pMenu->cursor == 0) {
        pLineColour = g_colourWhite0x00524968;
        pLineShadow = g_colourShadowWhite0x00524974;
    } else {
        pLineColour = g_colourText0x0052496c;
        pLineShadow = g_colourShadowText0x00524978;
    }
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 99) / 640;
    g_frontendDrawRect[1] = top;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 0x11a) / 0x280;
    g_frontendDrawRect[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineShadow, 1);
    g_frontendDrawRect[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineColour, 1);
    i = 0;
    pItem = pMenu->items;
    if (pMenu->itemCount > 0) {
        do {
            x = (int)(g_pGraphics->resX * 0x7a) / 0x280;
            rect.y = (int)(g_pGraphics->resY * 2) / 0x1e0 + (int)(g_pGraphics->resY * 0x12) / 0x1e0 + top +
                     ((int)(g_pGraphics->resY * 0x24) / 0x1e0 * i - CFrontend::m_pAr640ATexture->height / 2);
            if (pMenu->cursor == i) {
                pColour = g_colourWhite0x00524968;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, &rect,
                             CFrontend::m_pAr640ATexture, 1, 0, 0, NULL, g_colourWhite0x00524968, 8);
            } else {
                pColour = g_colourText0x0052496c;
                if (pItem->enabled == 0)
                    pColour = g_colourDim0x00524970;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, &rect,
                             CFrontend::m_pAr640DTexture, 1, 0, 0, NULL, pColour, 8);
            }
            switch (pItem->value) {
            case 0:
            {
                pColour = (pMenu->cursor == i) ? g_colourWhite0x00524968 : g_colourText0x0052496c;
                Font_DrawText(1, CFrontend::GetTextString(0x1f7), x,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                x = x + (int)(g_pGraphics->resX * 10) / 0x280
                    + Font_GetTextWidth(1, (BYTE *)CFrontend::GetTextString(0x1f7));
                for (j = 0; j < Menu_GetItem(pMenu, 0)->min; j++) {
                    index = j;
                    if ((Menu_GetItem(pMenu, 0)->min == 5 && j == 4) ||
                        (Menu_GetItem(pMenu, 0)->min == 9 && j == 8))
                        index = 10;
                    flags = CGameInfo::GetStageOptionState(Menu_GetItem(pMenu, 1)->max, index);
                    if (pMenu->cursor == i) {
                        if ((flags & 1) != 0)
                            pColour = (pItem->max == j) ? g_colourWhite0x00524968 : g_colourText0x0052496c;
                        else
                            pColour = (pItem->max == j) ? g_colourText0x0052496c : g_colourDim0x00524970;
                    } else {
                        pColour = ((flags & 1) != 0) ? g_colourText0x0052496c : g_colourDim0x00524970;
                    }
                    Font_DrawText(1, names[index], x,
                                  (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                    x += Font_GetTextWidth(1, (BYTE *)names[j]);
                }
                break;
            }
            case 1:
                sprintf(CFrontend::m_stringDest, g_strLabelText, CFrontend::GetTextString(0x30),
                        CFrontend::GetTextString(Menu_GetItem(pMenu, 1)->max + 0x27));
                Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 0x280,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            case 2:
            case 3:
                Font_DrawText(1, CFrontend::GetTextString(pItem->id), (int)(g_pGraphics->resX * 0x7a) / 0x280,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            }
            if (pMenu->cursor == i + 1 || pMenu->cursor == i) {
                pLineColour = g_colourWhite0x00524968;
                pLineShadow = g_colourShadowWhite0x00524974;
            } else {
                pLineColour = g_colourText0x0052496c;
                pLineShadow = g_colourShadowText0x00524978;
            }
            g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 0x24) / 0x1e0 * (i + 1) + top;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineShadow, 1);
            g_frontendDrawRect[1]++;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineColour, 1);
            i++;
            pItem++;
        } while (i < pMenu->itemCount);
    }
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}


// FUNCTION: CMR2 0x004e8500
void FrontendMenu_DrawNetworkRallySetup(Menu *pMenu)
{
    short rect[4];
    int i;
    int top;
    char *texts[3];
    char buffer[100];
    MenuItem *pItem;
    char *pText;
    BYTE *pColour;
    BYTE *pLineColour;
    BYTE *pLineShadow;
    int x;

    rect[0] = (int)(g_pGraphics->resX * 100) / 640;
    rect[1] = 0;
    rect[2] = CFrontend::m_pAr640ATexture->width;
    rect[3] = CFrontend::m_pAr640ATexture->height;
    texts[0] = CFrontend::GetTextString(0x12);
    texts[1] = CFrontend::GetTextString(0x1d9);
    texts[2] = CFrontend::GetTextString(0x1dd);
    FrontendDraw_Breadcrumb((int)(g_pGraphics->resX * 0x18) / 0x280,
                            (int)(g_pGraphics->resY * 0x26) / 0x1e0, texts, 3);
    top = ((int)(g_pGraphics->resY * 8) / 0x1e0 + (int)(g_pGraphics->resY * 0x26) / 0x1e0 +
           (int)(g_pGraphics->resY * 0x180) / 0x1e0) / 2 -
          ((int)(g_pGraphics->resY * 0x24) / 0x1e0 * pMenu->itemCount) / 2;
    FrontendDraw_PlayTime();
    if (pMenu->cursor == 0) {
        pLineColour = g_colourWhite0x00524968;
        pLineShadow = g_colourShadowWhite0x00524974;
    } else {
        pLineColour = g_colourText0x0052496c;
        pLineShadow = g_colourShadowText0x00524978;
    }
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 99) / 640;
    g_frontendDrawRect[1] = top;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 0x11a) / 0x280;
    g_frontendDrawRect[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineShadow, 1);
    g_frontendDrawRect[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineColour, 1);
    i = 0;
    pItem = pMenu->items;
    if (pMenu->itemCount > 0) {
        do {
            x = (int)(g_pGraphics->resX * 0x7a) / 0x280;
            rect[1] = (int)(g_pGraphics->resY * 2) / 0x1e0 + (int)(g_pGraphics->resY * 0x12) / 0x1e0 + top +
                      ((int)(g_pGraphics->resY * 0x24) / 0x1e0 * i - CFrontend::m_pAr640ATexture->height / 2);
            if (pMenu->cursor == i) {
                pColour = g_colourWhite0x00524968;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, (SpriteRect *)rect,
                             CFrontend::m_pAr640ATexture, 1, 0, NULL, NULL, pColour, 8);
            } else {
                pColour = g_colourText0x0052496c;
                if (pItem->enabled == 0)
                    pColour = g_colourDim0x00524970;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, (SpriteRect *)rect,
                             CFrontend::m_pAr640DTexture, 1, 0, NULL, NULL, pColour, 8);
            }
            switch (pItem->value) {
            case 1:
                sprintf(CFrontend::m_stringDest, g_strLabelText, CFrontend::GetTextString(0x31),
                        &g_unk0x00818d84[Menu_GetItem(pMenu, 1)->max * 3]);
                Font_DrawText(1, CFrontend::m_stringDest, x,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            case 0:
                sprintf(CFrontend::m_stringDest, g_strLabelText, CFrontend::GetTextString(0x30),
                        CFrontend::GetTextString(Menu_GetItem(pMenu, 0)->max + 0x27));
                Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 0x280,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            case 2:
                if (Menu_GetItem(pMenu, 2)->max != 0) {
                    sprintf(buffer, CFrontend::GetTextString(0x1e0), Menu_GetItem(pMenu, 2)->max + 4);
                    sprintf(CFrontend::m_stringDest, g_strLabelText, CFrontend::GetTextString(0x1de), buffer);
                } else {
                    sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x1e1));
                }
                Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 0x280,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            case 3:
            case 4:
                Font_DrawText(1, CFrontend::GetTextString(pItem->id), (int)(g_pGraphics->resX * 0x7a) / 0x280,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            }
            if (pMenu->cursor == i + 1 || pMenu->cursor == i) {
                pLineColour = g_colourWhite0x00524968;
                pLineShadow = g_colourShadowWhite0x00524974;
            } else {
                pLineColour = g_colourText0x0052496c;
                pLineShadow = g_colourShadowText0x00524978;
            }
            g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 0x24) / 0x1e0 * (i + 1) + top;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineShadow, 1);
            g_frontendDrawRect[1]++;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineColour, 1);
            pItem++;
            i++;
        } while (i < pMenu->itemCount);
    }
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}


// Draws the race-settings list: breadcrumb, framed title, one row per item
// (row sprite plus its label/value) and the carousel arrows.
// match 85%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004e90f0
void FrontendMenu_DrawNetworkExtendedStageSetup(Menu *pMenu)
{
    SpriteRect rect;
    char *text[3];
    BYTE *pColour;
    BYTE *pLineColour;
    BYTE *pLineShadow;
    MenuItem *pItem;
    char buffer[100];
    int top;
    int i;

    rect.x = (int)(g_pGraphics->resX * 100) / 640;
    rect.y = 0;
    rect.w = CFrontend::m_pAr640ATexture->width;
    rect.h = CFrontend::m_pAr640ATexture->height;
    text[0] = CFrontend::GetTextString(0x12);
    text[1] = CFrontend::GetTextString(0x1d9);
    text[2] = CFrontend::GetTextString(0xd8);
    FrontendDraw_Breadcrumb((int)(g_pGraphics->resX * 0x18) / 0x280, (int)(g_pGraphics->resY * 0x26) / 0x1e0, text, 3);
    top = ((int)(g_pGraphics->resY * 8) / 0x1e0 + (int)(g_pGraphics->resY * 0x26) / 0x1e0 +
           (int)(g_pGraphics->resY * 0x180) / 0x1e0) / 2 -
          ((int)(g_pGraphics->resY * 0x24) / 0x1e0 * pMenu->itemCount) / 2;
    FrontendDraw_PlayTime();
    if (pMenu->cursor == 0) {
        pLineColour = g_colourWhite0x00524968;
        pLineShadow = g_colourShadowWhite0x00524974;
    } else {
        pLineColour = g_colourText0x0052496c;
        pLineShadow = g_colourShadowText0x00524978;
    }
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 99) / 640;
    g_frontendDrawRect[1] = top;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 0x11a) / 0x280;
    g_frontendDrawRect[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineShadow, 1);
    g_frontendDrawRect[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineColour, 1);
    i = 0;
    pItem = pMenu->items;
    if (pMenu->itemCount > 0) {
        do {
            rect.y = (int)(g_pGraphics->resY * 2) / 0x1e0 + (int)(g_pGraphics->resY * 0x12) / 0x1e0 + top +
                     ((int)(g_pGraphics->resY * 0x24) / 0x1e0 * i - CFrontend::m_pAr640ATexture->height / 2);
            if (pMenu->cursor == i) {
                pColour = g_colourWhite0x00524968;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, &rect, CFrontend::m_pAr640ATexture, 1, 0, 0, NULL, pColour, 8);
            } else {
                if (pItem->enabled != 0)
                    pColour = g_colourText0x0052496c;
                else
                    pColour = g_colourDim0x00524970;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, &rect, CFrontend::m_pAr640DTexture, 1, 0, 0, NULL, pColour, 8);
            }
            switch (pItem->value) {
            case 0:
                sprintf(CFrontend::m_stringDest, g_strLabelText, CFrontend::GetTextString(0x30),
                        CFrontend::GetTextString(g_unk0x00818ed4[Menu_GetItem(pMenu, 0)->max] + 0x27));
                Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 0x280,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            case 1:
                if (Menu_GetItem(pMenu, 1)->max != 0) {
                    sprintf(buffer, CFrontend::GetTextString(0x1e0), Menu_GetItem(pMenu, 1)->max + 4);
                    sprintf(CFrontend::m_stringDest, g_strLabelText, CFrontend::GetTextString(0x1de), buffer);
                } else {
                    strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(0x1e1));
                }
                Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 0x280,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            case 2:
            case 3:
                Font_DrawText(1, CFrontend::GetTextString(pItem->id), (int)(g_pGraphics->resX * 0x7a) / 0x280,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
                break;
            default:
                goto next;
            }
        next:
            if (pMenu->cursor == i + 1 || pMenu->cursor == i) {
                pLineColour = g_colourWhite0x00524968;
                pLineShadow = g_colourShadowWhite0x00524974;
            } else {
                pLineColour = g_colourText0x0052496c;
                pLineShadow = g_colourShadowText0x00524978;
            }
            g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 0x24) / 0x1e0 * (i + 1) + top;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineShadow, 1);
            g_frontendDrawRect[1]++;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineColour, 1);
            i++;
            pItem++;
        } while (i < pMenu->itemCount);
    }
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

// Draws the stage list screen: the breadcrumbs, the separator bar and one row
// per item (the row sprite, its label and the value strings of the item).
// FUNCTION: CMR2 0x004d8ed0
void FrontendMenu_DrawChampionshipEntry(Menu *pMenu)
{
    SpriteRect rect;
    char label[4];
    char *names[2];
    MenuItem *pItem;
    BYTE *pLineColour;
    BYTE *pLineShadow;
    int y;
    int x;
    int i;
    int j;

    rect.x = (int)(g_pGraphics->resX * 100) / 640;
    rect.y = 0;
    rect.w = CFrontend::m_pAr640ATexture->width;
    rect.h = CFrontend::m_pAr640ATexture->height;
    FrontendDraw_PlayTime();
    sprintf(g_str0x008188a4, CFrontend::GetTextString(0xdc),
            (BYTE)CGameInfo::GetConfiguredPlayerCount() - (BYTE)FrontendProfile_GetPlayersRemaining());
    strcpy(label, (char *)RallyData_GetRecord((BYTE)(0xff - (BYTE)FrontendProfile_GetPlayersRemaining() + (BYTE)CGameInfo::GetConfiguredPlayerCount())));
    names[0] = g_str0x008188a4;
    names[1] = label;
    FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, 3, names, 2);
    y = ((int)(g_pGraphics->resY * 56) / 480 + (int)(g_pGraphics->resY * 374) / 480) / 2 -
        ((int)(g_pGraphics->resY * 36) / 480 * pMenu->itemCount) / 2;
    if (pMenu->cursor == 0) {
        pLineColour = g_colourWhite0x00524968;
        pLineShadow = g_colourShadowWhite0x00524974;
    } else {
        pLineColour = g_colourText0x0052496c;
        pLineShadow = g_colourShadowText0x00524978;
    }
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 99) / 640;
    g_frontendDrawRect[1] = y;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 282) / 640;
    g_frontendDrawRect[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineShadow, 1);
    g_frontendDrawRect[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineColour, 1);
    i = 0;
    pItem = pMenu->items;
    if (pMenu->itemCount > 0) {
        do {
            rect.y = (int)(g_pGraphics->resY * 20) / 480 + y +
                     ((int)(g_pGraphics->resY * 36) / 480 * i - CFrontend::m_pAr640ATexture->height / 2);
            if (pMenu->cursor == i) {
                pLineColour = g_colourWhite0x00524968;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, &rect,
                             CFrontend::m_pAr640ATexture, 1, 0, 0, NULL, pLineColour, 8);
            } else {
                pLineColour = g_colourText0x0052496c;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, &rect,
                             CFrontend::m_pAr640DTexture, 1, 0, 0, NULL, pLineColour, 8);
            }
            if (pItem->value == 0x88) {
                strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pItem->id));
                x = (int)(g_pGraphics->resX * 0x7a) / 0x280;
                Font_DrawText(1, CFrontend::m_stringDest, x,
                              (int)(g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pLineColour, 0x11);
                for (j = 0; j < pItem->min; j++) {
                    if (pItem->max == j)
                        pLineColour = g_colourWhite0x00524968;
                    else
                        pLineColour = g_colourText0x0052496c;
                    x += (int)(g_pGraphics->resX * 10) / 640 + Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
                    Font_DrawText(1, CFrontend::GetTextString(j + 0x131), x,
                                  (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1], (int *)pLineColour, 0x11);
                    strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(j + 0x131));
                }
            }
            if (pMenu->cursor == i || pMenu->cursor == i + 1) {
                pLineColour = g_colourWhite0x00524968;
                pLineShadow = g_colourShadowWhite0x00524974;
            } else {
                pLineColour = g_colourText0x0052496c;
                pLineShadow = g_colourShadowText0x00524978;
            }
            g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 0x24) / 0x1e0 * (i + 1) + y;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineShadow, 1);
            g_frontendDrawRect[1]++;
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineColour, 1);
            i++;
            pItem++;
        } while (i < pMenu->itemCount);
    }
    FrontendDraw_ScrollerRow(FrontendScroller_GetRouteEntryScroller(), 0);
    FrontendDraw_HelpText(CFrontend::GetTextString(0x57), 1);
}


// ---------------------------------------------------------------------------
// Championship rally screen: the header panel with the rally name, the
// gearbox and the driver list, the keyboard control labels, the stage rows
// (sprite + text) and the car preview, then the carousel.
// ---------------------------------------------------------------------------
extern char g_keypadFormat[];   // 0x516940 "%s %s" (Input.cpp)
extern char g_str0x00519fb8[];  // 0x519fb8 "%s_" (GameMenus.cpp)
BYTE RallyData_GetDriverAutomaticGearbox(BYTE param1);
char *NetworkChat_GetLine(int index);
unsigned int NetPlayers_GetPlayerFlag5(int index);

// GLOBAL: CMR2 0x00524d9c
char g_str0x00524d9c[12] = "%s: < %s >";
// GLOBAL: CMR2 0x00524da8
char g_str0x00524da8[12] = "%s\n%s, %s";

// match 65%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004dd4b0
void FrontendMenu_DrawNetworkSessionSetup(Menu *pMenu)
{
    BYTE colour[4];
    BYTE *pColour;
    BYTE *pChatColour;
    int i;
    int j;
    MenuItem *pItem;
    short panel[4];
    SpriteRect rect;
    char *texts[4];
    int order[8];
    char *labels[11];
    char buf[256];
    int count;
    int top;
    BYTE *pLineColour;
    BYTE *pLineShadow;
    int next;
    BYTE *pYes;
    BYTE *pNo;

    colour[0] = 0xff;
    colour[1] = 0xff;
    colour[2] = 0xff;
    colour[3] = 0x20;
    panel[0] = (short)((int)(g_pGraphics->resX * 20) / 640);
    labels[0] = g_strNum1;
    labels[1] = g_keypad2;
    labels[2] = g_keypad3;
    labels[3] = g_keypad4;
    labels[4] = g_strNum5;
    labels[5] = g_keypad6;
    labels[6] = g_keypad7;
    labels[7] = g_keypad8;
    labels[8] = g_keypad9;
    labels[9] = g_strNum10;
    labels[10] = g_strFlagSS;
    panel[1] = (short)((int)(g_pGraphics->resY * 55) / 480);
    panel[2] = (short)((int)(g_pGraphics->resX * 425) / 640);
    panel[3] = (short)((int)(g_pGraphics->resY * 110) / 480);
    rect.x = (short)((int)(g_pGraphics->resX * 100) / 640);
    rect.y = 0;
    rect.w = CFrontend::m_pAr640ATexture->width;
    rect.h = CFrontend::m_pAr640ATexture->height;
    order[0] = 6;
    order[1] = 3;
    order[2] = 1;
    order[3] = 4;
    order[4] = 0;
    order[5] = 2;
    order[6] = 5;
    order[7] = 7;

    texts[0] = CFrontend::GetTextString(0x12);
    texts[1] = CGameInfo::GetCodeEntryText((CGameInfo::GetConfiguredGameMode() & 0xff) - 8);
    switch (CGameInfo::GetConfiguredGameMode() & 0xff) {
    case 8:
        sprintf(buf, CRegKey::m_regKeyPathFormatValue,
                CFrontend::GetTextString((RallyDataCountryIndex() & 0xff) + 0x27));
        break;
    case 9:
    case 10:
        sprintf(buf, g_keypadFormat,
                CFrontend::GetTextString((RallyDataCountryIndex() & 0xff) + 0x27),
                labels[RallyDataStageIndex()]);
        break;
    case 11:
    case 12:
        sprintf(buf, CRegKey::m_regKeyPathFormatValue,
                CFrontend::GetTextString(order[(RallyData_GetSelectionBits10To11() & 0xff) * 3 +
                                               (RallyData_GetSelectionBits12To13() & 0xff)] + 0x27));
        break;
    }
    texts[2] = buf;
    if (Network_GetSessionStateFlag() != 0)
        texts[3] = CFrontend::GetTextString(0x3d);
    else
        texts[3] = CFrontend::GetTextString(0x1c5);
    FrontendDraw_Breadcrumb(PATH_X(), PATH_Y(), texts, 4);
    if (g_lobbyChatSelected != 0) {
        colour[3] = 0x20;
        pChatColour = g_colourWhite0x00524968;
    } else {
        colour[3] = 0x10;
        pChatColour = g_colourText0x0052496c;
    }
    FrontendDraw_PlayTime();
    Sprite_FillRect(&g_pGraphics->field309_0x150, panel, colour, 1);
    panel[1] = (short)(panel[1] + (g_pGraphics->resY * 10) / 480 + panel[3]);
    panel[3] = (short)((g_pGraphics->resY * 30) / 480);
    Sprite_FillRect(&g_pGraphics->field309_0x150, panel, colour, 1);

    sprintf(CFrontend::m_stringDest, g_str0x00524da8, (char *)RallyData_GetRecord(0),
            CFrontend::GetModeSpecificStageText(RallyData_GetDriverRecordSelectionValue(0)),
            RallyData_GetDriverAutomaticGearbox(0) != 0 ? g_strGearboxAuto : g_strGearboxManual);
    if (Network_GetSessionStateFlag() != 0) {
        Font_DrawText(1, CFrontend::m_stringDest, g_pGraphics->resX - (g_pGraphics->resX * 20) / 640, (g_pGraphics->resY * 100) / 480, (int *)g_colourWhite0x00524968, 0xc);
    } else {
        if (g_unk0x00818ce4 != 0)
            Font_DrawText(1, CFrontend::m_stringDest, g_pGraphics->resX - (g_pGraphics->resX * 20) / 640, (g_pGraphics->resY * 100) / 480, (int *)g_colourWhite0x00524968, 0xc);
        else
            Font_DrawText(1, CFrontend::m_stringDest, g_pGraphics->resX - (g_pGraphics->resX * 20) / 640, (g_pGraphics->resY * 100) / 480, (int *)g_colourText0x0052496c, 0xc);
    }

    i = 1;
    for (j = 0, count = 7; count != 0; count--, j++) {
        if (i < 8 && NetPlayers_IsPlayerPresent(j) != 0) {
            char *pName = NetPlayers_GetPlayerName(j);
            if (pName != NULL)
                sprintf(CFrontend::m_stringDest, g_str0x00524da8, pName,
                        CFrontend::GetModeSpecificStageText(NetPlayers_GetCarSelection(j)),
                        (BYTE)NetPlayers_GetPlayerFlag5(j) != 0 ? g_strGearboxAuto : g_strGearboxManual);
            else
                sprintf(CFrontend::m_stringDest, g_str0x00524da8, CMain::m_logFileBlankLine,
                        CFrontend::GetModeSpecificStageText(NetPlayers_GetCarSelection(j)),
                        (BYTE)NetPlayers_GetPlayerFlag5(j) != 0 ? g_strGearboxAuto : g_strGearboxManual);
            if (NetPlayers_GetPlayerFlag6(j) != 0)
                Font_DrawText(1, CFrontend::m_stringDest, g_pGraphics->resX - (g_pGraphics->resX * 20) / 640,
                              (g_pGraphics->resY * 50) / 480 + ((g_pGraphics->resY * 40) / 480) * i, (int *)g_colourWhite0x00524968, 0xc);
            else
                Font_DrawText(1, CFrontend::m_stringDest, g_pGraphics->resX - (g_pGraphics->resX * 20) / 640,
                              (g_pGraphics->resY * 50) / 480 + ((g_pGraphics->resY * 40) / 480) * i, (int *)g_colourText0x0052496c, 0xc);
            i++;
        }
    }

    for (j = 4, count = 0; j >= 0; j--, count++) {
        Font_DrawText(1, NetworkChat_GetLine(j), (g_pGraphics->resX * 35) / 640,
                      ((count + 3) * g_pGraphics->resY * 20) / 480, (int *)pChatColour, 9);
    }
    if (g_lobbyChatSelected != 0) {
        if ((unsigned int)CFrontend::GetFrontendTimestamp() % 20 > 9)
            sprintf(CFrontend::m_stringDest, g_str0x00519fb8,
                    (char *)FrontendNetwork_GetLobbyChatLine());
        else
            sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue,
                    (char *)FrontendNetwork_GetLobbyChatLine());
        Font_DrawText(1, CFrontend::m_stringDest, (g_pGraphics->resX * 35) / 640,
                      ((count + 4) * g_pGraphics->resY * 20) / 480, (int *)g_colourWhite0x00524968, 9);
    }

    top = (g_pGraphics->resY * 40) / 480 + panel[1];
    if (pMenu->cursor == 1) {
        pLineColour = g_colourWhite0x00524968;
        pLineShadow = g_colourShadowWhite0x00524974;
    } else {
        pLineColour = g_colourText0x0052496c;
        pLineShadow = g_colourShadowText0x00524978;
    }
    g_frontendDrawRect[0] = (short)((int)(g_pGraphics->resX * 99) / 640);
    g_frontendDrawRect[2] = (short)((int)(g_pGraphics->resX * 0x11a) / 640);

    g_frontendDrawRect[3] = 1;
    g_frontendDrawRect[1] = (short)top;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineShadow, 1);
    g_frontendDrawRect[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineColour, 1);

    i = 0;
    pItem = pMenu->items + 1;
    while (i < pMenu->itemCount - 1) {
        rect.y = (short)((g_pGraphics->resY * 2) / 0x1e0 + (g_pGraphics->resY * 0x12) / 0x1e0 + top +
                         (g_pGraphics->resY * 0x24) / 0x1e0 * i -
                         CFrontend::m_pAr640ATexture->height / 2);
        next = i + 1;
        if (pMenu->cursor == next) {
            pColour = g_colourWhite0x00524968;
            Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, &rect,
                         CFrontend::m_pAr640ATexture, 1, 0, 0, NULL, pColour, 8);
        } else {
            if (pItem->enabled != 0)
                pColour = g_colourText0x0052496c;
            else
                pColour = g_colourDim0x00524970;
            Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, &rect,
                         CFrontend::m_pAr640DTexture, 1, 0, 0, NULL, pColour, 8);
        }
        switch (pItem->value) {
        case 0:
            sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x1c6));
            break;
        case 1:
            sprintf(CFrontend::m_stringDest, g_str0x00524d9c, CFrontend::GetTextString(0x87),
                    CFrontend::GetTextString(g_filteredRouteIds[Menu_GetItem(pMenu, 1)->max] + 0x98));
            break;
        case 2:
            sprintf(CFrontend::m_stringDest, g_str0x00524d9c, CFrontend::GetTextString(0x88),
                    CFrontend::GetTextString(Menu_GetItem(pMenu, 2)->max + 0x131));
            break;
        case 3:
            if (Network_GetSessionStateFlag() != 0) {
                strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(0x3f));
                break;
            }
            // the yes/no pair: the current choice is drawn in white
            if (g_unk0x00818ce4 != 0) {
                pYes = g_colourWhite0x00524968;
                pNo = g_colourText0x0052496c;
            } else {
                pYes = g_colourText0x0052496c;
                pNo = g_colourWhite0x00524968;
            }
            Font_DrawText(1, CFrontend::GetTextString(0x1c7), (g_pGraphics->resX * 0x7a) / 0x280, (g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
            Font_DrawText(1, CFrontend::GetTextString(5),
                          (g_pGraphics->resX * 10) / 0x280 + (g_pGraphics->resX * 0x7a) / 0x280 +
                              Font_GetTextWidth(1, (BYTE *)CFrontend::GetTextString(0x1c7)),
                          (g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pYes, 0x11);
            Font_DrawText(1, CFrontend::GetTextString(4),
                          (g_pGraphics->resX * 0x14) / 0x280 + (g_pGraphics->resX * 0x7a) / 0x280 +
                              Font_GetTextWidth(1, (BYTE *)CFrontend::GetTextString(0x1c7)) +
                              Font_GetTextWidth(1, (BYTE *)CFrontend::GetTextString(5)),
                          (g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pNo, 0x11);
            goto line;
        case 4:
            Font_DrawText(1, CFrontend::GetTextString(pItem->id), (g_pGraphics->resX * 0x7a) / 0x280, (g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
            goto line;
        default:
            goto line;
        }
        Font_DrawText(1, CFrontend::m_stringDest, (g_pGraphics->resX * 0x7a) / 0x280, (g_pGraphics->resY * 0x18) / 0x1e0 + g_frontendDrawRect[1], (int *)pColour, 0x11);
    line:
        if (pMenu->cursor == next + 1 || pMenu->cursor == next) {
            pLineColour = g_colourWhite0x00524968;
            pLineShadow = g_colourShadowWhite0x00524974;
        } else {
            pLineColour = g_colourText0x0052496c;
            pLineShadow = g_colourShadowText0x00524978;
        }
        g_frontendDrawRect[1] = (short)((int)(g_pGraphics->resY * 0x24) / 0x1e0 * (i + 1) + top);
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineShadow, 1);
        g_frontendDrawRect[1]++;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineColour, 1);
        i = next;
        pItem++;
    }
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

// PORT: the screen options, a page of the graphics menu: the sizes of the
// HUD and of the menus ([video] hud_scale and menu_scale, in percent of the
// original's at the screen's height), the vertical sync and the fullscreen
// window. They are saved in opencmr2.ini when OK is chosen; Back leaves them
// as they were. The original's layout in pixels (a size of 0) stays an
// opencmr2.ini setting.

Menu g_screenOptionsMenu;

enum ScreenOptionsItem {
    SCREEN_ITEM_HUD,
    SCREEN_ITEM_MENU,
    SCREEN_ITEM_VSYNC,
    SCREEN_ITEM_FULLSCREEN,
    SCREEN_ITEM_OK = 0xe
};

enum ScreenOptionsText {
    SCREEN_TEXT_TITLE,
    SCREEN_TEXT_HUD,
    SCREEN_TEXT_MENU,
    SCREEN_TEXT_VSYNC,
    SCREEN_TEXT_FULLSCREEN,
    SCREEN_TEXT_COUNT
};

// In the game's character set (Windows-1252; Polish letters in place of
// some symbols: 0xb1 is l with a stroke).
static const char *const g_screenOptionsText[6][SCREEN_TEXT_COUNT] = {
    { "Screen", "HUD Size", "Menu Size", "Vertical Sync", "Full Screen" },
    { "\xc9" "cran", "Taille du HUD", "Taille des menus", "Synchro verticale", "Plein \xe9" "cran" },
    { "Pantalla", "Tama\xf1o del HUD", "Tama\xf1o de los men\xfas", "Sincronizaci\xf3n vertical", "Pantalla completa" },
    { "Schermo", "Dimensione HUD", "Dimensione menu", "Sincronia verticale", "Schermo intero" },
    { "Bildschirm", "HUD-Gr\xf6\xdf" "e", "Men\xfcgr\xf6\xdf" "e", "VSync", "Vollbild" },
    { "Ekran", "Rozmiar HUD", "Rozmiar menu", "Synchronizacja pionowa", "Pe\xb1ny ekran" },
};

// Sizes offered, in percent.
#define SCREEN_SIZE_MIN 50
#define SCREEN_SIZE_STEP 10
#define SCREEN_SIZE_MAX 150

// A text in the language the menus show: English, French, Spanish, Italian
// or German, or Polish for that edition (its only language).
static char *ScreenOptions_Text(int text)
{
    int language = CGameInfo::GetGameRegion() == 3 ? 5 : CFrontend::m_textLanguage;
    if (language > 5)
        language = 0;
    return (char *)g_screenOptionsText[language][text];
}

// The sizes on offer and the one of a percentage (the nearest).
static int ScreenOptions_SizeCount(int largest)
{
    if (largest > SCREEN_SIZE_MAX)
        largest = SCREEN_SIZE_MAX;
    if (largest < SCREEN_SIZE_MIN)
        largest = SCREEN_SIZE_MIN;
    return (largest - SCREEN_SIZE_MIN) / SCREEN_SIZE_STEP + 1;
}

static int ScreenOptions_SizeIndex(int percent, int count)
{
    int index = (percent - SCREEN_SIZE_MIN + SCREEN_SIZE_STEP / 2) / SCREEN_SIZE_STEP;
    if (index < 0)
        index = 0;
    if (index > count - 1)
        index = count - 1;
    return index;
}

// The values the page opened with: unchanged ones are not written, so that a
// size set in opencmr2.ini between the steps offered stays.
static BYTE g_screenOptionsShown[4];

void FrontendMenu_EnterScreenOptions(Menu *pMenu, int param)
{
    MenuItem *pItem;

    pMenu->text = ScreenOptions_Text(SCREEN_TEXT_TITLE);
    pItem = Menu_GetItem(pMenu, SCREEN_ITEM_HUD);
    pItem->min = ScreenOptions_SizeCount(SCREEN_SIZE_MAX);
    pItem->max = ScreenOptions_SizeIndex(Sys_GetOption("video.hud_scale", 100), pItem->min);
    pItem = Menu_GetItem(pMenu, SCREEN_ITEM_MENU);
    pItem->min = ScreenOptions_SizeCount(MenuScreen_GetLargestScale());
    pItem->max = ScreenOptions_SizeIndex(Sys_GetOption("video.menu_scale", 100), pItem->min);
    // On is the first choice, as in the graphics menu.
    Menu_GetItem(pMenu, SCREEN_ITEM_VSYNC)->max = Sys_GetOption("video.vsync", 1) != 0 ? 0 : 1;
    Menu_GetItem(pMenu, SCREEN_ITEM_FULLSCREEN)->max = g_pGraphics->isFullscreen ? 0 : 1;
    for (int i = 0; i < 4; i++)
        g_screenOptionsShown[i] = Menu_GetItem(pMenu, i)->max;
}

void FrontendMenu_ApplyScreenOptions(Menu *pMenu, MenuItem *pItem)
{
    BYTE value;

    value = Menu_GetItem(pMenu, SCREEN_ITEM_HUD)->max;
    if (value != g_screenOptionsShown[SCREEN_ITEM_HUD])
        Sys_SetOption("video.hud_scale", SCREEN_SIZE_MIN + value * SCREEN_SIZE_STEP);
    value = Menu_GetItem(pMenu, SCREEN_ITEM_MENU)->max;
    if (value != g_screenOptionsShown[SCREEN_ITEM_MENU])
        Sys_SetOption("video.menu_scale", SCREEN_SIZE_MIN + value * SCREEN_SIZE_STEP);
    value = Menu_GetItem(pMenu, SCREEN_ITEM_VSYNC)->max;
    if (value != g_screenOptionsShown[SCREEN_ITEM_VSYNC])
        Sys_SetOption("video.vsync", value == 0);
    value = Menu_GetItem(pMenu, SCREEN_ITEM_FULLSCREEN)->max;
    if (value != g_screenOptionsShown[SCREEN_ITEM_FULLSCREEN]) {
        // As Alt+Enter does.
        g_pGraphics->isFullscreen = value == 0;
        Gfx_SetFullscreen(g_pGraphics->isFullscreen);
        Sys_SetOption("video.fullscreen", g_pGraphics->isFullscreen);
    }
}

// The rows of the advanced graphics page: a label, then the value as
// "< 100% >" or the choices, the chosen one highlighted.
void FrontendMenu_DrawScreenOptions(Menu *pMenu)
{
    short rect[4];
    MenuItem *pItem;
    BYTE *pColour;
    BYTE *pSelColour;
    BYTE *pUnselColour;
    BYTE *pLineColour;
    BYTE *pLineShadow;
    int textY;
    int x;
    int y;
    int i;
    int j;

    rect[0] = (int)(g_pGraphics->resX * 100) / 640;
    rect[2] = CFrontend::m_pAr640ATexture->width;
    rect[3] = CFrontend::m_pAr640ATexture->height;
    y = ((int)(g_pGraphics->resY * 8) / 480 + (int)(g_pGraphics->resY * 38) / 480 +
         (int)(g_pGraphics->resY * 384) / 480) / 2 -
        ((int)(g_pGraphics->resY * 36) / 480 * pMenu->itemCount) / 2;
    pMenu->text = ScreenOptions_Text(SCREEN_TEXT_TITLE);
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, 2, NULL, -1);
    pLineColour = pMenu->cursor == 0 ? g_colourWhite0x00524968 : g_colourText0x0052496c;
    pLineShadow = pMenu->cursor == 0 ? g_colourShadowWhite0x00524974 : g_colourShadowText0x00524978;
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 99) / 640;
    g_frontendDrawRect[1] = y;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 282) / 640;
    g_frontendDrawRect[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineShadow, 1);
    g_frontendDrawRect[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineColour, 1);
    pItem = pMenu->items;
    for (i = 0; i < pMenu->itemCount; i++, pItem++) {
        rect[1] = y + (int)(g_pGraphics->resY * 2) / 480 + (int)(g_pGraphics->resY * 18) / 480 +
                  ((int)(g_pGraphics->resY * 36) / 480 * i - CFrontend::m_pAr640ATexture->height / 2);
        if (pMenu->cursor == i) {
            pColour = pSelColour = g_colourWhite0x00524968;
            pUnselColour = g_colourText0x0052496c;
            Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, (SpriteRect *)rect,
                         CFrontend::m_pAr640ATexture, 1, 0, 0, NULL, pColour, 8);
        } else {
            pColour = pUnselColour = g_colourText0x0052496c;
            pSelColour = g_colourWhite0x00524968;
            Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, (SpriteRect *)rect,
                         CFrontend::m_pAr640DTexture, 1, 0, 0, NULL, pColour, 8);
        }
        x = (int)(g_pGraphics->resX * 122) / 640;
        textY = (int)(g_pGraphics->resY * 24) / 480 + g_frontendDrawRect[1];
        switch (pItem->value) {
        case SCREEN_ITEM_HUD:
        case SCREEN_ITEM_MENU:
            sprintf(CFrontend::m_stringDest, "%s  < %d%% >",
                    ScreenOptions_Text(pItem->value == SCREEN_ITEM_HUD ? SCREEN_TEXT_HUD : SCREEN_TEXT_MENU),
                    SCREEN_SIZE_MIN + pItem->max * SCREEN_SIZE_STEP);
            Font_DrawText(1, CFrontend::m_stringDest, x, textY, (int *)pColour, 0x11);
            break;
        case SCREEN_ITEM_VSYNC:
        case SCREEN_ITEM_FULLSCREEN:
            strcpy(CFrontend::m_stringDest,
                   ScreenOptions_Text(pItem->value == SCREEN_ITEM_VSYNC ? SCREEN_TEXT_VSYNC : SCREEN_TEXT_FULLSCREEN));
            Font_DrawText(1, CFrontend::m_stringDest, x, textY, (int *)pColour, 0x11);
            for (j = 0; j < pItem->min; j++) {
                x += (int)(g_pGraphics->resX * 10) / 640 + Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
                strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(j == 0 ? 0x134 : 0x133));
                Font_DrawText(1, CFrontend::m_stringDest, x, textY,
                              (int *)(pItem->max == j ? pSelColour : pUnselColour), 0x11);
            }
            break;
        default:
            Font_DrawText(1, CFrontend::GetTextString(pItem->id), x, textY, (int *)pColour, 0x11);
            break;
        }
        if (pMenu->cursor == i + 1 || pMenu->cursor == i) {
            pLineColour = g_colourWhite0x00524968;
            pLineShadow = g_colourShadowWhite0x00524974;
        } else {
            pLineColour = g_colourText0x0052496c;
            pLineShadow = g_colourShadowText0x00524978;
        }
        g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 36) / 480 * (i + 1) + y;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineShadow, 1);
        g_frontendDrawRect[1]++;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineColour, 1);
    }
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

extern Menu g_menu0x008210f8;

void FrontendMenu_BuildScreenOptions(void)
{
    Menu_Init(&g_screenOptionsMenu, ScreenOptions_Text(SCREEN_TEXT_TITLE), -1, 0, &g_menu0x008210f8, NULL, 1, 0, 1);
    Menu_AddItemType3(&g_screenOptionsMenu, 0, -1, ScreenOptions_SizeCount(SCREEN_SIZE_MAX), 0, 0, 0, 0,
                      SCREEN_ITEM_HUD);
    Menu_AddItemType3(&g_screenOptionsMenu, 0, -1, ScreenOptions_SizeCount(SCREEN_SIZE_MAX), 0, 0, 0, 0,
                      SCREEN_ITEM_MENU);
    Menu_AddItemType3(&g_screenOptionsMenu, 0, -1, 2, 0, 0, 0, 0, SCREEN_ITEM_VSYNC);
    Menu_AddItemType3(&g_screenOptionsMenu, 0, -1, 2, 0, 0, 0, 0, SCREEN_ITEM_FULLSCREEN);
    Menu_AddItemType1(&g_screenOptionsMenu, 0, 0x67, (MenuItemAction)FrontendMenu_ApplyScreenOptions, SCREEN_ITEM_OK);
    Menu_SetCallbacks(&g_screenOptionsMenu, FrontendMenu_EnterScreenOptions, NULL,
                      (MenuCallback)FrontendMenu_DrawScreenOptions, NULL);
    Menu_ValidateCursor(&g_screenOptionsMenu, 0);
}

Menu *FrontendMenu_GetScreenOptions(void)
{
    return &g_screenOptionsMenu;
}

// The row of the screen options in the graphics menu.
char *FrontendMenu_GetScreenOptionsLabel(void)
{
    sprintf(CFrontend::m_stringDest, g_strEllipsisFormat, ScreenOptions_Text(SCREEN_TEXT_TITLE));
    return CFrontend::m_stringDest;
}

// PORT: the controls pages, in place of the original's (Options >
// Controllers). The first chooses the bindings to edit, player 1's keys,
// player 2's, the gamepads' or a connected joystick's or wheel's (src/input,
// controls.ini), and sets their deadzone, steering curve and rumble; the
// second binds each driving action to the next key,
// button or axis given. OK saves controls.ini; Back drops the changes.

Menu g_controlsMenu;
Menu g_controlsBindingsMenu;

enum ControlsItem {
    CONTROLS_ITEM_DEVICE,
    CONTROLS_ITEM_BINDINGS,
    CONTROLS_ITEM_DEADZONE,
    CONTROLS_ITEM_CURVE,
    CONTROLS_ITEM_RUMBLE,
    CONTROLS_ITEM_DEFAULT,
    CONTROLS_ITEM_OK = 0xe
};

enum ControlsText {
    CONTROLS_TEXT_DEVICE,
    CONTROLS_TEXT_KEYBOARD,
    CONTROLS_TEXT_KEYBOARD2,
    CONTROLS_TEXT_GAMEPAD,
    CONTROLS_TEXT_DEADZONE,
    CONTROLS_TEXT_CURVE,
    CONTROLS_TEXT_RUMBLE,
    CONTROLS_TEXT_PRESS_KEY,
    CONTROLS_TEXT_PRESS_INPUT,
    CONTROLS_TEXT_TOUCH,
    CONTROLS_TEXT_TOUCH_NOTE,
    CONTROLS_TEXT_COUNT
};

// In the game's character set (Windows-1252; 0xb1 is Polish l with a stroke).
static const char *const g_controlsText[6][CONTROLS_TEXT_COUNT] = {
    { "Device", "Keyboard", "Keyboard (Player 2)", "Gamepad", "Deadzone", "Steering Curve", "Vibration",
      "Press a key", "Press a button or move an axis (Escape cancels)",
      "Touch Controls", "Set up in the app's Controls screen" },
    { "P\xe9riph\xe9rique", "Clavier", "Clavier (joueur 2)", "Manette", "Zone morte", "Courbe de direction",
      "Vibrations", "Appuyez sur une touche",
      "Appuyez sur un bouton ou bougez un axe (\xc9" "chap annule)", "Commandes tactiles",
      "R\xe9glages dans l'\xe9" "cran Commandes de l'appli" },
    { "Dispositivo", "Teclado", "Teclado (jugador 2)", "Mando", "Zona muerta", "Curva de direcci\xf3n", "Vibraci\xf3n",
      "Pulsa una tecla", "Pulsa un bot\xf3n o mueve un eje (Esc cancela)", "Controles t\xe1" "ctiles",
      "Se configuran en la pantalla Controles de la app" },
    { "Dispositivo", "Tastiera", "Tastiera (giocatore 2)", "Gamepad", "Zona morta", "Curva dello sterzo",
      "Vibrazione", "Premi un tasto", "Premi un pulsante o muovi un asse (Esc annulla)", "Comandi touch",
      "Si impostano nella schermata Controlli dell'app" },
    { "Ger\xe4t", "Tastatur", "Tastatur (Spieler 2)", "Gamepad", "Totzone", "Lenkkurve", "Vibration",
      "Taste dr\xfc" "cken", "Knopf dr\xfc" "cken oder Achse bewegen (Esc bricht ab)",
      "Touch-Steuerung", "Einstellung im Steuerung-Bildschirm der App" },
    { "Urzadzenie", "Klawiatura", "Klawiatura (gracz 2)", "Pad", "Martwa strefa", "Krzywa skretu", "Wibracje",
      "Nacisnij klawisz", "Nacisnij przycisk lub porusz osia (Esc anuluje)", "Sterowanie dotykowe",
      "Ustawienia na ekranie Sterowanie aplikacji" },
};

// The settings offered: deadzone 0..20%, curve and rumble 0..100% by tens.
#define CONTROLS_DEADZONE_COUNT 21
#define CONTROLS_PERCENT_COUNT 11

static char *Controls_Text(int text)
{
    int language = CGameInfo::GetGameRegion() == 3 ? 5 : CFrontend::m_textLanguage;
    if (language > 5)
        language = 0;
    return (char *)g_controlsText[language][text];
}

static int g_controlsProfile;           // the bindings shown
static int g_controlsShownProfile = -1;
static BOOL g_controlsSaved;
static BOOL g_controlsCapturing;

static char *Controls_ProfileName(int profile)
{
    switch (Input_GetProfileKind(profile)) {
    case INPUT_PROFILE_TOUCH: return Controls_Text(CONTROLS_TEXT_TOUCH);
    case INPUT_PROFILE_KEYBOARD: return Controls_Text(CONTROLS_TEXT_KEYBOARD);
    case INPUT_PROFILE_KEYBOARD2: return Controls_Text(CONTROLS_TEXT_KEYBOARD2);
    case INPUT_PROFILE_GAMEPAD: return Controls_Text(CONTROLS_TEXT_GAMEPAD);
    default: return (char *)Input_GetProfileName(profile);
    }
}

// The rows of the profile shown, and which of its settings apply: none for
// keys, rumble only for gamepads.
static void Controls_LoadRows(Menu *pMenu)
{
    static const BYTE settings[3] = { CONTROLS_ITEM_DEADZONE, CONTROLS_ITEM_CURVE, CONTROLS_ITEM_RUMBLE };
    MenuItem *pItem;
    int kind;
    int i;

    kind = Input_GetProfileKind(g_controlsProfile);
    i = Input_GetSetting(g_controlsProfile, INPUT_SETTING_DEADZONE);
    Menu_GetItem(pMenu, CONTROLS_ITEM_DEADZONE)->max = (BYTE)(i < CONTROLS_DEADZONE_COUNT - 1 ? i : CONTROLS_DEADZONE_COUNT - 1);
    Menu_GetItem(pMenu, CONTROLS_ITEM_CURVE)->max = (BYTE)((Input_GetSetting(g_controlsProfile, INPUT_SETTING_CURVE) + 5) / 10);
    Menu_GetItem(pMenu, CONTROLS_ITEM_RUMBLE)->max = (BYTE)((Input_GetSetting(g_controlsProfile, INPUT_SETTING_RUMBLE) + 5) / 10);
    for (i = 0; i < 3; i++) {
        pItem = Menu_GetItem(pMenu, settings[i]);
        pItem->enabled = pItem->visible = kind != INPUT_PROFILE_KEYBOARD && kind != INPUT_PROFILE_KEYBOARD2 &&
                                          kind != INPUT_PROFILE_TOUCH &&
                                          (settings[i] != CONTROLS_ITEM_RUMBLE || kind == INPUT_PROFILE_GAMEPAD);
    }
    // The touch controls are set up in the launcher: their buttons show the
    // actions they press, so they are not bound here.
    pItem = Menu_GetItem(pMenu, CONTROLS_ITEM_BINDINGS);
    pItem->enabled = pItem->visible = kind != INPUT_PROFILE_TOUCH;
    pItem = Menu_GetItem(pMenu, CONTROLS_ITEM_DEFAULT);
    pItem->enabled = pItem->visible = kind != INPUT_PROFILE_TOUCH;
    if (!Menu_GetItem(pMenu, pMenu->items[pMenu->cursor].value)->visible)
        Menu_ValidateCursor(pMenu, 0);
    g_controlsShownProfile = g_controlsProfile;
}

static void Controls_StoreRows(Menu *pMenu)
{
    Input_SetSetting(g_controlsProfile, INPUT_SETTING_DEADZONE, Menu_GetItem(pMenu, CONTROLS_ITEM_DEADZONE)->max);
    Input_SetSetting(g_controlsProfile, INPUT_SETTING_CURVE, Menu_GetItem(pMenu, CONTROLS_ITEM_CURVE)->max * 10);
    Input_SetSetting(g_controlsProfile, INPUT_SETTING_RUMBLE, Menu_GetItem(pMenu, CONTROLS_ITEM_RUMBLE)->max * 10);
}

// Coming from the options (param 0) starts the changes afresh; coming back
// from the bindings page keeps them.
void FrontendMenu_EnterControls(Menu *pMenu, int param)
{
    MenuItem *pDevice;

    if (param == 0)
        g_controlsSaved = FALSE;
    pDevice = Menu_GetItem(pMenu, CONTROLS_ITEM_DEVICE);
    pDevice->min = (BYTE)Input_GetProfileCount();
    if (g_controlsProfile >= pDevice->min)
        g_controlsProfile = 0;
    pDevice->max = (BYTE)g_controlsProfile;
    Controls_LoadRows(pMenu);
}

// Follows the device chosen and the controllers connected meanwhile.
void FrontendMenu_UpdateControls(Menu *pMenu)
{
    MenuItem *pDevice;

    pDevice = Menu_GetItem(pMenu, CONTROLS_ITEM_DEVICE);
    pDevice->min = (BYTE)Input_GetProfileCount();
    if (pDevice->max >= pDevice->min)
        pDevice->max = 0;
    if (pDevice->max != g_controlsShownProfile) {
        g_controlsProfile = pDevice->max;
        Controls_LoadRows(pMenu);
    } else {
        Controls_StoreRows(pMenu);
    }
}

void FrontendMenu_RestoreControlsDefaults(Menu *pMenu, MenuItem *pItem)
{
    Input_RestoreDefaults(g_controlsProfile);
    Controls_LoadRows(pMenu);
}

void FrontendMenu_ApplyControls(Menu *pMenu, MenuItem *pItem)
{
    Controls_StoreRows(pMenu);
    g_controlsSaved = TRUE;
}

// Leaving for the options (v 1): OK saved the changes, Back drops them.
void FrontendMenu_LeaveControls(Menu *pMenu, char v)
{
    if (v == 0)
        return;
    if (g_controlsSaved)
        Input_SaveBindings();
    else
        Input_ReloadBindings();
    CInput::RefreshControllerConfigurations();
}

void FrontendMenu_BeginControlsCapture(Menu *pMenu, MenuItem *pItem)
{
    Input_BeginCapture(g_controlsProfile, pItem->value);
    g_controlsCapturing = TRUE;
    Menu_SetFlags(pMenu, 0, 0, 0, 0);
}

void FrontendMenu_UpdateControlsBindings(Menu *pMenu)
{
    if (!g_controlsCapturing)
        return;
    Menu_SetFlags(pMenu, 0, 0, 0, 0);
    if (Input_UpdateCapture() == 0)
        return;
    g_controlsCapturing = FALSE;
    Menu_SetFlags(pMenu, 1, 1, 1, 1);
}

// Both pages as the screen options are drawn: a label, then the value.
static void Controls_Draw(Menu *pMenu, int rowHeight, BOOL carousel)
{
    short rect[4];
    MenuItem *pItem;
    BYTE *pColour;
    BYTE *pSelColour;
    BYTE *pUnselColour;
    BYTE *pLineColour;
    BYTE *pLineShadow;
    char binding[128];
    int valueX;
    int textY;
    int step;
    int x;
    int y;
    int row;
    int i;
    int j;

    step = (int)(g_pGraphics->resY * rowHeight) / 480;
    rect[0] = (int)(g_pGraphics->resX * 100) / 640;
    rect[2] = CFrontend::m_pAr640ATexture->width;
    rect[3] = CFrontend::m_pAr640ATexture->height;
    row = 0;
    for (i = 0; i < pMenu->itemCount; i++)
        row += pMenu->items[i].visible ? 1 : 0;
    y = ((int)(g_pGraphics->resY * 8) / 480 + (int)(g_pGraphics->resY * 38) / 480 +
         (int)(g_pGraphics->resY * 384) / 480) / 2 - (step * row) / 2;
    // The bindings start under the title, as the original's did, and their
    // inputs line up after the longest action.
    valueX = 0;
    if (pMenu == &g_controlsBindingsMenu) {
        y = (int)(g_pGraphics->resY * 65) / 480;
        for (i = 0; i <= INPUT_PAUSE; i++) {
            j = Font_GetTextWidth(1, (BYTE *)CFrontend::GetTextString(0x6b + i));
            valueX = j > valueX ? j : valueX;
        }
        valueX += (int)(g_pGraphics->resX * 122) / 640 + (int)(g_pGraphics->resX * 24) / 640;
    }
    FrontendDraw_PlayTime();
    FrontendDraw_MenuPath(pMenu, PATH_X(), PATH_Y(), 1, 2, NULL, -1);
    pLineColour = pMenu->cursor == 0 ? g_colourWhite0x00524968 : g_colourText0x0052496c;
    pLineShadow = pMenu->cursor == 0 ? g_colourShadowWhite0x00524974 : g_colourShadowText0x00524978;
    g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 99) / 640;
    g_frontendDrawRect[1] = y;
    g_frontendDrawRect[2] = (int)(g_pGraphics->resX * 282) / 640;
    g_frontendDrawRect[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineShadow, 1);
    g_frontendDrawRect[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineColour, 1);
    row = 0;
    pItem = pMenu->items;
    for (i = 0; i < pMenu->itemCount; i++, pItem++) {
        if (!pItem->visible)
            continue;
        rect[1] = y + (int)(g_pGraphics->resY * 2) / 480 + step / 2 + (step * row - CFrontend::m_pAr640ATexture->height / 2);
        if (pMenu->cursor == i) {
            pColour = pSelColour = g_colourWhite0x00524968;
            pUnselColour = g_colourText0x0052496c;
            Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, (SpriteRect *)rect,
                         CFrontend::m_pAr640ATexture, 1, 0, 0, NULL, pColour, 8);
        } else {
            pColour = pUnselColour = g_colourText0x0052496c;
            pSelColour = g_colourWhite0x00524968;
            Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, (SpriteRect *)rect,
                         CFrontend::m_pAr640DTexture, 1, 0, 0, NULL, pColour, 8);
        }
        x = (int)(g_pGraphics->resX * 122) / 640;
        textY = step * 2 / 3 + g_frontendDrawRect[1];
        if (pMenu == &g_controlsBindingsMenu && pItem->value <= INPUT_PAUSE) {
            if (g_controlsCapturing && pMenu->cursor == i)
                strcpy(binding, "...");
            else
                Input_GetBindingText(g_controlsProfile, pItem->value, binding, sizeof(binding));
            Font_DrawText(1, CFrontend::GetTextString(pItem->id), x, textY, (int *)pColour, 0x11);
            Font_DrawText(1, binding, valueX, textY, (int *)pColour, 0x11);
        } else if (pMenu == &g_controlsMenu && pItem->value == CONTROLS_ITEM_DEVICE) {
            sprintf(CFrontend::m_stringDest, "%s  < %s >", Controls_Text(CONTROLS_TEXT_DEVICE),
                    Controls_ProfileName(pItem->max));
            Font_DrawText(1, CFrontend::m_stringDest, x, textY, (int *)pColour, 0x11);
        } else if (pMenu == &g_controlsMenu && pItem->value >= CONTROLS_ITEM_DEADZONE &&
                   pItem->value <= CONTROLS_ITEM_RUMBLE) {
            sprintf(CFrontend::m_stringDest, "%s  < %d%% >",
                    Controls_Text(pItem->value == CONTROLS_ITEM_DEADZONE ? CONTROLS_TEXT_DEADZONE
                                  : pItem->value == CONTROLS_ITEM_CURVE ? CONTROLS_TEXT_CURVE : CONTROLS_TEXT_RUMBLE),
                    pItem->value == CONTROLS_ITEM_DEADZONE ? pItem->max : pItem->max * 10);
            Font_DrawText(1, CFrontend::m_stringDest, x, textY, (int *)pColour, 0x11);
        } else if (pItem->type == 2) {
            sprintf(CFrontend::m_stringDest, g_strEllipsisFormat, CFrontend::GetTextString(pItem->id));
            Font_DrawText(1, CFrontend::m_stringDest, x, textY, (int *)pColour, 0x11);
        } else {
            Font_DrawText(1, CFrontend::GetTextString(pItem->id), x, textY, (int *)pColour, 0x11);
        }
        if (pMenu->cursor == i + 1 || pMenu->cursor == i) {
            pLineColour = g_colourWhite0x00524968;
            pLineShadow = g_colourShadowWhite0x00524974;
        } else {
            pLineColour = g_colourText0x0052496c;
            pLineShadow = g_colourShadowText0x00524978;
        }
        row++;
        g_frontendDrawRect[1] = step * row + y;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineShadow, 1);
        g_frontendDrawRect[1]++;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_frontendDrawRect, pLineColour, 1);
    }
    if (pMenu == &g_controlsMenu && Input_GetProfileKind(g_controlsProfile) == INPUT_PROFILE_TOUCH)
        Font_DrawText(1, Controls_Text(CONTROLS_TEXT_TOUCH_NOTE), (int)(g_pGraphics->resX * 122) / 640,
                      step * row + y + step, (int *)g_colourText0x0052496c, 0x11);
    if (g_controlsCapturing && pMenu == &g_controlsBindingsMenu) {
        int kind = Input_GetProfileKind(g_controlsProfile);
        Font_DrawText(1, Controls_Text(kind == INPUT_PROFILE_KEYBOARD || kind == INPUT_PROFILE_KEYBOARD2
                                       ? CONTROLS_TEXT_PRESS_KEY : CONTROLS_TEXT_PRESS_INPUT),
                      (int)(g_pGraphics->resX * 122) / 640, step * row + y + step, (int *)g_colourWhite0x00524968, 0x11);
    }
    if (carousel)
        FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

void FrontendMenu_DrawControls(Menu *pMenu)
{
    Controls_Draw(pMenu, 36, TRUE);
}

void FrontendMenu_DrawControlsBindings(Menu *pMenu)
{
    Controls_Draw(pMenu, 30, FALSE);
}

void FrontendMenu_BuildInputControls(void)
{
    int i;

    Menu_Init(&g_controlsMenu, NULL, 0x18, 0, FrontendMenu_GetOptions(), NULL, 1, 0, 1);
    Menu_AddItemType3(&g_controlsMenu, 0, -1, 1, 0, 0, 0, 0, CONTROLS_ITEM_DEVICE);
    Menu_AddItemType2(&g_controlsMenu, 0, 0x65, &g_controlsBindingsMenu, 0, CONTROLS_ITEM_BINDINGS);
    Menu_AddItemType3(&g_controlsMenu, 0, -1, CONTROLS_DEADZONE_COUNT, 0, 0, 0, 0, CONTROLS_ITEM_DEADZONE);
    Menu_AddItemType3(&g_controlsMenu, 0, -1, CONTROLS_PERCENT_COUNT, 0, 0, 0, 0, CONTROLS_ITEM_CURVE);
    Menu_AddItemType3(&g_controlsMenu, 0, -1, CONTROLS_PERCENT_COUNT, 0, 0, 0, 0, CONTROLS_ITEM_RUMBLE);
    Menu_AddItemType4(&g_controlsMenu, 0, 0x1fe, (MenuItemAction)FrontendMenu_RestoreControlsDefaults, CONTROLS_ITEM_DEFAULT);
    Menu_AddItemType1(&g_controlsMenu, 0, 0x67, (MenuItemAction)FrontendMenu_ApplyControls, CONTROLS_ITEM_OK);
    Menu_SetCallbacks(&g_controlsMenu, FrontendMenu_EnterControls, (MenuCallback)FrontendMenu_UpdateControls,
                      (MenuCallback)FrontendMenu_DrawControls, (MenuCallback)FrontendMenu_LeaveControls);
    Menu_ValidateCursor(&g_controlsMenu, 0);

    Menu_Init(&g_controlsBindingsMenu, NULL, 0x65, 0, &g_controlsMenu, NULL, 1, 0, 1);
    for (i = 0; i <= INPUT_PAUSE; i++)
        Menu_AddItemType4(&g_controlsBindingsMenu, 0, 0x6b + i, (MenuItemAction)FrontendMenu_BeginControlsCapture, i);
    Menu_AddItemType1(&g_controlsBindingsMenu, 0, 0x67, NULL, CONTROLS_ITEM_OK);
    Menu_SetCallbacks(&g_controlsBindingsMenu, NULL, (MenuCallback)FrontendMenu_UpdateControlsBindings,
                      (MenuCallback)FrontendMenu_DrawControlsBindings, NULL);
    Menu_ValidateCursor(&g_controlsBindingsMenu, 0);
}

Menu *FrontendMenu_GetInputControls(void)
{
    return &g_controlsMenu;
}
