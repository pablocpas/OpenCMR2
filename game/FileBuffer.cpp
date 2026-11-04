#include "SavedGame.h"
#include "port/sys.h"
#include "port/disk.h"
#include "ChampionshipDriver.h"
#include "FileBuffer.h"
#include "GenericFileLoader.h"
#include "Game.h"
#include "Graphics.h"
#include "InstallInfo.h"
#include "main.h"
#include "BFL.h"

#include <stdio.h>
#include "zlib/zlib.h"

char CFileBuffer::m_unk0x00520f1c[4] = "rb"; // gzopen mode
int CFileBuffer::m_unk0x0066461c;
int CFileBuffer::m_unk0x00664620;

// FUNCTION: CMR2 0x004aad70
// PORT: GlobalAlloc(GHND) + GlobalLock: zero-filled heap memory.
void *CFileBuffer::AllocateLockedBuffer(size_t iSize)
{
    return calloc(1, iSize);
}

// FUNCTION: CMR2 0x004aad90
// PORT: GlobalReAlloc of the block (the grown part is not cleared).
void *CFileBuffer::ReallocateLockedBuffer(void *buffer, size_t iSize)
{
    if (buffer != NULL)
        return realloc(buffer, iSize);
    return AllocateLockedBuffer(iSize);
}

// match 76%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004aa220
// PORT: files are found through Sys (game data or user directory, any case).
// A missing install file asks to retry, as the original asked for the CD;
// the original's retry recursion is gone.
void *CFileBuffer::GetGenericFileBuffer(char *fileName, BOOL isLocalFile)
{
    // isLocalFile is basically is it on the HDD? this function is called by loadnetworkleaderboards,loadcontrollerconfig,etc. with param2 as 1
    void *unk0x004bdee0, *lpBuffer;
    BFLHeader pFileHeaderOut;
    DWORD fileSize, fileSizeRead;
    size_t iHeaderSize;
    bool bIsBFL;
    SysFile *file;
    char _fileName[MAX_PATH];
    char hostPath[1024];

    lpBuffer = NULL;
    bIsBFL = FALSE;
    snprintf(_fileName, sizeof(_fileName), "%s", fileName);

    unk0x004bdee0 = NULL;
    if (Sys_ResolvePath(_fileName, FALSE, hostPath, sizeof(hostPath)))
        unk0x004bdee0 = gzopen(hostPath, m_unk0x00520f1c);
    if (!unk0x004bdee0)
    {
        if (isLocalFile || !g_pGraphics)
            return NULL;

        // an install file: ask until it can be opened
        do
        {
            if (CInstallInfo::ShowNoCDErrorMessage() && Sys_ResolvePath(_fileName, FALSE, hostPath, sizeof(hostPath)))
                unk0x004bdee0 = gzopen(hostPath, m_unk0x00520f1c);
        } while (!unk0x004bdee0);
    }

    // check if its a BFL
    iHeaderSize = gzread(unk0x004bdee0, &pFileHeaderOut, 8);
    if (iHeaderSize == 8U && pFileHeaderOut.ident[0] == 0x43 && pFileHeaderOut.ident[1] == 0x4d && pFileHeaderOut.ident[2] == 0x50 && pFileHeaderOut.ident[3] == 0x52)
    {
        if (pFileHeaderOut.archiveSize == 0xffffffff)
            return NULL;

        lpBuffer = AllocateLockedBuffer(pFileHeaderOut.archiveSize);
        CGenericFileLoader::m_fileSize = pFileHeaderOut.archiveSize;
        gzread(unk0x004bdee0, lpBuffer, pFileHeaderOut.archiveSize);
        bIsBFL = TRUE;
    }

    gzclose(unk0x004bdee0);

    if (!bIsBFL)
    {
        file = Sys_OpenFile(_fileName, SYS_FILE_READ);
        if (file == NULL)
            return NULL;

        fileSize = Sys_GetFileSize(file);
        CGenericFileLoader::m_fileSize = fileSize;

        if (fileSize == 0xffffffff) {
            Sys_CloseFile(file);
            return NULL;
        }

        lpBuffer = AllocateLockedBuffer(fileSize);
        fileSizeRead = Sys_ReadFile(file, lpBuffer, fileSize);
        Sys_CloseFile(file);
        if (fileSizeRead != fileSize)
            return NULL;
    }

    return lpBuffer;
}

// FUNCTION: CMR2 0x004aade0
// PORT: GlobalUnlock + GlobalFree. Native copies of the file records in the
// buffer (port/disk.h) go with it.
void CFileBuffer::FreeGenericFileBuffer(void *param1)
{
    Disk_ReleaseBuffer(param1);
    free(param1);
}

// GLOBAL: CMR2 0x00531650
int g_profileFileCount;
// One value per player, cleared by RallyData_ResetSavedPlayerRecords
// GLOBAL: CMR2 0x00531654
int g_unk0x00531654[4];
// GLOBAL: CMR2 0x00531764
DriverProfileIdentifier *g_pProfileIdentifiers;
// CMR2 0x00818cd0: generic file load flag, cleared together with the load counter
// GLOBAL: CMR2 0x00818cd0
BYTE g_genericFileLoaded;

// GLOBAL: CMR2 0x00525258
char g_strPpsPathFormat[32] = "%s\\pps\\%s%.2d%.2d%.2d%.2d.pps";
// GLOBAL: CMR2 0x00818acc
char g_ppsPath[260];


#define SLOT(i) (g_saveSlots[i])

// Builds the path of a player profile save: <hd>\pps\<name><date>.pps.
// The packed dword after the 4-byte name holds the date fields.
// FUNCTION: CMR2 0x004eb2e0
char *Profile_BuildSavePath(char *pName)
{
    unsigned int packed = *(unsigned int *)(pName + 4);

    // the original passes one value more than the format uses
    sprintf(g_ppsPath, g_strPpsPathFormat, CInstallInfo::GetGameHDPath(), pName,
            packed >> 0x10 & 0x1f, packed >> 8 & 0xf, packed & 0xff, packed >> 0xc & 0xf,
            packed >> 0x16 & 0x3f);
    return g_ppsPath;
}

// Writes a 0x650-byte player profile to its .pps file.
// FUNCTION: CMR2 0x004eb340
BYTE Profile_WriteSaveFile(int unused, BYTE *pProfile)
{
    char *path = Profile_BuildSavePath((char *)pProfile + 0x10);

    CInstallInfo::WriteFileToDisk(path, 0, pProfile, 0x650);
    return 1;
}

// FUNCTION: CMR2 0x004eb450
BYTE *Profile_GetFileIdentifier(int index)
{
    return (BYTE *)&g_pProfileIdentifiers[index];
}

// Saves the profiles whose name has just been edited, clearing the dirty flag
// of the ones written; returns whether all of them were saved. Profiles with
// bit 0x200000 set are left alone.
// FUNCTION: CMR2 0x004eb3e0
bool Profile_SaveEditedNames(void)
{
    bool saved = true;
    int i;

    for (i = 0; i < 4; i++) {
        if ((g_saveProfiles[i].identifier.flags & 0x200000) == 0 &&
            g_saveProfiles[i].identifier.name[0] != 0 && g_saveDirty[i] != 0) {
            BYTE result = Profile_WriteSaveFile(0, (BYTE *)&g_saveProfiles[i]);
            if (result != 0)
                g_saveDirty[i] = 0;
            if (saved && result != 0)
                saved = true;
            else
                saved = false;
        }
    }
    return saved;
}

// FUNCTION: CMR2 0x004eb440
int Profile_GetFileCount(void)
{
    return g_profileFileCount;
}

// Saves every player profile marked dirty (unless it has flag 0x200000).
// FUNCTION: CMR2 0x004eb470
void Profile_SaveDirtyRecords(void)
{
    BYTE *pProfile;
    int i;

    for (i = 0; i < 4; i++) {
        if ((g_saveProfiles[i].identifier.flags & 0x200000) == 0 && g_saveDirty[i] != 0) {
            Profile_WriteSaveFile(0, (BYTE *)&g_saveProfiles[i]);
            g_saveDirty[i] = 0;
        }
    }
}

// FUNCTION: CMR2 0x004eb4b0
void *Profile_ReadLocalFile(char *param1, int param2)
{
    return CFileBuffer::GetGenericFileBuffer(param1, 1);
}

void RallyData_ValidateIndex(INT_PTR index);
void Profile_AssignAvailableCategory(int index, int profile);
void Profile_ResetCategoryData(int index);

// Loads the .pps profile named by slot `param_2` and, when its name and id
// dword match one of the four stored player records, copies the record over
// that profile and links the record to it; otherwise the record is moved to
// the free category 0xf. Returns 1, or 0 when the file cannot be loaded.
// FUNCTION: CMR2 0x004eb4c0
BYTE Profile_LoadAndLinkSavedRecord(int param_1, int param_2)
{
    DriverProfile *pBuffer;
    unsigned int diff;
    int i;

    RallyData_ValidateIndex(param_1);
    pBuffer = (DriverProfile *)Profile_ReadLocalFile((char *)Profile_BuildSavePath((char *)&g_pProfileIdentifiers[param_2]), 0);
    if (pBuffer != NULL) {
        pBuffer->options &= 0xffff807f;
        for (i = 0; i < 4; i++) {
            if (strcmp(g_saveProfiles[i].identifier.name, pBuffer->identifier.name) == 0) {
                diff = pBuffer->identifier.flags ^ g_saveProfiles[i].identifier.flags;
                if ((diff & 0x1f0f00) == 0 && (char)diff == 0 && (diff & 0xfc0f000) == 0)
                    goto matched;
            }
        }
notfound:
        SLOT(param_1).category = 0xf;
        Profile_AssignAvailableCategory(param_1, -1);
        Profile_ResetCategoryData(param_1);
found:
        memcpy(&g_saveProfiles[SLOT(param_1).category], pBuffer, sizeof(DriverProfile));
        g_saveProfiles[SLOT(param_1).category].identifier.counters[2] = 0;
        g_saveProfiles[SLOT(param_1).category].identifier.counters[1] = 0;
        g_saveProfiles[SLOT(param_1).category].identifier.counters[0] = 0;
        SLOT(param_1).car = g_saveProfiles[SLOT(param_1).category].options & 0x1f;
        SLOT(param_1).bit25 = (g_saveProfiles[SLOT(param_1).category].options & 0x40) >> 6;
        SLOT(param_1).seconds = 0;
        SLOT(param_1).bit13 = 1;
        SLOT(param_1).time = 0;
        SLOT(param_1).bits22 = 0;
        CFileBuffer::FreeGenericFileBuffer(pBuffer);
        return 1;
matched:
        // the search loop's result: a match at a valid index, else the not-found path
        if (i == 0xffffffff)
            goto notfound;
        SLOT(param_1).category = i;
        goto found;
    }
    return 0;
}

// Resets record `index` of the 0x531350 table to category 0xf, clearing bit 0x2000.
// FUNCTION: CMR2 0x004ebe80
void Profile_ResetRecordCategory(int index)
{
    unsigned int value = *(unsigned int *)&g_saveSlots[index];
    unsigned int *pRecord = (unsigned int *)&g_saveSlots[index];
    if ((value & 0x3c0000) != 0x3c0000)
        *pRecord = value & 0xffffdfff | 0x3c0000;
}

// FUNCTION: CMR2 0x004ebec0
void Profile_ResetAllRecordCategories(void)
{
    int i;

    i = 0;
    do {
        Profile_ResetRecordCategory(i);
        i++;
    } while (i < 0x10);
}

void RallyData_ValidateIndex(INT_PTR index);
void RallyData_SetPlayerDefaultCarSetup(int player);

// Gives record `index` a profile (category) unless it has one: `profile`
// if given; otherwise the first free profile no earlier record uses; else
// the free profile with the lowest age, or when they all have the same age
// the one unused for longest.
// match 51%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004eb860
void Profile_AssignAvailableCategory(int index, int profile)
{
    unsigned int *pRecord;
    unsigned int flags;
    unsigned int age;
    unsigned int minAge;
    unsigned int firstAge;
    unsigned int best;
    unsigned int now;
    int free[4];
    int count;
    int chosen;
    int c;
    int i;
    SaveSlot *pRec;
    char allSame;

    RallyData_ValidateIndex(index);
    pRecord = (unsigned int *)&g_saveSlots[index];
    if ((*pRecord & 0x3c0000) != 0x3c0000)
        return;
    if (profile != -1) {
        flags = g_saveProfiles[profile].options;
        *pRecord = ((flags & 0x40) << 1 | profile & 0xf) << 0x12 | *pRecord & 0xfdc3ffc0 | flags & 0x1f;
        return;
    }
    for (c = 0; c < 4; c++) {
        if (g_saveProfiles[c].identifier.name[0] == 0 || (g_saveProfiles[c].identifier.flags & 0x200000))
            break;
    }
    if (c < 4 && c != -1) {
        for (i = 0; i < index; i++) {
            if ((g_saveSlots[i].category) == (unsigned int)c)
                c = -1;
        }
        if (c != -1) {
            chosen = c;
            goto assign;
        }
    }
    count = 0;
    for (c = 0; c < 4; c++) {
        free[count] = -1;
        for (pRec = g_saveSlots; pRec < g_saveSlots + 16; pRec++) {
            if (pRec->category == (unsigned int)c)
                break;
        }
        if (pRec >= g_saveSlots + 16) {
            free[count] = c;
            count++;
        }
    }
    minAge = 0xff;
    allSame = 1;
    firstAge = g_saveProfiles[free[0]].options >> 7 & 0xff;
    chosen = index;
    for (i = 0; i < count; i++) {
        age = g_saveProfiles[free[i]].options >> 7 & 0xff;
        if (firstAge != age)
            allSame = 0;
        if (minAge >= age) {
            minAge = age;
            chosen = free[i];
        }
    }
    now = CMain::GetFrameDelta();
    if (allSame) {
        best = 0;
        for (i = 0; i < count; i++) {
            if (best <= now - g_unk0x00531654[free[i]]) {
                best = now - g_unk0x00531654[free[i]];
                chosen = free[i];
            }
        }
    }
assign:
    *pRecord = (*pRecord & 0xffc3ffff) | (chosen & 0xf) << 0x12;
    g_unk0x00531654[*pRecord >> 0x12 & 0xf] = CMain::GetFrameDelta();
}

// Clears the profile of record `index`'s category back to a new profile.
// FUNCTION: CMR2 0x004ebf20
void Profile_ResetCategoryData(int index)
{
    unsigned int category;

    RallyData_ValidateIndex(index);
    category = g_saveSlots[index].category;
    memset(&g_saveProfiles[category], 0, sizeof(DriverProfile));
    g_saveProfiles[category].car = 4;
    g_saveProfiles[category].identifier.flags =
        (g_saveProfiles[category].identifier.flags & 0xffe1f17e) | 0x1017e;
    g_saveProfiles[category].options |= 0x20;
    g_saveProfiles[category].awards = 0xf11;
    g_saveProfiles[category].hudFlags = (g_saveProfiles[category].hudFlags & 0xfc) | 0x3c;
    RallyData_SetPlayerDefaultCarSetup(index);
    g_saveDirty[(g_saveSlots[index].category)] = 0;
}

// Reads the profile file and copies its identifier (offset 0x10) into *pOut.
// FUNCTION: CMR2 0x004ebee0
BYTE Profile_ReadFileIdentifier(DriverProfileIdentifier *pOut, char *param2)
{
    void *pBuffer;

    pBuffer = CFileBuffer::GetGenericFileBuffer(param2, 1);
    if (pBuffer != NULL) {
        *pOut = ((DriverProfile *)pBuffer)->identifier;
        CFileBuffer::FreeGenericFileBuffer(pBuffer);
        return TRUE;
    }
    return FALSE;
}


// Releases the current generic file buffer and resets the load state.
// FUNCTION: CMR2 0x004eb6d0
char Profile_ReleaseLoadBuffer(void)
{
    if (g_pProfileIdentifiers != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_pProfileIdentifiers);
        g_pProfileIdentifiers = NULL;
    }
    g_profileFileCount = 0;
    g_genericFileLoaded = 0;
    return 1;
}

// GLOBAL: CMR2 0x00525278
char g_strPpsPattern[8] = "*.pps";
// GLOBAL: CMR2 0x00525280
char g_strPpsDirFormat[8] = "%s\\pps\\";

// Rebuilds the player-profile file list: enters <hd>\pps, scans *.pps and
// appends one 12-byte date block per file (the buffer grows by 12 bytes each time).
// FUNCTION: CMR2 0x004eb700
// PORT: Sys_ListFiles instead of SetCurrentDirectory + FindFirstFile; each
// profile is read by its full game path.
static int Profile_AddListedFile(const char *name, void *pDir)
{
    char path[520];
    char found;

    sprintf(path, "%s%s", (const char *)pDir, name);
    g_pProfileIdentifiers = (DriverProfileIdentifier *)CFileBuffer::ReallocateLockedBuffer(
        g_pProfileIdentifiers, (g_profileFileCount * 3 + 3) * 4);
    found = Profile_ReadFileIdentifier(&g_pProfileIdentifiers[g_profileFileCount], path);
    if (found != 0)
        g_profileFileCount++;
    return 1;
}

void Profile_RebuildFileList(void)
{
    char ppsDir[260];

    if (g_genericFileLoaded == 0) {
        g_genericFileLoaded = 1;
        CGame::RegisterCallback((void *)Profile_ReleaseLoadBuffer, NULL);
    }
    if (g_pProfileIdentifiers != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_pProfileIdentifiers);
        g_pProfileIdentifiers = NULL;
    }
    g_profileFileCount = 0;
    sprintf(ppsDir, g_strPpsDirFormat, CInstallInfo::GetGameHDPath());
    Sys_ListFiles(ppsDir, g_strPpsPattern, Profile_AddListedFile, ppsDir);
}
