// Euclidean distance transform shared by fonts and vector sprites.
#include "distance_field.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace DistanceField {
// Squared Euclidean distance transform (Felzenszwalb and Huttenlocher).
// Bolt optimization: precompute coordinates squared qq[q] = q*q once, cache
// f_plus_qq[q] to avoid recomputing f[p] + p*p in the parabola intersection loop,
// convert division by 2*(q-p) to multiplication by 0.5f / (q-p), and use single-precision float
// for intersection boundaries z.
static void DistanceTransform1D(const float *f, const float *qq,
                                float *f_plus_qq, float *d, int n, int *v,
                                float *z) {
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
      float inv_denom = 0.5f / (float)(q - p);
      s = (fq_plus_qq - f_plus_qq[p]) * inv_denom;
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
  for (int q = 0; q < n; q++) {
    while (z[k + 1] < q)
      k++;
    float dq = (float)(q - v[k]);
    d[q] = std::min(dq * dq + f[v[k]], 1e20f);
  }
}

// 2D distance transform using preallocated scratch buffers to eliminate
// redundant vector allocations between the inside and outside distance passes.
static void DistanceTransform(std::vector<float> &grid, int w, int h,
                              const std::vector<float> &qq,
                              std::vector<float> &f,
                              std::vector<float> &f_plus_qq,
                              std::vector<float> &d, std::vector<float> &z,
                              std::vector<int> &v) {
  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++)
      f[y] = grid[(size_t)y * w + x];
    DistanceTransform1D(f.data(), qq.data(), f_plus_qq.data(), d.data(), h,
                        v.data(), z.data());
    for (int y = 0; y < h; y++)
      grid[(size_t)y * w + x] = d[y];
  }
  for (int y = 0; y < h; y++) {
    DistanceTransform1D(&grid[(size_t)y * w], qq.data(), f_plus_qq.data(),
                        d.data(), w, v.data(), z.data());
    memcpy(&grid[(size_t)y * w], d.data(), w * sizeof(float));
  }
}

// Signed distances of a coverage's samples to its edge (at 0.5), in
// samples, positive inside.
std::vector<float> FromCoverage(const std::vector<float> &coverage, int w,
                                int h) {
  const float inf = 1e20f;
  std::vector<float> toInside(coverage.size()), toOutside(coverage.size());
  for (size_t i = 0; i < coverage.size(); i++) {
    bool in = coverage[i] >= 0.5f;
    toInside[i] = in ? 0.0f : inf;
    toOutside[i] = in ? inf : 0.0f;
  }
  // Bolt optimization: allocate 1D transform scratch buffers once per
  // FromCoverage call and reuse across both inside and outside 2D passes.
  // Precomputing qq = q*q and caching f_plus_qq eliminates redundant
  // multiplications in the inner loop.
  const int n = std::max(w, h);
  std::vector<float> qq(n);
  for (int i = 0; i < n; i++)
    qq[i] = (float)(i * i);
  std::vector<float> f(n), f_plus_qq(n), d(n);
  std::vector<float> z(n + 1);
  std::vector<int> v(n);

  DistanceTransform(toInside, w, h, qq, f, f_plus_qq, d, z, v);
  DistanceTransform(toOutside, w, h, qq, f, f_plus_qq, d, z, v);
  for (size_t i = 0; i < coverage.size(); i++)
    toInside[i] = toOutside[i] > 0 ? sqrtf(toOutside[i]) - 0.5f
                                   : 0.5f - sqrtf(toInside[i]);
  return toInside;
}

} // namespace DistanceField
