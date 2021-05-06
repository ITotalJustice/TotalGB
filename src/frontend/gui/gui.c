#include "gui.h"

#include <stdio.h>


static void nuklear_example(struct nk_context* ctx) {
    if (nk_begin(ctx, "Demo", nk_rect(0, 0, 160*2, 144*2),
      	0))
    {
        enum {EASY, HARD};
        static int op = EASY;
        static int property = 20;

        // seems that nuklear arrow keys are only for text...
        // this means that i will have to manually-ish navigate up / down
        // options by calling [input_is_key_pressed] and then changing the index
        // this isn't so bad, though it does mean more work and tracking
        // (and many future bug) then i wouldve liked

        nk_layout_row_static(ctx, 30, 80, 1);
        if (nk_button_label(ctx, "button"))
            fprintf(stdout, "button pressed\n");

        nk_layout_row_dynamic(ctx, 30, 2);
        if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
        if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;
        nk_layout_row_dynamic(ctx, 25, 1);
        nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);
    }
    nk_end(ctx);
}

void gui_run(mgb_t* self) {
	(void)self;
}

void gui_render(mgb_t* self) {
	struct nk_context* ctx = nk_interface_get_context(self->nk_interface);

	nuklear_example(ctx);

	nk_interface_render(self->nk_interface, NK_ANTI_ALIASING_ON);
}

bool gui_on_mouse_button(mgb_t* self,
    enum VideoInterfaceMouseButton button, int x, int y, bool down
) {
	return nk_interface_on_mouse_button(self->nk_interface,
		button, x, y, down
	);
}

bool gui_on_mouse_motion(mgb_t* self,
    int x, int y, int xrel, int yrel
) {
	return nk_interface_on_mouse_motion(self->nk_interface,
		x, y, xrel, yrel
	);
}

bool gui_on_key(mgb_t* self,
    enum VideoInterfaceKey key, uint8_t mod, bool down
) {
	return nk_interface_on_key(self->nk_interface,
		key, mod, down
	);
}

bool gui_on_button(mgb_t* self,
    enum VideoInterfaceButton button, bool down
) {
	return nk_interface_on_button(self->nk_interface,
		button, down
	);
}

bool gui_on_axis(mgb_t* self,
    enum VideoInterfaceAxis axis, int16_t pos, bool down
) {
	return nk_interface_on_axis(self->nk_interface,
		axis, pos, down
	);
}
