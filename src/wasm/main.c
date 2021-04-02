#include "core/gb.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>

#include <emscripten.h>
#include <emscripten/html5.h>


// NOTE TO SELF: do NOT enable -flto when building, this breaks audio
// try it with tetris.gb, it'll sound awful.


// main emu core
static struct GB_Core CORE = {0};

// 4mb static rom buffer, just makes things easier than malloc / free
static uint8_t ROM_DATA[0x400000] = {0};

static volatile bool rom_loaded = false;

// sdl2 stuff
static SDL_Window* window;
static SDL_Renderer* renderer;
static SDL_Texture* texture;
static int AUDIO_DEVICE_ID = 0;

#ifdef  GB_SDL_AUDIO_CALLBACK_STREAM
static SDL_AudioStream* AUDIO_STREAM = NULL;

// sdl audio stream is thread safe, no need to manually lock it!
static void core_audio_callback(struct GB_Core* gb, void* user_data, struct GB_ApuCallbackData* data) {
    SDL_AudioStreamPut(AUDIO_STREAM, data->samples, sizeof(data->samples));
}

static void sdl_audio_callback(void* user, uint8_t* buf, int len) {
    memset(buf, 0, len);
    SDL_AudioStreamGet(AUDIO_STREAM, buf, len);
}
#else
// don't use this...
static void core_audio_callback(struct GB_Core* gb, void* user_data, struct GB_ApuCallbackData* data) {
    // this blocking approach will not work in browsers because we cannot block
    // the main thread!
    // doing so will actually cause the audio to stop playing, so
    // this loop will never exit...
    // i think a better approach is to use the sdl audio callback system
    // as well as audio stream resampling.
    // we should push to the buffer as fast as possible, then the audio
    // thread will pull from it.    
    SDL_QueueAudio(AUDIO_DEVICE_ID, data->samples, sizeof(data->samples));
}
#endif // GB_SDL_AUDIO_CALLBACK_STREAM

static void OnQuitEvent(const SDL_QuitEvent* e) {
    printf("quit request at %u\n", e->timestamp);
    printf("quit request at %u\n", e->timestamp);
}

static void OnDropEvent(SDL_DropEvent* e) {
    switch (e->type) {
        case SDL_DROPFILE:
            printf("drop file event!\n");
            if (e->file != NULL) {
                SDL_free(e->file);
            }
            break;
        
        case SDL_DROPTEXT:
            break;

        case SDL_DROPBEGIN:
            break;

        case SDL_DROPCOMPLETE:
            break;
    }
}

static void OnAudioEvent(const SDL_AudioDeviceEvent* e) {
}

static void OnWindowEvent(const SDL_WindowEvent* e) {
    switch (e->event) {
        case SDL_WINDOWEVENT_EXPOSED:
            break;

        case SDL_WINDOWEVENT_RESIZED:
            break;

        case SDL_WINDOWEVENT_RESTORED:
            break;
        
        case SDL_WINDOWEVENT_CLOSE:
            break;
    }
}

static void OnDisplayEvent(const SDL_DisplayEvent* e) {
   switch (e->event) {
       case SDL_DISPLAYEVENT_NONE:
           break;

       case SDL_DISPLAYEVENT_ORIENTATION:
           break;
   }
}

static void OnKeyEvent(const SDL_KeyboardEvent* e) {
    struct KeyMapEntry {
        int key;
        enum GB_Button button;
    };

    static const struct KeyMapEntry keymap[8] = {
        {SDLK_x, GB_BUTTON_A},
        {SDLK_z, GB_BUTTON_B},
        {SDLK_RETURN, GB_BUTTON_START},
        {SDLK_SPACE, GB_BUTTON_SELECT},
        {SDLK_DOWN, GB_BUTTON_DOWN},
        {SDLK_UP, GB_BUTTON_UP},
        {SDLK_LEFT, GB_BUTTON_LEFT},
        {SDLK_RIGHT, GB_BUTTON_RIGHT},
    };

    const bool kdown = e->type == SDL_KEYDOWN;

    // first check if any of the mapped keys were pressed...
    for (size_t i = 0; i < 8; i++) {
        if (keymap[i].key == e->keysym.sym) {
            GB_set_buttons(&CORE, keymap[i].button, kdown);
            return;
        }
    }
}

static void OnJoypadAxisEvent(const SDL_JoyAxisEvent* e) {

}

static void OnJoypadButtonEvent(const SDL_JoyButtonEvent* e) {

}

static void OnJoypadHatEvent(const SDL_JoyHatEvent* e) {

}

static void OnJoypadDeviceEvent(const SDL_JoyDeviceEvent* e) {

}

static void OnControllerAxisEvent(const SDL_ControllerAxisEvent* e) {

}

static void OnControllerButtonEvent(const SDL_ControllerButtonEvent* e) {
}

static void OnControllerDeviceEvent(const SDL_ControllerDeviceEvent* e) {

}

static void OnTouchEvent(const SDL_TouchFingerEvent* e) {

}

static void OnUserEvent(SDL_UserEvent* e) {

}

static void events() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
                OnQuitEvent(&e.quit);
                break;
            
            case SDL_DROPFILE: case SDL_DROPTEXT: case SDL_DROPBEGIN: case SDL_DROPCOMPLETE:
                OnDropEvent(&e.drop);
                break;

            case SDL_DISPLAYEVENT:
                OnDisplayEvent(&e.display);
                break;

            case SDL_WINDOWEVENT:
                OnWindowEvent(&e.window);
                break;

            case SDL_KEYDOWN: case SDL_KEYUP:
                OnKeyEvent(&e.key);
                break;

            case SDL_CONTROLLERBUTTONDOWN: case SDL_CONTROLLERBUTTONUP:
                OnControllerButtonEvent(&e.cbutton);
                break;

            case SDL_CONTROLLERAXISMOTION:
                OnControllerAxisEvent(&e.caxis);
                break;

            case SDL_JOYBUTTONDOWN: case SDL_JOYBUTTONUP:
                OnJoypadButtonEvent(&e.jbutton);
                break;

            case SDL_JOYAXISMOTION:
                OnJoypadAxisEvent(&e.jaxis);
                break;

            case SDL_JOYHATMOTION:
                OnJoypadHatEvent(&e.jhat);
                break;

            case SDL_JOYDEVICEADDED: case SDL_JOYDEVICEREMOVED:
                OnJoypadDeviceEvent(&e.jdevice);
                break;

            case SDL_CONTROLLERDEVICEADDED: case SDL_CONTROLLERDEVICEREMOVED:
                OnControllerDeviceEvent(&e.cdevice);
                break;

            case SDL_FINGERDOWN: case SDL_FINGERUP:
                OnTouchEvent(&e.tfinger);
                break;
            
            case SDL_USEREVENT:
                OnUserEvent(&e.user);
                break;
        }
    }
}

static void core_vblank_callback(struct GB_Core* gb, void* user) {
    void* pixles; int pitch;
    SDL_LockTexture(texture, NULL, &pixles, &pitch);
    memcpy(pixles, gb->ppu.pixles, sizeof(gb->ppu.pixles));
    SDL_UnlockTexture(texture);
}

static void draw() {
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

static void run_core() {
    if (rom_loaded) {
        GB_run_frame(&CORE);
    }
}

static void main_loop(void) {
    events();
    run_core();
    draw();
}

EMSCRIPTEN_KEEPALIVE void emscripten_load_rom(void) {
    ssize_t rom_size = 0;
    FILE* rom_file = fopen("rom.bin", "rb");
    bool loaded = false;

    if (rom_file != NULL) {
        fseek(rom_file, 0, SEEK_END);
        rom_size = ftell(rom_file);
        fseek(rom_file, 0, SEEK_SET);

        printf("rom size %ld\n", rom_size);

        if (rom_size > 0 && rom_size <= sizeof(ROM_DATA)) {
            fread(ROM_DATA, 1, rom_size, rom_file);
            loaded = true;
            printf("reading...");
        } else {
            printf("rom too big?\n");
        }

        fclose(rom_file);
    }
    else {
        printf("failed to open file...\n");
        return;
    }


    if (loaded) {
        printf("loading rom!\n");
        GB_loadrom_data(&CORE, ROM_DATA, rom_size);
        rom_loaded = true;
    }
}

// #define AUDIO_FREQUENCY 96000
#define AUDIO_FREQUENCY 48000

int main(int argc, char** argv) {

    GB_init(&CORE);
    GB_set_vblank_callback(&CORE, core_vblank_callback, NULL);

    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    window = SDL_CreateWindow("gb-emu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 160 * 3, 144 * 3, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING, 160, 144);

    const SDL_AudioSpec wanted = {
        /* .freq = */ AUDIO_FREQUENCY,
        /* .format = */ AUDIO_S8,
#ifdef CHANNEL_8
        /* .channels = */ 8,
#else
        /* .channels = */ 2,
#endif
        /* .silence = */ 0, // calculated
        /* .samples = */ 512, // 512 * 2 (because stereo)
        /* .padding = */ 0,
        /* .size = */ 0, // calculated
#ifdef GB_SDL_AUDIO_CALLBACK_STREAM
        /* .callback = */ sdl_audio_callback,
        /* .userdata = */ NULL
#else
        /* .callback = */ NULL,
        /* .userdata = */ NULL
#endif
    };
    SDL_AudioSpec obtained = {0};


    AUDIO_DEVICE_ID = SDL_OpenAudioDevice(NULL, 0, &wanted, &obtained, 0);
    
    // check if an audio device was failed to be found...
    if (AUDIO_DEVICE_ID == 0) {
        printf("failed to find valid audio device\n");
    }
    else {
        printf("\nSDL_AudioSpec:\n");
        printf("\tfreq: %d\n", obtained.freq);
        printf("\tformat: %d\n", obtained.format);
        printf("\tchannels: %u\n", obtained.channels);
        printf("\tsilence: %u\n", obtained.silence);
        printf("\tsamples: %u\n", obtained.samples);
        printf("\tpadding: %u\n", obtained.padding);
        printf("\tsize: %u\n", obtained.size);

    #ifdef GB_SDL_AUDIO_CALLBACK_STREAM
        AUDIO_STREAM = SDL_NewAudioStream(
            AUDIO_S8, 2, 4213440/4,
            AUDIO_S8, 2, AUDIO_FREQUENCY
        );
        GB_set_apu_callback(&CORE, core_audio_callback, NULL);
    #else
        GB_set_apu_callback(&CORE, core_audio_callback, NULL);
    #endif // GB_SDL_AUDIO_CALLBACK_STREAM

        SDL_PauseAudioDevice(AUDIO_DEVICE_ID, 0);
    }

    // ssize_t rom_size = 0;
    // FILE* rom_file = fopen(ROM_PATH, "rb");

    // if (rom_file != NULL) {
    //     fseek(rom_file, 0, SEEK_END);
    //     rom_size = ftell(rom_file);
    //     fseek(rom_file, 0, SEEK_SET);

    //     if (rom_size > 0 && rom_size < sizeof(ROM_DATA)) {
    //         fread(ROM_DATA, 1, rom_size, rom_file);
    //         rom_loaded = true;
    //     }

    //     fclose(rom_file);
    // }
    // else {
    //     printf("failed to open file...\n");
    // }


    // if (rom_loaded) {
    //     printf("loading rom!\n");
    //     GB_loadrom_data(&CORE, ROM_DATA, rom_size);
    //     rom_loaded = true;
    // }

    printf("we are here\n");

    emscripten_set_main_loop(main_loop, 0, 1);
    
    printf("exiting...\n");

    SDL_CloseAudioDevice(AUDIO_DEVICE_ID);

#ifdef GB_SDL_AUDIO_CALLBACK_STREAM
    SDL_FreeAudioStream(AUDIO_STREAM);
#endif

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}

