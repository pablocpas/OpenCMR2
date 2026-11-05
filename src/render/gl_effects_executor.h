// Included inside the GL Executor. Deliberately GL-only: no game data or shared
// options, no changes to SDL_GPU, and no extra third-party GL API requirement.
void ReleaseEffectTargets()
{
    gl.DeleteTextures(4, effectTex); gl.DeleteFramebuffers(4, effectFbo);
    gl.DeleteTextures(1, &historyTex);gl.DeleteTextures(1,&cloudMaskTexture);gl.DeleteFramebuffers(1,&cloudMaskFbo);cloudMaskTexture=cloudMaskFbo=0;
    memset(effectTex, 0, sizeof(effectTex)); memset(effectFbo, 0, sizeof(effectFbo));
    historyTex = 0; historyValid = false;
}
void ReleaseShadow()
{
    gl.DeleteTextures(1, &shadowMap); gl.DeleteRenderbuffers(1, &shadowDepth);
    gl.DeleteFramebuffers(1, &shadowFbo);
    shadowMap = shadowDepth = shadowFbo = 0; shadowReady = false;
}
void DestroyEffects()
{
    ReleaseEffectTargets(); ReleaseShadow();
    for (GLuint &p : effectPrograms) { gl.DeleteProgram(p); p = 0; }
    gl.DeleteProgram(shadowProgram); gl.DeleteProgram(weatherProgram);
    gl.DeleteBuffers(1, &effectUbo); gl.DeleteBuffers(1, &weatherBuffer);
    gl.DeleteVertexArrays(1, &effectVao); gl.DeleteVertexArrays(1, &weatherVao);
}
// Raw post-process operations intentionally bypass the ordinary draw cache.
// Forget it completely before returning to normal geometry; never leave cached
// depth, viewport, program or attribute assumptions from a full-screen pass.
void ForgetEffectState()
{
    bound = Bound();
    gl.BindVertexArray(vertexArray);
    gl.Enable(GL_SCISSOR_TEST);
}
void EffectState(GLuint fbo, int width, int height)
{
    gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl.Viewport(0, 0, width, height); gl.DepthRange(0, 1);
    gl.Disable(GL_SCISSOR_TEST); gl.Disable(GL_BLEND); gl.Disable(GL_CULL_FACE);
    gl.Disable(GL_DEPTH_TEST); gl.DepthMask(GL_FALSE);
    gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}
GLuint EffectTexture(int width, int height)
{
    GLuint t = 0; gl.GenTextures(1, &t); gl.BindTexture(GL_TEXTURE_2D, t);
    gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return t;
}
bool EffectTargets()
{
    if (effectTex[0]) return true;
    gl.ActiveTexture(GL_TEXTURE0);
    gl.GenFramebuffers(4, effectFbo);
    for (int i = 0; i < 4; ++i) {
        effectTex[i] = EffectTexture(backWidth, backHeight);
        gl.BindFramebuffer(GL_FRAMEBUFFER, effectFbo[i]);
        gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, effectTex[i], 0);
        if (gl.CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            Sys_Log("gfx: GL effect framebuffer incomplete; presenting original scene");
            ReleaseEffectTargets(); ForgetEffectState(); return false;
        }
    }
    historyTex = EffectTexture(backWidth, backHeight);
    if (!effectUbo) gl.GenBuffers(1, &effectUbo);
    if (!effectVao) gl.GenVertexArrays(1, &effectVao);
    ForgetEffectState(); return true;
}
GLuint EffectProgram(int effect)
{
    if (effectPrograms[effect]) return effectPrograms[effect];
    static const unsigned char *sources[] = {g_brightpass_frag_glsl, g_blur_frag_glsl,
        g_composite_frag_glsl, g_motionblur_frag_glsl, g_lens_grime_frag_glsl,
        g_dof_frag_glsl, g_godRays_frag_glsl, g_colorGrading_frag_glsl, g_spatial_aa_frag_glsl,g_clouds_frag_glsl,g_speedstreak_frag_glsl,g_sun_glare_frag_glsl,g_fog_frag_glsl};
    static const char *blocks[] = {"BrightPassUniforms", "BlurUniforms", "CompositeUniforms",
        "MotionBlurUniforms", "LensGrimeUniforms", "DoFUniforms", "GodRaysUniforms", "ColorGradingUniforms", "SpatialAAUniforms","CloudUniforms","SpeedStreakUniforms","SunGlareUniforms","FogUniforms"};
    static const char *samplers0[] = {"scene", "source", "scene", "current", "scene", "sceneTex", "scene", "scene", "scene", "scene", "scene", "scene", "scene"};
    static const char *samplers1[] = {NULL, NULL, "bloom", "previous", NULL, "depthTex", "depthTex", NULL, NULL,"depthTex", NULL,"depthTex", "depthTex"};
    GLuint p = Link(glEffectsVertex, sources[effect]);
    if (p) {
        GLuint block = gl.GetUniformBlockIndex(p, blocks[effect]);
        if (block != GL_INVALID_INDEX) gl.UniformBlockBinding(p, block, FRAGMENT_BLOCK);
        gl.UseProgram(p);
        gl.Uniform1i(gl.GetUniformLocation(p, samplers0[effect]), 0);
        if (samplers1[effect]) gl.Uniform1i(gl.GetUniformLocation(p, samplers1[effect]), 1);
    }
    if(p&&(effect==6||effect==11))gl.Uniform1i(gl.GetUniformLocation(p,"cloudTransmission"),2);
    effectPrograms[effect] = p;
    return p;
}
bool EffectPass(int effect, int target, GLuint source, GLuint second, const void *data, size_t bytes)
{
    // Prevent undefined read/write feedback even when effects are stacked.
    if (effectTex[target] == source || (second && effectTex[target] == second)) return false;
    GLuint p = EffectProgram(effect);
    if (!p) { ForgetEffectState(); return false; }
    EffectState(effectFbo[target], backWidth, backHeight);
    gl.UseProgram(p); gl.BindVertexArray(effectVao);
    GfxStageState linear; linear.minFilter = linear.magFilter = GFX_FILTER_LINEAR; linear.address = GFX_ADDRESS_CLAMP;
    GfxStageState nearest; nearest.address = GFX_ADDRESS_CLAMP;
    for (int i = 0; i < 2; ++i) {
        gl.ActiveTexture(GL_TEXTURE0 + i);
        gl.BindTexture(GL_TEXTURE_2D, i ? (second ? second : white2D) : source);
        gl.BindSampler(i, Sampler(i == 1 && second == backDepth ? nearest : linear));
    }
    // std140 blocks are padded, including shaders whose public argument is one float.
    if(effect==6||effect==11){gl.ActiveTexture(GL_TEXTURE0+2);gl.BindTexture(GL_TEXTURE_2D,cloudMask?cloudMask:white2D);gl.BindSampler(2,Sampler(linear));}
    // SpeedStreakUniforms is 1040 bytes (trail data the GL path never fills): keep the zeroed buffer at least that large so the block is never read out of range. Zero depth info = trails off.
    unsigned char block[sizeof(CloudUniforms) > 1040 ? sizeof(CloudUniforms) : 1040] = {};
    if (bytes > sizeof(block)) { ForgetEffectState(); return false; }
    memcpy(block, data, bytes);
    gl.BindBuffer(GL_UNIFORM_BUFFER, effectUbo);
    gl.BufferData(GL_UNIFORM_BUFFER, sizeof(block), block, GL_STREAM_DRAW);
    gl.BindBufferRange(GL_UNIFORM_BUFFER, FRAGMENT_BLOCK, effectUbo, 0, sizeof(block));
    gl.DrawArrays(GL_TRIANGLES, 0, 3);
    GLenum error = gl.GetError();
    ForgetEffectState();
    if (error != GL_NO_ERROR) { Sys_Log("gfx: GL effect %d failed (%#x); retaining previous result", effect, (unsigned)error); return false; }
    return true;
}
GLuint ApplyEffects()
{
    {   // speed streak easing (0.4 s), race frames only; mirrors the SDL_GPU path
        auto now = std::chrono::steady_clock::now();
        float dt = speedStreakTimed ? std::chrono::duration<float>(now - speedStreakTime).count() : 0.0f;
        speedStreakTime = now; speedStreakTimed = true;
        if (dt > 0.25f) dt = 0.25f;
        speedStreakClock = std::fmod(speedStreakClock + dt, 1000.0f);
        speedStreakSmooth += (speedStreakTarget - speedStreakSmooth) * (1.0f - std::exp(-dt / 0.4f));
        if (!effectsRaceFrame) speedStreakSmooth = 0.0f;
    }
    {   // tunnel vision easing (0.8 s), the same time constant and race-frame gate as the SDL_GPU path
        auto now = std::chrono::steady_clock::now();
        float dt = speedFocusTimed ? std::chrono::duration<float>(now - speedFocusTime).count() : 0.0f;
        speedFocusTime = now; speedFocusTimed = true;
        if (dt > 0.25f) dt = 0.25f;
        speedFocusSmooth += (speedFocusTarget - speedFocusSmooth) * (1.0f - std::exp(-dt / 0.8f));
        if (!effectsRaceFrame) speedFocusSmooth = 0.0f;
    }
    {   // tunnel road tracking easing (0.4 s), race frames only; mirrors the SDL_GPU path
        auto now = std::chrono::steady_clock::now();
        float dt = tunnelFocusTimed ? std::chrono::duration<float>(now - tunnelFocusTime).count() : 0.0f;
        tunnelFocusTime = now; tunnelFocusTimed = true;
        if (dt > 0.25f) dt = 0.25f;
        float k = 1.0f - std::exp(-dt / 0.4f);
        tunnelFocusU += ((tunnelFocusTargetValid ? tunnelFocusTargetU : 0.5f) - tunnelFocusU) * k;
        tunnelFocusV += ((tunnelFocusTargetValid ? tunnelFocusTargetV : 0.5f) - tunnelFocusV) * k;
        if (!effectsRaceFrame) { tunnelFocusU = 0.5f; tunnelFocusV = 0.5f; }
    }
    bool depthBlurOn = effectsRaceFrame && dofOn && dofRange > 0 && dofBlur > 0;
    bool tunnelOn = effectsRaceFrame && (speedFocusTarget > 0.0f || speedFocusSmooth > 0.001f);
    // Overlay status, same codes as SDL_GPU: 1 off, 3 nothing to do, 5 encoded (set below), 6 not a race frame.
    Gfx_SetDofStatus(GfxDofStatus{!effectsRaceFrame ? 6 : (depthBlurOn || tunnelOn) ? 4 : dofOn ? 3 : 1, speedFocusTarget, speedFocusSmooth, 0.0f, 0.0f});
    bool streakOn = speedStreakSmooth > 0.01f && effectsRaceFrame;
    auto glareNow=std::chrono::steady_clock::now();
    float glareDt=sunGlareTimed?std::chrono::duration<float>(glareNow-sunGlareTime).count():0.f;
    sunGlareTime=glareNow;sunGlareTimed=true;
    int glareAmount=Sys_GetOption("video.sun_glare",0);
    float glareStrength=sunGlare.Step(sunX,sunY,effectsRaceFrame&&sunVisible,glareAmount,glareDt);
    bool glareActive=effectsRaceFrame&&glareAmount>0&&glareStrength>.0001f;
    bool active = glareActive || streakOn || cloudsValid || spatialAA || bloomOn && bloomAmount > 0 || motionOn && motionMix > 0 ||
        depthBlurOn || tunnelOn || raysOn && raysAmount > 0 && sunVisible || effectsRaceFrame && fogOn && fogAmount > 0 ||
        grade[0] || grade[1] || grade[2] != 1 || grade[3] != 1 || grime[0] || grime[1] || grime[2];
    if (!active) { historyValid = false; return backFramebuffer; }
    if (!EffectTargets()) return backFramebuffer;
    GLuint source = backColor, sourceFbo = backFramebuffer;
    cloudMask=0;
    auto run = [&](int effect, GLuint second, const void *data, size_t bytes) {
        int target = source == effectTex[0] ? 1 : 0;
        if (EffectPass(effect, target, source, second, data, bytes)) {
            source = effectTex[target]; sourceFbo = effectFbo[target];
            static const unsigned bits[]={0,0,2,4,0,8,16,32,1,0,256,0,512};
            encodedEffects |= bits[effect];
            return true;
        }
        return false;
    };
    if(spatialAA){
        float size[4]={1.0f/backWidth,1.0f/backHeight,0,0};
        run(8,0,size,sizeof(size));
    }
    // Copy the cloud result out of scratch slot3 so later bloom cannot
    // overwrite its explicit transmission mask.
    if(cloudsValid && EffectPass(9,3,source,backDepth,&cloudUniforms,sizeof(cloudUniforms))){
        if(!cloudMaskTexture){cloudMaskTexture=EffectTexture(backWidth,backHeight);gl.GenFramebuffers(1,&cloudMaskFbo);gl.BindFramebuffer(GL_FRAMEBUFFER,cloudMaskFbo);gl.FramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,cloudMaskTexture,0);
            if(gl.CheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){gl.DeleteTextures(1,&cloudMaskTexture);gl.DeleteFramebuffers(1,&cloudMaskFbo);cloudMaskTexture=cloudMaskFbo=0;}
        }
        if(cloudMaskFbo){
        gl.BindFramebuffer(GL_READ_FRAMEBUFFER,effectFbo[3]);gl.BindFramebuffer(GL_DRAW_FRAMEBUFFER,cloudMaskFbo);gl.Disable(GL_SCISSOR_TEST);
        gl.BlitFramebuffer(0,0,backWidth,backHeight,0,0,backWidth,backHeight,GL_COLOR_BUFFER_BIT,GL_NEAREST);
        if(gl.GetError()==GL_NO_ERROR){source=cloudMaskTexture;sourceFbo=cloudMaskFbo;cloudMask=cloudMaskTexture;
            if(!cloudLogged){Sys_Log("options: GL volumetric clouds encoded (48 view / 4 sun samples, early-out density), transmission coupled to rays; native pixels unverified");cloudLogged=true;}
        }}ForgetEffectState();
    }
    if (bloomOn && bloomAmount > 0) {
        float bright[4] = {0.7f, 0.1f, 0, 0};
        bool ok = EffectPass(0, 2, source, 0, bright, sizeof(bright));
        // Match the existing finite Gaussian, with enough std140 storage.
        struct Blur { float x,y,scale; int samples; float pad[4]; };
        Blur horizontal = {1,0,1,3,{}}, vertical = {0,1,1,3,{}};
        for (int i = 0; ok && i < 2; ++i) {
            ok = EffectPass(1, 3, effectTex[2], 0, &horizontal, sizeof(horizontal));
            ok = ok && EffectPass(1, 2, effectTex[3], 0, &vertical, sizeof(vertical));
        }
        float amount[4] = {bloomAmount / 255.0f, 0, 0, 0};
        if (ok) run(2, effectTex[2], amount, sizeof(amount));
    }
    if (motionOn && motionMix > 0) {
        if (historyValid) {
            float mix[4] = {motionMix / 255.0f, 0, 0, 0};
            run(3, historyTex, mix, sizeof(mix));
        }
        // Seed history on the FIRST frame without blending uninitialized pixels.
        gl.BindFramebuffer(GL_FRAMEBUFFER, sourceFbo);
        gl.ActiveTexture(GL_TEXTURE0); gl.BindTexture(GL_TEXTURE_2D, historyTex);
        gl.CopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, backWidth, backHeight);
        historyValid = gl.GetError() == GL_NO_ERROR;
        ForgetEffectState();
    } else historyValid = false;
    if (grime[0] || grime[1] || grime[2]) run(4, 0, grime, sizeof(grime));
    // Atmosphere fog (video.fog): before DoF/rays so blur and shafts stack on the fogged
    // scene. Near/far come from the race camera's recorded projection (WORLD_END
    // capture); the shader un-remaps the GL half-range buffer depth via extra.z.
    // When recovery failed (nearZ/farZ stay 0) the shader keeps its raw-depth ramp.
    if (effectsRaceFrame && fogOn && fogAmount > 0) {
        // FogUniforms: packed by the single shared packer (render/fog_uniforms.h).
        // The depth gate is active here too: SetFogLights remaps each light's
        // depthRef into this path's half-range depth buffer.
        FogUniformParams fogP;
        fogP.density = Fog_Density(fogAmount);
        fogP.glow = fogGlow / 100.0f;
        fogP.nearZ = glFogNearZ; fogP.farZ = glFogFarZ;
        fogP.sunX = sunX; fogP.sunY = sunY;
        fogP.sunVisible = sunVisible ? 1.0f : 0.0f;
        fogP.aspect = backHeight ? (float)backWidth / (float)backHeight : 1.0f;
        fogP.clearDistance = (float)fogClearDistance;
        fogP.lightStrength = std::max(0, std::min(200, Sys_GetOption("video.fog_lights", 100))) / 100.0f;
        fogP.depthHalfRange = 1.0f;   // the GL buffer holds 0.5+0.5*d_game
        float u[kFogUniformFloats];
        FogUniforms_Pack(u, fogP, fogLights, fogLightCount);
        run(12, backDepth, u, sizeof(u));
    }
    if (depthBlurOn || tunnelOn) {
        // Same 8-float DoFUniforms layout as SDL_GPU. Depth blur keeps the existing GL look (raw depth, no near/far recovery);
        // tunnel vision adds the speed-driven peripheral blur with its own radius (kTunnelRadiusPerHeight x height px at full strength).
        float aspect = backHeight ? (float)backWidth / (float)backHeight : 1.0f;
        float focusRaw = glCarFocusRaw >= 0.0f ? glCarFocusRaw : dofFocus / 100.0f;
        float u[20] = {focusRaw, dofRange / 100.0f * (1.0f - 0.5f * std::min(speedFocusSmooth, 1.0f)), depthBlurOn ? dofBlur * 0.04f : 0.0f,
                      speedFocusSmooth, aspect, kTunnelRadiusPerHeight * (float)backHeight, 0.0f, 0.0f};
        // tunnelFocus at [16..19]: the eased road-tracking focus. GL's gl_FragCoord is bottom-up,
        // so the v-downwards feed is pre-flipped here (SDL_GPU uploads it unchanged). z 1 = use it.
        u[16] = tunnelFocusU; u[17] = 1.0f - tunnelFocusV; u[18] = 1.0f; u[19] = 0.0f;
        if (run(5, backDepth, u, sizeof(u)))
            Gfx_SetDofStatus(GfxDofStatus{5, speedFocusTarget, speedFocusSmooth, u[1], u[2], focusRaw, glCarFocusRaw >= 0.0f ? glCarDepth : 0.0f, 0, 0.0f, 0.0f, 0.0f, 0.0f, tunnelFocusU, tunnelFocusV, tunnelFocusTargetValid});
    }
    if (raysOn && raysAmount > 0 && sunVisible) {
        float u[4] = {raysAmount / 255.0f, sunX, sunY, 0};
        run(6, backDepth, u, sizeof(u));
    }
    if (streakOn) {
        // SpeedStreakUniforms: settings + zeroed trail/depth fields + tunnelFocus at [256..259]
        // (same eased focus as the DoF pass, v pre-flipped for GL's bottom-up gl_FragCoord).
        float u[260] = {speedStreakSmooth, backHeight ? (float)backWidth / (float)backHeight : 1.0f, speedStreakClock, speedStreakSmooth};
        u[256] = tunnelFocusU; u[257] = 1.0f - tunnelFocusV; u[258] = 1.0f; u[259] = 0.0f;
        run(10, 0, u, sizeof(u));
    }
    if (grade[0] || grade[1] || grade[2] != 1 || grade[3] != 1) run(7, 0, grade, sizeof(grade));
    if(glareActive){float u[4]={sunX,sunY,glareStrength,backHeight?(float)backWidth/backHeight:1.f};run(11,backDepth,u,sizeof(u));}
    return sourceFbo;
}
void PrepareShadow()
{
    shadowReady = false;
    if (!shadowType) return;
    if (!shadowProgram) {
        shadowProgram = Link(g_shadow_vert_glsl, g_shadow_frag_glsl);
        if(shadowProgram){
            gl.UseProgram(shadowProgram);
            gl.Uniform1i(gl.GetUniformLocation(shadowProgram,"coverage"),0);
            GLuint block=gl.GetUniformBlockIndex(shadowProgram,"ShadowCoverage");
            if(block!=GL_INVALID_INDEX)gl.UniformBlockBinding(shadowProgram,block,FRAGMENT_BLOCK);
        }
    }
    if (!shadowProgram) { ForgetEffectState(); return; }
    if (!shadowFbo) {
        gl.ActiveTexture(GL_TEXTURE0);
        gl.GenFramebuffers(1, &shadowFbo); gl.BindFramebuffer(GL_FRAMEBUFFER, shadowFbo);
        gl.GenTextures(1, &shadowMap); gl.BindTexture(GL_TEXTURE_2D, shadowMap);
        gl.TexImage2D(GL_TEXTURE_2D, 0, GL_R32F, shadowSize, shadowSize, 0, GL_RED, GL_FLOAT, NULL);
        gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadowMap, 0);
        gl.GenRenderbuffers(1, &shadowDepth); gl.BindRenderbuffer(GL_RENDERBUFFER, shadowDepth);
        gl.RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, shadowSize, shadowSize);
        gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, shadowDepth);
        if (gl.CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            Sys_Log("gfx: GL shadow framebuffer incomplete; original shadows retained");
            ReleaseShadow(); ForgetEffectState(); return;
        }
    }
    shadowReady = true; ForgetEffectState();
}
void RenderShadow(const GfxFrame &frame)
{
    if (!shadowReady) return;
    EffectState(shadowFbo, shadowSize, shadowSize);
    gl.Enable(GL_DEPTH_TEST); gl.DepthMask(GL_TRUE); gl.DepthFunc(GL_LEQUAL);
    gl.ClearColor(1,1,1,1); gl.ClearDepth(1); gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl.UseProgram(shadowProgram); gl.BindVertexArray(vertexArray);
    GfxTexture *target = NULL;
    for (size_t i = 0; i < frame.commands.size(); ++i) {
        const GfxCommand &c = frame.commands[i];
        if (c.type == GFX_CMD_TARGET) { target = c.target; continue; }
        if (c.type != GFX_CMD_DRAW || c.transformed || target || c.primitive != GFX_TRIANGLELIST) continue;
        const GfxDrawState &state = frame.states[c.state];
        if (!Gfx_ShadowCaster(state)) continue;
        bool cutout=Gfx_CutoutShadow(state);
        GlTexture *coverage=cutout?Ensure(Gfx_DrawnTexture(state.stages[0].texture)):nullptr;
        if(cutout&&!coverage)continue;
        gl.ActiveTexture(GL_TEXTURE0);gl.BindTexture(GL_TEXTURE_2D,cutout?coverage->texture:white2D);
        GfxStageState sample=state.stages[0];
        gl.BindSampler(0,Sampler(sample));
        if(!effectUbo)gl.GenBuffers(1,&effectUbo);
        float settings[4]={cutout?1.f:0.f,(state.alphaRef&255)/255.f,state.alphaFunc==GFX_CMP_GREATEREQUAL?1.f:0.f,state.stages[0].alphaOp==GFX_TOP_MODULATE?1.f:0.f};
        gl.BindBuffer(GL_UNIFORM_BUFFER,effectUbo);gl.BufferData(GL_UNIFORM_BUFFER,sizeof(settings),settings,GL_STREAM_DRAW);
        gl.BindBufferRange(GL_UNIFORM_BUFFER,FRAGMENT_BLOCK,effectUbo,0,sizeof(settings));
        gl.BindBufferRange(GL_UNIFORM_BUFFER, VERTEX_BLOCK, uniformBuffer,
            drawUniforms[i].vertexOffset, sizeof(VertexUniforms));
        BindVertices(c.buffer ? ((GlVertexBuffer *)c.buffer->gpu)->buffer : vertexBuffer, false);
        GLint first = (GLint)(c.firstVertexByte / sizeof(GfxVertex));
        if (c.firstIndex != ~0u)
            gl.DrawElementsBaseVertex(PrimitiveMode(c.primitive), c.indexCount, GL_UNSIGNED_SHORT,
                (const void *)(uintptr_t)(c.firstIndex * 2), first);
        else gl.DrawArrays(PrimitiveMode(c.primitive), first, c.vertexCount);
    }
    if (gl.GetError() != GL_NO_ERROR) { shadowReady = false; Sys_Log("gfx: GL shadow pass failed"); }
    if(shadowReady) encodedEffects |= 64;
    ForgetEffectState();
}
float WeatherRandom()
{
    weatherSeed = weatherSeed * 1664525u + 1013904223u;
    return (weatherSeed & 0x7fffffff) / float(0x7fffffff);
}
void RenderWeather()
{
    Uint64 now = SDL_GetTicks();
    float dt = weatherTicks ? std::min(0.05f, float(now - weatherTicks) / 1000.0f) : 0;
    weatherTicks = now;
    if (!weatherType || !weatherAmount) { weather.clear(); weatherCarry = 0; return; }
    if (!weatherProgram) weatherProgram = Link(glWeatherVertex, glWeatherFragment);
    if (!weatherProgram) { ForgetEffectState(); return; }
    weatherCarry += (weatherType == 1 ? 400.0f : 120.0f) * weatherAmount / 255.0f * dt;
    int count = int(weatherCarry); weatherCarry -= count;
    for (int i = 0; i < count && weather.size() < 2048; ++i) {
        float a = WeatherRandom(), b = WeatherRandom();
        weather.push_back(weatherType == 1 ? WeatherParticle{a,-0.05f-b*0.1f,0,0.9f+a*1.2f,0.0015f,0.018f,1.5f} :
            WeatherParticle{a,-0.05f-b*0.1f,(b-0.5f)*0.15f,0.15f+a*0.25f,0.008f,0.008f,6});
    }
    struct Vertex { float x,y,r,g,b,a; };
    std::vector<Vertex> vertices;
    // Screen-space snow glints (video.snow_glint): flakes twinkle toward the sun
    // (forward scatter) and brighten inside projected car light beams, tinted
    // per light. Same maths as the SDL_GPU particle path (weather_glint.h).
    float glintAmt = weatherType == 2 ? std::max(0, std::min(200, Sys_GetOption("video.snow_glint", 100))) / 100.0f : 0.0f;
    float glintAspect = backHeight > 0 ? (float)backWidth / (float)backHeight : 1.0f;
    int flakeIndex = 0;
    for (auto &p : weather) {
        p.x += p.vx * dt; p.y += p.vy * dt; p.life -= dt;
        if (p.life <= 0 || p.y > 1.05f || p.x < -0.05f || p.x > 1.05f) continue;
        float pr = weatherType == 1 ? 0.7f : 0.95f, pg = weatherType == 1 ? 0.8f : 0.98f, pb = 1.0f;
        if (glintAmt > 0.0f) {
            float prox = WeatherGlint_SunProximity(p.x, p.y, sunX, sunY, sunVisible ? 1 : 0, glintAspect);
            float m = WeatherGlint_Twinkle(WeatherGlint_Phase(flakeIndex, p.life), prox);
            m = 1.0f + (m - 1.0f) * glintAmt;
            pr *= m; pg *= m; pb *= m;
            for (int li = 0; li < fogLightCount; ++li) {
                const GfxFogLight *L = &fogLights[li];
                WeatherGlint_LightBoost(p.x, p.y, glintAspect, L->x, L->y, L->intensity, L->r, L->g, L->b, &pr, &pg, &pb);
            }
            if (pr > 1.f) pr = 1.f;
            if (pg > 1.f) pg = 1.f;
            if (pb > 1.f) pb = 1.f;
        }
        ++flakeIndex;
        static const int corner[6][2] = {{-1,-1},{1,-1},{-1,1},{-1,1},{1,-1},{1,1}};
        for (auto &c : corner) vertices.push_back({p.x+c[0]*p.sx,p.y+c[1]*p.sy,pr,pg,pb,
            weatherType == 1 ? 0.35f : 0.5f});
    }
    weather.erase(std::remove_if(weather.begin(), weather.end(), [](const WeatherParticle &p) {
        return p.life <= 0 || p.y > 1.05f || p.x < -0.05f || p.x > 1.05f;
    }), weather.end());
    if (vertices.empty()) { ForgetEffectState(); return; }
    if (!weatherVao) gl.GenVertexArrays(1, &weatherVao);
    if (!weatherBuffer) gl.GenBuffers(1, &weatherBuffer);
    EffectState(backFramebuffer, backWidth, backHeight);
    gl.Enable(GL_BLEND); gl.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl.UseProgram(weatherProgram); gl.BindVertexArray(weatherVao);
    gl.BindBuffer(GL_ARRAY_BUFFER, weatherBuffer);
    gl.BufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(Vertex), vertices.data(), GL_STREAM_DRAW);
    gl.EnableVertexAttribArray(0); gl.VertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),0);
    gl.EnableVertexAttribArray(1); gl.VertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,sizeof(Vertex),(const void *)(2*sizeof(float)));
    gl.DrawArrays(GL_TRIANGLES,0,vertices.size());
    if(gl.GetError()==GL_NO_ERROR) encodedEffects |= 128;
    ForgetEffectState();
}
