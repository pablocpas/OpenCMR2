#ifndef _FRONTEND_H
#define _FRONTEND_H

#include "GenericFileLoader.h"
#include "Texture.h"

// TODO: no idea if this is the right thing to call this
// Entries of CFrontend::m_matrixTextures: the trophies are at their place.
enum {
    FRONTEND_LG_MATRIX = 0,
    FRONTEND_TROPHY_BRONZE = 1,
    FRONTEND_TROPHY_SILVER = 2,
    FRONTEND_TROPHY_GOLD = 3
};

class CFrontend
{
public:
    static void LoadFrontendResourceArchives(void);
    static char *GetTextString(int index);
    static void SaveFrontendCallbackDepth(void);
    static void CacheCarClassTextLabels(void);
    static void FreeLocalizedTextStrings(void);
    static unsigned int GetCurrentRecordOptionValue(void);
    static char *GetModeSpecificCountryText(int offset);
    static char *GetModeSpecificStageText(int offset);
    static void *GetArchiveDirectoryEntry(int index);
    static int GetArchivePrimaryFlagEntry(int index);
    static int GetArchiveSecondaryFlagEntry(int index);
    static int GetArchivePrimaryIDEntry(int index);
    static int GetArchiveSecondaryIDEntry(int index);
    static void ResetFrontendPlayerInputState(void);
    static void ClearFrontendPlayerInputCounters(void);
    static DWORD GetDeviceCapabilityFieldA8(void);
    static unsigned int GetFrontendElapsedMilliseconds(void);
    static int GetFrontendTimestamp(void);
    static BYTE GetFrontendIntroFlag(void);
    static BOOL IsTextureWidthSupported(unsigned int param_1);
    static BOOL IsTextureHeightSupported(unsigned int param_1);
    static BOOL HasDXT1TextureSupport(void);
    static void LoadFrontendCarPreviewTextures(void);
    static bool LoadSplashScreens(bool param1);
    static void LoadLanguageFiles(void);
    static bool ReleaseLanguageFiles(void);
    static GenericFile* GetCopyrightFrontendArchive(void);
    static GenericFile* GetCommonFrontendArchive(void);
    static GenericFile* GetLocalizedFrontendArchive(int language);

    static void BuildLocalizedTextStringTable(int languages, int count, BYTE **pFiles);
    // GLOBAL: CMR2 0x0065aa6c
    static char **m_textStrings;
    // GLOBAL: CMR2 0x0065aa74
    static int m_textFirstId;
    // GLOBAL: CMR2 0x0065aa78
    static int m_textCount;
    // GLOBAL: CMR2 0x00520b00
    static char m_strInvalidTextString[20];
    // GLOBAL: CMR2 0x0065aa70
    static BYTE m_textLanguage;
    // GLOBAL: CMR2 0x0065aa71
    static BYTE m_textLanguageCount;

    // GLOBAL: CMR2 0x00818200
    static GenericFile m_languageFiles[5];
    // GLOBAL: CMR2 0x00818240
    static GenericFile m_commonFile;
    // GLOBAL: CMR2 0x0051a0f0
    static char m_strCommonBfl[14];
    // GLOBAL: CMR2 0x00523ec8
    static char m_strEnglishTextBfl[20];
    // GLOBAL: CMR2 0x00523ea0
    static char m_strFrenchTextBfl[19];
    // GLOBAL: CMR2 0x00523e78
    static char m_strGermanTextBfl[19];
    // GLOBAL: CMR2 0x00523e8c
    static char m_strSpanishTextBfl[20];
    // GLOBAL: CMR2 0x00523e64
    static char m_strItalianTextBfl[20];
    // GLOBAL: CMR2 0x00523edc
    static char m_strPolishTextBfl[19];
    // GLOBAL: CMR2 0x00523eb4
    static char m_strEngUSATextBfl[19];

    // GLOBAL: CMR2 0x00663b60
    static char m_stringDest[MAX_PATH];

    // GLOBAL: CMR2 0x00523dd0
    static char m_feRes640CountrySpecific[19];
    // GLOBAL: CMR2 0x00523de4
    static char m_feRes640CCountrySpecific[20];
    // GLOBAL: CMR2 0x00523df8
    static char m_feRes1024CountrySpecific[20];
    // GLOBAL: CMR2 0x00523e0c
    static char m_feRes1024CCountrySpecific[21];
    // GLOBAL: CMR2 0x00523e24
    static char m_feRes640[14];
    // GLOBAL: CMR2 0x00523e34
    static char m_feRes640C[15];
    // GLOBAL: CMR2 0x00523e44
    static char m_feRes1024[15];
    // GLOBAL: CMR2 0x00523e54
    static char m_feRes1024C[16];

    // unknowns
    // GLOBAL: CMR2 0x00818260;
    static GenericFile m_unk0x00818260;

    // GLOBAL: CMR2 0x00525034
    static char m_strFrontendTexturesAr640ATGA[36];

    // GLOBAL: CMR2 0x00525010
    static char m_strFrontendTexturesAr640DTGA[36];

    // GLOBAL: CMR2 0x00524fec
    static char m_strFrontendTexturesLgMatrixTGA[36];

    // GLOBAL: CMR2 0x00524fc8
    static char m_strFrontendTexturesSmMatrixTGA[36];
    
    // GLOBAL: CMR2 0x00818548
    static Texture* m_pAr640ATexture;

    // GLOBAL: CMR2 0x008185b8
    static Texture* m_pAr640DTexture;

    // The large matrix texture, then the trophies by place (m_matrixTextures
    // indices below). FrontendDraw_MatrixMedalRect picks a trophy with the
    // place it was won in, as an index into this table.
    // GLOBAL: CMR2 0x008182b8
    static Texture* m_matrixTextures[4];

    // GLOBAL: CMR2 0x00818324
    static Texture* m_pSmMatrixTexture;

    // GLOBAL: CMR2 0x00818328
    static Texture* m_pSetupRepBanners[8];

    // GLOBAL: CMR2 0x00818348
    static Texture* m_pTinyFlags[8];




    // GLOBAL: CMR2 0x00516c78
    static char m_strEsc[4];

    // GLOBAL: CMR2 0x00516c7c
    static char m_strPum[4];

    // GLOBAL: CMR2 0x00516c80
    static char m_str205[4];

    // GLOBAL: CMR2 0x00516c84
    static char m_strStr[4];

    // GLOBAL: CMR2 0x00516c88
    static char m_str6R4[4];

    // GLOBAL: CMR2 0x00516c8c
    static char m_strMin[4];

    // GLOBAL: CMR2 0x00516c90
    static char m_strSie[4];

    // GLOBAL: CMR2 0x00516c94
    static char m_strIA2[4];

    // GLOBAL: CMR2 0x00516c98
    static char m_strIA1[4];

    // GLOBAL: CMR2 0x00516c9c
    static char m_strInt[4];

    // GLOBAL: CMR2 0x00516ca0
    static char m_strSea[4];

    // GLOBAL: CMR2 0x00516ca4
    static char m_str206[4];

    // GLOBAL: CMR2 0x00516ca8
    static char m_strSubShort[4];

    // GLOBAL: CMR2 0x00516cac
    static char m_strCor[4];

    // GLOBAL: CMR2 0x00516cb0
    static char m_strMA3[4];

    // GLOBAL: CMR2 0x00516cb4
    static char m_strMA2[4];

    // GLOBAL: CMR2 0x00516cb8
    static char m_strMA1[4];

    // GLOBAL: CMR2 0x00516cbc
    static char m_strMit[4];

    // GLOBAL: CMR2 0x00516cc0
    static char m_strFA2[4];

    // GLOBAL: CMR2 0x00516cc4
    static char m_strFA1[4];

    // GLOBAL: CMR2 0x00516cc8
    static char m_strF99Short[4];

    // GLOBAL: CMR2 0x00516ccc
    static char m_strFoc[4];

    // GLOBAL: CMR2 0x00519264
    static char m_strUK[3];

    // GLOBAL: CMR2 0x00525058
    static char m_strIta[4];

    // GLOBAL: CMR2 0x0052505c
    static char m_strKen[4];

    // GLOBAL: CMR2 0x00525060
    static char m_strItaly[8];

    // GLOBAL: CMR2 0x00525068
    static char m_strAus[4];

    // GLOBAL: CMR2 0x0052506c
    static char m_strSwe[4];
    
    // GLOBAL: CMR2 0x00525070
    static char m_strKenya[8];
    
    // GLOBAL: CMR2 0x00525078
    static char m_strFra[4];

    // GLOBAL: CMR2 0x0052507c
    static char m_strSweden[8];
    
    // GLOBAL: CMR2 0x00525084
    static char m_strFrance[8];

    // GLOBAL: CMR2 0x0052508c
    static char m_strGre[4];

    // GLOBAL: CMR2 0x00525090
    static char m_strFin[4];

    // GLOBAL: CMR2 0x00525094
    static char m_strFinland[8];

    // GLOBAL: CMR2 0x0052509c
    static char m_strGreece[8];    
    
    // GLOBAL: CMR2 0x00524ba0
    static char m_strFrontendTexturesCarsLivery[40];

    // GLOBAL: CMR2 0x00524bc8
    static char m_strFrontendTexturesCarsB01[36];

    // GLOBAL: CMR2 0x00524bec
    static char m_strFrontendTexturesCarsF01[36];    

    // GLOBAL: CMR2 0x00524c10
    static char m_strEscort[7];

    // GLOBAL: CMR2 0x00524c18
    static char m_strPuma[7];
    
    // GLOBAL: CMR2 0x00524c20
    static char m_strStrts[7];

    // GLOBAL: CMR2 0x00524c28
    static char m_str6r4[4];

    // GLOBAL: CMR2 0x00524c2c
    static char m_strMini[7];

    // GLOBAL: CMR2 0x00524c34
    static char m_strSerra[7];

    // GLOBAL: CMR2 0x00524c3c
    static char m_strDelta[7];

    // GLOBAL: CMR2 0x00524c44
    static char m_strCrdba[7];

    // GLOBAL: CMR2 0x00524c4c
    static char m_strSub[4];

    // GLOBAL: CMR2 0x00524c50
    static char m_strCrlla[7];

    // GLOBAL: CMR2 0x00524c58
    static char m_strLncer[7];

    // GLOBAL: CMR2 0x00524c60
    static char m_strF99[4];

    // GLOBAL: CMR2 0x00524c64
    static char m_strF2000[7];

    // GLOBAL: CMR2 0x00524fa0
    static char m_strSetupRepTexturesBanners[40];

    // GLOBAL: CMR2 0x00524f78
    static char m_strFrontendTinyFlags[40];

    // GLOBAL: CMR2 0x00524f10
    static char m_strFrontendTexturesTBronzeTGA[36];    

    // GLOBAL: CMR2 0x00524f34
    static char m_strFrontendTexturesTSilverTGA[36];

    // GLOBAL: CMR2 0x00524f58
    static char m_strFrontendTexturesTGoldTGA[32];

    // GLOBAL: CMR2 0x00523c78
    static char m_strFrontEndBinkTGA[24];

    // GLOBAL: CMR2 0x00523c90
    static char m_strFrontEndTexturesCopyright[40];

    // GLOBAL: CMR2 0x00523cb8
    static char m_strFrontEndTexturesCmr2TGA[32];

    // GLOBAL: CMR2 0x00818530
    static Texture* m_unk0x00818530[3];
    
    // GLOBAL: CMR2 0x0081853c
    static char* m_unk0x0081853c;

    // GLOBAL: CMR2 0x008182cc
    static Texture* m_unk0x008182cc[22];

    // GLOBAL: CMR2 0x0081884c
    static Texture* m_unk0x0081884c[22];

    // Splash screen textures loaded by LoadSplashScreens
    // GLOBAL: CMR2 0x00817ebc
    static Texture* m_unk0x00817ebc;

    // GLOBAL: CMR2 0x00817fd0
    static Texture* m_unk0x00817fd0;

    // GLOBAL: CMR2 0x00817fd4
    static Texture* m_unk0x00817fd4[4];
};

#endif
