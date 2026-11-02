// Unit test for BrushRelief (Greece1 brush-wall draw-time depth): exact
// perimeter, interior displacement by alpha, back-layer geometry, determinism.
#include "../src/port/compat/windows.h"
#include "../game/BrushRelief.h"
#include <cstdio>
#include <cmath>
static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { printf("FAIL %d: %s\n", __LINE__, #cond); failures++; } } while (0)
int main() {
    // Wall quad: 17 wide, 3 tall, in the z=0 plane; corners in UV-corner order.
    MeshVertexF c[4] = {};
    c[0].x=-8.5f; c[0].y=4.0f; c[0].textureCoordinates[0][0]=0; c[0].textureCoordinates[0][1]=0;
    c[1].x= 8.5f; c[1].y=4.0f; c[1].textureCoordinates[0][0]=1; c[1].textureCoordinates[0][1]=0;
    c[2].x=-8.5f; c[2].y=1.0f; c[2].textureCoordinates[0][0]=0; c[2].textureCoordinates[0][1]=1;
    c[3].x= 8.5f; c[3].y=1.0f; c[3].textureCoordinates[0][0]=1; c[3].textureCoordinates[0][1]=1;
    float n[3] = {0.f, 0.f, 1.f};
    // Alpha grid: solid middle, empty edges.
    const int AW = 64, AH = 32;
    unsigned char alpha[AW*AH];
    for (int y = 0; y < AH; y++) for (int x = 0; x < AW; x++)
        alpha[y*AW+x] = (x > 8 && x < 56 && y > 4 && y < 28) ? 255 : 0;

    std::vector<BrushRelief::Vertex> v;
    CHECK(BrushRelief::Build(c, n, alpha, AW, AH, 100, true, v));
    CHECK(v.size() == 2u * (BrushRelief::kGridX+1) * (BrushRelief::kGridY+1));

    // Front layer: edge vertices keep the exact source perimeter (z == 0).
    for (int iy = 0; iy <= BrushRelief::kGridY; iy++)
        for (int ix = 0; ix <= BrushRelief::kGridX; ix++) {
            if (ix && ix != BrushRelief::kGridX && iy && iy != BrushRelief::kGridY) continue;
            const auto &q = v[iy*(BrushRelief::kGridX+1)+ix];
            CHECK(std::fabs(q.z) < 1e-5f);
        }
    // Interior vertex over solid alpha displaces along +z.
    const auto &mid = v[(BrushRelief::kGridY/2)*(BrushRelief::kGridX+1) + BrushRelief::kGridX/2];
    CHECK(mid.z > 0.8f && mid.z <= 0.9f + 1e-5f);
    CHECK(mid.shade > 1.0f);   // relief brightens displaced verts

    // Back layer: offset inward, taller than the front, darker.
    size_t back0 = (BrushRelief::kGridX+1) * (BrushRelief::kGridY+1);
    const auto &bt = v[back0];                 // back top-left
    CHECK(bt.z > 0.4f && bt.z < 0.5f);         // inward offset only, no relief
    CHECK(bt.y > 4.0f + 0.5f);                 // taller than the source top edge
    CHECK(bt.shade < 0.75f);
    CHECK(bt.u > 0.3f && bt.u < 0.32f);        // shifted UV window

    // Indices cover both layers, all in range.
    std::vector<unsigned short> ix;
    BrushRelief::Indices(true, ix);
    CHECK(ix.size() == 2u * BrushRelief::kGridX * BrushRelief::kGridY * 6);
    for (auto i : ix) CHECK(i < v.size());

    // Relief dial 0 -> no displacement, back layer still exact offset.
    std::vector<BrushRelief::Vertex> v0;
    CHECK(BrushRelief::Build(c, n, alpha, AW, AH, 0, true, v0));
    const auto &m0 = v0[(BrushRelief::kGridY/2)*(BrushRelief::kGridX+1) + BrushRelief::kGridX/2];
    CHECK(std::fabs(m0.z) < 1e-5f);

    // Determinism: identical inputs, identical output.
    std::vector<BrushRelief::Vertex> v2;
    CHECK(BrushRelief::Build(c, n, alpha, AW, AH, 100, true, v2));
    CHECK(v2.size() == v.size() && v2[100].x == v[100].x && v2[100].z == v[100].z);

    // No back layer mode.
    std::vector<BrushRelief::Vertex> v3;
    CHECK(BrushRelief::Build(c, n, alpha, AW, AH, 100, false, v3));
    CHECK(v3.size() == (BrushRelief::kGridX+1u) * (BrushRelief::kGridY+1u));

    // Road-facing side: wall in z=0 plane, road on +z -> away = -z; and the reverse.
    {
        float qa[3]={-8.5f,4,0},qb[3]={8.5f,4,0},nn[3]={0,0,1},away[3];
        float roadPos[]={-20,6, 20,6};
        CHECK(BrushRelief::AwayFromRoad(qa,qb,nn,roadPos,2,60,away) && away[2] < -0.99f);
        float roadNeg[]={-20,-6, 20,-6};
        CHECK(BrushRelief::AwayFromRoad(qa,qb,nn,roadNeg,2,60,away) && away[2] > 0.99f);
        float flipped[3]={0,0,-1};   // quad winding must not matter
        CHECK(BrushRelief::AwayFromRoad(qa,qb,flipped,roadPos,2,60,away) && away[2] < -0.99f);
        float farRoad[]={-20,500, 20,500};
        CHECK(!BrushRelief::AwayFromRoad(qa,qb,nn,farRoad,2,60,away));
        float awayN[3]={0,0,-1};
        std::vector<BrushRelief::Vertex> vv;
        CHECK(BrushRelief::Build(c,awayN,alpha,AW,AH,100,true,vv,BrushRelief::kVolumeSlices));
        CHECK(vv.size()==(size_t)(2+BrushRelief::kVolumeSlices)*(BrushRelief::kGridX+1)*(BrushRelief::kGridY+1));
        float minz=1e9f;
        for(auto&q:vv){CHECK(q.z<=1e-5f);if(q.z<minz)minz=q.z;}
        CHECK(minz < -(BrushRelief::kVolumeSlices*BrushRelief::kVolumeStep)+0.01f);
        for(int iy=0;iy<=BrushRelief::kGridY;iy++)for(int ix=0;ix<=BrushRelief::kGridX;ix++)
            if(!ix||ix==BrushRelief::kGridX||!iy||iy==BrushRelief::kGridY)CHECK(std::fabs(vv[iy*(BrushRelief::kGridX+1)+ix].z)<1e-5f);
        std::vector<unsigned short> vi;BrushRelief::Indices(true,vi,BrushRelief::kVolumeSlices);
        for(auto i:vi)CHECK(i<vv.size());
    }

    if (!failures) printf("brush_relief ok\n");
    return failures ? 1 : 0;
}
