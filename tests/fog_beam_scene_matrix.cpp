// Scene-matrix continuity regression for the fog-pass car lights
// (video.fog_lights): drives the PRODUCTION FogBeam_ProjectLights through
// cockpit/bonnet/bumper/chase/chase-far/side views against fine car yaw and
// pitch sweeps, and fails on any of the 2026-11-01 "headlights/backlights
// pointing all over" signatures:
//   - the behind-camera sentinel (-100,-100 px) surfacing as a lamp/halo uv
//     (near-plane-clipped anchors lost ~1 ulp in the view->world->view round
//     trip: bonnet/bumper lamps carried the sentinel);
//   - a wedge direction flip or length jump between adjacent sweep samples at
//     visible length (the transverse-formula length re-amplified any off-axis
//     angle past ~2 degrees to a full-screen wedge sweeping up/down/sideways);
//   - a halo depth reference outside (0,1] (the 3-unit forward anchor put
//     REAR lamp halos within a unit of the chase camera: the gate reference
//     went negative, the shader's halos[i].z > 0 test disabled the gate, and
//     red bubbles painted over the car unoccluded);
//   - an on-screen halo position jump between samples.
// CPU-only: no GL, no assets. Race projection constants from Game.cpp's
// SetProjection(0x25645,0x4326e,0xfa0000,0x10000) with m_projectionScale 0.5.
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <utility>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <initializer_list>
#include "Graphics.h"
#include "FixedPoint.h"
#include "SceneNode.h"
#include "LightField.h"
#include "FogBeamProjection.h"

FogLightSource g_fogLightSources[LIGHT_FIELD_MAX];
int g_fogLightSourceCount = 0;
// Defined in game/SceneNode.cpp in production; the beam-direction
// normalization uses FixVecLength, which reads this table. Fill it exactly
// as Scene_InitTables does (sqrt of (8+16n)/65536 in 16.16).
unsigned short g_sqrtTable[4096];

// Test-scoped definitions of the globals/functions the compiled game TUs
// reference outside the projection path under test (View_* helpers pulled in
// via FixedPoint.cpp). Never called by this test.
extern const float g_floatHalf = 0.5f;
double CGraphics::m_oneOver65536 = 1.0 / 65536.0;
double CGraphics::m_65536 = 65536.0;
Graphics *g_pGraphics;
static Graphics g_gfx;
static D3DTextureManager g_texManager;
D3DTextureManager *CGraphics::m_pTextureManager = &g_texManager;

static void matFromYPR(FixMatrix *m, double px, double py, double pz, double yawDeg, double pitchDeg) {
    double cy = cos(yawDeg*M_PI/180), sy = sin(yawDeg*M_PI/180), cp = cos(pitchDeg*M_PI/180), sp = sin(pitchDeg*M_PI/180);
    m->right.x=(int)(cy*65536); m->right.y=0; m->right.z=(int)(-sy*65536);
    m->up.x=(int)(sy*sp*65536); m->up.y=(int)(cp*65536); m->up.z=(int)(cy*sp*65536);
    m->forward.x=(int)(sy*cp*65536); m->forward.y=(int)(-sp*65536); m->forward.z=(int)(cy*cp*65536);
    m->position.x=(int)(px*65536); m->position.y=(int)(py*65536); m->position.z=(int)(pz*65536);
}
// Roll-aware variant: roll rotates right/up about the (yaw,pitch) forward
// axis, matching the car-mounted camera rolling with the car.
static void matFromYPRR(FixMatrix *m, double px, double py, double pz, double yawDeg, double pitchDeg, double rollDeg) {
    double cy=cos(yawDeg*M_PI/180), sy=sin(yawDeg*M_PI/180);
    double cp=cos(pitchDeg*M_PI/180), sp=sin(pitchDeg*M_PI/180);
    double cr=cos(rollDeg*M_PI/180), sr=sin(rollDeg*M_PI/180);
    double rx=cy, ry=0, rz=-sy;
    double ux=sy*sp, uy=cp, uz=cy*sp;
    double fx=sy*cp, fy=-sp, fz=cy*cp;
    double nrx=rx*cr+ux*sr, nry=ry*cr+uy*sr, nrz=rz*cr+uz*sr;
    double nux=ux*cr-rx*sr, nuy=uy*cr-ry*sr, nuz=uz*cr-rz*sr;
    m->right.x=(int)(nrx*65536); m->right.y=(int)(nry*65536); m->right.z=(int)(nrz*65536);
    m->up.x=(int)(nux*65536); m->up.y=(int)(nuy*65536); m->up.z=(int)(nuz*65536);
    m->forward.x=(int)(fx*65536); m->forward.y=(int)(fy*65536); m->forward.z=(int)(fz*65536);
    m->position.x=(int)(px*65536); m->position.y=(int)(py*65536); m->position.z=(int)(pz*65536);
}
struct ViewDef { const char *name; double x,y,z,yaw,pitch; };
static const ViewDef VIEWS[] = {
    {"cockpit", 0.0,1.05,0.35, 0,4}, {"bonnet", 0.0,0.95,1.75, 0,6},
    {"bumper",  0.0,0.55,2.35, 0,2}, {"chase",  0.0,2.40,-6.8, 0,14},
    {"chase-far",0.0,3.00,-11.0,0,11},{"side",   7.5,1.60,0.0, -90,6},
};
int failures = 0;
static void fail(const char *view, double ang, bool isYaw, int li, const char *what, float a, float b) {
    printf("FAIL %-9s %s=%+.0f L%d %s (%.3f vs %.3f)\n", view, isYaw?"yaw":"pitch", ang, li, what, a, b);
    failures++;
}
// True screen-space beam axis from an independent projection of the matched
// source's lamp and 30m beam end (aspect-corrected, normalized). A wedge
// direction flip is only a failure when the true axis did not also flip: a
// beam pivoting past pointing-at-the-camera flips physically, because its
// projected end passes through the lamp position.
static bool trueBeamAxis(const GfxFogLight &fl, SceneNode &view, double *ax, double *ay) {
    float best = 1e9f; int bi = -1;
    for (int j=0;j<g_fogLightSourceCount;j++) {
        FixVector pv; FixMatrix_ProjectWorldPointToView((int*)&pv, &g_fogLightSources[j].pos, &view);
        float du = (float)(pv.x>>16)/1920.0f - fl.x, dv = (float)(pv.y>>16)/1080.0f - fl.y;
        float d = du*du+dv*dv;
        if (d < best) { best = d; bi = j; }
    }
    if (bi < 0) return false;
    FogLightSource *src = &g_fogLightSources[bi];
    FixVector end = { src->pos.x + 30*src->dir.x, src->pos.y + 30*src->dir.y, src->pos.z + 30*src->dir.z };
    FixVector p0, p1;
    FixMatrix_ProjectWorldPointToView((int*)&p0, &src->pos, &view);
    FixMatrix_ProjectWorldPointToView((int*)&p1, &end, &view);
    double x = ((double)(p1.x>>16)-(double)(p0.x>>16))/1920.0*(1920.0/1080.0), y = ((double)(p1.y>>16)-(double)(p0.y>>16))/1080.0;
    double l = sqrt(x*x+y*y);
    if (l <= 1e-5) return false;
    *ax = x/l; *ay = y/l; return true;
}


int main() {
    for (int n = 0, i = 8; n < 4096; n++, i += 16)
        g_sqrtTable[n] = (unsigned short)(int)(sqrt((double)i / 65536.0) * 65536.0 + 0.5);
    memset(&g_gfx,0,sizeof g_gfx); g_gfx.resX=1920; g_gfx.resY=1080; g_pGraphics=&g_gfx;
    memset(&g_texManager,0,sizeof g_texManager);
    g_texManager.fixedProjection[0]=(int)(2.33711243*0.5*65536.0);
    g_texManager.fixedProjection[5]=(int)(4.19720459*0.5*65536.0);
    g_texManager.fixedProjection[10]=(int)(250.0/249.0*65536.0);
    g_texManager.fixedProjection[11]=0x10000;
    g_texManager.fixedProjection[14]=(int)(-250.0/249.0*65536.0);
    const double P33=250.0/249.0, P43=-250.0/249.0;
    const double lampYF=0.55, lampZF=2.05, lampYR=0.70, lampZR=-2.0, lampXF=0.65, lampXR=0.62;
    // Forward-headlight visibility: the direction can be camera-axial while
    // the offset lamps still project a useful run toward the road. The rear
    // axial case must stay dark. Exercise the same camera positions at level
    // pitch too, rather than letting camera pitch accidentally supply light.
    for (const ViewDef &v : VIEWS) {
        if (!strcmp(v.name,"side") || !strcmp(v.name,"chase-far")) continue;
        for (double pitch : {0.0, v.pitch}) {
            SceneNode view = {}; matFromYPR(&view.world,v.x,v.y,v.z,0,pitch);
            LightField_Clear();
            for (int side : {-1,1}) {
                FixVector pos={(int)(side*lampXF*65536),(int)(lampYF*65536),(int)(lampZF*65536)};
                FixVector dir={0,0,65536};
                LightField_Add(&pos,&dir,65536,65536,65536,65536);
            }
            GfxFogLight lights[LIGHT_FIELD_MAX];
            int n=FogBeam_ProjectLights(lights,LIGHT_FIELD_MAX,&view,1920,1080,P33,P43);
            float best=0;
            for (int i=0;i<n;i++) best=fmaxf(best,lights[i].beamLen);
            if (n!=2 || best<0.1f) fail(v.name,pitch,false,0,"forward axial lamps must retain their projected road run",best,0.1f);
        }
    }
    // Separate unbiased segment anchor from the bodywork self-occlusion
    // gate. Depth interpolation must not inflate near-clipped segments.
    for (const auto projection : {std::pair<double,double>{250.0/249.0,-250.0/249.0},
                                  std::pair<double,double>{1.0/1.5,-0.5/1.5}}) {
        SceneNode view={}; FixMatrix_Identity(&view.world);
        LightField_Clear();
        FixVector pos={65536,0,0},dir={0,0,65536};
        LightField_Add(&pos,&dir,65536,65536,65536,65536);
        GfxFogLight lights[LIGHT_FIELD_MAX];
        int n=FogBeam_ProjectLights(lights,LIGHT_FIELD_MAX,&view,1920,1080,projection.first,projection.second);
        if(n!=1 || lights[0].haloRef>=lights[0].depthRef)
            fail("clipped",0,false,0,"segment anchor must be unbiased",n?lights[0].haloRef:0,n?lights[0].depthRef:0);
        if(n==1) {
            const double expected=projection.first+projection.second/(0x10100/65536.0);
            if(fabs(lights[0].haloRef-expected)>1e-5)
                fail("clipped",0,false,0,"unbiased anchor matches clipped plane",lights[0].haloRef,expected);
        }
        // Raw depths are affine in 1/z; GL's 0.5+0.5*d mapping preserves
        // interpolation and ordering. A road at2.4 is behind the physical
        // halfway segment, but in front of the old biased gate.
        double a=projection.first+projection.second/1.0;
        double b=projection.first+projection.second/31.0;
        double biased=projection.first+projection.second/1.5;
        double road=projection.first+projection.second/2.4;
        for(bool half : {false,true}) {
            auto remap=[&](double d){return half?0.5+0.5*d:d;};
            double correct=(remap(a)+remap(b))*0.5;
            double wrong=(remap(biased)+remap(b))*0.5;
            if(!(correct<remap(road) && remap(road)<wrong))
                fail("clipped",0,false,0,"full/half range segment ordering",correct,remap(road));
        }
    }
    for (const ViewDef &v : VIEWS) {
        SceneNode view; memset(&view,0,sizeof view);
        matFromYPR(&view.world, v.x,v.y,v.z, v.yaw,v.pitch);
        for (int axis=0; axis<2; axis++) {
            double lo = axis==0 ? -40 : -12, hi = axis==0 ? 40 : 12, step = 1.0;
            GfxFogLight prev[LIGHT_FIELD_MAX]; int prevN = -1;
            double ptax[LIGHT_FIELD_MAX] = {0}, ptay[LIGHT_FIELD_MAX] = {0};
            for (double a=lo; a<=hi+0.001; a+=step) {
                FixMatrix car; matFromYPR(&car, 0,0,0, axis==0?a:0, axis==1?a:0);
                LightField_Clear();
                struct {double x,y,z,dx,dy,dz;} L[4] = {
                    {-lampXF,lampYF,lampZF, 0,0,1}, {lampXF,lampYF,lampZF, 0,0,1},
                    {-lampXR,lampYR,lampZR, 0,0,-1}, {lampXR,lampYR,lampZR, 0,0,-1}};
                for (auto &l : L) {
                    // Car at the world origin: rotation only, no translation.
                    FixVector lp={(int)(l.x*65536),(int)(l.y*65536),(int)(l.z*65536)};
                    FixVector ld={(int)(l.dx*65536),(int)(l.dy*65536),(int)(l.dz*65536)};
                    FixVector wp,wd; FixMatrix_RotateVector(&wp,&lp,&car); FixMatrix_RotateVector(&wd,&ld,&car);
                    LightField_Add(&wp,&wd,0x10000, 0x10000,0x10000,0x10000);
                }
                GfxFogLight fl[LIGHT_FIELD_MAX];
                int n = FogBeam_ProjectLights(fl, LIGHT_FIELD_MAX, &view, 1920.0f,1080.0f, (float)P33,(float)P43);
                double tax[LIGHT_FIELD_MAX], tay[LIGHT_FIELD_MAX];
                for (int i=0;i<n;i++) trueBeamAxis(fl[i], view, &tax[i], &tay[i]);
                for (int i=0;i<n;i++) {
                    // sentinel check: the behind-camera projector returns exactly
                    // (-100 px, -100 px) -> (-100/1920, -100/1080); real off-screen
                    // projections are fine (the beam can still enter the frame).
                    if (fabsf(fl[i].x - (-100.0f/1920.0f)) < 0.002f && fabsf(fl[i].y - (-100.0f/1080.0f)) < 0.002f)
                        fail(v.name,a,axis==0,i,"lamp uv is the behind-camera sentinel", fl[i].x, fl[i].y);
                    // halo pinned to the lamp: any divergence or invalid ref is a regression
                    if ((fl[i].haloX != fl[i].x || fl[i].haloY != fl[i].y) && (fl[i].haloRef <= 0.0f || fl[i].haloRef > 1.0f))
                        fail(v.name,a,axis==0,i,"halo gate invalid (ungated bubble)", fl[i].haloRef, 0);
                    if (fabsf(fl[i].haloX - (-100.0f/1920.0f)) < 0.002f && fabsf(fl[i].haloY - (-100.0f/1080.0f)) < 0.002f)
                        fail(v.name,a,axis==0,i,"halo uv is the behind-camera sentinel", fl[i].haloX, fl[i].haloY);
                    if (prevN == n && i < prevN) {
                        // direction flip at visible length on BOTH samples
                        if (fl[i].beamLen > 0.05f && prev[i].beamLen > 0.05f) {
                            float dot = fl[i].beamX*prev[i].beamX + fl[i].beamY*prev[i].beamY;
                            double tdot = tax[i]*ptax[i] + tay[i]*ptay[i];
                            if (dot < 0.0f && tdot >= 0.0) fail(v.name,a,axis==0,i,"wedge direction flip at visible length", prev[i].beamLen, fl[i].beamLen);
                            if (fabsf(fl[i].beamLen - prev[i].beamLen) > 0.15f)
                                fail(v.name,a,axis==0,i,"wedge length jump", prev[i].beamLen, fl[i].beamLen);
                        }
                        float hdx = fl[i].haloX-prev[i].haloX, hdy = fl[i].haloY-prev[i].haloY;
                        bool onscreen = fl[i].haloX > -0.05f && fl[i].haloX < 1.05f && fl[i].haloY > -0.05f && fl[i].haloY < 1.05f &&
                                        prev[i].haloX > -0.05f && prev[i].haloX < 1.05f && prev[i].haloY > -0.05f && prev[i].haloY < 1.05f;
                        if (onscreen && sqrtf(hdx*hdx+hdy*hdy) > 0.10f)
                            fail(v.name,a,axis==0,i,"halo position jump on screen", prev[i].haloX, fl[i].haloX);
                    }
                }
                memcpy(prev, fl, sizeof fl); memcpy(ptax, tax, sizeof tax); memcpy(ptay, tay, sizeof tay); prevN = n;
            }
        }
    }

    // Direction-length robustness: the beam direction is normalized before the
    // 30-unit beam-end extension, so scaled/short dirs must give the SAME beam
    // as the unit dir, and zero/tiny dirs must give no beam (never a crash).
    {
        SceneNode view; memset(&view,0,sizeof view);
        matFromYPR(&view.world, 10,0,0, -90,0);   // side view: fully transverse
        const int scales[] = {0x10000, 0x18000, 0x8000, 0x100};
        float refX=0, refLen=-1;
        for (int si=0; si<4; si++) {
            LightField_Clear();
            FixVector lp={0,0,0};
            FixVector ld={0,0,scales[si]};        // (0,0,k) - same axis, k from 1/256 to 1.5
            LightField_Add(&lp,&ld,0x10000, 0x10000,0x10000,0x10000);
            GfxFogLight fl[LIGHT_FIELD_MAX];
            int n = FogBeam_ProjectLights(fl, LIGHT_FIELD_MAX, &view, 1920.0f,1080.0f, (float)P33,(float)P43);
            if (n != 1) { fail("scale",si,true,0,"light count", n, 1); continue; }
            if (si == 0) { refX = fl[0].beamX; refLen = fl[0].beamLen; }
            else {
                if (fabsf(fl[0].beamX - refX) > 0.001f || fabsf(fl[0].beamLen - refLen) > 0.001f)
                    fail("scale",si,true,0,"scaled dir changes the beam", fl[0].beamLen, refLen);
            }
        }
        {   LightField_Clear();   // zero dir: no beam
            FixVector lp={0,0,0}, ld={0,0,0};
            LightField_Add(&lp,&ld,0x10000, 0x10000,0x10000,0x10000);
            GfxFogLight fl[LIGHT_FIELD_MAX];
            int n = FogBeam_ProjectLights(fl, LIGHT_FIELD_MAX, &view, 1920.0f,1080.0f, (float)P33,(float)P43);
            if (n != 1 || fl[0].beamLen != 0.0f) fail("scale",0,true,0,"zero dir must give no beam", n?fl[0].beamLen:-1, 0); }
        {   LightField_Clear();   // tiny dir (1,0,0): length rounds to zero - no beam, no div-by-zero
            FixVector lp={0,0,0}, ld={1,0,0};
            LightField_Add(&lp,&ld,0x10000, 0x10000,0x10000,0x10000);
            GfxFogLight fl[LIGHT_FIELD_MAX];
            int n = FogBeam_ProjectLights(fl, LIGHT_FIELD_MAX, &view, 1920.0f,1080.0f, (float)P33,(float)P43);
            if (n != 1 || fl[0].beamLen != 0.0f) fail("scale",0,true,0,"tiny dir must give no beam", n?fl[0].beamLen:-1, 0); }
    }
    // Rolled views (2026-11-01 user screenshots: detached near-vertical
    // streaks from a rolled car-mounted camera). The wedge direction must be
    // the TRUE projected lamp->beam-end axis; the view-space transverse
    // component used before is a tiny pitch residual for near-axial beams and
    // pointed 90-180 degrees off under roll. Two camera families, roll swept
    // 0..345 in 15 deg steps: (a) car-mounted offset rotated WITH the car,
    // (b) the user's actual rolled cameras from the in-game debug overlays
    // (side ~1.93, up ~0.5, dist ~2.0). Pins: wedge direction within 15 deg
    // of the independently projected lamp->end axis at visible length
    // (pre-fix measured 148 deg at roll180, 90 deg at the user cameras), and
    // no direction flip between adjacent roll samples.
    for (int fam=0; fam<2; fam++) {
        GfxFogLight prev[LIGHT_FIELD_MAX]; int prevN = -1;
        double ptax[LIGHT_FIELD_MAX] = {0}, ptay[LIGHT_FIELD_MAX] = {0};
        for (double roll=0; roll<360; roll+=15) {
            FixMatrix car; matFromYPRR(&car, 0,0,0, 0,0,roll);
            LightField_Clear();
            struct {double x,y,z,dx,dy,dz;} L[4] = {
                {-lampXF,lampYF,lampZF, 0,0,1}, {lampXF,lampYF,lampZF, 0,0,1},
                {-lampXR,lampYR,lampZR, 0,0,-1}, {lampXR,lampYR,lampZR, 0,0,-1}};
            for (auto &l : L) {
                FixVector lp={(int)(l.x*65536),(int)(l.y*65536),(int)(l.z*65536)};
                FixVector ld={(int)(l.dx*65536),(int)(l.dy*65536),(int)(l.dz*65536)};
                FixVector wp,wd; FixMatrix_RotateVector(&wp,&lp,&car); FixMatrix_RotateVector(&wd,&ld,&car);
                LightField_Add(&wp,&wd,0x10000, 0x10000,0x10000,0x10000);
            }
            // camera offset in CAR space, rotated by the car matrix
            double ox = fam==0 ? 0.0 : 1.93, oy = fam==0 ? 1.05 : 0.50, oz = fam==0 ? 0.35 : -1.99;
            double cx = ox*car.right.x/65536.0 + oy*car.up.x/65536.0 + oz*car.forward.x/65536.0;
            double cy = ox*car.right.y/65536.0 + oy*car.up.y/65536.0 + oz*car.forward.y/65536.0;
            double cz = ox*car.right.z/65536.0 + oy*car.up.z/65536.0 + oz*car.forward.z/65536.0;
            SceneNode view; memset(&view,0,sizeof view);
            matFromYPRR(&view.world, cx,cy,cz, 0,4, roll);
            GfxFogLight fl[LIGHT_FIELD_MAX];
            int n = FogBeam_ProjectLights(fl, LIGHT_FIELD_MAX, &view, 1920.0f,1080.0f, (float)P33,(float)P43);
            const char *famName = fam==0 ? "roll-carmount" : "roll-usercam";
            double tax[LIGHT_FIELD_MAX], tay[LIGHT_FIELD_MAX];
            for (int i=0;i<n;i++) trueBeamAxis(fl[i], view, &tax[i], &tay[i]);
            for (int i=0;i<n;i++) {
                if (fl[i].beamLen > 0.02f) {
                    double dot = tax[i]*fl[i].beamX + tay[i]*fl[i].beamY;
                    if (dot > 1.0) dot = 1.0; if (dot < -1.0) dot = -1.0;
                    double ang = acos(dot)*180.0/M_PI;
                    if (ang >= 15.0) { printf("FAIL %s roll=%.0f L%d wedge %.0f deg off the projected beam axis\n", famName, roll, i, ang); failures++; }
                }
                if (prevN == n && i < prevN && fl[i].beamLen > 0.05f && prev[i].beamLen > 0.05f) {
                    float dot = fl[i].beamX*prev[i].beamX + fl[i].beamY*prev[i].beamY;
                    double tdot = tax[i]*ptax[i] + tay[i]*ptay[i];
                    if (dot < 0.0f && tdot >= 0.0) fail(famName,roll,true,i,"wedge direction flip across roll", prev[i].beamLen, fl[i].beamLen);
                }
            }
            memcpy(prev, fl, sizeof fl); memcpy(ptax, tax, sizeof tax); memcpy(ptay, tay, sizeof tay); prevN = n;
        }
    }
    printf(failures==0 ? "SWEEP CLEAN\n" : "SWEEP: %d failures\n", failures);
    return failures != 0;
}
