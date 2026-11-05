# Windshield dirt composition repair (2026-11-05)

Native report: dirt map total grows and the overlay reports drawing, but no smudge
is visible in the cockpit, including rainy driving.

`RallyData_UpdateViewFrameAndDebugOverlay` prepares a view. Its caller then runs
`RallyData_DrawViewEndFrameOverlay`, which draws the entire 3D world through
`Game_DrawSceneViewport`. The immediate dirt draw in the preparation callback
was therefore covered by the world.

Keep accumulation in the preparation callback, compose the dirt after world
effects and before queued HUD layers. The draw now isolates and restores state,
including texture colour/alpha ops, depth, alpha test, fog, culling and texgen.

Rendered regression uses the actual production draw through recording and the
real GL executor. Old ordering is pixel-identical to clean despite drawing;
new ordering shows dirt and wiped arcs while leaving the dash band untouched.
Poisoned inherited world states do not change the fixed output. The old test's
red-channel darkening assumption was invalid on the warm dirt tint; measure the
blue channel instead. Repair stale game-side link stubs to run that harness.

Linux GL is not native Metal acceptance. This fixes composition of the existing
dirt/wet-wipe map, not new physically refractive raindrops. Native visual review
remains open. Reproduce: python3 tests/windshield_render.py --build build/linux.
