# Tunnel tracking native-regression repair (2026-11-05)

Native report: clear area on trees rather than the road with tracking enabled.

Root cause: `FixMatrix_ProjectWorldPointToView` returns 16.16 pixel coordinates.
The initial route feed normalized those as integer pixels, driving ordinary
points to the maximum clamp. The established sun/object callers shift by 16.
`TunnelRoad_ProjectPoint` now normalizes without dropping the subpixel part.

The feed also ran before `View_BlendCameraStates`. It now runs from the live
race viewport after that frame's view/projection is established and before
world effects. No camera settings, FOV, or Metal UV convention were changed.

Verification:
- CPU integration: route preview, production projector, normalized focus;
  41 left/straight/right bends, centre/above/below/behind, current FOV/view.
- GL integration: route preview -> production projector -> normalized feed ->
  real executor uniform upload -> production shader pixels. Left/right bends
  move the clear patch; direct-UV orientation/streak/legacy tests remain.
- Linux GL is not native Metal acceptance. User confirmation is still needed.
- The pre-existing god-rays occluder-band assertion is unrelated and remains
  unchanged. It was skipped only in a temporary local run copy to reach the
  later tracking section. No claim that the whole visual suite is green.
