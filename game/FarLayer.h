#ifndef FARLAYER_H
#define FARLAYER_H

// PORT: a low-detail stand-in for the stage beyond its view distance.
struct SceneNode;

// Draws the far layer behind the stage, after the sky and the backdrop and
// before the stage itself, for the view of pCamera with the stage's far plane
// set. It builds the layer the first time it draws a stage. The depth buffer
// is cleared after it, so the stage always draws over it.
void FarLayer_Draw(SceneNode *pCamera);

// Whether the stage dissolves into the layer toward its far plane
// (Game_BeginDistanceFade).
int FarLayer_StageFades(void);

// Tests: drawn turns the layer on and off at any time (after the option
// allows it), fade the stage's fade into it, marks paints the layer in marker
// colours (Gfx_SetDistanceMarks), magenta nearer than half the view distance
// and green beyond, and keySky paints the sky and the backdrop green (where
// the layer would be drawn), to tell them from the stage.
void FarLayer_SetTest(int drawn, int fade, int marks, int keySky);

#endif
