// Overrides used by the stage probe and startup tests. Normal play uses SDL.
#ifndef OPENCMR2_PLATFORM_TESTING_H
#define OPENCMR2_PLATFORM_TESTING_H
#include "port/types.h"
#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_messagebox.h>
struct PlatformTestClock {
    DWORD (*ticks)();
    void (*sleep)(DWORD);
    unsigned int unixTime;
    double (*ticksPrecise)(); // optional: ticks with fractions of a millisecond
};
void Platform_SetTestClock(const PlatformTestClock *clock);
void Platform_SetTestKeyboard(const BYTE *keys); // 256 DIK entries, NULL disables
void Platform_SetTestUserDir(const char *dir);   // ends with '/'
void Input_ReloadBindings(void);                 // controls.ini of the user directory
// Startup tests provide folder results without opening desktop dialogs.
struct PlatformTestDialogs {
    void (*openFolder)(SDL_DialogFileCallback, void *);
    bool (*messageBox)(const SDL_MessageBoxData *, int *);
};
void Platform_SetTestDialogs(const PlatformTestDialogs *dialogs);
#endif
