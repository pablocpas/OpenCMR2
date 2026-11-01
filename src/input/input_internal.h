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
    AXIS_NEGATIVE,          // the half below the centre
    AXIS_POSITIVE,          // the half above it (a gamepad trigger)
    AXIS_RISING,            // the whole axis, released at its minimum (a pedal)
    AXIS_FALLING,           // the whole axis, released at its maximum
};

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
    bool smoothSteering = false;    // the game eases the wheel towards the axis
    int rumble = 100;               // percent of the gamepad's rumble
    int steeringCurve = 0;          // percent of cubic response of the steering axes
};

bool IsAnalog(const Source &source);
std::string FormatSource(const Profile &profile, const Source &source);

// controls.ini: defaults, reading and writing.
std::deque<Profile> &Bindings_Get(void);
Profile *Bindings_Find(const std::string &section);
// The profile of a joystick or wheel without one: SDL's layout of a known
// gamepad, else the joystick defaults with its pedals found from where its
// axes rest.
Profile *Bindings_ForJoystick(SDL_Joystick *joystick, bool gamepad);
void Bindings_Load(void);
void Bindings_MarkChanged(void);
// The defaults of a profile, from the device for a joystick's.
void Bindings_SetDefaults(Profile &profile, SDL_Joystick *joystick);
// Player 1's default keys: the keys Android's touch controls press, which
// keep working whatever the keyboard is bound to.
const Profile &Bindings_TouchProfile(void);

#endif
