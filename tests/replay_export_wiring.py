#!/usr/bin/env python3
"""Wiring checks for --replay-export (fixed-step replay video export with audio)."""
import sys
def rd(p): return open(p, encoding="utf-8").read()
def need(c, m):
    if not c:
        print("FAIL:", m); sys.exit(1)
en = rd("src/platform/enhancements.h"); sysh = rd("src/port/sys.h"); sysc = rd("src/platform/sys.cpp")
au_h = rd("src/port/audio.h"); au = rd("src/audio/audio_sdl.cpp"); rx = rd("game/ReplayExport.h")
car = rd("game/Car.cpp"); race = rd("game/Race.cpp"); fe = rd("game/FrontendScreens.cpp")
need('--replay-export' in en and 'replayExportDir' in en, "CLI flag must parse into EnhancementOptions")
need('Sys_GetReplayExportDir' in sysh and 'Sys_GetReplayExportDir' in sysc, "Sys must expose the export dir")
need('Sys_RunDetached' in sysh and 'posix_spawnp' in sysc and '_spawnvp' in sysc, "detached no-shell spawn in the port layer")
need('Audio_MixOffline' in au_h and 'Audio_MixOffline' in au, "offline mix tap must exist")
need('MixInto' in au and 'MixInto(s_mix.data(), frames);' in au, "Feed must share the mixing body with the offline tap")
need('!s_paused' in au, "offline tap must refuse while the device is unpaused (cursor race guard)")
need('SDL_PauseAudioStreamDevice' in au, "pause must stop device consumption, not just mute output")
need('ReplayExport::OverrideClock(&g_carSimulationFrameMs);' in car, "sim-clock gate hook at the replay clock feed")
need('s.heldMs += kStepMs' in rx and 'kStepMs = 40' in rx, "one 40 ms step per acknowledged frame")
need('Gfx_GetCaptureSerial' in rx and 'Gfx_RequestCapture' in rx, "capture handshake")
need('kSaveTimeoutFrames' in rx, "save-ack timeout must exist (no hang)")
need('size <= 0' in rx, "serial bumps even on SDL_SavePNG failure, so the saved file must be verified")
need('ReplayExport::Tick();' in race, "per-frame driver hook in the race update")
need('ReplayExport::OnMainMenu();' in fe, "menu-exit finalize hook")
need('Audio_SetPaused(0);' in rx and 'Audio_SetPaused(1);' in rx, "live audio must be paused for the tap and restored after")
need('Audio_GetOutputRate' in rx, "export requires an initialized mixer")
need('PatchWavHeader' in rx and 'ftell' in rx, "WAV must stay valid on crash/quit (patched per tap)")
need('framerate' in rx and 'libx264' in rx and 'export-command.txt' in rx, "ffmpeg assembly + saved manual command")
print("replay export wiring ok")
