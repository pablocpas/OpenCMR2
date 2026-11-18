// Failing-first regression for the native "FOV dolly does nothing" report (Oct 30): a chase view has TWO camera
// records (record index = slot + view * 2, the active slot toggles on every camera switch) and the dolly keyed the
// per-VIEW speed-FOV state by the record INDEX. Whenever the active chase record had an odd index the dolly read
// the other player's (zero) speed FOV and did nothing; view 1's records (index 2, 3) never saw any widening at all.
// Both records of a view must read that view's widening. Exercises the real View_SetPlayerProjection +
// ChaseCamera_BuildMatrix for both views.
#include "port/types.h"
#include "Graphics.h"
#include "CameraRecord.h"
#include "FixedPoint.h"
#include "port/sys.h"
#include "platform/testing.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
extern CameraRecord g_viewRecords[4];
extern BYTE g_activeViewRecord[4], g_playerCameraMode[8];
extern FixVector g_chaseCameraOffsets[6], g_chaseCameraViewOffsets[6];
extern int g_viewZooms[2], g_viewNearPlanes[2], g_viewFarPlanes[2];
void View_SetPlayerProjection(unsigned int);
void ChaseCamera_BuildMatrix(CameraRecord *pObj, FixMatrix *pRef);
void View_GetFovDolly(unsigned int view, int *scale, int *widening);
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
static DWORD ticks(){return 1000;}static double precise(){return 1000;}
static CameraRecord Chase(int index,int view){ CameraRecord r={}; r.index=(BYTE)index; r.view=(BYTE)view; r.car=(BYTE)view; r.type=4; return r; }
// Both records of a view, same offset and basis, must land at the same dollied distance. The caller varies
// the per-view speed FOV between views, so the widening/scale ranges isolate each view's own eased state.
static int CheckViewDolly(int view, int widenLo, int widenHi, int scaleLo, int scaleHi){
 int slot=view*2;
 g_playerCameraMode[slot]=g_playerCameraMode[slot+1]=4;
 g_chaseCameraViewOffsets[view]={0,0x13333,-0x50000};
 g_chaseCameraOffsets[2+slot]={0,65536,0};g_chaseCameraViewOffsets[2+slot]={0,0,65536};
 g_chaseCameraOffsets[3+slot]={0,65536,0};g_chaseCameraViewOffsets[3+slot]={0,0,65536};
 FixMatrix ref={};ref.right.x=65536;ref.up.y=65536;ref.forward.z=65536;
 CameraRecord a=Chase(slot,view),b=Chase(slot+1,view);
 ChaseCamera_BuildMatrix(&a,&ref);
 ChaseCamera_BuildMatrix(&b,&ref);
 // The dolly is engaged (widening x2.0 over the saved view): the camera is closer than the saved -0x50000 offset...
 CHECK(a.matrix.position.z > -0x50000 && a.matrix.position.z < 0);
 // ...and the view's second record gets exactly the same distance, not the full one.
 CHECK(b.matrix.position.z == a.matrix.position.z);
 CHECK(b.matrix.position.y == a.matrix.position.y);
 // The overlay reads the same per-view state the records used.
 int scale=0,widening=0;View_GetFovDolly(view,&scale,&widening);
 CHECK(widening > widenLo && widening < widenHi);
 CHECK(scale > scaleLo && scale < scaleHi);
 return 0;
}
int main(){
 auto old=std::filesystem::current_path();auto tmp=std::filesystem::temp_directory_path()/"opencmr-fov-dolly-keying";
 std::filesystem::create_directories(tmp);std::filesystem::current_path(tmp);std::ofstream("opencmr2.ini")<<"[video]\n";
 PlatformTestClock clock={ticks,nullptr,0,precise};Platform_SetTestClock(&clock);
 Graphics graphics={};graphics.resX=1920;graphics.resY=1080;g_pGraphics=&graphics;
 CGraphics::m_projectionScale=1;
 Sys_SetOption("video.speed_effects",1);Sys_SetOption("video.speed_fov",100);Sys_SetOption("video.speed_fov_hold",1);
 Sys_SetOption("video.camera_effects",0);   // no handheld-operator shake in the matrix build
 // View 0 at full speed FOV: hold snaps its eased value to full - widening x2.0, dolly at the 0.70 floor
 // (full compensation at x2.0 is 0.50, amount 150 overshoots into the clamp).
 g_viewZooms[0]=65536;g_viewNearPlanes[0]=65536;g_viewFarPlanes[0]=1000*65536;
 g_activeViewRecord[0]=0;g_viewRecords[0].type=4;
 View_SetPlayerProjection(0);
 if(CheckViewDolly(0, 65536*19/10, 65536*21/10, 65536*69/100, 65536*71/100))return 1;
 // View 1 at one tenth of the speed FOV: widening x1.1, compensation above the floor. A shared or
 // record-index-keyed state would read view 0's x2.0 here (or nothing, as in the native bug).
 Sys_SetOption("video.speed_fov",10);
 g_viewZooms[1]=65536;g_viewNearPlanes[1]=65536;g_viewFarPlanes[1]=1000*65536;
 g_activeViewRecord[1]=0;g_viewRecords[2].type=4;
 View_SetPlayerProjection(1);
 if(CheckViewDolly(1, 65536*105/100, 65536*115/100, 65536*75/100, 65536*95/100))return 1;
 // Cross-view isolation: view 1's setup must not clobber view 0's eased state (Devin review on #371).
 if(CheckViewDolly(0, 65536*19/10, 65536*21/10, 65536*69/100, 65536*71/100))return 1;
 Platform_SetTestClock(nullptr);g_pGraphics=nullptr;std::filesystem::current_path(old);std::filesystem::remove_all(tmp);
 puts("chase records key the dolly on their view: per-view widenings x2.0 and x1.1 stay isolated across both records and views");
}
