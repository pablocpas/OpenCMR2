#include <stdlib.h>
#include "FloatToInt.h"
#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include "SceneNode.h"
#include "FileBuffer.h"
#include "Graphics.h"
#include "Game.h"
#include "Sector.h"
#include "Mesh.h"
#include "MeshVertex.h"
#include "ShadowPart.h"
#include "LightZone.h"

SceneNode *g_sceneNodes[4096];
int g_sceneNodeCount;

// Per-frame statistics of SceneNode_UpdateTree
// GLOBAL: CMR2 0x00683374
int g_sceneStatCopied;
// GLOBAL: CMR2 0x00683378
int g_sceneStatMultiplied;
// GLOBAL: CMR2 0x0068337c
int g_sceneStatClean;
// GLOBAL: CMR2 0x00683380
int g_sceneStatHidden;

unsigned short g_sqrtTable[4096];
int g_sinTable[4096];
int g_tanTable[4096];
unsigned short g_atanTable[512];

// Rodrigues rotation scratch values (shared globals in the original)
// GLOBAL: CMR2 0x0067f250
int g_rotSin;
// GLOBAL: CMR2 0x0067f248
int g_rotCos;
// GLOBAL: CMR2 0x0068336c
int g_rotOneMinusCos;
// GLOBAL: CMR2 0x0067f254
int g_rotAxisXX;
// GLOBAL: CMR2 0x0067f258
int g_rotAxisYY;
// GLOBAL: CMR2 0x0067f25c
int g_rotAxisZZ;
// GLOBAL: CMR2 0x0067f238
int g_rotAxisXY;
// GLOBAL: CMR2 0x0067f244
int g_rotAxisXZ;
// GLOBAL: CMR2 0x0067f24c
int g_rotAxisYZ;

inline void SceneNode_NormalizeInto(FixVector *out, FixVector *v)
{
    FixVector *dest = out;
    int len = FixVecLength(v);

    if (len == 0) {
        dest->x = 0;
        dest->y = 0;
        dest->z = 0;
    } else {
        FixVecScaleRecip(dest, v, len);
    }
}

#define SCENENODE_NORMALIZE(out, src) SceneNode_NormalizeInto(&(out), &(src))

// Rotates vectors A and B about the unit axis K by angle (Rodrigues) and
// renormalises them. The original expands this three times over locals, so
// it is a macro rather than a function; the rotation is built as a full
// 4x4 matrix. The right-axis expansion uses the pointer normalizer for both
// output vectors; the other two expand the first normalization directly.
#define ROTATE_ABOUT_AXIS(kx, ky, kz, a, b, angle, normalize_a)                       \
    {                                                                                 \
        g_rotSin = FixSin(-(angle));                                                  \
        g_rotCos = FixCos(angle);                                                     \
        g_rotOneMinusCos = 0x10000 - g_rotCos;                                        \
        g_rotAxisXX = FixMul(kx, kx);                                                 \
        g_rotAxisYY = FixMul(ky, ky);                                                 \
        g_rotAxisZZ = FixMul(kz, kz);                                                 \
        g_rotAxisXY = FixMul(FixMul(kx, ky), g_rotOneMinusCos);                       \
        g_rotAxisXZ = FixMul(FixMul(kx, kz), g_rotOneMinusCos);                       \
        g_rotAxisYZ = FixMul(FixMul(kz, ky), g_rotOneMinusCos);                       \
        m.right.x = FixMul(g_rotCos, 0x10000 - g_rotAxisXX) + g_rotAxisXX;            \
        m.right.y = g_rotAxisXY - FixMul(kz, g_rotSin);                               \
        m.right.z = FixMul(ky, g_rotSin) + g_rotAxisXZ;                               \
        m.up.x = FixMul(kz, g_rotSin) + g_rotAxisXY;                                  \
        m.up.y = FixMul(g_rotCos, 0x10000 - g_rotAxisYY) + g_rotAxisYY;               \
        m.up.z = g_rotAxisYZ - FixMul(kx, g_rotSin);                                  \
        m.forward.x = g_rotAxisXZ - FixMul(ky, g_rotSin);                             \
        m.forward.y = FixMul(kx, g_rotSin) + g_rotAxisYZ;                             \
        m.forward.z = FixMul(g_rotCos, 0x10000 - g_rotAxisZZ) + g_rotAxisZZ;          \
        m.position.x = 0;                                                             \
        m.position.y = 0;                                                             \
        m.position.z = 0;                                                             \
        m.rw = 0;                                                                     \
        m.uw = 0;                                                                     \
        m.fw = 0;                                                                     \
        m.pw = 0x10000;                                                               \
        v.x = FixMul(m.right.x, a.x) + FixMul(m.up.x, a.y) + FixMul(m.forward.x, a.z); \
        v.y = FixMul(m.right.y, a.x) + FixMul(m.up.y, a.y) + FixMul(m.forward.y, a.z); \
        v.z = FixMul(m.right.z, a.x) + FixMul(m.up.z, a.y) + FixMul(m.forward.z, a.z); \
        normalize_a(a, v);                                                           \
        v.x = FixMul(m.right.x, b.x) + FixMul(m.up.x, b.y) + FixMul(m.forward.x, b.z); \
        v.y = FixMul(m.right.y, b.x) + FixMul(m.up.y, b.y) + FixMul(m.forward.y, b.z); \
        v.z = FixMul(m.right.z, b.x) + FixMul(m.up.z, b.y) + FixMul(m.forward.z, b.z); \
        SceneNode_NormalizeInto(&b, &v);                                              \
    }

// FUNCTION: CMR2 0x004ac820
void SceneNode_Rotate(SceneNode *pNode, FixVector *pTranslation, FixAngles *pAngles)
{
    // The axes are rescaled in place through a FixVector pointer, so they must
    // be real vectors: three loose ints are not guaranteed to be contiguous
    // (writing 12 bytes from &rx overwrote the return address).
    FixBasis basis;
    FixMatrix m;
    FixVector v;
    SceneNode *p;

    basis.right.x = pNode->current.right.x;
    basis.right.y = pNode->current.right.y;
    basis.right.z = pNode->current.right.z;
    basis.up.x = pNode->current.up.x;
    basis.up.y = pNode->current.up.y;
    basis.up.z = pNode->current.up.z;
    basis.forward.x = pNode->current.forward.x;
    basis.forward.y = pNode->current.forward.y;
    basis.forward.z = pNode->current.forward.z;

    if (pAngles->y != 0)
        ROTATE_ABOUT_AXIS(basis.up.x, basis.up.y, basis.up.z, basis.right, basis.forward, pAngles->y, FIX_NORMALIZE_INTO)
    if (pAngles->z != 0)
        ROTATE_ABOUT_AXIS(basis.forward.x, basis.forward.y, basis.forward.z, basis.right, basis.up, pAngles->z, FIX_NORMALIZE_INTO)
    if (pAngles->x != 0)
        ROTATE_ABOUT_AXIS(basis.right.x, basis.right.y, basis.right.z, basis.up, basis.forward, pAngles->x, SCENENODE_NORMALIZE)

    pNode->current.right.x = basis.right.x;
    pNode->current.right.y = basis.right.y;
    pNode->current.right.z = basis.right.z;
    pNode->current.up.x = basis.up.x;
    pNode->current.up.y = basis.up.y;
    pNode->current.up.z = basis.up.z;
    pNode->current.forward.x = basis.forward.x;
    pNode->current.forward.y = basis.forward.y;
    pNode->current.forward.z = basis.forward.z;
    pNode->current.position.x += pTranslation->x;
    pNode->current.position.y += pTranslation->y;
    pNode->current.position.z += pTranslation->z;
    for (p = pNode; p != NULL; p = p->pParent)
        p->dirty = 1;
}

// FUNCTION: CMR2 0x004ac140
void SceneNode_Init(SceneNode *pNode)
{
    SceneNode *p;

    pNode->pNext = NULL;
    pNode->pFirstChild = NULL;
    pNode->type = SCENE_NODE_EMPTY;
    pNode->pObject = NULL;
    pNode->viewMask = 0xff;
    pNode->pParent = NULL;
    pNode->sector = -1;
    pNode->pNextInSector = NULL;
    pNode->visible = 1;
    for (p = pNode; p != NULL; p = p->pParent)
        p->dirty = 1;
    pNode->useParentWorld = 0;
    pNode->colour[0] = 0x28;
    pNode->colour[1] = 0x28;
    pNode->colour[2] = 0x28;
    pNode->colour[3] = 0xff;
    pNode->translation.x = 0;
    pNode->translation.y = 0;
    pNode->translation.z = 0;
    pNode->angles.x = 0;
    pNode->angles.y = 0;
    pNode->angles.z = 0;
    pNode->allocated = 1;
    FixMatrix_Identity(&pNode->local);
    FixMatrix_Identity(&pNode->current);
    FixMatrix_Identity(&pNode->world);
}

// FUNCTION: CMR2 0x004ac1f0
int SceneNode_Unused(SceneNode *pNode)
{
    return 1;
}

// Appends pNode to pParent's child list (or resets its matrices when it
// is a root).
// FUNCTION: CMR2 0x004ac200
void SceneNode_Attach(SceneNode *pNode, SceneNode *pParent)
{
    SceneNode *p;

    if (pParent != NULL) {
        if (pParent->pFirstChild != NULL) {
            for (p = pParent->pFirstChild; p->pNext != NULL; p = p->pNext)
                ;
            p->pNext = pNode;
            pNode->pParent = pParent;
            return;
        }
        pParent->pFirstChild = pNode;
        pNode->pParent = pParent;
        return;
    }
    FixMatrix_Identity(&pNode->world);
    FixMatrix_Identity(&pNode->current);
}

// FUNCTION: CMR2 0x004ac260
void SceneNode_SetObject(SceneNode *pNode, int type, void *pObject)
{
    pNode->type = type;
    pNode->pObject = pObject;
}

// Recomputes the world matrices of pNode, its siblings and their subtrees.
// FUNCTION: CMR2 0x004ac280
void SceneNode_UpdateTree(SceneNode *pNode, int unused)
{
    SceneNode *pParent;
    SceneNode *p;

    while (pNode != NULL) {
        if (SceneNode_IsVisible(pNode)) {
            pParent = pNode->pParent;
            if (pParent != NULL && pNode->type >= 0 && pNode->type <= 3) {
                if (pNode->dirty == 1 || pParent->dirty == 1) {
                    if ((char)pParent->flags != -3) {
                        if (pNode->useParentWorld != 0) {
                            pNode->world = pNode->pParent->world;
                            g_sceneStatCopied++;
                        } else {
                            FixMatrix_Multiply(&pNode->world, &pNode->current, &pParent->world);
                            g_sceneStatMultiplied++;
                        }
                    } else {
                        pNode->world = pNode->current;
                        g_sceneStatCopied++;
                    }
                    if (pNode->sector != -1 && (char)pNode->flags != -3)
                        SceneNode_UpdateSector(pNode);
                    for (p = pNode; p != NULL; p = p->pParent)
                        p->dirty = 1;
                } else {
                    g_sceneStatClean++;
                }
            }
            if (pNode->pFirstChild != NULL)
                SceneNode_UpdateTree(pNode->pFirstChild, unused);
        } else {
            g_sceneStatHidden++;
        }
        pNode = pNode->pNext;
    }
}

// FUNCTION: CMR2 0x004ac390
int SceneNode_IsVisible(SceneNode *pNode)
{
    if (pNode->sector >= 0 && pNode->visible != 0 && (pNode->flags & 0x1000) == 0)
        return Sector_IsVisible(pNode->sector);
    return 1;
}

// FUNCTION: CMR2 0x004ac3d0
void SceneNode_SetTransform(SceneNode *pNode, FixVector *pTranslation, FixAngles *pAngles)
{
    if (pNode == NULL)
        return;
    switch (pNode->type) {
    case SCENE_NODE_MESH:
    case SCENE_NODE_EMPTY:
        pNode->current = pNode->local;
        SceneNode_Rotate(pNode, pTranslation, pAngles);
        break;
    case SCENE_NODE_TYPE1:
    case SCENE_NODE_TYPE2:
        pNode->translation = *pTranslation;
        pNode->angles = *pAngles;
        pNode->current = pNode->local;
        SceneNode_Rotate(pNode, pTranslation, pAngles);
        break;
    }
    pNode->useParentWorld = 0;
    do {
        pNode->dirty = 1;
        pNode = pNode->pParent;
    } while (pNode != NULL);
}

// FUNCTION: CMR2 0x004ac480
void SceneNode_SetPosition(SceneNode *pNode, FixVector *pPosition)
{
    if (pNode != NULL) {
        if (pNode->type > 0 && pNode->type <= 2)
            pNode->translation = *pPosition;
        pNode->current.position = *pPosition;
        pNode->useParentWorld = 0;
        do {
            pNode->dirty = 1;
            pNode = pNode->pParent;
        } while (pNode != NULL);
    }
}

// FUNCTION: CMR2 0x004ac4f0
void SceneNode_SetRotation(SceneNode *pNode, FixAngles *pAngles)
{
    FixVector zero;

    zero.x = 0;
    zero.y = 0;
    zero.z = 0;
    if (pNode == NULL)
        return;
    switch (pNode->type) {
    case SCENE_NODE_MESH:
    case SCENE_NODE_EMPTY:
        pNode->current.right.x = pNode->local.right.x;
        pNode->current.right.y = pNode->local.right.y;
        pNode->current.right.z = pNode->local.right.z;
        pNode->current.up.x = pNode->local.up.x;
        pNode->current.up.y = pNode->local.up.y;
        pNode->current.up.z = pNode->local.up.z;
        pNode->current.forward.x = pNode->local.forward.x;
        pNode->current.forward.y = pNode->local.forward.y;
        pNode->current.forward.z = pNode->local.forward.z;
        SceneNode_Rotate(pNode, &zero, pAngles);
        break;
    case SCENE_NODE_TYPE1:
    case SCENE_NODE_TYPE2:
        pNode->current.right.x = pNode->local.right.x;
        pNode->current.right.y = pNode->local.right.y;
        pNode->current.right.z = pNode->local.right.z;
        pNode->current.up.x = pNode->local.up.x;
        pNode->current.up.y = pNode->local.up.y;
        pNode->current.up.z = pNode->local.up.z;
        pNode->current.forward.x = pNode->local.forward.x;
        pNode->current.forward.y = pNode->local.forward.y;
        pNode->current.forward.z = pNode->local.forward.z;
        SceneNode_Rotate(pNode, &zero, pAngles);
        break;
    }
    pNode->useParentWorld = 0;
    do {
        pNode->dirty = 1;
        pNode = pNode->pParent;
    } while (pNode != NULL);
}

// Destroys pNode and its subtree, unlinking it from its parent.
// FUNCTION: CMR2 0x004ac620
int SceneNode_Destroy(SceneNode *pNode)
{
    SceneNode *pChild;
    SceneNode *pNext;
    SceneNode *pParent;
    SceneNode *p;

    pChild = pNode->pFirstChild;
    while (pChild != NULL) {
        pNext = pChild->pNext;
        SceneNode_Destroy(pChild);
        pChild = pNext;
    }
    pParent = pNode->pParent;
    if (pParent != NULL) {
        p = pParent->pFirstChild;
        if (p == pNode) {
            pParent->pFirstChild = pNode->pNext;
        } else {
            while (p->pNext != pNode)
                p = p->pNext;
            p->pNext = pNode->pNext;
        }
    }
    SceneNode_Free(pNode);
    return 1;
}

// FUNCTION: CMR2 0x004ac680
void SceneNode_Free(SceneNode *pNode)
{
    void *pObject;

    pObject = pNode->pObject;
    if (pObject != NULL) {
        switch (pNode->type) {
        case SCENE_NODE_MESH:
            Mesh_Free(pObject);
            break;
        case SCENE_NODE_TYPE1:
            Scene_FreeType1Object(pObject);
            break;
        case SCENE_NODE_TYPE2:
            Scene_FreeType2Object(pObject);
            break;
        }
    }
    g_sceneNodes[pNode->slot] = NULL;
    g_sceneNodeCount--;
    if (pNode->allocated != 0)
        CFileBuffer::FreeGenericFileBuffer(pNode);
}

// FUNCTION: CMR2 0x004ac6f0
SceneNode *SceneNode_Create(SceneNode *pParent)
{
    SceneNode *pNode;
    int i;

    for (i = 0; i < sizeof(g_sceneNodes) / sizeof(g_sceneNodes[0]); i++) {
        if (g_sceneNodes[i] == NULL) {
            pNode = (SceneNode *)CFileBuffer::AllocateLockedBuffer(sizeof(SceneNode));
            if (pNode == NULL)
                return NULL;
            SceneNode_Init(pNode);
            SceneNode_Attach(pNode, pParent);
            g_sceneNodes[i] = pNode;
            pNode->slot = (short)g_sceneNodeCount;
            g_sceneNodeCount++;
            return g_sceneNodes[i];
        }
    }
    return NULL;
}

// Moves pNode (and its subtree) under pNewParent.
// match 98%: the original returns the dirty value from ECX without reloading it.
// FUNCTION: CMR2 0x004ac7a0
int SceneNode_Reparent(SceneNode *pNode, SceneNode *pNewParent)
{
    SceneNode *p;

    if (pNode == NULL)
        return 0;
    if (pNewParent == pNode->pParent)
        return 0;
    if (pNewParent->pParent == pNode)
        return 0;
    p = pNode->pParent->pFirstChild;
    if (p == pNode) {
        pNode->pParent->pFirstChild = pNode->pNext;
    } else {
        while (p->pNext != pNode) {
            p = p->pNext;
            if (p == NULL)
                return 0;
        }
        p->pNext = pNode->pNext;
    }
    pNode->pParent = pNewParent;
    if (pNewParent->pFirstChild == NULL) {
        pNewParent->pFirstChild = pNode;
        pNode->pNext = NULL;
    } else {
        pNode->pNext = pNewParent->pFirstChild;
        pNewParent->pFirstChild = pNode;
    }
    p = pNode;
    do {
        p->dirty = 1;
        p = p->pParent;
    } while (p != NULL);
    return 1;
}

// Type 1 / type 2 scene objects: fixed pointer tables, released by lookup.
// GLOBAL: CMR2 0x006dffa4
void *g_sceneType1Objects[60];
// Colour and strength of the car shadows.
// GLOBAL: CMR2 0x006e0098
DWORD g_shadowColour;
// GLOBAL: CMR2 0x006e0b38
int g_shadowLevel;
// GLOBAL: CMR2 0x006e0b48
int g_sceneType1Count;
// GLOBAL: CMR2 0x00683388
void *g_sceneType2Objects[256];
// GLOBAL: CMR2 0x006838cc
int g_sceneType2Count;

// FUNCTION: CMR2 0x004b3480
void Scene_FreeType1Object(void *pObject)
{
    int i;

    for (i = 0; i < 60; i++) {
        if (g_sceneType1Objects[i] != NULL && g_sceneType1Objects[i] == pObject) {
            g_sceneType1Objects[i] = NULL;
            CFileBuffer::FreeGenericFileBuffer(pObject);
            g_sceneType1Count--;
            return;
        }
    }
    g_sceneType1Count--;
}

// A Direct3D light plus the device light index it occupies.
struct SceneLight {
    GfxLight light;
    int index;
};

// GLOBAL: CMR2 0x006e0b9c
int g_sceneLightCallbackSet;

// Release callback: frees every light still allocated.
// FUNCTION: CMR2 0x004b3450
int Scene_FreeAllLights(void)
{
    int i;

    g_sceneLightCallbackSet = 0;
    for (i = 0; i < 60; i++) {
        if (g_sceneType1Objects[i] != NULL)
            Scene_FreeType1Object(g_sceneType1Objects[i]);
    }
    return 1;
}

// Creates a Direct3D light (0 point, 1 spot, 2 directional, 3 parallel point) in the
// first free slot and optionally hangs it from a scene node placed at pPosition.
// Colour components and position are 16.16 fixed point.
// FUNCTION: CMR2 0x004b2f80
SceneNode *Scene_CreateLight(int type, int r, int g, int b, FixVector *pPosition, FixAngles *pAngles,
                             SceneNode *pParent)
{
    SceneNode *pNode;
    int index;
    float fr, fg, fb;
    FixVector dir;
    SceneLight *pLight;

    if (g_sceneLightCallbackSet == 0) {
        CGame::RegisterCallback(Scene_FreeAllLights, NULL);
        g_sceneLightCallbackSet = 1;
    }
    for (index = 0; index < 60; index++) {
        if (g_sceneType1Objects[index] == NULL)
            goto found;
    }
    return NULL;

found:

    pLight = (SceneLight *)CFileBuffer::AllocateLockedBuffer(sizeof(SceneLight));
    g_sceneType1Objects[index] = pLight;
    fr = X87S_MUL(X87S_FILD(float, r), CGraphics::m_oneOver65536);
    memset(pLight, 0, sizeof(GfxLight));
    fg = X87S_MUL(X87S_FILD(float, g), CGraphics::m_oneOver65536);
    ((SceneLight *)g_sceneType1Objects[index])->index = index;
    fb = X87S_MUL(X87S_FILD(float, b), CGraphics::m_oneOver65536);
    ((SceneLight *)g_sceneType1Objects[index])->light.diffuse.r = fr;
    ((SceneLight *)g_sceneType1Objects[index])->light.diffuse.g = fg;
    ((SceneLight *)g_sceneType1Objects[index])->light.diffuse.b = fb;
    ((SceneLight *)g_sceneType1Objects[index])->light.diffuse.a = 1.0f / 65536.0f;
    ((SceneLight *)g_sceneType1Objects[index])->light.specular.r = 0.0f;
    ((SceneLight *)g_sceneType1Objects[index])->light.specular.g = 0.0f;
    ((SceneLight *)g_sceneType1Objects[index])->light.specular.b = 0.0f;
    ((SceneLight *)g_sceneType1Objects[index])->light.specular.a = 0.0f;
    ((SceneLight *)g_sceneType1Objects[index])->light.ambient.r = 0.0f;
    ((SceneLight *)g_sceneType1Objects[index])->light.ambient.g = 0.0f;
    ((SceneLight *)g_sceneType1Objects[index])->light.ambient.b = 0.0f;
    ((SceneLight *)g_sceneType1Objects[index])->light.ambient.a = 0.0f;
    ((SceneLight *)g_sceneType1Objects[index])->light.position.x = X87S_MUL(X87S_FILD(float, pPosition->x), CGraphics::m_oneOver65536);
    ((SceneLight *)g_sceneType1Objects[index])->light.position.y = X87S_MUL(X87S_FILD(float, pPosition->y), CGraphics::m_oneOver65536);
    ((SceneLight *)g_sceneType1Objects[index])->light.position.z = X87S_MUL(X87S_FILD(float, pPosition->z), CGraphics::m_oneOver65536);
    ((SceneLight *)g_sceneType1Objects[index])->light.attenuation0 = 1.0f;
    ((SceneLight *)g_sceneType1Objects[index])->light.attenuation1 = 0.0f;
    ((SceneLight *)g_sceneType1Objects[index])->light.attenuation2 = 0.0f;
    ((SceneLight *)g_sceneType1Objects[index])->light.direction.x = 0.0f;
    ((SceneLight *)g_sceneType1Objects[index])->light.direction.y = 0.0f;
    ((SceneLight *)g_sceneType1Objects[index])->light.direction.z = 1.0f;
    ((SceneLight *)g_sceneType1Objects[index])->light.falloff = 1.0f;
    ((SceneLight *)g_sceneType1Objects[index])->light.theta = 0.5f;
    ((SceneLight *)g_sceneType1Objects[index])->light.phi = 1.0f;

    switch (type) {
    case 0:
        ((SceneLight *)g_sceneType1Objects[index])->light.type = GFX_LIGHT_POINT;
        ((SceneLight *)g_sceneType1Objects[index])->light.ambient.r = fr;
        ((SceneLight *)g_sceneType1Objects[index])->light.ambient.g = fg;
        ((SceneLight *)g_sceneType1Objects[index])->light.ambient.b = fb;
        ((SceneLight *)g_sceneType1Objects[index])->light.ambient.a = 1.0f / 65536.0f;
        ((SceneLight *)g_sceneType1Objects[index])->light.range = 60.0f;
        ((SceneLight *)g_sceneType1Objects[index])->light.attenuation0 = 0.0f;
        ((SceneLight *)g_sceneType1Objects[index])->light.attenuation1 = 0.0f;
        ((SceneLight *)g_sceneType1Objects[index])->light.attenuation2 = 0.1f;
        break;
    case 1:
        ((SceneLight *)g_sceneType1Objects[index])->light.type = GFX_LIGHT_SPOT;
        ((SceneLight *)g_sceneType1Objects[index])->light.ambient.r = fr;
        ((SceneLight *)g_sceneType1Objects[index])->light.ambient.g = fg;
        ((SceneLight *)g_sceneType1Objects[index])->light.ambient.b = fb;
        ((SceneLight *)g_sceneType1Objects[index])->light.ambient.a = 1.0f / 65536.0f;
        ((SceneLight *)g_sceneType1Objects[index])->light.range = 100.0f;
        ((SceneLight *)g_sceneType1Objects[index])->light.falloff = 1.0f;
        ((SceneLight *)g_sceneType1Objects[index])->light.theta = 0.2f;
        ((SceneLight *)g_sceneType1Objects[index])->light.phi = 2.0f;
        ((SceneLight *)g_sceneType1Objects[index])->light.attenuation0 = 0.0f;
        ((SceneLight *)g_sceneType1Objects[index])->light.attenuation1 = 0.0f;
        ((SceneLight *)g_sceneType1Objects[index])->light.attenuation2 = 0.0f;
        break;
    case 2:
        ((SceneLight *)g_sceneType1Objects[index])->light.type = GFX_LIGHT_DIRECTIONAL;
        dir.x = -pPosition->x;
        dir.y = -pPosition->y;
        dir.z = -pPosition->z;
        FIX_NORMALIZE_INTO(dir, dir);
        ((SceneLight *)g_sceneType1Objects[index])->light.direction.x = X87S_MUL(X87S_FILD(float, dir.x), CGraphics::m_oneOver65536);
        ((SceneLight *)g_sceneType1Objects[index])->light.direction.y = X87S_MUL(X87S_FILD(float, dir.y), CGraphics::m_oneOver65536);
        ((SceneLight *)g_sceneType1Objects[index])->light.direction.z = X87S_MUL(X87S_FILD(float, dir.z), CGraphics::m_oneOver65536);
        break;
    case 3:
        ((SceneLight *)g_sceneType1Objects[index])->light.type = GFX_LIGHT_PARALLELPOINT;
        break;
    }

    Gfx_SetLight(index, &((SceneLight *)g_sceneType1Objects[index])->light);
    Gfx_EnableLight(index, TRUE);
    pNode = NULL;
    if (pParent != NULL) {
        pNode = SceneNode_Create(pParent);
        SceneNode_SetObject(pNode, 1, g_sceneType1Objects[index]);
        SceneNode_SetTransform(pNode, pPosition, pAngles);
    }
    g_sceneType1Count++;
    return pNode;
}

// FUNCTION: CMR2 0x004adf60
void Scene_FreeType2Object(void *pObject)
{
    int i;
    int count;

    count = 0;
    for (i = 0; i < 256; i++) {
        if (g_sceneType2Objects[i] != NULL && g_sceneType2Objects[i] == pObject) {
            g_sceneType2Objects[i] = NULL;
            count++;
        }
    }
    if (count > 0) {
        CFileBuffer::FreeGenericFileBuffer(pObject);
        g_sceneType2Count--;
    }
}

// Creates the root node of the scene graph and marks it as the root.
// FUNCTION: CMR2 0x004ac760
SceneNode *SceneNode_CreateRoot(void)
{
    CGraphics::m_pTextureManager->pRootNode = SceneNode_Create(NULL);
    CGraphics::m_pTextureManager->pRootNode->flags =
        (CGraphics::m_pTextureManager->pRootNode->flags & 0xfffffffd) | 0xfd;
    return CGraphics::m_pTextureManager->pRootNode;
}

// GLOBAL: CMR2 0x006838c8
int g_sceneType2CallbackRegistered;

// Exit callback: releases every type 2 object still registered.
// FUNCTION: CMR2 0x004ae070
int SceneType2_ReleaseAll(void)
{
    int i;

    for (i = 0; i < 256; i++) {
        if (g_sceneType2Objects[i] != NULL)
            Scene_FreeType2Object(g_sceneType2Objects[i]);
    }
    g_sceneType2CallbackRegistered = 0;
    return 1;
}

// Allocates a type 2 object (0x104 bytes) and attaches it to pNode, creating
// the node under pParent (with the given transform) when none is passed.
// FUNCTION: CMR2 0x004adfa0
SceneNode *SceneType2_Create(FixVector *pTranslation, FixAngles *pAngles, SceneNode *pNode, SceneNode *pParent)
{
    SceneNode *p;
    void **pSlot;
    void *pObject;
    int i;
    unsigned int flags;

    if (g_sceneType2CallbackRegistered == 0) {
        CGame::RegisterCallback(SceneType2_ReleaseAll, NULL);
        g_sceneType2CallbackRegistered = 1;
    }
    if (pNode == NULL)
        p = SceneNode_Create(pParent);
    else
        p = pNode;
    i = 0;
    p->flags |= 0xff;
    for (i = 0; i < 256; i++) {
        if (g_sceneType2Objects[i] == NULL) {
            pObject = CFileBuffer::AllocateLockedBuffer(0x104);
            g_sceneType2Objects[i] = pObject;
            SceneNode_SetObject(p, SCENE_NODE_TYPE2, pObject);
            if (pNode == NULL) {
                p->translation.x = pTranslation->x;
                p->translation.y = pTranslation->y;
                p->translation.z = pTranslation->z;
                p->angles.x = pAngles->x;
                p->angles.y = pAngles->y;
                p->angles.z = pAngles->z;
                SceneNode_SetTransform(p, pTranslation, pAngles);
            }
            g_sceneType2Count++;
            return p;
        }
    }
    return NULL;
}

// FUNCTION: CMR2 0x004b4a80
void Scene_GetShadowColour(DWORD *pColour, int *pLevel)
{
    *pColour = g_shadowColour;
    *pLevel = g_shadowLevel;
}

// Scene lighting: ambient plus a light colour scaled by 50 levels.
// GLOBAL: CMR2 0x006e0118
FixVector g_sceneAmbient;
// GLOBAL: CMR2 0x006e0348
FixVector g_sceneLightColour;
// GLOBAL: CMR2 0x006e0244
int g_sceneLightDirty;
// GLOBAL: CMR2 0x006e0b34
BYTE *g_sceneLightTable;            // 50 RGBA colours
// GLOBAL: CMR2 0x006dfdc8
DWORD *g_sceneLightTableD3D;        // the same as D3D ARGB
// GLOBAL: CMR2 0x006dfd9c
DWORD *g_sceneShadowTable;          // shadow colour at each level
// GLOBAL: CMR2 0x006deacc
DWORD *g_sceneShadowTableD3D;

// Rebuilds the light and shadow colour tables.
// FUNCTION: CMR2 0x004b3940
void Scene_BuildLightTables(void)
{
    FixVector c;
    BYTE *p;
    DWORD *pShadow;
    int level;
    int step;
    int i;
    int v;
    BYTE alpha;

    if (g_sceneLightTable != NULL && g_sceneLightTableD3D != NULL) {
        step = FixDiv(0x10000, 0x310000);
        level = 0;
        p = g_sceneLightTable;
        i = 0;
        pShadow = g_sceneShadowTable;
        do {
            FixVecScale(&c, &g_sceneLightColour, level);
            c.x += g_sceneAmbient.x;
            c.y += g_sceneAmbient.y;
            c.z += g_sceneAmbient.z;
            v = c.x >> 16;
            if (v < 0)
                v = 0;
            else if (v > 0xff)
                v = 0xff;
            p[0] = (BYTE)v;
            v = c.y >> 16;
            if (v < 0)
                v = 0;
            else if (v > 0xff)
                v = 0xff;
            p[1] = (BYTE)v;
            v = c.z >> 16;
            if (v < 0)
                v = 0;
            else if (v > 0xff)
                v = 0xff;
            p[2] = (BYTE)v;
            p[3] = 0xff;
            g_sceneLightTableD3D[i] = ((p[0] | 0xffffff00) << 8 | p[1]) << 8 | (v & 0xff);
            *pShadow = g_shadowColour;
            alpha = (BYTE)FixMulShift32(g_shadowLevel, level);
            ((BYTE *)pShadow)[3] = alpha;
            g_sceneShadowTableD3D[i] =
                ((((DWORD)alpha << 8 | ((BYTE *)pShadow)[0]) << 8) | ((BYTE *)pShadow)[1]) << 8 | ((BYTE *)pShadow)[2];
            level += step;
            p += 4;
            i++;
            pShadow++;
        } while (i < 50);
    }
}

// Light colour at a level (0..1.0).
// FUNCTION: CMR2 0x004b3ae0
void Scene_GetLightColour(DWORD *pColour, int level)
{
    int i;

    if (g_sceneLightDirty != 0) {
        Scene_BuildLightTables();
        g_sceneLightDirty = 0;
    }
    i = level * 0x31 >> 16;
    if (i < 0)
        i = 0;
    else if (i > 0x31)
        i = 0x31;
    *pColour = ((DWORD *)g_sceneLightTable)[i];
}

// Light colour of the scene (absolute, 16.16 per channel).
// GLOBAL: CMR2 0x006e01a0
FixVector g_sceneLight;
// GLOBAL: CMR2 0x006e01ec
BYTE g_sceneAmbientColour[4];       // last ambient colour set
// GLOBAL: CMR2 0x006e020c
DWORD g_sceneAmbientD3D;
// GLOBAL: CMR2 0x006dfdc4
BYTE g_sceneLightColourBytes[4];

// Derives the shadow colour and strength from the contrast between the
// ambient and the light: the shadow is tinted by the ambient minus its
// weakest channel, or with boost set, a darker copy of the ambient and a
// stronger shadow.
// FUNCTION: CMR2 0x004b4910
void Scene_UpdateShadowColour(int boost)
{
    FixVector c;
    int r;
    int g;
    int b;

    g_shadowLevel = FIX_ABS(g_sceneAmbient.x - g_sceneLight.x);
    g_shadowLevel += FIX_ABS(g_sceneAmbient.y - g_sceneLight.y);
    g_shadowLevel += FIX_ABS(g_sceneAmbient.z - g_sceneLight.z);
    g_shadowLevel = FixMul(g_shadowLevel, 0x55);
    if (g_shadowLevel > 0x10000)
        g_shadowLevel = 0x10000;
    g_shadowLevel = FixMul(g_shadowLevel, 0xff0000);
    c = g_sceneAmbient;
    if (boost != 0) {
        g_shadowLevel = FixMul(g_shadowLevel, 0x20000);
        if (g_shadowLevel > 0xff0000)
            g_shadowLevel = 0xff0000;
        FixVecScale(&c, &c, 0x2aac);
        b = c.z;
        g = c.y;
        r = c.x;
    } else {
        r = c.x;
        g = c.y;
        b = c.z;
        if (g >= r && r <= b) {
            g -= r;
            b -= r;
            r = 0;
        } else if (g <= r && g <= b) {
            r -= g;
            b -= g;
            g = 0;
        } else {
            r -= b;
            g -= b;
            b = 0;
        }
    }
    ((BYTE *)&g_shadowColour)[0] = (BYTE)(r >> 16);
    ((BYTE *)&g_shadowColour)[1] = (BYTE)(g >> 16);
    ((BYTE *)&g_shadowColour)[2] = (BYTE)(b >> 16);
}

// Sets the ambient colour of the scene (RGBA bytes).
// FUNCTION: CMR2 0x004b3740
void Scene_SetAmbient(BYTE *pColour, int boost)
{
    g_sceneAmbientD3D = ((((DWORD)pColour[3] << 8 | pColour[0]) << 8) | pColour[1]) << 8 | pColour[2];
    if (pColour[0] != g_sceneAmbientColour[0] || pColour[1] != g_sceneAmbientColour[1] ||
        pColour[2] != g_sceneAmbientColour[2])
        g_sceneLightDirty = 1;
    g_sceneAmbientColour[0] = pColour[0];
    g_sceneAmbientColour[1] = pColour[1];
    g_sceneAmbientColour[2] = pColour[2];
    g_sceneAmbientColour[3] = pColour[3];
    g_sceneAmbient.x = g_sceneAmbientColour[0] << 16;
    g_sceneAmbient.y = g_sceneAmbientColour[1] << 16;
    g_sceneAmbient.z = g_sceneAmbientColour[2] << 16;
    g_sceneLightColour.x = g_sceneLight.x - g_sceneAmbient.x;
    g_sceneLightColour.y = g_sceneLight.y - g_sceneAmbient.y;
    g_sceneLightColour.z = g_sceneLight.z - g_sceneAmbient.z;
    Scene_UpdateShadowColour(boost);
    Gfx_SetAmbient(g_sceneAmbientD3D);
}

// Sets the light colour of the scene.
// FUNCTION: CMR2 0x004b3f90
void Scene_SetLight(FixVector *pLight, int boost)
{
    if (pLight->x != g_sceneLight.x || pLight->y != g_sceneLight.y || pLight->z != g_sceneLight.z)
        g_sceneLightDirty = 1;
    g_sceneLight.x = pLight->x;
    g_sceneLight.y = pLight->y;
    g_sceneLight.z = pLight->z;
    g_sceneLightColourBytes[0] = (BYTE)(g_sceneLight.x >> 16);
    g_sceneLightColourBytes[1] = (BYTE)(g_sceneLight.y >> 16);
    g_sceneLightColourBytes[2] = (BYTE)(g_sceneLight.z >> 16);
    g_sceneLightColour.x = g_sceneLight.x - g_sceneAmbient.x;
    g_sceneLightColour.y = g_sceneLight.y - g_sceneAmbient.y;
    g_sceneLightColour.z = g_sceneLight.z - g_sceneAmbient.z;
    Scene_UpdateShadowColour(boost);
}

// Light colour at a level (0..1.0), as D3D ARGB.
// FUNCTION: CMR2 0x004b3b40
void Scene_GetLightColourD3D(DWORD *pColour, int level)
{
    int i;

    if (g_sceneLightDirty != 0) {
        Scene_BuildLightTables();
        g_sceneLightDirty = 0;
    }
    i = level * 0x31 >> 16;
    if (i < 0)
        i = 0;
    else if (i > 0x31)
        i = 0x31;
    *pColour = g_sceneLightTableD3D[i];
}

// Shadow colour at a level (0..1.0), as D3D ARGB.
// FUNCTION: CMR2 0x004b3ba0
void Scene_GetShadowColourD3D(DWORD *pColour, int level)
{
    int i;

    if (g_sceneLightDirty != 0) {
        Scene_BuildLightTables();
        g_sceneLightDirty = 0;
    }
    i = level * 0x31 >> 16;
    if (i < 0)
        i = 0;
    else if (i > 0x31)
        i = 0x31;
    *pColour = g_sceneShadowTableD3D[i];
}

struct ShadowCaster;

// GLOBAL: CMR2 0x006e0124
ShadowCaster *g_shadowCasters[30];  // registered shadow casters
// GLOBAL: CMR2 0x006e01c4
Mesh *g_shadowCylinders[10];        // shadow cylinders, cached by mesh name
// GLOBAL: CMR2 0x006e019c
void *g_sceneSectorLights;          // per sector
// GLOBAL: CMR2 0x006dfd90
void *g_sceneSectorLights2;         // per sector
// GLOBAL: CMR2 0x006dfd98
Mesh **g_sceneShadowMeshes;         // shadow mesh of each sector
// GLOBAL: CMR2 0x006dfdcc
unsigned short *g_sceneLightData;   // lighting data of the stage
// GLOBAL: CMR2 0x006dfdbc
LightZone *g_sceneLightZones;
// GLOBAL: CMR2 0x006dfdfc
int *g_sceneSectorFlags;            // per sector
// GLOBAL: CMR2 0x006e0204
short *g_sceneSectorZone;           // zone of each sector (-1 none)
// GLOBAL: CMR2 0x006deab8
BYTE g_shadowCasterCount;           // used entries of g_shadowCasters
// GLOBAL: CMR2 0x006e0b99
BYTE g_shadowCylinderCount;         // used entries of g_shadowCylinders

void Sound_NoOpMusicCallback(INT_PTR unused);
int Scene_AttenuateSectorLight(int sector, int light);
extern int g_unk0x005210c0;

// Lights every vertex of a mesh from its per-vertex light levels.
#define MESH_LIGHT_VERTICES(m)                                                      \
    {                                                                               \
        int *pLevel = (m)->pLightLevels;                                            \
        BYTE *pColour = (BYTE *)(m)->pVertexData + 0x18;                            \
        for (i = 0; i < (m)->field_0x10; i++) {                                     \
            Scene_GetLightColourD3D((DWORD *)pColour, *pLevel);                     \
            pLevel++;                                                               \
            pColour += 0x30;                                                        \
        }                                                                           \
    }

// Relights a sector when the scene ambient or light colour changed since its
// last update (ground mesh, static objects, nodes), then its shadow mesh,
// attenuated by D3D light 1 through the sector light zone when enabled.
// match 40%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004b3c00
void Scene_RelightSector(int sector)
{
    Sector *pSector;
    Mesh *pMesh;
    Mesh *pShadow;
    StageObject *pObject;
    SceneNode *pNode;
    LightZone *pZone;
    DWORD colour;
    int *pLevel;
    BYTE *pVertex;
    int *pZoneVertex;
    int intensity;
    int force;
    int i;
    int j;
    int k;

    force = 0;
    if (g_sceneSectorLights == NULL)
        return;
    if (g_sceneAmbientColour[0] != ((BYTE *)g_sceneSectorLights)[sector * 4] ||
        g_sceneAmbientColour[1] != ((BYTE *)g_sceneSectorLights)[sector * 4 + 1] ||
        g_sceneAmbientColour[2] != ((BYTE *)g_sceneSectorLights)[sector * 4 + 2] ||
        g_sceneLightColourBytes[0] != ((BYTE *)g_sceneSectorLights2)[sector * 4] ||
        g_sceneLightColourBytes[1] != ((BYTE *)g_sceneSectorLights2)[sector * 4 + 1] ||
        g_sceneLightColourBytes[2] != ((BYTE *)g_sceneSectorLights2)[sector * 4 + 2]) {
        ((DWORD *)g_sceneSectorLights)[sector] = *(DWORD *)g_sceneAmbientColour;
        ((DWORD *)g_sceneSectorLights2)[sector] = *(DWORD *)g_sceneLightColourBytes;
        pSector = g_sectors[sector];
        if (pSector != NULL) {
            pMesh = (Mesh *)pSector->pMesh;
            if (pMesh != NULL) {
                pLevel = pMesh->pLightLevels;
                pVertex = (BYTE *)pMesh->pVertexData;
                for (i = 0; i < pMesh->field_0x10; i++) {
                    Scene_GetLightColourD3D((DWORD *)(pVertex + 0x18), *pLevel);
                    pLevel++;
                    pVertex += 0x30;
                }
                Mesh_RefreshVertices(pMesh);
            }
            for (pObject = pSector->pObjects; pObject != NULL; pObject = pObject->pNext) {
                pMesh = pObject->pMesh;
                if ((pMesh->flags & 0x80) == 0) {
                    Scene_GetLightColourD3D(&colour, pObject->lightLevel);
                    Mesh_SetColourAndRefresh(pMesh, colour);
                } else {
                    pLevel = pMesh->pLightLevels;
                    pVertex = (BYTE *)pMesh->pVertexData;
                    for (i = 0; i < pMesh->field_0x10; i++) {
                        Scene_GetLightColourD3D((DWORD *)(pVertex + 0x18), *pLevel);
                        pLevel++;
                        pVertex += 0x30;
                    }
                    Mesh_RefreshVertices(pMesh);
                }
            }
            for (pNode = pSector->pFirstNode; pNode != NULL; pNode = pNode->pNextInSector)
                Sound_NoOpMusicCallback((INT_PTR)pNode);
            if (g_sceneShadowMeshes == NULL)
                return;
            if (g_sceneShadowMeshes[sector] != NULL)
                force = 1;
        }
    }
    if (((g_sceneShadowMeshes != NULL && g_sceneShadowMeshes[sector] != NULL && g_unk0x005210c0 != 0 &&
          Scene_AttenuateSectorLight(sector, 1) != 0) ||
         force) &&
        (pShadow = g_sceneShadowMeshes[sector]) != NULL) {
        pLevel = pShadow->pLightLevels;
        if (g_unk0x005210c0 != 0) {
            pZone = &g_sceneLightZones[g_sceneSectorZone[sector]];
            pVertex = (BYTE *)pShadow->pVertexData;
            pZoneVertex = &pZone->pItems->vertexCount;
            for (k = 0; k < pZone->itemCount; k++) {
                intensity = pZoneVertex[-2];
                if (intensity == 0x10000) {
                    for (j = 0; j < *pZoneVertex; j++) {
                        Scene_GetShadowColourD3D((DWORD *)(pVertex + 0x18), *pLevel);
                        pLevel++;
                        pVertex += 0x30;
                    }
                } else {
                    for (j = 0; j < *pZoneVertex; j++) {
                        Scene_GetShadowColourD3D((DWORD *)(pVertex + 0x18), FixMul(intensity, *pLevel));
                        pLevel++;
                        pVertex += 0x30;
                    }
                }
                pZoneVertex += sizeof(LightZoneItem) / sizeof(int);
            }
            Mesh_RefreshVertices(pShadow);
            return;
        } else {
            pVertex = (BYTE *)pShadow->pVertexData + 0x18;
            for (i = 0; i < pShadow->field_0x10; i++) {
                Scene_GetShadowColourD3D((DWORD *)pVertex, *pLevel);
                pLevel++;
                pVertex += 0x30;
            }
        }
        Mesh_RefreshVertices(pShadow);
    }
}

int Scene_ApplyGroundLightToMeshTree(SceneNode *pNode, int param_2);

// Steps the red channel of the ambient colour of every sector light around
// 0xf0 and relights the node list of each sector.
// match 74%: the logic and the instruction sequence follow the original; MSVC
// keeps the sector-lights pointer in EAX here (the original reused the ECX of
// the null test), so ours loads it again at the store and the loop header loses
// the `jmp` that skips that reload on the first iteration.
// FUNCTION: CMR2 0x004b3f20
void Scene_AnimateAmbientRedChannel(void)
{
    unsigned int sector;
    SceneNode *pNode;

    if (g_sceneSectorLights != NULL) {
        for (sector = 0; sector < (unsigned int)g_sectorCount; sector++) {
            if (g_sceneAmbientColour[0] < 0xf0)
                ((BYTE *)g_sceneSectorLights)[sector * 4] = (BYTE)(g_sceneAmbientColour[0] + 10);
            else
                ((BYTE *)g_sceneSectorLights)[sector * 4] = (BYTE)(g_sceneAmbientColour[0] - 10);
            for (pNode = g_sectors[sector]->pFirstNode; pNode != NULL;
                 pNode = pNode->pNextInSector)
                Scene_ApplyGroundLightToMeshTree(pNode, 0xffff0000);
            Scene_RelightSector(sector);
        }
    }
}

// Marks one mesh part of a registered shadow caster for rebuilding.
// FUNCTION: CMR2 0x004b4100
void Scene_MarkShadowPartDirty(SceneNode *pNode, Mesh *pMesh)
{
    ShadowCaster *pCaster = NULL;
    ShadowPart *pPart = NULL;
    int i;
    int count;
    if (g_sceneShadowMeshes != NULL) {
        i = 0;
        count = g_shadowCasterCount & 0xff;
        if (count > 0) {
            do {
                ShadowCaster *p = g_shadowCasters[i];
                if (p->pNode == pNode) {
                    pCaster = p;
                    i = count;
                }
                ++i;
            } while (i < count);
            if (pCaster != NULL) {
                i = 0;
                count = pCaster->partCount;
                if (count > 0) {
                    do {
                        if (pCaster->pParts[i].pMesh == pMesh) {
                            pPart = &pCaster->pParts[i];
                            i = count;
                        }
                        ++i;
                    } while (i < count);
                    if (pPart != NULL) {
                        pPart->verticesDirty = 1;
                        pCaster->projectionDirty = 1;
                    }
                }
            }
        }
    }
}

// Mesh nodes whose flags type is 6, 14 or 15 cast no shadow.
#define CASTS_SHADOW(n)                                                              \
    ((n)->type == SCENE_NODE_MESH && (n)->pObject != NULL && ((n)->flags & 0xff) != 6 && \
     ((n)->flags & 0xff) != 0xf && ((n)->flags & 0xff) != 0xe)

static __forceinline void ShadowPart_Init(ShadowPart *pPart)
{
    int i;

    pPart->pSourceNormals = (float (*)[3])CFileBuffer::AllocateLockedBuffer(pPart->pMesh->field_0x10 * sizeof(*pPart->pSourceNormals));
    pPart->pLightDotProducts = (float *)CFileBuffer::AllocateLockedBuffer(pPart->pMesh->field_0x10 * sizeof(*pPart->pLightDotProducts));
    pPart->pBiasedPositions = (float (*)[3])CFileBuffer::AllocateLockedBuffer(pPart->pMesh->field_0x10 * sizeof(*pPart->pBiasedPositions));
    pPart->pFaceVertices = (MeshVertexF **)CFileBuffer::AllocateLockedBuffer(pPart->pMesh->triangleCount * 3 * sizeof(*pPart->pFaceVertices));
    pPart->pVertices = (MeshVertexF *)CFileBuffer::AllocateLockedBuffer(pPart->pMesh->field_0x10 * sizeof(*pPart->pVertices));
    for (i = 0; i < pPart->pMesh->field_0x10; i++)
        pPart->pVertices[i].specularColour = 0xff000000;
    pPart->pEmittedPositions = (float (*)[3])CFileBuffer::AllocateLockedBuffer(pPart->pMesh->field_0x10 * sizeof(*pPart->pEmittedPositions));
    pPart->verticesDirty = 1;
}

// Registers pNode and every mesh below it as a shadow caster (at most 29). Small
// objects (flags type <= 4) use a cylinder around the mesh unless exactMeshes is set.
// match 55%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004b45d0
void Scene_AddShadowCaster(SceneNode *pNode, int exactMeshes)
{
    ShadowCaster *pCaster;
    ShadowPart *pPart;
    SceneNode *pChild;
    SceneNode *p;

    if (pNode == NULL || g_shadowCasterCount >= 29)
        return;
    pCaster = (ShadowCaster *)CFileBuffer::AllocateLockedBuffer(sizeof(ShadowCaster));
    g_shadowCasters[g_shadowCasterCount++] = pCaster;
    pCaster->pNode = pNode;
    pCaster->pParts = NULL;
    pCaster->partCount = 0;
    pCaster->projectionDirty = 1;
    if (CASTS_SHADOW(pNode))
        pCaster->partCount = 1;
    for (pChild = pNode->pFirstChild; pChild != NULL; pChild = pChild->pNext) {
        for (p = pChild; p != NULL; p = p->pFirstChild) {
            if (CASTS_SHADOW(p))
                pCaster->partCount++;
        }
    }
    pCaster->pParts = (ShadowPart *)CFileBuffer::AllocateLockedBuffer(pCaster->partCount * sizeof(ShadowPart));
    pPart = pCaster->pParts;
    if (CASTS_SHADOW(pNode)) {
        pPart->pMesh = (Mesh *)pNode->pObject;
        if (exactMeshes == 0 && (pNode->flags & 0xff) <= 4)
            pPart->pMesh = Mesh_GetShadowCylinder((Mesh *)pNode->pObject);
        ShadowPart_Init(pPart);
        pPart->faceCount = 0;
        pPart->pNode = pNode;
        pPart->vertexCount = 0;
        pPart++;
    }
    for (pChild = pNode->pFirstChild; pChild != NULL; pChild = pChild->pNext) {
        for (p = pChild; p != NULL; p = p->pFirstChild) {
            if (CASTS_SHADOW(p)) {
                if (exactMeshes == 0 && (pNode->flags & 0xff) <= 4)
                    pPart->pMesh = Mesh_GetShadowCylinder((Mesh *)p->pObject);
                else
                    pPart->pMesh = (Mesh *)p->pObject;
                ShadowPart_Init(pPart);
                pPart->pNode = p;
                pPart->faceCount = 0;
                pPart->vertexCount = 0;
                pPart++;
            }
        }
    }
}

extern int *g_triangleVertexHeights;
void Mesh_BuildParts(Mesh *pMesh);
void Mesh_UploadVertices(Mesh *pMesh);

// GLOBAL: CMR2 0x006dfe00
FixVector g_sceneLightDir;
// GLOBAL: CMR2 0x006e00a0
FixVector g_sceneLightBasis[3];
// GLOBAL: CMR2 0x006e0b28
FixVector g_sceneLightRight;        // basis[0] flattened
// GLOBAL: CMR2 0x006dfda0
FixVector g_sceneLightForward;      // basis[2] flattened
// GLOBAL: CMR2 0x006e01b0
float g_sceneLightDirF[3];
// GLOBAL: CMR2 0x006e0220
float g_sceneLightBasisF[9];
// GLOBAL: CMR2 0x005210ec
char g_strShadowMeshName[] = "SHAD%d";

// PORT: native copies of the lighting records (see Scene_LoadLighting).
static BYTE *g_sceneLightNative;

// Reads the lighting data of a stage: the light direction and basis, then
// the shadow zones (one per sector with a shadow mesh). Offsets in the data
// are turned into pointers, zones are matched to their sector and their
// items to the scene objects there, and a shadow mesh is built for each.
// match 34%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004b4aa0
void Scene_LoadLighting(int *pData)
{
    LightZoneMesh *pHeader;
    LightZoneItem *pItem;
    StageObject *pObj;
    Mesh *pMesh;
    FixVector *pBasis;
    unsigned short *pCursor;
    unsigned int s;
    unsigned int found;
    unsigned int next;
    int zone;
    int item;
    int d;
    int i;
    unsigned int flags;
    BOOL done;

    if (pData == NULL)
        return;
    g_sceneLightDir.x = pData[0];
    g_sceneLightDir.y = pData[1];
    g_sceneLightDir.z = pData[2];
    pBasis = g_sceneLightBasis;
    pCursor = (unsigned short *)(pData + 3);
    do {
        pBasis->x = *(int *)pCursor;
        pBasis->y = *(int *)(pCursor + 2);
        pBasis->z = *(int *)(pCursor + 4);
        pBasis++;
        pCursor += 6;
    } while (pBasis < &g_sceneLightBasis[3]);
    g_sceneLightRight.x = g_sceneLightBasis[0].x;
    g_sceneLightDirF[0] = X87S_MUL(X87S_FILD(float, g_sceneLightDir.x), (float)CGraphics::m_oneOver65536);
    g_sceneLightRight.z = g_sceneLightBasis[0].z;
    g_sceneLightRight.y = 0;
    g_sceneLightForward.x = g_sceneLightBasis[2].x;
    g_sceneLightForward.y = 0;
    g_sceneLightForward.z = g_sceneLightBasis[2].z;
    g_sceneLightDirF[1] = X87S_MUL(X87S_FILD(float, g_sceneLightDir.y), (float)CGraphics::m_oneOver65536);
    g_sceneLightDirF[2] = X87S_MUL(X87S_FILD(float, g_sceneLightDir.z), (float)CGraphics::m_oneOver65536);
    g_sceneLightBasisF[0] = X87S_MUL(X87S_FILD(float, g_sceneLightBasis[0].x), (float)CGraphics::m_oneOver65536);
    g_sceneLightBasisF[1] = X87S_MUL(X87S_FILD(float, g_sceneLightBasis[0].y), (float)CGraphics::m_oneOver65536);
    g_sceneLightBasisF[2] = X87S_MUL(X87S_FILD(float, g_sceneLightBasis[0].z), (float)CGraphics::m_oneOver65536);
    g_sceneLightBasisF[3] = X87S_MUL(X87S_FILD(float, g_sceneLightBasis[1].x), (float)CGraphics::m_oneOver65536);
    g_sceneLightBasisF[4] = X87S_MUL(X87S_FILD(float, g_sceneLightBasis[1].y), (float)CGraphics::m_oneOver65536);
    g_sceneLightBasisF[5] = X87S_MUL(X87S_FILD(float, g_sceneLightBasis[1].z), (float)CGraphics::m_oneOver65536);
    g_sceneLightBasisF[6] = X87S_MUL(X87S_FILD(float, g_sceneLightBasis[2].x), (float)CGraphics::m_oneOver65536);
    g_sceneLightBasisF[7] = X87S_MUL(X87S_FILD(float, g_sceneLightBasis[2].y), (float)CGraphics::m_oneOver65536);
    g_sceneLightBasisF[8] = X87S_MUL(X87S_FILD(float, g_sceneLightBasis[2].z), (float)CGraphics::m_oneOver65536);

    // PORT: the zones, their items and the zones' mesh headers hold 32-bit
    // file offsets that the original turns into pointers in place, and a
    // native pointer does not fit there. They are copied into native records
    // (one buffer, freed by Scene_FreeLighting); vertices, triangles and
    // levels have no pointers and stay in the file, which the original
    // also reads in this order.
    {
        unsigned int zoneCount = *pCursor;
        const BYTE *pDisk = (const BYTE *)(pCursor + 1);
        const BYTE *pFile;
        unsigned int itemCount = 0;
        LightZone *pZones;
        LightZoneMesh *pMeshes;
        LightZoneItem *pNextItem;

        for (zone = 0; zone < (int)zoneCount; zone++)
            itemCount += *(const unsigned short *)(pDisk + zone * 0x14 + 2);
        g_sceneLightNative = (BYTE *)CFileBuffer::AllocateLockedBuffer(
            zoneCount * (sizeof(LightZone) + sizeof(LightZoneMesh)) + itemCount * sizeof(LightZoneItem) + 1);
        pZones = (LightZone *)g_sceneLightNative;
        pMeshes = (LightZoneMesh *)(pZones + zoneCount);
        pNextItem = (LightZoneItem *)(pMeshes + zoneCount);
        pFile = pDisk + zoneCount * 0x14;
        for (zone = 0; zone < (int)zoneCount; zone++) {
            memcpy(&pZones[zone], pDisk + zone * 0x14, offsetof(LightZone, pItems));
            pZones[zone].pItems = pNextItem;
            pZones[zone].pMesh = &pMeshes[zone];
            for (item = 0; item < pZones[zone].itemCount; item++, pFile += 0x30, pNextItem++) {
                memset(pNextItem, 0, sizeof(*pNextItem));
                memcpy(pNextItem, pFile, offsetof(LightZoneItem, vertexCount) + sizeof(int));
            }
        }
        for (zone = 0; zone < (int)zoneCount; zone++, pFile += 0x14)
            memcpy(&pMeshes[zone], pFile, offsetof(LightZoneMesh, pVertices));
        for (zone = 0; zone < (int)zoneCount; zone++) {
            pMeshes[zone].pVertices = (MeshVertexF *)pFile;
            pFile += pMeshes[zone].vertexCount * 0x30;
        }
        for (zone = 0; zone < (int)zoneCount; zone++) {
            pMeshes[zone].pTriangles = (MeshTriangle *)pFile;
            pFile += pMeshes[zone].triangleCount * 0x4c;
        }
        for (zone = 0; zone < (int)zoneCount; zone++) {
            pMeshes[zone].pLevels = (int *)pFile;
            pFile += pMeshes[zone].vertexCount * 4;
        }
        g_sceneLightData = pCursor;
        g_sceneLightZones = pZones;
    }

    // Sector of each zone.
    for (zone = 0; zone < *g_sceneLightData; zone++) {
        found = 0xffffffff;
        s = 0;
        if (g_sectorCount == 0) {
            g_sceneLightZones[zone].sector = -1;
            continue;
        }
        do {
            next = s;
            if (g_sectors[s]->x == g_sceneLightZones[zone].x &&
                g_sectors[s]->z == g_sceneLightZones[zone].z) {
                found = s;
                next = g_sectorCount;
            }
            s = next + 1;
        } while (s < (unsigned int)g_sectorCount);
        if ((int)found >= 0)
            g_sceneLightZones[zone].sector = (short)found;
        else
            g_sceneLightZones[zone].sector = -1;
    }

    // Scene object under each item.
    for (zone = 0; zone < *g_sceneLightData; zone++) {
        for (item = 0; item < g_sceneLightZones[zone].itemCount; item++) {
            pItem = &g_sceneLightZones[zone].pItems[item];
            for (s = 0; s < (unsigned int)g_sectorCount; s++) {
                done = FALSE;
                pObj = g_sectors[s]->pObjects;
                if (pObj == NULL)
                    continue;
                do {
                    if (done)
                        break;
                    d = pObj->position.x - pItem->ownerX;
                    if (abs(d) < 0x28f) {
                        d = pObj->position.z - pItem->ownerZ;
                        if (FIX_ABS(d) < 0x28f) {
                            pItem->pOwner = pObj;
                            done = TRUE;
                        }
                    }
                    pObj = pObj->pNext;
                } while (pObj != NULL);
                if (done)
                    s = g_sectorCount;
            }
            pItem->intensity = 0x10000;
        }
    }
    // Each shadow triangle takes the texture of its item's object; until then
    // its texture field holds the index of the item.
    for (zone = 0; zone < *g_sceneLightData; zone++) {
        pHeader = g_sceneLightZones[zone].pMesh;
        for (i = 0; i < pHeader->triangleCount; i++) {
            MeshTriangle *pTri = &pHeader->pTriangles[i];
            LightZoneItem *pOwner = &g_sceneLightZones[zone].pItems[pTri->textures[0]];
            pTri->textures[0] = pOwner->pOwner->pMesh->pTriangles[0].textures[0];
        }
    }

    // A shadow mesh per zone.
    g_sceneShadowMeshes = (Mesh **)CFileBuffer::AllocateLockedBuffer(g_sectorCount * sizeof(*g_sceneShadowMeshes));
    g_sceneSectorZone = (short *)CFileBuffer::AllocateLockedBuffer(g_sectorCount * 2);
    s = 0;
    if (g_sectorCount != 0) {
        do {
            s++;
            g_sceneShadowMeshes[s - 1] = NULL;
            g_sceneSectorZone[s - 1] = -1;
        } while (s < (unsigned int)g_sectorCount);
    }
    // The original builds the flags from an uninitialised local that shares its
    // stack slot with the byte offset of the item loop's zone (zone count * 0x14).
    flags = ((*g_sceneLightData * 0x14) & 0xffffe02d) | 0x402d;
    for (zone = 0; zone < *g_sceneLightData; zone++) {
        if (g_sceneLightZones[zone].sector == -1)
            continue;
        g_sceneShadowMeshes[(unsigned short)g_sceneLightZones[zone].sector] = Mesh_Alloc();
        g_sceneSectorZone[(unsigned short)g_sceneLightZones[zone].sector] = (short)zone;
        pMesh = g_sceneShadowMeshes[(unsigned short)g_sceneLightZones[zone].sector];
        pMesh->field_0x108[0] = 0xff;
        pMesh->field_0x108[1] = 0xff;
        pMesh->field_0x108[2] = 0xff;
        // Clears the tail of the record, pCollisionRecords to the end.
        for (i = offsetof(Mesh, pCollisionRecords); i < (int)sizeof(Mesh); i++)
            ((BYTE *)pMesh)[i] = 0;
        pHeader = g_sceneLightZones[zone].pMesh;
        for (i = 0xc; i != 0; i--)
            sprintf((char *)pMesh, g_strShadowMeshName, zone);
        pMesh->field_0x10 = pHeader->vertexCount;
        pMesh->pVertexData = (DWORD *)pHeader->pVertices;
        pMesh->triangleCount = pHeader->triangleCount;
        pMesh->pTriangles = pHeader->pTriangles;
        pMesh->pField20 = NULL;
        pMesh->field_0x104[0] = 0;
        pMesh->pLightLevels = pHeader->pLevels;
        pMesh->flags = flags;
        pMesh->field_0x2c = 0;
        Mesh_BuildParts(pMesh);
        Mesh_UploadVertices(pMesh);
    }
}

// Frees the lighting of a stage (callback registered by Scene_InitLighting).
// FUNCTION: CMR2 0x004b5510
int Scene_FreeLighting(void)
{
    unsigned int i;

    if (g_sceneSectorLights != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_sceneSectorLights);
        g_sceneSectorLights = NULL;
    }
    if (g_sceneSectorLights2 != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_sceneSectorLights2);
        g_sceneSectorLights2 = NULL;
    }
    if (g_sceneLightTable != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_sceneLightTable);
        g_sceneLightTable = NULL;
    }
    if (g_sceneLightTableD3D != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_sceneLightTableD3D);
        g_sceneLightTableD3D = NULL;
    }
    if (g_sceneShadowTable != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_sceneShadowTable);
        g_sceneShadowTable = NULL;
    }
    if (g_sceneShadowTableD3D != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_sceneShadowTableD3D);
        g_sceneShadowTableD3D = NULL;
    }
    if (g_sceneShadowMeshes != NULL) {
        for (i = 0; i < (unsigned int)g_sectorCount; i++) {
            if (g_sceneShadowMeshes[i] != NULL) {
                Mesh_Free(g_sceneShadowMeshes[i]);
                CFileBuffer::FreeGenericFileBuffer(g_sceneShadowMeshes[i]);
                g_sceneShadowMeshes[i] = NULL;
            }
        }
        CFileBuffer::FreeGenericFileBuffer(g_sceneShadowMeshes);
        g_sceneShadowMeshes = NULL;
    }
    if (g_sceneSectorFlags != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_sceneSectorFlags);
        g_sceneSectorFlags = NULL;
    }
    if (g_sceneSectorZone != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_sceneSectorZone);
        g_sceneSectorZone = NULL;
    }
    // PORT: the native lighting records.
    if (g_sceneLightNative != NULL) {
        CFileBuffer::FreeGenericFileBuffer(g_sceneLightNative);
        g_sceneLightNative = NULL;
        g_sceneLightZones = NULL;
    }
    g_triangleVertexHeights = NULL;
    return 1;
}

// Sets up the lighting of a stage: the light and shadow tables, per-sector
// state, and the stage's lighting data (shadow meshes).
// FUNCTION: CMR2 0x004b5620
void Scene_InitLighting(int *pData, int *pHeights)
{
    int i;
    unsigned int k;

    for (i = 0; i < 30; i++)
        g_shadowCasters[i] = NULL;
    for (i = 0; i < 10; i++)
        g_shadowCylinders[i] = NULL;
    g_triangleVertexHeights = NULL;
    g_sceneSectorLights = NULL;
    g_sceneSectorLights2 = NULL;
    g_sceneLightTable = NULL;
    g_sceneLightTableD3D = NULL;
    g_sceneShadowTable = NULL;
    g_sceneShadowTableD3D = NULL;
    g_sceneShadowMeshes = NULL;
    g_sceneLightData = NULL;
    g_sceneLightZones = NULL;
    g_sceneSectorFlags = NULL;
    g_shadowCasterCount = 0;
    g_shadowCylinderCount = 0;
    g_sceneSectorLights = CFileBuffer::AllocateLockedBuffer(g_sectorCount * 4);
    g_sceneSectorLights2 = CFileBuffer::AllocateLockedBuffer(g_sectorCount * 4);
    g_sceneLightTable = (BYTE *)CFileBuffer::AllocateLockedBuffer(200);
    g_sceneLightTableD3D = (DWORD *)CFileBuffer::AllocateLockedBuffer(200);
    g_sceneShadowTable = (DWORD *)CFileBuffer::AllocateLockedBuffer(200);
    g_sceneShadowTableD3D = (DWORD *)CFileBuffer::AllocateLockedBuffer(200);
    g_sceneSectorFlags = (int *)CFileBuffer::AllocateLockedBuffer(g_sectorCount * 4);
    k = 0;
    if ((unsigned int)g_sectorCount > 0) {
        do {
            k++;
            g_sceneSectorFlags[k - 1] = 0;
        } while (k < (unsigned int)g_sectorCount);
    }
    Scene_LoadLighting(pData);
    if (pHeights != NULL)
        g_triangleVertexHeights = pHeights;
    CGame::RegisterCallback((void *)Scene_FreeLighting, NULL);
}

// Shadow geometry streamed into pVertexBuffer2 between Scene_BeginShadowBatch
// and Scene_EndShadowBatch, split into {first vertex, vertex count, texture,
// view mask} batches.
// GLOBAL: CMR2 0x006dead0
int g_shadowBatches[300][4];
// GLOBAL: CMR2 0x006e01f0
int *g_shadowBatch;
// GLOBAL: CMR2 0x006e0b90
void *g_shadowVertexData;
// GLOBAL: CMR2 0x006e0b94
int g_shadowVertexCount;
// GLOBAL: CMR2 0x006e0b98
BYTE g_shadowBatchCount;
// GLOBAL: CMR2 0x005210d4
int g_shadowLastTexture = -1;
// GLOBAL: CMR2 0x005210d8
int g_shadowLastFlags = -1;
// Direction the shadows are cast along (opposite of the light).
// GLOBAL: CMR2 0x006dfdd8
FixVector g_sceneShadowDir;

// Light zone vertices near the current car, collected by Scene_CollectNearbyLightZones and
// consumed by Scene_EmitNodeShadowGeometry (99 entries max).
// GLOBAL: CMR2 0x006dfe14
LightZoneItem *g_sceneZoneList[99];
// GLOBAL: CMR2 0x006e01c0
int g_sceneZoneCount;

int Mesh_GetCornerLight(Mesh *pMesh, MeshTriangle *pTri, FixVector *pDir, int corner);
DWORD Scene_GetGroundLight(FixVector *pPos, int *pLevel);

// Applies the ground light to every mesh node of the tree hanging from pNode:
// the light direction is the node's matrix (local when param_2 is -0x10000,
// world otherwise) applied to the scene shadow direction, normalised, and each
// triangle corner takes the light of its direction, clamped to the light level
// of the node.
// match 75%: same logic; the diff is register allocation (the original keeps
// the light level and the vector pointers in different registers) and the
// abs() of the level difference, which MSVC compiles with a branch here
// match 75%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004b50b0
int Scene_ApplyGroundLightToMeshTree(SceneNode *pNode, int param_2)
{
    SceneNode *p;
    Mesh *pMesh;
    MeshTriangle *pTri;
    FixMatrix *pMatrix;
    FixVector dir;
    FixVector pos;
    int level;
    int diff;
    int i;
    int j;

    p = pNode;
    if (p != NULL) {
    do {
        if (p->type == SCENE_NODE_MESH && p->visible != 0 && (pMesh = (Mesh *)p->pObject) != NULL &&
            (pMesh->flags & 0x800) != 0) {
            if (param_2 == -0x10000) {
                pMatrix = &p->current;
                FixMatrix_InverseRotateVector(&dir, &g_sceneShadowDir, pMatrix);
                FixMatrix_GetPosition(&pos, pMatrix);
            } else {
                pMatrix = &p->world;
                FixMatrix_InverseRotateVector(&dir, &g_sceneShadowDir, pMatrix);
                FixMatrix_GetPosition(&pos, pMatrix);
            }
            i = FixVecLength(&dir);
            if (i == 0) {
                dir.x = 0;
                dir.y = 0;
                dir.z = 0;
            } else {
                FixVecScaleRecip(&dir, &dir, i);
            }
            Scene_GetGroundLight(&pos, &level);
            if (param_2 != -0x10000) {
                diff = level - param_2;
                if (FIX_ABS(diff) > 0xccc)
                    level = FixMul(0x8000, diff) + param_2;
            }
            for (j = 0; j < pMesh->triangleCount; j++) {
                pTri = (MeshTriangle *)((BYTE *)pMesh->pTriangles + j * 0x4c);
                for (i = 0; i < 3; i++) {
                    pMesh->pLightLevels[pTri->vertexIndex[i]] = Mesh_GetCornerLight(pMesh, pTri, &dir, i);
                    if (level < pMesh->pLightLevels[pTri->vertexIndex[i]])
                        pMesh->pLightLevels[pTri->vertexIndex[i]] = level;
                }
            }
        }
        p = p->pFirstChild;
    } while (p != NULL);
    }
    return level;
}

// FUNCTION: CMR2 0x004b5340
void Scene_SetShadowDirection(FixVector *pLightDir)
{
    FixVecScale(&g_sceneShadowDir, pLightDir, -0x10000);
}

// Locks the shadow vertex buffer and starts the first batch.
// FUNCTION: CMR2 0x004b5e60
void Scene_BeginShadowBatch(void)
{
    if (g_sceneSectorZone != NULL) {
        g_shadowVertexData = Gfx_LockVertexBuffer(CGraphics::m_pTextureManager->pVertexBuffer2);
        g_shadowVertexCount = 0;
        g_shadowBatchCount = 0;
        g_shadowLastTexture = -1;
        g_shadowLastFlags = -1;
        g_shadowBatch = g_shadowBatches[0];
    }
}

// Closes the current batch and unlocks the shadow vertex buffer.
// FUNCTION: CMR2 0x004b5eb0
void Scene_EndShadowBatch(void)
{
    if (g_sceneSectorZone != NULL) {
        g_shadowBatch[1] = g_shadowVertexCount - g_shadowBatch[0];
        Gfx_UnlockVertexBuffer(CGraphics::m_pTextureManager->pVertexBuffer2);
    }
}

void Scene_ProjectCasterIntoLightZone(LightZoneItem *pItem, ShadowCaster *pCaster, BYTE param3);

// GLOBAL: CMR2 0x00511cf0
extern const double g_double0_01f = 0.009999999776482582;
extern const float g_netOne;           // 0x00511350, 1.0f   (NetRace.cpp)
extern const float g_netMinusOne;      // 0x0051134c, -1.0f  (NetRace.cpp)
int FloatMatrix_InverseRotateVector(float *pOut, float *pV, float *pM);

// Projects the shadow parts of pCaster through the projection box that the
// light zone pItem builds from its vertices, and appends the triangles that
// survive the scissor test to the shadow vertex batch.
// match 44%: the loops are the original's for loops over unsigned counts, the
// vertex step keeps the offset from the part's origin (dx, dy, dz) and the
// scaled axis in delta, as the original's stack shows. The rest is register
// and stack-slot allocation: MSVC 6 keeps minX on the FPU stack and spills
// six more dwords (frame 0xac against our 0x90).
// FUNCTION: CMR2 0x004b5770
void Scene_ProjectCasterIntoLightZone(LightZoneItem *pItem, ShadowCaster *pCaster, BYTE param3)
{
    StageObject *pLand;
    Mesh *pZone;
    ShadowCaster *pCaster2;
    ShadowPart *pPart;
    float *pVert;
    float *pf;
    float (*pIn)[3];
    MeshVertexF *pOut;
    MeshVertexF **pTri;
    MeshVertexF *pv0;
    MeshVertexF *pv1;
    MeshVertexF *pv2;
    float minY, maxY, minX, maxX;
    float minZ, maxZ, minW, maxW;
    float zoneScaleX, zoneScaleY;
    float scaleX, scaleY;
    float invX, invY;
    float spanZ, spanW;
    float projScaleX, projScaleY;
    float origin[3];
    float pos[3];
    float delta[3];
    float axis[3];
    float plane[3];
    float projX[3];
    float projY[3];
    float ray[3];
    float dx, dy, dz;
    float dot, inv;
    float depth;
    float qx, qy, qz, t;
    int count, i, offset, tex, triIndex, vi;

    pCaster2 = pCaster;
    minY = 0.0f;
    maxY = 0.0f;
    minX = 0.0f;
    maxX = 0.0f;
    minZ = 10.0f;
    maxZ = -10.0f;
    minW = 10.0f;
    maxW = -10.0f;
    pLand = pItem->pOwner;
    zoneScaleX = X87S_MUL(X87S_FILD(float, pLand->matrix.right.x), CGraphics::m_oneOver65536);
    zoneScaleY = X87S_MUL(X87S_FILD(float, pLand->matrix.up.y), CGraphics::m_oneOver65536);
    pZone = pLand->pMesh;
    count = pZone->field_0x10;
    if (count > 0) {
        pVert = (float *)pZone->pVertexData;
        do {
            float v;

            v = zoneScaleX * pVert[0];
            if (v > maxX)
                maxX = v;
            else if (v < minX)
                minX = v;
            v = zoneScaleY * pVert[1];
            if (v > maxY)
                maxY = v;
            else if (v < minY)
                minY = v;
            v = pVert[8];
            if (v < minZ)
                minZ = v;
            if (v > maxZ)
                maxZ = v;
            v = pVert[9];
            if (v < minW)
                minW = v;
            if (v > maxW)
                maxW = v;
            pVert += 0xc;
        } while (--count != 0);
    }
    scaleX = X87S_ADD(X87S_MUL(X87S_FILD(float, pItem->field_0x18), CGraphics::m_oneOver65536), g_netOne);
    maxX = scaleX * maxX;
    minX = scaleX * minX;
    scaleY = X87S_ADD(X87S_MUL(X87S_FILD(float, pItem->field_0x1c), CGraphics::m_oneOver65536), g_netOne);
    maxY = scaleY * maxY;
    spanZ = maxZ - minZ;
    spanW = maxW - minW;
    invX = g_netOne / (maxX - minX);
    invY = g_netOne / (maxY - minY);
    minX = invX * minX * spanZ;
    minY = (invY * minY + g_netOne) * spanW;
    projScaleX = invX * spanZ;
    projScaleY = invY * spanW;
    origin[0] = X87S_MUL(X87S_FILD(float, pLand->position.x), CGraphics::m_oneOver65536);
    origin[1] = X87S_MUL(X87S_FILD(float, pLand->position.y), CGraphics::m_oneOver65536);
    tex = pZone->pTriangles->textures[0];
    origin[2] = X87S_MUL(X87S_FILD(float, pLand->position.z), CGraphics::m_oneOver65536);
    if (g_shadowLastTexture == -1 || g_shadowLastFlags == -1) {
        ((BYTE *)g_shadowBatch)[0xc] = param3;
        g_shadowBatch[0] = g_shadowVertexCount;
        g_shadowBatch[2] = tex;
        g_shadowLastFlags = param3;
        g_shadowLastTexture = tex;
        g_shadowBatchCount++;
    } else if (g_shadowLastTexture != tex || g_shadowLastFlags != param3) {
        g_shadowBatch[1] = g_shadowVertexCount - g_shadowBatch[0];
        g_shadowBatch += 4;
        g_shadowBatchCount++;
        ((BYTE *)g_shadowBatch)[0xc] = param3;
        g_shadowBatch[0] = g_shadowVertexCount;
        g_shadowBatch[2] = tex;
        g_shadowLastTexture = tex;
        g_shadowLastFlags = param3;
    }
    i = 0;
    if (pCaster2->partCount != 0) {
        // 64BIT: the byte cursor preserves MSVC6's register schedule; its
        // pitch follows the native record instead of the Win32 0x58 bytes.
        offset = 0;
        do {
            pPart = (ShadowPart *)((BYTE *)pCaster2->pParts + offset);
            if (pPart->pNode->viewMask != 0) {
                float *pf = (float *)pPart;

                projY[0] = projScaleY * pf[6];
                axis[0] = pf[0];
                projY[1] = projScaleY * pf[7];
                axis[1] = pf[1];
                projY[2] = projScaleY * pf[8];
                axis[2] = pf[2];
                plane[0] = pf[3];
                plane[1] = pf[4];
                plane[2] = pf[5];
                projX[0] = projScaleX * pf[9];
                projX[1] = projScaleX * pf[10];
                projX[2] = projScaleX * pf[11];
                pos[0] = pPart->pNode->worldF[12];
                pos[1] = pPart->pNode->worldF[13];
                pos[2] = pPart->pNode->worldF[14];
                delta[0] = origin[0] - pos[0];
                delta[1] = origin[1] - pos[1];
                delta[2] = origin[2] - pos[2];
                FloatMatrix_InverseRotateVector(pos, delta, pPart->pNode->worldF);
                dot = plane[0] * axis[0] + plane[1] * axis[1] + plane[2] * axis[2];
                if (fabs(dot) > g_double0_01f) {
                    inv = g_netMinusOne / dot;
                    ray[0] = plane[0] * inv;
                    ray[1] = plane[1] * inv;
                    ray[2] = plane[2] * inv;
                    pOut = pPart->pVertices;
                    pIn = pPart->pEmittedPositions;
                    for (vi = 0; vi < (unsigned short)pPart->vertexCount; vi++) {
                        {
                            dx = (*pIn)[0] - pos[0];
                            dy = (*pIn)[1] - pos[1];
                            dz = (*pIn)[2] - pos[2];
                            t = dx * ray[0] + dy * ray[1] + dz * ray[2];
                            delta[0] = t * axis[0];
                            delta[1] = t * axis[1];
                            delta[2] = t * axis[2];
                            qx = dx + delta[0];
                            qy = dy + delta[1];
                            qz = dz + delta[2];
                            depth = qx * projX[0] + qy * projX[1] + qz * projX[2];
                            pOut->textureCoordinates[0][0] = depth - minX + minZ;
                            pOut->textureCoordinates[0][1] = minY - (qx * projY[0] + qy * projY[1] + qz * projY[2]) + minW;
                            pIn++;
                            pOut++;
                        }
                    }
                    pTri = pPart->pFaceVertices;
                    for (triIndex = 0; triIndex < (unsigned short)pPart->faceCount; triIndex++) {
                        {
                            pv0 = pTri[0];
                            pv1 = pTri[1];
                            pv2 = pTri[2];
                            if ((pv0->textureCoordinates[0][0] <= maxZ ||
                                 pv1->textureCoordinates[0][0] <= maxZ ||
                                 pv2->textureCoordinates[0][0] <= maxZ) &&
                                (pv0->textureCoordinates[0][0] >= minZ ||
                                 pv1->textureCoordinates[0][0] >= minZ ||
                                 pv2->textureCoordinates[0][0] >= minZ) &&
                                (pv0->textureCoordinates[0][1] <= maxW ||
                                 pv1->textureCoordinates[0][1] <= maxW ||
                                 pv2->textureCoordinates[0][1] <= maxW) &&
                                (pv0->textureCoordinates[0][1] >= minW ||
                                 pv1->textureCoordinates[0][1] >= minW ||
                                 pv2->textureCoordinates[0][1] >= minW)) {
                                ((MeshVertexF *)g_shadowVertexData)[g_shadowVertexCount] = *pv0;
                                ((MeshVertexF *)g_shadowVertexData)[g_shadowVertexCount + 1] = *pv1;
                                ((MeshVertexF *)g_shadowVertexData)[g_shadowVertexCount + 2] = *pv2;
                                g_shadowVertexCount += 3;
                            }
                            pTri += 3;
                        }
                    }
                }
            }
            i++;
            offset += sizeof(ShadowPart);
        } while (i < pCaster2->partCount);
    }
}
void Sector_GetGridDimensions(int *columns, int *rows);
void Scene_EmitProjectedShadowMesh(ShadowPart *pPart, int param_2);

// Scale of the normal added to the vertex position when a shadow part is
// projected (0x00511cec, 0.005f).
// GLOBAL: CMR2 0x00511cec
extern const float g_float0_005 = 0.005f;

// Projects the shadow parts of a caster that were marked dirty: for every vertex
// of the part the position and the normal are copied into its two work buffers
// and the normal scaled by 0x511cec is added to the position, the flag is
// cleared; then every part whose geometry was still pending is emitted with the
// shadow mesh builder.
// FUNCTION: CMR2 0x004b4490
void Scene_ProjectDirtyShadowParts(ShadowCaster *pCaster, int intensity)
{
    ShadowPart *pPart;
    int partIndex;
    int vertexIndex;
    float scaled[3];

    if (pCaster->projectionDirty != 0) {
        for (partIndex = 0; partIndex < pCaster->partCount; partIndex++) {
            pPart = &pCaster->pParts[partIndex];
            if (pPart->verticesDirty != 0) {
                {
                    MeshVertexF *pSrc = (MeshVertexF *)pPart->pMesh->pVertexData;
                    float (*pBiasedPosition)[3] = pPart->pBiasedPositions;
                    float (*pNormal)[3] = pPart->pSourceNormals;
                    for (vertexIndex = 0; vertexIndex < pPart->pMesh->field_0x10; vertexIndex++) {
                        (*pBiasedPosition)[0] = pSrc->x;
                        (*pBiasedPosition)[1] = pSrc->y;
                        (*pBiasedPosition)[2] = pSrc->z;
                        (*pNormal)[0] = pSrc->nx;
                        (*pNormal)[1] = pSrc->ny;
                        (*pNormal)[2] = pSrc->nz;
                        scaled[0] = (*pNormal)[0];
                        scaled[0] *= g_float0_005;
                        scaled[1] = (*pNormal)[1];
                        scaled[1] *= g_float0_005;
                        scaled[2] = (*pNormal)[2] * g_float0_005;
                        (*pBiasedPosition)[0] += scaled[0];
                        (*pBiasedPosition)[1] += scaled[1];
                        (*pBiasedPosition)[2] += scaled[2];
                        pNormal++;
                        pSrc++;
                        pBiasedPosition++;
                    }
                }
            }
            pPart->verticesDirty = 0;
        }
    }
    pCaster->projectionDirty = 0;
    if (pCaster->meshDirty != 0) {
        for (partIndex = 0; partIndex < pCaster->partCount; partIndex++)
            Scene_EmitProjectedShadowMesh(pCaster->pParts + partIndex, intensity);
    }
    pCaster->meshDirty = 0;
}

// Marks the node's shadow caster and emits every light zone vertex collected by
// Scene_CollectNearbyLightZones through the shadow geometry builder.
// FUNCTION: CMR2 0x004b5ee0
void Scene_EmitNodeShadowGeometry(SceneNode *pNode, int param2, BYTE param3)
{
    ShadowCaster *pCaster;
    int i;
    int count;

    pCaster = NULL;
    if (pNode != NULL && pNode->viewMask != 0 && g_sceneSectorZone != NULL &&
        (unsigned short)g_sceneZoneCount > 0) {
        i = 0;
        count = g_shadowCasterCount & 0xff;
        if (count > 0) {
            do {
                ShadowCaster *p = g_shadowCasters[i];
                if (p->pNode == pNode) {
                    pCaster = p;
                    i = count;
                }
                ++i;
            } while (i < count);
            if (pCaster != NULL) {
                i = 0;
                pCaster->meshDirty = 1;
                if ((unsigned short)g_sceneZoneCount > 0) {
                    do {
                        Scene_ProjectDirtyShadowParts(pCaster, param2);
                        Scene_ProjectCasterIntoLightZone(g_sceneZoneList[i], pCaster, param3);
                        ++i;
                    } while (i < (g_sceneZoneCount & 0xffff));
                }
            }
        }
    }
}

// Collects the light zone vertices around a node: the 3x3 neighbourhood of
// sectors around *pSector is mapped through g_sceneSectorZone, and every item
// whose box (in the light basis) contains the node within radius is stored in
// g_sceneZoneList.
// match 60%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004b5f90
void Scene_CollectNearbyLightZones(SceneNode *pNode, int radius, short *pSector)
{
    ShadowCaster *pCaster;
    int i;
    int count;

    pCaster = NULL;
    *(unsigned short *)&g_sceneZoneCount = 0;
    if (pNode != NULL && pNode->viewMask != 0 && g_sceneSectorZone != NULL) {
        i = 0;
        count = g_shadowCasterCount & 0xff;
        if (count > 0) {
            do {
                ShadowCaster *p = g_shadowCasters[i];
                if (p->pNode == pNode) {
                    pCaster = p;
                    i = count;
                }
                ++i;
            } while (i < count);
            if (pCaster != NULL) {
                int rows;
                int item;
                int itemOff;
                int n;
                FixVector pos;
                int columns;
                short samples[9];
                short s;
                LightZone *pZone;
                LightZoneItem *pItem;

                FixMatrix_GetPosition(&pos, &pNode->world);
                Sector_GetGridDimensions(&columns, &rows);
                n = 9;
                samples[0] = *pSector;
                samples[1] = samples[0] - 1;
                samples[2] = samples[0] + 1;
                samples[3] = samples[0] - columns;
                samples[4] = samples[3] - 1;
                samples[5] = samples[3] + 1;
                samples[6] = samples[0] + columns;
                samples[7] = samples[6] - 1;
                samples[8] = samples[6] + 1;
                pSector = samples;
                do {
                    s = *pSector++;
                    if ((unsigned int)s < (unsigned int)g_sectorCount && s >= 0) {
                        short zone = g_sceneSectorZone[s];
                        if (zone != -1) {
                            pZone = &g_sceneLightZones[zone];
                            for (item = 0, itemOff = 0; item < pZone->itemCount;
                                 item++, itemOff += sizeof(LightZoneItem)) {
                                pItem = (LightZoneItem *)((BYTE *)pZone->pItems + itemOff);
                                if (pItem != NULL && pItem->pOwner != NULL) {
                                    int dx;
                                    int dz;
                                    int d1;
                                    int d2;

                                    dx = pos.x - pItem->x;
                                    dz = pos.z - pItem->z;
                                    d1 = FIX_ABS(FixMul(dx, g_sceneLightRight.x) +
                                                 FixMul(dz, g_sceneLightRight.z)) -
                                         radius;
                                    d2 = FIX_ABS(FixMul(dx, g_sceneLightForward.x) +
                                                 FixMul(dz, g_sceneLightForward.z)) -
                                         radius;
                                    if (d1 <= pItem->halfSizeRight &&
                                        d2 <= pItem->halfSizeForward &&
                                        (unsigned short)g_sceneZoneCount < 99) {
                                        g_sceneZoneList[g_sceneZoneCount & 0xffff] = pItem;
                                        (*(unsigned short *)&g_sceneZoneCount)++;
                                    }
                                }
                            }
                        }
                    }
                } while (--n);
            }
        }
    }
}

// FUNCTION: CMR2 0x004b3840
void Scene_GetAmbientColour(DWORD *pColour)
{
    *pColour = *(DWORD *)g_sceneAmbientColour;
}

// FUNCTION: CMR2 0x004b3850
void Scene_GetLightColourBytes(DWORD *pColour)
{
    *pColour = *(DWORD *)g_sceneLightColourBytes;
}

// Sets the constant attenuation of the D3D light held by a type-1 node.
// FUNCTION: CMR2 0x004b3570
void Scene_SetLightAttenuation(SceneNode *pNode, int attenuation)
{
    SceneLight *pLight;

    if (pNode != NULL && (pLight = (SceneLight *)pNode->pObject) != NULL) {
        pLight->light.attenuation0 = X87S_MUL(X87S_FILD(float, attenuation), CGraphics::m_oneOver65536);
        Gfx_SetLight(pLight->index, &pLight->light);
    }
}

// Depth-first search (children before siblings) for the first node whose
// flags type byte is `type`.
// FUNCTION: CMR2 0x004ad890
SceneNode *SceneNode_FindByType(SceneNode *pNode, unsigned int type)
{
    SceneNode *pFound;

    while (pNode != NULL) {
        if ((pNode->flags & 0xff) == type)
            return pNode;
        if (pNode->pFirstChild != NULL && (pFound = SceneNode_FindByType(pNode->pFirstChild, type)) != NULL)
            return pFound;
        pNode = pNode->pNext;
    }
    return NULL;
}

// Sets the diffuse colour (16.16 per channel, clamped to 0..255) of the D3D
// light held by a type-1 node.
// FUNCTION: CMR2 0x004b34d0
void Scene_SetLightColour(SceneNode *pNode, int r, int g, int b)
{
    SceneLight *pLight;

    pLight = (SceneLight *)pNode->pObject;
    if (r < 0)
        r = 0;
    if (g < 0)
        g = 0;
    if (b < 0)
        b = 0;
    if (r > 0xff0000)
        r = 0xff0000;
    if (g > 0xff0000)
        g = 0xff0000;
    if (b > 0xff0000)
        b = 0xff0000;
    pLight->light.diffuse.r = X87S_MUL(X87S_FILD(float, r), CGraphics::m_oneOver65536);
    pLight->light.diffuse.g = X87S_MUL(X87S_FILD(float, g), CGraphics::m_oneOver65536);
    pLight->light.diffuse.b = X87S_MUL(X87S_FILD(float, b), CGraphics::m_oneOver65536);
    Gfx_SetLight(pLight->index, &pLight->light);
}

// Light level (0..1) of one corner of a mesh triangle for a light direction:
// four times the dot product of its vertex normal with pDir, clamped.
// FUNCTION: CMR2 0x004b4040
int Mesh_GetCornerLight(Mesh *pMesh, MeshTriangle *pTri, FixVector *pDir, int corner)
{
    FixVector normal;
    int level;

#define CORNER_NORMAL(i) \
    ((float *)((BYTE *)pMesh->pVertexData + pTri->vertexIndex[corner] * 0x30))[3 + (i)]
    normal.x = (int)FTOI(__int64, X87S_MUL(CORNER_NORMAL(0), CGraphics::m_65536));
    normal.y = (int)FTOI(__int64, X87S_MUL(CORNER_NORMAL(1), CGraphics::m_65536));
    normal.z = (int)FTOI(__int64, X87S_MUL(CORNER_NORMAL(2), CGraphics::m_65536));
#undef CORNER_NORMAL
    level = FixMul(FixVecDot(pDir, &normal), 0x40000);
    if (level < 0)
        return 0;
    if (level > 0x10000)
        level = 0x10000;
    return level;
}

// Light level and colour of the ground at a position: those of the nearest
// (in x/z) vertex of its sector's ground mesh. Returns r, g, b bytes.
// FUNCTION: CMR2 0x004b3860
DWORD Scene_GetGroundLight(FixVector *pPos, int *pLevel)
{
    Mesh *pMesh;
    float *pVertex;
    float *pNearest;
    int *pVertexLevel;
    float dx;
    float dz;
    float d2;
    float best;
    float pos[3];
    BYTE rgb[4];
    int i;
    int count;

    rgb[0] = 0xff;
    rgb[1] = 0xff;
    rgb[2] = 0xff;
    *pLevel = 0x10000;
    pos[0] = X87S_MUL(X87S_FILD(float, pPos->x), CGraphics::m_oneOver65536);
    pNearest = NULL;
    rgb[3] = 0;
    best = 32000.0f;
    pos[2] = X87S_MUL(X87S_FILD(float, pPos->z), CGraphics::m_oneOver65536);
    {
        int sector = (short)Sector_FromPosition(pPos);
        pMesh = (Mesh *)g_sectors[sector]->pMesh;
    }
    if (pMesh != NULL) {
        pVertex = (float *)pMesh->pVertexData;
        pVertexLevel = pMesh->pLightLevels;
        i = 0;
        count = pMesh->field_0x10;
        if (count > 0) {
            do {
                dx = pVertex[0] - pos[0];
                dz = pVertex[2] - pos[2];
                d2 = dx * dx + dz * dz;
                if (d2 < best) {
                    *pLevel = *pVertexLevel;
                    pNearest = pVertex;
                    best = d2;
                }
                pVertex += 12;
                pVertexLevel++;
                i++;
            } while (i < pMesh->field_0x10);
            if (pNearest != NULL) {
                rgb[0] = (BYTE)(((DWORD *)pNearest)[6] >> 16);
                rgb[1] = (BYTE)(((DWORD *)pNearest)[6] >> 8);
                rgb[2] = (BYTE)((DWORD *)pNearest)[6];
            }
        }
    }
    return *(DWORD *)rgb;
}

// Frees every shadow caster (with its per-part buffers) and every cached
// shadow cylinder.
// FUNCTION: CMR2 0x004b5380
void Scene_FreeShadowCasters(void)
{
    int n;
    int i;

    for (n = 0; n < 30; n++) {
        if (g_shadowCasters[n] != NULL) {
            if (g_shadowCasters[n]->pParts != NULL) {
                for (i = 0; i < g_shadowCasters[n]->partCount; i++) {
                    if (g_shadowCasters[n]->pParts[i].pSourceNormals != NULL) {
                        CFileBuffer::FreeGenericFileBuffer(g_shadowCasters[n]->pParts[i].pSourceNormals);
                        g_shadowCasters[n]->pParts[i].pSourceNormals = NULL;
                    }
                    if (g_shadowCasters[n]->pParts[i].pLightDotProducts != NULL) {
                        CFileBuffer::FreeGenericFileBuffer(g_shadowCasters[n]->pParts[i].pLightDotProducts);
                        g_shadowCasters[n]->pParts[i].pLightDotProducts = NULL;
                    }
                    if (g_shadowCasters[n]->pParts[i].pBiasedPositions != NULL) {
                        CFileBuffer::FreeGenericFileBuffer(g_shadowCasters[n]->pParts[i].pBiasedPositions);
                        g_shadowCasters[n]->pParts[i].pBiasedPositions = NULL;
                    }
                    if (g_shadowCasters[n]->pParts[i].pVertices != NULL) {
                        CFileBuffer::FreeGenericFileBuffer(g_shadowCasters[n]->pParts[i].pVertices);
                        g_shadowCasters[n]->pParts[i].pVertices = NULL;
                    }
                    if (g_shadowCasters[n]->pParts[i].pEmittedPositions != NULL) {
                        CFileBuffer::FreeGenericFileBuffer(g_shadowCasters[n]->pParts[i].pEmittedPositions);
                        g_shadowCasters[n]->pParts[i].pEmittedPositions = NULL;
                    }
                    if (g_shadowCasters[n]->pParts[i].pFaceVertices != NULL) {
                        CFileBuffer::FreeGenericFileBuffer(g_shadowCasters[n]->pParts[i].pFaceVertices);
                        g_shadowCasters[n]->pParts[i].pFaceVertices = NULL;
                    }
                }
                CFileBuffer::FreeGenericFileBuffer(g_shadowCasters[n]->pParts);
                g_shadowCasters[n]->pParts = NULL;
            }
            CFileBuffer::FreeGenericFileBuffer(g_shadowCasters[n]);
            g_shadowCasters[n] = NULL;
        }
        g_shadowCasters[n] = NULL;
    }
    for (n = 0; n < 10; n++) {
        if (g_shadowCylinders[n] != NULL) {
            if (g_shadowCylinders[n]->pTriangles != NULL) {
                CFileBuffer::FreeGenericFileBuffer(g_shadowCylinders[n]->pTriangles);
                g_shadowCylinders[n]->pTriangles = NULL;
            }
            if (g_shadowCylinders[n]->pVertexData != NULL) {
                CFileBuffer::FreeGenericFileBuffer(g_shadowCylinders[n]->pVertexData);
                g_shadowCylinders[n]->pVertexData = NULL;
            }
            CFileBuffer::FreeGenericFileBuffer(g_shadowCylinders[n]);
            g_shadowCylinders[n] = NULL;
        }
        g_shadowCylinders[n] = NULL;
    }
    g_shadowCasterCount = 0;
    g_shadowCylinderCount = 0;
    g_shadowVertexCount = 0;
    g_shadowBatchCount = 0;
}

void Frontend_SetObjectField118(struct Texture *pTexture, int value);
void Graphics_InvalidateTextureStageCache(void);

// Draws the shadow batches visible in view `view` (bit of each batch mask),
// with the batch texture forced to blend mode 10.
// FUNCTION: CMR2 0x004b6240
void Scene_DrawShadowBatches(unsigned int view)
{
    Texture *pTexture;
    Texture *pLast;
    int blend;
    int i;
    BYTE mask;

    if ((unsigned int)g_shadowVertexCount <= 0)
        return;
    CGraphics::SetZWriteEnable(0);
    CGraphics::SetCullMode(1);
    g_shadowBatch = g_shadowBatches[0];
    mask = (BYTE)(1u << view);
    pLast = NULL;
    for (i = 0; i < g_shadowBatchCount; i++) {
        if ((unsigned int)g_shadowBatch[1] > 0 && (*(BYTE *)&g_shadowBatch[3] & mask) != 0) {
            pTexture = CGraphics::m_pTextureManager->textureBuffer[g_shadowBatch[2]];
            if (pTexture != pLast) {
                pLast = pTexture;
                blend = pTexture->blendMode;
                Frontend_SetObjectField118(pTexture, 10);
                CGraphics::ApplyTextureStageChange(0, pTexture);
                Frontend_SetObjectField118(pTexture, blend);
            }
            Gfx_DrawPrimitiveVB(GFX_TRIANGLELIST, CGraphics::m_pTextureManager->pVertexBuffer2, g_shadowBatch[0], g_shadowBatch[1]);
        }
        g_shadowBatch += 4;
    }
    CGraphics::ApplyTextureStageChange(0, 0);
    CGraphics::SetZWriteEnable(1);
    CGraphics::SetCullMode(CGame::GetSectorDrawState());
    Graphics_InvalidateTextureStageCache();
    CGraphics::InvalidateBlendStateCache();
}

// Sets the byte at 0x17c (view mask) on a node list and, recursively, on the
// children of its mesh nodes.
// FUNCTION: CMR2 0x004add90
void SceneNode_SetViewMask(SceneNode *pNode, BYTE mask)
{
    for (; pNode != NULL; pNode = pNode->pNext) {
        if (pNode->type == SCENE_NODE_MESH) {
            pNode->viewMask = mask;
            if (pNode->pFirstChild != NULL)
                SceneNode_SetViewMask(pNode->pFirstChild, mask);
        }
    }
}

// Default material (white diffuse/ambient).
// GLOBAL: CMR2 0x006e00c8
GfxMaterial g_sceneMaterial;

// Restores the D3D lights, the default material and the ambient colour
// after the device was (re)created.
// match 80%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004b2e50
void Scene_RestoreLights(void)
{
    int i;

    for (i = 0; i < 60; i++) {
        if (g_sceneType1Objects[i] != NULL) {
            Gfx_SetLight(i, (GfxLight *)g_sceneType1Objects[i]);
            Gfx_EnableLight(i, TRUE);
        }
    }
    memset(&g_sceneMaterial, 0, sizeof(g_sceneMaterial));
    g_sceneMaterial.diffuse.r = 1.0f;
    g_sceneMaterial.ambient.r = 1.0f;
    g_sceneMaterial.diffuse.g = 1.0f;
    g_sceneMaterial.ambient.g = 1.0f;
    g_sceneMaterial.diffuse.b = 1.0f;
    g_sceneMaterial.ambient.b = 1.0f;
    g_sceneMaterial.diffuse.a = 1.0f;
    g_sceneMaterial.ambient.a = 1.0f;
    g_sceneMaterial.specular.r = 0.0f;
    g_sceneMaterial.emissive.r = 0.0f;
    g_sceneMaterial.specular.g = 0.0f;
    g_sceneMaterial.emissive.g = 0.0f;
    g_sceneMaterial.specular.b = 0.0f;
    g_sceneMaterial.emissive.b = 0.0f;
    g_sceneMaterial.specular.a = 0.0f;
    g_sceneMaterial.emissive.a = 0.0f;
    g_sceneMaterial.power = 0.0f;
    Gfx_SetMaterial(&g_sceneMaterial);
    {
        BYTE *p = g_sceneAmbientColour;
        g_sceneAmbientD3D = (((DWORD)p[3] << 8 | p[0]) << 8 | p[1]) << 8 | p[2];
    }
    Gfx_SetAmbient(g_sceneAmbientD3D);
}

// GLOBAL: CMR2 0x006838d0
int g_viewSetupMode;

// Sets the Direct3D view transform from a moved camera node (inverse of its
// world matrix), unless the view setup mode is 5 or more.
// match 44%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004ade00
void Scene_SetViewFromCamera(SceneNode *pCamera)
{
    float view[16];
    GfxMatrix *pView;

    if (g_viewSetupMode < 5 && pCamera->dirty != 0) {
        FixMatrix_Invert(&CGraphics::m_pTextureManager->viewMatrix, &pCamera->world);
        pView = (GfxMatrix *)view;
        view[0] = X87S_MUL(X87S_FILD(float, ((int *)&CGraphics::m_pTextureManager->viewMatrix)[0]), CGraphics::m_oneOver65536);
        view[1] = X87S_MUL(X87S_FILD(float, ((int *)&CGraphics::m_pTextureManager->viewMatrix)[1]), CGraphics::m_oneOver65536);
        view[2] = X87S_MUL(X87S_FILD(float, ((int *)&CGraphics::m_pTextureManager->viewMatrix)[2]), CGraphics::m_oneOver65536);
        view[3] = X87S_MUL(X87S_FILD(float, ((int *)&CGraphics::m_pTextureManager->viewMatrix)[3]), CGraphics::m_oneOver65536);
        view[4] = X87S_MUL(X87S_FILD(float, ((int *)&CGraphics::m_pTextureManager->viewMatrix)[4]), CGraphics::m_oneOver65536);
        view[5] = X87S_MUL(X87S_FILD(float, ((int *)&CGraphics::m_pTextureManager->viewMatrix)[5]), CGraphics::m_oneOver65536);
        view[6] = X87S_MUL(X87S_FILD(float, ((int *)&CGraphics::m_pTextureManager->viewMatrix)[6]), CGraphics::m_oneOver65536);
        view[7] = X87S_MUL(X87S_FILD(float, ((int *)&CGraphics::m_pTextureManager->viewMatrix)[7]), CGraphics::m_oneOver65536);
        view[8] = X87S_MUL(X87S_FILD(float, ((int *)&CGraphics::m_pTextureManager->viewMatrix)[8]), CGraphics::m_oneOver65536);
        view[9] = X87S_MUL(X87S_FILD(float, ((int *)&CGraphics::m_pTextureManager->viewMatrix)[9]), CGraphics::m_oneOver65536);
        view[10] = X87S_MUL(X87S_FILD(float, ((int *)&CGraphics::m_pTextureManager->viewMatrix)[10]), CGraphics::m_oneOver65536);
        view[11] = X87S_MUL(X87S_FILD(float, ((int *)&CGraphics::m_pTextureManager->viewMatrix)[11]), CGraphics::m_oneOver65536);
        view[12] = X87S_MUL(X87S_FILD(float, ((int *)&CGraphics::m_pTextureManager->viewMatrix)[12]), CGraphics::m_oneOver65536);
        view[13] = X87S_MUL(X87S_FILD(float, ((int *)&CGraphics::m_pTextureManager->viewMatrix)[13]), CGraphics::m_oneOver65536);
        view[14] = X87S_MUL(X87S_FILD(float, ((int *)&CGraphics::m_pTextureManager->viewMatrix)[14]), CGraphics::m_oneOver65536);
        view[15] = X87S_MUL(X87S_FILD(float, ((int *)&CGraphics::m_pTextureManager->viewMatrix)[15]), CGraphics::m_oneOver65536);
        Gfx_SetTransform(GFX_TRANSFORM_VIEW, (GfxMatrix *)&view[0]);
    }
}

// Moves a point/spot light: position (16.16) and a direction pointing back
// at the origin.
// FUNCTION: CMR2 0x004b35b0
void Scene_SetLightPosition(SceneNode *pNode, int x, int y, int z)
{
    SceneLight *pLight;
    FixVector d;

    pLight = (SceneLight *)pNode->pObject;
    pLight->light.position.x = X87S_MUL(X87S_FILD(float, x), CGraphics::m_oneOver65536);
    pLight->light.position.y = X87S_MUL(X87S_FILD(float, y), CGraphics::m_oneOver65536);
    pLight->light.position.z = X87S_MUL(X87S_FILD(float, z), CGraphics::m_oneOver65536);
    d.x = -x;
    d.y = -y;
    d.z = -z;
    FIX_NORMALIZE_INTO(d, d);
    pLight->light.direction.x = X87S_MUL(X87S_FILD(float, d.x), CGraphics::m_oneOver65536);
    pLight->light.direction.y = X87S_MUL(X87S_FILD(float, d.y), CGraphics::m_oneOver65536);
    pLight->light.direction.z = X87S_MUL(X87S_FILD(float, d.z), CGraphics::m_oneOver65536);
    Gfx_SetLight(pLight->index, &pLight->light);
}

// Pushes the world matrices of dirty visible nodes to their Direct3D
// objects: float matrix for meshes (types 0 and 3), position (and spot
// direction) for lights.
// FUNCTION: CMR2 0x004ad8d0
void SceneNode_FlushTransforms(SceneNode *pNode)
{
    SceneLight *pLight;
    int *m;

    for (; pNode != NULL; pNode = pNode->pNext) {
        if (SceneNode_IsVisible(pNode)) {
            switch (pNode->type) {
            case SCENE_NODE_MESH:
            case SCENE_NODE_EMPTY:
                if (pNode->dirty == 1) {
                    m = (int *)&pNode->world;
                    pNode->worldF[0] = X87S_MUL(X87S_FILD(float, m[0]), CGraphics::m_oneOver65536);
                    pNode->worldF[1] = X87S_MUL(X87S_FILD(float, m[1]), CGraphics::m_oneOver65536);
                    pNode->worldF[2] = X87S_MUL(X87S_FILD(float, m[2]), CGraphics::m_oneOver65536);
                    pNode->worldF[3] = X87S_MUL(X87S_FILD(float, m[3]), CGraphics::m_oneOver65536);
                    pNode->worldF[4] = X87S_MUL(X87S_FILD(float, m[4]), CGraphics::m_oneOver65536);
                    pNode->worldF[5] = X87S_MUL(X87S_FILD(float, m[5]), CGraphics::m_oneOver65536);
                    pNode->worldF[6] = X87S_MUL(X87S_FILD(float, m[6]), CGraphics::m_oneOver65536);
                    pNode->worldF[7] = X87S_MUL(X87S_FILD(float, m[7]), CGraphics::m_oneOver65536);
                    pNode->worldF[8] = X87S_MUL(X87S_FILD(float, m[8]), CGraphics::m_oneOver65536);
                    pNode->worldF[9] = X87S_MUL(X87S_FILD(float, m[9]), CGraphics::m_oneOver65536);
                    pNode->worldF[10] = X87S_MUL(X87S_FILD(float, m[10]), CGraphics::m_oneOver65536);
                    pNode->worldF[11] = X87S_MUL(X87S_FILD(float, m[11]), CGraphics::m_oneOver65536);
                    pNode->worldF[12] = X87S_MUL(X87S_FILD(float, m[12]), CGraphics::m_oneOver65536);
                    pNode->worldF[13] = X87S_MUL(X87S_FILD(float, m[13]), CGraphics::m_oneOver65536);
                    pNode->worldF[14] = X87S_MUL(X87S_FILD(float, m[14]), CGraphics::m_oneOver65536);
                    pNode->worldF[15] = X87S_MUL(X87S_FILD(float, m[15]), CGraphics::m_oneOver65536);
                    pNode->dirty = 0;
                }
                break;
            case SCENE_NODE_TYPE1:
                pLight = (SceneLight *)pNode->pObject;
                if (pNode->dirty == 1) {
                    if (pLight->light.type != GFX_LIGHT_DIRECTIONAL) {
                        if (pLight->light.type != GFX_LIGHT_POINT) {
                            if (pLight->light.type == GFX_LIGHT_SPOT) {
                                pLight->light.direction.x = X87S_MUL(X87S_FILD(float, pNode->world.right.x), CGraphics::m_oneOver65536);
                                pLight->light.direction.y = X87S_MUL(X87S_FILD(float, pNode->world.right.y), CGraphics::m_oneOver65536);
                                pLight->light.direction.z = X87S_MUL(X87S_FILD(float, pNode->world.right.z), CGraphics::m_oneOver65536);
                            } else {
                                pNode->dirty = 0;
                                goto next;
                            }
                        }
                        pLight->light.position.x = X87S_MUL(X87S_FILD(float, pNode->world.position.x), CGraphics::m_oneOver65536);
                        pLight->light.position.y = X87S_MUL(X87S_FILD(float, pNode->world.position.y), CGraphics::m_oneOver65536);
                        pLight->light.position.z = X87S_MUL(X87S_FILD(float, pNode->world.position.z), CGraphics::m_oneOver65536);
                        Gfx_SetLight(pLight->index, &pLight->light);
                    }
                    pNode->dirty = 0;
                }
                break;
            }
        }
    next:
        if (SceneNode_IsVisible(pNode) && pNode->pFirstChild != NULL)
            SceneNode_FlushTransforms(pNode->pFirstChild);
    }
}

// FUNCTION: CMR2 0x004b5760
void Scene_ApplyShadowLightDirection(FixVector *pLightDir)
{
    Scene_SetShadowDirection(pLightDir);
}

// Sets the view mask of a mesh node and of its whole subtree.
// FUNCTION: CMR2 0x004addd0
void SceneNode_SetViewMaskTree(SceneNode *pNode, BYTE mask)
{
    if (pNode->type == SCENE_NODE_MESH)
        pNode->viewMask = mask;
    if (pNode->pFirstChild != NULL)
        SceneNode_SetViewMask(pNode->pFirstChild, mask);
}

// GLOBAL: CMR2 0x00511cf8
extern const double g_doubleOneOver511 = 0.0019569471624266144;
// GLOBAL: CMR2 0x00511d00
extern const double g_doubleOneOver4095 = 0.0002442002442002442;
// GLOBAL: CMR2 0x00511d08
extern const double g_angle12ToRadians = 0.0015339807878856412;

extern short g_acosTable[4096];
extern const double g_fixedRadiansToAngle12;

// Fills the 16.16 maths tables the original computes at startup. The original
// uses the x87 instructions in brackets; this is plain C with the same maths:
//   g_sinTable  sin(i * 2*pi/4096)                  (fsin)
//   g_tanTable  tan(i * 2*pi/4096)                  (fptan)
//   g_sqrtTable sqrt(8 + 16 * i) * 256              (fsqrt)
//   g_acosTable asin(i / 4095) as a 12-bit angle    (_CIasin)
//   g_atanTable arctan(i / 511) as a 12-bit angle   (fpatan)
// MSVC6 /QIfist emits x87 FISTP without changing its rounding mode: nearest
// even in the audited original. The __int64 cast is not portable C++ rounding;
// other compilers normally truncate it. All loops fill their complete tables
// (4096 entries, except the 512-entry arctangent table). See the numeric audit
// in docs/audits/2026-10-10/numeric-semantics.md.
// match 68%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004b7b20
void Scene_InitFixedMathTables(void)
{
    int i;
    int n;
    int value;

    for (i = 0; i < 4096; i++) {
        g_sinTable[i] = (int)FTOI(__int64, X87S_MUL(sin(X87S_MUL((double)i, g_angle12ToRadians)), CGraphics::m_65536));
        g_tanTable[i] = (int)FTOI(__int64, X87S_MUL(tan(X87S_MUL((double)i, g_angle12ToRadians)), CGraphics::m_65536));
    }
    i = 8;
    // tan(90°) and tan(270°) would overflow: the original clamps them
    g_tanTable[3072] = 0x7fffffff;
    g_tanTable[1024] = 0x7fffffff;

    for (n = 0; n < 4096; n++, i += 16)
        g_sqrtTable[n] = (unsigned short)(int)FTOI(__int64, X87S_MUL(X87S_SQRT(X87S_MUL((double)i, CGraphics::m_oneOver65536)), CGraphics::m_65536));

    for (i = 0; i < 4096; i++) {
        value = (int)FTOI(__int64, X87S_MUL(asin(X87S_MUL((double)i, g_doubleOneOver4095)), CGraphics::m_65536));
        g_acosTable[i] = (short)FTOI(__int64, X87S_MUL((double)value, g_fixedRadiansToAngle12));
    }

    for (i = 0; i < 512; i++) {
        value = (int)FTOI(__int64, X87S_MUL(atan(X87S_MUL((double)i, g_doubleOneOver511)), CGraphics::m_65536));
        g_atanTable[i] = (unsigned short)FTOI(__int64, X87S_MUL((double)value, g_fixedRadiansToAngle12));
    }
}


int FloatMatrix_RotateVector(float *pOut, float *pV, float *pM);
int FloatMatrix_InverseRotateVector(float *pOut, float *pV, float *pM);
extern const float g_float10;   // 10.0f, defined in NetRace.cpp
extern const float g_netZero;          // 0x0051131c, 0.0f
extern const float g_float0_1;    // 0x00511ce8, 0.1f
// Scratch table of emitted vertex indices of the shadow mesh builder (-1 none).
// GLOBAL: CMR2 0x006e0354
unsigned short g_shadowVertexRemap[0x3f0];

// Transforms the light into mesh space and dots it with the source normals.
// Faces whose three normal/light dots are negative are dropped; accepted
// faces share remapped render vertices, with alpha ramped by each light dot.
// FUNCTION: CMR2 0x004b4180
void Scene_EmitProjectedShadowMesh(ShadowPart *pPart, int intensity)
{
    int *pSourceIndex;
    MeshVertexF **pFaceVertex;
    float alphaScale;
    float (*pEmittedPosition)[3];
    INT_PTR referenceOffset;
    int verticesLeft;
    unsigned short *pFace;
    int remaining;
    BYTE colour[4];
    BYTE baseAlpha;
    float worldPosition[3];
    int sourceIndices[3];
    float nodePosition[3];
    int sourceVertexIndex;
    short emittedIndex;
    MeshVertexF *pVertex;
    float (*pNormal)[3];
    float *pLightDot;

    *(DWORD *)colour = g_shadowColour;
    if (intensity > 0x10000)
        intensity = 0x10000;
    intensity = FixMul(g_shadowLevel, intensity);
    alphaScale = (float)(X87S_MUL((double)intensity, CGraphics::m_oneOver65536));
    baseAlpha = (BYTE)FTOI(__int64, alphaScale);
    pPart->faceCount = 0;
    pPart->vertexCount = 0;
    {
        unsigned short *pw = g_shadowVertexRemap;
        for (remaining = pPart->pMesh->field_0x10; remaining > 0; remaining--)
            *pw++ = 0xffff;
    }

    nodePosition[0] = pPart->pNode->worldF[12];
    nodePosition[1] = pPart->pNode->worldF[13];
    nodePosition[2] = pPart->pNode->worldF[14];
    FloatMatrix_InverseRotateVector(pPart->light, g_sceneLightDirF,
                                    pPart->pNode->worldF);
    FloatMatrix_InverseRotateVector(pPart->light + 3, g_sceneLightBasisF,
                                    pPart->pNode->worldF);
    FloatMatrix_InverseRotateVector(pPart->light + 6, g_sceneLightBasisF + 3,
                                    pPart->pNode->worldF);
    FloatMatrix_InverseRotateVector(pPart->light + 9, g_sceneLightBasisF + 6,
                                    pPart->pNode->worldF);

    {
        pNormal = pPart->pSourceNormals;
        pLightDot = pPart->pLightDotProducts;
        for (remaining = pPart->pMesh->field_0x10; remaining > 0; remaining--) {
            *pLightDot = (*pNormal)[2] * pPart->light[2] + (*pNormal)[1] * pPart->light[1] + (*pNormal)[0] * pPart->light[0];
            pLightDot++;
            pNormal++;
        }
    }

    pFaceVertex = pPart->pFaceVertices;
    pEmittedPosition = pPart->pEmittedPositions;
    pVertex = pPart->pVertices;
    remaining = pPart->pMesh->triangleCount;
    if (remaining > 0) {
        pFace = &pPart->pMesh->pTriangles->vertexIndex[1];
        do {
            sourceIndices[0] = pFace[-1];
            sourceIndices[1] = pFace[0];
            sourceIndices[2] = pFace[1];
            if (pPart->pLightDotProducts[sourceIndices[0]] >= g_netZero ||
                pPart->pLightDotProducts[sourceIndices[1]] >= g_netZero ||
                pPart->pLightDotProducts[sourceIndices[2]] >= g_netZero) {
                pSourceIndex = sourceIndices;
                // 64BIT: retain MSVC6'emittedIndex shared sourceIndices/destination cursor, but
                // account for native pointer slots being wider than ints.
                referenceOffset = (INT_PTR)pFaceVertex - (INT_PTR)sourceIndices;
                verticesLeft = 3;
                do {
                    sourceVertexIndex = *pSourceIndex;
                    emittedIndex = g_shadowVertexRemap[sourceVertexIndex];
                    if (emittedIndex == -1) {
                        DWORD *pSrcD = (DWORD *)pPart->pBiasedPositions[sourceVertexIndex];
                        DWORD *pOutD = (DWORD *)*pEmittedPosition;
                        pOutD[0] = pSrcD[0];
                        pOutD[1] = pSrcD[1];
                        pOutD[2] = pSrcD[2];
                        *(MeshVertexF **)((char *)pSourceIndex + referenceOffset + (pSourceIndex - sourceIndices) * (sizeof(*pFaceVertex) - sizeof(*pSourceIndex))) = pVertex;
                        g_shadowVertexRemap[sourceVertexIndex] = pPart->vertexCount;
                        {
                            float lightDot = pPart->pLightDotProducts[sourceVertexIndex];
                            colour[3] = baseAlpha;
                            if (lightDot < g_netZero)
                                colour[3] = 0;
                            else if (lightDot < g_float0_1)
                                colour[3] = (BYTE)FTOI(__int64, lightDot * g_float10 * alphaScale);
                        }
                        pVertex->diffuseColour =
                            ((((DWORD)colour[3] << 8 | colour[0]) << 8 | colour[1]) << 8) | colour[2];
                        FloatMatrix_RotateVector(worldPosition, *pEmittedPosition,
                                                 pPart->pNode->worldF);
                        worldPosition[0] = worldPosition[0] + nodePosition[0];
                        worldPosition[1] = worldPosition[1] + nodePosition[1];
                        worldPosition[2] = worldPosition[2] + nodePosition[2];
                        pVertex->x = worldPosition[0];
                        pVertex->y = worldPosition[1];
                        pVertex->z = worldPosition[2];
                        pEmittedPosition++;
                        pVertex++;
                        (pPart->vertexCount)++;
                    } else {
                        *(MeshVertexF **)((char *)pSourceIndex + referenceOffset + (pSourceIndex - sourceIndices) * (sizeof(*pFaceVertex) - sizeof(*pSourceIndex))) = pPart->pVertices + emittedIndex;
                    }
                    pSourceIndex++;
                } while (--verticesLeft);
                pFaceVertex += 3;
                (pPart->faceCount)++;
            }
            pFace += 0x26;
        } while (--remaining);
    }
}
