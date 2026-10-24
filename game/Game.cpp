#include "port/sys.h"
#include "RaceCallbackStorage.h"
#include "FloatToInt.h"
#include "StageBlock.h"
#include "Game.h"
#include "Menu.h"
#include "main.h"
#include "RegKey.h"
#include "GameInfo.h"
#include "InstallInfo.h"
#include "NetworkLeaderboards.h"
#include "Graphics.h"
#include "Input.h"
#include "FileBuffer.h"
#include "Mesh.h"
#include "Frontend.h"
#include "Texture.h"
#include "Sprite.h"
#include "Sound.h"
#include "Font.h"
#include "Sprite.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <float.h>
#include "Glow.h"
#include "LayoutChecks.h"
#include "CameraRecord.h"
#include "FarLayer.h"

BOOL CGame::m_shouldExit = FALSE;
BOOL CGame::m_isActive = FALSE;
int CGame::m_unk0x0059ce14;
int CGame::m_unk0x0059ce18;
// GLOBAL: CMR2 0x0059ce1c
int CGame::m_unk0x0059ce1c;
int CGame::m_unk0x0059ce20;
int CGame::m_deferredDrawCount;
int CGame::m_transparentDrawCount;
void *CGame::m_deferredDrawObjects[4096];
void *CGame::m_transparentDrawNodes[4096];
int CGame::m_unk0x005207f8 = 3;
int CGame::m_unk0x00663dc4;
DPlayConnection CGame::m_connections[10];
BYTE CGame::m_maxConnections;

int CGame::m_unk0x00523c58 = -1;
int CGame::m_unk0x00523c5c = -1;
// GLOBAL: CMR2 0x00817da0
CallbackStateMachine CGame::m_frontendCallbackMachine;
int CGame::m_unk0x0052ea4c;
BYTE CGame::m_unk0x0052ea51;
// GLOBAL: CMR2 0x00817eb0
bool CGame::m_frontendCallbackInitialized = false;
// GLOBAL: CMR2 0x00817d98
CallbackStateRecord CGame::m_frontendCallbackRecord;
// GLOBAL: CMR2 0x00523c18
unsigned int CGame::m_frontendStateRules[16] = {
    0x0101ff00, 0x0600ff00, 0x0706ff00, 0x0200ff01, 0x0300ff02, 0x0400ff03,
    0x0500ff04, 0x0600ff05, 0x0700ff06, 0x0902ff06, 0x0707ff06, 0x0807ff06,
    0x0900ff08, 0x0900ff07, 0x0000ff09, 0xffffffff,
};
// GLOBAL: CMR2 0x00593cac
BYTE CGame::m_skipRenderCallbacks;
// GLOBAL: CMR2 0x00593ba8
CallbackIndex CGame::m_callbackIndex;
// GLOBAL: CMR2 0x00593ba4
CallbackStateRecord *CGame::m_currentCallbackRecord;

BYTE CGame::m_unk0x00523d68 = 1;
BYTE CGame::m_unk0x008180f9;
BYTE CGame::m_unk0x008180fc;
BYTE CGame::m_unk0x00516120 = 1;
BYTE CGame::m_unk0x00531768;
BYTE CGame::m_unk0x0052ea58;
BYTE CGame::m_unk0x0052ea59;

// GLOBAL: CMR2 0x00593ba0;
int CGame::m_unk0x00593ba0;

// GLOBAL: CMR2 0x005939a0
void *CGame::m_callbacks[64];

// GLOBAL: CMR2 0x005a1818
BYTE CGame::m_sessionPlayerCount;
BYTE CGame::m_unk0x005a1819;
// GLOBAL: CMR2 0x005a1820
SessionPlayerRecord CGame::m_sessionPlayers[7];
int CGame::m_unk0x005a1e34;
bool CGame::m_unk0x005a1fc0;
void *CGame::m_pDirectPlay4A = NULL;
// GLOBAL: CMR2 0x005a1ea0
NetPlayerID CGame::m_localPlayerId = NULL;

void *CGame::m_pDirectPlayLobby3A;

BOOL CGame::m_unk0x005a1fbc;
void *CGame::m_unk0x005a1fb8;



// Boot states of the grouped callback machine (functions next to
// CGame::InitializeGame; the render halves are not written yet).
void Game_DrawBootTextureScreen(CallbackStateMachine *p1, BYTE p2);
void Game_WaitForButtonOrSplashTimeout(CallbackStateMachine *p1, BYTE p2);
void Game_WaitForSplashTimeout(CallbackStateMachine *p1, BYTE p2);
void Game_RunFrontendBootFrame(CallbackStateMachine *p1, BYTE p2);
void Game_DrawBootCameraAndFPS(CallbackStateMachine *p1, BYTE p2);
void Game_DrawCountryLoadingScreen(CallbackStateMachine *p1, BYTE p2);
void Game_PlayCountryIntroAfterFrontendDelay(CallbackStateMachine *p1, BYTE p2);
void Game_PlayCodemastersBootVideo(CallbackStateMachine *p1, BYTE state);
void Game_PlayIntroBootVideo(CallbackStateMachine *p1, BYTE state);
void Game_UpdateBootInputDelay(CallbackStateMachine *p1, BYTE state);
void OptionMenu_UpdateHiddenPreviewState(CallbackStateMachine *p1, BYTE state);
void OptionMenu_DrawBackgroundState(CallbackStateMachine *p1, BYTE state);
void OptionMenu_LeaveAndCommitState(CallbackStateMachine *p1, BYTE state);
void OptionMenu_LeaveWithoutCommitState(CallbackStateMachine *p1, BYTE state);
void Game_FinishRaceAndAdvanceBootState(CallbackStateMachine *p1, BYTE p2);
void Game_DrawLogoAndLoadingSprites(CallbackStateMachine *p1, BYTE p2);
void OptionMenu_StartDelayedFade(CallbackStateMachine *param1, int unused);
void Game_DrawMovieFrameAndCredits(CallbackStateMachine *p1, BYTE p2);

StateCallbackPair CGame::m_initializeGameGroupedFuncTable[10] = {
    {InitializeGame,
     NoOpSecondaryStateCallback},
    {Game_WaitForSplashTimeout, Game_DrawLogoAndLoadingSprites},
    {Game_WaitForSplashTimeout, Game_DrawMovieFrameAndCredits},
    {Game_PlayCodemastersBootVideo, NoOpSecondaryStateCallback},
    {Game_PlayIntroBootVideo, NoOpSecondaryStateCallback},
    {Game_WaitForButtonOrSplashTimeout, Game_DrawBootTextureScreen},
    {Game_RunFrontendBootFrame, Game_DrawBootCameraAndFPS},
    {Game_PlayCountryIntroAfterFrontendDelay, Game_DrawCountryLoadingScreen},
    {Game_UpdateBootInputDelay, NoOpSecondaryStateCallback},
    {Game_FinishRaceAndAdvanceBootState, NoOpSecondaryStateCallback},
};

// FUNCTION: CMR2 0x004a15a0
BOOL Network_GetSessionStateFlag(void)
{
    return CGameInfo::m_unk0x005a0060;
}

// FUNCTION: CMR2 0x004a9a40
void CGame::SetShouldExit(void)
{
    m_shouldExit = TRUE;
}

// FUNCTION: CMR2 0x004b7a40
void CGame::UpdateActiveSoundSlots(void)
{
    SoundSlot **ppSlot;

    if (CSound::m_unk0x006e0eec != 0) {
        ppSlot = CSound::m_soundSlots;
        do {
            if (*ppSlot != NULL)
                CSound::UpdateFinishedSoundSlot(*ppSlot);
            ppSlot++;
        } while ((INT_PTR)ppSlot < (INT_PTR)(CSound::m_soundSlots + 32));
    }
}

// FUNCTION: CMR2 0x004d0780
BOOL CGame::DispatchFrontendResourceState(void)
{
    BOOL result;

    m_unk0x00523c5c = GetFrontendResourceMode();
    if (m_unk0x00523c5c != m_unk0x00523c58)
        m_unk0x00523c58 = m_unk0x00523c5c;

    switch (m_unk0x00523c5c)
    {
    case 3:
        return UpdateInRaceCallbackMachine();

    // PORT: the menus laid out for the screen's size (see MenuScreen_Begin).
    case 2:
        MenuScreen_Begin();
        result = UpdateSecondaryCallbackMachine();
        MenuScreen_End();
        return result;

    case 0:
        MenuScreen_Begin();
        result = UpdateFrontendCallbackMachine();
        MenuScreen_End();
        return result;

    default:
        return FALSE;
    }
}

// Frontend callback group of the game state machine: built on the first call,
// then runs one update/render/timer step per frame.
// FUNCTION: CMR2 0x004d07c0
BOOL CGame::UpdateFrontendCallbackMachine(void)
{
    if (m_frontendCallbackInitialized)
    {
        RunStateUpdateCallbacks(&m_frontendCallbackMachine);
        RunStateRenderCallbacks(&m_frontendCallbackMachine);
        AdvanceCallbackStateTimers(&m_frontendCallbackMachine);
        return FALSE;
    }

    InitializeCallbackStateRecord(&m_frontendCallbackRecord, 0, 0xFF);
    InitializeCallbackStateMachine(&m_frontendCallbackMachine, 1, &m_frontendCallbackRecord, m_initializeGameGroupedFuncTable, m_frontendStateRules);
    m_frontendCallbackInitialized = true;
    return FALSE;
}

// Splash screen scene, created while the frontend resources load.
// GLOBAL: CMR2 0x00817fc4
SceneNode *g_splashCamera;
// GLOBAL: CMR2 0x00817fc8
SceneNode *g_splashSceneRoot;

// Destroys the splash screen scene (registered as a callback by Game_CreateSplashScene).
// FUNCTION: CMR2 0x004d0820
BYTE Game_DestroySplashScene(void)
{
    if (g_splashSceneRoot != NULL)
        SceneNode_Destroy(g_splashSceneRoot);

    return TRUE;
}

// Creates the empty scene the frontend draws while it loads its textures.
// FUNCTION: CMR2 0x004d0840
void Game_CreateSplashScene(void)
{
    FixVector translation;
    FixAngles angles;
    SceneNode *pNode;

    translation.x = 0;
    translation.y = 0;
    translation.z = 0xffec0000;
    angles.x = 0;
    angles.y = 0;
    angles.z = 0;
    angles.pad = 0;

    g_splashSceneRoot = SceneNode_CreateRoot();
    g_splashCamera = SceneType2_Create(&translation, &angles, NULL, g_splashSceneRoot);

    for (pNode = g_splashSceneRoot; pNode != NULL; pNode = pNode->pParent)
        pNode->dirty = 1;

    CGraphics::SetProjection(0x25645, 0x4326e, 0xfa0000, 0x10000);
    CGame::RegisterCallback(Game_DestroySplashScene, NULL);
}

// Set once the boot sequence has finished loading; the render states clear it.
// GLOBAL: CMR2 0x00817fcc
BYTE g_bootLoadingDone;
// Format of the debug FPS counter ("FPS: %.2f").
// GLOBAL: CMR2 0x00516e14
char g_strFpsFormat0x00516e14[10] = "FPS: %.2f";
// Colour table of the debug FPS text; only the entry 0 is read.
// GLOBAL: CMR2 0x00523c64
int g_fpsTextColours[5] = { -1, 120, 120, 120, 120 };

void FrontendMenu_DrawActiveMenu(void);
float Graphics_GetFrameRate(void);
int Game_PrepareScene(SceneNode *pRoot, SceneNode *pCamera, void *pRect, int param);
int Game_DrawSceneViewport(struct SceneNode *pRoot, struct SceneNode *pCamera, void *pRect, int bit, BYTE flag);
void Graphics_PresentFrameAndResetCounters(void);

// Boot render state: places the splash-scene camera, clears the target and
// prints the FPS counter while the graphics debug flag (bit 2) is set.
// FUNCTION: CMR2 0x004d0a80
void Game_DrawBootCameraAndFPS(CallbackStateMachine *p1, BYTE p2)
{
    FixVector translation;
    FixAngles angles;
    short rect[4];

    translation.x = 0;
    translation.y = 0;
    translation.z = 0xffec0000;
    angles.x = 0;
    angles.y = 0;
    angles.z = 0;
    angles.pad = 0;
    rect[0] = 0;
    rect[1] = 0;
    rect[2] = (short)g_pGraphics->resX;
    rect[3] = (short)g_pGraphics->resY;

    SceneNode_SetPosition(g_splashCamera, &translation);
    SceneNode_SetRotation(g_splashCamera, &angles);
    CGraphics::SetProjection(0x25645, 0x4326e, 0xfa0000, 0x10000);
    CGraphics::ClearTarget();
    CGraphics::ClearZBuffer();
    FrontendMenu_DrawActiveMenu();
    Game_PrepareScene(g_splashSceneRoot, g_splashCamera, rect, 0);
    if ((g_pGraphics->field913_0x3bc & 4) != 0) {
        sprintf(CFrontend::m_stringDest, g_strFpsFormat0x00516e14, Graphics_GetFrameRate());
        Font_DrawText(0, CFrontend::m_stringDest, 0, 0, g_fpsTextColours, 9);
    }
    Game_DrawSceneViewport(g_splashSceneRoot, g_splashCamera, rect, 0, 1);
    if (g_bootLoadingDone == 0)
        Graphics_PresentFrameAndResetCounters();
}

// Shared with the other boot renders of Game.cpp (0x4d0a80, 0x4d1370).

// Render half of boot state 1: draws the CMR2 logo and a row of up to four
// loading sprites, prepares the splash scene and shows the FPS counter when the
// debug flag of the graphics device is on.
// FUNCTION: CMR2 0x004d1080
void Game_DrawLogoAndLoadingSprites(CallbackStateMachine *p1, BYTE p2)
{
    BYTE colour[4];
    SpriteRect dst;
    SpriteRect src;
    SpriteRect screen;
    int centre[3];
    Texture *pTexture;
    int count;
    int i;

    // PORT: its pictures hold text, sharp only in the screen's own pixels
    // (see MenuScreen_Suspend).
    MenuScreen_Suspend();
    screen.x = 0;
    screen.y = 0;
    screen.w = g_pGraphics->resX;
    screen.h = g_pGraphics->resY;
    colour[0] = 0xff;
    colour[1] = 0xff;
    colour[2] = 0xff;
    colour[3] = 0xff;
    centre[0] = 0;
    centre[1] = 0;
    centre[2] = 0;

    CGraphics::SetProjection(0x25645, 0x4326e, 0xfa0000, 0x10000);
    CGraphics::ClearTarget();
    CGraphics::ClearZBuffer();

    pTexture = CFrontend::m_unk0x00817fd0;
    if (pTexture != NULL) {
        // the texture size is reused for the source, the destination and the centre
        src.x = pTexture->field_0x11c;
        src.y = pTexture->field_0x11e;
        short w = pTexture->width;
        src.w = w;
        short h = pTexture->height;
        src.h = h;
        dst.x = (int)(g_pGraphics->resX * 43) / 640;
        dst.y = (int)(g_pGraphics->resY * 202) / 480;
        dst.w = w;
        dst.h = h;
        centre[0] = w / 2;
        centre[1] = h / 2;
        Sprite_Queue(&src, &dst, pTexture, 1, 0, centre, NULL, colour, 8);
    }

    if (CGameInfo::GetScreenWidth() >= 0x400 &&
        CFrontend::IsTextureWidthSupported(0x400) &&
        CFrontend::IsTextureHeightSupported(0x400))
        count = 4;
    else
        count = 3;

    dst.x = (int)(g_pGraphics->resX * 43) / 640;
    dst.y = (int)(g_pGraphics->resY * 260) / 480;

    for (i = 0; i < count; i++) {
        pTexture = CFrontend::m_unk0x00817fd4[i];
        if (pTexture != NULL) {
            src.x = pTexture->field_0x11c;
            src.y = pTexture->field_0x11e;
            short w = pTexture->width;
            src.w = w;
            short h = pTexture->height;
            src.h = h;
            dst.w = w;
            dst.h = h;
            centre[0] = w / 2;
            centre[1] = h / 2;
            Sprite_Queue(&src, &dst, pTexture, 1, 0, centre, NULL, colour, 8);
            dst.x += dst.w;
        }
    }

    Game_PrepareScene(g_splashSceneRoot, g_splashCamera, &screen, 0);
    if (g_pGraphics->field913_0x3bc & 4) {
        sprintf(CFrontend::m_stringDest, g_strFpsFormat0x00516e14, Graphics_GetFrameRate());
        Font_DrawText(0, CFrontend::m_stringDest, 0, 0, (int *)colour, 9);
    }
    Game_DrawSceneViewport(g_splashSceneRoot, g_splashCamera, &screen, 0, 1);
    if (g_bootLoadingDone == 0)
        Graphics_PresentFrameAndResetCounters();
    MenuScreen_Resume();
}

int Game_DrawFadingBootLabel(int, int, char *, char);
unsigned char RallyDataCountryIndex(void);

// Boot render state shown while the country data loads: draws the country name
// with the current championship position, centred on the screen.
// FUNCTION: CMR2 0x004d0ba0
void Game_DrawCountryLoadingScreen(CallbackStateMachine *p1, BYTE p2)
{
    FixVector translation;
    FixAngles angles;
    short rect[4];
    int x;
    int y;

    translation.x = 0;
    translation.y = 0;
    translation.z = 0xffec0000;
    angles.x = 0;
    angles.y = 0;
    angles.z = 0;
    angles.pad = 0;

    if (CFrontend::GetFrontendIntroFlag() == 1) {
        rect[0] = 0;
        rect[1] = 0;
        rect[2] = (short)g_pGraphics->resX;
        rect[3] = (short)g_pGraphics->resY;
        SceneNode_SetPosition(g_splashCamera, &translation);
        SceneNode_SetRotation(g_splashCamera, &angles);
        CGraphics::SetProjection(0x25645, 0x4326e, 0xfa0000, 0x10000);
        CGraphics::SetClearColour(1, 0, 0, 0);
        CGraphics::ClearTarget();
        CGraphics::ClearZBuffer();
        x = (int)(g_pGraphics->resX * 0x1e) / 0x280;
        y = Game_DrawFadingBootLabel(x, (int)g_pGraphics->resY / 2, CFrontend::GetTextString(0x1bd), 0);
        // the original copies the country text with sprintf("%s").
        sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue,
                CFrontend::GetTextString((RallyDataCountryIndex() & 0xff) + 0x27));
        CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
        Game_DrawFadingBootLabel(y, (int)g_pGraphics->resY / 2, CFrontend::m_stringDest, 1);
        Game_PrepareScene(g_splashSceneRoot, g_splashCamera, rect, 0);
        Game_DrawSceneViewport(g_splashSceneRoot, g_splashCamera, rect, 0, 1);
        if (g_bootLoadingDone == 0)
            Graphics_PresentFrameAndResetCounters();
    }
}

// Sets the FPU control word to 53-bit precision (the CRT's default for the
// x87 unit before the game changes it). This is CRT startup code, which the
// original built with /Os: the size optimisation is what turns the cdecl
// cleanup into `pop ecx / pop ecx`.
// FUNCTION: CMR2 0x00405796
void Game_SetDoublePrecisionFPU(void)
{
    // PORT: x87 precision control. The port computes with SSE (floats in
    // single, doubles in double precision), so there is nothing to set.
}

// FUNCTION: CMR2 0x004057a8
int Game_ReturnZeroInitializationResult(void)
{
    return 0;
}

// FUNCTION: CMR2 0x004057ab
void Game_NoOpInitializationCallback(void)
{
}

// FUNCTION: CMR2 0x004057d0
int CGame::GetFrontendResourceMode(void)
{
    return m_unk0x0052ea4c;
}

// FUNCTION: CMR2 0x004057c0
void CGame::SetStartupFlag(void)
{
    m_unk0x0052ea51 = 1;
}

// FUNCTION: CMR2 0x004057e0
void CGame::SetFrontendResourceMode(int param1)
{
    m_unk0x0052ea4c = param1;
}

// --- Boot pieces called by CGame::InitializeGame that are not decompiled
// yet. They are empty STUBs so the calls are in place; they do nothing yet.
BYTE Game_GetOptionStateByte(void);
void Game_SetOptionStateByte(BYTE param1);
void RallyData_ClearDriverGroupRecords(void);
void RallyData_ClearDriverSkillFlags(void);
void GameInfo_ResetSessionTimestamp(void);
DWORD GameInfo_GetSessionTimestamp(void);
void Input_TranslatePedalsToMenuKeys(void);
void Input_MergeAssignedJoystickButtons(int slot, DeviceInfo *pOut);
extern int g_frontendTimestamp;
extern unsigned int g_frontendElapsedMs;

void RallyData_ResetSelection(void);
BOOL Network_GetSessionStateFlag(void);
void Session_SetOpen(char open);
void Game_SetConfigurationStateByte(BYTE param1);
bool SavedGames_ReleaseRecords(void);

// Globals the return-to-frontend path of InitializeGame resets.
// GLOBAL: CMR2 0x00819744
int g_unk0x00819744;

// GLOBAL: CMR2 0x00818ce4
BYTE g_unk0x00818ce4;

// Signatures follow the original's `ret N` (stdcall: N/4 arguments).
void FrontendMenu_BuildPagesAndSelectInitial(BYTE param1, BYTE param2);
void SavedGames_LoadRecords(void);
BYTE FrontendAudio_LoadSounds(void);
BYTE FrontendText_LoadFonts(void);
BYTE FrontendText_LoadRegionLanguages(void);
void FrontendCredits_LoadText(char registerRelease);
void RallyData_ResetSavedPlayerRecords(void);
void FrontendScroller_ResetAll(void);
void FrontendNetwork_SendPlayerDescription(void);
void GameInfo_RestoreFrontendOptionSettings(void);
void Game_SaveConfigurationPreservingFullscreen(void);
void FrontendAnimation_LoadDotTextures(void);
void FrontendAudio_ConfigureMenuSounds(void);
void Profile_SaveDirtyRecords(void);
void Profile_ResetAllRecordCategories(void);
HRESULT Sound_StartLoopingMusicStream(int param1);
void FrontendMap_FindStageFiles(void);

// Frontend music track ("%s\\select1.adp").
// GLOBAL: CMR2 0x00523d70
char g_strMusicSelect1Adp[16] = "%s\\select1.adp";

void FrontendMenu_UpdateAndSwitchActive(void);

int Game_PrepareScene(SceneNode *pRoot, SceneNode *pCamera, void *pRect, int param);
float Graphics_GetFrameRate(void);
int Game_DrawSceneViewport(struct SceneNode *pRoot, struct SceneNode *pCamera, void *pRect, int bit, BYTE flag);
void Graphics_PresentFrameAndResetCounters(void);

// FPS overlay format string ("FPS: %.2f").

// Renders the state-5 boot screen: clears both targets, queues the loaded
// frontend texture as a sprite centred over the screen and, when the debug
// flag is on, draws the frame rate.
// FUNCTION: CMR2 0x004d0ea0
void Game_DrawBootTextureScreen(CallbackStateMachine *p1, BYTE p2)
{
    SpriteRect screenRect;
    BYTE colour[4];
    int centre[3];
    SpriteRect src;
    SpriteRect dst;

    screenRect.x = 0;
    screenRect.y = 0;
    screenRect.w = g_pGraphics->resX;
    screenRect.h = g_pGraphics->resY;
    colour[0] = 0xff;
    colour[1] = 0xff;
    colour[2] = 0xff;
    colour[3] = 0xff;
    centre[0] = 0;
    centre[1] = 0;
    centre[2] = 0;

    CGraphics::SetClearColour(1, 0x9c, 0xb4, 0xac);
    CGraphics::SetProjection(0x25645, 0x4326e, 0xfa0000, 0x10000);
    CGraphics::ClearTarget();
    CGraphics::ClearZBuffer();

    if (CFrontend::m_unk0x00817fd0 != NULL) {
        src.x = CFrontend::m_unk0x00817fd0->field_0x11c;
        src.y = CFrontend::m_unk0x00817fd0->field_0x11e;
        src.w = CFrontend::m_unk0x00817fd0->width;
        src.h = CFrontend::m_unk0x00817fd0->height;
        dst.x = (int)(g_pGraphics->resX * 43) / 640;
        dst.y = (int)(g_pGraphics->resY * 202) / 480;
        dst.w = src.w;
        dst.h = src.h;
        centre[0] = src.w / 2;
        centre[1] = src.h / 2;
        Sprite_Queue(&src, &dst, CFrontend::m_unk0x00817fd0, 1, 0, centre, NULL, colour, 8);
    }

    Game_PrepareScene(g_splashSceneRoot, g_splashCamera, &screenRect, 0);

    if ((g_pGraphics->field913_0x3bc & 4) != 0) {
        sprintf(CFrontend::m_stringDest, g_strFpsFormat0x00516e14, Graphics_GetFrameRate());
        Font_DrawText(0, CFrontend::m_stringDest, 0, 0, (int *)colour, 9);
    }

    Game_DrawSceneViewport(g_splashSceneRoot, g_splashCamera, &screenRect, 0, 1);

    if (g_bootLoadingDone == 0)
        Graphics_PresentFrameAndResetCounters();
}

// Waits for the pad button (or 5 s), then restarts the music and asks for state 0.
// FUNCTION: CMR2 0x004d1a90
void Game_WaitForButtonOrSplashTimeout(CallbackStateMachine *p1, BYTE p2)
{
    char path[MAX_PATH];
    DeviceInfo *pDevice;

    g_frontendTimestamp = Sys_GetTicks();
    CInput::UpdateAllAvailableDevices();
    Input_TranslatePedalsToMenuKeys();
    pDevice = CInput::GetAvailableDeviceRecord(0);
    Input_MergeAssignedJoystickButtons(0, pDevice);
    if ((pDevice->field_0x8 & 0x10) != 0 || (unsigned int)(g_frontendTimestamp - GameInfo_GetSessionTimestamp()) > 0x1388) {
        GameInfo_ResetSessionTimestamp();
        sprintf(path, g_strMusicSelect1Adp, CInstallInfo::GetMusicDir());
        CSound::OpenStreamingMusicFile(path);
        CSound::SetMusicStreamVolume(CGameInfo::GetMasterSoundVolume());
        Sound_StartLoopingMusicStream(1);
        CGame::PromoteCallbackEntryByRule(p1, p2, 0, 2);
    }
}

// Waits ~5 s, then asks for state 0.
// FUNCTION: CMR2 0x004d1b40
void Game_WaitForSplashTimeout(CallbackStateMachine *p1, BYTE p2)
{
    g_frontendTimestamp = Sys_GetTicks();
    CInput::UpdateAllAvailableDevices();
    Input_TranslatePedalsToMenuKeys();
    Input_MergeAssignedJoystickButtons(0, CInput::GetAvailableDeviceRecord(0));
    if ((unsigned int)(g_frontendTimestamp - GameInfo_GetSessionTimestamp()) > 0x1388) {
        GameInfo_ResetSessionTimestamp();
        CGame::PromoteCallbackEntryByRule(p1, p2, 0, 2);
    }
}

// Stores the frame time and runs the frontend.
// FUNCTION: CMR2 0x004d1c90
void Game_RunFrontendBootFrame(CallbackStateMachine *p1, BYTE p2)
{
    g_frontendTimestamp = Sys_GetTicks();
    g_frontendElapsedMs = g_frontendTimestamp - GameInfo_GetSessionTimestamp();
    FrontendMenu_UpdateAndSwitchActive();
}

// Fade step of the boot/HUD colour: 1/1500 per elapsed millisecond.
// GLOBAL: CMR2 0x00513ec8
float g_bootFadeStep = 1.0f / 1500.0f;
extern const float g_netByteScale;
int Sprite_FillRect(BYTE *unused, short *pRect, BYTE *pColour, int layer);

// Draws a boot/HUD label at (x, y) in a colour that fades out 2.5 s after the
// frame timer was last reset; unless flag is set it also fills the 2 pixel wide
// bar that follows the text. Returns the x after the bar.
// FUNCTION: CMR2 0x004d0d30
int Game_DrawFadingBootLabel(int x, int y, char *pText, char flag)
{
    int elapsed;
    int alpha;
    BYTE colour[4];
    short rect[4];
    int px;

    px = x;
    colour[0] = 0xff;
    colour[1] = 0xff;
    colour[2] = 0xff;
    colour[3] = 0xff;
    elapsed = Sys_GetTicks();
    elapsed = elapsed - GameInfo_GetSessionTimestamp();
    if (elapsed > 0x9c4) {
        if (elapsed > 0xfa0)
            alpha = 0;
        else
            alpha = 0xff -
                    (int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, (elapsed - 0x9c4)), g_netByteScale), g_bootFadeStep));
    } else {
        alpha = 0xff;
    }
    colour[0] = alpha;
    colour[1] = alpha;
    colour[2] = alpha;
    Font_DrawText(2, pText, px, y, (int *)colour, 0x11);
    if (flag == 0) {
        px += Font_GetTextWidth(2, (BYTE *)pText) + (int)(g_pGraphics->resX * 10) / 0x280;
        rect[0] = (short)px;
        rect[1] = (int)(g_pGraphics->resY * 200) / 0x1e0;
        rect[2] = 2;
        rect[3] = (int)(g_pGraphics->resY * 60) / 0x1e0;
        Sprite_FillRect(&g_pGraphics->field309_0x150, rect, colour, 1);
        px += (int)(g_pGraphics->resX * 10) / 0x280;
    }
    return px;
}

// Prototypes for this boot state (defined in other modules or still stubs).
unsigned char RallyDataCountryIndex(void);
unsigned char RallyDataStageIndex(void);
int OptionMovie_StartPlayback(char *path, Texture *pTexture, short *pRect, unsigned int flags, unsigned int track);

// Cleared on entry and set while the intro video of the boot sequence is
// being shown.
extern BYTE g_frontendIntroFlag;

// Uppercase country codes of the intro videos. They are four bytes apart,
// 0x00519264..0x00519280; the last one is CFrontend::m_strUK (already
// declared in Frontend.h).
extern char g_str0x00519268[4];
extern char g_str0x0051926c[4];
extern char g_str0x00519270[4];
extern char g_str0x00519274[4];
extern char g_str0x00519278[4];
extern char g_str0x0051927c[4];
extern char g_str0x00519280[4];

// Country intro video path template ("%s\\%s\\%s.bik").
// GLOBAL: CMR2 0x00523dc0
char g_str0x00523dc0[14] = "%s\\%s\\%s.bik";

// Boot state 7: shows the frontend for four seconds and then plays the
// country intro video before asking for state 0.
// FUNCTION: CMR2 0x004d1cc0
void Game_PlayCountryIntroAfterFrontendDelay(CallbackStateMachine *p1, BYTE p2)
{
    char *codes[8] = {
        g_str0x00519280, g_str0x0051927c, g_str0x00519278, g_str0x00519274,
        g_str0x00519270, g_str0x0051926c, g_str0x00519268, CFrontend::m_strUK,
    };
    char path[MAX_PATH];
    DeviceInfo *pDevice;
    bool queuedVideo;

    g_frontendIntroFlag = 0;
    queuedVideo = false;
    if (CGame::m_unk0x00523d68 != 0 &&
        (CGameInfo::GetConfiguredGameMode() == 0 || CGameInfo::GetConfiguredGameMode() == 1) &&
        RallyDataStageIndex() == 0) {
        g_frontendIntroFlag = 1;
        CSound::CloseMusicStreamAndClearPath(0);
        g_frontendTimestamp = Sys_GetTicks();
        CInput::UpdateAllAvailableDevices();
        Input_TranslatePedalsToMenuKeys();
        pDevice = CInput::GetAvailableDeviceRecord(0);
        Input_MergeAssignedJoystickButtons(0, pDevice);
        if (g_frontendTimestamp - (int)GameInfo_GetSessionTimestamp() > 4000) {
            int language = CGameInfo::GetGameLanguage();
            sprintf(path, g_str0x00523dc0, CInstallInfo::GetCountrySpecificOtherDir(),
                    CGameInfo::GetGameRegionDirectory(), codes[RallyDataCountryIndex() & 0xff]);
            OptionMovie_StartPlayback(path, NULL, NULL, 2, language);
            queuedVideo = true;
        }
        if ((pDevice->field_0x8 & 0x10) == 0 && !queuedVideo)
            return;
    }
    CGame::PromoteCallbackEntryByRule(p1, p2, 0, 2);
}

// Boot pieces called by the exit path of the game state that live in other
// modules.
void GameInfo_ApplyCheatsForGameMode(void);
void GameInfo_SetPlayerOptionNibbles(unsigned int param1, unsigned int param2);
void RallyData_CopyCountryPlayerDefaults(void);
void RallyData_SetSelectionStateByte(char param1);
void RallyData_PickOpponentLineups(void);
bool Profile_SaveEditedNames(void);
void RallyData_IncrementSelectedCategoryCounters(void);
unsigned char RallyDataCountryIndex(void);
unsigned char RallyDataStageIndex(void);
extern int g_frontendCallbackDepth;

// Byte flag the exit path of the boot states tests to pick the next game
// state (also read by the function at 0x4d2070).
// GLOBAL: CMR2 0x008180fa
BYTE g_unk0x008180fa;

// Unwinds the group of callbacks of the current state, saves the data of the
// ending race and asks for the next state (level 2, state index 0).
// FUNCTION: CMR2 0x004d1e90
void Game_FinishRaceAndAdvanceBootState(CallbackStateMachine *p1, BYTE unused)
{
    CGame::SetSecondaryOptionStateByte(0);
    if (CGameInfo::IsRecordFlagSet(0x11))
        CGameInfo::SetFrontendSessionFlag(1);
    else
        CGameInfo::SetFrontendSessionFlag(0);
    if (CGameInfo::GetConfiguredGameMode() != 4)
        CFrontend::ResetFrontendPlayerInputState();

    CSound::CloseMusicStreamAndClearPath(0);
    GameInfo_ApplyCheatsForGameMode();

    if (CGame::m_unk0x008180f9 == 0) {
        if (CGameInfo::GetConfiguredGameMode() != 8 && CGameInfo::GetConfiguredGameMode() != 9 &&
            CGameInfo::GetConfiguredGameMode() != 10 && CGameInfo::GetConfiguredGameMode() != 11 &&
            CGameInfo::GetConfiguredGameMode() != 12)
            RallyData_PickOpponentLineups();

        RallyData_CopyCountryPlayerDefaults();
        RallyData_SetSelectionStateByte(-1);
        CGameInfo::ApplyStageOptionUnlockFlags();

        if (CGameInfo::GetGameInfoSessionFlag() == 0) {
            if (CGame::m_unk0x00523d68 != 0 || CGame::m_unk0x008180f9 != 0 || g_unk0x008180fa != 0)
                GameInfo_SetPlayerOptionNibbles(RallyDataCountryIndex() & 0xff, RallyDataStageIndex() & 0xff);

            Game_SaveConfigurationPreservingFullscreen();
            CNetworkLeaderboards::SaveLeaderboards();
            CInput::SaveControllerInfo();
            Profile_SaveEditedNames();
        }
    }

    CGame::UnwindCallbacks(g_frontendCallbackDepth);
    CGraphics::EvictManagedTextureResources();
    CGraphics::FreeTextureBuffers();
    if (CGameInfo::GetConfiguredGameMode() != 4)
        RallyData_IncrementSelectedCategoryCounters();

    if (CGame::m_unk0x00523d68 != 0) {
        CGame::PromoteCallbackEntryByRule(p1, 0, 0, 2);
        if (CGameInfo::GetConfiguredGameMode() == 0 || CGameInfo::GetConfiguredGameMode() == 1) {
            GameInfo_ResetSessionTimestamp();
            CGame::SetFrontendResourceMode(2);
            return;
        }
        if (CGameInfo::GetConfiguredGameMode() != 2 && CGameInfo::GetConfiguredGameMode() != 3 &&
            CGameInfo::GetConfiguredGameMode() != 8 && CGameInfo::GetConfiguredGameMode() != 9 &&
            CGameInfo::GetConfiguredGameMode() != 10) {
            CGame::SetFrontendResourceMode(3);
            CInput::UpdateInputFrameDelta();
            return;
        }
        CGame::SetFrontendResourceMode(2);
        CInput::UpdateInputFrameDelta();
        return;
    }

    if (CGame::m_unk0x008180f9 != 0) {
        CGame::PromoteCallbackEntryByRule(p1, 0, 0, 2);
        CGame::SetFrontendResourceMode(2);
        return;
    }
    if (g_unk0x008180fa != 0) {
        CGame::PromoteCallbackEntryByRule(p1, 0, 0, 2);
        CGame::SetFrontendResourceMode(3);
        return;
    }
    CGame::PromoteCallbackEntryByRule(p1, 0, 0, 2);
    CGame::SetShouldExit();
}

float Graphics_GetFrameRate(void);
int Game_DrawSceneViewport(struct SceneNode *pRoot, struct SceneNode *pCamera, void *pRect, int bit, BYTE flag);
void Graphics_PresentFrameAndResetCounters(void);
int Game_PrepareScene(SceneNode *pRoot, SceneNode *pCamera, void *pRect, int param);

// Needed by every boot-screen render of this file (0x4d0a80 / 0x4d0ea0 /
// 0x4d1080 / 0x4d1370); the duplicated definition is deduplicated on integration.

// Bink Video credit line drawn at the bottom of the boot screens.
// GLOBAL: CMR2 0x00523cd8
char g_strBinkCredit0x00523cd8[65] = "Uses Bink Video. Copyright (C) 1997-1999 by RAD Game Tools, Inc.";
// Bink Video credit line of the Polish build.
// GLOBAL: CMR2 0x00523d1c
char g_strBinkCreditPl0x00523d1c[73] = "Wykorzystuje Bink Video. Copyright (C) 1997-1999 by RAD Game Tools, Inc.";

// Renders the boot screens: clears the target white, draws the frontend movie
// frame centred, then the FPS counter and the Bink Video credit line.
// FUNCTION: CMR2 0x004d1370
void Game_DrawMovieFrameAndCredits(CallbackStateMachine *p1, BYTE p2)
{
    SpriteRect dst;
    SpriteRect viewport;
    SpriteRect src;

    viewport.x = 0;
    viewport.y = 0;
    viewport.w = (short)g_pGraphics->resX;
    viewport.h = (short)g_pGraphics->resY;
    BYTE white[4] = { 0xff, 0xff, 0xff, 0xff };
    BYTE black[4] = { 0x00, 0x00, 0x00, 0xff };
    int centre[3] = { 0, 0, 0 };

    CGraphics::SetClearColour(1, 0xff, 0xff, 0xff);
    CGraphics::SetProjection(0x25645, 0x4326e, 0xfa0000, 0x10000);
    CGraphics::ClearTarget();
    CGraphics::ClearZBuffer();
    if (CFrontend::m_unk0x00817ebc != NULL) {
        short w;
        short h;
        src.x = CFrontend::m_unk0x00817ebc->field_0x11c;
        src.y = CFrontend::m_unk0x00817ebc->field_0x11e;
        w = CFrontend::m_unk0x00817ebc->width;
        src.w = w;
        h = CFrontend::m_unk0x00817ebc->height;
        src.h = h;
        dst.x = (short)((int)(g_pGraphics->resX * 0x140) / 0x280);
        dst.y = (short)((int)(g_pGraphics->resY * 0xf0) / 0x1e0);
        dst.w = w;
        dst.h = h;
        dst.x -= w / 2;
        dst.y -= h / 2;
        centre[0] = w / 2;
        centre[1] = h / 2;
        Sprite_Queue(&src, &dst, CFrontend::m_unk0x00817ebc, 1, 0, centre, NULL, white, 8);
    }
    Game_PrepareScene(g_splashSceneRoot, g_splashCamera, &viewport, 0);
    if ((g_pGraphics->field913_0x3bc & 4) != 0) {
        sprintf(CFrontend::m_stringDest, g_strFpsFormat0x00516e14, Graphics_GetFrameRate());
        Font_DrawText(0, CFrontend::m_stringDest, 0, 0, (int *)white, 9);
    }
    if (CGameInfo::GetGameRegion() == 3)
        sprintf(CFrontend::m_stringDest, g_strBinkCreditPl0x00523d1c);
    else
        sprintf(CFrontend::m_stringDest, g_strBinkCredit0x00523cd8);
    Font_DrawText(0, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x140) / 0x280,
                  (int)(g_pGraphics->resY * 0x15e) / 0x1e0, (int *)black, 10);
    Game_DrawSceneViewport(g_splashSceneRoot, g_splashCamera, &viewport, 0, 1);
    if (g_bootLoadingDone == 0)
        Graphics_PresentFrameAndResetCounters();
}

// Boot states 0x4d1ba0/0x4d1c30/0x4d1e10 (cascade seed batch 1, verified 96-98%).
// GLOBAL: CMR2 0x00523da4
char g_strCmBik[] = "%s\\cm.bik";
// GLOBAL: CMR2 0x00523db0
char g_strIntroBik[] = "%s\\Intro.bik";
// GLOBAL: CMR2 0x00523d6c
int g_unk0x00523d6c = -1;
// GLOBAL: CMR2 0x00817ff0
unsigned int g_unk0x00817ff0;

// FUNCTION: CMR2 0x004d1ba0
void Game_PlayCodemastersBootVideo(CallbackStateMachine *p1, BYTE state)
{
    char path[260];

    CGraphics::SetClearColour(1, 0, 0, 0);
    CGraphics::ClearTarget();
    // PORT: shows the cleared back buffer (the original blitted it to the
    // primary surface).
    Gfx_Present(TRUE);
    sprintf(path, g_strCmBik, CInstallInfo::GetVideosDir());
    OptionMovie_StartPlayback(path, NULL, NULL, 2, 0);
    GameInfo_ResetSessionTimestamp();
    CGame::PromoteCallbackEntryByRule(p1, state, 0, 2);
}

// FUNCTION: CMR2 0x004d1c30
void Game_PlayIntroBootVideo(CallbackStateMachine *p1, BYTE state)
{
    char path[260];

    sprintf(path, g_strIntroBik, CInstallInfo::GetVideosDir());
    OptionMovie_StartPlayback(path, NULL, NULL, 2, 0);
    GameInfo_ResetSessionTimestamp();
    CGame::PromoteCallbackEntryByRule(p1, state, 0, 2);
}

// FUNCTION: CMR2 0x004d1e10
void Game_UpdateBootInputDelay(CallbackStateMachine *p1, BYTE state)
{
    CInput::UpdateAllAvailableDevices();
    Input_TranslatePedalsToMenuKeys();
    CInput::GetAvailableDeviceRecord(0);
    if (g_unk0x00523d6c == -1) {
        g_unk0x00817ff0 = CMain::GetFrameDelta();
        g_unk0x00523d6c = 0;
    }
    if (500 < (unsigned int)(CMain::GetFrameDelta() - g_unk0x00817ff0)) {
        if (g_unk0x00523d6c < 2) {
            g_unk0x00817ff0 = CMain::GetFrameDelta();
            g_unk0x00523d6c = g_unk0x00523d6c + 1;
            return;
        }
        CGame::PromoteCallbackEntryByRule(p1, state, 0, 2);
    }
}

// FUNCTION: CMR2 0x004d15e0
void CGame::InitializeGame(CallbackStateMachine *p1, BYTE p2)
{
    time_t srandSeed;
    char *skuValue, *skuRegion;
    bool didLoadGameInfo = false;

    srandSeed = Sys_GetUnixTime();
    srand(srandSeed);

    // europe sku check
    skuValue = CRegKey::GetValueFromKey(CRegKey::m_regKeySkuType);
    skuRegion = CRegKey::m_skuEurope;
    if (strcmp(skuRegion, skuValue) == 0)
       CGameInfo::SetGameRegion(0);
    else
    {
       // america sku check
       skuValue = CRegKey::GetValueFromKey(CRegKey::m_regKeySkuType);
       skuRegion = CRegKey::m_skuAmerica;
       if (strcmp(skuRegion, skuValue) == 0)
           CGameInfo::SetGameRegion(1);
       else
       {
           // japan sku check
           skuValue = CRegKey::GetValueFromKey(CRegKey::m_regKeySkuType);
           skuRegion = CRegKey::m_skuJapan;
           if (strcmp(skuRegion, skuValue) == 0)
               CGameInfo::SetGameRegion(2);
           else
           {
               // poland sku check
               skuValue = CRegKey::GetValueFromKey(CRegKey::m_regKeySkuType);
               skuRegion = CRegKey::m_skuPoland;
               if (strcmp(skuRegion, skuValue) == 0)
                   CGameInfo::SetGameRegion(3);
               else // otherwise die
                   CGame::SetShouldExit();
           }
       }
    }

    CGameInfo::InitRegionLanguageCount();
    m_unk0x00523d68 = 1;
    m_unk0x008180f9 = 0;
    m_unk0x008180fc = GetConfigurationStateByte();
    if (GetConfigurationStateByte() != 0)
    {
        // First boot: load the configuration, bring up DirectX, sound and the frontend.
        SetProfileSelectionState(0);
        SetSecondaryOptionStateByte(0);
        ConsumeOptionRefreshRequest();
        CGameInfo::SetGameModeOptionBit19(0);
        if ((BYTE)CInstallInfo::LoadInstallPathsFromRegistry() == 0)
            goto exit;

        CGameInfo::InitDefaultGameInfo();
        CGameInfo::ResetStageOptionStates();
        CGameInfo::ApplyStageOptionUnlockFlags();
        CGameInfo::SetupInputs(2);
        didLoadGameInfo = CGameInfo::LoadGameInfo();
        CNetworkLeaderboards::Reset();
        CNetworkLeaderboards::LoadLeaderboards();
        CGameInfo::SetGameModeOptionBit19(0);
        if (CGraphics::InitializeDirectX())
            CGraphics::SetDefaults();
        CInput::LoadControllerInfo();
        CGameInfo::SetInputAndGamePaused(1);
        CGameInfo::InitFrontendSessionBuffer();
        if (InitializeNetworkSubsystem() == false)
            goto exit;
        if (CGame::LoadAndInitializeSplashScreens(true) == false)
            goto exit;

        CGraphics::SetClearColour(1, 0, 0, 0);
        SavedGames_LoadRecords();
        CGame::RegisterCallback(SavedGames_ReleaseRecords, NULL);
        Sound_Init(0x5622, 2, 0x10, 0);
        CGraphics::SetClearColour(1, 0x9c, 0xb4, 0xac);
        FrontendAudio_LoadSounds();
        if (FrontendText_LoadFonts() == 0)
            goto exit;
        if (FrontendText_LoadRegionLanguages() == 0)
            goto exit;
        FrontendCredits_LoadText(1);
        RallyData_ResetSelection();
        RallyData_ResetSavedPlayerRecords();
        FrontendScroller_ResetAll();
        FrontendMenu_BuildPagesAndSelectInitial(didLoadGameInfo == false, 1);
        FrontendAnimation_LoadDotTextures();
        FrontendAudio_ConfigureMenuSounds();
        Game_SetConfigurationStateByte(0);
        RallyData_ClearDriverGroupRecords();
        RallyData_ClearDriverSkillFlags();
        CGame::PromoteCallbackEntryByRule(p1, p2, 1, 2);
        GameInfo_ResetSessionTimestamp();
        FrontendMap_FindStageFiles();
        return;
    }

    // Back from a race: save state, reload the frontend and restart its music.
    if (CGameInfo::GetConfiguredGameMode() == 4)
        g_unk0x00819744 = 0;
    SetSecondaryOptionStateByte(0);
    if (ConsumeOptionRefreshRequest())
        CGameInfo::SetGameModeOptionBit19(0);
    else
    {
        if (CGameInfo::GetGameModeOptionBit19() != 0 &&
            (CGameInfo::GetConfiguredGameMode() == 10 || CGameInfo::GetConfiguredGameMode() == 12))
            SetSecondaryOptionStateByte(1);
        if (Network_GetSessionStateFlag())
            Session_SetOpen(1);
        g_unk0x00818ce4 = 0;
        FrontendNetwork_SendPlayerDescription();
    }
    if (CGameInfo::GetGameInfoSessionFlag())
    {
        GameInfo_RestoreFrontendOptionSettings();
        CGameInfo::SetGameInfoSessionFlag(0);
    }
    CGameInfo::ApplyStageOptionUnlockFlags();
    Game_SaveConfigurationPreservingFullscreen();
    CNetworkLeaderboards::SaveLeaderboards();
    CInput::SaveControllerInfo();
    CGameInfo::SetInputAndGamePaused(1);
    if (CGame::LoadAndInitializeSplashScreens(false) == false)
        goto exit;

    if (Game_GetOptionStateByte() == 0)
        CGraphics::SetClearColour(1, 0x9c, 0xb4, 0xac);
    else
        CGraphics::SetClearColour(1, 0, 0, 0);
    SavedGames_LoadRecords();
    CGame::RegisterCallback(SavedGames_ReleaseRecords, NULL);
    Sound_Init(0x5622, 2, 0x10, 0);
    FrontendAudio_LoadSounds();
    if (FrontendText_LoadFonts() == 0 || FrontendText_LoadRegionLanguages() == 0)
        goto exit;
    FrontendCredits_LoadText(1);
    FrontendScroller_ResetAll();
    FrontendMenu_BuildPagesAndSelectInitial(0, 0);
    FrontendAnimation_LoadDotTextures();
    FrontendAudio_ConfigureMenuSounds();
    sprintf(CFrontend::m_stringDest, g_strMusicSelect1Adp, CInstallInfo::GetMusicDir());
    CSound::OpenStreamingMusicFile(CFrontend::m_stringDest);
    CSound::SetMusicStreamVolume(CGameInfo::GetMasterSoundVolume());
    Sound_StartLoopingMusicStream(1);
    Profile_SaveDirtyRecords();
    if (CGameInfo::GetGameModeOptionBit19() == 0 && Game_GetOptionStateByte() == 0)
        Profile_ResetAllRecordCategories();
    RallyData_ClearDriverGroupRecords();
    RallyData_ClearDriverSkillFlags();
    GameInfo_ResetSessionTimestamp();
    if (Game_GetOptionStateByte() == 1) {
        Game_SetOptionStateByte(0);
        CGame::PromoteCallbackEntryByRule(p1, p2, 6, 2);
    } else {
        CGame::PromoteCallbackEntryByRule(p1, p2, 0, 2);
    }
    FrontendMap_FindStageFiles();
    return;

exit:
    CGame::SetShouldExit();
}

// FUNCTION: CMR2 0x0049c2c0
void CGame::RunStateUpdateCallbacks(CallbackStateMachine *param1)
{
    StateUpdateCallback func;
    BYTE counter;
    BYTE stateIndex;

    counter = 0;
    stateIndex = 0;
    if (param1->count > 0)
    {
        do
        {
            func = param1->callbacks[param1->records[stateIndex].packedState & 0xFF].update;
            if (func != NULL)
                (func)(param1, stateIndex);

            counter++;
            stateIndex = counter;
        } while (counter < param1->count);
    }
}

// FUNCTION: CMR2 0x0049c310
void CGame::RunStateRenderCallbacks(CallbackStateMachine *param1)
{
    StateRenderCallback func;
    BYTE counter;
    BYTE stateIndex;

    if (m_skipRenderCallbacks == 0)
    {

        counter = 0;
        stateIndex = 0;
        if (param1->count > 0)
        {
            do
            {
                func = param1->callbacks[param1->records[stateIndex].packedState & 0xFF].render;
                if (func != NULL)
                    (func)(param1, stateIndex);

                counter++;
                stateIndex = counter;
            } while (counter < param1->count);
        }
    }
    else
        m_skipRenderCallbacks = 0;
}

// FUNCTION: CMR2 0x0049c370
void CGame::AdvanceCallbackStateTimers(CallbackStateMachine *param1)
{
    unsigned int tVar1;

    m_callbackIndex.index = 0;
    if (param1->count > 0)
    {
        do
        {
            m_currentCallbackRecord = &param1->records[m_callbackIndex.packed & 0xff];
            tVar1 = m_currentCallbackRecord->packedState;
            if (tVar1 & 0x3000000)
            {
                m_currentCallbackRecord->packedState = ((tVar1 >> 0x10) & 0xff) | (tVar1 & 0xffffff00);
                m_currentCallbackRecord->packedState = m_currentCallbackRecord->packedState & 0xff00ffff;
                m_currentCallbackRecord->packedState = m_currentCallbackRecord->packedState & 0xfcffffff;
                m_currentCallbackRecord->elapsedTicks = 0;
            }
            else
                m_currentCallbackRecord->elapsedTicks = m_currentCallbackRecord->elapsedTicks + 1;

            m_callbackIndex.index++;
        } while (m_callbackIndex.index < param1->count);
    }
}

// FUNCTION: CMR2 0x0049c150
void CGame::InitializeCallbackStateRecord(CallbackStateRecord *param1, int param2, int param3)
{
    param1->elapsedTicks = 0;
    param1->bits.state = (BYTE)param2;
    param1->bits.value = (BYTE)param3;
    param1->bits.rule = 0;
    param1->bits.level = 0;
}

// FUNCTION: CMR2 0x0049c190
void CGame::InitializeCallbackStateMachine(CallbackStateMachine *p1, BYTE count, CallbackStateRecord *records, StateCallbackPair *callbacks, unsigned int *rules)
{
    p1->count = count;
    p1->records = records;
    p1->callbacks = callbacks;
    p1->rules = rules;
}

// Promotes entry index of the table to the given level when a rule of
// p->rules (terminated by 0xffffffff, 0xff bytes are wildcards) matches it
// with the given value; the rule's top byte becomes the entry's third byte.
// FUNCTION: CMR2 0x0049c1c0
int CGame::PromoteCallbackEntryByRule(CallbackStateMachine *p, BYTE index, BYTE value, int level)
{
    unsigned int *pEntry;
    unsigned int *pRule;
    unsigned int entry;
    unsigned int rule;

    pEntry = (unsigned int *)&p->records[index].packedState;
    entry = *pEntry;
    if ((entry & 0x3000000) == 0x3000000 || (int)((entry >> 24) & 3) < level) {
        for (pRule = p->rules;; pRule++) {
            rule = *pRule;
            if ((rule & 0xff) == 0xff && (rule & 0xff00) == 0xff00 && (rule & 0xff0000) == 0xff0000 &&
                (rule & 0xff000000) == 0xff000000)
                break;
            if ((((rule ^ entry) & 0xff) == 0 || (rule & 0xff) == 0xff) &&
                (((rule ^ entry) & 0xff00) == 0 || (rule & 0xff00) == 0xff00) && ((rule >> 16) & 0xff) == value) {
                ((CallbackStateRecord *)pEntry)->bits.level = level;
                ((CallbackStateRecord *)pEntry)->bits.rule = *pRule >> 24;
                return 1;
            }
        }
    }
    return 0;
}
// GLOBAL: CMR2 0x0082a7f0
CallbackStateMachine g_secondaryCallbackMachine;

// FUNCTION: CMR2 0x004ff440
CallbackStateMachine *Game_GetSecondaryCallbackMachine(void)
{
    return &g_secondaryCallbackMachine;
}
// GLOBAL: CMR2 0x0082a800
CallbackStateRecord g_secondaryCallbackRecord;
// GLOBAL: CMR2 0x0082a908
BYTE g_secondaryCallbackInitialized;
// State 0x500df0 (country menus) is defined in GameInfo.cpp.
void OptionMenu_EnterCountryState(CallbackStateMachine *p1, BYTE p2);

// State of the option menu machine (GameInfo.cpp).
void OptionMenu_EnterStartState(CallbackStateMachine *p1, BYTE state);

void OptionMenu_UpdateFadeState(CallbackStateMachine *p1, BYTE state);
void OptionMenu_FadeBackgroundToGrey(int param1, int unused);
void OptionMenu_FadeToStoredColour(int param1, int unused);

// GLOBAL: CMR2 0x00526ee0
StateCallbackPair g_secondaryStateCallbacks[7] = {
    {OptionMenu_EnterStartState, NULL},
    {OptionMenu_UpdateHiddenPreviewState, (StateRenderCallback)OptionMenu_FadeBackgroundToGrey},
    {OptionMenu_EnterCountryState, NULL},
    {OptionMenu_UpdateFadeState, OptionMenu_DrawBackgroundState},
    {(StateUpdateCallback)OptionMenu_StartDelayedFade, (StateRenderCallback)OptionMenu_FadeToStoredColour},
    {OptionMenu_LeaveAndCommitState, CGame::NoOpSecondaryStateCallback},
    {OptionMenu_LeaveWithoutCommitState, CGame::NoOpSecondaryStateCallback},
};
// Transition rules of the secondary (option/country menu) callback machine.
// Keep the entire table and its terminator: the dispatcher scans its address.
// GLOBAL: CMR2 0x00526f18
unsigned int g_secondaryStateRules[10] = {
    0x0100ff00, 0x0200ff01, 0x0300ff02, 0x0400ff03, 0x0201ff03,
    0x0500ff04, 0x0602ff04, 0x0000ff05, 0x0000ff06, 0xffffffff,
};

// FUNCTION: CMR2 0x004ff450
BOOL CGame::UpdateSecondaryCallbackMachine()
{
    if (g_secondaryCallbackInitialized != 0) {
        RunStateUpdateCallbacks(&g_secondaryCallbackMachine);
        RunStateRenderCallbacks(&g_secondaryCallbackMachine);
        AdvanceCallbackStateTimers(&g_secondaryCallbackMachine);
        return FALSE;
    }
    InitializeCallbackStateRecord(&g_secondaryCallbackRecord, 0, 0xFF);
    InitializeCallbackStateMachine(&g_secondaryCallbackMachine, 1, &g_secondaryCallbackRecord, g_secondaryStateCallbacks, g_secondaryStateRules);
    g_secondaryCallbackInitialized = 1;
    return FALSE;
}
// In-race callback group of the game state machine (0x0041b060), filled by
// InitializeCallbackStateMachine; its slots live at 0x537de0.
extern RaceCallbackStorage g_raceCallbackStorage;
extern BYTE g_raceEndLatch;
extern BYTE g_unk0x00537ef5;
extern int g_raceEndPending;
extern int g_raceEndState;
extern int g_unk0x00537df0;

void InRaceMenu_UpdateStateMachineFrame(CallbackStateMachine *param1);
void InRaceMenu_AdvanceScreenEntries(CallbackStateMachine *param1);
void StageUI_ResetRaceEndEventCount(void);
unsigned int *RallyData_GetChampionshipState(void);
char RallyData_GetUsableRecordCategory(BYTE param1);
int RallyData_IsChampionshipFinalStage(void);
void RallyData_SetSelectionBits14To15(BYTE param1);
void Car_BuildRaceOrder(int count);
void Dash_Reset(void);
void Race_EnterCurrentModeScene(CallbackStateMachine *param1, int param2);
void Race_UpdatePauseState(CallbackStateMachine *p, BYTE index);
void RallyData_DrawProjectedChallengeScene(int param1, int param2);
void Race_StartArcadeScene(CallbackStateMachine *param1, int param2);
void RallyData_RunChallengeLoadingScreen(int param1, char param2);
void GameInfo_EnterStageGroup(CallbackStateMachine *param1, int param2);
void GameInfo_LeaveStageGroup(CallbackStateMachine *param1, int param2);
void InRaceMenu_AdvanceCascade(CallbackStateMachine *param1, int param2);
void RallyData_GrowEntryAndInvokeAction(CallbackStateMachine *param1, unsigned int param2);
void Knockout_UpdateRaceStateAndFades(CallbackStateMachine *param_1, unsigned int param_2);
void StageObject_ClearAndDrawSplitPositions(int unused1, int unused2);
void Race_UpdateDriverPreRaceScene(CallbackStateMachine *param1, unsigned int param2);
void Race_UpdateDriverCountdown(CallbackStateMachine *param1, unsigned int param2);
void Race_UpdateDriverReadyAndRecordState(CallbackStateMachine *param1, unsigned int param2);
void Race_HandleStageReplayViewTransitions(CallbackStateMachine *param1, unsigned int param2);
void Race_UpdateInterStageFadeJobs(CallbackStateMachine *param1, unsigned int param2);
void Race_UpdateState12(CallbackStateMachine *p, BYTE index);
void Race_LeaveAndFadeOut(CallbackStateMachine *param1, char param2);
void RallyData_GrowEntryAndRefreshRecords(CallbackStateMachine *param1, unsigned int param2);
void RallyData_GrowEntryPanel(CallbackStateMachine *param1, unsigned int param2);
void RallyData_UpdateUnavailableEntryText(CallbackStateMachine *param1, unsigned int param2);
void RallyData_ShrinkEntryAndNotifyCompletion(CallbackStateMachine *param1, unsigned int param2);
void RallyData_ShrinkEntryAndDrawStageStarted(CallbackStateMachine *param1, unsigned int param2);

// In-race state table: pairs of (update, render) callbacks indexed by the state
// of each slot of the callback group (0x537dd0).
// GLOBAL: CMR2 0x005190b0
StateCallbackPair g_raceStateCallbacks[14] = {
    {(StateUpdateCallback)Race_EnterCurrentModeScene, NULL},
    {(StateUpdateCallback)Race_UpdatePauseState, (StateRenderCallback)RallyData_DrawProjectedChallengeScene},
    {(StateUpdateCallback)Race_StartArcadeScene, (StateRenderCallback)RallyData_RunChallengeLoadingScreen},
    {(StateUpdateCallback)GameInfo_EnterStageGroup, NULL},
    {(StateUpdateCallback)GameInfo_LeaveStageGroup, NULL},
    {(StateUpdateCallback)InRaceMenu_AdvanceCascade, (StateRenderCallback)RallyData_GrowEntryAndInvokeAction},
    {(StateUpdateCallback)Knockout_UpdateRaceStateAndFades, (StateRenderCallback)StageObject_ClearAndDrawSplitPositions},
    {(StateUpdateCallback)Race_UpdateDriverPreRaceScene, (StateRenderCallback)RallyData_GrowEntryAndRefreshRecords},
    {(StateUpdateCallback)Race_UpdateDriverCountdown, (StateRenderCallback)RallyData_GrowEntryPanel},
    {(StateUpdateCallback)Race_UpdateDriverReadyAndRecordState, (StateRenderCallback)RallyData_UpdateUnavailableEntryText},
    {(StateUpdateCallback)Race_UpdateInterStageFadeJobs, (StateRenderCallback)RallyData_ShrinkEntryAndNotifyCompletion},
    {(StateUpdateCallback)Race_HandleStageReplayViewTransitions, (StateRenderCallback)CGame::NoOpSecondaryStateCallback},
    {(StateUpdateCallback)Race_UpdateState12, NULL},
    {(StateUpdateCallback)Race_LeaveAndFadeOut, (StateRenderCallback)RallyData_ShrinkEntryAndDrawStageStarted},
};

// State transition rules of the in-race machine (byte 0 = current state, 0xff
// any; byte 1 = slot level, 0xff any; byte 2 = value; byte 3 = next state),
// terminated by 0xffffffff.
// GLOBAL: CMR2 0x00519120
unsigned int g_raceStateRules[26] = {
    0x0100ff00, 0x0200ff01, 0x0c01ff01, 0x0300ff02, 0x0400ff03, 0x0500ff04,
    0x0700ff05, 0x0605ff05, 0x0d06ff05, 0x0a01ff05, 0x0700ff06, 0x0800ff07,
    0x0900ff08, 0x0a00ff09, 0x0b00000a, 0x0b00010a, 0x0b00040a, 0x0b00020a,
    0x0b00030a, 0x0000ff0b, 0x0407ff0b, 0x0403ff0b, 0x0000ff0c, 0x0504ffff,
    0x0b01ffff, 0xffffffff
};

// Entry point of the in-race state machine: on the first frame it builds the
// callback group of the current in-race mode, on later frames it only
// dispatches the transition of the current state.
//
// The logic is faithful; the remaining gap is codegen shape. Exact differences
// against the original (643 bytes):
//  1. switch block layout (the bulk). The original puts the switch epilogue
//     (`mov [esp+0x14],2; jmp state1`) right after case 1 (0x41b190), so every
//     `break` reaches it with a short jump and case 2's second FUN_00408500 call
//     is tail-merged into case 1's (`jmp 0x41b187`). MSVC6 12.00.8804 from any
//     source shape probed instead keeps an epilogue copy after every case and
//     duplicates case 2's call. Case 0 keeps its own copy of the epilogue in the
//     original (0x41b14e), reproduced here with the explicit `goto state1`.
//  2. stack slots are swapped: original has `state` at [esp+0x10] and `level` at
//     [esp+0x14]; ours is the reverse (9 operand mismatches). Declaration order
//     does not move them.
//  3. the loop's else branch has a dead `mov al,[g_raceEndLatch]; test al,al`
//     before `push 0` that no source form probed reproduces (2 instructions).
//  4. the loop counter is a dword in the original (`mov ebx,ebp; and ebx,0xff;
//     dec ebx`); the byte `i` here compiles to `dec bl`.
//  5. `level` is masked into a copy in the original (`mov ecx,edi; and ecx,0xff`)
//     keeping the unmasked value in edi for the call argument; ours masks edi.
// match 82%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0041b060
BOOL CGame::UpdateInRaceCallbackMachine(void)
{
    unsigned int *pState;
    CallbackStateRecord *pSlot;
    BYTE level;
    BYTE state;
    BYTE i;
    char c;

    if (g_raceEndLatch != 0 && g_unk0x00537ef5 == 0 && g_raceEndPending == 0) {
        if (CGameInfo::IsInRaceMenuOpen() != 0) {
            InRaceMenu_UpdateStateMachineFrame(&g_raceCallbackStorage.machine);
            InRaceMenu_AdvanceScreenEntries(&g_raceCallbackStorage.machine);
            AdvanceCallbackStateTimers(&g_raceCallbackStorage.machine);
            return FALSE;
        }
        RunStateUpdateCallbacks(&g_raceCallbackStorage.machine);
        RunStateRenderCallbacks(&g_raceCallbackStorage.machine);
        AdvanceCallbackStateTimers(&g_raceCallbackStorage.machine);
        return FALSE;
    }
    pState = RallyData_GetChampionshipState();
    if (g_raceEndLatch == 0)
        StageUI_ResetRaceEndEventCount();
    state = CGameInfo::GetConfiguredPlayerCount();
    c = (char)CGameInfo::GetConfiguredGameMode();
    if (c == 4) {
        switch ((int)((*pState >> 3) & 7) - 1) {
        case 0:
            if (RallyData_GetUsableRecordCategory((BYTE)pState[((*pState >> 0xc) & 0xf) * 3 + 0x16] & 0x1f) == -1 &&
                RallyData_GetUsableRecordCategory((pState[((*pState >> 0xc) & 0xf) * 3 + 0x16] >> 5) & 0x1f) == -1)
                goto fail;
            level = 2;
            goto state1;
        case 1:
            if (RallyData_GetUsableRecordCategory((BYTE)pState[((*pState >> 0xc) & 0xf) * 3 + 10] & 0x1f) == -1 &&
                RallyData_GetUsableRecordCategory((pState[((*pState >> 0xc) & 0xf) * 3 + 10] >> 5) & 0x1f) == -1)
                goto fail;
            level = 2;
            goto state1;
        case 2:
            if (RallyData_GetUsableRecordCategory((BYTE)pState[((*pState >> 0xc) & 0xf) * 3 + 4] & 0x1f) == -1 &&
                RallyData_GetUsableRecordCategory((pState[((*pState >> 0xc) & 0xf) * 3 + 4] >> 5) & 0x1f) == -1)
                goto fail;
            level = 2;
            goto state1;
        case 3:
            if (RallyData_GetUsableRecordCategory((BYTE)pState[1] & 0x1f) == -1 &&
                RallyData_GetUsableRecordCategory((pState[1] >> 5) & 0x1f) == -1)
                goto fail;
            level = 2;
            goto state1;
        default:
fail:
            level = 3;
            state = 2;
            goto done;
        }
    } else {
        if ((BYTE)RallyData_IsChampionshipFinalStage() == 0 && state > 1) {
            if (CGameInfo::GetConfiguredGameMode() != 3 && 2 >= state && CGameInfo::IsConfiguredMultiplayer() == 0) {
                state = 2;
                level = 1;
                goto done;
            }
            level = 4;
        } else {
            level = 0;
        }
    }
state1:
    state = 1;
done:
    RallyData_SetSelectionBits14To15(state);
    g_unk0x00537df0 = level;
    if (state > 0) {
        pSlot = g_raceCallbackStorage.records;
        i = state;
        do {
            if (g_raceEndPending != 0) {
                g_raceEndState = 1;
                InitializeCallbackStateRecord(pSlot, 5, level);
            } else {
                InitializeCallbackStateRecord(pSlot, 0, level);
            }
            pSlot++;
            i--;
        } while (i != 0);
    }
    InitializeCallbackStateMachine(&g_raceCallbackStorage.machine, state, g_raceCallbackStorage.records,
                 g_raceStateCallbacks, g_raceStateRules);
    if (g_raceEndPending != 0) {
        Car_BuildRaceOrder(2);
        Dash_Reset();
    }
    g_raceEndLatch = 1;
    g_unk0x00537ef5 = 0;
    g_raceEndPending = 0;
    return FALSE;
}


// FUNCTION: CMR2 0x00501680
void CGame::NoOpSecondaryStateCallback(struct CallbackStateMachine *, BYTE) { return; }

extern SceneNode *g_optionMenuCamera;

void FixMatrix_ProjectWorldPointToView(int *pOut, FixVector *pPoint, SceneNode *pView);

// Projects a 16.16 point through the option menu's background camera and scales
// the result by 2/3.
// FUNCTION: CMR2 0x00501690
void Game_ProjectMenuBackgroundPoint(int *param1, FixVector *param2)
{
    FixMatrix_ProjectWorldPointToView(param1, param2, g_optionMenuCamera);
    param1[0] = FixMul(param1[0], FixDiv(0x20000, 0x30000));
    param1[1] = FixMul(param1[1], FixDiv(0x20000, 0x30000));
}

// Saves the game configuration (Configuration\GameInfo.rcf), keeping the
// current fullscreen setting.
// FUNCTION: CMR2 0x004ea840
void Game_SaveConfigurationPreservingFullscreen(void)
{
    CGameInfo::SetFullscreen((BYTE)g_pGraphics->isFullscreen);
    sprintf(CFrontend::m_stringDest, CGameInfo::m_stringGameInfoRCF, CInstallInfo::GetGameHDPath());
    CInstallInfo::WriteFileToDisk(CFrontend::m_stringDest, 0, &CGameInfo::m_gameInfo, sizeof(GameInfo));
}

// FUNCTION: CMR2 0x004ea880
BYTE CGame::GetConfigurationStateByte(void) { return m_unk0x00516120; }

// FUNCTION: CMR2 0x004ea890
void Game_SetConfigurationStateByte(BYTE param1)
{
    CGame::m_unk0x00516120 = param1;
}

// FUNCTION: CMR2 0x004083e0
void CGame::SetProfileSelectionState(BYTE param1)
{
    m_unk0x00531768 = param1;
}

// GLOBAL: CMR2 0x0052ea50
BYTE g_unk0x0052ea50;

// FUNCTION: CMR2 0x004067b0
BYTE Game_GetOptionStateByte(void)
{
    return g_unk0x0052ea50;
}

// FUNCTION: CMR2 0x004067c0
void Game_SetOptionStateByte(BYTE param1)
{
    g_unk0x0052ea50 = param1;
}

// FUNCTION: CMR2 0x004067d0
void Game_RequestOptionRefresh(void)
{
    CGame::m_unk0x0052ea58 = 1;
}

// FUNCTION: CMR2 0x00406800
BYTE Game_GetSecondaryOptionStateByte(void)
{
    return CGame::m_unk0x0052ea59;
}

// FUNCTION: CMR2 0x00406810
void CGame::SetSecondaryOptionStateByte(BYTE param1)

{
    m_unk0x0052ea59 = param1;
    return;
}

// FUNCTION: CMR2 0x004067e0
bool CGame::ConsumeOptionRefreshRequest(void)
{
    if (m_unk0x0052ea58 != 0)
    {
        m_unk0x0052ea58 = 0;
        return true;
    }
    return false;
}

// FUNCTION: CMR2 0x0049c0a0
int CGame::RegisterCallback(void *param1, void *param2) {
    int iVar2;
    void **piVar3;

    if (param1 == NULL)
        return -1;

    iVar2 = 0;
    if (m_unk0x00593ba0 > 0) {
        piVar3 = m_callbacks;
        while (1) {
            if (param1 == *piVar3)
                return iVar2;
    
            iVar2++;
            piVar3++;
            
            if (iVar2 >= m_unk0x00593ba0) break;
        }
    }

    if (m_unk0x00593ba0 >= 0x40)
        return -1;

    m_callbacks[m_unk0x00593ba0] = param1;
    return m_unk0x00593ba0++;
}

// FUNCTION: CMR2 0x004a17b0
void CGame::FreeNetworkReceiveBuffer(void) {
    m_unk0x005a1fbc = FALSE;
    if (m_unk0x005a1fb8 != NULL) {
        CFileBuffer::FreeGenericFileBuffer(m_unk0x005a1fb8);
        m_unk0x005a1fb8 = NULL;
    }    
}

// The original calls this wrapper instead of FreeNetworkReceiveBuffer from CGame::Cleanup;
// it compiles to a five byte tail jump.
// FUNCTION: CMR2 0x004a17e0
void CGame::ReleaseNetworkReceiveBuffer(void) {
    FreeNetworkReceiveBuffer();
}

// match 56%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004a17f0
void CGame::ResetSessionPlayerTable(bool param1) {
    if (param1)
        m_unk0x005a1fc0 = false;

    // The original walks a pointer to longName and stops when it passes
    // 0x5a1e34, which clears exactly the 7 entries (0x5a1820..0x5a1dd0).
    SessionPlayerRecord *dest = m_sessionPlayers;
    do {
        // The original calls sprintf through a fixed 2-argument prototype; the
        // non-variadic call makes MSVC6 defer the stack cleanup across the pair
        // (the merged `add esp,0x10` of the original).
        // PORT: copy text directly; avoid calling sprintf through an incompatible type.
        strcpy(dest->shortName, CMain::m_logFileBlankLine);
        strcpy(dest->longName, CMain::m_logFileBlankLine);
        dest->playerId = 0;
        dest->active = 0;
        dest++;
    } while ((INT_PTR)dest->longName < (INT_PTR)m_sessionPlayers[7].longName); // 0x5a1e34 in the original

    m_sessionPlayerCount = 0;
}

// FUNCTION: CMR2 0x004a1a90
BOOL CGame::DestroyLocalNetworkPlayer(void) {
    if (m_unk0x005a1fc0 && GetDirectPlay() != NULL)
        return Net_DestroyPlayer(m_localPlayerId) == NET_OK;
    return FALSE;
}

// FUNCTION: CMR2 0x004aaa10
void CGame::FreeServiceProviderConnections(void) {
    int i;

    for (i = 0; i < 10; i++) {
        if (m_connections[i].pConnection != NULL) {
            CFileBuffer::FreeGenericFileBuffer(m_connections[i].pConnection);
            m_connections[i].pConnection = NULL;
        }
    }
}

// FUNCTION: CMR2 0x004aaac0
bool CGame::Cleanup(void)
{
  DestroyLocalNetworkPlayer();
  DestroyDirectPlayLobby();
  DestroyDirectPlay();
  FreeServiceProviderConnections();
  ReleaseNetworkReceiveBuffer();
  return 1;
}

// FUNCTION: CMR2 0x004aab40
// PORT: closes the session (the network layer itself stays).
void CGame::DestroyDirectPlay(void) {
    if (m_pDirectPlay4A != NULL) {
        Net_Close();
        m_pDirectPlay4A = NULL;
    }
}

// FUNCTION: CMR2 0x004aabb0
void CGame::DestroyDirectPlayLobby(void) {
    m_pDirectPlayLobby3A = NULL;
}

// FUNCTION: CMR2 0x004aad40
void *CGame::GetDirectPlay(void) {
    return m_pDirectPlay4A;
}

// FUNCTION: CMR2 0x004aaa40
bool CGame::InitializeNetworkSubsystem(void) {
    int i;

    m_pDirectPlay4A = NULL;
    m_pDirectPlayLobby3A = NULL;

    CGameInfo::ResetNetworkSessionState();
    ResetSessionPlayerTable(true);
    FreeNetworkReceiveBuffer();
    
    for (i = 0; i < 10; i++) {
        sprintf(m_connections[i].name, CMain::m_logFileBlankLine);
        m_connections[i].pConnection = NULL;
    }

    m_connectionCount = 0;

    RegisterCallback(Cleanup, NULL);

    return true;
}

// FUNCTION: CMR2 0x004d0a50
bool CGame::LoadAndInitializeSplashScreens(bool param1) {
    BOOL didLoadSplashScreens;
    BOOL bVar2;

    didLoadSplashScreens = CFrontend::LoadSplashScreens(param1);
    if (didLoadSplashScreens != FALSE) {
        Font_SetBlendMode(TRUE);
        LoadFrontendCommonAndCountryTextures();
        
        return true;
    }

    return false;
}

// FUNCTION: CMR2 0x004e2e50
void CGame::LoadFrontendCommonAndCountryTextures(void) {
    char countryNames[8][10] = { "Finland", "Greece", "France", "Sweden", "Aus", "Kenya", "Italy", "UK" };
    char countryCodes[8][10] = { "Fin", "Gre", "Fra", "Swe", "Aus", "Ken", "Ita", "UK" };
    BOOL bZero = false;

    sprintf(CFrontend::m_stringDest, CFrontend::m_strFrontendTexturesAr640ATGA, CInstallInfo::GetGameCDPath());
    CFrontend::m_pAr640ATexture = CTexture::FindLoadTexture(CGenericFileLoader::GetGenericFile(), CFrontend::m_stringDest, NULL, NULL, bZero, bZero);

    sprintf(CFrontend::m_stringDest, CFrontend::m_strFrontendTexturesAr640DTGA, CInstallInfo::GetGameCDPath());
    CFrontend::m_pAr640DTexture = CTexture::FindLoadTexture(CGenericFileLoader::GetGenericFile(), CFrontend::m_stringDest, NULL, NULL, bZero, bZero);

    sprintf(CFrontend::m_stringDest, CFrontend::m_strFrontendTexturesLgMatrixTGA, CInstallInfo::GetGameCDPath());
    CFrontend::m_pLgMatrixTexture = CTexture::FindLoadTexture(CGenericFileLoader::GetGenericFile(), CFrontend::m_stringDest, NULL, NULL, bZero, bZero);

    sprintf(CFrontend::m_stringDest, CFrontend::m_strFrontendTexturesSmMatrixTGA, CInstallInfo::GetGameCDPath());
    CFrontend::m_pSmMatrixTexture = CTexture::FindLoadTexture(CGenericFileLoader::GetGenericFile(), CFrontend::m_stringDest, NULL, NULL, bZero, bZero);

    // Load country banners and flags
    for (int i = 0; i < 8; i++) {
        sprintf(CFrontend::m_stringDest, CFrontend::m_strSetupRepTexturesBanners, CInstallInfo::GetGameCDPath(), countryCodes[i]);
        CFrontend::m_pSetupRepBanners[i] = CTexture::FindLoadTexture(CGenericFileLoader::GetGenericFile(), CFrontend::m_stringDest, NULL, NULL, bZero, bZero);
        
        sprintf(CFrontend::m_stringDest, CFrontend::m_strFrontendTinyFlags, CInstallInfo::GetGameCDPath(), countryNames[i]);
        CFrontend::m_pTinyFlags[i] = CTexture::FindLoadTexture(CGenericFileLoader::GetGenericFile(), CFrontend::m_stringDest, NULL, NULL, bZero, bZero);
    }

    // Load medal textures
    sprintf(CFrontend::m_stringDest, CFrontend::m_strFrontendTexturesTGoldTGA, CInstallInfo::GetGameCDPath());
    CFrontend::m_pTGold = CTexture::FindLoadTexture(CGenericFileLoader::GetGenericFile(), CFrontend::m_stringDest, NULL, NULL, bZero, bZero);
    
    sprintf(CFrontend::m_stringDest, CFrontend::m_strFrontendTexturesTSilverTGA, CInstallInfo::GetGameCDPath());
    CFrontend::m_pTSilver = CTexture::FindLoadTexture(CGenericFileLoader::GetGenericFile(), CFrontend::m_stringDest, NULL, NULL, bZero, bZero);
    
    sprintf(CFrontend::m_stringDest, CFrontend::m_strFrontendTexturesTBronzeTGA, CInstallInfo::GetGameCDPath());
    CFrontend::m_pTBronze = CTexture::FindLoadTexture(CGenericFileLoader::GetGenericFile(), CFrontend::m_stringDest, NULL, NULL, bZero, bZero);
    
    CFrontend::LoadFrontendCarPreviewTextures();
}

// FUNCTION: CMR2 0x004a9b00
BOOL CGame::IsActive(void)
{
    return m_isActive;
}

// FUNCTION: CMR2 0x004a9b10
void CGame::SetGameInputFocusState(int param1)
{
    m_unk0x00663dc4 = param1;
}

// FUNCTION: CMR2 0x004a9b20
int CGame::GetGameInputFocusState(void)
{
    return m_unk0x00663dc4;
}

// FUNCTION: CMR2 0x004aaaf0
// PORT: the network layer needs no object; a marker says it is in use.
bool CGame::CreateDirectPlay(void)
{
    static int s_network;

    m_pDirectPlay4A = &s_network;
    return true;
}

// FUNCTION: CMR2 0x004aab60
bool CGame::CreateDirectPlayLobby(void)
{
    static int s_lobby;

    m_pDirectPlayLobby3A = &s_lobby;
    return true;
}

// FUNCTION: CMR2 0x004aa8e0
// PORT: there is no IPX provider, which the original sorted first.
int __cdecl CGame::CompareConnections(const void *a, const void *b)
{
    return 0;
}

// FUNCTION: CMR2 0x004aa880
void CGame::ClearConnections(void)
{
    int i;

    for (i = 0; i < 10; i++) {
        sprintf(m_connections[i].name, CMain::m_logFileBlankLine);
        if (m_connections[i].pConnection != NULL) {
            CFileBuffer::FreeGenericFileBuffer(m_connections[i].pConnection);
            m_connections[i].pConnection = NULL;
        }
        m_connections[i].pConnection = NULL;
        memset(&m_connections[i].guidSP, 0, sizeof(m_connections[i].guidSP));
    }
    m_connectionCount = 0;
    m_maxConnections = 10;
}

// match 56%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004aa930
void CGame::AddConnection(char *name, void *pConnection, unsigned int size, NetGuid *pGuidSP)
{
    void *pCopy;

    if (m_connectionCount < 10) {
        strcpy(m_connections[m_connectionCount].name, name);
        m_connections[m_connectionCount].guidSP = *pGuidSP;
        pCopy = CFileBuffer::AllocateLockedBuffer(size);
        m_connections[m_connectionCount].pConnection = pCopy;
        if (pCopy != NULL) {
            memcpy(pCopy, pConnection, size);
            m_connectionCount++;
        }
    }
    qsort(m_connections, m_connectionCount, sizeof(DPlayConnection), CompareConnections);
}

// match 22%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)


// FUNCTION: CMR2 0x004aad00
DPlayConnection *CGame::GetConnection(BYTE index)
{
    if (index < m_connectionCount)
        return &m_connections[index];
    return NULL;
}

// FUNCTION: CMR2 0x004aad30
bool CGame::RejectUnsupportedNetworkOperation(BYTE param1, int param2, int param3)
{
    return false;
}

struct Menu;
void Race_SetFlag38108(void);
void Race_SetFlag37F94(void);

// Item callbacks of the menu built by 0x475f00.
// FUNCTION: CMR2 0x0049c070
void Game_NoOpInRaceMenuItemEvent(Menu *pMenu, int param)
{
    Race_SetFlag38108();
}

// FUNCTION: CMR2 0x0049c080
void Game_SetRaceExitFlags(Menu *pMenu, int param)
{
    Race_SetFlag38108();
    Race_SetFlag37F94();
}

Texture *InRaceMenu_GetUpArrowTexture(void);
Texture *InRaceMenu_GetDownArrowTexture(void);

// Draws the in-race menu built by 0x475f00: the title, then one row per item
// with its banner sprite, the item text and the separator line above the list
// and under every row. The selected row and the lines around it are drawn in
// the bright colour, the rest in the dim one.
// FUNCTION: CMR2 0x0049bcb0
void Game_DrawInRaceActionMenu(Menu *pMenu)
{
    BYTE colourWhite[4];
    BYTE colourText[4];
    BYTE colourDim[4];
    short line[4];
    short rect[4];
    MenuItem *pItem;
    int cursor;
    int x;
    int i;

    colourText[0] = 0x4f; colourText[1] = 0x4f; colourText[2] = 0x4f; colourText[3] = 0xff;
    colourDim[0] = 0x4f; colourDim[1] = 0x4f; colourDim[2] = 0x4f; colourDim[3] = 0xff;
    colourWhite[0] = 0xff; colourWhite[1] = 0xff; colourWhite[2] = 0xff; colourWhite[3] = 0xff;

    cursor = pMenu->cursor;
    Font_DrawText(0, CFrontend::GetTextString(0xf3), (int)(g_pGraphics->resX * 0xf0) / 0x280,
                  (int)(g_pGraphics->resY * 0xc8) / 0x1e0, (int *)colourText, 0x11);
    x = (int)(g_pGraphics->resX * 0xf0) / 0x280;
    rect[1] = 0;
    rect[0] = (short)x;
    if (InRaceMenu_GetUpArrowTexture() != 0) {
        rect[2] = InRaceMenu_GetUpArrowTexture()->width;
        rect[3] = InRaceMenu_GetUpArrowTexture()->height;
    }
    line[0] = (short)x;
    line[1] = (short)((int)(g_pGraphics->resY * 0xd7) / 0x1e0);
    line[2] = (short)((int)(g_pGraphics->resX * 0xa2) / 0x280);
    line[3] = 1;
    if (cursor == 0)
        Sprite_FillRect(&g_pGraphics->field309_0x150, line, colourWhite, 1);
    else
        Sprite_FillRect(&g_pGraphics->field309_0x150, line, colourDim, 1);
    pItem = pMenu->items;
    for (i = 0; i < 2; i++, pItem++) {
        if (InRaceMenu_GetUpArrowTexture() != 0)
            rect[1] = (short)(line[1] + (int)(g_pGraphics->resY * 0xe) / 0x1e0 -
                              InRaceMenu_GetUpArrowTexture()->height / 2);
        sprintf(CFrontend::m_stringDest, CRegKey::m_regKeyPathFormatValue,
                CFrontend::GetTextString(pItem->id));
        if (i == cursor) {
            Font_DrawText(0, CFrontend::m_stringDest,
                          (int)(g_pGraphics->resX * 0xf0) / 0x280 +
                              (int)(g_pGraphics->resX * 0x14) / 0x280,
                          line[1] + (int)(g_pGraphics->resY * 0x12) / 0x1e0, (int *)colourWhite, 0x11);
            if (InRaceMenu_GetDownArrowTexture() != 0)
                Sprite_Queue((SpriteRect *)&InRaceMenu_GetDownArrowTexture()->field_0x11c, (SpriteRect *)rect,
                             InRaceMenu_GetDownArrowTexture(), 1, 0, NULL, NULL, colourWhite, 8);
        } else {
            Font_DrawText(0, CFrontend::m_stringDest,
                          (int)(g_pGraphics->resX * 0xf0) / 0x280 +
                              (int)(g_pGraphics->resX * 0x14) / 0x280,
                          line[1] + (int)(g_pGraphics->resY * 0x12) / 0x1e0, (int *)colourText, 0x11);
            if (InRaceMenu_GetDownArrowTexture() != 0)
                Sprite_Queue((SpriteRect *)&InRaceMenu_GetDownArrowTexture()->field_0x11c, (SpriteRect *)rect,
                             InRaceMenu_GetDownArrowTexture(), 1, 0, NULL, NULL, colourText, 8);
        }
        line[1] = (short)((int)(g_pGraphics->resY * 0x18) / 0x1e0 * (i + 1) +
                          (int)(g_pGraphics->resY * 0xd7) / 0x1e0);
        if (i == cursor || i + 1 == cursor)
            Sprite_FillRect(&g_pGraphics->field309_0x150, line, colourWhite, 1);
        else
            Sprite_FillRect(&g_pGraphics->field309_0x150, line, colourDim, 1);
    }
}

// FUNCTION: CMR2 0x0049c090
int CGame::GetCallbackCount(void)
{
    return m_unk0x00593ba0;
}

// FUNCTION: CMR2 0x0049c0f0
void CGame::UnwindCallbacks(int count)
{
    while (m_unk0x00593ba0-- > count)
        ((void (*)(void))m_callbacks[m_unk0x00593ba0])();
    m_unk0x00593ba0 = count;
}

// FUNCTION: CMR2 0x0049c140
void CGame::SkipNextCallbackRenderPass(void)
{
    m_skipRenderCallbacks = 1;
}

// FUNCTION: CMR2 0x0049c400
int CGame::GetDrawnMeshTriangleCount(void)
{
    return m_unk0x0059ce18;
}

// FUNCTION: CMR2 0x0049c410
int CGame::GetDrawnOverlayTriangleCount(void)
{
    return m_unk0x0059ce20;
}

// FUNCTION: CMR2 0x0049c420
void CGame::SetObjectRenderMode(int param1)
{
    m_unk0x0059ce14 = param1;
}

// FUNCTION: CMR2 0x0049c430
int CGame::GetObjectRenderMode(void)
{
    return m_unk0x0059ce14;
}

// Sets field 0x2c of the mesh triangles whose flags match the group mask
// (bits 0..6 against flags 0..6, bits 7..13 against flags 9..15).
// FUNCTION: CMR2 0x0049c440
void Game_SetTriangleField2CByGroup(Mesh *pMesh, int mask, int value)
{
    MeshTriangle *pTri;
    int low = mask & 0x7f;
    int high = (mask >> 7) & 0x7f;
    int i;

    if (pMesh != NULL) {
        for (i = 0; i < pMesh->triangleCount; i++) {
            if ((pMesh->pTriangles[i].flags & low & 0x7f) != 0 ||
                (high & (pMesh->pTriangles[i].flags >> 9)) != 0)
                pMesh->pTriangles[i].field_0x2c = value;
        }
    }
}

// Same as Game_SetTriangleField2CByGroup for field 0x30 (the mesh must exist).
// FUNCTION: CMR2 0x0049c4b0
void Game_SetTriangleField30ByGroup(Mesh *pMesh, int mask, int value)
{
    int low = mask & 0x7f;
    int high = (mask >> 7) & 0x7f;
    int i;

    for (i = 0; i < pMesh->triangleCount; i++) {
        if ((pMesh->pTriangles[i].flags & low & 0x7f) != 0 ||
            (high & (pMesh->pTriangles[i].flags >> 9)) != 0)
            pMesh->pTriangles[i].field_0x30 = value;
    }
}

extern unsigned short g_unk0x0059be74[];

// Draws the triangles of a mesh in runs that share a texture, using the mesh
// slot already reserved in the shared vertex buffer, and clamps texture
// addressing for the material groups that need it.
// match 74%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0049c510
void Game_DrawMeshTextureRuns(Mesh *pMesh)
{
    int i;
    int texture;
    int prev = -1;
    int count = 0;
    int total = pMesh->triangleCount;
    MeshTriangle *pTri = pMesh->pTriangles;

    for (i = total; i > 0; i--) {
        texture = pTri->textures[pTri->field_0x2c];
        if (prev != texture) {
            if (count > 0) {
                Gfx_DrawIndexedPrimitiveVB(GFX_TRIANGLELIST, CGraphics::m_pTextureManager->pVertexBuffers[pMesh->vertexBufferIndex], pMesh->vertexOffset, pMesh->field_0x10, g_unk0x0059be74, count);
            }
            prev = texture;
            count = 0;
            CGraphics::ApplyTextureStageChange(0, CGraphics::m_pTextureManager->textureBuffer[texture]);
        }
        g_unk0x0059be74[count++] = pTri->vertexIndex[0];
        g_unk0x0059be74[count++] = pTri->vertexIndex[1];
        g_unk0x0059be74[count++] = pTri->vertexIndex[2];
        CGame::m_unk0x0059ce18++;
        if ((pTri->flags & 0x7f) == 0x70 || (pTri->flags & 0x7f) == 0x71 ||
            (pTri->flags & 0x7f) == 0x73 || (pTri->flags & 0x7f) == 0x74) {
            CGraphics::SetTextureAddressClamp(0);
        } else {
            CGraphics::SetTextureAddressClamp(1);
        }
        pTri++;
    }
    if (count != 0) {
        CGraphics::ApplyTextureStageChange(0, CGraphics::m_pTextureManager->textureBuffer[
            pMesh->pTriangles[total - 1].textures[pMesh->pTriangles[total - 1].field_0x2c]]);
        Gfx_DrawIndexedPrimitiveVB(GFX_TRIANGLELIST, CGraphics::m_pTextureManager->pVertexBuffers[pMesh->vertexBufferIndex], pMesh->vertexOffset, pMesh->field_0x10, g_unk0x0059be74, count);
    }
}

void Graphics_DrawMeshLOD(Mesh *pMesh, int useParts, int clampTexture, int markTextures);

// Draws every mesh node of a node list whose view mask contains the given bit,
// recursing into the children of the nodes that match.
// FUNCTION: CMR2 0x0049ca50
void Game_DrawViewMaskNodes(SceneNode *pNode, int bit)
{
    for (; pNode != NULL; pNode = pNode->pNext) {
        if (pNode->type == SCENE_NODE_MESH) {
            Mesh *pMesh = (Mesh *)pNode->pObject;
            int mask = 1 << bit;
            if ((mask & pNode->viewMask) != 0) {
                if (pMesh != NULL) {
                    Gfx_SetTransform(GFX_TRANSFORM_WORLD, (GfxMatrix *)pNode->worldF);
                    Graphics_DrawMeshLOD(pMesh, 0, 0, 0);
                }
                if ((pNode->viewMask & mask) != 0 && pNode->pFirstChild != NULL)
                    Game_DrawViewMaskNodes(pNode->pFirstChild, bit);
            }
        }
    }
}

// Draws a single node: if the node is a mesh whose view mask contains the
// given bit it binds the node world matrix and draws its mesh, and then
// recurses into the children that match the view mask as well.
// FUNCTION: CMR2 0x0049cad0
void Game_DrawViewMaskNode(SceneNode *pNode, int bit)
{
    if (pNode->type == SCENE_NODE_MESH) {
        Mesh *pMesh = (Mesh *)pNode->pObject;
        if ((pNode->viewMask & (1 << bit)) != 0) {
            if (pMesh != NULL) {
                Gfx_SetTransform(GFX_TRANSFORM_WORLD, (GfxMatrix *)pNode->worldF);
                Graphics_DrawMeshLOD(pMesh, 0, 0, 0);
            }
        }
    }
    if ((pNode->viewMask & (1 << bit)) != 0 && pNode->pFirstChild != NULL)
        Game_DrawViewMaskNodes(pNode->pFirstChild, bit);
}

// FUNCTION: CMR2 0x0049cb50
void CGame::QueuePrimaryDrawObject(void *param1)
{
    m_deferredDrawObjects[m_deferredDrawCount] = param1;
    m_deferredDrawCount++;
}

// FUNCTION: CMR2 0x0049cb70
void CGame::QueueSecondaryDrawObject(void *param1)
{
    m_transparentDrawNodes[m_transparentDrawCount] = param1;
    m_transparentDrawCount++;
}

// qsort comparator of the deferred draw list (0x49cc50): sorts by the
// depth at +0x114 of each entry's node, farthest first.
// FUNCTION: CMR2 0x0049cb90
int __cdecl Game_CompareDeferredDrawDepth(const void *a, const void *b)
{
    int depthA = (*(StageObject **)a)->pMesh->field_0x114;
    int depthB = (*(StageObject **)b)->pMesh->field_0x114;
    return depthA < depthB ? 1 : -1;
}

// Drawing view of a sector's stage objects (StageObject in Sector.h). It lives
// here instead of in the shared header because touching Sector.h perturbs the
// codegen of unrelated translation units.
struct StageObjectDraw {
    FixVector position;         // 0x0  world position (16.16)
    Mesh *pMesh;                // 0xc
    int field_0x10;
    BYTE field_0x14;            // 0x14 non-zero while the object is drawn
    BYTE field_0x15[3];
    int scaleX;                 // 0x18 scale of the first world matrix row (16.16)
    int field_0x1c[4];
    int scaleY;                 // 0x2c scale of the second world matrix row (16.16)
    int field_0x30[4];
    int scaleZ;                 // 0x40 scale of the third world matrix row (16.16)
    int field_0x44;
    int offsetX;                // 0x48 world matrix translation (16.16)
    int offsetY;                // 0x4c
    int offsetZ;                // 0x50
    int field_0x54;
    float matrix[16];           // 0x58 world matrix, built from the camera matrix
    StageObjectDraw *pNext;     // 0x98 next object of the sector
    int lightLevel;             // 0x9c light level when not lit per vertex
};
CMR2_LAYOUT_CHECK(StageObjectDraw_size, sizeof(StageObjectDraw) == 0xa0);

// Non-zero while the draw lists are depth sorted before being drawn
// (defined in Graphics.cpp).
extern int g_depthSortDrawLists;

void Frontend_SetObjectField118(struct Texture *pTexture, int value);

// PORT: the original sorted these objects, and chose to blend them within
// 200 m, by a distance it kept in their mesh, which the copies of a tree or
// a sign share: the distance of whichever copy was queued last. All the
// copies sorted as one, and their edges switched between blended and cut out
// together when that one copy crossed 200 m. Each object now sorts by its own
// distance, and all are blended, as they are drawn farthest first, so no edge
// changes at a distance.
struct DeferredDraw {
    int distance;
    StageObjectDraw *pObject;
};

static int __cdecl Game_CompareDeferredDistance(const void *a, const void *b)
{
    int distanceA = ((const DeferredDraw *)a)->distance;
    int distanceB = ((const DeferredDraw *)b)->distance;
    return distanceA < distanceB ? 1 : distanceA > distanceB ? -1 : 0;
}

extern SceneNode *g_unk0x0059be6c;

// Draws the objects the deferred pass of the static stage objects queued:
// sorts them by view depth (farthest first) and, for each one, binds its
// texture, sets its world matrix and marks the mesh for the culling test of
// the next frame before drawing it.
// FUNCTION: CMR2 0x0049cc50
void Game_DrawDeferredObjects(void)
{
    static DeferredDraw sorted[4096];
    StageObjectDraw *pObject;
    FixVector delta;
    unsigned int i;

    if ((unsigned int)CGame::m_deferredDrawCount >= 1) {
        for (i = 0; i < (unsigned int)CGame::m_deferredDrawCount; i++) {
            pObject = (StageObjectDraw *)CGame::m_deferredDrawObjects[i];
            delta.x = g_unk0x0059be6c->world.position.x - pObject->position.x;
            delta.y = 0;
            delta.z = g_unk0x0059be6c->world.position.z - pObject->position.z;
            sorted[i].distance = FixVecLength(&delta);
            sorted[i].pObject = pObject;
        }
        if (g_depthSortDrawLists != 0)
            qsort(sorted, CGame::m_deferredDrawCount, sizeof(sorted[0]), Game_CompareDeferredDistance);
        for (i = 0; i < (unsigned int)CGame::m_deferredDrawCount; i++) {
            pObject = sorted[i].pObject;
            Frontend_SetObjectField118(CGraphics::m_pTextureManager->textureBuffer[pObject->pMesh->pTriangles->textures[0]], 0);
            Gfx_SetTransform(GFX_TRANSFORM_WORLD, (GfxMatrix *)pObject->matrix);
            pObject->pMesh->flags |= 8;
            Graphics_DrawMeshLOD(pObject->pMesh, 1, 0, 0);
        }
        CGame::m_deferredDrawCount = 0;
    }
}

// qsort comparator of the transparent draw list (0x49cd20): type 0x14 goes
// last, type 5 sorts after type 0 at equal depth, else farthest first.
// match 50%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0049cbc0
int __cdecl Game_CompareTransparentDrawEntries(const void *a, const void *b)
{
    SceneNode *pA = *(SceneNode **)a;
    SceneNode *pB = *(SceneNode **)b;
    int depthA;
    int depthB;

    if ((pA->flags & 0xff) == 0x14)
        return 1;
    if ((pB->flags & 0xff) == 0x14)
        return 1;
    depthA = pA->cameraDistance;
    depthB = pB->cameraDistance;
    if (FIX_ABS(depthA - depthB) < 0x10000) {
        if ((pA->flags & 0xff) == 5 && (pB->flags & 0xff) == 0)
            return 1;
        if ((pA->flags & 0xff) == 0 && (pB->flags & 0xff) == 5)
            return -1;
    }
    return depthA < depthB ? 1 : -1;
}

extern unsigned short g_visibleSectorIndices[];
// GLOBAL: CMR2 0x0059be6c
SceneNode *g_unk0x0059be6c;
// GLOBAL: CMR2 0x00597cc0
GfxMatrix g_unk0x00597cc0;

void Graphics_DrawMeshLOD(Mesh *pMesh, int useParts, int clampTexture, int markTextures);

// Draws the static stage objects of the culled sectors. Objects whose mesh is
// flagged for the deferred pass (mesh flag 2) get their world matrix and view
// depth updated and are queued; the rest are drawn right away.
// FUNCTION: CMR2 0x0049d040
void Game_DrawStaticStageObjects(void)
{
    StageObjectDraw *pObject;
    unsigned int i;
    FixVector delta;

    for (i = 0; i < (unsigned int)g_sectorCullEnabled; i++) {
        pObject = (StageObjectDraw *)g_sectors[g_visibleSectorIndices[i]]->pObjects;
        while (pObject != NULL) {
            if (pObject->field_0x14 != 0) {
                if ((pObject->pMesh->flags & 2) != 0) {
                    *(GfxMatrix *)pObject->matrix = g_unk0x00597cc0;
                    if (pObject->scaleX != 0x10000 || pObject->scaleY != 0x10000 ||
                        pObject->scaleZ != 0x10000) {
                        float scale;
                        scale = X87S_MUL(X87S_FILD(float, pObject->scaleX), CGraphics::m_oneOver65536);
                        pObject->matrix[0] = scale * pObject->matrix[0];
                        pObject->matrix[1] = scale * pObject->matrix[1];
                        pObject->matrix[2] = scale * pObject->matrix[2];
                        scale = X87S_MUL(X87S_FILD(float, pObject->scaleY), CGraphics::m_oneOver65536);
                        pObject->matrix[4] = scale * pObject->matrix[4];
                        pObject->matrix[5] = scale * pObject->matrix[5];
                        pObject->matrix[6] = scale * pObject->matrix[6];
                        scale = X87S_MUL(X87S_FILD(float, pObject->scaleZ), CGraphics::m_oneOver65536);
                        pObject->matrix[8] = scale * pObject->matrix[8];
                        pObject->matrix[9] = scale * pObject->matrix[9];
                        pObject->matrix[10] = scale * pObject->matrix[10];
                    }
                    pObject->matrix[12] = X87S_MUL(X87S_FILD(float, pObject->offsetX), CGraphics::m_oneOver65536);
                    pObject->matrix[13] = X87S_MUL(X87S_FILD(float, pObject->offsetY), CGraphics::m_oneOver65536);
                    pObject->matrix[14] = X87S_MUL(X87S_FILD(float, pObject->offsetZ), CGraphics::m_oneOver65536);
                }
                if ((pObject->pMesh->flags & 2) != 0) {
                    delta.x = g_unk0x0059be6c->world.position.x - pObject->position.x;
                    delta.y = 0;
                    delta.z = g_unk0x0059be6c->world.position.z - pObject->position.z;
                    pObject->pMesh->field_0x114 = FixVecLength(&delta);
                    CGame::QueuePrimaryDrawObject(pObject);
                } else {
                    Graphics_DrawMeshLOD(pObject->pMesh, 1, 0, 0);
                }
            }
            pObject = pObject->pNext;
        }
    }
}

// Identity world transform: the deferred draw paths set it before drawing
// geometry whose vertices are already in world space.
// GLOBAL: CMR2 0x005207b8
GfxMatrix g_identityWorldMatrix = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
};

void Graphics_DrawMeshLOD(Mesh *pMesh, int useParts, int clampTexture, int markTextures);
void Quad2D_DrawLayer(unsigned int layer);
void Pulse_Update(unsigned int dt);
extern Mesh **g_sceneShadowMeshes;

// Draws the queue of scene nodes collected by 0x0049d290: depth sorts it
// (farthest first) when depth sorting is enabled, draws every node of the
// queue with the given view mask bit and empties it.
// FUNCTION: CMR2 0x0049cd20
void Game_DrawSortedNodes(int bit)
{
    unsigned int i;

    if ((unsigned int)CGame::m_transparentDrawCount >= 1) {
        if (g_depthSortDrawLists != 0)
            qsort(CGame::m_transparentDrawNodes, CGame::m_transparentDrawCount, sizeof(CGame::m_transparentDrawNodes[0]), Game_CompareTransparentDrawEntries);
        for (i = 0; i < (unsigned int)CGame::m_transparentDrawCount; i++)
            Game_DrawViewMaskNode((SceneNode *)CGame::m_transparentDrawNodes[i], bit);
        CGame::m_transparentDrawCount = 0;
    }
}

// Draws every mesh node of the scene in world space with Z test and write
// enabled, then the 2D layer 0x10.
// FUNCTION: CMR2 0x0049cd90
void Game_DrawWorldMeshNodesAnd2DLayer(void)
{
    SceneNode *pNode;
    Mesh *pMesh;
    unsigned int i;

    CGraphics::SetZEnable(1);
    CGraphics::SetZWriteEnable(1);
    for (i = 0; i < (unsigned int)g_sceneNodeCount; i++) {
        pNode = g_sceneNodes[i];
        if (pNode != NULL && pNode->type == SCENE_NODE_MESH) {
            pMesh = (Mesh *)pNode->pObject;
            if (pNode->viewMask != 0 && pMesh != NULL) {
                Gfx_SetTransform(GFX_TRANSFORM_WORLD, (GfxMatrix *)pNode->worldF);
                Graphics_DrawMeshLOD(pMesh, 0, 0, 0);
            }
        }
    }
    Quad2D_DrawLayer(0x10);
}

// Draws the view-mask scene nodes of the culled sectors that were not queued
// for depth sorting (visible == 0), with Z test and Z write disabled, then
// restores them.
// FUNCTION: CMR2 0x0049ce40
void Game_DrawUnsortedNodes(int bit)
{
    SceneNode *pNode;
    unsigned int i;

    CGraphics::SetZEnable(0);
    CGraphics::SetZWriteEnable(0);
    for (i = 0; i < (unsigned int)g_sectorCullEnabled; i++) {
        pNode = g_sectors[g_visibleSectorIndices[i]]->pFirstNode;
        while (pNode != NULL) {
            if (pNode->visible == 0)
                Game_DrawViewMaskNode(pNode, bit);
            pNode = pNode->pNextInSector;
        }
    }
    CGraphics::SetZEnable(1);
    CGraphics::SetZWriteEnable(1);
}

// Advances the pulse effect by the frame delta and draws the ground mesh of
// every culled sector in world space.
// The two render passes have independent lazy frame stamps.
// GLOBAL: CMR2 0x00597d00
BYTE g_sectorFrameInit;
// GLOBAL: CMR2 0x00597d01
BYTE g_shadowFrameInit;
// GLOBAL: CMR2 0x0059bd68
unsigned int g_sectorFrameStart;
// GLOBAL: CMR2 0x00597cb0
unsigned int g_sectorFramePrevious;
// GLOBAL: CMR2 0x00597cb4
unsigned int g_shadowFrameStart;
// GLOBAL: CMR2 0x0059be70
unsigned int g_shadowFramePrevious;

// FUNCTION: CMR2 0x0049cec0
void Game_DrawCulledSectorMeshes(void)
{
    Mesh *pMesh;
    unsigned int i;

    if ((g_sectorFrameInit & 1) == 0) {
        g_sectorFrameInit |= 1;
        g_sectorFrameStart = CMain::GetFrameDelta();
    }
    if ((g_sectorFrameInit & 2) == 0) {
        g_sectorFrameInit |= 2;
        g_sectorFramePrevious = CMain::GetFrameDelta();
    }
    Pulse_Update(CMain::GetFrameDelta() - g_sectorFramePrevious);
    g_sectorFramePrevious = CMain::GetFrameDelta();
    Gfx_SetTransform(GFX_TRANSFORM_WORLD, &g_identityWorldMatrix);
    for (i = 0; i < (unsigned int)g_sectorCullEnabled; i++) {
        pMesh = (Mesh *)g_sectors[g_visibleSectorIndices[i]]->pMesh;
        if (pMesh != NULL) {
            if ((pMesh->flags & 0x1000) != 0)
                Graphics_DrawMeshLOD(pMesh, 0, 1, 0);
            else
                Graphics_DrawMeshLOD(pMesh, 1, 0, 0);
        }
    }
}

// Advances the pulse effect by the frame delta and draws the shadow mesh of
// every culled sector in world space, with Z writes disabled.
// FUNCTION: CMR2 0x0049cf80
void Game_DrawCulledSectorShadows(void)
{
    Mesh *pMesh;
    unsigned int i;

    if ((g_shadowFrameInit & 1) == 0) {
        g_shadowFrameInit |= 1;
        g_shadowFrameStart = CMain::GetFrameDelta();
    }
    if ((g_shadowFrameInit & 2) == 0) {
        g_shadowFrameInit |= 2;
        g_shadowFramePrevious = CMain::GetFrameDelta();
    }
    Pulse_Update(CMain::GetFrameDelta() - g_shadowFramePrevious);
    g_shadowFramePrevious = CMain::GetFrameDelta();
    Gfx_SetTransform(GFX_TRANSFORM_WORLD, &g_identityWorldMatrix);
    if (g_sceneShadowMeshes != NULL) {
        CGraphics::SetZWriteEnable(0);
        for (i = 0; i < (unsigned int)g_sectorCullEnabled; i++) {
            pMesh = g_sceneShadowMeshes[g_visibleSectorIndices[i]];
            if (pMesh != NULL)
                Graphics_DrawMeshLOD(pMesh, 1, 0, 1);
        }
        CGraphics::SetZWriteEnable(1);
    }
}

// Computes the view depth of every visible node of the culled sectors and
// queues them for drawing.
// FUNCTION: CMR2 0x0049d290
void Game_QueueVisibleSectorNodes(int param1)
{
    SceneNode *pNode;
    FixVector delta;
    unsigned short *pIndex;
    unsigned int i;

    i = 0;
    if ((unsigned int)g_sectorCullEnabled > 0) {
        pIndex = (unsigned short *)g_visibleSectorIndices;
        do {
            pNode = g_sectors[*pIndex]->pFirstNode;
            while (pNode != NULL) {
                if (pNode->visible != 0) {
                    delta.x = g_unk0x0059be6c->world.position.x - pNode->world.position.x;
                    delta.y = 0;
                    delta.z = g_unk0x0059be6c->world.position.z - pNode->world.position.z;
                    pNode->cameraDistance = FixVecLength(&delta);
                    CGame::QueueSecondaryDrawObject(pNode);
                }
                pNode = pNode->pNextInSector;
            }
            i++;
            pIndex++;
        } while (i < (unsigned int)g_sectorCullEnabled);
    }
}

// Draws one frame of the race view: sets the viewport and the render states of
// the 3D pass and of the 2D layers, draws the world (relit sectors, static and
// deferred objects, ground and shadow meshes, view masked nodes) with the given
// view bit, then restores the full screen viewport and flushes the remaining 2D
// layers. Returns 1 (also when the device refuses to start a scene).
void Graphics_SetLightingMode(int mode);
void Graphics_EnableFog(void);
void Graphics_DisableFog(void);
void Graphics_SetRenderTarget(Texture *pTexture);
void ScreenLine2D_Draw(int layer);
void Tri2D_DrawLayer(int layer);
void Line2D_Draw(void);
void Quad2D_DrawLayer(unsigned int layer);
void Billboard_Draw(SceneNode *pCamera);
void Particle_DrawAll(SceneNode *pCamera, BYTE view);
void Scene_DrawShadowBatches(unsigned int view);
void Scene_RelightSector(int sector);
BYTE Flare_SampleVisibility(short *pRect, BYTE *pColour, BYTE tolerance);
void Sector_CullGridAroundViewNode(SceneNode *pNode, void *pRect);
void Graphics_UpdateFrameStatistics(void);
int Graphics_GetFlareStateValue(void);
void Graphics_SwitchAlphaBlendAndTest(int enable);
void Game_DrawStaticStageObjects(void);
void Game_DrawWorldMeshNodesAnd2DLayer(void);
void Game_DrawCulledSectorMeshes(void);
void Game_DrawCulledSectorShadows(void);
void Game_DrawUnsortedNodes(int bit);
void Game_DrawSortedNodes(int bit);
void Game_QueueVisibleSectorNodes(int param1);
void Game_DrawDeferredObjects(void);
extern FixMatrix g_cubeFaceViewMatrix;
GfxMatrix *FixMatrix_ToFloat(GfxMatrix *pOut, FixMatrix *pIn);

// PORT: the original cut the scenery at the far plane, so whatever entered
// the view distance appeared at once on the horizon. Over the last 15% of the
// view distance the scenery now dissolves into the sky and backdrop already
// drawn behind it. The view distance stays the stage's: past it the stages
// are not built to be seen (hill tops with nothing under them, missing
// triangles).
static void Game_BeginDistanceFade(void)
{
    if (!FarLayer_StageFades())
        return;
    int end = CGraphics::m_farPlaneFixed;
    int start = FixMul(end, 0xd99a);

    Gfx_CaptureBackdrop();
    Gfx_SetDistanceFade(X87S_MUL(X87S_FILD(float, start), CGraphics::m_oneOver65536),
                        X87S_MUL(X87S_FILD(float, end), CGraphics::m_oneOver65536));
}

// match 94%: the residual diff is register allocation (the relight loop walks
// the sector list from a register where the original keeps the pointer in the
// argument slot, and the inlined fixed point normalisation uses different
// scratch slots), plus the 0.0f store which the original materialises as an
// immediate.
// FUNCTION: CMR2 0x0049d3f0
int Game_DrawSceneViewport(SceneNode *pRoot, SceneNode *pCamera, void *pRect, int bit, BYTE flag)
{
    GfxViewport viewport;
    FixVector cameraPosition;
    FixVector forward;
    int farPlane;
    unsigned int i;
    unsigned short *pIndex;

    g_unk0x0059be6c = pCamera;
    CGame::m_unk0x0059ce18 = 0;
    CGame::m_unk0x0059ce1c = 0;
    CGame::m_unk0x0059ce20 = 0;
    // PORT: the renderer has no lost device to fail BeginScene.
    Gfx_BeginScene();
    memset(&viewport, 0, sizeof(viewport));
    viewport.x = ((short *)pRect)[0];
    viewport.y = ((short *)pRect)[1];
    viewport.width = ((short *)pRect)[2];
    viewport.height = ((short *)pRect)[3];
    viewport.minZ = 0.0f;
    viewport.maxZ = 1.0f;
    Gfx_SetViewport(&viewport);
    Graphics_SwitchAlphaBlendAndTest(1);
    CGraphics::SetCullMode(3);
    Graphics_SetLightingMode(5);
    // PORT: the background layer is drawn before the scene and with the
    // depth state the previous drawing left, which is off at the end of a
    // frame but on after the car reflections (Graphics_RenderNodeCubeMapFaces)
    // were rendered for this one. Its 2D triangles have depth 0, so the
    // full-screen fill of the in-race display options page hid the whole
    // stage but the sky (#26). It is drawn without depth, as on every other
    // frame.
    CGraphics::SetZWriteEnable(0);
    CGraphics::SetZEnable(0);
    ScreenLine2D_Draw(4);
    Tri2D_DrawLayer(4);
    Sprite_DrawLayer(4);
    if (flag != 0) {
        if (CGraphics::m_unk0x0072d56c != 0) {
            Gfx_SetTransform(GFX_TRANSFORM_WORLD, &g_identityWorldMatrix);
            Sector_CullGridAroundViewNode(pCamera, pRect);
            for (i = 0; i < (unsigned int)g_sectorCullEnabled; i++)
                Scene_RelightSector(((unsigned short *)g_visibleSectorIndices)[i]);
        }
        CGraphics::SetCullMode(CGame::GetSectorDrawState());
        FixMatrix_GetPosition(&cameraPosition, &pCamera->world);
        Particle_DrawAll(pCamera, (BYTE)bit);
        forward.x = g_unk0x0059be6c->world.forward.x;
        forward.y = 0;
        forward.z = g_unk0x0059be6c->world.forward.z;
        FIX_NORMALIZE_INTO(forward, forward);
        g_cubeFaceViewMatrix.right.x = forward.z;
        g_cubeFaceViewMatrix.right.y = 0;
        g_cubeFaceViewMatrix.right.z = -forward.x;
        g_cubeFaceViewMatrix.rw = 0;
        g_cubeFaceViewMatrix.up.x = 0;
        g_cubeFaceViewMatrix.up.y = 0x10000;
        g_cubeFaceViewMatrix.up.z = 0;
        g_cubeFaceViewMatrix.uw = 0;
        g_cubeFaceViewMatrix.forward.x = forward.x;
        g_cubeFaceViewMatrix.forward.y = 0;
        g_cubeFaceViewMatrix.forward.z = forward.z;
        g_cubeFaceViewMatrix.fw = 0;
        g_cubeFaceViewMatrix.position.x = 0;
        g_cubeFaceViewMatrix.position.y = 0;
        g_cubeFaceViewMatrix.position.z = 0;
        g_cubeFaceViewMatrix.pw = 0x10000;
        FixMatrix_ToFloat(&g_unk0x00597cc0, &g_cubeFaceViewMatrix);
        if ((unsigned int)g_sectorCount > 0) {
            Gfx_SetFog(Graphics_GetFlareStateValue());
            Graphics_DisableFog();
            Graphics_SetLightingMode(5);
            farPlane = CGraphics::m_farPlaneFixed;
            CGraphics::SetProjection(0, 0, 0x960000, 0);
            Game_DrawUnsortedNodes(bit);
            CGraphics::SetProjection(0, 0, farPlane, 0);
            // PORT: the stage beyond its view distance at low detail
            // (FarLayer.cpp), and the scenery fades into it and the backdrop
            // (Game_BeginDistanceFade).
            FarLayer_Draw(pCamera);
            Game_BeginDistanceFade();
            Graphics_EnableFog();
            Graphics_SetLightingMode(0);
            Game_DrawCulledSectorMeshes();
            Graphics_SetLightingMode(5);
            Game_DrawCulledSectorShadows();
            Graphics_SwitchAlphaBlendAndTest(1);
            Glow_Draw(pCamera, (BYTE)bit);
            Quad2D_DrawLayer(8);
            Graphics_SetLightingMode(4);
            Quad2D_DrawLayer(0x20);
            Graphics_SetLightingMode(1);
            if (CGame::m_unk0x0059ce14 != 0) {
                Game_DrawStaticStageObjects();
                Game_DrawDeferredObjects();
                Gfx_SetTransform(GFX_TRANSFORM_WORLD, &g_identityWorldMatrix);
                Graphics_SwitchAlphaBlendAndTest(1);
                CGraphics::SetCachedSourceDestinationBlend(5, 6);
                Gfx_SetFog(0);
                Graphics_SetLightingMode(5);
                Quad2D_DrawLayer(0x10);
                Line2D_Draw();
                Billboard_Draw(pCamera);
                Graphics_SetLightingMode(1);
                Game_QueueVisibleSectorNodes(bit);
                Game_DrawSortedNodes(bit);
            } else {
                Game_DrawStaticStageObjects();
                Game_QueueVisibleSectorNodes(bit);
                Game_DrawSortedNodes(bit);
                Game_DrawDeferredObjects();
            }
            Gfx_SetDistanceFade(0.0f, 0.0f);
        } else {
            Game_DrawWorldMeshNodesAnd2DLayer();
        }
        Gfx_SetTransform(GFX_TRANSFORM_WORLD, &g_identityWorldMatrix);
        Graphics_SwitchAlphaBlendAndTest(1);
        CGraphics::SetCachedSourceDestinationBlend(5, 6);
        Gfx_SetFog(0);
        Graphics_SetLightingMode(5);
        Quad2D_DrawLayer(0x10);
        Graphics_SetLightingMode(4);
        Quad2D_DrawLayer(0x40);
        Graphics_SetLightingMode(5);
        Line2D_Draw();
        Billboard_Draw(pCamera);
        Scene_DrawShadowBatches(bit);
    }
    Flare_SampleVisibility(NULL, NULL, 0);
    CGraphics::SetZWriteEnable(0);
    CGraphics::SetZEnable(0);
    CGraphics::SetCachedSourceDestinationBlend(5, 6);
    memset(&viewport, 0, sizeof(viewport));
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = g_pGraphics->resX;
    viewport.height = g_pGraphics->resY;
    viewport.minZ = 0.0f;
    viewport.maxZ = 1.0f;
    Gfx_SetViewport(&viewport);
    CGraphics::SetCullMode(3);
    ScreenLine2D_Draw(3);
    Tri2D_DrawLayer(3);
    Sprite_DrawLayer(3);
    ScreenLine2D_Draw(2);
    Tri2D_DrawLayer(2);
    Sprite_DrawLayer(2);
    ScreenLine2D_Draw(1);
    Tri2D_DrawLayer(1);
    Sprite_DrawLayer(1);
    CGraphics::SetCullMode(CGame::GetSectorDrawState());
    Graphics_UpdateFrameStatistics();
    Gfx_EndScene();
    return 1;
}

// FUNCTION: CMR2 0x0049dca0
void CGame::SetSectorDrawState(int param1)
{
    m_unk0x005207f8 = param1;
}

// FUNCTION: CMR2 0x0049dcb0
int CGame::GetSectorDrawState(void)
{
    return m_unk0x005207f8;
}

// GLOBAL: CMR2 0x00537f5c
int g_unk0x00537f5c;
// GLOBAL: CMR2 0x00665324
int g_networkResourceRegistered;

// FUNCTION: CMR2 0x0041f260
void CGame::SaveRaceCallbackDepth(void)
{
    g_unk0x00537f5c = GetCallbackCount();
}

// FUNCTION: CMR2 0x004aae10
int Network_ClearResourceRegistrationFlag(void)
{
    g_networkResourceRegistered = 0;
    return 1;
}

// FUNCTION: CMR2 0x004aad50
void CGame::RegisterNetworkResourceRelease(void)
{
    RegisterCallback(Network_ClearResourceRegistrationFlag, NULL);
    g_networkResourceRegistered = 1;
}


// FUNCTION: CMR2 0x004764c0
void InteriorCamera_EndLeave(CameraRecord *pRecord)
{
    if (g_carInteriorRoots[pRecord->car] != NULL)
        CGame::SetObjectRenderMode(0);
}

// FUNCTION: CMR2 0x00476500
void InteriorCamera_EndEnter(CameraRecord *pRecord)
{
    if (g_carInteriorRoots[pRecord->car] != NULL)
        CGame::SetObjectRenderMode(1);
}

void CarCamera_BeginLeave(CameraRecord *pRecord, CameraRecord *pOther);
void TracksideCamera_BeginLeave(CameraRecord *pRecord, CameraRecord *pOther);

// Dispatches by the camera type.
// FUNCTION: CMR2 0x00423810
void Camera_BeginLeave(CameraRecord *pRecord, CameraRecord *pOther)
{
    switch (pRecord->type) {
    case 1:
    case 2:
    case 10:
        CarCamera_BeginLeave(pRecord, pOther);
        return;
    case 7:
        TracksideCamera_BeginLeave(pRecord, pOther);
    }
}

void CarCamera_EndEnter(CameraRecord *pRecord, CameraRecord *pOther);
void CarCamera_SetFullCarLift(CameraRecord *pRecord, CameraRecord *pOther);
void InteriorCamera_EndEnter(CameraRecord *pRecord);
void TracksideCamera_EndEnter(CameraRecord *pRecord, CameraRecord *pOther);

// Dispatches by the camera type.
// match 64%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00423900
void Camera_EndEnter(CameraRecord *pRecord, CameraRecord *pOther)
{
    switch (pRecord->type) {
    case 3:
        InteriorCamera_EndEnter(pRecord);
        return;
    case 2:
        CarCamera_SetFullCarLift(pRecord, pOther);
        return;
    case 1:
    case 10:
        CarCamera_EndEnter(pRecord, pOther);
        return;
    case 7:
        TracksideCamera_EndEnter(pRecord, pOther);
    }
}

void CarCamera_BeginEnter(CameraRecord *pRecord, CameraRecord *pOther);
void CarInterior_ResetDriverPoseOnNextUpdate(CameraRecord *pRecord);
FixMatrix *Car_GetCameraReferenceMatrix(BYTE index);
void ChaseCamera_BeginEnter(CameraRecord *pRecord, CameraRecord *pOther, FixMatrix *pRef);
void TracksideCamera_BeginEnter(CameraRecord *pRecord, CameraRecord *pOther);

// Dispatches by the camera type.
// match 60%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00423860
void Camera_BeginEnter(CameraRecord *pRecord, CameraRecord *pOther)
{
    switch (pRecord->type) {
    case 4:
    case 5:
        ChaseCamera_BeginEnter(pRecord, pOther, Car_GetCameraReferenceMatrix(pRecord->car));
        return;
    case 3:
        CarInterior_ResetDriverPoseOnNextUpdate(pRecord);
        return;
    case 1:
    case 2:
    case 10:
        CarCamera_BeginEnter(pRecord, pOther);
        return;
    case 7:
        TracksideCamera_BeginEnter(pRecord, pOther);
    }
}

// FUNCTION: CMR2 0x004238e0
void Camera_EndLeave(CameraRecord *pRecord, CameraRecord *pOther)
{
    if (pRecord->type == 3)
        InteriorCamera_EndLeave(pRecord);
}


// GLOBAL: CMR2 0x005a0068
NetSessionDesc g_networkSession;

// The current session description and the list of enumerated sessions
#define SESSION g_networkSession
#define SESSIONS CGameInfo::m_enumeratedSessions

// Reads the description of the joined session into SESSION (the session
// name is copied to m_unk0x005a00b8).
// match 86%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004a0d60
BOOL Network_ReadJoinedSessionDescription(void)
{
    NetSessionDesc desc;

    if (CGame::GetDirectPlay() == NULL)
        return FALSE;
    if (Net_GetSessionDesc(&desc) != NET_OK)
        return FALSE;
    SESSION.dwSize = desc.dwSize;
    SESSION.dwFlags = desc.dwFlags;
    SESSION.guidInstance = desc.guidInstance;
    SESSION.guidApplication = desc.guidApplication;
    SESSION.dwMaxPlayers = desc.dwMaxPlayers;
    SESSION.dwCurrentPlayers = desc.dwCurrentPlayers;
    strcpy((char *)&CGameInfo::m_unk0x005a00b8, desc.lpszSessionNameA);
    SESSION.dwReserved1 = desc.dwReserved1;
    SESSION.dwReserved2 = desc.dwReserved2;
    SESSION.dwUser1 = desc.dwUser1;
    SESSION.dwUser2 = desc.dwUser2;
    SESSION.dwUser3 = desc.dwUser3;
    SESSION.dwUser4 = desc.dwUser4;
    return TRUE;
}

// The GUID of the CMR2 DirectPlay application (shared with GameInfo.cpp).
extern int g_unk0x00511cd8[4];

// Creates (hosts) a DirectPlay session with the given name, password and user
// values. Returns 1 when the session was created.
// match 87%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004a0ec0
int Network_HostNamedSession(char *pSessionName, char *pPassword, DWORD user1, DWORD user2,
                 DWORD user3, DWORD user4, DWORD maxPlayers)
{
    HRESULT hr;

    memset(&g_networkSession, 0, sizeof(g_networkSession));
    SESSION.dwSize = sizeof(NetSessionDesc);
    SESSION.dwFlags = 0x2064;
    memcpy(&SESSION.guidApplication, g_unk0x00511cd8, sizeof(NetGuid));
    SESSION.dwMaxPlayers = maxPlayers;
    strcpy((char *)&CGameInfo::m_unk0x005a00b8, pSessionName);
    SESSION.lpszSessionNameA = (LPSTR)&CGameInfo::m_unk0x005a00b8;
    strcpy((char *)&CGameInfo::m_unk0x005a02c0, pPassword);
    SESSION.lpszPasswordA = (LPSTR)&CGameInfo::m_unk0x005a02c0;
    SESSION.dwUser1 = user1;
    SESSION.dwUser2 = user2;
    SESSION.dwUser3 = user3;
    SESSION.dwUser4 = user4;
    if (CGame::GetDirectPlay() == NULL)
        return 0;
    hr = Net_Open(&SESSION, NET_OPEN_CREATE);
    switch (hr) {
    case NET_ERR_ALREADYINITIALIZED: return 0;
    case NET_ERR_ACCESSDENIED: return 0;
    case NET_ERR_INVALIDFLAGS: return 0;
    case NET_ERR_NOCONNECTION: return 0;
    case NET_ERR_TIMEOUT: return 0;
    case NET_ERR_USERCANCEL: return 0;
    case NET_ERR_UNINITIALIZED: return 0;
    case NET_ERR_NONEWPLAYERS: return 0;
    case NET_ERR_INVALIDPASSWORD: return 0;
    case NET_OK:
        CGameInfo::m_unk0x005a0060 = 1;
        CGameInfo::m_unk0x005a1814 = 1;
        return 1;
    }
    return 0;
}

// Opens (joins) the enumerated session at index into SESSION using the given
// password and pushes it to DirectPlay. Returns 1 when the join is still in
// progress, and sets *pInvalidPassword when DirectPlay rejects the password.
// FUNCTION: CMR2 0x004a10b0
int Network_JoinEnumeratedSession(BYTE index, char *pPassword, BYTE *pInvalidPassword)
{
    HRESULT hr;

    *pInvalidPassword = 0;
    if (index < CGameInfo::m_unk0x005a01bc) {
        memset(&g_networkSession, 0, sizeof(g_networkSession));
        SESSION.dwSize = sizeof(NetSessionDesc);
        SESSION.guidInstance = SESSIONS[index].guidInstance;
        SESSION.lpszSessionNameA = (LPSTR)&CGameInfo::m_unk0x005a00b8;
        SESSION.lpszPasswordA = pPassword;
        if (CGame::GetDirectPlay() != NULL) {
            hr = Net_Open(&SESSION, NET_OPEN_JOIN);
            switch (hr) {
            case NET_ERR_INVALIDPASSWORD:
                *pInvalidPassword = 1;
                return 0;
            case NET_ERR_ALREADYINITIALIZED:
                return 0;
            case NET_ERR_ACCESSDENIED:
                return 0;
            case NET_ERR_INVALIDFLAGS:
                return 0;
            case NET_ERR_NOCONNECTION:
                return 0;
            case NET_ERR_TIMEOUT:
                return 0;
            case NET_ERR_USERCANCEL:
                return 0;
            case NET_ERR_UNINITIALIZED:
                return 0;
            case NET_ERR_NONEWPLAYERS:
                return 0;
            default:
                return 1;
            case NET_OK:
                CGameInfo::m_unk0x005a1814 = TRUE;
                Network_ReadJoinedSessionDescription();
                return 1;
            }
        }
    }
    return 0;
}

// Closes the DirectPlay session object.
// FUNCTION: CMR2 0x004a1280
int Network_CloseSession(void)
{
    HRESULT hr;

    if (CGame::GetDirectPlay() == NULL)
        return 0;
    hr = Net_Close();
    if (hr != 0)
        return 0;
    CGameInfo::m_unk0x005a1814 = hr;
    return 1;
}

// FUNCTION: CMR2 0x004a14e0
// PORT: pushes SESSION to the other machines (SetSessionDesc).
bool Network_TestLocalPlayerStatus(void)
{
    if (CGame::GetDirectPlay() == NULL)
        return false;
    return Net_SetSessionDesc(&SESSION) == NET_OK;
}

// FUNCTION: CMR2 0x004a1480
unsigned int Network_GetEnumeratedSessionCount(void)
{
    return *(unsigned int *)&CGameInfo::m_unk0x005a01bc & 0xff;
}

// FUNCTION: CMR2 0x004a1490
char *Network_GetEnumeratedSessionName(BYTE index)
{
    if (index < CGameInfo::m_unk0x005a01bc)
        return SESSIONS[index].lpszSessionNameA;
    return NULL;
}

// FUNCTION: CMR2 0x004a14c0
LPVOID *Network_GetSessionNamePointer(void)
{
    return (LPVOID *)g_networkSession.lpszSessionNameA;
}

// FUNCTION: CMR2 0x004a14d0
LPVOID *Network_GetSessionPasswordPointer(void)
{
    return (LPVOID *)g_networkSession.lpszPasswordA;
}

// FUNCTION: CMR2 0x004a15b0
void Network_SetSessionStateFlag(BOOL param1)
{
    CGameInfo::m_unk0x005a0060 = param1;
}

// FUNCTION: CMR2 0x004a15c0
int Network_CopyEnumeratedSessionGUID(BYTE index, NetGuid *pOut)
{
    if (index < CGameInfo::m_unk0x005a01bc) {
        *pOut = SESSIONS[index].guidInstance;
        return 1;
    }
    return 0;
}

// Sets one of the four session user values and pushes the description.
// FUNCTION: CMR2 0x004a16c0
void Session_SetUserValue(BYTE index, int value)
{
    Network_ReadJoinedSessionDescription();
    switch (index) {
    case 0:
        g_networkSession.dwUser1 = value;
        Network_TestLocalPlayerStatus();
        return;
    case 1:
        g_networkSession.dwUser2 = value;
        Network_TestLocalPlayerStatus();
        return;
    case 2:
        g_networkSession.dwUser3 = value;
        Network_TestLocalPlayerStatus();
        return;
    }
    g_networkSession.dwUser4 = value;
    Network_TestLocalPlayerStatus();
}

// FUNCTION: CMR2 0x004a1720
DWORD Network_GetSessionPlayerCount(int index)
{
    if (index < 0)
        return SESSION.dwCurrentPlayers;
    return SESSIONS[index].dwCurrentPlayers;
}

// FUNCTION: CMR2 0x004a1740
DWORD Network_GetSessionMaxPlayers(BYTE index)
{
    return SESSIONS[index].dwMaxPlayers;
}

// FUNCTION: CMR2 0x004a1760
void Network_SetSessionDescription(NetSessionDesc *pDesc)
{
    SESSION = *pDesc;
    g_networkSession.lpszSessionNameA = (LPSTR)&CGameInfo::m_unk0x005a00b8;
    g_networkSession.lpszPasswordA = (LPSTR)&CGameInfo::m_unk0x005a02c0;
}

// FUNCTION: CMR2 0x004a1790
BYTE Network_IsSessionFlag10Set(BYTE index)
{
    BYTE r = (BYTE)(SESSIONS[index].dwFlags >> 10);
    r &= 1;
    return r;
}

// Adds a remote player to the session player table (at most 7 players,
// ignoring the local player and players already listed).
// FUNCTION: CMR2 0x004a1850
void Network_AddRemoteSessionPlayer(char *shortName, char *longName, NetPlayerID dpId)
{
    SessionPlayerRecord *pPlayer;
    int i;

    if (CGame::m_localPlayerId == dpId)
        return;
    for (i = 0; i < 7; i++) {
        if (CGame::m_sessionPlayers[i].playerId == dpId)
            return;
    }
    if (CGame::m_sessionPlayerCount >= 7)
        return;
    for (i = 0; i < 7; i++) {
        if (CGame::m_sessionPlayers[i].active == 0) {
            if (shortName != NULL)
                strcpy(CGame::m_sessionPlayers[i].shortName, shortName);
            if (longName != NULL)
                strcpy(CGame::m_sessionPlayers[i].longName, longName);
            CGame::m_sessionPlayers[i].playerId = dpId;
            CGame::m_sessionPlayers[i].active = 1;
            CGame::m_sessionPlayerCount++;
            return;
        }
    }
}

// IDirectPlay4::EnumPlayers callback of Network_RebuildSessionPlayerList.
// FUNCTION: CMR2 0x004a1ad0
BOOL Network_EnumeratePlayerCallback(NetPlayerID dpId, const NetName *lpName, void *lpContext)
{
    Network_AddRemoteSessionPlayer(lpName->lpszShortNameA, lpName->lpszLongNameA, dpId);
    return 1;
}

// FUNCTION: CMR2 0x004a1af0
int Network_RebuildSessionPlayerList(void)
{
    CGame::ResetSessionPlayerTable(0);
    if (CGame::GetDirectPlay() == NULL)
        return 0;
    return Net_EnumPlayers(Network_EnumeratePlayerCallback, NULL) == NET_OK;
}

// EnumConnections callback: keeps every service provider connection.
// FUNCTION: CMR2 0x004aabd0
// PORT: one connection per network provider; its data is the provider index.
BOOL Network_EnumerateConnectionCallback(int provider, const char *name, void *lpContext)
{
    NetGuid guid;

    memset(&guid, 0, sizeof(guid));
    guid.bytes[0] = (BYTE)(provider + 1);
    CGame::AddConnection((char *)name, &provider, sizeof(provider), &guid);
    return TRUE;
}

// FUNCTION: CMR2 0x004aac00
bool Network_EnumerateServiceProviders(void)
{
    CGame::ClearConnections();
    return Net_EnumProviders(Network_EnumerateConnectionCallback, NULL) == NET_OK;
}


// Sets the local player data (guaranteed).
// FUNCTION: CMR2 0x004a1cb0
char Network_SetLocalPlayerData(void *data, int size)
{
    if (CGame::GetDirectPlay() != NULL)
        return Net_SetPlayerData(CGame::m_localPlayerId, data, size) == NET_OK;
    return 0;
}

// Sends a message to a player (0 = all), guaranteed when requested.
// match 82%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004a1c50
char Network_SendPlayerMessage(int to, int guaranteed, void *data, int size)
{
    if (CGame::GetDirectPlay() != NULL)
        return Net_Send(CGame::m_localPlayerId, to, guaranteed == 1, data, size) == NET_OK;
    return 0;
}


// GLOBAL: CMR2 0x005a1fa8
NetName g_networkPlayerName;
// FUNCTION: CMR2 0x004a1a10
int Network_CreateLocalPlayer(char *param1, char *param2, void *param3, int param4)
{
    memset(&g_networkPlayerName, 0, sizeof(g_networkPlayerName));
    g_networkPlayerName.dwSize = sizeof(g_networkPlayerName);
    g_networkPlayerName.lpszShortNameA = param1;
    g_networkPlayerName.lpszLongNameA = param2;
    if (CGame::GetDirectPlay() == NULL)
        return 0;
    if (Net_CreatePlayer(&CGame::m_localPlayerId, &g_networkPlayerName, param3, param4) != NET_OK)
        return 0;
    CGame::m_unk0x005a1fc0 = 1;
    return 1;
}


// Releases the scene resources held by the 0x58d3xx/0x58d5xx/0x58d6xx blocks
// (the 16 rows at 0x58d3b8 hold file buffers, not scene nodes).
// FUNCTION: CMR2 0x004779e0
BOOL Game_ReleaseSceneResourceBlocks(void)
{
    int i;

    for (i = 0; i < 2; i++) {
        if (g_carInteriorRoots[i] != NULL) {
            if (g_carInteriorNodeRows[i].steeringReferenceNode != 0)
                SceneNode_Destroy((SceneNode *)g_carInteriorNodeRows[i].steeringReferenceNode);
            SceneNode_Destroy(g_carInteriorRoots[i]);
            g_carInteriorNodeRows[i].steeringReferenceNode = 0;
            g_carInteriorNodeRows[i].driverPoseNode = 0;
            g_carInteriorNodeRows[i].steeringWheelNode = 0;
            g_carInteriorNodeRows[i].wiperNodes[0] = 0;
            g_carInteriorNodeRows[i].wiperNodes[1] = 0;
        }
    }
    for (i = 0; i < 2; i++) {
        if (g_carInteriorModelBuffers[i] != NULL) {
            CFileBuffer::FreeGenericFileBuffer(g_carInteriorModelBuffers[i]);
            g_carInteriorModelBuffers[i] = NULL;
        }
    }
    for (i = 0; i < 2; i++) {
        g_carInteriorDashTextures[i].revCounter = 0;
        g_carInteriorDashTextures[i].digit = 0;
    }
    for (i = 0; i < 16; i++) {
        if (g_carInteriorArchives[i].buffer != NULL) {
            CFileBuffer::FreeGenericFileBuffer(g_carInteriorArchives[i].buffer);
            g_carInteriorArchives[i].buffer = NULL;
        }
        g_carInteriorArchives[i].buffer = 0;
        g_carInteriorArchives[i].fileSize = 0;
        g_carInteriorArchives[i].didFileLoad = 0;
    }
    return TRUE;
}

// Adds the player slot to the DirectPlay session.
// match 63%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004aac40
// PORT: initialises the chosen provider (InitializeConnection).
bool Network_AddSessionPlayerSlot(BYTE param1)
{
    if ((BYTE)param1 >= CGame::m_connectionCount)
        return false;
    if (Net_InitializeConnection(*(int *)CGame::m_connections[param1 & 0xff].pConnection) != NET_OK)
        return false;
    CGame::m_maxConnections = (BYTE)param1;
    return true;
}

// Enumera las sesiones o vuelca el buffer recibido en *param2.
// FUNCTION: CMR2 0x004a1b90
int Network_PollReceivedMessageBuffer(DWORD *param1, void **param2)
{
    DWORD bufferSize = CGame::m_unk0x005a1fbc;
    NetPlayerID receiver;
    HRESULT hr;

    if (CGame::GetDirectPlay() == NULL)
        return 0;
    hr = Net_Receive(param1, &receiver, CGame::m_unk0x005a1fb8, &bufferSize);
    if (hr == NET_ERR_BUFFERTOOSMALL) {
        // The original grows the buffer and returns: the message comes with
        // the next poll.
        if (CGame::m_unk0x005a1fb8 != NULL) {
            CFileBuffer::FreeGenericFileBuffer(CGame::m_unk0x005a1fb8);
            CGame::m_unk0x005a1fb8 = NULL;
        }
        CGame::m_unk0x005a1fb8 = CFileBuffer::AllocateLockedBuffer(bufferSize);
        if (CGame::m_unk0x005a1fb8 != NULL)
            CGame::m_unk0x005a1fbc = bufferSize;
        return 0;
    }
    if (hr != NET_OK)
        return 0;
    *param2 = CGame::m_unk0x005a1fb8;
    return 1;
}


// Removes a player (by DirectPlay id) from the session player table.
// match 88%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004a1940
void Network_RemoveSessionPlayerByID(NetPlayerID *pId)
{
    int i;

    for (i = 0; i < 7; i++) {
        if (CGame::m_sessionPlayers[i].playerId == *pId) {
            // Non-variadic prototype: MSVC6 defers the pair's stack cleanup
            // (merged `add esp,0x10` in the original).
            strcpy(CGame::m_sessionPlayers[i].shortName, CMain::m_logFileBlankLine);
            strcpy(CGame::m_sessionPlayers[i].longName, CMain::m_logFileBlankLine);
            CGame::m_sessionPlayers[i].playerId = 0;
            CGame::m_sessionPlayers[i].active = 0;
            CGame::m_sessionPlayerCount--;
            return;
        }
    }
}

// One of the four user values of a listed session.
// FUNCTION: CMR2 0x004a1610
int Session_GetListedUserValue(unsigned int session, BYTE index)
{
    switch (index) {
    case 0:
        return (int)SESSIONS[session & 0xff].dwUser1;
    case 1:
        return (int)SESSIONS[session & 0xff].dwUser2;
    case 2:
        return (int)SESSIONS[session & 0xff].dwUser3;
    }
    return (int)SESSIONS[session & 0xff].dwUser4;
}

// FUNCTION: CMR2 0x004a19c0
int Network_FindSessionPlayerIndex(NetPlayerID *pId, char *pIndex)
{
    int i;

    for (i = 0; i < 7; i++) {
        if (CGame::m_sessionPlayers[i].playerId == *pId) {
            *pIndex = i;
            return 1;
        }
    }
    *pIndex = 0;
    return 0;
}

// FUNCTION: CMR2 0x004a1a00
NetPlayerID Network_GetLocalPlayerID(void)
{
    return CGame::m_localPlayerId;
}

// FUNCTION: CMR2 0x004a1b60
char *Network_GetActiveSessionPlayerLongName(BYTE index)
{
    if (CGame::m_sessionPlayers[index].active != 0)
        return CGame::m_sessionPlayers[index].longName;
    return NULL;
}

// FUNCTION: CMR2 0x004a1b30
SessionPlayerRecord *Network_GetActiveSessionPlayerRecord(BYTE index)
{
    if (CGame::m_sessionPlayers[index].active != 0)
        return &CGame::m_sessionPlayers[index];
    return NULL;
}

BOOL Network_ReadJoinedSessionDescription(void);
bool Network_TestLocalPlayerStatus(void);

// Session name, password and player limit of the network session description.
// FUNCTION: CMR2 0x004a1510
void Session_SetName(LPVOID pName)
{
    Network_ReadJoinedSessionDescription();
    g_networkSession.lpszSessionNameA = (LPSTR)pName;
    Network_TestLocalPlayerStatus();
}

// FUNCTION: CMR2 0x004a1530
void Session_SetPassword(LPVOID pPassword)
{
    Network_ReadJoinedSessionDescription();
    g_networkSession.lpszPasswordA = (LPSTR)pPassword;
    Network_TestLocalPlayerStatus();
}

// FUNCTION: CMR2 0x004a1550
char Session_SetMaxPlayers(int count)
{
    Network_ReadJoinedSessionDescription();
    g_networkSession.dwMaxPlayers = count;
    return Network_TestLocalPlayerStatus();
}

void SceneNode_UpdateTree(SceneNode *pNode, int unused);
void SceneNode_FlushTransforms(SceneNode *pNode);
void Scene_SetViewFromCamera(SceneNode *pCamera);

// Updates a scene tree for drawing from a camera.
// FUNCTION: CMR2 0x0049ce10
int Game_PrepareScene(SceneNode *pRoot, SceneNode *pCamera, void *pRect, int param)
{
    SceneNode_UpdateTree(pRoot, param);
    SceneNode_FlushTransforms(pRoot);
    Scene_SetViewFromCamera(pCamera);
    return 1;
}

// Network session flag 0x20 (set when `open` is 0).
// FUNCTION: CMR2 0x004a1570
void Session_SetOpen(char open)
{
    Network_ReadJoinedSessionDescription();
    if (open != 0) {
        g_networkSession.dwFlags &= 0xffffffdf;
        Network_TestLocalPlayerStatus();
        return;
    }
    g_networkSession.dwFlags |= 0x20;
    Network_TestLocalPlayerStatus();
}

// One of the four session user values (0x40..0x4c of the description).
// FUNCTION: CMR2 0x004a1680
int Session_GetUserValue(BYTE index)
{
    Network_ReadJoinedSessionDescription();
    switch (index) {
    case 0:
        return g_networkSession.dwUser1;
    case 1:
        return g_networkSession.dwUser2;
    case 2:
        return g_networkSession.dwUser3;
    }
    return g_networkSession.dwUser4;
}
