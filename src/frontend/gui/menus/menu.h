#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#include "../gui.h"


struct Button {
	const char* const title;
	void (*callback)(struct mgb* self, struct nk_context* ctx);
};


void change_menu(
	struct mgb* self, enum GuiMenu menu
);

void draw_menu_button(
	struct mgb* self, struct nk_context* ctx,
	const struct Button* button, bool active
);

void draw_menu_main(
	struct mgb* self, struct nk_context* ctx
);

void draw_menu_loadrom(
	struct mgb* self, struct nk_context* ctx
);

void draw_menu_core_settings(
	struct mgb* self, struct nk_context* ctx
);

void draw_menu_gui_settings(
	struct mgb* self, struct nk_context* ctx
);

void draw_menu_help(
	struct mgb* self, struct nk_context* ctx
);

#ifdef __cplusplus
}
#endif
