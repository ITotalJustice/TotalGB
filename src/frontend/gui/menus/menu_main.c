#include "menu.h"
#include "../../mgb.h"

// seems that nuklear arrow keys are only for text...
// this means that i will have to manually-ish navigate up / down
// options by calling [input_is_key_pressed] and then changing the index
// this isn't so bad, though it does mean more work and tracking
// (and many future bug) then i wouldve liked


static void button_on_loadrom(
	struct mgb* self, struct nk_context* ctx
) {
	STUB(ctx);

	change_menu(self, GuiMenu_LOADROM);
}

static void button_on_core_settings(
	struct mgb* self, struct nk_context* ctx
) {
	STUB(ctx);

	change_menu(self, GuiMenu_CORE_SETTINGS);
}

static void button_on_gui_settings(
	struct mgb* self, struct nk_context* ctx
) {
	STUB(ctx);

	change_menu(self, GuiMenu_GUI_SETTINGS);
}

static void button_on_help(
	struct mgb* self, struct nk_context* ctx
) {
	STUB(ctx);

	change_menu(self, GuiMenu_HELP);
}

static void button_on_quit(
	struct mgb* self, struct nk_context* ctx
) {
	STUB(ctx);

	self->running = false;
}

void draw_menu_main(
	struct mgb* self, struct nk_context* ctx
) {
    nk_begin(
    	ctx, "TotalGB - Main Menu",
    	nk_rect(0, 0, self->gui.window_w, self->gui.window_h),
      	NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR
    );

    nk_layout_row_dynamic(ctx, 46, 1);

	static const struct Button buttons[] = {
		[GuiMainMenuOptions_LOADROM] = {
			.title = "Loadrom",
			.callback = button_on_loadrom
		},
		[GuiMainMenuOptions_CORE_SETTINGS] = {
			.title = "Core Settings",
			.callback = button_on_core_settings
		},
		[GuiMainMenuOptions_GUI_SETTINGS] = {
			.title = "Gui Settings",
			.callback = button_on_gui_settings
		},
		[GuiMainMenuOptions_HELP] = {
			.title = "Help",
			.callback = button_on_help
		},
		[GuiMainMenuOptions_QUIT] = {
			.title = "Quit",
			.callback = button_on_quit
		},
	};

	for (size_t i = 0; i < ARRAY_SIZE(buttons); ++i) {
		draw_menu_button(self, ctx, &buttons[i], false);
	}

    nk_end(ctx);
}
