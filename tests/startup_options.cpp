#include "StartupOptions.h"
#include "port/types.h"
#include "port/sys.h"
#include <string>
#include <cstring>
#include <cstdarg>
#include <cstdio>
static std::string logs,mode;static int opens=0;
extern "C" void __wrap_Sys_Log(const char *fmt,...){char out[2048];va_list args;va_start(args,fmt);vsnprintf(out,sizeof out,fmt,args);va_end(args);logs+=out;logs+='\n';}
extern "C" int __wrap_Sys_GetOption(const char *key,int value){if(mode=="disabled"&&(!strcmp(key,"video.scenery_overrides")||!strcmp(key,"video.driver_name_overrides")))return 0;return value;}
extern "C" const char *__wrap_Sys_GetOptionText(const char*,const char *value){return value;}
extern "C" SysFile *__wrap_Sys_OpenFile(const char*,int){opens++;return nullptr;}
int main(int argc,char **argv){
 mode=argc>1?argv[1]:"missing";LogStartupOptions();
 const char *required[]={"configured requests, NOT proof", "requested video.metalfx_spatial=0","requested video.dust_volume=1","requested video.motion_blur=0","temporal reconstruction is not implemented",
  mode=="disabled"?"driver names=original (disabled)":"driver names=original (sidecar missing)",
  mode=="disabled"?"scenery=original (overrides disabled)":"scenery=original (manifest missing/unreadable)"};
 for(auto text:required)if(logs.find(text)==std::string::npos){fprintf(stderr,"missing: %s\n%s",text,logs.c_str());return 1;}
 if(opens!=(mode=="disabled"?0:2))return 2;
 puts(logs.c_str());return 0;
}
