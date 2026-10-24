// PORT: the far layer, the stage beyond its view distance.
//
// The original draws only the sectors within the route's view distance, so
// hills, trees and objects appear at once as their sector comes within it.
// The stage is all loaded, but it is not built to be seen from every distance
// (hill tops with nothing under them, gaps), so its view distance stays.
// Instead, a layer drawn behind it holds the whole stage out to FAR_DISTANCE:
// its ground meshes, static objects and camera-facing objects as the stage
// draws them, over a solid height grid of its ground that fills what is
// hollow, and the land around the stage, generated from its edges with rolling
// hills and forest. The stage, drawn over the layer with the depth buffer
// cleared, hides it wherever it has something and dissolves into it toward its
// far plane (Game_BeginDistanceFade): it fades into itself, so nothing appears
// or moves as it comes into view. The stage's own copies in the layer leave
// out what is nearer than a share of the view distance
// (Gfx_SetDistanceClip), where the stage covers them.

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <unordered_map>
#include "FarLayer.h"
#include "Sector.h"
#include "SceneNode.h"
#include "Graphics.h"
#include "Game.h"
#include "Mesh.h"
#include "port/gfx.h"
#include "port/sys.h"

void Graphics_SwitchAlphaBlendAndTest(int enable);
void Graphics_SetLightingMode(int mode);
void Graphics_EnableFog(void);
void Graphics_DrawMeshLOD(Mesh *pMesh, int useParts, int clampTexture, int markTextures);
void Frontend_SetObjectField118(struct Texture *pTexture, int value);
void Scene_RelightSector(int sector);
extern GfxMatrix g_identityWorldMatrix;
extern GfxMatrix g_unk0x00597cc0;   // the camera's turn about the vertical (Game_DrawSceneViewport)
extern FixVector g_sceneAmbient;    // the scene's light ramp (Scene_BuildLightTables): ambient,
extern FixVector g_sceneLightColour;    // and light at full level less ambient
extern void *g_sceneSectorLights;   // per sector, the light it was last lit with (Scene_RelightSector)
void FarLayer_ApplyTexture(Texture *pTexture);

namespace {

const float CELL = 20.0f;           // grid step (m)
const float MARGIN = 1000.0f;       // generated land around the stage (m)
const float FAR_DISTANCE = 10000.0f;    // the layer's far plane (m), beyond the end of its land: nothing
                                        // of it crosses the plane, so nothing appears there
const float NEAR_DISTANCE = 4.0f;       // its near plane (m), for the depth buffer's precision that far
const float RIM = 400.0f;           // the generated land rises to a ridge over this much of its outer edge (m)
const float RIM_HEIGHT = 80.0f;     // the ridge's height (m)
const float CLIP_SHARE = 0.8f;      // the stage's copy leaves out what is nearer than this share of the view distance
                                    // (the stage fades into it from 0.85, Game_BeginDistanceFade)
const float MARK_SHARE = 0.5f;      // tests: the layer is marked near within this share of the view distance
const float SINK = 3.0f;            // the height grid below the stage's ground (m)
const float HILLS = 40.0f;          // height of the generated hills (m)
const float GROUND_SLOPE = 0.35f;   // normal y of the steepest ground triangle
const float STEEP_SLOPE = 0.82f;    // normal y under which a cell takes the steep texture
const float TREE_SLOPE = 0.85f;     // normal y under which no tree is planted
const float TREE_HEIGHT = 5.0f;     // camera-facing objects taller than this serve as trees (m)
const int TREE_CLEARANCE = 3;       // generated trees keep this many cells off the stage
const int CHUNK = 16;               // cells per chunk side
const float STAGE_CHUNK = 640.0f;   // side of the areas the stage's ground and objects are merged by (m)

struct Batch {
    int texture;
    bool trees;         // generated trees, lit as the stage lights its own
    std::vector<GfxVertex> vertices;
    GfxVertexBuffer *pBuffer;
    DWORD count;
};

struct Chunk {
    float x, z, radius;
    std::vector<Batch> batches;
};

struct TreeTemplate {
    Mesh *pMesh;
    const int *pLevels;     // its light levels: per vertex, or level for all
    int level;
    float sx, sy, sz;
    float base;     // height of the mesh's origin above its lowest point
};

struct EdgeKey {
    int a[3], b[3];
    bool operator==(const EdgeKey &o) const { return memcmp(this, &o, sizeof(*this)) == 0; }
};

struct EdgeHash {
    size_t operator()(const EdgeKey &k) const
    {
        size_t h = 1469598103934665603ull;
        for (int i = 0; i < 3; i++)
            h = ((h ^ (size_t)(unsigned)k.a[i]) * 1099511628211ull ^ (size_t)(unsigned)k.b[i]) * 1099511628211ull;
        return h;
    }
};

std::vector<Chunk> s_chunks;
std::vector<float> s_sectorBounds;  // per sector: centre x, z and radius in x/z

// Per sector, its static objects and its camera-facing ones (trees, bushes).
std::vector<const StageObject *> s_staticObjects, s_facingObjects;
std::vector<int> s_sectorStatic, s_sectorFacing;    // per sector, its first (and one past the last)
bool s_lit;                          // the scene lights the stage (Scene_RelightSector)

// The stage's ground, static objects and camera-facing objects, merged by
// area of STAGE_CHUNK and by texture and drawing group, so that a frame makes
// a few draws for them; the camera-facing ones as billboards
// (Gfx_SetBillboards), which the GPU turns to the camera. Meshes drawn
// otherwise (blended, environment mapped, in texture runs, or in a lighting
// mode of their own) are drawn one by one as the stage draws them.
enum { GROUND, OBJECTS, OBJECTS_TWO_SIDED, FACING, GROUPS };
struct StageBatch {
    int texture, group, lighting;
    std::vector<GfxVertex> vertices;
    GfxVertexBuffer *pBuffer;
    DWORD count;
};
struct StageChunk {
    float x, z, radius;
    std::vector<int> sectors;
    std::vector<StageBatch> batches;
    std::vector<Mesh *> others;         // the ground meshes and objects drawn one by one
    std::vector<int> othersGroup;
};
std::vector<StageChunk> s_stageChunks;
std::vector<int> s_sectorChunk;     // per sector, its chunk
int s_chunksX, s_chunksZ;
Mesh *s_builtFor;
int s_builtSectors = -1;
int s_testDrawn = 1, s_testFade = 1, s_testMarks = 0, s_testKeySky = 0;   // FarLayer_SetTest

float Hash(int x, int z, int seed)
{
    unsigned h = (unsigned)x * 374761393u + (unsigned)z * 668265263u + (unsigned)seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return (float)((h ^ (h >> 16)) & 0xffff) / 65535.0f;
}

float ValueNoise(float x, float z, int seed)
{
    int ix = (int)floorf(x), iz = (int)floorf(z);
    float fx = x - ix, fz = z - iz;
    fx = fx * fx * (3 - 2 * fx);
    fz = fz * fz * (3 - 2 * fz);
    float a = Hash(ix, iz, seed), b = Hash(ix + 1, iz, seed);
    float c = Hash(ix, iz + 1, seed), d = Hash(ix + 1, iz + 1, seed);
    float bottom = a + (b - a) * fx, top = c + (d - c) * fx;
    return bottom + (top - bottom) * fz;
}

// Rolling hills of 0..1, a few octaves from 400 m across.
float Hills(float x, float z)
{
    float sum = 0, amplitude = 1, frequency = 1.0f / 400.0f, total = 0;
    for (int octave = 0; octave < 4; octave++) {
        sum += ValueNoise(x * frequency, z * frequency, octave) * amplitude;
        total += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    return sum / total;
}

// y of the triangle's unit normal, 0 when it has no area.
float NormalY(const GfxVertex &a, const GfxVertex &b, const GfxVertex &c)
{
    float e1[3] = { b.x - a.x, b.y - a.y, b.z - a.z }, e2[3] = { c.x - a.x, c.y - a.y, c.z - a.z };
    float n[3] = { e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0] };
    float length = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    return length > 0.0f ? fabsf(n[1]) / length : 0.0f;
}

// Calls f(pMesh, pTriangle, a, b, c) for every triangle of the stage's ground
// meshes.
template <class F> void ForEachStageTriangle(F f)
{
    for (int s = 0; s < g_sectorCount; s++) {
        Mesh *pMesh = g_sectors[s] != NULL ? g_sectors[s]->pMesh : NULL;
        if (pMesh == NULL || pMesh->pTriangles == NULL || pMesh->pVertexData == NULL)
            continue;
        const GfxVertex *v = (const GfxVertex *)pMesh->pVertexData;
        for (int t = 0; t < pMesh->triangleCount; t++) {
            MeshTriangle *pTri = &pMesh->pTriangles[t];
            f(pMesh, pTri, v[pTri->vertexIndex[0]], v[pTri->vertexIndex[1]], v[pTri->vertexIndex[2]]);
        }
    }
}

void ApplyTexture(Texture *pTexture)
{
    FarLayer_ApplyTexture(pTexture);
}

int TriangleTexture(const MeshTriangle *pTri)
{
    return pTri->textures[pTri->field_0x2c];
}

// Breadth-first distance (in steps) from the seeds over a w x h grid; each
// reached point calls spread(from, to).
template <class F> void Spread(std::vector<int> &distance, int w, int h, F spread)
{
    std::vector<size_t> queue;
    for (size_t i = 0; i < distance.size(); i++)
        if (distance[i] == 0)
            queue.push_back(i);
    static const int steps[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
    for (size_t q = 0; q < queue.size(); q++) {
        size_t i = queue[q];
        int x = (int)(i % w), z = (int)(i / w);
        for (const auto &step : steps) {
            int nx = x + step[0], nz = z + step[1];
            if (nx < 0 || nz < 0 || nx >= w || nz >= h)
                continue;
            size_t j = (size_t)nz * w + nx;
            if (distance[j] >= 0)
                continue;
            distance[j] = distance[i] + 1;
            spread(i, j);
            queue.push_back(j);
        }
    }
}

Batch &BatchFor(Chunk &chunk, int texture, bool trees)
{
    for (Batch &b : chunk.batches)
        if (b.texture == texture && b.trees == trees)
            return b;
    chunk.batches.push_back(Batch());
    Batch &b = chunk.batches.back();
    b.texture = texture;
    b.trees = trees;
    b.pBuffer = NULL;
    b.count = 0;
    return b;
}

// The stage's trees, as templates for the generated forest: camera-facing
// objects taller than TREE_HEIGHT.
void CollectTrees(std::vector<TreeTemplate> &templates)
{
    for (int s = 0; s < g_sectorCount; s++) {
        if (g_sectors[s] == NULL)
            continue;
        for (const StageObject *pObject = g_sectors[s]->pObjects; pObject != NULL; pObject = pObject->pNext) {
            Mesh *pMesh = pObject->pMesh;
            if (pMesh == NULL || (pMesh->flags & 2) == 0 || pMesh->pTriangles == NULL || pMesh->pVertexData == NULL)
                continue;
            const GfxVertex *v = (const GfxVertex *)pMesh->pVertexData;
            float low = 1e30f, high = -1e30f;
            for (int i = 0; i < pMesh->field_0x10; i++) {
                low = fminf(low, v[i].y);
                high = fmaxf(high, v[i].y);
            }
            // The placement matrix carries the scale on its diagonal.
            TreeTemplate t = { pMesh, (pMesh->flags & 0x80) != 0 ? pMesh->pLightLevels : NULL, pObject->lightLevel,
                               pObject->matrix.right.x / 65536.0f, pObject->matrix.up.y / 65536.0f,
                               pObject->matrix.forward.z / 65536.0f, 0.0f };
            t.base = -low * t.sy;
            if ((high - low) * t.sy > TREE_HEIGHT)
                templates.push_back(t);
        }
    }
}

// The layer's colours follow the scene's light on the GPU (Gfx_SetLightRamp):
// each vertex holds the step of the light ramp its light level falls on, as
// Scene_GetLightColourD3D picks it, and a shade, in its specular colour.
const DWORD KEEP_COLOUR = 0xff000000;   // no level: the vertex's own colour

DWORD RampSpecular(int level, float shade)
{
    int step = level * 0x31 >> 16;
    step = step < 0 ? 0 : step > 0x31 ? 0x31 : step;
    int red = (int)(shade * 128.0f + 0.5f);
    red = red < 0 ? 0 : red > 255 ? 255 : red;
    return (DWORD)step << 24 | (DWORD)red << 16;
}

// The specular colour of a mesh's vertex: its light level, per vertex
// (pLevels) or the same for all (level), when the scene lights the stage.
DWORD VertexRamp(const GfxVertex &v, int index, const int *pLevels, int level)
{
    return s_lit ? RampSpecular(pLevels != NULL ? pLevels[index] : level, 1.0f) : v.specular;
}

// A camera-facing mesh at (x, y, z) for its origin, scaled by (sx, sy, sz), as
// billboard vertices (Gfx_SetBillboards), lit by its light levels (pLevels or
// level, VertexRamp), appended to out by part.
template <class Out>
void AddFacing(const Mesh *pMesh, const int *pLevels, int level, float x, float y, float z, float sx, float sy, float sz,
               Out out)
{
    const GfxVertex *source = (const GfxVertex *)pMesh->pVertexData;
    for (int p = 0; p < pMesh->partCount; p++) {
        const MeshPart *pPart = pMesh->pParts[p];
        std::vector<GfxVertex> &vertices = out(pPart->texture);
        for (int k = 0; k < pPart->indexCount; k++) {
            int index = pPart->minIndex + pPart->pData[k];
            GfxVertex v = source[index];
            v.specular = VertexRamp(v, index, pLevels, level);
            v.nx = v.x * sx;
            v.ny = v.y * sy;
            v.nz = sz;
            v.x = x;
            v.y = y;
            v.z = z;
            vertices.push_back(v);
        }
    }
}

DWORD Scale(DWORD colour, float scale)
{
    BYTE *c = (BYTE *)&colour;
    for (int k = 0; k < 3; k++)
        c[k] = (BYTE)fminf(c[k] * scale, 255.0f);
    return colour;
}

void Release(void)
{
    for (Chunk &c : s_chunks)
        for (Batch &b : c.batches)
            if (b.pBuffer != NULL)
                Gfx_DestroyVertexBuffer(b.pBuffer);
    s_chunks.clear();
    for (StageChunk &c : s_stageChunks)
        for (StageBatch &b : c.batches)
            if (b.pBuffer != NULL)
                Gfx_DestroyVertexBuffer(b.pBuffer);
    s_stageChunks.clear();
}

// Whether a mesh draws as the merged batches do: lit in the mode of its group
// (flag 0x20 keeps the mode), opaque, by its parts, without an environment
// map.
bool Mergeable(const Mesh *pMesh)
{
    return (pMesh->flags & 0x20) != 0 && (pMesh->flags & (0x8 | 0x200 | 0x1000 | 0x40000)) == 0 &&
           pMesh->lodIndex == 0;
}

StageBatch &StageBatchFor(StageChunk &chunk, int texture, int group, int lighting)
{
    for (StageBatch &b : chunk.batches)
        if (b.texture == texture && b.group == group && b.lighting == lighting)
            return b;
    chunk.batches.push_back(StageBatch());
    StageBatch &b = chunk.batches.back();
    b.texture = texture;
    b.group = group;
    b.lighting = lighting;
    b.pBuffer = NULL;
    b.count = 0;
    return b;
}

// Appends the mesh's parts, lit by its light levels (pLevels or level,
// VertexRamp), to the chunk's batches.
void MergeMesh(StageChunk &chunk, const Mesh *pMesh, int group, const int *pLevels, int level)
{
    const GfxVertex *source = (const GfxVertex *)pMesh->pVertexData;
    for (int p = 0; p < pMesh->partCount; p++) {
        const MeshPart *pPart = pMesh->pParts[p];
        std::vector<GfxVertex> &vertices = StageBatchFor(chunk, pPart->texture, group, 0).vertices;
        for (int k = 0; k < pPart->indexCount; k++) {
            int index = pPart->minIndex + pPart->pData[k];
            GfxVertex v = source[index];
            v.specular = VertexRamp(v, index, pLevels, level);
            vertices.push_back(v);
        }
    }
}

// The chunk's batches from its sectors' meshes, uploaded once: the scene's
// light changes their colours on the GPU (Gfx_SetLightRamp). Ground is lit
// per vertex, objects per vertex when flagged 0x80 and else by their own
// level (Scene_RelightSector).
void MergeChunk(StageChunk &chunk)
{
    for (int s : chunk.sectors) {
        Mesh *pGround = g_sectors[s]->pMesh;
        if (pGround != NULL && pGround->pTriangles != NULL) {
            if (Mergeable(pGround) && (!s_lit || pGround->pLightLevels != NULL))
                MergeMesh(chunk, pGround, GROUND, pGround->pLightLevels, 0);
            else {
                chunk.others.push_back(pGround);
                chunk.othersGroup.push_back(GROUND);
            }
        }
        for (int o = s_sectorStatic[s]; o < s_sectorStatic[s + 1]; o++) {
            Mesh *pMesh = s_staticObjects[o]->pMesh;
            int group = (pMesh->flags & 1) != 0 ? OBJECTS_TWO_SIDED : OBJECTS;
            const int *pLevels = (pMesh->flags & 0x80) != 0 ? pMesh->pLightLevels : NULL;
            if (Mergeable(pMesh) && (!s_lit || (pMesh->flags & 0x80) == 0 || pLevels != NULL))
                MergeMesh(chunk, pMesh, group, pLevels, s_staticObjects[o]->lightLevel);
            else {
                chunk.others.push_back(pMesh);
                chunk.othersGroup.push_back(OBJECTS);
            }
        }
        // Meshes flagged 0x20 keep the lighting mode they are drawn in, mode
        // 1 where the stage draws its objects (Game_DrawSceneViewport).
        for (int o = s_sectorFacing[s]; o < s_sectorFacing[s + 1]; o++) {
            const StageObject *pObject = s_facingObjects[o];
            const Mesh *pMesh = pObject->pMesh;
            int lighting = (pMesh->flags & 0x20) != 0 ? 1 : (pMesh->flags & 0x40000) != 0 ? 3 : 2;
            AddFacing(pMesh, (pMesh->flags & 0x80) != 0 ? pMesh->pLightLevels : NULL, pObject->lightLevel,
                      pObject->matrix.position.x / 65536.0f, pObject->matrix.position.y / 65536.0f,
                      pObject->matrix.position.z / 65536.0f, pObject->matrix.right.x / 65536.0f,
                      pObject->matrix.up.y / 65536.0f, pObject->matrix.forward.z / 65536.0f,
                      [&](int texture) -> std::vector<GfxVertex> & {
                          return StageBatchFor(chunk, texture, FACING, lighting).vertices;
                      });
        }
    }
    for (StageBatch &b : chunk.batches) {
        b.count = (DWORD)b.vertices.size();
        b.pBuffer = Gfx_CreateVertexBuffer(b.count);
        memcpy(Gfx_LockVertexBuffer(b.pBuffer), b.vertices.data(), b.count * sizeof(GfxVertex));
        Gfx_UnlockVertexBuffer(b.pBuffer);
        Gfx_KeepVertexBuffer(b.pBuffer);
        std::vector<GfxVertex>().swap(b.vertices);
    }
}

// The sectors gathered into chunks of STAGE_CHUNK by their centres.
void MergeStage(void)
{
    s_sectorChunk.assign(g_sectorCount, -1);
    std::unordered_map<long long, int> chunkAt;
    for (int s = 0; s < g_sectorCount; s++) {
        if (g_sectors[s] == NULL)
            continue;
        float cx = s_sectorBounds[(size_t)s * 3], cz = s_sectorBounds[(size_t)s * 3 + 1];
        long long key = (long long)floorf(cx / STAGE_CHUNK) * 1000003ll + (long long)floorf(cz / STAGE_CHUNK);
        auto found = chunkAt.find(key);
        if (found == chunkAt.end()) {
            found = chunkAt.emplace(key, (int)s_stageChunks.size()).first;
            s_stageChunks.push_back(StageChunk());
        }
        s_sectorChunk[s] = found->second;
        s_stageChunks[found->second].sectors.push_back(s);
    }
    for (StageChunk &chunk : s_stageChunks) {
        float sumX = 0, sumZ = 0;
        for (int s : chunk.sectors) {
            sumX += s_sectorBounds[(size_t)s * 3];
            sumZ += s_sectorBounds[(size_t)s * 3 + 1];
        }
        chunk.x = sumX / chunk.sectors.size();
        chunk.z = sumZ / chunk.sectors.size();
        chunk.radius = 0;
        for (int s : chunk.sectors) {
            float dx = s_sectorBounds[(size_t)s * 3] - chunk.x, dz = s_sectorBounds[(size_t)s * 3 + 1] - chunk.z;
            chunk.radius = fmaxf(chunk.radius, sqrtf(dx * dx + dz * dz) + s_sectorBounds[(size_t)s * 3 + 2]);
        }
        MergeChunk(chunk);
    }
}

// Each sector's centre and reach in x/z, from its ground and objects.
void MeasureSectors(void)
{
    s_sectorBounds.assign((size_t)g_sectorCount * 3, 0.0f);
    for (int s = 0; s < g_sectorCount; s++) {
        if (g_sectors[s] == NULL)
            continue;
        float cx = g_sectors[s]->x / 65536.0f, cz = g_sectors[s]->z / 65536.0f, reach = 0.0f;
        Mesh *pMesh = g_sectors[s]->pMesh;
        if (pMesh != NULL && pMesh->pVertexData != NULL) {
            const GfxVertex *v = (const GfxVertex *)pMesh->pVertexData;
            for (int i = 0; i < pMesh->field_0x10; i++)
                reach = fmaxf(reach, sqrtf((v[i].x - cx) * (v[i].x - cx) + (v[i].z - cz) * (v[i].z - cz)));
        }
        for (const StageObject *pObject = g_sectors[s]->pObjects; pObject != NULL; pObject = pObject->pNext) {
            float dx = pObject->position.x / 65536.0f - cx, dz = pObject->position.z / 65536.0f - cz;
            reach = fmaxf(reach, sqrtf(dx * dx + dz * dz) + 50.0f);
        }
        s_sectorBounds[(size_t)s * 3] = cx;
        s_sectorBounds[(size_t)s * 3 + 1] = cz;
        s_sectorBounds[(size_t)s * 3 + 2] = reach;
    }
}

// Lists the static and the camera-facing objects of every sector.
void CollectObjects(void)
{
    s_staticObjects.clear();
    s_facingObjects.clear();
    s_sectorStatic.assign((size_t)g_sectorCount + 1, 0);
    s_sectorFacing.assign((size_t)g_sectorCount + 1, 0);
    for (int s = 0; s < g_sectorCount; s++) {
        s_sectorStatic[s] = (int)s_staticObjects.size();
        s_sectorFacing[s] = (int)s_facingObjects.size();
        if (g_sectors[s] == NULL)
            continue;
        for (const StageObject *pObject = g_sectors[s]->pObjects; pObject != NULL; pObject = pObject->pNext)
            if (pObject->pMesh != NULL && pObject->visible != 0)
                ((pObject->pMesh->flags & 2) != 0 ? s_facingObjects : s_staticObjects).push_back(pObject);
    }
    s_sectorStatic[g_sectorCount] = (int)s_staticObjects.size();
    s_sectorFacing[g_sectorCount] = (int)s_facingObjects.size();
}

void Build(void)
{
    Release();
    MeasureSectors();
    CollectObjects();
    MergeStage();

    // The grid covers the stage and MARGIN of land around it.
    float minX = 1e30f, minZ = 1e30f, maxX = -1e30f, maxZ = -1e30f;
    ForEachStageTriangle([&](Mesh *, MeshTriangle *, const GfxVertex &a, const GfxVertex &b, const GfxVertex &c) {
        for (const GfxVertex *p : { &a, &b, &c }) {
            minX = fminf(minX, p->x);
            maxX = fmaxf(maxX, p->x);
            minZ = fminf(minZ, p->z);
            maxZ = fmaxf(maxZ, p->z);
        }
    });
    if (minX > maxX)
        return;
    const float x0 = minX - MARGIN, z0 = minZ - MARGIN;
    const int nx = (int)ceilf((maxX - minX + 2 * MARGIN) / CELL) + 1;
    const int nz = (int)ceilf((maxZ - minZ + 2 * MARGIN) / CELL) + 1;
    const int cellsX = nx - 1, cellsZ = nz - 1;
    const size_t points = (size_t)nx * nz, cells = (size_t)cellsX * cellsZ;

    // Each point on the stage's ground takes the highest ground there, with
    // that triangle's texture, colour and light level; its distance from the
    // ground is 0.
    std::vector<float> height(points, -1e30f);
    std::vector<int> texture(points, -1);
    std::vector<DWORD> colour(points, 0xff808080);
    std::vector<int> level(points, -1);
    std::vector<int> distance(points, -1);
    std::unordered_map<int, std::pair<double, int> > texelScale;   // texture: uv per metre, samples
    ForEachStageTriangle([&](Mesh *pMesh, MeshTriangle *pTri, const GfxVertex &a, const GfxVertex &b, const GfxVertex &c) {
        if (NormalY(a, b, c) < GROUND_SLOPE)
            return;
        int tex = TriangleTexture(pTri);
        const int *pLevels = pMesh->pLightLevels;
        int lightLevel = pLevels == NULL ? -1 : (pLevels[pTri->vertexIndex[0]] + pLevels[pTri->vertexIndex[1]] +
                                                 pLevels[pTri->vertexIndex[2]]) / 3;
        float worldLength = sqrtf((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y) + (b.z - a.z) * (b.z - a.z));
        float uvLength = sqrtf((b.u0 - a.u0) * (b.u0 - a.u0) + (b.v0 - a.v0) * (b.v0 - a.v0));
        if (worldLength > 0.5f && uvLength > 0.0f) {
            auto &scale = texelScale[tex];
            scale.first += uvLength / worldLength;
            scale.second++;
        }
        DWORD average = 0xff000000;
        for (int k = 0; k < 3; k++) {
            int sum = ((const BYTE *)&a.diffuse)[k] + ((const BYTE *)&b.diffuse)[k] + ((const BYTE *)&c.diffuse)[k];
            average |= (DWORD)(sum / 3) << (8 * k);
        }
        float e1x = b.x - a.x, e1z = b.z - a.z, e2x = c.x - a.x, e2z = c.z - a.z;
        float det = e1x * e2z - e2x * e1z;
        if (det == 0.0f)
            return;
        int gx0 = (int)ceilf((fminf(a.x, fminf(b.x, c.x)) - x0) / CELL);
        int gx1 = (int)floorf((fmaxf(a.x, fmaxf(b.x, c.x)) - x0) / CELL);
        int gz0 = (int)ceilf((fminf(a.z, fminf(b.z, c.z)) - z0) / CELL);
        int gz1 = (int)floorf((fmaxf(a.z, fmaxf(b.z, c.z)) - z0) / CELL);
        for (int gz = gz0; gz <= gz1; gz++)
            for (int gx = gx0; gx <= gx1; gx++) {
                float px = x0 + gx * CELL - a.x, pz = z0 + gz * CELL - a.z;
                float u = (px * e2z - e2x * pz) / det, w = (e1x * pz - px * e1z) / det;
                if (u < -0.001f || w < -0.001f || u + w > 1.001f)
                    continue;
                size_t i = (size_t)gz * nx + gx;
                float y = a.y + u * (b.y - a.y) + w * (c.y - a.y);
                if (y > height[i]) {
                    height[i] = y;
                    texture[i] = tex;
                    colour[i] = average;
                    level[i] = lightLevel;
                    distance[i] = 0;
                }
            }
    });

    // The cells any stage triangle reaches (walls and banks too), with the
    // lowest stage height in each, and how far every cell is from them:
    // generated trees keep off the stage.
    std::vector<int> footprint(cells, -1);
    std::vector<float> stageLow(cells, 1e30f);
    ForEachStageTriangle([&](Mesh *, MeshTriangle *, const GfxVertex &a, const GfxVertex &b, const GfxVertex &c) {
        float span = fmaxf(fmaxf(fabsf(b.x - a.x) + fabsf(b.z - a.z), fabsf(c.x - a.x) + fabsf(c.z - a.z)),
                           fabsf(c.x - b.x) + fabsf(c.z - b.z));
        int steps = (int)ceilf(span / (CELL * 0.25f)) + 1;
        float low = fminf(a.y, fminf(b.y, c.y));
        for (int i = 0; i <= steps; i++)
            for (int j = 0; i + j <= steps; j++) {
                float u = (float)i / steps, w = (float)j / steps;
                int cx = (int)floorf((a.x + (b.x - a.x) * u + (c.x - a.x) * w - x0) / CELL);
                int cz = (int)floorf((a.z + (b.z - a.z) * u + (c.z - a.z) * w - z0) / CELL);
                if (cx >= 0 && cz >= 0 && cx < cellsX && cz < cellsZ) {
                    footprint[(size_t)cz * cellsX + cx] = 0;
                    stageLow[(size_t)cz * cellsX + cx] = fminf(stageLow[(size_t)cz * cellsX + cx], low);
                }
            }
    });
    Spread(footprint, cellsX, cellsZ, [](size_t, size_t) {});

    // The land textures: those of the ground on the stage's open outer
    // edges (grass, banks; seldom the road), the flat ones and the steepest.
    std::unordered_map<EdgeKey, std::pair<int, MeshTriangle *>, EdgeHash> edges;
    std::unordered_map<MeshTriangle *, float> slopes;
    ForEachStageTriangle([&](Mesh *, MeshTriangle *pTri, const GfxVertex &a, const GfxVertex &b, const GfxVertex &c) {
        float slope = NormalY(a, b, c);
        if (slope < GROUND_SLOPE)
            return;
        slopes[pTri] = slope;
        const GfxVertex *p[3] = { &a, &b, &c };
        for (int k = 0; k < 3; k++) {
            const GfxVertex *from = p[k], *to = p[(k + 1) % 3];
            int qa[3] = { (int)floorf(from->x * 20 + 0.5f), (int)floorf(from->y * 20 + 0.5f), (int)floorf(from->z * 20 + 0.5f) };
            int qb[3] = { (int)floorf(to->x * 20 + 0.5f), (int)floorf(to->y * 20 + 0.5f), (int)floorf(to->z * 20 + 0.5f) };
            bool order = memcmp(qa, qb, sizeof(qa)) < 0;
            EdgeKey key;
            memcpy(key.a, order ? qa : qb, sizeof(qa));
            memcpy(key.b, order ? qb : qa, sizeof(qb));
            auto &use = edges[key];
            use.first++;
            use.second = pTri;
        }
    });
    std::unordered_map<int, int> flatUses, steepUses;
    for (auto &e : edges)
        if (e.second.first == 1)
            (slopes[e.second.second] > 0.85f ? flatUses : steepUses)[TriangleTexture(e.second.second)]++;
    std::vector<int> land;      // the most used flat textures, at most 3
    for (int pick = 0; pick < 3; pick++) {
        int best = -1, bestUses = 0;
        for (auto &u : flatUses) {
            bool taken = false;
            for (int l : land)
                taken |= l == u.first;
            if (!taken && u.second > bestUses) {
                best = u.first;
                bestUses = u.second;
            }
        }
        if (best < 0 || (!land.empty() && bestUses * 10 < flatUses[land[0]]))
            break;
        land.push_back(best);
    }
    int steepTexture = land.empty() ? -1 : land[0], steepBest = 0;
    for (auto &u : steepUses)
        if (u.second > steepBest) {
            steepTexture = u.first;
            steepBest = u.second;
        }
    if (land.empty() || steepTexture < 0)
        return;

    // Off the ground: start from the nearest ground point, smooth toward the
    // neighbours (the ground stays), then raise hills growing away from it.
    Spread(distance, nx, nz, [&](size_t from, size_t to) {
        height[to] = height[from];
        texture[to] = texture[from];
        colour[to] = colour[from];
        level[to] = level[from];
    });
    std::vector<float> smoothed(height);
    for (int pass = 0; pass < 150; pass++) {
        for (int gz = 1; gz < nz - 1; gz++)
            for (int gx = 1; gx < nx - 1; gx++) {
                size_t i = (size_t)gz * nx + gx;
                if (distance[i] != 0)
                    smoothed[i] = 0.25f * (height[i - 1] + height[i + 1] + height[i - nx] + height[i + nx]);
            }
        height.swap(smoothed);
        smoothed = height;
    }
    for (size_t i = 0; i < points; i++) {
        if (distance[i] != 0)
            height[i] += Hills(x0 + (i % nx) * CELL, z0 + (i / nx) * CELL) * HILLS *
                         fminf(distance[i] * CELL / 150.0f, 1.0f);
        height[i] -= SINK;
    }
    // The land rises to a ridge along its outer edge, so that where it ends
    // it meets the backdrop as a hill top, not as an edge in the air.
    for (int gz = 0; gz < nz; gz++)
        for (int gx = 0; gx < nx; gx++) {
            int edge = gx < nx - 1 - gx ? gx : nx - 1 - gx;
            edge = edge < gz ? edge : gz;
            edge = edge < nz - 1 - gz ? edge : nz - 1 - gz;
            float rise = 1.0f - fminf(edge * CELL / RIM, 1.0f);
            height[(size_t)gz * nx + gx] += RIM_HEIGHT * rise * rise * (3.0f - 2.0f * rise);
        }

    // Where the stage has anything, the grid stays under all of it: a road
    // between two banks may fall between the grid's points, and the grid
    // must not span from bank to bank over it (it would show in the sky over
    // the car).
    for (int gz = 0; gz < nz; gz++)
        for (int gx = 0; gx < nx; gx++) {
            float cap = 1e30f;
            for (int cz = gz - 1; cz <= gz; cz++)
                for (int cx = gx - 1; cx <= gx; cx++)
                    if (cx >= 0 && cz >= 0 && cx < cellsX && cz < cellsZ)
                        cap = fminf(cap, stageLow[(size_t)cz * cellsX + cx]);
            float &h = height[(size_t)gz * nx + gx];
            h = fminf(h, cap - 1.0f);
        }

    // Slopes, and the generated land lit by them (a sun from above at an
    // angle) in broad patches; the stage's own colours carry its light.
    static const float sun[3] = { 0.45f, 0.75f, 0.48f };
    std::vector<float> normalY(points), shade(points, 1.0f);
    for (int gz = 0; gz < nz; gz++)
        for (int gx = 0; gx < nx; gx++) {
            size_t i = (size_t)gz * nx + gx;
            float dx = height[gx < nx - 1 ? i + 1 : i] - height[gx > 0 ? i - 1 : i];
            float dz = height[gz < nz - 1 ? i + nx : i] - height[gz > 0 ? i - nx : i];
            float n[3] = { -dx / (2 * CELL), 1.0f, -dz / (2 * CELL) };
            float length = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
            normalY[i] = 1.0f / length;
            if (distance[i] == 0)
                continue;
            float light = fmaxf((n[0] * sun[0] + n[1] * sun[1] + n[2] * sun[2]) / length, 0.0f);
            float x = x0 + gx * CELL, z = z0 + gz * CELL;
            shade[i] = (0.45f + 0.75f * light) * (0.8f + 0.4f * ValueNoise(x / 120.0f, z / 120.0f, 7));
        }

    // Every cell, in chunks of CHUNK x CHUNK: the steep texture on slopes,
    // land textures elsewhere (the ground's own when it is one, else broad
    // patches of them), and forest on the generated land.
    std::vector<TreeTemplate> trees;
    CollectTrees(trees);
    s_chunksX = (cellsX + CHUNK - 1) / CHUNK;
    s_chunksZ = (cellsZ + CHUNK - 1) / CHUNK;
    s_chunks.assign((size_t)s_chunksX * s_chunksZ, Chunk());
    for (int cz = 0; cz < s_chunksZ; cz++)
        for (int cx = 0; cx < s_chunksX; cx++) {
            Chunk &chunk = s_chunks[(size_t)cz * s_chunksX + cx];
            chunk.x = x0 + (cx + 0.5f) * CHUNK * CELL;
            chunk.z = z0 + (cz + 0.5f) * CHUNK * CELL;
            chunk.radius = CHUNK * CELL * 0.75f;
        }
    for (int gz = 0; gz < cellsZ; gz++)
        for (int gx = 0; gx < cellsX; gx++) {
            Chunk &chunk = s_chunks[(size_t)(gz / CHUNK) * s_chunksX + gx / CHUNK];
            const size_t corner[4] = { (size_t)gz * nx + gx, (size_t)gz * nx + gx + 1,
                                       (size_t)(gz + 1) * nx + gx + 1, (size_t)(gz + 1) * nx + gx };
            size_t farthest = corner[0];
            float slope = 0.0f;
            for (size_t k : corner) {
                if (distance[k] > distance[farthest])
                    farthest = k;
                slope += normalY[k] * 0.25f;
            }
            float centreX = x0 + (gx + 0.5f) * CELL, centreZ = z0 + (gz + 0.5f) * CELL;
            int tex = texture[farthest];
            bool isLand = false;
            for (int l : land)
                isLand |= l == tex;
            if (slope < STEEP_SLOPE)
                tex = steepTexture;
            else if (!isLand)
                tex = land[(int)(ValueNoise(centreX / 250.0f, centreZ / 250.0f, 11) * 0.999f * land.size())];
            float uvPerMetre = 1.0f / 8.0f;
            auto scale = texelScale.find(tex);
            if (scale != texelScale.end() && scale->second.second > 0)
                uvPerMetre = (float)(scale->second.first / scale->second.second);
            GfxVertex v[4];
            for (int k = 0; k < 4; k++) {
                size_t i = corner[k];
                memset(&v[k], 0, sizeof(v[k]));
                v[k].x = x0 + (i % nx) * CELL;
                v[k].y = height[i];
                v[k].z = z0 + (i / nx) * CELL;
                v[k].ny = 1.0f;
                v[k].diffuse = Scale(colour[i], shade[i]);
                v[k].specular = s_lit && level[i] >= 0 ? RampSpecular(level[i], shade[i]) : KEEP_COLOUR;
                v[k].u0 = v[k].u1 = v[k].x * uvPerMetre;
                v[k].v0 = v[k].v1 = v[k].z * uvPerMetre;
            }
            Batch &batch = BatchFor(chunk, tex, false);
            const GfxVertex quad[6] = { v[0], v[1], v[2], v[0], v[2], v[3] };
            batch.vertices.insert(batch.vertices.end(), quad, quad + 6);

            // Forest in broad patches, sparse trees between them.
            if (trees.empty() || footprint[(size_t)gz * cellsX + gx] < TREE_CLEARANCE || slope < TREE_SLOPE)
                continue;
            float want = ValueNoise(centreX / 300.0f, centreZ / 300.0f, 23) > 0.45f ? 4.0f : 0.2f;
            int count = (int)want + (Hash(gx, gz, 31) < want - (int)want ? 1 : 0);
            for (int n = 0; n < count; n++) {
                float fx = Hash(gx, gz, 40 + n * 3), fz = Hash(gx, gz, 41 + n * 3);
                const TreeTemplate &t = trees[(size_t)(Hash(gx, gz, 42 + n * 3) * 0.999f * trees.size())];
                float front = v[0].y + (v[1].y - v[0].y) * fx, back = v[3].y + (v[2].y - v[3].y) * fx;
                float size = 0.8f + 0.4f * Hash(gx, gz, 60 + n);
                AddFacing(t.pMesh, t.pLevels, t.level, x0 + (gx + fx) * CELL, front + (back - front) * fz + t.base * size - 0.5f,
                          z0 + (gz + fz) * CELL, t.sx * size, t.sy * size, t.sz * size,
                          [&](int texture) -> std::vector<GfxVertex> & { return BatchFor(chunk, texture, true).vertices; });
            }
        }

    // Each batch to the GPU once.
    size_t triangles = 0;
    for (Chunk &chunk : s_chunks)
        for (Batch &b : chunk.batches) {
            b.count = (DWORD)b.vertices.size();
            triangles += b.count / 3;
            b.pBuffer = Gfx_CreateVertexBuffer(b.count);
            Gfx_KeepVertexBuffer(b.pBuffer);
            memcpy(Gfx_LockVertexBuffer(b.pBuffer), b.vertices.data(), b.count * sizeof(GfxVertex));
            Gfx_UnlockVertexBuffer(b.pBuffer);
            std::vector<GfxVertex>().swap(b.vertices);
        }
    Sys_Log("far layer: %dx%d cells of %.0f m, %zu triangles of land", cellsX, cellsZ, CELL, triangles);
}

} // namespace

namespace {

// The horizontal view: the camera's position and heading in x/z and the half
// angle the projection shows, with a margin.
struct View {
    float x, z;
    float forwardX, forwardZ;
    float halfAngle;

    // Whether a circle of the given radius at (cx, cz), between near and far
    // from the camera, can be in view.
    bool Sees(float cx, float cz, float radius, float near, float far) const
    {
        float dx = cx - x, dz = cz - z;
        float d = sqrtf(dx * dx + dz * dz);
        if (d + radius < near || d - radius > far)
            return false;
        if (d <= radius)
            return true;
        float cosine = (dx * forwardX + dz * forwardZ) / d;
        float angle = acosf(fmaxf(-1.0f, fminf(1.0f, cosine)));
        return angle - asinf(radius / d) < halfAngle;
    }
};

// The light ramp the scene lights the stage with now (Scene_BuildLightTables),
// for Gfx_SetLightRamp.
void SetLightRamp(void)
{
    float ambient[3] = { g_sceneAmbient.x / 65536.0f, g_sceneAmbient.y / 65536.0f, g_sceneAmbient.z / 65536.0f };
    float light[3] = { g_sceneLightColour.x / 65536.0f, g_sceneLightColour.y / 65536.0f,
                       g_sceneLightColour.z / 65536.0f };
    if (s_lit)
        Gfx_SetLightRamp(ambient, light);
}

// The stage's chunks in view between the clip distance and FAR_DISTANCE, as
// the stage draws them (Game_DrawSceneViewport): ground in lighting mode 0,
// static objects in mode 1, then the camera-facing objects (trees, bushes) as
// billboards. textures is the texture manager's table. The meshes drawn one
// by one keep the colours the scene gives them (Scene_RelightSector), which it
// does only for the sectors in view.
void DrawStage(const View &view, float clip, Texture *const *textures)
{
    static std::vector<const StageChunk *> chunks;
    chunks.clear();
    for (const StageChunk &chunk : s_stageChunks) {
        if (!view.Sees(chunk.x, chunk.z, chunk.radius, clip, FAR_DISTANCE))
            continue;
        if (s_lit && !chunk.others.empty())
            for (int s : chunk.sectors)
                Scene_RelightSector(s);
        chunks.push_back(&chunk);
    }
    Gfx_SetTransform(GFX_TRANSFORM_WORLD, &g_identityWorldMatrix);
    Graphics_SwitchAlphaBlendAndTest(0);
    for (int group = 0; group < GROUPS; group++) {
        Graphics_SetLightingMode(group == GROUND ? 0 : 1);
        CGraphics::SetCullMode(group == OBJECTS_TWO_SIDED || group == FACING ? 1 : CGame::GetSectorDrawState());
        Gfx_SetBillboards(group == FACING);
        for (const StageChunk *pChunk : chunks)
            for (const StageBatch &b : pChunk->batches)
                if (b.group == group) {
                    if (group == FACING) {
                        Frontend_SetObjectField118(textures[b.texture], 0);
                        Graphics_SetLightingMode(b.lighting);
                    }
                    ApplyTexture(textures[b.texture]);
                    Gfx_DrawPrimitiveVB(GFX_TRIANGLELIST, b.pBuffer, 0, b.count);
                }
        Gfx_SetBillboards(0);
        Gfx_SetLightRamp(NULL, NULL);
        for (const StageChunk *pChunk : chunks)
            for (size_t i = 0; i < pChunk->others.size(); i++)
                if (pChunk->othersGroup[i] == group) {
                    Mesh *pMesh = pChunk->others[i];
                    if ((pMesh->flags & 0x1000) != 0)
                        Graphics_DrawMeshLOD(pMesh, 0, 1, 0);
                    else
                        Graphics_DrawMeshLOD(pMesh, 1, 0, 0);
                }
        SetLightRamp();
    }
}

} // namespace

void FarLayer_Draw(SceneNode *pCamera)
{
    static const int enabled = Sys_GetOption("video.far_layer", 1);
    if (s_testKeySky)
        Gfx_Clear(GFX_CLEAR_TARGET, 0xff00ff00, 1.0f);
    if (!enabled || !s_testDrawn || g_sectorCount <= 0 || g_sectors[0] == NULL)
        return;
    // The stage's sectors are loaded once per stage. When the scene lights
    // them, the layer follows its light (SetLightRamp); else it keeps the
    // colours they have.
    if (s_builtFor != g_sectors[0]->pMesh || s_builtSectors != g_sectorCount) {
        s_builtFor = g_sectors[0]->pMesh;
        s_builtSectors = g_sectorCount;
        s_lit = CGraphics::m_unk0x0072d56c != 0 && g_sceneSectorLights != NULL;
        Build();
    }

    int farPlane = CGraphics::m_farPlaneFixed, nearPlane = (int)(CGraphics::m_nearPlane * 65536.0f);
    float clip = farPlane / 65536.0f * CLIP_SHARE;
    View view;
    view.x = pCamera->world.position.x / 65536.0f;
    view.z = pCamera->world.position.z / 65536.0f;
    float fx = (float)pCamera->world.forward.x, fz = (float)pCamera->world.forward.z;
    float length = sqrtf(fx * fx + fz * fz);
    view.forwardX = length > 0.0f ? fx / length : 0.0f;
    view.forwardZ = length > 0.0f ? fz / length : 1.0f;
    // The projection's x scale is the cotangent of half the horizontal view;
    // 10 degrees spare for the camera's roll and the slopes.
    view.halfAngle = atanf(1.0f / CGraphics::m_projection11) + 0.17f;
    int zEnable = CGraphics::m_zEnable, zWrite = CGraphics::m_zWriteEnable, clamp = CGraphics::m_textureAddressClamp;
    Texture *const *textures = CGraphics::m_pTextureManager->textureBuffer;

    CGraphics::SetProjection(0, 0, (int)(FAR_DISTANCE * 65536.0f), (int)(NEAR_DISTANCE * 65536.0f));
    Gfx_SetTransform(GFX_TRANSFORM_WORLD, &g_identityWorldMatrix);
    Graphics_EnableFog();
    if (s_testMarks)
        Gfx_SetDistanceMarks(farPlane / 65536.0f * MARK_SHARE);
    CGraphics::SetZEnable(1);
    CGraphics::SetZWriteEnable(1);
    SetLightRamp();

    // Nearest first, so that what lies behind is left out early by depth: the
    // stage itself beyond the clip distance, then the generated trees, lit as
    // the stage's own (mode 1), then the height grid and the generated land
    // at any distance, by chunk nearest first.
    Gfx_SetDistanceClip(clip);
    DrawStage(view, clip, textures);
    Gfx_SetDistanceClip(0.0f);
    static std::vector<std::pair<float, const Chunk *> > inView;
    inView.clear();
    for (const Chunk &chunk : s_chunks)
        if (!chunk.batches.empty() && view.Sees(chunk.x, chunk.z, chunk.radius, 0.0f, FAR_DISTANCE)) {
            float dx = chunk.x - view.x, dz = chunk.z - view.z;
            inView.push_back(std::make_pair(dx * dx + dz * dz, &chunk));
        }
    qsort(inView.data(), inView.size(), sizeof(inView[0]), [](const void *a, const void *b) {
        float x = ((const std::pair<float, const Chunk *> *)a)->first;
        float y = ((const std::pair<float, const Chunk *> *)b)->first;
        return x < y ? -1 : x > y;
    });
    Graphics_SwitchAlphaBlendAndTest(0);
    CGraphics::SetCullMode(1);
    CGraphics::SetTextureAddressClamp(0);
    for (int trees = 1; trees >= 0; trees--) {
        Graphics_SetLightingMode(trees ? 1 : 2);
        Gfx_SetBillboards(trees);
        for (const auto &near : inView)
            for (const Batch &b : near.second->batches)
                if (b.trees == (trees != 0)) {
                    ApplyTexture(textures[b.texture]);
                    Gfx_DrawPrimitiveVB(GFX_TRIANGLELIST, b.pBuffer, 0, b.count);
                }
    }
    Gfx_SetBillboards(0);
    Gfx_SetLightRamp(NULL, NULL);
    Gfx_SetDistanceMarks(-1.0f);
    CGraphics::SetTextureAddressClamp(clamp);

    Gfx_Clear(GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f);
    CGraphics::SetZWriteEnable(zWrite);
    CGraphics::SetZEnable(zEnable);
    CGraphics::SetCullMode(CGame::GetSectorDrawState());
    CGraphics::SetProjection(0, 0, farPlane, nearPlane);
}

// The texture of stage 0 (CGraphics::ApplyTextureStageChange, for the
// layer's helpers).
void FarLayer_ApplyTexture(Texture *pTexture)
{
    CGraphics::ApplyTextureStageChange(0, pTexture);
}

int FarLayer_StageFades(void)
{
    return s_testFade;
}

void FarLayer_SetTest(int drawn, int fade, int marks, int keySky)
{
    s_testDrawn = drawn;
    s_testFade = fade;
    s_testMarks = marks;
    s_testKeySky = keySky;
}
