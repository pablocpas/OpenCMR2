#ifndef OPENCMR2_REPLAY_EXPORT_H
#define OPENCMR2_REPLAY_EXPORT_H
// --replay-export DIR: export the first replay played after launch as a video.
//
// The replay is re-simulated deterministically at 25 sim steps/sec. This driver
// captures exactly one PNG per sim step: it gates the sim clock (OverrideClock)
// so the sim advances a single 40 ms step only after the previous frame's save
// has been acknowledged (Gfx_GetCaptureSerial), however long PNG encoding takes
// in wall clock. No simulated frame is ever dropped.
//
// Audio: the game mixer is pull-based (audio_sdl.cpp Feed). With the device
// paused (Audio_SetPaused, which stops device consumption, not just output) the
// driver taps rate/25 mixed samples per captured frame via Audio_MixOffline -
// the same mix the player hears - into a WAV. Game sound parameters are set
// from the sim-step update, so per-step tapping stays in sync with the frames.
//
// Mux: ffmpeg is a hard requirement, preflighted (Sys_CommandExists) before any
// frame is captured; without it the export fails early with a clear dependency
// message instead of producing frames that are not a video. At the end the exact
// command is written to export-command.txt and ffmpeg runs synchronously
// (Sys_RunAndWait): success requires exit code 0 AND a nonempty replay.mp4,
// anything else is reported as a failure with the manual command kept.
//
// Scope: export of the FIRST replay played this launch; capture only, no
// gameplay or physics input. Audio is required: any persistent audio failure
// aborts the whole export. Every abort leaves the WAV valid up to the last
// written sample, restores the pre-export audio pause state, and lets the
// replay play on.
#include "port/sys.h"
#include "port/gfx.h"
#include "port/audio.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

bool ReplayControls_Active(void);

namespace ReplayExport {

static const int kStepMs = 40;          // one sim step = 1/25 s
static const int kSaveTimeoutFrames = 900; // rendered frames to await a save ack before aborting
static const unsigned kMaxWavData = 0x7f000000u; // stay well under the 4 GiB RIFF limit

struct State {
    bool init = false, enabled = false;
    bool started = false, done = false, failed = false;
    bool pending = false;        // a capture request is awaiting its save ack
    bool gateInit = false;
    int heldMs = 0;              // virtual sim clock while exporting
    int lastRealMs = 0;          // last real clock seen, for the gate base
    bool haveLast = false;
    int waitFrames = 0;
    unsigned serial = 0;
    long frames = 0;             // PNG frames acknowledged
    long taps = 0;               // audio slices written (== frames when in sync)
    double tapFrac = 0.0;        // fractional sample accumulator (rate/25 may be non-integer)
    BOOL wasPaused = 0;          // audio pause state before the export; restored at the end
    int rate = 0, channels = 0;
    unsigned wavData = 0;
    FILE *wav = NULL;
    char dir[600] = "";
    char framePath[1200] = "";
};

inline State &S() { static State s; return s; }

inline bool Enabled() {
    State &s = S();
    if (!s.init) {
        s.init = true;
        const char *d = Sys_GetReplayExportDir();
        if (d && *d) { s.enabled = true; snprintf(s.dir, sizeof s.dir, "%s", d); }
    }
    return s.enabled;
}

// Export is driving the clock only while a replay plays and the export is live.
inline bool Active() {
    State &s = S();
    return Enabled() && !s.done && !s.failed && ReplayControls_Active();
}

// Sim-clock gate, consulted once per frame where the replay clock feeds the
// step count (Car.cpp). While exporting the real clock is replaced by a virtual
// one that advances exactly one 40 ms step per acknowledged frame save, so the
// accumulator schedules exactly one sim step per exported frame and never a
// real-time catchup burst. Outside an active export the clock passes through.
inline void OverrideClock(int *frameMs) {
    State &s = S();
    if (!Enabled() || s.done || s.failed)
        return;
    if (!ReplayControls_Active()) {
        // Not in a replay (before start or after the end): pass through and
        // track the real clock for the gate base. Tick/OnMainMenu finalize.
        s.lastRealMs = *frameMs; s.haveLast = true;
        return;
    }
    if (!s.gateInit) {
        s.gateInit = true;
        // Start held at the previous real value so the first gated frame
        // schedules zero steps: frame 0 is the replay start state.
        s.heldMs = s.haveLast ? s.lastRealMs : *frameMs;
    }
    *frameMs = s.heldMs;
}

inline void PatchWavHeader(State &s) {
    if (!s.wav)
        return;
    long here = ftell(s.wav);
    unsigned char h[4];
    unsigned riff = 36 + s.wavData;
    h[0] = (unsigned char)(riff & 0xff); h[1] = (unsigned char)((riff >> 8) & 0xff);
    h[2] = (unsigned char)((riff >> 16) & 0xff); h[3] = (unsigned char)((riff >> 24) & 0xff);
    fseek(s.wav, 4, SEEK_SET); fwrite(h, 1, 4, s.wav);
    h[0] = (unsigned char)(s.wavData & 0xff); h[1] = (unsigned char)((s.wavData >> 8) & 0xff);
    h[2] = (unsigned char)((s.wavData >> 16) & 0xff); h[3] = (unsigned char)((s.wavData >> 24) & 0xff);
    fseek(s.wav, 40, SEEK_SET); fwrite(h, 1, 4, s.wav);
    fseek(s.wav, here, SEEK_SET);
    fflush(s.wav);
}

inline bool WriteWavHeader(State &s) {
    static const unsigned char hdr[44] = {
        'R','I','F','F', 0,0,0,0, 'W','A','V','E', 'f','m','t',' ',
        16,0,0,0, 1,0, 0,0, 0,0,0,0, 0,0,0,0, 0,0, 16,0,
        'd','a','t','a', 0,0,0,0 };
    if (fwrite(hdr, 1, 44, s.wav) != 44)
        return false;
    unsigned char fix[8];
    fix[0] = (unsigned char)(s.channels & 0xff); fix[1] = (unsigned char)((s.channels >> 8) & 0xff);
    fseek(s.wav, 22, SEEK_SET); fwrite(fix, 1, 2, s.wav);
    fix[0] = (unsigned char)(s.rate & 0xff); fix[1] = (unsigned char)((s.rate >> 8) & 0xff);
    fix[2] = (unsigned char)((s.rate >> 16) & 0xff); fix[3] = (unsigned char)((s.rate >> 24) & 0xff);
    fwrite(fix, 1, 4, s.wav);
    unsigned byteRate = (unsigned)(s.rate * s.channels * 2);
    fix[0] = (unsigned char)(byteRate & 0xff); fix[1] = (unsigned char)((byteRate >> 8) & 0xff);
    fix[2] = (unsigned char)((byteRate >> 16) & 0xff); fix[3] = (unsigned char)((byteRate >> 24) & 0xff);
    fwrite(fix, 1, 4, s.wav);
    unsigned short align = (unsigned short)(s.channels * 2);
    fix[0] = (unsigned char)(align & 0xff); fix[1] = (unsigned char)((align >> 8) & 0xff);
    fwrite(fix, 1, 2, s.wav);
    fseek(s.wav, 44, SEEK_SET);
    return true;
}

// One audio slice per exported frame: rate/25 samples (fractional remainder
// accumulated so long exports do not drift). Tapped at ack time, after the sim
// step for that frame ran, so slice k is the sound of the step frame k shows.
inline void Finalize(const char *reason);
inline void Fail(State &s, const char *why);

inline void TapAudio(State &s) {
    s.tapFrac += (double)s.rate / 25.0;
    int frames = (int)s.tapFrac;
    s.tapFrac -= frames;
    if (frames <= 0)
        return;
    if (s.wavData + (unsigned)(frames * s.channels * 2) > kMaxWavData) {
        Fail(s, "WAV size cap reached");
        Finalize("aborted");
        return;
    }
    short *buf = new short[(size_t)frames * s.channels];
    int got = Audio_MixOffline(buf, frames);
    if (got != frames) {
        // The tap refuses while the device is unpaused. The export hold blocks
        // the focus handler's unpause, so a refusal here means real trouble;
        // re-pause and retry once before giving up.
        Audio_SetPaused(1);
        got = Audio_MixOffline(buf, frames);
    }
    if (got != frames) {
        delete[] buf;
        // Audio is required: a persistent mix failure aborts the whole export.
        Sys_Log("replay-export: offline mix refused (%d of %d frames)", got, frames);
        Fail(s, "audio mix failed");
        Finalize("aborted");
        return;
    }
    size_t wrote = fwrite(buf, 2, (size_t)frames * s.channels, s.wav);
    delete[] buf;
    if (wrote != (size_t)frames * s.channels) {
        Fail(s, "audio write failed");
        Finalize("aborted");
        return;
    }
    s.wavData += (unsigned)(wrote * 2);
    s.taps++;
    PatchWavHeader(s); // keep the WAV valid even if the game exits mid-export
}

inline void Fail(State &s, const char *why) {
    Sys_Log("replay-export: aborting export: %s (partial output kept)", why);
    s.failed = true;
}

inline void Finalize(const char *reason) {
    State &s = S();
    if (!s.started || s.done)
        return;
    s.done = true;
    if (s.wav) {
        PatchWavHeader(s);
        fclose(s.wav);
        s.wav = NULL;
    }
    // Release the mixer and put audio back the way it was before the export.
    Audio_SetExportHold(0);
    Audio_SetPaused(s.wasPaused);
    double secs = s.frames / 25.0;
    Sys_Log("replay-export: %s - %ld frames (%.1f s), %ld audio slices into %s",
            reason, s.frames, secs, s.taps, s.dir);
    if (s.failed || s.frames < 1) {
        if (!s.failed)
            Sys_Log("replay-export: nothing captured, no video assembled");
        return;
    }
    char cmdPath[1200], mp4[1200], pattern[1200], wavPath[1200];
    snprintf(pattern, sizeof pattern, "%s/frame-%%05d.png", s.dir);
    snprintf(wavPath, sizeof wavPath, "%s/audio.wav", s.dir);
    snprintf(mp4, sizeof mp4, "%s/replay.mp4", s.dir);
    const char *argv[] = { "ffmpeg", "-y", "-loglevel", "error",
        "-framerate", "25", "-i", pattern,
        "-i", wavPath,
        "-c:v", "libx264", "-pix_fmt", "yuv420p", "-c:a", "aac", "-shortest", mp4, NULL };
    snprintf(cmdPath, sizeof cmdPath, "%s/export-command.txt", s.dir);
    FILE *cmd = fopen(cmdPath, "w");
    if (cmd) {
        fprintf(cmd, "ffmpeg -y -loglevel error -framerate 25 -i \"%s\" -i \"%s\" -c:v libx264 -pix_fmt yuv420p -c:a aac -shortest \"%s\"\n",
                pattern, wavPath, mp4);
        fclose(cmd);
    }
    // Encode now, synchronously: the replay is over and an export was asked
    // for, so the game waits. Done means done: exit code 0 and a nonempty file.
    Sys_Log("replay-export: encoding %ld frames with ffmpeg; the game waits for it", s.frames);
    int rc = Sys_RunAndWait("ffmpeg", argv);
    FILE *out = fopen(mp4, "rb");
    long outSize = 0;
    if (out) { fseek(out, 0, SEEK_END); outSize = ftell(out); fclose(out); }
    if (rc == 0 && outSize > 0)
        Sys_Log("replay-export: video written: %s (%ld bytes, %ld frames, %.1f s)", mp4, outSize, s.frames, secs);
    else
        Sys_Log("replay-export: ffmpeg failed (exit %d, output %ld bytes); frames + WAV kept, run %s to assemble the video",
                rc, outSize, cmdPath);
}

// Called once per rendered race frame (Race_UpdateFrameEffects), like
// AttractCapture::Tick. Drives begin, save acknowledgement, audio taps, and the
// clock releases that step the sim.
inline void Tick() {
    State &s = S();
    if (!Enabled() || s.done)
        return;
    if (!ReplayControls_Active()) {
        if (s.started)
            Finalize("replay ended");
        return;
    }
    if (s.failed)
        return;
    if (!s.started) {
        // Begin: the export needs the mixer (audio is the point), ffmpeg (the
        // video), and a writable folder. All are preflighted before the first
        // frame is captured, so a missing dependency fails early and clearly
        // instead of leaving frames that are not a video.
        if (!Sys_CommandExists("ffmpeg")) {
            Sys_Log("replay-export: ffmpeg not found on PATH; it is required to assemble the video. Install it (e.g. brew install ffmpeg) and run again; nothing captured");
            s.failed = true; s.done = true;
            return;
        }
        s.rate = Audio_GetOutputRate();
        s.channels = Audio_GetOutputChannels();
        if (s.rate <= 0 || s.channels <= 0) {
            Sys_Log("replay-export: audio is not initialized; export aborted (audio is required)");
            s.failed = true; s.done = true;
            return;
        }
#ifdef _WIN32
        _mkdir(s.dir);
#else
        mkdir(s.dir, 0755);
#endif
        char wavPath[1200];
        snprintf(wavPath, sizeof wavPath, "%s/audio.wav", s.dir);
        s.wav = fopen(wavPath, "wb");
        if (!s.wav || !WriteWavHeader(s)) {
            Sys_Log("replay-export: cannot write %s; export aborted", wavPath);
            if (s.wav) { fclose(s.wav); s.wav = NULL; }
            s.failed = true; s.done = true;
            return;
        }
        // Stop live device consumption and take ownership: while the hold is
        // set, the window-focus handler cannot resume the device mid-export.
        s.wasPaused = Audio_IsPaused();
        Audio_SetPaused(1);
        Audio_SetExportHold(1);
        s.started = true;
        TapAudio(s); // slice 0: the start state's sound, aligned with frame 0
        s.serial = Gfx_GetCaptureSerial();
        snprintf(s.framePath, sizeof s.framePath, "%s/frame-%05ld.png", s.dir, s.frames);
        Gfx_RequestCapture(s.framePath);
        s.pending = true;
        s.waitFrames = 0;
        Sys_Log("replay-export: capturing into %s (one frame + one audio slice per sim step)", s.dir);
        return;
    }
    if (s.pending) {
        if (Gfx_GetCaptureSerial() == s.serial) {
            if (++s.waitFrames > kSaveTimeoutFrames) {
                s.pending = false;
                Fail(s, "frame save not acknowledged");
                Finalize("aborted");
            }
            return;
        }
        // Save acknowledged; the serial bumps even when SDL_SavePNG failed, so
        // verify the file actually landed and has content.
        FILE *f = fopen(s.framePath, "rb");
        long size = 0;
        if (f) { fseek(f, 0, SEEK_END); size = ftell(f); fclose(f); }
        if (size <= 0) {
            s.pending = false;
            Fail(s, "frame file missing or empty after save ack");
            Finalize("aborted");
            return;
        }
        s.pending = false;
        s.frames++;
        if (s.frames > 1)
            TapAudio(s); // slice k covers the step that produced frame k
        s.heldMs += kStepMs; // release: the next frame's schedule runs exactly one step
        return;
    }
    // Step ran this frame and was presented: capture it.
    s.serial = Gfx_GetCaptureSerial();
    snprintf(s.framePath, sizeof s.framePath, "%s/frame-%05ld.png", s.dir, s.frames);
    Gfx_RequestCapture(s.framePath);
    s.pending = true;
    s.waitFrames = 0;
}

// The replay can also end by leaving to the main menu (Esc); same hook point as
// AttractCapture::OnMainMenu.
inline void OnMainMenu() {
    State &s = S();
    if (Enabled() && s.started && !s.done)
        Finalize("left replay");
}

}
#endif
