#include "interface.h"

#include <stdlib.h>
#include <assert.h>

// these are just wrappers for the struct's data.
// this is so that it makes the caller code much cleaner
// and won't break on changing the struct details, or making
// the struct opaque (which i think i might do).


void video_interface_quit(
    struct VideoInterface* self
) {
    assert(self->quit);
    self->quit(self->_private);
    free(self);
}

void video_interface_render_begin(
    struct VideoInterface* self
) {
    assert(self->render_begin);
    self->render_begin(self->_private);
}

void video_interface_render_game(
    struct VideoInterface* self
) {
    assert(self->render_game);
    self->render_game(self->_private);
}

void video_interface_render_end(
    struct VideoInterface* self
) {
    assert(self->render_end);
    self->render_end(self->_private);
}

void video_interface_update_game_texture(
    struct VideoInterface* self,
    const struct VideoInterfaceGameTexture* game_texture
) {
    assert(self->update_game_texture);
    self->update_game_texture(self->_private, game_texture);
}

void video_interface_poll_events(
    struct VideoInterface* self
) {
    assert(self->poll_events);
    self->poll_events(self->_private);
}

void video_interface_toggle_fullscreen(
    struct VideoInterface* self
) {
    assert(self->toggle_fullscreen);
    self->toggle_fullscreen(self->_private);
}

void video_interface_set_window_name(
    struct VideoInterface* self,
    const char* name
) {
    assert(self->set_window_name);
    self->set_window_name(self->_private, name);
}
