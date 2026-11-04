#include "CameraMove.h"
#include "CameraOperator.h"
#include "port/sys.h"
#include "SpeedFov.h"
#include "ReplayCameraCycle.h"
#include "FreeCam.h"
#include "ReplayChaseOrbit.h"
#include <math.h>
#include "FloatToInt.h"
#include <stddef.h>
#include "Graphics.h"
#include "Car.h"
#include "RallyRouteNode.h"
#include "CarResources.h"
#include "CarParts.h"
#include "FileBuffer.h"
#include "Game.h"
#include "GameInfo.h"
#include "RallyData.h"
#include "Mesh.h"
#include "main.h"
#include "ReplayStream.h"
#include <string.h>

Car *g_cars[40];
int g_carCount;
Car *g_carBuffer;
Car *g_pCurrentCar;
CarTransforms g_carTransforms[8];
FixMatrix g_carWheelTransforms[8][4];
short g_carOrderCount;
short g_carOrder[48];
int g_carViewScale[15][2];
SceneNode *g_viewNodes[3];
// Previous camera state of each view, laid out like the view records.
// GLOBAL: CMR2 0x00538e38
CameraRecord g_viewPreviousStates[2];
// GLOBAL: CMR2 0x00538e04
int g_viewFarPlanes[2];
// GLOBAL: CMR2 0x00538e0c
BYTE g_activeViewRecord[4];
// Blended camera state of each view, laid out like the view records.
// GLOBAL: CMR2 0x00538d28
CameraRecord g_viewStates[2];
// GLOBAL: CMR2 0x00538df0
int g_viewNearPlanes[2];
// GLOBAL: CMR2 0x00538df8
short g_unk0x00538df8[2];
// GLOBAL: CMR2 0x00538c98
int g_unk0x00538c98[2];
// GLOBAL: CMR2 0x00538f00
int g_unk0x00538f00[2];
// View camera records: 2 per player (the player's two view modes).
// GLOBAL: CMR2 0x00539018
CameraRecord g_viewRecords[4];
// GLOBAL: CMR2 0x00538ca0
FixMatrix g_unk0x00538ca0[2];

// Difference between the two stored view positions for one car slot.
// FUNCTION: CMR2 0x004220d0
void Car_GetViewPositionDelta(FixVector *pOut, unsigned int view)
{
    FixVector a;
    FixVector b;
    int offset = (view & 0xff) * 100;

    FixMatrix_GetPosition(&b, (FixMatrix *)((BYTE *)&g_viewPreviousStates[0].matrix + offset));
    FixMatrix_GetPosition(&a, (FixMatrix *)((BYTE *)&g_viewStates[0].matrix + offset));
    pOut->x = a.x - b.x;
    pOut->y = a.y - b.y;
    pOut->z = a.z - b.z;
}

// FUNCTION: CMR2 0x00422f90
void View_SetFarPlane(BYTE index, int value)
{
    g_viewFarPlanes[index] = value;
}

// FUNCTION: CMR2 0x00422f50
int View_GetActiveCameraMode(BYTE index)
{
    if (g_viewStates[index].type != 8)
        return g_viewRecords[(unsigned int)g_activeViewRecord[index] + index * 2].type;
    return 8;
}

// FUNCTION: CMR2 0x00422fb0
BYTE View_GetActiveCameraFlags(BYTE index)
{
    return g_viewRecords[(unsigned int)g_activeViewRecord[index] + index * 2].car;
}

// match 67%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00423d70
FixMatrix *Car_GetCameraReferenceMatrix(BYTE index)
{
    BYTE state = (BYTE)RallyDataState();
    return index < state ? &g_unk0x00538ca0[index] : Car_Get(index)->pWorld;
}

// Release callback of Car_AllocateTable.
// FUNCTION: CMR2 0x00428790
int Car_FreeTable(void)
{
    if (g_carBuffer != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_carBuffer);
        g_carBuffer = NULL;
    }
    g_carCount = 0;
    return 1;
}

// Allocates one Car per slot and registers the 0x428790 callback.
// FUNCTION: CMR2 0x004287c0
void Car_AllocateTable(int count)
{
    Car *pBuffer;
    int i;

    pBuffer = (Car *)CFileBuffer::AllocateLockedBuffer(count * sizeof(Car));
    g_carBuffer = pBuffer;
    g_carCount = count;
    for (i = 0; i < count; i++) {
        g_cars[i] = pBuffer;
        pBuffer++;
    }
    CGame::RegisterCallback(Car_FreeTable, NULL);
}

int NetPlayers_GetPlayerField8(int index);
ReplayStream *Race_GetPlayerRecordPointer(int index);
unsigned int NetPlayers_IsPlayerPresent(int index);
void Car_MarkHiddenFromCameras(Car *pCar);
extern short g_unk0x0053a314[8];
extern short g_unk0x0053bd6c[26];
extern short g_unk0x0053c9a0;

// GLOBAL: CMR2 0x0053a270
short g_unk0x0053a270[80];
// GLOBAL: CMR2 0x0053a310
short g_unk0x0053a310;
// GLOBAL: CMR2 0x0053b4f0
short g_unk0x0053b4f0[8];

// Builds the race car order of the current view: the active players first (and
// whoever reports itself), then splits them into the on-screen, shadowed and
// drawn lists and re-applies the view transform of the on-screen ones.
// FUNCTION: CMR2 0x00428810
void Car_BuildViewOrder(void)
{
    short *p;
    int i;
    int j;
    int flag;
    Car *pCar;
    if (CGameInfo::GetConfiguredGameMode() == 8 || CGameInfo::GetConfiguredGameMode() == 9 ||
        CGameInfo::GetConfiguredGameMode() == 0xb || CGameInfo::GetConfiguredGameMode() == 0xc) {
        g_carOrder[0] = 0;
        g_carOrderCount = 1;
        for (i = 0; i < 7; i++) {
            flag = 0;
            ReplayStream *pPlayer = Race_GetPlayerRecordPointer(i + 1);
            if (pPlayer != 0 && pPlayer->playing != 0 && pPlayer->field_0xe8 != 0)
                flag = 1;
            if ((BYTE)NetPlayers_IsPlayerPresent(i) != 0 || flag) {
                g_carOrder[g_carOrderCount] = NetPlayers_GetPlayerField8(i);
                g_carOrderCount++;
            }
        }
    }
    g_carOrder[26] = 0;
    g_unk0x0053a310 = 0;
    g_unk0x0053c9a0 = 0;
    g_carOrder[25] = 0;
    g_carOrder[44] = 0;
    j = 0;
    if (g_carOrderCount > 0) {
        p = g_carOrder;
        do {
            pCar = g_carBuffer + *p;
            pCar->field_0xb70 = 0;
            pCar->field_0xb6c = 0;
            pCar->firstCameraBlocked = 0;
            if (pCar->field_0xc18 == 0) {
                if (pCar->simulationStepsRemaining > 0) {
                    g_unk0x0053b4f0[g_unk0x0053a310] = *p;
                    g_unk0x0053a310++;
                    g_unk0x0053a314[g_unk0x0053c9a0] = *p;
                    g_unk0x0053c9a0++;
                    pCar->field_0xb64 = 0;
                    goto nextCar;
                }
            } else {
nextCar:
                if (pCar->simulationStepsRemaining > 0) {
                    g_unk0x0053a270[g_carOrder[44]] = *p;
                    g_carOrder[44]++;
                }
            }
            if (pCar->field_0xc18 != 0 && pCar->simulationStepsRemaining > 0) {
                g_unk0x0053bd6c[g_carOrder[25]] = *p;
                g_carOrder[25]++;
            }
            j++;
            p++;
        } while (j < g_carOrderCount);
    }
    i = 0;
    if (g_unk0x0053a310 > 0) {
        p = g_unk0x0053b4f0;
        do {
            Car_MarkHiddenFromCameras(g_carBuffer + *p);
            i++;
            p++;
        } while (i < g_unk0x0053a310);
    }
}

unsigned char RallyData_GetSelectionFlag26(void);
BYTE *StageUI_GetRaceResultTable(void);

// Marks which player camera records are too far from, or face away from, the
// car. The fourth word is set when every active camera has been rejected.
// match 43%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00428a00
void Car_MarkHiddenFromCameras(Car *pCar)
{
    int *pRejected;
    FixMatrix *pView;
    FixVector position;
    FixVector forward;
    FixVector delta;
    int count;
    int rejected;
    int i;

    pRejected = &pCar->firstCameraBlocked;
    pCar->field_0xb70 = 0;
    pCar->field_0xb6c = 0;
    *pRejected = 0;
    if ((char)RallyData_GetSelectionFlag26() != 0 && (char)((CallbackStateMachine *)StageUI_GetRaceResultTable())->records[0].packedState != '\n' &&
        (char)((CallbackStateMachine *)StageUI_GetRaceResultTable())->records[0].packedState != '\t' && (char)((CallbackStateMachine *)StageUI_GetRaceResultTable())->records[0].packedState != '\r') {
        rejected = 0;
        count = RallyDataState() & 0xff;
        pView = &g_viewStates[0].matrix;
        for (i = 0; i < count; i++, pView = (FixMatrix *)((BYTE *)pView + 100)) {
            FixMatrix_GetPosition(&position, pView);
            FixMatrix_GetForward(&forward, pView);
            if (*pRejected == 0) {
                delta.x = pCar->position.x - position.x;
                delta.y = pCar->position.y - position.y;
                delta.z = pCar->position.z - position.z;
                if (FIX_ABS(delta.x) > 0x3c0000 || FIX_ABS(delta.y) > 0x3c0000 ||
                    FIX_ABS(delta.z) > 0x3c0000)
                    *pRejected = 1;
                else if (FixVecDot(&delta, &delta) > 0xe100000)
                    *pRejected = 1;
                if (*pRejected == 0 && FixVecDot(&forward, &delta) < 0)
                    *pRejected = 1;
            }
            if (*pRejected != 0)
                rejected++;
            pRejected++;
        }
        if (rejected == count)
            pCar->field_0xb70 = 1;
    }
}

unsigned char RallyData_GetSelectionFlag26(void);
int InRaceMenu_IsPlayerEditingCarSetup(unsigned int param1);
void StageObject_SetCarValuePendingFlag(int index, int reset);
extern float g_65536f;
bool Graphics_AllCarsFullDetail(void);

// Picks the up to three cars to display in one split-screen view (nearest to
// the view position, in or near the viewport) and hides the remaining ones.
// match 59%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00428bf0
void View_SelectVisibleCars(unsigned int view, short *pRect)
{
    char visible[8];
    int i;
    int j;
    int k;
    int car;
    int bestIdx;
    int bestDist;
    SceneNode *pView;
    FixMatrix *pCurrent;
    FixVector pos;
    FixVector otherPos;
    FixVector delta;
    FixVector up;
    int distTable[8];
    FixMatrix savedWorld;
    int proj1[2];
    int proj2[2];
    int ix;
    int iy;
    int xx;
    int yy;
    int extend;
    float ftmp;
    int sx;
    int sy;

    // PORT: Graphics_AllCarsFullDetail (record flag 0x10 in the original):
    // there is no simpler body to switch the distant cars to, so every car
    // keeps its own, also after a race that had them.
    if (Graphics_AllCarsFullDetail()) {
        for (i = 0; i < g_carOrderCount; i++)
            StageObject_SetCarValuePendingFlag(g_carOrder[i], 1);
        return;
    }
    if ((char)RallyData_GetSelectionFlag26() && RallyData_IsHeadToHeadRaceMode() != 0) {
        StageObject_SetCarValuePendingFlag(0, 1);
        StageObject_SetCarValuePendingFlag(1, 1);
        return;
    }
    if (!((char)RallyData_GetSelectionFlag26() || CGameInfo::GetConfiguredGameMode() == 8 || CGameInfo::GetConfiguredGameMode() == 9 ||
          CGameInfo::GetConfiguredGameMode() == 10 || CGameInfo::GetConfiguredGameMode() == 11 ||
          CGameInfo::GetConfiguredGameMode() == 12 || CGameInfo::GetConfiguredGameMode() == 8))
        return;

    i = g_carOrderCount;
    if (i > 0) {
        short *p = g_carOrder;
        do {
            visible[*p++] = 0;
        } while (--i);
    }

    pView = g_viewNodes[view & 0xff];
    pCurrent = &pView->current;
    FixMatrix_GetPosition(&pos, pCurrent);

    i = 0;
    if (g_carOrderCount > 0) {
        do {
            FixMatrix_GetPosition(&otherPos, &Car_Get(g_carOrder[i])->pSceneRoot->current);
            delta.x = pos.x - otherPos.x;
            delta.y = pos.y - otherPos.y;
            delta.z = pos.z - otherPos.z;
            if (FIX_ABS(delta.x) <= 0x960000 && FIX_ABS(delta.y) <= 0x960000 && FIX_ABS(delta.z) <= 0x960000)
                distTable[g_carOrder[i]] = FixVecDot(&delta, &delta);
            else
                distTable[g_carOrder[i]] = 0x7fbc0000;
            i++;
        } while (i < g_carOrderCount);
    }

    FixMatrix_GetUp(&up, pCurrent);
    FIX_NORMALIZE_INTO(up, up)

    savedWorld = pView->world;
    pView->world = pView->current;

    j = 1;
    if (g_carOrderCount > 1) {
        do {
            FixMatrix_GetPosition(&otherPos, &Car_Get(g_carOrder[j])->pSceneRoot->current);
            FixMatrix_ProjectWorldPointToView(proj1, &otherPos, pView);
            proj1[0] >>= 16;
            proj1[1] >>= 16;
            FixVecScale(&delta, &up, Car_Get(g_carOrder[j])->collisionRadius);
            delta.x += otherPos.x;
            delta.y += otherPos.y;
            delta.z += otherPos.z;
            FixMatrix_ProjectWorldPointToView(proj2, &delta, pView);
            proj2[0] >>= 16;
            proj2[1] >>= 16;
            if ((proj1[0] == -100 && proj1[1] == -100) || (proj2[0] == -100 && proj2[1] == -100)) {
                distTable[g_carOrder[j]] = 0x7fbc0000;
            } else {
                ftmp = X87S_DIV(X87S_FILD(float, (proj1[0] - proj2[0])), X87S_FILD(float, (int)g_pGraphics->resX));
                ix = FTOI(int, ftmp * g_65536f);
                ftmp = X87S_DIV(X87S_FILD(float, (proj1[1] - proj2[1])), X87S_FILD(float, (int)g_pGraphics->resY));
                iy = FTOI(int, ftmp * g_65536f);
                xx = FixMul(FixMul(ix, 0x2800000), 0x28f);
                yy = FixMul(iy, 0x1e00000);
                yy = FixMul(yy, 0x28f);
                extend = FixMul(FixSqrt(FixMul(xx, xx) + FixMul(yy, yy)), 0x4b0000) >> 16;
                sx = (int)g_pGraphics->resX * extend / 0x280;
                sy = (int)g_pGraphics->resY * extend / 0x1e0;
                if (proj1[0] < (short)(pRect[0] - sx) || proj1[0] > (short)(pRect[0] + pRect[2] + sx) ||
                    proj1[1] < (short)(pRect[1] - sy) || proj1[1] > (short)(pRect[1] + pRect[3] + sy))
                    distTable[g_carOrder[j]] = 0x7fbc0000;
            }
            j++;
        } while (j < g_carOrderCount);
    }
    pView->world = savedWorld;

    for (k = 0; k < 3; k++) {
        bestIdx = -1;
        bestDist = 0x7fff0000;
        if (g_carOrderCount >= 1) {
            for (i = 0; i < g_carOrderCount; i++) {
                car = g_carOrder[i];
                if (visible[car] == 0) {
                    if (car == 0) {
                        visible[car] = 2;
                        break;
                    }
                    if (distTable[car] < bestDist) {
                        bestIdx = car;
                        bestDist = distTable[car];
                    }
                }
            }
            if (bestIdx != -1) {
                if (bestDist > 0xe100000)
                    break;
                visible[bestIdx] = 2;
            }
        }
    }

    i = 0;
    if (g_carOrderCount > 0) {
        do {
            StageObject_SetCarValuePendingFlag(g_carOrder[i], visible[g_carOrder[i]] == 2);
            i++;
        } while (i < g_carOrderCount);
    }
}

// FUNCTION: CMR2 0x0042b5f0
Car *Car_Get(int index)
{
    return g_cars[index];
}

// Dampens the car's secondary body motion while this mode is active.
// FUNCTION: CMR2 0x00437d70
void Car_DampenBodyMotion(void)
{
    if (g_pCurrentCar->field_0xb28 == 0 && g_pCurrentCar->field_0xb74 != 0) {
        int change = g_pCurrentCar->engineSpeed - g_pCurrentCar->field_0x7a8;
        g_pCurrentCar->field_0x5d0.z -= FixMul(0x1999, change);
    }
}

// Road speed of a wheel from its spin: raw (unit 2), in km/h (0) or mph (1).
// FUNCTION: CMR2 0x0042b600
int Car_GetWheelSpeed(Car *pCar, BYTE wheel, int unit)
{
    int speed;

    speed = FixMul(pCar->wheelLoad[wheel], 0x4000);
    switch (unit) {
    case 0:
        return FixMul(speed, 0x431168);
    case 1:
        return FixMul(speed, 0x6bfa9f);
    }
    return speed;
}

#define ADD_POSITION(p, pos)    \
    (p)->x += (pos).x;          \
    (p)->y += (pos).y;          \
    (p)->z += (pos).z;          \
    (p)++;

// Selects the per-gear speed table of the current car (eight 16.16 values at
// 0x7dc) and rescales the engine-speed accumulator (0x788) accordingly, then
// normalises the table by 0x123d7.
// FUNCTION: CMR2 0x0043e1f0
void Car_SelectGearSpeedTable(unsigned int param_1)
{
    int i;
    int v;

    switch (param_1) {
    case 4:
        g_pCurrentCar->gearSpeed[0] = 0;
        g_pCurrentCar->gearSpeed[1] = 0x553f;
        g_pCurrentCar->gearSpeed[2] = 0x7893;
        g_pCurrentCar->gearSpeed[3] = 0x9ef9;
        g_pCurrentCar->gearSpeed[4] = 0xc28f;
        g_pCurrentCar->gearSpeed[5] = 0xe4dd;
        g_pCurrentCar->gearSpeed[6] = 0x10ac0;
        g_pCurrentCar->gearSpeed[7] = 0xffffaac1;
        break;
    case 3:
        g_pCurrentCar->gearSpeed[0] = 0;
        g_pCurrentCar->gearSpeed[1] = 0x5374;
        g_pCurrentCar->gearSpeed[2] = 0x73b6;
        g_pCurrentCar->gearSpeed[3] = 0x9604;
        g_pCurrentCar->gearSpeed[4] = 0xb4fd;
        g_pCurrentCar->gearSpeed[5] = 0xd26e;
        g_pCurrentCar->gearSpeed[6] = 0xf26e;
        g_pCurrentCar->gearSpeed[7] = 0xffffac8c;
        g_pCurrentCar->maxThrottleTorque = g_pCurrentCar->maxThrottleTorque + 0x1eb;
        break;
    case 2:
        g_pCurrentCar->gearSpeed[0] = 0;
        g_pCurrentCar->gearSpeed[1] = 0x5168;
        g_pCurrentCar->gearSpeed[2] = 0x6ed9;
        g_pCurrentCar->gearSpeed[3] = 0x8d0e;
        g_pCurrentCar->gearSpeed[4] = 0xa76c;
        g_pCurrentCar->gearSpeed[5] = 0xc000;
        g_pCurrentCar->gearSpeed[6] = 0xd9db;
        g_pCurrentCar->gearSpeed[7] = 0xffffae98;
        g_pCurrentCar->maxThrottleTorque = g_pCurrentCar->maxThrottleTorque + 0x3d7;
        break;
    case 1:
        g_pCurrentCar->gearSpeed[0] = 0;
        g_pCurrentCar->gearSpeed[1] = 0x4f9d;
        g_pCurrentCar->gearSpeed[2] = 0x69fb;
        g_pCurrentCar->gearSpeed[3] = 0x83d7;
        g_pCurrentCar->gearSpeed[4] = 0x9999;
        g_pCurrentCar->gearSpeed[5] = 0xad4f;
        g_pCurrentCar->gearSpeed[6] = 0xc189;
        g_pCurrentCar->gearSpeed[7] = 0xffffb063;
        g_pCurrentCar->maxThrottleTorque = g_pCurrentCar->maxThrottleTorque + 0x5c2;
        break;
    case 0:
        g_pCurrentCar->gearSpeed[0] = 0;
        g_pCurrentCar->gearSpeed[1] = 0x4d91;
        g_pCurrentCar->gearSpeed[2] = 0x651e;
        g_pCurrentCar->gearSpeed[3] = 0x7ae1;
        g_pCurrentCar->gearSpeed[4] = 0x8c08;
        g_pCurrentCar->gearSpeed[5] = 0x9ae1;
        g_pCurrentCar->gearSpeed[6] = 0xa8f5;
        g_pCurrentCar->gearSpeed[7] = 0xffffb26f;
        g_pCurrentCar->maxThrottleTorque = g_pCurrentCar->maxThrottleTorque + 0x7ae;
        break;
    }
    for (i = 0; i < 8; i++) {
        v = g_pCurrentCar->gearSpeed[i];
        g_pCurrentCar->gearSpeed[i] = FixDiv(v, 0x123d7);
    }
}

// Sets three handling factors of the current car from a 16.16 level.
// FUNCTION: CMR2 0x0043e530
void Car_SetSteeringSensitivity(int level)
{
    g_pCurrentCar->steeringForceScale = FixMul(0xcccc, level) + 0x9999;
    g_pCurrentCar->steeringOutwardRate = FixMul(0xa3d, level) + 0xccc;
    g_pCurrentCar->steeringCentringRate = FixMul(0x3333, level) + 0x1999;
}

CarSceneRecord *StageTiming_GetStartTableRecord(int index);
SceneNode *SceneNode_FindByType(SceneNode *pNode, unsigned int type);
int SceneNode_Reparent(SceneNode *pNode, SceneNode *pNewParent);
SceneNode *RallyData_GetChallengeRenderState(void);
void Car_SwapWheelTextures(char mode, SceneNode **pWheels);

// Binds a car to its loaded model: body nodes, wheels and view nodes, and
// moves both body nodes under the stage root.
// FUNCTION: CMR2 0x0043e5a0
void Car_BindModel(int model, Car *pCar)
{
    CarSceneRecord *pModel = StageTiming_GetStartTableRecord(model);
    SceneNode *pBody = pModel->rootNode;
    SceneNode *pBody2 = pModel->bodyNode;

    pCar->pSceneRoot = pBody;
    pCar->pBodyNode = pBody2;
    pCar->pWheelNodes[0] = SceneNode_FindByType(pBody, 1);
    pCar->pWheelNodes[1] = SceneNode_FindByType(pBody, 2);
    pCar->pWheelNodes[2] = SceneNode_FindByType(pBody, 3);
    pCar->pWheelNodes[3] = SceneNode_FindByType(pBody, 4);
    pCar->pViewNodeFar = SceneNode_FindByType(pBody2, 0xe);
    pCar->pViewNodeNear = SceneNode_FindByType(pBody2, 0xf);
    Car_SwapWheelTextures(StageTiming_GetStartTableRecord(pCar->index)->detailCode, pCar->pWheelNodes);
    if (pBody->pParent != RallyData_GetChallengeRenderState())
        SceneNode_Reparent(pBody, RallyData_GetChallengeRenderState());
    if (pCar->pBodyNode->pParent != RallyData_GetChallengeRenderState())
        SceneNode_Reparent(pCar->pBodyNode, RallyData_GetChallengeRenderState());
}

// Recomputes the eight world-space corners of the car's box from its
// half extents and world matrix, then applies the suspension offsets.
// match 76%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0043eef0
void Car_UpdateCorners(Car *pCar)
{
    int hx;
    int hy;
    int hz;
    FixMatrix *pM;
    int ax;
    int ay;
    int az;
    FixVector *p;

    hx = pCar->halfExtents.x;
    pM = pCar->pWorld;
    hy = pCar->halfExtents.y;
    hz = pCar->halfExtents.z;
    g_pCurrentCar = pCar;

    ax = FixMul(pM->right.x, hx);
    ay = FixMul(pM->up.x, hy);
    az = FixMul(pM->forward.x, hz);
    pCar->corners[4].x = az + ay + ax;
    pCar->corners[5].x = (ay - az) + ax;
    pCar->corners[7].x = (ay - az) - ax;
    pCar->corners[6].x = (az - ax) + ay;

    ax = FixMul(pM->right.y, hx);
    ay = FixMul(pM->up.y, hy);
    az = FixMul(pM->forward.y, hz);
    pCar->corners[4].y = az + ay + ax;
    pCar->corners[5].y = (ay - az) + ax;
    pCar->corners[7].y = (ay - az) - ax;
    pCar->corners[6].y = (az - ax) + ay;

    ax = FixMul(pM->right.z, hx);
    ay = FixMul(pM->up.z, hy);
    az = FixMul(pM->forward.z, hz);
    pCar->corners[4].z = az + ay + ax;
    pCar->corners[5].z = (ay - az) + ax;
    pCar->corners[7].z = (ay - az) - ax;
    pCar->corners[6].z = (az - ax) + ay;

    pCar->corners[0].x = -pCar->corners[7].x;
    pCar->corners[1].x = -pCar->corners[6].x;
    pCar->corners[2].x = -pCar->corners[5].x;
    pCar->corners[3].x = -pCar->corners[4].x;
    pCar->corners[0].y = -pCar->corners[7].y;
    pCar->corners[1].y = -pCar->corners[6].y;
    pCar->corners[2].y = -pCar->corners[5].y;
    pCar->corners[3].y = -pCar->corners[4].y;
    pCar->corners[0].z = -pCar->corners[7].z;
    pCar->corners[1].z = -pCar->corners[6].z;
    pCar->corners[2].z = -pCar->corners[5].z;
    pCar->corners[3].z = -pCar->corners[4].z;

    p = pCar->corners;
    ADD_POSITION(p, pCar->position)
    ADD_POSITION(p, pCar->position)
    ADD_POSITION(p, pCar->position)
    ADD_POSITION(p, pCar->position)
    ADD_POSITION(p, pCar->position)
    ADD_POSITION(p, pCar->position)
    ADD_POSITION(p, pCar->position)
    ADD_POSITION(p, pCar->position)

    if (g_pCurrentCar->field_0xb64 == 0)
        Car_ApplyCornerOffsets();
}

// Shifts the four upper corners of g_pCurrentCar by the scaled body
// offset vectors (suspension travel).
// FUNCTION: CMR2 0x0043f280
void Car_ApplyCornerOffsets(void)
{
    FixVector v[3];

    FixVecScale(&v[0], &g_pCurrentCar->right, g_pCurrentCar->scale0x764);
    FixVecScale(&v[1], &g_pCurrentCar->forward, g_pCurrentCar->scale0x76c);
    FixVecScale(&v[2], &g_pCurrentCar->right, g_pCurrentCar->scale0x768);
    g_pCurrentCar->corners[4].x -= v[0].x;
    g_pCurrentCar->corners[4].y -= v[0].y;
    g_pCurrentCar->corners[4].z -= v[0].z;
    g_pCurrentCar->corners[4].x -= v[1].x;
    g_pCurrentCar->corners[4].y -= v[1].y;
    g_pCurrentCar->corners[4].z -= v[1].z;
    g_pCurrentCar->corners[5].x -= v[0].x;
    g_pCurrentCar->corners[5].y -= v[0].y;
    g_pCurrentCar->corners[5].z -= v[0].z;
    g_pCurrentCar->corners[5].x += v[1].x;
    g_pCurrentCar->corners[5].y += v[1].y;
    g_pCurrentCar->corners[5].z += v[1].z;
    g_pCurrentCar->corners[6].x += v[2].x;
    g_pCurrentCar->corners[6].y += v[2].y;
    g_pCurrentCar->corners[6].z += v[2].z;
    g_pCurrentCar->corners[6].x -= v[1].x;
    g_pCurrentCar->corners[6].y -= v[1].y;
    g_pCurrentCar->corners[6].z -= v[1].z;
    g_pCurrentCar->corners[7].x += v[2].x;
    g_pCurrentCar->corners[7].y += v[2].y;
    g_pCurrentCar->corners[7].z += v[2].z;
    g_pCurrentCar->corners[7].x += v[1].x;
    g_pCurrentCar->corners[7].y += v[1].y;
    g_pCurrentCar->corners[7].z += v[1].z;
}

// Applies the stored transforms of every car to its scene nodes for the
// given view. Cars closer than 2.0 to the view get the transforms as they
// are; farther cars are pulled 1.5 units towards the view and their axes
// scaled by (1 - 1.5 / distance), which is stored in g_carViewScale.
// FUNCTION: CMR2 0x004292c0
void Car_ApplyViewTransforms(int viewIndex)
{
    FixVector viewPos;
    FixVector delta;
    FixVector bodyPos;
    FixVector axis;
    int pull;
    int n;
    int i;
    short *pOrder;
    int carIndex;
    int wheel;
    int scale;
    int len;
    CarTransforms *pT;

    pull = 0x18000;
    FixMatrix_GetPosition(&viewPos, &g_viewNodes[viewIndex]->current);
    i = g_carOrderCount - 1;
    if (i >= 0) {
        pOrder = &g_carOrder[i];
        n = i + 1;
        do {
            scale = 0x10000;
            carIndex = *pOrder;
            pT = &g_carTransforms[carIndex];
            i = carIndex;
            FixMatrix_GetPosition(&bodyPos, &pT->body);
            for (wheel = 0; wheel < 4; wheel++) {
                FixMatrix_CopyRotation(&g_carWheelTransforms[carIndex][wheel],
                                       &g_carBuffer[carIndex].pWheelNodes[wheel]->current);
            }
            delta.x = viewPos.x - bodyPos.x;
            delta.y = viewPos.y - bodyPos.y;
            delta.z = viewPos.z - bodyPos.z;
            len = FixVecLength(&delta);
            if (len > 0x20000) {
                if (len != 0) {
                    scale = 0x10000 - FixDiv(pull, len);
                    FixVecScaleRecip(&delta, &delta, len);
                    FixVecScale(&delta, &delta, pull);
                    bodyPos.x += delta.x;
                    bodyPos.y += delta.y;
                    bodyPos.z += delta.z;
                    FixMatrix_SetPosition(&bodyPos, &g_carBuffer[carIndex].pSceneRoot->current);
                    FixMatrix_GetRight(&axis, &pT->body);
                    FixVecScale(&axis, &axis, scale);
                    FixMatrix_SetRight(&axis, &g_carBuffer[carIndex].pSceneRoot->current);
                    FixMatrix_GetUp(&axis, &pT->body);
                    FixVecScale(&axis, &axis, scale);
                    FixMatrix_SetUp(&axis, &g_carBuffer[carIndex].pSceneRoot->current);
                    FixMatrix_GetForward(&axis, &pT->body);
                    FixVecScale(&axis, &axis, scale);
                    FixMatrix_SetForward(&axis, &g_carBuffer[carIndex].pSceneRoot->current);
                    FixMatrix_SetPosition(&bodyPos, &g_carBuffer[carIndex].pBodyNode->current);
                    FixMatrix_GetRight(&axis, &pT->body2);
                    FixVecScale(&axis, &axis, scale);
                    FixMatrix_SetRight(&axis, &g_carBuffer[carIndex].pBodyNode->current);
                    FixMatrix_GetUp(&axis, &pT->body2);
                    FixVecScale(&axis, &axis, scale);
                    FixMatrix_SetUp(&axis, &g_carBuffer[carIndex].pBodyNode->current);
                    FixMatrix_GetForward(&axis, &pT->body2);
                    FixVecScale(&axis, &axis, scale);
                    FixMatrix_SetForward(&axis, &g_carBuffer[carIndex].pBodyNode->current);
                    carIndex = i;
                }
            } else {
                FixMatrix_CopyRotation(&pT->body, &g_carBuffer[carIndex].pSceneRoot->current);
                FixMatrix_CopyRotation(&pT->body2, &g_carBuffer[carIndex].pBodyNode->current);
            }
            g_carViewScale[carIndex][viewIndex] = scale;
            pOrder--;
            n--;
        } while (n != 0);
    }
}

#define SCALE_NODE_AXES(pNode, s)                                            \
    (pNode)->current.right.x = FixMul((pNode)->local.right.x, s);            \
    (pNode)->current.right.y = FixMul((pNode)->local.right.y, s);            \
    (pNode)->current.right.z = FixMul((pNode)->local.right.z, s);            \
    (pNode)->current.up.x = FixMul((pNode)->local.up.x, s);                  \
    (pNode)->current.up.y = FixMul((pNode)->local.up.y, s);                  \
    (pNode)->current.up.z = FixMul((pNode)->local.up.z, s);                  \
    (pNode)->current.forward.x = FixMul((pNode)->local.forward.x, s);        \
    (pNode)->current.forward.y = FixMul((pNode)->local.forward.y, s);        \
    (pNode)->current.forward.z = FixMul((pNode)->local.forward.z, s);        \
    (pNode)->useParentWorld = 0;                                             \
    for (p = (pNode); p != NULL; p = p->pParent)                             \
        p->dirty = 1;

// Places the two view-dependent child nodes of every car (0x748 in front of
// the body towards the view, 0x74c behind it) along the car -> view
// direction, scaled with the distance.
// match 75%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00429810
void Car_UpdateViewNodes(int viewIndex)
{
    int len;
    short *pOrder;
    int nearDist;
    int n;
    int s;
    int i;
    int farDist;
    FixVector viewPos;
    FixVector pos;
    FixVector local;
    FixVector carPos;
    FixVector delta;
    Car *pCar;
    SceneNode *p;

    nearDist = 0x1999;
    farDist = 0x8000;
    FixMatrix_GetPosition(&pos, &g_viewNodes[viewIndex]->current);
    i = g_carOrderCount - 1;
    if (i >= 0) {
        n = i + 1;
        pOrder = &g_carOrder[i];
        do {
            pCar = Car_Get(*pOrder);
            if (pCar->pViewNodeNear != NULL || pCar->pViewNodeFar != NULL) {
                FixMatrix_GetPosition(&carPos, &pCar->pBodyNode->current);
                delta.x = carPos.x - pos.x;
                delta.y = carPos.y - pos.y;
                delta.z = carPos.z - pos.z;
                if (FIX_ABS(delta.x) <= 0x800000 && FIX_ABS(delta.y) <= 0x800000 && FIX_ABS(delta.z) <= 0x800000) {
                    len = FixVecLength(&delta);
                    if (len == 0)
                        goto next;
                    FixVecScaleRecip(&delta, &delta, len);
                } else {
                    delta.x /= 128;
                    delta.y /= 128;
                    delta.z /= 128;
                    len = FixVecLength(&delta);
                    FixVecScaleRecip(&delta, &delta, len);
                    len <<= 7;
                }
                if (len != 0) {
                    FixMatrix_InverseRotateVector(&local, &delta, &pCar->pBodyNode->current);
                    if (pCar->pViewNodeNear != NULL) {
                        FixVecScale(&viewPos, &local, nearDist);
                        FixMatrix_SetPosition(&viewPos, &(pCar->pViewNodeNear)->current);
                        s = FixDiv(nearDist, len) + 0x10000;
                        SCALE_NODE_AXES(pCar->pViewNodeNear, s)
                    }
                    if (pCar->pViewNodeFar != NULL) {
                        FixVecScale(&viewPos, &local, -farDist);
                        FixMatrix_SetPosition(&viewPos, &(pCar->pViewNodeFar)->current);
                        s = 0x10000 - FixDiv(farDist, len);
                        SCALE_NODE_AXES(pCar->pViewNodeFar, s)
                    }
                }
            }
next:
            n--;
            pOrder--;
        } while (n != 0);
    }
}

// Physics time scale (16.16, 1.0 in the shipped data).
int g_physicsTimeStep = 0x10000;

// Updates the body axes of g_pCurrentCar: the up vector is pulled towards
// the wheel plane and the velocity, forward/right are re-orthogonalised,
// then a fraction of the velocity is turned into lateral force and damped.
// match 88%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00436630
void Car_UpdateBodyAxes(void)
{
    int step;
    int damping;
    int maxStep;
    FixVector localUp;
    FixVector world;
    FixVector diff;

    if (g_pCurrentCar->field_0xb28 > 0 && g_pCurrentCar->field_0xb74 != 0) {
        damping = FixMul(0x624, g_physicsTimeStep);
        maxStep = FixMul(0x312, g_physicsTimeStep);
        step = FixMul(g_pCurrentCar->angularVelocity.y, 0x3e80000);
        step = step - FixMul(step, damping);
        g_pCurrentCar->angularVelocity.y = FixMul(step, 0x41);

        if (FIX_ABS(g_pCurrentCar->speed) < 0x28f) {
            localUp.x = 0;
            localUp.y = 0;
            localUp.z = 0;
        } else {
            localUp.x = g_sinTable[(unsigned short)(g_pCurrentCar->wheelSteeringAngle + 0x400) & 0xfff];
            localUp.y = 0;
            localUp.z = -g_sinTable[g_pCurrentCar->wheelSteeringAngle & 0xfff];
        }
        FixMatrix_RotateVector(&world, &localUp, g_pCurrentCar->pWorld);
        if (FixVecDot(&g_pCurrentCar->velocity, &g_pCurrentCar->right) >= 0)
            FixVecScale(&world, &world, FixMul(g_pCurrentCar->speed, 0x2604));
        else
            FixVecScale(&world, &world, -FixMul(g_pCurrentCar->speed, 0x2604));

        localUp.x = g_pCurrentCar->right.x + world.x;
        localUp.y = g_pCurrentCar->right.y + world.y;
        localUp.z = g_pCurrentCar->right.z + world.z;
        FIX_NORMALIZE_INTO(localUp, localUp)
        diff.x = localUp.x - g_pCurrentCar->right.x;
        diff.y = localUp.y - g_pCurrentCar->right.y;
        diff.z = localUp.z - g_pCurrentCar->right.z;
        {
            FixVector *pUp;
            step = FixVecLength(&diff);
            if (step < maxStep) {
                g_pCurrentCar->right = localUp;
            } else {
                FixVecScaleRecip(&localUp, &diff, step);
                FixVecScale(&localUp, &localUp, maxStep);
                localUp.x = localUp.x + g_pCurrentCar->right.x;
                localUp.y = localUp.y + g_pCurrentCar->right.y;
                localUp.z = localUp.z + g_pCurrentCar->right.z;
                pUp = &g_pCurrentCar->right;
                FIX_NORMALIZE_INTO((*pUp), localUp)
            }
        }

        {
            FixVector *pForward;
            step = FixVecDot(&g_pCurrentCar->right, &g_pCurrentCar->forward);
            FixVecScale(&localUp, &g_pCurrentCar->right, step);
            localUp.x = g_pCurrentCar->forward.x - localUp.x;
            localUp.y = g_pCurrentCar->forward.y - localUp.y;
            localUp.z = g_pCurrentCar->forward.z - localUp.z;
            pForward = &g_pCurrentCar->forward;
            FIX_NORMALIZE_INTO((*pForward), localUp)
        }

        {
            FixVector *pRight;
            FixVecCross(&localUp, &g_pCurrentCar->forward, &g_pCurrentCar->right);
            pRight = &g_pCurrentCar->up;
            FIX_NORMALIZE_INTO((*pRight), localUp)
        }

        {
            step = FixVecDot(&g_pCurrentCar->velocity, &g_pCurrentCar->up);
            FixVecScale(&localUp, &g_pCurrentCar->up, step);
            localUp.x = g_pCurrentCar->velocity.x - localUp.x;
            localUp.y = g_pCurrentCar->velocity.y - localUp.y;
            localUp.z = g_pCurrentCar->velocity.z - localUp.z;
        }
        if (FixVecLength(&localUp) > 0) {
            int s = FixMul(FixMul(0x1e0000, g_pCurrentCar->speed), g_pCurrentCar->steeringAccumulator);
            if (g_pCurrentCar->handbrake != 0)
                s += FixMul(s, 0x20000);
            FixVecScale(&localUp, &g_pCurrentCar->forward, s);
            g_pCurrentCar->cornerForce[0].x += localUp.x;
            g_pCurrentCar->cornerForce[0].y += localUp.y;
            g_pCurrentCar->cornerForce[0].z += localUp.z;
            g_pCurrentCar->cornerForce[1].x += localUp.x;
            g_pCurrentCar->cornerForce[1].y += localUp.y;
            g_pCurrentCar->cornerForce[1].z += localUp.z;
        }

        localUp = g_pCurrentCar->velocity;
        localUp.y = 0;
        step = FixVecLength(&localUp);
        if (step > 0) {
            {
                int d;
                FixVecScaleRecip(&localUp, &localUp, step);
                d = FixVecDot(&localUp, &g_pCurrentCar->groundNormal);
                step = FixVecDot(&localUp, &g_pCurrentCar->normal0x498) - d;
                if (step > 0x41) {
                    step = FixMul(step, g_pCurrentCar->field_0x9c4);
                    if (step > 0x10000)
                        step = 0x10000;
                    step = 0x10000 - step;
                    FixVecScale(&g_pCurrentCar->velocity, &g_pCurrentCar->velocity, step);
                }
            }
        }
    }
}

// Resets the current wheels' loads to the engine torque (front, rear or all
// four, by the drive split) after a gear change.
// FUNCTION: CMR2 0x0043f570
void Car_ResetWheelLoadsAfterShift(Car *pCar)
{
    int load;

    pCar->gear = 1;
    if (pCar->driveSplit == 0) {
        load = FixMul(pCar->engineSpeed, pCar->gearSpeed[1]);
        pCar->wheelLoad[3] = load;
        pCar->wheelLoad[2] = load;
    } else {
        if (pCar->driveSplit == 0x10000) {
            load = FixMul(pCar->engineSpeed, pCar->gearSpeed[1]);
        } else {
            load = FixMul(pCar->engineSpeed, pCar->gearSpeed[1]);
            pCar->wheelLoad[3] = load;
            pCar->wheelLoad[2] = load;
        }
        pCar->wheelLoad[1] = load;
        pCar->wheelLoad[0] = load;
    }
    pCar->field_0xb1f = 10;
    g_pCurrentCar->field_0x7ac = g_pCurrentCar->engineSpeed;
}

// Marks the four wheel corners grounded and initializes their quarter-mass loads.
// FUNCTION: CMR2 0x0043fb50
void Car_SetupDriveTrain(void)
{
    int i;

    for (i = 0; i < 4; i++) {
        g_pCurrentCar->cornerOnGround[i] = 1;
        g_pCurrentCar->field_0xb28++;
    }
    g_pCurrentCar->field_0xb28 = 4;
    g_pCurrentCar->tyreGrip = g_pCurrentCar->mass / 4;
    for (i = 3; i >= 0; i--)
        g_pCurrentCar->cornerWeight[i] = g_pCurrentCar->tyreGrip;
}

extern FixVector g_carStepAccel;

// Sets the current car's corner mass and the per-corner spring rates from its
// mass and suspension lengths.
// FUNCTION: CMR2 0x0043fbd0
void Car_SetupSuspensionRates(void)
{
    int k;
    int i;

    g_pCurrentCar->cornerMass = g_pCurrentCar->mass / 4;
    g_carStepAccel.x = 0;
    g_carStepAccel.y = 0;
    g_carStepAccel.z = 0;
    k = FixMul(g_pCurrentCar->cornerMass + 0x1e0000, 0x8000);
    g_pCurrentCar->field_0xa5c[0] = FixMul(k, g_pCurrentCar->wheelSurfaceFx[0].drag + g_pCurrentCar->cornerGrip[0].grip2A);
    g_pCurrentCar->field_0xa4c[0] = FixMul(k, g_pCurrentCar->wheelSurfaceFx[0].drag + g_pCurrentCar->cornerGrip[0].gripA);
    for (i = 1; i < 4; i++) {
        g_pCurrentCar->field_0xa5c[i] = g_pCurrentCar->field_0xa5c[0];
        g_pCurrentCar->field_0xa4c[i] = g_pCurrentCar->field_0xa4c[0];
    }
}

// Same as Car_UpdateBodyAxes without the final velocity damping.
// FUNCTION: CMR2 0x0043fcc0
void Car_UpdateBodyAxesNoDamping(void)
{
    int step;
    int damping;
    int maxStep;
    FixVector localUp;
    FixVector world;
    FixVector diff;

    if (g_pCurrentCar->field_0xb28 > 0 && g_pCurrentCar->field_0xb74 != 0) {
        damping = FixMul(0x624, g_physicsTimeStep);
        maxStep = FixMul(0x312, g_physicsTimeStep);
        step = FixMul(g_pCurrentCar->angularVelocity.y, 0x3e80000);
        step = step - FixMul(step, damping);
        g_pCurrentCar->angularVelocity.y = FixMul(step, 0x41);

        if (FIX_ABS(g_pCurrentCar->speed) < 0x28f) {
            localUp.x = 0;
            localUp.y = 0;
            localUp.z = 0;
        } else {
            localUp.x = g_sinTable[(unsigned short)(g_pCurrentCar->wheelSteeringAngle + 0x400) & 0xfff];
            localUp.y = 0;
            localUp.z = -g_sinTable[g_pCurrentCar->wheelSteeringAngle & 0xfff];
        }
        FixMatrix_RotateVector(&world, &localUp, g_pCurrentCar->pWorld);
        if (FixVecDot(&g_pCurrentCar->velocity, &g_pCurrentCar->right) >= 0)
            FixVecScale(&world, &world, FixMul(g_pCurrentCar->speed, 0x2604));
        else
            FixVecScale(&world, &world, -FixMul(g_pCurrentCar->speed, 0x2604));

        localUp.x = g_pCurrentCar->right.x + world.x;
        localUp.y = g_pCurrentCar->right.y + world.y;
        localUp.z = g_pCurrentCar->right.z + world.z;
        FIX_NORMALIZE_INTO(localUp, localUp)
        diff.x = localUp.x - g_pCurrentCar->right.x;
        diff.y = localUp.y - g_pCurrentCar->right.y;
        diff.z = localUp.z - g_pCurrentCar->right.z;
        {
            FixVector *pUp;
            step = FixVecLength(&diff);
            if (step < maxStep) {
                g_pCurrentCar->right = localUp;
            } else {
                FixVecScaleRecip(&localUp, &diff, step);
                FixVecScale(&localUp, &localUp, maxStep);
                localUp.x = localUp.x + g_pCurrentCar->right.x;
                localUp.y = localUp.y + g_pCurrentCar->right.y;
                localUp.z = localUp.z + g_pCurrentCar->right.z;
                pUp = &g_pCurrentCar->right;
                FIX_NORMALIZE_INTO((*pUp), localUp)
            }
        }

        {
            FixVector *pForward;
            step = FixVecDot(&g_pCurrentCar->right, &g_pCurrentCar->forward);
            FixVecScale(&localUp, &g_pCurrentCar->right, step);
            localUp.x = g_pCurrentCar->forward.x - localUp.x;
            localUp.y = g_pCurrentCar->forward.y - localUp.y;
            localUp.z = g_pCurrentCar->forward.z - localUp.z;
            pForward = &g_pCurrentCar->forward;
            FIX_NORMALIZE_INTO((*pForward), localUp)
        }

        {
            FixVector *pRight;
            FixVecCross(&localUp, &g_pCurrentCar->forward, &g_pCurrentCar->right);
            pRight = &g_pCurrentCar->up;
            FIX_NORMALIZE_INTO((*pRight), localUp)
        }

        {
            step = FixVecDot(&g_pCurrentCar->velocity, &g_pCurrentCar->up);
            FixVecScale(&localUp, &g_pCurrentCar->up, step);
            localUp.x = g_pCurrentCar->velocity.x - localUp.x;
            localUp.y = g_pCurrentCar->velocity.y - localUp.y;
            localUp.z = g_pCurrentCar->velocity.z - localUp.z;
        }
        if (FixVecLength(&localUp) > 0) {
            int s = FixMul(FixMul(0x1e0000, g_pCurrentCar->speed), g_pCurrentCar->steeringAccumulator);
            if (g_pCurrentCar->handbrake != 0)
                s += FixMul(s, 0x20000);
            FixVecScale(&localUp, &g_pCurrentCar->forward, s);
            g_pCurrentCar->cornerForce[0].x += localUp.x;
            g_pCurrentCar->cornerForce[0].y += localUp.y;
            g_pCurrentCar->cornerForce[0].z += localUp.z;
            g_pCurrentCar->cornerForce[1].x += localUp.x;
            g_pCurrentCar->cornerForce[1].y += localUp.y;
            g_pCurrentCar->cornerForce[1].z += localUp.z;
        }

    }
}

// Copies the body axes and position of g_pCurrentCar into its body matrix
// and rebuilds it from the wheel plane.
// FUNCTION: CMR2 0x00432c20
void Car_StoreBodyMatrix(void)
{
    g_pCurrentCar->pBodyMatrix->position = g_pCurrentCar->position;
    g_pCurrentCar->pBodyMatrix->right = g_pCurrentCar->right;
    g_pCurrentCar->pBodyMatrix->up = g_pCurrentCar->up;
    g_pCurrentCar->pBodyMatrix->forward = g_pCurrentCar->forward;
    Car_UpdateBodyMatrix();
}

// int field of g_pCurrentCar at byte offset off

#define FIX_NORMALIZE_FLIP(v)                                                       \
    {                                                                               \
        int len = FixVecLength(&v);                                                 \
        if (len == 0) {                                                             \
            v.x = 0;                                                                \
            v.y = 0;                                                                \
            v.z = 0;                                                                \
        } else {                                                                    \
            FixVecScaleRecip(&v, &v, len);                                          \
            if (v.y < 0) {                                                          \
                v.y = -v.y;                                                         \
                v.x = -v.x;                                                         \
                v.z = -v.z;                                                         \
            }                                                                       \
        }                                                                           \
    }

// Tilts the body matrix of g_pCurrentCar to the plane through the wheel
// contact points (suspension compressions), keeping the right vector, and
// lifts its position by the mean compression.
// FUNCTION: CMR2 0x00432cc0
void Car_UpdateBodyMatrix(void)
{
    FixVector a;
    FixVector n1;
    FixVector n2;
    FixVector b;
    FixVector world;
    FixVector axes[3];
    int avg[2];
    int comp[4];
    FixVector points[3];
    int i;
    int factor;

    for (i = 0; i < 4; i++) {
        comp[i] = g_pCurrentCar->wheelRideHeight[i] + g_pCurrentCar->wheelSpringOffset[i] + g_pCurrentCar->bodyLeanHeight[i];
        if (CGameInfo::IsActiveCheatEnabled(6) != 0) {
            if (comp[i] < -0x5999)
                comp[i] = -0x5999;
        } else if (comp[i] < -0x1999) {
            comp[i] = -0x1999;
        }
    }
    avg[1] = comp[2] + comp[3];
    avg[1] = FixMul(0x8000, avg[1]);
    avg[0] = comp[0] + comp[1];
    avg[0] = FixMul(0x8000, avg[0]);

    points[0] = g_pCurrentCar->wheelPos[0];
    points[1] = g_pCurrentCar->wheelPos[1];
    points[2] = g_pCurrentCar->wheelPos[2];
    a.x = points[1].x - points[0].x;
    a.y = comp[1] - comp[0];
    a.z = points[1].z - points[0].z;
    b.x = points[2].x - points[0].x;
    b.y = avg[1] - comp[0];
    b.z = -points[0].z;
    FixVecCross(&n1, &a, &b);
    FIX_NORMALIZE_FLIP(n1)

    points[0] = g_pCurrentCar->wheelPos[2];
    points[1] = g_pCurrentCar->wheelPos[3];
    points[2] = g_pCurrentCar->wheelPos[0];
    a.x = points[1].x - points[0].x;
    a.y = comp[3] - comp[2];
    a.z = points[1].z - points[0].z;
    b.x = points[2].x - points[0].x;
    b.y = avg[0] - comp[2];
    b.z = -points[0].z;
    FixVecCross(&n2, &a, &b);
    {
        int len = FixVecLength(&n2);
        if (len == 0) {
            n2.x = 0;
            n2.y = 0;
            n2.z = 0;
        } else {
            FixVecScaleRecip(&n2, &n2, len);
            if (n2.y < 0) {
                n2.x = -n2.x;
                n2.y = -n2.y;
                n2.z = -n2.z;
            }
        }
    }

    a.x = n2.x + n1.x;
    a.y = n2.y + n1.y;
    a.z = n2.z + n1.z;
    FIX_NORMALIZE_INTO(a, a)
    FixMatrix_RotateVector(&world, &a, g_pCurrentCar->pWorld);
    FixMatrix_GetRight(&axes[0], g_pCurrentCar->pBodyMatrix);
    FixMatrix_GetUp(&axes[1], g_pCurrentCar->pBodyMatrix);
    FixMatrix_GetForward(&axes[2], g_pCurrentCar->pBodyMatrix);
    axes[1].x = world.x;
    axes[1].y = world.y;
    axes[1].z = world.z;
    {
        factor = FixVecDot(&axes[0], &axes[1]);
        FixVecScale(&a, &axes[1], factor);
        a.x = axes[0].x - a.x;
        a.y = axes[0].y - a.y;
        a.z = axes[0].z - a.z;
    }
    FIX_NORMALIZE_INTO(axes[0], a)
    FixVecCross(&a, &axes[0], &axes[1]);
    {
        FixVector *pForward = &axes[2];
        int length = FixVecLength(&a);
        if (length == 0) {
            pForward->x = 0;
            pForward->y = 0;
            pForward->z = 0;
        } else {
            FixVecScaleRecip(pForward, &a, length);
        }
    }
    factor = FixMul(0x8000, avg[0] + avg[1]);
    FixVecScale(&a, &g_pCurrentCar->up, factor);
    a.x += g_pCurrentCar->position.x;
    a.y += g_pCurrentCar->position.y;
    a.z += g_pCurrentCar->position.z;
    FixMatrix_SetRight(&axes[0], g_pCurrentCar->pBodyMatrix);
    FixMatrix_SetUp(&axes[1], g_pCurrentCar->pBodyMatrix);
    FixMatrix_SetForward(&axes[2], g_pCurrentCar->pBodyMatrix);
    FixMatrix_SetPosition(&a, g_pCurrentCar->pBodyMatrix);
}

extern CarPartSet *g_physicsPartSet;

// unsigned short field of g_pCurrentCar at byte offset off

// Advances the per-wheel suspension travel angle (0xb08) from the wheel load
// and turns it into the per-wheel contact-point offset (0x6fc/0x700), scaled
// by the per-wheel spring constant of the car setup.
// GLOBAL: CMR2 0x00511398
double g_minusFixedRadiansToAngle12 = -0.009947183943243459;
// FUNCTION: CMR2 0x004336f0
void Car_UpdateWheelTravel(void)
{
    int i;
    int absLoad;
    short delta;

    for (i = 0; i < 4; i++) {
        absLoad = FIX_ABS(g_pCurrentCar->wheelLoad[i]);
        if (absLoad > 0x10000) {
            if (g_pCurrentCar->wheelLoad[i] > 0) {
                g_pCurrentCar->wheelPhase[i] += -0x28b;
            } else {
                g_pCurrentCar->wheelPhase[i] += 0x28b;
            }
        } else {
            delta = (short)FTOI(__int64, X87S_MUL((double)g_pCurrentCar->wheelLoad[i], g_minusFixedRadiansToAngle12));
            g_pCurrentCar->wheelPhase[i] += delta;
        }
        if (g_physicsPartSet->damageValues[i] > 0) {
            g_pCurrentCar->wheelOffset[i][0] =
                FixMul(g_sinTable[g_pCurrentCar->wheelPhase[i] & 0xfff],
                       FixMul(g_physicsPartSet->damageValues[i], 0xa3d));
            g_pCurrentCar->wheelOffset[i][1] =
                FixMul(-g_sinTable[(unsigned short)(g_pCurrentCar->wheelPhase[i] + 0x400) & 0xfff],
                       FixMul(g_physicsPartSet->damageValues[i], 0xa3d));
        } else {
            g_pCurrentCar->wheelOffset[i][1] = 0;
            g_pCurrentCar->wheelOffset[i][0] = 0;
        }
    }
}
// Normalises a body axis of g_pCurrentCar in place through a pointer.
#define CAR_NORMALIZE_AXIS(field)                                                   \
    {                                                                               \
        FixVector *pField = &g_pCurrentCar->field;                                  \
        FixVector *pAxis = &g_pCurrentCar->field;                                   \
        int len = FixVecLength(pField);                                             \
        if (len == 0) {                                                             \
            pAxis->x = 0;                                                           \
            pAxis->y = 0;                                                           \
            pAxis->z = 0;                                                           \
        } else {                                                                    \
            FixVecScaleRecip(pAxis, pField, len);                                   \
        }                                                                           \
    }

void Car_FlagAirborneCorners(void);
void Car_SettleFreeCorners(void);
void Car_SolveUpright(void);
void Car_IntegrateWheelTravel(void);
void StageObject_UpdateCarCornerGroundHeights(Car *pCar, int count);
void CarDamage_ApplyCollisionDeformImpulse(Car *pCar, int *param_2, FixVector *param_3, int param_4,
                  unsigned char param_5, int param_6);
int NetRace_IsPlayerFadeTimed(BYTE index);

// Scratch vectors of the ground-contact step (referenced only from 0x42eae0).
// GLOBAL: CMR2 0x0053c9c8
FixVector g_carContactNormal;
// GLOBAL: CMR2 0x0053ca08
FixVector g_carContactWork;
// GLOBAL: CMR2 0x0053ca40
FixVector g_carContactWork2;
// GLOBAL: CMR2 0x0053ca50
short g_carSurfaceSnapshot[4];
// GLOBAL: CMR2 0x0053ca68
FixVector g_carContactWork3;
// GLOBAL: CMR2 0x0053ca98
FixVector g_carContactSaved;
// GLOBAL: CMR2 0x0053caf8
FixVector g_carContactWork4;
// GLOBAL: CMR2 0x0053cc08
FixVector g_carContactPoint;

// Ground contact and roll-over step of the current car: refreshes the corner
// contact normals from the body axes, decays the roll-over counters, and,
// when no corner is grounded, finds the corner that supports the body against
// the ground normal, pushes it into the deformation solver, damps the angular
// velocity when the deepest corner changes and updates the tumble timers.
// FUNCTION: CMR2 0x0042eae0
void Car_UpdateGroundContact(void)
{
    int flag;
    int i;
    int k;
    int d;
    int dot;
    short prevGrounded;
    int prevDeepest;


    flag = 0;
    g_pCurrentCar->tipAngle = 0;
    g_pCurrentCar->tipRatio = 0;
    g_pCurrentCar->normal0x498 = g_pCurrentCar->groundNormal;
    for (i = 0; i < 8; i++)
        g_pCurrentCar->cornerNormal[i] = g_pCurrentCar->cornerAxis[i];

    if (g_pCurrentCar->field_0xbfc != 0) {
        if (g_pCurrentCar->field_0xbdc != 0) {
            BYTE step = (BYTE)(FixMul(0xa0000, g_physicsTimeStep) >> 16);
            if (step > g_pCurrentCar->field_0xb25)
                g_pCurrentCar->field_0xb25 = 0;
            else
                g_pCurrentCar->field_0xb25 -= step;
            if (g_pCurrentCar->field_0xb25 <= 0)
                g_pCurrentCar->field_0xbfc = 0;
        } else if (g_pCurrentCar->field_0xbe0 != 0) {
            BYTE step = (BYTE)(FixMul(0xa0000, g_physicsTimeStep) >> 16);
            if (step > g_pCurrentCar->field_0xb26)
                g_pCurrentCar->field_0xb26 = 0;
            else
                g_pCurrentCar->field_0xb26 -= step;
            if (g_pCurrentCar->field_0xb26 <= 0)
                g_pCurrentCar->field_0xbfc = 0;
        } else {
            BYTE step = (BYTE)(FixMul(0xa0000, g_physicsTimeStep) >> 16);
            if (step > g_pCurrentCar->field_0xb27)
                g_pCurrentCar->field_0xb27 = 0;
            else
                g_pCurrentCar->field_0xb27 -= step;
            if (g_pCurrentCar->field_0xb27 <= 0)
                g_pCurrentCar->field_0xbfc = 0;
        }
        if (g_pCurrentCar->field_0xbfc == 0) {
            g_pCurrentCar->field_0xbe4 = 0;
            g_pCurrentCar->field_0xbe0 = 0;
            g_pCurrentCar->field_0xbdc = 0;
            g_pCurrentCar->field_0xb27 = 0;
            g_pCurrentCar->field_0xb25 = 0;
            g_pCurrentCar->field_0xb26 = 0;
            g_pCurrentCar->field_0xbf8 = 1;
        }
    } else {
        if (NetRace_IsPlayerFadeTimed(g_pCurrentCar->index) != 0) {
            memcpy(g_pCurrentCar->bodyAxes, g_pCurrentCar->targetAxes, sizeof(g_pCurrentCar->bodyAxes));
            g_pCurrentCar->position = g_pCurrentCar->positionPrev;
        } else {
            for (i = 4; i < 8; i++) {
                if (g_pCurrentCar->cornerFlags[i] == 0) {
                    if (g_pCurrentCar->field_0x95c < 0x640000)
                        g_pCurrentCar->field_0x95c += g_physicsTimeStep;
                    else
                        g_pCurrentCar->field_0xbf8 = 1;
                    break;
                }
            }
        }
    }

    prevGrounded = (char)g_pCurrentCar->field_0xb34;
    for (i = 3; i >= 0; i--)
        g_carSurfaceSnapshot[i] = g_pCurrentCar->wheelSurface[i];
    StageObject_UpdateCarCornerGroundHeights(g_pCurrentCar, 8);
    Car_FlagAirborneCorners();

    if (g_pCurrentCar->field_0xb34 != 0) {
        Car_UpdateGroundNormal();
        Car_SettleFreeCorners();
        Car_FlagAirborneCorners();
        if (prevGrounded != 0) {
            Car_SolveUpright();
            flag = 1;
        } else {
            for (i = 0; i < 8; i++) {
                if (g_pCurrentCar->cornerFlags[i] == 0) {
                dot = FixVecDot(&g_pCurrentCar->cornerVelocity[i], &g_pCurrentCar->groundNormal);
                g_carContactSaved = g_pCurrentCar->field_0x5c4;
                g_pCurrentCar->field_0x5c4.x = dot;
                g_pCurrentCar->field_0x5c4.y = 0;
                g_pCurrentCar->field_0x5c4.z = 0;

                FixVecScale(&g_carContactWork, &g_pCurrentCar->groundNormal, -0x20000);
                FixMatrix_InverseRotateVector(&g_pCurrentCar->field_0x5dc,
                                              &g_carContactWork, g_pCurrentCar->pWorld);

                d = g_pCurrentCar->field_0x5dc.x;
                if (FIX_ABS(d) > g_pCurrentCar->halfExtents.x)
                    g_pCurrentCar->field_0x5dc.x = d < 0 ? -g_pCurrentCar->halfExtents.x : g_pCurrentCar->halfExtents.x;
                d = g_pCurrentCar->field_0x5dc.y;
                if (FIX_ABS(d) > g_pCurrentCar->halfExtents.y)
                    g_pCurrentCar->field_0x5dc.y = d < 0 ? -g_pCurrentCar->halfExtents.y : g_pCurrentCar->halfExtents.y;
                d = g_pCurrentCar->field_0x5dc.z;
                if (FIX_ABS(d) > g_pCurrentCar->halfExtents.z)
                    g_pCurrentCar->field_0x5dc.z = d < 0 ? -g_pCurrentCar->halfExtents.z : g_pCurrentCar->halfExtents.z;

                g_carContactPoint.x = 0;
                g_carContactPoint.y = 0x7d000000;
                g_carContactPoint.z = 0;
                g_carContactNormal.x = 0;
                g_carContactNormal.y = 0x10000;
                g_carContactNormal.z = 0;
                for (k = 0; k < 4; k++) {
                    if (g_pCurrentCar->corners[k].y < g_carContactPoint.y)
                        g_carContactPoint = g_pCurrentCar->corners[k];
                }
                if (g_pCurrentCar->up.y < 0) {
                    FixVecScale(&g_carContactWork2, &g_pCurrentCar->up,
                                FixMul(0x20000, g_pCurrentCar->halfExtents.y));
                    g_carContactPoint.x += g_carContactWork2.x;
                    g_carContactPoint.y += g_carContactWork2.y;
                    g_carContactPoint.z += g_carContactWork2.z;
                }
                CarDamage_ApplyCollisionDeformImpulse(g_pCurrentCar, (int *)&g_carContactPoint, &g_carContactNormal, 0, 0, 0);
                FixMatrix_RotateVector(&g_carContactWork3,
                                       &g_pCurrentCar->field_0x5dc,
                                       g_pCurrentCar->pWorld);
                g_carContactWork3.x += g_pCurrentCar->position.x;
                g_carContactWork3.y += g_pCurrentCar->position.y;
                g_carContactWork3.z += g_pCurrentCar->position.z;
                g_pCurrentCar->field_0x5c4 = g_carContactSaved;
                break;
                }
            }
        }
        d = FixVecDot(&g_pCurrentCar->groundNormal, &g_pCurrentCar->velocity);
        if (d < 0) {
            FixVecScale(&g_carContactWork4, &g_pCurrentCar->groundNormal, d);
            g_pCurrentCar->velocity.x -= g_carContactWork4.x;
            g_pCurrentCar->velocity.y -= g_carContactWork4.y;
            g_pCurrentCar->velocity.z -= g_carContactWork4.z;
        }
        g_pCurrentCar->groundRightDot = FixVecDot(&g_pCurrentCar->right, &g_pCurrentCar->groundNormal);
        g_pCurrentCar->groundUpDot = FixVecDot(&g_pCurrentCar->up, &g_pCurrentCar->groundNormal);
        dot = FixVecDot(&g_pCurrentCar->forward, &g_pCurrentCar->groundNormal);
    } else {
        for (i = 0; i < 8; i++) {
            d = g_pCurrentCar->corners[i].y - g_pCurrentCar->cornerHeight[i];
            if (FIX_ABS(d) < 0x3333) {
                Car_UpdateGroundNormal();
                Car_SolveUpright();
                flag = 1;
                break;
            }
        }
        g_pCurrentCar->groundRightDot = FixVecDot(&g_pCurrentCar->right, &g_pCurrentCar->groundNormal);
        g_pCurrentCar->groundUpDot = FixVecDot(&g_pCurrentCar->up, &g_pCurrentCar->groundNormal);
        dot = FixVecDot(&g_pCurrentCar->forward, &g_pCurrentCar->groundNormal);
    }
    g_pCurrentCar->groundForwardDot = dot;

    if (flag == 0) {
        g_pCurrentCar->field_0xba0[2] = 0;
        g_pCurrentCar->field_0xba0[1] = 0;
        g_pCurrentCar->field_0xba0[0] = 0;
    }

    g_pCurrentCar->groundHeightCorrection = 0;
    prevDeepest = g_pCurrentCar->deepestCorner;
    g_pCurrentCar->deepestCorner = -1;
    for (i = 0; i < 4; i++) {
        if (i < 2)
            FixVecScale(&g_carContactWork, &g_pCurrentCar->up, g_pCurrentCar->field_0x770[1]);
        else
            FixVecScale(&g_carContactWork, &g_pCurrentCar->up, g_pCurrentCar->field_0x770[0]);
        g_carContactWork.x += g_pCurrentCar->corners[i].x;
        g_carContactWork.y += g_pCurrentCar->corners[i].y;
        g_carContactWork.z += g_pCurrentCar->corners[i].z;
        d = FixMul(g_pCurrentCar->cornerHeight[i + 4] + g_pCurrentCar->cornerHeight[i], 0x8000) -
            g_carContactWork.y;
        if (d > g_pCurrentCar->groundHeightCorrection) {
            g_pCurrentCar->groundHeightCorrection = d;
            g_pCurrentCar->deepestCorner = (char)i;
        }
    }
    if (g_pCurrentCar->groundHeightCorrection == 0) {
        g_pCurrentCar->groundHeightCorrection = 0x1999;
        for (i = 0; i < 4; i++) {
            if (g_pCurrentCar->cornerFlags[i] == 0) {
                d = g_pCurrentCar->corners[i].y - g_pCurrentCar->cornerHeight[i];
                if (d < g_pCurrentCar->groundHeightCorrection)
                    g_pCurrentCar->groundHeightCorrection = d > 0 ? d : 0;
            }
        }
        g_pCurrentCar->groundHeightCorrection = -g_pCurrentCar->groundHeightCorrection;
    }
    if (g_pCurrentCar->deepestCorner != -1 && g_pCurrentCar->deepestCorner != prevDeepest) {
        FixMatrix_InverseRotateVector(&g_carContactWork, &g_pCurrentCar->groundNormal,
                                      g_pCurrentCar->pWorld);
        dot = FixVecDot(&g_carContactWork, &g_pCurrentCar->angularVelocity);
        FixVecScale(&g_carContactWork, &g_carContactWork, dot);
        g_carContactWork.x = g_pCurrentCar->angularVelocity.x - g_carContactWork.x;
        g_carContactWork.y = g_pCurrentCar->angularVelocity.y - g_carContactWork.y;
        g_carContactWork.z = g_pCurrentCar->angularVelocity.z - g_carContactWork.z;
        FixVecScale(&g_carContactWork, &g_carContactWork, FixMul(0xccc, g_physicsTimeStep));
        g_pCurrentCar->angularVelocity.x -= g_carContactWork.x;
        g_pCurrentCar->angularVelocity.y -= g_carContactWork.y;
        g_pCurrentCar->angularVelocity.z -= g_carContactWork.z;
    }

    Car_RelaxBodyAxes(1);
    Car_IntegrateWheelTravel();
    d = g_pCurrentCar->groundUpDot;
    if (g_pCurrentCar->field_0xc04[0] == 0 && d < 0)
        g_pCurrentCar->field_0xc04[0] = 1;
    if (d < 0)
        g_pCurrentCar->field_0xc10 = 1;
    g_pCurrentCar->field_0x96c -= FixMul(0x1eb8, g_physicsTimeStep);
    if (g_pCurrentCar->field_0x96c < 0)
        g_pCurrentCar->field_0x96c = 0;
    if (g_pCurrentCar->field_0x96c > 0)
        return;
    if (d <= 0xfc28)
        return;
    if (g_pCurrentCar->field_0xc10 != 0) {
        if (FIX_ABS(g_pCurrentCar->angularVelocity.x) >= 0x28f)
            return;
        if (FIX_ABS(g_pCurrentCar->angularVelocity.z) >= 0x28f)
            return;
    }
    if (g_pCurrentCar->field_0xb28 != 0) {
        g_pCurrentCar->useUpperCollisionCorners = 0;
        g_pCurrentCar->field_0x96c = 0;
        g_pCurrentCar->field_0xc04[0] = 0;
        g_pCurrentCar->field_0xc10 = 0;
    }
}

// Flags the box corners of the current car that are off the ground and lists
// (from 0xb36) the ones that touch it.
// match 78%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0042f820
void Car_FlagAirborneCorners(void)
{
    int i;

    g_pCurrentCar->field_0xb34 = 0;
    g_pCurrentCar->field_0xb35[0] = 1;
    for (i = 7; i >= 0; i--) {
        int h = g_pCurrentCar->cornerHeight[i] + 0x1999;

        if (g_pCurrentCar->corners[i].y <= h) {
            g_pCurrentCar->cornerFlags[i] = 0;
            g_pCurrentCar->field_0xb35[0] = 0;
            g_pCurrentCar->field_0xb35[1 + g_pCurrentCar->field_0xb34] = (BYTE)i;
            g_pCurrentCar->field_0xb34++;
        } else {
            g_pCurrentCar->cornerFlags[i] = 1;
        }
    }
}

// Relaxes the body up and forward vectors of g_pCurrentCar towards their
// targets (0x390/0x39c) with a rate that grows with the distance, then
// re-orthogonalises the basis and writes it to the world matrix.
// bFast selects the faster blend rates.
// FUNCTION: CMR2 0x00431220
void Car_RelaxBodyAxes(int bFast)
{
    int rateMin;
    int rateRange;
    int gain;
    FixVector diff;
    FixVector proj;
    int len;
    int t;
    int d;

    if (bFast != 0) {
        rateMin = 0x8000;
        rateRange = 0x8000;
    } else {
        rateMin = 0x1999;
        rateRange = 0x4ccc;
    }
    gain = FixMul(0xc0000, g_physicsTimeStep);

    if (FIX_ABS(g_pCurrentCar->groundUpDot) > 0xfae1) {
        diff.x = g_pCurrentCar->up.x - g_pCurrentCar->targetUp.x;
        diff.y = g_pCurrentCar->up.y - g_pCurrentCar->targetUp.y;
        diff.z = g_pCurrentCar->up.z - g_pCurrentCar->targetUp.z;
        len = FixVecLength(&diff);
        if (len > 0) {
            t = FixMul(len, gain);
            if (t > 0x10000)
                t = 0x10000;
            FixVecScale(&diff, &diff, FixMul(t, rateRange) + rateMin);
            g_pCurrentCar->up.x = g_pCurrentCar->targetUp.x + diff.x;
            g_pCurrentCar->up.y = g_pCurrentCar->targetUp.y + diff.y;
            g_pCurrentCar->up.z = g_pCurrentCar->targetUp.z + diff.z;
            CAR_NORMALIZE_AXIS(up)
        }
    }

    if (FIX_ABS(g_pCurrentCar->groundForwardDot) > 0x3333 && FIX_ABS(g_pCurrentCar->groundRightDot) < 0x1999) {
        diff.x = g_pCurrentCar->forward.x - g_pCurrentCar->targetForward.x;
        diff.y = g_pCurrentCar->forward.y - g_pCurrentCar->targetForward.y;
        diff.z = g_pCurrentCar->forward.z - g_pCurrentCar->targetForward.z;
        d = FixVecDot(&g_pCurrentCar->up, &diff);
        FixVecScale(&proj, &g_pCurrentCar->up, d);
        diff.x = diff.x - proj.x;
        diff.y = diff.y - proj.y;
        diff.z = diff.z - proj.z;
        len = FixVecLength(&diff);
        if (len > 0) {
            t = FixMul(len, gain);
            if (t > 0x10000)
                t = 0x10000;
            FixVecScale(&diff, &diff, (FixMul(t, rateRange) - 0x10000) + rateMin);
            g_pCurrentCar->forward.x = g_pCurrentCar->forward.x + diff.x;
            g_pCurrentCar->forward.y = g_pCurrentCar->forward.y + diff.y;
            g_pCurrentCar->forward.z = g_pCurrentCar->forward.z + diff.z;
            CAR_NORMALIZE_AXIS(forward)
        }
    }

    d = FixVecDot(&g_pCurrentCar->up, &g_pCurrentCar->forward);
    FixVecScale(&diff, &g_pCurrentCar->up, d);
    g_pCurrentCar->forward.x = g_pCurrentCar->forward.x - diff.x;
    g_pCurrentCar->forward.y = g_pCurrentCar->forward.y - diff.y;
    g_pCurrentCar->forward.z = g_pCurrentCar->forward.z - diff.z;
    CAR_NORMALIZE_AXIS(forward)
    FixVecCross(&g_pCurrentCar->right, &g_pCurrentCar->up, &g_pCurrentCar->forward);
    CAR_NORMALIZE_AXIS(right)
    g_pCurrentCar->pWorld->right = g_pCurrentCar->right;
    g_pCurrentCar->pWorld->up = g_pCurrentCar->up;
    g_pCurrentCar->pWorld->forward = g_pCurrentCar->forward;
    g_pCurrentCar->pWorld->position = g_pCurrentCar->position;
}

// Velocity of each box corner of g_pCurrentCar: v + w x r, with the
// angular velocity in body space and the corners as +-half extents.
// match 66%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00434f50
void Car_UpdateCornerVelocities(void)
{
    FixVector *pCv;
    FixMatrix *pM;
    FixVector *pVel;
    FixVector *pW;
    int wyhz;
    int wzhy;
    int wzhx;
    int wxhz;
    int wxhy;
    int wyhx;
    int a;
    int b;
    int c;
    int d;
    int e;
    int f;
    int c0;
    int c1;
    int c2;
    int c3;

    pM = g_pCurrentCar->pWorld;
    pW = &g_pCurrentCar->angularVelocity;
    pVel = &g_pCurrentCar->velocity;
    pCv = g_pCurrentCar->cornerVelocity;
    wyhz = FixMul(pW->y, g_pCurrentCar->halfExtents.z);
    wzhy = FixMul(pW->z, g_pCurrentCar->halfExtents.y);
    wzhx = FixMul(pW->z, g_pCurrentCar->halfExtents.x);
    wxhz = FixMul(pW->x, g_pCurrentCar->halfExtents.z);
    wxhy = FixMul(pW->x, g_pCurrentCar->halfExtents.y);
    wyhx = FixMul(pW->y, g_pCurrentCar->halfExtents.x);

    a = FixMul(pM->right.x, wyhz);
    b = FixMul(pM->right.x, wzhy);
    c = FixMul(pM->up.x, wzhx);
    d = FixMul(pM->up.x, wxhz);
    e = FixMul(pM->forward.x, wxhy);
    f = FixMul(pM->forward.x, wyhx);
    pCv[0].x = (((c - f) - e) - d) + b;
    pCv[0].x += a;
    pCv[1].x = (((d - f) - e) - a) + c + b;
    pCv[2].x = (((f - e) - d) - c) + b + a;
    pCv[3].x = (((f - e) - c) - a) + d + b;

    a = FixMul(pM->right.y, wyhz);
    b = FixMul(pM->right.y, wzhy);
    c = FixMul(pM->up.y, wzhx);
    d = FixMul(pM->up.y, wxhz);
    e = FixMul(pM->forward.y, wxhy);
    f = FixMul(pM->forward.y, wyhx);
    pCv[0].y = (((c - f) - e) - d) + b + a;
    pCv[1].y = (((d - f) - e) - a) + c + b;
    pCv[2].y = (((f - e) - d) - c) + b + a;
    pCv[3].y = (((f - e) - c) - a) + d + b;

    a = FixMul(pM->right.z, wyhz);
    b = FixMul(pM->right.z, wzhy);
    c = FixMul(pM->up.z, wzhx);
    d = FixMul(pM->up.z, wxhz);
    e = FixMul(pM->forward.z, wxhy);
    f = FixMul(pM->forward.z, wyhx);
    c0 = (((c - f) - e) - d) + b + a;
    c1 = (((d - f) - e) - a) + c + b;
    pCv[1].z = c1;
    pCv[0].z = c0;
    c2 = (((f - e) - d) - c) + b + a;
    c3 = (((f - e) - c) - a) + d + b;
    pCv[3].z = c3;
    pCv[2].z = c2;

    pCv[4].x = -pCv[3].x;
    pCv[4].y = -pCv[3].y;
    pCv[4].z = -c3;
    pCv[5].x = -pCv[2].x;
    pCv[5].y = -pCv[2].y;
    pCv[5].z = -c2;
    pCv[6].x = -pCv[1].x;
    pCv[6].y = -pCv[1].y;
    pCv[6].z = -c1;
    pCv[7].x = -pCv[0].x;
    pCv[7].y = -pCv[0].y;
    pCv[7].z = -c0;

    pCv[0].x = pCv[0].x + pVel->x;
    pCv[0].y = pCv[0].y + pVel->y;
    pCv[0].z = pCv[0].z + pVel->z;
    pCv[1].x = pCv[1].x + pVel->x;
    pCv[1].y = pCv[1].y + pVel->y;
    pCv[1].z = pCv[1].z + pVel->z;
    pCv[2].x = pCv[2].x + pVel->x;
    pCv[2].y = pCv[2].y + pVel->y;
    pCv[2].z = pCv[2].z + pVel->z;
    pCv[3].x = pCv[3].x + pVel->x;
    pCv[3].y = pCv[3].y + pVel->y;
    pCv[3].z = pCv[3].z + pVel->z;
    pCv[4].x = pCv[4].x + pVel->x;
    pCv[4].y = pCv[4].y + pVel->y;
    pCv[4].z = pCv[4].z + pVel->z;
    pCv[5].x = pCv[5].x + pVel->x;
    pCv[5].y = pCv[5].y + pVel->y;
    pCv[5].z = pCv[5].z + pVel->z;
    pCv[6].x = pCv[6].x + pVel->x;
    pCv[6].y = pCv[6].y + pVel->y;
    pCv[6].z = pCv[6].z + pVel->z;
    pCv[7].x = pCv[7].x + pVel->x;
    pCv[7].y = pCv[7].y + pVel->y;
    pCv[7].z = pCv[7].z + pVel->z;

}

// Corner i of the box with its y replaced by the ground height under it.
#define GROUND_CORNER(v, i)                                                         \
    v.x = g_pCurrentCar->corners[i].x;                                              \
    v.y = g_pCurrentCar->cornerHeight[i];                                           \
    v.z = g_pCurrentCar->corners[i].z;

// Picks the box face closest to horizontal and rebuilds the ground normal
// from the two triangles formed by its four corners projected onto the ground.
// FUNCTION: CMR2 0x0042de20
void Car_UpdateGroundNormal(void)
{
    int absRight = FIX_ABS(g_pCurrentCar->right.y);
    int absUp = FIX_ABS(g_pCurrentCar->up.y);
    int absForward = FIX_ABS(g_pCurrentCar->forward.y);
    FixVector a;
    FixVector b;
    FixVector c;
    FixVector d;
    FixVector edge1;
    FixVector edge3;
    FixVector edge4;
    FixVector edge2;
    FixVector n;

    if (absUp >= absRight && absUp >= absForward) {
        b.x = g_pCurrentCar->corners[0].x;
        a.x = g_pCurrentCar->corners[1].x;
        c.x = g_pCurrentCar->corners[2].x;
        d.x = g_pCurrentCar->corners[3].x;
        b.y = g_pCurrentCar->cornerHeight[0];
        a.y = g_pCurrentCar->cornerHeight[1];
        c.y = g_pCurrentCar->cornerHeight[2];
        d.y = g_pCurrentCar->cornerHeight[3];
        b.z = g_pCurrentCar->corners[0].z;
        a.z = g_pCurrentCar->corners[1].z;
        c.z = g_pCurrentCar->corners[2].z;
        d.z = g_pCurrentCar->corners[3].z;
    } else if (absRight >= absUp && absRight >= absForward) {
        b.x = g_pCurrentCar->corners[0].x;
        a.x = g_pCurrentCar->corners[1].x;
        c.x = g_pCurrentCar->corners[4].x;
        d.x = g_pCurrentCar->corners[5].x;
        b.y = g_pCurrentCar->cornerHeight[0];
        a.y = g_pCurrentCar->cornerHeight[1];
        c.y = g_pCurrentCar->cornerHeight[4];
        d.y = g_pCurrentCar->cornerHeight[5];
        b.z = g_pCurrentCar->corners[0].z;
        a.z = g_pCurrentCar->corners[1].z;
        c.z = g_pCurrentCar->corners[4].z;
        d.z = g_pCurrentCar->corners[5].z;
    } else {
        b.x = g_pCurrentCar->corners[0].x;
        a.x = g_pCurrentCar->corners[2].x;
        c.x = g_pCurrentCar->corners[4].x;
        d.x = g_pCurrentCar->corners[6].x;
        b.y = g_pCurrentCar->cornerHeight[0];
        a.y = g_pCurrentCar->cornerHeight[2];
        c.y = g_pCurrentCar->cornerHeight[4];
        d.y = g_pCurrentCar->cornerHeight[6];
        b.z = g_pCurrentCar->corners[0].z;
        a.z = g_pCurrentCar->corners[2].z;
        c.z = g_pCurrentCar->corners[4].z;
        d.z = g_pCurrentCar->corners[6].z;
    }

    edge1.x = a.x - b.x;
    edge1.y = a.y - b.y;
    edge1.z = a.z - b.z;
    edge3.x = c.x - b.x;
    edge3.y = c.y - b.y;
    edge3.z = c.z - b.z;
    FixVecCross(&n, &edge3, &edge1);
    if (n.y < 0) {
        n.x = -n.x;
        n.y = -n.y;
        n.z = -n.z;
    }
    {
        FixVector *pOut = &g_pCurrentCar->groundNormal;
        int len = FixVecLength(&n);
        if (len == 0) {
            pOut->x = 0;
            pOut->y = 0;
            pOut->z = 0;
        } else {
            FixVecScaleRecip(pOut, &n, len);
        }
    }

    edge2.x = a.x - d.x;
    edge2.y = a.y - d.y;
    edge2.z = a.z - d.z;
    edge4.x = c.x - d.x;
    edge4.y = c.y - d.y;
    edge4.z = c.z - d.z;
    FixVecCross(&n, &edge4, &edge2);
    if (n.y < 0) {
        n.x = -n.x;
        n.y = -n.y;
        n.z = -n.z;
    }
    FIX_NORMALIZE_INTO(n, n);

    {
        FixVector *pOut;
        n.x += g_pCurrentCar->groundNormal.x;
        n.y += g_pCurrentCar->groundNormal.y;
        n.z += g_pCurrentCar->groundNormal.z;
        pOut = &g_pCurrentCar->groundNormal;
        {
            int len = FixVecLength(&n);
            if (len == 0) {
                pOut->x = 0;
                pOut->y = 0;
                pOut->z = 0;
            } else {
                FixVecScaleRecip(pOut, &n, len);
            }
        }
    }
}

FixVector g_leanDamping;
FixVector g_leanAccel;
FixVector g_leanDelta;
FixVector g_carAccel;
FixBasis g_leanBasis;

int g_physicsScale = 0x10000;

// Integrates the body lean (the chassis pitching/rolling against its own
// acceleration) and rebuilds the body matrix from the two lean angles.
// match 85%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00443250
void Car_UpdateBodyLean(void)
{
    int len;
    int maxStep;
    short angleZ;
    int angleX;
    int sinZ;
    int cosZ;
    int sinX;
    int cosX;
    int z;

    g_carAccel.x = g_pCurrentCar->velocityNext.x - g_pCurrentCar->velocity.x;
    g_carAccel.y = g_pCurrentCar->velocityNext.y - g_pCurrentCar->velocity.y;
    g_carAccel.z = g_pCurrentCar->velocityNext.z - g_pCurrentCar->velocity.z;
    FixMatrix_InverseRotateVector(&g_leanAccel, &g_carAccel, g_pCurrentCar->pWorld);
    if (FixVecLength(&g_leanAccel) == 0 || g_pCurrentCar->field_0xb60 != 0) {
        g_leanAccel.x = 0;
        g_leanAccel.y = 0;
        g_leanAccel.z = 0;
    }
    FixVecScale(&g_leanAccel, &g_leanAccel, g_physicsScale);
    g_leanAccel.z = g_leanAccel.z * 2;

    len = FixVecLength(&g_pCurrentCar->lean);
    if (len > 0) {
        int damping = -FixMul(len, g_pCurrentCar->bodyLeanDamping);
        FixVecScaleRecip(&g_leanDamping, &g_pCurrentCar->lean, len);
        FixVecScale(&g_leanDamping, &g_leanDamping, damping);
    } else {
        g_leanDamping.x = 0;
        g_leanDamping.y = 0;
        g_leanDamping.z = 0;
    }

    g_leanDelta.x = g_leanDamping.x + g_leanAccel.x;
    g_leanDelta.y = g_leanDamping.y + g_leanAccel.y;
    g_leanDelta.z = g_leanDamping.z + g_leanAccel.z;
    FixVecScale(&g_leanDelta, &g_leanDelta, FixMul(g_physicsTimeStep, 0x13333));
    g_leanDelta.y = 0;
    len = FixVecLength(&g_leanDelta);
    maxStep = FixMul(0x312, g_physicsTimeStep);
    if (len > maxStep) {
        FixVecScaleRecip(&g_leanDelta, &g_leanDelta, len);
        FixVecScale(&g_leanDelta, &g_leanDelta, FixMul(0x312, g_physicsTimeStep));
    }

    g_pCurrentCar->lean.x += g_leanDelta.x;
    g_pCurrentCar->lean.y += g_leanDelta.y;
    g_pCurrentCar->lean.z += g_leanDelta.z;
    {
        FixVector *pLean = &g_pCurrentCar->lean;
        g_leanAccel = *pLean;
    }

    if (FIX_ABS(g_leanAccel.x) > 0x170a) {
        g_leanAccel.x = g_leanAccel.x > 0 ? 0x170a : -0x170a;
    }
    z = g_leanAccel.z;
    if (FIX_ABS(z) > 0x170a) {
        if (z > 0) {
            z = 0x170a;
            g_leanAccel.z = z;
        } else {
            z = -0x170a;
            g_leanAccel.z = z;
        }
    }
    if (z < 0) {
        z = -z;
    }

    angleZ = FixAtan2(z, FixMul(0x10000, 0x10000));
    if (g_leanAccel.z < 0) {
        angleZ = -angleZ;
    }
    sinZ = g_sinTable[angleZ & 0xfff];
    cosZ = g_sinTable[(angleZ + 0x400) & 0xfff];

    angleX = FixAtan2(FIX_ABS(g_leanAccel.x), FixMul(0x10000, 0x10000));
    if (g_leanAccel.x < 0) {
        angleX = -angleX;
    }
    sinX = g_sinTable[angleX & 0xfff];
    cosX = g_sinTable[(angleX + 0x400) & 0xfff];

    g_leanBasis.right.z = 0;
    g_leanBasis.right.x = cosX;
    g_leanBasis.right.y = -sinX;
    g_leanBasis.up.x = FixMul(sinX, cosZ);
    g_leanBasis.up.y = FixMul(cosX, cosZ);
    g_leanBasis.up.z = sinZ;
    g_leanBasis.forward.x = FixMul(sinX, -sinZ);
    g_leanBasis.forward.y = FixMul(-sinZ, cosX);
    g_leanBasis.forward.z = cosZ;

    g_pCurrentCar->pBodyMatrix->right = g_leanBasis.right;
    g_pCurrentCar->pBodyMatrix->up = g_leanBasis.up;
    g_pCurrentCar->pBodyMatrix->forward = g_leanBasis.forward;
}

// Evens out the two wheels of an axle once they drift too far apart.
// FUNCTION: CMR2 0x0043af70
void Car_BalanceWheelPairs(void)
{
    int diff = g_pCurrentCar->wheelLoad[0] - g_pCurrentCar->wheelLoad[1];
    if (diff < 0) {
        diff = -diff;
    }
    if (diff > 0x1999) {
        int sum = g_pCurrentCar->wheelLoad[1] + g_pCurrentCar->wheelLoad[0];
        int avg = FixMul(sum, 0x8000);
        g_pCurrentCar->wheelLoad[1] = avg;
        g_pCurrentCar->wheelLoad[0] = avg;
    }
    diff = g_pCurrentCar->wheelLoad[2] - g_pCurrentCar->wheelLoad[3];
    if (diff < 0) {
        diff = -diff;
    }
    if (diff > 0x1999) {
        int sum = g_pCurrentCar->wheelLoad[3] + g_pCurrentCar->wheelLoad[2];
        int avg = FixMul(sum, 0x8000);
        g_pCurrentCar->wheelLoad[3] = avg;
        g_pCurrentCar->wheelLoad[2] = avg;
    }
}

// arccos as a 12-bit angle: 4096 entries for a dot product in [-1, 1]
// GLOBAL: CMR2 0x006e6ef4
short g_acosTable[4096];

// Builds the sideways (friction) force at every corner that is sliding against
// its contact normal, sharing the given grip between the sliding corners.
// FUNCTION: CMR2 0x0043a920
void Car_ApplyCornerFriction(int grip)
{
    int sliding[8];
    int strength[8];
    FixVector dir;
    int count = 0;
    int cornerCount;
    int i;
    int along;

    if (g_pCurrentCar->useUpperCollisionCorners == 0) {
        cornerCount = 4;
    } else {
        cornerCount = 8;
        if (g_pCurrentCar->field_0xb34 > 0) {
            grip = FixDiv(g_pCurrentCar->mass, 0x80000);
        }
    }

    i = 0;
    if (cornerCount > 0) {
        do {
            sliding[i] = 0;
            if ((g_pCurrentCar->useUpperCollisionCorners == 0 && g_pCurrentCar->cornerOnGround[i] != 0) ||
                (g_pCurrentCar->useUpperCollisionCorners != 0 && g_pCurrentCar->cornerFlags[i] == 0)) {
                along = FixVecDot(&g_pCurrentCar->cornerAxis[i], &g_pCurrentCar->cornerNormal[i]);
                if (along >= 0x10000) {
                    strength[i] = 0;
                } else {
                    strength[i] = (0x400 - FixAcos(along)) * 0x1680;
                }
                if (strength[i] > 0xa0000 &&
                    g_pCurrentCar->cornerAxis[i].y < g_pCurrentCar->cornerNormal[i].y) {
                    along = FixVecDot(&g_pCurrentCar->cornerVelocity[i], &g_pCurrentCar->cornerNormal[i]);
                    FixVecScale(&dir, &g_pCurrentCar->cornerNormal[i], along);
                    dir.x = g_pCurrentCar->cornerVelocity[i].x - dir.x;
                    dir.y = g_pCurrentCar->cornerVelocity[i].y - dir.y;
                    dir.z = g_pCurrentCar->cornerVelocity[i].z - dir.z;
                    if (FixVecDot(&dir, &g_pCurrentCar->cornerAxis[i]) < 0) {
                        sliding[i] = 1;
                        count++;
                    }
                }
            }
            i++;
        } while (i < cornerCount);

        if (count > 0) {
            int share = FixDiv(0x140000, count << 16);
            i = 0;
            do {
                if (sliding[i] != 0) {
                    int len;

                    along = FixVecDot(&g_pCurrentCar->cornerAxis[i], &g_pCurrentCar->cornerNormal[i]);
                    FixVecScale(&dir, &g_pCurrentCar->cornerNormal[i], along);
                    dir.x = g_pCurrentCar->cornerAxis[i].x - dir.x;
                    dir.y = g_pCurrentCar->cornerAxis[i].y - dir.y;
                    dir.z = g_pCurrentCar->cornerAxis[i].z - dir.z;
                    len = FixVecLength(&dir);
                    if (len == 0) {
                        dir.x = 0;
                        dir.y = 0;
                        dir.z = 0;
                    } else {
                        FixVecScaleRecip(&dir, &dir, len);
                    }
                    along = FixVecDot(&dir, &g_pCurrentCar->cornerVelocity[i]);
                    if (FIX_ABS(along) > 0x10000) {
                        if (along < 0) {
                            dir.x = -dir.x;
                            dir.y = -dir.y;
                            dir.z = -dir.z;
                        }
                    } else {
                        FixVecScale(&dir, &dir, along);
                    }
                    strength[i] = strength[i] - 0xa0000;
                    along = FixMul(strength[i], 0x51e);
                    if (along > 0x10000) {
                        along = 0x10000;
                    }
                    FixVecScale(&dir, &dir, (-FixMul(along, FixMul(grip, share))));
                    g_pCurrentCar->cornerForce[i].x += dir.x;
                    g_pCurrentCar->cornerForce[i].y += dir.y;
                    g_pCurrentCar->cornerForce[i].z += dir.z;
                }
                i++;
            } while (i < cornerCount);
        }
    }
}

extern CarPartSet *g_physicsPartSet;

// Scratch values of the tyre model (Car_UpdateTyreForces).
// GLOBAL: CMR2 0x0053c9b4
int g_tyreGripScale;
// GLOBAL: CMR2 0x0053c9b8
FixVector g_tyreLatForce;
// GLOBAL: CMR2 0x0053c9d8
FixVector g_carStepAccel;
// GLOBAL: CMR2 0x0053c9e8
FixVector g_tyreLongForce;
// GLOBAL: CMR2 0x0053ca4c
int g_tyreSlipScale;
// GLOBAL: CMR2 0x0053ca88
FixVector g_tyreForce;
// GLOBAL: CMR2 0x0053ca94
int g_wheelSpinStep;
// GLOBAL: CMR2 0x0053cae0
int g_tyreForceMax;
// GLOBAL: CMR2 0x0053cc20
int g_tyreForceRecip;

#define FIX_SQR(x) FixMul((x), (x))

inline int FixVecNormalizeLen(FixVector *pOut, FixVector *pV)
{
    int len;

    len = FixVecLength(pV);
    if (len == 0) {
        pOut->x = 0;
        pOut->y = 0;
        pOut->z = 0;
    } else {
        FixVecScaleRecip(pOut, pV, len);
    }
    return len;
}

// Brakes a spinning wheel: the brake of its axle plus its rolling resistance.
#define WHEEL_BRAKE(i, resist)                                                              \
    if (g_pCurrentCar->wheelLoad[i] > 0) {                                                 \
        g_pCurrentCar->wheelTorque[i] -= (i) < 2 ? brakeFront : brakeRear;                  \
        g_pCurrentCar->wheelTorque[i] = g_pCurrentCar->wheelTorque[i] - (resist);           \
    } else if (g_pCurrentCar->wheelLoad[i] < 0) {                                           \
        g_pCurrentCar->wheelTorque[i] += (i) < 2 ? brakeFront : brakeRear;                  \
        g_pCurrentCar->wheelTorque[i] = g_pCurrentCar->wheelTorque[i] + (resist);           \
    }

// Integrates the wheel spin; an undriven wheel (or one against the gear)
// stops instead of reversing.
#define WHEEL_SPIN(i) \
    old = g_pCurrentCar->wheelLoad[i]; \
    g_pCurrentCar->wheelLoad[i] += FixMul(g_pCurrentCar->wheelTorque[i], g_wheelSpinStep); \
    if (g_pCurrentCar->gear != 0 && g_pCurrentCar->shiftInProgress == 0 && \
        g_pCurrentCar->driveSplit != ((i) < 2 ? 0 : 0x10000)) { \
        gear = g_pCurrentCar->gearRatio[g_pCurrentCar->gear]; \
        if ((gear > 0 && g_pCurrentCar->wheelLoad[i] < 0) || \
            (gear < 0 && g_pCurrentCar->wheelLoad[i] > 0)) \
            g_pCurrentCar->wheelLoad[i] = 0; \
    } else { \
        if ((old > 0 && g_pCurrentCar->wheelLoad[i] < 0) || \
            (old < 0 && g_pCurrentCar->wheelLoad[i] > 0)) \
            g_pCurrentCar->wheelLoad[i] = 0; \
    }

// Longitudinal and lateral force scaled down to at most 1.0 before being
// combined; the length is restored afterwards.
#define TYRE_FORCE_SCALED(i)                                                                \
    g_tyreForceRecip = FixDiv(0x10000, g_tyreForceMax);                                     \
    FixVecScale(&g_tyreLongForce, &dir[i], FixMul(longForce, g_tyreForceRecip));            \
    FixVecScale(&g_tyreLatForce, &axis[i], FixMul(latForce, g_tyreForceRecip));             \
    g_tyreForce.x = g_tyreLatForce.x + g_tyreLongForce.x;                                   \
    g_tyreForce.y = g_tyreLatForce.y + g_tyreLongForce.y;                                   \
    g_tyreForce.z = g_tyreLatForce.z + g_tyreLongForce.z;                                   \
    mag = FixMul(FIX_SQR(FixVecNormalizeLen(&g_pCurrentCar->cornerForce[i], &g_tyreForce)), g_tyreForceMax)

// Sliding friction of the box corners without a wheel: the part of each corner
// velocity along the ground plane is opposed by a force proportional to it (plus
// the step acceleration while almost stopped), clamped to the corner grip.
// match 80%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00438460
void Car_UpdateCornerFriction(void)
{
    int i;
    int len;
    int force;
    FixVector tangent;
    FixVector dir;

    if (g_pCurrentCar->field_0xb74 != 0)
        return;
    for (i = 7; i >= 0; i--) {
        if (g_pCurrentCar->cornerFlags[i] != 0)
            continue;
        len = FixVecDot(&g_pCurrentCar->groundNormal, &g_pCurrentCar->cornerVelocity[i]);
        FixVecScale(&tangent, &g_pCurrentCar->groundNormal, len);
        tangent.x = g_pCurrentCar->cornerVelocity[i].x - tangent.x;
        tangent.y = g_pCurrentCar->cornerVelocity[i].y - tangent.y;
        tangent.z = g_pCurrentCar->cornerVelocity[i].z - tangent.z;
        len = FixVecLength(&tangent);
        if (len > 0) {
            FixVecScaleRecip(&dir, &tangent, len);
            force = -FixMul(len, g_pCurrentCar->cornerMass);
            if (len < 0x10000)
                force -= FixVecLength(&g_carStepAccel);
            if (((force) < 0 ? -(force) : (force)) > g_pCurrentCar->cornerGripB[i]) {
                if (force > 0)
                    force = g_pCurrentCar->cornerGripA[i];
                else
                    force = -g_pCurrentCar->cornerGripA[i];
            }
            FixVecScale(&g_pCurrentCar->cornerForce[i], &dir, force);
        }
    }
}

// Tyre model of a car on the ground: spins the wheels up and down with the
// drive torque, brakes and handbrake, then turns the rolling and lateral slip
// of each wheel into a force at its corner, capped by a friction ellipse.
// With the car in the air (or without tyre contact) only the wheel spin is
// integrated.
// FUNCTION: CMR2 0x004387a0
void Car_UpdateTyreForces(void)
{
    FixVector dir[4];
    FixVector axis[4];
    FixVector v;
    FixVector t;
    FixVector *pNormal;
    int dirValid[4] = {1, 1, 1, 1};
    int axisValid[4] = {1, 1, 1, 1};
    int rollSpeed[4];
    int brakeFront;
    int brakeRear;
    int longForce;
    int latForce;
    int longAssist;
    int latAssist;
    int speed;
    int slip;
    int spinSlip;
                int vectorOffset;
    int crossing;
    int grip;
    int resist;
    int limB;
    int b;
    int d;
    int d_2;
    int e;
    int s;
    int old;
    int gear;
    int len;
    short angle;
    int i;

    latForce = 0;
    longForce = 0;
    latAssist = 0;
    longAssist = 0;
    pNormal = &g_pCurrentCar->groundNormal;
    if (g_pCurrentCar->flag0x1d0[2] == 0 && g_pCurrentCar->flag0x1d0[3] == 0 &&
        g_pCurrentCar->gear != 0) {
        if (g_pCurrentCar->speed < 0x1999) {
            brakeFront = FixMul(0x1999, g_pCurrentCar->brakeBias);
            brakeRear = FixMul(0x1999, 0x10000 - g_pCurrentCar->brakeBias);
        } else {
            brakeFront = 0;
            brakeRear = 0;
        }
    } else {
        resist = FixMul(g_pCurrentCar->brakeInput, g_physicsPartSet->frontBrakeScale);
        brakeFront = FixMul(resist, g_pCurrentCar->brakeBias);
        resist = FixMul(g_pCurrentCar->brakeInput, g_physicsPartSet->rearBrakeScale);
        brakeRear = FixMul(resist, 0x10000 - g_pCurrentCar->brakeBias);
    }
    if (g_pCurrentCar->handbrake != 0)
        brakeRear += g_pCurrentCar->handbrakeForce;

    if (g_pCurrentCar->field_0xb28 != 0 && g_pCurrentCar->field_0xb74 != 0) {
        // Rolling directions and lateral axes of the front and rear wheels,
        // flattened onto the ground.
        d_2 = -FixVecDot(pNormal, &g_pCurrentCar->frontWheelDir);
        FixVecScale(&t, pNormal, d_2);
        v.x = g_pCurrentCar->frontWheelDir.x + t.x;
        v.y = g_pCurrentCar->frontWheelDir.y + t.y;
        v.z = g_pCurrentCar->frontWheelDir.z + t.z;
        len = FixVecLength(&v);
        if (len == 0) {
            dir[0].x = 0;
            dir[0].y = 0;
            dir[0].z = 0;
        } else {
            FixVecScaleRecip(&dir[0], &v, len);
        }
        dir[1] = dir[0];
        if (len == 0) {
            dirValid[1] = 0;
            dirValid[0] = 0;
        }
        d_2 = -FixVecDot(pNormal, &g_pCurrentCar->frontWheelAxis);
        FixVecScale(&t, pNormal, d_2);
        v.x = g_pCurrentCar->frontWheelAxis.x + t.x;
        v.y = g_pCurrentCar->frontWheelAxis.y + t.y;
        v.z = g_pCurrentCar->frontWheelAxis.z + t.z;
        len = FixVecLength(&v);
        if (len == 0) {
            axis[0].x = 0;
            axis[0].y = 0;
            axis[0].z = 0;
        } else {
            FixVecScaleRecip(&axis[0], &v, len);
        }
        axis[1] = axis[0];
        if (len == 0) {
            axisValid[1] = 0;
            axisValid[0] = 0;
        }
        d_2 = -FixVecDot(pNormal, &g_pCurrentCar->rearWheelDir);
        FixVecScale(&t, pNormal, d_2);
        v.x = g_pCurrentCar->rearWheelDir.x + t.x;
        v.y = g_pCurrentCar->rearWheelDir.y + t.y;
        v.z = g_pCurrentCar->rearWheelDir.z + t.z;
        len = FixVecNormalizeLen(&dir[2], &v);
        dir[3] = dir[2];
        if (len == 0) {
            dirValid[3] = 0;
            dirValid[2] = 0;
        }
        FixVecCross(&g_tyreForce, &g_pCurrentCar->rearWheelDir, &g_pCurrentCar->up);
                d_2 = -FixVecDot(pNormal, &g_tyreForce);
        FixVecScale(&t, pNormal, d_2);
                v.z = g_tyreForce.z + t.z;
                v.y = g_tyreForce.y + t.y;
                v.x = g_tyreForce.x + t.x;
        len = FixVecNormalizeLen(&axis[2], &v);
        axis[3] = axis[2];
        if (len == 0) {
            axisValid[3] = 0;
            axisValid[2] = 0;
        }
        g_pCurrentCar->groundDir[0] = dir[0];
        g_pCurrentCar->groundDir[1] = dir[2];
        g_pCurrentCar->groundAxis[0] = axis[0];
        g_pCurrentCar->groundAxis[1] = axis[2];

                g_tyreGripScale = FixMul(g_pCurrentCar->tyreGrip, g_physicsScale);
                g_tyreSlipScale = FixMul(g_physicsScale, 0xf000);
        g_wheelSpinStep = FixMul(g_physicsTimeStep, 0x11113);

        // Wheel spin: brakes, then the drive torque limited by the grip.
        for (i = 3; i >= 0; i--) {
            if (g_pCurrentCar->cornerOnGround[i] != 0) {
                resist = g_physicsPartSet->wheelDamageDrag[i];
                WHEEL_BRAKE(i, resist);
                if (dirValid[i] != 0) {
                    speed = FixVecDot(&g_pCurrentCar->cornerVelocity[i], &dir[i]);
                    rollSpeed[i] = speed;
                    spinSlip = speed * 4 - g_pCurrentCar->wheelLoad[i];
                    crossing = FALSE;
                    if ((speed > 0 && spinSlip < 0) || (speed < 0 && spinSlip > 0))
                        crossing = TRUE;
                    g_pCurrentCar->wheelSlipping[i] = 0;
                    if (speed * 4 > 0) {
                        if (g_pCurrentCar->wheelLoad[i] > speed * 4 || g_pCurrentCar->wheelLoad[i] < -0x1999)
                            g_pCurrentCar->wheelSlipping[i] = 1;
                    } else if (g_pCurrentCar->wheelLoad[i] < speed * 4 || g_pCurrentCar->wheelLoad[i] > 0x1999) {
                        g_pCurrentCar->wheelSlipping[i] = 1;
                    }
                    if (i > 1 && g_pCurrentCar->handbrake != 0 && !crossing) {
                        g_pCurrentCar->wheelTorque[i] = 0;
                        WHEEL_BRAKE(i, resist);
                    } else {
                        longForce = FixMul(spinSlip, g_tyreSlipScale) * 4;
                        if (g_pCurrentCar->flag0x1d0[2] == 0 && g_pCurrentCar->flag0x1d0[3] == 0) {
                            grip = g_pCurrentCar->field_0xa5c[i];
                        } else {
                            resist = FixMul(FIX_ABS(spinSlip), 0x4000);
                            resist -= FIX_ABS(FixVecDot(&g_pCurrentCar->cornerVelocity[i], &axis[i]));
                            if (resist < 0)
                                resist = 0;
                            resist = FixMul(resist, 0xccc) + 0x51e;
                            if (g_pCurrentCar->cornerGrip[i].drag != 0)
                                resist += FixMul(0xe666, g_pCurrentCar->cornerGrip[i].drag);
                            grip = FixMul(g_pCurrentCar->field_0xa5c[i], resist);
                        }
                        if (longForce > grip || (grip = -grip, longForce < grip))
                            longForce = grip;
                        if (g_pCurrentCar->flag0x1d0[2] == 0 && g_pCurrentCar->flag0x1d0[3] == 0)
                            spinSlip = longForce / 4;
                        else
                            spinSlip = longForce / 8;
                        if (g_pCurrentCar->field_0xb1f != 0)
                            spinSlip = FixMul(spinSlip, 0x1999);
                        g_pCurrentCar->wheelTorque[i] += spinSlip;
                    }
                }
            }
            WHEEL_SPIN(i);
        }
        Car_BalanceWheelPairs();

        // Tyre forces.
        for (i = 3; i >= 0; i--) {
            if (g_pCurrentCar->cornerOnGround[i] == 0)
                continue;
            if (dirValid[i] != 0) {
                slip = rollSpeed[i] * 4 - g_pCurrentCar->wheelLoad[i];
                g_pCurrentCar->wheelSlip[i] = slip;
                if (FIX_ABS(slip) < 0x10000) {
                    d_2 = FixVecDot(&g_carStepAccel, &dir[i]);
                    longAssist = -d_2;
                    e = FixMul(rollSpeed[i], g_tyreGripScale);
                    if ((longAssist > 0 && e > 0) || (longAssist < 0 && e < 0)) {
                        if (FIX_ABS(longAssist) > FIX_ABS(e))
                            longAssist -= e;
                        else
                            longAssist = 0;
                    }
                    gear = g_pCurrentCar->gearRatio[g_pCurrentCar->gear];
                    if (((gear > 0 && longAssist < 0) || (gear < 0 && longAssist > 0) || gear == 0) &&
                        g_pCurrentCar->brakeInput == 0)
                        longAssist = 0;
                }
                longForce = -FixMul(FixMul(slip, 0x4000), g_tyreGripScale);
            }
            if (axisValid[i] != 0) {
                speed = FixVecDot(&g_pCurrentCar->cornerVelocity[i], &axis[i]);
                g_pCurrentCar->wheelSlipLateral[i] = speed;
                if (FIX_ABS(speed) < 0x10000)
                    latAssist = -FixVecDot(&g_carStepAccel, &axis[i]);
                latForce = -FixMul(speed, g_tyreGripScale);
            }
            longForce += longAssist;
            latForce += latAssist;
            if (FIX_ABS(longForce) > 0x10000 && FIX_ABS(longForce) >= FIX_ABS(latForce)) {
                g_tyreForceMax = FIX_ABS(longForce);
                g_tyreForceRecip = FixDiv(0x10000, g_tyreForceMax);
                e = FixMul(longForce, g_tyreForceRecip);
                FixVecScale(&g_tyreLongForce, &dir[i], e);
                FixVecScale(&g_tyreLatForce, &axis[i], FixMul(latForce, g_tyreForceRecip));
                g_tyreForce.x = g_tyreLongForce.x + g_tyreLatForce.x;
                g_tyreForce.y = g_tyreLongForce.y + g_tyreLatForce.y;
                g_tyreForce.z = g_tyreLongForce.z + g_tyreLatForce.z;
                resist = FixMul(FIX_SQR(FixVecNormalizeLen(&g_pCurrentCar->cornerForce[i], &g_tyreForce)), g_tyreForceMax);
            } else if (FIX_ABS(latForce) > 0x10000 && FIX_ABS(latForce) >= FIX_ABS(longForce)) {
                g_tyreForceMax = FIX_ABS(latForce);
                g_tyreForceRecip = FixDiv(0x10000, g_tyreForceMax);
                e = FixMul(longForce, g_tyreForceRecip);
                FixVecScale(&g_tyreLongForce, &dir[i], e);
                FixVecScale(&g_tyreLatForce, &axis[i], FixMul(latForce, g_tyreForceRecip));
                g_tyreForce.x = g_tyreLongForce.x + g_tyreLatForce.x;
                g_tyreForce.y = g_tyreLongForce.y + g_tyreLatForce.y;
                g_tyreForce.z = g_tyreLongForce.z + g_tyreLatForce.z;
                resist = FixMul(FIX_SQR(FixVecNormalizeLen(&g_pCurrentCar->cornerForce[i], &g_tyreForce)), g_tyreForceMax);
            } else {
                FixVecScale(&g_tyreLongForce, &dir[i], longForce);
                FixVecScale(&g_tyreLatForce, &axis[i], latForce);
                g_tyreForce.x = g_tyreLongForce.x + g_tyreLatForce.x;
                g_tyreForce.y = g_tyreLongForce.y + g_tyreLatForce.y;
                g_tyreForce.z = g_tyreLongForce.z + g_tyreLatForce.z;
                resist = FIX_SQR(FixVecNormalizeLen(&g_pCurrentCar->cornerForce[i], &g_tyreForce));
            }

            // Friction ellipse of the tyre.
            if (longForce < 0)
                longForce = -longForce;
            if (latForce < 0)
                latForce = -latForce;
            if (i < 2) {
                d_2 = g_pCurrentCar->cornerGrip[i].gripB + g_pCurrentCar->wheelSurfaceFx[i].extraGrip;
                b = g_pCurrentCar->cornerGrip[i].grip2B + g_pCurrentCar->wheelSurfaceFx[i].extraGrip;
            } else {
                d_2 = g_pCurrentCar->cornerGrip[i].gripB - 0x4ccc + g_pCurrentCar->wheelSurfaceFx[i].extraGrip;
                b = g_pCurrentCar->cornerGrip[i].grip2B - 0x4ccc + g_pCurrentCar->wheelSurfaceFx[i].extraGrip;
            }
            // brakeRear is free by now; the original keeps the first limit in it.
            if (longForce < 0x6666) {
                limB = FixMul(g_pCurrentCar->field_0xa5c[i], b);
                brakeRear = FixMul(g_pCurrentCar->field_0xa4c[i], d_2);
            } else if (latForce < 0x6666) {
                limB = g_pCurrentCar->field_0xa5c[i];
                brakeRear = g_pCurrentCar->field_0xa4c[i];
            } else {
                angle = FixAtan2(FixMul(longForce, d_2), latForce);
                d_2 = FixMul(d_2, g_sinTable[(angle + 0x400) & 0xfff]);
                s = g_sinTable[angle & 0xfff];
                brakeRear = FixSqrt(FixMul(d_2, d_2) + FixMul(s, s));
                brakeRear = FixMul(brakeRear, g_pCurrentCar->field_0xa4c[i]);
                angle = FixAtan2(FixMul(longForce, b), latForce);
                d = FixMul(b, g_sinTable[(angle + 0x400) & 0xfff]);
                s = g_sinTable[angle & 0xfff];
                limB = FixMul(FixSqrt(FixMul(d, d) + FixMul(s, s)), g_pCurrentCar->field_0xa5c[i]);
            }
            if (resist > brakeRear)
                resist = limB;
            FixVecScale(&g_pCurrentCar->cornerForce[i], &g_pCurrentCar->cornerForce[i], resist);

            // Rolling resistance of the tyre.
            if (g_pCurrentCar->cornerGrip[i].drag != 0) {
                d_2 = FixVecDot(&dir[i], &g_pCurrentCar->cornerVelocity[i]);
                if (d_2 != 0) {
                    longForce = -FixMul(d_2 / 4, FixMul(g_pCurrentCar->tyreGrip, g_pCurrentCar->cornerGrip[i].drag));
                    FixVecScale(&g_tyreForce, &dir[i], longForce);
                    (&g_pCurrentCar->cornerForce[i])->x += g_tyreForce.x;
                    (&g_pCurrentCar->cornerForce[i])->y += g_tyreForce.y;
                    (&g_pCurrentCar->cornerForce[i])->z += g_tyreForce.z;
                }
            }
        }
        Car_ApplyCornerFriction(g_tyreGripScale);
    } else {
        g_wheelSpinStep = FixMul(g_physicsTimeStep, 0xa0000);
        for (i = 3; i >= 0; i--) {
            resist = g_physicsPartSet->wheelDamageDrag[i];
            WHEEL_BRAKE(i, resist);
            WHEEL_SPIN(i);
        }
        Car_BalanceWheelPairs();
    }
}


int CarPart_GetHiddenState(Car *pCar, int slot);

extern double g_fixedDegreesToAngle12;

// Orientation of a lean vector: up along (lean.x, 1, lean.z) with the tilt
// limited to about 0.09, right the X axis made perpendicular to it.
#define LEAN_BASIS(lean)                                                     \
    t = (lean);                                                              \
    if (FIX_ABS(t.x) > 0x170a)                                               \
        t.x = t.x > 0 ? 0x170a : -0x170a;                                    \
    if (FIX_ABS(t.z) > 0x170a)                                               \
        t.z = t.z > 0 ? 0x170a : -0x170a;                                    \
    t.y = 0x10000;                                                           \
    FIX_NORMALIZE_INTO(up, t);                                               \
    xAxis.x = 0x10000;                                                       \
    xAxis.y = 0;                                                             \
    xAxis.z = 0;                                                             \
    FixVecScale(&t, &up, FixVecDot(&up, &xAxis));                            \
    t.x = xAxis.x - t.x;                                                     \
    t.y = xAxis.y - t.y;                                                     \
    t.z = xAxis.z - t.z;                                                     \
    FIX_NORMALIZE_INTO(right, t);                                            \
    FixVecCross(&t, &right, &up);                                            \
    FIX_NORMALIZE_INTO(fwd, t);                                              \
    m.right = right;                                                         \
    m.up = up;                                                               \
    m.forward = fwd

// Damping of a lean vector: against it, proportional to its length.
#define LEAN_DAMP(lean, rate)                                                \
    len = FixVecLength(&(lean));                                             \
    if (len > 0) {                                                           \
        k = -FixMul(len, (rate));                                            \
        FixVecScaleRecip(&damp, &(lean), len);                               \
        FixVecScale(&damp, &damp, k);                                        \
    } else {                                                                 \
        damp.x = 0;                                                          \
        damp.y = 0;                                                          \
        damp.z = 0;                                                          \
    }

// Starts the first pending countdown of the current car (200 steps).
// FUNCTION: CMR2 0x0043b020
void Car_StartPendingCountdown(void)
{
    if (g_pCurrentCar->field_0xbdc != 0) {
        g_pCurrentCar->field_0xbfc = 1;
        g_pCurrentCar->field_0xb25 = 200;
        return;
    }
    if (g_pCurrentCar->field_0xbe0 != 0) {
        g_pCurrentCar->field_0xbfc = 1;
        g_pCurrentCar->field_0xb26 = 200;
        return;
    }
    if (g_pCurrentCar->field_0xbe4 != 0) {
        g_pCurrentCar->field_0xbfc = 1;
        g_pCurrentCar->field_0xb27 = 200;
    }
}

// Rates of the car scaled by the physics time step.
// FUNCTION: CMR2 0x0043b090
void Car_ScaleRatesByTimeStep(Car *pCar)
{
    pCar->throttleRampStep = FixMul(0xccc, g_physicsTimeStep);
    pCar->brakeRampStep = FixMul(0xccc, g_physicsTimeStep);
    pCar->handbrakeRampStep = FixMul(0x1999, g_physicsTimeStep);
    pCar->steeringReturnRate = FixMul(0x3333, g_physicsTimeStep);
}

inline void Car_NormalizeForceVector(FixVector *pOut, FixVector *pV)
{
    int len = FixVecLength(pV);
    if (len == 0) {
        pOut->x = 0;
        pOut->y = 0;
        pOut->z = 0;
    } else {
        FixVecScaleRecip(pOut, pV, len);
    }
}

// Suspension of the car: two lean vectors (the wheel frame and the body)
// are pushed by the car's acceleration and damped, and the height of each
// corner of the body under the leaned frames gives the suspension travel of
// that corner. The body lean also rolls with the steering on the cars that
// use it.
// FUNCTION: CMR2 0x0043b100
void Car_UpdateSuspension(void)
{
    FixMatrix m;
    FixVector box[4];
    FixVector basis[3];
    FixVector damp;
    FixVector delta;
    FixVector accel;
    FixVector scaled;
    FixVector diff;
    FixVector flat;
    FixVector *pV;
    int len;
    int k;
    int i;
    short angle;

    if (g_pCurrentCar->field_0xb28 != 0 && g_pCurrentCar->field_0xb74 != 0) {
        diff.x = g_pCurrentCar->velocityNext.x - g_pCurrentCar->velocity.x;
        diff.y = g_pCurrentCar->velocityNext.y - g_pCurrentCar->velocity.y;
        diff.z = g_pCurrentCar->velocityNext.z - g_pCurrentCar->velocity.z;
        FixMatrix_InverseRotateVector(&accel, &diff, g_pCurrentCar->pWorld);
        len = FixVecLength(&accel);
        accel.y = 0;
        if (len == 0 || g_pCurrentCar->field_0xb60 != 0) {
            accel.x = 0;
            accel.y = 0;
            accel.z = 0;
        }
        FixVecScale(&scaled, &accel, g_physicsScale);
    } else {
        accel.x = 0;
        accel.y = 0;
        accel.z = 0;
        scaled = accel;
    }

    // Wheel frame.
    LEAN_DAMP(g_pCurrentCar->wheelLean, g_pCurrentCar->wheelLeanDamping);
    delta.x = damp.x + accel.x;
    delta.y = damp.y + accel.y;
    delta.z = damp.z + accel.z;
    FixVecScale(&delta, &delta, FixMul(g_physicsTimeStep, 0x13333));
    flat = delta;
    flat.y = 0;
    len = FixVecLength(&flat);
    if (len > 0x312) {
        FixVecScaleRecip(&flat, &flat, len);
        FixVecScale(&flat, &flat, 0x312);
        delta.x = flat.x;
        delta.z = flat.z;
    }
    g_pCurrentCar->wheelLean.x += delta.x;
    g_pCurrentCar->wheelLean.y += delta.y;
    g_pCurrentCar->wheelLean.z += delta.z;
    accel = (g_pCurrentCar->wheelLean);
    if (FIX_ABS(accel.x) > 0x170a)
        accel.x = accel.x > 0 ? 0x170a : -0x170a;
    if (FIX_ABS(accel.z) > 0x170a)
        accel.z = accel.z > 0 ? 0x170a : -0x170a;
    accel.y = 0x10000;
    Car_NormalizeForceVector(&basis[1], &accel);
    basis[0].x = 0x10000;
    basis[0].y = 0;
    basis[0].z = 0;
    len = FixVecDot(&basis[0], &basis[1]);
    FixVecScale(&flat, &basis[1], len);
    flat.x = basis[0].x - flat.x;
    flat.y = basis[0].y - flat.y;
    flat.z = basis[0].z - flat.z;
    Car_NormalizeForceVector(&basis[0], &flat);
    FixVecCross(&basis[2], &basis[0], &basis[1]);
    Car_NormalizeForceVector(&basis[2], &basis[2]);
    m.right = basis[0];
    m.up = basis[1];
    m.forward = basis[2];
    box[0].x = g_pCurrentCar->halfExtents.x;
    box[0].y = 0;
    box[0].z = g_pCurrentCar->halfExtents.z;
    box[1].x = g_pCurrentCar->halfExtents.x;
    box[1].y = 0;
    box[1].z = -g_pCurrentCar->halfExtents.z;
    box[2].x = -g_pCurrentCar->halfExtents.x;
    box[2].y = 0;
    box[2].z = g_pCurrentCar->halfExtents.z;
    box[3].x = -g_pCurrentCar->halfExtents.x;
    box[3].y = 0;
    box[3].z = -g_pCurrentCar->halfExtents.z;
    for (i = 0; i < 4; i++) {
        FixMatrix_RotateVector(&flat, &box[i], &m);
        g_pCurrentCar->wheelLeanHeight[i] = flat.y;
        g_pCurrentCar->wheelLeanHeight[i] += g_physicsPartSet->suspensionDamageOffset[i];
    }

    // Body.
    scaled.z = FixMul(scaled.z, 0x20000);
    LEAN_DAMP(g_pCurrentCar->lean, g_pCurrentCar->bodyLeanDamping);
    delta.x = scaled.x + damp.x;
    delta.y = scaled.y + damp.y;
    delta.z = scaled.z + damp.z;
    FixVecScale(&delta, &delta, FixMul(g_physicsTimeStep, 0x13333));
    flat = delta;
    flat.y = 0;
    len = FixVecLength(&flat);
    if (len > FixMul(0x312, g_physicsTimeStep)) {
        FixVecScaleRecip(&flat, &flat, len);
        FixVecScale(&flat, &flat, FixMul(0x312, g_physicsTimeStep));
        delta.x = flat.x;
        delta.z = flat.z;
    }
    g_pCurrentCar->lean.x += delta.x;
    g_pCurrentCar->lean.y += delta.y;
    g_pCurrentCar->lean.z += delta.z;
    accel = (g_pCurrentCar->lean);
    if (FIX_ABS(accel.x) > 0x170a)
        accel.x = accel.x > 0 ? 0x170a : -0x170a;
    if (FIX_ABS(accel.z) > 0x170a)
        accel.z = accel.z > 0 ? 0x170a : -0x170a;
    accel.y = 0x10000;
    Car_NormalizeForceVector(&basis[1], &accel);
    basis[0].x = 0x10000;
    basis[0].y = 0;
    basis[0].z = 0;
    len = FixVecDot(&basis[0], &basis[1]);
    FixVecScale(&flat, &basis[1], len);
    flat.x = basis[0].x - flat.x;
    flat.y = basis[0].y - flat.y;
    flat.z = basis[0].z - flat.z;
    Car_NormalizeForceVector(&basis[0], &flat);
    FixVecCross(&basis[2], &basis[0], &basis[1]);
    Car_NormalizeForceVector(&basis[2], &basis[2]);
    m.right = basis[0];
    m.up = basis[1];
    m.forward = basis[2];
    if (g_pCurrentCar->field_0xb1d != 0 && g_pCurrentCar->gearRatio[g_pCurrentCar->gear] != 0) {
        flat.x = 0;
        flat.y = 0;
        flat.z = 0x10000;
        angle = FTOI(short, X87S_MUL((double)FixMul(g_pCurrentCar->field_0xb1d << 16, 0x3333), g_fixedDegreesToAngle12));
        ((void (__stdcall *)(FixMatrix *, FixVector *, short))FixMatrix_FromAxisAngle)(&m, &flat, angle);
        for (i = 0; i < 3; i++) {
            FixMatrix_RotateVector(&flat, &basis[i], &m);
            Car_NormalizeForceVector(&basis[i], &flat);
        }
        m.right = basis[0];
        m.up = basis[1];
        m.forward = basis[2];
    }
    for (i = 0; i < 4; i++) {
        FixMatrix_RotateVector(&flat, &box[i], &m);
        g_pCurrentCar->bodyLeanHeight[i] = flat.y;
        g_pCurrentCar->bodyLeanHeight[i] += g_physicsPartSet->suspensionDamageOffset[i];
    }
    if (g_pCurrentCar->field_0xb28 == 4) {
        Car_UpdateWheelLoads();
        return;
    } else {
        for (i = 7; i >= 0; i--)
            g_pCurrentCar->cornerWeight[i] = g_pCurrentCar->tyreGrip;
        return;
    }
}

// Bounds the suspension-height differences, converts them into shares of
// the car's mass, and biases the first wheel pair towards half the mass
// when partHidden[3] is set. These loads feed the corner grip calculation.
// FUNCTION: CMR2 0x0043c640
void Car_UpdateWheelLoads(void)
{
    int quarterMass = g_pCurrentCar->mass / 4;
    int maxHeightOffset = 0;
    int *pHeight = g_pCurrentCar->wheelLeanHeight;
    int i = 4;

    do {
        int height = *pHeight;
        if (FIX_ABS(height) > maxHeightOffset) {
            maxHeightOffset = FIX_ABS(height);
        }
        pHeight++;
        i--;
    } while (i != 0);

    if (maxHeightOffset > 0x4000) {
        int scale = FixDiv(0x4000, maxHeightOffset);
        for (i = 0; i < 4; i++) {
            g_pCurrentCar->wheelLeanHeight[i] = FixMul(g_pCurrentCar->wheelLeanHeight[i], scale);
        }
    }

    for (i = 0; i < 4; i++) {
        int heightTerm = FixMul(g_pCurrentCar->wheelLeanHeight[i], 0x40000);
        heightTerm += g_pCurrentCar->wheelTravelLoadBias[i];
        g_pCurrentCar->cornerWeight[i] = quarterMass - FixMul(heightTerm, g_pCurrentCar->suspensionLoadScale);
    }

    if (CarPart_GetHiddenState(g_pCurrentCar, 3) != 0) {
        int rate = FixMul(g_pCurrentCar->speed, 0x2000);

        if (rate > 0x10000) {
            rate = 0x10000;
        }
        int halfMass = g_pCurrentCar->mass / 2;
        // The original steps a byte offset through cornerWeight[0..3]; on
        // 64-bit targets the field lies elsewhere, so take its native offset.
        for (i = offsetof(Car, cornerWeight); i < (int)offsetof(Car, cornerWeight[4]); i += sizeof(int)) {
            int target = i < (int)offsetof(Car, cornerWeight[2]) ? halfMass : 0;
            *(int *)((BYTE *)g_pCurrentCar + i) += FixMul(rate, target - *(int *)((BYTE *)g_pCurrentCar + i));
        }
    }
}

// GLOBAL: CMR2 0x0053cc1c
CarPartSet *g_physicsPartSet;
// GLOBAL: CMR2 0x00519c88
int g_dragBase = 0x10000;

// One physics step of the rigid body: sums the corner forces into a force and
// a torque, integrates linear and angular velocity, moves the body and rebuilds
// its world matrix.
// The yaw-only path doubles corner forces 3 and 1; the full-rotation path
// accumulates all four lower corner forces. The yaw step rotates right and
// forward (body-axis rows 0 and 2), leaving the up row unchanged.
// FUNCTION: CMR2 0x00440840
void Car_Integrate(void)
{
    FixVector force;
    FixVector work;
    FixVector angularStep;
    FixVector torque;
    FixVector positionStep;
    FixVector cornerTorque;
    FixVector leverArm;
    FixVector acceleration;
    int yawOnly = 0;
    int index;
    int halfStep;
    int drag;

    if (g_pCurrentCar->angularVelocity.x == 0 && g_pCurrentCar->angularVelocity.z == 0) {
        yawOnly = 1;
    }
    force = g_pCurrentCar->baseForce;
    torque.x = 0;
    torque.y = 0;
    torque.z = 0;

    if (yawOnly != 0) {
        index = 3;
        do {
            work.x = g_pCurrentCar->cornerForce[index].x;
            work.y = g_pCurrentCar->cornerForce[index].y;
            work.z = g_pCurrentCar->cornerForce[index].z;
            force.x = force.x + work.x;
            force.y = force.y + work.y;
            force.z = force.z + work.z;
            force.x = force.x + work.x;
            force.y = force.y + work.y;
            force.z = force.z + work.z;
            torque.y = torque.y +
                       FixMul(FixVecDot(&g_pCurrentCar->cornerForce[index], &g_pCurrentCar->forward),
                              g_pCurrentCar->wheelPos[index].x) * 2;
            index -= 2;
        } while (index >= 0);
    } else {
        index = 3;
        do {
            work.x = g_pCurrentCar->cornerForce[index].x;
            work.y = g_pCurrentCar->cornerForce[index].y;
            work.z = g_pCurrentCar->cornerForce[index].z;
            force.x = force.x + work.x;
            force.y = force.y + work.y;
            force.z = force.z + work.z;
            leverArm.x = g_pCurrentCar->corners[index].x - g_pCurrentCar->position.x;
            leverArm.y = g_pCurrentCar->corners[index].y - g_pCurrentCar->position.y;
            leverArm.z = g_pCurrentCar->corners[index].z - g_pCurrentCar->position.z;
            FixVecCross(&cornerTorque, &work, &leverArm);
            torque.x = torque.x + cornerTorque.x;
            torque.y = torque.y + cornerTorque.y;
            torque.z = torque.z + cornerTorque.z;
            index--;
        } while (index >= 0);
        FixMatrix_InverseRotateVector(&work, &torque, g_pCurrentCar->pWorld);
        torque = work;
    }

    torque.y = FixMul(torque.y, 0xb333);
    FixVecScale(&force, &force, 0xd999);
    drag = FixMul(FIX_ABS(g_pCurrentCar->speed), g_physicsPartSet->bodyDamageDrag + g_dragBase);
    FixVecScale(&work, &g_pCurrentCar->velocity, drag);
    force.x = force.x - work.x;
    force.y = force.y - work.y;
    force.z = force.z - work.z;

    FixVecScale(&acceleration, &force, g_pCurrentCar->inverseMass);
    acceleration.x = acceleration.x + g_pCurrentCar->field_0x5c4.x;
    acceleration.y = acceleration.y + g_pCurrentCar->field_0x5c4.y;
    acceleration.z = acceleration.z + g_pCurrentCar->field_0x5c4.z;
    FixVecScale(&acceleration, &acceleration, 0x3333);
    g_pCurrentCar->velocityNext = g_pCurrentCar->velocity;
    FixVecScale(&acceleration, &acceleration, g_physicsTimeStep);
    g_pCurrentCar->velocity.x = g_pCurrentCar->velocity.x + acceleration.x;
    g_pCurrentCar->velocity.y = g_pCurrentCar->velocity.y + acceleration.y;
    g_pCurrentCar->velocity.z = g_pCurrentCar->velocity.z + acceleration.z;
    g_pCurrentCar->speed = FixVecLength(&g_pCurrentCar->velocity);

    FixVecScale(&positionStep, &g_pCurrentCar->velocity, g_physicsTimeStep);
    halfStep = g_physicsTimeStep / 2;
    FixVecScale(&acceleration, &acceleration, halfStep);
    positionStep.x = positionStep.x - acceleration.x;
    positionStep.y = positionStep.y - acceleration.y;
    positionStep.z = positionStep.z - acceleration.z;

    if (yawOnly != 0) {
        int angAccelY = -FixMul(torque.y, g_pCurrentCar->inertia.y);

        angAccelY = angAccelY - FixMul(g_pCurrentCar->field_0x5d0.y, FixMul(g_pCurrentCar->inertia.y, g_pCurrentCar->mass));
        angAccelY = FixMul(g_physicsTimeStep, angAccelY);
        g_pCurrentCar->angularVelocity.y = g_pCurrentCar->angularVelocity.y + angAccelY;
        angularStep.y = FixMul(g_pCurrentCar->angularVelocity.y, g_physicsTimeStep);
        halfStep = g_physicsTimeStep / 2;
        angAccelY = FixMul(halfStep, angAccelY);
        angularStep.y = angularStep.y - angAccelY;
    } else {
        FixVector angAccel;

        angAccel.x = -FixMul(g_pCurrentCar->inertia.x, torque.x);
        angAccel.y = -FixMul(torque.y, g_pCurrentCar->inertia.y);
        angAccel.z = -FixMul(g_pCurrentCar->inertia.z, torque.z);
        angAccel.x = angAccel.x - FixMul(g_pCurrentCar->field_0x5d0.x,
                                         FixMul(g_pCurrentCar->inertia.x, g_pCurrentCar->mass));
        angAccel.y = angAccel.y - FixMul(g_pCurrentCar->field_0x5d0.y,
                                         FixMul(g_pCurrentCar->inertia.y, g_pCurrentCar->mass));
        angAccel.z = angAccel.z - FixMul(g_pCurrentCar->field_0x5d0.z,
                                         FixMul(g_pCurrentCar->inertia.z, g_pCurrentCar->mass));
        FixVecScale(&angAccel, &angAccel, g_physicsTimeStep);
        g_pCurrentCar->angularVelocity.x = g_pCurrentCar->angularVelocity.x + angAccel.x;
        g_pCurrentCar->angularVelocity.y = g_pCurrentCar->angularVelocity.y + angAccel.y;
        g_pCurrentCar->angularVelocity.z = g_pCurrentCar->angularVelocity.z + angAccel.z;
        FixVecScale(&angularStep, &g_pCurrentCar->angularVelocity, g_physicsTimeStep);
        halfStep = g_physicsTimeStep / 2;
        FixVecScale(&angAccel, &angAccel, halfStep);
        angularStep.x = angularStep.x - angAccel.x;
        angularStep.y = angularStep.y - angAccel.y;
        angularStep.z = angularStep.z - angAccel.z;
    }

    if (g_pCurrentCar->field_0xb34 > 0) {
        int upDot = FixVecDot(&g_pCurrentCar->up, &g_pCurrentCar->groundNormal);
        if (FIX_ABS(angularStep.x) < 6 && FIX_ABS(upDot) > 0xcccc) {
            angularStep.x = 0;
        }
        if ((g_pCurrentCar->flag0x1d0[0] == 0 && g_pCurrentCar->flag0x1d0[1] == 0) ||
            FIX_ABS(g_pCurrentCar->speed) < 0x28f) {
            if (FIX_ABS(angularStep.y) < 6) {
                angularStep.y = 0;
            }
        }
        if (FIX_ABS(angularStep.z) < 6) {
            angularStep.z = 0;
        }
        if (g_pCurrentCar->speed < 0xccc) {
            positionStep.x = 0;
            positionStep.y = 0;
            positionStep.z = 0;
        }
    }

    if (yawOnly != 0) {
        angularStep.x = 0;
        angularStep.z = 0;
    }
    g_pCurrentCar->position.x = g_pCurrentCar->position.x + positionStep.x;
    g_pCurrentCar->position.y = g_pCurrentCar->position.y + positionStep.y;
    g_pCurrentCar->position.z = g_pCurrentCar->position.z + positionStep.z;
    g_pCurrentCar->field_0xb60 = 0;
    if (angularStep.x == 0 && angularStep.y == 0 && angularStep.z == 0) {
        if (g_pCurrentCar->speed < 0x51e) {
            g_pCurrentCar->field_0xb60 = 1;
        }
    } else if (yawOnly == 0) {
        FixBasis_Integrate(g_pCurrentCar->bodyAxisVectors, &angularStep);
    } else {
        index = 0;
        do {
            work.x = FixMul(angularStep.y, g_pCurrentCar->bodyAxisVectors[index].z);
            work.y = 0;
            work.z = -FixMul(angularStep.y, g_pCurrentCar->bodyAxisVectors[index].x);
            g_pCurrentCar->bodyAxisVectors[index].x = g_pCurrentCar->bodyAxisVectors[index].x + work.x;
            g_pCurrentCar->bodyAxisVectors[index].y = g_pCurrentCar->bodyAxisVectors[index].y + work.y;
            g_pCurrentCar->bodyAxisVectors[index].z = g_pCurrentCar->bodyAxisVectors[index].z + work.z;
            index += 2;
        } while (index < 3);
    }

    g_pCurrentCar->pWorld->right = g_pCurrentCar->right;
    g_pCurrentCar->pWorld->up = g_pCurrentCar->up;
    g_pCurrentCar->pWorld->forward = g_pCurrentCar->forward;
    g_pCurrentCar->pWorld->position = g_pCurrentCar->position;
    Car_UpdateCorners(g_pCurrentCar);
}

// Builds the force each wheel puts on the ground: first the drive/brake torque
// per wheel (with the handbrake and the traction limits), then the tyre force
// itself, combining the rolling and the lateral slip and capping it with the
// tyre's friction ellipse. Only the right hand wheels are solved; the left
// hand ones are mirrored from them. See also Car_UpdateTyreForces.
// FUNCTION: CMR2 0x00441500
void Car_UpdateWheelForces(void)
{
    FixVector wheelDir[2];
    FixVector axis[2];
    FixVector lateralAxis;
    FixVector longVec;
    FixVector latForceVec;
    FixVector direction;
    FixVector force;
    int dot[4];
    int brakeA;
    int brakeB;
    int scale;
    int loadScale;
    int torqueScale;
    int torqueLimit;
    int i;
    int combined;
    int fLong;
    int fLat;

    int recip;
    int a;
    int b;
    int sine;
    int limA;
    int limB;
    union { short angle; int raw; } trig;

    if (g_pCurrentCar->flag0x1d0[2] == 0 && g_pCurrentCar->flag0x1d0[3] == 0 &&
        g_pCurrentCar->gear != 0) {
        brakeA = FixMul(0x1999, g_pCurrentCar->brakeBias);
        brakeB = FixMul(0x1999, 0x10000 - g_pCurrentCar->brakeBias);
    } else {
        combined = FixMul(g_pCurrentCar->brakeInput, g_physicsPartSet->frontBrakeScale);
        brakeA = FixMul(combined, g_pCurrentCar->brakeBias);
        combined = FixMul(g_pCurrentCar->brakeInput, g_physicsPartSet->rearBrakeScale);
        brakeB = FixMul(combined, 0x10000 - g_pCurrentCar->brakeBias);
    }
    if (g_pCurrentCar->handbrake != 0) {
        brakeB = brakeB + g_pCurrentCar->handbrakeForce;
    }

    wheelDir[0] = g_pCurrentCar->frontWheelDir;
    axis[0] = g_pCurrentCar->frontWheelAxis;
    wheelDir[1] = g_pCurrentCar->rearWheelDir;
    FixVecCross(&axis[1], &g_pCurrentCar->rearWheelDir, &g_pCurrentCar->up);
    Car_NormalizeForceVector(&axis[1], &axis[1]);

    scale = FixMul(g_pCurrentCar->tyreGrip, g_physicsScale);
    loadScale = FixMul(g_physicsScale, 0xf000);
    torqueScale = FixMul(g_physicsTimeStep, 0x11113);
    torqueLimit = FIX_ABS(FixMul(g_pCurrentCar->engineSpeedLimit,
                                 g_pCurrentCar->gearSpeed[g_pCurrentCar->gear]));

    i = 3;
    do {
        int load;
        int slip;
        int brake;
        int limit;
        int crossing;

        direction = i < 2 ? wheelDir[0] : wheelDir[1];
        load = g_pCurrentCar->wheelLoad[(unsigned int)i];
        combined = g_physicsPartSet->wheelDamageDrag[i];
        if (load > 0) {
            brake = i < 2 ? brakeA : brakeB;
            g_pCurrentCar->wheelTorque[(unsigned int)i] = g_pCurrentCar->wheelTorque[(unsigned int)i] - brake;
            g_pCurrentCar->wheelTorque[(unsigned int)i] = g_pCurrentCar->wheelTorque[(unsigned int)i] - combined;
        } else if (load < 0) {
            brake = i < 2 ? brakeA : brakeB;
            g_pCurrentCar->wheelTorque[(unsigned int)i] = g_pCurrentCar->wheelTorque[(unsigned int)i] + brake;
            g_pCurrentCar->wheelTorque[(unsigned int)i] = g_pCurrentCar->wheelTorque[(unsigned int)i] + combined;
        }

        slip = FixVecDot(&g_pCurrentCar->cornerVelocity[i], &direction);
        load = g_pCurrentCar->wheelLoad[(unsigned int)i];
        dot[i] = slip;
        fLat = slip * 4 - load;
        crossing = 0;
        if ((slip > 0 && fLat < 0) || (slip < 0 && fLat > 0)) crossing = 1;
        if (i > 1 && g_pCurrentCar->handbrake != 0 && crossing == 0) {
            g_pCurrentCar->wheelTorque[(unsigned int)i] = 0;
            load = g_pCurrentCar->wheelLoad[(unsigned int)i];
            if (load > 0) {
                brake = i < 2 ? brakeA : brakeB;
                g_pCurrentCar->wheelTorque[(unsigned int)i] = g_pCurrentCar->wheelTorque[(unsigned int)i] - brake;
                g_pCurrentCar->wheelTorque[(unsigned int)i] = g_pCurrentCar->wheelTorque[(unsigned int)i] - combined;
            } else if (load < 0) {
                brake = i < 2 ? brakeA : brakeB;
                g_pCurrentCar->wheelTorque[(unsigned int)i] = g_pCurrentCar->wheelTorque[(unsigned int)i] + brake;
                g_pCurrentCar->wheelTorque[(unsigned int)i] = g_pCurrentCar->wheelTorque[(unsigned int)i] + combined;
            }
        } else {
            int grip;
            fLong = FixMul(fLat, loadScale) * 4;

            if (g_pCurrentCar->flag0x1d0[2] == 0 && g_pCurrentCar->flag0x1d0[3] == 0) {
                grip = g_pCurrentCar->field_0xa5c[(unsigned int)i];
            } else {
                combined = FixMul(FIX_ABS(fLat), 0x4000);
                // The original indexes axes 3 and 1. The first read overruns
                // the two-axis array into uninitialised lateral scratch.
                combined -= FIX_ABS(FixVecDot(&g_pCurrentCar->cornerVelocity[i], &axis[i]));

                if (combined < 0) {
                    combined = 0;
                }
                combined = FixMul(combined, 0xccc) + 0x51e;
                if (g_pCurrentCar->cornerGrip[i].drag != 0) {
                    combined = combined + FixMul(0xe666, g_pCurrentCar->cornerGrip[i].drag);
                }
                grip = FixMul(g_pCurrentCar->field_0xa5c[(unsigned int)i], combined);
            }
            if (fLong > grip || (grip = -grip, fLong < grip)) {
                fLong = grip;
            }
            if (g_pCurrentCar->flag0x1d0[2] == 0 && g_pCurrentCar->flag0x1d0[3] == 0) {
                fLong = fLong / 4;
            } else {
                fLong = fLong / 8;
            }
            g_pCurrentCar->wheelTorque[(unsigned int)i] = g_pCurrentCar->wheelTorque[(unsigned int)i] + fLong;
        }

        load = g_pCurrentCar->wheelLoad[(unsigned int)i];
        g_pCurrentCar->wheelLoad[(unsigned int)i] = g_pCurrentCar->wheelLoad[(unsigned int)i] +
                                      FixMul(g_pCurrentCar->wheelTorque[(unsigned int)i], torqueScale);
        if (g_pCurrentCar->wheelLoad[(unsigned int)i] > torqueLimit)
            g_pCurrentCar->wheelLoad[(unsigned int)i] = torqueLimit;
        else if (g_pCurrentCar->wheelLoad[(unsigned int)i] < -torqueLimit)
            g_pCurrentCar->wheelLoad[(unsigned int)i] = -torqueLimit;
        if (g_pCurrentCar->gear != 0 && g_pCurrentCar->shiftInProgress == 0 &&
            g_pCurrentCar->driveSplit != (i < 2 ? 0 : 0x10000)) {
            int gear = g_pCurrentCar->gearRatio[g_pCurrentCar->gear];

            if ((gear > 0 && g_pCurrentCar->wheelLoad[(unsigned int)i] < 0) ||
                (gear < 0 && g_pCurrentCar->wheelLoad[(unsigned int)i] > 0)) {
                g_pCurrentCar->wheelLoad[(unsigned int)i] = 0;
            }
        } else {
            if ((load > 0 && g_pCurrentCar->wheelLoad[(unsigned int)i] < 0) ||
                (load < 0 && g_pCurrentCar->wheelLoad[(unsigned int)i] > 0)) {
                g_pCurrentCar->wheelLoad[(unsigned int)i] = 0;
            }
        }
        i -= 2;
    } while (i >= 0);

    dot[0] = dot[1];
    dot[2] = dot[3];
    g_pCurrentCar->wheelTorque[0] = g_pCurrentCar->wheelTorque[1];
    g_pCurrentCar->wheelTorque[2] = g_pCurrentCar->wheelTorque[3];
    g_pCurrentCar->wheelLoad[0] = g_pCurrentCar->wheelLoad[1];
    g_pCurrentCar->wheelLoad[2] = g_pCurrentCar->wheelLoad[3];

    i = 3;
    do {
        int len;

        direction = i < 2 ? wheelDir[0] : wheelDir[1];
        lateralAxis = i < 2 ? axis[0] : axis[1];
        // Slip and lateral force share the original's scalar scratch slot.
        fLat = dot[i] * 4 - g_pCurrentCar->wheelLoad[(unsigned int)i];
        g_pCurrentCar->wheelSlip[i] = fLat;
        fLong = -FixMul(FixMul(fLat, 0x4000), scale);
        fLat = FixVecDot(&g_pCurrentCar->cornerVelocity[i], &lateralAxis);
        g_pCurrentCar->wheelSlipLateral[i] = fLat;
        fLat = -FixMul(fLat, scale);

        if (FIX_ABS(fLong) > 0x10000 && FIX_ABS(fLong) >= FIX_ABS(fLat)) {
            combined = FIX_ABS(fLong);
            recip = FixDiv(0x10000, combined);
            FixVecScale(&longVec, &direction, FixMul(fLong, recip));
            FixVecScale(&latForceVec, &lateralAxis, FixMul(fLat, recip));
            force.x = longVec.x + latForceVec.x;
            force.y = longVec.y + latForceVec.y;
            force.z = longVec.z + latForceVec.z;
            combined = FixMul(FIX_SQR(FixVecNormalizeLen(&g_pCurrentCar->cornerForce[i], &force)), combined);
        } else if (FIX_ABS(fLat) > 0x10000 && FIX_ABS(fLat) >= FIX_ABS(fLong)) {
            combined = FIX_ABS(fLat);
            recip = FixDiv(0x10000, combined);
            FixVecScale(&longVec, &direction, FixMul(fLong, recip));
            FixVecScale(&latForceVec, &lateralAxis, FixMul(fLat, recip));
            force.x = longVec.x + latForceVec.x;
            force.y = longVec.y + latForceVec.y;
            force.z = longVec.z + latForceVec.z;
            combined = FixMul(FIX_SQR(FixVecNormalizeLen(&g_pCurrentCar->cornerForce[i], &force)), combined);
        } else {
            FixVecScale(&longVec, &direction, fLong);
            FixVecScale(&latForceVec, &lateralAxis, fLat);
            force.x = longVec.x + latForceVec.x;
            force.y = longVec.y + latForceVec.y;
            force.z = longVec.z + latForceVec.z;
            combined = FIX_SQR(FixVecNormalizeLen(&g_pCurrentCar->cornerForce[i], &force));
        }

        // Friction ellipse of the tyre, wider on the easier grip settings.
        {
            if (fLong < 0) fLong = -fLong;
            if (fLat < 0) fLat = -fLat;
            if (i < 2) {
                a = g_pCurrentCar->cornerGrip[i].gripB + g_pCurrentCar->wheelSurfaceFx[i].extraGrip;
                b = g_pCurrentCar->cornerGrip[i].grip2B + g_pCurrentCar->wheelSurfaceFx[i].extraGrip;
            } else {
                a = g_pCurrentCar->cornerGrip[i].gripB - 0x4ccc + g_pCurrentCar->wheelSurfaceFx[i].extraGrip;
                b = g_pCurrentCar->cornerGrip[i].grip2B - 0x4ccc + g_pCurrentCar->wheelSurfaceFx[i].extraGrip;
            }
            if (CGameInfo::GetConfiguredDifficulty() == 2) {
                if ((BYTE)RallyData_GetSelectionBits10To11() == 0 && (BYTE)RallyData_GetSelectionBits12To13() == 1) {
                    b = FixMul(b, 0x18000);
                    a = FixMul(a, 0x18000);
                }
                if ((BYTE)RallyData_GetSelectionBits10To11() == 0 && (BYTE)RallyData_GetSelectionBits12To13() == 2) {
                    b = FixMul(b, 0x13333);
                    a = FixMul(a, 0x13333);
                }
            }
            if (fLong < 0x6666) {
                limB = FixMul(g_pCurrentCar->field_0xa5c[(unsigned int)i], b);
                limA = FixMul(g_pCurrentCar->field_0xa4c[i], a);
            } else if (fLat < 0x6666) {
                limB = g_pCurrentCar->field_0xa5c[(unsigned int)i];
                limA = g_pCurrentCar->field_0xa4c[i];
            } else {
                trig.angle = FixAtan2(FixMul(fLong, a), fLat);
                a = FixMul(a, g_sinTable[(trig.raw + 0x400) & 0xfff]);
                sine = g_sinTable[trig.raw & 0xfff];
                limA = FixMul(FixSqrt(FixMul(a, a) + FixMul(sine, sine)), g_pCurrentCar->field_0xa4c[i]);
                trig.angle = FixAtan2(FixMul(fLong, b), fLat);
                // The longitudinal coefficient is dead after the first limit.
                a = FixMul(b, g_sinTable[(trig.raw + 0x400) & 0xfff]);
                sine = g_sinTable[trig.raw & 0xfff];
                limB = FixMul(FixSqrt(FixMul(a, a) + FixMul(sine, sine)), g_pCurrentCar->field_0xa5c[(unsigned int)i]);
            }
            if (combined > limA) {
                combined = limB;
            }
            FixVecScale(&g_pCurrentCar->cornerForce[i], &g_pCurrentCar->cornerForce[i], combined);
        }

        if (g_pCurrentCar->cornerGrip[i].drag != 0) {
            int along = FixVecDot(&direction, &g_pCurrentCar->cornerVelocity[i]);

            if (along != 0) {
                fLong = -FixMul(along / 4,
                                   FixMul(g_pCurrentCar->tyreGrip,
                                          g_pCurrentCar->cornerGrip[i].drag));

                FixVecScale(&force, &direction, fLong);
                g_pCurrentCar->cornerForce[i].x += force.x;
                g_pCurrentCar->cornerForce[i].y += force.y;
                g_pCurrentCar->cornerForce[i].z += force.z;
            }
        }
        i -= 2;
    } while (i >= 0);

    g_pCurrentCar->wheelSlip[0] = g_pCurrentCar->wheelSlip[1];
    g_pCurrentCar->wheelSlip[2] = g_pCurrentCar->wheelSlip[3];
    g_pCurrentCar->cornerForce[0] = g_pCurrentCar->cornerForce[1];
    g_pCurrentCar->cornerForce[2] = g_pCurrentCar->cornerForce[3];
}

// GLOBAL: CMR2 0x0053a3a8
CarTransforms g_carPhysicsTransforms[8];
// GLOBAL: CMR2 0x0053acc8
BYTE g_carDrawnFlags[16];
// Invalidate interpolation from the old physics row: the next draw copies the
// latest shadow into it before interpolation. Eight entries, indexed by car id.
// GLOBAL: CMR2 0x0053acf0
int g_carTransformsInvalid[8];
// GLOBAL: CMR2 0x0053ad10
CarTransforms g_carTransformsShadow[8];
// GLOBAL: CMR2 0x0053c5a0
FixVector g_carRenderBoxCorners[10][8];
// GLOBAL: CMR2 0x0053c9a8
int g_unk0x0053c9a8;

// GLOBAL: CMR2 0x0053a230
short g_wheelRotation[8][4];

extern int g_unk0x005199c8[14];
extern int g_unk0x00519a00[14];
CarPartSet *CarPart_GetSet(int carIndex);

// Rebuilds the transforms of the cars of a list: the previous rows are
// restored into the physics rows and moved by the ride height, and the wheel
// records of each car are refreshed from its setup record.
// Differentially checked against the original with real helpers and guarded
// transforms. Wheel angle inputs are at setup +0x240; suspension is at +0x250.
// FUNCTION: CMR2 0x0042af50
void Car_StoreRenderTransforms(short *pList, short count)
{
    int i;
    int j;
    int car;
    BYTE type;
    int factor;
    int sum;
    short typeAngle;
    short steering;
    CarPartSet *pRecord;
    Car *pCar;
    CarTransforms *pRow;
    CarTransforms *pShadow;
    CarWheelRecord *pWheels;
    FixVector up;
    FixVector ride;
    FixVector pos;

    for (i = count - 1; i >= 0; i--) {
        car = pList[i];
        pCar = &g_carBuffer[car];
        type = StageTiming_GetStartTableRecord(pCar->index)->modelClass;
        pRow = &g_carPhysicsTransforms[car];
        pShadow = &g_carTransformsShadow[car];
        memcpy(&pRow->body, &pShadow->body, sizeof(FixMatrix));
        memcpy(&pRow->body2, &pShadow->body2, sizeof(FixMatrix));
        for (j = 0; j < 4; j++) {
            memcpy(&pRow->wheels[j], &pShadow->wheels[j], sizeof(CarWheelRecord));
            pRow->cornerHeight[j] = pShadow->cornerHeight[j];
        }
        pRow->groundNormal = pShadow->groundNormal;
        if (pCar->groundHeightCorrection != 0) {
            pCar->physicsMatrix.position.y += pCar->groundHeightCorrection;
            pCar->bodyMatrix.position.y += pCar->groundHeightCorrection;
        }
        FixMatrix_GetUp(&up, &pCar->physicsMatrix);
        if (type != pCar->type) {
            FixVecScale(&up, &up, g_unk0x00519a00[type] + g_unk0x005199c8[pCar->type]);
        } else {
            FixVecScale(&up, &up, g_unk0x005199c8[pCar->type]);
        }
        if (CGameInfo::IsActiveCheatEnabled(6) != 0) {
            FixMatrix_GetUp(&ride, &pCar->pSceneRoot->current);
            FixVecScale(&ride, &ride, FixMul(pCar->cheatBodyLift, 0x8000));
            up.x += ride.x;
            up.y += ride.y;
            up.z += ride.z;
        }
        FixMatrix_CopyRotation(&pCar->physicsMatrix, &pShadow->body);
        FixMatrix_CopyRotation(&pCar->bodyMatrix, &pShadow->body2);
        pShadow->groundNormal = pCar->groundNormal;
        for (j = 0; j < 4; j++)
            pShadow->cornerHeight[j] = pCar->cornerHeight[j];
        FixMatrix_GetPosition(&pos, &pShadow->body);
        pos.x += up.x;
        pos.y += up.y;
        pos.z += up.z;
        FixMatrix_SetPosition(&pos, &pShadow->body);
        FixMatrix_GetPosition(&pos, &pShadow->body2);
        pos.x += up.x;
        pos.y += up.y;
        pos.z += up.z;
        FixMatrix_SetPosition(&pos, &pShadow->body2);
    }
    for (i = count - 1; i >= 0; i--) {
        car = pList[i];
        pCar = &g_carBuffer[car];
        pShadow = &g_carTransformsShadow[car];
        // One 0x18-byte record per wheel at +0x80: hub position, then the
        // three wheel angles. The original walks wheels 3..0 with every
        // pointer going down, so record j always belongs to wheel j (the
        // steering angle goes to the front wheels 0 and 1).
        pWheels = pShadow->wheels;
        steering = pCar->renderSteeringAngle;
        if (pCar->field_0xb70 != 0) {
            for (j = 3; j >= 0; j--)
                pWheels[j].position = pCar->wheelEmitter[j];
        } else {
            pRecord = CarPart_GetSet(car);
            for (j = 3; j >= 0; j--) {
                typeAngle = (short)(int)FTOI(__int64, X87S_MUL((double)FixMul(0x50000,
                        pRecord->damageValues[j]), g_fixedDegreesToAngle12));
                if (j < 2) {
                    pWheels[j].tiltDegrees = typeAngle * 0x1680;
                    pWheels[j].steeringDegrees = steering * 0x1680;
                    pWheels[j].spinDegrees = g_wheelRotation[car][j] * 0x1680;
                } else {
                    pWheels[j].tiltDegrees = typeAngle * 0x1680;
                    pWheels[j].steeringDegrees = 0;
                    pWheels[j].spinDegrees = g_wheelRotation[car][j] * 0x1680;
                }
                up = pCar->wheelEmitter[j];
                up.y += pCar->field_0x928[j];
                sum = (CarPart_GetSet(pCar->index))->damageValues[4 + j] +
                      pCar->field_0x978[j];
                if (sum > 0x10000)
                    sum = 0x10000;
                up.y += FixMul(0x10000 - sum, pCar->field_0x938[j]);
                up.x += pCar->wheelOffset[j][0];
                up.y += pCar->wheelOffset[j][1];
                pWheels[j].position = up;
            }
        }
    }
}

// FUNCTION: CMR2 0x0042b5b0
void Car_ClearWheelRotation(int first, int count)
{
    int n = first + count;

    if (first < n) {
        short *p = &g_wheelRotation[first][1];

        n -= first;
        do {
            p[-1] = 0;
            p[0] = 0;
            p[1] = 0;
            p[2] = 0;
            p += 4;
            n--;
        } while (n != 0);
    }
}

void Graphics_SetPlaybackRate(float value);

// FUNCTION: CMR2 0x0042b7e0
void Car_ResetPlaybackSpeed(void)
{
    Graphics_SetPlaybackRate(100.0f);
    g_unk0x0053c9a8 = 6;
}

unsigned short Input_GetControllerSlotMapping(unsigned short);
void Car_SetDrawnFlag(int, char);
void Car_ResetRenderTransforms(void);

// GLOBAL: CMR2 0x0053a314
short g_unk0x0053a314[8];
// GLOBAL: CMR2 0x0053bd6c
short g_unk0x0053bd6c[26];
// GLOBAL: CMR2 0x0053c9a0
short g_unk0x0053c9a0;

// Builds the race car order for the given number of cars and clears the
// per-car "drawn" flags.
// match 80%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0042b660
void Car_BuildRaceOrder(int count)
{
    int i;
    int j;

    for (i = 0; i < count; i++) {
        g_carOrder[i] = i;
        g_unk0x0053a314[i] = i;
        g_unk0x0053bd6c[i] = i;
    }
    g_carOrderCount = count;
    g_unk0x0053c9a0 = count;
    g_carOrder[44] = count;
    g_carOrder[25] = count;
    memset(g_carDrawnFlags, 0, 8);
    for (j = 0; j < 2; j++) {
        g_carDrawnFlags[j] = Input_GetControllerSlotMapping(j);
        if (!CGameInfo::GetGameModeOptionBit19() && j >= (int)(BYTE)RallyDataState())
            Car_SetDrawnFlag(j, -1);
    }
    Car_ResetRenderTransforms();
}

// FUNCTION: CMR2 0x0042b6f0
short *Car_GetOrder(void)
{
    return g_carOrder;
}

// FUNCTION: CMR2 0x0042b700
short Car_GetOrderCount(void)
{
    return g_carOrderCount;
}

// FUNCTION: CMR2 0x0042b710
BYTE Car_GetDrawnFlag(int index)
{
    return g_carDrawnFlags[index];
}

// FUNCTION: CMR2 0x0042b720
void Car_SetDrawnFlag(int index, char value)
{
    g_carDrawnFlags[index] = value;
}

void Collision_BuildOrientedBoxWorldCorners(FixVector *pCorners, FixVector *pCenter, FixMatrix *pRot, FixVector *pHalf);

void NetRace_DrainPendingMessages(void);
void Replay_AdvanceLiveRecordingStreams(void);
void Replay_PlaybackAllStreamFrames(void);
void NetRace_ReceiveAndIntegrateListedCars(Car *pCars, short *pIndices, short count);
void Collision_UpdateOrderedCars(Car *pCars, short *pOrder, short count);
void StageObject_UpdateMovingTransforms(void);
void NetRace_ExtrapolateOrderedCarPoses(Car *pCars, short *pOrder, short count);
void Replay_RecordAllStreamFrames(void);
void Car_PrepareStep(Car *base, short *pList, short count);
void Replay_RecordPeriodicCarSamples(void);
void CarPart_UpdateOrderedCars(Car *pCars, short *pOrder, short count);
void StageObject_UpdateEnabledCornerContactFrames(Car *base, short *pList, short count);
void ForceFeedback_UpdatePlayerSlot(int param_1);
void NetRace_PackListedCars(Car *base, short *pList, short count);
void Car_RunStepPasses(Car *carBase, short *pOrder, short count);
void Car_UpdateSuspensionPass(Car *param_1, short *param_2, short param_3);
void Car_StepAll(Car *base, short *pList, short count);
void Car_PrepareBodies(Car *base, short *pList, short count);
void Car_IntegrateWheelRotation(int *pList, short count);

void Car_ClearRecords(int first, int count);
void Car_InvalidateTransformsRange(int first, int count);
void StageTiming_PlaceCarViewNodesAndBody(SceneNode *pNodeA, SceneNode *pNodeB, int carIndex, int param_4,
                  FixAngles *pAngles, FixVector *pPosition, int param_7);
void CarPart_ClearSetRange(int first, int count);
void CarDamage_SetupOrderedCarParts(int lock, int keep, short *pOrder, short count);
void CarTyres_CommitPlayerWear(int a, int b, int count);
FixAngles *StageTiming_GetStartTableField244(int unused);
int *StageTiming_GetCarStartPosition(int car);
void Events_Flush(void);
void Particle_KillAll(void);
void Car_SyncBodySceneNodes(Car *pCar);
unsigned int RallyData_GetSelectionFlag27(void);
void RallyData_ResetRaceRecordAndRouteProbe(int index);

// Scratch copies of the node pointers and the engine state of every car, saved
// while the race order is rebuilt and restored afterwards (0x42b800).
// GLOBAL: CMR2 0x0053a280
SceneNode *g_savedCarExtraNodes[8][4];
// GLOBAL: CMR2 0x0053a300
short g_unk0x0053a300[8];
// GLOBAL: CMR2 0x0053a378
SceneNode *g_savedCarSceneRoots[8];
// GLOBAL: CMR2 0x0053a398
BYTE g_unk0x0053a398[8];
// GLOBAL: CMR2 0x0053ab88
int g_unk0x0053ab88[8];
// GLOBAL: CMR2 0x0053abc8
SceneNode *g_savedCarWheelNodes[8][4];
// GLOBAL: CMR2 0x0053acd0
SceneNode *g_savedCarFarViewNodes[8];
// GLOBAL: CMR2 0x0053b510
SceneNode *g_savedCarBodyNodes[8];
// GLOBAL: CMR2 0x0053b538
SceneNode *g_savedCarNearViewNodes[8];
// GLOBAL: CMR2 0x0053bd44
int g_unk0x0053bd44[8];
// GLOBAL: CMR2 0x0053bd7c
SceneNode *g_savedCarAlternateBodyNodes[8];

// Rebuilds the node pointers and the engine state of the given cars: saves them
// into the scratch rows, reloads the models (which clears the rows), writes the
// saved values back, refreshes the scene nodes, the surfaces and the renderer,
// and finally restores the race order.
// FUNCTION: CMR2 0x0042b800
void Car_ReloadModels(int first, int count, int param_3)
{
    int end = first + count;
    int i;
    int k;

    for (i = 0; i < count; i++)
        g_unk0x0053a300[i] = first + i;
    for (i = first; i < end; i++) {
        g_savedCarSceneRoots[i] = g_carBuffer[i].pSceneRoot;
        g_savedCarBodyNodes[i] = g_carBuffer[i].pBodyNode;
        g_unk0x0053a398[i] = (BYTE)g_carBuffer[i].type;
        g_savedCarAlternateBodyNodes[i] = g_carBuffer[i].pAlternateBodyNode;
        g_savedCarNearViewNodes[i] = g_carBuffer[i].pViewNodeNear;
        g_savedCarFarViewNodes[i] = g_carBuffer[i].pViewNodeFar;
        for (k = 0; k < 4; k++) {
            g_savedCarExtraNodes[i][k] = g_carBuffer[i].pExtraNodes[k];
            g_savedCarWheelNodes[i][k] = g_carBuffer[i].pWheelNodes[k];
        }
        g_unk0x0053bd44[i] = g_carBuffer[i].isGhost;
        g_unk0x0053ab88[i] = g_carBuffer[i].field_0xb58;
        if (CGameInfo::GetConfiguredGameMode() == 4)
            g_unk0x0053a398[i] = 0;
    }
    Car_ClearRecords(first, count);
    Car_ClearWheelRotation(first, count);
    for (i = first; i < end; i++) {
        g_carBuffer[i].pSceneRoot = g_savedCarSceneRoots[i];
        g_carBuffer[i].pBodyNode = g_savedCarBodyNodes[i];
        g_carBuffer[i].pAlternateBodyNode = g_savedCarAlternateBodyNodes[i];
        g_carBuffer[i].pViewNodeNear = g_savedCarNearViewNodes[i];
        g_carBuffer[i].pViewNodeFar = g_savedCarFarViewNodes[i];
        for (k = 0; k < 4; k++) {
            g_carBuffer[i].pExtraNodes[k] = g_savedCarExtraNodes[i][k];
            g_carBuffer[i].pWheelNodes[k] = g_savedCarWheelNodes[i][k];
        }
        g_carBuffer[i].isGhost = g_unk0x0053bd44[i];
        g_carBuffer[i].field_0xb58 = g_unk0x0053ab88[i];
    }
    for (i = first; i < end; i++)
        StageTiming_PlaceCarViewNodesAndBody(g_savedCarSceneRoots[i], g_savedCarBodyNodes[i], i,
                     g_unk0x0053a398[i], StageTiming_GetStartTableField244(i), (FixVector *)StageTiming_GetCarStartPosition(i), 1);
    CarPart_ClearSetRange(first, count);
    CarDamage_SetupOrderedCarParts(1, param_3, g_unk0x0053a300, count);
    CarTyres_CommitPlayerWear(1, param_3, (BYTE)RallyDataState());
    Events_Flush();
    Particle_KillAll();
    Car_InvalidateTransformsRange(first, count);
    for (i = first; i < end; i++)
        Car_SyncBodySceneNodes(Car_Get(i));
    if ((BYTE)RallyData_GetSelectionFlag26() == 0 && (BYTE)RallyData_GetSelectionFlag27() == 0) {
        for (i = first; i < end; i++)
            RallyData_ResetRaceRecordAndRouteProbe(i);
    }
    Car_StoreRenderTransforms(g_unk0x0053a300, count);
}

// Rebuilds the view lists of the race and runs one whole update step over all
// of them: prepares the listed cars (wheel travel, body state, matrices), runs
// the rendering lists of the ordered, view and extra cars, then restores the
// car order and frees the temporary lists.
// FUNCTION: CMR2 0x0042baf0
void Car_UpdateAndRenderAll(void)
{
    NetRace_DrainPendingMessages();
    Car_BuildViewOrder();
    Replay_AdvanceLiveRecordingStreams();
    Replay_PlaybackAllStreamFrames();
    Car_StepAll(g_carBuffer, g_unk0x0053b4f0, g_unk0x0053a310);
    Car_RunStepPasses(g_carBuffer, (short *)&g_carViewScale[8][0], g_carOrder[26]);
    NetRace_ReceiveAndIntegrateListedCars((Car *)g_carBuffer, g_unk0x0053bd6c, g_carOrder[25]);
    Collision_UpdateOrderedCars(g_carBuffer, g_unk0x0053a270, g_carOrder[44]);
    StageObject_UpdateMovingTransforms();
    Car_PrepareBodies(g_carBuffer, g_unk0x0053b4f0, g_unk0x0053a310);
    if (g_carOrder[26] > 0)
        Car_UpdateSuspensionPass(g_carBuffer, (short *)&g_carViewScale[8][0], g_carOrder[26]);
    NetRace_ExtrapolateOrderedCarPoses((Car *)g_carBuffer, g_unk0x0053bd6c, g_carOrder[25]);
    Replay_RecordAllStreamFrames();
    Car_PrepareStep(g_carBuffer, g_unk0x0053bd6c, g_carOrder[25]);
    Car_IntegrateWheelRotation((int *)g_unk0x0053a314, g_unk0x0053c9a0);
    Car_IntegrateWheelRotation((int *)g_unk0x0053bd6c, g_carOrder[25]);
    Car_StoreRenderTransforms(g_unk0x0053a314, g_unk0x0053c9a0);
    Car_StoreRenderTransforms(g_unk0x0053bd6c, g_carOrder[25]);
    Replay_RecordPeriodicCarSamples();
    CarPart_UpdateOrderedCars(g_carBuffer, g_unk0x0053a314, g_unk0x0053c9a0);
    StageObject_UpdateEnabledCornerContactFrames(g_carBuffer, g_unk0x0053b4f0, g_unk0x0053a310);
    ForceFeedback_UpdatePlayerSlot(0);
    NetRace_PackListedCars(g_carBuffer, g_carOrder, g_carOrderCount);
}

// Consumes one scheduled physics step for each car with steps remaining.
// FUNCTION: CMR2 0x0042bc80
void Car_DecrementContactTimers(short *pList, short count)
{
    int i;
    Car *pCar;

    for (i = count - 1; i >= 0; i--) {
        pCar = &g_carBuffer[pList[i]];
        if (pCar->simulationStepsRemaining > 0)
            pCar->simulationStepsRemaining--;
    }
}

// Rebuilds the per-car render transforms at the start of an update: takes the
// body matrices from the scene nodes, clears the wheel records and the ground
// data, mirrors the row into the shadow copy and hands the car position to the
// transform record of the renderer.
// FUNCTION: CMR2 0x0042bcd0
void Car_ResetRenderTransforms(void)
{
    short i;
    int j;
    int car;
    int *pWheel;
    int *pHeight;
    FixVector position;

    for (i = g_carOrderCount - 1; i >= 0; i--) {
        car = g_carOrder[i];
        FixMatrix_CopyRotation(&g_carBuffer[car].pBodyNode->current, &g_carPhysicsTransforms[car].body2);
        FixMatrix_CopyRotation(&g_carBuffer[car].pSceneRoot->current, &g_carPhysicsTransforms[car].body);
        pHeight = g_carPhysicsTransforms[car].cornerHeight;
        // MSVC6 keeps the reset cursor at the second word of each wheel.
        // Keep that cursor bias to reproduce the original six stores.
        pWheel = &g_carPhysicsTransforms[car].wheels[0].position.y;
        for (j = 4; j != 0; j--) {
            pWheel[-1] = 0;
            pWheel[0] = 0;
            pWheel[1] = 0;
            pWheel[2] = 0;
            pWheel[3] = 0;
            pWheel[4] = 0;
            *pHeight = 0;
            pHeight++;
            pWheel += sizeof(CarWheelRecord) / sizeof(int);
        }
        g_carPhysicsTransforms[car].groundNormal.x = 0;
        g_carPhysicsTransforms[car].groundNormal.y = 0x10000;
        g_carPhysicsTransforms[car].groundNormal.z = 0;
        memcpy(&g_carTransformsShadow[car], &g_carPhysicsTransforms[car], sizeof(CarTransforms));
        memcpy(&g_carTransforms[car], &g_carPhysicsTransforms[car], sizeof(CarTransforms));
        FixMatrix_GetPosition(&position, &g_carPhysicsTransforms[car].body);
        Collision_BuildOrientedBoxWorldCorners(g_carRenderBoxCorners[car], &position, &g_carPhysicsTransforms[car].body,
                     &g_carBuffer[car].halfExtents);
    }
}

int Car_UsesNarrowWheels(Car *pCar, int param2);
void CarTransforms_Copy(CarTransforms *pDst, CarTransforms *pSrc);
void Car_UpdateWheelMeshStates(void);

// 16.16 degrees to a sine table index (4096 per turn); defined in GameInfo.cpp.
extern double g_fixedDegreesToAngle12;

// Interpolates the render transforms of every car in the draw order between
// the physics row (0x53a3a8) and its shadow by the car's time step, rebuilds
// the four wheel matrices from the wheel records, normalises the ground normal
// and marks the nodes of the extra parts for a refresh.
// match 30%: implemented from the disassembly; the instruction sequence of the
// wheel loop, the angle conversion (g_fixedDegreesToAngle12 + fistp), the FixVecLength/
// FixVecScaleRecip pair and the calls all line up, but the frame layout and the
// address temps the original kept in locals differ, and that changes the whole
// register allocation. Kept as FUNCTION so reccmp keeps measuring it.
// FUNCTION: CMR2 0x0042bf70
void Car_InterpolateRenderTransforms(void)
{
    int i;
    short *pOrder;
    int car;
    CarTransforms *pRow;
    CarTransforms *pShadow;
    CarTransforms *pTrans;
    FixMatrix *pWheelM;
    int scale;
    int wheel;
    int k;
    int n;
    FixBasis basis;
    FixVector v;
    FixVector position;
    short angles[3];
    int result[3];
    int ca;
    int sa;
    FixVector *p;
    int *pYZ;
    SceneNode *pNode;

    n = g_carOrderCount - 1;
    if (n >= 0) {
        pOrder = &g_carOrder[n];
        i = n + 1;
        do {
            car = *pOrder;
            scale = g_carBuffer[car].simulationInterpolation;
            pRow = &g_carPhysicsTransforms[car];
            pShadow = &g_carTransformsShadow[car];
            pTrans = &g_carTransforms[car];
            if (g_carTransformsInvalid[g_carBuffer[car].index] != 0) {
                CarTransforms_Copy(pRow, pShadow);
                g_carTransformsInvalid[g_carBuffer[car].index] = 0;
            }
            FixMatrix_Interpolate(&pTrans->body2, &pRow->body2, &pShadow->body2,
                                  scale, scale, scale, 1);
            FixMatrix_CopyRotation(&pTrans->body2, &g_carBuffer[car].pBodyNode->current);
            FixMatrix_Interpolate(&pTrans->body, &pRow->body, &pShadow->body,
                                  scale, scale, scale, 1);
            FixMatrix_CopyRotation(&pTrans->body, &g_carBuffer[car].pSceneRoot->current);

            for (wheel = 0; wheel < 4; wheel++) {
                // Each wheel record is lerped from the physics row towards
                // the shadow: hub position, then the three wheel angles.
                v.x = pShadow->wheels[wheel].position.x - pRow->wheels[wheel].position.x;
                v.y = pShadow->wheels[wheel].position.y - pRow->wheels[wheel].position.y;
                v.z = pShadow->wheels[wheel].position.z - pRow->wheels[wheel].position.z;
                FixVecScale(&v, &v, scale);
                pTrans->wheels[wheel].position.x = pRow->wheels[wheel].position.x + v.x;
                pTrans->wheels[wheel].position.y = pRow->wheels[wheel].position.y + v.y;
                pTrans->wheels[wheel].position.z = pRow->wheels[wheel].position.z + v.z;
                v.x = pShadow->wheels[wheel].tiltDegrees - pRow->wheels[wheel].tiltDegrees;
                v.y = pShadow->wheels[wheel].steeringDegrees - pRow->wheels[wheel].steeringDegrees;
                v.z = pShadow->wheels[wheel].spinDegrees - pRow->wheels[wheel].spinDegrees;
                FixVecScale(&v, &v, scale);
                result[0] = pRow->wheels[wheel].tiltDegrees + v.x;
                result[1] = pRow->wheels[wheel].steeringDegrees + v.y;
                result[2] = pRow->wheels[wheel].spinDegrees + v.z;
                pTrans->wheels[wheel].tiltDegrees = result[0];
                pTrans->wheels[wheel].steeringDegrees = result[1];
                pTrans->wheels[wheel].spinDegrees = result[2];

                // The right-hand wheels point backwards: 180 degrees plus the
                // wheel angles.
                if (wheel % 2 == 1) {
                    angles[0] = (short)FTOI(__int64, X87S_MUL((double)(-result[0]), g_fixedDegreesToAngle12));
                    angles[1] = (short)FTOI(__int64, X87S_MUL((double)(result[1] + 0xb40000), g_fixedDegreesToAngle12));
                    angles[2] = (short)FTOI(__int64, X87S_MUL((double)(-result[2]), g_fixedDegreesToAngle12));
                } else {
                    angles[0] = (short)FTOI(__int64, X87S_MUL((double)result[0], g_fixedDegreesToAngle12));
                    angles[1] = (short)FTOI(__int64, X87S_MUL((double)result[1], g_fixedDegreesToAngle12));
                    angles[2] = (short)FTOI(__int64, X87S_MUL((double)result[2], g_fixedDegreesToAngle12));
                }

                basis.right.x = 0x10000;
                basis.up.y = 0x10000;
                basis.forward.z = 0x10000;
                basis.right.y = 0;
                basis.right.z = 0;
                basis.up.x = 0;
                basis.up.z = 0;
                basis.forward.x = 0;
                basis.forward.y = 0;
                FixBasis_Rotate(&basis, (unsigned short *)angles);

                // Camber: every row of the wheel basis is turned about x
                // (its y and z), the right wheels one way and the left ones
                // the other.
                if (wheel % 2 == 1) {
                    ca = g_sinTable[0x41c];
                    sa = g_sinTable[0x1c];
                } else {
                    ca = g_sinTable[0x3e4];
                    sa = g_sinTable[0xfe4];
                }
                pYZ = &basis.right.y;
                for (k = 3; k != 0; k--) {
                    pYZ[0] = FixMul(pYZ[0], ca) - FixMul(pYZ[1], sa);
                    pYZ[1] = FixMul(pYZ[0], sa) + FixMul(pYZ[1], ca);
                    pYZ += 3;
                }

                if (Car_UsesNarrowWheels(&g_carBuffer[car], 0) != 0)
                    FixVecScale(&basis.forward, &basis.forward, 0x9999);
                if (CGameInfo::IsActiveCheatEnabled(6) != 0) {
                    p = &basis.right;
                    for (k = 3; k != 0; k--) {
                        FixVecScale(p, p, 0x28000);
                        p++;
                    }
                }

                pWheelM = &g_carWheelTransforms[car][wheel];
                FixMatrix_SetRight(&basis.right, pWheelM);
                FixMatrix_SetUp(&basis.up, pWheelM);
                FixMatrix_SetForward(&basis.forward, pWheelM);
                FixMatrix_SetPosition(&pTrans->wheels[wheel].position, pWheelM);
                FixMatrix_CopyRotation(pWheelM, &g_carBuffer[car].pWheelNodes[wheel]->current);
                if (g_carBuffer[car].pExtraNodes[wheel] != NULL) {
                    FixMatrix_CopyRotation(pWheelM, &g_carBuffer[car].pExtraNodes[wheel]->current);
                    if (Car_UsesNarrowWheels(&g_carBuffer[car], 0) == 0 && Car_UsesNarrowWheels(&g_carBuffer[car], 1) != 0) {
                        FixMatrix_GetForward(&basis.forward, &g_carBuffer[car].pExtraNodes[wheel]->current);
                        FixVecScale(&basis.forward, &basis.forward, 0x9999);
                        FixMatrix_SetForward(&basis.forward, &g_carBuffer[car].pExtraNodes[wheel]->current);
                    }
                    // Mark the node and its parents up to the root dirty.
                    for (pNode = g_carBuffer[car].pExtraNodes[wheel]; pNode != NULL; pNode = pNode->pParent)
                        pNode->dirty = 1;
                }
            }

            v.x = pShadow->groundNormal.x - pRow->groundNormal.x;
            v.y = pShadow->groundNormal.y - pRow->groundNormal.y;
            v.z = pShadow->groundNormal.z - pRow->groundNormal.z;
            FixVecScale(&v, &v, scale);
            pTrans->groundNormal.x = pRow->groundNormal.x + v.x;
            pTrans->groundNormal.y = pRow->groundNormal.y + v.y;
            pTrans->groundNormal.z = pRow->groundNormal.z + v.z;
            FixVecNormalizeLen(&pTrans->groundNormal, &pTrans->groundNormal);
            for (k = 0; k < 4; k++)
                pTrans->cornerHeight[k] =
                    FixMul(pShadow->cornerHeight[k] - pRow->cornerHeight[k], scale) + pRow->cornerHeight[k];
            FixMatrix_GetPosition(&position, (FixMatrix *)pTrans);
            Collision_BuildOrientedBoxWorldCorners(g_carRenderBoxCorners[car], &position,
                         (FixMatrix *)pTrans, &g_carBuffer[car].halfExtents);
            pOrder--;
        } while (--i != 0);
    }
    Car_UpdateWheelMeshStates();
}

// Copies a car's render transforms (body matrices, wheel records, ground data).
// FUNCTION: CMR2 0x0042c7b0
void CarTransforms_Copy(CarTransforms *pDst, CarTransforms *pSrc)
{
    int i;

    FixMatrix_CopyRotation(&pSrc->body2, &pDst->body2);
    FixMatrix_CopyRotation(&pSrc->body, &pDst->body);
    for (i = 0; i < 4; i++) {
        memcpy(&pDst->wheels[i], &pSrc->wheels[i], sizeof(CarWheelRecord));
        pDst->cornerHeight[i] = pSrc->cornerHeight[i];
    }
    pDst->groundNormal = pSrc->groundNormal;
}

// FUNCTION: CMR2 0x0042c840
void Car_InvalidateTransformsRange(int first, int count)
{
    int i;

    for (i = first; i < first + count; i++)
        g_carTransformsInvalid[i] = 1;
}

// FUNCTION: CMR2 0x0042c870
void Car_InvalidateTransforms(int index)
{
    g_carTransformsInvalid[index] = 1;
}

// Simulation clock: lazily seeded epoch, current frame and previous frame
// times in milliseconds. The rate determines each car's scheduled steps.
// GLOBAL: CMR2 0x0053b530
BYTE g_carSimulationClockFlags;
// GLOBAL: CMR2 0x0053bd64
int g_carSimulationEpochMs;
// GLOBAL: CMR2 0x0053bd40
int g_carSimulationFrameMs;
#include "ReplayControls.h"
float ReplayControls_TimeScale(void);   // 1 unless a replay is playing; pause/slow/fast from the replay keys (RallyData.cpp)
static ReplayClock g_replayClock;
// GLOBAL: CMR2 0x0053a374
int g_carSimulationPreviousMs;
// GLOBAL: CMR2 0x005113a0
float g_carMillisecondsToSeconds = 0.001f;

BYTE InRaceMenu_IsOpening(void);
void InRaceMenu_ClearOpeningFlag(void);
int Race_ConsumeTransitionLatch(void);

// Historical numeric behaviour: fromStart is stored as float, but toEnd stays
// in x87 precision until nearest-even FISTP. Half-step boundaries can be
// counted twice depending on render cadence. Preserve this for matching; see
// docs/audits/2026-10-10/numeric-semantics.md.
// Schedules physics steps from elapsed time and each car's rate, storing the
// remaining step count and fractional interpolation. Returns the largest
// step count so the race loop can run each scheduled car the correct times.
// match 51%: same logic, but MSVC6 keeps the original in an EBP frame with
// memory-resident loop counters; the register allocation does not reproduce.
// match 50%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0042c890
int Car_UpdateEngineNoteFalloff(int *pOut)
{
    int i;
    int max;
    int offset;
    int mid;
    float fromStart;
    float toEnd;
    float frac;
    Car *pCar;

    if ((g_carSimulationClockFlags & 1) == 0) {
        g_carSimulationClockFlags |= 1;
        g_carSimulationEpochMs = CMain::GetFrameTime();
    }
    if ((g_carSimulationClockFlags & 2) == 0) {
        g_carSimulationClockFlags |= 2;
        g_carSimulationFrameMs = CMain::GetFrameTime();
    }
    if ((g_carSimulationClockFlags & 4) == 0) {
        g_carSimulationClockFlags |= 4;
        g_carSimulationPreviousMs = CMain::GetFrameTime();
    }
    g_carSimulationFrameMs = g_replayClock.Frame(CMain::GetFrameTime(), ReplayControls_TimeScale());   // OpenCMR2: replay pause/slow/fast scale the real time that feeds the step count; scale 1 = unchanged
    if (InRaceMenu_IsOpening() != 0)
        g_carSimulationPreviousMs = g_carSimulationFrameMs;
    if (Race_ConsumeTransitionLatch() != 0)
        g_carSimulationPreviousMs = g_carSimulationFrameMs;
    InRaceMenu_ClearOpeningFlag();
    max = 0;
    for (i = 0; i < g_carOrderCount; i++) {
        pCar = &g_carBuffer[g_carOrder[i]];
        if (g_carOrder[i] > 0 &&
            pCar->simulationRateHz == g_carBuffer->simulationRateHz) {
            pCar->simulationInterpolation = g_carBuffer->simulationInterpolation;
            offset = g_carBuffer->simulationStepsRemaining;
        } else {
            fromStart = X87S_MUL((X87S_MUL(X87S_FILD(float, (unsigned int)(g_carSimulationPreviousMs - g_carSimulationEpochMs)), g_carMillisecondsToSeconds)), pCar->simulationRateHz);
            toEnd = X87S_MUL((X87S_MUL(X87S_FILD(float, (unsigned int)(g_carSimulationFrameMs - g_carSimulationEpochMs)), g_carMillisecondsToSeconds)), pCar->simulationRateHz);
            mid = (int)FTOI(__int64, toEnd);
            frac = X87S_SUB(toEnd, X87S_FILD(float, mid));
            pCar->simulationInterpolation = FTOI(int, frac * g_65536f);
            offset = mid - (int)FTOI(__int64, fromStart);
            if (offset > 5)
                offset = 5;
        }
        pCar->simulationStepsRemaining = (char)offset;
        if ((offset & 0xff) > max)
            max = offset & 0xff;
    }
    *pOut = g_carBuffer->simulationInterpolation;
    g_carSimulationPreviousMs = g_carSimulationFrameMs;
    return max;
}

// FUNCTION: CMR2 0x0042ca70
CarTransforms *Car_GetTransforms(int index)
{
    return &g_carTransforms[index];
}

// FUNCTION: CMR2 0x0042ca90
CarTransforms *Car_GetPhysicsRow(int index)
{
    return &g_carPhysicsTransforms[index];
}

// FUNCTION: CMR2 0x0042cab0
FixMatrix *Car_GetWheelTransforms(int index)
{
    return g_carWheelTransforms[index];
}

// FUNCTION: CMR2 0x0042cac0
FixVector *Car_GetRendererRecord(int index)
{
    return g_carRenderBoxCorners[index];
}

CarSceneRecord *StageTiming_GetStartTableRecord(int index);

// Whether the car uses the narrow wheel setup (car class 6 on the normal
// surfaces); param2 also accepts the 'A' variant.
// FUNCTION: CMR2 0x0042cae0
int Car_UsesNarrowWheels(Car *pCar, int param2)
{
    char stage;

    stage = StageTiming_GetStartTableRecord(pCar->index)->modelClass;
    if (!CGameInfo::IsActiveCheatEnabled(6) && pCar->surfaceDragLevel == 6 && stage != 0xb && stage != 8 && stage != 0xa &&
        stage != 0xd && (StageTiming_GetStartTableRecord(pCar->index)->detailCode != 'A' || param2 != 0))
        return 1;
    return 0;
}

// Sets the suspension geometry of the current car from the ride-height
// setting (0..0x10000).
// FUNCTION: CMR2 0x0043dff0
void Car_SetSuspension(int heightSetting)
{
    int wheel;
    int inverseHeightSetting;
    int normalDragRange;
    int loadTravelRange;

    if (CGameInfo::IsActiveCheatEnabled(6) != 0)
        heightSetting = -0x6666;
    g_pCurrentCar->wheelLeanDamping = FixMul(heightSetting, 0x6666) + 0x9999;
    g_pCurrentCar->bodyLeanDamping = FixMul(heightSetting, 0x3333) + 0x6666;
    normalDragRange = FixMul(heightSetting, -0x1aaaa) + 0x50000;
    loadTravelRange = FixMul(heightSetting, -0x1999) + 0x3333;
    g_pCurrentCar->suspensionLoadScale = g_pCurrentCar->mass / 8;
    g_pCurrentCar->wheelTravelLoadScale = FixDiv(0x10000, loadTravelRange);
    g_pCurrentCar->field_0x9c4 = FixDiv(0x10000, normalDragRange);
    inverseHeightSetting = 0x10000 - heightSetting;
    for (wheel = 0; wheel < 4; wheel++) {
        g_pCurrentCar->wheelSpringStiffness[wheel] = 0x4ccc;
        g_pCurrentCar->wheelSpringDamping[wheel] = FixMul(heightSetting, 0x5999) + 0x4000;
        g_pCurrentCar->wheelRideHeight[wheel] = FixMul(0x1999, inverseHeightSetting);
        g_pCurrentCar->field_0x978[wheel] = FixMul(heightSetting, 0x8000) + 0x8000;
    }
    g_pCurrentCar->field_0xa08 = -0x1999;
}

// FUNCTION: CMR2 0x0043e160
void Car_SetBrakePower(int value)
{
    g_pCurrentCar->maxBrakeForce = FixMul(0x13333, value) + 0x3333;
}

// FUNCTION: CMR2 0x0043e190
void Car_SetBrakeBias(int value)
{
    g_pCurrentCar->brakeBias = value;
}

// FUNCTION: CMR2 0x0043e1b0
void Car_SetDriveSplit(int value)
{
    g_pCurrentCar->driveSplit = value;
}

// FUNCTION: CMR2 0x0043e1d0
void Car_SetTyreType(int value)
{
    g_pCurrentCar->surfaceDragLevel = (BYTE)value;
}

int Car_GetScaledSteerFollowRate(void);

// Engine torque of the current car (less the speed drag) split between the
// front and rear wheels by the drive split.
// match 63%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00442fc0
void Car_SplitEngineTorque(void)
{
    int torque;
    int front;
    int split;

    torque = Car_GetScaledSteerFollowRate();
    g_pCurrentCar->engineNetTorque = torque - FixMul(g_pCurrentCar->engineDragCoefficient,
        FixMul(g_pCurrentCar->engineSpeed, g_pCurrentCar->engineSpeed));
    split = FixMul(g_pCurrentCar->engineNetTorque, g_pCurrentCar->gearRatio[g_pCurrentCar->gear]);
    front = FixMul(split, g_pCurrentCar->driveSplit) / 2;
    g_pCurrentCar->wheelTorque[0] = front;
    g_pCurrentCar->wheelTorque[1] = front;
    front = split / 2 - front;
    g_pCurrentCar->wheelTorque[2] = front;
    g_pCurrentCar->wheelTorque[3] = front;
}

// Integrates the ride-height accumulator of the current car toward the
// average wheel load (or the engine speed while it is off the ground).
// match 88%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004430b0
void Car_UpdateRideHeight(void)
{
    int hi;
    int lo;

    if (g_pCurrentCar->gear == 0) {
        g_pCurrentCar->engineSpeed += FixMul(g_pCurrentCar->engineNetTorque, 0xa0000);
        if (g_pCurrentCar->engineSpeed > g_pCurrentCar->engineSpeedLimit) {
            g_pCurrentCar->engineSpeed = g_pCurrentCar->engineSpeedLimit;
            return;
        }
    } else {
        lo = FixMul(g_pCurrentCar->wheelLoad[0], g_pCurrentCar->gearRatio[g_pCurrentCar->gear]);
        lo += FixMul(g_pCurrentCar->wheelLoad[1], g_pCurrentCar->gearRatio[g_pCurrentCar->gear]);
        hi = FixMul(g_pCurrentCar->wheelLoad[2], g_pCurrentCar->gearRatio[g_pCurrentCar->gear]);
        hi += FixMul(g_pCurrentCar->wheelLoad[3], g_pCurrentCar->gearRatio[g_pCurrentCar->gear]);
        hi /= 2;
        lo /= 2;
        lo = FixMul(lo - hi, g_pCurrentCar->driveSplit) - g_pCurrentCar->engineSpeed;
        lo += hi;
        g_pCurrentCar->engineSpeed += FixMul(lo, 0x10000);
    }
}

// FUNCTION: CMR2 0x00443230
void Car_CopySteeringFollowState(void)
{
    g_pCurrentCar->field_0x7ac = g_pCurrentCar->engineSpeed;
}

// FUNCTION: CMR2 0x00417760
int Car_IsStateC10Set(int index)
{
    return Car_Get(index)->field_0xc10 != 0;
}

// Starts (param2 != 0) or stops the camera shake of a view: 0 idle, 1 active,
// 2 locked, 3 stopping.
// FUNCTION: CMR2 0x00423010
void View_SetShake(int view, int start)
{
    if (start != 0) {
        if (g_unk0x00538f00[view] != 2) {
            if (g_unk0x00538f00[view] == 0) {
                g_unk0x00538df8[view] = 0;
                g_unk0x00538c98[view] = 0;
            }
            g_unk0x00538f00[view] = 1;
        }
        return;
    }
    if (g_unk0x00538f00[view] != 0)
        g_unk0x00538f00[view] = 3;
}


// Whether the car shows its clean wheels: not yet damaged and not on one
// of the snow/night stages.
// FUNCTION: CMR2 0x0042cb50
BOOL Car_ShowsCleanWheels(Car *pCar)
{
    char stage;

    stage = StageTiming_GetStartTableRecord(pCar->index)->modelClass;
    if (pCar->surfaceDragLevel <= 1 && stage != 11 && stage != 8 && stage != 10 && stage != 13)
        return TRUE;
    return FALSE;
}

void Graphics_ReloadTexture(Texture *pTexture);

// GLOBAL: CMR2 0x00519c94
char g_strWheelVariantL[4] = "L";
// GLOBAL: CMR2 0x00519c98
char g_strWheelVariantN[4] = "N";

// Swaps the textures of the four wheel meshes between their "L" and "N"
// variants (the letter 9 characters from the end of the texture name) to
// match Car_ShowsCleanWheels, reloading every texture that changed.
// FUNCTION: CMR2 0x0042cb90
void Car_SwapWheelTextures(char mode, SceneNode **pWheels)
{
    int w;
    int t;
    int k;
    Mesh *pMesh;
    Texture *pTex;

    if (mode == 'A')
        return;
    for (w = 0; w < 4; w++) {
        if (pWheels[w] == NULL)
            continue;
        pMesh = (Mesh *)pWheels[w]->pObject;
        for (t = 0; t < pMesh->triangleCount; t++) {
            for (k = 0; k < 10; k++) {
                pTex = CGraphics::m_pTextureManager->textureBuffer[((int *)&pMesh->pTriangles[t])[k + 1]];
                if (pTex == NULL)
                    continue;
                if (Car_ShowsCleanWheels(g_pCurrentCar)) {
                    if (strncmp(pTex->name + strlen(pTex->name) - 9, g_strWheelVariantN, 1) == 0) {
                        strncpy(pTex->name + strlen(pTex->name) - 9, g_strWheelVariantL, 1);
                        Graphics_ReloadTexture(pTex);
                    }
                } else if (strncmp(pTex->name + strlen(pTex->name) - 9, g_strWheelVariantL, 1) == 0) {
                    strncpy(pTex->name + strlen(pTex->name) - 9, g_strWheelVariantN, 1);
                    Graphics_ReloadTexture(pTex);
                }
            }
        }
    }
}

int Track_GetGroundHeightSurface(FixVector *pPoint, FixVector *pNormal, short *pTri, short *pSurfaceClass,
                                 unsigned short *pSurface, int defaultY);

// GLOBAL: CMR2 0x0053cadc
int g_gravityScale;

// Velocity of the box corners of the current car: the two opposite corners
// get the rotational component (half-width times the angular velocity through
// the world matrix) plus the body velocity; the other six copy them.
// match 76%: same logic; MSVC put pWorld in ESI and pVel in EDI instead of
// the EDI/ESI the original used (induction/pointer register numbering).
// match 75%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00443a20
void Car_UpdateCornerVelocityPair(void)
{
    FixMatrix *pWorld;
    FixVector *pVel;
    FixVector *pCorner;
    int a;
    int b;

    pVel = &g_pCurrentCar->velocity;
    pWorld = g_pCurrentCar->pWorld;
    pCorner = g_pCurrentCar->cornerVelocity;
    a = FixMul(g_pCurrentCar->halfExtents.x, g_pCurrentCar->angularVelocity.z);
    b = FixMul(g_pCurrentCar->angularVelocity.y, g_pCurrentCar->halfExtents.x);
    pCorner[0].x = FixMul(pWorld->up.x, a);
    pCorner[0].x -= FixMul(pWorld->forward.x, b);
    pCorner[0].y = FixMul(pWorld->up.y, a) - FixMul(pWorld->forward.y, b);
    pCorner[0].z = FixMul(pWorld->up.z, a) - FixMul(pWorld->forward.z, b);
    pCorner[2].x = -pCorner[0].x;
    pCorner[2].y = -pCorner[0].y;
    pCorner[2].z = -pCorner[0].z;
    pCorner[0].x += pVel->x;
    pCorner[0].y += pVel->y;
    pCorner[0].z += pVel->z;
    pCorner[2].x += pVel->x;
    pCorner[2].y += pVel->y;
    pCorner[2].z += pVel->z;
    pCorner[1] = pCorner[0];
    pCorner[3] = pCorner[2];
    pCorner[4] = pCorner[0];
    pCorner[5] = pCorner[0];
    pCorner[6] = pCorner[2];
    pCorner[7] = pCorner[2];
}

void Car_FollowGround(void);
void Car_RebuildBodyAxes(void);

// Runs the suspension/geometry pass over the listed cars: selects each car,
// caches its setup, copies the wheel surfaces, rebuilds the body and keeps the
// smallest corner clearance of the eight body corners.
// FUNCTION: CMR2 0x00443bf0
void Car_UpdateSuspensionPass(Car *param_1, short *param_2, short param_3)
{
    short *p;
    int n;
    int i;
    int j;
    int v;

    n = param_3 - 1;
    if (n < 0)
        return;
    p = param_2 + n;
    n++;
    do {
        g_pCurrentCar = param_1 + *p;
        g_physicsPartSet = CarPart_GetSet((int)g_pCurrentCar->index);
        Car_UpdateWheelTravel();
        g_pCurrentCar->previousWheelSurface[0] = g_pCurrentCar->wheelSurface[0];
        g_pCurrentCar->previousWheelSurface[1] = g_pCurrentCar->wheelSurface[1];
        g_pCurrentCar->previousWheelSurface[2] = g_pCurrentCar->wheelSurface[2];
        g_pCurrentCar->previousWheelSurface[3] = g_pCurrentCar->wheelSurface[3];
        Car_FollowGround();
        Car_RebuildBodyAxes();
        g_pCurrentCar->field_0x960 = 0x3e80000;
        for (i = 0; i < 8; i++) {
            v = g_pCurrentCar->corners[i].y - g_pCurrentCar->cornerHeight[i];
            if (v < g_pCurrentCar->field_0x960)
                g_pCurrentCar->field_0x960 = v;
        }
        g_pCurrentCar->field_0x960 += g_pCurrentCar->groundHeightCorrection;
        p--;
    } while (--n);
}

// out = v / |v|, or zero when v is zero: FIX_NORMALIZE_INTO with the output
// passed by pointer (computed once, the zero case stores through it).
static inline void FixVecNormalizeInto(FixVector *out, FixVector *v)
{
    int len = FixVecLength(v);
    if (len == 0) {
        out->x = 0;
        out->y = 0;
        out->z = 0;
    } else {
        FixVecScaleRecip(out, v, len);
    }
}

// FUNCTION: CMR2 0x00443d10
void Car_FollowGround(void)
{
    FixVector n;
    FixVector oldNormal;
    FixVector dn;
    FixVector rel;
    FixVector corner;
    FixVector p;
    FixVector d;
    FixVector t;
    FixVector step;
    unsigned short surface;
    int ease;
    int along;
    int slow;
    int h;
    int len;
    int k;
    int i;
    int damping;

    g_pCurrentCar->cornerHeight[0] =
        Track_GetGroundHeightSurface(&g_pCurrentCar->position, &n, &g_pCurrentCar->cornerTriangle[0],
                                     &g_pCurrentCar->wheelSurface[0], &surface, g_pCurrentCar->cornerHeight[0]);
    if (g_pCurrentCar->cornerOnGround[4] != 0) {
        g_pCurrentCar->cornerHeight[0] += g_pCurrentCar->field_0x8fc[0];
        n = g_pCurrentCar->field_0x564[0];
        g_pCurrentCar->wheelSurface[0] = 0x2f;
    }
    if (g_pCurrentCar->wheelSurface[0] == 0xf && g_pCurrentCar->field_0xa7c == 0 && g_pCurrentCar->field_0xbf8 == 0)
        g_pCurrentCar->field_0xa7c = 0x190000;

    // Ease the ground normal towards the new one.
    oldNormal = g_pCurrentCar->groundNormal;
    dn.x = n.x - g_pCurrentCar->groundNormal.x;
    dn.y = n.y - g_pCurrentCar->groundNormal.y;
    dn.z = n.z - g_pCurrentCar->groundNormal.z;
    ease = FixMul(0x8000, g_physicsTimeStep);
    if (ease > 0x8000)
        ease = 0x8000;
    FixVecScale(&step, &dn, ease);
    dn.x -= step.x;
    dn.y -= step.y;
    dn.z -= step.z;
    n.x = g_pCurrentCar->groundNormal.x + dn.x;
    n.y = g_pCurrentCar->groundNormal.y + dn.y;
    n.z = g_pCurrentCar->groundNormal.z + dn.z;
    FixVecNormalizeInto(&g_pCurrentCar->groundNormal, &n);
    along = FixVecDot(&g_pCurrentCar->groundNormal, &g_pCurrentCar->velocity);
    slow = FixMul(g_pCurrentCar->speed, 0x8000);
    if (slow > 0x10000)
        slow = 0x10000;
    slow = FixMul(0x10000 - slow, 0x1999);
    if (g_pCurrentCar->field_0xb74 != 0) {
        if (along <= slow) {
            g_pCurrentCar->up = g_pCurrentCar->groundNormal;
            FixVecScale(&t, &g_pCurrentCar->up, FixVecDot(&g_pCurrentCar->up, &g_pCurrentCar->right));
            t.x = g_pCurrentCar->right.x - t.x;
            t.y = g_pCurrentCar->right.y - t.y;
            t.z = g_pCurrentCar->right.z - t.z;
            FixVecNormalizeInto(&g_pCurrentCar->right, &t);
            FixVecCross(&t, &g_pCurrentCar->right, &g_pCurrentCar->up);
            FixVecNormalizeInto(&g_pCurrentCar->forward, &t);
        } else {
            g_pCurrentCar->groundNormal = oldNormal;
        }
    }
    g_pCurrentCar->pWorld->right = g_pCurrentCar->right;
    g_pCurrentCar->pWorld->up = g_pCurrentCar->up;
    g_pCurrentCar->pWorld->forward = g_pCurrentCar->forward;
    g_pCurrentCar->pWorld->position = g_pCurrentCar->position;

    // Drop the lowest corner of the box onto the ground.
    t.x = -g_pCurrentCar->groundNormal.x;
    t.y = -g_pCurrentCar->groundNormal.y;
    t.z = -g_pCurrentCar->groundNormal.z;
    FixMatrix_InverseRotateVector(&rel, &t, g_pCurrentCar->pWorld);
    t.x = g_pCurrentCar->halfExtents.x;
    t.y = -g_pCurrentCar->halfExtents.y;
    t.z = g_pCurrentCar->halfExtents.z;
    if (rel.y >= 0)
        t.y = g_pCurrentCar->halfExtents.y;
    if (rel.x < 0)
        t.x = -g_pCurrentCar->halfExtents.x;
    if (rel.z < 0)
        t.z = -g_pCurrentCar->halfExtents.z;
    FixMatrix_RotateVector(&p, &t, g_pCurrentCar->pWorld);
    p.x += g_pCurrentCar->position.x;
    p.y += g_pCurrentCar->position.y;
    p.z += g_pCurrentCar->position.z;
    d.x = g_pCurrentCar->position.x - p.x;
    d.y = g_pCurrentCar->cornerHeight[0] - p.y;
    d.z = g_pCurrentCar->position.z - p.z;
    h = FixDiv(FixVecDot(&g_pCurrentCar->groundNormal, &d), g_pCurrentCar->groundNormal.y);
    for (i = 0; i < 8; i++) {
        g_pCurrentCar->cornerAxis[i].x = 0;
        g_pCurrentCar->cornerAxis[i].y = 0x10000;
        g_pCurrentCar->cornerAxis[i].z = 0;
        g_pCurrentCar->cornerNormal[i] = g_pCurrentCar->cornerAxis[i];
    }
    g_pCurrentCar->normal0x498 = g_pCurrentCar->groundNormal;
    for (i = 1; i < 4; i++) {
        g_pCurrentCar->cornerHeight[i] = g_pCurrentCar->cornerHeight[0];
        g_pCurrentCar->wheelSurface[i] = g_pCurrentCar->wheelSurface[0];
    }
    ((char *)&g_pCurrentCar->field_0xb34)[1] = 0;
    g_pCurrentCar->field_0xb34 = 4;
    ((char *)&g_pCurrentCar->field_0xb34)[2] = 0;
    ((char *)&g_pCurrentCar->field_0xb34)[3] = 1;
    ((char *)&g_pCurrentCar->field_0xb34)[4] = 2;
    ((char *)&g_pCurrentCar->field_0xb34)[5] = 3;
    g_pCurrentCar->cornerFlags[0] = 0;
    g_pCurrentCar->cornerFlags[1] = 0;
    g_pCurrentCar->cornerFlags[2] = 0;
    g_pCurrentCar->cornerFlags[3] = 0;
    g_pCurrentCar->cornerFlags[4] = 1;
    g_pCurrentCar->cornerFlags[5] = 1;
    g_pCurrentCar->cornerFlags[6] = 1;
    g_pCurrentCar->cornerFlags[7] = 1;
    if (h > 0) {
        g_pCurrentCar->position.y += h;
        g_pCurrentCar->pWorld->position = g_pCurrentCar->position;
    }
    if (h >= -0x1999) {
        if (h < 0)
            g_pCurrentCar->groundHeightCorrection = h;
        else
            g_pCurrentCar->groundHeightCorrection = 0;
        // Computed and never used (the asm helper is not optimised away).
        damping = FixMul(0x1eb8, g_physicsTimeStep);
        if (along < 0) {
            FixVecScale(&t, &g_pCurrentCar->groundNormal, along);
            g_pCurrentCar->velocity.x -= t.x;
            g_pCurrentCar->velocity.y -= t.y;
            g_pCurrentCar->velocity.z -= t.z;
        }
        g_pCurrentCar->baseForce.x = 0;
        g_pCurrentCar->baseForce.y = 0;
        g_pCurrentCar->baseForce.z = 0;
        g_pCurrentCar->angularVelocity.z = 0;
        g_pCurrentCar->angularVelocity.x = 0;
    } else {
        g_pCurrentCar->groundHeightCorrection = -0x1999;
        g_pCurrentCar->baseForce.x = 0;
        g_pCurrentCar->baseForce.y = -FixMul(g_pCurrentCar->mass, FixMul(0x8000, g_gravityScale));
        g_pCurrentCar->baseForce.z = 0;
    }
    if (h < -0xcccc) {
        if (g_pCurrentCar->field_0xb74 != 0) {
            // Tip over with the change of the ground normal.
            FixMatrix_InverseRotateVector(&corner, &dn, g_pCurrentCar->pWorld);
            corner.y = 0;
            len = FixVecLength(&corner);
            if (len > 10000)
                FixVecScale(&corner, &corner, FixDiv(10000, len));
            k = -FixMul(g_pCurrentCar->halfExtents.y, FixMul(0x4ccc, g_physicsTimeStep));
            g_pCurrentCar->angularVelocity.x -= FixMul(corner.z, k);
            g_pCurrentCar->angularVelocity.z += FixMul(corner.x, k);
        }
        g_pCurrentCar->field_0xb74 = 0;
        return;
    }
    g_pCurrentCar->field_0xb74 = 1;
}

// Switches the car to the tumbling (eight corner) model and stores the
// ground normal in its body axes.
#define CAR_START_TUMBLING()                                                                         \
    g_pCurrentCar->useUpperCollisionCorners = 1;                                                                  \
    g_pCurrentCar->field_0x96c = 0x10000;                                                            \
    g_pCurrentCar->groundRightDot = FixVecDot(&g_pCurrentCar->groundNormal, &g_pCurrentCar->right);     \
    g_pCurrentCar->groundUpDot = FixVecDot(&g_pCurrentCar->groundNormal, &g_pCurrentCar->up);        \
    g_pCurrentCar->groundForwardDot = FixVecDot(&g_pCurrentCar->groundNormal, &g_pCurrentCar->forward)

// Roll-over: on a slope too steep to stand on (over about 30 degrees) the
// car gets a growing torque that tips it over; otherwise, sliding sideways
// faster than the wheels can hold tips the body, and past the limit the car
// starts to tumble.
// FUNCTION: CMR2 0x004373c0
void Car_UpdateRollover(void)
{
    struct {
        FixVector a;
        FixVector b;
        FixVector c;
        FixVector t;
    } vectors;
    int k;
    int sum;
    int limit;
    int slide;
    int target;
    int diff;
    int mag;
    int i;

    k = 0;
    if (g_pCurrentCar->field_0xb28 == 0 && g_pCurrentCar->field_0xb34 == 0)
        return;
    if (g_pCurrentCar->groundNormal.y < 0x10000 && (short)(0x400 - FixAcos(g_pCurrentCar->groundNormal.y)) > 0x155 &&
        g_pCurrentCar->field_0xb42 <= 0) {
        FixVecScale(&vectors.t, &g_pCurrentCar->groundNormal, 0x280000);
        FixMatrix_InverseRotateVector(&vectors.a, &vectors.t, g_pCurrentCar->pWorld);
        FixVecScale(&vectors.t, &g_pCurrentCar->baseForce,
                    FixMul(g_pCurrentCar->steepTime << 16, FixMul(FixDiv(0x10000, 0xc80000), 0x8000)));
        FixMatrix_InverseRotateVector(&vectors.b, &vectors.t, g_pCurrentCar->pWorld);
        FixVecCross(&vectors.c, &vectors.b, &vectors.a);
        FixVecScale(&vectors.c, &vectors.c, g_pCurrentCar->inverseMass);
        if (g_pCurrentCar->useUpperCollisionCorners == 0) {
            g_pCurrentCar->useUpperCollisionCorners = 1;
            g_pCurrentCar->field_0x96c = 0x10000;
            g_pCurrentCar->groundRightDot = FixVecDot(&g_pCurrentCar->right, &g_pCurrentCar->groundNormal);
            g_pCurrentCar->groundUpDot = FixVecDot(&g_pCurrentCar->up, &g_pCurrentCar->groundNormal);
            g_pCurrentCar->groundForwardDot = FixVecDot(&g_pCurrentCar->forward, &g_pCurrentCar->groundNormal);
        }
        g_pCurrentCar->field_0x5d0.x += vectors.c.x;
        g_pCurrentCar->field_0x5d0.y += vectors.c.y;
        g_pCurrentCar->field_0x5d0.z += vectors.c.z;
        if (g_pCurrentCar->steepTime < 200)
            g_pCurrentCar->steepTime++;
    } else {
        g_pCurrentCar->steepTime = 0;
    }
    if (g_pCurrentCar->useUpperCollisionCorners != 0)
        return;

    // Sideways slide against the grip of the wheels.
    sum = 0;
    for (i = 0; i < 4; i++)
        sum += g_pCurrentCar->wheelSurfaceFx[i].effect[0] << 16;
    sum = FixMul(sum, 0x4000);
    limit = FixMul(sum, FixDiv(0x10000, 0x431168));
    FixVecScale(&vectors.t, &g_pCurrentCar->groundNormal, FixVecDot(&g_pCurrentCar->forward, &g_pCurrentCar->groundNormal));
    vectors.t.x = g_pCurrentCar->forward.x - vectors.t.x;
    vectors.t.y = g_pCurrentCar->forward.y - vectors.t.y;
    vectors.t.z = g_pCurrentCar->forward.z - vectors.t.z;
    FIX_NORMALIZE_INTO(vectors.t, vectors.t);
    slide = FixVecDot(&vectors.t, &g_pCurrentCar->velocity);
    if (FIX_ABS(slide) > limit) {
        sum = 0;
        for (i = 0; i < 4; i++)
            sum += g_pCurrentCar->wheelSurfaceFx[i].effect[1] << 16;
        sum = FixMul(sum, 0x4000);
        sum = FixMul(sum, FixDiv(0x10000, 0x431168));
        if (limit != 0)
            k = FixDiv(0x10000, sum - limit);
        if (slide > 0)
            sum = slide - limit;
        else
            sum = slide + limit;
        sum = FixMul(sum, k);
        if (FIX_ABS(sum) > 0x10000)
            sum = sum > 0 ? 0x10000 : -0x10000;
    } else
        sum = 0;
    target = FixMul(sum, 0x10000);
    diff = target - g_pCurrentCar->tipRatio;
    k = 0x1999;
    k = FixMul(k, g_physicsTimeStep);
    if (FIX_ABS(diff) < k) {
        g_pCurrentCar->tipRatio = target;
    } else {
        if (diff > 0)
            g_pCurrentCar->tipRatio += k;
        else
            g_pCurrentCar->tipRatio -= k;
    }
    if (FIX_ABS(g_pCurrentCar->tipRatio) > FixMul(0xcccc, 0x10000))
        mag = 1;
    else
        mag = 0;
    g_pCurrentCar->tipAngle = FixAtan2(g_pCurrentCar->tipRatio, 0x20000);
    if (mag) {
        g_pCurrentCar->tipAngle = 0;
        g_pCurrentCar->useUpperCollisionCorners = 1;
        g_pCurrentCar->field_0x96c = 0x10000;
        g_pCurrentCar->groundRightDot = FixVecDot(&g_pCurrentCar->right, &g_pCurrentCar->groundNormal);
        g_pCurrentCar->groundUpDot = FixVecDot(&g_pCurrentCar->up, &g_pCurrentCar->groundNormal);
        g_pCurrentCar->groundForwardDot = FixVecDot(&g_pCurrentCar->forward, &g_pCurrentCar->groundNormal);
        g_pCurrentCar->field_0x5d0.x += FixMul(FixMul(slide, g_physicsScale), -0xa0000);
    }
}

extern int g_unk0x0053c9d4;

// Lowers the current car's target (0x7a4) toward 0x794 minus a fading offset.
int StageTiming_GetCheckpointField12(int index);

void Car_BalanceTwoPlayerRideHeight(Car *param_1, short *param_2, short param_3);
void Car_StartPendingCountdown(void);
void Car_UpdateLowSpeedWheelLoadTimer(void);
void CarEffects_UpdateBrokenLightFlicker(Car *pCar);
void Car_UpdateSlopeGrip(void);
void Car_ShareWeightOnWheels(void);
void Car_UpdateEngineTorque(void);
void Car_IntegrateContacts(void);
void Car_UpdateEngineSpeed(void);
void Car_UpdateResponseValueCountdown(void);
int StageObject_GetCarWeatherRampValue(Car *pCar);
void Car_UpdateSurfaceParams(Car *pCar, int blend);
void Car_UpdateCornerLoads(void);
void Car_UpdateSteering(void);
void Car_UpdateCornerVelocities(void);
short Sector_GetNeighbours(FixVector *pPos, short *pOut);
void AutoGear_UpdateCarGearState(Car *pCar);

// One whole update step over every car of the list: rebuilds the body frame and
// the corner state of each car, runs its ground/contact, engine, steering and
// friction updates, then publishes the final position and the surface of every
// car.
// FUNCTION: CMR2 0x00433890
void Car_StepAll(Car *pCars, short *pOrder, short count)
{
    FixVector bodyGroundNormal;
    short neighbourSectors[3];
    int halfGravity;
    int mass;
    int orderIndex;
    int cornerIndex;

    Car_BalanceTwoPlayerRideHeight(pCars, pOrder, count);
    for (orderIndex = count - 1; orderIndex >= 0; orderIndex--) {
        g_unk0x0053c9d4 = 0;
        g_carStepAccel.x = 0;
        g_carStepAccel.y = 0;
        g_carStepAccel.z = 0;
        g_pCurrentCar = pCars + pOrder[orderIndex];
        Car_ScaleRatesByTimeStep(g_pCurrentCar);
        for (cornerIndex = 0; cornerIndex < 4; cornerIndex++) {
            g_pCurrentCar->cornerPrev2[cornerIndex] = g_pCurrentCar->cornerPrev[cornerIndex];
            g_pCurrentCar->cornerPrev[cornerIndex] = g_pCurrentCar->corners[cornerIndex];
        }
        g_pCurrentCar->positionPrev2 =
            g_pCurrentCar->positionPrev;
        g_pCurrentCar->positionPrev = g_pCurrentCar->position;
        memcpy(g_pCurrentCar->targetAxes, g_pCurrentCar->bodyAxes, sizeof(g_pCurrentCar->bodyAxes));
        g_pCurrentCar->field_0x968 = g_pCurrentCar->field_0x964;
        g_pCurrentCar->field_0x964 = g_pCurrentCar->field_0x960;
        if (g_pCurrentCar->field_0xbfc == 0)
            Car_StartPendingCountdown();
        Car_UpdateSurfaceParams(g_pCurrentCar, StageObject_GetCarWeatherRampValue(g_pCurrentCar));
        Car_UpdateLowSpeedWheelLoadTimer();
        CarEffects_UpdateBrokenLightFlicker(g_pCurrentCar);
    }
    for (orderIndex = count - 1; orderIndex >= 0; orderIndex--) {
        g_pCurrentCar = pCars + pOrder[orderIndex];
        AutoGear_UpdateCarGearState(g_pCurrentCar);
        for (cornerIndex = 3; cornerIndex >= 0; cornerIndex--) {
            g_pCurrentCar->wheelSlip[cornerIndex] = 0;
            g_pCurrentCar->wheelSlipLateral[cornerIndex] = 0;
        }
    }
    for (orderIndex = count - 1; orderIndex >= 0; orderIndex--) {
        g_pCurrentCar = pCars + pOrder[orderIndex];
        g_pCurrentCar->field_0xb74 = 1;
        FixMatrix_InverseRotateVector(&bodyGroundNormal, &g_pCurrentCar->groundNormal, g_pCurrentCar->pWorld);
        if (bodyGroundNormal.y <= 0x106 || FIX_ABS(bodyGroundNormal.x) > FixMul(abs(bodyGroundNormal.y), 0x93cd) ||
            FIX_ABS(bodyGroundNormal.z) > FixMul(FIX_ABS(bodyGroundNormal.y), 0x10000))
            g_pCurrentCar->field_0xb74 = 0;
    }
    for (orderIndex = count - 1; orderIndex >= 0; orderIndex--) {
        g_pCurrentCar = pCars + pOrder[orderIndex];
        g_physicsPartSet = CarPart_GetSet((int)g_pCurrentCar->index);
        if (g_physicsPartSet->glassDebrisEmitted == 0 && g_physicsPartSet->glassCooldown != 0)
            g_physicsPartSet->glassCooldown -= 1;
        g_physicsPartSet->glassDebrisEmitted = 0;
        Car_UpdateCornerVelocities();
        memset(&g_pCurrentCar->cornerLoad, 0, sizeof(g_pCurrentCar->cornerLoad));
        memset(&g_pCurrentCar->cornerForce, 0, sizeof(g_pCurrentCar->cornerForce));
        g_pCurrentCar->baseForce.x = 0;
        halfGravity = FixMul(0x8000, g_gravityScale);
        mass = g_pCurrentCar->mass;
        g_pCurrentCar->baseForce.y = -FixMul(mass, halfGravity);
        g_pCurrentCar->baseForce.z = 0;
        Car_ShareWeightOnWheels();
        Car_UpdateSuspension();
        Car_UpdateSlopeGrip();
        Car_UpdateCornerLoads();
        Car_UpdateSteering();
    }
    for (orderIndex = count - 1; orderIndex >= 0; orderIndex--) {
        g_pCurrentCar = pCars + pOrder[orderIndex];
        g_physicsPartSet = CarPart_GetSet((int)g_pCurrentCar->index);
        Car_UpdateEngineTorque();
    }
    for (orderIndex = count - 1; orderIndex >= 0; orderIndex--) {
        g_pCurrentCar = pCars + pOrder[orderIndex];
        Car_UpdateCornerFriction();
    }
    for (orderIndex = count - 1; orderIndex >= 0; orderIndex--) {
        g_pCurrentCar = pCars + pOrder[orderIndex];
        g_physicsPartSet = CarPart_GetSet((int)g_pCurrentCar->index);
        Car_IntegrateContacts();
    }
    for (orderIndex = count - 1; orderIndex >= 0; orderIndex--) {
        g_pCurrentCar = pCars + pOrder[orderIndex];
        g_pCurrentCar->field_0x7a8 = g_pCurrentCar->engineSpeed;
        Car_UpdateEngineSpeed();
        Car_UpdateResponseValueCountdown();
        g_pCurrentCar->field_0x5c4.x = 0;
        g_pCurrentCar->field_0x5c4.y = 0;
        g_pCurrentCar->field_0x5c4.z = 0;
        g_pCurrentCar->field_0x5d0.x = 0;
        g_pCurrentCar->field_0x5d0.y = 0;
        g_pCurrentCar->field_0x5d0.z = 0;
    }
    for (orderIndex = count - 1; orderIndex >= 0; orderIndex--) {
        g_pCurrentCar = pCars + pOrder[orderIndex];
        g_pCurrentCar->sector =
            Sector_GetNeighbours(&g_pCurrentCar->position, neighbourSectors);
        g_pCurrentCar->neighbours.s[0] = neighbourSectors[0];
        g_pCurrentCar->neighbours.s[1] = neighbourSectors[1];
        g_pCurrentCar->neighbours.s[2] = neighbourSectors[2];
        g_pCurrentCar->field_0x2dc = g_pCurrentCar->position;
    }
}

// Transfers ride height between the two cars so the one further back along
// the road is raised; only active in the 2-player game (state 2).
// FUNCTION: CMR2 0x00433e80
void Car_BalanceTwoPlayerRideHeight(Car *param_1, short *param_2, short param_3)
{
    Car *pCar1;
    Car *pCar2;
    int diff;
    int dist;
    int t;
    int amount;

    if ((BYTE)RallyDataState() != 2)
        return;
    if (param_3 != 2)
        return;
    if ((BYTE)RallyData_GetFlag24() == 0)
        return;
    if ((BYTE)CGameInfo::GetNetworkOptionBit11() == 0)
        return;
    pCar1 = param_1 + param_2[0];
    pCar2 = param_1 + param_2[1];
    diff = StageTiming_GetCheckpointField12(pCar1->index);
    diff -= StageTiming_GetCheckpointField12(pCar2->index);
    dist = diff * 0x10000;
    if (dist < 0)
        dist = -dist;
    if (dist <= 0x30000) {
        t = 0;
    } else if (dist < 0xf0000) {
        t = dist - 0x30000;
        t = FixMul(t, FixDiv(0x10000, 0xc0000));
        if (t < 0)
            t = 0;
        else if (t > 0x10000)
            t = 0x10000;
    } else {
        t = 0x10000;
    }
    amount = FixMul(t, 0xccc);
    if (diff < 0) {
        pCar2->maxThrottleTorque = pCar2->baseThrottleTorque - amount;
        pCar1->maxThrottleTorque = pCar1->baseThrottleTorque + amount;
    } else {
        pCar1->maxThrottleTorque = pCar1->baseThrottleTorque - amount;
        pCar2->maxThrottleTorque = pCar2->baseThrottleTorque + amount;
    }
}

// FUNCTION: CMR2 0x00433fd0
void Car_UpdateResponseValueCountdown(void)
{
    int target = g_pCurrentCar->engineSpeedLimit - FixMul(g_pCurrentCar->field_0xb1d << 16, 0x3333);

    if (g_pCurrentCar->engineSpeed - target >= -0x28f) {
        g_pCurrentCar->engineSpeed = target;
        if (g_pCurrentCar->field_0xb1d == 0)
            g_pCurrentCar->field_0xb1d = 3;
    }
    if (g_pCurrentCar->field_0xb1d != 0) {
        g_pCurrentCar->field_0xb1d--;
        g_pCurrentCar->field_0x7ac = g_pCurrentCar->engineSpeed;
        return;
    }
    g_pCurrentCar->field_0x7ac = g_pCurrentCar->engineSpeed;
}

// Ground grip factor of the current car from the slope it stands on.
// FUNCTION: CMR2 0x00434070
void Car_UpdateSlopeGrip(void)
{
    int grip;
    int y = g_pCurrentCar->groundNormal.y;

    if (y < 0xf0a3) {
        grip = FixMul(FixDiv(0x10000, 0x3d70), y - 0xb333);
        if (grip < 0)
            grip = 0;
        g_unk0x0053c9d4 = FixMul(0xe666, grip);
        return;
    }
    g_unk0x0053c9d4 = 0xe666;
}

// Counts field 0xb1f of the current car down while it is slow compared to
// the load of the (front or rear) wheels; clears it once it is fast enough.
// FUNCTION: CMR2 0x004340f0
void Car_UpdateLowSpeedWheelLoadTimer(void)
{
    int load;

    if (g_pCurrentCar->field_0xb1f != 0) {
        if (g_pCurrentCar->driveSplit == 0)
            load = g_pCurrentCar->speed * 4 - g_pCurrentCar->wheelLoad[2];
        else
            load = g_pCurrentCar->speed * 4 - g_pCurrentCar->wheelLoad[0];
        if (load > -0x1999) {
            g_pCurrentCar->field_0xb1f = 0;
            return;
        }
        g_pCurrentCar->field_0xb1f--;
    }
}

// Steering: the rolling direction of the car eases towards the body's
// right axis (faster with field_0x79c), and the steered wheels turn from it
// by the steering angle, which wobbles with the position on the stage on
// the cars whose setup asks for it.
// match 78%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00434140
void Car_UpdateSteering(void)
{
    FixMatrix m;
    FixVector right;
    FixVector t;
    unsigned short angle;
    int w;

    if (FIX_ABS(g_pCurrentCar->speed) < 0x28f) {
        angle = 0;
    } else {
        angle = g_pCurrentCar->wheelSteeringAngle;
        if (g_physicsPartSet->steeringWobble > 0) {
            if (FIX_ABS(g_pCurrentCar->position.x) - FIX_ABS(g_pCurrentCar->position.z) < 0)
                w = -FIX_ABS(g_pCurrentCar->position.x) + FIX_ABS(g_pCurrentCar->position.z);
            else
                w = FIX_ABS(g_pCurrentCar->position.x) - FIX_ABS(g_pCurrentCar->position.z);
            w = FixMul(0x20000, (w % 0x401 - 0x200) * 0x40);
            if (g_pCurrentCar->speed < 0x10000)
                w = FixMul(w, g_pCurrentCar->speed);
            w = FixMul(w, g_physicsPartSet->steeringWobble);
            angle += (short)FTOI(__int64, X87S_MUL((double)w, g_fixedDegreesToAngle12));
        }
    }
    right = g_pCurrentCar->right;
    FixVecScale(&t, &g_pCurrentCar->up, FixVecDot(&g_pCurrentCar->rearWheelDir, &g_pCurrentCar->up));
    t.x = g_pCurrentCar->rearWheelDir.x - t.x;
    t.y = g_pCurrentCar->rearWheelDir.y - t.y;
    t.z = g_pCurrentCar->rearWheelDir.z - t.z;
    FixVecNormalizeLen(&g_pCurrentCar->rearWheelDir, &t);
    t.x = right.x - g_pCurrentCar->rearWheelDir.x;
    t.y = right.y - g_pCurrentCar->rearWheelDir.y;
    t.z = right.z - g_pCurrentCar->rearWheelDir.z;
    w = FixVecLength(&t);
    w = FixMul(w, 0x13333);
    w = FixMul(w, FixMul(FixMul(0xcccd, g_pCurrentCar->throttleTorque) + 0x3333, g_physicsTimeStep));
    if (w >= 0x10000) {
        g_pCurrentCar->rearWheelDir = right;
    } else {
        FixVecScale(&t, &t, w);
        g_pCurrentCar->rearWheelDir.x += t.x;
        g_pCurrentCar->rearWheelDir.y += t.y;
        g_pCurrentCar->rearWheelDir.z += t.z;
        FixVecNormalizeLen(&g_pCurrentCar->rearWheelDir, &g_pCurrentCar->rearWheelDir);
    }
    if (angle == 0) {
        g_pCurrentCar->frontWheelDir = g_pCurrentCar->rearWheelDir;
    } else {
        ((void (__stdcall *)(FixMatrix *, FixVector *, short))FixMatrix_FromAxisAngle)(&m, &g_pCurrentCar->up, angle);
        FixMatrix_RotateVector(&t, &g_pCurrentCar->rearWheelDir, &m);
        FixVecNormalizeLen(&g_pCurrentCar->frontWheelDir, &t);
    }
    FixVecCross(&g_pCurrentCar->frontWheelAxis, &g_pCurrentCar->frontWheelDir, &g_pCurrentCar->up);
}

// GLOBAL: CMR2 0x0053c9d4
int g_unk0x0053c9d4;
// Direction of gravity.
// GLOBAL: CMR2 0x0053cad0
FixVector g_gravityDir;

// Ground load: the normal force of the ground spread over the touching
// corners, and from the suspension load of each wheel its grip limits.
// match 76%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004348c0
void Car_UpdateCornerLoads(void)
{
    FixVector n0;
    FixVector f;
    int k;
    int n;
    int load;
    int grip;
    int i;
    int *pGripB;

    n = g_pCurrentCar->field_0xb34;
    if (n == 0) {
        g_pCurrentCar->cornerMass = 0;
    } else {
        if (n == 4)
            load = g_pCurrentCar->mass / 4;
        else if (n == 2)
            load = g_pCurrentCar->mass / 2;
        else {
            load = g_pCurrentCar->mass;
            if (n != 1)
                load = load / n;
        }
        g_pCurrentCar->cornerMass = load;
    }
    n0 = g_pCurrentCar->groundNormal;
    load = -FixMul(g_pCurrentCar->cornerMass + 0x1e0000, FixVecDot(&n0, &g_gravityDir));
    FixVecScale(&f, &n0, FixMul(load, 0x10000));
    grip = FixMul(g_pCurrentCar->speed, FixMul(0x1cccc, 0x10000));
    if (grip > 0x1cccc)
        grip = 0x1cccc;
    grip += 0x10000;
    for (i = 0; i < 8; i++) {
        pGripB = &g_pCurrentCar->cornerGripB[i];
        if (g_pCurrentCar->cornerFlags[i] == 0 || (i <= 3 && g_pCurrentCar->cornerOnGround[i] != 0)) {
            if (n != 4) {
                g_pCurrentCar->cornerLoad[i].x += f.x;
                g_pCurrentCar->cornerLoad[i].y += f.y;
                g_pCurrentCar->cornerLoad[i].z += f.z;
            }
            if (i < 4 && g_pCurrentCar->field_0xb74 != 0) {
                load = FixMul(FixMul(g_pCurrentCar->cornerAxis[i].y, 0x8000), g_pCurrentCar->cornerWeight[i] + 0x1e0000);
                g_pCurrentCar->field_0xa5c[i] =
                    FixMul(load, g_pCurrentCar->wheelSurfaceFx[i].drag +
                                     g_pCurrentCar->cornerGrip[i].grip2A);
                g_pCurrentCar->field_0xa4c[i] =
                    FixMul(load, g_pCurrentCar->wheelSurfaceFx[i].drag +
                                     g_pCurrentCar->cornerGrip[i].gripA);
            } else {
                g_pCurrentCar->cornerGripA[i] = FixMul(load, g_pCurrentCar->cornerGrip[i].grip2A);
                *pGripB = FixMul(load, g_pCurrentCar->cornerGrip[i].gripA);
                g_pCurrentCar->cornerGripA[i] = FixMul(g_pCurrentCar->cornerGripA[i], grip);
                *pGripB = FixMul(*pGripB, grip);
            }
        } else if (i < 4) {
            *pGripB = 0;
            g_pCurrentCar->cornerGripA[i] = 0;
            g_pCurrentCar->field_0xa4c[i] = 0;
            g_pCurrentCar->field_0xa5c[i] = 0;
        } else {
            *pGripB = 0;
            g_pCurrentCar->cornerGripA[i] = 0;
        }
    }
    if (g_pCurrentCar->field_0xb34 > 0 || g_pCurrentCar->field_0xb28 != 0) {
        // The base force without its push into the ground, shared by the
        // touching corners.
        FixVecScale(&f, &n0, FixVecDot(&g_pCurrentCar->baseForce, &n0));
        f.x = g_pCurrentCar->baseForce.x - f.x;
        f.y = g_pCurrentCar->baseForce.y - f.y;
        f.z = g_pCurrentCar->baseForce.z - f.z;
        FixVecScale(&f, &f, 0x10000 - g_unk0x0053c9d4);
        if (g_pCurrentCar->field_0xb74 == 0) {
            if (g_pCurrentCar->field_0xb34 >= 1)
                FixVecScaleRecip(&g_carStepAccel, &f,
                                 (int)FTOI(__int64, X87S_MUL((double)g_pCurrentCar->field_0xb34, CGraphics::m_65536)));
            else
                FixVecScaleRecip(&g_carStepAccel, &f,
                                 (int)FTOI(__int64, X87S_MUL((double)g_pCurrentCar->field_0xb28, CGraphics::m_65536)));
        } else if (g_pCurrentCar->field_0xb28 > 0) {
            FixVecScaleRecip(&g_carStepAccel, &f, (int)FTOI(__int64, X87S_MUL((double)g_pCurrentCar->field_0xb28, CGraphics::m_65536)));
        } else {
            FixVecScaleRecip(&g_carStepAccel, &f, (int)FTOI(__int64, X87S_MUL((double)g_pCurrentCar->field_0xb34, CGraphics::m_65536)));
        }
        k = FixMul(-0xb333, FixDiv(0x10000, 0x3d70));
        if (k < 0)
            k = 0;
        FixVecScale(&g_carStepAccel, &g_carStepAccel, k);
    }
    if (g_pCurrentCar->field_0xb34 > 0)
        g_pCurrentCar->baseForce = f;
}

// Updates the engine speed from the selected gear or the startup animation,
// then applies the rev limit and its excess-speed flag.
// FUNCTION: CMR2 0x004380c0
void Car_UpdateEngineSpeed(void)
{
    int front;
    int rear;
    int excess;
    short angle;

    if (g_pCurrentCar->gear == 0 || g_pCurrentCar->shiftInProgress != 0) {
        if (g_pCurrentCar->engineRestartPending == 0 && g_pCurrentCar->engineStartTimer == 0) {
            g_pCurrentCar->engineSpeed += FixMul(g_pCurrentCar->engineNetTorque, 0xa0000);
        } else if (g_pCurrentCar->engineRestartPending != 0) {
            if (g_pCurrentCar->engineStartTimer > 1000) {
                g_pCurrentCar->engineStartTimer -= (short)(FixMul(g_physicsTimeStep, 0x3e80000) >> 16);
                if (g_pCurrentCar->engineStartTimer < 0)
                    g_pCurrentCar->engineStartTimer = 0;
            } else {
                g_pCurrentCar->engineRestartPending = 0;
                g_pCurrentCar->engineStartTimer = 25000;
            }
        } else if (g_pCurrentCar->engineStartTimer > 0) {
            excess = 0x190000 - FixMul(0x41, g_pCurrentCar->engineStartTimer << 16);
            excess = FixMul(excess, 0xa3d);
            angle = (short)FTOI(__int64, X87S_MUL((double)FixMul(excess, 0x8c0000), g_fixedDegreesToAngle12));
            g_pCurrentCar->engineSpeed = FixMul(FixMul(g_sinTable[(unsigned short)angle & 0xfff], 0x10000), g_pCurrentCar->engineSpeedLimit);
            g_pCurrentCar->engineStartTimer -= (short)(FixMul(g_physicsTimeStep, 0x3e80000) >> 16);
            if (g_pCurrentCar->engineStartTimer < 0)
                g_pCurrentCar->engineStartTimer = 0;
            if (angle < 0x400)
                g_pCurrentCar->throttleTorque = 0x10000;
        }
    } else {
        front = FixMul(g_pCurrentCar->wheelLoad[0], g_pCurrentCar->gearRatio[g_pCurrentCar->gear]);
        front += FixMul(g_pCurrentCar->wheelLoad[1], g_pCurrentCar->gearRatio[g_pCurrentCar->gear]);
        if (g_pCurrentCar->handbrake != 0 && g_pCurrentCar->driveSplit != 0) {
            rear = front;
        } else {
            rear = FixMul(g_pCurrentCar->wheelLoad[2], g_pCurrentCar->gearRatio[g_pCurrentCar->gear]);
            rear += FixMul(g_pCurrentCar->wheelLoad[3], g_pCurrentCar->gearRatio[g_pCurrentCar->gear]);
        }
        rear /= 2;
        front /= 2;
        front = FixMul(front - rear, g_pCurrentCar->driveSplit) - g_pCurrentCar->engineSpeed;
        front += rear;
        g_pCurrentCar->engineSpeed += FixMul(front, 0x10000);
    }

    excess = g_pCurrentCar->engineSpeed;
    if (excess < 0) {
        g_pCurrentCar->engineSpeed = 0;
    } else if (excess > g_pCurrentCar->engineSpeedLimit) {
        rear = excess - g_pCurrentCar->engineSpeedLimit;
        if (rear > 0xcccc) {
            rear -= 0xcccc;
            excess = FixMul(rear, 0x10000);
            if (excess > 0x10000)
                excess = 0x10000;
            g_pCurrentCar->revLimiterTorque = FixMul(excess, 0x51eb);
            g_pCurrentCar->revLimiterActive = 1;
        } else {
            g_pCurrentCar->revLimiterActive = 0;
        }
        g_pCurrentCar->engineSpeed = g_pCurrentCar->engineSpeedLimit;
        return;
    }
    g_pCurrentCar->revLimiterActive = 0;
}

// Rebuilds the body matrix axes and lifts the body along its up axis by the
// front suspension height. When field_0xb74 is set the axes are taken from
// the world matrix instead.
// FUNCTION: CMR2 0x00444a70
void Car_RebuildBodyAxes(void)
{
    FixVector v[3];
    FixVector tmp;
    int scale;
    int i;

    if (g_pCurrentCar->field_0xb74 != 0) {
        FixMatrix_GetRight(&v[0], g_pCurrentCar->pBodyMatrix);
        FixMatrix_GetUp(&v[1], g_pCurrentCar->pBodyMatrix);
        FixMatrix_GetForward(&v[2], g_pCurrentCar->pBodyMatrix);
        for (i = 0; i < 3; i++) {
            FixMatrix_RotateVector(&tmp, &v[i], g_pCurrentCar->pWorld);
            v[i] = tmp;
        }
        FixMatrix_SetRight(&v[0], g_pCurrentCar->pBodyMatrix);
        FixMatrix_SetUp(&v[1], g_pCurrentCar->pBodyMatrix);
        FixMatrix_SetForward(&v[2], g_pCurrentCar->pBodyMatrix);
    } else {
        FixMatrix_SetRight(&g_pCurrentCar->right, g_pCurrentCar->pBodyMatrix);
        FixMatrix_SetUp(&g_pCurrentCar->up, g_pCurrentCar->pBodyMatrix);
        FixMatrix_SetForward(&g_pCurrentCar->forward, g_pCurrentCar->pBodyMatrix);
    }
    scale = g_pCurrentCar->wheelRideHeight[0];
    FixVecScale(&tmp, &g_pCurrentCar->up, scale);
    tmp.x += g_pCurrentCar->position.x;
    tmp.y += g_pCurrentCar->position.y;
    tmp.z += g_pCurrentCar->position.z;
    g_pCurrentCar->pBodyMatrix->position = tmp;
}

void Car_ShareWeightOnWheels(void);
void Car_LiftOutOfGround(void);
void Car_UpdateSurfaceParams(Car *pCar, int blend);
void CarDamage_StepClimbingCarRecords(short *param_1, short count);
short Sector_GetNeighbours(FixVector *pPos, short *pOut);

// Prepares every car of the order list for a physics step: restores the body
// frame from the world matrix (or resets the per-step state when the car is
// not settled), refreshes the velocity magnitude, the corner velocities, the
// body axes and the wheel loads, lifts the car out of the ground and refreshes
// its surface parameters and the sector neighbours of its position.
// FUNCTION: CMR2 0x00444c10
void Car_PrepareStep(Car *pCars, short *pOrder, short count)
{
    short *pCarIndex;
    int remaining;
    int i;
    int lift;
    short neighbourSectors[3];
    FixVector work;
    FixVector *pAxis;


    remaining = count - 1;
    if (remaining >= 0) {
        pCarIndex = &pOrder[remaining];
        remaining++;
        do {
            g_pCurrentCar = pCars + *pCarIndex;
            lift = g_pCurrentCar->field_0x960;
            if (lift < 0)
                lift = 0;

            if (g_pCurrentCar->field_0xc20 == 0) {
                for (i = 0; i < 4; i++) {
                    g_pCurrentCar->cornerPrev2[i] =
                        g_pCurrentCar->cornerPrev[i];
                    g_pCurrentCar->cornerPrev[i] =
                        g_pCurrentCar->corners[i];
                    g_pCurrentCar->previousWheelSurface[i] = g_pCurrentCar->wheelSurface[i];
                }
                for (i = 0; i < 8; i++) {
                    g_pCurrentCar->cornerNormal[i] =
                        g_pCurrentCar->cornerAxis[i];
                    g_pCurrentCar->objectContactState[i] = 0;
                }
                g_pCurrentCar->field_0x7a8 = g_pCurrentCar->engineSpeed;
                g_pCurrentCar->positionPrev2 = g_pCurrentCar->positionPrev;
                g_pCurrentCar->positionPrev = g_pCurrentCar->position;
                g_pCurrentCar->normal0x498 = g_pCurrentCar->groundNormal;
                memcpy(g_pCurrentCar->targetAxes, g_pCurrentCar->bodyAxes, sizeof(g_pCurrentCar->targetAxes));
                FixMatrix_GetPosition(&g_pCurrentCar->position, g_pCurrentCar->pWorld);
                FixMatrix_GetRight(&g_pCurrentCar->right, g_pCurrentCar->pWorld);
                FixMatrix_GetUp(&g_pCurrentCar->up, g_pCurrentCar->pWorld);
                FixMatrix_GetForward(&g_pCurrentCar->forward, g_pCurrentCar->pWorld);
                Car_UpdateCorners(g_pCurrentCar);
            }

            g_pCurrentCar->speed = FixVecLength(&g_pCurrentCar->velocity);
            for (i = 0; i < 8; i++)
                g_pCurrentCar->cornerVelocity[i] = g_pCurrentCar->velocity;
            g_pCurrentCar->renderSteeringAngle = *(short *)&g_pCurrentCar->wheelSteeringAngle;
            g_pCurrentCar->targetSteeringAngle = g_pCurrentCar->renderSteeringAngle;
            g_pCurrentCar->groundHeightCorrection = 0;
            g_pCurrentCar->reversing = 0;
            g_pCurrentCar->wheelLoad[3] = FixVecDot(&g_pCurrentCar->velocity, &g_pCurrentCar->right) * 4;
            g_pCurrentCar->wheelLoad[2] = g_pCurrentCar->wheelLoad[3];
            g_pCurrentCar->wheelLoad[1] = g_pCurrentCar->wheelLoad[2];
            g_pCurrentCar->wheelLoad[0] = g_pCurrentCar->wheelLoad[1];
            g_pCurrentCar->engineSpeed = 0;
            g_pCurrentCar->gear = 0;
            for (i = 0; i < 8; i++) {
                int u = FixMul(g_pCurrentCar->wheelLoad[0], g_pCurrentCar->gearRatio[i]);
                if (u > g_pCurrentCar->engineSpeed && u <= g_pCurrentCar->engineSpeedLimit) {
                    g_pCurrentCar->engineSpeed = u;
                    g_pCurrentCar->gear = (char)i;
                }
            }
            g_pCurrentCar->field_0x7ac = g_pCurrentCar->engineSpeed;
            g_pCurrentCar->field_0xb60 = 0;
            StageObject_UpdateCarCornerGroundHeights(g_pCurrentCar, 8);
            Car_FlagAirborneCorners();
            Car_ShareWeightOnWheels();
            Car_UpdateGroundNormal();

            if (g_pCurrentCar->field_0xc20 != 0) {
                if (g_pCurrentCar->useUpperCollisionCorners != 0) {
                    Car_LiftOutOfGround();
                    g_pCurrentCar->position.y += lift;
                    g_pCurrentCar->corners[0].y += lift;
                    g_pCurrentCar->corners[1].y += lift;
                    g_pCurrentCar->corners[2].y += lift;
                    g_pCurrentCar->corners[3].y += lift;
                    g_pCurrentCar->corners[4].y += lift;
                    g_pCurrentCar->corners[5].y += lift;
                    g_pCurrentCar->corners[6].y += lift;
                    g_pCurrentCar->corners[7].y += lift;
                    Car_FlagAirborneCorners();
                    Car_ShareWeightOnWheels();
                } else {
                    g_pCurrentCar->up = g_pCurrentCar->groundNormal;
                    i = FixVecDot(&g_pCurrentCar->up, &g_pCurrentCar->right);
                    FixVecScale(&work, &g_pCurrentCar->up, i);
                    work.x = g_pCurrentCar->right.x - work.x;
                    work.y = g_pCurrentCar->right.y - work.y;
                    work.z = g_pCurrentCar->right.z - work.z;
                    pAxis = &g_pCurrentCar->right;
                    FIX_NORMALIZE_INTO(pAxis[0], work)
                    FixVecCross(&work, pAxis, &g_pCurrentCar->up);
                    pAxis = &g_pCurrentCar->forward;
                    FIX_NORMALIZE_INTO(pAxis[0], work)
                    FixMatrix_SetRight(&g_pCurrentCar->right, g_pCurrentCar->pWorld);
                    FixMatrix_SetUp(&g_pCurrentCar->up, g_pCurrentCar->pWorld);
                    FixMatrix_SetForward(&g_pCurrentCar->forward, g_pCurrentCar->pWorld);
                    Car_UpdateCorners(g_pCurrentCar);
                    StageObject_UpdateCarCornerGroundHeights(g_pCurrentCar, 8);
                    Car_LiftOutOfGround();
                    g_pCurrentCar->position.y += lift;
                    g_pCurrentCar->corners[0].y += lift;
                    g_pCurrentCar->corners[1].y += lift;
                    g_pCurrentCar->corners[2].y += lift;
                    g_pCurrentCar->corners[3].y += lift;
                    g_pCurrentCar->corners[4].y += lift;
                    g_pCurrentCar->corners[5].y += lift;
                    g_pCurrentCar->corners[6].y += lift;
                    g_pCurrentCar->corners[7].y += lift;
                    Car_FlagAirborneCorners();
                    Car_ShareWeightOnWheels();
                    Car_UpdateGroundNormal();
                }
                FixMatrix_SetPosition(&g_pCurrentCar->position, g_pCurrentCar->pWorld);
            }

            if (g_pCurrentCar->field_0xbf8 != 0) {
                for (i = 0; i < 4; i++) {
                    g_pCurrentCar->cornerPrev2[i] =
                        g_pCurrentCar->corners[i];
                    g_pCurrentCar->cornerPrev[i] =
                        g_pCurrentCar->corners[i];
                    g_pCurrentCar->previousWheelSurface[i] = g_pCurrentCar->wheelSurface[i];
                }
                for (i = 0; i < 8; i++) {
                    g_pCurrentCar->cornerNormal[i] =
                        g_pCurrentCar->cornerAxis[i];
                    g_pCurrentCar->objectContactState[i] = 0;
                }
                g_pCurrentCar->field_0x7a8 = g_pCurrentCar->engineSpeed;
                g_pCurrentCar->positionPrev2 = g_pCurrentCar->position;
                g_pCurrentCar->positionPrev = g_pCurrentCar->position;
                g_pCurrentCar->normal0x498 = g_pCurrentCar->groundNormal;
                memcpy(g_pCurrentCar->targetAxes, g_pCurrentCar->bodyAxes, sizeof(g_pCurrentCar->targetAxes));
                g_pCurrentCar->wheelLean.x = 0;
                g_pCurrentCar->wheelLean.y = 0;
                g_pCurrentCar->wheelLean.z = 0;
                g_pCurrentCar->lean.x = 0;
                g_pCurrentCar->lean.y = 0;
                g_pCurrentCar->lean.z = 0;
            }
            g_pCurrentCar->field_0xbf8 = 0;
            g_pCurrentCar->field_0xc20 = 0;
            g_pCurrentCar->field_0xb74 = 1;
            FixMatrix_InverseRotateVector(&work, &g_pCurrentCar->groundNormal, g_pCurrentCar->pWorld);
            if (!(work.y > 0x106 && FIX_ABS(work.x) <= FixMul(FIX_ABS(work.y), 0x93cd) &&
                  FIX_ABS(work.z) <= FixMul(FIX_ABS(work.y), 0x10000)))
                g_pCurrentCar->field_0xb74 = 0;
            for (i = 0; i < 4; i++) {
                g_pCurrentCar->wheelSpinForWheelLean[i] = 0;
                g_pCurrentCar->wheelSpinForBodyLean[i] = 0;
            }
            Car_UpdateSurfaceParams(g_pCurrentCar, 0);
            Car_IntegrateWheelTravel();
            g_pCurrentCar->field_0xb1d = 0;
            Car_UpdateSuspension();
            Car_StoreBodyMatrix();
            g_pCurrentCar->sector =
                Sector_GetNeighbours(&g_pCurrentCar->position, neighbourSectors);
            g_pCurrentCar->neighbours.s[0] = neighbourSectors[0];
            g_pCurrentCar->neighbours.s[1] = neighbourSectors[1];
            g_pCurrentCar->neighbours.s[2] = neighbourSectors[2];
            pCarIndex--;
        } while (--remaining != 0);
    }
    CarDamage_StepClimbingCarRecords(pOrder, count);
}

// Lifts the current car out of the ground by the deepest penetration of any
// of its eight box corners, rising or sinking as needed.
// FUNCTION: CMR2 0x004458d0
void Car_LiftOutOfGround(void)
{
    int maxDrop = 0;
    int foundDrop = 0;
    int maxRise = -0x3e80000;
    int foundRise = 0;
    int i;
    int d;
    int lift = 0;

    for (i = 7; i >= 0; i--) {
        d = g_pCurrentCar->cornerHeight[i] - g_pCurrentCar->corners[i].y;
        if (d >= 0 && d > maxDrop) {
            maxDrop = d;
            foundDrop = 1;
        }
        if (d <= 0 && d > maxRise) {
            maxRise = d;
            foundRise = 1;
        }
    }
    if (foundDrop)
        lift = maxDrop;
    else if (foundRise)
        lift = maxRise;
    g_pCurrentCar->position.y += lift;
    g_pCurrentCar->corners[0].y += lift;
    g_pCurrentCar->corners[1].y += lift;
    g_pCurrentCar->corners[2].y += lift;
    g_pCurrentCar->corners[3].y += lift;
    g_pCurrentCar->corners[4].y += lift;
    g_pCurrentCar->corners[5].y += lift;
    g_pCurrentCar->corners[6].y += lift;
    g_pCurrentCar->corners[7].y += lift;
}

// GLOBAL: CMR2 0x0053cdb4
int g_unk0x0053cdb4;

// FUNCTION: CMR2 0x00445a20
int View_IsPlaybackCameraActive(void)
{
    if (View_GetActiveCameraMode(0) != 9 && g_unk0x0053cdb4 == 0)
        return 0;
    return 1;
}

CarTransforms *Car_GetPhysicsRow(int index);
int StageObject_IsNegativeRightAngleCameraSpot(CameraRecord *pRecord);
int StageObject_GetCarCameraSpotIndex(CameraRecord *pRecord);

// Rotation of car `index`'s camera placement.
// FUNCTION: CMR2 0x00423a00
FixMatrix *View_GetCarCameraRotation(FixMatrix *pOut, BYTE index)
{
    FixMatrix_CopyRotationFrom(pOut, &Car_GetPhysicsRow(index)->body);
    return pOut;
}

// Position of a car's camera target node (at +0x750).
// FUNCTION: CMR2 0x00423db0
FixVector *View_GetCarCameraTarget(FixVector *pOut, BYTE index)
{
    FixMatrix_GetPosition(pOut, Car_Get(index)->pWorld);
    return pOut;
}

// Current camera mode record (100 bytes) of a view.
#define VIEW_MODE_RECORD(i) (&g_viewRecords[(unsigned int)g_activeViewRecord[i] + (i) * 2])

struct Unk00423ee0Block {
    int value[16];
};

void FixMatrix_Interpolate(FixMatrix *pOut, FixMatrix *pA, FixMatrix *pB, int tRight, int tAxis, int tPos, int mode);

// Interpolates between two state records (matrix and the values at +0x48).
// FUNCTION: CMR2 0x00423de0
void CameraState_Interpolate(CameraRecord *pOut, CameraRecord *pA, CameraRecord *pB, int t)
{
    FixMatrix_Interpolate(&pOut->matrix, &pA->matrix, &pB->matrix, t, t, t, 1);
    pOut->carLift = pA->carLift + FixMul(pB->carLift - pA->carLift, t);
    pOut->nearPlane = pA->nearPlane + FixMul(pB->nearPlane - pA->nearPlane, t);
    pOut->zoom = pA->zoom + FixMul(pB->zoom - pA->zoom, t);
    pOut->liftPitch = pA->liftPitch + FixMul(pB->liftPitch - pA->liftPitch, t);
    pOut->lookDistance = pA->lookDistance + FixMul(pB->lookDistance - pA->lookDistance, t);
    pOut->field_0x50 = pA->field_0x50 + FixMul(pB->field_0x50 - pA->field_0x50, t);
    pOut->clearance = pA->clearance + FixMul(pB->clearance - pA->clearance, t);
}

// Copies the state record at src into dst (fields 0x4..0x64 except 0x0).
// FUNCTION: CMR2 0x00423ee0
void CameraState_Copy(CameraRecord *dst, CameraRecord *src)
{
    dst->type = src->type;
    *(Unk00423ee0Block *)&dst->matrix = *(Unk00423ee0Block *)&src->matrix;
    dst->carLift = src->carLift;
    dst->nearPlane = src->nearPlane;
    dst->zoom = src->zoom;
    dst->liftPitch = src->liftPitch;
    dst->lookDistance = src->lookDistance;
    dst->field_0x50 = src->field_0x50;
    dst->clearance = src->clearance;
}

// Scale of the render distance for the detail level: base * (1 + step).
// FUNCTION: CMR2 0x00423f30
int Render_GetDetailDistanceScale(void)
{
    float steps[10];
    float base;

    steps[0] = -0.5f;
    steps[1] = 0.0f;
    steps[2] = 0.5f;
    steps[3] = 1.0f;
    steps[4] = 1.5f;
    steps[5] = 2.0f;
    steps[6] = 2.5f;
    steps[7] = 3.0f;
    steps[8] = 3.5f;
    steps[9] = 4.0f;
    *(volatile float *)&base =
        (float)(X87S_MUL(g_pGraphics->baseDrawDistance, CGraphics::m_oneOver65536));
    float scale = base * steps[CGameInfo::GetGraphicsOptionBits21To24()];

    return (int)FTOI(__int64, X87S_MUL((scale + base), CGraphics::m_65536));
}

// FUNCTION: CMR2 0x00423fc0
int View_IsNegativeRightAngleCameraSpot(int view)
{
    return StageObject_IsNegativeRightAngleCameraSpot(VIEW_MODE_RECORD(view));
}

int RallyData_IsDriverViewModeAllowed(unsigned char index, int mode);
void View_SwitchCamera(unsigned char index, int a, int b, unsigned char c, int d);
unsigned int StageObject_FindNearestCameraSpot(FixVector *pPos);
int View_GetCameraSpotIndex(unsigned int view);

// Next free view-mode slot (0..0xa) of a player's view record.
// FUNCTION: CMR2 0x004218d0
extern BYTE g_droneVariant[8], g_dronePairDone[8];
extern signed char g_droneOverride[8];
// OpenCMR2: --attract-capture state signature: the active camera record type and whether it is the drone variant.
void AttractCapture_ViewSignature(int view, int *type, int *drone)
{
    BYTE rec = (BYTE)(g_activeViewRecord[view & 1] + (view & 1) * 2);
    *type = g_viewRecords[rec & 3].type;
    *drone = g_droneVariant[rec & 7];
}
int View_FindFreeModeSlot(unsigned char view)
{
    int mode;
    int found;

    mode = g_viewRecords[(BYTE)(g_activeViewRecord[view & 0xff] + (char)view * 2)].type;
    // OpenCMR2: the drone camera is a second stop on the original chase view (type 5),
    // so every original mode stays in the cycle. Shown only when video.camera_operator is on.
    if (mode == 5 && Sys_GetOption("video.camera_operator", 0) != 0) {
        BYTE activeRecord = (BYTE)(g_activeViewRecord[view & 0xff] + (char)view * 2);
        BYTE nextRecord = (BYTE)((char)view * 2 - g_activeViewRecord[view & 0xff] + 1);
        if (activeRecord < 8 && nextRecord < 8 && g_dronePairDone[activeRecord] == 0) {
            g_droneOverride[nextRecord] = g_droneVariant[activeRecord] ? 0 : 1;
            View_SwitchCamera(view, 5, 0xffff, View_GetActiveCameraFlags(view), 1);
            return 5;
        }
    }
    do {
        mode++;
        if (mode == 0xb)
            mode = 0;
        found = RallyData_IsDriverViewModeAllowed(view, mode);
    } while (found == 0);
    View_SwitchCamera(view, mode, 0xffff, View_GetActiveCameraFlags(view), 1);
    return mode;
}

// OpenCMR2 replay camera cycle (Tab), order in ReplayCameraCycle.h. The drone is the second stop of chase 5 when video.camera_operator is on. A mode
// that View_SwitchCamera would refuse (trackside 7 without camera spots, an unloaded cockpit) is skipped, so a press always lands on a camera. Returns
// the new mode, or the unchanged one when nothing else is available.
int View_IsModeAvailable(BYTE index, int mode);
static FreeCam g_freeCam;
static FreeCamInput g_freeCamInput;
static double g_freeCamLastMs = 0;
int View_FreeCamActive(void) { return g_freeCam.active ? 1 : 0; }
void View_FreeCamStop(void) { g_freeCam.active = false; }
// Starts (or restarts, for R) the free camera at the pose of the active camera record: the chase camera behind the car on entry.
static void View_FreeCamStart(unsigned char view)
{
    BYTE active = (BYTE)(g_activeViewRecord[view & 0xff] + (char)view * 2);
    const FixMatrix *m = &g_viewRecords[active].matrix;
    double pos[3] = {m->position.x / 65536.0, m->position.y / 65536.0, m->position.z / 65536.0};
    double f[3] = {m->forward.x / 65536.0, m->forward.y / 65536.0, m->forward.z / 65536.0};
    double r[3] = {m->right.x / 65536.0, m->right.y / 65536.0, m->right.z / 65536.0};
    g_freeCam.Start(pos, f, r);
    g_freeCamLastMs = Sys_GetTicksPrecise();
    Sys_Log("replay free camera: start pos %.1f %.1f %.1f yaw %.2f pitch %.2f hand %d", pos[0], pos[1], pos[2], g_freeCam.yaw, g_freeCam.pitch, g_freeCam.hand);
}
// Per frame, after the sim steps and the game's own camera update, so it also works with zero steps (paused). Replaces only the shown pose.
void View_FreeCamFrame(unsigned char view, const FreeCamInput *in, int reset)
{
    if (!g_freeCam.active) return;
    if (reset) View_FreeCamStart(view);
    double now = Sys_GetTicksPrecise();
    g_freeCam.Update(*in, (now - g_freeCamLastMs) / 1000.0);
    g_freeCamLastMs = now;
    FreeCamAxes a = g_freeCam.Axes();
    FixMatrix *m = &g_viewStates[view].matrix;
    #define FREECAM_FIX(v) ((int)std::lround((v) * 65536.0))
    m->right.x = FREECAM_FIX(a.r[0]); m->right.y = FREECAM_FIX(a.r[1]); m->right.z = FREECAM_FIX(a.r[2]);
    m->up.x = FREECAM_FIX(a.u[0]); m->up.y = FREECAM_FIX(a.u[1]); m->up.z = FREECAM_FIX(a.u[2]);
    m->forward.x = FREECAM_FIX(a.f[0]); m->forward.y = FREECAM_FIX(a.f[1]); m->forward.z = FREECAM_FIX(a.f[2]);
    m->position.x = FREECAM_FIX(g_freeCam.pos[0]); m->position.y = FREECAM_FIX(g_freeCam.pos[1]); m->position.z = FREECAM_FIX(g_freeCam.pos[2]);
    #undef FREECAM_FIX
    g_viewPreviousStates[view].matrix = *m;   // no blend back toward the old pose
}
int View_CycleReplayCamera(unsigned char view)
{
    int mode = View_GetActiveCameraMode(view);
    BYTE flags = View_GetActiveCameraFlags(view);
    if (g_freeCam.active) { g_freeCam.active = false; mode = kReplayFreeCamMode; }
    if (mode == 5 && Sys_GetOption("video.camera_operator", 0) != 0) {
        BYTE activeRecord = (BYTE)(g_activeViewRecord[view & 0xff] + (char)view * 2);
        if (activeRecord < 8 && g_dronePairDone[activeRecord] == 0) {
            int r = View_FindFreeModeSlot(view);   // the drone is the second stop of chase 5; it stays on mode 5
            Sys_Log("replay camera: %d -> drone/chase variant (mode %d)", mode, r);
            return r;
        }
    }
    unsigned available = 1u << kReplayFreeCamMode;
    for (int m = 1; m <= 7; m++) {
        int ok = m == 7 ? View_IsModeAvailable(view, 7) : RallyData_IsDriverViewModeAllowed(view, m);
        if (ok) available |= 1u << m;
    }
    int next = ReplayCamera_Next(mode, available);
    Sys_Log("replay camera: mode %d, available mask 0x%x, next %d", mode, available, next);
    if (next < 0) return mode;
    if (next == kReplayFreeCamMode) {   // pose override on top of the chase camera, so the car is drawn from outside even when coming from the cockpit
        if (View_GetActiveCameraMode(view) != 4) View_SwitchCamera(view, 4, 0xffff, flags, 1);
        View_FreeCamStart(view);
        return next;
    }
    View_SwitchCamera(view, next, 0xffff, flags, next == 7 ? 0 : 1);
    return next;
}

// Moves the view's mode record to the surface the camera target sits on.
// FUNCTION: CMR2 0x00421930
void View_UpdateModeSurface(unsigned int view)
{
    FixVector target;
    unsigned int surface;

    surface = StageObject_FindNearestCameraSpot(View_GetCarCameraTarget(&target, View_GetActiveCameraFlags(view)));
    if (surface != View_GetCameraSpotIndex(view))
        View_SwitchCamera(view, 7, surface, View_GetActiveCameraFlags(view), 0);
}

// FUNCTION: CMR2 0x00421980
int View_GetCameraSpotIndex(unsigned int view)
{
    view &= 0xff;
    return StageObject_GetCarCameraSpotIndex(VIEW_MODE_RECORD(view));
}

// FUNCTION: CMR2 0x00437f90
int Car_GetScaledSteerFollowRate(void)
{
    return FixMul(g_pCurrentCar->throttleTorque, g_physicsPartSet->engineTorqueScale);
}

int CarInterior_IsLoaded(BYTE index);
int StageObject_GetSelectedRecordListState(void);

// Whether view mode `mode` is available for car `index`.
// FUNCTION: CMR2 0x004232a0
int View_IsModeAvailable(BYTE index, int mode)
{
    int result;

    result = 0;
    switch (mode) {
    case 1:
    case 2:
    case 4:
    case 5:
    case 6:
    case 10:
        return 1;
    case 7:
        return (unsigned int)StageObject_GetSelectedRecordListState() > 0;
    case 3:
        result = CarInterior_IsLoaded((BYTE)index);
        break;
    }
    return result;
}

// Clears `count` car records from `first`.
// FUNCTION: CMR2 0x0042b740
void Car_ClearRecords(int first, int count)
{
    int i;

    for (i = first; i < count + first; i++)
        memset(g_carBuffer + i, 0, sizeof(Car));
}

float Graphics_GetFrameRate(void);
void Graphics_SetPlaybackRate(float value);
void StageObject_SetFrameRate(int value);
extern int g_unk0x0053c9a8;

// GLOBAL: CMR2 0x005210c8
float g_65536f = 65536.0f;

// Counts down the frame-rate hold (restoring rate 1.0 when it ends) and
// passes the current frame rate on to the stage objects.
// FUNCTION: CMR2 0x0042b790
void Physics_UpdateRateHold(void)
{
    if (g_unk0x0053c9a8 != 0) {
        if (g_unk0x0053c9a8 == 1)
            Graphics_SetPlaybackRate(1.0f);
        g_unk0x0053c9a8--;
    }
    {
        float rate = Graphics_GetFrameRate();
        // PORT: fld/fmul/fistp, rounding to nearest.
        int scaled = (int)lrintf(rate * g_65536f);

        StageObject_SetFrameRate(scaled);
    }
}

// Sets the physics scale (value / 25, at least 0.6) and the time step
// (its reciprocal).
// FUNCTION: CMR2 0x00433840
void Physics_SetScale(int value)
{
    g_physicsScale = FixMul(0xa3d, value);
    if (g_physicsScale < 0x9999)
        g_physicsScale = 0x9999;
    g_physicsTimeStep = FixDiv(0x10000, g_physicsScale);
}

void FixMatrix_GetForward(FixVector *pOut, FixMatrix *pM);

// Heading (12-bit angle) of a view's camera from its forward vector.
// FUNCTION: CMR2 0x00421fe0
void View_GetHeading(short *pOut, unsigned int view)
{
    FixVector forward;
    int az;
    int ax;
    short angle;

    FixMatrix_GetForward(&forward, &g_viewStates[view & 0xff].matrix);
    az = forward.z < 0 ? -forward.z : forward.z;
    ax = forward.x < 0 ? -forward.x : forward.x;
    angle = FixAtan2(ax, az);
    *pOut = angle;
    if (forward.z < 0)
        *pOut = 0x800 - angle;
    if (forward.x < 0)
        *pOut = -*pOut;
}

void FixMatrix_CopyRotationFrom(FixMatrix *pDst, FixMatrix *pSrc);
void FixMatrix_SetPosition(FixVector *pV, FixMatrix *pM);

// A car's body matrix, raised by its camera shake when that option is on.
// FUNCTION: CMR2 0x00423a30
FixMatrix *View_GetCarBodyMatrix(FixMatrix *pOut, BYTE car)
{
    FixVector up;
    FixVector position;
    int shake;

    FixMatrix_CopyRotationFrom(pOut, Car_Get(car)->pBodyMatrix);
    if (CGameInfo::IsActiveCheatEnabled(6)) {
        FixMatrix_GetUp(&up, Car_Get(car)->pWorld);
        shake = FixMul(Car_Get(car)->cheatBodyLift, 0x8000);
        FixVecScale(&up, &up, shake);
        FixMatrix_GetPosition(&position, pOut);
        position.x += up.x;
        position.y += up.y;
        position.z += up.z;
        FixMatrix_SetPosition(&position, pOut);
    }
    return pOut;
}

int RallyData_GetRouteAdditionalState(void);
void RallyData_GetRouteNodeGroundPosition(int index, int *pOut);
int StageObject_UsesExtendedMode(void);

typedef void (*CarFadeCallback)(BYTE index);
void NetRace_FadeOutPlayerScreen(BYTE index, CarFadeCallback pfnDone, int param3, int param4, int param5, char force);

void Car_ResetToStage(unsigned int param_1);

// Starts the reset fade for the current car when its reset request is active.
// FUNCTION: CMR2 0x00431c10
void Car_StartResetFade(void)
{
    union {
        BYTE channels[4];
        int value;
    } colour;

    colour.channels[0] = 0;
    colour.channels[1] = 0;
    colour.channels[2] = 0;
    colour.channels[3] = 0;

    if (g_pCurrentCar->field_0xbf8 != 0)
        NetRace_FadeOutPlayerScreen((BYTE)g_pCurrentCar->index, (CarFadeCallback)Car_ResetToStage, 2, 1, colour.value, 0);
}

// Relative position of the current car against every other car in the race
// order: returns 1 as soon as one overlaps it (distance below the sum of the
// two body radii), keeping the relative position in 0x53ca58 and its largest
// absolute component in 0x53ca24.
// GLOBAL: CMR2 0x0053c9c4
int g_unk0x0053c9c4;
// GLOBAL: CMR2 0x0053ca24
int g_unk0x0053ca24;
// GLOBAL: CMR2 0x0053ca58
FixVector g_unk0x0053ca58;
// GLOBAL: CMR2 0x0053cae8
FixVector g_unk0x0053cae8;
int RallyData_GetActiveCarRaceRecordField0(Car *p);
RallyRouteNode *RallyData_GetAvailableRouteNodeRecord(int index);
int RallyData_GetRouteAvailabilityState(void);
void RallyRoute_CopyIndexedNodeDirection(unsigned int nodeIndex, FixVector *pOut);
int Car_OverlapsOtherCar(int param_1);
void Car_PlaceAtStart(int *param_1, int *param_2);
void Race_StorePlayerRouteAndClearSlots(int player);
void View_ResetCameras(int view);
void Particle_KillAll(void);

// Restores the car of the given player to the stage start: finds the closest
// record of the player that is still valid, takes its position and heading
// from the scene node (or from the stored body vectors when the record has no
// node), rebuilds the body frame, kills the particles of the previous state
// and clears the reset flags.
// FUNCTION: CMR2 0x00431c50
void Car_ResetToStage(unsigned int param_1)
{
    FixVector heading;
    FixVector position;
    int node;
    RallyRouteNode *pRecord;

    g_pCurrentCar = Car_Get(param_1 & 0xff);
    node = RallyData_GetActiveCarRaceRecordField0(g_pCurrentCar);
    pRecord = RallyData_GetAvailableRouteNodeRecord(node);
    if (pRecord != NULL) {
        while (Car_OverlapsOtherCar(node) != 0) {
            node--;
            if (node < 0) {
                if (RallyData_GetRouteAdditionalState() != 0)
                    node += RallyData_GetRouteAvailabilityState();
                else
                    node = 0;
            }
        }
        while ((pRecord->flags & 2) != 0 && node > 0) {
            node--;
            pRecord = RallyData_GetAvailableRouteNodeRecord(node);
        }
        RallyRoute_CopyIndexedNodeDirection(node, &heading);
        RallyData_GetRouteNodeGroundPosition(node, (int *)&position);
    } else {
        position = g_pCurrentCar->position;
        heading = g_pCurrentCar->right;
    }
    Car_PlaceAtStart((int *)&position, (int *)&heading);
    Race_StorePlayerRouteAndClearSlots(param_1 & 0xff);
    Particle_KillAll();
    if ((BYTE)param_1 < (BYTE)RallyDataState())
        View_ResetCameras(param_1);
    g_pCurrentCar->field_0xbf8 = 0;
    g_pCurrentCar->field_0xa7c = 0;
    g_pCurrentCar->field_0xa80 = 0;
}

// Checks whether the current car overlaps another car in extended mode.
// FUNCTION: CMR2 0x00431d80
int Car_OverlapsOtherCar(int param_1)
{
    short *pOrder;
    short count;
    short i;
    int result;
    Car *pOther;

    result = 0;
    if (param_1 == 0 && RallyData_GetRouteAdditionalState() == 0)
        return 0;
    pOrder = Car_GetOrder();
    count = Car_GetOrderCount();
    RallyData_GetRouteNodeGroundPosition(param_1, (int *)&g_unk0x0053cae8);
    for (i = 0; i < count; i++) {
        pOther = Car_Get(pOrder[i]);
        if (pOther->index != g_pCurrentCar->index && StageObject_UsesExtendedMode() != 0) {
            g_unk0x0053ca58.x = g_unk0x0053cae8.x - pOther->position.x;
            g_unk0x0053ca58.y = g_unk0x0053cae8.y - pOther->position.y;
            g_unk0x0053ca58.z = g_unk0x0053cae8.z - pOther->position.z;
            if (FIX_ABS(g_unk0x0053ca58.x) > FIX_ABS(g_unk0x0053ca58.y) &&
                FIX_ABS(g_unk0x0053ca58.x) > FIX_ABS(g_unk0x0053ca58.z))
                g_unk0x0053ca24 = FIX_ABS(g_unk0x0053ca58.x);
            else if (FIX_ABS(g_unk0x0053ca58.y) > FIX_ABS(g_unk0x0053ca58.x) &&
                     FIX_ABS(g_unk0x0053ca58.y) > FIX_ABS(g_unk0x0053ca58.z))
                g_unk0x0053ca24 = FIX_ABS(g_unk0x0053ca58.y);
            else
                g_unk0x0053ca24 = FIX_ABS(g_unk0x0053ca58.z);
            if (g_unk0x0053ca24 != 0) {
                FixVecScaleRecip(&g_unk0x0053ca58, &g_unk0x0053ca58, g_unk0x0053ca24);
            }
            g_unk0x0053c9c4 = FixVecLength(&g_unk0x0053ca58);
            g_unk0x0053c9c4 = FixMul(g_unk0x0053c9c4, g_unk0x0053ca24);
            if (g_unk0x0053c9c4 < pOther->collisionRadius + g_pCurrentCar->collisionRadius) {
                result = 1;
                i = count;
            }
        }
    }
    return result;
}

// Counts the current car's wheels near the ground and shares its weight out.
// FUNCTION: CMR2 0x00432b30
void Car_ShareWeightOnWheels(void)
{
    int i;
    int d;

    g_pCurrentCar->field_0xb28 = 0;
    for (i = 0; i < 4; i++) {
        d = FIX_ABS(g_pCurrentCar->cornerHeight[i] - g_pCurrentCar->corners[i].y);
        if (d < 0xcccc) {
            g_pCurrentCar->cornerOnGround[i] = 1;
            g_pCurrentCar->field_0xb28++;
        } else {
            g_pCurrentCar->cornerOnGround[i] = 0;
        }
    }
    if (g_pCurrentCar->field_0xb28 == 0)
        g_pCurrentCar->tyreGrip = 0;
    else if (g_pCurrentCar->field_0xb28 == 4)
        g_pCurrentCar->tyreGrip = g_pCurrentCar->mass / 4;
    else if (g_pCurrentCar->field_0xb28 == 2)
        g_pCurrentCar->tyreGrip = g_pCurrentCar->mass / 2;
    else if (g_pCurrentCar->field_0xb28 == 1)
        g_pCurrentCar->tyreGrip = g_pCurrentCar->mass;
    else
        g_pCurrentCar->tyreGrip = g_pCurrentCar->mass / g_pCurrentCar->field_0xb28;
}

void Car_FilterWheelSpin(void);

// Updates the engine torque figure and splits it between the wheels, or
// clears the wheel torques when the car is off its wheels.
// FUNCTION: CMR2 0x00437dc0
void Car_UpdateEngineTorque(void)
{
    int value;

    if (g_pCurrentCar->revLimiterActive != 0) {
        if (g_pCurrentCar->gear != 0)
            g_pCurrentCar->engineNetTorque = -g_pCurrentCar->revLimiterTorque;
        else
            g_pCurrentCar->engineNetTorque = 0;
    } else {
        if (g_pCurrentCar->engineSpeed >= g_pCurrentCar->engineSpeedLimit) {
            g_pCurrentCar->engineNetTorque = 0;
        } else if (g_pCurrentCar->gear != 0 && g_pCurrentCar->shiftInProgress == 1) {
            g_pCurrentCar->engineNetTorque = FixMul(g_pCurrentCar->engineDragCoefficient, FixMul(g_pCurrentCar->engineSpeed, g_pCurrentCar->engineSpeed));
        } else {
            value = Car_GetScaledSteerFollowRate();
            g_pCurrentCar->engineNetTorque = value -
                FixMul(g_pCurrentCar->engineDragCoefficient, FixMul(g_pCurrentCar->engineSpeed, g_pCurrentCar->engineSpeed));
        }
    }
    if (g_pCurrentCar->gear != 0 && g_pCurrentCar->shiftInProgress == 0) {
        int torque = FixMul(g_pCurrentCar->engineNetTorque, g_pCurrentCar->gearRatio[g_pCurrentCar->gear]);
        int front = FixMul(torque, g_pCurrentCar->driveSplit) / 2;
        g_pCurrentCar->wheelTorque[0] = front;
        g_pCurrentCar->wheelTorque[1] = front;
        g_pCurrentCar->wheelTorque[2] = torque / 2 - front;
        g_pCurrentCar->wheelTorque[3] = torque / 2 - front;
    } else {
        g_pCurrentCar->wheelTorque[0] = 0;
        g_pCurrentCar->wheelTorque[1] = 0;
        g_pCurrentCar->wheelTorque[2] = 0;
        g_pCurrentCar->wheelTorque[3] = 0;
    }
    Car_UpdateTyreForces();
    Car_FilterWheelSpin();
}

// Filters each driven wheel's spin (peak hold with decay) while it keeps the
// same surface and touches the ground.
// FUNCTION: CMR2 0x00437fd0
void Car_FilterWheelSpin(void)
{
    int i;
    int a;
    int d;

    for (i = 0; i < 4; i++) {
        if (g_pCurrentCar->previousWheelSurface[i] == g_pCurrentCar->wheelSurface[i] && g_pCurrentCar->cornerOnGround[i] != 0 &&
            g_pCurrentCar->field_0xb74 != 0) {
            a = g_pCurrentCar->wheelSlip[i] < 0 ? -g_pCurrentCar->wheelSlip[i] : g_pCurrentCar->wheelSlip[i];
            d = a - g_pCurrentCar->wheelSpinForWheelLean[i];
            if (d < 0)
                g_pCurrentCar->wheelSpinForWheelLean[i] = a;
            else
                g_pCurrentCar->wheelSpinForWheelLean[i] += FixMul(d, g_pCurrentCar->cornerGrip[i].field_0x18);
            a = g_pCurrentCar->wheelSlipLateral[i] < 0 ? -g_pCurrentCar->wheelSlipLateral[i] : g_pCurrentCar->wheelSlipLateral[i];
            d = a - g_pCurrentCar->wheelSpinForBodyLean[i];
            if (d < 0)
                g_pCurrentCar->wheelSpinForBodyLean[i] = a;
            else
                g_pCurrentCar->wheelSpinForBodyLean[i] += FixMul(d, g_pCurrentCar->cornerGrip[i].field_0x18);
        } else {
            g_pCurrentCar->wheelSpinForBodyLean[i] = 0;
            g_pCurrentCar->wheelSpinForWheelLean[i] = 0;
        }
    }
}

// GLOBAL: CMR2 0x00538d20
int g_unk0x00538d20[2];
// GLOBAL: CMR2 0x005391a8
int g_unk0x005391a8[2];
// GLOBAL: CMR2 0x005391b0
int g_unk0x005391b0[2];
// GLOBAL: CMR2 0x005391c4
int g_unk0x005391c4[2];

void ChaseCamera_Reset(BYTE index);
int SceneNode_Destroy(SceneNode *pNode);

// Releases both players' view nodes and resets their view state (callback).
// match 62%: revisada. El original direcciona los arrays de ints con un
// desplazamiento EN BYTES sobre el sÃ­mbolo ([esi + g_viewNodes],
// [esi + g_unk0x005391b0], ...), no con Ã­ndice escalado; reproducido con
// ((BYTE *)array + off). Lo que queda es asignaciÃ³n de registros: el contador
// en AL (no EAX) y quÃ© arrays strength-reduce MSVC (g_activeViewRecord).
// match 62%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00421590
BYTE View_ReleaseNodes(void)
{
    BYTE i;
    int off;

    for (i = 0, off = 0; i < 2; off += 4) {
        // off steps over ints; scaled where a pointer is wider (factor 1 on Win32).
        SceneNode **pNode = (SceneNode **)((BYTE *)g_viewNodes + off * (int)(sizeof(SceneNode *) / sizeof(int)));

        if (*pNode != NULL) {
            SceneNode_Destroy(*pNode);
            *pNode = NULL;
        }
        g_viewStates[i].type = 0;
        *(int *)((BYTE *)g_unk0x00538d20 + off) = -0x10000;
        g_activeViewRecord[i] = 0;
        *(int *)((BYTE *)g_unk0x005391b0 + off) = 0;
        *(int *)((BYTE *)g_unk0x005391c4 + off) = 0;
        *(int *)((BYTE *)g_unk0x005391a8 + off) = 0;
        *(int *)((BYTE *)g_unk0x00538f00 + off) = 0;
        ChaseCamera_Reset(i);
        i++;
    }
    return 1;
}

// Lifts the current car out of the ground by the deepest penetration of a
// free corner.
// FUNCTION: CMR2 0x0042f8c0
void Car_LiftFreeCorners(void)
{
    int lift = 0;
    int found = 0;
    int i;
    int d;
    int *pHeight;
    int *pCorner;
    int deepest = 0;

    i = 7;
    pCorner = &g_pCurrentCar->corners[7].y;
    pHeight = &g_pCurrentCar->cornerHeight[7];
    do {
        if (g_pCurrentCar->cornerFlags[i] == 0) {
            d = *pHeight - *pCorner;
            if (d > 0 && d > deepest) {
                deepest = d;
                found = 1;
            }
        }
        i--;
        pHeight--;
        pCorner -= 3;
    } while (i >= 0);
    if (found)
        lift = deepest;
    g_pCurrentCar->position.y += lift;
    g_pCurrentCar->corners[0].y += lift;
    g_pCurrentCar->corners[1].y += lift;
    g_pCurrentCar->corners[2].y += lift;
    g_pCurrentCar->corners[3].y += lift;
    g_pCurrentCar->corners[4].y += lift;
    g_pCurrentCar->corners[5].y += lift;
    g_pCurrentCar->corners[6].y += lift;
    g_pCurrentCar->corners[7].y += lift;
}

// Lifts the current car out of the ground by the deepest penetration of a
// free corner, rising or sinking as needed.
// FUNCTION: CMR2 0x0042f9d0
void Car_SettleFreeCorners(void)
{
    int lift = 0;
    int maxDrop = 0;
    int foundDrop = 0;
    int maxRise = -0x3e80000;
    int foundRise = 0;
    int i;
    int d;

    for (i = 7; i >= 0; i--) {
        if (g_pCurrentCar->cornerFlags[i] != 0)
            continue;
        d = g_pCurrentCar->cornerHeight[i] - g_pCurrentCar->corners[i].y;
        if (d >= 0 && d > maxDrop) {
            maxDrop = d;
            foundDrop = 1;
        }
        if (d <= 0 && d > maxRise) {
            maxRise = d;
            foundRise = 1;
        }
    }
    if (foundDrop)
        lift = maxDrop;
    else if (foundRise)
        lift = maxRise;
    g_pCurrentCar->position.y += lift;
    g_pCurrentCar->corners[0].y += lift;
    g_pCurrentCar->corners[1].y += lift;
    g_pCurrentCar->corners[2].y += lift;
    g_pCurrentCar->corners[3].y += lift;
    g_pCurrentCar->corners[4].y += lift;
    g_pCurrentCar->corners[5].y += lift;
    g_pCurrentCar->corners[6].y += lift;
    g_pCurrentCar->corners[7].y += lift;
}

// Per-wheel spin flag of each car (load change above half a unit this step).
// GLOBAL: CMR2 0x0053ac48
int g_wheelSpinning[8][4];

// Integrates each car's wheel rotation angles from the wheel loads and flags
// the wheels spinning fast.
// match 65%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x0042b4a0
void Car_IntegrateWheelRotation(int *pList, short count)
{
    int local_c;
    short *local_8;
    short local_14;

    local_c = count;
    if (local_c - 1 >= 0) {
        // The list holds shorts; the parameter is reused below for wheelLoad.
        local_8 = (short *)pList + (local_c - 1);
        do {
            int carIndex = *local_8;
            Car *carBase = g_carBuffer + carIndex;
            int w = 0;
            pList = carBase->wheelLoad;
            do {
                int v = FixMul(*pList, g_physicsTimeStep);
                if (carBase->field_0xb60 == 0 || (BYTE)carBase->flag0x1d0[2] > 0 ||
                    (BYTE)carBase->flag0x1d0[3] > 0 || carBase->handbrake > 0) {
                    local_14 = (short)FTOI(__int64, X87S_MUL((double)v, g_minusFixedRadiansToAngle12));
                    g_wheelRotation[carIndex][w] += local_14;
                }
                if (v < 0)
                    v = -v;
                if (v > 0x8000)
                    g_wheelSpinning[carIndex][w] = 1;
                else
                    g_wheelSpinning[carIndex][w] = 0;
                w++;
                pList++;
            } while (w < 4);
            local_8--;
            local_c--;
        } while (local_c != 0);
    }
}

int StageTiming_GetRelativeTyreRecordValue(char car, int wheel);
void Game_SetTriangleField2CByGroup(Mesh *pMesh, int mask, int value);

// Sets four wheel mesh states for each car in draw order.
// FUNCTION: CMR2 0x0042be30
void Car_UpdateWheelMeshStates(void)
{
    int order;
    int wheel;
    int car;
    int state;
    int appearance;
    SceneNode *pNode;

    for (order = g_carOrderCount - 1; order >= 0; order--) {
        car = g_carOrder[order];
        for (wheel = 0; wheel < 4; wheel++) {
            state = StageTiming_GetRelativeTyreRecordValue(car, wheel);
            if (g_wheelSpinning[car][wheel] != 0) {
                switch (state) {
                case 0: appearance = 6; break;
                case 1: appearance = 4; break;
                case 2: appearance = 7; break;
                case 3: appearance = 9; break;
                default: appearance = 6; break;
                }
            } else {
                switch (state) {
                case 0: appearance = 0; break;
                case 1: appearance = 3; break;
                case 2: appearance = 5; break;
                case 3: appearance = 8; break;
                default: appearance = 0; break;
                }
            }
            pNode = g_carBuffer[car].pWheelNodes[wheel];
            Game_SetTriangleField2CByGroup((Mesh *)pNode->pObject, 1, appearance);
            pNode = g_carBuffer[car].pExtraNodes[wheel];
            if (pNode != NULL)
                Game_SetTriangleField2CByGroup((Mesh *)pNode->pObject, 1, appearance);
        }
    }
}

// Smooths the current car's three force feedback levels.
// FUNCTION: CMR2 0x00442e90
void Car_SmoothForceFeedback(void)
{
    int i;
    int magnitude;
    int delta;

    i = 4;
    do {
        if (g_pCurrentCar->previousWheelSurface[i] == g_pCurrentCar->wheelSurface[i] &&
            g_pCurrentCar->cornerOnGround[i] != 0 && g_pCurrentCar->field_0xb74 != 0) {
            magnitude = g_pCurrentCar->wheelSlip[i];
            if (magnitude < 0) magnitude = -magnitude;
            delta = magnitude - g_pCurrentCar->wheelSpinForWheelLean[i];
            if (delta < 0)
                g_pCurrentCar->wheelSpinForWheelLean[i] = magnitude;
            else
                g_pCurrentCar->wheelSpinForWheelLean[i] += FixMul(delta, g_pCurrentCar->cornerGrip[i].field_0x18);

            magnitude = g_pCurrentCar->wheelSlipLateral[i];
            if (magnitude < 0) magnitude = -magnitude;
            delta = magnitude - g_pCurrentCar->wheelSpinForBodyLean[i];
            if (delta < 0)
                g_pCurrentCar->wheelSpinForBodyLean[i] = magnitude;
            else
                g_pCurrentCar->wheelSpinForBodyLean[i] += FixMul(delta, g_pCurrentCar->cornerGrip[i].field_0x18);
        } else {
            g_pCurrentCar->wheelSpinForBodyLean[i] = 0;
            g_pCurrentCar->wheelSpinForWheelLean[i] = 0;
        }
        i -= 2;
    } while (i >= 0);
    g_pCurrentCar->wheelSpinForBodyLean[0] = g_pCurrentCar->wheelSpinForBodyLean[1];
    g_pCurrentCar->wheelSpinForWheelLean[0] = g_pCurrentCar->wheelSpinForBodyLean[1];
    g_pCurrentCar->wheelSpinForBodyLean[2] = g_pCurrentCar->wheelSpinForBodyLean[3];
    g_pCurrentCar->wheelSpinForWheelLean[2] = g_pCurrentCar->wheelSpinForBodyLean[3];
}

// GLOBAL: CMR2 0x005391cc
int g_viewZooms[2];

extern const double g_fixedRadiansToAngle12;

// Integrates contact forces and body motion, including the transition to tumbling.
// match 70%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00435470
void Car_IntegrateContacts(void)
{
    FixVector force, torque, tmp, bodyForce, cross, accel, angAccel, posStep, angStep;
    short angles[3];
    int i, corner, scale, drag;

    Car_UpdateBodyAxes();
    Car_UpdateRollover();
    Car_DampenBodyMotion();
    torque.x = torque.y = torque.z = 0;
    force = g_pCurrentCar->baseForce;
    if (CGameInfo::IsActiveCheatEnabled(1)) {
        FixVecScale(&force, &force, 0x6666);
    } else {
        FixVecScale(&force, &force, 0xcccc);
    }
    if (g_pCurrentCar->field_0xb34 > 0) {
        scale = FixMul(g_pCurrentCar->speed, FixMul(0x10000, 0x13333));
        if (scale > 0x13333) scale = 0x13333;
        scale += 0x10000;
        for (i = g_pCurrentCar->field_0xb34 - 1; i >= 0; i--) {
            corner = ((signed char *)&g_pCurrentCar->field_0xb35[1])[i];
            if (corner >= 4) {
                FixVecScale(&tmp, &g_pCurrentCar->cornerForce[corner], scale);
                tmp.x += g_pCurrentCar->cornerLoad[corner].x;
                tmp.y += g_pCurrentCar->cornerLoad[corner].y;
                tmp.z += g_pCurrentCar->cornerLoad[corner].z;
                force.x += g_pCurrentCar->cornerForce[corner].x;
                force.y += g_pCurrentCar->cornerForce[corner].y;
                force.z += g_pCurrentCar->cornerForce[corner].z;
            } else if (g_pCurrentCar->useUpperCollisionCorners != 0) {
                if (g_pCurrentCar->field_0xb74 != 0) {
                    tmp = g_pCurrentCar->cornerForce[corner];
                    tmp.x += g_pCurrentCar->cornerLoad[corner].x;
                    tmp.y += g_pCurrentCar->cornerLoad[corner].y;
                    tmp.z += g_pCurrentCar->cornerLoad[corner].z;
                } else {
                    FixVecScale(&tmp, &g_pCurrentCar->cornerForce[corner], scale);
                    tmp.x += g_pCurrentCar->cornerLoad[corner].x;
                    tmp.y += g_pCurrentCar->cornerLoad[corner].y;
                    tmp.z += g_pCurrentCar->cornerLoad[corner].z;
                }
            } else {
                tmp.x = tmp.y = tmp.z = 0;
            }
            FixMatrix_InverseRotateVector(&bodyForce, &tmp, g_pCurrentCar->pWorld);
            FixVecCross(&cross, &bodyForce, &g_pCurrentCar->collisionCornersLocal[corner]);
            torque.x += cross.x;
            torque.y += cross.y;
            torque.z += cross.z;
        }
        if (g_pCurrentCar->useUpperCollisionCorners != 0) {
            if ((torque.x > 0 && g_pCurrentCar->angularVelocity.x < 0) ||
                (torque.x < 0 && g_pCurrentCar->angularVelocity.x > 0)) {
                int damping = FixMul(FIX_ABS(g_pCurrentCar->angularVelocity.x), 0x50000);
                if (damping > 0x10000) damping = 0x10000;
                torque.x = FixMul(torque.x, FixMul(0x10000 - damping, 0xf333));
            }
            if ((torque.y > 0 && g_pCurrentCar->angularVelocity.y < 0) ||
                (torque.y < 0 && g_pCurrentCar->angularVelocity.y > 0)) {
                int damping = FixMul(FIX_ABS(g_pCurrentCar->angularVelocity.y), 0x50000);
                if (damping > 0x10000) damping = 0x10000;
                torque.y = FixMul(torque.y, FixMul(0x10000 - damping, 0xcccc));
            }
            if ((torque.z > 0 && g_pCurrentCar->angularVelocity.z < 0) ||
                (torque.z < 0 && g_pCurrentCar->angularVelocity.z > 0)) {
                int damping = FixMul(FIX_ABS(g_pCurrentCar->angularVelocity.z), 0x50000);
                if (damping > 0x10000) damping = 0x10000;
                torque.z = FixMul(torque.z, FixMul(0x10000 - damping, 0xf333));
            }
        }
        for (i = 3; i >= 0; i--) {
            if (g_pCurrentCar->cornerOnGround[i] != 0) {
                tmp = g_pCurrentCar->cornerForce[i];
                force.x += tmp.x;
                force.y += tmp.y;
                force.z += tmp.z;
                if (g_pCurrentCar->useUpperCollisionCorners == 0) {
                    FixMatrix_InverseRotateVector(&bodyForce, &tmp, g_pCurrentCar->pWorld);
                    FixVecCross(&cross, &bodyForce, &g_pCurrentCar->wheelPos[i]);
                    torque.x += cross.x;
                    torque.y += cross.y;
                    torque.z += cross.z;
                }
            }
        }
    }
    if (g_pCurrentCar->useUpperCollisionCorners == 0 &&
        FixMul(FIX_ABS(g_pCurrentCar->field_0x5d0.x) + FIX_ABS(g_pCurrentCar->field_0x5d0.z),
               g_pCurrentCar->mass) > 0xbb80000) {
        g_pCurrentCar->useUpperCollisionCorners = 1;
        g_pCurrentCar->field_0x96c = 0x10000;
        g_pCurrentCar->field_0xc04[0] = 1;
        g_pCurrentCar->groundRightDot = FixVecDot(&g_pCurrentCar->right, &g_pCurrentCar->groundNormal);
        g_pCurrentCar->groundUpDot = FixVecDot(&g_pCurrentCar->up, &g_pCurrentCar->groundNormal);
        g_pCurrentCar->groundForwardDot = FixVecDot(&g_pCurrentCar->forward, &g_pCurrentCar->groundNormal);
    }
    drag = FixMul(FIX_ABS(g_pCurrentCar->speed), g_physicsPartSet->bodyDamageDrag + g_dragBase);
    FixVecScale(&tmp, &g_pCurrentCar->velocity, drag);
    force.x -= tmp.x;
    force.y -= tmp.y;
    force.z -= tmp.z;
    FixVecScale(&accel, &force, g_pCurrentCar->inverseMass);
    accel.x += g_pCurrentCar->field_0x5c4.x;
    accel.y += g_pCurrentCar->field_0x5c4.y;
    accel.z += g_pCurrentCar->field_0x5c4.z;
    angAccel.x = -FixMul(torque.x, g_pCurrentCar->inertia.x);
    angAccel.y = -FixMul(torque.y, g_pCurrentCar->inertia.y);
    angAccel.z = -FixMul(torque.z, g_pCurrentCar->inertia.z);
    angAccel.x -= FixMul(g_pCurrentCar->field_0x5d0.x, FixMul(g_pCurrentCar->inertia.x, g_pCurrentCar->mass));
    angAccel.y -= FixMul(g_pCurrentCar->field_0x5d0.y, FixMul(g_pCurrentCar->inertia.y, g_pCurrentCar->mass));
    angAccel.z -= FixMul(g_pCurrentCar->field_0x5d0.z, FixMul(g_pCurrentCar->inertia.z, g_pCurrentCar->mass));
    FixVecScale(&accel, &accel, 0x3333);
    g_pCurrentCar->velocityNext = g_pCurrentCar->velocity;
    FixVecScale(&accel, &accel, g_physicsTimeStep);
    g_pCurrentCar->velocity.x += accel.x;
    g_pCurrentCar->velocity.y += accel.y;
    g_pCurrentCar->velocity.z += accel.z;
    g_pCurrentCar->speed = FixVecLength(&g_pCurrentCar->velocity);
    FixVecScale(&angAccel, &angAccel, g_physicsTimeStep);
    g_pCurrentCar->angularVelocity.x += angAccel.x;
    g_pCurrentCar->angularVelocity.y += angAccel.y;
    g_pCurrentCar->angularVelocity.z += angAccel.z;
    FixVecScale(&posStep, &g_pCurrentCar->velocity, g_physicsTimeStep);
    FixVecScale(&accel, &accel, g_physicsTimeStep / 2);
    posStep.x -= accel.x;
    posStep.y -= accel.y;
    posStep.z -= accel.z;
    if (g_pCurrentCar->steepTime != 0) {
        if (FIX_ABS(g_pCurrentCar->angularVelocity.x) > 0x3333)
            g_pCurrentCar->angularVelocity.x = g_pCurrentCar->angularVelocity.x > 0 ? 0x3333 : -0x3333;
        if (FIX_ABS(g_pCurrentCar->angularVelocity.y) > 0x3333)
            g_pCurrentCar->angularVelocity.y = g_pCurrentCar->angularVelocity.y > 0 ? 0x3333 : -0x3333;
        if (FIX_ABS(g_pCurrentCar->angularVelocity.z) > 0x3333)
            g_pCurrentCar->angularVelocity.z = g_pCurrentCar->angularVelocity.z > 0 ? 0x3333 : -0x3333;
    }
    if (g_pCurrentCar->useUpperCollisionCorners == 0) {
        FixVecScale(&angStep, &g_pCurrentCar->angularVelocity, g_physicsTimeStep);
        FixVecScale(&angAccel, &angAccel, g_physicsTimeStep / 2);
        angStep.x -= angAccel.x;
        angStep.y -= angAccel.y;
        angStep.z -= angAccel.z;
        angles[0] = (short)FTOI(__int64, X87S_MUL((double)angStep.x, g_fixedRadiansToAngle12));
        angles[1] = (short)FTOI(__int64, X87S_MUL((double)angStep.y, g_fixedRadiansToAngle12));
        angles[2] = (short)FTOI(__int64, X87S_MUL((double)angStep.z, g_fixedRadiansToAngle12));
    } else {
        angles[0] = (short)FTOI(__int64, X87S_MUL((double)FixMul(g_pCurrentCar->angularVelocity.x, g_physicsTimeStep), g_fixedRadiansToAngle12));
        angles[1] = (short)FTOI(__int64, X87S_MUL((double)FixMul(g_pCurrentCar->angularVelocity.y, g_physicsTimeStep), g_fixedRadiansToAngle12));
        angles[2] = (short)FTOI(__int64, X87S_MUL((double)FixMul(g_pCurrentCar->angularVelocity.z, g_physicsTimeStep), g_fixedRadiansToAngle12));
    }
    if (g_pCurrentCar->field_0xb34 > 0) {
        int upDot = FixVecDot(&g_pCurrentCar->up, &g_pCurrentCar->groundNormal);
        if (FIX_ABS(angles[0]) < 10 && FIX_ABS(upDot) > 0xcccc) angles[0] = 0;
        if ((g_pCurrentCar->flag0x1d0[0] == 0 && g_pCurrentCar->flag0x1d0[1] == 0) ||
            FIX_ABS(g_pCurrentCar->speed) < 0x28f || g_pCurrentCar->field_0xb74 == 0) {
            if (FIX_ABS(angles[1]) < 10) angles[1] = 0;
        }
        if (FIX_ABS(angles[2]) < 10) angles[2] = 0;
        if (g_pCurrentCar->speed < 0xccc) posStep.x = posStep.y = posStep.z = 0;
    }
    if (g_pCurrentCar->field_0xbfc != 0) {
        if (g_pCurrentCar->field_0xbdc != 0) {
            g_pCurrentCar->angularVelocity.z = 0;
            g_pCurrentCar->angularVelocity.x = 0;
            angles[2] = angles[0] = 0;
        } else if (g_pCurrentCar->field_0xbe0 != 0) {
            g_pCurrentCar->angularVelocity.y = 0;
            g_pCurrentCar->angularVelocity.x = 0;
            angles[1] = angles[0] = 0;
        }
    }
    g_pCurrentCar->position.x += posStep.x;
    g_pCurrentCar->position.y += posStep.y;
    g_pCurrentCar->position.z += posStep.z;
    g_pCurrentCar->field_0xb60 = 0;
    if (angles[0] == 0 && angles[1] == 0 && angles[2] == 0) {
        if (g_pCurrentCar->speed < 0x51e) g_pCurrentCar->field_0xb60 = 1;
    } else {
        FixBasis_Rotate((FixBasis *)&g_pCurrentCar->right, (unsigned short *)angles);
    }
    g_pCurrentCar->pWorld->right = g_pCurrentCar->right;
    g_pCurrentCar->pWorld->up = g_pCurrentCar->up;
    g_pCurrentCar->pWorld->forward = g_pCurrentCar->forward;
    g_pCurrentCar->pWorld->position = g_pCurrentCar->position;
    Car_UpdateCorners(g_pCurrentCar);
}

// --- 0x0043e680 (layer 0) ----------------------------------------------------
void StageObject_UpdateCarCornerGroundHeights(Car *pCar, int count);
int StageObject_GetCarWeatherRampValue(Car *pCar);
void StageObject_CopyCarSurfaceNoiseTarget(Car *pCar);
void Car_UpdateSurfaceParams(Car *pCar, int blend);

// The original preserves the ground normal as up and normalises only right
// and right-cross-up (forward).
// Rebuilds the body basis of the car from the ground normal and the previous
// right vector, copies it into the render node, updates the corners and
// resets the per-stage state.
// FUNCTION: CMR2 0x0043e680
void Car_ResetBodyBasis(Car *param_1)
{
    FixVector v;
    int dot;
    int i;


    g_pCurrentCar = param_1;
    StageObject_UpdateCarCornerGroundHeights(param_1, 8);
    Car_UpdateGroundNormal();
    g_pCurrentCar->up = g_pCurrentCar->groundNormal;
    dot = FixVecDot(&g_pCurrentCar->right,
                    &g_pCurrentCar->groundNormal);
    FixVecScale(&v, &g_pCurrentCar->groundNormal, dot);
    v.x = g_pCurrentCar->right.x - v.x;
    v.y = g_pCurrentCar->right.y - v.y;
    v.z = g_pCurrentCar->right.z - v.z;
    {
        FixVector *pT = &g_pCurrentCar->right;
        int len = FixVecLength(&v);
        if (len == 0) {
            pT->x = 0;
            pT->y = 0;
            pT->z = 0;
        } else {
            FixVecScaleRecip(pT, &v, len);
        }
    }
    FixVecCross(&v, &g_pCurrentCar->right, &g_pCurrentCar->up);
    {
        FixVector *pT = &g_pCurrentCar->forward;
        int len = FixVecLength(&v);
        if (len == 0) {
            pT->x = 0;
            pT->y = 0;
            pT->z = 0;
        } else {
            FixVecScaleRecip(pT, &v, len);
        }
    }
    g_pCurrentCar->pWorld->right = g_pCurrentCar->right;
    g_pCurrentCar->pWorld->up = g_pCurrentCar->up;
    g_pCurrentCar->pWorld->forward = g_pCurrentCar->forward;
    Car_UpdateCorners(g_pCurrentCar);
    g_pCurrentCar->position.y += g_pCurrentCar->cornerHeight[0] - g_pCurrentCar->corners[0].y;
    g_pCurrentCar->pWorld->position = g_pCurrentCar->position;
    SceneNode_SetPosition(g_pCurrentCar->pBodyNode, &g_pCurrentCar->position);
    Car_UpdateCorners(g_pCurrentCar);
    Car_FlagAirborneCorners();
    g_pCurrentCar->rearWheelDir = g_pCurrentCar->right;
    Car_UpdateSurfaceParams(g_pCurrentCar, StageObject_GetCarWeatherRampValue(g_pCurrentCar));
    StageObject_CopyCarSurfaceNoiseTarget(g_pCurrentCar);
    for (i = 0; i < 4; i++) {
        g_pCurrentCar->cornerPrev[i] = g_pCurrentCar->corners[i];
        g_pCurrentCar->cornerPrev2[i] = g_pCurrentCar->cornerPrev[i];
    }
    for (i = 0; i < 8; i++)
        g_pCurrentCar->cornerNormal[i] = g_pCurrentCar->cornerAxis[i];
    g_pCurrentCar->normal0x498 = g_pCurrentCar->groundNormal;
    g_pCurrentCar->positionPrev = g_pCurrentCar->position;
    g_pCurrentCar->positionPrev2 = g_pCurrentCar->position;
    g_pCurrentCar->field_0x2dc = g_pCurrentCar->position;
    memcpy(g_pCurrentCar->targetAxes, g_pCurrentCar->bodyAxes, sizeof(g_pCurrentCar->targetAxes));
    g_pCurrentCar->field_0x960 = 0;
    g_pCurrentCar->field_0x968 = g_pCurrentCar->field_0x960;
    g_pCurrentCar->field_0x964 = g_pCurrentCar->field_0x960;
    g_pCurrentCar->field_0xb45 = 3;
    g_pCurrentCar->cornerOnGround[3] = 1;
    g_pCurrentCar->cornerOnGround[2] = 1;
    g_pCurrentCar->cornerOnGround[1] = 1;
    g_pCurrentCar->cornerOnGround[0] = 1;
    g_pCurrentCar->field_0xb28 = 4;
    g_pCurrentCar->field_0xb74 = 1;
    g_pCurrentCar->field_0xb60 = 1;

}

// --- 0x0042e450 (layer 0) ----------------------------------------------------
void CarPhysics_IntegrateWheelSuspension(void);

// match 47.56%: implementada; misma logica y mismos operandos de memoria, pero
// MSVC6 elige otros registros/slots en los bucles (la nuestra compila mas corta).
// Integrates the per-wheel suspension travel of the current car: the vertical
// span of each wheel is clamped against the ground and the pitch/roll rate, the
// offsets are normalised against the highest one and the wheel forces of the
// next frame are rebuilt.
// FUNCTION: CMR2 0x0042e450
void Car_IntegrateWheelTravel(void)
{
    int v[4];
    int dot;
    int sum;
    int avg;
    int max;
    int t;
    int d;
    int i;
    int a;
    int b;


    sum = 0;
    dot = FixVecDot(&g_pCurrentCar->up,
                    &g_pCurrentCar->groundNormal);
    if (dot > 0xcccc) {
        for (i = 0; i < 4; i++) {
            t = FixMul(-FixMul(g_pCurrentCar->corners[i].y - g_pCurrentCar->cornerHeight[i], g_pCurrentCar->groundNormal.y),
                       FixDiv(0x10000, dot));
            sum += t;
            v[i] = t;
            d = g_pCurrentCar->cornerHeight[i] - g_pCurrentCar->corners[i].y - g_pCurrentCar->groundHeightCorrection;
            if (d < 0) {
                if (d < g_pCurrentCar->field_0xa08)
                    g_pCurrentCar->field_0x928[i] = g_pCurrentCar->field_0xa08;
                else
                    g_pCurrentCar->field_0x928[i] = d;
            } else {
                g_pCurrentCar->field_0x928[i] = 0;
            }
        }
        avg = sum / 4;
        max = 0;
        for (i = 0; i < 4; i++) {
            t = FixMul(g_pCurrentCar->wheelTravelLoadScale, v[i] - avg);
            g_pCurrentCar->wheelTravelLoadBias[i] = t;
            if (max < FIX_ABS(t))
                max = FIX_ABS(t);
        }
        if (max > 0x10000) {
            for (i = 0; i < 4; i++)
                g_pCurrentCar->wheelTravelLoadBias[i] = FixMul(FixDiv(0x10000, max), g_pCurrentCar->wheelTravelLoadBias[i]);
        }
    } else {
        for (i = 0; i < 4; i++) {
            g_pCurrentCar->field_0x928[i] = 0;
            g_pCurrentCar->wheelTravelLoadBias[i] = 0;
        }
    }
    if (g_pCurrentCar->useUpperCollisionCorners != 0) {
        for (i = 0; i < 4; i++) {
            g_pCurrentCar->field_0x938[i] = 0;
            g_pCurrentCar->field_0x948[i] = 0;
        }
    } else {
        for (i = 0; i < 4; i++) {
            if (g_pCurrentCar->cornerGrip[i].field_0x1c < 1 || g_pCurrentCar->cornerFlags[i] != 0 ||
                g_pCurrentCar->field_0xb74 == 0) {
                g_pCurrentCar->field_0x938[i] = 0;
                g_pCurrentCar->field_0x948[i] = 0;
            } else {
                t = g_pCurrentCar->speed - FixVecDot(&g_pCurrentCar->cornerVelocity[i], &g_pCurrentCar->groundNormal);
                if (t > 0x10000)
                    t = 0x10000;
                else if (t < 0xccc)
                    t = 0;
                if (g_pCurrentCar->field_0x948[i] < 1) {
                    a = g_pCurrentCar->corners[i].x;
                    b = g_pCurrentCar->corners[i].z;
                    if (g_pCurrentCar->field_0x938[i] == 0) {
                        g_pCurrentCar->field_0x938[i] =
                            FixMul(FixMul((FIX_ABS(FIX_ABS(a) - FIX_ABS(b)) % 0x401) << 6, t),
                                   g_pCurrentCar->cornerGrip[i].field_0x1c);
                    } else {
                        g_pCurrentCar->field_0x938[i] = 0;
                        g_pCurrentCar->field_0x948[i] =
                            FixMul(FixMul((FIX_ABS(FIX_ABS(a) - FIX_ABS(b)) % 0x201) << 7, 0x320000),
                                   g_pCurrentCar->cornerGrip[i].field_0x20);
                    }
                } else {
                    g_pCurrentCar->field_0x948[i] -= t;
                    if (g_pCurrentCar->field_0x948[i] < 0)
                        g_pCurrentCar->field_0x948[i] = 0;
                }
            }
        }
    }
    CarPhysics_IntegrateWheelSuspension();

}

// --- 0x00431ff0 / 0x0043c7f0 ------------------------------------------------

// Places the current car at a stage start: rebuilds the body frame from the
// heading vector param_2 (only x and z are used) and the position param_1,
// orthonormalises it against the ground normal the corner update produced,
// writes the resulting matrix into the car and into the scene node matrices,
// and clears the whole per-stage state.
// match 56%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// Implementada; las 13 llamadas y los offsets de campo coinciden, difiere el reparto de registros/espacios de pila (MSVC6 reutiliza ADDs donde nosotros usamos desplazamientos).
// FUNCTION: CMR2 0x00431ff0
void Car_PlaceAtStart(int *param_1, int *param_2)
{
    FixVector v;
    int l;
    int dot;
    int i;


    v.x = param_2[0];
    v.y = 0;
    v.z = param_2[2];
    l = FixVecLength(&v);
    if (l == 0) {
        g_pCurrentCar->right.x = 0;
        g_pCurrentCar->right.y = 0;
        g_pCurrentCar->right.z = 0;
    } else {
        FixVecScaleRecip(&g_pCurrentCar->right, &v, l);
    }
    g_pCurrentCar->up.x = 0;
    g_pCurrentCar->up.y = 0x10000;
    g_pCurrentCar->up.z = 0;
    g_pCurrentCar->forward.x = g_pCurrentCar->right.z;
    g_pCurrentCar->forward.y = 0;
    g_pCurrentCar->forward.z = -g_pCurrentCar->right.x;
    g_pCurrentCar->position.x = param_1[0];
    g_pCurrentCar->position.y = param_1[1] - 0x640000;
    g_pCurrentCar->position.z = param_1[2];
    g_pCurrentCar->pWorld->right = g_pCurrentCar->right;
    g_pCurrentCar->pWorld->up = g_pCurrentCar->up;
    g_pCurrentCar->pWorld->forward = g_pCurrentCar->forward;
    Car_UpdateCorners(g_pCurrentCar);
    StageObject_UpdateCarCornerGroundHeights(g_pCurrentCar, 8);
    Car_UpdateGroundNormal();
    g_pCurrentCar->up = g_pCurrentCar->groundNormal;
    dot = FixVecDot(&g_pCurrentCar->up, &g_pCurrentCar->right);
    FixVecScale(&v, &g_pCurrentCar->up, dot);
    v.x = g_pCurrentCar->right.x - v.x;
    v.y = g_pCurrentCar->right.y - v.y;
    v.z = g_pCurrentCar->right.z - v.z;
    FIX_NORMALIZE_INTO(g_pCurrentCar->right, v);
    FixVecCross(&v, &g_pCurrentCar->right, &g_pCurrentCar->up);
    FIX_NORMALIZE_INTO(g_pCurrentCar->forward, v);
    g_pCurrentCar->pWorld->right = g_pCurrentCar->right;
    g_pCurrentCar->pWorld->up = g_pCurrentCar->up;
    g_pCurrentCar->pWorld->forward = g_pCurrentCar->forward;
    Car_UpdateCorners(g_pCurrentCar);
    StageObject_UpdateCarCornerGroundHeights(g_pCurrentCar, 8);
    Car_FlagAirborneCorners();
    Car_LiftFreeCorners();
    Car_FlagAirborneCorners();
    Car_ShareWeightOnWheels();
    g_pCurrentCar->pWorld->position = g_pCurrentCar->position;
    *g_pCurrentCar->pBodyMatrix = *g_pCurrentCar->pWorld;
    g_pCurrentCar->groundRightDot = 0;
    g_pCurrentCar->groundUpDot = 0x10000;
    g_pCurrentCar->groundForwardDot = 0;
    g_pCurrentCar->field_0xba0[2] = 0;
    g_pCurrentCar->field_0xba0[1] = 0;
    g_pCurrentCar->field_0xba0[0] = 0;
    g_pCurrentCar->velocity.x = 0;
    g_pCurrentCar->velocity.y = 0;
    g_pCurrentCar->velocity.z = 0;
    g_pCurrentCar->velocityNext.x = 0;
    g_pCurrentCar->velocityNext.y = 0;
    g_pCurrentCar->velocityNext.z = 0;
    g_pCurrentCar->speed = 0;
    g_pCurrentCar->angularVelocity.x = 0;
    g_pCurrentCar->angularVelocity.y = 0;
    g_pCurrentCar->angularVelocity.z = 0;
    for (i = 0; i < 9; i++)
        g_pCurrentCar->targetAxes[i] = g_pCurrentCar->bodyAxes[i];
    g_pCurrentCar->normal0x498 = g_pCurrentCar->groundNormal;
    for (i = 0; i < 8; i++)
        g_pCurrentCar->cornerNormal[i] = g_pCurrentCar->cornerAxis[i];
    g_pCurrentCar->field_0x5c4.x = 0;
    g_pCurrentCar->field_0x5c4.y = 0;
    g_pCurrentCar->field_0x5c4.z = 0;
    g_pCurrentCar->field_0x5d0.x = 0;
    g_pCurrentCar->field_0x5d0.y = 0;
    g_pCurrentCar->field_0x5d0.z = 0;
    g_pCurrentCar->field_0x2dc = g_pCurrentCar->position;
    g_pCurrentCar->positionPrev2 = g_pCurrentCar->position;
    g_pCurrentCar->positionPrev = g_pCurrentCar->position;
    for (i = 0; i < 4; i++) {
        g_pCurrentCar->cornerPrev2[i] = g_pCurrentCar->corners[i];
        g_pCurrentCar->cornerPrev[i] = g_pCurrentCar->cornerPrev2[i];
    }
    g_pCurrentCar->frontWheelAxis = g_pCurrentCar->forward;
    g_pCurrentCar->rearWheelDir = g_pCurrentCar->right;
    g_pCurrentCar->frontWheelDir = g_pCurrentCar->rearWheelDir;
    g_pCurrentCar->useUpperCollisionCorners = 0;
    g_pCurrentCar->field_0xc10 = 0;
    g_pCurrentCar->field_0x96c = 0;
    g_pCurrentCar->field_0xc04[0] = 0;
    g_pCurrentCar->steepTime = 0;
    g_pCurrentCar->field_0x95c = 0;
    g_pCurrentCar->tipAngle = 0;
    g_pCurrentCar->tipRatio = 0;
    g_pCurrentCar->field_0xb45 = 3;
    g_pCurrentCar->wheelLean.x = 0;
    g_pCurrentCar->wheelLean.y = 0;
    g_pCurrentCar->wheelLean.z = 0;
    g_pCurrentCar->lean.x = 0;
    g_pCurrentCar->lean.y = 0;
    g_pCurrentCar->lean.z = 0;
    for (i = 0; i < 4; i++)
        g_pCurrentCar->wheelLoad[i] = 0;
    g_pCurrentCar->throttleTorque = 0;
    g_pCurrentCar->throttlePhase = 0;
    g_pCurrentCar->engineSpeed = 0;
    g_pCurrentCar->field_0x7ac = 0;
    g_pCurrentCar->brakeInput = 0;
    g_pCurrentCar->brakePhase = 0;
    g_pCurrentCar->handbrakeForce = 0;
    g_pCurrentCar->handbrakePhase = 0;
    g_pCurrentCar->steeringAccumulator = 0;
    for (i = 0; i < 8; i++)
        g_pCurrentCar->cornerTriangle[i] = 0;
    g_pCurrentCar->wheelSteeringAngle = 0;
    g_pCurrentCar->targetSteeringAngle = 0;
    g_pCurrentCar->field_0xbfc = 0;
    g_pCurrentCar->field_0xbe4 = 0;
    g_pCurrentCar->field_0xbe0 = 0;
    g_pCurrentCar->field_0xbdc = 0;
    g_pCurrentCar->automaticReverse = 0;
    g_pCurrentCar->gear = 1;
    g_pCurrentCar->revLimiterActive = 0;
    g_pCurrentCar->field_0x960 = 0;
    g_pCurrentCar->field_0x964 = g_pCurrentCar->field_0x960;
    g_pCurrentCar->field_0x968 = g_pCurrentCar->field_0x960;
    Car_InvalidateTransforms((int)g_pCurrentCar->index);
    g_pCurrentCar->field_0xc04[1] = 1;
    g_pCurrentCar->field_0x970[0] = 0x320000;
    g_pCurrentCar->field_0xc14 = 1;
    l = StageObject_GetCarWeatherRampValue(g_pCurrentCar);
    Car_UpdateSurfaceParams(g_pCurrentCar, l);
    StageObject_CopyCarSurfaceNoiseTarget(g_pCurrentCar);

}

// Mirror of the CFrontend statics used here (espejo de Frontend.h).
class CFrontend
{
public:
    static int GetArchivePrimaryFlagEntry(int index);
    static int GetArchiveSecondaryFlagEntry(int index);
    static int GetArchivePrimaryIDEntry(int index);
};

unsigned char RallyData_GetSelectionFlag28(void);
unsigned int StageTiming_GetTotalCarCount(void);
int Replay_GetSelectionStateByte(void);
BYTE RallyData_GetDriverAutomaticGearbox(BYTE param1);
CarSetup *RallyData_GetCountryCarSetup(void);
CarSetup *RallyData_GetChallengeCarSetup(void);
CarSetup *RallyData_GetPlayerCarSetup(int index);
CarSetup *RallyData_GetAlternateCountryCarSetup(void);
BYTE StageUI_GetRaceEndEventCount(void);

// Spawns the current car at a stage/restart: picks the car model from the
// rally selection, sets up its tuning from the per-model table, its body box
// from the model dimensions and all the per-stage counters.
// match 52%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// Implementada; las 68 llamadas coinciden en simbolo y frecuencia y no hay offsets inventados; difieren el reparto de registros y algunas expresiones reasociadas por el optimizador.
// FUNCTION: CMR2 0x0043c7f0
void Car_Spawn(Car *param_1, SceneNode *param_2, int param_3, FixVector *param_4, int param_5,
                  int param_6)
{
    FixVector v;
    int i;
    int carType;
    int limit;
    int bVar6;
    int flag8;
    int flagC;
    int a;
    int b;
    int c;
    int hx;
    int hy;
    int hz;
    CarSetup *pSetup;


    limit = 0;
    flag8 = 0;
    flagC = 0;
    g_pCurrentCar = param_1;
    if (RallyData_GetSelectionFlag28() != 0 && CGameInfo::GetSoundOptionBit30() != 0 &&
        param_5 == (int)StageTiming_GetTotalCarCount() - 1) {
        if (Replay_GetSelectionStateByte() == -1) {
            carType = 0;
            flag8 = 1;
        } else {
            carType = CFrontend::GetArchivePrimaryIDEntry(RallyData_GetDriverRecordSelectionValue((BYTE)Replay_GetSelectionStateByte()));
            flagC = 1;
        }
    } else {
        carType = param_3;
    }
    g_pCurrentCar->index = (char)param_5;
    g_pCurrentCar->type = (char)carType;
    if (param_6 == 0)
        Car_BindModel(param_5, param_1);
    g_pCurrentCar->cheatWheelDrop = FixMul(0x5999, 0x20000);
    g_pCurrentCar->cheatBodyLift = g_pCurrentCar->cheatWheelDrop + FixMul(0x5999, 0x18000);
    for (i = 0; i < 4; i++) {
        FixMatrix_GetPosition(&g_pCurrentCar->wheelEmitter[i],
                              &g_pCurrentCar->pWheelNodes[i]->local);
        if (CGameInfo::IsActiveCheatEnabled(6) != 0)
            g_pCurrentCar->wheelEmitter[i].y -= g_pCurrentCar->cheatWheelDrop;
    }
    g_pCurrentCar->position = *param_4;
    g_pCurrentCar->gear = 0;
    FixMatrix *pBasis = &param_2->current;
    g_pCurrentCar->pWorld = &g_pCurrentCar->physicsMatrix;
    FixMatrix_GetRight(&g_pCurrentCar->right, pBasis);
    FixMatrix_GetUp(&g_pCurrentCar->up, pBasis);
    FixMatrix_GetForward(&g_pCurrentCar->forward, pBasis);
    g_pCurrentCar->pWorld->right = g_pCurrentCar->right;
    g_pCurrentCar->pWorld->up = g_pCurrentCar->up;
    g_pCurrentCar->pWorld->forward = g_pCurrentCar->forward;
    g_pCurrentCar->pBodyMatrix = &g_pCurrentCar->bodyMatrix;
    FixMatrix_GetRight(&v, &g_pCurrentCar->pBodyNode->current);
    g_pCurrentCar->pWorld->right = v;
    FixMatrix_GetUp(&v, &g_pCurrentCar->pBodyNode->current);
    g_pCurrentCar->pWorld->up = v;
    FixMatrix_GetForward(&v, &g_pCurrentCar->pBodyNode->current);
    g_pCurrentCar->pWorld->forward = v;
    switch (carType) {
    case 3:
        limit = 0x3e80000;
        g_pCurrentCar->bodySize.x = 0x44560;
        g_pCurrentCar->bodySize.y = 0x15eb8;
        g_pCurrentCar->bodySize.z = 0x1cfdf;
        g_pCurrentCar->maxThrottleTorque = 0x570a;
        g_pCurrentCar->engineSpeedLimit = 0x98b02;
        g_pCurrentCar->scale0x764 = 0x1cccc;
        g_pCurrentCar->scale0x768 = 0x14ccc;
        g_pCurrentCar->scale0x76c = 0x4ccc;
        g_pCurrentCar->field_0x770[1] = 0xb0a3;
        g_pCurrentCar->field_0x770[0] = 0xfa9f;
        break;
    case 0:
        limit = 0x3e80000;
        g_pCurrentCar->bodySize.x = 0x426e9;
        g_pCurrentCar->bodySize.y = 0x16b85;
        g_pCurrentCar->bodySize.z = 0x1c51e;
        g_pCurrentCar->maxThrottleTorque = 0x570a;
        g_pCurrentCar->engineSpeedLimit = 0x98b02;
        g_pCurrentCar->scale0x764 = 0x1cccc;
        g_pCurrentCar->scale0x768 = 0x8000;
        g_pCurrentCar->scale0x76c = 0x3d70;
        g_pCurrentCar->field_0x770[1] = 0xcf5c;
        g_pCurrentCar->field_0x770[0] = 0x10ccc;
        break;
    case 6:
        limit = 0x3a60000;
        g_pCurrentCar->bodySize.x = 0x3e3d7;
        g_pCurrentCar->bodySize.y = 0x15eb8;
        g_pCurrentCar->bodySize.z = 0x1c28f;
        g_pCurrentCar->maxThrottleTorque = 0x63d7;
        g_pCurrentCar->engineSpeedLimit = 0x98b02;
        g_pCurrentCar->scale0x764 = 0x1ae14;
        g_pCurrentCar->scale0x768 = 0x9c28;
        g_pCurrentCar->scale0x76c = 0x4ccc;
        g_pCurrentCar->field_0x770[1] = 0xcf5c;
        g_pCurrentCar->field_0x770[0] = 0xfae1;
        break;
    case 7:
        limit = 0x3c70000;
        g_pCurrentCar->bodySize.x = 0x475c2;
        g_pCurrentCar->bodySize.y = 0x1570a;
        g_pCurrentCar->bodySize.z = 0x1c28f;
        g_pCurrentCar->maxThrottleTorque = 0x4a3d;
        g_pCurrentCar->engineSpeedLimit = 0x88b02;
        g_pCurrentCar->scale0x764 = 0x1e147;
        g_pCurrentCar->scale0x768 = 0x1028f;
        g_pCurrentCar->scale0x76c = 0x4ccc;
        g_pCurrentCar->field_0x770[1] = 0xcf5c;
        g_pCurrentCar->field_0x770[0] = 0xfae1;
        break;
    case 2:
        limit = 0x3e80000;
        g_pCurrentCar->bodySize.x = 0x40f5c;
        g_pCurrentCar->bodySize.y = 0x163d7;
        g_pCurrentCar->bodySize.z = 0x1c51e;
        g_pCurrentCar->maxThrottleTorque = 0x5687;
        g_pCurrentCar->engineSpeedLimit = 0x98b02;
        g_pCurrentCar->scale0x764 = 0x1cccc;
        g_pCurrentCar->scale0x768 = 0x451e;
        g_pCurrentCar->scale0x76c = 0x4ccc;
        g_pCurrentCar->field_0x770[1] = 0xcf5c;
        g_pCurrentCar->field_0x770[0] = 0x1147a;
        break;
    case 1:
        limit = 0x4000000;
        g_pCurrentCar->bodySize.x = 0x4451e;
        g_pCurrentCar->bodySize.y = 0x15999;
        g_pCurrentCar->bodySize.z = 0x1d70a;
        g_pCurrentCar->maxThrottleTorque = 0x570a;
        g_pCurrentCar->engineSpeedLimit = 0x98b02;
        g_pCurrentCar->scale0x764 = 0x1cccc;
        g_pCurrentCar->scale0x768 = 0xf851;
        g_pCurrentCar->scale0x76c = 0x570a;
        g_pCurrentCar->field_0x770[1] = 0xcf5c;
        g_pCurrentCar->field_0x770[0] = 0x1147a;
        break;
    case 8:
        limit = 0x1bf0000;
        g_pCurrentCar->bodySize.x = 0x30083;
        g_pCurrentCar->bodySize.y = 0x14041;
        g_pCurrentCar->bodySize.z = 0x18000;
        g_pCurrentCar->maxThrottleTorque = 0x3ae1;
        g_pCurrentCar->engineSpeedLimit = 0x88b02;
        g_pCurrentCar->scale0x764 = 0x13333;
        g_pCurrentCar->scale0x768 = 0x4ccc;
        g_pCurrentCar->scale0x76c = 0x2666;
        g_pCurrentCar->field_0x770[1] = 0xb5c2;
        g_pCurrentCar->field_0x770[0] = 0xfa9f;
        break;
    case 5:
        limit = 0x3e80000;
        g_pCurrentCar->bodySize.x = 0x41c28;
        g_pCurrentCar->bodySize.y = 0x154bc;
        g_pCurrentCar->bodySize.z = 0x1d47a;
        g_pCurrentCar->maxThrottleTorque = 0x5c28;
        g_pCurrentCar->engineSpeedLimit = 0x98b02;
        g_pCurrentCar->scale0x764 = 0x1c000;
        g_pCurrentCar->scale0x768 = 0x10000;
        g_pCurrentCar->scale0x76c = 0x4ccc;
        g_pCurrentCar->field_0x770[1] = 0xe3d7;
        g_pCurrentCar->field_0x770[0] = 0x12dd2;
        break;
    case 4:
        limit = 0x3e80000;
        g_pCurrentCar->bodySize.x = 0x40312;
        g_pCurrentCar->bodySize.y = 0x14ccc;
        g_pCurrentCar->bodySize.z = 0x1c51e;
        g_pCurrentCar->maxThrottleTorque = 0x570a;
        g_pCurrentCar->engineSpeedLimit = 0x98b02;
        g_pCurrentCar->scale0x764 = 0x1c000;
        g_pCurrentCar->scale0x768 = 0xcccc;
        g_pCurrentCar->scale0x76c = 0x4ccc;
        g_pCurrentCar->field_0x770[1] = 0xe3d7;
        g_pCurrentCar->field_0x770[0] = 0x12dd2;
        break;
    case 9:
        limit = 0x3450000;
        g_pCurrentCar->bodySize.x = 0x3b958;
        g_pCurrentCar->bodySize.y = 0x15db2;
        g_pCurrentCar->bodySize.z = 0x1e041;
        g_pCurrentCar->maxThrottleTorque = 0x6666;
        g_pCurrentCar->engineSpeedLimit = 0x98b02;
        g_pCurrentCar->scale0x764 = 0x1a666;
        g_pCurrentCar->scale0x768 = 0x9999;
        g_pCurrentCar->scale0x76c = 0x4ccc;
        g_pCurrentCar->field_0x770[1] = 0xe3d7;
        g_pCurrentCar->field_0x770[0] = 0xe106;
        break;
    case 11:
        limit = 0x2fc0000;
        g_pCurrentCar->bodySize.x = 0x3d333;
        g_pCurrentCar->bodySize.y = 0x15999;
        g_pCurrentCar->bodySize.z = 0x1c312;
        g_pCurrentCar->maxThrottleTorque = 0x75c2;
        g_pCurrentCar->engineSpeedLimit = 0x98b02;
        g_pCurrentCar->scale0x764 = 0x1a666;
        g_pCurrentCar->scale0x768 = 0x9999;
        g_pCurrentCar->scale0x76c = 0x4ccc;
        g_pCurrentCar->field_0x770[1] = 0xe3d7;
        g_pCurrentCar->field_0x770[0] = 0xe106;
        break;
    case 10:
        limit = 0x2dc0000;
        g_pCurrentCar->bodySize.x = 0x3b333;
        g_pCurrentCar->bodySize.y = 0x106a7;
        g_pCurrentCar->bodySize.z = 0x1cf5c;
        g_pCurrentCar->maxThrottleTorque = 0x570a;
        g_pCurrentCar->engineSpeedLimit = 0x98b02;
        g_pCurrentCar->scale0x764 = 0x1a666;
        g_pCurrentCar->scale0x768 = 0x13333;
        g_pCurrentCar->scale0x76c = 0x4ccc;
        g_pCurrentCar->field_0x770[1] = 0xca3d;
        g_pCurrentCar->field_0x770[0] = 0xe106;
        break;
    case 12:
        limit = 0x34e0000;
        g_pCurrentCar->bodySize.x = 0x3ec49;
        g_pCurrentCar->bodySize.y = 0x146a7;
        g_pCurrentCar->bodySize.z = 0x1c28f;
        g_pCurrentCar->maxThrottleTorque = 0x570a;
        g_pCurrentCar->engineSpeedLimit = 0x98b02;
        g_pCurrentCar->scale0x764 = 0x1a666;
        g_pCurrentCar->scale0x768 = 0x13333;
        g_pCurrentCar->scale0x76c = 0x4ccc;
        g_pCurrentCar->field_0x770[1] = 0xca3d;
        g_pCurrentCar->field_0x770[0] = 0xe106;
        break;
    case 13:
        limit = 0x1bf0000;
        g_pCurrentCar->bodySize.x = 0x41687;
        g_pCurrentCar->bodySize.y = 0x16147;
        g_pCurrentCar->bodySize.z = 0x1bb22;
        g_pCurrentCar->maxThrottleTorque = 0x3ae1;
        g_pCurrentCar->engineSpeedLimit = 0x88b02;
        g_pCurrentCar->scale0x764 = 0x1ae14;
        g_pCurrentCar->scale0x768 = 0xfd70;
        g_pCurrentCar->scale0x76c = 0x428f;
        g_pCurrentCar->field_0x770[1] = 0xd70a;
        g_pCurrentCar->field_0x770[0] = 0xf581;
        break;
    }
    bVar6 = 0;
    if (RallyData_GetSelectionFlag26() != 0 && RallyDataState() == 1 && g_pCurrentCar->index != 0) {
        switch (g_pCurrentCar->type) {
        case 6:
        case 7:
        case 9:
        case 10:
        case 0xb:
        case 0xd:
            g_pCurrentCar->maxThrottleTorque = 0x570a;
            limit = 0x3e80000;
            bVar6 = 1;
            g_pCurrentCar->engineSpeedLimit = 0x98b02;
        }
    }
    if (CGameInfo::IsActiveCheatEnabled(6) != 0) {
        g_pCurrentCar->bodySize.y += g_pCurrentCar->cheatBodyLift;
        g_pCurrentCar->field_0x770[1] += g_pCurrentCar->cheatBodyLift;
        g_pCurrentCar->field_0x770[0] += g_pCurrentCar->cheatBodyLift;
    }
    g_pCurrentCar->gearSpeed[0] = 0;
    g_pCurrentCar->gearSpeed[1] = FixDiv(0xc0000, 0x240000);
    g_pCurrentCar->gearSpeed[2] = FixDiv(0x100000, 0x220000);
    g_pCurrentCar->gearSpeed[3] = FixDiv(0x120000, 0x1d0000);
    g_pCurrentCar->gearSpeed[4] = FixDiv(0x130000, 0x190000);
    g_pCurrentCar->gearSpeed[5] = FixDiv(0x110000, 0x130000);
    g_pCurrentCar->gearSpeed[6] = FixDiv(0x190000, 0x180000);
    g_pCurrentCar->gearSpeed[7] = -FixDiv(0xc0000, 0x240000);
    g_pCurrentCar->maxThrottleTorque += -0x51e;
    if (CGameInfo::IsActiveCheatEnabled(7) != 0) {
        g_pCurrentCar->maxThrottleTorque *= 2;
        g_pCurrentCar->engineSpeedLimit = FixMul(g_pCurrentCar->engineSpeedLimit, 0x14ccc);
    }
    limit += 0x780000;
    g_pCurrentCar->mass = limit;
    g_pCurrentCar->inverseMass = FixDiv(0x10000, g_pCurrentCar->mass);
    g_pCurrentCar->steeringTorqueScale = 0x1333;
    g_pCurrentCar->steeringSpeedScale = FixDiv(0x10000, 0x30000);
    g_pCurrentCar->field_0xb7c = CFrontend::GetArchiveSecondaryFlagEntry((int)g_pCurrentCar->type);
    g_pCurrentCar->field_0xb80 = CFrontend::GetArchivePrimaryFlagEntry((int)g_pCurrentCar->type);
    if (flagC != 0) {
        pSetup = RallyData_GetPlayerCarSetup(Replay_GetSelectionStateByte());
        goto LAB_0043d703;
    }
    if (RallyData_GetFlag24() != 0) {
        pSetup = RallyData_GetChallengeCarSetup();
        goto LAB_0043d703;
    }
    if (CGameInfo::GetConfiguredGameMode() == 4 ||
        (RallyData_GetFlag25() != 0 && CGameInfo::GetGameModeOptionBit19() == 0 &&
         RallyData_GetUsableRecordCategory((BYTE)g_pCurrentCar->index) != -1) ||
        (RallyData_GetFlag25() != 0 && flag8 != 0 && CGameInfo::GetGameModeOptionBit19() == 0)) {
LAB_0043d6fe:
        pSetup = RallyData_GetAlternateCountryCarSetup();
        goto LAB_0043d703;
    }
    if (!(CGameInfo::IsConfiguredMultiplayer() == 0 || param_5 != 1 || RallyData_GetFlag25() == 0)) {
        if (flag8 != 0) {
LAB_0043d6f7:
            pSetup = RallyData_GetCountryCarSetup();
            goto LAB_0043d703;
        }
        if (CGameInfo::GetGameModeOptionBit19() == 0)
            goto LAB_0043d6fe;
    } else {
        if (flag8 != 0)
            goto LAB_0043d6f7;
    }
    pSetup = RallyData_GetPlayerCarSetup((StageUI_GetRaceEndEventCount() & 0xff) + (int)g_pCurrentCar->index);
LAB_0043d703:
    Car_SetSuspension(FixMul((int)pSetup->suspension << 0x10, 0x28f));
    if (pSetup->brakeBias == 50) {
        i = 0x8000;
    } else {
        i = FixMul((int)pSetup->brakeBias << 0x10, 0x28f);
    }
    Car_SetBrakeBias(i);
    if (g_pCurrentCar->field_0xb80 != 0) {
        if (pSetup->powerRatio == 50) {
            i = 0x8000;
        } else {
            i = FixMul((int)pSetup->powerRatio << 0x10, 0x83) + 0x6666;
        }
        if (g_pCurrentCar->type == 9)
            i += -0x2666;
        Car_SetDriveSplit(i);
    } else {
        switch (g_pCurrentCar->type) {
        case 8:
            Car_SetDriveSplit(0x10000);
            break;
        case 7:
        case 0xd:
            Car_SetDriveSplit(0);
            break;
        case 10:
            Car_SetDriveSplit(0x1999);
            break;
        default:
            Car_SetDriveSplit(0x8000);
        }
    }
    Car_SetBrakePower(FixMul((int)pSetup->brakePower << 0x10, 0x28f));
    Car_SetTyreType(pSetup->tyres);
    Car_SelectGearSpeedTable((unsigned int)pSetup->gearbox);
    Car_SetSteeringSensitivity(FixMul((int)pSetup->steering << 0x10, 0x28f));
    if (bVar6)
        g_pCurrentCar->driveSplit = 0x8000;
    g_pCurrentCar->engineDragCoefficient = 0x83;
    g_pCurrentCar->throttleRampStep = 0xccc;
    g_pCurrentCar->field_0x798 = FixDiv(0x10000, g_pCurrentCar->engineSpeedLimit);
    g_pCurrentCar->brakeRampStep = 0xccc;
    g_pCurrentCar->handbrakeRampStep = 0x1999;
    g_pCurrentCar->maxHandbrakeForce = 0x8000;
    g_pCurrentCar->flags = 3;
    g_pCurrentCar->maxSteeringAngleDegrees = 0x2aa;
    g_pCurrentCar->steeringReturnRate = 0x3333;
    g_pCurrentCar->gearRatio[0] = 0;
    g_pCurrentCar->gearRatio[1] = FixDiv(0x10000, g_pCurrentCar->gearSpeed[1]);
    g_pCurrentCar->gearRatio[2] = FixDiv(0x10000, g_pCurrentCar->gearSpeed[2]);
    g_pCurrentCar->gearRatio[3] = FixDiv(0x10000, g_pCurrentCar->gearSpeed[3]);
    g_pCurrentCar->gearRatio[4] = FixDiv(0x10000, g_pCurrentCar->gearSpeed[4]);
    g_pCurrentCar->gearRatio[5] = FixDiv(0x10000, g_pCurrentCar->gearSpeed[5]);
    g_pCurrentCar->gearRatio[6] = FixDiv(0x10000, g_pCurrentCar->gearSpeed[6]);
    g_pCurrentCar->gearRatio[7] = FixDiv(0x10000, g_pCurrentCar->gearSpeed[7]);
    g_pCurrentCar->baseDriveSplit = g_pCurrentCar->driveSplit;
    FixVecScale(&v, &g_pCurrentCar->bodySize, 0x8000);
    g_pCurrentCar->collisionRadius = FixVecLength(&v);
    g_pCurrentCar->inertia.x = 1;
    g_pCurrentCar->inertia.y = 2;
    g_pCurrentCar->inertia.z = 1;
    hx = g_pCurrentCar->bodySize.x / 2;
    hy = g_pCurrentCar->bodySize.y / 2;
    hz = g_pCurrentCar->bodySize.z / 2;
    g_pCurrentCar->halfExtents.x = hx;
    g_pCurrentCar->halfExtents.y = hy;
    g_pCurrentCar->halfExtents.z = hz;
    g_pCurrentCar->wheelPos[1].x = hx;
    g_pCurrentCar->wheelPos[1].y = -hy;
    g_pCurrentCar->wheelPos[1].z = -hz;
    g_pCurrentCar->wheelPos[0].x = hx;
    g_pCurrentCar->wheelPos[0].y = -hy;
    g_pCurrentCar->wheelPos[0].z = hz;
    g_pCurrentCar->wheelPos[2].x = -hx;
    g_pCurrentCar->wheelPos[2].y = -hy;
    g_pCurrentCar->wheelPos[2].z = hz;
    g_pCurrentCar->wheelPos[3].x = -hx;
    g_pCurrentCar->wheelPos[3].y = -hy;
    g_pCurrentCar->wheelPos[3].z = -hz;
    g_pCurrentCar->upperCornersLocal[1].x = hx;
    g_pCurrentCar->upperCornersLocal[1].y = hy;
    g_pCurrentCar->upperCornersLocal[1].z = -hz;
    g_pCurrentCar->upperCornersLocal[0].x = hx;
    g_pCurrentCar->upperCornersLocal[0].y = hy;
    g_pCurrentCar->upperCornersLocal[0].z = hz;
    g_pCurrentCar->upperCornersLocal[2].x = -hx;
    g_pCurrentCar->upperCornersLocal[2].y = hy;
    g_pCurrentCar->upperCornersLocal[2].z = hz;
    g_pCurrentCar->upperCornersLocal[3].x = -hx;
    g_pCurrentCar->upperCornersLocal[3].y = hy;
    g_pCurrentCar->upperCornersLocal[3].z = -hz;
    // Leftover of an extreme-corner search: the loop and the copies are no-ops,
    // but the original kept them (the indices only fold after the copy check).
    for (i = 0; i < (int)sizeof(g_pCurrentCar->collisionCornersLocal); i += sizeof(FixVector))
        g_pCurrentCar->collisionCornersLocal[i / sizeof(FixVector)].x = g_pCurrentCar->collisionCornersLocal[i / sizeof(FixVector)].x;
    i = 1;
    g_pCurrentCar->wheelPos[1].x = g_pCurrentCar->wheelPos[i].x;
    g_pCurrentCar->wheelPos[0].x = g_pCurrentCar->wheelPos[i - 1].x;
    g_pCurrentCar->wheelPos[0].z = g_pCurrentCar->wheelPos[i - 1].z;
    g_pCurrentCar->wheelPos[2].z = g_pCurrentCar->wheelPos[1 + i].z;
    Car_UpdateCorners(g_pCurrentCar);
    g_gravityDir.x = 0;
    g_gravityDir.y = -0x8000;
    g_gravityDir.z = 0;
    g_pCurrentCar->groundRightDot = 0;
    g_pCurrentCar->groundUpDot = 0x10000;
    g_pCurrentCar->groundForwardDot = 0;
    if (flagC != 0) {
        if (RallyData_GetDriverAutomaticGearbox((BYTE)Replay_GetSelectionStateByte()) != 0)
            g_pCurrentCar->field_0xb48 = 2;
        else
            g_pCurrentCar->field_0xb48 = 1;
    } else if (flag8 != 0) {
        g_pCurrentCar->field_0xb48 = 2;
    } else if ((int)g_pCurrentCar->index < (int)(unsigned int)RallyDataState()) {
        if (RallyData_GetDriverAutomaticGearbox((BYTE)(StageUI_GetRaceEndEventCount() + g_pCurrentCar->index)) != 0)
            g_pCurrentCar->field_0xb48 = 2;
        else
            g_pCurrentCar->field_0xb48 = 1;
    } else {
        g_pCurrentCar->field_0xb48 = 2;
    }
    g_pCurrentCar->field_0xb50 = 1;
    if (CGameInfo::GetConfiguredGameMode() == 5 || CGameInfo::GetConfiguredGameMode() == 6 ||
        CGameInfo::GetConfiguredGameMode() == 7 ||
        (CGameInfo::GetConfiguredGameMode() == 4 && g_pCurrentCar->index != 0) ||
        (RallyData_GetFlag25() != 0 && CGameInfo::GetGameModeOptionBit19() == 0 &&
         RallyData_GetUsableRecordCategory((BYTE)g_pCurrentCar->index) != -1) ||
        (RallyData_GetSelectionFlag28() != 0 && CGameInfo::GetSoundOptionBit30() != 0) ||
        CGameInfo::GetGameInfoSessionFlag() != 0 || CGameInfo::GetConfiguredGameMode() == 0xb ||
        CGameInfo::GetConfiguredGameMode() == 0xc) {
        g_pCurrentCar->field_0xb50 = 0;
    }
    if (CGameInfo::IsConfiguredMultiplayer() != 0 && param_5 == 1 && RallyData_GetFlag25() != 0)
        g_pCurrentCar->field_0xb50 = 0;
    if (CGameInfo::GetConfiguredGameMode() == 4)
        g_pCurrentCar->field_0xb50 = 0;
    g_pCurrentCar->engineRestartPending = 0;
    g_pCurrentCar->engineStartTimer = 0;
    g_pCurrentCar->gearAtOrBelowBest = 1;
    g_pCurrentCar->field_0xb44 = (char)0xff;
    g_gravityScale = 0x9999;
    g_pCurrentCar->baseThrottleTorque = g_pCurrentCar->maxThrottleTorque;
    g_pCurrentCar->simulationRateHz = 25.0f;
    if (CGameInfo::GetGameModeOptionBit19() != 0 && g_pCurrentCar->index > 0) {
        g_pCurrentCar->field_0xc18 = 1;
        g_pCurrentCar->field_0xc1c = 1;
    }


}

int ChaseCamera_GetLag(BYTE index);
void FixMatrix_RebuildBasis(FixMatrix *pOut);

// Places one car's view-camera basis for the given view slot: copies the car
// rotation, applies the suspension up-offset when the option is on and, for
// non-zero view records, rotates the basis by the tilt angle before blending.
// match 26%: reviewed (W172) - calls and constants match; the diff is register
// allocation and stack frame size (original 0xf0 vs ours 0xdc).
// match 26%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00423b20
void View_PlaceCarCamera(BYTE param_1)
{
    FixMatrix src;
    FixMatrix basis;
    FixMatrix rot;
    int scale;
    BYTE index;
    unsigned short angle;
    FixMatrix *pView;
    FixVector up;
    FixVector pos;
    int blend;

    index = param_1 & 0xff;
    FixMatrix_CopyRotationFrom(&g_unk0x00538ca0[index], Car_Get(index)->pWorld);
    pView = &g_unk0x00538ca0[index];
    if (CGameInfo::IsActiveCheatEnabled(6) != 0) {
        FixMatrix_GetUp(&up, Car_Get(index)->pWorld);
        scale = FixMul(Car_Get(index)->cheatBodyLift, 0x8000);
        FixVecScale(&up, &up, scale);
        FixMatrix_GetPosition(&pos, Car_Get(index)->pWorld);
        pos.x += up.x;
        pos.y += up.y;
        pos.z += up.z;
        FixMatrix_SetPosition(&pos, pView);
    }
    if (g_unk0x00538f00[index] != 0) {
        angle = (short)FTOI(__int64, X87S_MUL((double)FixMul(
                    g_sinTable[g_unk0x00538df8[index] & 0xfff],
                    FixDiv(ChaseCamera_GetLag(param_1) - 0x80000, 0x100000) * 0x5a + 0x140000), g_fixedDegreesToAngle12));
        FixMatrix_CopyRotationFrom(&src, Car_Get(index)->pBodyMatrix);
        basis = *pView;
        FixMatrix_RebuildBasis(&basis);
        rot.right.x = g_sinTable[(angle + 0x400) & 0xfff];
        rot.right.y = 0;
        rot.up.x = 0;
        rot.up.y = 0x10000;
        rot.right.z = -g_sinTable[angle & 0xfff];
        rot.up.z = 0;
        rot.forward.x = g_sinTable[angle & 0xfff];
        rot.forward.y = 0;
        rot.forward.z = g_sinTable[(angle + 0x400) & 0xfff];
        rot.position.x = 0;
        rot.position.y = 0;
        rot.position.z = 0;
        rot.pw = 0x10000;
        rot.rw = 0;
        rot.uw = 0;
        rot.fw = 0;
        FixMatrix_Multiply(&basis, &rot, &basis);
        blend = g_unk0x00538c98[index];
        FixMatrix_Interpolate(pView, &src, &basis, blend, blend, blend, 0);
    }
}

void Car_StepGroundContact(void);
void Car_UpdateGroundContact(void);

// One step of the car preparation for every car of the list: points the
// current car at each entry, updates its setup record and wheel travel,
// resolves the body state (4- or 8-corner mode), stores the body matrix and
// recomputes its lowest corner clearance, which becomes the sink depth of the
// step.
// FUNCTION: CMR2 0x0042cd00
void Car_PrepareBodies(Car *base, short *pList, short count)
{
    int i;
    int j;
    int v;

    for (i = count - 1; i >= 0; i--) {
        g_pCurrentCar = base + pList[i];
        g_physicsPartSet = CarPart_GetSet((int)g_pCurrentCar->index);
        Car_UpdateWheelTravel();
        g_pCurrentCar->previousWheelSurface[0] = g_pCurrentCar->wheelSurface[0];
        g_pCurrentCar->previousWheelSurface[1] = g_pCurrentCar->wheelSurface[1];
        g_pCurrentCar->previousWheelSurface[2] = g_pCurrentCar->wheelSurface[2];
        g_pCurrentCar->previousWheelSurface[3] = g_pCurrentCar->wheelSurface[3];
        if (g_pCurrentCar->useUpperCollisionCorners != 0)
            Car_UpdateGroundContact();
        else
            Car_StepGroundContact();
        Car_StoreBodyMatrix();
        Car_StartResetFade();
        if (g_pCurrentCar->field_0xa80 > FixMul(0xc0000, g_physicsScale))
            g_pCurrentCar->field_0xbf8 = 1;
        g_pCurrentCar->field_0x960 = 0x3e80000;
        for (j = 0; j < 8; j++) {
            v = g_pCurrentCar->corners[j].y - g_pCurrentCar->cornerHeight[j];
            if (v < g_pCurrentCar->field_0x960)
                g_pCurrentCar->field_0x960 = v;
        }
        g_pCurrentCar->field_0x960 += g_pCurrentCar->groundHeightCorrection;
    }
}

// --- 0x0042ce60 (layer 0) ----------------------------------------------------
// Per-step ground-contact update of the current car. Publishes the car to the
// renderer, then either integrates the change of the ground normal into the
// body torque (field_0x5d0) when the car is moving fast along the normal, or
// rebuilds the body basis from the ground normal so the car keeps standing on
// its wheels. The body matrix is finally written with the optional tip
// rotation (tipAngle) and the sink depth (groundHeightCorrection) is recomputed from the
// first four box corners.
// FUNCTION: CMR2 0x0042ce60
void Car_StepGroundContact(void)
{
    // Reuse the saved normal for the final rotated axis after contact work.
    FixVector prevNormal;
    struct {
        FixVector vOut;
        FixVector worldUp;
        FixVector delta;
        FixVector tv;
    } vectors;
    FixMatrix rotMat;
    int groundSpeed;
    int t;
    int i;
    char gotNormal;

    g_pCurrentCar->normal0x498 = g_pCurrentCar->groundNormal;
    for (i = 0; i < 8; i++)
        g_pCurrentCar->cornerNormal[i] = g_pCurrentCar->cornerAxis[i];
    g_pCurrentCar->field_0xba0[2] = 0;
    g_pCurrentCar->field_0xba0[1] = 0;
    g_pCurrentCar->field_0xba0[0] = 0;
    StageObject_UpdateCarCornerGroundHeights(g_pCurrentCar, 4);
    gotNormal = 0;
    if (g_pCurrentCar->field_0xb35[0] == 1)
        Car_FlagAirborneCorners();

    if (g_pCurrentCar->field_0xb35[0] == 0) {
        prevNormal = g_pCurrentCar->groundNormal;
        Car_UpdateGroundNormal();
        groundSpeed = FixVecDot(&g_pCurrentCar->groundNormal, &g_pCurrentCar->velocity);

        t = g_pCurrentCar->speed;
        if (t > 0x1570a)
            t = 0x1570a;
        t = FixMul(t, 0x8000);
        if (t > 0x10000)
            t = 0x10000;

        if (groundSpeed <= FixMul(0x10000 - t, 0x1999)) {
            // Slow: re-orthogonalise the body axes against the ground normal.
            t = FixVecDot(&g_pCurrentCar->up, &g_pCurrentCar->groundNormal);
            if (t < 0xb334) {
                g_pCurrentCar->useUpperCollisionCorners = 1;
                g_pCurrentCar->field_0x96c = 0x10000;
                g_pCurrentCar->field_0xc04[0] = 1;
            } else if (g_pCurrentCar->field_0xb60 == 0 || t < 0xfd70) {
                if (!(g_pCurrentCar->field_0xb2a == 0)) {
                    g_pCurrentCar->up = g_pCurrentCar->groundNormal;
                } else {
                    vectors.delta.x = g_pCurrentCar->groundNormal.x - g_pCurrentCar->up.x;
                    vectors.delta.y = g_pCurrentCar->groundNormal.y - g_pCurrentCar->up.y;
                    vectors.delta.z = g_pCurrentCar->groundNormal.z - g_pCurrentCar->up.z;
                    FixVecScale(&vectors.delta, &vectors.delta, 0x8000);
                    vectors.delta.x += g_pCurrentCar->up.x;
                    vectors.delta.y += g_pCurrentCar->up.y;
                    vectors.delta.z += g_pCurrentCar->up.z;
                    FIX_NORMALIZE_INTO(g_pCurrentCar->up, vectors.delta)
                }
                gotNormal = 1;
                t = FixVecDot(&g_pCurrentCar->up, &g_pCurrentCar->right);
                FixVecScale(&vectors.delta, &g_pCurrentCar->up, t);
                vectors.delta.x = g_pCurrentCar->right.x - vectors.delta.x;
                vectors.delta.y = g_pCurrentCar->right.y - vectors.delta.y;
                vectors.delta.z = g_pCurrentCar->right.z - vectors.delta.z;
                FIX_NORMALIZE_INTO(g_pCurrentCar->right, vectors.delta)
                FixVecCross(&vectors.delta, &g_pCurrentCar->right, &g_pCurrentCar->up);
                FIX_NORMALIZE_INTO(g_pCurrentCar->forward, vectors.delta)
            } else {
                memcpy(g_pCurrentCar->bodyAxes, g_pCurrentCar->targetAxes, sizeof(g_pCurrentCar->bodyAxes));
            }
            g_pCurrentCar->groundRightDot = FixVecDot(&g_pCurrentCar->right, &g_pCurrentCar->groundNormal);
            g_pCurrentCar->groundUpDot = FixVecDot(&g_pCurrentCar->up, &g_pCurrentCar->groundNormal);
            g_pCurrentCar->groundForwardDot = FixVecDot(&g_pCurrentCar->forward, &g_pCurrentCar->groundNormal);
            g_pCurrentCar->pWorld->right = g_pCurrentCar->right;
            g_pCurrentCar->pWorld->up = g_pCurrentCar->up;
            g_pCurrentCar->pWorld->forward = g_pCurrentCar->forward;
            g_pCurrentCar->pWorld->position = g_pCurrentCar->position;
            Car_UpdateCorners(g_pCurrentCar);
            if (!(g_pCurrentCar->useUpperCollisionCorners == 0))
                StageObject_UpdateCarCornerGroundHeights(g_pCurrentCar, 8);
            else
                StageObject_UpdateCarCornerGroundHeights(g_pCurrentCar, 4);
        } else {
            // Fast along the normal: turn the change of the ground normal
            // into a body torque perpendicular to it.
            vectors.delta.x = g_pCurrentCar->groundNormal.x - prevNormal.x;
            vectors.delta.y = g_pCurrentCar->groundNormal.y - prevNormal.y;
            vectors.delta.z = g_pCurrentCar->groundNormal.z - prevNormal.z;
            t = FixVecLength(&vectors.delta);
            if (t > 0) {
                FixVecScaleRecip(&vectors.delta, &vectors.delta, t);
                vectors.worldUp.x = 0;
                vectors.worldUp.y = 0x10000;
                vectors.worldUp.z = 0;
                FixMatrix_InverseRotateVector(&vectors.vOut, &vectors.delta, g_pCurrentCar->pWorld);
                FixVecCross(&vectors.tv, &vectors.vOut, &vectors.worldUp);
                vectors.tv.x = FixMul(vectors.tv.x, 0x14ccc);
                vectors.tv.z = FixMul(vectors.tv.z, 0xe666);
                vectors.tv.y = 0;
                g_pCurrentCar->field_0x5d0.x += vectors.tv.x;
                g_pCurrentCar->field_0x5d0.y += vectors.tv.y;
                g_pCurrentCar->field_0x5d0.z += vectors.tv.z;
            }
            g_pCurrentCar->groundNormal = prevNormal;
        }
        Car_LiftFreeCorners();
        Car_FlagAirborneCorners();
        if (g_pCurrentCar->field_0xb35[0] == 0) {
            t = FixMul(0x1eb8, g_physicsTimeStep);
            if (groundSpeed < 0) {
                FixVecScale(&vectors.delta, &g_pCurrentCar->groundNormal, groundSpeed);
                g_pCurrentCar->velocity.x -= vectors.delta.x;
                g_pCurrentCar->velocity.y -= vectors.delta.y;
                g_pCurrentCar->velocity.z -= vectors.delta.z;
            }
            g_pCurrentCar->angularVelocity.x = FixMul(g_pCurrentCar->angularVelocity.x, t);
            g_pCurrentCar->angularVelocity.z = FixMul(g_pCurrentCar->angularVelocity.z, t);
        }
    }

    Car_IntegrateWheelTravel();
    g_pCurrentCar->field_0xb2a = gotNormal;
    if (g_pCurrentCar->tipAngle != 0) {
        g_pCurrentCar->pWorld->right = g_pCurrentCar->right;
        FixMatrix_FromAxisAngle(&rotMat, &g_pCurrentCar->right, g_pCurrentCar->tipAngle);
        FixMatrix_RotateVector(&prevNormal, &g_pCurrentCar->up, &rotMat);
        FIX_NORMALIZE_INTO(prevNormal, prevNormal)
        g_pCurrentCar->pWorld->up = prevNormal;
        FixMatrix_RotateVector(&prevNormal, &g_pCurrentCar->forward, &rotMat);
        FIX_NORMALIZE_INTO(prevNormal, prevNormal)
        g_pCurrentCar->pWorld->forward = prevNormal;
    } else {
        g_pCurrentCar->pWorld->right = g_pCurrentCar->right;
        g_pCurrentCar->pWorld->up = g_pCurrentCar->up;
        g_pCurrentCar->pWorld->forward = g_pCurrentCar->forward;
    }
    g_pCurrentCar->pWorld->position = g_pCurrentCar->position;

    g_pCurrentCar->groundHeightCorrection = 0x1999;
    for (i = 0; i < 4; i++) {
        int d = g_pCurrentCar->corners[i].y - g_pCurrentCar->cornerHeight[i];
        if (g_pCurrentCar->cornerFlags[i] == 0 && d < g_pCurrentCar->groundHeightCorrection)
            g_pCurrentCar->groundHeightCorrection = d > 0 ? d : 0;
    }
    g_pCurrentCar->groundHeightCorrection = -g_pCurrentCar->groundHeightCorrection;
}

// --- 0x0043f630 (layer 0) ----------------------------------------------------
void AutoGear_UpdateCarGearState(Car *pCar);
void Surface_BlendWheelContactParameters(Car *pWheel, int unused);
short Sector_GetNeighbours(FixVector *pPos, short *pOut);

// Race context handed to the per-car step chain: the car buffer, the car order
// (car indices) and how many cars it holds.
// GLOBAL: CMR2 0x0053cc14
Car *g_stepCars;
// GLOBAL: CMR2 0x0053c9b0
short *g_stepCarOrder;
// GLOBAL: CMR2 0x0053ca28
short g_stepCarCount;

// One step of the race's per-car chain. Runs nine passes over the cars of the
// order array (in reverse), so every car finishes a given stage of the step
// before any car starts the next one: body-frame refresh, physics, corner
// loads, body lean, wheel forces, body axes, integration, accumulated vectors
// and finally the box-corner sector id and the previous-position copy.
// FUNCTION: CMR2 0x0043f630
void Car_RunStepPasses(Car *carBase, short *pOrder, short count)
{
    int i;
    int j;
    short nb[3];

    g_stepCars = carBase;
    g_stepCarOrder = pOrder;
    g_stepCarCount = count;

    // Pass 1: reset the step scratch and refresh the body frame.
    for (i = count - 1; i >= 0; i--) {
        g_pCurrentCar = carBase + pOrder[i];
        g_carStepAccel.x = 0;
        g_carStepAccel.y = 0;
        g_carStepAccel.z = 0;
        g_unk0x0053c9d4 = 0;
        Car_ScaleRatesByTimeStep(g_pCurrentCar);
        g_pCurrentCar->positionPrev2 =
            g_pCurrentCar->positionPrev;
        g_pCurrentCar->positionPrev = g_pCurrentCar->position;
        memcpy(g_pCurrentCar->targetAxes, g_pCurrentCar->bodyAxes, sizeof(g_pCurrentCar->bodyAxes));
        g_pCurrentCar->field_0x968 = g_pCurrentCar->field_0x964;
        g_pCurrentCar->field_0x964 = g_pCurrentCar->field_0x960;
        Surface_BlendWheelContactParameters(g_pCurrentCar, 0);
        Car_UpdateLowSpeedWheelLoadTimer();
    }

    // Pass 2: clear the per-wheel accumulators walked over by the surface code.
    for (i = count - 1; i >= 0; i--) {
        g_pCurrentCar = carBase + pOrder[i];
        AutoGear_UpdateCarGearState(g_pCurrentCar);
        for (j = 3; j >= 0; j--) {
            g_pCurrentCar->wheelSlip[j] = 0;
            g_pCurrentCar->wheelSlipLateral[j] = 0;
        }
    }

    // Pass 3: corner loads and forces.
    for (i = count - 1; i >= 0; i--) {
        g_pCurrentCar = carBase + pOrder[i];
        g_physicsPartSet = CarPart_GetSet((int)g_pCurrentCar->index);
        Car_UpdateCornerVelocityPair();
        for (j = 7; j >= 0; j--) {
            g_pCurrentCar->cornerLoad[j].x = 0;
            g_pCurrentCar->cornerLoad[j].y = 0;
            g_pCurrentCar->cornerLoad[j].z = 0;
            g_pCurrentCar->cornerForce[j].x = 0;
            g_pCurrentCar->cornerForce[j].y = 0;
            g_pCurrentCar->cornerForce[j].z = 0;
        }
        if (g_pCurrentCar->field_0xb74 != 0)
            Car_SetupDriveTrain();
    }

    // Pass 4: body lean and steering.
    for (i = count - 1; i >= 0; i--) {
        g_pCurrentCar = carBase + pOrder[i];
        g_physicsPartSet = CarPart_GetSet((int)g_pCurrentCar->index);
        Car_UpdateBodyLean();
        if (g_pCurrentCar->field_0xb74 != 0) {
            Car_SetupSuspensionRates();
            Car_UpdateSteering();
        }
    }

    // Pass 5: wheel forces.
    for (i = count - 1; i >= 0; i--) {
        g_pCurrentCar = carBase + pOrder[i];
        g_physicsPartSet = CarPart_GetSet((int)g_pCurrentCar->index);
        if (g_pCurrentCar->field_0xb74 != 0) {
            Car_SplitEngineTorque();
            Car_UpdateWheelForces();
            Car_SmoothForceFeedback();
        }
    }

    // Pass 6: body axes without damping.
    for (i = count - 1; i >= 0; i--) {
        g_pCurrentCar = carBase + pOrder[i];
        g_physicsPartSet = CarPart_GetSet((int)g_pCurrentCar->index);
        if (g_pCurrentCar->field_0xb74 != 0)
            Car_UpdateBodyAxesNoDamping();
    }

    // Pass 7: integrate the rigid body.
    for (i = count - 1; i >= 0; i--) {
        g_pCurrentCar = carBase + pOrder[i];
        g_physicsPartSet = CarPart_GetSet((int)g_pCurrentCar->index);
        Car_Integrate();
    }

    // Pass 8: accumulate the body vectors and drop them once the step ends.
    for (i = count - 1; i >= 0; i--) {
        g_pCurrentCar = carBase + pOrder[i];
        if (g_pCurrentCar->field_0xb74 != 0) {
            Car_UpdateRideHeight();
            Car_CopySteeringFollowState();
        }
        g_pCurrentCar->field_0x5c4.x = 0;
        g_pCurrentCar->field_0x5c4.y = 0;
        g_pCurrentCar->field_0x5c4.z = 0;
        g_pCurrentCar->field_0x5d0.x = 0;
        g_pCurrentCar->field_0x5d0.y = 0;
        g_pCurrentCar->field_0x5d0.z = 0;
    }

    // Pass 9: sector of the box position and previous-position copy.
    for (i = count - 1; i >= 0; i--) {
        g_pCurrentCar = carBase + pOrder[i];
        g_pCurrentCar->sector =
            Sector_GetNeighbours(&g_pCurrentCar->position, nb);
        g_pCurrentCar->neighbours.s[0] = nb[0];
        g_pCurrentCar->neighbours.s[1] = nb[1];
        g_pCurrentCar->neighbours.s[2] = nb[2];
        g_pCurrentCar->field_0x2dc = g_pCurrentCar->position;
    }
}

void CarDamage_ApplyCollisionDeformImpulse(Car *pCar, int *param_2, FixVector *param_3, int param_4,
                  unsigned char param_5, int param_6);


// Upright solver of the current car. Derives a damping factor from the angular
// velocity magnitude (0x50000 per unit, capped at 0xcccc) and from how far the
// forward speed sits above the roll rate, then projects the velocity on the
// ground normal (0x48c) to check the three body axes for a sign flip: the dot of
// the normal with right (0x91c), up (0x920) and forward (0x924) is stored back
// and a flip makes the body lock out the corresponding angular component. Each
// locked axis keeps a counter (0xb27 roll, 0xb25 pitch, 0xb26 yaw) that grows
// with 0xa0000 * physics step while the axis stays beyond its threshold
// (0xe666 roll, 0xf333 pitch/forward) and flips the lock flag (0xbe4, 0xbdc,
// 0xbe0) after 100 frames. Timers 0xba0/0xba4/0xba8 hold the "just spun out"
// state of each axis and clear once the two remaining angular components grow
// past 0xf5c. When an axis is unlocked and unflipped, the angular velocity is
// projected off the normal in body space and damped. The three axes are then
// re-orthonormalized (right, and up/forward rebuilt from it) and, when any axis
// flipped, the deepest corner against the normal is pushed to the deformation
// solver.
// FUNCTION: CMR2 0x0042fb20
void Car_SolveUpright(void)
{
    FixVector tmp;
    FixVector corner;
    FixVector scaled;
    FixVector upBody;
    FixVector out;
    int flipRoll;
    int flipPitch;
    int flipYaw;
    int damp;
    int mag;
    int dot;
    int i;

    flipRoll = 0;
    flipPitch = 0;
    flipYaw = 0;
    dot = FixVecLength(&g_pCurrentCar->angularVelocity);
    mag = FixMul(dot, 0x50000);
    if (mag > 0xcccc)
        mag = 0xcccc;
    dot = 0x10000 - mag;
    damp = FixMul(0x8000, dot);
    dot = FIX_ABS(FixVecDot(&g_pCurrentCar->groundNormal, &g_pCurrentCar->velocity));
    if (g_pCurrentCar->speed - dot < 0x8000)
        damp = 0x8000;
    damp = FixMul(damp, g_physicsTimeStep);
    if (damp > 0x10000)
        damp = 0x10000;

    dot = FixVecDot(&g_pCurrentCar->right, &g_pCurrentCar->groundNormal);
    if ((g_pCurrentCar->groundRightDot > 0 && dot < 0) || (g_pCurrentCar->groundRightDot < 0 && dot > 0))
        flipRoll = 1;
    g_pCurrentCar->groundRightDot = dot;
    if (g_pCurrentCar->field_0xbe4 == 0) {
        if (dot >= -0xe666 && dot <= 0xe666)
            g_pCurrentCar->field_0xb27 = 0;
        else
            g_pCurrentCar->field_0xb27 += FixMul(0xa0000, g_physicsTimeStep) >> 16;
        if (g_pCurrentCar->field_0xb27 > 100)
            g_pCurrentCar->field_0xbe4 = 1;
    }

    dot = FixVecDot(&g_pCurrentCar->up, &g_pCurrentCar->groundNormal);
    if ((g_pCurrentCar->groundUpDot > 0 && dot < 0) || (g_pCurrentCar->groundUpDot < 0 && dot > 0))
        flipPitch = 1;
    g_pCurrentCar->groundUpDot = dot;
    if (g_pCurrentCar->field_0xbdc == 0) {
        if (dot < -0xf333)
            g_pCurrentCar->field_0xb25 += FixMul(0xa0000, g_physicsTimeStep) >> 16;
        else
            g_pCurrentCar->field_0xb25 = 0;
        if (g_pCurrentCar->field_0xb25 > 100)
            g_pCurrentCar->field_0xbdc = 1;
    }

    dot = FixVecDot(&g_pCurrentCar->forward, &g_pCurrentCar->groundNormal);
    if ((g_pCurrentCar->groundForwardDot > 0 && dot < 0) || (g_pCurrentCar->groundForwardDot < 0 && dot > 0))
        flipYaw = 1;
    g_pCurrentCar->groundForwardDot = dot;
    if (g_pCurrentCar->field_0xbe0 == 0) {
        if (dot >= -0xf333 && dot <= 0xf333)
            g_pCurrentCar->field_0xb26 = 0;
        else
            g_pCurrentCar->field_0xb26 += FixMul(0xa0000, g_physicsTimeStep) >> 16;
        if (g_pCurrentCar->field_0xb26 > 100)
            g_pCurrentCar->field_0xbe0 = 1;
    }

    if (g_pCurrentCar->field_0xba0[0] != 0) {
        if (FixSqrt(FixMul(g_pCurrentCar->angularVelocity.y, g_pCurrentCar->angularVelocity.y) +
                    FixMul(g_pCurrentCar->angularVelocity.z, g_pCurrentCar->angularVelocity.z)) >= 0xf5c)
            g_pCurrentCar->field_0xba0[0] = 0;
    }
    if (g_pCurrentCar->field_0xba0[1] != 0) {
        if (FixSqrt(FixMul(g_pCurrentCar->angularVelocity.x, g_pCurrentCar->angularVelocity.x) +
                    FixMul(g_pCurrentCar->angularVelocity.z, g_pCurrentCar->angularVelocity.z)) >= 0xf5c)
            g_pCurrentCar->field_0xba0[1] = 0;
    }
    if (g_pCurrentCar->field_0xba0[2] != 0) {
        if (FixSqrt(FixMul(g_pCurrentCar->angularVelocity.x, g_pCurrentCar->angularVelocity.x) +
                    FixMul(g_pCurrentCar->angularVelocity.y, g_pCurrentCar->angularVelocity.y)) >= 0xf5c)
            g_pCurrentCar->field_0xba0[2] = 0;
    }

    if (flipRoll != 0 && g_pCurrentCar->field_0xba0[0] == 0) {
        FixMatrix_InverseRotateVector(&tmp, &g_pCurrentCar->groundNormal, g_pCurrentCar->pWorld);
        dot = FixVecDot(&tmp, &g_pCurrentCar->angularVelocity);
        FixVecScale(&tmp, &tmp, dot);
        tmp.x -= g_pCurrentCar->angularVelocity.x;
        tmp.y -= g_pCurrentCar->angularVelocity.y;
        tmp.z -= g_pCurrentCar->angularVelocity.z;
        FixVecScale(&tmp, &tmp, damp);
        tmp.x = 0;
        g_pCurrentCar->angularVelocity.y += tmp.y;
        g_pCurrentCar->angularVelocity.z += tmp.z;
        if (dot < 0)
            dot = -dot;
        if (dot < 0xf5c && g_pCurrentCar->speed < 0x8000)
            g_pCurrentCar->field_0xba0[0] = 1;
    }
    if (flipPitch != 0 && g_pCurrentCar->field_0xba0[1] == 0) {
        FixMatrix_InverseRotateVector(&tmp, &g_pCurrentCar->groundNormal, g_pCurrentCar->pWorld);
        dot = FixVecDot(&tmp, &g_pCurrentCar->angularVelocity);
        FixVecScale(&tmp, &tmp, dot);
        tmp.x -= g_pCurrentCar->angularVelocity.x;
        tmp.y -= g_pCurrentCar->angularVelocity.y;
        tmp.z -= g_pCurrentCar->angularVelocity.z;
        FixVecScale(&tmp, &tmp, damp);
        tmp.y = 0;
        g_pCurrentCar->angularVelocity.x += tmp.x;
        g_pCurrentCar->angularVelocity.y += tmp.y;
        g_pCurrentCar->angularVelocity.z += tmp.z;
    }
    if (flipYaw != 0 && g_pCurrentCar->field_0xba0[2] == 0) {
        FixMatrix_InverseRotateVector(&tmp, &g_pCurrentCar->groundNormal, g_pCurrentCar->pWorld);
        dot = FixVecDot(&tmp, &g_pCurrentCar->angularVelocity);
        FixVecScale(&tmp, &tmp, dot);
        tmp.x -= g_pCurrentCar->angularVelocity.x;
        tmp.y -= g_pCurrentCar->angularVelocity.y;
        tmp.z -= g_pCurrentCar->angularVelocity.z;
        FixVecScale(&tmp, &tmp, damp);
        tmp.z = 0;
        g_pCurrentCar->angularVelocity.x += tmp.x;
        g_pCurrentCar->angularVelocity.y += tmp.y;
        g_pCurrentCar->angularVelocity.z += tmp.z;
        if (dot < 0)
            dot = -dot;
        if (dot < 0xf5c && g_pCurrentCar->speed < 0x8000)
            g_pCurrentCar->field_0xba0[2] = 1;
    }

    if (g_pCurrentCar->field_0xba0[0] != 0 || g_pCurrentCar->field_0xba0[1] != 0 || g_pCurrentCar->field_0xba0[2] != 0) {
        if (g_pCurrentCar->field_0xba0[0] != 0) {
            FixVecScale(&tmp, &g_pCurrentCar->groundNormal, g_pCurrentCar->groundRightDot);
            tmp.x = g_pCurrentCar->right.x - tmp.x;
            tmp.y = g_pCurrentCar->right.y - tmp.y;
            tmp.z = g_pCurrentCar->right.z - tmp.z;
            FIX_NORMALIZE_INTO(tmp, tmp);
            g_pCurrentCar->right = tmp;
            g_pCurrentCar->angularVelocity.y = FixMul(g_pCurrentCar->angularVelocity.y, 0xe666);
            g_pCurrentCar->angularVelocity.z = FixMul(g_pCurrentCar->angularVelocity.z, 0xe666);
        }
        if (g_pCurrentCar->field_0xba0[1] != 0) {
            FixVecScale(&tmp, &g_pCurrentCar->groundNormal, g_pCurrentCar->groundUpDot);
            tmp.x = g_pCurrentCar->up.x - tmp.x;
            tmp.y = g_pCurrentCar->up.y - tmp.y;
            tmp.z = g_pCurrentCar->up.z - tmp.z;
            FIX_NORMALIZE_INTO(tmp, tmp);
            g_pCurrentCar->up = tmp;
            g_pCurrentCar->angularVelocity.x = FixMul(g_pCurrentCar->angularVelocity.x, 0xe666);
            g_pCurrentCar->angularVelocity.z = FixMul(g_pCurrentCar->angularVelocity.z, 0xe666);
        }
        if (g_pCurrentCar->field_0xba0[2] != 0) {
            FixVecScale(&tmp, &g_pCurrentCar->groundNormal, g_pCurrentCar->groundForwardDot);
            tmp.x = g_pCurrentCar->forward.x - tmp.x;
            tmp.y = g_pCurrentCar->forward.y - tmp.y;
            tmp.z = g_pCurrentCar->forward.z - tmp.z;
            FIX_NORMALIZE_INTO(tmp, tmp);
            g_pCurrentCar->forward = tmp;
            g_pCurrentCar->angularVelocity.x = FixMul(g_pCurrentCar->angularVelocity.x, 0xe666);
            g_pCurrentCar->angularVelocity.y = FixMul(g_pCurrentCar->angularVelocity.y, 0xe666);
        }
        dot = FixVecDot(&g_pCurrentCar->right, &g_pCurrentCar->up);
        FixVecScale(&tmp, &g_pCurrentCar->right, dot);
        tmp.x = g_pCurrentCar->up.x - tmp.x;
        tmp.y = g_pCurrentCar->up.y - tmp.y;
        tmp.z = g_pCurrentCar->up.z - tmp.z;
        FixVecNormalizeLen(&g_pCurrentCar->up, &tmp);
        FixVecCross(&tmp, &g_pCurrentCar->right, &g_pCurrentCar->up);
        FixVecNormalizeLen(&g_pCurrentCar->forward, &tmp);
    }

    if (flipRoll != 0 || flipPitch != 0 || flipYaw != 0) {
        for (i = 0; i < 8; i++) {
            dot = FixVecDot(&g_pCurrentCar->cornerVelocity[i], &g_pCurrentCar->groundNormal);
            if (dot < -0x3333)
                goto found;
        }
        return;
found:
        g_pCurrentCar->field_0x5c4.x = dot;
        g_pCurrentCar->field_0x5c4.y = 0;
        g_pCurrentCar->field_0x5c4.z = 0;
        FixVecScale(&tmp, &g_pCurrentCar->groundNormal, -0x30000);
        FixMatrix_InverseRotateVector(&g_pCurrentCar->field_0x5dc, &tmp, g_pCurrentCar->pWorld);
        i = g_pCurrentCar->field_0x5dc.x;
        if (FIX_ABS(i) > g_pCurrentCar->halfExtents.x)
            g_pCurrentCar->field_0x5dc.x = i < 0 ? -g_pCurrentCar->halfExtents.x : g_pCurrentCar->halfExtents.x;
        i = g_pCurrentCar->field_0x5dc.y;
        if (FIX_ABS(i) > g_pCurrentCar->halfExtents.y)
            g_pCurrentCar->field_0x5dc.y = i < 0 ? -g_pCurrentCar->halfExtents.y : g_pCurrentCar->halfExtents.y;
        i = g_pCurrentCar->field_0x5dc.z;
        if (FIX_ABS(i) > g_pCurrentCar->halfExtents.z)
            g_pCurrentCar->field_0x5dc.z = i < 0 ? -g_pCurrentCar->halfExtents.z : g_pCurrentCar->halfExtents.z;
        corner.x = 0;
        corner.y = 0x7d000000;
        corner.z = 0;
        upBody.x = 0;
        upBody.y = 0x10000;
        upBody.z = 0;
        for (i = 0; i < 4; i++) {
            if (g_pCurrentCar->corners[i].y < corner.y)
                corner = g_pCurrentCar->corners[i];
        }
        if (g_pCurrentCar->up.y < 0) {
            FixVecScale(&scaled, &g_pCurrentCar->up, FixMul(0x20000, g_pCurrentCar->halfExtents.y));
            corner.x += scaled.x;
            corner.y += scaled.y;
            corner.z += scaled.z;
        }
        CarDamage_ApplyCollisionDeformImpulse(g_pCurrentCar, (int *)&corner, &upBody, 0, 0, 0);
        FixMatrix_RotateVector(&out, &g_pCurrentCar->field_0x5dc, g_pCurrentCar->pWorld);
    }
}


// --- per-view camera control (0x421610-0x423780) ---------------------------
// Each view has two camera records in g_viewRecords (index view * 2 + the
// active slot g_activeViewRecord[view]), laid out as CameraRecord.
// g_viewStates holds the blended state of each view in the same layout
// (type 8 while a blend between the two records runs).

#define VIEW_RECORD(i) (&g_viewRecords[i])
#define VIEW_STATE(v) (&g_viewStates[v])
#define VIEW_PREV_STATE(v) (&g_viewPreviousStates[v])
// Smooth-step weight of a camera blend timer (0xc8000 = the whole transition).
#define VIEW_EASE(t)                                                                               \
    (0x10000 - (FixCos((short)(int)FTOI(__int64, X87S_MUL((double)(FixDiv((t), 0xc8000) * 180), g_minusFixedDegreesToAngle12))) + \
                0x10000) / 2)

extern double g_minusFixedDegreesToAngle12;
extern double g_fixedDegreesToAngle12;
void Camera_BeginLeave(CameraRecord *pRecord, CameraRecord *pOther);
void Camera_BeginEnter(CameraRecord *pRecord, CameraRecord *pOther);
void Camera_EndLeave(CameraRecord *pRecord, CameraRecord *pOther);
void Camera_EndEnter(CameraRecord *pRecord, CameraRecord *pOther);
void Camera_AfterLeave(CameraRecord *pRecord);
void Glow_NoOpEntryCallback(BYTE a, BYTE b, int c, int d);
void StageObject_SelectAndCopyCarNodePayload(CameraRecord *pObj, int *pSrc, BYTE index, BYTE value);
void StageObject_SyncStateAndSceneMatrix(CameraRecord *p, int *src, int unused, BYTE value);
void ChaseCamera_Start(CameraRecord *param_1, FixMatrix *param_2, int param_3);
void View_PlaceTracksideCameraAtSpot(CameraRecord *pRecord, FixMatrix *pRef, int param);
void StageObject_RebuildMirroredTiltMatrix(CameraRecord *pObj, int *pSrc);
void StageObject_BuildCarNodeOrientation(CameraRecord *object, int *src);
void ChaseCamera_Restart(CameraRecord *pObj, FixMatrix *pRef);
void View_RestartTracksideCameraDolly(CameraRecord *pRecord, FixMatrix *pRef);
void View_PositionFromCarProfile(CameraRecord *pObj, FixMatrix *pRef);
void StageObject_BuildCarMountWorldMatrix(CameraRecord *object, void *unused);
void ChaseCamera_BuildMatrix(CameraRecord *pObj, FixMatrix *pRef);
void View_BuildTracksideCameraMatrix(CameraRecord *pRecord, FixMatrix *pRef);
void StageObject_InterpolateReferenceMatrix(CameraRecord *pObj, int *pSrc, int param_3);
void StageObject_DispatchActiveCarObjectUpdate(CameraRecord *pObj, FixMatrix *pRef, int b);
void ChaseCamera_Update(CameraRecord *param_1, FixMatrix *param_2, int param_3);
void View_UpdateTracksideZoomAndShake(CameraRecord *pRecord, FixMatrix *pRef);
int Track_GetGroundHeight5(FixVector *pPoint, FixVector *pNormal, short *pTri, short *pSurfaceClass, int defaultY);
void StageObject_LoadAndAttachCarInterior(BYTE record, BYTE car);
unsigned short RallyData_GetPrimaryStageScoreScale(void);
unsigned short RallyData_GetSecondaryStageScoreScale(void);
void StageObject_InitCarSceneTables(void);
void CarInfo_CacheCameraOffsets(void);
void StageTiming_SetViewRouteDistanceLimit(BYTE player, unsigned int node, int dir);
void View_ResetShownFarPlane(BYTE view);
SceneNode *RallyData_GetChallengeRenderState(void);
void View_PlaceCarCamera(BYTE param_1);
int View_IsModeAvailable(BYTE index, int mode);
FixMatrix *View_GetCarBodyMatrix(FixMatrix *pOut, BYTE car);
void FixMatrix_GetPosition(FixVector *pOut, FixMatrix *pM);

// Starts the camera of camera type `type` on a view record.
// FUNCTION: CMR2 0x00423300
void Camera_Start(CameraRecord *pRecord, int type, int param)
{
    BYTE car;
    BYTE view;
    Car *pCar;
    FixMatrix body;

    view = pRecord->view;
    car = pRecord->car;
    pCar = Car_Get(car);
    View_GetCarBodyMatrix(&body, car);
    switch (type) {
    case 6:
        ChaseCamera_Start(pRecord, Car_GetCameraReferenceMatrix(car), 3);
        return;
    case 4:
        ChaseCamera_Start(pRecord, Car_GetCameraReferenceMatrix(car), 0);
        return;
    case 5:
        ChaseCamera_Start(pRecord, Car_GetCameraReferenceMatrix(car), 4);
        return;
    case 3:
        StageObject_SyncStateAndSceneMatrix(pRecord, (int *)Car_GetCameraReferenceMatrix(car), View_GetActiveCameraMode(view), (BYTE)pCar->type);
        return;
    case 2:
        StageObject_SelectAndCopyCarNodePayload(pRecord, (int *)&body, *(BYTE *)&pCar->index, 0);
        return;
    case 10:
        StageObject_SelectAndCopyCarNodePayload(pRecord, (int *)&body, *(BYTE *)&pCar->index, 2);
        return;
    case 1:
        StageObject_SelectAndCopyCarNodePayload(pRecord, (int *)&body, *(BYTE *)&pCar->index, 1);
        return;
    case 7:
        View_PlaceTracksideCameraAtSpot(pRecord, Car_GetCameraReferenceMatrix(car), param);
    }
}

// Per-frame update of a view record's camera, plus its ground clearance
// (how far the camera sits above the stage, eased towards the new value).
// match 48%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00423460
void Camera_Update(CameraRecord *pRecord)
{
    FixMatrix body;
    FixVector position;
    FixVector normal;
    Car *pCar;
    BYTE car;
    int onCar;
    int clearance;
    int check;
    short tri;
    short surface;

    onCar = 0;
    if (pRecord->carLift != 0 || pRecord->lookDistance != 0)
        check = 1;
    else
        check = 0;
    car = pRecord->car;
    pCar = Car_Get(car);
    if (pCar->field_0xc04[0] != 0 && g_unk0x00538f00[pRecord->car] == 0)
        onCar = 1;
    View_GetCarBodyMatrix(&body, car);
    switch (pRecord->type) {
    case 4:
    case 5:
    case 6:
        ChaseCamera_Update(pRecord, Car_GetCameraReferenceMatrix(car), onCar);
        break;
    case 3:
        StageObject_DispatchActiveCarObjectUpdate(pRecord, Car_GetCameraReferenceMatrix(car),
                                                  (onCar == 0 && pCar->field_0xb60 == 0) ? 0 : 1);
        break;
    case 2:
        StageObject_InterpolateReferenceMatrix(pRecord, (int *)&body, (onCar == 0 && pCar->field_0xb60 == 0) ? 0 : 1);
        break;
    case 10:
        StageObject_InterpolateReferenceMatrix(pRecord, (int *)&body, (onCar == 0 && pCar->field_0xb60 == 0) ? 0 : 1);
        break;
    case 1:
        StageObject_InterpolateReferenceMatrix(pRecord, (int *)&body, (onCar == 0 && pCar->field_0xb60 == 0) ? 0 : 1);
        break;
    case 7:
        View_UpdateTracksideZoomAndShake(pRecord, Car_GetCameraReferenceMatrix(car));
        break;
    }
    // Operator camera constrains its own world position, including replay.
    if(CameraOperator_Enabled(pRecord)){pRecord->clearance=0;return;}
    if (View_IsPlaybackCameraActive() == 0 && check != 0) {
        FixMatrix_GetPosition(&position, &pRecord->matrix);
        tri = 0;
        clearance = Track_GetGroundHeight5(&position, &normal, &tri, &surface,
                                           Car_Get(pRecord->car)->cornerHeight[2]) -
                    position.y + 0x10000;
        if (clearance < 0)
            clearance = 0;
        if (clearance < pRecord->clearance)
            clearance += FixMul(pRecord->clearance - clearance, 0x10000 - FixMul(0xccc, g_physicsTimeStep));
        pRecord->clearance = clearance;
        return;
    }
    pRecord->clearance = 0;
}

// Restarts the camera of a view record (after a car reset).
// FUNCTION: CMR2 0x004236b0
void Camera_Restart(CameraRecord *pRecord)
{
    FixMatrix body;
    BYTE car;

    car = pRecord->car;
    View_GetCarBodyMatrix(&body, car);
    switch (pRecord->type) {
    case 4:
    case 5:
    case 6:
        ChaseCamera_Restart(pRecord, Car_GetCameraReferenceMatrix(car));
        return;
    case 3:
        StageObject_BuildCarNodeOrientation(pRecord, (int *)Car_GetCameraReferenceMatrix(car));
        return;
    case 2:
        StageObject_RebuildMirroredTiltMatrix(pRecord, (int *)&body);
        return;
    case 10:
        StageObject_RebuildMirroredTiltMatrix(pRecord, (int *)&body);
        return;
    case 1:
        StageObject_RebuildMirroredTiltMatrix(pRecord, (int *)&body);
        return;
    case 7:
        View_RestartTracksideCameraDolly(pRecord, Car_GetCameraReferenceMatrix(car));
    }
}

// Snaps the camera of a view record to a reference matrix.
// FUNCTION: CMR2 0x00423780
void Camera_SnapTo(CameraRecord *pRecord, FixMatrix *pRef)
{
    switch (pRecord->type) {
    case 5:
    case 6:
        ChaseCamera_BuildMatrix(pRecord, pRef);
        return;
    case 4:
        ChaseCamera_BuildMatrix(pRecord, pRef);
        return;
    case 3:
        StageObject_BuildCarMountWorldMatrix(pRecord, pRef);
        return;
    case 10:
        View_PositionFromCarProfile(pRecord, pRef);
        return;
    case 1:
    case 2:
        View_PositionFromCarProfile(pRecord, pRef);
        return;
    case 7:
        View_BuildTracksideCameraMatrix(pRecord, pRef);
    }
}

// Sets up the view cameras of a stage: the four camera records, then for both
// views a camera scene node and a cleared view state.
// FUNCTION: CMR2 0x00421610
void View_SetupCameras(void)
{
    FixVector position;
    FixAngles angles;
    BYTE i;

    if ((unsigned short)RallyData_GetPrimaryStageScoreScale() < 0x834)
        RallyData_GetSecondaryStageScoreScale();
    CGame::RegisterCallback(View_ReleaseNodes, NULL);
    StageObject_InitCarSceneTables();
    CarInfo_CacheCameraOffsets();
    for (i = 0; i < 4; i++) {
        g_viewRecords[i].index = i;
        g_viewRecords[i].view = i >> 1;
        g_viewRecords[i].car = i >> 1;
        g_viewRecords[i].clearance = 0;
        StageObject_LoadAndAttachCarInterior(i, g_viewRecords[i].car);
    }
    position.x = 0;
    position.y = 0;
    position.z = 0;
    angles.x = 0;
    angles.y = 0;
    angles.z = 0;
    i = 0;
    for (i = 0; i < 2; i++) {
        g_viewNodes[i] = SceneType2_Create(&position, &angles, NULL, RallyData_GetChallengeRenderState());
        g_viewStates[i].type = 0;
        g_unk0x00538d20[i] = -0x10000;
        g_activeViewRecord[i] = 0;
        g_unk0x005391b0[i] = 0;
        g_unk0x005391c4[i] = 0;
        g_unk0x005391a8[i] = 0;
        StageTiming_SetViewRouteDistanceLimit(0, 0, 1);
        // PORT: the far plane starts at the stage's distance (FixedPoint.cpp).
        View_ResetShownFarPlane((BYTE)i);
        g_unk0x00538f00[i] = 0;
        ChaseCamera_Reset(i);
    }
}

// Switches a view to camera type `type`: the inactive record takes the new
// camera, and either the view blends into it (timed transition) or swaps to
// it at once.
// FUNCTION: CMR2 0x00421720
void View_SwitchCamera(BYTE view, int type, int param, BYTE target, int blend)
{
    int ok;
    BYTE active;
    BYTE next;

    if (View_GetActiveCameraMode(view) != 8 && View_IsModeAvailable(view, type) != 0)
        ok = 1;
    else
        ok = 0;
    View_PlaceCarCamera(view);
    if (ok == 0)
        return;
    active = g_activeViewRecord[view] + view * 2;
    next = view * 2 - g_activeViewRecord[view] + 1;
    if (blend == 0 || type == 3 || VIEW_RECORD(active)->type == 3)
        blend = 0;
    else
        blend = 1;
    if (type == 7 && param == 0xffff)
        param = StageObject_FindNearestCameraSpot(&Car_Get(view)->position);
    VIEW_RECORD(next)->type = type;
    if (VIEW_RECORD(next)->car != target) {
        Glow_NoOpEntryCallback(VIEW_RECORD(next)->car, VIEW_RECORD(next)->view, 1, 1);
        Glow_NoOpEntryCallback(target, VIEW_RECORD(next)->view, 1, 1);
    }
    VIEW_RECORD(next)->car = target;
    Camera_BeginLeave(VIEW_RECORD(active), VIEW_RECORD(next));
    Camera_BeginEnter(VIEW_RECORD(next), VIEW_RECORD(active));
    Camera_Start(VIEW_RECORD(next), type, param);
    VIEW_RECORD(next)->clearance = 0;
    if (blend) {
        g_unk0x00538d20[view] = 0xc8000;
        return;
    }
    Camera_EndLeave(VIEW_RECORD(active), VIEW_RECORD(next));
    Camera_EndEnter(VIEW_RECORD(next), VIEW_RECORD(active));
    Camera_AfterLeave(VIEW_RECORD(active));
    g_unk0x005391b0[view] = 1;
    g_activeViewRecord[view] = 1 - g_activeViewRecord[view];
}

// Per-frame camera of a view: the shake animation, both records' cameras and
// the blend between them while a transition runs.
// match 56%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004219b0
void View_UpdateCamera(BYTE view)
{
    BYTE index;
    BYTE active;
    BYTE next;
    int *pTimer;
    CameraRecord *pActive;
    CameraRecord *pNext;
    int typeA;
    int typeB;
    int activeFree;
    int activeTracked;
    int nextFree;
    int nextTracked;
    int t;

    index = (BYTE)view;
    active = g_activeViewRecord[index] + (BYTE)view * 2;
    next = (BYTE)view * 2 - g_activeViewRecord[index] + 1;
    if (g_unk0x00538f00[index] != 0) {
        if (g_unk0x00538f00[index] == 1) {
            g_unk0x00538c98[index] += 0xccc;
            if (g_unk0x00538c98[index] >= 0x10000) {
                g_unk0x00538f00[index] = 2;
                g_unk0x00538c98[index] = 0x10000;
            }
        } else if (g_unk0x00538f00[index] == 3) {
            g_unk0x00538c98[index] += -0xccc;
            if (g_unk0x00538c98[index] <= 0) {
                g_unk0x00538f00[index] = 0;
                g_unk0x00538c98[index] = 0;
            }
        }
        g_unk0x00538df8[index] += 0x44;
        if (g_unk0x00538df8[index] >= 0x1000)
            g_unk0x00538df8[index] -= 0x1000;
    }
    View_PlaceCarCamera(view);
    pTimer = &g_unk0x00538d20[index];
    if (g_unk0x00538d20[index] >= 0)
        CameraState_Copy(VIEW_PREV_STATE(index), VIEW_STATE(index));
    else
        CameraState_Copy(VIEW_PREV_STATE(index), VIEW_RECORD(active));
    pActive = VIEW_RECORD(active);
    Camera_Update(pActive);
    g_unk0x005391c4[index] = g_unk0x005391b0[index];
    g_unk0x005391b0[index] = 0;
    if (*pTimer < 0) {
        CameraState_Copy(VIEW_STATE(index), pActive);
        return;
    }
    pNext = VIEW_RECORD(next);
    Camera_Update(pNext);
    if (*pTimer - g_physicsTimeStep > 0) {
        typeA = pActive->type;
        if (typeA == 4 || typeA == 5 || typeA == 10)
            activeFree = 1;
        else
            activeFree = 0;
        if (typeA == 1 || typeA == 2)
            activeTracked = 1;
        else
            activeTracked = 0;
        typeB = pNext->type;
        if (typeB == 4 || typeB == 5 || typeB == 10)
            nextFree = 1;
        else
            nextFree = 0;
        nextTracked = 0;
        if (typeB == 1 || typeB == 2)
            nextTracked = 1;
        t = 0x10000 - FixMul(VIEW_EASE(*pTimer), VIEW_EASE(*pTimer));
        CameraState_Interpolate(VIEW_STATE(index), pActive, pNext, t);
        if ((activeFree && nextTracked) || (activeTracked && nextFree))
            VIEW_STATE(index)->matrix.position.y +=
                FixMul(g_sinTable[(short)(int)FTOI(__int64, X87S_MUL((double)(t * 180), g_fixedDegreesToAngle12)) & 0xfff], 0x10000);
        g_viewStates[index].type = 8;
        *pTimer -= g_physicsTimeStep;
        return;
    }
    Camera_EndLeave(pActive, pNext);
    Camera_EndEnter(pNext, pActive);
    Camera_AfterLeave(pActive);
    CameraState_Copy(VIEW_STATE(index), pNext);
    g_activeViewRecord[index] = 1 - g_activeViewRecord[index];
    *pTimer -= g_physicsTimeStep;
}

// OpenCMR2: rebuilds view 0's camera from the frozen car while the race is paused, so live camera tuning (F10 panel) shows at once.
// Visual only: no simulation step. Skipped while a camera transition blend runs.
void View_RefreshForTuning(void)
{
    BYTE index = 0;
    if (g_unk0x00538d20[index] >= 0) {
        static double lastSkip = 0; double now = Sys_GetTicksPrecise();
        if (now - lastSkip > 1000.0) { lastSkip = now; Sys_Log("tuning diag: camera refresh skipped, a camera transition is running (timer %d)", (int)g_unk0x00538d20[index]); }
        return;
    }
    CameraRecord *pActive = VIEW_RECORD(g_activeViewRecord[index] + index * 2);
    Camera_Update(pActive);
    CameraState_Copy(VIEW_STATE(index), pActive);
}

// Presentation-only lookup. Car_FreeTable leaves legacy slot pointers behind,
// and a new table can be visible before spawn initializes its matrix pointer.
// Never inspect a stale slot or follow an uninitialized pWorld from an overlay.
static Car *View_GetLivePresentationCar(unsigned view)
{
    if (view >= 2 || !g_carBuffer || g_carCount <= 0 || g_carCount > 40 ||
        view >= (unsigned)g_carCount) return nullptr;
    Car *expected = g_carBuffer + view;
    if (g_cars[view] != expected) return nullptr;
    if (expected->pWorld != &expected->physicsMatrix) return nullptr;
    return expected;
}

// OpenCMR2 debug overlay: camera position relative to the car, measured this frame, in world units, along the car body matrix axes
// (forward is the matrix forward axis, + ahead of the car, - behind). dist is the straight distance.
void View_GetCameraCarOffset(unsigned view, double *dist, double *fwd, double *side, double *up)
{
    BYTE v = (BYTE)(view & 1);
    Car *car = View_GetLivePresentationCar(view);
    *dist = *fwd = *side = *up = 0;
    if (!car || !car->pWorld) return;
    double d[3] = {(double)(VIEW_STATE(v)->matrix.position.x - car->position.x) / 65536.0, (double)(VIEW_STATE(v)->matrix.position.y - car->position.y) / 65536.0, (double)(VIEW_STATE(v)->matrix.position.z - car->position.z) / 65536.0};
    const FixVector *f = &car->pWorld->forward, *r = &car->pWorld->right, *u = &car->pWorld->up;
    *dist = sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
    *fwd = (d[0]*f->x + d[1]*f->y + d[2]*f->z) / 65536.0;
    *side = (d[0]*r->x + d[1]*r->y + d[2]*r->z) / 65536.0;
    *up = (d[0]*u->x + d[1]*u->y + d[2]*u->z) / 65536.0;
}

// Camera position of a view in fixed-point world coordinates (x, z). Used to cull tyre-mark history around the camera.
void View_GetCameraXZ(unsigned view, int *x, int *z)
{
    BYTE v = (BYTE)(view & 1);
    *x = VIEW_STATE(v)->matrix.position.x;
    *z = VIEW_STATE(v)->matrix.position.z;
}

// Resets the cameras of a view to their records (after a restart).
// FUNCTION: CMR2 0x00421d80
void View_ResetCameras(int view)
{
    BYTE active;
    BYTE next;

    active = g_activeViewRecord[(BYTE)view] + (BYTE)view * 2;
    next = (BYTE)view * 2 - g_activeViewRecord[(BYTE)view] + 1;
    View_PlaceCarCamera(view);
    Camera_Restart(VIEW_RECORD(active));
    VIEW_RECORD(active)->clearance = 0;
    if (View_GetActiveCameraMode(view) == 8) {
        Camera_Restart(VIEW_RECORD(next));
        VIEW_RECORD(next)->clearance = 0;
    }
    g_unk0x005391b0[(BYTE)view] = 1;
}

// Snaps a view's cameras to the car's camera placement and refreshes the view
// state (blending when a transition is running).
// match 69%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00421e20
void View_SnapCameras(BYTE view)
{
    FixMatrix placement;
    BYTE active;
    BYTE next;

    active = g_activeViewRecord[view] + view * 2;
    next = view * 2 - g_activeViewRecord[view] + 1;
    View_PlaceCarCamera(view);
    View_GetCarCameraRotation(&placement, view);
    Camera_SnapTo(VIEW_RECORD(active), &placement);
    if (View_GetActiveCameraMode(view) == 8) {
            Camera_SnapTo(VIEW_RECORD(next), &placement);
        CameraState_Interpolate(VIEW_STATE(view), VIEW_RECORD(active), VIEW_RECORD(next),
                     0x10000 - FixMul(VIEW_EASE(g_unk0x00538d20[view]), VIEW_EASE(g_unk0x00538d20[view])));
        View_UpdateCamera(view);
        return;
    }
    CameraState_Copy(VIEW_STATE(view), VIEW_RECORD(active));
    View_UpdateCamera(view);
}

// Switches a view to camera type `type` (0 = its current one) on `target`.
// FUNCTION: CMR2 0x00422fe0
void View_SetCameraType(int view, int type, BYTE target, int blend)
{
    if (type == 0)
        type = View_GetActiveCameraMode(view);
    View_SwitchCamera(view, type, 0xffff, target, blend);
}

// Blends a view's two stored camera states, carries the blended record's
// shake into the player's camera nodes, rebuilds the view node's matrix
// (pitching the forward axis and re-orthogonalising the basis) and marks the
// node and its ancestors dirty.
// OpenCMR2 (visual only, default off): video.speed_fov widens the regular cameras' field of view with the player
// car's speed, by that many percent of the projection scale at full speed (0 to 40). The drone camera has its own
// speed lens (video.drone_speed_lens) and is skipped. The widening eases in and out over about half a second and
// never changes the camera position, the car or any physics input.
extern BYTE g_playerCameraMode[8];
extern BYTE g_droneVariant[8];
static int g_speedFovShown[2], g_speedFovPct[2];
// The camera operator's drone is the active camera of this player (same test the operator itself uses).
bool View_IsDroneCamera(int view) {
    if (view < 0 || view >= 2 || !Sys_GetOption("video.camera_operator", 0)) return false;
    unsigned slot = g_activeViewRecord[view];
    if (slot > 1) return false;
    unsigned record = slot + unsigned(view) * 2;
    return g_viewRecords[record].type == 5 && g_playerCameraMode[record] == 4 && g_droneVariant[record];
}
// Diagnostics overlay: the effective percent setting and the eased 0..65536 speed ratio of a view.
void SpeedFov_GetStatus(int view, int *pct, int *shown) { *pct = g_speedFovPct[view & 1]; *shown = g_speedFovShown[view & 1]; }
// Called by View_SetPlayerProjection once per rendered view (not from the logic step, whose cadence did not track the
// frame clock: a native run showed the eased value stuck at 13% from 22 to 146 kph). Eases in real time.
int View_SpeedFovZoom(BYTE view, int zoom)
{
    static double lastMs[2];
    int (&shown)[2] = g_speedFovShown;
    int pct = Sys_GetOption("video.speed_effects", 1) != 0 ? Sys_GetOption("video.speed_fov", 0) : 0;   // menu switch off: no speed FOV widening
    {   // Cockpit only: a share of the speed FOV (default 50%, 100 = the full amount as before). Stored scaled, so the readout and the
        // widening-dependent effects see the same effective amount. Other camera types are untouched.
        int type = 0, drone = 0;
        if (view < 2) AttractCapture_ViewSignature(view, &type, &drone);
        if (type == 3) { int share = Sys_GetOption("video.speed_fov_cockpit", 25); if (share < 0) share = 0; if (share > 100) share = 100; pct = pct * share / 100; }
    }
    g_speedFovPct[view & 1] = pct;
    if (pct <= 0 || view >= 2) { shown[view & 1] = 0; return zoom; }
    if (pct > kSpeedFovMaxPct) pct = kSpeedFovMaxPct;
    if (View_IsDroneCamera(view)) { shown[view] = 0; return zoom; }   // no speed FOV in the drone; the eased value is reset so the dolly and readouts see no widening
    Car *car = View_GetLivePresentationCar(view);
    int target = SpeedFov_HeldTarget(SpeedFov_Target(car ? car->speed : 0), Sys_GetOption("video.speed_fov_hold", 0));
    double now = Sys_GetTicksPrecise();
    int elapsed = lastMs[view] == 0.0 ? 1000 : (int)(now - lastMs[view]);
    if (elapsed >= 1 || lastMs[view] == 0.0) {   // sub-millisecond repeat calls in one frame leave the state alone
        lastMs[view] = now;
        shown[view] = SpeedFov_Ease(shown[view], target, elapsed);
    }
    return SpeedFov_Zoom(zoom, pct, shown[view]);
}

// OpenCMR2: how much the speed FOV currently widens a view, 16.16 (65536 = not at all). The same eased value the projection uses.
int View_SpeedFovMagnification(BYTE view)
{
    int pct = g_speedFovPct[view & 1];
    if (pct <= 0 || view >= 2) return 65536;
    if (pct > kSpeedFovMaxPct) pct = kSpeedFovMaxPct;
    return 65536 + (int)((long long)pct * g_speedFovShown[view & 1] / 100);
}

// OpenCMR2 debug camera move (visual only): each camera family has its own signed setting, the amount at full speed, and the move blends from 0 standing
// still to that amount at full speed (the same 0..1 speed ratio as the speed FOV). Chase (types 4-6): video.camera_move_chase. Cockpit (3):
// video.camera_move_cockpit. Car-mounted (1, 2, 10: bonnet, bumper): video.camera_move_car. Returns 16.16 model units; 0.05 per setting step.
int View_CameraMoveStep(int cameraType, int view)
{
    const char *key = (cameraType >= 4 && cameraType <= 6) ? "video.camera_move_chase" : cameraType == 3 ? "video.camera_move_cockpit" : "video.camera_move_car";
    int setting;
    {   // a per-type value (--camera-move-type N=V, F10 row, video.camera_move_type<N>) replaces the family value for that type, also when it is an explicit 0
        char tk[40]; snprintf(tk, sizeof tk, "video.camera_move_type%d", cameraType);
        setting = CameraMove_Effective(Sys_GetOption(tk, kCamMoveUnset), Sys_GetOption(key, 0));
    }
    if (setting == 0) return 0;
    Car *car = View_GetLivePresentationCar((unsigned)view);
    int ratio = SpeedFov_HeldTarget(SpeedFov_Target(car ? car->speed : 0), Sys_GetOption("video.speed_fov_hold", 0));
    return (int)((long long)setting * 0xCCC * ratio / 65536);
}

bool ReplayControls_Active();
// Replay arrows adjust only render-time offsets; original camera records remain intact.
static ReplayChaseOrbit g_replayChaseOrbit[2];
static double g_replayChaseLastMs[2]={};
static int g_replayChaseSignature[2]={-1,-1};
void View_ReplayChaseReset(){for(int i=0;i<2;i++){g_replayChaseOrbit[i].Reset();g_replayChaseLastMs[i]=0;g_replayChaseSignature[i]=-1;}}
void View_ReplayChaseFrame(unsigned view,int left,int right,int up,int down,int reset){
 if(view>=2)return;
 int mode=View_GetActiveCameraMode((BYTE)view);
 unsigned record=g_activeViewRecord[view]+view*2;
 int signature=mode*256+(record<8?g_droneVariant[record]*16:0)+View_GetActiveCameraFlags((BYTE)view);
 if(g_replayChaseSignature[view]!=signature){g_replayChaseOrbit[view].Reset();g_replayChaseLastMs[view]=0;g_replayChaseSignature[view]=signature;}
 double now=Sys_GetTicksPrecise(),dt=g_replayChaseLastMs[view]?(now-g_replayChaseLastMs[view])/1000.:0;
 g_replayChaseLastMs[view]=now;
 if(reset)g_replayChaseOrbit[view].Reset();else g_replayChaseOrbit[view].Step(left,right,up,down,dt);
}
static void View_ApplyReplayChase(BYTE view,FixMatrix&matrix,int lookDistance){
 if(view>=2||!ReplayControls_Active()||View_FreeCamActive()||!ReplayChaseOrbit::Chase(View_GetActiveCameraMode(view)))return;
 const auto&o=g_replayChaseOrbit[view];if(o.yaw==0&&o.pitch==0)return;
 double p[3]={matrix.position.x/65536.,matrix.position.y/65536.,matrix.position.z/65536.};
 double f[3]={matrix.forward.x/65536.,matrix.forward.y/65536.,matrix.forward.z/65536.};
 double u[3]={matrix.up.x/65536.,matrix.up.y/65536.,matrix.up.z/65536.};
 double r[3]={matrix.right.x/65536.,matrix.right.y/65536.,matrix.right.z/65536.};
 // Prefer the camera's own aim distance; legacy chase records without it use
 // the distance to the followed car, not a player-indexed or guessed slot.
 double distance=lookDistance/65536.;
 if(distance<1){Car*car=View_GetLivePresentationCar(view);if(!car)return;double dx=p[0]-car->position.x/65536.,dy=p[1]-car->position.y/65536.,dz=p[2]-car->position.z/65536.;distance=std::sqrt(dx*dx+dy*dy+dz*dz);}
 o.Apply(p,r,u,f,distance);
 auto fixed=[](const double*v){return FixVector{(int)std::lround(v[0]*65536),(int)std::lround(v[1]*65536),(int)std::lround(v[2]*65536)};};
 matrix.position=fixed(p);matrix.right=fixed(r);matrix.up=fixed(u);matrix.forward=fixed(f);
}

// FUNCTION: CMR2 0x00422140
void View_BlendCameraStates(BYTE view, int t)
{
    CameraRecord state;
    FixMatrix matrix;
    FixVector pitch;
    FixVector right;
    FixVector flat;
    FixVector forward;
    SceneNode *node;
    int len;
    int f48;
    int f58;
    int f5c;
    int f60;
    int shake;

    CameraState_Interpolate(&state, VIEW_PREV_STATE(view), VIEW_STATE(view), t);
    g_viewZooms[view] = state.zoom;
    g_unk0x005391a8[view] = state.field_0x50;
    g_viewNearPlanes[view] = state.nearPlane;
    matrix = state.matrix;
    f48 = state.carLift;
    f58 = state.liftPitch;
    f5c = state.lookDistance;
    f60 = state.clearance;
    if (f60 != 0) {
        Car *car = Car_Get(view);
        int *pY;

        shake = FixMul(f48, f60);
        pY = &car->pSceneRoot->current.position.y;
        *pY += shake;
        pY = &car->pBodyNode->current.position.y;
        *pY += shake;
        matrix.position.y += f60;
        shake = FixMul(f60, f58);
        if (f5c != 0 && shake != 0) {
            FixMatrix_GetForward(&forward, &matrix);
            flat = forward;
            flat.y = 0;
            len = FixVec_Length(&flat);
            FixVecScale(&flat, &flat, forward.y);
            flat.y = -len;
            FixVecScale(&pitch, &forward, f5c);
            FixVecScale(&flat, &flat, shake);
            forward.x = flat.x + pitch.x;
            forward.y = flat.y + pitch.y;
            forward.z = flat.z + pitch.z;
            FixVec_Normalize(&forward, &forward);
            FixMatrix_SetForward(&forward, &matrix);
            FixMatrix_GetRight(&right, &matrix);
            FixVecScale(&pitch, &forward, FixVecDot(&right, &forward));
            right.x -= pitch.x;
            right.y -= pitch.y;
            right.z -= pitch.z;
            FixVec_Normalize(&right, &right);
            FixMatrix_SetRight(&right, &matrix);
            FixVecCross(&pitch, &forward, &right);
            FixMatrix_SetUp(&pitch, &matrix);
        }
    }
    View_ApplyReplayChase(view,matrix,f5c);
    node = g_viewNodes[view];
    FixMatrix_CopyRotation(&matrix, &node->current);
    while (node != NULL) {
        node->dirty = 1;
        node = node->pParent;
    }
}
