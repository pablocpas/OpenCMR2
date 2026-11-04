#include "SoundCurve.h"
#include <windows.h>
#include "SurfaceTypes.h"
#include "FixedPoint.h"
#include "StageTiming.h"
#include "GenericFileLoader.h"
#include "Car.h"
#include "Game.h"
#include "GameInfo.h"
#include "Input.h"
#include <stdio.h>
#include <stddef.h>
#include "InstallInfo.h"
#include "RallyData.h"
#include "Frontend.h"

int g_surfaceDrag[63] = {
    0, 0, -65536, -32768, -65536, -65536, -65536, -131072,
    -98304, 0, -49152, 0, -32768, -32768, -32768, -32768,
    -98304, -65536, 0, -16384, -32768, 0, -16384, -16384,
    -32768, -98304, -32768, 0, -32768, -16384, -16384, 0,
    -32768, -16384, -32768, 0, 0, -16384, -32768, -16384,
    -32768, 0, -16384, -65536, -32768, 0, -32768, -16384,
    -32768, -16384, -16384, 0, -32768, 0, 0, -65536,
    -49152, -65536, -16384, -32768, -32768, 0, -16384,
};
// GLOBAL: CMR2 0x0051df58
BYTE g_surfaceIndexMap[96] = {
    0x19,0x18,0x06,0x0b,0x0a,0x09,0x08,0x07,0x0b,0x0a,0x09,0x08,0x07,0x0b,0x0b,0x0a,
    0x0b,0x01,0x01,0x0d,0x0d,0x05,0x05,0x05,0x05,0x02,0x0c,0x0c,0x0c,0x0d,0x02,0x0c,
    0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x0c,0x0c,0x0c,0x10,0x03,0x03,
    0x03,0x03,0x03,0x04,0x04,0x04,0x04,0x05,0x05,0x0d,0x10,0x0f,0x0f,0x12,0x16,0x17,
    0x1e,0x11,0x14,0x15,0x13,0x1d,0x0e,0x0c,0x01,0x05,0x05,0x0d,0x0c,0x05,0x05,0x05,
    0x0c,0x09,0x13,0x1a,0x0e,0x0e,0x0f,0x0f,0x0f,0x0f,0x0e,0x1b,0x1b,0x1b,0x1b,0x1c
};

// FUNCTION: CMR2 0x00478a10
short Surface_GetMappedIndex(short index)
{
    // PORT: a point off the stage's mesh gets no surface, and callers pass
    // what their variable held (Track_GetGroundHeight leaves it unset, one
    // passes the default height). The original read whatever was that far
    // from the table, up to 32 KB away; such a point gets the first class.
    if ((unsigned short)index >= sizeof(g_surfaceIndexMap))
        return g_surfaceIndexMap[0];
    return g_surfaceIndexMap[index];
}
// Path of the surface texture file; empty when the data came from memory.
// GLOBAL: CMR2 0x0058db50
char g_surfaceTexturePath[MAX_PATH];
// GLOBAL: CMR2 0x0058dc54
void *g_unk0x0058dc54;

#include "FileBuffer.h"
#include "Frontend.h"

// Releases the surface texture data (registered callback of 0x478a20).
// FUNCTION: CMR2 0x00478b20
BYTE Surface_FreeTextureData(void)
{
    if (g_unk0x0058dc54 != NULL) {
        if (g_surfaceTexturePath[0] == 0)
            CFileBuffer::FreeGenericFileBuffer(g_unk0x0058dc54);
        g_unk0x0058dc54 = NULL;
    }
    CFrontend::FreeLocalizedTextStrings();
    return 1;
}

int g_surfaceGrip[48][2] = {
    15073, 98304, 14745, 91750, 13107, 78643, 9830, 78643,
    13107, 78643, 13107, 78643, 15400, 91750, 14090, 91750,
    14417, 91750, 14745, 91750, 14417, 91750, 15400, 91750,
    15728, 98304, 15728, 98304, 15728, 137625, 15728, 137625,
    15728, 137625, 12451, 85196, 11796, 88473, 13107, 81920,
    13107, 176947, 13107, 176947, 13107, 81920, 13107, 73400,
    14417, 203161, 15728, 207093, 15728, 98304, 15400, 91750,
    14745, 91750, 14417, 203161, 13107, 79953, 13434, 91750,
    14090, 91750, 14090, 91750, 12451, 98304, 13107, 98304,
    13107, 98304, 13107, 98304, 7864, 78643, 9830, 78643,
    9830, 78643, 11796, 137625, 11141, 176947, 13762, 176947,
    14090, 91750, 13434, 91750, 11141, 176947, 15728, 207093,
};
int g_surfaceGrip2[48][2] = {
    12451, 98304, 12124, 91750, 10485, 78643, 7208, 78643,
    10485, 78643, 10485, 78643, 12779, 91750, 11468, 91750,
    11796, 91750, 12451, 91750, 12779, 91750, 13107, 91750,
    13107, 98304, 13107, 98304, 13107, 137625, 13107, 137625,
    13107, 137625, 9830, 88473, 9175, 88473, 10485, 81920,
    10485, 176947, 10485, 176947, 10485, 88473, 10485, 85852,
    13762, 196608, 14417, 204472, 13107, 98304, 12779, 91750,
    12124, 91750, 13762, 196608, 10485, 92405, 10813, 91750,
    11796, 91750, 11468, 91750, 9830, 98304, 10485, 98304,
    10485, 98304, 10485, 98304, 5242, 78643, 7864, 78643,
    7864, 78643, 9830, 137625, 10485, 173670, 13107, 173670,
    11468, 91750, 10813, 91750, 10485, 173670, 14417, 204472,
};
int g_surface0x51e2b8[48] = {
    0, 3932, 0, 0, 0, 0, 2621, 6553,
    5242, 7864, 6553, 3932, 0, 0, 0, 0,
    0, 1966, 0, 0, 0, 0, 3932, 13762,
    0, 0, 0, 3932, 3932, 0, 13762, 6553,
    6553, 5242, 3932, 3932, 3932, 3932, 0, 0,
    0, 0, 0, 0, 3932, 6553,
};
int g_surfaceSoftness[48][2] = {
    0, 0, 2621, 6553, 0, 0, 0, 0,
    0, 0, 0, 0, 2621, 6553, 3932, 6553,
    3276, 6553, 3932, 6553, 3276, 6553, 2621, 6553,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 13107, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 26869, 0, 36700,
    0, 0, 0, 0, 0, 0, 2621, 6553,
    2621, 6553, 0, 0, 0, 36700, 2621, 6553,
    2621, 6553, 2621, 6553, 1310, 3276, 1310, 3276,
    1310, 3276, 1310, 3276, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    2621, 6553, 2621, 6553,
};
int g_surface0x51e4f8[48] = {
    0, 9175, 5242, 5242, 2621, 2621, 3276, 4587,
    2621, 4587, 2621, 2621, 5242, 0, 0, 0,
    7864, 0, 0, 5242, 2621, 2621, 0, 0,
    0, 0, 5242, 3932, 9175, 0, 0, 9175,
    2621, 3276, 0, 0, 3276, 3276, 3276, 3276,
    3276, 7864, 0, 0, 3932, 9175,
};
int g_surface0x51e5b8[48] = {
    0, 5242, 2621, 1310, 327, 327, 458, 65,
    196, 196, 327, 655, 2621, 0, 0, 0,
    1310, 0, 0, 524, 524, 524, 0, 0,
    0, 0, 2621, 655, 5242, 0, 0, 5242,
    655, 458, 0, 0, 2621, 2621, 2621, 2621,
    2621, 5242, 0, 0, 655, 5242,
};
int g_surface0x51e678[48] = {
    13107, 0, 9830, 13107, 9830, 4915, 0, 3276,
    1638, 0, 0, 0, 11141, 0, 52428, 327680,
    0, 0, 0, 26214, 13107, 13107, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 16384, 0, 6553, 4915, 16384, 9830,
    4915,
};
int g_surface0x51e738[48] = {
    13107, 13107, 0, 0, 0, 0, 13107, 19660,
    16384, 19660, 16384, 13107, 32768, 13107, 32768, 32768,
    16384, 6553, 3276, 13107, 13107, 13107, 15073, 12451,
    13107, 16384, 13107, 13107, 13107, 13107, 12451, 13107,
    13107, 13107, 19660, 19660, 32768, 19660, 0, 0,
    0, 16384, 13107, 16384, 13107, 13107, 13107, 16384,
};
BYTE g_surfaceEffect[48][2] = {
    0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64,
    0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64,
    0x32, 0x50, 0x50, 0x8c, 0x50, 0x8c, 0x50, 0x8c, 0x50, 0x8c, 0x50, 0x8c, 0x50, 0x8c, 0x50, 0x8c,
    0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64, 0x50, 0x8c, 0x4b, 0x64,
    0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64,
    0x4b, 0x64, 0x32, 0x50, 0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64, 0x4b, 0x64,
};
BYTE g_surfaceNoise[48] = {
    0x50, 0x50, 0x50, 0x64, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x64, 0x64,
    0x64, 0x4b, 0x46, 0x50, 0x64, 0x82, 0x50, 0x50, 0x6e, 0x82, 0x50, 0x50, 0x50, 0x6e, 0x50, 0x50,
    0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x64, 0x50, 0x50, 0x64, 0x6e, 0x64, 0x50, 0x50, 0x6e, 0x82,
};
// Blend rate of each surface towards its "next" surface (see 0x4789d0).
unsigned short g_surfaceBlendRate[48] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x8000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x3333, 0x3333, 0x3333, 0x3333, 0x3333,
    0x3333, 0x3333, 0x4ccc, 0x4ccc, 0x4ccc, 0xb333, 0xb333, 0xb333, 0x3333, 0x3333, 0xb333,
};
BYTE g_surfaceNext[48] = {
    0x22, 0x1f, 0x02, 0x26, 0x27, 0x28, 0x21, 0x07, 0x08, 0x09, 0x0a, 0x20, 0x24, 0x23, 0x0e, 0x0f,
    0x29, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x2a, 0x2b, 0x25, 0x2c, 0x2d, 0x2e, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
};
BYTE g_surfaceDragIndex[48] = {
    0x03, 0x05, 0x08, 0x08, 0x08, 0x08, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x03, 0x03, 0x00, 0x00,
    0x01, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x01, 0x01, 0x03, 0x05, 0x05, 0x01, 0x07, 0x06,
    0x06, 0x06, 0x04, 0x04, 0x04, 0x04, 0x08, 0x08, 0x08, 0x02, 0x02, 0x02, 0x06, 0x06, 0x02, 0x02,
};

#define FIX_ABS(x) ((x) < 0 ? -(x) : (x))

// Typed views of the original interior cursors. offsetof follows the field,
// while the cursor arrays in Car keep the complete walks in one allocation.
inline CarCornerGrip *CarSurfaceGripOutput(int *p) {
    return (CarCornerGrip *)((BYTE *)p - offsetof(CarCornerGrip, gripB));
}
inline CarWheelSurface *CarSurfaceWheelOutput(int *p) {
    return (CarWheelSurface *)((BYTE *)p - offsetof(CarWheelSurface, drag));
}
inline CarCornerGrip *CarSurfacePairedGripOutput(int *p) {
    return (CarCornerGrip *)((BYTE *)p - offsetof(CarCornerGrip, grip2B));
}
inline CarWheelSurface *CarSurfacePairedWheelOutput(BYTE *p) {
    return (CarWheelSurface *)(p - offsetof(CarWheelSurface, effect) - 1);
}

// Rebuilds the per-corner grip parameters of a car by blending the surface it
// stands on with the next one, and smooths the resulting rolling noise level.
// match 48%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004781d0
void Car_UpdateSurfaceParams(Car *pCar, int blend)
{
    int i = 7;
    short noise = 0;
    int *pComp = &pCar->surfaceCompressionWords[(offsetof(Car, cornerWeight) + sizeof(int) - offsetof(Car, surfaceCompressionWords)) / sizeof(int)];
    short noiseNext = 0;
    short *pSurf = &pCar->wheelSurface[7];
    int *pOut = &pCar->surfaceOutputWords[(offsetof(Car, cornerGrip) + 7 * sizeof(CarCornerGrip) + offsetof(CarCornerGrip, gripB)) / sizeof(int)];
    short s0;
    unsigned short next;
    int s1;
    int gripA;
    int gripB;
    int grip2A;
    int grip2B;
    int v2b8;
    int v4f8;
    int v5b8;
    int v738;
    int dSoftA;
    int dSoftB;
    int v678;
    int diff;

    for (;;) {
        s0 = *pSurf;
        next = g_surfaceNext[s0];
        s1 = (short)next;
        diff = g_surfaceGrip[s1][0] - g_surfaceGrip[s0][0];
        gripA = FixMul(diff, blend) + g_surfaceGrip[s0][0];
        diff = g_surfaceGrip[s1][1] - g_surfaceGrip[s0][1];
        gripB = FixMul(diff, blend) + g_surfaceGrip[s0][1];
        diff = g_surfaceGrip2[s1][0] - g_surfaceGrip2[s0][0];
        grip2A = FixMul(diff, blend) + g_surfaceGrip2[s0][0];
        diff = g_surfaceGrip2[s1][1] - g_surfaceGrip2[s0][1];
        grip2B = FixMul(diff, blend) + g_surfaceGrip2[s0][1];
        diff = g_surface0x51e2b8[s1] - g_surface0x51e2b8[s0];
        v2b8 = FixMul(diff, blend) + g_surface0x51e2b8[s0];
        diff = g_surface0x51e4f8[s1] - g_surface0x51e4f8[s0];
        v4f8 = FixMul(diff, blend) + g_surface0x51e4f8[s0];
        diff = g_surface0x51e5b8[s1] - g_surface0x51e5b8[s0];
        v5b8 = FixMul(diff, blend) + g_surface0x51e5b8[s0];
        diff = g_surface0x51e738[s1] - g_surface0x51e738[s0];
        v738 = FixMul(diff, blend) + g_surface0x51e738[s0];
        diff = g_surfaceSoftness[s1][0] - g_surfaceSoftness[s0][0];
        dSoftA = FixMul(diff, blend);
        dSoftA += g_surfaceSoftness[s0][0];
        diff = g_surfaceSoftness[s1][1] - g_surfaceSoftness[s0][1];
        dSoftB = FixMul(diff, blend) + g_surfaceSoftness[s0][1];
        diff = g_surface0x51e678[s1] - g_surface0x51e678[s0];
        v678 = FixMul(diff, blend) + g_surface0x51e678[s0];

        if (i >= 4) {
            CarSurfaceGripOutput(pOut)->gripB = gripB;
            CarSurfaceGripOutput(pOut)->gripA = gripA;
            CarSurfaceGripOutput(pOut)->grip2A = grip2A;
store_grip2B:
            CarSurfaceGripOutput(pOut)->grip2B = grip2B;
        } else {
            int comp = pComp[-4];

            if (comp != 0) {
                int soft;

                if (comp > 0x10000) {
                    comp = 0x10000;
                }
                soft = FixMul(comp, dSoftA);
                CarSurfaceGripOutput(pOut)->gripA = soft + gripA;
                CarSurfaceGripOutput(pOut)->grip2A = soft + grip2A;
                CarSurfaceGripOutput(pOut)->gripB = FixDiv(FixMul(gripA, gripB), CarSurfaceGripOutput(pOut)->gripA);
                CarSurfaceGripOutput(pOut)->grip2B = FixDiv(FixMul(grip2A, grip2B), CarSurfaceGripOutput(pOut)->grip2A);
            } else {
                CarSurfaceGripOutput(pOut)->grip2A = grip2A;
                CarSurfaceGripOutput(pOut)->gripA = gripA;
                CarSurfaceGripOutput(pOut)->gripB = gripB;
                CarSurfaceGripOutput(pOut)->grip2B = grip2B;
            }
            comp = *pComp;
            if (comp != 0) {
                int soft;

                if (comp > 0x10000) {
                    comp = 0x10000;
                }
                soft = FixMul(comp, dSoftB);
                grip2B = CarSurfaceGripOutput(pOut)->grip2B + soft;
                CarSurfaceGripOutput(pOut)->gripB = CarSurfaceGripOutput(pOut)->gripB + soft;
                goto store_grip2B;
            }
        }

        CarSurfaceGripOutput(pOut)->field_0x18 = v2b8;
        CarSurfaceGripOutput(pOut)->field_0x14 = v738;
        CarSurfaceGripOutput(pOut)->field_0x20 = v5b8;
        CarSurfaceGripOutput(pOut)->field_0x1c = v4f8;
        CarSurfaceGripOutput(pOut)->drag = v678;
        i--;
        pSurf--;
        pComp--;
        pOut -= sizeof(CarCornerGrip) / sizeof(*pOut);
        if (i < 0) {
            int level;
            int previous;
            int diff;

            i = 4;
            pOut = &pCar->surfaceOutputWords[(offsetof(Car, wheelSurfaceFx) + 3 * sizeof(CarWheelSurface) + offsetof(CarWheelSurface, drag)) / sizeof(int)];
            pSurf = &pCar->wheelSurface[3];
            do {
                int id = *pSurf;
                BYTE next = g_surfaceNext[id];
                int drag;
                int dragNext;

                CarSurfaceWheelOutput(pOut)->effect[0] = g_surfaceEffect[id][0];
                CarSurfaceWheelOutput(pOut)->effect[1] = g_surfaceEffect[id][1];
                drag = FixMul(g_surfaceDrag[g_surfaceDragIndex[id] + pCar->surfaceDragLevel * 9], 0x51e);
                dragNext = FixMul(g_surfaceDrag[g_surfaceDragIndex[(int)(short)(unsigned short)next] +
                                                pCar->surfaceDragLevel * 9], 0x51e);
                CarSurfaceWheelOutput(pOut)->extraGrip = 0;
                CarSurfaceWheelOutput(pOut)->drag = FixMul(dragNext - drag, blend) + drag;
                pOut -= sizeof(CarWheelSurface) / sizeof(*pOut);
                noise = noise + (unsigned short)g_surfaceNoise[id];
                noiseNext = noiseNext + (unsigned short)g_surfaceNoise[(int)(short)(unsigned short)next];
                pSurf--;
                i--;
            } while (i != 0);

            level = FixMul(((int)noiseNext >> 2) * 0x10000 + ((int)noise & 0xfffffffc) * -0x4000, blend);
            level = FixMul(((int)noise & 0xfffffffc) * 0x4000 + level, 0x28f);
            previous = pCar->surfaceNoise;
            diff = level - previous;
            pCar->surfaceNoiseTarget = level;
            if (FIX_ABS(diff) < 0x3334) {
                pCar->surfaceNoise = level;
                return;
            }
            if (diff > 0) {
                pCar->surfaceNoise = previous + 0x3333;
                return;
            }
            pCar->surfaceNoise = previous - 0x3333;
            return;
        }
    }
}

// GLOBAL: CMR2 0x0051e994
char g_fontNames[7][20] = {
    "general\\hel_13pt", "general\\hel_20pt", "general\\hel_36pt", "general\\lcd_14pt",
    "general\\lcd_640", "general\\ocr_12pt", "general\\ocr_60pt",
};

// The global that follows g_fontNames; the original uses it as the loop end.
extern char g_strMenuSoundNames[5][7];

int Font_InitTable(unsigned int count);
void Font_Load(char *name, GenericFile *pFile, unsigned int index);

// Loads the seven game fonts from stage file 1.
// FUNCTION: CMR2 0x00478b50
void Surface_LoadStageFonts(void)
{
    char *pName;
    int i;

    Font_InitTable(7);
    i = 0;
    // The original walks the array up to its end (the address of the global that follows it there).
    for (pName = g_fontNames[0]; (INT_PTR)pName < (INT_PTR)g_fontNames[7]; pName += 20) {
        Font_Load(pName, (GenericFile *)StageTiming_GetStageFile1(), i);
        i++;
    }
}

// Blend rate between a surface and its "next" surface, at t.
// FUNCTION: CMR2 0x004789d0
int Surface_GetTransitionBlend(int surface, int t)
{
    int base = g_surfaceBlendRate[surface];
    int next = g_surfaceNext[surface];
    int delta = g_surfaceBlendRate[next] - base;

    return FixMul(delta, t) + base;
}

// Per-player surface sound state (two entries each).
// GLOBAL: CMR2 0x0058dd70
int g_surfaceSoundState[2];
// GLOBAL: CMR2 0x0058ddc0
int g_unk0x0058ddc0[2];
// GLOBAL: CMR2 0x0058de00
int g_unk0x0058de00[2];
// GLOBAL: CMR2 0x0058de08
int g_unk0x0058de08[2];
// GLOBAL: CMR2 0x0058df20
int g_unk0x0058df20[2];

int Sound_IsPlaying(unsigned int handle);
void Sound_Free(unsigned int handle);
void Sound_FreeSamplesFromIndex(int first);
int Race_GetResourceState(void);


// Engine sound curves: sample count and input range, then the pitch samples
// and the volume bytes (the pointers are set by FUN_00478xxx at stage start).
// The range and interpolation consumers use the shared SoundCurve descriptor.

// GLOBAL: CMR2 0x0051ea60
SoundCurve g_engineSoundCurve = { 20, 2000, 3000, 0, 0 };
// GLOBAL: CMR2 0x0051eab8
SoundCurve g_engineLoadSoundCurve = { 126, 2200, 8500, 0, 0 };
// GLOBAL: CMR2 0x0051ec50
SoundCurve g_engineWhineSoundCurve = { 126, 2200, 8500, 0, 0 };
// GLOBAL: CMR2 0x0051ede8
SoundCurve g_speedSoundCurve = { 193, 0, 160, 0, 0 };

// GLOBAL: CMR2 0x0051ea74
unsigned short g_unk0x0051ea74[21] = {
    0x5622, 0x56e1, 0x57a3, 0x5866, 0x592b, 0x59f2, 0x5aba, 0x5b84, 0x5c50, 0x5d1e, 0x5ded, 0x5ebe,
    0x5f92, 0x6066, 0x613d, 0x6216, 0x62f0, 0x63cd, 0x64ab, 0x658b, 0x666e,
};
// GLOBAL: CMR2 0x0051ea9e
BYTE g_unk0x0051ea9e[0x1a] = {
    0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x4d, 0x48, 0x41,
    0x3b, 0x32, 0x29, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
// GLOBAL: CMR2 0x0051eacc
unsigned short g_unk0x0051eacc[127] = {
    0x2b11, 0x2b8a, 0x2c06, 0x2c82, 0x2d01, 0x2d80, 0x2e01, 0x2e83, 0x2f07, 0x2f8c, 0x3013, 0x309b,
    0x3125, 0x31b0, 0x323c, 0x32cb, 0x335b, 0x33ec, 0x347f, 0x3514, 0x35aa, 0x3642, 0x36dc, 0x3777,
    0x3814, 0x38b3, 0x3954, 0x39f6, 0x3a9a, 0x3b40, 0x3be8, 0x3c92, 0x3d3d, 0x3deb, 0x3e9a, 0x3f4b,
    0x3fff, 0x40b4, 0x416b, 0x4224, 0x42e0, 0x439d, 0x445d, 0x451e, 0x45e2, 0x46a8, 0x4770, 0x483a,
    0x4907, 0x49d6, 0x4aa7, 0x4b7a, 0x4c50, 0x4d28, 0x4e03, 0x4ee0, 0x4fbf, 0x50a1, 0x5185, 0x526c,
    0x5356, 0x5442, 0x5530, 0x5622, 0x5715, 0x580c, 0x5905, 0x5a02, 0x5b00, 0x5c02, 0x5d07, 0x5e0e,
    0x5f19, 0x6026, 0x6136, 0x624a, 0x6360, 0x6479, 0x6596, 0x66b6, 0x67d9, 0x68ff, 0x6a28, 0x6b55,
    0x6c85, 0x6db8, 0x6eef, 0x7029, 0x7167, 0x72a8, 0x73ed, 0x7535, 0x7681, 0x77d0, 0x7924, 0x7a7b,
    0x7bd6, 0x7d35, 0x7e97, 0x7ffe, 0x8168, 0x82d7, 0x8449, 0x85c0, 0x873b, 0x88ba, 0x8a3d, 0x8bc4,
    0x8d50, 0x8ee1, 0x9075, 0x920e, 0x93ac, 0x954e, 0x96f5, 0x98a1, 0x9a51, 0x9c06, 0x9dc0, 0x9f7f,
    0xa142, 0xa30b, 0xa4d9, 0xa6ac, 0xa884, 0xaa61, 0xac44,
};
// GLOBAL: CMR2 0x0051ebca
BYTE g_unk0x0051ebca[0x86] = {
    0x00, 0x1c, 0x29, 0x32, 0x3b, 0x41, 0x48, 0x4d, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53,
    0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53,
    0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53,
    0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53,
    0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53,
    0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53,
    0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53,
    0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
// GLOBAL: CMR2 0x0051ec64
unsigned short g_unk0x0051ec64[127] = {
    0x2b11, 0x2b8a, 0x2c06, 0x2c82, 0x2d01, 0x2d80, 0x2e01, 0x2e83, 0x2f07, 0x2f8c, 0x3013, 0x309b,
    0x3125, 0x31b0, 0x323c, 0x32cb, 0x335b, 0x33ec, 0x347f, 0x3514, 0x35aa, 0x3642, 0x36dc, 0x3777,
    0x3814, 0x38b3, 0x3954, 0x39f6, 0x3a9a, 0x3b40, 0x3be8, 0x3c92, 0x3d3d, 0x3deb, 0x3e9a, 0x3f4b,
    0x3fff, 0x40b4, 0x416b, 0x4224, 0x42e0, 0x439d, 0x445d, 0x451e, 0x45e2, 0x46a8, 0x4770, 0x483a,
    0x4907, 0x49d6, 0x4aa7, 0x4b7a, 0x4c50, 0x4d28, 0x4e03, 0x4ee0, 0x4fbf, 0x50a1, 0x5185, 0x526c,
    0x5356, 0x5442, 0x5530, 0x5622, 0x5715, 0x580c, 0x5905, 0x5a02, 0x5b00, 0x5c02, 0x5d07, 0x5e0e,
    0x5f19, 0x6026, 0x6136, 0x624a, 0x6360, 0x6479, 0x6596, 0x66b6, 0x67d9, 0x68ff, 0x6a28, 0x6b55,
    0x6c85, 0x6db8, 0x6eef, 0x7029, 0x7167, 0x72a8, 0x73ed, 0x7535, 0x7681, 0x77d0, 0x7924, 0x7a7b,
    0x7bd6, 0x7d35, 0x7e97, 0x7ffe, 0x8168, 0x82d7, 0x8449, 0x85c0, 0x873b, 0x88ba, 0x8a3d, 0x8bc4,
    0x8d50, 0x8ee1, 0x9075, 0x920e, 0x93ac, 0x954e, 0x96f5, 0x98a1, 0x9a51, 0x9c06, 0x9dc0, 0x9f7f,
    0xa142, 0xa30b, 0xa4d9, 0xa6ac, 0xa884, 0xaa61, 0xac44,
};
// GLOBAL: CMR2 0x0051ed62
BYTE g_unk0x0051ed62[0x86] = {
    0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64,
    0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64,
    0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64,
    0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64,
    0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64,
    0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64,
    0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64,
    0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
// GLOBAL: CMR2 0x0051edfc
unsigned short g_unk0x0051edfc[194] = {
    0x2b11, 0x2b60, 0x2bb0, 0x2c01, 0x2c52, 0x2ca4, 0x2cf6, 0x2d49, 0x2d9d, 0x2df1, 0x2e46, 0x2e9b,
    0x2ef1, 0x2f48, 0x2f9f, 0x2ff7, 0x304f, 0x30a8, 0x3102, 0x315d, 0x31b8, 0x3213, 0x3270, 0x32cd,
    0x332b, 0x3389, 0x33e8, 0x3448, 0x34a9, 0x350a, 0x356c, 0x35ce, 0x3632, 0x3696, 0x36fa, 0x3760,
    0x37c6, 0x382d, 0x3895, 0x38fd, 0x3966, 0x39d0, 0x3a3b, 0x3aa6, 0x3b12, 0x3b7f, 0x3bed, 0x3c5c,
    0x3ccb, 0x3d3b, 0x3dac, 0x3e1e, 0x3e91, 0x3f04, 0x3f79, 0x3fee, 0x4064, 0x40db, 0x4152, 0x41cb,
    0x4244, 0x42bf, 0x433a, 0x43b6, 0x4433, 0x44b1, 0x452f, 0x45af, 0x4630, 0x46b1, 0x4734, 0x47b7,
    0x483b, 0x48c1, 0x4947, 0x49ce, 0x4a56, 0x4ae0, 0x4b6a, 0x4bf5, 0x4c81, 0x4d0e, 0x4d9d, 0x4e2c,
    0x4ebc, 0x4f4d, 0x4fe0, 0x5073, 0x5108, 0x519d, 0x5234, 0x52cb, 0x5364, 0x53fe, 0x5499, 0x5535,
    0x55d2, 0x5671, 0x5710, 0x57b1, 0x5853, 0x58f6, 0x599a, 0x5a3f, 0x5ae6, 0x5b8e, 0x5c37, 0x5ce1,
    0x5d8c, 0x5e39, 0x5ee7, 0x5f96, 0x6046, 0x60f8, 0x61ab, 0x625f, 0x6315, 0x63cc, 0x6484, 0x653d,
    0x65f8, 0x66b4, 0x6772, 0x6831, 0x68f1, 0x69b3, 0x6a76, 0x6b3a, 0x6c00, 0x6cc7, 0x6d90, 0x6e5a,
    0x6f26, 0x6ff3, 0x70c2, 0x7192, 0x7263, 0x7336, 0x740b, 0x74e1, 0x75b9, 0x7692, 0x776d, 0x7849,
    0x7927, 0x7a07, 0x7ae8, 0x7bcb, 0x7caf, 0x7d96, 0x7e7d, 0x7f67, 0x8052, 0x813f, 0x822d, 0x831d,
    0x840f, 0x8503, 0x85f9, 0x86f0, 0x87e9, 0x88e4, 0x89e0, 0x8adf, 0x8bdf, 0x8ce1, 0x8de5, 0x8eeb,
    0x8ff3, 0x90fc, 0x9208, 0x9315, 0x9425, 0x9536, 0x964a, 0x975f, 0x9876, 0x9990, 0x9aab, 0x9bc8,
    0x9ce8, 0x9e0a, 0x9f2d, 0xa053, 0xa17b, 0xa2a5, 0xa3d1, 0xa4ff, 0xa630, 0xa762, 0xa897, 0xa9ce,
    0xab08, 0xac44,
};
// GLOBAL: CMR2 0x0051ef80
BYTE g_unk0x0051ef80[0xc4] = {
    0x4d, 0x4c, 0x4c, 0x4c, 0x4c, 0x4c, 0x4b, 0x4b, 0x4b, 0x4b, 0x4a, 0x4a, 0x4a, 0x4a, 0x4a, 0x4a,
    0x49, 0x49, 0x49, 0x49, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x47, 0x47, 0x47, 0x46, 0x46, 0x46,
    0x46, 0x46, 0x46, 0x45, 0x45, 0x45, 0x45, 0x44, 0x44, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43,
    0x43, 0x42, 0x42, 0x41, 0x41, 0x41, 0x41, 0x40, 0x40, 0x40, 0x40, 0x40, 0x3f, 0x3f, 0x3f, 0x3f,
    0x3e, 0x3e, 0x3e, 0x3e, 0x3d, 0x3d, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3c, 0x3b, 0x3b,
    0x3a, 0x3a, 0x3a, 0x39, 0x39, 0x39, 0x39, 0x38, 0x38, 0x37, 0x37, 0x37, 0x37, 0x36, 0x36, 0x36,
    0x36, 0x35, 0x35, 0x34, 0x34, 0x34, 0x34, 0x33, 0x33, 0x33, 0x33, 0x32, 0x32, 0x32, 0x32, 0x32,
    0x30, 0x30, 0x30, 0x30, 0x2f, 0x2f, 0x2e, 0x2e, 0x2e, 0x2e, 0x2d, 0x2d, 0x2d, 0x2d, 0x2c, 0x2c,
    0x2b, 0x2b, 0x2b, 0x2b, 0x2a, 0x2a, 0x2a, 0x2a, 0x29, 0x29, 0x28, 0x28, 0x28, 0x26, 0x26, 0x26,
    0x26, 0x25, 0x25, 0x24, 0x24, 0x24, 0x24, 0x22, 0x22, 0x22, 0x22, 0x21, 0x21, 0x1f, 0x1f, 0x1f,
    0x1f, 0x1e, 0x1e, 0x1e, 0x1e, 0x1c, 0x1a, 0x1a, 0x1a, 0x1a, 0x18, 0x18, 0x18, 0x18, 0x16, 0x16,
    0x14, 0x14, 0x14, 0x14, 0x11, 0x11, 0x11, 0x11, 0x0e, 0x0e, 0x0a, 0x0a, 0x0a, 0x0a, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

// Sound archive folder of each car (SOUNDS\\Car\\<name>.bfl and <name>\\*.wav),
// indexed by the car's team number.
// GLOBAL: CMR2 0x0051f044
char g_carSoundDir[14][0x14] = {
    "FOCUS", "LANCER", "COROLLA", "SUBARU", "206", "SEAT", "DELTA", "COSWORTH", "MINI", "METRO", "STRATOS", "205", "FOCUS", "FOCUS"
};
// Short prefix of each car's engine samples (fs-<short>.wav and so on).
// GLOBAL: CMR2 0x0051f15c
char g_carSoundShort[14][0x14] = {
    "focus", "lancer", "toy", "sub", "206", "seat", "delta", "cos", "mini", "met", "stratos", "205", "focus", "focus"
};

// Keeps .cmrzero file-backed (not a symbol from the original).
// PORT: section placement only mattered for the matching build.
int g_cmr2ZeroDataSeed = 1;
// GLOBAL: CMR2 0x0058dd68
int g_unk0x0058dd68[2];
// GLOBAL: CMR2 0x0058dd78
int g_unk0x0058dd78[2];
// GLOBAL: CMR2 0x0058dd80
int g_unk0x0058dd80[2];
// GLOBAL: CMR2 0x0058dd88
int g_unk0x0058dd88[2];
// GLOBAL: CMR2 0x0058dd90
int g_unk0x0058dd90[2];
// GLOBAL: CMR2 0x0058dd98
int g_unk0x0058dd98[2];
// GLOBAL: CMR2 0x0058ddac
int g_unk0x0058ddac[2];
// GLOBAL: CMR2 0x0058ddd0
int g_unk0x0058ddd0[2];
// GLOBAL: CMR2 0x0058ddd8
int g_unk0x0058ddd8[2];
// GLOBAL: CMR2 0x0058dde0
int g_unk0x0058dde0[2];
// GLOBAL: CMR2 0x0058dde8
int g_unk0x0058dde8[2];
// GLOBAL: CMR2 0x0058ddf0
int g_unk0x0058ddf0[2];
// GLOBAL: CMR2 0x0058ddf8
int g_unk0x0058ddf8[2];
// GLOBAL: CMR2 0x0058de10
int g_unk0x0058de10[2];
// GLOBAL: CMR2 0x0058de18
int g_unk0x0058de18[2];
// GLOBAL: CMR2 0x0058df30
int g_unk0x0058df30[2];
// Two sound handles; the release loop walks them as one block.
// GLOBAL: CMR2 0x0058ddc8
int g_unk0x0058ddc8Pair[2];

void Sound_SetMasterVolume(int volume);
unsigned char RallyDataState(void);
unsigned char RallyData_GetFlag25(void);
int StageObject_GetCarSoundElapsedTime(int index);
int Surface_ReleaseStageSounds(void);

// Resets the per-player surface sound state and installs the sound parameter
// tables; registered as the stage cleanup callback.
// FUNCTION: CMR2 0x00478dc0
void SurfaceSound_ResetPlayerParameters(void)
{
    int flag;
    int i;

    Sound_SetMasterVolume(0x10000);
    if ((BYTE)RallyDataState() == 1 && (BYTE)RallyData_GetFlag25() != 0 &&
        CGameInfo::GetGameModeOptionBit19() == 0 && CGameInfo::GetConfiguredGameMode() != 3)
        flag = 1;
    else
        flag = 0;
    for (i = 0; i < (int)((RallyDataState() & 0xff) + flag); i = i + 1) {
        g_unk0x0058dd78[i] = 0;
        g_unk0x0058de08[i] = 0;
        g_unk0x0058de00[i] = 0;
        g_unk0x0058de18[i] = -1;
        g_unk0x0058dd68[i] = -1;
        g_unk0x0058dd88[i] = StageObject_GetCarSoundElapsedTime(i);
        g_unk0x0058de10[i] = -1;
        g_unk0x0058ddf0[i] = -1;
        g_unk0x0058dde0[i] = -1;
        g_unk0x0058dde8[i] = -1;
        g_unk0x0058ddac[i] = -1;
        g_unk0x0058dd80[i] = -1;
        g_unk0x0058ddc0[i] = -1;
        g_unk0x0058dd90[i] = -1;
        g_unk0x0058ddd0[i] = -1;
        g_unk0x0058ddd8[i] = -1;
        g_unk0x0058df30[i] = -1;
        g_unk0x0058ddf8[i] = 0;
        g_unk0x0058dd98[i] = 0;
        g_surfaceSoundState[i] = 0;
        g_unk0x0058df20[i] = 0;
    }
    g_unk0x0058ddc8Pair[0] = -1;
    g_unk0x0058ddc8Pair[1] = -1;
    g_engineSoundCurve.pSamples = g_unk0x0051ea74;
    g_engineSoundCurve.pBytes = g_unk0x0051ea9e;
    g_engineLoadSoundCurve.pSamples = g_unk0x0051eacc;
    g_engineLoadSoundCurve.pBytes = g_unk0x0051ebca;
    g_engineWhineSoundCurve.pSamples = g_unk0x0051ec64;
    g_engineWhineSoundCurve.pBytes = g_unk0x0051ed62;
    g_speedSoundCurve.pSamples = g_unk0x0051edfc;
    g_speedSoundCurve.pBytes = g_unk0x0051ef80;
    CGame::RegisterCallback((void *)Surface_ReleaseStageSounds, 0);
}

// Releases the stage's surface sounds (registered callback of 0x478dc0).
// FUNCTION: CMR2 0x00478f30
int Surface_ReleaseStageSounds(void)
{
    Sound_FreeSamplesFromIndex(Race_GetResourceState());
    return 1;
}

// GLOBAL: CMR2 0x0051f36c
char g_strRadioOutsideWav[] = "radio_outside.wav";
// GLOBAL: CMR2 0x0051f380
char g_strRadioInsideWav[] = "radio_inside.wav";
// GLOBAL: CMR2 0x0051f394
char g_strCarDetIdleWav[] = "%s\\car\\%s\\detidle.wav";
// GLOBAL: CMR2 0x0051f3ac
char g_strCarDet4Wav[] = "%s\\car\\%s\\det4.wav";
// GLOBAL: CMR2 0x0051f3c0
char g_strCarDet3Wav[] = "%s\\car\\%s\\det3.wav";
// GLOBAL: CMR2 0x0051f3d4
char g_strCarDet2Wav[] = "%s\\car\\%s\\det2.wav";
// GLOBAL: CMR2 0x0051f3e8
char g_strCarDet1Wav[] = "%s\\car\\%s\\det1.wav";
// GLOBAL: CMR2 0x0051f3fc
char g_strCarChatterWav[] = "%s\\car\\%s\\chatter.wav";
// GLOBAL: CMR2 0x0051f414
char g_strCarRearTurboWav[] = "%s\\car\\%s\\rt-%s.wav";
// GLOBAL: CMR2 0x0051f428
char g_strCarFrontTurboWav[] = "%s\\car\\%s\\ft-%s.wav";
// GLOBAL: CMR2 0x0051f43c
char g_strCarWhineWav[] = "%s\\car\\%s\\whine.wav";
// GLOBAL: CMR2 0x0051f450
char g_strCarRearMidWav[] = "%s\\car\\%s\\rm-%s.wav";
// GLOBAL: CMR2 0x0051f464
char g_strCarFrontMidWav[] = "%s\\car\\%s\\fm-%s.wav";
// GLOBAL: CMR2 0x0051f478
char g_strCarRearSlowWav[] = "%s\\car\\%s\\rs-%s.wav";
// GLOBAL: CMR2 0x0051f48c
char g_strCarFrontSlowWav[] = "%s\\car\\%s\\fs-%s.wav";
// GLOBAL: CMR2 0x0051f4a0
char g_strCarSoundBfl[] = "%s\\car\\%s.bfl";
// Sound archives of the loaded car sound sets.
// GLOBAL: CMR2 0x0058df48
GenericFile g_carEngineSoundArchives[2];

extern int g_unk0x0058ddb4[2];
BYTE RallyData_GetDriverSelectGridSlot(int param1);
BYTE StageUI_GetRaceEndEventCount(void);
int StageTiming_GetSplitSecondaryEntry(int iSplit);
int Sound_GetSampleCount(void);
BOOL Sound_LoadSample(char *name, BYTE flags, GenericFile *pFile);

// Loads one sample from an archive.
// FUNCTION: CMR2 0x004792a0
void Surface_LoadArchiveSample(char *name, GenericFile *pFile)
{
    Sound_LoadSample(name, 0, pFile);
}

// Loads the engine sound sets: one per player (plus the opponent's in a
// single-player head-to-head), each from the car's sound archive.
// FUNCTION: CMR2 0x00478f50
void Surface_LoadCarEngineSounds(void)
{
    char archive[260];
    char name[260];
    char *pDir;
    int opponent;
    int i;
    int team;
    char *pCarName;
    char *pShortName;
    GenericFile *pFile;

    pDir = CInstallInfo::GetSoundsDir();
    if ((BYTE)RallyDataState() == 1 && (char)RallyData_GetFlag25() && CGameInfo::GetGameModeOptionBit19() == 0 &&
        CGameInfo::GetConfiguredGameMode() != 3)
        opponent = 1;
    else
        opponent = 0;
    for (i = 0; i < (int)(BYTE)RallyDataState() + opponent; i++) {
        if (opponent != 0 && i > 0)
            team = CFrontend::GetArchivePrimaryIDEntry(RallyData_GetDriverSelectGridSlot(StageTiming_GetSplitSecondaryEntry(StageUI_GetRaceEndEventCount())));
        else
            team = CFrontend::GetArchivePrimaryIDEntry(RallyData_GetDriverRecordSelectionValue(StageUI_GetRaceEndEventCount() + i));
        g_unk0x0058ddb4[i] = Sound_GetSampleCount();
        pCarName = g_carSoundDir[team];
        sprintf(archive, g_strCarSoundBfl, CInstallInfo::GetSoundsDir(), pCarName);
        CGenericFileLoader::LoadIntoFileRecord(&g_carEngineSoundArchives[i], archive);
        pShortName = g_carSoundShort[team];
        sprintf(name, g_strCarFrontSlowWav, pDir, pCarName, pShortName);
        Surface_LoadArchiveSample(name, &g_carEngineSoundArchives[i]);
        sprintf(name, g_strCarRearSlowWav, pDir, pCarName, pShortName);
        Surface_LoadArchiveSample(name, &g_carEngineSoundArchives[i]);
        sprintf(name, g_strCarFrontMidWav, pDir, pCarName, pShortName);
        Surface_LoadArchiveSample(name, &g_carEngineSoundArchives[i]);
        sprintf(name, g_strCarRearMidWav, pDir, pCarName, pShortName);
        Surface_LoadArchiveSample(name, &g_carEngineSoundArchives[i]);
        sprintf(name, g_strCarWhineWav, pDir, pCarName);
        Surface_LoadArchiveSample(name, &g_carEngineSoundArchives[i]);
        sprintf(name, g_strCarFrontTurboWav, pDir, pCarName, pShortName);
        Surface_LoadArchiveSample(name, &g_carEngineSoundArchives[i]);
        sprintf(name, g_strCarRearTurboWav, pDir, pCarName, pShortName);
        Surface_LoadArchiveSample(name, &g_carEngineSoundArchives[i]);
        sprintf(name, g_strCarChatterWav, pDir, pCarName);
        Surface_LoadArchiveSample(name, &g_carEngineSoundArchives[i]);
        sprintf(name, g_strCarDet1Wav, pDir, pCarName);
        Surface_LoadArchiveSample(name, &g_carEngineSoundArchives[i]);
        sprintf(name, g_strCarDet2Wav, pDir, pCarName);
        Surface_LoadArchiveSample(name, &g_carEngineSoundArchives[i]);
        sprintf(name, g_strCarDet3Wav, pDir, pCarName);
        Surface_LoadArchiveSample(name, &g_carEngineSoundArchives[i]);
        sprintf(name, g_strCarDet4Wav, pDir, pCarName);
        Surface_LoadArchiveSample(name, &g_carEngineSoundArchives[i]);
        sprintf(name, g_strCarDetIdleWav, pDir, pCarName);
        Surface_LoadArchiveSample(name, &g_carEngineSoundArchives[i]);
        if (CGameInfo::IsRecordFlagSet(0x13)) {
            sprintf(name, "%s", g_strRadioInsideWav);
            Surface_LoadArchiveSample(name, (GenericFile *)StageTiming_GetStageFile0());
            sprintf(name, "%s", g_strRadioOutsideWav);
            Surface_LoadArchiveSample(name, (GenericFile *)StageTiming_GetStageFile0());
        }
    }
    for (pFile = g_carEngineSoundArchives; (INT_PTR)pFile < (INT_PTR)&g_carEngineSoundArchives[2]; pFile++) {
        if (pFile->buffer != NULL) {
            CFileBuffer::FreeGenericFileBuffer(pFile->buffer);
            pFile->buffer = NULL;
        }
        pFile->fileSize = 0;
        pFile->didFileLoad = 0;
    }
    SurfaceSound_ResetPlayerParameters();
}

// Stops the player's surface sound started by flag g_surfaceSoundState.
// FUNCTION: CMR2 0x004792c0
void Surface_StopFirstOneShotSound(int player)
{
    if (g_surfaceSoundState[player] != 0) {
        if (Sound_IsPlaying(g_unk0x0058ddc0[player])) {
            Sound_Free(g_unk0x0058ddc0[player]);
            g_unk0x0058ddc0[player] = -1;
        }
        g_surfaceSoundState[player] = 0;
    }
}


#include "main.h"

// GLOBAL: CMR2 0x0051f290
int g_unk0x0051f290 = 0xb333;
// Loop start of the gear-whine sample per car.
// GLOBAL: CMR2 0x0051f2a0
int g_gearWhineLoopStart[14] = {
    0x32a8, 0x42d6, 0, 0x5a08, 0x5a08, 0, 0, 0x880, 0x3af2, 0x21c0, 0, 0x2400, 0x1444, 0xf80,
};
// Per player: when the random one-shot surface sound last started or stopped.
// GLOBAL: CMR2 0x0058dd60
unsigned int g_oneShotSoundLastTime[2];
// GLOBAL: CMR2 0x0058dda0
unsigned int g_unk0x0058dda0[2];
// GLOBAL: CMR2 0x0058df28
unsigned int g_unk0x0058df28[2];
// GLOBAL: CMR2 0x0058df60
int g_unk0x0058df60[2];
// GLOBAL: CMR2 0x0058df68
int g_unk0x0058df68[2];
// GLOBAL: CMR2 0x0058df70
int g_unk0x0058df70[2];
// GLOBAL: CMR2 0x0058df78
int g_unk0x0058df78[2];
// GLOBAL: CMR2 0x0058df80
int g_unk0x0058df80[2];
// GLOBAL: CMR2 0x0058df88
int g_unk0x0058df88[2];

extern int g_unk0x0058dda8;
extern int g_unk0x0051f274;
extern int g_unk0x0051f278;
extern int g_unk0x0051f27c;
extern int g_unk0x0051f294;
extern int g_unk0x0051f298;
extern int g_unk0x0051f29c;
extern int g_unk0x0058df38;
extern int g_unk0x0058df3c;
extern int g_unk0x0058df40;
extern int g_unk0x0051f2d8[14];
BYTE NetRace_GetRaceSoundMode(void);
bool NetRace_IsValueWithinCurveRange(int value, SoundCurve *pRange);
unsigned int NetRace_InterpolateByteCurve(int value, SoundCurve *pCurve);
unsigned int NetRace_InterpolateWordCurve(int value, SoundCurve *pCurve);
int NetRace_GetListenerDistanceAttenuation(unsigned int view, int listener);
unsigned short NetRace_ScaleViewAngleByDistance(int param_1, int param_2, unsigned short param_3);
int Race_GetPlayerRecordField4(BYTE index);
int Sound_PlaySampleWithParameters(unsigned short id, int volume, int frequency, int loopStart, int loops, int is3D);
void Sound_SetPlayingSlotVolume(unsigned int handle, int volume);
void Sound_SetPan(unsigned int handle, int pan);
void StageTiming_ForceWheelTrailSurface(int car, int surface);
void Surface_SmoothPlayerSoundLevel(int target, int player);
void Surface_StopSecondOneShotSound(int player);
void Surface_UpdatePlayerSkidSound(int player, int *pState, int listener);
int StageObject_GetCarSoundElapsedTime(int index);
#define CAR_VOLUME(x, scale) FixMul(NetRace_GetListenerDistanceAttenuation(player, listener), FixMul(g_unk0x0058dda8, FixMul((x), (scale))))

// Per-frame engine sounds of one player from its state (pitch, alternate
// pitch, speed, surface level, clutch, ...): the engine pair, the load pair
// (quieter as the throttle lifts), the idle/backfire pops with their timers,
// the gear whine and the transmission loop, then the skid and extra loops.
// FUNCTION: CMR2 0x00479360
void Surface_UpdatePlayerEngineSounds(int *pState, int player, int listener)
{
    Car *pCar;
    int inEngine;
    int inLoad;
    int inWhine;
    int pitch;
    unsigned int now;
    int backfire;
    int hi;
    int lo;
    int volume;
    int pct;

    now = CMain::GetFrameDelta();
    pCar = Car_Get(player);
    pitch = pState[0];
    g_unk0x0058df88[player] = pState[3];
    if (pState[3] > 1)
        g_unk0x0058df68[player] = 1;
    if (pitch > 5000)
        g_unk0x0058df70[player] = 1;
    inEngine = NetRace_IsValueWithinCurveRange(pitch, &g_engineSoundCurve);
    inLoad = NetRace_IsValueWithinCurveRange(pitch, &g_engineLoadSoundCurve);
    inWhine = NetRace_IsValueWithinCurveRange(pitch, &g_engineWhineSoundCurve);
    if (!inEngine) {
        if (Sound_IsPlaying(g_unk0x0058dde8[player])) {
            Sound_SetPan(g_unk0x0058dde8[player], 0x2b11);
            Sound_SetPlayingSlotVolume(g_unk0x0058dde8[player], 0);
        }
        if (Sound_IsPlaying(g_unk0x0058ddac[player])) {
            Sound_SetPan(g_unk0x0058ddac[player], 0x2b11);
            Sound_SetPlayingSlotVolume(g_unk0x0058ddac[player], 0);
        }
        if (Sound_IsPlaying(g_unk0x0058dde8[player])) {
            Sound_Free(g_unk0x0058dde8[player]);
            g_unk0x0058dde8[player] = -1;
        }
        if (Sound_IsPlaying(g_unk0x0058ddac[player])) {
            Sound_Free(g_unk0x0058ddac[player]);
            g_unk0x0058ddac[player] = -1;
        }
    } else {
        if (Sound_IsPlaying(g_unk0x0058dde8[player]) == 0)
            g_unk0x0058dde8[player] = Sound_PlaySampleWithParameters((unsigned short)g_unk0x0058ddb4[player], 0, 0x5622, 0, 1, 0);
        if (Sound_IsPlaying(g_unk0x0058ddac[player]) == 0)
            g_unk0x0058ddac[player] = Sound_PlaySampleWithParameters((unsigned short)(g_unk0x0058ddb4[player] + 1), 0, 0x5622, 0, 1, 0);
        if (Sound_IsPlaying(g_unk0x0058dde8[player])) {
            Sound_SetPan(g_unk0x0058dde8[player], NetRace_InterpolateWordCurve(pitch, &g_engineSoundCurve));
            Sound_SetPlayingSlotVolume(g_unk0x0058dde8[player],
                         CAR_VOLUME((int)(NetRace_InterpolateByteCurve(pitch, &g_engineSoundCurve) << 16) / 100, g_unk0x0058df38));
        }
        if (Sound_IsPlaying(g_unk0x0058ddac[player])) {
            Sound_SetPan(g_unk0x0058ddac[player], NetRace_InterpolateWordCurve(pitch, &g_engineSoundCurve));
            Sound_SetPlayingSlotVolume(g_unk0x0058ddac[player],
                         CAR_VOLUME((int)(NetRace_InterpolateByteCurve(pitch, &g_engineSoundCurve) << 16) / 100, g_unk0x0051f274));
        }
    }
    if (!inLoad) {
        if (Sound_IsPlaying(g_unk0x0058ddf0[player])) {
            Sound_SetPan(g_unk0x0058ddf0[player], 0x2b11);
            Sound_SetPlayingSlotVolume(g_unk0x0058ddf0[player], 0);
        }
        if (Sound_IsPlaying(g_unk0x0058dde0[player])) {
            Sound_SetPan(g_unk0x0058dde0[player], 0x2b11);
            Sound_SetPlayingSlotVolume(g_unk0x0058dde0[player], 0);
        }
    } else {
        if (Sound_IsPlaying(g_unk0x0058ddf0[player])) {
            volume = FixMul(0x10000 - FixDiv(pCar->throttleTorque, pCar->maxThrottleTorque),
                            CAR_VOLUME((int)(NetRace_InterpolateByteCurve(pitch, &g_engineLoadSoundCurve) << 16) / 100,
                                       g_unk0x0058df3c));
            if (Race_GetPlayerRecordField4((BYTE)player))
                Sound_SetPan(g_unk0x0058ddf0[player],
                             NetRace_ScaleViewAngleByDistance(player, player,
                                          NetRace_InterpolateWordCurve(pitch, &g_engineLoadSoundCurve)));
            else
                Sound_SetPan(g_unk0x0058ddf0[player],
                             NetRace_InterpolateWordCurve(pitch, &g_engineLoadSoundCurve));
            Sound_SetPlayingSlotVolume(g_unk0x0058ddf0[player], volume);
        }
        if (Sound_IsPlaying(g_unk0x0058dde0[player])) {
            volume = FixMul(0x10000 - FixDiv(pCar->throttleTorque, pCar->maxThrottleTorque),
                            CAR_VOLUME((int)(NetRace_InterpolateByteCurve(pitch, &g_engineLoadSoundCurve) << 16) / 100,
                                       g_unk0x0051f278));
            if (Race_GetPlayerRecordField4((BYTE)player))
                Sound_SetPan(g_unk0x0058dde0[player],
                             NetRace_ScaleViewAngleByDistance(player, player,
                                          NetRace_InterpolateWordCurve(pitch, &g_engineLoadSoundCurve)));
            else
                Sound_SetPan(g_unk0x0058dde0[player],
                             NetRace_InterpolateWordCurve(pitch, &g_engineLoadSoundCurve));
            Sound_SetPlayingSlotVolume(g_unk0x0058dde0[player], volume);
        }
    }
    Surface_SmoothPlayerSoundLevel(pState[4], player);
    if (!NetRace_GetRaceSoundMode()) {
        if (now - g_oneShotSoundLastTime[player] > g_unk0x0058df28[player]) {
            if (g_unk0x0058ddf8[player] != 0) {
                Surface_StopSecondOneShotSound(player);
                g_unk0x0058ddf8[player] = 0;
            }
            if (g_unk0x0058dd98[player] != 0) {
                g_unk0x0058dd98[player] = 0;
                g_oneShotSoundLastTime[player] = now;
                g_unk0x0058df28[player] = rand() % 10 + 5;
            } else if (pState[1] < 4000) {
                int v;
                g_unk0x0058dd98[player] = 1;
                g_oneShotSoundLastTime[player] = now;
                if (pState[1] <= 3000)
                    v = 10;
                else if (pState[1] >= 5000)
                    v = 0;
                else
                    v = (3000 - pState[1]) * 10 / 2000 + 10;
                g_unk0x0058df28[player] = v;
            } else {
                g_oneShotSoundLastTime[player] = now;
                g_unk0x0058df28[player] = rand() % 10 + 5;
            }
        }
        backfire = 0;
        if (CFrontend::GetArchiveSecondaryFlagEntry(pCar->type) != 0 &&
            g_unk0x0058df88[player] >= 2 && g_unk0x0058df88[player] <= 6 &&
            g_unk0x0058dd68[player] != g_unk0x0058df88[player] &&
            g_unk0x0058dd68[player] < g_unk0x0058df88[player]) {
            switch (g_unk0x0058df88[player]) {
            case 2:
                if (rand() % 100 >= 10)
                    backfire = 1;
                break;
            case 3:
                if (rand() % 100 >= 25)
                    backfire = 1;
                break;
            case 4:
                if (rand() % 100 >= 50)
                    backfire = 1;
                break;
            case 5:
                if (rand() % 100 >= 80)
                    backfire = 1;
            case 6:
                if (rand() % 100 >= 90)
                    backfire = 1;
                break;
            }
        }
        if (CGameInfo::IsActiveCheatEnabled(5) && pCar->field_0xa84 > 0)
            backfire = 1;
        if (pState[3] == 0) {
            hi = 3000;
            lo = 0xaf0;
        } else {
            hi = 5000;
            lo = 3000;
        }
        if (g_unk0x0058df78[player] != 0) {
            if (pState[1] < lo || pState[4] != 0)
                g_unk0x0058df78[player] = 0;
        } else if (CFrontend::GetArchiveSecondaryFlagEntry(pCar->type) != 0) {
            if (pState[4] == 0 && pState[1] > hi) {
                g_unk0x0058df78[player] = 1;
                g_oneShotSoundLastTime[player] = now;
                g_unk0x0058dda0[player] = now;
                g_unk0x0058df28[player] = rand() % 10 + 10;
                g_unk0x0058dd98[player] = 0;
            }
        } else {
            g_unk0x0058df78[player] = 0;
        }
        if (g_unk0x0058df78[player] != 0 || backfire) {
            Surface_StopFirstOneShotSound(player);
            if (backfire) {
                if (Sound_IsPlaying(g_unk0x0058ddc0[player]) == 0) {
                    g_unk0x0058ddc0[player] =
                        Sound_PlaySampleWithParameters((unsigned short)(rand() % 4 + g_unk0x0058ddb4[player] + 8), 0, 0x5622, 0, 0, 0);
                    g_oneShotSoundLastTime[player] = now;
                    g_unk0x0058df20[player] = 1;
                    g_unk0x0058ddf8[player] = 1;
                    g_unk0x0058df28[player] = rand() % 10 + 5;
                    StageTiming_ForceWheelTrailSurface(player, g_unk0x0058df28[player] >> 2);
                }
                Sound_SetPlayingSlotVolume(g_unk0x0058ddc0[player], CAR_VOLUME(0x10000, g_unk0x0051f294));
            } else if (g_unk0x0058dd98[player] != 0) {
                if (Sound_IsPlaying(g_unk0x0058ddc0[player]))
                    Sound_SetPlayingSlotVolume(g_unk0x0058ddc0[player], CAR_VOLUME(0, g_unk0x0051f294));
                else
                    g_unk0x0058ddc0[player] = -1;
            } else {
                if (Sound_IsPlaying(g_unk0x0058ddc0[player]) == 0) {
                    g_unk0x0058ddc0[player] =
                        Sound_PlaySampleWithParameters((unsigned short)(rand() % 4 + g_unk0x0058ddb4[player] + 8), 0, 0x5622, 0, 0, 0);
                    g_unk0x0058df20[player] = 1;
                }
                pct = 100 - (now * 100 - g_oneShotSoundLastTime[player] * 100) / g_unk0x0058df28[player];
                Sound_SetPlayingSlotVolume(g_unk0x0058ddc0[player], CAR_VOLUME(0x10000, g_unk0x0051f294));
                if (pct == 100 && rand() % pct < 75)
                    StageTiming_ForceWheelTrailSurface(player, rand() % 2 + 2);
            }
        } else if (pState[4] == 0) {
            Surface_StopSecondOneShotSound(player);
            if (pState[1] < lo) {
                if (g_surfaceSoundState[player] == 0) {
                    g_unk0x0058ddc0[player] =
                        Sound_PlaySampleWithParameters((unsigned short)(g_unk0x0058ddb4[player] + 0xc), 0, 0x5622, 0, 1, 0);
                    g_surfaceSoundState[player] = 1;
                }
                Sound_SetPlayingSlotVolume(g_unk0x0058ddc0[player],
                             CAR_VOLUME(((0xaf0 - pState[1]) * 100 / 0xaf0 / 2 + 50 << 16) / 100, g_unk0x0051f294));
            }
        } else {
            Surface_StopFirstOneShotSound(player);
        }
    }
    if (!inWhine) {
        if (Sound_IsPlaying(g_unk0x0058dd90[player])) {
            Sound_Free(g_unk0x0058dd90[player]);
            g_unk0x0058dd90[player] = -1;
        }
        if (Sound_IsPlaying(g_unk0x0058ddd0[player])) {
            Sound_Free(g_unk0x0058ddd0[player]);
            g_unk0x0058ddd0[player] = -1;
        }
    } else if (g_unk0x0058de08[player] == 0) {
        if (Sound_IsPlaying(g_unk0x0058dd90[player])) {
            Sound_Free(g_unk0x0058dd90[player]);
            g_unk0x0058dd90[player] = -1;
        }
        if (Sound_IsPlaying(g_unk0x0058ddd0[player])) {
            Sound_Free(g_unk0x0058ddd0[player]);
            g_unk0x0058ddd0[player] = -1;
        }
        if (g_unk0x0058df68[player] != 0 && g_unk0x0058df70[player] != 0 && g_unk0x0058df60[player] == 0) {
            Sound_PlaySampleWithParameters((unsigned short)(g_unk0x0058ddb4[player] + 7), CAR_VOLUME(0x10000, g_unk0x0051f290),
                         0x5622, 0, 0, 0);
            g_unk0x0058df60[player] = 1;
            g_unk0x0058df68[player] = 0;
            g_unk0x0058df70[player] = 0;
        }
    } else {
        volume = (int)NetRace_InterpolateByteCurve(pitch, &g_engineWhineSoundCurve);
        volume = g_unk0x0058de08[player] * volume / 100;
        if (g_unk0x0058de10[player] != -1)
            StageObject_GetCarSoundElapsedTime(player);
        g_unk0x0058df60[player] = 0;
        if (Sound_IsPlaying(g_unk0x0058dd90[player]) == 0)
            g_unk0x0058dd90[player] = Sound_PlaySampleWithParameters(
                (unsigned short)(g_unk0x0058ddb4[player] + 5), 0, 0x5622,
                g_gearWhineLoopStart[CFrontend::GetArchivePrimaryIDEntry(RallyData_GetDriverRecordSelectionValue((BYTE)(StageUI_GetRaceEndEventCount() + player)))],
                1, 0);
        if (Race_GetPlayerRecordField4((BYTE)player))
            Sound_SetPan(g_unk0x0058dd90[player],
                         NetRace_ScaleViewAngleByDistance(player, player,
                                      NetRace_InterpolateWordCurve(pitch, &g_engineWhineSoundCurve)));
        else
            Sound_SetPan(g_unk0x0058dd90[player], NetRace_InterpolateWordCurve(pitch, &g_engineWhineSoundCurve));
        Sound_SetPlayingSlotVolume(g_unk0x0058dd90[player], CAR_VOLUME((volume << 16) / 100, g_unk0x0058df40));
        if (Sound_IsPlaying(g_unk0x0058ddd0[player]) == 0)
            g_unk0x0058ddd0[player] = Sound_PlaySampleWithParameters(
                (unsigned short)(g_unk0x0058ddb4[player] + 6), 0, 0x5622,
                g_unk0x0051f2d8[CFrontend::GetArchivePrimaryIDEntry(RallyData_GetDriverRecordSelectionValue((BYTE)(StageUI_GetRaceEndEventCount() + player)))],
                1, 0);
        if (Race_GetPlayerRecordField4((BYTE)player))
            Sound_SetPan(g_unk0x0058ddd0[player],
                         NetRace_ScaleViewAngleByDistance(player, player,
                                      NetRace_InterpolateWordCurve(pitch, &g_engineWhineSoundCurve)));
        else
            Sound_SetPan(g_unk0x0058ddd0[player], NetRace_InterpolateWordCurve(pitch, &g_engineWhineSoundCurve));
        Sound_SetPlayingSlotVolume(g_unk0x0058ddd0[player], CAR_VOLUME((volume << 16) / 100, g_unk0x0051f27c));
    }
    Surface_UpdatePlayerSkidSound(player, pState, listener);
    g_unk0x0058df80[player] = ((BYTE *)pCar)[0x1d2];
    g_unk0x0058dd68[player] = g_unk0x0058df88[player];
    if (CGameInfo::IsRecordFlagSet(0x13)) {
        if (Sound_IsPlaying(g_unk0x0058ddd8[player]))
            Sound_SetPlayingSlotVolume(g_unk0x0058ddd8[player], CAR_VOLUME(0x10000, g_unk0x0051f298));
        if (Sound_IsPlaying(g_unk0x0058df30[player]))
            Sound_SetPlayingSlotVolume(g_unk0x0058df30[player], CAR_VOLUME(0x10000, g_unk0x0051f29c));
    }
}
#undef CAR_VOLUME

// Stops the player's surface sound started by flag g_unk0x0058df20.
// FUNCTION: CMR2 0x00479310
void Surface_StopSecondOneShotSound(int player)
{
    if (g_unk0x0058df20[player] != 0) {
        if (Sound_IsPlaying(g_unk0x0058ddc0[player])) {
            Sound_Free(g_unk0x0058ddc0[player]);
            g_unk0x0058ddc0[player] = -1;
        }
        g_unk0x0058df20[player] = 0;
    }
}


BYTE NetRace_GetRaceSoundMode(void);
bool NetRace_IsValueWithinCurveRange(int value, SoundCurve *pRange);
unsigned int NetRace_InterpolateByteCurve(int value, SoundCurve *pCurve);
unsigned int NetRace_InterpolateWordCurve(int value, SoundCurve *pCurve);
int NetRace_GetListenerDistanceAttenuation(unsigned int view, int listener);
unsigned short NetRace_ScaleViewAngleByDistance(int param_1, int param_2, unsigned short param_3);
int Race_GetPlayerRecordField4(BYTE index);
void Sound_SetPlayingSlotVolume(unsigned int handle, int volume);
void Sound_SetPan(unsigned int handle, int pan);
extern int g_unk0x0058dda8;

// Master scale of the skid loop volume.
// GLOBAL: CMR2 0x0051f280
int g_skidLoopVolumeScale = 0xb333;
// Volume dip (percent) over the first 100 ticks after the skid level rises.
// GLOBAL: CMR2 0x0051f310
int g_skidVolumeDip[23] = {
    30, 20, 10, 20, 50, 90, 100, 50, 10, 30, 60, 30, 10, 40, 70, 20, 5, 20, 50, 30, 20, 10, 5,
};


// Volume scales of the car sound loops, swapped between the outside and the
// in-car views.
// GLOBAL: CMR2 0x0051f274
int g_unk0x0051f274 = 0xcccc;
// GLOBAL: CMR2 0x0051f278
int g_unk0x0051f278 = 0xc000;
// GLOBAL: CMR2 0x0051f294
int g_unk0x0051f294 = 0x30000;
// GLOBAL: CMR2 0x0051f298
int g_unk0x0051f298 = 0x30000;
// GLOBAL: CMR2 0x0051f29c
int g_unk0x0051f29c = 0x30000;
// GLOBAL: CMR2 0x0058df38
int g_unk0x0058df38;
// GLOBAL: CMR2 0x0058df3c
int g_unk0x0058df3c;
// GLOBAL: CMR2 0x0058df40
int g_unk0x0058df40;
extern int g_unk0x0051f27c;
int View_GetActiveCameraMode(BYTE index);
int Sound_PlaySampleWithParameters(unsigned short id, int volume, int frequency, int loopStart, int loops, int is3D);
void Surface_UpdatePlayerEngineSounds(int *pState, int player, int listener);

// Per-frame car sounds of one player: picks the volume set of the camera
// (in-car views 1-3 or outside), makes sure the engine, skid and optional
// loops are playing, then hands the engine pitch, speed, surface level and
// wheel-slip state to Surface_UpdatePlayerEngineSounds.
// FUNCTION: CMR2 0x0047a710
void SurfaceSound_UpdatePlayerLoops(int player, int listener)
{
    Car *pCar;
    int pitch;
    int speed;
    int state[7];

    if (View_GetActiveCameraMode((BYTE)listener) != 1 && View_GetActiveCameraMode((BYTE)listener) != 2 &&
        View_GetActiveCameraMode((BYTE)listener) != 3) {
        g_unk0x0058df38 = 0x6666;
        g_unk0x0051f274 = 0xcccc;
        g_unk0x0058df3c = 0x6000;
        g_unk0x0051f278 = 0xc000;
        g_unk0x0058df40 = 0x9999;
        g_unk0x0051f27c = 0x10000;
        g_unk0x0051f294 = 0x20000;
        g_unk0x0051f29c = 0x20000;
        g_unk0x0051f298 = 0;
    } else {
        g_unk0x0058df38 = 0xcccc;
        g_unk0x0051f274 = 0x6666;
        g_unk0x0058df3c = 0xc000;
        g_unk0x0051f278 = 0x6000;
        g_unk0x0058df40 = 0x10000;
        g_unk0x0051f27c = 0x9999;
        g_unk0x0051f294 = 0x10000;
        g_unk0x0051f29c = 0;
        g_unk0x0051f298 = 0x20000;
    }
    pCar = Car_Get(player);
    if (Sound_IsPlaying(g_unk0x0058ddf0[player]) == 0)
        g_unk0x0058ddf0[player] = Sound_PlaySampleWithParameters((unsigned short)(g_unk0x0058ddb4[player] + 2), 0, 0x5622, 0, 1, 0);
    if (Sound_IsPlaying(g_unk0x0058dde0[player]) == 0)
        g_unk0x0058dde0[player] = Sound_PlaySampleWithParameters((unsigned short)(g_unk0x0058ddb4[player] + 3), 0, 0x5622, 0, 1, 0);
    if (!NetRace_GetRaceSoundMode()) {
        if (Sound_IsPlaying(g_unk0x0058dd80[player]) == 0)
            g_unk0x0058dd80[player] = Sound_PlaySampleWithParameters((unsigned short)(g_unk0x0058ddb4[player] + 4), 0, 0x5622, 0, 1, 0);
    }
    if (CGameInfo::IsRecordFlagSet(0x13)) {
        if (Sound_IsPlaying(g_unk0x0058ddd8[player]) == 0)
            g_unk0x0058ddd8[player] = Sound_PlaySampleWithParameters((unsigned short)(g_unk0x0058ddb4[player] + 0xd), 0x10000, 0x2b11, 0, 1, 0);
        if (Sound_IsPlaying(g_unk0x0058df30[player]) == 0)
            g_unk0x0058df30[player] = Sound_PlaySampleWithParameters((unsigned short)(g_unk0x0058ddb4[player] + 0xe), 0x10000, 0x2b11, 0, 1, 0);
    }
    pitch = FixMul(FixMul(pCar->field_0x7ac, pCar->field_0x798), 0x19640000) >> 16;
    state[1] = pitch;
    if (state[1] < 2000)
        state[1] = 2000;
    else if (state[1] > 0x2134)
        state[1] = 0x2134;
    pitch += 2000;
    if (pitch < 2000)
        pitch = 2000;
    else if (pitch > 0x2134)
        pitch = 0x2134;
    speed = FixDiv(FixMul(pCar->speed, 0x431168), 0x9ef9) >> 16;
    if (speed < 0)
        speed = 0;
    else if (speed > 0x104)
        speed = 0x104;
    state[0] = pitch;
    state[2] = speed;
    state[3] = pCar->gear;
    state[4] = (FixDiv(FixMul(pCar->throttleTorque, 0x640000), pCar->maxThrottleTorque) < 0x50000 ? 0 : 0x640000) >> 16;
    state[5] = 0;
    state[6] = 0;
    Surface_UpdatePlayerEngineSounds(state, player, listener);
}

// Skid sound of one player: when the skid level (+0xc) rises it stops the
// one-shot surface sounds and restarts the level timer (sometimes 100 ticks
// late), then while the speed (+8) is in range fades the loop in (+0x10 ==
// 100), sets its pitch from the speed curve and its volume from the curve,
// the level's fade-in dip, distance and master volume; otherwise silences it.
// FUNCTION: CMR2 0x0047a3d0
void Surface_UpdatePlayerSkidSound(int player, int *pState, int listener)
{
    int speed;
    int level;
    int locked;
    int *pHandle;
    int volume;
    int dip;
    int inRange;

    speed = pState[2];
    level = pState[3];
    if (pState[5] != 0 || pState[6] != 0)
        locked = 1;
    else
        locked = 0;
    if (g_unk0x0058dd68[player] < level) {
        if (Sound_IsPlaying(g_unk0x0058ddd0[player])) {
            Sound_Free(g_unk0x0058ddd0[player]);
            g_unk0x0058ddd0[player] = -1;
        }
        if (Sound_IsPlaying(g_unk0x0058dd90[player])) {
            Sound_Free(g_unk0x0058dd90[player]);
            g_unk0x0058dd90[player] = -1;
        }
        g_unk0x0058de10[player] = StageObject_GetCarSoundElapsedTime(player);
    } else {
        StageObject_GetCarSoundElapsedTime(player);
        g_unk0x0058de10[player] = -1;
    }
    if (level != g_unk0x0058dd68[player] && level > g_unk0x0058dd68[player]) {
        if (rand() % 4 == 0)
            g_unk0x0058dd88[player] = StageObject_GetCarSoundElapsedTime(player) + 100;
        else
            g_unk0x0058dd88[player] = StageObject_GetCarSoundElapsedTime(player);
    }
    if (level >= 1 && level <= 3) {
        if ((unsigned int)(StageObject_GetCarSoundElapsedTime(player) - g_unk0x0058dd88[player]) < 100)
            dip = g_skidVolumeDip[(unsigned int)(StageObject_GetCarSoundElapsedTime(player) * 23 - g_unk0x0058dd88[player] * 23) / 100];
        else
            dip = 0;
    }
    if (speed < g_speedSoundCurve.max && level != 0) {
        if (pState[4] == 100) {
            g_unk0x0058dd78[player] += 10;
            if (g_unk0x0058dd78[player] > 100)
                g_unk0x0058dd78[player] = 100;
        } else {
            g_unk0x0058dd78[player] = 0;
        }
        if (!NetRace_GetRaceSoundMode()) {
            inRange = NetRace_IsValueWithinCurveRange(speed, &g_speedSoundCurve);
            if (inRange != 0) {
                pHandle = &g_unk0x0058dd80[player];
                if (Sound_IsPlaying(g_unk0x0058dd80[player]) == 0)
                    return;
                if (Race_GetPlayerRecordField4((BYTE)player))
                    Sound_SetPan(*pHandle, NetRace_ScaleViewAngleByDistance(player, player,
                                 NetRace_InterpolateWordCurve(speed, &g_speedSoundCurve)));
                else
                    Sound_SetPan(*pHandle, NetRace_InterpolateWordCurve(speed, &g_speedSoundCurve));
                volume = NetRace_InterpolateByteCurve(speed, &g_speedSoundCurve);
                if (locked)
                    return;
                if (level >= 1 && level <= 3)
                    volume -= dip;
                if (volume < 0)
                    volume = 10;
                volume = g_unk0x0058dd78[player] * volume / 100;
                if (inRange && g_unk0x0058de10[player] != -1)
                    StageObject_GetCarSoundElapsedTime(player);
                if (volume < 0)
                    volume = 0;
                else if (volume > 100)
                    volume = 100;
                Sound_SetPlayingSlotVolume(*pHandle, FixMul(NetRace_GetListenerDistanceAttenuation(player, listener),
                                              FixMul(g_unk0x0058dda8, FixMul((volume << 16) / 100, g_skidLoopVolumeScale))));
                return;
            }
        }
        if (Sound_IsPlaying(g_unk0x0058dd80[player])) {
            Sound_SetPlayingSlotVolume(g_unk0x0058dd80[player], 0);
            Sound_SetPan(g_unk0x0058dd80[player], 0x2b11);
        }
    } else if (Sound_IsPlaying(g_unk0x0058dd80[player])) {
        Sound_SetPlayingSlotVolume(g_unk0x0058dd80[player], 0);
        Sound_SetPan(g_unk0x0058dd80[player], 0x2b11);
    }
}

// Moves the player's value toward target, at most 20 up or 10 down per call.
// FUNCTION: CMR2 0x0047a380
void Surface_SmoothPlayerSoundLevel(int target, int player)
{
    int cur = g_unk0x0058de00[player];

    if (target - cur > 20) {
        target = cur + 20;
        g_unk0x0058de00[player] = target;
    } else if (cur - target > 10) {
        target = cur - 10;
        g_unk0x0058de00[player] = target;
    }
    if (target < 0)
        target = 0;
    g_unk0x0058de00[player] = target;
    g_unk0x0058de08[player] = target;
}

// GLOBAL: CMR2 0x0058dc58
DWORD g_unk0x0058dc58;
// GLOBAL: CMR2 0x0058dc5c
int g_unk0x0058dc5c;

void Menu_SetInputStateFlag(char param1);

// Menu setup of the surface screen: input repeat from the options and the
// button mapping stored in g_unk0x0058dc58.
// FUNCTION: CMR2 0x00478be0
void Surface_ConfigureMenuInput(void)
{
    int rate;

    g_unk0x0058dc5c = -1;
    CInput::SetMenuSoundVolume(((int)(CGameInfo::GetEffectsSoundVolume() << 16) / 100) / 4);
    CInput::SetMenuSounds(g_unk0x0058dc58, g_unk0x0058dc58 + 1, g_unk0x0058dc58 + 2, g_unk0x0058dc58 + 3,
                         g_unk0x0058dc58 + 4);
    Menu_SetInputStateFlag(1);
}

// GLOBAL: CMR2 0x0051e948
char g_strTxtFormat[] = "%s.txt";
// GLOBAL: CMR2 0x0051e950
char g_strLangPolish[] = "gpolish";
// GLOBAL: CMR2 0x0051e958
char g_strLangEngUsa[] = "gengusa";
// GLOBAL: CMR2 0x0051e960
char g_strLangGerman[] = "ggerman";
// GLOBAL: CMR2 0x0051e968
char g_strLangItalian[] = "gitalian";
// GLOBAL: CMR2 0x0051e974
char g_strLangSpanish[] = "gspanish";
// GLOBAL: CMR2 0x0051e980
char g_strLangFrench[] = "gfrench";
// GLOBAL: CMR2 0x0051e988
char g_strLangEnglish[] = "genglish";

#include "StageTiming.h"
#include "GenericFileLoader.h"
#include "Game.h"
#include <stdio.h>
#include <stddef.h>

BYTE Surface_FreeTextureData(void);

// Loads the game text of the region's language and splits it into the
// string table (release callback Surface_FreeTextureData).
// match 80%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00478a20
BYTE SurfaceText_LoadRegionalStringTable(void)
{
    char *names[10];
    char **pList;

    names[5] = g_strLangEnglish;
    names[6] = g_strLangFrench;
    names[7] = g_strLangSpanish;
    names[8] = g_strLangItalian;
    names[9] = g_strLangGerman;
    names[2] = g_strLangEngUsa;
    names[3] = g_strLangFrench;
    names[4] = g_strLangSpanish;
    names[0] = g_strLangEnglish;
    names[1] = g_strLangPolish;
    switch (CGameInfo::GetGameRegion()) {
    case 0:
        pList = &names[5];
        break;
    case 1:
        pList = &names[2];
        break;
    case 2:
        pList = &names[0];
        break;
    case 3:
        pList = &names[1];
        break;
    default:
        // the original loads names[1] itself as the list (a region outside 0..3)
        pList = (char **)names[1];
        break;
    }
    sprintf(CFrontend::m_stringDest, g_strTxtFormat, pList[CGameInfo::GetGameLanguage()]);
    g_unk0x0058dc54 = CGenericFileLoader::FindFile((GenericFile *)StageTiming_GetStageFile6(), CFrontend::m_stringDest,
                                                   (BYTE *)g_surfaceTexturePath, 0, 0);
    if (g_unk0x0058dc54 != NULL) {
        CFrontend::BuildLocalizedTextStringTable(1, 0x104, (BYTE **)&g_unk0x0058dc54);
        CGame::RegisterCallback(Surface_FreeTextureData, 0);
        return 1;
    }
    return 0;
}


// Blends the driving parameters of the two surfaces a wheel touches and stores
// the per-wheel block (grip, noise, effect bytes and the drag-derived value).
// match 63%: implemented; the two pointer walks use different index registers here
// FUNCTION: CMR2 0x004786b0
void Surface_BlendWheelContactParameters(Car *pWheel, int unused)
{
    int v;
    int total;
    int i;
    int *pMix;
    short *pId;
    int *pOut;
    BYTE *pOut2;
    int id;
    int blend;

    pMix = &pWheel->surfaceCompressionWords[(offsetof(Car, wheelSpinForBodyLean) + 3 * sizeof(int) - offsetof(Car, surfaceCompressionWords)) / sizeof(int)];
    pId = &pWheel->wheelSurface[3];
    pOut = &pWheel->surfaceOutputWords[(offsetof(Car, cornerGrip) + 3 * sizeof(CarCornerGrip) + offsetof(CarCornerGrip, grip2B)) / sizeof(int)];
    total = 0;
    i = 2;
    do {
        id = *pId;
        if (pMix[-4] != 0) {
            blend = pMix[-4];
            if (blend > 0x10000)
                blend = 0x10000;
            v = FixMul(blend, g_surfaceSoftness[id][0]);
            CarSurfacePairedGripOutput(pOut)->gripA = g_surfaceGrip[id][0] + v;
            CarSurfacePairedGripOutput(pOut)->grip2A = g_surfaceGrip2[id][0] + v;
            CarSurfacePairedGripOutput(pOut)->gripB = FixDiv(FixMul(g_surfaceGrip[id][0], g_surfaceGrip[id][1]), CarSurfacePairedGripOutput(pOut)->gripA);
            CarSurfacePairedGripOutput(pOut)->grip2B = FixDiv(FixMul(g_surfaceGrip2[id][0], g_surfaceGrip2[id][1]), CarSurfacePairedGripOutput(pOut)->grip2A);
        } else {
            CarSurfacePairedGripOutput(pOut)->gripA = g_surfaceGrip[id][0];
            CarSurfacePairedGripOutput(pOut)->grip2A = g_surfaceGrip2[id][0];
            CarSurfacePairedGripOutput(pOut)->gripB = g_surfaceGrip[id][1];
            CarSurfacePairedGripOutput(pOut)->grip2B = g_surfaceGrip2[id][1];
        }
        if (*pMix != 0) {
            blend = *pMix;
            if (blend > 0x10000)
                blend = 0x10000;
            v = FixMul(blend, g_surfaceSoftness[id][1]);
            CarSurfacePairedGripOutput(pOut)->gripB += v;
            CarSurfacePairedGripOutput(pOut)->grip2B += v;
        }
        pMix -= 2;
        CarSurfacePairedGripOutput(pOut)->drag = g_surface0x51e678[id];
        CarSurfacePairedGripOutput(pOut)->field_0x14 = g_surface0x51e738[id];
        CarSurfacePairedGripOutput(pOut)->field_0x18 = g_surface0x51e2b8[id];
        CarSurfacePairedGripOutput(pOut)->field_0x1c = g_surface0x51e4f8[id];
        CarSurfacePairedGripOutput(pOut)->field_0x20 = g_surface0x51e5b8[id];
        CarSurfacePairedGripOutput(pOut)[-1].grip2A = CarSurfacePairedGripOutput(pOut)->grip2A;
        CarSurfacePairedGripOutput(pOut)[-1].grip2B = CarSurfacePairedGripOutput(pOut)->grip2B;
        CarSurfacePairedGripOutput(pOut)[-1].gripA = CarSurfacePairedGripOutput(pOut)->gripA;
        CarSurfacePairedGripOutput(pOut)[-1].gripB = CarSurfacePairedGripOutput(pOut)->gripB;
        CarSurfacePairedGripOutput(pOut)[-1].drag = CarSurfacePairedGripOutput(pOut)->drag;
        CarSurfacePairedGripOutput(pOut)[-1].field_0x14 = CarSurfacePairedGripOutput(pOut)->field_0x14;
        CarSurfacePairedGripOutput(pOut)[-1].field_0x18 = CarSurfacePairedGripOutput(pOut)->field_0x18;
        CarSurfacePairedGripOutput(pOut)[-1].field_0x1c = CarSurfacePairedGripOutput(pOut)->field_0x1c;
        CarSurfacePairedGripOutput(pOut)[-1].field_0x20 = CarSurfacePairedGripOutput(pOut)->field_0x20;
        pOut -= 2 * sizeof(CarCornerGrip) / sizeof(*pOut);
        pId -= 2;
    } while (--i);

    pId = &pWheel->wheelSurface[3];
    pOut2 = &pWheel->surfaceOutputBytes[offsetof(Car, wheelSurfaceFx) + 3 * sizeof(CarWheelSurface) + offsetof(CarWheelSurface, effect) + 1];
    i = 2;
    do {
        id = *pId;
        CarSurfacePairedWheelOutput(pOut2)->effect[0] = g_surfaceEffect[id][0];
        CarSurfacePairedWheelOutput(pOut2)->effect[1] = g_surfaceEffect[id][1];
        CarSurfacePairedWheelOutput(pOut2)->drag =
            FixMul(0x51e, g_surfaceDrag[g_surfaceDragIndex[id] + pWheel->surfaceDragLevel * 9]);
        CarSurfacePairedWheelOutput(pOut2)->extraGrip = 0;
        total += g_surfaceNoise[id] * 2;
        CarSurfacePairedWheelOutput(pOut2)[-1].effect[0] = CarSurfacePairedWheelOutput(pOut2)->effect[0];
        CarSurfacePairedWheelOutput(pOut2)[-1].effect[1] = CarSurfacePairedWheelOutput(pOut2)->effect[1];
        CarSurfacePairedWheelOutput(pOut2)[-1].drag = CarSurfacePairedWheelOutput(pOut2)->drag;
        CarSurfacePairedWheelOutput(pOut2)[-1].extraGrip = CarSurfacePairedWheelOutput(pOut2)->extraGrip;
        pId -= 2;
        pOut2 -= 2 * sizeof(CarWheelSurface);
    } while (--i);

    total = (total & 0xfffffffc) << 14;
    pWheel->surfaceNoiseTarget = total;
    v = FixMul(total, 0x28f);
    blend = pWheel->surfaceNoise;
    pWheel->surfaceNoiseTarget = v;
    i = v - blend;
    if (i < 0)
        i = -i;
    if (i <= 0x3333) {
        pWheel->surfaceNoise = v;
        return;
    }
    if (v - blend > 0) {
        pWheel->surfaceNoise = blend + 0x3333;
        return;
    }
    pWheel->surfaceNoise = blend - 0x3333;
}

// Stops the surface sounds of every active player (the per-player handles set
// up by 0x478dc0) and frees the two shared surface sound handles.
// FUNCTION: CMR2 0x00478c40
void Surface_StopAndFreeSounds(void)
{
    int flag;
    int i;
    int *pHandle;

    if ((BYTE)RallyDataState() == 1 && (BYTE)RallyData_GetFlag25() != 0 &&
        CGameInfo::GetGameModeOptionBit19() == 0 && CGameInfo::GetConfiguredGameMode() != 3)
        flag = 1;
    else
        flag = 0;
    for (i = 0; i < (int)((RallyDataState() & 0xff) + flag); i = i + 1) {
        if (Sound_IsPlaying((unsigned int)g_unk0x0058ddf0[i]) != 0)
            Sound_SetPlayingSlotVolume((unsigned int)g_unk0x0058ddf0[i], 0);
        if (Sound_IsPlaying((unsigned int)g_unk0x0058dde0[i]) != 0)
            Sound_SetPlayingSlotVolume((unsigned int)g_unk0x0058dde0[i], 0);
        if (Sound_IsPlaying((unsigned int)g_unk0x0058dde8[i]) != 0)
            Sound_SetPlayingSlotVolume((unsigned int)g_unk0x0058dde8[i], 0);
        if (Sound_IsPlaying((unsigned int)g_unk0x0058ddac[i]) != 0)
            Sound_SetPlayingSlotVolume((unsigned int)g_unk0x0058ddac[i], 0);
        if (Sound_IsPlaying((unsigned int)g_unk0x0058dd80[i]) != 0)
            Sound_SetPlayingSlotVolume((unsigned int)g_unk0x0058dd80[i], 0);
        if (Sound_IsPlaying((unsigned int)g_unk0x0058ddc0[i]) != 0)
            Sound_SetPlayingSlotVolume((unsigned int)g_unk0x0058ddc0[i], 0);
        if (Sound_IsPlaying((unsigned int)g_unk0x0058dd90[i]) != 0)
            Sound_SetPlayingSlotVolume((unsigned int)g_unk0x0058dd90[i], 0);
        if (Sound_IsPlaying((unsigned int)g_unk0x0058ddd0[i]) != 0)
            Sound_SetPlayingSlotVolume((unsigned int)g_unk0x0058ddd0[i], 0);
    }
    pHandle = &g_unk0x0058ddc8Pair[0];
    while ((INT_PTR)pHandle < (INT_PTR)(g_unk0x0058ddc8Pair + 2)) { // 0x58ddd0 in the original
        if (*pHandle != -1)
            Sound_Free((unsigned int)*pHandle);
        pHandle++;
    }
}
