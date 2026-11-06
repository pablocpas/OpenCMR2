#include "CockpitMesh.h"
#include <cstdio>
#define CHECK(c) do{ if(!(c)){ std::printf("FAIL %d: %s\n", __LINE__, #c); ++fails; } }while(0)
static bool near_(float a, float b) { return std::fabs(a - b) < 1e-4f; }
int main()
{
    int fails = 0;
    // Writer -> parser round trip (V flip is symmetric).
    CmObjWriter w;
    w.Group(3, 1, 12);
    CmVert a = { 0.1f, 0.2f, 0.3f, 0.25f, 0.75f }, b = { 1, 2, 3, 0.5f, 0.5f }, c = { -1, -2, -3, 1.0f, 0.0f };
    w.Tri(a, b, c);
    std::vector<CmTri> tris;
    CmParseResult r;
    CHECK(CockpitMesh_ParseObj(w.text.data(), w.text.size(), tris, r));
    CHECK(tris.size() == 1 && tris[0].tex == 12);
    CHECK(near_(tris[0].v[0].x, 0.1f) && near_(tris[0].v[2].z, -3.0f));
    CHECK(near_(tris[0].v[0].u, 0.25f) && near_(tris[0].v[0].v, 0.75f) && near_(tris[0].v[2].v, 0.0f));

    // Synthetic file: quad (fan -> 2 tris), v//vn form, negative indices, CRLF, Blender ".001" material suffix,
    // untextured face skipped, comments and unknown lines ignored.
    const char *obj =
        "# test\r\nmtllib x.mtl\r\no piece\r\n"
        "v 0 0 0\r\nv 1 0 0\r\nv 1 1 0\r\nv 0 1 0\r\n"
        "vt 0 0\r\nvt 1 0\r\nvt 1 1\r\nvt 0 1\r\nvn 0 0 1\r\n"
        "usemtl tex5.001\r\nf 1/1/1 2/2/1 3/3/1 4/4/1\r\n"
        "usemtl Material\r\nf 1 2 3\r\n"
        "usemtl tex7\r\nf -4//1 -3//1 -2//1\r\n";
    CHECK(CockpitMesh_ParseObj(obj, strlen(obj), tris, r));
    CHECK(tris.size() == 3);
    CHECK(r.skippedNoTexture == 1);
    CHECK(tris[0].tex == 5 && tris[1].tex == 5 && tris[2].tex == 7);
    CHECK(near_(tris[1].v[2].x, 0.0f) && near_(tris[1].v[2].y, 1.0f));   // fan: (1,3,4)
    CHECK(near_(tris[2].v[1].x, 1.0f) && near_(tris[2].v[2].y, 1.0f));
    // Missing vt -> uv 0,0 and still parsed.
    const char *noUv = "v 0 0 0\nv 1 0 0\nv 0 1 0\nusemtl tex1\nf 1 2 3\n";
    CHECK(CockpitMesh_ParseObj(noUv, strlen(noUv), tris, r) && tris.size() == 1 && near_(tris[0].v[1].u, 0.0f));

    // Hard errors.
    const char *bad = "v 0 0 0\nusemtl tex1\nf 1 2 3\n";
    CHECK(!CockpitMesh_ParseObj(bad, strlen(bad), tris, r) && !r.error.empty());
    CHECK(CockpitMesh_ParseObj("", 0, tris, r) && tris.empty());

    // Matrix helpers: node->root and back is the identity on a point.
    float root[16] = { 0, 0, 1, 0,  0, 1, 0, 0,  -1, 0, 0, 0,  5, 1, -2, 1 };
    float node[16] = { 1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  5.5f, 1.2f, -2.1f, 1 };
    float inv[16], rel[16], back[16];
    CHECK(Cm_AffineInverse(root, inv));
    Cm_Mul(node, inv, rel);
    Cm_Mul(rel, root, back);
    for (int i = 0; i < 16; i++) CHECK(near_(back[i], node[i]));
    float p[3];
    Cm_Transform(rel, 0, 0, 0, p);   // node origin expressed in root space, then back to world
    float q[3];
    Cm_Transform(root, p[0], p[1], p[2], q);
    CHECK(near_(q[0], 5.5f) && near_(q[1], 1.2f) && near_(q[2], -2.1f));
    float sing[16] = {};
    CHECK(!Cm_AffineInverse(sing, inv));

    std::printf(fails ? "FAILED\n" : "cockpit mesh ok\n");
    return fails ? 1 : 0;
}
