#pragma once
// Streams a child process's stdout to our own stdout as it arrives and returns
// everything collected. PrepareEnhancementPack used SDL_ReadProcess, which holds
// all output until the helper exits: the pack verify/convert phases take minutes
// on a real Mac, so the game looked hung even after the helper started flushing
// progress lines (Devin finding on PR #386).
#include <SDL3/SDL.h>
#include <cstdio>
#include <string>

inline std::string StreamProcessOutput(SDL_IOStream *io) {
    std::string text;
    char chunk[4096];
    while (io != nullptr) {
        size_t n = SDL_ReadIO(io, chunk, sizeof chunk);
        if (n > 0) {
            fwrite(chunk, 1, n, stdout);
            fflush(stdout);
            text.append(chunk, n);
            continue;
        }
        SDL_IOStatus status = SDL_GetIOStatus(io);
        if (status == SDL_IO_STATUS_NOT_READY) { SDL_Delay(10); continue; }   // pipe not fed yet
        break;   // EOF or error: the child is done writing
    }
    return text;
}
