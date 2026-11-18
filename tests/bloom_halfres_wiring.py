#!/usr/bin/env python3
"""Bloom half-resolution wiring: the bright/blur passes run at half resolution (the four
blur passes drop to a quarter of the pixels; the composite upsamples linearly). Pins the
buffer sizes, the radius-matched blur offsets, the half-size pass viewport and the linear
composite sampler, and that no other pass still uses a bloom buffer as a full-res target.
The numeric equivalence itself is behavioural: post_shader_semantics.py's bloom section."""
import pathlib, re, sys
r = pathlib.Path(__file__).resolve().parents[1]
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False

gfx = (r / "src/render/gfx_gpu.cpp").read_text()

# Buffers are created at half resolution, rounded up.
need(re.search(r"bloomBuf1 = CreateTexture\([^;]*\(width \+ 1\) / 2, \(height \+ 1\) / 2", gfx, re.S),
     "bloomBuf1 created at half resolution")
need(re.search(r"bloomBuf2 = CreateTexture\([^;]*\(width \+ 1\) / 2, \(height \+ 1\) / 2", gfx, re.S),
     "bloomBuf2 created at half resolution")

# The chain renders into the half-size viewport, not the default full-res one.
need(re.search(r"bloomW = \(backWidth \+ 1\) / 2, bloomH = \(backHeight \+ 1\) / 2", gfx),
     "bloom block derives the half-size viewport from the back buffer")
need(gfx.count("bloomW, bloomH") >= 3,
     "bright pass and both blur passes render at bloomW x bloomH")

# One half-res texel spans two full-res ones: offsetScale 0.5 keeps the blur radius
# the same in full-res pixels (asserted numerically in post_shader_semantics.py).
need("BlurUniform hUniforms = {1,0,0.5f,3,{}}" in gfx, "horizontal blur offsetScale 0.5")
need("BlurUniform vUniforms = {0,1,0.5f,3,{}}" in gfx, "vertical blur offsetScale 0.5")

# The composite upsamples the half-res bloom buffer with a LINEAR filter; the default
# stage state is POINT, which would upscale blocky.
comp = gfx.index("PostProcessPass(cmd, BloomCompositePipeline()")
block = gfx[max(0, comp - 800):comp]
need("bloomUp.minFilter = bloomUp.magFilter = GFX_FILTER_LINEAR" in block,
     "composite builds a linear sampler for the bloom buffer")
need("{ bloomBuf1, Sampler(bloomUp) }" in block, "composite binds the bloom buffer linearly")

# Nothing else may use a bloom buffer as a full-res render target.
need("? bloomBuf2 :" not in gfx and "bloomBuf2 : motionBlurTemp" not in gfx,
     "no pass uses a (half-res) bloom buffer as a full-res target")

# Lens grime's scratch when motionBlurTemp is the input is a DEDICATED full-res target, not
# gradingTarget: the later speed streak skips while presentSource == gradingTarget and color
# grading renders into it (Devin review on #373 caught the gradingTarget version).
need(re.search(r"lensGrimeTarget = CreateTexture\([^;]*(?<!\()width, height", gfx, re.S),
     "lensGrimeTarget created at full resolution")
need(gfx.count("SDL_ReleaseGPUTexture(device, lensGrimeTarget);") == 2,
     "lensGrimeTarget released at both teardown sites")
need("bloomBuf2 == NULL || lensGrimeTarget == NULL" in gfx, "lensGrimeTarget in the creation null-check")
need("? lensGrimeTarget : motionBlurTemp" in gfx and "lensGrimeTarget : gradingTarget" not in gfx,
     "lens grime uses the dedicated scratch, never gradingTarget")

sem = (r / "tests/post_shader_semantics.py").read_text()
need("bloom half-res vs full-res" in sem and "bloom_chain" in sem,
     "post_shader_semantics.py keeps the behavioural half-res equivalence case")

print("bloom half-res wiring ok" if ok else "FAILED"); sys.exit(0 if ok else 1)
