#ifndef _INPUT_H
#define _INPUT_H
#if defined(_MSC_VER) && _MSC_VER <= 1200
#ifndef CMR2_LAYOUT_CHECK
#define CMR2_LAYOUT_CHECK(name, condition) typedef char name[condition ? 1 : -1]
#endif
#else
#include "LayoutChecks.h"
#endif
#include "port/input.h"

// One of the eight axes of a joystick (x, y, z, rx, ry, rz, two sliders).
struct JoystickBinding {
    BOOL present;       // 0x00 the device reports a range for the axis
    int range;          // 0x04
    int deadzone;       // 0x08 0..10000
    int saturation;     // 0x0c 0..10000
    int position;       // 0x10 last polled position
};

struct JoystickInfo {
    int controlCount;               // axes the device reports
    JoystickBinding bindings[8];
};

// Key (DIK_*) of each button bit of a keyboard device: bit 1 << n for the
// first ten (the actions of ControllerData::actionMasks), then Escape and
// function keys.
struct KeyboardInfo {
    BYTE steerLeftKey;
    BYTE steerRightKey;
    BYTE throttleKey;
    BYTE brakeKey;
    BYTE handbrakeKey;
    BYTE shiftUpKey;
    BYTE shiftDownKey;
    BYTE cameraCycleKey;
    BYTE driverCycleKey;
    BYTE backKey;
    BYTE field_0x472;
    BYTE field_0x473;
    BYTE field_0x474;
    BYTE field_0x475;
    BYTE field_0x476;
    BYTE field_0x477;
};

struct DeviceInfo {
    DWORD type;             // 0/3 joystick, 1 keyboard, 2 mouse, -1 none
    unsigned int held;      // buttons held
    unsigned int pressed;   // buttons pressed this frame
    unsigned int repeated;  // direction buttons auto-repeated this frame
    int repeatTime;         // next auto-repeat time
    int buttonCount;        // joystick buttons read (names in field_0x284)
    int joystickIndex;      // into CInput::m_joysticks, -1 without one
    int field_0x1c;
    BYTE pad2[92];
    char deviceInstanceName[MAX_PATH];
    char deviceProductName[MAX_PATH];
    CHAR field_0x284[8][20];
    BYTE pad4[240];
    CHAR field_0x414[20];
    CHAR field_0x425[20];
    CHAR field_0x436[20];
    CHAR field_0x447[20];
    BOOL supportsForceFeedback; // GetCapabilities reports DIDC_FORCEFEEDBACK
    union {
        KeyboardInfo keyboard;
        JoystickInfo joystick;
    };
};

struct DeviceCounts {
    BYTE nextDevice;
    BYTE keyboardCount;
    BYTE joystickCount;
    BYTE attachedJoystickCount;
};

struct ForceFeedbackDevice {
    BOOL field_0x0;              // +0x0 (active flag)
    InputDevice *device;         // +0x4
    BYTE field_0x8;              // +0x8 (flag cleared at end)
    BYTE padding1[3];            // +0x9-0xB
    InputEffect *effects[10];    // +0xC (10 effect pointers)
};

// A control's copy of the JoystickBinding of the device axis it is bound to;
// the rest stays opaque (naming those fields changes
// OptionMenu_DrawResultsRallyInfo in GameInfo.cpp).
struct ControllerDataUnk0x210 {
    BOOL present;
    BYTE field_0x4[16];   // range, deadzone, saturation, position
};

struct ControllerData {
    unsigned int field_0x0;
    unsigned int field_0x4;
    char name[MAX_PATH];
    DWORD hasAxes;              // a joystick or mouse
    DWORD analogSteering;       // steering from axisIndices[0], [1]
    DWORD analogPedals;         // throttle and brake from axisIndices[2], [3]
    DWORD forceFeedback;        // the device has force feedback
    DWORD field_0x11c;
    DWORD throttleOnPositive;   // one pedal axis: throttle on its positive half
    DWORD smoothSteering;       // Car::steeringInputMode 1 (eased), else 2 (direct)
    union {
        unsigned short actionMasks[10]; // 0x128 indexed action/device masks
        struct {
            unsigned short steerLeftMask;
            unsigned short steerRightMask;
            unsigned short throttleMask;
            unsigned short brakeMask;
            unsigned short handbrakeMask;
            unsigned short shiftUpMask;
            unsigned short shiftDownMask;
            unsigned short cameraCycleMask;
            unsigned short driverCycleMask;
            unsigned short backMask;
        };
    };
    unsigned short field_0x13c;
    BYTE keys[9];            // key (DIK_*) of each keyboard binding
    char keyNames[9][20];           // 0x147 text shown for each binding
    BYTE field_0x1fb[0x15];
    ControllerDataUnk0x210 axisBindings[10];
    unsigned short axisIndices[10];
    BYTE index;                     // 0x2ec
    BYTE field_0x2ed[3];
};

struct ControllerInfo {
    int field_0x0;
    int field_0x4;
    BYTE field_0x8;
    BYTE field_0x9;
    BYTE field_0xa_padding[308];
    BYTE field_0x13e;
    BYTE field_0x13f;
    BYTE field_0x140;
    BYTE field_0x141;
    BYTE field_0x142;
    BYTE field_0x143;
    BYTE field_0x144;
    BYTE field_0x145;
    BYTE field_0x146;
    BYTE pad[4185];
};

// PORT: the DirectInput GUIDs and data formats are gone.

struct InputDeviceState {
    DeviceInfo availableDevices[8];
    InputDevice *keyboard;
};
CMR2_LAYOUT_CHECK(InputDeviceStateSize, sizeof(InputDeviceState) == 0x2864);

struct InputFeedbackState {
    ForceFeedbackDevice devices[8];
    DWORD gains[8];
};
CMR2_LAYOUT_CHECK(InputFeedbackStateSize, sizeof(InputFeedbackState) == 0x1c0);

#define m_availableDevices m_deviceState.availableDevices
#define m_pDirectInputKeyboard m_deviceState.keyboard
#define m_forceFeedbackDevices m_feedbackState.devices
#define m_unk0x00666ec8 m_feedbackState.gains

class CInput {
public:
    static void *m_lpDirectInput7;          // PORT: unused (IDirectInput7)
    static DeviceCounts m_deviceCounts;
    static char m_strKeyboard[12];
    static InputDeviceState m_deviceState;
    static unsigned int m_keyboardDelay;
    static DWORD m_mouseGranularity;
    static unsigned int m_keyboardSpeed;


    static InputDevice *m_pDirectInputMouse;
    

    
    static USHORT m_axisOffsets[8];
    static InputDevice *m_joysticks[8];
    static CHAR m_strD[4];
    static CHAR m_strU[4];
    static CHAR m_strR[4];
    static CHAR m_strL[4];
    static InputFeedbackState m_feedbackState;
    static BOOL m_unk0x00666ee8;

    // GLOBAL: CMR2 0x00667000
    static char m_formatBuffer[512];
    // GLOBAL: CMR2 0x0059f7c8
    static BYTE m_keyboardState[256];
    // GLOBAL: CMR2 0x00520808
    static DWORD m_buttonMasks[24];
    // GLOBAL: CMR2 0x0059f8f0
    static DWORD m_unk0x0059f8f0;
    // GLOBAL: CMR2 0x0059f8f4
    static DWORD m_unk0x0059f8f4;
    // GLOBAL: CMR2 0x0059f8f8
    static DWORD m_unk0x0059f8f8;
    // GLOBAL: CMR2 0x0059f900
    static DWORD m_unk0x0059f900;
    // GLOBAL: CMR2 0x0059f90c
    static DWORD m_unk0x0059f90c;
    // GLOBAL: CMR2 0x0059f910
    static DWORD m_unk0x0059f910;

    // GLOBAL: CMR2 0x00516908
    static char m_strControllerInfoDir[32];

    // GLOBAL: CMR2 0x005334f0
    static BOOL m_hasLoadedControllerInfo;

    // GLOBAL: CMR2 0x00532250
    static ControllerData m_controllerInfo[6];

    // GLOBAL: CMR2 0x005168f4
    static unsigned short m_playerControllers[8];

    static BOOL DInputCreate(void);
    static InputDevice *DInputCreateDevice(int type, int index);
    static BOOL DInputRelease(void);
    static int CountAttachedInputDevices();
    static BOOL CountJoystickEnumerationCallback(void);
    static BOOL SetupKeyboard(void);
    static void SetupMouse(void);
    static void SetMouseCoopLevel(BOOL param1);
    static BOOL GetAttachedJoysticks(void);
    static BOOL SetupJoystick(int joystickIndex);
    static void SetupJoystickDeviceInfo(DeviceInfo *deviceStruct);
    static void SetJoystickAxisRange(int param1, int param2, DWORD range);
    static void SetJoystickAxisDeadzone(int deviceID, int axisID, DWORD deadzone);
    static void SetJoystickAxisSaturation(int deviceID, int axisID, DWORD saturation);
    static BOOL InitializeForceFeedbackDevice(int deviceID, InputDevice *pDevice);
    static bool IsForceFeedbackCallSuccessful(HRESULT hr);
    static void SetForceFeedbackAutocenter(DWORD param1, int deviceID);
    static void ResetForceFeedbackEffects(void);
    static BOOL ResetForceFeedbackEffectsAlt(void);
    static void DInputReleaseDevices(void);
    static void LoadControllerInfo(void);
    static void ApplyControllerButtonBindings(unsigned short param1);
    static void AssignUnmappedControllerActions(unsigned int deviceIndex, ControllerData *pController);
    static void SetDeviceKeyBinding(int param1, unsigned int param2, unsigned int param3);
    static DeviceInfo *GetAvailableDeviceRecord(int index);
    static BYTE FindFirstClearBindingBit(unsigned int param1);
    static void RebuildAvailableInputDevices(void);
    static int GetFirstPressedKey(void);
    static BOOL IsShiftPressed(void);
    static void ClearFirstJoystickControlBindings(void);
    static void QueueInputCharacter(int param1);
    static void QueueVirtualKeyPress(unsigned int param1);
    static void UpdateInputFrameDelta(void);
    static void UpdateAllAvailableDevices(void);
    // Race steps may update only the opponent. Keep edges until their device is read by a player.
    static void UpdateRaceDevices(unsigned int readyDevices);
    static void SetControllerForceFeedbackValue(unsigned short param1, DWORD param2);
    static void ReadKeyboardState(void);
    static int GetButtonIndexFromMask(unsigned int mask);
    static void SetInputRepeatTimingParameters(DWORD p1, DWORD p2, DWORD p3, DWORD p4, DWORD p5);
    static void SetInputRepeatTimingState(DWORD param1);
    static DWORD GetControllerField114(unsigned int param1);
    static DWORD GetControllerField120(unsigned int param1);
    static DWORD GetControllerField124(unsigned short param1);
    static unsigned int GetEnabledControllerAxisBinding(unsigned int param1, int param2);
    static BOOL IsControllerActionBound(int action, ControllerData *pController);
    static void SetForceFeedbackDeviceValue(DWORD param1, int index);
    static void StartForceFeedbackEffect(int effectIndex, int deviceIndex);
    static char *FormatString(LPCSTR format, ...);
    static void ReadMouse(DeviceInfo *pDevice);
    static void ReadKeyboardDevice(DeviceInfo *pDevice);
    static void ReadJoystick(DeviceInfo *pDevice);
    static void SetIndexedWordBinding(unsigned short *values, int index, unsigned short value);
    static void SetIndexedByteBinding(BYTE *values, int index, BYTE value);
    static DeviceInfo *UpdateDevice(int index);
    static int CreateForceFeedbackEffect(int effectType, DWORD duration, LONG coefficient, LONG offset, int triggerButton, int deviceIndex);
    static int CreateSpringEffect(DWORD duration, LONG coefficient, LONG offset, int triggerButton, int deviceIndex);
    static int CreateDamperEffect(DWORD duration, LONG coefficient, LONG offset, int triggerButton, int deviceIndex);
    static short GetButtonMapping(unsigned short controllerSlot, int action);
    static HRESULT SetEffectGain(int effectIndex, DWORD gain, int deviceIndex);
    static HRESULT SetEffectGainAndDirection(int effectIndex, DWORD gain, LONG direction, int deviceIndex);
    static int CreateConstantForceEffect(DWORD duration, LONG direction, LONG magnitude, DWORD attackTime, DWORD attackLevel, DWORD fadeTime, DWORD fadeLevel, int triggerButton, int deviceIndex);
    static HRESULT SetConditionCoefficient(int effectIndex, LONG coefficient, int deviceIndex);
    static void SaveControllerInfo(void);
    static void RefreshControllerConfigurations(void);
    static void InitDetectedControllerSlot(DeviceInfo *pDevice, int index);

    // GLOBAL: CMR2 0x00516904
    static unsigned short m_controllerCount;

    // GLOBAL: CMR2 0x00520868
    static unsigned int m_directionButtonMask;
    // GLOBAL: CMR2 0x0052086c
    static BOOL m_unk0x0052086c;
    // GLOBAL: CMR2 0x00665328
    // PORT: the parameters of each effect (DIEFFECT with its DICONDITION).
    static InputEffectParams m_forceFeedbackEffects[80];
};

#endif
