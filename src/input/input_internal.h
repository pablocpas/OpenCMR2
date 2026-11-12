// Shared by the input sources (input_sdl.cpp) and the bindings (bindings.cpp).
#ifndef OPENCMR2_INPUT_INTERNAL_H
#define OPENCMR2_INPUT_INTERNAL_H

#include "port/input.h"

#include <SDL3/SDL.h>

#include <deque>
#include <string>
#include <vector>

// One input an action is bound to.
enum SourceKind {
    SOURCE_KEY,             // index: SDL_Scancode
    SOURCE_BUTTON,          // index: SDL_GamepadButton
    SOURCE_AXIS,            // index: SDL_GamepadAxis
    SOURCE_JOY_BUTTON,      // index: joystick button
    SOURCE_JOY_AXIS,        // index: joystick axis
    SOURCE_JOY_HAT,         // index: hat, direction: SDL_HAT_UP/RIGHT/DOWN/LEFT
};

// How an axis gives an action its value.
enum AxisRange {
    AXIS_NEGATIVE,          // from where the axis rests towards its minimum
    AXIS_POSITIVE,          // from where it rests towards its maximum (a trigger)
    AXIS_RISING,            // the whole axis, released at its minimum (a pedal)
    AXIS_FALLING,           // the whole axis, released at its maximum
};

// Where a joystick's axis rests, snapped to its centre or one of its ends:
// a pedal, a clutch or a throttle lever rests at an end. Without a reading
// yet, nothing is known.
enum AxisRest {
    REST_UNKNOWN,
    REST_CENTRE,
    REST_MIN,
    REST_MAX,
};
AxisRest Joystick_AxisRest(SDL_Joystick *joystick, int axis);

struct Source {
    SourceKind kind;
    int index;
    int direction;          // AxisRange, or the hat direction
};

// Bindings of one kind of device: the keyboard players, gamepads in SDL's
// layout, and joysticks and wheels by model.
struct Profile {
    std::string section;            // "keyboard", "keyboard2", "gamepad", "joystick", "device <GUID>"
    std::string name;               // the device's name, for a device section
    std::vector<Source> actions[INPUT_ACTION_COUNT];
    int deadzone = 2;               // percent of the axis left out around its rest
    int saturation = 100;           // percent of the axis that gives the full value
    int player = 0;                 // 1 or 2; 0 follows the connection order
    int rumble = 100;               // percent of the gamepad's rumble
    int steeringCurve = 0;          // percent of cubic response of the steering axes
    int force = 100;                // percent of the game's force feedback (the original's at 100)
    bool enabled = true;            // a device section: false leaves the device out
    unsigned chosen = 0;            // SETTING_* bits set by the user (written to controls.ini)
    // The keys' actions held at each player's previous update (keyboard
    // profiles), for telling when one is pressed.
    mutable bool keyActive[INPUT_PLAYERS][INPUT_ACTION_COUNT] = {};
};

// Settings of a profile; those not chosen follow the defaults of its kind.
enum {
    SETTING_DEADZONE = 1,
    SETTING_SATURATION = 2,
    SETTING_CURVE = 4,
    SETTING_RUMBLE = 8,
    SETTING_FORCE = 16,
};

bool IsAnalog(const Source &source);
std::string FormatSource(const Profile &profile, const Source &source);

// controls.ini: defaults, reading and writing.
std::deque<Profile> &Bindings_Get(void);
Profile *Bindings_Find(const std::string &section);
// The profile of a joystick or wheel: SDL's layout of a known gamepad, else
// its device section, made from where its axes rest when it has none
// (*created set).
Profile *Bindings_ForJoystick(SDL_Joystick *joystick, bool gamepad, bool *created);
void Bindings_Load(void);
// Writes the driving bindings of a profile to the log.
void Bindings_LogProfile(const Profile &profile, const char *what);
void Bindings_MarkChanged(void);
// The defaults of a profile, from the device for a joystick's.
void Bindings_SetDefaults(Profile &profile, SDL_Joystick *joystick);
// The defaults of the settings not chosen: for a device section, from the
// device (NULL: as the joystick defaults).
void Bindings_DefaultSettings(Profile &profile, SDL_Joystick *joystick);
// Player 1's default keys: the keys Android's touch controls press, which
// keep working whatever the keyboard is bound to.
const Profile &Bindings_TouchProfile(void);

#endif
