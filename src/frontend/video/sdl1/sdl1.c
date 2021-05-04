#include "sdl1.h"
#include "base/base.h"


typedef struct Ctx {
    struct Base base;
    SDL_Surface* texture;
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

static int surface_lock(SDL_Surface* surface) {
	if (SDL_MUSTLOCK(surface)) {
		return SDL_LockSurface(surface);
    }

    return 0; // no locking needed
}

static void surface_unlock(SDL_Surface* surface) {
	if (SDL_MUSTLOCK(surface)) {
		SDL_UnlockSurface(surface);
    }
}

struct VideoInterface* video_interface_init_sdl1(
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
        fprintf(stderr, "%s\n\n", SDL_GetError());
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
        .window_flags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_RESIZABLE,
        .w = info->w,
        .h = info->h,
        .bpp = 32
    };

    if (!base_sdl1_init_system(&self->base)) {
    	goto fail;
	}

    if (!base_sdl1_init_window(&self->base, &base_config, callbacks)) {
        goto fail;
    }

    #if SDL_BYTEORDER == SDL_LIL_ENDIAN
	    const uint32_t rmask = 0x1F << 0;
	    const uint32_t gmask = 0x1F << 5;
	    const uint32_t bmask = 0x1F << 10;
	    const uint32_t amask = 0x00;
	#else
	    const uint32_t rmask = 0x1F << 10;
	    const uint32_t gmask = 0x1F << 5;
	    const uint32_t bmask = 0x1F << 0;
	    const uint32_t amask = 0x00;
	#endif

    self->texture = SDL_CreateRGBSurface(
    	SDL_SWSURFACE,
    	160, 144,
    	15,
    	rmask, gmask, bmask, amask
    );

    if (!self->texture) {
        fprintf(stderr, "%s\n", SDL_GetError());
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
            SDL_FreeSurface(self->texture);
        }

        base_sdl1_exit(&self->base);
    }

    SDL_free(self);
}

static void render(
    void* _private
) {
    CTX_FROM_PRIVATE;

    if (SDL_BlitSurface(
    	self->texture, NULL,
    	self->base.window, NULL
    )) {
    	fprintf(stderr, "%s\n", SDL_GetError());
    }

    if (SDL_Flip(self->base.window)) {
    	fprintf(stderr, "%s\n", SDL_GetError());
    }
}

static void update_game_texture(
    void* _private,
    const struct VideoInterfaceGameTexture* game_texture
) {
    CTX_FROM_PRIVATE;

    if (surface_lock(self->texture)) {
    	fprintf(stderr, "%s\n", SDL_GetError());
    	return;
    }
    
    memcpy(
    	self->texture->pixels,
    	game_texture->pixels,
    	160 * 144 * sizeof(uint16_t)
    );

    surface_unlock(self->texture);
}

static void poll_events(
    void* _private
) {
    CTX_FROM_PRIVATE;

    base_sdl1_poll_events(&self->base);
}

static void toggle_fullscreen(
    void* _private
) {
    CTX_FROM_PRIVATE;

    base_sdl1_toggle_fullscreen(&self->base);
}

static void set_window_name(
    void* _private,
    const char* name
) {
    CTX_FROM_PRIVATE;

    base_sdl1_set_window_name(&self->base, name);
}

