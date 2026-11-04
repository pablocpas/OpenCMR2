/*
 * OpenCMR2 audio interface, implemented in src/audio (one SDL3 audio stream
 * fed by our mixer).
 *
 * The game was written against DirectSound buffers and keeps that model:
 * buffers of PCM data that play (once or looping) at their own frequency,
 * volume and pan; duplicates that share a buffer's data; ring buffers that
 * are refilled behind the play cursor (the streamed music). Volume and pan
 * are in hundredths of a decibel, as in DirectSound.
 */
#ifndef OPENCMR2_PORT_AUDIO_H
#define OPENCMR2_PORT_AUDIO_H

#include "port/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AudioBuffer AudioBuffer;
typedef struct AudioStream AudioStream;

#define AUDIO_VOLUME_MAX 0
#define AUDIO_VOLUME_MIN (-10000)
#define AUDIO_PAN_LEFT (-10000)
#define AUDIO_PAN_CENTER 0
#define AUDIO_PAN_RIGHT 10000

/* Opens the output device. channels is 1, 2, 4, 6 (5.1) or 8 (7.1). */
BOOL Audio_Init(int sampleRate, int channels);
void Audio_Shutdown(void);
/* Holds the output while the game is in the background, as DirectSound
   silenced a window without the focus; the buffers keep their positions. */
void Audio_SetPaused(BOOL paused);
/* Sets the output channel count (2, 4, 6 or 8). Must be called before
   Audio_Init, or the device is recreated on the next Audio_Init. */
BOOL Audio_SetOutputChannels(int channels);
/* Returns the current output channel count. */
int Audio_GetOutputChannels(void);

/* A buffer of 8- or 16-bit PCM. */
AudioBuffer *Audio_CreateBuffer(DWORD sampleRate, int bits, int channels, DWORD bytes);
/* A new buffer that plays the same data with its own position and settings. */
AudioBuffer *Audio_DuplicateBuffer(AudioBuffer *source);
/* Stops and frees the buffer; the data goes with its last user. */
void Audio_ReleaseBuffer(AudioBuffer *buffer);

DWORD Audio_GetBufferSize(AudioBuffer *buffer);
void Audio_GetBufferFormat(AudioBuffer *buffer, DWORD *sampleRate, int *bits, int *channels);

/* Gives access to bytes [offset, offset + bytes) of the buffer, wrapping at
   its end: the first part in (*p1, *n1), the wrapped part in (*p2, *n2). */
BOOL Audio_LockBuffer(AudioBuffer *buffer, DWORD offset, DWORD bytes, void **p1, DWORD *n1, void **p2, DWORD *n2);
void Audio_UnlockBuffer(AudioBuffer *buffer);

void Audio_Play(AudioBuffer *buffer, BOOL loop);
void Audio_Stop(AudioBuffer *buffer);
BOOL Audio_IsPlaying(AudioBuffer *buffer);
DWORD Audio_GetPlayPosition(AudioBuffer *buffer);
void Audio_SetPlayPosition(AudioBuffer *buffer, DWORD position);

void Audio_SetVolume(AudioBuffer *buffer, LONG volume);
void Audio_SetPan(AudioBuffer *buffer, LONG pan);
void Audio_SetFrequency(AudioBuffer *buffer, DWORD frequency);

/* ---- spatial audio ------------------------------------------------------- */

/* Sets the 3D position (in arbitrary world units) of a sound source. The
   mixer pans and attenuates the buffer based on the listener's position and
   orientation (set with Audio_SetListener*). Default position is (0, 0, 0). */
void Audio_SetPosition(AudioBuffer *buffer, float x, float y, float z);
/* Gets the 3D position of a sound source. */
void Audio_GetPosition(AudioBuffer *buffer, float *x, float *y, float *z);

/* Sets the listener (camera) position and orientation. The listener is what
   all spatial panning is relative to. Default position is (0, 0, 0), facing
   down +Z with +Y up. */
void Audio_SetListenerPosition(float x, float y, float z);
void Audio_SetListenerOrientation(float fx, float fy, float fz,
                                  float ux, float uy, float uz);
void Audio_GetListenerPosition(float *x, float *y, float *z);
void Audio_GetListenerOrientation(float *fx, float *fy, float *fz,
                                  float *ux, float *uy, float *uz);

/* Distance attenuation for spatial sounds. minDistance is where full volume
   starts dropping off; maxDistance is where the sound is silent. rolloff
   controls the curve (1.0 = linear inverse distance). Defaults:
   minDistance=1.0, maxDistance=100.0, rolloff=1.0. */
void Audio_SetMinDistance(AudioBuffer *buffer, float distance);
void Audio_SetMaxDistance(AudioBuffer *buffer, float distance);
void Audio_SetRolloffFactor(AudioBuffer *buffer, float rolloff);
void Audio_GetMinDistance(AudioBuffer *buffer, float *distance);
void Audio_GetMaxDistance(AudioBuffer *buffer, float *distance);
void Audio_GetRolloffFactor(AudioBuffer *buffer, float *rolloff);

/* ---- offline mix tap (replay video export) -------------------------------
   Audio_GetOutputRate returns 0 when audio is not initialized.
   Audio_MixOffline renders `frames` output frames as 16-bit PCM and advances
   buffer playback exactly like live output. It refuses (returns 0) unless the
   device is paused with Audio_SetPaused(1), so the live callback and the
   offline tap can never advance the same cursors concurrently. */
int Audio_GetOutputRate(void);
int Audio_MixOffline(short *dst, int frames);

/* Queued float PCM for movie sound. The mixer resamples to its output rate.
   Starts after the first video frame is presented; releasing discards queued
   sound immediately, including when the user skips a movie. */
AudioStream *Audio_CreateStream(DWORD sampleRate, int channels);
BOOL Audio_QueueStream(AudioStream *stream, const float *samples, DWORD sampleCount);
void Audio_StartStream(AudioStream *stream);
void Audio_ReleaseStream(AudioStream *stream);

/* ---- wave files ---------------------------------------------------------- */

#define AUDIO_WAVE_FORMAT_PCM 1
#define AUDIO_WAVE_FORMAT_ADPCM 2

#pragma pack(push, 1)
/* The "fmt " chunk of a .wav file (WAVEFORMATEX); cbSize extra bytes follow. */
typedef struct AudioWaveFormat {
    WORD wFormatTag;
    WORD nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD nBlockAlign;
    WORD wBitsPerSample;
    WORD cbSize;
} AudioWaveFormat;
#pragma pack(pop)

/* Decodes whole Microsoft ADPCM blocks (format from the file) into 16-bit
   PCM. Returns the number of bytes written; stops when dst is full. */
DWORD Audio_DecodeMsAdpcm(const AudioWaveFormat *format, const BYTE *src, DWORD srcBytes, BYTE *dst, DWORD dstBytes);

#ifdef __cplusplus
}
#endif

#endif
