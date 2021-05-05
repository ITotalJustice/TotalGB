#pragma once

#include "mgb.h"
#include "types.h"
#include "util.h"

#include "video/interface.h"
#include "audio/interface.h"
#include "gui/nk/interface.h"

enum MuiState {
	// all inputs will be sent to mgb
	MuiState_MGB,
	// all inputs will be sent to mui, mgb is paused
	MuiState_MUI,
};

typedef struct mui {
	struct VideoInterface* video_interface;
    struct AudioInterface* audio_interface;
    struct NkInterface* nk_interface;
    struct nk_context* nk_ctx;

	struct mgb mgb;
	
	enum MuiState state;

	bool running;
} mui_t;


bool mui_init(mui_t* self);
void mui_exit(mui_t* self);
void mui_loop(mui_t* self);
