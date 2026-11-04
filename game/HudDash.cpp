#include "CornerCameraRoll.h"
#include "CameraOperator.h"
#include "port/sys.h"
#include <stdio.h>
#include "FloatToInt.h"
#include <stdlib.h>
#include "Car.h"
#include "Font.h"
#include "Frontend.h"
#include "GameInfo.h"
#include "Graphics.h"
#include "InstallInfo.h"
#include "RallyData.h"
#include "Sprite.h"
#include "StageTiming.h"
#include "Texture.h"

// In-car dashboard of the HUD: the rev counter (an analogue dial with a
// needle, or a digital bar), the gear and the speed of each of the (up to
// two) players. The shown speed and revs are interpolated between the
// physics steps.

struct GenericFile;
extern char g_strPathConcat[];
int Car_GetWheelSpeed(Car *pCar, BYTE wheel, int unit);
BYTE *RallyData_GetDriverHudFlags(BYTE index);
BYTE StageUI_GetRaceEndEventCount(void);
extern float g_oneOverRandMax;
void StageTiming_ResetDashGearMarkers(void);

struct DashGearNames {
    char c[12];
};

// GLOBAL: CMR2 0x0053ce10
int g_dashGearMarkerY[2];       // gear marker position drawn last frame
// GLOBAL: CMR2 0x0053ce18
int g_dashGearFlash[2];         // counts down after a gear change
// GLOBAL: CMR2 0x0053ce38
int g_dashSpeed[2];             // interpolated speed
// GLOBAL: CMR2 0x0053ce40
int g_dashAspect;
// GLOBAL: CMR2 0x0053ce44
int g_dashSpeedNext[2];
// GLOBAL: CMR2 0x0053ce4c
int g_dashSimple;               // plain rectangles instead of the textured bar
// GLOBAL: CMR2 0x0053ce50
unsigned short *g_dashRevTicks;
// GLOBAL: CMR2 0x0053ce54
int g_dashRev[2];               // interpolated revs (1.0 = red line)
// GLOBAL: CMR2 0x0053ce5c
int g_dashLastResX;
// GLOBAL: CMR2 0x0053ce60
int g_dashRevPrev[2];
// GLOBAL: CMR2 0x0053ce68
int g_dashSpeedPrev[2];
// GLOBAL: CMR2 0x0053ce70
int g_dashGear[2];              // index into g_dashGearNames
// GLOBAL: CMR2 0x0053ce78
int g_dashDigital[8];           // per car: digital bar rather than the dial
// GLOBAL: CMR2 0x0053ce98
int g_dashGearMarker[2];        // gear marker position, eased towards the gear
// GLOBAL: CMR2 0x0053cfa0
int g_dashRevNext[2];
// GLOBAL: CMR2 0x0053cfa8
int g_dashIdle[2];              // steps spent standing still
// GLOBAL: CMR2 0x0053cfb0
int g_dashLastGear[2];
// GLOBAL: CMR2 0x0053cfd0
Texture *g_dashMphTexture;
// GLOBAL: CMR2 0x0053cfd4
Texture *g_dashKphTexture;
// GLOBAL: CMR2 0x0053cfd8
Texture *g_dashDialTexture;
// GLOBAL: CMR2 0x0053cfdc
Texture *g_dashBarTexture;
// GLOBAL: CMR2 0x0053cfe0
SpriteRect g_dashBarOff;        // unlit bar in g_dashBarTexture
// GLOBAL: CMR2 0x0053cfe8
SpriteRect g_dashBarOn;         // lit bar

// GLOBAL: CMR2 0x00519d08
SpriteRect g_dashDialSrc = {0, 0, 0xbe, 0xbe};
// GLOBAL: CMR2 0x00519d10
SpriteRect g_dashUnk0x00519d10 = {0, 0, 0xbe, 0xbe};
// GLOBAL: CMR2 0x00519d18
BYTE g_dashFlashAlpha[4] = {0xff, 0, 0, 0};
// GLOBAL: CMR2 0x00519d1c
BYTE g_dashWhite[4] = {0xff, 0xff, 0xff, 0xff};
// GLOBAL: CMR2 0x00519d20
BYTE g_dashShadow[4] = {0, 0, 0, 0xff};
// GLOBAL: CMR2 0x00519d24
BYTE g_dashBarColour[4] = {0, 0xf8, 0xf8, 0xff};
// GLOBAL: CMR2 0x00519d28
BYTE g_dashBarRedColour[4] = {0xf8, 0x34, 0, 0xff};
// GLOBAL: CMR2 0x00519d2c
BYTE g_dashNeedleColour[4] = {0xf6, 0xf6, 0xf6, 0xff};
// GLOBAL: CMR2 0x00519d30
BYTE g_dashUnk0x00519d30[4] = {0xf6, 0xf6, 0xf6, 0x78};
// GLOBAL: CMR2 0x00519d34
BYTE g_dashGearColour[4] = {0x65, 0x92, 0xff, 0xc8};
// GLOBAL: CMR2 0x00519d38
BYTE g_dashReverseColour[4] = {0xff, 0, 0, 0xc8};
// GLOBAL: CMR2 0x00519d3c
BYTE g_dashNeutralColour[4] = {0, 0xff, 0, 0xc8};
// GLOBAL: CMR2 0x00519d40
BYTE g_dashDialColour[4] = {0xff, 0xff, 0xff, 0xff};
// Per car: the digital bar instead of the dial.
// GLOBAL: CMR2 0x00519d44
int g_dashCarDigital[14] = {1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0};
// Lit width of the bar at each 1000 rpm step (640 and 1024 wide screens),
// then the last resolution height. Dash_DrawBar reads up to three steps past
// the table in use at the rev limiter: the 640 table runs into the 1024 one
// and that one into g_dashLastResY and the 0xffff after it, as in the original.
struct DashRevTables {
    unsigned short ticks640[28];    // 0x519d7c
    unsigned short ticks1024[28];   // 0x519db4
    int lastResY;                   // 0x519dec
    unsigned short field_0x74;      // 0x519df0
};
// GLOBAL: CMR2 0x00519d7c
DashRevTables g_dashRevTables = {
    {5,  8,  13, 16, 20, 24, 30, 36,  42,  48,  54,  60,  66,  72,
     78, 84, 91, 98, 105, 112, 119, 127, 135, 143, 151, 159, 167, 176},
    {7,   14,  21,  28,  36,  44,  52,  60,  69,  77,  87,  96,  105, 114,
     124, 135, 145, 157, 168, 180, 191, 203, 215, 228, 241, 254, 268, 282},
    -1,
    0xffff
};
#define g_dashRevTicks640 (g_dashRevTables.ticks640)
#define g_dashRevTicks1024 (g_dashRevTables.ticks1024)
#define g_dashLastResY (g_dashRevTables.lastResY)
// GLOBAL: CMR2 0x00519df4
char g_strDashDialTga[] = "\\NEWIMAGE\\INTERIOR\\REV_640.TGA";
// GLOBAL: CMR2 0x00519e14
char g_strDashBarTga[] = "\\NEWIMAGE\\OSD\\DIGITAL\\REVDIGITAL.TGA";
// GLOBAL: CMR2 0x00519e3c
char g_strDashKphTga[] = "\\NEWIMAGE\\INTERIOR\\KPH.TGA";
// GLOBAL: CMR2 0x00519e58
char g_strDashMphTga[] = "\\NEWIMAGE\\INTERIOR\\MPH.TGA";
// GLOBAL: CMR2 0x00519e74
char g_strDashSpeedFormat[] = "%s %03d";
// GLOBAL: CMR2 0x00519e7c
char g_strDashKmh[] = "KMH";
// GLOBAL: CMR2 0x00519e80
char g_strDashMph[] = "MPH";
// GLOBAL: CMR2 0x00519e84
char g_strDashChar[] = "%c";
// GLOBAL: CMR2 0x00519e88
DashGearNames g_dashGearNames = {"RN123456"};
// GLOBAL: CMR2 0x00519e94
char g_strDashSpeedDigits[] = "%03d";

// Screen coordinates as a fraction of the resolution.
#define DASH_X(f) (short)(FixMul(g_pGraphics->resX << 16, (f)) >> 16)
#define DASH_Y(f) (short)(FixMul(g_pGraphics->resY << 16, (f)) >> 16)
#define DASH_HIRES()                                                                                   \
    (CGameInfo::GetScreenWidth() >= 0x400 && CFrontend::IsTextureWidthSupported(0x400) && CFrontend::IsTextureHeightSupported(0x400))
#define DASH_RAND() (int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, rand()), g_oneOverRandMax), CGraphics::m_65536))

// FUNCTION: CMR2 0x00445a60
void Dash_LoadTextures(void)
{
    char path[260];
    bool loaded;

    sprintf(path, g_strPathConcat, CInstallInfo::GetTexturesDirectory(), g_strDashMphTga);
    g_dashMphTexture = CTexture::FindLoadTexture((GenericFile *)StageTiming_GetStageFile1(), path, &loaded, NULL, 0, 0);
    sprintf(path, g_strPathConcat, CInstallInfo::GetTexturesDirectory(), g_strDashKphTga);
    g_dashKphTexture = CTexture::FindLoadTexture((GenericFile *)StageTiming_GetStageFile1(), path, &loaded, NULL, 0, 0);
    sprintf(path, g_strPathConcat, CInstallInfo::GetTexturesDirectory(), g_strDashBarTga);
    g_dashBarTexture = CTexture::FindLoadTexture((GenericFile *)StageTiming_GetStageFile1(), path, &loaded, NULL, 0, 0);
    if (DASH_HIRES()) {
        g_dashBarOff.w = 0x125;
        g_dashBarOff.h = 0xc5;
        g_dashBarOff.x = 0;
        g_dashBarOff.y = 0;
        g_dashBarOn.w = 0x125;
        g_dashBarOn.h = 0xc5;
        g_dashBarOn.x = 0;
        g_dashBarOn.y = 0xc4;
    } else {
        g_dashBarOff.w = 0xb5;
        g_dashBarOff.h = 0x7b;
        g_dashBarOff.x = 0;
        g_dashBarOff.y = 0;
        g_dashBarOn.w = 0xb5;
        g_dashBarOn.h = 0x7b;
        g_dashBarOn.x = 0;
        g_dashBarOn.y = 0x7a;
    }
    sprintf(path, g_strPathConcat, CInstallInfo::GetTexturesDirectory(), g_strDashDialTga);
    g_dashDialTexture = CTexture::FindLoadTexture((GenericFile *)StageTiming_GetStageFile1(), path, &loaded, NULL, 0, 0);
    if (DASH_HIRES()) {
        g_dashDialSrc.x = 0;
        g_dashDialSrc.y = 0;
        g_dashDialSrc.w = 0xf4;
        g_dashDialSrc.h = 0xf4;
        return;
    }
    g_dashDialSrc.x = 0;
    g_dashDialSrc.y = 0;
    g_dashDialSrc.w = 0x96;
    g_dashDialSrc.h = 0x96;
}

// Picks the dial or the digital bar for every car.
// FUNCTION: CMR2 0x00445d30
void Dash_InitStyle(void)
{
    int i;
    int *p;

    i = 0;
    if (CGameInfo::GetConfiguredPlayerCount() > 0) {
        p = g_dashDigital;
        do {
            if (RallyData_IsHeadToHeadRaceMode() != 0) {
                *p = 1;
                g_dashSimple = 1;
            } else {
                if (CGameInfo::GetNetworkOptionBits1To2() == 0)
                    *p = g_dashCarDigital[CFrontend::GetArchivePrimaryIDEntry(RallyData_GetDriverRecordSelectionValue(i))];
                else
                    *p = CGameInfo::GetNetworkOptionBits1To2() == 1;
                g_dashSimple = 0;
            }
            i++;
            p++;
        } while (i < CGameInfo::GetConfiguredPlayerCount());
    }
}

// FUNCTION: CMR2 0x00445c80
void Dash_Reset(void)
{
    int i;

    for (i = 0; i < 2; i++) {
        g_dashGearMarker[i] = -1;
        g_dashSpeedNext[i] = 0;
        g_dashRevNext[i] = 0;
        g_dashGear[i] = 0;
        g_dashIdle[i] = 0;
        g_dashRevPrev[i] = 0;
        g_dashSpeedPrev[i] = 0;
        g_dashGearMarkerY[i] = 0;
    }
    Dash_InitStyle();
    if (DASH_HIRES()) {
        g_dashRevTicks = g_dashRevTicks1024;
        return;
    }
    g_dashRevTicks = g_dashRevTicks640;
}

// Shown speed and revs between the last two physics steps (t = 0..1).
// FUNCTION: CMR2 0x00445df0
void Dash_Interpolate(int t)
{
    int inv;
    unsigned int i;

    inv = 0x10000 - t;
    i = 0;
    if ((BYTE)RallyDataState() > 0) {
        do {
            g_dashSpeed[i] = FixMul(g_dashSpeedNext[i], t) + FixMul(g_dashSpeedPrev[i], inv);
            g_dashRev[i] = FixMul(g_dashRevNext[i], t) + FixMul(g_dashRevPrev[i], inv);
            i++;
        } while (i < (RallyDataState() & 0xff));
    }
}

// Physics step of the dashboard: smooths the speed and revs of the car
// (with an idle flicker while standing), and eases the gear marker.
// FUNCTION: CMR2 0x00445ea0
void Dash_Update(int player)
{
    int y;
    int d;
    int lim;

    if (player >= 2)
        return;
    if (g_dashGearFlash[player] > 0)
        g_dashGearFlash[player] = g_dashGearFlash[player] - 1;
    g_dashSpeedPrev[player] = g_dashSpeedNext[player];
    g_dashRevPrev[player] = g_dashRevNext[player];
    g_dashGear[player] = Car_Get(player)->gear;
    if ((BYTE)CGameInfo::IsDashOptionEnabled() != 0)
        g_dashSpeedNext[player] = FixMul(0x9999, Car_GetWheelSpeed(Car_Get(player), 0, 0)) +
                                  FixMul(0x6666, g_dashSpeedNext[player]);
    else
        g_dashSpeedNext[player] = FixMul(0x9999, Car_GetWheelSpeed(Car_Get(player), 0, 1)) +
                                  FixMul(0x6666, g_dashSpeedNext[player]);
    g_dashRevNext[player] =
        FixMul(0x9999, FixDiv(Car_Get(player)->field_0x7ac, Car_Get(player)->engineSpeedLimit)) +
        FixMul(0x6666, g_dashRevNext[player]);
    if (Car_Get(player)->field_0xb48 != 1) {
        if (g_dashSpeedNext[player] < 0x10000) {
            if (g_dashIdle[player] > 5) {
                if (g_dashRevNext[player] < 0x1eb8)
                    g_dashRevNext[player] = FixMul(DASH_RAND(), 0x7ae) + 0x170a;
            } else {
                g_dashIdle[player]++;
            }
        } else {
            g_dashIdle[player] = 0;
        }
    } else {
        if (g_dashSpeedNext[player] < 0x10000) {
            if (g_dashIdle[player] > 5) {
                if (g_dashRevNext[player] < 0x1eb8)
                    g_dashRevNext[player] = FixMul(DASH_RAND(), 0x7ae) + 0x170a;
            } else {
                g_dashIdle[player]++;
            }
        } else {
            g_dashIdle[player] = 0;
        }
    }
    if (g_dashRevNext[player] < 0x147a)
        g_dashRevNext[player] = FixMul(DASH_RAND(), 0x28f) + 0x11eb;
    g_dashSpeedNext[player] = FIX_ABS(g_dashSpeedNext[player]);
    if (g_dashGear[player] == 7)
        g_dashGear[player] = 0;
    else
        g_dashGear[player] = g_dashGear[player] + 1;
    // PORT: the marker moves on the HUD's screen (see Hud_BeginLayout).
    Hud_BeginLayout();
    y = FixMul(g_pGraphics->resY << 16, g_dashGear[player] * -0xae1 + 0xbe66) >> 16;
    if (g_dashGearMarker[player] == -1)
        g_dashGearMarker[player] = y;
    g_dashGearMarkerY[player] = g_dashGearMarker[player];
    d = y - g_dashGearMarker[player];
    lim = FixMul(g_pGraphics->resY << 16, 0x2d7) >> 16;
    Hud_EndLayout();
    if (d > lim)
        d = lim;
    if (d < -lim)
        d = -lim;
    g_dashGearMarker[player] += d;
    if (g_dashGear[player] != g_dashLastGear[player]) {
        if (g_dashGearFlash[player] == 0)
            g_dashGearFlash[player] = 6;
        g_dashLastGear[player] = g_dashGear[player];
    }
}

// Digital rev counter: the lit part of the bar texture (or plain
// rectangles), the gear letter and the speed.
// match 27%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00446270
void Dash_DrawBar(int player, int *pViewRect)
{
    char gears[] = "RN123456";
    char text[16];
    short rect[4];
    SpriteRect src;
    SpriteRect dst;
    BYTE colour[4];
    int lit;
    short dx;
    short dy;
    short scale;
    int len;
    int split;
    int rest;
    int pos;
    int index;
    int frac;
    int extra;

    dx = 0;
    dy = 0;
    lit = 0;
    if (g_dashSimple == 0) {
        pos = FixMul(0x250000, g_dashRev[player]) - 0xa0000;
        index = pos >> 16;
        if (index >= 0) {
            lit = g_dashRevTicks[index];
            scale = (int)FTOI(__int64, X87S_MUL((double)(int)(g_dashRevTicks[index + 1] - lit), CGraphics::m_65536));
            frac = pos - (index << 16);
            lit = lit + (FixMul(frac, scale) >> 16);
            if ((int)lit >= (int)g_dashRevTicks[27])
                lit = g_dashRevTicks[27];
            if (DASH_HIRES())
                extra = 5;
            else
                extra = 3;
            if ((int)(extra + lit) >= g_dashBarOn.w)
                extra = g_dashBarOn.w - lit - 1;
            src = g_dashBarOn;
            src.w = (short)lit;
            dst = src;
            dst.x = DASH_X(0xfd70) - g_dashBarOn.w;
            dst.y = DASH_Y(0xfd70) - g_dashBarOn.h;
            Sprite_Queue(&src, &dst, g_dashBarTexture, 3, 0, NULL, NULL, g_dashWhite, 8);
            if (extra != 0) {
                *(DWORD *)colour = *(DWORD *)g_dashWhite;
                dst.x = dst.x + dst.w;
                src.x = src.x + src.w;
                colour[3] = 0x5a;
                dst.w = (short)extra;
                src.w = (short)extra;
                Sprite_Queue(&src, &dst, g_dashBarTexture, 3, 0, NULL, NULL, colour, 8);
            }
        }
        src = g_dashBarOn;
        src.w = (short)lit;
        dst = src;
        dst.x = DASH_X(0xfd70) - g_dashBarOn.w;
        dst.y = DASH_Y(0xfd70) - g_dashBarOn.h;
        Sprite_Queue(&src, &dst, g_dashBarTexture, 3, 0, NULL, NULL, g_dashWhite, 8);
        dst.x = DASH_X(0xfd70) - g_dashBarOn.w;
        dst.y = DASH_Y(0xfd70) - g_dashBarOn.h;
        dst.w = g_dashBarOn.w;
        dst.h = g_dashBarOn.h;
        src = g_dashBarOff;
        dst.x = DASH_X(0xfd70) - dst.w;
        Sprite_Queue(&src, &dst, g_dashBarTexture, 3, 0, NULL, NULL, g_dashWhite, 8);
    } else {
        scale = (int)FTOI(__int64, X87S_MUL((double)((g_pGraphics->resX * 0x4b) / 640), CGraphics::m_65536));
        len = FixMul(g_dashRevNext[player], scale) >> 16;
        if (CGameInfo::IsSplitBarEnabled() == 0) {
            if (player == 0)
                dx = -(short)(g_pGraphics->resX / 2);
        } else if (player == 0) {
            dy = -(short)(g_pGraphics->resY / 2);
        }
        split = (g_pGraphics->resY * 0x41) / 480;
        if (len > split) {
            rest = len - split;
        } else {
            split = len;
            rest = 0;
        }
        rect[0] = DASH_X(0xfd70) - (short)((g_pGraphics->resX * 0x82) / 640) + dx;
        rect[1] = DASH_Y(0xfd70) - (short)((g_pGraphics->resY * 0x28) / 480) + dy;
        rect[2] = (short)split;
        rect[3] = (short)((g_pGraphics->resY * 0x14) / 480);
        Sprite_FillRect(&g_pGraphics->field309_0x150, rect, g_dashBarColour, 3);
        if (rest != 0) {
            rect[0] = rect[0] + rect[2];
            rect[2] = (short)rest;
            Sprite_FillRect(&g_pGraphics->field309_0x150, rect, g_dashBarRedColour, 3);
        }
        rect[0] = DASH_X(0xfd70) - (short)((g_pGraphics->resX * 0x82) / 640) - 1 + dx;
        rect[2] = 1;
        rect[1] = DASH_Y(0xfd70) - (short)((g_pGraphics->resY * 0x28) / 480) - 1 + dy;
        rect[3] = (short)((g_pGraphics->resY * 0x14) / 480) + 2;
        Sprite_FillRect(&g_pGraphics->field309_0x150, rect, g_dashWhite, 3);
        rect[3] = 1;
        rect[2] = (short)((g_pGraphics->resX * 0x4b) / 640) + 2;
        Sprite_FillRect(&g_pGraphics->field309_0x150, rect, g_dashWhite, 3);
        rect[2] = 1;
        rect[0] = DASH_X(0xfd70) + 1 + dx +
                  ((short)((g_pGraphics->resX * 0x4b) / 640) - (short)((g_pGraphics->resX * 0x82) / 640));
        rect[3] = (short)((g_pGraphics->resY * 0x14) / 480) + 2;
        Sprite_FillRect(&g_pGraphics->field309_0x150, rect, g_dashWhite, 3);
        rect[0] = DASH_X(0xfd70) - (short)((g_pGraphics->resX * 0x82) / 640) - 1 + dx;
        rect[1] = DASH_Y(0xfd70) + dy +
                  ((short)((g_pGraphics->resY * 0x14) / 480) - (short)((g_pGraphics->resY * 0x28) / 480));
        rect[3] = 1;
        rect[2] = (short)((g_pGraphics->resX * 0x4b) / 640) + 2;
        Sprite_FillRect(&g_pGraphics->field309_0x150, rect, g_dashWhite, 3);
    }

    // Gear and speed.
    rect[0] = DASH_X(0xfd70) + 1 + dx;
    rect[1] = DASH_Y(0xfd70) + 3 + dy;
    sprintf(text, g_strDashChar, gears[g_dashGear[player]]);
    rect[1] = rect[1] + (short)((g_pGraphics->resY * -2) / 480);
    Font_DrawText(6, text, rect[0], rect[1], (int *)g_dashWhite, 0x24);
    rect[0] = rect[0] + (short)((g_pGraphics->resX * -0x67) / 640);
    rect[1] = rect[1] + (short)((g_pGraphics->resY * 3) / 480);
    if ((BYTE)CGameInfo::IsDashOptionEnabled() != 0)
        sprintf(text, "%s", g_strDashMph);
    else
        sprintf(text, "%s", g_strDashKmh);
    sprintf(CFrontend::m_stringDest, g_strDashSpeedFormat, text, g_dashSpeedNext[player] >> 16);
    rect[1] = rect[1] + (short)((g_pGraphics->resY * -4) / 480);
    Font_DrawText(5, CFrontend::m_stringDest, rect[0] + 1, rect[1] + 1, (int *)g_dashShadow, 0x21);
    Font_DrawText(5, CFrontend::m_stringDest, rect[0], rect[1], (int *)g_dashWhite, 0x21);
}

// Draws the dial face of the rev counter centred on pCentre (fractions of
// the screen).
// FUNCTION: CMR2 0x00447100
void Dash_DrawDialFace(int *pCentre, int unused, Texture *pTexture)
{
    SpriteRect dst;
    int x;
    int y;
    int w;
    int h;

    x = FixMul(g_pGraphics->resX << 16, pCentre[0]) >> 16;
    y = FixMul(g_pGraphics->resY << 16, pCentre[1]) >> 16;
    w = g_dashDialSrc.w;
    h = g_dashDialSrc.h;
    dst.x = x - w;
    dst.y = y - h / 2;
    dst.w = w;
    dst.h = h;
    Sprite_Queue(&g_dashDialSrc, &dst, pTexture, 2, 0, NULL, NULL, g_dashDialColour, 8);
}

// Draws the needle of the dial: a thin quad from the tail to the tip, with
// its point, rotated by angle about pCentre.
// FUNCTION: CMR2 0x004471b0
void Dash_DrawNeedle(int *pCentre, int width, int tipWidth, int tail, int mid, int tip, unsigned int angle,
                     BYTE *pColour, int *pViewRect)
{
    BYTE edge[4];
    BYTE point[4];
    BYTE body[4];
    int a[2];
    int b[2];
    int c[2];
    int d[2];
    int e[2];
    int w1[2];
    int w2[2];
    int tipV[2];
    int midV[2];
    int tailV[2];
    int sx;
    int sy;
    int cx;
    int cy;

    point[0] = 0xf6;
    point[1] = 0xf6;
    point[2] = 0xf6;
    point[3] = pColour[3];
    body[0] = 0xfa;
    body[1] = 0xfa;
    body[2] = 0xfa;
    body[3] = pColour[3];
    edge[0] = 0xf6;
    edge[1] = 0xf6;
    edge[2] = 0xf6;
    edge[3] = pColour[3];
    if (DASH_HIRES()) {
        sx = 0x4000000;
        sy = 0x3000000;
    } else {
        sx = 0x2800000;
        sy = 0x1e00000;
    }
    cx = FixMul(pCentre[0], g_pGraphics->resX << 16);
    cy = FixMul(pCentre[1], g_pGraphics->resY << 16);
    cx += -(g_dashDialSrc.w / 2) << 16;
    w1[0] = FixMul(width, g_sinTable[angle & 0xfff]);
    w1[1] = FixMul(width, g_sinTable[(angle + 0x400) & 0xfff]);
    w1[0] = FixMul(w1[0], sx);
    w1[1] = FixMul(w1[1], sy);
    w1[1] = FixMul(w1[1], g_dashAspect);
    w2[0] = FixMul(tipWidth, g_sinTable[angle & 0xfff]);
    w2[1] = FixMul(tipWidth, g_sinTable[(angle + 0x400) & 0xfff]);
    w2[0] = FixMul(w2[0], sx);
    w2[1] = FixMul(w2[1], sy);
    w2[1] = FixMul(w2[1], g_dashAspect);
    tipV[0] = -FixMul(tip, g_sinTable[(angle + 0x400) & 0xfff]);
    tipV[1] = FixMul(tip, g_sinTable[angle & 0xfff]);
    tipV[0] = FixMul(tipV[0], sx);
    tipV[1] = FixMul(tipV[1], sy);
    tipV[1] = FixMul(tipV[1], g_dashAspect);
    midV[0] = -FixMul(mid, g_sinTable[(angle + 0x400) & 0xfff]);
    midV[1] = FixMul(mid, g_sinTable[angle & 0xfff]);
    midV[0] = FixMul(midV[0], sx);
    midV[1] = FixMul(midV[1], sy);
    midV[1] = FixMul(midV[1], g_dashAspect);
    tailV[0] = -FixMul(tail, g_sinTable[(angle + 0x400) & 0xfff]);
    tailV[1] = FixMul(tail, g_sinTable[angle & 0xfff]);
    tailV[0] = FixMul(tailV[0], sx);
    tailV[1] = FixMul(tailV[1], sy);
    tailV[1] = FixMul(tailV[1], g_dashAspect);
    a[0] = tailV[0] + w1[0] + cx;
    a[1] = w1[1] + tailV[1] + cy;
    b[0] = tailV[0] - w1[0] + cx;
    b[1] = tailV[1] - w1[1] + cy;
    c[0] = midV[0] + w2[0] + cx;
    c[1] = w2[1] + midV[1] + cy;
    d[0] = midV[0] - w2[0] + cx;
    d[1] = midV[1] - w2[1] + cy;
    e[0] = tipV[0] + cx;
    e[1] = tipV[1] + cy;
    Tri2D_Queue(a, c, e, point, 2);
    Tri2D_Queue(a, e, b, body, 2);
    Tri2D_Queue(d, b, e, edge, 2);
}

// Analogue rev counter: the dial with its needle, the gear marker beside
// the gear letters, the speed and the MPH/KPH plate.
// match 99.7%: only the order of two `and`s differs (100% ignoring registers).
// FUNCTION: CMR2 0x00446bf0
void Dash_DrawDial(int player, int *pViewRect)
{
    char gears[] = "RN123456";
    char speed[12];
    char unit[12];
    char letter[2];
    int dialCentre[2];
    int needleCentre[2];
    short rect[4];
    SpriteRect src;
    SpriteRect dst;
    BYTE colour[4];
    BYTE shadow[4];
    Texture *pTexture;
    short angle;
    int flash;
    int i;
    int k;

    sprintf(speed, g_strDashSpeedDigits, g_dashSpeedNext[player] >> 16);
    sprintf(unit, "%s", g_strDashMph);
    g_dashAspect = 0x15553;
    if (g_pGraphics->resX != g_dashLastResX || g_pGraphics->resY != g_dashLastResY) {
        StageTiming_ResetDashGearMarkers();
        g_dashLastResX = g_pGraphics->resX;
        g_dashLastResY = g_pGraphics->resY;
    }
    k = FixMul(FixDiv(g_dashSpeedPrev[player] + g_dashSpeedNext[player], 0x640000), 0x9110000);
    dialCentre[0] = 0xf333;
    dialCentre[1] = 0xd113;
    needleCentre[0] = 0xf333;
    needleCentre[1] = 0xd113;
    angle = (short)(0x732 - (short)(FixMul(g_dashRev[player] + 0x1eb8, 0xb330000) >> 16));
    Dash_DrawNeedle(needleCentre, 0x100, 0xcc, -0x8cc, 0x1799, 0x18cc, angle, g_dashNeedleColour, pViewRect);
    Dash_DrawDialFace(dialCentre, 0x3c01, g_dashDialTexture);
    Dash_DrawNeedle(needleCentre, 0x100, 0x100, -0x8cc, 0x1799, 0x18cc, angle, g_dashNeedleColour, pViewRect);

    // Gear marker, flashing white after a gear change.
    rect[3] = DASH_Y(0x886);
    rect[2] = DASH_X(0x666);
    rect[0] = DASH_X(0xf45a) - 5;
    if (g_dashGear[player] == 0)
        *(DWORD *)colour = *(DWORD *)g_dashReverseColour;
    else if (g_dashGear[player] == 1)
        *(DWORD *)colour = *(DWORD *)g_dashNeutralColour;
    else
        *(DWORD *)colour = *(DWORD *)g_dashGearColour;
    flash = g_dashGearFlash[player];
    if (flash > 0) {
        if (flash > 3) {
            colour[0] = 0xff;
            colour[1] = 0xff;
            colour[2] = 0xff;
            colour[3] = g_dashFlashAlpha[0];
        } else {
            colour[0] += ((0xff - colour[0]) * flash) / 3;
            colour[1] += ((0xff - colour[1]) * flash) / 3;
            colour[2] += ((0xff - colour[2]) * flash) / 3;
            colour[3] += ((g_dashFlashAlpha[0] - colour[3]) * flash) / 3;
        }
    }
    rect[1] = (short)g_dashGearMarkerY[player];
    *(DWORD *)shadow = *(DWORD *)colour;
    shadow[3] = colour[3] >> 1;
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, shadow, 3);
    rect[1] = (short)g_dashGearMarker[player];
    Sprite_FillRect(&g_pGraphics->field309_0x150, rect, colour, 3);
    Font_DrawText(4, speed, FixMul(g_pGraphics->resX << 16, 0xf467) >> 16, FixMul(g_pGraphics->resY << 16, 0xd90f) >> 16,
                  (int *)g_dashWhite, 0x24);

    // Speed unit plate.
    src.x = 0;
    src.y = 0;
    pTexture = (BYTE)CGameInfo::IsDashOptionEnabled() != 0 ? g_dashMphTexture : g_dashKphTexture;
    src.w = pTexture->width;
    src.h = pTexture->height;
    dst.x = DASH_X(0xf78d);
    dst.y = (short)((int)(g_pGraphics->resY * 0xd113) >> 16);
    dst.w = src.w;
    dst.h = src.h;
    if (DASH_HIRES()) {
        dst.x = dst.x - src.w / 2;
        dst.y = dst.y - 0x12;
    } else {
        dst.x = dst.x - src.w / 2;
        dst.y = dst.y - 0xc;
    }
    if ((BYTE)CGameInfo::IsDashOptionEnabled() != 0)
        Sprite_Queue(&src, &dst, g_dashMphTexture, 2, 0, NULL, NULL,
                     g_dashWhite, 8);
    else
        Sprite_Queue(&src, &dst, g_dashKphTexture, 2, 0, NULL, NULL,
                     g_dashWhite, 8);

    // Gear letters down the side of the dial.
    for (i = 0; i < 8; i++) {
        sprintf(letter, g_strDashChar, gears[i]);
        Font_DrawText(0, letter, (FixMul(g_pGraphics->resX << 16, 0xf78d) >> 16) - 5,
                      FixMul(g_pGraphics->resY << 16, 0xc553 - i * 0xae1) >> 16, (int *)g_dashWhite, 0x12);
    }
}

// Draws the dashboard of a player whose car shows one.
// FUNCTION: CMR2 0x00446210
void Dash_Draw(int player, int *pViewRect)
{
    if (player < 2 && (*RallyData_GetDriverHudFlags(StageUI_GetRaceEndEventCount() + (char)player) & 0x10) != 0) {
        if (g_dashDigital[StageUI_GetRaceEndEventCount() + player] != 0) {
            Dash_DrawBar(player, pViewRect);
            return;
        }
        Dash_DrawDial(player, pViewRect);
    }
}

void RallyData_GetDriverCameraOffsets(FixVector *pOffset, short *pPitch, int *pLag, int index);
void ChaseCamera_BuildMatrix(CameraRecord *pObj, FixMatrix *pRef);
void ChaseCamera_SetOffset(BYTE index, FixVector *pOffset);
void ChaseCamera_SetPitch(BYTE index, short value);
void ChaseCamera_SetLag(BYTE index, int value);
extern BYTE g_playerCameraMode[8];
// OpenCMR2 drone camera: an additional variant of the original chase view (camera
// type 5). Per view record. The original chase camera is unchanged when the variant is 0.
BYTE g_droneVariant[8];
// Set by the camera cycle just before it restarts type 5: next variant (0/1), -1 none.
signed char g_droneOverride[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
// 1 once the cycle has shown both variants for this record, so the next press moves on.
BYTE g_dronePairDone[8];


// Chase camera pitch of each player, and from index 2 the copy reduced with
// the camera's distance that ChaseCamera_BuildMatrix applies.
// GLOBAL: CMR2 0x0053d090
short g_chaseCameraPitch[4];
// Follow time of each player's chase camera (ChaseCameraSettings::lag).
// GLOBAL: CMR2 0x0053d098
int g_chaseCameraLag[4];

// FUNCTION: CMR2 0x00447e00
short ChaseCamera_GetPitch(BYTE index)
{
    return g_chaseCameraPitch[index];
}

extern FixVector g_chaseCameraOffsets[6];

// Sets a player's chase camera pitch and the copy of it reduced as the camera
// moves away from the car (index + 2).
// FUNCTION: CMR2 0x00447e20
void ChaseCamera_SetPitch(BYTE index, short value)
{

    g_chaseCameraPitch[index] = value;
    g_chaseCameraPitch[index + 2] =
        (short)FixMul(0x10000 - FixMul(FixDiv(g_chaseCameraOffsets[index].z - 0x50000, 0x50000), 0xcccc), value);
}

// FUNCTION: CMR2 0x00447ea0
int ChaseCamera_GetLag(BYTE index)
{
    return g_chaseCameraLag[index];
}

// FUNCTION: CMR2 0x00447ec0
void ChaseCamera_SetLag(BYTE index, int value)
{
    g_chaseCameraLag[index] = value;
}

// Chase camera offset of each player (entries 0, 1), then the up vector of
// each view record's chase camera (2 + record index).
// GLOBAL: CMR2 0x0053d048
FixVector g_chaseCameraOffsets[6];

// FUNCTION: CMR2 0x00447cf0
void ChaseCamera_GetOffset(FixVector *pOut, BYTE index)
{
    *pOut = g_chaseCameraOffsets[index];
}

// Chase camera mode of each view record: 0, 4 or 3 for camera types 4, 5 and
// 6 (Camera_Start); 4 uses the player's own offset (g_chaseCameraViewOffsets),
// the others g_fixedCameraDefaultOffsets.
// GLOBAL: CMR2 0x0053cff8
BYTE g_playerCameraMode[8];
// Each player's view offset derived from the chase camera offset (height
// eased with the distance, z negated: behind the car), then the forward vector
// of each view record's chase camera (2 + record index).
// GLOBAL: CMR2 0x0053d000
FixVector g_chaseCameraViewOffsets[6];
// Default view offsets of the fixed camera modes.
// GLOBAL: CMR2 0x00519ea0
FixVector g_fixedCameraDefaultOffsets[3] = { { 0, 0x13333, -0x50000 }, { 0, 0x13333, -0x50000 }, { 0, 0x13333, -0x50000 } };

int Race_IsMultiplayerRecordMode10(void);
int RallyData_IsHeadToHeadRaceMode(void);

// View offset of a player's camera (lowered in the split-screen cockpit view).
// FUNCTION: CMR2 0x00447ee0
void ChaseCamera_GetViewOffset(FixVector *pOut, CameraRecord *pSel)
{
    BYTE mode;
    FixVector *p;

    mode = g_playerCameraMode[pSel->index];
    if (mode == 4)
        p = &g_chaseCameraViewOffsets[pSel->view];
    else
        p = &g_fixedCameraDefaultOffsets[mode];
    *pOut = *p;
    if (Race_IsMultiplayerRecordMode10() == 0 && RallyData_IsHeadToHeadRaceMode() != 0 && CGameInfo::IsSplitBarEnabled()) {
        pOut->y -= 0x3333;
        pOut->z -= 0x9999;
    }
}

void FixMatrix_GetUp(FixVector *pOut, FixMatrix *pM);
void FixMatrix_GetRight(FixVector *pOut, FixMatrix *pM);

// Takes the camera up/right vectors of a view from the matrix, or copies them
// from the other view when both follow the same car in modes 4..6.
// FUNCTION: CMR2 0x00447be0
void ChaseCamera_BeginEnter(CameraRecord *pRecord, CameraRecord *pOther, FixMatrix *pRef)
{
    int mode;

    if (pOther->car == pRecord->car) {
        mode = pOther->type;
        if (mode == 4 || mode == 5 || mode == 6) {
            g_chaseCameraOffsets[2 + pRecord->index] = g_chaseCameraOffsets[2 + pOther->index];
            g_chaseCameraViewOffsets[2 + pRecord->index] = g_chaseCameraViewOffsets[2 + pOther->index];
            return;
        }
    }
    FixMatrix_GetUp(&g_chaseCameraOffsets[2 + pRecord->index], pRef);
    FixMatrix_GetRight(&g_chaseCameraViewOffsets[2 + pRecord->index], pRef);
}

// Sets a player's chase camera offset and the view offset derived from it, then
// recomputes the distance-scaled pitch.
// FUNCTION: CMR2 0x00447d20
void ChaseCamera_SetOffset(BYTE index, FixVector *pOffset)
{
    int i = index;

    g_chaseCameraOffsets[i] = *pOffset;
    g_chaseCameraViewOffsets[i] = *pOffset;
    g_chaseCameraViewOffsets[i].y =
        0x20000 - FixMul(0x10000 - FixMul(FixDiv(g_chaseCameraOffsets[i].z - 0x50000, 0x50000), 0xcccc),
                         0x20000 - g_chaseCameraViewOffsets[i].y);
    g_chaseCameraViewOffsets[i].z = -g_chaseCameraViewOffsets[i].z;
    ChaseCamera_SetPitch(index, ChaseCamera_GetPitch(index));
}

// Resets a player's camera to the default offset, height and distance.
// FUNCTION: CMR2 0x00447ca0
void ChaseCamera_Reset(BYTE index)
{
    FixVector offset;

    offset.x = 0;
    offset.y = 0x18000;
    offset.z = 0x68000;
    ChaseCamera_SetOffset(index, &offset);
    ChaseCamera_SetPitch(index, 0x2d);
    ChaseCamera_SetLag(index, 0xe0000);
}

void RallyData_GetDriverCameraOffsets(FixVector *pOffset, short *pPitch, int *pLag, int index);
void ChaseCamera_BuildMatrix(CameraRecord *pObj, FixMatrix *pRef);

// Starts a chase camera (types 4, 5, 6 as modes 0, 4, 3): applies the chase
// camera settings of the followed car's driver (driver 0 past the drivers of
// this event) to the view, then builds the matrix.
// match 29.79%: implementada; MSVC6 mantiene param_1 en ESI (y no en EDI) y el marco es de
// 12 bytes en vez de 16, asi que todos los desplazamientos de pila difieren; la logica y el
// orden de llamadas coinciden.
// FUNCTION: CMR2 0x00447530
void ChaseCamera_Start(CameraRecord *param_1, FixMatrix *param_2, int param_3)
{
    FixVector offset;
    short heading;
    BYTE index;
    CameraRecord *pSettings = param_1;

    BYTE prevMode = pSettings->index < 8 ? g_playerCameraMode[pSettings->index] : 0xff;
    CameraOperator_Reset(pSettings->index);
    CornerCameraRoll_Reset(pSettings->index);
    index = pSettings->view;
    g_playerCameraMode[pSettings->index] = (BYTE)param_3;
    if (pSettings->index < 8) {
        if (param_3 != 4) {
            g_droneVariant[pSettings->index] = 0;
            g_dronePairDone[pSettings->index] = 0;
        } else if (g_droneOverride[pSettings->index] >= 0) {
            g_droneVariant[pSettings->index] = (BYTE)g_droneOverride[pSettings->index];
            g_dronePairDone[pSettings->index] = 1;
            g_droneOverride[pSettings->index] = -1;
        } else if (prevMode == 4) {
            // Restart of the view already in this mode (replay, split-screen change, reload):
            // keep the stop the player chose, classic or drone.
        } else {
            // Not from the cycle (menu, saved selection): keep the old behaviour, drone
            // when the option is on.
            g_droneVariant[pSettings->index] = Sys_GetOption("video.camera_operator", 0) != 0;
            g_dronePairDone[pSettings->index] = 0;
        }
    }
    if (pSettings->car >= (BYTE)RallyDataState()) {
        RallyData_GetDriverCameraOffsets(&offset, &heading, &param_3, 0);
        ChaseCamera_SetOffset(index, &offset);
        ChaseCamera_SetLag(index, param_3);
        ChaseCamera_SetPitch(index, heading);
    } else {
        RallyData_GetDriverCameraOffsets(&offset, &heading, &param_3,
                               (int)(BYTE)StageUI_GetRaceEndEventCount() + pSettings->car);
        ChaseCamera_SetOffset(index, &offset);
        ChaseCamera_SetLag(index, param_3);
        ChaseCamera_SetPitch(index, heading);
    }
    ChaseCamera_BuildMatrix(pSettings, param_2);
}

void FixMatrix_RebuildBasis(FixMatrix *pOut);

// Rebuilds a cockpit view matrix: an orthonormal basis is built from the
// player's stored up/right vectors, rolled 45 degrees when the player's camera
// mode is 3, and then interpolated towards the previous view unless every axis
// is already within 0x51e of it.
// match 70%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// Implementada; MSVC6 reparte los locales al reves (marco 0xa8 en vez de 0xb0, ESI/EDI invertidos); la logica y el orden de llamadas son exactos.
// FUNCTION: CMR2 0x004475f0
void ChaseCamera_Update(CameraRecord *param_1, FixMatrix *param_2, int param_3)
{
    FixVector *pUp;
    FixVector *pRight;
    FixVector cross;
    FixMatrix mat;
    FixMatrix xform;
    int index;

    FixVector operatorOffset;ChaseCamera_GetViewOffset(&operatorOffset,param_1);
    CameraOperator_Update(param_1,param_2,&operatorOffset,g_chaseCameraPitch[2+param_1->view]);
    CornerCameraRoll_Update(param_1,param_2);
    index = param_1->index;
    pRight = &g_chaseCameraViewOffsets[2 + index];
    pUp = &g_chaseCameraOffsets[2 + index];
    FixVecCross(&cross, pUp, pRight);
    FixMatrix_Identity(&mat);
    FixMatrix_SetRight(&cross, &mat);
    FixMatrix_SetUp(pUp, &mat);
    FixMatrix_SetForward(pRight, &mat);

    if (param_3 != 0) {
        xform = mat;
        FixMatrix_RebuildBasis(&xform);
    } else {
        xform = *param_2;
        xform.forward.x = param_2->right.x;
        xform.forward.y = param_2->right.y;
        xform.forward.z = param_2->right.z;
        xform.right.x = -param_2->forward.x;
        xform.right.y = -param_2->forward.y;
        xform.right.z = -param_2->forward.z;
    }

    if (g_playerCameraMode[index] == 3) {
        FixVector up;

        up = xform.up;
        xform.up.x = FixMul(xform.forward.x, g_sinTable[0x600]) + FixMul(up.x, g_sinTable[0x200]);
        xform.up.y = FixMul(xform.forward.y, g_sinTable[0x600]) + FixMul(up.y, g_sinTable[0x200]);
        xform.up.z = FixMul(xform.forward.z, g_sinTable[0x600]) + FixMul(up.z, g_sinTable[0x200]);
        xform.forward.x = FixMul(xform.forward.x, g_sinTable[0x200]) - FixMul(up.x, g_sinTable[0x600]);
        xform.forward.y = FixMul(xform.forward.y, g_sinTable[0x200]) - FixMul(up.y, g_sinTable[0x600]);
        xform.forward.z = FixMul(xform.forward.z, g_sinTable[0x200]) - FixMul(up.z, g_sinTable[0x600]);
    }

    if (FIX_ABS(mat.right.x - xform.right.x) >= 0x51e ||
        FIX_ABS(mat.right.y - xform.right.y) >= 0x51e ||
        FIX_ABS(mat.right.z - xform.right.z) >= 0x51e ||
        FIX_ABS(mat.up.x - xform.up.x) >= 0x51e ||
        FIX_ABS(mat.up.y - xform.up.y) >= 0x51e ||
        FIX_ABS(mat.up.z - xform.up.z) >= 0x51e ||
        FIX_ABS(mat.forward.x - xform.forward.x) >= 0x51e ||
        FIX_ABS(mat.forward.y - xform.forward.y) >= 0x51e ||
        FIX_ABS(mat.forward.z - xform.forward.z) >= 0x51e) {
        int scale;
        int t;

        if (g_playerCameraMode[index] == 4)
            scale = g_chaseCameraLag[param_1->view];
        else
            scale = 0xe0000;
        t = FixMul(FixDiv(g_physicsTimeStep, scale),
                   Car_Get(param_1->car)->surfaceNoise);
        FixMatrix_Interpolate(&xform, &mat, &xform, t, t, 0x10000, 0);
        FixMatrix_GetUp(pUp, &xform);
        FixMatrix_GetForward(pRight, &xform);
    }
    ChaseCamera_BuildMatrix(param_1, param_2);
}

// Mirror of the CarStageTiming layout that StageTiming.cpp owns; the race
// reset below writes the per-driver timing state directly through it.
struct CarStageTiming {
    int startTime;          // 0x00
    int field_0x4[9];       // 0x04
    int splits[9];          // 0x28
    int splitTimes[12];     // 0x4c
    int lastTime;           // 0x7c
    char field_0x80;        // 0x80
    char field_0x81;        // 0x81
    char field_0x82;        // 0x82
    BYTE field_0x83;        // 0x83
    BYTE field_0x84;        // 0x84
    BYTE pad_0x85[3];
};

extern char g_unk0x0053ddb0[108];
extern char g_splitLeaders[108][8];
extern int g_carSplitTimes[8][12][5];
extern int g_driverSplitClocks[82];
extern BYTE g_finishedDriverCount;
extern BYTE g_unk0x0053e18d[2];
extern BYTE g_unk0x0053e18f;
extern CarStageTiming g_carStageTiming[8];
extern BYTE g_unk0x0053d1da[8];
extern BYTE g_unk0x0053d1d8;
extern BYTE g_unk0x0053d1d9;
extern int g_unk0x0053d1b4;
extern char g_pendingDrivers[2];
extern BYTE g_unk0x0053d1a6;
extern BYTE g_unk0x0053d1a7;
extern char g_unk0x0053dda8[8];
extern unsigned char RallyData_GetSelectionFlag28(void);
extern unsigned int RallyData_GetSetupFlag11(void);
extern int RallyTiming_GetChampionshipDriverPosition(int index);
extern int Stage_GetDriverCount(void);

// Clears the stage timing state of every driver of the race that is starting:
// the split-leader tables, the ordering of the driver slots and each car's
// split/checkpoint times.
// match 69%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// Implementada; el original guarda el contador en el slot salvado de EDI (3 pushes), nosotros usamos 4 y otro reparto de registros.
// FUNCTION: CMR2 0x00447f70
void StageTiming_ResetRaceDriverTimes(void)
{
    int count;
    int i;
    int j;

    count = Stage_GetDriverCount();
    g_unk0x0053d1b4 = 0;
    if ((BYTE)RallyData_GetFlag24() != 0 || (BYTE)RallyData_GetFlag25() != 0) {
        g_finishedDriverCount = 0;
        for (i = 0; i < 0x1b; i++)
            ((int *)g_unk0x0053ddb0)[i] = 0;
        for (i = 0; i < 0x12; i++)
            ((int *)g_driverSplitClocks)[i] = 0;
        for (i = 0; i < 2; i++) {
            g_unk0x0053e18d[i] = 0;
            g_unk0x0053e18f = 0;
        }
        if (count > 0) {
            for (i = 0; i < count; i++) {
                BYTE *p = (BYTE *)g_splitLeaders + i;

                for (j = 0; j < 0xc; j++) {
                    int k;

                    for (k = 0; k < 9; k++) {
                        *p = 0xff;
                        p += 8;
                    }
                }
            }
        }
        if (count > 0) {
            for (i = 0; i < count; i++) {
                CarStageTiming *car = &g_carStageTiming[i];

                car->field_0x81 = (char)(count - i - 1);
                if ((char)CGameInfo::GetConfiguredGameMode() == 5 && count > 2)
                    car->field_0x81 = (char)(count - (char)RallyTiming_GetChampionshipDriverPosition(i) - 1);
                car->field_0x82 = -1;
                car->startTime = 0;
                g_unk0x0053dda8[car->field_0x81] = (char)i;
                car->lastTime = 0;
                if (RallyData_GetFlag24() != 0) {
                    for (j = 0; j < 0x3c; j++)
                        ((int *)g_carSplitTimes[i])[j] = 0;
                }
                for (j = 0; j < 9; j++)
                    car->field_0x4[j] = 0;
                car->field_0x80 = -1;
                car->field_0x83 = 0;
            }
        }
        if ((BYTE)RallyData_GetSetupFlag11() != 0) {
            *(short *)g_pendingDrivers = 0;
            g_unk0x0053d1a7 = 0;
            g_unk0x0053d1a6 = 0;
        }
    }
    if (count > 0) {
        for (i = 0; i < count; i++)
            g_unk0x0053d1da[i] = 0;
    }
    if (RallyData_GetFlag24() != 0 && RallyData_GetSelectionFlag28() != 0)
        g_unk0x0053d1d9 = 1;
    else
        g_unk0x0053d1d9 = 0;
    g_unk0x0053d1d8 = 0;
}
