#include "gui.h"

#include <stdio.h>


static void nuklear_example(struct nk_context* ctx) {
    if (nk_begin(ctx, "Demo", nk_rect(50, 50, 230, 250),
        NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
        NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
    {
        enum {EASY, HARD};
        static int op = EASY;
        static int property = 20;
        static struct nk_colorf bg = {0};

        nk_layout_row_static(ctx, 30, 80, 1);
        if (nk_button_label(ctx, "button"))
            fprintf(stdout, "button pressed\n");

        nk_layout_row_dynamic(ctx, 30, 2);
        if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
        if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;
        nk_layout_row_dynamic(ctx, 25, 1);
        nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);

        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "background:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 25, 1);

        if (nk_combo_begin_color(ctx, nk_rgb_cf(bg), nk_vec2(nk_widget_width(ctx),400))) {
            nk_layout_row_dynamic(ctx, 120, 1);
            bg = nk_color_picker(ctx, bg, NK_RGBA);
            nk_layout_row_dynamic(ctx, 25, 1);
            bg.r = nk_propertyf(ctx, "#R:", 0, bg.r, 1.0f, 0.01f,0.005f);
            bg.g = nk_propertyf(ctx, "#G:", 0, bg.g, 1.0f, 0.01f,0.005f);
            bg.b = nk_propertyf(ctx, "#B:", 0, bg.b, 1.0f, 0.01f,0.005f);
            bg.a = nk_propertyf(ctx, "#A:", 0, bg.a, 1.0f, 0.01f,0.005f);
            nk_combo_end(ctx);
        }
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
	printf("mouse button down at x: %d y: %d\n", x, y);
	
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
