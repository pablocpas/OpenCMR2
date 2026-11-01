// Exercise the real .rcs writer, file enumeration and state restoration with
// synthetic assets. Byte checks use original disk offsets independently of
// the new structs, so mutually wrong reader/writer layouts cannot pass.
#include "port/types.h"
#include "SavedGame.h"
#include "GameInfo.h"
#include "InstallInfo.h"
#include "RallyData.h"
#include "RallyTiming.h"
#include "main.h"
#include "platform/platform.h"
#include <SDL3/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

extern void SavedGames_LoadRecords();
extern bool SavedGames_ReleaseRecords();
extern int SavedGames_GetCount();
extern void *SavedGames_GetFileName(int);
extern void GameInfo_SetConfiguredGameMode(BYTE);
extern void GameInfo_SetConfiguredDifficulty(BYTE);
extern void GameInfo_SetConfiguredPlayerCount(BYTE);
extern void GameInfo_SetConfiguredMultiplayer(BYTE);
extern void RallyData_ResetSavedPlayerRecords();
extern void Profile_AssignAvailableCategory(int, int);
extern void Profile_ResetCategoryData(int);
extern BYTE *RallyData_GetDriverCategoryProfile(int);
extern BYTE *RallyData_GetDriverEntryRecord(int);
struct RallyCarDamageState;
extern RallyCarDamageState *RallyData_GetDriverGroupRecord(int);
extern BYTE *RallyData_GetPlayerCarSetup(int);
extern int *RallyData_GetDriverSettingPair(int);
extern int *RallyData_GetDriverPrimaryPairRecord(int);
extern BYTE *RallyData_GetDriverSelectionFlagRecord(int);
extern int *RallyData_GetDriverPairRecord(int);
extern int *RallyData_GetDriverSecondaryPairRecord(int);
extern void RallyData_SetCountrySelectionBits(BYTE);
extern void RallyData_SetStageSelectionAndRefreshFlags(BYTE);
extern void RallyData_SetSelectionStateByte(char);
extern BYTE RallyData_GetSelectionStateByte();
extern void RallyTiming_SetStageTimeSeconds(int, int);

namespace fs = std::filesystem;
static void Check(bool ok, const char *message)
{
    if (!ok) { fprintf(stderr, "saved game: %s\n", message); exit(1); }
}
static void Write(const fs::path &path, const void *data, size_t size)
{
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file.write((const char *)data, size);
    Check(bool(file), "write fixture");
}
static unsigned Word(const std::vector<BYTE> &bytes, size_t offset)
{
    unsigned value;
    memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}

int main()
{
    const fs::path root = fs::temp_directory_path() / ("opencmr2-save-" + std::to_string(SDL_GetPerformanceCounter()));
    const fs::path data = root / "Data", user = root / "user";
    for (const char *path : {"Game/Bigfiles/common.bfl", "FrontEnd/common.bfl", "FrontEnd/Res640.bfl",
                             "FrontEnd/Res640C.bfl", "Sounds/surface.bfl",
                             "CountrySpecific/Europe/EnglishText.bfl", "SetupRep/640/common.bfl"})
        Write(data / path, "fixture", 7);
    const char ini[] = "[install]\nmenu_shortcut = 0\n";
    Write(user / "opencmr2.ini", ini, sizeof(ini) - 1);
    CMain::m_exitCode = 1;
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    std::string dataPath = data.u8string(), userPath = user.u8string();
    char program[] = "saved_game", dataFlag[] = "--data", userFlag[] = "--user-dir";
    char *args[] = {program, dataFlag, dataPath.data(), userFlag, userPath.data()};
    Check(Platform_Init(5, args), "initialise platform");
    Check(CInstallInfo::LoadInstallPathsFromRegistry() != 0, "initialise paths");

    for (int players = 1; players <= 4; players++) {
        GameInfo_SetConfiguredGameMode(1);
        GameInfo_SetConfiguredDifficulty(2);
        GameInfo_SetConfiguredPlayerCount(players);
        GameInfo_SetConfiguredMultiplayer(players > 1);
        RallyData_ResetSavedPlayerRecords();
        for (int i = 0; i < players; i++) {
            Profile_AssignAvailableCategory(i, -1);
            Profile_ResetCategoryData(i);
            DriverProfile &profile = *(DriverProfile *)RallyData_GetDriverCategoryProfile(i);
            profile.identifier.name[0] = 'A' + i;
            profile.identifier.flags = 0x123400 + i;
            memset(profile.identifier.counters, 0, sizeof(profile.identifier.counters));
            profile.setup = {1, 2, 3, 2, 5, 123};
            memset(RallyData_GetDriverGroupRecord(i), 0x21 + i, sizeof(SavedGameCarRecord));
            memset(RallyData_GetPlayerCarSetup(i), 0x41 + i, sizeof(CarSetup));
        }
        for (int i = 0; i < 16; i++) {
            SaveSlot &slot = *(SaveSlot *)RallyData_GetDriverEntryRecord(i);
            slot.car = i;
            RallyTiming_SetStageTimeSeconds(i, 10 + i);
            RallyTiming_SetOverallTimeRaw(i, 20000 + i * 100);
            g_rallyStageTables.tieBreak[i] = i + 1;
        }
        memset(RallyData_GetDriverSettingPair(0), 0x61, 0xa0);
        memset(RallyData_GetDriverPrimaryPairRecord(0), 0x62, 0x50);
        memset(RallyData_GetDriverSelectionFlagRecord(0), 0x63, 0x14);
        memset(RallyData_GetDriverPairRecord(0), 0x64, 0x50);
        memset(RallyData_GetDriverSecondaryPairRecord(0), 0x65, 0x50);
        RallyData_SetCountrySelectionBits(5);
        RallyData_SetStageSelectionAndRefreshFlags(3);
        RallyData_SetSelectionStateByte(2);
        char title[] = "save %n test";
        Check(SavedGames_WriteSessionImage(-1, title) == 1, "write actual session");
        char fileName[32];
        snprintf(fileName, sizeof(fileName), "game%05d.rcs", players - 1);
        std::ifstream file(user / "gamesave" / fileName, std::ios::binary);
        std::vector<BYTE> bytes((std::istreambuf_iterator<char>(file)), {});
        Check(bytes.size() == 0x7f4, "original disk size");
        Check(Word(bytes, 0x30) == unsigned(1 | (2 << 7) | (players << 10) | ((players > 1) << 14)), "original packed options");
        Check(!memcmp(bytes.data() + 0x10, title, sizeof(title)), "literal save name at original offset");
        Check(bytes[0xc4] == 'A' && bytes[0xf4] == 0x21 && bytes[0x614] == 0x41, "player records at original offsets");
        Check((Word(bytes, 0xb4) & 0x1fff) == unsigned(1 | (2 << 3) | (3 << 6) | (2 << 8) | (5 << 10)), "original packed setup");
        Check(Word(bytes, 0x7d8) == (5 | (3 << 5)) && bytes[0x7d4] == 2 && bytes[0x7dc] == 1, "original stage and tie break offsets");
        Check(bytes[0x630] == 0x61 && bytes[0x6d0] == 0x62 && bytes[0x720] == 0x63 && bytes[0x734] == 0x64 && bytes[0x784] == 0x65, "original table offsets");
        SavedGames_LoadRecords();
        Check(SavedGames_GetCount() == players, "enumerate multiple saved games");
        int index = -1;
        for (int i = 0; i < players; i++)
            if (!strcmp((char *)SavedGames_GetFileName(i), fileName)) index = i;
        Check(index >= 0, "find saved file");
        const SavedGameImage image = *SavedGames_GetRecord(index);
        Check(SavedGames_WriteSessionImage(index, nullptr) == 1, "overwrite selected save");
        // Destroy live data before restoring; a reader that skips a field
        // must not pass just because the original value is still in memory.
        GameInfo_SetConfiguredGameMode(0);
        GameInfo_SetConfiguredPlayerCount(1);
        RallyData_ResetSavedPlayerRecords();
        memset(RallyData_GetDriverSettingPair(0), 0, 0xa0);
        memset(RallyData_GetDriverPrimaryPairRecord(0), 0, 0x50);
        memset(RallyData_GetDriverSelectionFlagRecord(0), 0, 0x14);
        memset(RallyData_GetDriverPairRecord(0), 0, 0x50);
        memset(RallyData_GetDriverSecondaryPairRecord(0), 0, 0x50);
        Check(FrontendProfile_ApplyDriverProfileBlock(&image) == 1, "restore actual session");
        Check(CGameInfo::GetConfiguredGameMode() == 1 && CGameInfo::GetConfiguredPlayerCount() == players && CGameInfo::GetConfiguredDifficulty() == 2, "restore game options");
        const auto &progress = CGameInfo::m_gameInfo.rallyUnlockProgress;
        Check(progress.easyRallies >= 6 && progress.normalRallies >= 6 && progress.hardRallies >= 6, "restore rally unlock progress");
        for (int i = 0; i < players; i++) {
            const DriverProfile &profile = *(DriverProfile *)RallyData_GetDriverCategoryProfile(i);
            Check(RallyData_GetRecord(i) != nullptr && !strcmp((char *)RallyData_GetRecord(i), image.playerIdentifiers[i].name), "restore non-null player name used by garage");
            Check(profile.setup.value0 == 1 && profile.setup.value1 == 2 && profile.setup.value2 == 3 && profile.setup.value3 == 2 && profile.setup.value4 == 5, "restore setup");
            Check(!memcmp(RallyData_GetDriverGroupRecord(i), &image.playerCars[i], sizeof(image.playerCars[i])), "restore car record");
            Check(!memcmp(RallyData_GetPlayerCarSetup(i), &image.playerCarSetups[i], sizeof(image.playerCarSetups[i])), "restore controller record");
        }
        for (int i = 0; i < 16; i++) {
            const SaveSlot &slot = *(SaveSlot *)RallyData_GetDriverEntryRecord(i);
            Check(slot.car == image.drivers[i].car && slot.seconds == image.drivers[i].seconds && slot.time == image.drivers[i].time, "restore all driver slots");
            Check(g_rallyStageTables.tieBreak[i] == image.tieBreak[i], "restore tie break");
        }
        Check(RallyDataCountryIndex() == 5 && RallyDataStageIndex() == 3 && RallyData_GetSelectionStateByte() == 2, "restore stage selection");
        Check(!memcmp(RallyData_GetDriverSettingPair(0), image.settingPairs, sizeof(image.settingPairs)), "restore setting pairs");
        Check(!memcmp(RallyData_GetDriverPrimaryPairRecord(0), image.primaryValues, sizeof(image.primaryValues)), "restore primary values");
        Check(!memcmp(RallyData_GetDriverSelectionFlagRecord(0), image.selectionFlags, sizeof(image.selectionFlags)), "restore selection flags");
        Check(!memcmp(RallyData_GetDriverPairRecord(0), image.values, sizeof(image.values)), "restore values");
        Check(!memcmp(RallyData_GetDriverSecondaryPairRecord(0), image.secondaryValues, sizeof(image.secondaryValues)), "restore secondary values");
        SavedGameImage broken = image;
        broken.options.playerCount = 0;
        const unsigned state = CGameInfo::m_gameInfo.field_0x14;
        Check(!FrontendProfile_ApplyDriverProfileBlock(&broken) && CGameInfo::m_gameInfo.field_0x14 == state, "reject zero players without modifying live state");
        Check(!SavedGames_IsValidSessionImage(&image, sizeof(image) - 1), "reject truncated image");
        broken.options.playerCount = 5;
        Check(!FrontendProfile_ApplyDriverProfileBlock(&broken), "reject too many players");
        broken = image;
        broken.stage.stage = 11;
        Check(!FrontendProfile_ApplyDriverProfileBlock(&broken), "reject stage beyond the menu label table");
        broken = image;
        memset(broken.playerIdentifiers[0].name, 'X', sizeof(broken.playerIdentifiers[0].name));
        Check(!FrontendProfile_ApplyDriverProfileBlock(&broken), "reject unterminated player name");
        broken = image;
        broken.options.playerCount = 0;
        Write(user / "gamesave" / "broken.rcs", &broken, sizeof(broken));
        Write(user / "gamesave" / "short.rcs", &image, 8);
        SavedGames_LoadRecords();
        Check(SavedGames_GetCount() == players, "ignore damaged files in menu listing");
        Check(SavedGames_GetRecord(-1) == nullptr && SavedGames_GetRecord(players) == nullptr, "reject invalid selection");
    }
    SavedGames_ReleaseRecords();
    Platform_Shutdown();
    fs::remove_all(root);
    puts("saved game: original layout, 1-4 players, write/list/overwrite/load and damaged files passed");
}
