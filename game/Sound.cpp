#include <math.h>
#include "FloatToInt.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Graphics.h"
#include "Sprite.h"
#include "Game.h"
#include "Sound.h"
#include "PcmWaveHeader.h"
#include "main.h"
#include "InstallInfo.h"
#include "GameInfo.h"
#include "FileBuffer.h"

SoundSlot *CSound::m_soundSlots[32];
BOOL CSound::m_unk0x006e0eec;
SoundSlot *CSound::m_soundSlotsEnd;

BOOL CSound::m_unk0x005a2728;
BOOL CSound::m_unk0x005a272c;
BOOL CSound::m_unk0x005a2730;
MMIOData *CSound::m_pMMIO;
AudioBuffer* CSound::m_pDirectSoundBuffer;
int CSound::m_unk0x00816a7c;
BOOL CSound::m_unk0x005a2720;
BOOL CSound::m_unk0x005a2724;
BOOL CSound::m_unk0x005a2734 = FALSE;
char CSound::m_unk0x005a2738[256];

// GLOBAL: CMR2 0x00520a3c
char g_strCouldNotCreateStreamingBuffer[36] = "Could not create streaming buffer";
// GLOBAL: CMR2 0x00520a60
char g_strCouldNotOpenAdpcm[44] = "Could not open Microsoft ADPCM Audio CODEC";
// GLOBAL: CMR2 0x00520a8c
char g_strCouldNotOpenMusicFile[28] = "Could not open music file";

BOOL Sound_CreateMusicStreamingBuffer(void);
BOOL Sound_OpenADPCMDecoder(void);
void Sound_NoOpMusicCallback(INT_PTR unused);
HRESULT Sound_StartLoopingMusicStream(int param1);

// Opens a music file (.wav with Microsoft ADPCM data) and prepares it for
// streaming: creates the streaming buffer and the ACM decoder; on failure the
// music is stopped again. The name is remembered in m_unk0x005a2738.
// match 74%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004a28d0
void CSound::OpenStreamingMusicFile(char *path) {
    if (m_unk0x006e0eec == 0)
        return;

    m_unk0x005a2728 = FALSE;
    m_unk0x005a272c = FALSE;
    m_unk0x005a2730 = FALSE;
    if (m_pMMIO != NULL) {
        CloseMMIO(m_pMMIO);
        if (m_pMMIO != NULL) {
            MMIOData *pMMIO = m_pMMIO;
            CloseAndCleanupMMIO(m_pMMIO);
            delete pMMIO;
            m_pMMIO = NULL;
        }
    }

    m_pMMIO = new MMIOData();
    if (m_pMMIO->Open(path) != 0) {
        Sound_NoOpMusicCallback((INT_PTR)g_strCouldNotOpenMusicFile);
    } else {
        m_unk0x005a2728 = TRUE;
        if (IsSoundCallSuccessful(Sound_CreateMusicStreamingBuffer()) != 0) {
            m_unk0x005a272c = TRUE;
            if (m_unk0x005a2734 == FALSE)
                CGame::RegisterCallback((void *)CloseMusicStreamResources, NULL);
            if (Sound_OpenADPCMDecoder() == 1) {
                m_unk0x005a2730 = TRUE;
                m_unk0x005a2734 = TRUE;
                strcpy(m_unk0x005a2738, path);
            } else {
                Sound_NoOpMusicCallback((INT_PTR)g_strCouldNotOpenAdpcm);
            }
        } else {
            Sound_NoOpMusicCallback((INT_PTR)g_strCouldNotCreateStreamingBuffer);
        }
    }
    if (m_unk0x005a2730 == FALSE)
        CloseMusicStreamAndClearPath(FALSE);
}
// FUNCTION: CMR2 0x004a2b50
void CSound::CloseMusicStreamAndClearPath(BOOL param1) {
    CloseMusicStreamResources();
    if (param1 == 0) {
        m_unk0x005a2734 = FALSE;
        strcpy(m_unk0x005a2738, CMain::m_logFileBlankLine);
    }
}

// FUNCTION: CMR2 0x004a2ac0
BOOL __fastcall CSound::CloseMusicStreamResources(void) {
    MMIOData* pvVar1;

    if (m_unk0x005a2728 != 0) {
        if (m_unk0x005a2730 != 0) {
            CloseADPCMDecoder();
        }

        if (m_unk0x005a272c != 0) {
            StopDirectSoundBuffer();
            if (m_pDirectSoundBuffer != NULL) {
                Audio_ReleaseBuffer(m_pDirectSoundBuffer);
            }

            m_pDirectSoundBuffer = NULL;
        }

        CloseMMIO(m_pMMIO);

        if (m_pMMIO != NULL) {
            pvVar1 = m_pMMIO;
            CloseAndCleanupMMIO(m_pMMIO);
            delete pvVar1;
        }

        m_pMMIO = NULL;
        m_unk0x005a2728 = FALSE;
        m_unk0x005a272c = FALSE;
        m_unk0x005a2730 = FALSE;
    }

    return TRUE;
}

// Converts one 16 KB block of the compressed music stream into pDst.
// FUNCTION: CMR2 0x004bd1b0
// PORT: decoded with the built-in MS-ADPCM decoder (the original used ACM)
// in the format of the open music file.
BOOL Sound_DecodeADPCMBlock(BYTE *pSrc, BYTE *pDst)
{
    if (!CSound::m_unk0x00816a7c || CSound::m_pMMIO == NULL || CSound::m_pMMIO->pBuffer == NULL)
        return FALSE;
    return Audio_DecodeMsAdpcm(CSound::m_pMMIO->pBuffer, pSrc, 0x4000, pDst, 0xfe80) != 0;
}

// FUNCTION: CMR2 0x004bd230
bool CSound::CloseADPCMDecoder(void) {
    m_unk0x00816a7c = 0;
    return true;
}

// FUNCTION: CMR2 0x004a2f00
HRESULT CSound::StopDirectSoundBuffer(void) {
    if (m_unk0x005a2730 != 0) {
        if (m_pDirectSoundBuffer != NULL) {
            if (Audio_IsPlaying(m_pDirectSoundBuffer))
                Audio_Stop(m_pDirectSoundBuffer);
        }

        m_unk0x005a2720 = FALSE;
    }

    return 0;
}


// FUNCTION: CMR2 0x004a3250
BOOL CSound::IsSoundCallSuccessful(HRESULT param_1) {
    return param_1 >= 0;
}

// FUNCTION: CMR2 0x004bd960
UINT __fastcall CSound::CloseMMIO(MMIOData* pMMIOData) {
    Sys_CloseFile(pMMIOData->hmmio);
    pMMIOData->hmmio = NULL;
    return 0;
}

// FUNCTION: CMR2 0x004bd8c0
void __fastcall CSound::CloseAndCleanupMMIO(MMIOData* pMMIOData) {
    CloseMMIO(pMMIOData);
    
    if (pMMIOData->pBuffer != NULL) {
        delete[] (BYTE *)pMMIOData->pBuffer;
        pMMIOData->pBuffer = NULL;
    }
}

// FUNCTION: CMR2 0x004a31f0
void CSound::SetMusicStreamVolume(int volume)
{
    LONG vol;

    if (m_unk0x005a2730 != 0) {
        if (volume == 0)
            vol = AUDIO_VOLUME_MIN;
        else
            vol = (volume - 100) * 100 / 4;
        Audio_SetVolume(m_pDirectSoundBuffer, vol);
    }
}

// PORT: AcmFormatEnumCallback (0x004bd250) and AcmDriverEnumCallback
// (0x004bd2b0) were ACM enumeration callbacks; the decoder is built in.

// FUNCTION: CMR2 0x004bd3c0
// PORT: the built-in decoder handles MS-ADPCM (tag 2); its "driver" is 1.
int AcmFindDriver(WORD wFormatTag)
{
    return wFormatTag == AUDIO_WAVE_FORMAT_ADPCM || wFormatTag == AUDIO_WAVE_FORMAT_PCM;
}

// FUNCTION: CMR2 0x004bd400
// PORT: the 44.1 kHz stereo format the original asked the driver for.
AudioWaveFormat *AcmGetDriverFormat(int hadid, WORD wFormatTag)
{
    AudioWaveFormat *pwfx;

    if (!hadid)
        return NULL;
    pwfx = (AudioWaveFormat *)malloc(sizeof(AudioWaveFormat));
    memset(pwfx, 0, sizeof(AudioWaveFormat));
    pwfx->wFormatTag = wFormatTag;
    pwfx->nChannels = 2;
    pwfx->nSamplesPerSec = 44100;
    pwfx->wBitsPerSample = 16;
    pwfx->nBlockAlign = 4;
    pwfx->nAvgBytesPerSec = 44100 * 4;
    return pwfx;
}

// FUNCTION: CMR2 0x004bd520
// PORT: RIFF parsing on a Sys file (mmioDescend/mmioRead in the original).
static BOOL Wave_FindChunk(SysFile *file, DWORD id, DWORD start, DWORD end, WaveChunk *pChunk)
{
    DWORD header[2];
    DWORD pos = start;

    while (pos + 8 <= end) {
        if (Sys_SeekFile(file, pos, SYS_SEEK_SET) == 0xffffffff || Sys_ReadFile(file, header, 8) != 8)
            return FALSE;
        if (header[0] == id) {
            pChunk->ckid = header[0];
            pChunk->cksize = header[1];
            pChunk->dwDataOffset = pos + 8;
            pChunk->fccType = 0;
            pChunk->dwFlags = 0;
            return TRUE;
        }
        pos += 8 + ((header[1] + 1) & ~1u);
    }
    return FALSE;
}

HRESULT ReadMMIO(SysFile *hmmioIn, WaveChunk *pckInRIFF, AudioWaveFormat **ppwfxInfo)
{
    DWORD riff[3];
    WaveChunk ckIn;
    AudioWaveFormat format;
    WORD cbExtraBytes;

    *ppwfxInfo = NULL;

    if (Sys_SeekFile(hmmioIn, 0, SYS_SEEK_SET) != 0 || Sys_ReadFile(hmmioIn, riff, 12) != 12)
        return -1;
    if (riff[0] != MAKEFOURCC('R', 'I', 'F', 'F') || riff[2] != MAKEFOURCC('W', 'A', 'V', 'E'))
        return -1;
    pckInRIFF->ckid = riff[0];
    pckInRIFF->cksize = riff[1];
    pckInRIFF->fccType = riff[2];
    pckInRIFF->dwDataOffset = 8;
    pckInRIFF->dwFlags = 0;

    if (!Wave_FindChunk(hmmioIn, MAKEFOURCC('f', 'm', 't', ' '), 12, 8 + pckInRIFF->cksize, &ckIn))
        return -1;
    if (ckIn.cksize < 16)
        return -1;

    memset(&format, 0, sizeof(format));
    if (Sys_ReadFile(hmmioIn, &format, 16) != 16)
        return -1;

    cbExtraBytes = 0;
    if (format.wFormatTag != AUDIO_WAVE_FORMAT_PCM && ckIn.cksize >= 18) {
        if (Sys_ReadFile(hmmioIn, &cbExtraBytes, sizeof(WORD)) != sizeof(WORD))
            return -1;
    }
    *ppwfxInfo = (AudioWaveFormat *)new BYTE[sizeof(AudioWaveFormat) + cbExtraBytes];
    memcpy(*ppwfxInfo, &format, 16);
    (*ppwfxInfo)->cbSize = cbExtraBytes;
    if (cbExtraBytes != 0 && Sys_ReadFile(hmmioIn, (BYTE *)*ppwfxInfo + sizeof(AudioWaveFormat), cbExtraBytes) != cbExtraBytes) {
        delete[] (BYTE *)*ppwfxInfo;
        *ppwfxInfo = NULL;
        return -1;
    }
    return 0;
}

// FUNCTION: CMR2 0x004bd6b0
HRESULT WaveOpenFile(LPSTR strFileName, SysFile **phmmioIn, AudioWaveFormat **ppwfxInfo, WaveChunk *pckInRIFF)
{
    HRESULT hr;
    SysFile *hmmioIn;

    hmmioIn = Sys_OpenFile(strFileName, SYS_FILE_READ);
    while (hmmioIn == NULL) {
        if (!CInstallInfo::ShowNoCDErrorMessage())
            return -1;
        hmmioIn = Sys_OpenFile(strFileName, SYS_FILE_READ);
    }

    hr = ReadMMIO(hmmioIn, pckInRIFF, ppwfxInfo);
    if (hr < 0) {
        Sys_CloseFile(hmmioIn);
        return hr;
    }

    *phmmioIn = hmmioIn;
    return 0;
}

// FUNCTION: CMR2 0x004bd740
HRESULT WaveStartDataRead(SysFile **phmmioIn, WaveChunk *pckIn, WaveChunk *pckInRIFF, DWORD *pdwSize)
{
    if (!Wave_FindChunk(*phmmioIn, MAKEFOURCC('d', 'a', 't', 'a'), 12, 8 + pckInRIFF->cksize, pckIn))
        return -1;
    if (Sys_SeekFile(*phmmioIn, pckIn->dwDataOffset, SYS_SEEK_SET) == 0xffffffff)
        return -1;

    *pdwSize = pckIn->cksize;
    return 0;
}

// FUNCTION: CMR2 0x004bd7b0
HRESULT WaveReadFile(SysFile *hmmioIn, UINT cbRead, BYTE *pbDest, WaveChunk *pckIn, UINT *cbActualRead)
{
    UINT cbDataIn;

    *cbActualRead = 0;

    cbDataIn = cbRead;
    if (cbDataIn > pckIn->cksize)
        cbDataIn = pckIn->cksize;

    pckIn->cksize -= cbDataIn;

    if (Sys_ReadFile(hmmioIn, pbDest, cbDataIn) != cbDataIn)
        return -1;

    *cbActualRead = cbDataIn;
    return 0;
}

// FUNCTION: CMR2 0x004bd8b0
MMIOData::MMIOData()
{
    pBuffer = NULL;
    hmmio = NULL;
}

// FUNCTION: CMR2 0x004bd8e0
HRESULT MMIOData::Open(LPSTR strFileName)
{
    HRESULT hr;

    if (pBuffer != NULL) {
        delete[] (BYTE *)pBuffer;
        pBuffer = NULL;
    }

    hr = WaveOpenFile(strFileName, &hmmio, &pBuffer, &ckRiff);
    if (SUCCEEDED(hr))
        hr = StartDataRead();
    return hr;
}

// FUNCTION: CMR2 0x004bd920
HRESULT MMIOData::StartDataRead(void)
{
    return WaveStartDataRead(&hmmio, &ck, &ckRiff, &dwSize);
}

// FUNCTION: CMR2 0x004bd940
HRESULT MMIOData::Read(UINT cbRead, BYTE *pbDest, UINT *pcbRead)
{
    return WaveReadFile(hmmio, cbRead, pbDest, &ck, pcbRead);
}

// Releases the sound data held by one slot and clears the slot.
// FUNCTION: CMR2 0x004b7620
void CSound::ReleaseSoundSlotData(int index)
{
    CFileBuffer::FreeGenericFileBuffer(m_soundSlots[index]);
    m_soundSlots[index] = NULL;
}

// Polls the buffer status and (re)starts it with the given play flags.
// FUNCTION: CMR2 0x004a23f0
void CSound::EnsureBufferPlaying(AudioBuffer *pBuffer, int flags)
{
    Audio_Play(pBuffer, (flags & 1) != 0);
}

// Drops a finished sound slot: restarts looping sounds, otherwise releases the
// buffer once the slot stops asking for it.
// FUNCTION: CMR2 0x004a27c0
void CSound::UpdateFinishedSoundSlot(SoundSlot *pSlot)
{
    AudioBuffer *pBuffer;

    pBuffer = pSlot->pBuffer;
    if (pBuffer != NULL) {
        if (Audio_IsPlaying(pBuffer))
            return;
        if (pSlot->field_0x30 != 0) {
            EnsureBufferPlaying(pSlot->pLoopBuffer, 1);
            return;
        }
        if (pSlot->field_0x2c != 0) {
            Audio_ReleaseBuffer(pSlot->pBuffer);
            pSlot->pBuffer = NULL;
        }
        ReleaseSoundSlotData(pSlot->id);
    }
}

// FUNCTION: CMR2 0x004a28c0
void CSound::NoOpSoundDeviceCallback(void)
{
}

// FUNCTION: CMR2 0x004b7b10
void CSound::RunSoundDeviceCallback(void)
{
    NoOpSoundDeviceCallback();
}

// Stops the shared DirectSound buffer when it is still playing.
// FUNCTION: CMR2 0x004a31a0
void CSound::StopSharedMusicBuffer(void)
{
    if (m_unk0x005a2728 != 0 && m_pDirectSoundBuffer != NULL) {
        if (Audio_IsPlaying(m_pDirectSoundBuffer)) {
            Audio_Stop(m_pDirectSoundBuffer);
            m_unk0x005a2720 = 1;
        }
    }
}

// GLOBAL: CMR2 0x00816978
int g_unk0x00816978;

// FUNCTION: CMR2 0x004bd100
BOOL Sound_FindADPCMDriver(void)
{
    int id;

    id = AcmFindDriver(2);
    g_unk0x00816978 = id;
    return id != 0;
}

// Opens the ACM stream that decodes the ADPCM music into PCM.
// FUNCTION: CMR2 0x004bd120
// PORT: checks the music file is MS-ADPCM, which the built-in decoder reads.
BOOL Sound_OpenADPCMDecoder(void)
{
    CSound::m_unk0x00816a7c = 0;
    if (!g_unk0x00816978 || CSound::m_pMMIO == NULL || CSound::m_pMMIO->pBuffer == NULL ||
        CSound::m_pMMIO->pBuffer->wFormatTag != AUDIO_WAVE_FORMAT_ADPCM)
        return FALSE;
    CSound::m_unk0x00816a7c = 1;
    return TRUE;
}

// FUNCTION: CMR2 0x004a3160
void CSound::PauseMusicStreaming(void)
{
    if (m_unk0x005a2730 != 0) {
        m_unk0x005a2724 = 1;
        StopSharedMusicBuffer();
    }
}

// GLOBAL: CMR2 0x005a2710
int g_unk0x005a2710;
// GLOBAL: CMR2 0x005a2714
int g_unk0x005a2714;
// GLOBAL: CMR2 0x005a2718
int g_unk0x005a2718;

// FUNCTION: CMR2 0x004a2d30
void Sound_UpdateMusicRingBufferCursor(void)
{
    unsigned int v;

    v = Audio_GetPlayPosition(CSound::m_pDirectSoundBuffer) / 0xfe80u;
    g_unk0x005a2710 = (int)v;
    v -= g_unk0x005a2714;
    if ((int)v > 0)
        g_unk0x005a2718 = (int)v;
    else
        g_unk0x005a2718 = (int)v + 8;
    g_unk0x005a2718--;
}

extern int g_unk0x005a271c;

// Decodes `count` 16 KB blocks of the music file into pDst (0xfe80 bytes of
// PCM each). At the end of the file the block is padded to the ADPCM block size
// and the file is rewound, so the music loops.
// FUNCTION: CMR2 0x004a2d90
HRESULT Sound_DecodeMusicBlocks(BYTE *pDst, int count)
{
    BYTE buffer[0x4000];
    UINT read;
    BYTE *pOut;
    UINT pos;
    UINT pad;
    UINT blocks;
    HRESULT hr;
    int i;

    for (i = 0; i < count; i++) {
        pOut = pDst + i * 0xfe80;
        hr = CSound::m_pMMIO->Read(0x4000, buffer, &read);
        if (hr != 0)
            return hr;
        if (read < 0x4000) {
            pos = read;
            do {
                pad = 0x800 - (read & 0x7ff);
                memset(buffer + read, 0, pad);
                pos += pad;
                Sound_DecodeADPCMBlock(buffer, pOut);
                blocks = FTOI(UINT, X87S_MUL(X87S_LOAD(read), (1.0f / 2048.0f)));
                CSound::IsSoundCallSuccessful(CSound::m_pMMIO->StartDataRead());
                CSound::IsSoundCallSuccessful(CSound::m_pMMIO->Read(0x4000 - pos, buffer + pos, &read));
                Sound_DecodeADPCMBlock(buffer, pDst + (blocks + 0xfe80) * i);
                pos += read;
            } while (pos < 0x4000);
        } else {
            Sound_DecodeADPCMBlock(buffer, pOut);
        }
        g_unk0x005a2714++;
        if (g_unk0x005a2714 == 8)
            g_unk0x005a2714 = 0;
    }
    return 0;
}

// Rewinds the music file and fills the whole streaming buffer from the start.
// FUNCTION: CMR2 0x004a2c70
HRESULT Sound_RewindAndFillMusicBuffer(int unused)
{
    void *pAudio1 = NULL;
    void *pAudio2 = NULL;
    DWORD bytes1;
    DWORD bytes2;

    if (CSound::m_pDirectSoundBuffer == NULL)
        return -1;
    CSound::m_pMMIO->StartDataRead();
    Audio_SetPlayPosition(CSound::m_pDirectSoundBuffer, 0);
    Sound_UpdateMusicRingBufferCursor();
    Audio_LockBuffer(CSound::m_pDirectSoundBuffer, 0, g_unk0x005a271c, &pAudio1, &bytes1, &pAudio2, &bytes2);
    CSound::IsSoundCallSuccessful(Sound_DecodeMusicBlocks((BYTE *)pAudio1, 8));
    Audio_UnlockBuffer(CSound::m_pDirectSoundBuffer);
    return 0;
}

// Restarts playing the currently opened music stream if one is open.
// FUNCTION: CMR2 0x004a2f50
int Sound_RestartOpenedMusicStream(void)
{
    if (CSound::m_unk0x005a2730 != 0) {
        CSound::StopDirectSoundBuffer();
        Sound_StartLoopingMusicStream(1);
    }
    return 0;
}

// Restores the streaming buffer if it was lost, then refills it.
// FUNCTION: CMR2 0x004a2f70
// PORT: renderer-independent audio buffers are never lost.
HRESULT Sound_RestoreLostMusicBuffer(int param1)
{
    return 0;
}

// GLOBAL: CMR2 0x00520aa8
char g_strCouldNotPlayMusicFile[28] = "Could not play music file";
// GLOBAL: CMR2 0x00520ac4
char g_strCouldNotFillMusicBuffer[28] = "Could not fill music buffer";
// GLOBAL: CMR2 0x00520ae0
char g_strCouldNotRestoreMusicBuffer[32] = "Could not restore music buffer";

void Sound_NoOpMusicCallback(INT_PTR unused);

// Starts playing the opened music from the beginning, looping.
// FUNCTION: CMR2 0x004a2bd0
HRESULT Sound_StartLoopingMusicStream(int param1)
{
    if (CSound::m_unk0x005a2730 != 0) {
        g_unk0x005a2710 = 0;
        g_unk0x005a2714 = 0;
        CSound::m_unk0x005a2720 = FALSE;
        CSound::m_unk0x005a2724 = FALSE;
        if (CSound::m_pDirectSoundBuffer == NULL)
            return -1;
        if (CSound::IsSoundCallSuccessful(Sound_RestoreLostMusicBuffer(param1)) == 0)
            Sound_NoOpMusicCallback((INT_PTR)g_strCouldNotRestoreMusicBuffer);
        if (CSound::IsSoundCallSuccessful(Sound_RewindAndFillMusicBuffer(param1)) == 0)
            Sound_NoOpMusicCallback((INT_PTR)g_strCouldNotFillMusicBuffer);
        Audio_Play(CSound::m_pDirectSoundBuffer, TRUE);
    }
    return 0;
}

// One chunk (0xfe80 bytes) as a 16.16 fraction; the original's constant block.
// GLOBAL: CMR2 0x00511420
extern const float g_floatOneOver65152 = 1.0f / 0xfe80;

// Refills the part of the streaming buffer that has already been played.

// FUNCTION: CMR2 0x004a3050
HRESULT Sound_RefillMusicBufferRegions(int unused)
{
    void *pAudio1 = NULL;
    void *pAudio2 = NULL;
    DWORD bytes2;
    DWORD bytes1;

    Sound_UpdateMusicRingBufferCursor();
    if (g_unk0x005a2718 > 0) {
        if (Audio_LockBuffer(CSound::m_pDirectSoundBuffer, g_unk0x005a2714 * 0xfe80, g_unk0x005a2718 * 0xfe80,
                             &pAudio1, &bytes1, &pAudio2, &bytes2)) {
            if (pAudio1 != NULL) {
                UINT n1 = FTOI(UINT, X87S_MUL(X87S_LOAD(bytes1), g_floatOneOver65152));
                CSound::IsSoundCallSuccessful(Sound_DecodeMusicBlocks((BYTE *)pAudio1, n1));
            }
            if (pAudio2 != NULL) {
                UINT n2 = FTOI(UINT, X87S_MUL(X87S_LOAD(bytes2), g_floatOneOver65152));
                CSound::IsSoundCallSuccessful(Sound_DecodeMusicBlocks((BYTE *)pAudio2, n2));
            }
            Audio_UnlockBuffer(CSound::m_pDirectSoundBuffer);
        }
    }
    return 0;
}

// Per-frame music update: keeps the streaming buffer filled while it plays,
// or restarts it after a pause.
// FUNCTION: CMR2 0x004a2fe0
void Sound_UpdateMusicStreaming(void)
{
    if (CSound::m_unk0x005a2724 == 0 && CSound::m_unk0x005a2730 != 0) {
        if (CSound::m_unk0x005a2720 == 0) {
            if (Audio_IsPlaying(CSound::m_pDirectSoundBuffer)) {
                Sound_RefillMusicBufferRegions(1);
                CSound::m_unk0x005a2720 = FALSE;
                return;
            }
        } else {
            Audio_Play(CSound::m_pDirectSoundBuffer, TRUE);
        }
        CSound::m_unk0x005a2720 = FALSE;
    }
}

// FUNCTION: CMR2 0x004a2430
int Sound_IsSlotPlayingOrPending(SoundSlot *pSlot)
{
    int result;

    result = 1;
    if (!Audio_IsPlaying(pSlot->pBuffer)) {
        if (pSlot->field_0x30 == 0)
            result = 0;
    }
    return result;
}

// Per-sample 3D interfaces and buffers loaded by the sound bank.
// PORT: the game never places 3D sounds, so they are plain buffers and the
// 3D interfaces are gone.
// GLOBAL: CMR2 0x005a1fc8
AudioBuffer *g_sound3DBuffers[200];
// GLOBAL: CMR2 0x005a23ec
AudioBuffer *g_soundBuffers[200];
// GLOBAL: CMR2 0x005a283c
BOOL g_unk0x005a283c;

extern int g_unk0x005a2844;
void Sound_ApplySlotFrequency(SoundSlot *pSlot);
BOOL Sound_IsModernWindowsVersion(void);
int Sound_GetMasterVolume(void);
SoundSlot *Sound_GetSlot(int index);

// The original imported DirectSoundCreate from DSOUND.dll (the SilentPatch exe
// routes it through SPCMR2.dll ordinal 1).

// 3D sound enabled (primary buffer with CTRL3D and a listener)
// GLOBAL: CMR2 0x005a2838
BOOL g_sound3DEnabled;
// Speaker configuration read from DirectSound (DSSPEAKER_*)
// GLOBAL: CMR2 0x00520890
DWORD g_soundSpeakerConfig = 4;     // DSSPEAKER_STEREO

extern int g_unk0x005a2848;
extern int g_unk0x005a284c;
BOOL Sound_FindADPCMDriver(void);

// Creates the DirectSound device and sets the format of the primary buffer
// (16-bit PCM at sampleRate; mono when the speakers are mono). bits and
// unused are ignored: the original always uses 16 bits.
// FUNCTION: CMR2 0x004a1d60
// PORT: opens the audio output (stereo; there is no speaker query).
BOOL Sound_InitDevice(int sampleRate, int channels, int bits, int unused)
{
    if (channels == 1)
        g_sound3DEnabled = FALSE;
    if (!Audio_Init(sampleRate, channels))
        return FALSE;
    g_unk0x005a2844 = 1;
    if (g_sound3DEnabled)
        g_unk0x005a284c = 1;

    Sound_FindADPCMDriver();
    CSound::m_unk0x005a2734 = FALSE;
    strcpy(CSound::m_unk0x005a2738, CMain::m_logFileBlankLine);
    CSound::m_pMMIO = NULL;
    return TRUE;
}

// GLOBAL: CMR2 0x00520a34
char g_strWave[8] = "WAVE";

int Sound_GetLoadedSampleCount(void);
BOOL Sound_CreatePcmSampleBuffer(int device, AudioBuffer **ppBuffer, DWORD rate, int bits, int channels, int is3D,
                  DWORD size);
BOOL Sound_CopyBufferData(AudioBuffer *pBuffer, DWORD offset, void *pData, DWORD size);

// Loads a .wav from pFile into the next free sample slot: creates its buffer
// (a 3D one when flags & 1 and 3D sound is on) and copies the PCM data.
// The file buffer is freed unless it lives inside the archive.
// FUNCTION: CMR2 0x004a1f50
BOOL Sound_LoadWave(char *name, BYTE flags, GenericFile *pFile)
{
    PcmWaveHeader *pWave;
    BYTE inArchive;
    int channels, bits;
    DWORD rate;
    BYTE *pData;
    int slot;

    pWave = (PcmWaveHeader *)CGenericFileLoader::FindFile(pFile, name, &inArchive, NULL, 0);
    if (pWave == NULL)
        return FALSE;

    if (strncmp(pWave->waveId, g_strWave, 4) == 0) {
        channels = pWave->channels;
        rate = pWave->sampleRate;
        bits = pWave->bitsPerSample;
        pData = (BYTE *)(pWave + 1);
        if ((flags & 1) == 0 || !g_sound3DEnabled) {
            if (!Sound_CreatePcmSampleBuffer(g_unk0x005a2844, &g_soundBuffers[Sound_GetLoadedSampleCount()], rate, bits, channels, 0,
                              pWave->dataSize))
                return FALSE;
            Sound_CopyBufferData(g_soundBuffers[Sound_GetLoadedSampleCount()], 0, pData, pWave->dataSize);
        } else {
            if (!Sound_CreatePcmSampleBuffer(g_unk0x005a2844, &g_soundBuffers[Sound_GetLoadedSampleCount()], rate, bits, channels, 1,
                              pWave->dataSize))
                return FALSE;
            // PORT: no separate 3D interface.
            if (!Sound_CopyBufferData(g_soundBuffers[Sound_GetLoadedSampleCount()], 0, pData, pWave->dataSize))
                return FALSE;
        }
    }
    if (inArchive == 0)
        CFileBuffer::FreeGenericFileBuffer(pWave);
    return TRUE;
}

// Creates a PCM buffer of the given format (3D buffers use the HRTF light
// algorithm on Windows 98 and later).
// match 84%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// The 3D algorithm and null GUIDs come from the SDK; the original keeps them at
// these addresses (same values).
// GLOBAL: CMR2 0x00511ca8
// DS3DALG_HRTF_LIGHT
// GLOBAL: CMR2 0x00513ea8
// GUID_NULL

// FUNCTION: CMR2 0x004a20c0
// PORT: a plain audio buffer (3D buffers are never placed by the game).
BOOL Sound_CreatePcmSampleBuffer(int device, AudioBuffer **ppBuffer, DWORD rate, int bits, int channels, int is3D,
                  DWORD size)
{
    if (!device)
        return FALSE;
    *ppBuffer = Audio_CreateBuffer(rate, bits, channels, size);
    return *ppBuffer != NULL;
}

// Copies size bytes of data into the buffer at the given offset.
// FUNCTION: CMR2 0x004a2210
BOOL Sound_CopyBufferData(AudioBuffer *pBuffer, DWORD offset, void *pData, DWORD size)
{
    void *p1;
    DWORD n1;
    void *p2;
    DWORD n2;

    if (Audio_LockBuffer(pBuffer, offset, size, &p1, &n1, &p2, &n2)) {
        if (p1 != NULL)
            memcpy(p1, pData, n1);
        if (p2 != NULL)
            memcpy(p2, (BYTE *)pData + n1, n2);
        Audio_UnlockBuffer(pBuffer);
        return TRUE;
    }
    return FALSE;
}

// Builds the looping buffer of the slot from the part of the sample after
// the loop start.
// match 85%: below the 90% bar; kept as FUNCTION on purpose so reccmp measures it (see CONVENCIONES)
// FUNCTION: CMR2 0x004a24a0
void Sound_BuildSlotLoopBuffer(SoundSlot *pSlot)
{
    DWORD rate;
    int bits;
    int channels;
    void *p1;
    DWORD n1;
    void *p2;
    DWORD n2;

    Audio_GetBufferFormat(pSlot->pBuffer, &rate, &bits, &channels);
    Audio_LockBuffer(pSlot->pBuffer, 0, Audio_GetBufferSize(pSlot->pBuffer), &p1, &n1, &p2, &n2);
    if (Sound_CreatePcmSampleBuffer(g_unk0x005a2844, &pSlot->pLoopBuffer, rate, bits, channels, pSlot->field_0x14,
                                    n1 - pSlot->field_0x18))
        Sound_CopyBufferData(pSlot->pLoopBuffer, 0, (BYTE *)p1 + pSlot->field_0x18, n1 - pSlot->field_0x18);
    Audio_UnlockBuffer(pSlot->pBuffer);
}

// The scale factors of the logarithmic attenuation, from the original's block.
// GLOBAL: CMR2 0x00511418
extern const float g_floatOneOver65536 = 1.0f / 65536.0f;
// GLOBAL: CMR2 0x00511410
extern const double g_doubleMinus10 = -10.0;
// GLOBAL: CMR2 0x005113c0
extern const double g_double10 = 10.0;

// Applies the slot volume (scaled by the master volume) as a logarithmic
// attenuation in hundredths of a decibel.
// FUNCTION: CMR2 0x004a25f0
void Sound_ApplySlotVolumeAttenuation(SoundSlot *pSlot)
{
    int volume;
    int attenuation;

    volume = FTOI(int, X87S_MUL(X87S_MUL(X87S_FILD(float, Sound_GetMasterVolume()), X87S_LOAD(pSlot->field_0xc)), g_floatOneOver65536));
    attenuation = AUDIO_VOLUME_MIN -
                  FTOI(int, X87S_MUL(log((double)volume), g_doubleMinus10)) * abs(10000) / FTOI(int, X87S_MUL(log(CGraphics::m_65536), g_double10));
    Audio_SetVolume(pSlot->pBuffer, attenuation);
    if (pSlot->pLoopBuffer != NULL)
        Audio_SetVolume(pSlot->pLoopBuffer, attenuation);
}

// Gives the slot a buffer for its sample (a duplicate when the sample is
// already playing), sets it up and starts it.
// FUNCTION: CMR2 0x004a22c0
int Sound_StartSlotBuffer(SoundSlot *pSlot)
{
    AudioBuffer *pSource;
    SoundSlot *pOther;
    int flags;
    int shared;
    int i;

    i = 0;
    flags = 0;
    pSource = g_soundBuffers[pSlot->sampleId];
    if (pSource == NULL)
        return 0;
    if (pSlot->field_0x10 != 0 && pSlot->field_0x18 == 0)
        flags = 1;      // DSBPLAY_LOOPING
    shared = 0;
    for (i = 0; i < 32; i++) {
        pOther = Sound_GetSlot(i);
        if (pOther != NULL && pOther->sampleId == pSlot->sampleId && pOther->id != pSlot->id) {
            shared = 1;
            break;
        }
    }
    if (Audio_IsPlaying(pSource) || shared) {
        pSlot->pBuffer = Audio_DuplicateBuffer(g_soundBuffers[pSlot->sampleId]);
        pSlot->field_0x2c = 1;
    } else {
        pSlot->pBuffer = g_soundBuffers[pSlot->sampleId];
    }
    if (pSlot->field_0x10 != 0 && pSlot->field_0x18 != 0)
        Sound_BuildSlotLoopBuffer(pSlot);
    Sound_ApplySlotVolumeAttenuation(pSlot);
    Sound_ApplySlotFrequency(pSlot);
    CSound::EnsureBufferPlaying(pSlot->pBuffer, flags);
    return 1;
}

// FUNCTION: CMR2 0x004a2690
void Sound_ApplySlotFrequency(SoundSlot *pSlot)
{
    if (pSlot->field_0xa < 100)
        pSlot->field_0xa = 100;
    if (pSlot->field_0xa > 100000)
        pSlot->field_0xa = 34464;
    Audio_SetFrequency(pSlot->pBuffer, pSlot->field_0xa);
    if (pSlot->pLoopBuffer != NULL)
        Audio_SetFrequency(pSlot->pLoopBuffer, pSlot->field_0xa);
}

// GLOBAL: CMR2 0x005a271c
int g_unk0x005a271c;
// PORT: 1 once the audio output is open (was the IDirectSound object).
// GLOBAL: CMR2 0x005a2844
int g_unk0x005a2844;


// Stops the slot's buffers and releases the looping ones.
// FUNCTION: CMR2 0x004a26f0
void Sound_StopSlotBuffers(SoundSlot *pSlot)
{
    if (pSlot->pBuffer == NULL)
        return;
    if (pSlot->field_0x30 != 0) {
        Audio_Stop(pSlot->pLoopBuffer);
        if (pSlot->pLoopBuffer != NULL) {
            Audio_ReleaseBuffer(pSlot->pLoopBuffer);
            pSlot->pLoopBuffer = NULL;
        }
    }
    Audio_Stop(pSlot->pBuffer);
    Audio_SetPlayPosition(pSlot->pBuffer, 0);
    if (pSlot->field_0x2c != 0 && pSlot->pBuffer != NULL) {
        Audio_ReleaseBuffer(pSlot->pBuffer);
        pSlot->pBuffer = NULL;
    }
}

int Sound_FindFreeSlot(void);
unsigned int Sound_MakeHandle(unsigned int index, unsigned short serial);
int Sound_FindHandle(unsigned int handle);
void Sound_SetPan(unsigned int handle, int pan);

// Whether the system is Windows 98 / NT 5 or later.
// FUNCTION: CMR2 0x004b75c0
// PORT: always (the original checked for Windows 98 / NT 5).
BOOL Sound_IsModernWindowsVersion(void)
{
    return TRUE;
}

// Sets the volume of a playing sound.
// FUNCTION: CMR2 0x004b79a0
void Sound_SetPlayingSlotVolume(unsigned int handle, int volume)
{
    int index;

    index = Sound_FindHandle(handle);
    if (index != -1 && CSound::m_soundSlots[index]->field_0xc != volume) {
        CSound::m_soundSlots[index]->field_0xc = volume;
        Sound_ApplySlotVolumeAttenuation(CSound::m_soundSlots[index]);
    }
}

// Starts a sound: takes a free slot, fills it and plays it. Returns the slot
// handle, or -1 when no slot is free or the sample cannot be played.
// FUNCTION: CMR2 0x004b7790
int Sound_PlaySampleWithParameters(unsigned short id, int volume, int frequency, int loopStart, int loops, int is3D)
{
    int index;
    unsigned int handle;

    index = Sound_FindFreeSlot();
    if (index == -1 || id >= 200)
        return -1;
    handle = Sound_MakeHandle(index, id);
    memset(CSound::m_soundSlots[index] = (SoundSlot *)CFileBuffer::AllocateLockedBuffer(sizeof(SoundSlot)), 0,
           sizeof(SoundSlot));
    CSound::m_soundSlots[index]->sampleId = id;
    CSound::m_soundSlots[index]->id = index;
    CSound::m_soundSlots[index]->handle = handle;
    CSound::m_soundSlots[index]->field_0x10 = loops;
    CSound::m_soundSlots[index]->field_0x14 = is3D;
    CSound::m_soundSlots[index]->field_0x18 = loopStart;
    CSound::m_soundSlots[index]->field_0x30 = loopStart != 0;
    CSound::m_soundSlots[index]->field_0xc = volume;
    CSound::m_soundSlots[index]->field_0xa = frequency;
    if (!Sound_StartSlotBuffer(CSound::m_soundSlots[index])) {
        CFileBuffer::FreeGenericFileBuffer(CSound::m_soundSlots[index]);
        CSound::m_soundSlots[index] = NULL;
        return -1;
    }
    Sound_SetPlayingSlotVolume(handle, volume);
    Sound_SetPan(handle, frequency);
    return handle;
}

// GLOBAL: CMR2 0x005210f4
int g_soundMasterVolume = 0x10000;
// GLOBAL: CMR2 0x006e0ef0
int g_soundSampleCount;

int Sound_IsSlotPlayingOrPending(SoundSlot *pSlot);
int Sound_FindHandle(unsigned int handle);
void Sound_StopSlotBuffers(SoundSlot *pSlot);
void Sound_ApplySlotFrequency(SoundSlot *pSlot);

// FUNCTION: CMR2 0x004b7610
SoundSlot *Sound_GetSlot(int index)
{
    return CSound::m_soundSlots[index];
}

void Sound_FreeAll(void);
void Sound_ReleaseSampleBuffers(int sample);

// Frees every loaded sound and releases the samples from index first on.
// GLOBAL: CMR2 0x005210f8
char g_strFailedToLoad[20] = "Failed to load \"%s\"";

int Sound_ShutdownSystem(void);

// Starts the sound system: clears the sound slots, creates the DirectSound
// device and registers the shutdown callback.
// FUNCTION: CMR2 0x004b7650
BOOL Sound_Init(int sampleRate, int channels, int bits, int unused)
{
    int i;

    if (CSound::m_unk0x006e0eec != 0)
        return FALSE;
    g_soundMasterVolume = 0x10000;
    for (i = 0; i < 32; i++)
        CSound::m_soundSlots[i] = NULL;
    if (!Sound_InitDevice(sampleRate, channels, bits, unused))
        return FALSE;
    CGame::RegisterCallback(Sound_ShutdownSystem, NULL);
    CSound::m_unk0x006e0eec = 1;
    return TRUE;
}

// Loads a sample into the next slot (at most 200).
// FUNCTION: CMR2 0x004b76c0
BOOL Sound_LoadSample(char *name, BYTE flags, GenericFile *pFile)
{
    char message[260];

    if (CSound::m_unk0x006e0eec == 0 || (unsigned int)g_soundSampleCount >= 200)
        return FALSE;
    if (Sound_LoadWave(name, flags, pFile)) {
        g_soundSampleCount++;
        return TRUE;
    }
    // the original formats an error message nobody reads
    sprintf(message, g_strFailedToLoad, name);
    return FALSE;
}

// FUNCTION: CMR2 0x004b7740
void Sound_FreeSamplesFromIndex(int first)
{
    int count = g_soundSampleCount;

    if (CSound::m_unk0x006e0eec) {
        Sound_FreeAll();
        for (; first < g_soundSampleCount; first++) {
            Sound_ReleaseSampleBuffers(first);
            count--;
        }
        g_soundSampleCount = count;
    }
}

// FUNCTION: CMR2 0x004b7780
int Sound_GetLoadedSampleCount(void)
{
    return g_soundSampleCount;
}

// FUNCTION: CMR2 0x004b78a0
int Sound_IsPlaying(unsigned int handle)
{
    int index = Sound_FindHandle(handle);
    if (index != -1)
        return Sound_IsSlotPlayingOrPending(CSound::m_soundSlots[index]);
    return 0;
}

// FUNCTION: CMR2 0x004b78d0
void Sound_Free(unsigned int handle)
{
    int index = Sound_FindHandle(handle);
    if (index != -1) {
        Sound_StopSlotBuffers(CSound::m_soundSlots[index]);
        CFileBuffer::FreeGenericFileBuffer(CSound::m_soundSlots[index]);
        CSound::m_soundSlots[index] = NULL;
    }
}

// FUNCTION: CMR2 0x004b7910
void Sound_FreeAll(void)
{
    SoundSlot **pp = CSound::m_soundSlots;

    do {
        if (*pp != NULL) {
            Sound_StopSlotBuffers(*pp);
            CFileBuffer::FreeGenericFileBuffer(*pp);
            *pp = NULL;
        }
        pp++;
    } while ((INT_PTR)pp < (INT_PTR)(CSound::m_soundSlots + 32));
}

// FUNCTION: CMR2 0x004b7940
int Sound_GetSampleCount(void)
{
    return g_soundSampleCount;
}

// FUNCTION: CMR2 0x004b7990
int Sound_GetMasterVolume(void)
{
    return g_soundMasterVolume;
}

// FUNCTION: CMR2 0x004b79e0
void Sound_SetPan(unsigned int handle, int pan)
{
    int index = Sound_FindHandle(handle);
    if (index != -1) {
        CSound::m_soundSlots[index]->field_0xa = pan;
        Sound_ApplySlotFrequency(CSound::m_soundSlots[index]);
    }
}

// FUNCTION: CMR2 0x004b7a10
int Sound_GetVolume(unsigned int handle)
{
    int index = Sound_FindHandle(handle);
    if (index != -1)
        return CSound::m_soundSlots[index]->field_0xc;
    return 0;
}

// FUNCTION: CMR2 0x004b7a70
int Sound_FindFreeSlot(void)
{
    int i;
    SoundSlot **pp;

    for (i = 0, pp = CSound::m_soundSlots; (INT_PTR)pp < (INT_PTR)(CSound::m_soundSlots + 32); pp++, i++) {
        if (*pp == NULL)
            return i;
    }
    return -1;
}

// FUNCTION: CMR2 0x004b7a90
unsigned int Sound_MakeHandle(unsigned int index, unsigned short serial)
{
    return (serial & 0xffff) << 8 | index & 0x3f;
}

// FUNCTION: CMR2 0x004b7ab0
int Sound_FindHandle(unsigned int handle)
{
    if ((int)handle >= 0 && CSound::m_soundSlots[handle & 0x3f] != NULL &&
        CSound::m_soundSlots[handle & 0x3f]->handle == handle)
        return handle & 0x3f;
    return -1;
}

// FUNCTION: CMR2 0x004a1d00
int Sound_GetSampleTableState(void)
{
    return g_unk0x005a2844;
}

// FUNCTION: CMR2 0x004a3180
void Sound_ClearMusicPauseFlag(void)
{
    if (CSound::m_unk0x005a2730 != 0)
        CSound::m_unk0x005a2724 = 0;
}

// FUNCTION: CMR2 0x004a3240
void Sound_NoOpMusicCallback(INT_PTR unused)
{
}


// Sets the master volume (0..1) and re-applies it to every sound slot.
// FUNCTION: CMR2 0x004b7950
void Sound_SetMasterVolume(int volume)
{
    SoundSlot **ppSlot;

    if (volume >= 0 && volume <= 0x10000)
        g_soundMasterVolume = volume;
    ppSlot = CSound::m_soundSlots;
    do {
        if (*ppSlot != NULL)
            Sound_ApplySlotVolumeAttenuation(*ppSlot);
        ppSlot++;
    } while ((INT_PTR)ppSlot < (INT_PTR)(CSound::m_soundSlots + 32));
}

// Releases the (3D) buffers of a sample.
// FUNCTION: CMR2 0x004a1d10
void Sound_ReleaseSampleBuffers(int sample)
{
    if (g_soundBuffers[sample] != NULL) {
        Audio_ReleaseBuffer(g_soundBuffers[sample]);
        g_soundBuffers[sample] = NULL;
    }
    g_sound3DBuffers[sample] = NULL;
}

// PORT: unused (the DirectSound 3D listener).
// GLOBAL: CMR2 0x005a2848
int g_unk0x005a2848;
// GLOBAL: CMR2 0x005a284c
int g_unk0x005a284c;

int Sound_GetLoadedSampleCount(void);

// Releases every sample buffer, the primary buffer and DirectSound itself.
// FUNCTION: CMR2 0x004a2830
void Sound_ReleaseDirectSoundResources(void)
{
    int i;

    for (i = 0; i < Sound_GetLoadedSampleCount(); i++)
        Sound_ReleaseSampleBuffers(i);
    if (g_unk0x005a2844)
        Audio_Shutdown();
    g_unk0x005a2844 = 0;
    g_unk0x005a284c = 0;
}

// Creates the shared 16-bit stereo 44.1 kHz streaming buffer.
// FUNCTION: CMR2 0x004a2a20
// PORT: the 8-block ring buffer of 16-bit stereo 44.1 kHz PCM.
BOOL Sound_CreateMusicStreamingBuffer(void)
{
    g_unk0x005a271c = 0x7f400;
    CSound::m_pDirectSoundBuffer = Audio_CreateBuffer(44100, 16, 2, 0x7f400);
    return CSound::m_pDirectSoundBuffer != NULL ? 0 : -1;
}

void Sound_ReleaseDirectSoundResources(void);

// Shuts the sound system down (registered callback of 0x4b7650).
// FUNCTION: CMR2 0x004b7ae0
int Sound_ShutdownSystem(void)
{
    Sound_FreeAll();
    Sound_SetMasterVolume(0);
    Sound_ReleaseDirectSoundResources();
    g_soundSampleCount = 0;
    CSound::m_unk0x006e0eec = 0;
    return 1;
}

// Restarts the music stream after the Direct3D device is created: reopens
// the file at 0x5a2738, re-applies the music volume and starts playback.
// FUNCTION: CMR2 0x004a2ba0
void Sound_RestartMusicAfterDeviceCreation(void)
{
    if (CSound::m_unk0x005a2734) {
        CSound::OpenStreamingMusicFile(CSound::m_unk0x005a2738);
        CSound::SetMusicStreamVolume(CGameInfo::GetMasterSoundVolume());
        Sound_StartLoopingMusicStream(1);
    }
}

