// Audio on SDL3: see port/audio.h. Every buffer is mixed in software into one
// float stereo stream, with DirectSound's volume and pan laws.

#include "platform/platform.h"
#include "port/audio.h"
#include "port/sys.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <vector>
#include <algorithm>

namespace {

struct AudioData {
    BYTE *bytes;
    DWORD size;
    DWORD sampleRate;
    int bits;
    int channels;
    int refs;
};

}

struct AudioBuffer {
    AudioData *data;
    bool playing = false;
    bool loop = false;
    double position = 0.0;      // in frames
    DWORD frequency;
    float leftGain = 1.0f, rightGain = 1.0f;
    LONG volume = 0, pan = 0;

    // ---- spatial audio ---------------------------------------------------
    float posX = 0.0f, posY = 0.0f, posZ = 0.0f;  // source position
    float minDistance = 1.0f;   // full volume inside this
    float maxDistance = 100.0f; // silent beyond this
    float rolloff = 1.0f;       // 1.0 = linear inverse distance
    bool spatial = false;       // when false, use legacy pan/volume only
};

struct AudioStream {
    SDL_AudioStream *converter;
    bool playing = false;
};

namespace {

SDL_AudioStream *s_stream;
bool s_paused;
SDL_Mutex *s_lock;
int s_outputRate = 44100;
int s_outputChannels = 2;   // 2, 4, 6 (5.1) or 8 (7.1)
std::vector<AudioBuffer *> s_buffers;
std::vector<AudioStream *> s_movieStreams;
std::vector<float> s_mix;
std::vector<float> s_movieMix;

// Listener (camera) state. Default: at origin, facing +Z, up +Y.
float s_listenerX = 0.0f, s_listenerY = 0.0f, s_listenerZ = 0.0f;
float s_listenerFX = 0.0f, s_listenerFY = 0.0f, s_listenerFZ = 1.0f;
float s_listenerUX = 0.0f, s_listenerUY = 1.0f, s_listenerUZ = 0.0f;

// Speaker angles in radians, measured clockwise from front (+Z), for
// channel index i in the output stream. LFE (index 2 in 5.1/7.1) is not
// panned; it receives the sum of low frequencies.
static const float s_speakerAngles[8] = {
    -0.524f,  // -30 deg: front left
     0.524f,  // +30 deg: front right
     0.0f,    //   0 deg: front center
     0.0f,    // LFE (not spatially panned)
    -1.920f,  // -110 deg: rear left
     1.920f,  // +110 deg: rear right
    -1.571f,  // -90 deg: side left  (7.1 only)
     1.571f,  // +90 deg: side right (7.1 only)
};

// DirectSound's laws: volume and pan in hundredths of a decibel.
float DecibelGain(LONG hundredths)
{
    if (hundredths <= -10000)
        return 0.0f;
    return powf(10.0f, hundredths / 2000.0f);
}

void UpdateGains(AudioBuffer *b)
{
    float volume = DecibelGain(b->volume);
    float left = b->pan > 0 ? DecibelGain(-b->pan) : 1.0f;
    float right = b->pan < 0 ? DecibelGain(b->pan) : 1.0f;
    b->leftGain = volume * left;
    b->rightGain = volume * right;
}

inline float SampleAt(const AudioData *d, DWORD frame, int channel)
{
    int ch = channel < d->channels ? channel : 0;
    if (d->bits == 16) {
        const Sint16 *p = (const Sint16 *)d->bytes;
        return p[frame * d->channels + ch] / 32768.0f;
    }
    return (d->bytes[frame * d->channels + ch] - 128) / 128.0f;
}

// Computes the per-channel gain for a sound at (sx, sy, sz) relative to the
// listener, using vector-base amplitude panning. The result is written into
// gains[0..nChannels-1]. Returns the distance attenuation factor.
void ComputeSpatialGains(float sx, float sy, float sz,
                         float *gains, int nChannels)
{
    // Vector from listener to source.
    float dx = sx - s_listenerX;
    float dy = sy - s_listenerY;
    float dz = sz - s_listenerZ;

    // Build listener basis: forward, up, right (all normalized).
    float fx = s_listenerFX, fy = s_listenerFY, fz = s_listenerFZ;
    float ux = s_listenerUX, uy = s_listenerUY, uz = s_listenerUZ;
    float flen = sqrtf(fx*fx + fy*fy + fz*fz);
    float ulen = sqrtf(ux*ux + uy*uy + uz*uz);
    if (flen > 0.0f) { fx /= flen; fy /= flen; fz /= flen; }
    if (ulen > 0.0f) { ux /= ulen; uy /= ulen; uz /= ulen; }
    float rx = fy*uz - fz*uy;
    float ry = fz*ux - fx*uz;
    float rz = fx*uy - fy*ux;
    float rlen = sqrtf(rx*rx + ry*ry + rz*rz);
    if (rlen > 0.0f) { rx /= rlen; ry /= rlen; rz /= rlen; }

    // Source direction in listener space.
    float lx = dx*rx + dy*ry + dz*rz;     // right component
    float ly = dx*ux + dy*uy + dz*uz;     // up component
    float lz = dx*fx + dy*fy + dz*fz;     // forward component

    float dist = sqrtf(dx*dx + dy*dy + dz*dz);

    // Horizontal angle: 0 = front, positive = right, range [-pi, pi].
    float az = atan2f(lx, lz);

    // For each speaker, compute gain via vector-base amplitude panning.
    // gain = max(0, cos(angle between sound and speaker))^sharpness
    const float sharpness = 1.5f;
    float totalPower = 0.0f;
    float rawGains[8] = {0};
    int nLFE = (nChannels == 6 || nChannels == 8) ? 1 : 0;
    int nSpatial = nChannels - nLFE;  // non-LFE channels

    for (int i = 0; i < nSpatial; i++) {
        float sa = s_speakerAngles[i];
        // Angle difference between sound and speaker, wrapped to [-pi, pi].
        float d = az - sa;
        while (d >  3.14159f) d -= 6.28318f;
        while (d < -3.14159f) d += 6.28318f;
        float cosD = cosf(d);
        // Also account for front/back via the forward component sign.
        float frontBack = (lz >= 0.0f) ? 1.0f : 0.5f;
        float g = cosD > 0.0f ? powf(cosD, sharpness) : 0.0f;
        g *= frontBack;
        rawGains[i] = g;
        totalPower += g * g;
    }

    // Normalize: preserve total power (constant power panning).
    float norm = (totalPower > 0.0f) ? sqrtf((float)nSpatial / totalPower) : 1.0f;
    for (int i = 0; i < nSpatial; i++) {
        gains[i] = rawGains[i] * norm;
    }
    // LFE channel: not spatially panned.
    if (nLFE)
        gains[nSpatial] = 0.0f;
}

// Distance attenuation (inverse distance model).
float DistanceAttenuation(float dist, float minDist, float maxDist, float rolloff)
{
    if (dist <= minDist)
        return 1.0f;
    if (dist >= maxDist)
        return 0.0f;
    return minDist / (minDist + rolloff * (dist - minDist));
}

// Mixes one buffer's audio into the output mix, applying spatial panning and
// distance attenuation when spatial is enabled.
void MixBuffer(AudioBuffer *b, float *out, int frames, int nChannels)
{
    AudioData *d = b->data;
    DWORD frameBytes = (DWORD)(d->bits / 8 * d->channels);
    DWORD total = frameBytes ? d->size / frameBytes : 0;
    double step = (double)b->frequency / s_outputRate;

    if (total == 0) {
        b->playing = false;
        return;
    }

    // Compute spatial gains once per mix (position is static per buffer).
    float gains[8] = {0};
    float distAtt = 1.0f;
    if (b->spatial) {
        float dx = b->posX - s_listenerX;
        float dy = b->posY - s_listenerY;
        float dz = b->posZ - s_listenerZ;
        float dist = sqrtf(dx*dx + dy*dy + dz*dz);
        distAtt = DistanceAttenuation(dist, b->minDistance, b->maxDistance, b->rolloff);
        ComputeSpatialGains(b->posX, b->posY, b->posZ, gains, nChannels);
    } else {
        // Legacy stereo pan/volume, mapped to front L/R.
        gains[0] = b->leftGain;
        gains[1] = b->rightGain;
        for (int i = 2; i < nChannels; i++)
            gains[i] = 0.0f;
    }

    float volume = DecibelGain(b->volume);

    for (int i = 0; i < frames; i++) {
        if (b->position >= total) {
            if (!b->loop) {
                b->playing = false;
                b->position = 0.0;
                return;
            }
            b->position = fmod(b->position, (double)total);
        }
        DWORD f0 = (DWORD)b->position;
        DWORD f1 = f0 + 1 < total ? f0 + 1 : (b->loop ? 0 : f0);
        float t = (float)(b->position - f0);
        float l = SampleAt(d, f0, 0) * (1.0f - t) + SampleAt(d, f1, 0) * t;
        float r = SampleAt(d, f0, 1) * (1.0f - t) + SampleAt(d, f1, 1) * t;

        // For mono sources, l == r; pan/spatial applies to the mix.
        float baseL = l * volume * distAtt;
        float baseR = r * volume * distAtt;

        for (int ch = 0; ch < nChannels; ch++) {
            float g = gains[ch];
            // Front channels use L/R based on position; rear/side use a
            // blend weighted by front/back. For simplicity in the legacy
            // non-spatial path, only front L/R get signal.
            float sample = (ch == 0) ? baseL : (ch == 1) ? baseR : 0.0f;
            out[i * nChannels + ch] += sample * g;
        }
        b->position += step;
    }
}

// Mixes `frames` output frames of every playing buffer and movie stream into
// dst (float, s_outputChannels interleaved, caller-allocated). Advances buffer
// playback cursors: whoever calls this owns consumption while it runs.
// OpenCMR2: shared by the live device callback (Feed) and the offline replay
// export tap (Audio_MixOffline); never both at once - the export pauses the
// device first and refuses to mix while unpaused.
void MixInto(float *dst, int frames)
{
    s_movieMix.resize((size_t)frames * s_outputChannels);
    SDL_LockMutex(s_lock);
    for (AudioBuffer *b : s_buffers)
        if (b->playing)
            MixBuffer(b, dst, frames, s_outputChannels);
    for (AudioStream *movie : s_movieStreams) {
        if (!movie->playing)
            continue;
        int bytes = SDL_GetAudioStreamData(movie->converter, s_movieMix.data(), frames * s_outputChannels * sizeof(float));
        for (int i = 0; i < bytes / (int)sizeof(float); ++i)
            dst[i] += s_movieMix[i];
    }
    SDL_UnlockMutex(s_lock);
}

void SDLCALL Feed(void *, SDL_AudioStream *stream, int additional, int)
{
    int frames = additional / (int)(s_outputChannels * sizeof(float));
    if (frames <= 0)
        return;
    s_mix.assign((size_t)frames * s_outputChannels, 0.0f);
    MixInto(s_mix.data(), frames);
    for (float &v : s_mix)
        v = v > 1.0f ? 1.0f : (v < -1.0f ? -1.0f : v);
    SDL_PutAudioStreamData(stream, s_mix.data(), frames * s_outputChannels * (int)sizeof(float));
}

struct Lock {
    Lock() { if (s_lock) SDL_LockMutex(s_lock); }
    ~Lock() { if (s_lock) SDL_UnlockMutex(s_lock); }
};

} // namespace

extern "C" BOOL Audio_Init(int sampleRate, int channels)
{
    if (s_stream != NULL)
        return TRUE;
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        Sys_Log("audio: %s", SDL_GetError());
        return FALSE;
    }
    s_lock = SDL_CreateMutex();
    s_outputRate = sampleRate > 0 ? sampleRate : 44100;
    // Validate channel count: 2 (stereo), 4 (quad), 6 (5.1), 8 (7.1).
    int ch = s_outputChannels;
    if (ch != 2 && ch != 4 && ch != 6 && ch != 8)
        ch = 2;
    s_outputChannels = ch;
    SDL_AudioSpec spec = { SDL_AUDIO_F32, s_outputChannels, s_outputRate };
    s_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, Feed, NULL);
    if (s_stream == NULL) {
        Sys_Log("audio: %s", SDL_GetError());
        return FALSE;
    }
    if (!s_paused)
        SDL_ResumeAudioStreamDevice(s_stream);
    return TRUE;
}

extern "C" void Audio_SetPaused(BOOL paused)
{
    s_paused = paused != 0;
    if (s_stream == NULL)
        return;
    if (s_paused)
        SDL_PauseAudioStreamDevice(s_stream);
    else
        SDL_ResumeAudioStreamDevice(s_stream);
}

extern "C" int Audio_GetOutputRate(void)
{
    return s_stream != NULL ? s_outputRate : 0;
}

extern "C" int Audio_MixOffline(short *dst, int frames)
{
    // Offline export tap: renders `frames` output frames as 16-bit PCM.
    // Refuses while the live device is unpaused so the callback and this tap
    // can never advance the same buffer cursors concurrently; the caller must
    // Audio_SetPaused(1) first (pause stops device consumption itself).
    if (s_stream == NULL || !s_paused || dst == NULL || frames <= 0)
        return 0;
    std::vector<float> mix((size_t)frames * s_outputChannels, 0.0f);
    MixInto(mix.data(), frames);
    for (int i = 0; i < frames * s_outputChannels; ++i) {
        float v = mix[i] > 1.0f ? 1.0f : (mix[i] < -1.0f ? -1.0f : mix[i]);
        dst[i] = (short)(v * 32767.0f);
    }
    return frames;
}

extern "C" AudioStream *Audio_CreateStream(DWORD sampleRate, int channels)
{
    if (!sampleRate || (channels != 1 && channels != 2))
        return NULL;
    // Boot movies can precede the game's sound initialisation.
    if (!s_stream && !Audio_Init(44100, 2))
        return NULL;
    SDL_AudioSpec input = { SDL_AUDIO_F32, channels, (int)sampleRate };
    SDL_AudioSpec output = { SDL_AUDIO_F32, s_outputChannels, s_outputRate };
    SDL_AudioStream *converter = SDL_CreateAudioStream(&input, &output);
    if (!converter)
        return NULL;
    AudioStream *movie = new AudioStream{converter};
    Lock lock;
    s_movieStreams.push_back(movie);
    return movie;
}

extern "C" BOOL Audio_QueueStream(AudioStream *stream, const float *samples, DWORD count)
{
    if (!stream || count > 0x7fffffff / sizeof(float))
        return FALSE;
    Lock lock;
    return SDL_PutAudioStreamData(stream->converter, samples, count * sizeof(float));
}

extern "C" void Audio_StartStream(AudioStream *stream)
{
    if (!stream)
        return;
    Lock lock;
    stream->playing = true;
}

extern "C" void Audio_ReleaseStream(AudioStream *stream)
{
    if (!stream)
        return;
    Lock lock;
    auto it = std::find(s_movieStreams.begin(), s_movieStreams.end(), stream);
    if (it != s_movieStreams.end())
        s_movieStreams.erase(it);
    SDL_DestroyAudioStream(stream->converter);
    delete stream;
}

extern "C" void Audio_Shutdown(void)
{
    if (s_stream != NULL) {
        SDL_DestroyAudioStream(s_stream);
        s_stream = NULL;
    }
    // Buffers the game did not release keep their data until exit.
}

extern "C" AudioBuffer *Audio_CreateBuffer(DWORD sampleRate, int bits, int channels, DWORD bytes)
{
    if ((bits != 8 && bits != 16) || channels < 1 || channels > 2 || bytes == 0)
        return NULL;
    AudioData *d = new AudioData;
    d->bytes = (BYTE *)calloc(1, bytes);
    d->size = bytes;
    d->sampleRate = sampleRate;
    d->bits = bits;
    d->channels = channels;
    d->refs = 1;
    if (bits == 8)
        memset(d->bytes, 0x80, bytes);

    AudioBuffer *b = new AudioBuffer;
    b->data = d;
    b->frequency = sampleRate;
    Lock lock;
    s_buffers.push_back(b);
    return b;
}

extern "C" AudioBuffer *Audio_DuplicateBuffer(AudioBuffer *source)
{
    if (source == NULL)
        return NULL;
    Lock lock;
    AudioBuffer *b = new AudioBuffer;
    b->data = source->data;
    b->data->refs++;
    b->frequency = source->frequency;
    b->volume = source->volume;
    b->pan = source->pan;
    UpdateGains(b);
    s_buffers.push_back(b);
    return b;
}

extern "C" void Audio_ReleaseBuffer(AudioBuffer *buffer)
{
    if (buffer == NULL)
        return;
    Lock lock;
    for (size_t i = 0; i < s_buffers.size(); i++) {
        if (s_buffers[i] == buffer) {
            s_buffers.erase(s_buffers.begin() + i);
            break;
        }
    }
    if (--buffer->data->refs == 0) {
        free(buffer->data->bytes);
        delete buffer->data;
    }
    delete buffer;
}

extern "C" DWORD Audio_GetBufferSize(AudioBuffer *buffer)
{
    return buffer ? buffer->data->size : 0;
}

extern "C" void Audio_GetBufferFormat(AudioBuffer *buffer, DWORD *sampleRate, int *bits, int *channels)
{
    *sampleRate = buffer->data->sampleRate;
    *bits = buffer->data->bits;
    *channels = buffer->data->channels;
}

extern "C" BOOL Audio_LockBuffer(AudioBuffer *buffer, DWORD offset, DWORD bytes, void **p1, DWORD *n1, void **p2,
                                 DWORD *n2)
{
    if (buffer == NULL)
        return FALSE;
    AudioData *d = buffer->data;
    offset %= d->size;
    if (bytes > d->size)
        bytes = d->size;
    *p1 = d->bytes + offset;
    if (offset + bytes <= d->size) {
        *n1 = bytes;
        *p2 = NULL;
        *n2 = 0;
    } else {
        *n1 = d->size - offset;
        *p2 = d->bytes;
        *n2 = bytes - *n1;
    }
    return TRUE;
}

extern "C" void Audio_UnlockBuffer(AudioBuffer *)
{
    // The mixer reads the data in place, as DirectSound's software buffers.
}

extern "C" void Audio_Play(AudioBuffer *buffer, BOOL loop)
{
    if (buffer == NULL)
        return;
    Lock lock;
    buffer->loop = loop != 0;
    buffer->playing = true;
}

extern "C" void Audio_Stop(AudioBuffer *buffer)
{
    if (buffer == NULL)
        return;
    Lock lock;
    buffer->playing = false;
}

extern "C" BOOL Audio_IsPlaying(AudioBuffer *buffer)
{
    if (buffer == NULL)
        return FALSE;
    Lock lock;
    return buffer->playing;
}

extern "C" DWORD Audio_GetPlayPosition(AudioBuffer *buffer)
{
    if (buffer == NULL)
        return 0;
    Lock lock;
    return (DWORD)buffer->position * (DWORD)(buffer->data->bits / 8 * buffer->data->channels);
}

extern "C" void Audio_SetPlayPosition(AudioBuffer *buffer, DWORD position)
{
    if (buffer == NULL)
        return;
    Lock lock;
    buffer->position = (double)(position / (DWORD)(buffer->data->bits / 8 * buffer->data->channels));
}

extern "C" void Audio_SetVolume(AudioBuffer *buffer, LONG volume)
{
    if (buffer == NULL)
        return;
    Lock lock;
    buffer->volume = volume < AUDIO_VOLUME_MIN ? AUDIO_VOLUME_MIN : (volume > 0 ? 0 : volume);
    UpdateGains(buffer);
}

extern "C" void Audio_SetPan(AudioBuffer *buffer, LONG pan)
{
    if (buffer == NULL)
        return;
    Lock lock;
    buffer->pan = pan < AUDIO_PAN_LEFT ? AUDIO_PAN_LEFT : (pan > AUDIO_PAN_RIGHT ? AUDIO_PAN_RIGHT : pan);
    UpdateGains(buffer);
}

extern "C" void Audio_SetFrequency(AudioBuffer *buffer, DWORD frequency)
{
    if (buffer == NULL)
        return;
    Lock lock;
    buffer->frequency = frequency == 0 ? buffer->data->sampleRate : frequency;
}

extern "C" BOOL Audio_SetOutputChannels(int channels)
{
    if (channels != 2 && channels != 4 && channels != 6 && channels != 8)
        return FALSE;
    s_outputChannels = channels;
    return TRUE;
}

extern "C" int Audio_GetOutputChannels(void)
{
    return s_outputChannels;
}

extern "C" void Audio_SetPosition(AudioBuffer *buffer, float x, float y, float z)
{
    if (buffer == NULL)
        return;
    Lock lock;
    buffer->posX = x;
    buffer->posY = y;
    buffer->posZ = z;
    buffer->spatial = true;
}

extern "C" void Audio_GetPosition(AudioBuffer *buffer, float *x, float *y, float *z)
{
    if (buffer == NULL) {
        if (x) *x = 0.0f;
        if (y) *y = 0.0f;
        if (z) *z = 0.0f;
        return;
    }
    Lock lock;
    if (x) *x = buffer->posX;
    if (y) *y = buffer->posY;
    if (z) *z = buffer->posZ;
}

extern "C" void Audio_SetListenerPosition(float x, float y, float z)
{
    Lock lock;
    s_listenerX = x;
    s_listenerY = y;
    s_listenerZ = z;
}

extern "C" void Audio_SetListenerOrientation(float fx, float fy, float fz,
                                             float ux, float uy, float uz)
{
    Lock lock;
    s_listenerFX = fx;
    s_listenerFY = fy;
    s_listenerFZ = fz;
    s_listenerUX = ux;
    s_listenerUY = uy;
    s_listenerUZ = uz;
}

extern "C" void Audio_GetListenerPosition(float *x, float *y, float *z)
{
    Lock lock;
    if (x) *x = s_listenerX;
    if (y) *y = s_listenerY;
    if (z) *z = s_listenerZ;
}

extern "C" void Audio_GetListenerOrientation(float *fx, float *fy, float *fz,
                                             float *ux, float *uy, float *uz)
{
    Lock lock;
    if (fx) *fx = s_listenerFX;
    if (fy) *fy = s_listenerFY;
    if (fz) *fz = s_listenerFZ;
    if (ux) *ux = s_listenerUX;
    if (uy) *uy = s_listenerUY;
    if (uz) *uz = s_listenerUZ;
}

extern "C" void Audio_SetMinDistance(AudioBuffer *buffer, float distance)
{
    if (buffer == NULL)
        return;
    Lock lock;
    buffer->minDistance = distance;
}

extern "C" void Audio_SetMaxDistance(AudioBuffer *buffer, float distance)
{
    if (buffer == NULL)
        return;
    Lock lock;
    buffer->maxDistance = distance;
}

extern "C" void Audio_SetRolloffFactor(AudioBuffer *buffer, float rolloff)
{
    if (buffer == NULL)
        return;
    Lock lock;
    buffer->rolloff = rolloff;
}

extern "C" void Audio_GetMinDistance(AudioBuffer *buffer, float *distance)
{
    if (buffer == NULL) {
        if (distance) *distance = 0.0f;
        return;
    }
    Lock lock;
    if (distance) *distance = buffer->minDistance;
}

extern "C" void Audio_GetMaxDistance(AudioBuffer *buffer, float *distance)
{
    if (buffer == NULL) {
        if (distance) *distance = 0.0f;
        return;
    }
    Lock lock;
    if (distance) *distance = buffer->maxDistance;
}

extern "C" void Audio_GetRolloffFactor(AudioBuffer *buffer, float *rolloff)
{
    if (buffer == NULL) {
        if (rolloff) *rolloff = 0.0f;
        return;
    }
    Lock lock;
    if (rolloff) *rolloff = buffer->rolloff;
}

// ---- MS-ADPCM ---------------------------------------------------------------------

extern "C" DWORD Audio_DecodeMsAdpcm(const AudioWaveFormat *format, const BYTE *src, DWORD srcBytes, BYTE *dst,
                                     DWORD dstBytes)
{
    static const int adaptation[16] = { 230, 230, 230, 230, 307, 409, 512, 614,
                                        768, 614, 512, 409, 307, 230, 230, 230 };
    static const short standardCoefs[7][2] = { { 256, 0 }, { 512, -256 }, { 0, 0 }, { 192, 64 },
                                               { 240, 0 }, { 460, -208 }, { 392, -232 } };
    short coefs[32][2];
    int coefCount = 7;
    int channels = format->nChannels;
    DWORD blockAlign = format->nBlockAlign;
    DWORD written = 0;

    memcpy(coefs, standardCoefs, sizeof(standardCoefs));
    if (format->cbSize >= 4) {
        const BYTE *extra = (const BYTE *)format + sizeof(AudioWaveFormat);
        WORD count = (WORD)(extra[2] | extra[3] << 8);
        if (count >= 7 && count <= 32 && format->cbSize >= 4 + count * 4) {
            coefCount = count;
            for (int i = 0; i < count; i++) {
                coefs[i][0] = (short)(extra[4 + i * 4] | extra[5 + i * 4] << 8);
                coefs[i][1] = (short)(extra[6 + i * 4] | extra[7 + i * 4] << 8);
            }
        }
    }
    if (channels < 1 || channels > 2 || blockAlign < (DWORD)(7 * channels))
        return 0;

    Sint16 *out = (Sint16 *)dst;
    DWORD outMax = dstBytes / 2;
    for (DWORD offset = 0; offset + blockAlign <= srcBytes; offset += blockAlign) {
        const BYTE *p = src + offset;
        int predictor[2], delta[2], s1[2], s2[2];
        for (int c = 0; c < channels; c++)
            predictor[c] = p[c] < coefCount ? p[c] : 0;
        p += channels;
        for (int c = 0; c < channels; c++, p += 2)
            delta[c] = (Sint16)(p[0] | p[1] << 8);
        for (int c = 0; c < channels; c++, p += 2)
            s1[c] = (Sint16)(p[0] | p[1] << 8);
        for (int c = 0; c < channels; c++, p += 2)
            s2[c] = (Sint16)(p[0] | p[1] << 8);
        // The header's two samples come out oldest first.
        for (int c = 0; c < channels && written < outMax; c++)
            out[written++] = (Sint16)s2[c];
        for (int c = 0; c < channels && written < outMax; c++)
            out[written++] = (Sint16)s1[c];
        const BYTE *end = src + offset + blockAlign;
        int channel = 0;
        for (; p < end && written < outMax; p++) {
            for (int half = 0; half < 2 && written < outMax; half++) {
                int nibble = half == 0 ? p[0] >> 4 : p[0] & 0xf;
                int signedNibble = nibble >= 8 ? nibble - 16 : nibble;
                int c = channel;
                int prediction = (s1[c] * coefs[predictor[c]][0] + s2[c] * coefs[predictor[c]][1]) / 256;
                int sample = prediction + signedNibble * delta[c];
                sample = sample > 32767 ? 32767 : (sample < -32768 ? -32768 : sample);
                out[written++] = (Sint16)sample;
                s2[c] = s1[c];
                s1[c] = sample;
                delta[c] = delta[c] * adaptation[nibble] / 256;
                if (delta[c] < 16)
                    delta[c] = 16;
                channel = (channel + 1) % channels;
            }
        }
    }
    return written * 2;
}
