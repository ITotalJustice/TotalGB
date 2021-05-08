#include "base.h"


static const uint8_t CONTROLLER_BUTTON_MAP[SDL_CONTROLLER_BUTTON_MAX] = {
    [SDL_CONTROLLER_BUTTON_A]               = VideoInterfaceControllerButton_A,
    [SDL_CONTROLLER_BUTTON_B]               = VideoInterfaceControllerButton_B,
    [SDL_CONTROLLER_BUTTON_X]               = VideoInterfaceControllerButton_X,
    [SDL_CONTROLLER_BUTTON_Y]               = VideoInterfaceControllerButton_Y,

    [SDL_CONTROLLER_BUTTON_START]           = VideoInterfaceControllerButton_START,
    [SDL_CONTROLLER_BUTTON_BACK]            = VideoInterfaceControllerButton_SELECT,
    [SDL_CONTROLLER_BUTTON_GUIDE]           = VideoInterfaceControllerButton_HOME,

    [SDL_CONTROLLER_BUTTON_LEFTSHOULDER]    = VideoInterfaceControllerButton_L1,
    [SDL_CONTROLLER_BUTTON_LEFTSTICK]       = VideoInterfaceControllerButton_L3,
    [SDL_CONTROLLER_BUTTON_RIGHTSHOULDER]   = VideoInterfaceControllerButton_R1,
    [SDL_CONTROLLER_BUTTON_RIGHTSTICK]      = VideoInterfaceControllerButton_R3,

    [SDL_CONTROLLER_BUTTON_DPAD_UP]         = VideoInterfaceControllerButton_UP,
    [SDL_CONTROLLER_BUTTON_DPAD_DOWN]       = VideoInterfaceControllerButton_DOWN,
    [SDL_CONTROLLER_BUTTON_DPAD_LEFT]       = VideoInterfaceControllerButton_LEFT,
    [SDL_CONTROLLER_BUTTON_DPAD_RIGHT]      = VideoInterfaceControllerButton_RIGHT,
};

static const uint8_t CONTROLLER_AXIS_MAP[SDL_CONTROLLER_AXIS_MAX] = {
    [SDL_CONTROLLER_AXIS_TRIGGERLEFT]   = VideoInterfaceControllerAxis_L2,
    [SDL_CONTROLLER_AXIS_TRIGGERRIGHT]  = VideoInterfaceControllerAxis_R2,

    [SDL_CONTROLLER_AXIS_LEFTX]         = VideoInterfaceControllerAxis_LEFTX,
    [SDL_CONTROLLER_AXIS_LEFTY]         = VideoInterfaceControllerAxis_LEFTY,
    [SDL_CONTROLLER_AXIS_RIGHTX]        = VideoInterfaceControllerAxis_RIGHTX,
    [SDL_CONTROLLER_AXIS_RIGHTY]        = VideoInterfaceControllerAxis_RIGHTY,
};

static const uint8_t MOUSE_BUTTON_MAP[256] = {
    [SDL_BUTTON_LEFT]   = VideoInterfaceMouseButton_LEFT,
    [SDL_BUTTON_MIDDLE] = VideoInterfaceMouseButton_MIDDLE,
    [SDL_BUTTON_RIGHT]  = VideoInterfaceMouseButton_RIGHT
};

static const uint16_t KEY_MAP[SDL_NUM_SCANCODES] = {
    [SDL_SCANCODE_Q]            = VideoInterfaceKey_Q,
    [SDL_SCANCODE_W]            = VideoInterfaceKey_W,
    [SDL_SCANCODE_E]            = VideoInterfaceKey_E,
    [SDL_SCANCODE_R]            = VideoInterfaceKey_R,
    [SDL_SCANCODE_T]            = VideoInterfaceKey_T,
    [SDL_SCANCODE_Y]            = VideoInterfaceKey_Y,
    [SDL_SCANCODE_U]            = VideoInterfaceKey_U,
    [SDL_SCANCODE_I]            = VideoInterfaceKey_I,
    [SDL_SCANCODE_O]            = VideoInterfaceKey_O,
    [SDL_SCANCODE_P]            = VideoInterfaceKey_P,
    [SDL_SCANCODE_A]            = VideoInterfaceKey_A,
    [SDL_SCANCODE_S]            = VideoInterfaceKey_S,
    [SDL_SCANCODE_D]            = VideoInterfaceKey_D,
    [SDL_SCANCODE_F]            = VideoInterfaceKey_F,
    [SDL_SCANCODE_G]            = VideoInterfaceKey_G,
    [SDL_SCANCODE_H]            = VideoInterfaceKey_H,
    [SDL_SCANCODE_J]            = VideoInterfaceKey_J,
    [SDL_SCANCODE_K]            = VideoInterfaceKey_K,
    [SDL_SCANCODE_L]            = VideoInterfaceKey_L,
    [SDL_SCANCODE_Z]            = VideoInterfaceKey_Z,
    [SDL_SCANCODE_X]            = VideoInterfaceKey_X,
    [SDL_SCANCODE_C]            = VideoInterfaceKey_C,
    [SDL_SCANCODE_V]            = VideoInterfaceKey_V,
    [SDL_SCANCODE_B]            = VideoInterfaceKey_B,
    [SDL_SCANCODE_N]            = VideoInterfaceKey_N,
    [SDL_SCANCODE_M]            = VideoInterfaceKey_M,

    [SDL_SCANCODE_0]            = VideoInterfaceKey_0,
    [SDL_SCANCODE_1]            = VideoInterfaceKey_1,
    [SDL_SCANCODE_2]            = VideoInterfaceKey_2,
    [SDL_SCANCODE_3]            = VideoInterfaceKey_3,
    [SDL_SCANCODE_4]            = VideoInterfaceKey_4,
    [SDL_SCANCODE_5]            = VideoInterfaceKey_5,
    [SDL_SCANCODE_6]            = VideoInterfaceKey_6,
    [SDL_SCANCODE_7]            = VideoInterfaceKey_7,
    [SDL_SCANCODE_8]            = VideoInterfaceKey_8,
    [SDL_SCANCODE_9]            = VideoInterfaceKey_9,

    [SDL_SCANCODE_F1]           = VideoInterfaceKey_F1,
    [SDL_SCANCODE_F2]           = VideoInterfaceKey_F2,
    [SDL_SCANCODE_F3]           = VideoInterfaceKey_F3,
    [SDL_SCANCODE_F4]           = VideoInterfaceKey_F4,
    [SDL_SCANCODE_F5]           = VideoInterfaceKey_F5,
    [SDL_SCANCODE_F6]           = VideoInterfaceKey_F6,
    [SDL_SCANCODE_F7]           = VideoInterfaceKey_F7,
    [SDL_SCANCODE_F8]           = VideoInterfaceKey_F8,
    [SDL_SCANCODE_F9]           = VideoInterfaceKey_F9,
    [SDL_SCANCODE_F10]          = VideoInterfaceKey_F10,
    [SDL_SCANCODE_F11]          = VideoInterfaceKey_F11,
    [SDL_SCANCODE_F12]          = VideoInterfaceKey_F12,

    [SDL_SCANCODE_RETURN]       = VideoInterfaceKey_ENTER,
    [SDL_SCANCODE_BACKSPACE]    = VideoInterfaceKey_BACKSPACE,
    [SDL_SCANCODE_SPACE]        = VideoInterfaceKey_SPACE,
    [SDL_SCANCODE_ESCAPE]       = VideoInterfaceKey_ESCAPE,
    [SDL_SCANCODE_DELETE]       = VideoInterfaceKey_DELETE,
    [SDL_SCANCODE_LSHIFT]       = VideoInterfaceKey_LSHIFT,
    [SDL_SCANCODE_RSHIFT]       = VideoInterfaceKey_RSHIFT,

    [SDL_SCANCODE_UP]           = VideoInterfaceKey_UP,
    [SDL_SCANCODE_DOWN]         = VideoInterfaceKey_DOWN,
    [SDL_SCANCODE_LEFT]         = VideoInterfaceKey_LEFT,
    [SDL_SCANCODE_RIGHT]        = VideoInterfaceKey_RIGHT,
};

static void push_user_event(
    struct Base* self, const union VideoInterfaceEvent* e
) {
    self->on_event(self->user, e);
}

// sdl events
static void OnQuitEvent(
    struct Base* self, const SDL_QuitEvent* e
) {
    (void)e;

    const union VideoInterfaceEvent event = {
        .quit = {
            .type = VideoInterfaceEventType_QUIT,
            .reason = VideoInterfaceQuitReason_DEFAULT
        }
    };

    push_user_event(self, &event);
}

static void OnDropEvent(
    struct Base* self, SDL_DropEvent* e
) {
    switch (e->type) {
        case SDL_DROPFILE:
            if (e->file != NULL) {
                const union VideoInterfaceEvent event = {
                    .file_drop = {
                        .type = VideoInterfaceEventType_FILE_DROP,
                        .path = e->file
                    }
                };

                push_user_event(self, &event);
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

static void OnWindowEvent(
    struct Base* self, const SDL_WindowEvent* e
) {
    (void)self;

    switch (e->event) {
        case SDL_WINDOWEVENT_SHOWN:
        case SDL_WINDOWEVENT_EXPOSED:
        case SDL_WINDOWEVENT_RESTORED: {
            const union VideoInterfaceEvent event = {
                .shown = {
                    .type = VideoInterfaceEventType_SHOWN,
                }
            };

            push_user_event(self, &event);
        }   break;

        case SDL_WINDOWEVENT_MINIMIZED:
        case SDL_WINDOWEVENT_HIDDEN: {
            const union VideoInterfaceEvent event = {
                .hidden = {
                    .type = VideoInterfaceEventType_HIDDEN,
                }
            };

            push_user_event(self, &event);
        }   break;

        case SDL_WINDOWEVENT_MOVED:
            break;

        case SDL_WINDOWEVENT_RESIZED:
        case SDL_WINDOWEVENT_SIZE_CHANGED: {
            const union VideoInterfaceEvent event = {
                .resize = {
                    .type = VideoInterfaceEventType_RESIZE,
                    .w = e->data1,
                    .h = e->data2,
                    .display_w = e->data1,
                    .display_h = e->data2,
                }
            };

            push_user_event(self, &event);

            self->callbacks.on_resize(
                self->callbacks.user, e->data1, e->data2
            );
        }   break;

        case SDL_WINDOWEVENT_MAXIMIZED:
            break;

        case SDL_WINDOWEVENT_ENTER:
            break;

        case SDL_WINDOWEVENT_LEAVE:
            break;

        case SDL_WINDOWEVENT_FOCUS_GAINED:
            break;

        case SDL_WINDOWEVENT_FOCUS_LOST:
            break;

        case SDL_WINDOWEVENT_CLOSE:
            break;

#if SDL_VERSION_ATLEAST(2, 0, 5)
        case SDL_WINDOWEVENT_TAKE_FOCUS:
            break;

        case SDL_WINDOWEVENT_HIT_TEST:
            break;
#endif // SDL_VERSION_ATLEAST(2, 0, 5)
    }
}

#if SDL_VERSION_ATLEAST(2, 0, 4)
static void OnAudioDeviceEvent(
    struct Base* self, const SDL_AudioDeviceEvent* e
) {
    (void)self; (void)e;
}
#endif // SDL_VERSION_ATLEAST(2, 0, 4)

#if SDL_VERSION_ATLEAST(2, 0, 9)
static void OnDisplayEvent(
    struct Base* self, const SDL_DisplayEvent* e
) {
    (void)self;

    switch (e->event) {
        case SDL_DISPLAYEVENT_NONE:
            SDL_Log("[SDL2] SDL_DISPLAYEVENT_NONE\n");
            break;

        case SDL_DISPLAYEVENT_ORIENTATION:
            SDL_Log("[SDL2] SDL_DISPLAYEVENT_ORIENTATION\n");
            break;
   }
}
#endif // SDL_VERSION_ATLEAST(2, 0, 9)

static void OnMouseButtonEvent(
    struct Base* self, const SDL_MouseButtonEvent* e
) {
    if (e->which == SDL_TOUCH_MOUSEID) {
        // don't hadle touch events
        return;
    }

    if (MOUSE_BUTTON_MAP[e->button]) {
        const union VideoInterfaceEvent event = {
            .mbutton = {
                .type = VideoInterfaceEventType_MBUTTON,
                .button = MOUSE_BUTTON_MAP[e->button],
                .x = e->x,
                .y = e->y,
                .down = e->type == SDL_MOUSEBUTTONDOWN
            }
        };

        push_user_event(self, &event);
    }
}

static void OnMouseMotionEvent(
    struct Base* self, const SDL_MouseMotionEvent* e
) {
    const union VideoInterfaceEvent event = {
        .mmotion = {
            .type = VideoInterfaceEventType_MMOTION,
            .x = e->x,
            .y = e->y,
            .xrel = e->xrel,
            .yrel = e->yrel,
        }
    };

    push_user_event(self, &event);
}

static void OnMouseWheelEvent(
    struct Base* self, const SDL_MouseWheelEvent* e
) {
    (void)self; (void)e;
}

static void OnKeyEvent(
    struct Base* self, const SDL_KeyboardEvent* e
) {
    // only handle if we have mapped the key.
    // the emun starts at 1, so all values are > 0.
    if (KEY_MAP[e->keysym.scancode]) {
        uint16_t mod = VideoInterfaceKeyMod_NONE;

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

        const union VideoInterfaceEvent event = {
            .key = {
                .type = VideoInterfaceEventType_KEY,
                .key = KEY_MAP[e->keysym.scancode],
                .mod = mod,
                .down = e->type == SDL_KEYDOWN
            }
        };

        push_user_event(self, &event);
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

static void OnJoypadDeviceEvent(
    struct Base* self, const SDL_JoyDeviceEvent* e
) {
    (void)self; (void)e;
}

static void OnControllerAxisEvent(
    struct Base* self, const SDL_ControllerAxisEvent* e
) {
    // sdl recommends deadzone of 8000
    enum {
        DEADZONE = 8000,
        LEFT     = -DEADZONE,
        RIGHT    = +DEADZONE,
        UP       = -DEADZONE,
        DOWN     = +DEADZONE,
    };

    if (CONTROLLER_AXIS_MAP[e->axis]) {
        bool down = false;

        switch (e->axis) {
            case SDL_CONTROLLER_AXIS_LEFTX:
            case SDL_CONTROLLER_AXIS_RIGHTX:
                if (e->value < LEFT) {
                    down = true;
                }
                else if (e->value > RIGHT) {
                    down = false;
                }
                else {
                    return;
                }
                break;

            case SDL_CONTROLLER_AXIS_LEFTY:
            case SDL_CONTROLLER_AXIS_RIGHTY:
                if (e->value < UP) {
                    down = true;
                }
                else if (e->value > DOWN) {
                    down = false;
                }
                else {
                    return;
                }
                break;

            default:
                return;
        }

        const union VideoInterfaceEvent event = {
            .caxis = {
                .type = VideoInterfaceEventType_CAXIS,
                .axis = CONTROLLER_AXIS_MAP[e->axis],
                .pos = e->value,
                .down = down
            }
        };

        push_user_event(self, &event);
    }
}

static void OnControllerButtonEvent(
    struct Base* self, const SDL_ControllerButtonEvent* e
) {
    if (CONTROLLER_BUTTON_MAP[e->button]) {
        const union VideoInterfaceEvent event = {
            .cbutton = {
                .type = VideoInterfaceEventType_CBUTTON,
                .button = CONTROLLER_BUTTON_MAP[e->button],
                .down = e->type == SDL_CONTROLLERBUTTONDOWN
            }
        };

        push_user_event(self, &event);
    }
}

static void OnControllerDeviceEvent(
    struct Base* self, const SDL_ControllerDeviceEvent* e
) {
    (void)self; (void)e;
}

static void OnTouchEvent(
    struct Base* self, const SDL_TouchFingerEvent* e
) {
    (void)self; (void)e;
}

static void OnMultiGestureEvent(
    struct Base* self, const SDL_MultiGestureEvent* e
) {
    (void)self; (void)e;
}

static void OnDollarGestureEvent(
    struct Base* self, const SDL_DollarGestureEvent* e
) {
    (void)self; (void)e;
}

static void OnTextEditEvent(
    struct Base* self, const SDL_TextEditingEvent* e
) {
    (void)self; (void)e;
}

static void OnTextInputEvent(
    struct Base* self, const SDL_TextInputEvent* e
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


bool base_sdl2_init_system(
    struct Base* self
) {
    if (SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s\n\n", SDL_GetError());
        goto fail;
    }

    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s\n\n", SDL_GetError());
        goto fail;
    }

    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s\n\n", SDL_GetError());
        goto fail;
    }

    return true;

fail:
    base_sdl2_exit(self);
    return false;
}

bool base_sdl2_init_window(
	struct Base* self,
	const struct BaseConfig* config
) {
    self->window = SDL_CreateWindow(
        config->window_name,
        config->x, config->y, config->w, config->h,
        config->window_flags
    );

    if (!self->window) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s\n", SDL_GetError());
        goto fail;
    }

    if (config->set_min_win) {
	    SDL_SetWindowMinimumSize(
	    	self->window, config->min_win_w, config->min_win_h
	    );
	}

	if (config->set_max_win) {
	    SDL_SetWindowMaximumSize(
	    	self->window, config->max_win_w, config->max_win_h
	    );
	}

    // save the user callbacks
    self->user = config->user;
    self->on_event = config->on_event;
    self->callbacks = config->callbacks;

    return true;

fail:
	base_sdl2_exit(self);
	return false;
}

void base_sdl2_exit(struct Base* self) {
	if (SDL_WasInit(SDL_INIT_JOYSTICK)) {
        SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    }

    if (SDL_WasInit(SDL_INIT_GAMECONTROLLER)) {
        SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    }
    
    if (SDL_WasInit(SDL_INIT_VIDEO)) {
		if (self->window) {
	        SDL_DestroyWindow(self->window);
	    }

	    SDL_QuitSubSystem(SDL_INIT_VIDEO);
	}
}

void base_sdl2_poll_events(struct Base* self) {
	SDL_Event e;

    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
                OnQuitEvent(self, &e.quit);
                break;

            case SDL_DROPFILE:
            case SDL_DROPTEXT:
            case SDL_DROPBEGIN:
            case SDL_DROPCOMPLETE:
                OnDropEvent(self, &e.drop);
                break;

#if SDL_VERSION_ATLEAST(2, 0, 9)
            case SDL_DISPLAYEVENT:
                OnDisplayEvent(self, &e.display);
                break;
#endif // SDL_VERSION_ATLEAST(2, 0, 9)

            case SDL_WINDOWEVENT:
                OnWindowEvent(self, &e.window);
                break;

#if SDL_VERSION_ATLEAST(2, 0, 4)
            case SDL_AUDIODEVICEADDED:
            case SDL_AUDIODEVICEREMOVED:
                OnAudioDeviceEvent(self, &e.adevice);
                break;
#endif // SDL_VERSION_ATLEAST(2, 0, 4)

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                OnMouseButtonEvent(self, &e.button);
                break;

            case SDL_MOUSEMOTION:
                OnMouseMotionEvent(self, &e.motion);
                break;

            case SDL_MOUSEWHEEL:
                OnMouseWheelEvent(self, &e.wheel);
                break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                OnKeyEvent(self, &e.key);
                break;

            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
                OnControllerButtonEvent(self, &e.cbutton);
                break;

            case SDL_CONTROLLERAXISMOTION:
                OnControllerAxisEvent(self, &e.caxis);
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

            case SDL_JOYDEVICEADDED:
            case SDL_JOYDEVICEREMOVED:
                OnJoypadDeviceEvent(self, &e.jdevice);
                break;

            case SDL_CONTROLLERDEVICEADDED:
            case SDL_CONTROLLERDEVICEREMOVED:
                OnControllerDeviceEvent(self, &e.cdevice);
                break;

            case SDL_FINGERDOWN:
            case SDL_FINGERUP:
            case SDL_FINGERMOTION:
                OnTouchEvent(self, &e.tfinger);
                break;

            case SDL_TEXTEDITING:
                OnTextEditEvent(self, &e.edit);
                break;

            case SDL_TEXTINPUT:
                OnTextInputEvent(self, &e.text);
                break;

            case SDL_SYSWMEVENT:
                OnSysWMEvent(self, &e.syswm);
                break;

            case SDL_USEREVENT:
                OnUserEvent(self, &e.user);
                break;

            case SDL_MULTIGESTURE:
                OnMultiGestureEvent(self, &e.mgesture);
                break;

            case SDL_DOLLARGESTURE:
                OnDollarGestureEvent(self, &e.dgesture);
                break;
        }
    }
}

void base_sdl2_toggle_fullscreen(struct Base* self) {
	const int flags = SDL_GetWindowFlags(self->window);

    // check if we are already in fullscreen mode
    if (flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) {
        SDL_SetWindowFullscreen(self->window, 0);
    }
    else {
        SDL_SetWindowFullscreen(self->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
}

void base_sdl2_set_window_name(struct Base* self, const char* name) {
	SDL_SetWindowTitle(self->window, name);
}
