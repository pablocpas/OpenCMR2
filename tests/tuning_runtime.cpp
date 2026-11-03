#include "port/types.h"
#include "platform/testing.h"
#include "port/sys.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <string>
void TuningPanel_Update();bool TuningPanel_IsOpen();bool TuningPanel_InfoShown();bool TuningPanel_FpsOnly();bool TuningPanel_PreviewActive();
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 auto old=std::filesystem::current_path();auto tmp=std::filesystem::temp_directory_path()/("opencmr-tuning-runtime-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(tmp);std::filesystem::current_path(tmp);std::ofstream("opencmr2.ini")<<"[video]\ndrone_distance=60\n";
 BYTE keys[256]={};Platform_SetTestKeyboard(keys);TuningPanel_Update();
 CHECK(TuningPanel_InfoShown()&&!TuningPanel_IsOpen()&&!TuningPanel_FpsOnly());
 for(int lap=0;lap<2;lap++)for(int mode=1;mode<=4;mode++){
  keys[68]=0x80;TuningPanel_Update();int m=mode%4;
  CHECK(TuningPanel_IsOpen()==(m==1));CHECK(TuningPanel_FpsOnly()==(m==3));CHECK(TuningPanel_InfoShown()==(m!=2));CHECK(TuningPanel_PreviewActive());
  TuningPanel_Update();CHECK(TuningPanel_IsOpen()==(m==1)&&TuningPanel_FpsOnly()==(m==3));
  keys[68]=0;TuningPanel_Update();CHECK(TuningPanel_IsOpen()==(m==1)&&TuningPanel_FpsOnly()==(m==3));
 }
 // Actual live panel controls, save/lock/unlock/reset through raw keys.
 auto press=[&](int k){keys[k]=0x80;TuningPanel_Update();keys[k]=0;TuningPanel_Update();};
 press(68);CHECK(TuningPanel_IsOpen());
 for(int i=0;i<38;i++)press(3); // distance row, after 35 legacy rows (incl. fog clear + fog cull + fog spatial/evolve + fog lights + snow glints + dust plume + spray + exhaust smoke + splash + backfire + weather sidecar) and lock row
 press(5);CHECK(Sys_GetOption("video.drone_distance",60)==65);
 press(2);press(5); // lock row -> save and lock
 CHECK(Sys_GetOption("video.drone_locked",0)==1);
 std::string ini;{std::ifstream stored("opencmr2.ini");ini.assign((std::istreambuf_iterator<char>(stored)),{});} // handle closed before later saves/cleanup: Windows rename/remove fail on open files
 CHECK(ini.find("drone_distance")!=std::string::npos);
 Sys_SetOptionTemporary("video.drone_distance",50);CHECK(Sys_GetOption("video.drone_distance",60)==65); // stale launch override cannot bypass saved lock
 Sys_SetOptionTemporary("video.drone_distance",65);
 press(3);press(5);CHECK(Sys_GetOption("video.drone_distance",60)==65);
 press(7);CHECK(Sys_GetOption("video.drone_distance",60)==65); // reset-all cannot alter locked preset
 press(2);press(4);CHECK(Sys_GetOption("video.drone_locked",0)==0);
 press(3);press(5);CHECK(Sys_GetOption("video.drone_distance",60)==70);
 press(6);CHECK(Sys_GetOption("video.drone_distance",60)==60); // reset selected to opening
 Platform_SetTestKeyboard(nullptr);std::filesystem::current_path(old);std::filesystem::remove_all(tmp);
 puts("actual F10 raw input: four modes, hold/release, repeat laps, preview persistence pass");
}
