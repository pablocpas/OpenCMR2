#ifndef CMR2_PCM_WAVE_HEADER_H
#define CMR2_PCM_WAVE_HEADER_H

#include <windows.h>

// Fixed 44-byte file header assumed by Sound_LoadWave. This loader does not
// scan RIFF chunks or validate the format/data identifiers and sizes.
struct PcmWaveHeader {
    char riffId[4];                 // 0x00
    DWORD riffSize;                 // 0x04
    char waveId[4];                 // 0x08
    char formatId[4];               // 0x0c
    DWORD formatSize;               // 0x10
    WORD formatTag;                 // 0x14
    WORD channels;                  // 0x16
    DWORD sampleRate;               // 0x18
    DWORD averageBytesPerSecond;    // 0x1c
    WORD blockAlign;                // 0x20
    WORD bitsPerSample;             // 0x22
    char dataId[4];                 // 0x24
    DWORD dataSize;                 // 0x28
};

#endif
