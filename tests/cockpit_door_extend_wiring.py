#!/usr/bin/env python3
"""Wiring checks for video.cockpit_door_extend (default off, Subaru-only, idempotent match)."""
import re, sys
def rd(p): return open(p, encoding="utf-8").read()
def need(c, m):
    if not c:
        print("FAIL:", m); sys.exit(1)
so = rd("game/StageObjects.cpp"); en = rd("src/platform/enhancements.h"); rb = rd("rebuild.sh")
need('Sys_GetOption("video.cockpit_door_extend", 0) > 0' in so, "feature must default off (option default 0)")
need("field_0x10 != 327" in so, "only the 327-vertex Subaru cabin mesh may be touched")
need(all(k in so for k in ("top < 0 || bot < 0 || topNb < 0 || botNb < 0",)), "all four reference vertices must match before anything moves")
need(so.count("Mesh_RefreshVertices(pLod)") >= 1, "vertices must be re-uploaded")
need("StageObject_ExtendInteriorDoorCard(g_carInteriorRoots[car]" in so, "must be called after the interior is built")
need("--cockpit-door-extend" in en and "video.cockpit_door_extend" in en, "CLI flag must map to the option")
need("n>80" in en, "CLI range must be capped")
need("--cockpit-door-extend" in rb, "rebuild.sh must pass the flag through")
print("cockpit door extend wiring ok")
