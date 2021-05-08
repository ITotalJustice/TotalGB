#include "sdl2.h"
#include "base/base.h"


typedef struct Ctx {
    struct Base base;

    SDL_Renderer* renderer;
    SDL_Texture* texture;
} ctx_t;

#define CTX_FROM_PRIVATE ctx_t* self = (ctx_t*)_private;


static void quit(
    void* _private
);

static void render(
    void* _private
);

static void update_game_texture(
    void* _private,
    const struct VideoInterfaceGameTexture* game_texture
);

static void poll_events(
    void* _private
);

static void toggle_fullscreen(
    void* _private
);

static void set_window_name(
    void* _private,
    const char* name
);

struct VideoInterface* video_interface_init_sdl2(
    const struct VideoInterfaceInfo* info,
    const struct VideoInterfaceUserCallbacks* callbacks
) {
    struct VideoInterface* iface = NULL;
    ctx_t* self = NULL;

    iface = malloc(sizeof(struct VideoInterface));
    self = SDL_malloc(sizeof(struct Ctx));

    if (!iface) {
        goto fail;
    }

    if (!self) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s\n\n", SDL_GetError());
        goto fail;
    }

    const struct VideoInterface internal_interface = {
        ._private = self,
        .quit = quit,
        .render = render,
        .update_game_texture = update_game_texture,
        .poll_events = poll_events,
        .toggle_fullscreen = toggle_fullscreen,
        .set_window_name = set_window_name,
    };

    // set the internal data!
    *iface = internal_interface;

    const struct BaseConfig base_config = {
        .window_name = info->window_name,
        .window_flags = 0,
        .x = SDL_WINDOWPOS_CENTERED,
        .y = SDL_WINDOWPOS_CENTERED,
        .w = info->w,
        .h = info->h,
        
        .min_win_w = 160,
        .min_win_h = 144,

        .max_win_w = 0,
        .max_win_h = 0,

        .set_min_win = true,
        .set_max_win = false
    };

    if (!base_sdl2_init(&self->base, &base_config, callbacks)) {
        goto fail;
    }

    const int render_flags = SDL_RENDERER_ACCELERATED |SDL_RENDERER_PRESENTVSYNC;

    self->renderer = SDL_CreateRenderer(
        self->base.window, -1, render_flags
    );

    if (!self->renderer) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s\n", SDL_GetError());
        goto fail;
    }

    // try and enable blending
    if (SDL_SetRenderDrawBlendMode(self->renderer, SDL_BLENDMODE_BLEND)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s\n", SDL_GetError());
    }

    self->texture = SDL_CreateTexture(
        self->renderer,
        SDL_PIXELFORMAT_BGR555, SDL_TEXTUREACCESS_STREAMING,
        160, 144
        // game_info.w, game_info.h
    );

    if (!self->texture) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s\n", SDL_GetError());
        goto fail;
    }

    return iface;

fail:
    if (iface) {
        free(iface);
        iface = NULL;
    }

    if (self) {
        quit(self);
    }

    return NULL;
}

static void quit(
    void* _private
) {
    CTX_FROM_PRIVATE;

    if (SDL_WasInit(SDL_INIT_VIDEO)) {
        if (self->texture) {
            SDL_DestroyTexture(self->texture);
        }

        if (self->renderer) {
            SDL_DestroyRenderer(self->renderer);
        }

        base_sdl2_exit(&self->base);
    }

    SDL_free(self);
}

static void render(
    void* _private
) {
    CTX_FROM_PRIVATE;

    SDL_RenderClear(self->renderer);

    SDL_RenderCopy(
        self->renderer, self->texture,
        NULL, NULL
    );

    SDL_RenderPresent(self->renderer);
}

static void update_game_texture(
    void* _private,
    const struct VideoInterfaceGameTexture* game_texture
) {
    CTX_FROM_PRIVATE;

    void* pixels; int pitch;

    SDL_LockTexture(self->texture, NULL, &pixels, &pitch);
    memcpy(pixels, game_texture->pixels, 160*144*2);
    SDL_UnlockTexture(self->texture);
}

static void poll_events(
    void* _private
) {
    CTX_FROM_PRIVATE;

    base_sdl2_poll_events(&self->base);
}

static void toggle_fullscreen(
    void* _private
) {
    CTX_FROM_PRIVATE;

    base_sdl2_toggle_fullscreen(&self->base);
}

static void set_window_name(
    void* _private,
    const char* name
) {
    CTX_FROM_PRIVATE;

    base_sdl2_set_window_name(&self->base, name);
}
