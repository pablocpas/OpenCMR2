// Runtime wiring for the cockpit windshield glass (video.windshield_cracks):
// resolves, per car, which CIN window quad is the front windshield, its
// damage break slot and exterior material group, and the exterior mesh's
// damaged/broken glass texture ids, so the interior glass can follow the real
// damage appearance. Every inconsistency resolves to "feature off" (-1) with
// a log line; nothing falls back to a hardcoded slot or decorative marks.
//
// Geometry lives in game/WindshieldGlass.h (unit-tested pure functions).

#include "port/types.h"
#include "WindshieldGlass.h"
#include "FixedPoint.h"
#include "SceneNode.h"
#include "Car.h"
#include "CarParts.h"
#include "CarInterior.h"
#include "StageTiming.h"
#include "StageBlock.h"
#include "GraphicsOptions.h"
#include "port/sys.h"
#include "port/gfx.h"
#include "Graphics.h"
#include <cstdio>
#include <cmath>

// CarEffects.cpp (the window layout owner).
const FixVector *CarEffects_CarWindowVertices(int car);
void CarEffects_GetWindowLayout(unsigned char vertex[6][4], int mirrorZ[6][4]);

#define WG_MAX_CARS 8

struct WindshieldGlassInfo {
    int resolved;        // 0 unresolved, 1 ok, -1 off (with reason logged)
    int window;          // CIN window quad index, -1 unknown
    int slot;            // damage break slot, -1 unknown
    unsigned group;      // exterior triangle group mask, 0 unknown
    int damagedTexture;  // exterior textures[] id, damaged state, -1 unknown
    int brokenTexture;   // exterior textures[] id, broken state, -1 unknown
};

static WindshieldGlassInfo g_wgInfo[WG_MAX_CARS];

static WgVec3 WgFromFix(const FixVector *v)
{
    WgVec3 r = {(float)v->x, (float)v->y, (float)v->z};
    return r;
}

// Accumulates a node's rest transform chain into car (body-root) space: the
// node's own `local` multiplied by each parent's `local` up to and including
// the body node, stopping before the car's scene placement.
static int WgNodePointInCarSpace(SceneNode *node, SceneNode *bodyNode, WgVec3 *out)
{
    if (node == NULL || bodyNode == NULL)
        return 0;
    FixMatrix acc;
    FixMatrix_Identity(&acc);
    int reached = 0;
    for (SceneNode *p = node; p != NULL; p = p->pParent) {
        FixMatrix next;
        FixMatrix_Multiply(&next, &p->local, &acc);
        acc = next;
        if (p == bodyNode) { reached = 1; break; }
    }
    if (!reached)
        return 0;   // node is not under the body: frame mismatch, stay off
    FixVector origin = {0, 0, 0}, world;
    FixMatrix_TransformPoint(&world, &origin, &acc);
    *out = WgFromFix(&world);
    return 1;
}

// Same group test as Game_SetTriangleField2CByGroup (the native damage swap):
// the mask's low 7 bits match the triangle flag's low 7 bits, its next 7 bits
// match flag bits 9..15. Group 0x100 is therefore flag bit 0x400 and group
// 0x80 is flag bit 0x200; a plain flags & group is not the native test.
static int WgTriInGroup(unsigned flags, unsigned group)
{
    unsigned low = group & 0x7f, high = (group >> 7) & 0x7f;
    return (flags & low) != 0 || (high & (flags >> 9)) != 0;
}

// Finds the exterior damaged/broken glass texture ids for one triangle group
// on the node-type-7 body mesh: the same entries the native swap selects
// (StageObject_SetModelSubmeshVisibility: damaged -> 3, broken -> 5).
static int WgExteriorGlassTextures(int car, unsigned group, int *damaged, int *broken)
{
    CarPartSet *pParts = StageTiming_GetCarReplayRecord(car);
    if (pParts == NULL)
        return 0;
    int i = StageTiming_FindModelPartByNodeType(7, pParts);
    if (i < 0)
        return 0;
    Mesh *pMesh = pParts->geometry.meshes[i];
    if (pMesh == NULL)
        return 0;
    for (int t = 0; t < pMesh->triangleCount; t++) {
        MeshTriangle *pTri = &pMesh->pTriangles[t];
        if (!WgTriInGroup(pTri->flags, group))
            continue;
        if (pTri->textures[3] > -1 && pTri->textures[5] > -1) {
            *damaged = pTri->textures[3];
            *broken = pTri->textures[5];
            return 1;
        }
        return 0;   // group found but its damage entries are not drawable
    }
    return 0;
}

static const char *g_wgFailReason[WG_MAX_CARS];
const char *WindshieldGlass_FailReason(int car)
{
    return (car >= 0 && car < WG_MAX_CARS && g_wgFailReason[car]) ? g_wgFailReason[car] : "not attempted";
}

int WindshieldGlass_ResolveForCar(int car)
{
    if (car < 0 || car >= WG_MAX_CARS)
        return -1;
    WindshieldGlassInfo *info = &g_wgInfo[car];
    if (info->resolved != 0)
        return info->resolved == 1 ? info->window : -1;
    info->resolved = -1;
    info->window = info->slot = -1;
    info->group = 0;
    info->damagedTexture = info->brokenTexture = -1;

    const FixVector *verts = CarEffects_CarWindowVertices(car);
    if (verts == NULL) {
        // Transient: the CIN glass profile can load after the first interior
        // attach. Leave resolved at 0 so the next setup retries instead of
        // latching the feature off for the whole stage.
        Sys_Log("windshield glass: car %d has no glass profile yet, will retry", car);
        info->resolved = 0;
        g_wgFailReason[car] = "no glass profile";
        return -1;
    }
    unsigned char desc[6][4];
    int mirror[6][4];
    CarEffects_GetWindowLayout(desc, mirror);
    int rawVerts[WG_WINDOWS][3];
    for (int w = 0; w < WG_WINDOWS; w++) {
        rawVerts[w][0] = verts[w].x;
        rawVerts[w][1] = verts[w].y;
        rawVerts[w][2] = verts[w].z;
    }
    WgQuad quads[WG_WINDOWS];
    WindshieldGlass_BuildQuads(rawVerts, desc, mirror, quads);

    Car *pCar = Car_Get(car);
    CarInteriorNodes *nodes = &g_carInteriorNodeRows[car];
    WgVec3 pivots[2];
    int pivotCount = 0;
    for (int w = 0; w < 2; w++) {
        if (nodes->wiperNodes[w] != NULL &&
            WgNodePointInCarSpace(nodes->wiperNodes[w], pCar ? pCar->pBodyNode : NULL, &pivots[pivotCount]))
            pivotCount++;
    }

    int window = -1;
    if (pivotCount > 0)
        window = WindshieldGlass_ClassifyByPivots(quads, pivots, pivotCount);

    // Cross-check with the rest eye ray when both references exist; a
    // disagreement means the geometry is ambiguous, so the feature stays off
    // rather than risk damaging the wrong window.
    CarInteriorProfilePointers *profile = &g_carInteriorProfilePointers[car];
    if (profile->driverPoseOffset != NULL && pivotCount > 0 && window >= 0) {
        WgVec3 eye = WgFromFix(profile->driverPoseOffset);
        WgVec3 centroid = {(pivots[0].x + pivots[pivotCount - 1].x) / 2.0f,
                           (pivots[0].y + pivots[pivotCount - 1].y) / 2.0f,
                           (pivots[0].z + pivots[pivotCount - 1].z) / 2.0f};
        WgVec3 dir = WgSub(centroid, eye);
        int ray = WindshieldGlass_Classify(quads, eye, dir);
        if (ray >= 0 && ray != window) {
            Sys_Log("windshield glass: car %d ambiguous front glass (pivot %d vs ray %d), feature off",
                    car, window, ray);
            g_wgFailReason[car] = "ambiguous front glass";
            return -1;
        }
    } else if (window < 0 && profile->driverPoseOffset != NULL) {
        // No usable wiper nodes: the eye ray alone, toward the front-most
        // quad centre ahead of the driver, is the only reference left.
        Sys_Log("windshield glass: car %d has no wiper pivots, feature off", car);
        g_wgFailReason[car] = "no wiper pivots";
        return -1;
    }
    if (window < 0) {
        Sys_Log("windshield glass: car %d front glass unresolved, feature off", car);
        g_wgFailReason[car] = "front glass unresolved";
        return -1;
    }

    int slot = WindshieldGlass_SlotForWindow(window);
    unsigned group = WindshieldGlass_GroupForSlot(slot);
    int damaged = -1, broken = -1;
    if (group == 0 || !WgExteriorGlassTextures(car, group, &damaged, &broken)) {
        Sys_Log("windshield glass: car %d window %d has no drawable damage textures yet, will retry",
                car, window);
        // Transient like the profile miss: the exterior part set may not be built at the first attach.
        // WindshieldGlass_RetryPending runs once more after StageObjects_Init; it is bounded to that one call per stage setup.
        info->resolved = 0;
        g_wgFailReason[car] = "no damage textures";
        return -1;
    }

    info->resolved = 1;
    g_wgFailReason[car] = "ok";
    info->window = window;
    info->slot = slot;
    info->group = group;
    info->damagedTexture = damaged;
    info->brokenTexture = broken;
    Sys_Log("windshield glass: car %d front window %d (slot %d, group 0x%x, textures %d/%d)",
            car, window, slot, group, damaged, broken);
    return window;
}

// Read-only view of the resolved state for the damage hook and the overlay.
int WindshieldGlass_GetInfo(int car, int *window, int *slot, unsigned *group,
                            int *damagedTexture, int *brokenTexture)
{
    if (car < 0 || car >= WG_MAX_CARS || g_wgInfo[car].resolved != 1)
        return 0;
    if (window) *window = g_wgInfo[car].window;
    if (slot) *slot = g_wgInfo[car].slot;
    if (group) *group = g_wgInfo[car].group;
    if (damagedTexture) *damagedTexture = g_wgInfo[car].damagedTexture;
    if (brokenTexture) *brokenTexture = g_wgInfo[car].brokenTexture;
    return 1;
}


// ---- interior glass selection and damage swap ------------------------------
// The interior (cockpit) model draws by MeshPart (Graphics_DrawMeshPartsByTexture),
// one texture per part, so the damage swap rewrites part->texture on the parts
// that form the FRONT windshield glass and restores them otherwise. A part is
// glass by the draw path's own predicate (mesh flag 0x8 + semi-transparent
// vertex alpha, Graphics.cpp Graphics_PartHasSemiTransparentAlpha); among the
// glass parts, the front ones are the parts whose car-space centroid sits on
// the resolved front window quad. Any mismatch leaves the part untouched.

#include "MeshVertex.h"

#define WG_MAX_GLASS_PARTS 32

struct WgGlassPart {
    MeshPart *part;      // part inside an interior mesh LOD record
    int originalTexture;
};

static WgGlassPart g_wgGlassParts[WG_MAX_CARS][WG_MAX_GLASS_PARTS];
static int g_wgGlassPartCount[WG_MAX_CARS];
static int g_wgAppliedState[WG_MAX_CARS];   // 0 intact, 1 damaged, 2 broken, -1 not applied

// Centroid of a part's referenced vertices, node-local (float game units).
static int WgPartCentroid(Mesh *pMesh, MeshPart *pPart, WgVec3 *out)
{
    if (pMesh->pVertexData == NULL || pPart->pData == NULL || pPart->indexCount <= 0)
        return 0;
    const MeshVertexF *verts = (const MeshVertexF *)pMesh->pVertexData;
    double x = 0, y = 0, z = 0;
    int n = 0;
    for (int i = 0; i < pPart->indexCount; i++) {
        int vi = pPart->minIndex + pPart->pData[i];
        if (vi < 0 || vi >= pMesh->field_0x10)
            return 0;
        x += verts[vi].x; y += verts[vi].y; z += verts[vi].z;
        n++;
    }
    out->x = (float)(x / n); out->y = (float)(y / n); out->z = (float)(z / n);
    return 1;
}

static int WgPartIsGlass(Mesh *pMesh, MeshPart *pPart)
{
    if ((pMesh->flags & 0x8u) == 0 || pMesh->pVertexData == NULL ||
        pPart->pData == NULL || pPart->indexCount <= 0)
        return 0;
    const MeshVertexF *verts = (const MeshVertexF *)pMesh->pVertexData;
    for (int i = 0; i < pPart->indexCount; i++) {
        DWORD d = verts[pPart->minIndex + pPart->pData[i]].diffuseColour;
        unsigned a = (unsigned)(d >> 24);
        if (a > 0 && a < 250)
            return 1;
    }
    return 0;
}

// Distance check against the resolved front quad: on-plane (within 10% of the
// quad diagonal) and inside the quad bounds expanded by 25%. Tolerances are
// relative so fixed/float unit scale differences do not leak in.
static int WgCentroidOnFrontQuad(WgVec3 c, const WgQuad *q)
{
    WgVec3 e1 = WgSub(q->v[1], q->v[0]), e2 = WgSub(q->v[3], q->v[0]);
    WgVec3 n = WgCross(e1, e2);
    float nl2 = WgDot(n, n);
    if (nl2 < 1e-6f)
        return 0;
    float diag = (float)sqrt((double)(WgDot(e1, e1) + WgDot(e2, e2)));
    WgVec3 d = WgSub(c, q->v[0]);
    float planeDist = (float)fabs((double)WgDot(d, n)) / (float)sqrt((double)nl2);
    if (planeDist > 0.10f * diag)
        return 0;
    // Inside the (possibly skewed) quad: project onto e1/e2 coordinates.
    float d11 = WgDot(e1, e1), d12 = WgDot(e1, e2), d22 = WgDot(e2, e2);
    float b1 = WgDot(d, e1), b2 = WgDot(d, e2);
    float det = d11 * d22 - d12 * d12;
    if (fabs((double)det) < 1e-6)
        return 0;
    float u = (b1 * d22 - b2 * d12) / det;
    float v = (b2 * d11 - b1 * d12) / det;
    return u > -0.25f && u < 1.25f && v > -0.25f && v < 1.25f;
}

static void WgCollectInteriorGlass(SceneNode *node, SceneNode *bodyNode, int car,
                                   const WgQuad *frontQuad)
{
    for (SceneNode *p = node; p != NULL; p = p->pNext) {
        if (p->type == SCENE_NODE_MESH && p->pObject != NULL) {
            // Accumulate this node's rest chain into car space.
            FixMatrix acc;
            FixMatrix_Identity(&acc);
            int reached = 0;
            for (SceneNode *q = p; q != NULL; q = q->pParent) {
                FixMatrix next;
                FixMatrix_Multiply(&next, &q->local, &acc);
                acc = next;
                if (q == bodyNode) { reached = 1; break; }
            }
            if (reached) {
                Mesh *pMesh = (Mesh *)p->pObject;
                int lods = pMesh->lodCount > 0 ? pMesh->lodCount : 1;
                for (int l = 0; l < lods; l++) {
                    Mesh *pLod = (Mesh *)((BYTE *)pMesh + l * offsetof(Mesh, field_0x108));
                    for (int pi = 0; pi < pLod->partCount && pi < 50; pi++) {
                        MeshPart *pPart = pLod->pParts[pi];
                        if (pPart == NULL || !WgPartIsGlass(pLod, pPart))
                            continue;
                        WgVec3 local;
                        if (!WgPartCentroid(pLod, pPart, &local))
                            continue;
                        FixVector fx = {(int)(local.x * 65536.0f), (int)(local.y * 65536.0f),
                                        (int)(local.z * 65536.0f)}, fw;
                        FixMatrix_TransformPoint(&fw, &fx, &acc);
                        WgVec3 carSpace = {fw.x / 65536.0f, fw.y / 65536.0f, fw.z / 65536.0f};
                        if (!WgCentroidOnFrontQuad(carSpace, frontQuad))
                            continue;
                        if (g_wgGlassPartCount[car] < WG_MAX_GLASS_PARTS) {
                            WgGlassPart *slot = &g_wgGlassParts[car][g_wgGlassPartCount[car]++];
                            slot->part = pPart;
                            slot->originalTexture = pPart->texture;
                        }
                    }
                }
            }
        }
        if (p->pFirstChild != NULL)
            WgCollectInteriorGlass(p->pFirstChild, bodyNode, car, frontQuad);
    }
}

// Failure-path diagnostics: when no interior part survives the glass
// predicate + front-quad test, report what WAS there so a native log says
// exactly which condition excluded the real asset. Runs only on the failure
// path (feature would otherwise silently stay off).
static void WgDiagnoseInterior(SceneNode *node, SceneNode *bodyNode, int car,
                               const WgQuad *frontQuad, int *stats)
{
    for (SceneNode *p = node; p != NULL; p = p->pNext) {
        if (p->type == SCENE_NODE_MESH && p->pObject != NULL) {
            FixMatrix acc;
            FixMatrix_Identity(&acc);
            int reached = 0;
            for (SceneNode *q = p; q != NULL; q = q->pParent) {
                FixMatrix next;
                FixMatrix_Multiply(&next, &q->local, &acc);
                acc = next;
                if (q == bodyNode) { reached = 1; break; }
            }
            Mesh *pMesh = (Mesh *)p->pObject;
            int lods = pMesh->lodCount > 0 ? pMesh->lodCount : 1;
            for (int l = 0; l < lods; l++) {
                Mesh *pLod = (Mesh *)((BYTE *)pMesh + l * offsetof(Mesh, field_0x108));
                stats[0]++;                                   // mesh-lods visited
                if (reached) stats[1]++;                      // under the body node
                if ((pLod->flags & 0x8u) != 0) stats[2]++;    // alpha-blend meshes
                for (int pi = 0; pi < pLod->partCount && pi < 50; pi++) {
                    MeshPart *pPart = pLod->pParts[pi];
                    if (pPart == NULL || pPart->pData == NULL || pPart->indexCount <= 0 ||
                        pLod->pVertexData == NULL)
                        continue;
                    const MeshVertexF *verts = (const MeshVertexF *)pLod->pVertexData;
                    int semi = 0;
                    for (int i = 0; i < pPart->indexCount; i++) {
                        unsigned a = (unsigned)(verts[pPart->minIndex + pPart->pData[i]].diffuseColour >> 24);
                        if (a > 0 && a < 250) { semi = 1; break; }
                    }
                    if (!semi)
                        continue;
                    stats[3]++;                               // semi-transparent parts (any mesh flags)
                    WgVec3 local;
                    if (!WgPartCentroid(pLod, pPart, &local))
                        continue;
                    FixVector fx = {(int)(local.x * 65536.0f), (int)(local.y * 65536.0f),
                                    (int)(local.z * 65536.0f)}, fw;
                    FixMatrix_TransformPoint(&fw, &fx, &acc);
                    WgVec3 carSpace = {fw.x / 65536.0f, fw.y / 65536.0f, fw.z / 65536.0f};
                    Sys_Log("windshield glass: diag car %d semi-part meshFlags=%x reached=%d centroid %.3f %.3f %.3f onQuad=%d",
                            car, pLod->flags, reached, carSpace.x, carSpace.y, carSpace.z,
                            reached ? WgCentroidOnFrontQuad(carSpace, frontQuad) : -1);
                }
            }
        }
        if (p->pFirstChild != NULL)
            WgDiagnoseInterior(p->pFirstChild, bodyNode, car, frontQuad, stats);
    }
}

// Restores every patched part (stage unload / cache reset).
void WindshieldGlass_RestoreAll(void)
{
    for (int car = 0; car < WG_MAX_CARS; car++) {
        for (int i = 0; i < g_wgGlassPartCount[car]; i++)
            g_wgGlassParts[car][i].part->texture = g_wgGlassParts[car][i].originalTexture;
        g_wgAppliedState[car] = 0;
    }
}

// Applies (or restores) the damage appearance on the selected interior front
// glass parts. state: 0 intact, 1 damaged, 2 broken. Returns the applied
// state, or -1 when nothing is selected/resolved (feature off).
int WindshieldGlass_ApplyDamageState(int car, int state)
{
    if (car < 0 || car >= WG_MAX_CARS)
        return -1;
    if (g_wgInfo[car].resolved != 1 || g_wgGlassPartCount[car] == 0)
        return -1;
    if (state == g_wgAppliedState[car])
        return state;
    int texture = g_wgGlassParts[car][0].originalTexture;
    if (state == 1) texture = g_wgInfo[car].damagedTexture;
    else if (state == 2) texture = g_wgInfo[car].brokenTexture;
    for (int i = 0; i < g_wgGlassPartCount[car]; i++)
        g_wgGlassParts[car][i].part->texture =
            state == 0 ? g_wgGlassParts[car][i].originalTexture : texture;
    g_wgAppliedState[car] = state;
    return state;
}

// Full per-car setup after the interior model is attached: resolve the front
// window, then select the interior glass parts on it. Safe to call again
// after a stage reload (WindshieldGlass_ResetCache).
int WindshieldGlass_SetupCar(int car)
{
    if (WindshieldGlass_ResolveForCar(car) < 0)
        return -1;
    Car *pCar = Car_Get(car);
    if (pCar == NULL || pCar->pBodyNode == NULL)
        return -1;
    // Rebuild the front quad for the selection check.
    const FixVector *verts = CarEffects_CarWindowVertices(car);
    unsigned char desc[6][4];
    int mirror[6][4];
    CarEffects_GetWindowLayout(desc, mirror);
    int rawVerts[WG_WINDOWS][3];
    for (int w = 0; w < WG_WINDOWS; w++) {
        rawVerts[w][0] = verts[w].x;
        rawVerts[w][1] = verts[w].y;
        rawVerts[w][2] = verts[w].z;
    }
    WgQuad quads[WG_WINDOWS];
    WindshieldGlass_BuildQuads(rawVerts, desc, mirror, quads);
    WgQuad front = quads[g_wgInfo[car].window];
    for (int j = 0; j < 4; j++) {   // CIN fixed 16.16 -> float game units
        front.v[j].x /= 65536.0f;
        front.v[j].y /= 65536.0f;
        front.v[j].z /= 65536.0f;
    }
    g_wgGlassPartCount[car] = 0;
    g_wgAppliedState[car] = 0;
    if (g_carInteriorRoots[car] == NULL)
        return -1;
    WgCollectInteriorGlass(g_carInteriorRoots[car], pCar->pBodyNode, car, &front);
    if (g_wgGlassPartCount[car] == 0) {
        Sys_Log("windshield glass: car %d front interior glass not found, feature off", car);
        Sys_Log("windshield glass: diag car %d front quad (%.3f %.3f %.3f) (%.3f %.3f %.3f) (%.3f %.3f %.3f) (%.3f %.3f %.3f)",
                car, front.v[0].x, front.v[0].y, front.v[0].z, front.v[1].x, front.v[1].y, front.v[1].z,
                front.v[2].x, front.v[2].y, front.v[2].z, front.v[3].x, front.v[3].y, front.v[3].z);
        int stats[4] = {0, 0, 0, 0};
        WgDiagnoseInterior(g_carInteriorRoots[car], pCar->pBodyNode, car, &front, stats);
        Sys_Log("windshield glass: diag car %d lods=%d underBody=%d blendMeshes=%d semiParts=%d",
                car, stats[0], stats[1], stats[2], stats[3]);
        if (Sys_GetOption("video.windshield_crack_overlay", 0) != 0) {
            // The interior model has no glass pane to retexture (the Subaru cockpit
            // is open at the windscreen). Keep the resolved front window and its
            // damage textures: WindshieldGlass_DrawCrackOverlay draws them on the
            // projected window quad instead.
            g_wgFailReason[car] = "overlay pending";
            return g_wgInfo[car].window;
        }
        g_wgInfo[car].resolved = -1;
        g_wgFailReason[car] = "no interior glass part";
        return -1;
    }
    Sys_Log("windshield glass: car %d selected %d interior glass parts", car, g_wgGlassPartCount[car]);
    return g_wgInfo[car].window;
}

// ---- world-space crack pane (video.windshield_crack_overlay, default off) ----
// Cockpits whose interior model has no glass part (see "no interior glass part")
// get the car's own exterior front-glass triangles drawn again in the 3D pass:
// the same vertices, UVs and node transform the body mesh uses, with the
// damaged/broken texture the native swap would select (textures[3] / [5] of the
// triangle), alpha blended with depth test on and depth write off, so the dash,
// pillars and mirror hide it through the real depth buffer. The glass is the
// inner face set (flag 0x8000 clear), the one that faces the cockpit; the 0x8000
// faces are the outer set. Nothing is derived from the CIN window quad except
// the window/group choice the resolver already made.
int WindshieldGlass_DrawCrackPane(int car, int state)
{
    if (state < 1 || Sys_GetOption("video.windshield_crack_overlay", 0) == 0)
        return 0;
    if (car < 0 || car >= WG_MAX_CARS || g_wgInfo[car].resolved != 1 || g_wgGlassPartCount[car] != 0)
        return 0;   // not this cockpit's problem: the interior glass parts handle it
    CarPartSet *pParts = StageTiming_GetCarReplayRecord(car);
    int mi = pParts != NULL ? StageTiming_FindModelPartByNodeType(7, pParts) : -1;
    Mesh *pMesh = mi >= 0 ? pParts->geometry.meshes[mi] : NULL;
    SceneNode *node = mi >= 0 ? pParts->geometry.nodes[mi] : NULL;
    if (pMesh == NULL || node == NULL || pMesh->pVertexData == NULL || pMesh->pTriangles == NULL) {
        g_wgFailReason[car] = "overlay: no exterior body mesh";
        return 0;
    }
    const MeshVertexF *verts = (const MeshVertexF *)pMesh->pVertexData;
    enum { MAXT = 24 };
    GfxVertex gv[MAXT * 3];
    int tex[MAXT];
    int nt = 0, pureSeen = 0;
    for (int pass = 0; pass < 2 && nt == 0; pass++) {   // inner faces first, any face as the fallback
        for (int t = 0; t < pMesh->triangleCount && nt < MAXT; t++) {
            MeshTriangle *tri = &pMesh->pTriangles[t];
            if (!WgTriInGroup(tri->flags, g_wgInfo[car].group))
                continue;
            if (pass == 0 && (tri->flags & 0x8000) != 0)
                continue;
            int tx = tri->textures[state >= 2 ? 5 : 3];
            if (tx < 0 || tx >= 2048 || CGraphics::GetTextureBuffer()[tx] == NULL ||
                CGraphics::GetTextureBuffer()[tx]->pSurface == NULL)
                continue;
            int ok = 1;
            for (int k = 0; k < 3; k++)
                if (tri->vertexIndex[k] >= pMesh->field_0x10) ok = 0;
            if (!ok) continue;
            for (int k = 0; k < 3; k++) {
                const MeshVertexF *v = &verts[tri->vertexIndex[k]];
                GfxVertex *o = &gv[nt * 3 + k];
                o->x = v->x; o->y = v->y; o->z = v->z;
                o->nx = v->nx; o->ny = v->ny; o->nz = v->nz;
                o->diffuse = 0xffffffff; o->specular = 0;
                o->u0 = v->textureCoordinates[0][0]; o->v0 = v->textureCoordinates[0][1];
                o->u1 = 0; o->v1 = 0;
            }
            tex[nt++] = tx;
            if (pass == 0) pureSeen = 1;
        }
    }
    if (nt == 0) {
        g_wgFailReason[car] = "overlay: no drawable exterior glass triangles";
        return 0;
    }
    GfxMatrix wm;
    const int *m = (const int *)&node->world;
    float *wf = (float *)&wm;
    for (int i = 0; i < 16; i++) wf[i] = m[i] / 65536.0f;
    static int s_logged[WG_MAX_CARS];
    if (s_logged[car] != state) {
        s_logged[car] = state;
        Sys_Log("windshield glass: car %d crack pane state %d: %d triangles (%s faces), texture %d, group 0x%x, node world pos (%.2f %.2f %.2f)",
                car, state, nt, pureSeen ? "inner" : "fallback", tex[0], g_wgInfo[car].group,
                wf[12], wf[13], wf[14]);
    }
    Gfx_PushState();
    Gfx_SetTransform(GFX_TRANSFORM_WORLD, &wm);
    Gfx_SetLighting(FALSE);
    Gfx_SetDepthTest(TRUE);
    Gfx_SetDepthWrite(FALSE);
    Gfx_SetCullMode(GFX_CULL_NONE);
    Gfx_SetAlphaBlend(TRUE);
    Gfx_SetSrcBlend(GFX_BLEND_SRCALPHA);
    Gfx_SetDestBlend(GFX_BLEND_INVSRCALPHA);
    Gfx_SetStageColorOp(0, GFX_TOP_MODULATE);
    Gfx_SetStageColorArg1(0, GFX_TA_TEXTURE);
    Gfx_SetStageColorArg2(0, GFX_TA_DIFFUSE);
    Gfx_SetStageAlphaOp(0, GFX_TOP_MODULATE);
    Gfx_SetStageAlphaArg1(0, GFX_TA_TEXTURE);
    Gfx_SetStageAlphaArg2(0, GFX_TA_DIFFUSE);
    Gfx_SetStageColorOp(1, GFX_TOP_DISABLE);
    Gfx_SetStageAlphaOp(1, GFX_TOP_DISABLE);
    for (int t = 0; t < nt; t++) {
        Gfx_SetTexture(0, CGraphics::GetTextureBuffer()[tex[t]]->pSurface);
        Gfx_DrawPrimitive(GFX_TRIANGLELIST, &gv[t * 3], 3);
    }
    Gfx_SetTexture(0, NULL);
    Gfx_PopState();
    g_wgFailReason[car] = "overlay drawn";
    return 1;
}

// Second attempt for cars whose first setup hit a transient miss (resolved == 0:
// no glass profile / no damage textures yet). Called once after StageObjects_Init,
// when CarEffects_Init has filled its tables and the car part sets exist. Cars that
// resolved (1) or failed for a deterministic reason (-1) are not touched, and a car
// that misses again stays at 0 with its reason shown in F10; there is no further loop.
void WindshieldGlass_RetryPending(void)
{
    for (int car = 0; car < WG_MAX_CARS; car++) {
        if (g_wgInfo[car].resolved != 0 || g_wgFailReason[car] == NULL || g_carInteriorRoots[car] == NULL)
            continue;   // resolved, deterministic failure, never attempted this stage, or no interior
        if (!Sys_GetOption("video.windshield_cracks", 1))
            continue;
        WindshieldGlass_SetupCar(car);
    }
}

void WindshieldGlass_ResetCache(void)
{
    WindshieldGlass_RestoreAll();
    for (int i = 0; i < WG_MAX_CARS; i++) {
        g_wgInfo[i].resolved = 0;
        g_wgFailReason[i] = NULL;   // "never attempted" for the next stage; RetryPending only retries a miss from this stage
        g_wgGlassPartCount[i] = 0;
        g_wgAppliedState[i] = 0;
    }
}
