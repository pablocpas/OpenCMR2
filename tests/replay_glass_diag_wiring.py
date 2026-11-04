#!/usr/bin/env python3
"""Wiring checks for video.diag_replay_glass (default off; replay-only transition logging)."""
import sys
def rd(p): return open(p, encoding="utf-8").read()
def need(c, m):
    if not c:
        print("FAIL:", m); sys.exit(1)
en = rd("src/platform/enhancements.h"); st = rd("game/StageTiming.cpp"); so = rd("game/StageObjects.cpp")
need('Sys_GetOption("video.diag_replay_glass", 0)' in st and 'Sys_GetOption("video.diag_replay_glass", 0)' in so, "both call sites must read the option with default 0")
need(st.count('CGameInfo::GetConfiguredGameMode() == 4') >= 2 and 'CGameInfo::GetConfiguredGameMode() == 4' in so, "logging must be replay-mode only (game mode 4)")
need('replay-glass-diag: break' in st and 'replay-glass-diag: damage-flag' in st and 'replay-glass-diag: reset-restore' in so, "all three transition logs present")
need('pRecord->partBroken[i] = 1;' in st and 'pRecord->partDamaged[i] = 1;' in st, "flag writes unchanged")
need('--diag-replay-glass' in en and 'video.diag_replay_glass' in en, "CLI flag must map to the option")
print("replay glass diag wiring ok")
