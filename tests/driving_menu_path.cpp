#include "port/types.h"
#include "Input.h"
#include "Menu.h"
#include "port/sys.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <chrono>
#include <cstdio>
#include <cstring>
extern void FrontendMenu_BuildControls(),FrontendMenu_BuildDeviceConfiguration(),FrontendMenu_BuildDeviceBindings();
extern Menu *FrontendMenu_GetControls(),*FrontendMenu_GetDeviceConfiguration(),*FrontendMenu_GetDeviceBindings();
extern Menu *Menu_Update(Menu*,unsigned);
extern void Menu_QueueDefaultAction();
extern ControllerData g_controlsCopy[6];
extern int g_controlsSelectedSlot;
struct ControlsDeviceSelection{unsigned short slots[8],lastDevice;};extern ControlsDeviceSelection g_controlsDeviceSelection;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 auto old=std::filesystem::current_path();auto tmp=std::filesystem::temp_directory_path()/("cmr-wiper-row-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(tmp);std::filesystem::current_path(tmp);std::ofstream("opencmr2.ini")<<"[video]\n";

 FrontendMenu_BuildControls();FrontendMenu_BuildDeviceConfiguration();FrontendMenu_BuildDeviceBindings();
 Menu *controls=FrontendMenu_GetControls(),*config=FrontendMenu_GetDeviceConfiguration(),*bindings=FrontendMenu_GetDeviceBindings();
 CHECK(controls->itemCount==3&&controls->items[0].pSubMenu==config&&controls->items[1].pSubMenu==config);
 CHECK(!strcmp(config->items[6].text,"Rebind driving controls")&&config->items[6].pSubMenu==bindings);
 CHECK(!strcmp(bindings->items[10].text,"wipers"));
 CHECK(!strcmp(bindings->items[11].text,"Save bindings")&&bindings->items[11].pSubMenu==controls);
 // Real menu select follows the exposed link, not an invented UI shortcut.
 // Its device-poll callback depends on an active SDL window, so isolate just
 // construction+navigation here; capture actual function is tested separately.
 config->pfnCallback1=nullptr;config->pfnCallback3=nullptr;config->cursor=6;
 CHECK(Menu_Update(config,0x10)==bindings);
 Menu_SetActionLatch(0);Menu_QueueDefaultAction();
 // Actual enter+save callbacks on a synthetic keyboard preserve edited keys.
 CInput::m_availableDevices[0].type=1;g_controlsSelectedSlot=0;g_controlsDeviceSelection.slots[0]=0;
 bindings->pfnCallback0(bindings,0);for(int i=0;i<9;i++)CHECK(bindings->items[i].enabled&&bindings->items[i].visible);
 g_controlsCopy[0]=CInput::m_controllerInfo[0];g_controlsCopy[0].keys[2]=0x1e;
 bindings->cursor=11;CHECK(Menu_Update(bindings,0x10)==controls);CHECK(CInput::m_controllerInfo[0].keys[2]==0x1e);
 // Both player device pages reach same global extension capture, not a
 // tenth byte written into the packed nine-key save record.
 for(int slot=0;slot<2;slot++){
  g_controlsSelectedSlot=slot;g_controlsDeviceSelection.slots[slot]=0;bindings->pfnCallback0(bindings,0);
  CHECK(bindings->items[10].enabled&&bindings->items[10].visible);
  memset(CInput::m_keyboardState,0,256);Menu_SetActionLatch(0);Menu_QueueDefaultAction();bindings->cursor=10;
  CHECK(Menu_Update(bindings,0x10)==nullptr);CHECK(!bindings->flag2);
  bindings->pfnCallback1(bindings,0);CInput::m_keyboardState[0x13]=0x80;bindings->pfnCallback1(bindings,0);
  CHECK(bindings->flag2&&Sys_GetOption("video.windscreen_wiper_key",17)==0x13);
 }
 std::filesystem::current_path(old);std::filesystem::remove_all(tmp);
 puts("driving menu actual construction/rebind select/save path pass; native pixels/window polling separate");return 0;
}
