#include "sdl_helper.h"

#ifdef HAS_SDL2
#ifdef WIN32
#include <Windows.h>

// this isn't perfect yet as it needs to check for '\' and '/'

int SDL_CreateDirectory(const char* path, Uint32 flags) {
    char* str = NULL;
    char* ptr = NULL;
    size_t len = 0;
    // int mode = 0; // unused currently...

    if (!path)
        return SDL_InvalidParamError("path");

    //if (!mode)
    //    return SDL_InvalidParamError("flags");

    len = SDL_strlen(path);
    if (!len) // sanity check
        return -1;

    str = SDL_strdup(path);
    if (!str)
        return SDL_OutOfMemory();

    if (str[len - 1] == '/')
        str[len - 1] = '\0';

    for (ptr = str + 1; *ptr; ptr++) {
        if (*ptr == '/') {
            *ptr = '\0';
            if (!CreateDirectory(str, NULL)) {
                if (GetLastError() != ERROR_ALREADY_EXISTS) {
                    goto error;
                }
            }
            *ptr = '/';
        }
    }

    if (!CreateDirectory(str, NULL)) {
    error:
        SDL_SetError("Couldn't create directory '%s': '%s'", str, strerror(errno));
        SDL_free(str);
        return -1;
    }

    printf("created directory str: %s path: %s\n", str, path);

    SDL_free(str);
    return 0;
}
#else
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

int SDL_CreateDirectory(const char* path, Uint32 flags) {
    char* str = NULL;
    char *ptr = NULL;
    size_t len = 0;
    int mode = 0;

    if (!path)
        return SDL_InvalidParamError("path");

    if (flags & SDL_DIRECTORY_READ)
        mode |= S_IRUSR | S_IRGRP | S_IROTH;

    if (flags & SDL_DIRECTORY_WRITE)
        mode |= S_IWUSR | S_IWGRP | S_IWOTH;

    if (flags & SDL_DIRECTORY_EXECUTE)
        mode |= S_IXUSR | S_IXGRP | S_IXOTH;

    if (!mode)
        return SDL_InvalidParamError("flags");

    len = SDL_strlen(path);
    if (!len) // sanity check
        return -1;

    str = SDL_strdup(path);
    if (!str)
        return SDL_OutOfMemory();

    if (str[len - 1] == '/')
        str[len - 1] = '\0';

    for (ptr = str+1; *ptr; ptr++) {
        if (*ptr == '/') {
            *ptr = '\0';
            if (mkdir(str, mode) != 0 && errno != EEXIST)
                goto error;
            *ptr = '/';
        }
    }

    if (mkdir(str, mode) != 0 && errno != EEXIST) {
error:
        SDL_SetError("Couldn't create directory '%s': '%s'", str, strerror(errno));
        SDL_free(str);
        return -1;
    }

    SDL_free(str);
    return 0;
}
#endif
#endif // HAS_SDL2
