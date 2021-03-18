#include "gb.h"
#include "internal.h"

static inline GB_BOOL GB_is_directional(const struct GB_Data* gb) {
    return !!(IO_JYP & 0x10);
}

static inline GB_BOOL GB_is_button(const struct GB_Data* gb) {
    return !!(IO_JYP & 0x20);
}

static inline GB_U8 GB_joypad_get_internal(const struct GB_Data* gb) {
    return ((gb->joypad.var >> (GB_is_button(gb) << 2)) & 0xF);
}

void GB_set_buttons(struct GB_Data* gb, GB_U8 buttons, GB_BOOL is_down) {
    gb->joypad.var = !is_down ? gb->joypad.var | buttons : gb->joypad.var & (~buttons);
}

GB_U8 GB_get_buttons(const struct GB_Data* gb) {
    return gb->joypad.var;
}

GB_BOOL GB_is_button_down(const struct GB_Data* gb, enum GB_Button button) {
    return (gb->joypad.var & button) > 0;
}

GB_U8 GB_joypad_get(const struct GB_Data* gb) {
    return (0xC0 | (IO_JYP & 0x30) | ((gb->joypad.var >> (GB_is_button(gb) << 2)) & 0xF));
}
