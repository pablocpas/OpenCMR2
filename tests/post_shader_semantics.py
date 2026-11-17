#!/usr/bin/env python3
"""Optional translated-shader semantics test for dof.frag and motionblur.frag.

Compiles the production GLSL (glslang -> SPIR-V -> spirv-cross GLSL 330) and runs it on EGL (llvmpipe in CI-like hosts)
with synthetic stripes and known depths. It checks the maths of the translated shaders ONLY. It does not exercise the
SDL_GPU wiring, Metal, the real depth buffer, game assets or native appearance, and it does not say any look is desirable.
Cases that record current behaviour (raw-depth fallback saturation, the recursive motion blur ghost) are labelled as such.
"""
import pathlib
import re
import subprocess
import tempfile

import moderngl
import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[1]
SHADERS = ROOT / 'src/render/shaders'
W, H = 96, 64
NEAR, FAR = 0.1, 150.0


def translate(name):
    with tempfile.TemporaryDirectory() as scratch:
        spv = str(pathlib.Path(scratch) / 'x.spv')
        subprocess.run(['glslangValidator', '-V', str(SHADERS / (name + '.frag')), '-o', spv], check=True, stdout=subprocess.DEVNULL)
        return subprocess.check_output(['spirv-cross', spv, '--version', '330', '--no-es'], text=True)


ctx = moderngl.create_standalone_context(require=330, backend='egl')
VERT = '''#version 330
out vec2 inTex;
void main() { vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2); gl_Position = vec4(p * 2.0 - 1.0, 0, 1); inTex = p; }'''


def run(program, block, uniforms, textures):
    program[block].binding = 0
    ubo = ctx.buffer(np.array(uniforms, dtype='f4').tobytes()); ubo.bind_to_uniform_block(0)
    held = []
    for unit, (uniform, array, comps, dtype) in enumerate(textures):
        t = ctx.texture((W, H), comps, np.ascontiguousarray(array).tobytes(), dtype=dtype)
        t.filter = (moderngl.NEAREST, moderngl.NEAREST); t.repeat_x = t.repeat_y = False
        t.use(unit); program[uniform].value = unit; held.append(t)
    target = ctx.texture((W, H), 4); fbo = ctx.framebuffer([target]); fbo.use()
    vao = ctx.vertex_array(program, []); vao.render(vertices=3)
    out = np.frombuffer(fbo.read(components=4), dtype='u1').reshape(H, W, 4).copy()
    for o in held + [vao, fbo, target, ubo]: o.release()
    return out


def raw_depth(z):
    # hardware depth for view distance z, same formula dof.frag inverts
    return FAR / (FAR - NEAR) * (1.0 - NEAR / z)


def focus_pos(z):
    return float(np.clip(np.log(max(z, NEAR) / NEAR) / np.log(FAR / NEAR), 0, 1))


stripes = np.zeros((H, W, 4), dtype='u1'); stripes[:, :, 3] = 255
stripes[:, ((np.arange(W) // 3) % 2) == 0, :3] = 255  # 3 px wide white/black vertical stripes


def contrast(img, region=(slice(24, 40), slice(36, 60))):
    return float(img[region][:, :, 0].astype(float).std())


# ---------------- depth of field ----------------
dof = ctx.program(vertex_shader=VERT, fragment_shader=translate('dof'))


def dof_run(depth, focal, rng, blur, tunnel=0.0, aspect=1.0, tunnel_px=0.0, near=NEAR, far=FAR):
    u = [focal, rng, blur, tunnel, aspect, tunnel_px, near, far]
    return run(dof, 'DoFUniforms', u, [('sceneTex', stripes, 4, 'f1'), ('depthTex', np.asarray(depth, dtype='f4'), 1, 'f4')])


base = contrast(stripes)
# 1) In-focus distance is preserved exactly (log focus, real near/far).
z_focus = 12.0
same = dof_run(np.full((H, W), raw_depth(z_focus)), focus_pos(z_focus), 0.2, 6)
assert np.array_equal(same, stripes), 'pixels at the focus distance must be untouched'
# 2) Contrast falls monotonically with |log distance - focus| (log CoC), and far geometry is softer than focus.
rows = []
for z in [12, 16, 24, 40, 80, 140]:
    c = contrast(dof_run(np.full((H, W), raw_depth(z)), focus_pos(z_focus), 0.45, 6))
    rows.append((z, c))
vals = [c for _, c in rows]
assert all(a >= b - 1e-6 for a, b in zip(vals, vals[1:])), ('contrast must not rise with distance from focus', rows)
print('dof contrast vs z (focus 12 m):', [(z, round(c, 1)) for z, c in rows], 'input', round(base, 1)); assert vals[0] == base and vals[-1] < 0.8 * base, rows
# 3) Mixed scene: a focused near band stays sharp while the far band is blurred in the same frame.
mixed = np.full((H, W), raw_depth(140.0), dtype='f4'); mixed[:, :W // 2] = raw_depth(z_focus)
m = dof_run(mixed, focus_pos(z_focus), 0.3, 6)
assert np.array_equal(m[:, 4:W // 2 - 8], stripes[:, 4:W // 2 - 8]) and contrast(m, (slice(24, 40), slice(W // 2 + 12, W - 10))) < base
# 4) Raw-depth fallback (near/far unknown) is the old linear |depth-focus| rule. Recorded behaviour: hardware depths of
# 12 m and 140 m are both above 0.97, so with a mid focus they get the SAME saturated blur (no distance discrimination).
a = contrast(dof_run(np.full((H, W), raw_depth(12.0)), 0.5, 0.44, 6, near=0, far=0))
b = contrast(dof_run(np.full((H, W), raw_depth(140.0)), 0.5, 0.44, 6, near=0, far=0))
assert abs(a - b) < 1e-6 and a < base, ('raw fallback saturates', a, b)
# 5) Tunnel vision with DoF blur 0: peripheral blur radius only, centre untouched, grows with radius from centre.
flat = np.full((H, W), raw_depth(z_focus), dtype='f4')
t0 = dof_run(flat, focus_pos(z_focus), 0.3, 0, tunnel=0.0, aspect=W / H, tunnel_px=8)
assert np.array_equal(t0, stripes), 'tunnel 0 and blur 0 must be identical to input'
t1 = dof_run(flat, focus_pos(z_focus), 0.3, 0, tunnel=1.0, aspect=W / H, tunnel_px=8)
centre = (slice(24, 40), slice(W // 2 - 8, W // 2 + 8))
left = (slice(24, 40), slice(2, 14))
assert np.array_equal(t1[centre], stripes[centre]), 'centre must stay sharp'
assert contrast(t1, left) < 0.8 * base, 'periphery must be blurred by tunnel vision alone'
radial = [contrast(t1, (slice(26, 38), slice(x, x + 6))) for x in range(W // 2, 0, -8)]
print('tunnel contrast centre->edge:', [round(r, 1) for r in radial]); assert all(p >= q - 1e-6 for p, q in zip(radial, radial[1:])), ('tunnel blur must not shrink toward the edge', radial)
# 6) Tunnel closes with speed: higher tunnel means equal or lower contrast at a fixed peripheral pixel.
cs = [contrast(dof_run(flat, focus_pos(z_focus), 0.3, 0, tunnel=t, aspect=W / H, tunnel_px=8), (slice(26, 38), slice(10, 22))) for t in (0.25, 0.5, 0.75, 1.0)]
assert all(p >= q - 1e-6 for p, q in zip(cs, cs[1:])), cs

# ---------------- motion blur (frame feedback) ----------------
mb = ctx.program(vertex_shader=VERT, fragment_shader=translate('motionblur'))
mix_max = int(re.search(r'kMotionBlurMixMax\s*=\s*(\d+)', (ROOT / 'src/render/motion_blur_limits.h').read_text()).group(1))


def solid(v):
    a = np.zeros((H, W, 4), dtype='u1'); a[:, :, :3] = v; a[:, :, 3] = 255; return a


def blur_step(cur, prev, mix, keep=0.0, rect=(0.0, 0.0, 0.0, 0.0), feather=0.02):
    return run(mb, 'MotionBlurUniforms', [mix, keep, feather, 0.0, *rect], [('current', cur, 4, 'f1'), ('previous', prev, 4, 'f1')])


# one-frame blend is mix(current, previous, m)
for m in (0.0, 0.25, 0.5, 1.0):
    out = blur_step(solid(200), solid(40), m)
    assert abs(int(out[10, 10, 0]) - round(200 * (1 - m) + 40 * m)) <= 1, (m, out[10, 10])


def frames_to_reach(mix, target, limit=600):
    # live frame is white (255) from frame 0; previous starts black; out feeds back as previous
    prev, cur = solid(0), solid(255)
    for n in range(1, limit + 1):
        prev = blur_step(cur, prev, mix)
        if prev[5, 5, 0] >= target: return n, int(prev[5, 5, 0])
    return None, int(prev[5, 5, 0])


# recorded behaviour: at m=1.0 the output never leaves the old frame (the freeze reported at motion blur 100)
n, v = frames_to_reach(1.0, 1, limit=40)
assert n is None and v == 0, ('mix 1.0 must freeze', n, v)
# at the clamp the picture does update: N-frame recursion follows 255*(1-m^N), within 8-bit rounding drift
m = mix_max / 255.0
prev, cur = solid(0), solid(255)
for n in range(1, 31):
    prev = blur_step(cur, prev, m)
    if n in (1, 5, 20):
        expected = 255 * (1 - m ** n)
        assert abs(int(prev[5, 5, 0]) - expected) <= 4, (n, int(prev[5, 5, 0]), expected)
n90, _ = frames_to_reach(m, 230)
# recorded behaviour, not a desired look: the ghost of a flip from black to white lasts this many frames at the clamp
print('clamp mix', mix_max, '/255; frames for a hard cut to reach 230/255 (90%%): %s; fraction of old frame left after 20 frames: %.2f' % (n90, m ** 20))
assert n90 is not None

# car keep (video.motion_blur_car): inside the followed car's screen rectangle the mix is gated to 0 -
# the car stays sharp while the world around it blends. rect = u0 v0 u1 v1 (uv, v downwards).
cur, prev = solid(200), solid(40)
gated = blur_step(cur, prev, 0.9, keep=1.0, rect=(0.25, 0.25, 0.75, 0.75))
mid = int(gated[H // 2, W // 2, 0])
edge = int(gated[H // 2, 4, 0])
assert abs(mid - 200) <= 1, ('inside the car rect the pixel is the current frame', mid)
assert abs(edge - round(200 * 0.1 + 40 * 0.9)) <= 1, ('outside the rect the world still blurs', edge)
# feather: across the rect boundary the blend ramps smoothly, no hard edge
ramp = [int(gated[H // 2, x, 0]) for x in range(int(0.25 * W) - 4, int(0.25 * W) + 5)]
assert all(a <= b + 1 for a, b in zip(ramp, ramp[1:])), ('feather ramps monotonically from world to car', ramp)
# a zero rectangle never gates (no valid car region, e.g. cockpit camera)
zero = blur_step(cur, prev, 0.9, keep=1.0, rect=(0.0, 0.0, 0.0, 0.0))
assert abs(int(zero[H // 2, W // 2, 0]) - round(200 * 0.1 + 40 * 0.9)) <= 1, ('zero rect = no gate', int(zero[H // 2, W // 2, 0]))
print('car keep: inside %d, outside %d, zero-rect %d (current 200, previous 40, mix 0.9)' % (mid, edge, int(zero[H // 2, W // 2, 0])))
print('PASS translated dof.frag and motionblur.frag semantics on', ctx.info['GL_RENDERER'], '(shader maths only; no SDL_GPU/Metal/depth-buffer/native claim)')
