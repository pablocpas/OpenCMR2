#include "Input.h"
#include "GameInfo.h"
#include "Graphics.h"
#include "main.h"
#include "InstallInfo.h"
#include "Frontend.h"
#include "FileBuffer.h"
#include "Game.h"
#include "RallyData.h"

#include "port/sys.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// PORT: the DirectInput 7 data formats (c_dfDIMouse2, c_dfDIKeyboard,
// c_dfDIJoystick2 and their descriptor arrays) are gone: the input layer
// returns the state structures directly.

// PORT: the game reads the players of src/input through four device
// records. Records 0 and 1 are players 1 and 2 as keyboards: the buttons of
// every device of the player. Records 2 and 3 are the same players as
// joysticks, with their steering, throttle and brake as axes; the player
// drives with it (CInput::m_playerControllers) and it reads as a keyboard
// while the player's last driving inputs came from keys or buttons, so that
// the game steers as it did with keys (see UpdateDevice). The bits follow the original's
// keyboard: in the menus (CGameInfo::m_fixedMenuKeys) the cursor keys,
// Return, Escape, F1 and F2, and the pause's bit; on a stage bit 1 << action
// for the actions, Escape's bit for the pause as well, and F1, F2 and F4.
static int Input_PlayerOfDevice(int index)
{
    return index < 2 ? index : index - 2;
}

static BOOL Input_IsPressed(int player, int action)
{
    // An axis counts as pressed past 0.6 of its travel, as the original
    // turned a joystick's stick into directions.
    return Input_GetAction(player, action) > INPUT_FULL * 3 / 5;
}

static unsigned int Input_PlayerButtons(int player)
{
    static const unsigned int menuBits[6] = { 0x1, 0x2, 0x4, 0x8, 0x10, 0x20 };
    unsigned int buttons = 0;
    int i;

    if (CGameInfo::m_fixedMenuKeys != 0) {
        for (i = 0; i < 6; i++)
            if (Input_IsPressed(player, INPUT_MENU_LEFT + i))
                buttons |= menuBits[i];
        // The pause's bit too: the in-race menu closes with the button that
        // opened it (InRaceMenu_IsBackPressed), as Start does on a gamepad.
        if (Input_IsPressed(player, INPUT_PAUSE))
            buttons |= 1u << INPUT_PAUSE;
    } else {
        for (i = 0; i <= INPUT_PAUSE; i++)
            if (Input_IsPressed(player, i))
                buttons |= 1u << i;
        if (buttons & (1u << INPUT_PAUSE))
            buttons |= 0x400;
        if (CInput::m_keyboardState[INPUT_KEY_F4] & 0x80)
            buttons |= 0x4000;
    }
    if (CInput::m_keyboardState[INPUT_KEY_F1] & 0x80)
        buttons |= 0x1000;
    if (CInput::m_keyboardState[INPUT_KEY_F2] & 0x80)
        buttons |= 0x2000;
    return buttons;
}

// The configuration of a player's keyboard (records 0 and 1): no masks, so
// that GetButtonMapping gives bit 1 << action. Not InitDetectedControllerSlot,
// which also names the keys for the original's controls pages, gone, and
// runs before the texts are loaded: "INVALID TEXT STRING 4" overflowed a
// 20-byte key name.
static void Input_InitKeyboardConfiguration(int index)
{
    ControllerData *pController;

    pController = &CInput::m_controllerInfo[index];
    memset(pController, 0, sizeof(*pController));
    pController->index = (BYTE)index;
    strcpy(pController->name, CInput::m_availableDevices[index].deviceInstanceName);
    pController->field_0x0 = 0x8000;
    pController->field_0x4 = 0x8000;
}

// The configuration of a player's joystick (records 2 and 3): its axes,
// and bit 1 << action for every action, as the keyboard records give them.
static void Input_InitPlayerConfiguration(int index)
{
    static const BYTE axes[4] = { 0, 0, 5, 2 };   // steering left, right, throttle, brake
    ControllerData *pController;
    DeviceInfo *pDevice;
    int i;

    pController = &CInput::m_controllerInfo[index];
    pDevice = &CInput::m_availableDevices[index];
    memset(pController, 0, sizeof(*pController));
    pController->index = (BYTE)index;
    strcpy(pController->name, pDevice->deviceInstanceName);
    pController->field_0x0 = 0x8000;
    pController->field_0x4 = 0x8000;
    pController->hasAxes = 1;
    pController->analogSteering = 1;
    pController->analogPedals = 1;
    pController->forceFeedback = pDevice->supportsForceFeedback;
    pController->smoothSteering = Input_IsSmoothSteering(index - 2);
    for (i = 0; i < 10; i++)
        pController->actionMasks[i] = (unsigned short)(1 << i);
    for (i = 0; i < 4; i++) {
        pController->axisBindings[i] = *(ControllerDataUnk0x210 *)&pDevice->joystick.bindings[axes[i]];
        pController->axisIndices[i] = axes[i];
    }
}

// GLOBAL: CMR2 0x00511758
// IID_IDirectInput7A

// GLOBAL: CMR2 0x0059f8c8
void *CInput::m_lpDirectInput7;

// GLOBAL: CMR2 0x00520874
char CInput::m_strKeyboard[12] = "Keyboard";

// GLOBAL: CMR2 0x0059f8cc
DeviceCounts CInput::m_deviceCounts; // device count or something

// GLOBAL: CMR2 0x0059ce48
InputDeviceState CInput::m_deviceState;

// GLOBAL: CMR2 0x0059f8d4
DWORD CInput::m_mouseGranularity;

// GLOBAL: CMR2 0x0059f8d8
unsigned int CInput::m_keyboardDelay;

// GLOBAL: CMR2 0x0059f8dc
unsigned int CInput::m_keyboardSpeed;


// GLOBAL: CMR2 0x0059f7c4
InputDevice *CInput::m_pDirectInputMouse = NULL;

// GLOBAL: CMR2 0x00511400
USHORT CInput::m_axisOffsets[8] = {0, 4, 8, 12, 16, 20, 24, 28};

// GLOBAL: CMR2 0x0059f6b0
InputDevice *CInput::m_joysticks[8];

// GLOBAL: CMR2 0x00520880
CHAR CInput::m_strD[4] = " D";
// GLOBAL: CMR2 0x00520884
CHAR CInput::m_strU[4] = " U";
// GLOBAL: CMR2 0x00520888
CHAR CInput::m_strR[4] = " R";
// GLOBAL: CMR2 0x0052088C
CHAR CInput::m_strL[4] = " L";

// GLOBAL: CMR2 0x00666d28
InputFeedbackState CInput::m_feedbackState;

// GLOBAL: CMR2 0x00666ee8
BOOL CInput::m_unk0x00666ee8 = FALSE;

char CInput::m_formatBuffer[512];
BYTE CInput::m_keyboardState[256];
DWORD CInput::m_buttonMasks[24] = {
    0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000,
    0x10000, 0x20000, 0x40000, 0x80000, 0x100000, 0x200000, 0x400000, 0x800000, 0x1, 0x2, 0x4, 0x8
};
unsigned int CInput::m_directionButtonMask = 0xf;
BOOL CInput::m_unk0x0052086c = TRUE;
InputEffectParams CInput::m_forceFeedbackEffects[80];
DWORD CInput::m_unk0x0059f8f0;
DWORD CInput::m_unk0x0059f8f4;
DWORD CInput::m_unk0x0059f8f8;
DWORD CInput::m_unk0x0059f900;
DWORD CInput::m_unk0x0059f90c;
DWORD CInput::m_unk0x0059f910;

unsigned short CInput::m_controllerCount = 1;
char CInput::m_strControllerInfoDir[32] = "%s\\Configuration\\Controller.rcf";
BOOL CInput::m_hasLoadedControllerInfo;
ControllerData CInput::m_controllerInfo[6];
unsigned short CInput::m_playerControllers[8] = {0, 1, 2, 3, 0, 0, 0, 0};

// FUNCTION: CMR2 0x0049fd30
// PORT: SDL is initialised by the platform layer.
BOOL CInput::DInputCreate(void) {
    CGame::RegisterCallback(DInputRelease, NULL);
    return TRUE;
}

// FUNCTION: CMR2 0x0049fe30
// PORT: type 0 keyboard, 1 mouse; the cooperative levels become the mouse
// capture below. Joysticks are the players' devices of src/input.
InputDevice *CInput::DInputCreateDevice(int type, int index) {
    InputDevice *pDevice;

    if (type == 0)
        return Input_OpenKeyboard();
    if (type == 1) {
        pDevice = Input_OpenMouse();
        if (pDevice != NULL)
            Input_SetMouseExclusive(pDevice, g_pGraphics->isFullscreen);
        return pDevice;
    }
    return NULL;
}

// FUNCTION: CMR2 0x0049fd60
BOOL CInput::DInputRelease(void) {
    DInputReleaseDevices();
    m_lpDirectInput7 = NULL;
    return TRUE;
};

// FUNCTION: CMR2 0x0049fd90
void CInput::DInputReleaseDevices(void) {
    int i;

    if (m_pDirectInputKeyboard != NULL) {
        Input_CloseDevice(m_pDirectInputKeyboard);
        m_pDirectInputKeyboard = NULL;
    }

    if (m_pDirectInputMouse != NULL) {
        Input_CloseDevice(m_pDirectInputMouse);
        m_pDirectInputMouse = NULL;
    }

    CountAttachedInputDevices();

    // PORT: the force-feedback devices of the players belong to src/input.
    for (i = 0; i < 4; i++)
        m_joysticks[i] = NULL;
}

// FUNCTION: CMR2 0x0049ef90
int CInput::CountAttachedInputDevices(void) {
  int i;

  m_deviceCounts.attachedJoystickCount = 0;
  // PORT: one joystick per player (see SetupJoystick).
  for (i = 0; i < INPUT_PLAYERS; i++)
      CountJoystickEnumerationCallback();
  return m_deviceCounts.keyboardCount + m_deviceCounts.attachedJoystickCount;
}

// FUNCTION: CMR2 0x0049f6b0
BOOL CInput::CountJoystickEnumerationCallback(void) {
    m_deviceCounts.attachedJoystickCount++;
    return TRUE;
}

// match 89%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0049f0e0
BOOL CInput::SetupKeyboard(void) {
    unsigned int uVar1;
    int iVar3 = 0;

    do {
        uVar1 = m_deviceCounts.nextDevice;
        m_availableDevices[uVar1].type = 1;
        m_availableDevices[uVar1].joystickIndex = FALSE;

        // PORT: copy text directly; avoid calling sprintf through an incompatible type.
        strcpy(m_availableDevices[uVar1].deviceInstanceName, m_strKeyboard);
        strcpy(m_availableDevices[m_deviceCounts.nextDevice].deviceProductName, m_strKeyboard);
        
        uVar1 = m_deviceCounts.nextDevice;
        m_availableDevices[uVar1].supportsForceFeedback = FALSE;

        if (!iVar3) {
            m_availableDevices[uVar1].keyboard.steerLeftKey = 0xcb;
            m_availableDevices[uVar1].keyboard.steerRightKey = 0xcd;
            m_availableDevices[uVar1].keyboard.throttleKey = 0xc8;
            m_availableDevices[uVar1].keyboard.brakeKey = 0xd0;
            m_availableDevices[uVar1].keyboard.handbrakeKey = 0x39;
            m_availableDevices[uVar1].keyboard.shiftUpKey = 0x1b;
            m_availableDevices[uVar1].keyboard.shiftDownKey = 0x1a;
            m_availableDevices[uVar1].keyboard.cameraCycleKey = 0x2e;
            m_availableDevices[uVar1].keyboard.driverCycleKey = 0x13;
        } else {
            m_availableDevices[uVar1].keyboard.steerLeftKey = 0x4b;
            m_availableDevices[uVar1].keyboard.steerRightKey = 0x4d;
            m_availableDevices[uVar1].keyboard.throttleKey = 0x48;
            m_availableDevices[uVar1].keyboard.brakeKey = 0x50;
            m_availableDevices[uVar1].keyboard.handbrakeKey = 0x51;
            m_availableDevices[uVar1].keyboard.shiftUpKey = 0xc9;
            m_availableDevices[uVar1].keyboard.shiftDownKey = 0xd1;
            m_availableDevices[uVar1].keyboard.cameraCycleKey = 0x49;
            m_availableDevices[uVar1].keyboard.driverCycleKey = 0x47;            
        }

        m_availableDevices[uVar1].keyboard.backKey = 0x0;
        m_availableDevices[uVar1].keyboard.field_0x474 = 0x3b;
        m_availableDevices[uVar1].keyboard.field_0x475 = 0x3c;
        m_availableDevices[uVar1].keyboard.field_0x476 = 0x3e;
        m_availableDevices[uVar1].keyboard.field_0x477 = 0x0;
        m_availableDevices[uVar1].keyboard.field_0x472 = 0x1;
        m_availableDevices[uVar1].keyboard.field_0x473 = 0x0;

        m_deviceCounts.nextDevice = m_deviceCounts.nextDevice + 1;
        iVar3++;
    } while (iVar3 < 2);

    // PORT: the system repeat delay and interval (SystemParametersInfo).
    {
        DWORD delay, interval;

        Input_GetKeyRepeat(&delay, &interval);
        m_keyboardDelay = delay;
        m_keyboardSpeed = interval;
    }

    m_pDirectInputKeyboard = DInputCreateDevice(0, 0);
    if (m_pDirectInputKeyboard != NULL)
        return TRUE;

    return FALSE;
}

// FUNCTION: CMR2 0x0049f060
// PORT: no buffered data or granularity (it was 1 for every mouse).
void CInput::SetupMouse(void) {
    m_pDirectInputMouse = DInputCreateDevice(1, 0);
    m_mouseGranularity = 1;
    SetMouseCoopLevel(1);
}

// FUNCTION: CMR2 0x0049f000
void CInput::SetMouseCoopLevel(BOOL param1) {
    if (m_pDirectInputMouse != NULL)
        Input_SetMouseExclusive(m_pDirectInputMouse, param1 && g_pGraphics->isFullscreen);
}

// FUNCTION: CMR2 0x0049f690
// PORT: the players' joysticks, once the connected devices are known.
BOOL CInput::GetAttachedJoysticks(void) {
    int i;

    Input_Update();
    Input_DevicesChanged();
    for (i = 0; i < INPUT_PLAYERS; i++)
        SetupJoystick(i);
    return TRUE;
}

// DirectInput enumeration callback for one joystick: creates the device, names
// its buttons (DIJOFS_BUTTON) and the four directions of its first POV hat
// (DIJOFS_POV(0)), and enables autocentre-off force feedback when present.
// FUNCTION: CMR2 0x0049f6d0
// PORT: sets up the joystick of player joystickIndex: its steering, brake
// and throttle axes (SetupJoystickDeviceInfo), no buttons of its own, and
// the force feedback of a device of the player that has it.
BOOL CInput::SetupJoystick(int joystickIndex) {
    DeviceInfo *pDeviceInfo;
    InputDevice *pFeedback;
    int i;

    pDeviceInfo = &m_availableDevices[m_deviceCounts.nextDevice];
    pDeviceInfo->type = 3;
    sprintf(pDeviceInfo->deviceInstanceName, "Player %d", joystickIndex + 1);
    strcpy(pDeviceInfo->deviceProductName, pDeviceInfo->deviceInstanceName);
    pDeviceInfo->joystickIndex = m_deviceCounts.joystickCount;
    pFeedback = Input_GetFeedbackDevice(joystickIndex);
    m_joysticks[m_deviceCounts.joystickCount] = pFeedback;

    SetupJoystickDeviceInfo(pDeviceInfo);
    for (i = 0; i < pDeviceInfo->joystick.controlCount; i++) {
        if (pDeviceInfo->joystick.bindings[i].present != FALSE) {
            SetJoystickAxisRange(m_deviceCounts.nextDevice, i, pDeviceInfo->joystick.bindings[i].range);
            SetJoystickAxisDeadzone(m_deviceCounts.nextDevice, i, pDeviceInfo->joystick.bindings[i].deadzone);
            SetJoystickAxisSaturation(m_deviceCounts.nextDevice, i, pDeviceInfo->joystick.bindings[i].saturation);
        }
    }

    if (pFeedback != NULL) {
        pDeviceInfo->supportsForceFeedback = TRUE;
        Input_SetAutocenter(pFeedback, FALSE);
        InitializeForceFeedbackDevice(m_deviceCounts.nextDevice, pFeedback);
    } else {
        pDeviceInfo->supportsForceFeedback = FALSE;
    }
    pDeviceInfo->buttonCount = 0;

    m_deviceCounts.joystickCount++;
    m_deviceCounts.nextDevice++;
    return TRUE;
}

// FUNCTION: CMR2 0x0049fad0
void CInput::SetupJoystickDeviceInfo(DeviceInfo *deviceInfo) {
    int i;

    deviceInfo->joystick.controlCount = 0;
    for (i = 0; i < 8; i++) {
        // PORT: a player's joystick has steering (X), brake (Z) and
        // throttle (Rz), see ReadJoystick.
        if (i == 0 || i == 2 || i == 5) {
            deviceInfo->joystick.bindings[i].present = TRUE;
            deviceInfo->joystick.bindings[i].deadzone = 0xc8;
            deviceInfo->joystick.bindings[i].range = 0x10000;
            deviceInfo->joystick.bindings[i].saturation = 0x2710;

            deviceInfo->joystick.controlCount ++;
        }
        else {
            deviceInfo->joystick.bindings[i].present = FALSE;
        }
    }
}

// FUNCTION: CMR2 0x0049ee10
// PORT: kept for the controls pages; the axes are scaled by their bindings
// (src/input, controls.ini).
void CInput::SetJoystickAxisRange(int deviceID, int axisID, DWORD range) {
    DeviceInfo * pDeviceInfo = &m_availableDevices[deviceID];

    pDeviceInfo->joystick.bindings[axisID].range = range;
}

// FUNCTION: CMR2 0x0049ee90
// PORT: kept for the controls pages; the axes are scaled by their bindings
// (src/input, controls.ini).
void CInput::SetJoystickAxisDeadzone(int deviceID, int axisID, DWORD deadzone) {
    DeviceInfo * pDeviceInfo = &m_availableDevices[deviceID];

    pDeviceInfo->joystick.bindings[axisID].deadzone = deadzone;
}

// FUNCTION: CMR2 0x0049ef10
// PORT: kept for the controls pages; the axes are scaled by their bindings
// (src/input, controls.ini).
void CInput::SetJoystickAxisSaturation(int deviceID, int axisID, DWORD saturation) {
    DeviceInfo * pDeviceInfo = &m_availableDevices[deviceID];

    pDeviceInfo->joystick.bindings[axisID].saturation = saturation;
}

// FUNCTION: CMR2 0x004aae20
BOOL CInput::InitializeForceFeedbackDevice(int deviceID, InputDevice *pDevice) {
    if (m_forceFeedbackDevices[deviceID].field_0x0 == 0) {
        m_forceFeedbackDevices[deviceID].device = pDevice;
        if (g_pGraphics->isFullscreen)
            Input_StopAllEffects(pDevice);

        SetForceFeedbackAutocenter(0, deviceID);
        m_forceFeedbackDevices[deviceID].field_0x0 = TRUE;
        ResetForceFeedbackEffects();
    }

    if (m_unk0x00666ee8 == FALSE) {
        m_unk0x00666ee8 = TRUE;
        CGame::RegisterCallback(ResetForceFeedbackEffectsAlt, NULL);
    }

    return TRUE;
}

// FUNCTION: CMR2 0x004ab5f0
bool CInput::IsForceFeedbackCallSuccessful(HRESULT hr) {
    return hr == 0;
}

// FUNCTION: CMR2 0x004aafd0
void CInput::SetForceFeedbackAutocenter(DWORD param1, int deviceID) {
    if (m_forceFeedbackDevices[deviceID].field_0x0 != FALSE)
        Input_SetAutocenter(m_forceFeedbackDevices[deviceID].device, param1 != 0);
}

// 96.55% match, only concern is this
// 0x4aaf35	-cmp edi, 0x666ed4
//          +cmp edi, CInput::m_dinputRefGuidKeyboard (DATA) (Input.cpp:560) <-- why  are you that
// FUNCTION: CMR2 0x004aaf00
void CInput::ResetForceFeedbackEffects(void) {
    int device;
    int i;

    for (device = 0; device < 8; device++) {
        ForceFeedbackDevice *pDevice = &m_forceFeedbackDevices[device];

        if (pDevice->field_0x0 != FALSE) {
            for (i = 0; i < 10; i++) {
                if (pDevice->effects[i] != NULL) {
                    Input_DestroyEffect(pDevice->effects[i]);
                    pDevice->effects[i] = NULL;
                }
            }
            pDevice->field_0x8 = 0;
        }
    }
}

// 96.77% match, only concern is this
// 0x4aaeda	-cmp edi, 0x666ec8
// 	        +cmp edi, CInput::m_dinputRefGuidKeyboard (DATA) (Input.cpp:587)  <-- why  are you that
// FUNCTION: CMR2 0x004aaea0
BOOL CInput::ResetForceFeedbackEffectsAlt(void) {
    int device;
    int i;

    for (device = 0; device < 8; device++) {
        ForceFeedbackDevice *pDevice = &m_forceFeedbackDevices[device];

        if (pDevice->field_0x0 != FALSE) {
            for (i = 0; i < 10; i++) {
                if (pDevice->effects[i] != NULL) {
                    Input_DestroyEffect(pDevice->effects[i]);
                    pDevice->effects[i] = NULL;
                }
            }
            pDevice->field_0x8 = 0;
            pDevice->field_0x0 = 0;
        }
    }
    m_unk0x00666ee8 = FALSE;
    return TRUE;
}

// FUNCTION: CMR2 0x0040bf70
// PORT: the configurations follow the players' devices
// (RefreshControllerConfigurations). The keys of an older Controller.rcf
// become the keyboard bindings when controls.ini does not exist yet, and
// controls.ini is written.
void CInput::LoadControllerInfo(void) {
    ControllerData *fileBuffer;

    m_hasLoadedControllerInfo = 0;
    sprintf(CFrontend::m_stringDest, m_strControllerInfoDir, CInstallInfo::GetGameHDPath());

    fileBuffer = (ControllerData*)CFileBuffer::GetGenericFileBuffer(CFrontend::m_stringDest, TRUE);
    if (fileBuffer != NULL) {
        if (CGenericFileLoader::GetGenericFileSize() == 0x11a4) {
            Input_ImportKeyboardKeys(0, fileBuffer[0].keys);
            Input_ImportKeyboardKeys(1, fileBuffer[1].keys);
        }
        CFileBuffer::FreeGenericFileBuffer(fileBuffer);
    }
    RefreshControllerConfigurations();
    Input_SaveBindings();
}

// FUNCTION: CMR2 0x0040be90
void CInput::ApplyControllerButtonBindings(unsigned short param1) {
    int uVar1;
    unsigned int raw;
    unsigned short uVar2;
    ControllerData * pController;

    raw = param1;
    uVar2 = param1;
    uVar1 = m_playerControllers[uVar2];
    pController = &m_controllerInfo[uVar1];

    if (uVar1 > 1) {
        AssignUnmappedControllerActions(raw, pController);
    }

    SetDeviceKeyBinding(uVar2, 1, pController->keys[0]);
    SetDeviceKeyBinding(uVar2, 2, pController->keys[1]);
    SetDeviceKeyBinding(uVar2, 4, pController->keys[2]);
    SetDeviceKeyBinding(uVar2, 8, pController->keys[3]);
    SetDeviceKeyBinding(uVar2, 0x10, pController->keys[4]);
    SetDeviceKeyBinding(uVar2, 0x20, pController->keys[5]);
    SetDeviceKeyBinding(uVar2, 0x40, pController->keys[6]);
    SetDeviceKeyBinding(uVar2, 0x80, pController->keys[7]);
    SetDeviceKeyBinding(uVar2, 0x100, pController->keys[8]);
}

// FUNCTION: CMR2 0x0040c440
void CInput::AssignUnmappedControllerActions(unsigned int deviceIndex, ControllerData* pController) {
    unsigned short* pActionMask;
    unsigned short assignedMask;
    unsigned char freeBit;
    int action;
    unsigned short usedMasks;
    
    action = 0;
    pActionMask = pController->actionMasks;
    
    do {
        if (*pActionMask == 0) {
            if (pController->keys[action] != 0) {
                
                usedMasks = pController->backMask | pController->cameraCycleMask |
                                pController->shiftDownMask | pController->shiftUpMask |
                                pController->handbrakeMask | pController->brakeMask |
                                pController->throttleMask | pController->steerRightMask |
                                pController->steerLeftMask | 0x400;

                freeBit = FindFirstClearBindingBit(usedMasks & 0xffff);
                
                assignedMask = 1 << freeBit;
                
                SetDeviceKeyBinding(deviceIndex & 0xffff, assignedMask & 0xffff, pController->keys[action]);
                SetDeviceKeyBinding(deviceIndex & 0xffff, (1 << action) & 0xffff, 0);
                
                *pActionMask = assignedMask;
            }
        }
        
        action++;
        pActionMask++;
    } while (action < 10);
}

// FUNCTION: CMR2 0x0049eb90
void CInput::SetDeviceKeyBinding(int param1, unsigned int param2, unsigned int param3) {
    DeviceInfo* device = &m_availableDevices[param1];
    switch(param2) {
        case 1:
            device->keyboard.steerLeftKey = param3;
            break;

        case 2:
            device->keyboard.steerRightKey = param3;
            break;

        case 4:
            device->keyboard.throttleKey = param3;
            break;

        case 8:
            device->keyboard.brakeKey = param3;
            break;

        case 0x10:
            device->keyboard.handbrakeKey = param3;
            break;

        case 0x20:
            device->keyboard.shiftUpKey = param3;
            break;

        case 0x40:
            device->keyboard.shiftDownKey = param3;
            break;
        
        case 0x80:
            device->keyboard.cameraCycleKey = param3;
            break;

        case 0x100:
            device->keyboard.driverCycleKey = param3;
            break;

        case 0x200:
            device->keyboard.backKey = param3;
            break;

        case 0x400:
            device->keyboard.field_0x472 = param3;
            break;

        case 0x800:
            device->keyboard.field_0x473 = param3;
            break;
        
        case 0x1000:
            device->keyboard.field_0x474 = param3;
            break;

        case 0x2000:
            device->keyboard.field_0x475 = param3;
            break;
        
        case 0x4000:
            device->keyboard.field_0x476 = param3;
            break;

        case 0x8000:
            device->keyboard.field_0x477 = param3;
            break;
    }
}

// FUNCTION: CMR2 0x0040c530 
BYTE CInput::FindFirstClearBindingBit(unsigned int param1) {
    int iVar1 = 0;

    while (iVar1 < 32) {
        if (!(param1 & 1)) break;
        
        param1 >>= 1;
        iVar1++;
    }

    return iVar1;
}

static unsigned int s_pendingRacePresses[8];

// FUNCTION: CMR2 0x0049eb50
void CInput::RebuildAvailableInputDevices(void)
{
    int i;

    DInputReleaseDevices();
    memset(s_pendingRacePresses, 0, sizeof(s_pendingRacePresses));
    m_deviceCounts.keyboardCount = 2;
    m_deviceCounts.nextDevice = 0;
    m_deviceCounts.joystickCount = 0;
    for (i = 0; i < 8; i++) {
        m_availableDevices[i].type = -1;
        m_availableDevices[i].joystickIndex = -1;
    }
    SetupKeyboard();
    SetupMouse();
    GetAttachedJoysticks();
}

// PORT: the original enumerated joysticks once, at startup. Sets up the
// players' joysticks again when a device was connected or disconnected, for
// the force feedback of the device that came or went. Called in the menus.
void Input_RescanChangedJoysticks(void)
{
    if (!Input_DevicesChanged())
        return;
    CInput::ResetForceFeedbackEffectsAlt();
    CInput::RebuildAvailableInputDevices();
    CInput::RefreshControllerConfigurations();
    Input_SaveBindings();   // the profile of a new joystick or wheel
}

// FUNCTION: CMR2 0x0049edd0
int CInput::GetFirstPressedKey(void)
{
    int key;
    int result;

    result = -1;
    for (key = 0; key < 0xdd; key++) {
        if (m_keyboardState[key] & 0x80) {
            result = key;
            break;
        }
    }
    return result;
}

// FUNCTION: CMR2 0x0049edf0
BOOL CInput::IsShiftPressed(void)
{
    if ((m_keyboardState[INPUT_KEY_LSHIFT] & 0x80) == 0 && (m_keyboardState[INPUT_KEY_RSHIFT] & 0x80) == 0)
        return FALSE;
    return TRUE;
}

// FUNCTION: CMR2 0x0049efc0
void CInput::ClearFirstJoystickControlBindings(void)
{
    int i;

    for (i = 0; i < 8; i++) {
        if (m_availableDevices[i].type == 2) {
            m_availableDevices[i].joystick.bindings[0].range = 0;
            m_availableDevices[i].joystick.bindings[0].present = 0;
            m_availableDevices[i].joystick.controlCount = 0;
            return;
        }
    }
}

// FUNCTION: CMR2 0x0049f300
void CInput::ReadKeyboardState(void)
{
    if (m_pDirectInputKeyboard != NULL)
        Input_ReadKeyboard(m_pDirectInputKeyboard, m_keyboardState);
}

// FUNCTION: CMR2 0x0049fd00
int CInput::GetButtonIndexFromMask(unsigned int mask)
{
    int i;

    for (i = 0; i < 24; i++) {
        if (m_buttonMasks[i] & mask)
            break;
    }
    if (i == 24)
        i = -1;
    return i;
}

// FUNCTION: CMR2 0x0049ff80
void CInput::SetInputRepeatTimingParameters(DWORD p1, DWORD p2, DWORD p3, DWORD p4, DWORD p5)
{
    m_unk0x0059f8f8 = p1;
    m_unk0x0059f910 = p2;
    m_unk0x0059f8f4 = p3;
    m_unk0x0059f8f0 = p4;
    m_unk0x0059f90c = p5;
}

// FUNCTION: CMR2 0x0049ffc0
void CInput::SetInputRepeatTimingState(DWORD param1)
{
    m_unk0x0059f900 = param1;
}

// FUNCTION: CMR2 0x0040be00
DWORD CInput::GetControllerField114(unsigned int param1)
{
    return m_controllerInfo[m_playerControllers[(unsigned short)param1]].analogPedals;
}

// FUNCTION: CMR2 0x0040be30
DWORD CInput::GetControllerField120(unsigned int param1)
{
    return m_controllerInfo[m_playerControllers[(unsigned short)param1]].throttleOnPositive;
}

// FUNCTION: CMR2 0x0040be60
DWORD CInput::GetControllerField124(unsigned short param1)
{
    return m_controllerInfo[m_playerControllers[(unsigned short)param1]].smoothSteering;
}

// FUNCTION: CMR2 0x0040c210
unsigned int CInput::GetEnabledControllerAxisBinding(unsigned int param1, int param2)
{
    unsigned int controller;

    controller = m_playerControllers[(unsigned short)param1];
    if (m_controllerInfo[controller].axisBindings[param2].present != 0)
        return m_controllerInfo[controller].axisIndices[param2];
    return -1;
}

// FUNCTION: CMR2 0x0040c270
BOOL CInput::IsControllerActionBound(int action, ControllerData *pController)
{
    if (pController->actionMasks[action] != 0 && pController->keys[action] != 0)
        return TRUE;
    return FALSE;
}

// FUNCTION: CMR2 0x004aaf50
void CInput::SetForceFeedbackDeviceValue(DWORD param1, int index)
{
    m_unk0x00666ec8[index] = param1;
}

// FUNCTION: CMR2 0x004aaf70
void CInput::StartForceFeedbackEffect(int effectIndex, int deviceIndex)
{
    ForceFeedbackDevice *pDevice;
    InputEffect *pEffect;

    pDevice = &m_forceFeedbackDevices[deviceIndex];
    if (pDevice->field_0x0 != 0) {
        pEffect = pDevice->effects[effectIndex];
        if (pEffect != NULL && !Input_IsEffectPlaying(pEffect))
            Input_StartEffect(pEffect);
    }
}

// FUNCTION: CMR2 0x004ab600
char *CInput::FormatString(LPCSTR format, ...)
{
    va_list args;

    va_start(args, format);
    vsprintf(m_formatBuffer, format, args);
    return m_formatBuffer;
}

// FUNCTION: CMR2 0x0049f360
void CInput::ReadMouse(DeviceInfo *pDevice)
{
    InputMouseState mouseState;
    int i;

    pDevice->held = 0;
    if (m_pDirectInputMouse == NULL || !Input_ReadMouse(m_pDirectInputMouse, &mouseState))
        return;

    pDevice->joystick.bindings[0].position += mouseState.lX * 150;
    if (pDevice->joystick.bindings[0].position > 0x10000)
        pDevice->joystick.bindings[0].position = 0x10000;
    if (pDevice->joystick.bindings[0].position < -0x10000)
        pDevice->joystick.bindings[0].position = -0x10000;
    pDevice->joystick.bindings[1].position += mouseState.lY * 150;
    pDevice->joystick.bindings[2].position += mouseState.lZ * 150;

    for (i = 0; i < 8; i++) {
        if (i < 24 && (mouseState.rgbButtons[i] & 0x80))
            pDevice->held |= m_buttonMasks[i];
    }
}

// FUNCTION: CMR2 0x0049f480
// PORT: the buttons of a player (Input_PlayerButtons).
void CInput::ReadKeyboardDevice(DeviceInfo *pDevice)
{
    pDevice->held = Input_PlayerButtons(Input_PlayerOfDevice(pDevice - m_availableDevices));
}

// FUNCTION: CMR2 0x0049fb70
// PORT: a player's joystick: steering on X, the throttle on Rz and the brake
// on Z, each read as a wheel's pedal (the maximum when released), and the
// player's buttons.
void CInput::ReadJoystick(DeviceInfo *pDevice)
{
    int player;

    player = Input_PlayerOfDevice(pDevice - m_availableDevices);
    pDevice->joystick.bindings[0].position = Input_GetAction(player, INPUT_STEER_RIGHT) - Input_GetAction(player, INPUT_STEER_LEFT);
    pDevice->joystick.bindings[2].position = INPUT_FULL - 2 * Input_GetAction(player, INPUT_BRAKE);
    pDevice->joystick.bindings[5].position = INPUT_FULL - 2 * Input_GetAction(player, INPUT_THROTTLE);
    pDevice->held = Input_PlayerButtons(player);
}

// FUNCTION: CMR2 0x0040c130
void CInput::SetIndexedWordBinding(unsigned short *values, int index, unsigned short value)
{
    switch (index) {
    case 0: values[0] = value; break;
    case 1: values[1] = value; break;
    case 2: values[2] = value; break;
    case 3: values[3] = value; break;
    case 4: values[4] = value; break;
    case 5: values[5] = value; break;
    case 6: values[6] = value; break;
    case 7: values[7] = value; break;
    case 8: values[8] = value; break;
    case 9: values[9] = value; break;
    }
}

// FUNCTION: CMR2 0x0040c550
void CInput::SetIndexedByteBinding(BYTE *values, int index, BYTE value)
{
    switch (index) {
    case 0: values[0] = value; break;
    case 1: values[1] = value; break;
    case 2: values[2] = value; break;
    case 3: values[3] = value; break;
    case 4: values[4] = value; break;
    case 5: values[5] = value; break;
    case 6: values[6] = value; break;
    case 7: values[7] = value; break;
    case 8: values[8] = value; break;
    }
}

// FUNCTION: CMR2 0x0049e960
DeviceInfo *CInput::UpdateDevice(int index)
{
    DeviceInfo *pDevice;
    unsigned int oldButtons;
    unsigned int buttons;
    unsigned int dirButtons;
    int now;

    if (index < 0 || index >= 8)
        return NULL;

    pDevice = &m_availableDevices[index];
    oldButtons = pDevice->held;
    if (index > 1) {
        if (pDevice->joystickIndex < 0 || pDevice->joystickIndex > 3)
            return NULL;
    }

    // PORT: the devices are read once, with record 0. A player's joystick
    // reads as a keyboard while the player drives with keys or buttons only;
    // its configuration takes the steering, and the throttle and brake, from
    // its axes while they come from axes (a D-pad steers as keys do while
    // the triggers stay analogue).
    if (index == 0) {
        Input_SetLocalPlayers(RallyDataState());
        Input_Update();
    }
    if (index == 2 || index == 3) {
        m_controllerInfo[index].analogSteering = Input_IsAnalogSteering(index - 2);
        m_controllerInfo[index].analogPedals = Input_IsAnalogPedals(index - 2);
        m_controllerInfo[index].smoothSteering = Input_IsSmoothSteering(index - 2);
        pDevice->type = m_controllerInfo[index].analogSteering || m_controllerInfo[index].analogPedals ? 3 : 1;
    }

    switch (pDevice->type) {
    case 1:
        if (index == 0)
            ReadKeyboardState();
        ReadKeyboardDevice(pDevice);
        break;
    case 0:
    case 3:
        ReadJoystick(pDevice);
        break;
    case 2:
        ReadMouse(pDevice);
        break;
    }

    buttons = pDevice->held;
    pDevice->pressed = (buttons ^ oldButtons) & buttons;
    if (buttons & 0xfffffff0)
        pDevice->held = buttons | 0x10000;

    now = CMain::GetFrameTime();
    dirButtons = pDevice->held & m_directionButtonMask;
    if (dirButtons != 0) {
        if (dirButtons != (m_directionButtonMask & oldButtons)) {
            pDevice->repeated = ~oldButtons & pDevice->held & m_directionButtonMask;
            pDevice->repeatTime = (int)m_keyboardDelay + now;
        } else if (pDevice->repeatTime - now <= 0) {
            pDevice->repeated = dirButtons;
            pDevice->repeatTime = (int)m_keyboardSpeed + now;
        } else {
            pDevice->repeated = 0;
        }
    } else {
        pDevice->repeated = 0;
        pDevice->repeatTime = 0;
    }

    if (m_unk0x0052086c != 0)
        pDevice->pressed |= pDevice->repeated;

    return pDevice;
}

// FUNCTION: CMR2 0x004ab380
// PORT: the condition parameters go into the effect's InputEffectParams.
int CInput::CreateForceFeedbackEffect(int effectType, DWORD duration, LONG coefficient, LONG offset, int triggerButton, int deviceIndex)
{
    ForceFeedbackDevice *pDevice;
    InputEffectParams *pParams;
    int slot;
    int i;

    i = 0;
    pDevice = &m_forceFeedbackDevices[deviceIndex];
    if (pDevice->field_0x0 == 0)
        return -1;

    while (pDevice->effects[i] != NULL) {
        if (i == 10)
            return -1;
        i++;
    }

    slot = deviceIndex * 10 + i;
    pParams = &m_forceFeedbackEffects[slot];
    memset(pParams, 0, sizeof(*pParams));
    switch (effectType) {
    case 0xb:
        pParams->type = INPUT_EFFECT_SPRING;
        break;
    case 0xc:
        pParams->type = INPUT_EFFECT_INERTIA;
        break;
    case 0xd:
        pParams->type = INPUT_EFFECT_DAMPER;
        break;
    case 0xe:
        pParams->type = INPUT_EFFECT_FRICTION;
        break;
    }
    pParams->offset = offset;
    pParams->coefficient = coefficient;
    pParams->saturation = 10000;
    pParams->deadBand = 0;
    pParams->duration = duration;
    pParams->gain = 10000;
    pParams->triggerButton = triggerButton == -1 ? INPUT_EFFECT_NO_TRIGGER : 0x30 + triggerButton;

    pDevice->effects[i] = Input_CreateEffect(pDevice->device, pParams);
    return i;
}

// FUNCTION: CMR2 0x004ab590
int CInput::CreateSpringEffect(DWORD duration, LONG coefficient, LONG offset, int triggerButton, int deviceIndex)
{
    return CreateForceFeedbackEffect(0xb, duration, coefficient, offset, triggerButton, deviceIndex);
}

// FUNCTION: CMR2 0x004ab5c0
int CInput::CreateDamperEffect(DWORD duration, LONG coefficient, LONG offset, int triggerButton, int deviceIndex)
{
    return CreateForceFeedbackEffect(0xd, duration, coefficient, offset, triggerButton, deviceIndex);
}

// FUNCTION: CMR2 0x0040c2a0
short CInput::GetButtonMapping(unsigned short controllerSlot, int action)
{
    int controllerIndex;
    ControllerData *pController;
    unsigned short *pActionMask;
    short mapping;

    mapping = 0;
    controllerIndex = m_playerControllers[controllerSlot];
    pActionMask = m_controllerInfo[controllerIndex].actionMasks;
    switch (action) {
    case 0: mapping = pActionMask[0]; break;
    case 1: mapping = pActionMask[1]; break;
    case 2: mapping = pActionMask[2]; break;
    case 3: mapping = pActionMask[3]; break;
    case 4: mapping = pActionMask[4]; break;
    case 5: mapping = pActionMask[5]; break;
    case 6: mapping = pActionMask[6]; break;
    case 7: mapping = pActionMask[7]; break;
    case 8: mapping = pActionMask[8]; break;
    case 9: mapping = pActionMask[9]; break;
    default: goto defaults;
    }

    if (mapping == 0) {
defaults:
        if (controllerSlot == 0 || controllerSlot == 1)
            goto fixed;
    }

    pController = &m_controllerInfo[controllerIndex];
    if (!IsControllerActionBound(action, pController))
        return mapping;

fixed:
    switch (action) {
    case 0: mapping = 1; break;
    case 1: mapping = 2; break;
    case 2: mapping = 4; break;
    case 3: mapping = 8; break;
    case 4: mapping = 0x10; break;
    case 5: mapping = 0x20; break;
    case 6: mapping = 0x40; break;
    case 7: mapping = 0x80; break;
    case 8: mapping = 0x100; break;
    case 9: mapping = 0x200; break;
    }
    return mapping;
}

// FUNCTION: CMR2 0x004ab030
HRESULT CInput::SetEffectGain(int effectIndex, DWORD gain, int deviceIndex)
{
    ForceFeedbackDevice *pDevice;
    InputEffectParams *pParams;

    pDevice = &m_forceFeedbackDevices[deviceIndex];
    if (pDevice->field_0x0 != 0 && pDevice->effects[effectIndex] != NULL) {
        pParams = &m_forceFeedbackEffects[deviceIndex * 10 + effectIndex];
        pParams->gain = gain;
        Input_UpdateEffect(pDevice->effects[effectIndex], pParams);
        return 0;
    }
    return -1;
}

// FUNCTION: CMR2 0x004ab0b0
HRESULT CInput::SetEffectGainAndDirection(int effectIndex, DWORD gain, LONG direction, int deviceIndex)
{
    ForceFeedbackDevice *pDevice;
    InputEffectParams *pParams;

    pDevice = &m_forceFeedbackDevices[deviceIndex];
    if (pDevice->field_0x0 != 0 && pDevice->effects[effectIndex] != NULL) {
        pParams = &m_forceFeedbackEffects[deviceIndex * 10 + effectIndex];
        pParams->gain = gain;
        pParams->direction = direction;
        Input_UpdateEffect(pDevice->effects[effectIndex], pParams);
        return 0;
    }
    return -1;
}

// FUNCTION: CMR2 0x004ab150
int CInput::CreateConstantForceEffect(DWORD duration, LONG direction, LONG magnitude, DWORD attackTime, DWORD attackLevel, DWORD fadeTime, DWORD fadeLevel, int triggerButton, int deviceIndex)
{
    ForceFeedbackDevice *pDevice;
    InputEffectParams *pParams;
    int i;

    i = 0;
    pDevice = &m_forceFeedbackDevices[deviceIndex];
    if (pDevice->field_0x0 == 0)
        return -1;

    while (pDevice->effects[i] != NULL) {
        if (i == 10)
            return -1;
        i++;
    }

    // PORT: the original built its DIEFFECT on the stack; the port keeps the
    // parameters in the effect's slot so later gain/direction changes have
    // them.
    pParams = &m_forceFeedbackEffects[deviceIndex * 10 + i];
    memset(pParams, 0, sizeof(*pParams));
    pParams->type = INPUT_EFFECT_CONSTANT;
    pParams->magnitude = magnitude;
    pParams->attackTime = attackTime;
    pParams->attackLevel = attackLevel;
    pParams->fadeTime = fadeTime;
    pParams->fadeLevel = fadeLevel;
    pParams->direction = direction;
    pParams->gain = 10000;
    pParams->duration = duration;
    pParams->triggerButton = triggerButton == -1 ? INPUT_EFFECT_NO_TRIGGER : 0x30 + triggerButton;

    pDevice->effects[i] = Input_CreateEffect(pDevice->device, pParams);
    return i;
}

// FUNCTION: CMR2 0x004ab2b0
HRESULT CInput::SetConditionCoefficient(int effectIndex, LONG coefficient, int deviceIndex)
{
    ForceFeedbackDevice *pDevice;
    InputEffectParams *pParams;

    pDevice = &m_forceFeedbackDevices[deviceIndex];
    if (pDevice->field_0x0 != 0 && pDevice->effects[effectIndex] != NULL) {
        pParams = &m_forceFeedbackEffects[deviceIndex * 10 + effectIndex];
        pParams->offset = 0;
        pParams->coefficient = coefficient;
        pParams->saturation = 10000;
        pParams->deadBand = 0;
        Input_UpdateEffect(pDevice->effects[effectIndex], pParams);
        return 0;
    }
    return -1;
}

// FUNCTION: CMR2 0x0040bff0
// PORT: the bindings are saved in controls.ini.
void CInput::SaveControllerInfo(void)
{
    Input_SaveBindings();
}

// Key labels of the keyboard controls: the first keyboard uses text for the
// cursor keys and [ ] C R, the second one the numeric keypad keys.
// GLOBAL: CMR2 0x00516928
char g_keypad7[] = "7";
// GLOBAL: CMR2 0x0051692c
char g_keypad9[] = "9";
// GLOBAL: CMR2 0x00516930
char g_keypad3[] = "3";
// GLOBAL: CMR2 0x00516934
char g_keypad2[] = "2";
// GLOBAL: CMR2 0x00516938
char g_keypad8[] = "8";
// GLOBAL: CMR2 0x0051693c
char g_keypad6[] = "6";
// GLOBAL: CMR2 0x00516940
char g_keypadFormat[] = "%s %s";
// GLOBAL: CMR2 0x00516948
char g_keypad4[] = "4";
// GLOBAL: CMR2 0x0051694c
char g_keyR[] = "R";
// GLOBAL: CMR2 0x00516950
char g_keyC[] = "C";
// GLOBAL: CMR2 0x00516954
char g_keyOpenBracket[] = "[";
// GLOBAL: CMR2 0x00516958
char g_keyCloseBracket[] = "]";


// Fills controller slot index from a detected device: its name, type flags,
// the default bindings and the texts shown for them in the controls menu.
// FUNCTION: CMR2 0x0040c610
void CInput::InitDetectedControllerSlot(DeviceInfo *pDevice, int index)
{
    ControllerData *pController;

    pController = &m_controllerInfo[index];
    pController->index = (BYTE)index;
    strcpy(pController->name, pDevice->deviceInstanceName);
    pController->hasAxes = (pDevice->type == 3 || pDevice->type == 2) ? 1 : 0;
    pController->analogSteering = (pDevice->type == 3 || pDevice->type == 2) ? 1 : 0;
    pController->analogPedals = (pDevice->type == 3 || pDevice->type == 2) ? 1 : 0;
    pController->forceFeedback = pDevice->supportsForceFeedback;
    m_controllerInfo[index].field_0x4 = 0x8000;
    m_controllerInfo[index].field_0x0 = 0x8000;
    if (pDevice->type == 1) {
        if (index == 0) {
            m_controllerInfo[0].keys[0] = pDevice->keyboard.steerLeftKey;
            strcpy(m_controllerInfo[0].keyNames[0], CFrontend::GetTextString(0x1f9));
            m_controllerInfo[0].keys[1] = pDevice->keyboard.steerRightKey;
            strcpy(m_controllerInfo[0].keyNames[1], CFrontend::GetTextString(0x1fa));
            m_controllerInfo[0].keys[2] = pDevice->keyboard.throttleKey;
            strcpy(m_controllerInfo[0].keyNames[2], CFrontend::GetTextString(0x1fb));
            m_controllerInfo[0].keys[3] = pDevice->keyboard.brakeKey;
            strcpy(m_controllerInfo[0].keyNames[3], CFrontend::GetTextString(0x1fc));
            m_controllerInfo[0].keys[4] = pDevice->keyboard.handbrakeKey;
            strcpy(m_controllerInfo[0].keyNames[4], CFrontend::GetTextString(0x1fd));
            m_controllerInfo[0].keys[5] = pDevice->keyboard.shiftUpKey;
            strcpy(m_controllerInfo[0].keyNames[5], g_keyCloseBracket);
            m_controllerInfo[0].keys[6] = pDevice->keyboard.shiftDownKey;
            strcpy(m_controllerInfo[0].keyNames[6], g_keyOpenBracket);
            m_controllerInfo[0].keys[7] = pDevice->keyboard.cameraCycleKey;
            strcpy(m_controllerInfo[0].keyNames[7], g_keyC);
            m_controllerInfo[0].keys[8] = pDevice->keyboard.driverCycleKey;
            strcpy(m_controllerInfo[0].keyNames[8], g_keyR);
            return;
        }
        pController->keys[0] = pDevice->keyboard.steerLeftKey;
        strcpy(pController->keyNames[0], FormatString(g_keypadFormat, CFrontend::GetTextString(0x203), g_keypad4));
        pController->keys[1] = pDevice->keyboard.steerRightKey;
        strcpy(pController->keyNames[1], FormatString(g_keypadFormat, CFrontend::GetTextString(0x203), g_keypad6));
        pController->keys[2] = pDevice->keyboard.throttleKey;
        strcpy(pController->keyNames[2], FormatString(g_keypadFormat, CFrontend::GetTextString(0x203), g_keypad8));
        pController->keys[3] = pDevice->keyboard.brakeKey;
        strcpy(pController->keyNames[3], FormatString(g_keypadFormat, CFrontend::GetTextString(0x203), g_keypad2));
        pController->keys[4] = pDevice->keyboard.handbrakeKey;
        strcpy(pController->keyNames[4], FormatString(g_keypadFormat, CFrontend::GetTextString(0x203), g_keypad3));
        pController->keys[5] = pDevice->keyboard.shiftUpKey;
        strcpy(pController->keyNames[5], CFrontend::GetTextString(0x204));
        pController->keys[6] = pDevice->keyboard.shiftDownKey;
        strcpy(pController->keyNames[6], CFrontend::GetTextString(0x205));
        pController->keys[7] = pDevice->keyboard.cameraCycleKey;
        strcpy(pController->keyNames[7], FormatString(g_keypadFormat, CFrontend::GetTextString(0x203), g_keypad9));
        pController->keys[8] = pDevice->keyboard.driverCycleKey;
        strcpy(pController->keyNames[8], FormatString(g_keypadFormat, CFrontend::GetTextString(0x203), g_keypad7));
        return;
    }
    if (pDevice->type == 2) {
        pDevice->joystick.bindings[0].present = 1;
        pController->axisBindings[0] = *(ControllerDataUnk0x210 *)&pDevice->joystick.bindings[0];
        pController->axisIndices[0] = 0;
        pController->axisBindings[1] = *(ControllerDataUnk0x210 *)&pDevice->joystick.bindings[0];
        pController->axisIndices[1] = 0;
        pController->steerLeftMask = 0;
        pController->steerRightMask = 0;
        pController->throttleMask = 0x10;
        pController->brakeMask = 0x20;
        pController->handbrakeMask = 0;
        pController->shiftUpMask = 0;
        pController->shiftDownMask = 0;
        pController->cameraCycleMask = 0;
        pController->driverCycleMask = 0;
        pController->backMask = 0;
        pController->keys[4] = (BYTE)pDevice->joystick.bindings[0].present;
        strcpy(pController->keyNames[4], CFrontend::GetTextString(0x1fe));
        pController->keys[5] = pDevice->keyboard.shiftUpKey;
        strcpy(pController->keyNames[5], CFrontend::GetTextString(0x1fe));
        pController->keys[6] = pDevice->keyboard.shiftDownKey;
        strcpy(pController->keyNames[6], CFrontend::GetTextString(0x1fe));
        pController->keys[7] = pDevice->keyboard.cameraCycleKey;
        strcpy(pController->keyNames[7], CFrontend::GetTextString(0x1fe));
        pController->keys[8] = pDevice->keyboard.driverCycleKey;
        strcpy(pController->keyNames[8], CFrontend::GetTextString(0x1fe));
        return;
    }
    if (pController->hasAxes != 0) {
        pController->steerLeftMask = 1;
        pController->steerRightMask = 2;
        if (pDevice->joystick.bindings[0].present != 0) {
            pController->axisBindings[0] = *(ControllerDataUnk0x210 *)&pDevice->joystick.bindings[0];
            pController->axisIndices[0] = 0;
            if (pDevice->joystick.bindings[0].present != 0) {
                pController->axisBindings[1] = *(ControllerDataUnk0x210 *)&pDevice->joystick.bindings[0];
                pController->axisIndices[1] = 0;
            }
        }
        pController->throttleMask = 4;
        if (pDevice->joystick.bindings[1].present != 0) {
            pController->axisBindings[2] = *(ControllerDataUnk0x210 *)&pDevice->joystick.bindings[1];
            pController->axisIndices[2] = 1;
        }
        pController->brakeMask = 8;
        if (pDevice->joystick.bindings[1].present != 0) {
            pController->axisBindings[3] = *(ControllerDataUnk0x210 *)&pDevice->joystick.bindings[1];
            pController->axisIndices[3] = 1;
        }
        pController->handbrakeMask = 0x10;
        pController->shiftUpMask = 0x20;
        pController->shiftDownMask = 0x40;
        pController->cameraCycleMask = 0x80;
        pController->driverCycleMask = 0x100;
        pController->backMask = 0x200;
        return;
    }
    memset(pController->axisBindings, 0, 0x32 * 4);
    pController->steerLeftMask = 1;
    pController->steerRightMask = 2;
    pController->throttleMask = 0x10;
    pController->brakeMask = 0x20;
    pController->handbrakeMask = 0x40;
    pController->shiftUpMask = 0x80;
    pController->shiftDownMask = 0x100;
    pController->cameraCycleMask = 0x200;
    pController->driverCycleMask = 0x800;
    pController->backMask = 0x400;
}

// FUNCTION: CMR2 0x0040c050
// PORT: the configurations of the four device records, made afresh: the
// keyboards of players 1 and 2, then the players' joysticks, which they
// drive with. The bindings themselves are in controls.ini (src/input).
void CInput::RefreshControllerConfigurations(void)
{
    int i;

    m_controllerCount = CountAttachedInputDevices();
    for (i = 0; i < m_controllerCount && i < 6; i++) {
        UpdateDevice(i);
        if (i < 2)
            Input_InitKeyboardConfiguration(i);
        else
            Input_InitPlayerConfiguration(i);
    }
    if (m_controllerCount < 6)
        memset(&m_controllerInfo[m_controllerCount], 0, (6 - m_controllerCount) * sizeof(ControllerData));
    for (i = 0; i < INPUT_PLAYERS; i++)
        m_playerControllers[i] = (unsigned short)(2 + i);
    ApplyControllerButtonBindings(0);
    ApplyControllerButtonBindings(1);
    m_hasLoadedControllerInfo = TRUE;
}

struct InputQueues {
    int characters[30];
    int keys[30];
};

// GLOBAL: CMR2 0x006ed3f4
InputQueues g_inputQueues;

// FUNCTION: CMR2 0x004b7c80
void Input_ClearCharacterQueue(void)
{
    int i;

    for (i = 0; i < 30; i++)
        g_inputQueues.characters[i] = 0;
}

// Queues one character for the input ring buffer.
// FUNCTION: CMR2 0x004b7ca0
void CInput::QueueInputCharacter(int param1)
{
    int *p;
    int i;

    i = 0;
    p = g_inputQueues.characters;
    while ((INT_PTR)p < (INT_PTR)(g_inputQueues.characters + 30)) {
        if (*p == 0) {
            g_inputQueues.characters[i] = param1;
            return;
        }
        p++;
        i++;
    }
}

// Pops the oldest character of the input ring buffer.
// FUNCTION: CMR2 0x004b7cd0
bool Input_PopQueuedCharacter(int *pOut)
{
    int i;

    if (g_inputQueues.characters[0] != 0) {
        *pOut = g_inputQueues.characters[0];
        i = 0;
        do {
            g_inputQueues.characters[i] = g_inputQueues.characters[i + 1];
            i++;
        } while (i < 29);
        g_inputQueues.characters[29] = 0;
        return true;
    }
    return false;
}

// Queues one key press (only when the scan code carries a virtual key).
// FUNCTION: CMR2 0x004b7d10
void CInput::QueueVirtualKeyPress(unsigned int param1)
{
    int *p;
    int i;

    // PORT: param1 is the key code (the original got the WM_KEYDOWN lParam
    // and required its scan code byte to be set).
    if (param1 != 0) {
        i = 0;
        p = g_inputQueues.keys;
        while (p < g_inputQueues.keys + 30) {
            if (*p == 0) {
                g_inputQueues.keys[i] = param1;
                return;
            }
            p++;
            i++;
        }
    }
}

// GLOBAL: CMR2 0x005320a4
int g_inputFrameDelta;

// FUNCTION: CMR2 0x0040af20
void CInput::UpdateInputFrameDelta(void)
{
    g_inputFrameDelta = CMain::GetFrameDelta();
}

// FUNCTION: CMR2 0x0049eab0
void CInput::UpdateAllAvailableDevices(void)
{
    // Frontend/pause transitions must not replay unconsumed driving actions.
    memset(s_pendingRacePresses, 0, sizeof(s_pendingRacePresses));
    // PORT: the force feedback of controllers connected or disconnected
    // since is taken in the menus.
    Input_RescanChangedJoysticks();
    int i;

    for (i = 0; i < 8; i++)
        UpdateDevice(i);
}

// PORT: poll held buttons and axes on every simulation step, but deliver
// presses only on a step that updates a player using that device. This also
// retains a complete press/release between two player steps. Reusing an edge
// on every simulation step would instead shift multiple gears on a held key.
void CInput::UpdateRaceDevices(unsigned int readyDevices)
{
    for (int i = 0; i < 8; ++i) {
        DeviceInfo *device = UpdateDevice(i);
        if (device == NULL)
            continue;
        s_pendingRacePresses[i] |= device->pressed;
        device->pressed = 0;
        if ((readyDevices & (1u << i)) != 0) {
            device->pressed = s_pendingRacePresses[i];
            s_pendingRacePresses[i] = 0;
        }
    }
}

// match 57%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0040bc90
void CInput::SetControllerForceFeedbackValue(unsigned short param1, DWORD param2)
{
    unsigned short index;

    index = CInput::m_playerControllers[param1];
    SetForceFeedbackDeviceValue(m_controllerInfo[index].field_0x11c = param2, index);
}


// FUNCTION: CMR2 0x0049ead0
DeviceInfo *CInput::GetAvailableDeviceRecord(int index)
{
    return &m_availableDevices[index];
}

// FUNCTION: CMR2 0x004b7d40
void Input_ClearKeyPressQueue(void)
{
    int i;

    for (i = 0; i < 30; i++)
        g_inputQueues.keys[i] = 0;
}

// Pops the oldest key of the key press queue.
// FUNCTION: CMR2 0x004b7d60
int Input_PopQueuedKeyPress(int *pOut)
{
    int i;

    if (g_inputQueues.keys[0] != 0) {
        *pOut = g_inputQueues.keys[0];
        i = 0;
        do {
            g_inputQueues.keys[i] = g_inputQueues.keys[i + 1];
            i++;
        } while (i < 29);
        g_inputQueues.keys[29] = 0;
        return 1;
    }
    return 0;
}

// Latched state of the two throttle axes of each device (pressed once until released).
// GLOBAL: CMR2 0x005334f4
int g_latchedThrottleAxes[8][2];

// Turns separate joystick/wheel pedals into one-shot up/down menu presses
// (bits 2 and 3).
// FUNCTION: CMR2 0x0040bad0
// PORT: the menus take the inputs bound to the menu actions (controls.ini).
void Input_TranslatePedalsToMenuKeys(void)
{
}

// Controller slots are mapped to entries of m_controllerInfo through m_playerControllers.
// FUNCTION: CMR2 0x0040bba0
unsigned short Input_GetAssignedController(void)
{
    return CInput::m_controllerCount;
}

// FUNCTION: CMR2 0x0040bbb0
ControllerData *Input_GetControllerTable(void)
{
    return CInput::m_controllerInfo;
}

// FUNCTION: CMR2 0x0040bbc0
unsigned short Input_GetControllerSlotMapping(unsigned short slot)
{
    return CInput::m_playerControllers[slot];
}

// FUNCTION: CMR2 0x0040bbe0
void Input_SetControllerSlotMapping(unsigned short slot, unsigned short index)
{
    CInput::m_playerControllers[slot] = index;
}

// FUNCTION: CMR2 0x0040bc00
unsigned int Input_GetControllerPrimaryFlags(unsigned short slot)
{
    return CInput::m_controllerInfo[CInput::m_playerControllers[slot]].field_0x0;
}

// FUNCTION: CMR2 0x0040bc30
unsigned int Input_GetControllerSecondaryFlags(unsigned short slot)
{
    return CInput::m_controllerInfo[CInput::m_playerControllers[slot]].field_0x4;
}

// FUNCTION: CMR2 0x0040bc60
void Input_SetControllerPrimaryFlags(unsigned short slot, unsigned int value)
{
    CInput::m_controllerInfo[CInput::m_playerControllers[slot]].field_0x0 = value;
}

// FUNCTION: CMR2 0x0040bcd0
DWORD Input_GetControllerField11C(unsigned short slot)
{
    return CInput::m_controllerInfo[CInput::m_playerControllers[slot]].field_0x11c;
}

// FUNCTION: CMR2 0x0040bd00
void Input_SetControllerSecondaryFlags(unsigned short slot, unsigned int value)
{
    CInput::m_controllerInfo[CInput::m_playerControllers[slot]].field_0x4 = value;
}

// FUNCTION: CMR2 0x0040bd30
DWORD Input_GetControllerField118(unsigned short slot)
{
    return CInput::m_controllerInfo[CInput::m_playerControllers[slot]].forceFeedback;
}

// Merges the buttons of the joystick assigned to the slot into pOut.
// FUNCTION: CMR2 0x0040bd60
void Input_MergeAssignedJoystickButtons(int slot, DeviceInfo *pOut)
{
    DeviceInfo *pDev;

    if ((short)Input_GetControllerSlotMapping(slot) != 0 && (short)Input_GetControllerSlotMapping(slot) != 1) {
        pDev = CInput::GetAvailableDeviceRecord(Input_GetControllerSlotMapping(slot));
        if (pDev->type == 0 || pDev->type == 3) {
            pOut->pressed |= pDev->pressed;
            pOut->held |= pDev->held;
            pOut->repeated |= pDev->repeated;
        }
    }
}


// FUNCTION: CMR2 0x0040bdd0
DWORD Input_GetControllerField110(unsigned short slot)
{
    return CInput::m_controllerInfo[CInput::m_playerControllers[slot]].analogSteering;
}


int Input_PopQueuedKeyPress(int *pOut);

// Name of the last key pressed (waits until the key queue is empty).
// FUNCTION: CMR2 0x0049ed80
void Input_GetLastKeyName(LPSTR pName, unsigned int size)
{
    LONG key;

    key = 0;
    while (Input_PopQueuedKeyPress((int *)&key) != 0)
        ;
    // PORT: the queue holds key codes (DIK_*), named in the current layout.
    strncpy(pName, key != 0 ? Input_GetKeyName(key) : "", size & 0xff);
    if ((size & 0xff) != 0)
        pName[(size & 0xff) - 1] = 0;
}
