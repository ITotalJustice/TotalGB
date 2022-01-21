#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAS_SDL2
// stuff that i want in SDL, will PR things over
#include <SDL.h>

typedef enum
{
    SDL_DIRECTORY_READ = 1 << 0,
    SDL_DIRECTORY_WRITE = 1 << 1,
    SDL_DIRECTORY_EXECUTE = 1 << 2,

    SDL_DIRECTORY_RW = SDL_DIRECTORY_READ | SDL_DIRECTORY_WRITE,
    SDL_DIRECTORY_RWX = SDL_DIRECTORY_READ | SDL_DIRECTORY_WRITE | SDL_DIRECTORY_EXECUTE,
} SDL_DirectoryFlags;

int SDL_CreateDirectory(const char* path, Uint32 flags);
#endif // HAS_SDL2

#ifdef __cplusplus
}
#endif
