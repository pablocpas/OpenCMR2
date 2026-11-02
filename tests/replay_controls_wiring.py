#!/usr/bin/env python3
"""Replay controls: clock hook, update/draw sites and key filter exist and are replay-only. Source wiring only, not a native check."""
import pathlib, sys
r = pathlib.Path(__file__).resolve().parents[1]
rd = lambda p: (r / p).read_text(encoding="utf-8")  # README carries UTF-8 punctuation; Windows default cp1252 decode fails
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False
need("g_replayClock.Frame(CMain::GetFrameTime(), ReplayControls_TimeScale())" in rd("game/Car.cpp"), "sim clock hook")
rally = rd("game/RallyData.cpp")
need("float ReplayControls_TimeScale(void) { return ReplayControls_Active() ? g_replayControls.Scale() : 1.0f; }" in rally, "scale must be 1 outside replay")
need("if (!ReplayControls_Active()) { g_replayControls.Reset(); View_FreeCamStop(); View_ReplayChaseReset(); return; }" in rally, "state resets when not replaying (free cam AND chase orbit)")
need("ReplayControls_Draw();" in rally and "void ReplayControls_Draw(void)\n{\n    if (!ReplayControls_Active()) return;" in rally, "OSD only while replaying")
need("ReplayControls_Update();" in rd("game/Race.cpp"), "update site")
need("ReplayControls_KeyIsReplayKey" in rd("game/Input.cpp"), "keys must not reach the game")
need("key == INPUT_KEY_SPACE || Tuning_ToggleKeyAllowed(key)" in rally and "Space" in rd("README.md"), "Space is a replay key; refused keys stay refused; no driving-binding veto")
need("int View_CycleReplayCamera(unsigned char view)" in rd("game/Car.cpp") and "ReplayCamera_Next(mode, available)" in rd("game/Car.cpp"), "camera cycle computes the wrap before switching")
need("View_CycleReplayCamera(0);" in rally and "View_FindFreeModeSlot" not in rally.split("static void ReplayControls_CycleCamera")[1].split("void ReplayControls_Update")[0], "no double switch through the game step")
need("ReplayStepsTotal(packets, count)" in rally, "total from the recorded packets")
cam=rd("game/Car.cpp")
need("View_FreeCamFrame(0, &in," in rally and "View_FreeCamStop();" in rally.split("void ReplayControls_Update")[1].split("}")[0], "free cam runs every frame from the replay update (zero sim steps too) and stops when the replay ends")
need("ReplayControls_KeyIsFreeCamKey" in rd("game/Input.cpp") and "for (int i = 0; i < 13; i++) if (ReplayControls_KeyIsFreeCamKey(freeCamKeys[i])) m_keyboardState[freeCamKeys[i]] = 0;" in rd("game/Input.cpp"), "free cam keys are removed from the driving state only while the replay free cam is active")
# The orbit-key capture gate uses the OPEN tuning panel, never the sticky
# engaged/preview flag (dead arrows after any F10 press was the user bug).
need("ReplayChaseOrbit::InputCaptured(ReplayMenuOwnsInput(), TuningPanel_IsOpen())" in rally and "CGameInfo::IsInputAndGamePaused() != 0" in rally, "orbit keys captured by any menu/pause input mode or the OPEN tuning panel")
need("!ReplayMenuOwnsInput() && ReplayKeyUsable(key)" in rally, "replay keys (3, 4, Space, Tab) also reach an open menu instead of being swallowed")
need("TuningPanel_PreviewActive()){View_ReplayChaseFrame" not in rally, "sticky tuning preview must not zero the orbit input")
need("g_viewPreviousStates[view].matrix = *m" in cam and "kReplayFreeCamMode" in cam, "free cam overrides only the shown pose")
need("free cam" in rd("src/platform/enhancements.h").lower() or "Free camera" in rd("src/platform/enhancements.h"), "help names the free camera keys")

print("replay controls wiring ok" if ok else "FAILED"); sys.exit(0 if ok else 1)
