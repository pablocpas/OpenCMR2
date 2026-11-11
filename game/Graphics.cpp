#include "EffectAmounts.h"
#include "render/effect_settings.h"
#include "France1Textures.h"
#include "SceneryPack.h"
#include "DustPlume.h"
#include "AirMote.h"
#include "LightField.h"
#include "SnowGlint.h"
#include "CarBodyDirt.h"
#include "WorldDust.h"
static WorldDust::Pool worldDust;
#include "GraphicsOptions.h"
#include "port/sys.h"
#include "port/vector_sprite.h"
#include <stdio.h>
#include "FloatToInt.h"
#include <stddef.h>
#include <math.h>
#include <string.h>
#include "Graphics.h"
#include "Car.h"
#include "CarResources.h"
#include "Glow.h"
#include "Sprite.h"
#include "Frontend.h"
#include "GenericFileLoader.h"
#include "FileBuffer.h"
#include "GameInfo.h"
#include "InstallInfo.h"
#include "RegKey.h"
#include "main.h"
#include "Sound.h"
#include "Game.h"
#include <cstring>
#include <stdlib.h>
#include "LayoutChecks.h"
#include "LightZone.h"

// Not declared in Mesh.h yet; the other two are not analysed yet.
void Mesh_ReuploadAll(void);
void Scene_RestoreLights(void);
void Sound_RestartMusicAfterDeviceCreation(void);

// GLOBAL: CMR2 0x00660830
Graphics g_graphics;

// GLOBAL: CMR2 0x00520b74
Graphics *g_pGraphics = &g_graphics;
// The Direct3D device state (vertex buffers, textures, matrices).
// GLOBAL: CMR2 0x00660d90
D3DTextureManager g_textureManager;
D3DTextureManager *CGraphics::m_pTextureManager = &g_textureManager;

char CGraphics::m_strSettingConfigurationToDefault[36] = "Setting configuration to defaults";

BOOL CGraphics::m_unk0x00520b7c = TRUE;
unsigned int CGraphics::m_unk0x0065fa2c;
unsigned int CGraphics::m_textureCount;
unsigned int CGraphics::m_lockedTextureCount;
LockedTexture CGraphics::m_lockedTextures[5];
Texture CGraphics::m_textureCache[64];
unsigned int CGraphics::m_unk0x00520b2c = 0xff;
unsigned int CGraphics::m_unk0x00520b30 = 0xff;
int CGraphics::m_unk0x0065fa44;
int CGraphics::m_unk0x0065fa48;
float CGraphics::m_oneOver128 = 1.0f / 128.0f;
float CGraphics::m_unk0x00520b34 = 1.0f;
float CGraphics::m_unk0x00520b38 = 1.0f;
BYTE CGraphics::m_tgaPixel[4];
float CGraphics::m_bumpScale = 1.0f;
char CGraphics::m_ddsExtension[8] = ".DDS";
char CGraphics::m_tgaExtension[8] = ".TGA";
TGAImageInfo CGraphics::m_tgaImageInfo;
int CGraphics::m_unk0x00816a80;
int CGraphics::m_unk0x00816a84;
int CGraphics::m_mipMapSurfaces[2];
DWORD CGraphics::m_cubeMapSize = 64;
int CGraphics::m_unk0x00520b14 = 1;
char CGraphics::m_strSuffixBU[4] = "BU";
char CGraphics::m_strSuffixRU[4] = "RU";
char CGraphics::m_strSuffixBR[4] = "BR";
char CGraphics::m_strSuffixBODF[8] = "BODF";
char CGraphics::m_strSuffixDIGIT[8] = "DIGIT";
char CGraphics::m_strSuffixREVCT[8] = "REVCT";
DWORD CGraphics::m_clearColour;
int CGraphics::m_cullMode;
int CGraphics::m_zEnable;
int CGraphics::m_textureAddressClamp;
int CGraphics::m_zWriteEnable;
int CGraphics::m_texCoordIndex[8];
Unk0x006e0bb0 CGraphics::m_d3dDeviceDesc7;
double CGraphics::m_oneOver65536 = 1.0 / 65536.0;
double CGraphics::m_65536 = 65536.0;
float CGraphics::m_projectionScale = 0.5f;
float CGraphics::m_nearPlane = 1.0f;
float CGraphics::m_farPlane = 250.0f;
float CGraphics::m_fovX = 2.337f;
float CGraphics::m_fovY = 4.197f;
int CGraphics::m_farPlaneFixed = 65536000;
float CGraphics::m_projection11;
float CGraphics::m_projection22;
float CGraphics::m_projection33;
float CGraphics::m_projection43;
char CGraphics::m_strSetDesktopTo16Bit[48] = "Set windows desktop to 16 bit for this 3D card";
int CGraphics::m_unk0x0072d56c;
BOOL CGraphics::m_unk0x00660bfc;
TextureFormat CGraphics::m_texFormat16;
TextureFormat CGraphics::m_texFormat16Alpha;
TextureFormat CGraphics::m_texFormatDXT1_16;
TextureFormat CGraphics::m_texFormatDXT5_16;
TextureFormat CGraphics::m_texFormatBump16;
TextureFormat CGraphics::m_texFormat24;
TextureFormat CGraphics::m_texFormat32;
TextureFormat CGraphics::m_texFormatDXT1_32;
TextureFormat CGraphics::m_texFormatDXT5_32;
TextureFormat CGraphics::m_texFormatBump32;
BOOL CGraphics::m_hasTexFormat16;
BOOL CGraphics::m_hasTexFormatDXT1_16;
BOOL CGraphics::m_hasTexFormat16Alpha;
BOOL CGraphics::m_hasTexFormatDXT5_16;
BOOL CGraphics::m_hasTexFormat24;
BOOL CGraphics::m_hasTexFormatDXT1_32;
BOOL CGraphics::m_hasTexFormat32;
BOOL CGraphics::m_hasTexFormatDXT5_32;
BOOL CGraphics::m_hasTexFormatBump16;
BOOL CGraphics::m_hasTexFormatBump32;
unsigned int CGraphics::m_unk0x0065fa28;
int CGraphics::m_unk0x006dd890;
int CGraphics::m_unk0x00663b1c;
Unk0x0065ff90 CGraphics::m_unk0x0065ff90[10];
BOOL CGraphics::m_unk0x0081709c;
DDDeviceEnumBuffer CGraphics::m_unk0x0065fd08;
DDDeviceEnumBuffer CGraphics::m_displayDevicePool;
int CGraphics::m_lifetimeDisplayDeviceCount = 0;
int CGraphics::m_totalPixelsForScreen = 0;
Unk0x00660040 CGraphics::m_unk0x00660040[10];
int CGraphics::m_unk0x00663b18 = 0;
int CGraphics::m_unk0x00663b20 = 0;
int CGraphics::m_unk0x00663b24 = 0;
int CGraphics::m_selectedDisplayDeviceIx = 0;
DWORD CGraphics::m_displayCount = 0;
Entry CGraphics::m_unk0x006634d8[10];
DisplayMode CGraphics::m_displays[10];
int CGraphics::m_releaseSurfaceCallbackID;
char CGraphics::m_direct3DHAL[13] = "Direct3D HAL";
char CGraphics::m_direct3DTLHAL[18] = "Direct3D T&L HAL";

// FUNCTION: CMR2 0x00405830
// PORT: the "graphics card" is the OpenCMR2 renderer, so the configuration
// is reset once (first run), and with a log line instead of a message box.
// The name kept is the same whichever API it draws with.
bool CGraphics::InitializeDirectX(void) {
    static const char deviceName[] = "OpenCMR2 SDL_GPU";

    if (strcmp(CGameInfo::m_gameInfo.graphicsCardName, deviceName) == 0)
        return false;

    sprintf(CGameInfo::m_gameInfo.graphicsCardName, CRegKey::m_regKeyPathFormatValue, deviceName);
    Sys_Log("%s", m_strSettingConfigurationToDefault);
    return true;
}

// FUNCTION: CMR2 0x00405990
// PORT: a new configuration starts at the display's aspect ratio
// (Gfx_GetDefaultDisplayMode) instead of 640x480, in 32-bit colour, the only
// depth the renderer offers (16-bit made it fall back to 640x480), and at
// high car detail, which the menu allows with 1024-pixel textures.
void CGraphics::SetDefaults(void) {
    int width, height;

    Gfx_GetDefaultDisplayMode(&width, &height);
    CGameInfo::m_gameInfo.graphicsOptions |= 0x40000000;
    CGameInfo::m_gameInfo.screenWidth = width;
    g_pGraphics->resX = width;
    CGameInfo::m_gameInfo.screenHeight = height;
    g_pGraphics->resY = height;
    CGameInfo::m_gameInfo.screenColourDepth = 0x20;
    g_pGraphics->depth = 0x20;
    CGameInfo::m_gameInfo.graphicsOptions |= 1;
    g_pGraphics->isFullscreen = 1;
    CGameInfo::m_gameInfo.graphicsOptions &= 0xfffffff9;
    g_pGraphics->field913_0x3bc &= 0xfffffff7;
    g_pGraphics->field913_0x3bc &= 0xffffffef;
    g_pGraphics->field913_0x3bc &= 0xffffff7f;
    CGameInfo::m_gameInfo.graphicsOptions &= 0xfffffff7;
    g_pGraphics->field913_0x3bc &= 0xffffffdf;
    CGameInfo::m_gameInfo.graphicsOptions &= 0xffffffef;
    g_pGraphics->field913_0x3bc &= 0xffffffbf;
    CGameInfo::m_gameInfo.graphicsOptions &= 0xfff3e01f;
    g_pGraphics->field913_0x3bc &= 0xfffffffe;
    g_pGraphics->field913_0x3bc &= 0xfffffffd;

    CGameInfo::m_gameInfo.graphicsOptions =
        (CGameInfo::m_gameInfo.graphicsOptions & 0xfe3fffff) | 0x200000;

    g_pGraphics->field917_0x3c0 = 1;

    CGameInfo::m_gameInfo.graphicsOptions =
        (CGameInfo::m_gameInfo.graphicsOptions & 0xebffffff) | 0xa000000;
    CGameInfo::m_gameInfo.field_0x34 &= 0xfffffffc;
    CGameInfo::m_gameInfo.graphicsOptions &= 0xdfffffff;

    g_pGraphics->field913_0x3bc &= 0xfffffffb;
}

// FUNCTION: CMR2 0x004a78a0
bool CGraphics::RecreateGraphicsDeviceAndResources(unsigned int screenWidth, unsigned int screenHeight, unsigned int colourDepth, unsigned int param4, unsigned int param5) {
    if (m_unk0x00520b7c == 0) {
        CSound::CloseMusicStreamAndClearPath(TRUE);
        EvictManagedTextureResources(); // TODO: UNFINISHED
        ReleaseDirect3D();
        ReleaseSurfaces();
    }

    SetSelectedDisplayDriverIndex(param4);
    SetSelectedRenderDeviceIndex(param5);

    if (!CreateSelectedDisplayAndRenderDevice(screenWidth, screenHeight, colourDepth) ||
        !CreateDirect3DDevice(screenWidth, screenHeight, colourDepth)) {
        Sys_Log("gfx: device initialization failed; stopping before resource loading");
        CMain::m_exitCode = 1;
        CGame::SetShouldExit();
        return false;
    }
    Mesh_ReuploadAll();
    Scene_RestoreLights();
    Sound_RestartMusicAfterDeviceCreation();
    return true;
}

// FUNCTION: CMR2 0x004a5be0
// PORT: destroys the renderer's textures; the game reloads them on use.
BYTE CGraphics::EvictManagedTextureResources(void) {
    unsigned int index, textureID;
    int face;

    m_unk0x0065fa2c = 0;

    index = 0;
    textureID = 0;
    do {
        Texture* pTexture = m_pTextureManager->textureBuffer[index];
        if (pTexture != NULL && pTexture->pSurface != NULL && textureID == pTexture->textureId) {
            Gfx_DestroyTexture(pTexture->pSurface);
            m_pTextureManager->textureBuffer[index]->pSurface = NULL;
        }
        index++;
        textureID++;
    } while (index < 2048);

    ReleaseCachedTextureSurfaces();

    for (index = 0; index < m_unk0x0065fa28; index++) {
        RenderTexture *pCube = (RenderTexture *)m_pTextureManager->textureBuffer2[index];
        for (face = 5; face >= 0; face--) {
            if (pCube->faces[face].pSurface != NULL) {
                Gfx_DestroyTexture(pCube->faces[face].pSurface);
                pCube->faces[face].pSurface = NULL;
            }
        }
    }

    return TRUE;
}

// FUNCTION: CMR2 0x004a8810
// PORT: only the vertex buffers belong to the "device" now.
BOOL CGraphics::ReleaseDirect3D(void)
{
    ReleaseVertexBuffers();
    m_pTextureManager->pD3D = NULL;
    m_pTextureManager->pDD = NULL;
    return TRUE;
}

// GLOBAL: CMR2 0x0065fa30
unsigned int g_unk0x0065fa30;

// Releases the 64 cached surfaces held by m_textureCache.
// FUNCTION: CMR2 0x004a5ba0
void CGraphics::ReleaseCachedTextureSurfaces(void)
{
    unsigned int i;

    for (i = 0; i < 64; i++) {
        if (m_textureCache[i].pSurface != NULL) {
            Gfx_DestroyTexture(m_textureCache[i].pSurface);
            m_textureCache[i].pSurface = NULL;
        }
    }
    g_unk0x0065fa30 = 0;
}

// FUNCTION: CMR2 0x004b1de0
void CGraphics::ReleaseVertexBuffers(void) {
    int index = 99;

    if (m_pTextureManager->pVertexBuffer3 != NULL) {
        Gfx_DestroyVertexBuffer(m_pTextureManager->pVertexBuffer3);
        m_pTextureManager->pVertexBuffer3 = NULL;
    }
    if (m_pTextureManager->pVertexBuffer2 != NULL) {
        Gfx_DestroyVertexBuffer(m_pTextureManager->pVertexBuffer2);
        m_pTextureManager->pVertexBuffer2 = NULL;
    }
    if (m_pTextureManager->pVertexBuffer1 != NULL) {
        Gfx_DestroyVertexBuffer(m_pTextureManager->pVertexBuffer1);
        m_pTextureManager->pVertexBuffer1 = NULL;
    }

    do {
        if (m_pTextureManager->pVertexBuffers[index] != NULL) {
            Gfx_DestroyVertexBuffer(m_pTextureManager->pVertexBuffers[index]);
            m_pTextureManager->pVertexBuffers[index] = NULL;
        }

        m_pTextureManager->vertexBufferFill[index] = 0;
        index--;
    } while (index >= 0);

    m_unk0x006dd890 = 0;
}

// FUNCTION: CMR2 0x004a8040
// PORT: there are no DirectDraw surfaces; the window and the GPU device stay
// (Platform_Shutdown releases them at exit).
int CGraphics::ReleaseSurfaces(void) {
    g_pGraphics->pSurface3 = NULL;
    g_pGraphics->pBackBufferSurface = NULL;
    g_pGraphics->pPrimarySurface = NULL;
    g_pGraphics->pDD7 = NULL;
    return 1;
}

// FUNCTION: CMR2 0x004a8bd0
void CGraphics::SetSelectedDisplayDriverIndex(int param1) {
    m_unk0x00663b1c = param1;
}

// FUNCTION: CMR2 0x004a8d90
void CGraphics::SetSelectedRenderDeviceIndex(int param1) {
    m_unk0x00663b24 = param1;
}

// FUNCTION: CMR2 0x004a7910
// PORT: picks the mode from the renderer's list as the original did from
// DirectDraw's, then sets the renderer's video mode. Fullscreen comes from
// the OpenCMR2 option video.fullscreen (the original was always fullscreen,
// its windowed build never).
BOOL CGraphics::CreateSelectedDisplayAndRenderDevice(int screenWidth, int screenHeight, int colourDepth) {
    BOOL findMatchingDevice = FALSE;
    DWORD tier = 0;
    int desktopWidth, desktopHeight;
    int count, i, width, height;

    m_pTextureManager->textureInfo2 = NULL;
    m_pTextureManager->textureInfo5 = NULL;
    m_pTextureManager->textureInfo1 = NULL;

    EnumerateAndProbeDisplayDevices(&m_unk0x0065fd08);
    tier = GetTextureFormatCap1(m_unk0x00663b1c);
    while (tier == 0) {
        if (m_unk0x00663b1c + 1 > m_unk0x0065fd08.count - 1) {
            return FALSE;
        }

        SetSelectedDisplayDriverIndex(m_unk0x00663b1c + 1U);
        tier = GetTextureFormatCap1(m_unk0x00663b1c);
    }

    m_unk0x0065fd08.reserved = m_unk0x00663b1c;

    g_pGraphics->resX = screenWidth;
    g_pGraphics->resY = screenHeight;
    g_pGraphics->depth = colourDepth;
    Sys_GetDesktopSize(&desktopWidth, &desktopHeight);
    g_pGraphics->screenResX = desktopWidth;
    g_pGraphics->screenResY = desktopHeight;
    g_pGraphics->isFullscreen = Sys_GetOption("video.fullscreen", 1) != 0;

    m_unk0x00663b18 = 0;
    m_unk0x00663b20 = 0;
    EnumerateDisplayDriverDescriptions();
    EnumeratePreferredHalDevices();

    m_displayCount = 0;
    count = Gfx_GetDisplayModeCount();
    for (i = 0; i < count; i++) {
        Gfx_GetDisplayMode(i, &width, &height);
        EnumerateCompatibleDisplayModes(width, height, 32);
    }

    findMatchingDevice = IsDisplayModeEnumerated(screenWidth, screenHeight, colourDepth);
    if (findMatchingDevice != FALSE) {
        g_pGraphics->resX = screenWidth;
        g_pGraphics->resY = screenHeight;
        g_pGraphics->depth = colourDepth;
    } else {
        // PORT: a saved resolution the display no longer offers (another
        // monitor, or the list of an earlier version) falls back to the
        // display's default instead of 640x480.
        int width, height;
        Gfx_GetDefaultDisplayMode(&width, &height);
        g_pGraphics->resX = width;
        g_pGraphics->resY = height;
        g_pGraphics->depth = 32;
    }

    SelectMatchingDisplayMode(g_pGraphics->resX, g_pGraphics->resY, g_pGraphics->depth);

    if (!Gfx_SetVideoMode(g_pGraphics->resX, g_pGraphics->resY, g_pGraphics->isFullscreen))
        return FALSE;
    EffectSettings::Apply();

    g_pGraphics->field590_0x26c = 0;
    g_pGraphics->field591_0x26e = 0;
    g_pGraphics->field592_0x270 = (WORD)g_pGraphics->resX;
    g_pGraphics->field593_0x272 = (WORD)g_pGraphics->resY;

    LPDWORD pField590AsDword = (LPDWORD)&g_pGraphics->field590_0x26c;
    g_pGraphics->field295_0x13c = pField590AsDword[0];
    g_pGraphics->field296_0x140 = pField590AsDword[1];

    if (m_unk0x00520b7c != 0) {
        m_releaseSurfaceCallbackID = CGame::RegisterCallback(ReleaseSurfaces,NULL);
    }

    return TRUE;
}

// FUNCTION: CMR2 0x004bdb60
// PORT: one display device, the renderer. Its probe and capability
// functions fill the entry as DirectDraw's would for a modern card.
BOOL CGraphics::EnumerateAndProbeDisplayDevices(DDDeviceEnumBuffer* param1) {
    if (m_unk0x0081709c == FALSE)  {
        m_displayDevicePool.count = 0;
        memset(param1, 0, sizeof(DDDeviceEnumBuffer));

        EnumerateDisplayDeviceGUIDCallback(param1);
        param1->count = m_displayDevicePool.count;

        if (m_displayDevicePool.count > 0) {
            ProbeDisplayDriverCapabilities(&param1->entries[0]);
            EnumerateRenderDeviceCapabilities(&param1->entries[0]);
        }

        // The original then copied param1 to 0x20 bytes into
        // m_displayDevicePool, overrunning it by 0x20 bytes; nothing reads
        // that copy, so the port leaves it out.
        m_unk0x0081709c = TRUE;
    }

    return TRUE;
}

// FUNCTION: CMR2 0x004bdc80
// PORT: called once, for the renderer (the primary device has no GUID).
BOOL CGraphics::EnumerateDisplayDeviceGUIDCallback(DDDeviceEnumBuffer *pBuffer)
{
    if (m_displayDevicePool.count == 10)
        return FALSE;

    pBuffer->entries[m_displayDevicePool.count].device.pGUID = NULL;

    m_lifetimeDisplayDeviceCount++;
    m_displayDevicePool.count++;

    return TRUE;
}

// FUNCTION: CMR2 0x004bdd30
// PORT: the renderer has enough memory (capFlag1), stretches blits
// (capFlag200) and can render in a window (capFlag80000).
BOOL CGraphics::ProbeDisplayDriverCapabilities(DDEnumDeviceBufferEntry *pEnumDevice) {
    int width, height;

    if (pEnumDevice->device.pGUID == NULL) {
        Sys_GetDesktopSize(&width, &height);
        m_totalPixelsForScreen = width * height;
    }

    pEnumDevice->capFlag1 = 1;
    pEnumDevice->capFlag200 = 0x200;
    pEnumDevice->capFlag80000 = 0x80000;
    return TRUE;
}

// FUNCTION: CMR2 0x004bde20
void CGraphics::EnumerateRenderDeviceCapabilities(DDEnumDeviceBufferEntry *pEnumDevice) {
    CollectRenderDeviceCapabilitiesCallback(pEnumDevice);
}

// FUNCTION: CMR2 0x004bde60
// PORT: the renderer filters textures linearly, rasterises in hardware,
// has a 24-bit depth buffer and renders 32-bit colour.
int CGraphics::CollectRenderDeviceCapabilitiesCallback(DDEnumDeviceBufferEntry *pEntry) {
    pEntry->capTextureFilter1 = 1;
    pEntry->capTextureFilter3 = 1;
    pEntry->capTextureFilter2 = 1;
    pEntry->capHardwareRasterization = 1;
    pEntry->zBufferBitDepth = 0x18;
    pEntry->hasZBuffer = 1;
    pEntry->capRender16Bit = 1;
    return 1;
}

// FUNCTION: CMR2 0x004a96c0
DWORD CGraphics::GetTextureFormatCap1(int param1) {
    return m_unk0x0065fd08.entries[param1].capFlag1;
}

// FUNCTION: CMR2 0x004a8bc0
INT32 CGraphics::GetSelectedDisplayDriverIndex(void) {
    return m_unk0x00663b1c;
}

// FUNCTION: CMR2 0x004a96e0
DWORD CGraphics::GetTextureFormatCap80000(int param_1) {
    return m_unk0x0065fd08.entries[param_1].capFlag80000;
}

// FUNCTION: CMR2 0x004a8b30
// PORT: one driver, named after the renderer and the API it draws with.
BOOL CGraphics::EnumerateDisplayDriverDescriptions(void) {
    sprintf(m_unk0x006634d8[m_unk0x00663b18].unk_0x00, CRegKey::m_regKeyPathFormatValue, Gfx_GetRendererName());
    m_unk0x00663b18++;
    return TRUE;
}

// FUNCTION: CMR2 0x004a8da0
// PORT: called for each of the renderer's modes. The original's checks
// (at least 640x480, 16 or 32 bits, three buffers fit in video memory) are
// kept; the table holds 10 modes.
HRESULT CGraphics::EnumerateCompatibleDisplayModes(int width, int height, int bpp) {
    if (m_displayCount < 10 && width >= 0x280 && height >= 0x1e0 && (bpp == 0x10 || bpp == 0x20)) {
        DWORD textureMemory = QueryAvailableVideoMemoryForCaps(0x10000000);

        if ((DWORD)(bpp / 8) * height * width * 3 < textureMemory) {
            m_displays[m_displayCount].width = width;
            m_displays[m_displayCount].height = height;
            m_displays[m_displayCount].colourDepth = bpp;
            m_displayCount++;
        }
    }

    return 1;
}

// FUNCTION: CMR2 0x004a96f0
int CGraphics::DeviceCanRender16Bit(int param1) {
    return CGraphics::m_unk0x0065fd08.entries[param1].capRender16Bit;
}

// FUNCTION: CMR2 0x004bdd00
// PORT: a fixed 256 MB, plenty for the game's detail and mode checks.
DWORD CGraphics::QueryAvailableVideoMemoryForCaps(DWORD caps) {
    return 0x10000000;
}

// FUNCTION: CMR2 0x004a8f60
BOOL CGraphics::IsDisplayModeEnumerated(int width, int height, int colourDepth)
{
    for (int i = 0; i < m_displayCount; i++) {
        if (width == m_displays[i].width && height == m_displays[i].height && colourDepth == m_displays[i].colourDepth)
            return TRUE;
    }

    return FALSE;
}

// FUNCTION: CMR2 0x004a8ec0
void CGraphics::SelectMatchingDisplayMode(int width, int height, int colourDepth)
{
    for (int i = 0; i < m_displayCount; i++) {
        if (width == m_displays[i].width && height == m_displays[i].height && colourDepth == m_displays[i].colourDepth)
            m_selectedDisplayDeviceIx = i;
    }
}

// FUNCTION: CMR2 0x004a8d60
DWORD CGraphics::GetSelectedRenderDeviceSurfaceCaps(void) {
  return m_unk0x00660040[m_unk0x00663b24].surfaceCap;
}

// FUNCTION: CMR2 0x004a8c30
// PORT: the renderer is a hardware transform-and-lighting device (surface
// caps 2, "Direct3D T&L HAL").
HRESULT CGraphics::EnumeratePreferredHalDevices(void) {
    m_unk0x00660040[0].surfaceCap = 2;
    sprintf(m_unk0x0065ff90[0].deviceDesc, CRegKey::m_regKeyPathFormatValue, Gfx_GetRendererName());
    sprintf(m_unk0x0065ff90[0].deviceName, CRegKey::m_regKeyPathFormatValue, m_direct3DTLHAL);
    m_unk0x00663b20 = 1;
    return TRUE;
}

// Replaces three alpha values of a texture (4444 or 8888 formats), e.g. to
// recolour masked areas. The untouched original is first saved in cache slot
// cacheSlot so BltTexture can restore it.
// match 42%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// Number of bits set in a pixel-format channel mask.
static inline BYTE Bump_CountBits(DWORD mask)
{
    int count = 0;
    int i;

    for (i = 32; i != 0; i--) {
        if (mask & 1)
            count++;
        mask >>= 1;
    }
    return count;
}

// Bit position of a pixel-format channel mask's lowest set bit (32 for an empty mask).
static inline BYTE Bump_LowestSetBit(DWORD mask)
{
    int count;

    for (count = 0; count < 32; count++) {
        if (mask & 1)
            break;
        mask >>= 1;
    }
    return count;
}

// FUNCTION: CMR2 0x004a4d30
// PORT: renderer lock and copy instead of DirectDraw surfaces.
void CGraphics::RemapTextureAlpha(Texture *pTexture, WORD from0, WORD to0, WORD from1, WORD to1, WORD from2, WORD to2,
                                  int cacheSlot)
{
    GfxLockedRect desc;
    RECT rect;
    unsigned int width;
    unsigned int height;
    int bits;
    unsigned int x;
    unsigned int y;
    int skip;
    DWORD *p32;
    WORD *p16;
    short w;
    short h;
    DWORD f0, t0, f1, t1, f2, t2;

    rect.left = 0;
    w = pTexture->width;
    h = pTexture->height;
    rect.right = w;
    rect.top = 0;
    rect.bottom = h;
    if (m_textureCache[cacheSlot].pSurface == NULL) {
        CreateTextureSurface(&m_textureCache[cacheSlot], w, h, 9);
        Gfx_CopyTexture(m_textureCache[cacheSlot].pSurface, pTexture->pSurface);
        g_unk0x0065fa30++;
    }
    if (!Gfx_LockTexture(pTexture->pSurface, 0, 0, &desc))
        return;
    width = pTexture->width;
    height = pTexture->height;
    bits = Bump_CountBits(desc.format.alphaMask);
    switch (bits) {
    case 4:
        skip = desc.pitch - width * 2;
        p16 = (WORD *)desc.pixels;
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                if ((WORD)((*p16 & 0xf000) >> 8) == (WORD)(((from0 << 8) & 0xf000) >> 8))
                    *p16 = (WORD)(((to0 & 0xf0) << 8) | (*p16 & 0xfff));
                if ((WORD)((*p16 & 0xf000) >> 8) == (WORD)(((from1 << 8) & 0xf000) >> 8))
                    *p16 = (WORD)(((to1 & 0xf0) << 8) | (*p16 & 0xfff));
                if ((WORD)((*p16 & 0xf000) >> 8) == (WORD)(((from2 << 8) & 0xf000) >> 8))
                    *p16 = (WORD)(((to2 & 0xf0) << 8) | (*p16 & 0xfff));
                p16++;
            }
            p16 += skip;
        }
        break;
    case 8:
        skip = (unsigned int)(desc.pitch - width * 4) >> 2;
        p32 = (DWORD *)desc.pixels;
        f0 = from0;
        f1 = from1;
        t0 = to0;
        t1 = to1;
        t2 = to2;
        f2 = from2;
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                if ((*p32 >> 24) == f0)
                    *p32 = (*p32 & 0xffffff) | (t0 << 24);
                if ((*p32 >> 24) == f1)
                    *p32 = (*p32 & 0xffffff) | (t1 << 24);
                if ((*p32 >> 24) == f2)
                    *p32 = (*p32 & 0xffffff) | (t2 << 24);
                p32++;
            }
            p32 += skip;
        }
        break;
    }
    Gfx_UnlockTexture(pTexture->pSurface, 0, 0);
}

// FUNCTION: CMR2 0x004a5080
// PORT: renderer copy instead of a DirectDraw Blt.
void CGraphics::BltTexture(Texture *pTexture, int surfaceIndex)
{
    RECT rect;

    rect.left = 0;
    rect.right = pTexture->width;
    rect.top = 0;
    rect.bottom = pTexture->height;
    if (pTexture->pSurface != NULL && m_textureCache[surfaceIndex].pSurface != NULL)
        Gfx_CopyTexture(pTexture->pSurface, m_textureCache[surfaceIndex].pSurface);
}

// Moves an 8-bit channel value into a 16-bit pixel channel whose top bit is
// `depth` bits above bit 7 (negative: below).
#define PACK_CHANNEL(mask, depth, v)                                                \
    ((depth) >= 0 ? (((mask) >> abs(depth)) & (v)) << abs(depth)                    \
                  : (((mask) << abs(depth)) & (v)) >> abs(depth))

extern const float g_netOne;
extern const float g_netZero;
extern const float g_floatOneOver255;

// Blends pColour (r, g, b, a) into pixel (x, y) of a locked texture.
// match 24%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004a52d0
void CGraphics::BlendPixel(Texture *pTexture, unsigned int x, unsigned int y, BYTE *pColour)
{
    unsigned int i;
    int stride;
    int r, g, b;
    float f, inv;
    BYTE px;
    BYTE dst[3];
    WORD *p16;
    DWORD *p32;

    g = pColour[1];
    r = pColour[0];
    b = pColour[2];
    if (x >= (unsigned int)pTexture->width || y >= (unsigned int)pTexture->height)
        return;
    for (i = 0; i < m_lockedTextureCount; i++) {
        if (pTexture == m_lockedTextures[i].pTexture)
            break;
    }
    if (i == m_lockedTextureCount)
        return;
    stride = m_lockedTextures[i].desc.pitch - ((m_lockedTextures[i].desc.width * m_lockedTextures[i].desc.format.bitsPerPixel) >> 3);
    if (m_lockedTextures[i].desc.format.bitsPerPixel == 16) {
        p16 = (WORD *)m_lockedTextures[i].desc.pixels + ((m_lockedTextures[i].desc.width + stride) * y + x);
        if (pColour[3] != 0xff) {
            px = *(BYTE *)p16;
            dst[0] = (BYTE)((BYTE)m_lockedTextures[i].masks[0] & px) >> (BYTE)m_lockedTextures[i].depths[0];
            dst[1] = (BYTE)((BYTE)m_lockedTextures[i].masks[1] & px) << (BYTE)m_lockedTextures[i].depths[1];
            dst[2] = (BYTE)((BYTE)m_lockedTextures[i].masks[2] & px) << (BYTE)-m_lockedTextures[i].depths[2];
            f = X87S_MUL((float)pColour[3], g_floatOneOver255);
            inv = g_netOne - f;
            r = (int)FTOI(__int64, X87S_ADD(X87S_MUL(X87S_FILD(float, r), f), X87S_MUL((float)dst[0], inv)));
            g = (int)FTOI(__int64, X87S_ADD(X87S_MUL(X87S_FILD(float, g), f), X87S_MUL((float)dst[1], inv)));
            b = (int)FTOI(__int64, X87S_ADD(X87S_MUL(X87S_FILD(float, b), f), X87S_MUL((float)dst[2], inv)));
        }
        *p16 = (WORD)(PACK_CHANNEL(m_lockedTextures[i].masks[3], (short)m_lockedTextures[i].depths[3], 0xff) |
                      PACK_CHANNEL(m_lockedTextures[i].masks[0], (short)m_lockedTextures[i].depths[0], r) |
                      PACK_CHANNEL(m_lockedTextures[i].masks[1], (short)m_lockedTextures[i].depths[1], g) |
                      PACK_CHANNEL(m_lockedTextures[i].masks[2], (short)m_lockedTextures[i].depths[2], b));
    } else if (m_lockedTextures[i].desc.format.bitsPerPixel == 32) {
        p32 = (DWORD *)m_lockedTextures[i].desc.pixels + ((m_lockedTextures[i].desc.width + stride) * y + x);
        if (pColour[3] != 0xff) {
            dst[0] = 0;
            dst[1] = 0;
            dst[2] = *(BYTE *)p32;
            f = X87S_MUL((float)pColour[3], g_floatOneOver255);
            inv = g_netOne - f;
            r = (int)FTOI(__int64, X87S_ADD(X87S_MUL(X87S_FILD(float, r), f), g_netZero * inv));
            g = (int)FTOI(__int64, X87S_ADD(X87S_MUL(X87S_FILD(float, g), f), g_netZero * inv));
            b = (int)FTOI(__int64, X87S_ADD(X87S_MUL(X87S_FILD(float, b), f), X87S_MUL((float)dst[2], inv)));
        }
        *p32 = GFX_RGBA(r, g, b, 0xff);
    }
}

// FUNCTION: CMR2 0x004a56c0
void CGraphics::UnlockTexture(Texture *pTexture)
{
    unsigned int i;

    for (i = 0; i < m_lockedTextureCount; i++) {
        if (pTexture == m_lockedTextures[i].pTexture)
            break;
    }
    if (i != m_lockedTextureCount) {
        Gfx_UnlockTexture(m_lockedTextures[i].pTexture->pSurface, 0, 0);
        m_lockedTextures[i].pTexture = NULL;
        m_lockedTextureCount--;
    }
}

// Position of the lowest set bit of a channel mask.
static inline BYTE Pixel_MaskShift(unsigned int mask)
{
    int shift;

    for (shift = 0; shift < 32; shift++) {
        if (mask & 1)
            break;
        mask >>= 1;
    }
    return shift;
}

// Number of bits set in a channel mask.
static inline BYTE Pixel_MaskBits(unsigned int mask)
{
    int count = 0;
    int i;

    for (i = 32; i != 0; i--) {
        if (mask & 1)
            count++;
        mask >>= 1;
    }
    return count;
}

// FUNCTION: CMR2 0x004a5730
unsigned int CGraphics::GetPixelRed(GfxLockedRect *pDesc, int x, int y)
{
    WORD *pPixel;
    int pad;

    pad = pDesc->pitch - (pDesc->width * pDesc->format.bitsPerPixel >> 3);
    if (pDesc->format.bitsPerPixel == 16) {
        pPixel = (WORD *)pDesc->pixels + (pDesc->width + pad) * y + x;
        return (((*pPixel & pDesc->format.redMask) >> Pixel_MaskShift(pDesc->format.redMask)) & 0xff)
               << (8 - Pixel_MaskBits(pDesc->format.redMask));
    } else if (pDesc->format.bitsPerPixel == 32) {
        return (BYTE)(((DWORD *)pDesc->pixels)[(pDesc->width + pad) * y + x] >> 16);
    }
    return 0;
}

// FUNCTION: CMR2 0x004a57e0
unsigned int CGraphics::GetPixelAlpha(GfxLockedRect *pDesc, int x, int y)
{
    WORD *pPixel;
    int pad;

    pad = pDesc->pitch - (pDesc->width * pDesc->format.bitsPerPixel >> 3);
    if (pDesc->format.bitsPerPixel == 16) {
        pPixel = (WORD *)pDesc->pixels + (pDesc->width + pad) * y + x;
        return (((*pPixel & pDesc->format.alphaMask) >> Pixel_MaskShift(pDesc->format.alphaMask)) & 0xff)
               << (8 - Pixel_MaskBits(pDesc->format.alphaMask));
    } else if (pDesc->format.bitsPerPixel == 32) {
        return ((DWORD *)pDesc->pixels)[(pDesc->width + pad) * y + x] >> 24;
    }
    return 0;
}

// FUNCTION: CMR2 0x004a5d10
BOOL CGraphics::FreeTextureBuffers(void)
{
    int i;

    for (i = 0; i < 2048; i++) {
        if (m_pTextureManager->textureBuffer[i] != NULL) {
            CFileBuffer::FreeGenericFileBuffer(m_pTextureManager->textureBuffer[i]);
            m_pTextureManager->textureBuffer[i] = NULL;
            if (m_textureCount > 0)
                m_textureCount--;
        }
    }
    for (i = 0; i < 20; i++) {
        if (m_pTextureManager->textureBuffer2[i] != NULL) {
            CFileBuffer::FreeGenericFileBuffer(m_pTextureManager->textureBuffer2[i]);
            m_pTextureManager->textureBuffer2[i] = NULL;
            if (m_unk0x0065fa28 > 0)
                m_unk0x0065fa28--;
        }
    }
    return TRUE;
}

// FUNCTION: CMR2 0x004a5fe0
unsigned int CGraphics::GetTextureMemoryKilobytes(void)
{
    return m_unk0x0065fa2c >> 10;
}

// FUNCTION: CMR2 0x004a5ff0
void CGraphics::SetPrimaryColourByte(BYTE param1)
{
    if (param1 <= 0xff)
        m_unk0x00520b2c = param1;
}

// FUNCTION: CMR2 0x004a6040
void CGraphics::SetPrimaryColourBias(BYTE param1)
{
    if (param1 <= 0xff)
        m_unk0x0065fa44 = param1 - 0x80;
}

// FUNCTION: CMR2 0x004a6060
void CGraphics::SetSecondaryColourByte(BYTE param1)
{
    if (param1 <= 0xff)
        m_unk0x00520b30 = param1;
}

// FUNCTION: CMR2 0x004a60b0
void CGraphics::SetSecondaryColourBias(BYTE param1)
{
    if (param1 <= 0xff)
        m_unk0x0065fa48 = param1 - 0x80;
}

// FUNCTION: CMR2 0x004a87c0
// PORT: render targets get their depth buffer from the renderer.
HRESULT CGraphics::CopyZBufferPixelFormat(void)
{
    return 0;
}

// FUNCTION: CMR2 0x004a8be0
int CGraphics::GetDisplayDriverCount(void)
{
    return m_unk0x00663b18;
}

// FUNCTION: CMR2 0x004a8bf0
void CGraphics::GetDisplayDeviceNames(int index, LPSTR description, LPSTR name)
{
    sprintf(description, CRegKey::m_regKeyPathFormatValue, m_unk0x006634d8[index].unk_0x00);
    sprintf(name, CRegKey::m_regKeyPathFormatValue, m_unk0x006634d8[index].name);
}

// FUNCTION: CMR2 0x004a8d80
int CGraphics::GetSelectedRenderDeviceIndex(void)
{
    return m_unk0x00663b24;
}

// FUNCTION: CMR2 0x004a8eb0
int CGraphics::GetSelectedDisplayDeviceIx(void)
{
    return m_selectedDisplayDeviceIx;
}

// FUNCTION: CMR2 0x004a8f10
DWORD CGraphics::GetDisplayCount(void)
{
    return m_displayCount;
}

// FUNCTION: CMR2 0x004a8f20
void CGraphics::GetDisplayMode(int index, DWORD *pWidth, DWORD *pHeight, DWORD *pColourDepth)
{
    *pWidth = m_displays[index].width;
    *pHeight = m_displays[index].height;
    *pColourDepth = m_displays[index].colourDepth;
}

// FUNCTION: CMR2 0x004a96d0
DWORD CGraphics::GetTextureFormatCap200(int param1)
{
    return m_unk0x0065fd08.entries[param1].capFlag200;
}

// FUNCTION: CMR2 0x004a6010
void CGraphics::SetPrimaryColourScale(BYTE param1)
{
    if (param1 <= 0xff)
        m_unk0x00520b34 = X87S_MUL((float)param1, m_oneOver128);
}

// FUNCTION: CMR2 0x004a6080
void CGraphics::SetSecondaryColourScale(BYTE param1)
{
    if (param1 <= 0xff)
        m_unk0x00520b38 = X87S_MUL((float)param1, m_oneOver128);
}

// Header of a .tga file (18 bytes).
struct TGAHeader {
    BYTE idLength;          // 0x00 bytes of image id after the header
    BYTE colourMapType;     // 0x01
    BYTE imageType;         // 0x02 2 = uncompressed true colour
    BYTE colourMapSpec[5];  // 0x03
    short xOrigin;          // 0x08
    short yOrigin;          // 0x0a
    unsigned short width;   // 0x0c
    unsigned short height;  // 0x0e
    BYTE bitsPerPixel;      // 0x10
    BYTE descriptor;        // 0x11 bits 0..3 alpha bits, 4..5 origin
};

// Accepts an uncompressed 24-bit or 32-bit (8-bit alpha) TGA without colour
// map, origin or right-to-left / top-down order, and describes its pixels.
// FUNCTION: CMR2 0x004a6670
TGAImageInfo *CGraphics::ParseTGAHeader(BYTE *pData)
{
    TGAHeader *pHeader = (TGAHeader *)pData;

    if (pHeader == NULL || pHeader->colourMapType != 0 || pHeader->xOrigin != 0 || pHeader->yOrigin != 0 ||
        (pHeader->descriptor & 0x70) != 0 || pHeader->imageType == 3 || pHeader->imageType != 2)
        return NULL;

    if (pHeader->bitsPerPixel == 24) {
        m_tgaImageInfo.bytesPerPixel = 3;
    } else {
        if (pHeader->bitsPerPixel != 32)
            return NULL;
        if ((pHeader->descriptor & 0xf) != 8)
            return NULL;
        m_tgaImageInfo.bytesPerPixel = 4;
    }
    m_tgaImageInfo.width = pHeader->width;
    m_tgaImageInfo.height = pHeader->height;
    m_tgaImageInfo.pixels = pData + pHeader->idLength + sizeof(TGAHeader);
    return &m_tgaImageInfo;
}

#define RENDER_TEXTURE(i) ((RenderTexture *)m_pTextureManager->textureBuffer2[i])

// FUNCTION: CMR2 0x004a82c0
// PORT: the renderer never loses surfaces, so there is nothing to restore.
void CGraphics::RestoreSurfaces(void)
{
}

// Number of low zero bits of a texture dimension (its mip level count).
static inline BYTE Mip_LowZeroBits(unsigned int dim)
{
    int count;

    for (count = 0; count < 32; count++) {
        if (dim & 1)
            break;
        dim >>= 1;
    }
    return count;
}

// FUNCTION: CMR2 0x004bd970
// PORT: returns the level count (at most 3) instead of filling a surface
// description.
int CGraphics::SetMipMapCount(int width, int height)
{
    int count;

    if (width > height)
        count = Mip_LowZeroBits(width);
    else
        count = Mip_LowZeroBits(height);
    if (count > 3)
        count = 3;
    return count;
}

// FUNCTION: CMR2 0x004bd9d0
// PORT: records which of the two levels below the top one exist.
void CGraphics::GetMipMapSurfaces(Texture *pTexture)
{
    int levels;
    int i;

    levels = Gfx_GetTextureMipCount(pTexture->pSurface);
    for (i = 0; i < 2; i++)
        m_mipMapSurfaces[i] = i + 1 < levels;
}

// FUNCTION: CMR2 0x004bda60
int CGraphics::GetMipMapDataSize(Texture *pTexture)
{
    int width;
    int height;
    int levels;
    int total;
    int i;
    int size;

    width = m_unk0x00816a80;
    height = m_unk0x00816a84;
    total = 0;
    levels = 0;
    for (i = 0; i < 2; i++) {
        if (m_mipMapSurfaces[i] == NULL)
            break;
        levels++;
    }
    for (i = levels; i > 0; i--) {
        width /= 2;
        height /= 2;
        total += height * width;
    }

    size = total * 2;
    if (pTexture->bitsPerPixel != 16)
        size = total * 4;
    return size;
}

// FUNCTION: CMR2 0x004bdad0
int CGraphics::GetMipMapPixelCount(Texture *pTexture)
{
    int width;
    int height;
    int levels;
    int total;
    int i;

    width = m_unk0x00816a80;
    height = m_unk0x00816a84;
    total = 0;
    levels = 0;
    for (i = 0; i < 2; i++) {
        if (m_mipMapSurfaces[i] == NULL)
            break;
        levels++;
    }
    for (i = levels; i > 0; i--) {
        width /= 2;
        height /= 2;
        total += height * width;
    }
    return total;
}

// FUNCTION: CMR2 0x004bdb20
// PORT: the renderer fills the levels from the top one (the original
// stretch-blitted the top level into each).
void CGraphics::BltMipMaps(Texture *pTexture)
{
    if (m_mipMapSurfaces[0])
        Gfx_GenerateMipmaps(pTexture->pSurface);
}

// FUNCTION: CMR2 0x004a50f0
// PORT: the renderer locks the whole level; a rectangle moves the pointer.
void CGraphics::LockTexture(Texture *pTexture, RECT *pRect)
{
    unsigned int index;
    unsigned int i;
    unsigned int bits;
    int count;

    index = m_lockedTextureCount;
    if (pTexture == NULL || pTexture->pSurface == NULL)
        return;

    for (i = 0; i < m_lockedTextureCount; i++) {
        if (pTexture == m_lockedTextures[i].pTexture)
            return;
    }

    m_lockedTextures[m_lockedTextureCount].pTexture = pTexture;
    memset(&m_lockedTextures[m_lockedTextureCount].desc, 0, sizeof(GfxLockedRect));
    if (!Gfx_LockTexture(m_lockedTextures[index].pTexture->pSurface, 0, 0, &m_lockedTextures[m_lockedTextureCount].desc))
        return;
    if (pRect != NULL) {
        GfxLockedRect *pDesc = &m_lockedTextures[m_lockedTextureCount].desc;
        pDesc->pixels = (BYTE *)pDesc->pixels + pRect->top * pDesc->pitch + pRect->left * (pDesc->format.bitsPerPixel / 8);
    }

    if (m_lockedTextures[m_lockedTextureCount].desc.format.bitsPerPixel == 16) {
        m_lockedTextures[m_lockedTextureCount].masks[0] = m_lockedTextures[m_lockedTextureCount].desc.format.redMask;
        m_lockedTextures[m_lockedTextureCount].masks[1] = m_lockedTextures[m_lockedTextureCount].desc.format.greenMask;
        m_lockedTextures[m_lockedTextureCount].masks[2] = m_lockedTextures[m_lockedTextureCount].desc.format.blueMask;
        m_lockedTextures[m_lockedTextureCount].masks[3] = m_lockedTextures[m_lockedTextureCount].desc.format.alphaMask;

        bits = m_lockedTextures[m_lockedTextureCount].masks[0];
        for (count = 0; count < 32; count++) {
            if (bits & 1)
                break;
            bits >>= 1;
        }
        m_lockedTextures[m_lockedTextureCount].shifts[0] = (BYTE)count;
        bits = m_lockedTextures[m_lockedTextureCount].masks[1];
        for (count = 0; count < 32; count++) {
            if (bits & 1)
                break;
            bits >>= 1;
        }
        m_lockedTextures[m_lockedTextureCount].shifts[1] = (BYTE)count;
        bits = m_lockedTextures[m_lockedTextureCount].masks[2];
        for (count = 0; count < 32; count++) {
            if (bits & 1)
                break;
            bits >>= 1;
        }
        m_lockedTextures[m_lockedTextureCount].shifts[2] = (BYTE)count;
        bits = m_lockedTextures[m_lockedTextureCount].masks[3];
        for (count = 0; count < 32; count++) {
            if (bits & 1)
                break;
            bits >>= 1;
        }
        m_lockedTextures[m_lockedTextureCount].shifts[3] = (BYTE)count;

        bits = m_lockedTextures[m_lockedTextureCount].masks[0];
        for (count = 0; count < 32; count++) {
            if (bits == 0)
                break;
            bits >>= 1;
        }
        m_lockedTextures[m_lockedTextureCount].depths[0] = (BYTE)count - 8;
        bits = m_lockedTextures[m_lockedTextureCount].masks[1];
        for (count = 0; count < 32; count++) {
            if (bits == 0)
                break;
            bits >>= 1;
        }
        m_lockedTextures[m_lockedTextureCount].depths[1] = (BYTE)count - 8;
        bits = m_lockedTextures[m_lockedTextureCount].masks[2];
        for (count = 0; count < 32; count++) {
            if (bits == 0)
                break;
            bits >>= 1;
        }
        m_lockedTextures[m_lockedTextureCount].depths[2] = (BYTE)count - 8;
        bits = m_lockedTextures[m_lockedTextureCount].masks[3];
        for (count = 0; count < 32; count++) {
            if (bits == 0)
                break;
            bits >>= 1;
        }
        m_lockedTextures[m_lockedTextureCount].depths[3] = (BYTE)count - 8;
    }
    m_lockedTextureCount++;
}

// FUNCTION: CMR2 0x004a76d0
// PORT: one renderer cube map (a render target with its own depth buffer);
// faces 1-5 are its face views.
RenderTexture *CGraphics::CreateCubeMapSurfaces(RenderTexture *pTexture)
{
    int i;

    pTexture->faces[0].pSurface = Gfx_CreateTexture(m_cubeMapSize, m_cubeMapSize, m_pTextureManager->textureInfo1->gfxFormat, 1,
                                                    GFX_TEXTURE_RENDER_TARGET | GFX_TEXTURE_CUBE);
    for (i = 1; i < 6; i++)
        pTexture->faces[i].pSurface = Gfx_GetCubeFace(pTexture->faces[0].pSurface, i);
    for (i = 0; i < 6; i++)
        pTexture->pZBuffers[i] = NULL;
    return pTexture;
}

// FUNCTION: CMR2 0x004a91b0
// PORT: called by SelectTextureFormats for each renderer format.
HRESULT CGraphics::EnumTextureFormatsCallback(int gfxFormat, GfxPixelFormat *pddpf)
{
    int i;
    int count;
    unsigned int bits;
    unsigned int alphaMask;
    unsigned int rBits;
    unsigned int gBits;
    unsigned int bBits;
    unsigned int aBits;
    int rShift;
    int gShift;
    int bShift;
    int aShift;
    TextureFormat *pFormat;
    HRESULT result;

    result = 1;

    if (pddpf->flags & GFX_PF_RGB) {
        bits = pddpf->redMask;
        alphaMask = ~(pddpf->redMask | pddpf->greenMask | pddpf->blueMask) & pddpf->alphaMask;
        count = 0;
        for (i = 32; i != 0; i--) {
            if (bits & 1)
                count++;
            bits >>= 1;
        }
        rBits = (BYTE)count;
    rBits = rBits;
        bits = pddpf->greenMask;
        count = 0;
        for (i = 32; i != 0; i--) {
            if (bits & 1)
                count++;
            bits >>= 1;
        }
        gBits = (BYTE)count;
    gBits = gBits;
        bits = pddpf->blueMask;
        count = 0;
        for (i = 32; i != 0; i--) {
            if (bits & 1)
                count++;
            bits >>= 1;
        }
        bBits = (BYTE)count;
    bBits = bBits;
        bits = alphaMask;
        count = 0;
        for (i = 32; i != 0; i--) {
            if (bits & 1)
                count++;
            bits >>= 1;
        }
        aBits = (BYTE)count;
    aBits = aBits;

        if (rBits < 1 || gBits < 1 || bBits < 1)
            return result;

        bits = pddpf->redMask;
        for (rShift = 0; rShift < 32; rShift++) {
            if (bits & 1)
                break;
            bits >>= 1;
        }
        bits = pddpf->greenMask;
        for (gShift = 0; gShift < 32; gShift++) {
            if (bits & 1)
                break;
            bits >>= 1;
        }
        bits = pddpf->blueMask;
        for (bShift = 0; bShift < 32; bShift++) {
            if (bits & 1)
                break;
            bits >>= 1;
        }
        bits = alphaMask;
        for (aShift = 0; aShift < 32; aShift++) {
            if (bits & 1)
                break;
            bits >>= 1;
        }

        if (bBits + aBits + gBits + rBits <= 16) {
            if (aBits != 0) {
                if (rBits == 5) {
                    if (gBits != 5)
                        return result;
                    if (bBits != 5)
                        return result;
                    if (aBits != 1)
                        return result;
                } else {
                    if (rBits != 4)
                        return result;
                    if (gBits != 4)
                        return result;
                    if (bBits != 4)
                        return result;
                    if (aBits != 4)
                        return result;
                }
                if (m_texFormat16Alpha.bits[0] != 4) {
                m_texFormat16Alpha.format = *pddpf;
            m_texFormat16Alpha.gfxFormat = gfxFormat;
            m_texFormat16Alpha.bits[2] = (BYTE)gBits;
            m_texFormat16Alpha.bits[0] = (BYTE)rBits;
            m_texFormat16Alpha.bits[1] = (BYTE)bBits;
            m_texFormat16Alpha.bits[3] = (BYTE)aBits;
            m_texFormat16Alpha.shifts[0] = (BYTE)rShift;
            m_texFormat16Alpha.shifts[1] = (BYTE)bShift;
            m_texFormat16Alpha.shifts[2] = (BYTE)gShift;
            m_texFormat16Alpha.shifts[3] = (BYTE)aShift;
                m_hasTexFormat16Alpha = TRUE;
                    return result;
                }
            } else {
                if (rBits == 5 && (gBits == 5 || gBits == 6) && bBits == 5 && m_texFormat16.bits[2] != 6) {
                    m_texFormat16.format = *pddpf;
            m_texFormat16.gfxFormat = gfxFormat;
                    m_texFormat16.bits[2] = (BYTE)gBits;
                    m_texFormat16.bits[1] = (BYTE)bBits;
                    m_texFormat16.shifts[2] = (BYTE)gShift;
                    m_texFormat16.shifts[1] = (BYTE)bShift;
                    m_texFormat16.bits[0] = 5;
                    m_texFormat16.bits[3] = 0;
                    m_texFormat16.shifts[0] = (BYTE)rShift;
                    m_texFormat16.shifts[3] = (BYTE)aShift;
                    m_hasTexFormat16 = TRUE;
                    return result;
                }
            }
        } else {
            if (bBits + aBits + gBits + rBits == 24) {
            m_texFormat24.format = *pddpf;
            m_texFormat24.gfxFormat = gfxFormat;
            m_texFormat24.bits[2] = (BYTE)gBits;
            m_texFormat24.bits[0] = (BYTE)rBits;
            m_texFormat24.bits[1] = (BYTE)bBits;
            m_texFormat24.bits[3] = (BYTE)aBits;
            m_texFormat24.shifts[0] = (BYTE)rShift;
            m_texFormat24.shifts[1] = (BYTE)bShift;
            m_texFormat24.shifts[2] = (BYTE)gShift;
            m_texFormat24.shifts[3] = (BYTE)aShift;
                m_hasTexFormat24 = TRUE;
                return result;
            }
            if (bBits + aBits + gBits + rBits == 32) {
            m_texFormat32.format = *pddpf;
            m_texFormat32.gfxFormat = gfxFormat;
            m_texFormat32.bits[2] = (BYTE)gBits;
            m_texFormat32.bits[0] = (BYTE)rBits;
            m_texFormat32.bits[1] = (BYTE)bBits;
            m_texFormat32.bits[3] = (BYTE)aBits;
            m_texFormat32.shifts[0] = (BYTE)rShift;
            m_texFormat32.shifts[1] = (BYTE)bShift;
            m_texFormat32.shifts[2] = (BYTE)gShift;
            m_texFormat32.shifts[3] = (BYTE)aShift;
                m_hasTexFormat32 = TRUE;
                return result;
            }
        }
    } else {
        if (pddpf->flags & GFX_PF_BUMP) {
            pFormat = NULL;
            if (pddpf->bitsPerPixel == 16) {
                pFormat = &m_texFormatBump16;
                m_hasTexFormatBump16 = TRUE;
            } else if (pddpf->bitsPerPixel == 24 || pddpf->bitsPerPixel == 32) {
                pFormat = &m_texFormatBump32;
                m_hasTexFormatBump32 = TRUE;
            }
            pFormat->format = *pddpf;
            pFormat->gfxFormat = gfxFormat;
            bits = pddpf->redMask;
            rBits = 0;
            for (i = 32; i != 0; i--) {
                if (bits & 1)
                    rBits++;
                bits >>= 1;
            }
        pFormat->bits[0] = (BYTE)rBits;
            bits = pddpf->greenMask;
            rBits = 0;
            for (i = 32; i != 0; i--) {
                if (bits & 1)
                    rBits++;
                bits >>= 1;
            }
        pFormat->bits[1] = (BYTE)rBits;
            bits = pddpf->blueMask;
            rBits = 0;
            for (i = 32; i != 0; i--) {
                if (bits & 1)
                    rBits++;
                bits >>= 1;
            }
        pFormat->bits[2] = (BYTE)rBits;
            bits = pddpf->redMask;
            for (rShift = 0; rShift < 32; rShift++) {
                if (bits & 1)
                    break;
                bits >>= 1;
            }
        pFormat->shifts[0] = (BYTE)rShift;
            bits = pddpf->greenMask;
            for (rShift = 0; rShift < 32; rShift++) {
                if (bits & 1)
                    break;
                bits >>= 1;
            }
        pFormat->shifts[1] = (BYTE)rShift;
            bits = pddpf->blueMask;
            for (rShift = 0; rShift < 32; rShift++) {
                if (bits & 1)
                    break;
                bits >>= 1;
            }
        pFormat->shifts[2] = (BYTE)rShift;

            return result;
        }

        if (pddpf->flags & GFX_PF_FOURCC) {
            if (pddpf->fourCC == MAKEFOURCC('D', 'X', 'T', '1')) {
                m_texFormatDXT1_16.format = *pddpf;
            m_texFormatDXT1_16.gfxFormat = gfxFormat;
                m_hasTexFormatDXT1_16 = TRUE;
                m_texFormatDXT1_32.format = *pddpf;
            m_texFormatDXT1_32.gfxFormat = gfxFormat;
                m_hasTexFormatDXT1_32 = TRUE;
            } else if (pddpf->fourCC == MAKEFOURCC('D', 'X', 'T', '5')) {
                m_texFormatDXT5_16.format = *pddpf;
            m_texFormatDXT5_16.gfxFormat = gfxFormat;
                m_hasTexFormatDXT5_16 = TRUE;
                m_texFormatDXT5_32.format = *pddpf;
            m_texFormatDXT5_32.gfxFormat = gfxFormat;
                m_hasTexFormatDXT5_32 = TRUE;
                return result;
            }
        }
    }
    return result;
}

// FUNCTION: CMR2 0x004a8fb0
// PORT: the formats come from the renderer.
void CGraphics::SelectTextureFormats(void)
{
    memset(&m_texFormat16, 0, sizeof(TextureFormat));
    memset(&m_texFormatDXT1_16, 0, sizeof(TextureFormat));
    memset(&m_texFormat16Alpha, 0, sizeof(TextureFormat));
    memset(&m_texFormatDXT5_16, 0, sizeof(TextureFormat));
    memset(&m_texFormat24, 0, sizeof(TextureFormat));
    memset(&m_texFormatDXT1_32, 0, sizeof(TextureFormat));
    memset(&m_texFormat32, 0, sizeof(TextureFormat));
    memset(&m_texFormatDXT5_32, 0, sizeof(TextureFormat));
    memset(&m_texFormatBump16, 0, sizeof(TextureFormat));
    memset(&m_texFormatBump32, 0, sizeof(TextureFormat));
    m_hasTexFormat16 = FALSE;
    m_hasTexFormatDXT1_16 = FALSE;
    m_hasTexFormat16Alpha = FALSE;
    m_hasTexFormatDXT5_16 = FALSE;
    m_hasTexFormat24 = FALSE;
    m_hasTexFormatDXT1_32 = FALSE;
    m_hasTexFormat32 = FALSE;
    m_hasTexFormatDXT5_32 = FALSE;
    m_hasTexFormatBump16 = FALSE;
    m_hasTexFormatBump32 = FALSE;

    {
        static const int formats[] = { GFX_FORMAT_BGRX8, GFX_FORMAT_BGRA8, GFX_FORMAT_BC1, GFX_FORMAT_BC3,
                                       GFX_FORMAT_BUMP_DUDVL };
        GfxPixelFormat format;
        unsigned int i;

        for (i = 0; i < sizeof(formats) / sizeof(formats[0]); i++) {
            Gfx_GetPixelFormat(formats[i], &format);
            EnumTextureFormatsCallback(formats[i], &format);
        }
    }

    if (m_texFormat24.bits[0] != 8) {
        m_hasTexFormat24 = TRUE;
        m_texFormat24 = m_texFormat32;
    }
    if (m_hasTexFormatBump16 && !m_hasTexFormatBump32) {
        m_hasTexFormatBump32 = TRUE;
        m_texFormatBump32 = m_texFormatBump16;
        m_texFormatBump32.format = m_texFormatBump16.format;
    }

    if (g_pGraphics->depth != 16) {
        if (g_pGraphics->depth == 32) {
            m_pTextureManager->textureInfo1 = &m_texFormat24;
            m_pTextureManager->textureInfo2 = &m_texFormat32;
            m_pTextureManager->textureInfo3 = &m_texFormatDXT1_32;
            m_pTextureManager->textureInfo4 = &m_texFormatDXT5_32;
            if (m_hasTexFormatBump32)
                m_pTextureManager->textureInfo5 = &m_texFormatBump32;
        }
    } else {
        m_pTextureManager->textureInfo1 = &m_texFormat16;
        m_pTextureManager->textureInfo2 = &m_texFormat16Alpha;
        m_pTextureManager->textureInfo3 = &m_texFormatDXT1_16;
        m_pTextureManager->textureInfo4 = &m_texFormatDXT5_16;
        if (m_hasTexFormatBump16)
            m_pTextureManager->textureInfo5 = &m_texFormatBump16;
    }
}

// Allocates the shared vertex buffers: 100 small ones plus three larger
// ones, bumping the vertex memory counter after each allocation.
// FUNCTION: CMR2 0x004b1980
// PORT: renderer vertex buffers (the vertex format is fixed: FVF 0x2d2).
void CGraphics::AllocateSharedVertexBuffers(void)
{
    int i;

    for (i = 0; i < 100; i++) {
        m_pTextureManager->pVertexBuffers[i] = Gfx_CreateVertexBuffer(2000);
        m_pTextureManager->vertexBufferFill[i] = 0;
        m_unk0x006dd890 += 0x17700;
    }

    m_pTextureManager->pVertexBuffer1 = Gfx_CreateVertexBuffer(0xC000);
    m_unk0x006dd890 += 0x48000;

    m_pTextureManager->pVertexBuffer2 = Gfx_CreateVertexBuffer(0x7080);
    m_unk0x006dd890 += 0x151800;

    m_pTextureManager->pVertexBuffer3 = Gfx_CreateVertexBuffer(1);
    m_unk0x006dd890 += 0x30;
}

// FUNCTION: CMR2 0x0049df90
void CGraphics::ConfigureDefaultRenderStates(BOOL param1, int param2)
{
	// PORT: fill mode, shading, dithering, antialiasing, perspective
	// correction, stipple, colour key and clipping are fixed in the renderer.
	Gfx_SetDepthFunc(GFX_CMP_LESSEQUAL);
	Gfx_SetAlphaRef(1);
	Gfx_SetAlphaTest(1);
	Gfx_SetAlphaFunc(GFX_CMP_GREATEREQUAL);
	Gfx_SetAlphaBlend(1);
	Gfx_SetFog(0);
	Gfx_SetSpecular(0);
	Gfx_SetColorVertex(1);
	Gfx_SetDiffuseMaterialSource(GFX_MCS_MATERIAL);
	Gfx_SetSpecularMaterialSource(GFX_MCS_MATERIAL);
	Gfx_SetAmbientMaterialSource(GFX_MCS_MATERIAL);
	Gfx_SetEmissiveMaterialSource(GFX_MCS_COLOR1);

	SetZEnable(1);
	SetZWriteEnable(1);
	SetCullMode(CGame::GetSectorDrawState());

	Gfx_SetStageAddress(0, GFX_ADDRESS_CLAMP);
	Gfx_SetStageAddress(1, GFX_ADDRESS_CLAMP);
	Gfx_SetStageAddress(2, GFX_ADDRESS_CLAMP);
	Gfx_SetStageMagFilter(0, GFX_FILTER_LINEAR);
	Gfx_SetStageMagFilter(1, GFX_FILTER_LINEAR);
	Gfx_SetStageMagFilter(2, GFX_FILTER_LINEAR);

	SetTextureAddressClamp(1);
}

// FUNCTION: CMR2 0x004a8450
// PORT: the renderer creates its own depth buffer and device (at
// Gfx_SetVideoMode); what is left is the original's bookkeeping and the
// state set-up.
BOOL CGraphics::CreateDirect3DDevice(int param1, int param2, int param3)
{
    if (m_unk0x00520b7c != 0) {
        m_unk0x0072d56c = 0;
        CGame::RegisterCallback(ReleaseDirect3D, NULL);
        CGame::RegisterCallback(FreeTextureBuffers, NULL);
        CGame::RegisterCallback(EvictManagedTextureResources, NULL);
    }
    m_unk0x00660bfc = TRUE;

    CacheRenderDeviceCapabilities();
    SelectTextureFormats();
    ConfigureDefaultRenderStates(m_unk0x00520b7c, 1);
    AllocateSharedVertexBuffers();
    m_unk0x00520b7c = FALSE;
    return TRUE;
}

// FUNCTION: CMR2 0x004a8890
void CGraphics::SetProjection(int fovX, int fovY, int farPlane, int nearPlane)
{
    float scale;
    float range;
    GfxMatrix matrix;
    GfxMatrix tmp;

    scale = m_projectionScale;
    if (fovX != 0)
        m_fovX = X87S_MUL(X87S_FILD(float, fovX), m_oneOver65536);
    if (fovY != 0)
        m_fovY = X87S_MUL(X87S_FILD(float, fovY), m_oneOver65536);
    if (farPlane != 0)
        m_farPlane = X87S_MUL(X87S_FILD(float, farPlane), m_oneOver65536);
    if (nearPlane != 0)
        m_nearPlane = X87S_MUL(X87S_FILD(float, nearPlane), m_oneOver65536);

    matrix._12 = 0.0f;
    matrix._13 = 0.0f;
    range = m_farPlane - m_nearPlane;
    m_projection33 = m_farPlane / range;
    matrix._14 = 0.0f;
    matrix._21 = 0.0f;
    matrix._23 = 0.0f;
    matrix._24 = 0.0f;
    matrix._31 = 0.0f;
    matrix._32 = 0.0f;
    matrix._34 = 1.0f;
    matrix._41 = 0.0f;
    matrix._42 = 0.0f;
    matrix._44 = 0.0f;
    m_projection43 = -(m_nearPlane * m_farPlane) / range;
    matrix._33 = m_projection33;
    m_projection11 = m_fovX * scale;
    matrix._11 = m_projection11;
    matrix._22 = m_projection22 = scale * m_fovY;
    matrix._43 = m_projection43;
    m_pTextureManager->fixedProjection[0] = (int)FTOI(__int64, X87S_MUL(m_projection11, m_65536));
    m_pTextureManager->fixedProjection[1] = 0;
    m_pTextureManager->fixedProjection[2] = 0;
    m_pTextureManager->fixedProjection[3] = 0;
    m_pTextureManager->fixedProjection[4] = 0;
    m_pTextureManager->fixedProjection[5] = (int)FTOI(__int64, X87S_MUL(matrix._22, m_65536));
    m_pTextureManager->fixedProjection[6] = 0;
    m_pTextureManager->fixedProjection[7] = 0;
    m_pTextureManager->fixedProjection[8] = 0;
    m_pTextureManager->fixedProjection[8] = 0;
    m_pTextureManager->fixedProjection[10] = (int)FTOI(__int64, X87S_MUL(matrix._33, m_65536));
    m_pTextureManager->fixedProjection[11] = 0x10000;
    m_pTextureManager->fixedProjection[12] = 0;
    m_pTextureManager->fixedProjection[12] = 0;
    m_pTextureManager->fixedProjection[14] = (int)FTOI(__int64, X87S_MUL(matrix._43, m_65536));
    m_pTextureManager->fixedProjection[15] = 0;

    tmp = matrix;
    Gfx_SetTransform(GFX_TRANSFORM_PROJECTION, &tmp);
    m_farPlaneFixed = farPlane;
}

// FUNCTION: CMR2 0x004a5880
void CGraphics::GenerateBumpMap(Texture *pSrc, Texture *pDst)
{
    GfxLockedRect srcDesc;
    GfxLockedRect dstDesc;
    unsigned int duMask;
    unsigned int dvMask;
    unsigned int lumMask;
    WORD duDrop;
    WORD dvDrop;
    WORD lumDrop;
    WORD duShift;
    WORD dvShift;
    WORD lumShift;
    int x;
    int y;
    int h;
    int h1;
    int h2;
    int du;
    int dv;
    int lum;
    BYTE *pOut;
    WORD *pOut16;

    memset(&srcDesc, 0, sizeof(srcDesc));
    memset(&dstDesc, 0, sizeof(dstDesc));
    Gfx_LockTexture(pSrc->pSurface, 0, 0, &srcDesc);
    Gfx_LockTexture(pDst->pSurface, 0, 0, &dstDesc);

    if (dstDesc.format.bitsPerPixel != 16 && dstDesc.format.bitsPerPixel != 24 &&
        dstDesc.format.bitsPerPixel != 32) {
        Gfx_UnlockTexture(pSrc->pSurface, 0, 0);
        Gfx_UnlockTexture(pDst->pSurface, 0, 0);
        return;
    }

    duMask = dstDesc.format.redMask;
    dvMask = dstDesc.format.greenMask;
    lumMask = dstDesc.format.blueMask;
    duDrop = 8 - Bump_CountBits(duMask);
    dvDrop = 8 - Bump_CountBits(dvMask);
    lumDrop = 8 - Bump_CountBits(lumMask);
    duShift = Bump_LowestSetBit(duMask);
    dvShift = Bump_LowestSetBit(dvMask);
    lumShift = Bump_LowestSetBit(lumMask);

    pOut = (BYTE *)dstDesc.pixels;
    pOut16 = (WORD *)dstDesc.pixels;
    for (y = 0; y < pDst->height; y++) {
        for (x = 0; x < pDst->width; x++) {
            h = GetPixelRed(&srcDesc, x, y);
            if (x < pDst->width - 1)
                h1 = GetPixelRed(&srcDesc, x + 1, y);
            else
                h1 = GetPixelRed(&srcDesc, 0, y);
            if (y < pDst->height - 1)
                h2 = GetPixelRed(&srcDesc, x, y + 1);
            else
                h2 = GetPixelRed(&srcDesc, x, 0);
            du = abs(h - h1);
            dv = abs(h - h2);
            lum = GetPixelAlpha(&srcDesc, x, y);
            if (dstDesc.format.bitsPerPixel == 16) {
                *pOut16++ = (WORD)(((lum >> lumDrop) << lumShift) | ((dv >> dvDrop) << dvShift) | ((du >> duDrop) << duShift));
            } else if (dstDesc.format.bitsPerPixel == 24) {
                *pOut++ = (BYTE)du;
                *pOut++ = (BYTE)dv;
                *pOut++ = (BYTE)lum;
            } else {
                *pOut++ = (BYTE)du;
                *pOut++ = (BYTE)dv;
                *pOut++ = (BYTE)lum;
                *pOut++ = 0;
            }
        }
    }

    Gfx_UnlockTexture(pSrc->pSurface, 0, 0);
    Gfx_UnlockTexture(pDst->pSurface, 0, 0);
}

// FUNCTION: CMR2 0x004a7410
// PORT: creates the renderer texture in the format the original picked
// (flags 0x2000 DXT5, 0x4000 DXT1, 0x1 alpha, 0x20 bump, else opaque), with
// mip levels for flag 0x10 when mipmapping is on, as a render target for
// flag 0x4 and a cube map for 0x200. Plain offscreen surfaces (0x2) are
// CPU-side textures.
void CGraphics::CreateTextureSurface(Texture *pTexture, int width, int height, unsigned int flags)
{
    int format;
    int mipLevels;
    DWORD gfxFlags;

    gfxFlags = 0;
    if (flags & 0x2)
        gfxFlags |= GFX_TEXTURE_STAGING;
    if (flags & 0x8)
        gfxFlags |= GFX_TEXTURE_DYNAMIC;
    if (flags & 0x4)
        gfxFlags |= GFX_TEXTURE_RENDER_TARGET;
    if (flags & 0x200)
        gfxFlags = GFX_TEXTURE_RENDER_TARGET | GFX_TEXTURE_CUBE;
    mipLevels = 1;
    if ((flags & 0x10) && (g_pGraphics->field913_0x3bc & 3))
        mipLevels = SetMipMapCount(width, height);

    if ((flags & 0x2000) && m_pTextureManager->textureInfo4 != NULL) {
        format = m_pTextureManager->textureInfo4->gfxFormat;
    } else if ((flags & 0x4000) && m_pTextureManager->textureInfo3 != NULL) {
        format = m_pTextureManager->textureInfo3->gfxFormat;
    } else if ((flags & 0x1) && m_pTextureManager->textureInfo2 != NULL) {
        format = m_pTextureManager->textureInfo2->gfxFormat;
    } else if ((flags & 0x20) && m_hasTexFormatBump16 && m_hasTexFormatBump32) {
        format = m_pTextureManager->textureInfo5->gfxFormat;
    } else {
        format = m_pTextureManager->textureInfo1->gfxFormat;
    }

    if (width != 2 && width != 4 && width != 8 && width != 16 && width != 32 && width != 64 &&
        width != 128 && width != 256 && width != 512 && width != 1024 && width != 2048)
        _splitpath(pTexture->name, NULL, NULL, CFrontend::m_stringDest, NULL);
    if (height != 2 && height != 4 && height != 8 && height != 16 && height != 32 && height != 64 &&
        height != 128 && height != 256 && height != 512 && height != 1024 && height != 2048)
        _splitpath(pTexture->name, NULL, NULL, CFrontend::m_stringDest, NULL);

    pTexture->pSurface = Gfx_CreateTexture(width, height, format, mipLevels, gfxFlags);
    pTexture->field_0x11c = 0;
    pTexture->field_0x11e = 0;
    pTexture->width = (short)width;
    pTexture->height = (short)height;
    pTexture->bitsPerPixel = (short)g_pGraphics->depth;
}

// Ported from upstream branch add/FUN_004a8450 (Matt Hadden)
// FUNCTION: CMR2 0x004b74b0
BOOL CGraphics::GetRasterCapabilityField84(void) {
  return m_d3dDeviceDesc7.field0x84;
}

// FUNCTION: CMR2 0x004b74c0
DWORD Graphics_GetDeviceCaps90(void)
{
    return CGraphics::m_d3dDeviceDesc7.field0x90;
}

// FUNCTION: CMR2 0x004b74d0
DWORD Graphics_GetDeviceCaps94(void)
{
    return CGraphics::m_d3dDeviceDesc7.field0x94;
}

// FUNCTION: CMR2 0x004b74e0
DWORD Graphics_GetDeviceCaps3C(void)
{
    return CGraphics::m_d3dDeviceDesc7.field0x3c;
}

// FUNCTION: CMR2 0x004b74f0
DWORD Graphics_GetDeviceCaps40(void)
{
    return CGraphics::m_d3dDeviceDesc7.field0x40;
}

// FUNCTION: CMR2 0x004b7500
DWORD Graphics_GetDeviceCaps44(void)
{
    return CGraphics::m_d3dDeviceDesc7.field0x44;
}

// FUNCTION: CMR2 0x004b7510
DWORD Graphics_GetDeviceCaps64(void)
{
    return CGraphics::m_d3dDeviceDesc7.field0x64;
}

// FUNCTION: CMR2 0x004b7530
DWORD Graphics_GetDeviceCaps98(void)
{
    return CGraphics::m_d3dDeviceDesc7.field0x98;
}

// FUNCTION: CMR2 0x004b7540
DWORD Graphics_GetDeviceCaps9C(void)
{
    return CGraphics::m_d3dDeviceDesc7.field0x9c;
}

// FUNCTION: CMR2 0x004b7550
DWORD Graphics_GetDeviceCapsA4(void)
{
    return CGraphics::m_d3dDeviceDesc7.field0xa4;
}

// FUNCTION: CMR2 0x004b7210
// PORT: the capabilities the renderer offers, written as the Direct3D 7
// device description of a hardware transform-and-lighting card (two
// simultaneous textures, bump env map luminance, cube maps, 24-bit depth
// with stencil); the decoding below is the original's.
void CGraphics::CacheRenderDeviceCapabilities(void) {
    struct {
        DWORD dwDevCaps;
        struct {
            DWORD dwMiscCaps, dwRasterCaps, dwZCmpCaps, dwSrcBlendCaps, dwDestBlendCaps, dwAlphaCmpCaps;
            DWORD dwShadeCaps, dwTextureCaps, dwTextureFilterCaps, dwTextureBlendCaps, dwTextureAddressCaps;
        } dpcTriCaps;
        DWORD dwDeviceZBufferBitDepth;
        DWORD dwMinTextureWidth, dwMinTextureHeight, dwMaxTextureWidth, dwMaxTextureHeight;
        DWORD dwStencilCaps;
        DWORD dwTextureOpCaps;
        WORD wMaxTextureBlendStages;
        WORD wMaxSimultaneousTextures;
    } d3ddesc;

    memset(&m_d3dDeviceDesc7, 0, sizeof(Unk0x006e0bb0));
    d3ddesc.dwDevCaps = 0x200 | 0x1000 | 0x8000 | 0x10000 | 0x80000;
    d3ddesc.dpcTriCaps.dwMiscCaps = 0xff;
    d3ddesc.dpcTriCaps.dwRasterCaps = 0xffffffff & ~0x8000;   // has a z-buffer (not ZBUFFERLESSHSR)
    d3ddesc.dpcTriCaps.dwZCmpCaps = 0xff;
    d3ddesc.dpcTriCaps.dwSrcBlendCaps = 0x1fff;
    d3ddesc.dpcTriCaps.dwDestBlendCaps = 0x1fff;
    d3ddesc.dpcTriCaps.dwAlphaCmpCaps = 0xff;
    d3ddesc.dpcTriCaps.dwShadeCaps = 0xffffffff;
    d3ddesc.dpcTriCaps.dwTextureCaps = 0x1 | 0x2 | 0x4 | 0x8 | 0x800;  // not SQUAREONLY
    d3ddesc.dpcTriCaps.dwTextureFilterCaps = 0x3f;
    d3ddesc.dpcTriCaps.dwTextureBlendCaps = 0xff;
    d3ddesc.dpcTriCaps.dwTextureAddressCaps = 0x7;
    d3ddesc.dwDeviceZBufferBitDepth = 0x400 | 0x200 | 0x100;
    d3ddesc.dwMinTextureWidth = 1;
    d3ddesc.dwMinTextureHeight = 1;
    d3ddesc.dwMaxTextureWidth = 2048;
    d3ddesc.dwMaxTextureHeight = 2048;
    d3ddesc.dwStencilCaps = 0xff;
    d3ddesc.dwTextureOpCaps = 0xffffffff;
    d3ddesc.wMaxTextureBlendStages = 8;
    d3ddesc.wMaxSimultaneousTextures = 2;

    if ((d3ddesc.dwDevCaps & 0x100) != 0) {
        m_d3dDeviceDesc7.flag100 = 1;
    }

    if ((d3ddesc.dwDevCaps & 0x200) != 0) {
        m_d3dDeviceDesc7.flag200 = 1;
    }

    if ((d3ddesc.dwDevCaps & 0x1000) != 0) {
        m_d3dDeviceDesc7.flag1000 = 1;
    }

    m_d3dDeviceDesc7.field0x80 = d3ddesc.dwDeviceZBufferBitDepth;
    if ((d3ddesc.dpcTriCaps.dwRasterCaps & 0x8000) != 0) {
        m_d3dDeviceDesc7.field0x84 = 1;
    }

    if (((BYTE)d3ddesc.dpcTriCaps.dwZCmpCaps & 0x80) != 0) {
        m_d3dDeviceDesc7.field0x88 = 1;
    }

    if ((d3ddesc.wMaxSimultaneousTextures > 1) && (d3ddesc.dwTextureOpCaps & 8) != 0) {
        m_d3dDeviceDesc7.field0x8c = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwTextureAddressCaps & 4) != 0) {
        m_d3dDeviceDesc7.field0x1c = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwTextureFilterCaps & 1) != 0) {
        m_d3dDeviceDesc7.field0x20 = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwTextureFilterCaps & 2) != 0) {
        m_d3dDeviceDesc7.field0x2c = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwTextureFilterCaps & 0x20) != 0) {
        m_d3dDeviceDesc7.field0x30 = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwTextureCaps & 1) != 0) {
        m_d3dDeviceDesc7.field0x24 = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwTextureCaps & 0x20) != 0) {
        m_d3dDeviceDesc7.field0x28 = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwTextureCaps & 8) != 0) {
        m_d3dDeviceDesc7.field0x34 = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwTextureBlendCaps & 0x80) != 0) {
        m_d3dDeviceDesc7.field0x48 = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwTextureBlendCaps & 0x40) != 0) {
        m_d3dDeviceDesc7.field0x4c = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwTextureBlendCaps & 1) != 0) {
        m_d3dDeviceDesc7.field0x50 = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwTextureBlendCaps & 4) != 0) {
        m_d3dDeviceDesc7.field0x54 = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwTextureBlendCaps & 2) != 0) {
        m_d3dDeviceDesc7.field0x58 = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwTextureBlendCaps & 8) != 0) {
        m_d3dDeviceDesc7.field0x5c = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwTextureBlendCaps & 0x20) != 0) {
        m_d3dDeviceDesc7.field0x60 = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwAlphaCmpCaps & 0x40) != 0) {
        m_d3dDeviceDesc7.field0x64 = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwTextureCaps & 4) != 0) {
        m_d3dDeviceDesc7.field0x68 = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwRasterCaps & 0x200) != 0) {
        m_d3dDeviceDesc7.field0x6c = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwShadeCaps & 0x1000) != 0) {
        m_d3dDeviceDesc7.field0x70 = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwShadeCaps & 0x2000) != 0) {
        m_d3dDeviceDesc7.field0x74 = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwShadeCaps & 0x4000) != 0) {
        m_d3dDeviceDesc7.field0x78 = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwShadeCaps & 0x8000) != 0) {
        m_d3dDeviceDesc7.field0x7c = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwSrcBlendCaps & 0x10) != 0 &&
        (d3ddesc.dpcTriCaps.dwDestBlendCaps & 0x20) != 0) {
        m_d3dDeviceDesc7.field0x38 = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwSrcBlendCaps & 2) != 0 &&
        (d3ddesc.dpcTriCaps.dwDestBlendCaps & 2) != 0) {
        m_d3dDeviceDesc7.field0x3c = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwSrcBlendCaps & 4) != 0 &&
        (d3ddesc.dpcTriCaps.dwDestBlendCaps & 2) != 0) {
        m_d3dDeviceDesc7.field0x40 = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwSrcBlendCaps & 2) != 0 &&
        (d3ddesc.dpcTriCaps.dwDestBlendCaps & 8) != 0) {
        m_d3dDeviceDesc7.field0x44 = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwMiscCaps & 0x40) != 0) {
        m_d3dDeviceDesc7.field0xc = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwMiscCaps & 0x20) != 0) {
        m_d3dDeviceDesc7.field0x8 = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwMiscCaps & 0x10) != 0) {
        m_d3dDeviceDesc7.field0x4 = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwMiscCaps & 0x80) != 0) {
        m_d3dDeviceDesc7.field0x90 = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwRasterCaps & 0x100) != 0) {
        m_d3dDeviceDesc7.field0x94 = 1;
    }

    if (d3ddesc.wMaxTextureBlendStages >= 2) {
        m_d3dDeviceDesc7.field0x98 = 1;
    }

    m_d3dDeviceDesc7.field0xa8 = 1;

    if ((d3ddesc.dwTextureOpCaps & 0x400000) != 0) {
        m_d3dDeviceDesc7.field0x9c = 1;
    }

    if (((d3ddesc.dwStencilCaps & 0x40) != 0) &&
        ((d3ddesc.dwStencilCaps & 0x80) != 0) &&
        ((d3ddesc.dwStencilCaps & 1) != 0) &&
        ((d3ddesc.dwStencilCaps & 4) != 0)) {
        m_d3dDeviceDesc7.field0xa0 = 1;
    }

    if ((d3ddesc.dpcTriCaps.dwTextureCaps & 0x800) != 0) {
        m_d3dDeviceDesc7.field0xa4 = 1;
    }

    m_d3dDeviceDesc7.minTextureWidth = d3ddesc.dwMinTextureWidth;
    m_d3dDeviceDesc7.minTextureHeight = d3ddesc.dwMinTextureHeight;
    m_d3dDeviceDesc7.maxTextureWidth = d3ddesc.dwMaxTextureWidth;
    m_d3dDeviceDesc7.maxTextureHeight = d3ddesc.dwMaxTextureHeight;
}

// FUNCTION: CMR2 0x0049d940
void CGraphics::SetClearColour(int unused, int r, int g, int b)
{
    ((BYTE *)&m_clearColour)[0] = r;
    ((BYTE *)&m_clearColour)[1] = g;
    ((BYTE *)&m_clearColour)[2] = b;
}

// FUNCTION: CMR2 0x0049d960
void CGraphics::ClearTarget(void)
{

    Gfx_Clear(GFX_CLEAR_TARGET, GFX_RGBA(((BYTE *)&m_clearColour)[0], ((BYTE *)&m_clearColour)[1], ((BYTE *)&m_clearColour)[2], 0xff), 1.0f);
}

// FUNCTION: CMR2 0x0049d9d0
BOOL CGraphics::ClearZBuffer(void)
{
    DWORD flags;

    if (!GetRasterCapabilityField84()) {
        flags = GFX_CLEAR_ZBUFFER;
        if (g_pGraphics->field913_0x3bc & 0x20)
            flags = GFX_CLEAR_ZBUFFER | 0;
        Gfx_Clear(flags, 0xff000000, 1.0f);
    }
    return TRUE;
}

// Reloads every texture from its archive (.DDS first, else .TGA) and
// recreates the cube map surfaces, e.g. after the device was lost.
// match 89%: MSVC puts the LoadDDS branch inline here; the original defers it past
// the function epilogue (same CFG, different block order). No source shape tried reproduces it.
// FUNCTION: CMR2 0x004a4c40
void Graphics_ReloadAllTextures(void)
{
    Texture *pTexture;
    char *pExt;
    void *pData;
    unsigned int i;

    for (i = 0; i < CGraphics::m_textureCount; i++) {
        pTexture = CGraphics::m_pTextureManager->textureBuffer[i];
        if (pTexture->textureId == i) {
            pExt = &pTexture->name[strlen(pTexture->name) - 4];
            strncpy(pExt, CGraphics::m_ddsExtension, 4);
            pData = CGenericFileLoader::FindFile((GenericFile *)pTexture->pArchive, pTexture->name, 0, 0, 0);
            if (pData == NULL) {
                strncpy(pExt, CGraphics::m_tgaExtension, 4);
                pData = CGenericFileLoader::FindFile((GenericFile *)pTexture->pArchive, pTexture->name, 0, 0, 0);
                CGraphics::LoadTGATexture((BYTE *)pData, pTexture);
            } else {
                CGraphics::LoadDDSTexture((DDSFile *)pData, pTexture);
            }
            France1Textures_Apply(pTexture,pData,CGenericFileLoader::GetGenericFileSize());
        }
    }
    for (i = 0; i < CGraphics::m_unk0x0065fa28; i++)
        CGraphics::CreateCubeMapSurfaces((RenderTexture *)CGraphics::m_pTextureManager->textureBuffer2[i]);
}

// Whether a 2D point lies inside a triangle (x0, y0, x1, y1, x2, y2), all
// 16.16; the edge tests work on values scaled by 0.01 to avoid overflow.
// FUNCTION: CMR2 0x0049da50
int Tri2D_Contains(int *pPoint, int *pTri)
{
    int normal[3][2];
    int edge[3][2];
    int side0;
    int side1;
    int side2;

    if ((pPoint[0] < pTri[0] && pPoint[0] < pTri[2] && pPoint[0] < pTri[4]) ||
        (pPoint[0] > pTri[0] && pPoint[0] > pTri[2] && pPoint[0] > pTri[4]))
        return 0;
    if ((pPoint[1] < pTri[1] && pPoint[1] < pTri[3] && pPoint[1] < pTri[5]) ||
        (pPoint[1] > pTri[1] && pPoint[1] > pTri[3] && pPoint[1] > pTri[5]))
        return 0;
    edge[0][0] = pTri[2] - pTri[0];
    edge[0][1] = pTri[3] - pTri[1];
    edge[1][0] = pTri[4] - pTri[2];
    edge[1][1] = pTri[5] - pTri[3];
    edge[2][0] = pTri[0] - pTri[4];
    edge[2][1] = pTri[1] - pTri[5];
    normal[0][0] = FixMul(edge[0][1], 0x28f);
    normal[0][1] = -FixMul(edge[0][0], 0x28f);
    normal[1][0] = FixMul(edge[1][1], 0x28f);
    normal[1][1] = -FixMul(edge[1][0], 0x28f);
    normal[2][0] = FixMul(edge[2][1], 0x28f);
    normal[2][1] = -FixMul(edge[2][0], 0x28f);
    edge[0][0] = FixMul(pPoint[0] - pTri[0], 0x28f);
    edge[0][1] = FixMul(pPoint[1] - pTri[1], 0x28f);
    edge[1][0] = FixMul(pPoint[0] - pTri[2], 0x28f);
    edge[1][1] = FixMul(pPoint[1] - pTri[3], 0x28f);
    edge[2][0] = FixMul(pPoint[0] - pTri[4], 0x28f);
    edge[2][1] = FixMul(pPoint[1] - pTri[5], 0x28f);
    side0 = FixMul(edge[0][0], normal[0][0]) + FixMul(edge[0][1], normal[0][1]);
    side1 = FixMul(edge[1][0], normal[1][0]) + FixMul(edge[1][1], normal[1][1]);
    side2 = FixMul(edge[2][0], normal[2][0]) + FixMul(edge[2][1], normal[2][1]);
    if (side0 >= 0 && side1 >= 0 && side2 >= 0)
        return 1;
    return 0;
}

// FUNCTION: CMR2 0x0049dc70
void CGraphics::SetCullMode(int mode)
{
    if (mode != m_cullMode) {
        Gfx_SetCullMode(mode);
        m_cullMode = mode;
    }
}

// GLOBAL: CMR2 0x0059ce30
int g_unk0x0059ce30;

// Switches alpha blending; with alpha test support the reference value follows.
// FUNCTION: CMR2 0x0049dcc0
void Graphics_SwitchAlphaBlendAndTest(int enable)
{
    if (enable != g_unk0x0059ce30) {
        Gfx_SetAlphaBlend(enable);
        if (Graphics_GetDeviceCaps64()) {
            if (enable != 0) {
                Gfx_SetAlphaRef(1);
            } else {
                Gfx_SetAlphaRef(0x80);
            }
            g_unk0x0059ce30 = enable;
            return;
        }
        Gfx_SetAlphaBlend(1);
        g_unk0x0059ce30 = enable;
    }
}

// FUNCTION: CMR2 0x0049dd40
void CGraphics::SetZEnable(int enable)
{
    if (enable != m_zEnable) {
        Gfx_SetDepthTest(enable);
        m_zEnable = enable;
    }
}

// FUNCTION: CMR2 0x0049dd70
void CGraphics::SetTextureAddressClamp(int clamp)
{
    if (clamp != m_textureAddressClamp) {
        if (clamp != 0)
            Gfx_SetStageAddress(0, GFX_ADDRESS_CLAMP);
        else
            Gfx_SetStageAddress(0, GFX_ADDRESS_WRAP);
        m_textureAddressClamp = clamp;
    }
}

// FUNCTION: CMR2 0x0049ddc0
void CGraphics::SetZWriteEnable(int enable)
{
    if (enable != m_zWriteEnable) {
        Gfx_SetDepthWrite(enable);
        m_zWriteEnable = enable;
    }
}

// Current cached depth-write state, so scoped no-depth-write draws (cockpit
// window glass) restore what the caller had instead of a hardcoded 1.
int CGraphics::GetZWriteEnable(void)
{
    return m_zWriteEnable;
}

// FUNCTION: CMR2 0x0049ddf0
void CGraphics::SetTexCoordIndex(int stage, int index)
{
    Gfx_SetStageTexCoordIndex(stage, index);
    Gfx_SetStageTextureTransform(stage, 0);
    m_texCoordIndex[stage] = index;
}

int Args_Has(char *pArg);

extern int g_sceneStatCopied;
extern int g_sceneStatMultiplied;
extern int g_sceneStatClean;
extern int g_sceneStatHidden;
extern int g_fixMatrixMultiplyCount;

// Frame rate counter argument ("gamegauge"): when present the vsync wait is
// skipped.
// GLOBAL: CMR2 0x005207fc
char g_str0x005207fc[] = "gamegauge";

// Ends the frame: resets the per frame counters and presents the back buffer,
// which in windowed mode means blitting it into the client area of the game
// window and in fullscreen mode flipping (waiting for the vertical blank only
// when the frame rate counter is off).
// FUNCTION: CMR2 0x0049de40
// PORT: one present for both window modes; the original waited for the
// vertical blank unless the command line had the no-wait argument, which
// now combines with the video.vsync option.
void Graphics_PresentFrameAndResetCounters(void)
{
    CMain::UpdateFrameTime();
    g_sceneStatCopied = 0;
    g_sceneStatMultiplied = 0;
    g_sceneStatClean = 0;
    g_sceneStatHidden = 0;
    g_fixMatrixMultiplyCount = 0;
    CGraphics::m_unk0x0065fa24 = 0;
    Gfx_Present(Args_Has(g_str0x005207fc) == 0 && Sys_GetOption("video.vsync", 1) != 0);
}

// Drawing view of a sector's static stage objects (StageObject in Sector.h):
// the same layout as the copy Game.cpp keeps locally, for the same reason
// (touching Sector.h perturbs the codegen of unrelated files).
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
    int lightLevel;             // 0x9c
};

extern GfxMatrix g_unk0x00597cc0;
extern GfxMatrix g_identityWorldMatrix;
extern unsigned short g_visibleSectorIndices[];
GfxMatrix *FixMatrix_ToFloat(GfxMatrix *pOut, FixMatrix *pIn);
void Graphics_SetShadowGeometryState(int value);
void Game_QueueVisibleSectorNodes(int param1);
void Game_DrawSortedNodes(int bit);

// PORT: the faces of the cube map end at 80 m (120 m for the view-mask nodes)
// and see at most that times the square root of three away, but the original
// drew every sector of the view in all six, most of them clipped away. While
// the faces are drawn, the culled list keeps only the sectors whose ground
// rectangle is within 272 m (208 m plus room for the objects that reach out
// of their sector) of the cube's centre, and is restored after.
static unsigned short g_cubeSavedSectorIndices[4096];

static int Graphics_KeepCubeMapSectors(const FixVector *pCentre)
{
    int saved = g_sectorCullEnabled;
    int kept = 0;
    int cx = pCentre->x >> 16;
    int cz = pCentre->z >> 16;

    memcpy(g_cubeSavedSectorIndices, g_visibleSectorIndices, saved * sizeof(g_visibleSectorIndices[0]));
    for (int i = 0; i < saved; i++) {
        Sector *pSector = g_sectors[g_cubeSavedSectorIndices[i]];
        int minX = pSector->bounds[0][0] >> 16, maxX = minX;
        int minZ = pSector->bounds[0][1] >> 16, maxZ = minZ;
        for (int k = 1; k < 4; k++) {
            int x = pSector->bounds[k][0] >> 16;
            int z = pSector->bounds[k][1] >> 16;
            minX = x < minX ? x : minX;
            maxX = x > maxX ? x : maxX;
            minZ = z < minZ ? z : minZ;
            maxZ = z > maxZ ? z : maxZ;
        }
        int dx = cx < minX ? minX - cx : cx > maxX ? cx - maxX : 0;
        int dz = cz < minZ ? minZ - cz : cz > maxZ ? cz - maxZ : 0;
        if (dx * dx + dz * dz <= 272 * 272)
            g_visibleSectorIndices[kept++] = g_cubeSavedSectorIndices[i];
    }
    g_sectorCullEnabled = kept;
    return saved;
}

static void Graphics_RestoreCubeMapSectors(int saved)
{
    memcpy(g_visibleSectorIndices, g_cubeSavedSectorIndices, saved * sizeof(g_visibleSectorIndices[0]));
    g_sectorCullEnabled = saved;
}

// Scratch view matrix rebuilt for every cube map face (0x0049d3f0 uses it too).
// GLOBAL: CMR2 0x0059bd28
FixMatrix g_cubeFaceViewMatrix;

// Renders the scene into the six faces of the cube map the mesh of pNode
// belongs to: for each face it derives a view matrix from the node's transform,
// points the render target at that face and draws the culled sectors (ground
// meshes, static objects and view-mask nodes). Restores the transforms and the
// back buffer afterwards.
// FUNCTION: CMR2 0x0049e1f0
int Graphics_RenderNodeCubeMapFaces(SceneNode *pNode, int bit)
{
    GfxViewport viewport;
    int farPlane;
    int cubeIndex;
    GfxMatrix view;
    GfxMatrix projection;
    FixMatrix inverse;
    GfxMatrix floatMatrix;
    FixVector forward;
    FixVector tmp;
    FixMatrix *pCurrent;
    Mesh *pNodeMesh;
    Mesh *pMesh;
    StageObjectDraw *pObject;
    Sector *pSector;
    SceneNode *pChild;
    int i;
    unsigned int sectorIndex;
    int savedSectors;
    int objectDistance;

    farPlane = CGraphics::m_farPlaneFixed;
    pCurrent = &pNode->current;
    pNodeMesh = (Mesh *)pNode->pObject;
    cubeIndex = (pNodeMesh->flags >> 15) & 7;
    if (CGraphics::m_pTextureManager->textureBuffer2[cubeIndex] == NULL)
        return 1;

    Graphics_SetShadowGeometryState(1);
    Graphics_SwitchAlphaBlendAndTest(0);

    memset(&viewport, 0, sizeof(viewport));
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = CGraphics::m_cubeMapSize;
    viewport.height = CGraphics::m_cubeMapSize;
    viewport.minZ = 0.0f;
    viewport.maxZ = 1.0f;
    Gfx_SetViewport(&viewport);
    Gfx_GetTransform(GFX_TRANSFORM_VIEW, &view);
    Gfx_GetTransform(GFX_TRANSFORM_PROJECTION, &projection);
    // PORT: only the sectors the faces can reach (Graphics_KeepCubeMapSectors).
    savedSectors = Graphics_KeepCubeMapSectors(&pCurrent->position);

    for (i = 0; i < 6; i++) {
        FixMatrix face;
        switch (i) {
        case 0:
            face = *pCurrent;
            tmp = face.right;
            face.right.x = -face.forward.x;
            face.right.y = -face.forward.y;
            face.right.z = -face.forward.z;
            face.forward = tmp;
            break;
        case 1:
            face = *pCurrent;
            tmp = face.right;
            face.right = face.forward;
            face.forward.x = -tmp.x;
            face.forward.y = -tmp.y;
            face.forward.z = -tmp.z;
            break;
        case 2:
            face = *pCurrent;
            tmp = face.forward;
            face.forward = face.up;
            face.up.x = -tmp.x;
            face.up.y = -tmp.y;
            face.up.z = -tmp.z;
            break;
        case 3:
            face = *pCurrent;
            tmp = face.forward;
            face.forward.x = -face.up.x;
            face.forward.y = -face.up.y;
            face.forward.z = -face.up.z;
            face.up = tmp;
            break;
        case 4:
            face = *pCurrent;
            break;
        case 5:
            face = *pCurrent;
            face.right.x = -face.right.x;
            face.right.y = -face.right.y;
            face.right.z = -face.right.z;
            face.forward.x = -face.forward.x;
            face.forward.y = -face.forward.y;
            face.forward.z = -face.forward.z;
            break;
        }
        FixMatrix_Invert(&inverse, &face);
        FixMatrix_ToFloat(&floatMatrix, &inverse);
        Gfx_SetTransform(GFX_TRANSFORM_VIEW, &floatMatrix);
        forward.x = face.forward.x;
        forward.y = 0;
        forward.z = face.forward.z;
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
        Graphics_SetRenderTarget(&CGraphics::m_pTextureManager->textureBuffer2[cubeIndex][i]);
        Gfx_Clear(GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f);
        Gfx_BeginScene();
        CGraphics::SetProjection(0x20000, 0x20000, 0x780000, 0x1999);
        CGraphics::SetZEnable(0);
        CGraphics::SetZWriteEnable(0);
        for (sectorIndex = 0; sectorIndex < (unsigned int)g_sectorCullEnabled; sectorIndex++) {
            pChild = g_sectors[(unsigned short)g_visibleSectorIndices[sectorIndex]]->pFirstNode;
            while (pChild != NULL) {
                if (pChild->visible == 0)
                    Game_DrawViewMaskNode(pChild, 0);
                pChild = pChild->pNextInSector;
            }
        }
        CGraphics::SetZEnable(1);
        CGraphics::SetZWriteEnable(1);
        CGraphics::SetProjection(0x20000, 0x20000, 0x500000, 0x1999);
        for (sectorIndex = 0; sectorIndex < (unsigned int)g_sectorCullEnabled; sectorIndex++) {
            pSector = g_sectors[(unsigned short)g_visibleSectorIndices[sectorIndex]];
            pMesh = (Mesh *)pSector->pMesh;
            pObject = (StageObjectDraw *)pSector->pObjects;
            if (pMesh != NULL) {
                Gfx_SetTransform(GFX_TRANSFORM_WORLD, &g_identityWorldMatrix);
                Graphics_DrawMeshLOD(pMesh, 1, 0, 0);
                while (pObject != NULL) {
                    // PORT: the camera-facing objects within 20 m of the cube,
                    // each by its own distance; the original read the one kept
                    // in their shared mesh (Game_DrawDeferredObjects).
                    if ((pObject->pMesh->flags & 2) != 0) {
                        FixVector delta;
                        delta.x = pCurrent->position.x - pObject->position.x;
                        delta.y = 0;
                        delta.z = pCurrent->position.z - pObject->position.z;
                        objectDistance = FixVecLength(&delta);
                    } else {
                        objectDistance = pObject->pMesh->field_0x114;
                    }
                    if (objectDistance < 0x140000) {
                        if ((pObject->pMesh->flags & 2) != 0) {
                            float scale;

                            *(GfxMatrix *)pObject->matrix = g_unk0x00597cc0;
                            if (pObject->scaleX != 0x10000 || pObject->scaleY != 0x10000 ||
                                pObject->scaleZ != 0x10000) {
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
                            Gfx_SetTransform(GFX_TRANSFORM_WORLD, (GfxMatrix *)pObject->matrix);
                        } else {
                            Gfx_SetTransform(GFX_TRANSFORM_WORLD, &g_identityWorldMatrix);
                        }
                        Graphics_DrawMeshLOD(pObject->pMesh, 1, 0, 0);
                    }
                    pObject = pObject->pNext;
                }
            }
        }
        Game_QueueVisibleSectorNodes(bit);
        Game_DrawSortedNodes(bit);
        Gfx_EndScene();
    }
    Graphics_RestoreCubeMapSectors(savedSectors);
    CGraphics::SetProjection(0x20000, 0x20000, farPlane, 0x10000);
    // PORT: the original passed a Texture laid over g_graphics whose pSurface
    // is the back buffer.
    Graphics_SetRenderTarget(NULL);
    Gfx_SetTransform(GFX_TRANSFORM_VIEW, &view);
    Gfx_SetTransform(GFX_TRANSFORM_PROJECTION, &projection);
    Graphics_SwitchAlphaBlendAndTest(1);
    return 1;
}

// FUNCTION: CMR2 0x004a6e30
Texture *CGraphics::LoadDDSTexture(DDSFile *pDDS, Texture *pTexture)
{
    GfxLockedRect desc;
    DDSHeader header;
    GfxTexture *pTemp;
    int format;
    Texture tmpTexture;
    BYTE *pSrc;
    BYTE *pDst;
    unsigned int rowBytes;
    unsigned int y;
    int width;
    int height;

    height = pDDS->desc.dwHeight;
    width = pDDS->desc.dwWidth;
    if (width != 2 && width != 4 && width != 8 && width != 16 && width != 32 && width != 64 &&
        width != 128 && width != 256 && width != 512 && width != 1024 && width != 2048)
        _splitpath(pTexture->name, NULL, NULL, CFrontend::m_stringDest, NULL);
    if (height != 2 && height != 4 && height != 8 && height != 16 && height != 32 && height != 64 &&
        height != 128 && height != 256 && height != 512 && height != 1024 && height != 2048)
        _splitpath(pTexture->name, NULL, NULL, CFrontend::m_stringDest, NULL);

    header = pDDS->desc;
    if (header.ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '1'))
        pTexture->flags |= 0x4000;
    else if (header.ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '5'))
        pTexture->flags |= 0x2000;
    // PORT: the file's top level goes into a CPU-side texture of its format
    // (DXT1, DXT5 or 32-bit), which the renderer converts when copying.
    if (header.ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '1'))
        format = GFX_FORMAT_BC1;
    else if (header.ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '5'))
        format = GFX_FORMAT_BC3;
    else
        format = header.ddpfPixelFormat.dwRGBAlphaBitMask != 0 ? GFX_FORMAT_BGRA8 : GFX_FORMAT_BGRX8;
    pTemp = Gfx_CreateTexture(header.dwWidth, header.dwHeight, format, 1, GFX_TEXTURE_STAGING);
    Gfx_LockTexture(pTemp, 0, 0, &desc);
    pSrc = pDDS->data;
    if (header.dwFlags & DDS_LINEARSIZE) {
        memcpy(desc.pixels, pSrc, header.dwLinearSize);
    } else {
        // PORT: the original stepped the source by the height instead of
        // the row size (it only ever loaded compressed files).
        rowBytes = header.ddpfPixelFormat.dwRGBBitCount * header.dwWidth >> 3;
        pDst = (BYTE *)desc.pixels;
        for (y = 0; y < header.dwHeight; y++) {
            memcpy(pDst, pSrc, rowBytes);
            pDst += desc.pitch;
            pSrc += rowBytes;
        }
    }
    Gfx_UnlockTexture(pTemp, 0, 0);

    if (header.ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '1')) {
        if (!m_hasTexFormatDXT1_16 || !m_hasTexFormatDXT1_32)
            pTexture->flags &= ~0x4000;
        else
            pTexture->flags |= 0x4000;
    } else if (header.ddpfPixelFormat.dwFourCC == MAKEFOURCC('D', 'X', 'T', '5')) {
        if (!m_hasTexFormatDXT5_16 || !m_hasTexFormatDXT5_32)
            pTexture->flags = (pTexture->flags & ~0x6000) | 0x1;
        else
            pTexture->flags |= 0x2000;
    }

    if (strncmp(pTexture->name + strlen(pTexture->name) - 6, m_strSuffixBR, 2) == 0)
        pTexture->flags = (pTexture->flags & ~0x6000) | 0x1;
    if (strncmp(pTexture->name + strlen(pTexture->name) - 6, m_strSuffixRU, 2) == 0)
        pTexture->flags = (pTexture->flags & ~0x6000) | 0x1;
    if (m_unk0x00520b14 == 0) {
        if (strncmp(pTexture->name + strlen(pTexture->name) - 8, m_strSuffixBODF, 4) == 0)
            pTexture->flags = (pTexture->flags & ~0x6000) | 0x1;
    }
    if (strncmp(pTexture->name + strlen(pTexture->name) - 9, m_strSuffixDIGIT, 5) == 0)
        pTexture->flags = (pTexture->flags & ~0x6000) | 0x1;
    if (strncmp(pTexture->name + strlen(pTexture->name) - 9, m_strSuffixREVCT, 5) == 0)
        pTexture->flags = (pTexture->flags & ~0x6000) | 0x1;
    if (g_pGraphics->field913_0x3bc & 0x10) {
        if (strncmp(pTexture->name + strlen(pTexture->name) - 6, m_strSuffixBU, 2) == 0)
            pTexture->flags = (pTexture->flags & ~0x6001) | 0x20;
    }

    if (!(pTexture->flags & 0x20) || !(g_pGraphics->field913_0x3bc & 0x10)) {
        CreateTextureSurface(pTexture, pDDS->desc.dwWidth, pDDS->desc.dwHeight, pTexture->flags);
        Gfx_CopyTexture(pTexture->pSurface, pTemp);
        Gfx_DestroyTexture(pTemp);
        pTemp = NULL;
    } else {
        tmpTexture.pSurface = Gfx_CreateTexture(header.dwWidth, header.dwHeight, m_pTextureManager->textureInfo2->gfxFormat, 1,
                                                GFX_TEXTURE_STAGING);
        Gfx_CopyTexture(tmpTexture.pSurface, pTemp);
        CreateTextureSurface(pTexture, pDDS->desc.dwWidth, pDDS->desc.dwHeight, pTexture->flags);
        GenerateBumpMap(&tmpTexture, pTexture);
        Gfx_DestroyTexture(tmpTexture.pSurface);
        tmpTexture.pSurface = NULL;
        Gfx_DestroyTexture(pTemp);
        pTemp = NULL;
    }

    if ((g_pGraphics->field913_0x3bc & 3) && (pTexture->flags & 0x10))
        GetMipMapSurfaces(pTexture);
    if ((g_pGraphics->field913_0x3bc & 3) && (pTexture->flags & 0x10))
        m_unk0x0065fa2c += GetMipMapPixelCount(pTexture);
    m_unk0x0065fa2c += pDDS->desc.dwHeight * pDDS->desc.dwWidth;
    if ((g_pGraphics->field913_0x3bc & 3) && (pTexture->flags & 0x10))
        BltMipMaps(pTexture);
    if (!(pTexture->flags & 0x1000))
        CFileBuffer::FreeGenericFileBuffer(pDDS);
    VectorSprite_Replace(pTexture->name, pTexture->pSurface);
    return pTexture;
}

// GLOBAL: CMR2 0x005210b8
int g_unk0x005210b8 = -1;
// GLOBAL: CMR2 0x005210bc
int g_unk0x005210bc = 1;
// GLOBAL: CMR2 0x005210c0
int g_unk0x005210c0 = 1;
// GLOBAL: CMR2 0x005210c4
int g_unk0x005210c4 = 5;
// One of the 32 interpolation timers (0x30 bytes): a value that moves from
// start to end along a curve, one step every 16 ms.
struct InterpolationTimer {
    BYTE shape;             // 0x00 curve 0-4; 5 = free or finished
    signed char direction;  // 0x01 sign of scale (wobble at the end of curve 4)
    BYTE pad_0x2[2];
    unsigned int length;    // 0x04 steps of the curve
    int base;               // 0x08 value the curve starts from
    int end;                // 0x0c value once finished
    int start;              // 0x10 value while delayed
    BYTE holdAtEnd;         // 0x14 still running for 3 steps past length
    BYTE pad_0x15[3];
    BYTE bitUnion;          // 0x18 Graphics_GetTimerRecordBitUnion at start
    BYTE pad_0x19[3];
    int scale;              // 0x1c curve slope, 4096 = 1
    BYTE *pOwner;           // 0x20 slot byte the timer was registered with
    unsigned int delay;     // 0x24 steps left before it starts
    unsigned int step;      // 0x28
    int lastTime;           // 0x2c frame time of the last step
};
CMR2_LAYOUT_CHECK(InterpolationTimerSize, sizeof(InterpolationTimer) == 0x30);
CMR2_LAYOUT_CHECK(InterpolationTimerOwner, offsetof(InterpolationTimer, pOwner) == 0x20);

struct TimerPulseState {
    InterpolationTimer timers[32];
    int rising;
    float maximum;
};
CMR2_LAYOUT_CHECK(TimerPulseStateSize, sizeof(TimerPulseState) == 0x608);
// GLOBAL: CMR2 0x00521138
TimerPulseState g_timerPulseState = { {
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 },
    { 5 }
}, 1, 0.3f };
#define g_pulseRising (g_timerPulseState.rising)
#define g_pulseMax (g_timerPulseState.maximum)

// FUNCTION: CMR2 0x004b2970
void Graphics_SetShadowGeometryState(int value)
{
    g_unk0x005210bc = value;
}

// GLOBAL: CMR2 0x005210d0
float g_unk0x005210d0 = 0.5f;
// GLOBAL: CMR2 0x006dfdf8
float g_unk0x006dfdf8;

FixMatrix *FloatMatrix_ToFix(FixMatrix *pOut, GfxMatrix *pIn);
GfxMatrix *FixMatrix_ToFloat(GfxMatrix *pOut, FixMatrix *pIn);
void Frontend_SetObjectField118(struct Texture *pTexture, int value);
extern unsigned int g_carReflectionCubeMaps[20];
extern unsigned short g_unk0x006dd9bc[2000];

// Draws the triangles of a mesh in contiguous texture runs, with the reserved
// cube map of the mesh projected on them: the inverse of the view * world
// matrix (both translations removed) becomes the texture transform of stage 0,
// so the texture coordinates are generated from the camera space position.
// FUNCTION: CMR2 0x004b2980
void Mesh_DrawEnvMapped(Mesh *pMesh)
{
    GfxMatrix world;
    GfxMatrix view;
    FixMatrix worldFix;
    FixMatrix combined;
    FixMatrix inverse;
    FixMatrix viewFix;
    GfxMatrix projected;
    int triangleCount;
    int count;
    int currentTexture;
    int i;
    int textureIndex;
    int texture;

    count = 0;
    triangleCount = pMesh->triangleCount;
    currentTexture = -1;
    if ((*(BYTE *)&g_pGraphics->field913_0x3bc & 0x80) != 0) {
        if ((int)g_carReflectionCubeMaps[(pMesh->flags >> 15) & 7] < 0) {
            Graphics_DrawCubeMappedShadowTriangles(pMesh);
            return;
        }
        Gfx_GetTransform(GFX_TRANSFORM_VIEW, &view);
        Gfx_GetTransform(GFX_TRANSFORM_WORLD, &world);
        view._41 = view._42 = view._43 = view._44 = 0.0f;
        world._41 = world._42 = world._43 = world._44 = 0.0f;
        FloatMatrix_ToFix(&worldFix, &world);
        FloatMatrix_ToFix(&viewFix, &view);
        FixMatrix_Multiply(&combined, &worldFix, &viewFix);
        FixMatrix_Invert(&inverse, &combined);
        FixMatrix_ToFloat(&projected, &inverse);
        Gfx_SetTransform(GFX_TRANSFORM_TEXTURE0, &projected);
        Gfx_SetNormalizeNormals(1);
        Gfx_SetLocalViewer(1);
        Graphics_SwitchAlphaBlendAndTest(1);
        CGraphics::SetTextureAddressClamp(0);
        Frontend_SetObjectField118(
            CGraphics::m_pTextureManager->textureBuffer2[g_carReflectionCubeMaps[(pMesh->flags >> 15) & 7]],
            0xb);
        CGraphics::ApplyTextureStageChange(
            0, CGraphics::m_pTextureManager->textureBuffer2[g_carReflectionCubeMaps[(pMesh->flags >> 15) & 7]]);
        if ((g_unk0x005210b8 >= 0 || (int)g_carReflectionCubeMaps[(pMesh->flags >> 15) & 7] >= 0) &&
            triangleCount > 0) {
            for (i = 0; i < triangleCount; i++) {
                textureIndex = pMesh->pTriangles[i].field_0x30;
                if (currentTexture != pMesh->pTriangles[i].textures[textureIndex]) {
                    if (count > 0) {
                        Gfx_DrawIndexedPrimitiveVB(GFX_TRIANGLELIST, CGraphics::m_pTextureManager->pVertexBuffers[pMesh->vertexBufferIndex], pMesh->vertexOffset, pMesh->field_0x10, g_unk0x006dd9bc, count);
                    }
                    count = 0;
                    currentTexture = pMesh->pTriangles[i].textures[textureIndex];
                    if (currentTexture > -1) {
                        Frontend_SetObjectField118(
                            CGraphics::m_pTextureManager->textureBuffer[currentTexture], 5);
                        if (g_unk0x005210bc != 0)
                            CGraphics::ApplyTextureStageChange(
                                1, CGraphics::m_pTextureManager->textureBuffer[currentTexture]);
                    }
                }
                if (currentTexture > -1) {
                    g_unk0x006dd9bc[count++] = pMesh->pTriangles[i].vertexIndex[0];
                    g_unk0x006dd9bc[count++] = pMesh->pTriangles[i].vertexIndex[1];
                    g_unk0x006dd9bc[count++] = pMesh->pTriangles[i].vertexIndex[2];
                    CGame::m_unk0x0059ce18++;
                }
            }
            if (count != 0) {
                texture = pMesh->pTriangles[triangleCount - 1].textures[textureIndex];
                if (texture > -1) {
                    Frontend_SetObjectField118(
                        CGraphics::m_pTextureManager->textureBuffer[texture], 5);
                    if (g_unk0x005210bc != 0)
                        CGraphics::ApplyTextureStageChange(1, CGraphics::m_pTextureManager->textureBuffer[texture]);
                    Gfx_DrawIndexedPrimitiveVB(GFX_TRIANGLELIST, CGraphics::m_pTextureManager->pVertexBuffers[pMesh->vertexBufferIndex], pMesh->vertexOffset, pMesh->field_0x10, g_unk0x006dd9bc, count);
                }
            }
        }
        CGraphics::SetTextureAddressClamp(1);
        CGraphics::ApplyTextureStageChange(1, 0);
        CGraphics::ApplyTextureStageChange(2, 0);
        Gfx_SetStageTextureTransform(0, 0);
        Gfx_SetNormalizeNormals(0);
        Gfx_SetLocalViewer(0);
    }
}

// 1.0 lives at the original's network constant block (0x511350); using the
// named global instead of a literal keeps reccmp's operand symbol identical.
extern const float g_netOne;

// Startup (C runtime .CRT$XCU) initializer of g_unk0x006dfdf8.
// FUNCTION: CMR2 0x004b2e20
void __cdecl Graphics_InitializeShadowGeometryStorage(void)
{
    g_unk0x006dfdf8 = g_netOne - g_unk0x005210d0;
}

// PORT: was a function pointer in MSVC's .CRT$XCU startup table; standard
// C++ dynamic initialisation runs it at startup.
static int s_graphicsInit = (Graphics_InitializeShadowGeometryStorage(), 0);

// FUNCTION: CMR2 0x004b2e40
void Graphics_SetRecordField2C(Mesh *p, int value)
{
    p->field_0x2c = value;
}

// Shadow volumes: up to 9 cylinders built from meshes, cached by mesh name.
extern Mesh *g_shadowCylinders[10];
extern BYTE g_shadowCylinderCount;
// 4096 / (360 * 65536): 16.16 degrees to a sine table index.
extern double g_fixedDegreesToAngle12;

#include "MeshVertex.h"

// Returns a closed 10-sided cylinder (22 vertices, 20 triangles) enclosing pMesh:
// its radius is the furthest point where a triangle crosses z = 0, its height the
// mesh's z range. Cylinders are cached by name; returns pMesh when the cache is full.
// match 72%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004b67f0
Mesh *Mesh_GetShadowCylinder(Mesh *pMesh)
{
    Mesh *pFound;
    Mesh *pCyl;
    MeshVertexF *pVerts;
    MeshVertexF *pA;
    MeshVertexF *pB;
    MeshVertexF *pC;
    MeshVertexF *pLone;
    MeshTriangle *pTri;
    float radius;
    float minZ;
    float maxZ;
    float d;
    int count;
    int angle;
    unsigned short idx;
    int i;
    int n;

    count = g_shadowCylinderCount;
    pFound = NULL;
    for (i = 0; i < count; i++) {
        if (strcmp((char *)g_shadowCylinders[i], (char *)pMesh) == 0) {
            pFound = g_shadowCylinders[i];
            i = count;
        }
    }
    if (pFound != NULL)
        return pFound;

    if (g_shadowCylinderCount < 9) {
        pVerts = (MeshVertexF *)pMesh->pVertexData;
        radius = 0.0f;
        minZ = 0.0f;
        maxZ = 0.0f;
        for (i = 0; i < pMesh->field_0x10; i++) {
            if (pVerts[i].z > maxZ)
                maxZ = pVerts[i].z;
            if (pVerts[i].z < minZ)
                minZ = pVerts[i].z;
        }
        pTri = pMesh->pTriangles;
        for (n = pMesh->triangleCount; n > 0; n--, pTri++) {
            pLone = NULL;
            pA = &pVerts[pTri->vertexIndex[0]];
            pB = &pVerts[pTri->vertexIndex[1]];
            pC = &pVerts[pTri->vertexIndex[2]];
            if ((pA->z < 0.0f && pB->z > 0.0f && pC->z > 0.0f) ||
                (pA->z > 0.0f && pB->z < 0.0f && pC->z < 0.0f))
                pLone = pA;
            if ((pB->z < 0.0f && pA->z > 0.0f && pC->z > 0.0f) ||
                (pB->z > 0.0f && pA->z < 0.0f && pC->z < 0.0f))
                pLone = pB;
            if (!((pC->z < 0.0f && pB->z > 0.0f && pA->z > 0.0f) ||
                  (pC->z > 0.0f && pB->z < 0.0f && pA->z < 0.0f)))
                pC = pLone;
            if (pC != NULL) {
                d = (float)X87S_SQRT(pC->x * pC->x + pC->y * pC->y);
                if (d > radius)
                    radius = d;
            }
        }

        // A native Mesh object needs its complete header, including the tail.
        pCyl = (Mesh *)CFileBuffer::AllocateLockedBuffer(sizeof(Mesh));
        g_shadowCylinders[g_shadowCylinderCount++] = pCyl;
        memset(pCyl, 0, offsetof(Mesh, field_0x108));
        strcpy((char *)pCyl, (char *)pMesh);
        pCyl->field_0x10 = 22;
        pCyl->triangleCount = 20;
        pCyl->pTriangles = (MeshTriangle *)CFileBuffer::AllocateLockedBuffer(20 * sizeof(MeshTriangle));
        pCyl->pVertexData = (DWORD *)CFileBuffer::AllocateLockedBuffer(pCyl->field_0x10 * sizeof(MeshVertexF));

        // Bottom ring: centre plus 10 points every 36 degrees.
        ((MeshVertexF *)pCyl->pVertexData)[0].x = 0.0f;
        ((MeshVertexF *)pCyl->pVertexData)[0].y = 0.0f;
        angle = 0;
        for (i = 1; i <= 10; i++) {
            idx = (unsigned short)FTOI(__int64, X87S_MUL((double)angle, g_fixedDegreesToAngle12));
            ((MeshVertexF *)pCyl->pVertexData)[i].x =
                X87S_MUL(X87S_MUL(X87S_FILD(float, g_sinTable[idx & 0xfff]), CGraphics::m_oneOver65536), radius);
            ((MeshVertexF *)pCyl->pVertexData)[i].y =
                X87S_MUL(X87S_MUL(X87S_FILD(float, g_sinTable[(idx + 0x400) & 0xfff]), CGraphics::m_oneOver65536), radius);
            angle += FixDiv(360 << 16, 10 << 16);
        }
        for (i = 0; i < 11; i++) {
            ((MeshVertexF *)pCyl->pVertexData)[i].z = minZ;
            ((MeshVertexF *)pCyl->pVertexData)[i].nx = 0.0f;
            ((MeshVertexF *)pCyl->pVertexData)[i].ny = 0.0f;
            ((MeshVertexF *)pCyl->pVertexData)[i].nz = -1.0f;
        }
        // Top ring: copy of the bottom one at the top of the mesh.
        for (i = 11; i < pCyl->field_0x10; i++) {
            ((MeshVertexF *)pCyl->pVertexData)[i] = ((MeshVertexF *)pCyl->pVertexData)[i - 11];
            ((MeshVertexF *)pCyl->pVertexData)[i].z = maxZ;
            ((MeshVertexF *)pCyl->pVertexData)[i].nx = 0.0f;
            ((MeshVertexF *)pCyl->pVertexData)[i].ny = 0.0f;
            ((MeshVertexF *)pCyl->pVertexData)[i].nz = 1.0f;
        }
        // Bottom cap as a fan around the centre, top cap offset by 11 vertices.
        for (i = 0; i < 10; i++) {
            pCyl->pTriangles[i].vertexIndex[0] = 0;
            pCyl->pTriangles[i].vertexIndex[1] = i + 1;
            if (i == 9)
                pCyl->pTriangles[9].vertexIndex[2] = 1;
            else
                pCyl->pTriangles[i].vertexIndex[2] = i + 2;
        }
        for (i = 10; i < pCyl->triangleCount; i++) {
            pCyl->pTriangles[i].vertexIndex[0] = pCyl->pTriangles[i - 10].vertexIndex[0] + 11;
            pCyl->pTriangles[i].vertexIndex[1] = pCyl->pTriangles[i - 10].vertexIndex[1] + 11;
            pCyl->pTriangles[i].vertexIndex[2] = pCyl->pTriangles[i - 10].vertexIndex[2] + 11;
        }
        if (pCyl != NULL)
            return pCyl;
    }
    return pMesh;
}

extern void *g_sceneType1Objects[60];
extern int *g_sceneSectorFlags;
extern LightZone *g_sceneLightZones;
extern short *g_sceneSectorZone;

// Attenuates the light-zone vertices of a sector by D3D light `light` (by
// horizontal distance); outside the light's reach they go back to full
// intensity. Returns 1 when any intensity changed.
// match 48%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004b6ca0
int Scene_AttenuateSectorLight(int sector, int light)
{
    GfxLight *pLight;
    LightZone *pZone;
    int *pIntensity;
    int changed;
    int range;
    int att0;
    int base;
    int att2;
    int limit;
    FixVector lpos;
    FixVector d;
    int dx, dz;
    int wasLit;
    int old;
    int d2;
    int i;

    changed = 0;
    pLight = (GfxLight *)g_sceneType1Objects[light];
    if (pLight == NULL)
        return changed;
    range = (int)FTOI(__int64, X87S_MUL(pLight->range, CGraphics::m_65536));
    att0 = (int)FTOI(__int64, X87S_MUL(pLight->attenuation0, CGraphics::m_65536));
    base = FixMul(0x20000, att0);
    att2 = (int)FTOI(__int64, X87S_MUL(pLight->attenuation2, CGraphics::m_65536));
    limit = g_sectorHalfSize + range + 0x140000;
    lpos.x = (int)FTOI(__int64, X87S_MUL(pLight->position.x, CGraphics::m_65536));
    lpos.y = (int)FTOI(__int64, X87S_MUL(pLight->position.y, CGraphics::m_65536));
    lpos.z = (int)FTOI(__int64, X87S_MUL(pLight->position.z, CGraphics::m_65536));
    wasLit = g_sceneSectorFlags[sector];
    d.x = g_sectors[sector]->x - lpos.x;
    d.y = g_sectors[sector]->y - lpos.y;
    d.z = g_sectors[sector]->z - lpos.z;
    g_sceneSectorFlags[sector] = 0;
    if (FIX_ABS(d.x) <= limit && FIX_ABS(d.z) <= limit)
        g_sceneSectorFlags[sector] = 1;
    if (g_sceneSectorFlags[sector] != 0) {
        pZone = &g_sceneLightZones[g_sceneSectorZone[sector]];
        pIntensity = &pZone->pItems->intensity;
        for (i = 0; i < pZone->itemCount; i++) {
            old = *pIntensity;
            dx = lpos.x - pIntensity[-6];
            dz = lpos.z - pIntensity[-5];
            *pIntensity = 0x10000;
            if (dx <= range || dz <= range) {
                d2 = FixMul(dx, dx) + FixMul(dz, dz);
                d2 = FixMul(d2, att2) + base;
                d2 = FixDiv(d2, 0x1000000);
                if (d2 < 0)
                    d2 = 0;
                else if (d2 > 0x10000)
                    d2 = 0x10000;
                *pIntensity = d2;
            }
            if (*pIntensity != old)
                changed = 1;
            pIntensity += sizeof(LightZoneItem) / sizeof(int);
        }
    } else if (wasLit != 0) {
        pZone = &g_sceneLightZones[g_sceneSectorZone[sector]];
        if (pZone->itemCount != 0) {
            pIntensity = &pZone->pItems->intensity;
            for (i = 0; i < pZone->itemCount; i++) {
                old = *pIntensity;
                *pIntensity = 0x10000;
                if (old != 0x10000)
                    changed = 1;
                pIntensity += sizeof(LightZoneItem) / sizeof(int);
            }
            return changed;
        }
    }
    return changed;
}

// FUNCTION: CMR2 0x004b6ef0
void Graphics_SetGeometryStateValue(int value)
{
    g_unk0x005210c0 = value;
}

extern DWORD g_sceneAmbientD3D;
// Ambient colour used outside the stage lighting.
// GLOBAL: CMR2 0x006e0bac
DWORD g_defaultAmbientD3D;

// Sets up Direct3D lighting for a rendering mode: 0/1 object lights (light 1
// or 2), 2/3 stage ambient with light 0, 4/5 unlit. Lights 1 and 2 stay off
// while g_unk0x005210c0 is clear.
// FUNCTION: CMR2 0x004b6f00
void Graphics_SetLightingMode(int mode)
{
    switch (mode) {
    case 0:
        Gfx_SetLighting(TRUE);
        Gfx_SetAmbient(g_defaultAmbientD3D);
        Gfx_SetColorVertex(TRUE);
        Gfx_SetDiffuseMaterialSource(GFX_MCS_MATERIAL);
        Gfx_EnableLight(0, FALSE);
        Gfx_EnableLight(1, TRUE);
        Gfx_EnableLight(2, FALSE);
        break;
    case 1:
        Gfx_SetLighting(TRUE);
        Gfx_SetAmbient(g_defaultAmbientD3D);
        Gfx_SetColorVertex(TRUE);
        Gfx_SetDiffuseMaterialSource(GFX_MCS_MATERIAL);
        Gfx_EnableLight(0, FALSE);
        Gfx_EnableLight(1, FALSE);
        Gfx_EnableLight(2, TRUE);
        break;
    case 2:
        Gfx_SetLighting(TRUE);
        Gfx_SetAmbient(g_sceneAmbientD3D);
        Gfx_SetColorVertex(FALSE);
        Gfx_SetDiffuseMaterialSource(GFX_MCS_MATERIAL);
        Gfx_EnableLight(0, TRUE);
        Gfx_EnableLight(1, FALSE);
        Gfx_EnableLight(2, FALSE);
        break;
    case 3:
        Gfx_SetLighting(TRUE);
        Gfx_SetAmbient(g_sceneAmbientD3D);
        Gfx_SetColorVertex(TRUE);
        Gfx_SetDiffuseMaterialSource(GFX_MCS_COLOR1);
        Gfx_EnableLight(0, TRUE);
        Gfx_EnableLight(1, FALSE);
        Gfx_EnableLight(2, FALSE);
        break;
    case 4:
    case 5:
        Gfx_SetLighting(FALSE);
        Gfx_SetAmbient(g_defaultAmbientD3D);
        Gfx_SetColorVertex(TRUE);
        Gfx_SetDiffuseMaterialSource(GFX_MCS_MATERIAL);
        Gfx_EnableLight(0, FALSE);
        Gfx_EnableLight(1, FALSE);
        Gfx_EnableLight(2, FALSE);
        break;
    }
    if (g_unk0x005210c0 == 0) {
        Gfx_EnableLight(1, FALSE);
        Gfx_EnableLight(2, FALSE);
    }
    g_unk0x005210c4 = mode;
}

// FUNCTION: CMR2 0x004b7200
int Graphics_GetGeometryStateValue(void)
{
    return g_unk0x005210c4;
}

// FUNCTION: CMR2 0x004b98f0
void Graphics_AccumulateCounterOrInitialize(int *p, int value)
{
    if (*p == -1) {
        *p = 0;
        return;
    }
    *p += value;
}

// Damped wobble applied by timer shapes 1 and 4 during their last 16 steps.
// GLOBAL: CMR2 0x00521120
signed char g_timerWobble[24] = { 3, 6, 6, 3, 0, 2, 4, 4, 2, 0, 1, 2, 2, 1, 0, 1, 0 };

// Current value of a timer (32 slots of 0x30 bytes): base + scale * shape(step)
// / 4096, stepping once every 16 ms; once finished the slot goes to shape 5
// and returns its end value.
// match 98%: 2 instr left (case 4: `sub edx,0x11`+`lea eax,[edx+1]` vs `lea eax,[edx-0x11]`+`add edx,-0x10`)
// FUNCTION: CMR2 0x004bc110
int Timer_GetValue(BYTE index)
{
    InterpolationTimer *t;
    unsigned int pos;
    unsigned int dur;
    int v;
    BYTE step;
    int now;

    index &= 0xff;
    t = &g_timerPulseState.timers[index];
    pos = t->step;
    dur = t->length;
    step = (BYTE)pos;
    if (pos >= dur)
        step = (BYTE)dur;
    if (t->delay > 0) {
        (t->delay)--;
        return t->start;
    }
    switch (t->shape) {
    case 0:
        v = step;
        break;
    case 1:
        if (step > dur - 0x11)
            goto wobble;
        v = dur - step - 0x11;
        break;
    case 2:
        v = (int)((dur - step + 1) * (dur - step)) / 2;
        break;
    case 3:
        v = ((step + 1) * step) / 2;
        break;
    case 4:
        if (step > dur - 0x11) {
        wobble:
            v = (int)g_timerWobble[16 + step - dur] * (int)t->direction;
            t->scale = 0x1000;
        } else {
            v = (int)((dur - step - 0x11 + 1) * (dur - step - 0x11)) / 2;
        }
        break;
    case 5:
        v = (unsigned int)t->end;
        break;
    default:
        return t->end;
    }
    if (pos < dur + 3) {
        now = CMain::GetFrameTime();
        if (abs(now - t->lastTime) >= 17) {
            (t->step)++;
            t->lastTime = CMain::GetFrameTime();
        }
    }
    if (t->shape == 1 || t->step < t->length)
        return (int)(t->scale * v) / 4096 + t->base;
    t->shape = 5;
    return t->end;
}

// Whether the timer that p was registered with (its first byte is the slot)
// is still running.
// FUNCTION: CMR2 0x004bc0c0
BYTE Graphics_IsRegisteredTimerRunning(BYTE *p)
{
    InterpolationTimer *timer = &g_timerPulseState.timers[*p];

    if (timer->pOwner != p)
        return 0;
    if (timer->holdAtEnd != 0)
        return timer->step < timer->length + 3;
    return timer->step < timer->length;
}

BYTE Graphics_GetTimerRecordBitUnion(BYTE index);
BYTE Timer_FindFree(void);

// When set, Timer_Start collapses the range to its end value (the demo
// replays use it to freeze the interpolated values).
// GLOBAL: CMR2 0x00816290
int g_timersFrozen;

// Starts the timer whose slot index is stored in *pSlot (the previous owner,
// if any, sends it to shape 5): the value interpolates from start to end over
// length steps following one of five shapes, and the slot keeps the
// interpolation speed, the shape sign and the caller's extra parameters.
// FUNCTION: CMR2 0x004bc290
void Graphics_StartShapedInterpolationTimer(BYTE *pSlot, int shape, int length, int param4, int start, int end, BYTE param7)
{
    InterpolationTimer *rec;
    int speed;
    int tri;

    if (g_timersFrozen != 0) {
        param4 = 0;
        start = end;
        param7 = 0;
    }
    tri = (length + 1) * length / 2;
    if (*pSlot < 0x20 && g_timerPulseState.timers[*pSlot].pOwner == pSlot)
        g_timerPulseState.timers[*pSlot].shape = 5;
    *pSlot = Timer_FindFree();
    rec = &g_timerPulseState.timers[*pSlot];
    switch (shape) {
    case 0:
        rec->base = start;
        rec->scale = (end - start) * 0x1000 / length;
        break;
    case 1:
        speed = (start - end) * 0x1000 / length;
        rec->scale = speed;
        rec->base = end;
        length += 0x11;
        break;
    case 2:
        rec->base = end;
        rec->scale = (start - end) * 0x1000 / tri;
        break;
    case 3:
        rec->base = start;
        rec->scale = (end - start) * 0x1000 / tri;
        break;
    case 4:
        rec->scale = (start - end) * 0x1000 / tri;
        rec->base = end;
        length += 0x11;
        break;
    default:
        return;
    }
    rec->shape = (BYTE)shape;
    rec->length = length;
    rec->direction = rec->scale < 0 ? -1 : 1;
    rec->holdAtEnd = param7;
    rec->delay = param4;
    rec->start = start;
    rec->end = end;
    rec->pOwner = pSlot;
    rec->step = 0;
    rec->bitUnion = Graphics_GetTimerRecordBitUnion(*pSlot);
}

// FUNCTION: CMR2 0x004bc3e0
BYTE Graphics_GetTimerRecordBitUnion(BYTE index)
{
    BYTE *p = (BYTE *)&g_timerPulseState.timers[index & 0xff];
    BYTE r = (BYTE)index;
    int i = 24;

    do {
        r |= *p++;
    } while (--i != 0);
    return r;
}

// FUNCTION: CMR2 0x004bc440
void Graphics_ResetInterpolationTimers(void)
{
    InterpolationTimer *p;

    memset(g_timerPulseState.timers, 0, sizeof(g_timerPulseState.timers));
    p = g_timerPulseState.timers;
    do {
        p->shape = 5;
        p++;
    } while ((INT_PTR)p < (INT_PTR)&g_timerPulseState.timers[32]);
}

// FUNCTION: CMR2 0x004bc470
void Graphics_StopRegisteredTimer(BYTE *p)
{
    g_timerPulseState.timers[*p].shape = 5;
}

// GLOBAL: CMR2 0x008164c8
int g_unk0x008164c8;
// GLOBAL: CMR2 0x00816298
BYTE g_unk0x00816298[0x230];

// GLOBAL: CMR2 0x00816704
int g_unk0x00816704;
// GLOBAL: CMR2 0x00816820
char *g_unk0x00816820[20];
// GLOBAL: CMR2 0x00816974
unsigned int g_unk0x00816974;

// Slow pulsing level (0..0.3) eased near its ends, plus a free-running phase.
// GLOBAL: CMR2 0x0081680c
float g_pulsePhase;
// GLOBAL: CMR2 0x00816810
float g_pulseLevel;
// GLOBAL: CMR2 0x00816814
int g_pulseFrozen;
// GLOBAL: CMR2 0x00816818
float g_pulseMin;
// GLOBAL: CMR2 0x0081681c
float g_pulseSpeed;


// Level thresholds and the phase step, taken from the original's constant block.
// GLOBAL: CMR2 0x00511ce8
extern const float g_float0_1 = 0.1f;
// GLOBAL: CMR2 0x00511d10
extern const float g_float0_25 = 0.25f;
// GLOBAL: CMR2 0x00511d14
extern const float g_float0_15 = 0.15f;
// GLOBAL: CMR2 0x00511d18
extern const float g_float0_05 = 0.05f;
// GLOBAL: CMR2 0x00511d1c
extern const float g_float0_004 = 0.004f;
// GLOBAL: CMR2 0x0051136c
extern const float g_float0_2 = 0.2f;

// FUNCTION: CMR2 0x004bcc60
void Pulse_Update(unsigned int dt)
{
    if (g_pulseFrozen != 0)
        return;
    g_pulsePhase = X87S_ADD(X87S_MUL(X87S_FILD(float, dt), g_float0_004), g_pulsePhase);
    if (g_pulseRising != 0) {
        if (g_pulseLevel <= g_float0_05)
            g_pulseSpeed = 0.0007f;
        else if (g_pulseLevel <= g_float0_1)
            g_pulseSpeed = 0.0011f;
        else if (g_pulseLevel <= g_float0_15)
            g_pulseSpeed = 0.0014f;
        else if (g_pulseLevel <= g_float0_2)
            g_pulseSpeed = 0.0011f;
        else if (g_pulseLevel <= g_float0_25)
            g_pulseSpeed = 0.0007f;
        else
            g_pulseSpeed = 0.0004f;
    } else {
        if (g_pulseLevel <= g_float0_05)
            g_pulseSpeed = 0.0007f;
        else if (g_pulseLevel <= g_float0_1)
            g_pulseSpeed = 0.0011f;
        else if (g_pulseLevel <= g_float0_15)
            g_pulseSpeed = 0.0014f;
        else if (g_pulseLevel <= g_float0_2)
            g_pulseSpeed = 0.0011f;
        else if (g_pulseLevel <= g_float0_25)
            g_pulseSpeed = 0.0007f;
        else
            g_pulseSpeed = 0.0004f;
    }
    if (g_pulseLevel >= g_pulseMax)
        g_pulseRising = 0;
    if (g_pulseLevel <= g_pulseMin && g_pulseRising == 0) {
        g_pulseLevel = X87S_ADD(X87S_MUL(X87S_FILD(float, dt), g_pulseSpeed), g_pulseLevel);
        g_pulseRising = 1;
    } else if (g_pulseRising != 0) {
        g_pulseLevel = X87S_ADD(X87S_MUL(X87S_FILD(float, dt), g_pulseSpeed), g_pulseLevel);
    } else {
        g_pulseLevel = X87S_SUB(g_pulseLevel, X87S_MUL(X87S_FILD(float, dt), g_pulseSpeed));
    }
}

// FUNCTION: CMR2 0x004bcad0
void Graphics_SetFlareStateValue(int value)
{
    g_unk0x00816704 = value;
}

// FUNCTION: CMR2 0x004bcae0
int Graphics_GetFlareStateValue(void)
{
    return g_unk0x00816704;
}

// FUNCTION: CMR2 0x004bcfe0
char *Graphics_GetBoundedStringEntry(unsigned int index)
{
    if (index >= 20)
        return CMain::m_logFileBlankLine;
    if (index >= g_unk0x00816974)
        return CMain::m_logFileBlankLine;
    return g_unk0x00816820[index];
}

// GLOBAL: CMR2 0x00816970
int g_argsParsed;

// Release callback: frees the argument copies.
// FUNCTION: CMR2 0x004bcf80
int Args_Free(void)
{
    unsigned int i;

    // PORT: the copies come from malloc instead of GlobalAlloc.
    for (i = 0; i < g_unk0x00816974; i++)
        free(g_unk0x00816820[i]);
    return 1;
}

// Splits the command line into space-separated arguments (GlobalAlloc copies);
// returns their count.
// FUNCTION: CMR2 0x004bce80
int Args_Parse(char *pCommandLine)
{
    char word[260];
    char *pOut;
    int last;
    int i;

    last = 0;
    g_unk0x00816974 = 0;
    for (i = 0; i < 20; i++)
        g_unk0x00816820[i] = NULL;
    if (pCommandLine == NULL || *pCommandLine == '\0') {
        g_argsParsed = 1;
        return 0;
    }
    for (;;) {
        memset(word, 0, sizeof(word));
        pOut = word;
        while (*pCommandLine == ' ')
            pCommandLine++;
        while (*pCommandLine != ' ') {
            if (*pCommandLine == '\0') {
                last = 1;
                break;
            }
            *pOut++ = *pCommandLine++;
        }
        g_unk0x00816820[g_unk0x00816974] = (char *)calloc(1, strlen(word) + 2);
        strcpy(g_unk0x00816820[g_unk0x00816974], word);
        g_unk0x00816974++;
        if (last) {
            g_argsParsed = 1;
            CGame::RegisterCallback(Args_Free, NULL);
            return g_unk0x00816974;
        }
    }
}

// Case-insensitive search for an argument; returns 1 when present.
// FUNCTION: CMR2 0x004bd010
int Args_Has(char *pArg)
{
    char wanted[260];
    char arg[260];
    unsigned int i;

    strcpy(wanted, pArg);
    _strlwr(wanted);
    for (i = 0; i < g_unk0x00816974; i++) {
        strcpy(arg, Graphics_GetBoundedStringEntry(i));
        _strlwr(arg);
        if (strcmp(wanted, arg) == 0)
            return 1;
    }
    return 0;
}

// FUNCTION: CMR2 0x004bcac0
int Graphics_ClearFlareTextureRegistration(void)
{
    g_unk0x008164c8 = 0;
    return 1;
}

// Fog distances (floats written to the render states as raw bits).
// GLOBAL: CMR2 0x008165f4
float g_fogEnd;
// GLOBAL: CMR2 0x008165f8
float g_fogField2;
// GLOBAL: CMR2 0x008165fc
float g_fogField1;
// GLOBAL: CMR2 0x00816600
float g_fogStart;

// Sets the fog range (16.16) and colour (r, g, b in the low bytes) and turns
// on linear vertex fog when fog is enabled.
// FUNCTION: CMR2 0x004bcaf0
void Graphics_SetFog(int start, int end, int a, int b, DWORD colour)
{
    g_fogStart = X87S_MUL(X87S_FILD(float, start), CGraphics::m_oneOver65536);
    g_fogEnd = X87S_MUL(X87S_FILD(float, end), CGraphics::m_oneOver65536);
    g_fogField1 = X87S_MUL(X87S_FILD(float, a), CGraphics::m_oneOver65536);
    g_fogField2 = X87S_MUL(X87S_FILD(float, b), CGraphics::m_oneOver65536);
    Gfx_SetFogColor(GFX_RGBA(((BYTE *)&colour)[0], ((BYTE *)&colour)[1], ((BYTE *)&colour)[2], 0xff));
    if (Graphics_GetDeviceCaps90() != 0) {
        Gfx_SetFogTableMode(GFX_FOG_NONE);
        Gfx_SetFogVertexMode(GFX_FOG_LINEAR);
    }
}

// Re-enables linear vertex fog with the stored range.
// FUNCTION: CMR2 0x004bcbb0
void Graphics_EnableFog(void)
{
    if (Graphics_GetDeviceCaps90() != 0) {
        Gfx_SetFogTableMode(GFX_FOG_NONE);
        Gfx_SetFogVertexMode(GFX_FOG_LINEAR);
        Gfx_SetFogStart(g_fogStart);
        Gfx_SetFogEnd(g_fogEnd);
    }
}

// FUNCTION: CMR2 0x004bcc20
void Graphics_DisableFog(void)
{
    if (Graphics_GetDeviceCaps94() != 0 || Graphics_GetDeviceCaps90() != 0) {
        Gfx_SetFogTableMode(GFX_FOG_NONE);
        Gfx_SetFogVertexMode(GFX_FOG_NONE);
    }
}

// FUNCTION: CMR2 0x004b2d70
// PORT: NULL draws into the back buffer.
void Graphics_SetRenderTarget(Texture *pTexture)
{
    Gfx_SetRenderTarget(pTexture != NULL ? pTexture->pSurface : NULL);
}

// Stores a 3-bit value in bits 15..17 of the flags of every mesh in a hierarchy.
// match 57%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004b2d90
void SceneNode_SetMeshFlagBits(SceneNode *pNode, unsigned int value)
{
    SceneNode *pChild;
    SceneNode *p;
    Mesh *pMesh;

    if (pNode == NULL)
        return;
    if (pNode->type == SCENE_NODE_MESH && (pMesh = (Mesh *)pNode->pObject) != NULL)
        pMesh->flags = (value & 7) << 15 | pMesh->flags & 0xfffc7fff;
    p = pNode->pFirstChild;
    while (p != NULL) {
        pChild = p;
        while (p != NULL) {
            if (p->type == SCENE_NODE_MESH && (pMesh = (Mesh *)p->pObject) != NULL)
                pMesh->flags = (value & 7) << 15 | pMesh->flags & 0xfffc7fff;
            p = p->pFirstChild;
        }
        p = pChild->pNext;
    }
}

// GLOBAL: CMR2 0x0052111c
char g_strSuffixW[4] = "W";

extern char g_fontTgaFormat[12];
int Graphics_HasLocalSuffix(char *pName);
Texture *Graphics_FindAndEnsureResidentTexture(char *name);

// Variants of the texture file suffix the load flags depend on.
// GLOBAL: CMR2 0x00521114
char g_str0x00521114[4] = "B3";
// GLOBAL: CMR2 0x00521118
char g_str0x00521118[4] = "B2";

// Loads every texture of a list of texture records. Each record holds a header
// with the number of textures followed by that many index/offset entries into
// the texture directory; the file name is built from the directory of the game
// plus the last path component of the texture entry. Resident textures are
// skipped when the record type is 6.
// match 62%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004b9910
void Graphics_LoadTextureRecordList(BYTE *param1, char *param2, unsigned int param3, int param4, GenericFile *archive)
{
    unsigned short *pEntry;
    unsigned int remaining;
    char *pName;
    char fileName[260];
    unsigned int i;
    unsigned int flags = param3;

    if (param3 != 0) {
        remaining = param3;
        do {
            i = 0;
            if (*(unsigned short *)(param1 + 0xc) > 0) {
                pEntry = (unsigned short *)(param1 + 0x14);
                do {
                    pName = param2 + *pEntry * 0x104;
                    CGenericFileLoader::StrUpperPolish((BYTE *)pName);
                    strcpy(CFrontend::m_stringDest, CInstallInfo::GetTexturesDirectory());
                    // PORT: preserve the variadic ABI (macOS ARM64).
                    sprintf(fileName, g_fontTgaFormat, CFrontend::m_stringDest,
                            strchr(pName, '\\') + 1);
                    if (param4 == 6) {
                        if (Graphics_FindAndEnsureResidentTexture(fileName) != 0)
                            goto nextEntry;
                        if (strncmp(fileName + strlen(fileName) - 6, CGraphics::m_strSuffixBU, 2) != 0 &&
                            strncmp(fileName + strlen(fileName) - 6, g_str0x00521118, 2) != 0 &&
                            strncmp(fileName + strlen(fileName) - 6, g_str0x00521114, 2) != 0)
                            flags = Graphics_HasLocalSuffix(fileName) != 0 ? 0x40 : 0x100;
                    } else {
                        flags = param4 != 10 ? 0x90 : 0;
                    }
                    CTexture::FindLoadTexture(archive, fileName, 0, 0, 0, flags);
nextEntry:
                    pEntry += 4;
                    i++;
                } while (i < *(unsigned short *)(param1 + 0xc));
            }
            param1 = param1 + 0x10 + i * 8;
            remaining--;
        } while (remaining != 0);
    }
}

// Returns 1 when a file name ends in a localised suffix: "RU"/"BR" before
// the extension, or 'W' two characters earlier.
// FUNCTION: CMR2 0x004b9af0
int Graphics_HasLocalSuffix(char *pName)
{
    if (strncmp(pName + strlen(pName) - 6, CGraphics::m_strSuffixBR, 2) != 0 &&
        strncmp(pName + strlen(pName) - 6, CGraphics::m_strSuffixRU, 2) != 0 &&
        strncmp(pName + strlen(pName) - 8, g_strSuffixW, 1) != 0)
        return 0;
    return 1;
}

// FUNCTION: CMR2 0x004bca70
void Graphics_InitializeFlareTextureSurface(short *param1)
{
    if (g_unk0x008164c8 != 0)
        return;
    CGame::RegisterCallback(Graphics_ClearFlareTextureRegistration, NULL);
    if (param1[2] != 0 && param1[3] != 0) {
        CGraphics::CreateTextureSurface((Texture *)g_unk0x00816298, param1[2], param1[3], 8);
        g_unk0x008164c8 = 1;
    }
}

// Screen rectangle and colour of the sun for the lens flare test.
// GLOBAL: CMR2 0x008164d0
short g_flareRect[4];
// GLOBAL: CMR2 0x008164d8
BYTE g_flareColour[3];
// GLOBAL: CMR2 0x008164dc
BYTE g_flareVisibility;
// GLOBAL: CMR2 0x008164dd
BYTE g_flareTolerance;

// Lowest set bit and number of set bits of a colour mask.
#define MASK_SHIFT(mask, out)             \
    v = (mask);                           \
    for (k = 0; k < 32; k++) {            \
        if (v & 1)                        \
            break;                        \
        v >>= 1;                          \
    }                                     \
    out = (BYTE)k
#define MASK_BITS(mask, out)              \
    v = (mask);                           \
    n = 0;                                \
    for (k = 32; k != 0; k--) {           \
        if (v & 1)                        \
            n++;                          \
        v >>= 1;                          \
    }                                     \
    out = (BYTE)n

// Lens flare occlusion test. With a rectangle and a colour, stores them for
// the next test and returns the last result; without, copies that screen
// rectangle out of the back buffer and returns the percentage of its pixels
// still showing the sun's colour (within the tolerance).
// match 34%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004bc490
BYTE Flare_SampleVisibility(short *pRect, BYTE *pColour, BYTE tolerance)
{
    GfxLockedRect desc;
    RECT src;
    RECT dst;
    unsigned short resX;
    unsigned short resY;
    short x;
    int y;
    int w;
    short h;
    short rLo;
    short rHi;
    short gLo;
    short gHi;
    short bLo;
    short bHi;
    short c;
    int area;
    int count;
    int rShift;
    int gShift;
    int bShift;
    int rBits;
    int gBits;
    int bBits;
    unsigned int v;
    unsigned int pixel;
    unsigned short row;
    unsigned short col;
    int k;
    int n;

    resX = (unsigned short)g_pGraphics->resX;
    resY = (unsigned short)g_pGraphics->resY;
    count = 0;
    if (g_unk0x008164c8 == 0) {
        if (pRect == NULL)
            goto sample;
        Graphics_InitializeFlareTextureSurface(pRect);
        if (g_unk0x008164c8 == 0)
            return 0;
    }
    if (pRect != NULL && pColour != NULL) {
        g_flareRect[0] = pRect[0];
        g_flareRect[1] = pRect[1];
        g_flareRect[2] = pRect[2];
        g_flareRect[3] = pRect[3];
        g_flareColour[0] = pColour[0];
        g_flareColour[1] = pColour[1];
        g_flareColour[2] = pColour[2];
        g_flareTolerance = tolerance;
        return g_flareVisibility;
    }
sample:
    rLo = g_flareColour[0] - g_flareTolerance;
    rHi = g_flareColour[0] + g_flareTolerance;
    gLo = g_flareColour[1] - g_flareTolerance;
    gHi = g_flareColour[1] + g_flareTolerance;
    bLo = g_flareColour[2] - g_flareTolerance;
    area = g_flareRect[2] * g_flareRect[3];
    bHi = g_flareColour[2] + g_flareTolerance;
    if (area == 0)
        return 0;

    // Clip the rectangle to the screen.
    x = g_flareRect[0];
    y = g_flareRect[1];
    w = g_flareRect[2];
    h = g_flareRect[3];
    if (x < 0) {
        if (w > -x) {
            w += x;
            x = 0;
        } else {
            w = 0;
            x = 0;
        }
    }
    if (y < 0) {
        if (h > -y) {
            h += y;
            y = 0;
        } else {
            h = 0;
            y = 0;
        }
    }
    if (x >= (int)resX) {
        w = 0;
        x = 0;
    }
    if (y >= (int)resY) {
        h = 0;
        y = 0;
    }
    if (x + w >= (int)resX)
        w = resX - x;
    if (y + h >= (int)resY)
        h = resY - y;
    if (w == 0 || h == 0)
        return 0;

    src.left = x;
    src.top = y;
    src.right = x + w;
    src.bottom = y + h;
    dst.left = 0;
    dst.top = 0;
    dst.right = w;
    dst.bottom = h;
    // PORT: the rectangle of the back buffer comes from the renderer (as
    // rendered at the end of the previous frame), as 32-bit pixels.
    {
        static DWORD pixels[640 * 480];

        if (w * h > (int)(sizeof(pixels) / sizeof(pixels[0])) || !Gfx_ReadBackBuffer(x, y, w, h, pixels))
            return g_flareVisibility;
        memset(&desc, 0, sizeof(desc));
        desc.pixels = pixels;
        desc.pitch = w * 4;
        desc.width = w;
        desc.height = h;
        Gfx_GetPixelFormat(GFX_FORMAT_BGRA8, &desc.format);
    }
    MASK_SHIFT(desc.format.redMask, rShift);
    MASK_SHIFT(desc.format.greenMask, gShift);
    MASK_SHIFT(desc.format.blueMask, bShift);
    MASK_BITS(desc.format.redMask, rBits);
    MASK_BITS(desc.format.greenMask, gBits);
    MASK_BITS(desc.format.blueMask, bBits);
    if (desc.format.bitsPerPixel == 16) {
        for (row = 0; row < h; row++) {
            for (col = 0; col < w; col++) {
                pixel = ((unsigned short *)desc.pixels)[(row * desc.pitch) / 2 + col];
                c = (BYTE)(((pixel & desc.format.redMask) >> rShift) << (8 - rBits));
                if (c < rLo || c > rHi)
                    continue;
                c = (BYTE)(((pixel & desc.format.greenMask) >> gShift) << (8 - gBits));
                if (c < gLo || c > gHi)
                    continue;
                c = (BYTE)(((pixel & desc.format.blueMask) >> bShift) << (8 - bBits));
                if (c < bLo || c > bHi)
                    continue;
                count++;
            }
        }
    } else if (desc.format.bitsPerPixel == 32) {
        for (row = 0; row < h; row++) {
            for (col = 0; col < w; col++) {
                pixel = ((DWORD *)desc.pixels)[(row * desc.pitch) / 4 + col];
                c = (BYTE)((pixel & desc.format.redMask) >> rShift);
                if (c < rLo || c > rHi)
                    continue;
                c = (BYTE)((pixel & desc.format.greenMask) >> gShift);
                if (c < gLo || c > gHi)
                    continue;
                c = (BYTE)((pixel & desc.format.blueMask) >> bShift);
                if (c < bLo || c > bHi)
                    continue;
                count++;
            }
        }
    }
    // PORT: nothing to unlock.
    g_flareVisibility = (BYTE)(count * 100 / area);
    return g_flareVisibility;
}

// GLOBAL: CMR2 0x006db200
unsigned short g_unk0x006db200[800 * 6];
// GLOBAL: CMR2 0x006dd784
int g_unk0x006dd784;
// GLOBAL: CMR2 0x006dd788
int g_unk0x006dd788;
extern int g_billboardsEnabled;
void Billboard_Reset(void);

// Builds the 800-entry triangle-strip index table.
// match 76%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004b1150
void Particle_BuildTriangleStripIndices(void)
{
    g_unk0x006dd784 = 0;
    g_unk0x006dd788 = 0;
    int vertex = 3;
    WORD *pIndex = g_unk0x006db200 + 1;
    int remaining = 800;
    do {
        int previous = vertex - 2;
        pIndex[-1] = (WORD)(vertex - 3);
        pIndex[0] = vertex;
        pIndex[1] = previous + 1;
        pIndex[2] = vertex - 3;
        pIndex[3] = previous;
        pIndex[4] = vertex;
        pIndex += 6;
        vertex += 4;
    } while (--remaining);
    g_billboardsEnabled = 1;
    CGame::RegisterCallback(Billboard_Reset, NULL);
}

// Camera-facing quad requested by the game (0x24 bytes); 16.16 fixed point.

// Queued billboard in render format (0x58 bytes).
struct BillboardQuad {
    GfxVector corner[4];        // 0x0  offsets from pos in camera space (y right, z up)
    unsigned short texture;     // 0x30
    GfxVector pos;              // 0x34
    DWORD colour;            // 0x40
    BYTE field_0x44[0xc];
    int mirror;                 // 0x50 flip the texture horizontally
    short field_0x54;           // 0x54 rotation about the view axis (12-bit angle)
};

// Runs of consecutive quads sharing a texture: {count, texture}.
// GLOBAL: CMR2 0x006c84f8
int *g_billboardRun;
// GLOBAL: CMR2 0x006c8500
int g_billboardRuns[800][2];
// GLOBAL: CMR2 0x006c9e00
BillboardQuad g_billboards[800];
// GLOBAL: CMR2 0x006dd780
int g_billboardsEnabled;
// GLOBAL: CMR2 0x006dd78c
int g_billboardsRequested;

void Scene_GetLightColour(DWORD *pColour, int level);

// Queues a billboard for this frame (at most 800), grouping it with the
// previous one when both use the same texture.
// match 70%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004b11c0
// OpenCMR2 diagnostics: why queued billboards were refused, and the peak queue depth before a draw (cap 800 per frame).
unsigned g_billboardRefusedDisabled = 0, g_billboardRefusedFull = 0, g_billboardPeakQueue = 0;
void Billboard_Add(BillboardDef *pDef, Texture *pTexture)
{
    BillboardQuad *pQuad;
    float left;
    float top;
    float right;
    float bottom;
    BYTE dark[4];
    BYTE bright[4];
    int t;

    if (g_billboardsEnabled == 0) {
        if (g_billboardsRequested == 0)
            g_billboardsRequested = 1;
        ++g_billboardRefusedDisabled;
        return;
    }
    if (pTexture != NULL && pDef != NULL && (unsigned int)g_unk0x006dd784 >= 800) ++g_billboardRefusedFull;
    if (pTexture == NULL || pDef == NULL || (unsigned int)g_unk0x006dd784 >= 800)
        return;
    pQuad = &g_billboards[g_unk0x006dd784];
    left = X87S_MUL(X87S_FILD(float, pDef->left), CGraphics::m_oneOver65536);
    top = X87S_MUL(X87S_FILD(float, pDef->top), CGraphics::m_oneOver65536);
    right = X87S_MUL(X87S_FILD(float, pDef->right), CGraphics::m_oneOver65536);
    bottom = X87S_MUL(X87S_FILD(float, pDef->bottom), CGraphics::m_oneOver65536);
    pQuad->corner[0].x = 0.0f;
    pQuad->corner[1].x = 0.0f;
    pQuad->corner[2].x = 0.0f;
    pQuad->corner[3].x = 0.0f;
    pQuad->corner[0].y = -left;
    pQuad->corner[0].z = top;
    pQuad->corner[1].y = -left;
    pQuad->corner[1].z = bottom;
    pQuad->corner[2].y = -right;
    pQuad->corner[2].z = top;
    pQuad->corner[3].y = -right;
    pQuad->corner[3].z = bottom;
    pQuad->texture = pTexture->textureId;
    pQuad->pos.x = X87S_MUL(X87S_FILD(float, pDef->pos.x), CGraphics::m_oneOver65536);
    pQuad->pos.y = X87S_MUL(X87S_FILD(float, pDef->pos.y), CGraphics::m_oneOver65536);
    pQuad->pos.z = X87S_MUL(X87S_FILD(float, pDef->pos.z), CGraphics::m_oneOver65536);
    if ((pDef->flags & 2) == 0) {
        pQuad->colour = GFX_RGBA(pDef->r, pDef->g, pDef->b, pDef->a);
    } else {
        Scene_GetLightColour((DWORD *)dark, 0);
        Scene_GetLightColour((DWORD *)bright, 0x10000);
        dark[0] = dark[0] * pDef->r / 256;
        dark[1] = dark[1] * pDef->g / 256;
        dark[2] = dark[2] * pDef->b / 256;
        bright[0] = bright[0] * pDef->r / 256;
        bright[1] = bright[1] * pDef->g / 256;
        bright[2] = bright[2] * pDef->b / 256;
        t = pDef->shade;
        pQuad->colour = GFX_RGBA((bright[0] * (256 - t) + dark[0] * t) >> 8,
                                  (bright[1] * (256 - t) + dark[1] * t) >> 8,
                                  (bright[2] * (256 - t) + dark[2] * t) >> 8, pDef->a);
    }
    pQuad->mirror = pDef->flags & 1;
    pQuad->field_0x54 = pDef->field_0x20;
    if (g_unk0x006dd784 == 0) {
        g_billboardRun = g_billboardRuns[0];
        g_billboardRuns[0][0] = 1;
        g_billboardRuns[0][1] = pQuad->texture;
        g_unk0x006dd788 = 1;
        g_unk0x006dd784++;
        return;
    }
    if (pQuad->texture != g_billboardRun[1]) {
        g_billboardRun += 2;
        g_unk0x006dd788++;
        g_billboardRun[0] = 1;
        g_billboardRun[1] = pQuad->texture;
        g_unk0x006dd784++;
        return;
    }
    g_billboardRun[0]++;
    g_unk0x006dd784++;
}

// Release callback: empties the billboard queue and disables it.
// FUNCTION: CMR2 0x004b1500
void Billboard_Reset(void)
{
    g_unk0x006dd784 = 0;
    memset(g_billboards, 0, sizeof(g_billboards));
    g_unk0x006dd788 = 0;
    g_billboardsEnabled = 0;
}

// Vertex of a billboard (D3DFVF_XYZ | NORMAL | DIFFUSE | SPECULAR | TEX2).
struct BillboardVertex {
    float x, y, z;
    float nx, ny, nz;
    DWORD diffuse;
    DWORD specular;
    float u, v;
    float u2, v2;
};

// GLOBAL: CMR2 0x006a2cf8
BillboardVertex g_billboardVerts[800 * 4];

void Graphics_InvalidateTextureStageCache(void);
GfxMatrix *FixMatrix_ToFloat(GfxMatrix *pOut, FixMatrix *pIn);

// Turns the queued billboards into quads facing pCamera (rotated by their
// angle), draws them one texture run at a time and empties the queue.
// match 56%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004b1530
void Billboard_Draw(SceneNode *pCamera)
{
    BillboardVertex *pVert;
    BillboardQuad *pQuad;
    GfxMatrix axes;
    GfxMatrix view;
    GfxVector pos;
    GfxVector c0, c1, c2, c3;
    unsigned short angle;
    DWORD colour;
    float c, s;
    int n;
    unsigned int i;
    int first;

    first = 0;
    if (g_billboardsEnabled == 0 || g_unk0x006dd784 == 0)
        return;
    pVert = g_billboardVerts;
    FixMatrix_ToFloat(&view, &pCamera->world);
    if (g_unk0x006dd784 != 0) {
        n = g_unk0x006dd784;
        pQuad = g_billboards;
        CGame::m_unk0x0059ce20 += g_unk0x006dd784 * 2;
        do {
            pos = pQuad->pos;
            c0 = pQuad->corner[0];
            c1 = pQuad->corner[1];
            c2 = pQuad->corner[2];
            c3 = pQuad->corner[3];
            angle = pQuad->field_0x54;
            colour = pQuad->colour;
            if (angle != 0) {
                c = X87S_MUL(X87S_FILD(float, g_sinTable[(angle + 0x400) & 0xfff]), CGraphics::m_oneOver65536);
                s = X87S_MUL(X87S_FILD(float, g_sinTable[angle & 0xfff]), CGraphics::m_oneOver65536);
                axes._11 = view._11 * c + view._21 * s;
                axes._12 = view._12 * c + view._22 * s;
                axes._13 = view._13 * c + view._23 * s;
                axes._21 = view._21 * c - view._11 * s;
                axes._22 = view._22 * c - view._12 * s;
                axes._23 = view._23 * c - view._13 * s;
            } else {
                axes = view;
            }
            pVert[0].x = c0.z * axes._21 + c0.y * axes._11 + pos.x;
            pVert[0].y = c0.z * axes._22 + c0.y * axes._12 + pos.y;
            pVert[0].z = c0.z * axes._23 + c0.y * axes._13 + pos.z;
            if (pQuad->mirror == 0) {
                pVert[0].u = 0.0f;
                pVert[0].v = 1.0f;
            } else {
                pVert[0].u = 1.0f;
                pVert[0].v = 1.0f;
            }
            pVert[0].diffuse = colour;
            pVert[0].specular = 0xff000000;
            pVert[1].x = c1.z * axes._21 + c1.y * axes._11 + pos.x;
            pVert[1].y = c1.z * axes._22 + c1.y * axes._12 + pos.y;
            pVert[1].z = c1.z * axes._23 + c1.y * axes._13 + pos.z;
            pVert[1].u = pQuad->mirror == 0 ? 0.0f : 1.0f;
            pVert[1].v = 0.0f;
            pVert[1].diffuse = colour;
            pVert[1].specular = 0xff000000;
            pVert[2].x = c2.z * axes._21 + c2.y * axes._11 + pos.x;
            pVert[2].y = c2.z * axes._22 + c2.y * axes._12 + pos.y;
            pVert[2].z = c2.z * axes._23 + c2.y * axes._13 + pos.z;
            if (pQuad->mirror != 0) {
                pVert[2].u = 0.0f;
            } else {
                pVert[2].u = 1.0f;
            }
            pVert[2].v = 1.0f;
            pVert[2].diffuse = colour;
            pVert[2].specular = 0xff000000;
            pVert[3].x = c3.z * axes._21 + c3.y * axes._11 + pos.x;
            pVert[3].y = c3.z * axes._22 + c3.y * axes._12 + pos.y;
            pVert[3].z = c3.z * axes._23 + c3.y * axes._13 + pos.z;
            if (pQuad->mirror == 0)
                pVert[3].u = 1.0f;
            else
                pVert[3].u = 0.0f;
            pVert[3].v = 0.0f;
            pVert[3].diffuse = colour;
            pVert[3].specular = 0xff000000;
            pVert += 4;
            pQuad++;
        } while (--n != 0);
    }
    if ((unsigned)g_unk0x006dd784 > g_billboardPeakQueue) g_billboardPeakQueue = (unsigned)g_unk0x006dd784;
    {   // Diagnostic: draw state in force when queued billboards are drawn (about every 600th draw), z-write is turned off just below.
        static unsigned calls = 0;
        if (++calls % 600 == 1) {
            int st[10] = {};
            Gfx_GetBlendDepthState(st);
            Sys_Log("billboard draw state: queued=%d depthTest=%d depthFunc=%d alphaBlend=%d src=%d dst=%d alphaTest=%d alphaRef=%d fog=%d lighting=%d (z-write forced off for this draw; GFX enum values)",
                g_unk0x006dd784, st[0], st[2], st[3], st[4], st[5], st[6], st[7], st[8], st[9]);
        }
    }
    CGraphics::SetZWriteEnable(0);
    g_billboardRun = g_billboardRuns[0];
    for (i = 0; i < (unsigned int)g_unk0x006dd788; i++) {
        CGraphics::ApplyTextureStageChange(0, CGraphics::m_pTextureManager->textureBuffer[g_billboardRun[1]]);
        Gfx_DrawIndexedPrimitive(GFX_TRIANGLELIST, (GfxVertex *)&g_billboardVerts[first * 4], g_billboardRun[0] * 4, g_unk0x006db200, g_billboardRun[0] * 6);
        first += g_billboardRun[0];
        g_billboardRun += 2;
    }
    CGraphics::SetZWriteEnable(1);
    g_unk0x006dd784 = 0;
    g_unk0x006dd788 = 0;
    Graphics_InvalidateTextureStageCache();
    CGraphics::InvalidateBlendStateCache();
}

// Takes a free texture slot, copies the 0x130-byte texture into it and
// returns the slot index (or -1 when the table is full).
// FUNCTION: CMR2 0x004a4bd0
int Graphics_CopyIntoFreeTextureSlot(void *pSource, int param2)
{
    int i;

    for (i = 0; i < 0x800; i++) {
        if (CGraphics::m_pTextureManager->textureBuffer[i] == NULL) {
            CGraphics::m_pTextureManager->textureBuffer[i] =
                (Texture *)CFileBuffer::AllocateLockedBuffer(sizeof(Texture));
            memcpy(CGraphics::m_pTextureManager->textureBuffer[i], pSource, sizeof(Texture));
            CGraphics::m_textureCount++;
            return i;
        }
    }
    return -1;
}

// GLOBAL: CMR2 0x006a2a98
GlowLight *g_glowEntries;
// GLOBAL: CMR2 0x006a2bcc
int g_glowCapacity;
// GLOBAL: CMR2 0x006a2bc8
int g_glowAllocatedCount;
// GLOBAL: CMR2 0x00520f94
BYTE g_layerQuadColour[4] = { 0, 0, 0, 70 };
// GLOBAL: CMR2 0x00520f98
Quad2DInputVertex g_projectedQuad[4] = {
    { 0, 0, 0, { 0xff, 0xff, 0xff, 0xff }, 0, 0 },
    { 0, 0, 0, { 0xff, 0xff, 0xff, 0xff }, 0xfff9, 0 },
    { 0, 0, 0, { 0xff, 0xff, 0xff, 0xff }, 0xfff9, 0xfff9 },
    { 0, 0, 0, { 0xff, 0xff, 0xff, 0xff }, 0, 0xfff9 },
};
// Glow basis: [0] the horizontal camera forward (normalised), [1] and [2] the
// axes of the projected layer quad ([2] follows the camera).
// GLOBAL: CMR2 0x00520ff8
FixVector g_glowBasis[3] = {{0x10000, 0, 0}, {0, 0x10000, 0}, {0, 0, 0x10000}};
#define g_glowForward (g_glowBasis[0])

// FUNCTION: CMR2 0x004ae140
void Graphics_SetLayerQuadColour(BYTE *pColour)
{
    g_layerQuadColour[0] = pColour[0];
    g_layerQuadColour[1] = pColour[1];
    g_layerQuadColour[2] = pColour[2];
    g_layerQuadColour[3] = pColour[3];
}

// FUNCTION: CMR2 0x004ae230
void Glow_SetSymmetricBounds(BillboardDef *p, int x, int y)
{
    p->top = -x;
    p->left = y;
    p->bottom = x;
    p->right = -y;
}

// FUNCTION: CMR2 0x004ae260
void Glow_ResetEntries(void)
{
    int i;

    for (i = 0; i < g_glowCapacity; i++)
        g_glowEntries[i].type = 0;
    g_glowAllocatedCount = 0;
}

// FUNCTION: CMR2 0x004ae3d0
void Glow_SetEnabled(GlowLight *p, BYTE value)
{
    if (p != NULL)
        p->enabled = value;
}

// FUNCTION: CMR2 0x004ae3f0
void Glow_SetIntensity(GlowLight *p, int value)
{
    if (p != NULL)
        p->intensity = value;
}

// FUNCTION: CMR2 0x004ae410
void Glow_NoOpEntryCallback(BYTE a, BYTE b, int c, int d)
{
}

// Draws a fading rectangle around a point projected onto the given plane.
// The corners and colours use the fixed-point triangle queue's shared scratch.
// FUNCTION: CMR2 0x004ae950
void Graphics_DrawProjectedQuad(GlowLight *pSurface, FixVector *pPoint, FixVector *pTarget, FixVector *pUnused)
{
    FixVector projected;
    FixVector axisA;
    FixVector axisB;
    FixVector toTarget;
    int depth;
    int fade;
    BYTE colour[4];
    int i;

    projected.x = pPoint->x - pSurface->planePoint.x;
    projected.y = pPoint->y - pSurface->planePoint.y;
    projected.z = pPoint->z - pSurface->planePoint.z;
    depth = FixVecDot(&projected, &pSurface->planeNormal);
    FixVecScale(&projected, &pSurface->planeNormal, depth);
    projected.x = pPoint->x - projected.x;
    projected.y = pPoint->y - projected.y;
    projected.z = pPoint->z - projected.z;
    fade = depth;
    if (fade < 0)
        fade = -fade;
    fade -= 0x6666;
    fade = FixMul(fade, 0x1aaac);
    if (fade > 0x10000)
        fade = 0x10000;
    else if (fade < 0)
        fade = 0;
    fade = 0x10000 - fade;

    axisA.x = 0x10000;
    axisA.y = 0;
    axisA.z = 0;
    depth = FixVecDot(&axisA, &pSurface->planeNormal);
    FixVecScale(&toTarget, &pSurface->planeNormal, depth);
    axisA.x -= toTarget.x;
    axisA.y -= toTarget.y;
    axisA.z -= toTarget.z;
    depth = FixVecLength(&axisA);
    if (depth == 0) {
        axisA.x = 0; axisA.y = 0; axisA.z = 0;
    } else {
        FixVecScaleRecip(&axisA, &axisA, depth);
    }
    FixVecCross(&axisB, &axisA, &pSurface->planeNormal);
    depth = FixVecLength(&axisB);
    if (depth == 0) {
        axisB.x = 0; axisB.y = 0; axisB.z = 0;
    } else {
        FixVecScaleRecip(&axisB, &axisB, depth);
    }
    FixVecScale(&axisA, &axisA, FixMul(pSurface->projectedSizeScale, pSurface->sizeX));
    FixVecScale(&axisB, &axisB, FixMul(pSurface->projectedSizeScale, pSurface->sizeX));

    toTarget.x = pTarget->x - projected.x;
    toTarget.y = pTarget->y - projected.y;
    toTarget.z = pTarget->z - projected.z;
    depth = FixVecLength(&toTarget);
    if (depth >= 0x10000) {
        depth = FixDiv(0x10000, depth);
        FixVecScale(&toTarget, &toTarget, depth);
        projected.x += toTarget.x;
        projected.y += toTarget.y;
        projected.z += toTarget.z;
        depth = 0x10000 - depth;
        FixVecScale(&axisA, &axisA, depth);
        FixVecScale(&axisB, &axisB, depth);
    }

    g_projectedQuad[0].x = projected.x + axisA.x;
    g_projectedQuad[0].y = projected.y + axisA.y;
    g_projectedQuad[0].z = projected.z + axisA.z;
    g_projectedQuad[2].x = projected.x - axisA.x;
    g_projectedQuad[2].y = projected.y - axisA.y;
    g_projectedQuad[2].z = projected.z - axisA.z;
    g_projectedQuad[1].x = g_projectedQuad[0].x + axisB.x;
    g_projectedQuad[1].y = g_projectedQuad[0].y + axisB.y;
    g_projectedQuad[1].z = g_projectedQuad[0].z + axisB.z;
    g_projectedQuad[0].x -= axisB.x;
    g_projectedQuad[0].y -= axisB.y;
    g_projectedQuad[0].z -= axisB.z;
    g_projectedQuad[3].x = g_projectedQuad[2].x - axisB.x;
    g_projectedQuad[3].y = g_projectedQuad[2].y - axisB.y;
    g_projectedQuad[3].z = g_projectedQuad[2].z - axisB.z;
    g_projectedQuad[2].x += axisB.x;
    g_projectedQuad[2].y += axisB.y;
    g_projectedQuad[2].z += axisB.z;

    colour[0] = (BYTE)((pSurface->projectedBrightness * FixMul(fade, pSurface->intensity)) >> 16);
    colour[1] = colour[0];
    colour[2] = colour[0];
    colour[3] = 0xff;
    for (i = 0; i < 4; i++)
        *(DWORD *)g_projectedQuad[i].colour = *(DWORD *)colour;
    Quad2D_QueueFixedTriangle(0, &g_projectedQuad[0], &g_projectedQuad[1], &g_projectedQuad[2],
                              pSurface->pTexture, 0xe);
    Quad2D_QueueFixedTriangle(0, &g_projectedQuad[0], &g_projectedQuad[2], &g_projectedQuad[3],
                              pSurface->pTexture, 0xe);
}

// FUNCTION: CMR2 0x004b1970
int CGraphics::GetSharedVertexMemoryKilobytes(void)
{
    return (unsigned int)m_unk0x006dd890 >> 10;
}
// GLOBAL: CMR2 0x006a2a10
Quad2DInputVertex g_layerQuad[4];
// Glow billboard being built; its position anchors the layer quad.
// GLOBAL: CMR2 0x006a2a70
BillboardDef g_glowDef;

// Projects the layer anchor onto a plane, stretches it toward a target and
// draws a four-vertex strip with distance-based greyscale opacity.
// FUNCTION: CMR2 0x004ae470
void Graphics_DrawLayerQuad(GlowLight *pSurface, FixVector *pTarget)
{
    FixVector quad[4];
    FixVector axisA;
    FixVector toTarget;
    FixVector axisB;
    int depth;
    int extension;
    int t;
    BYTE colour[4];
    int i;

    // quad[0] holds the glow position projected onto the layer plane.
    quad[0].x = g_glowDef.pos.x - pSurface->planePoint.x;
    quad[0].y = g_glowDef.pos.y - pSurface->planePoint.y;
    quad[0].z = g_glowDef.pos.z - pSurface->planePoint.z;
    depth = FixVecDot(&quad[0], &pSurface->planeNormal);
    FixVecScale(&quad[0], &pSurface->planeNormal, depth);
    quad[0].x = g_glowDef.pos.x - quad[0].x;
    quad[0].y = g_glowDef.pos.y - quad[0].y;
    quad[0].z = g_glowDef.pos.z - quad[0].z;
    FixVecScale(&axisA, &g_glowBasis[1], FixMul(depth, 0x20000));
    FixVecScale(&axisB, &g_glowBasis[2], FixMul(g_glowDef.top, 0x9999));

    // Pull it towards the target, by at most 10 units.
    toTarget.x = pTarget->x - quad[0].x;
    toTarget.y = pTarget->y - quad[0].y;
    toTarget.z = pTarget->z - quad[0].z;
    t = FixVecLength(&toTarget);
    FixVecScaleRecip(&toTarget, &toTarget, t);
    extension = t - 0x30000;
    if (extension > 0xa0000)
        extension = 0xa0000;
    FixVecScale(&toTarget, &toTarget, extension);
    quad[0].x += toTarget.x;
    quad[0].y += toTarget.y;
    quad[0].z += toTarget.z;

    t = 0x10000 - FixDiv(extension, t);
    FixVecScale(&axisA, &axisA, t);
    FixVecScale(&axisB, &axisB, t);

    quad[2].x = quad[0].x - axisA.x;
    quad[2].y = quad[0].y - axisA.y;
    quad[2].z = quad[0].z - axisA.z;
    quad[1].x = quad[0].x + axisB.x;
    quad[1].y = quad[0].y + axisB.y;
    quad[1].z = quad[0].z + axisB.z;
    quad[0].x -= axisB.x;
    quad[0].y -= axisB.y;
    quad[0].z -= axisB.z;
    quad[3].x = quad[2].x - axisB.x;
    quad[3].y = quad[2].y - axisB.y;
    quad[3].z = quad[2].z - axisB.z;
    quad[2].x += axisB.x;
    quad[2].y += axisB.y;
    quad[2].z += axisB.z;

    t = depth - 0x20000;
    if (t < 0)
        t = 0;
    t = FixMul(t, 0x20000);
    if (t > 0x10000)
        t = 0x10000;
    t = FixMul(0x10000 - t, pSurface->layerIntensity);
    colour[0] = (BYTE)(FixMul(t, (int)g_glowDef.r << 16) >> 16);
    colour[1] = colour[0];
    colour[2] = colour[0];
    colour[3] = 0xff;
    for (i = 0; i < 4; i++) {
        *(FixVector *)&g_layerQuad[i] = quad[i];
        *(DWORD *)g_layerQuad[i].colour = *(DWORD *)colour;
    }
    Quad2D_QueueFixedTriangle(0, &g_layerQuad[0], &g_layerQuad[1], &g_layerQuad[2],
                              pSurface->pLayerTexture, 0x16);
    Quad2D_QueueFixedTriangle(0, &g_layerQuad[0], &g_layerQuad[2], &g_layerQuad[3],
                              pSurface->pLayerTexture, 0x16);
}
// Reserves a free glow slot and copies its position, direction, and draw settings.
// Free glow light slot, or NULL when all are in use.
static inline GlowLight *Glow_FindFreeSlot(void)
{
    int i;

    if (g_glowAllocatedCount < g_glowCapacity) {
        for (i = 0; i < g_glowCapacity; i++) {
            if (g_glowEntries[i].type == 0)
                return &g_glowEntries[i];
        }
    }
    return NULL;
}

// FUNCTION: CMR2 0x004ae2f0
GlowLight *Glow_Add(int type, FixVector *pos, FixVector *dir, FixVector *unused1,
                    int sizeX, int sizeY, Texture *billboardTexture, Texture *layerTexture,
                    int intensity, SceneNode *node, BYTE projectedBrightness, FixVector *unused2,
                    int projectedSizeScale)
{
    GlowLight *light;

    light = Glow_FindFreeSlot();
    if (light == NULL)
        return NULL;
    light->type = type;
    light->pos = *pos;
    if (dir == NULL) {
        light->dir.x = 0;
        light->dir.y = 0;
        light->dir.z = 0;
    } else {
        light->dir = *dir;
    }
    light->sizeX = sizeX;
    light->sizeY = sizeY;
    light->pTexture = billboardTexture;
    light->pLayerTexture = layerTexture;
    light->intensity = intensity;
    light->pNode = node;
    light->enabled = 1;
    light->projectedBrightness = projectedBrightness;
    light->projectedSizeScale = projectedSizeScale;
    g_glowAllocatedCount++;
    return light;
}

// FUNCTION: CMR2 0x004ae2a0
void Glow_SetPosition(GlowLight *pLight, FixVector *pPos, FixVector *pDir)
{
    if (pLight != NULL) {
        if (pPos != NULL)
            pLight->pos = *pPos;
        if (pDir != NULL)
            pLight->dir = *pDir;
    }
}

// FUNCTION: CMR2 0x004ae420
void Glow_SetLayerPlane(GlowLight *pLight, FixVector *pPoint, FixVector *pNormal, int layerIntensity)
{
    if (pLight != NULL) {
        pLight->planePoint = *pPoint;
        pLight->planeNormal = *pNormal;
        pLight->layerIntensity = layerIntensity;
    }
}

// GLOBAL: CMR2 0x006a2aa0
BillboardDef g_glowBillboard;

int FixMatrix_RotateVector(FixVector *pOut, FixVector *pV, FixMatrix *pM);
void FixMatrix_GetPosition(FixVector *pOut, FixMatrix *pM);
void FixMatrix_GetForward(FixVector *pOut, FixMatrix *pM);

// Draws the glow of every enabled light seen from pCamera in view `view`: a
// billboard pulled one unit toward the camera and faded by the viewing angle,
// plus the optional ground layer and projected quads.
// FUNCTION: CMR2 0x004af120
void Glow_Draw(SceneNode *pCamera, BYTE view)
{
    FixMatrix *pCamMatrix;
    GlowLight *pLight;
    FixVector camPos;
    FixVector dir;
    FixVector pos;
    FixVector nodePos;
    FixVector toLight;
    FixVector toCamera;
    BYTE mask;
    int len;
    int d;
    int m;
    int degrees;
    int fade;
    int intensity;
    int inv;
    int scale;
    int i;
    short a;

    pCamMatrix = &pCamera->world;
    FixMatrix_GetPosition(&camPos, pCamMatrix);
    mask = 1 << view;
    FixMatrix_GetForward(&g_glowForward, pCamMatrix);
    g_glowForward.y = 0;
    FIX_NORMALIZE_INTO(g_glowForward, g_glowForward);
    g_glowBasis[2].x = -g_glowForward.z;
    g_glowBasis[2].y = 0;
    g_glowBasis[2].z = g_glowForward.x;

    for (i = 0; i < g_glowCapacity; i++) {
        pLight = &g_glowEntries[i];
        if (pLight->type == 0 || pLight->enabled == 0 || pLight->intensity <= 0)
            continue;
        if (pLight->pNode != NULL && (mask & pLight->pNode->viewMask) == 0)
            continue;
        if (pLight->pNode != NULL && pLight->pNode->pParent != NULL &&
            (mask & pLight->pNode->pParent->viewMask) == 0)
            continue;
        if (pLight->pNode == NULL) {
            dir = pLight->dir;
            pos = pLight->pos;
        } else {
            FixMatrix_RotateVector(&dir, &pLight->dir, &pLight->pNode->world);
            FixMatrix_RotateVector(&pos, &pLight->pos, &pLight->pNode->world);
            FixMatrix_GetPosition(&nodePos, &pLight->pNode->world);
            pos.x += nodePos.x;
            pos.y += nodePos.y;
            pos.z += nodePos.z;
        }

        if (pLight->type == 2 || pLight->type == 3) {
            FixMatrix_GetPosition(&toLight, pCamMatrix);
            toLight.x = pos.x - toLight.x;
            toLight.y = pos.y - toLight.y;
            toLight.z = pos.z - toLight.z;
            // Pre-scale by the largest component so the length cannot overflow.
            if (FIX_ABS(toLight.x) > FIX_ABS(toLight.y) && FIX_ABS(toLight.x) > FIX_ABS(toLight.z))
                m = FIX_ABS(toLight.x);
            else if (FIX_ABS(toLight.y) > FIX_ABS(toLight.x) && FIX_ABS(toLight.y) > FIX_ABS(toLight.z))
                m = FIX_ABS(toLight.y);
            else
                m = FIX_ABS(toLight.z);
            if (m != 0)
                FixVecScaleRecip(&toLight, &toLight, m);
            if (toLight.x == 0 && toLight.y == 0 && toLight.z == 0)
                toLight.x = 0x10000;
            FIX_NORMALIZE_INTO(toLight, toLight);
            d = FixVecDot(&toLight, &dir);
            if (FIX_ABS(d) >= 0xfd70) {
                if (d > 0)
                    degrees = 0;
                else
                    degrees = 0xb40000;
            } else {
                a = FixAcos(d);
                degrees = (0x400 - a) * 0x1680;
            }
            if (degrees > 0x5a0000) {
                degrees -= 0x5a0000;
                fade = FixMul(degrees, 0x3d7);
            } else {
                if (degrees >= 0x5a0000 || pLight->type != 3)
                    goto projected;
                fade = FixMul(0x5a0000 - degrees, 0x3d7);
            }
            if (fade > 0x10000)
                fade = 0x10000;
            else if (fade <= 0)
                goto projected;
        } else {
            fade = 0x10000;
        }

        intensity = FixMul(fade, pLight->intensity);
        Glow_SetSymmetricBounds(&g_glowDef, pLight->sizeX, pLight->sizeY);
        g_glowDef.r = (BYTE)((intensity * 254) >> 16);
        g_glowDef.g = g_glowDef.r;
        g_glowDef.b = g_glowDef.r;
        g_glowDef.a = 0xff;
        g_glowDef.pos = pos;
        g_glowBillboard = g_glowDef;
        toCamera.x = camPos.x - pos.x;
        toCamera.y = camPos.y - pos.y;
        toCamera.z = camPos.z - pos.z;
        len = FixVecLength(&toCamera);
        if (len >= 0x10000) {
            len = FixDiv(0x10000, len);
            FixVecScale(&toCamera, &toCamera, len);
            g_glowBillboard.pos.x += toCamera.x;
            g_glowBillboard.pos.y += toCamera.y;
            g_glowBillboard.pos.z += toCamera.z;
            len = 0x10000 - len;
            g_glowBillboard.bottom = FixMul(g_glowBillboard.bottom, len);
            d = g_glowBillboard.right;
            g_glowBillboard.right = FixMul(d, len);
            g_glowBillboard.top = FixMul(g_glowBillboard.top, len);
            d = g_glowBillboard.left;
            g_glowBillboard.left = FixMul(d, len);
        }
        Billboard_Add(&g_glowBillboard, pLight->pTexture);
        if (pLight->layerIntensity != 0)
            Graphics_DrawLayerQuad(pLight, &camPos);
    projected:
        if (pLight->projectedBrightness != 0)
            Graphics_DrawProjectedQuad(pLight, &pos, &camPos, &dir);
    }
}

// Release callback of Glow_AllocateEntryTable: frees the glow table.
// FUNCTION: CMR2 0x004ae200
int Glow_FreeTable(void)
{
    if (g_glowEntries != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_glowEntries);
        g_glowEntries = NULL;
    }
    g_glowCapacity = 0;
    return 1;
}

// FUNCTION: CMR2 0x004ae170
void Glow_AllocateEntryTable(int param1)
{
    if (g_glowEntries != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_glowEntries);
        g_glowEntries = NULL;
    }
    g_glowEntries = (GlowLight *)CFileBuffer::AllocateLockedBuffer((param1 & 0xff) * sizeof(GlowLight));
    g_glowCapacity = (BYTE)param1;
    g_layerQuad[0].u = 0;
    g_layerQuad[0].v = 0;
    g_layerQuad[1].u = 0xfff9;
    g_layerQuad[1].v = 0;
    g_layerQuad[2].u = 0xfff9;
    g_layerQuad[2].v = 0xfff9;
    g_layerQuad[3].u = 0;
    g_layerQuad[3].v = 0xfff9;
    CGame::RegisterCallback(Glow_FreeTable, NULL);
}

// GLOBAL: CMR2 0x006a2a08
GfxVertexBuffer *g_quadVertexBuffer;

// Release callback of Graphics_CreateSharedWriteOnlyVertexBuffer: releases the quad vertex buffer.
// FUNCTION: CMR2 0x004ae120
int QuadVB_Release(void)
{
    if (g_quadVertexBuffer != NULL) {
        Gfx_DestroyVertexBuffer(g_quadVertexBuffer);
        g_quadVertexBuffer = NULL;
    }
    return 1;
}


// Creates the 0x1000-vertex write-only buffer once and registers its callback.
// FUNCTION: CMR2 0x004ae0a0
void Graphics_CreateSharedWriteOnlyVertexBuffer(void)
{
    // PORT: a renderer vertex buffer.
    g_quadVertexBuffer = Gfx_CreateVertexBuffer(0x1000);
    CGame::RegisterCallback(QuadVB_Release, NULL);
}


// Finds a free entry of the 0x2384 table and initialises its six 0x130-byte
// records, or notifies the failure through FUN_004a76d0(NULL).
// Allocates a free cube-map slot (six 0x130-byte faces) and creates its
// surfaces; returns the render texture (NULL when out of memory).
// match 36%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004a4b10
RenderTexture *Graphics_AllocateCubeMapTextureSlot(void)
{
    Texture *p = NULL;
    int i;
    int j;

    // The original passes the pool byte size as memset's fill value and 0 as
    // the count: the call is a no-op (kept so the code matches the original).
    if (CGraphics::m_unk0x0065fa28 == 0)
        memset(CGraphics::m_pTextureManager->textureBuffer2, 0x9060, 0);
    for (i = 0; i < 0x14; i++) {
        if (CGraphics::m_pTextureManager->textureBuffer2[i] == NULL) {
            CGraphics::m_pTextureManager->textureBuffer2[i] =
                (Texture *)CFileBuffer::AllocateLockedBuffer(sizeof(RenderTexture));
            p = CGraphics::m_pTextureManager->textureBuffer2[i];
            for (j = 0; j < 6; j++) {
                p[j].textureId = (unsigned short)i;
                p[j].field_0x11c = 0;
                p[j].field_0x11e = 0;
                p[j].width = CGraphics::m_cubeMapSize;
                p[j].height = CGraphics::m_cubeMapSize;
            }
            if (p == NULL)
                return NULL;
            CGraphics::m_unk0x0065fa28++;
            break;
        }
    }
    return CGraphics::CreateCubeMapSurfaces((RenderTexture *)p);
}

// GLOBAL: CMR2 0x0067f228
int g_unk0x0067f228;



// Detail level letter ('A' best .. 'D') of each stage/texture group.
// GLOBAL: CMR2 0x0051a3d0
char g_stageQualityCodes[24] = {
    0x43, 0x43, 0x43, 0x43, 0x44, 0x44, 0x44, 0x44, 0x44, 0x43, 0x44, 0x43, 0x44, 0x44, 0x44, 0x44,
    0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
};


// PORT: whether every car gets full detail, as the game does with record
// flag 0x10 (set by the driver name MORRISMODE): the best model and textures
// for each car of each mode, and no simpler body for the distant ones.
// Otherwise only the player's car had them; even at high car detail the other
// cars had models and textures of lower detail, and the distant ones a
// simpler body. [video] car_detail = 0 keeps the original detail, and so does
// the low car detail setting.
bool Graphics_AllCarsFullDetail(void)
{
    if (CGameInfo::IsRecordFlagSet(0x10))
        return true;
    return Sys_GetOption("video.car_detail", 1) != 0 && CGameInfo::GetPreviewMode() != 2;
}

// Raises the detail levels when the hardware allows it (texture memory, caps).
// FUNCTION: CMR2 0x00457c50
void Graphics_RaiseSupportedDetailLevels(void)
{
    // PORT: also with Graphics_AllCarsFullDetail.
    if (Graphics_AllCarsFullDetail() && CFrontend::IsTextureWidthSupported(0x400) != 0 &&
        CFrontend::IsTextureHeightSupported(0x400) != 0) {
        g_stageQualityCodes[0] = 'A';
        g_stageQualityCodes[1] = 'A';
        g_stageQualityCodes[3] = 'A';
        g_stageQualityCodes[2] = 'A';
        g_stageQualityCodes[4] = 'C';
        g_stageQualityCodes[5] = 'C';
        g_stageQualityCodes[6] = 'C';
        g_stageQualityCodes[7] = 'A';
        g_stageQualityCodes[8] = 'A';
        g_stageQualityCodes[13] = 'A';
        g_stageQualityCodes[14] = 'A';
        g_stageQualityCodes[15] = 'A';
        g_stageQualityCodes[16] = 'A';
        g_stageQualityCodes[17] = 'A';
        g_stageQualityCodes[18] = 'A';
        g_stageQualityCodes[19] = 'A';
        g_knockoutPlayerCarDetail = 'A';
        g_knockoutOpponentCarDetail = 'A';
        g_stageQualityCodes[11] = 'A';
        g_stageQualityCodes[12] = 'A';
        // PORT: flag 0x10 left the arcade cars (4, 5) at 'C', below the
        // player's 'A' at high car detail, and the super special cars (9, 10)
        // at 'C' and 'D'. Every car gets the player's model.
        g_stageQualityCodes[4] = 'A';
        g_stageQualityCodes[5] = 'A';
        g_stageQualityCodes[9] = 'A';
        g_stageQualityCodes[10] = 'A';
        return;
    }
    switch (CGameInfo::GetPreviewMode()) {
    case 2:
        g_stageQualityCodes[0] = 'C';
        g_stageQualityCodes[1] = 'C';
        g_stageQualityCodes[3] = 'D';
        g_stageQualityCodes[2] = 'D';
        g_stageQualityCodes[4] = 'D';
        g_stageQualityCodes[5] = 'E';
        g_stageQualityCodes[6] = 'F';
        g_stageQualityCodes[7] = 'D';
        g_stageQualityCodes[8] = 'D';
        g_stageQualityCodes[13] = 'D';
        g_stageQualityCodes[14] = 'D';
        g_stageQualityCodes[15] = 'D';
        g_stageQualityCodes[16] = 'D';
        g_stageQualityCodes[17] = 'D';
        g_stageQualityCodes[18] = 'E';
        g_stageQualityCodes[19] = 'F';
        g_knockoutPlayerCarDetail = 'D';
        g_knockoutOpponentCarDetail = 'D';
        g_stageQualityCodes[11] = 'C';
        g_stageQualityCodes[12] = 'D';
        break;
    case 0:
        g_stageQualityCodes[0] = 'A';
        g_stageQualityCodes[1] = 'A';
        g_stageQualityCodes[4] = 'A';
        g_stageQualityCodes[5] = 'C';
        g_stageQualityCodes[6] = 'D';
        g_stageQualityCodes[7] = 'A';
        g_stageQualityCodes[8] = 'C';
        g_stageQualityCodes[13] = 'D';
        g_stageQualityCodes[14] = 'D';
        g_stageQualityCodes[3] = 'C';
        g_stageQualityCodes[2] = 'A';
        g_stageQualityCodes[17] = 'D';
        g_stageQualityCodes[18] = 'E';
        g_stageQualityCodes[19] = 'F';
        g_knockoutPlayerCarDetail = 'A';
        g_knockoutOpponentCarDetail = 'C';
        g_stageQualityCodes[11] = 'A';
        g_stageQualityCodes[12] = 'D';
        break;
    }
}


// GLOBAL: CMR2 0x0052109c
float g_playbackRate = 1.0f;
// GLOBAL: CMR2 0x005210a0
float g_frameRate = 25.0f;
// GLOBAL: CMR2 0x005210a4
float g_averageFps = 25.0f;
// GLOBAL: CMR2 0x005210a8
float g_fps = 25.0f;
extern int g_unk0x005210ac;
#define g_fpsWarmup g_unk0x005210ac
// GLOBAL: CMR2 0x005210b0
float g_frameRate2 = 25.0f;
// GLOBAL: CMR2 0x006dd9a8
int g_averageFpsDone;
// GLOBAL: CMR2 0x006dd9b0
int g_frameCount;
// GLOBAL: CMR2 0x006dd9b4
int g_framesThisSecond;
// GLOBAL: CMR2 0x006dd9b8
unsigned int g_lastSecond;
// GLOBAL: CMR2 0x006dd9bc
unsigned short g_unk0x006dd9bc[2000];
extern const float g_netFontInverseScale;

// The four timestamps are guarded function statics in the original; modelled
// here as real globals so the initialisation guard and its addresses are explicit.
// GLOBAL: CMR2 0x006dd894
static BYTE g_unk0x006dd894;
// GLOBAL: CMR2 0x006dd8a0
static unsigned int g_unk0x006dd8a0;
// GLOBAL: CMR2 0x006dd8a4
static unsigned int g_unk0x006dd8a4;
// GLOBAL: CMR2 0x006dd898
static unsigned int g_unk0x006dd898;
// GLOBAL: CMR2 0x006dd89c
static unsigned int g_unk0x006dd89c;

// Frame timing: average fps after a 3 s warm-up, fps of the last second and
// the rate of the current frame (1000 / frame time in whole ms, infinite when
// the frame took under one).
// FUNCTION: CMR2 0x004b21e0
void Graphics_UpdateFrameStatistics(void)
{
    double elapsed;

    if ((g_unk0x006dd894 & 1) == 0) {
        g_unk0x006dd894 |= 1;
        g_unk0x006dd8a0 = CMain::GetFrameTime();
    }
    if ((g_unk0x006dd894 & 2) == 0) {
        g_unk0x006dd894 |= 2;
        g_unk0x006dd8a4 = CMain::GetFrameTime();
    }
    if ((g_unk0x006dd894 & 4) == 0) {
        g_unk0x006dd894 |= 4;
        g_unk0x006dd898 = CMain::GetFrameTime() - 40;
    }
    if ((g_unk0x006dd894 & 8) == 0) {
        g_unk0x006dd894 |= 8;
        g_unk0x006dd89c = CMain::GetFrameTime();
    }

    g_unk0x006dd898 = g_unk0x006dd8a4;
    g_unk0x006dd8a4 = CMain::GetFrameTime();
    g_frameCount++;
    g_framesThisSecond++;
    if (g_fpsWarmup != 0) {
        if (g_unk0x006dd8a4 - g_unk0x006dd8a0 > 3000) {
            g_fpsWarmup = 0;
            g_averageFpsDone = 0;
            g_unk0x006dd8a0 = g_unk0x006dd8a4;
            g_frameCount = 0;
            g_averageFps = 0.0f;
        }
    } else {
        if (g_averageFpsDone == 0 && g_unk0x006dd8a4 - g_unk0x006dd8a0 > 3000) {
            g_averageFpsDone = 1;
            g_averageFps = X87S_DIV(X87S_MUL(X87S_FILD(float, g_frameCount), g_netFontInverseScale), (double)(g_unk0x006dd8a4 - g_unk0x006dd8a0));
        }
        if (g_lastSecond != g_unk0x006dd8a4 / 1000) {
            g_lastSecond = g_unk0x006dd8a4 / 1000;
            elapsed = (double)(g_unk0x006dd8a4 - g_unk0x006dd89c);
            g_unk0x006dd89c = g_unk0x006dd8a4;
            g_fps = X87S_DIV(X87S_MUL(X87S_FILD(float, g_framesThisSecond), g_netFontInverseScale), elapsed);
            g_framesThisSecond = 0;
        }
    }
    {
        // PORT: the original divided by the frame time in whole milliseconds.
        // Above 1000 frames a second some frames last under one, the rate
        // became infinite and the weather took its longest step (that of 15
        // frames a second) on them: the clouds turned and the wind changed
        // tens of times too fast (#29). The frame time is measured in
        // fractions of a millisecond, and a frame the clock did not see keeps
        // the last rate.
        static double previous = Sys_GetTicksPrecise() - 40.0;
        double now = Sys_GetTicksPrecise();
        double dt = now - previous;

        previous = now;
        if (dt > 0.0) {
            g_frameRate2 = X87S_DIV(g_netFontInverseScale, dt);
            g_frameRate = X87S_DIV(g_netFontInverseScale, dt);
        }
    }
}

// FUNCTION: CMR2 0x004b23a0
float Graphics_GetFrameRate(void)
{
    return g_frameRate;
}

// OpenCMR2: whether the in-game debug overlay (FPS and draw stats) is shown:
// the original graphics debug flag (bit 2 of field913_0x3bc) or [video]
// video.debug_draw.
bool Graphics_DebugDrawWanted(void)
{
    return (g_pGraphics->field913_0x3bc & 4) != 0 || Sys_GetOption("video.debug_draw", OPENCMR2_DEBUG_DRAW_DEFAULT) != 0;
}

// FUNCTION: CMR2 0x004b23b0
void Graphics_SetPlaybackRate(float value)
{
    g_playbackRate = value;
}

#include "Particle.h"

extern double g_minus65536;

// GLOBAL: CMR2 0x005112f0
float g_oneOverRandMax = 1.0f / RAND_MAX;
// GLOBAL: CMR2 0x006a2cd0
int g_particleTypeCount;
// GLOBAL: CMR2 0x006a2cd4
int g_particleCount;
// GLOBAL: CMR2 0x006a2cd8
ParticleType *g_particleTypes;
// GLOBAL: CMR2 0x006a2cdc
Particle *g_particles;
// GLOBAL: CMR2 0x006a2ce0
ParticleType *g_pEditParticleType;
// GLOBAL: CMR2 0x006a2ce4
int g_nextParticle;

// Selects the particle type to edit and resets it to defaults.
// FUNCTION: CMR2 0x004af940
void ParticleEdit_Select(int index)
{
    if (index >= 0 && index < g_particleTypeCount) {
        g_pEditParticleType = &g_particleTypes[index];
        g_pEditParticleType->lifetime = 0;
        g_pEditParticleType->gravity = 0;
        g_pEditParticleType->drag = 0;
        g_pEditParticleType->bounce = 0;
        g_pEditParticleType->friction = 0;
        g_pEditParticleType->type = 0x80;
        g_pEditParticleType->alphaEnd = 0;
        g_pEditParticleType->alphaStep = 0;
        g_pEditParticleType->colour[0] = 0xff;
        g_pEditParticleType->colour[1] = 0xff;
        g_pEditParticleType->colour[2] = 0xff;
        g_pEditParticleType->texture = NULL;
        g_pEditParticleType->frames = NULL;
        g_pEditParticleType->field0x50 = 0;
        g_pEditParticleType->field0x54 = 0;
        g_pEditParticleType->field0x58 = 0;
        g_pEditParticleType->postUpdate = NULL;
        g_pEditParticleType->callback = NULL;
        g_pEditParticleType->field0x6c = 0;
        g_pEditParticleType->update = NULL;
        g_pEditParticleType->draw = 0;
        g_pEditParticleType->flags &= 0xfe;
        g_pEditParticleType->flags &= 0xfd;
        g_pEditParticleType->flags &= 0xfb;
        g_pEditParticleType->flags &= 0xf7;
        g_pEditParticleType->flags &= 0xef;
        g_pEditParticleType->flags &= 0xdf;
        g_pEditParticleType->flags &= 0xbf;
        g_pEditParticleType->flags &= 0x7f;
        g_pEditParticleType->directionFlags &= 0xfe;
        g_pEditParticleType->directionFlags &= 0xfd;
        g_pEditParticleType->directionFlags &= 0xfb;
        g_pEditParticleType->directionFlags &= 0xf7;
        g_pEditParticleType->spread.x = 0;
        g_pEditParticleType->spread.y = 0;
        g_pEditParticleType->spread.z = 0;
        g_pEditParticleType->texture = NULL;
        return;
    }
    g_pEditParticleType = NULL;
}

// Particle type editor: setters for g_pEditParticleType.

// Copies a template particle type (flag 1) into the type being edited.
// FUNCTION: CMR2 0x004afb20
void ParticleEdit_CopyTemplate(int index)
{
    ParticleType *pSrc;

    if (g_pEditParticleType != NULL && index >= 0 && index < g_particleTypeCount) {
        pSrc = &g_particleTypes[index];
        if (pSrc->flags & 1)
            *g_pEditParticleType = *pSrc;
    }
}

// FUNCTION: CMR2 0x004afb70
void ParticleEdit_SetTextureParams(Texture *texture, int b, int c, int d, int e)
{
    if (g_pEditParticleType != NULL) {
        g_pEditParticleType->texture = texture;
        g_pEditParticleType->field0x3c = b;
        g_pEditParticleType->field0x40 = c;
        g_pEditParticleType->field0x44 = d;
        g_pEditParticleType->field0x48 = e;
    }
}

// FUNCTION: CMR2 0x004afbc0
void ParticleEdit_SetExtendedParams(Texture **frames, int b, int c, int d, BYTE flag4, BYTE flag8, int e, int f, int g, int h)
{
    if (g_pEditParticleType != NULL) {
        g_pEditParticleType->frames = frames;
        g_pEditParticleType->field0x50 = b;
        g_pEditParticleType->field0x54 = c;
        g_pEditParticleType->field0x58 = d;
        g_pEditParticleType->directionFlags = (flag4 & 1) << 2 | g_pEditParticleType->directionFlags & 0xfb;
        g_pEditParticleType->directionFlags = (flag8 & 1) << 3 | g_pEditParticleType->directionFlags & 0xf7;
        g_pEditParticleType->field0x3c = e;
        g_pEditParticleType->field0x40 = f;
        g_pEditParticleType->field0x44 = g;
        g_pEditParticleType->field0x48 = h;
    }
}

// FUNCTION: CMR2 0x004afc70
void ParticleEdit_SetMotion(int lifetime, int gravity, int drag, int bounce, int friction, char bounces,
                            BYTE killBelowFloor)
{
    if (g_pEditParticleType != NULL) {
        g_pEditParticleType->lifetime = lifetime;
        g_pEditParticleType->gravity = gravity;
        g_pEditParticleType->drag = drag;
        g_pEditParticleType->bounce = bounce;
        g_pEditParticleType->friction = friction;
        g_pEditParticleType->flags = g_pEditParticleType->flags & 0x7f | bounces << 7;
        g_pEditParticleType->flags = (killBelowFloor & 1) << 1 | g_pEditParticleType->flags & 0xfd;
    }
}

// FUNCTION: CMR2 0x004afcf0
void ParticleEdit_SetAlphaRamp(BYTE start, BYTE end, char step, BYTE killAtEnd)
{
    if (g_pEditParticleType != NULL) {
        g_pEditParticleType->type = start;
        g_pEditParticleType->alphaEnd = end;
        g_pEditParticleType->alphaStep = step;
        g_pEditParticleType->flags = ((step != 0) & 1) << 6 | g_pEditParticleType->flags & 0xbf;
        g_pEditParticleType->flags = (killAtEnd & 1) << 2 | g_pEditParticleType->flags & 0xfb;
    }
}

// FUNCTION: CMR2 0x004afd60
void ParticleEdit_SetColour(BYTE r, BYTE g, BYTE b, BYTE flag)
{
    if (g_pEditParticleType != NULL) {
        g_pEditParticleType->directionFlags = (flag & 1) << 1 | g_pEditParticleType->directionFlags & 0xfd;
        g_pEditParticleType->colour[0] = r;
        g_pEditParticleType->colour[1] = g;
        g_pEditParticleType->colour[2] = b;
    }
}

// FUNCTION: CMR2 0x004afdb0
void ParticleEdit_SetSizeRamp(int size, int target, int step)
{
    if (g_pEditParticleType != NULL) {
        g_pEditParticleType->flags &= 0xef;
        g_pEditParticleType->size = size;
        g_pEditParticleType->sizeVariation = target;
        g_pEditParticleType->sizeStep = step;
        g_pEditParticleType->flags = ((step != 0) & 1) << 3 | g_pEditParticleType->flags & 0xf7;
    }
}

// FUNCTION: CMR2 0x004afe10
void ParticleEdit_SetSizeRange(int min, int max)
{
    if (g_pEditParticleType != NULL) {
        g_pEditParticleType->flags &= 0xf7;
        g_pEditParticleType->sizeStep = 0;
        if (min < max) {
            g_pEditParticleType->size = min;
            g_pEditParticleType->sizeVariation = max - min;
            g_pEditParticleType->flags |= 0x10;
            return;
        }
        g_pEditParticleType->size = min;
        g_pEditParticleType->sizeVariation = 0;
        g_pEditParticleType->flags &= 0xef;
    }
}

// FUNCTION: CMR2 0x004afec0
void ParticleEdit_SetSpread(int x, int y, int z, BYTE flag)
{
    if (g_pEditParticleType != NULL) {
        g_pEditParticleType->spread.x = x;
        g_pEditParticleType->spread.y = y;
        g_pEditParticleType->spread.z = z;
        g_pEditParticleType->directionFlags = (g_pEditParticleType->directionFlags ^ flag) & 1 ^
                                              g_pEditParticleType->directionFlags;
    }
}

// FUNCTION: CMR2 0x004aff10
void ParticleEdit_SetCallbacks(ParticleDrawCallback draw, ParticleUpdateCallback update,
                               ParticleUpdateCallback postUpdate,
                               ParticleInitCallback callback, int field0x6c)
{
    if (g_pEditParticleType != NULL) {
        g_pEditParticleType->postUpdate = postUpdate;
        g_pEditParticleType->callback = callback;
        g_pEditParticleType->field0x6c = field0x6c;
        g_pEditParticleType->update = update;
        g_pEditParticleType->draw = draw;
    }
}

// FUNCTION: CMR2 0x004afe80
void ParticleEdit_SetField2CAndFlag5(short param1)
{
    if (g_pEditParticleType != NULL) {
        g_pEditParticleType->field0x2c = param1;
        g_pEditParticleType->flags = ((param1 != 0) & 1) << 5 | g_pEditParticleType->flags & 0xdf;
    }
}

// FUNCTION: CMR2 0x004aff60
void ParticleEdit_CommitCurrentType(void)
{
    if (g_pEditParticleType != NULL) {
        g_pEditParticleType->flags |= 1;
        g_pEditParticleType = NULL;
    }
}

// FUNCTION: CMR2 0x004b0100
void Particle_Kill(Particle *p)
{
    p->active = 0;
}

// FUNCTION: CMR2 0x004aff80
void Particle_KillAll(void)
{
    worldDust.Clear();
    int i;
    Particle *p = g_particles;

    for (i = 0; i < g_particleCount; i++, p++) {
        if (p->active)
            Particle_Kill(p);
    }
}

// FUNCTION: CMR2 0x004affb0
void Particle_ClearActiveFlags(void)
{
    worldDust.Clear();
    int i;
    BYTE *p;

    i = 0;
    if (g_particleCount > 0) {
        p = &g_particles->active;
        do {
            *p = 0;
            i++;
            p += sizeof(Particle);
        } while (i < g_particleCount);
    }
}

// FUNCTION: CMR2 0x004affe0
void ParticleType_ClearEnabledFlags(void)
{
    int i;
    BYTE *p;

    i = 0;
    if (g_particleTypeCount > 0) {
        p = &g_particleTypes->flags;
        do {
            *p &= ~1;
            i++;
            p += sizeof(ParticleType);
        } while (i < g_particleTypeCount);
    }
}

void Particle_ClearActiveFlags(void);

// Release callback of the particle system.
// FUNCTION: CMR2 0x004b0010
BYTE Particle_Shutdown(void)
{
    ParticleType_ClearEnabledFlags();
    Particle_ClearActiveFlags();
    if (g_particleTypes != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_particleTypes);
        g_particleTypes = NULL;
    }
    if (g_particles != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_particles);
        g_particles = NULL;
    }
    g_particleTypeCount = 0;
    g_particleCount = 0;
    return 1;
}

// Allocates the particle type table and the particle pool.
// FUNCTION: CMR2 0x004b0060
void Particle_Init(int typeCount, int particleCount)
{
    g_particleTypes = (ParticleType *)CFileBuffer::AllocateLockedBuffer(typeCount * sizeof(ParticleType));
    g_particles = (Particle *)CFileBuffer::AllocateLockedBuffer(particleCount * sizeof(Particle));
    if (g_particleTypes != NULL) {
        if (g_particles != NULL) {
            g_particleTypeCount = typeCount;
            g_particleCount = particleCount;
            ParticleType_ClearEnabledFlags();
            Particle_ClearActiveFlags();
            CGame::RegisterCallback(Particle_Shutdown, NULL);
            return;
        }
        CFileBuffer::FreeGenericFileBuffer(g_particleTypes);
        g_particleTypes = NULL;
    }
    if (g_particles != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_particles);
        g_particles = NULL;
    }
    g_particleTypeCount = 0;
    g_particleCount = 0;
}

// FUNCTION: CMR2 0x004b1140
void Particle_SetPoolCapacity(int count)
{
    g_particleCount = count;
}

// Wind the particle drag pulls toward.
// GLOBAL: CMR2 0x006a2ce8
FixVector g_particleWind;
FixVector g_airMoteDrift = {0,0,0};
float g_airMoteSunScatter = 0.f;
unsigned g_airMoteNextSeed = 0;
int g_snowGlintNextPeak = 0;
FogLightSource g_fogLightSources[LIGHT_FIELD_MAX];
int g_fogLightSourceCount = 0;

// Advances every active particle by one step: age, drag, gravity, alpha, size
// and angle ramps, floor kill or bounce (unless the type has its own update).
// match 50%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004b0110
void Particle_UpdateAll(SceneNode *pView)
{
    Particle *p;
    ParticleType *pType;
    FixVector wind;
    FixVector rel;
    int keep;
    int a;
    int size;
    short angle;
    int i;

    p = g_particles;
    for (i = 0; i < g_particleCount; i++, p++) {
        if (p->active == 0)
            continue;
        pType = p->pType;
        p->vector0x10 = p->vector0x28;
        p->age -= 0x10000;
        if (p->age <= 0) {
            Particle_Kill(p);
            continue;
        }
        if (pType->update != NULL) {
            pType->update(p, pType, pView);
        } else {
            if (pType->drag > 0) {
                keep = 0x10000 - FixMul(pType->drag, 0x290);
                FixVecScale(&wind, &g_particleWind, 0x1999);
                rel.x = p->position.x - wind.x;
                rel.y = p->position.y - wind.y;
                rel.z = p->position.z - wind.z;
                FixVecScale(&rel, &rel, keep);
                p->position.x = rel.x + wind.x;
                p->position.y = rel.y + wind.y;
                p->position.z = rel.z + wind.z;
            }
            p->position.y -= pType->gravity;
            p->sourceVector.x += p->position.x;
            p->sourceVector.y += p->position.y + pType->gravity / 2;
            p->sourceVector.z += p->position.z;
            if (pType->flags & 0x40) {
                p->type0x52 = p->type0x56;
                a = p->type0x56;
                if (pType->type < pType->alphaEnd) {
                    a += pType->alphaStep;
                    if (a > pType->alphaEnd)
                        a = pType->alphaEnd;
                } else if (pType->type > pType->alphaEnd) {
                    a -= pType->alphaStep;
                    if (a < pType->alphaEnd)
                        a = pType->alphaEnd;
                }
                p->type0x56 = (BYTE)a;
                if ((pType->flags & 4) && a == pType->alphaEnd) {
                    Particle_Kill(p);
                    continue;
                }
            }
            if (pType->flags & 8) {
                size = p->size;
                if (pType->size < pType->sizeVariation) {
                    size += pType->sizeStep;
                    if (size > pType->sizeVariation)
                        size = pType->sizeVariation;
                } else if (pType->size > pType->sizeVariation) {
                    size -= pType->sizeStep;
                    if (size < pType->sizeVariation)
                        size = pType->sizeVariation;
                }
                p->size = size;
            }
            if (pType->flags & 0x20) {
                angle = p->field0x50 + pType->field0x2c;
                p->field0x50 = angle;
                if (angle > 0x1000)
                    p->field0x50 = angle - 0x1000;
                else if (angle < 0)
                    p->field0x50 = angle + 0x1000;
            }
            if ((pType->flags & 2) && p->sourceVector.y < p->field0x48) {
                Particle_Kill(p);
                continue;
            }
            if (pType->flags & 0x80) {
                keep = 0x10000 - pType->friction;
                if (p->field0x58 != 0) {
                    p->sourceVector.y = p->field0x48;
                    p->position.x = FixMul(p->position.x, keep);
                    p->position.z = FixMul(p->position.z, keep);
                } else {
                    if (p->sourceVector.y < p->field0x48) {
                        p->position.y = -FixMul(p->position.y, pType->bounce);
                        p->sourceVector.y = p->field0x48;
                        p->position.x = FixMul(p->position.x, keep);
                        p->position.z = FixMul(p->position.z, keep);
                        if (p->position.y < 0x1999) {
                            p->position.y = 0;
                            p->field0x58 = 1;
                        }
                    }
                }
            }
            p->vector0x28 = p->sourceVector;
        }
        if (pType->postUpdate != NULL)
            pType->postUpdate(p, pType, pView);
    }
}

// Interpolates every active particle between its last two positions (and
// alpha values) by t (0..1) for drawing.
// match 42%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004b06a0
void Particle_Interpolate(int t)
{
    Particle *p;
    FixVector d;
    int i;

    i = 0;
    if (g_particleCount > 0) {
        p = g_particles;
        do {
            if (p->active != 0) {
                d.x = p->vector0x28.x - p->vector0x10.x;
                d.y = p->vector0x28.y - p->vector0x10.y;
                d.z = p->vector0x28.z - p->vector0x10.z;
                FixVecScale(&d, &d, t);
                p->vector0x1c.x = p->vector0x10.x + d.x;
                p->vector0x1c.y = p->vector0x10.y + d.y;
                p->vector0x1c.z = p->vector0x10.z + d.z;
                p->type0x53 = (BYTE)((p->type0x52 * 0x10000 + FixMul((p->type0x56 - p->type0x52) * 0x10000, t)) >> 16);
            }
            i++;
            p++;
        } while (i < g_particleCount);
    }
}

// Queues a billboard for every active particle visible in view `view`
// (animated texture frames, size scaling, spin, lighting), or calls the
// type's own draw callback.
// The original walks the 0x68-byte particle records with a byte cursor held
// at particle+0x55 (the view mask and the active flag are the first fields
// the loop tests), so the body addresses the record through that cursor:
//   pb-0x55 pType        pb-0x39 vector0x1c   pb-0x15 field0x40  pb-0x11 age
//   pb-9    size         pb-5    field0x50   pb-2    type0x53   pb-1  field0x54
//   pb+2    active       pb+0    field0x55   pb+4..6 colour      pb+0xb field0x60
// match 40%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// Optional streak look for the speed motes (video.speed_motes_stretch, 0 to 100, default 0 = round dots). The billboard
// quad is stretched along the screen-space direction from the view axis to the mote, which is the direction a
// world-fixed point moves across the screen as the car approaches it, and rotated to match (BillboardDef rotation is a
// 12-bit angle about the view axis, see Billboard_Draw). Nearby motes fade out so a streak never fills the screen.
// Visual only: reads the camera node and the mote's own position.
static float g_speedMoteRatio = 0.f;
void SpeedMote_ApplyStreak(BillboardDef *def, SceneNode *camera)
{
    int sizePct = SpeedMote_SizePercent(Sys_GetOption("video.speed_motes_size", 100));
    if (sizePct != 100) {   // bigger specks for diagnosis; 100 = the original size
        def->top = (int)((long long)def->top * sizePct / 100); def->bottom = (int)((long long)def->bottom * sizePct / 100);
        def->left = (int)((long long)def->left * sizePct / 100); def->right = (int)((long long)def->right * sizePct / 100);
    }
    {   // Diagnostic: the candidate quad of one mote (before Billboard_Add accepts it) about every 300 draws (camera-space offsets, quad half-sizes, alpha).
        static unsigned calls = 0;
        if (camera && ++calls % 300 == 1) {
            const FixMatrix &cm = camera->world;
            float dx = def->pos.x - cm.position.x, dy = def->pos.y - cm.position.y, dz = def->pos.z - cm.position.z;
            const float sc = 1.f / (65536.f * 65536.f);
            Sys_Log("speed mote candidate quad: depth=%.1f right=%.1f up=%.1f half_w=%.3f half_h=%.3f alpha=%d size_pct=%d (16.16 world units, camera node axes)",
                (dx * cm.forward.x + dy * cm.forward.y + dz * cm.forward.z) * sc, (dx * cm.right.x + dy * cm.right.y + dz * cm.right.z) * sc,
                (dx * cm.up.x + dy * cm.up.y + dz * cm.up.z) * sc, (def->right - def->left) / 131072.f, (def->top - def->bottom) / 131072.f,
                (int)def->a, sizePct);
        }
    }
    int stretch = Sys_GetOption("video.speed_motes_stretch", 0);
    if (stretch <= 0 || !camera) return;
    const FixMatrix &m = camera->world;
    float rx = def->pos.x - m.position.x, ry = def->pos.y - m.position.y, rz = def->pos.z - m.position.z;
    const float scale = 1.f / (65536.f * 65536.f); // both vectors are 16.16
    float right = (rx * m.right.x + ry * m.right.y + rz * m.right.z) * scale;
    float up = (rx * m.up.x + ry * m.up.y + rz * m.up.z) * scale;
    float depth = (rx * m.forward.x + ry * m.forward.y + rz * m.forward.z) * scale;
    int angle; float lengthMul, alphaMul;
    if (!SpeedMote_StreakParams(right, up, depth, g_speedMoteRatio, stretch, &angle, &lengthMul, &alphaMul)) return;
    def->field_0x20 = (short)angle;
    def->left = (int)(def->left * lengthMul);
    def->right = (int)(def->right * lengthMul);
    def->a = (BYTE)(def->a * alphaMul);
}
#define DRAW_PARTICLE (*(Particle *)(pb - offsetof(Particle, field0x55)))
// FUNCTION: CMR2 0x004b0480
static unsigned g_speedMotesRefused=0,g_speedMotesDrawn=0,g_speedMotesSpawned=0,g_speedMotesBudgetSkipped=0,g_speedMotesNoGround=0;
static unsigned g_airMotesRefused=0,g_airMotesDrawn=0,g_airMotesSpawned=0,g_airMotesBudgetSkipped=0;
void Particle_DrawAll(SceneNode *pCamera, BYTE view)
{
    BillboardDef def;
    ParticleType *pType;
    int frame;
    int elapsed;
    Texture *texture;
    int i;
    BYTE mask;
    BYTE *pb;

    def.flags = 0;
    GfxDustLobe lobes[32]={};int lobeCount=0;
    bool volume=Sys_GetOption("video.dust_volume",1)!=0;
    lobeCount=worldDust.Sample(lobes,32);
    for(int i=0;i<lobeCount;i++)lobes[i].colourDensity[3]*=Effect_Clamp(Sys_GetOption("video.dust_amount",100))/100.f;
    if(volume && lobeCount && pCamera){FixVector cam;FixMatrix_GetPosition(&cam,&pCamera->world);
        Gfx_NoteDustSamplingCamera(cam.x/65536.f,cam.y/65536.f,cam.z/65536.f,
            pCamera->world.forward.x/65536.f,pCamera->world.forward.y/65536.f,pCamera->world.forward.z/65536.f);}
    bool volumeQueued=volume && lobeCount && Gfx_DrawDustVolume(lobes,lobeCount);
    // Never draw the camera-facing cloud sheets over a successful world-volume
    // submission. Original wheel chips/spray remain separate.
    mask = 1 << view;
    // No live cloud can exist when the cap is zero or the reserved pool is
    // exhausted. Keep baseline wheel dust visible with the same soft-smoke
    // material, without changing births, pool ownership or simulation.
    bool fallbackCloudPresent=false;
    if(g_particleTypes && g_particleTypeCount>DUST_PLUME_TYPE)
        for(int j=0;j<g_particleCount;j++)
            if(g_particles[j].active && (g_particles[j].field0x55&mask) &&
               g_particles[j].pType==&g_particleTypes[DUST_PLUME_TYPE]){
                fallbackCloudPresent=true;break;
            }
    if (0 < g_particleCount) {
        pb = (BYTE *)g_particles + offsetof(Particle, field0x55);
        for (i = 0; i < g_particleCount; i++, pb += sizeof(Particle)) {
            if (DRAW_PARTICLE.active == 0 || (DRAW_PARTICLE.field0x55 & mask) == 0)
                continue;
            pType = DRAW_PARTICLE.pType;
            // Only dry wheel smoke explicitly tagged at emission is replaced.
            // Snow, water, debris and other uses of these templates stay intact.
            // Dry wheel sheets use leaf/grass sprite frames. They are not
            // the soft-smoke cloud fallback. If no replacement cloud exists,
            // reuse this baseline particle only as soft smoke at draw time.
            // Its leaf frame, callbacks and motion streak are never drawn.
            bool baselineDryCloud=DRAW_PARTICLE.dryWheelDust;
            if(baselineDryCloud){
                if(volumeQueued || fallbackCloudPresent)continue;
                if(!g_particleTypes || g_particleTypeCount<=DUST_PLUME_TYPE)continue;
                pType=&g_particleTypes[DUST_PLUME_TYPE];
                texture=pType->texture;
                goto draw;
            }
            if(volumeQueued && pType==&g_particleTypes[DUST_PLUME_TYPE])continue;
            if (pType->draw != 0) {
                pType->draw(&DRAW_PARTICLE, pType, pCamera);
                continue;
            }
            if (pType->frames != NULL) {
                frame = 0;
                elapsed = pType->lifetime - DRAW_PARTICLE.age;
                if (elapsed > pType->field0x54 && pType->field0x58 > 0) {
                    frame = FixDiv(elapsed - pType->field0x54, pType->field0x58) >> 16;
                    if (pType->directionFlags & 8)
                        frame += ((UINT_PTR)&DRAW_PARTICLE & 0xffff) % (unsigned int)pType->field0x50;
                    if (frame >= pType->field0x50 && (pType->directionFlags & 4) == 0) {
                        texture = pType->frames[pType->field0x50 - 1];
                        goto draw;
                    }
                    frame %= pType->field0x50;
                }
                texture = pType->frames[frame];
            } else {
                texture = DRAW_PARTICLE.texture;
            }
        draw:
            if (texture == NULL)
                continue;
            if (DRAW_PARTICLE.size == 0x10000) {
                def.top = pType->field0x3c;
                def.left = pType->field0x40;
                def.bottom = pType->field0x44;
                def.right = pType->field0x48;
            } else {
                def.top = FixMul(pType->field0x3c, DRAW_PARTICLE.size);
                def.left = FixMul(DRAW_PARTICLE.size, pType->field0x40);
                def.bottom = FixMul(pType->field0x44, DRAW_PARTICLE.size);
                def.right = FixMul(pType->field0x48, DRAW_PARTICLE.size);
            }
            if ((pType->flags & 0x20) != 0)
                def.field_0x20 = DRAW_PARTICLE.field0x50;
            else
                def.field_0x20 = 0;
            def.pos = DRAW_PARTICLE.vector0x1c;
            if (DRAW_PARTICLE.attachmentMatrix != 0) {
                def.pos.x += DRAW_PARTICLE.attachmentMatrix->position.x;
                def.pos.y += DRAW_PARTICLE.attachmentMatrix->position.y;
                def.pos.z += DRAW_PARTICLE.attachmentMatrix->position.z;
            }
            def.flags ^= (pType->directionFlags ^ def.flags) & 2;
            def.r = DRAW_PARTICLE.colour[0];
            def.g = DRAW_PARTICLE.colour[1];
            def.b = DRAW_PARTICLE.colour[2];
            def.a = DRAW_PARTICLE.type0x53;
            if(baselineDryCloud){
                float r=def.r/255.f,g=def.g/255.f,b=def.b/255.f;
                DustPlume_CloudTint(r,g,b);
                def.r=(BYTE)(r*255.f+.5f);def.g=(BYTE)(g*255.f+.5f);def.b=(BYTE)(b*255.f+.5f);
            }
            if(pType==&g_particleTypes[DUST_PLUME_TYPE])def.a=(BYTE)Effect_Scale(def.a,Sys_GetOption("video.dust_amount",100));
            // Wheel-thrown dust/chip specks (0x10..0x17): rotate and stretch
            // the quad along its screen-space motion so the specks read as
            // soft wedges thrown off the wheels, not separate round blobs
            // (video.dust_speck_streak, default 1). One step of motion is
            // sourceVector - vector0x10 (previous position).
            if (pCamera != NULL && g_particleTypeCount > 0x17 &&
                pType >= &g_particleTypes[0x10] && pType <= &g_particleTypes[0x17] &&
                Sys_GetOption("video.dust_speck_streak", 1) != 0) {
                const FixMatrix &sm = pCamera->world;
                float mdx = (DRAW_PARTICLE.sourceVector.x - DRAW_PARTICLE.vector0x10.x) / 65536.f;
                float mdy = (DRAW_PARTICLE.sourceVector.y - DRAW_PARTICLE.vector0x10.y) / 65536.f;
                float mdz = (DRAW_PARTICLE.sourceVector.z - DRAW_PARTICLE.vector0x10.z) / 65536.f;
                float sRight = (mdx * sm.right.x + mdy * sm.right.y + mdz * sm.right.z) / 65536.f;
                float sUp = (mdx * sm.up.x + mdy * sm.up.y + mdz * sm.up.z) / 65536.f;
                // depth of the speck itself, so wedges behind the camera keep the round quad
                float pdx = (def.pos.x - sm.position.x) / 65536.f;
                float pdy = (def.pos.y - sm.position.y) / 65536.f;
                float pdz = (def.pos.z - sm.position.z) / 65536.f;
                float speckDepth = (pdx * sm.forward.x + pdy * sm.forward.y + pdz * sm.forward.z) / 65536.f;
                int speckAngle; float speckLen, speckAlpha;
                if (WheelSpeck_StreakParams(sRight, sUp, speckDepth, 100, &speckAngle, &speckLen, &speckAlpha)) {
                    def.field_0x20 = (short)speckAngle;
                    def.left = (int)(def.left * speckLen);
                    def.right = (int)(def.right * speckLen);
                    def.a = (BYTE)(def.a * speckAlpha);
                }
            }
            def.shade = DRAW_PARTICLE.field0x54;
            if (g_particleTypeCount > SPEED_MOTE_TYPE && pType == &g_particleTypes[SPEED_MOTE_TYPE])
                SpeedMote_ApplyStreak(&def, pCamera);
            const int queuedBefore = g_unk0x006dd784;
            Billboard_Add(&def, texture);
            if (g_particleTypeCount > SPEED_MOTE_TYPE && pType == &g_particleTypes[SPEED_MOTE_TYPE]) {
                if (g_unk0x006dd784 > queuedBefore) ++g_speedMotesDrawn; else ++g_speedMotesRefused;  // refused = queue disabled or full
            }
            if (g_particleTypeCount > AIR_MOTE_TYPE && pType == &g_particleTypes[AIR_MOTE_TYPE]) {
                if (g_unk0x006dd784 > queuedBefore) ++g_airMotesDrawn; else ++g_airMotesRefused;
            }
        }
    }
    // Camera-facing cloud sheets are fallback only. Original chips/spray are
    // independent; accepted volume commands snapshot this camera once.
    // Volume command was queued once before sheet fallback selection.

}

#undef DRAW_PARTICLE

// Allocates and initialises a particle, optionally orienting its random spread
// around the supplied position vector.
// FUNCTION: CMR2 0x004b07a0
void Particle_Spawn(int typeIndex, FixVector *pSource, FixVector *pPosition,
                    int field0x48, FixMatrix *attachmentMatrix, BYTE *pColour,
                    BYTE field0x54, void *callbackParam, BYTE field0x55)
{
    Particle_SpawnTagged(typeIndex,pSource,pPosition,field0x48,attachmentMatrix,pColour,
                         field0x54,callbackParam,field0x55,false);
}

void Particle_SpawnTagged(int typeIndex, FixVector *pSource, FixVector *pPosition,
                    int field0x48, FixMatrix *attachmentMatrix, BYTE *pColour,
                    BYTE field0x54, void *callbackParam, BYTE field0x55, bool dryWheelDust)
{
    ParticleType *pType = NULL;
    bool hasBasis;
    FixVector right;
    FixVector direction;
    FixVector up;
    int length;
    int dot;
    int i;
    int best;
    int selected;
    Particle *pParticle;
    int randomX;
    int randomY;
    int randomZ;
    BYTE directionMask = 1;
    // Extra cosmetic clouds must not consume the gameplay/global rand stream.
    static unsigned cloudSeed = 0x5a17d93u;
    auto random = [&]() -> int {
        if (typeIndex != DUST_PLUME_TYPE && typeIndex != SPEED_MOTE_TYPE && typeIndex != AIR_MOTE_TYPE) return rand();
        cloudSeed = cloudSeed * 1664525u + 1013904223u;
        return (int)((cloudSeed >> 1) % ((unsigned)RAND_MAX + 1u));
    };

    if (typeIndex >= 0 && typeIndex < g_particleTypeCount)
        pType = &g_particleTypes[typeIndex];
    if (pType == NULL || g_particles == NULL || g_particleCount <= 0 ||
        (pType->flags & directionMask) == 0)
        return;
    hasBasis = false;

    if ((pType->directionFlags & directionMask) != 0) {
        int x = pPosition->x;
        if (x < 0)
            x = -x;
        if (x > 0xa0) {
            int z = pPosition->z;
            if (z < 0)
                z = -z;
            if (z > 0xa0) {
                length = FixVecLength(pPosition);
                if (length == 0) {
                    direction.x = 0;
                    direction.y = 0;
                    direction.z = 0;
                } else {
                    FixVecScaleRecip(&direction, pPosition, length);
                }

                if (direction.y > 0x8000) {
                    up.x = direction.x;
                    up.y = 0;
                    up.z = direction.z;
                    length = FixVecLength(&up);
                    if (length == 0) {
                        up.x = 0;
                        up.y = 0;
                        up.z = 0;
                    } else {
                        FixVecScaleRecip(&up, &up, length);
                    }
                } else {
                    up.x = 0;
                    up.y = 0x10000;
                    up.z = 0;
                }

                dot = FixVecDot(&direction, &up);
                up.x -= FixMul(direction.x, dot);
                up.y -= FixMul(direction.y, dot);
                up.z -= FixMul(direction.z, dot);
                length = FixVecLength(&up);
                if (length == 0) {
                    up.x = 0;
                    up.y = 0;
                    up.z = 0;
                } else {
                    FixVecScaleRecip(&up, &up, length);
                }

                FixVecCross(&right, &direction, &up);
                length = FixVecLength(&right);
                if (length == 0) {
                    right.x = 0;
                    right.y = 0;
                    right.z = 0;
                } else {
                    FixVecScaleRecip(&right, &right, length);
                }
                hasBasis = true;
            }
        }
    }

    best = 0;
    selected = g_nextParticle;
    pParticle = &g_particles[g_nextParticle];
    for (i = 0; i < g_particleCount; ) {
        if (g_nextParticle >= g_particleCount) {
            g_nextParticle = 0;
            pParticle = g_particles;
        }
        if (pParticle->active == 0) {
            if (pParticle != NULL)
                goto particleFound;
            break;
        }
        // Optional speed/air motes yield first: when the pool is full, a spawn of any other type takes a mote slot
        // before it would evict an original particle. Never true with the mote effects off (no motes exist).
        if ((typeIndex != SPEED_MOTE_TYPE && g_particleTypeCount > SPEED_MOTE_TYPE &&
            pParticle->pType == &g_particleTypes[SPEED_MOTE_TYPE]) ||
            (typeIndex != AIR_MOTE_TYPE && g_particleTypeCount > AIR_MOTE_TYPE &&
            pParticle->pType == &g_particleTypes[AIR_MOTE_TYPE])) {
            selected = g_nextParticle;
            best = 0x7fffffff;
        } else if (best != 0x7fffffff && pParticle->pType->lifetime - pParticle->age > best) {
            selected = g_nextParticle;
            best = pParticle->pType->lifetime - pParticle->age;
        }
        i++;
        pParticle++;
        g_nextParticle++;
    }
    pParticle = &g_particles[selected];

particleFound:
    pParticle->dryWheelDust = dryWheelDust;

    randomX = -0x8000 - (int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, random()), g_oneOverRandMax), g_minus65536));
    randomY = -0x8000 - (int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, random()), g_oneOverRandMax), g_minus65536));
    randomZ = -0x8000 - (int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, random()), g_oneOverRandMax), g_minus65536));

    pParticle->position.x = pPosition->x;
    pParticle->position.y = pPosition->y;
    pParticle->position.z = pPosition->z;
    pParticle->sourceVector.x = pSource->x;
    pParticle->sourceVector.y = pSource->y;
    pParticle->sourceVector.z = pSource->z;
    pParticle->vector0x10 = pParticle->sourceVector;
    pParticle->vector0x1c = pParticle->sourceVector;
    pParticle->vector0x28 = pParticle->sourceVector;
    pParticle->field0x48 = field0x48;
    pParticle->attachmentMatrix = attachmentMatrix;

    if ((pType->directionFlags & 1) != 0) {
        if (hasBasis) {
            if (pType->spread.x != 0) {
                FixVecScale(&right, &right, FixMul(randomX, pType->spread.x));
                pParticle->position.x += right.x;
                pParticle->position.y += right.y;
                pParticle->position.z += right.z;
            }
            if (pType->spread.y != 0) {
                FixVecScale(&up, &up, FixMul(randomY, pType->spread.y));
                pParticle->position.x += up.x;
                pParticle->position.y += up.y;
                pParticle->position.z += up.z;
            }
            if (pType->spread.z != 0) {
                FixVecScale(&direction, &direction, FixMul(randomZ, pType->spread.z));
                pParticle->position.x += direction.x;
                pParticle->position.y += direction.y;
                pParticle->position.z += direction.z;
            }
        } else {
            pParticle->position.x += FixMul(randomX, pType->spread.x);
            pParticle->position.y += FixMul(randomZ, pType->spread.z);
            pParticle->position.z += FixMul(randomZ, pType->spread.y);
        }
    } else {
        pParticle->position.x += FixMul(randomX, pType->spread.x);
        pParticle->position.y += FixMul(randomY, pType->spread.y);
        pParticle->position.z += FixMul(randomZ, pType->spread.z);
    }

    pParticle->pType = pType;
    pParticle->age = pType->lifetime;
    {
        BYTE type = pType->type;
        pParticle->type0x56 = type;
        pParticle->type0x53 = type;
        pParticle->type0x52 = type;
    }
    if ((pType->flags & 0x10) != 0) {
        int sizeVariation = pType->sizeVariation;
        int variation = (int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, random()), g_oneOverRandMax), CGraphics::m_65536));
        pParticle->size = pType->size + FixMul(sizeVariation, variation);
    } else {
        pParticle->size = pType->size;
    }
    {
        int zero = 0;
        if (pColour != NULL) {
            pParticle->colour[0] = pColour[0];
            pParticle->colour[1] = pColour[1];
            pParticle->colour[2] = pColour[2];
        } else {
            pParticle->colour[0] = pType->colour[0];
            pParticle->colour[1] = pType->colour[1];
            pParticle->colour[2] = pType->colour[2];
        }
        if ((pType->directionFlags & 2) != 0)
            pParticle->field0x54 = field0x54;
        pParticle->texture = pType->texture;
        pParticle->field0x55 = field0x55;
        pParticle->active = 1;
        pParticle->field0x58 = (BYTE)zero;
        pParticle->field0x50 = (short)zero;
        if (pType->callback != NULL)
            pType->callback(pParticle, pType, callbackParam);
    }
}

// GLOBAL: CMR2 0x006dfe10
int *g_triangleVertexHeights;

// Interpolates the height of a point on a triangle whose vertex heights are stored separately.
// FUNCTION: CMR2 0x004b6340
int Graphics_GetTriangleHeight(unsigned short *pHeightIndices, FixVector *pVertices, FixVector *pPosition)
{
    if (g_triangleVertexHeights != 0) {
        FixVector vertices[3];
        FixVector edgeB;  // vertices[1] - vertices[0]
        FixVector edgeA;  // vertices[2] - vertices[0]
        FixVector normal;

        vertices[0] = pVertices[0];
        vertices[1] = pVertices[1];
        vertices[2] = pVertices[2];
        vertices[0].y = g_triangleVertexHeights[pHeightIndices[0]];
        vertices[1].y = g_triangleVertexHeights[pHeightIndices[1]];
        vertices[2].y = g_triangleVertexHeights[pHeightIndices[2]];

        edgeB.x = vertices[1].x - vertices[0].x;
        edgeB.y = vertices[1].y - vertices[0].y;
        edgeB.z = vertices[1].z - vertices[0].z;
        edgeA.x = vertices[2].x - vertices[0].x;
        edgeA.y = vertices[2].y - vertices[0].y;
        edgeA.z = vertices[2].z - vertices[0].z;

        int length = FixVecLength(&edgeB);
        if (length == 0) {
            edgeB.x = 0;
            edgeB.y = 0;
            edgeB.z = 0;
        } else {
            FixVecScaleRecip(&edgeB, &edgeB, length);
        }

        length = FixVecLength(&edgeA);
        if (length == 0) {
            edgeA.x = 0;
            edgeA.y = 0;
            edgeA.z = 0;
        } else {
            FixVecScaleRecip(&edgeA, &edgeA, length);
        }

        FixVecCross(&normal, &edgeB, &edgeA);
        length = FixVecLength(&normal);
        if (length == 0) {
            normal.x = 0;
            normal.y = 0;
            normal.z = 0;
        } else {
            FixVecScaleRecip(&normal, &normal, length);
        }

        int planeDistance = FixVecDot(&vertices[0], &normal);
        if (normal.y != 0) {
            return FixDiv(planeDistance - FixMul(pPosition->x, normal.x) -
                              FixMul(pPosition->z, normal.z),
                          normal.y);
        }
    }

    return -65470464;
}

int CGraphics::m_unk0x00520b1c = 1;
int CGraphics::m_unk0x00520b20 = 1;

// GLOBAL: CMR2 0x00520b18
int g_unk0x00520b18 = -1;

// FUNCTION: CMR2 0x004a3dc0
void Graphics_SetTextureStageState(int param1)
{
    CGraphics::m_unk0x00520b14 = param1;
}

// FUNCTION: CMR2 0x004a3dd0
void Graphics_InvalidateTextureStageCache(void)
{
    g_unk0x00520b18 = -1;
}

// FUNCTION: CMR2 0x004a3de0
void CGraphics::InvalidateBlendStateCache(void)
{
    m_unk0x00520b1c = 1;
    m_unk0x00520b20 = 1;
}
int CGraphics::m_unk0x00520b28 = -1;
unsigned int CGraphics::m_unk0x0065fa24;
Texture *CGraphics::m_unk0x0065fa38;

// Sets render states 0x13/0x14 when the pair changes.
// FUNCTION: CMR2 0x004a3e40
void CGraphics::SetCachedSourceDestinationBlend(int param1, int param2)
{
    if (param1 == m_unk0x00520b1c && param2 == m_unk0x00520b20)
        return;
    Gfx_SetSrcBlend(param1);
    Gfx_SetDestBlend(param2);
    m_unk0x00520b1c = param1;
    m_unk0x00520b20 = param2;
}

void Frontend_SetObjectField118(struct Texture *pTexture, int value);

// Creates a texture from file data already in memory (DDS or TGA).
// FUNCTION: CMR2 0x004a48c0
Texture *CGraphics::CreateTextureFromImageData(char *name, void *pData, unsigned int flags)
{
    Texture *pTexture;
    BOOL isDDS;
    int i;

    pTexture = NULL;
    isDDS = FALSE;
    // The original passes the count as memset's fill value and 0 as the count:
    // the call is a no-op (kept so the code matches the original).
    if (CGraphics::m_textureCount == 0)
        memset(m_pTextureManager->textureBuffer, 0x98000, 0);
    if (*(DWORD *)pData == 0x20534444)
        isDDS = TRUE;
    for (i = 0; i < 0x800; i++) {
        if (m_pTextureManager->textureBuffer[i] == NULL) {
            m_pTextureManager->textureBuffer[i] = (Texture *)CFileBuffer::AllocateLockedBuffer(sizeof(Texture));
            pTexture = m_pTextureManager->textureBuffer[i];
            pTexture->textureId = i;
            Frontend_SetObjectField118(pTexture, 0);
            if (pTexture == NULL)
                return NULL;
            m_textureCount++;
            break;
        }
    }
    if (isDDS)
        strncpy(name + strlen(name) - 4, m_ddsExtension, 4);
    sprintf(pTexture->name, "%s", name);
    pTexture->flags |= flags | 0x1000;
    if (isDDS)
        return LoadDDSTexture((DDSFile *)pData, pTexture);
    return LoadTGATexture((BYTE *)pData, pTexture);
}

// Loads a texture by file name, trying the .DDS file first and the .TGA
// file next.
// match 46%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004a49c0
Texture *CGraphics::LoadDDSThenTGATexture(char *name, unsigned int flags)
{
    Texture *pTexture;
    void *pData;
    BOOL isDDS;
    char *pExtension;
    int i;

    pTexture = NULL;
    isDDS = FALSE;
    // The original passes the pool byte size as memset's fill value and 0 as
    // the count: the call is a no-op (kept so the code matches the original).
    if (CGraphics::m_textureCount == 0)
        memset(m_pTextureManager->textureBuffer, 0x98000, 0);
    pExtension = name + strlen(name) - 4;
    strncpy(pExtension, m_ddsExtension, 4);
    pData = CFileBuffer::GetGenericFileBuffer(name, FALSE);
    if (pData != NULL) {
        isDDS = TRUE;
    } else {
        strncpy(pExtension, m_tgaExtension, 4);
        pData = CFileBuffer::GetGenericFileBuffer(name, FALSE);
    }
    for (i = 0; i < 0x800; i++) {
        if (m_pTextureManager->textureBuffer[i] == NULL) {
            m_pTextureManager->textureBuffer[i] = (Texture *)CFileBuffer::AllocateLockedBuffer(sizeof(Texture));
            pTexture = m_pTextureManager->textureBuffer[i];
            pTexture->textureId = i;
            pTexture->pArchive = NULL;
            Frontend_SetObjectField118(pTexture, 0);
            if (pTexture == NULL)
                return NULL;
            m_textureCount++;
            break;
        }
    }
    sprintf(pTexture->name, "%s", name);
    SceneryPack_RegisterSource(pTexture,pData,CGenericFileLoader::GetGenericFileSize());
    pTexture->flags |= flags;
    if ((pTexture->flags & 0x20) && (g_pGraphics->field913_0x3bc & 0x10))
        return LoadTGABumpMap((BYTE *)pData, pTexture);
    Texture *loaded=isDDS ? LoadDDSTexture((DDSFile *)pData,pTexture) : LoadTGATexture((BYTE *)pData,pTexture);
    France1Textures_Apply(loaded,pData,CGenericFileLoader::GetGenericFileSize());
    return loaded;
}

// Converts a TGA image into a texture: the pixels (read through
// FUN_004a60d0) are packed into a system memory surface of the texture
// format, which is then copied (or turned into a bump map) into the texture.
// match 44%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004a6710
Texture *CGraphics::LoadTGATexture(BYTE *pTGA, Texture *pTexture)
{
    TGAImageInfo *pInfo;
    GfxLockedRect lockDesc;
    Texture tmpTexture;
    TextureFormat *pFormat;
    unsigned int width;
    unsigned int height;
    unsigned int x;
    unsigned int y;
    unsigned int rMask;
    unsigned int gMask;
    unsigned int bMask;
    unsigned int aMask;
    unsigned int mask;
    short rShift;
    short gShift;
    unsigned short bShift;
    unsigned short aShift;
    short rDepth;
    short gDepth;
    short bDepth;
    short aDepth;
    unsigned short bitCount;
    int padding;
    int i;
    BYTE *p;
    unsigned int r;
    unsigned int g;
    unsigned int b;
    unsigned int a;
    WORD *pDst16;
    DWORD *pDst32;

    pInfo = ParseTGAHeader(pTGA);
    if (pInfo == NULL)
        return NULL;
    if (pInfo->bytesPerPixel == 4)
        pTexture->flags |= 1;
    height = pInfo->height;
    width = pInfo->width;
    // PORT: a CPU-side texture of the texture format, copied by the renderer.
    if ((pTexture->flags & 1) && m_pTextureManager->textureInfo2 != NULL)
        tmpTexture.pSurface = Gfx_CreateTexture(width, height, m_pTextureManager->textureInfo2->gfxFormat, 1, GFX_TEXTURE_STAGING);
    else
        tmpTexture.pSurface = Gfx_CreateTexture(width, height, m_pTextureManager->textureInfo1->gfxFormat, 1, GFX_TEXTURE_STAGING);
    memset(&lockDesc, 0, sizeof(lockDesc));
    Gfx_LockTexture(tmpTexture.pSurface, 0, 0, &lockDesc);

    rMask = lockDesc.format.redMask;
    gMask = lockDesc.format.greenMask;
    bMask = lockDesc.format.blueMask;
    aMask = lockDesc.format.alphaMask;
    for (i = 0, mask = rMask; i < 32 && !(mask & 1); i++)
        mask >>= 1;
    rShift = (BYTE)i;
    for (i = 0, mask = gMask; i < 32 && !(mask & 1); i++)
        mask >>= 1;
    gShift = (BYTE)i;
    for (i = 0, mask = bMask; i < 32 && !(mask & 1); i++)
        mask >>= 1;
    bShift = (BYTE)i;
    for (i = 0, mask = aMask; i < 32 && !(mask & 1); i++)
        mask >>= 1;
    aShift = (BYTE)i;
    for (i = 0, mask = rMask; i < 32 && mask != 0; i++)
        mask >>= 1;
    rDepth = (BYTE)i - 8;
    for (i = 0, mask = gMask; i < 32 && mask != 0; i++)
        mask >>= 1;
    gDepth = (BYTE)i - 8;
    for (i = 0, mask = bMask; i < 32 && mask != 0; i++)
        mask >>= 1;
    bDepth = (BYTE)i - 8;
    for (i = 0, mask = aMask; i < 32 && mask != 0; i++)
        mask >>= 1;
    aDepth = (BYTE)i - 8;

    bitCount = (unsigned short)lockDesc.format.bitsPerPixel;
    padding = lockDesc.pitch - (bitCount * width >> 3);
    if (bitCount == 16) {
        pDst16 = (WORD *)lockDesc.pixels;
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                p = SampleTGAPixel(x, y, pInfo, pTexture->flags);
                r = p[0];
                g = p[1];
                b = p[2];
                if (aShift < pInfo->bytesPerPixel * 8)
                    a = p[3];
                else
                    a = 0xff;
                if (aDepth >= 0)
                    a = ((aMask >> ((aDepth) < 0 ? -(aDepth) : (aDepth))) & a) << abs(aDepth);
                else
                    a = ((aMask << abs(aDepth)) & a) >> abs(aDepth);
                if (rDepth >= 0)
                    r = ((rMask >> abs(rDepth)) & r) << abs(rDepth);
                else
                    r = ((rMask << abs(rDepth)) & r) >> abs(rDepth);
                if (gDepth >= 0)
                    g = ((gMask >> abs(gDepth)) & g) << abs(gDepth);
                else
                    g = ((gMask << abs(gDepth)) & g) >> abs(gDepth);
                if (bDepth >= 0)
                    b = ((bMask >> abs(bDepth)) & b) << abs(bDepth);
                else
                    b = ((bMask << abs(bDepth)) & b) >> abs(bDepth);
                *pDst16++ = (WORD)(r | g | b | a);
            }
            if (y != height - 1)
                pDst16 += (unsigned short)padding;
        }
    } else if (bitCount == 32) {
        pDst32 = (DWORD *)lockDesc.pixels;
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                p = SampleTGAPixel(x, y, pInfo, pTexture->flags);
                a = 0;
                if (aShift < pInfo->bytesPerPixel * 8)
                    a = p[3];
                *pDst32++ = (p[0] << rShift) | (p[1] << gShift) | (p[2] << bShift) | (a << aShift);
            }
            if (y != height - 1)
                pDst32 += padding;
        }
    } else {
        Gfx_UnlockTexture(tmpTexture.pSurface, 0, 0);
        Gfx_DestroyTexture(tmpTexture.pSurface);
        if (pTexture->pSurface != NULL) {
            Gfx_DestroyTexture(pTexture->pSurface);
            pTexture->pSurface = NULL;
        }
        return NULL;
    }

    if ((g_pGraphics->field913_0x3bc & 3) && (pTexture->flags & 0x10))
        m_unk0x0065fa2c += GetMipMapDataSize(pTexture);
    if (bitCount == 16)
        m_unk0x0065fa2c += height * width * 2;
    else
        m_unk0x0065fa2c += height * width * 4;
    Gfx_UnlockTexture(tmpTexture.pSurface, 0, 0);
    if (g_pGraphics->field913_0x3bc & 0x10) {
        if (strncmp(pTexture->name + strlen(pTexture->name) - 6, m_strSuffixBU, 2) == 0)
            pTexture->flags = (pTexture->flags & ~1) | 0x20;
    }
    pTexture->flags &= ~0x6000;
    if ((pTexture->flags & 0x20) && (g_pGraphics->field913_0x3bc & 0x10)) {
        CreateTextureSurface(pTexture, width, height, pTexture->flags);
        GenerateBumpMap(&tmpTexture, pTexture);
    } else {
        CreateTextureSurface(pTexture, width, height, pTexture->flags);
        Gfx_CopyTexture(pTexture->pSurface, tmpTexture.pSurface);
    }
    Gfx_DestroyTexture(tmpTexture.pSurface);
    tmpTexture.pSurface = NULL;
    if ((g_pGraphics->field913_0x3bc & 3) && (pTexture->flags & 0x10))
        GetMipMapSurfaces(pTexture);
    if ((g_pGraphics->field913_0x3bc & 3) && (pTexture->flags & 0x10))
        BltMipMaps(pTexture);
    if (!(pTexture->flags & 0x1000))
        CFileBuffer::FreeGenericFileBuffer(pTGA);
    VectorSprite_Replace(pTexture->name, pTexture->pSurface);
    return pTexture;
}

// 1/255 and the double 1.0 the original keeps in its constant block.
// GLOBAL: CMR2 0x00511364
extern const float g_floatOneOver255 = 1.0f / 255.0f;
// GLOBAL: CMR2 0x00511428
extern const double g_doubleOne = 1.0;
extern const float g_netZero;
extern const float g_netByteScale;
extern const float g_netOne;

// The D3DX colour helper the original links statically (ours is the import).
// LIBRARY: CMR2 0x004c674c
// _D3DXColorAdjustContrast@12

// Reads pixel (x, y) of a bottom-up TGA image as R, G, B, A in m_tgaPixel,
// applying the brightness and contrast of the car (flag 0x80) or track
// (flag 0x100) textures.
// FUNCTION: CMR2 0x004a60d0
BYTE *CGraphics::SampleTGAPixel(unsigned int x, unsigned int y, TGAImageInfo *pInfo, unsigned int flags)
{
    BYTE *p;
    float contrast;
    int brightness;
    int r;
    int g;
    int b;
    GfxColorValue in;
    GfxColorValue out;

    if (x >= pInfo->width || y >= pInfo->height) {
        m_tgaPixel[3] = 0;
        m_tgaPixel[2] = 0;
        m_tgaPixel[1] = 0;
        m_tgaPixel[0] = 0;
        return m_tgaPixel;
    }
    p = ((pInfo->height - y - 1) * pInfo->width + x) * pInfo->bytesPerPixel + pInfo->pixels;
    m_tgaPixel[2] = p[0];
    p++;
    m_tgaPixel[1] = p[0];
    p++;
    m_tgaPixel[0] = p[0];
    if (pInfo->bytesPerPixel == 4)
        m_tgaPixel[3] = p[1];
    if (flags & 0x80) {
        brightness = m_unk0x0065fa44;
        contrast = m_unk0x00520b34;
    } else if (flags & 0x100) {
        brightness = m_unk0x0065fa48;
        contrast = m_unk0x00520b38;
    }
    if (!(flags & 0x80) && !(flags & 0x100))
        return m_tgaPixel;
    if (brightness != 0) {
        r = m_tgaPixel[0] + brightness;
        g = m_tgaPixel[1] + brightness;
        b = m_tgaPixel[2] + brightness;
        if (r < 0)
            r = 0;
        else if (r > 0xff)
            r = 0xff;
        if (g < 0)
            g = 0;
        else if (g > 0xff)
            g = 0xff;
        if (b < 0)
            b = 0;
        else if (b > 0xff)
            b = 0xff;
        m_tgaPixel[0] = r;
        m_tgaPixel[1] = g;
        m_tgaPixel[2] = b;
    }
    if (contrast != g_doubleOne) {
        in.r = X87S_MUL(X87S_LOAD(m_tgaPixel[0]), g_floatOneOver255);
        in.g = X87S_MUL(X87S_LOAD(m_tgaPixel[1]), g_floatOneOver255);
        in.b = X87S_MUL(X87S_LOAD(m_tgaPixel[2]), g_floatOneOver255);
        // PORT: D3DXColorAdjustContrast: each channel scaled about 0.5.
        out.r = 0.5f + contrast * (in.r - 0.5f);
        out.g = 0.5f + contrast * (in.g - 0.5f);
        out.b = 0.5f + contrast * (in.b - 0.5f);
        out.a = in.a;
        if (out.r > g_netOne)
            out.r = 1.0f;
        if (out.g > g_netOne)
            out.g = 1.0f;
        if (out.b > g_netOne)
            out.b = 1.0f;
        if (out.r < g_netZero)
            out.r = 0.0f;
        if (out.g < g_netZero)
            out.g = 0.0f;
        if (out.b < g_netZero)
            out.b = 0.0f;
        m_tgaPixel[0] = (BYTE)FTOI(int, out.r * 255.0f);
        m_tgaPixel[1] = (BYTE)FTOI(int, out.g * 255.0f);
        m_tgaPixel[2] = (BYTE)FTOI(int, out.b * 255.0f);
    }
    return m_tgaPixel;
}

// Builds a bump map texture from a height map TGA: the height differences
// to the right (dU) and lower (dV) neighbours plus the alpha channel as
// luminance, packed into the 16 or 24 bit bump map format.
// match 48%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004a6370
Texture *CGraphics::LoadTGABumpMap(BYTE *pTGA, Texture *pTexture)
{
    TGAImageInfo *pInfo;
    GfxLockedRect desc;
    unsigned int mask;
    int count;
    int uBits;
    int vBits;
    int lBits;
    int uShift;
    int vShift;
    int lShift;
    int i;
    unsigned int x;
    unsigned int y;
    unsigned int height;
    unsigned int right;
    unsigned int down;
    int du;
    int dv;
    char l;
    WORD *pDst16;
    BYTE *pDst24;

    if (m_pTextureManager->textureInfo5 == NULL)
        return NULL;
    pInfo = ParseTGAHeader(pTGA);
    if (pInfo == NULL)
        return NULL;
    if (pInfo->bytesPerPixel == 3)
        return NULL;
    CreateTextureSurface(pTexture, pInfo->width, pInfo->height, pTexture->flags);
    memset(&desc, 0, sizeof(desc));
    Gfx_LockTexture(pTexture->pSurface, 0, 0, &desc);
    if (desc.format.bitsPerPixel != 16 && desc.format.bitsPerPixel != 24) {
        Gfx_UnlockTexture(pTexture->pSurface, 0, 0);
        return NULL;
    }
    count = 0;
    for (mask = desc.format.redMask, i = 32; i != 0; i--, mask >>= 1)
        if (mask & 1)
            count++;
    uBits = 8 - (BYTE)count;
    count = 0;
    for (mask = desc.format.greenMask, i = 32; i != 0; i--, mask >>= 1)
        if (mask & 1)
            count++;
    vBits = 8 - (BYTE)count;
    count = 0;
    for (mask = desc.format.blueMask, i = 32; i != 0; i--, mask >>= 1)
        if (mask & 1)
            count++;
    lBits = 8 - (BYTE)count;
    for (uShift = 0, mask = desc.format.redMask; uShift < 32 && !(mask & 1); uShift++)
        mask >>= 1;
    for (vShift = 0, mask = desc.format.greenMask; vShift < 32 && !(mask & 1); vShift++)
        mask >>= 1;
    for (lShift = 0, mask = desc.format.blueMask; lShift < 32 && !(mask & 1); lShift++)
        mask >>= 1;

    pDst16 = (WORD *)desc.pixels;
    pDst24 = (BYTE *)desc.pixels;
    for (y = 0; y < pInfo->height; y++) {
        for (x = 0; x < pInfo->width; x++) {
            height = *SampleTGAPixel(x, y, pInfo, 0);
            if (x < pInfo->width - 1)
                right = *SampleTGAPixel(x + 1, y, pInfo, 0);
            else
                right = height + 0x80;
            if (y < pInfo->height - 1)
                down = *SampleTGAPixel(x, y + 1, pInfo, 0);
            else
                down = height + 0x80;
            du = abs((int)(height - right));
            dv = abs((int)(height - down));
            l = SampleTGAPixel(x, y, pInfo, 0)[3];
            if (desc.format.bitsPerPixel == 16) {
                *pDst16++ = (WORD)(((l >> lBits) << lShift) | ((dv >> vBits) << vShift) | ((du >> uBits) << uShift));
            } else {
                pDst24[0] = (BYTE)du;
                pDst24[1] = (BYTE)dv;
                pDst24[2] = l;
                pDst24 += 3;
            }
        }
    }
    Gfx_UnlockTexture(pTexture->pSurface, 0, 0);
    CFileBuffer::FreeGenericFileBuffer(pTGA);
    return pTexture;
}

// Sets up the texture stage for the texture's blend mode (stage alone with
// no texture): colour/alpha operations, filters, blend render states and
// texture coordinate set.
// FUNCTION: CMR2 0x004a3e90
void CGraphics::ConfigureTextureStageBlendMode(int stage, Texture *pTexture)
{
    float value;

    if (pTexture == NULL) {
        g_unk0x00520b18 = -1;
        Gfx_SetStageColorArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageColorArg2(stage, GFX_TA_DIFFUSE);
        Gfx_SetStageColorOp(stage, GFX_TOP_DISABLE);
        Gfx_SetStageAlphaArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageAlphaArg2(stage, GFX_TA_DIFFUSE);
        Gfx_SetStageAlphaOp(stage, GFX_TOP_DISABLE);
        return;
    }
    switch (pTexture->blendMode) {
    case 0:
        Gfx_SetStageColorArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageColorArg2(stage, GFX_TA_DIFFUSE);
        Gfx_SetStageColorOp(stage, GFX_TOP_MODULATE);
        Gfx_SetStageAlphaArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageAlphaArg2(stage, GFX_TA_DIFFUSE);
        Gfx_SetStageAlphaOp(stage, GFX_TOP_MODULATE);
        if (g_pGraphics->field913_0x3bc & 1)
            Gfx_SetStageMipFilter(stage, GFX_MIPFILTER_POINT);
        else if (g_pGraphics->field913_0x3bc & 2)
            Gfx_SetStageMipFilter(stage, GFX_MIPFILTER_LINEAR);
        else
            Gfx_SetStageMipFilter(stage, GFX_MIPFILTER_NONE);
        Gfx_SetStageMaxMipLevel(stage, 0);
        SetCachedSourceDestinationBlend(5, 6);
        SetTexCoordIndex(stage, 0);
        break;
    case 1:
        Gfx_SetStageColorArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageColorArg2(stage, GFX_TA_DIFFUSE);
        Gfx_SetStageColorOp(stage, GFX_TOP_MODULATE);
        Gfx_SetStageAlphaArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageAlphaArg2(stage, GFX_TA_DIFFUSE);
        Gfx_SetStageAlphaOp(stage, GFX_TOP_DISABLE);
        SetCachedSourceDestinationBlend(2, 2);
        SetTexCoordIndex(stage, 0);
        break;
    case 2:
        Gfx_SetStageColorArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageColorArg2(stage, GFX_TA_DIFFUSE);
        Gfx_SetStageColorOp(stage, GFX_TOP_MODULATE);
        Gfx_SetStageAlphaArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageAlphaArg2(stage, GFX_TA_DIFFUSE);
        Gfx_SetStageAlphaOp(stage, GFX_TOP_MODULATE);
        SetCachedSourceDestinationBlend(5, 2);
        SetTexCoordIndex(stage, 0);
        break;
    case 3:
        Gfx_SetStageColorArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageColorArg2(stage, GFX_TA_DIFFUSE);
        Gfx_SetStageColorOp(stage, GFX_TOP_MODULATE);
        Gfx_SetStageAlphaArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageAlphaArg2(stage, GFX_TA_DIFFUSE);
        Gfx_SetStageAlphaOp(stage, GFX_TOP_MODULATE);
        SetCachedSourceDestinationBlend(1, 4);
        SetTexCoordIndex(stage, 0);
        break;
    case 4:
        Gfx_SetStageColorArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageColorArg2(stage, GFX_TA_DIFFUSE);
        Gfx_SetStageColorOp(stage, GFX_TOP_MODULATE);
        Gfx_SetStageAlphaArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageAlphaArg2(stage, GFX_TA_DIFFUSE);
        Gfx_SetStageAlphaOp(stage, GFX_TOP_SELECTARG2);
        SetTexCoordIndex(stage, 1);
        break;
    case 5:
        Gfx_SetStageColorArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageColorArg2(stage, GFX_TA_CURRENT);
        Gfx_SetStageColorOp(stage, GFX_TOP_SELECTARG2);
        Gfx_SetStageAlphaArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageAlphaArg2(stage, GFX_TA_CURRENT);
        Gfx_SetStageAlphaOp(stage, GFX_TOP_SELECTARG1);
        SetCachedSourceDestinationBlend(5, 2);
        SetTexCoordIndex(stage, 0);
        break;
    case 6:
        Gfx_SetStageColorArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageColorArg2(stage, GFX_TA_CURRENT);
        Gfx_SetStageColorOp(stage, GFX_TOP_SELECTARG1);
        Gfx_SetStageAlphaArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageAlphaArg2(stage, GFX_TA_CURRENT);
        Gfx_SetStageAlphaOp(stage, GFX_TOP_DISABLE);
        SetTexCoordIndex(stage, 0);
        break;
    case 7:
        Gfx_SetStageColorArg1(stage, GFX_TA_TEXTURE | GFX_TA_COMPLEMENT);
        Gfx_SetStageColorArg2(stage, GFX_TA_CURRENT);
        Gfx_SetStageColorOp(stage, GFX_TOP_ADDSIGNED);
        Gfx_SetStageAlphaArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageAlphaArg2(stage, GFX_TA_CURRENT);
        Gfx_SetStageAlphaOp(stage, GFX_TOP_DISABLE);
        SetCachedSourceDestinationBlend(9, 3);
        SetTexCoordIndex(stage, 1);
        break;
    case 8:
        Gfx_SetStageColorOp(stage, GFX_TOP_BUMPENVMAPLUMINANCE);
        Gfx_SetStageColorArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageColorArg2(stage, GFX_TA_DIFFUSE);
        value = m_bumpScale;
        Gfx_SetStageBumpEnvMat(stage, 0, value);
        value = 0.0f;
        Gfx_SetStageBumpEnvMat(stage, 1, value);
        value = 0.0f;
        Gfx_SetStageBumpEnvMat(stage, 2, value);
        value = m_bumpScale;
        Gfx_SetStageBumpEnvMat(stage, 3, value);
        value = 1.0f;
        Gfx_SetStageBumpEnvLScale(stage, value);
        value = 0.0f;
        Gfx_SetStageBumpEnvLOffset(stage, value);
        Gfx_SetStageAlphaArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageAlphaArg2(stage, GFX_TA_CURRENT);
        Gfx_SetStageAlphaOp(stage, GFX_TOP_DISABLE);
        SetTexCoordIndex(stage, 0);
        break;
    case 9:
        Gfx_SetStageColorArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageColorArg2(stage, GFX_TA_CURRENT);
        Gfx_SetStageColorOp(stage, GFX_TOP_MODULATE);
        Gfx_SetStageAlphaArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageAlphaArg2(stage, GFX_TA_CURRENT);
        Gfx_SetStageAlphaOp(stage, GFX_TOP_DISABLE);
        Gfx_SetStageMagFilter(stage, GFX_FILTER_LINEAR);
        Gfx_SetStageMinFilter(stage, GFX_FILTER_LINEAR);
        SetCachedSourceDestinationBlend(2, 2);
        SetTexCoordIndex(stage, 1);
        break;
    case 10:
        Gfx_SetStageColorArg1(stage, GFX_TA_TEXTURE | GFX_TA_ALPHAREPLICATE);
        Gfx_SetStageColorArg2(stage, GFX_TA_DIFFUSE);
        Gfx_SetStageColorOp(stage, GFX_TOP_MODULATE);
        Gfx_SetStageAlphaArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageAlphaArg2(stage, GFX_TA_DIFFUSE);
        Gfx_SetStageAlphaOp(stage, GFX_TOP_MODULATE);
        SetCachedSourceDestinationBlend(5, 6);
        SetTexCoordIndex(stage, 0);
        break;
    case 11:
        Gfx_SetStageColorArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageColorArg2(stage, GFX_TA_DIFFUSE);
        Gfx_SetStageColorOp(stage, GFX_TOP_MODULATE);
        Gfx_SetStageAlphaArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageAlphaArg2(stage, GFX_TA_DIFFUSE);
        Gfx_SetStageAlphaOp(stage, GFX_TOP_SELECTARG2);
        Gfx_SetStageMagFilter(stage, GFX_FILTER_LINEAR);
        Gfx_SetStageMinFilter(stage, GFX_FILTER_LINEAR);
        Gfx_SetStageTexCoordIndex(stage, GFX_TCI_CAMERASPACEREFLECTIONVECTOR | 1);
        Gfx_SetStageTextureTransform(stage, GFX_TTFF_COUNT3);
        break;
    case 0xff:
        Gfx_SetStageColorArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageColorArg2(stage, GFX_TA_DIFFUSE);
        Gfx_SetStageColorOp(stage, GFX_TOP_MODULATE);
        Gfx_SetStageAlphaArg1(stage, GFX_TA_TEXTURE);
        Gfx_SetStageAlphaArg2(stage, GFX_TA_DIFFUSE);
        Gfx_SetStageAlphaOp(stage, GFX_TOP_MODULATE);
        SetCachedSourceDestinationBlend(5, 6);
        SetTexCoordIndex(stage, 0);
        break;
    }
    g_unk0x00520b18 = pTexture->blendMode;
}

// Applies a texture stage change and forwards it to the render states.
// FUNCTION: CMR2 0x004a4850
void CGraphics::ApplyTextureStageChange(int stage, Texture *pTexture)
{
    if (m_unk0x0065fa38 == pTexture && m_unk0x00520b28 == stage)
        return;
    m_unk0x0065fa38 = pTexture;
    m_unk0x00520b28 = stage;
    if (pTexture != NULL) {
        Gfx_SetTexture((DWORD)stage, pTexture->pSurface);
        ConfigureTextureStageBlendMode(stage, pTexture);
    } else {
        Gfx_SetTexture((DWORD)stage, NULL);
        ConfigureTextureStageBlendMode(stage, NULL);
    }
    m_unk0x0065fa24++;
}

// GLOBAL: CMR2 0x0065ad18
DWORD g_textureFactor;

// Sets the alpha of the texture factor render state from pColour[3].
// FUNCTION: CMR2 0x004a3df0
void Graphics_SetTextureFactorAlpha(BYTE *pColour)
{
    g_textureFactor = pColour[3] << 24;
    Gfx_SetTextureFactor(g_textureFactor);
}

// GLOBAL: CMR2 0x00520b4c
char g_strTextureNotFound[28] = "couldn't find texture =  %s";

// Reloads the image of a loaded texture from its archive, e.g. after its
// name was changed to another variant.
// FUNCTION: CMR2 0x004a5da0
void Graphics_ReloadTexture(Texture *pTexture)
{
    int i;
    Texture *p;
    DWORD *pData;

    for (i = 0; i < 2048; i++) {
        p = CGraphics::m_pTextureManager->textureBuffer[i];
        if (p == pTexture) {
            if (p->pSurface != NULL) {
                Gfx_DestroyTexture(p->pSurface);
                p->pSurface = NULL;
            }
            pData = (DWORD *)CGenericFileLoader::FindFile((GenericFile *)p->pArchive, p->name, NULL, NULL, 0);
            if (pData == NULL) {
                sprintf(CFrontend::m_stringDest, g_strTextureNotFound, p->name);
                return;
            }
            if (*pData == 0x20534444)
                CGraphics::LoadDDSTexture((DDSFile *)pData, p);
            else
                CGraphics::LoadTGATexture((BYTE *)pData, p);
        }
    }
}

// Looks for a free timer slot (shape 5) and returns its index.
// FUNCTION: CMR2 0x004bc410
BYTE Timer_FindFree(void)
{
    int slot;
    int i;

    slot = 0;
    for (i = 0; i < 32; i++) {
        if (g_timerPulseState.timers[slot].shape == 5)
            return (BYTE)slot;
        slot = (slot + 1) % 32;
    }
    return (BYTE)slot;
}

// Cube-map texture ids reserved for the car reflections (-1 when unused).
// GLOBAL: CMR2 0x006de95c
unsigned int g_carReflectionCubeMaps[20];

// Reserves count cube maps of the given size, then loads the environment
// texture; returns 0 when it is missing.
// FUNCTION: CMR2 0x004b23c0
int Graphics_ReserveCubeMapsAndLoadEnvironment(char *name, int count, GenericFile *pFile, DWORD size)
{
    unsigned int *pId;
    unsigned short *pTexture;

    // More detailed existing dynamic reflections, not new reflective materials.
    // video.reflection_resolution=0 restores the original stage-selected size.
    CGraphics::m_cubeMapSize = ReflectionCube_Size(
        Sys_GetOption("video.reflection_resolution", 128), (int)size);
    memset(g_carReflectionCubeMaps, 0xff, sizeof(g_carReflectionCubeMaps));
    if ((*(BYTE *)&g_pGraphics->field913_0x3bc & 0x80) != 0 && count > 0) {
        pId = g_carReflectionCubeMaps;
        do {
            *pId++ = *(unsigned short *)Graphics_AllocateCubeMapTextureSlot();
        } while (--count != 0);
    }
    pTexture = (unsigned short *)CTexture::FindLoadTexture(pFile, name, NULL, 0, 0, 0x8000);
    if (pTexture == NULL)
        return 0;
    {
        g_unk0x005210b8 = *pTexture;
        Frontend_SetObjectField118(CGraphics::m_pTextureManager->textureBuffer[g_unk0x005210b8], 4);
        return 1;
    }
}

GfxMatrix *FloatMatrix_Multiply(GfxMatrix *pOut, GfxMatrix *pA, GfxMatrix *pB);
extern const float g_netOne;

// Scale applied to the shadow vertex positions (0.5).
// GLOBAL: CMR2 0x00511424
extern const float g_floatHalf = 0.5f;

// Builds the view * world matrix, uses it to project every mesh vertex (taking
// the midpoint between the vertex and the next one, 0x30 bytes apart) into the
// x/y stored at offsets 0x28/0x2c of the vertex, and re-uploads the vertices
// to the mesh's slot of the shared vertex buffer.
// FUNCTION: CMR2 0x004b2460
void Graphics_ProjectMeshVertexMidpoints(Mesh *pMesh)
{
    GfxMatrix transform;
    GfxMatrix world;
    GfxMatrix view;
    float m11, m21, m31, m12, m22, m32;
    float *pVertexData = (float *)pMesh->pVertexData;
    void *pVertices;
    int i;
    int j;

    Gfx_GetTransform(GFX_TRANSFORM_VIEW, &view);
    Gfx_GetTransform(GFX_TRANSFORM_WORLD, &world);
    FloatMatrix_Multiply(&transform, &view, &world);
    m11 = transform._11;
    m21 = transform._21;
    m31 = transform._31;
    m12 = transform._12;
    m22 = transform._22;
    m32 = transform._32;
    for (i = 0; i < pMesh->triangleCount; i++) {
        for (j = 0; j < 3; j++) {
            int index = pMesh->pTriangles[i].vertexIndex[j];
            float *pVertex = (float *)((BYTE *)pVertexData + index * 0x30);
            float x = (pVertex[3] + pVertex[0]) * g_floatHalf;
            float y = (pVertex[4] + pVertex[1]) * g_floatHalf;
            float z = (pVertex[5] + pVertex[2]) * g_floatHalf;

            pVertex[10] = (z * m31 + y * m21 + x * m11 + g_netOne) * g_floatHalf;
            pVertex[11] = (g_netOne - (z * m32 + y * m22 + x * m12)) * g_floatHalf;
        }
    }
    pVertices = Gfx_LockVertexBuffer(CGraphics::m_pTextureManager->pVertexBuffers[pMesh->vertexBufferIndex]);
    memcpy((BYTE *)pVertices + pMesh->vertexOffset * 0x30, pMesh->pVertexData, pMesh->field_0x10 * 0x30);
    Gfx_UnlockVertexBuffer(CGraphics::m_pTextureManager->pVertexBuffers[pMesh->vertexBufferIndex]);
}

// Draws the triangles of a mesh in contiguous texture runs, setting the
// reserved cube map as texture and clamping the texture address while the
// shadow geometry is drawn.
// FUNCTION: CMR2 0x004b2610
void Graphics_DrawCubeMappedShadowTriangles(Mesh *pMesh)
{
    int triangleCount;
    int count;
    int currentTexture;
    int i;
    int textureIndex;
    int texture;

    Graphics_ProjectMeshVertexMidpoints(pMesh);
    count = 0;
    triangleCount = pMesh->triangleCount;
    currentTexture = -1;
    Graphics_SwitchAlphaBlendAndTest(1);
    CGraphics::SetTextureAddressClamp(0);
    if ((g_unk0x005210b8 < 0 && (int)g_carReflectionCubeMaps[(pMesh->flags >> 15) & 7] < 0) || triangleCount <= 0)
        goto done;
    for (i = 0; i < triangleCount; i++) {
        textureIndex = pMesh->pTriangles[i].field_0x30;
        if (currentTexture != pMesh->pTriangles[i].textures[textureIndex]) {
            if (count > 0) {
                Gfx_DrawIndexedPrimitiveVB(GFX_TRIANGLELIST, CGraphics::m_pTextureManager->pVertexBuffers[pMesh->vertexBufferIndex], pMesh->vertexOffset, pMesh->field_0x10, g_unk0x006dd9bc, count);
            }
            count = 0;
            currentTexture = pMesh->pTriangles[i].textures[textureIndex];
            if (currentTexture > -1) {
                if ((g_pGraphics->field913_0x3bc & 0x10) != 0) {
                    Frontend_SetObjectField118(CGraphics::m_pTextureManager->textureBuffer[currentTexture], 8);
                    CGraphics::ApplyTextureStageChange(0, CGraphics::m_pTextureManager->textureBuffer[currentTexture]);
                    Frontend_SetObjectField118(CGraphics::m_pTextureManager->textureBuffer[g_unk0x005210b8], 9);
                    if (g_unk0x005210bc != 0)
                        CGraphics::ApplyTextureStageChange(1, CGraphics::m_pTextureManager->textureBuffer[g_unk0x005210b8]);
                } else {
                    Frontend_SetObjectField118(CGraphics::m_pTextureManager->textureBuffer[g_unk0x005210b8], 4);
                    CGraphics::ApplyTextureStageChange(0, CGraphics::m_pTextureManager->textureBuffer[g_unk0x005210b8]);
                    Frontend_SetObjectField118(CGraphics::m_pTextureManager->textureBuffer[currentTexture], 5);
                    if (g_unk0x005210bc != 0)
                        CGraphics::ApplyTextureStageChange(1, CGraphics::m_pTextureManager->textureBuffer[currentTexture]);
                }
            }
        }
        if (currentTexture > -1) {
            g_unk0x006dd9bc[count++] = pMesh->pTriangles[i].vertexIndex[0];
            g_unk0x006dd9bc[count++] = pMesh->pTriangles[i].vertexIndex[1];
            g_unk0x006dd9bc[count++] = pMesh->pTriangles[i].vertexIndex[2];
            CGame::m_unk0x0059ce18++;
        }
    }
    if (count == 0)
        goto done;
    texture = pMesh->pTriangles[triangleCount - 1].textures[textureIndex];
    if (texture > -1) {
        if ((g_pGraphics->field913_0x3bc & 0x10) != 0) {
            Frontend_SetObjectField118(CGraphics::m_pTextureManager->textureBuffer[texture], 8);
            CGraphics::ApplyTextureStageChange(0, CGraphics::m_pTextureManager->textureBuffer[texture]);
            Frontend_SetObjectField118(CGraphics::m_pTextureManager->textureBuffer[g_unk0x005210b8], 9);
            if (g_unk0x005210bc != 0)
                CGraphics::ApplyTextureStageChange(1, CGraphics::m_pTextureManager->textureBuffer[g_unk0x005210b8]);
            } else {
            Frontend_SetObjectField118(CGraphics::m_pTextureManager->textureBuffer[g_unk0x005210b8], 4);
            CGraphics::ApplyTextureStageChange(0, CGraphics::m_pTextureManager->textureBuffer[g_unk0x005210b8]);
            Frontend_SetObjectField118(CGraphics::m_pTextureManager->textureBuffer[texture], 5);
            if (g_unk0x005210bc != 0)
                CGraphics::ApplyTextureStageChange(1, CGraphics::m_pTextureManager->textureBuffer[texture]);
        }
        Gfx_DrawIndexedPrimitiveVB(GFX_TRIANGLELIST, CGraphics::m_pTextureManager->pVertexBuffers[pMesh->vertexBufferIndex], pMesh->vertexOffset, pMesh->field_0x10, g_unk0x006dd9bc, count);
    }
done:
    CGraphics::SetTextureAddressClamp(1);
    CGraphics::ApplyTextureStageChange(1, 0);
    CGraphics::ApplyTextureStageChange(2, 0);
}

// Finds a loaded texture by name (not for "local" textures) and makes sure
// it is resident.
// FUNCTION: CMR2 0x004b9b80
Texture *Graphics_FindAndEnsureResidentTexture(char *name)
{
    int i;

    if (Graphics_HasLocalSuffix(name) == 0) {
        for (i = 0; i < (int)CGraphics::m_textureCount; i++) {
            if (CGraphics::m_pTextureManager->textureBuffer[i] != NULL &&
                strcmp(CGraphics::m_pTextureManager->textureBuffer[i]->name, name) == 0) {
                i = Graphics_CopyIntoFreeTextureSlot(CGraphics::m_pTextureManager->textureBuffer[i], i);
                return CGraphics::m_pTextureManager->textureBuffer[i];
            }
        }
    }
    return NULL;
}

// Non-zero while the draw lists are depth sorted before being drawn
// (0x49cbc0/0x49cc50/0x49cd20).
// GLOBAL: CMR2 0x005207b4
int g_depthSortDrawLists = 1;

// Draws the LOD record a mesh is using: takes the whole-triangle-list or the
// part-list path its flags ask for and binds cull mode, alpha blending and
// lighting for it, restoring the lighting mode afterwards.
// FUNCTION: CMR2 0x0049c940
void Graphics_DrawMeshLOD(Mesh *pMesh, int useParts, int clampTexture, int markTextures)
{
    Mesh *pLod = (Mesh *)((BYTE *)pMesh + pMesh->lodIndex * offsetof(Mesh, field_0x108));
    unsigned int lightingMode;

    if (pLod->pTriangles == NULL)
        return;

    // Triangle runs are only stored for meshes flagged with the run list.
    if (useParts == 0 && (pLod->flags & 0x2000) == 0)
        useParts = 1;

    if ((pLod->flags & 1) != 0)
        CGraphics::SetCullMode(1);
    else
        CGraphics::SetCullMode(CGame::GetSectorDrawState());

    // Meshes with flag 0x20 keep their own lighting mode and the restore at
    // the end skips them, so the saved mode stays uninitialised for them
    // (the original reads it anyway).
    if ((pLod->flags & 0x20) == 0) {
        lightingMode = (unsigned int)Graphics_GetGeometryStateValue();
        if ((pLod->flags & 0x40000) != 0)
            Graphics_SetLightingMode(3);
        else
            Graphics_SetLightingMode(2);
    }

    if (g_depthSortDrawLists != 0)
        Graphics_SwitchAlphaBlendAndTest((int)((pLod->flags >> 3) & 1));
    else
        Graphics_SwitchAlphaBlendAndTest(0);

    if (useParts != 0) {
        if (markTextures != 0)
            Graphics_MarkTexturesAndDrawMeshParts(pLod);
        else
            Graphics_DrawMeshPartsByTexture(pLod);
    } else if (clampTexture != 0) {
        Game_DrawMeshTextureRuns(pLod);
    } else {
        Graphics_DrawMeshTextureBatches(pLod);
    }

    Gfx_SetBodyDirt(0,0,0,0); // never pigment the additive reflection pass
    if ((pLod->flags & 0x200) != 0 && (pLod->flags & 0x40000) == 0 &&
        (g_pGraphics->field913_0x3bc & 8) != 0) {
        if ((g_pGraphics->field913_0x3bc & 0x80) != 0)
            Mesh_DrawEnvMapped(pLod);
        else
            Graphics_DrawCubeMappedShadowTriangles(pLod);
    }

    if ((pLod->flags & 0x20) == 0)
        Graphics_SetLightingMode(lightingMode);
}

// Called only inside a verified live car-node draw scope, then material
// qualified. Glass/frame atlases, wheels and unknown mod names stay clean.
void Graphics_BindCarPaintDirt(Texture *texture){
 int car=CarBodyDirt::drawingCar;
 if(car>=0&&car<8&&texture&&CarBodyDirt::IsPaintAtlas(texture->name,Sys_GetOptionText("video.body_dirt_extra",""))&&Sys_GetOption("video.body_dirt",1)){
  const auto &d=CarBodyDirt::cars[car];Gfx_SetBodyDirt(d.amount,d.red,d.green,d.blue);
 }else{
  Gfx_SetBodyDirt(0,0,0,0);
  // Name the textures a mod/overlay car body actually draws with, once each,
  // so the next native log can feed video.body_dirt_extra (or the known
  // list). Bounded: at most 16 unique stems per run, only while a car body
  // is being drawn and dirt is on.
  if(car>=0&&car<8&&texture&&Sys_GetOption("video.body_dirt",1)){
   char stem[32];CarBodyDirt::StemOf(texture->name,stem);
   if(stem[0]){
    static char seen[16][32]={};static int seenCount=0;bool dup=false;
    for(int i=0;i<seenCount;i++)if(std::strcmp(seen[i],stem)==0){dup=true;break;}
    if(!dup){
     if(seenCount<16){std::strncpy(seen[seenCount],stem,31);seen[seenCount][31]=0;seenCount++;
      Sys_Log("body dirt: unbound car texture '%s' (car %d; add to video.body_dirt_extra to pigment)",stem,car);}
    }
   }
  }
 }
}

// video.cockpit_glass_fog (default on): a part of a mesh flagged
// MESH_FLAG_GLASS_NO_ZWRITE that actually contains semi-transparent vertices
// (windscreen glass) is drawn without depth writes; fully opaque parts of the
// same mesh keep writing depth. Mesh flag 0x8 (alpha blending enabled for the
// mesh) is required - without blending the alpha would not show anyway.
// Diagnostic only (cockpit fog investigation). Counts draws of meshes carrying MESH_FLAG_GLASS_NO_ZWRITE (set on every
// mesh of the interior model when video.cockpit_glass_fog is on), by what the draw path did with them. These are draw
// counts per path, NOT a per-pixel or per-material classification: they cannot say which interior part is the windscreen,
// and meshes that are not flagged (anything outside the interior model) are not counted at all.
//   noZParts       part drawn with depth writes off (mesh flag 0x8 set and the part has alpha 1..249 vertices)
//   flagBlendOff   part of a flagged mesh whose mesh flag 0x8 (alpha blending) is clear: keeps writing depth
//   flagOpaqueAlpha part of a flagged mesh with 0x8 set but no vertex alpha in 1..249: keeps writing depth
//   batchMeshes    whole flagged meshes drawn by Graphics_DrawMeshTextureBatches, which does not apply the glass rule
unsigned g_interiorDiag[4] = {0, 0, 0, 0};
static inline void Graphics_CountInteriorGlassPart(const Mesh *pMesh, bool glassNoZ)
{
    if (!(pMesh->flags & MESH_FLAG_GLASS_NO_ZWRITE)) return;
    if (glassNoZ) g_interiorDiag[0]++;
    else if (!(pMesh->flags & 0x8u)) g_interiorDiag[1]++;
    else g_interiorDiag[2]++;
}
static bool Graphics_PartHasSemiTransparentAlpha(Mesh *pMesh, MeshPart *pPart)
{
    if (pMesh->pVertexData == NULL || pPart->pData == NULL || pPart->indexCount <= 0)
        return false;
    const MeshVertexF *verts = (const MeshVertexF *)pMesh->pVertexData;
    for (int i = 0; i < pPart->indexCount; i++) {
        DWORD d = verts[pPart->minIndex + pPart->pData[i]].diffuseColour;
        BYTE a = (BYTE)(d >> 24);
        if (a > 0 && a < 250)
            return true;
    }
    return false;
}

// Draws every part of a mesh from its vertex buffer, one texture at a time,
// and counts the triangles drawn.
// FUNCTION: CMR2 0x0049c880
void Graphics_DrawMeshPartsByTexture(Mesh *pMesh)
{
    int i;
    MeshPart **ppPart;
    MeshPart *pPart;

    i = 0;
    if (pMesh->partCount > 0) {
        ppPart = pMesh->pParts;
        do {
            pPart = *ppPart;
            CGraphics::ApplyTextureStageChange(0, CGraphics::m_pTextureManager->textureBuffer[pPart->texture]);
            Graphics_BindCarPaintDirt(CGraphics::m_pTextureManager->textureBuffer[pPart->texture]);
            bool glassNoZ = (pMesh->flags & (MESH_FLAG_GLASS_NO_ZWRITE | 0x8u)) == (MESH_FLAG_GLASS_NO_ZWRITE | 0x8u) &&
                            Graphics_PartHasSemiTransparentAlpha(pMesh, pPart);
            Graphics_CountInteriorGlassPart(pMesh, glassNoZ);
            if (glassNoZ) CGraphics::SetZWriteEnable(0);
            if(!SceneryPack_DrawFixedPart(pMesh,pPart))
            Gfx_DrawIndexedPrimitiveVB(GFX_TRIANGLELIST, CGraphics::m_pTextureManager->pVertexBuffers[pMesh->vertexBufferIndex], pMesh->vertexOffset + pPart->minIndex, pMesh->field_0x10 - pPart->minIndex, pPart->pData, pPart->indexCount);
            if (glassNoZ) CGraphics::SetZWriteEnable(1);
            CGame::m_unk0x0059ce18 += pPart->indexCount / 3;
            i++;
            ppPart++;
        } while (i < pMesh->partCount);
    }
}

// Same as Graphics_DrawMeshPartsByTexture, first marking each part's texture as used (10).
// FUNCTION: CMR2 0x0049c7b0
void Graphics_MarkTexturesAndDrawMeshParts(Mesh *pMesh)
{
    int i;
    MeshPart **ppPart;
    MeshPart *pPart;

    i = 0;
    if (pMesh->partCount > 0) {
        ppPart = pMesh->pParts;
        do {
            pPart = *ppPart;
            Frontend_SetObjectField118(CGraphics::m_pTextureManager->textureBuffer[pPart->texture], 10);
            CGraphics::ApplyTextureStageChange(0, CGraphics::m_pTextureManager->textureBuffer[pPart->texture]);
            Graphics_BindCarPaintDirt(CGraphics::m_pTextureManager->textureBuffer[pPart->texture]);
            bool glassNoZ = (pMesh->flags & (MESH_FLAG_GLASS_NO_ZWRITE | 0x8u)) == (MESH_FLAG_GLASS_NO_ZWRITE | 0x8u) &&
                            Graphics_PartHasSemiTransparentAlpha(pMesh, pPart);
            Graphics_CountInteriorGlassPart(pMesh, glassNoZ);
            if (glassNoZ) CGraphics::SetZWriteEnable(0);
            if(!SceneryPack_DrawFixedPart(pMesh,pPart))
            Gfx_DrawIndexedPrimitiveVB(GFX_TRIANGLELIST, CGraphics::m_pTextureManager->pVertexBuffers[pMesh->vertexBufferIndex], pMesh->vertexOffset + pPart->minIndex, pMesh->field_0x10 - pPart->minIndex, pPart->pData, pPart->indexCount);
            if (glassNoZ) CGraphics::SetZWriteEnable(1);
            CGame::m_unk0x0059ce18 += pPart->indexCount / 3;
            i++;
            ppPart++;
        } while (i < pMesh->partCount);
    }
}

// GLOBAL: CMR2 0x0059be74
unsigned short g_unk0x0059be74[2000];

// Draws a mesh's triangles in contiguous texture runs.
// match 50%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0049c680
void Graphics_DrawMeshTextureBatches(Mesh *pMesh)
{
    if (pMesh->flags & MESH_FLAG_GLASS_NO_ZWRITE) g_interiorDiag[3]++;   // diagnostic only, see g_interiorDiag
    MeshTriangle *pTri = pMesh->pTriangles;
    int total = pMesh->triangleCount;
    int count = 0;
    int currentTexture = -1;
    int n;

    for (n = total; n > 0; n--) {
        int texture = pTri->textures[pTri->field_0x2c];
        if (texture != currentTexture) {
            if (count > 0) {
                Gfx_DrawIndexedPrimitiveVB(GFX_TRIANGLELIST, CGraphics::m_pTextureManager->pVertexBuffers[pMesh->vertexBufferIndex], pMesh->vertexOffset, pMesh->field_0x10, g_unk0x0059be74, count);
            }
            count = 0;
            CGraphics::ApplyTextureStageChange(0, CGraphics::m_pTextureManager->textureBuffer[texture]);
            Graphics_BindCarPaintDirt(CGraphics::m_pTextureManager->textureBuffer[texture]);
            currentTexture = texture;
        }
        g_unk0x0059be74[count++] = pTri->vertexIndex[0];
        g_unk0x0059be74[count++] = pTri->vertexIndex[1];
        g_unk0x0059be74[count++] = pTri->vertexIndex[2];
        CGame::m_unk0x0059ce18++;
        pTri++;
    }
    if (count != 0) {
        MeshTriangle *pLast = &pMesh->pTriangles[total - 1];
        int texture = pLast->textures[pLast->field_0x2c];
        CGraphics::ApplyTextureStageChange(0, CGraphics::m_pTextureManager->textureBuffer[texture]);
        Graphics_BindCarPaintDirt(CGraphics::m_pTextureManager->textureBuffer[texture]);
        Gfx_DrawIndexedPrimitiveVB(GFX_TRIANGLELIST, CGraphics::m_pTextureManager->pVertexBuffers[pMesh->vertexBufferIndex], pMesh->vertexOffset, pMesh->field_0x10, g_unk0x0059be74, count);
    }
}


void DustPlume_AdvanceWorld(int step16_16){if(step16_16>0)worldDust.Step(step16_16/(65536.f*25.f));}

void DustPlume_Update(Particle *p, ParticleType *, SceneNode *)
{
    DustPlume_Step(*p, g_particleWind);
}

void SpeedMote_Update(Particle *p, ParticleType *, SceneNode *)
{
    SpeedMote_Step(*p);
}

void SpeedMote_SetRatio(float ratio) { g_speedMoteRatio = ratio; }

void SpeedMote_Spawn(FixVector *world)
{
    if (!world || !g_particles || !g_particleTypes || g_particleTypeCount <= SPEED_MOTE_TYPE || g_particleCount <= 64) return;
    int motes = 0, active = 0;
    for (int i=0; i<g_particleCount; ++i) {
        if (!g_particles[i].active) continue;
        ++active;
        if (g_particles[i].pType == &g_particleTypes[SPEED_MOTE_TYPE]) ++motes;
    }
    // Never evict original particles: Particle_Spawn reuses the oldest slot when the pool is full.
    if (!SpeedMote_HasBudget(active, motes, g_particleCount, Sys_GetOption("video.speed_motes_amount", 100))) { ++g_speedMotesBudgetSkipped; return; }
    FixVector still = {0,0,0};
    Particle_Spawn(SPEED_MOTE_TYPE, world, &still, world->y, NULL, NULL, 0, NULL, 1);
    ++g_speedMotesSpawned;
}

void SpeedMote_GetStats(int *live, unsigned *drawn)
{
    int n = 0;
    if (g_particles && g_particleTypes && g_particleTypeCount > SPEED_MOTE_TYPE)
        for (int i = 0; i < g_particleCount; ++i)
            if (g_particles[i].active && g_particles[i].pType == &g_particleTypes[SPEED_MOTE_TYPE]) ++n;
    *live = n; *drawn = g_speedMotesDrawn;
}

void SpeedMote_NoteSkippedGround(void) { ++g_speedMotesNoGround; }

void SnowGlint_Update(Particle *p, ParticleType *, SceneNode *)
{
    SnowGlint_Step(*p);
}

// Same pool discipline as the air motes: glints never evict original particles
// and stay within their small cap; Particle_Spawn reuses the oldest slot when full.
void SnowGlint_Spawn(FixVector *world, int peakAlpha)
{
    if (!world || !g_particles || !g_particleTypes || g_particleTypeCount <= SNOW_GLINT_TYPE || g_particleCount <= 64) return;
    int glints = 0, active = 0;
    for (int i=0; i<g_particleCount; ++i) {
        if (!g_particles[i].active) continue;
        ++active;
        if (g_particles[i].pType == &g_particleTypes[SNOW_GLINT_TYPE]) ++glints;
    }
    if (glints >= SNOW_GLINT_CAP || active >= g_particleCount - 64) return;
    FixVector still = {0,0,0};
    g_snowGlintNextPeak = peakAlpha;   // the init callback stamps this glint's peak
    Particle_Spawn(SNOW_GLINT_TYPE, world, &still, world->y, NULL, NULL, 0, NULL, 1);
}

// Stamps the peak alpha at spawn (see SnowGlint.h); the caller publishes it
// through g_snowGlintNextPeak right before Particle_Spawn.
void SnowGlint_Init(Particle *p, ParticleType *, void *)
{
    if (!p) return;
    int peak = g_snowGlintNextPeak;
    std::memcpy(p->pad0x5c, &peak, 4);
}

void AttractCapture_ViewSignature(int view, int *type, int *drone);   // Car.cpp
void AirMote_Update(Particle *p, ParticleType *type, SceneNode *)
{
    AirMote_Step(*p, g_airMoteDrift);
    // Size scales (AirMote.h): every view takes video.air_motes_size, the cockpit also its own extra scale.
    if (p && type) {
        int vt = 0, dr = 0;
        AttractCapture_ViewSignature(0, &vt, &dr);
        p->size = AirMote_Size(type->size, vt, Sys_GetOption("video.air_motes_cockpit_size", 50), Sys_GetOption("video.air_motes_size", 60));
    }
}

// Stamps the per-particle wander seed at spawn (see AirMote.h). callbackParam is unused;
// the caller publishes the seed through g_airMoteNextSeed right before Particle_Spawn.
void AirMote_Init(Particle *p, ParticleType *, void *)
{
    if (!p) return;
    unsigned seed = g_airMoteNextSeed ? g_airMoteNextSeed : 0x9e3779b9u;
    std::memcpy(p->pad0x5c, &seed, 4);
}

void AirMote_Spawn(FixVector *world)
{
    if (!world || !g_particles || !g_particleTypes || g_particleTypeCount <= AIR_MOTE_TYPE || g_particleCount <= 64) return;
    int motes = 0, active = 0;
    for (int i=0; i<g_particleCount; ++i) {
        if (!g_particles[i].active) continue;
        ++active;
        if (g_particles[i].pType == &g_particleTypes[AIR_MOTE_TYPE]) ++motes;
    }
    // Never evict original particles: Particle_Spawn reuses the oldest slot when the pool is full.
    if (!AirMote_HasBudget(active, motes, g_particleCount, Sys_GetOption("video.air_motes_amount", 200))) { ++g_airMotesBudgetSkipped; return; }
    FixVector still = {0,0,0};
    Particle_Spawn(AIR_MOTE_TYPE, world, &still, world->y, NULL, NULL, 0, NULL, 1);
    ++g_airMotesSpawned;
}

void AirMote_GetStats(int *live, unsigned *drawn)
{
    int n = 0;
    if (g_particles && g_particleTypes && g_particleTypeCount > AIR_MOTE_TYPE)
        for (int i = 0; i < g_particleCount; ++i)
            if (g_particles[i].active && g_particles[i].pType == &g_particleTypes[AIR_MOTE_TYPE]) ++n;
    *live = n; *drawn = g_airMotesDrawn;
}

void AirMote_LogStats(int stepsElapsed)
{
    static int steps = 0;
    steps += stepsElapsed > 0 ? 1 : 0;
    if (steps < 125) return; // about 5 s of 25 Hz steps
    steps = 0;
    int live = 0;
    if (g_particles && g_particleTypes && g_particleTypeCount > AIR_MOTE_TYPE)
        for (int i = 0; i < g_particleCount; ++i)
            if (g_particles[i].active && g_particles[i].pType == &g_particleTypes[AIR_MOTE_TYPE]) ++live;
    Sys_Log("air motes: spawned=%u budget_skipped=%u live=%d billboards_queued=%u refused_by_queue=%u (cumulative; queued = accepted by Billboard_Add, not proof of visible pixels)",
        g_airMotesSpawned, g_airMotesBudgetSkipped, live, g_airMotesDrawn, g_airMotesRefused);
}

void SpeedMote_LogStats(float speedRatio, int stepsElapsed)
{
    static int steps = 0;
    steps += stepsElapsed > 0 ? 1 : 0;
    if (steps < 125) return; // about 5 s of 25 Hz steps
    steps = 0;
    int live = 0;
    if (g_particles && g_particleTypes && g_particleTypeCount > SPEED_MOTE_TYPE)
        for (int i = 0; i < g_particleCount; ++i)
            if (g_particles[i].active && g_particles[i].pType == &g_particleTypes[SPEED_MOTE_TYPE]) ++live;
    Sys_Log("speed motes: speed_ratio=%.2f spawned=%u budget_skipped=%u no_ground_height=%u live=%d billboards_queued=%u refused_by_queue=%u (cumulative; queued = accepted by Billboard_Add, not proof of visible pixels)",
        speedRatio, g_speedMotesSpawned, g_speedMotesBudgetSkipped, g_speedMotesNoGround, live, g_speedMotesDrawn, g_speedMotesRefused);
    Sys_Log("billboard queue: peak_depth=%u of 800 refused_disabled=%u refused_full=%u (all billboard users, cumulative)",
        g_billboardPeakQueue, g_billboardRefusedDisabled, g_billboardRefusedFull);
}

void DustPlume_Emit(FixVector *world, FixVector *velocity, BYTE *colour, BYTE shade, BYTE viewMask)
{
    BYTE cloudColour[4]={0,0,0,255};
    if(colour){
        float r=colour[0]/255.f,g=colour[1]/255.f,b=colour[2]/255.f;
        DustPlume_CloudTint(r,g,b);
        cloudColour[0]=(BYTE)(r*255.f+.5f);
        cloudColour[1]=(BYTE)(g*255.f+.5f);
        cloudColour[2]=(BYTE)(b*255.f+.5f);
    }
    if(world && colour && viewMask){
        float r=cloudColour[0]/255.f,g=cloudColour[1]/255.f,b=cloudColour[2]/255.f;
        // Existing particle velocities are displacement per25Hz reference step.
        // Copy the birth impulse; never read a later car position or velocity.
        worldDust.Emit(world->x/65536.f,world->y/65536.f,world->z/65536.f,r,g,b,
            velocity?velocity->x*25.f/65536.f:0,velocity?velocity->z*25.f/65536.f:0);
    }
    if (!world || !colour || !g_particles || !g_particleTypes || g_particleTypeCount <= DUST_PLUME_TYPE || g_particleCount <= 64 || !viewMask) return;
    int clouds = 0, active = 0;
    for (int i=0; i<g_particleCount; ++i) {
        if (!g_particles[i].active) continue;
        ++active;
        if (g_particles[i].pType == &g_particleTypes[DUST_PLUME_TYPE]) ++clouds;
    }
    {   int plumeAmount=Sys_GetOption("video.dust_plume_amount",200);
        if (!DustPlume_HasBudget(active, clouds, g_particleCount, DustPlume_LiveCap(plumeAmount))) return; }
    // No attachment matrix or owner callback: this cloud stays on the old road.
    Particle_Spawn(DUST_PLUME_TYPE, world, velocity, world->y, NULL, cloudColour, shade, NULL, viewMask);
}
