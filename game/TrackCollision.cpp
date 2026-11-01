#include <math.h>
#include "CallbackState.h"
#include "FloatToInt.h"
#include <windows.h>
#include "FixedPoint.h"
#include "Car.h"
#include "Mesh.h"
#include "MeshVertex.h"
#include "CarParts.h"
#include "FinRecord.h"
#include "FileBuffer.h"
#include "port/disk.h"
#include <stdio.h>
#include "Frontend.h"
#include "GenericFileLoader.h"
#include "StageTiming.h"
#include "TrackCollisionData.h"
#include "GameInfo.h"
#include "ReplayStream.h"

// Ground queries against the stage collision mesh loaded by StageTiming_IndexSerializedStageTables:
// triangles (three vertex indices plus a surface byte, 8 bytes each), their
// vertices, and a five-level quadtree whose leaves list the triangles of a cell.

// Pointers into the stage collision block (see StageTiming_IndexSerializedStageTables).

short Surface_GetMappedIndex(short index);

// Scratch values of the point-in-triangle edge test.
// GLOBAL: CMR2 0x00591af4
int g_trackEdgeSide;
// GLOBAL: CMR2 0x00591b28
int g_trackEdgeDX;
// GLOBAL: CMR2 0x00591b2c
int g_trackEdgeDZ;
// GLOBAL: CMR2 0x00591c50
int g_trackEdgeNX;
// GLOBAL: CMR2 0x00591c54
int g_trackEdgeNZ;
// GLOBAL: CMR2 0x00591c58
FixVector g_trackTriangle[3];

// Copies the three vertices of a collision triangle.
// FUNCTION: CMR2 0x00490e00
int Track_GetTriangle(FixVector *pOut, short tri)
{
#define TRACK_VERTEX(n) g_trackVertices[g_trackTriangles[tri].v[n]]
    pOut[0].x = TRACK_VERTEX(0).x;
    pOut[0].y = TRACK_VERTEX(0).y;
    pOut[0].z = TRACK_VERTEX(0).z;
    pOut[1].x = TRACK_VERTEX(1).x;
    pOut[1].y = TRACK_VERTEX(1).y;
    pOut[1].z = TRACK_VERTEX(1).z;
    pOut[2].x = TRACK_VERTEX(2).x;
    pOut[2].y = TRACK_VERTEX(2).y;
    pOut[2].z = TRACK_VERTEX(2).z;
#undef TRACK_VERTEX
    return 1;
}

#define TRACK_EDGE_TEST(a, b)                                                                  \
    g_trackEdgeDX = pTri[b].x - pTri[a].x;                                                     \
    g_trackEdgeDZ = pTri[b].z - pTri[a].z;                                                     \
    g_trackEdgeNX = g_trackEdgeDZ;                                                             \
    g_trackEdgeNZ = -g_trackEdgeDX;                                                            \
    g_trackEdgeDX = pPoint->x - pTri[a].x;                                                     \
    g_trackEdgeDZ = pPoint->z - pTri[a].z;                                                     \
    g_trackEdgeSide = FixMul(g_trackEdgeDX, g_trackEdgeNX) + FixMul(g_trackEdgeDZ, g_trackEdgeNZ)

// Whether the point lies inside the triangle when seen from above (the X/Z
// bounding box first, then the three edges). pTri may be NULL to load it.
// match 56%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00490f20
int Track_PointInTriangle(FixVector *pPoint, short tri, FixVector *pTri)
{
    int v;

    if (pTri == NULL) {
        pTri = g_trackTriangle;
        if (!Track_GetTriangle(g_trackTriangle, tri))
            return 0;
    }
    v = pPoint->x;
    if ((v >= pTri[0].x || v >= pTri[1].x || v >= pTri[2].x) && (v <= pTri[0].x || v <= pTri[1].x || v <= pTri[2].x)) {
        v = pPoint->z;
        if ((v >= pTri[0].z || v >= pTri[1].z || v >= pTri[2].z) &&
            (v <= pTri[0].z || v <= pTri[1].z || v <= pTri[2].z)) {
            TRACK_EDGE_TEST(0, 1);
            if (g_trackEdgeSide >= 0) {
                TRACK_EDGE_TEST(1, 2);
                if (g_trackEdgeSide >= 0) {
                    TRACK_EDGE_TEST(2, 0);
                    if (g_trackEdgeSide >= 0)
                        return 1;
                }
            }
        }
    }
    return 0;
}

// Height of the point on the triangle's plane; pNormal receives the plane
// normal and pSurface the triangle's surface type.
// match 78%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// 8-byte record of the triangle table at g_trackTriangles.

// FUNCTION: CMR2 0x004910f0
int Track_GetHeight(FixVector *pPoint, short tri, int defaultY, FixVector *pNormal, unsigned short *pSurface)
{
    FixVector t[3];
    FixVector e1;
    FixVector e2;
    int len;

    if (!Track_GetTriangle(t, tri))
        return 0;
    *pSurface = g_trackTriangles[tri].surface;
    e1.x = t[1].x - t[0].x;
    e1.y = t[1].y - t[0].y;
    e1.z = t[1].z - t[0].z;
    e2.x = t[2].x - t[0].x;
    e2.y = t[2].y - t[0].y;
    e2.z = t[2].z - t[0].z;
    FIX_NORMALIZE_INTO(e1, e1);
    FIX_NORMALIZE_INTO(e2, e2);
    FixVecCross(pNormal, &e1, &e2);
    len = FixVecLength(pNormal);
    if (len == 0) {
        pNormal->x = 0;
        pNormal->y = 0;
        pNormal->z = 0;
    } else {
        FixVecScaleRecip(pNormal, pNormal, len);
    }
    len = FixVecDot(&t[0], pNormal);
    if (pNormal->y != 0)
        return FixDiv(len - FixMul(pPoint->x, pNormal->x) - FixMul(pPoint->z, pNormal->z), pNormal->y);
    return defaultY;
}

// Of the listed triangles under the point, the one whose centre height is
// closest to y.
// match 89%: remaining diff is codegen-only. The original's 4th parameter is a
// short (guard does `movsx eax, word ptr [esp+0x3c]; test eax,eax; jle` and the
// loop end rematerialises `movsx eax, word ptr [esp+0x4c]`); declaring it `int`
// reproduces the whole structure but loads 32-bit, while declaring it `short`
// makes MSVC6 hoist/spill the sign extension and wreck the frame layout. The
// t-sum and the p++/i++ order below are already tuned to the original.
// FUNCTION: CMR2 0x004916a0
int Track_FindNearestTriangle(FixVector *pPoint, short *pOut, int y, short count, short *pList)
{
    FixVector t[3];
    BOOL first;
    int best;

    int bestIndex;
    int i;
    int h;
    int d;
    short *p;

    best = 0;
    bestIndex = 0;
    i = 0;
    first = TRUE;
    if (i < count) {
        p = pList;
        do {
            if (Track_GetTriangle(t, *p) && Track_PointInTriangle(pPoint, *p, t)) {
                h = (t[0].y + t[1].y + t[2].y) / 3;
                d = y - h;
                if (d < 0)
                    d = h - y;
                if (first) {
                    best = d;
                    bestIndex = i;
                    first = FALSE;
                } else if (d < best) {
                    best = d;
                    bestIndex = i;
                }
            }
            i++;
            p++;
        } while (i < count);
        if (!first) {
            *pOut = pList[bestIndex];
            return 1;
        }
    }
    return 0;
}


// Finds the triangle under the point by walking down the quadtree from the
// top-level grid cell.
// match 39%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00491550
int Track_FindTriangle(FixVector *pPoint, short *pOut, int y)
{
    int dx;
    int dz;
    int mask;
    int shift;
    short row;
    short col;
    short level;
    int node;

    dx = pPoint->x - g_trackCollisionHeader->x;
    dz = pPoint->z - g_trackCollisionHeader->z;
    col = (short)(dx >> 24);
    level = 0;
    row = (short)(dz >> 24);
    mask = 0xffffff;
    node = *g_trackGridColumns * row + col;
    shift = 24;
    if (col < 0 || col >= *g_trackGridColumns || row < 0 || row >= *g_trackGridRows)
        return 0;
    while (g_trackCollisionTables.traversalLevels[level][node].triangleCount == -1) {
        if (level >= 5)
            return 0;
        dx &= mask;
        dz &= mask;
        mask >>= 2;
        shift -= 2;
        node = (short)(dx >> shift) + g_trackCollisionTables.traversalLevels[level][node].firstIndex + (short)(dz >> shift) * 4;
        level++;
    }
    return Track_FindNearestTriangle(pPoint, pOut, y, g_trackCollisionTables.traversalLevels[level][node].triangleCount,
                                     g_trackTriangleIndices + g_trackCollisionTables.traversalLevels[level][node].firstIndex);
}

// GLOBAL: CMR2 0x005918d8
BYTE g_unk0x005918d8;
// GLOBAL: CMR2 0x0072d250
int g_finCount;
// GLOBAL: CMR2 0x0072d254
FinRecord *g_finData;
// GLOBAL: CMR2 0x0051fc38
char g_strFinFormat[] = "%sfin.dat";

BYTE *GameMenu_GetChampionshipTransitionState(void);

// Loads the stage's fin.dat table (0x30-byte records).
// FUNCTION: CMR2 0x00490c30
void TrackCollision_LoadFinishRecords(void)
{
    DWORD size = 0;

    g_unk0x005918d8 = 0;
    sprintf(CFrontend::m_stringDest, g_strFinFormat, GameMenu_GetChampionshipTransitionState());
    BYTE *pFile = (BYTE *)CGenericFileLoader::FindFile((GenericFile *)StageTiming_GetStageFile3(),
                                                    CFrontend::m_stringDest, 0, &size, 0);
    g_finData = NULL;
    g_finCount = size / 0x30;
    // PORT: the records are linked in place through the 32-bit field at +0x28
    // (Sector_RebuildStageGrid), which cannot hold a pointer on 64-bit
    // targets: copy them to native FinRecords, freed with the file buffer.
    if (pFile != NULL) {
        FinRecord *pNative = (FinRecord *)CFileBuffer::AllocateLockedBuffer(g_finCount * sizeof(FinRecord) + 1);
        int i;

        for (i = 0; i < g_finCount; i++) {
            BYTE *pDisk = pFile + i * 0x30;

            memcpy(&pNative[i], pDisk, offsetof(FinRecord, sector));
            pNative[i].pNext = NULL;
            pNative[i].sector = (int)Disk_Offset(pDisk, 0x28);
            pNative[i].type = (char)pDisk[0x2c];
            pNative[i].flags = pDisk[0x2d];
            pNative[i].surfaces[0] = pDisk[0x2e];
            pNative[i].surfaces[1] = pDisk[0x2f];
        }
        Disk_AttachToBuffer(pFile, pNative);
        g_finData = pNative;
    }
}

// Ground height under the point, reusing the cached triangle *pTri when the
// point is still over it; defaultY when the point is off the mesh.
// FUNCTION: CMR2 0x00490c90
int Track_GetGroundHeight(FixVector *pPoint, FixVector *pNormal, short *pTri, unsigned short *pSurface, int defaultY)
{
    if (*pTri < 0)
        *pTri = -1;
    if (*pTri == -1) {
        if (Track_FindTriangle(pPoint, pTri, defaultY))
            return Track_GetHeight(pPoint, *pTri, defaultY, pNormal, pSurface);
        return defaultY;
    }
    if (Track_PointInTriangle(pPoint, *pTri, NULL))
        return Track_GetHeight(pPoint, *pTri, defaultY, pNormal, pSurface);
    if (Track_FindTriangle(pPoint, pTri, defaultY))
        return Track_GetHeight(pPoint, *pTri, defaultY, pNormal, pSurface);
    *pTri = -1;
    return defaultY;
}

// Same, also returning the surface class of the ground.
// FUNCTION: CMR2 0x004932b0
int Track_GetGroundHeightSurface(FixVector *pPoint, FixVector *pNormal, short *pTri, short *pSurfaceClass,
                                 unsigned short *pSurface, int defaultY)
{
    int height;

    height = Track_GetGroundHeight(pPoint, pNormal, pTri, pSurface, defaultY);
    *pSurfaceClass = Surface_GetMappedIndex(*pSurface);
    return height;
}

// GLOBAL: CMR2 0x0059226c
Car *g_pAutoGearCar;
// GLOBAL: CMR2 0x00592270
CarPartSet *g_autoGearPartSet;

// Requests the adjacent automatic gear while the driver is shifting.  On an
// up-shift, a little extra hysteresis is derived from the difference between
// the first and third body corners.
// FUNCTION: CMR2 0x00493890
void AutoGear_RequestAdjacentGear(void)
{
    char gear;
    int load;
    int threshold;

    if (g_pAutoGearCar->field_0x1d4[0] == 1) {
        gear = g_pAutoGearCar->gear;
        if (gear < 6) {
            int amount;
            int difference;

            g_pAutoGearCar->requestedGear = gear + 1;
            g_pAutoGearCar->damageShiftDelay = (char)g_autoGearPartSet->gearShiftDamage;
            load = g_autoGearPartSet->damageValues[14];
            if (load > 0xb333) {
                amount = FixMul(load - 0xb333, 0x3553f);
                if (amount > 0x10000)
                    amount = 0x10000;
                else if (amount < 0)
                    amount = 0;

                threshold = FixMul(amount, 0x4ccc);
                if (FIX_ABS(g_pAutoGearCar->corners[0].x) - FIX_ABS(g_pAutoGearCar->corners[0].z) < 0)
                    difference = -(FIX_ABS(g_pAutoGearCar->corners[0].x) - FIX_ABS(g_pAutoGearCar->corners[0].z));
                else
                    difference = FIX_ABS(g_pAutoGearCar->corners[0].x) - FIX_ABS(g_pAutoGearCar->corners[0].z);
                if ((difference % 0x401) * 0x40 < threshold) {
                    if (g_pAutoGearCar->requestedGear < 6) {
                        g_pAutoGearCar->requestedGear++;
                    }
                }
            }
            g_pAutoGearCar->shiftInProgress = 1;
            return;
        }
        if (gear == 7) {
            g_pAutoGearCar->requestedGear = 0;
            g_pAutoGearCar->shiftInProgress = 1;
            g_pAutoGearCar->damageShiftDelay = (char)g_autoGearPartSet->gearShiftDamage;
            return;
        }
    } else if (g_pAutoGearCar->field_0x1d4[0] == 0xff) {
        gear = g_pAutoGearCar->gear;
        if (gear < 7) {
            g_pAutoGearCar->requestedGear = gear - 1;
            g_pAutoGearCar->shiftInProgress = 1;
            g_pAutoGearCar->damageShiftDelay = (char)g_autoGearPartSet->gearShiftDamage;
            if (g_pAutoGearCar->requestedGear < 0)
                g_pAutoGearCar->requestedGear = 7;
        }
    }
}

// Selects the automatic gearbox's next gear from engine speed and road load.
// It also chooses reverse when the car stops against the driving direction.
// FUNCTION: CMR2 0x00493b30
void Car_UpdateAutomaticGear(void)
{
    int i;
    int selected;
    int best;
    int candidate;
    int engine;
    int load;
    int chance;
    int difference;
    int limit;
    int dot;
    BOOL wheelAvailable = FALSE;

    if (g_pAutoGearCar->automaticGearbox == 0)
        return;

    for (i = 0; i < 4; i++) {
        if (g_pAutoGearCar->cornerFlags[i] == 0) {
            wheelAvailable = TRUE;
            break;
        }
    }
    if (g_pAutoGearCar->autoShiftDelay > 0)
        g_pAutoGearCar->autoShiftDelay--;

    if (g_pAutoGearCar->automaticReverse == 0 && wheelAvailable) {
        best = (int)0xd8f00000;
        engine = FixMul(g_pAutoGearCar->engineSpeed, g_pAutoGearCar->gearSpeed[g_pAutoGearCar->gear]);
        selected = 0;
        for (i = 1; i < 7; i++) {
            candidate = FixMul(engine, g_pAutoGearCar->gearRatio[i]);
            if (candidate > best &&
                (candidate < FixMul(g_pAutoGearCar->engineSpeedLimit, 0xfae1) || i == 6)) {
                best = candidate;
                selected = i;
            }
        }
        if (selected != g_pAutoGearCar->gear) {
            load = FIX_ABS(g_pAutoGearCar->wheelSlip[0]);
            if ((load < 0x8000 || selected < g_pAutoGearCar->gear || g_pAutoGearCar->flag0x1d0[2] == 0) &&
                (g_pAutoGearCar->autoShiftDelay == 0 ||
                 ((g_pAutoGearCar->lastShiftDirection != 2 || selected >= g_pAutoGearCar->gear) &&
                  (g_pAutoGearCar->lastShiftDirection != 1 || selected <= g_pAutoGearCar->gear)))) {
                g_pAutoGearCar->shiftInProgress = 1;
                if (selected > g_pAutoGearCar->gear)
                    g_pAutoGearCar->lastShiftDirection = 2;
                else
                    g_pAutoGearCar->lastShiftDirection = 1;
                g_pAutoGearCar->autoShiftDelay = 10;

                if (selected > g_pAutoGearCar->gear && g_autoGearPartSet->damageValues[14] > 0xb333) {
                    chance = g_autoGearPartSet->damageValues[14] - 0xb333;
                    chance = FixMul(chance, 0x3553f);
                    if (chance > 0x10000)
                        chance = 0x10000;
                    else if (chance < 0)
                        chance = 0;
                    limit = FixMul(chance, 0x4ccc);
                    difference = FIX_ABS(FIX_ABS(g_pAutoGearCar->corners[0].x) -
                                         FIX_ABS(g_pAutoGearCar->corners[0].z));
                    if ((difference % 0x401) * 0x40 < limit && selected < 6)
                        selected++;
                }
                g_pAutoGearCar->requestedGear = (char)selected;
                g_pAutoGearCar->damageShiftDelay = (char)g_autoGearPartSet->gearShiftDamage;
            }
        }
    }

    dot = FixVecDot(&g_pAutoGearCar->velocity, &g_pAutoGearCar->right);
    if (g_pAutoGearCar->flag0x1d0[3] != 0 && g_pAutoGearCar->gear == 1 &&
        g_pAutoGearCar->flag0x1d0[2] == 0 && g_pAutoGearCar->engineSpeed < 0x1999 && dot < 0x1999) {
        g_pAutoGearCar->requestedGear = 7;
        g_pAutoGearCar->shiftInProgress = 1;
        g_pAutoGearCar->automaticReverse = 1;
        g_pAutoGearCar->damageShiftDelay = (char)g_autoGearPartSet->gearShiftDamage;
        return;
    }
    if (g_pAutoGearCar->flag0x1d0[2] != 0 && g_pAutoGearCar->flag0x1d0[3] == 0 &&
        g_pAutoGearCar->gear == 7) {
        g_pAutoGearCar->requestedGear = 1;
        g_pAutoGearCar->shiftInProgress = 1;
        g_pAutoGearCar->automaticReverse = 0;
        g_pAutoGearCar->damageShiftDelay = (char)g_autoGearPartSet->gearShiftDamage;
    }
}

void StageObject_GetSkyNode(SceneNode **pOut);
void StageObject_GetGroundNode(SceneNode **pOut);
void StageObject_GetCloudNodes(SceneNode **pOut1, SceneNode **pOut2);
extern Mesh *g_groundReferenceMesh;
extern FixVector g_lightMeshReferencePoint;

// GLOBAL: CMR2 0x00591c80
int g_stageHeightSamples[200];
// GLOBAL: CMR2 0x00591fa0
unsigned short *g_stageRandomTextures[3];   // one is picked at random for the whole stage mesh (0x492b50)
// GLOBAL: CMR2 0x00591fac
int g_stageHeightTarget;
// GLOBAL: CMR2 0x005920b8
SceneNode *g_stageLightNode;
// GLOBAL: CMR2 0x005920bc
SceneNode *g_stageLightObject;
// GLOBAL: CMR2 0x005920c0
SceneNode *g_stageLightRoot;
// GLOBAL: CMR2 0x005920c4
Mesh *g_stageMesh0;
// GLOBAL: CMR2 0x005920c8
Mesh *g_stageMesh0Copy;
// GLOBAL: CMR2 0x005920cc
Mesh *g_stageMesh1;
// GLOBAL: CMR2 0x005920d0
Mesh *g_stageMesh1Copy;
// GLOBAL: CMR2 0x005920d4
Mesh *g_stageMesh2;
// GLOBAL: CMR2 0x005920d8
Mesh *g_stageMesh2Copy;
// GLOBAL: CMR2 0x005920dc
Mesh *g_stageMesh3;
// GLOBAL: CMR2 0x005920e0
Mesh *g_stageMesh3Copy;
// GLOBAL: CMR2 0x005920e4
Mesh *g_stageMesh4;
// GLOBAL: CMR2 0x005920e8
Mesh *g_stageMesh4Copy;
// GLOBAL: CMR2 0x005920ec
Mesh *g_stageMesh5;
// GLOBAL: CMR2 0x005920f4
Mesh *g_stageMesh6;
// GLOBAL: CMR2 0x005920f8
Mesh *g_stageMesh6Copy;
// GLOBAL: CMR2 0x00592108
FixVector g_stageRangeOrigin;
// GLOBAL: CMR2 0x00592120
int g_stageHeightMin;
// GLOBAL: CMR2 0x00592124
int g_stageHeightScale;
// GLOBAL: CMR2 0x00592138
short g_stageMesh2Count;
// GLOBAL: CMR2 0x0059213a
short g_stageMesh3Count;
// GLOBAL: CMR2 0x0059213c
short g_stageMesh1Count;
// GLOBAL: CMR2 0x0059213e
short g_stageMesh0Count;
// GLOBAL: CMR2 0x00592140
short g_stageMesh4Count;
// GLOBAL: CMR2 0x00592142
short g_stageMesh5Count;
// GLOBAL: CMR2 0x00592144
short g_stageMesh6Count;
// GLOBAL: CMR2 0x00592146
BYTE g_groundColourAlpha;
// GLOBAL: CMR2 0x00592147
BYTE g_stageColourAlpha;
// GLOBAL: CMR2 0x00592148
int g_stageColourDirty;
// GLOBAL: CMR2 0x0059214c
int g_stageColourMode;
// GLOBAL: CMR2 0x00592150
int g_stageColourValue;
// GLOBAL: CMR2 0x00592154
int g_stageColourStep;
// GLOBAL: CMR2 0x0059215c
int g_stageColourState;
// GLOBAL: CMR2 0x00592158
int g_stageLightReady;

// Loads the stage's light meshes and their height samples.  The samples are
// fixed point values converted from the first mesh's floating point vertices.
// FUNCTION: CMR2 0x004919a0
void Stage_InitLightMeshes(void)
{
    SceneNode *object0;
    SceneNode *context;
    SceneNode *object2;
    SceneNode *object3;
    SceneNode *node;
    SceneNode *root;
    SceneNode *child;
    int i;
    int value;
    int minimum;
    int maximum;
    float *vertices;

    StageObject_GetSkyNode(&object0);
    StageObject_GetGroundNode(&context);
    StageObject_GetCloudNodes(&object2, &object3);
    node = NULL;
    root = NULL;
    child = NULL;
    if (object0 != NULL && (node = object0->pFirstChild) != NULL &&
        (root = node->pNext) != NULL)
        child = root->pNext;

    g_stageRandomTextures[0] = NULL;
    g_stageRandomTextures[1] = NULL;
    g_stageRandomTextures[2] = NULL;
    g_stageLightNode = node;
    g_stageLightObject = SceneType2_Create(&node->translation, &node->angles, NULL, node);
    g_stageLightRoot = child;
    g_stageMesh0Copy = NULL;
    g_stageMesh0Count = 0;
    g_stageMesh1Copy = NULL;
    g_stageMesh1Count = 0;
    g_stageMesh2Copy = NULL;
    g_stageMesh2Count = 0;
    g_stageMesh3Copy = NULL;
    g_stageMesh3Count = 0;
    g_stageMesh4Copy = NULL;
    g_stageMesh4Count = 0;
    g_groundReferenceMesh = NULL;
    g_stageMesh5Count = 0;
    g_stageMesh6Copy = NULL;
    g_stageMesh6Count = 0;

    g_stageMesh0 = (Mesh *)object0->pObject;
    g_stageMesh0Copy = g_stageMesh0;
    g_stageMesh0Count = (short)Mesh_GetField0x10(g_stageMesh0);
    g_stageMesh1 = (Mesh *)context->pObject;
    g_stageMesh1Copy = g_stageMesh1;
    g_stageMesh1Count = (short)Mesh_GetField0x10(g_stageMesh1);
    g_stageMesh2 = (Mesh *)object2->pObject;
    g_stageMesh2Copy = g_stageMesh2;
    g_stageMesh2Count = (short)Mesh_GetField0x10(g_stageMesh2);
    if (object3 != NULL) {
        g_stageMesh3 = (Mesh *)object3->pObject;
        g_stageMesh3Copy = g_stageMesh3;
        g_stageMesh3Count = (short)Mesh_GetField0x10(g_stageMesh3);
    }
    g_stageMesh4 = (Mesh *)root->pObject;
    g_stageMesh4Copy = g_stageMesh4;
    g_stageMesh4Count = (short)Mesh_GetField0x10(g_stageMesh4);
    g_stageMesh5 = (Mesh *)node->pObject;
    g_groundReferenceMesh = g_stageMesh5;
    g_stageMesh5Count = (short)Mesh_GetField0x10(g_stageMesh5);
    if (child != NULL) {
        g_stageMesh6 = (Mesh *)child->pObject;
        g_stageMesh6Copy = g_stageMesh6;
        g_stageMesh6Count = (short)Mesh_GetField0x10(g_stageMesh6);
    }
    g_stageColourDirty = 0;
    g_stageColourMode = 0;
    g_stageColourValue = 0;
    g_stageColourStep = 0;
    g_stageLightReady = 0;
    g_stageColourState = 0;

    if (g_stageMesh0Count != 0) {
        maximum = 0;
        minimum = 0;
        for (i = g_stageMesh0Count - 1; i >= 0; i--) {
            value = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)g_stageMesh0Copy->pVertexData)[i].pos[1], CGraphics::m_65536));
            if (i == g_stageMesh0Count - 1) {
                minimum = value;
                maximum = value;
            } else {
                if (value > maximum)
                    maximum = value;
                if (value < minimum)
                    minimum = value;
            }
            if (i == 0x68) {
                g_lightMeshReferencePoint.y = value;
                g_lightMeshReferencePoint.x = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)g_stageMesh0Copy->pVertexData)[i].pos[0], CGraphics::m_65536));
                g_lightMeshReferencePoint.z = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)g_stageMesh0Copy->pVertexData)[i].pos[2], CGraphics::m_65536));
            }
            g_stageHeightSamples[i] = value;
        }
        if (maximum != minimum) {
            g_stageHeightScale = FixDiv(0x10000, maximum - minimum);
            g_stageHeightMin = minimum;
        }
    }
    if (g_stageMesh1Count != 0) {
        g_stageRangeOrigin.x = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)g_stageMesh1Copy->pVertexData)[0x32].pos[0], CGraphics::m_65536));
        g_stageRangeOrigin.y = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)g_stageMesh1Copy->pVertexData)[0x32].pos[1], CGraphics::m_65536));
        g_stageRangeOrigin.z = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)g_stageMesh1Copy->pVertexData)[0x32].pos[2], CGraphics::m_65536));
        minimum = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)g_stageMesh1Copy->pVertexData)[0].pos[1], CGraphics::m_65536));
        maximum = minimum;
        for (i = g_stageMesh1Count - 1; i >= 0; i--) {
            value = (int)FTOI(__int64, X87S_MUL(((DeformFloatVertex *)g_stageMesh1Copy->pVertexData)[i].pos[1], CGraphics::m_65536));
            if (value > maximum)
                maximum = value;
            if (value < minimum)
                minimum = value;
        }
        g_stageHeightTarget = minimum + FixMul(maximum - minimum, 0xc000);
    }
    for (i = g_stageMesh4Count - 1; i >= 0; i--)
        ((DeformFloatVertex *)g_stageMesh4Copy->pVertexData)[i].colour = 0;
    g_stageLightReady = 1;
}

// Sets vertex colours from their heights, with a separate colour for the
// reference vertex recorded by Stage_InitLightMeshes.
// match 90%: the original places an integer instruction between each fld and
// fmul, and loads the alpha byte with xor/mov in the reference branch.
// FUNCTION: CMR2 0x00491d40
void Stage_SetHeightColours(BYTE *pLow, BYTE *pHigh, BYTE *pReference, int referenceBlend)
{
    FixVector low;
    FixVector delta;
    FixVector ref;
    FixVector colour;
    BYTE refColour[3];
    BYTE rgb[2];
    int value;
    int height;
    int t;
    int i;

    colour.x = (unsigned int)pHigh[0] << 16;
    colour.y = (unsigned int)pHigh[1] << 16;
    colour.z = (unsigned int)pHigh[2] << 16;
    low.x = (unsigned int)pLow[0] << 16;
    low.y = (unsigned int)pLow[1] << 16;
    low.z = (unsigned int)pLow[2] << 16;
    delta.x = colour.x - low.x;
    delta.y = colour.y - low.y;
    delta.z = colour.z - low.z;

    t = FixMul(g_lightMeshReferencePoint.y - g_stageHeightMin, g_stageHeightScale);
    if (t < 0)
        t = 0;
    else if (t > 0x10000)
        t = 0x10000;
    FixVecScale(&colour, &delta, t);
    colour.x = colour.x + low.x;
    colour.y = colour.y + low.y;
    colour.z = colour.z + low.z;
    ref.x = (unsigned int)pReference[0] << 16;
    ref.y = (unsigned int)pReference[1] << 16;
    ref.z = (unsigned int)pReference[2] << 16;
    ref.x -= colour.x;
    ref.y -= colour.y;
    ref.z -= colour.z;
    FixVecScale(&ref, &ref, referenceBlend);
    colour.x = colour.x + ref.x;
    colour.y = colour.y + ref.y;
    colour.z = colour.z + ref.z;
    refColour[0] = (BYTE)(colour.x >> 16);
    refColour[1] = (BYTE)(colour.y >> 16);
    refColour[2] = (BYTE)(colour.z >> 16);

    volatile BYTE *pAlpha = &g_stageColourAlpha;

    for (i = g_stageMesh0Count - 1; i >= 0; i--) {
        float *vertex = (float *)((BYTE *)g_stageMesh0Copy->pVertexData + i * 0x30);
        float vy = vertex[1];
        height = (int)FTOI(__int64, X87S_MUL((double)vy, CGraphics::m_65536));
        float vx = vertex[0];
        float vz = vertex[2];
        if (height == g_lightMeshReferencePoint.y &&
            g_lightMeshReferencePoint.x == (int)FTOI(__int64, X87S_MUL((double)vx, CGraphics::m_65536)) &&
            g_lightMeshReferencePoint.z == (int)FTOI(__int64, X87S_MUL((double)vz, CGraphics::m_65536))) {
            ((MeshVertexF *)g_stageMesh0Copy->pVertexData)[i].diffuseColour = ((0xffffff00 | refColour[0]) << 8 | refColour[1]) << 8 | refColour[2];
            ((MeshVertexF *)g_stageMesh0Copy->pVertexData)[i].specularColour = (DWORD)*pAlpha << 24;
        } else {
            t = FixMul(height - g_stageHeightMin, g_stageHeightScale);
            if (t < 0)
                t = 0;
            else if (t > 0x10000)
                t = 0x10000;
            FixVecScale(&colour, &delta, t);
            colour.x = colour.x + low.x;
            colour.y = colour.y + low.y;
            colour.z = colour.z + low.z;
            value = colour.x >> 16;
            if (value > 255) value = 255;
            else if (value < 0) value = 0;
            rgb[0] = (BYTE)value;
            value = colour.y >> 16;
            if (value > 255) value = 255;
            else if (value < 0) value = 0;
            rgb[1] = (BYTE)value;
            value = colour.z >> 16;
            if (value > 255) value = 255;
            else if (value < 0) value = 0;
            ((MeshVertexF *)g_stageMesh0Copy->pVertexData)[i].diffuseColour =
                ((0xffffff00 | rgb[0]) << 8 | rgb[1]) << 8 | (BYTE)value;
            ((MeshVertexF *)g_stageMesh0Copy->pVertexData)[i].specularColour = (DWORD)*pAlpha << 24;
        }
    }
    g_stageColourDirty = 1;
}

// Ramps field 0x818 of the auto-gear car toward +1 or -1 by its two flags.
// FUNCTION: CMR2 0x00494540
void AutoGear_RampDirectionState(void)
{
    if (g_pAutoGearCar->flag0x1d0[0] != 0) {
        if (g_pAutoGearCar->steeringInput < 0)
            g_pAutoGearCar->steeringInput = 0;
        g_pAutoGearCar->steeringInput += 0x10000;
        if (g_pAutoGearCar->steeringInput > 0x10000)
            g_pAutoGearCar->steeringInput = 0x10000;
    } else if (g_pAutoGearCar->flag0x1d0[1] != 0) {
        if (g_pAutoGearCar->steeringInput > 0)
            g_pAutoGearCar->steeringInput = 0;
        g_pAutoGearCar->steeringInput -= 0x10000;
        if (g_pAutoGearCar->steeringInput < -0x10000)
            g_pAutoGearCar->steeringInput = -0x10000;
    } else {
        g_pAutoGearCar->steeringInput = 0;
    }
}


// GLOBAL: CMR2 0x005920b0
SceneNode *g_stageAmbientNode;

void Scene_SetLightColour(SceneNode *pNode, int r, int g, int b);
void Mesh_RefreshVertices(Mesh *pMesh);

#define CLAMP_UNIT(v) ((v) > 0x10000 ? 0x10000 : ((v) < 0 ? 0 : (v)))

// Sets the stage ambient light from an 8-bit RGB triple (scaled to 16.16).
// FUNCTION: CMR2 0x00492e60
void TrackLighting_SetAmbientRGB(int *pRGB)
{
    int r = FixMul(pRGB[0], 0x106);
    int g;
    int b;

    if (r > 0x10000)
        r = 0x10000;
    else if (r < 0)
        r = 0;
    g = FixMul(pRGB[1], 0x106);
    if (g > 0x10000)
        g = 0x10000;
    else if (g < 0)
        g = 0;
    b = FixMul(pRGB[2], 0x106);
    if (b > 0x10000)
        b = 0x10000;
    else if (b < 0)
        b = 0;
    Scene_SetLightColour(g_stageAmbientNode, r, g, b);
    return;
}

// Pushes the vertex colours of every recoloured stage mesh.
// FUNCTION: CMR2 0x00492f10
void TrackLighting_PushRecolouredMeshVertices(void)
{
    if (g_stageColourStep != 0) {
        Mesh_RefreshVertices(g_stageMesh2);
        if (g_stageMesh3Copy != NULL)
            Mesh_RefreshVertices(g_stageMesh3);
        g_stageColourStep = 0;
    }
    if (g_stageLightReady != 0) {
        Mesh_RefreshVertices(g_stageMesh4);
        g_stageLightReady = 0;
    }
    if (g_stageColourMode != 0) {
        Mesh_RefreshVertices(g_stageMesh1);
        g_stageColourMode = 0;
    }
    if (g_stageColourDirty != 0) {
        Mesh_RefreshVertices(g_stageMesh0);
        g_stageColourDirty = 0;
    }
    if (g_stageColourValue != 0) {
        Mesh_RefreshVertices(g_stageMesh5);
        g_stageColourValue = 0;
    }
    if (g_stageColourState != 0) {
        Mesh_RefreshVertices(g_stageMesh6);
        g_stageColourState = 0;
    }
}

extern double g_fixedDegreesToAngle12;

// Swings field 0x848 of the auto-gear car toward its target over time (a
// quarter sine), or resets it when the swing is off.
// FUNCTION: CMR2 0x00494960
void AutoGear_UpdateSecondarySwing(void)
{
    unsigned short angle;

    if (g_pAutoGearCar->handbrake != 0) {
        g_pAutoGearCar->handbrakePhase += g_pAutoGearCar->handbrakeRampStep;
        if (g_pAutoGearCar->handbrakePhase > 0x10000) {
            g_pAutoGearCar->handbrakePhase = 0x10000;
            g_pAutoGearCar->handbrakeForce = g_pAutoGearCar->maxHandbrakeForce;
            return;
        }
        angle = (unsigned short)FTOI(__int64, X87S_MUL(FixMul(g_pAutoGearCar->handbrakePhase, 0x5a0000), g_fixedDegreesToAngle12));
        g_pAutoGearCar->handbrakeForce = FixMul(g_pAutoGearCar->maxHandbrakeForce, g_sinTable[angle & 0xfff]);
    } else {
        g_pAutoGearCar->handbrakePhase = 0;
        g_pAutoGearCar->handbrakeForce = 0;
    }
}

void Graphics_SetFog(int start, int end, int a, int b, DWORD colour);

// Sets the fog and the matching sky alpha: the sky fades out as the draw
// distance reaches into the fog.
// FUNCTION: CMR2 0x00492fe0
void Track_SetFogAndSkyAlpha(DWORD *pColour, int start, int end)
{
    BYTE rgb[3];
    int distance;
    int t;
    int alpha;
    int range;
    char a;

    rgb[0] = ((BYTE *)pColour)[0];
    rgb[1] = ((BYTE *)pColour)[1];
    rgb[2] = ((BYTE *)pColour)[2];
    Graphics_SetFog(start, end, start, end, *(DWORD *)rgb);
    distance = (CGameInfo::GetGraphicsOptionBits21To24() + 2) * 0x320000;
    if (distance < start) {
        a = -1;
    } else if (distance > end) {
        a = 0;
    } else {
        t = distance - start;
        range = end - start;
        if (range == 0)
            g_groundColourAlpha = 0;
        else
            t = FixDiv(t, range);
        alpha = FixMul(t, 0xff0000) >> 16;
        if (alpha > 0xff)
            alpha = 0xff;
        else if (alpha < 0)
            alpha = 0;
        a = -1 - alpha;
    }
    g_groundColourAlpha = a;
    g_stageColourAlpha = a;
}

// Integrates the throttle phase and torque from the accelerator, or from
// the brake pedal in automatic reverse. Latches accelerator input while
// the forward throttle ramp is decaying.
// FUNCTION: CMR2 0x004946c0
void AutoGear_UpdateRollingFollowRate(void)
{
    unsigned short angle;

    if (g_pAutoGearCar->flag0x1d0[2] == 0 &&
        (g_pAutoGearCar->automaticReverse == 0 || g_pAutoGearCar->flag0x1d0[3] == 0)) {
        g_pAutoGearCar->throttlePhase =
            g_pAutoGearCar->throttlePhase - FixMul(g_pAutoGearCar->throttleRampStep, 0x20000);
        if (g_pAutoGearCar->throttlePhase < 0) {
            g_pAutoGearCar->throttlePhase = 0;
            g_pAutoGearCar->throttleTorque = 0;
        } else {
            angle = (unsigned short)FTOI(__int64, X87S_MUL(FixMul(g_pAutoGearCar->throttlePhase, 0x5a0000), g_fixedDegreesToAngle12));
            g_pAutoGearCar->throttleTorque = FixMul(g_pAutoGearCar->maxThrottleTorque, g_sinTable[angle & 0xfff]);
        }
        if (g_pAutoGearCar->throttleTorque != 0 && g_pAutoGearCar->automaticReverse == 0) {
            g_pAutoGearCar->flag0x1d0[2] = 0x3f;
            return;
        }
    } else {
        g_pAutoGearCar->throttlePhase = g_pAutoGearCar->throttlePhase + g_pAutoGearCar->throttleRampStep;
        if (g_pAutoGearCar->throttlePhase > 0x10000) {
            g_pAutoGearCar->throttlePhase = 0x10000;
            g_pAutoGearCar->throttleTorque = g_pAutoGearCar->maxThrottleTorque;
            return;
        }
        angle = (unsigned short)FTOI(__int64, X87S_MUL(FixMul(g_pAutoGearCar->throttlePhase, 0x5a0000), g_fixedDegreesToAngle12));
        g_pAutoGearCar->throttleTorque = FixMul(g_pAutoGearCar->maxThrottleTorque, g_sinTable[angle & 0xfff]);
    }
}

// Swings field 0x838 of the auto-gear car toward 0x82c over time while its
// flag 3 is set and the shift is unlocked; else resets it.
// FUNCTION: CMR2 0x00494880
void AutoGear_UpdateSteeringSwing(void)
{
    unsigned short angle;

    if (g_pAutoGearCar->flag0x1d0[3] != 0 && g_pAutoGearCar->automaticReverse == 0) {
        g_pAutoGearCar->brakePhase += g_pAutoGearCar->brakeRampStep;
        if (g_pAutoGearCar->brakePhase > 0x10000) {
            g_pAutoGearCar->brakePhase = 0x10000;
            g_pAutoGearCar->brakeInput = g_pAutoGearCar->maxBrakeForce;
            return;
        }
        angle = (unsigned short)FTOI(__int64, X87S_MUL(FixMul(g_pAutoGearCar->brakePhase, 0x5a0000), g_fixedDegreesToAngle12));
        g_pAutoGearCar->brakeInput = FixMul(g_pAutoGearCar->maxBrakeForce, g_sinTable[angle & 0xfff]);
        return;
    }
    g_pAutoGearCar->brakePhase = 0;
    g_pAutoGearCar->brakeInput = 0;
}

// Flags whether the auto-gear car is at or below its best gear for the
// current revs (gear with the most torque below 98% of the limit).
// FUNCTION: CMR2 0x00493a40
void AutoGear_UpdateBestGearFlag(void)
{
    BYTE best;
    int bestTorque;
    int torque;
    int gear;

    if (g_pAutoGearCar->gearRatio[g_pAutoGearCar->gear] > 0) {
        int baseTorque;
        bestTorque = 0xd8f00000;
        baseTorque = FixMul(g_pAutoGearCar->engineSpeed, g_pAutoGearCar->gearSpeed[g_pAutoGearCar->gear]);
        best = 0;
        for (gear = 1; gear < 7; gear++) {
            torque = FixMul(baseTorque, g_pAutoGearCar->gearRatio[gear]);
            if (torque > bestTorque && (torque < FixMul(g_pAutoGearCar->engineSpeedLimit, 0xfae1) || gear == 6)) {
                bestTorque = torque;
                best = gear;
            }
        }
    } else {
        best = 7;
    }
    if (g_pAutoGearCar->gear > best) {
        g_pAutoGearCar->gearAtOrBelowBest = 0;
        return;
    }
    g_pAutoGearCar->gearAtOrBelowBest = 1;
}

// GLOBAL: CMR2 0x00592164
int g_unk0x00592164;

// GLOBAL: CMR2 0x00592160
int g_unk0x00592160;

// Reduces the steering scale as the car's forward velocity aligns with its
// right axis.  The result is expressed in the same 0x1680 units as field b16.
// FUNCTION: CMR2 0x004943d0
void AutoGear_UpdateSteeringScale(void)
{
    int alignment;
    int amount;

    if ((g_pAutoGearCar->flags & 1) != 0) {
        alignment = FIX_ABS(FixVecDot(&g_pAutoGearCar->velocity, &g_pAutoGearCar->right));
        amount = FixMul(alignment, g_pAutoGearCar->steeringSpeedScale);
        if (amount >= 0xb333)
            amount = 0xb333;
        g_unk0x00592160 = FixMul(0x10000 - amount, g_pAutoGearCar->maxSteeringAngleDegrees) * 0x1680;
        return;
    }
    g_unk0x00592160 = g_pAutoGearCar->maxSteeringAngleDegrees * 0x1680;
}

void AutoGear_RampDirectionState(void);
extern int g_physicsTimeStep;

// Steering torque of the auto-gear car from its steering swing.
// FUNCTION: CMR2 0x004945d0
void AutoGear_ComputeSteeringTorque(void)
{
    int torque;
    int a;
    int r;

    AutoGear_RampDirectionState();
    if (g_pAutoGearCar->flag0x1d0[1] == 0 && g_pAutoGearCar->flag0x1d0[0] == 0) {
        g_unk0x00592164 = 0;
        return;
    }
    torque = FixMul(g_pAutoGearCar->steeringTorqueScale, g_physicsTimeStep);
    if ((g_pAutoGearCar->flags & 2) != 0) {
        torque = FixMul(torque, FixMul(
            (g_pAutoGearCar->steeringAccumulator < 0 ? -g_pAutoGearCar->steeringAccumulator : g_pAutoGearCar->steeringAccumulator) - 0x10000,
            (g_pAutoGearCar->steeringAccumulator < 0 ? -g_pAutoGearCar->steeringAccumulator : g_pAutoGearCar->steeringAccumulator) - 0x10000));
    }
    a = g_pAutoGearCar->steeringInput < 0 ? -g_pAutoGearCar->steeringInput : g_pAutoGearCar->steeringInput;
    r = FixMul(torque, a);
    if (g_pAutoGearCar->flag0x1d0[1] != 0) {
        g_unk0x00592164 = -r;
        return;
    }
    if (g_pAutoGearCar->flag0x1d0[0] != 0)
        g_unk0x00592164 = r;
}

void Scene_SetLightPosition(SceneNode *pNode, int x, int y, int z);
extern int g_unk0x00592128;
extern int g_unk0x0059212c;
extern int g_unk0x00592130;

// Shifts stage mesh heights and moves the ambient light with the stage.
// FUNCTION: CMR2 0x004925c0
void Track_ShiftMeshAndAmbientHeights(int oldHeight, int newHeight, int mode)
{
    int height = g_stageHeightTarget - oldHeight + newHeight;
    int i;
    SceneNode *object;
    FixVector position;

    for (i = g_stageMesh0Count - 1; i >= 0; i--) {
        if (g_stageHeightSamples[i] < height) {
            float *vertex = (float *)((BYTE *)g_stageMesh0Copy->pVertexData + i * 0x30);
            float vx = vertex[0];
            float vy = vertex[1];
            float vz = vertex[2];
            if (g_lightMeshReferencePoint.x != (int)FTOI(__int64, X87S_MUL((double)vx, CGraphics::m_65536)) ||
                g_lightMeshReferencePoint.y != (int)FTOI(__int64, X87S_MUL((double)vy, CGraphics::m_65536)) ||
                g_lightMeshReferencePoint.z != (int)FTOI(__int64, X87S_MUL((double)vz, CGraphics::m_65536)))
                vertex[1] = (float)(X87S_MUL((double)height, CGraphics::m_oneOver65536));
        }
    }
    g_stageColourDirty = 1;
    g_unk0x00592128 = mode;
    g_unk0x00592130 = newHeight;
    g_unk0x0059212c = oldHeight;
    StageObject_GetSkyNode(&object);
    FixMatrix_RotateVector(&position, &g_lightMeshReferencePoint, &object->current);
    position.y += g_unk0x0059212c;
    Scene_SetLightPosition(g_stageAmbientNode, position.x, position.y, position.z);
}

int RallyData_IsChampionshipFinalStage(void);
unsigned char RallyDataState(void);
unsigned char RallyData_GetFlag24(void);
unsigned int RallyData_GetSelectionFlag27(void);
BYTE *StageUI_GetRaceResultTable(void);
int Race_GetPlayerRecordField4(BYTE index);
ReplayStream *Race_GetPlayerRecordPointer(int index);
void StageObject_SpawnCarHeadlightGlow(BYTE car);

// Applies the automatic gearbox's mid-shift body nudge: while a shift is in
// progress the body is pushed along its right axis by an amount derived from
// the road speed, then the shift flag is cleared.
// FUNCTION: CMR2 0x004932f0
void AutoGear_ApplyShiftBodyNudge(void)
{
    int shifted;
    int speed;
    FixVector offset;

    shifted = 0;
    if (CGameInfo::IsActiveCheatEnabled(5) != 0) {
        g_pAutoGearCar->field_0xa84 = 0;
        if ((int)g_pAutoGearCar->index < (int)(BYTE)RallyDataState() &&
            Race_GetPlayerRecordField4(g_pAutoGearCar->index) != 0 &&
            (char)Race_GetPlayerRecordPointer((int)g_pAutoGearCar->index)->field_0x10c == 8)
            shifted = 1;
        if ((char)((CallbackStateMachine *)StageUI_GetRaceResultTable())->records[g_pAutoGearCar->index].packedState == 8 ||
            shifted) {
            if (g_pAutoGearCar->handbrake != 0) {
                speed = g_pAutoGearCar->speed;
                if (speed <= 0x2c000) {
                    g_pAutoGearCar->field_0xa84 = 0x10000;
                } else if (speed <= 0x30000) {
                    g_pAutoGearCar->field_0xa84 = speed - 0x2c000;
                    g_pAutoGearCar->field_0xa84 =
                        FixMul(g_pAutoGearCar->field_0xa84, 0x40000);
                    if (g_pAutoGearCar->field_0xa84 > 0x10000)
                        g_pAutoGearCar->field_0xa84 = 0x10000;
                    g_pAutoGearCar->field_0xa84 =
                        0x10000 - g_pAutoGearCar->field_0xa84;
                }
                FixVecScale(&offset, &g_pAutoGearCar->right,
                            FixMul(g_pAutoGearCar->field_0xa84, 0x4000));
                g_pAutoGearCar->field_0x5c4.x += offset.x;
                g_pAutoGearCar->field_0x5c4.y += offset.y;
                g_pAutoGearCar->field_0x5c4.z += offset.z;
            }
            g_pAutoGearCar->handbrake = 0;
        }
    } else {
        if (CGameInfo::IsActiveCheatEnabled(0) != 0) {
            if (((char)RallyData_IsChampionshipFinalStage() != 0 || (char)RallyData_GetFlag24() != 0 ||
                 (char)RallyData_GetSelectionFlag27() != 0) &&
                (g_pAutoGearCar->automaticGearbox != 0 && g_pAutoGearCar->handbrake != 0))
                StageObject_SpawnCarHeadlightGlow(g_pAutoGearCar->index);
            g_pAutoGearCar->handbrake = 0;
        }
    }
}

extern double g_fixedDegreesToAngle12;

// Eases the car's steering angle towards the value taken from its track
// surface, then stores the damped angle into 0xb10/0xb12.
// match 87%: implemented; MSVC keeps the two scale factors in different registers and
// reorders the fixpoint intermediates
// FUNCTION: CMR2 0x00493ed0
void CarPhysics_DampSurfaceSteeringAngle(void)
{
    int outwardRate;
    int centringRate;
    int value;
    int other;
    short target;
    short current;
    short delta;

    outwardRate = g_pAutoGearCar->steeringOutwardRate;
    centringRate = g_pAutoGearCar->steeringCentringRate;
    other = FixMul(*(BYTE *)&g_pAutoGearCar->flag0x1d0[1] << 16, 0x410);
    if (other > 0x10000)
        other = 0x10000;
    value = FixMul(*(BYTE *)&g_pAutoGearCar->flag0x1d0[0] << 16, 0x410);
    if (value > 0x10000)
        value = 0x10000;
    other = other - value;
    value = other;
    AutoGear_UpdateSteeringScale();

    if (value < -0x10000)
        value = -0x10000;
    else if (value > 0x10000)
        value = 0x10000;
    if (value == 0)
        target = 0;
    else
        target = (short)FTOI(__int64, X87S_MUL((double)FixMul(g_unk0x00592160, value), g_fixedDegreesToAngle12));

    if (g_pAutoGearCar->steeringInputMode == 2) {
        *(short *)&g_pAutoGearCar->wheelSteeringAngle = target;
        g_pAutoGearCar->targetSteeringAngle = *(short *)&g_pAutoGearCar->wheelSteeringAngle;
        return;
    }

    current = *(short *)&g_pAutoGearCar->wheelSteeringAngle;
    delta = current - target;
    if (delta > 0x800)
        delta = 0x1000 - delta;
    if (delta >= 0x20 || delta <= -0x20) {
        if (current == 0)
            delta = (short)FTOI(__int64, X87S_MUL((double)FixMul(delta * 0x1680, outwardRate), g_fixedDegreesToAngle12));
        else if (current > 0) {
            if (delta < 0)
                delta = (short)FTOI(__int64, X87S_MUL((double)FixMul(delta * 0x1680, outwardRate), g_fixedDegreesToAngle12));
            else
                delta = (short)FTOI(__int64, X87S_MUL((double)FixMul(delta * 0x1680, centringRate), g_fixedDegreesToAngle12));
        } else {
            if (delta > 0)
                delta = (short)FTOI(__int64, X87S_MUL((double)FixMul(delta * 0x1680, outwardRate), g_fixedDegreesToAngle12));
            else
                delta = (short)FTOI(__int64, X87S_MUL((double)FixMul(delta * 0x1680, centringRate), g_fixedDegreesToAngle12));
        }
    }
    *(short *)&g_pAutoGearCar->wheelSteeringAngle -= delta;
    g_pAutoGearCar->targetSteeringAngle = *(short *)&g_pAutoGearCar->wheelSteeringAngle;
}

// GLOBAL: CMR2 0x00592168
int g_unk0x00592168;
// GLOBAL: CMR2 0x00511308
double g_minusFixedDegreesToAngle12 = -4096.0 / (360.0 * 65536.0);

// Integrates the auto-gear steering accumulator for the current surface and
// eases the signed 12-bit wheel angle towards the new target.
// Keeps the original intermediate forces and materialized zero bounds.
// Differential coverage: guarded car, signed limits, angles and real helpers.
// FUNCTION: CMR2 0x00494110
void AutoGear_IntegrateSteeringAccumulator(void)
{
    int v;
    int sum;
    int cur;
    int magCur;
    int magSum;
    int deadZone;
    short target;
    short step;
    short limit;

    AutoGear_UpdateSteeringScale();
    AutoGear_ComputeSteeringTorque();
    g_unk0x00592168 = g_pAutoGearCar->steeringReturnRate;
    if (g_pAutoGearCar->steeringAccumulator == 0) {
        g_unk0x00592168 = 0;
    } else {
        if (FixMul(g_unk0x00592160, g_pAutoGearCar->steeringAccumulator) > 0)
            g_unk0x00592168 = -g_unk0x00592168;
    }

    v = 0x10000 - FixMul(g_pAutoGearCar->cornerGrip[1].field_0x14 +
                         g_pAutoGearCar->cornerGrip[0].field_0x14, 0x8000);
    g_unk0x00592164 = FixMul(g_unk0x00592164, v);
    g_unk0x00592168 = FixMul(g_unk0x00592168, v);
    g_unk0x00592164 = FixMul(g_unk0x00592164, g_pAutoGearCar->steeringForceScale);
    g_unk0x00592168 = FixMul(g_unk0x00592168, g_pAutoGearCar->steeringForceScale);

    if ((g_pAutoGearCar->flag0x1d0[0] == 0 &&
         g_pAutoGearCar->flag0x1d0[1] == 0) ||
        (g_pAutoGearCar->flag0x1d0[0] != 0 && g_unk0x00592168 > 0) ||
        (g_pAutoGearCar->flag0x1d0[1] != 0 && g_unk0x00592168 < 0)) {
        sum = g_unk0x00592164 + g_unk0x00592168;
        cur = g_pAutoGearCar->steeringAccumulator;
        magCur = cur < 0 ? -cur : cur;
        magSum = sum < 0 ? -sum : sum;
        if (magCur <= magSum)
            g_pAutoGearCar->steeringAccumulator = 0;
        else
            g_pAutoGearCar->steeringAccumulator = cur + sum;
        goto convert;
    } else if (g_pAutoGearCar->flag0x1d0[1] != 0) {
        deadZone = FixMul(0, v);
        g_pAutoGearCar->steeringAccumulator += g_unk0x00592164;
        if (g_pAutoGearCar->steeringAccumulator < -0x10000)
            g_pAutoGearCar->steeringAccumulator = 0xffff0000;
        else if (g_pAutoGearCar->steeringAccumulator > -deadZone)
            g_pAutoGearCar->steeringAccumulator = -deadZone;
        goto convert;
    } else if (g_pAutoGearCar->flag0x1d0[0] != 0) {
        deadZone = FixMul(0, v);
        g_pAutoGearCar->steeringAccumulator += g_unk0x00592164;
        if (g_pAutoGearCar->steeringAccumulator > 0x10000)
            g_pAutoGearCar->steeringAccumulator = 0x10000;
        else if (g_pAutoGearCar->steeringAccumulator < deadZone)
            g_pAutoGearCar->steeringAccumulator = deadZone;
    }
convert:
    target = (short)FTOI(__int64, X87S_MUL((double)FixMul(g_unk0x00592160,
                                             g_pAutoGearCar->steeringAccumulator), g_minusFixedDegreesToAngle12));
    g_pAutoGearCar->targetSteeringAngle = target;
    step = g_pAutoGearCar->targetSteeringAngle - *(short *)&g_pAutoGearCar->wheelSteeringAngle;
    limit = (short)(FixMul(0x22, 0x10000 - v) + 0x22);
    if (step < 0)
        cur = -step;
    else
        cur = step;
    if (cur < limit) {
        *(short *)&g_pAutoGearCar->wheelSteeringAngle = g_pAutoGearCar->targetSteeringAngle;
        return;
    }
    if (step > 0)
        *(short *)&g_pAutoGearCar->wheelSteeringAngle += limit;
    else
        *(short *)&g_pAutoGearCar->wheelSteeringAngle -= limit;
}

CarPartSet *CarPart_GetSet(int carIndex);
extern double g_fixedDegreesToAngle12;

// Applies the automatic-gear state to the car for one frame: rebuilds the body
// pitch/roll from the setup data, advances the active gear and the shifting
// hysteresis, and updates the gear ratios written to the car.
// match 49%: implementada, caching de registros y orden de bloques distinto
// FUNCTION: CMR2 0x00493520
void AutoGear_UpdateCarGearState(Car *pCar)
{
    char cVar1;
    int t;
    int u;
    int v;

    g_pAutoGearCar = pCar;
    g_autoGearPartSet = CarPart_GetSet((int)pCar->index);
    AutoGear_ApplyShiftBodyNudge();
    if (g_pAutoGearCar->analogThrottle != 0) {
        if (*(BYTE *)&g_pAutoGearCar->flag0x1d0[2] != 0) {
            t = FixMul((int)((unsigned int)*(BYTE *)&g_pAutoGearCar->flag0x1d0[2] << 16), 0x410);
            if (t > 0x10000)
                t = 0x10000;
            g_pAutoGearCar->throttleTorque = FixMul(g_pAutoGearCar->maxThrottleTorque, t);
        } else {
            g_pAutoGearCar->throttleTorque = 0;
        }
    } else {
        AutoGear_UpdateRollingFollowRate();
    }
    if (g_pAutoGearCar->gearRatio[g_pAutoGearCar->gear] < 0)
        g_pAutoGearCar->reversing = 1;
    else
        g_pAutoGearCar->reversing = 0;
    g_pAutoGearCar->braking = 0;
    if (g_pAutoGearCar->automaticReverse != 0) {
        if (g_pAutoGearCar->flag0x1d0[2] != 0)
            g_pAutoGearCar->braking = 1;
    } else {
        if (g_pAutoGearCar->flag0x1d0[3] != 0)
            g_pAutoGearCar->braking = 1;
    }
    if (g_pAutoGearCar->field_0xb1f == 0) {
        if (g_pAutoGearCar->shiftInProgress != 0) {
            if (g_pAutoGearCar->damageShiftDelay > 0) {
                g_pAutoGearCar->damageShiftDelay = g_pAutoGearCar->damageShiftDelay - 1;
                *(BYTE *)&g_pAutoGearCar->gear = 0;
            } else {
                g_pAutoGearCar->shiftInProgress = 0;
                *(BYTE *)&g_pAutoGearCar->gear = *(BYTE *)&g_pAutoGearCar->requestedGear;
            }
        } else {
            if ((g_pAutoGearCar->field_0xb48 == 1) && (g_pAutoGearCar->field_0x1d4[0] != 0))
                AutoGear_RequestAdjacentGear();
            else if (g_pAutoGearCar->field_0xb48 == 2)
                Car_UpdateAutomaticGear();
        }
    }
    if (g_pAutoGearCar->steeringInputMode != 0)
        CarPhysics_DampSurfaceSteeringAngle();
    else
        AutoGear_IntegrateSteeringAccumulator();
    if (g_pAutoGearCar->analogBrake != 0) {
        if (*(BYTE *)&g_pAutoGearCar->flag0x1d0[3] != 0) {
            t = FixMul((int)((unsigned int)*(BYTE *)&g_pAutoGearCar->flag0x1d0[3] << 16), 0x410);
            if (t > 0x10000)
                t = 0x10000;
            if (g_pAutoGearCar->automaticReverse != 0) {
                g_pAutoGearCar->throttleTorque = FixMul(g_pAutoGearCar->maxThrottleTorque, t);
                g_pAutoGearCar->brakeInput = 0;
            } else {
                int target = FixMul(g_pAutoGearCar->maxBrakeForce, t);
                int diff = target - g_pAutoGearCar->brakeInput;
                int adiff = (diff < 0) ? -diff : diff;
                if (adiff < FixMul(g_pAutoGearCar->maxBrakeForce, 0xccc))
                    g_pAutoGearCar->brakeInput = target;
                else
                    g_pAutoGearCar->brakeInput = g_pAutoGearCar->brakeInput + FixMul(diff, 0x23d7);
            }
        } else {
            g_pAutoGearCar->brakeInput = 0;
        }
    } else {
        AutoGear_UpdateSteeringSwing();
        if ((g_pAutoGearCar->analogThrottle != 0) && (g_pAutoGearCar->automaticReverse != 0)) {
            t = FixMul((int)((unsigned int)*(BYTE *)&g_pAutoGearCar->flag0x1d0[3] << 16), 0x410);
            if (t > 0x10000)
                t = 0x10000;
            g_pAutoGearCar->throttleTorque = FixMul(g_pAutoGearCar->maxThrottleTorque, t);
        }
    }
    AutoGear_UpdateSecondarySwing();
    if (g_pAutoGearCar->field_0xb48 == 1)
        AutoGear_UpdateBestGearFlag();

    t = FixMul(*(short *)&g_pAutoGearCar->wheelSteeringAngle * 0x1680, 0x20000);
    u = t;
    if (u < 0)
        u = -u;
    if (u > g_unk0x00592160) {
        if (t > 0)
            t = g_unk0x00592160;
        else
            t = -g_unk0x00592160;
    }
    g_pAutoGearCar->renderSteeringAngle = (short)FTOI(__int64, X87S_MUL((double)t, g_fixedDegreesToAngle12));
}
