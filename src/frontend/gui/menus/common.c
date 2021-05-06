#include "menu.h"
#include "../../mgb.h"


void change_menu(
	struct mgb* self, enum GuiMenu menu
) {
	self->gui.menu = menu;
}

void draw_menu_button(
	struct mgb* self, struct nk_context* ctx,
	const struct Button* button, bool active
) {
	STUB(active);

	if (nk_button_label(ctx, button->title)) {
		button->callback(self, ctx);
    }
}
