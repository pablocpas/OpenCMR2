#include "port/movie.h"
#include "port/sys.h"
// The original sums the two packed DWORDs, including both high shorts.
inline int OptionMenu_AddPackedScreenRectangleWords(int *pWords)
{
    int value = pWords[1];
    value += pWords[0];
    return value;
}

#include "GameInfo.h"
#include "FloatToInt.h"
#include "Menu.h"
#include "Graphics.h"
#include "Input.h"
#include "Frontend.h"
#include "RegKey.h"
#include "InstallInfo.h"
#include "FileBuffer.h"
#include "GenericFileLoader.h"
#include "main.h"
#include "Font.h"
#include "FixedPoint.h"
#include "Game.h"
#include "Sound.h"
#include "RallyData.h"
#include "SavedGame.h"
#include "NetPlayers.h"

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "LayoutChecks.h"

// GLOBAL: CMR2 0x00516134
char gameRegionPoland[9] = "\\Poland\\";
// GLOBAL: CMR2 0x00516140
char gameRegionJapan[8] = "\\Japan\\";
// GLOBAL: CMR2 0x00516148
char gameRegionUSA[6] = "\\Usa\\";
// GLOBAL: CMR2 0x00516150
char gameRegionEurope[9] = "\\Europe\\";

GameInfo CGameInfo::m_gameInfo;
BYTE CGameInfo::m_unk0x0052ea52;
unsigned int CGameInfo::m_unk0x0052af80;
unsigned char CGameInfo::m_unk0x0052af40;
unsigned int CGameInfo::m_unk0x0052af84;
unsigned int CGameInfo::m_unk0x0052af88;
unsigned int CGameInfo::m_unk0x0052af8c;
unsigned int CGameInfo::m_unk0x0052af94;
unsigned int CGameInfo::m_unk0x0052af98;
unsigned int CGameInfo::m_unk0x0052af9c;
unsigned int CGameInfo::m_unk0x0052e93c;
unsigned int CGameInfo::m_unk0x0052e940;
unsigned int CGameInfo::m_unk0x0052ea44;
unsigned int CGameInfo::m_unk0x0052ea48;
unsigned int CGameInfo::m_gameRegion;
char *CGameInfo::m_gameRegionStrings[4] = {
    gameRegionEurope, gameRegionUSA, gameRegionJapan, gameRegionPoland};

BYTE CGameInfo::m_unk0x00817574;
int CGameInfo::m_unk0x0081a754;

char CGameInfo::m_stringCMR[4] = "cmr";
char CGameInfo::m_stringGameInfoRCF[32] = "%s\\Configuration\\GameInfo.rcf";
unsigned int CGameInfo::m_fixedMenuKeys;
unsigned int CGameInfo::m_unk0x00520870 = 1;
void* CGameInfo::m_unk0x0081777c = NULL;
BOOL CGameInfo::m_unk0x00817678 = FALSE;
char CGameInfo::m_unk0x005a00b8[0x104];
char CGameInfo::m_unk0x005a02c0[0x104];
char CGameInfo::m_sessionNames[20][0x104];
BOOL CGameInfo::m_unk0x005a0060;
HRESULT CGameInfo::m_unk0x005a1814;
BYTE CGameInfo::m_unk0x005a01bc;
NetSessionDesc CGameInfo::m_enumeratedSessions[20];

// FUNCTION: CMR2 0x004057f0
unsigned char CGameInfo::GetGameLanguage(void)
{
    return m_gameInfo.field_0x14 & 7;
}

// FUNCTION: CMR2 0x00405800
unsigned int CGameInfo::GetGameRegion(void)
{
    return m_gameRegion;
}

// FUNCTION: CMR2 0x00405810
void CGameInfo::SetGameRegion(unsigned int region)
{
    m_gameRegion = region;
}

// FUNCTION: CMR2 0x00405820
char *CGameInfo::GetGameRegionDirectory(void)
{
    return m_gameRegionStrings[m_gameRegion];
}

// FUNCTION: CMR2 0x00405d80
unsigned char CGameInfo::GetConfiguredGameMode(void)
{
    return m_gameInfo.field_0x14 >> 3 & 0x7f;
}

// FUNCTION: CMR2 0x00405d90
unsigned char CGameInfo::GetConfiguredDifficulty(void)
{
    return m_gameInfo.field_0x14 >> 10 & 7;
}

// FUNCTION: CMR2 0x00405d70
unsigned char CGameInfo::GetConfiguredPlayerCount(void)
{
    return m_gameInfo.field_0x14 >> 13 & 0xf;
}

// FUNCTION: CMR2 0x00405da0
unsigned char CGameInfo::IsConfiguredMultiplayer(void)
{
    return m_gameInfo.field_0x14 >> 17 & 1;
}

// FUNCTION: CMR2 0x00405c10
unsigned int CGameInfo::GetScreenWidth(void)
{
    return m_gameInfo.screenWidth;
}

// FUNCTION: CMR2 0x00405c30
unsigned int CGameInfo::GetScreenHeight(void)
{
    return m_gameInfo.screenHeight;
}

// FUNCTION: CMR2 0x00405c50
unsigned int CGameInfo::GetColourDepth(void)
{
    return m_gameInfo.screenColourDepth;
}

// FUNCTION: CMR2 0x00405c20
void CGameInfo::SetScreenWidth(unsigned int width)
{
    m_gameInfo.screenWidth = width;
}

// FUNCTION: CMR2 0x00405c40
void CGameInfo::SetScreenHeight(unsigned int height)
{
    m_gameInfo.screenHeight = height;
}

// FUNCTION: CMR2 0x00405c60
void CGameInfo::SetColourDepth(unsigned int depth)
{
    m_gameInfo.screenColourDepth = depth;
}

// FUNCTION: CMR2 0x004f4b40
void CGameInfo::InitRegionLanguageCount(void)
{
    switch (GetGameRegion())
    {
    case 0:
        m_unk0x0081a754 = 5;
        return;
    case 1:
        m_unk0x0081a754 = 3;
        return;
    case 2:
    case 3:
        m_unk0x0081a754 = 1;
    }
}

// FUNCTION: CMR2 0x00405de0
void CGameInfo::SetGameModeOptionBit19(BYTE param1)
{
    m_gameInfo.field_0x14 = ((param1 & 1) << 0x13) | (m_gameInfo.field_0x14 & 0xfff7ffffU);
}

// FUNCTION: CMR2 0x00405dc0
unsigned char CGameInfo::IsSplitBarEnabled(void)
{
    return m_gameInfo.field_0x14 >> 18 & 1;
}

// FUNCTION: CMR2 0x00405dd0
unsigned char CGameInfo::GetGameModeOptionBits20To22(void)
{
    return m_gameInfo.field_0x14 >> 20 & 7;
}

// FUNCTION: CMR2 0x00405e00
unsigned char CGameInfo::GetGameModeOptionBit19(void)
{
    return m_gameInfo.field_0x14 >> 19 & 1;
}

// FUNCTION: CMR2 0x00405e10
void CGameInfo::SetMasterSoundVolume(unsigned int param1)
{
    m_gameInfo.field_0x18 = m_gameInfo.field_0x18 ^ (m_gameInfo.field_0x18 ^ param1) & 0x7f;
    CSound::SetMusicStreamVolume(m_gameInfo.field_0x18 & 0x7f);
}

// FUNCTION: CMR2 0x00405e40
unsigned int CGameInfo::GetMasterSoundVolume(void)
{
    return m_gameInfo.field_0x18 & 0x7f;
}

// FUNCTION: CMR2 0x00405e50
void CGameInfo::SetEffectsSoundVolume(unsigned int param1)
{
    m_gameInfo.field_0x18 = ((param1 & 0x7f) << 7) | (m_gameInfo.field_0x18 & 0xffffc07fU);
}

// FUNCTION: CMR2 0x00405e70
unsigned int CGameInfo::GetEffectsSoundVolume(void)
{
    return m_gameInfo.field_0x18 >> 7 & 0x7f;
}

// FUNCTION: CMR2 0x00405e80
void CGameInfo::SetCoDriverSoundVolume(unsigned int param1)
{
    m_gameInfo.field_0x18 = ((param1 & 0x7f) << 14) | (m_gameInfo.field_0x18 & 0xffe03fffU);
}

// FUNCTION: CMR2 0x00405ea0
unsigned int CGameInfo::GetCoDriverSoundVolume(void)
{
    return m_gameInfo.field_0x18 >> 14 & 0x7f;
}

// FUNCTION: CMR2 0x00405eb0
unsigned int CGameInfo::IsDashOptionEnabled(void)
{
    return m_gameInfo.field_0x18 >> 31;
}

// FUNCTION: CMR2 0x00405ec0
void CGameInfo::SetDashOptionEnabled(BYTE param1)
{
    m_gameInfo.field_0x18 = (param1 << 31) | (m_gameInfo.field_0x18 & 0x7fffffffU);
}

// FUNCTION: CMR2 0x00405ef0
unsigned int CGameInfo::GetNetworkOptionBits1To2(void)
{
    return m_gameInfo.field_0x1c >> 1 & 3;
}

// FUNCTION: CMR2 0x00405f00
void CGameInfo::SetSoundOptionBits24To25(unsigned int param1)
{
    m_gameInfo.field_0x18 = ((param1 & 3) << 24) | (m_gameInfo.field_0x18 & 0xfcffffffU);
}

// FUNCTION: CMR2 0x00405f20
void CGameInfo::SetSoundOptionBit27(BYTE param1)
{
    m_gameInfo.field_0x18 = ((param1 & 1) << 27) | (m_gameInfo.field_0x18 & 0xf7ffffffU);
}

// FUNCTION: CMR2 0x00405f40
void CGameInfo::SetSoundOptionBit26(BYTE param1)
{
    m_gameInfo.field_0x18 = ((param1 & 1) << 26) | (m_gameInfo.field_0x18 & 0xfbffffffU);
}

// FUNCTION: CMR2 0x00405f60
void CGameInfo::SetSoundOptionBit28(BYTE param1)
{
    m_gameInfo.field_0x18 = ((param1 & 1) << 28) | (m_gameInfo.field_0x18 & 0xefffffffU);
}

// FUNCTION: CMR2 0x00405f80
void CGameInfo::SetSoundOptionBit29(BYTE param1)
{
    m_gameInfo.field_0x18 = ((param1 & 1) << 29) | (m_gameInfo.field_0x18 & 0xdfffffffU);
}

// FUNCTION: CMR2 0x00405fa0
void CGameInfo::SetDefaultCameraParameters(FixVector *pOffset, WORD pitch, DWORD lag)
{
    m_gameInfo.chaseCameraHeight = pOffset->y;
    m_gameInfo.chaseCameraDistance = pOffset->z;
    m_gameInfo.chaseCameraPitch = pitch;
    m_gameInfo.chaseCameraLag = lag;
}

// FUNCTION: CMR2 0x00405fd0
void CGameInfo::SetGameInfoField98(unsigned int param1)
{
    *(unsigned int *)&m_gameInfo.field_0x98 = param1;
}

// FUNCTION: CMR2 0x00405db0
unsigned int *CGameInfo::GetGameInfoField9CAddress(void)
{
    return &m_gameInfo.field_0x9c;
}

// FUNCTION: CMR2 0x00405fe0
GameInfo0xa4 *CGameInfo::GetGameInfoFieldA4Address(void)
{
    return &m_gameInfo.field_0xa4;
}

// FUNCTION: CMR2 0x00405ff0
GameInfo0xa4 *CGameInfo::GetPlayerRecordTable(int param1)
{
    return param1 != 0 ? &m_gameInfo.field_0x1368 : &m_gameInfo.field_0x262c;
}

// FUNCTION: CMR2 0x00405b30
void CGameInfo::SetFullscreen(BYTE fullscreen)
{
    m_gameInfo.graphicsOptions = (fullscreen & 1) | (m_gameInfo.graphicsOptions & 0xfffffffeU);
}

// FUNCTION: CMR2 0x00405b50
unsigned int CGameInfo::GetGraphicsOptionBits1To2(void)
{
    return m_gameInfo.graphicsOptions >> 1 & 3;
}

// FUNCTION: CMR2 0x00405b60
void CGameInfo::SetGraphicsOptionBits1To2(unsigned int param1)
{
    m_gameInfo.graphicsOptions = ((param1 & 3) << 1) | (m_gameInfo.graphicsOptions & 0xfffffff9U);
}

// FUNCTION: CMR2 0x00405b80
void CGameInfo::SetGraphicsOptionBit3(BYTE param1)
{
    m_gameInfo.graphicsOptions = ((param1 & 1) << 3) | (m_gameInfo.graphicsOptions & 0xfffffff7U);
}

// FUNCTION: CMR2 0x00405ba0
unsigned int CGameInfo::GetGraphicsOptionBit4(void)
{
    return m_gameInfo.graphicsOptions >> 4 & 1;
}

// FUNCTION: CMR2 0x00405bb0
void CGameInfo::SetGraphicsOptionBit4(BYTE param1)
{
    m_gameInfo.graphicsOptions = ((param1 & 1) << 4) | (m_gameInfo.graphicsOptions & 0xffffffefU);
}

// FUNCTION: CMR2 0x00405be0
void CGameInfo::SetGraphicsOptionBits5To8(unsigned int param1)
{
    m_gameInfo.graphicsOptions = ((param1 & 0xf) << 5) | (m_gameInfo.graphicsOptions & 0xfffffe1fU);
}

// FUNCTION: CMR2 0x00405c70
unsigned int CGameInfo::GetGraphicsOptionBits18To19(void)
{
    return m_gameInfo.graphicsOptions >> 18 & 3;
}

// FUNCTION: CMR2 0x00405c80
void CGameInfo::SetGraphicsOptionBits18To19(unsigned int param1)
{
    m_gameInfo.graphicsOptions = ((param1 & 3) << 18) | (m_gameInfo.graphicsOptions & 0xfff3ffffU);
}

// FUNCTION: CMR2 0x00405cb0
void CGameInfo::SetGraphicsOptionBits21To24(unsigned int param1)
{
    m_gameInfo.graphicsOptions = ((param1 & 0xf) << 21) | (m_gameInfo.graphicsOptions & 0xfe1fffffU);
}

// FUNCTION: CMR2 0x00405cd0
unsigned int CGameInfo::GetGraphicsOptionBits25To26(void)
{
    return m_gameInfo.graphicsOptions >> 25 & 3;
}

// FUNCTION: CMR2 0x00405ce0
void CGameInfo::SetGraphicsOptionBits25To26(unsigned int param1)
{
    m_gameInfo.graphicsOptions = ((param1 & 3) << 25) | (m_gameInfo.graphicsOptions & 0xf9ffffffU);
}

// FUNCTION: CMR2 0x00405d00
unsigned int CGameInfo::GetGraphicsOptionBits27To28(void)
{
    return m_gameInfo.graphicsOptions >> 27 & 3;
}

// FUNCTION: CMR2 0x00405d20
void CGameInfo::SetGraphicsOptionBits27To28(unsigned int param1)
{
    m_gameInfo.graphicsOptions = ((param1 & 3) << 27) | (m_gameInfo.graphicsOptions & 0xe7ffffffU);
}

// FUNCTION: CMR2 0x00405d60
unsigned int CGameInfo::GetGraphicsOptionBit29(void)
{
    return m_gameInfo.graphicsOptions >> 29 & 1;
}

// FUNCTION: CMR2 0x00405d10
unsigned int CGameInfo::GetPreviewMode(void)
{
    return m_gameInfo.field_0x34 & 3;
}

// FUNCTION: CMR2 0x00405d40
void CGameInfo::SetPreviewMode(unsigned int param1)
{
    m_gameInfo.field_0x34 = m_gameInfo.field_0x34 ^ (m_gameInfo.field_0x34 ^ param1) & 3;
}

// FUNCTION: CMR2 0x00406310
unsigned char CGameInfo::GetSoundOptionBit30(void)
{
    return m_gameInfo.field_0x18 >> 30 & 1;
}

// FUNCTION: CMR2 0x00406320
BYTE CGameInfo::GetGameInfoSessionFlag(void)
{
    return m_unk0x0052ea52;
}

// FUNCTION: CMR2 0x00406330
void CGameInfo::SetGameInfoSessionFlag(BYTE param1)
{
    m_unk0x0052ea52 = param1;
}

// FUNCTION: CMR2 0x00406340
void CGameInfo::SetSplitBarEnabled(BYTE param1)
{
    m_gameInfo.field_0x14 = ((param1 & 1) << 18) | (m_gameInfo.field_0x14 & 0xfffbffffU);
}

// FUNCTION: CMR2 0x00406360
unsigned int CGameInfo::GetUnlockFlagMask(int param1)
{
    return (BYTE)(1 << param1) & m_gameInfo.field_0x20;
}

// FUNCTION: CMR2 0x00406380
void CGameInfo::SetUnlockFlag(int param1, int param2)
{
    BYTE mask;

    mask = (BYTE)(1 << param1);
    if (param2 != 0) {
        m_gameInfo.field_0x20 |= mask;
        return;
    }
    m_gameInfo.field_0x20 &= (BYTE)~mask | 0xffffff00;
}

// FUNCTION: CMR2 0x004063d0
int CGameInfo::IsConfiguredCheatEnabled(int param1)
{
    return ((BYTE)(m_gameInfo.field_0x20 >> 8) & (BYTE)(1 << param1)) != 0;
}

// FUNCTION: CMR2 0x004063f0
int CGameInfo::IsActiveCheatEnabled(int param1)
{
    return ((BYTE)(m_gameInfo.field_0x20 >> 16) & (BYTE)(1 << param1)) != 0;
}

// FUNCTION: CMR2 0x00406430
unsigned int CGameInfo::GetActiveCheatMask(void)
{
    return ((BYTE *)&m_gameInfo.field_0x20)[2];
}

// FUNCTION: CMR2 0x00406440
unsigned int CGameInfo::GetNetworkOptionBit11(void)
{
    return m_gameInfo.field_0x1c >> 11 & 1;
}

// FUNCTION: CMR2 0x00406450
unsigned int CGameInfo::GetRecordFlagsWord(unsigned int **param1)
{
    if (param1 != NULL)
        *param1 = &m_gameInfo.field_0x38f4;
    return m_gameInfo.field_0x38f4;
}

// FUNCTION: CMR2 0x00406470
void CGameInfo::SaveFrontendOptionSettings(void)
{
    m_unk0x0052af94 = GetConfiguredGameMode();
    m_unk0x0052ea44 = GetConfiguredPlayerCount();
    m_unk0x0052af9c = GetConfiguredDifficulty();
    m_unk0x0052af80 = RallyData_GetSecondarySelectionNibble();
    m_unk0x0052af84 = (BYTE)RallyData_GetSelectionBits10To11();
    m_unk0x0052af8c = (BYTE)RallyData_GetSelectionBits12To13();
    m_unk0x0052e93c = (BYTE)RallyData_GetSelectionBits16To19();
    m_unk0x0052ea48 = (BYTE)RallyDataStageIndex();
    m_unk0x0052e940 = (BYTE)RallyDataCountryIndex();
    m_unk0x0052af98 = m_gameInfo.field_0x14 >> 23 & 0xf;
    m_unk0x0052af88 = m_gameInfo.field_0x14 >> 27 & 0xf;
}

// FUNCTION: CMR2 0x00406520
BYTE CGameInfo::GetStageOptionState(int param1, int param2)
{
    return m_gameInfo.field_0x38f8[param1 * 11 + param2];
}

// FUNCTION: CMR2 0x00406540
void CGameInfo::SetStageOptionState(int param1, int param2, BYTE param3)
{
    m_gameInfo.field_0x38f8[param1 * 11 + param2] = param3;
}

// FUNCTION: CMR2 0x00406690
char *CGameInfo::GetSessionName(void)
{
    return m_gameInfo.field_0x3950;
}

// FUNCTION: CMR2 0x004066a0
void CGameInfo::SetSessionName(char *name)
{
    strcpy(m_gameInfo.field_0x3950, name);
}

// FUNCTION: CMR2 0x004066d0
char *CGameInfo::GetSessionPassword(void)
{
    return m_gameInfo.field_0x3965;
}

// FUNCTION: CMR2 0x004066e0
void CGameInfo::SetSessionPassword(char *name)
{
    strcpy(m_gameInfo.field_0x3965, name);
}

void Input_TranslatePedalsToMenuKeys(void);
void Input_MergeAssignedJoystickButtons(int slot, DeviceInfo *pOut);
void Sound_UpdateMusicStreaming(void);
int Sound_GetSampleTableState(void);
BOOL OptionMovie_PlayFrame(BYTE skipOnSpace);
void OptionMovie_Close(void);


// Bink movie state: the open movie and its dimensions, the Bink buffer the
// frames are played on, the DirectDraw surface they are converted to, and the
// surface type returned for it (-1 while unknown). The four screen coordinates
// are the rectangle the movie is scaled into (left, top, right, bottom).
// GLOBAL: CMR2 0x005297d0
int g_binkSurfaceType = -1;
// GLOBAL: CMR2 0x00831ac8
unsigned int g_unk0x00831ac8;
// GLOBAL: CMR2 0x00831acc
unsigned int g_unk0x00831acc;
// PORT: the movie (our Bink decoder); the Bink buffer is gone, the movie
// presents itself.
// GLOBAL: CMR2 0x00831ad0
Movie *g_pUnk0x00831ad0;
// GLOBAL: CMR2 0x00831ad4
void *g_pUnk0x00831ad4;
// GLOBAL: CMR2 0x00831c54
GfxTexture *g_pUnk0x00831c54;
// GLOBAL: CMR2 0x00831c58
int g_unk0x00831c58;
// GLOBAL: CMR2 0x00831c5c
int g_unk0x00831c5c;
// GLOBAL: CMR2 0x00831c60
int g_unk0x00831c60;
// GLOBAL: CMR2 0x00831c64
int g_unk0x00831c64;
// GLOBAL: CMR2 0x00831c68
int g_unk0x00831c68;
// GLOBAL: CMR2 0x00831c6c
void *g_pUnk0x00831c6c;          // PORT: unused (the DirectDraw surface frames were converted to)

// Starts the movie in the file: remembers the rectangle it is played in (or
// uses the whole screen when no rectangle is given) and the surface it is
// copied to, then opens it. With flag 4 only one frame is played and 2 is
// returned while frames are left; otherwise it plays to the end. Returns 1 when
// it is done, or when the movie could not be opened.
// FUNCTION: CMR2 0x0050fdf0
int OptionMovie_StartPlayback(char *path, Texture *pTexture, short *pRect, unsigned int flags, unsigned int track)
{
    if (g_unk0x00831c68 == 0) {
        if (pRect != NULL) {
            g_unk0x00831c58 = pRect[0];
            g_unk0x00831c5c = pRect[1];
            g_unk0x00831c60 = pRect[2] + pRect[0];
            g_unk0x00831c64 = pRect[3] + pRect[1];
        } else {
            g_unk0x00831c58 = 0;
            g_unk0x00831c5c = 0;
            g_unk0x00831c60 = g_pGraphics->resX;
            g_unk0x00831c64 = g_pGraphics->resY;
        }
        // PORT: the rectangle stays in back buffer coordinates (the original
        // moved it to the window's desktop position when windowed).
        if (pTexture != NULL)
            g_pUnk0x00831c54 = pTexture->pSurface;
        else
            g_pUnk0x00831c54 = NULL;
        if (OptionMovie_Open(path, track) == 0)
            return 1;
        g_unk0x00831c68 = 1;
    }
    if (flags & 4) {
        if (OptionMovie_PlayFrame((BYTE)flags) != 0)
            return 2;
    } else {
        while (OptionMovie_PlayFrame((BYTE)flags) != 0) {
        }
    }
    OptionMovie_Close();
    return 1;
}

// Opens the movie file with Bink, creates the DirectDraw surface its frames are
// copied to and opens a Bink buffer on the game window. Returns 0 if the CD is
// missing.
// FUNCTION: CMR2 0x0050ff90
// PORT: opens the movie with our decoder; there is no surface or Bink buffer
// to create.
int OptionMovie_Open(char *fileName, unsigned int trackIndex)
{
    g_pUnk0x00831ad0 = Movie_Open(fileName, trackIndex);
    while (g_pUnk0x00831ad0 == NULL) {
        if (!CInstallInfo::ShowNoCDErrorMessage())
            return 0;
        g_pUnk0x00831ad0 = Movie_Open(fileName, trackIndex);
    }

    g_unk0x00831ac8 = Movie_GetWidth(g_pUnk0x00831ad0);
    g_unk0x00831acc = Movie_GetHeight(g_pUnk0x00831ad0);
    return 1;
}

// Half of the letterbox border added to centre the movie (0.5).
// GLOBAL: CMR2 0x005113b8
double g_doubleHalf = 0.5;

// PORT: movie playback blocks the main loop, so it must dispatch SDL events
// itself. Consume Enter here so a skip does not also confirm the next menu.
static BOOL OptionMovie_PollEvents(void)
{
    SysEvent event;
    BOOL skip = FALSE;
    while (Sys_PollEvent(&event)) {
        if (event.type == SYS_EVENT_KEY_DOWN &&
            (event.key == INPUT_KEY_RETURN || event.key == INPUT_KEY_NUMPADENTER)) {
            if (!event.repeat)
                skip = TRUE;
        } else
            CMain::HandleEvent(&event);
    }
    // Record the held button before returning so the same press cannot become
    // a new controller confirmation when the following movie opens.
    if (skip)
        CInput::UpdateAllAvailableDevices();
    return skip || CGame::m_shouldExit;
}

// Plays one frame of the current movie: scales and offsets the Bink buffer to
// the game window, copies the frame into it and blits it to the screen.
// Returns whether the movie has more frames left.
// FUNCTION: CMR2 0x00510120
// PORT: stretch into the requested rectangle (the whole back buffer for
// intros), independently of the old DirectDraw scaling capability flags.
BOOL OptionMovie_PlayFrame(BYTE skipOnSpace)
{
    DeviceInfo *pDevice;
    int waitResult;

    if (OptionMovie_PollEvents())
        return FALSE;
    CInput::UpdateAllAvailableDevices();
    pDevice = CInput::GetAvailableDeviceRecord(0);
    Input_MergeAssignedJoystickButtons(0, pDevice);
    if (pDevice->pressed & 0x10)
        return FALSE;
    if (skipOnSpace & 1) {
        if (Input_IsKeyDown(INPUT_KEY_SPACE))
            return TRUE;
    }

    Movie_DecodeFrame(g_pUnk0x00831ad0);
    GfxViewport previousViewport;
    Gfx_GetViewport(&previousViewport);
    GfxViewport viewport = { 0, 0, (DWORD)g_pGraphics->resX, (DWORD)g_pGraphics->resY, 0.0f, 1.0f };
    Gfx_SetViewport(&viewport);
    Movie_Present(g_pUnk0x00831ad0, g_unk0x00831c58, g_unk0x00831c5c,
                  g_unk0x00831c60 - g_unk0x00831c58, g_unk0x00831c64 - g_unk0x00831c5c);
    Gfx_SetViewport(&previousViewport);
    Movie_NextFrame(g_pUnk0x00831ad0);
    waitResult = Movie_Wait(g_pUnk0x00831ad0);
    while (waitResult != 0) {
        if (OptionMovie_PollEvents())
            return FALSE;
        Sys_Sleep(1);
        waitResult = Movie_Wait(g_pUnk0x00831ad0);
    }
    return Movie_GetFrameNumber(g_pUnk0x00831ad0) <= Movie_GetFrameCount(g_pUnk0x00831ad0);
}

// Releases the DirectDraw surface the movie frames are converted to and closes
// the open movie.
// FUNCTION: CMR2 0x005103d0
void OptionMovie_Close(void)
{
    g_unk0x00831c68 = 0;
    g_pUnk0x00831c54 = NULL;
    if (g_pUnk0x00831ad0 != NULL) {
        Movie_Close(g_pUnk0x00831ad0);
        g_pUnk0x00831ad0 = NULL;
    }
}

// match 74%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00510410
void CGameInfo::InitDefaultGameInfo(void)
{
    memset(&m_gameInfo, 0, sizeof(m_gameInfo));
    m_gameInfo.magicNumber = 0x11;

    strcpy(m_gameInfo.field_0x3950, CMain::m_logFileBlankLine);
    strcpy(m_gameInfo.field_0x3965, CMain::m_logFileBlankLine);

    m_gameInfo.field_0x3980 = 8;
    m_gameInfo.field_0x3990 = 8;
    m_gameInfo.field_0x9c = (m_gameInfo.field_0x9c & 0xffc11157U) | 0x11154;
    m_gameInfo.field_0x18 = (m_gameInfo.field_0x18 & 0xfcffffffU) | 0xbc000000;
    m_gameInfo.field_0x1c = (m_gameInfo.field_0x1c & 0xfff33d19U) | 0x33518;
    m_gameInfo.field_0x14 = (m_gameInfo.field_0x14 & 0x807627ffU) | 0x42400;
    m_gameInfo.field_0x397c = 10;
    m_gameInfo.field_0x3984 = 6;
    m_gameInfo.field_0x3988 = 3;
    m_gameInfo.field_0x398c = 0x1e;

    ResetDefaultCameraParameters();

    *(int *)&m_gameInfo.field_0x98 = 4;
    m_gameInfo.field_0x18 = (m_gameInfo.field_0x18 & 0xfff93264) | 0x40393264;

    InitProfileRecordDefaults(&m_gameInfo.field_0xa4);
    InitProfileRecordDefaults(&m_gameInfo.field_0x1368);
    InitProfileRecordDefaults(&m_gameInfo.field_0x262c);

    m_gameInfo.field_0x14 = m_gameInfo.field_0x14 & 0x7fffffff;
}

// FUNCTION: CMR2 0x00510570
void CGameInfo::ResetDefaultCameraParameters(void)
{
    m_gameInfo.chaseCameraHeight = 0x18000;
    m_gameInfo.chaseCameraDistance = 0x68000;
    m_gameInfo.chaseCameraPitch = 0x2d;
    m_gameInfo.chaseCameraLag = 0xe0000;
}

// Initializes profile record labels, flags, default times and split tables.
// FUNCTION: CMR2 0x00406010
void CGameInfo::InitProfileRecordDefaults(GameInfo0xa4 *pInfo)
{
    unsigned int *pFlags = &pInfo->firstLoop[0].flags;
    int group = 3;
    int count;
    int track;
    int row;
    int split;
    unsigned int *pFinalTime;
    short *pFinalSplits;
    do {
        count = 5;
        do {
            strcpy((char *)(pFlags - 1), m_stringCMR);
            *pFlags = (*pFlags & 0xffffc000) | 0x3c000;
            memset(pFlags + 1, 0, 3);
            pFlags += 3;
        } while (--count);
    } while (--group);
    pFlags = &pInfo->secondLoop[0].flags;
    track = 0xcb;
    pFinalSplits = pInfo->rallyStageRecordSplits[10];
    pFinalTime = &pInfo->rallyStageRecordTimes[10].value;
    do {
        group = 3;
        do {
            count = 5;
            do {
                strcpy((char *)(pFlags - 1), m_stringCMR);
                *pFlags &= 0xffffff80;
                pFlags[1] = 360000;
                *pFlags = (*pFlags & 0xffff87ff) | 0x780;
                pFlags += 3;
            } while (--count);
        } while (--group);
        row = 0;
        do {
            GameInfo0xa4SubStruct8 *pTime = (GameInfo0xa4SubStruct8 *)((BYTE *)pInfo + (row + track) * 8 - 4);
            strcpy(pTime->ident, m_stringCMR);
            unsigned int value = pTime->value & 0xffffff80;
            pTime->value = value;
            unsigned int *pDest = &pTime->value;
            if (row == 10) {
                value = (value & 0xffaee07f) | 0x2ee000;
                pDest = pFinalTime;
            } else {
                value = (value & 0xffddc07f) | 0x5dc000;
            }
            row++;
            *pDest = value;
        } while (row < 11);
        row = 0;
        do {
            short *pSplit;
            if (row == 10) {
                pSplit = pFinalSplits;
                split = 0;
                do {
                    *pSplit++ = (short)(split * 12000);
                    split++;
                } while (split < 10);
            } else {
                split = 0;
                pSplit = (short *)((BYTE *)pInfo + (row + track) * 20 - 0x6c8);
                do {
                    *pSplit++ = (short)(split * 6000);
                    split++;
                } while (split < 10);
            }
            row++;
        } while (row < 11);
        pFinalTime += 22;
        pFinalSplits += 110;
        track += 11;
    } while (track < 0x123);
    pFlags = &pInfo->thirdLoop[0].flags;
    track = 3;
    do {
        group = 3;
        do {
            count = 5;
            do {
                strcpy((char *)(pFlags - 1), m_stringCMR);
                *pFlags = (*pFlags & 0xffff0280) | 0x280;
                pFlags += 3;
            } while (--count);
        } while (--group);
    } while (--track);
    pFlags = &pInfo->arcadeRecordTimes[0].value;
    short *pSplits = pInfo->arcadeRecordSplits[0];
    track = 3;
    do {
        group = 3;
        do {
            strcpy((char *)(pFlags - 1), m_stringCMR);
            *pFlags = (*pFlags & 0xffaee000) | 0x2ee000;
            split = 0;
            do {
                *pSplits++ = (short)(split * 6000);
                split++;
            } while (split < 6);
            pFlags += 2;
        } while (--group);
    } while (--track);
}

// FUNCTION: CMR2 0x00406560
void CGameInfo::ResetStageOptionStates(void)
{
    memset(m_gameInfo.field_0x38f8, 0x03, sizeof(m_gameInfo.field_0x38f8));
}

// match 78%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00406580
void CGameInfo::ApplyStageOptionUnlockFlags(void) {
    char bVar1;
    int uVar5 = 0;
    int uVar2 = 0;
    int iVar3 = 0;
    char *piVar4 = m_gameInfo.field_0x38f8;
    char *end = m_gameInfo.field_0x38f8 + sizeof(m_gameInfo.field_0x38f8);
    bool bVar6 = false;

    do {
        if (((INT_PTR)piVar4 < (INT_PTR)(m_gameInfo.field_0x38f8 + 44)) && (*piVar4 & 2) != 0) {
            *piVar4 = *piVar4 & 0xfd;
        }

        if (uVar5 < (int)(m_gameInfo.field_0x9c >> 8 & 0xf)) {
            iVar3 = 0;
            do {
                if ((piVar4[iVar3] & 2) != 0) {
                    piVar4[iVar3] = piVar4[iVar3] & 0xfd;
                }
                iVar3 ++;
            } while (iVar3 < 4);

            if (uVar5 < (int)(m_gameInfo.field_0x9c >> 0xc & 0xf)) {
                iVar3 = 4;
                do {
                    if ((piVar4[iVar3] & 2) != 0) {
                        piVar4[iVar3] = piVar4[iVar3] & 0xfd;
                    }
                    iVar3 ++;
                } while (iVar3 < 8);
            
                if ((uVar5 < (int)(m_gameInfo.field_0x9c >> 0x10 & 0xf)) && (m_gameInfo.field_0x9c & 1) != 0) {
                    iVar3 = 8;
                    do {
                        if ((piVar4[iVar3] & 2) != 0) {
                            piVar4[iVar3] = piVar4[iVar3] & 0xfd;
                        }
                        iVar3 ++;
                    } while (iVar3 < 10);
                }
            } 
        }

        uVar2 = uVar5;
        uVar2 = uVar2 & 0x80000001;

        if (uVar2 < 0) {
            uVar2 = (uVar2 - 1) | 0xfffffffe;
            uVar2++;
        }

        if (uVar2) {
            uVar2 = 1 << (((uVar5 + 1) / 2 - 1) & 0x1f);

            if (((((uVar2 & m_gameInfo.field_0xa0 & 0x1f) != 0) ||
                (uVar2 & m_gameInfo.field_0xa0 >> 5 & 0x1f) != 0) ||
                (0 != (uVar2 & m_gameInfo.field_0xa0 >> 10 & 0x1f))) && ((piVar4[10] & 2) != 0)
                ) {
                piVar4[10] = piVar4[10] & 0xfd;
            }            
        } else {
            piVar4[10] = 4;
        }
        
        piVar4 = piVar4 + 0xb;
        uVar5++;
    } while (piVar4 < end);
}

// TODO: is this actually gameinfo related?
// FUNCTION: CMR2 0x0049eaf0
DWORD CGameInfo::SetupInputs(int unused) {
    if (CInput::DInputCreate()) {
        CInput::m_deviceCounts.nextDevice = 0;
        CInput::m_deviceCounts.keyboardCount = 2;
        CInput::m_deviceCounts.joystickCount = 0;
        
        for (int i = 0; i < 8; i++) {
            CInput::m_availableDevices[i].type = -1;
            CInput::m_availableDevices[i].joystickIndex = -1;
        }
        
        CInput::SetupKeyboard();
        CInput::SetupMouse();
        CInput::GetAttachedJoysticks();
    }

    return CInput::m_deviceCounts.nextDevice & 0xFF;
}

// match 76%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004ea5e0
bool CGameInfo::LoadGameInfo(void) {
    char *hdPath;
    GameInfo *fileBuffer;
    size_t fileSize;
    int iVar4;
    unsigned int graphicsOptions, temp_field0x3bc;

    hdPath = CInstallInfo::GetGameHDPath();
    sprintf(CFrontend::m_stringDest, m_stringGameInfoRCF, hdPath);

    fileBuffer = (GameInfo*)CFileBuffer::GetGenericFileBuffer(CFrontend::m_stringDest, TRUE);
    if (fileBuffer == NULL)
        return false;

    // make sure file size is valid and that it has the magic number
    fileSize = CGenericFileLoader::GetGenericFileSize();
    if (fileSize != 0x399c || fileBuffer->magicNumber != 0x11) {
        CFileBuffer::FreeGenericFileBuffer(fileBuffer);
        return false;
    }

    memcpy(&m_gameInfo, fileBuffer, sizeof(GameInfo));

    g_pGraphics->resX = m_gameInfo.screenWidth;
    g_pGraphics->resY = m_gameInfo.screenHeight;
    g_pGraphics->depth = m_gameInfo.screenColourDepth;

    g_pGraphics->isFullscreen = IsFullscreen() & 0xff;

    graphicsOptions = m_gameInfo.graphicsOptions & 6;
    if (graphicsOptions == 2) {
        g_pGraphics->field913_0x3bc |= 8;
        g_pGraphics->field913_0x3bc &= 0xffffffef;
        g_pGraphics->field913_0x3bc &= 0xffffff7f;
    } else if (graphicsOptions == 4) {
        g_pGraphics->field913_0x3bc |= 8;
        g_pGraphics->field913_0x3bc |= 0x10;
        g_pGraphics->field913_0x3bc &= 0xffffff7f;
    } else if (graphicsOptions == 6) {
        g_pGraphics->field913_0x3bc |= 8;
        g_pGraphics->field913_0x3bc &= 0xffffffef;
        g_pGraphics->field913_0x3bc |= 0x80;
    } else {
        g_pGraphics->field913_0x3bc &= 0xfffffff7;
        g_pGraphics->field913_0x3bc &= 0xffffffef;
        g_pGraphics->field913_0x3bc &= 0xffffff7f;
    }

    g_pGraphics->field913_0x3bc = (m_gameInfo.graphicsOptions & 0x8) << 2 | g_pGraphics->field913_0x3bc & 0xffffffdf;
    g_pGraphics->field913_0x3bc = (m_gameInfo.graphicsOptions & 0x10) << 2 | g_pGraphics->field913_0x3bc & 0xffffffbf;

    graphicsOptions = m_gameInfo.graphicsOptions & 0xc0000;
    if (graphicsOptions == 0x40000) {
        g_pGraphics->field913_0x3bc |= 1;
        g_pGraphics->field913_0x3bc &= 0xfffffffd;
    } else if (graphicsOptions == 0x80000) {
        g_pGraphics->field913_0x3bc &= 0xfffffffe;
        g_pGraphics->field913_0x3bc |= 2;
    } else {
        g_pGraphics->field913_0x3bc &= 0xfffffffe;
        g_pGraphics->field913_0x3bc &= 0xfffffffd;
    }

    g_pGraphics->field917_0x3c0 = GetGraphicsOptionBits21To24();
    g_pGraphics->field913_0x3bc = (m_gameInfo.graphicsOptions >> 0x1b & 4) | g_pGraphics->field913_0x3bc & 0xfffffffb;

    if (IsRecordFlagSet(0x11) != 0)
        ToggleRecordFlag(0x11);

    SetFrontendSessionFlag(0);
    CFileBuffer::FreeGenericFileBuffer(fileBuffer);

    return true;
}

// FUNCTION: CMR2 0x00405b20
unsigned int CGameInfo::IsFullscreen(void) {
    return m_gameInfo.graphicsOptions & 1;
}

// FUNCTION: CMR2 0x00405ca0
int CGameInfo::GetGraphicsOptionBits21To24(void)
{
  return (int)(m_gameInfo.graphicsOptions >> 0x15 & 0xf);
}

// FUNCTION: CMR2 0x00406410
bool CGameInfo::IsRecordFlagSet(int param1) {
    bool response = m_gameInfo.field_0x38f0 & (1 << param1);
    return response;
}

// FUNCTION: CMR2 0x004eac50
bool CGameInfo::ToggleRecordFlag(int param1) {
    bool response = IsRecordFlagSet(param1);
    m_gameInfo.field_0x38f0 = m_gameInfo.field_0x38f0 ^ 1 << param1;
    return !response;
}

// GLOBAL: CMR2 0x00817780
int g_unk0x00817780;
// GLOBAL: CMR2 0x00817784
char g_unk0x00817784[5][256];
// GLOBAL: CMR2 0x00817c84
char *g_networkChatLines[5];

// FUNCTION: CMR2 0x004d05f0
void NetworkChat_ClearLog(void)
{
    char **pp = g_networkChatLines;
    char *p = g_unk0x00817784[0];

    do {
        *p = 0;
        *pp = p;
        p += 256;
        pp++;
    // 0x817c84 in the original (g_networkChatLines, the next global).
    } while ((INT_PTR)p < (INT_PTR)g_unk0x00817784[5]);
    g_unk0x00817780 = -1;
}

int Network_FindSessionPlayerIndex(NetPlayerID *pId, char *pIndex);
SessionPlayerRecord *Network_GetActiveSessionPlayerRecord(BYTE index);
char Network_SendPlayerMessage(int to, int guaranteed, void *data, int size);

// GLOBAL: CMR2 0x00523bbc
char g_chatLineFormat[] = "%s > %s";

// Adds a chat line "name > text" to the ring of the last five lines and
// rebuilds g_networkChatLines newest first.
// FUNCTION: CMR2 0x004d0620
void NetworkChat_AppendLine(NetPlayerID *pFrom, char *text, char local)
{
    int line;
    char **pp;
    int i;

    if (local) {
        if (++g_unk0x00817780 >= 5)
            g_unk0x00817780 = 0;
        sprintf(g_unk0x00817784[g_unk0x00817780], g_chatLineFormat, (char *)RallyData_GetRecord(0), text);
    } else {
        if (!Network_FindSessionPlayerIndex(pFrom, &local))
            return;
        if (++g_unk0x00817780 >= 5)
            g_unk0x00817780 = 0;
        sprintf(g_unk0x00817784[g_unk0x00817780], g_chatLineFormat, Network_GetActiveSessionPlayerRecord(local)->shortName, text);
    }
    line = g_unk0x00817780 + 5;
    pp = g_networkChatLines;
    i = 5;
    do {
        *pp = g_unk0x00817784[line-- % 5];
        pp++;
    } while (--i);
}

// Sends a chat line to every player and adds it to the local log.
// FUNCTION: CMR2 0x004d0700
void NetworkChat_SendLine(char *text)
{
    char message[0x101];

    message[0] = 0;
    strcpy(message + 1, text);
    Network_SendPlayerMessage(0, 0, message, 0x101);
    NetworkChat_AppendLine(NULL, text, 1);
}

// FUNCTION: CMR2 0x00406710
int GameInfo_GetSessionField397C(void)
{
    return CGameInfo::m_gameInfo.field_0x397c;
}

// FUNCTION: CMR2 0x00406730
int GameInfo_GetSessionField3984(void)
{
    return CGameInfo::m_gameInfo.field_0x3984;
}

// FUNCTION: CMR2 0x00406720
void GameInfo_SetSessionField397C(int param1)
{
    CGameInfo::m_gameInfo.field_0x397c = param1;
}

// FUNCTION: CMR2 0x00406740
void GameInfo_SetSessionField3984(int param1)
{
    CGameInfo::m_gameInfo.field_0x3984 = param1;
}

// FUNCTION: CMR2 0x00406750
int GameInfo_GetSessionField3988(void)
{
    return CGameInfo::m_gameInfo.field_0x3988;
}

// FUNCTION: CMR2 0x00406760
void GameInfo_SetSessionField3988(int param1)
{
    CGameInfo::m_gameInfo.field_0x3988 = param1;
}

// FUNCTION: CMR2 0x00406770
int GameInfo_GetSessionField398C(void)
{
    return CGameInfo::m_gameInfo.field_0x398c;
}

// FUNCTION: CMR2 0x00406790
int GameInfo_GetSessionField3990(void)
{
    return CGameInfo::m_gameInfo.field_0x3990;
}

// FUNCTION: CMR2 0x004067a0
void GameInfo_SetSessionField3990(int param1)
{
    CGameInfo::m_gameInfo.field_0x3990 = param1;
}

// FUNCTION: CMR2 0x00406780
void GameInfo_SetSessionField398C(int param1)
{
    CGameInfo::m_gameInfo.field_0x398c = param1;
}

// FUNCTION: CMR2 0x004d0580
unsigned char GameInfo_GetFrontendSessionFlag(void) {
    return CGameInfo::m_unk0x00817574;
}

// FUNCTION: CMR2 0x004d0590
void CGameInfo::SetFrontendSessionFlag(BYTE param1) {
    m_unk0x00817574 = param1;
}

// FUNCTION: CMR2 0x0049ea90
void CGameInfo::SetInputAndGamePaused(unsigned int param1) {
    SetInputPausedFlag(param1);
    m_fixedMenuKeys = param1;
}

// FUNCTION: CMR2 0x0049e930
void CGameInfo::SetInputPausedFlag(unsigned int param1) {
    m_unk0x00520870 = param1;
}

// FUNCTION: CMR2 0x004d05d0
void CGameInfo::InitFrontendSessionBuffer(void) {
    m_unk0x0081777c = 0;
    m_unk0x00817678 = FALSE;
    CGame::RegisterCallback(ReleaseFrontendSessionBuffer, NULL);
}

// FUNCTION: CMR2 0x004d05a0
bool CGameInfo::ReleaseFrontendSessionBuffer(void) {
    if (m_unk0x0081777c != NULL) {
        CFileBuffer::FreeGenericFileBuffer(m_unk0x0081777c);
        m_unk0x0081777c = NULL;
    }

    m_unk0x00817678 = FALSE;
    return true;
}

// FUNCTION: CMR2 0x004a0c60
void CGameInfo::ResetNetworkSessionState(void) {
    g_networkSession.lpszSessionNameA = (LPSTR)m_unk0x005a00b8;
    g_networkSession.lpszPasswordA = (LPSTR)m_unk0x005a02c0;
    m_unk0x005a0060 = FALSE;
    memset(m_enumeratedSessions, 0, sizeof(m_enumeratedSessions));
    m_unk0x005a1814 = FALSE;
    m_unk0x005a01bc = false;
}

// FUNCTION: CMR2 0x00405bd0
unsigned int CGameInfo::GetGraphicsOptionBits5To8(void) {
  return m_gameInfo.graphicsOptions >> 5 & 0xf;
}

// FUNCTION: CMR2 0x00405c00
unsigned int CGameInfo::GetGraphicsOptionBits9To12(void) {
  return m_gameInfo.graphicsOptions >> 9 & 0xf;
}

// GLOBAL: CMR2 0x0082af88
int g_unk0x0082af88;
// GLOBAL: CMR2 0x0082af8c
BYTE g_unk0x0082af8c;
// GLOBAL: CMR2 0x0082b0a8
int g_optionPlayerSlotOffset;
// GLOBAL: CMR2 0x0082b0ac
BYTE g_unk0x0082b0ac;
// GLOBAL: CMR2 0x0082ac58
int g_unk0x0082ac58;
// GLOBAL: CMR2 0x0082ac5c
int g_unk0x0082ac5c;
// GLOBAL: CMR2 0x0082b0a0
int g_unk0x0082b0a0;
// GLOBAL: CMR2 0x0082b2c0
Unk0x0082b2c0 g_optionRecordTransitions[8];
// Byte flag the option menu state (0x500c80) derives from the stage index.
// GLOBAL: CMR2 0x00527000
BYTE g_unk0x00527000 = 1;
// Option menu music track ("%s\\select3.adp").
// GLOBAL: CMR2 0x0052703c
char g_str0x0052703c[16] = "%s\\select3.adp";
// GLOBAL: CMR2 0x00526f54
int g_optionColumnLabelIds[7] = { 0, 1, 3, 4, 5, 5, 7 };
// GLOBAL: CMR2 0x00526f70
int g_optionColumnUnitIds[7] = { 0, 1, 2, 3, 4, 5, 6 };
// GLOBAL: CMR2 0x00526f8c
int g_optionColumnWeights[11] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
// GLOBAL: CMR2 0x00527098
int g_optionColumnBaseWeights[7] = { 12000, 30000, 48000, 42000, 24000, 24000, 30000 };
// GLOBAL: CMR2 0x005270b4
int g_recordGroupWeights[12] = { 0, 96000, 48000, 48000, 72000, 60000, 60000, 84000, 36000, 24000, 48000, 48000 };
// GLOBAL: CMR2 0x0082a90c
int g_optionSlotTypes[6];
// GLOBAL: CMR2 0x0082a928
int g_unk0x0082a928;
// GLOBAL: CMR2 0x0082a92c
int g_unk0x0082a92c;
// GLOBAL: CMR2 0x0082a930
int g_unk0x0082a930;
// GLOBAL: CMR2 0x0082af78
int g_unk0x0082af78[4];
// GLOBAL: CMR2 0x0082af90
int g_unk0x0082af90[4];
// GLOBAL: CMR2 0x0082b1b4
SceneNode *g_optionMenuSceneRoot;
// GLOBAL: CMR2 0x0082b1bc
int g_unk0x0082b1bc;
// GLOBAL: CMR2 0x0082b488
Menu g_advancedOptionsMenu;
// GLOBAL: CMR2 0x0082ba28
Menu g_optionStatusMenu;
// The option screens' car setups: rows 0..3 are each player's edited setup,
// rows 4..7 the setups they had when the screen was entered.
// GLOBAL: CMR2 0x0082bee8
CarSetup g_optionCarSetups[8];
// GLOBAL: CMR2 0x0082bf20
BYTE g_optionColumnDirty[16][7];
#include "OptionPreviewGeometry.h"
#include "CarImpact.h"

// Working copy of each player's car damage in the option (service) menus:
// OptionMenu_RevertRallyRecordGroup clears a damage group from records[] and
// sets dirty[player][group]; OptionMenu_ApplyRallyRecordGroup copies the group
// back from the driver's record and clears the flag.
struct PlayerOptionCache {
    BYTE dirty[4][12];
    RallyCarDamageState records[4];
};
CMR2_LAYOUT_CHECK(PlayerOptionCacheSize, sizeof(PlayerOptionCache) == 0x550);
// GLOBAL: CMR2 0x0082c040
PlayerOptionCache g_playerOptionCache;

// GLOBAL: CMR2 0x0082d220
OptionPreviewMeshRecord g_previewMeshRecords[16];
// GLOBAL: CMR2 0x00831778
Menu *g_pMenu0x00831778;
// GLOBAL: CMR2 0x0083177c
Menu *g_pMenu0x0083177c;
// The three option menu archives below are GenericFile records. Declared as
// GenericFile, FrontendText_LoadRegionLanguages and
// OptionMenu_DrawResultsRallyInfo lose bytes (register ties in this TU), so
// they stay int[4] and are read through GenericFile *.
// GLOBAL: CMR2 0x00831a90
int g_optionCommonArchive[4];
// GLOBAL: CMR2 0x00831aa0
int g_optionStageArchive[4];
// GLOBAL: CMR2 0x00831ab0
int g_optionLanguageArchive[4];

// FUNCTION: CMR2 0x004ff4b0
int OptionMenu_GetColumnLabelId(int index)
{
    return g_optionColumnLabelIds[index];
}

// FUNCTION: CMR2 0x004ff4c0
int OptionMenu_GetColumnUnitId(int index)
{
    return g_optionColumnUnitIds[index];
}

// FUNCTION: CMR2 0x004ff4d0
int OptionMenu_GetColumnWeight(int index)
{
    return g_optionColumnWeights[index];
}

// GLOBAL: CMR2 0x00526f44
int g_unk0x00526f44 = -1;
// GLOBAL: CMR2 0x00526f48
int g_unk0x00526f48 = -1;
// GLOBAL: CMR2 0x00526f4c
int g_unk0x00526f4c = -1;
// GLOBAL: CMR2 0x00526f50
int g_unk0x00526f50 = -1;

void OptionMenu_ResetRecordTransitions(int count);

// Clears the option menu counters and rebuilds the eight option records.
// FUNCTION: CMR2 0x004ff4e0
void OptionMenu_ResetRecords(void)
{
    g_unk0x00526f48 = 0;
    g_unk0x00526f4c = 0;
    g_unk0x00526f50 = 0;
    g_unk0x0082a92c = 0;
    g_unk0x0082ac58 = 0;
    g_unk0x0082ac5c = 0;
    g_optionSlotTypes[0] = 0;
    g_unk0x00526f44 = 2;
    g_optionSlotTypes[1] = 3;
    g_optionSlotTypes[2] = 3;
    g_optionSlotTypes[3] = 3;
    g_optionSlotTypes[4] = 3;
    g_optionSlotTypes[5] = 3;
    OptionMenu_ResetRecordTransitions(8);
}

// FUNCTION: CMR2 0x004ff540
int OptionMenu_GetScrollDistance(void)
{
    return g_unk0x0082a928 - g_unk0x0082a930;
}

// FUNCTION: CMR2 0x004ff5a0
int OptionMenu_GetSlotType(int index)
{
    return g_optionSlotTypes[index];
}

// FUNCTION: CMR2 0x005004a0
int OptionMenu_GetSharedValue(void)
{
    return g_unk0x0082a92c;
}

// FUNCTION: CMR2 0x005004b0
void OptionMenu_SetSharedValue(int value)
{
    g_unk0x0082a92c = value;
}

// FUNCTION: CMR2 0x00500520
void OptionMenu_ClearTimeout(void)
{
    g_unk0x0082ac58 = 0;
    g_unk0x0082ac5c = 0;
}

// FUNCTION: CMR2 0x00500530
void OptionMenu_UpdateTimeout(void)
{
    if (g_unk0x0082ac58 != 0 && (unsigned int)(CMain::GetFrameDelta() - g_unk0x0082ac5c) >= 30)
        OptionMenu_ClearTimeout();
}

// The option menu's interpolation base (0x82acf0) and the eight bytes at
// 0x82ace8 that the record loop walks over before it.
struct Unk0x0082ace8 {
    short pad[4];
    int pairs[16][2];
};
// GLOBAL: CMR2 0x0082ace8
Unk0x0082ace8 g_unk0x0082ace8;
// 16.16 coordinate pairs of the option menu's layout records: the current
// position, the target it animates to and the interpolated delta.
// GLOBAL: CMR2 0x0082ac68
int g_optionLayoutCoords[16][2];
// GLOBAL: CMR2 0x0082ad70
int g_unk0x0082ad70[16][2];
// GLOBAL: CMR2 0x0082adf0
int g_unk0x0082adf0[16][2];

// A 32-byte row of the option layout table: four slots of two coordinates.
struct Unk0x0082ac68Row {
    int v[4][2];
};

int OptionMenu_GetRecordTransitionMode(int index);

void Network_SetSessionStateFlag(BOOL param1);
void Network_RemoveSessionPlayerByID(DWORD *pId);
void NetPlayers_RemovePlayerByID(int *pId);

// Recomputes the four layout pairs of option record param1; param2 selects a
// straight copy of the target pairs instead of the param3 percent blend.
// FUNCTION: CMR2 0x00500920
void OptionMenu_InterpolateLayoutPairs(int param1, int param2, int param3)
{
    int i;

    if (param2 == 0) {
        for (i = 0; i < 4; i++) {
            g_optionLayoutCoords[param1 * 4 + i][0] =
                (g_unk0x0082adf0[param1 * 4 + i][0] * param3) / 100 + g_unk0x0082ace8.pairs[param1 * 4 + i][0];
            g_optionLayoutCoords[param1 * 4 + i][1] =
                (g_unk0x0082adf0[param1 * 4 + i][1] * param3) / 100 + g_unk0x0082ace8.pairs[param1 * 4 + i][1];
        }
        return;
    }
    for (i = 0; i < 8; i++)
        ((int *)g_optionLayoutCoords[param1 * 4])[i] = ((int *)g_unk0x0082ad70[param1 * 4])[i];
}

// Stores a new target pair per layout slot of option record param1 and rebuilds
// the deltas; param3 also warps the current pairs to the target.
// FUNCTION: CMR2 0x005009c0
void OptionMenu_SetTargetLayoutPairs(int param1, int *param2, int param3)
{
    int i;

    for (i = 0; i < 4; i++) {
        g_unk0x0082ace8.pairs[param1 * 4 + i][0] = g_optionLayoutCoords[param1 * 4 + i][0];
        g_unk0x0082ace8.pairs[param1 * 4 + i][1] = g_optionLayoutCoords[param1 * 4 + i][1];
        g_unk0x0082ad70[param1 * 4 + i][0] = param2[0];
        g_unk0x0082ad70[param1 * 4 + i][1] = param2[1];
        if (param3 != 0) {
            g_optionLayoutCoords[param1 * 4 + i][0] = param2[0];
            g_optionLayoutCoords[param1 * 4 + i][1] = param2[1];
            g_unk0x0082ace8.pairs[param1 * 4 + i][0] = g_unk0x0082ad70[param1 * 4 + i][0];
            g_unk0x0082ace8.pairs[param1 * 4 + i][1] = g_unk0x0082ad70[param1 * 4 + i][1];
        }
        g_unk0x0082adf0[param1 * 4 + i][0] =
            g_unk0x0082ad70[param1 * 4 + i][0] - g_unk0x0082ace8.pairs[param1 * 4 + i][0];
        g_unk0x0082adf0[param1 * 4 + i][1] =
            g_unk0x0082ad70[param1 * 4 + i][1] - g_unk0x0082ace8.pairs[param1 * 4 + i][1];
        param2 += 2;
    }
}

// Option menu item notification: refreshes the player list of a network device
// entry when it changes, or opens the advanced options on the select action.
// pFrom is the sender id (a DWORD); declaring it DWORD * flips a register
// tie in FrontendText_LoadRegionLanguages, as other symbol changes in this TU do.
// FUNCTION: CMR2 0x00500a70
void OptionMenu_HandleItemNotification(unsigned int *pFrom, int *param2)
{
    if (*param2 != NET_SYS_DESTROYPLAYER) {
        if (*param2 == NET_SYS_HOST)
            Network_SetSessionStateFlag(1);
        return;
    }
    NetMsgDestroyPlayer *message = (NetMsgDestroyPlayer *)param2;
    Network_RemoveSessionPlayerByID(&message->dpId);
    NetPlayers_RemovePlayerByID((int *)&message->dpId);
}

void NetPlayers_ReceiveStatistics(NetPlayerID *pId, NetStats *pStats);
void NetPlayers_MarkPlayerReadyByID(NetPlayerID *pId);
void NetPlayers_RecordPlayerFinishTime(NetPlayerID *pId, unsigned int time, int value);
void NetPlayers_MarkPlayerFinishedByID(NetPlayerID *pId);
void NetPlayers_RecordRemoteSplitTime(NetPlayerID *pId, int split, unsigned int time);
void NetPlayers_SetRemoteCarClass(NetPlayerID *pId, unsigned int carClass);
void NetPlayers_BuildFinalClassification(void);
void NetPlayers_ReceivePublishedLeaderboard(char valid, BYTE *p);
void OptionMenu_Start(char param1);
CallbackStateMachine *Game_GetSecondaryCallbackMachine(void);

// Handles an option menu notification of a network player: the first byte of the
// record selects the operation, the following ones carry its arguments.
// FUNCTION: CMR2 0x00500aa0
void OptionMenu_HandleNetworkNotification(NetPlayerID *pId, BYTE *pData)
{
    // the case order mirrors the original's jump table layout
    switch (pData[0]) {
    case 11:
        NetPlayers_ReceiveStatistics(pId, (NetStats *)(pData + 2));
        break;
    case 7:
        NetPlayers_MarkPlayerReadyByID(pId);
        break;
    case 8:
        NetPlayers_RecordRemoteSplitTime(pId, pData[1], *(unsigned int *)(pData + 4));
        break;
    case 10:
        NetPlayers_MarkPlayerFinishedByID(pId);
        NetPlayers_RecordPlayerFinishTime(pId, *(unsigned int *)(pData + 4), *(int *)(pData + 8));
        break;
    case 12:
        OptionMenu_Start(1);
        NetPlayers_BuildFinalClassification();
        break;
    case 6:
        NetPlayers_SetRemoteCarClass(pId, pData[1]);
        break;
    case 16:
        g_unk0x0082b0ac = 1;
        CGame::PromoteCallbackEntryByRule(Game_GetSecondaryCallbackMachine(), 0, 0, 2);
        NetPlayers_BuildFinalClassification();
        break;
    case 17:
        NetPlayers_ReceivePublishedLeaderboard(pData[1], pData + 4);
        NetPlayers_BuildFinalClassification();
        break;
    }
}

int Network_PollReceivedMessageBuffer(DWORD *param1, void **param2);

// Drains the pending network messages: those coming from the system id (0) go to
// the player-list handler, the rest to the option notification handler.
// FUNCTION: CMR2 0x00500ba0
void GameInfo_ProcessNetworkMessages(void)
{
    DWORD senderId;
    void *pMessage;

    while (Network_PollReceivedMessageBuffer(&senderId, &pMessage) != 0) {
        if (senderId == 0)
            OptionMenu_HandleItemNotification((unsigned int *)&senderId, (int *)pMessage);
        else
            OptionMenu_HandleNetworkNotification((NetPlayerID *)&senderId, (BYTE *)pMessage);
    }
}

// Arms the countdown of every option record that has not been started yet.
// FUNCTION: CMR2 0x00500ec0
void OptionMenu_ArmRecordCountdowns(void)
{
    if (OptionMenu_GetRecordTransitionMode(2) == 0)
        CGameInfo::StartOptionRecordTransition(2, 0x28, 0);
    if (OptionMenu_GetRecordTransitionMode(6) == 0)
        CGameInfo::StartOptionRecordTransition(6, 0xf, 0);
    if (OptionMenu_GetRecordTransitionMode(7) == 0)
        CGameInfo::StartOptionRecordTransition(7, 0xf, 0);
    if (OptionMenu_GetRecordTransitionMode(4) == 0)
        CGameInfo::StartOptionRecordTransition(4, 0x1e, 0);
    if (OptionMenu_GetRecordTransitionMode(5) == 0)
        CGameInfo::StartOptionRecordTransition(5, 0x1e, 1);
    if (OptionMenu_GetRecordTransitionMode(0) == 0) {
        CGameInfo::StartOptionRecordTransition(0, 0x14, 1);
        return;
    }
    if (OptionMenu_GetRecordTransitionMode(0) == 2) {
        if (OptionMenu_GetRecordTransitionMode(1) == 0)
            CGameInfo::StartOptionRecordTransition(1, 0x14, 0);
        if (OptionMenu_GetRecordTransitionMode(3) == 0)
            CGameInfo::StartOptionRecordTransition(3, 0x14, 0);
    }
}

void OptionMenu_ResetModeCursor(void);
extern FixAngles g_previewPartRotationPresets[12];
extern PreviewProjectedQuad g_previewProjectedPartQuads[12];
void OptionPreview_SetWheelNodesVisible(int index, char visible);
void OptionPreview_SetTargetModelAngles(int param1, FixAngles *param2, int param3);

// Boot state of the country menus: marks the item of the selected country and
// points its two record tables at the matching entry.
// FUNCTION: CMR2 0x00500df0
void OptionMenu_EnterCountryState(CallbackStateMachine *p1, BYTE p2)
{
    int i;

    OptionMenu_ResetModeCursor();
    for (i = 0; i < (int)(CGameInfo::GetConfiguredPlayerCount() & 0xff); i = i + 1) {
        if (i == CGameInfo::GetActiveOptionSlot()) {
            OptionPreview_SetWheelNodesVisible(i, 1);
            OptionPreview_SetTargetModelAngles(i, &g_previewPartRotationPresets[OptionMenu_GetColumnWeight(
                             OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 1)].max)], 0);
            OptionMenu_SetTargetLayoutPairs(i, &g_previewProjectedPartQuads[OptionMenu_GetColumnWeight(
                             OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 1)].max)].points[0].x, 1);
        } else {
            OptionPreview_SetWheelNodesVisible(i, 0);
        }
    }
    CGame::PromoteCallbackEntryByRule(p1, p2, 0, 2);
    g_unk0x0082b0a0 = CMain::GetFrameDelta();
}

// FUNCTION: CMR2 0x005011d0
int OptionMenu_GetPlayerSlotOffset(void)
{
    return g_optionPlayerSlotOffset;
}

// FUNCTION: CMR2 0x005011e0
void OptionMenu_SetPlayerSlotOffset(int value)
{
    g_optionPlayerSlotOffset = value;
}

// FUNCTION: CMR2 0x005011f0
int OptionMenu_GetSlotSelector(int index)
{
    return g_unk0x0082af90[index];
}

// FUNCTION: CMR2 0x00501200
int OptionMenu_GetSlotBaseSelector(int index)
{
    return g_unk0x0082af78[index];
}

// FUNCTION: CMR2 0x00501210
void OptionMenu_SetSlotSelector(int index, int value)
{
    g_unk0x0082af90[index] = value;
}

// FUNCTION: CMR2 0x00501510
SceneNode *OptionMenu_GetBackgroundRoot(void)
{
    return g_optionMenuSceneRoot;
}

// PORT: the service area's car preview projection. The original's X scale
// fits 4:3 only and stretches the car on wider screens; (4/3) * (resY/resX)
// is the same at 4:3 and keeps the car's proportions on any other shape, as
// View_SetPlayerProjection does for the stage.
static void OptionMenu_SetPreviewProjection(void)
{
    CGraphics::SetProjection(FTOI(int, X87S_DIV(X87S_MUL(0x30978 * (4.0 / 3.0), g_pGraphics->resY), g_pGraphics->resX)),
                             0x4326e, 0xfa0000, 0x10000);
}

void OptionMenu_CreateBackgroundWorld(void);
void OptionMenu_LoadArchives(void);
void OptionPreview_FillTextureSetIndices(void);
void RallyData_LoadCurrentRallyWeatherTextures(void);
void RallyData_SetupWeatherTextureEntries(void);
GenericFile *OptionMenu_GetCommonArchive(void);
void Particle_BuildTriangleStripIndices(void);
void Graphics_SetShadowGeometryState(int value);

// Format of the environment texture of the track. The %s is the install
// directory returned by the setup.
// GLOBAL: CMR2 0x00527070
char g_str0x00527070[] = "%s\\textures\\environment\\environment.tga";

// Starts the option menu background: clear colour and blend mode, then the
// option menu data, the world, the weather textures and their stage entries.
// Finally it registers the environment texture of the track, at the resolution
// the display option asks for.
// The only differences against the original are the call sites reccmp shows as
// <OFFSETn>: 0x4b1150, 0x503ea0, 0x5040f0 and the two 0x4b23c0 calls go to
// functions that are still annotated TODO, so reccmp cannot name them.
// FUNCTION: CMR2 0x00501520
void OptionMenu_InitBackground(void)
{
    CGraphics::SetClearColour(1, 0x8d, 0x97, 0x9f);
    Font_SetBlendMode(1);
    CGameInfo::SaveOptionMenuCallbackCount();
    OptionMenu_CreateBackgroundWorld();
    OptionMenu_LoadArchives();
    Particle_BuildTriangleStripIndices();
    CGame::SetSectorDrawState(3);
    RallyData_LoadCurrentRallyWeatherTextures();
    RallyData_SetupWeatherTextureEntries();
    OptionPreview_FillTextureSetIndices();
    sprintf(CFrontend::m_stringDest, g_str0x00527070, CInstallInfo::GetSetupRepDir());
    Graphics_SetShadowGeometryState(!CGameInfo::IsRecordFlagSet(0xf));
    if (CGameInfo::IsRecordFlagSet(0x10))
        Graphics_ReserveCubeMapsAndLoadEnvironment(CFrontend::m_stringDest, 0, OptionMenu_GetCommonArchive(), 0x80);
    else
        Graphics_ReserveCubeMapsAndLoadEnvironment(CFrontend::m_stringDest, 0, OptionMenu_GetCommonArchive(), 0x40);
}

// FUNCTION: CMR2 0x00501d00
void OptionMenu_ClearRecordTransition(int index)
{
    Unk0x0082b2c0 *p = &g_optionRecordTransitions[index];

    p->field_0x8 = 0;
    p->field_0x4 = 0;
    p->field_0x0 = 0;
    p->field_0xc = 0;
}

// FUNCTION: CMR2 0x00501d20
void OptionMenu_ResetRecordTransitions(int count)
{
    int i;

    g_unk0x0082b1bc = count;
    for (i = 0; i < g_unk0x0082b1bc; i++)
        OptionMenu_ClearRecordTransition(i);
}

// FUNCTION: CMR2 0x005021c0
int OptionMenu_GetRecordTransitionMode(int index)
{
    return g_optionRecordTransitions[index].field_0xc;
}

// FUNCTION: CMR2 0x005021e0
void OptionMenu_FinishRecordTransitions(void)
{
    int i;
    Unk0x0082b2c0 *p;

    if (g_unk0x0082b1bc > 0) {
        p = g_optionRecordTransitions;
        i = g_unk0x0082b1bc;
        do {
            p->field_0x0 = 0x10000;
            p->field_0xc = 2;
            p++;
        } while (--i != 0);
    }
}

// FUNCTION: CMR2 0x00502210
Menu *OptionMenu_GetStatusMenu(void)
{
    return &g_optionStatusMenu;
}

// FUNCTION: CMR2 0x00502220
Menu *OptionMenu_GetAdvancedOptionsMenu(void)
{
    return &g_advancedOptionsMenu;
}

// FUNCTION: CMR2 0x00502990
BYTE OptionMenu_GetColumnDirtyFlag(int i, int j)
{
    return g_optionColumnDirty[i][j];
}

// Option menu builders and loaders that live in the cascade scaffold.
void OptionMenu_RefreshStageSetup(void);
void OptionMenu_LoadRallyRecords(void);
void OptionMenu_LoadCarSetups(void);
void OptionMenu_InitBackground(void);
BYTE OptionMenu_LoadSounds(void);
void OptionMenu_ConfigureSoundHandles(void);
void OptionMenu_LoadFonts(void);
BYTE OptionMenu_LoadRegionalText(void);
void OptionMenu_BuildAndActivatePages(void);
void SavedGames_LoadRecords(void);
bool SavedGames_ReleaseRecords(void);
void NetPlayers_SendCarClass(BYTE carClass);
HRESULT Sound_StartLoopingMusicStream(int param1);

// Starts the option menu state: rebuilds the option records, starts the menu
// music and asks for the next state (level 2) of the grouped callback machine.
// FUNCTION: CMR2 0x00500c80
void OptionMenu_EnterStartState(CallbackStateMachine *p1, BYTE state)
{
    char path[MAX_PATH];
    int i;

    g_unk0x0082b0ac = 0;
    CGameInfo::SetInputAndGamePaused(1);
    if (CGameInfo::GetGameModeOptionBit19() != 0)
        NetPlayers_SendCarClass(1);
    Menu_SetInputStateFlag(0);
    CInput::SetMenuSounds(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);
    g_optionPlayerSlotOffset = CGameInfo::GetConfiguredPlayerCount() & 0xff;
    OptionMenu_RefreshStageSetup();
    OptionMenu_LoadRallyRecords();
    OptionMenu_LoadCarSetups();
    OptionMenu_InitBackground();
    SavedGames_LoadRecords();
    CGame::RegisterCallback(SavedGames_ReleaseRecords, NULL);
    Sound_Init(0x5622, 2, 0x10, 0);
    OptionMenu_LoadSounds();
    OptionMenu_ConfigureSoundHandles();
    OptionMenu_LoadFonts();
    OptionMenu_LoadRegionalText();
    OptionMenu_BuildAndActivatePages();

    if (!(CGameInfo::GetConfiguredGameMode() == 0 || CGameInfo::GetConfiguredGameMode() == 1)) {
        g_unk0x00527000 = 1;
    } else {
        g_unk0x00527000 = 0;
        if ((int)RallyDataStageIndex() % 4 == 0)
            g_unk0x00527000 = 1;
    }

    // The original re-reads the flag before each pair of stores: both arms of
    // the conditional hold the same value, so the branch is gone from the
    // binary but the load and its test are still there.
    for (i = 0; i < (int)(CGameInfo::GetConfiguredPlayerCount() & 0xff); i = i + 1) {
        if (g_unk0x00527000) {
            g_unk0x0082af90[i] = 360000;
            g_unk0x0082af78[i] = 0x3c;
        } else {
            g_unk0x0082af90[i] = 360000;
            g_unk0x0082af78[i] = 0x3c;
        }
    }

    sprintf(path, g_str0x0052703c, CInstallInfo::GetMusicDir());
    CSound::OpenStreamingMusicFile(path);
    CSound::SetMusicStreamVolume(CGameInfo::GetMasterSoundVolume());
    Sound_StartLoopingMusicStream(1);
    CGame::PromoteCallbackEntryByRule(p1, state, 0, 2);
    g_unk0x0082af8c = 1;
}

// FUNCTION: CMR2 0x00502d40
int OptionMenu_GetColumnBaseWeight(int index)
{
    return g_optionColumnBaseWeights[index];
}

RallyCarDamageState *RallyData_GetDriverGroupRecord(int index);

// Copies every option record from the rally data into the working table and
// clears the per-record dirty words.
// FUNCTION: CMR2 0x00502d50
void OptionMenu_LoadRallyRecords(void)
{
    int i;
    int j;
    RallyCarDamageState *pDst;
    int *pDirty;
    RallyCarDamageState *pSrc;

    i = 0;
    if (CGameInfo::GetConfiguredPlayerCount() > 0) {
        pDirty = (int *)g_playerOptionCache.dirty;
        pDst = g_playerOptionCache.records;
        do {
            pSrc = RallyData_GetDriverGroupRecord(i);
            i++;
            memcpy(pDst, pSrc, sizeof(RallyCarDamageState));
            for (j = 0; j < 3; j++)
                pDirty[j] = 0;
            pDst++;
            pDirty += 3;
        } while (i < (int)(CGameInfo::GetConfiguredPlayerCount() & 0xff));
    }
}

// FUNCTION: CMR2 0x00503930
int OptionMenu_GetRecordGroupWeight(int index)
{
    return g_recordGroupWeights[index];
}

// FUNCTION: CMR2 0x00503940
BYTE OptionMenu_GetRecordGroupAppliedFlag(int i, int j)
{
    return g_playerOptionCache.dirty[i][j];
}

// FUNCTION: CMR2 0x00509d00
void OptionPreview_ClearCornerHeightOffsets(int index)
{
    OptionPreviewMeshRecord *pRec = &g_previewMeshRecords[index];
    int i;

    for (i = 0; i < 4; i++)
        pRec->cornerHeightOffsets[i] = 0;
}

// FUNCTION: CMR2 0x00509d90
void OptionPreview_ClearPartDamageFractions(int index)
{
    OptionPreviewMeshRecord *pRec = &g_previewMeshRecords[index];
    int i;

    for (i = 0; i < 4; i++)
        pRec->partDamageFractions[i] = 0;
}

// FUNCTION: CMR2 0x0050a020
int OptionPreview_UsesWheelVariantSix(int mode, int type)
{
    if (type == 6 && mode != 11 && mode != 8 && mode != 10 && mode != 13)
        return 1;
    return 0;
}

// FUNCTION: CMR2 0x0050a050
int OptionPreview_UsesLowWheelVariant(int mode, int type)
{
    if (type <= 1 && mode != 11 && mode != 8 && mode != 10 && mode != 13)
        return 1;
    return 0;
}

// Rotation presets of the twelve selectable preview part categories.
// GLOBAL: CMR2 0x005273c0
FixAngles g_previewPartRotationPresets[12] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0xfc00, 0x0000, 0x0000, 0x0000,
    0x0000, 0x01c7, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0400, 0x0000, 0x0000, 0x0000, 0x0000, 0x01c7, 0x0000, 0x0000,
    0xfc00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0238, 0x0000, 0x0000,
    0x0000, 0x0400, 0xffbc, 0x0000, 0x0400, 0x0000, 0x0000, 0x0000,
    0x0400, 0x0000, 0x0000, 0x0000, 0x0400,
};

// Reference hull points of the option menu car preview: one 12x4 block of vertices
// per car (14 cars, one per id of g_unk0x00516b40.ids).
// GLOBAL: CMR2 0x00527420
PreviewModelReferenceGeometry g_previewReferenceModels[14] = {
    { {
        { { 78577, -48496, 49807 }, { 78577, -48496, -49807 }, { -96927, -48496, 49807 }, { -96927, -48496, -49807 } },
        { { 22937, -19660, 0 }, { 22937, -19660, 0 }, { 22937, -19660, 0 }, { 22937, -19660, 0 } },
        { { 67436, 5898, 0 }, { 67436, 5898, 0 }, { 67436, 5898, 0 }, { 67436, 5898, 0 } },
        { { 81199, -3276, 57671 }, { 81199, -3276, -57671 }, { -92340, -3276, 57671 }, { -92340, -3276, -57671 } },
        { { 75300, 38010, 0 }, { 75300, 38010, 0 }, { -81199, 38010, 0 }, { -81199, 38010, 0 } },
        { { 81199, -3276, 57671 }, { 81199, -3276, -57671 }, { -92340, -3276, 57671 }, { -92340, -3276, -57671 } },
        { { 106102, 11796, 0 }, { 106102, 11796, 0 }, { -43253, 54394, 0 }, { -43253, 54394, 0 } },
        { { 78577, -48496, 49807 }, { 78577, -48496, 49807 }, { 78577, -48496, -49807 }, { 78577, -48496, -49807 } },
        { { 121831, 0, 40632 }, { 121831, 0, 40632 }, { 121831, 0, -40632 }, { 121831, 0, -40632 } },
        { { -134742, -27459, -34013 }, { -134742, -27459, -34013 }, { -134742, -27459, -34013 }, { -134742, -27459, -34013 } },
        { { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 } },
        { { 75300, -38010, 0 }, { 75300, -38010, 0 }, { -81199, -38010, 0 }, { -81199, -38010, 0 } }
    } },
    { {
        { { 85131, -51118, 49807 }, { 85131, -51118, -49807 }, { -83820, -51118, 49807 }, { -83820, -49020, -49807 } },
        { { 22937, -19660, 0 }, { 22937, -19660, 0 }, { 22937, -19660, 0 }, { 22937, -19660, 0 } },
        { { 67436, 5898, 0 }, { 67436, 5898, 0 }, { 67436, 5898, 0 }, { 67436, 5898, 0 } },
        { { 87752, -3538, 57671 }, { 87752, -3538, -57671 }, { -79233, -3538, 57671 }, { -79233, -3538, -57671 } },
        { { 75300, 38010, 0 }, { 75300, 38010, 0 }, { -81199, 38010, 0 }, { -81199, 38010, 0 } },
        { { 87752, -3538, 57671 }, { 87752, -3538, -57671 }, { -79233, -3538, 57671 }, { -79233, -3538, -57671 } },
        { { 106102, 11796, 0 }, { 106102, 11796, 0 }, { -43253, 54394, 0 }, { -43253, 54394, 0 } },
        { { 85131, -48758, 49807 }, { 85131, -48758, 49807 }, { 85131, -48758, -49807 }, { 85131, -48758, -49807 } },
        { { 121831, 0, 40632 }, { 121831, 0, 40632 }, { 121831, 0, -40632 }, { 121831, 0, -40632 } },
        { { -136052, -26411, 31260 }, { -136052, -26411, 31260 }, { -136052, -26411, 31260 }, { -136052, -26411, 31260 } },
        { { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 } },
        { { 75300, -38010, 0 }, { 75300, -38010, 0 }, { -81199, -38010, 0 }, { -81199, -38010, 0 } }
    } },
    { {
        { { 78577, -48496, 49807 }, { 78577, -48496, -49807 }, { -85196, -48496, 49807 }, { -85196, -48496, -49807 } },
        { { 22937, -19660, 0 }, { 22937, -19660, 0 }, { 22937, -19660, 0 }, { 22937, -19660, 0 } },
        { { 67436, 5898, 0 }, { 67436, 5898, 0 }, { 67436, 5898, 0 }, { 67436, 5898, 0 } },
        { { 81199, -3276, 57671 }, { 81199, -3276, -57671 }, { -85196, -3276, 57671 }, { -85196, -3276, -57671 } },
        { { 75300, 38010, 0 }, { 75300, 38010, 0 }, { -81199, 38010, 0 }, { -81199, 38010, 0 } },
        { { 81199, -3276, 57671 }, { 81199, -3276, -57671 }, { -85196, -3276, 57671 }, { -85196, -3276, -57671 } },
        { { 106102, 11796, 0 }, { 106102, 11796, 0 }, { -43253, 54394, 0 }, { -43253, 54394, 0 } },
        { { 78577, -48496, 49807 }, { 78577, -48496, 49807 }, { 78577, -48496, -49807 }, { 78577, -48496, -49807 } },
        { { 121831, 0, 36700 }, { 121831, 0, 36700 }, { 121831, 0, -36700 }, { 121831, 0, -36700 } },
        { { -126156, -27656, -28508 }, { -126156, -27656, -28508 }, { -126156, -27656, -28508 }, { -126156, -27656, -28508 } },
        { { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 } },
        { { 75300, -38010, 0 }, { 75300, -38010, 0 }, { -81199, -38010, 0 }, { -81199, -38010, 0 } }
    } },
    { {
        { { 80543, -43909, 55705 }, { 80543, -43909, -55705 }, { -84475, -43909, 55705 }, { -84475, -43909, -55705 } },
        { { 39321, -11796, 0 }, { 39321, -11796, 0 }, { 39321, -11796, 0 }, { 39321, -11796, 0 } },
        { { 86441, 6553, 0 }, { 86441, 6553, 0 }, { 86441, 6553, 0 }, { 86441, 6553, 0 } },
        { { 80543, -1310, 55705 }, { 80543, -1310, -55705 }, { -84475, -1310, 55705 }, { -84475, -1310, -55705 } },
        { { 75300, 38010, 0 }, { 75300, 38010, 0 }, { -81199, 38010, 0 }, { -81199, 38010, 0 } },
        { { 80543, -1310, 55705 }, { 80543, -1310, -55705 }, { -84475, -1310, 55705 }, { -84475, -1310, -55705 } },
        { { 106102, 11796, 0 }, { 106102, 11796, 0 }, { -43253, 54394, 0 }, { -43253, 54394, 0 } },
        { { 80543, -46530, 47185 }, { 80543, -46530, 47185 }, { 80543, -46530, -47185 }, { 80543, -46530, -47185 } },
        { { 127008, -3276, 35389 }, { 127008, -3276, 35389 }, { 127008, -3276, -35389 }, { 127008, -3276, -35389 } },
        { { -139853, -28639, -21299 }, { -139853, -28639, -21299 }, { -139853, -28639, -21299 }, { -139853, -28639, -21299 } },
        { { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 } },
        { { 75300, -38010, 0 }, { 75300, -38010, 0 }, { -81199, -38010, 0 }, { -81199, -38010, 0 } }
    } },
    { {
        { { 78577, -48496, 49807 }, { 78577, -48496, -49807 }, { -96927, -48496, 49807 }, { -96927, -48496, -49807 } },
        { { 22937, -19660, 0 }, { 22937, -19660, 0 }, { 22937, -19660, 0 }, { 22937, -19660, 0 } },
        { { 67436, 5898, 0 }, { 67436, 5898, 0 }, { 67436, 5898, 0 }, { 67436, 5898, 0 } },
        { { 81199, -3276, 57671 }, { 81199, -3276, -57671 }, { -92340, -3276, 57671 }, { -92340, -3276, -57671 } },
        { { 75300, 38010, 0 }, { 75300, 38010, 0 }, { -81199, 38010, 0 }, { -81199, 38010, 0 } },
        { { 81199, -3276, 57671 }, { 81199, -3276, -57671 }, { -92340, -3276, 57671 }, { -92340, -3276, -57671 } },
        { { 106102, 11796, 0 }, { 106102, 11796, 0 }, { -43253, 54394, 0 }, { -43253, 54394, 0 } },
        { { 78577, -48496, 49807 }, { 78577, -48496, 49807 }, { 78577, -48496, -49807 }, { 78577, -48496, -49807 } },
        { { 121831, 0, 40632 }, { 121831, 0, 40632 }, { 121831, 0, -40632 }, { 121831, 0, -40632 } },
        { { -129236, -31326, 32243 }, { -129236, -31326, 32243 }, { -129236, -31326, 32243 }, { -129236, -31326, 32243 } },
        { { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 } },
        { { 75300, -38010, 0 }, { 75300, -38010, 0 }, { -81199, -38010, 0 }, { -81199, -38010, 0 } }
    } },
    { {
        { { 78577, -48496, 49807 }, { 78577, -48496, -49807 }, { -96927, -48496, 49807 }, { -96927, -48496, -49807 } },
        { { 22937, -19660, 0 }, { 22937, -19660, 0 }, { 22937, -19660, 0 }, { 22937, -19660, 0 } },
        { { 67436, 5898, 0 }, { 67436, 5898, 0 }, { 67436, 5898, 0 }, { 67436, 5898, 0 } },
        { { 81199, -3276, 57671 }, { 81199, -3276, -57671 }, { -92340, -3276, 57671 }, { -92340, -3276, -57671 } },
        { { 75300, 38010, 0 }, { 75300, 38010, 0 }, { -81199, 38010, 0 }, { -81199, 38010, 0 } },
        { { 81199, -3276, 57671 }, { 81199, -3276, -57671 }, { -92340, -3276, 57671 }, { -92340, -3276, -57671 } },
        { { 106102, 11796, 0 }, { 106102, 11796, 0 }, { -43253, 54394, 0 }, { -43253, 54394, 0 } },
        { { 78577, -48496, 49807 }, { 78577, -48496, 49807 }, { 78577, -48496, -49807 }, { 78577, -48496, -49807 } },
        { { 121831, 0, 40632 }, { 121831, 0, 40632 }, { 121831, 0, -40632 }, { 121831, 0, -40632 } },
        { { -134217, -21757, 30539 }, { -134217, -21757, 30539 }, { -134217, -21757, 30539 }, { -134217, -21757, 30539 } },
        { { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 } },
        { { 75300, -38010, 0 }, { 75300, -38010, 0 }, { -81199, -38010, 0 }, { -81199, -38010, 0 } }
    } },
    { {
        { { 78577, -48496, 49807 }, { 78577, -48496, -49807 }, { -96927, -48496, 49807 }, { -96927, -48496, -49807 } },
        { { 22937, -19660, 0 }, { 22937, -19660, 0 }, { 22937, -19660, 0 }, { 22937, -19660, 0 } },
        { { 67436, 5898, 0 }, { 67436, 5898, 0 }, { 67436, 5898, 0 }, { 67436, 5898, 0 } },
        { { 81199, -3276, 57671 }, { 81199, -3276, -57671 }, { -92340, -3276, 57671 }, { -92340, -3276, -57671 } },
        { { 75300, 38010, 0 }, { 75300, 38010, 0 }, { -81199, 38010, 0 }, { -81199, 38010, 0 } },
        { { 81199, -3276, 57671 }, { 81199, -3276, -57671 }, { -92340, -3276, 57671 }, { -92340, -3276, -57671 } },
        { { 106102, 11796, 0 }, { 106102, 11796, 0 }, { -43253, 54394, 0 }, { -43253, 54394, 0 } },
        { { 78577, -48496, 49807 }, { 78577, -48496, 49807 }, { 78577, -48496, -49807 }, { 78577, -48496, -49807 } },
        { { 121831, 0, 40632 }, { 121831, 0, 40632 }, { 121831, 0, -40632 }, { 121831, 0, -40632 } },
        { { -124518, -28835, -28246 }, { -124518, -28835, -28246 }, { -124518, -28835, -28246 }, { -124518, -28835, -28246 } },
        { { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 } },
        { { 75300, -38010, 0 }, { 75300, -38010, 0 }, { -81199, -38010, 0 }, { -81199, -38010, 0 } }
    } },
    { {
        { { 80543, -43909, 55705 }, { 80543, -43909, -55705 }, { -84475, -43909, 55705 }, { -84475, -43909, -55705 } },
        { { 39321, -11796, 0 }, { 39321, -11796, 0 }, { 39321, -11796, 0 }, { 39321, -11796, 0 } },
        { { 86441, 6553, 0 }, { 86441, 6553, 0 }, { 86441, 6553, 0 }, { 86441, 6553, 0 } },
        { { 80543, -1310, 55705 }, { 80543, -1310, -55705 }, { -84475, -1310, 55705 }, { -84475, -1310, -55705 } },
        { { 75300, 38010, 0 }, { 75300, 38010, 0 }, { -81199, 38010, 0 }, { -81199, 38010, 0 } },
        { { 80543, -1310, 55705 }, { 80543, -1310, -55705 }, { -84475, -1310, 55705 }, { -84475, -1310, -55705 } },
        { { 106102, 11796, 0 }, { 106102, 11796, 0 }, { -43253, 54394, 0 }, { -43253, 54394, 0 } },
        { { 80543, -46530, 47185 }, { 80543, -46530, 47185 }, { 80543, -46530, -47185 }, { 80543, -46530, -47185 } },
        { { 127008, -3276, 35389 }, { 127008, -3276, 35389 }, { 127008, -3276, -35389 }, { 127008, -3276, -35389 } },
        { { -154140, -26083, -17498 }, { -154140, -26083, -17498 }, { -154140, -26083, -17498 }, { -154140, -26083, -17498 } },
        { { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 } },
        { { 75300, -38010, 0 }, { 75300, -38010, 0 }, { -81199, -38010, 0 }, { -81199, -38010, 0 } }
    } },
    { {
        { { 66191, -45875, 43909 }, { 66191, -45875, -43909 }, { -70057, -45875, 43909 }, { -70057, -45875, -43909 } },
        { { 37355, -11141, 0 }, { 37355, -11141, 0 }, { 37355, -11141, 0 }, { 37355, -11141, 0 } },
        { { 61538, 8519, 0 }, { 61538, 8519, 0 }, { 61538, 8519, 0 }, { 61538, 8519, 0 } },
        { { 66191, -9830, 43909 }, { 66191, -9830, -43909 }, { -70057, -9830, 43909 }, { -70057, -9830, -43909 } },
        { { 58327, -28835, 0 }, { 58327, -28835, 0 }, { -58327, -28835, 0 }, { -58327, -28835, 0 } },
        { { 66191, -9830, 43909 }, { 66191, -9830, -43909 }, { -70057, -9830, 43909 }, { -70057, -9830, -43909 } },
        { { 73990, 17694, 0 }, { 73990, 17694, 0 }, { -20971, 52428, 0 }, { -20971, 52428, 0 } },
        { { 66191, -45875, 43909 }, { 66191, -45875, 43909 }, { 66191, -45875, -43909 }, { 66191, -45875, -43909 } },
        { { 108068, 3932, 34734 }, { 108068, 3932, 34734 }, { 108068, 3932, -34734 }, { 108068, 3932, -34734 } },
        { { -102498, -28377, 196 }, { -102498, -28377, 196 }, { -102498, -28377, 196 }, { -102498, -28377, 196 } },
        { { 65, -30801, 0 }, { 65, -30801, 0 }, { 65, -30801, 0 }, { 65, -30801, 0 } },
        { { 58327, -28835, 0 }, { 58327, -28835, 0 }, { -58327, -28835, 0 }, { -58327, -28835, 0 } }
    } },
    { {
        { { 78577, -48496, 49807 }, { 78577, -48496, -49807 }, { -96927, -48496, 49807 }, { -96927, -48496, -49807 } },
        { { 22937, -19660, 0 }, { 22937, -19660, 0 }, { 22937, -19660, 0 }, { 22937, -19660, 0 } },
        { { 67436, 5898, 0 }, { 67436, 5898, 0 }, { 67436, 5898, 0 }, { 67436, 5898, 0 } },
        { { 81199, -3276, 57671 }, { 81199, -3276, -57671 }, { -92340, -3276, 57671 }, { -92340, -3276, -57671 } },
        { { 75300, 38010, 0 }, { 75300, 38010, 0 }, { -81199, 38010, 0 }, { -81199, 38010, 0 } },
        { { 81199, -3276, 57671 }, { 81199, -3276, -57671 }, { -92340, -3276, 57671 }, { -92340, -3276, -57671 } },
        { { 106102, 11796, 0 }, { 106102, 11796, 0 }, { -43253, 54394, 0 }, { -43253, 54394, 0 } },
        { { 78577, -48496, 49807 }, { 78577, -48496, 49807 }, { 78577, -48496, -49807 }, { 78577, -48496, -49807 } },
        { { 121831, 0, 40632 }, { 121831, 0, 40632 }, { 121831, 0, -40632 }, { 121831, 0, -40632 } },
        { { -121896, -31784, -36438 }, { -121896, -31784, 36438 }, { -121896, -31784, -36438 }, { -121896, -31784, 36438 } },
        { { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 } },
        { { 75300, -38010, 0 }, { 75300, -38010, 0 }, { -81199, -38010, 0 }, { -81199, -38010, 0 } }
    } },
    { {
        { { 73334, -41287, 42598 }, { 73334, -41287, -42598 }, { -79888, -41287, 42598 }, { -79888, -41287, -42598 } },
        { { 22937, -19660, 0 }, { 22937, -19660, 0 }, { 22937, -19660, 0 }, { 22937, -19660, 0 } },
        { { 67436, 5898, 0 }, { 67436, 5898, 0 }, { 67436, 5898, 0 }, { 67436, 5898, 0 } },
        { { 73334, -1966, 42598 }, { 73334, -1966, -42598 }, { -79888, -1966, 42598 }, { -79888, -1966, -42598 } },
        { { 75300, 38010, 0 }, { 75300, 38010, 0 }, { -81199, 38010, 0 }, { -81199, 38010, 0 } },
        { { 73334, -1966, 42598 }, { 73334, -1966, -42598 }, { -79888, -1966, 42598 }, { -79888, -1966, -42598 } },
        { { 106102, 11796, 0 }, { 106102, 11796, 0 }, { -43253, 54394, 0 }, { -43253, 54394, 0 } },
        { { 73334, -41287, 42598 }, { 73334, -41287, 42598 }, { 73334, -41287, -42598 }, { 73334, -41287, -42598 } },
        { { 121831, 0, 40632 }, { 121831, 0, 40632 }, { 121831, 0, -40632 }, { 121831, 0, -40632 } },
        { { -115015, -17694, -25952 }, { -115015, -17694, 25952 }, { -115015, -17694, -25952 }, { -115015, -17694, 25952 } },
        { { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 } },
        { { 75300, -38010, 0 }, { 75300, -38010, 0 }, { -81199, -38010, 0 }, { -81199, -38010, 0 } }
    } },
    { {
        { { 78577, -48496, 49807 }, { 78577, -48496, -49807 }, { -96927, -48496, 49807 }, { -96927, -48496, -49807 } },
        { { 22937, -19660, 0 }, { 22937, -19660, 0 }, { 22937, -19660, 0 }, { 22937, -19660, 0 } },
        { { 67436, 5898, 0 }, { 67436, 5898, 0 }, { 67436, 5898, 0 }, { 67436, 5898, 0 } },
        { { 81199, -3276, 57671 }, { 81199, -3276, -57671 }, { -92340, -3276, 57671 }, { -92340, -3276, -57671 } },
        { { 75300, 38010, 0 }, { 75300, 38010, 0 }, { -81199, 38010, 0 }, { -81199, 38010, 0 } },
        { { 81199, -3276, 57671 }, { 81199, -3276, -57671 }, { -92340, -3276, 57671 }, { -92340, -3276, -57671 } },
        { { 106102, 11796, 0 }, { 106102, 11796, 0 }, { -43253, 54394, 0 }, { -43253, 54394, 0 } },
        { { 78577, -48496, 49807 }, { 78577, -48496, 49807 }, { 78577, -48496, -49807 }, { 78577, -48496, -49807 } },
        { { 121831, 0, 40632 }, { 121831, 0, 40632 }, { 121831, 0, -40632 }, { 121831, 0, -40632 } },
        { { -124125, -26738, -1769 }, { -124125, -26738, -1769 }, { -124125, -26738, -1769 }, { -124125, -26738, -1769 } },
        { { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 } },
        { { 75300, -38010, 0 }, { 75300, -38010, 0 }, { -81199, -38010, 0 }, { -81199, -38010, 0 } }
    } },
    { {
        { { 73334, -41287, 42598 }, { 73334, -41287, -42598 }, { -79888, -41287, 42598 }, { -79888, -41287, -42598 } },
        { { 22937, -19660, 0 }, { 22937, -19660, 0 }, { 22937, -19660, 0 }, { 22937, -19660, 0 } },
        { { 67436, 5898, 0 }, { 67436, 5898, 0 }, { 67436, 5898, 0 }, { 67436, 5898, 0 } },
        { { 73334, -1966, 42598 }, { 73334, -1966, -42598 }, { -79888, -1966, 42598 }, { -79888, -1966, -42598 } },
        { { 75300, 38010, 0 }, { 75300, 38010, 0 }, { -81199, 38010, 0 }, { -81199, 38010, 0 } },
        { { 73334, -1966, 42598 }, { 73334, -1966, -42598 }, { -79888, -1966, 42598 }, { -79888, -1966, -42598 } },
        { { 106102, 11796, 0 }, { 106102, 11796, 0 }, { -43253, 54394, 0 }, { -43253, 54394, 0 } },
        { { 73334, -41287, 42598 }, { 73334, -41287, 42598 }, { 73334, -41287, -42598 }, { 73334, -41287, -42598 } },
        { { 121831, 0, 40632 }, { 121831, 0, 40632 }, { 121831, 0, -40632 }, { 121831, 0, -40632 } },
        { { -127336, -34406, -23986 }, { -127336, -34406, -23986 }, { -127336, -34406, -23986 }, { -127336, -34406, -23986 } },
        { { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 }, { 0, -38010, 0 } },
        { { 75300, -38010, 0 }, { 75300, -38010, 0 }, { -81199, -38010, 0 }, { -81199, -38010, 0 } }
    } },
    { {
        { { 93650, -48496, 44564 }, { 93650, -48496, -44564 }, { -75300, -48496, 44564 }, { -75300, -48496, -44564 } },
        { { 33423, -7864, 0 }, { 33423, -7864, 0 }, { 33423, -7864, 0 }, { 33423, -7864, 0 } },
        { { 67436, 5898, 0 }, { 67436, 5898, 0 }, { 67436, 5898, 0 }, { 67436, 5898, 0 } },
        { { 93650, -4587, 44564 }, { 93650, -4587, -44564 }, { -75300, -4587, 44564 }, { -75300, -4587, -44564 } },
        { { 75300, 38010, 0 }, { 75300, 38010, 0 }, { -81199, 38010, 0 }, { -81199, 38010, 0 } },
        { { 93650, -4587, 44564 }, { 93650, -4587, -44564 }, { -75300, -4587, 44564 }, { -75300, -4587, -44564 } },
        { { 102825, 16384, 0 }, { 102825, 16384, 0 }, { -22937, 51118, 0 }, { -22937, 51118, 0 } },
        { { 93650, -48496, 44564 }, { 93650, -48496, 44564 }, { 93650, -48496, -44564 }, { 93650, -48496, -44564 } },
        { { 137625, -65, 35389 }, { 137625, -65, 35389 }, { 137625, -65, -35389 }, { 137625, -65, -35389 } },
        { { -126418, -25559, 25559 }, { -126418, -25559, 25559 }, { -126418, -25559, 25559 }, { -126418, -25559, 25559 } },
        { { 2621, -29491, 0 }, { 2621, -29491, 0 }, { 2621, -29491, 0 }, { 2621, -29491, 0 } },
        { { 91029, -29491, 0 }, { 91029, -29491, 0 }, { -69402, -29491, 0 }, { -69402, -29491 } }
    } }
};

// Projected 2D points of the option menu car preview: 4 points per part.
// GLOBAL: CMR2 0x008313c8
PreviewProjectedQuad g_previewProjectedPartQuads[12];

void OptionPreview_RotateSlotVector(int param1, FixVector *param2, FixVector *param3, FixAngles *param4);
void Game_ProjectMenuBackgroundPoint(int *param1, FixVector *param2);

// Projects the reference hull points of one car model: every part's
// 4 vertices are rotated by the part angles and the projected points are
// stored in the shared preview buffer.
// FUNCTION: CMR2 0x0050f120
void OptionPreview_ProjectReferencePartQuads(int param1)
{
    FixVector *pVec;
    PreviewScreenPoint *pOut;
    FixAngles *pAngles;
    FixVector local;
    int k;

    pOut = g_previewProjectedPartQuads[0].points;
    pAngles = g_previewPartRotationPresets;
    pVec = g_previewReferenceModels[param1].partVertices[0];
    do {
        for (k = 0; k < 4; k++) {
            OptionPreview_RotateSlotVector(0, pVec, &local, pAngles);
            Game_ProjectMenuBackgroundPoint(&pOut->x, &local);
            pVec++;
            pOut++;
        }
        pAngles++;
    } while ((INT_PTR)pAngles < (INT_PTR)(g_previewPartRotationPresets + 12)); // 0x527420 in the original
}

// FUNCTION: CMR2 0x0050f1c0
void OptionMenu_ResetActivePage(void)
{
    g_pMenu0x00831778 = g_pMenu0x0083177c;
}

// Per-frame update of the option menu: refreshes the active menu, polls the
// input device and forwards its state to Menu_Update.
// FUNCTION: CMR2 0x0050f1d0
void OptionMenu_UpdateActivePage(void)
{
    int input;
    DeviceInfo *pDevice;
    Menu *pNextMenu;

    OptionMenu_ResetActivePage();
    Sound_UpdateMusicStreaming();
    CInput::UpdateAllAvailableDevices();
    input = 0;
    if (CGameInfo::GetActiveOptionSlot() == 1) {
        if (CGameInfo::IsConfiguredMultiplayer() == 0)
            input = 1;
    }
    Input_TranslatePedalsToMenuKeys();
    pDevice = CInput::GetAvailableDeviceRecord(input);
    Input_MergeAssignedJoystickButtons(input, pDevice);
    pNextMenu = (Menu *)Menu_Update(g_pMenu0x00831778, pDevice->pressed);
    if (pNextMenu != NULL)
        g_pMenu0x0083177c = pNextMenu;
}

// FUNCTION: CMR2 0x0050f230
void OptionMenu_DrawActivePage(void)
{
    Menu_CallCallback2(g_pMenu0x00831778);
}

// Option menu text file prefixes, indexed by game language ("s" + language).
// GLOBAL: CMR2 0x0052962c
char g_str0x0052962c[8] = "spolish";
// GLOBAL: CMR2 0x00529634
char g_str0x00529634[8] = "sengusa";
// GLOBAL: CMR2 0x0052963c
char g_str0x0052963c[8] = "sgerman";
// GLOBAL: CMR2 0x00529644
char g_str0x00529644[12] = "sitalian";
// GLOBAL: CMR2 0x00529650
char g_str0x00529650[12] = "sspanish";
// GLOBAL: CMR2 0x0052965c
char g_str0x0052965c[8] = "sfrench";
// GLOBAL: CMR2 0x00529664
char g_str0x00529664[12] = "senglish";

extern char g_strTxtFormat[];
extern BYTE *g_optionLanguageText;
extern BYTE g_optionLanguageTextInArchive;
GenericFile *OptionMenu_GetCommonArchive(void);
GenericFile *OptionMenu_GetLanguageArchive(void);
GenericFile *OptionMenu_GetStageArchive(void);
bool OptionMenu_ReleaseSoundBuffer(void);
int OptionMenu_ReleaseTextures(void);

// Loads the option menu text file of the current region and language and
// registers it with the frontend.
// FUNCTION: CMR2 0x0050f240
BYTE OptionMenu_LoadRegionalText(void)
{
    char *europe[5];
    char *usa[3];
    char *japan[1];
    char *poland[1];
    char **names;

    europe[0] = g_str0x00529664;
    europe[1] = g_str0x0052965c;
    europe[2] = g_str0x00529650;
    europe[3] = g_str0x00529644;
    europe[4] = g_str0x0052963c;
    usa[0] = g_str0x00529634;
    usa[1] = g_str0x0052965c;
    usa[2] = g_str0x00529650;
    japan[0] = g_str0x00529664;
    poland[0] = g_str0x0052962c;

    names = NULL;
    switch (CGameInfo::GetGameRegion()) {
    case 0:
        names = europe;
        break;
    case 1:
        names = usa;
        break;
    case 2:
        names = japan;
        break;
    case 3:
        names = poland;
    }

    sprintf(CFrontend::m_stringDest, g_strTxtFormat, names[CGameInfo::GetGameLanguage()]);
    g_optionLanguageText = (BYTE *)CGenericFileLoader::FindFile(OptionMenu_GetLanguageArchive(),
                                                       CFrontend::m_stringDest, &g_optionLanguageTextInArchive, NULL, 0);
    // the original tests the address of the buffer, so this is always true
    if (&g_optionLanguageText != NULL) {
        CFrontend::BuildLocalizedTextStringTable(1, 0x146, &g_optionLanguageText);
        CGame::RegisterCallback(OptionMenu_ReleaseSoundBuffer, NULL);
        return 1;
    }
    return 0;
}

// FUNCTION: CMR2 0x0050f620
GenericFile *OptionMenu_GetCommonArchive(void)
{
    return (GenericFile *)g_optionCommonArchive;
}

// FUNCTION: CMR2 0x0050f630
GenericFile *OptionMenu_GetLanguageArchive(void)
{
    return (GenericFile *)g_optionLanguageArchive;
}

// FUNCTION: CMR2 0x0050f640
GenericFile *OptionMenu_GetStageArchive(void)
{
    return (GenericFile *)g_optionStageArchive;
}

unsigned char GameInfo_GetFrontendSessionFlag(void);
void OptionMenu_ArmRecordCountdowns(void);
void OptionMenu_UpdateRecordTransitions(void);
void OptionMenu_UpdateActivePage(void);
void OptionPreview_AnimateModelRotations(void);
void GameInfo_ProcessNetworkMessages(void);
int NetPlayers_GetInputFrameDelta(void);
void NetPlayers_SendType16Notification(void);
BOOL Network_GetSessionStateFlag(void);
void StageUI_PublishNetworkRaceWins(void);
extern int g_unk0x0082b0a4;

// Set when the option menu has finished fading; drawn by the boot state 0x500c80.

// Frame callback of the boot state machine: while the fade has not timed out
// (or the menu is not the options one) it keeps the option menu alive,
// otherwise it lets the state machine advance.
// FUNCTION: CMR2 0x00500f80
void OptionMenu_UpdateFadeState(CallbackStateMachine *p1, BYTE state)
{
    if (GameInfo_GetFrontendSessionFlag() == 0) {
        if ((unsigned int)(CMain::GetFrameDelta() - g_unk0x0082b0a0) > 0x17d4
            && CGameInfo::GetGameModeOptionBit19() != 0 && CGameInfo::GetConfiguredGameMode() != 0xa)
            goto other;
        if (CMain::GetFrameDelta() - NetPlayers_GetInputFrameDelta() > GameInfo_GetSessionField397C() * 6000
            && CGameInfo::GetGameModeOptionBit19() != 0 && CGameInfo::GetConfiguredGameMode() == 0xa
            && Network_GetSessionStateFlag() != 0 && GameInfo_GetSessionField397C() != 0)
            goto other;
        OptionMenu_ArmRecordCountdowns();
        OptionMenu_UpdateRecordTransitions();
        OptionMenu_UpdateActivePage();
        OptionPreview_AnimateModelRotations();
        goto tail;
    }
other:
    if (CMain::GetFrameDelta() - NetPlayers_GetInputFrameDelta() > GameInfo_GetSessionField397C() * 6000
        && CGameInfo::GetConfiguredGameMode() == 0xa && Network_GetSessionStateFlag() != 0 && GameInfo_GetSessionField397C() != 0) {
        g_unk0x0082b0ac = 1;
        StageUI_PublishNetworkRaceWins();
        NetPlayers_SendType16Notification();
    }
    CGame::PromoteCallbackEntryByRule(p1, state, 0, 2);
    g_unk0x0082b0a4 = CMain::GetFrameDelta();
tail:
    if (CGameInfo::GetGameModeOptionBit19() != 0)
        GameInfo_ProcessNetworkMessages();
}

// FUNCTION: CMR2 0x005011a0
void CGameInfo::SaveOptionMenuCallbackCount(void)
{
    g_unk0x0082af88 = CGame::GetCallbackCount();
}

// FUNCTION: CMR2 0x004f8a70
char *CGameInfo::GetCodeEntryText(int index)
{
    return CFrontend::GetTextString(index + 0x1f1);
}

// FUNCTION: CMR2 0x005011b0
int CGameInfo::GetActiveOptionSlot(void)
{
    return GetConfiguredPlayerCount() - g_optionPlayerSlotOffset;
}

// FUNCTION: CMR2 0x00500500
void CGameInfo::StartOptionMenuTimeout(void)
{
    g_unk0x0082ac58 = 1;
    g_unk0x0082ac5c = CMain::GetFrameDelta();
}

// FUNCTION: CMR2 0x005012c0
int CGameInfo::GetOptionMenuRemainingTime(void)
{
    int result;

    result = g_unk0x0082b0a0 - CMain::GetFrameDelta() + 0x17d5;
    if (result < 0)
        result = 0;
    return result;
}

// FUNCTION: CMR2 0x0040a420
int CGameInfo::GetNetworkStageBestTime(int index)
{
    if (GetConfiguredGameMode() == 0xc)
        return g_netStageBest[0];
    return g_netStageBest[index - 1];
}

// FUNCTION: CMR2 0x00501cc0
void CGameInfo::StartOptionRecordTransition(int index, int param2, int param3)
{
    Unk0x0082b2c0 *pEntry;

    pEntry = &g_optionRecordTransitions[index];
    pEntry->field_0x8 = CMain::GetFrameDelta();
    pEntry->field_0x4 = param2;
    pEntry->field_0x0 = 0;
    pEntry->field_0xc = 1;
    pEntry->field_0x10 = param3;
}

// match 75%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x005004c0
int CGameInfo::IsOptionMenuTimeoutPulseOn(void)
{
    unsigned int delta;

    if (g_unk0x0082ac58 != 0) {
        delta = CMain::GetFrameDelta() - g_unk0x0082ac5c;
        return (unsigned char)~(delta / 10) & 1;
    }
    return 0;
}

// FUNCTION: CMR2 0x00501230
int CGameInfo::GetPreviewLayoutMode(void)
{
    if (RallyDataStageIndex() == 0xa ||
        GetConfiguredGameMode() == 2 ||
        GetConfiguredGameMode() == 3 ||
        GetConfiguredGameMode() == 8 ||
        GetConfiguredGameMode() == 9 ||
        GetConfiguredGameMode() == 0xa)
        return 1;
    return 2;
}

// 0x50-byte entry of the table at 0x82c6c8 (FixInterp in GameInfo.h).

// GLOBAL: CMR2 0x0082c6c8
FixInterp g_optionPreviewPanels[16];
// GLOBAL: CMR2 0x0082ca1c
BYTE g_selectedOptionPreviewPanel;
// GLOBAL: CMR2 0x0082c6c0
int g_unk0x0082c6c0;
// GLOBAL: CMR2 0x0082cb44
int g_unk0x0082cb44;

// Selects a new entry only when the current entry has no pending activity.
// match 75%: the original pushes ESI in the prologue and only then reloads the
// parameter (at [esp+8]); this compiler sinks the push of the callee-saved
// register into the taken branch and computes the return value one store early
// (identical byte count, different scheduling).
// FUNCTION: CMR2 0x00505e10
int CGameInfo::SelectIdleOptionPreviewEntry(BYTE param1)
{
    int result = 0;
    FixInterp *pEntry;
    if (g_selectedOptionPreviewPanel == 0xff ||
        ((pEntry = &g_optionPreviewPanels[(signed char)g_selectedOptionPreviewPanel])->active == 0 &&
         pEntry->current == 0)) {
        result = 1;
        g_selectedOptionPreviewPanel = param1;
        g_unk0x0082c6c0 = CMain::GetFrameDelta();
        g_unk0x0082cb44 = 0;
    }
    return result;
}

// Adds the mesh of a scene node to a preview mesh record: stores
// the node, its mesh and its vertex count in the slot of the node's mesh id,
// expands the +/-100 bounding box with every vertex and stores the centre and
// the half size of the slot. Bumps the counter of used slots.
// FUNCTION: CMR2 0x00507290
void OptionPreview_AddNodeMeshToRecord(SceneNode *pNode, OptionPreviewMeshRecord *pRecord)
{
    DeformFloatVertex *pVertex;
    FixVector halfSize;
    Mesh *pMesh;
    WORD count;
    int slot;
    int maxX, maxY, maxZ;
    int minX, minY, minZ;
    int x, y, z;
    int i;

    slot = (pNode->flags & 0xff) - 5;
    pMesh = (Mesh *)pNode->pObject;
    if (pMesh != NULL && pNode->type == 0) {
        pRecord->geometry.nodes[slot] = pNode;
        pRecord->geometry.meshes[slot] = pMesh;
        count = (WORD)Mesh_GetField0x10(pMesh);
        pRecord->vertexCount[slot] = count;
        if (pRecord->geometry.meshes[slot] != NULL && count > 0) {
            maxZ = -0x640000;
            maxY = -0x640000;
            maxX = -0x640000;
            minZ = 0x640000;
            minY = 0x640000;
            minX = 0x640000;
            for (i = 0; i < (int)pRecord->vertexCount[slot]; i++) {
                x = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)pRecord->geometry.meshes[slot]->pVertexData)[i].pos[0], CGraphics::m_65536));
                pVertex = ((DeformFloatVertex *)pRecord->geometry.meshes[slot]->pVertexData) + i;
                y = (int)FTOI(__int64, X87S_MUL(pVertex->pos[1], CGraphics::m_65536));
                z = (int)FTOI(__int64, X87S_MUL(pVertex->pos[2], CGraphics::m_65536));
                if (x > maxX)
                    maxX = x;
                if (x < minX)
                    minX = x;
                if (y > maxY)
                    maxY = y;
                if (y < minY)
                    minY = y;
                if (z > maxZ)
                    maxZ = z;
                if (z < minZ)
                    minZ = z;
                if (x >= 0) {
                    if (x > pRecord->maxX)
                        pRecord->maxX = x;
                } else {
                    if (x < pRecord->minX)
                        pRecord->minX = x;
                }
                if (z >= 0) {
                    if (z > pRecord->maxZ)
                        pRecord->maxZ = z;
                } else {
                    if (z < pRecord->minZ)
                        pRecord->minZ = z;
                }
            }
            halfSize.x = minX - maxX;
            halfSize.y = minY - maxY;
            halfSize.z = minZ - maxZ;
            FixVecScale(&halfSize, &halfSize, 0x8000);
            pRecord->centre[slot].x = maxX + halfSize.x;
            pRecord->centre[slot].y = maxY + halfSize.y;
            pRecord->centre[slot].z = maxZ + halfSize.z;
            pRecord->halfSize[slot].x = maxX - pRecord->centre[slot].x;
            pRecord->halfSize[slot].y = maxY - pRecord->centre[slot].y;
            pRecord->halfSize[slot].z = maxZ - pRecord->centre[slot].z;
            pRecord->meshCount++;
        }
    }
}

void OptionPreview_SetMeshRenderFlags(int index, int param2, BYTE param3);

// Restores the converted source vertex data of every preview mesh back into
// its 0x30-byte mesh vertices, rebuilds the meshes, marks the option state of
// each slot and resets the 8 option meshes of the record.
// FUNCTION: CMR2 0x005074d0
void OptionPreview_RestoreOriginalMeshVertices(int index)
{
    OptionPreviewMeshRecord *pRecord;
    FixVector position;
    FixVector normal;
    int i;
    int j;

    pRecord = &g_previewMeshRecords[index];
    if (pRecord->verticesAssigned == 0)
        return;

    for (i = 0; i < (int)pRecord->meshCount; i++) {
        for (j = 0; j < (int)pRecord->vertexCount[i]; j++) {
            position = pRecord->geometry.vertices[i][j].pos;
            ((DeformFloatVertex *)pRecord->geometry.meshes[i]->pVertexData)[j].pos[0] =
                X87S_MUL(X87S_FILD(float, position.x), CGraphics::m_oneOver65536);
            ((DeformFloatVertex *)pRecord->geometry.meshes[i]->pVertexData)[j].pos[1] =
                X87S_MUL(X87S_FILD(float, position.y), CGraphics::m_oneOver65536);
            ((DeformFloatVertex *)pRecord->geometry.meshes[i]->pVertexData)[j].pos[2] =
                X87S_MUL(X87S_FILD(float, position.z), CGraphics::m_oneOver65536);
            normal = pRecord->geometry.vertices[i][j].normal;
            ((DeformFloatVertex *)pRecord->geometry.meshes[i]->pVertexData)[j].normal[0] =
                X87S_MUL(X87S_FILD(float, normal.x), CGraphics::m_oneOver65536);
            ((DeformFloatVertex *)pRecord->geometry.meshes[i]->pVertexData)[j].normal[1] =
                X87S_MUL(X87S_FILD(float, normal.y), CGraphics::m_oneOver65536);
            ((DeformFloatVertex *)pRecord->geometry.meshes[i]->pVertexData)[j].normal[2] =
                X87S_MUL(X87S_FILD(float, normal.z), CGraphics::m_oneOver65536);
        }
        if (pRecord->geometry.nodes[i]->pObject != NULL)
            Mesh_Rebuild((Mesh *)pRecord->geometry.nodes[i]->pObject);
        pRecord->meshVisible[i] = 1;
    }
    for (i = 0; i < 8; i++)
        OptionPreview_SetMeshRenderFlags(index, 0, i);
}

void OptionPreview_DecodeImpactPayload(CarImpactPayload *pColour);
void OptionPreview_ApplyDecodedImpact(int index);
void OptionPreview_SelectDamageMaterials(int index);
void OptionPreview_UpdateCornerHeightOffsets(int index);
void OptionPreview_UnpackPartDamageFractions(int index);

// Mesh slots hidden by the four broken-part flags of the preview model.
// GLOBAL: CMR2 0x00527364
BYTE g_previewBreakMeshSlots[4] = { 4, 5, 6, 7 };

// Applies stored damage to the preview model: restores original vertices,
// replays the linked list of 13-byte impacts, selects damage materials and
// hides broken parts before deriving corner heights and part damage fractions.
// FUNCTION: CMR2 0x00507650
void OptionPreview_ApplyStoredDamage(int index)
{
    OptionPreviewMeshRecord *pRecord;
    RallyCarDamageState *pRally;
    CarDamageLink *pEntry;
    char next;
    int i;

    pRecord = &g_previewMeshRecords[index];
    OptionPreview_RestoreOriginalMeshVertices(index);
    pRally = RallyData_GetDriverGroupRecord(index);
    pEntry = &pRally->impacts.links[pRally->impacts.firstLink];
    if (pRally->impacts.count != 0) {
        while (pEntry != NULL) {
            OptionPreview_DecodeImpactPayload(&pEntry->impact);
            OptionPreview_ApplyDecodedImpact(index);
            next = (char)pEntry->next;
            if (next == -1)
                break;
            pEntry = &pRally->impacts.links[next];
        }
    }
    OptionPreview_SelectDamageMaterials(index);
    for (i = 0; i < 4; i++) {
        if (pRally->damage.partHidden[i] != 0)
            pRecord->meshVisible[g_previewBreakMeshSlots[i]] = 0;
    }
    OptionPreview_UpdateCornerHeightOffsets(index);
    OptionPreview_UnpackPartDamageFractions(index);
}

// GLOBAL: CMR2 0x0082d120
FixVector g_previewDentPosition;
// GLOBAL: CMR2 0x0082d12c
FixVector g_previewDentNormal;
// GLOBAL: CMR2 0x0082d138
FixVector g_previewDentAxis;
// GLOBAL: CMR2 0x0082d144
int g_previewDentAxisRadius;
// GLOBAL: CMR2 0x0082d148
int g_previewDentStrength;
// GLOBAL: CMR2 0x0082d14c
BYTE g_previewDentMode;

// Decodes a packed impact into the preview dent parameters: position scaled
// by 10/127, normal/axis by 1/127, strength by 1/255 and radius for mode 1.
// FUNCTION: CMR2 0x00507710
void OptionPreview_DecodeImpactPayload(CarImpactPayload *pColour)
{
    g_previewDentPosition.x = (int)(signed char)pColour->position[0] << 16;
    g_previewDentPosition.y = (int)(signed char)pColour->position[1] << 16;
    g_previewDentPosition.z = (int)(signed char)pColour->position[2] << 16;
    FixVecScale(&g_previewDentPosition, &g_previewDentPosition, FixMul(0xa0000, FixDiv(0x10000, 0x7f0000)));
    g_previewDentNormal.x = (int)(signed char)pColour->normal[0] << 16;
    g_previewDentNormal.y = (int)(signed char)pColour->normal[1] << 16;
    g_previewDentNormal.z = (int)(signed char)pColour->normal[2] << 16;
    // The original expands the 1/127 scale as a 64-bit division (its compiler
    // keeps the constant divisor in a register, like at 0x4689c8).
    FixVecScaleRecip(&g_previewDentNormal, &g_previewDentNormal, 0x7f0000);
    g_previewDentAxis.x = (int)(signed char)pColour->axis[0] << 16;
    g_previewDentAxis.y = (int)(signed char)pColour->axis[1] << 16;
    g_previewDentAxis.z = (int)(signed char)pColour->axis[2] << 16;
    FixVecScaleRecip(&g_previewDentAxis, &g_previewDentAxis, 0x7f0000);
    g_previewDentStrength = (int)pColour->strength << 16;
    g_previewDentStrength = FixDiv(g_previewDentStrength, 0xff0000);
    g_previewDentMode = pColour->mode;
    if (pColour->mode == 1) {
        g_previewDentAxisRadius = (int)pColour->axisRadius << 16;
        g_previewDentAxisRadius = FixMul(g_previewDentAxisRadius, FixMul(0xa0000, FixDiv(0x10000, 0xff0000)));
    }
}

// Number of credit entries (pairs of quoted strings) in the credits file
// GLOBAL: CMR2 0x0081a728
int g_creditsLineCount;
// GLOBAL: CMR2 0x0081a72c
void *g_pCreditsRoleTable;
// GLOBAL: CMR2 0x0081a730
void *g_creditsFileBuffer;
// GLOBAL: CMR2 0x0081a734
void *g_creditsLines;

// Credits file names per region (index = CGameInfo::GetGameLanguage())
// GLOBAL: CMR2 0x00525aa4
char g_strCreditsPathFormat[12] = "%s%s%s.txt";
// GLOBAL: CMR2 0x00525ab0
char g_strCreditsPolish[16] = "credits_polish";
// GLOBAL: CMR2 0x00525ac0
char g_strCreditsJapanese[20] = "credits_japanese";
// GLOBAL: CMR2 0x00525ad4
char g_strCreditsSpanishUsa[20] = "credits_spanishusa";
// GLOBAL: CMR2 0x00525ae8
char g_strCreditsFrenchUsa[20] = "credits_frenchusa";
// GLOBAL: CMR2 0x00525afc
char g_strCreditsUsa[12] = "credits_usa";
// GLOBAL: CMR2 0x00525b08
char g_strCreditsGerman[16] = "credits_german";
// GLOBAL: CMR2 0x00525b18
char g_strCreditsItalian[16] = "credits_italian";
// GLOBAL: CMR2 0x00525b28
char g_strCreditsSpanish[16] = "credits_spanish";
// GLOBAL: CMR2 0x00525b38
char g_strCreditsFrench[16] = "credits_french";
// GLOBAL: CMR2 0x00525b48
char g_strCreditsEnglish[16] = "credits_english";

bool FrontendCredits_ReleaseText(void);

// Loads the credits text of the current language and splits it into
// entries of two quoted strings ("role", "name"); each quote-delimited
// string is terminated in place. Optionally registers the release callback.
// FUNCTION: CMR2 0x004f4910
void FrontendCredits_LoadText(char registerRelease)
{
    char *europe[5];
    char *usa[3];
    char *japan[1];
    char *poland[1];
    char **names;
    char line[256];
    char *p;
    int size;
    int i, n;

    europe[0] = g_strCreditsEnglish;
    europe[1] = g_strCreditsFrench;
    europe[2] = g_strCreditsSpanish;
    europe[3] = g_strCreditsItalian;
    europe[4] = g_strCreditsGerman;
    usa[0] = g_strCreditsUsa;
    usa[1] = g_strCreditsFrenchUsa;
    usa[2] = g_strCreditsSpanishUsa;
    japan[0] = g_strCreditsJapanese;
    poland[0] = g_strCreditsPolish;

    names = NULL;
    switch (CGameInfo::GetGameRegion()) {
    case 0:
        names = europe;
        break;
    case 1:
        names = usa;
        break;
    case 2:
        names = japan;
        break;
    case 3:
        names = poland;
        break;
    }

    sprintf(CFrontend::m_stringDest, g_strCreditsPathFormat, CInstallInfo::GetCountrySpecificDir(),
            CGameInfo::GetGameRegionDirectory(), names[CGameInfo::GetGameLanguage()]);
    g_creditsFileBuffer = CFileBuffer::GetGenericFileBuffer(CFrontend::m_stringDest, TRUE);

    size = CGenericFileLoader::GetGenericFileSize();
    for (i = 0; i < size; i++) {
        if (((char *)g_creditsFileBuffer)[i] == '"')
            g_creditsLineCount++;
    }
    g_creditsLineCount = g_creditsLineCount / 4;

    // Two strings per credits line, like p.
    g_creditsLines = CFileBuffer::AllocateLockedBuffer(g_creditsLineCount * 2 * sizeof(p));
    g_pCreditsRoleTable = CFileBuffer::AllocateLockedBuffer(g_creditsLineCount * 4);
    if (g_creditsLines != NULL) {
        p = (char *)g_creditsFileBuffer;
        for (i = 0; i < g_creditsLineCount; i++) {
            p = strchr(p, '"') + 1;
            ((char **)g_creditsLines)[i * 2] = p;
            for (n = 0; *p != '"'; n++, p++)
                line[n] = *p;
            *p = '\0';
            line[n] = '\0';

            p = strchr(p + 1, '"') + 1;
            ((char **)g_creditsLines)[i * 2 + 1] = p;
            for (n = 0; *p != '"'; n++, p++)
                line[n] = *p;
            *p = '\0';
            p++;
        }
    } else {
        g_creditsLineCount = 0;
    }

    if (registerRelease)
        CGame::RegisterCallback(FrontendCredits_ReleaseText, NULL);
}

// FUNCTION: CMR2 0x004f4b10
void *FrontendCredits_GetNameTable(void)
{
    return g_creditsLines;
}

// FUNCTION: CMR2 0x004f4b20
int FrontendCredits_GetEntryCount(void)
{
    return g_creditsLineCount;
}

// FUNCTION: CMR2 0x004f4b30
void *FrontendCredits_GetRoleTable(void)
{
    return g_pCreditsRoleTable;
}

// Frontend text files per region (index = language, see CGameInfo::InitRegionLanguageCount)
// GLOBAL: CMR2 0x00525b58
char g_strTextPolish[8] = "fpolish";
// GLOBAL: CMR2 0x00525b60
char g_strTextEngUsa[8] = "fengusa";
// GLOBAL: CMR2 0x00525b68
char g_strTextGerman[8] = "fgerman";
// GLOBAL: CMR2 0x00525b70
char g_strTextItalian[12] = "fitalian";
// GLOBAL: CMR2 0x00525b7c
char g_strTextSpanish[12] = "fspanish";
// GLOBAL: CMR2 0x00525b88
char g_strTextFrench[8] = "ffrench";
// GLOBAL: CMR2 0x00525b90
char g_strTextEnglish[12] = "fenglish";

// Frontend fonts (Fonts\<name>.tga + .pcf); FrontendText_ReloadFonts walks them as one array
// GLOBAL: CMR2 0x00525b9c
char g_frontendFontNames[4][20] = { "general\\hel_12pt", "general\\hel_15pt", "general\\hel_36pt", "general\\dot" };
#define g_strFontHel12 g_frontendFontNames[0]
#define g_strFontHel15 g_frontendFontNames[1]
#define g_strFontHel36 g_frontendFontNames[2]
#define g_strFontDot g_frontendFontNames[3]

// Text buffers of the frontend languages and whether each one lives inside
// its archive (then it is not freed on release)
// GLOBAL: CMR2 0x0081a738
void *g_languageTexts[5];
// GLOBAL: CMR2 0x0081a74c
BYTE g_languageTextInArchive[5];

extern char g_strTxtFormat[];

// Release callback of FrontendText_LoadRegionLanguages.
// FUNCTION: CMR2 0x004f4cc0
BYTE FrontendText_ReleaseRegionLanguages(void)
{
    BYTE i;

    for (i = 0; i < CGameInfo::m_unk0x0081a754; i++) {
        if (g_languageTextInArchive[i] == 0) {
            CFileBuffer::FreeGenericFileBuffer(g_languageTexts[i]);
            g_languageTexts[i] = NULL;
        }
        g_languageTexts[i] = NULL;
    }
    CFrontend::FreeLocalizedTextStrings();
    return 1;
}

// Loads the text file of every frontend language of this region from its
// language archive and hands them to the frontend text tables.
// FUNCTION: CMR2 0x004f4b90
BYTE FrontendText_LoadRegionLanguages(void)
{
    char *japan[1];
    char *poland[1];
    char **names;
    char *usa[3];
    char *europe[5];
    BYTE i;

    europe[0] = g_strTextEnglish;
    europe[1] = g_strTextFrench;
    europe[2] = g_strTextSpanish;
    europe[3] = g_strTextItalian;
    europe[4] = g_strTextGerman;
    usa[0] = g_strTextEngUsa;
    usa[1] = g_strTextFrench;
    usa[2] = g_strTextSpanish;
    japan[0] = g_strTextEnglish;
    poland[0] = g_strTextPolish;

    switch (CGameInfo::GetGameRegion()) {
    case 0:
        names = europe;
        break;
    case 1:
        names = usa;
        break;
    case 2:
        names = japan;
        break;
    case 3:
        names = poland;
        break;
    }

    for (i = 0; i < CGameInfo::m_unk0x0081a754; i++) {
        g_languageTexts[i] = NULL;
        sprintf(CFrontend::m_stringDest, g_strTxtFormat, names[i]);
        g_languageTexts[i] = CGenericFileLoader::FindFile(CFrontend::GetLocalizedFrontendArchive(i), CFrontend::m_stringDest,
                                                        &g_languageTextInArchive[i], NULL, 0);
    }
    if (g_languageTexts[0] != NULL) {
        CFrontend::BuildLocalizedTextStringTable(CGameInfo::m_unk0x0081a754, 0x215, (BYTE **)g_languageTexts);
        CGame::RegisterCallback(FrontendText_ReleaseRegionLanguages, NULL);
        return 1;
    }
    return 0;
}

// Loads the four frontend fonts from the frontend archive.
// FUNCTION: CMR2 0x004f4d20
BYTE FrontendText_LoadFonts(void)
{
    if (Font_InitTable(4) != -1) {
        Font_Load(g_strFontHel12, CGenericFileLoader::GetGenericFile(), 0);
        Font_Load(g_strFontHel15, CGenericFileLoader::GetGenericFile(), 1);
        Font_Load(g_strFontHel36, CGenericFileLoader::GetGenericFile(), 2);
        Font_Load(g_strFontDot, CGenericFileLoader::GetGenericFile(), 3);
        return 1;
    }
    return 0;
}

// Reloads the four frontend fonts (after the display was re-created).
// FUNCTION: CMR2 0x004f4d80
void FrontendText_ReloadFonts(void)
{
    char *pName;
    int i;

    i = 0;
    pName = g_frontendFontNames[0];
    do {
        Font_Reload(pName, CGenericFileLoader::GetGenericFile(), i);
        pName += 20;
        i++;
    } while ((INT_PTR)pName < (INT_PTR)g_frontendFontNames[4]);
}

// FUNCTION: CMR2 0x004f48b0
bool FrontendCredits_ReleaseText(void)
{
    if (g_creditsFileBuffer != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_creditsFileBuffer);
        g_creditsFileBuffer = NULL;
    }
    if (g_creditsLines != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_creditsLines);
        g_creditsLines = NULL;
    }
    if (g_pCreditsRoleTable != NULL)
        CFileBuffer::FreeGenericFileBuffer(g_pCreditsRoleTable);
    g_creditsFileBuffer = NULL;
    g_creditsLines = NULL;
    g_pCreditsRoleTable = NULL;
    g_creditsLineCount = 0;
    return true;
}

// GLOBAL: CMR2 0x00819128
int g_firstVisibleDisplayDevice;

// FUNCTION: CMR2 0x004f2360
void FrontendMenu_EnterDisplayMode(Menu *pMenu, int param2)
{
    g_firstVisibleDisplayDevice = CGraphics::GetSelectedDisplayDeviceIx();
    if (g_firstVisibleDisplayDevice + 10 > (int)CGraphics::GetDisplayCount()) {
        g_firstVisibleDisplayDevice = (int)CGraphics::GetDisplayCount() - 10;
        if (g_firstVisibleDisplayDevice < 0)
            g_firstVisibleDisplayDevice = 0;
    }
    pMenu->items[0].min = (BYTE)CGraphics::GetDisplayCount();
    pMenu->items[0].max = (BYTE)CGraphics::GetSelectedDisplayDeviceIx();
    CGame::SetGameInputFocusState(0);
}

// FUNCTION: CMR2 0x004f2b00
void FrontendMenu_UpdateSoundOptions(Menu *pMenu)
{
    CGameInfo::SetMasterSoundVolume(pMenu->items[0].max * 10);
    CGameInfo::SetEffectsSoundVolume(pMenu->items[1].max * 10);
    CGameInfo::SetCoDriverSoundVolume(pMenu->items[2].max * 10);
    CInput::SetMenuSoundVolume((int)(CGameInfo::GetEffectsSoundVolume() << 16) / 100 / 4);
    CSound::NoOpSoundDeviceCallback();
}

// GLOBAL: CMR2 0x0081b14c
SavedGameImage *g_pSavedGames;
// GLOBAL: CMR2 0x0081b150
char **g_savedGameFileNames;
// GLOBAL: CMR2 0x0081b154
int g_savedGameCount;

// FUNCTION: CMR2 0x004f4db0
int SavedGames_GetCount(void)
{
    return g_savedGameCount;
}

// Saved games list: records of 0x7f4 bytes
// FUNCTION: CMR2 0x004f4dc0
BYTE *SavedGames_GetRecordData(int index)
{
    return (BYTE *)g_pSavedGames[index].name;
}

// FUNCTION: CMR2 0x004f4de0
unsigned int SavedGames_GetDifficulty(int index)
{
    return g_pSavedGames[index].options.playerCount;
}

// FUNCTION: CMR2 0x004f4e00
unsigned int SavedGames_GetDateLowBits(int index)
{
    return g_pSavedGames[index].stage.country;
}

// FUNCTION: CMR2 0x004f4e20
unsigned int SavedGames_GetDateMiddleBits(int index)
{
    return g_pSavedGames[index].stage.stage;
}

// FUNCTION: CMR2 0x004f4e50
const SavedGameImage *SavedGames_GetRecord(int index)
{
    if (index < 0 || index >= g_savedGameCount)
        return NULL;
    return &g_pSavedGames[index];
}

// FUNCTION: CMR2 0x004f4e70
void *SavedGames_GetFileName(int index)
{
    return g_savedGameFileNames[index];
}

// FUNCTION: CMR2 0x004f4e80
bool SavedGames_ReleaseRecords(void)
{
    int i;

    for (i = 0; i < g_savedGameCount; i++) {
        if (g_savedGameFileNames[i] != NULL) {
            CFileBuffer::FreeGenericFileBuffer(g_savedGameFileNames[i]);
            g_savedGameFileNames[i] = NULL;
        }
    }
    if (g_pSavedGames != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_pSavedGames);
        g_pSavedGames = NULL;
    }
    if (g_savedGameFileNames != NULL)
        CFileBuffer::FreeGenericFileBuffer(g_savedGameFileNames);
    g_pSavedGames = NULL;
    g_savedGameFileNames = NULL;
    g_savedGameCount = 0;
    return true;
}

// GLOBAL: CMR2 0x00525bec
char g_strSaveGamePattern[8] = "*.rcs";
// GLOBAL: CMR2 0x00525bf4
char g_strSaveGameDirFormat[16] = "%s\\gamesave\\";
// GLOBAL: CMR2 0x00525c04
char g_strSaveGameFileFormat[28] = "%s\\gamesave\\game%.5d.rcs";


// Formats into the buffer the first save game file name that does not exist
// yet (<install>\gamesave\gameNNNNN.rcs), starting from index 0.
// FUNCTION: CMR2 0x004f5150
void SavedGames_MakeUnusedFileName(char *pName)
{
    bool exists = true;
    int i;

    for (i = 0; exists; i++) {
        sprintf(pName, g_strSaveGameFileFormat, CInstallInfo::GetGameHDPath(), i);
        // PORT: _access through Sys.
        if (!Sys_FileExists(pName))
            exists = false;
    }
}

// Copies the string into CFrontend::m_stringDest and writes it back into the
// buffer in groups of four characters, one space between groups (the last
// character is dropped).
// FUNCTION: CMR2 0x004f8a90
void FrontendText_FormatCodeGroups(char *pText)
{
    int out = 0;
    strcpy(CFrontend::m_stringDest, pText);
    if ((int)strlen(CFrontend::m_stringDest) > 0) {
        int i = 1;
        char *source = CFrontend::m_stringDest - 1;
        do {
            pText[out++] = source[i];
            if (i % 4 == 0)
                pText[out++] = ' ';
            i++;
        } while (i - 1 < (int)strlen(CFrontend::m_stringDest));
    }
    pText[out] = 0;
}

// Reads every saved game (<install>\gamesave\*.rcs) into the saved games
// list: one 0x7f4-byte record per file plus its file name.
// FUNCTION: CMR2 0x004f4ef0
// PORT: Sys_ListFiles instead of SetCurrentDirectory + FindFirstFile; each
// save is read by its full game path, and the list keeps the file name.
static int SavedGames_AddListedFile(const char *name, void *pDir)
{
    char path[520];
    void *pRecord;

    sprintf(path, "%s%s", (const char *)pDir, name);
    pRecord = CFileBuffer::GetGenericFileBuffer(path, TRUE);
    if (pRecord != NULL && !SavedGames_IsValidSessionImage(pRecord, CGenericFileLoader::m_fileSize)) {
        Sys_Log("save: ignoring invalid session %s", name);
        CFileBuffer::FreeGenericFileBuffer(pRecord);
        return 1;
    }
    if (pRecord != NULL) {
        g_pSavedGames = (SavedGameImage *)CFileBuffer::ReallocateLockedBuffer(g_pSavedGames, (g_savedGameCount + 1) * sizeof(SavedGameImage));
        g_pSavedGames[g_savedGameCount] = *(SavedGameImage *)pRecord;
        g_savedGameFileNames = (char **)CFileBuffer::ReallocateLockedBuffer(g_savedGameFileNames, (g_savedGameCount + 1) * sizeof(*g_savedGameFileNames));
        g_savedGameFileNames[g_savedGameCount] = (char *)CFileBuffer::AllocateLockedBuffer(0x100);
        strcpy((char *)g_savedGameFileNames[g_savedGameCount], name);
        g_savedGameCount++;
        CFileBuffer::FreeGenericFileBuffer(pRecord);
    }
    return 1;
}

void SavedGames_LoadRecords(void)
{
    char saveDir[260];

    SavedGames_ReleaseRecords();
    g_savedGameCount = 0;
    sprintf(saveDir, g_strSaveGameDirFormat, CInstallInfo::GetGameHDPath());
    Sys_ListFiles(saveDir, g_strSaveGamePattern, SavedGames_AddListedFile, saveDir);
}

// GLOBAL: CMR2 0x00818ac8
int g_frontendOverlayMode;

// FUNCTION: CMR2 0x004ea480
void Frontend_SetOverlayMode(int param1)
{
    // Any other value (and every refused case) clears the mode.
    if (param1 == 1) {
        if (!CGameInfo::IsConfiguredMultiplayer() && CGameInfo::GetConfiguredGameMode() != 4) {
            g_frontendOverlayMode = 1;
            return;
        }
    } else if (param1 == 2) {
        if (!CGameInfo::IsConfiguredMultiplayer() && CGameInfo::GetConfiguredGameMode() != 4) {
            g_frontendOverlayMode = 2;
            return;
        }
    } else if (param1 == 3) {
        if (!CGameInfo::IsConfiguredMultiplayer()) {
            if (CGameInfo::GetConfiguredGameMode() != 4) {
                g_frontendOverlayMode = 3;
                return;
            }
            g_frontendOverlayMode = 3;
        }
    }
    g_frontendOverlayMode = 0;
}

// FUNCTION: CMR2 0x004ea500
int Frontend_GetOverlayMode(void)
{
    return g_frontendOverlayMode;
}

// FUNCTION: CMR2 0x004ea8e0
void GameInfo_SetConfiguredGameMode(BYTE param1)
{
    CGameInfo::m_gameInfo.field_0x14 = ((param1 & 0x7f) << 3) | (CGameInfo::m_gameInfo.field_0x14 & 0xfffffc07);
    if (param1 >= 8)
        CGameInfo::m_gameInfo.field_0x3980 = CGameInfo::m_gameInfo.field_0x14 >> 3 & 0x7f;
}

// FUNCTION: CMR2 0x004ea970
void GameInfo_SetGameModeOptionBits20To22(BYTE param1)
{
    CGameInfo::m_gameInfo.field_0x14 = ((param1 & 7) << 20) | (CGameInfo::m_gameInfo.field_0x14 & 0xff8fffff);
}

// FUNCTION: CMR2 0x004ea990
void GameInfo_GetPlayerOptionNibbles(int *pOut1, int *pOut2)
{
    *pOut1 = CGameInfo::m_gameInfo.field_0x14 >> 23 & 0xf;
    *pOut2 = CGameInfo::m_gameInfo.field_0x14 >> 27 & 0xf;
}

// FUNCTION: CMR2 0x004ea8a0
void GameInfo_SetGameModeBits0To2(BYTE param1)
{
    CGameInfo::m_gameInfo.field_0x14 = ((param1 & 7)) | (CGameInfo::m_gameInfo.field_0x14 & 0xfffffff8U);
}

// FUNCTION: CMR2 0x004ea8c0
void GameInfo_SetConfiguredPlayerCount(BYTE param1)
{
    CGameInfo::m_gameInfo.field_0x14 = ((param1 & 0xf) << 13) | (CGameInfo::m_gameInfo.field_0x14 & 0xfffe1fffU);
}

// FUNCTION: CMR2 0x004ea920
BYTE GameInfo_GetLastNetworkGameMode(void)
{
    return CGameInfo::m_gameInfo.field_0x3980;
}

// FUNCTION: CMR2 0x004ea930
void GameInfo_SetConfiguredDifficulty(BYTE param1)
{
    CGameInfo::m_gameInfo.field_0x14 = ((param1 & 7) << 10) | (CGameInfo::m_gameInfo.field_0x14 & 0xffffe3ffU);
}

// FUNCTION: CMR2 0x004ea950
void GameInfo_SetConfiguredMultiplayer(BYTE param1)
{
    CGameInfo::m_gameInfo.field_0x14 = ((param1 & 1) << 17) | (CGameInfo::m_gameInfo.field_0x14 & 0xfffdffffU);
}

// FUNCTION: CMR2 0x004ea9c0
void GameInfo_SetPlayerOptionNibbles(unsigned int param1, unsigned int param2)
{
    CGameInfo::m_gameInfo.field_0x14 = (((param2 & 0xf) << 4 | param1 & 0xf) << 23) | (CGameInfo::m_gameInfo.field_0x14 & 0x807fffffU);
}

// GLOBAL: CMR2 0x0052af90
DWORD g_sessionTimestamp;

// FUNCTION: CMR2 0x004ea9f0
void GameInfo_ResetSessionTimestamp(void)
{
    g_sessionTimestamp = Sys_GetTicks();
}

// FUNCTION: CMR2 0x004eaa00
DWORD GameInfo_GetSessionTimestamp(void)
{
    return g_sessionTimestamp;
}

// FUNCTION: CMR2 0x004eaa10
void GameInfo_SetSoundOptionBit22(BYTE param1)
{
    CGameInfo::m_gameInfo.field_0x18 = ((param1 & 1) << 22) | (CGameInfo::m_gameInfo.field_0x18 & 0xffbfffffU);
}

// FUNCTION: CMR2 0x004eaa30
char GameInfo_AreOptionsAvailable(void)
{
    return 1;
}

// FUNCTION: CMR2 0x004eaa40
void GameInfo_SetNetworkOptionBits1To2(unsigned int param1)
{
    CGameInfo::m_gameInfo.field_0x1c = ((param1 & 3) << 1) | (CGameInfo::m_gameInfo.field_0x1c & 0xfffffff9U);
}

// FUNCTION: CMR2 0x004eaa60
void GameInfo_SetNetworkOptionBits8To10(unsigned int param1)
{
    CGameInfo::m_gameInfo.field_0x1c = ((param1 & 7) << 8) | (CGameInfo::m_gameInfo.field_0x1c & 0xfffff8ffU);
}

// FUNCTION: CMR2 0x004eaa80
unsigned int GameInfo_GetNetworkOptionBits8To10(void)
{
    return CGameInfo::m_gameInfo.field_0x1c >> 8 & 7;
}

// FUNCTION: CMR2 0x004eaa90
void GameInfo_SetNetworkOptionBits3To7(unsigned int param1)
{
    CGameInfo::m_gameInfo.field_0x1c = ((param1 & 0x1f) << 3) | (CGameInfo::m_gameInfo.field_0x1c & 0xffffff07U);
}

// FUNCTION: CMR2 0x004eaab0
unsigned int GameInfo_GetNetworkOptionBits3To7(void)
{
    return CGameInfo::m_gameInfo.field_0x1c >> 3 & 0x1f;
}

// Number of bits set.
// FUNCTION: CMR2 0x004eaac0
int GameInfo_CountSetBits(unsigned int value)
{
    int count = 0;
    int i = 32;

    do {
        if (value & 1)
            count++;
        value >>= 1;
    } while (--i != 0);
    return count;
}



// Reads back the default chase camera settings.
// FUNCTION: CMR2 0x004eaae0
void GameInfo_GetDefaultCameraParameters(FixVector *pOffset, short *pPitch, int *pLag)
{
    pOffset->x = 0;
    pOffset->y = CGameInfo::m_gameInfo.chaseCameraHeight;
    pOffset->z = CGameInfo::m_gameInfo.chaseCameraDistance;
    *pPitch = CGameInfo::m_gameInfo.chaseCameraPitch;
    *pLag = CGameInfo::m_gameInfo.chaseCameraLag;
}

// FUNCTION: CMR2 0x004eab20
void GameInfo_SetSoundOptionBit30(BYTE param1)
{
    CGameInfo::m_gameInfo.field_0x18 = ((param1 & 1) << 30) | (CGameInfo::m_gameInfo.field_0x18 & 0xbfffffffU);
}

// Turns cheat `bit` on or off in the game options.
// FUNCTION: CMR2 0x004eab40
void GameInfo_SetConfiguredCheat(int bit, int value)
{
    BYTE mask = 1 << bit;

    if (value != 0) {
        *(unsigned int *)((BYTE *)&CGameInfo::m_gameInfo + 0x20) |= (unsigned int)mask << 8;
        return;
    }
    *(unsigned int *)((BYTE *)&CGameInfo::m_gameInfo + 0x20) &= (unsigned int)(BYTE)~mask << 8 | 0xffff00ff;
}

// FUNCTION: CMR2 0x004eab80
void GameInfo_SaveConfiguredCheats(void)
{
    CGameInfo::m_gameInfo.field_0x20 = ((CGameInfo::m_gameInfo.field_0x20 & 0xff00) << 16) | (CGameInfo::m_gameInfo.field_0x20 & 0xffffff);
}

// FUNCTION: CMR2 0x004eaba0
void GameInfo_RestoreConfiguredCheats(void)
{
    CGameInfo::m_gameInfo.field_0x20 = (CGameInfo::m_gameInfo.field_0x20 >> 24) << 8 | (CGameInfo::m_gameInfo.field_0x20 & 0xffff00ffU);
}

// FUNCTION: CMR2 0x004eac80
void GameInfo_SetNetworkOptionBit11(BYTE param1)
{
    CGameInfo::m_gameInfo.field_0x1c = ((param1 & 1) << 11) | (CGameInfo::m_gameInfo.field_0x1c & 0xfffff7ffU);
}

// FUNCTION: CMR2 0x004eaca0
int GameInfo_GetField98(void)
{
    return *(int *)&CGameInfo::m_gameInfo.field_0x98;
}

// FUNCTION: CMR2 0x004eacb0
void GameInfo_SetNetworkOptionBits12To15(unsigned int param1)
{
    CGameInfo::m_gameInfo.field_0x1c = ((param1 & 0xf) << 12) | (CGameInfo::m_gameInfo.field_0x1c & 0xffff0fffU);
}

// FUNCTION: CMR2 0x004eacd0
void GameInfo_SetNetworkOptionBits16To19(unsigned int param1)
{
    CGameInfo::m_gameInfo.field_0x1c = ((param1 & 0xf) << 16) | (CGameInfo::m_gameInfo.field_0x1c & 0xfff0ffffU);
}

// FUNCTION: CMR2 0x004eacf0
unsigned int GameInfo_GetNetworkOptionBits12To15(void)
{
    return CGameInfo::m_gameInfo.field_0x1c >> 12 & 0xf;
}

// FUNCTION: CMR2 0x004ead00
unsigned int GameInfo_GetNetworkOptionBits16To19(void)
{
    return CGameInfo::m_gameInfo.field_0x1c >> 16 & 0xf;
}

void RallyData_SetSelectionBits12To13(BYTE param1);
void RallyData_SetStageSelectionAndRefreshFlags(BYTE param1);
void RallyData_SetCountrySelectionBits(BYTE param1);

// Pushes the frontend option settings to the modules that use them.
// FUNCTION: CMR2 0x004ead10
void GameInfo_RestoreFrontendOptionSettings(void)
{
    GameInfo_SetConfiguredGameMode((BYTE)CGameInfo::m_unk0x0052af94);
    GameInfo_SetConfiguredPlayerCount((BYTE)CGameInfo::m_unk0x0052ea44);
    GameInfo_SetConfiguredDifficulty((BYTE)CGameInfo::m_unk0x0052af9c);
    RallyData_SetSecondarySelectionNibble((BYTE)CGameInfo::m_unk0x0052af80);
    RallyData_SetSelectionBits10To11((BYTE)CGameInfo::m_unk0x0052af84);
    RallyData_SetSelectionBits12To13((BYTE)CGameInfo::m_unk0x0052af8c);
    RallyData_SetSelectionBits16To19((BYTE)CGameInfo::m_unk0x0052e93c);
    RallyData_SetStageSelectionAndRefreshFlags((BYTE)CGameInfo::m_unk0x0052ea48);
    RallyData_SetCountrySelectionBits((BYTE)CGameInfo::m_unk0x0052e940);
    CGameInfo::m_gameInfo.field_0x14 = ((CGameInfo::m_unk0x0052af88 & 0xf) << 4 | CGameInfo::m_unk0x0052af98 & 0xf) << 0x17
        | CGameInfo::m_gameInfo.field_0x14 & 0x807fffff;
}

// FUNCTION: CMR2 0x004eabc0
void GameInfo_ApplyCheatsForGameMode(void)
{
    if (CGameInfo::GetGameInfoSessionFlag() || CGameInfo::GetConfiguredGameMode() == 0 ||
        CGameInfo::GetConfiguredGameMode() == 4) {
        CGameInfo::m_gameInfo.field_0x20 = (CGameInfo::m_gameInfo.field_0x20 & 0xff00ffff) |
                          ((CGameInfo::m_gameInfo.field_0x20 & 0x400) << 8);
    } else if (CGameInfo::GetConfiguredGameMode() == 5 || CGameInfo::GetConfiguredGameMode() == 6) {
        CGameInfo::m_gameInfo.field_0x20 = (CGameInfo::m_gameInfo.field_0x20 & 0xff00ffff) |
                          ((CGameInfo::m_gameInfo.field_0x20 & 0xd00) << 8);
    } else {
        CGameInfo::m_gameInfo.field_0x20 = (CGameInfo::m_gameInfo.field_0x20 & 0xff00ffff) |
                          ((CGameInfo::m_gameInfo.field_0x20 & 0xff00) << 8);
    }
}

// GLOBAL: CMR2 0x00511300
double g_fixedDegreesToAngle12 = 4096.0 / (360.0 * 65536.0);   // 16.16 degrees -> sine table index
// One RGBA colour: OptionMenu_GetStateByteAddress returns all four bytes, which callers
// read as a dword. Independent globals do not preserve this layout.
// GLOBAL: CMR2 0x0082b1b8
BYTE g_unk0x0082b1b8[4];


// GLOBAL: CMR2 0x00511cd8
int g_unk0x00511cd8[4] = {
    0x1bff2d87, 0x11d24cb7, 0x60004cb1, 0xe0014e08,
};

typedef HRESULT (__stdcall *DPMethod5GI)(void *pThis, DWORD a1, DWORD a2, DWORD a3, DWORD a4, DWORD a5);

// Adds a session found by DirectPlay to the list (at most 20, ignoring the
// blank-named ones): a copy of its description with its own name buffer.
// FUNCTION: CMR2 0x004a0ca0
void Session_AddToList(NetSessionDesc *pDesc)
{
    unsigned int n;

    if (CGameInfo::m_unk0x005a01bc >= 20)
        return;
    if (strcmp(CMain::m_logFileBlankLine, pDesc->lpszSessionNameA) == 0)
        return;
    n = CGameInfo::m_unk0x005a01bc;
    CGameInfo::m_enumeratedSessions[n] = *pDesc;
    strcpy(CGameInfo::m_sessionNames[n], pDesc->lpszSessionNameA);
    CGameInfo::m_enumeratedSessions[n].lpszSessionNameA = CGameInfo::m_sessionNames[n];
    CGameInfo::m_unk0x005a01bc++;
}

// DirectPlay EnumSessions callback: stops on time-out, otherwise lists the session.
// FUNCTION: CMR2 0x004a12b0
BOOL Session_EnumCallback(const NetSessionDesc *pDesc, void *pContext)
{
    Session_AddToList((NetSessionDesc *)pDesc);
    return TRUE;
}


// Fills the session descriptor at 0x5a0068 and enumerates the DirectPlay
// sessions. Returns 1 on NET_OK, -1 and -2 for two DirectPlay errors (the
// caller reports them), 0 otherwise.
// FUNCTION: CMR2 0x004a13b0
int CGameInfo::EnumerateNetworkSessions(void)
{
    NetGuid application;

    if (m_unk0x005a1814 != 0)
        return 0;
    ResetNetworkSessionState();
    memset(&g_networkSession, 0, sizeof(g_networkSession));
    ((int *)&g_networkSession.guidApplication)[0] = g_unk0x00511cd8[0];
    g_networkSession.lpszSessionNameA = (char *)&m_unk0x005a00b8;
    g_networkSession.dwSize = sizeof(g_networkSession);
    ((int *)&g_networkSession.guidApplication)[1] = g_unk0x00511cd8[1];
    ((int *)&g_networkSession.guidApplication)[2] = g_unk0x00511cd8[2];
    ((int *)&g_networkSession.guidApplication)[3] = g_unk0x00511cd8[3];
    if (CGame::GetDirectPlay() == NULL)
        return 0;
    // PORT: synchronous enumeration with the session callback.
    memcpy(&application, &g_networkSession.guidApplication, sizeof(application));
    switch (Net_EnumSessions(&application, 1000, Session_EnumCallback, NULL)) {
    case NET_OK:
        return 1;
    case 0x8877015e:
        return -1;
    case 0x88770118:
        return -2;
    case 0x8877005a:
        return 0;
    case 0x88770082:
        return 0;
    case 0x887700aa:
        return 0;
    case 0x88770140:
        return 0;
    }
    return 0;
}

// Same as EnumerateNetworkSessions, but also supplies the session password
// and enumerates with flags 0x51.
// FUNCTION: CMR2 0x004a12d0
int CGameInfo::EnumerateNetworkSessionsWithUserData(char *pPassword)
{
    NetGuid application;

    if (m_unk0x005a1814 != 0)
        return 0;
    ResetNetworkSessionState();
    memset(&g_networkSession, 0, sizeof(g_networkSession));
    g_networkSession.lpszPasswordA = pPassword;
    ((int *)&g_networkSession.guidApplication)[0] = g_unk0x00511cd8[0];
    g_networkSession.lpszSessionNameA = (char *)&m_unk0x005a00b8;
    g_networkSession.dwSize = sizeof(g_networkSession);
    ((int *)&g_networkSession.guidApplication)[1] = g_unk0x00511cd8[1];
    ((int *)&g_networkSession.guidApplication)[2] = g_unk0x00511cd8[2];
    ((int *)&g_networkSession.guidApplication)[3] = g_unk0x00511cd8[3];
    if (CGame::GetDirectPlay() == NULL)
        return 0;
    // PORT: synchronous enumeration with the session callback.
    memcpy(&application, &g_networkSession.guidApplication, sizeof(application));
    switch (Net_EnumSessions(&application, 1000, Session_EnumCallback, NULL)) {
    case NET_OK:
        return 1;
    case 0x8877015e:
        return -1;
    case 0x88770118:
        return -2;
    case 0x8877005a:
    case 0x88770082:
    case 0x887700aa:
    case 0x88770140:
        return 0;
    }
    return 0;
}



// Cambia el modo activo 0x82ca1c (intercambiando 0x3c con el modo anterior) y
// reinicia el temporizador.
// FUNCTION: CMR2 0x00505a60
void CGameInfo::SwitchOptionPreviewMode(int param1)
{
    int current;
    FixInterp *pNew;
    FixInterp *pOld;
    int tmp;

    current = (signed char)g_selectedOptionPreviewPanel;
    if (current == param1)
        return;
    if (g_selectedOptionPreviewPanel != 0xff && param1 != -1) {
        pOld = &g_optionPreviewPanels[current];
        pNew = &g_optionPreviewPanels[param1];
        tmp = pOld->active;
        pOld->active = pNew->active;
        pNew->active = tmp;
        tmp = pOld->current;
        pOld->current = pNew->current;
        pNew->current = tmp;
        tmp = pOld->end;
        pOld->end = pNew->end;
        pNew->end = tmp;
        tmp = pOld->phase2;
        pOld->phase2 = pNew->phase2;
        pNew->phase2 = tmp;
        tmp = pOld->start;
        pOld->start = pNew->start;
        pNew->start = tmp;
        tmp = pOld->distance;
        pOld->distance = pNew->distance;
        pNew->distance = tmp;
        tmp = pOld->phase1;
        pOld->phase1 = pNew->phase1;
        pNew->phase1 = tmp;
        tmp = pOld->startTime3;
        pOld->startTime3 = pNew->startTime3;
        pNew->startTime3 = tmp;
        tmp = pOld->startTime2;
        pOld->startTime2 = pNew->startTime2;
        pNew->startTime2 = tmp;
        tmp = pOld->startTime;
        pOld->startTime = pNew->startTime;
        pNew->startTime = tmp;

    }
    g_unk0x0082c6c0 = (int)CMain::GetFrameDelta();
    g_unk0x0082cb44 = 0;
    g_selectedOptionPreviewPanel = (BYTE)param1;
}


#define FIX_ABS(x) ((x) < 0 ? -(x) : (x))

// FUNCTION: CMR2 0x005039d0
void FixInterp_StartToOne(FixInterp *p)
{
    if (p->current != 0x10000) {
        p->start = p->current;
        p->end = 0x10000;
        p->active = 1;
        p->distance = FIX_ABS(FixMul(p->end, 0x320000) - FixMul(p->start, 0x320000));
        p->startTime = CMain::GetFrameDelta();
    }
}

// FUNCTION: CMR2 0x00503aa0
void FixInterp_StartToZero(FixInterp *p)
{
    if (p->current != 0) {
        p->start = p->current;
        p->end = 0;
        p->active = 1;
        p->distance = FIX_ABS(FixMul(p->end, 0x320000) - FixMul(p->start, 0x320000));
        p->startTime = CMain::GetFrameDelta();
    }
}

// GLOBAL: CMR2 0x0082b668
Menu g_controlSetupMenu;

// FUNCTION: CMR2 0x00502500
Menu *OptionMenu_GetControlSetupMenu(void)
{
    return &g_controlSetupMenu;
}

// GLOBAL: CMR2 0x0082b848
Menu g_valueSliderMenu;

// FUNCTION: CMR2 0x00502510
Menu *OptionMenu_GetValueSliderMenu(void)
{
    return &g_valueSliderMenu;
}

CarSetup *RallyData_GetPlayerCarSetup(int index);

// Copies the seven option bytes of every rally data record into rows 4..7 of
// the default table, mirrors them into rows 0..3 and clears the option flags.
// match 70%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00502570
void OptionMenu_LoadCarSetups(void)
{
    int i;

    for (i = 0; i < 4; i++) {
        CarSetup *p = RallyData_GetPlayerCarSetup(i);

        g_optionCarSetups[i + 4].brakeBias = p->brakeBias;
        g_optionCarSetups[i + 4].brakePower = p->brakePower;
        g_optionCarSetups[i + 4].gearbox = p->gearbox;
        g_optionCarSetups[i + 4].steering = p->steering;
        g_optionCarSetups[i + 4].powerRatio = p->powerRatio;
        g_optionCarSetups[i + 4].suspension = p->suspension;
        g_optionCarSetups[i + 4].tyres = p->tyres;
        g_optionCarSetups[i].brakeBias = g_optionCarSetups[i + 4].brakeBias;
        g_optionCarSetups[i].brakePower = g_optionCarSetups[i + 4].brakePower;
        g_optionCarSetups[i].gearbox = g_optionCarSetups[i + 4].gearbox;
        g_optionCarSetups[i].steering = g_optionCarSetups[i + 4].steering;
        g_optionCarSetups[i].powerRatio = g_optionCarSetups[i + 4].powerRatio;
        g_optionCarSetups[i].suspension = g_optionCarSetups[i + 4].suspension;
        g_optionCarSetups[i].tyres = g_optionCarSetups[i + 4].tyres;
        memset(&g_optionColumnDirty[i][0], 0, 7);
    }
}

void OptionMenu_ResetRecords(void);

// Resets the mode menu cursor and the value byte of its first two items.
// FUNCTION: CMR2 0x00502520
void OptionMenu_ResetModeCursor(void)
{
    (&g_controlSetupMenu)->cursor = 0;
    (OptionMenu_GetControlSetupMenu())->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 1)].max = 0;
    (OptionMenu_GetControlSetupMenu())->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 2)].max = 0;
    OptionMenu_ResetRecords();
}

// GLOBAL: CMR2 0x0052aa60
Texture *g_inRaceMenuUpArrowTexture;
// GLOBAL: CMR2 0x0052aa68
Texture *g_inRaceMenuDownArrowTexture;
// GLOBAL: CMR2 0x0052aa70
Menu g_menu0x0052aa70;
// GLOBAL: CMR2 0x0052af41
BYTE g_inRaceMenuOpening;
// GLOBAL: CMR2 0x0052af44
Menu *g_pMenu0x0052af44;
// GLOBAL: CMR2 0x0052af4c
Texture *g_inRaceMenuRoundBoxTexture;
// GLOBAL: CMR2 0x0052af50
Texture *g_inRaceMenuCurtainTexture;
extern BYTE g_unk0x0052af58[2];

// In-race network menus (built by 0x402c40..0x404000).
// GLOBAL: CMR2 0x00529918
Menu g_menu0x00529918;
// GLOBAL: CMR2 0x00529af8
Menu g_menu0x00529af8;
// GLOBAL: CMR2 0x00529ce8
Menu g_menu0x00529ce8;
// GLOBAL: CMR2 0x00529ed8
Menu g_menu0x00529ed8;
// GLOBAL: CMR2 0x0052a0c0
Menu g_menu0x0052a0c0;
// GLOBAL: CMR2 0x0052a2a8
Menu g_menu0x0052a2a8;
// GLOBAL: CMR2 0x0052a490
Menu g_menu0x0052a490;
// GLOBAL: CMR2 0x0052a670
Menu g_menu0x0052a670;
// GLOBAL: CMR2 0x0052a870
Menu g_menu0x0052a870;
// GLOBAL: CMR2 0x0052ad54
int g_unk0x0052ad54;
// GLOBAL: CMR2 0x0052ad60
Menu g_menu0x0052ad60;
// GLOBAL: CMR2 0x0052af48
Menu *g_pMenu0x0052af48;
// GLOBAL: CMR2 0x0052af6c
int g_unk0x0052af6c;

void InRaceMenu_Close(void);
void Menu_QueueDefaultAction(void);
void Sound_ClearMusicPauseFlag(void);
unsigned char RallyData_GetSelectionFlag26(void);
unsigned char RallyData_GetSelectionFlag28(void);
int RallyData_IsHeadToHeadRaceMode(void);

// FUNCTION: CMR2 0x00401850
void InRaceMenu_ResumeWithSound(Menu *pMenu, int param)
{
    if ((BYTE)RallyData_GetSelectionFlag26())
        Sound_ClearMusicPauseFlag();
    InRaceMenu_Close();
}

// FUNCTION: CMR2 0x00402bb0
void InRaceMenu_RememberOptionCursor(Menu *pMenu, char param)
{
    if (param == 0) {
        pMenu->cursor = (char)Menu_FindItem(pMenu, 7);
        g_unk0x0052af6c = pMenu->cursor;
        return;
    }
    g_unk0x0052af6c = pMenu->cursor;
}

// FUNCTION: CMR2 0x00402bf0
void InRaceMenu_SkipBlankOptionRow(Menu *pMenu)
{
    if (pMenu->cursor == 2)
        pMenu->cursor = g_unk0x0052af6c < 2 ? 3 : 1;
    g_unk0x0052af6c = pMenu->cursor;
}

// FUNCTION: CMR2 0x00402c30
void InRaceMenu_ResumeDrawCallback(Menu *pMenu)
{
    InRaceMenu_Close();
}

// FUNCTION: CMR2 0x00402c40
void InRaceMenu_BuildResumePage(void)
{
    Menu_Init(&g_menu0x00529ce8, 0, 0, 0, NULL, NULL, 1, 0, 1);
    Menu_AddItemType4(&g_menu0x00529ce8, 0, -1, 0, -1);
    Menu_SetCallbacks(&g_menu0x00529ce8, NULL, NULL, (MenuCallback)InRaceMenu_ResumeDrawCallback, NULL);
}

// Callback 0 of the network options menu: loads the two switches.
// FUNCTION: CMR2 0x00402eb0
void InRaceMenu_EnterNetworkOptions(Menu *pMenu, char param)
{
    if (param == 0) {
        if (CGameInfo::IsSplitBarEnabled() != 0)
            pMenu->items[Menu_FindItem(pMenu, 2)].max = 1;
        else
            pMenu->items[Menu_FindItem(pMenu, 2)].max = 0;
        if ((BYTE)CGameInfo::IsDashOptionEnabled()) {
            pMenu->items[Menu_FindItem(pMenu, 4)].max = 1;
            return;
        }
        pMenu->items[Menu_FindItem(pMenu, 4)].max = 0;
    }
}

// FUNCTION: CMR2 0x00402f70
void InRaceMenu_ReturnToPauseItem(Menu *pMenu, int param)
{
    Menu_SetNextAction(&g_menu0x0052ad60);
}

// FUNCTION: CMR2 0x00402f80
void InRaceMenu_ReturnToPauseUpdate(Menu *pMenu)
{
    Menu_SetNextAction(&g_menu0x0052ad60);
}

void InRaceMenu_ApplyNetworkOptions(Menu *pMenu, int param);
void InRaceMenu_DrawLabelAndValueRows(Menu *pMenu);
void InRaceMenu_DrawSliderRows(Menu *pMenu);
void InRaceMenu_DrawCarSetupChoices(Menu *pMenu);

// Builds the network options menu and disables the two-state entries when
// the current rally mode does not support them.
void InRaceMenu_ApplyNetworkOptions(Menu *pMenu, int param);
void InRaceMenu_DrawLabelAndValueRows(Menu *pMenu);
void InRaceMenu_DrawSliderRows(Menu *pMenu);
void InRaceMenu_DrawCarSetupChoices(Menu *pMenu);
// FUNCTION: CMR2 0x00402f90
void InRaceMenu_BuildNetworkOptions(void)
{
    int i;

    if ((char)RallyDataState() == 1)
        Menu_Init(&g_menu0x00529ed8, 0, 0, 0, &g_menu0x0052ad60, NULL, 1, 0, 1);
    else
        Menu_Init(&g_menu0x00529ed8, 0, 0, 0, &g_menu0x00529918, NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x00529ed8, 0, 0x33, &g_menu0x0052aa70, 0, 0);
    Menu_AddItemType2(&g_menu0x00529ed8, 0, 0x5c, &g_menu0x0052a870, 0, 1);
    Menu_AddItemType3(&g_menu0x00529ed8, 0, 0x35, 2, 0, 0, 0, 0, 2);
    Menu_AddItemType3(&g_menu0x00529ed8, 0, 0x39, 2, 0, 0, 0, 0, 4);
    Menu_AddItemType2(&g_menu0x00529ed8, 0, 0x3b, &g_menu0x0052ad60,
                      (MenuItemAction)InRaceMenu_ApplyNetworkOptions, -1);
    Menu_SetCallbacks(&g_menu0x00529ed8, (MenuCallback)InRaceMenu_EnterNetworkOptions, NULL,
                      (MenuCallback)InRaceMenu_DrawLabelAndValueRows, NULL);
    Menu_ValidateCursor(&g_menu0x00529ed8, 0);
    if (RallyData_IsHeadToHeadRaceMode() == 0) {
        for (i = 0; i < g_menu0x00529ed8.itemCount; i++) {
            if (g_menu0x00529ed8.items[i].value == 2) {
                g_menu0x00529ed8.items[i].enabled = 0;
                g_menu0x00529ed8.items[i].visible = 0;
            }
        }
    }
}

// FUNCTION: CMR2 0x00403360
void InRaceMenu_SelectLastItem(Menu *pMenu, int param)
{
    pMenu->cursor = pMenu->itemCount - 1;
}

// FUNCTION: CMR2 0x004035d0
void InRaceMenu_SelectPlayer(Menu *pMenu, int param)
{
    g_unk0x0052af58[1] = pMenu->cursor;
}

void InRaceMenu_ResetCameraDefaults(Menu *pMenu, char unused);
void InRaceMenu_EnterCameraOffsetOptions(Menu *pMenu, char unused);
void InRaceMenu_LeaveCameraOffsetOptions(Menu *pMenu, char cancel);
void InRaceMenu_UpdateCameraOffsets(Menu *pMenu);

// Builds the four-slider camera options menu.
// FUNCTION: CMR2 0x004035e0
void InRaceMenu_BuildCameraOffsetOptions(void)
{
    Menu_Init(&g_menu0x0052a870, 0, 0, 0, &g_menu0x00529ed8, NULL, 1, 0, 1);
    Menu_AddItemType3(&g_menu0x0052a870, 0, 0x5e, 0x15, 10, 0, 0, 0, 0);
    // PORT: distance and stiffness have ten more steps below the original's
    // range (see InRaceMenu_ReadCameraOffsetSliders).
    Menu_AddItemType3(&g_menu0x0052a870, 0, 0x5f, 0x1f, 10, 0, 0, 0, 1);
    Menu_AddItemType3(&g_menu0x0052a870, 0, 0x60, 0x1f, 10, 0, 0, 0, 2);
    Menu_AddItemType3(&g_menu0x0052a870, 0, 0x61, 0x15, 10, 0, 0, 0, 3);
    Menu_AddItemType4(&g_menu0x0052a870, 0, 0x62, (MenuItemAction)InRaceMenu_ResetCameraDefaults, 4);
    Menu_AddItemType2(&g_menu0x0052a870, 0, 0x3b, &g_menu0x00529ed8, 0, 5);
    Menu_SetCallbacks(&g_menu0x0052a870, (MenuCallback)InRaceMenu_EnterCameraOffsetOptions,
                      (MenuCallback)InRaceMenu_UpdateCameraOffsets, (MenuCallback)InRaceMenu_DrawSliderRows,
                      (MenuCallback)InRaceMenu_LeaveCameraOffsetOptions);
    Menu_ValidateCursor(&g_menu0x0052a870, 0);
}

// GLOBAL: CMR2 0x0052a0b8
short g_unk0x0052a0b8;

unsigned short Input_GetControllerSlotMapping(unsigned short slot);

// Returns 1 when the slot's device pressed "back" (button 0x400, or the
// joystick button mapped to action 9, which is then remembered).
// FUNCTION: CMR2 0x00404e10
int InRaceMenu_IsBackPressed(unsigned short slot)
{
    DeviceInfo *pDev = CInput::GetAvailableDeviceRecord(Input_GetControllerSlotMapping(slot));

    if (pDev->type == 1 || pDev->type == 2) {
        if ((pDev->pressed & 0x400) != 0)
            return 1;
    } else {
        if ((pDev->pressed & (unsigned short)CInput::GetButtonMapping(slot, 9)) != 0) {
            g_unk0x0052a0b8 = CInput::GetButtonMapping(slot, 9);
            return 1;
        }
        if ((CInput::GetAvailableDeviceRecord(0)->pressed & 0x400) != 0)
            return 1;
    }
    return 0;
}

// Chase camera settings chosen on the camera menu's sliders: pitch, offset
// (height, distance) and lag.
// GLOBAL: CMR2 0x0052a86c
short g_cameraMenuPitch;
// GLOBAL: CMR2 0x0052aa50
FixVector g_cameraMenuOffset;
// GLOBAL: CMR2 0x0052aa5c
int g_cameraMenuLag;

// Callback of the camera options menu: turns the four sliders into values.
// FUNCTION: CMR2 0x00403200
// PORT: the distance (5 to 10 in steps of 0.25) and the stiffness (8 to 16
// in steps of 0.4) reach ten steps lower, to 2.5 and 4: a closer camera and
// a stiffer one. The steps and the stored values are the original's.
void InRaceMenu_ReadCameraOffsetSliders(Menu *pMenu)
{
    g_cameraMenuLag = (int)(pMenu->items[2].max * 0xc0000) / (pMenu->items[2].min - 1) + 0x40000;
    g_cameraMenuPitch = (short)((int)(pMenu->items[3].max * 0x11c) / (pMenu->items[3].min - 1));
    g_cameraMenuOffset.y = (int)(pMenu->items[0].max * 0xcccd) / (pMenu->items[0].min - 1) + 0x13333;
    g_cameraMenuOffset.z = (int)(pMenu->items[1].max * 0x78000) / (pMenu->items[1].min - 1) + 0x28000;
}

int Race_IsFlag24Clear(void);

// Callback of the sound options menu: applies the three volumes.
// FUNCTION: CMR2 0x00401380
void InRaceMenu_ApplySoundVolumes(Menu *pMenu)
{
    int rate;

    CGameInfo::SetMasterSoundVolume(pMenu->items[Menu_FindItem(pMenu, 0)].max * 10);
    CGameInfo::SetEffectsSoundVolume(pMenu->items[Menu_FindItem(pMenu, 1)].max * 10);
    if (Race_IsFlag24Clear())
        CGameInfo::SetCoDriverSoundVolume(pMenu->items[Menu_FindItem(pMenu, 2)].max * 10);
    CInput::SetMenuSoundVolume(((int)(CGameInfo::GetEffectsSoundVolume() << 16) / 100) / 4);
}

// Values the in-race option menus started with (restored on cancel).
// GLOBAL: CMR2 0x005298f8
BYTE g_unk0x005298f8;
// GLOBAL: CMR2 0x00529ecc
int g_unk0x00529ecc;
// GLOBAL: CMR2 0x0052a488
int g_unk0x0052a488;
// GLOBAL: CMR2 0x0052a850
BYTE g_unk0x0052a850;
// GLOBAL: CMR2 0x0052ad50
int g_unk0x0052ad50;

BYTE StageUI_GetRaceEndEventCount(void);
BYTE *StageUI_GetRaceResultTable(void);
void Race_UpdatePlayerViewAndDrivenCars(BYTE, int);
void RallyData_SetDriverHudFlags(BYTE index, BYTE *pValue);
void RallyData_MarkDriverRecordDirty(int index);
int RallyData_IsHeadToHeadRaceMode(void);

/* --------------------------------------------------------------------------
   Stage entry / exit transitions of the frontend cascades (0x401000..0x405470).
   -------------------------------------------------------------------------- */

unsigned char Race_LoadSelectedStage(void);
unsigned char Race_LoadStageGeometry(void);
void StageUI_ClearRaceEndLatch(void);
void RallyData_LoadNavigationSounds(void);
void RallyData_RestoreAllCarRaceRecords(void);
void StageSound_LoadSurfaceVariations(void);
void Race_LoadCollisionSoundBanks(void);
void Race_InitCoDriverSamples(void);
void Race_ReleaseFrameResources(void);
void RallyData_DrawLoadingProgress(int progress, char drawScene, BYTE alpha);
void RallyData_GrowEntryAndNotifyCompletion(CallbackStateMachine *, unsigned int);
BYTE RallyData_FreeChallengeSceneObjects(void);

// GLOBAL: CMR2 0x005297f0
int g_unk0x005297f0;
// GLOBAL: CMR2 0x00536ac8
BYTE g_unk0x00536ac8;

// Enters a stage group: either builds the group's stage list (loading each
// entry) or runs the fade-in sequence of the next group, then forwards the
// event to the stage-list object.
// FUNCTION: CMR2 0x00401000
void GameInfo_EnterStageGroup(CallbackStateMachine *param1, int param2)
{
    int i;

    if ((char)param2 == 0) {
        if ((BYTE)Race_LoadSelectedStage() == 0) {
            i = 0;
            if (param1->count > 0) {
                do {
                    CGame::PromoteCallbackEntryByRule(param1, i, 1, 3);
                    i++;
                } while (i < (int)param1->count);
            }
            CGame::SetFrontendResourceMode(0);
            Race_ReleaseFrameResources();
            StageUI_ClearRaceEndLatch();
            return;
        }
        RallyData_DrawLoadingProgress(0x46, 1, 0xff);
        StageSound_LoadSurfaceVariations();
        Race_LoadCollisionSoundBanks();
        Race_InitCoDriverSamples();
        RallyData_LoadNavigationSounds();
        CGame::RegisterCallback((void *)RallyData_FreeChallengeSceneObjects, 0);
        RallyData_DrawLoadingProgress(0x55, 1, 0xff);
    }
    CGame::PromoteCallbackEntryByRule(param1, param2, 0, 2);
}

// Leaves a stage group: tears the stage list down or runs the fade-out
// sequence, then forwards the event to the stage-list object.
// FUNCTION: CMR2 0x004010a0
void GameInfo_LeaveStageGroup(CallbackStateMachine *param1, int param2)
{
    int i;

    if ((char)param2 == 0) {
        RallyData_RestoreAllCarRaceRecords();
        if ((BYTE)Race_LoadStageGeometry() == 0) {
            i = 0;
            if (param1->count > 0) {
                do {
                    CGame::PromoteCallbackEntryByRule(param1, i, 1, 3);
                    i++;
                } while (i < (int)param1->count);
            }
            CGame::SetFrontendResourceMode(0);
            Race_ReleaseFrameResources();
            StageUI_ClearRaceEndLatch();
            return;
        }
        RallyData_DrawLoadingProgress(0x64, 1, 0xff);
        if (CGameInfo::IsConfiguredMultiplayer())
            CGraphics::SetClearColour(0, 0, 0, 0);
        else
            CGraphics::SetClearColour(0, 0x9c, 0xb4, 0xac);
    }
    g_unk0x005297f0 = 1;
    g_unk0x00536ac8 = 1;
    CGame::PromoteCallbackEntryByRule(param1, param2, 0, 2);
}

void NetPlayers_SendCarClass(BYTE);
void NetRace_SetRaceControlFlag(BYTE);
void NetRace_ResetLastSentTimeIndices(void);
void Race_SetFlag37F08(void);
void NetRace_ResetStateAndTriangleTable(void);
void StageTiming_RestoreDeviceStageResources(void);
void ForceFeedback_ActivateIdleSlots(void);
void View_SetCameraType(int, int, BYTE, int);
BYTE StageUI_GetRaceEndEventCount(void);
int RallyData_GetDriverCarSelection(BYTE);
int RallyData_IsChampionshipFinalStage(void);
void RallyData_ApplyCountryCarSetups(void);
void RallyData_ClearDriverGroupRecords(void);
int View_IsModeAvailable(BYTE, int);
extern BYTE g_unk0x00537fd4;
// GLOBAL: CMR2 0x0053811d
BYTE g_unk0x0053811d;
// GLOBAL: CMR2 0x005298f4
int g_unk0x005298f4;

// Advances the in-race menu cascade one step: refreshes the per-player window
// entries and moves to the next state when the race is over.
// FUNCTION: CMR2 0x00401150
void InRaceMenu_AdvanceCascade(CallbackStateMachine *param1, int param2)
{
    int i;

    if (CGameInfo::GetGameModeOptionBit19() != 0)
        NetPlayers_SendCarClass(2);
    NetRace_SetRaceControlFlag(0);
    g_unk0x0053811d = 0;
    NetRace_ResetLastSentTimeIndices();
    CSound::NoOpSoundDeviceCallback();
    g_unk0x00537fd4 = 0;
    Race_SetFlag37F08();
    if ((char)param2 == 0) {
        if (g_unk0x005297f0 != 0) {
            NetRace_ResetStateAndTriangleTable();
            StageTiming_RestoreDeviceStageResources();
            g_unk0x005298f4 = 1;
            g_unk0x005297f0 = 0;
            if ((char)((CallbackStateMachine *)StageUI_GetRaceResultTable())->records[0].packedState != '\r')
                ForceFeedback_ActivateIdleSlots();
        }
        i = 0;
        if ((BYTE)RallyDataState() > 0) {
            do {
                if ((BYTE)CGameInfo::GetGameInfoSessionFlag() != 0 || (BYTE)RallyData_IsChampionshipFinalStage() != 0) {
                    View_SetCameraType(i, 7, i, 0);
                } else if (View_IsModeAvailable(i, RallyData_GetDriverCarSelection(StageUI_GetRaceEndEventCount() + i)) != 0) {
                    View_SetCameraType(i, RallyData_GetDriverCarSelection(StageUI_GetRaceEndEventCount() + i), i, 0);
                } else {
                    View_SetCameraType(i, 4, i, 0);
                }
                i++;
            } while (i < (BYTE)RallyDataState());
        }
        g_unk0x005298f4--;
        Race_UpdatePlayerViewAndDrivenCars((BYTE)param1->records[0].packedState, 0);
    }
    if (g_unk0x005298f4 <= 0) {
        if ((BYTE)CGameInfo::GetGameInfoSessionFlag() != 0) {
            CGame::PromoteCallbackEntryByRule(param1, param2, 6, 2);
            return;
        }
        if (CGameInfo::GetConfiguredGameMode() == 4) {
            CGameInfo::SetInputAndGamePaused(1);
            CGame::PromoteCallbackEntryByRule(param1, param2, 5, 2);
            return;
        }
        if ((BYTE)RallyData_IsChampionshipFinalStage() != 0) {
            RallyData_ApplyCountryCarSetups();
            RallyData_ClearDriverGroupRecords();
            CGame::PromoteCallbackEntryByRule(param1, param2, 1, 2);
            return;
        }
        CGame::PromoteCallbackEntryByRule(param1, param2, 0, 2);
    }
}

// Callback 0 of the sound options menu: loads the three volumes.
// FUNCTION: CMR2 0x004012d0
void InRaceMenu_EnterSoundOptions(Menu *pMenu, int param)
{
    g_unk0x00529ecc = CGameInfo::GetMasterSoundVolume();
    g_unk0x0052a488 = CGameInfo::GetEffectsSoundVolume();
    g_unk0x0052ad50 = CGameInfo::GetCoDriverSoundVolume();
    pMenu->items[Menu_FindItem(pMenu, 0)].max = g_unk0x00529ecc / 10;
    pMenu->items[Menu_FindItem(pMenu, 1)].max = g_unk0x0052a488 / 10;
    if (Race_IsFlag24Clear())
        pMenu->items[Menu_FindItem(pMenu, 2)].max = g_unk0x0052ad50 / 10;
}

// Callback 0 of the car setup menu: loads the four switches from the driver's
// setting byte (resetting them to on).
void InRaceMenu_EnterCarSetup(Menu *pMenu, int param);
void InRaceMenu_LeaveCarSetup(Menu *pMenu, char cancel);
void InRaceMenu_EncodeCarSetup(Menu *pMenu);

// Builds the car-setup menu used from the in-race network menu.
// match 80%: below the 90% bar; kept as FUNCTION so reccmp measures it.
// FUNCTION: CMR2 0x00404000
void InRaceMenu_BuildCarSetup(void)
{
    int settingId;

    Menu_Init(&g_menu0x0052aa70, 0, 0, 0, &g_menu0x00529ed8, NULL, 1, 0, 1);
    if ((BYTE)RallyData_GetFlag24() == 0 && (BYTE)RallyData_GetSelectionFlag28() == 0)
        Menu_AddItemType3(&g_menu0x0052aa70, 0, (short)0x64, 2, 0, 0, 0, 0, 0);
    else
        Menu_AddItemType3(&g_menu0x0052aa70, 0, (short)0x30, 2, 0, 0, 0, 0, 0);
    Menu_AddItemType3(&g_menu0x0052aa70, 0, 0x65, 2, 0, 0, 0, 0, 1);
    Menu_AddItemType3(&g_menu0x0052aa70, 0, 0x66, 2, 0, 0, 0, 0, 2);
    Menu_AddItemType3(&g_menu0x0052aa70, 0, 0x67, 2, 0, 0, 0, 0, 3);
    if (Race_IsFlag24Clear()) {
        if (RallyData_IsHeadToHeadRaceMode() != 0)
            Menu_AddItemType3(&g_menu0x0052aa70, 0, 0x68, (BYTE)2, 0, 0, 0, 0, 4);
        else
            Menu_AddItemType3(&g_menu0x0052aa70, 0, 0x68, (BYTE)3, 0, 0, 0, 0, 4);
    }
void InRaceMenu_ApplyDefaultCarSetup(Menu *pMenu, int param);

    Menu_AddItemType4(&g_menu0x0052aa70, 0, 0x62, (MenuItemAction)InRaceMenu_ApplyDefaultCarSetup, 5);
    Menu_AddItemType2(&g_menu0x0052aa70, 0, 0x3b, &g_menu0x00529ed8, 0, 6);
    Menu_SetCallbacks(&g_menu0x0052aa70, (MenuCallback)InRaceMenu_EnterCarSetup,
                      (MenuCallback)InRaceMenu_EncodeCarSetup, (MenuCallback)InRaceMenu_DrawCarSetupChoices,
                      (MenuCallback)InRaceMenu_LeaveCarSetup);
    Menu_ValidateCursor(&g_menu0x0052aa70, 0);
}

// FUNCTION: CMR2 0x00404130
void InRaceMenu_ApplyDefaultCarSetup(Menu *pMenu, int param)
{
    g_unk0x005298f8 = (g_unk0x005298f8 & 0xfc) | 0x3c;
    RallyData_SetDriverHudFlags(StageUI_GetRaceEndEventCount() + g_unk0x0052af58[1], &g_unk0x005298f8);
    pMenu->items[0].max = (g_unk0x005298f8 >> 2) & 1;
    pMenu->items[1].max = (g_unk0x005298f8 >> 3) & 1;
    pMenu->items[2].max = (g_unk0x005298f8 >> 4) & 1;
    pMenu->items[3].max = (g_unk0x005298f8 >> 5) & 1;
    if (RallyData_IsHeadToHeadRaceMode() != 0) {
        if (Race_IsFlag24Clear())
            pMenu->items[Menu_FindItem(pMenu, 4)].max = 1;
    } else if (Race_IsFlag24Clear()) {
        pMenu->items[Menu_FindItem(pMenu, 4)].max = 2;
    }
}

// Callback of the car setup menu: encodes the selected switches and tyres.
// match 72%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00404d00
void InRaceMenu_EncodeCarSetup(Menu *pMenu)
{
    g_unk0x005298f8 = (g_unk0x005298f8 & ~4) | ((pMenu->items[0].max & 1) << 2);
    g_unk0x005298f8 = (g_unk0x005298f8 & ~8) | ((pMenu->items[1].max & 1) << 3);
    g_unk0x005298f8 = (g_unk0x005298f8 & ~0x20) | ((pMenu->items[3].max & 1) << 5);

    if (Race_IsFlag24Clear()) {
        if (RallyData_IsHeadToHeadRaceMode()) {
            if (pMenu->items[Menu_FindItem(pMenu, 4)].max == 0)
                g_unk0x005298f8 = (g_unk0x005298f8 & ~1) | 2;
            else
                g_unk0x005298f8 &= ~3;
        } else {
            g_unk0x005298f8 ^= ((0xfe - pMenu->items[Menu_FindItem(pMenu, 4)].max) ^ g_unk0x005298f8) & 3;
        }
    }

    if (RallyData_IsHeadToHeadRaceMode()) {
        if (pMenu->items[Menu_FindItem(pMenu, 2)].max == 0)
            g_unk0x005298f8 &= ~0x10;
        else
            g_unk0x005298f8 |= 0x10;
    } else {
        if (pMenu->items[Menu_FindItem(pMenu, 2)].max == 0)
            g_unk0x005298f8 &= ~0x10;
        else
            g_unk0x005298f8 |= 0x10;
    }
    RallyData_SetDriverHudFlags(StageUI_GetRaceEndEventCount() + g_unk0x0052af58[1], &g_unk0x005298f8);
}

// Callback 1 of the car setup menu: stores the setting byte (or the other
// one when cancelling) and applies the switches.
// FUNCTION: CMR2 0x00404c50
void InRaceMenu_LeaveCarSetup(Menu *pMenu, char cancel)
{
    if (cancel != 0) {
        RallyData_SetDriverHudFlags(StageUI_GetRaceEndEventCount() + g_unk0x0052af58[1], &g_unk0x0052a850);
        return;
    }
    RallyData_SetDriverHudFlags(StageUI_GetRaceEndEventCount() + g_unk0x0052af58[1], &g_unk0x005298f8);
    CGameInfo::SetSoundOptionBit26((g_unk0x005298f8 >> 2) & 1);
    CGameInfo::SetSoundOptionBit27((g_unk0x005298f8 >> 3) & 1);
    CGameInfo::SetSoundOptionBit28((g_unk0x005298f8 >> 4) & 1);
    CGameInfo::SetSoundOptionBit29((g_unk0x005298f8 >> 5) & 1);
    CGameInfo::SetSoundOptionBits24To25(g_unk0x005298f8 & 3);
    RallyData_MarkDriverRecordDirty((StageUI_GetRaceEndEventCount() & 0xff) + g_unk0x0052af58[1]);
}

// Callback 2 of the network options menu.
// FUNCTION: CMR2 0x00403880
void InRaceMenu_UpdateCameraOffsets(Menu *pMenu)
{
    InRaceMenu_ReadCameraOffsetSliders(pMenu);
}

void NetRace_InitializeRaceSoundVolumes(void);

// Callback 1 of the sound options menu: applies the volumes (or restores the
// ones the menu started with when cancelled).
// match 79%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00401420
void InRaceMenu_LeaveSoundOptions(Menu *pMenu, char cancel)
{
    if (cancel != 0) {
        CGameInfo::SetMasterSoundVolume(g_unk0x00529ecc);
        CGameInfo::SetEffectsSoundVolume(g_unk0x0052a488);
        CGameInfo::SetCoDriverSoundVolume(g_unk0x0052ad50);
    } else {
        CGameInfo::SetMasterSoundVolume(pMenu->items[Menu_FindItem(pMenu, 0)].max * 10);
        CGameInfo::SetEffectsSoundVolume(pMenu->items[Menu_FindItem(pMenu, 1)].max * 10);
        if (Race_IsFlag24Clear())
            CGameInfo::SetCoDriverSoundVolume(pMenu->items[Menu_FindItem(pMenu, 2)].max * 10);
    }
    CInput::SetMenuSoundVolume((int)(CGameInfo::GetEffectsSoundVolume() << 16) / 100 / 4);
    NetRace_InitializeRaceSoundVolumes();
}

BYTE *RallyData_GetDriverHudFlags(BYTE index);

// Callback 0 of the car setup menu: loads the switches from the driver's
// setting byte (keeping a copy to restore on cancel).
// FUNCTION: CMR2 0x00404b80
void InRaceMenu_EnterCarSetup(Menu *pMenu, int param)
{
    g_unk0x0052a850 = *RallyData_GetDriverHudFlags(StageUI_GetRaceEndEventCount() + g_unk0x0052af58[1]);
    g_unk0x005298f8 = *RallyData_GetDriverHudFlags(StageUI_GetRaceEndEventCount() + g_unk0x0052af58[1]);
    pMenu->items[0].max = (g_unk0x005298f8 >> 2) & 1;
    pMenu->items[1].max = (g_unk0x005298f8 >> 3) & 1;
    pMenu->items[2].max = (g_unk0x005298f8 >> 4) & 1;
    pMenu->items[3].max = (g_unk0x005298f8 >> 5) & 1;
    if (Race_IsFlag24Clear()) {
        if (RallyData_IsHeadToHeadRaceMode()) {
            if ((g_unk0x005298f8 & 3) == 2) {
                pMenu->items[Menu_FindItem(pMenu, 4)].max = 0;
                return;
            }
            pMenu->items[Menu_FindItem(pMenu, 4)].max = 1;
            return;
        }
        pMenu->items[Menu_FindItem(pMenu, 4)].max = 2 - (g_unk0x005298f8 & 3);
    }
}

// Chase camera offset, lag and pitch the camera menu started with (restored on
// cancel).
// GLOBAL: CMR2 0x0052af60
FixVector g_cameraMenuStartOffset;
// GLOBAL: CMR2 0x00529914
int g_cameraMenuStartLag;
// GLOBAL: CMR2 0x0052a48c
short g_cameraMenuStartPitch;

void ChaseCamera_SetOffset(BYTE index, FixVector *pOffset);
void ChaseCamera_SetPitch(BYTE index, short value);
void ChaseCamera_SetLag(BYTE index, int value);
void RallyData_SetDriverCameraOffsets(FixVector *pOffset, short pitch, int lag, int index);

// Callback 1 of the camera options menu: applies the chosen offset (and
// stores it for the driver), or restores the old one when cancelled.
// FUNCTION: CMR2 0x004037c0
void InRaceMenu_LeaveCameraOffsetOptions(Menu *pMenu, char cancel)
{
    if (cancel != 0) {
        ChaseCamera_SetOffset(g_unk0x0052af58[1], &g_cameraMenuStartOffset);
        ChaseCamera_SetLag(g_unk0x0052af58[1], g_cameraMenuStartLag);
        ChaseCamera_SetPitch(g_unk0x0052af58[1], g_cameraMenuStartPitch);
        return;
    }
    ChaseCamera_SetLag(g_unk0x0052af58[1], g_cameraMenuLag);
    ChaseCamera_SetOffset(g_unk0x0052af58[1], &g_cameraMenuOffset);
    ChaseCamera_SetPitch(g_unk0x0052af58[1], g_cameraMenuPitch);
    CGameInfo::SetDefaultCameraParameters(&g_cameraMenuOffset, g_cameraMenuPitch, g_cameraMenuLag);
    int id = (StageUI_GetRaceEndEventCount() & 0xff) + g_unk0x0052af58[1];

    RallyData_SetDriverCameraOffsets(&g_cameraMenuOffset, g_cameraMenuPitch, g_cameraMenuLag, id);
}

// FUNCTION: CMR2 0x00404ea0
void InRaceMenu_Open(BYTE param1)
{
    g_unk0x0052af58[0] = param1;
    CGameInfo::m_unk0x0052af40 = 1;
    g_unk0x0052ad54 = 0;
    CGameInfo::SetInputAndGamePaused(1);
    g_inRaceMenuOpening = 1;
    g_pMenu0x0052af44 = NULL;
    g_pMenu0x0052af48 = &g_menu0x0052ad60;
    Menu_QueueDefaultAction();
}

// FUNCTION: CMR2 0x00404f10
void InRaceMenu_ClearOpeningFlag(void)
{
    g_inRaceMenuOpening = 0;
}

// FUNCTION: CMR2 0x00404f20
unsigned char CGameInfo::IsInRaceMenuOpen(void)
{
    return m_unk0x0052af40;
}

// FUNCTION: CMR2 0x00404f30
BYTE InRaceMenu_IsOpening(void)
{
    return g_inRaceMenuOpening;
}

// Chase camera state of the in-race step (0x404f40) while the camera menu is
// up: the player's current settings and their distance to the menu's, of
// which a tenth is applied each frame.
// GLOBAL: CMR2 0x00529ec8
int g_cameraMenuHeightStep;
// GLOBAL: CMR2 0x00529cd8
FixVector g_cameraMenuCurrentOffset;
// GLOBAL: CMR2 0x00529ed0
int g_cameraMenuCurrentLag;
// GLOBAL: CMR2 0x0052a0bc
int g_cameraMenuDistanceStep;
// GLOBAL: CMR2 0x0052a2a0
short g_cameraMenuCurrentPitch;
// GLOBAL: CMR2 0x0052aa64
int g_cameraMenuLagStep;
// GLOBAL: CMR2 0x0052aa6c
short g_cameraMenuPitchStep;
extern int g_unk0x00537f34[2];

extern BYTE g_unk0x0053811e;
extern BYTE g_unk0x0053811f;
extern BYTE g_unk0x00538120;
extern BYTE g_unk0x00538100;
extern int g_unk0x00537f30;
extern int g_netMenuFadeColour;

void NetPlayers_UpdateBestRaceTime(unsigned int time);
void NetPlayers_SendCarClass(BYTE carClass);
unsigned int NetPlayers_IsPlayerPresent(int index);
unsigned int NetPlayers_HasPlayerFinished(int index);
void View_SnapCameras(BYTE index);
void View_BlendCameraStates(BYTE index, int param);
void View_SwitchCamera(BYTE index, int a, int b, BYTE c, int d);
void ForceFeedback_StopSlotForces(void);
int StageTiming_EstimateElapsedStageTime(void);
int StageTiming_GetValidStartTime(int index);
void NetRace_SendStageAndOverallTimes(int time);
void NetRace_UpdatePlayerFlashTimer(unsigned int player, int check);
void NetRace_SetPlayerFlashIntensity(unsigned int player, int t, int check);
short Car_GetOrderCount(void);
int Race_ReadState37F60(void);
int View_GetActiveCameraMode(BYTE index);
BYTE View_GetActiveCameraFlags(BYTE index);
int ChaseCamera_GetLag(BYTE index);
void ChaseCamera_GetOffset(FixVector *pOut, BYTE index);
short ChaseCamera_GetPitch(BYTE index);
void ChaseCamera_SetOffset(BYTE index, FixVector *pOffset);
void ChaseCamera_SetPitch(BYTE index, short value);
void ChaseCamera_SetLag(BYTE index, int value);
int StageUI_GetRaceResultValue(void);
BYTE StageUI_GetRaceEndEventCount(void);
void GameMenu_FinishNetworkResultsQuitFade(BYTE index);
typedef void (*FadeCallback)(BYTE index);
void NetRace_FadeOutPlayerScreen(BYTE index, FadeCallback pfnDone, int param3, int param4, int param5, char force);
void View_SetShake(int view, int start);
void RallyData_SetDriverCarSelection(BYTE index, int value);

// PORT: the clock of the camera menu's preview.
static SysStepClock g_cameraPreviewClock;

// One frame of the in-race state machine: while the race menu is up it pushes
// the camera settings and the driver names of the active player, then updates
// the car order, the input menu and the race-start/restart transitions.
// FUNCTION: CMR2 0x00404f40
void InRaceMenu_UpdateStateMachineFrame(CallbackStateMachine *param1)
{
    int first;
    int second;
    int result;
    Menu *pNext;
    int i;
    DeviceInfo *pDev;

    ForceFeedback_StopSlotForces();
    if (g_pMenu0x0052af44 != &g_menu0x0052a870)
        goto updateMenus;

    result = View_GetActiveCameraMode(g_unk0x0052af58[1]);
    if (result == 5 || result == 8)
        goto pushCamera;

    View_SwitchCamera(g_unk0x0052af58[1], 5, 0xffff, View_GetActiveCameraFlags(g_unk0x0052af58[1]), 1);
    switch (StageUI_GetRaceResultValue()) {
    case 4:
        first = (StageUI_GetRaceEndEventCount() & 0xff) + *(BYTE *)&g_unk0x0052af58[1];
        break;
    case 0:
    case 1:
        first = g_unk0x0052af58[1];
        break;
    case 2:
        RallyData_GetRoundDrivers((unsigned int *)&first, (unsigned int *)&second);
        break;
    case 3:
        if (g_unk0x0052af58[1] == 0)
            RallyData_GetRoundDrivers((unsigned int *)&first, (unsigned int *)&second);
        else
            RallyData_GetRoundDrivers((unsigned int *)&second, (unsigned int *)&first);
        break;
    }
    RallyData_SetDriverCarSelection(first, 5);

pushCamera:
    // PORT: the original eased the camera towards the sliders, and shook it
    // on the stiffness slider, once per frame at the display's refresh;
    // without vertical sync the preview ran many times too fast. It runs
    // at 60 steps a second.
    for (i = Sys_TakeSteps(&g_cameraPreviewClock, 60, 5); i > 0; i--) {
        g_cameraMenuCurrentLag = ChaseCamera_GetLag(g_unk0x0052af58[1]);
        ChaseCamera_GetOffset(&g_cameraMenuCurrentOffset, g_unk0x0052af58[1]);
        g_cameraMenuCurrentPitch = ChaseCamera_GetPitch(g_unk0x0052af58[1]);
        g_cameraMenuLagStep = g_cameraMenuLag - g_cameraMenuCurrentLag;
        g_cameraMenuHeightStep = g_cameraMenuOffset.y - g_cameraMenuCurrentOffset.y;
        g_cameraMenuPitchStep = g_cameraMenuPitch - g_cameraMenuCurrentPitch;
        g_cameraMenuDistanceStep = g_cameraMenuOffset.z - g_cameraMenuCurrentOffset.z;
        g_cameraMenuPitch = g_cameraMenuCurrentPitch + FixMul(g_cameraMenuPitchStep, 0x1999);
        g_cameraMenuOffset.x = 0;
        g_cameraMenuOffset.y = g_cameraMenuCurrentOffset.y + FixMul(g_cameraMenuHeightStep, 0x1999);
        g_cameraMenuOffset.z = g_cameraMenuCurrentOffset.z + FixMul(g_cameraMenuDistanceStep, 0x1999);
        ChaseCamera_SetLag(g_unk0x0052af58[1], g_cameraMenuLag);
        ChaseCamera_SetOffset(g_unk0x0052af58[1], &g_cameraMenuOffset);
        ChaseCamera_SetPitch(g_unk0x0052af58[1], g_cameraMenuPitch);

        if (g_unk0x0052ad54 != 0) {
            View_SnapCameras(g_unk0x0052af58[1]);
            result = Race_ReadState37F60();
            View_BlendCameraStates(g_unk0x0052af58[1], result);
            result = g_pMenu0x0052af44 == &g_menu0x0052a870 &&
                     g_pMenu0x0052af44->cursor == 2 ? 1 : 0;
            View_SetShake(g_unk0x0052af58[1], result);
        }
    }

updateMenus:
    i = 0;
    if (Car_GetOrderCount() > 0) {
        do {
            NetRace_UpdatePlayerFlashTimer(i, 1);
            NetRace_SetPlayerFlashIntensity(i, Race_ReadState37F60(), 1);
            i++;
        } while (i < Car_GetOrderCount());
    }
    g_pMenu0x0052af44 = g_pMenu0x0052af48;
    CInput::UpdateAllAvailableDevices();
    Input_TranslatePedalsToMenuKeys();
    pDev = CInput::GetAvailableDeviceRecord(g_unk0x0052af58[0]);
    Input_MergeAssignedJoystickButtons(*(unsigned int *)&g_unk0x0052af58[0] & 0xff, pDev);
    if ((pDev->pressed & (unsigned short)g_unk0x0052a0b8) != 0) {
        g_unk0x0052a0b8 = 0;
        if (g_pMenu0x0052af44 != NULL)
            Menu_CallCallback3(g_pMenu0x0052af44);
        InRaceMenu_Close();
    } else {
        if (g_pMenu0x0052af44 != NULL)
            Menu_SetFlags(g_pMenu0x0052af44, 1, 1, 1, 1);
        pNext = Menu_Update(g_pMenu0x0052af44, pDev->pressed);
        if (pNext != NULL)
            g_pMenu0x0052af48 = pNext;
    }

    if (CGameInfo::GetGameModeOptionBit19() == 0)
        return;
    g_unk0x0053811e = 0;
    g_unk0x0053811f = 0;
    if (GameInfo_GetSessionField398C() > -1 && CGameInfo::GetConfiguredGameMode() != 0xa && CGameInfo::GetConfiguredGameMode() != 0xc) {
        if (g_unk0x0053811d != 0) {
            if ((unsigned int)(CMain::GetFrameDelta() - g_unk0x00537f30) >
                (unsigned int)(GameInfo_GetSessionField398C() * 100))
                g_unk0x0053811e = 1;
        } else {
            i = 0;
            do {
                if ((char)NetPlayers_IsPlayerPresent(i) != 0 && (char)NetPlayers_HasPlayerFinished(i) != 0) {
                    g_unk0x0053811d = 1;
                    if (g_unk0x00538120 == 0) {
                        g_unk0x00538120 = 1;
                        g_unk0x00537f30 = CMain::GetFrameDelta();
                    }
                }
                i++;
            } while (i < 7);
        }
    }
    if (CGameInfo::GetConfiguredGameMode() == 0xa || CGameInfo::GetConfiguredGameMode() == 0xc) {
        CGameInfo::GetConfiguredGameMode();
        i = CMain::GetFrameDelta();
        result = NetPlayers_GetInputFrameDelta();
        if ((unsigned int)(i - result) >
                (unsigned int)(GameInfo_GetSessionField397C() * 6000) &&
            Network_GetSessionStateFlag() != 0) {
            if (g_unk0x00538100 != 0)
                goto playerExit;
            g_unk0x0053811f = 1;
            NetPlayers_SendType16Notification();
            g_unk0x00538100 = 1;
            NetRace_FadeOutPlayerScreen(0, GameMenu_FinishNetworkResultsQuitFade, 1, 0, g_netMenuFadeColour, 1);
        }
        if (g_unk0x00538100 != 0) {
playerExit:
            Race_UpdatePlayerViewAndDrivenCars(*(BYTE *)param1->records, 1);
            return;
        }
    }
    if (g_unk0x0053811e != 0 || g_unk0x0053811f != 0) {
        if (CGameInfo::GetGameModeOptionBit19() != 0)
            NetPlayers_SendCarClass(3);
        g_unk0x00537f34[0] = CMain::GetFrameDelta();
    }
    CGameInfo::GetConfiguredGameMode();
    Race_UpdatePlayerViewAndDrivenCars(*(BYTE *)param1->records, 0);
    if (g_unk0x0053811e != 0) {
        View_SwitchCamera(0, 7, 0xffff, View_GetActiveCameraFlags(0), 0);
        if (g_unk0x0053811f == 0) {
            if (g_unk0x0053811e != 0)
                result = StageTiming_EstimateElapsedStageTime();
            else
                result = StageTiming_GetValidStartTime(0);
            NetRace_SendStageAndOverallTimes(result);
            if (g_unk0x00538120 == 0 && GameInfo_GetSessionField398C() > -1) {
                g_unk0x00538120 = 1;
                g_unk0x00537f30 = CMain::GetFrameDelta();
            }
            result = StageTiming_GetValidStartTime(0);
            NetPlayers_UpdateBestRaceTime(result);
        }
        if (g_unk0x0053811e != 0) {
            InRaceMenu_Close();
            CGame::PromoteCallbackEntryByRule(param1, 0, 0, 2);
        }
    }
}

void RallyData_GrowEntryAndNotifyCompletion(CallbackStateMachine *, unsigned int);

// Advances every entry of a menu screen one step.
// FUNCTION: CMR2 0x00405470
void InRaceMenu_AdvanceScreenEntries(CallbackStateMachine *param1)
{
    BYTE i;

    i = 0;
    if (param1->count > 0) {
        do {
            // the original passes the byte counter as a full dword (the callee
            // keeps only its low byte)
            RallyData_GrowEntryAndNotifyCompletion(param1, *(unsigned int *)&i);
            i++;
        } while (i < param1->count);
    }
}

void MenuScreen_BeginOverlay(void);
void MenuScreen_EndOverlay(void);

// FUNCTION: CMR2 0x004054a0
void InRaceMenu_DrawActivePage(void)
{
    // PORT: laid out for the screen's size, as the other menus.
    MenuScreen_BeginOverlay();
    Menu_CallCallback2(g_pMenu0x0052af44);
    MenuScreen_EndOverlay();
}

// FUNCTION: CMR2 0x004054b0
int InRaceMenu_IsPlayerEditingCarSetup(unsigned int param1)
{
    if (CGameInfo::IsInRaceMenuOpen() && g_unk0x0052af58[1] == param1 && g_pMenu0x0052af44 == &g_menu0x0052aa70)
        return 1;
    return 0;
}

// FUNCTION: CMR2 0x004055e0
Texture *InRaceMenu_GetUpArrowTexture(void)
{
    return g_inRaceMenuUpArrowTexture;
}

// FUNCTION: CMR2 0x004055f0
Texture *InRaceMenu_GetDownArrowTexture(void)
{
    return g_inRaceMenuDownArrowTexture;
}

// FUNCTION: CMR2 0x00405600
Texture *InRaceMenu_GetRoundBoxTexture(void)
{
    return g_inRaceMenuRoundBoxTexture;
}

// FUNCTION: CMR2 0x00405610
Texture *InRaceMenu_GetCurtainTexture(void)
{
    return g_inRaceMenuCurtainTexture;
}

// Text buffer for the truncated draw below (256 bytes up to the next global).
// GLOBAL: CMR2 0x0082b1c0
char g_truncatedTextBuffer[0x100];

// Draws the record's text; in mode 1 it truncates the string at the 16.16
// fraction of its length and draws the remainder separately.
// FUNCTION: CMR2 0x00501f80
void OptionMenu_DrawTransitionTextShortCoords(int index, int font1, int font2, char *text, short x, short y,
                  int *pColour1, int *pColour2, unsigned int flags)
{
    Unk0x0082b2c0 *pRec;
    int len;
    int count;
    int width;
    int i;

    pRec = &g_optionRecordTransitions[index];
    if (pRec->field_0xc == 2) {
        Font_DrawText(font1, text, x, y, pColour1, flags);
        return;
    }
    if (pRec->field_0xc == 1) {
        len = (int)strlen(text);
        count = FixMulShift32(len << 16, pRec->field_0x0);
        for (i = 0; i < count; i++)
            g_truncatedTextBuffer[i] = text[i];
        g_truncatedTextBuffer[count] = 0;
        Font_DrawText(font1, g_truncatedTextBuffer, x, y, pColour1, flags);
        if ((flags == 9 || flags == 0x11 || flags == 0x21) && count < len) {
            width = Font_GetTextWidth(font1, (BYTE *)g_truncatedTextBuffer);
            g_truncatedTextBuffer[0] = text[count];
            g_truncatedTextBuffer[1] = 0;
            Font_DrawText(font2, g_truncatedTextBuffer, width + x, y, pColour2, flags);
        }
    }
}

#include "Sprite.h"
#include "Graphics.h"

// Draws the four one pixel edges of a rectangle given as x, y, w, h.
// FUNCTION: CMR2 0x00403ef0
void DrawRectOutline(short *pRect, BYTE *pColour)
{
    short edge[4];

    edge[0] = pRect[0];
    edge[1] = pRect[1];
    edge[2] = pRect[2];
    edge[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, edge, pColour, 2);
    edge[0] = pRect[2] + pRect[0];
    edge[1] = pRect[1];
    edge[2] = 1;
    edge[3] = pRect[3] + 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, edge, pColour, 2);
    edge[0] = pRect[0];
    edge[1] = pRect[3] + pRect[1];
    edge[2] = pRect[2];
    edge[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, edge, pColour, 2);
    edge[0] = pRect[0];
    edge[1] = pRect[1];
    edge[2] = 1;
    edge[3] = pRect[3];
    Sprite_FillRect(&g_pGraphics->field309_0x150, edge, pColour, 2);
}

void View_SetShake(int view, int start);
extern BYTE g_unk0x0052af58[2];

// ---------------------------------------------------------------------------
// In-race network menu: the drawing helpers and the menu constructors.

// Colours the in-race menu text and the selected/unselected sprites use.
// GLOBAL: CMR2 0x00516074
int g_inRaceMenuSelectedColour = 0xfffafafa;
// GLOBAL: CMR2 0x00516078
int g_inRaceMenuUnselectedColour = 0xffdbaca7;
// GLOBAL: CMR2 0x00516084
int g_unk0x00516084 = 0x80fafafa;
// GLOBAL: CMR2 0x0051608c
int g_unk0x0051608c = 0xbfae8072;

void GameMenu_RequestStageRestart(Menu *pMenu, int param);
void GameMenu_RequestNetworkRestart(Menu *pMenu, int param);
void GameMenu_RequestNetworkQuit(Menu *pMenu, int param);
void GameMenu_RequestNetworkLeave(Menu *pMenu, int param);
void InRaceMenu_DrawMenuRows(Menu *pMenu);
void InRaceMenu_DrawPlayerClassRows(Menu *pMenu);
void InRaceMenu_DrawNetworkPlayerList(Menu *pMenu);
// Scratch rectangle the in-race menu draws reuse (0x52ad58, right below the main
// menu at 0x52ad60).
// GLOBAL: CMR2 0x0052ad58
short g_inRaceMenuScratchRect[4];
extern char g_classRowHeaderFormat[];
void OptionMenu_ConsumeRefreshRequest(int unused, int unused2);
void OptionMenu_UpdateMessagesAndRecordStates(Menu *);
void OptionMenu_LoadGameItem(Menu *, MenuItem *);
void OptionMenu_ApplyHighlightedValue(Menu *, MenuItem *);
void OptionMenu_AdvanceSelectedOption(int unused, int unused2);
void OptionMenu_ResetValueAnimation(int param1, int param2);
void OptionMenu_RestartCountdown(int unused, int unused2);
void OptionMenu_RestartEmptyItemRecords(Menu *pMenu, MenuItem *unused);
void OptionMenu_IncrementSharedValue(Menu *pMenu, char param);
void OptionMenu_BuildStatusMenu(void);
void OptionMenu_BuildAdvancedOptions(void);
void OptionMenu_BuildControlSetup(void);
void OptionMenu_BuildValueSliders(void);
void OptionMenu_LoadPartTextures(void);
void OptionPreview_UpdateAllSlots(void);
void OptionMenu_DispatchRecordEditState(Menu *);
void OptionMenu_RebuildOptions(void);

// Draws the header of the in-race menu bar: the car count above the bar and
// the bar itself.
// FUNCTION: CMR2 0x00401b60
void InRaceMenu_DrawHeaderBar(void)
{
    short rect[4];
    int x;
    int y;
    int count;

    x = (int)(g_pGraphics->resX * 0x1f) / 0x280;
    count = (StageUI_GetRaceEndEventCount() & 0xff) + (*(int *)g_unk0x0052af58 & 0xff) + 1;
    sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x98), count);
    Font_DrawText(2, CFrontend::m_stringDest, x, (int)(g_pGraphics->resY * 0x45) / 0x1e0,
                  &g_inRaceMenuSelectedColour, 0x11);
    y = (int)(g_pGraphics->resY * 10) / 0x1e0 + x
        + Font_GetTextWidth(2, (BYTE *)CFrontend::m_stringDest);
    rect[0] = (short)y;
    rect[2] = 1;
    rect[1] = (short)((int)(g_pGraphics->resY * 0x25) / 0x1e0);
    rect[3] = (short)((int)(g_pGraphics->resY * 0x2d) / 0x1e0);
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, (BYTE *)&g_inRaceMenuSelectedColour, 2);
    rect[0] = rect[0] + 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, (BYTE *)&g_unk0x00516084, 2);
    Font_DrawText(2, CFrontend::GetTextString(0x18), y + (int)(g_pGraphics->resY * 10) / 0x1e0,
                  (int)(g_pGraphics->resY * 0x45) / 0x1e0, &g_inRaceMenuSelectedColour, 0x11);
}

// Draws the list of an in-race menu (0x402c90) with the selected row in a
// brighter colour; the third row is left blank.
// FUNCTION: CMR2 0x00401870
void InRaceMenu_DrawOptionList(Menu *pMenu)
{
    MenuItem *pItem;
    short rect[4];
    int i;
    int y;
    char *string1;
    char *string2;

    rect[0] = 0;
    rect[1] = 0;
    rect[2] = (short)g_pGraphics->resX;
    rect[3] = (short)g_pGraphics->resY;
    Font_SetBlendMode(2);
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, (BYTE *)&g_unk0x0051608c, 2);
    InRaceMenu_DrawHeaderBar();
    rect[0] = (short)((int)(g_pGraphics->resX * 0x70) / 0x280);
    rect[2] = g_inRaceMenuUpArrowTexture->width;
    rect[3] = g_inRaceMenuUpArrowTexture->height;
    y = (int)(g_pGraphics->resY * 0xaa) / 0x1e0;
    i = 0;
    if (pMenu->itemCount > 0) {
        pItem = pMenu->items;
        do {
            rect[1] = (short)(y - (int)(g_pGraphics->resY * 0xd) / 0x1e0);
            if (i != 2) {
                string1 = pItem->text;
                if (string1 != 0) {
                    string2 = string1;
                } else {
                    string1 = CFrontend::GetTextString(pItem->id);
                    string2 = CFrontend::GetTextString(pItem->id + 1);
                }
                if (pMenu->cursor == i) {
                    Font_DrawText(1, string1, (int)(g_pGraphics->resX * 0x86) / 0x280, y,
                                  &g_inRaceMenuSelectedColour, 0x11);
                    Font_DrawText(0, string2, (int)(g_pGraphics->resX * 0x86) / 0x280,
                                  (int)(g_pGraphics->resY * 0xf) / 0x1e0 + y, &g_inRaceMenuSelectedColour, 0x11);
                    Sprite_Queue((SpriteRect *)&g_inRaceMenuUpArrowTexture->field_0x11c, (SpriteRect *)rect,
                                 g_inRaceMenuUpArrowTexture, 2, 0, NULL, NULL, (BYTE *)&g_inRaceMenuSelectedColour, 8);
                } else {
                    Font_DrawText(1, string1, (int)(g_pGraphics->resX * 0x86) / 0x280, y,
                                  &g_inRaceMenuUnselectedColour, 0x11);
                    Font_DrawText(0, string2, (int)(g_pGraphics->resX * 0x86) / 0x280,
                                  (int)(g_pGraphics->resY * 0xf) / 0x1e0 + y, &g_inRaceMenuUnselectedColour, 0x11);
                    Sprite_Queue((SpriteRect *)&g_inRaceMenuDownArrowTexture->field_0x11c, (SpriteRect *)rect,
                                 g_inRaceMenuDownArrowTexture, 2, 0, NULL, NULL, (BYTE *)&g_inRaceMenuUnselectedColour, 8);
                }
                y = y + (int)(g_pGraphics->resY * 0x36) / 0x1e0;
            }
            i++;
            pItem++;
        } while (i < pMenu->itemCount);
    }
    Font_SetBlendMode(2);
}

// Draws every row of an in-race menu (no blank row), selected row brighter.
// FUNCTION: CMR2 0x00401d20
void InRaceMenu_DrawMenuRows(Menu *pMenu)
{
    MenuItem *pItem;
    short rect[4];
    int i;
    int y;
    char *string1;
    char *string2;

    rect[0] = 0;
    rect[1] = 0;
    rect[2] = (short)g_pGraphics->resX;
    rect[3] = (short)g_pGraphics->resY;
    Font_SetBlendMode(2);
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, (BYTE *)&g_unk0x0051608c, 2);
    InRaceMenu_DrawHeaderBar();
    rect[0] = (short)((int)(g_pGraphics->resX * 0x70) / 0x280);
    rect[2] = g_inRaceMenuUpArrowTexture->width;
    rect[3] = g_inRaceMenuUpArrowTexture->height;
    y = (int)(g_pGraphics->resY * 0xaa) / 0x1e0;
    pItem = pMenu->items;
    for (i = 0; i < pMenu->itemCount; i++, pItem++) {
        rect[1] = (short)(y - (int)(g_pGraphics->resY * 0xd) / 0x1e0);
        string1 = pItem->text;
        string2 = string1;
        if (string1 == 0) {
            string1 = CFrontend::GetTextString(pItem->id);
            string2 = CFrontend::GetTextString(pItem->id + 1);
        }
        if (pMenu->cursor == i) {
            Font_DrawText(1, string1, (int)(g_pGraphics->resX * 0x86) / 0x280, y,
                          &g_inRaceMenuSelectedColour, 0x11);
            Font_DrawText(0, string2, (int)(g_pGraphics->resX * 0x86) / 0x280,
                          (int)(g_pGraphics->resY * 0xf) / 0x1e0 + y, &g_inRaceMenuSelectedColour, 0x11);
            Sprite_Queue((SpriteRect *)&g_inRaceMenuUpArrowTexture->field_0x11c, (SpriteRect *)rect,
                         g_inRaceMenuUpArrowTexture, 2, 0, NULL, NULL, (BYTE *)&g_inRaceMenuSelectedColour, 8);
        } else {
            Font_DrawText(1, string1, (int)(g_pGraphics->resX * 0x86) / 0x280, y,
                          &g_inRaceMenuUnselectedColour, 0x11);
            Font_DrawText(0, string2, (int)(g_pGraphics->resX * 0x86) / 0x280,
                          (int)(g_pGraphics->resY * 0xf) / 0x1e0 + y, &g_inRaceMenuUnselectedColour, 0x11);
            Sprite_Queue((SpriteRect *)&g_inRaceMenuDownArrowTexture->field_0x11c, (SpriteRect *)rect,
                         g_inRaceMenuDownArrowTexture, 2, 0, NULL, NULL, (BYTE *)&g_inRaceMenuUnselectedColour, 8);
        }
        y = y + (int)(g_pGraphics->resY * 0x36) / 0x1e0;
    }
    Font_SetBlendMode(2);
}

// Draws a player list of the in-race network menu: each row shows the player
// name, the secondary line and the class split entries.
// match 67%: the original reads an uninitialised local in the value != 0 path
// and keeps the item pointer in a register; kept as FUNCTION so reccmp measures it.
// FUNCTION: CMR2 0x00402460
void InRaceMenu_DrawNetworkPlayerList(Menu *pMenu)
{
    int i;
    int j;
    int x;
    char *str1;
    char *str2;
    char *sub[3];
    char *text;
    char **pA;
    char **pB;
    char single;
    bool twoLines;
    int *pColour;
    Texture *texture;

    g_inRaceMenuScratchRect[0] = 0;
    g_inRaceMenuScratchRect[1] = 0;
    g_inRaceMenuScratchRect[2] = (short)g_pGraphics->resX;
    g_inRaceMenuScratchRect[3] = (short)g_pGraphics->resY;
    Font_SetBlendMode(2);
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_inRaceMenuScratchRect, (BYTE *)&g_unk0x0051608c, 2);
    InRaceMenu_DrawHeaderBar();
    g_inRaceMenuScratchRect[0] = (short)((int)(g_pGraphics->resX * 0x70) / 0x280);
    g_inRaceMenuScratchRect[2] = g_inRaceMenuUpArrowTexture->width;
    g_inRaceMenuScratchRect[3] = g_inRaceMenuUpArrowTexture->height;
    for (i = 0; i < pMenu->itemCount; i++) {
        MenuItem *pItem = &pMenu->items[i];
        if (pItem->visible) {
            g_inRaceMenuScratchRect[1] = (short)((int)(g_pGraphics->resY * 0xaa) / 0x1e0
                                         + ((int)(g_pGraphics->resY * 0x36) / 0x1e0) * i
                                         - (int)(g_pGraphics->resY * 0xd) / 0x1e0);
            str1 = CFrontend::GetTextString(pItem->id);
            str2 = CFrontend::GetTextString(pItem->id + 1);
            if (pMenu->cursor == i) {
                pColour = &g_inRaceMenuSelectedColour;
                texture = g_inRaceMenuUpArrowTexture;
            } else {
                pColour = &g_inRaceMenuUnselectedColour;
                texture = g_inRaceMenuDownArrowTexture;
            }
            Sprite_Queue((SpriteRect *)&texture->field_0x11c, (SpriteRect *)g_inRaceMenuScratchRect,
                         texture, 2, 0, NULL, NULL, (BYTE *)&g_inRaceMenuUnselectedColour, 8);
            twoLines = true;
            if (pItem->value != 0) {
                sub[0] = CFrontend::GetTextString(pItem->id);
                twoLines = false;
            } else {
                for (j = 0; j < (int)(BYTE)pItem->min; j++)
                    sub[j] = CFrontend::GetTextString(j + 0x9c);
                single = 0;
            }
            strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pItem->id));
            x = (int)(g_pGraphics->resX * 0x86) / 0x280;
            Font_DrawText(1, str1, x,
                          (int)(g_pGraphics->resY * 0xaa) / 0x1e0
                              + ((int)(g_pGraphics->resY * 0x36) / 0x1e0) * i,
                          pColour, 0x11);
            if (twoLines)
                Font_DrawText(0, str2, x,
                              (int)(g_pGraphics->resY * 0xf) / 0x1e0
                                  + (int)(g_pGraphics->resY * 0xaa) / 0x1e0
                                  + ((int)(g_pGraphics->resY * 0x36) / 0x1e0) * i,
                              pColour, 0x11);
            pB = sub + 1;
            pA = sub;
            for (j = 0; j < (int)(BYTE)pItem->min; j++) {
                text = single == 0 ? *pA : *pB;
                x = (int)(g_pGraphics->resX * 10) / 0x280 + x
                    + Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
                if (pItem->max != j)
                    Font_DrawText(1, text, x,
                                  (int)(g_pGraphics->resY * 0xaa) / 0x1e0
                                      + ((int)(g_pGraphics->resY * 0x36) / 0x1e0) * i, &g_inRaceMenuUnselectedColour, 0x11);
                else
                    Font_DrawText(1, text, x,
                                  (int)(g_pGraphics->resY * 0xaa) / 0x1e0
                                      + ((int)(g_pGraphics->resY * 0x36) / 0x1e0) * i, &g_inRaceMenuSelectedColour, 0x11);
                strcpy(CFrontend::m_stringDest, text);
                pB--;
                pA++;
            }
        }
    }
    Font_SetBlendMode(2);
}

// Draws the player/class rows of the option menu: the class header (name plus
// its percentage) and the sprite behind each row.
// FUNCTION: CMR2 0x004028d0
void InRaceMenu_DrawPlayerClassRows(Menu *pMenu)
{
    MenuItem *pItem;
    short rect[4];
    int i;
    int *pColour;
    int type;

    rect[0] = 0;
    rect[1] = 0;
    rect[2] = (short)g_pGraphics->resX;
    rect[3] = (short)g_pGraphics->resY;
    Font_SetBlendMode(2);
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, (BYTE *)&g_unk0x0051608c, 2);
    InRaceMenu_DrawHeaderBar();
    rect[0] = (short)((int)(g_pGraphics->resX * 0x70) / 0x280);
    rect[2] = g_inRaceMenuUpArrowTexture->width;
    rect[3] = g_inRaceMenuUpArrowTexture->height;
    pItem = pMenu->items;
    for (i = 0; i < pMenu->itemCount; i++, pItem++) {
        pColour = pMenu->cursor == i ? &g_inRaceMenuSelectedColour : &g_inRaceMenuUnselectedColour;
        type = pItem->value;
        if (type >= 0) {
            if (type > 2) {
                if (type == 4) {
                    Font_DrawText(1, CFrontend::GetTextString(pItem->id),
                                  (int)(g_pGraphics->resX * 0x86) / 0x280,
                                  (int)(g_pGraphics->resY * 0xaa) / 0x1e0
                                      + ((int)(g_pGraphics->resY * 0x36) / 0x1e0) * i,
                                  pColour, 0x11);
                }
            } else {
                sprintf(CFrontend::m_stringDest, g_classRowHeaderFormat,
                        CFrontend::GetTextString(pItem->id), pItem->max * 10);
                Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x86) / 0x280,
                              (int)(g_pGraphics->resY * 0xaa) / 0x1e0
                                  + ((int)(g_pGraphics->resY * 0x36) / 0x1e0) * i,
                              pColour, 0x11);
            }
        }
        rect[1] = (short)(((int)(g_pGraphics->resY * 0xaa) / 0x1e0
                           + ((int)(g_pGraphics->resY * 0x36) / 0x1e0) * i)
                          - (int)(g_pGraphics->resY * 0xd) / 0x1e0);
        if (pMenu->cursor == i) {
            Sprite_Queue((SpriteRect *)&g_inRaceMenuUpArrowTexture->field_0x11c, (SpriteRect *)rect,
                         g_inRaceMenuUpArrowTexture, 2, 0, NULL, NULL, (BYTE *)&g_inRaceMenuSelectedColour, 8);
        } else {
            Sprite_Queue((SpriteRect *)&g_inRaceMenuDownArrowTexture->field_0x11c, (SpriteRect *)rect,
                         g_inRaceMenuDownArrowTexture, 2, 0, NULL, NULL, (BYTE *)&g_inRaceMenuUnselectedColour, 8);
        }
    }
    Font_SetBlendMode(2);
}

// Rebuilds the main in-race menu. When param is non-zero it keeps the value of
// the item currently under the cursor and re-selects it.
// FUNCTION: CMR2 0x00402c90
void InRaceMenu_RebuildMainPage(int param)
{
    int value;
    int index;
    short id;

    InRaceMenu_BuildResumePage();
    g_unk0x0052a0b8 = 0;
    value = param;
    if ((BYTE)param != 0)
        value = (int)g_menu0x0052ad60.items[g_menu0x0052ad60.cursor].value;
    Menu_Init(&g_menu0x0052ad60, 0, 0, 0, NULL, NULL, 1, 0, 1);
    if ((char)RallyDataState() == 1)
        Menu_AddItemType2(&g_menu0x0052ad60, 0, 9, &g_menu0x00529ed8, 0, 0);
    else
        Menu_AddItemType2(&g_menu0x0052ad60, 0, 9, &g_menu0x00529918, 0, 0);
    Menu_AddItemType2(&g_menu0x0052ad60, 0, 0xb, &g_menu0x0052a670, 0, 1);
    Menu_AddItemType2(&g_menu0x0052ad60, 0, 0xd, &g_menu0x0052a490, 0, 2);
    if (CGameInfo::GetGameModeOptionBit19() != 0) {
        if ((Network_GetSessionStateFlag() != 0 && CGameInfo::GetConfiguredGameMode() != 0xc) ||
            (Network_GetSessionStateFlag() == 0 && CGameInfo::GetConfiguredGameMode() == 0xa))
            Menu_AddItemType2(&g_menu0x0052ad60, 0, 0xf, &g_menu0x00529af8, 0, 3);
        if (Network_GetSessionStateFlag() != 0)
            Menu_AddItemType2(&g_menu0x0052ad60, 0, 0xf6, &g_menu0x0052a2a8, 0, 5);
    } else {
        Menu_AddItemType2(&g_menu0x0052ad60, 0, 0xf, &g_menu0x00529af8, 0, 3);
    }
    Menu_AddItemType2(&g_menu0x0052ad60, 0, 0x11, &g_menu0x0052a0c0, 0, 6);
    Menu_AddItemType4(&g_menu0x0052ad60, 0, 0x13, (MenuItemAction)InRaceMenu_ResumeWithSound, 7);
    Menu_SetCallbacks(&g_menu0x0052ad60, (MenuCallback)InRaceMenu_RememberOptionCursor, (MenuCallback)InRaceMenu_SkipBlankOptionRow,
                      (MenuCallback)InRaceMenu_DrawOptionList, NULL);
    if ((BYTE)param != 0)
        Menu_SelectItem(&g_menu0x0052ad60, value);
    else
        Menu_ValidateCursor(&g_menu0x0052ad60, 0);
    switch (CGameInfo::GetConfiguredGameMode()) {
    case 0:
    case 1:
    case 8:
        id = 0x25;
        break;
    case 2:
    case 3:
    case 9:
    case 10:
        id = 0x23;
        break;
    case 4:
        id = 0x27;
        break;
    case 5:
        id = 0x2b;
        break;
    case 6:
    case 7:
    case 0xb:
    case 0xc:
        id = 0x29;
        break;
    default:
        id = (short)param;
        break;
    }
    index = Menu_FindItem(&g_menu0x0052ad60, 3);
    g_menu0x0052ad60.items[index].id = id;
    index = Menu_FindItem(&g_menu0x00529af8, 3);
    g_menu0x00529af8.items[index].id = id;
}

// Builds the camera options submenu of the in-race menu.
// FUNCTION: CMR2 0x00403090
void InRaceMenu_BuildCameraModeOptions(void)
{
    Menu_Init(&g_menu0x0052a490, 0, 0, 0, &g_menu0x0052ad60, NULL, 1, 0, 1);
    Menu_AddItemType3(&g_menu0x0052a490, 0, 0x3c, 2, 0, 0, 0, 0, 0);;
    Menu_AddItemType4(&g_menu0x0052a490, 0, 0x3b, (MenuItemAction)InRaceMenu_ReturnToPauseItem, -1);
    Menu_SetCallbacks(&g_menu0x0052a490, (MenuCallback)CGame::NoOpSecondaryStateCallback,
                      (MenuCallback)InRaceMenu_ReturnToPauseUpdate, (MenuCallback)InRaceMenu_DrawNetworkPlayerList, NULL);
    Menu_ValidateCursor(&g_menu0x0052a490, 0);
}

// Builds the sound options submenu (the three volume sliders).
// FUNCTION: CMR2 0x004032a0
void InRaceMenu_BuildSoundOptions(void)
{
    Menu_Init(&g_menu0x0052a670, 0, 0, 0, &g_menu0x0052ad60, NULL, 1, 0, 1);
    Menu_AddItemType3(&g_menu0x0052a670, 0, 0x19, 0xb, 10, 0, 0, 0, 0);;
    Menu_AddItemType3(&g_menu0x0052a670, 0, 0x1a, 0xb, 10, 0, 0, 0, 1);;
    if (Race_IsFlag24Clear())
        Menu_AddItemType3(&g_menu0x0052a670, 0, 0x1b, 0xb, 10, 0, 0, 0, 2);;
    Menu_AddItemType2(&g_menu0x0052a670, 0, 0x3b, &g_menu0x0052ad60, 0, 4);
    Menu_SetCallbacks(&g_menu0x0052a670, (MenuCallback)InRaceMenu_EnterSoundOptions, (MenuCallback)InRaceMenu_ApplySoundVolumes,
                      (MenuCallback)InRaceMenu_DrawPlayerClassRows, (MenuCallback)InRaceMenu_LeaveSoundOptions);
    Menu_ValidateCursor(&g_menu0x0052a670, 0);
}

// Builds the in-race pause menu.
// FUNCTION: CMR2 0x00403370
void InRaceMenu_BuildPausePage(void)
{
    Menu_Init(&g_menu0x00529af8, 0, 0, 0, &g_menu0x0052ad60, NULL, 1, 0, 1);
    Menu_AddItemType4(&g_menu0x00529af8, 0, 0xf, (MenuItemAction)GameMenu_RequestStageRestart, 3);
    if (CGameInfo::GetConfiguredGameMode() == 2 || CGameInfo::GetConfiguredGameMode() == 3 ||
        CGameInfo::GetConfiguredGameMode() == 0xa || CGameInfo::GetConfiguredGameMode() == 9)
        Menu_AddItemType4(&g_menu0x00529af8, 0, 0x8b, (MenuItemAction)GameMenu_RequestNetworkRestart, 4);
    Menu_AddItemType1(&g_menu0x00529af8, 0, 0xa6, 0, -1);
    Menu_SetCallbacks(&g_menu0x00529af8, (MenuCallback)InRaceMenu_SelectLastItem, NULL, (MenuCallback)InRaceMenu_DrawMenuRows,
                      NULL);
    Menu_ValidateCursor(&g_menu0x00529af8, 0);
}

// Builds the "quit" submenu of the pause menu.
// FUNCTION: CMR2 0x00403420
void InRaceMenu_BuildQuitPage(void)
{
    Menu_Init(&g_menu0x0052a2a8, 0, 0, 0, &g_menu0x0052ad60, NULL, 1, 0, 1);
    Menu_AddItemType4(&g_menu0x0052a2a8, 0, 0xf6, (MenuItemAction)GameMenu_RequestNetworkLeave, 3);
    Menu_AddItemType1(&g_menu0x0052a2a8, 0, 0xa6, 0, -1);
    Menu_SetCallbacks(&g_menu0x0052a2a8, (MenuCallback)InRaceMenu_SelectLastItem, NULL, (MenuCallback)InRaceMenu_DrawMenuRows,
                      NULL);
    Menu_ValidateCursor(&g_menu0x0052a2a8, 0);
}

// Builds the "leave" submenu of the pause menu.
// FUNCTION: CMR2 0x00403490
void InRaceMenu_BuildLeavePage(void)
{
    Menu_Init(&g_menu0x0052a0c0, 0, 0, 0, &g_menu0x0052ad60, NULL, 1, 0, 1);
    Menu_AddItemType4(&g_menu0x0052a0c0, 0, 0xa8, (MenuItemAction)GameMenu_RequestNetworkQuit, 3);
    Menu_AddItemType1(&g_menu0x0052a0c0, 0, 0xaa, 0, -1);
    Menu_SetCallbacks(&g_menu0x0052a0c0, (MenuCallback)InRaceMenu_SelectLastItem, NULL, (MenuCallback)InRaceMenu_DrawMenuRows,
                      NULL);
    Menu_ValidateCursor(&g_menu0x0052a0c0, 0);
}

void InRaceMenu_BuildPlayerOptions(void);

// Resets the pause-menu state and rebuilds all of its menus.
// FUNCTION: CMR2 0x00403500
void InRaceMenu_ResetAndBuildPages(void)
{
    CGameInfo::m_unk0x0052af40 = 0;
    g_unk0x0052af58[0] = 0;
    g_unk0x0052af58[1] = 0;
    InRaceMenu_BuildPausePage();
    InRaceMenu_BuildQuitPage();
    InRaceMenu_BuildLeavePage();
    InRaceMenu_RebuildMainPage(0);
    InRaceMenu_BuildNetworkOptions();
    InRaceMenu_BuildPlayerOptions();
    InRaceMenu_BuildCameraOffsetOptions();
    InRaceMenu_BuildCarSetup();
    InRaceMenu_BuildSoundOptions();
    InRaceMenu_BuildCameraModeOptions();
}

// Builds the options submenu of the pause menu.
// FUNCTION: CMR2 0x00403550
void InRaceMenu_BuildPlayerOptions(void)
{
    Menu_Init(&g_menu0x00529918, 0, 0, 0, &g_menu0x0052ad60, NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x00529918, 0, 0x58, &g_menu0x00529ed8, (MenuItemAction)InRaceMenu_SelectPlayer, 0);
    Menu_AddItemType2(&g_menu0x00529918, 0, 0x5a, &g_menu0x00529ed8, (MenuItemAction)InRaceMenu_SelectPlayer, 1);
    Menu_SetCallbacks(&g_menu0x00529918, NULL, NULL, (MenuCallback)InRaceMenu_DrawOptionList, NULL);
    Menu_ValidateCursor(&g_menu0x00529918, 0);
}

// Printed when the weather/damage/car setup was re-initialised.
// GLOBAL: CMR2 0x00527004
char g_strNeedReinitWeather[] = "Need to re-initialise weather, damage and car setup\n";

void RallyData_ApplyCountryCarSetups(void);
BYTE RallyData_GetSelectionStateByte(void);
void RallyData_SetSelectionStateByte(char param1);
void RallyData_ClearDriverGroupRecords(void);
void RallyData_PickOpponentLineups(void);

// Re-initialises the weather/damage/car setup when the country or stage index
// changed, unless the race is already running.
// FUNCTION: CMR2 0x00500c00
void OptionMenu_RefreshStageSetup(void)
{
    short b;
    short c;

    b = (BYTE)RallyDataCountryIndex();
    c = (char)RallyData_GetSelectionStateByte();
    if (b != c) {
        puts(g_strNeedReinitWeather);
        if (CGameInfo::GetConfiguredGameMode() != 2 && CGameInfo::GetConfiguredGameMode() != 8 &&
            CGameInfo::GetConfiguredGameMode() != 9 && CGameInfo::GetConfiguredGameMode() != 0xa &&
            CGameInfo::GetConfiguredGameMode() != 0xb && CGameInfo::GetConfiguredGameMode() != 0xc)
            RallyData_PickOpponentLineups();
        RallyData_ApplyCountryCarSetups();
        RallyData_ClearDriverGroupRecords();
        RallyData_SetSelectionStateByte((char)b);
    }
}

// Rebuilds the options menu (record list plus the owner's help text).
// FUNCTION: CMR2 0x00502310
void OptionMenu_RebuildOptions(void)
{
    Menu_Init(&g_controlSetupMenu, 0, -1, 0, &g_controlSetupMenu, NULL, 1, 0, 0);
    Menu_AddItemType3(&g_controlSetupMenu, 0, 0x48, (BYTE)CGameInfo::GetPreviewLayoutMode(), 0, 0, 0,
                      (MenuItemAction)OptionMenu_ResetValueAnimation, 0);;
    Menu_AddItemType3(&g_controlSetupMenu, 0, 0x47, 7, 0, 1, 0, (MenuItemAction)OptionMenu_AdvanceSelectedOption, 1);;
    Menu_AddItemType3(&g_controlSetupMenu, 0, 0x46, 0xb, 0, 1, 0, OptionMenu_ApplyHighlightedValue, 2);;
    Menu_AddItemType3(&g_controlSetupMenu, 0, 0x3c, 1, 0, 0, 0, (MenuItemAction)OptionMenu_RestartCountdown, 3);;
    if (CGameInfo::GetConfiguredGameMode() == 0)
        Menu_AddItemType3(&g_controlSetupMenu, 0, 0xd3, 2, 0, 0, 0, OptionMenu_LoadGameItem, 4);;
    Menu_AddItemType3(&g_controlSetupMenu, 0, 0x88, 2, 1, 1, 0, OptionMenu_RestartEmptyItemRecords, 5);;
    Menu_SetCallbacks(&g_controlSetupMenu, (MenuCallback)OptionMenu_ConsumeRefreshRequest, (MenuCallback)OptionMenu_UpdateMessagesAndRecordStates,
                      (MenuCallback)OptionMenu_DispatchRecordEditState, (MenuCallback)OptionMenu_IncrementSharedValue);
    Menu_ValidateCursor(&g_controlSetupMenu, 0);
    OptionMenu_ResetRecords();
    OptionMenu_LoadPartTextures();
    OptionPreview_UpdateAllSlots();
    Menu_SetFlags(&g_controlSetupMenu, 0, 0, 0, 0);
}

// Builds the in-race option menus and switches the active menu to the first one.
// FUNCTION: CMR2 0x0050f180
void OptionMenu_BuildAndActivatePages(void)
{
    Menu_QueueDefaultAction();
    OptionMenu_RebuildOptions();
    OptionMenu_BuildValueSliders();
    OptionMenu_BuildControlSetup();
    OptionMenu_BuildStatusMenu();
    OptionMenu_BuildAdvancedOptions();
    g_pMenu0x00831778 = NULL;
    g_pMenu0x0083177c = OptionMenu_GetControlSetupMenu();
}


// FUNCTION: CMR2 0x00404ef0
void InRaceMenu_Close(void)
{
    CGameInfo::SetInputAndGamePaused(0);
    View_SetShake(g_unk0x0052af58[1], 0);
    CGameInfo::m_unk0x0052af40 = 0;
}

// Callback 0 of the camera options menu: turns the current values into the
// four slider positions.
// FUNCTION: CMR2 0x00403110
void InRaceMenu_FillCameraOffsetSliders(Menu *pMenu)
{
    // PORT: the ranges of InRaceMenu_ReadCameraOffsetSliders.
    int distance = FixDiv((pMenu->items[2].min - 1) * (g_cameraMenuLag - 0x40000), 0xc0000);
    int angle = FixDiv((pMenu->items[3].min - 1) * g_cameraMenuPitch, 0x11c);
    int height = FixDiv((pMenu->items[0].min - 1) * (g_cameraMenuOffset.y - 0x13333), 0xcccd);
    int depth = FixDiv((pMenu->items[1].min - 1) * (g_cameraMenuOffset.z - 0x28000), 0x78000);
    pMenu->items[2].max = (BYTE)((distance + 0x8000) >> 16);
    pMenu->items[3].max = (BYTE)((angle + 0x8000) >> 16);
    pMenu->items[0].max = (BYTE)((height + 0x8000) >> 16);
    pMenu->items[1].max = (BYTE)((distance + 0x8000) >> 16);
}

// GLOBAL: CMR2 0x005160a0
char g_strCrtnaTga[] = "%s\\game\\menus\\crtna640.tga";
// GLOBAL: CMR2 0x005160bc
char g_strRoCxTga[] = "%s\\game\\menus\\RoCx_640.tga";
// GLOBAL: CMR2 0x005160d8
char g_strAr640DTga[] = "%s\\game\\menus\\Ar_640D.tga";
// GLOBAL: CMR2 0x005160f4
char g_strAr640ATga[] = "%s\\game\\menus\\Ar_640A.tga";

#include "Texture.h"
#include "StageTiming.h"

void InRaceMenu_ApplyDefaultCarSetup(Menu *pMenu, int param);

// Loads the in-race menu textures (arrows, curtain and, in championships,
// the round box).
// FUNCTION: CMR2 0x004054f0
void InRaceMenu_LoadTextures(void)
{
    bool loaded;

    sprintf(CFrontend::m_stringDest, g_strAr640ATga, CInstallInfo::GetGameCDPath());
    g_inRaceMenuUpArrowTexture = CTexture::FindLoadTexture((GenericFile *)StageTiming_GetStageFile1(), CFrontend::m_stringDest,
                                                     &loaded, 0, 0, 0);
    sprintf(CFrontend::m_stringDest, g_strAr640DTga, CInstallInfo::GetGameCDPath());
    g_inRaceMenuDownArrowTexture = CTexture::FindLoadTexture((GenericFile *)StageTiming_GetStageFile1(), CFrontend::m_stringDest,
                                                     &loaded, 0, 0, 0);
    if (CGameInfo::GetConfiguredGameMode() == 4) {
        sprintf(CFrontend::m_stringDest, g_strRoCxTga, CInstallInfo::GetGameCDPath());
        g_inRaceMenuRoundBoxTexture = CTexture::FindLoadTexture((GenericFile *)StageTiming_GetStageFile0(),
                                                         CFrontend::m_stringDest, &loaded, 0, 0, 0);
    }
    sprintf(CFrontend::m_stringDest, g_strCrtnaTga, CInstallInfo::GetGameCDPath());
    g_inRaceMenuCurtainTexture = CTexture::FindLoadTexture((GenericFile *)StageTiming_GetStageFile1(), CFrontend::m_stringDest,
                                                     &loaded, 0, 0, 0);
}

// Driver record the camera options menu started with, and its working copy
// (ChaseCameraSettings; defining that struct in this file changes an addition
// in OptionMenu_DrawResultsRallyInfo).
// GLOBAL: CMR2 0x00529900
int g_cameraMenuStartDriverRecord[5];
// GLOBAL: CMR2 0x0052a858
int g_cameraMenuDriverRecord[5];

void ChaseCamera_GetOffset(FixVector *pOut, BYTE index);
int ChaseCamera_GetLag(BYTE index);
short ChaseCamera_GetPitch(BYTE index);
struct ChaseCameraSettings *RallyData_GetDriverCameraRecord(BYTE index);

// Callback 0 of the camera options menu: remembers the current camera and
// shows it on the sliders.
// FUNCTION: CMR2 0x00403700
void InRaceMenu_EnterCameraOffsetOptions(Menu *pMenu, char unused)
{
    BYTE index;

    memcpy(g_cameraMenuStartDriverRecord, RallyData_GetDriverCameraRecord(StageUI_GetRaceEndEventCount() + g_unk0x0052af58[1]), sizeof(g_cameraMenuStartDriverRecord));
    index = g_unk0x0052af58[1];
    memcpy(g_cameraMenuDriverRecord, g_cameraMenuStartDriverRecord, sizeof(g_cameraMenuDriverRecord));
    ChaseCamera_GetOffset(&g_cameraMenuStartOffset, index);
    g_cameraMenuStartLag = ChaseCamera_GetLag(g_unk0x0052af58[1]);
    g_cameraMenuStartPitch = ChaseCamera_GetPitch(g_unk0x0052af58[1]);
    g_cameraMenuOffset.x = g_cameraMenuStartOffset.x;
    g_cameraMenuOffset.y = g_cameraMenuStartOffset.y;
    g_cameraMenuOffset.z = g_cameraMenuStartOffset.z;
    g_cameraMenuLag = g_cameraMenuStartLag;
    g_cameraMenuPitch = g_cameraMenuStartPitch;
    InRaceMenu_FillCameraOffsetSliders(pMenu);
    g_unk0x0052ad54 = 1;
}

// Item callback of the camera menu "default" item.
// FUNCTION: CMR2 0x004036c0
void InRaceMenu_ResetCameraDefaults(Menu *pMenu, char unused)
{
    g_cameraMenuLag = 0xe0000;
    g_cameraMenuPitch = 0x2d;
    g_cameraMenuOffset.y = 0x18000;
    g_cameraMenuOffset.z = 0x68000;
    InRaceMenu_FillCameraOffsetSliders(pMenu);
}

// ===== Agent3 batch 1: GameInfo option records (>= 0x4f0000) =====

extern RallyCarDamageState *RallyData_GetDriverGroupRecord(int index);

// Draws the four one pixel edges of the option record's box (x, y, w, h);
// the vertical edges start one pixel inside the horizontal ones.
// match 82%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0050cb30
void OptionMenu_DrawBoxEdges(short *pRect, BYTE *pColour)
{
    short edge[4];

    edge[0] = pRect[0];
    edge[2] = pRect[2];
    edge[3] = 1;
    edge[1] = pRect[1];
    Sprite_FillRect(&g_pGraphics->field309_0x150, edge, pColour, 3);
    edge[1] = pRect[3] + pRect[1] - 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, edge, pColour, 3);
    edge[1] = pRect[1];
    edge[3] = pRect[3];
    edge[2] = 1;
    edge[0] = pRect[0];
    Sprite_FillRect(&g_pGraphics->field309_0x150, edge, pColour, 3);
    edge[0] = pRect[2] + pRect[0] - 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, edge, pColour, 3);
}

// Returns the byte at column type of the 7-byte option record index,
// sign extended; 0 for an unknown column.
// FUNCTION: CMR2 0x00502c60
int OptionMenu_GetEntryCarSetupValue(int index, int type)
{
    switch (type) {
    case 0: return g_optionCarSetups[index + 4].tyres;
    case 1: return g_optionCarSetups[index + 4].gearbox;
    case 6: return g_optionCarSetups[index + 4].steering;
    case 2: return g_optionCarSetups[index + 4].suspension;
    case 3: return g_optionCarSetups[index + 4].powerRatio;
    case 4: return g_optionCarSetups[index + 4].brakeBias;
    case 5: return g_optionCarSetups[index + 4].brakePower;
    }
    return 0;
}

// Eases the option record's bar towards its target: while the record is faded
// out (mode 0) both bar sizes are cleared; in mode 1 the current fraction is
// scaled by field_0x0 when direction is set, otherwise the remaining distance.
// FUNCTION: CMR2 0x00501d50
void OptionMenu_EaseRecordBar(int index, short *pBar, int direction, int unused)
{
    Unk0x0082b2c0 *p = &g_optionRecordTransitions[index];

    if (p->field_0xc == 1) {
        if (direction != 0) {
            pBar[2] = (short)FixMulShift32(pBar[2] << 16, p->field_0x0);
            return;
        }
        pBar[3] = (short)FixMulShift32(pBar[3] << 16, p->field_0x0);
        return;
    }
    if (p->field_0xc == 0) {
        pBar[3] = 0;
        pBar[2] = 0;
    }
}

// Returns the byte at column type of the 7-byte option record index; the
// signed columns are divided by 10.
// FUNCTION: CMR2 0x005028a0
BYTE OptionMenu_GetCarSetupChoice(int index, int type)
{
    switch (type) {
    case 0: return (BYTE)g_optionCarSetups[index].tyres;
    case 1: return (BYTE)g_optionCarSetups[index].gearbox;
    case 6: return g_optionCarSetups[index].steering / 10;
    case 2: return g_optionCarSetups[index].suspension / 10;
    case 3: return g_optionCarSetups[index].powerRatio / 10;
    case 4: return g_optionCarSetups[index].brakeBias / 10;
    case 5: return g_optionCarSetups[index].brakePower / 10;
    }
    return 0;
}

// Index of the first option slot that is not a plain list entry, or 5 when
// the slot at index 4 holds one; 0 when there is none.
// FUNCTION: CMR2 0x004ff550
int OptionMenu_FindFirstDetailedSlot(void)
{
    int i;
    int found;

    found = 0;
    for (i = 0; i < (char)OptionMenu_GetControlSetupMenu()->itemCount; i++) {
        if (g_optionSlotTypes[i] != 3) {
            found = 1;
            break;
        }
    }
    if (found) {
        if (CGameInfo::GetConfiguredGameMode() != 0) {
            if (i == 4)
                i = 5;
        }
        return i;
    }
    return 0;
}

// Draws the option record's text: mode 2 draws it whole; in mode 1 the part
// inside the 16.16 fraction of its length is drawn with the first font and
// the remainder with the second one, at that width.
// match 77%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x005020a0
void OptionMenu_DrawTransitionText(int index, int font1, int font2, char *text, short x, short y,
                  int *pColour1, int *pColour2, unsigned int flags)
{
    Unk0x0082b2c0 *p = &g_optionRecordTransitions[index];
    int len;
    int count;
    int width;
    int i;
    int j;

    if (p->field_0xc == 2) {
        Font_DrawText(font1, text, x, y, pColour1, flags);
        return;
    }
    if (p->field_0xc == 1) {
        len = (int)strlen(text);
        count = FixMulShift32(len << 16, p->field_0x0);
        for (i = 0; i < count; i++)
            g_truncatedTextBuffer[i] = text[i];
        g_truncatedTextBuffer[count] = 0;
        Font_DrawText(font1, g_truncatedTextBuffer, x, y, pColour1, flags);
        if (count < len) {
            x += Font_GetTextWidth(font1, (BYTE *)g_truncatedTextBuffer);
            for (i = count; i < len; i++)
                g_truncatedTextBuffer[i - count] = text[i];
            g_truncatedTextBuffer[len - count] = 0;
            Font_DrawText(font2, g_truncatedTextBuffer, x, y, pColour2, flags);
        }
    }
}

// Option record value as a percentage 0..100: picks the byte selected by
// type, letting a byte that follows it win when it is larger, and a later
// byte win when the value is still smaller.
// FUNCTION: CMR2 0x00502df0
unsigned int OptionMenu_GetRecordPercentage(int index, int type, int dynamic)
{
    RallyCarDamageState *p;
    int value;
    int other;

    value = 0;
    if (dynamic != 0)
        p = RallyData_GetDriverGroupRecord(index);
    else
        p = &g_playerOptionCache.records[index];
    switch (type) {
    case 1:
        value = p->damage.intensity[14];
        break;
    case 2:
        value = p->damage.intensity[15];
        break;
    case 3:
        value = p->damage.intensity[6];
        if (p->damage.intensity[7] > value)
            value = p->damage.intensity[7];
        if (p->damage.intensity[8] > value)
            value = p->damage.intensity[8];
        other = p->damage.intensity[9];
        if (other > value)
            value = other;
        break;
    case 4:
        value = p->damage.intensity[16];
        break;
    case 5:
        value = p->damage.intensity[10];
        if (p->damage.intensity[11] > value)
            value = p->damage.intensity[11];
        if (p->damage.intensity[12] > value)
            value = p->damage.intensity[12];
        other = p->damage.intensity[13];
        if (other > value)
            value = other;
        break;
    case 6:
        value = p->damage.intensity[17];
        break;
    case 7:
        value = p->damage.intensity[4];
        other = p->damage.intensity[5];
        if (other > value)
            value = other;
        break;
    case 8:
        value = p->damage.intensity[23];
        break;
    case 9:
        value = p->damage.intensity[24];
        break;
    case 10:
        value = p->damage.intensity[25];
        break;
    case 11:
        value = p->damage.intensity[0];
        if (p->damage.intensity[1] > value)
            value = p->damage.intensity[1];
        if (p->damage.intensity[2] > value)
            value = p->damage.intensity[2];
        other = p->damage.intensity[3];
        if (other > value)
            value = other;
        break;
    }
    other = (value * 100) / 0xff;
    if (other == 0) {
        if (value != 0)
            return 1;
    } else if (other > 100) {
        other = 100;
    }
    return other;
}

// Scales both bar sizes of the option record by field_0x0 while it fades in
// (mode 1), or clears them when it is fully out (mode 0).
// FUNCTION: CMR2 0x00501f00
void OptionMenu_ScaleRecordBar(int index, short *pBar)
{
    Unk0x0082b2c0 *p = &g_optionRecordTransitions[index];

    if (p->field_0xc == 1) {
        pBar[2] = (short)(FixMul(pBar[2] << 16, p->field_0x0) >> 16);
        pBar[3] = (short)(FixMul(pBar[3] << 16, p->field_0x0) >> 16);
        return;
    }
    if (p->field_0xc == 0) {
        pBar[3] = 0;
        pBar[2] = 0;
    }
}

// Address of the byte set by the option menu (0x82b1b8).
// FUNCTION: CMR2 0x00501ab0
BYTE *OptionMenu_GetStateByteAddress(void)
{
    return g_unk0x0082b1b8;
}

// Advances the option menu's overlay pulse: a sine running over a minute is
// mapped to a grey level and stored in the fade colour bytes.
// FUNCTION: CMR2 0x00501ac0
void OptionMenu_UpdateOverlayPulse(void)
{
    int value;
    BYTE shade;

    value = CMain::GetFrameDelta() % 0x3c;
    value = FixDiv(value << 16, 0x3c0000);
    value = FixMul(value, 0x1680000);
    value = g_sinTable[(unsigned short)FTOI(__int64, X87S_MUL(value, g_fixedDegreesToAngle12)) & 0xfff] + 0x10000;
    value = FixMul(value, 0x8000);
    if (value < 0)
        value = 0;
    else if (value > 0x10000)
        value = 0x10000;
    shade = (BYTE)((FixMul(value, 0x7f0000) + 0x800000) >> 16);
    g_unk0x0082b1b8[3] = 0xff;
    g_unk0x0082b1b8[2] = shade;
    g_unk0x0082b1b8[1] = shade;
    g_unk0x0082b1b8[0] = shade;
}

// Advances the 16.16 transition value of every option record: records that
// passed their duration switch to mode 2 at full scale, the rest receive the
// proportion of the elapsed time squared in mode 1 or square-rooted in mode 2.
// FUNCTION: CMR2 0x00501b90
void OptionMenu_UpdateRecordTransitions(void)
{
    int i;

    for (i = 0; i < g_unk0x0082b1bc; i++) {
        int elapsed;

        if (g_optionRecordTransitions[i].field_0xc != 1)
            continue;
        elapsed = CMain::GetFrameDelta() - g_optionRecordTransitions[i].field_0x8;
        if (elapsed >= g_optionRecordTransitions[i].field_0x4) {
            g_optionRecordTransitions[i].field_0xc = 2;
            g_optionRecordTransitions[i].field_0x0 = 0x10000;
        } else {
            int value = FixDiv(elapsed << 16, g_optionRecordTransitions[i].field_0x4 << 16);
            int kind = g_optionRecordTransitions[i].field_0x10;
            g_optionRecordTransitions[i].field_0x0 = value;
            switch (kind) {
            case 2:
                g_optionRecordTransitions[i].field_0x0 = FixSqrt(g_optionRecordTransitions[i].field_0x0);
                break;
            case 1:
                g_optionRecordTransitions[i].field_0x0 =
                    FixMul(g_optionRecordTransitions[i].field_0x0, g_optionRecordTransitions[i].field_0x0);
                break;
            }
        }
    }
    OptionMenu_UpdateOverlayPulse();
}

// True when the option slot is enabled: mode 2/4 are checked against their
// selectors, everything else is accepted.
// FUNCTION: CMR2 0x00501280
int OptionMenu_IsSlotEnabled(int mode, int index)
{
    int result = 1;

    if (mode == 2) {
        if (CFrontend::GetArchiveSecondaryFlagEntry(index) == 0)
            result = 0;
    } else if (mode == 4) {
        if (CFrontend::GetArchivePrimaryFlagEntry(index) == 0)
            result = 0;
    }
    return result;
}

// Whether the option slot has been raised above its base value.
// FUNCTION: CMR2 0x00502630
int OptionMenu_IsSlotValueAboveBase(int index, int type)
{
    int value;

    value = OptionMenu_GetSlotSelector(index);
    if (g_optionColumnBaseWeights[type] <= value)
        return 1;
    return g_optionColumnDirty[index][type] != 0;
}

// Whether the option slot value equals its default, per column.
// FUNCTION: CMR2 0x00502b10
BYTE OptionMenu_IsCarSetupValueUnchanged(int index, int type)
{
    switch (type) {
    case 0: return g_optionCarSetups[index + 4].tyres == g_optionCarSetups[index].tyres;
    case 1: return g_optionCarSetups[index + 4].gearbox == g_optionCarSetups[index].gearbox;
    case 6: return g_optionCarSetups[index + 4].steering == g_optionCarSetups[index].steering;
    case 2: return g_optionCarSetups[index + 4].suspension == g_optionCarSetups[index].suspension;
    case 3: return g_optionCarSetups[index + 4].powerRatio == g_optionCarSetups[index].powerRatio;
    case 4: return g_optionCarSetups[index + 4].brakeBias == g_optionCarSetups[index].brakeBias;
    case 5: return g_optionCarSetups[index + 4].brakePower == g_optionCarSetups[index].brakePower;
    }
    return 0;
}

// Whether the option slot value has risen above its base value.
// FUNCTION: CMR2 0x00502fc0
int OptionMenu_IsRecordPercentageAboveBase(int index, int type)
{
    int value;

    value = OptionMenu_GetSlotSelector(index);
    if (g_recordGroupWeights[type] <= value)
        return OptionMenu_GetRecordPercentage(index, type, 1) != 0;
    return g_playerOptionCache.dirty[index][type] != 0;
}

// Finds the index of the entry with the given id in the list at 0x3c, or -1.
// FUNCTION: CMR2 0x00508f60
int OptionPreview_FindMeshId(unsigned int id, OptionPreviewMeshRecord *param2)
{
    int *pEntry;
    int count;
    int i;

    count = param2->meshCount;
    i = 0;
    while (i < count) {
        if ((param2->geometry.nodes[i]->flags & 0xff) == id)
            return i;
        i++;
    }
    return -1;
}

// Screen-space viewport used by the stage HUD: x, y, width, height.
// GLOBAL: CMR2 0x0082c9ec
short g_optionPreviewExpandedRect[4];

// Maps a HUD point from viewport space to screen space; when the transform is
// marked as already scaled (field_0x1c == 0x10000) it only resolves the
// 640x480 reference to the current resolution.
// FUNCTION: CMR2 0x00503b70
void OptionMenu_TransformHudPointToScreen(FixInterp *p, short *pX, short *pY)
{
    int value;

    if (p->current != 0x10000) {
        value = (*pX - g_optionPreviewExpandedRect[0]) << 16;
        value = FixDiv(value, g_optionPreviewExpandedRect[2] << 16);
        *pX = (FixMul(value, p->dstX2 << 16) >> 16) + p->dstX1;
        value = (*pY - g_optionPreviewExpandedRect[1]) << 16;
        value = FixDiv(value, g_optionPreviewExpandedRect[3] << 16);
        *pY = (FixMul(value, p->dstY2 << 16) >> 16) + p->dstY1;
    }
    *pX = (short)((*pX * (int)g_pGraphics->resX) / 0x280);
    *pY = (short)((*pY * (int)g_pGraphics->resY) / 0x1e0);
}

// Re-runs the option-menu callback stored in the global (0x82b1b4).
// FUNCTION: CMR2 0x00501390
BYTE OptionMenu_UpdateBackgroundRoot(void)
{
    SceneNode_Destroy(g_optionMenuSceneRoot);
    return 1;
}

// Option menu sound names (7 bytes each) and the file name they are loaded
// from, plus the volume/index state of the option menu.
// GLOBAL: CMR2 0x005296c4
char g_optionMenuSoundNames[5][7] = {"move", "select", "back", "error", "toggle"};
// GLOBAL: CMR2 0x005296e8
char g_str0x005296e8[16] = "%s\\menu\\%s.wav";
// GLOBAL: CMR2 0x00831888
int g_unk0x00831888;
// GLOBAL: CMR2 0x0083188c
int g_unk0x0083188c;

// Loads the five option menu sounds. Returns 0 if any of them failed to load.
// FUNCTION: CMR2 0x0050f3c0
BYTE OptionMenu_LoadSounds(void)
{
    BYTE result;
    char *name;
    char *nameEnd;

    result = 1;
    g_unk0x00831888 = 0;
    name = (char *)g_optionMenuSoundNames;
    nameEnd = (char *)g_optionMenuSoundNames + sizeof(g_optionMenuSoundNames);
    do {
        sprintf(CFrontend::m_stringDest, g_str0x005296e8, CInstallInfo::GetSoundsDir(), name);
        if (Sound_LoadSample(CFrontend::m_stringDest, 0, OptionMenu_GetCommonArchive()) == 0)
            result = 0;
        name += 7;
    } while (name < nameEnd);
    return result;
}

// Sets the option menu volume and hands the menu sound handles to the input
// system.
// FUNCTION: CMR2 0x0050f420
void OptionMenu_ConfigureSoundHandles(void)
{
    g_unk0x0083188c = -1;
    // option menu volume: the stored 0..0x7f setting scaled to 16.16 and quartered
    CInput::SetMenuSoundVolume((((int)CGameInfo::GetEffectsSoundVolume() << 16) / 100) / 4);
    CInput::SetMenuSounds(g_unk0x00831888, g_unk0x00831888 + 1, g_unk0x00831888 + 2,
                         g_unk0x00831888 + 3, g_unk0x00831888 + 4);
    Menu_SetInputStateFlag(1);
}

// Option menu archives: every region uses the language directories of its own
// languages (region * 5 + language), indexed like the country codes.
// GLOBAL: CMR2 0x00529794
char g_str0x00529794[20] = "%s\\%d\\Common.bfl";
// GLOBAL: CMR2 0x0052976c
char g_str0x0052976c[20] = "%s\\%d\\%sDay%d.bfl";
// GLOBAL: CMR2 0x00529780
char g_str0x00529780[20] = "%s\\%d\\%sDay%dC.bfl";
// GLOBAL: CMR2 0x0051a100
char g_str0x0051a100[12] = "%s%s%s.bfl";

// Country code of each RallyDataCountryIndex value.
// GLOBAL: CMR2 0x00519260
char g_str0x00519260[4] = "JAP";
// GLOBAL: CMR2 0x00519268
char g_str0x00519268[4] = "ITA";
// GLOBAL: CMR2 0x0051926c
char g_str0x0051926c[4] = "KEN";
// GLOBAL: CMR2 0x00519270
char g_str0x00519270[4] = "AUS";
// GLOBAL: CMR2 0x00519274
char g_str0x00519274[4] = "SWE";
// GLOBAL: CMR2 0x00519278
char g_str0x00519278[4] = "FRA";
// GLOBAL: CMR2 0x0051927c
char g_str0x0051927c[4] = "GRE";
// GLOBAL: CMR2 0x00519280
char g_str0x00519280[4] = "FIN";
// GLOBAL: CMR2 0x005296f8
char *g_unk0x005296f8[9] = {g_str0x00519280, g_str0x0051927c, g_str0x00519278,
                            g_str0x00519274, g_str0x00519270, g_str0x0051926c,
                            g_str0x00519268, CFrontend::m_strUK, NULL};

// Language text directory of every region and language (region * 5 + language).
// Countries without a language of their own keep an empty entry.
// GLOBAL: CMR2 0x0051a00c
char g_str0x0051a00c[12] = "polishtext";
// GLOBAL: CMR2 0x0051a018
char g_str0x0051a018[12] = "engusatext";
// GLOBAL: CMR2 0x0051a024
char g_str0x0051a024[12] = "germantext";
// GLOBAL: CMR2 0x0051a030
char g_str0x0051a030[12] = "italiantext";
// GLOBAL: CMR2 0x0051a03c
char g_str0x0051a03c[12] = "spanishtext";
// GLOBAL: CMR2 0x0051a048
char g_str0x0051a048[12] = "frenchtext";
// GLOBAL: CMR2 0x0051a054
char g_str0x0051a054[12] = "englishtext";
// GLOBAL: CMR2 0x0052971c
char *g_unk0x0052971c[20] = {
    g_str0x0051a054, g_str0x0051a048, g_str0x0051a03c, g_str0x0051a030, g_str0x0051a024,
    g_str0x0051a018, g_str0x0051a048, g_str0x0051a03c, NULL, NULL,
    g_str0x0051a054, NULL, NULL, NULL, NULL,
    g_str0x0051a00c, NULL, NULL, NULL, NULL};

// Loads the three option menu archives: the common one, the day file of the
// stage and the region/language file.
// FUNCTION: CMR2 0x0050f4f0
void OptionMenu_LoadArchives(void)
{
    int resolution;
    int stage;

    stage = RallyDataStageIndex() >> 2;
    if (CGameInfo::GetScreenWidth() >= 0x400 && CFrontend::IsTextureWidthSupported(0x400) &&
        CFrontend::IsTextureHeightSupported(0x400))
        resolution = 0x400;
    else
        resolution = 0x280;

    sprintf(CFrontend::m_stringDest, g_str0x00529794, CInstallInfo::GetSetupRepDir(), resolution);
    CGenericFileLoader::LoadIntoFileRecord(OptionMenu_GetCommonArchive(), CFrontend::m_stringDest);

    if (CFrontend::HasDXT1TextureSupport()) {
        sprintf(CFrontend::m_stringDest, g_str0x00529780, CInstallInfo::GetSetupRepDir(), resolution,
                g_unk0x005296f8[RallyDataCountryIndex() & 0xff], stage + 1);
    } else {
        sprintf(CFrontend::m_stringDest, g_str0x0052976c, CInstallInfo::GetSetupRepDir(), resolution,
                g_unk0x005296f8[RallyDataCountryIndex() & 0xff], stage + 1);
    }
    CGenericFileLoader::LoadIntoFileRecord(OptionMenu_GetStageArchive(), CFrontend::m_stringDest);

    sprintf(CFrontend::m_stringDest, g_str0x0051a100, CInstallInfo::GetCountrySpecificDir(),
            CGameInfo::GetGameRegionDirectory(),
            g_unk0x0052971c[CGameInfo::GetGameRegion() * 5 + CGameInfo::GetGameLanguage()]);
    CGenericFileLoader::LoadIntoFileRecord(OptionMenu_GetLanguageArchive(), CFrontend::m_stringDest);

    CGame::RegisterCallback(OptionMenu_ReleaseTextures, NULL);
}

// Format string printed when the option menu's background world is created.
// GLOBAL: CMR2 0x00527050
char g_str0x00527050[] = "*** WORLD CREATED: 0x%08X ***\n";
// Camera node of the option menu's background world (0x82b1b0).
// GLOBAL: CMR2 0x0082b1b0
SceneNode *g_optionMenuCamera;

void Scene_SetAmbient(BYTE *pColour, int boost);
SceneNode *Scene_CreateLight(int type, int r, int g, int b, FixVector *pPosition, FixAngles *pAngles, SceneNode *pParent);
int Game_PrepareScene(SceneNode *pRoot, SceneNode *pCamera, void *pRect, int param);

// Builds the option menu's background world: root node, camera, ambient and key
// light, then registers the release callback.
// FUNCTION: CMR2 0x005013a0
void OptionMenu_CreateBackgroundWorld(void)
{
    SceneNode *pNode;
    BYTE colour[4];
    FixAngles angles;
    short view[4];
    FixVector translation;
    FixVector lightPosition;
    int i;

    view[0] = 0;
    view[1] = 0;
    view[2] = (short)((int)g_pGraphics->resX * 2 / 3);
    view[3] = (short)((int)g_pGraphics->resY * 2 / 3);
    translation.x = 0x23d7;
    translation.y = 0xffffe8f6;
    translation.z = 0xfffb0000;
    lightPosition.x = 0x50000;
    lightPosition.y = 0x50000;
    lightPosition.z = 0xfffb0000;
    angles.x = 0;
    angles.y = 0;
    angles.z = 0;
    angles.pad = 0;
    colour[0] = 0xc8;
    colour[1] = 0xc8;
    colour[2] = 0xc8;
    colour[3] = 0xff;
    g_optionMenuSceneRoot = SceneNode_CreateRoot();
    sprintf(CFrontend::m_stringDest, g_str0x00527050, g_optionMenuSceneRoot);
    puts(CFrontend::m_stringDest);
    g_optionMenuCamera = SceneType2_Create(&translation, &angles, NULL, g_optionMenuSceneRoot);
    for (pNode = g_optionMenuSceneRoot; pNode != NULL; pNode = pNode->pParent)
        pNode->dirty = 1;
    Scene_SetAmbient(colour, 0);
    Scene_CreateLight(2, 0x10000, 0x10000, 0x10000, &lightPosition, &angles, g_optionMenuSceneRoot);
    OptionMenu_SetPreviewProjection();
    Game_PrepareScene(g_optionMenuSceneRoot, g_optionMenuCamera, view, 0);
    CGame::RegisterCallback(OptionMenu_UpdateBackgroundRoot, NULL);
}
// Releases the three option menu textures and clears their handles.
// FUNCTION: CMR2 0x0050f480
int OptionMenu_ReleaseTextures(void)
{
    if (((GenericFile *)g_optionCommonArchive)->buffer != NULL) {
        CFileBuffer::FreeGenericFileBuffer(((GenericFile *)g_optionCommonArchive)->buffer);
        ((GenericFile *)g_optionCommonArchive)->buffer = NULL;
    }
    ((GenericFile *)g_optionCommonArchive)->didFileLoad = 0;
    ((GenericFile *)g_optionCommonArchive)->fileSize = 0;
    if (((GenericFile *)g_optionStageArchive)->buffer != NULL) {
        CFileBuffer::FreeGenericFileBuffer(((GenericFile *)g_optionStageArchive)->buffer);
        ((GenericFile *)g_optionStageArchive)->buffer = NULL;
    }
    ((GenericFile *)g_optionStageArchive)->didFileLoad = 0;
    ((GenericFile *)g_optionStageArchive)->fileSize = 0;
    if (((GenericFile *)g_optionLanguageArchive)->buffer != NULL) {
        CFileBuffer::FreeGenericFileBuffer(((GenericFile *)g_optionLanguageArchive)->buffer);
        ((GenericFile *)g_optionLanguageArchive)->buffer = NULL;
    }
    ((GenericFile *)g_optionLanguageArchive)->didFileLoad = 0;
    ((GenericFile *)g_optionLanguageArchive)->fileSize = 0;
    return 1;
}

// Derives the preview part damage fractions from the four quantised damage
// bytes at +0x108 of the rally record (channels 0..3).
// match 86%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00509d30
void OptionPreview_UnpackPartDamageFractions(int index)
{
    OptionPreviewMeshRecord *pRec;
    RallyCarDamageState *p;
    int i;

    pRec = &g_previewMeshRecords[index];
    p = RallyData_GetDriverGroupRecord(index);
    for (i = 0; i < 4; i++)
        pRec->partDamageFractions[i] = FixDiv(p->damage.intensity[i] << 16, 0xff0000);
}

// Derives the four corner height offsets from damage channels 6..9.
// match 81%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00509be0
void OptionPreview_UpdateCornerHeightOffsets(int index)
{
    OptionPreviewMeshRecord *pRec;
    RallyCarDamageState *p;
    int i;

    pRec = &g_previewMeshRecords[index];
    p = RallyData_GetDriverGroupRecord(index);
    for (i = 0; i < 4; i++)
        pRec->cornerHeightOffsets[i] = FixDiv(p->damage.intensity[6 + i] << 16, 0xff0000);
    pRec->cornerHeightOffsets[0] = FixMul(pRec->cornerHeightOffsets[0], FixMul(0x3333, 0xffff0000));
    pRec->cornerHeightOffsets[1] = FixMul(pRec->cornerHeightOffsets[1], FixMul(0x3333, 0xffff0000));
    pRec->cornerHeightOffsets[2] = FixMul(pRec->cornerHeightOffsets[2], FixMul(0x3333, 0x8000));
    pRec->cornerHeightOffsets[3] = FixMul(pRec->cornerHeightOffsets[3], FixMul(0x3333, 0xffff8000));
}

// Selects the preview damage material and mask for each part category.
void Game_SetTriangleField2CByGroup(Mesh *pMesh, int mask, int value);
void Game_SetTriangleField30ByGroup(Mesh *pMesh, int mask, int value);
// FUNCTION: CMR2 0x00508fa0
void OptionPreview_SetMeshRenderFlags(int index, int param2, BYTE param3){
    OptionPreviewMeshRecord *pRecord = &g_previewMeshRecords[index];
    int target;
    int mask;
    int i;

    switch (param2) {
    case 0:
        target = 0;
        mask = 1;
        break;
    case 1:
        target = 3;
        mask = 4;
        break;
    default:
        if (param3 == 4 || param3 == 5) {
            target = 3;
            mask = 4;
        } else {
            target = 5;
            mask = 7;
        }
        break;
    }
    switch (param3) {
    case 0:
        i = OptionPreview_FindMeshId(7, pRecord);
        if (i >= 0) {
            Game_SetTriangleField2CByGroup(pRecord->geometry.meshes[i], 0x100, target);
            Game_SetTriangleField30ByGroup(pRecord->geometry.meshes[i], 0x100, mask);
            return;
        }
        break;
    case 1:
        i = OptionPreview_FindMeshId(0xc, pRecord);
        if (i >= 0) {
            Game_SetTriangleField2CByGroup(pRecord->geometry.meshes[i], 0x20, target);
            Game_SetTriangleField30ByGroup(pRecord->geometry.meshes[i], 0x20, mask);
        }
        i = OptionPreview_FindMeshId(7, pRecord);
        if (i >= 0) {
            Game_SetTriangleField2CByGroup(pRecord->geometry.meshes[i], 0x20, target);
            Game_SetTriangleField30ByGroup(pRecord->geometry.meshes[i], 0x20, mask);
            return;
        }
        break;
    case 2:
        i = OptionPreview_FindMeshId(7, pRecord);
        if (i >= 0) {
            Game_SetTriangleField2CByGroup(pRecord->geometry.meshes[i], 0x40, target);
            Game_SetTriangleField30ByGroup(pRecord->geometry.meshes[i], 0x40, mask);
            return;
        }
        break;
    case 3:
        i = OptionPreview_FindMeshId(7, pRecord);
        if (i >= 0) {
            Game_SetTriangleField2CByGroup(pRecord->geometry.meshes[i], 0x80, target);
            Game_SetTriangleField30ByGroup(pRecord->geometry.meshes[i], 0x80, mask);
            return;
        }
        break;
    case 4:
        i = OptionPreview_FindMeshId(0xe, pRecord);
        if (i >= 0) {
            Game_SetTriangleField2CByGroup(pRecord->geometry.meshes[i], 4, target);
            return;
        }
        break;
    case 5:
        i = OptionPreview_FindMeshId(0xe, pRecord);
        if (i >= 0)
            Game_SetTriangleField2CByGroup(pRecord->geometry.meshes[i], 8, target);
        break;
    }
}

// Chooses the highlight value for the selected option item and stores it at
// +0x1e, together with the option's current value at +0x1f.
// (CFrontend::FreeLocalizedTextStrings)

// GLOBAL: CMR2 0x00831880
BYTE *g_optionLanguageText;
// GLOBAL: CMR2 0x00831884
BYTE g_optionLanguageTextInArchive;

// FUNCTION: CMR2 0x004ff5b0
void OptionMenu_FillValueSlider(void)
{
    unsigned int value;

    value = OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 1)].max;
    switch (value) {
    case 0:
        OptionMenu_GetValueSliderMenu()->items[0].min = 7;
        break;
    case 1:
        OptionMenu_GetValueSliderMenu()->items[0].min = 5;
        break;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        OptionMenu_GetValueSliderMenu()->items[0].min = 0xb;
        break;
    }
    OptionMenu_GetValueSliderMenu()->items[0].max = (BYTE)OptionMenu_GetCarSetupChoice(CGameInfo::GetActiveOptionSlot(), value);
}

// Frees the option menu sound buffer and stops the streaming sound.
// FUNCTION: CMR2 0x0050f340
bool OptionMenu_ReleaseSoundBuffer(void)
{
    if (&g_optionLanguageText != NULL) {
        if (g_optionLanguageTextInArchive == 0)
            CFileBuffer::FreeGenericFileBuffer(g_optionLanguageText);
        g_optionLanguageText = NULL;
    }
    CFrontend::FreeLocalizedTextStrings();
    return 1;
}

// GLOBAL: CMR2 0x0082ac60
BYTE g_unk0x0082ac60;
// GLOBAL: CMR2 0x0082ac4c
int g_unk0x0082ac4c;
// GLOBAL: CMR2 0x0082ac50
int g_unk0x0082ac50;
// Defined in FrontendScreens.cpp
extern int g_savedGameListSelection;
extern int g_savedGameListTop;
extern int g_unk0x0082aa40;
extern int g_unk0x0082ac48;
// GLOBAL: CMR2 0x0082ab44
BYTE g_unk0x0082ab44;
// GLOBAL: CMR2 0x00527380
int g_optionTextColours[3] = { -11579569, -11579569, -11579569 };
// GLOBAL: CMR2 0x0052738c
int g_optionTextBrightColours[3] = { -1, -2130706433, 1078939471 };
// Text of the option menu's status line (written elsewhere before it is drawn).
// GLOBAL: CMR2 0x0082a93c
char g_optionStatusLine[0x100];

// Requests a refresh of the option records.
// FUNCTION: CMR2 0x00502230
void OptionMenu_RequestRefresh(int unused, int unused2)
{
    g_unk0x0082ac60 = 1;
}

// GLOBAL: CMR2 0x0082bc08
Menu g_unk0x0082bc08;

void OptionMenu_DrawStatusLine(int unused);
void OptionMenu_SelectItemAndRefresh(Menu *pMenu, int unused);
void OptionMenu_LoadSelectedSavedGame(unsigned int, unsigned int);
void OptionMenu_DrawValueSelectionRows(Menu *);
void OptionMenu_DrawRaisedRallyInfo(Menu *);
void OptionMenu_IncrementSharedValue(Menu *pMenu, char param);
void OptionMenu_ApplyFirstModeItem(Menu *pMenu, int param);
void OptionMenu_DrawRallyInfoScreen(Menu *);

// Builds the option menu's status line: the record-list refresh callback and
// the owner's help-text draw.
// FUNCTION: CMR2 0x00502240
void OptionMenu_BuildStatusMenu(void)
{
    Menu_Init(&g_optionStatusMenu, 0, -1, 0, &g_controlSetupMenu, NULL, 1, 0, 0);
    Menu_AddItemType1(&g_optionStatusMenu, 0, -1, 0, -1);
    Menu_SetCallbacks(&g_optionStatusMenu, (MenuCallback)OptionMenu_RequestRefresh, NULL, (MenuCallback)OptionMenu_DrawStatusLine, NULL);
    Menu_ValidateCursor(&g_optionStatusMenu, 0);
}

// Builds the option menu's advanced-options list with its device and accept
// entries.
// FUNCTION: CMR2 0x005022a0
void OptionMenu_BuildAdvancedOptions(void)
{
    Menu_Init(&g_advancedOptionsMenu, 0, -1, 0, &g_controlSetupMenu, NULL, 0, 0, 1);
    Menu_AddItemType1(&g_advancedOptionsMenu, 0, 0x100, (MenuItemAction)OptionMenu_LoadSelectedSavedGame, -1);
    Menu_AddItemType1(&g_advancedOptionsMenu, 0, 0x101, 0, -1);
    Menu_SetCallbacks(&g_advancedOptionsMenu, OptionMenu_SelectItemAndRefresh, NULL, (MenuCallback)OptionMenu_DrawValueSelectionRows, NULL);
    Menu_ValidateCursor(&g_advancedOptionsMenu, 0);
}

// Builds the option menu's control-setup screen.
// FUNCTION: CMR2 0x00502440
void OptionMenu_BuildControlSetup(void)
{
    Menu_Init(&g_unk0x0082bc08, 0, -1, 0, &g_controlSetupMenu, NULL, 1, 0, 0);
    Menu_SetCallbacks(&g_unk0x0082bc08, NULL, (MenuCallback)RallyData_ValidateIndex, (MenuCallback)OptionMenu_DrawRaisedRallyInfo,
                      (MenuCallback)OptionMenu_IncrementSharedValue);
    Menu_ValidateCursor(&g_unk0x0082bc08, 0);
    Menu_SetFlags(&g_unk0x0082bc08, 0, 0, 0, 1);
}

// Builds the option menu's slider screen.
// FUNCTION: CMR2 0x005024a0
void OptionMenu_BuildValueSliders(void)
{
    Menu_Init(&g_valueSliderMenu, 0, -1, 0, &g_controlSetupMenu, NULL, 1, 0, 1);
    Menu_AddItemType3(&g_valueSliderMenu, 0, -1, 0x65, 0, 0, 0, (MenuItemAction)OptionMenu_ApplyFirstModeItem, 0);;
    Menu_SetCallbacks(&g_valueSliderMenu, NULL, (MenuCallback)RallyData_ValidateIndex, (MenuCallback)OptionMenu_DrawRallyInfoScreen, NULL);
    Menu_ValidateCursor(&g_valueSliderMenu, 0);
}

// Marks the given item selected and requests a refresh.
// FUNCTION: CMR2 0x004ffa50
void OptionMenu_SelectItemAndRefresh(Menu *pMenu, int unused)
{
    pMenu->cursor = 1;
    g_unk0x0082ac60 = 1;
}

// Restarts the option records and arms their countdown.
// FUNCTION: CMR2 0x00500110
void OptionMenu_RestartCountdown(int unused, int unused2)
{
    OptionMenu_FinishRecordTransitions();
    g_unk0x0082ac4c = 1;
}

// Rebuilds the option records once the refresh request is consumed.
// FUNCTION: CMR2 0x004ffa70
void OptionMenu_ConsumeRefreshRequest(int unused, int unused2)
{
    g_unk0x0082aa40 = SavedGames_GetCount();
    if (g_unk0x0082ac60 == 0) {
        g_unk0x0082ab44 = 0;
        g_savedGameListSelection = 0;
        g_savedGameListTop = 0;
        g_unk0x0082ac48 = 0;
        return;
    }
    g_unk0x0082ac60 = 0;
}

// Restarts the option records when the selected item has no value.
// FUNCTION: CMR2 0x00500190
void OptionMenu_RestartEmptyItemRecords(Menu *pMenu, MenuItem *unused)
{
    int index;

    index = Menu_FindItem(pMenu, 5);
    if (pMenu->items[index].max == 0) {
        OptionMenu_FinishRecordTransitions();
        g_unk0x0082ac50 = 1;
    }
}

// Draws the option menu's status line centred on the screen.
// FUNCTION: CMR2 0x0050e740
void OptionMenu_DrawStatusLine(int unused)
{
    OptionMenu_DrawTransitionTextShortCoords(4, 0, 0, g_optionStatusLine, (int)g_pGraphics->resX / 2, (int)g_pGraphics->resY / 2,
                 g_optionTextColours, g_optionTextBrightColours, 0x12);
}

// Fills pRect with `count` vertical strips shading from colour 0 to colour 1
// over the first half and from colour 1 to colour 2 over the second half
// (pColours: three RGBA colours; count must be at least 2). The strip widths
// are rounded so that together they cover the rectangle exactly.
// match 41%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0050ee50
void OptionMenu_DrawThreeColourGradientStrips(short *pRect, int unused, unsigned int count, BYTE *pColours)
{
    BYTE colour[4];
    short strip[4];
    FixVector c0;
    FixVector c1;
    FixVector d01;
    FixVector d12;
    FixVector c;
    int r;
    int g;
    int b;
    int n;
    int half;
    int i;
    int t;

    colour[0] = 0xff;
    colour[1] = 0xff;
    colour[2] = 0xff;
    colour[3] = 0xff;
    *(int *)&strip[0] = *(int *)&pRect[0];
    *(int *)&strip[2] = *(int *)&pRect[2];
    c0.x = pColours[0] << 16;
    c0.y = pColours[1] << 16;
    c0.z = pColours[2] << 16;
    c1.x = pColours[4] << 16;
    c1.y = pColours[5] << 16;
    c1.z = pColours[6] << 16;
    d01.x = c1.x - c0.x;
    d01.y = c1.y - c0.y;
    d01.z = c1.z - c0.z;
    d12.x = (pColours[8] << 16) - c1.x;
    d12.y = (pColours[9] << 16) - c1.y;
    n = count & 0xff;
    d12.z = (pColours[10] << 16) - c1.z;
    half = (count & 0xff) >> 1;
    for (i = 0, t = 0; i < n; i++, t += 0x10000) {
        strip[2] = (FixMul(t + 0x10000, FixDiv(pRect[2] << 16, n << 16)) >> 16) + pRect[0];
        strip[2] -= strip[0];
        if (i > half) {
            FixVecScale(&c, &d12, FixDiv(t - (half << 16), half << 16));
            c.x += c1.x;
            c.y += c1.y;
            c.z += c1.z;
        } else {
            FixVecScale(&c, &d01, FixDiv(i << 16, half << 16));
            c.x += c0.x;
            c.y += c0.y;
            c.z += c0.z;
        }
        r = c.x >> 16;
        if (r > 0xff)
            r = 0xff;
        else if (r < 0)
            r = 0;
        g = c.y >> 16;
        colour[0] = r;
        if (g > 0xff)
            g = 0xff;
        else if (g < 0)
            g = 0;
        b = c.z >> 16;
        colour[1] = g;
        if (b > 0xff)
            b = 0xff;
        else if (b < 0)
            b = 0;
        colour[2] = b;
        Sprite_FillRect(&g_pGraphics->field309_0x150, strip, colour, 3);
        strip[0] += strip[2];
    }
}

// GLOBAL: CMR2 0x0082b0a4
int g_unk0x0082b0a4;
// GLOBAL: CMR2 0x0082a938
BYTE g_unk0x0082a938;

CarSetup *RallyData_GetPlayerCarSetup(int index);

// GLOBAL: CMR2 0x00529670
char g_strFontGeneralHel15pt[17] = "general\\hel_15pt";
// GLOBAL: CMR2 0x00529684
char g_strFontGeneralHel12pt[17] = "general\\hel_12pt";
// GLOBAL: CMR2 0x00529698
char g_strFontGeneralDot[12] = "general\\dot";
// GLOBAL: CMR2 0x005296ac
char g_strFontGeneralHandel[15] = "general\\handel";

void Frontend_AccumulateDeviceKeyCounters(void);
CallbackStateMachine *Game_GetSecondaryCallbackMachine(void);
int Network_CloseSession(void);
void Game_RequestOptionRefresh(void);

// Starts the option menu: rebuilds the frontend, flags the option state, arms
// the fade timer and switches the grouped callback machine to level 2; when the
// menu is active and this is not the "restart" path it runs the transitions.
// FUNCTION: CMR2 0x005001c0
void OptionMenu_Start(char param1)
{
    CallbackStateMachine *p;

    Frontend_AccumulateDeviceKeyCounters();
    p = Game_GetSecondaryCallbackMachine();
    g_unk0x0082a938 = 1;
    g_unk0x0082b0a4 = CMain::GetFrameDelta();
    CGame::PromoteCallbackEntryByRule(p, 0, 0, 2);
    if (CGameInfo::GetGameModeOptionBit19() != '\0' && param1 == '\0') {
        CGame::DestroyLocalNetworkPlayer();
        Network_CloseSession();
        Game_RequestOptionRefresh();
    }
}

// Starts the fade of the option menu once more than 50 frames have passed.
// FUNCTION: CMR2 0x00501350
void OptionMenu_StartDelayedFade(CallbackStateMachine *param1, int unused)
{
    unsigned int delta = CMain::GetFrameDelta() - g_unk0x0082b0a4;

    if (delta > 0x32) {
        if (g_unk0x0082a938 != 0) {
            CGame::PromoteCallbackEntryByRule(param1, 0, 2, 2);
            return;
        }
        CGame::PromoteCallbackEntryByRule(param1, 0, 0, 2);
    }
}

// Menu colour the boot screens fade to/from, r/g/b in its low three bytes.
// GLOBAL: CMR2 0x0052704c
int g_menuFadeColour = 0x00acb49c;

int Game_PrepareScene(SceneNode *pRoot, SceneNode *pCamera, void *pRect, int param);
int Game_DrawSceneViewport(struct SceneNode *pRoot, struct SceneNode *pCamera, void *pRect, int bit, BYTE flag);
void Graphics_PresentFrameAndResetCounters(void);

// Clears the screen to a colour fading from the stored menu colour to grey over
// 50 frames, then draws the option menu scene.
// Option menu states 0x5010a0/0x501130/0x5012e0/0x5015d0 (cascade seed batch 1, verified 94-97%).
void OptionMenu_CommitRallyRecords(void);
void OptionMenu_StoreCarSetups(void);
/* ===== cascade: menu states (drafts, iterated against the scaffold) ===== */
void OptionMenu_SetBackgroundViewport(void);
void Graphics_PresentFrameAndResetCounters(void);
void OptionPreview_SetWheelNodesVisible(int index, char visible);
int Game_PrepareScene(SceneNode *pRoot, SceneNode *pCamera, void *pRect, int param);
void OptionMenu_DrawActivePage(void);
int Game_DrawSceneViewport(struct SceneNode *pRoot, struct SceneNode *pCamera, void *pRect, int bit, BYTE flag);


// FUNCTION: CMR2 0x005010a0
void OptionMenu_LeaveAndCommitState(CallbackStateMachine *p1, BYTE state)
{
    CSound::CloseMusicStreamAndClearPath(0);
    CGraphics::SetClearColour(1, 0x9c, 0xb4, 0xac);
    CGraphics::ClearTarget();
    CGraphics::ClearZBuffer();
    OptionMenu_SetBackgroundViewport();
    Graphics_PresentFrameAndResetCounters();
    OptionMenu_CommitRallyRecords();
    OptionMenu_StoreCarSetups();
    CGame::UnwindCallbacks(g_unk0x0082af88);
    CGraphics::EvictManagedTextureResources();
    CGraphics::FreeTextureBuffers();
    CGame::PromoteCallbackEntryByRule(p1, state, 0, 2);
    if (g_unk0x0082b0ac != 0) {
        CGame::SetFrontendResourceMode(0);
        CGameInfo::SetInputAndGamePaused(0);
        return;
    }
    CGame::SetFrontendResourceMode(3);
    CGameInfo::SetInputAndGamePaused(0);
}

// FUNCTION: CMR2 0x00501130
void OptionMenu_LeaveWithoutCommitState(CallbackStateMachine *p1, BYTE state)
{
    CSound::CloseMusicStreamAndClearPath(0);
    CGraphics::SetClearColour(1, 0x9c, 0xb4, 0xac);
    CGraphics::ClearTarget();
    CGraphics::ClearZBuffer();
    OptionMenu_SetBackgroundViewport();
    Graphics_PresentFrameAndResetCounters();
    CGame::UnwindCallbacks(g_unk0x0082af88);
    CGraphics::EvictManagedTextureResources();
    CGraphics::FreeTextureBuffers();
    CGame::PromoteCallbackEntryByRule(p1, state, 0, 2);
    CGame::SetFrontendResourceMode(0);
    CGameInfo::SetInputAndGamePaused(0);
}

// FUNCTION: CMR2 0x005012e0
void OptionMenu_UpdateHiddenPreviewState(CallbackStateMachine *p1, BYTE state)
{
    int i;

    if (g_unk0x0082af8c != 0) {
        CMain::UpdateFrameTime();
        g_unk0x0082b0a4 = CMain::GetFrameDelta();
        g_unk0x0082af8c = 0;
    }
    for (i = 0; i < (int)(CGameInfo::GetConfiguredPlayerCount() & 0xff); i = i + 1) {
        OptionPreview_SetWheelNodesVisible(i, 0);
    }
    if (0x32 < (unsigned int)(CMain::GetFrameDelta() - g_unk0x0082b0a4))
        CGame::PromoteCallbackEntryByRule(p1, 0, 0, 2);
}

// FUNCTION: CMR2 0x005015d0
void OptionMenu_DrawBackgroundState(CallbackStateMachine *p1, BYTE state)
{
    short rect[4];

    rect[0] = 0;
    rect[1] = 0;
    rect[2] = (short)((int)g_pGraphics->resX * 2 / 3);
    rect[3] = (short)((int)g_pGraphics->resY * 2 / 3);
    CGraphics::ClearTarget();
    CGraphics::ClearZBuffer();
    OptionMenu_SetPreviewProjection();
    OptionMenu_DrawActivePage();
    Game_PrepareScene(g_optionMenuSceneRoot, g_optionMenuCamera, &rect[0], 0);
    Game_DrawSceneViewport(g_optionMenuSceneRoot, g_optionMenuCamera, &rect[0], 0, 1);
    Graphics_PresentFrameAndResetCounters();
}

// FUNCTION: CMR2 0x00501780
void OptionMenu_FadeBackgroundToGrey(int param1, int unused)
{
    short rect[4];
    BYTE colour[4];
    int t;
    int inv;

    rect[0] = 0;
    rect[1] = 0;
    rect[2] = (short)((int)g_pGraphics->resX * 2 / 3);
    rect[3] = (short)((int)g_pGraphics->resY * 2 / 3);
    t = (CMain::GetFrameDelta() * 1000 - g_unk0x0082b0a4 * 1000) / 50;
    inv = 1000 - t;
    // Fades between the menu colour and the grey (141, 151, 159) used by
    // OptionMenu_InitBackground while the level is built.
    colour[0] = (BYTE)(((g_menuFadeColour & 0xff) * inv + 141 * t) / 1000);
    colour[1] = (BYTE)(((g_menuFadeColour >> 8 & 0xff) * inv + 151 * t) / 1000);
    colour[2] = (BYTE)(((g_menuFadeColour >> 16 & 0xff) * inv + 159 * t) / 1000);
    CGraphics::SetClearColour(1, colour[0], colour[1], colour[2]);
    CGraphics::ClearTarget();
    CGraphics::ClearZBuffer();
    OptionMenu_SetPreviewProjection();
    // The globals are the root and camera scene nodes; the third argument is a
    // temporary 4-short rectangle covering the middle of the screen.
    Game_PrepareScene(g_optionMenuSceneRoot, g_optionMenuCamera, rect, 0);
    Game_DrawSceneViewport(g_optionMenuSceneRoot, g_optionMenuCamera, rect, 0, 1);
    Graphics_PresentFrameAndResetCounters();
}

// Same as OptionMenu_FadeBackgroundToGrey but the colour fades from grey to the stored menu colour.
// The extra GetFrameDelta() result is discarded by the original.
// FUNCTION: CMR2 0x00501920
void OptionMenu_FadeToStoredColour(int param1, int unused)
{
    short rect[4];
    BYTE colour[4];
    int t;

    rect[0] = 0;
    rect[1] = 0;
    rect[2] = (short)((int)g_pGraphics->resX * 2 / 3);
    rect[3] = (short)((int)g_pGraphics->resY * 2 / 3);
    t = (CMain::GetFrameDelta() * 1000 - g_unk0x0082b0a4 * 1000) / 50;
    colour[0] = (BYTE)((((g_menuFadeColour & 0xff) - 141) * t + 141000) / 1000);
    colour[1] = (BYTE)((((g_menuFadeColour >> 8 & 0xff) - 151) * t + 151000) / 1000);
    colour[2] = (BYTE)((((g_menuFadeColour >> 16 & 0xff) - 159) * t + 159000) / 1000);
    CGraphics::SetClearColour(1, colour[0], colour[1], colour[2]);
    CGraphics::ClearTarget();
    CGraphics::ClearZBuffer();
    OptionMenu_SetPreviewProjection();
    CMain::GetFrameDelta();
    Game_PrepareScene(g_optionMenuSceneRoot, g_optionMenuCamera, rect, 0);
    Game_DrawSceneViewport(g_optionMenuSceneRoot, g_optionMenuCamera, rect, 0, 1);
    Graphics_PresentFrameAndResetCounters();
}

// Copies the option records back into the rally data (undo of OptionMenu_LoadRallyRecords).
// FUNCTION: CMR2 0x00502db0
void OptionMenu_CommitRallyRecords(void)
{
    RallyCarDamageState *pSrc;
    RallyCarDamageState *pDst;
    int i;
    int j;

    i = 0;
    if ((BYTE)CGameInfo::GetConfiguredPlayerCount() > 0) {
        pSrc = g_playerOptionCache.records;
        do {
            pDst = RallyData_GetDriverGroupRecord(i);
            i++;
            memcpy(pDst, pSrc, sizeof(RallyCarDamageState));
            pSrc++;
        } while (i < (int)(CGameInfo::GetConfiguredPlayerCount() & 0xff));
    }
}

// Copies the default option values from the global table into each record.
// FUNCTION: CMR2 0x005029b0
void OptionMenu_StoreCarSetups(void)
{
    int i;
    CarSetup *pDst;
    for (i = 0; i < 4; i++) {
        pDst = RallyData_GetPlayerCarSetup(i);
        pDst->brakeBias = g_optionCarSetups[i].brakeBias;
        pDst->brakePower = g_optionCarSetups[i].brakePower;
        pDst->gearbox = g_optionCarSetups[i].gearbox;
        pDst->steering = g_optionCarSetups[i].steering;
        pDst->powerRatio = g_optionCarSetups[i].powerRatio;
        pDst->suspension = g_optionCarSetups[i].suspension;
        pDst->tyres = g_optionCarSetups[i].tyres;
    }
}

// Applies the selected option: advances the menu when its value is set, or
// starts the fade otherwise.
// FUNCTION: CMR2 0x005000b0
void OptionMenu_AdvanceSelectedOption(int unused, int unused2)
{
    OptionMenu_FillValueSlider();
    if (OptionMenu_IsSlotValueAboveBase(CGameInfo::GetActiveOptionSlot(), OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 1)].max) != 0) {
        Menu_SetNextAction(OptionMenu_GetValueSliderMenu());
        return;
    }
    CGameInfo::StartOptionMenuTimeout();
}

// Loads the four option menu fonts.
// FUNCTION: CMR2 0x0050f370
void OptionMenu_LoadFonts(void)
{
    Font_InitTable(4);
    Font_Load(g_strFontGeneralHel15pt, OptionMenu_GetCommonArchive(), 0);
    Font_Load(g_strFontGeneralHel12pt, OptionMenu_GetCommonArchive(), 1);
    Font_Load(g_strFontGeneralDot, OptionMenu_GetCommonArchive(), 2);
    Font_Load(g_strFontGeneralHandel, OptionMenu_GetCommonArchive(), 3);
}

// Sets the whole mesh list of the record to opaque when the record has content
// and the per-mesh flag is set, to fully transparent otherwise.
// FUNCTION: CMR2 0x00509150
void OptionPreview_SetMeshOpacity(int index)
{
    OptionPreviewMeshRecord *pRecord = &g_previewMeshRecords[index];
    int i;
    if (pRecord->verticesAssigned != 0) {
        for (i = 0; i < pRecord->meshCount; ++i) {
            if (pRecord->meshVisible[i] != 0)
                pRecord->geometry.nodes[i]->viewMask = 0xff;
            else
                pRecord->geometry.nodes[i]->viewMask = 0;
        }
    }
}

/* ===== integrated from casc/s6 ===== */
// Textures of the option menu (symbols, banners and car parts).
// GLOBAL: CMR2 0x00831360
Texture *g_symbolDanRedTexture;
// GLOBAL: CMR2 0x00831364
Texture *g_symbolDanOrangeTexture;
// GLOBAL: CMR2 0x00831368
Texture *g_symbolDanYellowTexture;
// GLOBAL: CMR2 0x00831668
Texture *g_countryBannerTexture;
// GLOBAL: CMR2 0x0083166c
Texture *g_arrow640ATexture;
// GLOBAL: CMR2 0x00831670
Texture *g_arrow640DTexture;
// GLOBAL: CMR2 0x008313ac
Texture *g_symbolTickTexture;
// GLOBAL: CMR2 0x00831648
Texture *g_symbolCrossTexture;
// GLOBAL: CMR2 0x008313b0
Texture *g_symbolBoxTexture;
// GLOBAL: CMR2 0x00831674
Texture *g_symbolCircleTexture;
// GLOBAL: CMR2 0x0083137c
Texture *g_carPartTextures[12];
// Country banner codes, in banner order (the last one is CFrontend::m_strUK).
// Names of the car part textures.
// GLOBAL: CMR2 0x00529590
char g_str0x00529590[8] = "AXLES";
// GLOBAL: CMR2 0x00529598
char g_str0x00529598[8] = "DRIVE";
// GLOBAL: CMR2 0x005295a0
char g_str0x005295a0[8] = "EXHAUST";
// GLOBAL: CMR2 0x005295a8
char g_str0x005295a8[8] = "ELEC";
// GLOBAL: CMR2 0x005295b0
char g_str0x005295b0[8] = "STEER";
// GLOBAL: CMR2 0x005295b8
char g_str0x005295b8[8] = "BODY";
// GLOBAL: CMR2 0x005295c0
char g_str0x005295c0[8] = "BRAKES";
// GLOBAL: CMR2 0x005295c8
char g_str0x005295c8[8] = "DIFFER";
// GLOBAL: CMR2 0x005295d0
char g_str0x005295d0[8] = "SUSP";
// GLOBAL: CMR2 0x005295d8
char g_str0x005295d8[8] = "TURBO";
// GLOBAL: CMR2 0x005295e0
char g_str0x005295e0[8] = "GEAR";
// GLOBAL: CMR2 0x005295e8
char g_str0x005295e8[8] = "TYRES";
// GLOBAL: CMR2 0x0052956c
char g_str0x0052956c[] = "%s\\Textures\\Symbols\\%d\\DanRed.tga";
// GLOBAL: CMR2 0x00529548
char g_str0x00529548[] = "%s\\Textures\\Symbols\\%d\\DanOra.tga";
// GLOBAL: CMR2 0x00529524
char g_str0x00529524[] = "%s\\Textures\\Symbols\\%d\\DanYel.tga";
// GLOBAL: CMR2 0x00529508
char g_str0x00529508[] = "%s\\Textures\\Banners\\b%s.tga";
// GLOBAL: CMR2 0x005294f0
char g_str0x005294f0[] = "%s\\Textures\\Ar_640A.tga";
// GLOBAL: CMR2 0x005294d8
char g_str0x005294d8[] = "%s\\Textures\\Ar_640D.tga";
// GLOBAL: CMR2 0x005294b8
char g_str0x005294b8[] = "%s\\Textures\\Symbols\\%d\\Tick.tga";
// GLOBAL: CMR2 0x00529494
char g_str0x00529494[] = "%s\\Textures\\Symbols\\%d\\Cross.tga";
// GLOBAL: CMR2 0x00529474
char g_str0x00529474[] = "%s\\Textures\\Symbols\\%d\\Box.tga";
// GLOBAL: CMR2 0x00529450
char g_str0x00529450[] = "%s\\Textures\\Symbols\\%d\\Circle.tga";
// GLOBAL: CMR2 0x00529434
char g_str0x00529434[] = "%s\\Textures\\Parts\\%d\\%s.tga";

// Loads the option menu textures: the three "Dan" symbols, the country
// banner, the 640 arrows, the tick/cross/box/circle symbols and one texture
// per car part.
// FUNCTION: CMR2 0x0050a080
void OptionMenu_LoadPartTextures(void)
{
    char *pBanners[8] = { g_str0x00519280, g_str0x0051927c, g_str0x00519278, g_str0x00519274,
                          g_str0x00519270, g_str0x0051926c, g_str0x00519268, CFrontend::m_strUK };
    char *pParts[12] = { g_str0x005295e8, g_str0x005295e0, g_str0x005295d8, g_str0x005295d0,
                         g_str0x005295c8, g_str0x005295c0, g_str0x005295b8, g_str0x005295b0,
                         g_str0x005295a8, g_str0x005295a0, g_str0x00529598, g_str0x00529590 };
    int i;

    sprintf(CFrontend::m_stringDest, g_str0x0052956c, CInstallInfo::GetSetupRepDir(), 0x280);
    g_symbolDanRedTexture = CTexture::FindLoadTexture(OptionMenu_GetCommonArchive(), CFrontend::m_stringDest, 0, 0, 0, 0);
    sprintf(CFrontend::m_stringDest, g_str0x00529548, CInstallInfo::GetSetupRepDir(), 0x280);
    g_symbolDanOrangeTexture = CTexture::FindLoadTexture(OptionMenu_GetCommonArchive(), CFrontend::m_stringDest, 0, 0, 0, 0);
    sprintf(CFrontend::m_stringDest, g_str0x00529524, CInstallInfo::GetSetupRepDir(), 0x280);
    g_symbolDanYellowTexture = CTexture::FindLoadTexture(OptionMenu_GetCommonArchive(), CFrontend::m_stringDest, 0, 0, 0, 0);
    sprintf(CFrontend::m_stringDest, g_str0x00529508, CInstallInfo::GetSetupRepDir(),
            pBanners[RallyDataCountryIndex() & 0xff]);
    g_countryBannerTexture = CTexture::FindLoadTexture(OptionMenu_GetStageArchive(), CFrontend::m_stringDest, 0, 0, 0, 0);
    sprintf(CFrontend::m_stringDest, g_str0x005294f0, CInstallInfo::GetFrontendDir());
    g_arrow640ATexture = CTexture::FindLoadTexture(OptionMenu_GetCommonArchive(), CFrontend::m_stringDest, 0, 0, 0, 0);
    sprintf(CFrontend::m_stringDest, g_str0x005294d8, CInstallInfo::GetFrontendDir());
    g_arrow640DTexture = CTexture::FindLoadTexture(OptionMenu_GetCommonArchive(), CFrontend::m_stringDest, 0, 0, 0, 0);
    sprintf(CFrontend::m_stringDest, g_str0x005294b8, CInstallInfo::GetSetupRepDir(), 0x280);
    g_symbolTickTexture = CTexture::FindLoadTexture(OptionMenu_GetCommonArchive(), CFrontend::m_stringDest, 0, 0, 0, 0);
    sprintf(CFrontend::m_stringDest, g_str0x00529494, CInstallInfo::GetSetupRepDir(), 0x280);
    g_symbolCrossTexture = CTexture::FindLoadTexture(OptionMenu_GetCommonArchive(), CFrontend::m_stringDest, 0, 0, 0, 0);
    sprintf(CFrontend::m_stringDest, g_str0x00529474, CInstallInfo::GetSetupRepDir(), 0x280);
    g_symbolBoxTexture = CTexture::FindLoadTexture(OptionMenu_GetCommonArchive(), CFrontend::m_stringDest, 0, 0, 0, 0);
    sprintf(CFrontend::m_stringDest, g_str0x00529450, CInstallInfo::GetSetupRepDir(), 0x280);
    g_symbolCircleTexture = CTexture::FindLoadTexture(OptionMenu_GetCommonArchive(), CFrontend::m_stringDest, 0, 0, 0, 0);
    for (i = 0; i < 12; i++) {
        sprintf(CFrontend::m_stringDest, g_str0x00529434, CInstallInfo::GetSetupRepDir(), 0x280, pParts[i]);
        g_carPartTextures[i] = CTexture::FindLoadTexture(OptionMenu_GetCommonArchive(), CFrontend::m_stringDest,
                                                            0, 0, 0, 0);
    }
}

// Per-index block of the option menu's 3D preview (0x138 bytes each).
// Geometry of the option menu preview deform (0x138 bytes per record, 8 records).
// GLOBAL: CMR2 0x0082cb78
OptionPreviewSceneRecord g_previewSceneRecords[16];

void OptionPreview_AssignConvertedVertexBuffers(int param1, INT_PTR param2, OptionPreviewMeshRecord *param3);

// Rebuilds the mesh record of the option menu preview model: clears the
// record, adds the mesh of every node of the entry's node tree, compacts the
// empty slots and matches the converted vertex buffers.
// match 85%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00507080
void OptionPreview_RebuildMeshRecord(int index)
{
    OptionPreviewMeshRecord *pRecord;
    OptionPreviewSceneRecord *pEntry;
    SceneNode *pNode;
    SceneNode *pSibling;
    int i;
    int j;
    int moved;

    pRecord = &g_previewMeshRecords[index];
    memset(pRecord, 0, sizeof(OptionPreviewMeshRecord));
    pEntry = &g_previewSceneRecords[index];
    pRecord->meshCount = 0;
    if (pEntry->bodyNode == 0)
        return;
    pRecord->minZ = 0;
    pRecord->maxZ = 0;
    pRecord->minX = 0;
    pRecord->maxX = 0;
    OptionPreview_AddNodeMeshToRecord(pEntry->bodyNode, pRecord);
    pNode = pEntry->bodyNode->pFirstChild;
    if (pNode != 0) {
        do {
            pSibling = pNode;
            if ((BYTE)pNode->flags != 0x14 && pNode != 0) {
                do {
                    OptionPreview_AddNodeMeshToRecord(pNode, pRecord);
                    pNode = pNode->pFirstChild;
                } while (pNode != 0);
            }
            pNode = pSibling->pNext;
        } while (pNode != 0);
    }
    for (i = 0; i < 15; i++) {
        if (pRecord->geometry.nodes[i] != 0)
            continue;
        moved = 0;
        if (i < 14) {
            for (j = i; j < 14; j++) {
                if (pRecord->geometry.nodes[j] != 0 || pRecord->geometry.nodes[j + 1] != 0)
                    moved = 1;
                pRecord->geometry.meshes[j] = pRecord->geometry.meshes[j + 1];
                pRecord->geometry.nodes[j] = pRecord->geometry.nodes[j + 1];
                pRecord->geometry.vertices[j] = pRecord->geometry.vertices[j + 1];
                pRecord->vertexCount[j] = pRecord->vertexCount[j + 1];
                pRecord->centre[j] = pRecord->centre[j + 1];
                pRecord->halfSize[j] = pRecord->halfSize[j + 1];
            }
            if (moved)
                i--;
        }
    }
    OptionPreview_AssignConvertedVertexBuffers(index, (INT_PTR)pEntry->bodyNode, pRecord);
}

// GLOBAL: CMR2 0x0082fd00
OptionPreviewDeformGeometry g_previewDeformGeometry[8];


// Timestamp of the previous frame of the option menu preview animation.
// GLOBAL: CMR2 0x0082d118
unsigned int g_previewPreviousAnimationTime;

// Animates the four preview nodes of the option record: each one gets an
// identity (or mirrored) basis, and while the menu is fading the forward
// vector is shrunk to 0x9999.
// FUNCTION: CMR2 0x00509dc0
void OptionPreview_AnimatePreviewNodeTransforms(int index)
{
    OptionPreviewDeformGeometry *pList;
    OptionPreviewMeshRecord *pRec;
    FixBasis mirror;
    FixBasis basis;
    FixVector v;
    unsigned int elapsed;
    unsigned int now;
    int i;
    int value;

    pList = &g_previewDeformGeometry[index];
    pRec = &g_previewMeshRecords[index];
    basis.right.x = 0x10000;
    basis.right.y = 0;
    basis.right.z = 0;
    basis.up.x = 0;
    basis.up.y = 0x10000;
    basis.up.z = 0;
    basis.forward.x = 0;
    basis.forward.y = 0;
    basis.forward.z = 0x10000;
    mirror.right.x = -0x10000;
    mirror.right.y = 0;
    mirror.right.z = 0;
    mirror.up.x = 0;
    mirror.up.y = 0x10000;
    mirror.up.z = 0;
    mirror.forward.x = 0;
    mirror.forward.y = 0;
    mirror.forward.z = -0x10000;
    if (g_previewPreviousAnimationTime == 0) {
        g_previewPreviousAnimationTime = Sys_GetTicks();
        elapsed = 0;
    } else {
        now = Sys_GetTicks();
        elapsed = now - g_previewPreviousAnimationTime;
        g_previewPreviousAnimationTime = now;
        elapsed = FixDiv(elapsed << 16, 0x280000);
        elapsed = FixMul(0xa0000, elapsed);
    }
    value = pList->animationPhaseDegrees + elapsed;
    pList->animationPhaseDegrees = value;
    if (value > 0x1680000)
        pList->animationPhaseDegrees = value - 0x1680000;
    for (i = 0; i < 4; i++) {
        // the original discards this result
        value = pRec->partDamageFractions[i];
        value = FixMul(0x50000, value);
        if (i % 2 == 0) {
            FixMatrix_SetRight(&basis.right, &pList->pEntry->wheelNodes[i]->current);
            FixMatrix_SetUp(&basis.up, &pList->pEntry->wheelNodes[i]->current);
            FixMatrix_SetForward(&basis.forward, &pList->pEntry->wheelNodes[i]->current);
        } else {
            FixMatrix_SetRight(&mirror.right, &pList->pEntry->wheelNodes[i]->current);
            FixMatrix_SetUp(&mirror.up, &pList->pEntry->wheelNodes[i]->current);
            FixMatrix_SetForward(&mirror.forward, &pList->pEntry->wheelNodes[i]->current);
        }
        if ((OptionPreview_UsesWheelVariantSix(pList->pEntry->modelClass, (char)OptionMenu_GetCarSetupChoice(index, 0)) != 0 && CGameInfo::GetPreviewMode() == 2) ||
            CGameInfo::GetConfiguredPlayerCount() > 1) {
            FixMatrix_GetForward(&v, &pList->pEntry->wheelNodes[i]->current);
            FixVecScale(&v, &v, 0x9999);
            FixMatrix_SetForward(&v, &pList->pEntry->wheelNodes[i]->current);
        }
    }
}

// Normalises a vector and flips it when it points down.
#define OPTIONS_FIX_NORMALIZE_FLIP(v)                                               \
    {                                                                               \
        int len = FixVecLength(&v);                                                 \
        if (len == 0) {                                                             \
            v.x = 0;                                                                \
            v.y = 0;                                                                \
            v.z = 0;                                                                \
        } else {                                                                    \
            FixVecScaleRecip(&v, &v, len);                                          \
            if (v.y < 0)                                                            \
                FixVecScale(&v, &v, -0x10000);                                      \
        }                                                                           \
    }

// Builds the preview node's transform from the four corner heights stored in
// the option record (field_0x22c): the up axis is the sum of the two edge
// normals, the other axes come from Gram-Schmidt and the position from the
// average height.
// FUNCTION: CMR2 0x005091c0
void OptionPreview_BuildCornerHeightTransform(int index)
{
    OptionPreviewMeshRecord *pRec;
    OptionPreviewDeformGeometry *pList;
    int f[4];
    FixVector v;
    FixVector v2;
    FixVector cross1;
    FixVector cross2;
    FixVector c[3];
    FixVector *p0;
    FixVector *p2;
    FixBasis basis;
    int a;
    int b;
    int len;
    int i;
    int t;

    pRec = &g_previewMeshRecords[index];
    pList = &g_previewDeformGeometry[index];
    if (OptionMenu_FindFirstDetailedSlot() == 1 && OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 1)].max == 2) {
        t = (unsigned char)OptionMenu_GetValueSliderMenu()->items[0].max << 16;
        t = FixMul(t, 0x1999);
    } else {
        t = (int)(char)OptionMenu_GetCarSetupChoice(index, 2) << 16;
        t = FixMul(t, 0x1999);
    }
    t = FixMul(0x10000 - t, 0x1999);
    for (i = 0; i < 4; i++) {
        f[i] = pRec->cornerHeightOffsets[i] + t;
        if (f[i] < -0x1999)
            f[i] = -0x1999;
    }
    a = FixMul(0x8000, f[2] + f[3]);
    b = FixMul(0x8000, f[0] + f[1]);
    // The corners are copied whole (y included) before taking the differences.
    p0 = &pList->corner[0];
    c[0] = *p0;
    c[1] = pList->corner[1];
    p2 = &pList->corner[2];
    c[2] = *p2;
    v.x = c[1].x - c[0].x;
    v.y = f[1] - f[0];
    v.z = c[1].z - c[0].z;
    v2.x = c[2].x - c[0].x;
    v2.y = a - f[0];
    v2.z = -c[0].z;
    FixVecCross(&cross1, &v, &v2);
    OPTIONS_FIX_NORMALIZE_FLIP(cross1);
    c[0] = *p2;
    c[1] = pList->corner[3];
    c[2] = *p0;
    v.x = c[1].x - c[0].x;
    v.y = f[3] - f[2];
    v.z = c[1].z - c[0].z;
    v2.x = c[2].x - c[0].x;
    v2.y = b - f[2];
    v2.z = -c[0].z;
    FixVecCross(&cross2, &v, &v2);
    OPTIONS_FIX_NORMALIZE_FLIP(cross2);
    basis.right.x = 0x10000;
    basis.right.y = 0;
    basis.right.z = 0;
    basis.up.x = 0;
    basis.up.y = 0x10000;
    basis.up.z = 0;
    basis.forward.x = 0;
    basis.forward.y = 0;
    basis.forward.z = 0x10000;
    v.x = cross1.x + cross2.x;
    v.y = cross1.y + cross2.y;
    v.z = cross1.z + cross2.z;
    FIX_NORMALIZE_INTO(cross1, v)
    basis.up.x = cross1.x;
    basis.up.y = cross1.y;
    basis.up.z = cross1.z;
    len = FixVecDot(&basis.right, &basis.up);
    FixVecScale(&v, &basis.up, len);
    v.x = basis.right.x - v.x;
    v.y = basis.right.y - v.y;
    v.z = basis.right.z - v.z;
    FIX_NORMALIZE_INTO(basis.right, v)
    FixVecCross(&v, &basis.right, &basis.up);
    {
        FixVector *pForward = &basis.forward;

        len = FixVecLength(&v);
        if (len == 0) {
            pForward->x = 0;
            pForward->y = 0;
            pForward->z = 0;
        } else {
            FixVecScaleRecip(pForward, &v, len);
        }
    }
    t = FixMul(0x8000, b + a);
    v.x = 0;
    v.y = t;
    v.z = 0;
    pList->pEntry->bodyNode->useParentWorld = 0;
    FixMatrix_SetRight(&basis.right, &pList->pEntry->bodyNode->current);
    FixMatrix_SetUp(&basis.up, &pList->pEntry->bodyNode->current);
    FixMatrix_SetForward(&basis.forward, &pList->pEntry->bodyNode->current);
    FixMatrix_SetPosition(&v, &pList->pEntry->bodyNode->current);
}

/* ===== integrated from casc/s2 ===== */
// ---------------------------------------------------------------------------
// Per-slot preview tables used by wheel-mesh selection and orientation.
// ---------------------------------------------------------------------------

// Number of filled slots (1 or 2), from CGameInfo::GetPreviewLayoutMode.
// GLOBAL: CMR2 0x0082c694
int g_optionPreviewPanelCount;
// Texture set index of each slot (100 = no data); g_unk0x0082ca18 holds how
// many of them are valid.
// GLOBAL: CMR2 0x0082ca04
BYTE g_optionSlotTextureSets[0x14];
// GLOBAL: CMR2 0x0082ca18
BYTE g_unk0x0082ca18;

// 0x54-byte per-slot entry of the table at 0x82cb78: the scene node whose
// mesh is shown and the three wheel-mesh variants that can be assigned to it.

// One per preview slot (OptionPreview_LoadStageGeometryRecord), up to the
// next global at 0x82d19c.
// GLOBAL: CMR2 0x0082d15c
int g_previewWheelVariant[16];

// GLOBAL: CMR2 0x0082d0b8
PreviewSignedAngles g_previewRotationAngles[4];
// GLOBAL: CMR2 0x0082d0d8
PreviewSignedAngles g_previewRotationDeltas[4];
// GLOBAL: CMR2 0x0082d0f8
PreviewSignedAngles g_previewRotationStartAngles[4];
// GLOBAL: CMR2 0x0082fce0
PreviewSignedAngles g_previewRotationTargets[4];
// GLOBAL: CMR2 0x00831080
int g_previewRotationDuration;
// GLOBAL: CMR2 0x00831148
int g_previewRotationStartTimes[20];
// GLOBAL: CMR2 0x0083131c
BYTE g_previewRotationActive[4];
// Per-slot converted vertex buffer of the wheel meshes: each slot holds an
// array of one block per source mesh.
// All 16 preview slots are visited by session cleanup, including unused ones.
// GLOBAL: CMR2 0x00831198
DeformVertex **g_previewOriginalVertices[16];
// Per-slot table of source node types: the low byte of SceneNode::flags.
// GLOBAL: CMR2 0x0082d1dc
BYTE *g_previewOriginalNodeTypes[16];

int *RallyData_GetDriverPrimaryPairRecord(int index);
int *RallyData_GetDriverSettingPair(int index);
int *RallyData_GetDriverSecondaryPairRecord(int index);
BYTE OptionMenu_GetCarSetupChoice(int index, int type);
extern char g_strWheelVariantL[4];
extern char g_strWheelVariantN[4];
extern double g_fixedDegreesToAngle12;

// Fills the per-slot texture set indices (g_optionSlotTextureSets, counted by
// g_unk0x0082ca18) from the rally tables of the active slot; entries without
// a table value are marked 100.
// FUNCTION: CMR2 0x00505e70
void OptionPreview_FillTextureSetIndices(void)
{
    int *pStages;
    BYTE *pPairs;
    BYTE *pDest;
    int slot;
    int count;
    int i;
    int *pEntry;

    pStages = RallyData_GetDriverPrimaryPairRecord(0);
    pPairs = (BYTE *)RallyData_GetDriverSettingPair(0);
    // The original reads [index * 0x50 + 0x82c6c0]: byte 0x48 of the previous
    // 0x50-byte entry of the table at 0x82c6c8 (index is 1-based).
    count = g_optionPreviewPanels[g_optionPreviewPanelCount - 1].count;
    // 0x82c710 is byte 0x48 of the first panel, not independent storage.
    // It advances with the service day's first stage.
    slot = g_optionPreviewPanels[0].count;
    g_unk0x0082ca18 = count - slot + 2;
    pEntry = RallyData_GetDriverSecondaryPairRecord(slot);
    if (*pEntry != 0)
        g_optionSlotTextureSets[0] = 100;
    else
        g_optionSlotTextureSets[0] = pPairs[slot * 8];
    if (slot <= count) {
        i = slot;
        pDest = &g_optionSlotTextureSets[1];
        do {
            pEntry = RallyData_GetDriverSecondaryPairRecord(slot);
            if (*pEntry != 0)
                *pDest = 100;
            else
                *pDest = (BYTE)pStages[i];
            i++;
            pDest++;
        } while (i <= count);
    }
}

// Assigns one of the three wheel-mesh variants to the scene nodes of the slot
// according to the option tables, and when the textures do not come from the
// CD swaps the "L"/"N" variants of every wheel mesh triangle of the slot.
// Preserve the original byte-offset cursor while accessing the actual scene record.
#define PREVIEW_SCENE_AT_OFFSET(o) ((OptionPreviewSceneRecord *)((BYTE *)g_previewSceneRecords + (o)))

// FUNCTION: CMR2 0x00506080
void OptionPreview_UpdateWheelMeshVariants(int param1)
{
    int off;
    int i;
    int k;
    int w;
    Mesh *pMesh;
    Texture *pTex;

    off = param1 * sizeof(OptionPreviewSceneRecord);
    if (CGameInfo::GetPreviewMode() == 0) {
        if (g_previewWheelVariant[param1] != (int)(char)OptionMenu_GetCarSetupChoice(param1, 0)) {
            int *pVal = &g_previewWheelVariant[param1];
            *pVal = (int)(char)OptionMenu_GetCarSetupChoice(param1, 0);
            if (OptionPreview_UsesWheelVariantSix(PREVIEW_SCENE_AT_OFFSET(off)->modelClass, *pVal) != 0) {
                for (i = 0; i < 4; i++) {
                    if (PREVIEW_SCENE_AT_OFFSET(off)->sixWheelMeshes[i] != 0)
                        PREVIEW_SCENE_AT_OFFSET(off)->wheelNodes[i]->pObject = PREVIEW_SCENE_AT_OFFSET(off)->sixWheelMeshes[i];
                }
                return;
            }
            if (OptionPreview_UsesLowWheelVariant(PREVIEW_SCENE_AT_OFFSET(off)->modelClass, g_previewWheelVariant[param1]) != 0) {
                for (i = 0; i < 4; i++) {
                    if (PREVIEW_SCENE_AT_OFFSET(off)->lowWheelMeshes[i] != 0)
                        PREVIEW_SCENE_AT_OFFSET(off)->wheelNodes[i]->pObject = PREVIEW_SCENE_AT_OFFSET(off)->lowWheelMeshes[i];
                }
                return;
            }
            for (i = 0; i < 4; i++) {
                if (PREVIEW_SCENE_AT_OFFSET(off)->normalWheelMeshes[i] != 0)
                    PREVIEW_SCENE_AT_OFFSET(off)->wheelNodes[i]->pObject = PREVIEW_SCENE_AT_OFFSET(off)->normalWheelMeshes[i];
            }
            return;
        }
    } else {
        if (g_previewWheelVariant[param1] != (int)(char)OptionMenu_GetCarSetupChoice(param1, 0)) {
            g_previewWheelVariant[param1] = (int)(char)OptionMenu_GetCarSetupChoice(param1, 0);
            for (w = 0; w < 4; w++) {
                pMesh = (Mesh *)PREVIEW_SCENE_AT_OFFSET(off)->wheelNodes[0]->pObject;
                for (k = 0; k < pMesh->triangleCount; k++) {
                    pTex = CGraphics::m_pTextureManager->textureBuffer[pMesh->pTriangles[k].textures[0]];
                    if (OptionPreview_UsesLowWheelVariant(PREVIEW_SCENE_AT_OFFSET(off)->modelClass, g_previewWheelVariant[param1]) != 0) {
                        if (strncmp(pTex->name + strlen(pTex->name) - 9, g_strWheelVariantN, 1) == 0) {
                            strncpy(pTex->name + strlen(pTex->name) - 9, g_strWheelVariantL, 1);
                            Graphics_ReloadTexture(pTex);
                        }
                    } else if (strncmp(pTex->name + strlen(pTex->name) - 9, g_strWheelVariantL, 1) == 0) {
                        strncpy(pTex->name + strlen(pTex->name) - 9, g_strWheelVariantN, 1);
                        Graphics_ReloadTexture(pTex);
                    }
                }
            }
        }
    }
}

// Animates every slot's model rotation towards its target angles: slots whose
// animation time is over copy the target straight into the current angles, the
// rest write the animated angles scaled by the squared proportion of the
// elapsed time and hand them to the slot's scene node.
// FUNCTION: CMR2 0x00506720
void OptionPreview_AnimateModelRotations(void)
{
    unsigned int time;
    int i;

    time = CMain::GetFrameDelta();
    i = 0;
    if (CGameInfo::GetConfiguredPlayerCount() > 0) {
        do {
            unsigned int duration;
            PreviewSignedAngles *pAngles;

            duration = g_previewRotationDuration;
            if (g_previewRotationActive[i] != 0 && time - g_previewRotationStartTimes[i] > g_previewRotationDuration)
                g_previewRotationActive[i] = 0;
            pAngles = &g_previewRotationAngles[i];
            if (g_previewRotationActive[i] != 0) {
                unsigned int t = (time * 100 - g_previewRotationStartTimes[i] * 100) / duration;
                int k = (int)(t * t) / 100;

                pAngles->x = (short)(g_previewRotationDeltas[i].x * k / 100 + g_previewRotationStartAngles[i].x);
                pAngles->y = (short)(g_previewRotationDeltas[i].y * k / 100 + g_previewRotationStartAngles[i].y);
                pAngles->z = (short)(g_previewRotationDeltas[i].z * k / 100 + g_previewRotationStartAngles[i].z);
                OptionMenu_InterpolateLayoutPairs(i, 0, k);
            } else {
                pAngles->x = g_previewRotationTargets[i].x;
                pAngles->y = g_previewRotationTargets[i].y;
                pAngles->z = g_previewRotationTargets[i].z;
                OptionMenu_InterpolateLayoutPairs(i, 1, 0);
            }
            SceneNode_SetRotation(g_previewSceneRecords[i].rootNode, (FixAngles *)pAngles);
            OptionPreview_AnimatePreviewNodeTransforms(i);
            OptionPreview_BuildCornerHeightTransform(i);
            i++;
        } while (i < (CGameInfo::GetConfiguredPlayerCount() & 0xff));
    }
}

// Saves the rotation matrix of the slot's scene node, rotates the node to the
// given angles, rotates param2 by the new matrix into param3 and puts the old
// rotation matrix back.
// FUNCTION: CMR2 0x005068b0
void OptionPreview_RotateSlotVector(int param1, FixVector *param2, FixVector *param3, FixAngles *param4)
{
    SceneNode *pNode;
    FixMatrix saved;

    pNode = g_previewSceneRecords[param1].rootNode;
    saved = pNode->current;
    SceneNode_SetRotation(pNode, param4);
    FixMatrix_RotateVector(param3, param2, &g_previewSceneRecords[param1].rootNode->current);
    g_previewSceneRecords[param1].rootNode->current = saved;
}

// Stores the target 12-bit angles of a slot (signed 16-bit views) in the
// current tables and converts the delta to 16.16 degrees. Beyond +/-180 it
// reflects positive deltas around 360 or adds 360 to negative deltas once,
// then stores the result as signed 12-bit angle units for animation.
// match 60%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00506930
void OptionPreview_SetTargetModelAngles(int param1, FixAngles *param2, int param3)
{
    unsigned int time;
    short tX, tY, tZ;
    int d[3];
    int a[3];
    int abs;

    time = CMain::GetFrameDelta();
    g_previewRotationDuration = 0x14;
    tY = g_previewRotationAngles[param1].y;
    g_previewRotationStartTimes[param1] = time;
    tX = g_previewRotationAngles[param1].x;
    g_previewRotationActive[param1] = 1;
    tZ = g_previewRotationAngles[param1].z;
    g_previewRotationStartAngles[param1].x = tX;
    g_previewRotationStartAngles[param1].y = tY;
    g_previewRotationStartAngles[param1].z = tZ;
    g_previewRotationTargets[param1].x = (short)param2->x;
    g_previewRotationTargets[param1].y = (short)param2->y;
    g_previewRotationTargets[param1].z = (short)param2->z;
    if (param3 != 0) {
        g_previewRotationAngles[param1] = g_previewRotationTargets[param1];
        g_previewRotationStartAngles[param1] = g_previewRotationTargets[param1];
    }
    d[0] = g_previewRotationTargets[param1].x - g_previewRotationStartAngles[param1].x;
    d[1] = g_previewRotationTargets[param1].y - g_previewRotationStartAngles[param1].y;
    d[2] = g_previewRotationTargets[param1].z - g_previewRotationStartAngles[param1].z;
    a[0] = d[0] * 0x1680;
    a[1] = d[1] * 0x1680;
    a[2] = d[2] * 0x1680;
    abs = a[0];
    if (a[0] < 0)
        abs = -a[0];
    if (abs > 0xb40000) {
        if (a[0] >= 1)
            a[0] = 0x1680000 - a[0];
        else
            a[0] += 0x1680000;
    }
    abs = a[1];
    if (a[1] < 0)
        abs = -a[1];
    if (abs > 0xb40000) {
        if (a[1] > 0)
            a[1] = 0x1680000 - a[1];
        else
            a[1] += 0x1680000;
    }
    abs = a[2];
    if (a[2] < 0)
        abs = -a[2];
    if (abs > 0xb40000) {
        if (a[2] > 0)
            a[2] = 0x1680000 - a[2];
        else
            a[2] += 0x1680000;
    }
    g_previewRotationDeltas[param1].x = (short)FTOI(__int64, X87S_MUL((double)a[0], g_fixedDegreesToAngle12));
    g_previewRotationDeltas[param1].y = (short)FTOI(__int64, X87S_MUL((double)a[1], g_fixedDegreesToAngle12));
    g_previewRotationDeltas[param1].z = (short)FTOI(__int64, X87S_MUL((double)a[2], g_fixedDegreesToAngle12));
}

void SceneNode_SetViewMaskTree(SceneNode *pNode, BYTE mask);

// Shows or hides the wheel scene nodes of a slot: the root node and its
// bounding-box child receive the visibility mask, and showing them also
// re-applies the option state and the wheel mesh variants.
// FUNCTION: CMR2 0x00506b20
void OptionPreview_SetWheelNodesVisible(int index, char visible)
{
    if (visible != '\0') {
        SceneNode_SetViewMaskTree(g_previewSceneRecords[index].rootNode, 0xff);
        SceneNode_SetViewMaskTree(g_previewSceneRecords[index].bodyNode, 0xff);
        OptionPreview_SetMeshOpacity(index);
        OptionPreview_UpdateWheelMeshVariants(index);
        return;
    }
    SceneNode_SetViewMaskTree(g_previewSceneRecords[index].rootNode, 0);
    SceneNode_SetViewMaskTree(g_previewSceneRecords[index].bodyNode, 0);
}

// Allocates and fills the per-slot wheel vertex buffers of param3: the six
// floats of every source entry become 16.16 values and its normal is stored
// twice as three signed bytes biased by 0x80, the second copy scaled to unit
// length.
// match 57%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00506bb0
void OptionPreview_AllocateConvertedWheelVertices(int param1, INT_PTR param2, OptionPreviewMeshRecord *param3)
{
    DeformVertex ***ppBlock;
    BYTE **ppFlags;
    unsigned short *pCounts;
    FixVector normal;
    int totalSize;
    int i;
    int k;
    int srcOff;
    int destOff;
    int length;
    int v;
    DeformVertex *pDest;
    BYTE *pNormal;
    unsigned int n;
    BYTE nb[3];
    FixVector pos;

    ppBlock = &g_previewOriginalVertices[param1];
    *ppBlock = (DeformVertex **)CFileBuffer::AllocateLockedBuffer((unsigned int)param3->meshCount * sizeof(DeformVertex *));
    totalSize = (unsigned int)param3->meshCount << 2;
    ppFlags = &g_previewOriginalNodeTypes[param1];
    *ppFlags = (BYTE *)CFileBuffer::AllocateLockedBuffer((unsigned int)param3->meshCount);
    pCounts = param3->vertexCount;
    for (i = 0; i < (int)(param3->meshCount & 0xff); i++) {
        {
            (*ppBlock)[i] = (DeformVertex *)CFileBuffer::AllocateLockedBuffer((unsigned int)*pCounts << 5);
            totalSize += (unsigned int)*pCounts * 0x20;
            (*ppFlags)[i] = *(BYTE *)&param3->geometry.nodes[i]->flags;
            destOff = 0;
            srcOff = 0;
            for (k = 0; k < (int)(unsigned int)*pCounts; k++) {
                pDest = (DeformVertex *)((BYTE *)(*ppBlock)[i] + destOff);
                pDest->pos.x = (int)FTOI(__int64, X87S_MUL((double)((DeformFloatVertex *)((BYTE *)param3->geometry.meshes[i]->pVertexData + srcOff))->pos[0], CGraphics::m_65536));
                pDest->pos.y = (int)FTOI(__int64, X87S_MUL((double)((DeformFloatVertex *)((BYTE *)param3->geometry.meshes[i]->pVertexData + srcOff))->pos[1], CGraphics::m_65536));
                pDest->pos.z = (int)FTOI(__int64, X87S_MUL((double)((DeformFloatVertex *)((BYTE *)param3->geometry.meshes[i]->pVertexData + srcOff))->pos[2], CGraphics::m_65536));
                pDest->normal.x = (int)FTOI(__int64, X87S_MUL((double)((DeformFloatVertex *)((BYTE *)param3->geometry.meshes[i]->pVertexData + srcOff))->normal[0], CGraphics::m_65536));
                pDest->normal.y = (int)FTOI(__int64, X87S_MUL((double)((DeformFloatVertex *)((BYTE *)param3->geometry.meshes[i]->pVertexData + srcOff))->normal[1], CGraphics::m_65536));
                pDest->normal.z = (int)FTOI(__int64, X87S_MUL((double)((DeformFloatVertex *)((BYTE *)param3->geometry.meshes[i]->pVertexData + srcOff))->normal[2], CGraphics::m_65536));
                pos = pDest->pos;
                // Preserve BYTE writes of 0x81; signed member assignment changes scheduling.
                if (pos.x >= 0)
                    *(BYTE *)((BYTE *)pDest + offsetof(DeformVertex, rawNormal) + 0) = 0x81;
                else
                    pDest->rawNormal[0] = 0x7f;
                if (pos.y >= 0)
                    *(BYTE *)((BYTE *)pDest + offsetof(DeformVertex, rawNormal) + 1) = 0x81;
                else
                    pDest->rawNormal[1] = 0x7f;
                if (pos.z >= 0)
                    *(BYTE *)((BYTE *)pDest + offsetof(DeformVertex, rawNormal) + 2) = 0x81;
                else
                    pDest->rawNormal[2] = 0x7f;
                pNormal = (BYTE *)&((DeformFloatVertex *)((BYTE *)param3->geometry.meshes[i]->pVertexData + srcOff))->colour;
                n = *(unsigned int *)pNormal;
                nb[0] = (BYTE)(n >> 16);
                nb[2] = *pNormal;
                nb[1] = (BYTE)(n >> 8);
                v = (nb[0] & 0xff) - 0x80;
                if (v < -0x7f)
                    v = -0x7f;
                else if (v > 0x7f)
                    v = 0x7f;
                pDest->rawNormal[0] = (BYTE)v;
                v = (nb[1] & 0xff) - 0x80;
                if (v < -0x7f)
                    v = -0x7f;
                else if (v > 0x7f)
                    v = 0x7f;
                pDest->rawNormal[1] = (BYTE)v;
                v = (nb[2] & 0xff) - 0x80;
                if (v < -0x7f)
                    v = -0x7f;
                else if (v > 0x7f)
                    v = 0x7f;
                pDest->rawNormal[2] = (BYTE)v;
                normal.x = pDest->rawNormal[0] * -0x200;
                normal.y = pDest->rawNormal[1] * -0x200;
                normal.z = pDest->rawNormal[2] * -0x200;
                length = FixVecLength(&normal);
                if (length == 0) {
                    pDest->normal8[0] = 0;
                    pDest->normal8[1] = 0;
                    pDest->normal8[2] = 0;
                } else {
                    FixVecScaleRecip(&normal, &normal, length);
                    v = normal.x >> 9;
                    if (v >= 0x80)
                        v = 0x7f;
                    else if (v < -0x7f)
                        v = -0x7f;
                    pDest->normal8[0] = (BYTE)v;
                    v = normal.y >> 9;
                    if (v > 0x7f)
                        v = 0x7f;
                    else if (v < -0x7f)
                        v = -0x7f;
                    pDest->normal8[1] = (BYTE)v;
                    v = normal.z >> 9;
                    if (v > 0x7f)
                        v = 0x7f;
                    else if (v < -0x7f)
                        v = -0x7f;
                    pDest->normal8[2] = (BYTE)v;
                }
                srcOff += 0x30;
                destOff += 0x20;
            }
        }
        pCounts++;
    }
}

/* ===== integrated from casc/s4 ===== */

// Preview deformation shares the random scaling constant with GameInfo lots.
extern float g_oneOverRandMax;

// Preview dent parameters (16.16): advance, falloff width and scale,
// set by the historically named OptionPreview_ApplyDecodedImpact.
extern int g_previewDentRadius;  // defined in StageTiming.cpp
extern int g_previewDentFalloff;  // defined in StageTiming.cpp
extern int g_previewDentScale;  // defined in StageTiming.cpp

void StageDeform_ClampVertex(int *pPosition, int meshIndex, int vertexIndex, DeformMeshSources *pRecord);

void CarDamage_ApplyCameraSpaceMeshDent(OptionPreviewMeshRecord *pRecord);
void OptionPreview_RebuildDeformedGeometry(OptionPreviewMeshRecord *pObject, OptionPreviewDeformGeometry *pGeom);

// Replays one packed dent on a populated preview model with attached sources.
// Mode 0 uses the control-point plane; mode 1 uses the decoded axis radius;
// other modes use a fixed axis radius.
// FUNCTION: CMR2 0x005078e0
void OptionPreview_ApplyDecodedImpact(int index)
{
    int scale;

    if (g_previewMeshRecords[index].meshCount == 0)
        return;
    if (g_previewMeshRecords[index].verticesAssigned == 0)
        return;
    switch (g_previewDentMode) {
    case 0:
        g_previewDentRadius = FixMul(g_previewDentStrength, 0x5999);
        g_previewDentFalloff = FixMul(g_previewDentStrength, 0x9999);
        g_previewDentScale = FixMul(g_previewDentStrength, 0xb333);
        OptionPreview_RebuildDeformedGeometry(&g_previewMeshRecords[index], &g_previewDeformGeometry[index]);
        return;
    case 1:
        scale = FixMul(g_previewDentStrength, 0x8000);
        if (scale > 0x4000)
            scale = 0x4000;
        g_previewDentRadius = scale;
        g_previewDentFalloff = scale;
        g_previewDentScale = scale;
        CarDamage_ApplyCameraSpaceMeshDent(&g_previewMeshRecords[index]);
        return;
    default:
        g_previewDentAxisRadius = 0x4000;
        scale = FixMul(g_previewDentStrength, 0x8000);
        if (scale > g_previewDentAxisRadius)
            scale = g_previewDentAxisRadius;
        g_previewDentRadius = scale;
        g_previewDentFalloff = scale;
        g_previewDentScale = scale;
        CarDamage_ApplyCameraSpaceMeshDent(&g_previewMeshRecords[index]);
        return;
    }
}

// Builds the preview deformation box, twelve plane control points and four
// wheel anchors. Box extents depend on the model selection's archive class.
// FUNCTION: CMR2 0x00507a10
void OptionPreview_BuildDeformGeometry(OptionPreviewMeshRecord *pObject, int index)
{
    OptionPreviewDeformGeometry *pGeom;
    FixVector sizes;
    FixVector half;
    int i;
    int value;
    int offX0;
    int offX1;
    int offY0;
    int offY1;
    int offZ0;

    pGeom = &g_previewDeformGeometry[index];
    pGeom->pEntry = &g_previewSceneRecords[index];
    pGeom->animationPhaseDegrees = 0;
    for (i = 0; i < 4; i++) {
        FixMatrix_GetPosition(&pGeom->anchor[i],
                              &pGeom->pEntry->wheelNodes[i]->local);
        value = rand();
        pGeom->anchorPhaseDegrees[i] =
            FixMul((int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, value), g_oneOverRandMax), CGraphics::m_65536)), 0x1680000);
    }
    switch (CFrontend::GetArchivePrimaryIDEntry(RallyData_GetDriverRecordSelectionValue((BYTE)index))) {
    case 3:
        sizes.x = 0x44560;
        sizes.y = 0x15eb8;
        sizes.z = 0x1cfdf;
        offX0 = 0x1cccc;
        offX1 = 0x14ccc;
        offZ0 = 0x4ccc;
        offY0 = 0xb0a3;
        offY1 = 0xfa9f;
        break;
    case 0:
        sizes.x = 0x426e9;
        sizes.y = 0x16b85;
        sizes.z = 0x1c51e;
        offX0 = 0x1cccc;
        offX1 = 0x8000;
        offZ0 = 0x3d70;
        offY0 = 0xcf5c;
        offY1 = 0x10ccc;
        break;
    case 6:
        sizes.x = 0x3e3d7;
        sizes.y = 0x15eb8;
        sizes.z = 0x1c28f;
        offX0 = 0x1ae14;
        offX1 = 0x9c28;
        offZ0 = 0x4ccc;
        offY0 = 0xcf5c;
        offY1 = 0xfae1;
        break;
    case 2:
        sizes.x = 0x40f5c;
        sizes.y = 0x163d7;
        sizes.z = 0x1c51e;
        offX0 = 0x1cccc;
        offX1 = 0x451e;
        offZ0 = 0x4ccc;
        offY0 = 0xcf5c;
        offY1 = 0x1147a;
        break;
    case 7:
        sizes.x = 0x475c2;
        sizes.y = 0x1570a;
        sizes.z = 0x1c28f;
        offX0 = 0x1e147;
        offX1 = 0x1028f;
        offZ0 = 0x4ccc;
        offY0 = 0xcf5c;
        offY1 = 0xfae1;
        break;
    case 1:
        sizes.x = 0x4451e;
        sizes.y = 0x15999;
        sizes.z = 0x1d70a;
        offX0 = 0x1cccc;
        offX1 = 0xf851;
        offZ0 = 0x570a;
        offY0 = 0xcf5c;
        offY1 = 0x1147a;
        break;
    case 8:
        sizes.x = 0x30083;
        sizes.y = 0x14041;
        sizes.z = 0x18000;
        offX0 = 0x13333;
        offX1 = 0x4ccc;
        offZ0 = 0x2666;
        offY0 = 0xb5c2;
        offY1 = 0xfa9f;
        break;
    case 5:
        sizes.x = 0x41c28;
        sizes.y = 0x154bc;
        sizes.z = 0x1d47a;
        offX0 = 0x1c000;
        offX1 = 0x10000;
        offZ0 = 0x4ccc;
        offY0 = 0xe3d7;
        offY1 = 0x12dd2;
        break;
    case 4:
        sizes.x = 0x40312;
        sizes.y = 0x14ccc;
        sizes.z = 0x1c51e;
        offX0 = 0x1c000;
        offX1 = 0xcccc;
        offZ0 = 0x4ccc;
        offY0 = 0xe3d7;
        offY1 = 0x12dd2;
        break;
    case 9:
        sizes.x = 0x3b958;
        sizes.y = 0x15db2;
        sizes.z = 0x1e041;
        offX0 = 0x1a666;
        offX1 = 0x9999;
        offZ0 = 0x4ccc;
        offY0 = 0xe3d7;
        offY1 = 0xe106;
        break;
    case 11:
        sizes.x = 0x3d333;
        sizes.y = 0x15999;
        sizes.z = 0x1c312;
        offX0 = 0x1a666;
        offX1 = 0x9999;
        offZ0 = 0x4ccc;
        offY0 = 0xe3d7;
        offY1 = 0xe106;
        break;
    case 10:
        sizes.x = 0x3b333;
        sizes.y = 0x106a7;
        sizes.z = 0x1cf5c;
        offX0 = 0x1a666;
        offX1 = 0x13333;
        offZ0 = 0x4ccc;
        offY0 = 0xca3d;
        offY1 = 0xe106;
        break;
    case 12:
        sizes.x = 0x3ec49;
        sizes.y = 0x146a7;
        sizes.z = 0x1c28f;
        offX0 = 0x1a666;
        offX1 = 0x13333;
        offZ0 = 0x4ccc;
        offY0 = 0xca3d;
        offY1 = 0xe106;
        break;
    case 13:
        sizes.x = 0x41687;
        sizes.y = 0x16147;
        sizes.z = 0x1bb22;
        offX0 = 0x1ae14;
        offX1 = 0xfd70;
        offZ0 = 0x428f;
        offY0 = 0xd70a;
        offY1 = 0xf581;
        break;
    }
    FixVecScale(&half, &sizes, 0x8000);
    pGeom->corner[1].x = half.x;
    pGeom->corner[1].y = -half.y;
    pGeom->corner[1].z = -half.z;
    pGeom->corner[0].x = half.x;
    pGeom->corner[0].y = -half.y;
    pGeom->corner[0].z = half.z;
    pGeom->corner[2].x = -half.x;
    pGeom->corner[2].y = -half.y;
    pGeom->corner[2].z = half.z;
    pGeom->corner[3].x = -half.x;
    pGeom->corner[3].y = -half.y;
    pGeom->corner[3].z = -half.z;
    pGeom->corner[5].x = half.x;
    pGeom->corner[5].y = half.y;
    pGeom->corner[5].z = -half.z;
    pGeom->corner[4].x = half.x;
    pGeom->corner[4].y = half.y;
    pGeom->corner[4].z = half.z;
    pGeom->corner[6].x = -half.x;
    pGeom->corner[6].y = half.y;
    pGeom->corner[6].z = half.z;
    pGeom->corner[7].x = -half.x;
    pGeom->corner[7].y = half.y;
    pGeom->corner[7].z = -half.z;
    pGeom->vertex[0].x = pObject->maxX;
    pGeom->vertex[0].y = -half.y;
    pGeom->vertex[0].z = pObject->maxZ;
    pGeom->vertex[1].x = pObject->maxX;
    pGeom->vertex[1].y = -half.y;
    pGeom->vertex[1].z = pObject->minZ;
    pGeom->vertex[2].x = pObject->minX;
    pGeom->vertex[2].y = -half.y;
    pGeom->vertex[2].z = pObject->maxZ;
    pGeom->vertex[3].x = pObject->minX;
    pGeom->vertex[3].y = -half.y;
    pGeom->vertex[3].z = pObject->minZ;
    pGeom->vertex[4].x = pObject->maxX;
    pGeom->vertex[4].y = offY0 - half.y;
    pGeom->vertex[4].z = pObject->maxZ;
    pGeom->vertex[5].x = pObject->maxX;
    pGeom->vertex[5].y = offY0 - half.y;
    pGeom->vertex[5].z = pObject->minZ;
    pGeom->vertex[6].x = pObject->minX;
    pGeom->vertex[6].y = offY1 - half.y;
    pGeom->vertex[6].z = pObject->maxZ;
    pGeom->vertex[7].x = pObject->minX;
    pGeom->vertex[7].y = offY1 - half.y;
    pGeom->vertex[7].z = pObject->minZ;
    pGeom->vertex[8].x = half.x - offX0;
    pGeom->vertex[8].y = half.y;
    pGeom->vertex[8].z = offZ0 - half.z;
    pGeom->vertex[9].x = half.x - offX0;
    pGeom->vertex[9].y = half.y;
    pGeom->vertex[9].z = half.z - offZ0;
    pGeom->vertex[10].x = offX1 - half.x;
    pGeom->vertex[10].y = half.y;
    pGeom->vertex[10].z = half.z - offZ0;
    pGeom->vertex[11].x = offX1 - half.x;
    pGeom->vertex[11].y = half.y;
    pGeom->vertex[11].z = offZ0 - half.z;
}

// Projects the dent position onto the nearest control-point plane, then
// deforms preview vertices around it. The real clamp bounds displacement;
// the second float vector receives three times that displacement.
// FUNCTION: CMR2 0x00507fe0
void OptionPreview_RebuildDeformedGeometry(OptionPreviewMeshRecord *pObject, OptionPreviewDeformGeometry *pGeom)
{
    FixVector d;
    FixVector dv;
    FixVector pos;
    FixVector dest;
    FixVector saved;
    int positive;
    int minValue;
    int minPositive;
    int radius2;
    int invRadius;
    int band2;
    int invBand;
    int falloff;
    int value;
    int band;
    int angle;
    int dirty;
    int i;
    int j;

    FixVecScale(&d, &g_previewDentPosition, -0x10000);
    positive = FixVecDot(&g_previewDentNormal, &d) >= 0;
    minPositive = 0;
    minValue = 0;
    for (i = 0; i < 12; i++) {
        d.x = pGeom->vertex[i].x - g_previewDentPosition.x;
        d.y = pGeom->vertex[i].y - g_previewDentPosition.y;
        d.z = pGeom->vertex[i].z - g_previewDentPosition.z;
        value = FixVecDot(&g_previewDentNormal, &d);
        if (!positive)
            value = -value;
        if (value < minValue)
            minValue = value;
        if (value > 0 && (minPositive == 0 || value < minPositive))
            minPositive = value;
    }
    if (!positive) {
        minValue = -minValue;
        minPositive = -minPositive;
    }
    if (minValue != 0) {
        FixVecScale(&d, &g_previewDentNormal, minValue);
        g_previewDentPosition.x += d.x;
        g_previewDentPosition.y += d.y;
        g_previewDentPosition.z += d.z;
    } else if (minPositive != 0) {
        FixVecScale(&d, &g_previewDentNormal, minPositive);
        g_previewDentPosition.x += d.x;
        g_previewDentPosition.y += d.y;
        g_previewDentPosition.z += d.z;
    }
    positive = FixVecDot(&g_previewDentNormal, &g_previewDentPosition) >= 0;
    radius2 = FixMul(g_previewDentRadius, g_previewDentRadius);
    invRadius = FixDiv(0x10000, g_previewDentRadius);
    band = g_previewDentRadius + g_previewDentFalloff;
    band2 = FixMul(band, band);
    invBand = FixDiv(0x10000, g_previewDentFalloff);
    falloff = FixMul(g_previewDentScale, 0x3333);
    for (j = 0; j < pObject->meshCount; j++) {
        dirty = 0;
        for (i = 0; i < pObject->vertexCount[j]; i++) {
            pos.x = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)pObject->geometry.meshes[j]->pVertexData)[i].pos[0], CGraphics::m_65536));
            pos.y = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)pObject->geometry.meshes[j]->pVertexData)[i].pos[1], CGraphics::m_65536));
            pos.z = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)pObject->geometry.meshes[j]->pVertexData)[i].pos[2], CGraphics::m_65536));
            dv.x = g_previewDentPosition.x - pos.x;
            dv.y = g_previewDentPosition.y - pos.y;
            dv.z = g_previewDentPosition.z - pos.z;
            value = FixVecDot(&dv, &g_previewDentNormal);
            value = FixMul(value, value);
            if (value > band2)
                continue;
            saved = pos;
            if (value <= radius2) {
                value = g_previewDentRadius - FixMul(invRadius, value);
                FixVecScale(&dv, &g_previewDentNormal, value);
                if (positive) {
                    pos.x -= dv.x;
                    pos.y -= dv.y;
                    pos.z -= dv.z;
                } else {
                    pos.x += dv.x;
                    pos.y += dv.y;
                    pos.z += dv.z;
                }
            } else {
                value = FixMul(FixSqrt(value) - g_previewDentRadius, invBand);
                value = FixMul(value, falloff);
                angle = dv.x + dv.z;
                if (angle < 0)
                    angle = -angle;
                angle &= 0xffffff80;
                angle %= 1024;
                angle <<= 6;
                if (angle < 0x8000)
                    angle -= 0x10000;
                value = FixMul(value, angle);
                dest.x = (int)(signed char)pObject->geometry.vertices[j][i].normal8[0] << 9;
                dest.y = (int)(signed char)pObject->geometry.vertices[j][i].normal8[1] << 9;
                dest.z = (int)(signed char)pObject->geometry.vertices[j][i].normal8[2] << 9;
                FixVecScale(&dv, &dest, value);
                pos.x += dv.x;
                pos.y += dv.y;
                pos.z += dv.z;
            }
            StageDeform_ClampVertex(&pos.x, j, i, &pObject->geometry);
            dest.x = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)pObject->geometry.meshes[j]->pVertexData)[i].normal[0], CGraphics::m_65536));
            dest.y = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)pObject->geometry.meshes[j]->pVertexData)[i].normal[1], CGraphics::m_65536));
            dest.z = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)pObject->geometry.meshes[j]->pVertexData)[i].normal[2], CGraphics::m_65536));
            d.x = pos.x - saved.x;
            d.y = pos.y - saved.y;
            d.z = pos.z - saved.z;
            FixVecScale(&d, &d, 0x30000);
            dest.x += d.x;
            dest.y += d.y;
            dest.z += d.z;
            ((DeformFloatVertex *)pObject->geometry.meshes[j]->pVertexData)[i].normal[0] =
                (float)(X87S_MUL(dest.x, CGraphics::m_oneOver65536));
            ((DeformFloatVertex *)pObject->geometry.meshes[j]->pVertexData)[i].normal[1] =
                (float)(X87S_MUL(dest.y, CGraphics::m_oneOver65536));
            ((DeformFloatVertex *)pObject->geometry.meshes[j]->pVertexData)[i].normal[2] =
                (float)(X87S_MUL(dest.z, CGraphics::m_oneOver65536));
            dirty = 1;
        }
        if (dirty && pObject->geometry.nodes[j]->pObject != 0)
            Mesh_Rebuild((Mesh *)pObject->geometry.nodes[j]->pObject);
    }
}
// Copies the converted vertex buffers of the slot into its per-mesh pointer
// array: for every source mesh it finds the first buffer whose stored flag
// matches the mesh's flag and points the mesh entry at it.
// param2 (unused) is the preview's body node. Typed SceneNode * or void * it
// flips a load-order tie of the packed-rectangle sum in
// OptionMenu_DrawResultsRallyInfo; reordering that sum then flips a register
// tie in FrontendText_LoadRegionLanguages. Only this form keeps both exact.
// FUNCTION: CMR2 0x00506fc0
void OptionPreview_AssignConvertedVertexBuffers(int param1, INT_PTR param2, OptionPreviewMeshRecord *param3)
{
    int i;
    int j;
    int match;
    int target;

    OptionPreview_AllocateConvertedWheelVertices(param1, param2, param3);
    for (i = 0; i < param3->meshCount; i++) {
        j = 0;
        match = -1;
        target = param3->geometry.nodes[i]->flags & 0xff;
        for (; j <= param3->meshCount; j++) {
            if (g_previewOriginalNodeTypes[param1][j] == target) {
                match = j;
                j = param3->meshCount;
            }
        }
        if (match >= 0)
            param3->geometry.vertices[i] = g_previewOriginalVertices[param1][match];
    }
    param3->verticesAssigned = 1;
}

// Returns one entry of the pointer table used by the mode selection screens.
// FUNCTION: CMR2 0x004d06f0
char *NetworkChat_GetLine(int index)
{
    return g_networkChatLines[index];
}

// Steps the shared mode value one position; when it reaches the top it stores
// the frame time instead and flags the fade as finished.
// match 79%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00500130
void OptionMenu_AdvanceSharedModeValue(void)
{
    CallbackStateMachine *p = Game_GetSecondaryCallbackMachine();
    int value = OptionMenu_GetPlayerSlotOffset() - 1;

    if (value != 0) {
        CGame::PromoteCallbackEntryByRule(p, 0, 1, 2);
    } else {
        value = 1;
        CGame::PromoteCallbackEntryByRule(p, 0, 0, 2);
        g_unk0x0082a938 = 0;
        g_unk0x0082b0a4 = CMain::GetFrameDelta();
    }
    OptionMenu_SetPlayerSlotOffset(value);
}

// Starts the animation of the shared value towards one (or towards zero when it
// is already at one).
// FUNCTION: CMR2 0x00503960
void OptionMenu_ToggleValueAnimation(int param_1, int param_2)
{
    FixInterp *p;

    if (g_selectedOptionPreviewPanel == 0xff)
        return;
    p = &g_optionPreviewPanels[(signed char)g_selectedOptionPreviewPanel];
    if (param_1 != 0) {
        if (p->end != 0x10000)
            FixInterp_StartToOne(p);
        return;
    }
    if (param_2 != 0) {
        if (p->end != 0) {
            FixInterp_StartToZero(p);
            return;
        }
    } else {
        if (p->end == 0) {
            FixInterp_StartToOne(p);
            return;
        }
        if (p->end == 0x10000)
            FixInterp_StartToZero(p);
    }
}

// Stores one option value of a slot into the working table and, the first time
// the slot is touched, adds the option's weight to its interpolation distance.
// FUNCTION: CMR2 0x00502670
void OptionMenu_SetCarSetupChoice(int param_1, int param_2, char param_3)
{
    int distance;

    switch (param_2) {
    case 0:
        g_optionCarSetups[param_1].tyres = param_3;
        break;
    case 1:
        g_optionCarSetups[param_1].gearbox = param_3;
        break;
    case 6:
        g_optionCarSetups[param_1].steering = param_3 * 10;
        break;
    case 2:
        g_optionCarSetups[param_1].suspension = param_3 * 10;
        break;
    case 3:
        g_optionCarSetups[param_1].powerRatio = param_3 * 10;
        break;
    case 4:
        g_optionCarSetups[param_1].brakeBias = param_3 * 10;
        break;
    case 5:
        g_optionCarSetups[param_1].brakePower = param_3 * 10;
        break;
    }
    if (g_optionColumnDirty[param_1][param_2] == 0) {
        distance = OptionMenu_GetSlotSelector(param_1);
        OptionMenu_SetSlotSelector(param_1, distance - g_optionColumnBaseWeights[param_2]);
        g_optionColumnDirty[param_1][param_2] = 1;
    }
}

// Restores one option value of a slot from the default table and subtracts the
// option's weight from its interpolation distance.
// FUNCTION: CMR2 0x00502790
void OptionMenu_RevertCarSetupValue(int param_1, int param_2)
{
    int distance;

    switch (param_2) {
    case 0:
        g_optionCarSetups[param_1].tyres = g_optionCarSetups[param_1 + 4].tyres;
        break;
    case 1:
        g_optionCarSetups[param_1].gearbox = g_optionCarSetups[param_1 + 4].gearbox;
        break;
    case 6:
        g_optionCarSetups[param_1].steering = g_optionCarSetups[param_1 + 4].steering;
        break;
    case 2:
        g_optionCarSetups[param_1].suspension = g_optionCarSetups[param_1 + 4].suspension;
        break;
    case 3:
        g_optionCarSetups[param_1].powerRatio = g_optionCarSetups[param_1 + 4].powerRatio;
        break;
    case 4:
        g_optionCarSetups[param_1].brakeBias = g_optionCarSetups[param_1 + 4].brakeBias;
        break;
    case 5:
        g_optionCarSetups[param_1].brakePower = g_optionCarSetups[param_1 + 4].brakePower;
        break;
    }
    OptionMenu_SetSlotSelector(param_1, OptionMenu_GetSlotSelector(param_1) + g_optionColumnBaseWeights[param_2]);
    g_optionColumnDirty[param_1][param_2] = 0;
}

// Tells whether a slot option still holds the given value (options 2..6 store
// the value multiplied by ten).
// FUNCTION: CMR2 0x00502a00
BYTE OptionMenu_IsEntryCarSetupChoice(int param_1, int param_2, int param_3)
{
    switch (param_2) {
    case 0:
        return g_optionCarSetups[param_1 + 4].tyres == param_3;
    case 1:
        return g_optionCarSetups[param_1 + 4].gearbox == param_3;
    case 6:
        return g_optionCarSetups[param_1 + 4].steering == param_3 * 10;
    case 2:
        return g_optionCarSetups[param_1 + 4].suspension == param_3 * 10;
    case 3:
        return g_optionCarSetups[param_1 + 4].powerRatio == param_3 * 10;
    case 4:
        return g_optionCarSetups[param_1 + 4].brakeBias == param_3 * 10;
    case 5:
        return g_optionCarSetups[param_1 + 4].brakePower == param_3 * 10;
    }
    return 0;
}

// Runs the callback of every enabled slot: the table is 0x14 bytes per slot and
// each slot owns 8 of them.
void OptionPreview_LoadStageGeometryRecord(unsigned int);

// FUNCTION: CMR2 0x0050a3c0
void OptionPreview_UpdateAllSlots(void)
{
    int i = 0;

    if ((unsigned int)CGameInfo::GetConfiguredPlayerCount() > 0) {
        do {
            OptionPreview_LoadStageGeometryRecord(i);
            i++;
        } while (i < CGameInfo::GetConfiguredPlayerCount());
    }
}

// Moves the left or the right edge of the slot's layout rectangle towards the
// centre while its interpolation is running.
// FUNCTION: CMR2 0x00501de0
void OptionMenu_AnimateLayoutRectangleEdge(int param_1, short *param_2)
{
    Unk0x0082b2c0 *p;
    int lo;
    int hi;
    int a;
    int delta;

    p = &g_optionRecordTransitions[param_1];
    if (p->field_0xc == 1) {
        lo = param_2[2] / 2 + param_2[0];
        hi = lo + 1;
        lo--;
        if (p->field_0x0 < 0x8000) {
            param_2[0] = lo;
            param_2[2] = hi - lo;
            a = FixMul(p->field_0x0, 0x20000);
            a = FixMul(a, a);
            delta = param_2[3] - (FixMul(param_2[3] << 16, a) >> 16);
            param_2[1] += delta / 2;
            param_2[3] -= delta;
            return;
        }
        a = FixMul(p->field_0x0 - 0x8000, 0x20000);
        a = FixMul(a, a);
        delta = param_2[2] - (FixMul(param_2[2] << 16, a) >> 16);
        param_2[0] += delta / 2;
        param_2[2] -= delta;
        return;
    } else if (p->field_0xc == 0) {
        param_2[3] = 0;
        param_2[2] = 0;
    }
}

// Records the viewport of the active menu background (top-left origin and two
// thirds of the current resolution) in the shared rectangle.
// FUNCTION: CMR2 0x00501710
void OptionMenu_SetBackgroundViewport(void)
{
    short view[4];

    view[0] = 0;
    view[1] = 0;
    view[2] = (short)((int)g_pGraphics->resX * 2 / 3);
    view[3] = (short)((int)g_pGraphics->resY * 2 / 3);
    Game_DrawSceneViewport(g_optionMenuSceneRoot, g_optionMenuCamera, view, 0, 0);
}

// Restarts the shared value animation from zero (unused callback parameters).
// FUNCTION: CMR2 0x00500100
void OptionMenu_ResetValueAnimation(int param1, int param2)
{
    OptionMenu_ToggleValueAnimation(0, 0);
}

// Colour pairs of the results screen: the second pointer (0x5297b4) starts two
// entries into the first table.
// GLOBAL: CMR2 0x005297ac
int g_unk0x005297ac[6] = { -11579569, 1078939471, -1, -11579569, 538976288, 0 };
// GLOBAL: CMR2 0x005297c4
char g_str0x005297c4[] = "%s: %.2d.00";

// Draws the four result rows of the screen: the X coordinate and the viewport
// are fixed, the rows advance 14 units each.
// FUNCTION: CMR2 0x0050fd30
void OptionMenu_DrawResultRows(int param_1)
{
    int x = (int)g_pGraphics->resX * 0x1c / 0x280;
    int y = 0x19a;
    int index = param_1 * 4 + 0xd9;
    int count = 4;

    do {
        OptionMenu_DrawTransitionTextShortCoords(5, 1, 1, CFrontend::GetTextString(index), x,
                     (int)g_pGraphics->resY * y / 0x1e0, g_unk0x005297ac,
                     g_unk0x005297ac + 2, 0x11);
        y += 0xe;
        index++;
    } while (--count);
}

// Startup (.CRT$XCU) initializer: caches the render resolution in the globals
// the 2D drawing code reads.
// FUNCTION: CMR2 0x0050fdd0
void __cdecl OptionMovie_InitResolution(void)
{
    g_unk0x00831c60 = g_pGraphics->resX;
    g_unk0x00831c64 = g_pGraphics->resY;
}

// Startup (.CRT$XCU) initializer registered in the startup table. The linker
// folds it into a jump to the identical initializer above (both are reached).
// FUNCTION: CMR2 0x0050fdc0
void __cdecl OptionMovie_InitResolutionAlias(void)
{
    OptionMovie_InitResolution();
}

// PORT: was a function pointer in MSVC's .CRT$XCU startup table; standard
// C++ dynamic initialisation runs it at startup.
static int s_resolutionInit = (OptionMovie_InitResolutionAlias(), 0);

// Draws the title of the screen: the text table string is formatted into the
// shared buffer, right aligned at 0x140/0x280 of the resolution.
// FUNCTION: CMR2 0x0050e1c0
void OptionMenu_DrawScreenTitle(void)
{
    int flag = 0;
    int width;

    if (CGameInfo::GetActiveOptionSlot() == 1) {
        if (CGameInfo::IsConfiguredMultiplayer() == 0)
            flag = 1;
    }
    sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x131));
    Font_Unused(CFrontend::m_stringDest, flag);
    width = Font_GetTextWidth(0, (BYTE *)CFrontend::m_stringDest);
    OptionMenu_DrawTransitionTextShortCoords(4, 0, 0, CFrontend::m_stringDest,
                 (int)g_pGraphics->resX * 0x140 / 0x280 - width / 2,
                 (int)g_pGraphics->resY * 0xf0 / 0x1e0, g_optionTextColours, g_optionTextBrightColours, 0x11);
}

// Moves the shared value one position up; clamps it to the option count.
// FUNCTION: CMR2 0x00500210
void OptionMenu_IncrementSharedValue(Menu *pMenu, char param)
{
    CallbackStateMachine *p;
    int value;

    if (param == 0)
        return;
    p = Game_GetSecondaryCallbackMachine();
    value = OptionMenu_GetPlayerSlotOffset() + 1;
    if (value > CGameInfo::GetConfiguredPlayerCount())
        value = CGameInfo::GetConfiguredPlayerCount();
    else
        CGame::PromoteCallbackEntryByRule(p, 0, 1, 2);
    OptionMenu_SetPlayerSlotOffset(value);
}

// Applies the option selected in the first item of the mode menu: restores the
// old value when it still matches, otherwise stores the new one.
// FUNCTION: CMR2 0x005003d0
void OptionMenu_ApplyFirstModeItem(Menu *pMenu, int param)
{
    int index = Menu_FindItem(OptionMenu_GetControlSetupMenu(), 1);
    int value = OptionMenu_GetColumnUnitId((OptionMenu_GetControlSetupMenu())->items[index].max);
    int option = pMenu->items[0].max;

    if (OptionMenu_GetColumnDirtyFlag(CGameInfo::GetActiveOptionSlot(), value) &&
        OptionMenu_IsEntryCarSetupChoice(CGameInfo::GetActiveOptionSlot(), value, option)) {
        OptionMenu_RevertCarSetupValue(CGameInfo::GetActiveOptionSlot(), value);
        Menu_SetNextAction(pMenu->pParent);
        return;
    }
    if (!OptionMenu_IsEntryCarSetupChoice(CGameInfo::GetActiveOptionSlot(), value, option)) {
        if (OptionMenu_GetSlotSelector(CGameInfo::GetActiveOptionSlot()) - OptionMenu_GetColumnBaseWeight(value) < 0 &&
            !OptionMenu_GetColumnDirtyFlag(CGameInfo::GetActiveOptionSlot(), value))
            return;
        OptionMenu_SetCarSetupChoice(CGameInfo::GetActiveOptionSlot(), value, option);
    }
    Menu_SetNextAction(pMenu->pParent);
}

// Colour pair used by the highlighted option rows and the value at 0x52737c.
// GLOBAL: CMR2 0x00527378
int g_unk0x00527378 = -11250490;
// GLOBAL: CMR2 0x0052737c
int g_unk0x0052737c = -1;
// X multiplier of the option menu layout (450/640 of the resolution).
// GLOBAL: CMR2 0x005293a0
int g_optionLayoutXScale = 0x1c2;

// Draws the four option rows of the menu's right column: the first one uses the
// normal font, the other three the highlighted one.
// FUNCTION: CMR2 0x0050a680
void OptionMenu_DrawRightColumn(void)
{
    OptionMenu_DrawTransitionTextShortCoords(3, 0, 0,
                 CFrontend::GetTextString(CFrontend::GetArchivePrimaryIDEntry(
                     RallyData_GetDriverRecordSelectionValue(CGameInfo::GetActiveOptionSlot())) * 4 + 0x4c),
                 g_optionLayoutXScale * (int)g_pGraphics->resX / 0x280,
                 (int)g_pGraphics->resY * 0xb4 / 0x1e0, g_optionTextColours, g_optionTextBrightColours, 0x11);
    OptionMenu_DrawTransitionTextShortCoords(3, 1, 0,
                 CFrontend::GetTextString(CFrontend::GetArchivePrimaryIDEntry(
                     RallyData_GetDriverRecordSelectionValue(CGameInfo::GetActiveOptionSlot())) * 4 + 0x4d),
                 g_optionLayoutXScale * (int)g_pGraphics->resX / 0x280,
                 (int)g_pGraphics->resY * 0xc2 / 0x1e0, g_optionTextColours, g_optionTextBrightColours, 0x11);
    OptionMenu_DrawTransitionTextShortCoords(3, 1, 0,
                 CFrontend::GetTextString(CFrontend::GetArchivePrimaryIDEntry(
                     RallyData_GetDriverRecordSelectionValue(CGameInfo::GetActiveOptionSlot())) * 4 + 0x4e),
                 g_optionLayoutXScale * (int)g_pGraphics->resX / 0x280,
                 (int)g_pGraphics->resY * 0xd0 / 0x1e0, g_optionTextColours, g_optionTextBrightColours, 0x11);
    OptionMenu_DrawTransitionTextShortCoords(3, 1, 0,
                 CFrontend::GetTextString(CFrontend::GetArchivePrimaryIDEntry(
                     RallyData_GetDriverRecordSelectionValue(CGameInfo::GetActiveOptionSlot())) * 4 + 0x4f),
                 g_optionLayoutXScale * (int)g_pGraphics->resX / 0x280,
                 (int)g_pGraphics->resY * 0xde / 0x1e0, g_optionTextColours, g_optionTextBrightColours, 0x11);
}

// GLOBAL: CMR2 0x0051a904
char g_str0x0051a904[] = "%.2d:%.2d";
// GLOBAL: CMR2 0x005295f0
char g_str0x005295f0[] = " (mins)";

// Draws the stage time of the results screen: the elapsed time clamped to one
// minute around the target, the time of the target and the "(mins)" suffix.
// match 88%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0050a3f0
void OptionMenu_DrawClampedStageTime(void)
{
    int value = OptionMenu_GetSlotSelector(CGameInfo::GetActiveOptionSlot());
    int target = OptionMenu_GetSharedValue();
    int shown;
    int width;
    int x;

    if (target > value) {
        shown = target - 6000;
        if (shown < value)
            shown = value;
    } else if (target < value) {
        shown = target + 6000;
        if (shown > value)
            shown = value;
    } else {
        shown = value;
    }
    OptionMenu_DrawTransitionTextShortCoords(4, 0, 0, CFrontend::GetTextString(0x90),
                 g_optionLayoutXScale * (int)g_pGraphics->resX / 0x280,
                 (int)g_pGraphics->resY * 0x5d / 0x1e0, g_optionTextColours, g_optionTextBrightColours, 0x11);
    sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x91),
            OptionMenu_GetSlotBaseSelector(CGameInfo::GetActiveOptionSlot()));
    OptionMenu_DrawTransitionTextShortCoords(4, 1, 0, CFrontend::m_stringDest,
                 g_optionLayoutXScale * (int)g_pGraphics->resX / 0x280,
                 (int)g_pGraphics->resY * 0x6d / 0x1e0, g_optionTextColours, g_optionTextBrightColours, 0x11);
    OptionMenu_SetSharedValue(shown);
    sprintf(CFrontend::m_stringDest, g_str0x0051a904, (shown / 100) / 0x3c, (shown / 100) % 0x3c);
    if (CGameInfo::IsOptionMenuTimeoutPulseOn() != 0)
        OptionMenu_DrawTransitionTextShortCoords(4, 3, 3, CFrontend::m_stringDest,
                     g_optionLayoutXScale * (int)g_pGraphics->resX / 0x280,
                     (int)g_pGraphics->resY * 0x96 / 0x1e0,
                     &g_unk0x00527378, &g_unk0x00527378, 0x11);
    else
        OptionMenu_DrawTransitionTextShortCoords(4, 3, 3, CFrontend::m_stringDest,
                     g_optionLayoutXScale * (int)g_pGraphics->resX / 0x280,
                     (int)g_pGraphics->resY * 0x96 / 0x1e0,
                     &g_unk0x0052737c, &g_optionTextBrightColours[1], 0x11);
    width = Font_GetTextWidth(3, (BYTE *)CFrontend::m_stringDest);
    x = g_optionLayoutXScale * (int)g_pGraphics->resX / 0x280 + width;
    sprintf(CFrontend::m_stringDest, g_str0x005295f0);
    if (CGameInfo::IsOptionMenuTimeoutPulseOn() != 0)
        OptionMenu_DrawTransitionTextShortCoords(4, 0, 0, CFrontend::m_stringDest, x,
                     (int)g_pGraphics->resY * 0x90 / 0x1e0,
                     &g_unk0x00527378, &g_unk0x00527378, 0x11);
    else
        OptionMenu_DrawTransitionTextShortCoords(4, 0, 0, CFrontend::m_stringDest, x,
                     (int)g_pGraphics->resY * 0x90 / 0x1e0,
                     &g_unk0x0052737c, &g_optionTextBrightColours[1], 0x11);
}

// 13-byte entry of the option record's 0x104-byte block at +0x0.
// GLOBAL: CMR2 0x005270b8
int g_unk0x005270b8[11] = { 0x17700, 0xbb80, 0xbb80, 0x11940, 0xea60, 0xea60,
                            0x14820, 0x8ca0, 0x5dc0, 0xbb80, 0xbb80 };

// Copies one group of fields of a rally data record into the working option
// record of the given index and adds the group's weight to its value; clears the
// group's "already applied" flag.
// match 39%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x005034f0
void OptionMenu_ApplyRallyRecordGroup(int param_1, int param_2)
{
    RallyCarDamageState *pDest = &g_playerOptionCache.records[param_1];
    RallyCarDamageState *pSrc = RallyData_GetDriverGroupRecord(param_1);
    int value;
    int i;

    switch (param_2) {
    case 1:
        pDest->damage.intensity[14] = pSrc->damage.intensity[14];
        OptionMenu_SetSlotSelector(param_1, OptionMenu_GetSlotSelector(param_1) + g_unk0x005270b8[0]);
        g_playerOptionCache.dirty[param_1][1] = 0;
        return;
    case 2:
        pDest->damage.intensity[15] = pSrc->damage.intensity[15];
        OptionMenu_SetSlotSelector(param_1, OptionMenu_GetSlotSelector(param_1) + g_unk0x005270b8[1]);
        g_playerOptionCache.dirty[param_1][2] = 0;
        return;
    case 3:
        pDest->damage.intensity[6] = pSrc->damage.intensity[6];
        pDest->damage.intensity[7] = pSrc->damage.intensity[7];
        pDest->damage.intensity[8] = pSrc->damage.intensity[8];
        pDest->damage.intensity[9] = pSrc->damage.intensity[9];
        OptionMenu_SetSlotSelector(param_1, OptionMenu_GetSlotSelector(param_1) + g_unk0x005270b8[2]);
        OptionPreview_UpdateCornerHeightOffsets(param_1);
        g_playerOptionCache.dirty[param_1][3] = 0;
        return;
    case 4:
        pDest->damage.intensity[16] = pSrc->damage.intensity[16];
        OptionMenu_SetSlotSelector(param_1, OptionMenu_GetSlotSelector(param_1) + g_unk0x005270b8[3]);
        g_playerOptionCache.dirty[param_1][4] = 0;
        return;
    case 5:
        pDest->damage.intensity[10] = pSrc->damage.intensity[10];
        pDest->damage.intensity[11] = pSrc->damage.intensity[11];
        pDest->damage.intensity[12] = pSrc->damage.intensity[12];
        pDest->damage.intensity[13] = pSrc->damage.intensity[13];
        OptionMenu_SetSlotSelector(param_1, OptionMenu_GetSlotSelector(param_1) + g_unk0x005270b8[4]);
        g_playerOptionCache.dirty[param_1][5] = 0;
        return;
    case 6:
        pDest->damage.intensity[17] = pSrc->damage.intensity[17];
        pDest->damage.intensity[18] = pSrc->damage.intensity[18];
        pDest->damage.intensity[19] = pSrc->damage.intensity[19];
        pDest->damage.intensity[20] = pSrc->damage.intensity[20];
        pDest->damage.intensity[21] = pSrc->damage.intensity[21];
        pDest->damage.intensity[22] = pSrc->damage.intensity[22];
        pDest->damage.intensity[26] = pSrc->damage.intensity[26];
        pDest->damage.intensity[27] = pSrc->damage.intensity[27];
        pDest->damage.intensity[28] = pSrc->damage.intensity[28];
        pDest->damage.intensity[29] = pSrc->damage.intensity[29];
        pDest->damage.intensity[30] = pSrc->damage.intensity[30];
        pDest->damage.intensity[31] = pSrc->damage.intensity[31];
        pDest->damage.intensity[32] = pSrc->damage.intensity[32];
        pDest->damage.intensity[33] = pSrc->damage.intensity[33];
        for (i = 0; i < 3; i++)
            pDest->damage.lineGrounded[i] = pSrc->damage.lineGrounded[i];
        for (i = 0; i < 4; i++)
            pDest->damage.partHidden[i] = pSrc->damage.partHidden[i];
        pDest->impacts.count = pSrc->impacts.count;
        pDest->impacts.firstLink = pSrc->impacts.firstLink;
        for (i = 0; i < 20; i++) {
            pDest->impacts.links[i].impact.strength = pSrc->impacts.links[i].impact.strength;
            pDest->impacts.links[i].impact.mode = pSrc->impacts.links[i].impact.mode;
            pDest->impacts.links[i].impact.axisRadius = pSrc->impacts.links[i].impact.axisRadius;
            pDest->impacts.links[i].impact.normal[0] = pSrc->impacts.links[i].impact.normal[0];
            pDest->impacts.links[i].impact.normal[1] = pSrc->impacts.links[i].impact.normal[1];
            pDest->impacts.links[i].impact.normal[2] = pSrc->impacts.links[i].impact.normal[2];
            pDest->impacts.links[i].impact.axis[0] = pSrc->impacts.links[i].impact.axis[0];
            pDest->impacts.links[i].impact.axis[1] = pSrc->impacts.links[i].impact.axis[1];
            pDest->impacts.links[i].impact.axis[2] = pSrc->impacts.links[i].impact.axis[2];
            pDest->impacts.links[i].impact.position[0] = pSrc->impacts.links[i].impact.position[0];
            pDest->impacts.links[i].impact.position[1] = pSrc->impacts.links[i].impact.position[1];
            pDest->impacts.links[i].impact.position[2] = pSrc->impacts.links[i].impact.position[2];
            pDest->impacts.links[i].next = pSrc->impacts.links[i].next;
        }
        OptionMenu_SetSlotSelector(param_1, OptionMenu_GetSlotSelector(param_1) + g_unk0x005270b8[5]);
        g_playerOptionCache.dirty[param_1][6] = 0;
        OptionPreview_ApplyStoredDamage(param_1);
        return;
    case 7:
        pDest->damage.intensity[4] = pSrc->damage.intensity[4];
        pDest->damage.intensity[5] = pSrc->damage.intensity[5];
        OptionMenu_SetSlotSelector(param_1, OptionMenu_GetSlotSelector(param_1) + g_unk0x005270b8[6]);
        g_playerOptionCache.dirty[param_1][7] = 0;
        return;
    case 8:
        pDest->damage.intensity[23] = pSrc->damage.intensity[23];
        OptionMenu_SetSlotSelector(param_1, OptionMenu_GetSlotSelector(param_1) + g_unk0x005270b8[7]);
        g_playerOptionCache.dirty[param_1][8] = 0;
        return;
    case 9:
        pDest->damage.intensity[24] = pSrc->damage.intensity[24];
        OptionMenu_SetSlotSelector(param_1, OptionMenu_GetSlotSelector(param_1) + g_unk0x005270b8[8]);
        g_playerOptionCache.dirty[param_1][9] = 0;
        return;
    case 10:
        pDest->damage.intensity[25] = pSrc->damage.intensity[25];
        OptionMenu_SetSlotSelector(param_1, OptionMenu_GetSlotSelector(param_1) + g_unk0x005270b8[9]);
        g_playerOptionCache.dirty[param_1][10] = 0;
        return;
    case 11:
        pDest->damage.intensity[0] = pSrc->damage.intensity[0];
        pDest->damage.intensity[1] = pSrc->damage.intensity[1];
        pDest->damage.intensity[2] = pSrc->damage.intensity[2];
        pDest->damage.intensity[3] = pSrc->damage.intensity[3];
        OptionMenu_SetSlotSelector(param_1, OptionMenu_GetSlotSelector(param_1) + g_unk0x005270b8[10]);
        OptionPreview_UnpackPartDamageFractions(param_1);
        g_playerOptionCache.dirty[param_1][11] = 0;
    }
}

// Applies the second item of the mode menu when its value still differs from the
// stored option; the parent menu is always left as the next action.
// FUNCTION: CMR2 0x00500360
void OptionMenu_ApplySecondModeItem(Menu *pMenu, MenuItem *pItem)
{
    int index = Menu_FindItem(OptionMenu_GetControlSetupMenu(), 2);

    if (OptionMenu_GetRecordGroupAppliedFlag(CGameInfo::GetActiveOptionSlot(),
                     OptionMenu_GetColumnWeight((OptionMenu_GetControlSetupMenu())->items[index].max))) {
        OptionMenu_ApplyRallyRecordGroup(CGameInfo::GetActiveOptionSlot(),
                     OptionMenu_GetColumnWeight((OptionMenu_GetControlSetupMenu())->items[index].max));
    }
    Menu_SetNextAction(pMenu->pParent);
}

// Callback of the option menu: applies the highlighted item when the confirm
// button was pressed this frame.
// FUNCTION: CMR2 0x004ff630
void OptionMenu_ConfirmHighlightedItem(Menu *pMenu)
{
    int slot = 0;
    int index;
    int value;
    int option;

    if (CGameInfo::GetActiveOptionSlot() == 1) {
        if (CGameInfo::IsConfiguredMultiplayer() == 0)
            slot = 1;
    }
    if ((CInput::GetAvailableDeviceRecord(slot)->pressed & 0x20) == 0)
        return;
    if (pMenu->cursor == Menu_FindItem(pMenu, 0)) {
        OptionMenu_ToggleValueAnimation(0, 1);
        return;
    }
    if (pMenu->cursor == Menu_FindItem(pMenu, 2)) {
        index = Menu_FindItem(pMenu, 2);
        option = pMenu->items[index].max;
        if (OptionMenu_GetRecordGroupAppliedFlag(CGameInfo::GetActiveOptionSlot(), OptionMenu_GetColumnWeight(option)))
            OptionMenu_ApplyRallyRecordGroup(CGameInfo::GetActiveOptionSlot(), OptionMenu_GetColumnWeight(option));
    } else if (pMenu->cursor == Menu_FindItem(pMenu, 1)) {
        index = Menu_FindItem(pMenu, 1);
        value = pMenu->items[index].max;
        value = OptionMenu_GetColumnUnitId(value);
        if (OptionMenu_GetColumnDirtyFlag(CGameInfo::GetActiveOptionSlot(), value))
            OptionMenu_RevertCarSetupValue(CGameInfo::GetActiveOptionSlot(), value);
    }
}

#include "Sprite.h"

extern Texture *g_weatherSymbolTextures[9];
extern int g_weatherChoiceCount;
extern int g_weatherChoices[6];
extern BYTE g_splitTimeGradients[8];
extern int g_unk0x005270fc;
extern int g_unk0x00527100;
extern int g_unk0x005270b8[11];
void OptionMenu_DrawTransitionTextShortCoords(int index, int font1, int font2, char *text, short x, short y, int *pColour1,
                  int *pColour2, unsigned int flags);

// Vertical offsets of each weather symbol: the first table below 1024x768 (or
// without 1024 textures), the second otherwise.
// GLOBAL: CMR2 0x00527254
BYTE g_weatherSymbolYOffsetsLow[9] = { 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d };
// GLOBAL: CMR2 0x00527260
BYTE g_weatherSymbolYOffsetsHigh[9] = { 0x3a, 0x3a, 0x3b, 0x39, 0x39, 0x39, 0x3a, 0x3b, 0x3c };

// Draws the weather choices: a column of the g_weatherChoices symbols with
// their names, centred on an x that depends on the language and the country.
// FUNCTION: CMR2 0x005057e0
void OptionMenu_DrawWeatherChoices(void)
{
    SpriteRect dest;
    Texture *pTexture;
    int i;
    int width;
    int y;

    if (CGameInfo::GetGameLanguage() == 4)
        width = (BYTE)RallyDataCountryIndex() != 3 ? 0x139 : 0x148;
    else if (CGameInfo::GetGameLanguage() == 3)
        width = 0x140;
    else if (CGameInfo::GetGameLanguage() == 1 && (BYTE)RallyDataCountryIndex() == 3)
        width = 0x150;
    else
        width = 0x136;
    width = width * (int)g_pGraphics->resX / 0x280;
    pTexture = g_weatherSymbolTextures[0];
    dest.x = (short)width - pTexture->width / 2;
    dest.w = pTexture->width;
    dest.h = pTexture->height;
    for (i = 0; i < g_weatherChoiceCount; i++) {
        int num = 0xea0000;
        int scale = FixDiv(num, g_weatherChoiceCount * 0x10000 - 0x10000);
        int rowY = (FixMul(i << 16, scale) >> 16) + 0x93;
        OptionMenu_DrawTransitionTextShortCoords(3, 1, 1, CFrontend::GetTextString(g_weatherChoices[i] + 0x9d), width,
                     (int)g_pGraphics->resY * rowY / 0x1e0, &g_unk0x005270fc,
                     &g_unk0x00527100, 0x22);
        dest.y = (short)((int)g_pGraphics->resY * (rowY - 0x12) / 0x1e0);
        if (CGameInfo::GetScreenWidth() < 0x400 || !CFrontend::IsTextureWidthSupported(0x400) ||
            !CFrontend::IsTextureHeightSupported(0x400))
            dest.y = dest.y - g_weatherSymbolYOffsetsLow[g_weatherChoices[i]];
        else
            dest.y = dest.y - g_weatherSymbolYOffsetsHigh[g_weatherChoices[i]];
        pTexture = g_weatherSymbolTextures[g_weatherChoices[i]];
        Sprite_Queue((SpriteRect *)&pTexture->field_0x11c, &dest, pTexture, 3, 0, NULL, NULL,
                     g_splitTimeGradients, 8);
    }
}

// Draws the rows of an in-race menu: every visible row shows its label (and,
// for the value rows, each of its values) with the selected row brighter,
// plus the sprite behind the block.
// match 75%: implementada; misma lógica, MSVC elige &item+0xa como base del
// item (la original usa &item->id) y reordena el prólogo del rect.
// FUNCTION: CMR2 0x00402000
void InRaceMenu_DrawLabelAndValueRows(Menu *pMenu)
{
    MenuItem *pItem;
    int i;
    int j;
    int x;
    int y;
    char *str1;
    char *str2;
    char *sub[3];
    char *text;
    char **pA;
    char **pB;
    char single;
    bool twoLines;
    int *pColour;
    Texture *texture;

    g_inRaceMenuScratchRect[0] = 0;
    g_inRaceMenuScratchRect[1] = 0;
    g_inRaceMenuScratchRect[2] = (short)g_pGraphics->resX;
    y = (int)(g_pGraphics->resY * 0xaa) / 0x1e0;
    g_inRaceMenuScratchRect[3] = (short)g_pGraphics->resY;
    Font_SetBlendMode(2);
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_inRaceMenuScratchRect, (BYTE *)&g_unk0x0051608c, 2);
    InRaceMenu_DrawHeaderBar();
    g_inRaceMenuScratchRect[0] = (short)((int)(g_pGraphics->resX * 0x70) / 0x280);
    g_inRaceMenuScratchRect[2] = g_inRaceMenuUpArrowTexture->width;
    g_inRaceMenuScratchRect[3] = g_inRaceMenuUpArrowTexture->height;
    g_inRaceMenuScratchRect[1] = (short)((int)(g_pGraphics->resY * 0xaa) / 0x1e0
                                 - (int)(g_pGraphics->resY * 0xd) / 0x1e0);
    for (i = 0; i < pMenu->itemCount; i++) {
        pItem = &pMenu->items[i];
        if (pItem->visible) {
            str1 = CFrontend::GetTextString(pItem->id);
            str2 = CFrontend::GetTextString(pItem->id + 1);
            if (pMenu->cursor == i) {
                pColour = &g_inRaceMenuSelectedColour;
                texture = g_inRaceMenuUpArrowTexture;
            } else {
                pColour = &g_inRaceMenuUnselectedColour;
                texture = g_inRaceMenuDownArrowTexture;
            }
            Sprite_Queue((SpriteRect *)&texture->field_0x11c, (SpriteRect *)g_inRaceMenuScratchRect,
                         texture, 2, 0, NULL, NULL, (BYTE *)pColour, 8);
            twoLines = true;
            switch (pItem->value) {
            case 1:
                sub[0] = CFrontend::GetTextString(pItem->id);
                sub[1] = CFrontend::GetTextString(pItem->id + 1);
                single = 0;
                break;
            case 2:
                for (j = 0; j < (int)(BYTE)pItem->min; j++)
                    sub[j] = CFrontend::GetTextString(j + 0xa0);
                single = 0;
                break;
            case 4:
                for (j = 0; j < (int)(BYTE)pItem->min; j++)
                    sub[j] = CFrontend::GetTextString(j + 0x9e);
                /* the original falls through into the case below */
            case 0:
                single = 0;
                break;
            default:
                sub[0] = CFrontend::GetTextString(pItem->id);
                twoLines = false;
                break;
            }
            strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pItem->id));
            x = (int)(g_pGraphics->resX * 0x86) / 0x280;
            Font_DrawText(1, str1, x, y, pColour, 0x11);
            if (twoLines)
                Font_DrawText(0, str2, x, (int)(g_pGraphics->resY * 0xf) / 0x1e0 + y, pColour, 0x11);
            pB = sub + 1;
            pA = sub;
            for (j = 0; j < (int)(BYTE)pItem->min; j++) {
                text = *pB;
                if (single == 0)
                    text = *pA;
                x = (int)(g_pGraphics->resX * 10) / 0x280 + x
                    + Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
                Font_DrawText(1, text, x, y,
                              pItem->max != j ? &g_inRaceMenuUnselectedColour : &g_inRaceMenuSelectedColour, 0x11);
                strcpy(CFrontend::m_stringDest, text);
                pA++;
                pB--;
            }
            g_inRaceMenuScratchRect[1] = (short)(g_inRaceMenuScratchRect[1] + (int)(g_pGraphics->resY * 0x36) / 0x1e0);
            y = y + (int)(g_pGraphics->resY * 0x36) / 0x1e0;
        }
    }
    Font_SetBlendMode(2);
}

// GLOBAL: CMR2 0x0051607c
int g_unk0x0051607c = 0x80dbaca7;

// Draws the rows of a value menu: each row shows its label and, for the 0..3
// value rows, a slider bar (outline plus filled part) whose length is the
// value; the selected row is drawn brighter.
// match 46%: implementada; misma logica, MSVC ordena de otro modo el prologo,
// el maximo de anchos y las divisiones del slider (los dos rects en registros).
// match 48%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00403890
void InRaceMenu_DrawSliderRows(Menu *pMenu)
{
    MenuItem *pItem;
    short rect[4];
    short bar[4];
    int i;
    int k;
    int x;
    int y;
    int maxWidth;
    int width;
    int *pColour;
    char *text;
    BYTE fade[4];

    rect[0] = 0;
    rect[1] = 0;
    rect[2] = (short)g_pGraphics->resX;
    rect[3] = (short)g_pGraphics->resY;
    Font_SetBlendMode(2);
    *(DWORD *)fade = g_unk0x0051608c;
    fade[3] = 0x73;
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, fade, 2);
    InRaceMenu_DrawHeaderBar();
    maxWidth = 0;
    rect[0] = (short)((int)(g_pGraphics->resX * 0x70) / 0x280);
    rect[2] = g_inRaceMenuUpArrowTexture->width;
    rect[3] = g_inRaceMenuUpArrowTexture->height;
    for (k = 0; k < pMenu->itemCount; k++) {
        if (pMenu->items[k].id == 0x62)
            break;
        text = CFrontend::GetTextString(pMenu->items[k].id);
        width = Font_GetTextWidth(1, (BYTE *)text);
        if (width > maxWidth) {
            text = CFrontend::GetTextString(pMenu->items[k].id);
            maxWidth = Font_GetTextWidth(1, (BYTE *)text);
        }
    }
    i = 0;
    if (pMenu->itemCount > 0) {
        pItem = pMenu->items;
        do {
            pColour = pMenu->cursor == i ? &g_inRaceMenuSelectedColour : &g_inRaceMenuUnselectedColour;
            switch (pItem->value) {
            case 4:
                y = (int)(g_pGraphics->resY * 0xaa) / 0x1e0
                    + ((int)(g_pGraphics->resY * 0x2a) / 0x1e0) * i;
                Font_DrawText(1, CFrontend::GetTextString(0x62),
                              (int)(g_pGraphics->resX * 0x86) / 0x280, y,
                              pItem->max != 0 ? &g_unk0x0051607c : pColour, 0x11);
                break;
            case 0:
            case 1:
            case 2:
            case 3:
                y = (int)(g_pGraphics->resY * 0xaa) / 0x1e0
                    + ((int)(g_pGraphics->resY * 0x2a) / 0x1e0) * i;
                Font_DrawText(1, CFrontend::GetTextString(pItem->id),
                              (int)(g_pGraphics->resX * 0x86) / 0x280, y, pColour, 0x11);
                bar[0] = (short)((int)(g_pGraphics->resX * 5) / 0x280 + maxWidth
                                 + (int)(g_pGraphics->resX * 0x86) / 0x280);
                bar[1] = (short)((int)(g_pGraphics->resY * 0xaa) / 0x1e0
                                 + ((int)(g_pGraphics->resY * 0x2a) / 0x1e0) * i
                                 - (int)(g_pGraphics->resY * 0xe) / 0x1e0);
                bar[2] = (short)((int)(g_pGraphics->resX * 0xe) / 0x280
                                 + (int)(pItem->min * g_pGraphics->resX * 10) / 0x280);
                bar[3] = (short)((int)(g_pGraphics->resY * 0x10) / 0x1e0);
                DrawRectOutline(bar, (BYTE *)pColour);
                bar[0] = (short)((int)(g_pGraphics->resX * 2) / 0x280
                                 + (int)(g_pGraphics->resX * 5) / 0x280 + maxWidth
                                 + (int)(g_pGraphics->resX * 0x86) / 0x280
                                 + (int)(pItem->max * g_pGraphics->resX * 10) / 0x280);
                bar[1] = (short)((int)(g_pGraphics->resY * 0xaa) / 0x1e0
                                 + ((int)(g_pGraphics->resY * 0x2a) / 0x1e0) * i
                                 - (int)(g_pGraphics->resY * 0xc) / 0x1e0);
                bar[2] = (short)((int)(g_pGraphics->resX * 0x17) / 0x280);
                bar[3] = (short)((int)(g_pGraphics->resY * 0xe) / 0x1e0);
                Sprite_FillRect(&g_pGraphics->field309_0x150, bar, (BYTE *)pColour, 2);
                break;
            default:
                y = (int)(g_pGraphics->resY * 0xaa) / 0x1e0
                    + ((int)(g_pGraphics->resY * 0x2a) / 0x1e0) * i;
                Font_DrawText(1, CFrontend::GetTextString(pItem->id),
                              (int)(g_pGraphics->resX * 0x86) / 0x280, y, pColour, 0x11);
                break;
            }
            rect[1] = (short)((int)(g_pGraphics->resY * 0xaa) / 0x1e0
                              + ((int)(g_pGraphics->resY * 0x2a) / 0x1e0) * i
                              - (int)(g_pGraphics->resY * 0xd) / 0x1e0);
            if (pMenu->cursor == i)
                Sprite_Queue((SpriteRect *)&g_inRaceMenuUpArrowTexture->field_0x11c, (SpriteRect *)rect,
                             g_inRaceMenuUpArrowTexture, 2, 0, NULL, NULL, (BYTE *)&g_inRaceMenuSelectedColour, 8);
            else
                Sprite_Queue((SpriteRect *)&g_inRaceMenuDownArrowTexture->field_0x11c, (SpriteRect *)rect,
                             g_inRaceMenuDownArrowTexture, 2, 0, NULL, NULL, (BYTE *)&g_inRaceMenuUnselectedColour, 8);
            i++;
            pItem++;
        } while (i < pMenu->itemCount);
    }
    Font_SetBlendMode(2);
}


// Draws the rows of the car-setup menu (InRaceMenu_BuildCarSetup): each row paints the
// item's own label and then, for every value placed in the item's min/max
// bytes, the matching sub-string; the entry equal to max is highlighted and the
// selected row is drawn brighter with the highlighted row sprite.
// The original recomputes the row's y at every use (MENU_ITEM_Y).
#define MENU_ITEM_Y(i) (((int)(g_pGraphics->resY * 0x2a) / 0x1e0) * (i) + (int)(g_pGraphics->resY * 0xaa) / 0x1e0)

// FUNCTION: CMR2 0x004041e0
void InRaceMenu_DrawCarSetupChoices(Menu *pMenu)
{
    MenuItem *pItem;
    short rect[4];
    int i;
    int j;
    int k;
    int x;
    int width;
    int index;
    int *pColour;
    int *pSubColour;
    Texture *texture;

    rect[0] = 0;
    rect[1] = 0;
    rect[2] = (short)g_pGraphics->resX;
    rect[3] = (short)g_pGraphics->resY;
    Font_SetBlendMode(2);
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, (BYTE *)&g_unk0x0051608c, 4);
    rect[0] = (short)((int)(g_pGraphics->resX * 0x70) / 0x280);
    rect[2] = g_inRaceMenuUpArrowTexture->width;
    rect[3] = g_inRaceMenuUpArrowTexture->height;
    i = 0;
    if (pMenu->itemCount > 0) {
        pItem = pMenu->items;
        do {
            pColour = pMenu->cursor == i ? &g_inRaceMenuSelectedColour : &g_inRaceMenuUnselectedColour;
            switch (pItem->value) {
            case 0:
            case 1:
            case 3:
                strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pItem->id));
                x = (int)(g_pGraphics->resX * 0x86) / 0x280;
                Font_DrawText(1, CFrontend::m_stringDest, x, MENU_ITEM_Y(i), pColour, 0x11);
                j = 0;
                if (pItem->min != 0) {
                    do {
                        width = Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
                        x = (int)(g_pGraphics->resX * 10) / 0x280 + x + width;
                        pSubColour = pItem->max == j ? &g_inRaceMenuSelectedColour : &g_inRaceMenuUnselectedColour;
                        Font_DrawText(1, CFrontend::GetTextString(j + 0x9c), x, MENU_ITEM_Y(i),
                                      pSubColour, 0x11);
                        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(j + 0x9c));
                        j++;
                    } while (j < pItem->min);
                }
                break;
            case 2:
                strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pItem->id));
                x = (int)(g_pGraphics->resX * 0x86) / 0x280;
                Font_DrawText(1, CFrontend::m_stringDest, x, MENU_ITEM_Y(i), pColour, 0x11);
                j = 0;
                if (pItem->min == 2) {
                    do {
                        width = Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
                        x = (int)(g_pGraphics->resX * 10) / 0x280 + x + width;
                        pSubColour = pItem->max == j ? &g_inRaceMenuSelectedColour : &g_inRaceMenuUnselectedColour;
                        Font_DrawText(1, CFrontend::GetTextString(j + 0x9c), x, MENU_ITEM_Y(i),
                                      pSubColour, 0x11);
                        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(j + 0x9c));
                        j++;
                    } while (j < pItem->min);
                } else if (pItem->min != 0) {
                    do {
                        width = Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
                        x = (int)(g_pGraphics->resX * 10) / 0x280 + x + width;
                        pSubColour = pItem->max == j ? &g_inRaceMenuSelectedColour : &g_inRaceMenuUnselectedColour;
                        index = j == 0 ? 0x9c : j + 0xef;
                        Font_DrawText(1, CFrontend::GetTextString(index), x, MENU_ITEM_Y(i),
                                      pSubColour, 0x11);
                        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(index));
                        j++;
                    } while (j < pItem->min);
                }
                break;
            case 4:
                strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pItem->id));
                x = (int)(g_pGraphics->resX * 0x86) / 0x280;
                Font_DrawText(1, CFrontend::m_stringDest, x, MENU_ITEM_Y(i), pColour, 0x11);
                j = 0;
                if (pItem->min != 0) {
                    k = 0xa4;
                    do {
                        width = Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
                        pSubColour = pItem->max == j ? &g_inRaceMenuSelectedColour : &g_inRaceMenuUnselectedColour;
                        x = (int)(g_pGraphics->resX * 10) / 0x280 + x + width;
                        if (RallyData_IsHeadToHeadRaceMode() != 0)
                            strcpy(CFrontend::m_stringDest,
                                   CFrontend::GetTextString(j + 0x9c));
                        else
                            strcpy(CFrontend::m_stringDest,
                                   CFrontend::GetTextString(k));
                        Font_DrawText(1, CFrontend::m_stringDest, x, MENU_ITEM_Y(i), pSubColour, 0x11);
                        j++;
                        k--;
                    } while (j < pItem->min);
                }
                break;
            case 5:
                pSubColour = pItem->max != 0 ? &g_unk0x0051607c : pColour;
                Font_DrawText(1, CFrontend::GetTextString(0x62),
                              (int)(g_pGraphics->resX * 0x86) / 0x280, MENU_ITEM_Y(i), pSubColour, 0x11);
                break;
            default:
                Font_DrawText(1, CFrontend::GetTextString(pItem->id),
                              (int)(g_pGraphics->resX * 0x86) / 0x280, MENU_ITEM_Y(i), pColour, 0x11);
                break;
            }
            rect[1] = (short)(MENU_ITEM_Y(i) - (int)(g_pGraphics->resY * 0xd) / 0x1e0);
            if (pMenu->cursor == i) {
                pColour = &g_inRaceMenuSelectedColour;
                texture = g_inRaceMenuUpArrowTexture;
            } else {
                pColour = &g_inRaceMenuUnselectedColour;
                texture = g_inRaceMenuDownArrowTexture;
            }
            Sprite_Queue((SpriteRect *)&texture->field_0x11c, (SpriteRect *)rect,
                         texture, 2, 0, NULL, NULL, (BYTE *)pColour, 8);
            i++;
            pItem++;
        } while (i < pMenu->itemCount);
    }
    Font_SetBlendMode(2);
}


// ---------------------------------------------------------------------------
// Network options and the option screen separators.

void RallyData_LayoutPlayerSplitBars(void);
void Dash_Update(int player);
// Screen separator scratch rect of the option screens (x, y, width, height),
// also used by the controls pages; defined in FrontendMenus.cpp.
extern short g_controlsLine[4];

// White colour the option screen separators and the panel marker sprites use.
// GLOBAL: CMR2 0x00526ffc
BYTE g_optionWhite[4] = { 0xff, 0xff, 0xff, 0xff };

// Applies the two switches of the network options menu back to the game info,
// rebuilds the split bar and refreshes the dash of the selected player.
// FUNCTION: CMR2 0x00402f20
void InRaceMenu_ApplyNetworkOptions(Menu *pMenu, int param)
{
    int index;

    index = Menu_FindItem(pMenu, 2);
    CGameInfo::SetSplitBarEnabled(pMenu->items[index].max);
    RallyData_LayoutPlayerSplitBars();
    index = Menu_FindItem(pMenu, 4);
    CGameInfo::SetDashOptionEnabled(pMenu->items[index].max);
    Dash_Update(*(BYTE *)&g_unk0x0052af58[1]);
}

// Draws the separator lines of the option screen: one vertical line per slot of
// the layout record param1 (always 1 pixel wide, from the slot's row down to
// the bottom of the option area and alternating between the two sprite layers),
// then the horizontal line that closes the option area and, under its centre,
// the lower separator of the option screen.
// FUNCTION: CMR2 0x00500550
void OptionMenu_DrawSeparators(int param1)
{
    short rect[4];
    int minX;
    int maxX;
    int centre;
    int i;

    g_controlsLine[2] = 1;
    g_unk0x0082ace8.pad[3] = 1;
    if (g_symbolCircleTexture != 0) {
        rect[2] = g_symbolCircleTexture->width;
        rect[3] = g_symbolCircleTexture->height;
    }
    for (i = 0; i < 4; i++) {
        // The layout records hold 16.16 pairs: the integer part is the slot.
        g_controlsLine[0] = (short)(g_optionLayoutCoords[param1 * 4 + i][0] >> 16);
        g_controlsLine[1] = (short)(g_optionLayoutCoords[param1 * 4 + i][1] >> 16);
        g_controlsLine[3] = (short)((int)(g_pGraphics->resY * 0xf5) / 0x1e0) - g_controlsLine[1];
        // Drawn first: the bounds below read the line after Sprite_FillRect.
        if (i % 2 == 0) {
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_controlsLine,
                            (BYTE *)&g_optionWhite, 4);
        } else {
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_controlsLine,
                            (BYTE *)&g_optionWhite, 1);
        }
        if (i == 0) {
            minX = g_controlsLine[0];
            maxX = g_controlsLine[0];
        } else if (g_controlsLine[0] < minX) {
            minX = g_controlsLine[0];
        } else if (g_controlsLine[0] > maxX) {
            maxX = g_controlsLine[0];
        }
        if (g_symbolCircleTexture != 0) {
            if (CGameInfo::GetScreenWidth() >= 0x400 && CFrontend::IsTextureWidthSupported(0x400) &&
                CFrontend::IsTextureHeightSupported(0x400)) {
                rect[0] = g_controlsLine[0] - 6;
                rect[1] = g_controlsLine[1] - 6;
            } else {
                rect[0] = g_controlsLine[0] - 3;
                rect[1] = g_controlsLine[1] - 3;
            }
            if (i % 2 == 0) {
                Sprite_Queue((SpriteRect *)&g_symbolCircleTexture->field_0x11c, (SpriteRect *)rect,
                             g_symbolCircleTexture, 4, 0, NULL, NULL,
                             (BYTE *)&g_optionWhite, 8);
            } else {
                Sprite_Queue((SpriteRect *)&g_symbolCircleTexture->field_0x11c, (SpriteRect *)rect,
                             g_symbolCircleTexture, 1, 0, NULL, NULL,
                             (BYTE *)&g_optionWhite, 8);
            }
        }
    }
    if (maxX != minX) {
        g_unk0x0082ace8.pad[0] = (short)minX;
        g_unk0x0082ace8.pad[1] = (short)((int)(g_pGraphics->resY * 0xf5) / 0x1e0);
        g_unk0x0082ace8.pad[2] = (short)(maxX - minX + 1);
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_unk0x0082ace8.pad, (BYTE *)&g_optionWhite, 1);
    }
    centre = minX + (maxX - minX) / 2;
    g_controlsLine[0] = (short)centre;
    g_controlsLine[1] = (short)((int)(g_pGraphics->resY * 0xf5) / 0x1e0);
    if (centre <= (int)(g_pGraphics->resX * 0xe4) / 0x280) {
        g_controlsLine[3] = (short)((int)(g_pGraphics->resY * 0xff) / 0x1e0) -
                            (short)((int)(g_pGraphics->resY * 0xf5) / 0x1e0);
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_controlsLine, (BYTE *)&g_optionWhite, 1);
        return;
    }
    g_controlsLine[3] = (short)((int)(g_pGraphics->resY * 0x114) / 0x1e0) -
                        (short)((int)(g_pGraphics->resY * 0xf5) / 0x1e0);
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_controlsLine, (BYTE *)&g_optionWhite, 1);
    g_unk0x0082ace8.pad[0] = (short)((int)(g_pGraphics->resX * 0xe5) / 0x280);
    g_unk0x0082ace8.pad[1] = (short)((int)(g_pGraphics->resY * 0x114) / 0x1e0);
    g_unk0x0082ace8.pad[2] = (short)(centre - g_unk0x0082ace8.pad[0] + 1);
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_unk0x0082ace8.pad, (BYTE *)&g_optionWhite, 1);
}

// 0x50-byte entry of 0x82c6c8 as seen by the option panel animation: the source
// and destination rects, the slide ramp, the two fade phases and their start
// times.

// Advances the animation of the option panel selected by 0x82ca1c: eases its
// slide (the ramp is 16.16 and follows the square root of the elapsed
// fraction), copies the source rect while it has not started or the destination
// rect when it is over and interpolates the four edges in between, runs the two
// fade phases of the finished panel and finally rolls the highlight pulse.
// match 61%: implementada; el original resuelve la raiz cuadrada por g_sqrtTable con el
// mismo contador de saltos (bl) que nosotros, pero guarda los 16.16 en EBP-4/EBP-8 y reutiliza
// EAX/EDX en otro orden, asi que las cuatro interpolaciones de borde salen desplazadas.
// FUNCTION: CMR2 0x00505b40
void OptionMenu_AnimateSelectedPanel(void)
{
    FixInterp *pEntry;
    unsigned int now;
    int ratio;
    int index;

    if (g_selectedOptionPreviewPanel == 0xff)
        return;
    index = (signed char)g_selectedOptionPreviewPanel;
    pEntry = g_optionPreviewPanels + index;
    if (pEntry->active != 0) {
        now = CMain::GetFrameDelta();
        ratio = FixDiv((int)(now - pEntry->startTime) << 16, pEntry->distance);
        ratio = FixSqrt(ratio);
        if (ratio >= 0x10000) {
            pEntry->active = 0;
            pEntry->current = pEntry->end;
            pEntry->startTime2 = CMain::GetFrameDelta();
        } else {
            pEntry->current = FixMul(pEntry->end - pEntry->start, ratio) + pEntry->start;
        }
    }
    if (pEntry->current == 0) {
        *(int *)&pEntry->dstX1 = *(int *)&pEntry->srcX1;
        *(int *)&pEntry->dstX2 = *(int *)&pEntry->srcX2;
        pEntry->phase1 = 0;
        pEntry->phase2 = 0;
    } else if (pEntry->current == 0x10000) {
        pEntry->dstX1 = g_optionPreviewExpandedRect[0];
        pEntry->dstY1 = g_optionPreviewExpandedRect[1];
        pEntry->dstX2 = g_optionPreviewExpandedRect[2];
        pEntry->dstY2 = g_optionPreviewExpandedRect[3];
        if (pEntry->active == 0) {
            if (pEntry->phase1 != 0x10000) {
                pEntry->phase1 =
                    FixDiv((int)(CMain::GetFrameDelta() - pEntry->startTime2) << 16, 0x320000);
                if (pEntry->phase1 >= 0x10000) {
                    pEntry->phase1 = 0x10000;
                    pEntry->startTime3 = CMain::GetFrameDelta();
                }
            }
            if (pEntry->phase1 == 0x10000 && pEntry->phase2 != 0x10000) {
                pEntry->phase2 =
                    FixDiv((int)(CMain::GetFrameDelta() - pEntry->startTime3) << 16, 0x640000);
                if (pEntry->phase2 >= 0x10000)
                    pEntry->phase2 = 0x10000;
            }
        } else {
            pEntry->phase1 = 0;
            pEntry->phase2 = 0;
        }
    } else {
        pEntry->dstX1 = (short)((FixMul((g_optionPreviewExpandedRect[0] - pEntry->srcX1) << 16, pEntry->current) >> 16) + pEntry->srcX1);
        pEntry->dstY1 = (short)((FixMul((g_optionPreviewExpandedRect[1] - pEntry->srcY1) << 16, pEntry->current) >> 16) + pEntry->srcY1);
        pEntry->dstX2 = (short)(FixMulShift32((g_optionPreviewExpandedRect[2] - pEntry->srcX2) << 16,
                                              pEntry->current) + pEntry->srcX2);
        pEntry->dstY2 = (short)(FixMulShift32((g_optionPreviewExpandedRect[3] - pEntry->srcY2) << 16,
                                              pEntry->current) + pEntry->srcY2);
        pEntry->phase1 = 0;
        pEntry->phase2 = 0;
    }
    // Sawtooth highlight pulse, restarted when the panel changed (75 frames).
    {
        int t = FixDiv((int)(CMain::GetFrameDelta() - g_unk0x0082c6c0) << 16, 0x4b0000);
        g_unk0x0082cb44 = t - (t & 0xffff0000);
    }
}

// Selects the display mode of a slot: if the entry of the active mode has no
// pending switch, the new slot index (reduced modulo the slot count) is
// activated and the flags of the menu panes are set accordingly; otherwise the
// flags are cleared. Always ends by refreshing the mode animation.
// FUNCTION: CMR2 0x005059d0
void OptionMenu_SelectDisplaySlot(int param_1)
{
    if (g_optionPreviewPanels[(signed char)g_selectedOptionPreviewPanel].active == 0) {
        if ((signed char)g_selectedOptionPreviewPanel != param_1) {
            param_1 = param_1 % g_optionPreviewPanelCount;
            CGameInfo::SwitchOptionPreviewMode(param_1);
        }
        Menu_SetFlags(OptionMenu_GetControlSetupMenu(), 1, 1, 1, 1);
        if ((signed char)g_selectedOptionPreviewPanel != param_1) {
            CGameInfo::SelectIdleOptionPreviewEntry((BYTE)(param_1 % g_optionPreviewPanelCount));
            OptionMenu_AnimateSelectedPanel();
            return;
        }
    } else {
        Menu_SetFlags(OptionMenu_GetControlSetupMenu(), 0, 0, 0, 0);
    }
    OptionMenu_AnimateSelectedPanel();
}

// ---------------------------------------------------------------------------
// W171 batch: game-info state/menu functions (0x50a880-0x50ee10)
// ---------------------------------------------------------------------------
void RallyData_EnterSelectedOptionPanel(void);
void OptionMenu_DrawAnimatedScreenRows(short param1, short param2, short param3, short param4, short param5, int param6);
void OptionMenu_DrawStageSelectionPanel(Menu *pMenu);
void OptionMenu_DrawResultsOptions(int param1);
void OptionMenu_DrawGameInfoLayout(Menu *pMenu, int param2);
void OptionMenu_DrawResultsRallyInfo(int param1);

// Switches the active display slot of the current menu to param_1 and then
// refreshes the slot animation.
// FUNCTION: CMR2 0x0050a880
void OptionMenu_SelectAndAnimateSlot(int param_1, int param_2)
{
    OptionMenu_SelectDisplaySlot(param_1);
    RallyData_EnterSelectedOptionPanel();
}

// Draws the recalled rally entry of the mode panel: option row 7 with the
// "used car" colour 0xf2.
// FUNCTION: CMR2 0x0050a8a0
void OptionMenu_DrawRecalledRallyEntry(Menu *pMenu, int param_2)
{
    int value = OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 1)].max;
    OptionMenu_DrawAnimatedScreenRows(7, value, 2, g_optionLayoutXScale, 0xf2, param_2);
}

// Draws the stage list of the mode panel: option row 0xb with colour 0xaa.
// FUNCTION: CMR2 0x0050a8e0
void OptionMenu_DrawModeStageList(Menu *pMenu)
{
    int value = OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 2)].max;
    OptionMenu_DrawAnimatedScreenRows(0xb, value, 1, g_optionLayoutXScale, 0xaa, 0);
}

// Draws the game-info title string and the current option of the mode panel.
// FUNCTION: CMR2 0x0050e6a0
void OptionMenu_DrawModeTitle(void)
{
    OptionMenu_DrawTransitionTextShortCoords(4, 0, 0, CFrontend::GetTextString(0x133),
                 (int)g_pGraphics->resX * 0xf0 / 0x280, (int)g_pGraphics->resY * 200 / 0x1e0,
                 g_optionTextColours, g_optionTextBrightColours, 0x11);
    int value = OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 5)].max;
    OptionMenu_DrawAnimatedScreenRows(2, value, 3, 0xf0, 0xd7, 0);
}

// Clears the live slots and then re-renders the mode panel with the raised
// entry selected (menu action callback of the rally-info pane).
// FUNCTION: CMR2 0x0050edf0
void OptionMenu_DrawRaisedRallyInfo(Menu *pMenu)
{
    OptionMenu_DrawResultsOptions(0);
    OptionMenu_DrawStageSelectionPanel(pMenu);
}

// Renders the whole rally-info screen for param_1: the option panes, the
// recalled entry, the game-info layout and the rally preview text.
// FUNCTION: CMR2 0x0050ee10
void OptionMenu_DrawRallyInfoScreen(Menu *pMenu)
{
    OptionMenu_DrawResultsOptions(1);
    OptionMenu_DrawClampedStageTime();
    OptionMenu_DrawRightColumn();
    OptionMenu_DrawRecalledRallyEntry(pMenu, 1);
    OptionMenu_DrawGameInfoLayout(pMenu, 1);
    OptionMenu_DrawResultsRallyInfo(0);
    OptionMenu_DrawSeparators(CGameInfo::GetActiveOptionSlot());
}

// The PE entry point of CMR2.exe is the CRT's WinMainCRTStartup (crt0): it
// initialises the heap, the stdio tables, argv/envp and the C initialisers and
// then calls WinMain. It is not game code and our build links the same CRT
// startup from the library, so it is declared as a library symbol instead of
// being decompiled.
// LIBRARY: CMR2 0x00405626
// _WinMainCRTStartup

// Screen-space rectangle (x, y, w, h) the game-info option screens draw into.
// GLOBAL: CMR2 0x00831660
short g_gameInfoDrawRect[4];
// Background colour of the game-info option screens.
// GLOBAL: CMR2 0x005273b8
int g_gameInfoBackgroundColour = -2832441;

// Draws the game-info option screen of the menu's active item: the shared
// background panel, the moving highlight bar of the selected row and, per
// option type, the title and the value strings of the option list; the rows of
// the list screen (case 0) are laid out in pairs around their centred bar.
// FUNCTION: CMR2 0x0050cc10
void OptionMenu_DrawGameInfoLayout(Menu *pMenu, int param_2)
{
    int *pColour;
    int index;
    int count;
    int x;
    int y;
    int centre;
    int i;
    int width;

    index = OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 1)].max;
    pColour = &g_unk0x0052737c;
    g_gameInfoDrawRect[0] = (short)((int)g_pGraphics->resX * 0x1c / 0x280);
    g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0x160 / 0x1e0);
    g_gameInfoDrawRect[2] = (short)((int)g_pGraphics->resX * 0x147 / 0x280);
    g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0xd / 0x1e0);
    if (!(char)param_2)
        pColour = &g_optionTextColours[2];
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_gameInfoDrawRect, (BYTE *)&g_gameInfoBackgroundColour, 3);
    if ((char)param_2 != '\0' && (CMain::GetFrameDelta() / 0x14 & 1) == 0) {
        g_gameInfoDrawRect[0] -= 3;
        g_gameInfoDrawRect[2] += 6;
        g_gameInfoDrawRect[1] -= 3;
        if (index == 0)
            g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0x18e / 0x1e0) -
                                 g_gameInfoDrawRect[1];
        else
            g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0x181 / 0x1e0) -
                                 g_gameInfoDrawRect[1];
        OptionMenu_DrawBoxEdges(g_gameInfoDrawRect, (BYTE *)&g_unk0x0052737c);
    }
    count = OptionMenu_GetEntryCarSetupValue(CGameInfo::GetActiveOptionSlot(), index);
    switch (index) {
    case 0:
        if (count == 6)
            g_gameInfoDrawRect[0] = (short)((int)g_pGraphics->resX * 0x151 / 0x280);
        else
            g_gameInfoDrawRect[0] = (short)((count * 0x1824 / 0x78 + 0x1c) *
                                         (int)g_pGraphics->resX / 0x280);
        g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0x160 / 0x1e0 + 1);
        g_gameInfoDrawRect[2] = (short)((int)g_pGraphics->resX * 0x12 / 0x280);
        g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0xd / 0x1e0 - 2);
        if ((char)param_2 != '\0')
            OptionMenu_DrawBoxEdges(g_gameInfoDrawRect, (BYTE *)&g_optionTextColours[2]);
        if (OptionMenu_GetValueSliderMenu()->items[0].max == 6)
            g_gameInfoDrawRect[0] = (short)((int)g_pGraphics->resX * 0x151 / 0x280);
        else
            g_gameInfoDrawRect[0] = (short)((OptionMenu_GetValueSliderMenu()->items[0].max * 0x1824 / 0x78 + 0x1c) *
                                         (int)g_pGraphics->resX / 0x280);
        g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0x160 / 0x1e0 + 1);
        g_gameInfoDrawRect[2] = (short)((int)g_pGraphics->resX * 0x12 / 0x280);
        g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0xd / 0x1e0 - 2);
        if ((char)param_2 == '\0' || (CMain::GetFrameDelta() / 0x14 & 1) == 0)
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_gameInfoDrawRect, (BYTE *)pColour, 3);
        g_gameInfoDrawRect[2] = 1;
        g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0x1a / 0x1e0);
        g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0x16f / 0x1e0);
        for (i = 0; i < 7; i++) {
            if (i == 0) {
                x = (int)g_pGraphics->resX * 0x1c / 0x280;
                sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue,
                        CFrontend::GetTextString(0x41));
                y = (int)g_pGraphics->resY * 0x16c / 0x1e0;
                Font_DrawText(1, CFrontend::m_stringDest, x, y, g_optionTextColours, 9);
            } else if (i == 6) {
                x = (int)g_pGraphics->resX * 0x163 / 0x280;
                sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue,
                        CFrontend::GetTextString(0x40));
                y = (int)g_pGraphics->resY * 0x16c / 0x1e0;
                Font_DrawText(1, CFrontend::m_stringDest, x, y, g_optionTextColours, 0xc);
            } else {
                x = ((i * 0x1824) / 0x78 + 0x25) * (int)g_pGraphics->resX / 0x280;
                sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue,
                        CFrontend::GetTextString(i % 2 + 0x41));
                y = (int)g_pGraphics->resY * 0x16c / 0x1e0;
                Font_DrawText(1, CFrontend::m_stringDest, x, y, g_optionTextColours, 10);
            }
            if (i % 2 == 1) {
                if (i > 0)
                    centre = Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest) / 2 + x;
                x += (int)g_pGraphics->resX * -0x1a / 0x280;
                Font_DrawText(1, CFrontend::GetTextString(i / 2 + 0x3d), x,
                              (int)g_pGraphics->resY * 0x179 / 0x1e0,
                              g_optionTextColours, 10);
            }
            if (i % 2 == 0 && i > 0) {
                if (i == 6)
                    x -= Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
                else
                    x -= Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest) / 2;
                g_gameInfoDrawRect[0] = (short)((centre + x) / 2);
                Sprite_FillRect(&g_pGraphics->field309_0x150, g_gameInfoDrawRect,
                                (BYTE *)&g_optionTextColours[2], 3);
            }
        }
        return;
    case 1:
        if (count == 4)
            g_gameInfoDrawRect[0] = (short)((int)g_pGraphics->resX * 0x151 / 0x280);
        else
            g_gameInfoDrawRect[0] = (short)((count * 0x1e2d / 100 + 0x1c) *
                                         (int)g_pGraphics->resX / 0x280);
        g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0x160 / 0x1e0 + 1);
        g_gameInfoDrawRect[2] = (short)((int)g_pGraphics->resX * 0x12 / 0x280);
        g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0xd / 0x1e0 - 2);
        if ((char)param_2 != '\0')
            OptionMenu_DrawBoxEdges(g_gameInfoDrawRect, (BYTE *)&g_optionTextColours[2]);
        if (OptionMenu_GetValueSliderMenu()->items[0].max == 4)
            g_gameInfoDrawRect[0] = (short)((int)g_pGraphics->resX * 0x151 / 0x280);
        else
            g_gameInfoDrawRect[0] = (short)((OptionMenu_GetValueSliderMenu()->items[0].max * 0x1e2d / 100 + 0x1c) *
                                         (int)g_pGraphics->resX / 0x280);
        g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0x160 / 0x1e0 + 1);
        g_gameInfoDrawRect[2] = (short)((int)g_pGraphics->resX * 0x12 / 0x280);
        g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0xd / 0x1e0 - 2);
        if ((char)param_2 == '\0' || (CMain::GetFrameDelta() / 0x14 & 1) == 0)
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_gameInfoDrawRect, (BYTE *)pColour, 3);
        Font_DrawText(1, CFrontend::GetTextString(0xc9),
                      (int)g_pGraphics->resX * 0x1c / 0x280,
                      (int)g_pGraphics->resY * 0x16c / 0x1e0, g_optionTextColours, 9);
        Font_DrawText(1, CFrontend::GetTextString(0xca),
                      (int)g_pGraphics->resX * 0x163 / 0x280,
                      (int)g_pGraphics->resY * 0x16c / 0x1e0, g_optionTextColours, 0xc);
        return;
    case 2:
        g_gameInfoDrawRect[0] = (short)((count * 0x135 / 100 + 0x1c) *
                                     (int)g_pGraphics->resX / 0x280);
        g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0x160 / 0x1e0 + 1);
        g_gameInfoDrawRect[2] = (short)((int)g_pGraphics->resX * 0x12 / 0x280);
        g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0xd / 0x1e0 - 2);
        if ((char)param_2 != '\0')
            OptionMenu_DrawBoxEdges(g_gameInfoDrawRect, (BYTE *)&g_optionTextColours[2]);
        g_gameInfoDrawRect[0] = (short)((OptionMenu_GetValueSliderMenu()->items[0].max * 0x135 / 10 + 0x1c) *
                                     (int)g_pGraphics->resX / 0x280);
        g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0x160 / 0x1e0 + 1);
        g_gameInfoDrawRect[2] = (short)((int)g_pGraphics->resX * 0x12 / 0x280);
        g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0xd / 0x1e0 - 2);
        if ((char)param_2 == '\0' || (CMain::GetFrameDelta() / 0x14 & 1) == 0)
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_gameInfoDrawRect, (BYTE *)pColour, 3);
        Font_DrawText(1, CFrontend::GetTextString(0xcb),
                      (int)g_pGraphics->resX * 0x1c / 0x280,
                      (int)g_pGraphics->resY * 0x16c / 0x1e0, g_optionTextColours, 9);
        Font_DrawText(1, CFrontend::GetTextString(0xcc),
                      (int)g_pGraphics->resX * 0x163 / 0x280,
                      (int)g_pGraphics->resY * 0x16c / 0x1e0, g_optionTextColours, 0xc);
        return;
    case 3:
        g_gameInfoDrawRect[0] = (short)((count * 0x135 / 100 + 0x1c) *
                                     (int)g_pGraphics->resX / 0x280);
        g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0x160 / 0x1e0 + 1);
        g_gameInfoDrawRect[2] = (short)((int)g_pGraphics->resX * 0x12 / 0x280);
        g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0xd / 0x1e0 - 2);
        if ((char)param_2 != '\0')
            OptionMenu_DrawBoxEdges(g_gameInfoDrawRect, (BYTE *)&g_optionTextColours[2]);
        g_gameInfoDrawRect[0] = (short)((OptionMenu_GetValueSliderMenu()->items[0].max * 0x135 / 10 + 0x1c) *
                                     (int)g_pGraphics->resX / 0x280);
        g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0x160 / 0x1e0 + 1);
        g_gameInfoDrawRect[2] = (short)((int)g_pGraphics->resX * 0x12 / 0x280);
        g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0xd / 0x1e0 - 2);
        if ((char)param_2 == '\0' || (CMain::GetFrameDelta() / 0x14 & 1) == 0)
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_gameInfoDrawRect, (BYTE *)pColour, 3);
        Font_DrawText(1, CFrontend::GetTextString(0xce),
                      (int)g_pGraphics->resX * 0x1c / 0x280,
                      (int)g_pGraphics->resY * 0x16c / 0x1e0, g_optionTextColours, 9);
        Font_DrawText(1, CFrontend::GetTextString(0xcd),
                      (int)g_pGraphics->resX * 0x163 / 0x280,
                      (int)g_pGraphics->resY * 0x16c / 0x1e0, g_optionTextColours, 0xc);
        return;
    case 4:
        g_gameInfoDrawRect[0] = (short)((count * 0x135 / 100 + 0x1c) *
                                     (int)g_pGraphics->resX / 0x280);
        g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0x160 / 0x1e0 + 1);
        g_gameInfoDrawRect[2] = (short)((int)g_pGraphics->resX * 0x12 / 0x280);
        g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0xd / 0x1e0 - 2);
        if ((char)param_2 != '\0')
            OptionMenu_DrawBoxEdges(g_gameInfoDrawRect, (BYTE *)&g_optionTextColours[2]);
        g_gameInfoDrawRect[0] = (short)((OptionMenu_GetValueSliderMenu()->items[0].max * 0x135 / 10 + 0x1c) *
                                     (int)g_pGraphics->resX / 0x280);
        g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0x160 / 0x1e0 + 1);
        g_gameInfoDrawRect[2] = (short)((int)g_pGraphics->resX * 0x12 / 0x280);
        g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0xd / 0x1e0 - 2);
        if ((char)param_2 == '\0' || (CMain::GetFrameDelta() / 0x14 & 1) == 0)
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_gameInfoDrawRect, (BYTE *)pColour, 3);
        Font_DrawText(1, CFrontend::GetTextString(0xce),
                      (int)g_pGraphics->resX * 0x1c / 0x280,
                      (int)g_pGraphics->resY * 0x16c / 0x1e0, g_optionTextColours, 9);
        Font_DrawText(1, CFrontend::GetTextString(0xcd),
                      (int)g_pGraphics->resX * 0x163 / 0x280,
                      (int)g_pGraphics->resY * 0x16c / 0x1e0, g_optionTextColours, 0xc);
        return;
    case 6:
        g_gameInfoDrawRect[0] = (short)((count * 0x135 / 100 + 0x1c) *
                                     (int)g_pGraphics->resX / 0x280);
        g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0x160 / 0x1e0 + 1);
        g_gameInfoDrawRect[2] = (short)((int)g_pGraphics->resX * 0x12 / 0x280);
        g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0xd / 0x1e0 - 2);
        if ((char)param_2 != '\0')
            OptionMenu_DrawBoxEdges(g_gameInfoDrawRect, (BYTE *)&g_optionTextColours[2]);
        g_gameInfoDrawRect[0] = (short)((OptionMenu_GetValueSliderMenu()->items[0].max * 0x135 / 10 + 0x1c) *
                                     (int)g_pGraphics->resX / 0x280);
        g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0x160 / 0x1e0 + 1);
        g_gameInfoDrawRect[2] = (short)((int)g_pGraphics->resX * 0x12 / 0x280);
        g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0xd / 0x1e0 - 2);
        if ((char)param_2 == '\0' || (CMain::GetFrameDelta() / 0x14 & 1) == 0)
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_gameInfoDrawRect, (BYTE *)pColour, 3);
        Font_DrawText(1, CFrontend::GetTextString(0xd2),
                      (int)g_pGraphics->resX * 0x1c / 0x280,
                      (int)g_pGraphics->resY * 0x16c / 0x1e0, g_optionTextColours, 9);
        Font_DrawText(1, CFrontend::GetTextString(0xd1),
                      (int)g_pGraphics->resX * 0x163 / 0x280,
                      (int)g_pGraphics->resY * 0x16c / 0x1e0, g_optionTextColours, 0xc);
        return;
    case 5:
        g_gameInfoDrawRect[0] = (short)((count * 0x135 / 100 + 0x1c) *
                                     (int)g_pGraphics->resX / 0x280);
        g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0x160 / 0x1e0 + 1);
        g_gameInfoDrawRect[2] = (short)((int)g_pGraphics->resX * 0x12 / 0x280);
        g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0xd / 0x1e0 - 2);
        if ((char)param_2 != '\0')
            OptionMenu_DrawBoxEdges(g_gameInfoDrawRect, (BYTE *)&g_optionTextColours[2]);
        g_gameInfoDrawRect[0] = (short)((OptionMenu_GetValueSliderMenu()->items[0].max * 0x135 / 10 + 0x1c) *
                                     (int)g_pGraphics->resX / 0x280);
        g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0x160 / 0x1e0 + 1);
        g_gameInfoDrawRect[2] = (short)((int)g_pGraphics->resX * 0x12 / 0x280);
        g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0xd / 0x1e0 - 2);
        if ((char)param_2 == '\0' || (CMain::GetFrameDelta() / 0x14 & 1) == 0)
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_gameInfoDrawRect, (BYTE *)pColour, 3);
        Font_DrawText(1, CFrontend::GetTextString(0xcf),
                      (int)g_pGraphics->resX * 0x1c / 0x280,
                      (int)g_pGraphics->resY * 0x16c / 0x1e0, g_optionTextColours, 9);
        Font_DrawText(1, CFrontend::GetTextString(0xd0),
                      (int)g_pGraphics->resX * 0x163 / 0x280,
                      (int)g_pGraphics->resY * 0x16c / 0x1e0, g_optionTextColours, 0xc);
        break;
    }
}

// ---------------------------------------------------------------------------
// Boot/state subsystem and option value list (addresses 0x0050e280-0x0050eba0)
// ---------------------------------------------------------------------------

// Keyboard labels of the option value rows ("1".."10","SS").
extern char g_keypad2[];
extern char g_keypad3[];
extern char g_keypad4[];
extern char g_keypad6[];
extern char g_keypad7[];
extern char g_keypad8[];
extern char g_keypad9[];
extern char g_strNum1[4];
extern char g_strNum5[4];
extern char g_strNum10[4];
extern char g_strFlagSS[4];

// Undocumented batch functions of the option/boot subsystem.
void OptionMenu_DrawRecalledRallyEntry(Menu *, int);
void OptionMenu_DrawModeStageList(Menu *);
void OptionMenu_DrawResultsRallyInfo(int);
void OptionMenu_DrawSlotStripPanel(Menu *);
void OptionMenu_DrawResultsOptions(int);
void OptionMenu_DrawGameInfoLayout(Menu *, int);
void OptionMenu_DrawModeTitle(void);
void OptionMenu_DrawAnimatedScreenRows(short, short, short, short, short, int);
void OptionMenu_DrawRecordValues(Menu *);

// GLOBAL: CMR2 0x00529430
int g_unk0x00529430 = -1;

// Draws the option record values: composes the text of every record row from
// its value, its unit label and its sub-label, highlights the row currently
// being edited and finally lays out the value bar of the selected item.
// FUNCTION: CMR2 0x0050e280
void OptionMenu_DrawRecordValues(Menu *pMenu)
{
    char *pLabels[11] = { g_strNum1,  g_keypad2, g_keypad3, g_keypad4, g_strNum5, g_keypad6,
                          g_keypad7, g_keypad8, g_keypad9, g_strNum10, g_strFlagSS };
    int *pSel;
    int *pUnsel;
    int i;

    if (g_unk0x0082ab44 != 0) {
        pSel = &g_unk0x0052737c;
        pUnsel = &g_optionTextBrightColours[1];
    } else {
        pSel = &g_optionTextBrightColours[1];
        pUnsel = g_optionTextColours;
    }
    if (g_unk0x0082ac48 > 0) {
        for (i = 0; i < g_unk0x0082ac48; i++) {
            sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x145),
                    SavedGames_GetDifficulty(g_savedGameListTop + i),
                    CFrontend::GetTextString(SavedGames_GetDateLowBits(g_savedGameListTop + i) + 0x93),
                    pLabels[SavedGames_GetDateMiddleBits(g_savedGameListTop + i)]);
            if (g_savedGameListSelection == g_savedGameListTop + i) {
                OptionMenu_DrawTransitionTextShortCoords(4, 1, 1, (char *)SavedGames_GetFileName(g_savedGameListTop + i),
                             (int)g_pGraphics->resX * 0x32 / 0x280,
                             (int)g_pGraphics->resY * 100 / 0x1e0 +
                                 ((int)g_pGraphics->resY * 0x14) / 0x1e0 * i,
                             pSel, g_optionTextBrightColours, 0x11);
                OptionMenu_DrawTransitionTextShortCoords(4, 1, 1, CFrontend::m_stringDest,
                             (int)g_pGraphics->resX * 0x9b / 0x280,
                             (int)g_pGraphics->resY * 100 / 0x1e0 +
                                 ((int)g_pGraphics->resY * 0x14) / 0x1e0 * i,
                             pSel, g_optionTextBrightColours, 0x11);
                OptionMenu_DrawTransitionTextShortCoords(4, 1, 1, (char *)SavedGames_GetRecordData(g_savedGameListTop + i),
                             (int)g_pGraphics->resX * 0xff / 0x280,
                             (int)g_pGraphics->resY * 100 / 0x1e0 +
                                 ((int)g_pGraphics->resY * 0x14) / 0x1e0 * i,
                             pSel, g_optionTextBrightColours, 0x11);
            } else {
                OptionMenu_DrawTransitionTextShortCoords(4, 1, 1, (char *)SavedGames_GetFileName(g_savedGameListTop + i),
                             (int)g_pGraphics->resX * 0x32 / 0x280,
                             (int)g_pGraphics->resY * 100 / 0x1e0 +
                                 ((int)g_pGraphics->resY * 0x14) / 0x1e0 * i,
                             pUnsel, g_optionTextBrightColours, 0x11);
                OptionMenu_DrawTransitionTextShortCoords(4, 1, 1, CFrontend::m_stringDest,
                             (int)g_pGraphics->resX * 0x9b / 0x280,
                             (int)g_pGraphics->resY * 100 / 0x1e0 +
                                 ((int)g_pGraphics->resY * 0x14) / 0x1e0 * i,
                             pUnsel, g_optionTextBrightColours, 0x11);
                OptionMenu_DrawTransitionTextShortCoords(4, 1, 1, (char *)SavedGames_GetRecordData(g_savedGameListTop + i),
                             (int)g_pGraphics->resX * 0xff / 0x280,
                             (int)g_pGraphics->resY * 100 / 0x1e0 +
                                 ((int)g_pGraphics->resY * 0x14) / 0x1e0 * i,
                             pUnsel, g_optionTextBrightColours, 0x11);
            }
        }
    } else {
        OptionMenu_DrawTransitionTextShortCoords(4, 0, 0, CFrontend::GetTextString(0x140), (int)g_pGraphics->resX / 2,
                     (int)g_pGraphics->resY / 2, pUnsel, g_optionTextBrightColours, 0x12);
    }
    OptionMenu_DrawAnimatedScreenRows(2, pMenu->items[Menu_FindItem(pMenu, 4)].max,
                 4, 0x32, 0x17c, 1);
    if (CGameInfo::GetActiveOptionSlot() == 1)
        CGameInfo::IsConfiguredMultiplayer();
}

// FUNCTION: CMR2 0x0050e780
void OptionMenu_DrawValueSelectionRows(Menu *pMenu)
{
    short destRect[4];
    int i;
    int yBase;
    int colour;
    int x0;
    int sel;

    x0 = (int)g_pGraphics->resX * 0xf0 / 0x280;
    yBase = (int)g_pGraphics->resY * 0xd7 / 0x1e0;
    sel = pMenu->cursor;
    colour = g_unk0x0052737c;
    destRect[0] = (short)(x0 * (int)g_pGraphics->resX / 0x280);
    destRect[1] = 0;
    destRect[2] = 0;
    destRect[3] = 0;
    OptionMenu_DrawTransitionTextShortCoords(4, 0, 0, CFrontend::GetTextString(0x13f),
                 (int)g_pGraphics->resX * 0xf0 / 0x280, (int)g_pGraphics->resY * 200 / 0x1e0,
                 g_optionTextColours, g_optionTextBrightColours, 0x11);
    g_gameInfoDrawRect[0] = (short)x0;
    g_gameInfoDrawRect[1] = (short)yBase;
    g_gameInfoDrawRect[2] = (short)((int)g_pGraphics->resX * 0xa2 / 0x280);
    g_gameInfoDrawRect[3] = 1;
    OptionMenu_EaseRecordBar(0, g_gameInfoDrawRect, 1, 1);
    if (sel == 0)
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_gameInfoDrawRect, (BYTE *)&colour, 1);
    else
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_gameInfoDrawRect, (BYTE *)&g_optionTextColours[2], 1);
    for (i = 0; i < pMenu->itemCount; i++) {
        if (g_arrow640ATexture != 0) {
            if (CGameInfo::GetScreenWidth() < 0x400 || !CFrontend::IsTextureWidthSupported(0x400) ||
                !CFrontend::IsTextureHeightSupported(0x400))
                destRect[1] = (short)((int)g_pGraphics->resY * 0x18 / 0x1e0 / 2) +
                              g_gameInfoDrawRect[1] - 6;
            else
                destRect[1] = (short)((int)g_pGraphics->resY * 0x18 / 0x1e0 / 2) +
                              g_gameInfoDrawRect[1] - 0xa;
        }
        sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue,
                CFrontend::GetTextString(i + 0x100));
        if (i == sel) {
            ((void (__stdcall *)(int, int, int, char *, int, short, int *, int *, unsigned int))OptionMenu_DrawTransitionText)(7, 0, 0, CFrontend::m_stringDest,
                         (int)g_pGraphics->resX * 0x14 / 0x280 + x0,
                         (short)((int)g_pGraphics->resY * 0x12 / 0x1e0) + g_gameInfoDrawRect[1],
                         &colour, g_optionTextColours, 0x11);
            if (g_arrow640ATexture != 0)
                Sprite_Queue((SpriteRect *)&g_arrow640ATexture->field_0x11c, (SpriteRect *)destRect,
                             g_arrow640ATexture, 1, 0, 0, 0, (BYTE *)&colour, 8);
        } else {
            OptionMenu_DrawTransitionTextShortCoords(3, 0, 0, CFrontend::m_stringDest,
                         (int)g_pGraphics->resX * 0x14 / 0x280 + x0,
                         (short)((int)g_pGraphics->resY * 0x12 / 0x1e0 + g_gameInfoDrawRect[1]),
                         g_optionTextColours, g_optionTextBrightColours, 0x11);
            if (g_arrow640DTexture != 0)
                Sprite_Queue((SpriteRect *)&g_arrow640DTexture->field_0x11c, (SpriteRect *)destRect,
                             g_arrow640DTexture, 1, 0, 0, 0, (BYTE *)g_optionTextColours, 8);
        }
        g_gameInfoDrawRect[1] =
            (short)((int)g_pGraphics->resY * 0x18 / 0x1e0 * (i + 1) + yBase);
        if (i == sel || i + 1 == sel) {
            if (CGameInfo::IsOptionMenuTimeoutPulseOn() != 0)
                Sprite_FillRect(&g_pGraphics->field309_0x150, g_gameInfoDrawRect, (BYTE *)&g_unk0x00527378, 1);
            else
                Sprite_FillRect(&g_pGraphics->field309_0x150, g_gameInfoDrawRect, (BYTE *)&colour, 1);
        } else {
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_gameInfoDrawRect, (BYTE *)&g_optionTextColours[2], 1);
        }
    }
}

// Boot/state subsystem entry point: dispatches on the frontend state and
// updates the record tables of the option record being edited.
// FUNCTION: CMR2 0x0050eba0
void OptionMenu_DispatchRecordEditState(Menu *pMenu)
{
    OptionMenu_DrawResultsOptions(0);
    switch (OptionMenu_FindFirstDetailedSlot()) {
    case 1:
        OptionMenu_DrawClampedStageTime();
        OptionMenu_DrawRightColumn();
        OptionMenu_DrawRecalledRallyEntry(pMenu, 0);
        OptionMenu_DrawGameInfoLayout(OptionMenu_GetValueSliderMenu(), 0);
        OptionMenu_DrawResultsRallyInfo(0);
        OptionMenu_DrawSeparators(CGameInfo::GetActiveOptionSlot());
        if (g_unk0x00529430 !=
            OptionMenu_GetColumnLabelId(OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 1)].max)) {
            OptionPreview_SetTargetModelAngles(CGameInfo::GetActiveOptionSlot(),
                         &g_previewPartRotationPresets[OptionMenu_GetColumnLabelId(
                             OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 1)].max)],
                         0);
            OptionMenu_SetTargetLayoutPairs(CGameInfo::GetActiveOptionSlot(),
                         &g_previewProjectedPartQuads[OptionMenu_GetColumnLabelId(
                             OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 1)].max)].points[0].x,
                         0);
            g_unk0x00529430 = OptionMenu_GetColumnLabelId(OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 1)].max);
            return;
        }
        break;
    case 2:
        OptionMenu_DrawClampedStageTime();
        OptionMenu_DrawModeStageList(pMenu);
        OptionMenu_DrawResultsRallyInfo(1);
        OptionMenu_DrawSeparators(CGameInfo::GetActiveOptionSlot());
        if (g_unk0x00529430 !=
            OptionMenu_GetColumnWeight(OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 2)].max)) {
            OptionPreview_SetTargetModelAngles(CGameInfo::GetActiveOptionSlot(),
                         &g_previewPartRotationPresets[OptionMenu_GetColumnWeight(
                             OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 2)].max)],
                         0);
            OptionMenu_SetTargetLayoutPairs(CGameInfo::GetActiveOptionSlot(),
                         &g_previewProjectedPartQuads[OptionMenu_GetColumnWeight(
                             OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 2)].max)].points[0].x,
                         0);
            g_unk0x00529430 = OptionMenu_GetColumnWeight(OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 2)].max);
            return;
        }
        break;
    case 0:
        OptionMenu_DrawSlotStripPanel(pMenu);
        return;
    case 3:
        OptionMenu_DrawScreenTitle();
        return;
    case 4:
        OptionMenu_DrawRecordValues(pMenu);
        return;
    case 5:
        OptionMenu_DrawModeTitle();
    }
}


// Panel colours used by the rally information screen (amber, blue, green and
// grey, always opaque).
// GLOBAL: CMR2 0x005273a8
BYTE g_colour0x005273a8[8] = { 0xff, 0xff, 0xff, 0xff, 0xfc, 0xf0, 0xcc, 0xff };
// GLOBAL: CMR2 0x005273b0
BYTE g_colour0x005273b0[8] = { 0xe7, 0xeb, 0xc6, 0xff, 0xc9, 0xd6, 0xd8, 0xff };
// Working rectangle shared by the rally information screen (0x831660..0x831667).
// Stage duration in hundredths of a second, indexed by the stage type.
// GLOBAL: CMR2 0x005293c0
int g_unk0x005293c0[7] = { 2, 5, 8, 7, 4, 4, 5 };
// GLOBAL: CMR2 0x0052960c
char g_str0x0052960c[] = "Weather Forecast: %s";
// GLOBAL: CMR2 0x00529624
char g_str0x00529624[] = "Stage";
// GLOBAL: CMR2 0x00524e20
char g_str0x00524e20[] = "100";

extern char g_stageNumberFormat[];
extern char g_str0x00519fb0[];


// stdcall callees that are not implemented yet (ret 8 / ret 0x18).
void OptionMenu_DrawAnimatedScreenRows(short, short, short, short, short, int);
void OptionMenu_SelectAndAnimateSlot(int, int);

// Draws the rally information panel of the results screen: the two shaded
// bars, the stage name, the stage thumbnail and (second mode only) the time
// bar, the target time and the stage row labels.
// FUNCTION: CMR2 0x0050a920
void OptionMenu_DrawResultsRallyInfo(int param_1)
{
    BYTE col[12] = { 0xae, 0xc9, 0x65, 0xff, 0x57, 0x94, 0xc4, 0xff,
                     0xc1, 0x61, 0xa0, 0xff };
    int t;
    int base;
    int w;
    int x;
    int y;
    int i;
    int *pCol;
    Texture *tex;

    g_gameInfoDrawRect[0] = (short)((int)g_pGraphics->resX * 0x1c / 0x280);
    g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0xff / 0x1e0);
    g_gameInfoDrawRect[2] = (short)((int)g_pGraphics->resX * 0xc9 / 0x280);
    g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0x5d / 0x1e0);
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_gameInfoDrawRect, g_colour0x005273b0, 3);
    g_gameInfoDrawRect[0] = g_gameInfoDrawRect[0] + g_gameInfoDrawRect[2];
    g_gameInfoDrawRect[3] = g_gameInfoDrawRect[3] + g_gameInfoDrawRect[1];
    g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0x12a / 0x1e0);
    g_gameInfoDrawRect[2] = (short)((int)g_pGraphics->resX * 0x7e / 0x280);
    g_gameInfoDrawRect[3] = g_gameInfoDrawRect[3] - g_gameInfoDrawRect[1];
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_gameInfoDrawRect, g_colour0x005273b0 + 4, 3);
    x = g_gameInfoDrawRect[2] / 2 + g_gameInfoDrawRect[0];
    y = (int)g_pGraphics->resY * 0x124 / 0x1e0;
    if (param_1 != 0)
        sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue,
                    CFrontend::GetTextString(0x134));
    else
        sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue,
                    CFrontend::GetTextString(0x135));
    x = x - Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest) / 2;
    Font_DrawText(1, CFrontend::m_stringDest, x, y, g_optionTextColours, 0x11);
    if (param_1 != 0) {
        tex = g_carPartTextures[OptionMenu_GetColumnWeight(
            OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 2)].max)];
    } else {
        tex = g_carPartTextures[OptionMenu_GetColumnLabelId(
            OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 1)].max)];
    }
    if (tex != NULL) {
        g_gameInfoDrawRect[0] = (short)((int)g_pGraphics->resX * 0x2c / 0x280);
        g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0x12d / 0x1e0);
        g_gameInfoDrawRect[2] = tex->width;
        g_gameInfoDrawRect[3] = tex->height;
        g_gameInfoDrawRect[1] = g_gameInfoDrawRect[1] - g_gameInfoDrawRect[3] / 2;
        Sprite_Queue((SpriteRect *)&tex->field_0x11c, (SpriteRect *)g_gameInfoDrawRect,
                     tex, 1, 0, NULL, NULL, g_colour0x005273a8, 8);
    }
    if (param_1 != 0) {
        g_gameInfoDrawRect[0] = (short)((int)g_pGraphics->resX * 0x1c / 0x280);
        g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0x160 / 0x1e0);
        g_gameInfoDrawRect[2] = (short)((int)g_pGraphics->resX * 0x147 / 0x280);
        g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0xd / 0x1e0);
        OptionMenu_DrawThreeColourGradientStrips(g_gameInfoDrawRect, 1, 0x15, col);
        sprintf(CFrontend::m_stringDest, g_str0x00519fb0);
        Font_DrawText(0, CFrontend::m_stringDest, g_gameInfoDrawRect[0],
                      (int)g_pGraphics->resY * 0x17c / 0x1e0, g_optionTextColours, 0x11);
        sprintf(CFrontend::m_stringDest, g_str0x00524e20);
        Font_DrawText(0, CFrontend::m_stringDest,
                      OptionMenu_AddPackedScreenRectangleWords((int *)g_gameInfoDrawRect),
                      (int)g_pGraphics->resY * 0x17c / 0x1e0, g_optionTextColours, 0x14);
        Font_DrawText(1, CFrontend::GetTextString(0xf9),
                      g_gameInfoDrawRect[2] / 2 + *(int *)&g_gameInfoDrawRect[0],
                      (int)g_pGraphics->resY * 0x17c / 0x1e0, g_optionTextColours, 0x12);
        int idx = OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 2)].max;
        t = FixDiv(OptionMenu_GetRecordPercentage(CGameInfo::GetActiveOptionSlot(), OptionMenu_GetColumnWeight(idx), 1) << 16,
                   0x640000);
        if (t < 0)
            t = 0;
        else if (t > 0x10000)
            t = 0x10000;
        g_gameInfoDrawRect[0] = (short)(((FixMul(t, 0x1350000) + 0x1c0000) >> 16) *
                                     (int)g_pGraphics->resX / 0x280);
        g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0x160 / 0x1e0 + 1);
        g_gameInfoDrawRect[2] = (short)((int)g_pGraphics->resX * 0x12 / 0x280);
        g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0xd / 0x1e0 - 2);
        OptionMenu_DrawBoxEdges(g_gameInfoDrawRect, (BYTE *)&g_optionTextColours[2]);
        t = FixDiv(OptionMenu_GetRecordPercentage(CGameInfo::GetActiveOptionSlot(), OptionMenu_GetColumnWeight(idx), 0) << 16,
                   0x640000);
        if (t < 0)
            t = 0;
        else if (t > 0x10000)
            t = 0x10000;
        g_gameInfoDrawRect[0] = (short)(((FixMul(t, 0x1350000) + 0x1c0000) >> 16) *
                                     (int)g_pGraphics->resX / 0x280);
        g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0x160 / 0x1e0 + 1);
        g_gameInfoDrawRect[2] = (short)((int)g_pGraphics->resX * 0x12 / 0x280);
        g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0xd / 0x1e0 - 2);
        if ((CMain::GetFrameDelta() / 0x14 & 1) == 0)
            Sprite_FillRect(&g_pGraphics->field309_0x150, g_gameInfoDrawRect,
                            (BYTE *)&g_unk0x0052737c, 3);
        w = OptionMenu_GetColumnWeight(idx);
        t = OptionMenu_GetRecordGroupWeight(w);
        base = 0xc;
    } else {
        w = OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 1)].max;
        base = 0xad;
        t = g_unk0x005293c0[w] * 6000;
    }
    sprintf(CFrontend::m_stringDest, g_str0x0051a904, (t / 100) / 60, (t / 100) % 60);
    if (CGameInfo::IsOptionMenuTimeoutPulseOn() != 0)
        Font_DrawText(3, CFrontend::m_stringDest, (int)g_pGraphics->resX * 0x124 / 0x280,
                          (int)g_pGraphics->resY * 0x150 / 0x1e0, &g_unk0x00527378, 0x12);
    else
        Font_DrawText(3, CFrontend::m_stringDest, (int)g_pGraphics->resX * 0x124 / 0x280,
                          (int)g_pGraphics->resY * 0x150 / 0x1e0, &g_unk0x0052737c, 0x12);
    x = (int)g_pGraphics->resX * 0x1c / 0x280;
    base += w * 4;
    y = 0x19a;
    i = 4;
    do {
        OptionMenu_DrawTransitionTextShortCoords(5, 1, 1, CFrontend::GetTextString(base), x,
                     (int)g_pGraphics->resY * y / 0x1e0,
                     g_optionTextColours, g_optionTextBrightColours, 0x11);
        y += 0xe;
        base++;
    } while (--i != 0);
}

// Draws the stage selection panel: the country name, its stage list (the
// current stage highlighted) and the weather forecast, then selects the mode
// of the active slot.
// FUNCTION: CMR2 0x0050c130
void OptionMenu_DrawStageSelectionPanel(Menu *pMenu)
{
    int i;
    int *pRec;

    Font_DrawText(0, CFrontend::GetTextString(0x9c),
                  (int)g_pGraphics->resX * 0x25f / 0x280,
                  (int)g_pGraphics->resY * 100 / 0x1e0, &g_unk0x0052737c, 0xc);
    sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue,
            CFrontend::GetTextString((RallyDataCountryIndex() & 0xff) + 0x93));
    Font_DrawText(0, CFrontend::m_stringDest,
                  (int)g_pGraphics->resX * 0x25f / 0x280,
                  (int)g_pGraphics->resY * 0x78 / 0x1e0, &g_unk0x0052737c, 0xc);
    sprintf(CFrontend::m_stringDest, g_str0x00529624);
    Font_DrawText(0, CFrontend::m_stringDest,
                  (int)g_pGraphics->resX * 0x25f / 0x280,
                  (int)g_pGraphics->resY * 0xa0 / 0x1e0, &g_unk0x0052737c, 0xc);
    for (i = 0; i < CGameInfo::GetPreviewLayoutMode(); i++) {
        sprintf(CFrontend::m_stringDest, g_stageNumberFormat,
                (RallyDataStageIndex() & 0xff) + 1 + i);
        if (i == OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 0)].max) {
            Font_DrawText(0, CFrontend::m_stringDest,
                          (int)g_pGraphics->resX * 0x25f / 0x280,
                          (int)g_pGraphics->resY * 0xb4 / 0x1e0 +
                              (int)g_pGraphics->resY * 0x14 / 0x1e0 * i,
                          &g_unk0x0052737c, 0xc);
        } else {
            Font_DrawText(0, CFrontend::m_stringDest,
                          (int)g_pGraphics->resX * 0x25f / 0x280,
                          (int)g_pGraphics->resY * 0xb4 / 0x1e0 +
                              (int)g_pGraphics->resY * 0x14 / 0x1e0 * i,
                          g_optionTextColours, 0xc);
        }
    }
    pRec = RallyData_GetDriverSettingPair(OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 0)].max);
    sprintf(CFrontend::m_stringDest, g_str0x0052960c,
            CFrontend::GetTextString(*pRec + 0x9d));
    Font_DrawText(0, CFrontend::m_stringDest,
                  (int)g_pGraphics->resX * 100 / 0x280,
                  (int)g_pGraphics->resY * 400 / 0x1e0, g_optionTextColours, 9);
    OptionMenu_SelectAndAnimateSlot(OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 0)].max, 1);
}

// Draws the bottom panel of the options screen: the animated strip of all the
// slots and, when the panel texture is loaded, its background sprite with the
// current slot texture; selects the mode of the active slot afterwards.
// FUNCTION: CMR2 0x0050bfd0
void OptionMenu_DrawSlotStripPanel(Menu *pMenu)
{
    SpriteRect local;

    OptionMenu_DrawAnimatedScreenRows((short)CGameInfo::GetPreviewLayoutMode(),
                 OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 0)].max,
                 0, g_optionLayoutXScale, 0x124, 0);
    if (g_countryBannerTexture != 0) {
        g_gameInfoDrawRect[0] = (short)(g_optionLayoutXScale * (int)g_pGraphics->resX / 0x280);
        g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0x6f / 0x1e0);
        g_gameInfoDrawRect[2] = (short)((int)g_pGraphics->resX * 0xa1 / 0x280);
        g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0x37 / 0x1e0);
        local = *(SpriteRect *)g_gameInfoDrawRect;
        local.y = 0;
        local.x = 0;
        OptionMenu_AnimateLayoutRectangleEdge(2, g_gameInfoDrawRect);
        Sprite_Queue(&local, (SpriteRect *)g_gameInfoDrawRect, g_countryBannerTexture,
                     1, 0, NULL, NULL, g_colour0x005273a8, 8);
    }
    OptionMenu_SelectAndAnimateSlot(OptionMenu_GetControlSetupMenu()->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 0)].max,
                 0);
}


// ===== Agent W171.GiD: results screens (0x50c420, 0x50f650) =====

extern char g_classRowHeaderFormat[];  // 0x00516098 "%s %d" (GameMenus.cpp)
extern char g_keypadFormat[];          // 0x00516940 "%s %s" (Input.cpp)
extern char g_strLabelText[];          // 0x00524d38 "%s: %s" (FrontendScreens.cpp)

#include "StageInfo.h"
BYTE FrontendRecords_GetStageEntryPayload(StageSurfaceShare **out, int row, int column);
const char *FrontendRecords_GetStageName(int row, int column);
StageInfoEntry *FrontendRecords_GetStageEntry(int row, int column);
BYTE RallyData_GetDriverGridRow(BYTE param1, char param2);
int RallyData_GetIndexedStageScoreScale(int row, int column, int variant);

// Space width of the menu font (0x00516870, inside the rally text table below);
// the original has no separate symbol for it, so it is a view into the table.
#include "CountryCarSetups.h"
extern CountryCarSetupBlock g_countryCarSetupBlock;

// Run of spaces that separates the two columns of the class table.
// GLOBAL: CMR2 0x005297bc
char g_str0x005297bc[] = "    ";

// Formats of the stage information panel.
// GLOBAL: CMR2 0x00524d0c
char g_str0x00524d0c[] = "%d%% %s";
// GLOBAL: CMR2 0x00524d14
char g_str0x00524d14[] = "%s: ";
// GLOBAL: CMR2 0x00524d1c
char g_str0x00524d1c[] = "km";
// GLOBAL: CMR2 0x00524d20
char g_str0x00524d20[] = "%s: %d.%d";
// GLOBAL: CMR2 0x00524d2c
char g_str0x00524d2c[] = "%s: %d.0";

// Scratch rectangle (x, y, w, h) of the screen rows.

// Draws the results screen: one row per menu option with the option text, its
// value and the separator line between the rows, then the screen title, the
// rally record and finally the stage time. The selected row is drawn with the
// pulsed menu colour, blended against the shared grey.
// match 72%: the instruction sequence is the original's (same calls, same 16.16
// blend through FixMul/FixDiv), but MSVC 6 packs this function's frame
// differently: it gives the `colour` local the dead parameter slot [ebp+8] and
// the fixed-point/FILD temporaries -0x14..-0x4, while the original keeps colour
// at [ebp-0x14] and spills the temporaries/factor to [ebp+8]/[ebp-0xc]/[ebp-8]/
// [ebp-4]. Every mismatch in the two blend blocks is that slot swap; the block
// ordering and the switch lowering are identical.
// FUNCTION: CMR2 0x0050c420
void OptionMenu_DrawResultsOptions(int param_1)
{
    int colour;
    int frames;
    int mode;
    int i;
    int a;
    int b;
    int x;
    int f;
    unsigned int remaining;

    x = (int)g_pGraphics->resX * 0x1c / 0x280;
    colour = *(int *)OptionMenu_GetStateByteAddress();
    if (param_1 != 0)
        colour = g_unk0x0052737c;
    mode = OptionMenu_GetSlotType(OptionMenu_FindFirstDetailedSlot());
    g_gameInfoDrawRect[2] = 1;
    g_gameInfoDrawRect[3] = (short)((int)g_pGraphics->resY * 0x18 / 0x1e0);
    g_gameInfoDrawRect[1] = (short)((int)g_pGraphics->resY * 0x1c / 0x1e0);
    frames = OptionMenu_GetScrollDistance();
    i = 0;
    switch (mode) {
    case 0:
        f = 0x10000 - FixDiv(0x10000 - (int)FTOI(__int64, X87S_MUL((double)(unsigned)frames, CGraphics::m_65536)), 0x10000);
        FixMul((int)FTOI(__int64, X87S_MUL((double)(colour & 0xff), CGraphics::m_65536)), f);
        FixMul((int)FTOI(__int64, X87S_MUL((double)(g_optionTextColours[0] & 0xff), CGraphics::m_65536)), 0x10000 - f);
        FixMul((int)FTOI(__int64, X87S_MUL((double)(colour >> 8 & 0xff), CGraphics::m_65536)), f);
        FixMul((int)FTOI(__int64, X87S_MUL((double)(g_optionTextColours[0] >> 8 & 0xff), CGraphics::m_65536)), 0x10000 - f);
        FixMul((int)FTOI(__int64, X87S_MUL((double)(colour >> 16 & 0xff), CGraphics::m_65536)), f);
        FixMul(0x10000 - f, (int)FTOI(__int64, X87S_MUL((double)(g_optionTextColours[0] >> 16 & 0xff), CGraphics::m_65536)));
        break;
    case 1:
        break;
    case 2:
        f = 0x10000 - FixDiv(0x10000 - (int)FTOI(__int64, X87S_MUL((double)(unsigned)frames, CGraphics::m_65536)), 0x10000);
        FixMul(f, (int)FTOI(__int64, X87S_MUL((double)(g_optionTextColours[0] & 0xff), CGraphics::m_65536)));
        FixMul((int)FTOI(__int64, X87S_MUL((double)(colour & 0xff), CGraphics::m_65536)), 0x10000 - f);
        FixMul((int)FTOI(__int64, X87S_MUL((double)(g_optionTextColours[0] >> 8 & 0xff), CGraphics::m_65536)), f);
        FixMul((int)FTOI(__int64, X87S_MUL((double)(colour >> 8 & 0xff), CGraphics::m_65536)), 0x10000 - f);
        FixMul((int)FTOI(__int64, X87S_MUL((double)(g_optionTextColours[0] >> 16 & 0xff), CGraphics::m_65536)), f);
        FixMul(0x10000 - f, (int)FTOI(__int64, X87S_MUL((double)(colour >> 16 & 0xff), CGraphics::m_65536)));
        break;
    }
    while (i < (signed char)OptionMenu_GetControlSetupMenu()->itemCount) {
        a = OptionMenu_FindFirstDetailedSlot();
        b = OptionMenu_FindFirstDetailedSlot();
        if (CGameInfo::GetConfiguredGameMode() != 0 && i == 4)
            a--;
        if (CGameInfo::GetConfiguredGameMode() != 0 && i == 3)
            b--;
        if (a == i)
            OptionMenu_DrawTransitionText(6, 0, 0,
                         CFrontend::GetTextString(OptionMenu_GetControlSetupMenu()->items[i].id),
                         x, (int)g_pGraphics->resY * 0x2c / 0x1e0, &colour, g_optionTextColours, 0x11);
        else
            Font_DrawText(0,
                          CFrontend::GetTextString(OptionMenu_GetControlSetupMenu()->items[i].id),
                          x, (int)g_pGraphics->resY * 0x2c / 0x1e0, g_optionTextColours, 0x11);
        x += Font_GetTextWidth(0, (BYTE *)CFrontend::GetTextString(
                                      OptionMenu_GetControlSetupMenu()->items[i].id));
        x += Font_GetTextWidth(0, (BYTE *)g_countryCarSetupBlock.space);
        g_gameInfoDrawRect[0] = (short)x;
        if (i != (signed char)OptionMenu_GetControlSetupMenu()->itemCount - 1) {
            if (a == i || b == i + 1)
                Sprite_FillRect(&g_pGraphics->field309_0x150, g_gameInfoDrawRect, (BYTE *)&colour, 3);
            else
                Sprite_FillRect(&g_pGraphics->field309_0x150, g_gameInfoDrawRect,
                                (BYTE *)(g_optionTextColours + 2), 3);
        }
        x += 2 + Font_GetTextWidth(0, (BYTE *)g_countryCarSetupBlock.space);
        i++;
    }
    x = g_optionLayoutXScale * (int)g_pGraphics->resX / 0x280;
    sprintf(CFrontend::m_stringDest, g_classRowHeaderFormat, CFrontend::GetTextString(0x4a),
            CGameInfo::GetActiveOptionSlot() + 1);
    Font_DrawText(0, CFrontend::m_stringDest, x, (int)g_pGraphics->resY * 0x2c / 0x1e0,
                  g_optionTextColours, 0x11);
    x += Font_GetTextWidth(0, (BYTE *)CFrontend::m_stringDest);
    x += Font_GetTextWidth(0, (BYTE *)g_countryCarSetupBlock.space);
    g_gameInfoDrawRect[0] = (short)x;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_gameInfoDrawRect, (BYTE *)(g_optionTextColours + 2), 3);
    x += 2 + Font_GetTextWidth(0, (BYTE *)g_countryCarSetupBlock.space);
    Font_DrawText(0, (char *)RallyData_GetRecord((BYTE)CGameInfo::GetActiveOptionSlot()), x,
                  (int)g_pGraphics->resY * 0x2c / 0x1e0, g_optionTextColours, 0x11);
    if (CGameInfo::GetGameModeOptionBit19() == 0)
        return;
    x = (int)g_pGraphics->resX * 0x1e / 0x280 + x +
        Font_GetTextWidth(0, (BYTE *)RallyData_GetRecord((BYTE)CGameInfo::GetActiveOptionSlot()));
    if (CGameInfo::GetConfiguredGameMode() == '\n') {
        unsigned int limit;
        unsigned int delta;

        if (GameInfo_GetSessionField397C() == 0)
            return;
        delta = CMain::GetFrameDelta() - NetPlayers_GetInputFrameDelta();
        limit = (unsigned int)(GameInfo_GetSessionField397C() * 6000);
        if (delta >= limit)
            remaining = 0;
        else
            remaining = limit - delta;
    } else {
        remaining = (unsigned int)CGameInfo::GetOptionMenuRemainingTime();
    }
    sprintf(CFrontend::m_stringDest, g_str0x0051a904, (remaining / 100) / 0x3c,
            (remaining / 100) % 0x3c);
    Font_DrawText(2, CFrontend::m_stringDest, x, (int)g_pGraphics->resY * 0x2c / 0x1e0,
                  g_optionTextColours, 0x11);
}

// Draws the stage summary panel: the event header (special stage or round), the
// stage name, the record time, the surface and severity values, the distance,
// the class rows in two columns and the panel title, plus the animated divider
// bar on the left.
// match 88%: same instructions; the residual is the frame layout. The original
// allocates 0x14 bytes (bar at [esp+8..0xf], v0/v1 at [esp+0x10], table at
// [esp+0x14] plus one unused 4-byte slot at [esp+0xc]); this build allocates
// 0x10 bytes (table at [esp+0x10]), so every frame-relative operand and the
// scheduling of `bar[2] = 1` differ. Source-level reordering (declaration order,
// separate pointer/second-column variables) does not move MSVC 6's slots.
// FUNCTION: CMR2 0x0050f650
void OptionMenu_DrawStageSummaryPanel(int param_1, int param_2)
{
    short bar[4];
    StageSurfaceShare *table;
    int x;
    int i;
    int count;
    int limit;
    int y;
    int w;
    int w2;
    int column;
    BYTE v0;
    BYTE v1;

    bar[2] = 1;
    x = (int)g_pGraphics->resX * 0x1c2 / 0x280;
    bar[3] = (short)((int)g_pGraphics->resY * 0x18 / 0x1e0);
    bar[1] = (short)((int)g_pGraphics->resY * 0xac / 0x1e0);
    OptionMenu_EaseRecordBar(0, bar, 0, 1);
    if (CGameInfo::GetConfiguredGameMode() != 0 && CGameInfo::GetConfiguredGameMode() != 1) {
        if (param_2 == 10)
            sprintf(CFrontend::m_stringDest, g_keypadFormat, CFrontend::GetTextString(0x9b),
                    CFrontend::GetTextString(0x13d));
        else
            sprintf(CFrontend::m_stringDest, g_classRowHeaderFormat, CFrontend::GetTextString(0x9b),
                    param_2 + 1);
        OptionMenu_DrawTransitionTextShortCoords(3, 0, 0, CFrontend::m_stringDest, x, (int)g_pGraphics->resY * 0xbe / 0x1e0,
                     g_unk0x005297ac, g_unk0x005297ac + 2, 0x11);
    } else {
        int value;
        int right;

        if (param_2 == 10) {
            switch (CGameInfo::GetConfiguredDifficulty()) {
            case 0:
                value = 2;
                break;
            case 2:
                value = 4;
                break;
            default:
                value = 3;
                break;
            }
        } else {
            value = param_2 / 4 + 1;
        }
        sprintf(CFrontend::m_stringDest, g_classRowHeaderFormat, CFrontend::GetTextString(0xfa),
                value);
        OptionMenu_DrawTransitionTextShortCoords(3, 0, 0, CFrontend::m_stringDest, x, (int)g_pGraphics->resY * 0xbe / 0x1e0,
                     g_unk0x005297ac, g_unk0x005297ac + 2, 0x11);
        right = Font_GetTextWidth(0, (BYTE *)CFrontend::m_stringDest) + x;
        right += Font_GetTextWidth(0, (BYTE *)g_countryCarSetupBlock.space);
        bar[0] = (short)right;
        Sprite_FillRect(&g_pGraphics->field309_0x150, bar, (BYTE *)(g_unk0x005297ac + 3), 3);
        right += 2 + Font_GetTextWidth(0, (BYTE *)g_countryCarSetupBlock.space);
        if (param_2 == 10)
            sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x13c),
                    RallyData_GetDriverGridRow(RallyDataCountryIndex(), CGameInfo::GetConfiguredDifficulty()) & 0xff,
                    RallyData_GetDriverGridRow(RallyDataCountryIndex(), CGameInfo::GetConfiguredDifficulty()) & 0xff);
        else
            sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x13c), param_2 + 1,
                    RallyData_GetDriverGridRow(RallyDataCountryIndex(), CGameInfo::GetConfiguredDifficulty()) & 0xff);
        OptionMenu_DrawTransitionTextShortCoords(3, 0, 0, CFrontend::m_stringDest, right, (int)g_pGraphics->resY * 0xbe / 0x1e0,
                     g_unk0x005297ac, g_unk0x005297ac + 2, 0x11);
    }
    sprintf(CFrontend::m_stringDest, g_strLabelText, CFrontend::GetTextString(0xff),
            FrontendRecords_GetStageName(param_1, param_2));
    OptionMenu_DrawTransitionTextShortCoords(3, 1, 1, CFrontend::m_stringDest, x, (int)g_pGraphics->resY * 0xd2 / 0x1e0,
                 g_unk0x005297ac, g_unk0x005297ac + 2, 0x11);
    sprintf(CFrontend::m_stringDest, g_str0x005297c4, CFrontend::GetTextString(0xfb),
            RallyData_GetIndexedStageScoreScale(param_1, param_2, 0) / 100);
    OptionMenu_DrawTransitionTextShortCoords(3, 1, 1, CFrontend::m_stringDest, x, (int)g_pGraphics->resY * 0xe0 / 0x1e0,
                 g_unk0x005297ac, g_unk0x005297ac + 2, 0x11);
    v0 = FrontendRecords_GetStageEntry(param_1, param_2)->lengthKm;
    v1 = FrontendRecords_GetStageEntry(param_1, param_2)->lengthTenths;
    if (v1 == 0)
        sprintf(CFrontend::m_stringDest, g_str0x00524d2c, CFrontend::GetTextString(0xfc), v0);
    else
        sprintf(CFrontend::m_stringDest, g_str0x00524d20, CFrontend::GetTextString(0xfc), v0, v1);
    strcat(CFrontend::m_stringDest, g_str0x00524d1c);
    OptionMenu_DrawTransitionTextShortCoords(3, 1, 1, CFrontend::m_stringDest, x, (int)g_pGraphics->resY * 0xee / 0x1e0,
                 g_unk0x005297ac, g_unk0x005297ac + 2, 0x11);
    sprintf(CFrontend::m_stringDest, g_str0x00524d14, CFrontend::GetTextString(0xfd));
    OptionMenu_DrawTransitionTextShortCoords(3, 1, 1, CFrontend::m_stringDest, x, (int)g_pGraphics->resY * 0xfc / 0x1e0,
                 g_unk0x005297ac, g_unk0x005297ac + 2, 0x11);
    count = FrontendRecords_GetStageEntryPayload(&table, param_1, param_2) & 0xff;
    limit = count;
    if (limit > 1)
        limit = 2;
    w = 0;
    y = 0x10a;
    for (i = 0; i < limit; i++) {
        sprintf(CFrontend::m_stringDest, g_str0x00524d0c, table[i].percent,
                CFrontend::GetTextString(table[i].surface + 0xd4));
        OptionMenu_DrawTransitionTextShortCoords(3, 1, 1, CFrontend::m_stringDest, x, (int)g_pGraphics->resY * y / 0x1e0,
                     g_unk0x005297ac, g_unk0x005297ac + 2, 0x11);
        y += 0xe;
        w2 = Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
        if (w2 > w)
            w = w2;
    }
    column = Font_GetTextWidth(1, (BYTE *)g_str0x005297bc) + w + x;
    y = 0x10a;
    for (i = limit; i < count; i++) {
        sprintf(CFrontend::m_stringDest, g_str0x00524d0c, table[i].percent,
                CFrontend::GetTextString(table[i].surface + 0xd4));
        OptionMenu_DrawTransitionTextShortCoords(3, 1, 1, CFrontend::m_stringDest, column, (int)g_pGraphics->resY * y / 0x1e0,
                     g_unk0x005297ac, g_unk0x005297ac + 2, 0x11);
        y += 0xe;
    }
    sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x13e));
    OptionMenu_DrawTransitionTextShortCoords(3, 1, 1, CFrontend::m_stringDest, x, (int)g_pGraphics->resY * 0x17d / 0x1e0,
                 g_unk0x005297ac, g_unk0x005297ac + 2, 0x21);
}

/* ===== integrated from w171/GiB ===== */
BYTE RallyData_GetDriverGridRow(BYTE param1, char param2);
extern char g_keypadFormat[];
extern char g_strLabelText[];

// Colour of a highlighted row (light grey).
// GLOBAL: CMR2 0x00527398
int g_highlightRowColour = -2142286001;
// Colour of a highlighted row (transparent white).
// x/y/w/h of the background bar of the row being drawn.
// Name of the current rally/weather of the rally screen rows.
// GLOBAL: CMR2 0x0082aa44
char g_unk0x0082aa44[0x100];
// Row of the highlighted weather entry: "%s: %s_".
// GLOBAL: CMR2 0x005295f8
char g_str0x005295f8[] = "%s: %s_";
// Row of the rally menu: "%s %d (%s)".
// GLOBAL: CMR2 0x00529600
char g_str0x00529600[] = "%s %d (%s)";
// Format drawn by the rows with no text.

/* ===== W194: saved-game image + option menu callbacks (0x4f5190-0x5004xx) ===== */

extern void RallyData_CopyDriverStageTimesToRecords(void);
extern BYTE RallyData_GetDriverAutomaticGearbox(BYTE param1);
extern BYTE *RallyData_GetDriverEntryRecord(int index);
extern int RallyTiming_GetStageTieBreak(int index);
extern BYTE *RallyData_GetDriverCategoryProfile(int index);
extern CarSetup *RallyData_GetPlayerCarSetup(int index);
extern int *RallyData_GetDriverSettingPair(int index);
extern int *RallyData_GetDriverPrimaryPairRecord(int index);
extern BYTE *RallyData_GetDriverSelectionFlagRecord(int index);
extern int *RallyData_GetDriverPairRecord(int index);
extern int *RallyData_GetDriverSecondaryPairRecord(int index);
extern unsigned char RallyDataCountryIndex(void);
extern unsigned char RallyDataStageIndex(void);
extern BYTE RallyData_GetSelectionStateByte(void);
extern void OptionMenu_ConfirmOrRevertHighlightedValue(Menu *, MenuItem *);
extern void OptionMenu_RevertRallyRecordGroup(int param_1, int param_2);
extern void OptionMenu_AdvanceSharedModeValue(void);
extern void OptionMenu_Start(char param1);
extern void OptionMenu_ClearRecordTransition(int index);
extern void OptionMenu_ClearTimeout(void);
extern void OptionMenu_UpdateTimeout(void);
extern void OptionPreview_SetWheelNodesVisible(int index, char visible);
extern void OptionMenu_FillValueSlider(void);
extern void OptionMenu_ConfirmHighlightedItem(Menu *pMenu);
extern int OptionMenu_IsSlotEnabled(int mode, int index);
extern int OptionMenu_GetColumnLabelId(int index);
extern int OptionMenu_GetSlotSelector(int index);
extern void OptionMenu_SetSlotSelector(int index, int value);
extern unsigned int OptionMenu_GetRecordPercentage(int index, int type, int dynamic);
extern void OptionPreview_RestoreOriginalMeshVertices(int index);
extern int g_unk0x005270b8[11];

// One object owns the entire original disk image; unrelated globals need not
// be adjacent or have the same alignment as the original executable.
// GLOBAL: CMR2 0x0081a858
static SavedGameImage g_savedGameImage;
// GLOBAL: CMR2 0x00525c20
char g_str0x00525c20[] = "%s\\gamesave\\%s";

// Builds the original session image. A name creates a new file; otherwise
// the selected file is overwritten, preserving its displayed name.
// FUNCTION: CMR2 0x004f5190
BYTE SavedGames_WriteSessionImage(int index, const char *name)
{
    SavedGameImage &image = g_savedGameImage;
    memset(&image, 0, sizeof(image));
    if (CGameInfo::GetConfiguredPlayerCount() < 1 || CGameInfo::GetConfiguredPlayerCount() > 4)
        return 0;
    RallyData_CopyDriverStageTimesToRecords();

    image.options.mode = CGameInfo::GetConfiguredGameMode();
    image.options.difficulty = CGameInfo::GetConfiguredDifficulty();
    image.options.playerCount = CGameInfo::GetConfiguredPlayerCount();
    image.options.multiplayer = CGameInfo::IsConfiguredMultiplayer();

    {
    for (int i = 0; i < 16; i++) {
        const SaveSlot &slot = *(const SaveSlot *)RallyData_GetDriverEntryRecord(i);
        image.drivers[i].car = slot.car;
        image.drivers[i].seconds = slot.seconds;
        image.drivers[i].time = slot.time;
        image.tieBreak[i] = (BYTE)RallyTiming_GetStageTieBreak(i);
    }
    }
    {
    for (unsigned i = 0; i < image.options.playerCount; i++) {
        const DriverProfile *profile = (const DriverProfile *)RallyData_GetDriverCategoryProfile(i);
        if (!profile)
            return 0;
        image.drivers[i].entryFlag = RallyData_GetDriverAutomaticGearbox((BYTE)i);
        image.playerSetup[i] = profile->setup;
        image.playerSetup[i].reserved = 0;
        image.playerIdentifiers[i] = profile->identifier;
        memcpy(&image.playerCars[i], RallyData_GetDriverGroupRecord(i), sizeof(image.playerCars[i]));
        memcpy(&image.playerCarSetups[i], RallyData_GetPlayerCarSetup(i), sizeof(image.playerCarSetups[i]));
    }
    }

    memcpy(image.settingPairs, RallyData_GetDriverSettingPair(0), sizeof(image.settingPairs));
    memcpy(image.primaryValues, RallyData_GetDriverPrimaryPairRecord(0), sizeof(image.primaryValues));
    memcpy(image.selectionFlags, RallyData_GetDriverSelectionFlagRecord(0), sizeof(image.selectionFlags));
    memcpy(image.values, RallyData_GetDriverPairRecord(0), sizeof(image.values));
    memcpy(image.secondaryValues, RallyData_GetDriverSecondaryPairRecord(0), sizeof(image.secondaryValues));
    image.stage.country = RallyDataCountryIndex();
    image.stage.stage = RallyDataStageIndex();
    image.selectionState = RallyData_GetSelectionStateByte();

    if (name != NULL) {
        _snprintf(image.name, sizeof(image.name), "%s", name);
        image.name[sizeof(image.name) - 1] = 0;
        SavedGames_MakeUnusedFileName(CFrontend::m_stringDest);
    } else if (index >= 0 && index < SavedGames_GetCount()) {
        strcpy(image.name, g_pSavedGames[index].name);
        sprintf(CFrontend::m_stringDest, g_str0x00525c20, CInstallInfo::GetGameHDPath(),
                (char *)g_savedGameFileNames[index]);
    } else {
        return 0;
    }
    if (!SavedGames_IsValidSessionImage(&image, sizeof(image)))
        return 0;
    return CInstallInfo::WriteFileToDisk(CFrontend::m_stringDest, 0, &image, sizeof(image)) == 1;
}

// Loads the highlighted saved game into the frontend and moves to the game
// screen; if the slot is empty it only reports the failure.
// FUNCTION: CMR2 0x00500020
void OptionMenu_LoadSelectedSavedGame(unsigned int param1, unsigned int param2)
{
    if (SavedGames_WriteSessionImage(g_savedGameListSelection, NULL)) {
        strcpy(g_optionStatusLine, CFrontend::GetTextString(0x143));
        Menu_SetNextAction(OptionMenu_GetStatusMenu());
        SavedGames_LoadRecords();
    } else {
        strcpy(g_optionStatusLine, CFrontend::GetTextString(0x144));
        Menu_SetNextAction(OptionMenu_GetStatusMenu());
    }
}

// "Load game" entry of the option menu: arms the load request and, once the
// records were re-read, starts the frontend on the selected saved game.
// FUNCTION: CMR2 0x004ffed0
void OptionMenu_LoadGameItem(Menu *pMenu, MenuItem *pItem)
{
    if (g_unk0x0082ab44 != 0) {
        Menu_SetNextAction(OptionMenu_GetAdvancedOptionsMenu());
        return;
    }
    if (pMenu->items[4].max == 0 && SavedGames_GetCount() > 0) {
        g_unk0x0082ab44 = 1;
        Menu_SetFlags(pMenu, 0, 0, 0, 0);
        return;
    }
    if (pMenu->items[4].max == 1) {
        if (SavedGames_WriteSessionImage(-1, g_unk0x0082aa44) != 0) {
            strcpy(g_unk0x0082aa44, CMain::m_logFileBlankLine);
            strcpy(g_optionStatusLine, CFrontend::GetTextString(0x143));
            Menu_SetNextAction(OptionMenu_GetStatusMenu());
            SavedGames_LoadRecords();
            g_savedGameListSelection = SavedGames_GetCount() - 1;
        } else {
            strcpy(g_optionStatusLine, CFrontend::GetTextString(0x144));
            Menu_SetNextAction(OptionMenu_GetStatusMenu());
        }
    }
}

// Applies the value of the highlighted entry of the option menu when it was
// confirmed: only stores it when it is a valid change for this slot.
// FUNCTION: CMR2 0x004fffe0
void OptionMenu_ApplyHighlightedValue(Menu *pMenu, MenuItem *pItem)
{
    if (OptionMenu_GetRecordGroupAppliedFlag(CGameInfo::GetActiveOptionSlot(), OptionMenu_GetColumnWeight(pItem->max)) != 0)
        OptionMenu_ApplySecondModeItem(pMenu, pItem);
    else
        OptionMenu_ConfirmOrRevertHighlightedValue(pMenu, pItem);
}

// Confirms the value of the highlighted entry of the option menu: applies it
// when the slot still had a different one, or reverts the highlighted row.
// FUNCTION: CMR2 0x00500270
void OptionMenu_ConfirmOrRevertHighlightedValue(Menu *pMenu, MenuItem *pItem)
{
    int index;

    index = Menu_FindItem(OptionMenu_GetControlSetupMenu(), 2);
    if (OptionMenu_GetSlotSelector(CGameInfo::GetActiveOptionSlot()) - OptionMenu_GetRecordGroupWeight(OptionMenu_GetColumnWeight((OptionMenu_GetControlSetupMenu())->items[index].max)) < 0) {
        if (OptionMenu_GetRecordGroupAppliedFlag(CGameInfo::GetActiveOptionSlot(), OptionMenu_GetColumnWeight((OptionMenu_GetControlSetupMenu())->items[index].max)) == 0) {
            if ((int)OptionMenu_GetRecordPercentage(CGameInfo::GetActiveOptionSlot(), OptionMenu_GetColumnWeight((OptionMenu_GetControlSetupMenu())->items[index].max), 0) > 0) {
                CGameInfo::StartOptionMenuTimeout();
                return;
            }
            return;
        }
    }
    if (OptionMenu_GetRecordGroupAppliedFlag(CGameInfo::GetActiveOptionSlot(), OptionMenu_GetColumnWeight((OptionMenu_GetControlSetupMenu())->items[index].max)) == 0)
        OptionMenu_RevertRallyRecordGroup(CGameInfo::GetActiveOptionSlot(), OptionMenu_GetColumnWeight((OptionMenu_GetControlSetupMenu())->items[index].max));
    Menu_SetNextAction(pMenu->pParent);
}

// Reverts one option group of the current slot: clears the group's "changed"
// flags and subtracts the group's weight from the slot value.
// (Not listed in functions.tsv; it is the counter part of OptionMenu_ApplyRallyRecordGroup.)
// match 76%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00503010
void OptionMenu_RevertRallyRecordGroup(int param_1, int param_2)
{
    RallyCarDamageState *pDest = &g_playerOptionCache.records[param_1];
    int value;
    int i;

    switch (param_2) {
    case 1:
        if (pDest->damage.intensity[14] == 0)
            return;
        pDest->damage.intensity[14] = 0;
        value = OptionMenu_GetSlotSelector(param_1);
        OptionMenu_SetSlotSelector(param_1, value - g_unk0x005270b8[0]);
        g_playerOptionCache.dirty[param_1][1] = 1;
        return;
    case 2:
        if (pDest->damage.intensity[15] == 0)
            return;
        pDest->damage.intensity[15] = 0;
        value = OptionMenu_GetSlotSelector(param_1);
        OptionMenu_SetSlotSelector(param_1, value - g_unk0x005270b8[1]);
        g_playerOptionCache.dirty[param_1][2] = 1;
        return;
    case 3:
        if (pDest->damage.intensity[6] == 0 && pDest->damage.intensity[7] == 0 && pDest->damage.intensity[8] == 0 && pDest->damage.intensity[9] == 0)
            return;
        pDest->damage.intensity[6] = 0;
        pDest->damage.intensity[7] = 0;
        pDest->damage.intensity[8] = 0;
        pDest->damage.intensity[9] = 0;
        value = OptionMenu_GetSlotSelector(param_1);
        OptionMenu_SetSlotSelector(param_1, value - g_unk0x005270b8[2]);
        OptionPreview_ClearCornerHeightOffsets(param_1);
        g_playerOptionCache.dirty[param_1][3] = 1;
        return;
    case 4:
        if (pDest->damage.intensity[16] == 0)
            return;
        pDest->damage.intensity[16] = 0;
        value = OptionMenu_GetSlotSelector(param_1);
        OptionMenu_SetSlotSelector(param_1, value - g_unk0x005270b8[3]);
        g_playerOptionCache.dirty[param_1][4] = 1;
        return;
    case 5:
        if (pDest->damage.intensity[10] == 0 && pDest->damage.intensity[11] == 0 && pDest->damage.intensity[12] == 0 && pDest->damage.intensity[13] == 0)
            return;
        pDest->damage.intensity[10] = 0;
        pDest->damage.intensity[11] = 0;
        pDest->damage.intensity[12] = 0;
        pDest->damage.intensity[13] = 0;
        value = OptionMenu_GetSlotSelector(param_1);
        OptionMenu_SetSlotSelector(param_1, value - g_unk0x005270b8[4]);
        g_playerOptionCache.dirty[param_1][5] = 1;
        return;
    case 6:
        {
            int any = 0;
            int changed = 0;
            int has104 = 0;

            if (pDest->damage.intensity[17] != 0 || pDest->damage.intensity[18] != 0 || pDest->damage.intensity[19] != 0 ||
                pDest->damage.intensity[20] != 0 || pDest->damage.intensity[21] != 0 || pDest->damage.intensity[22] != 0 ||
                pDest->damage.intensity[26] != 0 || pDest->damage.intensity[27] != 0 || pDest->damage.intensity[28] != 0 ||
                pDest->damage.intensity[29] != 0 || pDest->damage.intensity[30] != 0 || pDest->damage.intensity[31] != 0 ||
                pDest->damage.intensity[32] != 0 || pDest->damage.intensity[33] != 0)
                any = 1;
            for (i = 0; i < 3; i++)
                if (pDest->damage.lineGrounded[i] != 0)
                    changed = 1;
            for (i = 0; i < 4; i++)
                if (pDest->damage.partHidden[i] != 0)
                    changed = 1;
            if (pDest->impacts.count != 0)
                has104 = 1;
            if (any != 0 || changed != 0 || has104 != 0) {
                pDest->damage.intensity[17] = 0;
                pDest->damage.intensity[18] = 0;
                pDest->damage.intensity[19] = 0;
                pDest->damage.intensity[20] = 0;
                pDest->damage.intensity[21] = 0;
                pDest->damage.intensity[22] = 0;
                pDest->damage.intensity[26] = 0;
                pDest->damage.intensity[27] = 0;
                pDest->damage.intensity[28] = 0;
                pDest->damage.intensity[29] = 0;
                pDest->damage.intensity[30] = 0;
                pDest->damage.intensity[31] = 0;
                pDest->damage.intensity[32] = 0;
                pDest->damage.intensity[33] = 0;
                pDest->damage.lineGrounded[0] = 0;
                pDest->damage.lineGrounded[1] = 0;
                pDest->damage.lineGrounded[2] = 0;
                pDest->damage.partHidden[0] = 0;
                pDest->damage.partHidden[1] = 0;
                pDest->damage.partHidden[2] = 0;
                pDest->damage.partHidden[3] = 0;
                pDest->impacts.count = 0;
                pDest->impacts.firstLink = 0;
                for (i = 0; i < 20; i++) {
                    pDest->impacts.links[i].impact.strength = 0;
                    pDest->impacts.links[i].impact.mode = 0;
                    pDest->impacts.links[i].impact.axisRadius = 0;
                    pDest->impacts.links[i].impact.normal[0] = 0;
                    pDest->impacts.links[i].impact.normal[1] = 0;
                    pDest->impacts.links[i].impact.normal[2] = 0;
                    pDest->impacts.links[i].impact.axis[0] = 0;
                    pDest->impacts.links[i].impact.axis[1] = 0;
                    pDest->impacts.links[i].impact.axis[2] = 0;
                    pDest->impacts.links[i].impact.position[0] = 0;
                    pDest->impacts.links[i].impact.position[1] = 0;
                    pDest->impacts.links[i].impact.position[2] = 0;
                    pDest->impacts.links[i].next = 0xff;
                }
                value = OptionMenu_GetSlotSelector(param_1);
                OptionMenu_SetSlotSelector(param_1, value - g_unk0x005270b8[5]);
                g_playerOptionCache.dirty[param_1][6] = 1;
            }
            OptionPreview_RestoreOriginalMeshVertices(param_1);
            return;
        }
    case 7:
        if (pDest->damage.intensity[4] == 0 && pDest->damage.intensity[5] == 0)
            return;
        pDest->damage.intensity[4] = 0;
        pDest->damage.intensity[5] = 0;
        value = OptionMenu_GetSlotSelector(param_1);
        OptionMenu_SetSlotSelector(param_1, value - g_unk0x005270b8[6]);
        g_playerOptionCache.dirty[param_1][7] = 1;
        return;
    case 8:
        if (pDest->damage.intensity[23] == 0)
            return;
        pDest->damage.intensity[23] = 0;
        value = OptionMenu_GetSlotSelector(param_1);
        OptionMenu_SetSlotSelector(param_1, value - g_unk0x005270b8[7]);
        g_playerOptionCache.dirty[param_1][8] = 1;
        return;
    case 9:
        if (pDest->damage.intensity[24] == 0)
            return;
        pDest->damage.intensity[24] = 0;
        value = OptionMenu_GetSlotSelector(param_1);
        OptionMenu_SetSlotSelector(param_1, value - g_unk0x005270b8[8]);
        g_playerOptionCache.dirty[param_1][9] = 1;
        return;
    case 10:
        if (pDest->damage.intensity[25] == 0)
            return;
        pDest->damage.intensity[25] = 0;
        value = OptionMenu_GetSlotSelector(param_1);
        OptionMenu_SetSlotSelector(param_1, value - g_unk0x005270b8[9]);
        g_playerOptionCache.dirty[param_1][10] = 1;
        return;
    case 11:
        if (pDest->damage.intensity[0] == 0 && pDest->damage.intensity[1] == 0 && pDest->damage.intensity[2] == 0 && pDest->damage.intensity[3] == 0)
            return;
        pDest->damage.intensity[0] = 0;
        pDest->damage.intensity[1] = 0;
        pDest->damage.intensity[2] = 0;
        pDest->damage.intensity[3] = 0;
        value = OptionMenu_GetSlotSelector(param_1);
        OptionMenu_SetSlotSelector(param_1, value - g_unk0x005270b8[10]);
        OptionPreview_ClearPartDamageFractions(param_1);
        g_playerOptionCache.dirty[param_1][11] = 1;
        return;
    }
}

extern void OptionMenu_UpdateLoadGameAndNameEntry(Menu *pMenu);
extern void OptionMenu_UpdateMessagesAndRecordStates(unsigned int param1);
extern bool Input_PopQueuedCharacter(int *pOut);
extern void Input_ClearCharacterQueue(void);

// Name of a saved game being edited (0x526fb8 is the accepted character set).
// GLOBAL: CMR2 0x00526fb8
char g_str0x00526fb8[] = "abcdefghijklmnopqrstuvwxyz. ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
// Buffer whose contents the name editor is editing.
// GLOBAL: CMR2 0x0082a934
char *g_nameEditorBuffer;
// Set when the name editor was opened this frame.
// GLOBAL: CMR2 0x00526f40
BYTE g_nameEditorOpened = 1;

// Callback of the load-game screen of the option menu: scrolls the saved-games
// list while it is armed, and feeds the name editor (append/backspace, width and
// character-set checks) while the highlighted row is being edited.
// FUNCTION: CMR2 0x004ff720
void OptionMenu_UpdateLoadGameAndNameEntry(Menu *pMenu)
{
    int value;
    DeviceInfo *pDevice;
    int index;
    int length;

    g_unk0x0082aa40 = SavedGames_GetCount();
    if (g_unk0x0082ab44 != 0) {
        Menu_SetFlags(pMenu, 0, 0, 1, 1);
        pDevice = CInput::GetAvailableDeviceRecord(0);
        if (g_unk0x0082aa40 > 0) {
            if (g_savedGameListSelection == -1) {
                g_savedGameListSelection = 0;
            } else if ((pDevice->pressed & 0x20) != 0) {
                g_unk0x0082ab44 = 0;
                g_unk0x0082ac60 = 1;
            } else if ((pDevice->pressed & 4) != 0 && g_savedGameListSelection > 0) {
                g_savedGameListSelection--;
            } else if ((pDevice->pressed & 8) != 0 && g_savedGameListSelection < g_unk0x0082aa40 - 1) {
                g_savedGameListSelection++;
            }
        } else {
            g_savedGameListSelection = -1;
            g_unk0x0082ab44 = 0;
            g_savedGameListTop = 0;
        }
        if (g_savedGameListSelection < g_savedGameListTop) {
            g_savedGameListTop--;
            if (g_savedGameListTop < 0)
                g_savedGameListTop = 0;
        }
        if (g_savedGameListSelection >= g_savedGameListTop + 13)
            g_savedGameListTop++;
        if (g_unk0x0082aa40 > 13)
            g_unk0x0082ac48 = 13;
        else
            g_unk0x0082ac48 = g_unk0x0082aa40;
        return;
    }

    if (g_savedGameListSelection < g_savedGameListTop) {
        g_savedGameListTop--;
        if (g_savedGameListTop < 0)
            g_savedGameListTop = 0;
    }
    if (g_savedGameListSelection >= g_savedGameListTop + 13)
        g_savedGameListTop++;
    if (g_unk0x0082aa40 > 13)
        g_unk0x0082ac48 = 13;
    else
        g_unk0x0082ac48 = g_unk0x0082aa40;

    index = Menu_FindItem(pMenu, 4);
    if (pMenu->items[index].max == 1) {
        Menu_SetFlags(pMenu, 0, 0, 1, 0);
        Sys_RequestTextInput();   // PORT
        pDevice = CInput::GetAvailableDeviceRecord(0);
        if (pDevice->pressed == 0) {
            g_nameEditorOpened = 1;
        } else if (g_nameEditorOpened == 0) {
            goto editor;
        }
        if ((pDevice->pressed & 4) != 0) {
            index = Menu_FindItem(pMenu, 4);
            pMenu->items[index].max = 0;
            strcpy(g_unk0x0082aa44, CMain::m_logFileBlankLine);
        } else if ((pDevice->pressed & 1) != 0) {
            pMenu->cursor--;
            strcpy(g_unk0x0082aa44, CMain::m_logFileBlankLine);
        } else if ((pDevice->pressed & 2) != 0) {
            pMenu->cursor++;
            strcpy(g_unk0x0082aa44, CMain::m_logFileBlankLine);
        }
    editor:
        g_nameEditorBuffer = g_unk0x0082aa44;
        strcpy(CFrontend::m_stringDest, g_unk0x0082aa44);
        if (Input_PopQueuedCharacter(&value)) {
            if (value != 8) {
                length = strlen(CFrontend::m_stringDest);
                if (length < 0x1e && strchr(g_str0x00526fb8, (char)value) != NULL &&
                    Font_GetTextWidth(0, (BYTE *)g_nameEditorBuffer) <
                        (int)g_pGraphics->resX * 0xdc / 0x280) {
                    CFrontend::m_stringDest[length] = (char)value;
                    CFrontend::m_stringDest[length + 1] = 0;
                    Menu_PlaySoundId(1);
                }
            } else {
                if (CFrontend::m_stringDest[0] != 0) {
                    CFrontend::m_stringDest[strlen(CFrontend::m_stringDest) - 1] = 0;
                    Menu_PlaySoundId(2);
                }
            }
            strcpy(g_nameEditorBuffer, CFrontend::m_stringDest);
        }
    } else {
        Input_ClearCharacterQueue();
        strcpy(g_unk0x0082aa44, CMain::m_logFileBlankLine);
        g_nameEditorOpened = 0;
        Menu_SetFlags(pMenu, 1, 1, 1, 1);
    }
}

// Per-slot number of converted mesh blocks in g_previewOriginalVertices (stride 0x2ac).
// This is meshCount at +0x26a in the same records the converter fills.
// Keeping a separate array here makes session cleanup skip the vertex buffers.
// GLOBAL: CMR2 0x0082d19c
BYTE *g_previewNormalModelBuffers[16];
// GLOBAL: CMR2 0x008311d8
BYTE *g_previewLowWheelModelBuffers[16];
// GLOBAL: CMR2 0x00831158
BYTE *g_previewSixWheelModelBuffers[16];

// GLOBAL: CMR2 0x00831088
GenericFile g_unk0x00831088[16];
// GLOBAL: CMR2 0x00831318
BYTE g_previewSessionRegistered;

// Releases every resource of the rally session: the scene nodes and converted
// mesh buffers of every slot, its textures and scene nodes, and clears the
// session flag.
// FUNCTION: CMR2 0x00505f10
BYTE OptionPreview_ReleaseSessionResources(void)
{
    int i;
    int j;

    if (CGameInfo::GetPreviewMode() == 0) {
        for (i = 0; i < 16; i++) {
            for (j = 0; j < 4; j++) {
                if (g_previewSceneRecords[i].normalWheelMeshes[j] != 0)
                    g_previewSceneRecords[i].wheelNodes[j]->pObject =
                        g_previewSceneRecords[i].normalWheelMeshes[j];
            }
        }
    }
    for (i = 0; i < 16; i++) {
        if (g_previewOriginalVertices[i] != NULL) {
            for (j = 0; j < g_previewMeshRecords[i].meshCount; j++) {
                if (g_previewOriginalVertices[i][j] != NULL) {
                    CFileBuffer::FreeGenericFileBuffer(g_previewOriginalVertices[i][j]);
                    g_previewOriginalVertices[i][j] = NULL;
                }
            }
            CFileBuffer::FreeGenericFileBuffer(g_previewOriginalVertices[i]);
            g_previewOriginalVertices[i] = NULL;
        }
    }
    for (i = 0; i < 16; i++) {
        if (g_previewOriginalNodeTypes[i] != NULL) {
            CFileBuffer::FreeGenericFileBuffer(g_previewOriginalNodeTypes[i]);
            g_previewOriginalNodeTypes[i] = NULL;
        }
    }
    for (i = 0; i < 16; i++) {
        if (g_unk0x00831088[i].buffer != NULL) {
            CFileBuffer::FreeGenericFileBuffer(g_unk0x00831088[i].buffer);
            g_unk0x00831088[i].buffer = NULL;
        }
        g_unk0x00831088[i].buffer = NULL;
        g_unk0x00831088[i].didFileLoad = 0;
        g_unk0x00831088[i].fileSize = 0;
    }
    for (i = 0; i < (BYTE)CGameInfo::GetConfiguredPlayerCount(); i++) {
        if (g_previewSceneRecords[i].rootNode != NULL)
            SceneNode_Destroy(g_previewSceneRecords[i].rootNode);
        if (g_previewSceneRecords[i].lowWheelRoot != 0)
            SceneNode_Destroy(g_previewSceneRecords[i].lowWheelRoot);
        if (g_previewSceneRecords[i].sixWheelRoot != 0)
            SceneNode_Destroy(g_previewSceneRecords[i].sixWheelRoot);
        if (g_previewNormalModelBuffers[i] != NULL) {
            CFileBuffer::FreeGenericFileBuffer(g_previewNormalModelBuffers[i]);
            g_previewNormalModelBuffers[i] = NULL;
        }
        if (g_previewLowWheelModelBuffers[i] != NULL) {
            CFileBuffer::FreeGenericFileBuffer(g_previewLowWheelModelBuffers[i]);
            g_previewLowWheelModelBuffers[i] = NULL;
        }
        if (g_previewSixWheelModelBuffers[i] != NULL) {
            CFileBuffer::FreeGenericFileBuffer(g_previewSixWheelModelBuffers[i]);
            g_previewSixWheelModelBuffers[i] = NULL;
        }
    }
    // PORT: the released scenes are forgotten. A session loads only its
    // players' records, so the others kept the earlier session's wheel nodes
    // and scenes, and leaving the service area again wrote the wheels back
    // into, and destroyed, scenes already released.
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 4; j++) {
            g_previewSceneRecords[i].normalWheelMeshes[j] = 0;
            g_previewSceneRecords[i].wheelNodes[j] = NULL;
        }
        g_previewSceneRecords[i].rootNode = NULL;
        g_previewSceneRecords[i].lowWheelRoot = 0;
        g_previewSceneRecords[i].sixWheelRoot = 0;
    }
    g_previewSessionRegistered = 0;
    return 1;
}

// Per-frame callback of the option menu: ages the two message timers, resets the
// option rows when the highlighted entry changes, animates the row states and
// normalises the country and stage selections of the session.
// FUNCTION: CMR2 0x004ffab0
void OptionMenu_UpdateMessagesAndRecordStates(Menu *pMenu)
{
    int i;
    int value;
    int old;

    if (g_unk0x0082ac4c > 0) {
        g_unk0x0082ac4c--;
        if (g_unk0x0082ac4c == 0)
            OptionMenu_AdvanceSharedModeValue();
    }
    if (g_unk0x0082ac50 > 0) {
        g_unk0x0082ac50--;
        if (g_unk0x0082ac50 == 0)
            OptionMenu_Start(0);
    }
    g_unk0x0082a928 = CMain::GetFrameDelta();
    if (pMenu->cursor != g_unk0x00526f44) {
        OptionMenu_ClearRecordTransition(0);
        OptionMenu_ClearRecordTransition(1);
        OptionMenu_ClearRecordTransition(3);
        OptionMenu_ClearRecordTransition(2);
        OptionMenu_ClearRecordTransition(5);
        OptionMenu_ClearRecordTransition(6);
        OptionMenu_ClearRecordTransition(7);
        if ((pMenu->cursor != 2 || g_unk0x00526f44 != 1) &&
            (pMenu->cursor != 1 || g_unk0x00526f44 != 2))
            OptionMenu_ClearRecordTransition(4);
        g_unk0x0082a930 = g_unk0x0082a928;
        g_optionSlotTypes[g_unk0x00526f44] = 2;
        g_unk0x00526f44 = pMenu->cursor;
        OptionMenu_ClearTimeout();
    }
    for (i = 0; i < 6; i++) {
        switch (g_optionSlotTypes[i]) {
        case 0:
            if (i == pMenu->cursor)
                Menu_SetFlags(pMenu, 0, 0, 0, 0);
            if ((unsigned int)(g_unk0x0082a928 - g_unk0x0082a930) > 1)
                g_optionSlotTypes[i] = 1;
            break;
        case 1:
            if (i == pMenu->cursor && pMenu->cursor != Menu_FindItem(pMenu, 4))
                Menu_SetFlags(pMenu, 1, 1, 1, 1);
            break;
        case 2:
            if (i == pMenu->cursor)
                Menu_SetFlags(pMenu, 0, 0, 0, 0);
            if ((unsigned int)(g_unk0x0082a928 - g_unk0x0082a930) > 1) {
                g_unk0x0082a930 = g_unk0x0082a928;
                g_optionSlotTypes[i] = 3;
                g_optionSlotTypes[pMenu->cursor] = 0;
            }
            break;
        case 3:
            if (i == pMenu->cursor)
                Menu_SetFlags(pMenu, 0, 0, 0, 0);
            break;
        }
    }
    if (pMenu->cursor != Menu_FindItem(pMenu, 2) && pMenu->cursor != Menu_FindItem(pMenu, 1)) {
        for (i = 0; i < (BYTE)CGameInfo::GetConfiguredPlayerCount(); i++)
            OptionPreview_SetWheelNodesVisible(i, 0);
    } else {
        for (i = 0; i < (BYTE)CGameInfo::GetConfiguredPlayerCount(); i++) {
            if (i == CGameInfo::GetActiveOptionSlot())
                OptionPreview_SetWheelNodesVisible(i, 1);
            else
                OptionPreview_SetWheelNodesVisible(i, 0);
        }
    }
    if (pMenu->cursor != Menu_FindItem(pMenu, 5))
        (OptionMenu_GetControlSetupMenu())->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 5)].max = 1;
    OptionMenu_ConfirmHighlightedItem(pMenu);
    value = (OptionMenu_GetControlSetupMenu())->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 1)].max;
    while (OptionMenu_IsSlotEnabled(OptionMenu_GetColumnLabelId(value),
                        CFrontend::GetArchivePrimaryIDEntry(
                            RallyData_GetDriverRecordSelectionValue(CGameInfo::GetActiveOptionSlot()) & 0xff)) == 0) {
        if (g_unk0x00526f48 < value)
            value++;
        else
            value--;
    }
    (OptionMenu_GetControlSetupMenu())->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 1)].max = (BYTE)value;
    if (g_unk0x00526f48 != value) {
        OptionMenu_ClearTimeout();
        OptionMenu_ClearRecordTransition(5);
        OptionMenu_ClearRecordTransition(7);
    }
    g_unk0x00526f48 = value;
    value = (OptionMenu_GetControlSetupMenu())->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 2)].max;
    while (OptionMenu_IsSlotEnabled(OptionMenu_GetColumnWeight(value),
                        CFrontend::GetArchivePrimaryIDEntry(
                            RallyData_GetDriverRecordSelectionValue(CGameInfo::GetActiveOptionSlot()) & 0xff)) == 0) {
        if (g_unk0x00526f4c < value)
            value++;
        else
            value--;
    }
    (OptionMenu_GetControlSetupMenu())->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 2)].max = (BYTE)value;
    if (g_unk0x00526f4c != value) {
        OptionMenu_ClearTimeout();
        OptionMenu_ClearRecordTransition(5);
        OptionMenu_ClearRecordTransition(7);
    }
    g_unk0x00526f4c = value;
    old = (OptionMenu_GetControlSetupMenu())->items[Menu_FindItem(OptionMenu_GetControlSetupMenu(), 0)].max;
    if (g_unk0x00526f50 != old)
        OptionMenu_ClearRecordTransition(7);
    g_unk0x00526f50 = old;
    OptionMenu_UpdateTimeout();
    OptionMenu_FillValueSlider();
    if (pMenu->cursor == Menu_FindItem(pMenu, 4))
        OptionMenu_UpdateLoadGameAndNameEntry(pMenu);
    else
        g_nameEditorOpened = 0;
}

// The frontend and geometry loader use the same scratch buffer in the original.
SceneNode *SceneNode_FindByType(SceneNode *, unsigned int);
// Dependencias de OptionPreview_LoadStageGeometryRecord: las funciones de clase se toman de sus cabeceras.
#include "Game.h"
#include "Graphics.h"
#include "FileBuffer.h"
#include "SceneNode.h"
struct SceneNode *Sector_BuildC3DModelScene(void *data, struct SceneNode *parent, struct GenericFile *archive);
char *Car_GetTextureSetPath(int, int, int);
int  OptionPreview_UsesWheelVariantSix(int, int);
int  OptionPreview_UsesLowWheelVariant(int, int);
void OptionPreview_BuildDeformGeometry(OptionPreviewMeshRecord *, int);
void OptionPreview_ProjectReferencePartQuads(int);
SceneNode *OptionMenu_GetBackgroundRoot(void);


// Compone el nombre de la geometria del stage (base .c3d con los sufijos A1N/L/S y su .bfl), registra y
// carga los recursos, rellena el registro del stage (matrices de las ruedas, mallas y contadores) y libera
// lo temporal. Devuelve 1 si el stage se cargo.
// FUNCTION: CMR2 0x005062d0
int OptionPreview_LoadStageGeometryRecord(int index)
{
    OptionPreviewSceneRecord *pEntry;
    BYTE *hC3D;
    BYTE *hL;
    BYTE *hS;
    SceneNode *stage;
    unsigned char model;
    unsigned char variant;
    GenericFile *pRecord;
    SceneNode *root;
    SceneNode *nodeL;
    SceneNode *nodeS;
    int i;

    stage = OptionMenu_GetBackgroundRoot();
    CGameInfo::GetConfiguredGameMode();
    if (g_previewSessionRegistered == 0) {
        CGame::RegisterCallback((void *)OptionPreview_ReleaseSessionResources, 0);
        g_previewSessionRegistered = 1;
        for (i = 0; i < 16; i++) {
            g_unk0x00831088[i].buffer = NULL;
            g_unk0x00831088[i].didFileLoad = 0;
            g_unk0x00831088[i].fileSize = 0;
        }
    }
    model = (BYTE)(int)RallyData_GetDriverRecordSelectionValue(index);
    variant = (BYTE)CFrontend::GetArchivePrimaryIDEntry(model);
    sprintf(CFrontend::m_stringDest, "%s.c3d", Car_GetTextureSetPath(model, 0, 0));
    if (CGameInfo::GetPreviewMode() == 0) {
        strncpy(CFrontend::m_stringDest + strlen(CFrontend::m_stringDest) - 6, "A1N.c3d", 8);
        hC3D = (BYTE *)CFileBuffer::GetGenericFileBuffer(CFrontend::m_stringDest, 0);
        if (!(OptionPreview_UsesLowWheelVariant(variant, 1) == 0)) {
            strncpy(CFrontend::m_stringDest + strlen(CFrontend::m_stringDest) - 5, "L.c3d", 5);
            hL = (BYTE *)CFileBuffer::GetGenericFileBuffer(CFrontend::m_stringDest, 0);
        } else {
            hL = NULL;
        }
        if (OptionPreview_UsesWheelVariantSix(variant, 6) != 0) {
            strncpy(CFrontend::m_stringDest + strlen(CFrontend::m_stringDest) - 5, "S.c3d", 5);
            hS = (BYTE *)CFileBuffer::GetGenericFileBuffer(CFrontend::m_stringDest, 0);
        } else {
            hS = NULL;
        }
    } else {
        hC3D = (BYTE *)CFileBuffer::GetGenericFileBuffer(CFrontend::m_stringDest, 0);
        hL = NULL;
        hS = NULL;
    }
    g_previewNormalModelBuffers[index] = hC3D;
    g_previewLowWheelModelBuffers[index] = hL;
    g_previewSixWheelModelBuffers[index] = hS;

    sprintf(CFrontend::m_stringDest, "%s.bfl", Car_GetTextureSetPath(model, 0, 0));
    if (CGameInfo::GetPreviewMode() == 0)
        strncpy(CFrontend::m_stringDest + strlen(CFrontend::m_stringDest) - 6, "A1.bfl", 6);

    pEntry = &g_previewSceneRecords[index];
    // the .bfl and the geometry relocations go to the 12-byte loader record of
    // this entry (0x831088, cleared on the first call above), not to pEntry
    pRecord = &g_unk0x00831088[index];
    CGenericFileLoader::LoadIntoFileRecord(pRecord, CFrontend::m_stringDest);
    if (hC3D == NULL)
        return 0;

    nodeL = NULL;
    nodeS = NULL;
    root = Sector_BuildC3DModelScene(hC3D, stage, pRecord);
    if (CGameInfo::GetPreviewMode() == 0) {
        Mesh **p = &pEntry->normalWheelMeshes[0];
        for (i = 1; i <= 4; i++)
            *p++ = (Mesh *)SceneNode_FindByType(root, (unsigned int)i)->pObject;
        if (hL != NULL) {
            Mesh **q = &pEntry->lowWheelMeshes[0];
            nodeL = Sector_BuildC3DModelScene(hL, stage, pRecord);
            for (i = 1; i <= 4; i++)
                *q++ = (Mesh *)SceneNode_FindByType(nodeL, (unsigned int)i)->pObject;
            SceneNode_SetViewMaskTree(nodeL, 0);
        }
        if (hS != NULL) {
            Mesh **q = &pEntry->sixWheelMeshes[0];
            nodeS = Sector_BuildC3DModelScene(hS, stage, pRecord);
            for (i = 1; i <= 4; i++)
                *q++ = (Mesh *)SceneNode_FindByType(nodeS, (unsigned int)i)->pObject;
            SceneNode_SetViewMaskTree(nodeS, 0);
        }
    }
    pEntry->wheelNodes[0] = SceneNode_FindByType(root, (unsigned int)1);
    pEntry->wheelNodes[1] = SceneNode_FindByType(root, (unsigned int)2);
    pEntry->wheelNodes[2] = SceneNode_FindByType(root, (unsigned int)3);
    pEntry->wheelNodes[3] = SceneNode_FindByType(root, (unsigned int)4);
    pEntry->modelClass = variant;
    pEntry->rootNode = root;
    pEntry->bodyNode = SceneNode_FindByType(root, (unsigned int)5);
    pEntry->lowWheelRoot = nodeL;
    pEntry->sixWheelRoot = nodeS;
    OptionPreview_RebuildMeshRecord(index);
    OptionPreview_BuildDeformGeometry(&g_previewMeshRecords[index], index);
    {
        SceneNode *h = SceneNode_FindByType(root, (unsigned int)0xe);
        if (h != NULL) {
            Texture *tex = CGraphics::m_pTextureManager->textureBuffer[((Mesh *)h->pObject)->pTriangles->textures[0]];
            CGraphics::RemapTextureAlpha(tex, 0xbf, 0, 0x40, 0, 0x80, 0, index);
            CGraphics::RemapTextureAlpha(tex, 0xe0, 0, 0xe0, 0, 0xe0, 0, index);
        }
    }
    g_previewPreviousAnimationTime = 0;
    OptionPreview_ApplyStoredDamage(index);
    g_previewWheelVariant[index] = -1;
    OptionPreview_ProjectReferencePartQuads(variant);
    return 1;
}

// ---------------------------------------------------------------------------
// Identifier tables of the profile screens (defined in FrontendMenus.cpp).
// ---------------------------------------------------------------------------
#include "BestTimeRecords.h"

extern BYTE g_eventRecordTexts[0x4c];
extern BYTE g_profileCarClassRecordTexts[0x98];
extern BYTE g_stageRecordTexts[0x898];
extern BYTE g_carClassRecordTexts[0x258];
extern BYTE g_championshipRecordTexts[0x1cc];

CategoryBestResults *RallyData_GetDriverBestResults(int index);
void FrontendProfile_ScrambleIdentifier(unsigned int param_1, unsigned int *pNumber, char *pByte, char *pOut);

// Rewrites every best-time record of the selected rally as a scrambled
// identifier string (see FrontendProfile_ScrambleIdentifier/FrontendText_FormatCodeGroups) into the five tables the
// profile screens display.
// FUNCTION: CMR2 0x004f8b30
void FrontendRecords_BuildScrambledBestTimeTables(void)
{
    BestTimeCursor iVar2;
    BestTimeCursor iVar3;
    unsigned int *iVar4;
    unsigned int uVar5;
    unsigned int uVar8;
    unsigned int *puVar10;
    BYTE *puVar9;
    unsigned int *puVar7;
    union CodeTag {
        unsigned word;
        struct {
            unsigned char kind : 3;
            unsigned char driver : 5;
        } bits;
    } local_4;
    // The packed word is carried between all five passes, including its XOR.
    // Only the rally view is assigned through fields; the other layouts keep
    // their original masks so MSVC6 preserves the original store grouping.
    union CodeWord {
        unsigned word;
        struct {
            unsigned minutes : 6;
            unsigned country : 3;
            unsigned car : 6;
            unsigned automatic : 1;
            unsigned seconds : 6;
            unsigned hundredths : 7;
            unsigned difficulty : 2;
            unsigned rest : 1;
        } rally;
    } local_8;
    BestTimeCursor local_c;
    BestTimeCursor local_10;
    BYTE *local_18;
    int local_1c;

    local_8.word = 0;
    local_4.word = 0;
    local_10.index = 0;
    do {
        local_c.index = (local_10.index % 2) + 10;
        for (uVar8 = 0; (int)uVar8 < local_c.index; uVar8++) {
            {
                puVar7 = &RallyData_GetDriverBestResults(0)->rallyStageTimes[local_10.index][uVar8].time;
                puVar10 = &RallyData_GetDriverBestResults(0)->rallyStageTimes[local_10.index][uVar8].header;
                if (((*puVar10 & 0x80) == 0) || (*puVar7 / 6000 >= 16)) {
                    g_stageRecordTexts[(local_10.index * 11 + uVar8) * 0x19] = 0;
                } else {
                    local_8.word = (local_8.word & 0xfffff00f) |
                              (((uVar8 & 0xf) << 4 | (unsigned int)local_10.index & 0xf) << 4);
                    local_8.word = (local_8.word & 0xfffc0fff) | (*puVar10 & 0x3f) << 0xc;
                    local_8.word = (local_8.word & 0x7fffffff) | (~*puVar10 & 0xffffffc0) << 0x19;
                    local_8.word = local_8.word ^ ((*puVar7 / 6000 ^ local_8.word) & 0xf);
                    local_8.word = (local_8.word & 0xff03ffff) | ((*puVar7 / 100) % 0x3c & 0x3f) << 0x12;
                    local_8.word = (local_8.word & 0x80ffffff) | (*puVar7 % 100 & 0x7f) << 0x18;
                    local_4.bits.kind = 0;
                    local_4.bits.driver = *puVar10 >> 8;
                    puVar7 = &local_8.word;
                    iVar2.index = 4;
                    do {
                        *(BYTE *)puVar7 ^= (BYTE)(local_4.bits.driver << 1);
                        puVar7 = (unsigned int *)((BYTE *)puVar7 + 1);
                        iVar2.index--;
                    } while (iVar2.index != 0);
                    FrontendProfile_ScrambleIdentifier(0, &local_8.word, (char *)&local_4,
                                 (char *)&g_stageRecordTexts[(local_10.index * 11 + uVar8) * 0x19]);
                    FrontendText_FormatCodeGroups((char *)&g_stageRecordTexts[(local_10.index * 11 + uVar8) * 0x19]);
                }
            }
        }
        local_10.index = local_10.index + 1;
    } while (local_10.index < 8);

    local_10.bytes = g_carClassRecordTexts;
    local_1c = 0;
    do {
        uVar5 = 0;
        local_18 = local_10.bytes;
        do {
            iVar3.rally = (BestTimeRallyTail *)&RallyData_GetDriverBestResults(0)->rallyResults[uVar5][local_1c].result;
            iVar4 = &RallyData_GetDriverBestResults(0)->rallyResults[uVar5][local_1c].header;
            puVar7 = iVar4;
            if (((*(BYTE *)iVar4 & 0x80) == 0) || (iVar3.rally->centiseconds / 6000 >= 64)) {
                *local_18 = 0;
            } else {
                local_8.rally.country = uVar5;
                local_8.rally.car = *puVar7 & 0x3f;
                local_8.rally.automatic = (~*puVar7 & 0x40) >> 6;
                local_8.rally.minutes = iVar3.rally->centiseconds / 6000;
                local_8.rally.seconds = (iVar3.rally->centiseconds / 100) % 60;
                local_8.rally.hundredths = iVar3.rally->centiseconds % 100;
                local_8.rally.difficulty = local_1c;

                puVar7 = &local_8.word;
                local_4.bits.kind = 1;
                local_4.bits.driver = *iVar4 >> 8;
                iVar3.index = 4;
                do {
                    *(BYTE *)puVar7 ^= (BYTE)(local_4.bits.driver << 1);
                    puVar7 = (unsigned int *)((BYTE *)puVar7 + 1);
                    iVar3.index--;
                } while (iVar3.index != 0);
                FrontendProfile_ScrambleIdentifier(0, &local_8.word, (char *)&local_4, (char *)local_18);
                FrontendText_FormatCodeGroups((char *)local_18);
            }
            uVar5++;
            local_18 += 0x4b;
        } while ((int)uVar5 < 8);
        local_1c++;
        local_10.bytes += 0x19;
    } while (local_10.bytes < g_carClassRecordTexts + 0x4b);

    uVar5 = 0;
    puVar9 = g_eventRecordTexts;
    do {
        puVar7 = &RallyData_GetDriverBestResults(0)->playerResults[uVar5].result.result;
        puVar10 = &RallyData_GetDriverBestResults(0)->playerResults[uVar5].result.header;
        if ((*(BYTE *)puVar10 & 0x80) != 0) {
            local_8.word = (local_8.word & 0xfff83fff) | (*puVar7 & 0xf) << 0xe;
            local_8.word = (local_8.word & 0xffe7c07f) |
                      ((uVar5 & 3) << 0x12 | *puVar7 & 0x1fc0) << 1;
            local_8.word = local_8.word ^ ((*puVar10 ^ local_8.word) & 0x3f);
            local_8.word = local_8.word ^ ((~*puVar10 ^ local_8.word) & 0x40);
            local_4.bits.kind = 2;
            local_4.bits.driver = *puVar10 >> 8;

            puVar10 = &local_8.word;
            iVar2.index = 4;
            do {
                *(BYTE *)puVar10 ^= (BYTE)(local_4.bits.driver << 1);
                puVar10 = (unsigned int *)((BYTE *)puVar10 + 1);
                iVar2.index--;
            } while (iVar2.index != 0);
            FrontendProfile_ScrambleIdentifier(0, &local_8.word, (char *)&local_4, (char *)puVar9);
            FrontendText_FormatCodeGroups((char *)puVar9);
        } else {
            *puVar9 = 0;
        }
        puVar9 += 0x19;
        uVar5++;
    } while (puVar9 < g_eventRecordTexts + 0x4b);

    uVar5 = 0;
    local_18 = g_profileCarClassRecordTexts;
    do {
        uVar8 = 0;
        local_c.bytes = local_18;
        do {
            // The original keeps puVar7 from the championship pass here and
            // discards the first provider result. Preserve both calls.
            RallyData_GetDriverBestResults(0);
            puVar10 = &RallyData_GetDriverBestResults(0)->arcadeResults[uVar8][uVar5].header;
            if ((*puVar10 & 0x80) != 0) {
                local_8.word = (local_8.word & 0xffff8fff) | (*puVar7 & 7) << 0xc;
                local_8.word = (local_8.word & 0xfffe707f) |
                          ((uVar5 & 3) << 0xe | *puVar7 & 0x7c0) << 1;
                local_8.word = local_8.word ^ ((*puVar10 ^ local_8.word) & 0x3f);
                local_8.word = (local_8.word & 0xfff9ffbf) | (~*puVar10 & 0x40) | (uVar8 & 3) << 0x11;
                iVar2.index = 4;
                local_4.bits.kind = 3;
                local_4.bits.driver = *puVar10 >> 8;

                puVar10 = &local_8.word;
                do {
                    *(BYTE *)puVar10 ^= (BYTE)(local_4.bits.driver << 1);
                    puVar10 = (unsigned int *)((BYTE *)puVar10 + 1);
                    iVar2.index--;
                } while (iVar2.index != 0);
                FrontendProfile_ScrambleIdentifier(0, &local_8.word, (char *)&local_4, (char *)local_c.bytes);
                FrontendText_FormatCodeGroups((char *)local_c.bytes);
            } else {
                *local_c.bytes = 0;
            }
            uVar8++;
            local_c.bytes += 0x4b;
        } while ((int)uVar8 < 2);
        uVar5++;
        local_18 += 0x19;
    } while ((int)uVar5 < 3);

    uVar5 = 0;
    local_10.bytes = g_championshipRecordTexts;
    do {
        iVar2.index = ((int)uVar5 / 3) * 3 + (int)uVar5 % 3;
        puVar7 = &RallyData_GetDriverBestResults(0)->arcadeStageTimes[0][iVar2.index].time;
        iVar3.word = &RallyData_GetDriverBestResults(0)->arcadeStageTimes[0][iVar2.index].header;
        uVar8 = *iVar3.word;
        puVar10 = iVar3.word;
        if (((uVar8 & 0x80) == 0) || (*puVar7 / 6000 >= 64)) {
            *local_10.bytes = 0;
        } else {
            local_8.word = local_8.word ^ ((uVar8 ^ local_8.word) & 0x3f);
            local_8.word = (local_8.word & 0xfffffc3f) | (~*puVar10 & 0x40) | (uVar5 & 7) << 7;
            local_8.word = (local_8.word & 0xe07fffff) | (*puVar7 / 6000 & 0x3f) << 0x17;
            local_8.word = (local_8.word & 0xffff03ff) | ((*puVar7 / 100) % 0x3c & 0x3f) << 10;
            local_8.word = (local_8.word & 0xff80ffff) | (*puVar7 % 100 & 0x7f) << 0x10;
            iVar2.index = 4;

            puVar7 = &local_8.word;
            local_4.bits.kind = 4;
            local_4.bits.driver = *puVar10 >> 8;
            do {
                *(BYTE *)puVar7 ^= (BYTE)(local_4.bits.driver << 1);
                puVar7 = (unsigned int *)((BYTE *)puVar7 + 1);
                iVar2.index--;
            } while (iVar2.index != 0);
            FrontendProfile_ScrambleIdentifier(0, &local_8.word, (char *)&local_4, (char *)local_10.bytes);
            FrontendText_FormatCodeGroups((char *)local_10.bytes);
        }
        local_10.bytes += 0x19;
        uVar5++;
    } while (local_10.bytes < g_championshipRecordTexts + 0xc8);
}
