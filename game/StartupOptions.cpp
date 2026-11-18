#include "StartupOptions.h"
#include "AIHelper.h"
#include "SceneryPack.h"
#include "port/sys.h"
#include <cstdio>
#include <cstring>
void LogStartupOptions() {
 Sys_Log("options: configured requests, NOT proof of active GPU effects; runtime status follows");
 struct Option{const char *key;int value;};
 const char *requested=Sys_GetOptionText("video.graphics_api","auto");
 const char *api=!strcmp(requested,"sdl_gpu")?"sdl_gpu":!strcmp(requested,"opengl")?"opengl":!strcmp(requested,"auto")?"auto":"unrecognized (auto selection)";
 Sys_Log("options: requested video.graphics_api=%s",api);
 const Option options[]={
  {"fullscreen",1},{"vsync",1},{"anisotropy",8},{"smooth_scaling",1},
  {"spatial_aa",0},{"metalfx_spatial",0},{"dust_plume",1},{"dust_volume",1},
  {"reflection_resolution",128},{"motion_blur",0}/* retired: log fallback matches the effect default (owner decision 2026-11-18) */,{"bloom",1},{"particles",1},
  {"shadows",1},{"volumetric_clouds",1},{"body_dirt",1},{"reactive_spectators",1},{"depth_of_field",0},{"speed_motes",0},{"god_rays",0},{"color_grading",0}
 };
 for(const auto &option:options){char key[80];snprintf(key,sizeof key,"video.%s",option.key);
  Sys_Log("options: requested %s=%d",key,Sys_GetOption(key,option.value));}
 Sys_Log("options: volumetric clouds are disabled in this build (video.volumetric_clouds / --effect volumetric_clouds are ignored)");
 Sys_Log("options: temporal reconstruction is not implemented (spatial AA/MetalFX are separate)");
 CAIHelper::LogDriverNameStatus();
 SceneryPack_LogStatus();
}
