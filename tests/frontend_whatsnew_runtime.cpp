// Drives the real What's New screen (game/FrontendWhatsNew.cpp) through the
// real menu machine (game/Menu.cpp): build, manual open from the Options
// menu, enter, dismissal (OK and Back), seen persistence, no re-show, and a
// pixel dump of the real draw callback with block glyphs.
#include "port/types.h"
#include "Menu.h"
#include "Graphics.h"
#include "Frontend.h"
#include "FrontendDraw.h"
#include "FrontendMenus.h"
#include "Font.h"
#include "Input.h"
#include "Sound.h"
#include "WhatsNew.h"
#include "port/sys.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <vector>
#include <fstream>

// ---- stubs -------------------------------------------------------------
DWORD CInput::m_menuMoveSound = 0;
DWORD CInput::m_menuBackSound = 0;
DWORD CInput::m_menuErrorSound = 0;
DWORD CInput::m_menuSoundVolume = 0;
DWORD CInput::m_menuChangeSound = 0;
DWORD CInput::m_menuSelectSound = 0;
int Sound_PlaySampleWithParameters(unsigned short, int, int, int, int, int) { return 0; }

static Graphics g_graphics;
Graphics *g_pGraphics = &g_graphics;
BYTE g_colourWhite0x00524968[4] = {255, 255, 255, 255};
BYTE g_colourText0x0052496c[4] = {170, 170, 170, 255};
void FrontendDraw_Carousel(Menu *, char, char *) {}

char CFrontend::m_stringDest[MAX_PATH];
char *CFrontend::GetTextString(int) { static char ok[] = "OK"; return ok; }

static Menu g_testMainMenu;
Menu *FrontendMenu_GetMain(void) { return &g_testMainMenu; }
static Menu *g_activeMenu;
Menu *FrontendMenu_GetActiveMenu(void) { return g_activeMenu; }

static const int W = 640, H = 480;
static unsigned char fb[H][W][3];
struct DrawnLine { int font, x, y, white; char text[128]; };
static std::vector<DrawnLine> g_drawn;
void Font_DrawText(BYTE font, char *text, short x, short y, int *pColour, unsigned int)
{
    DrawnLine l{font, (int)x, (int)y, pColour == (int *)g_colourWhite0x00524968 ? 1 : 0, ""};
    std::strncpy(l.text, text, sizeof(l.text) - 1);
    g_drawn.push_back(l);
    // block glyph: 6x10 per character cell with 1px gaps, font 2 one third taller
    int cw = 6, ch = font == 2 ? 13 : 10;
    unsigned char shade = l.white ? 255 : 180;
    for (int c = 0; text[c]; c++)
        for (int gy = 0; gy < ch - 1; gy++)
            for (int gx = 0; gx < cw - 1; gx++) {
                int px = x + c * cw + gx, py = y + gy;
                if (px >= 0 && px < W && py >= 0 && py < H) { fb[py][px][0] = fb[py][px][1] = fb[py][px][2] = shade; }
            }
}

extern Menu *FrontendMenu_GetWhatsNew(void);
extern void FrontendMenu_BuildWhatsNew(void);
extern void FrontendMenu_OpenWhatsNew(Menu *, MenuItem *);
extern void FrontendMenu_DrawWhatsNew(Menu *);
extern Menu *g_whatsNewReturn;
char *FrontendMenu_WhatsNewText(void) { static char t[] = "What's new"; return t; }
static int g_quitCancelStub;
static void QuitCancelStub(Menu *, MenuItem *) { g_quitCancelStub = 1; }

#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
static int fails;

int main(int argc, char **argv)
{
    auto old = std::filesystem::current_path();
    auto tmp = std::filesystem::temp_directory_path() / "opencmr-whatsnew-runtime";
    std::filesystem::remove_all(tmp);
    std::filesystem::create_directories(tmp);
    std::filesystem::current_path(tmp);
    { FILE *f = std::fopen("opencmr2.ini", "w"); std::fputs("[video]\n", f); std::fclose(f); }
    g_graphics.resX = W; g_graphics.resY = H;

    Menu_Init(&g_testMainMenu, (char *)"main", 0, 0, NULL, NULL, 1, 0, 0);
    Menu_AddItemType4(&g_testMainMenu, 0, 0x4f, NULL, 0);
    Menu_ValidateCursor(&g_testMainMenu, 0);
    g_activeMenu = &g_testMainMenu;
    FrontendMenu_BuildWhatsNew();

    // No more main-menu popup: the screen opens only from its Options row.
    Menu *next = Menu_Update(&g_testMainMenu, 0);
    CHECK(next == NULL);
    FrontendMenu_OpenWhatsNew(NULL, NULL);
    next = Menu_Update(&g_testMainMenu, 0);
    CHECK(next == FrontendMenu_GetWhatsNew());
    CHECK(Sys_GetOption("ui.whatsnew_seen", -1) == -1);   // opened != acknowledged

    // The screen's first update fires its enter callback; parent is the main menu.
    Menu *screen = next;
    CHECK(Menu_Update(screen, 0) == NULL);
    CHECK(screen->pParent == &g_testMainMenu);
    CHECK(Sys_GetOption("ui.whatsnew_seen", -1) == -1);   // still not acknowledged on show

    // Pixel dump of the real draw: the Options row lists the whole log.
    std::memset(fb, 16, sizeof fb);
    g_drawn.clear();
    FrontendMenu_DrawWhatsNew(screen);
    if (argc > 1) {
        FILE *f = std::fopen(argv[1], "wb");
        if (f) { std::fprintf(f, "P6\n%d %d\n255\n", W, H); std::fwrite(fb, 1, sizeof fb, f); std::fclose(f); }
    }
    if (argc > 2) {   // drawn-line dump for the readable substitute-font preview
        FILE *f = std::fopen(argv[2], "w");
        if (f) { for (auto &l : g_drawn) std::fprintf(f, "%d %d %d %d %s\n", l.font, l.x, l.y, l.white, l.text); std::fclose(f); }
    }
    int lines = 0, okRow = 0, maxY = 0, prevY = -1; bool ordered = true;
    for (auto &l : g_drawn) {
        if (l.y > maxY) maxY = l.y;
        if (l.y < prevY) ordered = false; else prevY = l.y;
        if (l.font == 1 && std::strcmp(l.text, "OK") != 0) lines++;
        if (std::strcmp(l.text, "OK") == 0) okRow++;
    }
    int content; WhatsNew_Lines(content);
    CHECK(g_drawn.size() >= 2 && g_drawn[0].font == 2);   // title first
    CHECK(lines == content);                              // the row lists the whole log
    CHECK(okRow == 1);
    CHECK(ordered && maxY + 10 <= H);                     // top to bottom, nothing off-frame
    CHECK(g_drawn.back().y >= (int)(H * 420) / 480);      // OK anchored at the bottom

    // Dismiss with OK (select, bit 4): back to the main menu, seen marked on dismissal.
    next = Menu_Update(screen, 0x10);
    CHECK(next == &g_testMainMenu);
    CHECK(Sys_GetOption("ui.whatsnew_seen", 0) == kWhatsNewRevision);

    // Re-entering the main menu still does not queue anything.
    Menu_Update(&g_testMainMenu, 0);   // pending enter of main
    CHECK(Menu_Update(&g_testMainMenu, 0) == NULL);

    // Manual open from the Options row shows everything again...
    FrontendMenu_OpenWhatsNew(NULL, NULL);
    next = Menu_Update(&g_testMainMenu, 0);
    CHECK(next == screen);
    Menu_Update(screen, 0);
    g_drawn.clear();
    FrontendMenu_DrawWhatsNew(screen);
    int manualLines = 0;
    for (auto &l : g_drawn) if (l.font == 1 && std::strcmp(l.text, "OK") != 0) manualLines++;
    CHECK(manualLines == content);
    // ...and Back (bit 5) also marks nothing new (already seen) and returns.
    next = Menu_Update(screen, 0x20);
    CHECK(next == &g_testMainMenu);
    CHECK(Sys_GetOption("ui.whatsnew_seen", 0) == kWhatsNewRevision);

    // Upgrade path: a future revision with a saved older seen value queues
    // the screen and lists only the newer lines. (Simulated via the logic.)
    CHECK(WhatsNew_UnseenCount(kWhatsNewRevision) == 0);
    CHECK(WhatsNew_UnseenCount(0) == content);

    // ---- Options row select must open the screen --------------------------
    // Mirrors the real Options build line (game/FrontendMenus.cpp). The row
    // is type 2 with the screen as submenu: a type 1 selection runs the
    // action and then takes Menu.cpp's back path, which overwrites
    // g_menuNextAction with the parent - the old type-1 row made the screen
    // unreachable from the menu and dumped the user back out of Options
    // (native report: "what's new item isn't working and messed up the menu").
    {
        Menu options;
        Menu_Init(&options, (char *)"options", 0, 0, &g_testMainMenu, NULL, 1, 0, 0);
        Menu_AddItemType2(&options, FrontendMenu_WhatsNewText(), -1, FrontendMenu_GetWhatsNew(),
                          (MenuItemAction)FrontendMenu_OpenWhatsNew, -1);
        Menu_ValidateCursor(&options, 0);
        g_activeMenu = &options;
        Menu *sel = Menu_Update(&options, 0x10);            // select the row
        CHECK(sel == FrontendMenu_GetWhatsNew());           // opens the screen
        CHECK(g_whatsNewReturn == &options);                // return target captured
        Menu_Update(sel, 0);                                // screen enter
        CHECK(Menu_Update(sel, 0x20) == &options);          // Back returns to Options
        g_activeMenu = &g_testMainMenu;
    }

    // ---- Quit confirmation: every arrow must reach Yes -------------------
    // Mirrors FrontendMenu_BuildQuitConfirmation (game/FrontendMenus.cpp),
    // driving Menu_Update with the production MENU_INPUT_* bits (game/Input.h)
    // - the same word the dispatcher feeds in from DeviceInfo field_0x8. The
    // bits are physical arrows: LEFT=0x01, RIGHT=0x02, UP=0x04, DOWN=0x08.
    // The dialog opts into both-pairs navigation (flag7) because it has no
    // adjustable values: whichever pair the layout maps to value adjustment
    // would otherwise be dead (native reports: Up/Down dead in the original
    // layout, only Left/Right after the layout swap). Safe default stays No.
    {
        g_quitCancelStub = 0;
        Menu quit, quitTarget;
        Menu_Init(&quitTarget, 0, -1, 0, NULL, NULL, 1, 0, 0);
        Menu_Init(&quit, 0, 3, 0, &g_testMainMenu, NULL, 1, 1, 0);   // layout 0
        quit.flag7 = 1;                    // both arrow pairs navigate
        Menu_AddItemType2(&quit, 0, 5, &quitTarget, 0, 0);           // Yes -> quit transition
        Menu_AddItemType1(&quit, 0, 4, (MenuItemAction)QuitCancelStub, 0);  // No
        quit.cursor = 1;                // EnterQuitConfirmation forces No
        Menu_Update(&quit, 0);
        CHECK(quit.cursor == 1);                            // safe default: No
        CHECK(Menu_Update(&quit, MENU_INPUT_UP) == NULL);   // Up reaches Yes
        CHECK(quit.cursor == 0);
        CHECK(Menu_Update(&quit, MENU_INPUT_DOWN) == NULL); // Down back to No
        CHECK(quit.cursor == 1);
        CHECK(Menu_Update(&quit, MENU_INPUT_LEFT) == NULL); // Left reaches Yes
        CHECK(quit.cursor == 0);
        CHECK(Menu_Update(&quit, MENU_INPUT_RIGHT) == NULL);// Right back to No
        CHECK(quit.cursor == 1);
        CHECK(Menu_Update(&quit, MENU_INPUT_UP) == NULL);
        CHECK(quit.cursor == 0);
        CHECK(Menu_Update(&quit, MENU_INPUT_SELECT) == &quitTarget);  // select Yes: transition
        CHECK(g_quitCancelStub == 0);
    }

    // ---- Menu machine regression: the bit mapping itself ------------------
    // Without flag7 the pairs still swap with the layout: layout 1 navigates
    // with Up/Down (Left/Right adjust values, dead on type-1/2 items),
    // layout 0 navigates with Left/Right.
    {
        Menu probe, probeTarget;
        Menu_Init(&probeTarget, 0, -1, 0, NULL, NULL, 1, 0, 0);
        Menu_Init(&probe, 0, 3, 0, &g_testMainMenu, NULL, 1, 1, 1);   // layout 1
        Menu_AddItemType2(&probe, 0, 5, &probeTarget, 0, 0);
        Menu_AddItemType1(&probe, 0, 4, (MenuItemAction)QuitCancelStub, 0);
        probe.cursor = 1;
        Menu_Update(&probe, 0);
        CHECK(Menu_Update(&probe, MENU_INPUT_LEFT) == NULL);  // value adjust: dead
        CHECK(probe.cursor == 1);
        CHECK(Menu_Update(&probe, MENU_INPUT_UP) == NULL);    // prev
        CHECK(probe.cursor == 0);
        probe.layout = 0;
        CHECK(Menu_Update(&probe, MENU_INPUT_UP) == NULL);    // now value adjust: dead
        CHECK(probe.cursor == 0);
        CHECK(Menu_Update(&probe, MENU_INPUT_RIGHT) == NULL); // next
        CHECK(probe.cursor == 1);
    }

    std::filesystem::current_path(old);
    std::filesystem::remove_all(tmp);
    if (!fails) std::puts("frontend_whatsnew_runtime ok");
    return fails;
}
