#ifndef OPENCMR2_COCKPIT_MESH_H
#define OPENCMR2_COCKPIT_MESH_H
// Pure helpers for the cockpit add-on mesh (video.cockpit_mesh) and the
// interior scaffold dump (video.dump_interior). No engine types; unit tested in
// tests/cockpit_mesh.cpp.
//
// Coordinates are the interior ROOT node's space in game axes (x forward, y up,
// z lateral), written unchanged. V is flipped in the file (v_file = 1 - v_game)
// because OBJ puts the UV origin bottom-left and the game atlas top-left.
// Materials: "usemtl tex<N>" selects interior texture id N (the dump names every
// group that way). Faces with no tex<N> material are skipped.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>

struct CmVert { float x, y, z, u, v; };
struct CmTri { CmVert v[3]; int tex; };

// 4x4 row-major, row-vector convention (p' = p * M, translation in row 3),
// the same layout as a scene node's world matrix.
inline void Cm_Mul(const float *a, const float *b, float *o)
{
    float t[16];
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) {
            float s = 0;
            for (int k = 0; k < 4; k++) s += a[r * 4 + k] * b[k * 4 + c];
            t[r * 4 + c] = s;
        }
    memcpy(o, t, sizeof t);
}

// Inverse of an affine matrix (last column 0 0 0 1). False if singular.
inline bool Cm_AffineInverse(const float *m, float *o)
{
    double a = m[0], b = m[1], c = m[2], d = m[4], e = m[5], f = m[6], g = m[8], h = m[9], i = m[10];
    double det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
    if (std::fabs(det) < 1e-12) return false;
    double inv[9] = {
        (e * i - f * h) / det, (c * h - b * i) / det, (b * f - c * e) / det,
        (f * g - d * i) / det, (a * i - c * g) / det, (c * d - a * f) / det,
        (d * h - e * g) / det, (b * g - a * h) / det, (a * e - b * d) / det };
    for (int r = 0; r < 3; r++) {
        for (int k = 0; k < 3; k++) o[r * 4 + k] = (float)inv[r * 3 + k];
        o[r * 4 + 3] = 0;
    }
    double tx = m[12], ty = m[13], tz = m[14];
    for (int k = 0; k < 3; k++)
        o[12 + k] = (float)-(tx * inv[0 * 3 + k] + ty * inv[1 * 3 + k] + tz * inv[2 * 3 + k]);
    o[15] = 1;
    return true;
}

inline void Cm_Transform(const float *m, float x, float y, float z, float *o3)
{
    o3[0] = x * m[0] + y * m[4] + z * m[8] + m[12];
    o3[1] = x * m[1] + y * m[5] + z * m[9] + m[13];
    o3[2] = x * m[2] + y * m[6] + z * m[10] + m[14];
}

// OBJ writer for the dump: one group + material per (node, part).
struct CmObjWriter {
    std::string text;
    int vertexCount = 0;
    CmObjWriter()
    {
        text = "# OpenCMR2 interior dump (scaffold, read only)\n"
               "# Game axes written unchanged: x forward, y up, z lateral. Scale 1.\n"
               "# Group n<node>_p<part>, material tex<N> = interior texture id N.\n"
               "# Import AND export in Blender with the same axis settings: Forward X, Up Y.\n";
    }
    void Group(int node, int part, int tex)
    {
        char b[96];
        snprintf(b, sizeof b, "g n%d_p%d\nusemtl tex%d\n", node, part, tex);
        text += b;
    }
    void Tri(const CmVert &a, const CmVert &b, const CmVert &c)
    {
        char l[256];
        const CmVert *vs[3] = { &a, &b, &c };
        for (int k = 0; k < 3; k++) {
            snprintf(l, sizeof l, "v %.6f %.6f %.6f\nvt %.6f %.6f\n", vs[k]->x, vs[k]->y, vs[k]->z, vs[k]->u, 1.0f - vs[k]->v);
            text += l;
        }
        snprintf(l, sizeof l, "f %d/%d %d/%d %d/%d\n", vertexCount + 1, vertexCount + 1, vertexCount + 2, vertexCount + 2,
                 vertexCount + 3, vertexCount + 3);
        text += l;
        vertexCount += 3;
    }
};

struct CmParseResult { int triangles = 0, skippedNoTexture = 0, skippedBad = 0; std::string error; };

static const int kCmMaxTris = 100000;

// Parses OBJ text: v, vt, f (v, v/vt, v//vn, v/vt/vn, negative indices, n-gons
// as fans) and usemtl tex<N>. Everything else is ignored. Returns false only on
// a hard error (index out of range, too many triangles).
inline bool CockpitMesh_ParseObj(const char *text, size_t len, std::vector<CmTri> &out, CmParseResult &res)
{
    std::vector<float> pos, uv;
    int tex = -1;
    size_t i = 0;
    out.clear();
    res = CmParseResult();
    while (i < len) {
        size_t e = i;
        while (e < len && text[e] != '\n') e++;
        std::string line(text + i, e - i);
        i = e + 1;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const char *p = line.c_str();
        if (strncmp(p, "v ", 2) == 0) {
            float x, y, z;
            if (sscanf(p + 2, "%f %f %f", &x, &y, &z) == 3) { pos.push_back(x); pos.push_back(y); pos.push_back(z); }
        } else if (strncmp(p, "vt ", 3) == 0) {
            float u = 0, v = 0;
            if (sscanf(p + 3, "%f %f", &u, &v) >= 1) { uv.push_back(u); uv.push_back(1.0f - v); }
        } else if (strncmp(p, "usemtl", 6) == 0) {
            const char *q = p + 6;
            while (*q == ' ' || *q == '\t') q++;
            int n;
            tex = (strncmp(q, "tex", 3) == 0 && sscanf(q + 3, "%d", &n) == 1) ? n : -1;
        } else if (strncmp(p, "f ", 2) == 0) {
            std::vector<int> pi, ti;
            const char *q = p + 2;
            while (*q) {
                while (*q == ' ' || *q == '\t') q++;
                if (!*q) break;
                char *end;
                long a = strtol(q, &end, 10), t = 0;
                bool hasT = false;
                if (end == q) { res.error = "bad face token"; return false; }
                q = end;
                if (*q == '/') {
                    q++;
                    if (*q != '/') { t = strtol(q, &end, 10); hasT = end != q; q = end; }
                    if (*q == '/') { q++; strtol(q, &end, 10); q = end; }
                }
                int nv = (int)(pos.size() / 3), nt = (int)(uv.size() / 2);
                long ai = a < 0 ? nv + a : a - 1;
                long ti2 = !hasT ? -1 : (t < 0 ? nt + t : t - 1);
                if (ai < 0 || ai >= nv || (hasT && (ti2 < 0 || ti2 >= nt))) { res.error = "face index out of range"; return false; }
                pi.push_back((int)ai);
                ti.push_back((int)ti2);
                while (*q && *q != ' ' && *q != '\t') q++;
            }
            if (pi.size() < 3) { res.skippedBad++; continue; }
            if (tex < 0) { res.skippedNoTexture += (int)pi.size() - 2; continue; }
            for (size_t k = 1; k + 1 < pi.size(); k++) {
                if ((int)out.size() >= kCmMaxTris) { res.error = "too many triangles"; return false; }
                CmTri t;
                t.tex = tex;
                size_t idx[3] = { 0, k, k + 1 };
                for (int c = 0; c < 3; c++) {
                    int vi = pi[idx[c]], ui = ti[idx[c]];
                    t.v[c].x = pos[vi * 3]; t.v[c].y = pos[vi * 3 + 1]; t.v[c].z = pos[vi * 3 + 2];
                    t.v[c].u = ui >= 0 ? uv[ui * 2] : 0.0f;
                    t.v[c].v = ui >= 0 ? uv[ui * 2 + 1] : 0.0f;
                }
                out.push_back(t);
            }
        }
    }
    res.triangles = (int)out.size();
    return true;
}
#endif
