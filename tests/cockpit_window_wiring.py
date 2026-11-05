#!/usr/bin/env python3
"""Wiring checks for the cockpit exterior-window no-depth-write fix (video.cockpit_glass_fog)."""
import sys
def rd(p): return open(p, encoding="utf-8").read()
def need(c, m):
    if not c:
        print("FAIL:", m); sys.exit(1)
gm = rd("game/Game.cpp"); gh = rd("game/Graphics.h"); gc = rd("game/Graphics.cpp"); cw = rd("game/CockpitWindow.h")
need('CockpitWindow_SkipDepthWrite' in cw and 'nodeFlagsLowByte' in cw, "pure decision helper in CockpitWindow.h")
need('viewType == 3' in cw and '== 0x14' in cw, "helper gates on cockpit view type 3 and node flag 0x14")
need('CockpitWindow_SkipDepthWrite(node->flags&0xff,1,viewType,Sys_GetOption("video.cockpit_glass_fog",1),interiorNode)' in gm, "gate reads the option with default 1 and the live view type")
need('DrawCarScopedLOD(pNode,pMesh,bit)' in gm and 'DrawCarScopedLOD(pNode,pMesh,-1)' in gm, "view-mask draws pass the view bit; the full-scene pass passes -1 (unchanged)")
need('AttractCapture_ViewSignature(bit,&viewType,&viewDrone)' in gm, "view type comes from the actual view record")
need('int CGraphics::GetZWriteEnable(void)' in gc and 'static int GetZWriteEnable(void);' in gh, "depth-write state is saved, not hardcoded")
need('CGraphics::GetZWriteEnable()==1' in gm and 'CGraphics::SetZWriteEnable(0);' in gm and 'CGraphics::SetZWriteEnable(1);' in gm, "no-zwrite is scoped and restored")
need('cockpit window: no depth writes in cockpit view' in gm, "one-time engagement log present")
need(gm.count('DrawCarScopedLOD(SceneNode*node,Mesh*mesh,int bit)') == 1, "single scoped-LOD chokepoint carries the gate")
print("cockpit window wiring ok")

need("CockpitWindow_InInterior(node, g_carInteriorRoots[CarBodyDirt::drawingCar])" in gm,"exclude actual interior root and descendants, not guessed mesh counts")
