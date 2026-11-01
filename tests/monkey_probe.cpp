// Plays the real game with random input to find crashes: random menu keys
// (biased towards Return, so it gets into races), random driving controls,
// pauses and quits, on the virtual clock of stage_probe, so a seed gives the
// same run again. Run many seeds through tools/monkey.py, preferably with the
// sanitizer build (linux-debug).
//
//     monkey_probe DATA SEED FRAMES
//
// XDG_DATA_HOME must be a disposable directory (saves and settings go there).
#include "port/types.h"
#include "platform/platform.h"
#include "platform/testing.h"
#include "port/diagnostics.h"
#include "port/sys.h"
#include "port/input.h"
#include "main.h"
#include "Game.h"
#include "Frontend.h"
#include "FrontendMenus.h"
#include "GameInfo.h"
#include "Menu.h"
#include "Car.h"
#include "RallyData.h"
#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <unistd.h>

extern void Main_InitD3DX();
extern void Input_ClearKeyPressQueue();
extern int Args_Parse(char *);
extern Menu *FrontendMenu_GetActiveMenu();
extern Menu *g_pMenu0x00831778;
extern Menu *g_pHeaderMenu;

static DWORD virtualTime = 1000;
static BYTE g_keys[256];
static unsigned g_sleeps;   // since the last frame
static DWORD Ticks() { return virtualTime; }
// A movie plays inside one frame, waiting on the clock between its frames:
// when the waits pile up, press Return and Space, which skip it, instead of
// decoding it to the end.
static void Sleep(DWORD ms)
{
    virtualTime += ms;
    if (++g_sleeps > 200) {
        g_keys[INPUT_KEY_RETURN] = 0x80;
        g_keys[INPUT_KEY_SPACE] = 0x80;
    }
}

// xorshift64*: the run depends on the seed alone.
struct Random {
    unsigned long long s;
    unsigned Next()
    {
        s ^= s >> 12;
        s ^= s << 25;
        s ^= s >> 27;
        return unsigned((s * 2685821657736338717ULL) >> 32);
    }
    int Below(int n) { return int(Next() % unsigned(n)); }
};

static Random g_random;
static unsigned g_frame;

// Driving: controls held for a random number of steps.
struct Monkey : Diagnostics::Observer {
    unsigned char state = 0;
    unsigned ticks = 0, drivingTicks = 0, hold = 0;
    int digital[4]{}, handbrake = 0, shift[4]{};
    void BeginTick(unsigned char value) override { state = value; }
    void ControlsReady() override
    {
        if (state != 8)
            return;
        if (hold == 0) {
            hold = 5 + g_random.Below(60);
            // Mostly throttle, so the cars cover ground and hit things.
            const int steer = g_random.Below(5);
            digital[0] = steer == 0 ? 63 : steer == 1 ? 32 : 0;
            digital[1] = steer == 2 ? 63 : steer == 3 ? 32 : 0;
            digital[2] = g_random.Below(10) < 7 ? 63 : 0;
            digital[3] = digital[2] == 0 && g_random.Below(3) == 0 ? 63 : 0;
            handbrake = g_random.Below(12) == 0;
            for (int &s : shift)
                s = g_random.Below(40) == 0 ? 1 : 0;
        }
        --hold;
        Car &c = *Car_Get(0);
        for (int i = 0; i < 4; i++) {
            c.flag0x1d0[i] = char(digital[i]);
            c.field_0x1d4[i] = BYTE(shift[i]);
        }
        c.handbrake = handbrake;
        c.field_0x1dc = c.field_0x1e0 = 0;
        ++drivingTicks;
    }
    void EndTick() override { ++ticks; }
};

static void Hang(int)
{
    char text[64];
    int n = snprintf(text, sizeof(text), "monkey: hang at frame %u\n", g_frame);
    if (write(2, text, size_t(n)) < 0) {
    }
    _exit(124);
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "usage: monkey_probe DATA SEED FRAMES\n");
        return 2;
    }
    if (!getenv("XDG_DATA_HOME")) {
        fprintf(stderr, "Set XDG_DATA_HOME to a disposable directory\n");
        return 2;
    }
    const unsigned long long seed = strtoull(argv[2], nullptr, 0);
    const unsigned frames = unsigned(strtoul(argv[3], nullptr, 10));
    g_random.s = seed * 0x9e3779b97f4a7c15ULL + 1;
    SDL_SetMainReady();
    char dataArg[] = "--data";
    char userArg[] = "--user-dir";
    char *platformArgs[] = {argv[0], dataArg, argv[1], userArg, getenv("XDG_DATA_HOME")};
    if (!Platform_Init(5, platformArgs))
        return 1;
    const PlatformTestClock clock{Ticks, Sleep, 1700000000, nullptr};
    BYTE *keys = g_keys;
    Platform_SetTestClock(&clock);
    Platform_SetTestKeyboard(keys);
    // Recorded and random inputs only: no controller connected to the machine.
    Platform_SetTestNoControllers(true);
    Main_InitD3DX();
    CGame::RegisterNetworkResourceRelease();
    Input_ClearKeyPressQueue();
    char command[] = "";
    Args_Parse(command);
    if (!CMain::CreateGameWindow("OpenCMR2 monkey probe"))
        return 1;
    Monkey monkey;
    Diagnostics::observer = &monkey;
    signal(SIGALRM, Hang);

    std::set<int> menus, modes, gameModes;
    Menu *lastMenu = nullptr;
    int lastMode = -1;
    int key = 0;
    unsigned keyFrames = 0, gap = 0, races = 0;
    for (g_frame = 0; g_frame < frames && !CGame::m_shouldExit; ++g_frame) {
        // A frame takes far less than this; a blocking movie (the intro runs
        // 92 seconds) takes longer under the sanitizers.
        alarm(300);
        SDL_PumpEvents();
        const int mode = CGame::GetFrontendResourceMode();
        if (mode != lastMode) {
            fprintf(stderr, "monkey: frame %u mode %d game mode %u\n", g_frame, mode, CGameInfo::GetConfiguredGameMode());
            if (mode == 3) {
                ++races;
                gameModes.insert(int(CGameInfo::GetConfiguredGameMode()));
            }
            modes.insert(mode);
            lastMode = mode;
        }
        Menu *menu = mode == 3 ? g_pHeaderMenu : mode == 2 ? g_pMenu0x00831778 : FrontendMenu_GetActiveMenu();
        if (menu && menu != lastMenu) {
            fprintf(stderr, "monkey: frame %u menu %d\n", g_frame, menu->field_0x4);
            menus.insert(menu->field_0x4);
            lastMenu = menu;
        }

        // Keys are held a few frames, then released, so each is one press.
        memset(keys, 0, sizeof(g_keys));
        g_sleeps = 0;
        if (keyFrames > 0) {
            keys[key] = 0x80;
            --keyFrames;
        } else if (gap > 0) {
            --gap;
        } else {
            // The pause menu stops the steps, so the state stays driving.
            const bool driving = mode == 3 && monkey.state == 8 && !CGameInfo::IsInRaceMenuOpen();
            const int roll = g_random.Below(100);
            if (driving)
                key = roll < 2 ? INPUT_KEY_ESCAPE : 0;
            else
                key = roll < 50   ? INPUT_KEY_RETURN
                      : roll < 65 ? INPUT_KEY_DOWN
                      : roll < 75 ? INPUT_KEY_UP
                      : roll < 83 ? INPUT_KEY_LEFT
                      : roll < 91 ? INPUT_KEY_RIGHT
                      : roll < 96 ? INPUT_KEY_ESCAPE
                                  : 0;
            // Quit, the main menu's last item, would end the run.
            if (key == INPUT_KEY_RETURN && mode == 0 && menu == FrontendMenu_GetMain() &&
                menu->cursor == menu->itemCount - 1)
                key = INPUT_KEY_UP;
            keyFrames = key ? 2 + g_random.Below(3) : 0;
            gap = 2 + g_random.Below(driving ? 60 : 6);
            // Blocking movie playback reads SDL's events itself.
            if (key == INPUT_KEY_RETURN || key == INPUT_KEY_ESCAPE) {
                SDL_Event event{};
                event.type = SDL_EVENT_KEY_DOWN;
                event.key.scancode = key == INPUT_KEY_RETURN ? SDL_SCANCODE_RETURN : SDL_SCANCODE_ESCAPE;
                SDL_PushEvent(&event);
            }
        }

        CGame::UpdateActiveSoundSlots();
        CGame::DispatchFrontendResourceState();
        virtualTime += mode == 3 ? 17 : 33;
        if (g_frame % (getenv("MONKEY_VERBOSE") ? 200 : 5000) == 0)
            fprintf(stderr, "monkey: frame %u ticks %u driving %u races %u menus %zu state %u pause %d\n", g_frame,
                    monkey.ticks, monkey.drivingTicks, races, menus.size(), monkey.state,
                    int(CGameInfo::IsInRaceMenuOpen()));
    }
    alarm(0);
    Diagnostics::observer = nullptr;
    fprintf(stderr, "monkey: %s after %u frames, %u ticks, %u driving, %u races; modes", CGame::m_shouldExit ? "quit" : "done",
            g_frame, monkey.ticks, monkey.drivingTicks, races);
    for (int m : modes)
        fprintf(stderr, " %d", m);
    fprintf(stderr, "; game modes");
    for (int m : gameModes)
        fprintf(stderr, " %d", m);
    fprintf(stderr, "; menus");
    for (int m : menus)
        fprintf(stderr, " %d", m);
    fprintf(stderr, "\n");
    CMain::UnwindGameCallbacks();
    Platform_SetTestKeyboard(nullptr);
    Platform_SetTestClock(nullptr);
    Platform_Shutdown();
    return 0;
}
