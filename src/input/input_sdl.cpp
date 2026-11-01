// Input on SDL3: see port/input.h.

#include "input/input_internal.h"
#include "platform/platform.h"
#include "platform/testing.h"
#include "port/sys.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <sstream>
#ifdef __ANDROID__
#include <jni.h>
#endif

namespace {

enum DeviceKind { DEVICE_KEYBOARD, DEVICE_MOUSE, DEVICE_JOYSTICK };

}

// A joystick, gamepad or wheel, kept after it is disconnected so that its
// effects stay valid and it gets them back when it returns.
struct InputDevice {
    DeviceKind kind;
    SDL_JoystickID id = 0;              // 0 while disconnected
    SDL_Joystick *joystick = nullptr;
    SDL_Gamepad *gamepad = nullptr;     // controllers SDL knows the layout of
    SDL_Haptic *haptic = nullptr;
    std::string identity;               // GUID, plus the serial or path
    std::string name;
    Profile *profile = nullptr;
    int order = 0;                      // connection order
    bool wheel = false;
    bool rumble = false;                // a gamepad with rumble motors
    bool used = false;                  // gave an input since it was connected
    Uint16 rumbleLow = 0, rumbleHigh = 0;
    Uint64 rumbleSent = 0;              // when the rumble was last sent, ms
    std::vector<InputEffect *> effects;
};

struct InputEffect {
    InputDevice *device;
    int id;                             // SDL's effect, -1 until created on the device
    InputEffectParams params;
    bool playing;
};

namespace {

// Whether the steering, and the throttle and brake, come from an axis or
// from keys and buttons: decided apart, so that a D-pad can steer as keys do
// while the triggers stay analogue.
struct Channel {
    bool analog;
    bool decided;                       // set by an input, not assumed
};

enum { CHANNEL_STEERING, CHANNEL_PEDALS, CHANNEL_COUNT };

struct Player {
    int values[INPUT_ACTION_COUNT];
    Channel channels[CHANNEL_COUNT];
    const InputDevice *steering;        // the device last steered with an axis
};

// Inputs of one update of a player's channels.
struct ChannelUse {
    bool digital[CHANNEL_COUNT];
    bool analog[CHANNEL_COUNT];
    bool hasAxis[CHANNEL_COUNT];        // a device of the player has an axis bound to it
};

int ChannelOf(int action)
{
    return action <= INPUT_STEER_RIGHT ? CHANNEL_STEERING : CHANNEL_PEDALS;
}

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
bool s_testNoControllers;
bool s_heldBack[SDL_SCANCODE_COUNT];
std::deque<InputDevice> s_devices;
int s_nextOrder;
bool s_devicesChanged;
int s_localPlayers = 1;
Player s_players[INPUT_PLAYERS];
int s_pausePulse[INPUT_PLAYERS];        // updates the pause is held for after a disconnection

void EnsureInit()
{
    if (s_initialised)
        return;
    // The game opened joysticks with DISCL_BACKGROUND. SDL otherwise drops
    // joystick input while no window has the keyboard focus, which under
    // gamescope (Steam Deck Game Mode) can be the whole session.
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

std::string Identity(SDL_JoystickID id)
{
    char guid[33];
    SDL_GUIDToString(SDL_GetJoystickGUIDForID(id), guid, sizeof(guid));
    const char *path = SDL_GetJoystickPathForID(id);
    return std::string(guid) + "/" + (path ? path : "");
}

// ---- keyboard ------------------------------------------------------------------

bool KeyDown(SDL_Scancode scancode)
{
    if (s_testKeyboard) {
        int dik = Keys_FromScancode(scancode);
        return dik != 0 && (s_testKeyboard[dik] & 0x80);
    }
    int count = 0;
    const bool *state = SDL_GetKeyboardState(&count);
    if (scancode <= SDL_SCANCODE_UNKNOWN || scancode >= count || !state[scancode])
        return false;
    return scancode >= SDL_SCANCODE_COUNT || !s_heldBack[scancode];
}

void ReleaseHeldBackKeys()
{
    int count = 0;
    const bool *state = SDL_GetKeyboardState(&count);
    for (int scancode = 0; scancode < count && scancode < SDL_SCANCODE_COUNT; scancode++)
        if (s_heldBack[scancode] && !state[scancode])
            s_heldBack[scancode] = false;
}

// ---- joysticks -----------------------------------------------------------------

#ifdef __ANDROID__
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
#endif

void CreateEffectOnDevice(InputEffect *effect);

void Open(SDL_JoystickID id)
{
#ifdef __ANDROID__
    ApplyAndroidControllerProfile(id);
#endif
    SDL_Gamepad *gamepad = SDL_IsGamepad(id) ? SDL_OpenGamepad(id) : NULL;
    SDL_Joystick *joystick = gamepad ? SDL_GetGamepadJoystick(gamepad) : SDL_OpenJoystick(id);
    if (joystick == NULL) {
        Sys_Log("controller: cannot open %s: %s", SDL_GetJoystickNameForID(id), SDL_GetError());
        SDL_CloseGamepad(gamepad);
        return;
    }
#ifdef __ANDROID__
    // Android lists some built-in devices of phones as joysticks: only
    // gamepads and wheels are controllers there.
    if (gamepad == NULL && SDL_GetJoystickType(joystick) != SDL_JOYSTICK_TYPE_WHEEL) {
        SDL_CloseJoystick(joystick);
        return;
    }
#endif
    std::string identity = Identity(id);
    InputDevice *device = NULL;
    for (InputDevice &known : s_devices)
        if (known.id == 0 && known.identity == identity)
            device = &known;
    if (device == NULL) {
        s_devices.emplace_back();
        device = &s_devices.back();
        device->kind = DEVICE_JOYSTICK;
        device->identity = identity;
    }
    device->id = id;
    device->joystick = joystick;
    device->gamepad = gamepad;
    device->order = s_nextOrder++;
    device->wheel = SDL_GetJoystickType(joystick) == SDL_JOYSTICK_TYPE_WHEEL;
    const char *name = gamepad ? SDL_GetGamepadName(gamepad) : SDL_GetJoystickName(joystick);
    device->name = name ? name : "Joystick";
    device->profile = Bindings_ForJoystick(joystick, gamepad != NULL);
    device->used = false;
    device->rumble = gamepad != NULL &&
        SDL_GetBooleanProperty(SDL_GetGamepadProperties(gamepad), SDL_PROP_GAMEPAD_CAP_RUMBLE_BOOLEAN, false);
    device->rumbleLow = device->rumbleHigh = 0;
    // The game's effects are a wheel's forces; a gamepad whose haptic device
    // has only rumble motors gets SDL's rumble instead (Input_SetRumble).
    if (SDL_IsJoystickHaptic(joystick)) {
        device->haptic = SDL_OpenHapticFromJoystick(joystick);
        if (device->haptic != NULL &&
            (SDL_GetHapticFeatures(device->haptic) & (SDL_HAPTIC_CONSTANT | SDL_HAPTIC_SPRING | SDL_HAPTIC_DAMPER)) == 0) {
            SDL_CloseHaptic(device->haptic);
            device->haptic = NULL;
        }
        if (device->haptic != NULL)
            SDL_SetHapticAutocenter(device->haptic, 0);
    }
    for (InputEffect *effect : device->effects)
        CreateEffectOnDevice(effect);

    char guid[33];
    SDL_GUIDToString(SDL_GetJoystickGUID(joystick), guid, sizeof(guid));
    const char *path = SDL_GetJoystickPath(joystick);
    Sys_Log("controller: %s (%04x:%04x), %s, %d axes, %d buttons, %d hats%s%s%s; GUID %s, %s; [%s]", device->name.c_str(),
            SDL_GetJoystickVendor(joystick), SDL_GetJoystickProduct(joystick), JoystickKind(joystick, gamepad != NULL),
            SDL_GetNumJoystickAxes(joystick), SDL_GetNumJoystickButtons(joystick), SDL_GetNumJoystickHats(joystick),
            device->haptic != NULL ? ", force feedback" : "", device->rumble ? ", rumble" : "",
            (SDL_GetJoystickVendor(joystick) == 0x28de && SDL_GetJoystickProduct(joystick) == 0x11ff) ||
                    (gamepad && SDL_GetGamepadSteamHandle(gamepad)) ? ", Steam virtual gamepad" : "",
            guid, path ? path : "no path", device->profile->section.c_str());
    s_devicesChanged = true;
}

int Owner(const InputDevice &device);
bool Plays(int owner, int player);

void Close(InputDevice &device)
{
    Sys_Log("controller: %s disconnected", device.name.c_str());
    // Losing the controller a player drives with pauses a stage, as the
    // pause action held briefly (nothing in the menus).
    if (device.used)
        for (int player = 0; player < INPUT_PLAYERS; player++)
            if (Plays(Owner(device), player))
                s_pausePulse[player] = 2;
    for (InputEffect *effect : device.effects)
        effect->id = -1;
    if (device.haptic != NULL)
        SDL_CloseHaptic(device.haptic);
    if (device.gamepad != NULL)
        SDL_CloseGamepad(device.gamepad);   // and its joystick
    else
        SDL_CloseJoystick(device.joystick);
    device.haptic = NULL;
    device.gamepad = NULL;
    device.joystick = NULL;
    device.id = 0;
    s_devicesChanged = true;
}

void ScanJoysticks()
{
    int count = 0;
    SDL_JoystickID *ids = SDL_GetJoysticks(&count);
#ifdef __ANDROID__
    if (s_touchOnly)
        count = 0;
#endif
    if (s_testNoControllers)
        count = 0;
    std::vector<SDL_JoystickID> present(ids, ids + count);
    SDL_free(ids);
    for (InputDevice &device : s_devices)
        if (device.id != 0 && std::find(present.begin(), present.end(), device.id) == present.end())
            Close(device);
    for (SDL_JoystickID id : present) {
        bool open = false;
        for (const InputDevice &device : s_devices)
            open = open || device.id == id;
        if (!open)
            Open(id);
    }
}

// ---- actions -------------------------------------------------------------------

// The player a connected device belongs to (0 or 1): its profile's, else
// player 2 for the second controller connected.
int Owner(const InputDevice &device)
{
    if (device.profile->player != 0)
        return device.profile->player - 1;
    int earlier = 0;
    for (const InputDevice &other : s_devices)
        if (other.id != 0 && other.profile->player == 0 && other.order < device.order)
            earlier++;
    return earlier == 1 ? 1 : 0;
}

bool Plays(int owner, int player)
{
    return owner == player || (player == 0 && s_localPlayers < 2);
}

int Scale(const Profile &profile, int value)
{
    int deadzone = profile.deadzone * INPUT_FULL / 100;
    int saturation = profile.saturation * INPUT_FULL / 100;
    if (value <= deadzone)
        return 0;
    if (value >= saturation || saturation <= deadzone)
        return INPUT_FULL;
    return (int)((long long)(value - deadzone) * INPUT_FULL / (saturation - deadzone));
}

int AxisValue(Sint16 raw, int range)
{
    switch (range) {
    case AXIS_NEGATIVE: return raw < 0 ? (int)(-(long long)raw * INPUT_FULL / 32768) : 0;
    case AXIS_POSITIVE: return raw > 0 ? (int)((long long)raw * INPUT_FULL / 32767) : 0;
    case AXIS_RISING: return (int)(((long long)raw + 32768) * INPUT_FULL / 65535);
    default: return (int)((32767 - (long long)raw) * INPUT_FULL / 65535);
    }
}

int SourceValue(const InputDevice *device, const Profile &profile, const Source &source)
{
    switch (source.kind) {
    case SOURCE_KEY:
        return KeyDown((SDL_Scancode)source.index) ? INPUT_FULL : 0;
    case SOURCE_BUTTON:
        return device->gamepad && SDL_GetGamepadButton(device->gamepad, (SDL_GamepadButton)source.index) ? INPUT_FULL : 0;
    case SOURCE_AXIS:
        if (device->gamepad == NULL)
            return 0;
        return Scale(profile, AxisValue(SDL_GetGamepadAxis(device->gamepad, (SDL_GamepadAxis)source.index), source.direction));
    case SOURCE_JOY_BUTTON:
        return source.index < SDL_GetNumJoystickButtons(device->joystick) &&
               SDL_GetJoystickButton(device->joystick, source.index) ? INPUT_FULL : 0;
    case SOURCE_JOY_HAT:
        return source.index < SDL_GetNumJoystickHats(device->joystick) &&
               (SDL_GetJoystickHat(device->joystick, source.index) & source.direction) ? INPUT_FULL : 0;
    case SOURCE_JOY_AXIS:
        if (source.index >= SDL_GetNumJoystickAxes(device->joystick))
            return 0;
        return Scale(profile, AxisValue(SDL_GetJoystickAxis(device->joystick, source.index), source.direction));
    }
    return 0;
}

// Takes the larger value of each action from one profile into the player's,
// and notes whether a driving action came from a key or button or an axis.
// Blends the linear response with a cubic one, which moves less near the
// centre and reaches the full value at the same end.
int Curve(int value, int percent)
{
    long long v = value;
    long long cubic = v * v / INPUT_FULL * v / INPUT_FULL;
    return (int)((v * (100 - percent) + cubic * percent) / 100);
}

void Accumulate(Player &player, InputDevice *device, const Profile &profile, ChannelUse *use)
{
    for (int action = 0; action < INPUT_ACTION_COUNT; action++) {
        for (const Source &source : profile.actions[action]) {
            int value = SourceValue(device, profile, source);
            if (action <= INPUT_STEER_RIGHT && IsAnalog(source) && profile.steeringCurve != 0)
                value = Curve(value, profile.steeringCurve);
            player.values[action] = std::max(player.values[action], value);
            if (device != NULL && value > INPUT_FULL / 4)
                device->used = true;
            if (action > INPUT_BRAKE)
                continue;
            if (IsAnalog(source)) {
                use->hasAxis[ChannelOf(action)] = use->hasAxis[ChannelOf(action)] || device != NULL;
                if (value > INPUT_FULL / 4) {
                    use->analog[ChannelOf(action)] = true;
                    if (action <= INPUT_STEER_RIGHT)
                        player.steering = device;
                }
            } else if (value != 0) {
                use->digital[ChannelOf(action)] = true;
            }
        }
    }
}

bool SteersWithAxis(const Profile &profile)
{
    for (int action = INPUT_STEER_LEFT; action <= INPUT_STEER_RIGHT; action++)
        for (const Source &source : profile.actions[action])
            if (IsAnalog(source))
                return true;
    return false;
}

void UpdatePlayer(int index)
{
    Player &player = s_players[index];
    std::deque<Profile> &profiles = Bindings_Get();
    ChannelUse use = {};

    memset(player.values, 0, sizeof(player.values));
    for (Profile &profile : profiles)
        if ((profile.section == "keyboard" && Plays(0, index)) || (profile.section == "keyboard2" && Plays(1, index)))
            Accumulate(player, NULL, profile, &use);
#ifdef __ANDROID__
    if (index == 0)
        Accumulate(player, NULL, Bindings_TouchProfile(), &use);
#endif
    for (InputDevice &device : s_devices) {
        if (device.id == 0 || !Plays(Owner(device), index))
            continue;
        Accumulate(player, &device, *device.profile, &use);
    }
    if (s_pausePulse[index] > 0) {
        player.values[INPUT_PAUSE] = INPUT_FULL;
        s_pausePulse[index]--;
    }
    // The kind of input last used decides; before any input, an axis if the
    // player has one.
    for (int i = 0; i < CHANNEL_COUNT; i++) {
        Channel &channel = player.channels[i];
        if (use.analog[i] != use.digital[i]) {
            channel.analog = use.analog[i];
            channel.decided = true;
        } else if (!channel.decided || (channel.analog && !use.hasAxis[i])) {
            channel.analog = use.hasAxis[i];
        }
    }
}

// ---- force feedback --------------------------------------------------------------

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

// DIJOFS_BUTTON(n) of the trigger button.
const DWORD kFirstButtonOffset = 0x30;

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
            e->constant.button = (Uint16)(p.triggerButton - kFirstButtonOffset + 1);
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
        e->condition.button = (Uint16)(p.triggerButton - kFirstButtonOffset + 1);
}

void CreateEffectOnDevice(InputEffect *effect)
{
    SDL_Haptic *haptic = effect->device->haptic;
    if (haptic == NULL)
        return;
    SDL_HapticEffect e;
    BuildEffect(effect->params, &e);
    effect->id = SDL_CreateHapticEffect(haptic, &e);
    if (effect->id >= 0 && effect->playing)
        SDL_RunHapticEffect(haptic, effect->id, 1);
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

void Input_HoldBackKey(SDL_Scancode scancode)
{
    if (scancode > SDL_SCANCODE_UNKNOWN && scancode < SDL_SCANCODE_COUNT)
        s_heldBack[scancode] = true;
}

void Platform_SetTestKeyboard(const BYTE *keys) { s_testKeyboard = keys; }
void Platform_SetTestNoControllers(bool ignore) { s_testNoControllers = ignore; }

void Input_ReloadBindings(void)
{
    Bindings_Load();
    for (InputDevice &device : s_devices)
        device.profile = device.id != 0 ? Bindings_ForJoystick(device.joystick, device.gamepad != NULL) : NULL;
}

// ---- players and actions -------------------------------------------------------

extern "C" void Input_Update(void)
{
    EnsureInit();
    SDL_UpdateJoysticks();
    ScanJoysticks();
    ReleaseHeldBackKeys();
    for (int player = 0; player < INPUT_PLAYERS; player++)
        UpdatePlayer(player);
}

extern "C" void Input_SetLocalPlayers(int count)
{
    s_localPlayers = count < 2 ? 1 : 2;
}

extern "C" int Input_GetAction(int player, int action)
{
    if (player < 0 || player >= INPUT_PLAYERS || action < 0 || action >= INPUT_ACTION_COUNT)
        return 0;
    return s_players[player].values[action];
}

extern "C" BOOL Input_IsAnalogSteering(int player)
{
    return player >= 0 && player < INPUT_PLAYERS && s_players[player].channels[CHANNEL_STEERING].analog;
}

extern "C" BOOL Input_IsAnalogPedals(int player)
{
    return player >= 0 && player < INPUT_PLAYERS && s_players[player].channels[CHANNEL_PEDALS].analog;
}

extern "C" BOOL Input_IsSmoothSteering(int player)
{
    // A stick is short: followed directly, the wheels jump to its angle in
    // one 40 ms physics step. The device last steered with decides, else the
    // first that can steer.
    if (player < 0 || player >= INPUT_PLAYERS)
        return FALSE;
    const InputDevice *steering = s_players[player].steering;
    for (const InputDevice &device : s_devices)
        if (steering == &device && device.id != 0)
            return device.gamepad != NULL && !device.wheel;
    for (const InputDevice &device : s_devices)
        if (device.id != 0 && Plays(Owner(device), player) && SteersWithAxis(*device.profile))
            return device.gamepad != NULL && !device.wheel;
    return FALSE;
}

extern "C" BOOL Input_DevicesChanged(void)
{
    bool changed = s_devicesChanged;
    s_devicesChanged = false;
    return changed;
}

extern "C" InputDevice *Input_GetFeedbackDevice(int player)
{
    InputDevice *best = NULL;
    for (InputDevice &device : s_devices) {
        if (device.id == 0 || (device.haptic == NULL && !device.rumble) || !Plays(Owner(device), player))
            continue;
        // A device with force feedback first, wheels first among them.
        int rank = device.haptic != NULL ? (device.wheel ? 3 : 2) : 1;
        int bestRank = best == NULL ? 0 : best->haptic != NULL ? (best->wheel ? 3 : 2) : 1;
        if (rank > bestRank || (rank == bestRank && device.order < best->order))
            best = &device;
    }
    return best;
}

extern "C" void Input_SetRumble(int player, int low, int high)
{
    const Uint32 duration = 250;
    Uint64 now = SDL_GetTicks();
    for (InputDevice &device : s_devices) {
        if (device.id == 0 || !device.rumble || !Plays(Owner(device), player))
            continue;
        int scale = device.profile->rumble;
        Uint16 l = (Uint16)(SDL_clamp(low, 0, INPUT_FULL) * (Sint64)scale * 0xffff / (100 * (Sint64)INPUT_FULL));
        Uint16 h = (Uint16)(SDL_clamp(high, 0, INPUT_FULL) * (Sint64)scale * 0xffff / (100 * (Sint64)INPUT_FULL));
        // Sent when it changes noticeably, and renewed before it runs out.
        bool changed = abs(l - device.rumbleLow) > 0x400 || abs(h - device.rumbleHigh) > 0x400 ||
                       ((l == 0) != (device.rumbleLow == 0)) || ((h == 0) != (device.rumbleHigh == 0));
        bool renew = (l != 0 || h != 0) && now - device.rumbleSent >= duration / 2;
        if (!changed && !renew)
            continue;
        SDL_RumbleGamepad(device.gamepad, l, h, duration);
        device.rumbleLow = l;
        device.rumbleHigh = h;
        device.rumbleSent = now;
    }
}

// ---- raw keyboard and mouse ------------------------------------------------------

extern "C" InputDevice *Input_OpenKeyboard(void)
{
    InputDevice *device = new InputDevice;
    device->kind = DEVICE_KEYBOARD;
    device->name = "Keyboard";
    return device;
}

extern "C" InputDevice *Input_OpenMouse(void)
{
    InputDevice *device = new InputDevice;
    device->kind = DEVICE_MOUSE;
    device->name = "Mouse";
    SDL_GetRelativeMouseState(NULL, NULL);
    return device;
}

extern "C" void Input_CloseDevice(InputDevice *device)
{
    if (device == NULL || device->kind == DEVICE_JOYSTICK)
        return;
    if (device->kind == DEVICE_MOUSE && Platform_GetWindow() != NULL)
        SDL_SetWindowRelativeMouseMode(Platform_GetWindow(), false);
    delete device;
}

extern "C" void Input_SetMouseExclusive(InputDevice *device, BOOL exclusive)
{
    if (device->kind == DEVICE_MOUSE && Platform_GetWindow() != NULL)
        SDL_SetWindowRelativeMouseMode(Platform_GetWindow(), exclusive != 0);
}

extern "C" BOOL Input_ReadKeyboard(InputDevice *device, BYTE keys[256])
{
    (void)device;
    if (s_testKeyboard) {
        memcpy(keys, s_testKeyboard, 256);
        return TRUE;
    }
    int count = 0;
    const bool *state = SDL_GetKeyboardState(&count);

    memset(keys, 0, 256);
    ReleaseHeldBackKeys();
    for (int scancode = 0; scancode < count; scancode++) {
        if (state[scancode] && !(scancode < SDL_SCANCODE_COUNT && s_heldBack[scancode])) {
            int dik = Keys_FromScancode((SDL_Scancode)scancode);
            if (dik != 0)
                keys[dik] = 0x80;
        }
    }
    return TRUE;
}

extern "C" BOOL Input_ReadMouse(InputDevice *device, InputMouseState *state)
{
    (void)device;
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
    if (device == NULL || device->kind != DEVICE_JOYSTICK)
        return NULL;
    InputEffect *effect = new InputEffect;
    effect->device = device;
    effect->id = -1;
    effect->params = *params;
    effect->playing = false;
    CreateEffectOnDevice(effect);
    device->effects.push_back(effect);
    return effect;
}

extern "C" void Input_UpdateEffect(InputEffect *effect, const InputEffectParams *params)
{
    if (effect == NULL)
        return;
    effect->params = *params;
    if (effect->id < 0 || effect->device->haptic == NULL)
        return;
    SDL_HapticEffect e;
    BuildEffect(*params, &e);
    SDL_UpdateHapticEffect(effect->device->haptic, effect->id, &e);
}

extern "C" void Input_StartEffect(InputEffect *effect)
{
    if (effect == NULL)
        return;
    effect->playing = true;
    if (effect->id >= 0 && effect->device->haptic != NULL)
        SDL_RunHapticEffect(effect->device->haptic, effect->id, 1);
}

extern "C" BOOL Input_IsEffectPlaying(InputEffect *effect)
{
    if (effect == NULL)
        return FALSE;
    if (effect->id < 0 || effect->device->haptic == NULL)
        return effect->playing;
    return SDL_GetHapticEffectStatus(effect->device->haptic, effect->id);
}

extern "C" void Input_DestroyEffect(InputEffect *effect)
{
    if (effect == NULL)
        return;
    InputDevice *device = effect->device;
    if (effect->id >= 0 && device->haptic != NULL)
        SDL_DestroyHapticEffect(device->haptic, effect->id);
    device->effects.erase(std::remove(device->effects.begin(), device->effects.end(), effect), device->effects.end());
    delete effect;
}

extern "C" void Input_StopAllEffects(InputDevice *device)
{
    if (device == NULL)
        return;
    for (InputEffect *effect : device->effects)
        effect->playing = false;
    if (device->haptic != NULL)
        SDL_StopHapticEffects(device->haptic);
}

extern "C" void Input_SetAutocenter(InputDevice *device, BOOL enable)
{
    if (device != NULL && device->haptic != NULL)
        SDL_SetHapticAutocenter(device->haptic, enable ? 100 : 0);
}

// ---- controls pages ----------------------------------------------------------------

namespace {

std::vector<Profile *> EditableProfiles()
{
    std::vector<Profile *> profiles;
    for (const char *section : { "keyboard", "keyboard2", "gamepad" })
        profiles.push_back(Bindings_Find(section));
    for (InputDevice &device : s_devices)
        if (device.id != 0 && device.profile->section.compare(0, 7, "device ") == 0 &&
            std::find(profiles.begin(), profiles.end(), device.profile) == profiles.end())
            profiles.push_back(device.profile);
    return profiles;
}

Profile *EditableProfile(int index)
{
    Bindings_Get();
    std::vector<Profile *> profiles = EditableProfiles();
    return index >= 0 && index < (int)profiles.size() ? profiles[index] : NULL;
}

InputDevice *DeviceOf(const Profile *profile)
{
    for (InputDevice &device : s_devices)
        if (device.id != 0 && device.profile == profile)
            return &device;
    return NULL;
}

// UTF-8 to the game's character set (Windows-1252 for Latin-1's letters).
std::string GameText(const char *utf8)
{
    std::string text;
    Uint32 c;
    while ((c = SDL_StepUTF8(&utf8, NULL)) != 0)
        text += c < 0x100 ? (char)c : '?';
    return text;
}

// Button and axis names in the style of the gamepads connected.
enum PadFamily { PAD_XBOX, PAD_PLAYSTATION, PAD_NINTENDO };

PadFamily Family()
{
    for (const InputDevice &device : s_devices) {
        if (device.id == 0 || device.gamepad == NULL)
            continue;
        switch (SDL_GetGamepadType(device.gamepad)) {
        case SDL_GAMEPAD_TYPE_PS3:
        case SDL_GAMEPAD_TYPE_PS4:
        case SDL_GAMEPAD_TYPE_PS5:
            return PAD_PLAYSTATION;
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
        case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
            return PAD_NINTENDO;
        default:
            return PAD_XBOX;
        }
    }
    return PAD_XBOX;
}

std::string ButtonName(int button, PadFamily family)
{
    static const char *const faces[3][4] = {
        { "A", "B", "X", "Y" }, { "Cross", "Circle", "Square", "Triangle" }, { "B", "A", "Y", "X" },
    };
    static const char *const others[3][11] = {
        { "Back", "Guide", "Start", "LS", "RS", "LB", "RB", "D-pad Up", "D-pad Down", "D-pad Left", "D-pad Right" },
        { "Share", "PS", "Options", "L3", "R3", "L1", "R1", "D-pad Up", "D-pad Down", "D-pad Left", "D-pad Right" },
        { "-", "Home", "+", "LS", "RS", "L", "R", "D-pad Up", "D-pad Down", "D-pad Left", "D-pad Right" },
    };
    if (button <= SDL_GAMEPAD_BUTTON_NORTH)
        return faces[family][button];
    if (button <= SDL_GAMEPAD_BUTTON_DPAD_RIGHT)
        return others[family][button - SDL_GAMEPAD_BUTTON_BACK];
    if (button == SDL_GAMEPAD_BUTTON_MISC1)
        return family == PAD_PLAYSTATION ? "Mic" : family == PAD_NINTENDO ? "Capture" : "Share";
    if (button == SDL_GAMEPAD_BUTTON_TOUCHPAD)
        return "Touchpad";
    if (button >= SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1 && button <= SDL_GAMEPAD_BUTTON_LEFT_PADDLE2)
        return "Paddle " + std::to_string(button - SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1 + 1);
    return "Button " + std::to_string(button);
}

std::string AxisName(const Source &source, PadFamily family)
{
    static const char *const triggers[3][2] = { { "LT", "RT" }, { "L2", "R2" }, { "ZL", "ZR" } };
    switch (source.index) {
    case SDL_GAMEPAD_AXIS_LEFTX: return source.direction == AXIS_NEGATIVE ? "LS Left" : "LS Right";
    case SDL_GAMEPAD_AXIS_LEFTY: return source.direction == AXIS_NEGATIVE ? "LS Up" : "LS Down";
    case SDL_GAMEPAD_AXIS_RIGHTX: return source.direction == AXIS_NEGATIVE ? "RS Left" : "RS Right";
    case SDL_GAMEPAD_AXIS_RIGHTY: return source.direction == AXIS_NEGATIVE ? "RS Up" : "RS Down";
    case SDL_GAMEPAD_AXIS_LEFT_TRIGGER: return triggers[family][0];
    default: return triggers[family][1];
    }
}

std::string SourceName(const Source &source)
{
    switch (source.kind) {
    case SOURCE_KEY:
        return GameText(SDL_GetKeyName(SDL_GetKeyFromScancode((SDL_Scancode)source.index, SDL_KMOD_NONE, false)));
    case SOURCE_BUTTON:
        return ButtonName(source.index, Family());
    case SOURCE_AXIS:
        return AxisName(source, Family());
    case SOURCE_JOY_BUTTON:
        return "Button " + std::to_string(source.index + 1);
    case SOURCE_JOY_HAT:
        return std::string("Hat ") + (source.direction == SDL_HAT_UP ? "Up" : source.direction == SDL_HAT_DOWN ? "Down"
                                      : source.direction == SDL_HAT_LEFT ? "Left" : "Right");
    case SOURCE_JOY_AXIS:
        return "Axis " + std::to_string(source.index + 1) +
               (source.direction == AXIS_NEGATIVE ? "-" : source.direction == AXIS_POSITIVE ? "+" : "");
    }
    return "";
}

bool SameSource(const Source &a, const Source &b)
{
    return a.kind == b.kind && a.index == b.index && a.direction == b.direction;
}

// The input waited for by the controls pages.
struct Capture {
    Profile *profile = NULL;
    int action = 0;
    bool released = false;
    Uint64 start = 0;
    std::vector<std::vector<Sint16>> axes;      // per device of the profile, where its axes were
} s_capture;

std::vector<InputDevice *> CaptureDevices()
{
    std::vector<InputDevice *> devices;
    for (InputDevice &device : s_devices)
        if (device.id != 0 && device.profile == s_capture.profile)
            devices.push_back(&device);
    return devices;
}

bool IsKeyboardProfile(const Profile *profile)
{
    return profile->section == "keyboard" || profile->section == "keyboard2";
}

bool AnyKeyDown()
{
    int count = 0;
    const bool *state = SDL_GetKeyboardState(&count);
    for (int scancode = 1; scancode < count; scancode++)
        if (KeyDown((SDL_Scancode)scancode))
            return true;
    (void)state;
    return false;
}

// A gamepad's input in SDL's layout, or a joystick's raw one, that is
// pressed, or moved half way from where it was when the capture began.
bool FindInput(InputDevice *device, const std::vector<Sint16> &base, bool gamepadLayout, Source *out)
{
    const Sint16 travel = 16384;
    if (gamepadLayout) {
        for (int button = 0; button < SDL_GAMEPAD_BUTTON_COUNT; button++) {
            if (SDL_GetGamepadButton(device->gamepad, (SDL_GamepadButton)button)) {
                *out = { SOURCE_BUTTON, button, 0 };
                return true;
            }
        }
        for (int axis = 0; axis < SDL_GAMEPAD_AXIS_COUNT; axis++) {
            Sint16 value = SDL_GetGamepadAxis(device->gamepad, (SDL_GamepadAxis)axis);
            if (abs(value - base[axis]) < travel)
                continue;
            bool trigger = axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER || axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER;
            *out = { SOURCE_AXIS, axis, trigger || value > 0 ? AXIS_POSITIVE : AXIS_NEGATIVE };
            return true;
        }
        return false;
    }
    for (int button = 0; button < SDL_GetNumJoystickButtons(device->joystick); button++) {
        if (SDL_GetJoystickButton(device->joystick, button)) {
            *out = { SOURCE_JOY_BUTTON, button, 0 };
            return true;
        }
    }
    for (int hat = 0; hat < SDL_GetNumJoystickHats(device->joystick); hat++) {
        Uint8 value = SDL_GetJoystickHat(device->joystick, hat);
        for (Uint8 direction : { SDL_HAT_UP, SDL_HAT_RIGHT, SDL_HAT_DOWN, SDL_HAT_LEFT }) {
            if (value & direction) {
                *out = { SOURCE_JOY_HAT, hat, direction };
                return true;
            }
        }
    }
    for (int axis = 0; axis < SDL_GetNumJoystickAxes(device->joystick) && axis < (int)base.size(); axis++) {
        Sint16 value = SDL_GetJoystickAxis(device->joystick, axis);
        if (abs(value - base[axis]) < travel)
            continue;
        // An axis that was resting at one end is a pedal.
        int range = base[axis] <= -24000 ? AXIS_RISING : base[axis] >= 24000 ? AXIS_FALLING
                  : value > base[axis] ? AXIS_POSITIVE : AXIS_NEGATIVE;
        *out = { SOURCE_JOY_AXIS, axis, range };
        return true;
    }
    return false;
}

bool AnyInput(InputDevice *device, const std::vector<Sint16> &base, bool gamepadLayout)
{
    Source ignored;
    return FindInput(device, base, gamepadLayout, &ignored);
}

std::vector<Sint16> AxisState(InputDevice *device, bool gamepadLayout)
{
    std::vector<Sint16> axes;
    if (gamepadLayout) {
        for (int axis = 0; axis < SDL_GAMEPAD_AXIS_COUNT; axis++)
            axes.push_back(SDL_GetGamepadAxis(device->gamepad, (SDL_GamepadAxis)axis));
    } else {
        for (int axis = 0; axis < SDL_GetNumJoystickAxes(device->joystick); axis++)
            axes.push_back(SDL_GetJoystickAxis(device->joystick, axis));
    }
    return axes;
}

// Binds the input, trading places with a driving action that had it.
void Assign(Profile &profile, int action, const Source &source)
{
    std::vector<Source> previous = profile.actions[action];
    for (int other = 0; other <= INPUT_PAUSE; other++) {
        if (other == action)
            continue;
        std::vector<Source> &list = profile.actions[other];
        for (size_t i = 0; i < list.size(); i++) {
            if (!SameSource(list[i], source))
                continue;
            if (!previous.empty())
                list[i] = previous[0];
            else
                list.erase(list.begin() + i);
            break;
        }
    }
    profile.actions[action] = { source };
    Bindings_MarkChanged();
}

} // namespace

extern "C" int Input_GetProfileCount(void)
{
    Bindings_Get();
    return (int)EditableProfiles().size();
}

extern "C" int Input_GetProfileKind(int index)
{
    Profile *profile = EditableProfile(index);
    if (profile == NULL || profile->section == "keyboard")
        return INPUT_PROFILE_KEYBOARD;
    if (profile->section == "keyboard2")
        return INPUT_PROFILE_KEYBOARD2;
    return profile->section == "gamepad" ? INPUT_PROFILE_GAMEPAD : INPUT_PROFILE_JOYSTICK;
}

extern "C" const char *Input_GetProfileName(int index)
{
    static std::string name;
    Profile *profile = EditableProfile(index);
    InputDevice *device = profile != NULL ? DeviceOf(profile) : NULL;
    name = device != NULL ? GameText(device->name.c_str()) : profile != NULL ? GameText(profile->name.c_str()) : "";
    return name.c_str();
}

extern "C" void Input_GetBindingText(int index, int action, char *text, int size)
{
    std::string names;
    Profile *profile = EditableProfile(index);
    if (profile != NULL && action >= 0 && action < INPUT_ACTION_COUNT)
        for (const Source &source : profile->actions[action])
            names += (names.empty() ? "" : " / ") + SourceName(source);
    SDL_strlcpy(text, names.c_str(), size);
}

extern "C" int Input_GetSetting(int index, int setting)
{
    Profile *profile = EditableProfile(index);
    if (profile == NULL)
        return 0;
    switch (setting) {
    case INPUT_SETTING_DEADZONE: return profile->deadzone;
    case INPUT_SETTING_CURVE: return profile->steeringCurve;
    default: return profile->rumble;
    }
}

extern "C" void Input_SetSetting(int index, int setting, int value)
{
    Profile *profile = EditableProfile(index);
    if (profile == NULL || Input_GetSetting(index, setting) == value)
        return;
    switch (setting) {
    case INPUT_SETTING_DEADZONE: profile->deadzone = SDL_clamp(value, 0, 99); break;
    case INPUT_SETTING_CURVE: profile->steeringCurve = SDL_clamp(value, 0, 100); break;
    default: profile->rumble = SDL_clamp(value, 0, 100); break;
    }
    Bindings_MarkChanged();
}

extern "C" void Input_RestoreDefaults(int index)
{
    Profile *profile = EditableProfile(index);
    if (profile == NULL)
        return;
    int player = profile->player;
    InputDevice *device = DeviceOf(profile);
    Bindings_SetDefaults(*profile, device != NULL ? device->joystick : NULL);
    profile->player = player;
    Bindings_MarkChanged();
}

extern "C" void Input_BeginCapture(int index, int action)
{
    s_capture = Capture();
    s_capture.profile = EditableProfile(index);
    s_capture.action = action;
    s_capture.start = SDL_GetTicks();
    if (s_capture.profile == NULL || IsKeyboardProfile(s_capture.profile))
        return;
    for (InputDevice *device : CaptureDevices())
        s_capture.axes.push_back(AxisState(device, s_capture.profile->section == "gamepad"));
}

extern "C" int Input_UpdateCapture(void)
{
    Capture &capture = s_capture;
    if (capture.profile == NULL)
        return -1;
    bool keyboard = IsKeyboardProfile(capture.profile);
    bool gamepadLayout = capture.profile->section == "gamepad";
    std::vector<InputDevice *> devices = CaptureDevices();
    if (SDL_GetTicks() - capture.start > 10000 || (!keyboard && KeyDown(SDL_SCANCODE_ESCAPE)) ||
        (!keyboard && devices.size() != capture.axes.size())) {
        capture.profile = NULL;
        return -1;
    }
    // The input that opened the capture is let go first.
    if (!capture.released) {
        bool held = keyboard ? AnyKeyDown() : false;
        for (size_t i = 0; i < devices.size(); i++)
            held = held || AnyInput(devices[i], capture.axes[i], gamepadLayout);
        if (keyboard)
            for (InputDevice &device : s_devices)
                held = held || (device.id != 0 && device.gamepad != NULL &&
                                SDL_GetGamepadButton(device.gamepad, SDL_GAMEPAD_BUTTON_SOUTH));
        capture.released = !held;
        return 0;
    }
    Source source;
    bool found = false;
    if (keyboard) {
        int count = 0;
        SDL_GetKeyboardState(&count);
        for (int scancode = 1; scancode < count && !found; scancode++) {
            if (KeyDown((SDL_Scancode)scancode)) {
                source = { SOURCE_KEY, scancode, 0 };
                found = true;
            }
        }
        // A controller's B gives up waiting for a key.
        for (InputDevice &device : s_devices) {
            if (!found && device.id != 0 && device.gamepad != NULL &&
                SDL_GetGamepadButton(device.gamepad, SDL_GAMEPAD_BUTTON_EAST)) {
                capture.profile = NULL;
                return -1;
            }
        }
    } else {
        for (size_t i = 0; i < devices.size() && !found; i++)
            found = FindInput(devices[i], capture.axes[i], gamepadLayout, &source);
    }
    if (!found)
        return 0;
    Assign(*capture.profile, capture.action, source);
    capture.profile = NULL;
    return 1;
}
