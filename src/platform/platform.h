// Internal interface between the src/ subsystems (not used by game code).
#ifndef OPENCMR2_PLATFORM_H
#define OPENCMR2_PLATFORM_H

#include <SDL3/SDL.h>

// Sets up SDL, the directories and settings. Returns false (after telling
// the user) when setup is canceled or the game data cannot be found.
bool Platform_Init(int argc, char **argv);
bool Platform_WasSetupCanceled(void);
void Platform_Shutdown(void);

SDL_Window *Platform_GetWindow(void);
const char *Platform_GetDataDir(void);
const char *Platform_GetUserDir(void);

// Writes a whole file through a temporary renamed over it, so it is never
// left truncated. On failure returns false with the SDL error set.
bool Platform_SaveFile(const char *path, const void *data, size_t size);

// OpenCMR2 settings (opencmr2.ini in the user directory): "section.key".
const char *Platform_GetSetting(const char *key, const char *fallback);
int Platform_GetSettingInt(const char *key, int fallback);

// DirectInput scan codes (DIK_*), the game's key codes.
int Keys_FromScancode(SDL_Scancode scancode);
SDL_Scancode Keys_ToScancode(int dik);
// Name of a key for the controls screen (GetKeyNameText).
const char *Keys_GetName(int dik);

// Applications menu entry for this copy of the game (Linux .desktop file,
// Windows Start menu shortcut). Not supported for Flatpak, distribution
// packages or macOS, which provide their own.
bool Shortcut_IsSupported(void);
// "your applications menu", "the Start menu".
const char *Shortcut_MenuName(void);
// Creates or refreshes the entry so it starts this executable, or removes
// it. On failure returns false with the SDL error set.
bool Shortcut_Update(bool present);

// Hides a held key from the game's keyboard state until it is released
// (the key of a platform shortcut such as Alt+Enter).
void Input_HoldBackKey(SDL_Scancode scancode);
// Sparse SDL layout overrides, applied before opening the controller.
bool Input_ApplyGamepadMapping(SDL_JoystickID id, const char *overrides);

#endif
