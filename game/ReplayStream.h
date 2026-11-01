#ifndef _REPLAY_STREAM_H
#define _REPLAY_STREAM_H

#include "FixedPoint.h"

// Replay event: from `frame`, `count` more frames carry `value`.
struct ReplayEvent {
    short frame;            // 0x0
    short count;            // 0x2
    BYTE value;             // 0x4
    BYTE pad;
};

// Per-lane record of a type 0 (input) stream.
struct ReplayInputLane {
    union {
        BYTE inputs[0x110c];
        struct {
            BYTE partState[0x4d0];  // 0x0    the car's part set (Replay_CopyAuxState)
            BYTE carState[0xc24];   // 0x4d0  the start of its Car (Replay_CopyBlock309)
            int raceRecord[6];      // 0x10f4 its race record (Replay_CopyBlock6)
        };
    };
    ReplayEvent events[10]; // 0x110c
    BYTE eventCount;        // 0x1148
    BYTE pad[3];
};

// Per-lane record of a type 1/2 (state) stream.
struct ReplayStateLane {
    BYTE data[0x1c];
    ReplayEvent events[10]; // 0x1c
    BYTE eventCount;        // 0x58
    BYTE pad[3];
};

// The two car poses a type 2 stream interpolates between, and the velocity
// that carries the car from one to the next.
struct ReplayPose {
    FixMatrix from;         // 0x00
    FixMatrix to;           // 0x40
    FixVector velocity;     // 0x80
};


// ON-DISK: replay and ghost files are this header followed by the stream's
// arrays; the pointers are rebuilt by Replay_SetupPointers after loading.
// Its 0x110 Win32 bytes hold pointers, so a 64-bit build has to convert the
// header to read or write files of the original layout.
// One replay stream (recorder/player of one car); the slots hang off
// g_replaySlots. Type 2 streams store a 16-byte car sample every third frame
// and play them back by interpolating between two poses.
struct ReplayStream {
    DWORD steeringModeOption; // 0x00 recorded ControllerData::smoothSteering
    int playing;            // 0x04
    int playStarted;        // 0x08
    int recording;          // 0x0c
    int recordStarted;      // 0x10
    int field_0x14;         // 0x14
    int field_0x18;         // 0x18
    int type;               // 0x1c 0 inputs, 1 states, 2 car samples
    BYTE car;               // 0x20
    BYTE field_0x21;        // 0x21
    BYTE pad22[2];
    ReplayInputLane *pInputs;       // 0x24 type 0 lanes
    ReplayInputLane *pInputLane;    // 0x28 lane being played back
    ReplayInputLane *pInputRecLane; // 0x2c lane being recorded
    ReplayStateLane *pStates;       // 0x30 type 1/2 lanes
    ReplayStateLane *pStateLane;    // 0x34 lane being played back
    ReplayStateLane *pStateRecLane; // 0x38 lane being recorded
    BYTE *pFrames;          // 0x3c 4 bytes per frame
    BYTE *pSamples;         // 0x40 16 bytes per sample
    ReplayPose pose;        // 0x44
    int headingFrom;        // 0xd0
    int headingTo;          // 0xd4
    int steerFrom;          // 0xd8
    int steerTo;            // 0xdc
    int eventPrev;          // 0xe0
    int event;              // 0xe4
    unsigned int field_0xe8; // 0xe8
    unsigned int field_0xec; // 0xec
    unsigned int flagPrev;  // 0xf0
    unsigned int flag;      // 0xf4
    BYTE step;              // 0xf8
    BYTE padf9[3];
    short laneCapacity;     // 0xfc
    short samplesPerLane;   // 0xfe
    short laneCount;        // 0x100
    short pad102;
    short *pLaneSamples;    // 0x104
    short lane;             // 0x108
    short frame;            // 0x10a
    BYTE field_0x10c;       // 0x10c
    BYTE pad10d[3];
};


// Packed 16-byte car sample of a type 2 replay stream / network packet.
struct ReplaySample {
    int y;                          // 0x0 height
    int x : 16;                     // 0x4 position in the sector, 1/128 units
    int z : 16;
    unsigned int sector : 16;       // 0x8
    unsigned int rightAngles : 16;  // heading | pitch << 8 of the right vector
    unsigned int forwardHeading : 8; // 0xc
    unsigned int forwardPitch : 8;
    unsigned int level : 5;
    unsigned int bits21 : 3;
    unsigned int flag24 : 1;
    unsigned int steering : 1;
    unsigned int flag26 : 1;
    unsigned int flag27 : 1;
    unsigned int pad28 : 4;
};

#endif
