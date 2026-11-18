#ifndef OPENCMR2_MOTION_BLUR_HISTORY_H
#define OPENCMR2_MOTION_BLUR_HISTORY_H
// Motion blur history ping-pong (SDL_GPU executor): two textures alternate as "the last blended frame". The blend
// reads one slot and writes the other, which becomes next frame's history - so the steady state needs no
// full-screen copy per frame (the old code copied the blended output into a single history texture every frame).
// Blend semantics are unchanged: next frame's history is this frame's blended output, exactly what the copy made.
struct MotionBlurHistory {
    int read;    // the slot holding the previous blended frame (valid only when seeded)
    int seeded;  // 0 until a frame has been copied or blended into a slot (toggle, resize, non-race frame reset)
};
inline MotionBlurHistory MotionBlurHistory_Reset() { MotionBlurHistory h = {0, 0}; return h; }
// The slot the blend writes this frame. Always the one it does not read.
inline int MotionBlurHistory_Write(const MotionBlurHistory &h) { return 1 - h.read; }
// After a successful blend the written slot is next frame's history.
inline void MotionBlurHistory_Commit(MotionBlurHistory &h) { h.read = 1 - h.read; h.seeded = 1; }
// After a seed copy into the read slot (first frame after any reset): history exists, the index is unchanged.
inline void MotionBlurHistory_Seed(MotionBlurHistory &h) { h.seeded = 1; }
#endif
