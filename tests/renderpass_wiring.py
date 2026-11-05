from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'src/render/gfx_gpu.cpp').read_text()
p=s[s.index('SDL_GPUGraphicsPipeline *ParticlePipeline()'):s.index('SDL_GPUGraphicsPipeline *ShadowPipeline()')]
assert 'has_depth_stencil_target = false' in p
r=s[s.index('bool RenderParticles('):s.index('SDL_GPUSampler *Sampler(')]
assert 'Gfx_BeginColourOnlyPass(cmd,backColor,SDL_GPU_LOADOP_LOAD)' in r
assert 'SDL_GPUDepthStencilTargetInfo' not in r
# Remaining raw calls are actual geometry + shadow, both with validated depth.
assert s.count('SDL_BeginGPURenderPass(')==2
b=s[s.index('SDL_GPURenderPass *BeginPass('):s.index('void Execute(')]
assert 'if (!color.texture || !depth.texture) return nullptr' in b
sh=s[s.index('if (shadowType != 0 && shadowMap != NULL'):s.index('size_t worldEnd=frame.commands.size();')]
assert 'depth.texture = shadowDepth' in sh
print('all render-pass attachment sites audited; native Metal acceptance remains open')

# Default changes must respect saved choices, not set/overwrite them.
root=Path(__file__).resolve().parents[1]
car=(root/'game/CarEffects.cpp').read_text()
graphics=(root/'game/Graphics.cpp').read_text()
assert 'Sys_GetOption("video.dust_plume", 1)' in car
assert 'Sys_GetOption("video.dust_volume",1)' in graphics
assert 'Sys_SetOption("video.dust_' not in car+graphics

# Binding contract supplements the executable recorder's command-order test.
game=(root/'game/Game.cpp').read_text()
view=game[game.index('int Game_DrawSceneViewport(SceneNode *pRoot, SceneNode *pCamera, void *pRect, int bit, BYTE flag)\n{'):game.index('void CGame::SetSectorDrawState(')]
assert view.index('CGraphics::SetProjection(0, 0, CGraphics::m_farPlaneFixed, 0)') < view.index('Particle_DrawAll(')
assert view.index('Particle_DrawAll(') < view.index('Game_DrawCulledSectorMeshes(')
assert view.index('Scene_DrawShadowBatches(bit)') < view.index('Gfx_FlushDustVolume()') < view.index('Flare_SampleVisibility(') < view.index('Sprite_DrawLayer(3)')

# Every fullscreen constructor is bound by PostProcessPass, which has no
# depth attachment. Shader depth samplers do not make a depth target.
import re
constructors = dict(re.findall(r'SDL_GPUGraphicsPipeline \*(\w+)Pipeline\([^)]*\)\s*\{(.*?)\n    \}', s, re.S))
fullscreen = {"MotionBlur", "PostProcess", "LensGrime", "SpeedStreak", "Dof", "GodRays", "Fog", "ColorGrading"}
assert fullscreen <= constructors.keys(), "fullscreen constructor audit incomplete"
for name, body in constructors.items():
    if name in {"Clear", "Shadow"} or "SDL_GPUGraphicsPipelineCreateInfo" not in body:
        continue  # real geometry/shadow passes attach depth
    assert 'has_depth_stencil_target = false' in body, name + " must match its color-only pass"
    assert 'has_depth_stencil_target = true' not in body, name + " unexpectedly declares depth"
pass_source = (root/'src/render/postprocess_pass.h').read_text()
assert 'SDL_BeginGPURenderPass(cmd,&colour,1,nullptr)' in pass_source
helper = s[s.index('bool PostProcessPass('):s.index('bool PrepareDustResources(')]
assert 'Gfx_BeginPostProcessPass(' in helper
for name in fullscreen - {"PostProcess"}:
    assert re.search(r'PostProcessPass\(cmd,\s*'+name+r'Pipeline\(\)', s), name + " pass route changed"
print('all fullscreen pipeline declarations match color-only passes')
