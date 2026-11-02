// Follow archive loading, displayed language labels and the actual selection
// callback with synthetic PC assets. No GPU or original game files needed.
#include "port/types.h"
#include "Frontend.h"
#include "GameInfo.h"
#include "Graphics.h"
#include "InstallInfo.h"
#include "Menu.h"
#include "main.h"
#include "platform/platform.h"

#include <SDL3/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

extern BYTE FrontendText_LoadRegionLanguages();
extern BYTE FrontendText_ReleaseRegionLanguages();
extern bool FrontendCredits_ReleaseText();
extern void FrontendMenu_BuildLanguage();
extern Menu *FrontendMenu_GetLanguage();
extern void FrontendMenu_LeaveLanguage(Menu *, char);
extern void Frontend_SelectTextLanguage(int);
extern void *g_creditsLines;

namespace fs = std::filesystem;

static void Check(bool ok, const char *message)
{
    if (!ok) {
        fprintf(stderr, "frontend language: %s\n", message);
        exit(1);
    }
}

static void Write(const fs::path &path, const std::string &bytes)
{
    fs::create_directories(path.parent_path());
    Check(SDL_SaveFile(path.u8string().c_str(), bytes.data(), bytes.size()), "write fixture");
}

static void Dword(std::string &bytes, unsigned value)
{
    for (unsigned i = 0; i < 4; ++i)
        bytes += char(value >> (i * 8));
}

static std::string Archive(const std::string &name, const std::string &text)
{
    std::string bytes = text;
    while (bytes.size() % 4)
        bytes += '\0';
    const unsigned index = unsigned(bytes.size());
    Dword(bytes, unsigned(text.size()));
    Dword(bytes, 0);
    Dword(bytes, unsigned(name.size()));
    bytes += name;
    while (bytes.size() % 4)
        bytes += '\0';
    Dword(bytes, index);
    std::string header = "CMPR";
    Dword(header, unsigned(bytes.size()));
    return header + bytes;
}

static void CreateRegion(const fs::path &data, int region, bool swapped = false)
{
    const char *names[] = {region == 0 ? "english" : "engusa", "french", "spanish", "italian", "german"};
    const char *labels[] = {"english", "fran\xe7" "ais", "espa\xf1ol", "italiano", "deutsch"};
    const int count = region == 0 ? 5 : 3;
    const fs::path country = data / "CountrySpecific" / (region == 0 ? "Europe" : "Usa");
    for (int language = 0; language < count; ++language) {
        std::string text;
        for (int line = 0; line < 0x215; ++line) {
            const char *label = names[language];
            if (line >= 6 && line <= 10) {
                // EU text files of 2000 label 8 deutsch and 10 español, which
                // differ from the archive/language numbers (2/4); other
                // releases' files (swapped) have them the other way round.
                const int order[] = {0, 1, region == 0 && !swapped ? 4 : 2, 3, region == 0 && swapped ? 4 : 2};
                label = labels[order[line - 6]];
            }
            text += std::string(label) + "\r\n";
        }
        Write(country / (std::string(names[language]) + "text.bfl"),
              Archive(std::string("f") + names[language] + ".txt", text));
        Write(country / (std::string("credits_") + (language == 0 && region == 1 ? "usa" :
              language == 1 && region == 1 ? "frenchusa" : language == 2 && region == 1 ? "spanishusa" : names[language]) + ".txt"),
              std::string("\"test\" \"") + names[language] + "\"\r\n");
    }
    Write(country / "FERes640E.bfl", "synthetic resources");
    Write(country / "FERes640EC.bfl", "synthetic resources");
}

static void TestRegion(int region)
{
    const char *names[] = {region == 0 ? "english" : "engusa", "french", "spanish", "italian", "german"};
    const char *labels[] = {"english", "fran\xe7" "ais", "espa\xf1ol", "italiano", "deutsch"};
    const int count = region == 0 ? 5 : 3;
    CGameInfo::SetGameRegion(region);
    CGameInfo::InitRegionLanguageCount();
    CFrontend::LoadLanguageFiles();
    Check(FrontendText_LoadRegionLanguages() != 0, "load regional frontend text");
    FrontendMenu_BuildLanguage();
    Menu *menu = FrontendMenu_GetLanguage();
    Check(menu->itemCount == count, "region has the right number of languages");
    Menu parent{};
    Menu_SetParent(menu, &parent); // existing options menu, no first-run controls setup
    for (int current = 0; current < count; ++current) {
        Frontend_SelectTextLanguage(current);
        for (int language = 0; language < count; ++language) {
            Check(std::string(CFrontend::GetTextString(menu->items[language].id)) == labels[language],
                  "displayed language label matches its archive number");
            Menu_SetCursor(menu, BYTE(language));
            FrontendMenu_LeaveLanguage(menu, 0);
            Check(CGameInfo::GetGameLanguage() == language, "selection stores the chosen language");
            Check(std::string(CFrontend::GetTextString(0)) == names[language], "selection displays the chosen archive's text");
            Check(g_creditsLines && std::string(((char **)g_creditsLines)[1]) == names[language],
                  "selection loads matching credits");
        }
    }
    Menu_SetCursor(menu, 0);
    FrontendMenu_LeaveLanguage(menu, 1);
    Check(CGameInfo::GetGameLanguage() == count - 1, "cancel preserves the current language");
    FrontendCredits_ReleaseText();
    FrontendText_ReleaseRegionLanguages();
    CFrontend::ReleaseLanguageFiles();
}

int main()
{
    const fs::path root = fs::temp_directory_path() / ("opencmr2-language-" + std::to_string(SDL_GetPerformanceCounter()));
    const fs::path data = root / "Data", user = root / "user";
    Write(data / "Game/Bigfiles/common.bfl", "synthetic archive");
    Write(data / "FrontEnd/common.bfl", "synthetic archive");
    Write(data / "FrontEnd/Res640.bfl", "synthetic resources");
    Write(data / "FrontEnd/Res640C.bfl", "synthetic resources");
    Write(data / "Sounds/surface.bfl", "synthetic archive");
    Write(data / "SetupRep/640/common.bfl", "synthetic archive");
    Write(user / "opencmr2.ini", "[install]\nmenu_shortcut = 0\n");
    CreateRegion(data, 0);
    CreateRegion(data, 1);
    // A missing fixture must fail even if the game's error dialog exits.
    CMain::m_exitCode = 1;
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    std::string dataPath = data.u8string(), userPath = user.u8string();
    char program[] = "frontend_language", dataFlag[] = "--data", userFlag[] = "--user-dir";
    char *args[] = {program, dataFlag, dataPath.data(), userFlag, userPath.data()};
    Check(Platform_Init(5, args), "initialise platform with synthetic assets");
    Check(CInstallInfo::LoadInstallPathsFromRegistry() != 0, "initialise install paths");
    Graphics graphics{};
    graphics.resX = 640;
    graphics.resY = 480;
    g_pGraphics = &graphics;
    TestRegion(0);
    TestRegion(1);
    CreateRegion(data, 0, true);
    TestRegion(0);
    g_pGraphics = nullptr;
    Platform_Shutdown();
    fs::remove_all(root);
    puts("frontend language: EU (both label orders)/USA labels, selected text, credits and cancellation passed");
}
