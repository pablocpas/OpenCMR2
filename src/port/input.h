/*
 * OpenCMR2 input interface, implemented in src/input on SDL3.
 *
 * Input is read as actions of the two local players: driving actions with
 * an analogue value, and menu actions. Each action is bound to keys,
 * controller buttons, axis halves or hat directions in controls.ini (user
 * directory); every connected device belongs to a player. game/Input.cpp
 * presents the result to the game as the device records it was written for.
 *
 * The raw keyboard stays available for text entry: key codes are
 * DirectInput scan codes (DIK_*), the codes the game stores and names.
 */
#ifndef OPENCMR2_PORT_INPUT_H
#define OPENCMR2_PORT_INPUT_H

#include "port/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct InputDevice InputDevice;
typedef struct InputEffect InputEffect;

/* Key codes: DirectInput scan codes (the ones the game names). */
#define INPUT_KEY_ESCAPE 0x01
#define INPUT_KEY_RETURN 0x1C
#define INPUT_KEY_NUMPADENTER 0x9C
#define INPUT_KEY_LSHIFT 0x2A
#define INPUT_KEY_SPACE 0x39
#define INPUT_KEY_RSHIFT 0x36
#define INPUT_KEY_F1 0x3B
#define INPUT_KEY_F2 0x3C
#define INPUT_KEY_F4 0x3E
#define INPUT_KEY_UP 0xC8
#define INPUT_KEY_LEFT 0xCB
#define INPUT_KEY_RIGHT 0xCD
#define INPUT_KEY_DOWN 0xD0

/* ---- players and actions ------------------------------------------------- */

#define INPUT_PLAYERS 2

/* The driving actions come first, in the order of ControllerData's action
   masks (bit 1 << action of a keyboard device). */
enum InputAction {
    INPUT_STEER_LEFT,
    INPUT_STEER_RIGHT,
    INPUT_THROTTLE,
    INPUT_BRAKE,
    INPUT_HANDBRAKE,
    INPUT_SHIFT_UP,
    INPUT_SHIFT_DOWN,
    INPUT_CAMERA,
    INPUT_LOOK_BACK,
    INPUT_PAUSE,
    INPUT_MENU_LEFT,
    INPUT_MENU_RIGHT,
    INPUT_MENU_UP,
    INPUT_MENU_DOWN,
    INPUT_MENU_CONFIRM,
    INPUT_MENU_BACK,
    INPUT_ACTION_COUNT
};

/* Full value of an action: an axis pushed to its saturation, or a key. */
#define INPUT_FULL 0x10000

/* Reads every device once; the values below hold until the next call. */
void Input_Update(void);
/* Local players racing (1 or 2): with one, player 1 also takes the devices
   of player 2. */
void Input_SetLocalPlayers(int count);

/* Value of an action of a player, 0..INPUT_FULL. */
int Input_GetAction(int player, int action);
/* Whether the player drives with an axis rather than keys or buttons: the
   last steering, throttle or brake input came from an axis. */
BOOL Input_IsAnalog(int player);
/* Whether the game eases the steering towards the player's axis rather
   than following it directly (smooth_steering of its device). */
BOOL Input_IsSmoothSteering(int player);
/* Whether a joystick or controller was connected or disconnected since the
   previous call. */
BOOL Input_DevicesChanged(void);
/* A device of the player that has force feedback, or NULL. */
InputDevice *Input_GetFeedbackDevice(int player);
/* Whether the player has a device with force feedback or rumble. */
BOOL Input_HasFeedback(int player);

/* Keyboard keys (DIK_*) of the nine driving actions of the two keyboard
   players, from an older configuration (Controller.rcf): used only while
   controls.ini does not exist yet. */
void Input_ImportKeyboardKeys(int player, const BYTE keys[9]);
/* Writes controls.ini when the bindings changed since it was read. */
void Input_SaveBindings(void);

/* ---- raw keyboard and mouse ------------------------------------------------ */

typedef struct InputMouseState {
    LONG lX, lY, lZ;            /* movement since the last read */
    BYTE rgbButtons[8];
} InputMouseState;

InputDevice *Input_OpenKeyboard(void);
InputDevice *Input_OpenMouse(void);
void Input_CloseDevice(InputDevice *device);
/* Mouse capture: exclusive (relative, hidden) or shared. */
void Input_SetMouseExclusive(InputDevice *device, BOOL exclusive);

BOOL Input_ReadKeyboard(InputDevice *device, BYTE keys[256]);
BOOL Input_ReadMouse(InputDevice *device, InputMouseState *state);
/* Whether a key is held right now (GetAsyncKeyState). */
BOOL Input_IsKeyDown(int key);

/* Keyboard auto-repeat of the system, in milliseconds. */
void Input_GetKeyRepeat(DWORD *delay, DWORD *interval);
/* Name of a key (DIK_*) in the current keyboard layout. */
const char *Input_GetKeyName(int key);

/* ---- force feedback --------------------------------------------------------- */

enum InputEffectType {
    INPUT_EFFECT_CONSTANT,
    INPUT_EFFECT_SPRING,
    INPUT_EFFECT_DAMPER,
    INPUT_EFFECT_INERTIA,
    INPUT_EFFECT_FRICTION
};

#define INPUT_EFFECT_INFINITE 0xffffffff
#define INPUT_EFFECT_NO_TRIGGER 0xffffffff

/* DirectInput's DIEFFECT with its type-specific parameters folded in.
   Times in microseconds, levels and gains in 0..10000. */
typedef struct InputEffectParams {
    int type;
    DWORD duration;
    DWORD gain;
    DWORD triggerButton;        /* DIJOFS_BUTTON(n) or INPUT_EFFECT_NO_TRIGGER */
    LONG direction;             /* constant force: polar, hundredths of a degree */
    LONG magnitude;             /* constant force */
    DWORD attackTime, attackLevel, fadeTime, fadeLevel;   /* constant force envelope */
    LONG coefficient;           /* conditions (spring, damper, ...) */
    LONG offset;
    DWORD saturation;
    LONG deadBand;
} InputEffectParams;

/* Effects outlive a disconnection: they do nothing until the device is
   connected again. */
InputEffect *Input_CreateEffect(InputDevice *device, const InputEffectParams *params);
void Input_UpdateEffect(InputEffect *effect, const InputEffectParams *params);
void Input_StartEffect(InputEffect *effect);
BOOL Input_IsEffectPlaying(InputEffect *effect);
void Input_DestroyEffect(InputEffect *effect);
void Input_StopAllEffects(InputDevice *device);
void Input_SetAutocenter(InputDevice *device, BOOL enable);

#ifdef __cplusplus
}
#endif

#endif
