# Preserve opaque cockpit shell depth (2026-11-05)

The earlier exterior-glass gate used car ownership + node type low byte 0x14.
That type is reused by the original Subaru interior root: private source
suba5.c3d has root flags 0x1014 and its mesh contains 327 vertices / 291 triangles.
It is the opaque cockpit shell, not a glass-only pane. The type-only gate
therefore suppressed shell/bonnet/dashboard depth too. Two gate engagements
were not evidence of two actual glass panes.

Narrow the existing gate with actual ancestry against the owning car's interior
root. Its descendants retain normal depth; exterior glass keeps the previous
no-depth-write behaviour. No mesh-count or asset-name heuristic in runtime.

Tests:
- Interior root/child vs exterior sibling classification, option and view gates.
- Production decision -> real GL draw/depth -> real fog shader: old type-only
  gate lets a beam paint over an opaque shell quad; ancestry fix blocks that
  beam while a beam over open road remains visible.
- Existing exterior-glass fog/crack behaviour and chase identity still pass.
- Full Linux build + focused unit/wiring pass. Linux GL is not native Metal
  acceptance. Native bonnet and fog outcomes remain open.

Private game assets and derived render previews are not in this repository.
