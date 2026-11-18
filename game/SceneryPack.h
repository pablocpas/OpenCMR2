#ifndef OPENCMR2_SCENERY_PACK_H
#define OPENCMR2_SCENERY_PACK_H
struct StageObject;
struct GenericFile;
struct Texture;
struct Mesh;struct MeshPart;
bool SceneryPack_DrawFixedPart(Mesh*,MeshPart*);
void SceneryPack_RegisterSource(Texture *texture, const void *bytes, unsigned size);
// Visual-only overrides. Original meshes/collision records remain untouched.
void SceneryPack_RegisterArchive(GenericFile *archive, const char *path);
bool SceneryPack_Draw(StageObject *object, const float *billboardMatrix);
// OpenCMR2: between Begin and End, wind-animated tree draws are queued and their vertex math runs on worker
// threads; recording still happens on the calling thread in draw order. Call FlushBatch before any other
// draw that records, so order is kept. video.tree_threads=1 turns the workers off.
// eye: camera position in world units; pass null to disable distance effects.
// Batch callers must supply it so the tree wind distance falloff does not
// depend on game globals (tests link SceneryPack.cpp standalone).
void SceneryPack_BeginBatch(const float *eye, const void *eyeKey);
void SceneryPack_BeginBatch(void);
void SceneryPack_FlushBatch(void);
void SceneryPack_EndBatch(void);
void SceneryPack_Clear();
void SceneryPack_Advance(int step16_16);
// Car state (any rally car, slots 0..7) for reactive spectators (16.16 world position and speed).
void SceneryPack_SetReactionSource(int slot,int x16_16,int z16_16,int speed16_16,bool valid);
void SceneryPack_LogStatus();
// Last actual spectator frame draw, not a promise that a mapping is active.
bool SceneryPack_GetAnimationStatus(unsigned long long *tick,unsigned *frame,unsigned *changes);
// Running totals since launch; callers subtract the previous reading to get a per-frame count. Counts, not timing.
void SceneryPack_GetFrameTotals(unsigned long long *crowdDraws,unsigned long long *crowdTris,unsigned long long *treeTris,unsigned long long *treeUvVertices);
// Diagnostics: running totals (ns, calls) of wall-clock time inside SceneryPack_Draw and inside its whole-tree replacement part.
void SceneryPack_GetTimeSplit(unsigned long long *windMathNs,unsigned long long *submitNs);
void SceneryPack_GetTimeTotals(unsigned long long *replacementNs,unsigned long long *totalNs,unsigned long long *calls);
void SceneryPack_GetTreeStatus(unsigned *replacements,unsigned *wind,unsigned *originalSources,unsigned *farCards);
#endif
