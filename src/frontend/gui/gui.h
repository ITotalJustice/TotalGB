#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../video/interface.h"
#include "../audio/interface.h"
#include "../gui/nk/interface.h"

struct mgb;


enum GuiMenu {
	GuiMenu_MAIN,
	GuiMenu_LOADROM,
	GuiMenu_CORE_SETTINGS,
	GuiMenu_GUI_SETTINGS,
	GuiMenu_HELP,
};

enum GuiMainMenuOptions {
	GuiMainMenuOptions_LOADROM,
	GuiMainMenuOptions_CORE_SETTINGS,
	GuiMainMenuOptions_GUI_SETTINGS,
	GuiMainMenuOptions_HELP,
	GuiMainMenuOptions_QUIT,
};

struct GuiMainMenuData {
	enum GuiMainMenuOptions active_button;
};

union GuiMenuData {
	struct GuiMainMenuData main_menu;
};

struct Gui {
	int window_w, window_h;
	int viewport_w, viewport_h;

	enum GuiMenu menu;

	union GuiMenuData data;
};

void gui_run(
	struct mgb* self
);

void gui_render(
	struct mgb* self
);

void gui_set_window_size(
    struct mgb* self, int w, int h
);

void gui_set_viewport_size(
    struct mgb* self, int w, int h
);

bool gui_on_mouse_button(
	struct mgb* self,
    enum VideoInterfaceMouseButton button, int x, int y, bool down
);

bool gui_on_mouse_motion(
	struct mgb* self,
    int x, int y, int xrel, int yrel
);

bool gui_on_key(
	struct mgb* self,
    enum VideoInterfaceKey key, uint8_t mod, bool down
);

bool gui_on_button(
	struct mgb* self,
    enum VideoInterfaceButton button, bool down
);

bool gui_on_axis(
	struct mgb* self,
    enum VideoInterfaceAxis axis, int16_t pos, bool down
);

#ifdef __cplusplus
}
#endif
