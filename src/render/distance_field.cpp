// Euclidean distance transform shared by fonts and vector sprites.
#include "distance_field.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace DistanceField {
// Squared Euclidean distance transform (Felzenszwalb and Huttenlocher).
// Bolt optimization: precompute coordinates squared qq[q] = q*q once, cache
// f_plus_qq[q] to avoid recomputing f[p] + p*p in the parabola intersection loop,
// precompute inv_denom_table[diff] = 0.5f / diff to eliminate floating-point division
// and int-to-float conversions in the intersection search, cache active hull state
// (vk, zk1) and maintain float q_f in the distance assignment pass to eliminate
// inner int-to-float conversions, and use single-precision float for
// intersection boundaries z.
static void DistanceTransform1D(const float *f, const float *qq,
                                float *f_plus_qq, const float *inv_denom_table,
                                float *d, int n, int *v, float *z) {
  int k = 0;
  v[0] = 0;
  z[0] = -1e30f;
  z[1] = 1e30f;
  f_plus_qq[0] = f[0];
  for (int q = 1; q < n; q++) {
    float fq_plus_qq = f[q] + qq[q];
    f_plus_qq[q] = fq_plus_qq;
    float s;
    for (;;) {
      int p = v[k];
      s = (fq_plus_qq - f_plus_qq[p]) * inv_denom_table[q - p];
      if (s > z[k])
        break;
      k--;
    }
    k++;
    v[k] = q;
    z[k] = s;
    z[k + 1] = 1e30f;
  }
  k = 0;
  int vk = v[0];
  // Bolt optimization: cache f_vk = f[vk] in local register across queries q,
  // updating it only when k advances, avoiding repeated memory lookups f[vk].
  float f_vk = f[vk];
  float zk1 = z[1];
  float q_f = 0.0f;
  for (int q = 0; q < n; q++, q_f += 1.0f) {
    while (zk1 < q_f) {
      k++;
      vk = v[k];
      f_vk = f[vk];
      zk1 = z[k + 1];
    }
    float dq = q_f - (float)vk;
    d[q] = std::min(dq * dq + f_vk, 1e20f);
  }
}

// 2D distance transform using preallocated scratch buffers to eliminate
// redundant vector allocations between the inside and outside distance passes.
// Operates on raw pointers with cached column/row pointers to simplify indexing.
static void DistanceTransform(float *grid, int w, int h,
                              const float *qq, const float *inv_denom_table,
                              float *f, float *f_plus_qq,
                              float *d, float *z, int *v) {
  for (int x = 0; x < w; x++) {
    float *col = grid + x;
    for (int y = 0; y < h; y++)
      f[y] = col[(size_t)y * w];
    DistanceTransform1D(f, qq, f_plus_qq, inv_denom_table, d, h, v, z);
    for (int y = 0; y < h; y++)
      col[(size_t)y * w] = d[y];
  }
  for (int y = 0; y < h; y++) {
    float *row = grid + (size_t)y * w;
    DistanceTransform1D(row, qq, f_plus_qq, inv_denom_table, d, w, v, z);
    memcpy(row, d, w * sizeof(float));
  }
}

// Signed distances of a coverage's samples to its edge (at 0.5), in
// samples, positive inside.
std::vector<float> FromCoverage(const std::vector<float> &coverage, int w,
                                int h) {
  const float inf = 1e20f;
  const size_t sz = coverage.size();
  std::vector<float> toInside(sz), toOutside(sz);
  const float *cov = coverage.data();
  float *inside = toInside.data();
  float *outside = toOutside.data();
  for (size_t i = 0; i < sz; i++) {
    bool in = cov[i] >= 0.5f;
    inside[i] = in ? 0.0f : inf;
    outside[i] = in ? inf : 0.0f;
  }
  // Bolt optimization: eliminate 8 separate std::vector heap allocations per
  // FromCoverage call by using a contiguous stack scratch buffer for common UI
  // sizes (n <= 512, ~14 KB on stack) with heap fallback for larger dimensions.
  // Precomputing qq = q*q, inv_denom_table = 0.5f / diff, and caching f_plus_qq
  // eliminates divisions, float conversions, and redundant multiplications in
  // the inner loop.
  const int n = std::max(w, h);
  float stack_scratch[6 * 512 + 2];
  int stack_v[512];
  std::vector<float> heap_scratch;
  std::vector<int> heap_v;
  float *scratch = stack_scratch;
  int *v = stack_v;
  if (n > 512) {
    heap_scratch.resize(6 * n + 2);
    heap_v.resize(n);
    scratch = heap_scratch.data();
    v = heap_v.data();
  }
  float *qq = scratch;
  float *inv_denom = qq + n;
  float *f = inv_denom + (n + 1);
  float *f_plus_qq = f + n;
  float *d = f_plus_qq + n;
  float *z = d + n;

  for (int i = 0; i < n; i++)
    qq[i] = (float)(i * i);
  for (int i = 1; i <= n; i++)
    inv_denom[i] = 0.5f / (float)i;

  DistanceTransform(inside, w, h, qq, inv_denom, f, f_plus_qq, d, z, v);
  DistanceTransform(outside, w, h, qq, inv_denom, f, f_plus_qq, d, z, v);
  for (size_t i = 0; i < sz; i++) {
    float out_val = outside[i];
    inside[i] = (out_val > 0.0f) ? (sqrtf(out_val) - 0.5f) : (0.5f - sqrtf(inside[i]));
  }
  return toInside;
}

} // namespace DistanceField
