// Run the real menu/load/update path with a controlled clock and tick inputs.
// Original assets are required; run through tools/stage_baseline.py to isolate saves.
#include "port/types.h"
#include "platform/platform.h"
#include "platform/testing.h"
#include "port/diagnostics.h"
#include "diagnostics/timing.h"
#include "diagnostics/state_capture.h"
#include "port/disk_layouts.h"
#include "port/sys.h"
#include "port/input.h"
#include "main.h"
#include "Game.h"
#include "Frontend.h"
#include "GameInfo.h"
#include "FrontendMenus.h"
#include "Menu.h"
#include "Car.h"
#include "CarParts.h"
#include "ReplayStream.h"
#include "CarPhysics.h"
#include "NetPlayers.h"
#include "RallyData.h"
#include "SavedGame.h"
#include "Sector.h"
#include "Mesh.h"
#include "MeshVertex.h"
#include "SceneNode.h"
#include "LightZone.h"
#include "RallyRouteNode.h"
#include "FarLayer.h"
#include "port/gfx.h"
#include <cmath>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL_main.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

extern void Main_InitD3DX();
extern void Input_ClearKeyPressQueue();
extern int Args_Parse(char *);
extern Menu *FrontendMenu_GetActiveMenu();
extern Menu *g_pMenu0x00831778;
extern void RallyData_SetEditedDriverOrCategoryName(unsigned int, char *);
extern "C" unsigned int Crt_GetRandState();
extern CarPartSet *g_carPartSets;
extern CarDamageRecord *g_carDamageRecords;
extern int StageTiming_GetCheckpointGroupIndex(int);
extern int StageTiming_GetCheckpointSplitIndex(int);
extern BYTE StageTiming_GetCheckpointField19(int);
extern int StageTiming_GetCarSplitTime(int, int);
extern CarContact *g_carContacts;
// Match the production types: the MSVC ABI includes them in symbol names.
struct Unk0x00542e78;
struct CarStageTiming;
extern Unk0x00542e78 g_playerCheckpointRecords[8];
extern CarStageTiming g_carStageTiming[8];
extern int g_stageClock;
extern char g_sessionName[20];
extern ReplayStream *Race_GetPlayerRecordPointer(int);
extern int Replay_StopRecording(BYTE *);
extern void Race_StartPlayerAndOpponentReplays(int);
extern void StageTiming_RecordDriverTimeoutFinish(int);
extern BYTE *StageUI_GetRaceResultTable();
extern Menu *g_pHeaderMenu;
extern int NetRace_IsPlayerFadeActive(BYTE);
extern void GameInfo_SetConfiguredCheat(int, int);
extern void GameInfo_ApplyCheatsForGameMode();
extern char *Race_GetStagePathBuffer38340();
extern void SavedGames_LoadRecords();
extern int SavedGames_GetCount();
extern char g_winnerFormat[];
extern char g_ghostName[4];

static DWORD virtualTime = 1000;
static double virtualFraction; // of the current millisecond, above 1000 FPS
static DWORD Ticks() { return virtualTime; }
static double TicksPrecise() { return virtualTime + virtualFraction; }
static void Sleep(DWORD ms) { virtualTime += ms; }

struct Probe : Diagnostics::TimingObserver {
    struct Controls {
        unsigned tick, state;
        int digital[4], handbrake, steering, pedal, shift[4];
    } input{};
    std::vector<Controls> playback;
    FILE *trace;
    FILE *memory = nullptr;
    unsigned tick = 0, drivingTick = 0, target;
    unsigned frameTickStart = 0, frameRng = 0, renderOnlyRngChanges = 0;
    bool scenarioCorrect = true;
    bool playbackCorrect = true;
    bool finishInjected = false;
    bool leftRace = false;      // a state other than driving after the finish
    unsigned nextStageDrivingTicks = 0;
    BYTE state = 0;
    explicit Probe(FILE *file, unsigned count) : trace(file), target(count) {
        if (const char *path = getenv("OPENCMR2_STATE_CAPTURE")) {
            memory = fopen(path, "wb");
            if (!memory) { perror(path); exit(2); }
            CaptureHeader(memory);
        }
    }
    ~Probe() {
        if (memory) { CaptureFinish(memory, tick); fclose(memory); }
    }
    bool LoadInputs(const char *path) {
        std::ifstream file(path);
        std::string line;
        if (!std::getline(file, line) || line != "tick,state,left,right,throttle,brake,handbrake,steering,pedal,shift0,shift1,shift2,shift3") return false;
        while (std::getline(file, line)) {
            for (char &ch : line) if (ch == ',') ch = ' ';
            std::istringstream row(line);
            Controls controls{};
            if (!(row >> controls.tick >> controls.state)) return false;
            for (int &v : controls.digital) if (!(row >> v) || v < 0 || v > 63) return false;
            if (!(row >> controls.handbrake >> controls.steering >> controls.pedal) || controls.handbrake < 0 || controls.handbrake > 1) return false;
            for (int &v : controls.shift) if (!(row >> v) || v < 0 || v > 255) return false;
            std::string extra;
            if (row >> extra || controls.tick != playback.size() || controls.state > 255 || playback.size() >= target) return false;
            playback.push_back(controls);
        }
        return file.eof() && playback.size() == target;
    }
    void BeginFrame() override {
        TimingObserver::BeginFrame();
        frameTickStart = tick;
        frameRng = Crt_GetRandState();
    }
    void EndFrame() override {
        TimingObserver::EndFrame();
        if (CGame::GetFrontendResourceMode() == 3 && tick == frameTickStart && Crt_GetRandState() != frameRng)
            ++renderOnlyRngChanges;
    }
    void BeginTick(unsigned char value) override {
        TimingObserver::BeginTick(value);
        state = value;
        // OPENCMR2_PROBE_RESTART: the race after the finish is the same stage
        // again (Restart in the results menu), not Finland stage 2.
        if (finishInjected && value != 8)
            leftRace = true;
        if (finishInjected && value == 8 &&
            (getenv("OPENCMR2_PROBE_RESTART") ? leftRace : RallyDataCountryIndex() == 0 && RallyDataStageIndex() == 1)) {
            if (!nextStageDrivingTicks) fprintf(stderr, "probe: Finland stage 2 driving at tick %u\n", tick);
            ++nextStageDrivingTicks;
        }
        if (tick == 0) {
            fprintf(stderr, "probe: country=%u stage=%u mode=%u cars=%d model=%d\n", RallyDataCountryIndex(), RallyDataStageIndex(), CGameInfo::GetConfiguredGameMode(), g_carOrderCount, Car_Get(0)->type);
            const char *expectedCars = getenv("OPENCMR2_PROBE_EXPECT_CARS");
            const char *expectedMode = getenv("OPENCMR2_PROBE_EXPECT_MODE");
            const char *expectedCountry = getenv("OPENCMR2_PROBE_EXPECT_COUNTRY");
            const char *expectedStage = getenv("OPENCMR2_PROBE_EXPECT_STAGE");
            scenarioCorrect = RallyDataCountryIndex() == (expectedCountry ? atoi(expectedCountry) : 0) &&
                RallyDataStageIndex() == (expectedStage ? atoi(expectedStage) : 0) &&
                g_carOrderCount == (expectedCars ? atoi(expectedCars) : 1) &&
                (!expectedMode || CGameInfo::GetConfiguredGameMode() == atoi(expectedMode));
            if (const char *expected = getenv("OPENCMR2_PROBE_EXPECT_ARCADE_TRACK")) {
                const unsigned track = RallyData_GetSelectionBits10To11() * 3 + RallyData_GetSelectionBits12To13();
                char archive[16];
                snprintf(archive, sizeof(archive), "\\cha%02d%s", atoi(expected) + 1,
                         CGameInfo::GetGraphicsOptionBits27To28() == 0 ? "hi" : "lo");
                const char *path = Race_GetStagePathBuffer38340();
                fprintf(stderr, "probe: arcade track=%u archive=%s\n", track, path);
                scenarioCorrect = scenarioCorrect && track == unsigned(atoi(expected)) && strstr(path, archive);
            }
        }
    }
    void CaptureMemory() {
        if (tick >= target) return;
        if (memory) {
            StateCaptureView view{};
            // The capture holds the original (Win32) layout: where Car and
            // CarPartSet hold wider pointers, pack the captured ones first.
            static unsigned char cars[8][0xc24], parts[8][0x4d0];
            const void *worldMatrices[8]{}, *bodyMatrices[8]{};
            for (int i = 0; i < g_carOrderCount; ++i) {
                const int id = g_carOrder[i];
                if (id >= 0 && id < 8) {
                    worldMatrices[id] = g_carBuffer[id].pWorld;
                    bodyMatrices[id] = g_carBuffer[id].pBodyMatrix;
                }
            }
            view.worldMatrices = worldMatrices;
            view.bodyMatrices = bodyMatrices;
            const unsigned char *pCars = reinterpret_cast<const unsigned char *>(g_carBuffer);
            const unsigned char *pParts = reinterpret_cast<const unsigned char *>(g_carPartSets);
            if (sizeof(Car) != sizeof(cars[0]) || sizeof(CarPartSet) != sizeof(parts[0])) {
                for (int i = 0; i < g_carOrderCount; i++) {
                    int id = g_carOrder[i];
                    if (id < 0 || id >= 8) continue;
                    Disk_Pack(cars[id], &g_carBuffer[id], &g_diskLayout_Car);
                    if (g_carPartSets) Disk_Pack(parts[id], &g_carPartSets[id], &g_diskLayout_CarPartSet);
                }
                pCars = cars[0];
                pParts = g_carPartSets ? parts[0] : NULL;
            }
            view.cars = pCars;
            view.parts = pParts; view.damage = reinterpret_cast<const unsigned char *>(g_carDamageRecords);
            view.contacts = reinterpret_cast<const unsigned char *>(g_carContacts);
            view.timing = reinterpret_cast<const unsigned char *>(g_carStageTiming);
            view.checkpoints = reinterpret_cast<const unsigned char *>(g_playerCheckpointRecords);
            view.transforms = reinterpret_cast<const unsigned char *>(g_carTransforms);
            view.shadow = reinterpret_cast<const unsigned char *>(g_carTransformsShadow);
            view.wheels = reinterpret_cast<const unsigned char *>(g_carWheelTransforms);
            view.order = g_carOrder; view.count = g_carOrderCount;
            view.physicsStep = g_physicsTimeStep; view.physicsScale = g_physicsScale;
            view.stageClock = g_stageClock;
            StateCaptureInput controls{};
            controls.tick = input.tick; controls.state = input.state;
            memcpy(controls.digital, input.digital, sizeof(controls.digital));
            memcpy(controls.shift, input.shift, sizeof(controls.shift));
            controls.handbrake = input.handbrake; controls.steering = input.steering; controls.pedal = input.pedal;
            CaptureTick(memory, tick, state, Crt_GetRandState(), CMain::GetFrameTime(), drivingTick, controls, view);
        }
    }
    void ControlsReady() override {
        if (tick >= target) return;
        // Resolved digital controls, after the game's device mapping. No input
        // timing is derived from the number of drawn frames.
        Car &c = *Car_Get(0);
        if (!playback.empty()) {
            const Controls &controls = playback[tick];
            playbackCorrect = playbackCorrect && state == controls.state;
            for (int i = 0; i < 4; ++i) {
                c.flag0x1d0[i] = char(controls.digital[i]);
                c.field_0x1d4[i] = BYTE(controls.shift[i]);
            }
            c.handbrake = controls.handbrake;
            c.field_0x1dc = controls.steering;
            c.field_0x1e0 = controls.pedal;
        } else if (state == 8 && getenv("OPENCMR2_PROBE_IDLE")) {
            memset(c.flag0x1d0, 0, sizeof(c.flag0x1d0));
            memset(c.field_0x1d4, 0, sizeof(c.field_0x1d4));
            c.handbrake = 1;
            c.field_0x1dc = c.field_0x1e0 = 0;
        } else if (state == 8) {
            c.flag0x1d0[0] = drivingTick >= 100 && drivingTick < 125 ? 32 : 0;
            c.flag0x1d0[1] = drivingTick >= 150 && drivingTick < 175 ? 32 : 0;
            c.flag0x1d0[2] = drivingTick < 225 || drivingTick >= 275 ? 63 : 0;
            c.flag0x1d0[3] = drivingTick >= 225 && drivingTick < 275 ? 63 : 0;
            c.handbrake = drivingTick >= 300 && drivingTick < 310;
            c.field_0x1dc = c.field_0x1e0 = 0;
            memset(c.field_0x1d4, 0, sizeof(c.field_0x1d4));
        }
        input.tick = tick; input.state = state;
        for (int i = 0; i < 4; ++i) {
            input.digital[i] = c.flag0x1d0[i];
            input.shift[i] = c.field_0x1d4[i];
        }
        input.handbrake = c.handbrake; input.steering = c.field_0x1dc; input.pedal = c.field_0x1e0;
        if (state == 8) ++drivingTick;
    }
    void Vector(const char *name, const FixVector &v) {
        fprintf(trace, ",\"%s\":[%d,%d,%d]", name, v.x, v.y, v.z);
    }
    template<class T, size_t N> void Array(const char *name, const T (&v)[N]) {
        fprintf(trace, ",\"%s\":[", name);
        for (size_t i = 0; i < N; ++i) fprintf(trace, "%s%d", i ? "," : "", int(v[i]));
        fputc(']', trace);
    }
    void EndTick() override {
        TimingObserver::EndTick();
        if (tick >= target) return;
        if (const char *replayAt = getenv("OPENCMR2_PROBE_REPLAY_AT_TICK")) {
            if (tick + 1 == unsigned(atoi(replayAt))) {
                for (int car = 0; car < g_carOrderCount; ++car)
                    Replay_StopRecording(reinterpret_cast<BYTE *>(Race_GetPlayerRecordPointer(car)));
                Race_StartPlayerAndOpponentReplays(0);
                fprintf(stderr, "probe: recorded replay started at tick %u\n", tick + 1);
            }
            auto *stream = Race_GetPlayerRecordPointer(0);
            if (stream && tick % 25 == 0)
                fprintf(stderr, "probe: replay tick=%u playing=%d lane=%d frame=%d length=%d\n",
                    tick, stream->playing, stream->lane, stream->frame,
                    stream->lane < stream->laneCount ? stream->pLaneSamples[stream->lane] : 0);
        }
        if (const char *finishAt = getenv("OPENCMR2_PROBE_FINISH_AT_TICK")) {
            if (!finishInjected && tick + 1 == unsigned(atoi(finishAt)) && state == 8) {
                StageTiming_RecordDriverTimeoutFinish(0);
                CGame::PromoteCallbackEntryByRule((CallbackStateMachine *)StageUI_GetRaceResultTable(), 0, 0, 2);
                finishInjected = true;
                fprintf(stderr, "probe: finish transition requested at tick %u\n", tick + 1);
            }
        }
        CaptureMemory();
        fprintf(trace, "{\"tick\":%u,\"state\":%u,\"driving_tick\":%u,\"rng\":%u,\"input\":{\"digital\":[%d,%d,%d,%d],\"handbrake\":%d,\"steering\":%d,\"pedal\":%d,\"shift\":[%d,%d,%d,%d]},\"cars\":[",
                tick, state, drivingTick, Crt_GetRandState(), input.digital[0], input.digital[1], input.digital[2], input.digital[3], input.handbrake, input.steering, input.pedal,
                input.shift[0], input.shift[1], input.shift[2], input.shift[3]);
        for (int i = 0; i < g_carOrderCount; ++i) {
            const Car &c = g_carBuffer[g_carOrder[i]];
            if (getenv("OPENCMR2_PROBE_OPPONENT") && c.index == 1 && tick % 100 == 0) {
                const ReplayStream *p = Race_GetPlayerRecordPointer(1);
                if (p) fprintf(stderr, "opponent tick=%u rate=%g playing=%d started=%d type=%d lane=%d/%d frame=%d/%d repeat=%u capacity=%d\n",
                    tick, c.simulationRateHz, p->playing, p->playStarted, p->type, p->lane, p->laneCount,
                    p->frame, p->lane < p->laneCount ? p->pLaneSamples[p->lane] : -1, p->field_0x21, p->samplesPerLane);
            }
            fprintf(trace, "%s{\"id\":%d,\"type\":%d,\"gear\":%d,\"speed\":%d,\"sector\":%d,\"heading\":%u",
                    i ? "," : "", g_carOrder[i], c.type, c.gear, c.speed, c.sector, c.wheelSteeringAngle);
            Vector("position", c.position); Vector("position_prev", c.positionPrev);
            Vector("velocity", c.velocity); Vector("velocity_next", c.velocityNext);
            Vector("angular_velocity", c.angularVelocity);
            Vector("right", c.right); Vector("up", c.up); Vector("forward", c.forward);
            Vector("ground_normal", c.groundNormal);
            Array("controls", c.flag0x1d0); Array("contacts", c.cornerOnGround);
            Array("contact_triangles", c.cornerTriangle); Array("surface", c.wheelSurfaceType);
            Array("wheel_load", c.wheelLoad); Array("wheel_torque", c.wheelTorque);
            Array("wheel_slip", c.wheelSlip); Array("wheel_slip_lateral", c.wheelSlipLateral);
            Array("suspension", c.wheel0x988); Array("suspension_body", c.wheel0x9a8);
            Array("corner_height", c.cornerHeight); Array("disabled_corners", c.cornerFlags);
            if (g_carPartSets) {
                const CarPartSet &parts = g_carPartSets[int(c.index)];
                Array("damage_front", parts.damageGrid[0]); Array("damage_middle", parts.damageGrid[1]); Array("damage_rear", parts.damageGrid[2]);
                Array("damage_parts", parts.damageValues);
            }
            if (g_carDamageRecords) {
                const CarDamageRecord &damage = g_carDamageRecords[int(c.index)];
                Array("damage_intensity", damage.damage.intensity);
            }
            fprintf(trace, ",\"checkpoint_group\":%d,\"checkpoint_split\":%d,\"finish\":%u,\"split_times\":[",
                    StageTiming_GetCheckpointGroupIndex(c.index), StageTiming_GetCheckpointSplitIndex(c.index), StageTiming_GetCheckpointField19(c.index));
            for (int split = 0; split < 12; ++split) fprintf(trace, "%s%d", split ? "," : "", StageTiming_GetCarSplitTime(c.index, split));
            fputc(']', trace);
            fprintf(trace, ",\"handbrake\":%d,\"steering_filter\":%d,\"steering_torque\":%d,\"brake\":%d,\"engine\":%d,\"shift_delay\":%d,\"auto_gearbox\":%d}",
                    c.handbrake, c.steeringInput, c.steeringTorqueScale, c.brakeInput, c.engineSpeed, c.autoShiftDelay, c.automaticGearbox);
        }
        fputs("]}\n", trace);
        ++tick;
    }
};


RallyRouteNode *RallyData_GetAvailableRouteNodeRecord(int index);
int RallyData_GetRouteAvailabilityState(void);
void ChaseCamera_Reset(BYTE index);
void View_ResetShownFarPlane(BYTE view);
void StageTiming_SetViewRouteDistanceLimit(BYTE player, unsigned int node, int dir);
int Track_GetGroundHeightSurface(FixVector *pPoint, FixVector *pNormal, short *pTri, short *pSurfaceClass,
                                 unsigned short *pSurface, int defaultY);

// OPENCMR2_PROBE_SWEEP=metres: once the race runs, the car is placed along the
// route every that many metres, facing along it and then across it, and held
// there while the far layer (FarLayer.cpp) is checked on frames of the same
// moment (the clock stands still): the frame as the game draws it, one
// without the layer with the sky and backdrop keyed green, and one with the
// layer in marker colours (green, magenta nearer than half the view
// distance), the last two without the stage's fade. Each view writes a line
// to OPENCMR2_PROBE_SWEEP_OUT/sweep.csv: the layer's pixels where the stage
// was (faults: it must stay behind the stage), its near pixels where the sky
// was (the layer close to the car, through a gap or over it), its near and
// far pixels. Views with faults or near pixels in the sky, and every tenth,
// save the three frames.
struct Sweep {
    enum { HOLD = 28 };
    std::vector<unsigned> nodes;    // route nodes of the views
    std::vector<int> headings;      // per view: 0 along the route, 1 or -1 across it
    size_t view = 0;
    int frame = -1;
    std::string out;
    FILE *csv = nullptr;
    std::vector<uint32_t> shown, sky;
    int width = 0, height = 0;
    enum Want { NONE, SHOWN, SKY, MARKED } want = NONE;
    bool done = false;

    void Plan(float spacing) {
        int count = RallyData_GetRouteAvailabilityState();
        float travelled = spacing;
        for (int i = 0; i + 1 < count; i++) {
            const FixVector &a = RallyData_GetAvailableRouteNodeRecord(i)->position;
            const FixVector &b = RallyData_GetAvailableRouteNodeRecord(i + 1)->position;
            float dx = (b.x - a.x) / 65536.0f, dz = (b.z - a.z) / 65536.0f;
            travelled += std::sqrt(dx * dx + dz * dz);
            if (travelled >= spacing) {
                travelled = 0;
                nodes.push_back(unsigned(i));
                nodes.push_back(unsigned(i));
                headings.push_back(0);
                headings.push_back(nodes.size() / 2 % 2 ? 1 : -1);
            }
        }
        fprintf(stderr, "probe: sweep %zu views over %d route nodes\n", nodes.size(), count);
    }

    // Places the car at the view's node, facing along the route (toward a
    // node 25 m on) or across it.
    void Place(Car &car) {
        int count = RallyData_GetRouteAvailabilityState();
        unsigned node = nodes[view];
        const FixVector &at = RallyData_GetAvailableRouteNodeRecord(int(node))->position;
        unsigned ahead = node;
        float dx = 0, dz = 0;
        while (ahead + 1 < unsigned(count) && dx * dx + dz * dz < 25.0f * 25.0f) {
            ++ahead;
            const FixVector &b = RallyData_GetAvailableRouteNodeRecord(int(ahead))->position;
            dx = (b.x - at.x) / 65536.0f;
            dz = (b.z - at.z) / 65536.0f;
        }
        float length = std::sqrt(dx * dx + dz * dz);
        float fx = length > 0 ? dx / length : 0, fz = length > 0 ? dz / length : 1;
        if (headings[view]) {
            float t = fx;
            fx = -fz * headings[view];
            fz = t * headings[view];
        }
        FixVector position = at, normal;
        short triangle, surfaceClass;
        unsigned short surface;
        position.y = Track_GetGroundHeightSurface(&position, &normal, &triangle, &surfaceClass, &surface, at.y) + 0x9000;
        FixMatrix m = {};
        m.forward = { int(fx * 65536), 0, int(fz * 65536) };
        m.up = { 0, 0x10000, 0 };
        m.right = { m.forward.z, 0, -m.forward.x };
        m.position = position;
        m.pw = 0x10000;
        car.physicsMatrix = m;
        car.bodyMatrix = m;
        car.position = car.positionPrev = car.positionPrev2 = position;
        car.right = car.targetRight = m.right;
        car.up = car.targetUp = m.up;
        car.forward = car.targetForward = m.forward;
        car.velocity = car.velocityNext = car.angularVelocity = FixVector{};
    }

    // The captured frames show the same moment: the clock stands still
    // through them, so nothing moves but what the layer changes.
    bool Frozen() const { return !done && frame >= HOLD - 4; }

    // Before a frame of the race.
    void Frame(Car &car) {
        if (done)
            return;
        if (++frame >= HOLD) {
            frame = 0;
            if (++view >= nodes.size()) {
                done = true;
                FarLayer_SetTest(1, 1, 0, 0);
                return;
            }
        }
        Place(car);
        if (frame == 0) {
            StageTiming_SetViewRouteDistanceLimit(0, nodes[view], 1);
            View_ResetShownFarPlane(0);
            ChaseCamera_Reset(0);
        }
        want = frame == HOLD - 3 ? SHOWN : frame == HOLD - 2 ? SKY : frame == HOLD - 1 ? MARKED : NONE;
        FarLayer_SetTest(want != SKY, want == SHOWN || want == NONE, want == MARKED, want == SKY);
    }

    void Save(const char *name, const uint32_t *pixels) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s-%04zu.bmp", out.c_str(), name, view);
        SDL_Surface *surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_ARGB8888, (void *)pixels, width * 4);
        if (surface) {
            SDL_SaveBMP(surface, path);
            SDL_DestroySurface(surface);
        }
    }

    // A presented frame's pixels.
    void Present(const unsigned char *bgra, int w, int h) {
        const uint32_t *p = (const uint32_t *)bgra;
        width = w;
        height = h;
        if (want == SHOWN)
            shown.assign(p, p + size_t(w) * h);
        else if (want == SKY)
            sky.assign(p, p + size_t(w) * h);
        else if (want == MARKED && sky.size() == size_t(w) * h) {
            unsigned faults = 0, nearSky = 0, near = 0, far = 0;
            for (size_t i = 0; i < sky.size(); i++) {
                uint32_t c = p[i] & 0xffffff;
                // Sky, keyed green; the sun's glare (its strength read back
                // from the frame, so not the same with the layer) may tint it.
                int r = (sky[i] >> 16) & 0xff, g = (sky[i] >> 8) & 0xff, b = sky[i] & 0xff;
                bool wasSky = r <= 24 && b <= 24 && g >= 230;
                if (c == 0xff00ff) {
                    ++near;
                    nearSky += wasSky;
                } else if (c == 0x00ff00) {
                    ++far;
                } else {
                    continue;
                }
                faults += !wasSky;
            }
            const RallyRouteNode *node = RallyData_GetAvailableRouteNodeRecord(int(nodes[view]));
            fprintf(csv, "%zu,%u,%d,%d,%d,%u,%u,%u,%u,%d\n", view, nodes[view], headings[view], node->position.x >> 16,
                    node->position.z >> 16, faults, nearSky, near, far, w * h);
            fflush(csv);
            if (faults > 50 || nearSky > 500 || view % 10 == 0) {
                if (shown.size() == sky.size())
                    Save("shown", shown.data());
                Save("marked", p);
                Save("sky", sky.data());
            }
        }
        want = NONE;
    }

    static void Callback(const unsigned char *bgra, int w, int h, void *context) {
        static_cast<Sweep *>(context)->Present(bgra, w, h);
    }
};

// OPENCMR2_PROBE_CURSORS="id:cursor,..." forces the cursor of the menus with
// those ids (as printed by "probe: menu"), to reach other game modes.
static int ForcedCursor(int id) {
    const char *spec = getenv("OPENCMR2_PROBE_CURSORS");
    while (spec && *spec) {
        int menu, cursor, used = 0;
        if (sscanf(spec, "%d:%d%n", &menu, &cursor, &used) != 2) break;
        if (menu == id) return cursor;
        spec += used;
        if (*spec == ',') spec++;
    }
    return -1;
}

static void ConfigureMenu(Menu *menu) {
    if (!menu) return;
    // Unlock tracks only in this isolated test profile; still select them
    // through the real menu and verify the archive chosen by the loader.
    if (getenv("OPENCMR2_PROBE_UNLOCK_TRACKS") && !CGameInfo::IsRecordFlagSet(0xd))
        CGameInfo::ToggleRecordFlag(0xd);
    // The all-cars cheat, for the bonus cars.
    if (getenv("OPENCMR2_PROBE_UNLOCK_CARS") && !CGameInfo::IsRecordFlagSet(0xc))
        CGameInfo::ToggleRecordFlag(0xc);
    if (menu == FrontendMenu_GetNetworkSessionDetails())
        if (const char *name = getenv("OPENCMR2_PROBE_SESSION_NAME"))
            snprintf(g_sessionName, sizeof(g_sessionName), "%s", name);
    if (menu == FrontendMenu_GetNetworkSessionSetup())
        if (const char *role = getenv("OPENCMR2_PROBE_LAN_ROLE"))
            if (strcmp(role, "guest") == 0) menu->items[3].max = 0; // Ready menu option.
    int forced = ForcedCursor(menu->field_0x4);
    if (forced >= 0) {
        // Some screens share an id (New/Load and the one-row save list).
        menu->cursor = (char)(forced < menu->itemCount ? forced : menu->itemCount - 1);
        return;
    }
    if (menu == FrontendMenu_GetMain()) menu->cursor = 1;
    else if (menu == FrontendMenu_GetRallyModes()) menu->cursor = 2;
    else if (menu == FrontendMenu_GetTimeTrialDifficulty()) { menu->cursor = 0; menu->items[0].max = 0; }
    else if (menu == FrontendMenu_GetRallySelection()) menu->cursor = 0;
    else if (menu == FrontendMenu_GetRallyStageSelection()) menu->cursor = 0;
    else if (menu == FrontendMenu_GetAlternateRallyStageSelection()) menu->cursor = 0;
    else if (menu == FrontendMenu_GetProfileNameEntry()) {
        char name[] = "TST";
        RallyData_SetEditedDriverOrCategoryName(FrontendProfile_GetCurrentPlayer(), name);
        menu->cursor = 2;
        menu->items[2].max = 9;
    }
}

extern unsigned short *g_sceneLightData;
extern LightZone *g_sceneLightZones;
extern short *g_sceneSectorZone;
extern Mesh **g_sceneShadowMeshes;

// Pointer-free digests of the stage lighting records and shadow meshes, by
// part: zones, items, owners (sector, position in its object list), zone mesh
// data, sector shadow meshes.
static void LightingDigest(unsigned *zonesOut, unsigned long long out[5])
{
    for (int k = 0; k < 5; k++) out[k] = 1469598103934665603ULL;
    auto mix = [&](int k, const void *p, size_t n) {
        for (size_t i = 0; i < n; i++) { out[k] ^= ((const BYTE *)p)[i]; out[k] *= 1099511628211ULL; }
    };
    auto mixInt = [&](int k, long long v) { mix(k, &v, sizeof(v)); };
    unsigned zones = g_sceneLightData ? *g_sceneLightData : 0;
    *zonesOut = zones;
    for (unsigned z = 0; z < zones; z++) {
        const LightZone &zone = g_sceneLightZones[z];
        mixInt(0, zone.sector); mixInt(0, zone.itemCount); mixInt(0, zone.x); mixInt(0, zone.z);
        for (unsigned i = 0; i < zone.itemCount; i++) {
            const LightZoneItem &item = zone.pItems[i];
            mix(1, &item, offsetof(LightZoneItem, intensity));
            mixInt(1, item.field_0x24); mixInt(1, item.vertexCount);
            long long owner = -1;
            for (int s = 0; s < g_sectorCount && owner < 0; s++) {
                int n = 0;
                for (StageObject *o = g_sectors[s]->pObjects; o; o = o->pNext, n++)
                    if (o == item.pOwner) { owner = (long long)s << 16 | n; break; }
            }
            mixInt(2, owner);
        }
        const LightZoneMesh &mesh = *zone.pMesh;
        mixInt(3, mesh.triangleCount); mixInt(3, mesh.vertexCount);
        // Not the diffuse colours: the scene lights write them for the
        // sectors in view (Scene_RelightSector), as the view culling picks.
        for (int v = 0; v < mesh.vertexCount; v++) {
            mix(3, &mesh.pVertices[v], offsetof(MeshVertexF, diffuseColour));
            mix(3, &mesh.pVertices[v].specularColour, sizeof(MeshVertexF) - offsetof(MeshVertexF, specularColour));
        }
        mix(3, mesh.pTriangles, mesh.triangleCount * 0x4c);
        mix(3, mesh.pLevels, mesh.vertexCount * 4);
    }
    for (int s = 0; g_sceneSectorZone && s < g_sectorCount; s++) {
        mixInt(4, g_sceneSectorZone[s]);
        Mesh *m = g_sceneShadowMeshes ? g_sceneShadowMeshes[s] : nullptr;
        mixInt(4, m != nullptr);
        if (m) { mixInt(4, m->field_0x10); mixInt(4, m->triangleCount); mixInt(4, m->flags); mix(4, m->field_0x0, 0xc); }
    }
}

int main(int argc, char **argv) {
    // Keep probe arguments out of the legacy command-line parser.
    if (argc != 5 && argc != 6) { fprintf(stderr, "usage: stage_probe DATA TRACE.jsonl FPS|variable TICKS [INPUTS.csv]\n"); return 2; }
    if (!getenv("XDG_DATA_HOME")) {
        fprintf(stderr, "Set XDG_DATA_HOME to a disposable directory, or use tools/stage_baseline.py\n");
        return 2;
    }
    const bool variable = strcmp(argv[3], "variable") == 0;
    const unsigned fps = variable ? 60 : unsigned(strtoul(argv[3], nullptr, 10));
    const unsigned ticks = unsigned(strtoul(argv[4], nullptr, 10));
    if (fps < 10 || fps > 4000 || !ticks || ticks > 100000) return 2;
    FILE *file = fopen(argv[2], "w");
    if (!file) { perror(argv[2]); return 2; }
    Probe probe(file, ticks);
    if (argc == 6 && !probe.LoadInputs(argv[5])) {
        fprintf(stderr, "Invalid playback file: %s (requires exactly %u ordered control records)\n", argv[5], ticks);
        fclose(file);
        return 2;
    }
    SDL_SetMainReady();
    char dataArg[] = "--data";
    char userArg[] = "--user-dir";
    char *platformArgs[] = {argv[0], dataArg, argv[1], userArg, getenv("XDG_DATA_HOME")};
    if (!Platform_Init(5, platformArgs)) { fclose(file); return 1; }
    const PlatformTestClock clock{Ticks, Sleep, 1700000000, TicksPrecise};
    BYTE keys[256]{};
    Platform_SetTestClock(&clock);
    Platform_SetTestKeyboard(keys);
    Main_InitD3DX();
    CGame::RegisterNetworkResourceRelease();
    Input_ClearKeyPressQueue();
    char command[] = "";
    Args_Parse(command);
    if (!CMain::CreateGameWindow("OpenCMR2 stage probe")) return 1;
    Diagnostics::observer = &probe;
    DWORD raceOrigin = 0;
    unsigned raceFrame = 0;
    Menu *lastMenu = nullptr;
    int lastMode = -1;
    DWORD serviceOrigin = 0;
    bool mapOpening = false, mapExpanded = false;
    bool sessionSaved = false;
    const unsigned resultsPresses = getenv("OPENCMR2_PROBE_RESULTS_PRESSES") ? unsigned(atoi(getenv("OPENCMR2_PROBE_RESULTS_PRESSES"))) : 0;
    unsigned pressesDone = 0, pressMoves = 0;
    DWORD pressAt = 0;
    bool pressDown = false;
    int pressCursor = -1;
    const char *lanRole = getenv("OPENCMR2_PROBE_LAN_ROLE");
    const Uint64 lanOrigin = SDL_GetTicks();
    bool lanReady = false;
    // OPENCMR2_PROBE_FRAME_TIMES=1: the wall time of each frame while the car
    // drives (the GPU's included: frames wait for it), summed up at the end.
    // For measuring drawing costs, with vsync off in the profile.
    const bool frameTimes = getenv("OPENCMR2_PROBE_FRAME_TIMES") != nullptr;
    std::vector<double> frameMs;
    Uint64 lastFrame = 0;
    Sweep sweep;
    const float sweepSpacing = getenv("OPENCMR2_PROBE_SWEEP") ? float(atof(getenv("OPENCMR2_PROBE_SWEEP"))) : 0.0f;
    if (sweepSpacing > 0) {
        sweep.out = getenv("OPENCMR2_PROBE_SWEEP_OUT") ? getenv("OPENCMR2_PROBE_SWEEP_OUT") : ".";
        sweep.csv = fopen((sweep.out + "/sweep.csv").c_str(), "w");
        if (!sweep.csv) { perror("sweep.csv"); return 2; }
        fprintf(sweep.csv, "view,node,heading,x,z,faults,near_sky,near,far,pixels\n");
    }
    for (unsigned frame = 0; frame < 200000 && probe.tick < ticks && !CGame::m_shouldExit; ++frame) {
        // Socket threads use wall time, even when the simulation clock is
        // controlled. Give both independent game processes time to exchange
        // discovery, lobby and race-start packets.
        if (lanRole) {
            SDL_Delay(10);
            if (SDL_GetTicks() - lanOrigin > 40000) break;
        }
        SDL_PumpEvents();
        // The dummy driver does not focus windows. This test dispatches frames
        // explicitly, so SDL focus and wall time cannot influence its clock.
        const int mode = CGame::GetFrontendResourceMode();
        // Create the fixture from a real garage session. A second probe run
        // selects it through the frontend's Load Game menu.
        if (mode == 2 && !sessionSaved && getenv("OPENCMR2_PROBE_SAVE_SESSION")) {
            char name[] = "probe session";
            if (!SavedGames_WriteSessionImage(-1, name)) {
                fprintf(stderr, "probe: failed to save session\n");
                return 1;
            }
            SavedGames_LoadRecords();
            if (SavedGames_GetCount() != 1) return 1;
            sessionSaved = true;
            fprintf(stderr, "probe: saved garage session\n");
        }
        // The race clock of the reference captures; OPENCMR2_PROBE_RACE_CLOCK
        // picks another, which seeds the start side of the super specials.
        const char *raceClock = getenv("OPENCMR2_PROBE_RACE_CLOCK");
        if (mode == 3 && !raceOrigin && (getenv("OPENCMR2_REFERENCE_CLOCK") || raceClock)) {
            virtualTime = raceClock ? DWORD(strtoul(raceClock, nullptr, 10)) : 100000;
            CMain::UpdateFrameTime();
            raceOrigin = virtualTime;
        }
        if (mode != lastMode) { fprintf(stderr, "probe: resource mode %d at %u ms\n", mode, virtualTime); lastMode = mode; }
        if (getenv("OPENCMR2_PROBE_HEADLIGHT_CHEAT") && CGameInfo::GetConfiguredGameMode() == 5) {
            GameInfo_SetConfiguredCheat(0, 1);
            GameInfo_ApplyCheatsForGameMode();
        }
        // A profile that has played: every stage record holds split times
        // (the challenge reads one past them, for Japan, as a ghost name).
        if (mode != 3 && getenv("OPENCMR2_PROBE_STAGE_RECORDS")) {
            auto *records = CGameInfo::GetGameInfoFieldA4Address();
            for (int r = 0; r < 88; r++)
                for (int split = 0; split < 10; split++)
                    records->rallyStageRecordSplits[r][split] = short(0x1a2b + r * 37 + split * 401);
        }
        // The car detail setting (0 high, 2 low: the original's first-run
        // default, which its reference captures used).
        if (mode != 3)
            if (const char *detail = getenv("OPENCMR2_PROBE_CAR_DETAIL"))
                CGameInfo::SetPreviewMode(unsigned(atoi(detail)));
        // Stage lighting (shadow zones) is loaded only with this graphics option.
        if (mode != 3 && getenv("OPENCMR2_PROBE_LIGHTING")) CGameInfo::SetGraphicsOptionBit4(1);
        static bool lightingDone;
        if (mode == 3 && probe.tick > 0 && !lightingDone && getenv("OPENCMR2_PROBE_LIGHTING")) {
            lightingDone = true;
            unsigned zones;
            unsigned long long d[5];
            LightingDigest(&zones, d);
            fprintf(stderr, "probe: lighting %u zones %016llx %016llx %016llx %016llx %016llx\n",
                    zones, d[0], d[1], d[2], d[3], d[4]);
        }
        memset(keys, 0, sizeof(keys));
        // Rally Challenge has a bracket confirmation screen inside race
        // resource mode, before any simulation tick can run.
        if (mode == 3 && probe.tick == 0 && CGameInfo::GetConfiguredGameMode() == 4 && frame % 12 == 6)
            keys[INPUT_KEY_RETURN] = 0x80;
        bool holdMap = false;
        if (mode != 3 || probe.finishInjected) {
            Menu *menu = mode == 3 ? g_pHeaderMenu : mode == 2 ? g_pMenu0x00831778 : FrontendMenu_GetActiveMenu();
            if (menu && menu != lastMenu) {
                fprintf(stderr, "probe: menu %d (%d entries)\n", menu->field_0x4, menu->itemCount);
                if (getenv("OPENCMR2_PROBE_MENU_TEXT"))
                    for (int item = 0; item < menu->itemCount; item++)
                        fprintf(stderr, "probe:   %d: %s\n", item, menu->items[item].text ? menu->items[item].text
                                : menu->items[item].id >= 0 ? CFrontend::GetTextString(menu->items[item].id) : "");
                lastMenu = menu;
            }
            if (mode == 2 && getenv("OPENCMR2_PROBE_EXPAND_MAP")) {
                if (!serviceOrigin) serviceOrigin = virtualTime;
                if (g_selectedOptionPreviewPanel != 0xff) {
                    const FixInterp &panel = g_optionPreviewPanels[g_selectedOptionPreviewPanel];
                    if (panel.current > 0) mapOpening = true;
                    if (!mapExpanded && panel.current == 0x10000 && panel.phase2 == 0x10000) {
                        mapExpanded = true;
                        fprintf(stderr, "probe: preliminary map fully expanded at %u ms\n", virtualTime);
                    }
                }
                holdMap = !mapExpanded || virtualTime - serviceOrigin < 5000;
            }
            if (mode == 2 && menu && menu->itemCount >= 4) {
                int forced = ForcedCursor(menu->field_0x4);
                menu->cursor = holdMap ? 0 : forced >= 0 ? forced : 3;
            }
            else ConfigureMenu(menu);
            bool holdLobby = false;
            if (lanRole && menu == FrontendMenu_GetNetworkSessionSetup()) {
                holdLobby = true;
                if (strcmp(lanRole, "host") == 0)
                    for (const NetPlayer &player : g_netPlayers)
                        if ((player.flags & 0xc0) == 0xc0) {
                            holdLobby = false;
                            if (!lanReady) fprintf(stderr, "probe: LAN guest ready id=%d\n", player.id);
                            lanReady = true;
                        }
            }
            // OPENCMR2_PROBE_RESULTS_PRESSES=N: in the results menu after the
            // finish, press Down N times (80 ms each, 160 ms apart) instead
            // of Enter, and count how often the cursor moved.
            if (resultsPresses && mode == 3 && probe.finishInjected && menu && menu->itemCount == 2 &&
                pressesDone <= resultsPresses) {
                // The result menu can exist while its fade still blocks input.
                if (!pressAt && !NetRace_IsPlayerFadeActive(0)) {
                    pressAt = virtualTime + 200;
                    pressCursor = menu->cursor;
                }
                if (menu->cursor != pressCursor) { ++pressMoves; pressCursor = menu->cursor; }
                // Hold the final press for 80 ms too: at high frame rates a
                // one-frame pulse can fit entirely between simulation steps.
                if (pressAt && (pressesDone < resultsPresses || pressDown) && virtualTime >= pressAt) {
                    if (!pressDown) { pressDown = true; ++pressesDone; }
                    if (virtualTime < pressAt + 80) keys[INPUT_KEY_DOWN] = 0x80;
                    else { pressDown = false; pressAt += 160; }
                } else if (pressAt && pressesDone == resultsPresses && virtualTime >= pressAt + 80) {
                    fprintf(stderr, "probe: results menu %u presses, %u moves\n", pressesDone, pressMoves);
                    ++pressesDone;
                }
            } else
            if ((frame % 12 == 6 || !menu) && !(holdMap && mapOpening) && !holdLobby) {
                keys[INPUT_KEY_RETURN] = 0x80;
                // Blocking movie playback consumes SDL events itself.
                SDL_Event event{};
                event.type = SDL_EVENT_KEY_DOWN;
                event.key.scancode = SDL_SCANCODE_RETURN;
                SDL_PushEvent(&event);
            }
        }
        if (sweepSpacing > 0 && mode == 3 && probe.drivingTick > 0) {
            if (sweep.nodes.empty() && !sweep.done) {
                sweep.Plan(sweepSpacing);
                Gfx_SetFrameCallback(Sweep::Callback, &sweep);
            }
            sweep.Frame(g_carBuffer[g_carOrder[0]]);
            if (sweep.done)
                break;
        }
        if (frameTimes && mode == 3 && probe.drivingTick > 0) {
            Uint64 now = SDL_GetPerformanceCounter();
            if (lastFrame)
                frameMs.push_back(double(now - lastFrame) * 1000.0 / double(SDL_GetPerformanceFrequency()));
            lastFrame = now;
        }
        CGame::UpdateActiveSoundSlots();
        probe.BeginFrame();
        CGame::DispatchFrontendResourceState();
        probe.EndFrame();
        if (mode == 3 && sweepSpacing > 0 && sweep.Frozen()) {
        } else if (mode == 3) {
            if (!raceOrigin) raceOrigin = virtualTime;
            ++raceFrame;
            if (variable) {
                const DWORD pattern[] = {7, 13, 33, 5, 22, 48, 9, 23};
                virtualTime += pattern[(raceFrame - 1) % 8];
            } else {
                const uint64_t micros = uint64_t(raceFrame) * 1000000 / fps;
                virtualTime = raceOrigin + DWORD(micros / 1000);
                virtualFraction = double(micros % 1000) / 1000.0;
            }
        } else virtualTime += 33;
    }
    if (frameTimes && !frameMs.empty()) {
        std::vector<double> sorted = frameMs;
        qsort(sorted.data(), sorted.size(), sizeof(double), [](const void *a, const void *b) {
            double x = *(const double *)a, y = *(const double *)b;
            return x < y ? -1 : x > y;
        });
        fprintf(stderr, "probe: %zu driving frames, median %.2f ms, p90 %.2f ms, p99 %.2f ms\n", sorted.size(),
                sorted[sorted.size() / 2], sorted[sorted.size() * 9 / 10], sorted[sorted.size() * 99 / 100]);
    }
    Diagnostics::observer = nullptr;
    if (!probe.Write((std::string(argv[2]) + ".frames.csv").c_str())) return 1;
    const bool ok = probe.tick == ticks && probe.drivingTick > 0 && probe.scenarioCorrect && probe.playbackCorrect && !ferror(file) &&
        (!getenv("OPENCMR2_PROBE_EXPAND_MAP") || mapExpanded) &&
        (!getenv("OPENCMR2_PROBE_FINISH_AT_TICK") || (probe.finishInjected &&
            (resultsPresses ? pressesDone > resultsPresses : probe.nextStageDrivingTicks >= 100))) &&
        strcmp(g_winnerFormat, "%s, %s") == 0 && strlen(g_ghostName) < sizeof(g_ghostName);
    // The results menus' data after the ghost name must survive the results.
    if (strcmp(g_winnerFormat, "%s, %s") != 0 || strlen(g_ghostName) >= sizeof(g_ghostName))
        fprintf(stderr, "probe: results menu data overwritten (ghost name %zu bytes)\n", strlen(g_ghostName));
    if (probe.finishInjected) fprintf(stderr, "probe: Finland stage 2 driving ticks=%u\n", probe.nextStageDrivingTicks);
    fprintf(stderr, "probe: RNG changed in %u race frames without a simulation step\n", probe.renderOnlyRngChanges);
    fprintf(stderr, "probe: %u ticks, %u driving ticks, %u ms; %s\n", probe.tick, probe.drivingTick, virtualTime, ok ? "complete" : "FAILED");
    CMain::UnwindGameCallbacks();
    Platform_SetTestKeyboard(nullptr);
    Platform_SetTestClock(nullptr);
    fclose(file);
    Platform_Shutdown();
    return ok ? 0 : 1;
}
