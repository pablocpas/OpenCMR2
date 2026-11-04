// The players' actions (src/input) and the device records the game reads
// them through (game/Input.cpp). Uses SDL virtual devices, so no hardware or
// display is needed. The game window never has the keyboard focus here, as
// under gamescope: controllers must still be read.

#include "port/types.h"
#include "port/input.h"
#include "GameInfo.h"
#include "Input.h"
#include "Menu.h"
#include "platform/platform.h"
#include "platform/testing.h"
#include "input/input_internal.h"
#include "main.h"
#include "Car.h"

#include <SDL3/SDL.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

extern void Input_MergeAssignedJoystickButtons(int slot, DeviceInfo *out);
extern void Input_RescanChangedJoysticks();
extern BYTE g_carSimulationClockFlags;
extern int Car_UpdateEngineNoteFalloff(int *);
extern unsigned int g_selectedRallyData;
extern Menu *g_pMenu0x00818ac0;
extern void FrontendMenu_UpdateAndSwitchActive();
extern void FrontendMenu_BuildInputControls();
extern Menu *FrontendMenu_GetInputControls();
extern void FrontendMenu_EnterControls(Menu *, int);
bool Input_ApplyGamepadMapping(SDL_JoystickID id, const char *overrides);

namespace {

int s_failures;
BYTE s_keys[256];

void Check(bool ok, const char *what)
{
    if (!ok) {
        fprintf(stderr, "FAIL: %s\n", what);
        s_failures++;
    }
}

const BYTE kRightBracket = 0x1b, kLeftBracket = 0x1a, kC = 0x2e;

struct Pad {
    SDL_JoystickID id;
    SDL_Joystick *joystick;
};

Pad AttachPad(const char *name, Uint16 product)
{
    SDL_VirtualJoystickDesc desc;
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.vendor_id = 0x045e;
    desc.product_id = product;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.name = name;
    Pad pad;
    pad.id = SDL_AttachVirtualJoystick(&desc);
    pad.joystick = SDL_OpenJoystick(pad.id);
    // Released triggers rest at their minimum, as on real controllers.
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, SDL_JOYSTICK_AXIS_MIN);
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, SDL_JOYSTICK_AXIS_MIN);
    SDL_UpdateJoysticks();
    return pad;
}

void Detach(Pad &pad)
{
    SDL_CloseJoystick(pad.joystick);
    SDL_DetachVirtualJoystick(pad.id);
}

void Release(Pad &pad)
{
    for (int i = 0; i < SDL_GAMEPAD_AXIS_COUNT; ++i)
        SDL_SetJoystickVirtualAxis(pad.joystick, i, i >= SDL_GAMEPAD_AXIS_LEFT_TRIGGER ? SDL_JOYSTICK_AXIS_MIN : 0);
    for (int i = 0; i < SDL_GAMEPAD_BUTTON_COUNT; ++i)
        SDL_SetJoystickVirtualButton(pad.joystick, i, false);
}

// The game's input setup (CGameInfo::SetupInputs without the mouse, which
// needs the renderer) and its configurations.
void SetupGameInput()
{
    CInput::m_deviceCounts.nextDevice = 0;
    CInput::m_deviceCounts.keyboardCount = 2;
    CInput::m_deviceCounts.joystickCount = 0;
    for (DeviceInfo &device : CInput::m_availableDevices) {
        device.type = (DWORD)-1;
        device.joystickIndex = -1;
    }
    CInput::SetupKeyboard();
    CInput::GetAttachedJoysticks();
    CInput::RefreshControllerConfigurations();
}

DeviceInfo &Device(int index)
{
    return CInput::m_availableDevices[index];
}

void MenuFrame()
{
    CGameInfo::m_fixedMenuKeys = 1;
    CInput::UpdateAllAvailableDevices();
}

void RaceFrame()
{
    CGameInfo::m_fixedMenuKeys = 0;
    CInput::UpdateAllAvailableDevices();
}

void CheckGamepadActions(Pad &pad)
{
    Release(pad);
    Input_Update();
    Check(Input_GetAction(0, INPUT_THROTTLE) == 0 && Input_GetAction(0, INPUT_BRAKE) == 0, "released triggers");
    Check(Input_GetAction(0, INPUT_STEER_LEFT) == 0 && Input_GetAction(0, INPUT_STEER_RIGHT) == 0, "centred stick");

    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, SDL_JOYSTICK_AXIS_MAX);
    Input_Update();
    Check(Input_GetAction(0, INPUT_THROTTLE) == INPUT_FULL && Input_GetAction(0, INPUT_BRAKE) == 0,
          "the right trigger is the throttle alone");
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, SDL_JOYSTICK_AXIS_MAX);
    Input_Update();
    Check(Input_GetAction(0, INPUT_THROTTLE) == INPUT_FULL && Input_GetAction(0, INPUT_BRAKE) == INPUT_FULL,
          "throttle and brake held together");

    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFTX, SDL_JOYSTICK_AXIS_MIN);
    Input_Update();
    Check(Input_GetAction(0, INPUT_STEER_LEFT) == INPUT_FULL && Input_GetAction(0, INPUT_STEER_RIGHT) == 0,
          "full left");
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFTX, 16384);
    Input_Update();
    int right = Input_GetAction(0, INPUT_STEER_RIGHT);
    Check(right > INPUT_FULL * 50 / 100 && right < INPUT_FULL * 53 / 100, "half right, less the deadzones");
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFTX, 31500);
    Input_Update();
    Check(Input_GetAction(0, INPUT_STEER_RIGHT) == INPUT_FULL, "96% of the stick is full lock");
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFTX, 23170);
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFTY, -23170);
    Input_Update();
    Check(Input_GetAction(0, INPUT_STEER_RIGHT) == INPUT_FULL, "the edge of the gate on a diagonal is full lock");
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFTX, 8000);
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFTY, -1000);
    Input_Update();
    right = Input_GetAction(0, INPUT_STEER_RIGHT);
    Check(right > INPUT_FULL * 23 / 100 && right < INPUT_FULL * 25 / 100, "near the axis the response is unchanged");
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFTY, 0);
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFTX, 16384);
    Input_Update();
    right = Input_GetAction(0, INPUT_STEER_RIGHT);
    Bindings_Find("gamepad")->steeringCurve = 100;
    Input_Update();
    int curved = Input_GetAction(0, INPUT_STEER_RIGHT);
    Check(curved > right / 5 && curved < right / 3, "a cubic curve is gentler half way");
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFTX, SDL_JOYSTICK_AXIS_MAX);
    Input_Update();
    Check(Input_GetAction(0, INPUT_STEER_RIGHT) == INPUT_FULL, "and still reaches full lock");
    Bindings_Find("gamepad")->steeringCurve = 0;
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFTX, 300);
    Input_Update();
    Check(Input_GetAction(0, INPUT_STEER_RIGHT) == 0, "inside the deadzone");

    SDL_SetJoystickVirtualButton(pad.joystick, SDL_GAMEPAD_BUTTON_SOUTH, true);
    SDL_SetJoystickVirtualButton(pad.joystick, SDL_GAMEPAD_BUTTON_START, true);
    Input_Update();
    Check(Input_GetAction(0, INPUT_HANDBRAKE) == INPUT_FULL, "A is the handbrake");
    Check(Input_GetAction(0, INPUT_MENU_CONFIRM) == INPUT_FULL, "A confirms");
    Check(Input_GetAction(0, INPUT_PAUSE) == INPUT_FULL, "Start pauses");
    Release(pad);
    Input_Update();
}

// The records the game reads: menu bits in the menus, actions on a stage,
// the axes of the player's joystick.
void CheckRecords(Pad &pad)
{
    Release(pad);
    MenuFrame();
    Check(CInput::m_playerControllers[0] == 2 && CInput::m_playerControllers[1] == 3, "players drive with records 2, 3");
    Check(Device(2).type == 3, "a player with a stick drives with a joystick");

    const struct { SDL_GamepadButton button; unsigned bit; } buttons[] = {
        {SDL_GAMEPAD_BUTTON_DPAD_UP, 4}, {SDL_GAMEPAD_BUTTON_DPAD_DOWN, 8}, {SDL_GAMEPAD_BUTTON_DPAD_LEFT, 1},
        {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, 2}, {SDL_GAMEPAD_BUTTON_SOUTH, 0x10}, {SDL_GAMEPAD_BUTTON_EAST, 0x20},
    };
    for (const auto &press : buttons) {
        SDL_SetJoystickVirtualButton(pad.joystick, press.button, true);
        MenuFrame();
        Check((Device(0).pressed & 0x3f) == press.bit, "menu buttons reach player 1's menus");
        SDL_SetJoystickVirtualButton(pad.joystick, press.button, false);
        MenuFrame();
    }
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFTY, SDL_JOYSTICK_AXIS_MIN);
    MenuFrame();
    Check((Device(0).pressed & 0xf) == 4, "the stick navigates");
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFTY, 0);
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, SDL_JOYSTICK_AXIS_MAX);
    MenuFrame();
    Check((Device(0).pressed & 0x3f) == 0, "a trigger does not navigate");
    // Both triggers held: still no navigation, the driving pedal positions
    // are preserved, and the stick navigates on top of them.
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, SDL_JOYSTICK_AXIS_MAX);
    MenuFrame();
    Check((Device(0).pressed & 0xf) == 0, "both triggers do not navigate");
    Check(Device(2).joystick.bindings[5].position == -0x10000 && Device(2).joystick.bindings[2].position == -0x10000,
          "menu polling preserves both driving pedals");
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFTY, SDL_JOYSTICK_AXIS_MIN);
    MenuFrame();
    Check((Device(0).pressed & 0xf) == 4, "held triggers do not swallow stick navigation");
    Release(pad);
    MenuFrame();

    // A real menu follows them.
    Menu menu{};
    Menu_Init(&menu, nullptr, 0, 0, nullptr, nullptr, 1, 1, 1);
    for (int i = 0; i < 3; ++i)
        Menu_AddItemType1(&menu, nullptr, -1, nullptr, short(i));
    Menu_SetInputStateFlag(0); // no audio resources in this test
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFTY, SDL_JOYSTICK_AXIS_MIN);
    MenuFrame();
    Menu_Update(&menu, Device(0).pressed);
    Check(menu.cursor == 0, "the stick moves a menu's cursor up");
    Release(pad);
    MenuFrame();
    SDL_SetJoystickVirtualButton(pad.joystick, SDL_GAMEPAD_BUTTON_DPAD_DOWN, true);
    MenuFrame();
    Menu_Update(&menu, Device(0).pressed);
    Check(menu.cursor == 1, "the D-pad moves it down");
    Release(pad);
    MenuFrame();

    // On a stage: bit 1 << action, the pause with Escape's bit.
    SDL_SetJoystickVirtualButton(pad.joystick, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, true);
    SDL_SetJoystickVirtualButton(pad.joystick, SDL_GAMEPAD_BUTTON_START, true);
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFTX, SDL_JOYSTICK_AXIS_MIN);
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, SDL_JOYSTICK_AXIS_MAX);
    RaceFrame();
    Check((Device(0).held & 0x620) == 0x620, "shift up and pause on a stage");
    Check((Device(2).held & 0x620) == 0x620, "the joystick record has the buttons too");
    Check(Device(2).joystick.bindings[0].position == -0x10000, "steering on X");
    Check(Device(2).joystick.bindings[5].position == -0x10000, "throttle on Rz, pressed fully at the minimum");
    Check(Device(2).joystick.bindings[2].position == 0x10000, "brake on Z, released at the maximum");
    Check(CInput::GetEnabledControllerAxisBinding(0, 0) == 0 && CInput::GetEnabledControllerAxisBinding(0, 2) == 5 &&
          CInput::GetEnabledControllerAxisBinding(0, 3) == 2, "the configuration names those axes");
    Check((unsigned short)CInput::GetButtonMapping(0, 5) == 0x20, "the configuration names bit 1 << action");
    // Held into the pause menu, Start is no new press there; pressed again,
    // it is the pause's bit that closes the in-race menu.
    MenuFrame();
    Check((Device(2).pressed & 0x200) == 0, "a held Start does not press again in the pause menu");
    SDL_SetJoystickVirtualButton(pad.joystick, SDL_GAMEPAD_BUTTON_START, false);
    MenuFrame();
    SDL_SetJoystickVirtualButton(pad.joystick, SDL_GAMEPAD_BUTTON_START, true);
    MenuFrame();
    Check((Device(0).pressed & 0x200) != 0 && (Device(0).pressed & 0x3f) == 0, "Start again gives the pause's bit alone");
    Release(pad);
    RaceFrame();

    // Keys switch the player to driving as with a keyboard; the stick back.
    s_keys[INPUT_KEY_LEFT] = 0x80;
    RaceFrame();
    Check(CInput::m_controllerInfo[2].analogSteering == 0, "keys steer as a keyboard");
    Check((Device(2).held & 1) != 0, "with bit 1");
    s_keys[INPUT_KEY_UP] = 0x80;
    RaceFrame();
    Check(Device(2).type == 1, "keys alone drive as a keyboard");
    s_keys[INPUT_KEY_LEFT] = s_keys[INPUT_KEY_UP] = 0;
    RaceFrame();
    Check(Device(2).type == 1, "until an axis is used");
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFTX, SDL_JOYSTICK_AXIS_MAX);
    RaceFrame();
    Check(Device(2).type == 3, "the stick drives as a joystick again");
    Release(pad);
    RaceFrame();
}

void CheckRacePresses(Pad &pad)
{
    const unsigned actionBits = 0x20 | 0x40 | 0x80 | 0x400;
    const unsigned padBit = 1u << 2;
    memset(s_keys, 0, sizeof(s_keys));
    Release(pad);
    CGameInfo::m_fixedMenuKeys = 0;
    CInput::UpdateAllAvailableDevices();

    // The real mixed-rate scheduler supplies opponent-only steps, as on a
    // super special. Inject input exactly on those steps at common frame rates.
    Car cars[2] = {};
    Car *oldBuffer = g_carBuffer;
    const short oldCount = g_carOrderCount;
    short oldOrder[2] = {g_carOrder[0], g_carOrder[1]};
    g_carBuffer = cars;
    g_carOrderCount = 2;
    g_carOrder[0] = 0; g_carOrder[1] = 1;
    for (unsigned fps : {30u, 60u, 120u, 144u, 240u}) {
        memset(cars, 0, sizeof(cars));
        cars[0].simulationRateHz = 25.f;
        cars[1].simulationRateHz = 50.f;
        g_carSimulationClockFlags = 0;
        CMain::m_frameTime = 1000;
        int phase;
        Car_UpdateEngineNoteFalloff(&phase);
        memset(s_keys, 0, sizeof(s_keys));
        SDL_SetJoystickVirtualButton(pad.joystick, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, false);
        CInput::UpdateAllAvailableDevices();
        int injected = 0, delivered = 0;
        bool armed = true, pending = false;
        for (unsigned frame = 1; frame <= fps * 3; ++frame) {
            CMain::m_frameTime = 1000 + frame * 1000 / fps;
            int steps = Car_UpdateEngineNoteFalloff(&phase);
            for (int step = 0; step < steps; ++step) {
                bool playerStep = cars[0].simulationStepsRemaining > 0;
                bool press = !playerStep && armed;
                s_keys[kRightBracket] = s_keys[kLeftBracket] = s_keys[kC] = s_keys[INPUT_KEY_ESCAPE] = press ? 0x80 : 0;
                SDL_SetJoystickVirtualButton(pad.joystick, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, press);
                if (press) { ++injected; armed = false; pending = true; }
                CInput::UpdateRaceDevices(playerStep ? 1u | padBit : 0u);
                unsigned keyboard = Device(0).pressed & actionBits;
                unsigned joystick = Device(2).pressed & 0x20;
                if (!playerStep) Check(keyboard == 0 && joystick == 0, "opponent does not consume player presses");
                else {
                    Check(keyboard == (pending ? actionBits : 0), "gear/camera/pause edges reach player exactly once");
                    Check(joystick == (pending ? 0x20u : 0), "joystick edge reaches player after release");
                    if (pending) { ++delivered; pending = false; }
                    armed = true;
                }
                for (Car &car : cars) if (car.simulationStepsRemaining > 0) --car.simulationStepsRemaining;
            }
        }
        Check(injected > 0 && delivered >= injected - 1, "mixed-rate scheduler exercises lost-input path");
    }
    // Holding a button must not repeatedly shift gears.
    memset(s_keys, 0, sizeof(s_keys));
    Release(pad);
    CInput::UpdateAllAvailableDevices();
    s_keys[kRightBracket] = 0x80;
    CInput::UpdateRaceDevices(1);
    Check((Device(0).pressed & 0x20) != 0, "a held gear key shifts once");
    for (int i = 0; i < 10; ++i) {
        CInput::UpdateRaceDevices(1);
        Check((Device(0).pressed & 0x20) == 0, "held gear keys never repeat");
    }
    // Two players: each device keeps its edges until its own player runs,
    // and holding a button repeats for neither of them.
    memset(s_keys, 0, sizeof(s_keys));
    Release(pad);
    g_selectedRallyData = 2 << 14;   // two local players: device 1 is player 2's
    CInput::UpdateAllAvailableDevices();
    // Player 2's keyboard2 shift-up is PageUp (0xc9) by default; device 1's
    // record table is unset here, the bindings layer owns the keys now.
    s_keys[kRightBracket] = s_keys[0xc9] = 0x80;
    CInput::UpdateRaceDevices(0);
    CInput::UpdateRaceDevices(1);
    Check((Device(0).pressed & 0x20) != 0 && (Device(1).pressed & 0x20) == 0,
          "player one does not consume player two presses");
    CInput::UpdateRaceDevices(2);
    Check((Device(1).pressed & 0x20) != 0, "player two receives the deferred press");
    for (int i = 0; i < 10; ++i) {
        CInput::UpdateRaceDevices(3);
        Check(((Device(0).pressed | Device(1).pressed) & 0x20) == 0,
              "held gear buttons never repeat for either player");
    }
    g_selectedRallyData = 0;
    // Pending actions cannot leak out of a race into a frontend/pause menu.
    memset(s_keys, 0, sizeof(s_keys));
    CInput::UpdateAllAvailableDevices();
    s_keys[kRightBracket] = 0x80;
    CInput::UpdateRaceDevices(0);
    CInput::UpdateAllAvailableDevices();
    s_keys[kRightBracket] = 0;
    CInput::UpdateRaceDevices(1);
    Check((Device(0).pressed & 0x20) == 0, "frontend clears pending race actions");
    g_carBuffer = oldBuffer;
    g_carOrderCount = oldCount;
    g_carOrder[0] = oldOrder[0]; g_carOrder[1] = oldOrder[1];
}

// Steering on the D-pad with the throttle on a trigger: the steering is read
// as keys (no axis, the car's progressive steering), the pedals as axes.
void CheckMixedChannels(Pad &pad)
{
    Profile &gamepad = *Bindings_Find("gamepad");
    std::vector<Source> left = gamepad.actions[INPUT_STEER_LEFT];
    std::vector<Source> right = gamepad.actions[INPUT_STEER_RIGHT];
    gamepad.actions[INPUT_STEER_LEFT] = { { SOURCE_BUTTON, SDL_GAMEPAD_BUTTON_DPAD_LEFT, 0 } };
    gamepad.actions[INPUT_STEER_RIGHT] = { { SOURCE_BUTTON, SDL_GAMEPAD_BUTTON_DPAD_RIGHT, 0 } };
    Release(pad);
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, SDL_JOYSTICK_AXIS_MAX);
    RaceFrame();
    SDL_SetJoystickVirtualButton(pad.joystick, SDL_GAMEPAD_BUTTON_DPAD_LEFT, true);
    RaceFrame();
    RaceFrame();
    Check(Device(2).type == 3, "the trigger keeps the joystick record");
    Check(CInput::m_controllerInfo[2].analogSteering == 0 && CInput::m_controllerInfo[2].analogPedals != 0,
          "steering from buttons, pedals from axes");
    Check((Device(2).held & 1) != 0 && Device(2).joystick.bindings[5].position == -0x10000,
          "the D-pad steers with bit 1, the trigger is a full pedal");
    SDL_SetJoystickVirtualButton(pad.joystick, SDL_GAMEPAD_BUTTON_DPAD_LEFT, false);
    RaceFrame();
    Check(CInput::m_controllerInfo[2].analogSteering == 0, "still steering with buttons while only the trigger moves");
    gamepad.actions[INPUT_STEER_LEFT] = left;
    gamepad.actions[INPUT_STEER_RIGHT] = right;
    Release(pad);
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFTX, SDL_JOYSTICK_AXIS_MIN);
    RaceFrame();
    Check(CInput::m_controllerInfo[2].analogSteering != 0, "the stick steers as an axis again");
    Release(pad);
    RaceFrame();
}

// The second controller is player 2's when two players race; with one, any
// controller drives player 1.
void CheckTwoPlayers(Pad &first)
{
    Pad second = AttachPad("Virtual Pad B", 0x02ea);
    Release(first);
    SDL_SetJoystickVirtualButton(second.joystick, SDL_GAMEPAD_BUTTON_SOUTH, true);
    MenuFrame();
    Check((Device(0).held & 0x10) != 0, "with one player, the second pad works in player 1's menus");
    g_selectedRallyData = 2 << 14;
    MenuFrame();
    Check((Device(0).held & 0x10) == 0 && (Device(1).held & 0x10) != 0, "with two players, it is player 2's");
    SDL_SetJoystickVirtualButton(second.joystick, SDL_GAMEPAD_BUTTON_SOUTH, false);
    SDL_SetJoystickVirtualButton(first.joystick, SDL_GAMEPAD_BUTTON_SOUTH, true);
    MenuFrame();
    Check((Device(0).held & 0x10) != 0 && (Device(1).held & 0x10) == 0, "the first pad stays player 1's");
    s_keys[0x4b] = 0x80;   // keypad 4: player 2's keyboard
    RaceFrame();
    Check((Device(1).held & 1) != 0 && (Device(0).held & 1) == 0, "the keypad steers player 2");
    s_keys[0x4b] = 0;
    g_selectedRallyData = 0;
    Release(first);
    Detach(second);
    MenuFrame();
}

// A controller SDL has no layout for gets one from a manual profile.
void CheckManualMapping()
{
    SDL_VirtualJoystickDesc desc;
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_UNKNOWN;
    desc.name = "Unmapped handheld";
    desc.naxes = 2;
    desc.nbuttons = 36;
    SDL_JoystickID id = SDL_AttachVirtualJoystick(&desc);
    SDL_Joystick *virt = SDL_OpenJoystick(id);
    Check(virt && !SDL_IsGamepad(id), "unknown handheld initially has no SDL gamepad layout");
    Check(Input_ApplyGamepadMapping(id, "a:b20,b:b21,leftx:a0,lefty:a1,lefttrigger:b15,righttrigger:b16"),
          "a manual profile adds an unknown controller");
    SDL_SetJoystickVirtualButton(virt, 20, true);
    SDL_SetJoystickVirtualButton(virt, 16, true);
    SDL_SetJoystickVirtualAxis(virt, 0, SDL_JOYSTICK_AXIS_MIN);
    Input_Update();
    Check(Input_GetAction(0, INPUT_HANDBRAKE) == INPUT_FULL, "its mapped A is the handbrake");
    Check(Input_GetAction(0, INPUT_THROTTLE) == INPUT_FULL, "a digital trigger is the throttle");
    Check(Input_GetAction(0, INPUT_STEER_LEFT) == INPUT_FULL, "its stick steers");
    SDL_CloseJoystick(virt);
    SDL_DetachVirtualJoystick(id);
    Input_Update();
}

// A wheel's pedals are found from where its axes rest.
void CheckWheel()
{
    SDL_VirtualJoystickDesc desc;
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_WHEEL;
    desc.name = "Virtual Wheel";
    desc.vendor_id = 0x046d;
    desc.product_id = 0xc24f;
    desc.naxes = 4;
    desc.nbuttons = 12;
    desc.nhats = 1;
    SDL_JoystickID id = SDL_AttachVirtualJoystick(&desc);
    SDL_Joystick *virt = SDL_OpenJoystick(id);
    SDL_SetJoystickVirtualAxis(virt, 0, 0);
    SDL_SetJoystickVirtualAxis(virt, 1, SDL_JOYSTICK_AXIS_MIN);   // clutch: not a pedal used
    SDL_SetJoystickVirtualAxis(virt, 2, SDL_JOYSTICK_AXIS_MAX);
    SDL_SetJoystickVirtualAxis(virt, 3, SDL_JOYSTICK_AXIS_MAX);
    SDL_UpdateJoysticks();
    Input_Update();
    Check(Input_GetAction(0, INPUT_THROTTLE) == 0 && Input_GetAction(0, INPUT_BRAKE) == 0, "released pedals");
    SDL_SetJoystickVirtualAxis(virt, 1, SDL_JOYSTICK_AXIS_MAX);
    SDL_SetJoystickVirtualAxis(virt, 0, SDL_JOYSTICK_AXIS_MAX);
    Input_Update();
    Check(Input_GetAction(0, INPUT_THROTTLE) == INPUT_FULL, "the first pedal is the throttle");
    Check(Input_GetAction(0, INPUT_STEER_RIGHT) == INPUT_FULL, "the wheel steers on X, without deadzone");
    Check(!Input_IsSmoothSteering(0), "a wheel is followed directly");
    SDL_SetJoystickVirtualAxis(virt, 2, SDL_JOYSTICK_AXIS_MIN);
    Input_Update();
    Check(Input_GetAction(0, INPUT_BRAKE) == INPUT_FULL, "a pedal resting at the maximum is the brake");
    SDL_SetJoystickVirtualHat(virt, 0, SDL_HAT_DOWN);
    SDL_SetJoystickVirtualButton(virt, 0, true);
    Input_Update();
    Check(Input_GetAction(0, INPUT_MENU_DOWN) == INPUT_FULL && Input_GetAction(0, INPUT_MENU_CONFIRM) == INPUT_FULL,
          "the hat and first button work in the menus");
    SDL_CloseJoystick(virt);
    SDL_DetachVirtualJoystick(id);
    Input_Update();
}

// Losing the controller a player drives with pauses a stage; an unused one,
// or one lost in the menus, does nothing.
void CheckDisconnectPause()
{
    Pad pad = AttachPad("Virtual Pad D", 0x0b12);
    RaceFrame();
    Detach(pad);
    RaceFrame();
    Check((Device(0).pressed & 0x400) == 0, "an unused controller does not pause");

    pad = AttachPad("Virtual Pad D", 0x0b12);
    RaceFrame();
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, SDL_JOYSTICK_AXIS_MAX);
    RaceFrame();
    Release(pad);
    RaceFrame();
    Detach(pad);
    RaceFrame();
    Check((Device(0).pressed & 0x400) != 0 && (Device(2).pressed & 0x200) != 0, "losing the driving controller pauses");
    RaceFrame();
    RaceFrame();
    Check((Device(0).held & 0x600) == 0, "the pause is let go");

    pad = AttachPad("Virtual Pad D", 0x0b12);
    MenuFrame();
    SDL_SetJoystickVirtualButton(pad.joystick, SDL_GAMEPAD_BUTTON_SOUTH, true);
    MenuFrame();
    Release(pad);
    MenuFrame();
    Detach(pad);
    MenuFrame();
    Check((Device(0).pressed & 0x3f) == 0, "in the menus nothing is pressed");
    MenuFrame();
    MenuFrame();
}

Uint16 s_rumble[2];
int s_rumbleCalls;
int s_otherRumbleCalls;

bool SDLCALL RecordRumble(void *, Uint16 low, Uint16 high)
{
    s_rumble[0] = low;
    s_rumble[1] = high;
    s_rumbleCalls++;
    return true;
}

bool SDLCALL RecordOtherRumble(void *, Uint16, Uint16)
{
    s_otherRumbleCalls++;
    return true;
}

// A gamepad with rumble is the player's force-feedback device and rumbles
// with Input_SetRumble, scaled by its setting.
void CheckRumble(Pad &first)
{
    SDL_VirtualJoystickDesc desc;
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.vendor_id = 0x045e;
    desc.product_id = 0x0b13;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.name = "Virtual Rumble Pad";
    desc.Rumble = RecordRumble;
    SDL_JoystickID id = SDL_AttachVirtualJoystick(&desc);
    SDL_Joystick *virt = SDL_OpenJoystick(id);
    Detach(first);
    Input_Update();
    Check(Input_GetFeedbackDevice(0) != NULL, "a gamepad with rumble takes the force feedback");
    Input_SetRumble(0, INPUT_FULL / 2, INPUT_FULL);
    Check(s_rumbleCalls == 1 && s_rumble[0] > 0x7f00 && s_rumble[0] < 0x8100 && s_rumble[1] == 0xffff, "it rumbles");
    Input_SetRumble(0, INPUT_FULL / 2, INPUT_FULL);
    Check(s_rumbleCalls == 1, "an unchanged rumble is not sent again at once");
    Input_SetRumble(0, 0, 0);
    Check(s_rumbleCalls == 2 && s_rumble[0] == 0 && s_rumble[1] == 0, "it stops");
    Bindings_Find("gamepad")->rumble = 50;
    Input_SetRumble(0, 0, INPUT_FULL);
    Check(s_rumble[1] > 0x7f00 && s_rumble[1] < 0x8100, "rumble = 50 halves it");
    Bindings_Find("gamepad")->rumble = 100;
    Input_SetRumble(0, 0, 0);

    // With two, only the one last used rumbles: both drive player 1 (a
    // Steam Deck with a DualShock 4 connected).
    desc.product_id = 0x0b12;
    desc.name = "Virtual Other Rumble Pad";
    desc.Rumble = RecordOtherRumble;
    SDL_JoystickID otherId = SDL_AttachVirtualJoystick(&desc);
    SDL_Joystick *other = SDL_OpenJoystick(otherId);
    SDL_SetJoystickVirtualButton(virt, SDL_GAMEPAD_BUTTON_SOUTH, true);
    Input_Update();
    SDL_SetJoystickVirtualButton(virt, SDL_GAMEPAD_BUTTON_SOUTH, false);
    Input_Update();
    s_rumbleCalls = s_otherRumbleCalls = 0;
    Input_SetRumble(0, 0, INPUT_FULL);
    Check(s_rumbleCalls == 1 && s_otherRumbleCalls == 0, "the pad last used rumbles, not the other");
    Input_SetRumble(0, 0, 0);
    Input_DevicesChanged();
    Input_GetFeedbackDevice(1);   // set up for both players, as in the menus
    InputDevice *used = Input_GetFeedbackDevice(0);
    SDL_SetJoystickVirtualButton(other, SDL_GAMEPAD_BUTTON_SOUTH, true);
    Input_Update();
    SDL_SetJoystickVirtualButton(other, SDL_GAMEPAD_BUTTON_SOUTH, false);
    Input_Update();
    s_rumbleCalls = s_otherRumbleCalls = 0;
    Input_SetRumble(0, 0, INPUT_FULL);
    Check(s_rumbleCalls == 0 && s_otherRumbleCalls == 1, "and the other once it is used");
    Input_SetRumble(0, 0, 0);
    // The force feedback follows the controller used, set up again in the
    // menus; one without rumble or forces gets none (a wheel does not turn
    // by itself while a gamepad drives).
    Check(Input_DevicesChanged(), "using another controller sets up the force feedback again");
    Check(Input_GetFeedbackDevice(0) != used && Input_GetFeedbackDevice(0) != NULL, "on the controller used");
    Input_Update();
    Check(!Input_DevicesChanged(), "once");
    Pad plain = AttachPad("Virtual Pad B", 0x02ea);
    SDL_SetJoystickVirtualButton(plain.joystick, SDL_GAMEPAD_BUTTON_SOUTH, true);
    Input_Update();
    Release(plain);
    Input_Update();
    Check(Input_GetFeedbackDevice(0) == NULL, "none while a controller without rumble drives");
    s_rumbleCalls = s_otherRumbleCalls = 0;
    Input_SetRumble(0, 0, INPUT_FULL);
    Check(s_rumbleCalls == 0 && s_otherRumbleCalls == 0, "and the others do not rumble");
    Detach(plain);
    SDL_CloseJoystick(other);
    SDL_DetachVirtualJoystick(otherId);

    SDL_CloseJoystick(virt);
    SDL_DetachVirtualJoystick(id);
    first = AttachPad("Virtual Xbox 360 Controller", 0x028e);
    Input_Update();
}

// The controls pages, driven with a gamepad as the frontend runs them:
// choosing the gamepad's bindings, binding two actions (the second trades
// places), Back dropping the changes and OK saving them.
void CheckControlsPages(Pad &pad)
{
    std::filesystem::path dir = std::filesystem::temp_directory_path() / ("opencmr2-pages-" + std::to_string(SDL_GetTicksNS()));
    std::filesystem::create_directories(dir);
    std::string dirText = dir.string() + "/";
    Platform_SetTestUserDir(dirText.c_str());
    Input_ReloadBindings();
    Input_SaveBindings();
    Release(pad);
    Menu_SetInputStateFlag(0);
    FrontendMenu_BuildInputControls();
    Menu *controls = FrontendMenu_GetInputControls();
    auto frame = [&](SDL_GamepadButton button, bool down) {
        if (button != SDL_GAMEPAD_BUTTON_INVALID)
            SDL_SetJoystickVirtualButton(pad.joystick, button, down);
        CMain::m_frameTime += 16;
        CGameInfo::m_fixedMenuKeys = 1;
        FrontendMenu_UpdateAndSwitchActive();
    };
    auto press = [&](SDL_GamepadButton button) {
        frame(button, true);
        frame(button, false);
    };
    auto active = []() { return g_pMenu0x00818ac0; };
    char text[64];

    // The page opens on the bindings of the keys or controller used last.
    s_keys[INPUT_KEY_LEFT] = 0x80;
    Input_Update();
    s_keys[INPUT_KEY_LEFT] = 0;
    Input_Update();
    g_pMenu0x00818ac0 = controls;
    FrontendMenu_EnterControls(controls, 0);
    frame(SDL_GAMEPAD_BUTTON_INVALID, false);
    Check(controls->cursor == 0 && Menu_GetItem(controls, 0)->min >= 3, "the device row lists keys and gamepads");
    Check(Menu_GetItem(controls, 0)->max == 0, "after a key, the page opens on player 1's keys");
    Check(!Menu_GetItem(controls, 2)->visible, "keys have no deadzone");
    press(SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
    press(SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
    Check(Menu_GetItem(controls, 0)->max == 2 && Input_GetProfileKind(2) == INPUT_PROFILE_GAMEPAD, "the gamepad chosen");
    Check(Menu_GetItem(controls, 2)->visible && Menu_GetItem(controls, 4)->visible, "with deadzone and rumble");
    Check(Input_IsSmoothSteering(0) && CInput::m_controllerInfo[2].smoothSteering != 0,
          "a gamepad's stick is eased");
    press(SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    press(SDL_GAMEPAD_BUTTON_SOUTH);
    frame(SDL_GAMEPAD_BUTTON_INVALID, false);
    Menu *bindings = active();
    Check(bindings != controls && bindings != NULL, "Configuration opens the bindings");

    for (int i = 0; i < INPUT_HANDBRAKE; i++)
        press(SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    Check(bindings->cursor == INPUT_HANDBRAKE, "on the handbrake");
    frame(SDL_GAMEPAD_BUTTON_SOUTH, true);
    frame(SDL_GAMEPAD_BUTTON_SOUTH, true);
    frame(SDL_GAMEPAD_BUTTON_SOUTH, false);
    s_keys[INPUT_KEY_DOWN] = 0x80;
    frame(SDL_GAMEPAD_BUTTON_INVALID, false);
    s_keys[INPUT_KEY_DOWN] = 0;
    frame(SDL_GAMEPAD_BUTTON_INVALID, false);
    Check(bindings->cursor == INPUT_HANDBRAKE, "the menu waits while an input is awaited");
    frame(SDL_GAMEPAD_BUTTON_WEST, true);
    frame(SDL_GAMEPAD_BUTTON_WEST, false);
    Input_GetBindingText(2, INPUT_HANDBRAKE, text, sizeof(text));
    Check(strcmp(text, "X") == 0, "the handbrake is on X");
    press(SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    Check(bindings->cursor == INPUT_SHIFT_UP, "the menu works again");
    press(SDL_GAMEPAD_BUTTON_SOUTH);
    frame(SDL_GAMEPAD_BUTTON_INVALID, false);
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, SDL_JOYSTICK_AXIS_MAX);
    frame(SDL_GAMEPAD_BUTTON_INVALID, false);
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, SDL_JOYSTICK_AXIS_MIN);
    frame(SDL_GAMEPAD_BUTTON_INVALID, false);
    Input_GetBindingText(2, INPUT_SHIFT_UP, text, sizeof(text));
    Check(strcmp(text, "LT") == 0, "a trigger can shift up");
    Input_GetBindingText(2, INPUT_BRAKE, text, sizeof(text));
    Check(strcmp(text, "RB") == 0, "the brake trades places with it");

    press(SDL_GAMEPAD_BUTTON_EAST);
    frame(SDL_GAMEPAD_BUTTON_INVALID, false);
    Check(active() == controls, "Back returns to the controls page");
    press(SDL_GAMEPAD_BUTTON_EAST);
    frame(SDL_GAMEPAD_BUTTON_INVALID, false);
    Input_GetBindingText(2, INPUT_HANDBRAKE, text, sizeof(text));
    Check(strcmp(text, "A") == 0, "Back from the controls page drops the changes");

    g_pMenu0x00818ac0 = controls;
    FrontendMenu_EnterControls(controls, 0);
    controls->cursor = 0;
    Check(Menu_GetItem(controls, 0)->max == 2, "after the gamepad, on the gamepads'");
    press(SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    press(SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    press(SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
    press(SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
    Check(Input_GetSetting(2, INPUT_SETTING_DEADZONE) == 4, "the deadzone is set");
    while (controls->items[controls->cursor].value != 0xe)
        press(SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    press(SDL_GAMEPAD_BUTTON_SOUTH);
    frame(SDL_GAMEPAD_BUTTON_INVALID, false);
    Input_ReloadBindings();
    Check(Input_GetSetting(2, INPUT_SETTING_DEADZONE) == 4, "OK saves it in controls.ini");

    g_pMenu0x00818ac0 = NULL;
    Platform_SetTestUserDir("");
    Input_ReloadBindings();
    std::filesystem::remove_all(dir);
    Release(pad);
}

// controls.ini is written with the defaults and read back; an edited file
// rebinds the actions.
void CheckBindingsFile(Pad &pad)
{
    std::filesystem::path dir = std::filesystem::temp_directory_path() / ("opencmr2-input-" + std::to_string(SDL_GetTicksNS()));
    std::filesystem::create_directories(dir);
    std::string dirText = dir.string() + "/";
    Platform_SetTestUserDir(dirText.c_str());
    Input_ReloadBindings();
    Input_SaveBindings();
    std::filesystem::path file = dir / "controls.ini";
    Check(std::filesystem::exists(file), "controls.ini is written with the defaults");

    char *text = (char *)SDL_LoadFile(file.string().c_str(), nullptr);
    std::string content = text ? text : "";
    SDL_free(text);
    Check(content.find("handbrake = a\n") != std::string::npos, "the gamepad's handbrake is written");
    size_t at = content.find("handbrake = a\n");
    content.replace(at, strlen("handbrake = a\n"), "handbrake = x | leftstick\n");
    SDL_SaveFile(file.string().c_str(), content.data(), content.size());
    Input_ReloadBindings();
    Release(pad);
    SDL_SetJoystickVirtualButton(pad.joystick, SDL_GAMEPAD_BUTTON_SOUTH, true);
    Input_Update();
    Check(Input_GetAction(0, INPUT_HANDBRAKE) == 0, "A is no longer the handbrake");
    SDL_SetJoystickVirtualButton(pad.joystick, SDL_GAMEPAD_BUTTON_LEFT_STICK, true);
    Input_Update();
    Check(Input_GetAction(0, INPUT_HANDBRAKE) == INPUT_FULL, "the second input of the list is");

    // A controls.ini of format 1 wrote every setting: the defaults of the
    // time follow today's, a setting chosen stays.
    std::string old = "[gamepad]\ndeadzone = 2\nsaturation = 100\nrumble = 10\n";
    SDL_SaveFile(file.string().c_str(), old.data(), old.size());
    Input_ReloadBindings();
    Check(Input_GetSetting(2, INPUT_SETTING_DEADZONE) == 2 && Bindings_Find("gamepad")->saturation == 95,
          "the old defaults follow today's");
    Check(Input_GetSetting(2, INPUT_SETTING_RUMBLE) == 10, "a chosen setting stays");
    Input_SaveBindings();
    Bindings_MarkChanged();
    Input_SaveBindings();
    text = (char *)SDL_LoadFile(file.string().c_str(), nullptr);
    content = text ? text : "";
    SDL_free(text);
    Check(content.find("rumble = 10") != std::string::npos && content.find("saturation =") == std::string::npos,
          "only the chosen settings are written");

    // An older Controller.rcf only gives the keys while controls.ini is
    // missing.
    BYTE keys[9] = {0x1e, 0x20, 0x11, 0x1f, 0x39, 0x1b, 0x1a, 0x2e, 0x13};   // A D W S
    Input_ImportKeyboardKeys(0, keys);
    memset(s_keys, 0, sizeof(s_keys));
    s_keys[0x1e] = 0x80;
    Input_Update();
    Check(Input_GetAction(0, INPUT_STEER_LEFT) == 0, "the import does not replace controls.ini");
    std::filesystem::remove(file);
    Input_ReloadBindings();
    Input_ImportKeyboardKeys(0, keys);
    Input_Update();
    Check(Input_GetAction(0, INPUT_STEER_LEFT) == INPUT_FULL, "without it, the old keys are taken");
    s_keys[0x1e] = 0;
    Release(pad);

    Platform_SetTestUserDir("");
    Input_ReloadBindings();
    std::filesystem::remove_all(dir);
}

} // namespace

int main()
{
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    // Only the virtual devices below: a controller connected to the machine
    // would take a place among them.
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,
                "0x045e/0x028e,0x045e/0x02ea,0x045e/0x0b12,0x045e/0x0b13,0x046d/0xc24f,0x0000/0x0000");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window *window = SDL_CreateWindow("unfocused", 64, 64, SDL_WINDOW_HIDDEN);
    Check(window != NULL && SDL_GetKeyboardFocus() == NULL, "a game window without the keyboard focus");
    Platform_SetTestKeyboard(s_keys);

    // An Xbox 360 controller: SDL knows its layout, as for a real one.
    Pad pad = AttachPad("Virtual Xbox 360 Controller", 0x028e);
    if (pad.joystick == NULL) {
        fprintf(stderr, "virtual joystick: %s\n", SDL_GetError());
        return 1;
    }
    SetupGameInput();

    CheckGamepadActions(pad);
    CheckRecords(pad);
    CheckRacePresses(pad);
    CheckMixedChannels(pad);
    CheckTwoPlayers(pad);
    CheckManualMapping();
    CheckWheel();
    CheckDisconnectPause();
    CheckRumble(pad);
    CheckControlsPages(pad);
    CheckBindingsFile(pad);

    Check(Input_GetProfileCount() == 3 && Input_GetProfileKind(2) == INPUT_PROFILE_GAMEPAD,
          "the controls pages list the gamepads while one is connected");
    Detach(pad);
    Input_Update();
    Check(Input_GetProfileCount() == 2, "and not without one, as nothing could be bound");
    Platform_SetTestKeyboard(nullptr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    if (s_failures != 0) {
        fprintf(stderr, "%d failures\n", s_failures);
        return 1;
    }
    printf("input actions: ok\n");
    return 0;
}
