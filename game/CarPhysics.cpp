#include <stddef.h>
#include "FloatToInt.h"
#include "CarPhysics.h"
#include "Car.h"
#include "CarResources.h"
#include "CarParts.h"
#include "GameInfo.h"
#include "RallyData.h"
#include "StageTiming.h"
#include "Graphics.h"
#include "Sprite.h"

// Wheel / ground contact of the cars (0x494b50-0x498570). Every frame the
// car being simulated is loaded into the g_phys* globals (its transform, axes
// and wheel matrices); the contact patches, ground heights and grip of its
// wheels are kept in its CarContact record.

extern CarContact *g_carContacts;
extern int g_carContactCount;

// Skid-trail bend unit conversion (16.16 degree steps -> sine-table angle).
extern double g_fixedDegreesToAngle12;
extern double g_minusFixedDegreesToAngle12;

void *CarInfo_GetSection(Car *pCar, int section);
int Track_GetGroundHeightSurface(FixVector *pPoint, FixVector *pNormal, short *pTri, short *pSurfaceClass,
                                 unsigned short *pSurface, int defaultY);
SceneNode *RallyData_GetChallengeRenderState(void);
int Car_UsesNarrowWheels(Car *pCar, int param2);
void Scene_GetShadowColour(DWORD *pColour, int *pLevel);
void CarShadow_OffsetPointsTowardsCamera(int view, CarContact *pContact);
CarTransforms *Car_GetTransforms(int index);
FixMatrix *Car_GetWheelTransforms(int index);
FixVector *Car_GetRendererRecord(int index);
void CarPhysics_UpdateWheelContactPatches(Car *pCar);
void Graphics_SetLayerQuadColour(BYTE *pColour);
void Graphics_SetTextureFactorAlpha(BYTE *pColour);
void CarContact_BuildBodyPatch(Car *pCar, CarContact *pContact);
int Track_GetGroundHeight(FixVector *pPoint, FixVector *pNormal, short *pTri, unsigned short *pSurface, int defaultY);


// Per-car pointers into the car's handling data (see CarContact_InitStageRecords).
// GLOBAL: CMR2 0x00592278
BYTE *g_physSkidCount[8];
// GLOBAL: CMR2 0x00592298
FixVector *g_physGroundPoint;   // the car's 8 hull points
// GLOBAL: CMR2 0x0059229c
int *g_physWheelLength[8];
// GLOBAL: CMR2 0x005922bc
BYTE *g_physSkidRange[8];
// GLOBAL: CMR2 0x005922e0
FixVector g_physGroundPos;
// GLOBAL: CMR2 0x005922ec
CarTransforms *g_physBody;
// GLOBAL: CMR2 0x005922f0
FixVector g_physPos;
// GLOBAL: CMR2 0x005922fc
CarSkidProfilePoint *g_physSkidProfilePoints[8];
// GLOBAL: CMR2 0x00592320
FixVector g_physRight;
// GLOBAL: CMR2 0x0059232c
FixVector g_physUp;
// GLOBAL: CMR2 0x00592338
FixVector g_physForward;
// GLOBAL: CMR2 0x00592344
int *g_physSkidOffset[8];
// GLOBAL: CMR2 0x00592468
FixVector g_physPatchDir;
// GLOBAL: CMR2 0x00592474
FixVector g_physPatchSide;
// GLOBAL: CMR2 0x00592480
int g_physPatchLength;
// GLOBAL: CMR2 0x00592484
int g_physPatchWidth;
// GLOBAL: CMR2 0x00592488
FixMatrix *g_physWheels;
// Copy of the contact record being drawn, pulled towards the camera.
// GLOBAL: CMR2 0x00592490
CarContact g_physContactView;

// Scale of the body patch edges (the first two, then the last two) by the
// stage type of the car's timing record.
// GLOBAL: CMR2 0x00520150
int g_physPatchScaleA[14] = {0x10000, 0x1072b, 0xfa5e, 0x1076c, 0xf74b, 0xfd70, 0xefdf,
                             0x112f1, 0xb916,  0xe5a1, 0xe418,  0xebc6, 0xf1eb, 0xf1eb};
// GLOBAL: CMR2 0x00520188
int g_physPatchScaleB[14] = {0x10000, 0x10a3d, 0x10000, 0x10624, 0x10000, 0x108b4, 0xfe76,
                             0xfe76,  0xd8d4,  0x10f5c, 0x105e3, 0xfeb8,  0xfe76,  0xfe76};
// Pairs of opposite body patch corners.
// GLOBAL: CMR2 0x005201c0
BYTE g_physPatchEdges[5][2] = {{1, 2}, {0, 3}, {1, 0}, {2, 3}, {0, 0}};
// Reference axis of the skid trail and its length scale.
// GLOBAL: CMR2 0x005201c8
FixVector g_physTrailAxis = {0x10000, 0, 0};
// GLOBAL: CMR2 0x005201d4
int g_physTrailScale = 0x10000;

// Resets the contact records for a new stage and caches the pointers into
// every car's handling data. In the time trial modes only the first car
// (the player) is driven, the others are ghosts.
// FUNCTION: CMR2 0x00494bb0
void CarContact_InitStageRecords(void)
{
    int i;
    int j;
    int driven;
    BYTE *data;

    driven = (CGameInfo::GetConfiguredGameMode() == 5 || CGameInfo::GetConfiguredGameMode() == 6 ||
              CGameInfo::GetConfiguredGameMode() == 7);
    if (driven) {
        for (i = 0; i < g_carContactCount; i++) {
            g_carContacts[i].ghostContact = 1;
            g_carContacts[i].wheelPatchesEnabled = 1;
            if (i == 0) {
                g_carContacts[0].ghostContact = 0;
                if ((BYTE)RallyDataState() == 1)
                    g_carContacts[0].wheelPatchesEnabled = 1;
            }
            g_carContacts[i].shadowLevel = 0x9999;
            for (j = 0; j < 4; j++)
                g_carContacts[i].wheelGrip[j] = 0x10000;
        }
    } else {
        for (i = 0; i < g_carContactCount; i++) {
            g_carContacts[i].ghostContact = 0;
            g_carContacts[i].wheelPatchesEnabled = 1;
            g_carContacts[i].shadowLevel = 0x9999;
            for (j = 0; j < 4; j++)
                g_carContacts[i].wheelGrip[j] = 0x10000;
        }
    }
    for (i = 0; i < g_carContactCount; i++) {
        // MSVC6 needs the original byte induction; each typed view names its bias.
#define CONTACT_AT(member) ((CarContactProfile *)(data - offsetof(CarContactProfile, member)))
        data = (BYTE *)CarInfo_GetSection(Car_Get(i), CAR_INFO_CONTACT);
        g_physSkidCount[i] = &CONTACT_AT(pointCount)->pointCount;
        data += sizeof(((CarContactProfile *)0)->pointCount);
        g_physSkidRange[i] = &CONTACT_AT(visibleRangeStart)->visibleRangeStart;
        data += offsetof(CarContactProfile, longitudinalOffset) - offsetof(CarContactProfile, visibleRangeStart);
        g_physSkidOffset[i] = &CONTACT_AT(longitudinalOffset)->longitudinalOffset;
        data += sizeof(((CarContactProfile *)0)->longitudinalOffset);
        g_physWheelLength[i] = &CONTACT_AT(wheelPatchLength)->wheelPatchLength;
        data += sizeof(((CarContactProfile *)0)->wheelPatchLength);
        g_physSkidProfilePoints[i] = CONTACT_AT(points)->points;
#undef CONTACT_AT
    }
}

// Whether skid point index (counted from the front, or from the back when
// skidRangeAscending is clear) falls in the car's visible skid range.
// FUNCTION: CMR2 0x00494d40
int CarContact_IsSkidPointVisible(Car *pCar, CarContact *pContact, int index)
{
    if (pContact->skidIndexOffset != 0)
        index--;
    if (pContact->skidRangeAscending == 0)
        index = (*g_physSkidCount[pCar->index] - index) - 1;
    index -= *g_physSkidRange[pCar->index];
    if (index >= 0 && index < g_physSkidRange[pCar->index][1])
        return 1;
    return 0;
}

// Updates the contact patch of every wheel: its position under the car
// (dropped onto the ground), the grip left by the suspension travel and the
// four corners of the patch, flattened onto the ground plane.
// match 60%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00497db0
void CarPhysics_UpdateWheelContactPatches(Car *pCar)
{
    CarContact *pContact;
    FixVector offset;
    FixVector down;
    FixVector local;
    FixVector pos;
    FixVector fwd;
    FixVector side;
    FixVector d;
    FixVector *pCorner;
    int blend;
    int wheel;
    int travel;
    int length;
    int t;
    short i;
    short surfaceClass;

    pContact = &g_carContacts[pCar->index];
    if (pCar->field_0xb64 != 0) {
        blend = 0x10000;
    } else {
        blend = FixVecDot(&g_physUp, &g_physBody->groundNormal) - 0xcccc;
        if (blend < 0)
            blend = 0;
        blend = FixMul(blend, 0x50000);
        if (blend > 0x10000)
            blend = 0x10000;
    }
    FixVecScale(&offset, &g_physUp, -0x5999);
    if (CGameInfo::IsActiveCheatEnabled(6))
        FixVecScale(&offset, &offset, 0x28000);
    for (wheel = 0; wheel < 4; wheel++) {
        if (pCar->pWheelNodes[wheel]->pParent == RallyData_GetChallengeRenderState())
            continue;
        FixMatrix_GetPosition(&local, &g_physWheels[wheel]);
        FixMatrix_RotateVector(&pos, &local, &g_physBody->body);
        pos.x += g_physPos.x + offset.x;
        pos.y += g_physPos.y + offset.y;
        pos.z += g_physPos.z + offset.z;
        if (pCar->field_0xb64 == 0) {
            down.x = 0;
            down.y = 0x10000;
            down.z = 0;
            t = Track_GetGroundHeightSurface(&pos, &down, &pContact->wheelTriangle[wheel], &surfaceClass,
                                             (unsigned short *)&surfaceClass, pContact->wheelGroundY[wheel]);
            pContact->wheelGroundY[wheel] = t;
            travel = pos.y - t;
            pos.y = t;
            if (travel > 0) {
                t = FixMul(travel, FixDiv(0x10000, 0x4000));
                if (t > 0x10000)
                    t = 0x10000;
                pContact->wheelGrip[wheel] = 0x10000 - t;
            } else {
                pContact->wheelGrip[wheel] = 0x10000;
            }
        } else {
            pContact->wheelGrip[wheel] = 0x10000;
        }
        pContact->wheelGrip[wheel] = FixMul(blend, pContact->wheelGrip[wheel]);
        FixMatrix_GetForward(&local, &g_physWheels[wheel]);
        FixMatrix_RotateVector(&fwd, &local, &g_physBody->body);
        FixVecCross(&side, &g_physUp, &fwd);
        FixVecScale(&side, &side, 0x5999);
        length = *g_physWheelLength[pCar->index];
        FixVecScale(&fwd, &fwd, length);
        if (Car_UsesNarrowWheels(pCar, 1))
            FixVecScale(&fwd, &fwd, 0x9999);
        pCorner = pContact->wheelCorners[wheel];
        pCorner[0].x = side.x + pos.x;
        pCorner[0].y = side.y + pos.y;
        pCorner[0].z = side.z + pos.z;
        pCorner[2].x = pos.x - side.x;
        pCorner[2].y = pos.y - side.y;
        pCorner[2].z = pos.z - side.z;
        pCorner[1].x = pCorner[0].x - fwd.x;
        pCorner[1].y = pCorner[0].y - fwd.y;
        pCorner[1].z = pCorner[0].z - fwd.z;
        pCorner[0].x += fwd.x;
        pCorner[0].y += fwd.y;
        pCorner[0].z += fwd.z;
        pCorner[3].x = pCorner[2].x - fwd.x;
        pCorner[3].y = pCorner[2].y - fwd.y;
        pCorner[3].z = pCorner[2].z - fwd.z;
        pCorner[2].y = fwd.y + pCorner[2].y;
        pCorner[2].x = fwd.x + pCorner[2].x;
        pCorner[2].z = pCorner[2].z + fwd.z;
        if (pCar->field_0xb64 == 0) {
            for (i = 0; i < 4; i++) {
                d.x = pCorner[i].x - pos.x;
                d.y = pCorner[i].y - pos.y;
                d.z = pCorner[i].z - pos.z;
                t = FixVecDot(&down, &d);
                FixVecScale(&side, &down, t);
                d.x -= side.x;
                d.z -= side.z;
                d.y -= side.y;
                pCorner[i].x = d.x + pos.x;
                pCorner[i].y = d.y + pos.y;
                pCorner[i].z = d.z + pos.z;
            }
        }
    }
}

// Builds the contact patch under the body: a rectangle on the ground plane
// spanning the car's hull points (or taken from the car's box when the
// ground normal is unknown), or, for ghost cars, the hull points themselves
// rescaled for the stage.
// FUNCTION: CMR2 0x004962c0
void CarContact_BuildBodyPatch(Car *pCar, CarContact *pContact)
{
    BYTE stage;
    FixVector d;
    FixVector mid;
    FixVector fwd;
    FixVector side;
    int t;
    int u;
    int along;
    int i;

    stage = StageTiming_GetStartTableRecord(pCar->index)->modelClass;
    g_physPatchWidth = 0;
    g_physPatchLength = 0;
    if (pContact->ghostContact != 0 && pCar->useUpperCollisionCorners == 0) {
        pContact->points[1] = g_physGroundPoint[0];
        pContact->points[0] = g_physGroundPoint[1];
        pContact->points[3] = g_physGroundPoint[3];
        pContact->points[2] = g_physGroundPoint[2];
        if (pCar->field_0xb64 == 0) {
            pContact->points[1].y = g_physBody->cornerHeight[0];
            pContact->points[0].y = g_physBody->cornerHeight[1];
            pContact->points[3].y = g_physBody->cornerHeight[3];
            pContact->points[2].y = g_physBody->cornerHeight[2];
        }
        if (stage == pCar->type)
            return;
        for (i = 0; i < 4; i++) {
            d.x = pContact->points[g_physPatchEdges[i][0]].x - pContact->points[g_physPatchEdges[i][1]].x;
            d.y = pContact->points[g_physPatchEdges[i][0]].y - pContact->points[g_physPatchEdges[i][1]].y;
            d.z = pContact->points[g_physPatchEdges[i][0]].z - pContact->points[g_physPatchEdges[i][1]].z;
            FixVecScale(&d, &d, 0x8000);
            mid.x = pContact->points[g_physPatchEdges[i][1]].x + d.x;
            mid.y = pContact->points[g_physPatchEdges[i][1]].y + d.y;
            mid.z = pContact->points[g_physPatchEdges[i][1]].z + d.z;
            if (i < 2)
                FixVecScale(&d, &d, g_physPatchScaleA[stage]);
            else
                FixVecScale(&d, &d, g_physPatchScaleB[stage]);
            pContact->points[g_physPatchEdges[i][0]].x = d.x + mid.x;
            pContact->points[g_physPatchEdges[i][0]].y = d.y + mid.y;
            pContact->points[g_physPatchEdges[i][0]].z = d.z + mid.z;
            pContact->points[g_physPatchEdges[i][1]].x = mid.x - d.x;
            pContact->points[g_physPatchEdges[i][1]].y = mid.y - d.y;
            pContact->points[g_physPatchEdges[i][1]].z = mid.z - d.z;
        }
        return;
    }
    if (pCar->field_0xb64 == 0) {
        t = FixVecDot(&g_physBody->groundNormal, &g_physRight);
        if (FIX_ABS(t) > 0xfd70) {
            FixVector *pUp = &g_physUp;
            t = FixVecDot(&g_physBody->groundNormal, pUp);
            FixVecScale(&d, &g_physBody->groundNormal, t);
            g_physPatchDir.x = pUp->x - d.x;
            g_physPatchDir.y = pUp->y - d.y;
            g_physPatchDir.z = pUp->z - d.z;
            FIX_NORMALIZE_INTO(g_physPatchDir, g_physPatchDir);
            FixVecCross(&g_physPatchSide, &g_physPatchDir, &g_physBody->groundNormal);
        } else {
            FixVecScale(&d, &g_physBody->groundNormal, t);
            g_physPatchDir.x = g_physRight.x - d.x;
            g_physPatchDir.y = g_physRight.y - d.y;
            g_physPatchDir.z = g_physRight.z - d.z;
            FIX_NORMALIZE_INTO(g_physPatchDir, g_physPatchDir);
            FixVecCross(&g_physPatchSide, &g_physPatchDir, &g_physBody->groundNormal);
        }
        for (i = 0; i < 8; i++) {
            d.x = g_physGroundPoint[i].x - g_physPos.x;
            d.y = g_physGroundPoint[i].y - g_physPos.y;
            d.z = g_physGroundPoint[i].z - g_physPos.z;
            along = FixVecDot(&d, &g_physPatchDir);
            u = FixVecDot(&d, &g_physPatchSide);
            if (along > 0 && along > g_physPatchLength)
                g_physPatchLength = along;
            if (u > 0 && u > g_physPatchWidth)
                g_physPatchWidth = u;
        }
        FixVecScale(&fwd, &g_physPatchDir, g_physPatchLength);
        FixVecScale(&side, &g_physPatchSide, g_physPatchWidth);
        pContact->points[1].x = fwd.x + side.x;
        pContact->points[1].y = fwd.y + side.y;
        pContact->points[1].z = fwd.z + side.z;
        pContact->points[0].x = fwd.x - side.x;
        pContact->points[0].y = fwd.y - side.y;
        pContact->points[0].z = fwd.z - side.z;
        FixVecScale(&fwd, &fwd, -0x10000);
        pContact->points[2].x = fwd.x + side.x;
        pContact->points[2].y = fwd.y + side.y;
        pContact->points[2].z = fwd.z + side.z;
        pContact->points[3].x = fwd.x - side.x;
        pContact->points[3].y = fwd.y - side.y;
        pContact->points[3].z = fwd.z - side.z;
        for (i = 0; i < 4; i++) {
            pContact->points[i].x += g_physGroundPos.x;
            pContact->points[i].y += g_physGroundPos.y;
            pContact->points[i].z += g_physGroundPos.z;
        }
        return;
    }
    g_physPatchDir = pCar->right;
    g_physPatchSide = pCar->forward;
    g_physPatchLength = pCar->halfExtents.x;
    g_physPatchWidth = pCar->halfExtents.z;
    d.x = g_physGroundPos.x - pCar->position.x;
    d.y = g_physGroundPos.y - pCar->position.y;
    d.z = g_physGroundPos.z - pCar->position.z;
    FixVecScale(&mid, &pCar->up, pCar->halfExtents.y);
    d.x += mid.x;
    d.y += mid.y;
    d.z += mid.z;
    for (i = 0; i <= 3; i++) {
        pContact->points[i].x = pCar->corners[i].x + d.x;
        pContact->points[i].y = d.y + pCar->corners[i].y;
        pContact->points[i].z = pCar->corners[i].z + d.z;
    }
    d = pContact->points[2];
    pContact->points[2] = pContact->points[3];
    pContact->points[3] = d;
}

#define FIX_MIDPOINT(out, a, b) \
    out.x = (a).x - (b).x;      \
    out.y = (a).y - (b).y;      \
    out.z = (a).z - (b).z;      \
    FixVecScale(&out, &out, 0x8000); \
    out.x += (b).x;             \
    out.y += (b).y;             \
    out.z += (b).z

#define SHADOW_TRIANGLE(p0, c0, p1, c1, p2, c2) \
    *(FixVector *)&vertices[0] = (p0);         \
    *(FixVector *)&vertices[1] = (p1);         \
    *(FixVector *)&vertices[2] = (p2);         \
    *(DWORD *)vertices[0].colour = *(DWORD *)(c0); \
    *(DWORD *)vertices[1].colour = *(DWORD *)(c1); \
    *(DWORD *)vertices[2].colour = *(DWORD *)(c2); \
    Quad2D_QueueFixedTriangle(0, &vertices[0], &vertices[1], &vertices[2], NULL, 10)

// Octagon around the body patch (its corners cut at 5% / 95% of each edge)
// and the solid core, 60% of its size.
#define SHADOW_OCTAGON(pBody, pRing, pCore, centre)                        \
    for (i = 0; i < 4; i++) {                                              \
        d.x = pBody[(i + 1) % 4].x - pBody[i].x;                           \
        d.y = pBody[(i + 1) % 4].y - pBody[i].y;                           \
        d.z = pBody[(i + 1) % 4].z - pBody[i].z;                           \
        FixVecScale(&e, &d, 0xf333);                                       \
        FixVecScale(&d, &d, 0xccc);                                        \
        pRing[i * 2 + 1].x = pBody[i].x + d.x;                             \
        pRing[i * 2 + 1].y = d.y + pBody[i].y;                             \
        pRing[i * 2 + 1].z = d.z + pBody[i].z;                             \
        pRing[(i * 2 + 2) % 8].x = pBody[i].x + e.x;                       \
        pRing[(i * 2 + 2) % 8].y = pBody[i].y + e.y;                       \
        pRing[(i * 2 + 2) % 8].z = pBody[i].z + e.z;                       \
    }                                                                      \
    for (i = 0; i < 8; i++) {                                              \
        d.x = pRing[i].x - centre.x;                                       \
        d.y = pRing[i].y - centre.y;                                       \
        d.z = pRing[i].z - centre.z;                                       \
        FixVecScale(&d, &d, 0x9999);                                       \
        pCore[i].x = centre.x + d.x;                                       \
        pCore[i].y = centre.y + d.y;                                       \
        pCore[i].z = centre.z + d.z;                                       \
    }

// Draws the shadow of the car: a soft octagon under the body, a patch under
// every wheel on the ground and, for the driven car, a smear along its skid
// trail. Each is a solid core fading out to a transparent rim.
// match 33%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00494db0
void CarPhysics_DrawBodyWheelAndSkidShadows(Car *pCar, int view)
{
    Quad2DInputVertex vertices[3];
    FixVector ring[18];
    FixVector outline[18];
    FixVector e;
    FixVector centre;
    FixVector d;
    FixVector *pSrc;
    BYTE colour[4];
    BYTE clear[4];
    int level;
    int alpha;
    int count;
    BOOL all;
    int i;
    int k;
    int t;
    short total;
    short n;

    colour[0] = 0;
    colour[1] = 0;
    colour[2] = 0;
    colour[3] = 0x32;
    count = *g_physSkidCount[pCar->index];
    all = TRUE;
    if (CarPart_GetSet(pCar->index)->partHidden[3] != 0) {
        all = FALSE;
        count -= g_physSkidRange[pCar->index][1];
    }
    Scene_GetShadowColour((DWORD *)colour, &level);
    colour[3] = 0x32;
    *(DWORD *)clear = *(DWORD *)colour;
    clear[3] = 0;
    g_physContactView = g_carContacts[pCar->index];
    CarShadow_OffsetPointsTowardsCamera(view, &g_physContactView);
    vertices[0].u = 0;
    vertices[0].v = 0;
    vertices[1].u = 0;
    vertices[1].v = 0;
    vertices[2].u = 0;
    vertices[2].v = 0;

    // Wheels.
    if (g_physContactView.wheelPatchesEnabled != 0) {
        for (i = 0; i < 4; i++) {
            if (pCar->pWheelNodes[i]->pParent == RallyData_GetChallengeRenderState() ||
                g_physContactView.wheelGrip[i] == 0)
                continue;
            alpha = g_physContactView.wheelGrip[i];
            alpha = FixMul(0xe60000, alpha);
            pSrc = g_carContacts[pCar->index].wheelCorners[i];
            FIX_MIDPOINT(d, pSrc[0], pSrc[2]);
            FIX_MIDPOINT(centre, pSrc[1], pSrc[3]);
            g_carContacts[pCar->index].wheelFrontMid[i].x = d.x;
            g_carContacts[pCar->index].wheelFrontMid[i].y = d.y;
            g_carContacts[pCar->index].wheelFrontMid[i].z = d.z;
            g_carContacts[pCar->index].wheelRearMid[i].x = centre.x;
            g_carContacts[pCar->index].wheelRearMid[i].y = centre.y;
            g_carContacts[pCar->index].wheelRearMid[i].z = centre.z;
            FIX_MIDPOINT(d, g_physContactView.wheelCorners[i][0], g_physContactView.wheelCorners[i][2]);
            FIX_MIDPOINT(centre, g_physContactView.wheelCorners[i][1], g_physContactView.wheelCorners[i][3]);
            colour[3] = (BYTE)(alpha >> 16);
            SHADOW_TRIANGLE(g_physContactView.wheelCorners[i][0], clear, g_physContactView.wheelCorners[i][1], clear, d, colour);
            SHADOW_TRIANGLE(d, colour, g_physContactView.wheelCorners[i][1], clear, centre, colour);
            SHADOW_TRIANGLE(d, colour, centre, colour, g_physContactView.wheelCorners[i][2], clear);
            SHADOW_TRIANGLE(centre, colour, g_physContactView.wheelCorners[i][3], clear, g_physContactView.wheelCorners[i][2], clear);
        }
    }

    FIX_MIDPOINT(centre, g_physContactView.points[0], g_physContactView.points[2]);

    // Skid trail: the trailing edge of the body followed by the skid points.
    if (!(g_pGraphics->field913_0x3bc & 0x20) && g_physContactView.ghostContact == 0) {
        k = g_physContactView.trailEdge;
        outline[0] = g_physContactView.points[k];
        outline[1] = g_physContactView.points[(k + 1) % 4];
        n = 2;
        for (i = 0; i < *g_physSkidCount[pCar->index] + 1; i++) {
            if (all || !CarContact_IsSkidPointVisible(pCar, &g_physContactView, i)) {
                outline[n] = g_physContactView.points[4 + i];
                n++;
            }
        }
        outline[count + 3] = g_physContactView.points[(k + 3) % 4];
        outline[count + 4] = g_physContactView.points[(k + 2) % 4];
        total = (short)(count + 4);
        t = 0x10000 - g_physContactView.shadowLevel;
        t = 0xfd70 - FixMul(t, 0x23d7);
        for (i = 0; i < total + 1; i++) {
            d.x = outline[i].x - centre.x;
            d.y = outline[i].y - centre.y;
            d.z = outline[i].z - centre.z;
            FixVecScale(&d, &d, t);
            ring[i].x = centre.x + d.x;
            ring[i].y = d.y + centre.y;
            ring[i].z = d.z + centre.z;
        }
        // This multiply is emitted by the original even though its result
        // is discarded before the shadow-level calculation.
        t = g_physContactView.shadowLevel;
        FixMul(0xc80000, t);
        colour[3] = (BYTE)(FixMul(level, pCar->field_0xa70) >> 16);
        for (i = 1; i <= total; i++) {
            SHADOW_TRIANGLE(ring[i - 1], colour, centre, colour, ring[i % total], colour);
            level = i % total;
            SHADOW_TRIANGLE(outline[i - 1], clear, ring[i - 1], colour, outline[level], clear);
            SHADOW_TRIANGLE(outline[level], clear, ring[i - 1], colour, ring[level], colour);
        }
    }

    // Body: fades out as the car leaves the ground.
    if (g_physContactView.ghostContact != 0) {
        SHADOW_OCTAGON(g_physContactView.points, ring, outline, centre);
        colour[3] = 0xff;
    } else {
        SHADOW_OCTAGON(g_physContactView.points, ring, outline, centre);
        t = pCar->corners[0].y - pCar->cornerHeight[0];
        for (i = 1; i < 8; i++) {
            if (pCar->corners[i].y - pCar->cornerHeight[i] < t)
                t = pCar->corners[i].y - pCar->cornerHeight[i];
        }
        if (t <= 0) {
            t = 0x10000;
        } else {
            t = FixMul(t, 0x20000);
            if (t > 0x10000)
                t = 0x10000;
            t = 0x10000 - t;
        }
        colour[3] = FixMulShift32(t, 0xff0000);
    }
    for (i = 1; i <= 8; i++) {
        level = i % 8;
        SHADOW_TRIANGLE(outline[level], colour, outline[i - 1], colour, centre, colour);
        SHADOW_TRIANGLE(ring[i - 1], clear, outline[i - 1], colour, ring[level], clear);
        SHADOW_TRIANGLE(outline[i - 1], colour, outline[level], colour, ring[level], clear);
    }
}

// Pulls the shadow points one unit towards the camera so they do not sink
// into the ground: the body and skid points, and the wheel patches when drawn.
// FUNCTION: CMR2 0x00495f50
void CarShadow_OffsetPointsTowardsCamera(int view, CarContact *pContact)
{
    FixVector cam;
    FixVector d;
    FixVector *p;
    int len;
    int i;

    FixMatrix_GetPosition(&cam, &g_viewNodes[view]->current);
    for (i = 0; i < pContact->pointCount; i++) {
        p = &pContact->points[i];
        d.x = cam.x - p->x;
        d.y = cam.y - p->y;
        d.z = cam.z - p->z;
        len = FixVecLength(&d);
        if (len > 0x20000 && len != 0) {
            FixVecScaleRecip(&d, &d, len);
            FixVecScale(&d, &d, 0x10000);
            // The original adds the components out of order here (it picks the
            // base register of the point from the component touched last).
            p->x += d.x;
            p->z += d.z;
            p->y += d.y;
        }
    }
    if (pContact->wheelPatchesEnabled != 0) {
        p = pContact->wheelCorners[0];
        for (i = 16; i != 0; i--) {
            d.x = cam.x - p->x;
            d.y = cam.y - p->y;
            d.z = cam.z - p->z;
            len = FixVecLength(&d);
            if (len > 0x20000 && len != 0) {
                FixVecScaleRecip(&d, &d, len);
                FixVecScale(&d, &d, 0x10000);
                p->x += d.x;
                p->y += d.y;
                p->z += d.z;
            }
            p++;
        }
    }
}

// Loads the car into the g_phys* globals, drops it onto the ground and
// updates its contact record: the wheel and body patches and, for a driven
// car, the skid trail laid out behind the body patch along the car's skid
// profile, bent round when the car is sliding sideways. Every point is then
// eased towards the ground height under it.
// match 62%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00496e00
void CarPhysics_UpdateBodyContactAndSkidTrail(Car *pCar)
{
    CarContact *pContact;
    CarSkidProfilePoint *pProfile;
    FixVector flat;
    FixVector p;
    FixVector v;
    FixVector w;
    FixVector *pNormal;
    int h;
    int scale;
    int cosA;
    int sideDot;
    int fwd;
    int side;
    int grip;
    int bend;
    int base;
    int lateral;
    int firstAlong;
    int firstLat;
    int acc;
    int limit;
    int len;
    int c;
    int k;
    int first;
    short angle;
    int skip;

    pContact = &g_carContacts[pCar->index];
    firstAlong = 0;
    firstLat = 0;
    acc = 0;
    side = 0;
    first = 0;
    pProfile = g_physSkidProfilePoints[pCar->index];
    g_physBody = Car_GetTransforms(pCar->index);
    g_physWheels = Car_GetWheelTransforms(pCar->index);
    FixMatrix_GetRight(&g_physRight, &g_physBody->body);
    FixMatrix_GetUp(&g_physUp, &g_physBody->body);
    FixMatrix_GetForward(&g_physForward, &g_physBody->body);
    FixMatrix_GetPosition(&g_physPos, &g_physBody->body);
    g_physGroundPoint = Car_GetRendererRecord(pCar->index);
    if (pCar->field_0xb64 != 0) {
        h = FixMul(g_physBody->groundNormal.y, g_physBody->cornerHeight[0] - g_physPos.y);
    } else {
        g_physGroundPos = *g_physGroundPoint;
        g_physGroundPos.x -= g_physPos.x;
        g_physGroundPos.y = g_physBody->cornerHeight[0] - g_physPos.y;
        pNormal = &g_physBody->groundNormal;
        g_physGroundPos.z -= g_physPos.z;
        h = FixVecDot(pNormal, &g_physGroundPos);
    }
    FixVecScale(&g_physGroundPos, &g_physBody->groundNormal, h);
    g_physGroundPos.x += g_physPos.x;
    g_physGroundPos.y += g_physPos.y;
    g_physGroundPos.z += g_physPos.z;
    if (pContact->wheelPatchesEnabled != 0)
        CarPhysics_UpdateWheelContactPatches(pCar);
    CarContact_BuildBodyPatch(pCar, pContact);
    if (pContact->ghostContact != 0 || (g_pGraphics->field913_0x3bc & 0x20)) {
        pContact->pointCount = 4;
    } else {
        // Trail direction relative to the car, flattened onto the ground.
        scale = FixDiv(g_physPatchLength, pProfile->longitudinalDistance);
        flat = g_physPatchDir;
        flat.y = 0;
        FIX_NORMALIZE_INTO(flat, flat);
        flat.y = 0;
        cosA = FixVecDot(&g_physTrailAxis, &flat);
        w.x = -flat.z;
        w.y = 0;
        w.z = flat.x;
        sideDot = FixVecDot(&g_physTrailAxis, &w);
        grip = FixVecDot(&g_physUp, &g_physBody->groundNormal);
        grip -= 0xcccc;
        if (grip < 0) {
            grip = 0;
        } else {
            grip = FixMul(grip, FixDiv(0x10000, 0x3334));
            if (grip > 0x10000)
                grip = 0x10000;
        }
        fwd = FixMul(cosA, g_physTrailScale);
        side = FixMul(sideDot, g_physTrailScale);
        bend = FixMul(grip, side);
        if (fwd < 0) {
            sideDot = pProfile[*g_physSkidCount[pCar->index] - 1].lateralWidth;
        } else {
            sideDot = pProfile[0].lateralWidth;
        }
        base = FixMul(sideDot + *g_physSkidOffset[pCar->index], fwd);
        lateral = g_physPatchWidth;
        if (side < 0)
            lateral = -g_physPatchWidth;
        lateral += FixMul(*g_physSkidOffset[pCar->index], side);
        if (fwd < 0) {
            if (side < 0) {
                pContact->trailEdge = 1;
                skip = 1;
            } else {
                pContact->trailEdge = 0;
                skip = 0;
            }
        } else if (side < 0) {
            skip = 0;
            pContact->trailEdge = 2;
        } else {
            pContact->trailEdge = 3;
            skip = 1;
        }
        pContact->skidIndexOffset = skip;
        if (side < 0)
            pContact->skidRangeAscending = 0;
        else
            pContact->skidRangeAscending = 1;
        for (c = 0; c < *g_physSkidCount[pCar->index]; c++) {
            sideDot = pProfile[c].longitudinalDistance;
            h = FixMul(pProfile[c].lateralWidth, bend);
            h += lateral;
            if (c != 0 && c != *g_physSkidCount[pCar->index] - 1) {
                if (fwd >= 0)
                    grip = pProfile[c - 1].longitudinalDistance - sideDot;
                else
                    grip = pProfile[c + 1].longitudinalDistance - sideDot;
                sideDot += FixMul(grip, FIX_ABS(cosA));
            }
            grip = FixMul(sideDot + base, scale);
            if (fwd < 0 ? c == *g_physSkidCount[pCar->index] - 1 : c == 0) {
                firstAlong = grip;
                firstLat = h;
            }
            FixVecScale(&p, &g_physPatchDir, grip);
            FixVecScale(&v, &g_physPatchSide, h);
            p.x += v.x;
            p.y += v.y;
            p.z += v.z;
            if (side >= 0)
                k = c + 4;
            else
                k = *g_physSkidCount[pCar->index] - c + 3;
            if (skip)
                k++;
            if ((c == 0 && side >= 0) || (c == *g_physSkidCount[pCar->index] - 1 && side < 0))
                first = k;
            pContact->points[k].x = p.x + g_physGroundPos.x;
            pContact->points[k].y = p.y + g_physGroundPos.y;
            pContact->points[k].z = p.z + g_physGroundPos.z;
        }
        // Closing point, just outside the body patch.
        if (side < 0)
            firstLat += FixMul(g_physPatchWidth, 0x20000);
        else
            firstLat -= FixMul(g_physPatchWidth, 0x20000);
        FixVecScale(&v, &g_physPatchSide, firstLat);
        p.y = FixMul(g_physPatchDir.y, firstAlong) + v.y;
        p.z = FixMul(g_physPatchDir.z, firstAlong) + v.z;
        p.x = FixMul(g_physPatchDir.x, firstAlong) + v.x;
        if (skip == 0) {
            pContact->points[*g_physSkidCount[pCar->index] + 4].x = p.x + g_physGroundPos.x;
            pContact->points[*g_physSkidCount[pCar->index] + 4].y = g_physGroundPos.y + p.y;
            pContact->points[*g_physSkidCount[pCar->index] + 4].z = g_physGroundPos.z + p.z;
        } else {
            pContact->points[4].x = p.x + g_physGroundPos.x;
            pContact->points[4].y = p.y + g_physGroundPos.y;
            pContact->points[4].z = p.z + g_physGroundPos.z;
        }
        pContact->pointCount = *g_physSkidCount[pCar->index] + 5;
    }
    if (pContact->ghostContact != 0)
        return;
    if (!(g_pGraphics->field913_0x3bc & 0x20)) {
        // Bend the trail: no point may turn away from the trail direction
        // faster than 1.5 degrees per point.
        if (side < 0)
            c = *g_physSkidCount[pCar->index] - 1;
        else
            c = 0;
        while (c < *g_physSkidCount[pCar->index] && c >= 0) {
            k = c + (short)first;
            v.x = pContact->points[k % pContact->pointCount].x - g_physGroundPos.x;
            v.y = pContact->points[k % pContact->pointCount].y - g_physGroundPos.y;
            v.z = pContact->points[k % pContact->pointCount].z - g_physGroundPos.z;
            len = FixVecLength(&v);
            if (len > 0)
                FixVecScaleRecip(&v, &v, len);
            angle = FixAcos(FixVecDot(&g_physPatchDir, &v));
            limit = (0x400 - angle) * 0x1680;
            if (side < 0 ? c != *g_physSkidCount[pCar->index] - 1 : c != 0) {
                acc += 0x18000;
                if (limit < acc) {
                    if (side < 0)
                        FixVecScale(&w, &g_physPatchSide, FixSin(FTOI(__int64, X87S_MUL((double)-acc, g_fixedDegreesToAngle12))));
                    else
                        FixVecScale(&w, &g_physPatchSide, FixSin(FTOI(__int64, X87S_MUL((double)acc, g_fixedDegreesToAngle12))));
                    sideDot = FixCos(FTOI(__int64, X87S_MUL((double)acc, g_minusFixedDegreesToAngle12)));
                    p.x = FixMul(g_physPatchDir.x, sideDot) + w.x;
                    p.y = FixMul(g_physPatchDir.y, sideDot) + w.y;
                    p.z = FixMul(g_physPatchDir.z, sideDot);
                    p.z += w.z;
                    FIX_NORMALIZE_INTO(v, p);
                    FixVecScale(&v, &v, len);
                    pContact->points[((short)first + c) % pContact->pointCount].x = g_physGroundPos.x + v.x;
                    pContact->points[((short)first + c) % pContact->pointCount].y = g_physGroundPos.y + v.y;
                    pContact->points[((short)first + c) % pContact->pointCount].z = g_physGroundPos.z + v.z;
                    limit = acc;
                }
            }
            if (side < 0)
                c--;
            else
                c++;
            acc = limit;
        }
    }
    for (c = 0; c < pContact->pointCount; c++) {
        h = Track_GetGroundHeight(&pContact->points[c], &v, &pContact->pointTriangle[c], (unsigned short *)&first,
                                  pContact->pointGroundY[c]);
        pContact->pointGroundY[c] = h;
        h -= pContact->points[c].y;
        if (FIX_ABS(h) > 0x8000)
            h = h > 0 ? 0x8000 : -0x8000;
        pContact->points[c].y += h;
    }
}

// Points the skid trail reference axis along v (flattened, reversed) and
// sets the trail length from how far v leans out of the horizontal.
// FUNCTION: CMR2 0x00498370
void CarContact_SetSkidTrailAxis(FixVector *v)
{
    int len;

    g_physTrailAxis = *v;
    g_physTrailAxis.y = 0;
    len = FixVecLength(&g_physTrailAxis);
    if (len > 0)
        FixVecScaleRecip(&g_physTrailAxis, &g_physTrailAxis, -len);
    g_physTrailScale = FixMul(0x10000 - v->y, 0x20000);
}

// Sets the shadow level of a car and the matching blend colours.
// match 85%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004984b0
void CarShadow_SetLevel(int car, int level)
{
    BYTE colour[4] = {0, 0, 0, 0x32};
    BYTE factor[4] = {0, 0, 0, 0x32};
    BYTE alpha;

    g_carContacts[car].shadowLevel = level;
    colour[3] = FixMul(0x4b0000, g_carContacts[car].shadowLevel) >> 16;
    Graphics_SetLayerQuadColour(colour);
    alpha = FixMulShift32(0x4b0000, g_carContacts[car].shadowLevel) + 100;
    factor[3] = alpha;
    if (alpha > 0xff)
        factor[3] = 0xff;
    Graphics_SetTextureFactorAlpha(factor);
}
