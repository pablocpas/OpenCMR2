/*
 * OpenCMR2 input interface, implemented in src/input on SDL3.
 *
 * The game was written against DirectInput and keeps its model: devices it
 * polls each frame, keyboard state indexed by DirectInput scan codes (DIK_*,
 * the key codes in its bindings and save files), joystick axes scaled to a
 * range the game sets, POV hats in hundredths of a degree, buttons with bit
 * 0x80 set while held, and force-feedback effects.
 *
 * Joystick objects are named by their DirectInput data-format offsets
 * (DIJOFS_*): axes X, Y, Z, Rx, Ry, Rz at 0x00..0x14, sliders at 0x18 and
 * 0x1c, POV hats at 0x20 + 4 * n, buttons at 0x30 + n.
 */
#ifndef OPENCMR2_PORT_INPUT_H
#define OPENCMR2_PORT_INPUT_H

#include "port/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct InputDevice InputDevice;
typedef struct InputEffect InputEffect;

#define INPUT_OFFSET_X 0x00
#define INPUT_OFFSET_Y 0x04
#define INPUT_OFFSET_Z 0x08
#define INPUT_OFFSET_RX 0x0c
#define INPUT_OFFSET_RY 0x10
#define INPUT_OFFSET_RZ 0x14
#define INPUT_OFFSET_SLIDER(n) (0x18 + 4 * (n))
#define INPUT_OFFSET_POV(n) (0x20 + 4 * (n))
#define INPUT_OFFSET_BUTTON(n) (0x30 + (n))

/* Key codes: DirectInput scan codes (the ones the game names). */
#define INPUT_KEY_ESCAPE 0x01
#define INPUT_KEY_RETURN 0x1C
#define INPUT_KEY_NUMPADENTER 0x9C
#define INPUT_KEY_LSHIFT 0x2A
#define INPUT_KEY_SPACE 0x39
#define INPUT_KEY_RSHIFT 0x36
#define INPUT_KEY_F1 0x3B
#define INPUT_KEY_F2 0x3C
#define INPUT_KEY_UP 0xC8
#define INPUT_KEY_LEFT 0xCB
#define INPUT_KEY_RIGHT 0xCD
#define INPUT_KEY_DOWN 0xD0

/* POV value when centred. */
#define INPUT_POV_CENTERED 0xffffffff

typedef struct InputMouseState {
    LONG lX, lY, lZ;            /* movement since the last read */
    BYTE rgbButtons[8];
} InputMouseState;

typedef struct InputJoystickState {
    LONG lX, lY, lZ;
    LONG lRx, lRy, lRz;
    LONG rglSlider[2];
    DWORD rgdwPOV[4];
    BYTE rgbButtons[128];
} InputJoystickState;

typedef struct InputCaps {
    DWORD axes;
    DWORD buttons;
    DWORD povs;
    BOOL forceFeedback;
} InputCaps;

/* ---- devices ---------------------------------------------------------------- */

/* Joysticks the game can use: its controller table holds two keyboard
   configurations and four devices. */
int Input_GetJoystickCount(void);
/* Whether joysticks were connected or disconnected since they were opened. */
BOOL Input_JoysticksChanged(void);

InputDevice *Input_OpenKeyboard(void);
InputDevice *Input_OpenMouse(void);
InputDevice *Input_OpenJoystick(int index);
/* A controller in the Xbox layout (see input_sdl.cpp), rather than a raw
   joystick or wheel. */
BOOL Input_IsGamepad(InputDevice *device);
/* Whether the device reports itself as a steering wheel. */
BOOL Input_IsWheel(InputDevice *device);
void Input_CloseDevice(InputDevice *device);

const char *Input_GetInstanceName(InputDevice *device);
const char *Input_GetProductName(InputDevice *device);
void Input_GetCaps(InputDevice *device, InputCaps *caps);
/* Name of a joystick object by offset; 0 when the device has no such object. */
BOOL Input_GetObjectName(InputDevice *device, DWORD offset, char *name, size_t size);
/* Whether the joystick has the axis at that offset. */
BOOL Input_HasAxis(InputDevice *device, DWORD offset);

/* Axis properties, as DirectInput's DIPROP_RANGE / DEADZONE / SATURATION
   (dead zone and saturation in 0..10000 of the range). */
void Input_SetAxisRange(InputDevice *device, DWORD offset, LONG min, LONG max);
void Input_SetAxisDeadzone(InputDevice *device, DWORD offset, DWORD deadzone);
void Input_SetAxisSaturation(InputDevice *device, DWORD offset, DWORD saturation);

/* Mouse capture: exclusive (relative, hidden) or shared. */
void Input_SetMouseExclusive(InputDevice *device, BOOL exclusive);

BOOL Input_ReadKeyboard(InputDevice *device, BYTE keys[256]);
BOOL Input_ReadMouse(InputDevice *device, InputMouseState *state);
BOOL Input_ReadJoystick(InputDevice *device, InputJoystickState *state);

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
    DWORD triggerButton;        /* INPUT_OFFSET_BUTTON(n) or INPUT_EFFECT_NO_TRIGGER */
    LONG direction;             /* constant force: polar, hundredths of a degree */
    LONG magnitude;             /* constant force */
    DWORD attackTime, attackLevel, fadeTime, fadeLevel;   /* constant force envelope */
    LONG coefficient;           /* conditions (spring, damper, ...) */
    LONG offset;
    DWORD saturation;
    LONG deadBand;
} InputEffectParams;

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
