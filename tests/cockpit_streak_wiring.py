"""Depth protection belongs to both the cockpit blur and later radial streak."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
f=(r/'src/render/shaders/speedstreak.frag').read_text();g=(r/'src/render/gfx_gpu.cpp').read_text();gl=(r/'src/render/gl_effects_executor.h').read_text()
assert 'vec4 cockpitDepth;'in f and'w *= cockpitEffect;'in f and'lines *= cockpitEffect;'in f
assert 'cockpitDepth.x * 1.4'in f
assert 'cockpitClear > 0.0f && sampledDepth && backDepth != nullptr'in g
assert 'ResolveProjection(&cockpitNear, &cockpitFar)'in g
assert '(vaporDraw || cockpitDepth)'in g
assert 'uniforms[4 + 60 * 4 + 8] = speedFocusSmooth'in g
assert 'float uniforms[4 + 60 * 4 + 4 + 4 + 4 + 4] = {}'in g
assert 'SpeedStreakUniforms is 1040 bytes'in gl
# GL now also uploads the trailing tunnel focus (zeroed trail/depth fields keep the mask off).
assert 'float u[260]'in gl and 'u[256] = tunnelFocusU' in gl and 'run(10, 0, u, sizeof(u))'in gl
print('cockpit streak mask wiring, depth validity, uniform offsets, GL disabled fallback: pass')
