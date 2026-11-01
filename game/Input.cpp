#include "Input.h"
#include "GameInfo.h"
#include "Graphics.h"
#include "main.h"
#include "InstallInfo.h"
#include "Frontend.h"
#include "FileBuffer.h"
#include "Game.h"

#include "port/sys.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// PORT: the DirectInput 7 data formats (c_dfDIMouse2, c_dfDIKeyboard,
// c_dfDIJoystick2 and their descriptor arrays) are gone: the input layer
// returns the state structures directly.

// GLOBAL: CMR2 0x00511758
// IID_IDirectInput7A

// GLOBAL: CMR2 0x0059f8c8
void *CInput::m_lpDirectInput7;

// GLOBAL: CMR2 0x00520874
char CInput::m_strKeyboard[12] = "Keyboard";

// GLOBAL: CMR2 0x0059f8cc
Unk0x0059f8cc CInput::m_unk0x0059f8cc; // device count or something

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
USHORT CInput::m_unk0x00511400[8] = {0, 4, 8, 12, 16, 20, 24, 28};

// GLOBAL: CMR2 0x0059f6b0
InputDevice *CInput::m_unk0x0059f6b0[8];

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
unsigned short CInput::m_unk0x005168f4[8] = {0, 1, 2, 3, 0, 0, 0, 0};

// FUNCTION: CMR2 0x0049fd30
// PORT: SDL is initialised by the platform layer.
BOOL CInput::DInputCreate(void) {
    CGame::RegisterCallback(DInputRelease, NULL);
    return TRUE;
}

// FUNCTION: CMR2 0x0049fe30
// PORT: type 0 keyboard, 1 mouse, 2 joystick (index); the cooperative
// levels become the mouse capture below.
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
    return Input_OpenJoystick(index);
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

    // PORT: every open joystick. The original closed as many as were attached
    // now, which leaves one open after it was disconnected.
    for (i = 0; i < 4; i++) {
        if (m_unk0x0059f6b0[i] != NULL) {
            Input_CloseDevice(m_unk0x0059f6b0[i]);
            m_unk0x0059f6b0[i] = NULL;
        }
    }
}

// FUNCTION: CMR2 0x0049ef90
int CInput::CountAttachedInputDevices(void) {
  int i;
  int count;

  m_unk0x0059f8cc.field_0x3 = 0;
  count = Input_GetJoystickCount();
  for (i = 0; i < count; i++)
      CountJoystickEnumerationCallback();
  return m_unk0x0059f8cc.field_0x1 + m_unk0x0059f8cc.field_0x3;
}

// FUNCTION: CMR2 0x0049f6b0
BOOL CInput::CountJoystickEnumerationCallback(void) {
    m_unk0x0059f8cc.field_0x3++;
    return TRUE;
}

// match 89%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0049f0e0
BOOL CInput::SetupKeyboard(void) {
    unsigned int uVar1;
    int iVar3 = 0;

    do {
        uVar1 = m_unk0x0059f8cc.field_0x0;
        m_availableDevices[uVar1].field_0x0 = 1;
        m_availableDevices[uVar1].field_0x18 = FALSE;

        // PORT: copy text directly; avoid calling sprintf through an incompatible type.
        strcpy(m_availableDevices[uVar1].deviceInstanceName, m_strKeyboard);
        strcpy(m_availableDevices[m_unk0x0059f8cc.field_0x0].deviceProductName, m_strKeyboard);
        
        uVar1 = m_unk0x0059f8cc.field_0x0;
        m_availableDevices[uVar1].supportsForceFeedback = FALSE;

        if (!iVar3) {
            m_availableDevices[uVar1].keyboard.field_0x468 = 0xcb;
            m_availableDevices[uVar1].keyboard.field_0x469 = 0xcd;
            m_availableDevices[uVar1].keyboard.field_0x46a = 0xc8;
            m_availableDevices[uVar1].keyboard.field_0x46b = 0xd0;
            m_availableDevices[uVar1].keyboard.field_0x46c = 0x39;
            m_availableDevices[uVar1].keyboard.field_0x46d = 0x1b;
            m_availableDevices[uVar1].keyboard.field_0x46e = 0x1a;
            m_availableDevices[uVar1].keyboard.field_0x46f = 0x2e;
            m_availableDevices[uVar1].keyboard.field_0x470 = 0x13;
        } else {
            m_availableDevices[uVar1].keyboard.field_0x468 = 0x4b;
            m_availableDevices[uVar1].keyboard.field_0x469 = 0x4d;
            m_availableDevices[uVar1].keyboard.field_0x46a = 0x48;
            m_availableDevices[uVar1].keyboard.field_0x46b = 0x50;
            m_availableDevices[uVar1].keyboard.field_0x46c = 0x51;
            m_availableDevices[uVar1].keyboard.field_0x46d = 0xc9;
            m_availableDevices[uVar1].keyboard.field_0x46e = 0xd1;
            m_availableDevices[uVar1].keyboard.field_0x46f = 0x49;
            m_availableDevices[uVar1].keyboard.field_0x470 = 0x47;            
        }

        m_availableDevices[uVar1].keyboard.field_0x471 = 0x0;
        m_availableDevices[uVar1].keyboard.field_0x474 = 0x3b;
        m_availableDevices[uVar1].keyboard.field_0x475 = 0x3c;
        m_availableDevices[uVar1].keyboard.field_0x476 = 0x3e;
        m_availableDevices[uVar1].keyboard.field_0x477 = 0x0;
        m_availableDevices[uVar1].keyboard.field_0x472 = 0x1;
        m_availableDevices[uVar1].keyboard.field_0x473 = 0x0;

        m_unk0x0059f8cc.field_0x0 = m_unk0x0059f8cc.field_0x0 + 1;
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
BOOL CInput::GetAttachedJoysticks(void) {
    int count;
    int i;

    count = Input_GetJoystickCount();
    for (i = 0; i < count; i++)
        SetupJoystick(i);
    return TRUE;
}

// DirectInput enumeration callback for one joystick: creates the device, names
// its buttons (DIJOFS_BUTTON) and the four directions of its first POV hat
// (DIJOFS_POV(0)), and enables autocentre-off force feedback when present.
// match 86%: the axis loop keeps two counters in the original (i, axisID).
// FUNCTION: CMR2 0x0049f6d0
// PORT: called for each joystick SDL reports.
BOOL CInput::SetupJoystick(int joystickIndex) {
    unsigned int uVar2;
    DeviceInfo *pDeviceInfo;
    InputDevice *pDevice;
    BYTE iVar6[4];
    int i, iVar11;
    int count;
    int offset;
    InputCaps devCaps;
    char objectName[260];

    uVar2 = m_unk0x0059f8cc.field_0x0;
    pDeviceInfo = &m_availableDevices[m_unk0x0059f8cc.field_0x0];
    {
        pDeviceInfo->field_0x0 = 3;
        pDevice = DInputCreateDevice(2, joystickIndex);
        m_unk0x0059f6b0[m_unk0x0059f8cc.field_0x2] = pDevice;

        if (pDevice != NULL) {
            strncpy(pDeviceInfo->deviceInstanceName, Input_GetInstanceName(pDevice), sizeof(pDeviceInfo->deviceInstanceName));
            strncpy(pDeviceInfo->deviceProductName, Input_GetProductName(pDevice), sizeof(pDeviceInfo->deviceProductName));
        
            pDeviceInfo->field_0x18 = m_unk0x0059f8cc.field_0x2;

            SetupJoystickDeviceInfo(pDeviceInfo);

            for (i = 0; i < pDeviceInfo->joystick.controlCount; i++) {
                if (pDeviceInfo->joystick.bindings[i].present != FALSE) {
                    SetJoystickAxisRange(m_unk0x0059f8cc.field_0x0, i, pDeviceInfo->joystick.bindings[i].range);
                    SetJoystickAxisDeadzone(m_unk0x0059f8cc.field_0x0, i, pDeviceInfo->joystick.bindings[i].deadzone);
                    SetJoystickAxisSaturation(m_unk0x0059f8cc.field_0x0, i, pDeviceInfo->joystick.bindings[i].saturation);
                }
            }

            Input_GetCaps(m_unk0x0059f6b0[m_unk0x0059f8cc.field_0x2], &devCaps);

            {
                if (devCaps.forceFeedback) {
                    pDeviceInfo->supportsForceFeedback = TRUE;

                    Input_SetAutocenter(m_unk0x0059f6b0[m_unk0x0059f8cc.field_0x2], FALSE);
                    InitializeForceFeedbackDevice(m_unk0x0059f8cc.field_0x0, m_unk0x0059f6b0[m_unk0x0059f8cc.field_0x2]);
                } else {
                    pDeviceInfo->supportsForceFeedback = FALSE;
                }

                // Button names (DIJOFS_BUTTON(i) = 0x30 + i).
                count = devCaps.buttons;
                pDeviceInfo->field_0x14 = 0;
                for (i = 0; i < count; i++) {
                    if (Input_GetObjectName(m_unk0x0059f6b0[m_unk0x0059f8cc.field_0x2], i + 0x30, objectName, sizeof(objectName))) {
                        strncpy(pDeviceInfo->field_0x284[i], objectName, 20);
                        pDeviceInfo->field_0x14++;
                    }
                }

                // The first POV hat (DIJOFS_POV(0) = 0x20) gives four direction names.
                count = devCaps.povs;
                i = 0;
                if (count > 0) {
                    offset = 0x20;
                    do {
                        if (offset > 0x20)
                            break;
                        if (Input_GetObjectName(m_unk0x0059f6b0[m_unk0x0059f8cc.field_0x2], offset, objectName, sizeof(objectName))) {
                            strncpy(pDeviceInfo->field_0x414, objectName, 0x11);
                            strcat(pDeviceInfo->field_0x414, m_strL);

                            strncpy(pDeviceInfo->field_0x425, objectName, 0x11);
                            strcat(pDeviceInfo->field_0x425, m_strR);

                            strncpy(pDeviceInfo->field_0x436, objectName, 0x11);
                            strcat(pDeviceInfo->field_0x436, m_strU);

                            strncpy(pDeviceInfo->field_0x447, objectName, 0x11);
                            strcat(pDeviceInfo->field_0x447, m_strD);
                        }
                        i++;
                        offset += 4;
                    } while (i < count);
                }

                i = 0;

                if (pDeviceInfo->field_0x14 > 0) {
                    int * pUnk0x1c = &pDeviceInfo->field_0x1c;
                    do {
                        *pUnk0x1c++ = 1u << i;
                        i++;
                    } while (i < pDeviceInfo->field_0x14);
                }

                m_unk0x0059f8cc.field_0x2++;
                m_unk0x0059f8cc.field_0x0++;
            }
        }
    }

    return TRUE;
}

// FUNCTION: CMR2 0x0049fad0
void CInput::SetupJoystickDeviceInfo(DeviceInfo *deviceInfo) {
    int i;

    deviceInfo->joystick.controlCount = 0;
    for (i = 0; i < 8; i++) {
        // PORT: the axis exists when it has a range (DIPROP_RANGE succeeded).
        if (Input_HasAxis(m_unk0x0059f6b0[deviceInfo->field_0x18], m_unk0x00511400[i])) {
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
void CInput::SetJoystickAxisRange(int deviceID, int axisID, DWORD range) {
    DeviceInfo * pDeviceInfo = &m_availableDevices[deviceID];

    Input_SetAxisRange(m_unk0x0059f6b0[pDeviceInfo->field_0x18], m_unk0x00511400[axisID], -(LONG)range, range);
    pDeviceInfo->joystick.bindings[axisID].range = range;
}

// FUNCTION: CMR2 0x0049ee90
void CInput::SetJoystickAxisDeadzone(int deviceID, int axisID, DWORD deadzone) {
    DeviceInfo * pDeviceInfo = &m_availableDevices[deviceID];

    Input_SetAxisDeadzone(m_unk0x0059f6b0[pDeviceInfo->field_0x18], m_unk0x00511400[axisID], deadzone);
    pDeviceInfo->joystick.bindings[axisID].deadzone = deadzone;
}

// FUNCTION: CMR2 0x0049ef10
void CInput::SetJoystickAxisSaturation(int deviceID, int axisID, DWORD saturation) {
    DeviceInfo * pDeviceInfo = &m_availableDevices[deviceID];

    Input_SetAxisSaturation(m_unk0x0059f6b0[pDeviceInfo->field_0x18], m_unk0x00511400[axisID], saturation);
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
void CInput::LoadControllerInfo(void) {
    ControllerData *fileBuffer;

    m_hasLoadedControllerInfo = 0;
    sprintf(CFrontend::m_stringDest, m_strControllerInfoDir, CInstallInfo::GetGameHDPath());

    fileBuffer = (ControllerData*)CFileBuffer::GetGenericFileBuffer(CFrontend::m_stringDest, TRUE);
    if (fileBuffer != NULL) {
        if (CGenericFileLoader::GetGenericFileSize() == 0x11a4) {
            memcpy(&m_controllerInfo, fileBuffer, sizeof(m_controllerInfo));

            // write the first two elements of the array
            memcpy(m_unk0x005168f4, &((unsigned int*)fileBuffer)[0x468], 4);

            CFileBuffer::FreeGenericFileBuffer(fileBuffer);
            m_hasLoadedControllerInfo = TRUE;

            ApplyControllerButtonBindings(0);
            ApplyControllerButtonBindings(1);
        }
    }
}

// FUNCTION: CMR2 0x0040be90
void CInput::ApplyControllerButtonBindings(unsigned short param1) {
    int uVar1;
    unsigned int raw;
    unsigned short uVar2;
    ControllerData * pController;

    raw = param1;
    uVar2 = param1;
    uVar1 = m_unk0x005168f4[uVar2];
    pController = &m_controllerInfo[uVar1];

    if (uVar1 > 1) {
        AssignUnmappedControllerActions(raw, pController);
    }

    SetDeviceKeyBinding(uVar2, 1, pController->field_0x13e[0]);
    SetDeviceKeyBinding(uVar2, 2, pController->field_0x13e[1]);
    SetDeviceKeyBinding(uVar2, 4, pController->field_0x13e[2]);
    SetDeviceKeyBinding(uVar2, 8, pController->field_0x13e[3]);
    SetDeviceKeyBinding(uVar2, 0x10, pController->field_0x13e[4]);
    SetDeviceKeyBinding(uVar2, 0x20, pController->field_0x13e[5]);
    SetDeviceKeyBinding(uVar2, 0x40, pController->field_0x13e[6]);
    SetDeviceKeyBinding(uVar2, 0x80, pController->field_0x13e[7]);
    SetDeviceKeyBinding(uVar2, 0x100, pController->field_0x13e[8]);
}

// FUNCTION: CMR2 0x0040c440
void CInput::AssignUnmappedControllerActions(unsigned int param1, ControllerData* param2) {
    unsigned short* puVar2;
    unsigned short uVar4;
    unsigned char bVar1;
    int iVar3;
    unsigned short combinedFlags;
    
    iVar3 = 0;
    puVar2 = &param2->field_0x128;
    
    do {
        if (*puVar2 == 0) {
            if (param2->field_0x13e[iVar3] != 0) {
                
                combinedFlags = param2->field_0x13a | param2->field_0x136 | 
                                param2->field_0x134 | param2->field_0x132 |
                                param2->field_0x130 | param2->field_0x12e | 
                                param2->field_0x12c | param2->field_0x12a |
                                param2->field_0x128 | 0x400;

                bVar1 = FindFirstClearBindingBit(combinedFlags & 0xffff);
                
                uVar4 = 1 << bVar1;
                
                SetDeviceKeyBinding(param1 & 0xffff, uVar4 & 0xffff, param2->field_0x13e[iVar3]);
                SetDeviceKeyBinding(param1 & 0xffff, (1 << iVar3) & 0xffff, 0);
                
                *puVar2 = uVar4;
            }
        }
        
        iVar3++;
        puVar2++;
    } while (iVar3 < 10);
}

// FUNCTION: CMR2 0x0049eb90
void CInput::SetDeviceKeyBinding(int param1, unsigned int param2, unsigned int param3) {
    DeviceInfo* device = &m_availableDevices[param1];
    switch(param2) {
        case 1:
            device->keyboard.field_0x468 = param3;
            break;

        case 2:
            device->keyboard.field_0x469 = param3;
            break;

        case 4:
            device->keyboard.field_0x46a = param3;
            break;

        case 8:
            device->keyboard.field_0x46b = param3;
            break;

        case 0x10:
            device->keyboard.field_0x46c = param3;
            break;

        case 0x20:
            device->keyboard.field_0x46d = param3;
            break;

        case 0x40:
            device->keyboard.field_0x46e = param3;
            break;
        
        case 0x80:
            device->keyboard.field_0x46f = param3;
            break;

        case 0x100:
            device->keyboard.field_0x470 = param3;
            break;

        case 0x200:
            device->keyboard.field_0x471 = param3;
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
    m_unk0x0059f8cc.field_0x1 = 2;
    m_unk0x0059f8cc.field_0x0 = 0;
    m_unk0x0059f8cc.field_0x2 = 0;
    for (i = 0; i < 8; i++) {
        m_availableDevices[i].field_0x0 = -1;
        m_availableDevices[i].field_0x18 = -1;
    }
    SetupKeyboard();
    SetupMouse();
    GetAttachedJoysticks();
}

// PORT: the original enumerated joysticks once, at startup. Rebuilds the
// devices when one was connected or disconnected since, so a wheel that
// switches out of its generic USB mode, or a controller connected later,
// can be chosen. Called on entering the controls menu.
void Input_RescanChangedJoysticks(void)
{
    if (!Input_JoysticksChanged())
        return;
    CInput::ResetForceFeedbackEffectsAlt();
    CInput::RebuildAvailableInputDevices();
    CInput::RefreshControllerConfigurations();
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
        if (m_availableDevices[i].field_0x0 == 2) {
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
    return m_controllerInfo[m_unk0x005168f4[(unsigned short)param1]].field_0x114;
}

// FUNCTION: CMR2 0x0040be30
DWORD CInput::GetControllerField120(unsigned int param1)
{
    return m_controllerInfo[m_unk0x005168f4[(unsigned short)param1]].field_0x120;
}

// FUNCTION: CMR2 0x0040be60
DWORD CInput::GetControllerField124(unsigned short param1)
{
    return m_controllerInfo[m_unk0x005168f4[(unsigned short)param1]].field_0x124;
}

// FUNCTION: CMR2 0x0040c210
unsigned int CInput::GetEnabledControllerAxisBinding(unsigned int param1, int param2)
{
    unsigned int controller;

    controller = m_unk0x005168f4[(unsigned short)param1];
    if (m_controllerInfo[controller].field_0x210[param2].present != 0)
        return m_controllerInfo[controller].field_0x2d8[param2];
    return -1;
}

// FUNCTION: CMR2 0x0040c270
BOOL CInput::IsControllerActionBound(int param1, ControllerData *param2)
{
    if ((&param2->field_0x128)[param1] != 0 && param2->field_0x13e[param1] != 0)
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

    pDevice->field_0x4 = 0;
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
            pDevice->field_0x4 |= m_buttonMasks[i];
    }
}

// FUNCTION: CMR2 0x0049f480
void CInput::ReadKeyboardDevice(DeviceInfo *pDevice)
{
    pDevice->field_0x4 = 0;
    if (CGameInfo::m_unk0x0059f8d0 != 0) {
        if (m_keyboardState[INPUT_KEY_LEFT] & 0x80)
            pDevice->field_0x4 = 1;
        if (m_keyboardState[INPUT_KEY_RIGHT] & 0x80)
            pDevice->field_0x4 |= 0x2;
        if (m_keyboardState[INPUT_KEY_UP] & 0x80)
            pDevice->field_0x4 |= 0x4;
        if (m_keyboardState[INPUT_KEY_DOWN] & 0x80)
            pDevice->field_0x4 |= 0x8;
        if (m_keyboardState[INPUT_KEY_RETURN] & 0x80)
            pDevice->field_0x4 |= 0x10;
        if (m_keyboardState[INPUT_KEY_ESCAPE] & 0x80)
            pDevice->field_0x4 |= 0x20;
        if (m_keyboardState[INPUT_KEY_F1] & 0x80)
            pDevice->field_0x4 |= 0x1000;
        if (m_keyboardState[INPUT_KEY_F2] & 0x80)
            pDevice->field_0x4 |= 0x2000;
    } else {
        if (m_keyboardState[pDevice->keyboard.field_0x468] & 0x80)
            pDevice->field_0x4 = 1;
        if (m_keyboardState[pDevice->keyboard.field_0x469] & 0x80)
            pDevice->field_0x4 |= 0x2;
        if (m_keyboardState[pDevice->keyboard.field_0x46a] & 0x80)
            pDevice->field_0x4 |= 0x4;
        if (m_keyboardState[pDevice->keyboard.field_0x46b] & 0x80)
            pDevice->field_0x4 |= 0x8;
        if (m_keyboardState[pDevice->keyboard.field_0x46c] & 0x80)
            pDevice->field_0x4 |= 0x10;
        if (m_keyboardState[pDevice->keyboard.field_0x46d] & 0x80)
            pDevice->field_0x4 |= 0x20;
        if (m_keyboardState[pDevice->keyboard.field_0x46e] & 0x80)
            pDevice->field_0x4 |= 0x40;
        if (m_keyboardState[pDevice->keyboard.field_0x46f] & 0x80)
            pDevice->field_0x4 |= 0x80;
        if (m_keyboardState[pDevice->keyboard.field_0x470] & 0x80)
            pDevice->field_0x4 |= 0x100;
        if (m_keyboardState[pDevice->keyboard.field_0x471] & 0x80)
            pDevice->field_0x4 |= 0x200;
        if (m_keyboardState[pDevice->keyboard.field_0x472] & 0x80)
            pDevice->field_0x4 |= 0x400;
        if (m_keyboardState[pDevice->keyboard.field_0x473] & 0x80)
            pDevice->field_0x4 |= 0x800;
        if (m_keyboardState[pDevice->keyboard.field_0x474] & 0x80)
            pDevice->field_0x4 |= 0x1000;
        if (m_keyboardState[pDevice->keyboard.field_0x475] & 0x80)
            pDevice->field_0x4 |= 0x2000;
        if (m_keyboardState[pDevice->keyboard.field_0x476] & 0x80)
            pDevice->field_0x4 |= 0x4000;
        if (m_keyboardState[pDevice->keyboard.field_0x477] & 0x80)
            pDevice->field_0x4 |= 0x8000;
    }
}

// PORT: the directions of each device's first POV hat (the D-pad of a
// gamepad), apart from its stick's: see Input_MergeConfiguredDeviceButtons.
static unsigned int s_hatDirections[8];

// FUNCTION: CMR2 0x0049fb70
void CInput::ReadJoystick(DeviceInfo *pDevice)
{
    InputJoystickState joyState;
    InputDevice *pJoystick;
    int i;

    pJoystick = m_unk0x0059f6b0[pDevice->field_0x18];
    pDevice->field_0x4 = 0;
    s_hatDirections[pDevice - m_availableDevices] = 0;
    if (pJoystick == NULL || !Input_ReadJoystick(pJoystick, &joyState))
        return;

    pDevice->joystick.bindings[0].position = joyState.lX;
    pDevice->joystick.bindings[1].position = joyState.lY;
    pDevice->joystick.bindings[2].position = joyState.lZ;
    pDevice->joystick.bindings[3].position = joyState.lRx;
    pDevice->joystick.bindings[4].position = joyState.lRy;
    pDevice->joystick.bindings[5].position = joyState.lRz;
    pDevice->joystick.bindings[6].position = joyState.rglSlider[0];
    pDevice->joystick.bindings[7].position = joyState.rglSlider[1];

    if (joyState.lX < -39321)
        pDevice->field_0x4 |= 0x1;
    else if (joyState.lX > 39321)
        pDevice->field_0x4 |= 0x2;
    if (joyState.lY < -39321)
        pDevice->field_0x4 |= 0x4;
    else if (joyState.lY > 39321)
        pDevice->field_0x4 |= 0x8;

    if ((joyState.rgdwPOV[0] & 0xffff) != 0xffff) {
        unsigned int hat = 0;
        if (joyState.rgdwPOV[0] >= 4500 && joyState.rgdwPOV[0] <= 13500)
            hat |= 0x2;
        if (joyState.rgdwPOV[0] >= 13500 && joyState.rgdwPOV[0] <= 22500)
            hat |= 0x8;
        if (joyState.rgdwPOV[0] >= 22500 && joyState.rgdwPOV[0] <= 31500)
            hat |= 0x1;
        if (joyState.rgdwPOV[0] >= 31500 || joyState.rgdwPOV[0] <= 4500)
            hat |= 0x4;
        pDevice->field_0x4 |= hat;
        s_hatDirections[pDevice - m_availableDevices] = hat;
    }

    for (i = 0; i < pDevice->field_0x14; i++) {
        if (i < 24 && (joyState.rgbButtons[i] & 0x80))
            pDevice->field_0x4 |= m_buttonMasks[i];
    }
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
    oldButtons = pDevice->field_0x4;
    if (index > 1) {
        if (pDevice->field_0x18 < 0 || pDevice->field_0x18 > 3)
            return NULL;
    }

    switch (pDevice->field_0x0) {
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

    buttons = pDevice->field_0x4;
    pDevice->field_0x8 = (buttons ^ oldButtons) & buttons;
    if (buttons & 0xfffffff0)
        pDevice->field_0x4 = buttons | 0x10000;

    now = CMain::GetFrameTime();
    dirButtons = pDevice->field_0x4 & m_directionButtonMask;
    if (dirButtons != 0) {
        if (dirButtons != (m_directionButtonMask & oldButtons)) {
            pDevice->field_0xc = ~oldButtons & pDevice->field_0x4 & m_directionButtonMask;
            pDevice->field_0x10 = (int)m_keyboardDelay + now;
        } else if (pDevice->field_0x10 - now <= 0) {
            pDevice->field_0xc = dirButtons;
            pDevice->field_0x10 = (int)m_keyboardSpeed + now;
        } else {
            pDevice->field_0xc = 0;
        }
    } else {
        pDevice->field_0xc = 0;
        pDevice->field_0x10 = 0;
    }

    if (m_unk0x0052086c != 0)
        pDevice->field_0x8 |= pDevice->field_0xc;

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
    pParams->triggerButton = triggerButton == -1 ? INPUT_EFFECT_NO_TRIGGER : INPUT_OFFSET_BUTTON(triggerButton);

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
short CInput::GetButtonMapping(unsigned short controller, int button)
{
    int index;
    ControllerData *pController;
    short mapping;

    mapping = 0;
    index = m_unk0x005168f4[controller] * sizeof(ControllerData);
    switch (button) {
    case 0: mapping = *(short *)((BYTE *)&m_controllerInfo[0].field_0x128 + index); break;
    case 1: mapping = *(short *)((BYTE *)&m_controllerInfo[0].field_0x12a + index); break;
    case 2: mapping = *(short *)((BYTE *)&m_controllerInfo[0].field_0x12c + index); break;
    case 3: mapping = *(short *)((BYTE *)&m_controllerInfo[0].field_0x12e + index); break;
    case 4: mapping = *(short *)((BYTE *)&m_controllerInfo[0].field_0x130 + index); break;
    case 5: mapping = *(short *)((BYTE *)&m_controllerInfo[0].field_0x132 + index); break;
    case 6: mapping = *(short *)((BYTE *)&m_controllerInfo[0].field_0x134 + index); break;
    case 7: mapping = *(short *)((BYTE *)&m_controllerInfo[0].field_0x136 + index); break;
    case 8: mapping = *(short *)((BYTE *)&m_controllerInfo[0].field_0x138 + index); break;
    case 9: mapping = *(short *)((BYTE *)&m_controllerInfo[0].field_0x13a + index); break;
    default: goto defaults;
    }

    if (mapping == 0) {
defaults:
        if (controller == 0 || controller == 1)
            goto fixed;
    }

    pController = (ControllerData *)((BYTE *)m_controllerInfo + index);
    if (!IsControllerActionBound(button, pController))
        return mapping;

fixed:
    switch (button) {
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
    pParams->triggerButton = triggerButton == -1 ? INPUT_EFFECT_NO_TRIGGER : INPUT_OFFSET_BUTTON(triggerButton);

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
void CInput::SaveControllerInfo(void)
{
    unsigned int *buffer;

    buffer = (unsigned int *)CFileBuffer::AllocateLockedBuffer(0x11a4);
    memcpy(buffer, m_controllerInfo, sizeof(m_controllerInfo));
    buffer[0x468] = *(unsigned int *)m_unk0x005168f4;
    sprintf(CFrontend::m_stringDest, m_strControllerInfoDir, CInstallInfo::GetGameHDPath());
    CInstallInfo::WriteFileToDisk(CFrontend::m_stringDest, 2, buffer, 0x11a4);
    CFileBuffer::FreeGenericFileBuffer(buffer);
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
    pController->field_0x10c = (pDevice->field_0x0 == 3 || pDevice->field_0x0 == 2) ? 1 : 0;
    pController->field_0x110 = (pDevice->field_0x0 == 3 || pDevice->field_0x0 == 2) ? 1 : 0;
    pController->field_0x114 = (pDevice->field_0x0 == 3 || pDevice->field_0x0 == 2) ? 1 : 0;
    pController->field_0x118 = pDevice->supportsForceFeedback;
    m_controllerInfo[index].field_0x4 = 0x8000;
    m_controllerInfo[index].field_0x0 = 0x8000;
    if (pDevice->field_0x0 == 1) {
        if (index == 0) {
            m_controllerInfo[0].field_0x13e[0] = pDevice->keyboard.field_0x468;
            strcpy(m_controllerInfo[0].keyNames[0], CFrontend::GetTextString(0x1f9));
            m_controllerInfo[0].field_0x13e[1] = pDevice->keyboard.field_0x469;
            strcpy(m_controllerInfo[0].keyNames[1], CFrontend::GetTextString(0x1fa));
            m_controllerInfo[0].field_0x13e[2] = pDevice->keyboard.field_0x46a;
            strcpy(m_controllerInfo[0].keyNames[2], CFrontend::GetTextString(0x1fb));
            m_controllerInfo[0].field_0x13e[3] = pDevice->keyboard.field_0x46b;
            strcpy(m_controllerInfo[0].keyNames[3], CFrontend::GetTextString(0x1fc));
            m_controllerInfo[0].field_0x13e[4] = pDevice->keyboard.field_0x46c;
            strcpy(m_controllerInfo[0].keyNames[4], CFrontend::GetTextString(0x1fd));
            m_controllerInfo[0].field_0x13e[5] = pDevice->keyboard.field_0x46d;
            strcpy(m_controllerInfo[0].keyNames[5], g_keyCloseBracket);
            m_controllerInfo[0].field_0x13e[6] = pDevice->keyboard.field_0x46e;
            strcpy(m_controllerInfo[0].keyNames[6], g_keyOpenBracket);
            m_controllerInfo[0].field_0x13e[7] = pDevice->keyboard.field_0x46f;
            strcpy(m_controllerInfo[0].keyNames[7], g_keyC);
            m_controllerInfo[0].field_0x13e[8] = pDevice->keyboard.field_0x470;
            strcpy(m_controllerInfo[0].keyNames[8], g_keyR);
            return;
        }
        pController->field_0x13e[0] = pDevice->keyboard.field_0x468;
        strcpy(pController->keyNames[0], FormatString(g_keypadFormat, CFrontend::GetTextString(0x203), g_keypad4));
        pController->field_0x13e[1] = pDevice->keyboard.field_0x469;
        strcpy(pController->keyNames[1], FormatString(g_keypadFormat, CFrontend::GetTextString(0x203), g_keypad6));
        pController->field_0x13e[2] = pDevice->keyboard.field_0x46a;
        strcpy(pController->keyNames[2], FormatString(g_keypadFormat, CFrontend::GetTextString(0x203), g_keypad8));
        pController->field_0x13e[3] = pDevice->keyboard.field_0x46b;
        strcpy(pController->keyNames[3], FormatString(g_keypadFormat, CFrontend::GetTextString(0x203), g_keypad2));
        pController->field_0x13e[4] = pDevice->keyboard.field_0x46c;
        strcpy(pController->keyNames[4], FormatString(g_keypadFormat, CFrontend::GetTextString(0x203), g_keypad3));
        pController->field_0x13e[5] = pDevice->keyboard.field_0x46d;
        strcpy(pController->keyNames[5], CFrontend::GetTextString(0x204));
        pController->field_0x13e[6] = pDevice->keyboard.field_0x46e;
        strcpy(pController->keyNames[6], CFrontend::GetTextString(0x205));
        pController->field_0x13e[7] = pDevice->keyboard.field_0x46f;
        strcpy(pController->keyNames[7], FormatString(g_keypadFormat, CFrontend::GetTextString(0x203), g_keypad9));
        pController->field_0x13e[8] = pDevice->keyboard.field_0x470;
        strcpy(pController->keyNames[8], FormatString(g_keypadFormat, CFrontend::GetTextString(0x203), g_keypad7));
        return;
    }
    if (pDevice->field_0x0 == 2) {
        pDevice->joystick.bindings[0].present = 1;
        pController->field_0x210[0] = *(ControllerDataUnk0x210 *)&pDevice->joystick.bindings[0];
        pController->field_0x2d8[0] = 0;
        pController->field_0x210[1] = *(ControllerDataUnk0x210 *)&pDevice->joystick.bindings[0];
        pController->field_0x2d8[1] = 0;
        pController->field_0x128 = 0;
        pController->field_0x12a = 0;
        pController->field_0x12c = 0x10;
        pController->field_0x12e = 0x20;
        pController->field_0x130 = 0;
        pController->field_0x132 = 0;
        pController->field_0x134 = 0;
        pController->field_0x136 = 0;
        pController->field_0x138 = 0;
        pController->field_0x13a = 0;
        pController->field_0x13e[4] = (BYTE)pDevice->joystick.bindings[0].present;
        strcpy(pController->keyNames[4], CFrontend::GetTextString(0x1fe));
        pController->field_0x13e[5] = pDevice->keyboard.field_0x46d;
        strcpy(pController->keyNames[5], CFrontend::GetTextString(0x1fe));
        pController->field_0x13e[6] = pDevice->keyboard.field_0x46e;
        strcpy(pController->keyNames[6], CFrontend::GetTextString(0x1fe));
        pController->field_0x13e[7] = pDevice->keyboard.field_0x46f;
        strcpy(pController->keyNames[7], CFrontend::GetTextString(0x1fe));
        pController->field_0x13e[8] = pDevice->keyboard.field_0x470;
        strcpy(pController->keyNames[8], CFrontend::GetTextString(0x1fe));
        return;
    }
    // PORT: a gamepad (Xbox layout, src/input) starts with the right trigger
    // (Rz) as throttle and the left (Z) as brake, each read as a pedal, rather
    // than both on the left stick's Y axis as for other joysticks.
    if (pController->field_0x10c != 0 && Input_IsGamepad(m_unk0x0059f6b0[pDevice->field_0x18]) &&
        pDevice->joystick.bindings[0].present != 0 && pDevice->joystick.bindings[2].present != 0 &&
        pDevice->joystick.bindings[5].present != 0) {
        pController->field_0x128 = 1;
        pController->field_0x12a = 2;
        pController->field_0x210[0] = *(ControllerDataUnk0x210 *)&pDevice->joystick.bindings[0];
        pController->field_0x2d8[0] = 0;
        pController->field_0x210[1] = *(ControllerDataUnk0x210 *)&pDevice->joystick.bindings[0];
        pController->field_0x2d8[1] = 0;
        pController->field_0x12c = 4;
        pController->field_0x210[2] = *(ControllerDataUnk0x210 *)&pDevice->joystick.bindings[5];
        pController->field_0x2d8[2] = 5;
        pController->field_0x12e = 8;
        pController->field_0x210[3] = *(ControllerDataUnk0x210 *)&pDevice->joystick.bindings[2];
        pController->field_0x2d8[3] = 2;
        pController->field_0x130 = 0x10;
        pController->field_0x132 = 0x20;
        pController->field_0x134 = 0x40;
        pController->field_0x136 = 0x80;
        pController->field_0x138 = 0x100;
        pController->field_0x13a = 0x200;
#ifdef __ANDROID__
        pController->field_0x13a = 0x800; // Start pauses on handhelds.
#endif
        return;
    }
    if (pController->field_0x10c != 0) {
        pController->field_0x128 = 1;
        pController->field_0x12a = 2;
        if (pDevice->joystick.bindings[0].present != 0) {
            pController->field_0x210[0] = *(ControllerDataUnk0x210 *)&pDevice->joystick.bindings[0];
            pController->field_0x2d8[0] = 0;
            if (pDevice->joystick.bindings[0].present != 0) {
                pController->field_0x210[1] = *(ControllerDataUnk0x210 *)&pDevice->joystick.bindings[0];
                pController->field_0x2d8[1] = 0;
            }
        }
        pController->field_0x12c = 4;
        if (pDevice->joystick.bindings[1].present != 0) {
            pController->field_0x210[2] = *(ControllerDataUnk0x210 *)&pDevice->joystick.bindings[1];
            pController->field_0x2d8[2] = 1;
        }
        pController->field_0x12e = 8;
        if (pDevice->joystick.bindings[1].present != 0) {
            pController->field_0x210[3] = *(ControllerDataUnk0x210 *)&pDevice->joystick.bindings[1];
            pController->field_0x2d8[3] = 1;
        }
        pController->field_0x130 = 0x10;
        pController->field_0x132 = 0x20;
        pController->field_0x134 = 0x40;
        pController->field_0x136 = 0x80;
        pController->field_0x138 = 0x100;
        pController->field_0x13a = 0x200;
        return;
    }
    memset(pController->field_0x210, 0, 0x32 * 4);
    pController->field_0x128 = 1;
    pController->field_0x12a = 2;
    pController->field_0x12c = 0x10;
    pController->field_0x12e = 0x20;
    pController->field_0x130 = 0x40;
    pController->field_0x132 = 0x80;
    pController->field_0x134 = 0x100;
    pController->field_0x136 = 0x200;
    pController->field_0x138 = 0x800;
    pController->field_0x13a = 0x400;
}

// PORT: whether player 1's device follows the controllers connected (the
// first one, or the keyboard without any) rather than a choice made in the
// controls menu ([input] auto_player1 in opencmr2.ini). The original kept
// player 1 on the keyboard until it was changed there, with the keyboard.
static BOOL Input_Player1Automatic(void)
{
    return Sys_GetOption("input.auto_player1", 1) != 0;
}

// The device player 1 follows automatically: of the controllers player 2
// does not use, the first wheel, else the first gamepad, else the first
// other joystick; the keyboard without any. Android lists some built-in
// devices of phones as joysticks: only gamepads and wheels count there.
unsigned short Input_GetAutomaticPlayer1Device(void)
{
    int best = 0;
    int bestRank = 0;
    int rank;
    int i;

    for (i = 2; i < CInput::m_controllerCount && i < 8; i++) {
        int joystick = CInput::m_availableDevices[i].field_0x18;
        if (i == CInput::m_unk0x005168f4[1] || CInput::m_availableDevices[i].field_0x0 != 3 || joystick < 0 || joystick > 3)
            continue;
        if (Input_IsWheel(CInput::m_unk0x0059f6b0[joystick]))
            rank = 3;
        else if (Input_IsGamepad(CInput::m_unk0x0059f6b0[joystick]))
            rank = 2;
        else
#ifdef __ANDROID__
            continue;
#else
            rank = 1;
#endif
        if (rank > bestRank) {
            best = i;
            bestRank = rank;
        }
    }
    return (unsigned short)best;
}

// Called when the controls menu stores player 1's device: choosing another
// than the automatic one keeps that choice; choosing it again follows the
// controllers again.
void Input_StorePlayer1Choice(void)
{
    BOOL automatic = CInput::m_unk0x005168f4[0] == Input_GetAutomaticPlayer1Device();

    if (Input_Player1Automatic() != automatic)
        Sys_SetOption("input.auto_player1", automatic);
}

// FUNCTION: CMR2 0x0040c050
void CInput::RefreshControllerConfigurations(void)
{
    DeviceInfo *pDevice;
    int i;
    // PORT: a controller keeps its configuration, and the players theirs,
    // when the controllers are listed in another order than when it was
    // saved, as they can be from one start to the next. The original matched
    // configuration i with device i by name only, and reset it otherwise.
    static ControllerData saved[6];
    int moved[6];
    BOOL taken[6];
    int j;
    int slot;
    BOOL lost;

    memcpy(saved, m_controllerInfo, sizeof(saved));
    for (j = 0; j < 6; j++) {
        moved[j] = -1;
        taken[j] = FALSE;
    }
    m_controllerCount = CountAttachedInputDevices();
    for (i = 0; i < m_controllerCount && i < 6; i++) {
        pDevice = UpdateDevice(i);
        j = -1;
        if (m_hasLoadedControllerInfo != 0) {
            if (strcmp(saved[i].name, pDevice->deviceInstanceName) == 0 && !taken[i])
                j = i;
            for (slot = 2; j < 0 && i >= 2 && slot < 6; slot++)
                if (!taken[slot] && strcmp(saved[slot].name, pDevice->deviceInstanceName) == 0)
                    j = slot;
        }
        if (j < 0) {
            memset(&m_controllerInfo[i], 0, sizeof(m_controllerInfo[i]));
            InitDetectedControllerSlot(pDevice, i);
            continue;
        }
        taken[j] = TRUE;
        moved[j] = i;
        if (j != i) {
            m_controllerInfo[i] = saved[j];
            m_controllerInfo[i].index = (BYTE)i;
        }
    }
    if (m_controllerCount < 6)
        memset(&m_controllerInfo[m_controllerCount], 0, (6 - m_controllerCount) * sizeof(ControllerData));
    // A player whose controller is gone goes back to the keyboard.
    lost = FALSE;
    for (slot = 0; slot < 8; slot++) {
        j = m_unk0x005168f4[slot];
        if (j < 2 && j < m_controllerCount)
            continue;
        if (j < 6 && moved[j] >= 0) {
            m_unk0x005168f4[slot] = (unsigned short)moved[j];
            continue;
        }
        m_unk0x005168f4[slot] = (unsigned short)(slot < 2 ? slot : 0);
        if (slot == 0)
            lost = TRUE;
    }
    // PORT: player 1 starts on the first controller connected, so one works
    // before the controls menu is reached (Steam Deck, Android handhelds),
    // until another device is chosen for player 1 there.
    if (Input_Player1Automatic() || lost)
        m_unk0x005168f4[0] = Input_GetAutomaticPlayer1Device();
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
    // PORT: controllers connected or disconnected since are taken in the
    // menus, so one can be used without a keyboard to open the controls menu.
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
        s_pendingRacePresses[i] |= device->field_0x8;
        device->field_0x8 = 0;
        if ((readyDevices & (1u << i)) != 0) {
            device->field_0x8 = s_pendingRacePresses[i];
            s_pendingRacePresses[i] = 0;
        }
    }
}

// match 57%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0040bc90
void CInput::SetControllerForceFeedbackValue(unsigned short param1, DWORD param2)
{
    unsigned short index;

    index = CInput::m_unk0x005168f4[param1];
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
// (bits 2 and 3). Gamepads use their stick and POV for menu navigation.
// FUNCTION: CMR2 0x0040bad0
void Input_TranslatePedalsToMenuKeys(void)
{
    unsigned int i;
    DeviceInfo *pDev;

    for (i = 0; i < 8; i++) {
        pDev = CInput::GetAvailableDeviceRecord(i);
        // PORT: gamepads navigate with the stick and POV already read by
        // ReadJoystick. Their separate driving triggers must not replace
        // those menu presses. Keep pedal navigation for wheels/joysticks.
        if (pDev->field_0x0 == 3 && pDev->field_0x18 >= 0 && pDev->field_0x18 < 8 &&
            Input_IsGamepad(CInput::m_unk0x0059f6b0[pDev->field_0x18])) {
            g_latchedThrottleAxes[i][0] = g_latchedThrottleAxes[i][1] = 0;
            continue;
        }
        if (pDev->field_0x0 == 3 && CInput::GetEnabledControllerAxisBinding(0, 2) != CInput::GetEnabledControllerAxisBinding(0, 3)) {
            pDev->field_0x8 &= 0xfffffff3;
            if (((int *)pDev)[CInput::GetEnabledControllerAxisBinding(0, 2) * 5 + 0x11f] < 0x3333) {
                if (g_latchedThrottleAxes[i][0] == 0) {
                    pDev->field_0x8 |= 4;
                    g_latchedThrottleAxes[i][0] = 1;
                }
            } else {
                g_latchedThrottleAxes[i][0] = 0;
            }
            if (((int *)pDev)[CInput::GetEnabledControllerAxisBinding(0, 3) * 5 + 0x11f] < 0x3333) {
                if (g_latchedThrottleAxes[i][1] == 0) {
                    pDev->field_0x8 |= 8;
                    g_latchedThrottleAxes[i][1] = 1;
                }
            } else {
                g_latchedThrottleAxes[i][1] = 0;
            }
        }
    }
}

// Controller slots are mapped to entries of m_controllerInfo through m_unk0x005168f4.
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
    return CInput::m_unk0x005168f4[slot];
}

// FUNCTION: CMR2 0x0040bbe0
void Input_SetControllerSlotMapping(unsigned short slot, unsigned short index)
{
    CInput::m_unk0x005168f4[slot] = index;
}

// FUNCTION: CMR2 0x0040bc00
unsigned int Input_GetControllerPrimaryFlags(unsigned short slot)
{
    return CInput::m_controllerInfo[CInput::m_unk0x005168f4[slot]].field_0x0;
}

// FUNCTION: CMR2 0x0040bc30
unsigned int Input_GetControllerSecondaryFlags(unsigned short slot)
{
    return CInput::m_controllerInfo[CInput::m_unk0x005168f4[slot]].field_0x4;
}

// FUNCTION: CMR2 0x0040bc60
void Input_SetControllerPrimaryFlags(unsigned short slot, unsigned int value)
{
    CInput::m_controllerInfo[CInput::m_unk0x005168f4[slot]].field_0x0 = value;
}

// FUNCTION: CMR2 0x0040bcd0
DWORD Input_GetControllerField11C(unsigned short slot)
{
    return CInput::m_controllerInfo[CInput::m_unk0x005168f4[slot]].field_0x11c;
}

// FUNCTION: CMR2 0x0040bd00
void Input_SetControllerSecondaryFlags(unsigned short slot, unsigned int value)
{
    CInput::m_controllerInfo[CInput::m_unk0x005168f4[slot]].field_0x4 = value;
}

// FUNCTION: CMR2 0x0040bd30
DWORD Input_GetControllerField118(unsigned short slot)
{
    return CInput::m_controllerInfo[CInput::m_unk0x005168f4[slot]].field_0x118;
}

// Merges the buttons of the joystick assigned to the slot into pOut.
// FUNCTION: CMR2 0x0040bd60
void Input_MergeAssignedJoystickButtons(int slot, DeviceInfo *pOut)
{
    DeviceInfo *pDev;
    int i;

    if ((short)Input_GetControllerSlotMapping(slot) != 0 && (short)Input_GetControllerSlotMapping(slot) != 1) {
        pDev = CInput::GetAvailableDeviceRecord(Input_GetControllerSlotMapping(slot));
        if (pDev->field_0x0 == 0 || pDev->field_0x0 == 3) {
            pOut->field_0x8 |= pDev->field_0x8;
            pOut->field_0x4 |= pDev->field_0x4;
            pOut->field_0xc |= pDev->field_0xc;
        }
    }
    // PORT: player 1's menus also take every controller player 2 does not
    // use, so any connected controller works in them, also one not chosen in
    // the controls menu yet. Only the chosen one drives.
    if (slot != 0)
        return;
    for (i = 2; i < 8; i++) {
        pDev = CInput::GetAvailableDeviceRecord(i);
        if (i == (short)Input_GetControllerSlotMapping(0) || i == (short)Input_GetControllerSlotMapping(1) ||
            (pDev->field_0x0 != 0 && pDev->field_0x0 != 3) || pDev->field_0x18 < 0 || pDev->field_0x18 > 3)
            continue;
        pOut->field_0x8 |= pDev->field_0x8;
        pOut->field_0x4 |= pDev->field_0x4;
        pOut->field_0xc |= pDev->field_0xc;
    }
}

// PORT: menu input of the controller a page of the controls menu sets up,
// which the original left to the keyboard. On the calibration page its
// stick and pedals are being tested: only its buttons and its D-pad count.
void Input_MergeConfiguredDeviceButtons(int index, DeviceInfo *pOut, BOOL padOnly)
{
    static unsigned int previousHat[8];
    DeviceInfo *pDev;
    unsigned int hat;

    if (index < 2 || index >= 8)
        return;
    pDev = CInput::GetAvailableDeviceRecord(index);
    if ((pDev->field_0x0 != 0 && pDev->field_0x0 != 3) || pDev->field_0x18 < 0 || pDev->field_0x18 > 3)
        return;
    hat = s_hatDirections[index];
    if (!padOnly) {
        pOut->field_0x8 |= pDev->field_0x8;
        pOut->field_0x4 |= pDev->field_0x4;
        pOut->field_0xc |= pDev->field_0xc;
    } else {
        pOut->field_0x8 |= (pDev->field_0x8 & ~CInput::m_directionButtonMask) | (hat & ~previousHat[index]);
        pOut->field_0x4 |= (pDev->field_0x4 & ~CInput::m_directionButtonMask) | hat;
    }
    previousHat[index] = hat;
}

// FUNCTION: CMR2 0x0040bdd0
DWORD Input_GetControllerField110(unsigned short slot)
{
    return CInput::m_controllerInfo[CInput::m_unk0x005168f4[slot]].field_0x110;
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
