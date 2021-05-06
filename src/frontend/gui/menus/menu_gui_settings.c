#include "menu.h"
#include "../../mgb.h"


void draw_menu_gui_settings(
	struct mgb* self, struct nk_context* ctx
) {
	nk_begin(
    	ctx, "TotalGB - Gui Settings Menu",
    	nk_rect(0, 0, self->gui.window_w, self->gui.window_h),
      	NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR
    );

	nk_end(ctx);	
}
