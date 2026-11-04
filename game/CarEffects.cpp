#include "port/sys.h"
#include "DustPlume.h"
#include "SnowGlint.h"
#include "LightField.h"
#include <cmath>
#include "AirMote.h"
extern FixVector g_particleWind;
void StageObject_GetRotatedStageLightVector(FixVector *pOut);
int StageObject_GetWeatherLightIntensity(void);
int Track_GetGroundHeightSurface(FixVector *pPoint, FixVector *pNormal, short *pTri, short *pSurfaceClass,
                                 unsigned short *pSurface, int defaultY);
#include "FogDynamics.h"
#include "RallyRoute.h"
#include "port/gfx.h"
#include <cmath>
#include "CarBodyDirt.h"
#include "WheelParticleEmission.h"
#include "ParticleAmount.h"
#include "CarInfo.h"
#include "FloatToInt.h"
#include "TyreWear.h"
#include <stdlib.h>
#include "Car.h"
#include "CarParts.h"
#include "Graphics.h"
#include "Particle.h"
#include "Sprite.h"
#include "RallyData.h"
#include "StageTiming.h"
#include "Glow.h"
#include "CarExhaust.h"

// Particle effects of the cars: breaking windows, glass shards and debris.

void Scene_GetLightColour(DWORD *pColour, int level);
unsigned char RallyData_GetSelectionFlag26(void);
void *CarInfo_GetSection(Car *pCar, int section);
short *Car_GetOrder(void);
short Car_GetOrderCount(void);
extern float g_oneOverRandMax;

#define EFFECT_RAND() (int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, rand()), g_oneOverRandMax), CGraphics::m_65536))

// A car window: four vertices of the car's window list, the Z of each
// mirrored for the other side of the car.
struct CarWindow {
    BYTE vertex[4];
    int mirrorZ[4];
};

// GLOBAL: CMR2 0x005203e0
CarWindow g_carWindows[6] = {
    {{0, 1, 1, 0}, {0, 0, 1, 1}}, {{4, 4, 3, 3}, {0, 1, 1, 0}}, {{0, 5, 2, 1}, {0, 0, 0, 0}},
    {{5, 4, 3, 2}, {0, 0, 0, 0}}, {{0, 1, 2, 5}, {1, 1, 1, 1}}, {{5, 2, 3, 4}, {1, 1, 1, 1}},
};

// GLOBAL: CMR2 0x00592870
BYTE g_windowSmashCount[8];         // queued window breaks per car
// Triangle of a spark streak.
// GLOBAL: CMR2 0x00592878
Quad2DInputVertex g_sparkTri[3];
// Random debris triangles.
// GLOBAL: CMR2 0x005928c0
FixVector g_debrisShapes[30][3];
// GLOBAL: CMR2 0x00592cf8
CarGlassProfile *g_carGlassProfiles[8]; // consumed glass section of each CIN file
// GLOBAL: CMR2 0x00592d18
FixVector *g_carWindowVerts[8];     // window vertices of each car
// Triangle of a debris piece.
// GLOBAL: CMR2 0x00592d38
Quad2DInputVertex g_debrisTri[3];
// Small random triangles used as glass shards.
// GLOBAL: CMR2 0x00592d80
FixVector g_glassShards[10][3];
// GLOBAL: CMR2 0x00592ee8
BYTE g_windowSmash[2][10];          // queued windows to break
// Break-up grid of a window (11 rows of 8 points).
// GLOBAL: CMR2 0x00592f38
FixVector g_windowGrid[11][8];
// GLOBAL: CMR2 0x00593358
FixVector g_windowSmashDir[2][10];  // impact direction of each queued break
// Triangle of a glass shard.
// GLOBAL: CMR2 0x00593718
Quad2DInputVertex g_shardTri[3];
// Colours of the debris pieces.
// GLOBAL: CMR2 0x005203d8
BYTE g_debrisColours[2][4] = {{0xff, 0xff, 0xff, 0xaa}, {0xff, 0, 0, 0xaa}};

// Prepares the effects for a stage: random glass shards and the window
// data of every car.
// match 79%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00498dd0
void CarEffects_Init(void)
{
    int *p;
    int *pBase;
    int *pV;
    FixVector mid;
    FixVector c;
    short *pOrder;
    int n;
    int j;
    int car;

    BYTE sparkColour[4] = {0xfe, 0xfe, 0xfe, 0xff};

    p = (int *)g_sparkTri[0].colour;
    do {
        *p = *(DWORD *)sparkColour;
        p += 6;
    } while ((INT_PTR)p < (INT_PTR)g_sparkTri[3].colour);
    pBase = (int *)g_glassShards[0] + 1; // &g_glassShards[0][0].y  (0x592d84 in the original)
    do {
        pV = pBase;

        for (j = 3; j != 0; j--) {
            pV[-1] = EFFECT_RAND();
            pV[0] = EFFECT_RAND();
            pV[1] = EFFECT_RAND();
            pV += 3;
        }
        mid.x = pBase[2] - pBase[-1];
        mid.y = pBase[3] - pBase[0];
        mid.z = pBase[4] - pBase[1];
        FixVecScale(&mid, &mid, 0x8000);
        mid.x += pBase[-1];
        mid.y += pBase[0];
        mid.z += pBase[1];
        c.x = pBase[5] - mid.x;
        c.y = pBase[6] - mid.y;
        c.z = pBase[7] - mid.z;
        FixVecScale(&c, &c, 0x8000);
        c.x += mid.x;
        c.y += mid.y;
        c.z += mid.z;
        pV = pBase - 1;
        for (j = 3; j != 0; j--) {
            pV[0] -= c.x;
            pV[1] -= c.y;
            pV[2] -= c.z;
            FixVecScale((FixVector *)pV, (FixVector *)pV, 0x3333);
            pV += 3;
        }
        pBase += 9;
    } while (pBase < (int *)g_glassShards[10] + 1);
    n = Car_GetOrderCount();
    pOrder = Car_GetOrder();
    while (--n >= 0) {
        car = pOrder[n];
        g_carGlassProfiles[car] = (CarGlassProfile *)CarInfo_GetSection(Car_Get(car), CAR_INFO_GLASS);
        g_carWindowVerts[car] = g_carGlassProfiles[car]->windowVertices;
    }
}

// Breaks a window of the car (quad corners in body space, pDir the impact
// direction): the window is cut into a grid and every cell throws a glass
// particle, faster near the impact and when the window faces the blow.
// match 68%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00499de0
void Car_ShatterWindow(FixVector *pQuad, FixVector *pDir, int window, Car *pCar)
{
    FixVector t;
    FixVector n;
    FixVector d;
    FixVector centre;
    FixVector u;
    FixVector v;
    FixVector pos;
    FixVector vel;
    FixVector *pRow;
    FixVector *pP;
    int param[2];
    int recip;
    int len;
    int facing;
    int r;
    int a;
    int b;
    int i;
    int j;

    param[0] = pCar->field_0xa70;
    pP = pQuad;
    for (i = 4; i != 0; i--) {
        FixMatrix_RotateVector(&t, pP, pCar->pWorld);
        *pP = t;
        pP++;
    }

    // Grid rows between the two side edges, then points along each row.
    u.x = pQuad[1].x - pQuad[0].x;
    u.y = pQuad[1].y - pQuad[0].y;
    u.z = pQuad[1].z - pQuad[0].z;
    v.x = pQuad[2].x - pQuad[3].x;
    v.y = pQuad[2].y - pQuad[3].y;
    v.z = pQuad[2].z - pQuad[3].z;
    i = 0;
    pRow = g_windowGrid[0];
    do {
        FixVecScale(&pRow[0], &u, FixMul(0x1999, i << 16));
        pRow[0].x += pQuad[0].x;
        pRow[0].y += pQuad[0].y;
        pRow[0].z += pQuad[0].z;
        FixVecScale(&pRow[7], &v, FixMul(0x1999, i << 16));
        pRow[7].x += pQuad[3].x;
        pRow[7].y += pQuad[3].y;
        i++;
        pRow[7].z += pQuad[3].z;
        pRow += 8;
    } while (pRow < g_windowGrid[11]);
    pRow = g_windowGrid[0];
    do {
        d.x = pRow[7].x - pRow[0].x;
        d.y = pRow[7].y - pRow[0].y;
        d.z = pRow[7].z - pRow[0].z;
        for (j = 1; j < 7; j++) {
            FixVecScale(&pRow[j], &d, FixMul(0x249b, j << 16));
            pRow[j].x += pRow[0].x;
            pRow[j].y += pRow[0].y;
            pRow[j].z += pRow[0].z;
        }
        pRow += 8;
    } while (pRow < g_windowGrid[11]);

    // Centre, size and facing of the window.
    d.x = pQuad[0].x - pQuad[2].x;
    d.y = pQuad[0].y - pQuad[2].y;
    d.z = pQuad[0].z - pQuad[2].z;
    recip = FixDiv(0x10000, FixMul(FixVecLength(&d), 0x8000));
    FixVecScale(&centre, &d, 0x8000);
    centre.x += pQuad[2].x;
    centre.y += pQuad[2].y;
    centre.z += pQuad[2].z;
    vel.x = pQuad[0].x - pQuad[1].x;
    vel.y = pQuad[0].y - pQuad[1].y;
    vel.z = pQuad[0].z - pQuad[1].z;
    FixVecCross(&d, &vel, &d);
    d.y = 0;
    FixMatrix_RotateVector(&vel, &d, pCar->pWorld);
    FIX_NORMALIZE_INTO(n, vel);
    facing = FixMul(0x6666, FixMul(FIX_ABS(FixVecDot(pDir, &n)), FixDiv(0x10000, 0x20000))) + 0x9999;
    if (facing > 0x10000)
        facing = 0x10000;

    // One particle per cell, at a random point of it.
    pRow = g_windowGrid[1];
    do {
        pP = pRow;
        for (i = 7; i != 0; i--) {
            u.x = pP[0].x - pP[-8].x;
            u.y = pP[0].y - pP[-8].y;
            u.z = pP[0].z - pP[-8].z;
            v.x = pP[-7].x - pP[-8].x;
            v.y = pP[-7].y - pP[-8].y;
            v.z = pP[-7].z - pP[-8].z;
            FixVecScale(&u, &u, EFFECT_RAND());
            FixVecScale(&v, &v, EFFECT_RAND());
            pos.y = pP[-8].y + u.y + v.y;
            pos.x = pP[-8].x + u.x + v.x;
            pos.z = pP[-8].z + u.z + v.z;
            t.x = pos.x - centre.x;
            t.y = pos.y - centre.y;
            t.z = pos.z - centre.z;
            len = FixVecLength(&t);
            if (len == 0) {
                t.x = 0;
                t.y = 0;
                t.z = 0;
            } else {
                FixVecScaleRecip(&t, &t, len);
            }
            len = FixMul(len, recip);
            if (len > 0xcccc)
                len = 0xcccc;
            r = FixMul(EFFECT_RAND(), 0x10000 - len);
            a = FixMul(facing, 0x1999);
            a = FixMul(r, a);
            b = FixMul(0x10000 - r, FixMul(facing, 0x1999));
            FixVecScale(&t, &t, b);
            FixVecScale(&vel, &n, a);
            vel.x += t.x;
            vel.y += t.y;
            vel.z += t.z;
            param[1] = (window == 7 || window == 6 ? 1 : 0) << 16;
            Particle_Spawn(0x1c, &pos, &vel, -0x40000, &pCar->pSceneRoot->current, NULL, 0, param,
                           pCar->pBodyNode->viewMask);
            pP++;
        }
        pRow += 8;
    } while (g_windowGrid[11] > pRow);
}

// Builds the quad of a window of the car from its window vertices.
// FUNCTION: CMR2 0x0049a870
int Car_BreakWindow(int window, FixVector *pDir, int unused, Car *pCar)
{
    FixVector *pVerts;
    FixVector quad[4];
    CarWindow *pWin;
    int i;

    pVerts = NULL;
    pWin = &g_carWindows[window];
    if (window <= 5)
        pVerts = g_carWindowVerts[pCar->index];
    for (i = 0; i < 4; i++) {
        quad[i] = pVerts[pWin->vertex[i]];
        if (pWin->mirrorZ[i] != 0)
            quad[i].z = -quad[i].z;
    }
    Car_ShatterWindow(quad, pDir, window, pCar);
    return 1;
}

// Queues the windows broken by damage to a part of the car; the rear and
// side part ids break two windows each.
// match 36%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0049a910
void Car_QueueWindowBreak(CarPartSet *pParts, Car *pCar, unsigned int part)
{
#define SMASH_SLOT (pCar->index * 10 + g_windowSmashCount[pCar->index])
    if (part == pParts->breakPartIndex[2]) {
        g_windowSmash[0][SMASH_SLOT] = 2;
        g_windowSmashDir[0][SMASH_SLOT] = pCar->field_0x5c4;
        g_windowSmashCount[pCar->index]++;
        g_windowSmash[0][SMASH_SLOT] = 3;
        g_windowSmashDir[0][SMASH_SLOT] = pCar->field_0x5c4;
        g_windowSmashCount[pCar->index]++;
    } else if (part == pParts->breakPartIndex[3]) {
        g_windowSmash[0][SMASH_SLOT] = 4;
        g_windowSmashDir[0][SMASH_SLOT] = pCar->field_0x5c4;
        g_windowSmashCount[pCar->index]++;
        g_windowSmash[0][SMASH_SLOT] = 5;
        g_windowSmashDir[0][SMASH_SLOT] = pCar->field_0x5c4;
        g_windowSmashCount[pCar->index]++;
    } else {
        if (part == pParts->breakPartIndex[0])
            g_windowSmash[0][SMASH_SLOT] = 0;
        else if (part == pParts->breakPartIndex[1])
            g_windowSmash[0][SMASH_SLOT] = 1;
        else
            return;
        g_windowSmashDir[0][SMASH_SLOT] = pCar->field_0x5c4;
        g_windowSmashCount[pCar->index]++;
    }
#undef SMASH_SLOT
}

// Breaks the queued windows of the car.
// FUNCTION: CMR2 0x0049ab20
void Car_BreakQueuedWindows(Car *pCar)
{
    int i;

    if (pCar->isGhost == 0) {
        i = g_windowSmashCount[pCar->index];
        while (--i >= 0)
            Car_BreakWindow(g_windowSmash[pCar->index][i], &g_windowSmashDir[pCar->index][i], 1, pCar);
        g_windowSmashCount[pCar->index] = 0;
    }
}

// Texture coordinates of the effect triangles.
// FUNCTION: CMR2 0x00499020
void CarEffects_InitUVs(Texture *unused1, Texture *unused2)
{
    g_sparkTri[0].u = 0x8000;
    g_sparkTri[0].v = 0;
    g_sparkTri[1].u = 0xffff;
    g_sparkTri[1].v = 0xffff;
    g_sparkTri[2].u = 0;
    g_sparkTri[2].v = 0xffff;
    g_shardTri[0].u = 0;
    g_shardTri[0].v = 0;
    g_shardTri[1].u = 0xffff;
    g_shardTri[1].v = 0xffff;
    g_shardTri[2].u = 0;
    g_shardTri[2].v = 0xffff;
}

// Position of a particle: relative to its matrix when it has one.
#define PARTICLE_POS(pos, p)                                          \
    pos = (p)->vector0x1c;                                            \
    if ((p)->attachmentMatrix != 0) {                                        \
        FixMatrix_GetPosition(&o, (p)->attachmentMatrix);       \
        pos.x += o.x;                                                 \
        pos.y += o.y;                                                 \
        pos.z += o.z;                                                 \
    }

// Draw callback of a spark: a streak along its velocity, facing the view.
// match 74%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00499070
void Spark_Draw(Particle *p, ParticleType *pType, SceneNode *pView)
{
    FixVector pos;
    FixVector o;
    FixVector cam;
    FixVector d;
    FixVector side;
    int len;

    if (p->age >= 0x130000)
        return;
    PARTICLE_POS(pos, p);
    FixMatrix_GetPosition(&cam, &pView->current);
    cam.y -= pos.y;
    cam.x -= pos.x;
    cam.z -= pos.z;
    FixVecCross(&side, &cam, &p->position);
    FIX_NORMALIZE_INTO(side, side);
    len = FixVecLength(&p->position);
    if (len > 0x5999) {
        FixVecScaleRecip(&cam, &p->position, len);
        FixVecScale(&cam, &cam, 0x5999);
        len = 0x5999;
    } else {
        cam = p->position;
    }
    FixVecScale(&side, &side, FixMul(len, 0x4000));
    FixVecScale(&d, &cam, 0x20000);
    g_sparkTri[0].x = d.x + pos.x;
    g_sparkTri[0].y = d.y + pos.y;
    g_sparkTri[0].z = d.z + pos.z;
    g_sparkTri[2].x = side.x + pos.x;
    g_sparkTri[1].x = pos.x - side.x;
    g_sparkTri[2].y = side.y + pos.y;
    g_sparkTri[2].z = side.z + pos.z;
    g_sparkTri[1].z = pos.z - side.z;
    g_sparkTri[1].y = pos.y - side.y;
    g_sparkTri[0].colour[3] = g_sparkTri[1].colour[3] = g_sparkTri[2].colour[3] = p->type0x53;
    Quad2D_QueueFixedTriangle(0, &g_sparkTri[0], &g_sparkTri[1], &g_sparkTri[2], pType->texture,
                              0x14);
}

// Lit colour of a car piece: the car's paint scaled by the scene light.
#define EFFECT_LIT_COLOUR(out, level, pPaint)                                     \
    Scene_GetLightColour((DWORD *)light, (level));                                \
    lr = FixMul(light[0] << 16, 0x106);                                           \
    if (lr > 0x10000)                                                             \
        lr = 0x10000;                                                             \
    lg = FixMul(light[1] << 16, 0x106);                                           \
    if (lg > 0x10000)                                                             \
        lg = 0x10000;                                                             \
    lb = FixMul(light[2] << 16, 0x106);                                           \
    if (lb > 0x10000)                                                             \
        lb = 0x10000;

// Update callback: once old enough the particle switches to its type's look.
// FUNCTION: CMR2 0x004994d0
void CarEffects_UpdateAgedParticleAppearance(Particle *p, ParticleType *pType, SceneNode *pView)
{

    if (p->age >= 0x78000)
        p->type0x56 = pType->type;
}

// Draw callback of a glass shard, tinted like the car's windows.
// match 79%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004994f0
void GlassShard_Draw(Particle *p, ParticleType *pType, SceneNode *unused)
{
    FixVector pos;
    BYTE light[8];
    int tint;
    int car;
    int shard;
    int k_idx;
    BYTE *pTint;
    DWORD *pC;

    if (p->age >= 0x130000)
        return;
    pos = p->vector0x1c;
    {
        FixVector o;

        FixMatrix_GetPosition(&o, p->attachmentMatrix);
        pos.x += o.x;
        pos.y += o.y;
        pos.z += o.z;
    }
    car = p->field0x64 >> 8;
    shard = p->field0x64 - car * 0x100;
    tint = 0;
    if (shard >= 3)
        tint = (shard >= 6) + 1;
    pTint = g_carGlassProfiles[car]->tint[tint];
    for (k_idx = 0; k_idx < 3; k_idx++) {
        g_shardTri[k_idx].x = ((FixVector *)(g_glassShards[shard]))[k_idx].x + pos.x;
        g_shardTri[k_idx].y = ((FixVector *)(g_glassShards[shard]))[k_idx].y + pos.y;
        g_shardTri[k_idx].z = ((FixVector *)(g_glassShards[shard]))[k_idx].z + pos.z;
        g_shardTri[k_idx].colour[0] = pTint[0];
        g_shardTri[k_idx].colour[1] = pTint[1];
        g_shardTri[k_idx].colour[2] = pTint[2];
        g_shardTri[k_idx].colour[3] = 0xff;
    }
    Scene_GetLightColour((DWORD *)light, p->size);
    pos.x = FixMul(light[0] << 16, 0x106);
    if (pos.x > 0x10000)
        pos.x = 0x10000;
    pos.y = FixMul(light[1] << 16, 0x106);
    if (pos.y > 0x10000)
        pos.y = 0x10000;
    pos.z = FixMul(light[2] << 16, 0x106);
    if (pos.z > 0x10000)
        pos.z = 0x10000;
    pos.x = FixMul(pos.x, g_carGlassProfiles[car]->tint[tint][0] << 16);
    pos.y = FixMul(pos.y, g_carGlassProfiles[car]->tint[tint][1] << 16);
    pos.z = FixMul(pos.z, g_carGlassProfiles[car]->tint[tint][2] << 16);
    light[0] = (BYTE)(pos.x >> 16);
    light[1] = (BYTE)(pos.y >> 16);
    light[2] = (BYTE)(pos.z >> 16);
    light[3] = 0xff;
    pC = (DWORD *)g_shardTri[0].colour;
    do {
        *pC = *(DWORD *)light;
        pC += 6;
    } while ((INT_PTR)pC < (INT_PTR)g_shardTri[3].colour);
    Quad2D_QueueFixedTriangle(0, &g_shardTri[0], &g_shardTri[1], &g_shardTri[2], pType->texture,
                              0x14);
}

// Spawn callback of a glass shard: remembers the car and picks a shard.
// FUNCTION: CMR2 0x00499710
void GlassShard_Init(Particle *p, ParticleType *pType, void *owner)
{
    Car *pCar = (Car *)owner;
    p->field0x64 = (unsigned short)((BYTE)pCar->index << 8);
    p->field0x64 += (short)(rand() % 10);
    p->size = pCar->field_0xa70;
}

// Throws debris (and, a limited number of times per car, glass) from a
// point of the car: count pieces with random velocities along the given
// axes.
// match 56%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00499750
void Car_SpawnDebris(int size, FixVector *pPos, Car *pCar, FixVector *pAxes, int count, int glassChance)
{
    FixVector off[3];
    FixVector vel;
    int ext[3];
    CarPartSet *pRecord;
    int *pExt;
    FixVector *pOff;
    int glass;
    int n;
    int k;
    BYTE light;

    glass = 0;
    pRecord = CarPart_GetSet(pCar->index);
    if ((BYTE)RallyData_GetSelectionFlag26() != 0 || pCar->isGhost != 0)
        return;
    n = EFFECT_RAND();
    n = (FixMul(count, n) >> 16) + 1;
    ext[0] = FixMul(0x3333, size);
    ext[1] = FixMul(0x1999, size);
    ext[2] = FixMul(0xccc, size);
    if (n <= 0)
        return;
    do {
        pExt = ext;
        pOff = off;
        for (k = 3; k != 0; k--) {
            FixVecScale(pOff, (FixVector *)((BYTE *)pAxes + ((BYTE *)pOff - (BYTE *)off)), FixMul(EFFECT_RAND(), *pExt));
            pExt++;
            pOff++;
        }
        if (EFFECT_RAND() < 0x8000) {
            off[0].x += off[1].x;
            off[0].y += off[1].y;
            off[0].z += off[1].z;
        } else {
            off[0].x -= off[1].x;
            off[0].y -= off[1].y;
            off[0].z -= off[1].z;
        }
        if (EFFECT_RAND() < 0x8000) {
            vel.x = off[2].x + off[0].x;
            vel.y = off[2].y + off[0].y;
            vel.z = off[2].z + off[0].z;
        } else {
            vel.x = off[0].x - off[2].x;
            vel.y = off[0].y - off[2].y;
            vel.z = off[0].z - off[2].z;
        }
        if (EFFECT_RAND() < glassChance) {
            light = pCar->pBodyNode->viewMask;
            if (pRecord->glassCooldown < 8) {
                Particle_Spawn(0x1b, pPos, &vel, pPos->y - 0x1999, &pCar->pSceneRoot->current, NULL, 0,
                               pCar, light);
                glass = 1;
            } else {
                Particle_Spawn(0x1a, pPos, &vel, pPos->y - 0x1999, &pCar->pSceneRoot->current, NULL, 0, 0,
                               light);
                glass = 1;
            }
        } else {
            Particle_Spawn(0x1a, pPos, &vel, pPos->y - 0x1999, &pCar->pSceneRoot->current, NULL, 0, 0,
                           pCar->pBodyNode->viewMask);
        }
    } while (--n != 0);
    if (glass != 0) {
        pRecord->glassDebrisEmitted = 1;
        if (pRecord->glassCooldown < 0xfe)
            pRecord->glassCooldown += 2;
    }
}

// Draw callback of a debris piece: a random triangle, lit, bulging
// towards the view.
// FUNCTION: CMR2 0x00499ac0
void Debris_Draw(Particle *p, ParticleType *pType, SceneNode *pView)
{
    FixVector pos;
    FixVector o;
    FixVector cam;
    BYTE light[4];
    DWORD *pC;
    int shape;
    int colour;
    FixVector lighting;
    int len;
    int k;

    shape = p->field0x64 >> 8;
    colour = p->field0x64 - shape * 0x100;
    Scene_GetLightColour((DWORD *)light, p->size);
    lighting.x = FixMul(light[0] << 16, 0x106);
    if (lighting.x > 0x10000) lighting.x = 0x10000;
    lighting.y = FixMul(light[1] << 16, 0x106);
    if (lighting.y > 0x10000) lighting.y = 0x10000;
    lighting.z = FixMul(light[2] << 16, 0x106);
    if (lighting.z > 0x10000) lighting.z = 0x10000;
    lighting.x = FixMul(lighting.x, g_debrisColours[colour][0] << 16);
    lighting.y = FixMul(lighting.y, g_debrisColours[colour][1] << 16);
    lighting.z = FixMul(lighting.z, g_debrisColours[colour][2] << 16);
    light[0] = (BYTE)(lighting.x >> 16);
    light[1] = (BYTE)(lighting.y >> 16);
    light[2] = (BYTE)(lighting.z >> 16);
    light[3] = 0xaa;
    pC = (DWORD *)g_debrisTri[0].colour;
    do {
        *pC = *(DWORD *)light;
        pC += 6;
    } while ((INT_PTR)pC < (INT_PTR)g_debrisTri[3].colour);
    pos = p->vector0x1c;
    FixMatrix_GetPosition(&o, p->attachmentMatrix);
    pos.x += o.x;
    pos.y += o.y;
    pos.z += o.z;
    for (k = 0; k < 3; k++) {
        g_debrisTri[k].x = g_debrisShapes[shape][k].x + pos.x;
        g_debrisTri[k].y = g_debrisShapes[shape][k].y + pos.y;
        g_debrisTri[k].z = g_debrisShapes[shape][k].z + pos.z;
    }
    FixMatrix_GetPosition(&cam, &pView->current);
    for (colour = 0; colour < 3; colour++) {
        o.x = cam.x - g_debrisTri[colour].x;
        o.y = cam.y - g_debrisTri[colour].y;
        o.z = cam.z - g_debrisTri[colour].z;
        len = FixVecLength(&o);
        if (len > 0x18000) {
            k = FixDiv(0x18000, len);
            FixVecScale(&o, &o, k);
        }
        g_debrisTri[colour].x += o.x;
        g_debrisTri[colour].y += o.y;
        g_debrisTri[colour].z += o.z;
    }
    Quad2D_QueueFixedTriangle(0, &g_debrisTri[0], &g_debrisTri[1], &g_debrisTri[2], pType->texture,
                              0x14);
}

// Makes the random debris triangles (at most 0.1 from their centre).
// match 65%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0049ab90
void CarEffects_InitDebris(void)
{
    FixVector *pShape;
    FixVector *pV;
    FixVector mid;
    FixVector c;
    int len;
    int i;

    g_debrisTri[0].v = 0;
    g_debrisTri[1].u = 0;
    g_debrisTri[1].v = 0x10000;
    g_debrisTri[2].u = 0x10000;
    g_debrisTri[2].v = 0x10000;
    g_debrisTri[0].u = 0x8000;
    pShape = g_debrisShapes[0];
    do {
        pShape[0].x = EFFECT_RAND();
        pShape[0].y = EFFECT_RAND();
        pShape[0].z = EFFECT_RAND();
        pShape[1].x = EFFECT_RAND();
        pShape[1].y = EFFECT_RAND();
        pShape[1].z = EFFECT_RAND();
        pShape[2].x = EFFECT_RAND();
        pShape[2].y = EFFECT_RAND();
        pShape[2].z = EFFECT_RAND();
        mid.y = pShape[1].y - pShape[0].y;
        mid.x = pShape[1].x - pShape[0].x;
        mid.z = pShape[1].z - pShape[0].z;
        FixVecScale(&mid, &mid, 0x8000);
        mid.z += pShape[0].z;
        mid.x += pShape[0].x;
        c.x = pShape[2].x - mid.x;
        mid.y += pShape[0].y;
        c.y = pShape[2].y - mid.y;
        c.z = pShape[2].z - mid.z;
        FixVecScale(&c, &c, 0x8000);
        c.y += mid.y;
        c.x += mid.x;
        c.z += mid.z;
        pV = pShape;
        for (i = 3; i != 0; i--) {
            pV->x -= c.x;
            pV->y -= c.y;
            pV->z -= c.z;
            len = FixVecLength(pV);
            if (len > 0x1999)
                FixVecScale(pV, pV, FixDiv(0x1999, len));
            pV++;
        }
        pShape += 3;
    } while (pShape < g_debrisShapes[30]);
}

TyreWear *RallyData_GetDriverTyreWear(BYTE index);
void RallyData_MarkDriverRecordDirty(int index);
BYTE StageUI_GetRaceEndEventCount(void);
int StageObject_GetCarWeatherRampValue(Car *pCar);
int Car_GetWheelSpeed(Car *pCar, BYTE wheel, int unit);

// Wheel spray effects: colour, whether it is dust (longer, flatter), whether
// it is thrown even on a slow surface, and the countdown range between
// particles.
// GLOBAL: CMR2 0x005206f8
DWORD g_sprayColour[13] = {0x003b4c5d, 0x005f6b78, 0x006f7576, 0x00065487, 0x0003446f, 0x00003a68, 0x00667daf,
                           0x00728abd, 0x00728abd, 0x006f7576, 0x006f7576, 0x0016262c, 0x004c5154};
// GLOBAL: CMR2 0x0052072c
int g_sprayDust[13] = {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0};
// GLOBAL: CMR2 0x00520760
int g_sprayAlways[13] = {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0};
// GLOBAL: CMR2 0x00520794
char g_sprayInterval[13][2] = {{10, 2}, {10, 2}, {4, 4}, {4, 4}, {2, 2}, {2, 2}, {4, 4},
                               {3, 3}, {2, 2}, {3, 3}, {2, 2}, {10, 2}, {10, 2}};

// GLOBAL: CMR2 0x00593860
short g_spraySurface;          // last surface looked up
// GLOBAL: CMR2 0x00593864
int g_sprayHalf;
// GLOBAL: CMR2 0x00593868
int g_sprayRange;
// GLOBAL: CMR2 0x0059386c
short g_sprayEffect;           // its effect
// GLOBAL: CMR2 0x00593870
int g_sprayRecip;
// GLOBAL: CMR2 0x00593874
char g_sprayCountdown[8][4];
// GLOBAL: CMR2 0x00593894
int g_sprayStep;
// GLOBAL: CMR2 0x00593898
int g_sprayThird;
// GLOBAL: CMR2 0x0059389c
BYTE g_sprayLife;

// Sets up the wheel spray for a spread and range.
// match 70%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0049af20
int WheelSpray_Init(int spread, int range)
{
    int result;
    int i;
    int *p;

    g_sprayRange = range;
    g_sprayStep = FixDiv(FixMul(0x40000, spread), g_sprayRange);
    g_sprayHalf = FixMul(g_sprayStep, 0x8000);
    g_sprayThird = FixMul(g_sprayStep, 0x5553);
    g_sprayRecip = FixDiv(0x10000, FixSqrt(FixMul(g_sprayStep, g_sprayStep) +
                                            FixMul(g_sprayStep, g_sprayStep) +
                                            FixMul(g_sprayThird, g_sprayThird)));
    result = FixDiv(FixMul(0x80000, spread), FixMul(g_sprayRange, g_sprayRange));
    g_sprayLife = (BYTE)(FixDiv(range, 0xc0000) >> 16);
    p = (int *)g_sprayCountdown;
    for (i = 8; i != 0; i--)
        *p++ = 0;
    g_sprayEffect = -1;
    g_spraySurface = -1;
    return result;
}

// Spray effect thrown on a surface in the current country (-1 for none).
// match 54%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0049b0d0
int WheelSpray_GetEffect(int surface)
{
    if (surface != g_spraySurface) {
        g_sprayEffect = -1;
        switch ((BYTE)RallyDataCountryIndex()) {
        case 0:
            switch (surface) {
            case 0xd:
                g_sprayEffect = 2;
                break;
            case 0x13:
                g_sprayEffect = 1;
                break;
            case 0x1a:
                g_sprayEffect = 0;
                break;
            }
            break;
        case 4:
            switch (surface) {
            case 2:
                g_sprayEffect = 3;
                break;
            case 0x47:
                g_sprayEffect = 5;
                break;
            case 0x53:
                g_sprayEffect = 4;
                break;
            }
            break;
        case 2:
            switch (surface) {
            case 0x1a:
            case 0x22:
                g_sprayEffect = 0;
                break;
            case 0x1c:
            case 0x1d:
                g_sprayEffect = 1;
                break;
            }
            break;
        case 1:
            switch (surface) {
            case 3:
                g_sprayEffect = 6;
                break;
            case 4:
            case 6:
                g_sprayEffect = 7;
                break;
            case 5:
            case 7:
                g_sprayEffect = 8;
                break;
            case 8:
                g_sprayEffect = 2;
                break;
            case 9:
            case 0xb:
                g_sprayEffect = 9;
                break;
            case 10:
            case 0xc:
                g_sprayEffect = 10;
                break;
            case 0x1b:
                g_sprayEffect = 1;
                break;
            }
            break;
        case 5:
            switch (surface) {
            case 3:
            case 8:
            case 0xd:
            case 0x10:
            case 0x11:
            case 0x12:
            case 0x13:
            case 0x1b:
            case 0x1c:
                g_sprayEffect = 3;
                break;
            }
            break;
        case 7:
            switch (surface) {
            case 0xf:
                g_sprayEffect = 9;
                break;
            case 0x13:
            case 0x14:
                g_sprayEffect = 0xb;
                break;
            case 0x1a:
            case 0x1f:
                g_sprayEffect = 0xc;
                break;
            }
            break;
        }
    }
    g_spraySurface = surface;
    return g_sprayEffect;
}


// Adds wear to a tyre of a driver and updates its state (1 worn, 2 badly
// worn, 3 damaged).
// FUNCTION: CMR2 0x0045c750
void Tyre_AddWear(int car, int wheel, int damage, int wear)
{
    TyreWear *pRecord;
    int i;

    pRecord = RallyData_GetDriverTyreWear(StageUI_GetRaceEndEventCount() + (char)car);
    if (pRecord == NULL)
        return;
    if (pRecord->damage[wheel] < 1000)
        pRecord->damage[wheel] += damage;
    if (pRecord->wear[wheel] < 1000)
        pRecord->wear[wheel] += wear;
    if (pRecord->damage[wheel] > pRecord->peakDamage[wheel])
        pRecord->peakDamage[wheel] = pRecord->damage[wheel];
    if (pRecord->wear[wheel] > pRecord->peakWear[wheel])
        pRecord->peakWear[wheel] = pRecord->wear[wheel];
    pRecord->state[wheel] = 0;
    for (i = 0; i < 4; i++) {
        if (pRecord->damage[i] > 0x32)
            pRecord->state[i] = 3;
        if (pRecord->wear[i] > 0x32)
            pRecord->state[i] = 1;
        if (pRecord->wear[i] > 300)
            pRecord->state[i] = 2;
    }
    RallyData_MarkDriverRecordDirty((StageUI_GetRaceEndEventCount() & 0xff) + car);
}

// Throws the spray of the car's sliding wheels (gravel, mud, dust, water)
// and wears the tyres on the loose surfaces.
// match 46%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0049b3e0
void WheelSpray_Update(int player)
{
    Car *pCar;
    FixVector *pEmit;
    int *pSlip;
    short *pSurface;
    FixVector v;
    FixVector world;
    FixVector vel;
    FixVector basis[3];
    DWORD colour;
    int wheel;
    int effect;
    int wet;
    int *pDust;
    int slip;
    int lat;
    int absSlip;
    int absLat;
    int cs;
    int sn;
    int r;
    int ground;
    int side;
    int along;
    int up;
    int across;
    int s;
    int n;
    BYTE level;

    pCar = Car_Get(player);
    if (pCar->field_0xb74 == 0)
        return;
    wheel = 0;
    pEmit = pCar->wheelEmitter;
    pSlip = pCar->wheelSlip;
    pSurface = pCar->wheelSurfaceType;
    do {
        if (pSlip[0xcf] != 0) {
            effect = WheelSpray_GetEffect(*pSurface);
            wet = StageObject_GetCarWeatherRampValue(pCar);
            // OpenCMR2: video.spray_amount (0..300, default 200 - deliberately
            // overdone, tone down in the tuning panel) scales the emission
            // interval; 0 disables wheel spray. Step-based, frame-rate
            // independent, original intervals untouched.
            int sprayAmount = Sys_GetOption("video.spray_amount", 200);
            if (sprayAmount <= 0)
                continue;
            if (effect >= 0 && (wet != 0 || g_sprayAlways[effect] != 0) &&
                --g_sprayCountdown[pCar->index][wheel] <= 0) {
                pDust = &g_sprayDust[effect];
                if (g_sprayDust[effect] == 0) {
                    if ((Car_GetWheelSpeed(Car_Get(player), wheel, 0) < 0 ?
                         -Car_GetWheelSpeed(Car_Get(player), wheel, 0) :
                         Car_GetWheelSpeed(Car_Get(player), wheel, 0)) > 0x1e0000)
                        Tyre_AddWear(pCar->index, wheel, 0, 1);
                }
                g_sprayCountdown[pCar->index][wheel] =
                    (char)ParticleAmount_ScaledInterval(
                        (FixMul((g_sprayInterval[effect][1] - g_sprayInterval[effect][0]) << 16, wet) >> 16) +
                        g_sprayInterval[effect][0], sprayAmount);
                if (pSlip[0xde] != 0)
                    slip = *pSlip;
                else
                    slip = FixMul(*pSlip, 0x4ccc);
                lat = pSlip[4];
                absSlip = FIX_ABS(slip);
                absLat = FIX_ABS(lat);
                if (pSlip[0xde] != 0 && absLat + absSlip > 0x5999) {
                    // Direction of the spray on the ground: the front wheels
                    // throw it along the body, the rear ones as they slide.
                    if (wheel < 2) {
                        cs = g_sinTable[(pCar->wheelSteeringAngle + 0x400) & 0xfff];
                        sn = g_sinTable[pCar->wheelSteeringAngle & 0xfff];
                        basis[0].x = FixMul(slip, cs);
                        basis[0].y = 0;
                        basis[0].z = FixMul(slip, -sn);
                        v.x = FixMul(lat, sn);
                        v.y = 0;
                        v.z = FixMul(cs, lat);
                        if (*pDust != 0) {
                            FixVecScale(&basis[0], &basis[0], 0x20000);
                            FixVecScale(&v, &v, 0x8000);
                        }
                        v.x += basis[0].x;
                        v.y = 0;
                        v.z += basis[0].z;
                    } else {
                        v.x = slip;
                        v.y = 0;
                        v.z = lat;
                        if (*pDust != 0) {
                            v.x = FixMul(v.x, 0x20000);
                            v.z = FixMul(v.z, 0x8000);
                        }
                    }
                    r = FixDiv(0x10000, FixSqrt(FixMul(v.x, v.x) + FixMul(v.z, v.z)));
                    basis[0].x = FixMul(r, v.x);
                    basis[0].y = 0;
                    basis[0].z = FixMul(r, v.z);
                    basis[1].x = 0;
                    basis[1].y = 0x10000;
                    basis[1].z = 0;
                    basis[2].x = -basis[0].z;
                    basis[2].y = 0;
                    basis[2].z = basis[0].x;
                    v = *pEmit;
                    v.y -= 0x5999;
                    FixMatrix_RotateVector(&world, &v, pCar->pWorld);
                    ground = world.y - 0x1999;
                    side = wheel % 2;
                    along = EFFECT_RAND();
                    up = EFFECT_RAND();
                    across = EFFECT_RAND();
                    along = g_sprayStep - FixMul(along, g_sprayHalf);
                    up = g_sprayStep - FixMul(up, g_sprayHalf);
                    across = g_sprayThird - FixMul(across, g_sprayThird);
                    if (EFFECT_RAND() > 0x8000)
                        across = -across;
                    s = FixSqrt(FixMul(slip, slip) + FixMul(lat, lat));
                    s = FixMul(s, 0x8000);
                    s = FixMul(s, g_sprayRecip);
                    if (s > 0x10000)
                        s = 0x10000;
                    along = FixMul(along, s);
                    up = FixMul(up, s);
                    across = FixMul(s, across);
                    if (*pDust != 0) {
                        along = FixMul(along, 0x20000);
                        up = FixMul(up, 0x8000);
                        across = FixMul(across, 0xb333);
                    }
                    vel.x = 0;
                    vel.y = 0;
                    vel.z = 0;
                    FixVecScale(&v, &basis[0], along);
                    vel.x += v.x;
                    vel.y += v.y;
                    vel.z += v.z;
                    FixVecScale(&v, &basis[1], up);
                    vel.x += v.x;
                    vel.y += v.y;
                    vel.z += v.z;
                    FixVecScale(&v, &basis[2], across);
                    vel.x += v.x;
                    vel.y += v.y;
                    vel.z += v.z;
                    // Only throw it away from the car.
                    n = 0;
                    if (wheel < 2) {
                        if (vel.x < 0)
                            n = 1;
                    } else if (vel.x > 0) {
                        n = 1;
                    }
                    if (side == 0) {
                        if (vel.z < 0)
                            n++;
                    } else if (vel.z > 0) {
                        n++;
                    }
                    if (n < 2) {
                        level = (BYTE)(-1 - (char)((unsigned int)(pCar->field_0xa70 * 0xff) >> 16));
                        FixMatrix_RotateVector(&v, &vel, pCar->pWorld);
                        colour = g_sprayColour[effect];
                        if (*pDust != 0)
                            Particle_Spawn(0x1d, &world, &v, ground, &pCar->pSceneRoot->current,
                                           (BYTE *)&colour, level, 0, pCar->pBodyNode->viewMask);
                        else
                            Particle_Spawn(0x1e, &world, &v, ground, &pCar->pSceneRoot->current,
                                           (BYTE *)&colour, level, 0, pCar->pBodyNode->viewMask);
                    }
                }
            }
        }
        wheel++;
        pSurface++;
        pSlip++;
        pEmit++;
    } while (wheel < 4);
}

#include "WheelTrail.h"

BYTE StageObject_GetViewWeatherStateByte(int index);
int StageObject_GetViewWeatherField54(int index);
extern CameraRecord g_viewStates[2];

#define EFFECT_RAND_NEG() (int)FTOI(__int64, X87S_MUL(X87S_MUL(X87S_FILD(float, rand()), g_oneOverRandMax), g_minus65536))
extern double g_minus65536;

// Colour of the wheel splashes and its brightened copy.
struct SplashColour {
    BYTE c0;
    BYTE c1;
    BYTE c2;
    BYTE c3;
};
// GLOBAL: CMR2 0x005435c0
SplashColour g_unk0x005435c0;
// GLOBAL: CMR2 0x005435cc
SplashColour g_unk0x005435cc;
// GLOBAL: CMR2 0x00543808
BYTE g_unk0x00543808;

// Sets the splash colour of a player's car (and its average brightness).
// FUNCTION: CMR2 0x0045d1e0
void CarEffects_SetSplashColour(int player, BYTE *pColour)
{
    if (player < 8) {
        g_unk0x005435c0 = *(SplashColour *)pColour;
        g_unk0x005435cc.c0 = (g_unk0x005435c0.c0 >> 1) + 0x7f;
        g_unk0x005435cc.c1 = (g_unk0x005435c0.c1 >> 1) + 0x7f;
        g_unk0x005435cc.c2 = (g_unk0x005435c0.c2 >> 1) + 0x7f;
        g_unk0x005435cc.c3 = g_unk0x005435c0.c3;
        g_unk0x00543808 = (BYTE)((pColour[0] + pColour[1] + pColour[2]) / 3);
    }
}
// Update callback: lifts the particle by half its size.
// FUNCTION: CMR2 0x0045d250
void CarEffects_UpdateRisingParticle(Particle *p, ParticleType *pType, SceneNode *pView)
{

    p->vector0x28.y += p->size / 2;
}

// Update callback: moves the particle with the car stored in its effect data.
// FUNCTION: CMR2 0x0045dea0
void CarEffects_UpdateParticleAtCarPosition(Particle *p, ParticleType *pType, SceneNode *pView)
{
    int car = p->field0x64;

    if (car < 8) {
        p->vector0x28.x += Car_Get(car)->position.x;
        p->vector0x28.y += Car_Get(car)->position.y;
        p->vector0x28.z += Car_Get(car)->position.z;
    }
}

// Update callback: rises by half its size (capped at 1.5) and follows its car.
// FUNCTION: CMR2 0x0045d270
void CarEffects_UpdateCarFollowingRisingParticle(Particle *p, ParticleType *pType, SceneNode *pView)
{
    Car *pCar;
    int car;

    p->vector0x28.y += p->size / 2;
    if (p->size > 0x18000)
        p->size = 0x18000;
    car = p->field0x64;
    if (car < 8) {
        pCar = Car_Get(car);
        p->vector0x28.x += pCar->position.x;
        p->vector0x28.y += pCar->position.y;
        p->vector0x28.z += pCar->position.z;
    }
}

// Exhaust emitter offsets of each car, rotated into the world.
// GLOBAL: CMR2 0x00543380
FixVector g_carExhaustWorldPositions[8];
// GLOBAL: CMR2 0x00543cf8
int g_exhaustParticleSide;
int FixMatrix_RotateVector(FixVector *pOut, FixVector *pV, FixMatrix *pM);

// Spawn callback: places the particle at one of the car's two exhausts
// (alternating) in world space.
// match 61%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0045d2d0
void CarEffect_SpawnExhaustParticle(Particle *p, ParticleType *pType, SceneNode *pView)
{
    int car = p->field0x64;
    Car *pCar;
    FixMatrix *pM;
    int x;
    int y;
    int z;

    if (car < 8) {
        if (g_carExhaustPoints[car][1] != 0)
            g_exhaustParticleSide = (g_exhaustParticleSide + 1) % 2;
        else
            g_exhaustParticleSide = 0;
        pCar = Car_Get(car);
        FixMatrix_RotateVector(&g_carExhaustWorldPositions[car], &g_carExhaustPoints[car][g_exhaustParticleSide]->pos, pCar->pBodyMatrix);
        pM = pCar->pBodyMatrix;
        x = p->vector0x28.x + pM->position.x;
        p->vector0x28.x = x;
        y = p->vector0x28.y + pM->position.y;
        z = p->vector0x28.z + pM->position.z;
        p->vector0x28.y = y;
        p->vector0x28.z = z;
        p->vector0x28.x = g_carExhaustWorldPositions[car].x + x;
        p->vector0x28.y = g_carExhaustWorldPositions[car].y + y;
        p->vector0x28.z = g_carExhaustWorldPositions[car].z + z;
    }
}

// Gives a particle a deterministic spiral offset and, while its owner is a
// car slot, moves it into that car's world position.
// FUNCTION: CMR2 0x0045d3a0
void CarEffects_UpdateSpiralParticle(Particle *pParticle, ParticleType *pType, SceneNode *pView)
{
    int scales[10] = { 0x1999, 0x3333, 0x4ccc, 0x6666, 0x8000,
                       0x9999, 0xb333, 0xcccc, 0xe666, 0x10000 };
    Particle *p;
    unsigned short angle;
    int index;
    int x;
    int z;
    int car;

    p = pParticle;
    index = ((int)(INT_PTR)p / 3 & 0xffffff) % 10;
    if (p->age == 0xc70000)
        p->field0x50 = (short)(((int)(INT_PTR)p / 7 & 0xffffff) % 0x24) * 0x71;
    p->field0x50 += 0x71;
    if (p->field0x50 > 0x1000)
        p->field0x50 -= 0x1000;
    if (p->field0x50 < 0)
        p->field0x50 += 0x1000;
    if ((pType->flags & 4) != 0)
        angle = (unsigned short)p->field0x50;
    else
        angle = (unsigned short)(0x1000 - p->field0x50);
    x = FixMul(scales[index], g_sinTable[(angle + 0x400) & 0xfff]);
    z = FixMul(scales[index], g_sinTable[angle & 0xfff]);
    car = p->field0x64;
    if (car < 8) {
        p->vector0x28.x += x;
        p->vector0x28.z += z;
        p->vector0x28.x += Car_Get(car)->position.x;
        p->vector0x28.y += Car_Get(car)->position.y;
        p->vector0x28.z += Car_Get(car)->position.z;
    }
}

// Spawn callback: stores the effect data passed by the spawner.
// FUNCTION: CMR2 0x0045de80
void CarEffects_InitParticleOwnerData(Particle *pParticle, ParticleType *pType, void *param)
{
    int value = *(int *)param;

    pParticle->field0x64 = value;
}

// Splashes and sparks thrown along the wheel trails of a player's car:
// water on the wet surfaces, sparks on some hard ones (when the stage asks
// for them), each particle placed at a random point of the trail between
// the two wheels of the axle.
// match 36%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0045c820
void WheelSplash_Update(int player)
{
    Car *pCar;
    FixVector *pVel;
    int *pOnGround;
    short *pSurface;
    FixVector seg;
    FixVector off;
    FixVector p;
    FixVector q;
    FixVector rel;
    FixVector vel;
    FixVector fwd;
    BYTE colour[4];
    int wheel;
    int other;
    int front;
    int leading;
    int wet;
    int sparks;
    int emitting;
    int on;
    int type;
    int count;
    int speed;
    int a;
    int k;
    int f48;
    FixVector *pPos;
    int surface;

    colour[0] = 0xff;
    colour[1] = 0xff;
    colour[2] = 0xff;
    colour[3] = 0xff;
    if (player >= 8)
        return;
    pCar = Car_Get(player);
    emitting = 0;
    pVel = pCar->cornerVelocity;
    pOnGround = pCar->cornerOnGround;
    pSurface = pCar->wheelSurface;
    wheel = 0;
    do {
        on = 0;
        type = 0;
        if (wheel == 0 || wheel == 2)
            front = 1;
        else
            front = 0;
        seg.x = g_trailPos[player][wheel].x - g_trailLastPos[player][wheel].x;
        seg.y = g_trailPos[player][wheel].y - g_trailLastPos[player][wheel].y;
        other = wheel ^ 1;
        seg.z = g_trailPos[player][wheel].z - g_trailLastPos[player][wheel].z;
        if ((pCar->gear == 7 && (wheel & 2) != 0) || (pCar->gear != 7 && (wheel & 2) == 0))
            leading = 1;
        else
            leading = 0;
        surface = *pSurface;
        sparks = surface == 0x19 || surface == 0x18 || surface == 0x2a;
        wet = surface == 0xe || surface == 0xf;
        if (StageObject_GetViewWeatherStateByte(player) == 1) {
            k = FixMul(StageObject_GetViewWeatherField54(player), 0xff0000) >> 16;
            if (k > 0x1e && sparks) {
                colour[3] = (BYTE)k;
                type = 0xc;
                emitting = 0;
            }
        }
        // OpenCMR2: video.splash_amount (0..300, default 200 - deliberately
        // overdone, tone down in the tuning panel) repeats the splash spawn
        // per wheel-step; each repeat re-randomizes position below.
        count = ParticleAmount_PuffCount(Sys_GetOption("video.splash_amount", 200));
        if (wet) {
            emitting = 1;
            colour[3] = 0xff;
            FixMatrix_GetForward(&fwd, &g_viewStates[player].matrix);
            if (FixVecDot(&pCar->right, &fwd) < 0)
                type = front == 0 ? 0xe : 0xf;
            else
                type = front != 0 ? 0xe : 0xf;
            if (EFFECT_RAND() < 0x8000) {
                if (!leading) {
                    type = (EFFECT_RAND() >= 0x8000) + 0xe;
                } else {
                    count = 0;
                }
            }
            on = 1;
        } else if (emitting) {
            on = 1;
        }
        if (*pOnGround == 0)
            on = 0;
        if (Car_GetWheelSpeed(pCar, 0, 0) < 0)
            speed = -Car_GetWheelSpeed(pCar, 0, 0);
        else
            speed = Car_GetWheelSpeed(pCar, 0, 0);
        if (speed < 0x1e0000 && FixDiv(speed, 0x1e0000) < EFFECT_RAND())
            on = 0;
        if ((player <= 0 || EFFECT_RAND() <= 0x8000) && on && count > 0) {
            do {
                FixVecScale(&off, &seg, EFFECT_RAND_NEG());
                // A random point between this wheel's trail and the other
                // wheel of the axle (a weights the other wheel).
                if (!leading && wet) {
                    if (type != 0xe && type != 0xf) {
                        a = EFFECT_RAND();
                        k = EFFECT_RAND_NEG();
                        a = FixDiv(a, 0x1547a - k * 9);
                        FixVecScale(&p, &g_trailPos[player][other], a);
                        FixVecScale(&q, &g_trailPos[player][wheel], 0x10000 - a);
                    } else {
                        a = EFFECT_RAND();
                        k = EFFECT_RAND_NEG();
                        a = FixDiv(a, 0x1547a - k * 9);
                        a = FixDiv(a, 0x30000 - EFFECT_RAND_NEG() * 14);
                        FixVecScale(&p, &g_trailPos[player][other], a);
                        FixVecScale(&q, &g_trailPos[player][wheel], 0x10000 - a);
                    }
                    p.x += q.x;
                    p.y += q.y;
                    p.z += q.z;
                } else {
                    // The original generates the numerator first. Two rand()
                    // calls in FixDiv's arguments run in the opposite order
                    // under MSVC6 and change the particle's trail position.
                    a = EFFECT_RAND();
                    k = EFFECT_RAND_NEG();
                    a = FixDiv(a, 0x1547a - k * 9);
                    a = FixDiv(a, 0x30000 - EFFECT_RAND_NEG() * 14);
                    FixVecScale(&p, &g_trailPos[player][other], a);
                    FixVecScale(&q, &g_trailPos[player][wheel], 0x10000 - a);
                    p.x = p.x + q.x + off.x;
                    p.y = p.y + q.y + off.y;
                    p.z = p.z + q.z + off.z;
                }
                rel.x = p.x - pCar->position.x;
                rel.y = p.y - pCar->position.y;
                rel.z = p.z - pCar->position.z;
                vel.y = pVel->y / 10;
                if (type == 0xe || type == 0xf) {
                    vel.x = pVel->x / -5;
                    vel.z = pVel->z / -5;
                } else if (wheel == 2 || wheel == 3) {
                    vel.x = pVel->x / -3;
                    vel.z = pVel->z / -3;
                } else {
                    vel.x = pVel->x * -2 / 3;
                    vel.z = pVel->z * -2 / 3;
                }
                speed = Car_GetWheelSpeed(pCar, 2, 0);
                if (speed > 0x1e0000) {
                    k = FixDiv(0x1e0000, speed);
                    vel.x = FixMul(vel.x, k);
                    vel.y = FixMul(vel.y, k);
                    vel.z = FixMul(vel.z, k);
                }
                if (type == 0xd) {
                    vel.x = pVel->x / 3;
                    vel.y = pVel->y / 3;
                    vel.z = pVel->z / 3;
                    pPos = &p;
                    f48 = p.y - 0x630000;
                } else {
                    pPos = &rel;
                    f48 = rel.y - 0x10000;
                }
                Particle_Spawn(type, pPos, &vel, f48, 0, colour, g_trailLevel[player][wheel], &player,
                               pCar->pBodyNode->viewMask);
            } while (--count != 0);
            count = 0;
        }
        wheel++;
        pSurface++;
        pOnGround++;
        pVel++;
    } while (wheel < 4);
}

// Spawn callback of a debris piece or glass fragment: a random shape and
// the colour passed by the emitter ({light level, colour index << 16}).
// FUNCTION: CMR2 0x00499a80
void Debris_Init(Particle *p, ParticleType *pType, void *data)
{
    int *pParam = (int *)data;
    int shape;

    shape = (rand() % 30) << 8;
    p->field0x64 = (short)shape;
    p->field0x64 = (short)((pParam[1] >> 16) + shape);
    p->size = pParam[0];
}

// Random earth colours of the plain wheel dust.
// GLOBAL: CMR2 0x0051abf8
BYTE g_wheelDustColours[10][4] = {
    { 0x88, 0x10, 0x0a, 0x00 }, { 0x87, 0x12, 0x0c, 0x00 }, { 0x78, 0x0e, 0x0a, 0x00 }, { 0x56, 0x0d, 0x0a, 0x00 },
    { 0x56, 0x2c, 0x14, 0x00 }, { 0x49, 0x21, 0x0c, 0x00 }, { 0x35, 0x17, 0x07, 0x00 }, { 0x95, 0x32, 0x09, 0x00 },
    { 0xb3, 0x41, 0x13, 0x00 }, { 0xcb, 0x61, 0x26, 0x00 },
};

// Emits one wheel dust/gravel particle: the particle type depends on the
// kind (plain, dust, debris) and the wheel side, its velocity follows the
// wheel with a random lift, its colour is random earth or the given colour.
// FUNCTION: CMR2 0x0045dc00
void CarEffects_EmitWheelDustParticle(int car, int wheel, FixVector *pPos, FixVector *pVel, int dryWheelDust, int side, FixVector *pA,
                  FixVector *pB, short *pC, int surfaced, int light, int debris, BYTE *pColour)
{
    FixVector velocity;
    int along;

    if (car >= 8)
        return;
    if (surfaced != 0) {
        if (debris != 0) {
            if (wheel == 0 || wheel == 2)
                light = 0x16;
            else
                light = 0x17;
        } else {
            if (light != 0) {
                if (wheel == 0 || wheel == 2)
                    light = 0x14;
                else
                    light = 0x15;
            } else {
                if (wheel == 0 || wheel == 2)
                    light = 0x12;
                else
                    light = 0x13;
            }
        }
    } else {
        if (wheel == 0 || wheel == 2)
            light = 0x10;
        else
            light = 0x11;
    }
    FixAtan2(pB->x, pB->z);
    along = FixMul(pVel->x, pB->x) + FixMul(pVel->y, pB->y) + FixMul(pVel->z, pB->z);
    if (along < 0)
        along = -along;
    if (along > 0x4ccc)
        along = 0x4ccc;
    FixVecScale(&velocity, pB, along);
    velocity = *pVel;
    velocity.y = rand() % 0x2666;
    if (surfaced == 0) {
        along = rand() % 10;
        ((BYTE *)&debris)[0] = g_wheelDustColours[along][0];
        ((BYTE *)&debris)[1] = g_wheelDustColours[along][1];
        ((BYTE *)&debris)[2] = g_wheelDustColours[along][2];
    } else {
        ((BYTE *)&debris)[0] = pColour[0];
        ((BYTE *)&debris)[1] = pColour[1];
        ((BYTE *)&debris)[2] = pColour[2];
    }
    ((BYTE *)&debris)[3] = 0xff;
    Particle_SpawnTagged(light, pPos, &velocity, pPos->y, 0, (BYTE *)&debris, g_trailLevel[car][wheel], &car,
                   Car_Get(car)->pBodyNode->viewMask, dryWheelDust != 0);
}

// Wheel dust, gravel and water of a car: the surface under each wheel and the
// stage's country pick the colours and kinds; the faster the wheel spins, the
// more particles.
// FUNCTION: CMR2 0x0045d540
void CarEffect_UpdateWheelSurfaceParticles(int car, bool emitParticles)
{
    Car *pCar;
    BYTE colour[4]={0x6b,0x5c,0x42,0xff};
    int wheel;
    int surface;
    int water;
    int dirt;
    int gravel;
    int snow;
    int ice;
    int emit;
    int dark;
    int light;
    int count;
    int spin;
    int side;
    int n;
    int shade;
    int k;
    FixVector position;
    FixVector velocity;
    int velY;
    int velX;
    int velZ;

    count = 0;
    if (car < 0 || car >= 8)
        return;
    pCar = Car_Get(car);
    // OpenCMR2: live wheel chips (types 0x10..0x17) for the flood guard below.
    extern Particle *g_particles;
    extern ParticleType *g_particleTypes;
    extern int g_particleCount;
    int liveChips = 0;
    if (g_particles && g_particleTypes)
        for (int pi = 0; pi < g_particleCount; pi++)
            if (g_particles[pi].active && g_particles[pi].pType >= &g_particleTypes[0x10] &&
                g_particles[pi].pType <= &g_particleTypes[0x17])
                liveChips++;
    // One phase per effects update, not wall-clock: catch-up simulation calls
    // must not all land on the same GetFrameTime()/20 phase.
    static unsigned cloudPhase[8] = {};
    unsigned phase = ++cloudPhase[car];
    for (wheel = 0; wheel < 4; wheel++) {
        snow = 0;
        surface = pCar->wheelSurfaceType[wheel];
        if (surface == 0x19 || surface == 0x2e || surface == 0x2f || surface == 0x30 || surface == 0x31 ||
            surface == 0x32 || surface == 0x4f)
            water = 1;
        else
            water = 0;
        if (surface == 0x15 || surface == 0x1d || surface == 0x20 || surface == 0x21 || surface == 0x22 ||
            surface == 0x23 || surface == 0x24 || surface == 0x25 || surface == 0x27 || surface == 0x33 ||
            surface == 0x36 || surface == 0x47 || surface == 0x4f || surface == 0x19)
            dirt = 1;
        else
            dirt = 0;
        if (surface == 0x16 || surface == 0x26 || surface == 0x28 || surface == 0x2c || surface == 0x34 ||
            surface == 0x35 || surface == 0x37 || surface == 0x38)
            gravel = 1;
        else
            gravel = 0;
        if (surface == 0x17 || surface == 0x29)
            snow = 1;
        ice = surface == 0x18;
        if (dirt == 0 && gravel == 0 && snow == 0 && ice == 0) {
            emit = ice;
        } else {
            emit = 1;
            switch ((BYTE)RallyDataCountryIndex()) {
            case 0:
                if (dirt) {
                    light = 0;
                    dark = 1;
                    colour[0] = 0x3d;
                    colour[1] = 0x40;
                    colour[2] = 0x0e;
                } else if (gravel) {
                    light = 1;
                    dark = 0;
                    colour[0] = 0x66;
                    colour[1] = 0x63;
                    colour[2] = 0x4a;
                }
                break;
            case 2:
                dark = 1;
                light = 0;
                colour[0] = 0x50;
                colour[1] = 0x5b;
                colour[2] = 0x42;
                break;
            case 4:
                if (dirt) {
                    light = 0;
                    dark = 1;
                    colour[0] = 0x20;
                    colour[1] = 0x34;
                    colour[2] = 3;
                } else if (gravel) {
                    light = 1;
                    dark = 0;
                    colour[0] = 0x9a;
                    colour[1] = 0x8e;
                    colour[2] = 0x20;
                }
                break;
            case 5:
                if (dirt) {
                    light = 0;
                    dark = 1;
                    colour[0] = 0x4f;
                    colour[1] = 0x49;
                    colour[2] = 0x33;
                } else if (gravel) {
                    light = 1;
                    dark = 0;
                    colour[0] = 0x50;
                    colour[1] = 0x40;
                    colour[2] = 0x1f;
                }
                break;
            case 6:
                if (dirt) {
                    light = 0;
                    dark = 1;
                    colour[0] = 0x49;
                    colour[1] = 0x58;
                    colour[2] = 0x3a;
                } else if (gravel) {
                    light = 1;
                    dark = 0;
                    colour[0] = 0x2f;
                    colour[1] = 0x3b;
                    colour[2] = 0x21;
                } else if (snow) {
                    light = 0;
                    dark = 1;
                    colour[0] = 0x49;
                    colour[1] = 0x58;
                    colour[2] = 0x3a;
                }
                break;
            case 7:
                if (dirt) {
                    light = 0;
                    dark = 1;
                    colour[0] = 0x1f;
                    colour[1] = 0x29;
                    colour[2] = 1;
                } else if (gravel) {
                    light = 1;
                    dark = 0;
                    colour[0] = 0x37;
                    colour[1] = 0x46;
                    colour[2] = 0x28;
                } else if (snow) {
                    light = 1;
                    dark = 0;
                    colour[0] = 0x31;
                    colour[1] = 0x3f;
                    colour[2] = 0x23;
                } else if (ice) {
                    light = 0;
                    dark = 1;
                    colour[0] = 0x41;
                    colour[1] = 0x51;
                    colour[2] = 0x31;
                }
                break;
            case 1:
            case 8:
                light = 1;
                dark = 0;
                if (dirt) {
                    colour[0] = 0x35;
                    colour[1] = 0x39;
                    colour[2] = 0x2d;
                } else if (gravel) {
                    colour[0] = 0x60;
                    colour[1] = 0x4f;
                    colour[2] = 0x40;
                }
                break;
            default:
                light = 0;
                dark = 1;
                colour[0] = 0xf0;
                colour[1] = 0xf0;
                colour[2] = 0xf0;
                break;
            }
        }
        if ((water == 0 && emit == 0) || pCar->cornerOnGround[wheel] == 0)
            continue;
        // Each grounded wheel supplies its own speed. The old rear-left proxy
        // hid front-only spin, leaked the count across wheels and rand()%4/4
        // could never emit a particle in the lower speed band.
        count = emitParticles ? WheelParticle_BoostedCount(Car_GetWheelSpeed(pCar, (BYTE)wheel, 0),
            pCar->cornerOnGround[wheel] != 0, water != 0 || emit != 0, (unsigned)rand(), Sys_GetOption("video.tire_debris", 800)) : 0;
        // Physical wheel/surface exposure, independent of billboard budget,
        // camera visibility and the airborne-particle toggle. Fixed effects
        // stepping supplies time, rather than rendered-frame dirt growth.
        int dustSpeed=Car_GetWheelSpeed(pCar,(BYTE)wheel,0);
        if((dustSpeed>0xa0000||dustSpeed< -0xa0000) && DustPlume_Surface(dirt,gravel,water,snow,ice) &&
           Sys_GetOption("video.body_dirt",1)!=0){
            float red=colour[0]/255.f,green=colour[1]/255.f,blue=colour[2]/255.f;
            DustPlume_SoilTint(red,green,blue);
            float exposure=std::min(std::abs(dustSpeed/65536.f)/60.f,2.f)*.00012f;
            CarBodyDirt::Deposit(car,exposure,red,green,blue);
        }
        if(!emitParticles)continue;
        // Default-on dry dust, separate from chips/water/snow. Explicit0 opts out. Existing per-wheel
        // contact/speed gating above supplies emission opportunity. Use a
        // stable per-wheel cadence, no extra global RNG/physics state changes.
        // OpenCMR2: video.dust_plume_amount (0..300, default 200 - deliberately
        // heavy per user request) scales the puff cadence; the live cloud cap
        // scales inside DustPlume_Emit. 0 keeps the original every-3rd cadence.
        int plumeAmount = Sys_GetOption("video.dust_plume_amount", 200);
        if (plumeAmount < 0) plumeAmount = 0;
        if (plumeAmount > 300) plumeAmount = 300;
        unsigned plumePeriod = plumeAmount >= 200 ? 1 : plumeAmount >= 100 ? 2 : 3;
        if (count > 0 && DustPlume_Surface(dirt, gravel, water, snow, ice) &&
            Sys_GetOption("video.dust_plume", 1) != 0 &&
            ((phase + (unsigned)wheel + (unsigned)car) % plumePeriod == 0)) {
            FixVector cloudWorld = g_trailPos[car][wheel];
            FixVector cloudVelocity = {-(pCar->cornerVelocity[wheel].x/4), 0x800,
                                       -(pCar->cornerVelocity[wheel].z/4)};
            DustPlume_Emit(&cloudWorld, &cloudVelocity, colour, g_trailLevel[car][wheel],
                          (BYTE)pCar->pBodyNode->viewMask);
        }
        FixVecLength(&pCar->cornerVelocity[wheel]);
        velY = pCar->cornerVelocity[wheel].y;
        velX = -(pCar->cornerVelocity[wheel].x / 4);
        velZ = -(pCar->cornerVelocity[wheel].z / 4);
        if (wheel == 0 || wheel == 2)
            side = -0x10000;
        else
            side = 0x10000;
        if (wheel != 3 && wheel != 2)
            side = FixMul(side, 0x3333);
        const bool dryWheelDust = DustPlume_Surface(dirt, gravel, water, snow, ice) &&
            Sys_GetOption("video.dust_plume", 1) != 0;
        for (k = count; k > 0; k--) {
            // OpenCMR2: chip flood guard - never let the dirt wall starve the
            // plume reserve, weather, skid smoke or other original effects.
            if (!WheelParticle_ChipBudgetOK(liveChips, g_particleCount))
                break;
            liveChips++;
            position.x = g_trailPos[car][wheel].x - pCar->position.x;
            position.y = g_trailPos[car][wheel].y - pCar->position.y;
            position.z = g_trailPos[car][wheel].z - pCar->position.z;
            velocity.x = velX;
            velocity.y = velY;
            velocity.z = velZ;
            if (water != 0) {
                CarEffects_EmitWheelDustParticle(car, wheel, &position, &velocity, 0, side, &pCar->groundAxis[1],
                             &pCar->groundDir[1], &pCar->cornerTriangle[wheel], 0, 0, 0, colour);
                continue;
            }
            if (StageObject_GetViewWeatherStateByte(car) == 1)
                shade = FixMul(StageObject_GetViewWeatherField54(car), 0xff0000) >> 16;
            else
                shade = 0;
            n = (0x100 - shade) / 128 + 1;
            if (dark != 0 && n > 0) {
                for (spin = n; spin != 0; spin--)
                    CarEffects_EmitWheelDustParticle(car, wheel, &position, &velocity, dryWheelDust, side, &pCar->groundAxis[1],
                                 &pCar->groundDir[1], &pCar->cornerTriangle[wheel], 1, 0, 0, colour);
            }
            if (light != 0 && n > 0) {
                for (; n != 0; n--)
                    CarEffects_EmitWheelDustParticle(car, wheel, &position, &velocity, dryWheelDust, side, &pCar->groundAxis[1],
                                 &pCar->groundDir[1], &pCar->cornerTriangle[wheel], 1, 1, 0, colour);
            }
        }
    }
}

// Optional speed cue: sparse world-fixed specks beside the road ahead of the player car (see DustPlume.h).
// Spawn only; the car, camera and physics are never read back from the particles. Own LCG, not rand().
bool Track_CameraGroundHeight(FixVector *point, int *height);
void SpeedMotes_Step(int step16_16)
{
    if (step16_16 <= 0 || Sys_GetOption("video.speed_motes", 0) == 0) return;
    Car *car = Car_Get(0);
    if (!car) return;
    float ratio = car->speed / 65536.f / 1.5f; // 1.0 is the speed the tunnel-vision ramp calls full
    if (ratio < 0.4f) return;
    if (ratio > 1.f) ratio = 1.f;
    SpeedMote_SetRatio(ratio);
    int amount = Sys_GetOption("video.speed_motes_amount", 100);
    if (amount < 0) amount = 0;
    if (amount > 300) amount = 300;
    static float carry = 0.f;
    static unsigned seed = 0x7e3a91c5u;
    auto unit = [&]() { seed = seed * 1664525u + 1013904223u; return (seed >> 8) / 16777216.f; };
    carry += (ratio - 0.4f) / 0.6f * 0.7f * (amount / 100.f) * (step16_16 / 65536.f);
    // The driving direction is Car::right in this code base (see SpeedMote_SpawnOffset); Car::forward is the side axis. Using forward
    // put every mote 27 to 54 world units to the SIDE of the road, outside the picture.
    float hx = car->right.x / 65536.f, hz = car->right.z / 65536.f;
    if (!(hx * hx + hz * hz > 0.04f)) { carry = 0.f; return; }
    int guard = 4;
    while (carry >= 1.f && guard-- > 0) {
        carry -= 1.f;
        // Place the mote so the car reaches it in 18 to 36 steps (0.7 to 1.4 s): distance = speed per step * steps, which is
        // independent of the world-unit scale. Life (48 steps) always outlasts the approach, so it passes the camera.
        float u1 = unit(), u2 = unit(), u3 = unit();
        float ox = 0.f, oz = 0.f;
        if (!SpeedMote_SpawnOffset(hx, hz, car->speed / 65536.f, u1, u2, u3, &ox, &oz)) break;   // ahead 27-54 units, 4-14 to the side, out of the central sightline
        float x = car->position.x / 65536.f + ox;
        float z = car->position.z / 65536.f + oz;
        FixVector p = { (int)std::lround(x * 65536.f), car->position.y, (int)std::lround(z * 65536.f) };
        int ground;
        if (Track_CameraGroundHeight(&p, &ground)) p.y = ground; else SpeedMote_NoteSkippedGround();
        p.y += (int)((0.3f + 3.2f * unit()) * 65536.f);
        SpeedMote_Spawn(&p);
    }
    if (carry > 4.f) carry = 4.f;
    SpeedMote_LogStats(ratio, step16_16);
}

// Optional dynamic fog (video.fog_spatial / video.fog_evolve): scales video.fog_amount by
// the viewed car's route progress; see FogDynamics.h. Render-only, same per-step cadence
// as the motes. Reversible prototype: turn both options back off and the static amount
// is restored on the next step.
int g_fogDynamicsEffective = 0;
int RallyData_GetActiveCarRaceRecordField0(Car *p);   // Car.cpp
void FogDynamics_Step(int step16_16)
{
    if (step16_16 <= 0 || Sys_GetOption("video.fog", 0) == 0) return;
    int spatial = Sys_GetOption("video.fog_spatial", 0);
    int evolve = Sys_GetOption("video.fog_evolve", 100);
    int base = Sys_GetOption("video.fog_amount", 65);
    if (base < 0) base = 0; if (base > 200) base = 200;
    int glow = Sys_GetOption("video.fog_glow", 60);
    if (glow < 0) glow = 0; if (glow > 100) glow = 100;
    if (!spatial && evolve == 100) {
        if (g_fogDynamicsEffective) {   // dynamics turned off mid-race: restore the static amount once
            g_fogDynamicsEffective = 0;
            Gfx_SetPostFog(1, base, glow);
        }
        return;
    }
    Car *car = Car_Get(0);
    if (!car || g_routeNodeCount < 2) return;
    int node = RallyData_GetActiveCarRaceRecordField0(car);
    float f = node / (float)(g_routeNodeCount - 1);
    if (f < 0.f) f = 0.f; if (f > 1.f) f = 1.f;
    float phase = (RallyDataStageIndex() & 7) / 8.f;
    float target = FogDynamics_Mult(f, phase, spatial, evolve);
    static float eased = 1.f;
    if (node == 0) eased = target;          // stage start/restart: no cross-stage carry-over
    eased += (target - eased) / 32.f;       // ~1.3 s at 25 Hz: hides route-node stepping
    int eff = (int)std::lround(base * eased);
    if (eff < 0) eff = 0; if (eff > 200) eff = 200;
    Gfx_SetPostFog(1, eff, glow);
    g_fogDynamicsEffective = eff;
}

// Optional snow glints (video.snow_glint): the snow surface responds to light.
// Per step, sample ground points ahead of the viewed car; where the collision
// mesh reports a snow/ice surface (the same raw ids the wheel particles use),
// compute the physical glitter response (sun/view half-vector against the true
// ground normal with a deterministic per-position facet jitter, SnowGlint.h)
// plus a cone boost from any car light beams (the fog light field). Spawns a
// short-lived static glint where the response is visible. Own LCG, never rand(),
// render-only.
void SnowGlint_Step(int step16_16)
{
    if (step16_16 <= 0 || Sys_GetOption("video.snow_glint", 200) == 0) return;
    Car *car = Car_Get(0);
    if (!car) return;
    int amount = Sys_GetOption("video.snow_glint", 200);
    FixVector sunV;
    StageObject_GetRotatedStageLightVector(&sunV);
    float sunLen = std::sqrt((float)sunV.x * sunV.x + (float)sunV.y * sunV.y + (float)sunV.z * sunV.z);
    if (sunLen < 1e-3f) return;
    float sunX = sunV.x / sunLen, sunY = sunV.y / sunLen, sunZ = sunV.z / sunLen;
    int weatherLight = StageObject_GetWeatherLightIntensity();
    float sunLevel = weatherLight <= 0 ? 0.f : weatherLight >= 0x10000 ? 1.f : weatherLight / 65536.f;
    // The view position: the active view node when available, else the car.
    FixVector eye = car->position;
    if (g_viewNodes[0]) FixMatrix_GetPosition(&eye, &g_viewNodes[0]->world);
    // Car forward (body axes), flattened: samples bias ahead of the car.
    FixVector fwd = car->forward;
    fwd.y = 0;
    float fwdLen = std::sqrt((float)fwd.x * fwd.x + (float)fwd.z * fwd.z);
    if (fwdLen < 1e-3f) { fwd.x = 0; fwd.z = 0x10000; fwdLen = 65536.f; }
    float fx = fwd.x / fwdLen, fz = fwd.z / fwdLen;
    static unsigned seed = 0x51a7c3d9u;
    for (int i = 0; i < 6; ++i) {
        seed = seed * 1664525u + 1013904223u; float u1 = (seed >> 8) / 16777216.f;
        seed = seed * 1664525u + 1013904223u; float u2 = (seed >> 8) / 16777216.f;
        // Annulus 4..40 world units, angle biased +-75 degrees around the car's forward.
        float ang = (u1 * 2.f - 1.f) * 1.309f;
        float dist = 4.f + u2 * u2 * 36.f;
        float ca = std::cos(ang), sa = std::sin(ang);
        float dx = (fx * ca - fz * sa) * dist, dz = (fz * ca + fx * sa) * dist;
        FixVector p;
        p.x = car->position.x + (int)(dx * 65536.f);
        p.y = car->position.y;
        p.z = car->position.z + (int)(dz * 65536.f);
        FixVector normal = {0, 0x10000, 0};
        short tri = -1, surfaceClass = 0;
        unsigned short surface = 0;
        p.y = Track_GetGroundHeightSurface(&p, &normal, &tri, &surfaceClass, &surface, p.y);
        if (tri < 0) continue;
        if (surface != 0x17 && surface != 0x29 && surface != 0x18) continue;   // snow/snow-packed/ice
        float wpx = p.x / 65536.f, wpy = p.y / 65536.f, wpz = p.z / 65536.f;
        float toEyeX = eye.x / 65536.f - wpx, toEyeY = eye.y / 65536.f - wpy, toEyeZ = eye.z / 65536.f - wpz;
        float el = std::sqrt(toEyeX*toEyeX + toEyeY*toEyeY + toEyeZ*toEyeZ);
        if (el < 1e-3f) continue;
        float glint = SnowGlint_Specular(normal.x / 65536.f, normal.y / 65536.f, normal.z / 65536.f,
                                         sunX, sunY, sunZ, toEyeX / el, toEyeY / el, toEyeZ / el,
                                         SnowGlint_FacetSeed(p.x, p.z)) * sunLevel;
        // Car light beams boost the surface inside them (night and dark weather).
        float boost = 0.f;
        for (int li = 0; li < g_fogLightSourceCount && li < LIGHT_FIELD_MAX; ++li) {
            FogLightSource *L = &g_fogLightSources[li];
            boost += SnowGlint_ConeBoost(wpx, wpy, wpz,
                                         L->pos.x / 65536.f, L->pos.y / 65536.f, L->pos.z / 65536.f,
                                         L->dir.x / 65536.f, L->dir.y / 65536.f, L->dir.z / 65536.f,
                                         L->intensity / 65536.f);
        }
        float response = glint + boost * 0.8f;
        if (response > 1.f) response = 1.f;
        int peak = (int)(response * SNOW_GLINT_ALPHA_MAX * (amount / 100.f));
        if (peak < 28) continue;   // below visible sparkle
        SnowGlint_Spawn(&p, peak);
    }
}

// Optional floating dust motes (video.air_motes): small bug-like specks flying on their
// own around the viewed car, visible at any speed (see AirMote.h). The shared drift is
// wind/16 plus most of the viewed car's velocity, so the swarm travels along instead of
// the car driving into stationary spots. Spawn only; own LCG, never rand(), and the
// particles never feed back into the car, the camera or physics.
void AirMotes_Step(int step16_16)
{
    if (step16_16 <= 0 || Sys_GetOption("video.air_motes", 0) == 0) return;
    Car *car = Car_Get(0);
    if (!car) return;
    int amount = Sys_GetOption("video.air_motes_amount", 200);
    // Sun forward scatter for the mote light response: strongest looking toward
    // the sun in clear air, gone under overcast (stage weather light intensity).
    {   FixVector sunV;
        StageObject_GetRotatedStageLightVector(&sunV);
        float sl = std::sqrt((float)sunV.x * sunV.x + (float)sunV.y * sunV.y + (float)sunV.z * sunV.z);
        float scatter = 0.f;
        int wl = StageObject_GetWeatherLightIntensity();
        if (wl > 0 && sl > 1e-3f && g_viewNodes[0]) {
            FixVector vf;
            FixMatrix_GetForward(&vf, &g_viewNodes[0]->world);
            float d = (vf.x * sunV.x + vf.y * sunV.y + vf.z * sunV.z) / (65536.f * sl);
            if (d > 0.f) {
                float s8 = d;
                for (int fsq = 0; fsq < 3; ++fsq) s8 *= s8;   // d^8 forward lobe
                scatter = s8 * (wl >= 0x10000 ? 1.f : wl / 65536.f);
            }
        }
        g_airMoteSunScatter = scatter;
    }
    g_airMoteDrift.x = g_particleWind.x / 16 + (int)(car->velocity.x * 0.65f);
    g_airMoteDrift.y = g_particleWind.y / 16 + (int)(car->velocity.y * 0.65f);
    g_airMoteDrift.z = g_particleWind.z / 16 + (int)(car->velocity.z * 0.65f);
    static float carry = 0.f;
    static unsigned seed = 0x53c9e1a7u;
    auto unit = [&]() { seed = seed * 1664525u + 1013904223u; return (seed >> 8) / 16777216.f; };
    carry += AirMote_SpawnRate(amount) * (step16_16 / 65536.f);
    int guard = 4;
    while (carry >= 1.f && guard-- > 0) {
        carry -= 1.f;
        float dx, dz, h;
        AirMote_SpawnOffset(unit(), unit(), unit(), &dx, &dz, &h);
        float x = car->position.x / 65536.f + dx;
        float z = car->position.z / 65536.f + dz;
        FixVector p = { (int)std::lround(x * 65536.f), car->position.y, (int)std::lround(z * 65536.f) };
        int ground;
        if (Track_CameraGroundHeight(&p, &ground)) p.y = ground;   // falls back to the car height
        p.y += (int)(h * 65536.f);
        AirMote_Spawn(&p);
    }
    if (carry > 4.f) carry = 4.f;
    AirMote_LogStats(step16_16);
}

// PORT: read-only views of the per-car window layout for the cockpit
// windshield glass wiring (WindshieldGlass.cpp); the table itself stays here.
const FixVector *CarEffects_CarWindowVertices(int car)
{
    if (car < 0 || car >= 8)
        return NULL;
    // OpenCMR2: the cockpit glass is resolved when the interior attaches, which
    // StageTiming_RestoreDeviceStageResources runs (View_SetupCameras) BEFORE
    // StageObjects_Init fills g_carWindowVerts (CarEffects_Init). The table was
    // therefore empty (first stage) or left over from the previous stage's CIN
    // buffer at attach, and the windshield feature latched off ("no glass
    // profile"). Read the CIN glass section directly when it is loaded: it is
    // the same field CarEffects_Init stores, CarInfo_GetSection is a pure
    // offset lookup, and the same attach already reads the interior profile
    // from this CIN. The table is only the fallback when the CIN is not there.
    Car *pCar = Car_Get(car);
    if (pCar != NULL && CarInfo_GetDirectory(pCar->index) != NULL) {
        CarGlassProfile *pProfile = (CarGlassProfile *)CarInfo_GetSection(pCar, CAR_INFO_GLASS);
        if (pProfile != NULL)
            return pProfile->windowVertices;
    }
    return g_carWindowVerts[car];
}

void CarEffects_GetWindowLayout(unsigned char vertex[6][4], int mirrorZ[6][4])
{
    for (int w = 0; w < 6; w++)
        for (int j = 0; j < 4; j++) {
            vertex[w][j] = g_carWindows[w].vertex[j];
            mirrorZ[w][j] = g_carWindows[w].mirrorZ[j];
        }
}
