// Input on SDL3: see port/input.h.

#include "platform/platform.h"
#include "platform/testing.h"
#include "port/input.h"
#include "port/sys.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <vector>
#include <string>
#include <sstream>
#ifdef __ANDROID__
#include <jni.h>
#endif

namespace {

enum DeviceType { DEVICE_KEYBOARD, DEVICE_MOUSE, DEVICE_JOYSTICK };

struct Axis {
    LONG min = -32768;
    LONG max = 32767;
    DWORD deadzone = 0;         // 0..10000
    DWORD saturation = 10000;   // 0..10000
};

}

struct InputDevice {
    DeviceType type;
    SDL_Joystick *joystick = nullptr;
    SDL_Gamepad *gamepad = nullptr;     // controllers SDL knows the layout of
    SDL_Haptic *haptic = nullptr;
    char name[260] = {};
    Axis axes[8];               // X, Y, Z, Rx, Ry, Rz, slider 0, slider 1
    int sdlAxisCount = 0;
    std::vector<InputEffect *> effects;
};

struct InputEffect {
    InputDevice *device;
    int id;
    InputEffectParams params;
};

namespace {

// The game's controller table: two keyboard configurations, four devices.
const int kMaxJoysticks = 4;

bool s_initialised;
#ifdef __ANDROID__
bool s_touchOnly;

bool AndroidTouchOnly()
{
    JNIEnv *env = (JNIEnv *)SDL_GetAndroidJNIEnv();
    jobject activity = (jobject)SDL_GetAndroidActivity();
    if (env == NULL || activity == NULL)
        return false;
    jclass type = env->GetObjectClass(activity);
    jmethodID method = env->GetMethodID(type, "useTouchControlsOnly", "()Z");
    bool result = method != NULL && env->CallBooleanMethod(activity, method);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        result = false;
    }
    env->DeleteLocalRef(type);
    env->DeleteLocalRef(activity);
    return result;
}
#endif
const BYTE *s_testKeyboard;
bool s_heldBack[SDL_SCANCODE_COUNT];
std::vector<SDL_JoystickID> s_openJoysticks;

void EnsureInit()
{
    if (!s_initialised) {
        // The game opened joysticks with DISCL_BACKGROUND. SDL otherwise
        // drops joystick input while no window has the keyboard focus, which
        // under gamescope (Steam Deck Game Mode) can be the whole session.
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
        SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD | SDL_INIT_HAPTIC);
        s_initialised = true;
#ifdef __ANDROID__
        s_touchOnly = AndroidTouchOnly();
#endif
        // Steam replaces the controllers it handles with virtual gamepads and
        // hides the real ones through these: log them with the controllers.
        for (const char *name : {"SteamVirtualGamepadInfo", "SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD",
                                 "SDL_GAMECONTROLLER_IGNORE_DEVICES", "SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT"})
            if (const char *value = SDL_getenv(name))
                Sys_Log("controllers: %s=%s", name, value);
    }
}

const char *JoystickKind(SDL_Joystick *joystick, bool gamepad)
{
    if (gamepad)
        return "gamepad";
    switch (SDL_GetJoystickType(joystick)) {
    case SDL_JOYSTICK_TYPE_WHEEL: return "wheel";
    case SDL_JOYSTICK_TYPE_GAMEPAD: return "gamepad without a known layout";
    default: return "joystick";
    }
}

// Axis index (0..7) of a DirectInput data-format offset, or -1.
int AxisOfOffset(DWORD offset)
{
    if (offset <= 0x1c && (offset & 3) == 0)
        return (int)(offset / 4);
    return -1;
}

// DirectInput's axis transform: dead zone and saturation around the centre,
// then the range.
LONG ScaleAxis(const Axis &axis, Sint16 raw)
{
    double v = raw < 0 ? raw / 32768.0 : raw / 32767.0;
    double magnitude = fabs(v);
    double dz = axis.deadzone / 10000.0;
    double sat = axis.saturation / 10000.0;
    double out;

    if (magnitude <= dz)
        out = 0.0;
    else if (magnitude >= sat || sat <= dz)
        out = 1.0;
    else
        out = (magnitude - dz) / (sat - dz);
    if (v < 0)
        out = -out;
    double centre = (axis.min + (double)axis.max) / 2.0;
    double half = (axis.max - (double)axis.min) / 2.0;
    return (LONG)lround(centre + out * half);
}

// Gamepads are presented as DirectInput showed an Xbox controller, which the
// game was made for: left stick X and Y, right stick Rx and Ry, buttons A, B,
// X, Y, LB, RB, Back, Start and the stick clicks, the D-pad as POV 0. The
// triggers are Z (left) and Rz (right), each read as a wheel's pedal:
// DirectInput drivers combined the triggers on Z, but separate axes let
// throttle and brake be held together. See RawAxis.
const SDL_GamepadAxis s_gamepadAxes[6] = {
    SDL_GAMEPAD_AXIS_LEFTX, SDL_GAMEPAD_AXIS_LEFTY, SDL_GAMEPAD_AXIS_LEFT_TRIGGER,
    SDL_GAMEPAD_AXIS_RIGHTX, SDL_GAMEPAD_AXIS_RIGHTY, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,
};
const SDL_GamepadButton s_gamepadButtons[10] = {
    SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_EAST, SDL_GAMEPAD_BUTTON_WEST, SDL_GAMEPAD_BUTTON_NORTH,
    SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, SDL_GAMEPAD_BUTTON_BACK,
    SDL_GAMEPAD_BUTTON_START, SDL_GAMEPAD_BUTTON_LEFT_STICK, SDL_GAMEPAD_BUTTON_RIGHT_STICK,
};

Uint8 HatOfGamepad(SDL_Gamepad *gamepad)
{
    Uint8 hat = SDL_HAT_CENTERED;
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP))
        hat |= SDL_HAT_UP;
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN))
        hat |= SDL_HAT_DOWN;
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT))
        hat |= SDL_HAT_LEFT;
    if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT))
        hat |= SDL_HAT_RIGHT;
    return hat;
}

// The game reads a throttle or brake on an axis of its own as DirectInput
// wheels report their pedals: the maximum when released, the minimum when
// pressed fully (a centred axis is half pressed). Returns axis i so: SDL's
// triggers go from 0 released to the maximum, and a raw axis that rested at
// its minimum when the device connected (SDL's initial state) is a pedal
// that goes the other way.
Sint16 RawAxis(const InputDevice *device, int i)
{
    if (device->gamepad != NULL) {
        Sint16 raw = SDL_GetGamepadAxis(device->gamepad, s_gamepadAxes[i]);
        if (s_gamepadAxes[i] != SDL_GAMEPAD_AXIS_LEFT_TRIGGER && s_gamepadAxes[i] != SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)
            return raw;
        return (Sint16)(SDL_JOYSTICK_AXIS_MAX - 2 * SDL_max(raw, 0));
    }
    const Sint16 pedalRest = 30000;
    Sint16 raw = SDL_GetJoystickAxis(device->joystick, i);
    Sint16 rest = 0;
    if (SDL_GetJoystickAxisInitialState(device->joystick, i, &rest) && rest <= -pedalRest)
        return (Sint16)(-1 - raw);   // a pedal resting at the minimum
    return raw;
}

DWORD PovOfHat(Uint8 hat)
{
    switch (hat) {
    case SDL_HAT_UP: return 0;
    case SDL_HAT_RIGHTUP: return 4500;
    case SDL_HAT_RIGHT: return 9000;
    case SDL_HAT_RIGHTDOWN: return 13500;
    case SDL_HAT_DOWN: return 18000;
    case SDL_HAT_LEFTDOWN: return 22500;
    case SDL_HAT_LEFT: return 27000;
    case SDL_HAT_LEFTUP: return 31500;
    default: return INPUT_POV_CENTERED;
    }
}

Sint16 ClampLevel(LONG value, DWORD gain)
{
    double v = (double)value * (gain / 10000.0) * (32767.0 / 10000.0);
    if (v > 32767.0)
        v = 32767.0;
    if (v < -32767.0)
        v = -32767.0;
    return (Sint16)v;
}

Uint16 ClampUnsigned(DWORD value)
{
    double v = value * (65535.0 / 10000.0);
    return (Uint16)(v > 65535.0 ? 65535.0 : v);
}

Uint32 Milliseconds(DWORD microseconds)
{
    return microseconds == INPUT_EFFECT_INFINITE ? SDL_HAPTIC_INFINITY : microseconds / 1000;
}

void BuildEffect(const InputEffectParams &p, SDL_HapticEffect *e)
{
    SDL_zerop(e);
    if (p.type == INPUT_EFFECT_CONSTANT) {
        e->type = SDL_HAPTIC_CONSTANT;
        e->constant.direction.type = SDL_HAPTIC_POLAR;
        e->constant.direction.dir[0] = p.direction;
        e->constant.length = Milliseconds(p.duration);
        e->constant.level = ClampLevel(p.magnitude, p.gain);
        e->constant.attack_length = (Uint16)(p.attackTime / 1000);
        e->constant.attack_level = ClampUnsigned(p.attackLevel);
        e->constant.fade_length = (Uint16)(p.fadeTime / 1000);
        e->constant.fade_level = ClampUnsigned(p.fadeLevel);
        if (p.triggerButton != INPUT_EFFECT_NO_TRIGGER)
            e->constant.button = (Uint16)(p.triggerButton - INPUT_OFFSET_BUTTON(0) + 1);
        return;
    }
    switch (p.type) {
    case INPUT_EFFECT_SPRING: e->type = SDL_HAPTIC_SPRING; break;
    case INPUT_EFFECT_DAMPER: e->type = SDL_HAPTIC_DAMPER; break;
    case INPUT_EFFECT_INERTIA: e->type = SDL_HAPTIC_INERTIA; break;
    default: e->type = SDL_HAPTIC_FRICTION; break;
    }
    // One axis (X), as the game sets up its conditions.
    e->condition.direction.type = SDL_HAPTIC_CARTESIAN;
    e->condition.direction.dir[0] = 1;
    e->condition.length = Milliseconds(p.duration);
    e->condition.right_sat[0] = ClampUnsigned(p.saturation);
    e->condition.left_sat[0] = ClampUnsigned(p.saturation);
    e->condition.right_coeff[0] = ClampLevel(p.coefficient, p.gain);
    e->condition.left_coeff[0] = ClampLevel(p.coefficient, p.gain);
    e->condition.deadband[0] = ClampUnsigned((DWORD)(p.deadBand < 0 ? 0 : p.deadBand));
    e->condition.center[0] = ClampLevel(p.offset, 10000);
    if (p.triggerButton != INPUT_EFFECT_NO_TRIGGER)
        e->condition.button = (Uint16)(p.triggerButton - INPUT_OFFSET_BUTTON(0) + 1);
}

} // namespace

// Apply only the controls the user changed, preserving SDL's automatic axes
// and other buttons. Resolve raw-input collisions so one press does not also
// perform the old action. This works for otherwise unknown joysticks too.
bool Input_ApplyGamepadMapping(SDL_JoystickID id, const char *overrides)
{
    if (overrides == NULL || *overrides == 0)
        return true;
    std::vector<std::pair<std::string, std::string>> changes;
    std::string field;
    std::istringstream input(overrides);
    while (std::getline(input, field, ',')) {
        size_t colon = field.find(':');
        if (colon != std::string::npos)
            changes.emplace_back(field.substr(0, colon), field.substr(colon + 1));
    }
    char guid[33];
    SDL_GUIDToString(SDL_GetJoystickGUIDForID(id), guid, sizeof(guid));
    const char *deviceName = SDL_GetJoystickNameForID(id);
    std::string name = deviceName ? deviceName : "Controller";
    std::replace(name.begin(), name.end(), ',', ' ');
    std::string mapping = std::string(guid) + "," + name + ",";
    char *base = SDL_GetGamepadMappingForID(id);
    if (base != NULL) {
        std::istringstream fields(base);
        std::getline(fields, field, ','); // GUID
        std::getline(fields, field, ','); // name
        while (std::getline(fields, field, ',')) {
            size_t colon = field.find(':');
            if (colon == std::string::npos)
                continue;
            const std::string target = field.substr(0, colon);
            const std::string source = field.substr(colon + 1);
            bool replaced = false;
            for (const auto &change : changes)
                if (target == change.first || (!change.second.empty() && source == change.second))
                    replaced = true;
            if (!replaced)
                mapping += field + ",";
        }
        SDL_free(base);
    }
    for (const auto &change : changes)
        if (!change.second.empty())
            mapping += change.first + ":" + change.second + ",";
    return SDL_SetGamepadMapping(id, mapping.c_str());
}

#ifdef __ANDROID__
namespace {
void ApplyAndroidControllerProfile(SDL_JoystickID id)
{
    // The launcher captures Android events and stores SDL bindings. Ask the
    // activity once when opening a device; polling remains entirely in SDL.
    JNIEnv *env = (JNIEnv *)SDL_GetAndroidJNIEnv();
    jobject activity = (jobject)SDL_GetAndroidActivity();
    if (env == NULL || activity == NULL)
        return;
    jclass type = env->GetObjectClass(activity);
    jmethodID method = env->GetMethodID(type, "getControllerMapping", "(Ljava/lang/String;II)Ljava/lang/String;");
    if (method != NULL) {
        const char *name = SDL_GetJoystickNameForID(id);
        jstring jname = env->NewStringUTF(name ? name : "");
        jstring result = (jstring)env->CallObjectMethod(activity, method, jname,
            (jint)SDL_GetJoystickVendorForID(id), (jint)SDL_GetJoystickProductForID(id));
        if (!env->ExceptionCheck() && result != NULL) {
            const char *mapping = env->GetStringUTFChars(result, NULL);
            if (mapping != NULL) {
                if (!Input_ApplyGamepadMapping(id, mapping))
                    Sys_Log("controller profile: %s", SDL_GetError());
                env->ReleaseStringUTFChars(result, mapping);
            }
            env->DeleteLocalRef(result);
        }
        env->DeleteLocalRef(jname);
    }
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        Sys_Log("controller profile: could not read Android settings");
    }
    env->DeleteLocalRef(type);
    env->DeleteLocalRef(activity);
}
}
#endif

// ---- devices -------------------------------------------------------------------

extern "C" int Input_GetJoystickCount(void)
{
    EnsureInit();
#ifdef __ANDROID__
    if (s_touchOnly) return 0;
#endif
    int count = 0;
    SDL_JoystickID *ids = SDL_GetJoysticks(&count);
    SDL_free(ids);
    return count > kMaxJoysticks ? kMaxJoysticks : count;
}

extern "C" BOOL Input_JoysticksChanged(void)
{
    EnsureInit();
#ifdef __ANDROID__
    if (s_touchOnly) return !s_openJoysticks.empty();
#endif
    int count = 0;
    SDL_JoystickID *ids = SDL_GetJoysticks(&count);
    std::vector<SDL_JoystickID> present(ids, ids + (count > kMaxJoysticks ? kMaxJoysticks : count));
    SDL_free(ids);
    std::vector<SDL_JoystickID> open = s_openJoysticks;
    std::sort(present.begin(), present.end());
    std::sort(open.begin(), open.end());
    return present != open;
}

extern "C" InputDevice *Input_OpenKeyboard(void)
{
    InputDevice *device = new InputDevice;
    device->type = DEVICE_KEYBOARD;
    snprintf(device->name, sizeof(device->name), "Keyboard");
    return device;
}

extern "C" InputDevice *Input_OpenMouse(void)
{
    InputDevice *device = new InputDevice;
    device->type = DEVICE_MOUSE;
    snprintf(device->name, sizeof(device->name), "Mouse");
    SDL_GetRelativeMouseState(NULL, NULL);
    return device;
}

extern "C" InputDevice *Input_OpenJoystick(int index)
{
    EnsureInit();
    int count = 0;
    SDL_JoystickID *ids = SDL_GetJoysticks(&count);
    if (ids == NULL || index < 0 || index >= count) {
        SDL_free(ids);
        return NULL;
    }
    SDL_JoystickID id = ids[index];
    SDL_free(ids);
#ifdef __ANDROID__
    ApplyAndroidControllerProfile(id);
#endif
    SDL_Gamepad *gamepad = SDL_IsGamepad(id) ? SDL_OpenGamepad(id) : NULL;
    SDL_Joystick *joystick = gamepad ? SDL_GetGamepadJoystick(gamepad) : SDL_OpenJoystick(id);
    if (joystick == NULL) {
        Sys_Log("controller %d: cannot open %s: %s", index, SDL_GetJoystickNameForID(id), SDL_GetError());
        SDL_CloseGamepad(gamepad);
        return NULL;
    }

    InputDevice *device = new InputDevice;
    device->type = DEVICE_JOYSTICK;
    device->joystick = joystick;
    device->gamepad = gamepad;
    device->sdlAxisCount = gamepad ? 6 : SDL_GetNumJoystickAxes(joystick);
    const char *name = gamepad ? SDL_GetGamepadName(gamepad) : SDL_GetJoystickName(joystick);
    snprintf(device->name, sizeof(device->name), "%s", name ? name : "Joystick");
    if (SDL_IsJoystickHaptic(joystick))
        device->haptic = SDL_OpenHapticFromJoystick(joystick);
    s_openJoysticks.push_back(id);
    char guid[33];
    SDL_GUIDToString(SDL_GetJoystickGUID(joystick), guid, sizeof(guid));
    const char *path = SDL_GetJoystickPath(joystick);
    Sys_Log("controller %d: %s (%04x:%04x), %s, %d axes, %d buttons, %d hats%s%s; GUID %s, %s", index, device->name,
            SDL_GetJoystickVendor(joystick), SDL_GetJoystickProduct(joystick), JoystickKind(joystick, gamepad != NULL),
            SDL_GetNumJoystickAxes(joystick), SDL_GetNumJoystickButtons(joystick), SDL_GetNumJoystickHats(joystick),
            device->haptic != NULL ? ", force feedback" : "",
            (SDL_GetJoystickVendor(joystick) == 0x28de && SDL_GetJoystickProduct(joystick) == 0x11ff) ||
                    (gamepad && SDL_GetGamepadSteamHandle(gamepad)) ? ", Steam virtual gamepad" : "",
            guid, path ? path : "no path");
    return device;
}

extern "C" void Input_CloseDevice(InputDevice *device)
{
    if (device == NULL)
        return;
    while (!device->effects.empty())
        Input_DestroyEffect(device->effects.back());
    if (device->haptic != NULL)
        SDL_CloseHaptic(device->haptic);
    if (device->joystick != NULL) {
        auto open = std::find(s_openJoysticks.begin(), s_openJoysticks.end(), SDL_GetJoystickID(device->joystick));
        if (open != s_openJoysticks.end())
            s_openJoysticks.erase(open);
    }
    if (device->gamepad != NULL)
        SDL_CloseGamepad(device->gamepad);   // and its joystick
    else if (device->joystick != NULL)
        SDL_CloseJoystick(device->joystick);
    if (device->type == DEVICE_MOUSE && Platform_GetWindow() != NULL)
        SDL_SetWindowRelativeMouseMode(Platform_GetWindow(), false);
    delete device;
}

extern "C" const char *Input_GetInstanceName(InputDevice *device)
{
    return device->name;
}

extern "C" const char *Input_GetProductName(InputDevice *device)
{
    return device->name;
}

extern "C" void Input_GetCaps(InputDevice *device, InputCaps *caps)
{
    memset(caps, 0, sizeof(*caps));
    if (device->joystick != NULL) {
        caps->axes = device->sdlAxisCount > 8 ? 8 : device->sdlAxisCount;
        caps->buttons = device->gamepad ? SDL_arraysize(s_gamepadButtons) : SDL_GetNumJoystickButtons(device->joystick);
        if (caps->buttons > 128)
            caps->buttons = 128;
        caps->povs = device->gamepad ? 1 : SDL_GetNumJoystickHats(device->joystick);
        if (caps->povs > 4)
            caps->povs = 4;
        caps->forceFeedback = device->haptic != NULL;
    }
}

extern "C" BOOL Input_GetObjectName(InputDevice *device, DWORD offset, char *name, size_t size)
{
    InputCaps caps;
    static const char *axisNames[8] = { "X Axis", "Y Axis", "Z Axis", "X Rotation", "Y Rotation", "Z Rotation",
                                        "Slider", "Slider" };

    Input_GetCaps(device, &caps);
    if (offset >= INPUT_OFFSET_BUTTON(0)) {
        DWORD button = offset - INPUT_OFFSET_BUTTON(0);
        if (button >= caps.buttons)
            return FALSE;
        snprintf(name, size, "Button %u", (unsigned)button + 1);
        return TRUE;
    }
    if (offset >= INPUT_OFFSET_POV(0)) {
        DWORD pov = (offset - INPUT_OFFSET_POV(0)) / 4;
        if (pov >= caps.povs)
            return FALSE;
        snprintf(name, size, "POV %u", (unsigned)pov + 1);
        return TRUE;
    }
    int axis = AxisOfOffset(offset);
    if (axis < 0 || axis >= (int)caps.axes)
        return FALSE;
    snprintf(name, size, "%s", axisNames[axis]);
    return TRUE;
}

extern "C" BOOL Input_IsGamepad(InputDevice *device)
{
    return device != NULL && device->gamepad != NULL;
}

extern "C" BOOL Input_IsWheel(InputDevice *device)
{
    return device != NULL && device->joystick != NULL && SDL_GetJoystickType(device->joystick) == SDL_JOYSTICK_TYPE_WHEEL;
}

extern "C" BOOL Input_HasAxis(InputDevice *device, DWORD offset)
{
    int axis = AxisOfOffset(offset);
    return device->joystick != NULL && axis >= 0 && axis < device->sdlAxisCount;
}

extern "C" void Input_SetAxisRange(InputDevice *device, DWORD offset, LONG min, LONG max)
{
    int axis = AxisOfOffset(offset);
    if (axis >= 0) {
        device->axes[axis].min = min;
        device->axes[axis].max = max;
    }
}

extern "C" void Input_SetAxisDeadzone(InputDevice *device, DWORD offset, DWORD deadzone)
{
    int axis = AxisOfOffset(offset);
    if (axis >= 0)
        device->axes[axis].deadzone = deadzone > 10000 ? 10000 : deadzone;
}

extern "C" void Input_SetAxisSaturation(InputDevice *device, DWORD offset, DWORD saturation)
{
    int axis = AxisOfOffset(offset);
    if (axis >= 0)
        device->axes[axis].saturation = saturation > 10000 ? 10000 : saturation;
}

void Input_HoldBackKey(SDL_Scancode scancode)
{
    if (scancode > SDL_SCANCODE_UNKNOWN && scancode < SDL_SCANCODE_COUNT)
        s_heldBack[scancode] = true;
}

extern "C" void Input_SetMouseExclusive(InputDevice *device, BOOL exclusive)
{
    if (device->type == DEVICE_MOUSE && Platform_GetWindow() != NULL)
        SDL_SetWindowRelativeMouseMode(Platform_GetWindow(), exclusive != 0);
}

extern "C" BOOL Input_ReadKeyboard(InputDevice *device, BYTE keys[256])
{
    if (s_testKeyboard) {
        memcpy(keys, s_testKeyboard, 256);
        return TRUE;
    }
    int count = 0;
    const bool *state = SDL_GetKeyboardState(&count);

    memset(keys, 0, 256);
    for (int scancode = 0; scancode < count; scancode++) {
        if (scancode < SDL_SCANCODE_COUNT && s_heldBack[scancode]) {
            if (state[scancode])
                continue;
            s_heldBack[scancode] = false;
        }
        if (state[scancode]) {
            int dik = Keys_FromScancode((SDL_Scancode)scancode);
            if (dik != 0)
                keys[dik] = 0x80;
        }
    }
    return TRUE;
}

void Platform_SetTestKeyboard(const BYTE *keys) { s_testKeyboard = keys; }

extern "C" BOOL Input_ReadMouse(InputDevice *device, InputMouseState *state)
{
    float dx = 0.0f, dy = 0.0f;
    SDL_MouseButtonFlags buttons = SDL_GetRelativeMouseState(&dx, &dy);

    memset(state, 0, sizeof(*state));
    state->lX = (LONG)dx;
    state->lY = (LONG)dy;
    state->lZ = 0;
    for (int i = 0; i < 5; i++)
        if (buttons & SDL_BUTTON_MASK(i + 1))
            state->rgbButtons[i] = 0x80;
    return TRUE;
}

extern "C" BOOL Input_ReadJoystick(InputDevice *device, InputJoystickState *state)
{
    LONG values[8];

    memset(state, 0, sizeof(*state));
    if (device->joystick == NULL || !SDL_JoystickConnected(device->joystick))
        return FALSE;
    SDL_UpdateJoysticks();
    for (int i = 0; i < 8; i++) {
        if (i >= device->sdlAxisCount) {
            values[i] = (device->axes[i].min + device->axes[i].max) / 2;
            continue;
        }
        values[i] = ScaleAxis(device->axes[i], RawAxis(device, i));
    }
    state->lX = values[0];
    state->lY = values[1];
    state->lZ = values[2];
    state->lRx = values[3];
    state->lRy = values[4];
    state->lRz = values[5];
    state->rglSlider[0] = values[6];
    state->rglSlider[1] = values[7];
    if (device->gamepad != NULL) {
        state->rgdwPOV[0] = PovOfHat(HatOfGamepad(device->gamepad));
        for (int i = 1; i < 4; i++)
            state->rgdwPOV[i] = INPUT_POV_CENTERED;
        for (size_t i = 0; i < SDL_arraysize(s_gamepadButtons); i++)
            if (SDL_GetGamepadButton(device->gamepad, s_gamepadButtons[i]))
                state->rgbButtons[i] = 0x80;
        return TRUE;
    }
    int hats = SDL_GetNumJoystickHats(device->joystick);
    for (int i = 0; i < 4; i++)
        state->rgdwPOV[i] = i < hats ? PovOfHat(SDL_GetJoystickHat(device->joystick, i)) : INPUT_POV_CENTERED;
    int buttons = SDL_GetNumJoystickButtons(device->joystick);
    for (int i = 0; i < buttons && i < 128; i++)
        if (SDL_GetJoystickButton(device->joystick, i))
            state->rgbButtons[i] = 0x80;
    return TRUE;
}

extern "C" BOOL Input_IsKeyDown(int key)
{
    if (s_testKeyboard)
        return key >= 0 && key < 256 && (s_testKeyboard[key] & 0x80);
    int count = 0;
    const bool *state = SDL_GetKeyboardState(&count);
    SDL_Scancode scancode = Keys_ToScancode(key);
    return scancode != SDL_SCANCODE_UNKNOWN && scancode < count && state[scancode];
}

extern "C" void Input_GetKeyRepeat(DWORD *delay, DWORD *interval)
{
    // Windows' defaults (SPI_GETKEYBOARDDELAY 1, SPEED 31 in the game's
    // conversion); SDL does not report the system's.
    *delay = (DWORD)Platform_GetSettingInt("input.key_repeat_delay", 750);
    *interval = (DWORD)Platform_GetSettingInt("input.key_repeat_interval", 100);
}

extern "C" const char *Input_GetKeyName(int key)
{
    return Keys_GetName(key);
}

// ---- force feedback --------------------------------------------------------------

extern "C" InputEffect *Input_CreateEffect(InputDevice *device, const InputEffectParams *params)
{
    if (device == NULL || device->haptic == NULL)
        return NULL;
    SDL_HapticEffect effect;
    BuildEffect(*params, &effect);
    int id = SDL_CreateHapticEffect(device->haptic, &effect);
    if (id < 0)
        return NULL;
    InputEffect *e = new InputEffect;
    e->device = device;
    e->id = id;
    e->params = *params;
    device->effects.push_back(e);
    return e;
}

extern "C" void Input_UpdateEffect(InputEffect *effect, const InputEffectParams *params)
{
    if (effect == NULL)
        return;
    effect->params = *params;
    SDL_HapticEffect e;
    BuildEffect(*params, &e);
    SDL_UpdateHapticEffect(effect->device->haptic, effect->id, &e);
}

extern "C" void Input_StartEffect(InputEffect *effect)
{
    if (effect != NULL)
        SDL_RunHapticEffect(effect->device->haptic, effect->id, 1);
}

extern "C" BOOL Input_IsEffectPlaying(InputEffect *effect)
{
    return effect != NULL && SDL_GetHapticEffectStatus(effect->device->haptic, effect->id);
}

extern "C" void Input_DestroyEffect(InputEffect *effect)
{
    if (effect == NULL)
        return;
    InputDevice *device = effect->device;
    SDL_DestroyHapticEffect(device->haptic, effect->id);
    for (size_t i = 0; i < device->effects.size(); i++) {
        if (device->effects[i] == effect) {
            device->effects.erase(device->effects.begin() + i);
            break;
        }
    }
    delete effect;
}

extern "C" void Input_StopAllEffects(InputDevice *device)
{
    if (device != NULL && device->haptic != NULL)
        SDL_StopHapticEffects(device->haptic);
}

extern "C" void Input_SetAutocenter(InputDevice *device, BOOL enable)
{
    if (device != NULL && device->haptic != NULL)
        SDL_SetHapticAutocenter(device->haptic, enable ? 100 : 0);
}
