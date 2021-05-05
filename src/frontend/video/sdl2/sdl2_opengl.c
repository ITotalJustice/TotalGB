#include "sdl2_opengl.h"
#include "base/base.h"

#include <SDL2/SDL_opengl.h>
#include <stdlib.h>


typedef struct Ctx {
    struct Base base;

    SDL_GLContext gl_self;
    GLuint texture;
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

static void on_resize(
    void* _private,
    int w, int h
) {
    (void)_private;
    glViewport(0, 0, w, h);
}

static void check_err() {
	GLenum err;
	while((err = glGetError()) != GL_NO_ERROR) {
		printf("got gl error 0x%X\n", err);
		exit(-1);
	  // Process/log the error.
	}
}

struct VideoInterface* video_interface_init_sdl2_opengl(
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
        .event_callbacks = {
            .user = self,
            .on_resize = on_resize
        },
        .window_name = info->window_name,
        .window_flags = SDL_WINDOW_OPENGL,
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

    #define CHECK(r, name) \
        if (r) { \
            fprintf(stderr, "failed to set %s\n", name); \
        }

    CHECK(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3), "SDL_GL_CONTEXT_MAJOR_VERSION 4");
    CHECK(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1), "SDL_GL_CONTEXT_MINOR_VERSION 3");
    CHECK(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY), "SDL_GL_CONTEXT_PROFILE_MASK SDL_GL_CONTEXT_PROFILE_CORE");
    CHECK(SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5), "SDL_GL_RED_SIZE 5");
    CHECK(SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5), "SDL_GL_GREEN_SIZE 5");
    CHECK(SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5), "SDL_GL_BLUE_SIZE 5");
    CHECK(SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16), "SDL_GL_DEPTH_SIZE 16");
    CHECK(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1), "SDL_GL_DOUBLEBUFFER 1");
    
    #undef CHECK

    if (!base_sdl2_init_window(&self->base, &base_config, callbacks)) {
        goto fail;
    }

    self->gl_self = SDL_GL_CreateContext(self->base.window);

	if (!self->gl_self) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s\n", SDL_GetError());
		goto fail;
	}

	if (SDL_GL_MakeCurrent(self->base.window, self->gl_self)) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s\n", SDL_GetError());
		goto fail;
	}

    if (SDL_GL_SetSwapInterval(1)) {
    	SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s\n", SDL_GetError());
		goto fail;
    }

    // 0x1be
    glGenTextures(1, &self->texture);
    glBindTexture(GL_TEXTURE_2D, self->texture);
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

    check_err();

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
			glDeleteTextures(1, &self->texture);
			self->texture = 0;
		}

		if (self->gl_self) {
			SDL_GL_DeleteContext(self->gl_self);
			self->gl_self = NULL;
		}

        base_sdl2_exit(&self->base);
    }

    SDL_free(self);
}

static void render_texture(ctx_t* self) {
    // Don't use depth test when writing overlay
    glDisable(GL_DEPTH_TEST);

    // Use precise pixel coordinates
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, 0.0f);

    // Setup 1:1 pixel ration
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    // Sets virtual size to 800x600 (so 0,0 is lower left cornor, and 
    // 800x600 is top right no matter what the size of the window is)
    glOrtho(0.0, 160.0, 0.0, 144.0, -1, 1);

    glBindTexture(GL_TEXTURE_2D, self->texture);
    glEnable(GL_TEXTURE_2D);

    // Draw our background quad
    glBegin(GL_QUADS);
        glNormal3f(0,0,1);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(0.0f, 144.f);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(160.f, 144.f);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(160.f, 0.0f);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
    glEnd();

    glPopMatrix();
}

static void render(
    void* _private
) {
    CTX_FROM_PRIVATE;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    render_texture(self);

    check_err();

    SDL_GL_SwapWindow(self->base.window);
}

static void update_game_texture(
    void* _private,
    const struct VideoInterfaceGameTexture* game_texture
) {
    CTX_FROM_PRIVATE;

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