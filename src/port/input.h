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

/* The Android text editor is modal; its chip alone is not. While open,
   game actions and raw keys are neutral so a controller cannot move the
   underlying menu away from the entry being edited. */
void Input_SetTextEntryOpen(BOOL open);

/* Value of an action of a player, 0..INPUT_FULL. */
int Input_GetAction(int player, int action);
/* Whether the player steers with an axis rather than keys or buttons, and
   whether the throttle and brake are axes: the last input of each came from
   an axis. An axis counts from where it rests, so a pedal or clutch resting
   at one end, or an axis held since the device was connected, gives no
   input of its own: a device becomes the one used only when it moves. */
BOOL Input_IsAnalogSteering(int player);
BOOL Input_IsAnalogPedals(int player);
/* Whether the game eases the steering towards the player's axis rather
   than following it directly: a gamepad's short stick is eased at the car
   setup's steering speed, about as fast as keys turn the wheels; a wheel's
   or joystick's axis is followed directly, as the original did. */
BOOL Input_IsSmoothSteering(int player);
/* Whether a joystick or controller was connected or disconnected since the
   previous call. */
BOOL Input_DevicesChanged(void);
/* The device the game sends a player's force feedback to: the controller
   the player steers with, if it has force feedback or rumble (NULL if not),
   so that pedals, a shifter or a handbrake of their own never take it.
   Before the player steers, the controller they last used if it can steer,
   else a wheel or joystick with force feedback, else a gamepad that can
   rumble; NULL without either. When that changes, Input_DevicesChanged
   reports it, for the menus to set it up. */
InputDevice *Input_GetFeedbackDevice(int player);
/* Rumble of the gamepad the player steers with, as the force feedback
   (all of the player's before one is used), 0..INPUT_FULL for the low and the high frequency motor,
   scaled by the rumble setting of each. Fades out unless it is set again
   within a quarter of a second. */
void Input_SetRumble(int player, int low, int high);

/* Keyboard keys (DIK_*) of the nine driving actions of the two keyboard
   players, from an older configuration (Controller.rcf): used only while
   controls.ini does not exist yet. */
void Input_ImportKeyboardKeys(int player, const BYTE keys[9]);
/* Writes controls.ini when the bindings changed since it was read. */
void Input_SaveBindings(void);

/* ---- controls pages ------------------------------------------------------- */

/* The bindings the controls pages edit: player 1's keys, player 2's keys,
   gamepads, then each connected joystick or wheel. */
enum InputProfileKind {
    INPUT_PROFILE_KEYBOARD,
    INPUT_PROFILE_KEYBOARD2,
    INPUT_PROFILE_GAMEPAD,
    INPUT_PROFILE_JOYSTICK,
    INPUT_PROFILE_TOUCH         /* Android's on-screen controls: nothing to edit here */
};

enum InputSetting {
    INPUT_SETTING_DEADZONE,     /* percent */
    INPUT_SETTING_CURVE,        /* percent */
    INPUT_SETTING_RUMBLE,       /* percent */
    INPUT_SETTING_PLAYER,       /* a joystick's or wheel's: 0 automatic, 1 or 2 */
    INPUT_SETTING_ENABLED,      /* a joystick's or wheel's: 0 leaves it out */
    INPUT_SETTING_FORCE         /* percent of the force feedback, 0..200 (100: the original's) */
};

/* Listed: the touch controls (Android), the keyboard players while a
   physical keyboard is connected, gamepads while one is, then each connected
   joystick or wheel. The touch controls are edited in the launcher: each of
   their buttons shows the action it presses. */
int Input_GetProfileCount(void);
int Input_GetProfileKind(int profile);
/* The profile whose keys or controller gave the last input, -1 before any
   or when it is not listed. */
int Input_GetLastUsedProfile(void);
/* The device's name for a joystick or wheel, in the game's character set. */
const char *Input_GetProfileName(int profile);
/* The inputs bound to an action, named as on the device (A or Cross, a key
   in the keyboard's layout), in the game's character set; empty when the
   action is not bound. */
void Input_GetBindingText(int profile, int action, char *text, int size);
/* The value an action has now from the profile's keys or devices alone,
   0..INPUT_FULL, to show what each input does. */
int Input_GetProfileAction(int profile, int action);
/* Leaves the action without inputs. */
void Input_ClearBinding(int profile, int action);
/* Whether the input that clears a binding was pressed at the last update:
   Delete or Backspace, or a gamepad's west face button (X, Square). */
BOOL Input_IsClearPressed(void);
/* The name of a gamepad's west face button as on the gamepads connected,
   NULL without one. */
const char *Input_GetClearButtonName(void);
/* The profile the controls pages show, -1 when they are closed: a device
   left out still moves through those pages, so that it can be turned on
   again with it. */
void Input_SetShownProfile(int profile);
int Input_GetSetting(int profile, int setting);
/* Changing the force or the vibration plays a short sample of it on the
   profile's device. */
void Input_SetSetting(int profile, int setting, int value);
/* Whether a connected device of the profile has force feedback (a wheel's
   or joystick's motor; a gamepad's rumble is not). */
BOOL Input_HasForceFeedback(int profile);
/* Plays a short sample of the force feedback or rumble on the profile's
   device, at its strength. */
void Input_TestFeedback(int profile);
/* The strength of the force feedback of the device a player's goes to,
   0..0x10000 as ControllerData's textureForce and effectsForce (0x8000 is
   the original's). */
unsigned int Input_GetFeedbackStrength(int player);

void Input_RestoreDefaults(int profile);
/* Waits for the next key, button, axis or hat of the profile's devices and
   binds it to the action; an input bound to another driving action of the
   profile trades places with the action's own. An axis half given for one
   steering direction binds the other half to the other. Delete or
   Backspace leaves the action unbound instead. */
void Input_BeginCapture(int profile, int action);
/* 1 once bound, -1 if given up (Escape for a controller, or ten seconds),
   0 while waiting. */
int Input_UpdateCapture(void);
/* Reads controls.ini again, dropping the changes not saved. */
void Input_ReloadBindings(void);

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
