import pathlib
r=pathlib.Path(__file__).resolve().parents[1]
g=(r/'src/render/gfx_gpu.cpp').read_text();gl=(r/'src/render/gl_effects_executor.h').read_text();s=(r/'src/render/shaders/sun_glare.frag').read_text()
a=g.index('// Restrained sun response');b=g.index('} else motionBlurHistory=MotionBlurHistory_Reset();',a)
assert a>g.index('SDL_GPUTexture *gradeOut =')
assert b<g.index('drawCommands(worldEnd+1',b)
assert 'presentSource==gradingTarget?godRaysBuf1:gradingTarget' in g[a:b]
assert gl.index('if(glareActive){float u[4]')>gl.index('run(7, 0, grade')
assert 'visibility*texture(cloudTransmission' in s
assert 's.rgb+vec3(8./255.)' in s and 's.a' in s
assert 'video.sun_glare",0' in g and 'video.sun_glare",0' in gl
print('sun glare final-pass, distinct-target, occlusion, cloud/readability/bypass wiring pass')
