#include "interface.h"

#include <stdlib.h>


void nk_interface_quit(
    struct NkInterface* self
) {
	self->quit(self->_private);
	free(self);
}

bool nk_interface_handle_event(
    struct NkInterface* self
) {
	return self->event(self->_private);
}

void nk_interface_render(
    struct NkInterface* self, enum nk_anti_aliasing AA
) {
	self->render(self->_private, AA);
}
