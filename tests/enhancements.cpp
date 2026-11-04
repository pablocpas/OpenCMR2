#include <vector>
#include "platform/enhancements.h"
#include "platform/testing.h"
#include "render/gpu_startup_options.h"
#include "Input.h"
#include "GameInfo.h"
#include "InstallInfo.h"
#include "platform/platform.h"
#include "port/sys.h"
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <string>
namespace fs=std::filesystem;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
void write(fs::path p,std::string s){fs::create_directories(p.parent_path());std::ofstream(p)<<s;}
std::string read(fs::path p){std::ifstream f(p);return std::string(std::istreambuf_iterator<char>(f),{});}
int main(){
 EnhancementOptions e;std::string error;
 {EnhancementOptions pack;char a0[]="test",a1[]="--asset-upgrades";char*av[]={a0,a1};int i=1;CHECK(pack.Consume(i,2,av,error)&&error.empty());pack.Apply();CHECK(pack.trees=="shell"&&pack.treeCoverage=="all"&&pack.spectators=="animated"&&pack.textures=="uprez"&&pack.roads=="finland1"&&pack.overrides["video.texture_uprez"]==1);}

 {EnhancementOptions road;std::string a[]={"test","--roads","curved"};char*av[]={a[0].data(),a[1].data(),a[2].data()};int k=1;CHECK(road.Consume(k,3,av,error)&&error.empty()&&road.roads=="curved"&&!road.NeedsPack());road.Apply();CHECK(road.overrides.empty());a[2]="original";av[2]=a[2].data();k=1;CHECK(road.Consume(k,3,av,error)&&error.empty()&&road.roads=="original");a[2]="bad";av[2]=a[2].data();k=1;CHECK(road.Consume(k,3,av,error)&&!error.empty());error.clear();}

 {EnhancementOptions texture;std::string a[]={"test","--textures","france1"};char*av[]={a[0].data(),a[1].data(),a[2].data()};int k=1;CHECK(texture.Consume(k,3,av,error)&&error.empty()&&texture.textures=="france1"&&!texture.NeedsPack());texture.Apply();CHECK(texture.overrides["video.texture_uprez"]==0);a[2]="wrong";av[2]=a[2].data();k=1;CHECK(texture.Consume(k,3,av,error)&&!error.empty());error.clear();}
 std::string args[]={"test","--trees","crossed","--spectators","original","--driver-names","real"};
 char *argv[7];for(int i=0;i<7;i++)argv[i]=args[i].data();
 for(int i=1;i<7;i++)CHECK(e.Consume(i,7,argv,error));
 e.Apply();CHECK(error.empty()&&e.NeedsPack());CHECK(e.overrides["video.tree_overrides"]==1&&e.overrides["video.spectator_overrides"]==0&&e.overrides["video.real_driver_names"]==1);
 args[2]="bogus";for(int i=0;i<7;i++)argv[i]=args[i].data();int i=1;CHECK(e.Consume(i,7,argv,error)&&!error.empty());
 std::string effectArgs[]={"test","--effect","dust_volume=off"};char *ef[]={effectArgs[0].data(),effectArgs[1].data(),effectArgs[2].data()};
 error.clear();i=1;CHECK(e.Consume(i,3,ef,error)&&error.empty()&&e.overrides["video.dust_volume"]==0);
 effectArgs[2]="unknown=on";ef[2]=effectArgs[2].data();i=1;CHECK(e.Consume(i,3,ef,error)&&!error.empty());
 std::string windArgs[]={"test","--tree-wind","off"};char *wv[]={windArgs[0].data(),windArgs[1].data(),windArgs[2].data()};
 error.clear();i=1;CHECK(e.Consume(i,3,wv,error)&&error.empty());e.Apply();CHECK(e.overrides["video.tree_wind"]==0);
 {EnhancementOptions t;std::string e2;auto run=[&](std::vector<std::string> a){std::vector<char*> v;for(auto&x:a)v.push_back(x.data());e2.clear();for(int k=1;k<(int)v.size();k++)if(!t.Consume(k,(int)v.size(),v.data(),e2))return false;return true;};
  CHECK(run({"t","--sun-glare","40"})&&e2.empty()&&t.overrides["video.sun_glare"]==40);
  CHECK(run({"t","--sun-glare","101"})&&!e2.empty());CHECK(run({"t","--sun-glare","-1"})&&!e2.empty());
  CHECK(run({"t","--speed-fov","12","--speed-motes-stretch=60","--speed-motes-amount","250"})&&e2.empty());
  CHECK(t.overrides["video.speed_fov"]==12&&t.overrides["video.speed_motes_stretch"]==60&&t.overrides["video.speed_motes_amount"]==250);
  CHECK(run({"t","--speed-fov","501"})&&!e2.empty());CHECK(run({"t","--speed-fov","x"})&&!e2.empty());CHECK(run({"t","--speed-motes-stretch","101"})&&!e2.empty());
  CHECK(run({"t","--speed-motes-amount","301"})&&!e2.empty());CHECK(run({"t","--test-preset","bad"})&&!e2.empty());
  CHECK(run({"t","--camera-roll","30"})&&e2.empty());
  CHECK(t.overrides["video.camera_roll"]==30);
  CHECK(run({"t","--camera-roll","101"})&&!e2.empty());
  CHECK(run({"t","--drone-distance","50","--speed-fov-cockpit=25","--cockpit-blur-clear","16"})&&e2.empty());
  CHECK(t.overrides["video.drone_distance"]==50&&t.overrides["video.speed_fov_cockpit"]==25&&t.overrides["video.cockpit_blur_clear"]==16);
  CHECK(run({"t","--drone-distance","49"})&&!e2.empty());CHECK(run({"t","--drone-distance","151"})&&!e2.empty());CHECK(run({"t","--speed-fov-cockpit","101"})&&!e2.empty());CHECK(run({"t","--cockpit-blur-clear","61"})&&!e2.empty());
  EnhancementOptions p;t=p;CHECK(run({"t","--test-preset","overdo","--speed-fov","5","--effect","motion_blur=off"})&&e2.empty());t.Apply();
  CHECK(t.overrides["video.speed_fov"]==5&&t.overrides["video.speed_fov_hold"]==1&&t.overrides["video.motion_blur"]==0&&t.overrides["video.speed_motes"]==0&&!t.overrides.count("video.speed_motes_stretch")&&t.overrides["video.dof_blur"]==100&&t.overrides["video.dof_blur_max_px"]==16&&t.overrides["video.speed_streak"]==100);
  t=p;CHECK(run({"t","--test-preset","overdo","--effect","speed_motes=on"})&&e2.empty());t.Apply();CHECK(t.overrides["video.speed_motes"]==1);   // legacy motes stay opt-in and beat the preset
  t=p;CHECK(run({"t","--test-preset","overdo","--speed-streak","0"})&&e2.empty());t.Apply();CHECK(t.overrides["video.speed_streak"]==0);   // explicit flag wins over the preset
  t=p;CHECK(run({"t","--test-preset","overdo","--speed-hold","0","--dof-focus-car","0"})&&e2.empty());t.Apply();CHECK(t.overrides["video.speed_fov_hold"]==0&&t.overrides["video.dof_focus_car"]==0);   // explicit flags beat the preset
  t=p;CHECK(run({"t","--test-preset","overdo"})&&e2.empty());t.Apply();CHECK(t.overrides["video.speed_fov_hold"]==1&&t.overrides["video.dof_focus_car"]==1);   // preset default unchanged except focus
  t=p;CHECK(run({"t","--test-preset","overdo"})&&e2.empty());t.Apply();CHECK(!t.overrides.count("video.fog")&&!t.overrides.count("video.air_motes")&&!t.overrides.count("video.god_rays"));   // overdo never enables atmosphere; --all-mods look unchanged
  t=p;CHECK(run({"t","--test-preset","atmosphere"})&&e2.empty());t.Apply();
  CHECK(t.overrides["video.fog"]==1&&t.overrides["video.fog_amount"]==65&&t.overrides["video.fog_glow"]==100&&t.overrides["video.fog_clear"]==60&&t.overrides["video.fog_lights"]==40&&t.overrides["video.fog_spatial"]==1&&t.overrides["video.fog_evolve"]==60&&t.overrides["video.air_motes"]==1&&t.overrides["video.air_motes_amount"]==500&&t.overrides["video.snow_glint"]==100&&t.overrides["video.god_rays"]==1&&t.overrides["video.god_rays_amount"]==80);   // exaggerated atmosphere starting look, with the light shafts
  CHECK(!t.overrides.count("video.speed_fov"));   // atmosphere leaves the overdo speed look alone
  t=p;CHECK(run({"t","--test-preset","atmosphere","--fog-amount","40","--effect","air_motes=off"})&&e2.empty());t.Apply();
  CHECK(t.overrides["video.fog_amount"]==40&&t.overrides["video.air_motes"]==0);   // explicit flags beat the atmosphere preset
  t=p;CHECK(run({"t","--test-preset","atmosphere","--fog-clear","60","--fog-cull","1"})&&e2.empty());t.Apply();
  CHECK(t.overrides["video.fog_clear"]==60&&t.overrides["video.fog_cull"]==1);   // fog clear/cull flags parse and override the preset defaults
  CHECK(run({"t","--fog-clear","201"})&&!e2.empty());CHECK(run({"t","--fog-clear","x"})&&!e2.empty());CHECK(run({"t","--fog-cull","2"})&&!e2.empty());
  t=p;CHECK(run({"t","--fog-spatial","1","--fog-evolve","-40"})&&e2.empty());t.Apply();
  CHECK(t.overrides["video.fog_spatial"]==1&&t.overrides["video.fog_evolve"]==60);   // signed CLI stored 0..200 neutral 100
  CHECK(run({"t","--fog-spatial","2"})&&!e2.empty());CHECK(run({"t","--fog-evolve","101"})&&!e2.empty());CHECK(run({"t","--fog-evolve","-101"})&&!e2.empty());
  t=p;CHECK(run({"t","--test-preset","foggy"})&&!e2.empty());   // unknown preset reports an error
  t=p;CHECK(run({"t","--test-preset","overdo,atmosphere"})&&e2.empty());t.Apply();
  CHECK(t.overrides["video.speed_fov"]==100&&t.overrides["video.fog"]==1&&t.overrides["video.air_motes_amount"]==500);   // combined preset: usual overdo look plus exaggerated atmosphere
  t=p;CHECK(run({"t","--test-preset","off,atmosphere"})&&!e2.empty());   // off does not combine
  t=p;CHECK(run({"t","--speed-hold","1"})&&e2.empty());t.Apply();CHECK(t.overrides["video.speed_fov_hold"]==1&&!t.overrides.count("video.dof_focus_car"));   // without the preset only what was asked
  t=p;CHECK(run({"t","--fov-preview","40"})&&e2.empty());t.Apply();CHECK(t.overrides["video.fov_preview"]==40);
  CHECK(run({"t","--fov-preview","0"})&&e2.empty());CHECK(run({"t","--fov-preview","19"})&&!e2.empty());CHECK(run({"t","--fov-preview","151"})&&!e2.empty());CHECK(run({"t","--fov-preview","x"})&&!e2.empty());
  CHECK(run({"t","--speed-hold","2"})&&!e2.empty());CHECK(run({"t","--dof-focus-car","x"})&&!e2.empty());
  CHECK(run({"t","--speed-streak","101"})&&!e2.empty());CHECK(run({"t","--speed-streak","x"})&&!e2.empty());
  t=p;CHECK(run({"t","--effect","vapor_trails=on","--effect","speed_lines=off"})&&e2.empty());t.Apply();CHECK(t.overrides["video.vapor_trails"]==1&&t.overrides["video.speed_lines"]==0);CHECK(run({"t","--effect","vapor_trails=maybe"})&&!e2.empty());
  CHECK(run({"t","--speed-motes-size","250"})&&e2.empty()&&t.overrides["video.speed_motes_size"]==250);CHECK(run({"t","--speed-motes-size","49"})&&!e2.empty());CHECK(run({"t","--speed-motes-size","801"})&&!e2.empty());
  CHECK(run({"t","--dof-blur-max-px","8"})&&e2.empty()&&t.overrides["video.dof_blur_max_px"]==8);CHECK(run({"t","--dof-blur-max-px","0"})&&!e2.empty());CHECK(run({"t","--dof-blur-max-px","41"})&&!e2.empty());
  CHECK(run({"t","--camera-move-type","5=30"})&&e2.empty()&&t.overrides["video.camera_move_type5"]==30);CHECK(run({"t","--camera-move-type","7=30"})&&!e2.empty());CHECK(run({"t","--camera-move-type","5=201"})&&!e2.empty());CHECK(run({"t","--camera-move-type","5"})&&!e2.empty());
CHECK(run({"t","--camera-move-chase","-60"})&&e2.empty()&&t.overrides["video.camera_move_chase"]==-60);CHECK(run({"t","--camera-move-cockpit","5"})&&e2.empty()&&t.overrides["video.camera_move_cockpit"]==5);CHECK(run({"t","--camera-move-car","201"})&&!e2.empty());
  CHECK(run({"t","--cockpit-eye-shift","-40"})&&e2.empty()&&t.overrides["video.cockpit_eye_shift"]==-40);CHECK(run({"t","--cockpit-eye-shift","101"})&&!e2.empty());CHECK(run({"t","--cockpit-eye-shift","x"})&&!e2.empty());
  CHECK(run({"t","--tree-rustle","150","--spectator-irregular","0"})&&e2.empty()&&t.overrides["video.tree_rustle"]==150&&t.overrides["video.spectator_irregular"]==0);CHECK(run({"t","--tree-rustle","301"})&&!e2.empty());CHECK(run({"t","--spectator-irregular","2"})&&!e2.empty());
  CHECK(run({"t","--tree-coverage","all","--spectator-gain","250"})&&e2.empty()&&t.treeCoverage=="all"&&t.spectatorGain==250);CHECK(run({"t","--tree-coverage","some"})&&!e2.empty());CHECK(run({"t","--spectator-gain","99"})&&!e2.empty());CHECK(run({"t","--spectator-gain","401"})&&!e2.empty());
  {EnhancementOptions q2;t=q2;CHECK(run({"t","--test-preset","overdo"})&&e2.empty());t.Apply();CHECK(t.overrides["video.tunnel_vision"]==130&&t.overrides["video.tunnel_lines"]==1);   // the preset follows the new default
   EnhancementOptions q3;t=q3;CHECK(run({"t","--test-preset","overdo","--tunnel-vision","120","--tunnel-lines=0"})&&e2.empty());t.Apply();CHECK(t.overrides["video.tunnel_vision"]==120&&t.overrides["video.tunnel_lines"]==0);   // explicit flags beat the preset
   {EnhancementOptions b;t=b;CHECK(run({"t","--brush-volume","1"})&&e2.empty());t.Apply();CHECK(t.overrides["video.brush_volume"]==1);}
   {EnhancementOptions b;t=b;CHECK(run({"t","--brush-volume","2"})&&!e2.empty());}
   {EnhancementOptions b;t=b;CHECK(run({"t"})&&e2.empty());t.Apply();CHECK(t.overrides.count("video.brush_volume")==0);}   // default stays off
   EnhancementOptions q4;t=q4;CHECK(run({"t","--tunnel-vision","201"})&&!e2.empty());CHECK(run({"t","--tunnel-lines","2"})&&!e2.empty());
   EnhancementOptions q5;t=q5;CHECK(run({"t","--speed-fov","500"})&&e2.empty());CHECK(t.overrides["video.speed_fov"]==500);}
  EnhancementOptions q;t=q;CHECK(run({"t","--effect","bloom=on"})&&e2.empty());t.Apply();CHECK(!t.overrides.count("video.speed_fov")&&!t.overrides.count("video.dof_blur"));}
 auto root=fs::temp_directory_path()/(std::string("cmr-enhancements-")+std::to_string(SDL_GetPerformanceCounter()));
 auto game=root/"owned game",user=root/"profile";
 write(game/"Game/Bigfiles/common.bfl","base");write(game/"FrontEnd/common.bfl","front");write(game/"Sounds/surface.bfl","sound");
 write(game/"CountrySpecific/Europe/englishtext.bfl","text");write(game/"SetupRep/640/common.bfl","setup");
 write(user/"opencmr2.ini","[video]\ndriver_name_overrides=1\nscenery_overrides=1\ntree_overrides=1\n");
 write(user/"pps/valued.pps","driver progress");write(user/"gamesave/race.rcs","race save");
 std::string ini=read(user/"opencmr2.ini");
 SDL_SetEnvironmentVariable(SDL_GetEnvironment(),"SDL_VIDEODRIVER","dummy",true);
 std::string actual[]={"test","--data",game.string(),"--user-dir",user.string(),"--trees","original","--spectators","original","--driver-names","original"};
 char *av[11];for(int j=0;j<11;j++)av[j]=actual[j].data();
 CHECK(Platform_Init(11,av));CHECK(Sys_GetOption("video.tree_overrides",1)==0&&Sys_GetOption("video.spectator_overrides",1)==0&&Sys_GetOption("video.driver_name_overrides",1)==0);
 CHECK(std::string(Platform_GetUserDir())==user.string()+"/");Platform_Shutdown();
 CHECK(read(user/"pps/valued.pps")=="driver progress"&&read(user/"gamesave/race.rcs")=="race save"&&read(user/"opencmr2.ini")==ini);
 // Starting without flags resets transient overrides, not the persisted profile.
 SDL_SetEnvironmentVariable(SDL_GetEnvironment(),"SDL_VIDEODRIVER","dummy",true);
 CHECK(Platform_Init(5,av));CHECK(Sys_GetOption("video.tree_overrides",1)==1);Platform_Shutdown();
 // Same startup reader the actual GPU constructor uses, not a log-only check.
 std::string aa[]={"test","--data",game.string(),"--user-dir",user.string(),"--effect=spatial_aa=on","--effect","metalfx_spatial=on"};
 char* aaArgs[8];for(int j=0;j<8;j++)aaArgs[j]=aa[j].data();
 SDL_SetEnvironmentVariable(SDL_GetEnvironment(),"SDL_VIDEODRIVER","dummy",true);CHECK(Platform_Init(8,aaArgs));auto gpu=GPUStartupOptions::Read();CHECK(gpu.spatialAA&&gpu.metalFXSpatial);
 CHECK(std::string(Sys_GetLaunchSummary(2)).find("CLI effects:2")!=std::string::npos);
 for(int i=0;i<3;i++){std::string summary=Sys_GetLaunchSummary(i);CHECK(summary.find(user.string())==std::string::npos&&summary.find("/tmp/")==std::string::npos);}

 CHECK(Platform_GetSettingInt("video.spatial_aa",0)==0&&Platform_GetSettingInt("video.metalfx_spatial",0)==0);Platform_Shutdown();
 CHECK(read(user/"opencmr2.ini")==ini);SDL_SetEnvironmentVariable(SDL_GetEnvironment(),"SDL_VIDEODRIVER","dummy",true);CHECK(Platform_Init(5,av));gpu=GPUStartupOptions::Read();CHECK(!gpu.spatialAA&&!gpu.metalFXSpatial);Platform_Shutdown();
 // Per-launch keys reach real gameplay polling, not persisted records/menu keys.
 write(user/"Configuration/Controller.rcf","sentinel saved controls");
 std::string bound[]={"test","--data",game.string(),"--user-dir",user.string(),"--bind-key","left=A","--bind-key","right=D"};
 char *bv[9];for(int j=0;j<9;j++)bv[j]=bound[j].data();SDL_SetEnvironmentVariable(SDL_GetEnvironment(),"SDL_VIDEODRIVER","dummy",true);CHECK(Platform_Init(9,bv));
 auto &device=CInput::m_availableDevices[0];device.type=1;device.keyboard.steerLeftKey=0xcb;device.keyboard.steerRightKey=0xcd;
 BYTE keys[256]={};
 Platform_SetTestKeyboard(keys);
 CGameInfo::m_fixedMenuKeys=0;keys[0x1e]=0x80;Input_Update();CInput::ReadKeyboardDevice(&device);CHECK(device.held==1&&device.keyboard.steerLeftKey==0xcb);
 keys[0x1e]=0;keys[0xcb]=0x80;Input_Update();CInput::ReadKeyboardDevice(&device);CHECK(device.held==0);
 CGameInfo::m_fixedMenuKeys=1;Input_Update();CInput::ReadKeyboardDevice(&device);CHECK(device.held==1);
 CHECK(read(user/"Configuration/Controller.rcf")=="sentinel saved controls");
 // Explicit in-game save serializes only actual control edits, never CLI keys.
 char installPath[]=".";CInstallInfo::SetGameHDPath(installPath);CInput::m_playerControllers[0]=0;CInput::m_playerControllers[1]=1;
 memset(CInput::m_controllerInfo,0,sizeof CInput::m_controllerInfo);CInput::m_controllerInfo[0].keys[0]=0x2c;
 strcpy(CInput::m_controllerInfo[0].keyNames[0],"Z");CInput::SaveControllerInfo();auto saved=read(user/"Configuration/Controller.rcf");CHECK(saved.size()==0x11a4);
 CInput::m_controllerInfo[0].keys[0]=0;CInput::LoadControllerInfo();CHECK(CInput::m_controllerInfo[0].keys[0]==0x2c);CHECK(device.keyboard.steerLeftKey==0x2c);
 Platform_Shutdown();CHECK(read(user/"Configuration/Controller.rcf")==saved);
 SDL_SetEnvironmentVariable(SDL_GetEnvironment(),"SDL_VIDEODRIVER","dummy",true);CHECK(Platform_Init(5,bv));CGameInfo::m_fixedMenuKeys=0;memset(keys,0,sizeof keys);keys[0x2c]=0x80;Input_Update();CInput::ReadKeyboardDevice(&device);CHECK(device.held==1);Platform_SetTestKeyboard(nullptr);Platform_Shutdown();
 CHECK(read(user/"Configuration/Controller.rcf")==saved&&read(user/"opencmr2.ini")==ini);
 fs::remove_all(root);puts("CLI parsing, transient settings, profile/save preservation: pass");return 0;
}
