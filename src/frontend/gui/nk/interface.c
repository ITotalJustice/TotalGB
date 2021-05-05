#include "interface.h"

#include <stdio.h>
#include <stdlib.h>


static const uint8_t MOUSE_BUTTON_MAP[VideoInterfaceMouseButton_MAX] = {
    [VideoInterfaceMouseButton_LEFT]	= NK_BUTTON_LEFT,
    [VideoInterfaceMouseButton_MIDDLE]	= NK_BUTTON_MIDDLE,
    [VideoInterfaceMouseButton_RIGHT]	= NK_BUTTON_RIGHT,
};

static const uint8_t BUTTON_MAP[VideoInterfaceButton_MAX] = {
    [VideoInterfaceButton_A]		= NK_KEY_ENTER,
    [VideoInterfaceButton_START]	= NK_KEY_ENTER,
    [VideoInterfaceButton_B]		= NK_KEY_BACKSPACE,
    [VideoInterfaceButton_SELECT]	= NK_KEY_BACKSPACE,
    // NK_KEY_COPY,
    // NK_KEY_CUT,
    // NK_KEY_PASTE,
    [VideoInterfaceButton_UP]		= NK_KEY_UP,
    [VideoInterfaceButton_DOWN]		= NK_KEY_DOWN,
    [VideoInterfaceButton_LEFT]		= NK_KEY_LEFT,
    [VideoInterfaceButton_RIGHT]	= NK_KEY_RIGHT,
};

static const uint8_t KEY_MAP[VideoInterfaceKey_MAX] = {
    [VideoInterfaceKey_LSHIFT]		= NK_KEY_SHIFT,
    [VideoInterfaceKey_RSHIFT]		= NK_KEY_SHIFT,
    [VideoInterfaceKey_LCTRL]		= NK_KEY_CTRL,
    [VideoInterfaceKey_RCTRL]		= NK_KEY_CTRL,
    [VideoInterfaceKey_DELETE]		= NK_KEY_DEL,
    [VideoInterfaceKey_ENTER]		= NK_KEY_ENTER,
    [VideoInterfaceKey_TAB]			= NK_KEY_TAB,
    [VideoInterfaceKey_BACKSPACE]	= NK_KEY_BACKSPACE,
    // NK_KEY_COPY,
    // NK_KEY_CUT,
    // NK_KEY_PASTE,
    [VideoInterfaceKey_UP]			= NK_KEY_UP,
    [VideoInterfaceKey_DOWN]		= NK_KEY_DOWN,
    [VideoInterfaceKey_LEFT]		= NK_KEY_LEFT,
    [VideoInterfaceKey_RIGHT]		= NK_KEY_RIGHT,
};


struct nk_context* nk_interface_get_context(
	struct NkInterface* self
) {
	return self->get_context(self->_private);
}

void nk_interface_quit(
    struct NkInterface* self
) {
	self->quit(self->_private);
	free(self);
}

void nk_interface_input_begin(
	struct NkInterface* self
) {
	struct nk_context* ctx = nk_interface_get_context(self);

	nk_input_begin(ctx);
}

void nk_interface_input_end(
	struct NkInterface* self
) {
	struct nk_context* ctx = nk_interface_get_context(self);

	nk_input_end(ctx);
}

bool nk_interface_on_mouse_button(struct NkInterface* self,
    enum VideoInterfaceMouseButton button, int x, int y, bool down
) {
	struct nk_context* ctx = nk_interface_get_context(self);

	if (MOUSE_BUTTON_MAP[button]) {
		printf("got mouse btton, its being handled\n");
		nk_input_button(ctx, MOUSE_BUTTON_MAP[button], x, y, down);
		return true;
	}
	else {
		printf("invalid mouse button %d\n", button);
		return false;
	}
}

bool nk_interface_on_mouse_motion(struct NkInterface* self,
    int x, int y, int xrel, int yrel
) {
	struct nk_context* ctx = nk_interface_get_context(self);

	(void)xrel; (void)yrel;

	// todo: handle grap
	nk_input_motion(ctx, x, y);

	return true; // always handle?
}

bool nk_interface_on_key(struct NkInterface* self,
    enum VideoInterfaceKey key, uint8_t mod, bool down
) {
	struct nk_context* ctx = nk_interface_get_context(self);

	(void)mod;

	// todo: handle controller mod
	if (KEY_MAP[key]) {
		nk_input_key(ctx, KEY_MAP[key], down);
		return true;
	}
	else {
		return false;
	}
}

bool nk_interface_on_button(struct NkInterface* self,
    enum VideoInterfaceButton button, bool down
) {
	struct nk_context* ctx = nk_interface_get_context(self);

	if (BUTTON_MAP[button]) {
		nk_input_key(ctx, BUTTON_MAP[button], down);
		return true;
	}
	else {
		return false;
	}
	(void)ctx; (void)button; (void)down;
}

bool nk_interface_on_axis(struct NkInterface* self,
    enum VideoInterfaceAxis axis, int16_t pos, bool down
) {
	struct nk_context* ctx = nk_interface_get_context(self);

	(void)ctx; (void)axis; (void)pos; (void)down;

	return false;
}

void nk_interface_render(
    struct NkInterface* self, enum nk_anti_aliasing AA
) {
	self->render(self->_private, AA);
}
