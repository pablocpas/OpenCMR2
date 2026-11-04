// Functional test for --replay-export (game/ReplayExport.h): the capture-handshake
// gate must schedule exactly one 40 ms sim step per acknowledged frame save, one
// PNG per step, one WAV slice (rate/25 samples) per frame. Audio is required:
// persistent mix failure aborts the export. ffmpeg is preflighted at begin and
// the encode outcome is verified (exit 0 + nonempty file). The pre-export audio
// pause state is restored, and the export hold blocks focus-handler unpauses.
#include "ReplayExport.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cmath>
#include <string>
#include <vector>
#include <unistd.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)

// ---- fakes ---------------------------------------------------------------
static std::string g_dir;
static unsigned g_serial = 0;
static std::string g_requested;      // capture path awaiting "present"
static bool g_replayActive = false;
static int g_paused = 0;
static int g_exportHold = 0;
static long g_mixedFrames = 0;
static int g_mixRate = 44100, g_mixCh = 2;
static bool g_mixRefuse = false;
static int g_mixWhileUnpaused = 0;   // contract violation counter
static bool g_haveFfmpeg = true;
static int g_encoderRc = 0;
static bool g_encoderWritesFile = true, g_encoderWritesEmpty = false, g_encoderRan = false;
static std::vector<std::string> g_spawnArgv;
static std::vector<std::string> g_logs;

bool ReplayControls_Active(void) { return g_replayActive; }
extern "C" {
const char *Sys_GetReplayExportDir(void) { return g_dir.c_str(); }
void Sys_Log(const char *fmt, ...) {
    char buf[1200]; va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
    g_logs.push_back(buf);
}
BOOL Sys_CommandExists(const char *name) { return name && std::string(name) == "ffmpeg" && g_haveFfmpeg ? TRUE : FALSE; }
int Sys_RunAndWait(const char *path, const char *const argv[]) {
    g_encoderRan = true;
    g_spawnArgv.clear();
    for (int i = 0; argv[i]; ++i) g_spawnArgv.push_back(argv[i]);
    if (g_encoderWritesFile) {
        std::string mp4 = g_dir + "/replay.mp4";
        FILE *f = fopen(mp4.c_str(), "wb");
        if (!g_encoderWritesEmpty) { const char v[] = "ftyp-fake-mp4"; fwrite(v, 1, sizeof v, f); }
        fclose(f);
    }
    return g_encoderRc;
}
void Gfx_RequestCapture(const char *p) { g_requested = p ? p : ""; }
unsigned Gfx_GetCaptureSerial(void) { return g_serial; }
int Audio_GetOutputRate(void) { return g_mixRate; }
int Audio_GetOutputChannels(void) { return g_mixCh; }
BOOL Audio_IsPaused(void) { return g_paused ? TRUE : FALSE; }
void Audio_SetExportHold(BOOL h) { g_exportHold = h != 0; }
void Audio_SetPaused(BOOL p) {
    // Mirror the real contract: while the export holds the mixer, unpause is ignored.
    if (!p && g_exportHold) return;
    g_paused = p != 0;
}
int Audio_MixOffline(short *dst, int frames) {
    if (!g_paused) { g_mixWhileUnpaused++; return 0; } // the real refusal contract
    if (g_mixRefuse) return 0;
    for (int i = 0; i < frames * g_mixCh; ++i)
        dst[i] = (short)((g_mixedFrames * g_mixCh + i) % 30000);
    g_mixedFrames += frames;
    return frames;
}
}
static bool g_silentRenderer = false;   // no save, no ack: the save never completes
static bool g_dropSave = false;         // ack without a file: SDL_SavePNG failed
static void Reset() {
    ReplayExport::S() = ReplayExport::State{};
    g_serial = 0; g_requested.clear(); g_replayActive = false;
    g_paused = 0; g_exportHold = 0; g_mixedFrames = 0; g_mixRefuse = false; g_mixWhileUnpaused = 0;
    g_haveFfmpeg = true; g_encoderRc = 0; g_encoderWritesFile = true; g_encoderWritesEmpty = false;
    g_encoderRan = false; g_spawnArgv.clear(); g_logs.clear();
    g_silentRenderer = false; g_dropSave = false;
    char tmpl[] = "/tmp/replexpXXXXXX";
    if (!mkdtemp(tmpl)) { fprintf(stderr, "mkdtemp failed\n"); exit(1); }
    g_dir = tmpl;
}
// The renderer "presents": saves the requested frame (unless told not to) and acks.
static void Present() {
    if (g_requested.empty() || g_silentRenderer) return;
    if (!g_dropSave) {
        FILE *f = fopen(g_requested.c_str(), "wb");
        const char png[] = "\x89PNG-fake"; fwrite(png, 1, sizeof png, f);
        fclose(f);
    }
    g_requested.clear();
    g_serial++;
}
static void RunFrames(int n, int &realMs, int &epochMs, int &prevMs, long &stepsTotal, int &maxSteps) {
    for (int i = 0; i < n; ++i) {
        realMs += 33; // slow-ish ~30 fps wall clock
        int frameMs = realMs;
        ReplayExport::OverrideClock(&frameMs);
        // The accumulator math from Car.cpp (double path): scheduled steps this frame.
        double fromStart = (double)(prevMs - epochMs) * 0.001 * 25.0;
        double toEnd = (double)(frameMs - epochMs) * 0.001 * 25.0;
        int steps = (int)toEnd - (int)fromStart;
        if (steps > maxSteps) maxSteps = steps;
        stepsTotal += steps;
        prevMs = frameMs;
        ReplayExport::Tick();
        Present();
    }
}
static long WavBytes(std::string &out) {
    FILE *f = fopen((g_dir + "/audio.wav").c_str(), "rb");
    CHECK(f != NULL);
    fseek(f, 0, SEEK_END); long size = ftell(f); fseek(f, 0, SEEK_SET);
    out.resize(size); size_t r = fread(&out[0], 1, size, f); (void)r; fclose(f);
    return size;
}
static unsigned U32(const std::string &s, size_t at) {
    return (unsigned char)s[at] | ((unsigned char)s[at+1] << 8) | ((unsigned char)s[at+2] << 16) | ((unsigned char)s[at+3] << 24);
}
static bool LogHas(const char *needle) {
    for (const auto &l : g_logs) if (l.find(needle) != std::string::npos) return true;
    return false;
}
int main() {
    // 1. Full export: 8 sim steps (9 frames incl. start state), then replay ends.
    Reset(); g_replayActive = true;
    int realMs = 100000, prevMs = 100000, epochMs = 100000, maxSteps = 0;
    long stepsTotal = 0;
    int ticks = 0;
    while (ReplayExport::S().frames < 9 && ticks < 200) { RunFrames(1, realMs, epochMs, prevMs, stepsTotal, maxSteps); ++ticks; }
    CHECK(ReplayExport::S().frames == 9);
    CHECK(maxSteps <= 1);                       // never a catchup burst
    CHECK(stepsTotal == 8);                     // exactly one step per frame past the first
    CHECK(ReplayExport::S().taps == 9);         // one audio slice per frame
    CHECK(g_mixedFrames == 9 * 1764);           // 44100/25 = 1764 samples per slice
    CHECK(g_exportHold == 1);                   // mixer owned during export
    g_replayActive = false;
    RunFrames(1, realMs, epochMs, prevMs, stepsTotal, maxSteps); // this Tick finalizes
    CHECK(ReplayExport::S().done);
    CHECK(g_exportHold == 0);                   // hold released
    CHECK(g_paused == 0);                       // prior (unpaused) state restored
    CHECK(g_encoderRan);                        // encode ran and its outcome was checked
    CHECK(LogHas("video written"));
    std::string wav;
    long wavSize = WavBytes(wav);
    CHECK(wavSize == 44 + 9 * 1764 * 2 * 2);
    CHECK(memcmp(wav.data(), "RIFF", 4) == 0 && memcmp(wav.data() + 8, "WAVE", 4) == 0);
    CHECK(U32(wav, 4) == 36 + 9 * 1764 * 2 * 2);
    CHECK(U32(wav, 40) == 9 * 1764 * 2 * 2);
    CHECK(U32(wav, 24) == 44100 && wav[22] == 2 && wav[23] == 0);
    short s0 = (short)(unsigned char)wav[44] | (short)(wav[45] << 8);
    CHECK(s0 == 0);
    CHECK(g_spawnArgv.size() >= 2 && g_spawnArgv[0] == "ffmpeg" && g_spawnArgv[1] == "-y");
    bool hasFramerate = false, hasAac = false, hasMp4 = false;
    for (const auto &a : g_spawnArgv) {
        hasFramerate = hasFramerate || a == "25";
        hasAac = hasAac || a == "aac";
        hasMp4 = hasMp4 || a.find("replay.mp4") != std::string::npos;
    }
    CHECK(hasFramerate && hasAac && hasMp4);
    FILE *cmd = fopen((g_dir + "/export-command.txt").c_str(), "r");
    CHECK(cmd != NULL); if (cmd) fclose(cmd);
    for (int i = 0; i < 9; ++i) { char p[1200]; snprintf(p, sizeof p, "%s/frame-%05d.png", g_dir.c_str(), i); CHECK(access(p, F_OK) == 0); }

    // 2. Audio already paused at begin (window in background): restored to paused, not unpaused.
    Reset(); g_replayActive = true; g_paused = 1;
    realMs = 150000; prevMs = 150000; epochMs = 150000; maxSteps = 0; stepsTotal = 0;
    ticks = 0;
    while (ReplayExport::S().frames < 3 && ticks < 200) { RunFrames(1, realMs, epochMs, prevMs, stepsTotal, maxSteps); ++ticks; }
    g_replayActive = false;
    RunFrames(1, realMs, epochMs, prevMs, stepsTotal, maxSteps);
    CHECK(ReplayExport::S().done);
    CHECK(g_paused == 1);                       // prior paused state restored
    CHECK(g_exportHold == 0);
    CHECK(LogHas("video written"));

    // 3. Save never acknowledged: abort after the timeout, WAV stays valid, prior state restored, no encode.
    Reset(); g_replayActive = true;
    realMs = 200000; prevMs = 200000; epochMs = 200000; maxSteps = 0; stepsTotal = 0;
    RunFrames(1, realMs, epochMs, prevMs, stepsTotal, maxSteps); // begin + request f0
    g_silentRenderer = true;
    RunFrames(ReplayExport::kSaveTimeoutFrames + 5, realMs, epochMs, prevMs, stepsTotal, maxSteps);
    CHECK(ReplayExport::S().failed && ReplayExport::S().done);
    CHECK(g_paused == 0 && g_exportHold == 0);
    CHECK(!g_encoderRan);
    long s2 = WavBytes(wav);
    CHECK(s2 == 44 + 1764 * 2 * 2); // only slice 0
    CHECK(U32(wav, 40) == 1764 * 2 * 2);

    // 4. Ack arrives but the frame file is missing: abort, partial output kept, no encode.
    Reset(); g_replayActive = true;
    realMs = 300000; prevMs = 300000; epochMs = 300000; maxSteps = 0; stepsTotal = 0;
    RunFrames(1, realMs, epochMs, prevMs, stepsTotal, maxSteps);
    g_dropSave = true;
    RunFrames(3, realMs, epochMs, prevMs, stepsTotal, maxSteps); // f0 acks, f1 requested, f1 ack observed without a file
    g_dropSave = false;
    CHECK(ReplayExport::S().failed && ReplayExport::S().done);
    CHECK(g_paused == 0 && g_exportHold == 0);
    CHECK(!g_encoderRan);

    // 5. Focus change mid-export: the hold blocks the unpause; every mix runs paused.
    Reset(); g_replayActive = true;
    realMs = 400000; prevMs = 400000; epochMs = 400000; maxSteps = 0; stepsTotal = 0;
    RunFrames(1, realMs, epochMs, prevMs, stepsTotal, maxSteps); // begin
    Audio_SetPaused(0);                          // the window-focus handler fires
    CHECK(g_paused == 1);                        // blocked by the hold
    ticks = 0;
    while (ReplayExport::S().frames < 4 && ticks < 200) { RunFrames(1, realMs, epochMs, prevMs, stepsTotal, maxSteps); ++ticks; }
    CHECK(ReplayExport::S().frames == 4);
    CHECK(g_mixWhileUnpaused == 0);              // no sample was ever mixed live
    g_replayActive = false;
    RunFrames(1, realMs, epochMs, prevMs, stepsTotal, maxSteps);
    CHECK(ReplayExport::S().done && LogHas("video written"));

    // 6. Persistent mix refusal: the whole export aborts (audio is required), no encode.
    Reset(); g_replayActive = true;
    realMs = 500000; prevMs = 500000; epochMs = 500000; maxSteps = 0; stepsTotal = 0;
    RunFrames(1, realMs, epochMs, prevMs, stepsTotal, maxSteps); // begin (tap 0 ok)
    g_mixRefuse = true;
    RunFrames(4, realMs, epochMs, prevMs, stepsTotal, maxSteps);
    CHECK(ReplayExport::S().failed && ReplayExport::S().done);
    CHECK(LogHas("audio mix failed") && LogHas("aborted"));
    CHECK(!g_encoderRan);
    CHECK(g_paused == 0 && g_exportHold == 0);

    // 7. ffmpeg missing: fail early at begin, nothing captured, clear dependency message.
    Reset(); g_replayActive = true; g_haveFfmpeg = false;
    realMs = 600000; prevMs = 600000; epochMs = 600000; maxSteps = 0; stepsTotal = 0;
    RunFrames(3, realMs, epochMs, prevMs, stepsTotal, maxSteps);
    CHECK(ReplayExport::S().failed && ReplayExport::S().done);
    CHECK(!ReplayExport::S().started);
    CHECK(ReplayExport::S().frames == 0);
    CHECK(LogHas("ffmpeg not found on PATH"));
    CHECK(!g_encoderRan);

    // 8. Encoder fails (exit 1): reported as failure, command file kept, no "video written".
    Reset(); g_replayActive = true; g_encoderRc = 1;
    realMs = 700000; prevMs = 700000; epochMs = 700000; maxSteps = 0; stepsTotal = 0;
    ticks = 0;
    while (ReplayExport::S().frames < 3 && ticks < 200) { RunFrames(1, realMs, epochMs, prevMs, stepsTotal, maxSteps); ++ticks; }
    g_replayActive = false;
    RunFrames(1, realMs, epochMs, prevMs, stepsTotal, maxSteps);
    CHECK(ReplayExport::S().done);
    CHECK(g_encoderRan);
    CHECK(LogHas("ffmpeg failed") && !LogHas("video written"));
    cmd = fopen((g_dir + "/export-command.txt").c_str(), "r");
    CHECK(cmd != NULL); if (cmd) fclose(cmd);

    // 9. Encoder exits 0 but the mp4 is empty: still a failure.
    Reset(); g_replayActive = true; g_encoderWritesEmpty = true;
    realMs = 800000; prevMs = 800000; epochMs = 800000; maxSteps = 0; stepsTotal = 0;
    ticks = 0;
    while (ReplayExport::S().frames < 3 && ticks < 200) { RunFrames(1, realMs, epochMs, prevMs, stepsTotal, maxSteps); ++ticks; }
    g_replayActive = false;
    RunFrames(1, realMs, epochMs, prevMs, stepsTotal, maxSteps);
    CHECK(ReplayExport::S().done);
    CHECK(LogHas("ffmpeg failed") && !LogHas("video written"));

    // 10. No audio at begin: export refuses to start (audio is required), no gating left behind.
    Reset(); g_replayActive = true; g_mixRate = 0;
    realMs = 900000; prevMs = 900000; epochMs = 900000; maxSteps = 0; stepsTotal = 0;
    RunFrames(3, realMs, epochMs, prevMs, stepsTotal, maxSteps);
    CHECK(ReplayExport::S().failed && ReplayExport::S().done);
    CHECK(!ReplayExport::S().started);
    g_mixRate = 44100;

    printf("replay_export test ok\n");
    return 0;
}
