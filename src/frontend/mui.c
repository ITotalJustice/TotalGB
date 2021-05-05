#include "mui.h"
#include "mgb.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef MGB_VIDEO_BACKEND_SDL1
    #include "video/sdl1/sdl1.h"
    #define VIDEO_INTERFACE_INIT video_interface_init_sdl1
#elif MGB_VIDEO_BACKEND_SDL1_OPENGL
    #include "video/sdl1/sdl1_opengl.h"
    #define VIDEO_INTERFACE_INIT video_interface_init_sdl1_opengl
	#include "gui/nk/gl2/gl2.h"
	#define NK_INTERFACE_INIT nk_interface_gl2_init
#elif MGB_VIDEO_BACKEND_SDL2
    #include "video/sdl2/sdl2.h"
    #define VIDEO_INTERFACE_INIT video_interface_init_sdl2
#elif MGB_VIDEO_BACKEND_SDL2_OPENGL
    #include "video/sdl2/sdl2_opengl.h"
    #define VIDEO_INTERFACE_INIT video_interface_init_sdl2_opengl
	#include "gui/nk/gl2/gl2.h"
	#define NK_INTERFACE_INIT nk_interface_gl2_init
#elif MGB_VIDEO_BACKEND_SDL2_VULKAN
    #include "video/sdl2/sdl2_vulkan.h"
    #define VIDEO_INTERFACE_INIT video_interface_init_sdl2_vulkan
#elif MGB_VIDEO_BACKEND_ALLEGRO4
    #include "video/allegro4/allegro4.h"
    #define VIDEO_INTERFACE_INIT video_interface_init_allegro4
#elif MGB_VIDEO_BACKEND_ALLEGRO5
    #include "video/allegro5/allegro5.h"
    #define VIDEO_INTERFACE_INIT video_interface_init_allegro5
#else
    #error "NO VIDEO BACKEND SELECTED FOR MGB!"
#endif



// [VIDEO INSTANCE CALLBACKS]
static void on_file_drop(void* user,
    const char* path
);
static void on_key(void* user,
    enum VideoInterfaceKey key, bool down
);
static void on_button(void* user,
    enum VideoInterfaceButton button, bool down
);
static void on_axis(void* user,
    enum VideoInterfaceAxis axis, int16_t pos, bool down
);
static void on_quit(void* user,
    enum VideoInterfaceQuitReason reason
);

static bool setup_video_interface(mui_t* self);
static bool setup_audio_interface(mui_t* self);

static void run_events(mui_t* self);
static void run_state(mui_t* self);
static void run_render(mui_t* self);


// [VIDEO INTERFACE CALLBACKS]
static void on_file_drop(void* user,
    const char* path
) {
    (void)user; (void)path;
}

static void on_key(void* user,
    enum VideoInterfaceKey key, bool down
) {
    mui_t* self = (mui_t*)user;

    switch (self->state) {
		case MuiState_MGB:
			mgb_on_key(&self->mgb, key, down);
			break;

		case MuiState_MUI:
			break;
	}
}

static void on_button(void* user,
    enum VideoInterfaceButton button, bool down
) {
    mui_t* self = (mui_t*)user;

    switch (self->state) {
		case MuiState_MGB:
			mgb_on_button(&self->mgb, button, down);
			break;

		case MuiState_MUI:
			break;
	}
}

static void on_axis(void* user,
    enum VideoInterfaceAxis axis, int16_t pos, bool down
) {
	mui_t* self = (mui_t*)user;

    switch (self->state) {
		case MuiState_MGB:
			mgb_on_axis(&self->mgb, axis, pos, down);
			break;

		case MuiState_MUI:
			break;
	}
}

static void on_quit(void* user,
    enum VideoInterfaceQuitReason reason
) {
	mui_t* self = (mui_t*)user;

	switch (reason) {
		case VideoInterfaceQuitReason_ERROR:
			printf("[MUI] exit requested due to error!\n");
			break;

    	case VideoInterfaceQuitReason_DEFAULT:
    		printf("[MUI] normal exit requested...\n");
    		break;
	}

    self->running = false;
}

static bool setup_video_interface(mui_t* self) {
    if (self->video_interface) {
        video_interface_quit(self->video_interface);
        self->video_interface = NULL;
    }

    const struct VideoInterfaceInfo info = {
        .window_name = "Hello, World!",
        .x = 0,
        .y = 0,
        .w = 160 * 2,
        .h = 144 * 2,
    };

    const struct VideoInterfaceUserCallbacks callbacks = {
        .user = self,
        .on_file_drop = on_file_drop,
        .on_key = on_key,
        .on_button = on_button,
        .on_axis = on_axis,
        .on_quit = on_quit
    };

    self->video_interface = VIDEO_INTERFACE_INIT(
        &info, &callbacks
    );

    return self->video_interface != NULL;
}

static bool setup_audio_interface(mui_t* self) {
    if (self->audio_interface) {
        // audio_interface_quit(self->audio_interface);
        self->audio_interface = NULL;
    }

    return true;

    // return self->audio_interface != NULL;
}

static bool setup_nk_interface(mui_t* self) {
	#ifdef NK_INTERFACE_INIT
		self->nk_interface = NK_INTERFACE_INIT(
			&self->nk_ctx
		);
		return self->nk_interface != NULL;
	#else
		// if we don't have a gui backend, then return true for now
		return true;
	#endif
}

static void run_events(mui_t* self) {
	video_interface_poll_events(self->video_interface);
}

static void run_state(mui_t* self) {
	switch (self->state) {
		case MuiState_MGB:
			mgb_run(&self->mgb);
			break;

		case MuiState_MUI:
			break;
	}
}

static void run_render(mui_t* self) {
    video_interface_render_begin(self->video_interface);
    
    switch (self->state) {
		case MuiState_MGB:
			video_interface_render_game(self->video_interface);
			break;

		case MuiState_MUI:
			break;
	}

    video_interface_render_end(self->video_interface);
}


void mui_loop(mui_t* self) {
    while (self->running == true) {
    	run_events(self);
    	run_state(self);
    	run_render(self);
    }
}

bool mui_init(mui_t* self) {
    if (!self) {
        return false;
    }

    memset(self, 0, sizeof(struct mui));

    if (!mgb_init(&self->mgb)) {
        goto fail;
    }

    if (!setup_video_interface(self)) {
        goto fail;
    }

    if (!setup_audio_interface(self)) {
        goto fail;
    }

    if (!setup_nk_interface(self)) {
    	goto fail;
    }

    self->running = true;
    return true;

fail:
    mui_exit(self);

    return false;
}

void mui_exit(mui_t* self) {
    if (!self) {
        return;
    }

    mgb_exit(&self->mgb);

    if (self->nk_interface) {
    	nk_interface_quit(self->nk_interface);
    	self->nk_interface = NULL;
    }

    if (self->video_interface) {
        video_interface_quit(self->video_interface);
        self->video_interface = NULL;
    }

    if (self->audio_interface) {
        // audio_interface_quit(self->audio_interface);
        self->audio_interface = NULL;
    }
}