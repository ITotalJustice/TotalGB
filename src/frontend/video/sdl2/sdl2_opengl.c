#include "sdl2_opengl.h"
#include "base/base.h"

#include <SDL2/SDL_opengl.h>
#include <stdlib.h>


typedef struct Ctx {
    struct Base base;

    SDL_GLContext gl_ctx;
    GLuint texture;
} ctx_t;

#define VOID_TO_SELF(_private) ctx_t* self = (ctx_t*)_private;


static void quit(
    void* _private
) {
    VOID_TO_SELF(_private);

    if (SDL_WasInit(SDL_INIT_VIDEO)) {
        if (self->texture) {
            glDeleteTextures(1, &self->texture);
            self->texture = 0;
        }

        if (self->gl_ctx) {
            SDL_GL_DeleteContext(self->gl_ctx);
            self->gl_ctx = NULL;
        }

        base_sdl2_exit(&self->base);
    }

    SDL_free(self);
}

static void render_begin(
    void* _private
) {
    VOID_TO_SELF(_private);
    (void)self;

    SDL_GL_MakeCurrent(self->base.window, self->gl_ctx);

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void render_game(
    void* _private
) {
    VOID_TO_SELF(_private);
    // SOURCE: https://discourse.libsdl.org/t/why-cant-we-do-blit-on-an-opengl-surface/10975/4

    // Use precise pixel coordinates
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.f, 0.f, 0.f);

    // Setup 1:1 pixel ration
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(0.0, 160.0, 0.0, 144.0, -1, 1);

    glBindTexture(GL_TEXTURE_2D, self->texture);
    glEnable(GL_TEXTURE_2D);

    // Draw our background quad
    glBegin(GL_QUADS);
        glTexCoord2f(0.f, 1.f); glVertex2f(0.f, 0.f);
        glTexCoord2f(1.f, 1.f); glVertex2f(160.f, 0.f);
        glTexCoord2f(1.f, 0.f); glVertex2f(160.f, 144.f);
        glTexCoord2f(0.f, 0.f); glVertex2f(0.f, 144.f);
    glEnd();

    glPopMatrix();
}

static void render_end(
    void* _private
) {
    VOID_TO_SELF(_private);

    SDL_GL_SwapWindow(self->base.window);
}

static void update_game_texture(
    void* _private,
    const struct VideoInterfaceGameTexture* game_texture
) {
    VOID_TO_SELF(_private);

    (void)self; (void)game_texture;

    glBindTexture(GL_TEXTURE_2D, self->texture);

    glTexSubImage2D(
        GL_TEXTURE_2D,
        0, // level
        0, 0, // x,y offset
        160, 144, // width, hieght
        GL_RGBA,
        GL_UNSIGNED_SHORT_1_5_5_5_REV,
        game_texture->pixels
    );
}

static void poll_events(
    void* _private
) {
    VOID_TO_SELF(_private);

    base_sdl2_poll_events(&self->base);
}

static void toggle_fullscreen(
    void* _private
) {
    VOID_TO_SELF(_private);

    base_sdl2_toggle_fullscreen(&self->base);
}

static void set_window_name(
    void* _private,
    const char* name
) {
    VOID_TO_SELF(_private);

    base_sdl2_set_window_name(&self->base, name);
}

static void on_resize(
    void* _private,
    int w, int h
) {
    (void)_private;
    glViewport(0, 0, w, h);
}

struct VideoInterface* video_interface_init_sdl2_opengl(
    const struct VideoInterfaceInfo* info,
    void* user, void (*on_event)(void*, const union VideoInterfaceEvent*)
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
        .render_begin = render_begin,
        .render_game = render_game,
        .render_end = render_end,
        .update_game_texture = update_game_texture,
        .poll_events = poll_events,
        .toggle_fullscreen = toggle_fullscreen,
        .set_window_name = set_window_name,
    };

    // set the internal data!
    *iface = internal_interface;

    const struct BaseConfig base_config = {
        .callbacks = {
            .user = self,
            .on_resize = on_resize
        },
        .user = user,
        .on_event = on_event,
        .window_name = info->window_name,
        .window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE,
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

    if (!base_sdl2_init_system(&self->base)) {
        goto fail;
    }

    #define CHECK(r) if (r) { SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s\n", SDL_GetError()); }

    CHECK(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2));
    CHECK(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0));
    CHECK(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY));
    CHECK(SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5));
    CHECK(SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5));
    CHECK(SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5));
    CHECK(SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16));
    CHECK(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1));
    
    #undef CHECK

    if (!base_sdl2_init_window(&self->base, &base_config)) {
        goto fail;
    }

    self->gl_ctx = SDL_GL_CreateContext(self->base.window);

	if (!self->gl_ctx) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s\n", SDL_GetError());
		goto fail;
	}

	if (SDL_GL_MakeCurrent(self->base.window, self->gl_ctx)) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s\n", SDL_GetError());
		goto fail;
	}

    if (SDL_GL_SetSwapInterval(1)) {
    	SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s\n", SDL_GetError());
		goto fail;
    }

    glGenTextures(1, &self->texture);
    glBindTexture(GL_TEXTURE_2D, self->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(
    	GL_TEXTURE_2D, // type
    	0, // idk
    	GL_RGB5_A1, // internal format
    	160, 144, // width, height
    	0, // idk
    	GL_RGBA, // idk
    	GL_UNSIGNED_SHORT_1_5_5_5_REV, // packing (555, 1 alpha)
    	NULL
    );

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
