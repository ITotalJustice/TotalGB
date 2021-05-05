#include "base.h"

#include <string.h>
#include <assert.h>


static const uint16_t KEY_MAP[512] = {
    [SDLK_q]            = VideoInterfaceKey_Q,
    [SDLK_w]            = VideoInterfaceKey_W,
    [SDLK_e]            = VideoInterfaceKey_E,
    [SDLK_r]            = VideoInterfaceKey_R,
    [SDLK_t]            = VideoInterfaceKey_T,
    [SDLK_y]            = VideoInterfaceKey_Y,
    [SDLK_u]            = VideoInterfaceKey_U,
    [SDLK_i]            = VideoInterfaceKey_I,
    [SDLK_o]            = VideoInterfaceKey_O,
    [SDLK_p]            = VideoInterfaceKey_P,
    [SDLK_a]            = VideoInterfaceKey_A,
    [SDLK_s]            = VideoInterfaceKey_S,
    [SDLK_d]            = VideoInterfaceKey_D,
    [SDLK_f]            = VideoInterfaceKey_F,
    [SDLK_g]            = VideoInterfaceKey_G,
    [SDLK_h]            = VideoInterfaceKey_H,
    [SDLK_j]            = VideoInterfaceKey_J,
    [SDLK_k]            = VideoInterfaceKey_K,
    [SDLK_l]            = VideoInterfaceKey_L,
    [SDLK_z]            = VideoInterfaceKey_Z,
    [SDLK_x]            = VideoInterfaceKey_X,
    [SDLK_c]            = VideoInterfaceKey_C,
    [SDLK_v]            = VideoInterfaceKey_V,
    [SDLK_b]            = VideoInterfaceKey_B,
    [SDLK_n]            = VideoInterfaceKey_N,
    [SDLK_m]            = VideoInterfaceKey_M,

    [SDLK_0]            = VideoInterfaceKey_0,
    [SDLK_1]            = VideoInterfaceKey_1,
    [SDLK_2]            = VideoInterfaceKey_2,
    [SDLK_3]            = VideoInterfaceKey_3,
    [SDLK_4]            = VideoInterfaceKey_4,
    [SDLK_5]            = VideoInterfaceKey_5,
    [SDLK_6]            = VideoInterfaceKey_6,
    [SDLK_7]            = VideoInterfaceKey_7,
    [SDLK_8]            = VideoInterfaceKey_8,
    [SDLK_9]            = VideoInterfaceKey_9,

    [SDLK_F1]           = VideoInterfaceKey_F1,
    [SDLK_F2]           = VideoInterfaceKey_F2,
    [SDLK_F3]           = VideoInterfaceKey_F3,
    [SDLK_F4]           = VideoInterfaceKey_F4,
    [SDLK_F5]           = VideoInterfaceKey_F5,
    [SDLK_F6]           = VideoInterfaceKey_F6,
    [SDLK_F7]           = VideoInterfaceKey_F7,
    [SDLK_F8]           = VideoInterfaceKey_F8,
    [SDLK_F9]           = VideoInterfaceKey_F9,
    [SDLK_F10]          = VideoInterfaceKey_F10,
    [SDLK_F11]          = VideoInterfaceKey_F11,
    [SDLK_F12]          = VideoInterfaceKey_F12,

    [SDLK_RETURN]       = VideoInterfaceKey_ENTER,
    [SDLK_BACKSPACE]    = VideoInterfaceKey_BACKSPACE,
    [SDLK_SPACE]        = VideoInterfaceKey_SPACE,
    [SDLK_ESCAPE]       = VideoInterfaceKey_ESCAPE,
    [SDLK_DELETE]       = VideoInterfaceKey_DELETE,
    [SDLK_LSHIFT]       = VideoInterfaceKey_LSHIFT,
    [SDLK_RSHIFT]       = VideoInterfaceKey_RSHIFT,

    [SDLK_UP]           = VideoInterfaceKey_UP,
    [SDLK_DOWN]         = VideoInterfaceKey_DOWN,
    [SDLK_LEFT]         = VideoInterfaceKey_LEFT,
    [SDLK_RIGHT]        = VideoInterfaceKey_RIGHT,
};

static const uint8_t MOUSE_BUTTON_MAP[256] = {
    [SDL_BUTTON_LEFT]   = VideoInterfaceMouseButton_LEFT,
    [SDL_BUTTON_MIDDLE] = VideoInterfaceMouseButton_MIDDLE,
    [SDL_BUTTON_RIGHT]  = VideoInterfaceMouseButton_RIGHT
};

// sdl events
static void OnQuitEvent(
    struct Base* self, const SDL_QuitEvent* e
) {
    (void)e;

    self->user_callbacks.on_quit(
        self->user_callbacks.user, VideoInterfaceQuitReason_DEFAULT
    );
}

static void OnActiveEvent(
	struct Base* self, const SDL_ActiveEvent* e
) {
	(void)self; (void)e;
}

static void OnVideoResizeEvent(
	struct Base* self, const SDL_ResizeEvent* e
) {
	// in SDL1, we have to get a new video mode...
	self->window = SDL_SetVideoMode(
		e->w, e->h,
		self->window->format->BitsPerPixel,
		self->window->flags
	);

	self->event_callbacks.on_resize(
		self->event_callbacks.user,
		e->w, e->h
	);

    self->user_callbacks.on_resize(
        self->user_callbacks.user,
        e->w, e->h
    );
}

static void OnVideoExposeEvent(
	struct Base* self, const SDL_ExposeEvent* e
) {
	(void)self; (void)e;
}

static void OnMouseButtonEvent(
    struct Base* self, const SDL_MouseButtonEvent* e
) {
    assert(self->user_callbacks.on_mouse_button);

    if (MOUSE_BUTTON_MAP[e->button]) {
        self->user_callbacks.on_mouse_button(self->user_callbacks.user,
            MOUSE_BUTTON_MAP[e->button], e->x, e->y,
            e->type == SDL_MOUSEBUTTONDOWN
        );
    }
}

static void OnMouseMotionEvent(
    struct Base* self, const SDL_MouseMotionEvent* e
) {
    assert(self->user_callbacks.on_mouse_motion);

    self->user_callbacks.on_mouse_motion(self->user_callbacks.user,
        e->x, e->y, e->xrel, e->yrel
    );
}

static void OnKeyEvent(
    struct Base* self, const SDL_KeyboardEvent* e
) {
    assert(self->user_callbacks.on_key);

    // only handle if we have mapped the key.
    // the emun starts at 1, so all values are > 0.
    if (KEY_MAP[e->keysym.sym]) {
        const uint16_t key = KEY_MAP[e->keysym.sym];
        const bool down = e->type == SDL_KEYDOWN;
        uint8_t mod = VideoInterfaceKeyMod_NONE;

        // idk how to map a bitfield in an array sainly...
        if (e->keysym.mod & KMOD_LSHIFT) {
            mod |= VideoInterfaceKeyMod_LSHIFT;
        }
        if (e->keysym.mod & KMOD_RSHIFT) {
            mod |= VideoInterfaceKeyMod_RSHIFT;
        }
        if (e->keysym.mod & KMOD_LCTRL) {
            mod |= VideoInterfaceKeyMod_LCTRL;
        }
        if (e->keysym.mod & KMOD_RCTRL) {
            mod |= VideoInterfaceKeyMod_RCTRL;
        }
        if (e->keysym.mod & KMOD_LALT) {
            mod |= VideoInterfaceKeyMod_LALT;
        }
        if (e->keysym.mod & KMOD_RALT) {
            mod |= VideoInterfaceKeyMod_RALT;
        }
        if (e->keysym.mod & KMOD_NUM) {
            mod |= VideoInterfaceKeyMod_NUM;
        }
        if (e->keysym.mod & KMOD_CAPS) {
            mod |= VideoInterfaceKeyMod_CAPS;
        }

        self->user_callbacks.on_key(self->user_callbacks.user,
            key, mod, down
        );
    }    
}

static void OnJoypadAxisEvent(
    struct Base* self, const SDL_JoyAxisEvent* e
) {
    (void)self; (void)e;
}

static void OnJoypadButtonEvent(
    struct Base* self, const SDL_JoyButtonEvent* e
) {
    (void)self; (void)e;
}

static void OnJoypadHatEvent(
    struct Base* self, const SDL_JoyHatEvent* e
) {
    (void)self; (void)e;
}

static void OnSysWMEvent(
    struct Base* self, const SDL_SysWMEvent* e
) {
    (void)self; (void)e;
}

static void OnUserEvent(
    struct Base* self, SDL_UserEvent* e
) {
    (void)self; (void)e;
}


bool base_sdl1_init_system(
    struct Base* self
) {
    memset(self, 0, sizeof(struct Base));

    if (SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        fprintf(stderr, "%s\n\n", SDL_GetError());
        goto fail;
    }

    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK)) {
        fprintf(stderr, "%s\n\n", SDL_GetError());
        goto fail;
    }

    const SDL_VideoInfo* video_info = SDL_GetVideoInfo();

    // log the best video info
    printf("\nBest Video Info:\n");
    printf("\thw_available:\t%s\n", video_info->hw_available ? "TRUE" : "FALE");
    printf("\twm_available:\t%s\n", video_info->wm_available ? "TRUE" : "FALE");
    printf("\tblit_hw:\t%s\n", video_info->blit_hw ? "TRUE" : "FALE");
    printf("\tblit_hw_CC:\t%s\n", video_info->blit_hw_CC ? "TRUE" : "FALE");
    printf("\tblit_hw_A:\t%s\n", video_info->blit_hw_A ? "TRUE" : "FALE");
    printf("\tblit_sw:\t%s\n", video_info->blit_sw ? "TRUE" : "FALE");
    printf("\tblit_sw_CC:\t%s\n", video_info->blit_sw_CC ? "TRUE" : "FALE");
    printf("\tblit_sw_A:\t%s\n", video_info->blit_sw_A ? "TRUE" : "FALE");
    printf("\tblit_fill:\t%u\n", video_info->blit_fill);
    printf("\tvideo_mem:\t%u\n", video_info->video_mem);

    return true;

fail:
    base_sdl1_exit(self);
    return false;
}

bool base_sdl1_init_window(
    struct Base* self,
    const struct BaseConfig* config
) {
    assert(SDL_WasInit(SDL_INIT_VIDEO));
    assert(SDL_WasInit(SDL_INIT_JOYSTICK));

    self->window = SDL_SetVideoMode(
        config->w, config->h,
        config->bpp, config->window_flags
    );

    if (!self->window) {
        fprintf(stderr, "failed to create screen: %s\n", SDL_GetError());
        goto fail;
    }

    // check which flags where set!
    printf("\nWindow Surface Flags:\n");

    if (self->window->flags & SDL_SWSURFACE) {
        printf("\tSDL_SWSURFACE\n");
    }
    if (self->window->flags & SDL_HWSURFACE) {
        printf("\tSDL_OPENGL\n");
    }
    if (self->window->flags & SDL_ASYNCBLIT) {
        printf("\tSDL_ASYNCBLIT\n");
    }
    if (self->window->flags & SDL_HWPALETTE) {
        printf("\tSDL_HWPALETTE\n");
    }
    if (self->window->flags & SDL_DOUBLEBUF) {
        printf("\tSDL_DOUBLEBUF\n");
    }
    if (self->window->flags & SDL_FULLSCREEN) {
        printf("\tSDL_FULLSCREEN\n");
    }
    if (self->window->flags & SDL_OPENGL) {
        printf("\tSDL_OPENGL\n");
    }
    if (self->window->flags & SDL_OPENGLBLIT) {
        printf("\tSDL_OPENGLBLIT\n");
    }
    if (self->window->flags & SDL_RESIZABLE) {
        printf("\tSDL_RESIZABLE\n");
    }
    if (self->window->flags & SDL_NOFRAME) {
        printf("\tSDL_NOFRAME\n");
    }

    // save the callbacks
    self->event_callbacks = config->event_callbacks;
    self->user_callbacks = config->user_callbacks;

    return true;

fail:
    base_sdl1_exit(self);
    return false;
}

void base_sdl1_exit(struct Base* self) {
    if (SDL_WasInit(SDL_INIT_JOYSTICK)) {
        SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    }

    if (SDL_WasInit(SDL_INIT_VIDEO)) {
        if (self->window) {
            // The framebuffer surface, or NULL if it fails.
            // The surface returned is freed by SDL_Quit()
            // and should nt be freed by the caller.
            // SOURCE: https://www.libsdl.org/release/SDL-1.2.15/docs/html/sdlsetvideomode.html
        }

        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }
}

void base_sdl1_poll_events(struct Base* self) {
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
                OnQuitEvent(self, &e.quit);
                break;

            case SDL_ACTIVEEVENT:
                OnActiveEvent(self, &e.active);
                break;

            case SDL_VIDEORESIZE:
                OnVideoResizeEvent(self, &e.resize);
                break;

            case SDL_VIDEOEXPOSE:
                OnVideoExposeEvent(self, &e.expose);
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                OnMouseButtonEvent(self, &e.button);
                break;

            case SDL_MOUSEMOTION:
                OnMouseMotionEvent(self, &e.motion);
                break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                OnKeyEvent(self, &e.key);
                break;

            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
                OnJoypadButtonEvent(self, &e.jbutton);
                break;

            case SDL_JOYAXISMOTION:
                OnJoypadAxisEvent(self, &e.jaxis);
                break;

            case SDL_JOYHATMOTION:
                OnJoypadHatEvent(self, &e.jhat);
                break;

            case SDL_SYSWMEVENT:
                OnSysWMEvent(self, &e.syswm);
                break;

            case SDL_USEREVENT:
                OnUserEvent(self, &e.user);
                break;
        }
    }
}

void base_sdl1_toggle_fullscreen(struct Base* self) {
    if (SDL_WM_ToggleFullScreen(self->window) == 0) {
        fprintf(stderr, "%s\n", SDL_GetError());
    }
    // NOTE: sdl fires a resize event after
}

void base_sdl1_set_window_name(struct Base* self, const char* name) {
    (void)self;

    // name, icon name (set icon name to window name)
    SDL_WM_SetCaption(name, name);
}
