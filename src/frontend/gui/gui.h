#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "../mgb.h"


void gui_run(mgb_t* self);

void gui_render(mgb_t* self);

bool gui_on_mouse_button(mgb_t* self,
    enum VideoInterfaceMouseButton button, int x, int y, bool down
);

bool gui_on_mouse_motion(mgb_t* self,
    int x, int y, int xrel, int yrel
);

bool gui_on_key(mgb_t* self,
    enum VideoInterfaceKey key, uint8_t mod, bool down
);

bool gui_on_button(mgb_t* self,
    enum VideoInterfaceButton button, bool down
);

bool gui_on_axis(mgb_t* self,
    enum VideoInterfaceAxis axis, int16_t pos, bool down
);

#ifdef __cplusplus
}
#endif
