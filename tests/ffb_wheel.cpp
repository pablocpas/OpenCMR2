// A force feedback wheel and pedals made with uinput, as the kernel shows
// real ones: SDL finds them through udev, reads them through evdev and
// uploads its effects to them, and this program answers as their drivers
// do, keeping each effect it is given. Checks the controls and the force
// feedback end to end, the game's own calls included (CInput, as the
// stages make them):
//
// - a wheel with every effect (a direct drive's driver), its pedals on the
//   same device, and one of their own
// - a wheel with only a constant force and an autocentre (Logitech's on
//   Linux, hid-lg4ff), whose centring spring is played as its autocentre
//
// The force a driver applies along the wheel's axis is computed as Linux's
// ff-memless does: the level times the sine of the direction, positive to
// the left (0x4000). The original asked DirectInput for a constant force
// coming from the east (9000, pushing left) for a negative force, from the
// west (27000) for a positive one.
//
// Needs read and write access to /dev/uinput (for instance
// sudo setfacl -m u:$USER:rw /dev/uinput): skipped without it.

// Linux's input headers first: game/Input.h has their include guard.
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>
#undef _INPUT_H

#include "port/types.h"
#include "port/input.h"
#include "GameInfo.h"
#include "Input.h"
#include "Menu.h"
#include "platform/platform.h"
#include "platform/testing.h"
#include "input/input_internal.h"
#include "main.h"

#include <SDL3/SDL.h>

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

extern void Input_RescanChangedJoysticks();
extern unsigned int g_selectedRallyData;

namespace {

int s_failures;
BYTE s_keys[256];

void Check(bool ok, const char *what)
{
    printf("%s: %s\n", ok ? "ok  " : "FAIL", what);
    if (!ok)
        s_failures++;
}

// One uinput device, and the thread that answers for its driver.
struct Device {
    int fd = -1;
    std::string name;
    std::thread thread;
    std::atomic<bool> stop{false};
    std::mutex lock;
    std::map<int, ff_effect> effects;   // uploaded, by id
    std::map<int, int> playing;         // play count, by id
    int gain = 0xffff;
    int autocentre = -1;                // -1 until set
    int uploads = 0;
    std::vector<ff_effect> history;     // every upload, in order
    std::vector<long long> historyClock; // when, on the monotonic clock (ns)

    bool Create(const char *deviceName, Uint16 product, bool wheel, bool pedals, unsigned effectBits)
    {
        name = deviceName;
        fd = open("/dev/uinput", O_RDWR | O_NONBLOCK);
        if (fd < 0)
            return false;
        ioctl(fd, UI_SET_EVBIT, EV_KEY);
        for (int button = BTN_TRIGGER; button <= BTN_BASE6; button++)
            ioctl(fd, UI_SET_KEYBIT, button);
        ioctl(fd, UI_SET_EVBIT, EV_ABS);
        struct Axis { int code, min, max, rest; };
        // The wheel on X; the pedals rest at their maximum, as a G29's on
        // Linux: Y the clutch, Z the throttle, RZ the brake.
        std::vector<Axis> axes;
        if (wheel)
            axes.push_back({ ABS_X, 0, 65535, 32768 });
        if (pedals) {
            axes.push_back({ ABS_Y, 0, 255, 255 });
            axes.push_back({ ABS_Z, 0, 255, 255 });
            axes.push_back({ ABS_RZ, 0, 255, 255 });
        }
        for (const Axis &axis : axes) {
            ioctl(fd, UI_SET_ABSBIT, axis.code);
            uinput_abs_setup setup{};
            setup.code = axis.code;
            setup.absinfo.minimum = axis.min;
            setup.absinfo.maximum = axis.max;
            setup.absinfo.value = axis.rest;
            ioctl(fd, UI_ABS_SETUP, &setup);
        }
        if (effectBits != 0) {
            ioctl(fd, UI_SET_EVBIT, EV_FF);
            for (int bit = 0; bit < 32; bit++)
                if (effectBits & (1u << bit))
                    ioctl(fd, UI_SET_FFBIT, FF_RUMBLE + bit);
        }
        uinput_setup setup{};
        setup.id.bustype = BUS_USB;
        setup.id.vendor = 0x046d;
        setup.id.product = product;
        setup.id.version = 0x111;
        snprintf(setup.name, sizeof(setup.name), "%s", deviceName);
        setup.ff_effects_max = effectBits != 0 ? 16 : 0;
        if (ioctl(fd, UI_DEV_SETUP, &setup) < 0 || ioctl(fd, UI_DEV_CREATE) < 0) {
            close(fd);
            fd = -1;
            return false;
        }
        thread = std::thread([this] { Serve(); });
        return true;
    }

    void Destroy()
    {
        if (fd < 0)
            return;
        stop = true;
        thread.join();
        ioctl(fd, UI_DEV_DESTROY);
        close(fd);
        fd = -1;
    }

    void Write(int type, int code, int value)
    {
        input_event event{};
        event.type = type;
        event.code = code;
        event.value = value;
        if (write(fd, &event, sizeof(event)) != (ssize_t)sizeof(event))
            perror("uinput write");
    }

    void Set(int code, int value)
    {
        Write(EV_ABS, code, value);
        Write(EV_SYN, SYN_REPORT, 0);
    }

    void Button(int code, bool down)
    {
        Write(EV_KEY, code, down ? 1 : 0);
        Write(EV_SYN, SYN_REPORT, 0);
    }

    // The driver's side: effects uploaded and erased, played and stopped,
    // the gain and the autocentre.
    void Serve()
    {
        while (!stop) {
            pollfd p = { fd, POLLIN, 0 };
            if (poll(&p, 1, 10) <= 0)
                continue;
            input_event event;
            while (read(fd, &event, sizeof(event)) == (ssize_t)sizeof(event)) {
                if (event.type == EV_UINPUT && event.code == UI_FF_UPLOAD) {
                    uinput_ff_upload upload{};
                    upload.request_id = event.value;
                    ioctl(fd, UI_BEGIN_FF_UPLOAD, &upload);
                    {
                        std::lock_guard<std::mutex> guard(lock);
                        effects[upload.effect.id] = upload.effect;
                        history.push_back(upload.effect);
                        timespec now;
                        clock_gettime(CLOCK_MONOTONIC, &now);
                        historyClock.push_back((long long)now.tv_sec * 1000000000LL + now.tv_nsec);
                        uploads++;
                    }
                    upload.retval = 0;
                    ioctl(fd, UI_END_FF_UPLOAD, &upload);
                } else if (event.type == EV_UINPUT && event.code == UI_FF_ERASE) {
                    uinput_ff_erase erase{};
                    erase.request_id = event.value;
                    ioctl(fd, UI_BEGIN_FF_ERASE, &erase);
                    {
                        std::lock_guard<std::mutex> guard(lock);
                        effects.erase(erase.effect_id);
                        playing.erase(erase.effect_id);
                    }
                    erase.retval = 0;
                    ioctl(fd, UI_END_FF_ERASE, &erase);
                } else if (event.type == EV_FF) {
                    std::lock_guard<std::mutex> guard(lock);
                    if (event.code == FF_GAIN)
                        gain = event.value;
                    else if (event.code == FF_AUTOCENTER)
                        autocentre = event.value;
                    else
                        playing[event.code] = event.value;
                }
            }
        }
    }

    // The playing effect of a type, NULL without one.
    bool Find(int type, ff_effect *out, bool mustPlay = true)
    {
        std::lock_guard<std::mutex> guard(lock);
        for (const auto &entry : effects) {
            if (entry.second.type != type)
                continue;
            if (mustPlay && playing[entry.first] == 0)
                continue;
            *out = entry.second;
            return true;
        }
        return false;
    }

    // The constant force along the wheel, as ff-memless applies it: 0..1
    // of full force, positive to the left.
    double ConstantLeft()
    {
        ff_effect effect;
        if (!Find(FF_CONSTANT, &effect))
            return 0.0;
        double angle = effect.direction * 2.0 * M_PI / 65536.0;
        return std::sin(angle) * effect.u.constant.level / 32767.0;
    }
};

const unsigned kAllEffects = (1u << (FF_CONSTANT - FF_RUMBLE)) | (1u << (FF_SPRING - FF_RUMBLE)) |
                             (1u << (FF_DAMPER - FF_RUMBLE)) | (1u << (FF_PERIODIC - FF_RUMBLE)) |
                             (1u << (FF_SINE - FF_RUMBLE)) | (1u << (FF_GAIN - FF_RUMBLE)) |
                             (1u << (FF_AUTOCENTER - FF_RUMBLE));
const unsigned kLogitechEffects = (1u << (FF_CONSTANT - FF_RUMBLE)) | (1u << (FF_AUTOCENTER - FF_RUMBLE));

// Updates until a device of that name is connected (udev takes a moment);
// its index in the controls pages, -1 if it never comes.
int WaitFor(const char *name)
{
    for (int tries = 0; tries < 300; tries++) {
        Input_Update();
        for (int i = 0; i < Input_GetProfileCount(); i++)
            if (strcmp(Input_GetProfileName(i), name) == 0)
                return i;
        SDL_Delay(10);
    }
    int count = 0;
    SDL_JoystickID *ids = SDL_GetJoysticks(&count);
    printf("      SDL lists %d joysticks:", count);
    for (int i = 0; i < count; i++)
        printf(" [%s]", SDL_GetJoystickNameForID(ids[i]));
    printf("\n");
    SDL_free(ids);
    return -1;
}

void WaitGone(const char *name)
{
    for (int tries = 0; tries < 300; tries++) {
        Input_Update();
        bool found = false;
        for (int i = 0; i < Input_GetProfileCount(); i++)
            found = found || strcmp(Input_GetProfileName(i), name) == 0;
        if (!found)
            return;
        SDL_Delay(10);
    }
}

// Updates long enough for evdev's events to reach SDL.
void Settle()
{
    for (int i = 0; i < 5; i++) {
        SDL_Delay(5);
        Input_Update();
    }
}

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

// The menus set the force feedback up again when the devices change.
void Rescan()
{
    Input_Update();
    Input_RescanChangedJoysticks();
}

// The record of player 1's joystick, the force feedback's device index.
const int kPlayerRecord = 2;

// The stage's effects, as ForceFeedback_CreateEffects makes them.
void CreateStageEffects()
{
    CInput::CreateSpringEffect(-1, 0x2710, 0, -1, kPlayerRecord);
    CInput::SetConditionCoefficient(0, 0, kPlayerRecord);
    CInput::CreateDamperEffect(-1, 0, 0, -1, kPlayerRecord);
    CInput::SetConditionCoefficient(1, 0, kPlayerRecord);
    CInput::CreateConstantForceEffect(-1, 0, 0x2710, 0, 0, 0, 0, -1, kPlayerRecord);
    CInput::SetEffectGain(2, 0, kPlayerRecord);
    for (int i = 0; i < 3; i++)
        CInput::StartForceFeedbackEffect(i, kPlayerRecord);
}

// A force of the stage, -10000..10000, as ForceFeedback_ApplyStoredForces
// gives it.
void ApplyConstant(int force)
{
    CInput::SetEffectGainAndDirection(2, (DWORD)abs(force), force < 0 ? 0x2328 : 0x6978, kPlayerRecord);
}

void StopStageEffects()
{
    CInput::ResetForceFeedbackEffectsAlt();
}

// Updates for a while, as frames do: a force eases over a physics step.
void Frames(int ms)
{
    Uint64 end = SDL_GetTicks() + (Uint64)ms;
    while (SDL_GetTicks() < end) {
        Input_Update();
        SDL_Delay(4);
    }
    SDL_Delay(10);
}

bool Near(double value, double expected, double tolerance = 0.02)
{
    return std::fabs(value - expected) <= tolerance;
}

// A wheel with every effect, pedals of its own and pedals on the wheel.
void CheckDirectDriveWheel()
{
    printf("-- wheel with every effect, pedals of its own\n");
    Device wheel, pedals;
    if (!wheel.Create("Virtual DD Wheel", 0xc24f, true, false, kAllEffects) ||
        !pedals.Create("Virtual Pedals", 0xc262, false, true, 0)) {
        Check(false, "uinput devices created");
        return;
    }
    int wheelIndex = WaitFor("Virtual DD Wheel");
    int pedalsIndex = WaitFor("Virtual Pedals");
    Check(wheelIndex >= 0 && pedalsIndex >= 0, "SDL finds the wheel and the pedals through udev");
    if (wheelIndex < 0 || pedalsIndex < 0) {
        wheel.Destroy();
        pedals.Destroy();
        return;
    }
    Check(Input_HasForceFeedback(wheelIndex) && !Input_HasForceFeedback(pedalsIndex),
          "the wheel has force feedback, the pedals none");
    Rescan();
    Settle();

    // Controls through evdev.
    Check(Input_GetAction(0, INPUT_STEER_LEFT) == 0 && Input_GetAction(0, INPUT_STEER_RIGHT) == 0 &&
          Input_GetAction(0, INPUT_THROTTLE) == 0 && Input_GetAction(0, INPUT_BRAKE) == 0,
          "at rest: no steering, no pedal");
    pedals.Set(ABS_Y, 0);
    Settle();
    Check(Input_GetAction(0, INPUT_STEER_LEFT) == 0 && Input_GetAction(0, INPUT_STEER_RIGHT) == 0 &&
          Input_GetAction(0, INPUT_THROTTLE) == 0 && Input_GetAction(0, INPUT_BRAKE) == 0,
          "the clutch does nothing");
    pedals.Set(ABS_Y, 255);
    pedals.Set(ABS_Z, 0);
    wheel.Set(ABS_X, 65535);
    Settle();
    Check(Input_GetAction(0, INPUT_THROTTLE) == INPUT_FULL, "the middle pedal accelerates fully");
    Check(Input_GetAction(0, INPUT_STEER_RIGHT) == INPUT_FULL, "the wheel at its right end is full lock");
    pedals.Set(ABS_Z, 128);
    wheel.Set(ABS_X, 32768 - 16384);
    Settle();
    Check(Near(Input_GetAction(0, INPUT_THROTTLE) / (double)INPUT_FULL, 0.5, 0.01),
          "half the throttle's travel is half throttle");
    Check(Near(Input_GetAction(0, INPUT_STEER_LEFT) / (double)INPUT_FULL, 0.5, 0.01),
          "a quarter of the turn left is half lock left, linear");
    pedals.Set(ABS_Z, 255);
    pedals.Set(ABS_RZ, 0);
    Settle();
    Check(Input_GetAction(0, INPUT_BRAKE) == INPUT_FULL && Input_GetAction(0, INPUT_THROTTLE) == 0,
          "the right pedal brakes");
    pedals.Set(ABS_RZ, 255);
    Check(Input_IsAnalogSteering(0) && Input_IsAnalogPedals(0) && !Input_IsSmoothSteering(0),
          "analogue steering and pedals, the wheel followed directly as the original");
    CGameInfo::m_fixedMenuKeys = 1;
    CInput::UpdateAllAvailableDevices();
    Check((CInput::m_availableDevices[0].held & 0x3f) == 0, "nothing held in the menus");
    wheel.Button(BTN_TRIGGER, true);
    Settle();
    CInput::UpdateAllAvailableDevices();
    Check((CInput::m_availableDevices[0].held & 0x10) != 0, "the wheel's first button confirms in the menus");
    wheel.Button(BTN_TRIGGER, false);
    wheel.Set(ABS_X, 32768);
    Settle();

    // The force feedback, set up as the menus do and played as a stage.
    Check(Input_GetFeedbackDevice(0) != NULL, "the force feedback goes to the wheel");
    CreateStageEffects();
    Settle();
    ff_effect effect;
    Check(wheel.Find(FF_SPRING, &effect) && wheel.Find(FF_DAMPER, &effect) && wheel.Find(FF_CONSTANT, &effect),
          "the stage's spring, damper and constant force are uploaded and playing");
    Check(Near(wheel.ConstantLeft(), 0.0), "no force before the car moves");
    ApplyConstant(-6000);
    Frames(60);
    double left = wheel.ConstantLeft();
    printf("      negative force -6000: %.3f to the left\n", left);
    Check(Near(left, 0.6), "a negative force pushes left at 60%, as the original's from the east");
    ApplyConstant(4000);
    Frames(60);
    left = wheel.ConstantLeft();
    printf("      positive force 4000: %.3f to the left\n", left);
    Check(Near(left, -0.4), "a positive force pushes right at 40%");
    CInput::SetConditionCoefficient(0, 5000, kPlayerRecord);
    SDL_Delay(20);
    Check(wheel.Find(FF_SPRING, &effect) && Near(effect.u.condition[0].right_coeff / 32767.0, 0.5) &&
          Near(effect.u.condition[0].left_coeff / 32767.0, 0.5) && effect.u.condition[0].center == 0,
          "the spring centres the wheel at half strength on both sides");
    CInput::SetConditionCoefficient(1, 2500, kPlayerRecord);
    SDL_Delay(20);
    Check(wheel.Find(FF_DAMPER, &effect) && Near(effect.u.condition[0].right_coeff / 32767.0, 0.25),
          "the damper at a quarter");
    CInput::SetConditionCoefficient(1, -3000, kPlayerRecord);
    SDL_Delay(20);
    Check(wheel.Find(FF_DAMPER, &effect) && effect.u.condition[0].right_coeff == 0,
          "a negative damper (the original's can reach one) holds nothing instead of pushing");

    // Pedals pressed do not take the force feedback.
    pedals.Set(ABS_Z, 0);
    Settle();
    Check(!Input_DevicesChanged(), "pressing the pedals leaves the force feedback on the wheel");
    pedals.Set(ABS_Z, 255);
    Settle();

    // The strength chosen.
    Check(Input_GetFeedbackStrength(0) == 0x8000, "the original's strength by default");
    Input_SetSetting(wheelIndex, INPUT_SETTING_FORCE, 200);
    Check(Input_GetFeedbackStrength(0) == 0x10000, "200% doubles it for the stage's forces");
    SDL_Delay(30);
    Check(wheel.Find(FF_PERIODIC, &effect) && effect.u.periodic.waveform == FF_SINE && effect.direction == 0x4000,
          "changing the force plays a sample along the wheel's axis");
    Input_SetSetting(wheelIndex, INPUT_SETTING_FORCE, 100);

    // Leaving the stage stops everything.
    StopStageEffects();
    SDL_Delay(30);
    Check(!wheel.Find(FF_CONSTANT, &effect) && !wheel.Find(FF_SPRING, &effect), "leaving the stage stops the forces");

    // Losing the wheel during a stage pauses it.
    wheel.Set(ABS_X, 50000);
    Settle();
    wheel.Destroy();
    CGameInfo::m_fixedMenuKeys = 0;
    bool paused = false;
    for (int tries = 0; tries < 300 && !paused; tries++) {
        SDL_Delay(10);
        CInput::UpdateAllAvailableDevices();
        paused = (CInput::m_availableDevices[0].held & 0x400) != 0;
    }
    Check(paused, "unplugging the wheel pauses the stage");
    pedals.Destroy();
    WaitGone("Virtual Pedals");
    Rescan();
}

// A wheel with only a constant force and an autocentre, and its pedals on
// it (a G29 with hid-lg4ff).
void CheckLogitechWheel()
{
    printf("-- wheel with a constant force and an autocentre only, pedals on it\n");
    Device wheel;
    if (!wheel.Create("Virtual G29", 0xc24f, true, true, kLogitechEffects)) {
        Check(false, "uinput device created");
        return;
    }
    int index = WaitFor("Virtual G29");
    Check(index >= 0, "SDL finds it");
    if (index < 0) {
        wheel.Destroy();
        return;
    }
    Rescan();
    Settle();
    CGameInfo::m_fixedMenuKeys = 1;
    CInput::UpdateAllAvailableDevices();
    Check((CInput::m_availableDevices[0].held & 0x3f) == 0, "its pedals on Y, Z and RZ do not move through the menus");
    wheel.Set(ABS_Z, 0);
    wheel.Set(ABS_X, 0);
    Settle();
    Check(Input_GetAction(0, INPUT_THROTTLE) == INPUT_FULL && Input_GetAction(0, INPUT_STEER_LEFT) == INPUT_FULL,
          "it steers and accelerates");
    wheel.Set(ABS_Z, 255);
    wheel.Set(ABS_X, 32768);
    Settle();
    Check(Input_GetFeedbackDevice(0) != NULL, "the force feedback goes to it");
    CreateStageEffects();
    Settle();
    ff_effect effect;
    Check(wheel.Find(FF_CONSTANT, &effect) && !wheel.Find(FF_SPRING, &effect, false),
          "the constant force is uploaded, no spring is asked of it");
    CInput::SetConditionCoefficient(0, 6000, kPlayerRecord);
    SDL_Delay(30);
    int autocentre;
    {
        std::lock_guard<std::mutex> guard(wheel.lock);
        autocentre = wheel.autocentre;
    }
    printf("      autocentre for a spring of 6000: %d (of 65535)\n", autocentre);
    Check(Near(autocentre / 65535.0, 0.6, 0.02), "the spring is played as the autocentre, at its strength");
    ApplyConstant(-10000);
    Frames(60);
    Check(Near(wheel.ConstantLeft(), 1.0), "a full negative force pushes fully left");
    StopStageEffects();
    SDL_Delay(30);
    {
        std::lock_guard<std::mutex> guard(wheel.lock);
        autocentre = wheel.autocentre;
    }
    Check(autocentre == 0, "leaving the stage lets the wheel go");
    wheel.Destroy();
    WaitGone("Virtual G29");
    Rescan();
}

} // namespace

// --record <seconds> <file>: a wheel with every effect that only answers as
// its driver, writing every effect it is given, in order (the constant
// force to the left, or a spring's or damper's coefficient, 0..1), for a
// stage played meanwhile (stage_probe with OPENCMR2_PROBE_CONTROLLERS).
int Record(int seconds, const char *path)
{
    Device wheel;
    if (!wheel.Create("Virtual DD Wheel", 0xc24f, true, false, kAllEffects)) {
        fprintf(stderr, "ffb wheel: cannot create the wheel\n");
        return 1;
    }
    SDL_Delay((Uint32)seconds * 1000);
    wheel.stop = true;
    wheel.thread.join();
    FILE *file = fopen(path, "w");
    fprintf(file, "clock,type,value\n");
    for (size_t i = 0; i < wheel.history.size(); i++) {
        const ff_effect &effect = wheel.history[i];
        long long i_clock = wheel.historyClock[i];
        if (effect.type == FF_CONSTANT)
            fprintf(file, "%lld,constant,%.4f\n", i_clock,
                    std::sin(effect.direction * 2.0 * M_PI / 65536.0) * effect.u.constant.level / 32767.0);
        else if (effect.type == FF_SPRING || effect.type == FF_DAMPER)
            fprintf(file, "%lld,%s,%.4f\n", i_clock, effect.type == FF_SPRING ? "spring" : "damper",
                    effect.u.condition[0].right_coeff / 32767.0);
        else
            fprintf(file, "%lld,other,0\n", i_clock);
    }
    fclose(file);
    ioctl(wheel.fd, UI_DEV_DESTROY);
    close(wheel.fd);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 4 && strcmp(argv[1], "--record") == 0)
        return Record(atoi(argv[2]), argv[3]);
    int probe = open("/dev/uinput", O_RDWR);
    if (probe < 0) {
        printf("ffb wheel: no access to /dev/uinput, skipped\n");
        return 77;
    }
    close(probe);
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD | SDL_INIT_HAPTIC)) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    std::filesystem::path dir = std::filesystem::temp_directory_path() / ("opencmr2-ffb-" + std::to_string(SDL_GetTicksNS()));
    std::filesystem::create_directories(dir);
    std::string dirText = dir.string() + "/";
    Platform_SetTestUserDir(dirText.c_str());
    Platform_SetTestKeyboard(s_keys);
    Input_ReloadBindings();
    SetupGameInput();

    CheckDirectDriveWheel();
    CheckLogitechWheel();

    Platform_SetTestKeyboard(nullptr);
    Platform_SetTestUserDir("");
    std::filesystem::remove_all(dir);
    SDL_Quit();
    if (s_failures != 0) {
        printf("%d failures\n", s_failures);
        return 1;
    }
    printf("ffb wheel: ok\n");
    return 0;
}
