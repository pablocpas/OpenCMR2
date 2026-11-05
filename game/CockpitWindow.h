#ifndef _COCKPIT_WINDOW_H
#define _COCKPIT_WINDOW_H

// video.cockpit_glass_fog (default on): in a cockpit camera the car's exterior
// window node must not write depth. Cockpit mode hides the whole exterior
// body except this node (node flag low byte 0x14, see the view-mask switch in
// RallyData.cpp), so it is the only exterior surface across the windscreen
// area - and its depth writes put glass distance into the depth buffer the
// fog pass samples (no fog through the windscreen) and occlude the
// depth-tested windscreen crack pane (cracks submitted but never visible).
// Skipping its depth writes in cockpit views only fixes both without
// changing chase views, where the same node keeps writing depth. The node is
// the window by classification, so no material (blend/vertex-alpha) guard is
// applied: real cockpit glass assets may carry texture-only alpha.
inline bool CockpitWindow_SkipDepthWrite(unsigned nodeFlagsLowByte, bool nodeBelongsToCar, int viewType, int optionOn, bool interiorNode = false)
{
    return !interiorNode && optionOn != 0 && nodeBelongsToCar && nodeFlagsLowByte == 0x14 && viewType == 3;
}

// Interior and exterior files reuse low-byte node type 0x14. The Subaru
// interior root is 0x1014 and contains the opaque 291-triangle cockpit shell,
// so type alone cannot identify glass. Check actual interior-root ancestry.
template<class Node> inline bool CockpitWindow_InInterior(Node *node, Node *root)
{
    if (!root) return false;
    for (Node *p = node; p; p = p->pParent) if (p == root) return true;
    return false;
}
#endif
