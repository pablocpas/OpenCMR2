#include "Collision2D.h"
#include "Car.h"
#include "Sector.h"
#include "LayoutChecks.h"

// GLOBAL: CMR2 0x00591438
FixVector g_collisionQuad[4];

#define FIX_ABS(x) ((x) < 0 ? -(x) : (x))

// Intersects the ray pDir with the four sides of the collision quad and returns
// the distance to the closest side (0x7d000000 when it misses), along with the
// side that was hit.
// FUNCTION: CMR2 0x00489060
int Collision_RayQuad(FixVector *pDir, int *pEdge, BYTE *pCorner)
{
    FixVector d3;
    FixVector d0;
    int ax;
    int az;
    int bz;
    int cx;
    int cz;
    int dx;
    int c;
    int t;
    int result;

    result = 0x7d000000;
    d3.x = g_collisionQuad[3].x - g_collisionQuad[2].x;
    d3.y = g_collisionQuad[3].y - g_collisionQuad[2].y;
    d3.z = g_collisionQuad[3].z - g_collisionQuad[2].z;
    d0.x = g_collisionQuad[1].x - g_collisionQuad[0].x;
    d0.y = g_collisionQuad[1].y - g_collisionQuad[0].y;
    d0.z = g_collisionQuad[1].z - g_collisionQuad[0].z;
    ax = g_collisionQuad[0].x - g_collisionQuad[2].x;
    az = g_collisionQuad[2].z - g_collisionQuad[0].z;
    cx = g_collisionQuad[1].x - g_collisionQuad[2].x;
    cz = g_collisionQuad[2].z - g_collisionQuad[1].z;
    dx = g_collisionQuad[0].x - g_collisionQuad[3].x;
    bz = g_collisionQuad[3].z - g_collisionQuad[0].z;

    // c is the cross product of the two directions, then its reciprocal.
    c = FixMul(d0.z, d3.x) - FixMul(d0.x, d3.z);
    if (FIX_ABS(c) > 0x28f) {
        c = FixDiv(0x10000, c);
        t = FixMul(d0.z, ax) + FixMul(d0.x, az);
        t = FixMul(t, c);
        if (t >= 0 && t <= 0x10000) {
            t = FixMul(d3.z, ax) + FixMul(d3.x, az);
            t = FixMul(t, c);
            if (t >= 0 && t <= 0x10000) {
                c = FixMul(d3.x, pDir->z) - FixMul(d3.z, pDir->x);
                if (FIX_ABS(c) > 0x28f) {
                    c = FixDiv(0x10000, c);
                    t = FixMul(pDir->z, ax) + FixMul(pDir->x, az);
                    t = FixMul(t, c);
                    if (t >= 0 && t <= 0x10000) {
                        t = FixMul(d3.z, ax) + FixMul(d3.x, az);
                        t = -FixMul(t, c);
                        if (t >= 0 && t < result) {
                            result = t;
                            *pEdge = 0;
                            *pCorner = 0;
                        }
                    }
                    t = FixMul(pDir->z, cx) + FixMul(pDir->x, cz);
                    t = FixMul(t, c);
                    if (t >= 0 && t <= 0x10000) {
                        t = FixMul(d3.z, cx) + FixMul(d3.x, cz);
                        t = -FixMul(t, c);
                        if (t >= 0 && t < result) {
                            result = t;
                            *pEdge = 0;
                            *pCorner = 1;
                        }
                    }
                }
                c = FixMul(d0.z, pDir->x) - FixMul(d0.x, pDir->z);
                if (FIX_ABS(c) > 0x28f) {
                    c = FixDiv(0x10000, c);
                    t = FixMul(pDir->z, ax) + FixMul(pDir->x, az);
                    t = FixMul(t, c);
                    if (t >= 0 && t <= 0x10000) {
                        t = FixMul(d0.z, ax) + FixMul(d0.x, az);
                        t = FixMul(t, c);
                        if (t >= 0 && t < result) {
                            result = t;
                            *pEdge = 1;
                            *pCorner = 2;
                        }
                    }
                    t = FixMul(pDir->z, dx) + FixMul(pDir->x, bz);
                    t = FixMul(t, c);
                    if (t >= 0 && t <= 0x10000) {
                        t = FixMul(d0.z, dx) + FixMul(d0.x, bz);
                        t = FixMul(t, c);
                        if (t >= 0 && t < result) {
                            result = t;
                            *pEdge = 1;
                            *pCorner = 3;
                        }
                    }
                }
            }
        }
    }
    return result;
}

// GLOBAL: CMR2 0x005914a8
FixVector g_collisionPush;
// GLOBAL: CMR2 0x005914b8
FixVector g_collisionPushB;
// GLOBAL: CMR2 0x005915e8
FixVector g_collisionContactNormal;

// Splits a movement of `amount` along pDir between the two collision boxes: the
// scale*amount part goes to pB and the opposite of the remainder to pA.  Both
// boxes get their four local vectors, their eight-vector array and the vertex
// at +0x94 moved by the same amount.
// match 77%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// The instruction sequence is the original's; only the register numbering of
// the induction variables and the base/index order of the two 0x60 loops differ.
// FUNCTION: CMR2 0x004894b0
void Collision_SplitBoxSeparationMovement(CollisionBox *pA, CollisionBox *pB, FixVector *pDir, int amount, int scale)
{
    FixVector v;
    int t;
    int rest;
    int *p;
    int i;
    int j;

    g_collisionContactNormal.x = pDir->x;
    g_collisionContactNormal.y = pDir->y;
    g_collisionContactNormal.z = pDir->z;
    if (amount > 0) {
        t = FixMul(amount, scale);
        rest = amount - t;
        if (scale != 0) {
            FixVecScale(&v, pDir, t);
            // the original accumulates y, z and x in this order
            g_collisionPushB.y += v.y;
            g_collisionPushB.z += v.z;
            g_collisionPushB.x += v.x;
            if (pB->pVertex != NULL && pB->pArray != 0) {
                pB->pVertex->x += v.x;
                pB->pVertex->y += v.y;
                pB->pVertex->z += v.z;
                for (i = 0; i < 8; i++) {
                    pB->pArray[i].x += v.x;
                    pB->pArray[i].y += v.y;
                    pB->pArray[i].z += v.z;
                }
                p = &pB->pointWords[offsetof(FixVector, y) / sizeof(int)];
                for (j = 4; j != 0; j--) {
                    CollisionBoxPointAtY(p)->x += v.x;
                    CollisionBoxPointAtY(p)->y += v.y;
                    CollisionBoxPointAtY(p)->z += v.z;
                    p += sizeof(FixVector) / sizeof(int);
                }
            }
        }
        if (scale != 0x10000) {
            FixVecScale(&v, pDir, -rest);
            g_collisionPush.y += v.y;
            g_collisionPush.z += v.z;
            g_collisionPush.x += v.x;
            if (pA->pVertex != NULL && pA->pArray != 0) {
                pA->pVertex->x += v.x;
                pA->pVertex->y += v.y;
                pA->pVertex->z += v.z;
                for (i = 0; i < 8; i++) {
                    pA->pArray[i].x += v.x;
                    pA->pArray[i].y += v.y;
                    pA->pArray[i].z += v.z;
                }
                p = &pA->pointWords[offsetof(FixVector, y) / sizeof(int)];
                for (j = 4; j != 0; j--) {
                    CollisionBoxPointAtY(p)->x += v.x;
                    CollisionBoxPointAtY(p)->y += v.y;
                    CollisionBoxPointAtY(p)->z += v.z;
                    p += sizeof(FixVector) / sizeof(int);
                }
            }
        }
    }
}

// Clamps the magnitude of each component of v to limit (y only when clampY).
// FUNCTION: CMR2 0x0048c6e0
void Collision_ClampContactOffset(int *v, int *limit, int clampY)
{
    int a;

    a = v[0] < 0 ? -v[0] : v[0];
    if (a > limit[0])
        v[0] = v[0] < 0 ? -limit[0] : limit[0];
    if (clampY != 0) {
        a = v[1] < 0 ? -v[1] : v[1];
        if (a > limit[1])
            v[1] = v[1] < 0 ? -limit[1] : limit[1];
    }
    a = v[2] < 0 ? -v[2] : v[2];
    if (a > limit[2])
        v[2] = v[2] < 0 ? -limit[2] : limit[2];
}

extern int g_physicsScale;

// Clamps a vector to the scaled limits: eight units on X/Z, four on Y.
// FUNCTION: CMR2 0x0048c750
void Collision_ClampAngularCorrection(int *v)
{
    if (v[0] > FixMul(0x80000, g_physicsScale))
        v[0] = FixMul(0x80000, g_physicsScale);
    else if (v[0] < -FixMul(0x80000, g_physicsScale))
        v[0] = -FixMul(0x80000, g_physicsScale);
    if (v[1] > FixMul(0x40000, g_physicsScale))
        v[1] = FixMul(0x40000, g_physicsScale);
    else if (v[1] < -FixMul(0x40000, g_physicsScale))
        v[1] = -FixMul(0x40000, g_physicsScale);
    if (v[2] > FixMul(0x80000, g_physicsScale))
        v[2] = FixMul(0x80000, g_physicsScale);
    else if (v[2] < -FixMul(0x80000, g_physicsScale))
        v[2] = -FixMul(0x80000, g_physicsScale);
}


extern int g_collisionSphereRadius;
extern FixVector g_collisionSphereCentre;

// Oriented box of a stage object: the half extents along its two horizontal
// axes, the axes, the eight box points (the first four are its footprint) and
// the object's own corners and centre, which a push moves along.

// Tests the car's bounding sphere (centre g_collisionSphereCentre, radius
// g_collisionSphereRadius) against an oriented box. When the sphere is diagonally
// outside the box it is tested against the nearest corner (side 3), otherwise
// against the faces (side 1 for axis 0, 2 for axis 1). The sphere's push out
// of the box, scaled by `scale`, also moves the object when it can move, and
// is published in g_collisionPush. pAlong0/pAlong1 receive the sphere's
// position along both axes relative to the reach of the box.
// FUNCTION: CMR2 0x00489b20
int Collision_SphereVsBox(CollisionBox *param_1, int *pAlong0, unsigned int *pAlong1, int scale)
{
    CollisionBox *pBox = param_1;
    FixVector delta;
    FixVector dir;
    int dirAlong0;
    int dirAlong1;
    int along0;
    int along1;
    int t0;
    int t1;
    int best;
    int push;
    int hit0;
    int hit1;
    int reach0;
    int reach1;
    int radius2;
    int dist2;
    int a;
    int b;
    int corner;
    int i;
    FixVector *p;
    char side;

    t0 = 0;
    t1 = 0;
    side = 0;
    push = 0;
    delta.x = g_collisionSphereCentre.x - pBox->pVertex->x;
    delta.y = g_collisionSphereCentre.y - pBox->pVertex->y;
    delta.z = g_collisionSphereCentre.z - pBox->pVertex->z;
    delta.y = 0;
    FIX_NORMALIZE_INTO(dir, delta);
    dirAlong0 = FixVecDot(&dir, &pBox->axisA);
    dirAlong1 = FixVecDot(&dir, &pBox->axisB);
    along0 = FixVecDot(&delta, &pBox->axisA);
    along1 = FixVecDot(&delta, &pBox->axisB);
    if (FIX_ABS(along0) > pBox->halfWidth && FIX_ABS(along1) > pBox->halfLength) {
        // Diagonally outside the box: the sphere against the nearest corner.
        if (along0 > 0)
            corner = along1 > 0;
        else
            corner = (along1 > 0) + 2;
        radius2 = FixMul(g_collisionSphereRadius, g_collisionSphereRadius);
        delta.x = g_collisionSphereCentre.x - pBox->points[corner].x;
        delta.y = g_collisionSphereCentre.y - pBox->points[corner].y;
        delta.z = g_collisionSphereCentre.z - pBox->points[corner].z;
        dist2 = FixMul(delta.x, delta.x) + FixMul(delta.z, delta.z);
        if (dist2 > radius2)
            goto done;
        b = FixMul(dir.x, delta.x) + FixMul(dir.z, delta.z);
        a = FixMul(dir.x, dir.x) + FixMul(dir.z, dir.z);
        dist2 = FixMul(b, b) - FixMul(a, dist2 - radius2);
        if (dist2 >= 0)
            push = FixDiv(FixSqrt(dist2) - b, a);
        side = 3;
    } else {
        // Against the faces: how far the sphere must move along `dir` to leave
        // the box through each pair of faces.
        reach0 = pBox->halfWidth + g_collisionSphereRadius;
        reach1 = g_collisionSphereRadius + pBox->halfLength;
        if (abs(along0) > reach0)
            goto done;
        if (FIX_ABS(along1) > reach1)
            goto done;
        hit1 = 0;
        hit0 = 0;
        best = 0x7d000000;
        if (dirAlong0 > 0x41) {
            t0 = FixDiv(reach0 - along0, dirAlong0);
            hit0 = 1;
        } else if (dirAlong0 < -0x41) {
            a = along0 + reach0;
            if (-a >= 0)
                a = -a;
            t0 = FixDiv(a, -dirAlong0);
            hit0 = 1;
        }
        if (dirAlong1 > 0x41) {
            t1 = FixDiv(reach1 - along1, dirAlong1);
            hit1 = 1;
        } else if (dirAlong1 < -0x41) {
            a = reach1 + along1;
            if (-a >= 0)
                a = -a;
            t1 = FixDiv(a, -dirAlong1);
            hit1 = 1;
        }
        if (hit0 && t0 < best) {
            best = t0;
            side = 1;
        }
        if (hit1 && t1 < best) {
            best = t1;
            side = 2;
        }
        if (best > 0 && (hit1 || hit0))
            push = best;
        if (side == 0)
            goto done;
    }

    if (push > 0) {
        FixVecScale(&dir, &dir, -FixMul(push, scale));
        if (pBox->pVertex != NULL && pBox->pArray != NULL) {
            pBox->pVertex->x += dir.x;
            pBox->pVertex->y += dir.y;
            pBox->pVertex->z += dir.z;
            for (i = 0; i < 8; i++) {
                pBox->pArray[i].x += dir.x;
                pBox->pArray[i].y += dir.y;
                pBox->pArray[i].z += dir.z;
            }
            p = pBox->points;
            for (i = 4; i != 0; i--) {
                p->x += dir.x;
                p->y += dir.y;
                p->z += dir.z;
                p++;
            }
        }
        g_collisionPush = dir;
    } else {
        g_collisionPush.x = 0;
        g_collisionPush.y = 0;
        g_collisionPush.z = 0;
    }
    *pAlong0 = FixDiv(FixMul(along0, pBox->halfWidth), pBox->halfWidth + g_collisionSphereRadius);
    *pAlong1 = FixDiv(FixMul(along1, pBox->halfLength), g_collisionSphereRadius + pBox->halfLength);
done:
    return side;
}

// Contact speed thresholds of a light and a hard impact, set after a contact.
// GLOBAL: CMR2 0x005915dc
int g_contactImpactLowThreshold;
// GLOBAL: CMR2 0x00591468
int g_contactImpactHighThreshold;

void StageObject_ApplyRecursiveFrameDelta(BYTE index, char other, int *pDelta, int flag);

// Resolves the collision of `car` against the oriented box `pBox`: builds the
// correction vector from the two box axes and the two factors the sphere test
// returns, rotates it into world space, picks the face to push along (the
// direction to the reference object, or one of the two axes) and moves the car
// and the body of the box by the tangential remainder of the correction.
// match 59%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x00489750
int Collision_CarVsBox(Car *car, CollisionBox *pBox, int scale)
{
    FixVector a;
    FixVector b;
    FixVector along;
    int factor;
    int t;
    int dx;
    int dy;
    int dz;
    int i;
    short j;
    char side;
    CollisionBox *pB = pBox;

    side = (char)Collision_SphereVsBox(pBox, &factor, (unsigned int *)&t, scale);
    if (side != 0) {

        FixVecScale(&a, &pB->axisA, factor);
        FixVecScale(&b, &pB->axisB, t);
        a.x += b.x;
        a.y += b.y;
        a.z += b.z;
        FixMatrix_InverseRotateVector(&car->field_0x5dc, &a,
                                      car->pWorld);

        if (side != 3) {
            if (side == 1)
                g_collisionContactNormal = pB->axisA;
            else
                g_collisionContactNormal = pB->axisB;
        } else {
            g_collisionContactNormal.x = pB->pVertex->x + a.x;
            g_collisionContactNormal.y = pB->pVertex->y + a.y;
            g_collisionContactNormal.z = pB->pVertex->z + a.z;
            g_collisionContactNormal.x -= g_collisionSphereCentre.x;
            g_collisionContactNormal.y = 0;
            g_collisionContactNormal.z -= g_collisionSphereCentre.z;
            FIX_NORMALIZE_INTO(g_collisionContactNormal, g_collisionContactNormal)
        }

        FixVecScale(&along, &g_collisionContactNormal, FixVecDot(&g_collisionPush, &g_collisionContactNormal));
        dx = along.x - g_collisionPush.x;
        dy = along.y - g_collisionPush.y;
        dz = along.z - g_collisionPush.z;

        if (pB->pVertex != NULL && pB->pArray != NULL) {
            pB->pVertex->x += dx;
            pB->pVertex->y += dy;
            pB->pVertex->z += dz;
            for (i = 0; i < 8; i++) {
                pB->pArray[i].x += dx;
                pB->pArray[i].y += dy;
                pB->pArray[i].z += dz;
            }
            for (j = 0; j < 4; j++) {
                pB->points[j].x += dx;
                pB->points[j].y += dy;
                pB->points[j].z += dz;
            }
        }

        StageObject_ApplyRecursiveFrameDelta((BYTE)car->index, -1, (int *)&along, 1);
        g_contactImpactLowThreshold = 0x8000;
        g_contactImpactHighThreshold = 0x1578d;
        return 1;
    }
    return 0;
}

extern char g_unk0x00590ecc[4];
extern char g_unk0x005914c4[4];
extern char g_unk0x005914d4;
extern char g_unk0x005915f4;

// The collision scratch record each car points at while a contact is resolved:
// a 0x98-byte block whose entries from index 4 on are world-space contact
// points. The pointers are set up by the caller (0x48a1f0).
// GLOBAL: CMR2 0x005915e0
CollisionBox *g_pContacts0x005915e0;
// GLOBAL: CMR2 0x00591394
CollisionBox *g_pContacts0x00591394;

// Minimal declaration of the game-info singleton (the real one lives in
// GameInfo.h; keeping it local avoids perturbing the line table of this file).
class CGameInfo
{
public:
    static int IsActiveCheatEnabled(int param1);
};

void Car_SpawnDebris(int size, FixVector *pPos, Car *pCar, FixVector *pAxes, int count, int glassChance);
void CarDamage_ApplyCollisionDeformImpulse(Car *pCar, int *param_2, FixVector *param_3, int param_4, unsigned char param_5, int param_6);

// Collision box of a car while a contact is resolved (0x98 bytes): the two body
// axes from 0x10, the eight contact points built by Collision_BuildCarBox and the two
// pointers at 0x90/0x94 to the point array and to the box vertex.

extern int Collision_TestOrientedBoxCornerOverlap(CollisionBox *pBoxA, CollisionBox *pBoxB, FixVector *pOffset, int scale);

// Collision bookkeeping shared with the stage object collision code
// (defined in StageObjects.cpp): the per-car contact list and its length, the
// last pair result and the two objects that walk the car order.
// GLOBAL: CMR2 0x005913f4
int g_unk0x005913f4;

extern int g_unk0x00591390;
extern BYTE g_carContactCounts[8];
extern BYTE g_unk0x005913f8[8][8];
extern CollisionBox g_carCollisionBoxes[8];
extern BYTE g_unk0x00590ec8[4];
extern BYTE g_unk0x005914a4[4];
extern int g_physicsTimeStep;

int RallyData_IsChampionshipFinalStage(void);
unsigned char RallyData_GetFlag24(void);
unsigned int RallyData_GetSelectionFlag27(void);
int Collision_DoSpheresOverlap(int r1, int r2, int *pA, int *pB);
void StageObject_TestHeadlightGlowsAgainstCarBox(Car *pCar, CollisionBox *pBox);
void Collision_BuildCarBox(CollisionBox *pBox, int *pBodyAxes, FixVector *pPosition, FixVector *pCorners);
int Collision_SeparateCarBoxes(Car *param_1, Car *param_2);
void Collision_ResolveCarContactImpulse(Car *param_1, Car *param_2);

// Tests every pair of cars of the given order for contact: pairs whose
// sectors (each car's own plus its three neighbours) share one are run through the 2D box overlap test
// (Collision_DoSpheresOverlap) and, for cars that are not locked, through the box collision
// response (Collision_SeparateCarBoxes); the two cars are then linked in each other's contact
// list and the impact is resolved. Finally the separation timers of every car
// are decayed and, while the race is being verified, the collision box of each
// car is rebuilt.
// FUNCTION: CMR2 0x0048a1f0
void Collision_TestOrderedCarPairs(Car *pCars, short *pOrder, short count)
{
    Car *pCarA;
    Car *pCarB;
    int result;
    int i;
    int j;
    int k;
    int m;
    int n;
    short idsB[4];
    short idsA[4];
    int flags[8];

    for (i = 0; i < 8; i++)
        flags[i] = 0;
    memset(g_carContactCounts, 0, 8);
    n = count - 1;
    for (i = 0; i < n; i++) {
        for (j = i + 1; j < count; j++) {
            pCarA = &pCars[pOrder[i]];
            pCarB = &pCars[pOrder[j]];
            idsA[0] = pCarA->sector;
            memcpy(&idsA[1], &pCarA->neighbours, 6);
            idsB[0] = pCarB->sector;
            memcpy(&idsB[1], &pCarB->neighbours, 6);
            result = 0;
            k = 0;
            for (;;) {
                for (m = 0; m < 4; m++) {
                    if (idsA[m] == idsB[k])
                        goto found;
                }
                if (++k >= 4)
                    goto pair_done;
            }
        found:
            result = 1;
            if (Collision_DoSpheresOverlap(pCarA->collisionRadius, pCarB->collisionRadius,
                                           (int *)&pCarA->position, (int *)&pCarB->position)) {
                flags[pCarB->index] = 1;
                flags[pCarA->index] = 1;
                if (pCarA->field_0xc04[1] == 0 && pCarB->field_0xc04[1] == 0 &&
                    pCarA->isGhost == 0 && pCarB->isGhost == 0 &&
                    (pCarA->field_0xc18 == 0 || pCarB->field_0xc18 == 0)) {
                    g_pContacts0x005915e0 = &g_carCollisionBoxes[pOrder[i]];
                    g_pContacts0x00591394 = &g_carCollisionBoxes[pOrder[j]];
                    Collision_BuildCarBox(g_pContacts0x005915e0, (int *)&pCarA->right,
                                                   &pCarA->position, pCarA->corners);
                    Collision_BuildCarBox(g_pContacts0x00591394, (int *)&pCarB->right,
                                                   &pCarB->position, pCarB->corners);
                    result = Collision_SeparateCarBoxes(pCarA, pCarB);
                    g_unk0x00591390 = 0;
                    if (result != 0) {
                        g_unk0x005913f8[pCarA->index][g_carContactCounts[pCarA->index]] = pCarB->index;
                        g_carContactCounts[pCarA->index]++;
                        g_unk0x005913f8[pCarB->index][g_carContactCounts[pCarB->index]] = pCarA->index;
                        g_carContactCounts[pCarB->index]++;
                        Collision_ResolveCarContactImpulse(pCarA, pCarB);
                        g_unk0x00591390 = 1;
                    }
                }
            }
        pair_done:
            g_unk0x005913f4 = result;
        }
    }
    for (i = n; i >= 0; i--) {
        pCarA = &pCars[pOrder[i]];
        if (pCarA->field_0xc04[1] != 0) {
            if (pCarA->field_0x970[0] == 0) {
                if (flags[pCarA->index] == 0)
                    pCarA->field_0xc04[1] = 0;
            } else {
                pCarA->field_0x970[0] -= g_physicsTimeStep;
                if (pCarA->field_0x970[0] < 0)
                    pCarA->field_0x970[0] = 0;
            }
            pCarA->field_0x970[1] += FixMul(g_physicsTimeStep, 0x4000);
        } else {
            pCarA->field_0x970[1] = 0;
        }
    }
    if (((BYTE)RallyData_IsChampionshipFinalStage() != 0 || (BYTE)RallyData_GetFlag24() != 0 ||
         (BYTE)RallyData_GetSelectionFlag27() != 0) &&
        CGameInfo::IsActiveCheatEnabled(0) != 0) {
        for (i = n; i >= 0; i--) {
            pCarA = &pCars[pOrder[i]];
            g_pContacts0x005915e0 = &g_carCollisionBoxes[pOrder[i]];
            Collision_BuildCarBox(g_pContacts0x005915e0, (int *)&pCarA->right,
                                           &pCarA->position, pCarA->corners);
            StageObject_TestHeadlightGlowsAgainstCarBox(pCarA, g_pContacts0x005915e0);
        }
    }
}

// Pushes the two cars' collision boxes apart after a contact: adds up the
// corner offsets of both quads, normalises the result into the contact normal
// and, when one of the two cars has no contact corner left, moves each box
// along its own axis by the projection of the normal; the normal is published
// at 0x5915e8 and each car's contact offset is rotated by the other box's
// offset. When both cars do have corners the boxes are handed to the
// deformation solver instead.
// FUNCTION: CMR2 0x0048a5f0
int Collision_SeparateCarBoxes(Car *param_1, Car *param_2)
{
    FixVector normal;
    FixVector sum;
    FixVector delta;
    FixVector tmp;
    unsigned short result;
    int countA;
    int countB;
    int total;
    int len;
    int dx;
    int dy;
    int dz;
    int i;

    result = Collision_TestOrientedBoxCornerOverlap(g_pContacts0x005915e0, g_pContacts0x00591394,
                          &param_1->positionPrev, 0x8000);
    if (result != 0) {
    sum.x = 0;
    sum.y = 0;
    sum.z = 0;
    normal.x = 0;
    normal.y = 0;
    normal.z = 0;
    countA = 0;
    if (g_unk0x005915f4 >= 1) {
        for (i = 0; i < g_unk0x005915f4; i++) {
            delta.x = param_1->corners[g_unk0x005914c4[i]].x -
                      param_1->position.x;
            delta.y = param_1->corners[g_unk0x005914c4[i]].y -
                      param_1->position.y;
            delta.z = param_1->corners[g_unk0x005914c4[i]].z -
                      param_1->position.z;
            delta.y = 0;
            sum.x += delta.x;
            sum.z += delta.z;
            if (g_unk0x00590ec8[i] == 0) {
                if (FixVecDot(&normal, &(g_pContacts0x00591394)->axisA) < 0) {
                    normal.x -= (g_pContacts0x00591394)->axisA.x;
                    normal.y -= (g_pContacts0x00591394)->axisA.y;
                    normal.z -= (g_pContacts0x00591394)->axisA.z;
                } else {
                    normal.x += (g_pContacts0x00591394)->axisA.x;
                    normal.y += (g_pContacts0x00591394)->axisA.y;
                    normal.z += (g_pContacts0x00591394)->axisA.z;
                }
            } else {
                if (FixVecDot(&normal, &(g_pContacts0x00591394)->axisB) < 0) {
                    normal.x -= (g_pContacts0x00591394)->axisB.x;
                    normal.y -= (g_pContacts0x00591394)->axisB.y;
                    normal.z -= (g_pContacts0x00591394)->axisB.z;
                } else {
                    normal.x += (g_pContacts0x00591394)->axisB.x;
                    normal.y += (g_pContacts0x00591394)->axisB.y;
                    normal.z += (g_pContacts0x00591394)->axisB.z;
                }
            }
        }
        countA = g_unk0x005915f4;
    }
    countB = 0;
    if (g_unk0x005914d4 > 0) {
        for (i = 0; i < g_unk0x005914d4; i++) {
            delta.x = param_2->corners[g_unk0x00590ecc[i]].x -
                      param_1->position.x;
            delta.y = param_2->corners[g_unk0x00590ecc[i]].y -
                      param_1->position.y;
            delta.z = param_2->corners[g_unk0x00590ecc[i]].z -
                      param_1->position.z;
            delta.y = 0;
            sum.x += delta.x;
            sum.z += delta.z;
            if (g_unk0x005914a4[i] == 0) {
                if (FixVecDot(&normal, &(g_pContacts0x005915e0)->axisA) < 0) {
                    normal.x -= (g_pContacts0x005915e0)->axisA.x;
                    normal.y -= (g_pContacts0x005915e0)->axisA.y;
                    normal.z -= (g_pContacts0x005915e0)->axisA.z;
                } else {
                    normal.x += (g_pContacts0x005915e0)->axisA.x;
                    normal.y += (g_pContacts0x005915e0)->axisA.y;
                    normal.z += (g_pContacts0x005915e0)->axisA.z;
                }
            } else {
                if (FixVecDot(&normal, &(g_pContacts0x005915e0)->axisB) < 0) {
                    normal.x -= (g_pContacts0x005915e0)->axisB.x;
                    normal.y -= (g_pContacts0x005915e0)->axisB.y;
                    normal.z -= (g_pContacts0x005915e0)->axisB.z;
                } else {
                    normal.x += (g_pContacts0x005915e0)->axisB.x;
                    normal.y += (g_pContacts0x005915e0)->axisB.y;
                    normal.z += (g_pContacts0x005915e0)->axisB.z;
                }
            }
        }
        countB = g_unk0x005914d4;
    }
    total = (countA + countB) << 16;
    FixVecScaleRecip(&sum, &sum, total);
    len = FixVecLength(&normal);
    if (len == 0) {
        normal.x = 0;
        normal.y = 0;
        normal.z = 0;
    } else {
        FixVecScaleRecip(&normal, &normal, len);
    }
    if (g_unk0x005915f4 == 0 || g_unk0x005914d4 == 0) {
        FixVecScale(&delta, &normal, FixVecDot(&g_collisionPush, &normal));
        dx = delta.x - g_collisionPush.x;
        dy = delta.y - g_collisionPush.y;
        dz = delta.z - g_collisionPush.z;
        if ((g_pContacts0x005915e0)->pVertex != NULL &&
            (g_pContacts0x005915e0)->pArray != NULL) {
            (g_pContacts0x005915e0)->pVertex->x += dx;
            (g_pContacts0x005915e0)->pVertex->y += dy;
            (g_pContacts0x005915e0)->pVertex->z += dz;
            for (i = 0; i < 8; i++) {
                (g_pContacts0x005915e0)->pArray[i].x += dx;
                (g_pContacts0x005915e0)->pArray[i].y += dy;
                (g_pContacts0x005915e0)->pArray[i].z += dz;
            }
            for (i = 0; i < 4; i++) {
                (g_pContacts0x005915e0)->points[i].x += dx;
                (g_pContacts0x005915e0)->points[i].y += dy;
                (g_pContacts0x005915e0)->points[i].z += dz;
            }
        }
        StageObject_ApplyRecursiveFrameDelta(param_1->index, param_2->index, (int *)&delta, 1);
        FixVecScale(&delta, &normal, FixVecDot(&g_collisionPushB, &normal));
        dx = delta.x - g_collisionPushB.x;
        dy = delta.y - g_collisionPushB.y;
        dz = delta.z - g_collisionPushB.z;
        if ((g_pContacts0x00591394)->pVertex != NULL &&
            (g_pContacts0x00591394)->pArray != NULL) {
            (g_pContacts0x00591394)->pVertex->x += dx;
            (g_pContacts0x00591394)->pVertex->y += dy;
            (g_pContacts0x00591394)->pVertex->z += dz;
            for (i = 0; i < 8; i++) {
                (g_pContacts0x00591394)->pArray[i].x += dx;
                (g_pContacts0x00591394)->pArray[i].y += dy;
                (g_pContacts0x00591394)->pArray[i].z += dz;
            }
            for (i = 0; i < 4; i++) {
                (g_pContacts0x00591394)->points[i].x += dx;
                (g_pContacts0x00591394)->points[i].y += dy;
                (g_pContacts0x00591394)->points[i].z += dz;
            }
        }
        StageObject_ApplyRecursiveFrameDelta(param_2->index, param_1->index, (int *)&delta, 1);
    }
    g_collisionContactNormal.x = normal.x;
    g_collisionContactNormal.y = normal.y;
    g_collisionContactNormal.z = normal.z;
    FixMatrix_InverseRotateVector(&param_1->field_0x5dc, &sum,
                                  param_1->pWorld);
    sum.x += (g_pContacts0x005915e0)->pVertex->x;
    sum.y += (g_pContacts0x005915e0)->pVertex->y;
    sum.z += (g_pContacts0x005915e0)->pVertex->z;
    tmp.x = sum.x - (g_pContacts0x00591394)->pVertex->x;
    tmp.y = sum.y - (g_pContacts0x00591394)->pVertex->y;
    tmp.z = sum.z - (g_pContacts0x00591394)->pVertex->z;
    FixMatrix_InverseRotateVector(&param_2->field_0x5dc, &tmp,
                                  param_2->pWorld);
    }
    return result;
}

// Resolves a collision between two cars. Clamps each car's contact offset (the
// vector at +0x5dc) by its half extents, builds the world-space velocity of
// that offset out of the angular velocity at that point plus the body velocity,
// and projects both on the collision normal 0x5915e8. The closing speed along
// the normal, weighted by each body's mass (0x75c), gives the common rebound
// velocity; the difference to each car is the impulse it takes along the
// normal, which is turned into a linear correction (0x5c4) and, crossed with
// the contact offset, into an angular one (0x5d0). Cars that are locked
// (0xc00) get a four-times stronger offset correction and are marked at 0xc00
// while separated. When the two do not actually touch, the impulse along the
// contact axis is passed to the body-deformation solver (0x466ef0); otherwise
// the tangential relative motion is normalized and debris is thrown along it.
// match 72%: equivalent; written from the original's helper sequence (zero
// restitution term, FixVecScale impulses). The rest is the frame-slot packing
// of the inline-asm temporaries.
// FUNCTION: CMR2 0x0048ae90
void Collision_ResolveCarContactImpulse(Car *param_1, Car *param_2)
{
    FixVector vA;
    FixVector tmp;
    FixVector vB;
    FixVector impA;
    FixVector impB;
    FixVector axis;
    FixVector debrisAxes[3];
    int dotA;
    int dotB;
    int bounce;
    int x;
    int newB;
    int dA;
    int dB;
    int d;
    int len;
    int size;
    int bSepA;

    Collision_ClampContactOffset((int *)&param_1->field_0x5dc, (int *)&param_1->halfExtents, 0);
    Collision_ClampContactOffset((int *)&param_2->field_0x5dc, (int *)&param_2->halfExtents, 0);

    FixVecCross(&vA, &param_1->angularVelocity, &param_1->field_0x5dc);
    FixMatrix_RotateVector(&tmp, &vA, param_1->pWorld);
    tmp.x += param_1->velocity.x;
    tmp.y += param_1->velocity.y;
    tmp.z += param_1->velocity.z;

    FixVecCross(&vA, &param_2->angularVelocity, &param_2->field_0x5dc);
    FixMatrix_RotateVector(&vB, &vA, param_2->pWorld);
    vB.x += param_2->velocity.x;
    vB.y += param_2->velocity.y;
    vB.z += param_2->velocity.z;

    // Mass-weighted common velocity along the debrisAxes[2]; the restitution term
    // is zero in the shipped game.
    dotA = FixVecDot(&tmp, &g_collisionContactNormal);
    dotB = FixVecDot(&vB, &g_collisionContactNormal);
    bounce = FixMul(dotA - dotB, 0);
    x = FixDiv(FixMul(dotA, param_1->mass) +
                   (FixMul(dotB, param_2->mass) -
                    FixMul(bounce, param_2->mass)),
               param_1->mass + param_2->mass);
    newB = x + bounce;

    dA = x - dotA;
    FixVecScale(&impA, &g_collisionContactNormal, dA);
    if (param_1->useUpperCollisionCorners == 0 && FIX_ABS(dA) > 0x9999 &&
        (param_1->field_0xb64 == 0 || param_2->field_0xb64 == 0)) {
        bSepA = 1;
        vA.x = -param_1->right.y;
        vA.y = -param_1->up.y;
        vA.z = -param_1->forward.y;
        FixVecScale(&vA, &vA, 0x40000);
        param_1->field_0x5dc.x += vA.x;
        param_1->field_0x5dc.y += vA.y;
        param_1->field_0x5dc.z += vA.z;
    } else {
        bSepA = 0;
    }

    dB = newB - dotB;
    FixVecScale(&impB, &g_collisionContactNormal, dB);
    if (param_2->useUpperCollisionCorners == 0 && FIX_ABS(dB) > 0x9999 &&
        (param_1->field_0xb64 == 0 || param_2->field_0xb64 == 0)) {
        dB = 1;
        vA.x = -param_2->right.y;
        vA.y = -param_2->up.y;
        vA.z = -param_2->forward.y;
        FixVecScale(&vA, &vA, 0x40000);
        param_2->field_0x5dc.x += vA.x;
        param_2->field_0x5dc.y += vA.y;
        param_2->field_0x5dc.z += vA.z;
    } else {
        dB = 0;
    }

    if (((char *)param_1->field_0xb35)[0] != 1 && impA.y < 0) {
        impB.y -= impA.y;
        impA.y = 0;
    }
    if (((char *)param_2->field_0xb35)[0] != 1 && impB.y < 0) {
        impA.y -= impB.y;
        impB.y = 0;
    }

    FixVecScale(&impA, &impA, g_physicsScale);
    FixMatrix_InverseRotateVector(&vA, &impA, param_1->pWorld);
    param_1->field_0x5c4.x += impA.x;
    param_1->field_0x5c4.y += impA.y;
    param_1->field_0x5c4.z += impA.z;
    FixVecCross(&axis, &vA, &param_1->field_0x5dc);
    if (bSepA) {
        FixVecScale(&axis, &axis, 0x40000);
        param_1->useUpperCollisionCorners = 1;
        param_1->field_0x96c = 0x10000;
    } else {
        FixVecScale(&axis, &axis, 0x20000);
    }
    param_1->field_0x5d0.x += axis.x;
    param_1->field_0x5d0.y += axis.y;
    param_1->field_0x5d0.z += axis.z;
    Collision_ClampAngularCorrection((int *)&param_1->field_0x5d0);

    FixVecScale(&impB, &impB, g_physicsScale);
    FixMatrix_InverseRotateVector(&vA, &impB, param_2->pWorld);
    param_2->field_0x5c4.x += impB.x;
    param_2->field_0x5c4.y += impB.y;
    param_2->field_0x5c4.z += impB.z;
    FixVecCross(&axis, &vA, &param_2->field_0x5dc);
    if (dB) {
        FixVecScale(&axis, &axis, 0x40000);
        param_2->useUpperCollisionCorners = 1;
        param_2->field_0x96c = 0x10000;
    } else {
        FixVecScale(&axis, &axis, 0x20000);
    }
    param_2->field_0x5d0.x += axis.x;
    param_2->field_0x5d0.y += axis.y;
    param_2->field_0x5d0.z += axis.z;
    Collision_ClampAngularCorrection((int *)&param_2->field_0x5d0);

    if (g_unk0x005915f4 != 0) {
        CarDamage_ApplyCollisionDeformImpulse(param_1, (int *)&g_pContacts0x005915e0->points[g_unk0x005914c4[0]],
                     &g_collisionContactNormal, 0, 0, 0);
        CarDamage_ApplyCollisionDeformImpulse(param_2, (int *)&g_pContacts0x005915e0->points[g_unk0x005914c4[0]],
                     &g_collisionContactNormal, 0, 2, 0);
    } else if (g_unk0x005914d4 != 0) {
        CarDamage_ApplyCollisionDeformImpulse(param_1, (int *)&g_pContacts0x00591394->points[g_unk0x00590ecc[0]],
                     &g_collisionContactNormal, 0, 2, 0);
        CarDamage_ApplyCollisionDeformImpulse(param_2, (int *)&g_pContacts0x00591394->points[g_unk0x00590ecc[0]],
                     &g_collisionContactNormal, 0, 0, 0);
    }

    // Tangential relative velocity.
    vA.x = tmp.x - vB.x;
    vA.y = tmp.y - vB.y;
    vA.z = tmp.z - vB.z;
    d = FixVecDot(&vA, &g_collisionContactNormal);
    FixVecScale(&debrisAxes[0], &g_collisionContactNormal, d);
    debrisAxes[0].x = vA.x - debrisAxes[0].x;
    debrisAxes[0].y = vA.y - debrisAxes[0].y;
    debrisAxes[0].z = vA.z - debrisAxes[0].z;

    if (CGameInfo::IsActiveCheatEnabled(4) != 0) {
        param_1->field_0x5c4.x -= impA.x;
        param_1->field_0x5c4.y -= impA.y;
        param_1->field_0x5c4.z -= impA.z;
        param_1->velocity.x += impA.x * 2;
        param_1->velocity.y += impA.y * 2;
        param_1->velocity.z += impA.z * 2;
        param_2->field_0x5c4.x -= impB.x;
        param_2->field_0x5c4.y -= impB.y;
        param_2->field_0x5c4.z -= impB.z;
        param_2->velocity.x += impB.x * 2;
        param_2->velocity.y += impB.y * 2;
        param_2->velocity.z += impB.z * 2;
    }

    d = FixVecLength(&debrisAxes[0]);
    if (d > 0x3333) {
        FixVecScaleRecip(&debrisAxes[0], &debrisAxes[0], -d);
        FixVecCross(&debrisAxes[1], &debrisAxes[0], &g_collisionContactNormal);
        len = FixVecLength(&debrisAxes[1]);
        if (len == 0) {
            debrisAxes[1].x = 0;
            debrisAxes[1].y = 0;
            debrisAxes[1].z = 0;
        } else {
            FixVecScaleRecip(&debrisAxes[1], &debrisAxes[1], len);
        }
        debrisAxes[2] = g_collisionContactNormal;

        if (g_unk0x005915f4 != 0) {
            FixVector *pContact = &g_pContacts0x005915e0->points[g_unk0x005914c4[0]];
            impA.x = pContact->x - param_1->position.x;
            impA.y = g_pContacts0x005915e0->points[g_unk0x005914c4[0]].y - param_1->position.y;
            impA.z = g_pContacts0x005915e0->points[g_unk0x005914c4[0]].z - param_1->position.z;
            impB.x = pContact->x - param_2->position.x;
            impB.y = g_pContacts0x005915e0->points[g_unk0x005914c4[0]].y - param_2->position.y;
            impB.z = g_pContacts0x005915e0->points[g_unk0x005914c4[0]].z - param_2->position.z;
        } else if (g_unk0x005914d4 != 0) {
            FixVector *pContact = &g_pContacts0x00591394->points[g_unk0x00590ecc[0]];
            impA.x = pContact->x - param_1->position.x;
            impA.y = g_pContacts0x00591394->points[g_unk0x00590ecc[0]].y - param_1->position.y;
            impA.z = g_pContacts0x00591394->points[g_unk0x00590ecc[0]].z - param_1->position.z;
            impB.x = pContact->x - param_2->position.x;
            impB.y = g_pContacts0x00591394->points[g_unk0x00590ecc[0]].y - param_2->position.y;
            impB.z = g_pContacts0x00591394->points[g_unk0x00590ecc[0]].z - param_2->position.z;
        }

        impA.y = 0;
        FixVecScale(&impA, &impA, 0xcccc);
        impB.y = 0;
        FixVecScale(&impB, &impB, 0xcccc);

        size = FixMul(d, 0x20000);
        if (size > 0x10000)
            size = 0x10000;
        if (param_1->field_0xb70 == 0 && param_2->field_0xb70 == 0) {
            Car_SpawnDebris(size, &impA, param_1, &debrisAxes[0], 0x90000, 0x6666);
            Car_SpawnDebris(size, &impB, param_2, &debrisAxes[0], 0x90000, 0x6666);
        }
    }
}

// The current sector object has a box projection rather than sphere bounds.
// Set to a boolean by Collision_TestCarAgainstSectorObjects, never counted.
// GLOBAL: CMR2 0x005914d8
int g_collisionObjectHasBox;
// Corner copy the wall collision keeps for the other body of the contact:
// the corners of the object box at 0x5915f8 (StageObjects.cpp), from +0x30.
extern CollisionBox g_sectorObjectCollisionBox;

// Slides one car along a static obstacle. Zeroes the vertical contact offset,
// clamps the offset by the half extents (including y this time) and turns the
// angular velocity at that point into the world-space velocity of the contact.
// The normal speed 0x5915e8 . v is compared against the two thresholds the wall
// solver left in 0x591468 / 0x5915dc to decide whether the impact is strong
// enough to separate the car (and lock it for a frame) or only to bounce it.
// The contact object of the obstacle may ask for a damped impulse (0x2001000),
// which scales the rebound by 0x28f. The impulse along the normal is rotated
// into body space and added to the linear correction 0x5c4, and its moment
// about the contact offset 0x5d0 is built either from the (scaled) relative
// velocity cross the offset, or from a vertical axis offset by the contact
// point when the car is already separating -- in the latter case a strong
// impact also raises the pitch velocity 0x40c. The contact slot is recorded in
// 0xae0 (up to five per step) and the deformation solver gets the contact
// point when it is new. Returns whether the impact was damped.
// match 40%: reviewed (W172) - calls, constants and branch logic match; the
// diff is the FPU-dense block scheduling and the reused stack slots of the
// original (CONOCIMIENTO 4.t).
// FUNCTION: CMR2 0x0048be20
int Collision_ResolveStaticObstacleContact(Car *pCar, StageObject **ppObject, FixVector *unusedPosition, int contactId)
{
    FixVector tmp;
    FixVector vA;
    FixVector imp;
    FixVector off;
    FixVector spin;
    FixVector vPos;
    int flagA;
    int flagB;
    int flagC;
    int impulse;
    int dot;
    int i;

    flagA = 0;
    flagB = 0;
    flagC = 0;
    pCar->field_0x5dc.y = 0;
    Collision_ClampContactOffset((int *)&pCar->field_0x5dc, (int *)&pCar->halfExtents, 1);

    FixVecCross(&tmp, &pCar->angularVelocity, &pCar->field_0x5dc);
    FixMatrix_RotateVector(&vA, &tmp, pCar->pWorld);
    vA.x += pCar->velocity.x;
    vA.y += pCar->velocity.y;
    vA.z += pCar->velocity.z;

    if (pCar->field_0xb64 == 0 && pCar->useUpperCollisionCorners == 0) {
        dot = FIX_ABS(FixVecDot(&vA, &g_collisionContactNormal));
        if (dot > g_contactImpactHighThreshold) {
            flagA = 1;
            flagB = 1;
        } else if (dot > g_contactImpactLowThreshold) {
            flagA = 1;
            flagB = 0;
        }
    }

    dot = FixVecDot(&vA, &g_collisionContactNormal);
    impulse = FixMul(dot, 0) - dot;
    if (((*ppObject)->flags & 0x2001000) != 0 && FIX_ABS(impulse) > 0) {
        flagC = 1;
        flagB = 0;
        impulse = FixMul(impulse, 0x28f);
    }

    FixVecScale(&imp, &g_collisionContactNormal, impulse);
    FixVecScale(&imp, &imp, g_physicsScale);

    FixMatrix_InverseRotateVector(&tmp, &imp, pCar->pWorld);
    pCar->field_0x5c4.x += imp.x;
    pCar->field_0x5c4.y += imp.y;
    pCar->field_0x5c4.z += imp.z;

    for (i = 0; i < pCar->previousContactCount; i++) {
        if (pCar->contactSurfaces[0][i] == contactId)
            goto known;
    }
    if (g_collisionObjectHasBox != 0) {
        if (g_unk0x005915f4 != 0) {
            CarDamage_ApplyCollisionDeformImpulse(pCar,
                         (int *)&pCar->corners[g_unk0x005914c4[0]],
                         &g_collisionContactNormal, 0, 0, 0);
        } else if (g_unk0x005914d4 != 0) {
            CarDamage_ApplyCollisionDeformImpulse(pCar,
                         (int *)&g_sectorObjectCollisionBox.points[g_unk0x00590ecc[0]],
                         &g_collisionContactNormal, 0, 2, 0);
        }
    } else {
        CarDamage_ApplyCollisionDeformImpulse(pCar, (int *)&g_collisionSphereCentre, &g_collisionContactNormal,
                     g_collisionSphereRadius, 1, 0);
    }
known:

    FixMatrix_RotateVector(&vPos, &pCar->field_0x5dc, pCar->pWorld);
    vPos.x += pCar->position.x;
    vPos.y += pCar->position.y;
    vPos.z += pCar->position.z;

    if (flagC == 0 && flagA != 0) {
        pCar->field_0x96c = 0x10000;
        pCar->useUpperCollisionCorners = 1;
        pCar->field_0xc04[0] = 1;
    }
    pCar->field_0x5dc.y = 0;

    if (flagA == 0) {
        FixVecCross(&spin, &tmp, &pCar->field_0x5dc);
        if (g_collisionObjectHasBox != 0)
            FixVecScale(&spin, &spin, 0x18000);
        else
            FixVecScale(&spin, &spin, 0x30000);
        pCar->field_0x5d0.x += spin.x;
        pCar->field_0x5d0.y += spin.y;
        pCar->field_0x5d0.z += spin.z;
    } else {
        if (flagB != 0) {
            off.x = -pCar->right.y;
            off.y = -pCar->up.y;
            off.z = -pCar->forward.y;
            FixVecScale(&off, &off, 0x40000);
            off.x += pCar->field_0x5dc.x;
            off.y += pCar->field_0x5dc.y;
            off.z += pCar->field_0x5dc.z;
            FixVecCross(&spin, &tmp, &off);
            FixVecScale(&spin, &spin, 0x40000);
            pCar->velocity.y += 0x4000;
        } else {
            off.x = -pCar->right.y;
            off.y = -pCar->up.y;
            off.z = -pCar->forward.y;
            FixVecScale(&off, &off, 0x10000);
            off.x += pCar->field_0x5dc.x;
            off.y += pCar->field_0x5dc.y;
            off.z += pCar->field_0x5dc.z;
            FixVecCross(&spin, &tmp, &off);
            FixVecScale(&spin, &spin, 0x40000);
        }
        pCar->field_0x5d0.x += spin.x;
        pCar->field_0x5d0.y += spin.y;
        pCar->field_0x5d0.z += spin.z;
    }
    Collision_ClampAngularCorrection((int *)&pCar->field_0x5d0);

    if (pCar->pendingContactCount < 5) {
        pCar->contactSurfaces[1][pCar->pendingContactCount] = (short)contactId;
        pCar->pendingContactCount = pCar->pendingContactCount + 1;
    }

    if (CGameInfo::IsActiveCheatEnabled(4) != 0) {
        pCar->field_0x5c4.x -= imp.x;
        pCar->field_0x5c4.y -= imp.y;
        pCar->field_0x5c4.z -= imp.z;
        pCar->velocity.x += imp.x * 2;
        pCar->velocity.y += imp.y * 2;
        pCar->velocity.z += imp.z * 2;
    }
    return flagC;
}
