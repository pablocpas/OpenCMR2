// Bindings of the actions: defaults, and controls.ini in the user directory.

#include "input/input_internal.h"
#include "platform/platform.h"
#include "port/sys.h"

#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <sstream>

namespace {

const char *const s_actionNames[INPUT_ACTION_COUNT] = {
    "steer_left", "steer_right", "throttle", "brake", "handbrake", "shift_up", "shift_down", "camera",
    "look_back", "pause", "menu_left", "menu_right", "menu_up", "menu_down", "menu_confirm", "menu_back",
};

std::deque<Profile> s_profiles;
bool s_changed;
bool s_loaded;
bool s_fileExists;

const char *const s_header =
    "# OpenCMR2 controls\n"
    "#\n"
    "# Each action lists the inputs bound to it, separated by '|'. Keys use SDL's\n"
    "# names (Left, Space, Keypad 4, Return). Gamepads use SDL's layout: a, b, x, y,\n"
    "# back, start, leftshoulder, rightshoulder, leftstick, rightstick, dpup, dpdown,\n"
    "# dpleft, dpright, misc1, paddle1-4, touchpad, the stick halves leftx-, leftx+,\n"
    "# lefty-, lefty+, rightx-, ... and lefttrigger, righttrigger. Joysticks and\n"
    "# wheels use button<n>, hat<n>up/down/left/right, the axis halves axis<n>- and\n"
    "# axis<n>+, and for a pedal axis<n> (released at the axis minimum) or ~axis<n>\n"
    "# (released at its maximum).\n"
    "#\n"
    "# deadzone and saturation are percentages of an axis: the part left out around\n"
    "# its rest, and the part that gives the full value. player is 1 or 2 for a\n"
    "# device, or auto: the second controller connected plays as player 2 when two\n"
    "# players race. smooth_steering eases the steering towards the axis instead of\n"
    "# following it directly. steering_curve, 0 to 100, makes a steering axis\n"
    "# gentler around its centre for finer corrections, keeping full lock at its\n"
    "# end (0 follows the axis linearly, as the original). rumble is the strength\n"
    "# of a gamepad's vibration in percent (0 turns it off).\n"
    "#\n"
    "# [keyboard] is player 1's keys and [keyboard2] player 2's. [gamepad] applies\n"
    "# to every controller SDL knows the layout of, [joystick] is the default for\n"
    "# other joysticks and wheels, and a [device <GUID>] section applies to one\n"
    "# model of controller.\n";

std::string Trim(const std::string &text)
{
    size_t begin = text.find_first_not_of(" \t\r");
    size_t end = text.find_last_not_of(" \t\r");
    return begin == std::string::npos ? std::string() : text.substr(begin, end - begin + 1);
}

std::string Lower(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return (char)tolower(c); });
    return text;
}

bool IsKeyboard(const Profile &profile)
{
    return profile.section == "keyboard" || profile.section == "keyboard2";
}

bool IsGamepad(const Profile &profile)
{
    return profile.section == "gamepad";
}

bool ParseSource(const Profile &profile, const std::string &token, Source *out)
{
    if (IsKeyboard(profile)) {
        SDL_Scancode scancode = SDL_GetScancodeFromName(token.c_str());
        if (scancode == SDL_SCANCODE_UNKNOWN)
            return false;
        *out = { SOURCE_KEY, scancode, 0 };
        return true;
    }
    std::string name = Lower(token);
    // A device section of a known gamepad can use the gamepad names too.
    SDL_GamepadButton button = SDL_GetGamepadButtonFromString(name.c_str());
    if (button != SDL_GAMEPAD_BUTTON_INVALID) {
        *out = { SOURCE_BUTTON, button, 0 };
        return true;
    }
    std::string axisName = name;
    int range = AXIS_POSITIVE;
    if (!axisName.empty() && (axisName.back() == '-' || axisName.back() == '+')) {
        range = axisName.back() == '-' ? AXIS_NEGATIVE : AXIS_POSITIVE;
        axisName.pop_back();
    }
    SDL_GamepadAxis axis = SDL_GetGamepadAxisFromString(axisName.c_str());
    if (axis != SDL_GAMEPAD_AXIS_INVALID) {
        *out = { SOURCE_AXIS, axis, range };
        return true;
    }
    if (IsGamepad(profile))
        return false;
    int index = 0;
    int length = 0;
    if (sscanf(name.c_str(), "button%d%n", &index, &length) == 1 && length == (int)name.size() && index >= 0) {
        *out = { SOURCE_JOY_BUTTON, index, 0 };
        return true;
    }
    char direction[8] = {};
    if (sscanf(name.c_str(), "hat%d%7s", &index, direction) == 2 && index >= 0) {
        static const struct { const char *name; int bit; } hats[] = {
            { "up", SDL_HAT_UP }, { "right", SDL_HAT_RIGHT }, { "down", SDL_HAT_DOWN }, { "left", SDL_HAT_LEFT },
        };
        for (const auto &hat : hats) {
            if (strcmp(direction, hat.name) == 0) {
                *out = { SOURCE_JOY_HAT, index, hat.bit };
                return true;
            }
        }
        return false;
    }
    bool falling = name[0] == '~';
    const char *axisText = name.c_str() + (falling ? 1 : 0);
    if (sscanf(axisText, "axis%d%n", &index, &length) == 1 && index >= 0) {
        const char *rest = axisText + length;
        if (*rest == 0)
            range = falling ? AXIS_FALLING : AXIS_RISING;
        else if (!falling && strcmp(rest, "-") == 0)
            range = AXIS_NEGATIVE;
        else if (!falling && strcmp(rest, "+") == 0)
            range = AXIS_POSITIVE;
        else
            return false;
        *out = { SOURCE_JOY_AXIS, index, range };
        return true;
    }
    return false;
}

void Bind(Profile &profile, int action, const char *list)
{
    profile.actions[action].clear();
    std::stringstream tokens(list);
    std::string token;
    while (std::getline(tokens, token, '|')) {
        token = Trim(token);
        Source source;
        if (token.empty())
            continue;
        if (ParseSource(profile, token, &source))
            profile.actions[action].push_back(source);
        else
            Sys_Log("controls.ini: [%s] %s: unknown input '%s'", profile.section.c_str(), s_actionNames[action],
                    token.c_str());
    }
}

Profile MakeProfile(const char *section, const char *const bindings[INPUT_ACTION_COUNT])
{
    Profile profile;
    profile.section = section;
    for (int action = 0; action < INPUT_ACTION_COUNT; action++)
        Bind(profile, action, bindings[action]);
    return profile;
}

// The original's keys: cursor keys, Space, ] and [ for the gears, C and R;
// the second keyboard player has the numeric keypad.
const char *const s_keyboard[INPUT_ACTION_COUNT] = {
    "Left", "Right", "Up", "Down", "Space", "]", "[", "C", "R", "Escape",
    "Left", "Right", "Up", "Down", "Return | Keypad Enter", "Escape",
};
const char *const s_keyboard2[INPUT_ACTION_COUNT] = {
    "Keypad 4", "Keypad 6", "Keypad 8", "Keypad 2", "Keypad 3", "PageUp", "PageDown", "Keypad 9", "Keypad 7", "Escape",
    "Left", "Right", "Up", "Down", "Return | Keypad Enter", "Escape",
};
const char *const s_gamepad[INPUT_ACTION_COUNT] = {
    "leftx-", "leftx+", "righttrigger", "lefttrigger", "a", "rightshoulder", "leftshoulder", "y", "b",
    "start | back",
    "dpleft | leftx-", "dpright | leftx+", "dpup | lefty-", "dpdown | lefty+", "a", "b",
};
// The original's joystick defaults: the X axis steers, the Y axis is throttle
// and brake, buttons 1 to 6 for the other actions.
const char *const s_joystick[INPUT_ACTION_COUNT] = {
    "axis0-", "axis0+", "axis1-", "axis1+", "button0", "button1", "button2", "button3", "button4", "button5",
    "hat0left | axis0-", "hat0right | axis0+", "hat0up | axis1-", "hat0down | axis1+", "button0", "button1",
};

void SetDefaults(void)
{
    s_profiles.clear();
    for (const char *section : { "keyboard", "keyboard2", "gamepad", "joystick" }) {
        s_profiles.emplace_back();
        s_profiles.back().section = section;
        Bindings_SetDefaults(s_profiles.back(), NULL);
    }
}

// Empty without a user directory (tests): the defaults, nothing written.
std::string Path(void)
{
    const char *dir = Platform_GetUserDir();
    return dir && *dir ? std::string(dir) + "controls.ini" : std::string();
}

void Read(const char *text)
{
    std::stringstream lines(text);
    std::string line;
    Profile *profile = NULL;
    while (std::getline(lines, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';')
            continue;
        if (line[0] == '[') {
            size_t end = line.find(']');
            std::string section = Trim(line.substr(1, end == std::string::npos ? std::string::npos : end - 1));
            profile = Bindings_Find(section);
            if (profile == NULL && section.compare(0, 7, "device ") == 0) {
                // Starts from the joystick defaults, as a new device would.
                s_profiles.push_back(*Bindings_Find("joystick"));
                profile = &s_profiles.back();
                profile->section = section;
            }
            if (profile == NULL)
                Sys_Log("controls.ini: unknown section [%s]", section.c_str());
            continue;
        }
        size_t equals = line.find('=');
        if (profile == NULL || equals == std::string::npos)
            continue;
        std::string key = Lower(Trim(line.substr(0, equals)));
        std::string value = Trim(line.substr(equals + 1));
        int action;
        for (action = 0; action < INPUT_ACTION_COUNT; action++)
            if (key == s_actionNames[action])
                break;
        if (action < INPUT_ACTION_COUNT)
            Bind(*profile, action, value.c_str());
        else if (key == "name")
            profile->name = value;
        else if (key == "deadzone")
            profile->deadzone = SDL_clamp(SDL_atoi(value.c_str()), 0, 99);
        else if (key == "saturation")
            profile->saturation = SDL_clamp(SDL_atoi(value.c_str()), 1, 100);
        else if (key == "player")
            profile->player = value == "1" ? 1 : value == "2" ? 2 : 0;
        else if (key == "smooth_steering")
            profile->smoothSteering = SDL_atoi(value.c_str()) != 0;
        else if (key == "steering_curve")
            profile->steeringCurve = SDL_clamp(SDL_atoi(value.c_str()), 0, 100);
        else if (key == "rumble")
            profile->rumble = SDL_clamp(SDL_atoi(value.c_str()), 0, 100);
        else
            Sys_Log("controls.ini: [%s] unknown setting '%s'", profile->section.c_str(), key.c_str());
    }
}

std::string Write(void)
{
    std::string text = s_header;
    for (const Profile &profile : s_profiles) {
        text += "\n[" + profile.section + "]\n";
        if (!profile.name.empty())
            text += "name = " + profile.name + "\n";
        for (int action = 0; action < INPUT_ACTION_COUNT; action++) {
            std::string list;
            for (const Source &source : profile.actions[action])
                list += (list.empty() ? "" : " | ") + FormatSource(profile, source);
            text += std::string(s_actionNames[action]) + " = " + list + "\n";
        }
        if (!IsKeyboard(profile)) {
            text += "deadzone = " + std::to_string(profile.deadzone) + "\n";
            text += "saturation = " + std::to_string(profile.saturation) + "\n";
            text += "smooth_steering = " + std::to_string(profile.smoothSteering ? 1 : 0) + "\n";
            text += "steering_curve = " + std::to_string(profile.steeringCurve) + "\n";
            text += "rumble = " + std::to_string(profile.rumble) + "\n";
            if (profile.section.compare(0, 7, "device ") == 0)
                text += std::string("player = ") + (profile.player ? std::to_string(profile.player) : "auto") + "\n";
        }
    }
    return text;
}

} // namespace

bool IsAnalog(const Source &source)
{
    return source.kind == SOURCE_AXIS || source.kind == SOURCE_JOY_AXIS;
}

std::string FormatSource(const Profile &profile, const Source &source)
{
    switch (source.kind) {
    case SOURCE_KEY: {
        const char *name = SDL_GetScancodeName((SDL_Scancode)source.index);
        return name ? name : "";
    }
    case SOURCE_BUTTON: {
        const char *name = SDL_GetGamepadStringForButton((SDL_GamepadButton)source.index);
        return name ? name : "";
    }
    case SOURCE_AXIS: {
        const char *name = SDL_GetGamepadStringForAxis((SDL_GamepadAxis)source.index);
        std::string text = name ? name : "";
        if (source.index == SDL_GAMEPAD_AXIS_LEFT_TRIGGER || source.index == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)
            return text;
        return text + (source.direction == AXIS_NEGATIVE ? "-" : "+");
    }
    case SOURCE_JOY_BUTTON:
        return "button" + std::to_string(source.index);
    case SOURCE_JOY_HAT: {
        const char *direction = source.direction == SDL_HAT_UP ? "up" : source.direction == SDL_HAT_RIGHT ? "right"
                              : source.direction == SDL_HAT_DOWN ? "down" : "left";
        return "hat" + std::to_string(source.index) + direction;
    }
    case SOURCE_JOY_AXIS: {
        std::string axis = "axis" + std::to_string(source.index);
        switch (source.direction) {
        case AXIS_NEGATIVE: return axis + "-";
        case AXIS_POSITIVE: return axis + "+";
        case AXIS_FALLING: return "~" + axis;
        default: return axis;
        }
    }
    }
    (void)profile;
    return "";
}

std::deque<Profile> &Bindings_Get(void)
{
    if (!s_loaded)
        Bindings_Load();
    return s_profiles;
}

Profile *Bindings_Find(const std::string &section)
{
    for (Profile &profile : s_profiles)
        if (profile.section == section)
            return &profile;
    return NULL;
}

void Bindings_Load(void)
{
    s_loaded = true;
    SetDefaults();
    size_t size = 0;
    std::string path = Path();
    char *text = path.empty() ? NULL : (char *)SDL_LoadFile(path.c_str(), &size);
    s_fileExists = text != NULL;
    if (text == NULL) {
        s_changed = true;   // written with the defaults
        return;
    }
    Read(text);
    SDL_free(text);
    s_changed = false;
}

void Bindings_MarkChanged(void)
{
    s_changed = true;
}

namespace {

// A joystick's: the joystick section, and a wheel's pedals, the axes that
// rest at one end; throttle and brake stay on the Y axis otherwise.
void JoystickDefaults(Profile &profile, SDL_Joystick *joystick)
{
    std::string section = profile.section;
    profile = *Bindings_Find("joystick");
    profile.section = section;
    const char *name = SDL_GetJoystickName(joystick);
    profile.name = name ? name : "";
    std::vector<Source> pedals;
    const Sint16 rest = 30000;
    for (int axis = 1; axis < SDL_GetNumJoystickAxes(joystick) && pedals.size() < 2; axis++) {
        Sint16 state = 0;
        if (!SDL_GetJoystickAxisInitialState(joystick, axis, &state))
            continue;
        if (state <= -rest)
            pedals.push_back({ SOURCE_JOY_AXIS, axis, AXIS_RISING });
        else if (state >= rest)
            pedals.push_back({ SOURCE_JOY_AXIS, axis, AXIS_FALLING });
    }
    if (pedals.size() == 2) {
        profile.actions[INPUT_THROTTLE] = { pedals[0] };
        profile.actions[INPUT_BRAKE] = { pedals[1] };
    }
    if (SDL_GetJoystickType(joystick) == SDL_JOYSTICK_TYPE_WHEEL)
        profile.deadzone = 0;
}

} // namespace

void Bindings_SetDefaults(Profile &profile, SDL_Joystick *joystick)
{
    static const struct { const char *section; const char *const *bindings; } defaults[] = {
        { "keyboard", s_keyboard }, { "keyboard2", s_keyboard2 }, { "gamepad", s_gamepad }, { "joystick", s_joystick },
    };
    for (const auto &entry : defaults) {
        if (profile.section == entry.section) {
            profile = MakeProfile(entry.section, entry.bindings);
            // A stick is short: followed directly, the wheels jump to its
            // angle in one physics step. Eased at the car setup's steering
            // speed, a gamepad turns them about as fast as the keys do. A
            // wheel is followed directly, as the original did.
            profile.smoothSteering = profile.section == "gamepad";
#ifdef __ANDROID__
            // Handhelds: Start pauses, Back follows the system.
            if (profile.section == "gamepad")
                Bind(profile, INPUT_PAUSE, "start");
#endif
            return;
        }
    }
    if (joystick != NULL)
        JoystickDefaults(profile, joystick);
}

const Profile &Bindings_TouchProfile(void)
{
    static Profile profile = MakeProfile("keyboard", s_keyboard);
    return profile;
}

Profile *Bindings_ForJoystick(SDL_Joystick *joystick, bool gamepad)
{
    Bindings_Get();
    char guid[33];
    SDL_GUIDToString(SDL_GetJoystickGUID(joystick), guid, sizeof(guid));
    std::string section = std::string("device ") + guid;
    if (Profile *profile = Bindings_Find(section))
        return profile;
    if (gamepad)
        return Bindings_Find("gamepad");

    Profile profile;
    profile.section = section;
    JoystickDefaults(profile, joystick);
    s_profiles.push_back(profile);
    s_changed = true;
    Sys_Log("controls: new profile for %s (%s), throttle %s, brake %s", profile.name.c_str(), guid,
            FormatSource(profile, profile.actions[INPUT_THROTTLE][0]).c_str(),
            FormatSource(profile, profile.actions[INPUT_BRAKE][0]).c_str());
    return &s_profiles.back();
}

extern "C" void Input_ImportKeyboardKeys(int player, const BYTE keys[9])
{
    Bindings_Get();
    if (s_fileExists || player < 0 || player > 1)
        return;
    Profile *profile = Bindings_Find(player == 0 ? "keyboard" : "keyboard2");
    for (int action = 0; action < 9; action++) {
        SDL_Scancode scancode = Keys_ToScancode(keys[action]);
        if (scancode != SDL_SCANCODE_UNKNOWN)
            profile->actions[action] = { { SOURCE_KEY, scancode, 0 } };
    }
    s_changed = true;
}

extern "C" void Input_SaveBindings(void)
{
    if (!s_loaded || !s_changed || Path().empty())
        return;
    std::string text = Write();
    if (!SDL_SaveFile(Path().c_str(), text.data(), text.size())) {
        Sys_Log("controls.ini: cannot write: %s", SDL_GetError());
        return;
    }
    s_changed = false;
    s_fileExists = true;
}
