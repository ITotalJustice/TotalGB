#include "gui.h"
#include "menus/menu.h"
#include "../mgb.h"

#include <stdio.h>
#include <string.h>


#define VOID_TO_SELF(type, v) type* self = (type*)v


static void run_menu(struct mgb* self) {
	struct nk_context* ctx = nk_interface_get_context(self->nk_interface);

	switch (self->gui.menu) {
		case GuiMenu_MAIN:
			draw_menu_main(self, ctx);
			break;

		case GuiMenu_LOADROM:
			draw_menu_loadrom(self, ctx);
			break;

		case GuiMenu_CORE_SETTINGS:
			draw_menu_core_settings(self, ctx);
			break;

		case GuiMenu_GUI_SETTINGS:
			draw_menu_gui_settings(self, ctx);
			break;

		case GuiMenu_HELP:
			draw_menu_help(self, ctx);
			break;
	}
}

void gui_run(
	struct mgb* self
) {
	(void)self;
}

void gui_render(
	struct mgb* self
) {
	run_menu(self);

	nk_interface_render(self->nk_interface, NK_ANTI_ALIASING_ON);
}

void gui_set_window_size(
    struct mgb* self, int w, int h
) {
	self->gui.window_w = w;
	self->gui.window_h = h;
	nk_interface_set_window_size(self->nk_interface, w, h);
}

void gui_set_viewport_size(
    struct mgb* self, int w, int h
) {
	self->gui.viewport_w = w;
	self->gui.viewport_h = h;
    nk_interface_set_viewport_size(self->nk_interface, w, h);
}

bool gui_on_mouse_button(
	struct mgb* self,
    enum VideoInterfaceMouseButton button, int x, int y, bool down
) {
	return nk_interface_on_mouse_button(self->nk_interface,
		button, x, y, down
	);
}

bool gui_on_mouse_motion(
	struct mgb* self,
    int x, int y, int xrel, int yrel
) {
	return nk_interface_on_mouse_motion(self->nk_interface,
		x, y, xrel, yrel
	);
}

bool gui_on_key(
	struct mgb* self,
    enum VideoInterfaceKey key, uint8_t mod, bool down
) {
	return nk_interface_on_key(self->nk_interface,
		key, mod, down
	);
}

bool gui_on_button(
	struct mgb* self,
    enum VideoInterfaceButton button, bool down
) {
	return nk_interface_on_button(self->nk_interface,
		button, down
	);
}

bool gui_on_axis(
	struct mgb* self,
    enum VideoInterfaceAxis axis, int16_t pos, bool down
) {
	return nk_interface_on_axis(self->nk_interface,
		axis, pos, down
	);
}
