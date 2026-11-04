#include "platform/key_remapping.h"
#include "Input.h"
#include "GameInfo.h"
#include "Menu.h"
#include "port/sys.h"
#include <cstdio>
#include <cstring>
extern ControllerData g_controlsCopy[6];
extern int g_controlsSelectedSlot, g_controlsBindingCaptureState, g_unk0x0082a7e0;
struct ControlsDeviceSelection {unsigned short slots[8],lastDevice;};
extern ControlsDeviceSelection g_controlsDeviceSelection;
extern void FrontendControls_ClearDuplicateBinding(ControllerData*,int);
extern void FrontendControls_UpdateBindingCapture(Menu*);
extern void FrontendControls_BeginWiperBinding(Menu*,MenuItem*);
extern void FrontendControls_UpdateWiperBinding(Menu*,int);
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
bool parse(KeyRemappingOptions &opts,const char *value,std::string &error){char a[]="test",b[]="--bind-key";char *args[]={a,b,const_cast<char*>(value)};int i=1;return opts.Consume(i,3,args,error);}
int main(){
 KeyRemappingOptions opts;std::string error;
 unsigned char base[9]={0xcb,0xcd,0xc8,0xd0,0x39,0x1b,0x1a,0x2e,0x13},out[9];
 memcpy(out,base,9);opts.Apply(0,out);CHECK(!memcmp(out,base,9));
 CHECK(parse(opts,"left=A",error)&&error.empty());CHECK(parse(opts,"accelerate=W",error)&&error.empty());CHECK(parse(opts,"2:brake=Keypad 2",error)&&error.empty());CHECK(opts.Validate(error));
 memcpy(out,base,9);opts.Apply(0,out);CHECK(out[0]==0x1e&&out[2]==0x11&&out[1]==base[1]);
 memcpy(out,base,9);opts.Apply(1,out);CHECK(out[3]==0x50&&out[0]==base[0]);
 KeyRemappingOptions swap;CHECK(parse(swap,"left=Right",error));CHECK(parse(swap,"right=Left",error));CHECK(swap.Validate(error));memcpy(out,base,9);swap.Apply(0,out);CHECK(out[0]==0xcd&&out[1]==0xcb);
 KeyRemappingOptions clear;CHECK(parse(clear,"left=Up",error));memcpy(out,base,9);clear.Apply(0,out);CHECK(out[0]==0xc8&&out[2]==0);CHECK(parse(clear,"left=none",error));memcpy(out,base,9);clear.Apply(0,out);CHECK(out[0]==0&&out[2]==0xc8);
 KeyRemappingOptions bad;CHECK(parse(bad,"left=A",error));CHECK(parse(bad,"right=A",error));CHECK(!bad.Validate(error));
 for(auto value:{"left=Escape","fly=A","3:left=A","left=not-a-key","left="}){error.clear();CHECK(parse(bad,value,error)&&!error.empty());}
 error.clear();char a[]="test",b[]="--bind-key";char *args[]={a,b};int i=1;CHECK(bad.Consume(i,2,args,error)&&!error.empty());
 // Actual gameplay/menu routing does not mutate device bindings or control table.
 DeviceInfo &device=CInput::m_availableDevices[0];memcpy(&device.keyboard.steerLeftKey,base,9);
 memset(CInput::m_keyboardState,0,256);CInput::m_keyboardState[0xcb]=0x80;CGameInfo::m_fixedMenuKeys=0;CInput::ReadKeyboardDevice(&device);CHECK(device.held==1);CHECK(!memcmp(&device.keyboard.steerLeftKey,base,9));
 CGameInfo::m_fixedMenuKeys=1;CInput::m_keyboardState[0x1c]=0x80;CInput::ReadKeyboardDevice(&device);CHECK(device.held==(1|0x10));
 // Nine keys/labels followed by a tenth button action: no overwrite of names.
 ControllerData data={};memset(data.keyNames,'Z',sizeof data.keyNames);data.keys[0]=data.keys[8]=0x11;data.actionMasks[0]=data.actionMasks[8]=1;
 FrontendControls_ClearDuplicateBinding(&data,8);CHECK(data.keys[0]==0&&data.keyNames[0][0]==0);CHECK(data.keyNames[8][0]=='Z');CHECK(data.actionMasks[0]==0);CHECK(data.keys[8]==0x11);
 char names[sizeof data.keyNames];memcpy(names,data.keyNames,sizeof names);FrontendControls_ClearDuplicateBinding(&data,9);CHECK(!memcmp(names,data.keyNames,sizeof names));
 // Capture Escape cancels, leaving the complete working/saved records alone.
 g_controlsSelectedSlot=0;g_controlsDeviceSelection.slots[0]=0;device.type=1;g_controlsCopy[0]=data;auto before=g_controlsCopy[0];g_controlsBindingCaptureState=1;g_unk0x0082a7e0=-1;memset(CInput::m_keyboardState,0,256);CInput::m_keyboardState[1]=0x80;Menu menu={};menu.cursor=0;FrontendControls_UpdateBindingCapture(&menu);CHECK(g_controlsBindingCaptureState==0);CHECK(!memcmp(&before,&g_controlsCopy[0],sizeof before));
 // Capture a held supported key, not the last unrelated queued event.
 g_controlsBindingCaptureState=1;g_unk0x0082a7e0=-1;memset(CInput::m_keyboardState,0,256);CInput::m_keyboardState[0x11]=0x80;CInput::QueueVirtualKeyPress(0x2e);FrontendControls_UpdateBindingCapture(&menu);CHECK(g_controlsBindingCaptureState==0);CHECK(g_controlsCopy[0].keys[0]==0x11);CHECK(g_controlsCopy[0].keyNames[0][0]!='C');
 // Extension capture is outside the packed driving keys. Confirm must release;
 // an occupied driving key is refused; Escape leaves both settings and keys alone.
 const int oldWiper=Sys_GetOption("video.windscreen_wiper_key",17);
 CInput::m_controllerInfo[0].keys[0]=0x11;
 g_controlsCopy[0].keys[1]=0x13; // unsaved driving edit must also be reserved
 FrontendControls_BeginWiperBinding(&menu,nullptr);
 FrontendControls_UpdateWiperBinding(&menu,0);CHECK(!menu.flag2);
 memset(CInput::m_keyboardState,0,256);FrontendControls_UpdateWiperBinding(&menu,0);
 CInput::m_keyboardState[0x11]=0x80;FrontendControls_UpdateWiperBinding(&menu,0);CHECK(!menu.flag2);
 memset(CInput::m_keyboardState,0,256);CInput::m_keyboardState[0x13]=0x80;FrontendControls_UpdateWiperBinding(&menu,0);CHECK(!menu.flag2);
 memset(CInput::m_keyboardState,0,256);CInput::m_keyboardState[1]=0x80;FrontendControls_UpdateWiperBinding(&menu,0);CHECK(menu.flag2);
 CHECK(Sys_GetOption("video.windscreen_wiper_key",17)==oldWiper);CHECK(CInput::m_controllerInfo[0].keys[0]==0x11);
 puts("key remapping: parsing, swaps, duplicates, two keyboards, menu context, capture cancel and bounded labels pass");return 0;
}
