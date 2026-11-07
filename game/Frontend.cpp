#include "Frontend.h"
#include "FloatToInt.h"
#include "Game.h"
#include "FileBuffer.h"
#include "GameInfo.h"
#include "CategoryBestResults.h"
#include "ProfileEventResults.h"
#include "RallyData.h"
#include "InstallInfo.h"
#include "main.h"
#include "Graphics.h"
#include "Sprite.h"
#include "FixedPoint.h"
#include "Texture.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Glow.h"

// Not ported/declared elsewhere yet.
void Scene_SetAmbient(BYTE *pColour, int boost);
void Particle_BuildTriangleStripIndices(void);
void Scene_InitFixedMathTables(void);
void Game_CreateSplashScene(void);

// Callees that live in other translation units and are not declared in their
// headers yet.
ProfileEventResults *RallyData_GetDriverEventResults(int index);
CategoryBestResults *RallyData_GetDriverBestResults(int index);
unsigned int RallyData_GetFlag30(void);
unsigned int RallyData_GetFlag23(void);
BYTE RallyData_GetDriverAutomaticGearbox(BYTE param1);
void RallyData_MarkDriverRecordDirty(int index);
BYTE RallyData_GetModeStageGroup(BYTE flags, char mode);
BYTE StageTiming_GetFinishStateEntry(int index);
BYTE StageTiming_GetFinishStateByte(void);
unsigned int StageTiming_GetDriverSplitClock(int index, int split);
int StageTiming_GetCarSplitMarker(int car, int index);
int StageTiming_GetCheckpointField2(int index);
int Stage_GetSplitPositionCount(void);
extern BYTE g_unk0x008180fa;

// Defined later in this file (device setup helpers).
BYTE *Frontend_GetEventRallyStageTime(int index);
BYTE *Frontend_GetEventRallyResult(int index);
void Frontend_CopyOptionWord(int *pDest, int *pSource);
void Frontend_MergeFourNibbleOptionFields(int *pDest, int *pSource);
void Frontend_MergeFourTwoSixBitOptionFields(int *pDest, int *pSource);
void Frontend_MergeThreeTwoSixBitOptionFields(int *pDest, int *pSource);
void Frontend_MergeNamedRecordFourBitFields(int *pDest, int *pSource);
void Frontend_MergeNamedRecordSevenBitFields(int *pDest, int *pSource);
void Frontend_MergeNamedRecordSixBitFields(int *pDest, int *pSource);

int FrontendRecords_InsertStageDeviceRecord(int param1, int index, char *pName);
int FrontendRecords_InsertStageCategoryRecord(int param1, int index, char *pName);
char FrontendRecords_InsertArcadeDeviceRecord(int param1, int index, char *pName);

char CFrontend::m_stringDest[MAX_PATH];

char CFrontend::m_feRes640CountrySpecific[19] = "%s%sFERes640%c.bfl";
char CFrontend::m_feRes640CCountrySpecific[20] = "%s%sFERes640%cC.bfl";
char CFrontend::m_feRes1024CountrySpecific[20] = "%s%sFERes1024%c.bfl";
char CFrontend::m_feRes1024CCountrySpecific[21] = "%s%sFERes1024%cC.bfl";
char CFrontend::m_feRes640[14] = "%s\\Res640.bfl";
char CFrontend::m_feRes640C[15] = "%s\\Res640C.bfl";
char CFrontend::m_feRes1024[15] = "%s\\Res1024.bfl";
char CFrontend::m_feRes1024C[16] = "%s\\Res1024C.bfl";
char CFrontend::m_strFrontendTexturesAr640ATGA[36] = "%s\\frontend\\Textures\\Ar_640A.tga";
char CFrontend::m_strFrontendTexturesAr640DTGA[36] = "%s\\frontend\\Textures\\Ar_640D.tga";
char CFrontend::m_strFrontendTexturesLgMatrixTGA[36] = "%s\\frontend\\Textures\\LgMatrix.tga";
char CFrontend::m_strFrontendTexturesSmMatrixTGA[36]= "%s\\frontend\\Textures\\SmMatrix.tga";

char CFrontend::m_strFrontEndBinkTGA[24] = "%s\\FrontEnd\\bink.tga";
char CFrontend::m_strFrontEndTexturesCopyright[40] = "%s\\FrontEnd\\Textures\\Copyright%d.tga";
char CFrontend::m_strFrontEndTexturesCmr2TGA[32] = "%s\\FrontEnd\\Textures\\cmr2.tga";

Texture* CFrontend::m_pAr640ATexture;
Texture* CFrontend::m_pAr640DTexture;
Texture* CFrontend::m_matrixTextures[4];
Texture* CFrontend::m_pSmMatrixTexture;
Texture* CFrontend::m_pSetupRepBanners[8];
Texture* CFrontend::m_pTinyFlags[8];

Texture* CFrontend::m_unk0x00817ebc;
Texture* CFrontend::m_unk0x00817fd0;
Texture* CFrontend::m_unk0x00817fd4[4];

char CFrontend::m_strUK[3] = "UK";
char CFrontend::m_strIta[4] = "Ita";
char CFrontend::m_strKen[4] = "Ken";
char CFrontend::m_strItaly[8] = "Italy";
char CFrontend::m_strAus[4] = "Aus";
char CFrontend::m_strSwe[4] = "Swe";
char CFrontend::m_strKenya[8] = "Kenya";
char CFrontend::m_strFra[4] = "Fra";
char CFrontend::m_strSweden[8] = "Sweden";
char CFrontend::m_strFrance[8] = "France";
char CFrontend::m_strGre[4] = "Gre";
char CFrontend::m_strFin[4] = "Fin";
char CFrontend::m_strFinland[8] = "Finland";
char CFrontend::m_strGreece[8] = "Greece";

char CFrontend::m_strEsc[4] = "Esc";
char CFrontend::m_strPum[4] = "Pum";
char CFrontend::m_str205[4] = "205";
char CFrontend::m_strStr[4] = "Str";
char CFrontend::m_str6R4[4] = "6R4";
char CFrontend::m_strMin[4] = "Min";
char CFrontend::m_strSie[4] = "Sie";
char CFrontend::m_strIA2[4] = "IA2";
char CFrontend::m_strIA1[4] = "IA1";
char CFrontend::m_strInt[4] = "Int";
char CFrontend::m_strSea[4] = "Sea";
char CFrontend::m_str206[4] = "206";
char CFrontend::m_strSubShort[4] = "Sub";
char CFrontend::m_strCor[4] = "Cor";
char CFrontend::m_strMA3[4] = "MA3";
char CFrontend::m_strMA2[4] = "MA2";
char CFrontend::m_strMA1[4] = "MA1";
char CFrontend::m_strMit[4] = "Mit";
char CFrontend::m_strFA2[4] = "FA2";
char CFrontend::m_strFA1[4] = "FA1";
char CFrontend::m_strF99Short[4] = "F99";
char CFrontend::m_strFoc[4] = "Foc";
char CFrontend::m_strEscort[7] = "escort";
char CFrontend::m_strPuma[7] = "puma";
char CFrontend::m_strStrts[7] = "strts";
char CFrontend::m_str6r4[4] = "6r4";
char CFrontend::m_strMini[7] = "mini";
char CFrontend::m_strSerra[7] = "serra";
char CFrontend::m_strDelta[7] = "delta";
char CFrontend::m_strCrdba[7] = "crdba";
char CFrontend::m_strSub[4] = "sub";
char CFrontend::m_strCrlla[7] = "crlla";
char CFrontend::m_strLncer[7] = "lncer";
char CFrontend::m_strF99[4] = "f99";
char CFrontend::m_strF2000[7] = "f2000";

char CFrontend::m_strSetupRepTexturesBanners[40] = "%s\\setuprep\\Textures\\banners\\b%s.tga";
char CFrontend::m_strFrontendTinyFlags[40] = "%s\\frontend\\Textures\\tinyflags\\t%s.tga";
char CFrontend::m_strFrontendTexturesTBronzeTGA[36] = "%s\\frontend\\Textures\\tbronze.tga";
char CFrontend::m_strFrontendTexturesTSilverTGA[36] = "%s\\frontend\\Textures\\tsilver.tga";
char CFrontend::m_strFrontendTexturesTGoldTGA[32] = "%s\\frontend\\Textures\\tgold.tga";

char CFrontend::m_strFrontendTexturesCarsLivery[40] = "%s\\frontend\\Textures\\cars\\livery%d.tga";
char CFrontend::m_strFrontendTexturesCarsB01[36] = "%s\\frontend\\Textures\\cars\\%sB01.tga";
char CFrontend::m_strFrontendTexturesCarsF01[36] = "%s\\frontend\\Textures\\cars\\%sF01.tga";

GenericFile CFrontend::m_unk0x00818260;
GenericFile CFrontend::m_languageFiles[5];
GenericFile CFrontend::m_commonFile;
char CFrontend::m_strCommonBfl[14] = "%s\\Common.bfl";
char CFrontend::m_strEnglishTextBfl[20] = "%s%sEnglishText.bfl";
char CFrontend::m_strFrenchTextBfl[19] = "%s%sFrenchText.bfl";
char CFrontend::m_strGermanTextBfl[19] = "%s%sGermanText.bfl";
char CFrontend::m_strSpanishTextBfl[20] = "%s%sSpanishText.bfl";
char CFrontend::m_strItalianTextBfl[20] = "%s%sItalianText.bfl";
char CFrontend::m_strPolishTextBfl[19] = "%s%sPolishText.bfl";
char CFrontend::m_strEngUSATextBfl[19] = "%s%sEngUSAText.bfl";

Texture* CFrontend::m_unk0x00818530[3];
char* CFrontend::m_unk0x0081853c;
Texture* CFrontend::m_unk0x008182cc[22];
Texture* CFrontend::m_unk0x0081884c[22];

// FUNCTION: CMR2 0x004b7520
DWORD CFrontend::GetDeviceCapabilityFieldA8(void)
{
    return CGraphics::m_d3dDeviceDesc7.field0xa8;
}

// FUNCTION: CMR2 0x004d21e0
void CFrontend::LoadFrontendResourceArchives(void)
{
    unsigned int regionID;
    char regionKey;

    if (CGenericFileLoader::m_genericFile.buffer != NULL)
    {
        CFileBuffer::FreeGenericFileBuffer(CGenericFileLoader::m_genericFile.buffer);
        CGenericFileLoader::m_genericFile.buffer = NULL;
    }

    CGenericFileLoader::m_genericFile.didFileLoad = 0;
    CGenericFileLoader::m_genericFile.fileSize = 0;

    if (CGameInfo::GetScreenWidth() >= 1024U && IsTextureWidthSupported(1024) != 0 && IsTextureHeightSupported(1024) != 0)
    {
        if (HasDXT1TextureSupport() != FALSE)
            sprintf(m_stringDest, m_feRes1024C, CInstallInfo::GetFrontendDir());
        else
            sprintf(m_stringDest, m_feRes1024, CInstallInfo::GetFrontendDir());
    }
    else
    {
        if (HasDXT1TextureSupport() != FALSE)
            sprintf(m_stringDest, m_feRes640C, CInstallInfo::GetFrontendDir());
        else
            sprintf(m_stringDest, m_feRes640, CInstallInfo::GetFrontendDir());
    }

    CGenericFileLoader::LoadIntoFileRecord(&CGenericFileLoader::m_genericFile, m_stringDest);
    if (m_unk0x00818260.buffer != NULL)
    {
        CFileBuffer::FreeGenericFileBuffer(m_unk0x00818260.buffer);
        m_unk0x00818260.buffer = NULL;
    }
    m_unk0x00818260.didFileLoad = 0;
    m_unk0x00818260.fileSize = 0;

    // almost certainly not how this was written but this gets us better instruction matching
    // this is basically (if regionID is 3, then use poland)
    regionID = CGameInfo::GetGameRegion();
    switch (regionID) {
    case 0:
        regionKey = 0x45; // E
        break;
    case 1:
        regionKey = 0x45; // E
        break;
    case 2:
        regionKey = 0x45; // E
        break;
    default:
        regionKey = 0x50; // P
        break;
    }

    if (CGameInfo::GetScreenWidth() >= 1024U && IsTextureWidthSupported(1024) != FALSE && IsTextureHeightSupported(1024) != FALSE)
    {
        if (HasDXT1TextureSupport())
            sprintf(m_stringDest, m_feRes1024CCountrySpecific, CInstallInfo::GetCountrySpecificDir(), CGameInfo::GetGameRegionDirectory(), regionKey);
        else
            sprintf(m_stringDest, m_feRes1024CountrySpecific, CInstallInfo::GetCountrySpecificDir(), CGameInfo::GetGameRegionDirectory(), regionKey);
    }
    else
    {
        if (HasDXT1TextureSupport())
            sprintf(m_stringDest, m_feRes640CCountrySpecific, CInstallInfo::GetCountrySpecificDir(), CGameInfo::GetGameRegionDirectory(), regionKey);
        else
            sprintf(m_stringDest, m_feRes640CountrySpecific, CInstallInfo::GetCountrySpecificDir(), CGameInfo::GetGameRegionDirectory(), regionKey);
    }

    CGenericFileLoader::LoadIntoFileRecord(&m_unk0x00818260, m_stringDest);
}

// FUNCTION: CMR2 0x004b7560
BOOL CFrontend::IsTextureWidthSupported(unsigned int param_1)
{
    if (param_1 >= CGraphics::m_d3dDeviceDesc7.minTextureWidth && param_1 <= CGraphics::m_d3dDeviceDesc7.maxTextureWidth)
        return TRUE;

    return FALSE;
}

// FUNCTION: CMR2 0x004b7590
BOOL CFrontend::IsTextureHeightSupported(unsigned int param_1)
{
    if (param_1 >= CGraphics::m_d3dDeviceDesc7.minTextureHeight && param_1 <= CGraphics::m_d3dDeviceDesc7.maxTextureHeight)
        return TRUE;

    return FALSE;
}

// FUNCTION: CMR2 0x004a9700
BOOL CFrontend::HasDXT1TextureSupport(void)
{
    if (!CGraphics::m_hasTexFormatDXT1_16 && !CGraphics::m_hasTexFormatDXT1_32)
        return FALSE;

    return TRUE;
}

// FUNCTION: CMR2 0x004d2590
void CFrontend::LoadFrontendCarPreviewTextures(void) {
    int index;
    int ix;
    const char* carNames[22] = {
        m_strF2000,
        m_strF99,
        CMain::m_logFileBlankLine,
        CMain::m_logFileBlankLine,
        m_strLncer,
        CMain::m_logFileBlankLine,
        CMain::m_logFileBlankLine,
        CMain::m_logFileBlankLine,
        m_strCrlla,
        m_strSub,
        m_str206,
        m_strCrdba,
        m_strDelta,
        CMain::m_logFileBlankLine,
        CMain::m_logFileBlankLine,
        m_strSerra,
        m_strMini,
        m_str6r4,
        m_strStrts,
        m_str205,
        m_strPuma,
        m_strEscort
    };

    index = -2;
    ix = 0;
    
    do {
        const char* carName = carNames[ix];
        if (carName[0] != '\0') {
            // Load front texture (F01)
            sprintf(CFrontend::m_stringDest, CFrontend::m_strFrontendTexturesCarsF01, CInstallInfo::GetGameCDPath(), carName);
            m_unk0x0081884c[ix] = CTexture::FindLoadTexture(CGenericFileLoader::GetGenericFile(), CFrontend::m_stringDest, 0, 0, 0, 0);
            
            // Load back texture (B01)
            sprintf(CFrontend::m_stringDest,CFrontend::m_strFrontendTexturesCarsB01, CInstallInfo::GetGameCDPath(), carName);
            m_unk0x008182cc[ix] = CTexture::FindLoadTexture(CGenericFileLoader::GetGenericFile(), CFrontend::m_stringDest, 0, 0, 0, 0);
        } else {
            // Handle blank entries with switch/case based on index
            switch(index) {
                case 0:  // index 0, 1
                case 1:
                    m_unk0x0081884c[ix] = m_unk0x0081884c[1];
                    m_unk0x008182cc[ix] = m_unk0x008182cc[1];
                    break;
                    
                case 3:   // index 3, 4, 5
                case 4:
                case 5:
                    m_unk0x0081884c[ix] = m_unk0x0081884c[4];
                    m_unk0x008182cc[ix] = m_unk0x008182cc[4];
                    break;
                    
                case 11:  // index 11, 12
                case 12:
                    m_unk0x0081884c[ix] = m_unk0x0081884c[12];
                    m_unk0x008182cc[ix] = m_unk0x008182cc[12];
                    break;
            }
        }
        
        index++;
        ix++;
    } while (index + 2 < 22);
    
    // Load livery textures (1-3)
    int liveryIndex = 0;
    Texture** pTexture = m_unk0x00818530;

    do {
        liveryIndex++;  // Increment first so we use 1, 2, 3
        sprintf(CFrontend::m_stringDest, CFrontend::m_strFrontendTexturesCarsLivery, CInstallInfo::GetGameCDPath(), liveryIndex);
        *pTexture = CTexture::FindLoadTexture(CGenericFileLoader::GetGenericFile(), CFrontend::m_stringDest, 0, 0, 0, 0);
        pTexture++;
    } while ((INT_PTR)pTexture < (INT_PTR)(m_unk0x00818530 + 3)); // 0x81853c in the original
}

// Returns the common frontend archive (Common.bfl).
// FUNCTION: CMR2 0x004d2190
GenericFile* CFrontend::GetCommonFrontendArchive(void) {
    return &m_commonFile;
}

// Returns the archive the copyright screens are loaded from (the one
// LoadFrontendResourceArchives opens).
// FUNCTION: CMR2 0x004d21b0
GenericFile* CFrontend::GetCopyrightFrontendArchive(void) {
    return &m_unk0x00818260;
}

// Returns the archive of one frontend language.
// FUNCTION: CMR2 0x004d21c0
GenericFile* CFrontend::GetLocalizedFrontendArchive(int language) {
    return &m_languageFiles[language];
}

// Loads the splash screen textures: the CMR2 logo, one "Copyright" screen per
// page and the Bink logo. The textures live inside the .bfl archives that
// LoadLanguageFiles opens, so they are looked up by name in those files.
// FUNCTION: CMR2 0x004d08d0
bool CFrontend::LoadSplashScreens(bool param1) {
    BYTE colour[4];
    colour[0] = 0xff;
    colour[1] = 0xff;
    colour[2] = 0xff;
    colour[3] = 0xff;
    int screenCount;
    int i;

    if (param1) {
        CGraphics::RecreateGraphicsDeviceAndResources(CGameInfo::GetScreenWidth(), CGameInfo::GetScreenHeight(), CGameInfo::GetColourDepth(), CGameInfo::GetGraphicsOptionBits5To8(), CGameInfo::GetGraphicsOptionBits9To12());
        Scene_InitFixedMathTables();
        Sprite_Init();
        Line2D_Init();
        Tri2D_Init();
    }

    RallyData_ValidateIndex(0);
    Glow_ResetEntries();
    SaveFrontendCallbackDepth();
    LoadLanguageFiles();

    if (CGameInfo::GetScreenWidth() >= 0x400 && IsTextureWidthSupported(0x400) != 0 && IsTextureHeightSupported(0x400) != 0)
        screenCount = 4;
    else
        screenCount = 3;

    Game_CreateSplashScene();
    CGame::SetSectorDrawState(3);
    Particle_BuildTriangleStripIndices();
    Scene_SetAmbient((BYTE *)&colour, 0);

    sprintf(m_stringDest, m_strFrontEndTexturesCmr2TGA, CInstallInfo::GetGameCDPath());
    m_unk0x00817fd0 = CTexture::FindLoadTexture(CGenericFileLoader::GetGenericFile(), m_stringDest, 0, 0, 0, 0);

    for (i = 0; i < screenCount; i++) {
        sprintf(m_stringDest, m_strFrontEndTexturesCopyright, CInstallInfo::GetGameCDPath(), i + 1);
        m_unk0x00817fd4[i] = CTexture::FindLoadTexture(GetCopyrightFrontendArchive(), m_stringDest, 0, 0, 0, 0);
    }

    sprintf(m_stringDest, m_strFrontEndBinkTGA, CInstallInfo::GetGameCDPath());
    m_unk0x00817ebc = CTexture::FindLoadTexture(CGenericFileLoader::GetGenericFile(), m_stringDest, 0, 0, 0, 0);

    return true;
}

// FUNCTION: CMR2 0x004d2100
bool CFrontend::ReleaseLanguageFiles(void)
{
    int i;

    if (m_commonFile.buffer != NULL) {
        CFileBuffer::FreeGenericFileBuffer(m_commonFile.buffer);
        m_commonFile.buffer = NULL;
    }
    m_commonFile.didFileLoad = FALSE;
    m_commonFile.fileSize = 0;
    if (CGenericFileLoader::m_genericFile.buffer != NULL) {
        CFileBuffer::FreeGenericFileBuffer(CGenericFileLoader::m_genericFile.buffer);
        CGenericFileLoader::m_genericFile.buffer = NULL;
    }
    CGenericFileLoader::m_genericFile.didFileLoad = FALSE;
    CGenericFileLoader::m_genericFile.fileSize = 0;
    if (m_unk0x00818260.buffer != NULL) {
        CFileBuffer::FreeGenericFileBuffer(m_unk0x00818260.buffer);
        m_unk0x00818260.buffer = NULL;
    }
    m_unk0x00818260.didFileLoad = FALSE;
    m_unk0x00818260.fileSize = 0;
    for (i = 0; i < 5; i++) {
        if (m_languageFiles[i].buffer != NULL) {
            CFileBuffer::FreeGenericFileBuffer(m_languageFiles[i].buffer);
            m_languageFiles[i].buffer = NULL;
        }
        m_languageFiles[i].didFileLoad = FALSE;
        m_languageFiles[i].fileSize = 0;
    }
    return true;
}

// FUNCTION: CMR2 0x004d2380
void CFrontend::LoadLanguageFiles(void)
{
    char *format;

    sprintf(m_stringDest, m_strCommonBfl, CInstallInfo::GetFrontendDir());
    CGenericFileLoader::LoadIntoFileRecord(&m_commonFile, m_stringDest);
    LoadFrontendResourceArchives();

    switch (CGameInfo::GetGameRegion()) {
    case 0:
        sprintf(m_stringDest, m_strEnglishTextBfl, CInstallInfo::GetCountrySpecificDir(), CGameInfo::GetGameRegionDirectory());
        CGenericFileLoader::LoadIntoFileRecord(&m_languageFiles[0], m_stringDest);
        sprintf(m_stringDest, m_strFrenchTextBfl, CInstallInfo::GetCountrySpecificDir(), CGameInfo::GetGameRegionDirectory());
        CGenericFileLoader::LoadIntoFileRecord(&m_languageFiles[1], m_stringDest);
        sprintf(m_stringDest, m_strGermanTextBfl, CInstallInfo::GetCountrySpecificDir(), CGameInfo::GetGameRegionDirectory());
        CGenericFileLoader::LoadIntoFileRecord(&m_languageFiles[4], m_stringDest);
        sprintf(m_stringDest, m_strSpanishTextBfl, CInstallInfo::GetCountrySpecificDir(), CGameInfo::GetGameRegionDirectory());
        CGenericFileLoader::LoadIntoFileRecord(&m_languageFiles[2], m_stringDest);
        sprintf(m_stringDest, m_strItalianTextBfl, CInstallInfo::GetCountrySpecificDir(), CGameInfo::GetGameRegionDirectory());
        CGenericFileLoader::LoadIntoFileRecord(&m_languageFiles[3], m_stringDest);
        break;
    case 1:
        sprintf(m_stringDest, m_strEngUSATextBfl, CInstallInfo::GetCountrySpecificDir(), CGameInfo::GetGameRegionDirectory());
        CGenericFileLoader::LoadIntoFileRecord(&m_languageFiles[0], m_stringDest);
        sprintf(m_stringDest, m_strFrenchTextBfl, CInstallInfo::GetCountrySpecificDir(), CGameInfo::GetGameRegionDirectory());
        CGenericFileLoader::LoadIntoFileRecord(&m_languageFiles[1], m_stringDest);
        sprintf(m_stringDest, m_strSpanishTextBfl, CInstallInfo::GetCountrySpecificDir(), CGameInfo::GetGameRegionDirectory());
        CGenericFileLoader::LoadIntoFileRecord(&m_languageFiles[2], m_stringDest);
        break;
    case 2:
        format = m_strEnglishTextBfl;
        sprintf(m_stringDest, format, CInstallInfo::GetCountrySpecificDir(), CGameInfo::GetGameRegionDirectory());
        CGenericFileLoader::LoadIntoFileRecord(&m_languageFiles[0], m_stringDest);
        break;
    default:
        format = m_strPolishTextBfl;
        sprintf(m_stringDest, format, CInstallInfo::GetCountrySpecificDir(), CGameInfo::GetGameRegionDirectory());
        CGenericFileLoader::LoadIntoFileRecord(&m_languageFiles[0], m_stringDest);
        break;
    }
    CGame::RegisterCallback(ReleaseLanguageFiles, NULL);
}

char **CFrontend::m_textStrings;
int CFrontend::m_textFirstId;
int CFrontend::m_textCount;
char CFrontend::m_strInvalidTextString[20] = "INVALID TEXT STRING";
BYTE CFrontend::m_textLanguage;
BYTE CFrontend::m_textLanguageCount;

// Looks up one of the loaded text strings; out-of-range ids fall back to the
// "INVALID TEXT STRING" placeholder.
// Selects the language of the text strings.
// FUNCTION: CMR2 0x004a3c30
void Frontend_SelectTextLanguage(int language)
{
    if (language < CFrontend::m_textLanguageCount) {
        CFrontend::m_textLanguage = language;
        CFrontend::m_textFirstId = CFrontend::m_textCount * language;
    }
}

BYTE *Frontend_TerminateTextLine(BYTE *p, char next);

// Splits the loaded text files (one per language) into the string table.
// FUNCTION: CMR2 0x004a3c90
void CFrontend::BuildLocalizedTextStringTable(int languages, int count, BYTE **pFiles)
{
    int i;
    int j;
    int row;
    BYTE *p;

    m_textLanguageCount = (BYTE)languages;
    m_textLanguage = 0;
    m_textFirstId = 0;
    m_textCount = count;
    m_textStrings = (char **)CFileBuffer::AllocateLockedBuffer(languages * count * sizeof(char *));
    if (m_textStrings != NULL) {
        for (i = 0, row = 0; i < languages; i++, row += count) {
            p = pFiles[i];
            for (j = 0; j < count; j++) {
                m_textStrings[row + j] = (char *)p;
                if (j < count - 1)
                    p = Frontend_TerminateTextLine(p, 1);
                else
                    Frontend_TerminateTextLine(p, 0);
            }
        }
    }
}

// FUNCTION: CMR2 0x004a3c60
char *CFrontend::GetTextString(int index)
{
    if (index >= m_textCount || index < 0)
        return m_strInvalidTextString;
    return m_textStrings[m_textFirstId + index];
}

// GLOBAL: CMR2 0x00817fe4
int g_frontendTimestamp;
// GLOBAL: CMR2 0x00817fe8
int g_frontendCallbackDepth;
// GLOBAL: CMR2 0x00817fec
BYTE g_frontendIntroFlag;
// GLOBAL: CMR2 0x00817ff4
unsigned int g_frontendElapsedMs;

// FUNCTION: CMR2 0x004d20c0
void CFrontend::SaveFrontendCallbackDepth(void)
{
    g_frontendCallbackDepth = CGame::GetCallbackCount();
}

// FUNCTION: CMR2 0x004d20d0
unsigned int CFrontend::GetFrontendElapsedMilliseconds(void)
{
    return g_frontendElapsedMs;
}

// FUNCTION: CMR2 0x004d20e0
int CFrontend::GetFrontendTimestamp(void)
{
    return g_frontendTimestamp;
}

// FUNCTION: CMR2 0x004d20f0
BYTE CFrontend::GetFrontendIntroFlag(void)
{
    return g_frontendIntroFlag;
}

// GLOBAL: CMR2 0x00818540
char *g_unk0x00818540;
// GLOBAL: CMR2 0x00818544
char *g_unk0x00818544;

// Colours of the background matrix cells (RGBA), indexed by the cell map.
// GLOBAL: CMR2 0x00523ef0
unsigned int g_matrixColours[16] = {
    0xff404040, 0xffffffff, 0xff0000ff, 0xff00ff00,
    0xffff0000, 0xff00ffff, 0xffde9c08, 0xff000000,
    0xff000000, 0xff000000, 0xff000000, 0xff000000,
    0xff000000, 0xff000000, 0xff000000, 0xffff00ff,
};
// Ripple phases of the background matrix, advanced with the frame time.
// GLOBAL: CMR2 0x008189b0
short g_matrixPhase1;
// GLOBAL: CMR2 0x008189b2
short g_matrixPhase2;
extern short g_frontendDrawRect[4];

// Height of a ripple centred on pCentre at (x, y): sine of the distance,
// with the given phase and wavelength (all 16.16).
// FUNCTION: CMR2 0x004d2bd0
int Frontend_ComputeRippleHeight(int *pCentre, int x, int y, int phase, int wavelength)
{
    FixVector d;

    d.z = 0;
    d.x = x - pCentre[0];
    d.y = y - pCentre[1];
    return g_sinTable[(FixDiv(FixVecLength(&d) % wavelength, wavelength) + phase) & 0xfff];
}

// Draws the one-pixel outline of pRect (x, y, width, height) in pColour as
// four one-pixel thick rectangles.
// FUNCTION: CMR2 0x004d27e0
void Frontend_DrawRectangleOutline(short *pRect, BYTE *pColour)
{
    short rect[4];

    rect[0] = pRect[0];
    rect[1] = pRect[1] + 1;
    rect[2] = 1;
    rect[3] = pRect[3] - 2;
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, pColour, 4);
    rect[0] = pRect[2] + pRect[0] - 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, pColour, 4);
    rect[0] = pRect[0];
    rect[1] = pRect[1];
    rect[2] = pRect[2];
    rect[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, pColour, 4);
    rect[1] = pRect[1] + pRect[3] - 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, pColour, 4);
}

// Draws the animated frontend background: an 18x12 grid of the large matrix
// texture, coloured by pMap and rippling from two centres (top-left and
// top-right of the screen).
// match 78%: every instruction matches except the stack slot of the loop
// counter `i` (and of the /QIfist temporaries that follow it); the frame size
// and the array slots are identical. Reordering the declarations does not move
// it (MSVC6 assigns slots by first use), so this is an allocation ceiling.
// FUNCTION: CMR2 0x004d28c0
void FrontendDraw_AnimatedMatrixBackground(short x0, short y0, char *pMap)
{
    int centre2[2];
    int centre1[2];
    BYTE colour[4];
    char *pCell;
    int u;
    int v;
    int wave1;
    int wave;
    int brightness;
    int i;
    int j;

    colour[0] = 0xff;
    centre1[0] = 0;
    centre1[1] = 0;
    colour[1] = 0xff;
    colour[2] = 0xff;
    colour[3] = 0xff;
    centre2[0] = g_pGraphics->resX;
    centre2[1] = 0;
    g_matrixPhase1 = (short)(((CMain::GetFrameDelta() + 1) * -0x6000) / 360);
    g_matrixPhase2 = (short)(((CMain::GetFrameDelta() + 1) * -0x3000) / 360);
    // PORT: retain the original matrix, palette and per-dot ripple exactly;
    // only the circular sprite's enlarged edge is evaluated as geometry.
    Gfx_VectorizeMenuDot(CFrontend::m_matrixTextures[FRONTEND_LG_MATRIX]->pSurface);
    g_frontendDrawRect[2] = CFrontend::m_matrixTextures[FRONTEND_LG_MATRIX]->width;
    g_frontendDrawRect[3] = CFrontend::m_matrixTextures[FRONTEND_LG_MATRIX]->height;
    for (i = 0; i < 0x12; i++) {
        pCell = pMap + i;
        for (j = 0; j < 0xc; j++) {
            g_frontendDrawRect[0] = (int)(g_pGraphics->resX * 18) / 640 * i + x0;
            g_frontendDrawRect[1] = (int)(g_pGraphics->resY * 18) / 480 * j + y0;
            u = FixDiv((int)FTOI(__int64, X87S_MUL(g_frontendDrawRect[0], CGraphics::m_65536)),
                       (int)FTOI(__int64, X87S_MUL((int)g_pGraphics->resX, CGraphics::m_65536)));
            v = FixDiv((int)FTOI(__int64, X87S_MUL(g_frontendDrawRect[1], CGraphics::m_65536)),
                       (int)FTOI(__int64, X87S_MUL((int)g_pGraphics->resX, CGraphics::m_65536)));
            wave1 = Frontend_ComputeRippleHeight(centre1, u, v, g_matrixPhase1, 0x20000);
            wave = (Frontend_ComputeRippleHeight(centre2, u, v, g_matrixPhase2, 0x140000) + wave1) / 2;
            g_frontendDrawRect[0] -= FixMulShift32(0x30000, wave);
            g_frontendDrawRect[1] -= (FixMul(0x30000, wave) >> 16);
            *(unsigned int *)colour = g_matrixColours[*pCell];
            brightness = wave / 4 + 0xc000;
            colour[0] = (FixMul((int)FTOI(__int64, X87S_MUL(colour[0], CGraphics::m_65536)), brightness) >> 16);
            colour[1] = FixMulShift32((int)FTOI(__int64, X87S_MUL(colour[1], CGraphics::m_65536)), brightness);
            colour[2] = FixMulShift32((int)FTOI(__int64, X87S_MUL(colour[2], CGraphics::m_65536)), brightness);
            Sprite_Queue((SpriteRect *)&CFrontend::m_matrixTextures[FRONTEND_LG_MATRIX]->field_0x11c, (SpriteRect *)g_frontendDrawRect,
                         CFrontend::m_matrixTextures[FRONTEND_LG_MATRIX], 1, 0, NULL, NULL, colour, 8);
            pCell += 0x12;
        }
    }
}

// GLOBAL: CMR2 0x00818848
BYTE g_recordsProfileMode;

// FUNCTION: CMR2 0x004d27d0
BYTE FrontendRecords_GetProfileMode(void)
{
    return g_recordsProfileMode;
}

// FUNCTION: CMR2 0x004d2790
void CFrontend::CacheCarClassTextLabels(void)
{
    CFrontend::m_unk0x0081853c = GetTextString(0xd0);
    g_unk0x00818540 = GetTextString(0xd1);
    g_unk0x00818544 = GetTextString(0xd2);
}

// Releases the text string table.
// Terminates the line at p and, when asked, returns the start of the next one.
// FUNCTION: CMR2 0x004a3d50
BYTE *Frontend_TerminateTextLine(BYTE *p, char next)
{
    while (*p >= 0x20)
        p++;
    *p = 0;
    if (next != 0) {
        do {
            p++;
        } while (*p < 0x20);
        return p;
    }
    return NULL;
}

// FUNCTION: CMR2 0x004a3d80
void CFrontend::FreeLocalizedTextStrings(void)
{
    if (m_textStrings != NULL) {
        CFileBuffer::FreeGenericFileBuffer(m_textStrings);
        m_textStrings = NULL;
        m_textLanguage = 0;
        m_textFirstId = 0;
        m_textLanguageCount = 0;
        m_textCount = 0;
    }
}

// GLOBAL: CMR2 0x00817404
int g_unk0x00817404;
// GLOBAL: CMR2 0x008173fc
int g_frontendRecordStage;

// FUNCTION: CMR2 0x004cfe50
unsigned int CFrontend::GetCurrentRecordOptionValue(void)
{
    GameInfo0xa4 *pInfo;
    int index;

    pInfo = CGameInfo::GetGameInfoFieldA4Address();
    index = g_unk0x00817404 * 3 + g_frontendRecordStage;
    return (pInfo->arcadeRecordTimes[index].value >> 7) & 0xffff;
}

// FUNCTION: CMR2 0x0040ede0
char *CFrontend::GetModeSpecificCountryText(int offset)
{
    if (CGame::GetFrontendResourceMode() == 0)
        return GetTextString(offset + 0x98);
    if (CGame::GetFrontendResourceMode() == 3)
        return GetTextString(offset + 0xc2);
    return NULL;
}

// FUNCTION: CMR2 0x0040ee20
char *CFrontend::GetModeSpecificStageText(int offset)
{
    if (CGame::GetFrontendResourceMode() == 0)
        return GetTextString(offset + 0xae);
    if (CGame::GetFrontendResourceMode() == 3)
        return GetTextString(offset + 0xd8);
    return NULL;
}

// One entry of the input state block (0x817420): two one-shot flags, the bit
// mask of the keys held and a counter of the frames each key has been held.
struct InputKeyState {
    BYTE field_0x0[2];
    BYTE field_0x2;
    BYTE field_0x3;
    short field_0x4;
    short field_0x6;
    short field_0x8;
};

// GLOBAL: CMR2 0x008173f0
int g_unk0x008173f0;
// GLOBAL: CMR2 0x008173f4
int g_unk0x008173f4;
// GLOBAL: CMR2 0x00817400
int g_frontendRecordDifficulty;
// Per-player input state, at most 4 players; the split-time arrays at
// 0x817448 start right after the 4th record.
// GLOBAL: CMR2 0x00817420
BYTE g_playerKeyStates[4 * 0xa];
// GLOBAL: CMR2 0x00817410
BYTE g_unk0x00817410;
// GLOBAL: CMR2 0x00817411
BYTE g_unk0x00817411;
// GLOBAL: CMR2 0x00817412
BYTE g_unk0x00817412;
// GLOBAL: CMR2 0x00817413
BYTE g_unk0x00817413;
// GLOBAL: CMR2 0x00817414
short g_unk0x00817414;
// GLOBAL: CMR2 0x00817416
short g_unk0x00817416;
// GLOBAL: CMR2 0x00817418
short g_unk0x00817418;

void Frontend_StoreCurrentCountryStageSelection(void);

// Reads the player/mode values of the game info block, clears the per-device
// input state for every device and resets the global key state.
// FUNCTION: CMR2 0x004cf060
void CFrontend::ResetFrontendPlayerInputState(void)
{
    InputKeyState *pState;
    int i;

    g_unk0x008173f0 = CGameInfo::GetConfiguredPlayerCount() & 0xff;
    g_unk0x008173f4 = CGameInfo::GetConfiguredGameMode() & 0xff;
    g_frontendRecordDifficulty = CGameInfo::GetConfiguredDifficulty() & 0xff;
    Frontend_StoreCurrentCountryStageSelection();
    pState = (InputKeyState *)g_playerKeyStates;
    for (i = 0; i < g_unk0x008173f0; i++) {
        pState->field_0x8 = 0;
        pState->field_0x4 = 0;
        pState->field_0x6 = 0;
        pState->field_0x0[0] = 0;
        pState->field_0x0[1] = 0;
        pState->field_0x2 = 0;
        pState->field_0x3 = 0;
        pState++;
    }
    g_unk0x00817418 = 0;
    g_unk0x00817414 = 0;
    g_unk0x00817416 = 0;
    g_unk0x00817410 = 0;
    g_unk0x00817411 = 0;
    g_unk0x00817412 = 0;
    g_unk0x00817413 = 0;
}

// FUNCTION: CMR2 0x004cf0f0
void CFrontend::ClearFrontendPlayerInputCounters(void)
{
    int i;
    BYTE index;

    for (i = 0; i < g_unk0x008173f0; i++) {
        g_playerKeyStates[i * 0xa] = 0;
        index = RallyDataStageIndex();
        if (index == 0)
            g_playerKeyStates[i * 0xa] = index;
    }
    g_unk0x00817410 = 0;
    index = RallyDataStageIndex();
    if (index == 0)
        g_unk0x00817410 = index;
}

void RallyData_IncrementCategoryCounter10(int index);

// Accumulates the pressed keys of every input device (and of the two global
// flags) into the per-key counters.
// FUNCTION: CMR2 0x004cf260
void Frontend_AccumulateDeviceKeyCounters(void)
{
    InputKeyState *pState;
    int i;
    int j;

    for (i = 0; i < 0xb; i++) {
        for (j = 0; j < g_unk0x008173f0; j++) {
            pState = (InputKeyState *)(g_playerKeyStates + j * 0xa);
            if ((1 << i) & pState->field_0x4) {
                pState->field_0x8++;
                RallyData_IncrementCategoryCounter10(j);
            }
        }
        if ((1 << i) & g_unk0x00817414) {
            g_unk0x00817418++;
            CGame::SetStartupFlag();
        }
    }
    for (i = 0; i < 8; i++) {
        for (j = 0; j < g_unk0x008173f0; j++) {
            pState = (InputKeyState *)(g_playerKeyStates + j * 0xa);
            if ((1 << i) & pState->field_0x6) {
                pState->field_0x8++;
                RallyData_IncrementCategoryCounter10(j);
            }
        }
        if ((1 << i) & g_unk0x00817416) {
            g_unk0x00817418++;
            CGame::SetStartupFlag();
        }
    }
    for (j = 0; j < g_unk0x008173f0; j++) {
        pState = (InputKeyState *)(g_playerKeyStates + j * 0xa);
        if (pState->field_0x2 != 0) {
            pState->field_0x8++;
            RallyData_IncrementCategoryCounter10(j);
        }
        if (pState->field_0x3 != 0) {
            pState->field_0x8++;
            RallyData_IncrementCategoryCounter10(j);
        }
    }
    if (g_unk0x00817412 != 0) {
        g_unk0x00817418++;
        CGame::SetStartupFlag();
    }
    if (g_unk0x00817413 != 0) {
        g_unk0x00817418++;
        CGame::SetStartupFlag();
    }
}

// GLOBAL: CMR2 0x008173f8
int g_frontendRecordCountry;

// Stores the country/stage selection of the current rally data record.
// FUNCTION: CMR2 0x004d0230
void Frontend_StoreCurrentCountryStageSelection(void)
{
    g_frontendRecordCountry = RallyDataCountryIndex() & 0xff;
    if ((BYTE)RallyData_GetFlag24() != 0) {
        g_frontendRecordStage = RallyData_GetSelectionBits12To13() & 0xff;
        g_unk0x00817404 = RallyData_GetSelectionBits10To11() & 0xff;
        return;
    }
    g_frontendRecordStage = RallyDataStageIndex() & 0xff;
}

// Indexed by id. A mix of small integers (car/stage index and 0/1 option
// flags) and pointers to the CFrontend car directory names, so it cannot be a
// uniform pointer array. Size derived from the 0x516c78 boundary (next known
// global), so it may cover further undeclared values.
struct Unk0x00516b40 {
    int ids[28];           // 0x00 car/stage index
    const char *dirs[22];  // 0x70 car directory name
    int flags[28];         // 0xc8 option flag
};
// GLOBAL: CMR2 0x00516b40
Unk0x00516b40 g_unk0x00516b40 = {
    { 0, 0, 0, 0, 1, 1, 1, 1, 2, 3, 4, 5, 6, 6, 6, 7, 8, 9, 10, 11, 12, 13, 0, 4, 8, 9, 10, 11 },
    {
        CFrontend::m_strFoc, CFrontend::m_strF99Short, CFrontend::m_strFA1, CFrontend::m_strFA2,
        CFrontend::m_strMit, CFrontend::m_strMA1, CFrontend::m_strMA2, CFrontend::m_strMA3,
        CFrontend::m_strCor, CFrontend::m_strSubShort, CFrontend::m_str206, CFrontend::m_strSea,
        CFrontend::m_strInt, CFrontend::m_strIA1, CFrontend::m_strIA2, CFrontend::m_strSie,
        CFrontend::m_strMin, CFrontend::m_str6R4, CFrontend::m_strStr, CFrontend::m_str205,
        CFrontend::m_strPum, CFrontend::m_strEsc,
    },
    { 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0 },
};

// FUNCTION: CMR2 0x0040ee60
void *CFrontend::GetArchiveDirectoryEntry(int index)
{
    return (void *)g_unk0x00516b40.dirs[index];
}

// FUNCTION: CMR2 0x0040ee70
int CFrontend::GetArchivePrimaryFlagEntry(int index)
{
    return g_unk0x00516b40.flags[index];
}

// FUNCTION: CMR2 0x0040ee80
int CFrontend::GetArchiveSecondaryFlagEntry(int index)
{
    return g_unk0x00516b40.flags[index + 14];
}

// FUNCTION: CMR2 0x0040ee90
int CFrontend::GetArchivePrimaryIDEntry(int index)
{
    return g_unk0x00516b40.ids[index];
}

// FUNCTION: CMR2 0x0040eea0
int CFrontend::GetArchiveSecondaryIDEntry(int index)
{
    return g_unk0x00516b40.ids[index + 22];
}

// FUNCTION: CMR2 0x004a3e20
void Frontend_SetObjectField118(Texture *pTexture, int value)
{
    if (pTexture != NULL)
        pTexture->blendMode = value;
}

// ---------------------------------------------------------------------------
// Frontend device setup: option words, per-player key state and split times.
// ---------------------------------------------------------------------------

// Split times mirrored out of the loaded game info record; 10 entries for the
// rally records, the arcade records only fill the first 6. The stage HUD reads
// twelve (Frontend_StoreSelectedDeviceOption); the last two are never written
// and read as zero, as the free space after the array does in the original.
// GLOBAL: CMR2 0x00817448
int g_loadedSplitTimes[12];
// Lowest split value seen so far, reset to a whole stage length.
// GLOBAL: CMR2 0x00817570
int g_lowestSplitValue;
// Placeholder string copied (and discarded) by FrontendRecords_CopyArcadeSplitMirror.
// GLOBAL: CMR2 0x00523bb4
char g_str0x00523bb4[8] = "ABCDEFG";

// Merges the frontend menu keys into the per-player key state: the first flag
// word of each state comes from the "keys" flag and the second from the
// "buttons" flag, each one shifted by the current stage index.
// FUNCTION: CMR2 0x004cf140
void Frontend_MergeMenuKeysIntoPlayerState(void)
{
    InputKeyState *pState;
    BYTE stage;
    int i;

    if ((BYTE)RallyData_GetFlag30()) {
        for (i = 0; i < g_unk0x008173f0; i++) {
            pState = (InputKeyState *)(g_playerKeyStates + i * 0xa);
            pState->field_0x4 |= (short)(pState->field_0x0[0] << RallyDataStageIndex());
        }
        g_unk0x00817414 |= (short)(g_unk0x00817410 << RallyDataStageIndex());
    }
    if ((BYTE)RallyData_GetFlag24()) {
        for (i = 0; i < g_unk0x008173f0; i++) {
            pState = (InputKeyState *)(g_playerKeyStates + i * 0xa);
            pState->field_0x4 |= (short)(StageTiming_GetFinishStateEntry(i) << RallyData_GetSelectionBits12To13());
        }
        g_unk0x00817414 |= (short)(StageTiming_GetFinishStateByte() << RallyData_GetSelectionBits12To13());
    }
    stage = RallyData_GetModeStageGroup(RallyDataCountryIndex(), (char)CGameInfo::GetConfiguredDifficulty());
    if (RallyDataStageIndex() == stage) {
        for (i = 0; i < g_unk0x008173f0; i++) {
            pState = (InputKeyState *)(g_playerKeyStates + i * 0xa);
            pState->field_0x6 |= (short)(pState->field_0x0[1] << RallyDataStageIndex());
        }
        g_unk0x00817416 |= (short)(g_unk0x00817411 << RallyDataStageIndex());
    }
}

// Clears the two key fields of a device option word.
// FUNCTION: CMR2 0x004cf390
void Frontend_ClearDeviceKeyFields(int index)
{
    unsigned int *pValue = (unsigned int *)Frontend_GetEventRallyResult(index);

    if (pValue != NULL)
        *pValue &= 0xffffc3ff;
}

// Advances the 4-bit option field of a device word; only for the first mode.
// FUNCTION: CMR2 0x004cf3b0
void Frontend_AdvanceDeviceOptionNibble(int index, int mode)
{
    unsigned int *pValue = (unsigned int *)Frontend_GetEventRallyResult(index);

    if (pValue != NULL && mode == 0)
        *pValue = (((*pValue & 0xfffffc00) + 0x400) ^ *pValue) & 0x3c00 ^ *pValue;
}

// Resets the value and the three stat counters of a device record.
// match 91%: the address increment and zero-register initialization are reversed.
// FUNCTION: CMR2 0x004cf3f0
void FrontendRecords_ResetEventPoints(int index)
{
    ProfileEventResults *pDevice = RallyData_GetDriverEventResults(index);

    if (pDevice != NULL) {
        pDevice->playerPoints = 0;
        memset(pDevice->playerPlacings, 0, 3);
    }
}

// Adds to the value field of a device record and bumps one of its three stats.
// FUNCTION: CMR2 0x004cf420
void Frontend_AddEventPointsAndPlacing(int index, int amount, int stat)
{
    ProfileEventResults *pDevice = RallyData_GetDriverEventResults(index);

    if (pDevice != NULL) {
        pDevice->playerPoints = pDevice->playerPoints + amount;
        if (stat < 3)
            pDevice->playerPlacings[stat]++;
    }
}

// Stores a value into the option block of a device (see Frontend_GetEventRallyStageTime).
// FUNCTION: CMR2 0x004cf450
void Frontend_SetEventRallyStageTime(int index, int arg, int value)
{
    int *pOption;

    Frontend_StoreCurrentCountryStageSelection();
    pOption = (int *)Frontend_GetEventRallyStageTime(index);
    if (pOption != NULL)
        *pOption = value;
}

// Writes a device option word: the option index in the low nibble, a 2-bit and
// a 4-bit field above it, and the value in the following dword.
struct DeviceBits4cf470 { unsigned a : 4; unsigned f : 4; unsigned b : 2; unsigned rest : 22; int value; };
// FUNCTION: CMR2 0x004cf470
void Frontend_SetEventRallyResult(int index, int value, unsigned int option, unsigned int field)
{
    DeviceBits4cf470 *p = (DeviceBits4cf470 *)Frontend_GetEventRallyResult(index);
    if (p != NULL) {
        p->a = option;
        p->f = field;
        p->value = value;
        p->b = option;
        if (p->b > 3)
            p->b = 0;
    }
}

// Writes the two 6-bit fields of a device record and clears its second dword.
struct DeviceBits4cf4d0 { unsigned a : 4; unsigned b : 2; unsigned f : 8; unsigned rest : 18; int extra; };
// FUNCTION: CMR2 0x004cf4d0
void Frontend_SetEventPlayerResult(int index, unsigned int value, unsigned int field)
{
    DeviceBits4cf4d0 *p = (DeviceBits4cf4d0 *)&RallyData_GetDriverEventResults(index)->playerResult;

    if (p != NULL) {
        p->f = field;
        p->a = value;
        p->b = value;
        p->extra = 0;
        if (p->b > 3)
            p->b = 0;
    }
}

// Stores a value into the extra dword of a device record.
// FUNCTION: CMR2 0x004cf530
void Frontend_SetEventArcadeStageTime(int index, int value)
{
    int *pField;

    Frontend_StoreCurrentCountryStageSelection();
    pField = (int *)&RallyData_GetDriverEventResults(index)->arcadeStageTime;
    if (pField != NULL)
        *pField = value;
}

// Writes the 6-bit, 2-bit and 3-bit fields of a device record and its extra
// dword.
struct DeviceBits4cf550 { unsigned a : 3; unsigned b : 2; unsigned f : 6; unsigned rest : 21; int extra; };
// FUNCTION: CMR2 0x004cf550
void Frontend_SetEventArcadeResult(int index, unsigned int value, unsigned int field, int extra)
{
    DeviceBits4cf550 *p = (DeviceBits4cf550 *)&RallyData_GetDriverEventResults(index)->arcadeResult;
    if (p != NULL) {
        p->extra = extra;
        p->f = field;
        p->a = value;
        p->b = value;
        if (p->b > 3)
            p->b = 0;
    }
}

// Rebuilds the option value of a device into the stage setup block when it is
// better than the stored one, and marks the player's key state as dirty.
// FUNCTION: CMR2 0x004cf5b0
BYTE Frontend_MergeBestRallyStageTime(int index, CategoryBestResults *pBlock)
{
    unsigned int *pOption;
    unsigned int *pEntry;
    unsigned int *pValue;
    unsigned int value;

    pOption = (unsigned int *)Frontend_GetEventRallyStageTime(index);
    pEntry = &pBlock->rallyStageTimes[g_frontendRecordCountry][g_frontendRecordStage].header;
    pValue = pEntry + 1;
    if (pOption != NULL && (*pOption < *pValue || (*pEntry & 0x80) == 0)) {
        *pEntry |= 0x80;
        value = rand();
        *pEntry = (value & 0x1f) << 8 | (*pEntry & 0xffffe0ff);
        Frontend_CopyOptionWord((int *)pValue, (int *)pOption);
        *pEntry = (RallyData_GetDriverRecordSelectionValue((BYTE)index) & 0x3f) | (*pEntry & 0xffffffc0);
        *pEntry = (RallyData_GetDriverAutomaticGearbox((BYTE)index) & 1) << 6 | (*pEntry & 0xffffffbf);
        RallyData_MarkDriverRecordDirty(index);
        g_playerKeyStates[index * 0xa] = 1;
        return 1;
    }
    return 0;
}

// Same as Frontend_MergeBestRallyStageTime for the button option word (uses the record option
// block of Frontend_GetEventRallyResult and the three-field merge of Frontend_MergeFourNibbleOptionFields).
// FUNCTION: CMR2 0x004cf660
BYTE Frontend_MergeBestRallyResult(int index, CategoryBestResults *pBlock)
{
    unsigned int *pOption;
    unsigned int *pEntry;
    unsigned int *pValue;
    unsigned int value;
    BOOL better;

    pOption = (unsigned int *)Frontend_GetEventRallyResult(index);
    better = FALSE;
    pEntry = &pBlock->rallyResults[g_frontendRecordCountry][g_frontendRecordDifficulty].header;
    pValue = pEntry + 1;
    if (pOption != NULL) {
        value = *pValue;
        if ((*pOption & 0xf) < (value & 0xf))
            better = TRUE;
        if ((((*pOption ^ value) & 0xf) == 0 && pOption[1] < pValue[1]) || better ||
            (*pEntry & 0x80) == 0) {
            *pEntry |= 0x80;
            value = rand();
            *pEntry = (value & 0x1f) << 8 | (*pEntry & 0xffffe0ff);
            Frontend_MergeFourNibbleOptionFields((int *)pValue, (int *)pOption);
            *pEntry = (RallyData_GetDriverRecordSelectionValue((BYTE)index) & 0x3f) | (*pEntry & 0xffffffc0);
            *pEntry = (RallyData_GetDriverAutomaticGearbox((BYTE)index) & 1) << 6 | (*pEntry & 0xffffffbf);
            RallyData_MarkDriverRecordDirty(index);
            g_playerKeyStates[index * 0xa + 1] = 1;
            return 1;
        }
    }
    return 0;
}

// Best-result records of the frontend: a header word (car, flags, a random
// tag) followed by the result, compared field by field.
#include "RecordBits.h"

struct RecordHeader {
    unsigned car : 6;
    unsigned automatic : 1;
    unsigned used : 1;
    unsigned tag : 5;
    unsigned rest : 19;
};

struct DeviceResult {
    unsigned level : 4;
    unsigned bits4 : 2;
    unsigned score : 8;
    unsigned rest : 18;
};

struct RecordResult {
    unsigned level : 3;
    unsigned bits3 : 2;
    unsigned score : 6;
    unsigned rest : 21;
};

struct RecordEntry {
    RecordHeader header;
    RecordResult result;
    int field_0x8;
};

// Same as Frontend_MergeBestRallyStageTime for the per-player word of the second dword, using the
// two 4-bit fields merge of Frontend_MergeFourTwoSixBitOptionFields.
// match 59%: register allocation of the device pointer and of the "better" flag
// differs (the original keeps more values on the stack).
// FUNCTION: CMR2 0x004cf740
BYTE Frontend_MergeBestPlayerResult(int index, CategoryBestResults *pBlock)
{
    DeviceResult *pDevice;
    RecordHeader *pEntry;
    DeviceResult *pValue;
    BOOL better;

    pDevice = (DeviceResult *)&RallyData_GetDriverEventResults(index)->playerResult;
    if (pDevice != NULL) {
        pEntry = (RecordHeader *)&pBlock->playerResults[g_frontendRecordDifficulty].result.header;
        pValue = (DeviceResult *)(pEntry + 1);
        better = FALSE;
        if (pDevice->level < pValue->level)
            better = TRUE;
        if ((pDevice->level == pValue->level && pDevice->score > pValue->score) || better || !pEntry->used) {
            pEntry->used = 1;
            pEntry->tag = rand();
            Frontend_MergeFourTwoSixBitOptionFields((int *)pValue, (int *)pDevice);
            pEntry->car = RallyData_GetDriverRecordSelectionValue((BYTE)index);
            pEntry->automatic = RallyData_GetDriverAutomaticGearbox((BYTE)index);
            RallyData_MarkDriverRecordDirty(index);
            g_playerKeyStates[index * 0xa + 2] = 1;
            return 1;
        }
    }
    return 0;
}

// Rebuilds the arcade-record option value of a device from the category record
// (pointer at +0x20) into the arcade block.
// FUNCTION: CMR2 0x004cf830
BYTE Frontend_MergeBestArcadeStageTime(int index)
{
    CategoryBestResults *base;
    unsigned int *pDevice;
    unsigned int *pEntry;
    unsigned int *pValue;
    unsigned int value;

    base = RallyData_GetDriverBestResults(index);
    pDevice = &RallyData_GetDriverEventResults(index)->arcadeStageTime;
    pEntry = &base->arcadeStageTimes[g_unk0x00817404][g_frontendRecordStage].header;
    pValue = pEntry + 1;
    if (pDevice != NULL && (*pDevice < *pValue || (*pEntry & 0x80) == 0)) {
        *pEntry |= 0x80;
        value = rand();
        *pEntry = (value & 0x1f) << 8 | (*pEntry & 0xffffe0ff);
        Frontend_CopyOptionWord((int *)pValue, (int *)pDevice);
        *pEntry = (RallyData_GetDriverRecordSelectionValue((BYTE)index) & 0x3f) | (*pEntry & 0xffffffc0);
        *pEntry = (RallyData_GetDriverAutomaticGearbox((BYTE)index) & 1) << 6 | (*pEntry & 0xffffffbf);
        RallyData_MarkDriverRecordDirty(index);
        return 1;
    }
    return 0;
}

// Same as Frontend_MergeBestPlayerResult for the 3-bit/6-bit option word at +0x18, using the
// three 3-bit fields merge of Frontend_MergeThreeTwoSixBitOptionFields.
// FUNCTION: CMR2 0x004cf8e0
BYTE Frontend_MergeBestArcadeResult(int index, CategoryBestResults *pBlock)
{
    RecordResult *pValue;
    RecordEntry *pEntry;
    BOOL better;

    pValue = (RecordResult *)&RallyData_GetDriverEventResults(index)->arcadeResult;
    pEntry = (RecordEntry *)&pBlock->arcadeResults[g_unk0x00817404][g_frontendRecordDifficulty];
    better = FALSE;
    if (pValue != NULL) {
        if (pValue->level < pEntry->result.level)
            better = TRUE;
        if ((pValue->level == pEntry->result.level && pValue->score > pEntry->result.score) || better ||
            !pEntry->header.used) {
            pEntry->header.used = 1;
            pEntry->header.tag = rand();
            Frontend_MergeThreeTwoSixBitOptionFields((int *)&pEntry->result, (int *)pValue);
            pEntry->header.car = RallyData_GetDriverRecordSelectionValue((BYTE)index);
            pEntry->header.automatic = RallyData_GetDriverAutomaticGearbox((BYTE)index);
            RallyData_MarkDriverRecordDirty(index);
            g_playerKeyStates[index * 0xa + 3] = 1;
            return 1;
        }
    }
    return 0;
}

// Keeps the lowest option value of a device in the running minimum.
// FUNCTION: CMR2 0x004cf9d0
int Frontend_AccumulateMinimumRallyStageTime(int param_1, int param_2)
{
    unsigned int *pOption;

    RallyData_GetDriverEventResults(param_2);
    pOption = (unsigned int *)Frontend_GetEventRallyStageTime(param_2);
    if (pOption != NULL && *pOption < (unsigned int)g_lowestSplitValue) {
        g_lowestSplitValue = *pOption;
        return 1;
    }
    return 0;
}

// Copies a stage record name and its split times into the mirror array; only
// when the option value beats the stored one.
// match 70%: register allocation and the stack frame differ (the original uses
// push ecx where we allocate two slots); the code is the same.
// FUNCTION: CMR2 0x004cfa10
int Frontend_CopyImprovedStageRecordAndSplits(int param_1, int param_2, char *pName)
{
    GameInfo0xa4 *pInfo;
    GameInfo0xa4SubStruct8 *pRecord;
    short *pSplits;
    unsigned int *pOption;
    int index;
    int i;

    RallyData_GetDriverEventResults(param_2);
    pOption = (unsigned int *)Frontend_GetEventRallyStageTime(param_2);
    pInfo = CGameInfo::GetGameInfoFieldA4Address();
    index = g_frontendRecordStage + g_frontendRecordCountry * 0xb;
    pSplits = pInfo->rallyStageRecordSplits[index];
    pRecord = &pInfo->rallyStageRecordTimes[index];
    if (pOption == NULL || *pOption >= pRecord->bits.time)
        return 0;
    strcpy(pRecord->ident, pName);
    pRecord->bits.car = RallyData_GetDriverRecordSelectionValue((BYTE)param_2);
    pRecord->bits.automatic = RallyData_GetDriverAutomaticGearbox((BYTE)param_2);
    pRecord->bits.time = *pOption;
    for (i = 0; i < 10; i++) {
        unsigned short value = (unsigned short)StageTiming_GetDriverSplitClock(param_1, i);
        pSplits[i] = value;
        g_loadedSplitTimes[i] = value;
    }
    g_unk0x00817410 = 1;
    return 1;
}

// Inserts a device name and its option word into the five-entry record list of
// the current stage group, keeping the list sorted by the option value.
// match 61%: the original spills `better` and the record base to the stack;
// MSVC keeps them in registers here, so the code differs only in allocation.
// match 60%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004cfb30
int FrontendRecords_InsertStageDeviceRecord(int param1, int index, char *pName)
{
    unsigned char *pInfo;
    unsigned int *pDevice;
    unsigned int *pWords;
    GameInfo0xa4SubStruct12 *pRecord;
    int better;
    int slot;
    int i;

    RallyData_GetDriverEventResults(index);
    pDevice = (unsigned int *)Frontend_GetEventRallyResult(index);
    pInfo = (unsigned char *)CGameInfo::GetGameInfoFieldA4Address();
    slot = 0;
    pInfo += (g_frontendRecordDifficulty + g_frontendRecordCountry * 3) * 0x3c;
    better = 0;
    pWords = (unsigned int *)(pInfo + 0xb8);
    for (;;) {
        pRecord = (GameInfo0xa4SubStruct12 *)(pWords - 1);
        if (pDevice != NULL) {
            if ((*pDevice & 0xf) < pRecord->bits.level)
                better = 1;
            if ((pRecord->bits.level == (*pDevice & 0xf) && pDevice[1] < pRecord->value) || better) {
                if (slot < 4) {
                    GameInfo0xa4SubStruct12 *p = (GameInfo0xa4SubStruct12 *)(pInfo + 0xe4);

                    // The original computes the source once: after the first
                    // move every call copies the same entry onto itself.
                    GameInfo0xa4SubStruct12 *pPrev = p - 1;

                    i = 4 - slot;
                    do {
                        Frontend_MergeNamedRecordFourBitFields((int *)p, (int *)pPrev);
                        p = pPrev;
                    } while (--i);
                }
                strcpy(pRecord->ident, pName);
                pRecord->bits.car = RallyData_GetDriverRecordSelectionValue((BYTE)index);
                pRecord->bits.automatic = RallyData_GetDriverAutomaticGearbox((BYTE)index);
                pRecord->bits.level = *pDevice;
                pRecord->value = pDevice[1];
                pRecord->bits.extra = (*pDevice & 0x3c00) >> 10;
                return (slot != 0) + 1;
            }
        }
        slot++;
        if (slot >= 5)
            return 0;
        pWords += 3;
    }
}


// Inserts a device name and its option word into the five-entry record list of
// the current car group, keeping the list sorted by the option value.

// FUNCTION: CMR2 0x004cfc90
int FrontendRecords_InsertStageCategoryRecord(int param1, int index, char *pName)
{
    ProfileEventResults *pDevice;
    GameInfo0xa4SubStruct12 *pRecords;
    int better;
    int slot;
    GameInfo0xa4SubStruct12 *pRecord;
    int k;
    int i;

    pDevice = RallyData_GetDriverEventResults(index);
    pRecords = (GameInfo0xa4SubStruct12 *)CGameInfo::GetGameInfoFieldA4Address();
    better = 0;
    slot = 0;
    pRecords = (GameInfo0xa4SubStruct12 *)((BYTE *)pRecords + g_frontendRecordDifficulty * 0x3c);
    for (;;) {
        pRecord = &pRecords[slot];
        if (pDevice != NULL) {
            if ((pDevice->playerResult & 0xf) < LEVEL_BITS(pRecord).level)
                better = 1;
            if ((LEVEL_BITS(pRecord).level == (pDevice->playerResult & 0xf) &&
                 (pDevice->playerResult & 0x3fc0) < ((pRecord->flags >> 1) & 0x1fc0)) || better) {
                for (i = 4; i > slot; i--)
                    Frontend_MergeNamedRecordSevenBitFields((int *)&pRecords[i], (int *)&pRecords[i - 1]);
                strcpy(pRecord->ident, pName);
                LEVEL_BITS(pRecord).car = RallyData_GetDriverRecordSelectionValue((BYTE)index);
                LEVEL_BITS(pRecord).automatic = RallyData_GetDriverAutomaticGearbox((BYTE)index);
                LEVEL_BITS(pRecord).level = pDevice->playerResult;
                LEVEL_BITS(pRecord).score = pDevice->playerResult >> 6;
                for (k = 0; k < 3; k++)
                    ((char *)&pRecord->value)[k] = pDevice->playerPlacings[k];
                g_unk0x00817412 = 1;
                return (slot != 0) + 1;
            }
        }
        slot++;
        if (slot >= 5)
            return 0;
    }
}

// Keeps the lowest arcade-record option value in the running minimum.
// FUNCTION: CMR2 0x004cfe20
int Frontend_AccumulateMinimumArcadeStageTime(int param_1, int param_2)
{
    unsigned int *pDevice;
    unsigned int value;

    pDevice = &RallyData_GetDriverEventResults(param_2)->arcadeStageTime;
    if (pDevice != NULL) {
        value = *pDevice;
        if (value < (unsigned int)g_lowestSplitValue) {
            g_lowestSplitValue = value;
            return 1;
        }
    }
    return 0;
}

// Copies an arcade record name and its (6) split times into the mirror array.
// match 76%: register allocation and the stack frame differ; the code is the
// same.
// match 76%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004cfe80
int FrontendRecords_CopyArcadeSplitMirror(int param_1, int param_2)
{
    GameInfo0xa4 *pInfo;
    GameInfo0xa4SubStruct8 *pRecord;
    short *pSplits;
    ProfileEventResults *pOption;
    unsigned int *pTime;
    char placeholder[16];
    int stage;
    int group;
    int i;

    pOption = RallyData_GetDriverEventResults(param_2);
    pInfo = CGameInfo::GetGameInfoFieldA4Address();
    stage = g_frontendRecordStage;
    group = g_unk0x00817404;
    StageTiming_GetCheckpointField2(param_1);
    strcpy(placeholder, g_str0x00523bb4);
    pSplits = pInfo->arcadeRecordSplits[stage + group * 3];
    pTime = &pOption->arcadeStageTime;
    pRecord = &pInfo->arcadeRecordTimes[stage + group * 3];
    if (pTime != NULL) {
        unsigned int value = *pTime;

        if (value < ((pRecord->value >> 7) & 0xffff)) {
            pRecord->value = (value & 0xffff) << 7 | (pRecord->value & 0xff80007f);
            for (i = 0; i < 6; i++) {
                unsigned short split = (unsigned short)StageTiming_GetCarSplitMarker(param_1, i);
                pSplits[i] = split;
                g_loadedSplitTimes[i] = split;
            }
            value = (pRecord->value >> 7) & 0xffff;
            i = Stage_GetSplitPositionCount();
            g_loadedSplitTimes[i] = value;
            strcpy(pRecord->ident, (char *)RallyData_GetRecord((BYTE)param_2));
            pRecord->value = (RallyData_GetDriverRecordSelectionValue((BYTE)param_2) & 0x3f) | (pRecord->value & 0xffffffc0);
            pRecord->value = (RallyData_GetDriverAutomaticGearbox((BYTE)param_2) & 1) << 6 | (pRecord->value & 0xffffffbf);
            return 1;
        }
    }
    return 0;
}

// Inserts a device name and its option word into the five-entry record list of
// the current arcade group, keeping the list sorted by the option value.
// match 57%: same code as the original; MSVC allocates the loop counter, the
// base and `better` to different places.
// match 56%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004cfff0
char FrontendRecords_InsertArcadeDeviceRecord(int param1, int index, char *pName)
{
    unsigned char *pInfo;
    unsigned int *pWords;
    RecordResult *pDevice;
    GameInfo0xa4SubStruct12 *pRecord;
    int better;
    int slot;
    int i;

    pDevice = (RecordResult *)&RallyData_GetDriverEventResults(index)->arcadeResult;
    pInfo = (unsigned char *)CGameInfo::GetGameInfoFieldA4Address();
    better = 0;
    slot = 0;
    pInfo += (g_frontendRecordDifficulty + g_unk0x00817404 * 3) * 0x3c;
    pWords = (unsigned int *)(pInfo + 0xff8);
    for (;;) {
        pRecord = (GameInfo0xa4SubStruct12 *)(pWords - 1);
        if (pDevice != NULL) {
            if (pDevice->level < SCORE_BITS(pRecord).level)
                better = 1;
            if ((SCORE_BITS(pRecord).level == pDevice->level && pDevice->score < SCORE_BITS(pRecord).score) || better) {
                if (slot < 4) {
                    GameInfo0xa4SubStruct12 *pMove = (GameInfo0xa4SubStruct12 *)(pInfo + 0x1024);

                    // The original computes the source once: after the first
                    // move every call copies the same entry onto itself.
                    GameInfo0xa4SubStruct12 *pPrev = pMove - 1;

                    for (i = 4 - slot; i != 0; i--) {
                        Frontend_MergeNamedRecordSixBitFields((int *)pMove, (int *)pPrev);
                        pMove = pPrev;
                    }
                }
                strcpy(pRecord->ident, pName);
                SCORE_BITS(pRecord).car = RallyData_GetDriverRecordSelectionValue((BYTE)index);
                SCORE_BITS(pRecord).automatic = RallyData_GetDriverAutomaticGearbox((BYTE)index);
                SCORE_BITS(pRecord).level = pDevice->level;
                SCORE_BITS(pRecord).score = pDevice->score;
                pRecord->value = ((unsigned int *)pDevice)[1];
                g_unk0x00817413 = 1;
                return (slot != 0) + 1;
            }
        }
        slot++;
        if (slot >= 5)
            return 0;
        pWords += 3;
    }
}


// Loads the current set of split times (rally or arcade) into the mirror array
// and resets the running minimum to a whole stage length.
// FUNCTION: CMR2 0x004d0180
void Frontend_LoadSplitTimeMirror(void)
{
    GameInfo0xa4 *pInfo;
    int *pDest;
    int index;
    int i;

    pInfo = CGameInfo::GetGameInfoFieldA4Address();
    if ((BYTE)RallyData_GetFlag23()) {
        Frontend_StoreCurrentCountryStageSelection();
        i = 0;
        pDest = g_loadedSplitTimes;
        do {
            if ((BYTE)RallyData_GetFlag24()) {
                index = g_frontendRecordStage + g_unk0x00817404 * 3;
                *pDest = ((unsigned short *)pInfo->arcadeRecordSplits[index])[i];
            } else {
                index = g_frontendRecordStage + g_frontendRecordCountry * 0xb;
                *pDest = ((unsigned short *)pInfo->rallyStageRecordSplits[index])[i];
            }
            pDest++;
            i++;
        } while ((INT_PTR)pDest < (INT_PTR)&g_loadedSplitTimes[10]);
        RallyData_GetFlag24();
        g_lowestSplitValue = 360000;
        return;
    }
    g_lowestSplitValue = 360000;
}

// Stores a value into the option block of the given device (see Frontend_GetEventRallyStageTime).
// FUNCTION: CMR2 0x004d0220
int Frontend_StoreSelectedDeviceOption(int index)
{
    return g_loadedSplitTimes[index];
}

// Returns the option block of a device when the current menu mode has one
// (five option words per device), or NULL.
// FUNCTION: CMR2 0x004d0280
BYTE *Frontend_GetEventRallyStageTime(int index)
{
    ProfileEventResults *pDevice = RallyData_GetDriverEventResults(index);

    switch (g_unk0x008173f4) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 8:
    case 9:
    case 10:
        return (BYTE *)&pDevice->rallyStageTime;
    }
    return NULL;
}

// Returns the setup word of a device in the single-player and the first
// multi-player modes.
// FUNCTION: CMR2 0x004d02d0
BYTE *Frontend_GetEventRallyResult(int index)
{
    ProfileEventResults *pDevice = RallyData_GetDriverEventResults(index);

    if (g_unk0x008173f4 >= 0 && (g_unk0x008173f4 <= 1 || g_unk0x008173f4 == 8))
        return (BYTE *)pDevice->rallyResult;
    return NULL;
}

// Copies one 32-bit option word.
// FUNCTION: CMR2 0x004d0300
void Frontend_CopyOptionWord(int *pDest, int *pSource)
{
    *pDest = *pSource;
}

// Merges the four 4-bit fields of a source word into a destination word.
// FUNCTION: CMR2 0x004d0310
void Frontend_MergeFourNibbleOptionFields(int *pDest, int *pSource)
{
    pDest[1] = pSource[1];
    *pDest = (((*pDest ^ *pSource) & 0xf) ^ *pDest);
    *pDest = (((*pDest ^ *pSource) & 0xf0) ^ *pDest);
    *pDest = (((*pDest ^ *pSource) & 0x300) ^ *pDest);
    *pDest = (((*pDest ^ *pSource) & 0x3c00) ^ *pDest);
}

// Merges the 4-bit, 2-bit and 6-bit fields of a source word into a
// destination word.
// FUNCTION: CMR2 0x004d0370
void Frontend_MergeFourTwoSixBitOptionFields(int *pDest, int *pSource)
{
    pDest[1] = pSource[1];
    *pDest = (((*pDest ^ *pSource) & 0xf) ^ *pDest);
    *pDest = (((*pDest ^ *pSource) & 0x30) ^ *pDest);
    *pDest = (((*pDest ^ *pSource) & 0x3fc0) ^ *pDest);
}

// Merges the 3-bit, 2-bit and 6-bit fields of a source word into a
// destination word.
// FUNCTION: CMR2 0x004d03b0
void Frontend_MergeThreeTwoSixBitOptionFields(int *pDest, int *pSource)
{
    pDest[1] = pSource[1];
    *pDest = (((*pDest ^ *pSource) & 7) ^ *pDest);
    *pDest = (((*pDest ^ *pSource) & 0x18) ^ *pDest);
    *pDest = (((*pDest ^ *pSource) & 0x7e0) ^ *pDest);
}

// Copies a record name and merges the 6-bit, 1-bit, 4-bit and 4-bit fields of
// the source word into the destination word.
// FUNCTION: CMR2 0x004d03f0
void Frontend_MergeNamedRecordFourBitFields(int *pDest, int *pSource)
{
    strcpy((char *)pDest, (char *)pSource);
    pDest[1] = (((pDest[1] ^ pSource[1]) & 0x3f) ^ pDest[1]);
    pDest[1] = (((pDest[1] ^ pSource[1]) & 0x40) ^ pDest[1]);
    pDest[1] = (((pDest[1] ^ pSource[1]) & 0x780) ^ pDest[1]);
    pDest[2] = pSource[2];
    pDest[1] = (((pDest[1] ^ pSource[1]) & 0x7800) ^ pDest[1]);
}

// Copies a record name and merges the 6-bit, 1-bit, 7-bit and 2-bit fields of
// the source word into the destination word, plus three trailing bytes.
// FUNCTION: CMR2 0x004d0470
void Frontend_MergeNamedRecordSevenBitFields(int *pDest, int *pSource)
{
    char *pDestBytes = (char *)pDest;
    char *pSourceBytes = (char *)pSource;
    int i;

    strcpy(pDestBytes, pSourceBytes);
    pDest[1] = (((pDest[1] ^ pSource[1]) & 0x3f) ^ pDest[1]);
    pDest[1] = (((pDest[1] ^ pSource[1]) & 0x40) ^ pDest[1]);
    pDest[1] = (((pDest[1] ^ pSource[1]) & 0x3f80) ^ pDest[1]);
    pDest[1] = (((pDest[1] ^ pSource[1]) & 0x3c000) ^ pDest[1]);
    for (i = 0; i < 3; i++)
        pDestBytes[8 + i] = pSourceBytes[8 + i];
}

// Copies a record name and merges the 6-bit, 1-bit, 6-bit and 3-bit fields of
// the source word into the destination word.
// FUNCTION: CMR2 0x004d0500
void Frontend_MergeNamedRecordSixBitFields(int *pDest, int *pSource)
{
    strcpy((char *)pDest, (char *)pSource);
    pDest[1] = (((pDest[1] ^ pSource[1]) & 0x3f) ^ pDest[1]);
    pDest[1] = (((pDest[1] ^ pSource[1]) & 0x40) ^ pDest[1]);
    pDest[1] = (((pDest[1] ^ pSource[1]) & 0xfc00) ^ pDest[1]);
    pDest[1] = (((pDest[1] ^ pSource[1]) & 0x380) ^ pDest[1]);
    pDest[2] = pSource[2];
}

// cross-range: 0x4d0770 belongs to the Game.cpp range but is only used by
// Frontend_SetDebugOverlayChannels here; it returns the callback machine state block.
// FUNCTION: CMR2 0x004d0770
CallbackStateMachine *Frontend_GetOverlayCallbackState(void)
{
    return &CGame::m_frontendCallbackMachine;
}

// Enables/disables the debug overlay channels and pushes the new flags into
// the grouped callback machine.
// FUNCTION: CMR2 0x004d2070
void Frontend_SetDebugOverlayChannels(BYTE param1, BYTE param2, BYTE param3)
{
    CallbackStateMachine *p;

    p = (CallbackStateMachine *)Frontend_GetOverlayCallbackState();
    CGame::m_unk0x00523d68 = param1;
    CGame::m_unk0x008180f9 = param2;
    g_unk0x008180fa = param3;
    if (param1 == 0 && param2 == 0 && param3 == 0)
        CGame::PromoteCallbackEntryByRule(p, 0, 7, 2);
    else
        CGame::PromoteCallbackEntryByRule(p, 0, 0, 2);
}
