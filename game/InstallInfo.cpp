#include "port/sys.h"
#include "InstallInfo.h"
#include <string.h>
#include "main.h"
#include "Logger.h"
#include "RegKey.h"
#include <stdio.h>

char CInstallInfo::m_noCDBodyEnglish[43] = "Make sure the CMR2 CD is in the CD drive";
char CInstallInfo::m_noCDBodyGerman[72] = "Vergewissern Sie sich, da\xdf sich die CMR2-CD im CD-ROM-Laufwerk befindet";
char CInstallInfo::m_noCDBodyItalian[48] = "Assicurate che il CD di CMR2 sia nel lettore CD";
char CInstallInfo::m_noCDBodySpanish[60] = "Asegura que el CD de CMR2 est\xe1 en la disquetera de los CD";
char CInstallInfo::m_noCDBodyFrench[55] = "Assurez vous que le CD CMR2 soit dans le lecteur de CD";
char CInstallInfo::m_noCDBodyPolish[43] = "P_Make sure the CMR2 CD is in the CD drive";
char CInstallInfo::m_noCDBodyAmerican[44] = "US_Make sure the CMR2 CD is in the CD drive";

char CInstallInfo::m_noCDHeaderEnglish[21] = "CMR2 File Load Error";
char CInstallInfo::m_noCDHeaderGerman[33] = "Fehler beim Laden der Datei CMR2";
char CInstallInfo::m_noCDHeaderItalian[37] = "Errore nel caricamento del file CMR2";
char CInstallInfo::m_noCDHeaderSpanish[31] = "Error cargando archivo de CMR2";
char CInstallInfo::m_noCDHeaderFrench[31] = "Erreur chargement fichier CMR2";
char CInstallInfo::m_noCDHeaderPolish[23] = "P_CMR2 File Load Error";
char CInstallInfo::m_noCDHeaderAmerican[24] = "US_CMR2 File Load Error";

char *CInstallInfo::m_noCDMessages[7][2] = {
    {m_noCDBodyEnglish, m_noCDHeaderEnglish},
    {m_noCDBodyGerman, m_noCDHeaderGerman},
    {m_noCDBodyItalian, m_noCDHeaderItalian},
    {m_noCDBodySpanish, m_noCDHeaderSpanish},
    {m_noCDBodyFrench, m_noCDHeaderFrench},
    {m_noCDBodyPolish, m_noCDHeaderPolish},
    {m_noCDBodyAmerican, m_noCDHeaderAmerican}};

char CInstallInfo::m_hdPath[MAX_PATH];
BOOL CInstallInfo::m_isHDInstall = FALSE;

char CInstallInfo::m_cdPath[MAX_PATH];
BOOL CInstallInfo::m_isCDInstall = FALSE;

// asset directories
char CInstallInfo::m_tracksDir[MAX_PATH] = "";
char CInstallInfo::m_carsDir[MAX_PATH] = "";
char CInstallInfo::m_soundsDir[MAX_PATH] = "";
char CInstallInfo::m_savesDir[MAX_PATH] = "";
char CInstallInfo::m_musicDir[MAX_PATH] = "";
char CInstallInfo::m_videosDir[MAX_PATH] = "";
char CInstallInfo::m_frontendDir[MAX_PATH] = "";
char CInstallInfo::m_countrySpecificDir[MAX_PATH] = "";
char CInstallInfo::m_countrySpecificOtherDir[MAX_PATH] = "";
char CInstallInfo::m_setupRepDir[MAX_PATH] = "";
char CInstallInfo::m_bigFilesDir[MAX_PATH] = "";
char CInstallInfo::m_texturesDir[MAX_PATH] = "";
char CInstallInfo::m_fontsDir[MAX_PATH] = "";
char CInstallInfo::m_surprisedDir[3] = "";

char CInstallInfo::m_tracksSubDir[15] = "%s\\Game\\Tracks";
char CInstallInfo::m_carsSubDir[13] = "%s\\Game\\Cars";
char CInstallInfo::m_soundsSubDir[10] = "%s\\sounds";
char CInstallInfo::m_savesSubDir[13] = "%s\\CMR2Saves";
char CInstallInfo::m_musicSubDir[16] = "%s\\sounds\\music";
char CInstallInfo::m_videosSubDir[19] = "%s\\FrontEnd\\Videos";
char CInstallInfo::m_frontendSubDir[12] = "%s\\FrontEnd";
char CInstallInfo::m_countrySpecificSubDir[19] = "%s\\CountrySpecific";
char CInstallInfo::m_setupRepSubDir[12] = "%s\\SetupRep";

char CInstallInfo::m_bigFilesSubDir[17] = "%s\\Game\\BigFiles";
char CInstallInfo::m_texturesSubDir[11] = ".\\Textures";
char CInstallInfo::m_fontsSubDir[8] = ".\\Fonts";
char CInstallInfo::m_surprisedSubDir[3] = "O:";

// FUNCTION: CMR2 0x0040ed20
char *CInstallInfo::GetTracksDir(void)
{
    return m_tracksDir;
}

// FUNCTION: CMR2 0x0040ed30
char *CInstallInfo::GetSoundsDir(void)
{
    return m_soundsDir;
}

// FUNCTION: CMR2 0x0040ed40
char *CInstallInfo::GetCarsDir(void)
{
    return m_carsDir;
}

// FUNCTION: CMR2 0x0040ed70
char *CInstallInfo::GetMusicDir(void)
{
    return m_musicDir;
}

// FUNCTION: CMR2 0x0040ed80
char *CInstallInfo::GetVideosDir(void)
{
    return m_videosDir;
}

// FUNCTION: CMR2 0x0040edb0
char *CInstallInfo::GetSetupRepDir(void)
{
    return m_setupRepDir;
}

// FUNCTION: CMR2 0x0040edc0
char *CInstallInfo::GetBigFilesDir(void)
{
    return m_bigFilesDir;
}

// FUNCTION: CMR2 0x0040edd0
char *CInstallInfo::GetCountrySpecificOtherDir(void)
{
    return m_countrySpecificOtherDir;
}

// FUNCTION: CMR2 0x0040ed60
char *CInstallInfo::GetFontsDir(void)
{
    return m_fontsDir;
}

// FUNCTION: CMR2 0x0040ed90
char *CInstallInfo::GetFrontendDir(void)
{
    return m_frontendDir;
}

// FUNCTION: CMR2 0x0040eda0
char *CInstallInfo::GetCountrySpecificDir(void)
{
    return m_countrySpecificDir;
}

// FUNCTION: CMR2 0x004aa480
BOOL CInstallInfo::ShowNoCDErrorMessage(void)
{
    char *gameLanguage;
    int languageID;
    // PORT: never initialised in the original (undefined behaviour for an
    // unknown language); English here.
    int unknownLanguageID = 0;

    gameLanguage = CRegKey::GetValueFromKey(CRegKey::m_regKeyLanguage);
    languageID = _stricmp(gameLanguage, CRegKey::m_regKeyValueEnglish);
    if (languageID != 0)
    {
        if (_stricmp(gameLanguage, CRegKey::m_regKeyValueFrench) == 0)
            languageID = 4;
        else
        {
            if (_stricmp(gameLanguage, CRegKey::m_regKeyValueGerman) == 0)
                languageID = 1;
            else
            {
                if (_stricmp(gameLanguage, CRegKey::m_regKeyValueItalian) == 0)
                    languageID = 2;
                else
                {
                    if (_stricmp(gameLanguage, CRegKey::m_regKeyValueSpanish) == 0)
                        languageID = 3;
                    else
                    {
                        if (_stricmp(gameLanguage, CRegKey::m_regKeyValuePolish) == 0)
                            languageID = 5;
                        else
                        {
                            if (_stricmp(gameLanguage, CRegKey::m_regKeyValueAmerican) == 0)
                                languageID = 6;
                            else
                                languageID = unknownLanguageID;
                        }
                    }
                }
            }
        }
    }

    // PORT: there is no CD; the message means a game data file is missing,
    // so name it instead of asking for the CD.
    char text[MAX_PATH + 256];
    sprintf(text, "This game file could not be read:\n%.*s\n\n"
                  "Check that the game folder holds the complete installation.",
            MAX_PATH, Sys_GetLastMissingFile());
    int reply = Sys_MessageBox(m_noCDMessages[languageID][1], text, SYS_MESSAGEBOX_RETRY_CANCEL);
    if (reply == SYS_MESSAGEBOX_RESULT_CANCEL)
    {
        CMain::UnwindGameCallbacks();
        CLogger::CloseLogFile();
        Sys_Exit(CMain::m_exitCode);
    }
    else if (reply == SYS_MESSAGEBOX_RESULT_RETRY)
    {
        return TRUE;
    }
    return FALSE;
}

// FUNCTION: CMR2 0x0040e8d0
char CInstallInfo::LoadInstallPathsFromRegistry(void)
{
    char *regKeyValue;
    char hdPath[MAX_PATH], cdPath[MAX_PATH], installVersion[MAX_PATH];

    // if the game is on the HD
    regKeyValue = CRegKey::GetValueFromKey(CRegKey::m_rkv_gameHDPath);
    if (*regKeyValue == 0)
        return 0;

    // store the HD path
    strcpy(hdPath, regKeyValue);
    CInstallInfo::SetGameHDPath(hdPath);

    // if the game is on the CD
    regKeyValue = CRegKey::GetValueFromKey(CRegKey::m_rkv_gameCDPath);
    if (*regKeyValue == 0)
        return 0;

    // store the CD path
    strcpy(cdPath, regKeyValue);
    CInstallInfo::SetGameCDPath(cdPath);

    // game install version
    regKeyValue = CRegKey::GetValueFromKey(CRegKey::m_rkv_installVersion);
    if (*regKeyValue == 0)
        return 0;

    strcpy(installVersion, regKeyValue);

    // PORT: call variadic sprintf with its declared type. Casting it to a
    // fixed-argument function uses the wrong argument ABI on macOS ARM64.
    // if its the minimum version
    if (_stricmp(installVersion, CRegKey::m_rkv_installMinimum) == 0)
    {
        sprintf(m_tracksDir, m_tracksSubDir, cdPath);
        sprintf(m_carsDir, m_carsSubDir, cdPath);
        sprintf(m_soundsDir, m_soundsSubDir, cdPath);
        sprintf(m_savesDir, m_savesSubDir, hdPath);
        sprintf(m_musicDir, m_musicSubDir, cdPath);
        sprintf(m_videosDir, m_videosSubDir, cdPath);
        sprintf(m_frontendDir, m_frontendSubDir, cdPath);
        sprintf(m_countrySpecificDir, m_countrySpecificSubDir, cdPath);
        sprintf(m_countrySpecificOtherDir, m_countrySpecificSubDir, cdPath);
        sprintf(m_setupRepDir, m_setupRepSubDir, cdPath);
        sprintf(m_bigFilesDir, m_bigFilesSubDir, cdPath);
    }
    else
    {
        // medium
        if (_stricmp(installVersion, CRegKey::m_rkv_installMedium) == 0)
        {
            sprintf(m_tracksDir, m_tracksSubDir, hdPath);
            sprintf(m_carsDir, m_carsSubDir, hdPath);
            sprintf(m_soundsDir, m_soundsSubDir, hdPath);
            sprintf(m_savesDir, m_savesSubDir, hdPath);
            sprintf(m_musicDir, m_musicSubDir, cdPath);
            sprintf(m_videosDir, m_videosSubDir, cdPath);
            sprintf(m_frontendDir, m_frontendSubDir, hdPath);
            sprintf(m_countrySpecificDir, m_countrySpecificSubDir, hdPath);
            sprintf(m_countrySpecificOtherDir, m_countrySpecificSubDir, cdPath);
            sprintf(m_setupRepDir, m_setupRepSubDir, hdPath);
            sprintf(m_bigFilesDir, m_bigFilesSubDir, hdPath);
        }
        else
        {
            // full/network
            if (_stricmp(installVersion, CRegKey::m_rkv_installFull) == 0 || _stricmp(installVersion, CRegKey::m_rkv_installNetworkOnly) == 0)
            {
                sprintf(m_tracksDir, m_tracksSubDir, hdPath);
                sprintf(m_carsDir, m_carsSubDir, hdPath);
                sprintf(m_soundsDir, m_soundsSubDir, hdPath);
                sprintf(m_savesDir, m_savesSubDir, hdPath);
                sprintf(m_musicDir, m_musicSubDir, hdPath);
                sprintf(m_videosDir, m_videosSubDir, hdPath);
                sprintf(m_frontendDir, m_frontendSubDir, hdPath);
                sprintf(m_countrySpecificDir, m_countrySpecificSubDir, hdPath);
                sprintf(m_countrySpecificOtherDir, m_countrySpecificSubDir, hdPath);
                sprintf(m_setupRepDir, m_setupRepSubDir, hdPath);
                sprintf(m_bigFilesDir, m_bigFilesSubDir, hdPath);
            }
            else
                return 0;
        }
    }

    // install dependant locations
    strcpy(m_texturesDir, m_texturesSubDir);
    strcpy(m_fontsDir, m_fontsSubDir);
    strcpy(m_surprisedDir, m_surprisedSubDir);

    return 1;
}

// FUNCTION: CMR2 0x004aa6c0
void CInstallInfo::SetGameHDPath(char *filePath)
{
    strcpy(m_hdPath, filePath);
    m_isHDInstall = TRUE;
    return;
}

// FUNCTION: CMR2 0x004aa6e0
void CInstallInfo::SetGameCDPath(char *filePath)
{
    strcpy(m_cdPath, filePath);
    m_isCDInstall = TRUE;
    return;
}


// FUNCTION: CMR2 0x004aa700
char* CInstallInfo::GetGameHDPath(void) {
    return m_isHDInstall ? m_hdPath : NULL;
}

// FUNCTION: CMR2 0x004aa710
char* CInstallInfo::GetGameCDPath(void) {
    return m_isCDInstall ? m_cdPath : NULL;
}

// FUNCTION: CMR2 0x004aa5a0
// PORT: written through Sys into the user directory, replacing the file in one
// step: the original truncated it first, so a crash while saving left an
// empty profile, saved game or configuration.
int CInstallInfo::WriteFileToDisk(char *name, int mode, LPCVOID data, DWORD size)
{
    char path[MAX_PATH];

    if (mode == 1) {
        strcpy(path, m_hdPath);
        strcat(path, name);
    } else {
        strcpy(path, name);
    }

    if (Sys_SaveFile(path, data, size))
        return 1;
    if (Sys_FileExists(path))
        return 2;
    return 0;
}

// FUNCTION: CMR2 0x0040ed50
char *CInstallInfo::GetTexturesDirectory(void)
{
    return m_texturesDir;
}
