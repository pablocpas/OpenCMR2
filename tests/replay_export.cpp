// Functional test for --replay-export (game/ReplayExport.h): the capture-handshake
// gate must schedule exactly one 40 ms sim step per acknowledged frame save, one
// PNG per step, one WAV slice (rate/25 samples) per frame, and the export must
// finalize cleanly on replay end, save timeout, and missing-frame failure.
#include "ReplayExport.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
static long g_mixedFrames = 0;       // total output frames the offline tap produced
static int g_mixRate = 44100, g_mixCh = 2;
static bool g_mixRefuse = false;
static std::vector<std::string> g_spawnArgv;
static int g_pauseCalls = 0;

bool ReplayControls_Active(void) { return g_replayActive; }
extern "C" {
const char *Sys_GetReplayExportDir(void) { return g_dir.c_str(); }
void Sys_Log(const char *, ...) {}
BOOL Sys_RunDetached(const char *path, const char *const argv[]) {
    g_spawnArgv.clear();
    for (int i = 0; argv[i]; ++i) g_spawnArgv.push_back(argv[i]);
    return path && argv ? TRUE : FALSE;
}
void Gfx_RequestCapture(const char *p) { g_requested = p ? p : ""; }
unsigned Gfx_GetCaptureSerial(void) { return g_serial; }
int Audio_GetOutputRate(void) { return g_mixRate; }
int Audio_GetOutputChannels(void) { return g_mixCh; }
void Audio_SetPaused(BOOL p) { g_paused = p != 0; g_pauseCalls++; }
int Audio_MixOffline(short *dst, int frames) {
    if (g_mixRefuse || !g_paused) return 0; // mirrors the real refusal contract
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
    g_paused = 0; g_mixedFrames = 0; g_mixRefuse = false; g_spawnArgv.clear(); g_pauseCalls = 0;
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
        int frameMs = ReplayExport::S().gateInit || ReplayExport::Enabled() ? realMs : realMs;
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
    g_replayActive = false;
    RunFrames(1, realMs, epochMs, prevMs, stepsTotal, maxSteps); // this Tick finalizes
    CHECK(ReplayExport::S().done);
    CHECK(g_paused == 0);                       // live audio restored
    std::string wav;
    long wavSize = WavBytes(wav);
    CHECK(wavSize == 44 + 9 * 1764 * 2 * 2);
    CHECK(memcmp(wav.data(), "RIFF", 4) == 0 && memcmp(wav.data() + 8, "WAVE", 4) == 0);
    CHECK(U32(wav, 4) == 36 + 9 * 1764 * 2 * 2);   // patched RIFF size
    CHECK(U32(wav, 40) == 9 * 1764 * 2 * 2);       // patched data size
    CHECK(U32(wav, 24) == 44100 && wav[22] == 2 && wav[23] == 0);
    // first PCM sample of slice 1 starts right after slice 0
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

    // 2. Save never acknowledged: abort after the timeout, WAV stays valid, audio restored.
    Reset(); g_replayActive = true;
    realMs = 200000; prevMs = 200000; epochMs = 200000; maxSteps = 0; stepsTotal = 0;
    RunFrames(1, realMs, epochMs, prevMs, stepsTotal, maxSteps); // begin + request f0
    g_silentRenderer = true; // the save never completes
    RunFrames(ReplayExport::kSaveTimeoutFrames + 5, realMs, epochMs, prevMs, stepsTotal, maxSteps);
    CHECK(ReplayExport::S().failed && ReplayExport::S().done);
    CHECK(g_paused == 0);
    long s2 = WavBytes(wav);
    CHECK(s2 == 44 + 1764 * 2 * 2); // only slice 0
    CHECK(U32(wav, 40) == 1764 * 2 * 2);

    // 3. Ack arrives but the frame file is missing: abort, partial output kept.
    Reset(); g_replayActive = true;
    realMs = 300000; prevMs = 300000; epochMs = 300000; maxSteps = 0; stepsTotal = 0;
    RunFrames(1, realMs, epochMs, prevMs, stepsTotal, maxSteps);
    g_dropSave = true;
    RunFrames(3, realMs, epochMs, prevMs, stepsTotal, maxSteps); // f0 acks, f1 requested, f1 ack observed without a file
    g_dropSave = false;
    CHECK(ReplayExport::S().failed && ReplayExport::S().done);
    CHECK(g_paused == 0);

    // 4. Offline mix refuses (audio lost): video continues, audio capped, still finalizes.
    Reset(); g_replayActive = true;
    realMs = 400000; prevMs = 400000; epochMs = 400000; maxSteps = 0; stepsTotal = 0;
    RunFrames(1, realMs, epochMs, prevMs, stepsTotal, maxSteps);
    g_mixRefuse = true; g_paused = 0; // focus change unpaused the device
    ticks = 0;
    while (ReplayExport::S().frames < 4 && ticks < 200) { RunFrames(1, realMs, epochMs, prevMs, stepsTotal, maxSteps); ++ticks; }
    CHECK(ReplayExport::S().frames == 4);        // frames kept coming
    CHECK(ReplayExport::S().audioCapped);
    CHECK(g_paused == 1);                        // the tap re-paused the device
    g_replayActive = false;
    RunFrames(1, realMs, epochMs, prevMs, stepsTotal, maxSteps);
    CHECK(ReplayExport::S().done && !g_spawnArgv.empty());

    // 5. No audio at begin: export refuses to start (audio is required), no gating left behind.
    Reset(); g_replayActive = true; g_mixRate = 0;
    realMs = 500000; prevMs = 500000; epochMs = 500000; maxSteps = 0; stepsTotal = 0;
    RunFrames(3, realMs, epochMs, prevMs, stepsTotal, maxSteps);
    CHECK(ReplayExport::S().failed && ReplayExport::S().done);
    CHECK(!ReplayExport::S().started);
    g_mixRate = 44100;

    printf("replay_export test ok\n");
    return 0;
}
