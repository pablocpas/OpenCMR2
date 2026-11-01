#include "port/sys.h"
#include "FrontendMenus.h"
#include "GameInfo.h"
#include "Graphics.h"
#include "Frontend.h"
#include "FrontendDraw.h"
#include "Input.h"
#include "Font.h"
#include "Sprite.h"
#include "Texture.h"
#include "StageInfo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "Game.h"
#include "RallyData.h"

extern BYTE g_profilePlayersRemaining;
extern int g_screenPaletteIds[23];

#define PATH_X() ((int)(g_pGraphics->resX * 24) / 640)
bool RallyData_HasCategoryAward(int index, int bit, char check);

// Lays out the route scroller and measures the available entries.
// FUNCTION: CMR2 0x004faa50
void FrontendMenu_InitRouteScroller(Menu *pMenu, int param)
{
    MenuScroller *pScroller;
    int i;
    int count;

    count = 0;
    i = 0;
    Frontend_SetOverlayMode(CGameInfo::GetConfiguredPlayerCount() - g_profilePlayersRemaining - 1);
    FrontendScroller_GetRouteEntryScroller()->pMenu = pMenu;
    pMenu->cursor = 0;
    for (; i < 0x16; i++) {
        if (CGameInfo::GetConfiguredGameMode() == 7 || CGameInfo::GetConfiguredGameMode() == 5 || CGameInfo::GetConfiguredGameMode() == 6) {
            if (RallyData_HasCategoryAward(CGameInfo::GetConfiguredPlayerCount() - g_profilePlayersRemaining - 1, i, 1)) {
                g_screenPaletteIds[count] = i;
                pMenu->items[count].id = i + 0x98;
                strcpy(CFrontend::m_stringDest, CFrontend::GetTextString((short)(i + 0x98)));
                CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
                FrontendScroller_GetRouteEntryScroller()->widths[count] = Font_GetTextWidth(2, (BYTE *)CFrontend::m_stringDest);
                if (i == RallyData_GetDriverRecordSelectionValue(CGameInfo::GetConfiguredPlayerCount() + (0xff - g_profilePlayersRemaining)))
                    pMenu->cursor = count;
                count++;
            }
        }
    }
    pMenu->itemCount = count;
    pScroller = FrontendScroller_GetRouteEntryScroller();
    pScroller->spacing = PATH_X();
    pScroller->count = pMenu->itemCount;
    pScroller->offset = 0;
    pScroller->startOffset = 0;
    for (i = 0; i < pMenu->itemCount; i++) {
        strcpy(CFrontend::m_stringDest, CFrontend::GetTextString(pMenu->items[i].id));
        CGenericFileLoader::StrLowerPolish(CFrontend::m_stringDest);
        pScroller->widths[i] = Font_GetTextWidth(2, (BYTE *)CFrontend::m_stringDest);
    }
}

BYTE RallyData_GetDriverAutomaticGearbox(BYTE index);

// Change callback of the championship entry screens. Its external byte state
// preserves the original byte loads without changing the other screen callbacks.
// FUNCTION: CMR2 0x004fac70
void FrontendMenu_UpdateChampionshipEntry(Menu *pMenu, char param)
{
    pMenu->items[0].max = RallyData_GetDriverAutomaticGearbox(CGameInfo::GetConfiguredPlayerCount() + (0xff - g_profilePlayersRemaining));
    Frontend_SetOverlayMode(CGameInfo::GetConfiguredPlayerCount() - g_profilePlayersRemaining - 1);
    if (param != 0)
        FrontendMenu_InitRouteScroller(FrontendScroller_GetRouteEntryScroller()->pMenu, 0);
}

// GLOBAL: CMR2 0x00525c30
StageInfoEntry g_eventEntries[8] = {
    { 1, 7, 1, 0, { { 0, 100 } } },
    { 2, 3, 1, 0, { { 2, 100 } } },
    { 2, 0, 1, 0, { { 1, 100 } } },
    { 2, 3, 1, 0, { { 1, 100 } } },
    { 2, 1, 1, 0, { { 1, 100 } } },
    { 2, 6, 1, 0, { { 0, 100 } } },
    { 2, 8, 1, 0, { { 1, 100 } } },
    { 3, 2, 2, 0, { { 4, 60 }, { 0, 40 } } },
};

// GLOBAL: CMR2 0x00525d50
StageInfoEntry g_stageEntries[88] = {
    { 5, 0, 2, 0, { { 1, 80 }, { 4, 20 } } },
    { 4, 9, 2, 0, { { 1, 70 }, { 4, 30 } } },
    { 2, 0, 2, 0, { { 1, 75 }, { 4, 25 } } },
    { 5, 0, 2, 0, { { 1, 86 }, { 4, 14 } } },
    { 5, 0, 2, 0, { { 1, 80 }, { 4, 20 } } },
    { 5, 0, 2, 0, { { 1, 90 }, { 4, 10 } } },
    { 5, 0, 2, 0, { { 1, 90 }, { 4, 10 } } },
    { 5, 0, 2, 0, { { 1, 80 }, { 4, 20 } } },
    { 5, 1, 2, 0, { { 1, 40 }, { 4, 60 } } },
    { 1, 9, 2, 0, { { 1, 50 }, { 4, 50 } } },
    { 2, 1, 1, 0, { { 1, 100 } } },
    { 5, 3, 1, 0, { { 1, 100 } } },
    { 5, 1, 1, 0, { { 1, 100 } } },
    { 5, 1, 1, 0, { { 1, 100 } } },
    { 5, 0, 1, 0, { { 1, 100 } } },
    { 5, 0, 1, 0, { { 1, 100 } } },
    { 2, 1, 1, 0, { { 1, 100 } } },
    { 4, 8, 1, 0, { { 1, 100 } } },
    { 5, 0, 1, 0, { { 1, 100 } } },
    { 5, 0, 1, 0, { { 1, 100 } } },
    { 5, 0, 1, 0, { { 1, 100 } } },
    { 2, 0, 1, 0, { { 1, 100 } } },
    { 5, 0, 1, 0, { { 0, 100 } } },
    { 4, 8, 1, 0, { { 0, 100 } } },
    { 4, 8, 1, 0, { { 0, 100 } } },
    { 5, 0, 1, 0, { { 0, 100 } } },
    { 5, 0, 1, 0, { { 0, 100 } } },
    { 4, 9, 1, 0, { { 0, 100 } } },
    { 5, 0, 1, 0, { { 0, 100 } } },
    { 2, 0, 1, 0, { { 0, 100 } } },
    { 2, 2, 1, 0, { { 0, 100 } } },
    { 4, 7, 1, 0, { { 0, 100 } } },
    { 2, 6, 1, 0, { { 0, 100 } } },
    { 4, 9, 1, 0, { { 2, 100 } } },
    { 4, 7, 2, 0, { { 2, 80 }, { 3, 20 } } },
    { 2, 0, 1, 0, { { 2, 100 } } },
    { 4, 8, 1, 0, { { 2, 100 } } },
    { 5, 2, 1, 0, { { 2, 100 } } },
    { 4, 9, 1, 0, { { 2, 100 } } },
    { 4, 3, 1, 0, { { 2, 100 } } },
    { 4, 9, 1, 0, { { 2, 100 } } },
    { 2, 0, 1, 0, { { 2, 100 } } },
    { 4, 8, 1, 0, { { 2, 100 } } },
    { 2, 6, 1, 0, { { 2, 100 } } },
    { 5, 1, 1, 0, { { 1, 100 } } },
    { 4, 4, 1, 0, { { 1, 100 } } },
    { 5, 0, 1, 0, { { 1, 100 } } },
    { 5, 3, 2, 0, { { 0, 5 }, { 1, 95 } } },
    { 5, 3, 2, 0, { { 1, 70 }, { 0, 30 } } },
    { 2, 1, 2, 0, { { 1, 99 }, { 0, 1 } } },
    { 5, 3, 2, 0, { { 0, 5 }, { 1, 95 } } },
    { 5, 3, 2, 0, { { 1, 70 }, { 0, 30 } } },
    { 4, 9, 1, 0, { { 1, 100 } } },
    { 2, 1, 1, 0, { { 1, 100 } } },
    { 2, 3, 1, 0, { { 1, 100 } } },
    { 4, 3, 2, 0, { { 0, 35 }, { 1, 65 } } },
    { 4, 9, 1, 0, { { 1, 100 } } },
    { 1, 9, 2, 0, { { 1, 90 }, { 0, 10 } } },
    { 4, 3, 2, 0, { { 0, 35 }, { 1, 65 } } },
    { 5, 4, 2, 0, { { 1, 90 }, { 0, 10 } } },
    { 5, 3, 3, 0, { { 4, 40 }, { 1, 30 }, { 0, 30 } } },
    { 2, 9, 1, 0, { { 1, 100 } } },
    { 5, 4, 2, 0, { { 1, 90 }, { 0, 10 } } },
    { 4, 7, 2, 0, { { 4, 60 }, { 1, 40 } } },
    { 5, 1, 2, 0, { { 4, 80 }, { 1, 20 } } },
    { 2, 8, 1, 0, { { 1, 100 } } },
    { 4, 2, 1, 0, { { 0, 100 } } },
    { 4, 8, 1, 0, { { 0, 100 } } },
    { 4, 9, 1, 0, { { 0, 100 } } },
    { 4, 9, 1, 0, { { 0, 100 } } },
    { 4, 6, 1, 0, { { 0, 100 } } },
    { 2, 3, 1, 0, { { 0, 100 } } },
    { 5, 0, 1, 0, { { 0, 100 } } },
    { 1, 9, 1, 0, { { 0, 100 } } },
    { 5, 3, 1, 0, { { 0, 100 } } },
    { 4, 9, 1, 0, { { 0, 100 } } },
    { 1, 7, 1, 0, { { 0, 100 } } },
    { 4, 0, 2, 0, { { 0, 20 }, { 4, 80 } } },
    { 2, 8, 2, 0, { { 4, 80 }, { 0, 20 } } },
    { 4, 9, 2, 0, { { 4, 55 }, { 0, 45 } } },
    { 4, 0, 2, 0, { { 0, 20 }, { 4, 80 } } },
    { 5, 0, 1, 0, { { 1, 100 } } },
    { 4, 6, 2, 0, { { 4, 25 }, { 1, 75 } } },
    { 5, 0, 1, 0, { { 1, 100 } } },
    { 2, 2, 2, 0, { { 4, 50 }, { 1, 50 } } },
    { 5, 1, 1, 0, { { 1, 100 } } },
    { 5, 1, 1, 0, { { 1, 100 } } },
    { 3, 2, 1, 0, { { 4, 100 } } },
};

// Pool of stage/event name literals. The original keeps it right after
// g_stageNames (0x526b30, ending where the 0x526df0 pointer table starts);
// both tables below point into it.
// GLOBAL: CMR2 0x00526b30
const char g_stageNameStrings[] =
    "Kington" "\0"
    "Penybont" "\0\0\0\0"
    "Caersw" "\0\0"
    "Ripley" "\0\0"
    "Darton" "\0\0"
    "Melsonby" "\0\0\0\0"
    "Dalton" "\0\0"
    "Richmond" "\0\0\0\0"
    "Rieti" "\0\0\0"
    "Celano" "\0\0"
    "Valenza" "\0"
    "Voghera" "\0"
    "Castel" "\0\0"
    "Lodi" "\0\0\0\0"
    "Vignola" "\0"
    "Carpi" "\0\0\0"
    "Modena" "\0\0"
    "Mirwani" "\0"
    "Pelekech" "\0\0\0\0"
    "Lodwar" "\0\0"
    "Choba" "\0\0\0"
    "Holta" "\0\0\0"
    "Buna" "\0\0\0\0"
    "Kalossia" "\0\0\0\0"
    "Kapuitr" "\0"
    "Lokichar" "\0\0\0\0"
    "Kooline" "\0"
    "Warrie" "\0\0"
    "Mornington" "\0\0"
    "Billiluna" "\0\0\0"
    "Ellendale" "\0\0\0"
    "Wiluna" "\0\0"
    "Earaheady" "\0\0\0"
    "Wongawol" "\0\0\0\0"
    "Ljungby" "\0"
    "Skara" "\0\0\0"
    "Tranas" "\0\0"
    "Falun" "\0\0\0"
    "Arvika" "\0\0"
    "Karlstad" "\0\0\0\0"
    "Ange" "\0\0\0\0"
    "Bispfors" "\0\0\0\0"
    "Hoting" "\0\0"
    "Mora" "\0\0\0\0"
    "Laon" "\0\0\0\0"
    "Chauny" "\0\0"
    "Damville" "\0\0\0\0"
    "Dreux" "\0\0\0"
    "Falaise" "\0"
    "Livarot" "\0"
    "Authon" "\0\0"
    "Ballon" "\0\0"
    "Veroia" "\0\0"
    "Kozani" "\0\0"
    "Siatista" "\0\0\0\0"
    "Grammos" "\0"
    "Smolikas" "\0\0\0\0"
    "Kilkis" "\0\0"
    "Dodona" "\0\0"
    "Kalabaka" "\0\0\0\0"
    "Kivotos" "\0"
    "Nowhere" "\0"
    "Tampere" "\0"
    "Jamsa" "\0\0\0"
    "Kittila" "\0"
    "Rovaniemi" "\0\0\0"
    "Kemijarvi" "\0\0\0"
    "Joensuu" "\0"
    "Kupio" "\0\0\0"
    "Mikkeli" "\0"
    "Redmire" "\0"
    "Lokichokio" "\0\0"
    "Zonza" "\0\0\0"
    "Ivalo" "\0\0\0"
    "Cables" "\0\0"
    "Tomaros" "\0"
    "Lysvik" "\0\0"
    "Berceto";

// GLOBAL: CMR2 0x005269b0
const char *g_eventNames[8] = {
    g_stageNameStrings + 0x2b8, g_stageNameStrings + 0x2b0, g_stageNameStrings + 0x2a8, g_stageNameStrings + 0x2a0,
    g_stageNameStrings + 0x298, g_stageNameStrings + 0x290, g_stageNameStrings + 0x284, g_stageNameStrings + 0x27c,
};

// GLOBAL: CMR2 0x005269d0
const char *g_stageNames[88] = {
    g_stageNameStrings + 0x274, g_stageNameStrings + 0x26c, g_stageNameStrings + 0x264, g_stageNameStrings + 0x274,
    g_stageNameStrings + 0x258, g_stageNameStrings + 0x24c, g_stageNameStrings + 0x244, g_stageNameStrings + 0x258,
    g_stageNameStrings + 0x23c, g_stageNameStrings + 0x234, g_stageNameStrings + 0x22c, g_stageNameStrings + 0x224,
    g_stageNameStrings + 0x218, g_stageNameStrings + 0x210, g_stageNameStrings + 0x208, g_stageNameStrings + 0x1fc,
    g_stageNameStrings + 0x1f4, g_stageNameStrings + 0x1e8, g_stageNameStrings + 0x1e0, g_stageNameStrings + 0x208,
    g_stageNameStrings + 0x1e0, g_stageNameStrings + 0x1d8, g_stageNameStrings + 0x1d0, g_stageNameStrings + 0x1c8,
    g_stageNameStrings + 0x1c0, g_stageNameStrings + 0x1b8, g_stageNameStrings + 0x1b8, g_stageNameStrings + 0x1c0,
    g_stageNameStrings + 0x1b0, g_stageNameStrings + 0x1a4, g_stageNameStrings + 0x19c, g_stageNameStrings + 0x194,
    g_stageNameStrings + 0x22c, g_stageNameStrings + 0x18c, g_stageNameStrings + 0x184, g_stageNameStrings + 0x178,
    g_stageNameStrings + 0x170, g_stageNameStrings + 0x164, g_stageNameStrings + 0x15c, g_stageNameStrings + 0x154,
    g_stageNameStrings + 0x18c, g_stageNameStrings + 0x14c, g_stageNameStrings + 0x144, g_stageNameStrings + 0x13c,
    g_stageNameStrings + 0x130, g_stageNameStrings + 0x124, g_stageNameStrings + 0x11c, g_stageNameStrings + 0x110,
    g_stageNameStrings + 0x104, g_stageNameStrings + 0xf8, g_stageNameStrings + 0x110, g_stageNameStrings + 0x104,
    g_stageNameStrings + 0xf0, g_stageNameStrings + 0xe8, g_stageNameStrings + 0x22c, g_stageNameStrings + 0xdc,
    g_stageNameStrings + 0xd4, g_stageNameStrings + 0xc8, g_stageNameStrings + 0xdc, g_stageNameStrings + 0xc0,
    g_stageNameStrings + 0xb8, g_stageNameStrings + 0xb0, g_stageNameStrings + 0xc0, g_stageNameStrings + 0xa8,
    g_stageNameStrings + 0x9c, g_stageNameStrings + 0x94, g_stageNameStrings + 0x8c, g_stageNameStrings + 0x84,
    g_stageNameStrings + 0x7c, g_stageNameStrings + 0x74, g_stageNameStrings + 0x6c, g_stageNameStrings + 0x64,
    g_stageNameStrings + 0x74, g_stageNameStrings + 0x5c, g_stageNameStrings + 0x54, g_stageNameStrings + 0x4c,
    g_stageNameStrings + 0x22c, g_stageNameStrings + 0x40, g_stageNameStrings + 0x38, g_stageNameStrings + 0x2c,
    g_stageNameStrings + 0x40, g_stageNameStrings + 0x24, g_stageNameStrings + 0x1c, g_stageNameStrings + 0x24,
    g_stageNameStrings + 0x1c, g_stageNameStrings + 0x14, g_stageNameStrings + 0x8, g_stageNameStrings + 0x0,
};

// FUNCTION: CMR2 0x004f89b0
BYTE FrontendRecords_GetStageEntryPayload(StageSurfaceShare **out, int row, int column)
{
    int index = column + row * 11;
    *out = g_stageEntries[index].surfaces;
    return g_stageEntries[index].surfaceCount;
}

// FUNCTION: CMR2 0x004f89e0
const char *FrontendRecords_GetStageName(int row, int column)
{
    return g_stageNames[column + row * 11];
}

// FUNCTION: CMR2 0x004f8a00
StageInfoEntry *FrontendRecords_GetStageEntry(int row, int column)
{
    return &g_stageEntries[column + row * 11];
}

// FUNCTION: CMR2 0x004f8a20
BYTE FrontendRecords_GetEventEntryPayload(StageSurfaceShare **out, int index)
{
    *out = g_eventEntries[index].surfaces;
    return g_eventEntries[index].surfaceCount;
}

// FUNCTION: CMR2 0x004f8a40
const char *FrontendRecords_GetEventName(int index)
{
    return g_eventNames[index];
}

// FUNCTION: CMR2 0x004f8a50
StageInfoEntry *FrontendRecords_GetEventEntry(int index)
{
    return &g_eventEntries[index];
}

// GLOBAL: CMR2 0x00825398
BYTE g_eventRecordTexts[0x4c];
// GLOBAL: CMR2 0x008253e4
BYTE g_profileCarClassRecordTexts[0x98];
// GLOBAL: CMR2 0x0082547c
BYTE g_stageRecordTexts[0x898];
// GLOBAL: CMR2 0x00825d14
BYTE g_carClassRecordTexts[0x258];
// GLOBAL: CMR2 0x00825f6c
BYTE g_championshipRecordTexts[0x1cc];

// FUNCTION: CMR2 0x004f9240
BYTE *FrontendRecords_GetStageRecordText(int row, int column)
{
    return g_stageRecordTexts + (column + row * 11) * 25;
}

// FUNCTION: CMR2 0x004f9260
BYTE *FrontendRecords_GetCarClassRecordText(int row, int column)
{
    return g_carClassRecordTexts + (column + row * 3) * 25;
}

// FUNCTION: CMR2 0x004f9280
BYTE *FrontendRecords_GetEventRecordText(int index)
{
    return g_eventRecordTexts + index * 25;
}

// FUNCTION: CMR2 0x004f92a0
BYTE *FrontendRecords_GetProfileCarClassRecordText(int row, int column)
{
    return g_profileCarClassRecordTexts + (column + row * 3) * 25;
}

// FUNCTION: CMR2 0x004f92c0
BYTE *FrontendRecords_GetChampionshipRecordText(int index)
{
    return g_championshipRecordTexts + index * 25;
}

// Working copy of the 6 controller configurations edited by the controls
// menus (copied back to CInput::m_controllerInfo when leaving).
// GLOBAL: CMR2 0x00829448
ControllerData g_controlsCopy[6];

// Calibration of one axis as shown by the controls menu.
// GLOBAL: CMR2 0x0082a5e8
JoystickBinding g_axisBindings[8];

void FrontendControls_ClearDuplicateBinding(ControllerData *p, int index);
void Input_GetLastKeyName(LPSTR pName, unsigned int size);
unsigned int Input_GetControllerEffectsForce(unsigned short slot);
unsigned int Input_GetControllerTextureForce(unsigned short slot);
DWORD Input_GetControllerQuickForceUpdate(unsigned short slot);
void Input_SetControllerEffectsForce(unsigned short slot, unsigned int value);
void Input_SetControllerTextureForce(unsigned short slot, unsigned int value);
void Input_RescanChangedJoysticks(void);
void Input_SetControllerSlotMapping(unsigned short slot, unsigned short index);
unsigned short Input_GetControllerSlotMapping(unsigned short slot);
unsigned short Input_GetAssignedController(void);

// Separator line under the rows of the controls pages.
// GLOBAL: CMR2 0x0082af70
short g_controlsLine[4];
// Player device slots and the adjacent device last shown by the settings page.
struct ControlsDeviceSelection {
    unsigned short slots[8];
    unsigned short lastDevice;
};
// GLOBAL: CMR2 0x0082a7c8
ControlsDeviceSelection g_controlsDeviceSelection;
// Set when the devices must be re-read (the game lost the focus).
// GLOBAL: CMR2 0x0082a7dc
int g_controlsRereadDevices;
// GLOBAL: CMR2 0x0082a7e4
int g_controlsBindingCaptureState;
// GLOBAL: CMR2 0x0082a7e8
int g_controlsCalibrationState;
// GLOBAL: CMR2 0x0082a7ec
int g_controlsSelectedSlot;

// Configuration of the device selected in the controls menu.
#define CONTROLS_SEL (g_controlsDeviceSelection.slots[g_controlsSelectedSlot & 0xffff])

// FUNCTION: CMR2 0x004fba80
short FrontendControls_GetSelectedSlot(void)
{
    return (short)g_controlsSelectedSlot;
}

// Item callback of the controls list: selects the device of this entry.
// FUNCTION: CMR2 0x004fbea0
void FrontendControls_SelectDevice(Menu *pMenu, int param)
{
    *(short *)&g_controlsSelectedSlot = pMenu->cursor;
}

// Enables and shows the 8 entries of the controls list.
// FUNCTION: CMR2 0x004fba90
void FrontendControls_ShowBindingEntries(Menu *pMenu, int param)
{
    MenuItem *pItem;
    int i;

    Input_RescanChangedJoysticks();   // PORT
    i = 8;
    pItem = pMenu->items;
    do {
        i--;
        pItem->enabled = 1;
        pItem->visible = 1;
        pItem++;
    } while (i != 0);
}

// GLOBAL: CMR2 0x00829428
int g_unk0x00829428[8];
// GLOBAL: CMR2 0x0082a7e0
int g_unk0x0082a7e0;

void Input_TranslatePedalsToMenuKeys(void);
void Input_ClearKeyPressQueue(void);

// Starts redefining a control: freezes the menu, waits for the keys to be
// released and snapshots the axes of the selected joystick/mouse.
// FUNCTION: CMR2 0x004fbec0
void FrontendControls_BeginBindingCapture(Menu *pMenu, int param)
{
    DeviceInfo *pDevice;
    int *pOut;
    int i;

    g_controlsBindingCaptureState = 1;
    Menu_SetFlags(pMenu, 0, 0, 0, 0);
    CInput::UpdateAllAvailableDevices();
    Input_TranslatePedalsToMenuKeys();
    g_unk0x0082a7e0 = CInput::GetFirstPressedKey();
    Input_ClearKeyPressQueue();
    pDevice = CInput::GetAvailableDeviceRecord(g_controlsDeviceSelection.slots[g_controlsSelectedSlot & 0xffff]);
    for (i = 0; i < 8; i++)
        g_unk0x00829428[i] = 0;
    if (pDevice->type == 3 || pDevice->type == 2) {
        for (i = 0; i < 8; i++) {
            if (pDevice->joystick.bindings[i].present != 0)
                g_unk0x00829428[i] = pDevice->joystick.bindings[i].position;
        }
    }
}

// Menu callback of the device page: shows the entries the selected device
// supports (no calibration entry for keyboard/mouse or fewer than 4 axes, no
// axis entries for the mouse).
// FUNCTION: CMR2 0x004fbf60
void FrontendControls_EnterDeviceBindings(Menu *pMenu, char param)
{
    DeviceInfo *pDevice;

    if (param == 0) {
        pDevice = CInput::GetAvailableDeviceRecord(g_controlsDeviceSelection.slots[g_controlsSelectedSlot & 0xffff]);
        pMenu->items[1].visible = 1;
        pMenu->items[0].enabled = 1;
        pMenu->items[0].visible = 1;
        pMenu->items[1].enabled = 1;
        pMenu->items[9].enabled = 1;
        pMenu->items[9].visible = 1;
        if (pDevice->type == 1 || pDevice->type == 2 || pDevice->buttonCount < 4) {
            pMenu->items[9].enabled = 0;
            pMenu->items[9].visible = 0;
        }
        if (pDevice->type == 2) {
            pMenu->items[0].enabled = 0;
            pMenu->items[0].visible = 0;
            pMenu->items[1].enabled = 0;
            pMenu->items[1].visible = 0;
        }
    }
}

ControllerData *Input_GetControllerTable(void);

// Leaving the device page: "back" goes to the controls menu; otherwise the
// edited configuration is stored and a joystick goes on to its calibration.
// FUNCTION: CMR2 0x004fbff0
void FrontendControls_LeaveDeviceBindings(Menu *pMenu, char back)
{
    if (back != 0) {
        Menu_SetNextAction(FrontendMenu_GetDeviceConfiguration());
        return;
    }
    memcpy(Input_GetControllerTable(), g_controlsCopy, sizeof(g_controlsCopy));
    CInput::ApplyControllerButtonBindings(g_controlsSelectedSlot & 0xffff);
    if (CInput::GetAvailableDeviceRecord(g_controlsDeviceSelection.slots[g_controlsSelectedSlot & 0xffff])->type == 3)
        Menu_SetNextAction(FrontendMenu_GetDeviceCalibration());
}

// Whether the entry under the cursor is bound: entries 0/1 by analogSteering,
// 2/3 by analogPedals of the selected configuration.
// FUNCTION: CMR2 0x004fc490
int FrontendControls_IsSelectedAxisBound(Menu *pMenu)
{
    if (g_controlsCopy[g_controlsDeviceSelection.slots[g_controlsSelectedSlot & 0xffff]].analogSteering != 0 && (pMenu->cursor == 0 || pMenu->cursor == 1))
        return 1;
    if (g_controlsCopy[g_controlsDeviceSelection.slots[g_controlsSelectedSlot & 0xffff]].analogPedals != 0 && (pMenu->cursor == 2 || pMenu->cursor == 3))
        return 1;
    return 0;
}

// Menu callback while a control is being redefined: waits for a new key,
// button or axis movement (more than 0.3 of the range), stores it for the
// entry under the cursor and unfreezes the menu.
// match 77%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004fc070
void FrontendControls_UpdateBindingCapture(Menu *pMenu)
{
    DeviceInfo *pDevice;
    int *pPosition;
    int *pReference;
    unsigned int buttons;
    BOOL done;
    BOOL axisPair;
    short axis;
    char count;
    int key;
    int i;

    done = FALSE;
    if (g_controlsBindingCaptureState == 0)
        return;
    Menu_SetFlags(pMenu, 0, 0, 0, 0);
    pDevice = CInput::GetAvailableDeviceRecord(CONTROLS_SEL);
    key = CInput::GetFirstPressedKey();
    if (g_unk0x0082a7e0 != -1) {
        g_unk0x0082a7e0 = key;
    } else if (key != -1 && FrontendControls_IsSelectedAxisBound(pMenu) == 0) {
        done = TRUE;
        Input_GetLastKeyName(g_controlsCopy[CONTROLS_SEL].keyNames[pMenu->cursor], 20);
        CInput::SetIndexedByteBinding(g_controlsCopy[CONTROLS_SEL].keys, pMenu->cursor, key);
        if (pDevice->type != 1)
            CInput::SetIndexedWordBinding(g_controlsCopy[CONTROLS_SEL].actionMasks, pMenu->cursor, 0);
    }
    axisPair = g_controlsCopy[CONTROLS_SEL].analogSteering != 0 && (pMenu->cursor == 0 || pMenu->cursor == 1);
    if (((g_controlsCopy[CONTROLS_SEL].analogPedals != 0 && (pMenu->cursor == 2 || pMenu->cursor == 3)) || axisPair)
        && (pDevice->type == 3 || pDevice->type == 2)) {
        pPosition = &pDevice->joystick.bindings[0].position;
        pReference = g_unk0x00829428;
        axis = 0;
        do {
            if (pPosition[-4] != 0 && abs(*pReference - *pPosition) > 0x4ccc) {
                done = TRUE;
                g_controlsCopy[CONTROLS_SEL].axisBindings[pMenu->cursor] = *(ControllerDataUnk0x210 *)(pPosition - 4);
                g_controlsCopy[CONTROLS_SEL].axisIndices[pMenu->cursor] = axis;
                CInput::SetIndexedWordBinding(g_controlsCopy[CONTROLS_SEL].actionMasks, pMenu->cursor, 0);
                CInput::SetIndexedByteBinding(g_controlsCopy[CONTROLS_SEL].keys, pMenu->cursor, 0);
                *pReference = *pPosition;
            }
            axis++;
            pPosition += 5;
            pReference++;
        } while (pReference < &g_unk0x00829428[8]);
    }
    if (pDevice->type == 3 || pDevice->type == 2) {
        if (g_controlsCopy[CONTROLS_SEL].analogSteering != 0)
            pDevice->pressed &= ~3;
        if (g_controlsCopy[CONTROLS_SEL].analogPedals != 0)
            pDevice->pressed &= ~0xc;
    }
    buttons = pDevice->pressed;
    count = 0;
    for (i = 32; i != 0; i--) {
        if (buttons & 1)
            count++;
        buttons >>= 1;
    }
    if (count == 1 && pDevice->type != 1 && FrontendControls_IsSelectedAxisBound(pMenu) == 0) {
        CInput::SetIndexedWordBinding(g_controlsCopy[CONTROLS_SEL].actionMasks, pMenu->cursor, (unsigned short)pDevice->pressed);
        CInput::SetIndexedByteBinding(g_controlsCopy[CONTROLS_SEL].keys, pMenu->cursor, 0);
    } else if (!done) {
        return;
    }
    CInput::UpdateAllAvailableDevices();
    Input_TranslatePedalsToMenuKeys();
    g_controlsBindingCaptureState = 0;
    Menu_SetFlags(pMenu, 1, 1, 1, 1);
    FrontendControls_ClearDuplicateBinding(&g_controlsCopy[CONTROLS_SEL], pMenu->cursor);
}

// Menu callback of the calibration page: one entry per axis the device has,
// with the stored calibration of the axis; the last entry leads to the
// pad or the joystick page.
// match 63%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004fc500
void FrontendControls_EnterCalibration(Menu *pMenu, int param)
{
    ControllerData *pData;
    unsigned int dev;
    unsigned int axis;
    JoystickBinding *pBinding;
    MenuItem *pItem;
    int *pFlag;
    int j;

    dev = CONTROLS_SEL;
    pItem = pMenu->items;
    pFlag = (int *)((BYTE *)CInput::GetAvailableDeviceRecord(dev) + 0x46c);
    axis = 0;
    pBinding = g_axisBindings;
    do {
        if (*pFlag != 0) {
            pData = &Input_GetControllerTable()[dev];
            pItem->enabled = 1;
            pItem->visible = 1;
            for (j = 0; j < 10; j++) {
                if (pData->axisIndices[j] == axis && pData->axisBindings[j].present != 0)
                    *(ControllerDataUnk0x210 *)pBinding = pData->axisBindings[j];
            }
        } else {
            pItem->enabled = 0;
            pItem->visible = 0;
        }
        pBinding++;
        axis++;
        pFlag += 5;
        pItem++;
    } while (pBinding < &g_axisBindings[8]);
    if (g_controlsCopy[dev].forceFeedback != 0)
        pMenu->items[axis].pSubMenu = FrontendMenu_GetPadSettings();
    else
        pMenu->items[axis].pSubMenu = FrontendMenu_GetControls();
}

// Menu callback while calibrating an axis: left/right (shift: saturation)
// move the deadzone in steps of 50; the confirm button applies every axis to
// the device and the reset button takes the device's current values.
// match 68%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004fc620
void FrontendControls_UpdateAxisCalibration(Menu *pMenu)
{
    DeviceInfo *pKeys;
    ControllerData *pData;
    JoystickBinding *pBinding;
    DeviceInfo *pDevice;
    unsigned int held;
    BOOL deadzone;
    int axis;
    int j;

    if (g_controlsCalibrationState != 0) {
        pKeys = CInput::GetAvailableDeviceRecord(0);
        if (pKeys->pressed & 0x10) {
            pData = &Input_GetControllerTable()[CONTROLS_SEL];
            for (axis = 0; axis < 8; axis++) {
                CInput::SetJoystickAxisSaturation(CONTROLS_SEL, axis, g_axisBindings[axis].saturation);
                CInput::SetJoystickAxisDeadzone(CONTROLS_SEL, axis, g_axisBindings[axis].deadzone);
                for (j = 0; j < 10; j++) {
                    if (pData->axisIndices[j] == axis && pData->axisBindings[j].present != 0)
                        pData->axisBindings[j] = *(ControllerDataUnk0x210 *)&g_axisBindings[axis];
                }
            }
            g_controlsCalibrationState = 0;
        }
        if (pKeys->pressed & 0x20) {
            pDevice = CInput::GetAvailableDeviceRecord(CONTROLS_SEL);
            g_axisBindings[pMenu->cursor].deadzone = pDevice->joystick.bindings[pMenu->cursor].deadzone;
            g_controlsCalibrationState = 0;
            g_axisBindings[pMenu->cursor].saturation = pDevice->joystick.bindings[pMenu->cursor].saturation;
        }
        held = pKeys->held;
        // PORT: up and down also set the deadzone, as Shift with left and
        // right does: a controller has no Shift (see FrontendControls_Text).
        deadzone = CInput::IsShiftPressed() != 0;
        if (held & 0xc) {
            deadzone = TRUE;
            held = (held & 4) ? 1 : 2;
        }
        // PORT: the original moved the marks once per frame, at the display's
        // refresh; without vertical sync that was a few hundred times a
        // second. They move at 60 steps a second.
        static SysStepClock clock;
        for (j = Sys_TakeSteps(&clock, 60, 5); j > 0; j--) {
            if (!deadzone) {
                if (held & 1) {
                    if (g_axisBindings[pMenu->cursor].saturation < 0x26de)
                        g_axisBindings[pMenu->cursor].saturation += 0x32;
                } else if (held & 2) {
                    if (g_axisBindings[pMenu->cursor].saturation > 0x32)
                        g_axisBindings[pMenu->cursor].saturation -= 0x32;
                }
            } else if (held & 1) {
                if (g_axisBindings[pMenu->cursor].deadzone > 0x32)
                    g_axisBindings[pMenu->cursor].deadzone -= 0x32;
            } else if (held & 2) {
                if (g_axisBindings[pMenu->cursor].deadzone < 0x26de)
                    g_axisBindings[pMenu->cursor].deadzone += 0x32;
            }
        }
        if (g_controlsCalibrationState != 0)
            return;
    }
    Menu_SetFlags(pMenu, 1, 1, 1, 1);
}

// Loads the pad page from the slot's settings: two sensitivities (0..10)
// and whether the pad vibrates.
// FUNCTION: CMR2 0x004fc880
void FrontendControls_EnterPadSettings(Menu *pMenu, int param)
{
    pMenu->items[0].max = (char)((int)(Input_GetControllerEffectsForce((unsigned short)g_controlsSelectedSlot) * 10) / 0x10000);
    pMenu->items[1].max = (char)((int)(Input_GetControllerTextureForce((unsigned short)g_controlsSelectedSlot) * 10) / 0x10000);
    if (Input_GetControllerQuickForceUpdate((unsigned short)g_controlsSelectedSlot) != 0)
        pMenu->items[2].max = 0;
    else
        pMenu->items[2].max = 1;
}

// Leaving the pad page: stores the settings unless backing out.
// FUNCTION: CMR2 0x004fc8f0
void FrontendControls_LeavePadSettings(Menu *pMenu, char back)
{
    if (back == 0) {
        Input_SetControllerEffectsForce((unsigned short)g_controlsSelectedSlot, (pMenu->items[0].max << 16) / 10);
        Input_SetControllerTextureForce((unsigned short)g_controlsSelectedSlot, (pMenu->items[1].max << 16) / 10);
        CInput::SetControllerQuickForceUpdate((unsigned short)g_controlsSelectedSlot, 1 - pMenu->items[2].max);
    }
}

// Leaving the device settings page: stores the slot assignments and the
// edited configurations unless backing out.
// FUNCTION: CMR2 0x004fc970
void FrontendControls_LeaveDeviceConfiguration(Menu *pMenu, char back)
{
    unsigned short *pSlot;
    int i;

    if (back == 0) {
        i = 0;
        pSlot = g_controlsDeviceSelection.slots;
        do {
            Input_SetControllerSlotMapping(i, *pSlot);
            pSlot++;
            i++;
        } while ((INT_PTR)pSlot < (INT_PTR)(g_controlsDeviceSelection.slots + 8));
        memcpy(Input_GetControllerTable(), g_controlsCopy, sizeof(g_controlsCopy));
    }
}

// Fills the device settings page from the configuration of the slot.
// FUNCTION: CMR2 0x004fcb30
void FrontendControls_FillDeviceConfiguration(void)
{
    Menu *pMenu;
    DeviceInfo *pDevice;
    unsigned short dev;

    pMenu = FrontendMenu_GetDeviceConfiguration();
    dev = CONTROLS_SEL;
    pMenu->items[0].max = (BYTE)dev;
    pDevice = CInput::UpdateDevice(dev);
    if (pDevice == NULL) {
        CONTROLS_SEL = (unsigned short)g_controlsSelectedSlot;
        dev = (unsigned short)g_controlsSelectedSlot;
        pDevice = CInput::UpdateDevice(g_controlsSelectedSlot & 0xffff);
    }
    pMenu->items[1].enabled = pDevice->type == 3;
    pMenu->items[1].max = (BYTE)g_controlsCopy[dev].analogSteering;
    pMenu->items[2].enabled = pDevice->type == 3;
    pMenu->items[2].max = (BYTE)g_controlsCopy[dev].analogPedals;
    pMenu->items[4].enabled = pDevice->type == 3;
    pMenu->items[5].enabled = pDevice->type == 3;
    if (pDevice->supportsForceFeedback != 0)
        pMenu->items[3].enabled = 1;
    else
        pMenu->items[3].enabled = 0;
    if (g_controlsCopy[dev].forceFeedback != 0)
        pMenu->items[3].max = 0;
    else
        pMenu->items[3].max = 1;
    if (g_controlsCopy[dev].throttleOnPositive != 0)
        pMenu->items[4].max = 0;
    else
        pMenu->items[4].max = 1;
    if (g_controlsCopy[dev].smoothSteering != 0)
        pMenu->items[5].max = 0;
    else
        pMenu->items[5].max = 1;
}

// Entering the device settings page: copies the slot assignments and every
// configuration.
// FUNCTION: CMR2 0x004fc9b0
void FrontendControls_EnterDeviceConfiguration(Menu *pMenu, int param)
{
    unsigned short *pSlot;
    int i;

    CInput::RefreshControllerConfigurations();
    i = 0;
    pSlot = g_controlsDeviceSelection.slots;
    do {
        *pSlot = Input_GetControllerSlotMapping(i);
        pSlot++;
        i++;
    } while ((INT_PTR)pSlot < (INT_PTR)(g_controlsDeviceSelection.slots + 8));
    memcpy(g_controlsCopy, Input_GetControllerTable(), sizeof(g_controlsCopy));
    FrontendControls_FillDeviceConfiguration();
}

// Update callback of the device settings page: leaves when no device is
// left, re-reads the devices after the game was inactive, then either stores
// the options or switches the slot to the configuration chosen in entry 0.
// match 59%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004fc9f0
void FrontendControls_UpdateDeviceConfiguration(Menu *pMenu)
{
    unsigned int dev;

    if (Input_GetAssignedController() == 0) {
        Menu_SetNextAction(FrontendMenu_GetOptions());
        return;
    }
    if (CGame::IsActive() == 0) {
        if (g_controlsRereadDevices != 0) {
            CInput::RebuildAvailableInputDevices();
            CInput::RefreshControllerConfigurations();
            FrontendControls_EnterDeviceConfiguration(pMenu, 0);
            Input_GetAssignedController();
            g_controlsRereadDevices = 0;
        }
    } else if (g_controlsRereadDevices != 0) {
        return;
    }
    if (pMenu->items[0].max == g_controlsDeviceSelection.lastDevice) {
        dev = CONTROLS_SEL;
        g_controlsCopy[dev].analogSteering = pMenu->items[1].max;
        g_controlsCopy[dev].analogPedals = pMenu->items[2].max;
        g_controlsCopy[dev].forceFeedback = 1 - pMenu->items[3].max;
        g_controlsCopy[dev].throttleOnPositive = 1 - pMenu->items[4].max;
        g_controlsCopy[dev].smoothSteering = 1 - pMenu->items[5].max;
        return;
    }
    if (pMenu->items[0].max == 0 && (short)g_controlsSelectedSlot == 1)
        pMenu->items[0].max = (char)g_controlsSelectedSlot;
    if (pMenu->items[0].max == 1 && (short)g_controlsSelectedSlot == 0)
        pMenu->items[0].max = g_controlsDeviceSelection.lastDevice != 0 ? 0 : 2;
    CONTROLS_SEL = pMenu->items[0].max;
    FrontendControls_FillDeviceConfiguration();
    g_controlsDeviceSelection.lastDevice = pMenu->items[0].max;
}

// Starts calibrating: freezes the menu and waits for the keys to be released.
// FUNCTION: CMR2 0x004fc850
void FrontendControls_BeginCalibration(Menu *pMenu, int param)
{
    g_controlsCalibrationState = 1;
    Menu_SetFlags(pMenu, 0, 0, 0, 0);
    CInput::UpdateAllAvailableDevices();
    Input_TranslatePedalsToMenuKeys();
}

// Clears every other binding of the configuration that uses the same button
// or key as binding `index`.
// FUNCTION: CMR2 0x004fcc50
void FrontendControls_ClearDuplicateBinding(ControllerData *p, int index)
{
    unsigned short *pButton;
    char *pName;
    int i;

    i = 0;
    pName = p->keyNames[0];
    pButton = p->actionMasks;
    do {
        if (i != index) {
            if (*pButton == p->actionMasks[index])
                *pButton = 0;
            if (p->keys[i] == p->keys[index]) {
                p->keys[i] = 0;
                *pName = 0;
            }
        }
        i++;
        pButton++;
        pName += 20;
    } while (i < 10);
}

// Current position of axis `index` of the selected device.
// FUNCTION: CMR2 0x004fbe60
JoystickBinding *FrontendControls_GetSelectedAxisPosition(int index)
{
    DeviceInfo *pDevice = CInput::GetAvailableDeviceRecord(g_controlsDeviceSelection.slots[g_controlsSelectedSlot & 0xffff]);

    g_axisBindings[index].position = pDevice->joystick.bindings[index].position;
    return &g_axisBindings[index];
}


// GLOBAL: CMR2 0x0082a788
char g_bindingText[64];

// Text shown for binding `index` of the selected device: the key name, the
// button name, "axis N", or the direction names of a pad/mouse.
// match 71%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004fbae0
char *FrontendControls_GetBindingDisplayText(int index)
{
    DeviceInfo *pDevice;
    unsigned int button;
    unsigned int dev;
    int i;

    pDevice = CInput::GetAvailableDeviceRecord(CONTROLS_SEL);
    if (pDevice->type == 1)
        return g_controlsCopy[CONTROLS_SEL].keyNames[index];
    if (CInput::IsControllerActionBound(index, &g_controlsCopy[CONTROLS_SEL]) != 0)
        return g_controlsCopy[CONTROLS_SEL].keyNames[index];
    dev = CONTROLS_SEL;
    if (g_controlsCopy[dev].axisBindings[index].present == 0 || pDevice->type != 3) {
        button = g_controlsCopy[dev].actionMasks[index];
        if (pDevice->type == 0 && button == 1)
            return CFrontend::GetTextString(0x1f9);
        if (pDevice->type == 0 && button == 2)
            return CFrontend::GetTextString(0x1fa);
        if (pDevice->type == 0 && button == 4)
            return CFrontend::GetTextString(0x1fb);
        if (pDevice->type == 0 && button == 8)
            return CFrontend::GetTextString(0x1fc);
        if (pDevice->type == 2 && index == 0)
            return CFrontend::GetTextString(0x1f9);
        if (pDevice->type == 2 && index == 1)
            return CFrontend::GetTextString(0x1fa);
        i = CInput::GetButtonIndexFromMask(button);
        if (i != -1)
            return pDevice->field_0x284[i];
    } else if (g_controlsCopy[dev].analogSteering == 0 && (index == 0 || index == 1)) {
        button = g_controlsCopy[dev].actionMasks[index];
        i = CInput::GetButtonIndexFromMask(button);
        if (i != -1) {
            if (strcmp(pDevice->field_0x284[i], CMain::m_logFileBlankLine) != 0)
                return pDevice->field_0x284[i];
            if (button == 1)
                return CFrontend::GetTextString(0x1f9);
            return CFrontend::GetTextString(0x1fa);
        }
    } else if (g_controlsCopy[dev].analogPedals != 0 || (index != 2 && index != 3)) {
        sprintf(g_bindingText, CFrontend::GetTextString(0x77), g_controlsCopy[dev].axisIndices[index]);
        return g_bindingText;
    } else {
        button = g_controlsCopy[dev].actionMasks[index];
        i = CInput::GetButtonIndexFromMask(button);
        if (i != -1) {
            if (strcmp(pDevice->field_0x284[i], CMain::m_logFileBlankLine) != 0)
                return pDevice->field_0x284[i];
            if (button == 4)
                return CFrontend::GetTextString(0x1fb);
            return CFrontend::GetTextString(0x1fc);
        }
    }
    return g_controlsCopy[CONTROLS_SEL].keyNames[index];
}

// FUNCTION: CMR2 0x004fbab0
BYTE *FrontendControls_GetSelectedDeviceName(void)
{
    return (BYTE *)g_controlsCopy[g_controlsDeviceSelection.slots[g_controlsSelectedSlot & 0xffff]].name;
}

// FUNCTION: CMR2 0x004fc060
int FrontendControls_GetBindingCaptureState(void)
{
    return g_controlsBindingCaptureState;
}

// FUNCTION: CMR2 0x004fc610
int FrontendControls_GetCalibrationState(void)
{
    return g_controlsCalibrationState;
}

// Draw callback of the controls menu: title, one row (icon + name) per
// visible entry, separators around the selected row, and the carousel.
// FUNCTION: CMR2 0x004fccb0
void FrontendControls_DrawDeviceList(Menu *pMenu)
{
    short icon[4];
    short line[4];
    BYTE *pLineShadow;
    BYTE *pLineColour;
    BYTE *pShadow;
    BYTE *pColour;
    MenuItem *pItem;
    int textX;
    short y0;
    int row;
    int i;

    row = 0;
    icon[0] = (int)(g_pGraphics->resX * 100) / 640;
    icon[1] = 0;
    icon[2] = CFrontend::m_pAr640ATexture->width;
    icon[3] = CFrontend::m_pAr640ATexture->height;
    FrontendDraw_PlayTime();
    FrontendDraw_BreadcrumbItem((int)(g_pGraphics->resX * 24) / 640, (int)(g_pGraphics->resY * 38) / 480,
                                g_colourWhite0x00524968, 1, CFrontend::GetTextString(pMenu->field_0x4));
    textX = (int)(g_pGraphics->resX * 122) / 640;
    y0 = (int)(g_pGraphics->resY * 170) / 480;
    line[0] = (int)(g_pGraphics->resX * 99) / 640;
    line[1] = y0;
    line[2] = (int)(g_pGraphics->resX * 282) / 640;
    line[3] = 1;
    if (pMenu->cursor == 0) {
        pColour = g_colourWhite0x00524968;
        pShadow = g_colourShadowWhite0x00524974;
    } else {
        pColour = g_colourText0x0052496c;
        pShadow = g_colourShadowText0x00524978;
    }
    Sprite_FillRect(&g_pGraphics->field309_0x150, line, pShadow, 1);
    line[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, line, pColour, 1);
    for (i = 0; i < pMenu->itemCount; i++) {
        pItem = &pMenu->items[i];
        if (pItem->visible) {
            icon[1] = (int)(g_pGraphics->resY * 20) / 480 + y0 + (int)(g_pGraphics->resY * 36) / 480 * row
                      - CFrontend::m_pAr640ATexture->height / 2;
            if (pMenu->cursor == i) {
                pShadow = g_colourWhite0x00524968;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, (SpriteRect *)icon,
                             CFrontend::m_pAr640ATexture, 1, 0, NULL, NULL, g_colourWhite0x00524968, 8);
            } else {
                pShadow = g_colourText0x0052496c;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, (SpriteRect *)icon,
                             CFrontend::m_pAr640DTexture, 1, 0, NULL, NULL, g_colourText0x0052496c, 8);
            }
            if (i < 2)
                sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(100), i + 1);
            else
                sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x67));
            Font_DrawText(1, CFrontend::m_stringDest, textX, (int)(g_pGraphics->resY * 24) / 480 + line[1],
                          (int *)pShadow, 0x11);
            if (pMenu->cursor == i || pMenu->cursor == i + 1) {
                pLineColour = g_colourWhite0x00524968;
                pLineShadow = g_colourShadowWhite0x00524974;
            } else {
                pLineColour = g_colourText0x0052496c;
                pLineShadow = g_colourShadowText0x00524978;
            }
            line[1] = (int)(g_pGraphics->resY * 36) / 480 * (row + 1) + y0;
            Sprite_FillRect(&g_pGraphics->field309_0x150, line, pLineShadow, 1);
            line[1]++;
            Sprite_FillRect(&g_pGraphics->field309_0x150, line, pLineColour, 1);
            row++;
        }
    }
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

extern char g_standingsRowFormat[];
extern BYTE g_colourShadowDim0x0052497c[4];

// Draw callback of the device page: "Controller N | <device>" title and one
// row per visible binding with its current assignment.
// FUNCTION: CMR2 0x004fd080
void FrontendControls_DrawDeviceBindings(Menu *pMenu)
{
    short icon[4];
    short line[4];
    char *text[2];
    BYTE *pLineShadow;
    BYTE *pLineColour;
    BYTE *pShadow;
    BYTE *pColour;
    MenuItem *pItem;
    int textX;
    short y0;
    int row;
    int i;

    icon[0] = (int)(g_pGraphics->resX * 100) / 640;
    icon[1] = 0;
    icon[2] = CFrontend::m_pAr640ATexture->width;
    icon[3] = CFrontend::m_pAr640ATexture->height;
    FrontendDraw_PlayTime();
    sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(100), FrontendControls_GetSelectedSlot() + 1);
    text[0] = CFrontend::m_stringDest;
    text[1] = CFrontend::GetTextString(pMenu->field_0x4);
    FrontendDraw_Breadcrumb((int)(g_pGraphics->resX * 24) / 640, (int)(g_pGraphics->resY * 38) / 480, text, 2);
    textX = (int)(g_pGraphics->resX * 122) / 640;
    y0 = (int)(g_pGraphics->resY * 65) / 480;
    line[0] = (int)(g_pGraphics->resX * 99) / 640;
    line[1] = y0;
    line[2] = (int)(g_pGraphics->resX * 282) / 640;
    line[3] = 1;
    if (pMenu->cursor == 0) {
        pColour = g_colourWhite0x00524968;
        pShadow = g_colourShadowWhite0x00524974;
    } else if (pMenu->items[0].enabled) {
        pColour = g_colourText0x0052496c;
        pShadow = g_colourShadowText0x00524978;
    } else {
        pColour = g_colourDim0x00524970;
        pShadow = g_colourShadowDim0x0052497c;
    }
    Sprite_FillRect(&g_pGraphics->field309_0x150, line, pShadow, 1);
    line[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, line, pColour, 1);
    row = 0;
    for (i = 0; i < pMenu->itemCount; i++) {
        pItem = &pMenu->items[i];
        if (pItem->enabled) {
            icon[1] = (int)(g_pGraphics->resY * 20) / 480 + y0 + (int)(g_pGraphics->resY * 36) / 480 * row
                      - CFrontend::m_pAr640ATexture->height / 2;
            if (pMenu->cursor == i) {
                pShadow = g_colourWhite0x00524968;
                pColour = g_colourWhite0x00524968;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, (SpriteRect *)icon, CFrontend::m_pAr640ATexture, 1, 0, NULL, NULL, pColour, 8);
            } else {
                pShadow = g_colourText0x0052496c;
                pColour = g_colourText0x0052496c;
                Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, (SpriteRect *)icon, CFrontend::m_pAr640DTexture, 1, 0, NULL, NULL, pColour, 8);
            }
            if (i >= 10) {
                sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(pItem->id));
            } else {
                if (FrontendControls_GetBindingCaptureState() != 0 && pMenu->cursor == i) {
                    sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(pItem->id));
                } else {
                    sprintf(CFrontend::m_stringDest, g_standingsRowFormat, CFrontend::GetTextString(pItem->id), FrontendControls_GetBindingDisplayText(i));
                }
            }
            Font_DrawText(1, CFrontend::m_stringDest, textX, (int)(g_pGraphics->resY * 24) / 480 + line[1],
                          (int *)pShadow, 0x11);
            if (pMenu->cursor == i || pMenu->cursor == i + 1) {
                pLineColour = g_colourWhite0x00524968;
                pLineShadow = g_colourShadowWhite0x00524974;
            } else {
                pLineColour = g_colourText0x0052496c;
                pLineShadow = g_colourShadowText0x00524978;
            }
            line[1] = (int)(g_pGraphics->resY * 36) / 480 * (row + 1) + y0;
            Sprite_FillRect(&g_pGraphics->field309_0x150, line, pLineShadow, 1);
            line[1]++;
            Sprite_FillRect(&g_pGraphics->field309_0x150, line, pLineColour, 1);
            row++;
        }
    }
}

void FrontendControls_DrawCalibrationAxisRow(short x, int y, Menu *pMenu, int index);

// GLOBAL: CMR2 0x00526ec8
char g_strPercentFormat[8] = "%s %d%%";
// GLOBAL: CMR2 0x00526ed0
char g_strSlashFormat[8] = "%s / %s";
// GLOBAL: CMR2 0x00526ed8
char g_strAngleFormat[8] = "< %s >";

// Settings rows: a label at x, then the two choices of the entry, the active
// one bright; every position is rescaled from 640x480 at each use.
#define FM_SX(v) (g_pGraphics->resX * (v) / 640)
#define FM_SY(v) (g_pGraphics->resY * (v) / 480)
#define FM_LABEL(text)                                                                     \
    Font_DrawText(1, text, x, FM_SY(24) + line[1], (int *)pLabel, 0x11);                  \
    x = Font_GetTextWidth(1, (BYTE *)(text)) + FM_SX(10) + x;
#define FM_CHOICE(idA, idB)                                                                \
    if (pItem->max == 0) {                                                                 \
        Font_DrawText(1, CFrontend::GetTextString(idA), x, FM_SY(24) + line[1], (int *)pBright, 0x11); \
        x = FM_SX(10) + x + Font_GetTextWidth(1, (BYTE *)CFrontend::GetTextString(idA));   \
        Font_DrawText(1, CFrontend::GetTextString(idB), x, FM_SY(24) + line[1], (int *)pDim, 0x11); \
    } else {                                                                               \
        Font_DrawText(1, CFrontend::GetTextString(idA), x, FM_SY(24) + line[1], (int *)pDim, 0x11); \
        x = FM_SX(10) + x + Font_GetTextWidth(1, (BYTE *)CFrontend::GetTextString(idA));   \
        Font_DrawText(1, CFrontend::GetTextString(idB), x, FM_SY(24) + line[1], (int *)pBright, 0x11); \
    }

// Draws the two choices of an entry after its label at x: the active one
// (A when value is 0) in pBright, the other one in pDim.
inline void FrontendMenus_DrawChoice(int x, int y, BYTE value, int idA, int idB, BYTE *pBright, BYTE *pDim)
{
    int xB;

    Font_DrawText(1, CFrontend::GetTextString(idA), x, y, (int *)(value == 0 ? pBright : pDim), 0x11);
    xB = (int)(g_pGraphics->resX * 10) / 640 + x + Font_GetTextWidth(1, (BYTE *)CFrontend::GetTextString(idA));
    Font_DrawText(1, CFrontend::GetTextString(idB), xB, y, (int *)(value == 0 ? pDim : pBright), 0x11);
}

// Draws a label at x and returns where its choices start.
inline int FrontendMenus_DrawLabel(char *text, int x, int y, BYTE *pColour)
{
    Font_DrawText(1, text, x, y, (int *)pColour, 0x11);
    return Font_GetTextWidth(1, (BYTE *)text) + (int)(g_pGraphics->resX * 10) / 640 + x;
}

// PORT: help of the calibration page for keyboards and controllers alike, in
// the game's character set (Windows-1252; Polish letters in place of some
// symbols: 0xb1 is l with a stroke, 0xbd s acute, 0xbe z acute).
static const char *const g_controlsCalibrationText[6][3] = {
    { "Left/Right: adjust saturation.",
      "Up/Down, or Shift with Left/Right: adjust deadzone.",
      "Return or the confirm button: accept the axis settings." },
    { "Gauche/droite : r\xe9gler la saturation.",
      "Haut/bas, ou Maj avec gauche/droite : r\xe9gler la zone morte.",
      "Entr\xe9" "e ou bouton de validation : confirmer l'axe." },
    { "Izquierda/derecha: ajustar la saturaci\xf3n.",
      "Arriba/abajo, o May\xfas con izquierda/derecha: ajustar la zona muerta.",
      "Retorno o el bot\xf3n de aceptar: confirmar el eje." },
    { "Sinistra/destra: regola la saturazione.",
      "Su/gi\xf9, o Maiusc con sinistra/destra: regola la zona morta.",
      "Invio o pulsante di conferma: conferma l'asse." },
    { "Links/Rechts: S\xe4ttigung einstellen.",
      "Hoch/Runter oder Umschalt mit Links/Rechts: Totzone einstellen.",
      "Eingabe oder Best\xe4tigungstaste: Achse \xfc" "bernehmen." },
    { "Lewo/prawo: nasycenie.",
      "G\xf3ra/d\xf3\xb1 lub Shift ze strza\xb1kami: martwa strefa.",
      "Enter lub przycisk akceptacji: zatwierd\xbe o\xbd." },
};

// A help line in the language the menus show, or Polish for that edition.
static char *FrontendControls_Text(int line)
{
    int language = CGameInfo::GetGameRegion() == 3 ? 5 : CFrontend::m_unk0x0065aa70;

    if (language < 0 || language > 5)
        language = 0;
    return (char *)g_controlsCalibrationText[language][line];
}

// Draw callback of the calibration page: one row per axis ("axis N" and its
// calibration bar), "back", and the key help at the bottom.
// match 79%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004fd480
void FrontendControls_DrawCalibration(Menu *pMenu)
{
    short icon[4];
    char *text[2];
    BYTE *pShadow;
    BYTE *pColour;
    int maxWidth;
    int width;
    short y0;
    int i;

    icon[0] = (int)(g_pGraphics->resX * 100) / 640;
    icon[1] = 0;
    icon[2] = CFrontend::m_pAr640ATexture->width;
    icon[3] = CFrontend::m_pAr640ATexture->height;
    FrontendDraw_PlayTime();
    sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(100), FrontendControls_GetSelectedSlot() + 1);
    text[0] = CFrontend::m_stringDest;
    text[1] = CFrontend::GetTextString(pMenu->field_0x4);
    FrontendDraw_Breadcrumb((int)(g_pGraphics->resX * 24) / 640, (int)(g_pGraphics->resY * 38) / 480, text, 2);
    y0 = (int)(g_pGraphics->resY * 75) / 480;
    if (pMenu->cursor == 0) {
        pColour = g_colourWhite0x00524968;
        pShadow = g_colourShadowWhite0x00524974;
    } else {
        pColour = g_colourText0x0052496c;
        pShadow = g_colourShadowText0x00524978;
    }
    g_controlsLine[0] = (int)(g_pGraphics->resX * 99) / 640;
    g_controlsLine[1] = y0;
    g_controlsLine[2] = (int)(g_pGraphics->resX * 282) / 640;
    g_controlsLine[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_controlsLine, pShadow, 1);
    g_controlsLine[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_controlsLine, pColour, 1);
    maxWidth = 0;
    for (i = 0; i < pMenu->itemCount; i++) {
        sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x77), i);
        width = Font_GetTextWidth(1, (BYTE *)CFrontend::m_stringDest);
        if (width > maxWidth)
            maxWidth = width;
    }
    maxWidth += (int)(g_pGraphics->resX * 5) / 640;
    for (i = 0; i < pMenu->itemCount; i++) {
        icon[1] = (int)(g_pGraphics->resY * 2) / 480 + (int)(g_pGraphics->resY * 18) / 480 + y0
                  + ((short)((int)(g_pGraphics->resY * 36) / 480) * (short)i - CFrontend::m_pAr640ATexture->height / 2);
        if (pMenu->cursor == i) {
            pColour = g_colourWhite0x00524968;
            Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640ATexture->field_0x11c, (SpriteRect *)icon, CFrontend::m_pAr640ATexture, 1, 0, NULL, NULL, pColour, 8);
        } else {
            pColour = g_colourText0x0052496c;
            if (!pMenu->items[i].enabled)
                pColour = g_colourDim0x00524970;
            Sprite_Queue((SpriteRect *)&CFrontend::m_pAr640DTexture->field_0x11c, (SpriteRect *)icon, CFrontend::m_pAr640DTexture, 1, 0, NULL, NULL, pColour, 8);
        }
        if (i < pMenu->itemCount - 1) {
            sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(0x77), i);
            Font_DrawText(1, CFrontend::m_stringDest, (int)(g_pGraphics->resX * 0x7a) / 640,
                          (short)((int)(g_pGraphics->resY * 24) / 480 + g_controlsLine[1]), (int *)pColour, 0x11);
            FrontendControls_DrawCalibrationAxisRow((int)(g_pGraphics->resX * 0x7a) / 640 + maxWidth + ((int)(g_pGraphics->resX * 200) / 640) / 2,
                         (int)(g_pGraphics->resY * 10) / 480 + icon[1], pMenu, i);
        } else {
            Font_DrawText(1, CFrontend::GetTextString(pMenu->items[i].id), (int)(g_pGraphics->resX * 0x7a) / 640,
                          (short)((int)(g_pGraphics->resY * 24) / 480 + g_controlsLine[1]), (int *)pColour, 0x11);
        }
        if (pMenu->cursor == i + 1 || pMenu->cursor == i) {
            pColour = g_colourWhite0x00524968;
            pShadow = g_colourShadowWhite0x00524974;
        } else {
            pColour = g_colourText0x0052496c;
            pShadow = g_colourShadowText0x00524978;
        }
        g_controlsLine[1] = (short)((int)(g_pGraphics->resY * 36) / 480) * ((short)i + 1) + y0;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_controlsLine, pShadow, 1);
        g_controlsLine[1]++;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_controlsLine, pColour, 1);
    }
    if (FrontendControls_GetCalibrationState() != 0) {
        // PORT: the original's help named only keys (0x207-0x209).
        Font_DrawText(1, FrontendControls_Text(0), (int)(g_pGraphics->resX * 24) / 640,
                      (int)(g_pGraphics->resY * 420) / 480, (int *)g_colourText0x0052496c, 9);
        Font_DrawText(1, FrontendControls_Text(1), (int)(g_pGraphics->resX * 24) / 640,
                      (int)(g_pGraphics->resY * 440) / 480, (int *)g_colourText0x0052496c, 9);
        Font_DrawText(1, FrontendControls_Text(2), (int)(g_pGraphics->resX * 24) / 640,
                      (int)(g_pGraphics->resY * 460) / 480, (int *)g_colourText0x0052496c, 9);
    } else {
        Font_DrawText(1, CFrontend::GetTextString(0x20a), (int)(g_pGraphics->resX * 24) / 640,
                      (int)(g_pGraphics->resY * 420) / 480, (int *)g_colourText0x0052496c, 9);
    }
}

// Draws row i of a settings page: picks the icon/label, the active and the
// inactive choice colours (disabled rows are all dim) and queues the row icon.
#define CONTROLS_ROW_DRAW(pMenu, i, pLabel, pBright, pDim, pTexture)                   \
    if ((pMenu)->cursor == (i)) {                                                      \
        pLabel = g_colourWhite0x00524968;                                              \
        pBright = g_colourWhite0x00524968;                                             \
        pDim = g_colourText0x0052496c;                                                 \
        pTexture = CFrontend::m_pAr640ATexture;                                        \
        Sprite_Queue((SpriteRect *)&pTexture->field_0x11c, (SpriteRect *)icon,         \
                     pTexture, 1, 0, NULL, NULL, pLabel, 8);                           \
    } else {                                                                           \
        if ((pMenu)->items[i].enabled) {                                               \
            pLabel = g_colourText0x0052496c;                                           \
            pBright = g_colourWhite0x00524968;                                         \
            pDim = g_colourText0x0052496c;                                             \
        } else {                                                                       \
            pLabel = g_colourDim0x00524970;                                            \
            pBright = g_colourDim0x00524970;                                           \
            pDim = g_colourDim0x00524970;                                              \
        }                                                                              \
        pTexture = CFrontend::m_pAr640DTexture;                                        \
        Sprite_Queue((SpriteRect *)&pTexture->field_0x11c, (SpriteRect *)icon,         \
                     pTexture, 1, 0, NULL, NULL, pLabel, 8);                           \
    }

// Draw callback of the pad page: the two sensitivities (%), the vibration
// on/off choice and "back", vertically centred.
// match 48%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004fdb10
void FrontendControls_DrawPadSettings(Menu *pMenu)
{
    short icon[4];
    char *text[2];
    BYTE *pLabel;
    BYTE *pBright;
    BYTE *pDim;
    BYTE *pShadow;
    BYTE *pColour;
    Texture *pTexture;
    MenuItem *pItem;
    short y0;
    int x;
    int i;

    icon[0] = (int)(g_pGraphics->resX * 100) / 640;
    icon[1] = 0;
    icon[2] = CFrontend::m_pAr640ATexture->width;
    icon[3] = CFrontend::m_pAr640ATexture->height;
    FrontendDraw_PlayTime();
    sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(100), FrontendControls_GetSelectedSlot() + 1);
    text[0] = CFrontend::m_stringDest;
    text[1] = CFrontend::GetTextString(pMenu->field_0x4);
    FrontendDraw_Breadcrumb((int)(g_pGraphics->resX * 24) / 640, (int)(g_pGraphics->resY * 38) / 480, text, 2);
    y0 = (short)(((int)(g_pGraphics->resY * 8) / 480 + (int)(g_pGraphics->resY * 38) / 480 + (int)(g_pGraphics->resY * 384) / 480) / 2)
         - (short)((int)(g_pGraphics->resY * 36) / 480 * pMenu->itemCount / 2);
    if (pMenu->cursor == 0) {
        pColour = g_colourWhite0x00524968;
        pShadow = g_colourShadowWhite0x00524974;
    } else {
        pColour = g_colourText0x0052496c;
        pShadow = g_colourShadowText0x00524978;
    }
    g_controlsLine[0] = (int)(g_pGraphics->resX * 99) / 640;
    g_controlsLine[1] = y0;
    g_controlsLine[2] = (int)(g_pGraphics->resX * 282) / 640;
    g_controlsLine[3] = 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_controlsLine, pShadow, 1);
    g_controlsLine[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, g_controlsLine, pColour, 1);
    for (i = 0; i < pMenu->itemCount; i++) {
        pItem = &pMenu->items[i];
        icon[1] = (int)(g_pGraphics->resY * 2) / 480 + (int)(g_pGraphics->resY * 18) / 480 + y0
                  + ((short)((int)(g_pGraphics->resY * 36) / 480) * (short)i - CFrontend::m_pAr640ATexture->height / 2);
        CONTROLS_ROW_DRAW(pMenu, i, pLabel, pBright, pDim, pTexture)
        switch (pItem->value) {
        case 0:
        case 1:
            sprintf(CFrontend::m_stringDest, g_strPercentFormat, CFrontend::GetTextString(pItem->id), pItem->max * 10);
            Font_DrawText(1, CFrontend::m_stringDest, FM_SX(0x7a), FM_SY(24) + g_controlsLine[1], (int *)pLabel, 0x11);
            break;
        case 2:
            Font_DrawText(1, CFrontend::GetTextString(pItem->id), FM_SX(0x7a), FM_SY(24) + g_controlsLine[1], (int *)pLabel, 0x11);
            x = Font_GetTextWidth(1, (BYTE *)CFrontend::GetTextString(pItem->id)) + FM_SX(10) + FM_SX(0x7a);
            if (pItem->max == 0) {
                Font_DrawText(1, CFrontend::GetTextString(0x134), x, FM_SY(24) + g_controlsLine[1], (int *)pBright, 0x11);
                x = FM_SX(10) + x + Font_GetTextWidth(1, (BYTE *)CFrontend::GetTextString(0x134));
                Font_DrawText(1, CFrontend::GetTextString(0x133), x, FM_SY(24) + g_controlsLine[1], (int *)pDim, 0x11);
            } else {
                Font_DrawText(1, CFrontend::GetTextString(0x134), x, FM_SY(24) + g_controlsLine[1], (int *)pDim, 0x11);
                x = FM_SX(10) + x + Font_GetTextWidth(1, (BYTE *)CFrontend::GetTextString(0x134));
                Font_DrawText(1, CFrontend::GetTextString(0x133), x, FM_SY(24) + g_controlsLine[1], (int *)pBright, 0x11);
            }
            break;
        case 3:
            Font_DrawText(1, CFrontend::GetTextString(pItem->id), FM_SX(0x7a), FM_SY(24) + g_controlsLine[1], (int *)pLabel, 0x11);
            break;
        }
        if (pMenu->cursor == i + 1 || pMenu->cursor == i) {
            pColour = g_colourWhite0x00524968;
            pShadow = g_colourShadowWhite0x00524974;
        } else {
            pColour = g_colourText0x0052496c;
            pShadow = g_colourShadowText0x00524978;
        }
        g_controlsLine[1] = (short)((int)(g_pGraphics->resY * 36) / 480) * ((short)i + 1) + y0;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_controlsLine, pShadow, 1);
        g_controlsLine[1]++;
        Sprite_FillRect(&g_pGraphics->field309_0x150, g_controlsLine, pColour, 1);
    }
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

// Draw callback of the device settings page: the configuration chosen for
// the slot and its on/off options.
// match 33%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004fe240
void FrontendControls_DrawDeviceConfiguration(Menu *pMenu)
{
    short icon[4];
    short line[4];
    char *text[2];
    BYTE *pLabel;
    BYTE *pBright;
    BYTE *pDim;
    BYTE *pShadow;
    BYTE *pColour;
    Texture *pTexture;
    MenuItem *pItem;
    short y0;
    int x;
    int i;

    icon[1] = 0;
    icon[0] = (int)(g_pGraphics->resX * 100) / 640;
    icon[2] = CFrontend::m_pAr640ATexture->width;
    icon[3] = CFrontend::m_pAr640ATexture->height;
    FrontendDraw_PlayTime();
    sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(100), FrontendControls_GetSelectedSlot() + 1);
    text[0] = CFrontend::m_stringDest;
    text[1] = CFrontend::GetTextString(pMenu->field_0x4);
    FrontendDraw_Breadcrumb((int)(g_pGraphics->resX * 24) / 640, (int)(g_pGraphics->resY * 38) / 480, text, 2);
    y0 = (int)(g_pGraphics->resY * 100) / 480;
    line[0] = (int)(g_pGraphics->resX * 99) / 640;
    line[3] = 1;
    line[2] = (int)(g_pGraphics->resX * 282) / 640;
    if (pMenu->cursor == 0) {
        pColour = g_colourWhite0x00524968;
        pShadow = g_colourShadowWhite0x00524974;
    } else if (!pMenu->items[0].enabled) {
        pColour = g_colourDim0x00524970;
        pShadow = g_colourShadowDim0x0052497c;
    } else {
        pColour = g_colourText0x0052496c;
        pShadow = g_colourShadowText0x00524978;
    }
    line[1] = y0;
    Sprite_FillRect(&g_pGraphics->field309_0x150, line, pShadow, 1);
    line[1]++;
    Sprite_FillRect(&g_pGraphics->field309_0x150, line, pColour, 1);
    for (i = 0; i < pMenu->itemCount; i++) {
        pItem = &pMenu->items[i];
        icon[1] = (int)(g_pGraphics->resY * 20) / 480 + y0 + (int)(g_pGraphics->resY * 36) / 480 * (short)i
                  - CFrontend::m_pAr640ATexture->height / 2;
        CONTROLS_ROW_DRAW(pMenu, i, pLabel, pBright, pDim, pTexture)
        switch (pItem->value) {
        case 0:
            sprintf(CFrontend::m_stringDest, g_strAngleFormat, FrontendControls_GetSelectedDeviceName());
            Font_DrawText(1, CFrontend::m_stringDest, FM_SX(0x7a), FM_SY(24) + line[1], (int *)pLabel, 0x11);
            break;
        case 1:
            x = FM_SX(0x7a);
            FM_LABEL(CFrontend::GetTextString(0x197));
            FM_CHOICE(0x69, 0x68);
            break;
        case 2:
            sprintf(CFrontend::m_stringDest, g_strSlashFormat, CFrontend::GetTextString(0x6d), CFrontend::GetTextString(0x6e));
            x = FM_SX(0x7a);
            FM_LABEL(CFrontend::m_stringDest);
            FM_CHOICE(0x69, 0x68);
            break;
        case 3:
            x = FM_SX(0x7a);
            FM_LABEL(CFrontend::GetTextString(pItem->id));
            FM_CHOICE(0x134, 0x133);
            break;
        case 4:
            x = FM_SX(0x7a);
            FM_LABEL(CFrontend::GetTextString(pItem->id));
            FM_CHOICE(5, 4);
            break;
        case 5:
            x = FM_SX(0x7a);
            FM_LABEL(CFrontend::GetTextString(pItem->id));
            FM_CHOICE(5, 4);
            break;
        case 6:
            sprintf(CFrontend::m_stringDest, CFrontend::GetTextString(pItem->id));
            Font_DrawText(1, CFrontend::m_stringDest, FM_SX(0x7a), FM_SY(24) + line[1], (int *)pLabel, 0x11);
            break;
        }
        if (pMenu->cursor == i || pMenu->cursor == i + 1) {
            pColour = g_colourWhite0x00524968;
            pShadow = g_colourShadowWhite0x00524974;
        } else {
            pColour = g_colourText0x0052496c;
            pShadow = g_colourShadowText0x00524978;
        }
        line[1] = (int)(g_pGraphics->resY * 36) / 480 * ((short)i + 1) + y0;
        Sprite_FillRect(&g_pGraphics->field309_0x150, line, pShadow, 1);
        line[1]++;
        Sprite_FillRect(&g_pGraphics->field309_0x150, line, pColour, 1);
    }
    FrontendDraw_Carousel(FrontendMenu_GetMain(), 0, NULL);
}

int FrontendControls_MultiplyScaledPercent(int a, int b);

// Draws the calibration bar of an axis centred on (x, y): the bar, the
// deadzone and saturation marks and the current position.
// match 62%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004ff0f0
void FrontendControls_DrawAxisCalibrationBar(short x, short y, DWORD colour, JoystickBinding *pAxis)
{
    short deadzone[4];
    short saturation[4];
    short bar[4];
    short position[4];
    short half;
    int v;

    bar[0] = x - (short)((int)(g_pGraphics->resX * 200) / 640 / 2);
    bar[1] = y - (short)((int)(g_pGraphics->resY * 10) / 480 / 2);
    bar[2] = (int)(g_pGraphics->resX * 200) / 640;
    bar[3] = (int)(g_pGraphics->resY * 10) / 480;
    Sprite_FillRect(&g_pGraphics->field309_0x150, bar, (BYTE *)&colour, 1);
    if (pAxis != NULL) {
        half = (short)(FrontendControls_MultiplyScaledPercent(pAxis->deadzone, (int)(g_pGraphics->resX * 200) / 640) / 2);
        deadzone[0] = x - half;
        deadzone[2] = 2;
        deadzone[1] = y - (short)((int)(g_pGraphics->resY * 10) / 480 / 2) - 2;
        deadzone[3] = (int)(g_pGraphics->resY * 10) / 480 + 4;
        Sprite_FillRect(&g_pGraphics->field309_0x150, deadzone, g_colourWhite0x00524968, 1);
        v = FrontendControls_MultiplyScaledPercent(pAxis->saturation, (int)(g_pGraphics->resX * 200) / 640);
        saturation[2] = deadzone[2];
        saturation[1] = deadzone[1];
        saturation[0] = x - (short)(v / 2);
        saturation[3] = deadzone[3];
        deadzone[0] = half + x;
        position[3] = deadzone[3];
        position[0] = (short)((int)(g_pGraphics->resX * 200) / 640 * pAxis->position / 2 / 0x10000) + x;
        position[1] = deadzone[1];
        position[2] = deadzone[2];
        Sprite_FillRect(&g_pGraphics->field309_0x150, deadzone, g_colourWhite0x00524968, 1);
        Sprite_FillRect(&g_pGraphics->field309_0x150, saturation, g_colourWhite0x00524968, 1);
        saturation[0] = (short)(v / 2) + x;
        Sprite_FillRect(&g_pGraphics->field309_0x150, saturation, g_colourWhite0x00524968, 1);
        Sprite_FillRect(&g_pGraphics->field309_0x150, position, g_colourWhite0x00524968, 1);
    }
}

// Draws row `index` of the calibration page: the axis bar in white when
// selected (red while calibrating), dim when the entry is hidden.
// FUNCTION: CMR2 0x004ff060
void FrontendControls_DrawCalibrationAxisRow(short x, int y, Menu *pMenu, int index)
{
    JoystickBinding *pAxis;
    BYTE red[4];
    red[1] = 0x14;
    red[2] = 0x14;
    red[0] = 0xf0;
    red[3] = 0xff;
    if (pMenu->items[index].visible) {
        pAxis = FrontendControls_GetSelectedAxisPosition(index);
        if (pMenu->cursor == index) {
            if (FrontendControls_GetCalibrationState() != 0)
                FrontendControls_DrawAxisCalibrationBar(x, y, *(DWORD *)red, pAxis);
            else
                FrontendControls_DrawAxisCalibrationBar(x, y, *(DWORD *)g_colourWhite0x00524968, pAxis);
        } else if (pMenu->items[index].enabled) {
            FrontendControls_DrawAxisCalibrationBar(x, y, *(DWORD *)g_colourText0x0052496c, pAxis);
        }
    } else {
        FrontendControls_DrawAxisCalibrationBar(x, y, *(DWORD *)g_colourDim0x00524970, NULL);
    }
}

// FUNCTION: CMR2 0x004ff420
int FrontendControls_MultiplyScaledPercent(int a, int b)
{
    return (a * b) / 10000;
}

// GLOBAL: CMR2 0x0081b158
Menu g_menu0x0081b158;
// GLOBAL: CMR2 0x0081b338
Menu g_menu0x0081b338;
// GLOBAL: CMR2 0x0081b518
Menu g_menu0x0081b518;
// Cheats menu.
// GLOBAL: CMR2 0x0081be78
Menu g_menu0x0081be78;
// GLOBAL: CMR2 0x0081bab8
Menu g_menu0x0081bab8;
// GLOBAL: CMR2 0x0081bc98
Menu g_menu0x0081bc98;
// GLOBAL: CMR2 0x0081c058
Menu g_menu0x0081c058;
// GLOBAL: CMR2 0x0081c418
Menu g_menu0x0081c418;
// GLOBAL: CMR2 0x0081c5f8
Menu g_menu0x0081c5f8;
// GLOBAL: CMR2 0x0081c7d8
Menu g_menu0x0081c7d8;
// GLOBAL: CMR2 0x0081cb98
Menu g_menu0x0081cb98;
// GLOBAL: CMR2 0x0081cd78
Menu g_menu0x0081cd78;
// GLOBAL: CMR2 0x0081cf58
Menu g_menu0x0081cf58;
// GLOBAL: CMR2 0x0081d138
Menu g_menu0x0081d138;
// GLOBAL: CMR2 0x0081d318
Menu g_menu0x0081d318;
// GLOBAL: CMR2 0x0081d4f8
Menu g_menu0x0081d4f8;
// GLOBAL: CMR2 0x0081d6d8
Menu g_menu0x0081d6d8;
// GLOBAL: CMR2 0x0081d8b8
Menu g_menu0x0081d8b8;
// GLOBAL: CMR2 0x0081dc78
Menu g_menu0x0081dc78;
// GLOBAL: CMR2 0x0081de58
Menu g_menu0x0081de58;
// GLOBAL: CMR2 0x0081e218
Menu g_menu0x0081e218;
// GLOBAL: CMR2 0x0081e3f8
Menu g_menu0x0081e3f8;
// GLOBAL: CMR2 0x0081e5d8
Menu g_menu0x0081e5d8;
// GLOBAL: CMR2 0x0081e998
Menu g_menu0x0081e998;
// GLOBAL: CMR2 0x0081eb78
Menu g_menu0x0081eb78;
// GLOBAL: CMR2 0x0081ed58
Menu g_menu0x0081ed58;
// GLOBAL: CMR2 0x0081ef38
Menu g_menu0x0081ef38;
// GLOBAL: CMR2 0x0081f118
Menu g_menu0x0081f118;
// GLOBAL: CMR2 0x0081f2f8
Menu g_menu0x0081f2f8;
// GLOBAL: CMR2 0x0081f4d8
Menu g_menu0x0081f4d8;
// GLOBAL: CMR2 0x0081f6b8
Menu g_menu0x0081f6b8;
// GLOBAL: CMR2 0x0081fa78
Menu g_menu0x0081fa78;
// GLOBAL: CMR2 0x0081fc58
Menu g_menu0x0081fc58;
// GLOBAL: CMR2 0x0081fe38
Menu g_menu0x0081fe38;
// GLOBAL: CMR2 0x00820018
Menu g_menu0x00820018;
// GLOBAL: CMR2 0x008201f8
Menu g_menu0x008201f8;
// GLOBAL: CMR2 0x008203d8
Menu g_menu0x008203d8;
// GLOBAL: CMR2 0x008205b8
Menu g_menu0x008205b8;
// GLOBAL: CMR2 0x00820798
Menu g_menu0x00820798;
// GLOBAL: CMR2 0x00820978
Menu g_menu0x00820978;
// GLOBAL: CMR2 0x00820b58
Menu g_menu0x00820b58;
// GLOBAL: CMR2 0x00820d38
Menu g_menu0x00820d38;
// GLOBAL: CMR2 0x008210f8
Menu g_menu0x008210f8;
// GLOBAL: CMR2 0x008212d8
Menu g_menu0x008212d8;
// GLOBAL: CMR2 0x008214b8
Menu g_menu0x008214b8;
// GLOBAL: CMR2 0x00821878
Menu g_menu0x00821878;
// GLOBAL: CMR2 0x00821c38
Menu g_menu0x00821c38;
// GLOBAL: CMR2 0x00821e18
Menu g_menu0x00821e18;
// GLOBAL: CMR2 0x008221d8
Menu g_menu0x008221d8;
// GLOBAL: CMR2 0x008223b8
Menu g_menu0x008223b8;
// GLOBAL: CMR2 0x00822598
Menu g_menu0x00822598;
// GLOBAL: CMR2 0x00822958
Menu g_menu0x00822958;
// GLOBAL: CMR2 0x00822d18
Menu g_menu0x00822d18;
// GLOBAL: CMR2 0x00822ef8
Menu g_menu0x00822ef8;
// GLOBAL: CMR2 0x008230d8
Menu g_menu0x008230d8;
// GLOBAL: CMR2 0x008232b8
Menu g_menu0x008232b8;
// GLOBAL: CMR2 0x00823498
Menu g_menu0x00823498;
// GLOBAL: CMR2 0x00823678
Menu g_menu0x00823678;
// GLOBAL: CMR2 0x00823858
Menu g_menu0x00823858;
// GLOBAL: CMR2 0x00823a38
Menu g_menu0x00823a38;
// GLOBAL: CMR2 0x00823c18
Menu g_menu0x00823c18;
// GLOBAL: CMR2 0x00823df8
Menu g_menu0x00823df8;
// GLOBAL: CMR2 0x00823fd8
Menu g_menu0x00823fd8;
// GLOBAL: CMR2 0x008241b8
Menu g_menu0x008241b8;
// GLOBAL: CMR2 0x00824498
Menu g_menu0x00824498;
// GLOBAL: CMR2 0x00824678
Menu g_menu0x00824678;
// GLOBAL: CMR2 0x00824858
Menu g_menu0x00824858;
// GLOBAL: CMR2 0x00824a38
Menu g_menu0x00824a38;
// GLOBAL: CMR2 0x00824c18
Menu g_menu0x00824c18;
// GLOBAL: CMR2 0x00824df8
Menu g_menu0x00824df8;
// GLOBAL: CMR2 0x00824fd8
Menu g_menu0x00824fd8;
// GLOBAL: CMR2 0x008251b8
Menu g_menu0x008251b8;
// GLOBAL: CMR2 0x00826140
Menu g_menu0x00826140;
// GLOBAL: CMR2 0x00826420
Menu g_menu0x00826420;
// GLOBAL: CMR2 0x00826600
Menu g_menu0x00826600;
// GLOBAL: CMR2 0x008267e0
Menu g_menu0x008267e0;
// GLOBAL: CMR2 0x008269c0
Menu g_menu0x008269c0;
// GLOBAL: CMR2 0x00826ba0
Menu g_menu0x00826ba0;
// GLOBAL: CMR2 0x00826d80
Menu g_menu0x00826d80;
// GLOBAL: CMR2 0x00826f60
Menu g_menu0x00826f60;
// GLOBAL: CMR2 0x00827140
Menu g_menu0x00827140;
// GLOBAL: CMR2 0x00827320
Menu g_menu0x00827320;
// GLOBAL: CMR2 0x00827500
Menu g_menu0x00827500;
// GLOBAL: CMR2 0x008276e0
Menu g_menu0x008276e0;
// GLOBAL: CMR2 0x008278c0
Menu g_menu0x008278c0;
// GLOBAL: CMR2 0x00827aa0
Menu g_menu0x00827aa0;
// GLOBAL: CMR2 0x00827c80
Menu g_menu0x00827c80;
// GLOBAL: CMR2 0x00827e60
Menu g_menu0x00827e60;
// GLOBAL: CMR2 0x00828040
Menu g_menu0x00828040;
// GLOBAL: CMR2 0x00828220
Menu g_menu0x00828220;
// GLOBAL: CMR2 0x00828500
Menu g_menu0x00828500;
// GLOBAL: CMR2 0x008286e0
Menu g_menu0x008286e0;
// GLOBAL: CMR2 0x008288c0
Menu g_menu0x008288c0;
// GLOBAL: CMR2 0x00828aa0
Menu g_menu0x00828aa0;
// GLOBAL: CMR2 0x00828c80
Menu g_menu0x00828c80;
// GLOBAL: CMR2 0x00828e60
Menu g_menu0x00828e60;
// GLOBAL: CMR2 0x00829140
Menu g_menu0x00829140;

// FUNCTION: CMR2 0x004f5520
void FrontendMenu_BuildNetworkWaitingRoom(void)
{
    Menu_Init(&g_menu0x008205b8, 0, 1, 0, &g_menu0x0081d6d8, NULL, 1, 0, 0);
    Menu_AddItemType1(&g_menu0x008205b8, 0, 0x1b, 0, 0);
    Menu_SetCallbacks(&g_menu0x008205b8, NULL, NULL, (MenuCallback)FrontendMenu_DrawNetworkWaitingRoom, NULL);
    Menu_ValidateCursor(&g_menu0x008205b8, 0);
}

void FrontendMenu_EnterLanguage(Menu *pMenu, int param);
void FrontendMenu_UpdateLanguage(Menu *pMenu);
void FrontendMenu_DrawLanguage(Menu *pMenu);
void FrontendMenu_LeaveLanguage(Menu *pMenu, char back);
void FrontendMenu_EnterMain(Menu *pMenu, int param);
void FrontendMenu_UpdateMain(Menu *pMenu);
void FrontendMenu_DrawMain(Menu *pMenu);
void FrontendMenu_LeaveMain(Menu *pMenu, int param);
void FrontendChampionship_StartNewEntry(Menu *pMenu, int param);
char GameInfo_AreOptionsAvailable(void);

// Language menu: five languages, or three in region 1.
// FUNCTION: CMR2 0x004f5580
// PORT: the entry is the language number (English, French, Spanish, Italian,
// German: text files, speech, credits and flag). The original labelled
// entries 2 and 4 with texts 8 "deutsch" and 10 "español", swapped: choosing
// "deutsch" showed the Spanish flag and loaded Spanish. Fixed here.
void FrontendMenu_BuildLanguage(void)
{
    Menu_Init(&g_menu0x008221d8, 0, 1, 0, NULL, NULL, 1, 0, 0);
    if (CGameInfo::GetGameRegion() == 0) {
        Menu_AddItemType2(&g_menu0x008221d8, 0, 6, &g_menu0x0081d6d8, 0, 0);
        Menu_AddItemType2(&g_menu0x008221d8, 0, 7, &g_menu0x0081d6d8, 0, 1);
        Menu_AddItemType2(&g_menu0x008221d8, 0, 10, &g_menu0x0081d6d8, 0, 2);
        Menu_AddItemType2(&g_menu0x008221d8, 0, 9, &g_menu0x0081d6d8, 0, 3);
        Menu_AddItemType2(&g_menu0x008221d8, 0, 8, &g_menu0x0081d6d8, 0, 4);
    } else {
        Menu_AddItemType2(&g_menu0x008221d8, 0, 6, &g_menu0x0081d6d8, 0, 0);
        Menu_AddItemType2(&g_menu0x008221d8, 0, 7, &g_menu0x0081d6d8, 0, 1);
        Menu_AddItemType2(&g_menu0x008221d8, 0, 8, &g_menu0x0081d6d8, 0, 2);
    }
    Menu_SetCallbacks(&g_menu0x008221d8, (MenuCallback)FrontendMenu_EnterLanguage, (MenuCallback)FrontendMenu_UpdateLanguage,
                      (MenuCallback)FrontendMenu_DrawLanguage, (MenuCallback)FrontendMenu_LeaveLanguage);
    Menu_ValidateCursor(&g_menu0x008221d8, 0);
}

// Main menu: single rally, championship, time trial, multiplayer, options,
// (extras when unlocked) and quit.
// FUNCTION: CMR2 0x004f5670
void FrontendMenu_BuildMain(void)
{
    Menu_Init(&g_menu0x0081d6d8, 0, 0, 0, &g_menu0x00823df8, NULL, 1, 1, 0);
    Menu_AddItemType2(&g_menu0x0081d6d8, 0, 0x4f, &g_menu0x008212d8, 0, 0);
    Menu_AddItemType2(&g_menu0x0081d6d8, 0, 0x50, &g_menu0x00823a38, 0, 1);
    Menu_AddItemType2(&g_menu0x0081d6d8, 0, 0x51, FrontendMenu_GetArcade(), 0, 2);
    Menu_AddItemType2(&g_menu0x0081d6d8, 0, 0x52, &g_menu0x008241b8, (MenuItemAction)FrontendChampionship_StartNewEntry, 3);
    Menu_AddItemType2(&g_menu0x0081d6d8, 0, 0x53, &g_menu0x0081f118, 0, 4);
    if (GameInfo_AreOptionsAvailable() != 0)
        Menu_AddItemType2(&g_menu0x0081d6d8, 0, 0x54, &g_menu0x00822958, 0, 5);
    Menu_AddItemType2(&g_menu0x0081d6d8, 0, 0x56, &g_menu0x00823df8, 0, 6);
    Menu_SetCallbacks(&g_menu0x0081d6d8, (MenuCallback)FrontendMenu_EnterMain, (MenuCallback)FrontendMenu_UpdateMain,
                      (MenuCallback)FrontendMenu_DrawMain, (MenuCallback)FrontendMenu_LeaveMain);
    Menu_ValidateCursor(&g_menu0x0081d6d8, 0);
}

void FrontendMenu_SetNextParent(Menu *pMenu, int param);
void FrontendProfile_CreateNewProfile(Menu *pMenu, int param);
void FrontendProfile_LoadSelectedProfile(Menu *pMenu, int param);
void FrontendMenu_EnterRallyProfile(Menu *pMenu, char back);
void FrontendScroller_UpdateMain(Menu *pMenu);
void FrontendMenu_DrawRallyProfile(Menu *pMenu);
void FrontendMenu_LeavePlayerProfile(Menu *pMenu, char back);

// Profile menu of a rally: continue, new profile and up to 4 saved ones.
// FUNCTION: CMR2 0x004f5770
void FrontendMenu_BuildRallyProfile(void)
{
    int i;

    Menu_Init(&g_menu0x008212d8, 0, 0xb, 0, &g_menu0x0081d6d8, NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x008212d8, 0, 0x102, &g_menu0x008203d8, (MenuItemAction)FrontendMenu_SetNextParent, -1);
    Menu_AddItemType4(&g_menu0x008212d8, 0, 0xe6, (MenuItemAction)FrontendProfile_CreateNewProfile, -1);
    i = 4;
    do {
        Menu_AddItemType4(&g_menu0x008212d8, 0, -1, (MenuItemAction)FrontendProfile_LoadSelectedProfile, -1);
        i--;
    } while (i != 0);
    Menu_SetCallbacks(&g_menu0x008212d8, (MenuCallback)FrontendMenu_EnterRallyProfile, (MenuCallback)FrontendScroller_UpdateMain,
                      (MenuCallback)FrontendMenu_DrawRallyProfile, (MenuCallback)FrontendMenu_LeavePlayerProfile);
    Menu_ValidateCursor(&g_menu0x008212d8, 0);
}

void FrontendMenu_EnterOptions(Menu *pMenu, int param);
void FrontendMenu_DrawOptions(Menu *pMenu);
void FrontendMenu_EnterCheats(Menu *pMenu, int param);
void FrontendMenu_DrawCheats(Menu *pMenu);
void FrontendMenu_LeaveCheats(Menu *pMenu, char back);
void FrontendMenu_ReturnToParent(Menu *pMenu, int param);

// Options menu: game, sound, graphics, controls, language (not in regions
// 2/3), cheats (id 1000) and back.
// FUNCTION: CMR2 0x004f5eb0
void FrontendMenu_BuildOptions(void)
{
    Menu_Init(&g_menu0x0081f118, 0, 0x13, 0, &g_menu0x0081d6d8, NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x0081f118, 0, 0x14, &g_menu0x0081ef38, 0, 0);
    Menu_AddItemType2(&g_menu0x0081f118, 0, 0x15, &g_menu0x0081fa78, 0, 0);
    Menu_AddItemType2(&g_menu0x0081f118, 0, 0x16, &g_menu0x00823c18, 0, 0);
    // PORT: the port's controls pages (FrontendScreens.cpp).
    Menu_AddItemType2(&g_menu0x0081f118, 0, 0x18, FrontendMenu_GetInputControls(), 0, 0);
    if (CGameInfo::GetGameRegion() != 3 && CGameInfo::GetGameRegion() != 2)
        Menu_AddItemType2(&g_menu0x0081f118, 0, 0x19, &g_menu0x008221d8, 0, 0);
    Menu_AddItemType2(&g_menu0x0081f118, 0, 0x148, &g_menu0x0081be78, 0, 1000);
    Menu_AddItemType1(&g_menu0x0081f118, 0, 0x67, 0, -1);
    Menu_SetCallbacks(&g_menu0x0081f118, (MenuCallback)FrontendMenu_EnterOptions, (MenuCallback)FrontendScroller_UpdateMain,
                      (MenuCallback)FrontendMenu_DrawOptions, NULL);
    Menu_ValidateCursor(&g_menu0x0081f118, 0);
    g_menu0x0081f118.items[6].enabled = 1;
    g_menu0x0081f118.items[6].visible = 1;
}

// Cheats menu: the 8 cheats as on/off entries.
// FUNCTION: CMR2 0x004f5fc0
void FrontendMenu_BuildCheats(void)
{
    int i;

    Menu_Init(&g_menu0x0081be78, 0, 0x148, 0, &g_menu0x0081f118, NULL, 1, 0, 1);
    i = 0;
    do {
        Menu_AddItemType3(&g_menu0x0081be78, 0, i + 0x149, 2, 0, 0, 0, (MenuItemAction)FrontendMenu_ReturnToParent, -1);
        i++;
    } while (i < 8);
    Menu_SetCallbacks(&g_menu0x0081be78, (MenuCallback)FrontendMenu_EnterCheats, NULL, (MenuCallback)FrontendMenu_DrawCheats,
                      (MenuCallback)FrontendMenu_LeaveCheats);
    Menu_ValidateCursor(&g_menu0x0081be78, 0);
}

void FrontendMenu_EnterDisplayDevice(Menu *pMenu, int param);
void FrontendMenu_SelectDisplayDevice(Menu *pMenu, int param);
void FrontendMenu_DrawDisplayDevice(Menu *pMenu);

// Display device menu: one entry per display device.
// FUNCTION: CMR2 0x004f63b0
void FrontendMenu_BuildDisplayDevice(void)
{
    unsigned int i;

    Menu_Init(&g_menu0x0081c7d8, 0, 0x14, 0, &g_menu0x008210f8, NULL, 1, 0, 1);
    i = 0;
    if ((unsigned int)CGraphics::GetDisplayDriverCount() > 0) {
        do {
            Menu_AddItemType4(&g_menu0x0081c7d8, 0, -1, (MenuItemAction)FrontendMenu_SelectDisplayDevice, 0);
            i++;
        } while (i < (unsigned int)CGraphics::GetDisplayDriverCount());
    }
    Menu_SetCallbacks(&g_menu0x0081c7d8, (MenuCallback)FrontendMenu_EnterDisplayDevice, NULL, (MenuCallback)FrontendMenu_DrawDisplayDevice, NULL);
    Menu_ValidateCursor(&g_menu0x0081c7d8, 0);
}

void FrontendRecords_CycleTableMode(Menu *pMenu, int param);
void FrontendMenu_DrawHighScores(Menu *pMenu);

// High score page (the table shown cycles on each press).
// FUNCTION: CMR2 0x004f66e0
void FrontendMenu_BuildHighScores(void)
{
    Menu_Init(&g_menu0x00820b58, 0, 0x162, 0, &g_menu0x00823c18, NULL, 1, 0, 0);
    Menu_AddItemType4(&g_menu0x00820b58, 0, -1, (MenuItemAction)FrontendRecords_CycleTableMode, -1);
    Menu_SetCallbacks(&g_menu0x00820b58, NULL, NULL, (MenuCallback)FrontendMenu_DrawHighScores, NULL);
    Menu_ValidateCursor(&g_menu0x00820b58, 0);
    g_menu0x00820b58.items[0].flag3 = 1;
}

void FrontendMenu_DrawProfileHighScores(Menu *pMenu);

// Record page (the table shown cycles on each press).
// FUNCTION: CMR2 0x004f69f0
void FrontendMenu_BuildProfileHighScores(void)
{
    Menu_Init(&g_menu0x00823498, 0, 0x162, 0, &g_menu0x00820d38, NULL, 1, 0, 0);
    Menu_AddItemType4(&g_menu0x00823498, 0, -1, (MenuItemAction)FrontendRecords_CycleTableMode, -1);
    Menu_SetCallbacks(&g_menu0x00823498, NULL, NULL, (MenuCallback)FrontendMenu_DrawProfileHighScores, NULL);
    Menu_ValidateCursor(&g_menu0x00823498, 0);
    g_menu0x00823498.items[0].flag3 = 1;
}

void FrontendScroller_InitBestStageTimes(Menu *pMenu, int param);
void FrontendScroller_UpdateBestStageTimes(Menu *pMenu);
void FrontendMenu_DrawProfileBestStageTimes(Menu *pMenu);

// Stage records page: one entry per rally.
// FUNCTION: CMR2 0x004f6b40
void FrontendMenu_BuildProfileBestStageTimes(void)
{
    Menu_Init(&g_menu0x00822ef8, 0, 0x164, 0, &g_menu0x00820d38, NULL, 1, 0, 0);
    Menu_AddItemType4(&g_menu0x00822ef8, 0, 0x27, 0, -1);
    Menu_AddItemType4(&g_menu0x00822ef8, 0, 0x28, 0, -1);
    Menu_AddItemType4(&g_menu0x00822ef8, 0, 0x29, 0, -1);
    Menu_AddItemType4(&g_menu0x00822ef8, 0, 0x2a, 0, -1);
    Menu_AddItemType4(&g_menu0x00822ef8, 0, 0x2b, 0, -1);
    Menu_AddItemType4(&g_menu0x00822ef8, 0, 0x2c, 0, -1);
    Menu_AddItemType4(&g_menu0x00822ef8, 0, 0x2d, 0, -1);
    Menu_AddItemType4(&g_menu0x00822ef8, 0, 0x2e, 0, -1);
    Menu_SetCallbacks(&g_menu0x00822ef8, (MenuCallback)FrontendScroller_InitBestStageTimes, (MenuCallback)FrontendScroller_UpdateBestStageTimes,
                      (MenuCallback)FrontendMenu_DrawProfileBestStageTimes, NULL);
    Menu_ValidateCursor(&g_menu0x00822ef8, 0);
    g_menu0x00822ef8.items[0].flag3 = 1;
    g_menu0x00822ef8.items[1].flag3 = 1;
    g_menu0x00822ef8.items[2].flag3 = 1;
    g_menu0x00822ef8.items[3].flag3 = 1;
    g_menu0x00822ef8.items[4].flag3 = 1;
    g_menu0x00822ef8.items[5].flag3 = 1;
    g_menu0x00822ef8.items[6].flag3 = 1;
    g_menu0x00822ef8.items[7].flag3 = 1;
}

void FrontendMenu_SelectDifficulty(Menu *pMenu, int param);
void FrontendScroller_InitDifficulty(Menu *pMenu, int param);
void FrontendMenu_EnterMultiplayerExtendedDifficulty(Menu *pMenu, int param);
void FrontendScroller_UpdateDifficulty(Menu *pMenu);
void FrontendMenu_DrawDifficulty(Menu *pMenu);

// Difficulty page (4 levels).
// FUNCTION: CMR2 0x004f6e50
void FrontendMenu_BuildSingleRallyDifficulty(char difficulty)
{
    Menu_Init(&g_menu0x0081d318, 0, 0x1c, 0, &g_menu0x0081d4f8, NULL, 1, 0, 0);
    Menu_AddItemType4(&g_menu0x0081d318, 0, 0xc5, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_AddItemType4(&g_menu0x0081d318, 0, 0xc6, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_AddItemType4(&g_menu0x0081d318, 0, 0xc7, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_AddItemType4(&g_menu0x0081d318, 0, 0xc8, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_SetCallbacks(&g_menu0x0081d318, (MenuCallback)FrontendScroller_InitDifficulty, (MenuCallback)FrontendScroller_UpdateDifficulty, (MenuCallback)FrontendMenu_DrawDifficulty, NULL);
    Menu_ValidateCursor(&g_menu0x0081d318, 0);
    g_menu0x0081d318.items[0].max = difficulty - 1;
}

// Difficulty page (4 levels).
// FUNCTION: CMR2 0x004f6f10
void FrontendMenu_BuildChampionshipDifficulty(char difficulty)
{
    Menu_Init(&g_menu0x00820978, 0, 0x1c, 0, &g_menu0x00824498, NULL, 1, 0, 0);
    Menu_AddItemType4(&g_menu0x00820978, 0, 0xc5, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_AddItemType4(&g_menu0x00820978, 0, 0xc6, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_AddItemType4(&g_menu0x00820978, 0, 0xc7, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_AddItemType4(&g_menu0x00820978, 0, 0xc8, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_SetCallbacks(&g_menu0x00820978, (MenuCallback)FrontendScroller_InitDifficulty, (MenuCallback)FrontendScroller_UpdateDifficulty, (MenuCallback)FrontendMenu_DrawDifficulty, NULL);
    Menu_ValidateCursor(&g_menu0x00820978, 0);
    g_menu0x00820978.items[0].max = difficulty - 1;
}

// Difficulty page (4 levels).
// FUNCTION: CMR2 0x004f6fd0
void FrontendMenu_BuildTimeTrialDifficulty(char difficulty)
{
    Menu_Init(&g_menu0x0081f2f8, 0, 0x1c, 0, &g_menu0x00823fd8, NULL, 1, 0, 0);
    Menu_AddItemType4(&g_menu0x0081f2f8, 0, 0xc5, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_AddItemType4(&g_menu0x0081f2f8, 0, 0xc6, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_AddItemType4(&g_menu0x0081f2f8, 0, 0xc7, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_AddItemType4(&g_menu0x0081f2f8, 0, 0xc8, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_SetCallbacks(&g_menu0x0081f2f8, (MenuCallback)FrontendScroller_InitDifficulty, (MenuCallback)FrontendScroller_UpdateDifficulty, (MenuCallback)FrontendMenu_DrawDifficulty, NULL);
    Menu_ValidateCursor(&g_menu0x0081f2f8, 0);
    g_menu0x0081f2f8.items[0].max = difficulty - 1;
}

// Difficulty page (4 levels).
// FUNCTION: CMR2 0x004f7090
void FrontendMenu_BuildMultiplayerDifficulty(char difficulty)
{
    Menu_Init(&g_menu0x00824678, 0, 0x1c, 0, &g_menu0x00823a38, NULL, 1, 0, 0);
    Menu_AddItemType4(&g_menu0x00824678, 0, 0xc5, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_AddItemType4(&g_menu0x00824678, 0, 0xc6, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_AddItemType4(&g_menu0x00824678, 0, 0xc7, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_AddItemType4(&g_menu0x00824678, 0, 0xc8, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_SetCallbacks(&g_menu0x00824678, (MenuCallback)FrontendScroller_InitDifficulty, (MenuCallback)FrontendScroller_UpdateDifficulty, (MenuCallback)FrontendMenu_DrawDifficulty, NULL);
    Menu_ValidateCursor(&g_menu0x00824678, 0);
    g_menu0x00824678.items[0].max = difficulty - 1;
}

// Difficulty page (8 levels).
// FUNCTION: CMR2 0x004f7150
void FrontendMenu_BuildMultiplayerExtendedDifficulty(char difficulty)
{
    Menu_Init(&g_menu0x0081e5d8, 0, 0x40, 0, &g_menu0x00823a38, NULL, 1, 0, 0);
    Menu_AddItemType4(&g_menu0x0081e5d8, 0, 0xc5, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_AddItemType4(&g_menu0x0081e5d8, 0, 0xc6, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_AddItemType4(&g_menu0x0081e5d8, 0, 0xc7, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_AddItemType4(&g_menu0x0081e5d8, 0, 0xc8, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_AddItemType4(&g_menu0x0081e5d8, 0, 0xc9, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_AddItemType4(&g_menu0x0081e5d8, 0, 0xca, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_AddItemType4(&g_menu0x0081e5d8, 0, 0xcb, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_AddItemType4(&g_menu0x0081e5d8, 0, 0xcc, (MenuItemAction)FrontendMenu_SelectDifficulty, -1);
    Menu_SetCallbacks(&g_menu0x0081e5d8, (MenuCallback)FrontendMenu_EnterMultiplayerExtendedDifficulty, (MenuCallback)FrontendScroller_UpdateDifficulty, (MenuCallback)FrontendMenu_DrawDifficulty, NULL);
    Menu_ValidateCursor(&g_menu0x0081e5d8, 0);
    g_menu0x0081e5d8.items[0].max = difficulty - 1;
}

void FrontendMenu_DrawTransmissionChoice(Menu *pMenu);
void FrontendMenu_UpdateTransmissionChoice(Menu *pMenu);
void FrontendMenu_SelectTransmission(Menu *pMenu, int param);

// Two-choice page after a difficulty page.
// FUNCTION: CMR2 0x004f7510
void FrontendMenu_BuildSingleRallyTransmission(char choice)
{
    Menu_Init(&g_menu0x008230d8, 0, 0x1e, 0, &g_menu0x0081d318, NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x008230d8, 0, 0xd3, &g_menu0x008241b8, (MenuItemAction)FrontendMenu_SelectTransmission, -1);
    Menu_AddItemType2(&g_menu0x008230d8, 0, 0xd4, &g_menu0x008241b8, (MenuItemAction)FrontendMenu_SelectTransmission, -1);
    Menu_SetCallbacks(&g_menu0x008230d8, NULL, (MenuCallback)FrontendMenu_UpdateTransmissionChoice, (MenuCallback)FrontendMenu_DrawTransmissionChoice, NULL);
    Menu_ValidateCursor(&g_menu0x008230d8, 0);
    g_menu0x008230d8.cursor = choice;
    g_menu0x008230d8.items[0].enabled = 1;
}

// Two-choice page after a difficulty page.
// FUNCTION: CMR2 0x004f75b0
void FrontendMenu_BuildChampionshipTransmission(char choice)
{
    Menu_Init(&g_menu0x00824858, 0, 0x1e, 0, &g_menu0x00820978, NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x00824858, 0, 0xd3, &g_menu0x008241b8, (MenuItemAction)FrontendMenu_SelectTransmission, -1);
    Menu_AddItemType2(&g_menu0x00824858, 0, 0xd4, &g_menu0x008241b8, (MenuItemAction)FrontendMenu_SelectTransmission, -1);
    Menu_SetCallbacks(&g_menu0x00824858, NULL, (MenuCallback)FrontendMenu_UpdateTransmissionChoice, (MenuCallback)FrontendMenu_DrawTransmissionChoice, NULL);
    Menu_ValidateCursor(&g_menu0x00824858, 0);
    g_menu0x00824858.cursor = choice;
    g_menu0x00824858.items[0].enabled = 1;
}

// Two-choice page after a difficulty page.
// FUNCTION: CMR2 0x004f7650
void FrontendMenu_BuildTimeTrialTransmission(char choice)
{
    Menu_Init(&g_menu0x008251b8, 0, 0x1e, 0, &g_menu0x0081f2f8, NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x008251b8, 0, 0xd3, &g_menu0x008241b8, (MenuItemAction)FrontendMenu_SelectTransmission, -1);
    Menu_AddItemType2(&g_menu0x008251b8, 0, 0xd4, &g_menu0x008241b8, (MenuItemAction)FrontendMenu_SelectTransmission, -1);
    Menu_SetCallbacks(&g_menu0x008251b8, NULL, (MenuCallback)FrontendMenu_UpdateTransmissionChoice, (MenuCallback)FrontendMenu_DrawTransmissionChoice, NULL);
    Menu_ValidateCursor(&g_menu0x008251b8, 0);
    g_menu0x008251b8.cursor = choice;
    g_menu0x008251b8.items[0].enabled = 1;
}

void FrontendMenu_EnterMultiplayerProfile(Menu *pMenu, char back);
void FrontendProfile_CreateNewPlayerProfile(Menu *pMenu, int param);
void FrontendProfile_SetupNextPlayer(Menu *pMenu, int param);
void FrontendProfile_SelectSavedPlayerProfile(Menu *pMenu, int param);
void FrontendMenu_DrawMultiplayerProfile(Menu *pMenu);

// Player profile menu of the multiplayer modes: continue, new profile,
// no profile and up to 4 saved ones.
// FUNCTION: CMR2 0x004f76f0
void FrontendMenu_BuildMultiplayerProfile(void)
{
    int i;

    Menu_Init(&g_menu0x008241b8, 0, 0x21, 0, NULL, NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x008241b8, 0, 0x102, &g_menu0x008203d8, (MenuItemAction)FrontendMenu_SetNextParent, -1);
    Menu_AddItemType4(&g_menu0x008241b8, 0, 0xe6, (MenuItemAction)FrontendProfile_CreateNewPlayerProfile, 0);
    Menu_AddItemType4(&g_menu0x008241b8, 0, 0xe5, (MenuItemAction)FrontendProfile_SetupNextPlayer, 0);
    i = 4;
    do {
        Menu_AddItemType4(&g_menu0x008241b8, 0, -1, (MenuItemAction)FrontendProfile_SelectSavedPlayerProfile, -1);
        i--;
    } while (i != 0);
    Menu_SetCallbacks(&g_menu0x008241b8, (MenuCallback)FrontendMenu_EnterMultiplayerProfile, NULL, (MenuCallback)FrontendMenu_DrawMultiplayerProfile,
                      (MenuCallback)FrontendMenu_LeavePlayerProfile);
    Menu_ValidateCursor(&g_menu0x008241b8, 0);
}

void FrontendMenu_EnterMultiplayerRaceSettings(Menu *pMenu, int param);
void FrontendMenu_StartMultiplayerKnockout(Menu *pMenu, int param);
void FrontendMenu_UpdateMultiplayerRaceSettings(Menu *pMenu);
void FrontendMenu_DrawMultiplayerRaceSettings(Menu *pMenu);

// Multiplayer race settings page: three settings and "start".
// FUNCTION: CMR2 0x004f8170
void FrontendMenu_BuildMultiplayerRaceSettings(void)
{
    Menu_Init(&g_menu0x00823858, 0, 0x46, 0, &g_menu0x00822d18, NULL, 1, 3, 1);
    Menu_AddItemType3(&g_menu0x00823858, 0, -1, 4, 0, 0, 0, 0, 0);
    Menu_AddItemType3(&g_menu0x00823858, 0, -1, 4, 0, 0, 0, 0, 1);
    Menu_AddItemType3(&g_menu0x00823858, 0, -1, 5, 0, 0, 0, 0, 2);
    Menu_AddItemType4(&g_menu0x00823858, 0, -1, (MenuItemAction)FrontendMenu_StartMultiplayerKnockout, 3);
    Menu_SetCallbacks(&g_menu0x00823858, (MenuCallback)FrontendMenu_EnterMultiplayerRaceSettings, (MenuCallback)FrontendMenu_UpdateMultiplayerRaceSettings,
                      (MenuCallback)FrontendMenu_DrawMultiplayerRaceSettings, NULL);
    Menu_ValidateCursor(&g_menu0x00823858, 0);
    g_menu0x00823858.items[0].flag3 = 1;
    g_menu0x00823858.items[1].flag3 = 1;
    g_menu0x00823858.items[2].flag3 = 1;
}

void FrontendMenu_DrawNetworkMessage(Menu *pMenu);
void FrontendMenu_DrawArcade(Menu *pMenu);
void FrontendMenu_LeaveArcade(Menu *pMenu, char back);
void FrontendMenu_SelectCarClass(Menu *pMenu, int param);
void FrontendMenu_EnterArcadeCarClass(Menu *pMenu, int param);
void FrontendMenu_DrawCarClass(Menu *pMenu);
void FrontendProfile_SetPlayersRemaining(int value);
void FrontendProfile_SetReturnMenu(Menu *value);
void GameInfo_SetConfiguredPlayerCount(BYTE param1);
void GameInfo_SetConfiguredMultiplayer(BYTE param1);

// Network message page (one hidden entry).
// FUNCTION: CMR2 0x004f9300
void FrontendMenu_BuildNetworkMessage(void)
{
    Menu_Init(&g_menu0x00826140, 0, -1, 0, FrontendMenu_GetMain(), NULL, 1, 0, 0);
    Menu_AddItemType1(&g_menu0x00826140, 0, -1, 0, -1);
    Menu_SetCallbacks(&g_menu0x00826140, NULL, NULL, (MenuCallback)FrontendMenu_DrawNetworkMessage, NULL);
    Menu_ValidateCursor(&g_menu0x00826140, 0);
}

// Item callback of "arcade": starts an arcade game for the chosen players.
// FUNCTION: CMR2 0x004fa9b0
void FrontendMenu_StartArcadeGame(Menu *pMenu, int param)
{
    GameInfo_SetConfiguredPlayerCount(1);
    FrontendProfile_SetPlayersRemaining(CGameInfo::GetConfiguredPlayerCount());
    GameInfo_SetConfiguredMultiplayer(0);
    Menu_SetParent(FrontendMenu_GetMultiplayerProfile(), pMenu);
    FrontendProfile_SetReturnMenu(pMenu);
    Menu_SetNextAction(FrontendMenu_GetMultiplayerProfile());
}

void FrontendMenu_StartArcadeGame(Menu *pMenu, int param);

// Arcade menu: two submenus and "start".
// FUNCTION: CMR2 0x004f9370
void FrontendMenu_BuildArcade(void)
{
    Menu_Init(&g_menu0x008267e0, 0, 0x58, 0, FrontendMenu_GetMain(), NULL, 1, 1, 1);
    Menu_AddItemType2(&g_menu0x008267e0, 0, 0xc, &g_menu0x00826600, 0, 0);
    Menu_AddItemType2(&g_menu0x008267e0, 0, 0xe2, &g_menu0x008276e0, 0, 0);
    Menu_AddItemType4(&g_menu0x008267e0, 0, 0x10, (MenuItemAction)FrontendMenu_StartArcadeGame, -1);
    Menu_SetCallbacks(&g_menu0x008267e0, NULL, (MenuCallback)FrontendScroller_UpdateMain, (MenuCallback)FrontendMenu_DrawArcade,
                      (MenuCallback)FrontendMenu_LeaveArcade);
    Menu_ValidateCursor(&g_menu0x008267e0, 0);
}

// Entering the arcade player-count page: sets up the scroller and the
// cursor from the number of players.
// FUNCTION: CMR2 0x004fa890
void FrontendMenu_EnterArcadePlayerCount(Menu *pMenu, int param)
{
    MenuScroller *p;
    int k;

    RallyData_SetSecondarySelectionNibble(5);
    p = FrontendScroller_GetDifficultyScroller();
    p->startTime = CFrontend::GetFrontendTimestamp();
    p->pMenu = pMenu;
    p->count = pMenu->itemCount;
    for (k = 0; k < pMenu->itemCount; k++)
        p->widths[k] = Font_GetTextWidth(2, (BYTE *)CFrontend::GetTextString(pMenu->items[k].id));
    if (CGameInfo::GetConfiguredPlayerCount() < 3)
        pMenu->cursor = CGameInfo::GetConfiguredPlayerCount() - 1;
    else
        pMenu->cursor = 0;
}

// Item callback of the arcade player-count page.
// FUNCTION: CMR2 0x004fa910
void FrontendMenu_SelectArcadePlayerCount(Menu *pMenu, int param)
{
    GameInfo_SetConfiguredPlayerCount(pMenu->cursor + 1);
    if (CGameInfo::GetConfiguredPlayerCount() == 1)
        RallyData_SetSecondarySelectionNibble(5);
    else
        RallyData_SetSecondarySelectionNibble(0);
    FrontendProfile_SetPlayersRemaining(CGameInfo::GetConfiguredPlayerCount());
    GameInfo_SetConfiguredMultiplayer(0);
    Menu_SetParent(FrontendMenu_GetMultiplayerProfile(), pMenu);
    FrontendProfile_SetReturnMenu(pMenu);
    Menu_SetNextAction(FrontendMenu_GetMultiplayerProfile());
}

void FrontendMenu_EnterArcadePlayerCount(Menu *pMenu, int param);
void FrontendMenu_SelectArcadePlayerCount(Menu *pMenu, int param);
void FrontendScroller_UpdateDifficulty(Menu *pMenu);
void FrontendMenu_DrawDifficulty(Menu *pMenu);

// Arcade player-count page (under the arcade "championship" submenu).
// FUNCTION: CMR2 0x004f9400
void FrontendMenu_BuildArcadeChampionshipPlayerCount(char players)
{
    Menu_Init(&g_menu0x008269c0, 0, 0x1c, 0, &g_menu0x00826600, NULL, 1, 0, 0);
    Menu_AddItemType4(&g_menu0x008269c0, 0, 0x19f, (MenuItemAction)FrontendMenu_SelectArcadePlayerCount, -1);
    Menu_AddItemType4(&g_menu0x008269c0, 0, 0x1a0, (MenuItemAction)FrontendMenu_SelectArcadePlayerCount, -1);
    Menu_SetCallbacks(&g_menu0x008269c0, (MenuCallback)FrontendMenu_EnterArcadePlayerCount, (MenuCallback)FrontendScroller_UpdateDifficulty,
                      (MenuCallback)FrontendMenu_DrawDifficulty, NULL);
    Menu_ValidateCursor(&g_menu0x008269c0, 0);
    g_menu0x008269c0.cursor = players - 1;
}

// Update callback of the arcade player-count page under the arcade menu.
// FUNCTION: CMR2 0x004f9520
void FrontendMenu_UpdateArcadePlayerCount(Menu *pMenu)
{
    FrontendScroller_UpdateDifficulty(pMenu);
}

// Arcade player-count page (under the arcade menu).
// FUNCTION: CMR2 0x004f9490
void FrontendMenu_BuildArcadePlayerCount(char players)
{
    Menu_Init(&g_menu0x008276e0, 0, 0x1c, 0, &g_menu0x008267e0, NULL, 1, 0, 0);
    Menu_AddItemType4(&g_menu0x008276e0, 0, 0x19f, (MenuItemAction)FrontendMenu_SelectArcadePlayerCount, -1);
    Menu_AddItemType4(&g_menu0x008276e0, 0, 0x1a0, (MenuItemAction)FrontendMenu_SelectArcadePlayerCount, -1);
    Menu_SetCallbacks(&g_menu0x008276e0, (MenuCallback)FrontendMenu_EnterArcadePlayerCount, (MenuCallback)FrontendMenu_UpdateArcadePlayerCount,
                      (MenuCallback)FrontendMenu_DrawDifficulty, NULL);
    Menu_ValidateCursor(&g_menu0x008276e0, 0);
    g_menu0x008276e0.items[0].max = players - 1;
}

// Sets up a one-player arcade game.
// FUNCTION: CMR2 0x004fa970
void FrontendMenu_SetupSinglePlayerArcade(void)
{
    GameInfo_SetConfiguredPlayerCount(1);
    RallyData_SetSecondarySelectionNibble(5);
    FrontendProfile_SetPlayersRemaining(1);
    GameInfo_SetConfiguredMultiplayer(0);
    Menu_SetParent(FrontendMenu_GetMultiplayerProfile(), FrontendMenu_GetArcadeGameType());
    FrontendProfile_SetReturnMenu(FrontendMenu_GetArcadeGameType());
}

// Arcade game type menu: three types.
// FUNCTION: CMR2 0x004f9530
void FrontendMenu_BuildArcadeGameType(char unused)
{
    Menu_Init(&g_menu0x00826600, 0, 0x1d, 0, &g_menu0x008267e0, NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x00826600, 0, 0xd0, FrontendMenu_GetMultiplayerProfile(), (MenuItemAction)FrontendMenu_SelectCarClass, 0);
    Menu_AddItemType2(&g_menu0x00826600, 0, 0xd1, FrontendMenu_GetMultiplayerProfile(), (MenuItemAction)FrontendMenu_SelectCarClass, 0);
    Menu_AddItemType2(&g_menu0x00826600, 0, 0xd2, FrontendMenu_GetMultiplayerProfile(), (MenuItemAction)FrontendMenu_SelectCarClass, 0);
    Menu_SetCallbacks(&g_menu0x00826600, (MenuCallback)FrontendMenu_EnterArcadeCarClass, NULL, (MenuCallback)FrontendMenu_DrawCarClass, NULL);
    Menu_ValidateCursor(&g_menu0x00826600, 0);
}

void FrontendMenu_StartQuickRace(Menu *pMenu, int param);
void FrontendMenu_EnterQuickRaceSettings(Menu *pMenu, int param);
void FrontendScroller_UpdateArcadeRally(Menu *pMenu);
void FrontendMenu_DrawQuickRaceSettings(Menu *pMenu);
void FrontendMenu_StartAdvancedQuickRace(Menu *pMenu, int param);
void FrontendMenu_EnterQuickRaceAdvancedSettings(Menu *pMenu, int param);
void FrontendMenu_UpdateQuickRaceAdvancedSettings(Menu *pMenu);
void FrontendMenu_DrawQuickRaceAdvancedSettings(Menu *pMenu);

// First quick race page: two settings and "start".
// FUNCTION: CMR2 0x004fa370
void FrontendMenu_BuildQuickRaceSettings(void)
{
    Menu_Init(&g_menu0x00828040, 0, 0x1a7, 0, FrontendMenu_GetArcadeRallySelection(), NULL, 1, 0, 1);
    Menu_AddItemType3(&g_menu0x00828040, 0, -1, 10, 0, 0, 0, 0, 0);
    Menu_AddItemType3(&g_menu0x00828040, 0, -1, 5, 0, 0, 0, 0, 1);
    Menu_AddItemType4(&g_menu0x00828040, 0, 0x67, (MenuItemAction)FrontendMenu_StartQuickRace, -1);
    Menu_SetCallbacks(&g_menu0x00828040, (MenuCallback)FrontendMenu_EnterQuickRaceSettings, (MenuCallback)FrontendScroller_UpdateArcadeRally,
                      (MenuCallback)FrontendMenu_DrawQuickRaceSettings, NULL);
    Menu_ValidateCursor(&g_menu0x00828040, 0);
}

// Second quick race page: game type, its setting, an on/off option and
// "start".
// FUNCTION: CMR2 0x004fa410
void FrontendMenu_BuildQuickRaceAdvancedSettings(void)
{
    Menu_Init(&g_menu0x00827aa0, 0, 0x1a7, 0, FrontendMenu_GetArcadeRallySelection(), NULL, 1, 0, 1);
    Menu_AddItemType3(&g_menu0x00827aa0, 0, -1, 3, 0, 0, 0, 0, 3);
    Menu_AddItemType3(&g_menu0x00827aa0, 0, -1, 5, 0, 0, 0, 0, -1);
    Menu_AddItemType3(&g_menu0x00827aa0, 0, -1, 5, 0, 0, 0, 0, 2);
    Menu_AddItemType4(&g_menu0x00827aa0, 0, 0x67, (MenuItemAction)FrontendMenu_StartAdvancedQuickRace, -1);
    Menu_SetCallbacks(&g_menu0x00827aa0, (MenuCallback)FrontendMenu_EnterQuickRaceAdvancedSettings, (MenuCallback)FrontendMenu_UpdateQuickRaceAdvancedSettings,
                      (MenuCallback)FrontendMenu_DrawQuickRaceAdvancedSettings, NULL);
    Menu_ValidateCursor(&g_menu0x00827aa0, 0);
}

// FUNCTION: CMR2 0x004f5810
void FrontendMenu_BuildRallyProfileDetails(void)
{
    Menu_Init(&g_menu0x0081fe38, 0, 0x1a, 0, &g_menu0x008212d8, NULL, 1, 0, 1);
    Menu_SetCallbacks(&g_menu0x0081fe38, NULL, NULL, (MenuCallback)FrontendDraw_DrawProfileMenu, NULL);
    Menu_ValidateCursor(&g_menu0x0081fe38, 0);
}

// FUNCTION: CMR2 0x004f5850
void FrontendMenu_BuildProfileActions(void)
{
    Menu_Init(&g_menu0x0081c058, 0, 0x59, 0, &g_menu0x008212d8, NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x0081c058, 0, 0x5a, &g_menu0x00823678, 0, -1);
    Menu_AddItemType2(&g_menu0x0081c058, 0, 0x82, &g_menu0x00820d38, 0, -1);
    Menu_AddItemType2(&g_menu0x0081c058, 0, 0x84, &g_menu0x00821878, 0, -1);
    Menu_SetCallbacks(&g_menu0x0081c058, (MenuCallback)FrontendMenu_EnterProfileActions, NULL, (MenuCallback)FrontendMenu_DrawProfileActions, NULL);
    Menu_ValidateCursor(&g_menu0x0081c058, 0);
}

// FUNCTION: CMR2 0x004f58e0
void FrontendMenu_BuildProfileSettings(void)
{
    Menu_Init(&g_menu0x00823678, 0, 0x5a, 0, &g_menu0x0081c058, NULL, 1, 0, 1);
    Menu_AddItemType4(&g_menu0x00823678, 0, -1, 0, -1);
    Menu_SetCallbacks(&g_menu0x00823678, NULL, NULL, (MenuCallback)FrontendMenu_DrawProfileSettings, NULL);
    Menu_ValidateCursor(&g_menu0x00823678, 0);
}

// FUNCTION: CMR2 0x004f5940
void FrontendMenu_BuildProfileRecordsSummary(void)
{
    Menu_Init(&g_menu0x00821878, 0, 0x84, 0, &g_menu0x0081c058, NULL, 1, 0, 1);
    Menu_AddItemType4(&g_menu0x00821878, 0, 0x82, 0, 0);
    Menu_SetCallbacks(&g_menu0x00821878, NULL, NULL, (MenuCallback)FrontendMenu_DrawProfileRecordsSummary, NULL);
    Menu_ValidateCursor(&g_menu0x00821878, 0);
}

// FUNCTION: CMR2 0x004f59a0
void FrontendMenu_BuildRallyModes(void)
{
    Menu_Init(&g_menu0x00823a38, 0, 0x58, 0, &g_menu0x0081d6d8, NULL, 1, 2, 1);
    Menu_AddItemType2(&g_menu0x00823a38, 0, 0xc, &g_menu0x0081dc78, 0, 0);
    Menu_AddItemType2(&g_menu0x00823a38, 0, 0xd, &g_menu0x00824498, 0, 0);
    Menu_AddItemType2(&g_menu0x00823a38, 0, 0xf, &g_menu0x00823fd8, 0, 0);
    Menu_AddItemType2(&g_menu0x00823a38, 0, 0x10, &g_menu0x00824678, 0, 0);
    Menu_AddItemType2(&g_menu0x00823a38, 0, 0x11, &g_menu0x0081e5d8, 0, 0);
    Menu_SetCallbacks(&g_menu0x00823a38, FrontendMenu_EnterRallyModes, (MenuCallback)FrontendScroller_UpdateMain, (MenuCallback)FrontendMenu_DrawRallyModes, FrontendMenu_LeaveRallyModes);
    Menu_ValidateCursor(&g_menu0x00823a38, 0);
}

// FUNCTION: CMR2 0x004f5a60
void FrontendMenu_BuildSingleRallyModes(void)
{
    Menu_Init(&g_menu0x0081dc78, 0, 0xda, 0, &g_menu0x00823a38, NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x0081dc78, 0, 0x95, &g_menu0x0081d4f8, 0, 0);
    Menu_AddItemType2(&g_menu0x0081dc78, 0, 0x8a, &g_menu0x00820018, 0, -1);
    Menu_SetCallbacks(&g_menu0x0081dc78, NULL, NULL, (MenuCallback)FrontendMenu_DrawSingleRallyModes, NULL);
    Menu_ValidateCursor(&g_menu0x0081dc78, 0);
}

// FUNCTION: CMR2 0x004f5ae0
void FrontendMenu_BuildSingleRallySelection(void)
{
    Menu_Init(&g_menu0x00820018, 0, 0xda, 0, &g_menu0x0081dc78, NULL, 1, 0, 0);
    Menu_AddItemType3(&g_menu0x00820018, 0, -1, 1, 0, 0, 0, (MenuItemAction)FrontendCar_ApplyPreviewSelection, -1);
    Menu_SetCallbacks(&g_menu0x00820018, FrontendMenu_EnterSingleRallySelection, (MenuCallback)FrontendMenu_UpdateSingleRallySelection, (MenuCallback)FrontendMenu_DrawSingleRallySelection, NULL);
    Menu_ValidateCursor(&g_menu0x00820018, 0);
}

// FUNCTION: CMR2 0x004f5b50
void FrontendMenu_BuildRallyStartTransition(void)
{
    Menu_Init(&g_menu0x008214b8, 0, 0, 0, NULL, NULL, 1, 0, 1);
    Menu_SetCallbacks(&g_menu0x008214b8, NULL, (MenuCallback)FrontendMenu_UpdateRallyStartTransition, NULL, NULL);
    Menu_ValidateCursor(&g_menu0x008214b8, 0);
}

// FUNCTION: CMR2 0x004f5b90
void FrontendMenu_BuildNetworkConnection(void)
{
    Menu_Init(&g_menu0x00822598, 0, 0x3c, 0, &g_menu0x008241b8, NULL, 1, 0, 0);
    Menu_AddItemType3(&g_menu0x00822598, 0, -1, 0xa, 0, 1, 0, (MenuItemAction)FrontendNetwork_OpenConnectedLeaderboard, 0);
    Menu_SetCallbacks(&g_menu0x00822598, FrontendMenu_EnterNetworkConnection, NULL, (MenuCallback)FrontendMenu_DrawNetworkConnection, (MenuCallback)FrontendNetwork_LeaveConnectionPages);
    Menu_ValidateCursor(&g_menu0x00822598, 0);
}

// FUNCTION: CMR2 0x004f5c00
void FrontendMenu_BuildNetworkSessionBrowser(void)
{
    Menu_Init(&g_menu0x00824c18, 0, 0x35, 0, &g_menu0x00822598, NULL, 1, 0, 1);
    Menu_AddItemType4(&g_menu0x00824c18, 0, 0x1ec, (MenuItemAction)FrontendNetwork_JoinSelectedSession, 0);
    Menu_AddItemType4(&g_menu0x00824c18, 0, 0x3d, (MenuItemAction)FrontendNetwork_CreateSelectedSession, 0);
    Menu_AddItemType1(&g_menu0x00824c18, 0, 0x1b, 0, 0);
    Menu_SetCallbacks(&g_menu0x00824c18, (MenuCallback)FrontendMenu_EnterNetworkSessionBrowser, (MenuCallback)FrontendMenu_UpdateNetworkSessionBrowser, (MenuCallback)FrontendMenu_DrawNetworkSessionBrowser, (MenuCallback)FrontendNetwork_LeaveConnectionPages);
    Menu_ValidateCursor(&g_menu0x00824c18, 0);
}

// FUNCTION: CMR2 0x004f5c90
void FrontendMenu_BuildNetworkSessionDetails(void)
{
    Menu_Init(&g_menu0x00821c38, 0, 0x3d, 0, &g_menu0x00824c18, NULL, 1, 0, 1);
    Menu_AddItemType4(&g_menu0x00821c38, 0, -1, 0, 0);
    Menu_AddItemType3(&g_menu0x00821c38, 0, -1, 5, 0, 1, 0, 0, 1);
    Menu_AddItemType3(&g_menu0x00821c38, 0, -1, 0x7a, 0xb, 1, 0, 0, 2);
    Menu_AddItemType3(&g_menu0x00821c38, 0, -1, 8, 6, 1, 0, 0, 3);
    Menu_AddItemType4(&g_menu0x00821c38, 0, -1, 0, 4);
    Menu_AddItemType2(&g_menu0x00821c38, 0, -1, &g_menu0x0081bab8, 0, 5);
    Menu_AddItemType4(&g_menu0x00821c38, 0, 0x67, (MenuItemAction)FrontendNetwork_ApplyHostSessionDetails, 6);
    Menu_AddItemType1(&g_menu0x00821c38, 0, 0x1b, 0, 7);
    Menu_SetCallbacks(&g_menu0x00821c38, (MenuCallback)FrontendMenu_EnterNetworkSessionDetails, (MenuCallback)FrontendMenu_UpdateNetworkSessionDetails, (MenuCallback)FrontendMenu_DrawNetworkSessionDetails, (MenuCallback)FrontendMenu_LeaveNetworkSessionDetails);
    Menu_ValidateCursor(&g_menu0x00821c38, 0);
}

// FUNCTION: CMR2 0x004f5d90
void FrontendMenu_BuildNetworkSessionSetup(void)
{
    Menu_Init(&g_menu0x0081e218, 0, 0x3e, 0, &g_menu0x00824c18, NULL, 1, 0, 1);
    Menu_AddItemType4(&g_menu0x0081e218, 0, -1, 0, 0);
    Menu_AddItemType3(&g_menu0x0081e218, 0, -1, 0x16, 0, 1, 0, 0, 1);
    Menu_AddItemType3(&g_menu0x0081e218, 0, -1, 2, 0, 1, 0, 0, 2);
    Menu_AddItemType3(&g_menu0x0081e218, 0, -1, 2, 0, 1, 0, (MenuItemAction)FrontendNetwork_StartHostRace, 3);
    Menu_AddItemType1(&g_menu0x0081e218, 0, 0x1b, 0, 4);
    Menu_SetCallbacks(&g_menu0x0081e218, (MenuCallback)FrontendMenu_EnterNetworkSessionSetup, (MenuCallback)FrontendMenu_UpdateNetworkSessionSetup, (MenuCallback)FrontendMenu_DrawNetworkSessionSetup, (MenuCallback)FrontendMenu_LeaveNetworkSessionSetup);
    Menu_ValidateCursor(&g_menu0x0081e218, 0);
}

// FUNCTION: CMR2 0x004f5e50
void FrontendMenu_BuildNetworkStageTimes(void)
{
    Menu_Init(&g_menu0x00824df8, 0, -1, 0, NULL, NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x00824df8, 0, -1, &g_menu0x0081e218, 0, -1);
    Menu_SetCallbacks(&g_menu0x00824df8, NULL, (MenuCallback)FrontendMenu_UpdateNetworkStageTimes, (MenuCallback)FrontendDraw_DrawStageTimes, NULL);
    Menu_ValidateCursor(&g_menu0x00824df8, 0);
}

// FUNCTION: CMR2 0x004f6040
void FrontendMenu_BuildGameOptions(void)
{
    Menu_Init(&g_menu0x0081ef38, 0, 0x14, 0, &g_menu0x0081f118, NULL, 1, 0, 1);
    Menu_AddItemType3(&g_menu0x0081ef38, 0, 0x91, 2, 0, 0, 0, 0, 0);
    Menu_AddItemType3(&g_menu0x0081ef38, 0, 0x93, 2, 0, 0, 0, 0, 2);
    Menu_AddItemType3(&g_menu0x0081ef38, 0, 0x96, 2, 0, 0, 0, 0, 3);
    Menu_AddItemType3(&g_menu0x0081ef38, 0, 0x151, 3, 0, 0, 0, 0, 4);
    Menu_AddItemType2(&g_menu0x0081ef38, 0, 0x8e, &g_menu0x008210f8, 0, -1);
    Menu_AddItemType2(&g_menu0x0081ef38, 0, 0x67, g_menu0x0081ef38.pParent, (MenuItemAction)FrontendMenu_ApplyGameOptions, -1);
    Menu_SetCallbacks(&g_menu0x0081ef38, (MenuCallback)FrontendMenu_EnterGameOptions, NULL, (MenuCallback)FrontendMenu_DrawGameOptions, NULL);
    Menu_ValidateCursor(&g_menu0x0081ef38, 0);
}

// FUNCTION: CMR2 0x004f6130
void FrontendMenu_BuildGraphicsOptions(void)
{
    Menu_Init(&g_menu0x008210f8, 0, 0x8e, 0, &g_menu0x0081ef38, NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x008210f8, 0, -1, &g_menu0x0081de58, 0, 0);
    Menu_AddItemType3(&g_menu0x008210f8, 0, -1, 2, 0, 0, 0, 0, 1);
    Menu_AddItemType2(&g_menu0x008210f8, 0, -1, &g_menu0x0081c7d8, 0, 2);
    Menu_AddItemType2(&g_menu0x008210f8, 0, -1, &g_menu0x0081cb98, 0, 3);
    Menu_AddItemType3(&g_menu0x008210f8, 0, -1, 3, 0, 0, 0, 0, 4);
    Menu_AddItemType3(&g_menu0x008210f8, 0, -1, 0xa, 0, 0, 0, 0, 5);
    Menu_AddItemType2(&g_menu0x008210f8, 0, 0x1ac, &g_menu0x0081cd78, 0, 6);
    // PORT: the screen options (HUD and menu sizes, vertical sync, fullscreen).
    Menu_AddItemType2(&g_menu0x008210f8, 0, -1, FrontendMenu_GetScreenOptions(), 0, 0xf);
    Menu_AddItemType1(&g_menu0x008210f8, 0, 0x67, FrontendMenu_ApplyGraphicsOptions, 0xe);
    Menu_SetCallbacks(&g_menu0x008210f8, FrontendMenu_EnterGraphicsOptions, NULL, (MenuCallback)FrontendMenu_DrawGraphicsOptions, (MenuCallback)FrontendMenu_LeaveGraphicsOptions);
    Menu_ValidateCursor(&g_menu0x008210f8, 0);
}

// FUNCTION: CMR2 0x004f6240
void FrontendMenu_BuildAdvancedGraphicsOptions(void)
{
    Menu_Init(&g_menu0x0081cd78, 0, 0x1b4, 0, &g_menu0x008210f8, NULL, 1, 0, 1);
    Menu_AddItemType3(&g_menu0x0081cd78, 0, 0x1b8, 3, 0, 0, 0, 0, 8);
    Menu_AddItemType3(&g_menu0x0081cd78, 0, 0x1b7, 2, 0, 0, 0, 0, 9);
    Menu_AddItemType3(&g_menu0x0081cd78, 0, 0x1b6, 2, 0, 0, 0, 0, 0xa);
    Menu_AddItemType1(&g_menu0x0081cd78, 0, 0x67, (MenuItemAction)FrontendMenu_ApplyAdvancedGraphicsOptions, 0xe);
    Menu_SetCallbacks(&g_menu0x0081cd78, FrontendMenu_EnterAdvancedGraphicsOptions, (MenuCallback)FrontendMenu_UpdateAdvancedGraphicsOptions, (MenuCallback)FrontendMenu_DrawAdvancedGraphicsOptions, NULL);
    Menu_ValidateCursor(&g_menu0x0081cd78, 0);
}

// FUNCTION: CMR2 0x004f6300
void FrontendMenu_BuildRenderDeviceOptions(void)
{
    Menu_Init(&g_menu0x0081de58, 0, 0x1b1, 0, &g_menu0x008210f8, NULL, 1, 0, 1);
    Menu_AddItemType4(&g_menu0x0081de58, 0, 0x133, (MenuItemAction)FrontendMenu_ApplyRenderDeviceOptions, 0);
    Menu_AddItemType4(&g_menu0x0081de58, 0, 0x1ae, (MenuItemAction)FrontendMenu_ApplyRenderDeviceOptions, 0);
    Menu_AddItemType4(&g_menu0x0081de58, 0, 0x1af, (MenuItemAction)FrontendMenu_ApplyRenderDeviceOptions, 0);
    Menu_AddItemType4(&g_menu0x0081de58, 0, 0x1b0, (MenuItemAction)FrontendMenu_ApplyRenderDeviceOptions, 0);
    Menu_SetCallbacks(&g_menu0x0081de58, FrontendMenu_EnterRenderDeviceOptions, NULL, (MenuCallback)FrontendMenu_DrawRenderDeviceOptions, NULL);
    Menu_ValidateCursor(&g_menu0x0081de58, 0);
}

// FUNCTION: CMR2 0x004f6420
void FrontendMenu_BuildDisplayMode(void)
{
    Menu_Init(&g_menu0x0081cb98, 0, 0x14, 0, &g_menu0x008210f8, NULL, 1, 0, 0);
    Menu_AddItemType3(&g_menu0x0081cb98, 0, -1, 2, 0, 0, 0, (MenuItemAction)FrontendMenu_ApplyDisplayMode, 0);
    Menu_SetCallbacks(&g_menu0x0081cb98, FrontendMenu_EnterDisplayMode, (MenuCallback)FrontendMenu_UpdateDisplayMode, (MenuCallback)FrontendMenu_DrawDisplayMode, NULL);
    Menu_ValidateCursor(&g_menu0x0081cb98, 0);
}

// FUNCTION: CMR2 0x004f6490
void FrontendMenu_BuildSoundOptions(void)
{
    Menu_Init(&g_menu0x0081fa78, 0, 0x15, 0, &g_menu0x0081f118, NULL, 1, 0, 1);
    Menu_AddItemType3(&g_menu0x0081fa78, 0, 0x5e, 0xb, 0xa, 0, 0, 0, 0);
    Menu_AddItemType3(&g_menu0x0081fa78, 0, 0x5f, 0xb, 0xa, 0, 0, 0, 1);
    Menu_AddItemType3(&g_menu0x0081fa78, 0, 0x60, 0xb, 0xa, 0, 0, 0, 2);
    Menu_AddItemType2(&g_menu0x0081fa78, 0, 0x67, g_menu0x0081fa78.pParent, 0, 4);
    Menu_SetCallbacks(&g_menu0x0081fa78, FrontendMenu_EnterSoundOptions, (MenuCallback)FrontendMenu_UpdateSoundOptions, (MenuCallback)FrontendMenu_DrawSoundOptions, (MenuCallback)FrontendMenu_LeaveSoundOptions);
    Menu_ValidateCursor(&g_menu0x0081fa78, 0);
}

// FUNCTION: CMR2 0x004f6540
void FrontendMenu_BuildRecords(void)
{
    Menu_Init(&g_menu0x00823c18, 0, 0x16, 0, &g_menu0x0081f118, NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x00823c18, 0, 0x162, &g_menu0x00820b58, 0, -1);
    Menu_AddItemType2(&g_menu0x00823c18, 0, 0x163, &g_menu0x0081c418, 0, -1);
    Menu_AddItemType2(&g_menu0x00823c18, 0, 0x164, &g_menu0x008223b8, 0, -1);
    Menu_AddItemType2(&g_menu0x00823c18, 0, 0x165, &g_menu0x008232b8, 0, -1);
    Menu_AddItemType2(&g_menu0x00823c18, 0, 0x166, &g_menu0x0081c5f8, 0, -1);
    Menu_SetCallbacks(&g_menu0x00823c18, FrontendRecords_ResetCarouselSelection, NULL, (MenuCallback)FrontendMenu_DrawRecordsRoot, NULL);
    Menu_ValidateCursor(&g_menu0x00823c18, 0);
}

// FUNCTION: CMR2 0x004f6610
void FrontendMenu_BuildProfileRecords(void)
{
    Menu_Init(&g_menu0x00820d38, 0, 0x16, 0, &g_menu0x0081c058, NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x00820d38, 0, 0x162, &g_menu0x00823498, 0, -1);
    Menu_AddItemType2(&g_menu0x00820d38, 0, 0x163, &g_menu0x008201f8, 0, -1);
    Menu_AddItemType2(&g_menu0x00820d38, 0, 0x164, &g_menu0x00822ef8, 0, -1);
    Menu_AddItemType2(&g_menu0x00820d38, 0, 0x165, &g_menu0x0081b518, 0, -1);
    Menu_AddItemType2(&g_menu0x00820d38, 0, 0x166, &g_menu0x0081f6b8, 0, -1);
    Menu_SetCallbacks(&g_menu0x00820d38, FrontendRecords_ResetCarouselSelection, NULL, (MenuCallback)FrontendMenu_DrawRecordsRoot, NULL);
    Menu_ValidateCursor(&g_menu0x00820d38, 0);
}

// FUNCTION: CMR2 0x004f6740
void FrontendMenu_BuildStageRecords(void)
{
    Menu_Init(&g_menu0x0081c418, 0, 0x163, 0, &g_menu0x00823c18, NULL, 1, 0, 0);
    Menu_AddItemType4(&g_menu0x0081c418, 0, 0x27, (MenuItemAction)FrontendRecords_CycleTableMode, -1);
    Menu_AddItemType4(&g_menu0x0081c418, 0, 0x28, (MenuItemAction)FrontendRecords_CycleTableMode, -1);
    Menu_AddItemType4(&g_menu0x0081c418, 0, 0x29, (MenuItemAction)FrontendRecords_CycleTableMode, -1);
    Menu_AddItemType4(&g_menu0x0081c418, 0, 0x2a, (MenuItemAction)FrontendRecords_CycleTableMode, -1);
    Menu_AddItemType4(&g_menu0x0081c418, 0, 0x2b, (MenuItemAction)FrontendRecords_CycleTableMode, -1);
    Menu_AddItemType4(&g_menu0x0081c418, 0, 0x2c, (MenuItemAction)FrontendRecords_CycleTableMode, -1);
    Menu_AddItemType4(&g_menu0x0081c418, 0, 0x2d, (MenuItemAction)FrontendRecords_CycleTableMode, -1);
    Menu_AddItemType4(&g_menu0x0081c418, 0, 0x2e, (MenuItemAction)FrontendRecords_CycleTableMode, -1);
    Menu_SetCallbacks(&g_menu0x0081c418, FrontendScroller_InitStageRecords, (MenuCallback)FrontendScroller_UpdateStageRecords, (MenuCallback)FrontendMenu_DrawStageRecords, NULL);
    Menu_ValidateCursor(&g_menu0x0081c418, 0);
}

// FUNCTION: CMR2 0x004f6830
void FrontendMenu_BuildBestStageTimes(void)
{
    Menu_Init(&g_menu0x008223b8, 0, 0x164, 0, &g_menu0x00823c18, NULL, 1, 0, 0);
    Menu_AddItemType4(&g_menu0x008223b8, 0, 0x27, 0, -1);
    Menu_AddItemType4(&g_menu0x008223b8, 0, 0x28, 0, -1);
    Menu_AddItemType4(&g_menu0x008223b8, 0, 0x29, 0, -1);
    Menu_AddItemType4(&g_menu0x008223b8, 0, 0x2a, 0, -1);
    Menu_AddItemType4(&g_menu0x008223b8, 0, 0x2b, 0, -1);
    Menu_AddItemType4(&g_menu0x008223b8, 0, 0x2c, 0, -1);
    Menu_AddItemType4(&g_menu0x008223b8, 0, 0x2d, 0, -1);
    Menu_AddItemType4(&g_menu0x008223b8, 0, 0x2e, 0, -1);
    Menu_SetCallbacks(&g_menu0x008223b8, FrontendScroller_InitBestStageTimes, (MenuCallback)FrontendScroller_UpdateBestStageTimes, (MenuCallback)FrontendMenu_DrawBestStageTimes, NULL);
    Menu_ValidateCursor(&g_menu0x008223b8, 0);
}

// FUNCTION: CMR2 0x004f6910
void FrontendMenu_BuildTransmissionRecords(void)
{
    Menu_Init(&g_menu0x008232b8, 0, 0x165, 0, &g_menu0x00823c18, NULL, 1, 0, 0);
    Menu_AddItemType4(&g_menu0x008232b8, 0, 0xe9, (MenuItemAction)FrontendRecords_CycleTableMode, -1);
    Menu_AddItemType4(&g_menu0x008232b8, 0, 0xe8, (MenuItemAction)FrontendRecords_CycleTableMode, -1);
    Menu_SetCallbacks(&g_menu0x008232b8, FrontendScroller_InitTransmissionRecords, (MenuCallback)FrontendScroller_UpdateTransmissionRecords, (MenuCallback)FrontendMenu_DrawTransmissionRecords, NULL);
    Menu_ValidateCursor(&g_menu0x008232b8, 0);
}

// FUNCTION: CMR2 0x004f6990
void FrontendMenu_BuildChampionshipRecords(void)
{
    Menu_Init(&g_menu0x0081c5f8, 0, 0x166, 0, &g_menu0x00823c18, NULL, 1, 0, 0);
    Menu_AddItemType4(&g_menu0x0081c5f8, 0, -1, 0, -1);
    Menu_SetCallbacks(&g_menu0x0081c5f8, NULL, NULL, (MenuCallback)FrontendMenu_DrawChampionshipRecords, NULL);
    Menu_ValidateCursor(&g_menu0x0081c5f8, 0);
}

// FUNCTION: CMR2 0x004f6a50
void FrontendMenu_BuildProfileStageRecords(void)
{
    Menu_Init(&g_menu0x008201f8, 0, 0x163, 0, &g_menu0x00820d38, NULL, 1, 0, 0);
    Menu_AddItemType4(&g_menu0x008201f8, 0, 0x27, (MenuItemAction)FrontendRecords_CycleTableMode, -1);
    Menu_AddItemType4(&g_menu0x008201f8, 0, 0x28, (MenuItemAction)FrontendRecords_CycleTableMode, -1);
    Menu_AddItemType4(&g_menu0x008201f8, 0, 0x29, (MenuItemAction)FrontendRecords_CycleTableMode, -1);
    Menu_AddItemType4(&g_menu0x008201f8, 0, 0x2a, (MenuItemAction)FrontendRecords_CycleTableMode, -1);
    Menu_AddItemType4(&g_menu0x008201f8, 0, 0x2b, (MenuItemAction)FrontendRecords_CycleTableMode, -1);
    Menu_AddItemType4(&g_menu0x008201f8, 0, 0x2c, (MenuItemAction)FrontendRecords_CycleTableMode, -1);
    Menu_AddItemType4(&g_menu0x008201f8, 0, 0x2d, (MenuItemAction)FrontendRecords_CycleTableMode, -1);
    Menu_AddItemType4(&g_menu0x008201f8, 0, 0x2e, (MenuItemAction)FrontendRecords_CycleTableMode, -1);
    Menu_SetCallbacks(&g_menu0x008201f8, FrontendScroller_InitStageRecords, (MenuCallback)FrontendScroller_UpdateStageRecords, (MenuCallback)FrontendMenu_DrawProfileStageRecords, NULL);
    Menu_ValidateCursor(&g_menu0x008201f8, 0);
}

// FUNCTION: CMR2 0x004f6c90
void FrontendMenu_BuildProfileTransmissionRecords(void)
{
    Menu_Init(&g_menu0x0081b518, 0, 0x165, 0, &g_menu0x00820d38, NULL, 1, 0, 0);
    Menu_AddItemType4(&g_menu0x0081b518, 0, 0xe9, (MenuItemAction)FrontendRecords_CycleTableMode, -1);
    Menu_AddItemType4(&g_menu0x0081b518, 0, 0xe8, (MenuItemAction)FrontendRecords_CycleTableMode, -1);
    Menu_SetCallbacks(&g_menu0x0081b518, FrontendScroller_InitTransmissionRecords, (MenuCallback)FrontendScroller_UpdateTransmissionRecords, (MenuCallback)FrontendMenu_DrawProfileTransmissionRecords, NULL);
    Menu_ValidateCursor(&g_menu0x0081b518, 0);
}

// FUNCTION: CMR2 0x004f6d10
void FrontendMenu_BuildProfileChampionshipRecords(void)
{
    Menu_Init(&g_menu0x0081f6b8, 0, 0x166, 0, &g_menu0x00820d38, NULL, 1, 0, 0);
    Menu_AddItemType4(&g_menu0x0081f6b8, 0, -1, 0, -1);
    Menu_SetCallbacks(&g_menu0x0081f6b8, NULL, NULL, (MenuCallback)FrontendMenu_DrawProfileChampionshipRecords, NULL);
    Menu_ValidateCursor(&g_menu0x0081f6b8, 0);
}

// FUNCTION: CMR2 0x004f6d70
void FrontendMenu_BuildCredits(void)
{
    Menu_Init(&g_menu0x00822958, 0, 0x34, 0, &g_menu0x0081d6d8, NULL, 1, 0, 0);
    Menu_AddItemType4(&g_menu0x00822958, 0, -1, 0, 0);
    Menu_SetCallbacks(&g_menu0x00822958, FrontendMenu_EnterCredits, NULL, (MenuCallback)FrontendMenu_DrawCredits, NULL);
    Menu_ValidateCursor(&g_menu0x00822958, 0);
}

// FUNCTION: CMR2 0x004f6dd0
void FrontendMenu_BuildQuitConfirmation(void)
{
    Menu_Init(&g_menu0x00823df8, 0, 3, 0, &g_menu0x0081d6d8, NULL, 1, 1, 1);
    Menu_AddItemType2(&g_menu0x00823df8, 0, 5, &g_menu0x00820798, 0, 0);
    Menu_AddItemType1(&g_menu0x00823df8, 0, 4, (MenuItemAction)FrontendMenu_CancelQuit, 0);
    Menu_SetCallbacks(&g_menu0x00823df8, FrontendMenu_EnterQuitConfirmation, (MenuCallback)FrontendScroller_UpdateMain, (MenuCallback)FrontendMenu_DrawQuitConfirmation, (MenuCallback)FrontendMenu_LeaveQuitConfirmation);
    Menu_ValidateCursor(&g_menu0x00823df8, 0);
}

// FUNCTION: CMR2 0x004f7270
void FrontendMenu_BuildSingleRallyCarClass(int unused)
{
    Menu_Init(&g_menu0x0081d4f8, 0, 0x1d, 0, &g_menu0x0081dc78, NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x0081d4f8, 0, 0xd0, &g_menu0x0081d318, (MenuItemAction)FrontendMenu_SelectCarClass, 0);
    Menu_AddItemType2(&g_menu0x0081d4f8, 0, 0xd1, &g_menu0x0081d318, (MenuItemAction)FrontendMenu_SelectCarClass, 0);
    Menu_AddItemType2(&g_menu0x0081d4f8, 0, 0xd2, &g_menu0x0081d318, (MenuItemAction)FrontendMenu_SelectCarClass, 0);
    Menu_SetCallbacks(&g_menu0x0081d4f8, FrontendMenu_EnterCarClass, NULL, (MenuCallback)FrontendMenu_DrawCarClass, NULL);
    Menu_ValidateCursor(&g_menu0x0081d4f8, 0);
}

// FUNCTION: CMR2 0x004f7310
void FrontendMenu_BuildChampionshipCarClass(int unused)
{
    Menu_Init(&g_menu0x00824498, 0, 0x1d, 0, &g_menu0x00823a38, NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x00824498, 0, 0xd0, &g_menu0x00820978, (MenuItemAction)FrontendMenu_SelectCarClass, 0);
    Menu_AddItemType2(&g_menu0x00824498, 0, 0xd1, &g_menu0x00820978, (MenuItemAction)FrontendMenu_SelectCarClass, 0);
    Menu_AddItemType2(&g_menu0x00824498, 0, 0xd2, &g_menu0x00820978, (MenuItemAction)FrontendMenu_SelectCarClass, 0);
    Menu_SetCallbacks(&g_menu0x00824498, FrontendMenu_EnterCarClass, NULL, (MenuCallback)FrontendMenu_DrawCarClass, NULL);
    Menu_ValidateCursor(&g_menu0x00824498, 0);
}

// FUNCTION: CMR2 0x004f73b0
void FrontendMenu_BuildTimeTrialCarClass(int unused)
{
    Menu_Init(&g_menu0x00823fd8, 0, 0x1d, 0, &g_menu0x00823a38, NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x00823fd8, 0, 0xd0, &g_menu0x0081f2f8, (MenuItemAction)FrontendMenu_SelectCarClass, 0);
    Menu_AddItemType2(&g_menu0x00823fd8, 0, 0xd1, &g_menu0x0081f2f8, (MenuItemAction)FrontendMenu_SelectCarClass, 0);
    Menu_AddItemType2(&g_menu0x00823fd8, 0, 0xd2, &g_menu0x0081f2f8, (MenuItemAction)FrontendMenu_SelectCarClass, 0);
    Menu_SetCallbacks(&g_menu0x00823fd8, FrontendMenu_EnterCarClass, NULL, (MenuCallback)FrontendMenu_DrawCarClass, NULL);
    Menu_ValidateCursor(&g_menu0x00823fd8, 0);
}

// FUNCTION: CMR2 0x004f7450
void FrontendMenu_BuildMultiplayerCarClass(void)
{
    Menu_Init(&g_menu0x0081ed58, 0, 0x41, 0, &g_menu0x00823a38, NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x0081ed58, 0, 0x42, &g_menu0x00823858, (MenuItemAction)FrontendMenu_SelectMultiplayerClass, -1);
    Menu_AddItemType2(&g_menu0x0081ed58, 0, 0xd0, &g_menu0x00823858, (MenuItemAction)FrontendMenu_SelectMultiplayerClass, -1);
    Menu_AddItemType2(&g_menu0x0081ed58, 0, 0xd1, &g_menu0x00823858, (MenuItemAction)FrontendMenu_SelectMultiplayerClass, -1);
    Menu_AddItemType2(&g_menu0x0081ed58, 0, 0xd2, &g_menu0x00823858, (MenuItemAction)FrontendMenu_SelectMultiplayerClass, -1);
    Menu_SetCallbacks(&g_menu0x0081ed58, FrontendMenu_EnterMultiplayerCarClass, NULL, (MenuCallback)FrontendMenu_DrawMultiplayerCarClass, NULL);
    Menu_ValidateCursor(&g_menu0x0081ed58, 0);
}

// FUNCTION: CMR2 0x004f77a0
void FrontendMenu_BuildMultiplayerStageSelection(void)
{
    Menu_Init(&g_menu0x008203d8, 0, 0x17a, 0, &g_menu0x008241b8, NULL, 1, 0, 1);
    Menu_AddItemType6(&g_menu0x008203d8, 0, -1, 1, 0, 1, 0, (MenuItemAction)FrontendProfile_ApplyStageSelectionAndAdvance, -1);
    Menu_AddItemType1(&g_menu0x008203d8, 0, 0x11a, 0, 0);
    Menu_SetCallbacks(&g_menu0x008203d8, FrontendMenu_EnterMultiplayerStageSelection, (MenuCallback)FrontendMenu_UpdateMultiplayerStageSelection, (MenuCallback)FrontendMenu_DrawMultiplayerStageSelection, FrontendMenu_LeaveMultiplayerStageSelection);
    Menu_ValidateCursor(&g_menu0x008203d8, 0);
}

// FUNCTION: CMR2 0x004f7820
void FrontendMenu_BuildProfileNameEntry(void)
{
    Menu_Init(&g_menu0x00824a38, 0, 0x22, 0, NULL, NULL, 1, 0, 1);
    Menu_AddItemType6(&g_menu0x00824a38, 0, -1, 0xa, 0, 1, 0, (MenuItemAction)FrontendProfile_PickNameCharacter, 0);
    Menu_AddItemType6(&g_menu0x00824a38, 0, -1, 0xa, 0, 1, 0, (MenuItemAction)FrontendProfile_PickNameCharacter, 1);
    Menu_AddItemType6(&g_menu0x00824a38, 0, -1, 0xa, 0, 1, 0, (MenuItemAction)FrontendProfile_PickNameCharacter, 2);
    Menu_SetCallbacks(&g_menu0x00824a38, (MenuCallback)FrontendMenu_EnterProfileNameEntry, (MenuCallback)FrontendMenu_UpdateProfileNameEntry, (MenuCallback)FrontendMenu_DrawProfileNameEntry, (MenuCallback)FrontendMenu_LeaveProfileNameEntry);
    Menu_ValidateCursor(&g_menu0x00824a38, 0);
}

// FUNCTION: CMR2 0x004f78c0
void FrontendMenu_BuildProfileRenameEntry(void)
{
    Menu_Init(&g_menu0x0081d8b8, 0, 0x8c, 0, &g_menu0x00824a38, NULL, 1, 0, 1);
    Menu_AddItemType6(&g_menu0x0081d8b8, 0, -1, 0xa, 0, 1, 0, (MenuItemAction)FrontendProfile_PickRenameCharacterOrCheat, 0);
    Menu_AddItemType6(&g_menu0x0081d8b8, 0, -1, 0xa, 0, 1, 0, (MenuItemAction)FrontendProfile_PickRenameCharacterOrCheat, 1);
    Menu_AddItemType6(&g_menu0x0081d8b8, 0, -1, 0xa, 0, 1, 0, (MenuItemAction)FrontendProfile_PickRenameCharacterOrCheat, 2);
    Menu_SetCallbacks(&g_menu0x0081d8b8, (MenuCallback)FrontendMenu_EnterProfileRenameEntry, (MenuCallback)FrontendMenu_UpdateProfileRenameEntry, (MenuCallback)FrontendMenu_DrawProfileRenameEntry, FrontendMenu_LeaveProfileRenameEntry);
    Menu_ValidateCursor(&g_menu0x0081d8b8, 0);
}

// FUNCTION: CMR2 0x004f7970
void FrontendMenu_BuildProfileDateEntry(void)
{
    Menu_Init(&g_menu0x0081bc98, 0, 0x8d, 0, &g_menu0x0081d8b8, NULL, 1, 0, 1);
    Menu_AddItemType3(&g_menu0x0081bc98, 0, -1, 0xff, 0x64, 1, 0, 0, 0);
    Menu_AddItemType3(&g_menu0x0081bc98, 0, -1, 0xc, 0, 1, 0, 0, 1);
    Menu_AddItemType3(&g_menu0x0081bc98, 0, -1, 0x1f, 0, 1, 0, 0, 2);
    Menu_AddItemType4(&g_menu0x0081bc98, 0, 0x67, (MenuItemAction)FrontendProfile_ApplyDateEntry, -1);
    Menu_SetCallbacks(&g_menu0x0081bc98, FrontendProfile_FillDateOfBirthPicker, (MenuCallback)FrontendMenu_UpdateProfileDateEntry, (MenuCallback)FrontendMenu_DrawProfileDateEntry, FrontendMenu_ClearOverlayMode);
    Menu_ValidateCursor(&g_menu0x0081bc98, 0);
}

// FUNCTION: CMR2 0x004f7a30
void FrontendMenu_BuildCarSetup(void)
{
    Menu_Init(&g_menu0x00821e18, 0, 0x33, 0, NULL, NULL, 1, 0, 0);
    Menu_AddItemType2(&g_menu0x00821e18, 0, 0x98, &g_menu0x0081cf58, (MenuItemAction)FrontendProfile_SelectPalette, -1);
    Menu_AddItemType2(&g_menu0x00821e18, 0, 0x99, &g_menu0x0081cf58, (MenuItemAction)FrontendProfile_SelectPalette, -1);
    Menu_AddItemType2(&g_menu0x00821e18, 0, 0x9a, &g_menu0x0081cf58, (MenuItemAction)FrontendProfile_SelectPalette, -1);
    Menu_AddItemType2(&g_menu0x00821e18, 0, 0x9b, &g_menu0x0081cf58, (MenuItemAction)FrontendProfile_SelectPalette, -1);
    Menu_AddItemType2(&g_menu0x00821e18, 0, 0x9c, &g_menu0x0081cf58, (MenuItemAction)FrontendProfile_SelectPalette, -1);
    Menu_AddItemType2(&g_menu0x00821e18, 0, 0x9d, &g_menu0x0081cf58, (MenuItemAction)FrontendProfile_SelectPalette, -1);
    Menu_AddItemType2(&g_menu0x00821e18, 0, 0x9e, &g_menu0x0081cf58, (MenuItemAction)FrontendProfile_SelectPalette, -1);
    Menu_AddItemType2(&g_menu0x00821e18, 0, 0x9f, &g_menu0x0081cf58, (MenuItemAction)FrontendProfile_SelectPalette, -1);
    Menu_AddItemType2(&g_menu0x00821e18, 0, 0xa0, &g_menu0x0081cf58, (MenuItemAction)FrontendProfile_SelectPalette, -1);
    Menu_AddItemType2(&g_menu0x00821e18, 0, 0xa1, &g_menu0x0081cf58, (MenuItemAction)FrontendProfile_SelectPalette, -1);
    Menu_AddItemType2(&g_menu0x00821e18, 0, 0xa2, &g_menu0x0081cf58, (MenuItemAction)FrontendProfile_SelectPalette, -1);
    Menu_AddItemType2(&g_menu0x00821e18, 0, 0xa3, &g_menu0x0081cf58, (MenuItemAction)FrontendProfile_SelectPalette, -1);
    Menu_AddItemType2(&g_menu0x00821e18, 0, 0xa4, &g_menu0x0081cf58, (MenuItemAction)FrontendProfile_SelectPalette, -1);
    Menu_AddItemType2(&g_menu0x00821e18, 0, 0xa5, &g_menu0x0081cf58, (MenuItemAction)FrontendProfile_SelectPalette, -1);
    Menu_AddItemType2(&g_menu0x00821e18, 0, 0xa6, &g_menu0x0081cf58, (MenuItemAction)FrontendProfile_SelectPalette, -1);
    Menu_AddItemType2(&g_menu0x00821e18, 0, 0xa7, &g_menu0x0081cf58, (MenuItemAction)FrontendProfile_SelectPalette, -1);
    Menu_AddItemType2(&g_menu0x00821e18, 0, 0xa8, &g_menu0x0081cf58, (MenuItemAction)FrontendProfile_SelectPalette, -1);
    Menu_AddItemType2(&g_menu0x00821e18, 0, 0xa9, &g_menu0x0081cf58, (MenuItemAction)FrontendProfile_SelectPalette, -1);
    Menu_AddItemType2(&g_menu0x00821e18, 0, 0xaa, &g_menu0x0081cf58, (MenuItemAction)FrontendProfile_SelectPalette, -1);
    Menu_AddItemType2(&g_menu0x00821e18, 0, 0xab, &g_menu0x0081cf58, (MenuItemAction)FrontendProfile_SelectPalette, -1);
    Menu_AddItemType2(&g_menu0x00821e18, 0, 0xac, &g_menu0x0081cf58, (MenuItemAction)FrontendProfile_SelectPalette, -1);
    Menu_AddItemType2(&g_menu0x00821e18, 0, 0xad, &g_menu0x0081cf58, (MenuItemAction)FrontendProfile_SelectPalette, -1);
    Menu_SetCallbacks(&g_menu0x00821e18, FrontendScroller_InitRallyStageSelection, (MenuCallback)FrontendScroller_UpdateChampionshipEntry, (MenuCallback)FrontendMenu_DrawCarSetup, FrontendMenu_ClearOverlayMode);
    Menu_ValidateCursor(&g_menu0x00821e18, 0);
}

// FUNCTION: CMR2 0x004f7d00
void FrontendMenu_BuildPaletteSelection(void)
{
    Menu_Init(&g_menu0x0081cf58, 0, 0x97, 0, &g_menu0x00821e18, NULL, 1, 0, 1);
    Menu_AddItemType3(&g_menu0x0081cf58, 0, 0x88, 2, 0, 0, 0, (MenuItemAction)FrontendProfile_OpenPaletteEditor, 0);
    Menu_SetCallbacks(&g_menu0x0081cf58, (MenuCallback)FrontendMenu_EnterPaletteSelection, (MenuCallback)FrontendScroller_UpdateChampionshipEntry, (MenuCallback)FrontendMenu_DrawSavedStageSelection, NULL);
    Menu_ValidateCursor(&g_menu0x0081cf58, 0);
}

// FUNCTION: CMR2 0x004f7d70
void FrontendMenu_BuildPaletteConfirmation(void)
{
    Menu_Init(&g_menu0x00822d18, 0, 0x33, 0, &g_menu0x008241b8, NULL, 1, 0, 1);
    Menu_AddItemType3(&g_menu0x00822d18, 0, 0x88, 2, 0, 0, 0, (MenuItemAction)FrontendChampionship_ToggleAndAdvanceEntry, 0);
    Menu_SetCallbacks(&g_menu0x00822d18, FrontendMenu_EnterPaletteConfirmation, NULL, (MenuCallback)FrontendMenu_DrawTimeAttackOptions, NULL);
    Menu_ValidateCursor(&g_menu0x00822d18, 0);
}

// FUNCTION: CMR2 0x004f7de0
void FrontendMenu_BuildRallySelection(void)
{
    Menu_Init(&g_menu0x0081eb78, 0, 0x24, 0, NULL, NULL, 1, 0, 0);
    Menu_AddItemType4(&g_menu0x0081eb78, 0, 0x27, (MenuItemAction)FrontendMenu_EnableRallyStageRows, -1);
    Menu_AddItemType4(&g_menu0x0081eb78, 0, 0x28, (MenuItemAction)FrontendMenu_EnableRallyStageRows, -1);
    Menu_AddItemType4(&g_menu0x0081eb78, 0, 0x29, (MenuItemAction)FrontendMenu_EnableRallyStageRows, -1);
    Menu_AddItemType4(&g_menu0x0081eb78, 0, 0x2a, (MenuItemAction)FrontendMenu_EnableRallyStageRows, -1);
    Menu_AddItemType4(&g_menu0x0081eb78, 0, 0x2b, (MenuItemAction)FrontendMenu_EnableRallyStageRows, -1);
    Menu_AddItemType4(&g_menu0x0081eb78, 0, 0x2c, (MenuItemAction)FrontendMenu_EnableRallyStageRows, -1);
    Menu_AddItemType4(&g_menu0x0081eb78, 0, 0x2d, (MenuItemAction)FrontendMenu_EnableRallyStageRows, -1);
    Menu_AddItemType4(&g_menu0x0081eb78, 0, 0x2e, (MenuItemAction)FrontendMenu_EnableRallyStageRows, -1);
    Menu_SetCallbacks(&g_menu0x0081eb78, FrontendMenu_EnterRallySelection, (MenuCallback)FrontendMenu_UpdateRallySelection, (MenuCallback)FrontendMenu_DrawRallySelection, NULL);
    Menu_ValidateCursor(&g_menu0x0081eb78, 0);
}

// FUNCTION: CMR2 0x004f7ed0
void FrontendMenu_BuildRallyStageSelection(void)
{
    Menu_Init(&g_menu0x0081e3f8, 0, 0x25, 0, &g_menu0x0081eb78, NULL, 1, 0, 0);
    Menu_AddItemType4(&g_menu0x0081e3f8, 0, 0xc5, (MenuItemAction)FrontendMenu_SelectRallyStage, -1);
    Menu_AddItemType4(&g_menu0x0081e3f8, 0, 0xc6, (MenuItemAction)FrontendMenu_SelectRallyStage, -1);
    Menu_AddItemType4(&g_menu0x0081e3f8, 0, 0xc7, (MenuItemAction)FrontendMenu_SelectRallyStage, -1);
    Menu_AddItemType4(&g_menu0x0081e3f8, 0, 0xc8, (MenuItemAction)FrontendMenu_SelectRallyStage, -1);
    Menu_AddItemType4(&g_menu0x0081e3f8, 0, 0xc9, (MenuItemAction)FrontendMenu_SelectRallyStage, -1);
    Menu_AddItemType4(&g_menu0x0081e3f8, 0, 0xca, (MenuItemAction)FrontendMenu_SelectRallyStage, -1);
    Menu_AddItemType4(&g_menu0x0081e3f8, 0, 0xcb, (MenuItemAction)FrontendMenu_SelectRallyStage, -1);
    Menu_AddItemType4(&g_menu0x0081e3f8, 0, 0xcc, (MenuItemAction)FrontendMenu_SelectRallyStage, -1);
    Menu_AddItemType4(&g_menu0x0081e3f8, 0, 0xcd, (MenuItemAction)FrontendMenu_SelectRallyStage, -1);
    Menu_AddItemType4(&g_menu0x0081e3f8, 0, 0xce, (MenuItemAction)FrontendMenu_SelectRallyStage, -1);
    Menu_AddItemType4(&g_menu0x0081e3f8, 0, 0xcf, (MenuItemAction)FrontendMenu_SelectRallyStage, -1);
    Menu_SetCallbacks(&g_menu0x0081e3f8, FrontendMenu_EnterRallyStageSelection, (MenuCallback)FrontendMenu_UpdateRallyStageSelection, (MenuCallback)FrontendMenu_DrawRallySummary, NULL);
    Menu_ValidateCursor(&g_menu0x0081e3f8, 0);
}

// FUNCTION: CMR2 0x004f8020
void FrontendMenu_BuildAlternateRallyStageSelection(void)
{
    Menu_Init(&g_menu0x0081f4d8, 0, 0x155, 0, &g_menu0x0081e3f8, NULL, 1, 0, 1);
    Menu_AddItemType4(&g_menu0x0081f4d8, 0, 0xc5, FrontendMenu_SelectAlternateRallyStage, -1);
    Menu_AddItemType4(&g_menu0x0081f4d8, 0, 0xc6, FrontendMenu_SelectAlternateRallyStage, -1);
    Menu_AddItemType4(&g_menu0x0081f4d8, 0, 0xc7, FrontendMenu_SelectAlternateRallyStage, -1);
    Menu_AddItemType4(&g_menu0x0081f4d8, 0, 0xc8, FrontendMenu_SelectAlternateRallyStage, -1);
    Menu_AddItemType4(&g_menu0x0081f4d8, 0, 0xc9, FrontendMenu_SelectAlternateRallyStage, -1);
    Menu_AddItemType4(&g_menu0x0081f4d8, 0, 0xca, FrontendMenu_SelectAlternateRallyStage, -1);
    Menu_AddItemType4(&g_menu0x0081f4d8, 0, 0xcb, FrontendMenu_SelectAlternateRallyStage, -1);
    Menu_AddItemType4(&g_menu0x0081f4d8, 0, 0xcc, FrontendMenu_SelectAlternateRallyStage, -1);
    Menu_AddItemType4(&g_menu0x0081f4d8, 0, 0xcd, FrontendMenu_SelectAlternateRallyStage, -1);
    Menu_AddItemType4(&g_menu0x0081f4d8, 0, 0xce, FrontendMenu_SelectAlternateRallyStage, -1);
    Menu_AddItemType4(&g_menu0x0081f4d8, 0, 0xcf, FrontendMenu_SelectAlternateRallyStage, -1);
    Menu_SetCallbacks(&g_menu0x0081f4d8, FrontendMenu_EnterAlternateRallyStageSelection, (MenuCallback)FrontendMenu_UpdateAlternateRallyStageSelection, (MenuCallback)FrontendMenu_DrawAlternateRallyStageSelection, NULL);
    Menu_ValidateCursor(&g_menu0x0081f4d8, 0);
}

// FUNCTION: CMR2 0x004f8250
void FrontendMenu_BuildQuitTransition(void)
{
    Menu_Init(&g_menu0x00820798, 0, -1, 0, &g_menu0x0081d6d8, NULL, 1, 0, 0);
    Menu_SetCallbacks(&g_menu0x00820798, NULL, (MenuCallback)FrontendMenu_UpdateQuitTransition, NULL, NULL);
    Menu_ValidateCursor(&g_menu0x00820798, 0);
}

// FUNCTION: CMR2 0x004f8500
void FrontendMenu_BuildNetworkPlayerSetup(void)
{
    Menu_Init(&g_menu0x00824fd8, 0, 0x13, 0, &g_menu0x00821c38, NULL, 1, 0, 1);
    Menu_AddItemType3(&g_menu0x00824fd8, 0, -1, 0xb, 0, 1, 0, (MenuItemAction)FrontendNetwork_ToggleStageOption, 0);
    Menu_AddItemType3(&g_menu0x00824fd8, 0, -1, 8, 0, 1, 0, 0, 1);
    Menu_AddItemType4(&g_menu0x00824fd8, 0, 0x67, (MenuItemAction)FrontendNetwork_ApplyPlayerRoleAndJoin, 2);
    Menu_AddItemType1(&g_menu0x00824fd8, 0, 0x1b, 0, 3);
    Menu_SetCallbacks(&g_menu0x00824fd8, FrontendMenu_EnterNetworkPlayerSetup, (MenuCallback)FrontendMenu_UpdateNetworkPlayerSetup, (MenuCallback)FrontendMenu_DrawNetworkPlayerSetup, NULL);
    Menu_ValidateCursor(&g_menu0x00824fd8, 0);
}

// FUNCTION: CMR2 0x004f85b0
void FrontendMenu_BuildNetworkCarSetup(void)
{
    Menu_Init(&g_menu0x0081fc58, 0, 0x13, 0, &g_menu0x00821c38, NULL, 1, 0, 1);
    Menu_AddItemType3(&g_menu0x0081fc58, 0, -1, 8, 0, 1, 0, 0, 0);
    Menu_AddItemType3(&g_menu0x0081fc58, 0, -1, 0xb, 0, 1, 0, 0, 1);
    Menu_AddItemType3(&g_menu0x0081fc58, 0, -1, 4, 0, 1, 0, 0, 2);
    Menu_AddItemType4(&g_menu0x0081fc58, 0, 0x67, (MenuItemAction)FrontendNetwork_ApplyRallyAndJoin, 3);
    Menu_AddItemType1(&g_menu0x0081fc58, 0, 0x1b, 0, 4);
    Menu_SetCallbacks(&g_menu0x0081fc58, (MenuCallback)FrontendMenu_EnterNetworkCarSetup, (MenuCallback)FrontendMenu_UpdateNetworkCarSetup, (MenuCallback)FrontendMenu_DrawNetworkCarSetup, NULL);
    Menu_ValidateCursor(&g_menu0x0081fc58, 0);
}

// FUNCTION: CMR2 0x004f8670
void FrontendMenu_BuildNetworkRallySetup(void)
{
    Menu_Init(&g_menu0x0081b338, 0, 0x13, 0, &g_menu0x00821c38, NULL, 1, 0, 1);
    Menu_AddItemType3(&g_menu0x0081b338, 0, -1, 8, 0, 1, 0, 0, 0);
    Menu_AddItemType3(&g_menu0x0081b338, 0, -1, 0xb, 0, 1, 0, 0, 1);
    Menu_AddItemType3(&g_menu0x0081b338, 0, -1, 0x39, 0, 1, 0, 0, 2);
    Menu_AddItemType4(&g_menu0x0081b338, 0, 0x67, (MenuItemAction)FrontendNetwork_ApplyTwoPlayerRally, 3);
    Menu_AddItemType1(&g_menu0x0081b338, 0, 0x1b, 0, 4);
    Menu_SetCallbacks(&g_menu0x0081b338, FrontendMenu_EnterNetworkRallySetup, (MenuCallback)FrontendMenu_UpdateNetworkRallySetup, (MenuCallback)FrontendMenu_DrawNetworkRallySetup, NULL);
    Menu_ValidateCursor(&g_menu0x0081b338, 0);
}

// FUNCTION: CMR2 0x004f8730
void FrontendMenu_BuildNetworkStageSetup(void)
{
    Menu_Init(&g_menu0x0081e998, 0, 0x13, 0, &g_menu0x00821c38, NULL, 1, 0, 1);
    Menu_AddItemType3(&g_menu0x0081e998, 0, -1, 8, 0, 1, 0, 0, 0);
    Menu_AddItemType3(&g_menu0x0081e998, 0, -1, 0xa, 0, 1, 0, 0, 1);
    Menu_AddItemType4(&g_menu0x0081e998, 0, 0x67, (MenuItemAction)FrontendNetwork_ApplySinglePlayerRally, 2);
    Menu_AddItemType1(&g_menu0x0081e998, 0, 0x1b, 0, 3);
    Menu_SetCallbacks(&g_menu0x0081e998, FrontendMenu_EnterNetworkStageSetup, (MenuCallback)FrontendNetwork_PumpMenuMessages, (MenuCallback)FrontendMenu_DrawNetworkStageSetup, NULL);
    Menu_ValidateCursor(&g_menu0x0081e998, 0);
}

// FUNCTION: CMR2 0x004f87d0
void FrontendMenu_BuildNetworkExtendedStageSetup(void)
{
    Menu_Init(&g_menu0x0081d138, 0, 0x13, 0, &g_menu0x00821c38, NULL, 1, 0, 1);
    Menu_AddItemType3(&g_menu0x0081d138, 0, -1, 8, 0, 1, 0, 0, 0);
    Menu_AddItemType3(&g_menu0x0081d138, 0, -1, 0x3d, 0, 1, 0, 0, 1);
    Menu_AddItemType4(&g_menu0x0081d138, 0, 0x67, (MenuItemAction)FrontendNetwork_ApplyChampionshipRally, 2);
    Menu_AddItemType1(&g_menu0x0081d138, 0, 0x1b, 0, 3);
    Menu_SetCallbacks(&g_menu0x0081d138, (MenuCallback)FrontendMenu_EnterNetworkExtendedStageSetup, (MenuCallback)FrontendNetwork_PumpMenuMessages, (MenuCallback)FrontendMenu_DrawNetworkExtendedStageSetup, NULL);
    Menu_ValidateCursor(&g_menu0x0081d138, 0);
}

// FUNCTION: CMR2 0x004f8870
void FrontendMenu_BuildNetworkNameEntry(void)
{
    Menu_Init(&g_menu0x0081b158, 0, 0x13, 0, &g_menu0x00824c18, NULL, 1, 0, 1);
    Menu_AddItemType4(&g_menu0x0081b158, 0, -1, (MenuItemAction)FrontendNetwork_JoinNamedSession, -1);
    Menu_SetCallbacks(&g_menu0x0081b158, FrontendMenu_EnterNetworkNameEntry, (MenuCallback)FrontendMenu_UpdateNetworkNameEntry, (MenuCallback)FrontendMenu_DrawNetworkNameEntry, NULL);
    Menu_ValidateCursor(&g_menu0x0081b158, 0);
}

// FUNCTION: CMR2 0x004f88e0
void FrontendMenu_BuildNetworkLeaderboard(void)
{
    Menu_Init(&g_menu0x0081bab8, 0, 0x13, 0, &g_menu0x00821c38, NULL, 1, 0, 1);
    Menu_AddItemType3(&g_menu0x0081bab8, 0, -1, 1, 0, 1, 0, (MenuItemAction)FrontendLeaderboard_SelectEntry, 0);
    Menu_AddItemType4(&g_menu0x0081bab8, 0, 0x20, (MenuItemAction)FrontendLeaderboard_AddEntry, 1);
    Menu_AddItemType4(&g_menu0x0081bab8, 0, 0x81, (MenuItemAction)FrontendLeaderboard_RefreshSelection, 2);
    Menu_AddItemType1(&g_menu0x0081bab8, 0, 0x1b, 0, -1);
    Menu_SetCallbacks(&g_menu0x0081bab8, FrontendMenu_EnterNetworkLeaderboard, (MenuCallback)FrontendMenu_UpdateNetworkLeaderboard, (MenuCallback)FrontendMenu_DrawNetworkLeaderboard, NULL);
    Menu_ValidateCursor(&g_menu0x0081bab8, 0);
}

// FUNCTION: CMR2 0x004f95d0
void FrontendMenu_BuildArcadeCarClass(int unused)
{
    Menu_Init(&g_menu0x00828220, 0, 0x1d, 0, &g_menu0x008267e0, NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x00828220, 0, 0xd0, &g_menu0x00827320, (MenuItemAction)FrontendMenu_SelectCarClass, 0);
    Menu_AddItemType2(&g_menu0x00828220, 0, 0xd1, &g_menu0x00827320, (MenuItemAction)FrontendMenu_SelectCarClass, 0);
    Menu_AddItemType2(&g_menu0x00828220, 0, 0xd2, &g_menu0x00827320, (MenuItemAction)FrontendMenu_SelectCarClass, 0);
    Menu_SetCallbacks(&g_menu0x00828220, FrontendMenu_EnterArcadeCarClass, NULL, (MenuCallback)FrontendMenu_DrawCarClass, NULL);
    Menu_ValidateCursor(&g_menu0x00828220, 0);
}

// FUNCTION: CMR2 0x004f9670
void FrontendMenu_BuildChampionshipRouteSelection(void)
{
    Menu_Init(&g_menu0x00826f60, 0, 0x33, 0, &g_menu0x008286e0, NULL, 1, 0, 0);
    Menu_AddItemType2(&g_menu0x00826f60, 0, 0x98, &g_menu0x00826d80, (MenuItemAction)FrontendChampionship_SelectRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00826f60, 0, 0x99, &g_menu0x00826d80, (MenuItemAction)FrontendChampionship_SelectRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00826f60, 0, 0x9a, &g_menu0x00826d80, (MenuItemAction)FrontendChampionship_SelectRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00826f60, 0, 0x9b, &g_menu0x00826d80, (MenuItemAction)FrontendChampionship_SelectRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00826f60, 0, 0x9c, &g_menu0x00826d80, (MenuItemAction)FrontendChampionship_SelectRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00826f60, 0, 0x9d, &g_menu0x00826d80, (MenuItemAction)FrontendChampionship_SelectRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00826f60, 0, 0x9e, &g_menu0x00826d80, (MenuItemAction)FrontendChampionship_SelectRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00826f60, 0, 0x9f, &g_menu0x00826d80, (MenuItemAction)FrontendChampionship_SelectRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00826f60, 0, 0xa0, &g_menu0x00826d80, (MenuItemAction)FrontendChampionship_SelectRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00826f60, 0, 0xa1, &g_menu0x00826d80, (MenuItemAction)FrontendChampionship_SelectRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00826f60, 0, 0xa2, &g_menu0x00826d80, (MenuItemAction)FrontendChampionship_SelectRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00826f60, 0, 0xa3, &g_menu0x00826d80, (MenuItemAction)FrontendChampionship_SelectRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00826f60, 0, 0xa4, &g_menu0x00826d80, (MenuItemAction)FrontendChampionship_SelectRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00826f60, 0, 0xa5, &g_menu0x00826d80, (MenuItemAction)FrontendChampionship_SelectRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00826f60, 0, 0xa6, &g_menu0x00826d80, (MenuItemAction)FrontendChampionship_SelectRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00826f60, 0, 0xa7, &g_menu0x00826d80, (MenuItemAction)FrontendChampionship_SelectRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00826f60, 0, 0xa8, &g_menu0x00826d80, (MenuItemAction)FrontendChampionship_SelectRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00826f60, 0, 0xa9, &g_menu0x00826d80, (MenuItemAction)FrontendChampionship_SelectRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00826f60, 0, 0xaa, &g_menu0x00826d80, (MenuItemAction)FrontendChampionship_SelectRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00826f60, 0, 0xab, &g_menu0x00826d80, (MenuItemAction)FrontendChampionship_SelectRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00826f60, 0, 0xac, &g_menu0x00826d80, (MenuItemAction)FrontendChampionship_SelectRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00826f60, 0, 0xad, &g_menu0x00826d80, (MenuItemAction)FrontendChampionship_SelectRouteEntry, -1);
    Menu_SetCallbacks(&g_menu0x00826f60, FrontendMenu_InitRouteScroller, (MenuCallback)FrontendScroller_UpdateChampionshipEntry, (MenuCallback)FrontendMenu_DrawPaletteSelection, NULL);
    Menu_ValidateCursor(&g_menu0x00826f60, 0);
}

// FUNCTION: CMR2 0x004f9940
void FrontendMenu_BuildChampionshipEntry(void)
{
    Menu_Init(&g_menu0x00826d80, 0, 0x97, 0, &g_menu0x00826f60, NULL, 1, 0, 1);
    Menu_AddItemType3(&g_menu0x00826d80, 0, 0x88, 2, 0, 0, 0, (MenuItemAction)FrontendChampionship_ToggleRouteEntry, 0x88);
    Menu_SetCallbacks(&g_menu0x00826d80, (MenuCallback)FrontendMenu_UpdateChampionshipEntry, (MenuCallback)FrontendScroller_UpdateChampionshipEntry, (MenuCallback)FrontendMenu_DrawChampionshipEntry, NULL);
    Menu_ValidateCursor(&g_menu0x00826d80, 0);
}

// FUNCTION: CMR2 0x004f99b0
void FrontendMenu_BuildAlternateChampionshipRouteSelection(void)
{
    Menu_Init(&g_menu0x00828500, 0, 0x33, 0, &g_menu0x00827c80, NULL, 1, 0, 0);
    Menu_AddItemType2(&g_menu0x00828500, 0, 0x98, &g_menu0x00827140, (MenuItemAction)FrontendChampionship_UpdateAlternateRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00828500, 0, 0x99, &g_menu0x00827140, (MenuItemAction)FrontendChampionship_UpdateAlternateRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00828500, 0, 0x9a, &g_menu0x00827140, (MenuItemAction)FrontendChampionship_UpdateAlternateRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00828500, 0, 0x9b, &g_menu0x00827140, (MenuItemAction)FrontendChampionship_UpdateAlternateRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00828500, 0, 0x9c, &g_menu0x00827140, (MenuItemAction)FrontendChampionship_UpdateAlternateRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00828500, 0, 0x9d, &g_menu0x00827140, (MenuItemAction)FrontendChampionship_UpdateAlternateRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00828500, 0, 0x9e, &g_menu0x00827140, (MenuItemAction)FrontendChampionship_UpdateAlternateRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00828500, 0, 0x9f, &g_menu0x00827140, (MenuItemAction)FrontendChampionship_UpdateAlternateRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00828500, 0, 0xa0, &g_menu0x00827140, (MenuItemAction)FrontendChampionship_UpdateAlternateRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00828500, 0, 0xa1, &g_menu0x00827140, (MenuItemAction)FrontendChampionship_UpdateAlternateRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00828500, 0, 0xa2, &g_menu0x00827140, (MenuItemAction)FrontendChampionship_UpdateAlternateRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00828500, 0, 0xa3, &g_menu0x00827140, (MenuItemAction)FrontendChampionship_UpdateAlternateRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00828500, 0, 0xa4, &g_menu0x00827140, (MenuItemAction)FrontendChampionship_UpdateAlternateRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00828500, 0, 0xa5, &g_menu0x00827140, (MenuItemAction)FrontendChampionship_UpdateAlternateRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00828500, 0, 0xa6, &g_menu0x00827140, (MenuItemAction)FrontendChampionship_UpdateAlternateRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00828500, 0, 0xa7, &g_menu0x00827140, (MenuItemAction)FrontendChampionship_UpdateAlternateRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00828500, 0, 0xa8, &g_menu0x00827140, (MenuItemAction)FrontendChampionship_UpdateAlternateRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00828500, 0, 0xa9, &g_menu0x00827140, (MenuItemAction)FrontendChampionship_UpdateAlternateRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00828500, 0, 0xaa, &g_menu0x00827140, (MenuItemAction)FrontendChampionship_UpdateAlternateRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00828500, 0, 0xab, &g_menu0x00827140, (MenuItemAction)FrontendChampionship_UpdateAlternateRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00828500, 0, 0xac, &g_menu0x00827140, (MenuItemAction)FrontendChampionship_UpdateAlternateRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00828500, 0, 0xad, &g_menu0x00827140, (MenuItemAction)FrontendChampionship_UpdateAlternateRouteEntry, -1);
    Menu_SetCallbacks(&g_menu0x00828500, FrontendMenu_InitRouteScroller, (MenuCallback)FrontendScroller_UpdateChampionshipEntry, (MenuCallback)FrontendMenu_DrawPaletteSelection, NULL);
    Menu_ValidateCursor(&g_menu0x00828500, 0);
}

// FUNCTION: CMR2 0x004f9c80
void FrontendMenu_BuildAlternateChampionshipEntry(void)
{
    Menu_Init(&g_menu0x00827140, 0, 0x97, 0, &g_menu0x00828500, NULL, 1, 0, 1);
    Menu_AddItemType3(&g_menu0x00827140, 0, 0x88, 2, 0, 0, 0, (MenuItemAction)FrontendChampionship_ApplySharedRouteEntry, 0x88);
    Menu_SetCallbacks(&g_menu0x00827140, (MenuCallback)FrontendMenu_UpdateChampionshipEntry, (MenuCallback)FrontendScroller_UpdateChampionshipEntry, (MenuCallback)FrontendMenu_DrawChampionshipEntry, NULL);
    Menu_ValidateCursor(&g_menu0x00827140, 0);
}

// FUNCTION: CMR2 0x004f9cf0
void FrontendMenu_BuildArcadeChampionshipRouteSelection(void)
{
    Menu_Init(&g_menu0x00827e60, 0, 0x33, 0, &g_menu0x008278c0, NULL, 1, 0, 0);
    Menu_AddItemType2(&g_menu0x00827e60, 0, 0x98, &g_menu0x00827500, (MenuItemAction)FrontendChampionship_UpdateArcadeRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00827e60, 0, 0x99, &g_menu0x00827500, (MenuItemAction)FrontendChampionship_UpdateArcadeRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00827e60, 0, 0x9a, &g_menu0x00827500, (MenuItemAction)FrontendChampionship_UpdateArcadeRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00827e60, 0, 0x9b, &g_menu0x00827500, (MenuItemAction)FrontendChampionship_UpdateArcadeRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00827e60, 0, 0x9c, &g_menu0x00827500, (MenuItemAction)FrontendChampionship_UpdateArcadeRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00827e60, 0, 0x9d, &g_menu0x00827500, (MenuItemAction)FrontendChampionship_UpdateArcadeRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00827e60, 0, 0x9e, &g_menu0x00827500, (MenuItemAction)FrontendChampionship_UpdateArcadeRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00827e60, 0, 0x9f, &g_menu0x00827500, (MenuItemAction)FrontendChampionship_UpdateArcadeRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00827e60, 0, 0xa0, &g_menu0x00827500, (MenuItemAction)FrontendChampionship_UpdateArcadeRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00827e60, 0, 0xa1, &g_menu0x00827500, (MenuItemAction)FrontendChampionship_UpdateArcadeRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00827e60, 0, 0xa2, &g_menu0x00827500, (MenuItemAction)FrontendChampionship_UpdateArcadeRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00827e60, 0, 0xa3, &g_menu0x00827500, (MenuItemAction)FrontendChampionship_UpdateArcadeRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00827e60, 0, 0xa4, &g_menu0x00827500, (MenuItemAction)FrontendChampionship_UpdateArcadeRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00827e60, 0, 0xa5, &g_menu0x00827500, (MenuItemAction)FrontendChampionship_UpdateArcadeRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00827e60, 0, 0xa6, &g_menu0x00827500, (MenuItemAction)FrontendChampionship_UpdateArcadeRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00827e60, 0, 0xa7, &g_menu0x00827500, (MenuItemAction)FrontendChampionship_UpdateArcadeRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00827e60, 0, 0xa8, &g_menu0x00827500, (MenuItemAction)FrontendChampionship_UpdateArcadeRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00827e60, 0, 0xa9, &g_menu0x00827500, (MenuItemAction)FrontendChampionship_UpdateArcadeRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00827e60, 0, 0xaa, &g_menu0x00827500, (MenuItemAction)FrontendChampionship_UpdateArcadeRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00827e60, 0, 0xab, &g_menu0x00827500, (MenuItemAction)FrontendChampionship_UpdateArcadeRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00827e60, 0, 0xac, &g_menu0x00827500, (MenuItemAction)FrontendChampionship_UpdateArcadeRouteEntry, -1);
    Menu_AddItemType2(&g_menu0x00827e60, 0, 0xad, &g_menu0x00827500, (MenuItemAction)FrontendChampionship_UpdateArcadeRouteEntry, -1);
    Menu_SetCallbacks(&g_menu0x00827e60, FrontendMenu_InitRouteScroller, (MenuCallback)FrontendScroller_UpdateChampionshipEntry, (MenuCallback)FrontendMenu_DrawPaletteSelection, NULL);
    Menu_ValidateCursor(&g_menu0x00827e60, 0);
}

// FUNCTION: CMR2 0x004f9fc0
void FrontendMenu_BuildArcadeChampionshipEntry(void)
{
    Menu_Init(&g_menu0x00827500, 0, 0x97, 0, &g_menu0x00827e60, NULL, 1, 0, 1);
    Menu_AddItemType3(&g_menu0x00827500, 0, 0x88, 2, 0, 0, 0, (MenuItemAction)FrontendChampionship_ToggleArcadeRouteEntry, 0x88);
    Menu_SetCallbacks(&g_menu0x00827500, (MenuCallback)FrontendMenu_UpdateChampionshipEntry, (MenuCallback)FrontendScroller_UpdateChampionshipEntry, (MenuCallback)FrontendMenu_DrawChampionshipEntry, NULL);
    Menu_ValidateCursor(&g_menu0x00827500, 0);
}

// FUNCTION: CMR2 0x004fa030
void FrontendMenu_BuildArcadeChampionshipTransmission(void)
{
    Menu_Init(&g_menu0x00826ba0, 0, 0x26, 0, &g_menu0x00826d80, NULL, 1, 0, 1);
    Menu_AddItemType4(&g_menu0x00826ba0, 0, 0xe9, (MenuItemAction)FrontendMenu_SelectArcadeChampionshipTransmission, 1);
    Menu_AddItemType4(&g_menu0x00826ba0, 0, 0xe8, (MenuItemAction)FrontendMenu_SelectArcadeChampionshipTransmission, 0);
    Menu_SetCallbacks(&g_menu0x00826ba0, FrontendMenu_EnterArcadeChampionshipTransmission, (MenuCallback)FrontendMenu_UpdateArcadeChampionshipTransmission, (MenuCallback)FrontendMenu_DrawArcadeChampionshipTransmission, NULL);
    Menu_ValidateCursor(&g_menu0x00826ba0, 0);
}

// FUNCTION: CMR2 0x004fa0b0
void FrontendMenu_BuildArcadeRallySelection(void)
{
    Menu_Init(&g_menu0x00827320, 0, 0x25, 0, &g_menu0x00828220, NULL, 1, 0, 0);
    Menu_AddItemType4(&g_menu0x00827320, 0, 0xfa, FrontendMenu_SelectArcadeRallyRow, -1);
    Menu_AddItemType4(&g_menu0x00827320, 0, 0xfb, FrontendMenu_SelectArcadeRallyRow, -1);
    Menu_AddItemType4(&g_menu0x00827320, 0, 0xfc, FrontendMenu_SelectArcadeRallyRow, -1);
    Menu_AddItemType4(&g_menu0x00827320, 0, 0xfd, FrontendMenu_SelectArcadeRallyRow, -1);
    Menu_AddItemType4(&g_menu0x00827320, 0, 0xfe, FrontendMenu_SelectArcadeRallyRow, -1);
    Menu_AddItemType4(&g_menu0x00827320, 0, 0xff, FrontendMenu_SelectArcadeRallyRow, -1);
    Menu_AddItemType4(&g_menu0x00827320, 0, 0x100, FrontendMenu_SelectArcadeRallyRow, -1);
    Menu_AddItemType4(&g_menu0x00827320, 0, 0x101, FrontendMenu_SelectArcadeRallyRow, -1);
    Menu_SetCallbacks(&g_menu0x00827320, FrontendScroller_InitArcadeRally, (MenuCallback)FrontendScroller_UpdateArcadeRally, (MenuCallback)FrontendMenu_DrawArcadeRallySelection, NULL);
    Menu_ValidateCursor(&g_menu0x00827320, 0);
}

// FUNCTION: CMR2 0x004fa1c0
void FrontendMenu_BuildArcadeChampionshipRallySelection(void)
{
    Menu_Init(&g_menu0x00826420, 0, 0x25, 0, &g_menu0x00827500, NULL, 1, 0, 0);
    Menu_AddItemType4(&g_menu0x00826420, 0, 0xfa, FrontendMenu_SelectArcadeRallyRow, -1);
    Menu_AddItemType4(&g_menu0x00826420, 0, 0xfb, FrontendMenu_SelectArcadeRallyRow, -1);
    Menu_AddItemType4(&g_menu0x00826420, 0, 0xfc, FrontendMenu_SelectArcadeRallyRow, -1);
    Menu_AddItemType4(&g_menu0x00826420, 0, 0xfd, FrontendMenu_SelectArcadeRallyRow, -1);
    Menu_AddItemType4(&g_menu0x00826420, 0, 0xfe, FrontendMenu_SelectArcadeRallyRow, -1);
    Menu_AddItemType4(&g_menu0x00826420, 0, 0xff, FrontendMenu_SelectArcadeRallyRow, -1);
    Menu_AddItemType4(&g_menu0x00826420, 0, 0x100, FrontendMenu_SelectArcadeRallyRow, -1);
    Menu_AddItemType4(&g_menu0x00826420, 0, 0x101, FrontendMenu_SelectArcadeRallyRow, -1);
    Menu_SetCallbacks(&g_menu0x00826420, FrontendScroller_InitArcadeRally, (MenuCallback)FrontendScroller_UpdateArcadeRally, (MenuCallback)FrontendMenu_DrawArcadeRallySelection, NULL);
    Menu_ValidateCursor(&g_menu0x00826420, 0);
}

// FUNCTION: CMR2 0x004f8290
Menu *FrontendMenu_GetInitialMenu(BYTE param1)
{
    if (param1 != 0 && CGameInfo::GetGameRegion() != 3 && CGameInfo::GetGameRegion() != 2)
        return &g_menu0x008221d8;
    return &g_menu0x0081d6d8;
}

// FUNCTION: CMR2 0x004f8330
Menu *FrontendMenu_GetRallyStartTransition(void)
{
    return &g_menu0x008214b8;
}

// FUNCTION: CMR2 0x004f8990
Menu *FrontendMenu_GetQuitTransition(void)
{
    return &g_menu0x00820798;
}

// FUNCTION: CMR2 0x004fa330
Menu *FrontendMenu_GetArcadeChampionshipTransmission(void)
{
    return &g_menu0x00826ba0;
}

// FUNCTION: CMR2 0x004f8410
Menu *FrontendMenu_GetMain(void)
{
    return &g_menu0x0081d6d8;
}

// FUNCTION: CMR2 0x004f83a0
Menu *FrontendMenu_GetMultiplayerProfile(void)
{
    return &g_menu0x008241b8;
}

// FUNCTION: CMR2 0x004f8450
Menu *FrontendMenu_GetNetworkSessionBrowser(void)
{
    return &g_menu0x00824c18;
}

// FUNCTION: CMR2 0x004f8470
Menu *FrontendMenu_GetNetworkSessionSetup(void)
{
    return &g_menu0x0081e218;
}

// FUNCTION: CMR2 0x004f8360
Menu *FrontendMenu_GetAlternateRallyStageSelection(void)
{
    return &g_menu0x0081f4d8;
}

// FUNCTION: CMR2 0x004f82c0
Menu *FrontendMenu_GetOptions(void)
{
    return &g_menu0x0081f118;
}

// FUNCTION: CMR2 0x004f82d0
Menu *FrontendMenu_GetLanguage(void)
{
    return &g_menu0x008221d8;
}

// FUNCTION: CMR2 0x004f82e0
Menu *FrontendMenu_GetSingleRallyDifficulty(void)
{
    return &g_menu0x0081d318;
}

// FUNCTION: CMR2 0x004f82f0
Menu *FrontendMenu_GetChampionshipDifficulty(void)
{
    return &g_menu0x00820978;
}

// FUNCTION: CMR2 0x004f8300
Menu *FrontendMenu_GetTimeTrialDifficulty(void)
{
    return &g_menu0x0081f2f8;
}

// FUNCTION: CMR2 0x004f8310
Menu *FrontendMenu_GetMultiplayerDifficulty(void)
{
    return &g_menu0x00824678;
}

// FUNCTION: CMR2 0x004f8320
Menu *FrontendMenu_GetMultiplayerExtendedDifficulty(void)
{
    return &g_menu0x0081e5d8;
}

// FUNCTION: CMR2 0x004f8340
Menu *FrontendMenu_GetRallySelection(void)
{
    return &g_menu0x0081eb78;
}

// FUNCTION: CMR2 0x004f8350
Menu *FrontendMenu_GetRallyStageSelection(void)
{
    return &g_menu0x0081e3f8;
}

// FUNCTION: CMR2 0x004f8370
Menu *FrontendMenu_GetSingleRallyTransmission(void)
{
    return &g_menu0x008230d8;
}

// FUNCTION: CMR2 0x004f8380
Menu *FrontendMenu_GetChampionshipTransmission(void)
{
    return &g_menu0x00824858;
}

// FUNCTION: CMR2 0x004f8390
Menu *FrontendMenu_GetTimeTrialTransmission(void)
{
    return &g_menu0x008251b8;
}

// FUNCTION: CMR2 0x004f83b0
Menu *FrontendMenu_GetMultiplayerStageSelection(void)
{
    return &g_menu0x008203d8;
}

// FUNCTION: CMR2 0x004f83c0
Menu *FrontendMenu_GetProfileNameEntry(void)
{
    return &g_menu0x00824a38;
}

// FUNCTION: CMR2 0x004f83d0
Menu *FrontendMenu_GetProfileRenameEntry(void)
{
    return &g_menu0x0081d8b8;
}

// FUNCTION: CMR2 0x004f83e0
Menu *FrontendMenu_GetProfileDateEntry(void)
{
    return &g_menu0x0081bc98;
}

// FUNCTION: CMR2 0x004f83f0
Menu *FrontendMenu_GetCarSetup(void)
{
    return &g_menu0x00821e18;
}

// FUNCTION: CMR2 0x004f8400
Menu *FrontendMenu_GetPaletteSelection(void)
{
    return &g_menu0x0081cf58;
}

// FUNCTION: CMR2 0x004f8420
Menu *FrontendMenu_GetMultiplayerRaceSettings(void)
{
    return &g_menu0x00823858;
}

// FUNCTION: CMR2 0x004f8430
Menu *FrontendMenu_GetPaletteConfirmation(void)
{
    return &g_menu0x00822d18;
}

// FUNCTION: CMR2 0x004f8440
Menu *FrontendMenu_GetNetworkConnection(void)
{
    return &g_menu0x00822598;
}

// FUNCTION: CMR2 0x004f8460
Menu *FrontendMenu_GetNetworkSessionDetails(void)
{
    return &g_menu0x00821c38;
}

// FUNCTION: CMR2 0x004f8480
Menu *FrontendMenu_GetRallyProfile(void)
{
    return &g_menu0x008212d8;
}

// FUNCTION: CMR2 0x004f8490
Menu *FrontendMenu_GetProfileActions(void)
{
    return &g_menu0x0081c058;
}

// FUNCTION: CMR2 0x004f84a0
Menu *FrontendMenu_GetRallyModes(void)
{
    return &g_menu0x00823a38;
}

// FUNCTION: CMR2 0x004f84b0
Menu *FrontendMenu_GetNetworkPlayerSetup(void)
{
    return &g_menu0x00824fd8;
}

// FUNCTION: CMR2 0x004f84c0
Menu *FrontendMenu_GetNetworkCarSetup(void)
{
    return &g_menu0x0081fc58;
}

// FUNCTION: CMR2 0x004f84d0
Menu *FrontendMenu_GetNetworkRallySetup(void)
{
    return &g_menu0x0081b338;
}

// FUNCTION: CMR2 0x004f84e0
Menu *FrontendMenu_GetNetworkStageSetup(void)
{
    return &g_menu0x0081e998;
}

// FUNCTION: CMR2 0x004f84f0
Menu *FrontendMenu_GetNetworkExtendedStageSetup(void)
{
    return &g_menu0x0081d138;
}

// FUNCTION: CMR2 0x004f88d0
Menu *FrontendMenu_GetNetworkNameEntry(void)
{
    return &g_menu0x0081b158;
}

// FUNCTION: CMR2 0x004f89a0
Menu *FrontendMenu_GetNetworkStageTimes(void)
{
    return &g_menu0x00824df8;
}

// FUNCTION: CMR2 0x004f9360
Menu *FrontendMenu_GetNetworkMessage(void)
{
    return &g_menu0x00826140;
}

// FUNCTION: CMR2 0x004fa2d0
Menu *FrontendMenu_GetArcade(void)
{
    return &g_menu0x008267e0;
}

// FUNCTION: CMR2 0x004fa2e0
Menu *FrontendMenu_GetArcadeGameType(void)
{
    return &g_menu0x00826600;
}

// FUNCTION: CMR2 0x004fa2f0
Menu *FrontendMenu_GetArcadeCarClass(void)
{
    return &g_menu0x00828220;
}

// FUNCTION: CMR2 0x004fa300
Menu *FrontendMenu_GetChampionshipRouteSelection(void)
{
    return &g_menu0x00826f60;
}

// FUNCTION: CMR2 0x004fa310
Menu *FrontendMenu_GetAlternateChampionshipRouteSelection(void)
{
    return &g_menu0x00828500;
}

// FUNCTION: CMR2 0x004fa320
Menu *FrontendMenu_GetArcadeChampionshipRouteSelection(void)
{
    return &g_menu0x00827e60;
}

// FUNCTION: CMR2 0x004fa340
Menu *FrontendMenu_GetArcadeRallySelection(void)
{
    return &g_menu0x00827320;
}

// FUNCTION: CMR2 0x004fa350
Menu *FrontendMenu_GetArcadeChampionshipRallySelection(void)
{
    return &g_menu0x00826420;
}

// FUNCTION: CMR2 0x004fa360
Menu *FrontendMenu_GetQuickRaceSettings(void)
{
    return &g_menu0x00828040;
}

// FUNCTION: CMR2 0x004fa4c0
Menu *FrontendMenu_GetQuickRaceAdvancedSettings(void)
{
    return &g_menu0x00827aa0;
}

// FUNCTION: CMR2 0x004fa4f0
Menu *FrontendMenu_GetControls(void)
{
    return &g_menu0x00828c80;
}

// FUNCTION: CMR2 0x004fa500
Menu *FrontendMenu_GetDeviceCalibration(void)
{
    return &g_menu0x00828aa0;
}

// FUNCTION: CMR2 0x004fa510
Menu *FrontendMenu_GetDeviceBindings(void)
{
    return &g_menu0x008288c0;
}

// FUNCTION: CMR2 0x004fa520
Menu *FrontendMenu_GetPadSettings(void)
{
    return &g_menu0x00828e60;
}

// FUNCTION: CMR2 0x004fa530
Menu *FrontendMenu_GetDeviceConfiguration(void)
{
    return &g_menu0x00829140;
}

void FrontendControls_DrawDeviceList(Menu *pMenu);

// Controls menu: two device entries and "back".
// FUNCTION: CMR2 0x004fa540
void FrontendMenu_BuildControls(void)
{
    Menu_Init(&g_menu0x00828c80, 0, 0x18, 0, FrontendMenu_GetOptions(), NULL, 1, 0, 1);
    Menu_AddItemType2(&g_menu0x00828c80, 0, -1, &g_menu0x00829140, (MenuItemAction)FrontendControls_SelectDevice, 0);
    Menu_AddItemType2(&g_menu0x00828c80, 0, -1, &g_menu0x00829140, (MenuItemAction)FrontendControls_SelectDevice, 0);
    Menu_AddItemType1(&g_menu0x00828c80, 0, 0x67, 0, 0);
    Menu_SetCallbacks(&g_menu0x00828c80, (MenuCallback)FrontendControls_ShowBindingEntries, NULL, (MenuCallback)FrontendControls_DrawDeviceList, NULL);
    Menu_ValidateCursor(&g_menu0x00828c80, 0);
}

void FrontendControls_BeginBindingCapture(Menu *pMenu, int param);
void FrontendControls_EnterDeviceBindings(Menu *pMenu, char param);
void FrontendControls_UpdateBindingCapture(Menu *pMenu);
void FrontendControls_DrawDeviceBindings(Menu *pMenu);
void FrontendControls_LeaveDeviceBindings(Menu *pMenu, char back);

// Device page of the controls menu: the 10 bindings and "back".
// FUNCTION: CMR2 0x004fa5d0
void FrontendMenu_BuildDeviceBindings(void)
{
    int i;

    Menu_Init(&g_menu0x008288c0, 0, 0x65, 0, &g_menu0x00828c80, NULL, 1, 0, 1);
    i = 0;
    do {
        Menu_AddItemType4(&g_menu0x008288c0, 0, i + 0x6b, (MenuItemAction)FrontendControls_BeginBindingCapture, i);
        i++;
    } while (i < 10);
    Menu_AddItemType2(&g_menu0x008288c0, 0, 0x67, &g_menu0x00828c80, 0, 0);
    Menu_SetCallbacks(&g_menu0x008288c0, (MenuCallback)FrontendControls_EnterDeviceBindings, (MenuCallback)FrontendControls_UpdateBindingCapture,
                      (MenuCallback)FrontendControls_DrawDeviceBindings, (MenuCallback)FrontendControls_LeaveDeviceBindings);
    Menu_ValidateCursor(&g_menu0x008288c0, 0);
}

void FrontendControls_BeginCalibration(Menu *pMenu, int param);
void FrontendControls_EnterCalibration(Menu *pMenu, int param);
void FrontendControls_UpdateAxisCalibration(Menu *pMenu);
void FrontendControls_DrawCalibration(Menu *pMenu);
void FrontendControls_EnterPadSettings(Menu *pMenu, int param);
void FrontendControls_DrawPadSettings(Menu *pMenu);
void FrontendControls_LeavePadSettings(Menu *pMenu, char back);
void FrontendControls_EnterDeviceConfiguration(Menu *pMenu, int param);
void FrontendControls_UpdateDeviceConfiguration(Menu *pMenu);
void FrontendControls_DrawDeviceConfiguration(Menu *pMenu);
void FrontendControls_LeaveDeviceConfiguration(Menu *pMenu, char back);

// Calibration page of the controls menu: one entry per axis and "back".
// FUNCTION: CMR2 0x004fa650
void FrontendMenu_BuildDeviceCalibration(void)
{
    int i;

    Menu_Init(&g_menu0x00828aa0, 0, 0x75, 0, &g_menu0x008288c0, NULL, 1, 0, 1);
    i = 0;
    do {
        Menu_AddItemType4(&g_menu0x00828aa0, 0, -1, (MenuItemAction)FrontendControls_BeginCalibration, i);
        i++;
    } while (i < 8);
    Menu_AddItemType2(&g_menu0x00828aa0, 0, 0x67, &g_menu0x00828c80, 0, 0);
    Menu_SetCallbacks(&g_menu0x00828aa0, (MenuCallback)FrontendControls_EnterCalibration, (MenuCallback)FrontendControls_UpdateAxisCalibration,
                      (MenuCallback)FrontendControls_DrawCalibration, NULL);
    Menu_ValidateCursor(&g_menu0x00828aa0, 0);
}

// Pad page of the controls menu: two sensitivities, vibration and "back".
// FUNCTION: CMR2 0x004fa6d0
void FrontendMenu_BuildPadSettings(void)
{
    Menu_Init(&g_menu0x00828e60, 0, 0x75, 0, &g_menu0x008288c0, NULL, 1, 0, 1);
    Menu_AddItemType3(&g_menu0x00828e60, 0, 0x7a, 0xb, 0, 0, 0, 0, 0);
    Menu_AddItemType3(&g_menu0x00828e60, 0, 0x7b, 0xb, 0, 0, 0, 0, 1);
    Menu_AddItemType3(&g_menu0x00828e60, 0, 0x1a8, 2, 0, 0, 0, 0, 2);
    Menu_AddItemType2(&g_menu0x00828e60, 0, 0x67, &g_menu0x00828c80, 0, 3);
    Menu_SetCallbacks(&g_menu0x00828e60, (MenuCallback)FrontendControls_EnterPadSettings, NULL, (MenuCallback)FrontendControls_DrawPadSettings,
                      (MenuCallback)FrontendControls_LeavePadSettings);
    Menu_ValidateCursor(&g_menu0x00828e60, 0);
}

// Device settings page of the controls menu: configuration of the slot
// (one of the connected devices) and its options.
// FUNCTION: CMR2 0x004fa780
void FrontendMenu_BuildDeviceConfiguration(void)
{
    Menu_Init(&g_menu0x00829140, 0, 0x65, 0, &g_menu0x00828c80, NULL, 1, 0, 1);
    Menu_AddItemType3(&g_menu0x00829140, 0, -1, (BYTE)CInput::CountAttachedInputDevices(), 0, 0, 0, 0, 0);
    Menu_AddItemType3(&g_menu0x00829140, 0, 0x68, 2, 0, 0, 0, 0, 1);
    Menu_AddItemType3(&g_menu0x00829140, 0, 0x68, 2, 0, 0, 0, 0, 2);
    Menu_AddItemType3(&g_menu0x00829140, 0, 0x6a, 2, 0, 0, 0, 0, 3);
    Menu_AddItemType3(&g_menu0x00829140, 0, 0x20b, 2, 0, 0, 0, 0, 4);
    Menu_AddItemType3(&g_menu0x00829140, 0, 0x20c, 2, 0, 0, 0, 0, 5);
    Menu_AddItemType2(&g_menu0x00829140, 0, 0x67, &g_menu0x008288c0, 0, 6);
    Menu_SetCallbacks(&g_menu0x00829140, (MenuCallback)FrontendControls_EnterDeviceConfiguration, (MenuCallback)FrontendControls_UpdateDeviceConfiguration,
                      (MenuCallback)FrontendControls_DrawDeviceConfiguration, (MenuCallback)FrontendControls_LeaveDeviceConfiguration);
    Menu_ValidateCursor(&g_menu0x00829140, 0);
}

// Builds every page of the controls menu.
// FUNCTION: CMR2 0x004fa4d0
void FrontendMenu_BuildAllControlsPages(void)
{
    FrontendMenu_BuildControls();
    FrontendMenu_BuildDeviceBindings();
    FrontendMenu_BuildDeviceCalibration();
    CInput::RefreshControllerConfigurations();
    FrontendMenu_BuildDeviceConfiguration();
    FrontendMenu_BuildPadSettings();
}
