#ifndef CMR2_SHADOW_PART_H
#define CMR2_SHADOW_PART_H

#include <windows.h>

struct Mesh;
struct MeshVertexF;
struct SceneNode;

// Runtime shadow work buffers; never an image of a file record.
// Original Win32 offsets below; native pointers determine the x64 layout.
struct ShadowPart {
    float light[12];            // 0x0 light direction and basis in mesh space
    Mesh *pMesh;                // 0x30 mesh (or its shadow cylinder)
    SceneNode *pNode;           // 0x34 node that owns the mesh
    float (*pSourceNormals)[3];    // 0x38 source normal per vertex
    float (*pBiasedPositions)[3];   // 0x3c source position + normal * 0.005
    float *pLightDotProducts;        // 0x40 normal dot light direction per vertex
    float (*pEmittedPositions)[3];   // 0x44 biased position per emitted vertex
    MeshVertexF *pVertices;     // 0x48 emitted render vertices
    MeshVertexF **pFaceVertices; // 0x4c three native vertex pointers per face
    short faceCount;            // 0x50 emitted faces
    short vertexCount;          // 0x52 emitted vertices
    int verticesDirty;          // 0x54 regenerate vertex projection buffers
};

struct ShadowCaster {
    SceneNode *pNode;           // 0x0
    ShadowPart *pParts;         // 0x4
    BYTE partCount;             // 0x8
    int projectionDirty;       // 0xc regenerate dirty vertex projections
    int meshDirty;             // 0x10 rebuild emitted shadow geometry
};

#endif
