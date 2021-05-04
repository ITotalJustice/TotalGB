#include "interface.h"


// these are just wrappers for the struct's data.
// this is so that it makes the caller code much cleaner
// and won't break on changing the struct details, or making
// the struct opaque (which i think i might do).


void video_interface_quit(
    struct VideoInterface* self
) {
    self->quit(self->_private);
}

void video_interface_render_begin(
    struct VideoInterface* self
) {
    self->render_begin(self->_private);
}

void video_interface_render_game(
    struct VideoInterface* self
) {
    self->render_game(self->_private);
}

void video_interface_render_end(
    struct VideoInterface* self
) {
    self->render_end(self->_private);
}

void video_interface_update_game_texture(
    struct VideoInterface* self,
    const struct VideoInterfaceGameTexture* game_texture
) {
    self->update_game_texture(self->_private, game_texture);
}

void video_interface_poll_events(
    struct VideoInterface* self
) {
    self->poll_events(self->_private);
}

void video_interface_toggle_fullscreen(
    struct VideoInterface* self
) {
    self->toggle_fullscreen(self->_private);
}

void video_interface_set_window_name(
    struct VideoInterface* self,
    const char* name
) {
    self->set_window_name(self->_private, name);
}

void video_interface_push_text_popup(
    struct VideoInterface* self,
    const char* text
) {
    (void)self; (void)text;
}

void video_interface_clear_text_popups(
    struct VideoInterface* self
) {
    (void)self;
}

void video_interface_get_text_popups_count(
    const struct VideoInterface* self
) {
    (void)self;
}

void video_interface_get_dimensions(
    const struct VideoInterface* self,
    int* x, int* y, int* w, int* h
) {
    if (x) { *x = self->x; }
    if (y) { *y = self->y; }
    if (w) { *w = self->w; }
    if (h) { *h = self->h; }
}

bool video_interface_has_file_dialog(
    const struct VideoInterface* self
) {
    return self->has_file_dialog;
}

bool video_interface_has_message_box(
    const struct VideoInterface* self
) {
    return self->has_message_box;
}

bool video_interface_has_controller(
    const struct VideoInterface* self
) {
    return self->has_controller;
}

bool video_interface_has_keyboard(
    const struct VideoInterface* self
) {
    return self->has_keyboard;
}

bool video_interface_has_mouse(
    const struct VideoInterface* self
) {
    return self->has_mouse;
}

bool video_interface_is_vsync(
    const struct VideoInterface* self
) {
    return self->is_vsync;
}

bool video_interface_is_hw_accel(
    const struct VideoInterface* self
) {
    return self->is_hw_accel;
}
