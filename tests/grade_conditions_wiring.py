# Per-condition race grade (video.color_grading) wiring: the race viewport's
# per-frame sun hook (view 0) drives the grade from live stage signals, the
# grade travels inside the recorded world-end command so the executors apply
# the record-time value (not whatever later Set calls left in their state),
# and the race frame wrapper restores the restrained menu recipe afterwards
# so the race grade cannot leak into menus. The mapping itself is
# behaviourally pinned in grade_conditions.cpp; this file pins the call sites.
import pathlib, sys

root = pathlib.Path(__file__).resolve().parents[1]

def rd(p): return (root / p).read_text()

hook = rd("game/StageObjects.cpp")
race = rd("game/RallyData.cpp")
settings = rd("src/render/effect_settings.h")
core = rd("src/render/gfx_core.cpp")
internal = rd("src/render/gfx_internal.h")
gl = rd("src/render/gfx_gl.cpp")
gpu = rd("src/render/gfx_gpu.cpp")

def need(cond, msg):
    if not cond:
        print("grade_conditions_wiring:", msg)
        sys.exit(1)

need("StageObject_UpdateConditionGrade();" in hook,
     "the per-frame race hook must update the condition grade (view 0 owns the frame)")
need("StageObject_GetViewWeatherStateByte(0)" in hook,
     "the grade must read the view-0 weather kind (wet mapping)")
need("StageObject_GetRotatedStageLightVector" in hook,
     "the grade must read the sun vector (golden-hour elevation)")
need("g_stageLighting" in hook and "lightColour" in hook,
     "the grade must read the blended stage light colour (golden-hour warmth)")
need("Sys_GetTicksPrecise" in hook,
     "the wet cross-fade must advance on wall-clock dt, not a fixed per-frame step")
need("GradeStrength" in settings and "ApplyGrade" in settings,
     "effect_settings must share the grade strength rule and menu recipe")
need("EffectSettings::ApplyGrade();" in race[race.index("Gfx_SetRaceWorld(FALSE);"):],
     "the race frame wrapper must restore the menu grade after race frames")
# The executors run effects when the recorded frame executes; the grade must
# be snapshotted into the world-end command at record time and applied from
# the command at execution time, in both backends.
need("int grade[4]" in internal,
     "GfxCommand must carry the record-time grade snapshot")
need("s_gradeArgs" in core and "memcpy(c.grade,s_gradeArgs" in core.replace(" ", ""),
     "Gfx_EndWorldEffects must snapshot the grade into the command")
need("SetColorGrading(c.grade[0],c.grade[1],c.grade[2],c.grade[3])" in gl.replace(" ", ""),
     "the GL executor must apply the command's grade snapshot at world end")
need("SetColorGrading(frame.commands[worldEnd].grade[0]" in gpu.replace(" ", ""),
     "the SDL_GPU executor must apply the command's grade snapshot at world end")
# No effects pass runs without a world-end boundary, so a boundary-less menu
# frame can never consume a stale race grade from executor state: the GL
# executor's only ApplyEffects call sits behind the single-boundary gate, and
# the SDL_GPU executor gates its post chain on worldEffects (boundaries==1).
gl_apply = [l for l in gl.splitlines() if "ApplyEffects()" in l and "GLuint" in l]
need(len(gl_apply) == 1, "the GL executor must have exactly one ApplyEffects call site")
gate_at = gl.index("boundaries==1")
need(gate_at < gl.index("ApplyEffects()"), "the GL single-boundary gate must precede ApplyEffects")
need("boolworldEffects=present&&boundaries==1;" in gpu.replace(" ", ""),
     "the SDL_GPU post chain must stay gated on a single world-end boundary")
print("grade_conditions_wiring OK")
